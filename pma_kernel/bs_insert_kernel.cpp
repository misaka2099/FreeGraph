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
    // 全分区: 二分读 + shift 写在寄存器上, 避免 BRAM 单口读写竞争 (原 II=2)
    int edge_buf[EDGE_SLOTS_PER_SEG];
    #pragma HLS ARRAY_PARTITION variable=edge_buf complete
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
        // 无符号半开区间 [0, valid): (lo+hi)>>1 直接移位, 避免带符号 (lo+hi)/2 的
        // 符号修正链 (add→bitselect→sub→lshr→zext→select ≈ 2.5ns 关键路径)。
        // edge_buf 中 UNUSED_ICON(-1) 无符号比较 = 0xFFFFFFFF → 视为 +∞, 语义保持。
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

        // 片上 FE 缓存 (per-batch): 二分搜索先在 HBM 上 burst 预取该 vertex 的 FE 区
        // 到 fe_cache, 再片上二分。原实现直接 HBM 二分 — 每步 77 周期读延迟不可重叠,
        // 每条边 ~log2(segs)×77 周期。片上二分每步 2 拍, 快 ~20 倍。
        // 批内边按 src 排序 → 同 vertex 连续, unique vertex 数通常 1~2。
        // 缓存上限 FE_CACHE_ENTRIES 段/vertex (4096 = 256 字 = 16KB BRAM);
        // 超限 vertex fallback 原 HBM 二分 (保正确性, 超大 vertex 罕见)。
        const int FE_CACHE_ENTRIES = 4096;
        const int FE_CACHE_WORDS = FE_CACHE_ENTRIES / FE_SOA_ENTRIES_PER_WORD;
        ap_uint<512> fe_cache[16][FE_CACHE_WORDS];
        #pragma HLS bind_storage variable=fe_cache type=RAM_2P impl=BRAM
        int unique_v[16];
        #pragma HLS ARRAY_PARTITION variable=unique_v complete
        int unique_cnt = 0;

        // ---- 收集批内 unique vertex (仅 INSERT 边; 超限 vertex 不进缓存 → fallback HBM) ----
        for (int i = 0; i < cnt; i++) {
            #pragma HLS PIPELINE II=1
            int ls = edges[i].src & 0xF;
            int op = (int)edges[i].opcode;
            if (op != PMA_UPDATE_INSERT_EDGE) continue;
            int vs_t = (ls == 0) ? 0 : vr[ls - 1];
            if (vr[ls] - vs_t > FE_CACHE_ENTRIES) continue;   // 超限 → HBM 二分
            bool found = false;
            for (int u = 0; u < unique_cnt; u++) {
                #pragma HLS UNROLL
                if (unique_v[u] == ls) found = true;
            }
            if (!found && unique_cnt < 16) {
                unique_v[unique_cnt] = ls;
                unique_cnt++;
            }
        }

        // ---- 预取每个 unique vertex 的 FE 区 (burst 读, 内层流水 II=1) ----
        for (int u = 0; u < unique_cnt; u++) {
            int ls = unique_v[u];
            int vs = (ls == 0) ? 0 : vr[ls - 1];
            int ve = vr[ls];
            int vertex_segs = ve - vs;
            if (vertex_segs > FE_CACHE_ENTRIES) continue;   // 超限 → fallback HBM 二分
            int fe_words = (vertex_segs + FE_SOA_ENTRIES_PER_WORD - 1) / FE_SOA_ENTRIES_PER_WORD;
            int fe_base = fe_hbm_base + vs / FE_SOA_ENTRIES_PER_WORD;
            for (int w = 0; w < fe_words; w++) {
                #pragma HLS PIPELINE II=1
                fe_cache[u][w] = hbm[fe_base + w];
            }
        }

        // Process edges — binary search on the per-segment first_edge entries:
        // result IS the exact segment (no fine search needed)
        bs_batch_t res;
        res.pma_idx = pma; res.op_mask = 0; res.count = cnt;
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
                    // 定位该 vertex 的缓存槽
                    int slot_u = -1;
                    for (int u = 0; u < unique_cnt; u++) {
                        #pragma HLS UNROLL
                        if (unique_v[u] == ls) slot_u = u;
                    }
                    // 无符号半开区间 [0, vertex_segs): (lo+hi)>>1 直接移位, 避免带符号
                    // 除法链 (关键路径 ~2.5ns)。vertex_segs=0 → hi=0 → 循环不进入。
                    // seg_rel 语义: 找到相等段 → mid; 未找到 → 最后一个 < dst 的段 (lo-1)。
                    unsigned lo = 0, hi = (unsigned)vertex_segs;
                    int seg_rel = -1;
                    int vs_word_base = vs >> 4;   // 缓存字基址 (循环不变量)
                    if (slot_u >= 0) {
                        // 片上二分 (缓存字 = 绝对字 (vs>>4) 起的相对偏移, 槽 = 绝对 entry % 16)
                        while (lo < hi) {
                            #pragma HLS PIPELINE II=2
                            unsigned mid = (lo + hi) >> 1;
                            int fe_idx = vs + (int)mid;
                            int fe_val = (int)fe_cache[slot_u][(fe_idx >> 4) - vs_word_base]
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
            int seg = seg_list[si];
            int base = (boff + HEADER_BYTES + total_hlp_words * 64 + seg * SEG_BYTES) / 64;
            seg_bases[si] = base;
            // 内层流水 II=1: 每段 4 字连续读 → 推断成 4-beat burst。外层串行逐段。
            // (原外层 PIPELINE + 内层 UNROLL 会因 4 次并行 HBM 访问端口受限, II 上不去)
            for (int w = 0; w < SEG_WORDS_PER_SEG; w++) {
                #pragma HLS PIPELINE II=1
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

        // --- Phase C1: burst-write all modified segments (pure write, no RMW) ---
        for (int si = 0; si < seg_cnt; si++) {
            int base = seg_bases[si];
            for (int w = 0; w < SEG_WORDS_PER_SEG; w++) {
                #pragma HLS PIPELINE II=1
                hbm[base + w] = seg_buf[si][w];
            }
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
