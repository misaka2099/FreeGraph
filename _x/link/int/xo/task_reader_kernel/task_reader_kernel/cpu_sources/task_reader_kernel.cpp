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

#include "radix_sort.h"         // radix_sort_src / sort_src_dst

#define WAVE_CAPACITY 8192      // max edges per wave (barrier closes the wave)

// ---- PMA 锁 (位图): 按 local_pma = pma_idx / CHANNEL_NUM 压缩, 1 bit/PM ----
// task_reader 只处理 pma % CHANNEL_NUM == worker_id 的 PMA (router 已按 channel 分发),
// 所以锁表索引用"本 worker 的第几个 PMA" (local_pma), 8 个 PMA 占 1 字节。
void lock_pma(uint8_t* pma_lock, int pma_idx, int worker_id) {
    int local_pma = pma_idx / CHANNEL_NUM;
    int lock_pma_word = local_pma / 8;
    int lock_pma_bit  = local_pma % 8;
    uint8_t mask = (1 << lock_pma_bit);
    pma_lock[lock_pma_word] |= mask;
}
void unlock_pma(uint8_t* pma_lock, int pma_idx, int worker_id) {
    int local_pma = pma_idx / CHANNEL_NUM;
    int lock_pma_word = local_pma / 8;
    int lock_pma_bit  = local_pma % 8;
    uint8_t mask = ~(1 << lock_pma_bit);
    pma_lock[lock_pma_word] &= mask;
}
static inline bool is_pma_locked(const uint8_t* pma_lock, int pma_idx, int worker_id) {
    int local_pma = pma_idx / CHANNEL_NUM;
    int lock_pma_word = local_pma / 8;
    int lock_pma_bit  = local_pma % 8;
    return ((pma_lock[lock_pma_word] >> lock_pma_bit) & 1) != 0;
}

// 非阻塞收 ACK: pre_ack_in (redistribution 预扩完成) + ack_in (bs_insert 批完成)
// 收到即解锁对应 PMA — 锁 = "该 PMA 有一个在飞操作 (预扩或数据批)"。
// 两个流都必须轮询: 只收 pre_ack 而忽略批 ACK 会导致批完成后锁永不解 → 死锁。
// 防御: ack == -1 (STOP 收尾标记) 跳过 — 否则 unlock_pma 越界写。
void check_ack_back(
    uint8_t* pma_lock,
    hls::stream<ap_uint<32>>& pre_ack_in,
    hls::stream<ap_uint<32>>& ack_in,
    int& inflight_pma_count,
    int worker_id
)
{
    if (!pre_ack_in.empty()) {
        ap_uint<32> ack = pre_ack_in.read();
        if (ack != -1) {
            unlock_pma(pma_lock, (int)ack, worker_id);
            inflight_pma_count--;
        }
    }
    if (!ack_in.empty()) {
        ap_uint<32> ack = ack_in.read();
        if (ack != -1) {
            unlock_pma(pma_lock, (int)ack, worker_id);
            inflight_pma_count--;
        }
    }
}

// ---- Task Reader (独立内核, 纯流, 0 m_axi) ----
// 读 online_in 流 (generator → router_stream 按 channel 分发), 收满 wave 8192 /
// FLUSH / STOP 即阻塞并处理当前波。处理流程:
//   sort → profile 计数 (每 PMA 每 vertex) → 发 pre_expand_cmd 并上锁
//        → dispatch 边批 (≤16/run, last 标记) 等 pre_ack 解锁后放行。
// 跨内核顺序保证: 一个 PMA 的边批只在 redistribution 完成预扩 (pre_ack) 后才发出,
// 所以 bs_insert 收到的边所在 PMA 一定已扩好。
static void task_reader(
    int worker_id,
    hls::stream<PMA_UPDATE_MESSAGE>& online_in,
    hls::stream<pre_expand_cmd_t>& pre_expand_out,
    hls::stream<PMA_UPDATE_MESSAGE>& edge_out,
    hls::stream<ap_uint<32>>& pre_ack_in,
    hls::stream<ap_uint<32>>& ack_in
) {

    // ---- Wave buffer: edges of the current wave (before processing) ----
    // Online edges land here; wave closes on wave full (8192) / STOP / FLUSH.
    // The wave is radix-sorted by src (= pma<<4 | vertex) so per-PMA segments
    // become contiguous; per-vertex counts fall out of a linear scan.
    int wave_buffer[WAVE_CAPACITY * 3];
    int radix_tmp[WAVE_CAPACITY * 3];   // sort_src_dst 的 ping-pong 伙伴
    int seperations[WAVE_CAPACITY];
    int seperation_cursors[WAVE_CAPACITY];
    #pragma HLS bind_storage variable=radix_tmp type=RAM_2P impl=BRAM
    #pragma HLS bind_storage variable=seperation_cursors type=RAM_2P impl=BRAM
    #pragma HLS bind_storage variable=seperations type=RAM_2P impl=BRAM
    #pragma HLS bind_storage variable=wave_buffer type=RAM_2P impl=BRAM
    // PMA 锁: 本轮已处理的 PMA (按 PMA 值直接索引, 1 字节/PM)。
    // 大小 65536: 支持 PMA 索引 < 65536 (顶点数 < 2^20 = 1048576)。
    #define PMA_LOCK_SIZE 65536
    uint8_t pma_lock[PMA_LOCK_SIZE];
    #pragma HLS bind_storage variable=pma_lock type=RAM_2P impl=BRAM

    bool online_done = false;
    bool flush = false;
    int inflight_pma_count = 0;
    while (true)
    {
        // ===== 收集阶段: online 收进 wave_buffer, 收满 8192 / FLUSH / STOP 即退出 =====
        // 收满 8192 直接阻塞 (wave_buffer 只有一份, 处理完当前波才能继续收)。
        int wave_count = 0;
        COLLET_EDGE_LOOP:
        while (!online_done && !flush && wave_count < WAVE_CAPACITY)
        {
            #pragma HLS PIPELINE II=1
            PMA_UPDATE_MESSAGE msg = online_in.read();
            if (msg.opcode == PMA_UPDATE_STOP) {
                online_done = true;
            }
            else if (msg.opcode == PMA_UPDATE_FLUSH_CACHE) {
                flush = true;
            }
            else {
                wave_buffer[wave_count * 3 + 0] = msg.src;
                wave_buffer[wave_count * 3 + 1] = msg.dst;
                wave_buffer[wave_count * 3 + 2] = msg.opcode;
                wave_count++;
            }
        }
        ap_wait();

        if (wave_count > 0)
        {
            // ===== 排序: src, dst 字典序升序 (排序后同 PMA 连续成段, 段内 dst 升序) =====
            sort_src_dst(wave_buffer, radix_tmp, wave_count);
            int current_pma = -1;
            int current_vc[16];
            #pragma HLS ARRAY_PARTITION variable=current_vc complete
            int seperation_count = 0;
            for (int i = 0 ; i < wave_count; i++)
            {
                check_ack_back(pma_lock, pre_ack_in, ack_in, inflight_pma_count, worker_id);
                int src = wave_buffer[i * 3 + 0];
                int pma_idx = src >> 4;
                if (pma_idx != current_pma)
                {
                    if (current_pma != -1)
                    {
                        seperations[seperation_count++] = i;
                        pre_expand_cmd_t pre_cmd;
                        pre_cmd.pma = current_pma;
                        for (int v = 0; v < 16; v++) {
                            #pragma HLS UNROLL
                            pre_cmd.count[v] = current_vc[v];
                        }
                        pre_expand_out.write(pre_cmd);
                        inflight_pma_count++;
                        lock_pma(pma_lock, current_pma, worker_id);
                    }
                    current_pma = pma_idx;
                    for (int v = 0; v < 16; v++) {
                        #pragma HLS UNROLL
                        current_vc[v] = 0;   // ★ 换 PMA: 清零 (否则旧 PMA 计数混入)
                    }
                }
                current_vc[src % 16]++;      // ★ 先判断换 PMA 再累加 (新 PMA 首条边不混入旧计数)
            }
            if (current_pma != -1)
            {
                seperations[seperation_count++] = wave_count;
                pre_expand_cmd_t pre_cmd;
                pre_cmd.pma = current_pma;
                for (int v = 0; v < 16; v++) {
                    #pragma HLS UNROLL
                    pre_cmd.count[v] = current_vc[v];
                }
                pre_expand_out.write(pre_cmd);
                inflight_pma_count++;
                lock_pma(pma_lock, current_pma, worker_id);
            }
            // ===== dispatch: 等 pre_ack 解锁后按 PMA 发边批 (≤16/run, last 标记) =====
            int remaining = wave_count;
            seperation_cursors[0] = 0;
            for (int i = 1 ; i < seperation_count; i++)
            {
                seperation_cursors[i] = seperations[i - 1];
            }
            while (remaining > 0)
            {
                check_ack_back(pma_lock, pre_ack_in, ack_in, inflight_pma_count, worker_id);
                for (int i = 0; i < seperation_count; i++)
                {
                    int pma_idx = wave_buffer[seperation_cursors[i] * 3 + 0] >> 4;
                    if (is_pma_locked(pma_lock, pma_idx, worker_id)) continue;
                    if (seperation_cursors[i] >= seperations[i]) continue;
                    int start = seperation_cursors[i];
                    int end = seperations[i];   // merge 按 PMA 粒度: 整 PMA 一次发完 (last=1 在 PMA 末边)
                    for (int j = start; j < end; j++)
                    {
                        int src = wave_buffer[j * 3 + 0];
                        int dst = wave_buffer[j * 3 + 1];
                        int opcode = wave_buffer[j * 3 + 2];
                        PMA_UPDATE_MESSAGE msg;
                        msg.src = src; msg.dst = dst; msg.opcode = (PMA_UPDATE_OPCODE)opcode; msg.last = (j == end - 1);
                        edge_out.write(msg);
                    }
                    inflight_pma_count++;
                    lock_pma(pma_lock, pma_idx, worker_id);
                    remaining -= (end - start);
                    seperation_cursors[i] = end;
                }
            }
        }
        ap_wait();
        // FLUSH barrier 已过: 复位, 下一轮收波继续收 FLUSH 后的 online
        if (flush) flush = false;

        // 收尾 ACK (最后一波的批可能仍在飞) — 否则 inflight 不降 → 死循环
        check_ack_back(pma_lock, pre_ack_in, ack_in, inflight_pma_count, worker_id);
        if (online_done && inflight_pma_count == 0) break;
        // 全部处理完但未结束: 空转等 online (下一轮顶部继续收波)
        if (wave_count == 0) ap_wait();
    }
    while (inflight_pma_count > 0) {
        check_ack_back(pma_lock, pre_ack_in, ack_in, inflight_pma_count, worker_id);
        ap_wait();
    }
    // Send STOP（所有 insert 完成后最后执行）
    PMA_UPDATE_MESSAGE stop_msg;
    stop_msg.src = -1; stop_msg.dst = -1;
    stop_msg.opcode = PMA_UPDATE_STOP; stop_msg.last = 1;
    edge_out.write(stop_msg);
}

// ---- Top-level ----

extern "C"
{
    void task_reader_kernel(
        int worker_id,
        hls::stream<PMA_UPDATE_MESSAGE>& online_in,
        hls::stream<pre_expand_cmd_t>& pre_expand_out,
        hls::stream<PMA_UPDATE_MESSAGE>& edge_out,
        hls::stream<ap_uint<32>>& pre_ack_in,
        hls::stream<ap_uint<32>>& ack_in)
    {
        #pragma HLS INTERFACE s_axilite port=worker_id bundle=control
        #pragma HLS INTERFACE s_axilite port=return bundle=control
        #pragma HLS INTERFACE axis port=online_in
        #pragma HLS INTERFACE axis port=pre_expand_out
        #pragma HLS INTERFACE axis port=edge_out
        #pragma HLS INTERFACE axis port=pre_ack_in
        #pragma HLS INTERFACE axis port=ack_in

        task_reader(worker_id, online_in, pre_expand_out, edge_out, pre_ack_in, ack_in);
    }
}
