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

static void flush_fe_cache_line(
    ap_uint<512>* hbm, int data_off, int line,
    ap_uint<512> fe_cache[BS_FE_CACHE_LINES][BS_FE_CACHE_SEGMENTS / FE_SOA_ENTRIES_PER_WORD],
    int cache_start, int cache_segs)
{
    #pragma HLS INLINE off
    int hbase = (cache_start - data_off) / 64;
    int fe_base = hbase + HEADER_WORDS;
    int words = fe_words_for(cache_segs);
    for (int w = 0; w < words; w++) {
        #pragma HLS PIPELINE II=1
        hbm[fe_base + w] = fe_cache[line][w];
    }
    ap_wait();
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
    // 全分区: 二分读 + shift 写在寄存器上, 避免 BRAM 单口读写竞争 (原 II=2)
    int edge_buf[EDGE_SLOTS_PER_SEG];
    #pragma HLS ARRAY_PARTITION variable=edge_buf complete
    for (int k = 0; k < EDGE_SLOTS_PER_SEG; k++) {
        #pragma HLS UNROLL
        int wi = k / 16, slot = k % 16;
        edge_buf[k] = (k < valid) ? (int)seg_in[wi].range(slot * 32 + 31, slot * 32) : UNUSED_ICON;
    }

    // Insert each new edge using binary search for O(log N) position finding
    // edge_buf[0..valid-1] is sorted ascending.
    for (int ei = 0; ei < dst_cnt; ei++) {
        #pragma HLS PIPELINE II=1
        int dst = dsts[ei];
        if (valid >= EDGE_SLOTS_PER_SEG) break;
        unsigned lo = 0, hi = (unsigned)valid;
        while (lo < hi) {
            #pragma HLS PIPELINE II=1
            unsigned mid = (lo + hi) >> 1;
            if ((unsigned)edge_buf[mid] < (unsigned)dst)
                lo = mid + 1;
            else
                hi = mid;
        }
        int pos = (int)lo;
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

// ---- Phase 1: BS Worker ----
// Reads ONLY the first_edge area from HBM (1 entry per segment) and binary
// searches directly for the exact target segment of each edge.

void bs_worker(
    ap_uint<512>* hbm,
    int data_off, int vi_wo,
    hls::stream<PMA_UPDATE_MESSAGE>& edge_in,
    hls::stream<bs_batch_t>& bs_out,
    hls::stream<fe_cache_update_t>& fe_update_in
)
{
    // Buffers declared outside while for HLS lifetime analysis
    PMA_UPDATE_MESSAGE edges[MAX_CMD_NUM];
    int vr[VERTEX_PER_PMA];
    #pragma HLS ARRAY_PARTITION variable=vr complete

    const int FE_CACHE_WORDS = BS_FE_CACHE_SEGMENTS / FE_SOA_ENTRIES_PER_WORD;
    ap_uint<512> fe_cache[BS_FE_CACHE_LINES][FE_CACHE_WORDS];
    #pragma HLS bind_storage variable=fe_cache type=RAM_2P impl=URAM
    bool cache_valid[BS_FE_CACHE_LINES];
    bool cache_loaded[BS_FE_CACHE_LINES];
    bool cache_dirty[BS_FE_CACHE_LINES];
    int cache_pma[BS_FE_CACHE_LINES];
    int cache_start[BS_FE_CACHE_LINES];
    int cache_segs[BS_FE_CACHE_LINES];
    ap_uint<32> cache_score[BS_FE_CACHE_LINES];
    ap_uint<32> cache_wave_updates[BS_FE_CACHE_LINES];
    for (int line = 0; line < BS_FE_CACHE_LINES; line++) {
        cache_valid[line] = false;
        cache_loaded[line] = false;
        cache_dirty[line] = false;
        cache_pma[line] = -1;
        cache_start[line] = 0;
        cache_segs[line] = 0;
        cache_score[line] = 0;
        cache_wave_updates[line] = 0;
    }

    while (true) {
        // Updates are issued by insert_phase before its batch ACK.  Therefore
        // all feedback for the preceding batch is available before task_reader
        // can send another batch for the same PMA.
        fe_cache_update_t feu;
        while (fe_update_in.read_nb(feu)) {
            for (int line = 0; line < BS_FE_CACHE_LINES; line++) {
                if (feu.seg < 0) {
                    // Redistribution may move/repack the complete PMA and
                    // rebuild every FE entry.  A per-segment patch is not
                    // sufficient; invalidate the whole cached line after
                    // the redistribution ACK has reached insert_phase.
                    if (cache_pma[line] == feu.pma) {
                        cache_valid[line] = false;
                        cache_loaded[line] = false;
                        cache_dirty[line] = false;
                    }
                } else if (cache_valid[line] && cache_loaded[line] &&
                    cache_pma[line] == feu.pma && feu.seg < cache_segs[line]) {
                    int word = feu.seg / FE_SOA_ENTRIES_PER_WORD;
                    int slot = feu.seg % FE_SOA_ENTRIES_PER_WORD;
                    fe_cache[line][word].range(slot * 32 + 31, slot * 32)
                        = (ap_uint<32>)feu.first_edge;
                    cache_dirty[line] = true;
                }
            }
        }
        PMA_UPDATE_MESSAGE first = edge_in.read();
        if (first.opcode == PMA_UPDATE_STOP && first.last == 1) {
            bs_batch_t s; s.pma_idx = -1; s.count = 0; bs_out.write(s); break;
        }
        if (first.opcode == PMA_UPDATE_CACHE_HINT) {
            int hit = -1;
            int empty = -1;
            int victim = 0;
            ap_uint<32> victim_value = ~ap_uint<32>(0);
            for (int line = 0; line < BS_FE_CACHE_LINES; line++) {
                if (cache_valid[line] && cache_pma[line] == first.src) hit = line;
                if (!cache_valid[line] && empty < 0) empty = line;
                ap_uint<32> predicted = cache_score[line] - (cache_score[line] >> 3)
                                              + cache_wave_updates[line];
                if (cache_valid[line] && predicted < victim_value) {
                    victim_value = predicted;
                    victim = line;
                }
            }
            if (hit < 0 && (empty >= 0 || (ap_uint<32>)first.dst > victim_value)) {
                int line = empty >= 0 ? empty : victim;
                if (cache_valid[line] && cache_loaded[line] && cache_dirty[line]) {
                    flush_fe_cache_line(hbm, data_off, line, fe_cache,
                                        cache_start[line], cache_segs[line]);
                }
                cache_valid[line] = true;
                cache_loaded[line] = false;
                cache_dirty[line] = false;
                cache_pma[line] = first.src;
                cache_start[line] = 0;
                cache_segs[line] = 0;
                cache_score[line] = 0;
                cache_wave_updates[line] = 0;
            }
            continue;
        }
        if (first.opcode == PMA_UPDATE_WAVE_END) {
            for (int line = 0; line < BS_FE_CACHE_LINES; line++) {
                if (cache_valid[line] && cache_loaded[line] && cache_dirty[line]) {
                    flush_fe_cache_line(hbm, data_off, line, fe_cache,
                                        cache_start[line], cache_segs[line]);
                    cache_dirty[line] = false;
                }
                ap_uint<33> next = (ap_uint<33>)cache_score[line]
                                 - (cache_score[line] >> 3)
                                 + cache_wave_updates[line];
                cache_score[line] = next[32] ? ~ap_uint<32>(0) : (ap_uint<32>)next;
                cache_wave_updates[line] = 0;
            }
            bs_batch_t s; s.pma_idx = CACHE_WAVE_ACK; s.count = 0; bs_out.write(s);
            continue;
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

        int cache_line = -1;
        for (int line = 0; line < BS_FE_CACHE_LINES; line++) {
            if (cache_valid[line] && cache_pma[line] == pma) cache_line = line;
        }
        int total_segs = vr[VERTEX_PER_PMA - 1];
        if (cache_line >= 0 && total_segs > BS_FE_CACHE_SEGMENTS) {
            cache_valid[cache_line] = false;
            cache_loaded[cache_line] = false;
            cache_line = -1;
        }
        if (cache_line >= 0 && (!cache_loaded[cache_line] ||
                               cache_start[cache_line] != start_byte ||
                               cache_segs[cache_line] != total_segs)) {
            if (cache_loaded[cache_line] && cache_dirty[cache_line]) {
                flush_fe_cache_line(hbm, data_off, cache_line, fe_cache,
                                    cache_start[cache_line], cache_segs[cache_line]);
                cache_dirty[cache_line] = false;
            }
            int words = fe_words_for(total_segs);
            for (int w = 0; w < words; w++) {
                #pragma HLS PIPELINE II=1
                fe_cache[cache_line][w] = hbm[fe_hbm_base + w];
            }
            cache_loaded[cache_line] = true;
            cache_dirty[cache_line] = false;
            cache_start[cache_line] = start_byte;
            cache_segs[cache_line] = total_segs;
        }
        if (cache_line >= 0) cache_wave_updates[cache_line] += cnt;

        // Process edges — binary search on the per-segment first_edge entries:
        // result IS the exact segment (no fine search needed)
        bs_batch_t res;
        res.pma_idx = pma; res.op_mask = 0; res.count = cnt;
        res.fe_cache_resident = (cache_line >= 0);
        res.start_byte = start_byte;
        res.range_word = range_word;
        res.edge_count_word = edge_count_word;

        // 注意: 外层不流水 (内层二分是可变 trip 循环, 流水外层会拉大 II)。
        // 每条边串行: 定位缓存槽 (UNROLL 并行比较) + 片上二分 (~log2 步 × 2 拍)。
        for (int i = 0; i < MAX_CMD_NUM; i++) {
            if (i < cnt) {
                PMA_UPDATE_MESSAGE e = edges[i];
                res.dst[i] = e.dst;
                res.local_src[i] = e.src & 0xF;
                int op = (int)e.opcode;
                if (op == PMA_UPDATE_INSERT_EDGE /* || op == PMA_UPDATE_DELETE_EDGE */) {
                    int ls = e.src & 0xF;
                    int vs = (ls == 0) ? 0 : vr[ls - 1];
                    int ve = vr[ls];
                    int vertex_segs = ve - vs;
                    // 无符号半开区间 [0, vertex_segs): (lo+hi)>>1 直接移位, 避免带符号
                    // 除法链 (关键路径 ~2.5ns)。vertex_segs=0 → hi=0 → 循环不进入。
                    // seg_rel 语义: 找到相等段 → mid; 未找到 → 最后一个 < dst 的段 (lo-1)。
                    unsigned lo = 0, hi = (unsigned)vertex_segs;
                    int seg_rel = -1;
                    if (cache_line >= 0) {
                        // Whole-PMA FE cache uses absolute segment indices.
                        while (lo < hi) {
                            #pragma HLS PIPELINE II=1
                            unsigned mid = (lo + hi) >> 1;
                            int fe_idx = vs + (int)mid;
                            int fe_val = (int)fe_cache[cache_line][fe_idx >> 4]
                                                          .range((fe_idx & 0xF) * 32 + 31, (fe_idx & 0xF) * 32);
                            if (fe_val == e.dst) { seg_rel = (int)mid; break; }
                            else if (fe_val < e.dst) { lo = mid + 1; }
                            else { hi = mid; }
                        }
                        // 未找到: 插入段 = 最后一个 < dst 的段; 全 FE > dst → 0
                        if (seg_rel == -1 && lo > 0) seg_rel = (int)lo - 1;
                    } else {
                        // fallback: HBM 二分 (vertex_segs > FE_CACHE_ENTRIES, 罕见)
                        while (lo < hi) {
                            #pragma HLS PIPELINE off
                            unsigned mid = (lo + hi) >> 1;
                            int fe_idx = vs + (int)mid;
                            int fe_word_off = fe_hbm_base + fe_idx / FE_SOA_ENTRIES_PER_WORD;
                            int fe_slot = fe_idx % FE_SOA_ENTRIES_PER_WORD;
                            ap_uint<512> fe_word = hbm[fe_word_off];
                            int fe_val = (int)fe_word.range(fe_slot * 32 + 31, fe_slot * 32);
                            if (fe_val == e.dst) { seg_rel = (int)mid; break; }
                            else if (fe_val < e.dst) { lo = mid + 1; }
                            else { hi = mid; }
                        }
                        if (seg_rel == -1 && lo > 0) seg_rel = (int)lo - 1;
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
    hls::stream<ap_uint<32>>& ack_out,
    hls::stream<fe_cache_update_t>& fe_update_out)
{
    int pending_cnt = 0;

    while (true) {
        // --- read_nb: poll redistribution ACK every cycle ---
        if (!redist_ack_in.empty()) {
            ap_uint<32> ack_val = redist_ack_in.read();
            int ack_pma = (int)ack_val;
            if (ack_pma != -1) {
                pending_cnt--;
                fe_cache_update_t inv;
                inv.pma = ack_pma; inv.seg = -1; inv.first_edge = UNUSED_ICON;
                fe_update_out.write(inv);
                ack_out.write((ap_uint<32>)ack_pma);
            }
        }

        if (bs_in.empty()) { ap_wait(); continue; }
        bs_batch_t b = bs_in.read();

        if (b.pma_idx == -1) { break; }
        if (b.pma_idx == CACHE_WAVE_ACK) {
            // A wave is complete only after all overflow redistribution ACKs
            // have returned.  This makes the HBM image authoritative before
            // task_reader starts the next wave's pre-expand reads.
            while (pending_cnt > 0) {
                if (!redist_ack_in.empty()) {
                    int ack_pma = (int)redist_ack_in.read();
                    if (ack_pma != -1) {
                        pending_cnt--;
                        fe_cache_update_t inv;
                        inv.pma = ack_pma; inv.seg = -1; inv.first_edge = UNUSED_ICON;
                        fe_update_out.write(inv);
                        ack_out.write((ap_uint<32>)ack_pma);
                    }
                } else ap_wait();
            }
            ack_out.write((ap_uint<32>)CACHE_WAVE_ACK);
            continue;
        }

        int pma = b.pma_idx;

        // Use forwarded header from bs_worker
        // (no VEC_INFO read, no header HBM read — only segments + VC are touched)
        int boff = b.start_byte - data_off;
        int hbase = boff / 64;
        int tseg = (int)b.range_word.range(15 * 32 + 31, 15 * 32);
        int fe_words = fe_words_for(tseg);
        int vc_words = vc_words_for(tseg);
        int total_hlp_words = fe_words + vc_words;
        int fe_hbm_base = hbase + HEADER_WORDS;
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
        for (int v = 0; v < VERTEX_PER_PMA; v++) 
        {
            #pragma HLS PIPELINE II=1
            ec_delta[v] = 0;
        }

        int seg_list[16], seg_cnt = 0;
        bool seg_overflow[16];
        for (int i = 0; i < 16; i++) 
        {
            #pragma HLS PIPELINE II=1
            seg_overflow[i] = false;
        }

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

        // Sort the small hit list so adjacent segments can be fetched as one
        // monotonic burst.  The old code emitted one 4-word transaction per
        // segment even when all targets were adjacent; this keeps II=1 but
        // leaves the HBM command overhead dominant.
        for (int i = 1; i < seg_cnt; i++) {
            #pragma HLS PIPELINE II=1
            int key = seg_list[i];
            int j = i - 1;
            while (j >= 0 && seg_list[j] > key) {
                seg_list[j + 1] = seg_list[j];
                j--;
            }
            seg_list[j + 1] = key;
        }

        int data_base = (boff + HEADER_BYTES + total_hlp_words * 64) / 64;
        for (int si = 0; si < seg_cnt; si++) 
        {
            #pragma HLS PIPELINE II=1
            seg_bases[si] = data_base + seg_list[si] * SEG_WORDS_PER_SEG;
        }
        for (int si = 0; si < seg_cnt; si++) {
            for (int w = 0; w < SEG_WORDS_PER_SEG; w++) {
#pragma HLS PIPELINE II=1
                seg_buf[si][w] = hbm[seg_bases[si] + w];
            }
        }

        // --- Phase A.5: distribute edges in batch to per-segment dst arrays ---
        int  seg_dsts[16][16];
        int  seg_dst_cnt[16];
        #pragma HLS ARRAY_PARTITION variable=seg_dst_cnt complete
        for (int i = 0; i < 16; i++) 
        {
            #pragma HLS PIPELINE II=1
            seg_dst_cnt[i] = 0;
        }

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

        // --- Phase C1: write through only the modified segments. ---
        for (int si = 0; si < seg_cnt; si++) {
            for (int w = 0; w < SEG_WORDS_PER_SEG; w++) {
#pragma HLS PIPELINE II=1
                hbm[seg_bases[si] + w] = seg_buf[si][w];
            }
        }

        // FE is write-through metadata.  Deduplicate FE words, patch each
        // touched segment's first edge, and mirror it into any resident FE
        // cache line through the feedback stream.
        int fe_uniq[16], fe_seg_grp[16], fe_uniq_cnt = 0;
        #pragma HLS ARRAY_PARTITION variable=fe_uniq complete
        #pragma HLS ARRAY_PARTITION variable=fe_seg_grp complete
        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS UNROLL
            int wix = seg_list[si] / FE_SOA_ENTRIES_PER_WORD;
            int grp = -1;
            for (int k = 0; k < 16; k++) {
                #pragma HLS UNROLL
                if (k < fe_uniq_cnt && fe_uniq[k] == wix) grp = k;
            }
            if (grp == -1) { fe_uniq[fe_uniq_cnt] = wix; grp = fe_uniq_cnt++; }
            fe_seg_grp[si] = grp;
        }
        ap_uint<512> fe_new_word[16];
        #pragma HLS ARRAY_PARTITION variable=fe_new_word complete
        for (int k = 0; k < fe_uniq_cnt; k++) {
            #pragma HLS PIPELINE II=1
            fe_new_word[k] = hbm[fe_hbm_base + fe_uniq[k]];
        }
        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS UNROLL
            int first_edge = seg_valid_buf[si] > 0
                ? (int)seg_buf[si][0].range(31, 0) : UNUSED_ICON;
            int slot = seg_list[si] % FE_SOA_ENTRIES_PER_WORD;
            fe_new_word[fe_seg_grp[si]].range(slot * 32 + 31, slot * 32)
                = (ap_uint<32>)first_edge;
        }
        // Resident FE lines are kept dirty on chip until WAVE_END.  A PMA
        // outside the FE cache still uses the direct HBM fallback.
        if (!b.fe_cache_resident) {
            for (int k = 0; k < fe_uniq_cnt; k++) {
                #pragma HLS PIPELINE II=1
                hbm[fe_hbm_base + fe_uniq[k]] = fe_new_word[k];
            }
        }
        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS PIPELINE II=1
            fe_cache_update_t u;
            u.pma = pma; u.seg = seg_list[si];
            u.first_edge = seg_valid_buf[si] > 0
                ? (int)seg_buf[si][0].range(31, 0) : UNUSED_ICON;
            fe_update_out.write(u);
        }

        // --- Phase C2: VC RMW — 去重 VC word, 一次读 → 片上 patch → 一次写 ---
        // 每个 VC word 装 16 段的 valid_count (word idx = seg/16)。逐段直接在 HBM 上 RMW
        // (读-改-写) 会因地址数据相关 (可能 alias) 让 HLS 保守串行化, II 高达 3017。
        // 改为: 先建去重后的 word 列表 (<=16), 三段各自独立 — 读 (II=1) → 片上 patch
        // (unroll) → 写 (II=1), 消除跨迭代 RMW 依赖。
        int vc_uniq[16];
        #pragma HLS ARRAY_PARTITION variable=vc_uniq complete
        int vc_seg_grp[16];
        #pragma HLS ARRAY_PARTITION variable=vc_seg_grp complete
        int vc_uniq_cnt = 0;
        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS UNROLL
            int wix = seg_list[si] / VC_SOA_ENTRIES_PER_WORD;
            int grp = -1;
            for (int k = 0; k < 16; k++) {
                #pragma HLS UNROLL
                if (k < vc_uniq_cnt && vc_uniq[k] == wix) grp = k;
            }
            if (grp == -1) {
                vc_uniq[vc_uniq_cnt] = wix;
                grp = vc_uniq_cnt;
                vc_uniq_cnt++;
            }
            vc_seg_grp[si] = grp;
        }
        ap_uint<512> vc_new_word[16];
        #pragma HLS ARRAY_PARTITION variable=vc_new_word complete
        for (int k = 0; k < vc_uniq_cnt; k++) {
            #pragma HLS PIPELINE II=1
            vc_new_word[k] = hbm[vc_hbm_base + vc_uniq[k]];   // 每个被触 word 只读一次
        }
        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS UNROLL
            int grp  = vc_seg_grp[si];
            int slot = seg_list[si] % VC_SOA_ENTRIES_PER_WORD;
            vc_new_word[grp].range(slot * 32 + 31, slot * 32) = (ap_uint<32>)seg_valid_buf[si];
        }
        for (int k = 0; k < vc_uniq_cnt; k++) {
            #pragma HLS PIPELINE II=1
            hbm[vc_hbm_base + vc_uniq[k]] = vc_new_word[k];   // 每个被触 word 只写一次
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

        // Detect overflow only after segment/FE/VC/header writes are issued.
        bool has_rebalance = false;
        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS PIPELINE II=1
            if (!seg_overflow[si]) continue;
            has_rebalance = true;
        }

        // Redistribution consumes HBM immediately.  For an overflow batch,
        // force the touched FE words through before publishing its task;
        // ordinary resident-cache batches remain deferred until WAVE_END.
        if (has_rebalance && b.fe_cache_resident) {
            for (int k = 0; k < fe_uniq_cnt; k++) {
                #pragma HLS PIPELINE II=1
                hbm[fe_hbm_base + fe_uniq[k]] = fe_new_word[k];
            }
            ap_wait();
        }

        if (has_rebalance) {
            // Only now publish overflow tasks: redistribution must observe the
            // fully committed PMA image.
            for (int si = 0; si < seg_cnt; si++) {
                #pragma HLS PIPELINE II=1
                if (!seg_overflow[si]) continue;
                rebalance_task_t t;
                t.pma        = pma;
                t.payload    = seg_list[si];
                t.start_byte = b.start_byte;
                t.range_word = b.range_word;
                task_fifo.write(t);
            }
            // ACK marker → redistribution → redist_ack → forwarded upstream
            rebalance_task_t ack_task;
            ack_task.pma        = pma;
            ack_task.payload    = -2;
            ack_task.start_byte = 0;
            ack_task.range_word = 0;
            task_fifo.write(ack_task);
            pending_cnt++;
        } else {
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
                    fe_cache_update_t inv;
                    inv.pma = ack_pma; inv.seg = -1; inv.first_edge = UNUSED_ICON;
                    fe_update_out.write(inv);
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
        hls::stream<fe_cache_update_t> fe_update;
        #pragma HLS STREAM variable=fe_update depth=64

        #pragma HLS DATAFLOW

        bs_worker(hbm_bs, pma_raw_data_start, pma_vecinfo_start / 64,
                  edge_in, bs_out, fe_update);

        insert_phase(hbm_rw, pma_raw_data_start, pma_vecinfo_start / 64,
                     bs_out,
                     task_out, redist_ack_in, ack_out, fe_update);
    }
}
