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

#define MAX_HELPER_WORDS 256

// Forwarded header info: pma_gate → bs_worker → insert_phase
// Eliminates redundant VEC_INFO + header HBM reads in downstream stages
struct fwd_header_t {
    int start_byte;              // absolute HBM byte offset of PMA data
    ap_uint<512> range_word;     // header[0]: vertex ranges [15:0] + total_segs[511:480]
    ap_uint<512> edge_count_word;// header[1]: per-vertex edge counts
};

// Rebalance task: insert_phase → redistribution_phase
// Carries forwarded header to avoid redundant VEC_INFO/header HBM reads
struct rebalance_task_t {
    int pma;                     // PMA index
    int payload;                 // segment index (>=0), -1=STOP, -2=ACK marker
    int start_byte;              // forwarded: PMA absolute HBM byte offset
    ap_uint<512> range_word;     // forwarded: header[0] — vertex ranges + total_segs
};

// ALLOC_BITMAP_WORDS defined in data_type.h (shared with alloc_access_kernel)

// File-scope helper: ALL_ONES and EMPTY_SEG with valid_count=0
// Empty segment ROM: 4 words × 512 bits, replaces make_empty_seg()
// Word 0-2: all ones (UNUSED_ICON for all 16 edge slots)
// Word 3:   all ones except bits[511:480]=0 (valid_count=0)
static const ap_uint<512> EMPTY_SEG_ROM[4] = {
    ~ap_uint<512>(0),            // all ones
    ~ap_uint<512>(0),            // all ones
    ~ap_uint<512>(0),            // all ones
    (~ap_uint<512>(0)) >> 32     // bits[511:480]=0, rest all ones
};

// Pre-computed header[0] for empty PMA: 16 vertices × 1 seg, total_segs=16
static const ap_uint<512> EMPTY_PMA_HDR0 =
    (ap_uint<512>(16) << 480) |
    (ap_uint<512>(15) << 448) |
    (ap_uint<512>(14) << 416) |
    (ap_uint<512>(13) << 384) |
    (ap_uint<512>(12) << 352) |
    (ap_uint<512>(11) << 320) |
    (ap_uint<512>(10) << 288) |
    (ap_uint<512>( 9) << 256) |
    (ap_uint<512>( 8) << 224) |
    (ap_uint<512>( 7) << 192) |
    (ap_uint<512>( 6) << 160) |
    (ap_uint<512>( 5) << 128) |
    (ap_uint<512>( 4) <<  96) |
    (ap_uint<512>( 3) <<  64) |
    (ap_uint<512>( 2) <<  32) |
    (ap_uint<512>( 1) <<   0);

// Pre-computed helper word for empty PMA: 8 entries of (vc=0, first=UNUSED_ICON)
static const ap_uint<512> EMPTY_HELPER_WORD =
    (ap_uint<512>(0x00000000FFFFFFFFull) << (7*64)) |
    (ap_uint<512>(0x00000000FFFFFFFFull) << (6*64)) |
    (ap_uint<512>(0x00000000FFFFFFFFull) << (5*64)) |
    (ap_uint<512>(0x00000000FFFFFFFFull) << (4*64)) |
    (ap_uint<512>(0x00000000FFFFFFFFull) << (3*64)) |
    (ap_uint<512>(0x00000000FFFFFFFFull) << (2*64)) |
    (ap_uint<512>(0x00000000FFFFFFFFull) << (1*64)) |
    (ap_uint<512>(0x00000000FFFFFFFFull) << (0*64));

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
    #pragma HLS INLINE off
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

// ---- Segment Insert (pure compute, no HBM access) ----
// Processes one segment: extract edges, insert new dsts, pack back.
// Designed for multiple parallel instances (INLINE off).

void seg_insert(
    ap_uint<512> seg_in[SEG_WORDS_PER_SEG],
    int dsts[16],
    int dst_cnt,
    ap_uint<512> seg_out[SEG_WORDS_PER_SEG],
    int &new_valid,
    bool &overflow
) {
    #pragma HLS INLINE off

    int valid = (int)seg_in[3].range(511, 480);

    // Extract existing edges into edge_buf
    int edge_buf[EDGE_SLOTS_PER_SEG];
    // #pragma HLS ARRAY_PARTITION variable=edge_buf complete
    for (int k = 0; k < EDGE_SLOTS_PER_SEG; k++) {
        #pragma HLS UNROLL
        int wi = k / 16, slot = k % 16;
        edge_buf[k] = (k < valid) ? (int)seg_in[wi].range(slot * 32 + 31, slot * 32) : UNUSED_ICON;
    }

    // Insert each new edge using binary search for O(log N) position finding
    // edge_buf[0..valid-1] is sorted ascending — binary search replaces 48-way
    // parallel comparator+popcount with ~6 sequential comparisons, saving LUTs
    // and shortening the critical path at negligible latency cost (pma_gate
    // HBM access dominates the pipeline anyway).
    for (int ei = 0; ei < dst_cnt; ei++) {
        #pragma HLS PIPELINE II=2
        int dst = dsts[ei];
        if (valid >= EDGE_SLOTS_PER_SEG) break;

        // Binary search for insertion position
        int lo = 0, hi = valid;
        while (lo < hi) {
            #pragma HLS PIPELINE II=1
            int mid = (lo + hi) / 2;
            if (edge_buf[mid] < dst)
                lo = mid + 1;
            else
                hi = mid;
        }
        int pos = lo;

        // Shift elements right to make room
        for (int k = valid; k > pos; k--) {
            // #pragma HLS UNROLL
            edge_buf[k] = edge_buf[k - 1];
        }
        edge_buf[pos] = dst;
        valid++;
    }

    overflow   = (valid > 47);
    new_valid  = valid;

    // Pack back into seg_out (UNUSED_ICON padding)
    for (int k = 0; k < EDGE_SLOTS_PER_SEG; k++) {
        #pragma HLS UNROLL
        int wi = k / 16, slot = k % 16;
        int val = (k < valid) ? edge_buf[k] : UNUSED_ICON;
        seg_out[wi].range(slot * 32 + 31, slot * 32) = (ap_uint<32>)val;
    }
    seg_out[3].range(511, 480) = (ap_uint<32>)valid;
}

// ---- pma_gate helper: handle existing (non-empty) PMA ----
// Extracted to a separate function to prevent HLS if-conversion
// from speculating alloc/free/m_axi writes when vi==0.
static void pma_gate_handle_existing(
    ap_uint<512>* hbm_gate,
    int pma_raw_data_start,
    int pma_vecinfo_start,
    int pma_idx,
    VEC_INFO vi,
    PMA_UPDATE_MESSAGE* cmd_batch,
    int cmd_count,
    ap_uint<512>* pma_buffer,
    hls::stream<ap_axiu<96,0,0,0>>& alloc_cmd_stream,
    hls::stream<ap_axiu<64,0,0,0>>& alloc_rsp_stream,
    hls::stream<fwd_header_t>& hdr_fwd)
{
    ap_uint<512> range_word = hbm_gate[(int)(vi.range(31,0) - pma_raw_data_start) / 64];
    ap_uint<512> edge_count_word = hbm_gate[(int)(vi.range(31,0) - pma_raw_data_start) / 64 + 1];
    int delta_edge[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    int delta_range[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    bool enlarge = false;
    int total_delta = 0;
    for (int i = 0 ; i < cmd_count ; i++)
    {
        #pragma HLS PIPELINE II=1
        PMA_UPDATE_MESSAGE cmd = cmd_batch[i];
        int local_src = cmd.src & 0xF;
        delta_edge[local_src]++;
    }
    for (int i = 0 ; i < 16 ; i++)
    {
        #pragma HLS PIPELINE II=1
        if (delta_edge[i] == 0) continue;
        int old_count = (int)edge_count_word.range(i * 32 + 31, i * 32);
        int new_count = old_count + delta_edge[i];
        int seg_start = (i == 0) ? 0 : (int)range_word.range((i - 1) * 32 + 31, (i - 1) * 32);
        int seg_end = (int)range_word.range(i * 32 + 31, i * 32);

        int seg_len = seg_end - seg_start;
        int max_edge = density_capacity(seg_len);
        if (new_count > max_edge)
        {
            // Tiered growth: ×4 for small vertices (fast ramp-up),
            // ×2 for large vertices (limit wasted space)
            delta_range[i] = (seg_len <= 4) ? 3*seg_len : seg_len;
            total_delta += delta_range[i];
            enlarge = true;
        }
    }
    if (enlarge)
    {
        // hls::print("ENLARGE trig\n");
        int old_tseg = (int)range_word.range(15 * 32 + 31, 15 * 32);
        int ntseg = old_tseg + total_delta;
        int nhpw   = (ntseg + 7) / 8;
        int npwords = HEADER_WORDS + nhpw + ntseg * SEG_WORDS_PER_SEG;
        int npbytes = npwords * 64;
        ap_axiu<96,0,0,0> acmd2;
        acmd2.data.range(31, 0)  = 0;
        acmd2.data.range(63, 32) = npbytes;
        acmd2.data.range(95, 64) = 1;
        acmd2.last = 0; acmd2.keep = 0xFFF;
        alloc_cmd_stream.write(acmd2);
        #pragma HLS protocol fixed

        // ---- Compute new range_word (no dependency on alloc response) ----
        ap_uint<512> old_range_word = range_word;
        int acc = 0;
        for (int i = 0 ; i < 16 ; i++)
        {
            #pragma HLS PIPELINE II=1
            acc += delta_range[i];
            int old_vr = (int)range_word.range(i * 32 + 31, i * 32);
            int new_vr = old_vr + acc;
            range_word.range(i * 32 + 31, i * 32) = new_vr;
        }

        // ---- Compute old layout + read old segments while alloc is in flight ----
        int old_boff = (int)vi.range(31, 0) - pma_raw_data_start;
        int old_hpw  = (old_tseg + 7) / 8;
        int old_seg_base = (old_boff + HEADER_BYTES + old_hpw * 64) / 64;
        int old_seg_words = old_tseg * SEG_WORDS_PER_SEG;
        for (int i = 0; i < old_seg_words; i++) {
            #pragma HLS PIPELINE II=1
            pma_buffer[i] = hbm_gate[old_seg_base + i];
        }

        // ---- Initialize nhlp with empty entries (while alloc is still in flight) ----
        ap_uint<512> nhlp[MAX_HELPER_WORDS];
        #pragma HLS bind_storage variable=nhlp type=RAM_1P impl=BRAM
        ap_uint<64> empty_entry = ((ap_uint<64>)(ap_uint<32>)0 << 32) | (ap_uint<64>)(ap_uint<32>)UNUSED_ICON;
        for (int w = 0; w < nhpw; w++) {
            #pragma HLS PIPELINE II=1
            nhlp[w] = 0;
            for (int z = 0; z < 8; z++) nhlp[w].range(z*64+63, z*64) = empty_entry;
        }

        // ---- FREE old PMA (all old reads complete; safe to free before alloc response) ----
        int obytes = (HEADER_WORDS + old_hpw + old_tseg * SEG_WORDS_PER_SEG) * 64;
        ap_axiu<96,0,0,0> fcmd;
        fcmd.data.range(31, 0)  = old_boff;
        fcmd.data.range(63, 32) = obytes;
        fcmd.data.range(95, 64) = 2;
        fcmd.last = 0; fcmd.keep = 0xFFF;
        alloc_cmd_stream.write(fcmd);

        // Alloc latency hidden by range_word + seg read + nhlp init + FREE above
        ap_wait();
        ap_axiu<64,0,0,0> arsp2 = alloc_rsp_stream.read();
        int nstart = (int)arsp2.data.range(31, 0);

        int nseg_base = nstart / 64 + HEADER_WORDS + nhpw;

        // Single pass over vertices: copy old segments, build helper from pma_buffer,
        // write empty segments for expanded portion — all in one loop
        int buf_pos = 0;
        for (int v = 0; v < 16; v++) {
            int ovs = (v == 0) ? 0 : (int)old_range_word.range((v-1)*32+31, (v-1)*32);
            int ove = (int)old_range_word.range(v*32+31, v*32);
            int nvs = (v == 0) ? 0 : (int)range_word.range((v-1)*32+31, (v-1)*32);
            int nve = (int)range_word.range(v*32+31, v*32);
            int ocnt = ove - ovs;
            int ncnt = nve - nvs;

            // Copy old segment words to new HBM location (burst write)
            for (int i = 0; i < ocnt * SEG_WORDS_PER_SEG; i++) {
                #pragma HLS PIPELINE II=1
                hbm_gate[nseg_base + nvs * SEG_WORDS_PER_SEG + i] = pma_buffer[buf_pos + i];
            }

            // Build helper entries for old segments from pma_buffer data
            // (first_edge from word 0 bits[31:0], valid_count from word 3 bits[511:480])
            for (int s = 0; s < ocnt; s++) {
                #pragma HLS PIPELINE II=1
                int nseg = nvs + s;
                int nw = nseg >> 3, ns = nseg & 7;
                int pb = buf_pos + s * SEG_WORDS_PER_SEG;
                ap_uint<64> hv = 0;
                hv.range(31, 0)  = pma_buffer[pb].range(31, 0);
                hv.range(63, 32) = pma_buffer[pb + 3].range(511, 480);
                nhlp[nw].range(ns * 64 + 63, ns * 64) = hv;
            }
            buf_pos += ocnt * SEG_WORDS_PER_SEG;

            // Write empty segments for expanded portion
            int extra = ncnt - ocnt;
            for (int i = 0; i < extra * SEG_WORDS_PER_SEG; i += SEG_WORDS_PER_SEG) {
                #pragma HLS PIPELINE II=1
                hbm_gate[nseg_base + (nvs + ocnt) * SEG_WORDS_PER_SEG + i + 0] = EMPTY_SEG_ROM[0];
                hbm_gate[nseg_base + (nvs + ocnt) * SEG_WORDS_PER_SEG + i + 1] = EMPTY_SEG_ROM[1];
                hbm_gate[nseg_base + (nvs + ocnt) * SEG_WORDS_PER_SEG + i + 2] = EMPTY_SEG_ROM[2];
                hbm_gate[nseg_base + (nvs + ocnt) * SEG_WORDS_PER_SEG + i + 3] = EMPTY_SEG_ROM[3];
            }
        }

        ap_uint<512> new_hdr[HEADER_WORDS];
        new_hdr[0] = range_word;
        new_hdr[1] = edge_count_word;
        new_hdr[2] = 0; new_hdr[3] = 0;
        int new_abs_base = nstart / 64;
        for (int w = 0; w < HEADER_WORDS; w++) {
            #pragma HLS PIPELINE II=1
            hbm_gate[new_abs_base + w] = new_hdr[w];
        }
        for (int w = 0; w < nhpw; w++) {
            #pragma HLS PIPELINE II=1
            hbm_gate[new_abs_base + HEADER_WORDS + w] = nhlp[w];
        }

        VEC_INFO nvi = 0;
        nvi.range(31, 0)  = (ap_uint<32>)(nstart + pma_raw_data_start);
        nvi.range(63, 32) = (ap_uint<32>)npbytes;
        vi_write(hbm_gate, pma_vecinfo_start, pma_idx, nvi);

        // Forward NEW header to bs_worker (post-enlarge)
        {
            fwd_header_t fh;
            fh.start_byte = nstart + pma_raw_data_start;
            fh.range_word = range_word;
            fh.edge_count_word = edge_count_word;  // unchanged by enlarge
            hdr_fwd.write(fh);
        }
    } else {
        // No enlarge: forward ORIGINAL header to bs_worker
        fwd_header_t fh;
        fh.start_byte = (int)vi.range(31, 0);
        fh.range_word = range_word;
        fh.edge_count_word = edge_count_word;
        hdr_fwd.write(fh);
    }
}

void pma_gate(
    ap_uint<512> *hbm_gate,
    int pma_raw_data_start,
    int pma_vecinfo_start,
    int bitmap_offset,
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_in,
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_out,
    hls::stream<ap_axiu<96,0,0,0>>& alloc_cmd_stream,
    hls::stream<ap_axiu<64,0,0,0>>& alloc_rsp_stream,
    hls::stream<ap_axiu<ALLOC_BITMAP_STORAGE_WORD_BITS,0,0,0>>& bitmap_init,
    hls::stream<fwd_header_t>& hdr_fwd)
{
    ap_uint<512> pma_buffer[4096];
    #pragma HLS bind_storage variable=pma_buffer type=RAM_2P impl=URAM

    // ---- Init bitmap: read from hbm_gate, send to alloc_access ----
    // Runs once before processing any commands. Uses hbm_gate (this function's
    // only m_axi), so no DATAFLOW conflict with pre-DATAFLOW sequential code.
    int bm_word_off = bitmap_offset / 64;
    for (int i = 0; i < ALLOC_BITMAP_WORDS; i++) {
        #pragma HLS PIPELINE II=1
        ap_axiu<ALLOC_BITMAP_STORAGE_WORD_BITS,0,0,0> w;
        w.data = hbm_gate[bm_word_off + i];
        w.last = 0;
        w.keep = -1;
        bitmap_init.write(w);
    }

    while (true)
    {
        if (cmd_in.empty()) { ap_wait(); continue; }
        PMA_UPDATE_MESSAGE first = cmd_in.read();
        if (first.opcode == PMA_UPDATE_STOP && first.last == 1) {
            cmd_out.write(first);
            // Send STOP to alloc_access
            ap_axiu<96,0,0,0> scmd;
            scmd.data.range(95, 64) = 0;  // opcode=STOP
            scmd.last = 1; scmd.keep = 0xFFF;
            alloc_cmd_stream.write(scmd);
            return;
        }
        PMA_UPDATE_MESSAGE cmd_batch[MAX_CMD_NUM];
        cmd_batch[0] = first;
        int cmd_count = 1;
        int pma_idx = first.src >> 4;
        bool done = (first.last == 1);
        for (int i = 1 ; i < MAX_CMD_NUM && !done; i++) {
            if (done) break;
            PMA_UPDATE_MESSAGE cmd = cmd_in.read();
            cmd_batch[i] = cmd; cmd_count++;
            done = (cmd.last == 1);
        }

        // check empty pma
        VEC_INFO vi = vi_read(hbm_gate, pma_vecinfo_start, pma_idx);
        // hls::print("GATE vi=%d\n", (unsigned)(int)vi.range(31,0));
        if (vi == 0)
        {
            // Initialize empty PMA in pma_buffer: header + helper + 16 segments
            int isegs = VERTEX_PER_PMA;  // 16 vertices, 1 segment each
            int ihlp  = (isegs + 7) / 8; // helper words = 2

            // Alloc new PMA space
            int ibytes = (HEADER_WORDS + ihlp + isegs * SEG_WORDS_PER_SEG) * 64;
            ap_axiu<96,0,0,0> acmd;
            acmd.data.range(31, 0)  = 0;        // start=0 for ALLOC
            acmd.data.range(63, 32) = ibytes;   // request bytes
            acmd.data.range(95, 64) = 1;        // opcode=ALLOC
            acmd.last = 0; acmd.keep = 0xFFF;
            alloc_cmd_stream.write(acmd);
            #pragma HLS protocol fixed

            // ---- Build PMA in pma_buffer from ROM WHILE alloc is in flight ----
            // (nstart not needed until write-back)

            // Header: from ROM
            pma_buffer[0] = EMPTY_PMA_HDR0;     // vertex_range: 1 seg per vertex, total=16
            pma_buffer[1] = 0;                   // edge_count all zero
            pma_buffer[2] = 0;
            pma_buffer[3] = 0;

            // Helper: 2 words of pre-computed empty entries
            pma_buffer[HEADER_WORDS + 0] = EMPTY_HELPER_WORD;
            pma_buffer[HEADER_WORDS + 1] = EMPTY_HELPER_WORD;

            // Segments: 16 × EMPTY_SEG_ROM
            int seg_off = HEADER_WORDS + ihlp;
            for (int i = 0; i < isegs * SEG_WORDS_PER_SEG; i += SEG_WORDS_PER_SEG) {
                #pragma HLS PIPELINE II=1
                pma_buffer[seg_off + i + 0] = EMPTY_SEG_ROM[0];
                pma_buffer[seg_off + i + 1] = EMPTY_SEG_ROM[1];
                pma_buffer[seg_off + i + 2] = EMPTY_SEG_ROM[2];
                pma_buffer[seg_off + i + 3] = EMPTY_SEG_ROM[3];
            }

            // Alloc latency hidden: wait for response, then write to HBM
            ap_wait();
            ap_axiu<64,0,0,0> arsp = alloc_rsp_stream.read();
            int nstart = (int)arsp.data.range(31, 0);

            int write_start_in_512_pattern = nstart / 64;
            for (int i = 0; i < HEADER_WORDS + ihlp + isegs * SEG_WORDS_PER_SEG; i++)
            {
                #pragma HLS PIPELINE II=1
                hbm_gate[write_start_in_512_pattern + i] = pma_buffer[i];
            }

            // Update VEC_INFO so subsequent accesses see the PMA
            VEC_INFO nvi = 0;
            nvi.range(31, 0)  = (ap_uint<32>)(nstart + pma_raw_data_start);
            nvi.range(63, 32) = (ap_uint<32>)ibytes;
            vi_write(hbm_gate, pma_vecinfo_start, pma_idx, nvi);

            // Forward NEW header to bs_worker (post-init)
            {
                fwd_header_t fh;
                fh.start_byte = nstart + pma_raw_data_start;
                fh.range_word = pma_buffer[0];  // hdr0: vertex ranges
                fh.edge_count_word = 0;          // hdr1: all edge counts zero
                hdr_fwd.write(fh);
            }
        }
        //check pma enlargement
        else
        {
            // hls::print("GATE else enter\n");
            pma_gate_handle_existing(hbm_gate, pma_raw_data_start, pma_vecinfo_start,
                                     pma_idx, vi, cmd_batch, cmd_count, pma_buffer,
                                     alloc_cmd_stream, alloc_rsp_stream, hdr_fwd);
        }
        for (int i = 0 ; i < cmd_count; i++)
        {
            cmd_out.write(cmd_batch[i]);
        }
    }
}

// ---- Phase 1: BS Worker ----
// Reads header+helper directly from HBM via m_axi, does binary search

void bs_worker(
    ap_uint<512>* hbm,
    int data_off,
    hls::stream<fwd_header_t>& hdr_fwd,
    hls::stream<PMA_UPDATE_MESSAGE>& edge_in,
    hls::stream<bs_batch_t>& bs_out
)
{
    while (true) {
        // Read first edge to determine batch type and pma_idx
        // #pragma HLS PIPELINE II=1
        PMA_UPDATE_MESSAGE first = edge_in.read();
        if (first.opcode == PMA_UPDATE_STOP && first.last == 1) {
            bs_batch_t s; s.pma_idx = -1; s.count = 0; bs_out.write(s); break;
        }

        int pma = first.src >> 4;
        // Read remaining edges until last flag; count = position of last
        PMA_UPDATE_MESSAGE edges[MAX_CMD_NUM];
        edges[0] = first;
        int cnt = 1;
        if (first.last != 1) {
            for (int i = 1; i < MAX_CMD_NUM; i++) {
                #pragma HLS PIPELINE II=1
                edges[i] = edge_in.read();
                cnt++;
                if (edges[i].last == 1) break;
            }
        }

        // Read forwarded header from pma_gate (avoids VEC_INFO + header HBM reads)
        fwd_header_t fh = hdr_fwd.read();
        int byte_off = fh.start_byte - data_off;
        int hbase = byte_off / 64;
        int total_segs = (int)fh.range_word.range(15 * 32 + 31, 15 * 32);

        // Read helper from HBM in single burst
        int hlp_words = (total_segs + HELPER_ENTRIES_PER_WORD - 1) / HELPER_ENTRIES_PER_WORD;
        if (hlp_words > MAX_HELPER_WORDS) hlp_words = MAX_HELPER_WORDS;
        ap_uint<512> hlp_buf[MAX_HELPER_WORDS];
        #pragma HLS bind_storage variable=hlp_buf type=RAM_2P impl=URAM
        for (int w = 0; w < hlp_words; w++) {
            #pragma HLS PIPELINE II=1
            hlp_buf[w] = hbm[hbase + HEADER_WORDS + w];
        }

        // Unpack bs_helper into URAM — no partition, serialized sub_search is fine
        int bs_helper[12040];
        #pragma HLS bind_storage variable=bs_helper type=RAM_2P impl=URAM
        for (int w = 0; w < hlp_words; w++) {
            #pragma HLS PIPELINE II=1
            ap_uint<512> hw = hlp_buf[w];
            for (int s = 0; s < 8; s++) {
                int seg = w * 8 + s;
                if (seg < total_segs)
                    bs_helper[seg] = (int)hw.range(s * 64 + 31, s * 64);
            }
        }

        // Extract vertex_range from forwarded header
        int vr[VERTEX_PER_PMA];
        for (int v = 0; v < VERTEX_PER_PMA; v++)
            #pragma HLS UNROLL
            vr[v] = (int)fh.range_word.range(v * 32 + 31, v * 32);

        // Process edges — serialized binary search (PIPELINE instead of UNROLL)
        bs_batch_t res;
        res.pma_idx = pma; res.op_mask = 0; res.count = cnt;
        res.start_byte = fh.start_byte;
        res.range_word = fh.range_word;
        res.edge_count_word = fh.edge_count_word;

        for (int i = 0; i < MAX_CMD_NUM; i++) {
            #pragma HLS PIPELINE II=1
            if (i < cnt) {
                PMA_UPDATE_MESSAGE e = edges[i];
                res.dst[i] = e.dst;
                res.local_src[i] = e.src & 0xF;
                int op = (int)e.opcode;
                if (op == PMA_UPDATE_INSERT_EDGE /* || op == PMA_UPDATE_DELETE_EDGE */) {
                    int ls = e.src & 0xF;
                    int lo = (ls == 0) ? 0 : vr[ls - 1];
                    int hi = vr[ls];
                    int rel = -1;
                    sub_search(bs_helper, lo, hi, e.dst, rel);
                    if (rel == -1) rel = 0;
                    res.target_seg[i] = rel + lo;
                } else {
                    res.target_seg[i] = 0;
                }
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
    hls::stream<bs_batch_t>& bs_in,
    hls::stream<rebalance_task_t>& task_fifo,
    hls::stream<ap_uint<32>>& redist_ack_in,
    hls::stream<ap_uint<32>>& ack_out)
{
    int pending_cnt = 0;

    while (true) {
        // #pragma HLS PIPELINE II=1

        // --- read_nb: poll redistribution ACK every cycle ---
        if (!redist_ack_in.empty()) {
            ap_uint<32> ack_val = redist_ack_in.read();
            int ack_pma = (int)ack_val;
            if (ack_pma != -1) {
                pending_cnt--;
                ack_out.write((ap_uint<32>)ack_pma);
            }
        }

        if (bs_in.empty()) { ap_wait(); continue; }
        bs_batch_t b = bs_in.read();

        if (b.pma_idx == -1) { break; }

        int pma = b.pma_idx;

        // Use forwarded header from pma_gate → bs_worker
        // (no VEC_INFO read, no header HBM read — only helper is read from HBM)
        int boff = b.start_byte - data_off;
        int hbase = boff / 64;
        int tseg = (int)b.range_word.range(15 * 32 + 31, 15 * 32);
        int hpw  = (tseg + 7) / 8;
        if (hpw > MAX_HELPER_WORDS) hpw = MAX_HELPER_WORDS;

        // Read only helper from HBM (header already forwarded via stream)
        ap_uint<512> hlp_buf[MAX_HELPER_WORDS];
        #pragma HLS bind_storage variable=hlp_buf type=RAM_2P impl=URAM
        for (int w = 0; w < hpw; w++) {
            #pragma HLS PIPELINE II=1
            hlp_buf[w] = hbm[hbase + HEADER_WORDS + w];
        }

        // Vertex ranges and edge counts from forwarded header
        int ec[VERTEX_PER_PMA], vr[VERTEX_PER_PMA];
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            vr[v] = (int)b.range_word.range(v * 32 + 31, v * 32);
            ec[v] = (int)b.edge_count_word.range(v * 32 + 31, v * 32);
        }

        // --- Process segment modifications ---
        int ec_delta[VERTEX_PER_PMA];
        // #pragma HLS ARRAY_PARTITION variable=ec_delta complete
        for (int v = 0; v < VERTEX_PER_PMA; v++) ec_delta[v] = 0;

        int seg_list[16], seg_cnt = 0;
        // #pragma HLS ARRAY_PARTITION variable=seg_list complete
        bool seg_overflow[16];
        // #pragma HLS ARRAY_PARTITION variable=seg_overflow complete
        for (int i = 0; i < 16; i++) seg_overflow[i] = false;

        for (int i = 0; i < b.count; i++) {
            #pragma HLS PIPELINE II=1
            int ts = b.target_seg[i], ls = b.local_src[i];
            ec_delta[ls]++;

            bool found = false;
            for (int j = 0; j < seg_cnt; j++) if (seg_list[j] == ts) { found = true; break; }
            if (!found && seg_cnt < 16) seg_list[seg_cnt++] = ts;
        }

        // --- Phase A: burst-read all modified segments ---
        ap_uint<512> seg_buf[16][SEG_WORDS_PER_SEG];
        // #pragma HLS ARRAY_PARTITION variable=seg_buf complete dim=2
        #pragma HLS bind_storage variable=seg_buf type=RAM_2P impl=BRAM
        int seg_bases[16];
        // #pragma HLS ARRAY_PARTITION variable=seg_bases complete
        int seg_valid_buf[16];
        // #pragma HLS ARRAY_PARTITION variable=seg_valid_buf complete

        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS PIPELINE II=SEG_WORDS_PER_SEG
            int seg = seg_list[si];
            int base = (boff + HEADER_BYTES + hpw * 64 + seg * SEG_BYTES) / 64;
            seg_bases[si] = base;
            for (int w = 0; w < SEG_WORDS_PER_SEG; w++) {
                #pragma HLS UNROLL
                seg_buf[si][w] = hbm[base + w];
            }
        }

        // --- Phase A.5: distribute edges in batch to per-segment dst arrays ---
        int  seg_dsts[16][16];
        // #pragma HLS ARRAY_PARTITION variable=seg_dsts complete dim=2
        int  seg_dst_cnt[16];
        #pragma HLS ARRAY_PARTITION variable=seg_dst_cnt complete
        for (int i = 0; i < 16; i++) seg_dst_cnt[i] = 0;

        for (int i = 0; i < b.count; i++) {
            #pragma HLS PIPELINE II=1
            int ts = b.target_seg[i];
            for (int si = 0; si < seg_cnt; si++) {
                #pragma HLS UNROLL
                if (seg_list[si] == ts) {
                    seg_dsts[si][seg_dst_cnt[si]++] = b.dst[i];
                }
            }
        }

        // --- Phase B: parallel seg_insert ---
        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS UNROLL factor=4
            seg_insert(seg_buf[si], seg_dsts[si], seg_dst_cnt[si],
                       seg_buf[si], seg_valid_buf[si], seg_overflow[si]);
        }

        // --- Phase C: burst-write all modified segments + update helper ---
        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS PIPELINE II=SEG_WORDS_PER_SEG
            int base = seg_bases[si];
            int seg  = seg_list[si];
            for (int w = 0; w < SEG_WORDS_PER_SEG; w++) {
                #pragma HLS UNROLL
                hbm[base + w] = seg_buf[si][w];
            }
            // Update helper in local buffer
            int hw_idx = seg / 8, hw_slot = seg % 8;
            ap_uint<64> hv = 0;
            hv.range(31, 0)  = seg_buf[si][0].range(31, 0);
            hv.range(63, 32) = (ap_uint<32>)seg_valid_buf[si];
            hlp_buf[hw_idx].range(hw_slot * 64 + 63, hw_slot * 64) = hv;
        }

        // Write-back helper to HBM
        int hlp_base = (boff + HEADER_BYTES) / 64;
        for (int w = 0; w < hpw; w++) {
            #pragma HLS PIPELINE II=1
            hbm[hlp_base + w] = hlp_buf[w];
        }

        // Update edge_count + write back header
        ap_uint<512> new_edge_count = b.edge_count_word;
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            ec[v] += ec_delta[v];
            new_edge_count.range(v * 32 + 31, v * 32) = (ap_uint<32>)ec[v];
        }
        int wb_base = boff / 64;
        hbm[wb_base + 0] = b.range_word;       // header[0]: vertex ranges (unchanged)
        hbm[wb_base + 1] = new_edge_count;     // header[1]: updated edge counts
        hbm[wb_base + 2] = 0;
        hbm[wb_base + 3] = 0;

        // Push SEGMENT_REBALANCE tasks for overflowed segments
        bool has_rebalance = false;
        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS PIPELINE II=1
            if (!seg_overflow[si]) continue;
            rebalance_task_t t;
            t.pma        = pma;
            t.payload    = seg_list[si];
            t.start_byte = b.start_byte;
            t.range_word = b.range_word;
            task_fifo.write(t);
            has_rebalance = true;
        }

        if (has_rebalance) {
            // ACK marker → redistribution → redist_ack → forwarded upstream
            rebalance_task_t ack_task;
            ack_task.pma        = pma;
            ack_task.payload    = -2;
            ack_task.start_byte = 0;
            ack_task.range_word = 0;
            task_fifo.write(ack_task);
            pending_cnt++;
        } else {
            // No rebalance: ACK upstream directly
            ack_out.write((ap_uint<32>)pma);
        }
    }

    // --- STOP: send STOP to redistribution, then drain pending table ---
    rebalance_task_t stop_task;
    stop_task.pma        = 0;
    stop_task.payload    = -1;
    stop_task.start_byte = 0;
    stop_task.range_word = 0;
    task_fifo.write(stop_task);

    while (pending_cnt > 0) {
        if (!redist_ack_in.empty()) {
            ap_uint<32> ack_val = redist_ack_in.read();
            int ack_pma = (int)ack_val;
            if (ack_pma != -1) {
                pending_cnt--;
                ack_out.write((ap_uint<32>)ack_pma);
            }
        }
    }

    // Send STOP ACK upstream
    ack_out.write((ap_uint<32>)-1);
}

// ---- process_cmd (unchanged) ----

// void process_cmd(
//     hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
//     hls::stream<int> &cmd_count_stream,
//     hls::stream<PMA_UPDATE_MESSAGE> &cmd_batch_stream
//     // hls::stream<pma_idx_info> pma_info_stream
// )
// {
//     printf("PROCESS_CMD\n");
//     // int next_worker = 0;
//     while(true)
//     {
//         if (cmd_stream.empty()) { ap_wait(); continue; }
//         PMA_UPDATE_MESSAGE cmd_batch[MAX_CMD_NUM];
//         #pragma HLS ARRAY_PARTITION variable=cmd_batch complete
//         int cmd_count = 0;
//         bool done = false;
//         PMA_UPDATE_MESSAGE first = cmd_stream.read();
//         // switch (first.opcode)
//         // {
//             // case PMA_UPDATE_STOP:
//             //     if (first.last == 1) {
//             //         pma_idx_info info; info.pma_idx = 0; info.command_type = INSERT_STOP;
//             //         // for (int i = 0; i < BS_WORKER_NUM; i++) pma_info_stream[i].write(info);
//             //         pma_info_stream.write(info);
//             //         return;
//             //     }
//             //     break;
//             // case PMA_UPDATE_FLUSH_CACHE:
//             // {
//             //     pma_idx_info info; info.pma_idx = first.src >> 4; info.command_type = INSERT_FLUSH_CACHE;
//             //     // for (int i = 0; i < BS_WORKER_NUM; i++) pma_info_stream[i].write(info);
//             //     pma_info_stream.write(info);
//             //     break;
//             // }
//             // default:
//         // {

//         cmd_batch[cmd_count++] = first;
//         if (first.last == 1) done = true;
//         for (int i = 1; i < MAX_CMD_NUM; i++) {
//             if (done) break;
//             PMA_UPDATE_MESSAGE cmd = cmd_stream.read();
//             cmd_batch[i] = cmd; cmd_count++;
//             if (cmd.last == 1) done = true;
//         }
//         // pma_idx_info info; info.pma_idx = cmd_batch[0].src >> 4; info.command_type = INSERT_GOING;
//         // pma_info_stream[next_worker].write(info);
//         // pma_info_stream.write(info);

//         PMA_UPDATE_MESSAGE edge_info[MAX_CMD_NUM];
//         #pragma HLS ARRAY_PARTITION variable=edge_info complete
//         for (int i = 0; i < cmd_count; i++) edge_info[i] = cmd_batch[i];
//         for (int i = cmd_count; i < MAX_CMD_NUM; i++) {
//             edge_info[i].src = 0x7FFFFFFF; edge_info[i].dst = 0x7FFFFFFF;
//             edge_info[i].opcode = (PMA_UPDATE_OPCODE)0x7FFFFFFF;
//         }
//         #define BCS(a,b) do { \
//             PMA_UPDATE_MESSAGE _ta = edge_info[a], _tb = edge_info[b]; \
//             bool _gt = ((int)_ta.src > (int)_tb.src) || \
//                 ((int)_ta.src == (int)_tb.src && ((int)_ta.dst > (int)_tb.dst || \
//                 ((int)_ta.dst == (int)_tb.dst && (int)_ta.opcode > (int)_tb.opcode))); \
//             if (_gt) { edge_info[a] = _tb; edge_info[b] = _ta; } \
//         } while(0)
//         BCS( 0, 1); BCS( 2, 3); BCS( 4, 5); BCS( 6, 7);
//         BCS( 8, 9); BCS(10,11); BCS(12,13); BCS(14,15);
//         BCS( 0, 2); BCS( 1, 3); BCS( 4, 6); BCS( 5, 7);
//         BCS( 8,10); BCS( 9,11); BCS(12,14); BCS(13,15);
//         BCS( 1, 2); BCS( 5, 6); BCS( 9,10); BCS(13,14);
//         BCS( 0, 4); BCS( 1, 5); BCS( 2, 6); BCS( 3, 7);
//         BCS( 8,12); BCS( 9,13); BCS(10,14); BCS(11,15);
//         BCS( 2, 4); BCS( 3, 5); BCS(10,12); BCS(11,13);
//         BCS( 1, 2); BCS( 3, 4); BCS( 5, 6);
//         BCS( 9,10); BCS(11,12); BCS(13,14);
//         BCS( 0, 8); BCS( 1, 9); BCS( 2,10); BCS( 3,11);
//         BCS( 4,12); BCS( 5,13); BCS( 6,14); BCS( 7,15);
//         BCS( 4, 8); BCS( 5, 9); BCS( 6,10); BCS( 7,11);
//         BCS( 2, 4); BCS( 3, 5); BCS( 6, 8); BCS( 7, 9);
//         BCS(10,12); BCS(11,13);
//         BCS( 1, 2); BCS( 3, 4); BCS( 5, 6); BCS( 7, 8);
//         BCS( 9,10); BCS(11,12); BCS(13,14);
//         #undef BCS
//         // cmd_count_stream[next_worker].write(cmd_count);
//         // cmd_count_stream.write(cmd_count);
//         for (int i = 0; i < cmd_count ; i++)
//             // cmd_batch_stream[next_worker].write(edge_info[i]);
//             cmd_batch_stream.write(edge_info[i]);
//         if (first.opcode == PMA_UPDATE_STOP && first.last == 1) {
//             return;
//         }
//         // next_worker = (next_worker + 1) % BS_WORKER_NUM;
//         // }
//         // }
//     }
// }

// ---- Phase 4: Redistribution (rtss-style deferred rebalance) ----
// Task types (in payload[31:0]):
//   -1 (0xFFFFFFFF): STOP
//   -2 (0xFFFFFFFE): ACK marker
//   0x80000000 | v: ENLARGE_REBALANCE — redistribute full new vertex range
//   seg (>=0):       SEGMENT_REBALANCE — power-of-2 group expansion starting at seg

void redistribution_phase(
    ap_uint<512>* hbm,
    int data_off, int vi_wo,
    hls::stream<rebalance_task_t>& task_fifo,
    hls::stream<ap_uint<32>>& redist_ack_out)
{
    while (true) {
        rebalance_task_t task = task_fifo.read();
        int payload = task.payload;
        int pm  = task.pma;
        if (payload == -1) {  // STOP
            ap_uint<32> stop_ack = (ap_uint<32>)(-1);
            redist_ack_out.write(stop_ack);
            break;
        }
        if (payload == -2) {  // ACK marker: notify insert_phase
            redist_ack_out.write((ap_uint<32>)pm);
            continue;
        }

        bool is_enlarge = (payload & 0x80000000) != 0;

        // Use forwarded header from insert_phase (no VEC_INFO/header HBM reads)
        int pma_boff = task.start_byte - data_off;
        int hbase = pma_boff / 64;
        int total_segs = (int)task.range_word.range(15 * 32 + 31, 15 * 32);
        if (total_segs == 0) continue;
        int hpw = (total_segs + 7) / 8;
        int hlp_base = hbase + HEADER_WORDS;

        // Burst-read helper from HBM (current valid counts needed for redistribution)
        ap_uint<512> hlp_local[MAX_HELPER_WORDS];
        #pragma HLS bind_storage variable=hlp_local type=RAM_2P impl=URAM
        for (int _w = 0; _w < hpw; _w++) {
            #pragma HLS PIPELINE II=1
            hlp_local[_w] = hbm[hlp_base + _w];
        }

        // Vertex ranges from forwarded header
        int vr[VERTEX_PER_PMA];
        for (int vv = 0; vv < VERTEX_PER_PMA; vv++) {
            #pragma HLS UNROLL
            vr[vv] = (int)task.range_word.range(vv * 32 + 31, vv * 32);
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
        ap_uint<512> seg_buf_rd[2560];
        for (int _w = 0; _w < seg_words_total; _w++) {
            #pragma HLS PIPELINE II=1
            seg_buf_rd[_w] = hbm[seg_base_left + _w];
        }

        // B: extract edges
        int edge_buf[10240];
        #pragma HLS bind_storage variable=edge_buf type=RAM_2P impl=URAM
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
        ap_uint<512> seg_buf_wr[512];
        #pragma HLS bind_storage variable=seg_buf_wr type=RAM_2P impl=BRAM
        for (int i = 0; i < seg_words_total; i += SEG_WORDS_PER_SEG) {
            #pragma HLS PIPELINE II=1
            seg_buf_wr[i + 0] = EMPTY_SEG_ROM[0];
            seg_buf_wr[i + 1] = EMPTY_SEG_ROM[1];
            seg_buf_wr[i + 2] = EMPTY_SEG_ROM[2];
            seg_buf_wr[i + 3] = EMPTY_SEG_ROM[3];
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
        for (int _w = 0; _w < seg_words_total; _w++) {
            #pragma HLS PIPELINE II=1
            hbm[seg_base_left + _w] = seg_buf_wr[_w];
        }

        // E: update local helper + burst-write back
        for (int s = left; s < right; s++) {
            int fill = base + ((s - left) < extra ? 1 : 0);
            ap_uint<64> hv2 = 0;
            hv2.range(31, 0)  = (ap_uint<32>)(fill > 0 ? (int)seg_buf_wr[(s-left)*SEG_WORDS_PER_SEG].range(31, 0) : UNUSED_ICON);
            hv2.range(63, 32) = (ap_uint<32>)fill;
            hlp_local[s / 8].range((s % 8) * 64 + 63, (s % 8) * 64) = hv2;
        }
        for (int _w = 0; _w < hpw; _w++) {
            #pragma HLS PIPELINE II=1
            hbm[hlp_base + _w] = hlp_local[_w];
        }

        #undef HLP_VC
    }
}

// ---- Task Reader: replaces generator + router ----
// Reads edges from per-worker HBM task buffer, groups by PMA into batches.
// Two-phase design: burst-read chunks into local BRAM, then process from BRAM
// to avoid data-dependent break that prevents HLS burst inference.

#define STANDALONE_POOL_SIZE 16
#define POOL_SLOT_CAPACITY 256  // max edges per slot (never flush mid-way)
#define TASK_BURST_CHUNK 256    // edges per burst chunk

// Lock table: small associative in-flight tracker (like router's in_flight)
// Only stores PMAs that are currently locked — works for any PMA index.
#define LOCK_TABLE_SIZE 16
struct LockTable {
    int pmas[LOCK_TABLE_SIZE];   // -1 = free
    int count;
};
static void lock_init(LockTable &lt) {
    for (int i = 0; i < LOCK_TABLE_SIZE; i++) {
        lt.pmas[i] = -1;
    }
    lt.count = 0;
}
static bool lock_is_locked(LockTable &lt, int pma) {
    for (int i = 0; i < LOCK_TABLE_SIZE; i++) {
        if (lt.pmas[i] == pma) return true;
    }
    return false;
}
static void lock_acquire(LockTable &lt, int pma) {
    for (int i = 0; i < LOCK_TABLE_SIZE; i++) {
        if (lt.pmas[i] == -1) {
            lt.pmas[i] = pma;
            lt.count++;
            return;
        }
    }
}
static void lock_release(LockTable &lt, int pma) {
    for (int i = 0; i < LOCK_TABLE_SIZE; i++) {
        if (lt.pmas[i] == pma) {
            lt.pmas[i] = -1;
            lt.count--;
        }
    }
}

static void task_reader(
    int* task_buffer,
    int edge_count,
    LockTable &lock_tbl,
    hls::stream<ap_uint<32>>& ack_in,
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_out,
    hls::stream<PMA_UPDATE_MESSAGE>& online_in
) {
    // ---- Pool: ring buffers, same pattern as router ----
    struct Slot {
        int pma;   // -1 = free
        int head;  // enqueue position
        int tail;  // dequeue position
        int count; // edges in queue
        PMA_UPDATE_MESSAGE edges[POOL_SLOT_CAPACITY];
    };
    Slot pool[STANDALONE_POOL_SIZE];
    #pragma HLS bind_storage variable=pool type=RAM_2P impl=BRAM

    // Tag arrays — small, let HLS decide placement (no partition)
    int pool_pma[STANDALONE_POOL_SIZE];
    int pool_cnt[STANDALONE_POOL_SIZE];

    for (int i = 0; i < STANDALONE_POOL_SIZE; i++) {
        pool[i].pma = -1; pool[i].head = 0; pool[i].tail = 0; pool[i].count = 0;
        pool_pma[i] = -1; pool_cnt[i] = 0;
    }

    // Slot ops — no INLINE (let HLS decide); sequential scan, no UNROLL needed
    auto slot_find = [&](int pma) -> int {
        for (int i = 0; i < STANDALONE_POOL_SIZE; i++) {
            if (pool_pma[i] == pma && pool_cnt[i] < POOL_SLOT_CAPACITY) return i;
        }
        for (int i = 0; i < STANDALONE_POOL_SIZE; i++) {
            if (pool_pma[i] == pma) return i;
        }
        return -1;
    };
    auto slot_alloc = [&](int pma) -> int {
        int free_slot = -1;
        for (int i = 0; i < STANDALONE_POOL_SIZE; i++) {
            if (pool_pma[i] == -1) free_slot = i;
        }
        if (free_slot != -1) {
            pool_pma[free_slot] = pma; pool_cnt[free_slot] = 0;
            pool[free_slot].pma = pma; pool[free_slot].head = 0;
            pool[free_slot].tail = 0; pool[free_slot].count = 0;
        }
        return free_slot;
    };
    auto slot_enq = [&](int s, PMA_UPDATE_MESSAGE m) {
        pool[s].edges[pool[s].head] = m;
        pool[s].head = (pool[s].head + 1) % POOL_SLOT_CAPACITY;
        pool[s].count++;
        pool_cnt[s] = pool[s].count;
    };
    auto slot_flush_one_batch = [&](int s, hls::stream<PMA_UPDATE_MESSAGE>& out) {
        int n = pool[s].count;
        if (n == 0) { pool_pma[s] = -1; return; }
        int batch = (n < MAX_CMD_NUM) ? n : MAX_CMD_NUM;
        for (int i = 0; i < batch; i++) {
            #pragma HLS PIPELINE II=1
            PMA_UPDATE_MESSAGE m = pool[s].edges[pool[s].tail];
            pool[s].tail = (pool[s].tail + 1) % POOL_SLOT_CAPACITY;
            m.last = (i == batch - 1) ? 1 : 0;
            out.write(m);
        }
        pool[s].count -= batch;
        pool_cnt[s] = pool[s].count;
        if (pool[s].count == 0) {
            pool_pma[s] = -1; pool[s].pma = -1;
        }
    };
    auto slot_evict = [&](int for_pma) -> int {
        int best = -1;
        for (int i = 0; i < STANDALONE_POOL_SIZE; i++) {
            int ev_pma = pool_pma[i];
            if (ev_pma == -1 || ev_pma == for_pma) continue;
            if (lock_is_locked(lock_tbl, ev_pma)) continue;
            if (best == -1 || pool_cnt[i] > 0) best = i;
        }
        return best;
    };

    // Burst-read buffer
    int burst_buf[TASK_BURST_CHUNK * 3];
    #pragma HLS bind_storage variable=burst_buf type=RAM_2P impl=BRAM

    int processed = 0;
    int base = 0;
    int live_edges = edge_count;  // grows with online updates
    bool online_done = false;
    bool sentinel_hit = false;
    
    lock_init(lock_tbl);
    while ((base < live_edges || !online_done) && !sentinel_hit) {
        // ---- Poll online updates, append to task HBM ----
        while   (!online_in.empty() && !online_done) {
            PMA_UPDATE_MESSAGE om = online_in.read();
            if (om.opcode == PMA_UPDATE_STOP) {
                online_done = true;  // any STOP terminates online stream
            } else {
                int wp = live_edges * 3;
                task_buffer[wp]     = om.src;
                task_buffer[wp + 1] = om.dst;
                task_buffer[wp + 2] = (int)om.opcode;
                live_edges++;
            }
        }

        if (base >= live_edges) continue;  // wait for more data

        int chunk = live_edges - base;
        if (chunk > TASK_BURST_CHUNK) chunk = TASK_BURST_CHUNK;

        // Phase A: burst-read chunk
        int m_axi_off = base * 3;
        int words = chunk * 3;
        for (int j = 0; j < words; j++) {
            #pragma HLS PIPELINE II=1
            burst_buf[j] = task_buffer[m_axi_off + j];
        }

        // Phase B: process from BRAM with router-style enqueue+retry
        for (int j = 0; j < chunk && !sentinel_hit; j++) {

            // Phase-independent: poll ACKs
            while (!ack_in.empty()) {
                ap_uint<32> val = ack_in.read();
                int ack_pma = (int)val;
                if (ack_pma != -1) lock_release(lock_tbl, ack_pma);
            }

            int src = burst_buf[j * 3];
            int dst = burst_buf[j * 3 + 1];
            int op  = burst_buf[j * 3 + 2];
            if (src == -1) { sentinel_hit = true; break; }

            int pma = src / VERTEX_PER_PMA;
            int slot = slot_find(pma);
            if (slot == -1) slot = slot_alloc(pma);

            // If slot is full, flush one batch (wait for lock first)
            if (slot != -1 && pool_cnt[slot] >= MAX_CMD_NUM) {
                while (lock_is_locked(lock_tbl, pma)) {
                    while (!ack_in.empty()) {
                        ap_uint<32> val = ack_in.read();
                        int ap = (int)val;
                        if (ap != -1) lock_release(lock_tbl, ap);
                    }
                }
                slot_flush_one_batch(slot, cmd_out);
                lock_acquire(lock_tbl, pma);
                // After flush, slot might be freed (if empty). If still used, find/create.
                if (pool_pma[slot] != pma) slot = slot_alloc(pma);
            }

            // If no slot (pool full), evict with retry
            if (slot == -1) {
                int ev = slot_evict(pma);
                if (ev != -1) {
                    int ev_pma = pool_pma[ev];
                    while (lock_is_locked(lock_tbl, ev_pma)) {
                        while (!ack_in.empty()) {
                            ap_uint<32> val = ack_in.read();
                            int ap = (int)val;
                            if (ap != -1) lock_release(lock_tbl, ap);
                        }
                    }
                    // Drain victim completely
                    while (pool_cnt[ev] > 0) {
                        slot_flush_one_batch(ev, cmd_out);
                    }
                    lock_acquire(lock_tbl, ev_pma);
                    // Reuse slot
                    pool[ev].pma = pma; pool[ev].head = 0;
                    pool[ev].tail = 0; pool[ev].count = 0;
                    pool_pma[ev] = pma; pool_cnt[ev] = 0;
                    slot = ev;
                }
            }

            // Enqueue edge (with retry if slot still unavailable)
            if (slot == -1) continue; // shouldn't happen; edge lost as last resort

            PMA_UPDATE_MESSAGE msg;
            msg.src = src; msg.dst = dst;
            msg.opcode = (PMA_UPDATE_OPCODE)op; msg.last = 0;
            slot_enq(slot, msg);
            processed++;
        }
        base += chunk;
    }

    // ---- Drain: flush all remaining slots with lock sync ----
    bool any_left = true;
    while (any_left) {
        while (!ack_in.empty()) {
            ap_uint<32> val = ack_in.read();
            int ack_pma = (int)val;
            if (ack_pma != -1) lock_release(lock_tbl, ack_pma);
        }
        any_left = false;
        for (int j = 0; j < STANDALONE_POOL_SIZE; j++) {
            if (pool_cnt[j] == 0) continue;
            int pma = pool_pma[j];
            if (lock_is_locked(lock_tbl, pma)) { any_left = true; continue; }
            lock_acquire(lock_tbl, pma);
            slot_flush_one_batch(j, cmd_out);
            if (pool_cnt[j] > 0) any_left = true;
        }
    }

    // DEBUG: write processed count to HBM
    task_buffer[edge_count * 3 + 3] = processed;

    // Send STOP
    PMA_UPDATE_MESSAGE stop_msg;
    stop_msg.src = -1; stop_msg.dst = -1;
    stop_msg.opcode = PMA_UPDATE_STOP; stop_msg.last = 1;
    cmd_out.write(stop_msg);
}
// ---- Top-level ----

extern "C"
{
    void pma_insert_edge_standalone(
        ap_uint<512>* hbm_gate,
        ap_uint<512>* hbm_bs,     // BS worker read-only
        ap_uint<512>* hbm_rw,     // insert phase read-write
        ap_uint<512>* hbm_red,    // redistribution read-write (separate port)
        int* task_buffer,          // per-worker edge task buffer
        int pma_raw_data_start,
        int pma_vecinfo_start,
        int edge_count,            // number of edges in this batch
        int bitmap_offset,         // byte offset in hbm_gate for bitmap
        hls::stream<ap_axiu<96,0,0,0>>& alloc_cmd_stream,
        hls::stream<ap_axiu<64,0,0,0>>& alloc_rsp_stream,
        hls::stream<ap_axiu<ALLOC_BITMAP_STORAGE_WORD_BITS,0,0,0>>& bitmap_init,  // bitmap → alloc_access
        hls::stream<PMA_UPDATE_MESSAGE>& online_in    // online updates from router
    )
    {
        #pragma HLS INTERFACE m_axi port=hbm_gate offset=slave bundle=gmem6 max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=hbm_bs   offset=slave bundle=gmem0 max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=hbm_rw   offset=slave bundle=gmem4 max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=hbm_red  offset=slave bundle=gmem5 max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=task_buffer offset=slave bundle=gmem7 max_read_burst_length=256 max_write_burst_length=256

        #pragma HLS INTERFACE s_axilite port=hbm_gate bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_bs   bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_rw   bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_red  bundle=control
        #pragma HLS INTERFACE s_axilite port=task_buffer bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_raw_data_start bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_vecinfo_start bundle=control
        #pragma HLS INTERFACE s_axilite port=edge_count bundle=control
        #pragma HLS INTERFACE s_axilite port=bitmap_offset bundle=control
        #pragma HLS INTERFACE s_axilite port=return bundle=control
        #pragma HLS INTERFACE axis port=alloc_cmd_stream
        #pragma HLS INTERFACE axis port=alloc_rsp_stream
        #pragma HLS INTERFACE axis port=bitmap_init
        #pragma HLS INTERFACE axis port=online_in

        hls::stream<PMA_UPDATE_MESSAGE> cmd_from_reader;
        #pragma HLS STREAM variable=cmd_from_reader depth=16

        hls::stream<PMA_UPDATE_MESSAGE> cmd_batch_stream;
        #pragma HLS STREAM variable=cmd_batch_stream depth=16

        hls::stream<bs_batch_t> bs_out;
        #pragma HLS STREAM variable=bs_out depth=8

        hls::stream<rebalance_task_t> task_fifo;
        #pragma HLS STREAM variable=task_fifo depth=64

        hls::stream<ap_uint<32>> redist_ack_stream;
        #pragma HLS STREAM variable=redist_ack_stream depth=16

        hls::stream<ap_uint<32>> dummy_ack_stream;
        #pragma HLS STREAM variable=dummy_ack_stream depth=16

        hls::stream<fwd_header_t> hdr_fwd;
        #pragma HLS STREAM variable=hdr_fwd depth=4

        // Per-PMA lock table (small associative set, works for any PMA index)
        LockTable lock_tbl;

        #pragma HLS DATAFLOW

        task_reader(task_buffer, edge_count, lock_tbl, dummy_ack_stream, cmd_from_reader, online_in);

        // pma_gate handles bitmap init internally (reads hbm_gate, sends to alloc_access)
        // before entering its main command-processing loop.
        // Forwards header to bs_worker via hdr_fwd to avoid redundant HBM reads.
        pma_gate(hbm_gate, pma_raw_data_start, pma_vecinfo_start / 64,
                 bitmap_offset,
                 cmd_from_reader, cmd_batch_stream,
                 alloc_cmd_stream, alloc_rsp_stream, bitmap_init, hdr_fwd);

        bs_worker(hbm_bs, pma_raw_data_start,
                  hdr_fwd,
                  cmd_batch_stream, bs_out);

        insert_phase(hbm_rw, pma_raw_data_start, pma_vecinfo_start / 64,
                     bs_out,
                     task_fifo, redist_ack_stream, dummy_ack_stream);

        redistribution_phase(hbm_red, pma_raw_data_start, pma_vecinfo_start / 64,
                            task_fifo, redist_ack_stream);

        // No post-DATAFLOW bitmap write-back — host reconstructs bitmap from data
    }
}
