#include "data_type.h"
#include <cstdio>
#include <cstring>
#include <cxxabi.h>
#include <hls_stream.h>
#include "debug.h"
#include <ap_utils.h>
#include "etc/autopilot_ssdm_op.h"
#include "hls_burst_maxi.h"
#include "hls_print.h"

#define BS_WORKER_NUM 4
#define HEADER_WORDS ((int)(sizeof(PMA_HEADER) / 64))
#define SEG_WORDS_PER_SEG (SEGMENT_SIZE * (int)sizeof(int) / 64)
#define HEADER_BYTES (HEADER_WORDS * 64)
#define SEG_BYTES (SEG_WORDS_PER_SEG * 64)

// ---- Cache (local to insert_phase) ----
#define CACHE_LINES 64
#define MAX_HELPER_WORDS 256

// File-scope helper: ALL_ONES and EMPTY_SEG with valid_count=0
static ap_uint<512> make_empty_seg(int word) {
    ap_uint<512> x = -1;
    // for (int b = 0; b < 512; b++) x[b] = 1;  // explicit all-ones
    if (word == 3) x.range(511, 480) = 0;
    return x;
}

// ---- HBM access helpers (m_axi) ----

static void hbm_read_words(ap_uint<512>* hbm_ptr, int byte_off, int words, ap_uint<512>* buf)
{
    int base = byte_off / 64;
    for (int w = 0; w < words; w++) {
        #pragma HLS PIPELINE II=1
        buf[w] = hbm_ptr[base + w];
    }
}

static void hbm_write_words(ap_uint<512>* hbm_ptr, int byte_off, int words, ap_uint<512>* buf)
{
    int base = byte_off / 64;
    for (int w = 0; w < words; w++) {
        #pragma HLS PIPELINE II=1
        hbm_ptr[base + w] = buf[w];
    }
}

// ---- VEC_INFO access ----

static VEC_INFO vi_read(ap_uint<512>* hbm, int vi_word_off, int pma_idx)
{
    int wi = pma_idx / 8, si = pma_idx % 8;
    ap_uint<512> w = hbm[vi_word_off + wi];
    VEC_INFO vi;
    vi.range(63, 0) = w.range(si * 64 + 63, si * 64);
    return vi;
}

static void vi_write(ap_uint<512>* hbm, int vi_word_off, int pma_idx, VEC_INFO vi)
{
    int wi = pma_idx / 8, si = pma_idx % 8;
    ap_uint<512> w = hbm[vi_word_off + wi];
    w.range(si * 64 + 63, si * 64) = vi.range(63, 0);
    hbm[vi_word_off + wi] = w;
}

// ---- Binary search helper ----

static inline int density_capacity(int seg_cnt) { return seg_cnt * 47; }

void sub_search(int* helper, int lo, int hi, int dst, int &result)
{
    #pragma HLS INLINE
    int low = lo, high = hi - 1;
    result = -1;
    while (low <= high) {
        #pragma HLS PIPELINE off
        int mid = (low + high) / 2;
        if (helper[mid] == dst) { result = mid - lo; return; }
        else if (helper[mid] < dst) { low = mid + 1; result = mid - lo; }
        else { high = mid - 1; }
    }
}

// ---- Phase 1: BS Worker ----
// Reads header+helper directly from HBM via m_axi, does binary search

void bs_worker(
    ap_uint<512>* hbm,
    int data_off, int vi_wo,
    hls::stream<PMA_UPDATE_MESSAGE>& edge_in,
    hls::stream<int>& cnt_in,
    hls::stream<pma_idx_info>& info_in,
    hls::stream<bs_batch_t>& bs_out)
{
    while (true) {
        auto sig = info_in.read();
        if (sig.command_type == INSERT_STOP) {
            bs_batch_t s; s.pma_idx = -1; s.count = 0; bs_out.write(s); break;
        }
        if (sig.command_type == INSERT_FLUSH_CACHE) {
            bs_batch_t f; f.pma_idx = -2; f.count = 0; bs_out.write(f); continue;
        }

        int pma = sig.pma_idx;
        int cnt = cnt_in.read();

        // Read VEC_INFO
        VEC_INFO vi = vi_read(hbm, vi_wo, pma);
        if (vi == 0) {
            // Empty PMA: 16 vertices × 1 segment each, vertex v = segment v
            // No binary search needed — target_seg = local_src
            bs_batch_t res;
            res.pma_idx = pma; res.op_mask = 0; res.count = cnt;
            for (int i = 0; i < cnt; i++) {
                #pragma HLS PIPELINE II=1
                PMA_UPDATE_MESSAGE e = edge_in.read();
                res.dst[i] = e.dst;
                res.local_src[i] = e.src & 0xF;
                res.target_seg[i] = e.src & 0xF;  // vertex v → segment v
            }
            bs_out.write(res);
            continue;
        }

        int start_byte = (int)vi.range(31, 0);
        int byte_off   = start_byte - data_off;

        // Read header + helper as one continuous burst (UG1399: merge adjacent reads)
        int hbase = byte_off / 64;
        int h_read_words = HEADER_WORDS + MAX_HELPER_WORDS;  // always read max to enable burst
        ap_uint<512> hdr[HEADER_WORDS];
        ap_uint<512> hlp_buf[MAX_HELPER_WORDS];
        #pragma HLS bind_storage variable=hlp_buf type=RAM_2P impl=BRAM
        for (int w = 0; w < h_read_words; w++) {
            #pragma HLS PIPELINE II=1
            ap_uint<512> d = hbm[hbase + w];
            if (w < HEADER_WORDS) hdr[w] = d;
            else                  hlp_buf[w - HEADER_WORDS] = d;
        }
        int total_segs = (int)hdr[0].range(15 * 32 + 31, 15 * 32);
        int hlp_words = (total_segs + HELPER_ENTRIES_PER_WORD - 1) / HELPER_ENTRIES_PER_WORD;
        if (hlp_words > MAX_HELPER_WORDS) hlp_words = MAX_HELPER_WORDS;

        // Unpack bs_helper
        int bs_helper[10240];
        #pragma HLS bind_storage variable=bs_helper type=RAM_2P impl=BRAM
        for (int w = 0; w < hlp_words; w++) {
            #pragma HLS PIPELINE II=1
            ap_uint<512> hw = hlp_buf[w];
            for (int s = 0; s < 8; s++) {
                int seg = w * 8 + s;
                if (seg < total_segs)
                    bs_helper[seg] = (int)hw.range(s * 64 + 31, s * 64);
            }
        }

        // Extract vertex_range from header
        int vr[VERTEX_PER_PMA];
        for (int v = 0; v < VERTEX_PER_PMA; v++)
            #pragma HLS UNROLL
            vr[v] = (int)hdr[0].range(v * 32 + 31, v * 32);

        // Process edges
        bs_batch_t res;
        res.pma_idx = pma; res.op_mask = 0; res.count = cnt;

        for (int i = 0; i < cnt; i++) {
            #pragma HLS PIPELINE II=1
            PMA_UPDATE_MESSAGE e = edge_in.read();
            res.dst[i] = e.dst;
            res.local_src[i] = e.src & 0xF;
            int op = (int)e.opcode;
            // Insert-only mode (delete edge commented out)
            if (op == PMA_UPDATE_INSERT_EDGE /* || op == PMA_UPDATE_DELETE_EDGE */) {
                int ls = e.src & 0xF;
                int lo = (ls == 0) ? 0 : vr[ls - 1];
                int hi = vr[ls];
                int rel = -1;
                sub_search(bs_helper, lo, hi, e.dst, rel);
                if (rel == -1) rel = 0;
                res.target_seg[i] = rel + lo;
                // if (op == PMA_UPDATE_DELETE_EDGE) res.op_mask[i] = 1;
            } else {
                res.target_seg[i] = 0;
            }
        }
        bs_out.write(res);
    }
}

// ---- Phase 2+3: Insert (segment modification + enlarge) ----
// Pushes modified segments to task_fifo for deferred redistribution
// ACKs are sent by redistribution_phase after all tasks are processed

void insert_phase(
    ap_uint<512>* hbm,
    int data_off, int vi_wo,
    hls::stream<bs_batch_t>& bs0, hls::stream<bs_batch_t>& bs1,
    hls::stream<bs_batch_t>& bs2, hls::stream<bs_batch_t>& bs3,
    hls::stream<ap_axiu<96,0,0,0>>& alloc_cmd_stream,
    hls::stream<ap_axiu<64,0,0,0>>& alloc_rsp_stream,
    hls::stream<ap_uint<64>>& task_fifo)
{
    // --- Local cache: header + helper ---
    ap_uint<512> ch[CACHE_LINES][HEADER_WORDS];
    #pragma HLS bind_storage variable=ch type=ram_2p impl=BRAM
    ap_uint<512> chp[CACHE_LINES][MAX_HELPER_WORDS];
    #pragma HLS bind_storage variable=chp type=ram_2p impl=URAM
    ap_uint<64>   cvi[CACHE_LINES];
    #pragma HLS bind_storage variable=cvi type=ram_1p impl=BRAM
    ap_uint<CACHE_LINES> cvalid = 0;
    int cpma[CACHE_LINES];
    int chpw[CACHE_LINES];
    int ctseg[CACHE_LINES];
    for (int i = 0; i < CACHE_LINES; i++) cpma[i] = -1;

    // Empty helper word: valid_count=0, first_edge=UNUSED_ICON for all 8 entries
    // (was all-1s which gave valid_count=-1 for empty segments, breaking rebalance)
    ap_uint<64> empty_h_entry = ((ap_uint<64>)(ap_uint<32>)0 << 32) | (ap_uint<64>)(ap_uint<32>)UNUSED_ICON;
    ap_uint<512> EMPTY_HELPER = 0;
    for (int z = 0; z < 8; z++)
        EMPTY_HELPER.range(z * 64 + 63, z * 64) = empty_h_entry;

    ap_uint<512> empty_vertex_range = 0;
    for (int v = 0; v < VERTEX_PER_PMA; v++)
        empty_vertex_range.range(v * 32 + 31, v * 32) = v + 1;

    int stop_cnt = 0, flush_cnt = 0;
    while (stop_cnt < BS_WORKER_NUM) {
        #pragma HLS PIPELINE II=1
        bs_batch_t b;
        bool got = false;
        if (!got && !bs0.empty()) { b = bs0.read(); got = true; }
        if (!got && !bs1.empty()) { b = bs1.read(); got = true; }
        if (!got && !bs2.empty()) { b = bs2.read(); got = true; }
        if (!got && !bs3.empty()) { b = bs3.read(); got = true; }
        if (!got) { ap_wait(); continue; }

        if (b.pma_idx == -1) { stop_cnt++; continue; }
        if (b.pma_idx == -2) { flush_cnt++; continue; }

        int pma = b.pma_idx;
        int line = ((pma % CACHE_LINES) + CACHE_LINES) % CACHE_LINES;

        // --- Load cache (init PMA if first access) ---
        bool hit = cvalid[line] && cpma[line] == pma;
        if (!hit) {
            cvalid[line] = 0;
            VEC_INFO vi = vi_read(hbm, vi_wo, pma);

            if (vi != 0) {
                // Existing PMA: load header + helper from HBM
                int sbyte = (int)vi.range(31, 0);
                int boff  = sbyte - data_off;
                int hbase = boff / 64;
                for (int w = 0; w < HEADER_WORDS; w++) {
                    #pragma HLS UNROLL
                    ch[line][w] = hbm[hbase + w];
                }
                int tseg = (int)ch[line][0].range(15 * 32 + 31, 15 * 32);
                int hpw  = (tseg + 7) / 8;
                if (hpw > 0) {
                    for (int w = 0; w < hpw; w++) {
                        #pragma HLS PIPELINE II=1
                        chp[line][w] = hbm[hbase + HEADER_WORDS + w];
                    }
                }
                cvi[line] = vi; cvalid[line] = 1; cpma[line] = pma;
                chpw[line] = hpw; ctseg[line] = tseg;
            } else {
                // Empty PMA (first access): alloc + init in HBM, then process edges below
                int isegs = 16, ihlp = (isegs + 7) / 8;
                int ibytes = (HEADER_WORDS + ihlp + isegs * SEG_WORDS_PER_SEG) * 64;
                ap_axiu<96,0,0,0> acmd;
                acmd.data.range(31,0)   = 0;        // start=0 for ALLOC
                acmd.data.range(63,32)  = ibytes;   // request bytes
                acmd.data.range(95,64)  = 1;        // opcode=ALLOC
                acmd.last = 0; acmd.keep = 0xFFF;
                alloc_cmd_stream.write(acmd);
                #pragma HLS protocol fixed
                ap_wait();
                ap_axiu<64,0,0,0> arsp = alloc_rsp_stream.read();
                int nstart = (int)arsp.data.range(31, 0);

                // Write header
                ap_uint<512> hdr[HEADER_WORDS];
                hdr[0] = empty_vertex_range; hdr[1] = 0; hdr[2] = 0; hdr[3] = 0;
                // Merged header+helper+segs into one continuous burst write (UG1399 coalesce)
                int nh_base = nstart / 64;
                int seg_off = HEADER_WORDS + ihlp;
                int total_wr = seg_off + isegs * SEG_WORDS_PER_SEG;  // 4 + 2 + 64 = 70
                for (int w = 0; w < total_wr; w++) {
                    #pragma HLS PIPELINE II=1
                    if (w < 4) {
                        hbm[nh_base + w] = hdr[w];
                    } else if (w < seg_off) {
                        hbm[nh_base + w] = EMPTY_HELPER;
                    } else {
                        hbm[nh_base + w] = make_empty_seg((w - seg_off) & 3);
                    }
                }

                VEC_INFO nvi = 0;
                nvi.range(31, 0) = (ap_uint<32>)(nstart + data_off);
                nvi.range(63, 32) = (ap_uint<32>)ibytes;
                vi_write(hbm, vi_wo, pma, nvi);

                // Fill cache
                for (int w = 0; w < HEADER_WORDS; w++) ch[line][w] = hdr[w];
                for (int w = 0; w < ihlp; w++) chp[line][w] = EMPTY_HELPER;
                cvi[line] = nvi; cvalid[line] = 1; cpma[line] = pma;
                chpw[line] = ihlp; ctseg[line] = isegs;
            }
        }

        int tseg  = ctseg[line];
        int hpw   = chpw[line];
        int sbyte = (int)cvi[line].range(31, 0);
        int boff  = sbyte - data_off;

        // Read edge_count + vertex_range from cache
        int ec[VERTEX_PER_PMA], vr[VERTEX_PER_PMA];
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            vr[v] = (int)ch[line][0].range(v * 32 + 31, v * 32);
            ec[v] = (int)ch[line][1].range(v * 32 + 31, v * 32);
        }

        // --- Process segment modifications ---
        int ec_delta[VERTEX_PER_PMA];
        #pragma HLS ARRAY_PARTITION variable=ec_delta complete
        ap_uint<16> ins_mask = 0;
        for (int v = 0; v < VERTEX_PER_PMA; v++) ec_delta[v] = 0;

        int seg_list[16], seg_cnt = 0;
        #pragma HLS ARRAY_PARTITION variable=seg_list complete
        bool seg_overflow[16];
        #pragma HLS ARRAY_PARTITION variable=seg_overflow complete
        int  seg_owner[16];
        #pragma HLS ARRAY_PARTITION variable=seg_owner complete
        for (int i = 0; i < 16; i++) { seg_overflow[i] = false; seg_owner[i] = -1; }

        for (int i = 0; i < b.count; i++) {
            #pragma HLS PIPELINE II=1
            int ts = b.target_seg[i], ls = b.local_src[i];
            // Insert-only mode (delete edge commented out)
            // bool del = b.op_mask[i];
            // if (del) ec_delta[ls]--;
            // else { ec_delta[ls]++; ins_mask[ls] = 1; }
            ec_delta[ls]++; ins_mask[ls] = 1;

            bool found = false;
            for (int j = 0; j < seg_cnt; j++) if (seg_list[j] == ts) { found = true; break; }
            if (!found && seg_cnt < 16) seg_list[seg_cnt++] = ts;
        }

        // --- Phase A: burst-read all modified segments (bursts pipelined between segments) ---
        ap_uint<512> seg_buf[16][SEG_WORDS_PER_SEG];
        #pragma HLS ARRAY_PARTITION variable=seg_buf complete dim=2
        int seg_bases[16];
        #pragma HLS ARRAY_PARTITION variable=seg_bases complete
        int seg_valid_buf[16];
        #pragma HLS ARRAY_PARTITION variable=seg_valid_buf complete

        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS PIPELINE II=1
            int seg = seg_list[si];
            int base = (boff + HEADER_BYTES + hpw * 64 + seg * SEG_BYTES) / 64;
            seg_bases[si] = base;
            for (int w = 0; w < SEG_WORDS_PER_SEG; w++) {
                #pragma HLS UNROLL
                seg_buf[si][w] = hbm[base + w];
            }
        }

        // --- Phase B: compute modifications on local buffers (pure BRAM, no m_axi) ---
        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS PIPELINE II=1
            int seg = seg_list[si];

            // Record owner vertex for O(1) enlarge-overlap check later
            for (int v = 0; v < VERTEX_PER_PMA; v++) {
                #pragma HLS UNROLL
                int vs = (v == 0) ? 0 : vr[v - 1];
                int ve = vr[v];
                if (seg >= vs && seg < ve) seg_owner[si] = v;
            }

            // Extract segment words + valid from buffer
            ap_uint<512> sw[SEG_WORDS_PER_SEG];
            for (int w = 0; w < SEG_WORDS_PER_SEG; w++) sw[w] = seg_buf[si][w];
            int valid = (int)sw[3].range(511, 480);

            // Extract existing edges into edge_buf
            int edge_buf[EDGE_SLOTS_PER_SEG];
            #pragma HLS ARRAY_PARTITION variable=edge_buf complete
            for (int k = 0; k < EDGE_SLOTS_PER_SEG; k++) {
                #pragma HLS UNROLL
                int wi = k / 16, slot = k % 16;
                edge_buf[k] = (k < valid) ? (int)sw[wi].range(slot * 32 + 31, slot * 32) : UNUSED_ICON;
            }

            // Collect edges in this batch targeting this segment
            int ed_list[16], ed_n = 0;
            #pragma HLS ARRAY_PARTITION variable=ed_list complete
            for (int i = 0; i < b.count; i++) {
                #pragma HLS UNROLL
                if (b.target_seg[i] == seg) ed_list[ed_n++] = i;
            }

            // Insert each new edge (int-by-int shift register)
            for (int ei = 0; ei < ed_n; ei++) {
                int dst = b.dst[ed_list[ei]];
                if (valid >= EDGE_SLOTS_PER_SEG) break;
                int pos = 0;
                for (int k = 0; k < valid; k++) {
                    #pragma HLS UNROLL
                    if (edge_buf[k] < dst) pos++;
                }
                for (int k = valid; k > pos; k--) edge_buf[k] = edge_buf[k - 1];
                edge_buf[pos] = dst;
                valid++;
            }

            // Record overflow
            seg_overflow[si] = (valid > 47);

            // Pack back into sw
            for (int k = 0; k < EDGE_SLOTS_PER_SEG; k++) {
                #pragma HLS UNROLL
                int wi = k / 16, slot = k % 16;
                int val = (k < valid) ? edge_buf[k] : UNUSED_ICON;
                sw[wi].range(slot * 32 + 31, slot * 32) = (ap_uint<32>)val;
            }
            sw[3].range(511, 480) = (ap_uint<32>)valid;

            // Store back to buffer
            for (int w = 0; w < SEG_WORDS_PER_SEG; w++) seg_buf[si][w] = sw[w];
            seg_valid_buf[si] = valid;
        }

        // --- Phase C: burst-write all modified segments + update helper cache ---
        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS PIPELINE II=1
            int base = seg_bases[si];
            int seg  = seg_list[si];
            for (int w = 0; w < SEG_WORDS_PER_SEG; w++) {
                #pragma HLS UNROLL
                hbm[base + w] = seg_buf[si][w];
            }
            // Update helper cache (alongside write, using local buffer data)
            int hw_idx = seg / 8, hw_slot = seg % 8;
            ap_uint<64> hv = 0;
            hv.range(31, 0)  = seg_buf[si][0].range(31, 0);
            hv.range(63, 32) = (ap_uint<32>)seg_valid_buf[si];
            chp[line][hw_idx].range(hw_slot * 64 + 63, hw_slot * 64) = hv;
        }

        // Write-back helper to HBM

        // Write-back helper to HBM
        int hlp_base = (boff + HEADER_BYTES) / 64;
        for (int w = 0; w < hpw; w++) {
            #pragma HLS PIPELINE II=1
            hbm[hlp_base + w] = chp[line][w];
        }

        // Update edge_count in cache
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            ec[v] += ec_delta[v];
            ch[line][1].range(v * 32 + 31, v * 32) = (ap_uint<32>)ec[v];
        }

        // --- Density check + enlarge ---
        bool enlarged = false;
        int esegs[VERTEX_PER_PMA];
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            esegs[v] = 0;
            int vs = (v == 0) ? 0 : vr[v - 1];
            int ve = vr[v], sc = ve - vs;
            if (ec[v] > density_capacity(sc)) {
                enlarged = true;
                esegs[v] = PMA_ENLARGE_FACTOR * sc;
            }
        }

        int work_bo = boff;  // HBM byte offset of current PMA
        int nvr[VERTEX_PER_PMA];  // new vertex_range, saved for segment remapping after enlarge
        for (int v = 0; v < VERTEX_PER_PMA; v++) nvr[v] = 0;

        if (enlarged) {
            int acc = 0;
            for (int v = 0; v < VERTEX_PER_PMA; v++) {
                #pragma HLS PIPELINE II=1
                acc += esegs[v];
                nvr[v] = vr[v] + acc;
            }
            int ntseg = nvr[VERTEX_PER_PMA - 1];
            int nhpw  = (ntseg + 7) / 8;
            int npwords = HEADER_WORDS + nhpw + ntseg * SEG_WORDS_PER_SEG;
            int npbytes = npwords * 64;

            ap_axiu<96,0,0,0> acmd2;
            acmd2.data.range(31,0)   = 0;         // start=0 for ALLOC
            acmd2.data.range(63,32)  = npbytes;   // request bytes
            acmd2.data.range(95,64)  = 1;         // opcode=ALLOC
            acmd2.last = 0; acmd2.keep = 0xFFF;
            alloc_cmd_stream.write(acmd2);
            #pragma HLS protocol fixed
            ap_wait();
            ap_axiu<64,0,0,0> arsp2 = alloc_rsp_stream.read();
            int nstart = (int)arsp2.data.range(31, 0);

            int ohpw = (tseg + 7) / 8;
            int oseg_base = boff + HEADER_BYTES + ohpw * 64;
            int nseg_base = nstart + HEADER_BYTES + nhpw * 64;

            // Move segments (burst-optimized: separate read/write phases per UG1399)
            // Phase 1: burst read all old segments → local buffer
            // Old segments are contiguous — pure read burst
            int total_old_words = tseg * SEG_WORDS_PER_SEG;
            ap_uint<512> seg_buf[4096];
            #pragma HLS bind_storage variable=seg_buf type=RAM_2P impl=URAM
            for (int i = 0; i < total_old_words; i++) {
                #pragma HLS PIPELINE II=1
                seg_buf[i] = hbm[oseg_base / 64 + i];
            }

            // Phase 2: per-vertex burst write from buffer to new layout
            int buf_pos = 0;
            for (int v = 0; v < VERTEX_PER_PMA; v++) {
                int ovs = (v == 0) ? 0 : vr[v - 1];
                int ocnt = vr[v] - ovs;
                int nvs = (v == 0) ? 0 : nvr[v - 1];
                int nve = nvr[v];
                // Contiguous burst write of existing segments
                for (int i = 0; i < ocnt * SEG_WORDS_PER_SEG; i++) {
                    #pragma HLS PIPELINE II=1
                    hbm[nseg_base / 64 + nvs * SEG_WORDS_PER_SEG + i] = seg_buf[buf_pos + i];
                }
                buf_pos += ocnt * SEG_WORDS_PER_SEG;
                // Contiguous burst write of empty segments (template)
                int extra = nve - nvs - ocnt;
                for (int i = 0; i < extra * SEG_WORDS_PER_SEG; i++) {
                    #pragma HLS PIPELINE II=1
                    hbm[nseg_base / 64 + (nvs + ocnt) * SEG_WORDS_PER_SEG + i] = make_empty_seg(i % SEG_WORDS_PER_SEG);
                }
            }

            // Rebuild helper
            ap_uint<512> nhlp[MAX_HELPER_WORDS];
            for (int w = 0; w < nhpw; w++) nhlp[w] = EMPTY_HELPER;
            for (int v = 0; v < VERTEX_PER_PMA; v++) {
                int ovs = (v == 0) ? 0 : vr[v - 1], ove = vr[v], ocnt = ove - ovs;
                int nvs = (v == 0) ? 0 : nvr[v - 1];
                for (int s = 0; s < ocnt; s++) {
                    int oseg = ovs + s, nseg = nvs + s;
                    int ow = oseg / 8, os = oseg % 8, nw = nseg / 8, ns = nseg % 8;
                    ap_uint<64> v = chp[line][ow].range(os * 64 + 63, os * 64);
                    nhlp[nw].range(ns * 64 + 63, ns * 64) = v;
                }
            }

            // Free old
            int obytes = (HEADER_WORDS + ohpw + tseg * SEG_WORDS_PER_SEG) * 64;
            ap_axiu<96,0,0,0> fcmd;
            fcmd.data.range(31,0)   = boff;      // start byte of old PMA
            fcmd.data.range(63,32)  = obytes;    // old PMA size in bytes
            fcmd.data.range(95,64)  = 2;         // opcode=FREE
            fcmd.last = 0; fcmd.keep = 0xFFF;
            alloc_cmd_stream.write(fcmd);
            // fire-and-forget: no response

            // Merged header+helper write into one burst (UG1399 coalesce)
            ap_uint<512> nhdr[HEADER_WORDS];
            nhdr[0] = 0;
            for (int v = 0; v < VERTEX_PER_PMA; v++)
                nhdr[0].range(v * 32 + 31, v * 32) = nvr[v];
            nhdr[1] = ch[line][1]; nhdr[2] = ch[line][2]; nhdr[3] = ch[line][3];
            int nen_base = nstart / 64;
            int nen_total = HEADER_WORDS + nhpw;
            for (int w = 0; w < nen_total; w++) {
                #pragma HLS PIPELINE II=1
                hbm[nen_base + w] = (w < HEADER_WORDS) ? nhdr[w] : nhlp[w - HEADER_WORDS];
            }

            // Update VEC_INFO
            VEC_INFO nvi = 0;
            nvi.range(31, 0) = (ap_uint<32>)(nstart + data_off);
            nvi.range(63, 32) = (ap_uint<32>)npbytes;
            vi_write(hbm, vi_wo, pma, nvi);

            // Update cache
            for (int w = 0; w < HEADER_WORDS; w++) ch[line][w] = nhdr[w];
            for (int w = 0; w < nhpw; w++) chp[line][w] = nhlp[w];
            cvi[line] = nvi; ctseg[line] = ntseg; chpw[line] = nhpw;

            work_bo = nstart;

            // Remap segment numbers in seg_list: old layout → new layout after enlarge
            for (int si = 0; si < seg_cnt; si++) {
                int old_seg = seg_list[si];
                for (int v = 0; v < VERTEX_PER_PMA; v++) {
                    #pragma HLS UNROLL
                    int ovs = (v == 0) ? 0 : vr[v - 1];
                    int ove = vr[v];
                    if (old_seg >= ovs && old_seg < ove) {
                        int nvs = (v == 0) ? 0 : nvr[v - 1];
                        seg_list[si] = nvs + (old_seg - ovs);
                    }
                }
            }
        }

        // Write back final header
        // Write back final header (4-word unrolled burst)
        int wb_base = work_bo / 64;
        for (int w = 0; w < HEADER_WORDS; w++) {
            #pragma HLS UNROLL
            hbm[wb_base + w] = ch[line][w];
        }

        // Build O(1) lookup: which vertices were enlarged
        bool vertex_enlarged[VERTEX_PER_PMA];
        #pragma HLS ARRAY_PARTITION variable=vertex_enlarged complete
        for (int v = 0; v < VERTEX_PER_PMA; v++) vertex_enlarged[v] = (esegs[v] > 0);

        // Push ENLARGE_REBALANCE tasks: redistribution range = full new vertex span
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            if (vertex_enlarged[v]) {
                ap_uint<64> t;
                t.range(31, 0) = 0x80000000 | (ap_uint<32>)v;
                t.range(63, 32) = pma;
                task_fifo.write(t);
            }
        }

        // Push SEGMENT_REBALANCE tasks: overflowed segments whose owner wasn't enlarged
        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS PIPELINE II=1
            if (!seg_overflow[si]) continue;
            if (vertex_enlarged[seg_owner[si]]) continue;  // O(1): covered by ENLARGE_REBALANCE
            ap_uint<64> t;
            t.range(31, 0) = seg_list[si];  // already remapped if enlarge happened
            t.range(63, 32) = pma;
            task_fifo.write(t);
        }

        // ACK marker per batch (always sent, even if no rebalance tasks)
        ap_uint<64> ack_task;
        ack_task.range(31, 0) = ap_uint<32>(-2);
        ack_task.range(63, 32) = pma;
        task_fifo.write(ack_task);

        // Invalidate cache line so next batch re-reads header+helper from HBM.
        // Redistribution may have modified the helper on HBM after this batch.
        cvalid[line] = 0;
    }
    // Send STOP to alloc_access (no ACK — rtss protocol) and signal stop to redistribution phase
    ap_axiu<96,0,0,0> scmd;
    scmd.data.range(95,64) = 0;  // opcode=STOP
    scmd.last = 1; scmd.keep = 0xFFF;
    alloc_cmd_stream.write(scmd);

    ap_uint<64> stop_task;
    stop_task.range(31, 0) = ap_uint<32>(-1);
    stop_task.range(63, 32) = 0;
    task_fifo.write(stop_task);
}

// ---- process_cmd (unchanged) ----

void process_cmd(
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
    hls::stream<int> cmd_count_stream[BS_WORKER_NUM],
    hls::stream<PMA_UPDATE_MESSAGE> cmd_batch_stream[BS_WORKER_NUM],
    hls::stream<pma_idx_info> pma_info_stream[BS_WORKER_NUM]
)
{
    printf("PROCESS_CMD\n");
    int next_worker = 0;
    while(true)
    {
        if (cmd_stream.empty()) { ap_wait(); continue; }
        PMA_UPDATE_MESSAGE cmd_batch[MAX_CMD_NUM];
        #pragma HLS ARRAY_PARTITION variable=cmd_batch complete
        int cmd_count = 0;
        bool done = false;
        PMA_UPDATE_MESSAGE first = cmd_stream.read();
        switch (first.opcode)
        {
            case PMA_UPDATE_STOP:
                if (first.last == 1) {
                    pma_idx_info info; info.pma_idx = 0; info.command_type = INSERT_STOP;
                    for (int i = 0; i < BS_WORKER_NUM; i++) pma_info_stream[i].write(info);
                    return;
                }
                break;
            case PMA_UPDATE_FLUSH_CACHE:
            {
                pma_idx_info info; info.pma_idx = first.src >> 4; info.command_type = INSERT_FLUSH_CACHE;
                for (int i = 0; i < BS_WORKER_NUM; i++) pma_info_stream[i].write(info);
                break;
            }
            default:
            {
                cmd_batch[cmd_count++] = first;
                if (first.last == 1) done = true;
                for (int i = 1; i < MAX_CMD_NUM; i++) {
                    if (done) break;
                    PMA_UPDATE_MESSAGE cmd = cmd_stream.read();
                    cmd_batch[i] = cmd; cmd_count++;
                    if (cmd.last == 1) done = true;
                }
                pma_idx_info info; info.pma_idx = cmd_batch[0].src >> 4; info.command_type = INSERT_GOING;
                pma_info_stream[next_worker].write(info);

                PMA_UPDATE_MESSAGE edge_info[MAX_CMD_NUM];
                #pragma HLS ARRAY_PARTITION variable=edge_info complete
                for (int i = 0; i < cmd_count; i++) edge_info[i] = cmd_batch[i];
                for (int i = cmd_count; i < MAX_CMD_NUM; i++) {
                    edge_info[i].src = 0x7FFFFFFF; edge_info[i].dst = 0x7FFFFFFF;
                    edge_info[i].opcode = (PMA_UPDATE_OPCODE)0x7FFFFFFF;
                }
                #define BCS(a,b) do { \
                    PMA_UPDATE_MESSAGE _ta = edge_info[a], _tb = edge_info[b]; \
                    bool _gt = ((int)_ta.src > (int)_tb.src) || \
                        ((int)_ta.src == (int)_tb.src && ((int)_ta.dst > (int)_tb.dst || \
                        ((int)_ta.dst == (int)_tb.dst && (int)_ta.opcode > (int)_tb.opcode))); \
                    if (_gt) { edge_info[a] = _tb; edge_info[b] = _ta; } \
                } while(0)
                BCS( 0, 1); BCS( 2, 3); BCS( 4, 5); BCS( 6, 7);
                BCS( 8, 9); BCS(10,11); BCS(12,13); BCS(14,15);
                BCS( 0, 2); BCS( 1, 3); BCS( 4, 6); BCS( 5, 7);
                BCS( 8,10); BCS( 9,11); BCS(12,14); BCS(13,15);
                BCS( 1, 2); BCS( 5, 6); BCS( 9,10); BCS(13,14);
                BCS( 0, 4); BCS( 1, 5); BCS( 2, 6); BCS( 3, 7);
                BCS( 8,12); BCS( 9,13); BCS(10,14); BCS(11,15);
                BCS( 2, 4); BCS( 3, 5); BCS(10,12); BCS(11,13);
                BCS( 1, 2); BCS( 3, 4); BCS( 5, 6);
                BCS( 9,10); BCS(11,12); BCS(13,14);
                BCS( 0, 8); BCS( 1, 9); BCS( 2,10); BCS( 3,11);
                BCS( 4,12); BCS( 5,13); BCS( 6,14); BCS( 7,15);
                BCS( 4, 8); BCS( 5, 9); BCS( 6,10); BCS( 7,11);
                BCS( 2, 4); BCS( 3, 5); BCS( 6, 8); BCS( 7, 9);
                BCS(10,12); BCS(11,13);
                BCS( 1, 2); BCS( 3, 4); BCS( 5, 6); BCS( 7, 8);
                BCS( 9,10); BCS(11,12); BCS(13,14);
                #undef BCS
                cmd_count_stream[next_worker].write(cmd_count);
                for (int i = 0; i < cmd_count ; i++)
                    cmd_batch_stream[next_worker].write(edge_info[i]);
                next_worker = (next_worker + 1) % BS_WORKER_NUM;
                break;
            }
        }
    }
}

// ---- Phase 4: Redistribution (rtss-style deferred rebalance) ----
// Task types (in payload[31:0]):
//   -1 (0xFFFFFFFF): STOP
//   -2 (0xFFFFFFFE): ACK marker
//   0x80000000 | v: ENLARGE_REBALANCE — redistribute full new vertex range
//   seg (>=0):       SEGMENT_REBALANCE — power-of-2 group expansion starting at seg

void redistribution_phase(
    ap_uint<512>* hbm,
    int data_off, int vi_wo,
    hls::stream<ap_uint<64>>& task_fifo,
    hls::stream<COMMAND_STREAM>& ack_out)
{
    while (true) {
        ap_uint<64> task = task_fifo.read();
        int payload = (int)task.range(31, 0);
        int pm  = (int)task.range(63, 32);
        if (payload == -1) {  // STOP
            COMMAND_STREAM stop_ack; stop_ack.data = -1; stop_ack.keep = 0xF; stop_ack.strb = 0xF; stop_ack.user = 0; stop_ack.last = 0;
            ack_out.write(stop_ack);
            break;
        }
        if (payload == -2) {  // ACK marker
            COMMAND_STREAM ack; ack.data = pm; ack.keep = 0xF; ack.strb = 0xF; ack.user = 0; ack.last = 0;
            ack_out.write(ack);
            continue;
        }

        bool is_enlarge = (payload & 0x80000000) != 0;

        // Read VEC_INFO → header → burst-read entire helper on-chip
        int wi = pm / 8, si = pm % 8;
        ap_uint<512> w = hbm[vi_wo + wi];
        VEC_INFO vi;
        vi.range(63, 0) = w.range(si * 64 + 63, si * 64);
        if (vi == 0) continue;
        int pma_start = (int)vi.range(31, 0);
        int pma_boff  = pma_start - data_off;

        int hbase = pma_boff / 64;
        ap_uint<512> hdr_local[HEADER_WORDS];
        memcpy(hdr_local, &hbm[hbase], HEADER_WORDS * sizeof(ap_uint<512>));
        int total_segs = (int)hdr_local[0].range(15 * 32 + 31, 15 * 32);
        if (total_segs == 0) continue;
        int hpw = (total_segs + 7) / 8;
        int hlp_base = hbase + HEADER_WORDS;

        // Burst-read entire helper into local BRAM
        ap_uint<512> hlp_local[MAX_HELPER_WORDS];
        #pragma HLS bind_storage variable=hlp_local type=ram_t2p impl=BRAM
        memcpy(hlp_local, &hbm[hlp_base], hpw * sizeof(ap_uint<512>));

        int vr[VERTEX_PER_PMA];
        for (int vv = 0; vv < VERTEX_PER_PMA; vv++) {
            #pragma HLS UNROLL
            vr[vv] = (int)hdr_local[0].range(vv * 32 + 31, vv * 32);
        }

        // Macro: valid_count from local BRAM
        #define HLP_VC(s) (int)hlp_local[(s)/8].range(((s)%8)*64+63, ((s)%8)*64+32)

        int left, right, total;

        if (is_enlarge) {
            // ENLARGE_REBALANCE: range = full vertex span (no power-of-2 needed)
            int v = payload & 0xF;
            left  = (v == 0) ? 0 : vr[v - 1];
            right = vr[v];
            total = 0;
            for (int s = left; s < right; s++) {
                #pragma HLS PIPELINE II=1
                total += HLP_VC(s);
            }
        } else {
            // SEGMENT_REBALANCE: overflow confirmed by insert_phase, find group via power-of-2
            int seg = payload;
            int owner_v = -1;
            for (int vv = 0; vv < VERTEX_PER_PMA; vv++) {
                #pragma HLS UNROLL
                int vs = (vv == 0) ? 0 : vr[vv - 1];
                if (seg >= vs && seg < vr[vv]) owner_v = vv;
            }
            if (owner_v < 0) continue;

            int v_start = (owner_v == 0) ? 0 : vr[owner_v - 1];
            int v_end   = vr[owner_v];
            int v_segs  = v_end - v_start;
            if (v_segs <= 1) continue;

            // Power-of-2 group expansion with binary-tree alignment.
            int rel = seg - v_start;
            left = seg; right = seg + 1;
            total = HLP_VC(seg);
            for (int target_gsz = 2; target_gsz <= v_segs * 2; target_gsz *= 2) {
                if (total <= (right - left) * 47) break;
                int nl = v_start + (rel / target_gsz) * target_gsz;
                int nr = v_start + ((rel / target_gsz) + 1) * target_gsz;
                if (nr > v_end) nr = v_end;
                if (nl == left && nr == right) continue;
                for (int s = nl; s < nr; s++) {
                    #pragma HLS PIPELINE II=1
                    if (s < left || s >= right) total += HLP_VC(s);
                }
                left = nl; right = nr;
            }
        }

        // --- Redistribute group [left, right) (common to both task types) ---
        int group_sz = right - left;
        int seg_words_total = group_sz * SEG_WORDS_PER_SEG;
        int seg_base_left = (pma_boff + HEADER_BYTES + hpw * 64 + left * SEG_BYTES) / 64;

        // A: burst-read segments
        ap_uint<512> seg_buf_rd[256];
        memcpy(seg_buf_rd, &hbm[seg_base_left], seg_words_total * sizeof(ap_uint<512>));

        // B: extract edges
        int edge_buf[10240];
        #pragma HLS bind_storage variable=edge_buf type=ram_1p impl=BRAM
        int pos = 0;
        for (int s = 0; s < group_sz; s++) {
            int vc = (int)seg_buf_rd[s * SEG_WORDS_PER_SEG + 3].range(511, 480);
            if (vc > EDGE_SLOTS_PER_SEG) vc = EDGE_SLOTS_PER_SEG;
            for (int k = 0; k < vc; k++) {
                #pragma HLS PIPELINE II=1
                int wi2 = s * SEG_WORDS_PER_SEG + k / 16;
                int si2 = k % 16;
                edge_buf[pos++] = (int)seg_buf_rd[wi2].range(si2 * 32 + 31, si2 * 32);
            }
        }

        // C: fill write buffer (pre-filled with EMPTY_SEG template)
        int base  = total / group_sz;
        int extra = total % group_sz;
        ap_uint<512> seg_buf_wr[256];
        for (int i = 0; i < seg_words_total; i++) {
            #pragma HLS PIPELINE II=1
            seg_buf_wr[i] = make_empty_seg(i % SEG_WORDS_PER_SEG);
        }
        pos = 0;
        for (int s = 0; s < group_sz; s++) {
            int fill = base + (s < extra ? 1 : 0);
            for (int k = 0; k < fill; k++) {
                #pragma HLS PIPELINE II=1
                int wi2 = s * SEG_WORDS_PER_SEG + k / 16;
                int si2 = k % 16;
                seg_buf_wr[wi2].range(si2 * 32 + 31, si2 * 32) = (ap_uint<32>)edge_buf[pos++];
            }
            seg_buf_wr[s * SEG_WORDS_PER_SEG + 3].range(511, 480) = (ap_uint<32>)fill;
        }

        // D: burst-write segments
        memcpy(&hbm[seg_base_left], seg_buf_wr, seg_words_total * sizeof(ap_uint<512>));

        // E: update local helper + burst-write back
        for (int s = left; s < right; s++) {
            int fill = base + ((s - left) < extra ? 1 : 0);
            ap_uint<64> hv2 = 0;
            hv2.range(31, 0)  = (ap_uint<32>)(fill > 0 ? (int)seg_buf_wr[(s-left)*SEG_WORDS_PER_SEG].range(31, 0) : UNUSED_ICON);
            hv2.range(63, 32) = (ap_uint<32>)fill;
            hlp_local[s / 8].range((s % 8) * 64 + 63, (s % 8) * 64) = hv2;
        }
        memcpy(&hbm[hlp_base], hlp_local, hpw * sizeof(ap_uint<512>));

        #undef HLP_VC
    }
}

// ---- Top-level ----

extern "C"
{
    void pma_insert_edge(
        ap_uint<512>* hbm_bs0,    // BS worker 0 read-only
        ap_uint<512>* hbm_bs1,    // BS worker 1 read-only
        ap_uint<512>* hbm_bs2,    // BS worker 2 read-only
        ap_uint<512>* hbm_bs3,    // BS worker 3 read-only
        ap_uint<512>* hbm_rw,     // insert phase read-write
        ap_uint<512>* hbm_red,    // redistribution read-write (separate port)
        int pma_raw_data_start,
        int pma_vecinfo_start,

        hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
        hls::stream<COMMAND_STREAM> &ack_back_to_router,
        hls::stream<ap_axiu<96,0,0,0>>& alloc_cmd_stream,
        hls::stream<ap_axiu<64,0,0,0>>& alloc_rsp_stream
    )
    {
        #pragma HLS INTERFACE m_axi port=hbm_bs0 offset=slave bundle=gmem0 max_read_burst_length=256 max_write_burst_length=256 
        #pragma HLS INTERFACE m_axi port=hbm_bs1 offset=slave bundle=gmem1 max_read_burst_length=256 max_write_burst_length=256 
        #pragma HLS INTERFACE m_axi port=hbm_bs2 offset=slave bundle=gmem2 max_read_burst_length=256 max_write_burst_length=256 
        #pragma HLS INTERFACE m_axi port=hbm_bs3 offset=slave bundle=gmem3 max_read_burst_length=256 max_write_burst_length=256  
        #pragma HLS INTERFACE m_axi port=hbm_rw  offset=slave bundle=gmem4 max_read_burst_length=256 max_write_burst_length=256 
        #pragma HLS INTERFACE m_axi port=hbm_red offset=slave bundle=gmem5 max_read_burst_length=256 max_write_burst_length=256 
        
        #pragma HLS INTERFACE s_axilite port=hbm_bs0 bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_bs1 bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_bs2 bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_bs3 bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_rw  bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_red bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_raw_data_start bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_vecinfo_start bundle=control
        #pragma HLS INTERFACE s_axilite port=return bundle=control
        #pragma HLS INTERFACE axis port=cmd_stream
        #pragma HLS INTERFACE axis port=ack_back_to_router
        #pragma HLS INTERFACE axis port=alloc_cmd_stream
        #pragma HLS INTERFACE axis port=alloc_rsp_stream

        hls::stream<int> cmd_count_stream[BS_WORKER_NUM];
        #pragma HLS STREAM variable=cmd_count_stream depth=8
        hls::stream<PMA_UPDATE_MESSAGE> cmd_batch_stream[BS_WORKER_NUM];
        #pragma HLS STREAM variable=cmd_batch_stream depth=32
        hls::stream<pma_idx_info> pma_info_stream[BS_WORKER_NUM];
        #pragma HLS STREAM variable=pma_info_stream depth=8

        hls::stream<bs_batch_t> bs_out[BS_WORKER_NUM];
        #pragma HLS STREAM variable=bs_out depth=8

        hls::stream<ap_uint<64>> task_fifo;
        #pragma HLS STREAM variable=task_fifo depth=64

        #pragma HLS DATAFLOW

        process_cmd(cmd_stream, cmd_count_stream, cmd_batch_stream, pma_info_stream);

        bs_worker(hbm_bs0, pma_raw_data_start, pma_vecinfo_start / 64,
                  cmd_batch_stream[0], cmd_count_stream[0], pma_info_stream[0], bs_out[0]);

        bs_worker(hbm_bs1, pma_raw_data_start, pma_vecinfo_start / 64,
                  cmd_batch_stream[1], cmd_count_stream[1], pma_info_stream[1], bs_out[1]);

        bs_worker(hbm_bs2, pma_raw_data_start, pma_vecinfo_start / 64,
                  cmd_batch_stream[2], cmd_count_stream[2], pma_info_stream[2], bs_out[2]);

        bs_worker(hbm_bs3, pma_raw_data_start, pma_vecinfo_start / 64,
                  cmd_batch_stream[3], cmd_count_stream[3], pma_info_stream[3], bs_out[3]);

        insert_phase(hbm_rw, pma_raw_data_start, pma_vecinfo_start / 64,
                     bs_out[0], bs_out[1], bs_out[2], bs_out[3],
                     alloc_cmd_stream, alloc_rsp_stream, task_fifo);

        redistribution_phase(hbm_red, pma_raw_data_start, pma_vecinfo_start / 64,
                            task_fifo, ack_back_to_router);
    }
}
