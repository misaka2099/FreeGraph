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

// ---- VEC_INFO access ----

static VEC_INFO vi_read(ap_uint<512>* hbm, int vi_word_off, int pma_idx)
{
    int wi = pma_idx / 8, si = pma_idx % 8;
    ap_uint<512> w = hbm[vi_word_off + wi];
    VEC_INFO vi;
    vi.range(63, 0) = w.range(si * 64 + 63, si * 64);
    return vi;
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
    for (int k = 0; k < EDGE_SLOTS_PER_SEG; k++) {
        #pragma HLS UNROLL
        int wi = k / 16, slot = k % 16;
        edge_buf[k] = (k < valid) ? (int)seg_in[wi].range(slot * 32 + 31, slot * 32) : UNUSED_ICON;
    }

    // Insert each new edge using binary search for O(log N) position finding
    // edge_buf[0..valid-1] is sorted ascending — binary search replaces 48-way
    // parallel comparator+popcount with ~6 sequential comparisons, saving LUTs
    // and shortening the critical path at negligible latency cost (HBM access
    // dominates the pipeline anyway).
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

// ---- Phase 1: BS Worker ----
// Reads ONLY the first_edge area from HBM (1 entry per segment) and binary
// searches directly for the exact target segment of each edge.

void bs_worker(
    ap_uint<512>* hbm,
    int data_off, int vi_wo,
    hls::stream<PMA_UPDATE_MESSAGE>& edge_in,
    hls::stream<bs_batch_t>& bs_out
)
{
    // Buffers declared outside while for HLS lifetime analysis
    PMA_UPDATE_MESSAGE edges[MAX_CMD_NUM];
    int vr[VERTEX_PER_PMA];
    #pragma HLS ARRAY_PARTITION variable=vr complete

    while (true) {
        PMA_UPDATE_MESSAGE first = edge_in.read();
        if (first.opcode == PMA_UPDATE_STOP && first.last == 1) {
            bs_batch_t s; s.pma_idx = -1; s.count = 0; bs_out.write(s); break;
        }

        int pma = first.src >> 4;
        // Read remaining edges until last flag; count = position of last
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

        // Read VEC_INFO + header directly from HBM.
        // PMA 已由 redistribution 的 pre_expand 预分配/扩容 (vi != 0)。
        VEC_INFO vi = vi_read(hbm, vi_wo, pma);
        int start_byte = (int)vi.range(31, 0);
        int byte_off = start_byte - data_off;
        int hbase = byte_off / 64;
        ap_uint<512> range_word = hbm[hbase];
        ap_uint<512> edge_count_word = hbm[hbase + 1];

        // Extract vertex_range from header
        for (int v = 0; v < VERTEX_PER_PMA; v++)
            #pragma HLS UNROLL
            vr[v] = (int)range_word.range(v * 32 + 31, v * 32);

        // FE area base (word offset): 1 entry per segment, 16 entries per 512-bit word
        int fe_hbm_base = hbase + HEADER_WORDS;

        // Process edges — direct HBM binary search on the per-segment first_edge
        // entries: result IS the exact segment (no fine search needed)
        bs_batch_t res;
        res.pma_idx = pma; res.op_mask = 0; res.count = cnt;
        res.start_byte = start_byte;
        res.range_word = range_word;
        res.edge_count_word = edge_count_word;

        for (int i = 0; i < MAX_CMD_NUM; i++) {
            #pragma HLS PIPELINE II=1
            if (i < cnt) {
                PMA_UPDATE_MESSAGE e = edges[i];
                res.dst[i] = e.dst;
                res.local_src[i] = e.src & 0xF;
                int op = (int)e.opcode;
                if (op == PMA_UPDATE_INSERT_EDGE /* || op == PMA_UPDATE_DELETE_EDGE */) {
                    int ls = e.src & 0xF;
                    int vs = (ls == 0) ? 0 : vr[ls - 1];
                    int ve = vr[ls];
                    // Direct HBM binary search for FE entry (1 entry per segment,
                    // 16 entries per 512-bit word). Result is the relative segment
                    // offset within this vertex → absolute = vs + seg_rel.
                    int lo = 0, hi = ve - vs - 1;
                    int seg_rel = -1;
                    while (lo <= hi) {
                        #pragma HLS PIPELINE off
                        int mid = (lo + hi) / 2;
                        int fe_idx = vs + mid;
                        int fe_word_off = fe_hbm_base + fe_idx / FE_SOA_ENTRIES_PER_WORD;
                        int fe_slot = fe_idx % FE_SOA_ENTRIES_PER_WORD;
                        ap_uint<512> fe_word = hbm[fe_word_off];
                        int fe_val = (int)fe_word.range(fe_slot * 32 + 31, fe_slot * 32);
                        if (fe_val == e.dst) { seg_rel = mid; break; }
                        else if (fe_val < e.dst) { lo = mid + 1; seg_rel = mid; }
                        else { hi = mid - 1; }
                    }
                    if (seg_rel == -1) seg_rel = 0;
                    res.target_seg[i] = vs + seg_rel;
                } else {
                    res.target_seg[i] = 0;
                }
            }
        }
        bs_out.write(res);
    }
}

// ---- Phase 2+3: Insert (segment modification) ----
// bs_worker already resolved the EXACT target segment (FE 1:1 search), so no
// fine search is needed here. Modified segments are read, patched via seg_insert,
// written back, and their valid_count is RMW-updated in the SoA VC area.
// Overflowed segments are pushed to task_fifo for deferred redistribution.
// ACKs are sent by redistribution_phase after all tasks are processed.

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

        // Use forwarded header from bs_worker
        // (no VEC_INFO read, no header HBM read — only segments + VC are touched)
        int boff = b.start_byte - data_off;
        int hbase = boff / 64;
        int tseg = (int)b.range_word.range(15 * 32 + 31, 15 * 32);
        int fe_words = fe_words_for(tseg);
        int vc_words = vc_words_for(tseg);
        int total_hlp_words = fe_words + vc_words;
        int vc_hbm_base = hbase + HEADER_WORDS + fe_words;

        // Vertex ranges and edge counts from forwarded header
        int ec[VERTEX_PER_PMA], vr[VERTEX_PER_PMA];
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            vr[v] = (int)b.range_word.range(v * 32 + 31, v * 32);
            ec[v] = (int)b.edge_count_word.range(v * 32 + 31, v * 32);
        }

        // --- Process segment modifications ---
        int ec_delta[VERTEX_PER_PMA];
        for (int v = 0; v < VERTEX_PER_PMA; v++) ec_delta[v] = 0;

        int seg_list[16], seg_cnt = 0;
        bool seg_overflow[16];
        for (int i = 0; i < 16; i++) seg_overflow[i] = false;

        // bs_worker already resolved the EXACT segment → dedup only
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
        #pragma HLS bind_storage variable=seg_buf type=RAM_2P impl=BRAM
        int seg_bases[16];
        int seg_valid_buf[16];

        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS PIPELINE II=1
            int seg = seg_list[si];
            int base = (boff + HEADER_BYTES + total_hlp_words * 64 + seg * SEG_BYTES) / 64;
            seg_bases[si] = base;
            for (int w = 0; w < SEG_WORDS_PER_SEG; w++) {
                #pragma HLS UNROLL
                seg_buf[si][w] = hbm[base + w];
            }
        }

        // --- Phase A.5: distribute edges in batch to per-segment dst arrays ---
        int  seg_dsts[16][16];
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

        // --- Phase C: burst-write all modified segments + VC RMW ---
        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS PIPELINE II=1
            int base = seg_bases[si];
            int seg  = seg_list[si];
            for (int w = 0; w < SEG_WORDS_PER_SEG; w++) {
                #pragma HLS UNROLL
                hbm[base + w] = seg_buf[si][w];
            }
            // RMW this segment's valid_count entry in the SoA VC area (1:1 per segment)
            int vc_word_idx = seg / VC_SOA_ENTRIES_PER_WORD;
            int vc_slot     = seg % VC_SOA_ENTRIES_PER_WORD;
            ap_uint<512> vc_word = hbm[vc_hbm_base + vc_word_idx];
            vc_word.range(vc_slot * 32 + 31, vc_slot * 32) = (ap_uint<32>)seg_valid_buf[si];
            hbm[vc_hbm_base + vc_word_idx] = vc_word;
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

// ---- Top-level ----

extern "C"
{
    void bs_insert_kernel(
        ap_uint<512>* hbm_bs,
        ap_uint<512>* hbm_rw,
        int pma_raw_data_start,
        int pma_vecinfo_start,
        hls::stream<PMA_UPDATE_MESSAGE>& edge_in,
        hls::stream<rebalance_task_t>& task_out,
        hls::stream<ap_uint<32>>& redist_ack_in,
        hls::stream<ap_uint<32>>& ack_out)
    {
        #pragma HLS INTERFACE m_axi port=hbm_bs offset=slave bundle=gmem0 max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=hbm_rw offset=slave bundle=gmem4 max_read_burst_length=256 max_write_burst_length=256

        #pragma HLS INTERFACE s_axilite port=hbm_bs bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_rw bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_raw_data_start bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_vecinfo_start bundle=control
        #pragma HLS INTERFACE s_axilite port=return bundle=control
        #pragma HLS INTERFACE axis port=edge_in
        #pragma HLS INTERFACE axis port=task_out
        #pragma HLS INTERFACE axis port=redist_ack_in
        #pragma HLS INTERFACE axis port=ack_out

        hls::stream<bs_batch_t> bs_out;
        #pragma HLS STREAM variable=bs_out depth=8

        #pragma HLS DATAFLOW

        bs_worker(hbm_bs, pma_raw_data_start, pma_vecinfo_start / 64,
                  edge_in, bs_out);

        insert_phase(hbm_rw, pma_raw_data_start, pma_vecinfo_start / 64,
                     bs_out,
                     task_out, redist_ack_in, ack_out);
    }
}
