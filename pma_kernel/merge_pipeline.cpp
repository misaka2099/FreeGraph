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
static VEC_INFO vi_read(ap_uint<512>* hbm, int vi_word_off, int pma_idx)
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
    int start_word, int end_word, int total_edges,
    int edge_offset,             // starting edge index in hbm_buffer
    int group_left,              // absolute segment index of group start
    int vc_hbm_base,             // valid_count area HBM base (word offset)
    int fe_hbm_base,             // first_edge area HBM base (word offset)
    ap_uint<512> onchip_buffer[MAX_ONCHIP_SEG_WORDS],
    ap_uint<512> edge_buf[MAX_ONCHIP_SEG_WORDS]
)
{
    int total_words = end_word - start_word;
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
        int pos = 0;
        for (int s = 0; s < chunk_segs; s++) {
            int fill = base + ((chunk_start + s) < extra ? 1 : 0);
            for (int k = 0; k < fill; k++) {
                #pragma HLS PIPELINE II=1
                int global_pos = edge_offset + edge_start + pos;
                int src_word = (global_pos >> 4) - hbm_edge_wo;
                int src_slot = global_pos & 0xF;
                int edge = (int)edge_buf[src_word].range(src_slot * 32 + 31, src_slot * 32);
                int dst_wi = s * SEG_WORDS_PER_SEG + (k >> 4);
                int dst_si = k & 0xF;
                onchip_buffer[dst_wi].range(dst_si * 32 + 31, dst_si * 32) = (ap_uint<32>)edge;
                pos++;
            }
            onchip_buffer[s * SEG_WORDS_PER_SEG + 3].range(511, 480) = (ap_uint<32>)fill;
        }

        // Phase 4: burst-write onchip_buffer → hbm
        int hbm_seg_wo = start_word + chunk_start * SEG_WORDS_PER_SEG;
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
// 每 run 处理完立即 ack (与 task_reader 每 run +1 记账配对)。STOP 时与 redistribution 握手退出。
static void merge_phase(
    ap_uint<512>* hbm_pma,
    ap_uint<512>* hbm_buffer,
    int data_off, int vi_wo,
    hls::stream<PMA_UPDATE_MESSAGE>& edge_in,
    hls::stream<rebalance_task_t>& task_out,
    hls::stream<ap_uint<32>>& redist_ack_in,
    hls::stream<ap_uint<32>>& ack_out)
{
    // 片上缓冲 (URAM)
    ap_uint<512> onchip_buffer[MAX_ONCHIP_SEG_WORDS];
    #pragma HLS bind_storage variable=onchip_buffer type=RAM_2P impl=URAM
    ap_uint<512> edge_buffer[MAX_ONCHIP_SEG_WORDS];
    #pragma HLS bind_storage variable=edge_buffer type=RAM_2P impl=URAM
    // HLS 流读写兜底 (用户要求): if(false) 永不执行, 但保证每个流端口有引用
    if (false) {
        edge_in.read();
        ack_out.write((ap_uint<32>)0);
        redist_ack_in.read();
        rebalance_task_t dummy;
        dummy.pma = 0; dummy.payload = 0; dummy.start_byte = 0; dummy.range_word = 0;
        task_out.write(dummy);
    }

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
        int start_byte = (int)vi.range(31, 0);
        int byte_off = start_byte - data_off;
        int hbase = byte_off / 64;
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
        int fe_hbm_base = hbase + HEADER_WORDS;
        int vc_hbm_base = fe_hbm_base + fe_words;
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
            int seg_start_word = (byte_off + HEADER_BYTES + total_hlp * 64 + vs * SEG_BYTES) / 64;
            int seg_end_word   = (byte_off + HEADER_BYTES + total_hlp * 64 + ve * SEG_BYTES) / 64;
            int seg_words = segs * SEG_WORDS_PER_SEG;

            // ---- 读该 vertex 的 segments 到 onchip_buffer ----
            for (int w = 0; w < seg_words; w++) {
                #pragma HLS PIPELINE II=1
                onchip_buffer[w] = hbm_pma[seg_start_word + w];
            }

            // ---- 流式归并: existing 从 onchip_buffer 逐个读, new 从 new_edges, 结果写 hbm_buffer ----
            // 只缓冲 new_edges[≤8192], 不整段缓冲 existing → 无 unbounded 片上数组。
            // hbm_buffer 是整条 HBM bank, 归并结果直接落这里, redistribute 再写回原地。
            int total = 0;
            int seg_idx = -1;      // 当前 segment (从 -1 开始, 首次自增到 0)
            int seg_rem = 0;       // 当前 segment 剩余有效边
            int slot = 0;          // 当前 slot
            int e_cur = 0;         // 当前读出的 existing 边
            bool have_e = false;   // e_cur 是否有效
            int n = 0;             // new_edges 游标
            int cur_w = -1;        // 当前写入的 hbm_buffer word

            while (true) {
                // 需要时加载下一个 existing 边
                if (!have_e) {
                    if (seg_rem > 0) {
                        int wi = seg_idx * SEG_WORDS_PER_SEG + (slot >> 4);
                        int si = slot & 0xF;
                        e_cur = (int)onchip_buffer[wi].range(si * 32 + 31, si * 32);
                        slot++; seg_rem--;
                        have_e = true;
                    } else {
                        seg_idx++;
                        while (seg_idx < segs) {
                            int vc = (int)onchip_buffer[seg_idx * SEG_WORDS_PER_SEG + 3].range(511, 480);
                            if (vc > EDGE_SLOTS_PER_SEG) vc = EDGE_SLOTS_PER_SEG;
                            if (vc > 0) {
                                seg_rem = vc; slot = 0;
                                e_cur = (int)onchip_buffer[seg_idx * SEG_WORDS_PER_SEG].range(31, 0);
                                slot = 1; seg_rem--;
                                have_e = true;
                                break;
                            }
                            seg_idx++;
                        }
                    }
                }
                // 两个源都耗尽 → 结束
                if (!have_e && n >= nidx) break;

                // 选较小的
                int pick;
                if (!have_e) { pick = new_edges[n++]; }
                else if (n >= nidx) { pick = e_cur; have_e = false; }
                else if (e_cur <= new_edges[n]) { pick = e_cur; have_e = false; }
                else { pick = new_edges[n++]; }

                // 写 hbm_buffer (16/word 打包; word 首次写时清零)
                int w = total >> 4, ss = total & 0xF;
                if (w != cur_w) { cur_w = w; hbm_buffer[w] = 0; }
                hbm_buffer[w].range(ss * 32 + 31, ss * 32) = (ap_uint<32>)pick;
                total++;
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

    // ---- STOP: 与 redistribution 握手退出 ----
    rebalance_task_t stop_task;
    stop_task.pma = 0;
    stop_task.payload = -1;     // STOP
    stop_task.start_byte = 0;
    stop_task.range_word = 0;
    task_out.write(stop_task);

    // 等 redistribution 的 redist_ack(-1)
    while (redist_ack_in.empty()) ap_wait();
    ap_uint<32> ra = redist_ack_in.read();
    (void)ra;

    ack_out.write((ap_uint<32>)-1);
}

// ---- Top-level ----

extern "C"
{
    void merge_pipeline(
        ap_uint<512>* hbm_pma,
        ap_uint<512>* hbm_buffer,
        int pma_raw_data_start,
        int pma_vecinfo_start,
        hls::stream<PMA_UPDATE_MESSAGE>& edge_in,
        hls::stream<rebalance_task_t>& task_out,
        hls::stream<ap_uint<32>>& redist_ack_in,
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
        #pragma HLS INTERFACE axis port=task_out
        #pragma HLS INTERFACE axis port=redist_ack_in
        #pragma HLS INTERFACE axis port=ack_out

        merge_phase(hbm_pma, hbm_buffer, pma_raw_data_start, pma_vecinfo_start / 64,
                    edge_in, task_out, redist_ack_in, ack_out);
    }
}
