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
    #pragma HLS ARRAY_PARTITION variable=edge_buf complete
    for (int k = 0; k < EDGE_SLOTS_PER_SEG; k++) {
        #pragma HLS UNROLL
        int wi = k / 16, slot = k % 16;
        edge_buf[k] = (k < valid) ? (int)seg_in[wi].range(slot * 32 + 31, slot * 32) : UNUSED_ICON;
    }

    // Insert each new edge (int-by-int shift register)
    for (int ei = 0; ei < dst_cnt; ei++) {
        int dst = dsts[ei];
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
    hls::stream<ap_axiu<64,0,0,0>>& alloc_rsp_stream)
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
        #pragma HLS UNROLL
        if (delta_edge[i] == 0) continue;
        int old_count = (int)edge_count_word.range(i * 32 + 31, i * 32);
        int new_count = old_count + delta_edge[i];
        int seg_start = (i == 0) ? 0 : (int)range_word.range((i - 1) * 32 + 31, (i - 1) * 32);
        int seg_end = (int)range_word.range(i * 32 + 31, i * 32);

        int seg_len = seg_end - seg_start;
        int max_edge = density_capacity(seg_len);
        if (new_count > max_edge)
        {
            delta_range[i] = 3*seg_len;
            total_delta += delta_range[i];
            enlarge = true;
        }
    }
    if (enlarge)
    {
        hls::print("ENLARGE trig\n");
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

        ap_uint<512> old_range_word = range_word;
        int acc = 0;
        for (int i = 0 ; i < 16 ; i++)
        {
            #pragma HLS UNROLL
            acc += delta_range[i];
            int old_vr = (int)range_word.range(i * 32 + 31, i * 32);
            int new_vr = old_vr + acc;
            range_word.range(i * 32 + 31, i * 32) = new_vr;
        }
        ap_wait();
        ap_axiu<64,0,0,0> arsp2 = alloc_rsp_stream.read();
        int nstart = (int)arsp2.data.range(31, 0);

        int old_boff = (int)vi.range(31, 0) - pma_raw_data_start;
        int old_hpw  = (old_tseg + 7) / 8;
        int old_seg_base = (old_boff + HEADER_BYTES + old_hpw * 64) / 64;
        int old_seg_words = old_tseg * SEG_WORDS_PER_SEG;
        for (int i = 0; i < old_seg_words; i++) {
            #pragma HLS PIPELINE II=1
            pma_buffer[i] = hbm_gate[old_seg_base + i];
        }

        int nseg_base = nstart / 64 + HEADER_WORDS + nhpw;
        int buf_pos = 0;
        for (int v = 0; v < 16; v++) {
            int ovs = (v == 0) ? 0 : (int)old_range_word.range((v-1)*32+31, (v-1)*32);
            int ove = (int)old_range_word.range(v*32+31, v*32);
            int nvs = (v == 0) ? 0 : (int)range_word.range((v-1)*32+31, (v-1)*32);
            int ocnt = ove - ovs;
            for (int i = 0; i < ocnt * SEG_WORDS_PER_SEG; i++) {
                #pragma HLS PIPELINE II=1
                hbm_gate[nseg_base + nvs * SEG_WORDS_PER_SEG + i] = pma_buffer[buf_pos + i];
            }
            buf_pos += ocnt * SEG_WORDS_PER_SEG;
        }

        for (int v = 0; v < 16; v++) {
            int ovs = (v == 0) ? 0 : (int)old_range_word.range((v-1)*32+31, (v-1)*32);
            int ove = (int)old_range_word.range(v*32+31, v*32);
            int nvs = (v == 0) ? 0 : (int)range_word.range((v-1)*32+31, (v-1)*32);
            int nve = (int)range_word.range(v*32+31, v*32);
            int ocnt = ove - ovs;
            int extra = nve - nvs - ocnt;
            for (int i = 0; i < extra * SEG_WORDS_PER_SEG; i++) {
                #pragma HLS PIPELINE II=1
                hbm_gate[nseg_base + (nvs + ocnt) * SEG_WORDS_PER_SEG + i] =
                    make_empty_seg(i % SEG_WORDS_PER_SEG);
            }
        }

        ap_uint<512> nhlp[MAX_HELPER_WORDS];
        ap_uint<64> empty_entry = ((ap_uint<64>)(ap_uint<32>)0 << 32) | (ap_uint<64>)(ap_uint<32>)UNUSED_ICON;
        for (int w = 0; w < nhpw; w++) {
            nhlp[w] = 0;
            for (int z = 0; z < 8; z++) nhlp[w].range(z*64+63, z*64) = empty_entry;
        }
        int old_hlp_base = (old_boff + HEADER_BYTES) / 64;
        ap_uint<512> old_hlp[MAX_HELPER_WORDS];
        for (int w = 0; w < old_hpw; w++) {
            #pragma HLS PIPELINE II=1
            old_hlp[w] = hbm_gate[old_hlp_base + w];
        }
        for (int v = 0; v < 16; v++) {
            int ovs = (v == 0) ? 0 : (int)old_range_word.range((v-1)*32+31, (v-1)*32);
            int ove = (int)old_range_word.range(v*32+31, v*32);
            int nvs = (v == 0) ? 0 : (int)range_word.range((v-1)*32+31, (v-1)*32);
            for (int s = 0; s < ove - ovs; s++) {
                int oseg = ovs + s, nseg = nvs + s;
                int ow = oseg / 8, os = oseg % 8;
                int nw = nseg / 8, ns = nseg % 8;
                ap_uint<64> hv = old_hlp[ow].range(os*64+63, os*64);
                nhlp[nw].range(ns*64+63, ns*64) = hv;
            }
        }

        // FREE old PMA (after all reads from old location are complete)
        int obytes = (HEADER_WORDS + old_hpw + old_tseg * SEG_WORDS_PER_SEG) * 64;
        ap_axiu<96,0,0,0> fcmd;
        fcmd.data.range(31, 0)  = old_boff;
        fcmd.data.range(63, 32) = obytes;
        fcmd.data.range(95, 64) = 2;
        fcmd.last = 0; fcmd.keep = 0xFFF;
        alloc_cmd_stream.write(fcmd);

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
    }
}

void pma_gate(
    ap_uint<512> *hbm_gate,
    int pma_raw_data_start,
    int pma_vecinfo_start,
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_in,
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_out,
    hls::stream<ap_axiu<96,0,0,0>>& alloc_cmd_stream,
    hls::stream<ap_axiu<64,0,0,0>>& alloc_rsp_stream)
{
    ap_uint<512> pma_buffer[4096];
    #pragma HLS bind_storage variable=pma_buffer type=RAM_2P impl=URAM
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
        hls::print("GATE vi=%d\n", (unsigned)(int)vi.range(31,0));
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
            ap_wait();
            ap_axiu<64,0,0,0> arsp = alloc_rsp_stream.read();
            int nstart = (int)arsp.data.range(31, 0);

            // Header[0]: vertex_range — vertex v owns segment v
            ap_uint<512> hdr0 = 0;
            for (int v = 0; v < VERTEX_PER_PMA; v++) {
                #pragma HLS UNROLL
                hdr0.range(v * 32 + 31, v * 32) = v + 1;
            }
            pma_buffer[0] = hdr0;
            pma_buffer[1] = 0;  // edge_count all zero
            pma_buffer[2] = 0;
            pma_buffer[3] = 0;

            // Helper: valid_count=0, first_dst=UNUSED_ICON for all 8 slots per word
            ap_uint<64> empty_entry = ((ap_uint<64>)(ap_uint<32>)0 << 32) | (ap_uint<64>)(ap_uint<32>)UNUSED_ICON;
            ap_uint<512> empty_helper = 0;
            for (int z = 0; z < 8; z++) {
                #pragma HLS UNROLL
                empty_helper.range(z * 64 + 63, z * 64) = empty_entry;
            }
            for (int w = 0; w < ihlp; w++) {
                #pragma HLS UNROLL
                pma_buffer[HEADER_WORDS + w] = empty_helper;
            }

            // Segments: 16 empty segments, 4 words each (make_empty_seg template)
            int seg_off = HEADER_WORDS + ihlp;
            for (int i = 0; i < isegs * SEG_WORDS_PER_SEG; i++) {
                #pragma HLS PIPELINE II=1
                pma_buffer[seg_off + i] = make_empty_seg(i % SEG_WORDS_PER_SEG);
            }
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
        }
        //check pma enlargement
        else
        {
            hls::print("GATE else enter\n");
            pma_gate_handle_existing(hbm_gate, pma_raw_data_start, pma_vecinfo_start,
                                     pma_idx, vi, cmd_batch, cmd_count, pma_buffer,
                                     alloc_cmd_stream, alloc_rsp_stream);
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
    int data_off, int vi_wo,
    hls::stream<PMA_UPDATE_MESSAGE>& edge_in,
    hls::stream<bs_batch_t>& bs_out
)
{
    while (true) {
        // Read first edge to determine batch type and pma_idx
        #pragma HLS PIPELINE II=1
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

        // VEC_INFO guaranteed non-zero (empty PMA handled upstream by pma_gate)
        VEC_INFO vi = vi_read(hbm, vi_wo, pma);
        int start_byte = (int)vi.range(31, 0);
        int byte_off   = start_byte - data_off;

        // Read header then helper
        int hbase = byte_off / 64;
        ap_uint<512> hdr[HEADER_WORDS];
        ap_uint<512> hlp_buf[MAX_HELPER_WORDS];
        #pragma HLS bind_storage variable=hlp_buf type=RAM_2P impl=BRAM
        for (int w = 0; w < HEADER_WORDS; w++) {
            #pragma HLS PIPELINE II=1
            hdr[w] = hbm[hbase + w];
        }
        int total_segs = (int)hdr[0].range(15 * 32 + 31, 15 * 32);
        int hlp_words = (total_segs + HELPER_ENTRIES_PER_WORD - 1) / HELPER_ENTRIES_PER_WORD;
        if (hlp_words > MAX_HELPER_WORDS) hlp_words = MAX_HELPER_WORDS;
        for (int w = 0; w < hlp_words; w++) {
            #pragma HLS PIPELINE II=1
            hlp_buf[w] = hbm[hbase + HEADER_WORDS + w];
        }

        // Unpack bs_helper — partition 16-way for parallel sub_search
        int bs_helper[10240];
        #pragma HLS bind_storage variable=bs_helper type=RAM_2P impl=BRAM
        #pragma HLS ARRAY_PARTITION variable=bs_helper cyclic factor=16
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

        // Process edges — 16-way parallel binary search
        bs_batch_t res;
        res.pma_idx = pma; res.op_mask = 0; res.count = cnt;

        for (int i = 0; i < MAX_CMD_NUM; i++) {
            #pragma HLS UNROLL
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
    hls::stream<ap_uint<64>>& task_fifo,
    hls::stream<ap_uint<32>>& redist_ack_in,
    hls::stream<COMMAND_STREAM>& ack_out)
{
    int pending_cnt = 0;

    while (true) {
        #pragma HLS PIPELINE II=1

        // --- read_nb: poll redistribution ACK every cycle ---
        if (!redist_ack_in.empty()) {
            ap_uint<32> ack_val = redist_ack_in.read();
            int ack_pma = (int)ack_val;
            if (ack_pma != -1) {
                pending_cnt--;
                COMMAND_STREAM up_ack;
                up_ack.data = ack_pma; up_ack.keep = 0xF;
                up_ack.strb = 0xF; up_ack.user = 0; up_ack.last = 0;
                ack_out.write(up_ack);
            }
        }

        if (bs_in.empty()) { ap_wait(); continue; }
        bs_batch_t b = bs_in.read();

        if (b.pma_idx == -1) { break; }

        int pma = b.pma_idx;

        // Read header+helper in one contiguous burst from HBM
        VEC_INFO vi = vi_read(hbm, vi_wo, pma);
        int boff = (int)vi.range(31, 0) - data_off;
        int hbase = boff / 64;
        // Read header first to determine hpw, then burst helper
        ap_uint<512> hdr[HEADER_WORDS];
        ap_uint<512> hlp_buf[MAX_HELPER_WORDS];
        #pragma HLS bind_storage variable=hlp_buf type=RAM_2P impl=BRAM
        int total_hdr_hlp = HEADER_WORDS + MAX_HELPER_WORDS;
        for (int w = 0; w < total_hdr_hlp; w++) {
            #pragma HLS PIPELINE II=1
            ap_uint<512> d = hbm[hbase + w];
            if (w < HEADER_WORDS) hdr[w] = d;
            else hlp_buf[w - HEADER_WORDS] = d;
        }
        int tseg = (int)hdr[0].range(15 * 32 + 31, 15 * 32);
        int hpw  = (tseg + 7) / 8;
        if (hpw > MAX_HELPER_WORDS) hpw = MAX_HELPER_WORDS;

        int ec[VERTEX_PER_PMA], vr[VERTEX_PER_PMA];
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            vr[v] = (int)hdr[0].range(v * 32 + 31, v * 32);
            ec[v] = (int)hdr[1].range(v * 32 + 31, v * 32);
        }

        // --- Process segment modifications ---
        int ec_delta[VERTEX_PER_PMA];
        #pragma HLS ARRAY_PARTITION variable=ec_delta complete
        for (int v = 0; v < VERTEX_PER_PMA; v++) ec_delta[v] = 0;

        int seg_list[16], seg_cnt = 0;
        #pragma HLS ARRAY_PARTITION variable=seg_list complete
        bool seg_overflow[16];
        #pragma HLS ARRAY_PARTITION variable=seg_overflow complete
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
        #pragma HLS ARRAY_PARTITION variable=seg_buf complete dim=2
        #pragma HLS bind_storage variable=seg_buf type=RAM_2P impl=BRAM
        int seg_bases[16];
        #pragma HLS ARRAY_PARTITION variable=seg_bases complete
        int seg_valid_buf[16];
        #pragma HLS ARRAY_PARTITION variable=seg_valid_buf complete

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
        #pragma HLS ARRAY_PARTITION variable=seg_dsts complete dim=2
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
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            ec[v] += ec_delta[v];
            hdr[1].range(v * 32 + 31, v * 32) = (ap_uint<32>)ec[v];
        }
        int wb_base = boff / 64;
        for (int w = 0; w < HEADER_WORDS; w++) {
            #pragma HLS UNROLL
            hbm[wb_base + w] = hdr[w];
        }

        // Push SEGMENT_REBALANCE tasks for overflowed segments
        bool has_rebalance = false;
        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS PIPELINE II=1
            if (!seg_overflow[si]) continue;
            ap_uint<64> t;
            t.range(31, 0) = seg_list[si];
            t.range(63, 32) = pma;
            task_fifo.write(t);
            has_rebalance = true;
        }

        if (has_rebalance) {
            // ACK marker → redistribution → redist_ack → forwarded upstream
            ap_uint<64> ack_task;
            ack_task.range(31, 0) = ap_uint<32>(-2);
            ack_task.range(63, 32) = pma;
            task_fifo.write(ack_task);
            pending_cnt++;
        } else {
            // No rebalance: ACK upstream directly
            COMMAND_STREAM up_ack;
            up_ack.data = pma; up_ack.keep = 0xF;
            up_ack.strb = 0xF; up_ack.user = 0; up_ack.last = 0;
            ack_out.write(up_ack);
        }
    }

    // --- STOP: send STOP to redistribution, then drain pending table ---
    ap_uint<64> stop_task;
    stop_task.range(31, 0) = ap_uint<32>(-1);
    stop_task.range(63, 32) = 0;
    task_fifo.write(stop_task);

    while (pending_cnt > 0) {
        if (!redist_ack_in.empty()) {
            ap_uint<32> ack_val = redist_ack_in.read();
            int ack_pma = (int)ack_val;
            if (ack_pma != -1) {
                pending_cnt--;
                COMMAND_STREAM up_ack;
                up_ack.data = ack_pma; up_ack.keep = 0xF;
                up_ack.strb = 0xF; up_ack.user = 0; up_ack.last = 0;
                ack_out.write(up_ack);
            }
        }
    }

    // Send STOP ACK upstream
    COMMAND_STREAM stop_ack;
    stop_ack.data = -1; stop_ack.keep = 0xF; stop_ack.strb = 0xF; stop_ack.user = 0; stop_ack.last = 0;
    ack_out.write(stop_ack);
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
    hls::stream<ap_uint<64>>& task_fifo,
    hls::stream<ap_uint<32>>& redist_ack_out)
{
    while (true) {
        ap_uint<64> task = task_fifo.read();
        int payload = (int)task.range(31, 0);
        int pm  = (int)task.range(63, 32);
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
        ap_uint<512> seg_buf_rd[2560];
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
        ap_uint<512>* hbm_gate,
        ap_uint<512>* hbm_bs,     // BS worker read-only
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
        #pragma HLS INTERFACE m_axi port=hbm_gate offset=slave bundle=gmem6 max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=hbm_bs   offset=slave bundle=gmem0 max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=hbm_rw   offset=slave bundle=gmem4 max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=hbm_red  offset=slave bundle=gmem5 max_read_burst_length=256 max_write_burst_length=256

        #pragma HLS INTERFACE s_axilite port=hbm_gate bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_bs   bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_rw   bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_red  bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_raw_data_start bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_vecinfo_start bundle=control
        #pragma HLS INTERFACE s_axilite port=return bundle=control
        #pragma HLS INTERFACE axis port=cmd_stream
        #pragma HLS INTERFACE axis port=ack_back_to_router
        #pragma HLS INTERFACE axis port=alloc_cmd_stream
        #pragma HLS INTERFACE axis port=alloc_rsp_stream

        hls::stream<PMA_UPDATE_MESSAGE> cmd_batch_stream;
        #pragma HLS STREAM variable=cmd_batch_stream depth=16

        hls::stream<bs_batch_t> bs_out;
        #pragma HLS STREAM variable=bs_out depth=8

        hls::stream<ap_uint<64>> task_fifo;
        #pragma HLS STREAM variable=task_fifo depth=64

        hls::stream<ap_uint<32>> redist_ack_stream;
        #pragma HLS STREAM variable=redist_ack_stream depth=16

        #pragma HLS DATAFLOW

        pma_gate(hbm_gate, pma_raw_data_start, pma_vecinfo_start / 64,
                 cmd_stream, cmd_batch_stream,
                 alloc_cmd_stream, alloc_rsp_stream);

        bs_worker(hbm_bs, pma_raw_data_start, pma_vecinfo_start / 64,
                  cmd_batch_stream, bs_out);

        insert_phase(hbm_rw, pma_raw_data_start, pma_vecinfo_start / 64,
                     bs_out,
                     task_fifo, redist_ack_stream, ack_back_to_router);

        redistribution_phase(hbm_red, pma_raw_data_start, pma_vecinfo_start / 64,
                            task_fifo, redist_ack_stream);
    }
}
