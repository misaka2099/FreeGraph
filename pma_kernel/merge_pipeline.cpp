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

// ---- 常量 (与 redistribution_kernel.cpp 一致; 不冒改它, 先复制) ----
static const int MAX_ONCHIP_SEG = 1024;
static const int MAX_ONCHIP_SEG_WORDS = MAX_ONCHIP_SEG * SEG_WORDS_PER_SEG;

// Empty segment ROM: 4 words × 512 bits, replaces make_empty_seg()
static const ap_uint<512> EMPTY_SEG_ROM[4] = {
    ~ap_uint<512>(0),
    ~ap_uint<512>(0),
    ~ap_uint<512>(0),
    (~ap_uint<512>(0)) >> 32     // bits[511:480]=0 (valid_count=0), rest all ones
};

// 单 PMA 整批边数上限 (一次 wave 一个 PMA 最多 8192 条)
static const int MAX_PMA_EDGES = 8192;

// ---- VEC_INFO access ----
static VEC_INFO vi_read(ap_uint<512>* hbm, long long vi_word_off, int pma_idx)
{
    int wi = pma_idx / 8, si = pma_idx % 8;
    ap_uint<512> w = hbm[vi_word_off + wi];
    VEC_INFO vi;
    vi.range(63, 0) = w.range(si * 64 + 63, si * 64);
    return vi;
}

// ---- Redistribute edges evenly across a group of segments (复制自 redistribution_kernel.cpp) ----
// Reads edges from hbm_buffer (flat packed array at edge_offset), redistributes with
// the base+extra pattern (first 'extra' segments get one more edge), and writes
// properly formed segments to hbm. VC (1:1 per segment) and FE (1:1 per segment)
// SoA helper entries are updated inline for each chunk.
void redistribute_edge_from_hbm(
    ap_uint<512>* hbm, ap_uint<512>* hbm_buffer,
    long long start_word, long long end_word, int total_edges,
    int edge_offset,             // starting edge index in hbm_buffer
    int group_left,              // absolute segment index of group start
    long long vc_hbm_base,       // valid_count area HBM base (word offset)
    long long fe_hbm_base,       // first_edge area HBM base (word offset)
    ap_uint<512> onchip_buffer[MAX_ONCHIP_SEG_WORDS],
    ap_uint<512> edge_buf[MAX_ONCHIP_SEG_WORDS]
)
{
    long long total_words = end_word - start_word;
    int total_segs  = total_words / SEG_WORDS_PER_SEG;

    // Distribution: base edges per segment, first 'extra' segments get +1
    int base  = total_edges / total_segs;
    int extra = total_edges % total_segs;

    int max_chunk_segs = MAX_ONCHIP_SEG_WORDS / SEG_WORDS_PER_SEG;  // 1024

    for (int chunk_start = 0; chunk_start < total_segs; chunk_start += max_chunk_segs) {
        int chunk_segs = total_segs - chunk_start;
        if (chunk_segs > max_chunk_segs) chunk_segs = max_chunk_segs;
        int chunk_words = chunk_segs * SEG_WORDS_PER_SEG;

        int edge_start = chunk_start * base + (chunk_start < extra ? chunk_start : extra);
        int edge_end   = (chunk_start + chunk_segs) * base + ((chunk_start + chunk_segs) < extra ? (chunk_start + chunk_segs) : extra);
        int chunk_edges = edge_end - edge_start;

        // Phase 1: init onchip_buffer to EMPTY_SEG for this chunk
        for (int i = 0; i < chunk_words; i += SEG_WORDS_PER_SEG) {
            #pragma HLS PIPELINE II=1
            onchip_buffer[i + 0] = EMPTY_SEG_ROM[0];
            onchip_buffer[i + 1] = EMPTY_SEG_ROM[1];
            onchip_buffer[i + 2] = EMPTY_SEG_ROM[2];
            onchip_buffer[i + 3] = EMPTY_SEG_ROM[3];
        }

        // Phase 2: burst-read edges from hbm_buffer → edge_buf
        int hbm_edge_wo = (edge_offset + edge_start) >> 4;
        int start_mod   = (edge_offset + edge_start) & 0xF;
        int edge_words  = ((start_mod + chunk_edges + 15) >> 4);
        for (int i = 0; i < edge_words; i++) {
            #pragma HLS PIPELINE II=1
            edge_buf[i] = hbm_buffer[hbm_edge_wo + i];
        }

        // Phase 3: pack edges into onchip_buffer with base+extra distribution
        // 原实现每迭代 edge_buf 读 + onchip_buffer 部分写 (RMW) → 两个 URAM 争读口, II=2。
        // 本版拆两段: 内层 k 循环只读 edge_buf 写寄存器 (无内存写竞争, II=1);
        // 段写循环整字写 onchip_buffer (无 RMW, II=1)。空槽保持全 1 = UNUSED_ICON。
        // 每段最多 EDGE_SLOTS_PER_SEG(63) 条边 → 占满 4 字 (字 3 槽 15 = valid_count)。
        int pos = 0;
        for (int s = 0; s < chunk_segs; s++) {
            int fill = base + ((chunk_start + s) < extra ? 1 : 0);
            ap_uint<512> packed[4];
            #pragma HLS ARRAY_PARTITION variable=packed complete
            packed[0] = ~ap_uint<512>(0);
            packed[1] = ~ap_uint<512>(0);
            packed[2] = ~ap_uint<512>(0);
            packed[3] = ~ap_uint<512>(0);
            for (int k = 0; k < fill; k++) {
                #pragma HLS PIPELINE II=1
                int global_pos = edge_offset + edge_start + pos;
                int src_word = (global_pos >> 4) - hbm_edge_wo;
                int src_slot = global_pos & 0xF;
                int edge = (int)edge_buf[src_word].range(src_slot * 32 + 31, src_slot * 32);
                packed[k >> 4].range((k & 0xF) * 32 + 31, (k & 0xF) * 32) = (ap_uint<32>)edge;
                pos++;
            }
            // valid_count 槽 = 字 3 槽 15 (fill ≤ 63 → 边最多占槽 0..62, 不冲突)
            packed[3].range(511, 480) = (ap_uint<32>)fill;
            for (int w = 0; w < 4; w++) {
                #pragma HLS PIPELINE II=1
                onchip_buffer[s * SEG_WORDS_PER_SEG + w] = packed[w];
            }
        }

        // Phase 4: burst-write onchip_buffer → hbm
        long long hbm_seg_wo = start_word + (long long)chunk_start * SEG_WORDS_PER_SEG;
        for (int i = 0; i < chunk_words; i++) {
            #pragma HLS PIPELINE II=1
            hbm[hbm_seg_wo + i] = onchip_buffer[i];
        }

        // Phase 5: update VC (HBM RMW) and FE (HBM RMW)
        int seg_chunk_start = group_left + chunk_start;

        // 5a: VC
        {
            int vc_word_start = seg_chunk_start / VC_SOA_ENTRIES_PER_WORD;
            int vc_word_end   = (seg_chunk_start + chunk_segs + VC_SOA_ENTRIES_PER_WORD - 1)
                                / VC_SOA_ENTRIES_PER_WORD;
            int vc_word_count = vc_word_end - vc_word_start;
            for (int w = 0; w < vc_word_count; w++) {
                #pragma HLS PIPELINE II=1
                edge_buf[w] = hbm[vc_hbm_base + vc_word_start + w];
            }
            for (int s = 0; s < chunk_segs; s++) {
                #pragma HLS PIPELINE II=1
                int global_seg = seg_chunk_start + s;
                int fill = base + ((chunk_start + s) < extra ? 1 : 0);
                int local_word = (global_seg / VC_SOA_ENTRIES_PER_WORD) - vc_word_start;
                int local_slot = global_seg % VC_SOA_ENTRIES_PER_WORD;
                edge_buf[local_word].range(local_slot * 32 + 31, local_slot * 32)
                    = (ap_uint<32>)fill;
            }
            for (int w = 0; w < vc_word_count; w++) {
                #pragma HLS PIPELINE II=1
                hbm[vc_hbm_base + vc_word_start + w] = edge_buf[w];
            }
        }

        // 5b: FE (1:1 — every segment gets its own first_edge; empty → UNUSED_ICON)
        {
            int fe_word_start = seg_chunk_start / FE_SOA_ENTRIES_PER_WORD;
            int fe_word_end   = (seg_chunk_start + chunk_segs + FE_SOA_ENTRIES_PER_WORD - 1)
                                / FE_SOA_ENTRIES_PER_WORD;
            int fe_word_count = fe_word_end - fe_word_start;
            for (int w = 0; w < fe_word_count; w++) {
                #pragma HLS PIPELINE II=1
                edge_buf[w] = hbm[fe_hbm_base + fe_word_start + w];
            }
            for (int s = 0; s < chunk_segs; s++) {
                #pragma HLS PIPELINE II=1
                int global_seg = seg_chunk_start + s;
                int local_word = global_seg / FE_SOA_ENTRIES_PER_WORD - fe_word_start;
                int local_slot = global_seg % FE_SOA_ENTRIES_PER_WORD;
                ap_uint<32> seg_fe = onchip_buffer[s * SEG_WORDS_PER_SEG].range(31, 0);
                edge_buf[local_word].range(local_slot * 32 + 31, local_slot * 32) = seg_fe;
            }
            for (int w = 0; w < fe_word_count; w++) {
                #pragma HLS PIPELINE II=1
                hbm[fe_hbm_base + fe_word_start + w] = edge_buf[w];
            }
        }
    }
}

// ---- Merge Phase (逐 PMA / 逐 vertex 归并) ----
// 读 task_reader 发来的整 PMA 批 (last=1 在 PMA 末边), 对每个被影响的 vertex:
//   读它的 segment 已有边 (有序) + 该 run 的新边 (有序) → 双指针归并 → 按 47/段写回
//   (复用 redistribute_edge_from_hbm 更新 VC/FE) → header.edge_count 累加。
// 每 PMA 处理完立即 ack (与 task_reader 每 PMA +1 记账配对)。
// 整合系统: merge 不再与 redistribution 交互 (pre-expand 保证容量, 无 overflow 任务),
// 也无 task_out/redist_ack_in 流。STOP 收到后直接 ack(-1) 退出; redistribution 的
// STOP 由 bs_insert 路径负责。
static void merge_phase(
    ap_uint<512>* hbm_pma,
    ap_uint<512>* hbm_buffer,
    long long data_off, long long vi_wo,
    hls::stream<PMA_UPDATE_MESSAGE>& edge_in,
    hls::stream<ap_uint<32>>& ack_out)
{
    // 片上缓冲 (URAM)
    ap_uint<512> onchip_buffer[MAX_ONCHIP_SEG_WORDS];
    #pragma HLS bind_storage variable=onchip_buffer type=RAM_2P impl=URAM
    ap_uint<512> edge_buffer[MAX_ONCHIP_SEG_WORDS];
    #pragma HLS bind_storage variable=edge_buffer type=RAM_2P impl=URAM

    while (true) {
        // ---- 读一个 PMA 的整批边 (last=1 在 PMA 末边, task_reader 整 PMA 发送) ----
        PMA_UPDATE_MESSAGE first = edge_in.read();
        if (first.opcode == PMA_UPDATE_STOP && first.last == 1) break;
        int pma = first.src >> 4;
        PMA_UPDATE_MESSAGE edges[MAX_PMA_EDGES];
        edges[0] = first;
        int cnt = 1;
        while (first.last != 1) {
            #pragma HLS PIPELINE II=1
            first = edge_in.read();
            edges[cnt++] = first;
        }

        // ---- 读 VEC_INFO + header ----
        VEC_INFO vi = vi_read(hbm_pma, vi_wo, pma);
        long long start_byte = (long long)vi.range(31, 0);
        long long byte_off = start_byte - data_off;
        long long hbase = byte_off / 64;
        ap_uint<512> range_word = hbm_pma[hbase];
        ap_uint<512> edge_count_word = hbm_pma[hbase + 1];

        int vr[VERTEX_PER_PMA], ec[VERTEX_PER_PMA];
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            vr[v] = (int)range_word.range(v * 32 + 31, v * 32);
            ec[v] = (int)edge_count_word.range(v * 32 + 31, v * 32);
        }
        int total_segs = (int)range_word.range(15 * 32 + 31, 15 * 32);
        int fe_words = fe_words_for(total_segs);
        long long fe_hbm_base = hbase + HEADER_WORDS;
        long long vc_hbm_base = fe_hbm_base + fe_words;
        int total_hlp = fe_words + vc_words_for(total_segs);

        // ---- 按 vertex 统计该 run 的新边数 ----
        int vertex_new_cnt[VERTEX_PER_PMA];
        #pragma HLS ARRAY_PARTITION variable=vertex_new_cnt complete
        for (int v = 0; v < VERTEX_PER_PMA; v++) vertex_new_cnt[v] = 0;
        for (int i = 0; i < cnt; i++) {
            #pragma HLS PIPELINE II=1
            if (edges[i].opcode == PMA_UPDATE_INSERT_EDGE)
                vertex_new_cnt[edges[i].src & 0xF]++;
        }

        bool header_changed = false;
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            int nc = vertex_new_cnt[v];
            if (nc == 0) continue;

            // 提取该 vertex 的新边 (PMA 批内按 src,dst 有序, 所以 dst 有序)
            int new_edges[MAX_PMA_EDGES];
            int nidx = 0;
            for (int i = 0; i < cnt; i++) {
                #pragma HLS PIPELINE II=1
                if (edges[i].opcode == PMA_UPDATE_INSERT_EDGE && (edges[i].src & 0xF) == v)
                    new_edges[nidx++] = edges[i].dst;
            }

            int vs = (v == 0) ? 0 : vr[v - 1];
            int ve = vr[v];
            int segs = ve - vs;
            if (segs <= 0) continue;
            long long seg_start_word = (byte_off + HEADER_BYTES + (long long)total_hlp * 64
                                        + (long long)vs * SEG_BYTES) / 64;
            long long seg_end_word   = (byte_off + HEADER_BYTES + (long long)total_hlp * 64
                                        + (long long)ve * SEG_BYTES) / 64;

            // ---- 流式归并: existing 从 onchip_buffer 逐个读, new 从 new_edges, 结果写 hbm_buffer ----
            // chunk 保护: 顶点 segment 数可 > MAX_ONCHIP_SEG(1024), 按 1024 段一块读入归并。
            // 块内归并只消费 <= 块末 existing 边的 new_edges; 块耗尽即跳到下一块 (new_edges 保留)。
            // 空块跳过 (不消费 new_edges, 避免把新边塞进无容量的空段)。
            // 全部块处理完后的尾部 new_edges 补发 (必 > 所有 existing 边, 保持有序)。
            // 归并结果总边数 total 全局累加, redistribute_edge_from_hbm 按整顶点段范围 base+extra
            // 分块写回 (其内部已按 MAX_ONCHIP_SEG 分块), 容量由 pre-expand 保证。
            int total = 0;
            int n = 0;             // new_edges 游标 (跨块全局)
            // 片上输出: 归并结果攒进 edge_buffer, 攒满一块 (MAX_ONCHIP_SEG_WORDS 字)
            // → 连续 burst 写 hbm_buffer。原实现逐边 hbm_buffer[w].range(...) 部分写触发
            // HLS read-modify-write (每条边 2 笔 HBM 事务, clustered 10000 边 ~20000 笔 —
            // 慢的根因); 本版整字累积 + burst 写, IO 降到每块 1 次连续突发。
            int out_block_base = 0;    // 已写到 hbm_buffer 的字数 (当前块基址)
            int out_word_idx = 0;      // edge_buffer 中已攒的整字数
            ap_uint<512> out_word = 0; // 正在攒的 512-bit 输出字 (16 条边)
            int out_slot = 0;          // 0..15

            // 解包缓存: 块内 existing 边 (每段 ≤ EDGE_SLOTS_PER_SEG 条, flat 16 槽/字)。
            // 归并读 existing_packed + 写 edge_buffer 是两个不同 URAM, 端口不争。
            // 容量: chunk 内 existing ≤ CHUNK_SEGS×63 + new ≤ 8192 必须 ≤ edge_buffer
            // 容量 (4096 字 × 16 = 65536 槽) — 否则归并循环内要 flush (嵌套循环破坏 II)。
            // 896×63 + 8192 = 64640 ≤ 65536 ✓
            const int CHUNK_SEGS = 896;
            ap_uint<512> existing_packed[(CHUNK_SEGS * EDGE_SLOTS_PER_SEG + 15) / 16];
            #pragma HLS bind_storage variable=existing_packed type=RAM_2P impl=URAM
            for (int chunk_start = 0; chunk_start < segs; chunk_start += CHUNK_SEGS) {
                int chunk_segs = segs - chunk_start;
                if (chunk_segs > CHUNK_SEGS) chunk_segs = CHUNK_SEGS;
                int chunk_words = chunk_segs * SEG_WORDS_PER_SEG;

                // 读该块 segments 到 onchip_buffer
                for (int w = 0; w < chunk_words; w++) {
                    #pragma HLS PIPELINE II=1
                    onchip_buffer[w] = hbm_pma[seg_start_word + chunk_start * SEG_WORDS_PER_SEG + w];
                }

                // ---- 解包 existing 边 → existing_packed (段序 flat) ----
                // PMA 段按边值划分, 段内/段间都有序 → 解包后整块边序列有序。
                // 双层循环内层流水: 外层逐段, 内层逐边 (每边 1 拍)。
                int existing_cnt = 0;
                for (int s = 0; s < chunk_segs; s++) {
                    int vc = (int)onchip_buffer[s * SEG_WORDS_PER_SEG + 3].range(511, 480);
                    if (vc > EDGE_SLOTS_PER_SEG) vc = EDGE_SLOTS_PER_SEG;
                    for (int k = 0; k < vc; k++) {
                        #pragma HLS PIPELINE II=1
                        int wi = s * SEG_WORDS_PER_SEG + (k >> 4);
                        int si = k & 0xF;
                        int edge = (int)onchip_buffer[wi].range(si * 32 + 31, si * 32);
                        existing_packed[existing_cnt >> 4].range((existing_cnt & 0xF) * 32 + 31, (existing_cnt & 0xF) * 32) = (ap_uint<32>)edge;
                        existing_cnt++;
                    }
                }

                // ---- 归并: existing (有序) + new_edges (有序) 双指针 → out_word/edge_buffer ----
                // 单层循环 (无嵌套, 无状态机), 每迭代输出 1 条边。
                // 输出容量: 块内 existing ≤ CHUNK_SEGS×63 + new ≤ 8192 → ≤ 64640 <
                // edge_buffer 容量 (4096 字 × 16 = 65536 槽) → 归并循环内不需要 flush,
                // flush 在每块结束后统一做 (循环外, 不破坏流水 II)。
                int e_idx = 0;
                while (e_idx < existing_cnt) {
                    #pragma HLS PIPELINE II=1
                    int e_cur = (int)existing_packed[e_idx >> 4].range((e_idx & 0xF) * 32 + 31, (e_idx & 0xF) * 32);
                    int pick;
                    if (n >= nidx) { pick = e_cur; e_idx++; }
                    else if (new_edges[n] < e_cur) { pick = new_edges[n++]; }
                    else { pick = e_cur; e_idx++; }
                    // 写输出: 攒 16 条 → edge_buffer (写口), 无嵌套 flush
                    out_word.range(out_slot * 32 + 31, out_slot * 32) = (ap_uint<32>)pick;
                    out_slot++;
                    if (out_slot == 16) {
                        edge_buffer[out_word_idx] = out_word;
                        out_word_idx++;
                        out_word = 0;
                        out_slot = 0;
                    }
                    total++;
                }

                // 块结束: flush edge_buffer 完整字 → hbm_buffer (循环外, 保 II=1)
                if (out_word_idx > 0) {
                    for (int bw = 0; bw < out_word_idx; bw++) {
                        #pragma HLS PIPELINE II=1
                        hbm_buffer[out_block_base + bw] = edge_buffer[bw];
                    }
                    out_block_base += out_word_idx;
                    out_word_idx = 0;
                }
            }

            // 归并循环结束: 部分字 (out_word/out_slot) 保留给尾部循环继续填 — 保证归并
            // 输出与尾部输出在同一字上连续; 否则部分字的空槽会作为垃圾边被 redistribute
            // 读回。完整字已在每块结束后 flush 到 hbm_buffer, 这里 out_word_idx 必为 0。

            // 尾部: 所有块处理完后剩余的 new_edges (必 > 所有 existing 边, 直接补在末尾)
            while (n < nidx) {
                #pragma HLS PIPELINE II=1
                out_word.range(out_slot * 32 + 31, out_slot * 32) = (ap_uint<32>)new_edges[n++];
                out_slot++;
                if (out_slot == 16) {
                    edge_buffer[out_word_idx] = out_word;
                    out_word_idx++;
                    out_word = 0;
                    out_slot = 0;
                }
                total++;
            }
            // 尾部 flush: 不足一块的剩余输出 (redistribute 按 edge_words 读回)
            if (out_slot > 0) { edge_buffer[out_word_idx] = out_word; out_word_idx++; }
            if (out_word_idx > 0) {
                for (int bw = 0; bw < out_word_idx; bw++) {
                    #pragma HLS PIPELINE II=1
                    hbm_buffer[out_block_base + bw] = edge_buffer[bw];
                }
            }

            // ---- 写回 segments (base+extra 分布, 更新 VC/FE) ----
            redistribute_edge_from_hbm(
                hbm_pma, hbm_buffer,
                seg_start_word, seg_end_word, total,
                0,             // edge_offset: merged 从 hbm_buffer[0] 开始
                vs,            // group_left: 该 vertex 起始绝对 segment
                vc_hbm_base, fe_hbm_base,
                onchip_buffer, edge_buffer);

            ec[v] += nc;
            header_changed = true;
        }

        // ---- 写回 header.edge_count (range 不变) ----
        if (header_changed) {
            ap_uint<512> new_ec = edge_count_word;
            for (int v = 0; v < VERTEX_PER_PMA; v++) {
                #pragma HLS UNROLL
                new_ec.range(v * 32 + 31, v * 32) = (ap_uint<32>)ec[v];
            }
            hbm_pma[hbase + 1] = new_ec;
        }

        // ---- ack (每 PMA 一次, 与 task_reader 整 PMA 发送 +1 记账配对) ----
        ack_out.write((ap_uint<32>)pma);
    }

    // ---- STOP: 直接 ack(-1) 退出 ----
    // 整合系统里 merge 不参与 redistribution 的 STOP 链 (那由 bs_insert 负责)。
    // ack(-1) 对 task_reader 只是协议收尾 (check_ack_back 跳过 -1)。
    ack_out.write((ap_uint<32>)-1);
}

// ---- Top-level ----

extern "C"
{
    void merge_pipeline(
        ap_uint<512>* hbm_pma,
        ap_uint<512>* hbm_buffer,
        long long pma_raw_data_start,
        long long pma_vecinfo_start,
        hls::stream<PMA_UPDATE_MESSAGE>& edge_in,
        hls::stream<ap_uint<32>>& ack_out)
    {
        #pragma HLS INTERFACE m_axi port=hbm_pma offset=slave bundle=gmem0 max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=hbm_buffer offset=slave bundle=gmem4 max_read_burst_length=256 max_write_burst_length=256

        #pragma HLS INTERFACE s_axilite port=hbm_pma bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_buffer bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_raw_data_start bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_vecinfo_start bundle=control
        #pragma HLS INTERFACE s_axilite port=return bundle=control
        #pragma HLS INTERFACE axis port=edge_in
        #pragma HLS INTERFACE axis port=ack_out

        merge_phase(hbm_pma, hbm_buffer, pma_raw_data_start, pma_vecinfo_start / 64,
                    edge_in, ack_out);
    }
}
