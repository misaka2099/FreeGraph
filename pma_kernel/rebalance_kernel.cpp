#include "data_type.h"
#include "common_function.h"
#include <cstdio>
#include <cstring>
#include <hls_stream.h>
#include <ap_utils.h>
#include "hls_burst_maxi.h"
#include "hls_print.h"

#ifndef REBALANCE_WORKER_NUM
#define REBALANCE_WORKER_NUM 2
#endif

// =========================================================================
// bitmap: stored in same HBM channel, 1 bit per 1024B block
// =========================================================================
static int bitmap_alloc(ap_uint<512>* hbm_ptr, int bitmap_base,
                        int size_bytes)
{
    #pragma HLS INLINE off
    int blocks_needed = (size_bytes + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
    int total_blocks = (int)(CHANNEL_CAPACITY / INIT_VEC_LEN_IN_BYTE);
    int bitmap_word_base = bitmap_base / 64;
    int run_start = -1, run_len = 0;
    for (int b = 0; b < total_blocks; b++) {
        #pragma HLS PIPELINE II=1
        int wi = bitmap_word_base + b / 512;
        int bi = b % 512;
        ap_uint<512> word = hbm_ptr[wi];
        if (!word[bi]) {
            if (run_len == 0) run_start = b;
            run_len++;
            if (run_len >= blocks_needed) {
                for (int rb = run_start; rb < run_start + blocks_needed; rb++) {
                    #pragma HLS PIPELINE II=1
                    int rw = bitmap_word_base + rb / 512;
                    int rbi = rb % 512;
                    ap_uint<512> ww = hbm_ptr[rw];
                    ww[rbi] = 1;
                    hbm_ptr[rw] = ww;
                }
                return run_start * INIT_VEC_LEN_IN_BYTE;
            }
        } else {
            run_len = 0;
        }
    }
    return -1;
}

static void bitmap_free(ap_uint<512>* hbm_ptr, int bitmap_base,
                        int offset_bytes, int size_bytes)
{
    #pragma HLS INLINE off
    int bitmap_word_base = bitmap_base / 64;
    int start_block = offset_bytes / INIT_VEC_LEN_IN_BYTE;
    int blocks = (size_bytes + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
    for (int b = start_block; b < start_block + blocks; b++) {
        #pragma HLS PIPELINE II=1
        int wi = bitmap_word_base + b / 512;
        int bi = b % 512;
        ap_uint<512> word = hbm_ptr[wi];
        word[bi] = 0;
        hbm_ptr[wi] = word;
    }
}

// =========================================================================
// enlarge: alloc new space, move data around enlarged vertex, free old.
// Uses old_addr from request (vecinfo is -1, can't read HBM).
// Does NOT write vecinfo — caller clears LOCK after redistribute.
// =========================================================================
static bool enlarge_vertex(
    PMA_HEADER& header, int local_vertex,
    ap_uint<512>* hbm_ptr, int bitmap_base,
    VEC_INFO old_addr,
    int& out_pma_start)
{
    #pragma HLS INLINE off
    int old_start = (int)old_addr.range(31, 0);
    int ow = old_start / 64;

    // 1. Record old vertex_range
    ap_uint<512> old_vr = 0;
    for (int i = 0; i < 16; i++)
        old_vr.range(32*(i+1)-1, 32*i) = (ap_uint<32>)header.vertex_range[i];

    // 2. Compute new vertex_range: enlarge local_vertex by 2x
    int old_v_start = (local_vertex == 0) ? 0 : (int)old_vr.range(32*local_vertex-1, 32*(local_vertex-1));
    int old_v_end   = (int)old_vr.range(32*(local_vertex+1)-1, 32*local_vertex);
    int old_seg_num = old_v_end - old_v_start;
    int new_seg_num = old_seg_num * 2;
    if (new_seg_num < 2) new_seg_num = 2;
    int seg_delta = new_seg_num - old_seg_num;

    ap_uint<512> new_vr = old_vr;
    for (int v = local_vertex; v < VERTEX_PER_PMA; v++) {
        int cur = (int)old_vr.range(32*(v+1)-1, 32*v);
        new_vr.range(32*(v+1)-1, 32*v) = (ap_uint<32>)(cur + seg_delta);
    }

    // 3. Update header.vertex_range with new values
    for (int v = local_vertex; v < VERTEX_PER_PMA; v++) {
        #pragma HLS UNROLL
        header.vertex_range[v] = (int)new_vr.range(32*(v+1)-1, 32*v);
    }

    int old_total_segs = (int)old_vr.range(32*VERTEX_PER_PMA-1, 32*(VERTEX_PER_PMA-1));
    int new_total_segs = old_total_segs + seg_delta;
    int old_size = (int)sizeof(PMA_HEADER) + old_total_segs * SEG_BYTES;
    int new_size = (int)sizeof(PMA_HEADER) + new_total_segs * SEG_BYTES;

    // 4. Alloc
    int new_addr = bitmap_alloc(hbm_ptr, bitmap_base, new_size);
    if (new_addr < 0) return false;
    out_pma_start = new_addr;
    int nw = new_addr / 64;

    // 5. Write header with updated vertex_range
    ap_uint<512> hdr[HEADER_WORDS] = {};
    for (int i = 0; i < 16; i++) {
        hdr[0].range(32*(i+1)-1, 32*i) = (ap_uint<32>)header.vertex_range[i];
        hdr[1].range(32*(i+1)-1, 32*i) = (ap_uint<32>)header.edge_count[i];
    }
    for (int j = 0; j < HEADER_WORDS; j++) hbm_ptr[nw + j] = hdr[j];

    // 6. Move: copy segments by old_vr / new_vr, fill empty slots for enlarged vertex
    ap_uint<512> es = 0;
    for (int k = 0; k < SEG_INT_SIZE; k++)
        es.range(32*(k+1)-1, 32*k) = (ap_uint<32>)UNUSED_ICON;

    // copy all old segments to new positions according to new_vr
    for (int s = 0; s < old_total_segs; s++) {
        #pragma HLS PIPELINE II=1
        int ds = s;
        if (s >= old_v_end) ds = s + seg_delta;
        hbm_ptr[nw + HEADER_WORDS + ds] = hbm_ptr[ow + HEADER_WORDS + s];
    }
    // fill new empty segments for enlarged vertex
    for (int s = 0; s < seg_delta; s++) {
        #pragma HLS PIPELINE II=1
        hbm_ptr[nw + HEADER_WORDS + old_v_end + s] = es;
    }

    // 7. Free
    bitmap_free(hbm_ptr, bitmap_base, old_start, old_size);
    return true;
}

// =========================================================================
// worker unit
// =========================================================================
static void rebalance_worker(
    hls::stream<REBALANCE_REQ>& work_in,
    ap_uint<512>* hbm_ptr,
    int bitmap_base,
    int vec_info_base,
    int worker_id)
{
    #pragma HLS INLINE off

    while (true) {
        if (work_in.empty()) { ap_wait(); continue; }
        REBALANCE_REQ req = work_in.read();

        if (req.flags == exit_flag) break;

        int pma_idx = req.src >> 4;
        int local_vertex = req.src & 0xF;

        // vecinfo table starts at vec_info_base + 64 (first 64B = max_pma_idx)
        int vi_table = vec_info_base + 64;
        int vi_word = vi_table / 64 + pma_idx / 8;
        int vi_shift = (pma_idx % 8) * 64;

        // Read CURRENT vecinfo from HBM.  Compare lengths: if the PMA has
        // been enlarged since this request was created, skip — the issue is
        // already resolved.  If the lengths match, the PMA hasn't changed
        // and we need to process this rebalance.
        ap_uint<512> vi_pkt = hbm_ptr[vi_word];
        VEC_INFO current_vi = vi_pkt.range(vi_shift + 63, vi_shift);
        int pma_start;
        if (current_vi == 0 || current_vi == (ap_uint<64>)(-1)) {
            // Empty or locked — use old_addr from request.
            pma_start = (int)req.old_addr.range(31, 0);
        } else {
            int current_len = (int)current_vi.range(63, 32);
            int old_len     = (int)req.old_addr.range(63, 32);
            if (current_len > old_len) {
                // PMA was already enlarged → stale request, drop it.
                continue;
            }
            pma_start = (int)current_vi.range(31, 0);
        }

        // Fast path: empty PMA allocation
        if (pma_start == 0) {
            int alloc_size = (int)sizeof(PMA_HEADER) + VERTEX_PER_PMA * SEG_BYTES;
            int alloc_addr = bitmap_alloc(hbm_ptr, bitmap_base, alloc_size);
            if (alloc_addr < 0) {
                continue;
            }
            int aw = alloc_addr / 64;
            PMA_HEADER new_header;
            for (int v = 0; v < VERTEX_PER_PMA; v++) {
                new_header.vertex_range[v] = v + 1;
                new_header.edge_count[v] = 0;
            }
            ap_uint<512> hdr[HEADER_WORDS] = {};
            for (int i = 0; i < 16; i++) {
                hdr[0].range(32*(i+1)-1, 32*i) = (ap_uint<32>)new_header.vertex_range[i];
                hdr[1].range(32*(i+1)-1, 32*i) = (ap_uint<32>)new_header.edge_count[i];
            }
            for (int j = 0; j < HEADER_WORDS; j++) hbm_ptr[aw + j] = hdr[j];
            ap_uint<512> es = 0;
            for (int k = 0; k < SEG_INT_SIZE; k++)
                es.range(32*(k+1)-1, 32*k) = (ap_uint<32>)UNUSED_ICON;
            for (int s = 0; s < VERTEX_PER_PMA; s++) {
                #pragma HLS PIPELINE II=1
                hbm_ptr[aw + HEADER_WORDS + s] = es;
            }
            ap_uint<512> vi_pkt = hbm_ptr[vi_word];
            vi_pkt.range(vi_shift+63, vi_shift) = ((ap_uint<64>)alloc_size << 32) | (ap_uint<32>)alloc_addr;
            hbm_ptr[vi_word] = vi_pkt;

            // Update max_pma_idx if needed
            ap_uint<512> max_word = hbm_ptr[vec_info_base / 64];
            ap_uint<64> cur_max = max_word.range(63, 0);
            if ((ap_uint<64>)pma_idx > cur_max) {
                max_word.range(63, 0) = (ap_uint<64>)pma_idx;
                hbm_ptr[vec_info_base / 64] = max_word;
            }
            continue;
        }

        int hdr_word = pma_start / 64;

        PMA_HEADER header;
        {
            ap_uint<512> hdr[HEADER_WORDS];
            for (int j = 0; j < HEADER_WORDS; j++) {
                #pragma HLS PIPELINE II=1
                hdr[j] = hbm_ptr[hdr_word + j];
            }
            for (int i = 0; i < 16; i++) {
                #pragma HLS UNROLL
                header.vertex_range[i] = (int)hdr[0].range(32*(i+1)-1, 32*i);
                header.edge_count[i]  = (int)hdr[1].range(32*(i+1)-1, 32*i);
            }
        }

        int v_start = (local_vertex == 0) ? 0 : header.vertex_range[local_vertex - 1];
        int v_end = header.vertex_range[local_vertex];
        int seg_num = v_end - v_start;
        int seg_words = SEG_INT_SIZE * (int)sizeof(int) / 64;

        // ================================================================
        // Step 1: Enlarge check
        // ================================================================
        bool enlarged = false;
        if (header.edge_count[local_vertex] > seg_num * 12) {
            if (!enlarge_vertex(header, local_vertex, hbm_ptr, bitmap_base,
                                req.old_addr, pma_start)) {
                continue;
            }
            enlarged = true;
            // Re-read after enlarge
            hdr_word = (pma_start) / 64;
            {
                ap_uint<512> hdr[HEADER_WORDS];
                for (int j = 0; j < HEADER_WORDS; j++) hdr[j] = hbm_ptr[hdr_word + j];
                for (int i = 0; i < 16; i++) {
                    header.vertex_range[i] = (int)hdr[0].range(32*(i+1)-1, 32*i);
                    header.edge_count[i]  = (int)hdr[1].range(32*(i+1)-1, 32*i);
                }
            }
            v_start = (local_vertex == 0) ? 0 : header.vertex_range[local_vertex - 1];
            v_end = header.vertex_range[local_vertex];
            seg_num = v_end - v_start;
        }

        // ================================================================
        // Step 2: Find minimal rebalance range
        //   If enlarged: redistribute entire vertex.
        //   Otherwise: scan for a full segment, binary-expand to find the
        //   smallest power-of-2 range where total valid <= range capacity.
        // ================================================================
        int rb_start = v_start;
        int rb_end   = v_end;

        if (!enlarged) {
            // Binary expansion from req.seg_idx (the full segment).
            // Expand left/right by powers of 2 until valid_count fits.
            int left = req.seg_idx;
            int right = req.seg_idx + 1;
            int total_vc = SEG_INT_SIZE;  // the full segment is full

            for (int sz = 1; left > v_start || right < v_end; sz *= 2) {
                int capacity = (right - left) * 12;
                if (total_vc <= capacity) break;

                if (left > v_start) {
                    int new_left = (left - sz >= v_start) ? left - sz : v_start;
                    for (int s = new_left; s < left; s++) {
                        #pragma HLS PIPELINE II=1
                        ap_uint<512> seg = hbm_ptr[hdr_word + HEADER_WORDS + s * seg_words];
                        for (int k = 0; k < SEG_INT_SIZE; k++) {
                            #pragma HLS UNROLL
                            if (seg.range(32*(k+1)-1, 32*k) != (ap_uint<32>)UNUSED_ICON)
                                total_vc++;
                        }
                    }
                    left = new_left;
                }
                if (right < v_end && total_vc > (right - left) * SEG_INT_SIZE) {
                    int new_right = (right + sz <= v_end) ? right + sz : v_end;
                    for (int s = right; s < new_right; s++) {
                        #pragma HLS PIPELINE II=1
                        ap_uint<512> seg = hbm_ptr[hdr_word + HEADER_WORDS + s * seg_words];
                        for (int k = 0; k < SEG_INT_SIZE; k++) {
                            #pragma HLS UNROLL
                            if (seg.range(32*(k+1)-1, 32*k) != (ap_uint<32>)UNUSED_ICON)
                                total_vc++;
                        }
                    }
                    right = new_right;
                }
            }
            rb_start = left;
            rb_end = right;
        }

        // ================================================================
        // Step 3: Redistribute — collect edges in [rb_start, rb_end),
        //         then distribute evenly across the range.
        // ================================================================
        {
            int tmp[SEG_INT_SIZE * 8192];
            #pragma HLS BIND_STORAGE variable=tmp type=ram_2p impl=BRAM
            int total = 0;
            int range_segs = rb_end - rb_start;

            for (int s = rb_start; s < rb_end; s++) {
                #pragma HLS PIPELINE II=1
                ap_uint<512> seg = hbm_ptr[hdr_word + HEADER_WORDS + s * seg_words];
                for (int k = 0; k < SEG_INT_SIZE; k++) {
                    #pragma HLS UNROLL
                    ap_uint<32> val = seg.range(32*(k+1)-1, 32*k);
                    if (val != (ap_uint<32>)UNUSED_ICON && total < SEG_INT_SIZE * 8192)
                        tmp[total++] = (int)val;
                }
            }

            int base = total / range_segs;
            int extra = total % range_segs;
            int pos = 0;
            for (int w = rb_start; w < rb_end; w++) {
                int fill = base + ((w - rb_start) < extra ? 1 : 0);
                ap_uint<512> wseg = 0;
                for (int k = 0; k < SEG_INT_SIZE; k++) {
                    #pragma HLS UNROLL
                    if (k < fill && pos < total)
                        wseg.range(32*(k+1)-1, 32*k) = (ap_uint<32>)tmp[pos++];
                    else
                        wseg.range(32*(k+1)-1, 32*k) = (ap_uint<32>)UNUSED_ICON;
                }
                hbm_ptr[hdr_word + HEADER_WORDS + w * seg_words] = wseg;
            }
        }

        // Write header
        {
            ap_uint<512> hdr[HEADER_WORDS] = {};
            for (int i = 0; i < 16; i++) {
                hdr[0].range(32*(i+1)-1, 32*i) = (ap_uint<32>)header.vertex_range[i];
                hdr[1].range(32*(i+1)-1, 32*i) = (ap_uint<32>)header.edge_count[i];
            }
            for (int j = 0; j < HEADER_WORDS; j++) hbm_ptr[hdr_word + j] = hdr[j];
        }

        // Clear LOCKED
        {
            int new_total_segs = header.vertex_range[VERTEX_PER_PMA - 1];
            int new_len = (int)sizeof(PMA_HEADER) + new_total_segs * SEG_BYTES;
            ap_uint<512> vi_pkt = hbm_ptr[vi_word];
            vi_pkt.range(vi_shift+63, vi_shift) = ((ap_uint<64>)new_len << 32) | (ap_uint<32>)pma_start;
            hbm_ptr[vi_word] = vi_pkt;
        }

    }
    printf("REBALANCE_WORKER[%d] DONE\n", worker_id);
}

// =========================================================================
// top-level
// =========================================================================
extern "C"
{
void rebalance_kernel(
    hls::stream<REBALANCE_REQ>& rebalance_req_stream,
    ap_uint<512>* hbm_ptr_0,
    ap_uint<512>* hbm_ptr_1,
    int bitmap_base,
    int vec_info_base)
{
    #pragma HLS INTERFACE axis port=rebalance_req_stream
    #pragma HLS INTERFACE m_axi port=hbm_ptr_0 offset=slave bundle=gmem0
    #pragma HLS INTERFACE m_axi port=hbm_ptr_1 offset=slave bundle=gmem1
    #pragma HLS INTERFACE s_axilite port=hbm_ptr_0
    #pragma HLS INTERFACE s_axilite port=hbm_ptr_1
    #pragma HLS INTERFACE s_axilite port=bitmap_base 
    #pragma HLS INTERFACE s_axilite port=vec_info_base 
    #pragma HLS INTERFACE s_axilite port=return
    #pragma HLS DATAFLOW

    hls::stream<REBALANCE_REQ> work_stream[REBALANCE_WORKER_NUM];
    #pragma HLS STREAM variable=work_stream depth=16

    dispatch<REBALANCE_REQ, REBALANCE_WORKER_NUM>(rebalance_req_stream, work_stream);

    rebalance_worker(work_stream[0], hbm_ptr_0, bitmap_base, vec_info_base, 0);
    rebalance_worker(work_stream[1], hbm_ptr_1, bitmap_base, vec_info_base, 1);
}
}
