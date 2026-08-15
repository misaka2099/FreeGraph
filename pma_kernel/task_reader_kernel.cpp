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
// 大小 65536: 支持 PMA 索引 < 65536 (顶点数 < 2^20 = 1048576)。
#define PMA_LOCK_SIZE 65536
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

// ---- 锁表管理进程 (独立 DATAFLOW 阶段) ----
// 职责: ① 轮询 3 个 ack 流 (pre_ack_in / ack_in_bs / ack_in_merge), 解锁锁表,
//       并把解锁事件回传给主进程 (unlock_out); ② 接受主进程的锁请求 (lock_req_in)。
// 与主进程解耦是本设计的死锁修复核心: 主进程阻塞在 edge/pre_expand 写时, 本进程
// 依然消费 ack — 下游 (insert_phase / redistribution) 不会被 ack 反压卡死,
// 主进程的阻塞写因此是安全的, 无需任何 write_nb。
// 协议:
//   lock_req_in: pma_idx (≥0) = 加锁请求; -1 = 主进程收尾 (所有锁已清) → 退出
//   unlock_out:  已解锁的 pma_idx (与 ack 一一对应)
//   ack == -1 (STOP 收尾标记) 直接丢弃, 无对应锁 (否则 unlock_pma 越界写)。
// 深度不变量: unlock_out 深度 (256) 必须 ≥ 主进程阻塞期间可能到达的在飞 ack 数上界
//   ≈ edge FIFO (2 批) + bs_out (8) + 3 个 ack FIFO (16×3) + task_in (64) ≈ 122。
//   若增大上述任一 FIFO 深度, 需同步放大 unlock_out。
static void lock_manager(
    hls::stream<ap_uint<32>>& pre_ack_in,
    hls::stream<ap_uint<32>>& ack_in_bs,
    hls::stream<ap_uint<32>>& ack_in_merge,
    hls::stream<ap_uint<32>>& lock_req_in,
    hls::stream<ap_uint<32>>& unlock_out,
    int worker_id)
{
    uint8_t pma_lock[PMA_LOCK_SIZE];
    #pragma HLS bind_storage variable=pma_lock type=RAM_2P impl=BRAM

    while (true) {
        bool busy = false;
        ap_uint<32> v;
        // 1. 轮询 ack 流 (非阻塞): 解锁 + 事件回传。3 个流用独立变量 (WAR 并行)。
        if (pre_ack_in.read_nb(v)) {
            if ((int)v != -1) { unlock_pma(pma_lock, (int)v, worker_id); unlock_out.write(v); }
            busy = true;
        }
        if (ack_in_bs.read_nb(v)) {
            if ((int)v != -1) { unlock_pma(pma_lock, (int)v, worker_id); unlock_out.write(v); }
            busy = true;
        }
        if (ack_in_merge.read_nb(v)) {
            if ((int)v != -1) { unlock_pma(pma_lock, (int)v, worker_id); unlock_out.write(v); }
            busy = true;
        }
        // 2. 接受主进程锁请求 (阻塞写的一方是主进程; 本进程从不卡死 —
        //    唯一可能的卡点 unlock_out 满受深度不变量保护)。
        if (!lock_req_in.empty()) {
            v = lock_req_in.read();
            if ((int)v == -1) break;          // 主进程收尾: 退出
            lock_pma(pma_lock, (int)v, worker_id);
            busy = true;
        }
        if (!busy) ap_wait();
    }
}

// ---- 主进程侧: 解锁事件轮询 + 锁提交 ----
// 主进程持有 shadow 锁表 (与管理器的锁表并行维护, 语义一致):
//   提交锁请求时置位, 收到解锁事件时清位。shadow 只被主进程读写, 无 RMW 竞争。
// 解锁错峰: 每调用至多清 1 个 shadow 位 (1 次 RMW) — 逐边扫描循环 II 不被拖垮
// (若循环被流水, 每迭代 ≤1 次 BRAM RMW; 解锁延迟 ≤ 事件数迭代, dispatch 轮询无感)。

// 收解锁事件: 管理器回传的"某 PMA 已解锁" (与 ack 一一对应)
static inline void poll_unlocks(
    hls::stream<ap_uint<32>>& unlock_in,
    uint8_t* pma_lock_shadow,
    int& inflight_pma_count,
    int worker_id)
{
    #pragma HLS INLINE
    if (!unlock_in.empty()) {
        ap_uint<32> v = unlock_in.read();
        unlock_pma(pma_lock_shadow, (int)v, worker_id);
        inflight_pma_count--;
    }
}

// 提交锁请求给管理器 + 置位本地 shadow。
// 用 write_nb + 自旋轮询 unlock, 打破 lock_req↔unlock 双阻塞死锁环:
// 当 lock_req FIFO 满 (管理器忙) 时, 主进程在自旋中持续消费 unlock →
// unlock 排空 → 管理器解除阻塞 → 消费 lock_req → 本写成功。
// (lock_req/unlock 都是内部 stream=ap_fifo, write_nb 合法; 顶部 axis 端口才禁)
static inline void submit_lock(
    hls::stream<ap_uint<32>>& lock_req_out,
    hls::stream<ap_uint<32>>& unlock_in,
    uint8_t* pma_lock_shadow,
    int& inflight_pma_count,
    int pma_idx,
    int worker_id)
{
    #pragma HLS INLINE
    while (!lock_req_out.write_nb((ap_uint<32>)pma_idx)) {
        poll_unlocks(unlock_in, pma_lock_shadow, inflight_pma_count, worker_id);
    }
    lock_pma(pma_lock_shadow, pma_idx, worker_id);
    inflight_pma_count++;
}

// ---- Gate 决策辅助: 读 PMA 的 VEC_INFO + header + VC (gate m_axi) ----
// 供 user-define 决策扩展用。当前 3 个测试策略 (BS_ONLY/MERGE_ONLY/COUNT_GT16)
// 只用到 pma_wave_edges, 不调用本函数 (HLS 会 DCE); 若要按顶点现边数精细决策,
// 在 decide_merge_path 之前调用并把结果传入即可。hbm_gate 映射到本 channel 的
// PMA bank, 读的是 pre-expand 之前的布局 (启发式可接受, 只影响性能)。
static void gate_read_pma_vertex_counts(
    ap_uint<512>* hbm_gate,
    int data_off, int vi_wo,
    int pma_idx,
    int vertex_counts[VERTEX_PER_PMA])
{
    int wi = pma_idx / 8, si = pma_idx % 8;
    ap_uint<512> w = hbm_gate[vi_wo + wi];
    VEC_INFO vi;
    vi.range(63, 0) = w.range(si * 64 + 63, si * 64);
    int start_byte = (int)vi.range(31, 0);
    int byte_off = start_byte - data_off;
    int hbase = byte_off / 64;
    ap_uint<512> range_word = hbm_gate[hbase];
    int total_segs = (int)range_word.range(15 * 32 + 31, 15 * 32);
    int fe_words = fe_words_for(total_segs);
    int vc_hbm_base = hbase + HEADER_WORDS + fe_words;
    for (int v = 0; v < VERTEX_PER_PMA; v++) {
        int vs = (v == 0) ? 0 : (int)range_word.range((v - 1) * 32 + 31, (v - 1) * 32);
        int ve = (int)range_word.range(v * 32 + 31, v * 32);
        int sum = 0;
        for (int s = vs; s < ve; s++) {
            int ww = vc_hbm_base + s / VC_SOA_ENTRIES_PER_WORD;
            int ss = s % VC_SOA_ENTRIES_PER_WORD;
            ap_uint<512> vw = hbm_gate[ww];
            int vcv = (int)vw.range(ss * 32 + 31, ss * 32);
            if (vcv > EDGE_SLOTS_PER_SEG) vcv = EDGE_SLOTS_PER_SEG;
            sum += vcv;
        }
        vertex_counts[v] = sum;
    }
}

// ---- Task Reader 主进程 (gate 路由内核) ----
// 读 online_in 流 (generator → router_stream 按 channel 分发), 收满 wave 8192 /
// FLUSH / STOP 即阻塞并处理当前波。处理流程:
//   sort → profile 计数 (每 PMA 每 vertex) → 发 pre_expand_cmd 并上锁
//        → 按 PMA 决策 (decide_merge_path) 确定发送粒度:
//           merge = 整 PMA 一次 issue 到 edge_out_merge (last=1 在 PMA 末边)
//           bs    = 16 边一批 issue 到 edge_out_bs (last=1 在每批末边), 一批后跳下
//                  一个 PMA (separation cursors), 保持其他 PMA 锁定时仍能发 cmd。
// 跨内核顺序保证: 一个 PMA 的边批只在 redistribution 完成预扩 (pre_ack) 后才发出,
// 所以下游内核收到的边所在 PMA 一定已扩好。
// 锁机制: 本进程不直接读 ack 流 — lock_manager 进程独立轮询 3 个 ack 流,
// 本进程通过 lock_req_out / unlock_in 与它交互 (见 lock_manager 注释)。
static void task_reader_main(
    int worker_id,
    ap_uint<512>* hbm_gate,
    int pma_raw_data_start,
    int pma_vecinfo_start,
    hls::stream<PMA_UPDATE_MESSAGE>& online_in,
    hls::stream<pre_expand_cmd_t>& pre_expand_out,
    hls::stream<PMA_UPDATE_MESSAGE>& edge_out_bs,
    hls::stream<PMA_UPDATE_MESSAGE>& edge_out_merge,
    hls::stream<ap_uint<32>>& lock_req_out,
    hls::stream<ap_uint<32>>& unlock_in
) {

    // ---- Wave buffer: edges of the current wave (before processing) ----
    // Online edges land here; wave closes on wave full (8192) / STOP / FLUSH.
    // The wave is radix-sorted by src (= pma<<4 | vertex) so per-PMA segments
    // become contiguous; per-vertex counts fall out of a linear scan.
    int wave_buffer[WAVE_CAPACITY * 3];
    int radix_tmp[WAVE_CAPACITY * 3];   // sort_src_dst 的 ping-pong 伙伴
    int seperations[WAVE_CAPACITY];
    int seperation_cursors[WAVE_CAPACITY];
    bool merge_path[WAVE_CAPACITY];          // 每 PMA 路由决策: true=merge, false=bs
    #pragma HLS bind_storage variable=radix_tmp type=RAM_2P impl=BRAM
    #pragma HLS bind_storage variable=seperation_cursors type=RAM_2P impl=BRAM
    #pragma HLS bind_storage variable=seperations type=RAM_2P impl=BRAM
    #pragma HLS bind_storage variable=merge_path type=RAM_2P impl=BRAM
    #pragma HLS bind_storage variable=wave_buffer type=RAM_2P impl=BRAM
    // shadow 锁表: 与管理器锁表并行维护 (置位: 提交锁请求; 清位: 收解锁事件)。
    uint8_t pma_lock_shadow[PMA_LOCK_SIZE];
    #pragma HLS bind_storage variable=pma_lock_shadow type=RAM_2P impl=BRAM

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
                poll_unlocks(unlock_in, pma_lock_shadow, inflight_pma_count, worker_id);
                int src = wave_buffer[i * 3 + 0];
                int pma_idx = src >> 4;
                if (pma_idx != current_pma)
                {
                    if (current_pma != -1)
                    {
                        seperations[seperation_count] = i;
                        // 决策: 本次 wave 该 PMA 的新边总数 > 阈值 → merge, 否则 bs
                        int pma_edges = 0;
                        for (int v = 0; v < 16; v++) {
                            #pragma HLS UNROLL
                            pma_edges += current_vc[v];
                        }
                        merge_path[seperation_count] = decide_merge_path(current_pma, pma_edges);
                        seperation_count++;
                        pre_expand_cmd_t pre_cmd;
                        pre_cmd.pma = current_pma;
                        for (int v = 0; v < 16; v++) {
                            #pragma HLS UNROLL
                            pre_cmd.count[v] = current_vc[v];
                        }
                        // 阻塞写安全: 锁表管理进程独立消费 ack, 本进程的写阻塞不再
                        // 引发 ack 反压闭环 (旧架构死锁的根因, 见 lock_manager 注释)。
                        pre_expand_out.write(pre_cmd);
                        submit_lock(lock_req_out, unlock_in, pma_lock_shadow, inflight_pma_count, current_pma, worker_id);
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
                seperations[seperation_count] = wave_count;
                int pma_edges = 0;
                for (int v = 0; v < 16; v++) {
                    #pragma HLS UNROLL
                    pma_edges += current_vc[v];
                }
                merge_path[seperation_count] = decide_merge_path(current_pma, pma_edges);
                seperation_count++;
                pre_expand_cmd_t pre_cmd;
                pre_cmd.pma = current_pma;
                for (int v = 0; v < 16; v++) {
                    #pragma HLS UNROLL
                    pre_cmd.count[v] = current_vc[v];
                }
                pre_expand_out.write(pre_cmd);
                submit_lock(lock_req_out, unlock_in, pma_lock_shadow, inflight_pma_count, current_pma, worker_id);
            }
            // ===== dispatch: 等 pre_ack 解锁后按 PMA 发边批 =====
            // 发送粒度由 gate 决策决定 (profile scan 已算好 merge_path[i]):
            //   merge → 整 PMA 一次发到 edge_out_merge (last=1 在 PMA 末边)
            //   bs    → 16 边一批发到 edge_out_bs (last=1 在每批末边), 一批后跳下一 PMA。
            // 循环逐 PMA 轮询: 某 PMA 被锁 (在飞) 时跳过, 继续发其他 PMA 的批, 保持
            // pipeline 有 cmd 可处理; 锁由管理器解锁事件释放后下一轮继续该 PMA 剩余批。
            int remaining = wave_count;
            seperation_cursors[0] = 0;
            for (int i = 1 ; i < seperation_count; i++)
            {
                seperation_cursors[i] = seperations[i - 1];
            }
            while (remaining > 0)
            {
                poll_unlocks(unlock_in, pma_lock_shadow, inflight_pma_count, worker_id);
                for (int i = 0; i < seperation_count; i++)
                {
                    int pma_idx = wave_buffer[seperation_cursors[i] * 3 + 0] >> 4;
                    if (is_pma_locked(pma_lock_shadow, pma_idx, worker_id)) continue;
                    if (seperation_cursors[i] >= seperations[i]) continue;
                    int start = seperation_cursors[i];
                    int end = seperations[i];
                    int chunk_end = merge_path[i] ? end : (start + 16 < end ? start + 16 : end);
                    bool to_merge = merge_path[i];
                    for (int j = start; j < chunk_end; j++)
                    {
                        int src = wave_buffer[j * 3 + 0];
                        int dst = wave_buffer[j * 3 + 1];
                        int opcode = wave_buffer[j * 3 + 2];
                        PMA_UPDATE_MESSAGE msg;
                        msg.src = src; msg.dst = dst; msg.opcode = (PMA_UPDATE_OPCODE)opcode; msg.last = (j == chunk_end - 1);
                        if (to_merge) edge_out_merge.write(msg);
                        else          edge_out_bs.write(msg);
                        // edge_out_bs.write(msg);
                    }
                    submit_lock(lock_req_out, unlock_in, pma_lock_shadow, inflight_pma_count, pma_idx, worker_id);
                    remaining -= (chunk_end - start);
                    seperation_cursors[i] = chunk_end;
                }
            }
        }
        ap_wait();
        // FLUSH barrier 已过: 复位, 下一轮收波继续收 FLUSH 后的 online
        if (flush) flush = false;

        // 收尾 ACK (最后一波的批可能仍在飞) — 否则 inflight 不降 → 死循环
        poll_unlocks(unlock_in, pma_lock_shadow, inflight_pma_count, worker_id);
        if (online_done && inflight_pma_count == 0) break;
        // 全部处理完但未结束: 空转等 online (下一轮顶部继续收波)
        if (wave_count == 0) ap_wait();
    }
    while (inflight_pma_count > 0) {
        poll_unlocks(unlock_in, pma_lock_shadow, inflight_pma_count, worker_id);
        ap_wait();
    }
    // Send STOP（所有批完成后最后执行）— 两条路径都要收, 各自收尾退出。
    // bs_insert 收到后经 redistribution 收尾 (redistribution 也由它带 STOP);
    // merge_pipeline 收到后直接 ack(-1) 退出 (不再与 redistribution 交互)。
    PMA_UPDATE_MESSAGE stop_msg;
    stop_msg.src = -1; stop_msg.dst = -1;
    stop_msg.opcode = PMA_UPDATE_STOP; stop_msg.last = 1;
    edge_out_bs.write(stop_msg);
    edge_out_merge.write(stop_msg);
    // 通知锁表管理进程退出 (所有真 ack 已消费, 无剩余锁)。
    // 此后的 STOP ack (-1) 无人消费也无妨: 无任何进程等待它。
    // write_nb + ap_wait 自旋: 即使 lock_req 暂时满, 管理器终会消费, 不会死锁。
    while (!lock_req_out.write_nb((ap_uint<32>)-1)) { ap_wait(); }
}

// ---- Top-level ----
// DATAFLOW 双进程: lock_manager (锁表 + ack 轮询) → task_reader_main (gate 路由)。
// 双向通道 (lock_req / unlock) 为 stream 反馈, UG1399 支持 hls::stream 反馈;
// 两侧消费均为非阻塞 (read_nb / empty+read), 无初始死锁。

extern "C"
{
    void task_reader_kernel(
        int worker_id,
        ap_uint<512>* hbm_gate,
        int pma_raw_data_start,
        int pma_vecinfo_start,
        hls::stream<PMA_UPDATE_MESSAGE>& online_in,
        hls::stream<pre_expand_cmd_t>& pre_expand_out,
        hls::stream<PMA_UPDATE_MESSAGE>& edge_out_bs,
        hls::stream<PMA_UPDATE_MESSAGE>& edge_out_merge,
        hls::stream<ap_uint<32>>& pre_ack_in,
        hls::stream<ap_uint<32>>& ack_in_bs,
        hls::stream<ap_uint<32>>& ack_in_merge)
    {
        #pragma HLS INTERFACE m_axi port=hbm_gate offset=slave bundle=gmem0 max_read_burst_length=256
        #pragma HLS INTERFACE s_axilite port=worker_id bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_gate bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_raw_data_start bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_vecinfo_start bundle=control
        #pragma HLS INTERFACE s_axilite port=return bundle=control
        #pragma HLS INTERFACE axis port=online_in
        #pragma HLS INTERFACE axis port=pre_expand_out
        #pragma HLS INTERFACE axis port=edge_out_bs
        #pragma HLS INTERFACE axis port=edge_out_merge
        #pragma HLS INTERFACE axis port=pre_ack_in
        #pragma HLS INTERFACE axis port=ack_in_bs
        #pragma HLS INTERFACE axis port=ack_in_merge

        // 内部通道: lock_req (main → manager, 深度 16 足够: main 提交慢, manager 消费快)
        //           unlock (manager → main, 深度 256: 主进程阻塞期间的 ack 缓冲垫, 见
        //           lock_manager 注释的深度不变量)
        hls::stream<ap_uint<32>> lock_req;
        hls::stream<ap_uint<32>> unlock;
        #pragma HLS STREAM variable=lock_req depth=16
        #pragma HLS STREAM variable=unlock depth=256

        #pragma HLS DATAFLOW

        lock_manager(pre_ack_in, ack_in_bs, ack_in_merge, lock_req, unlock, worker_id);
        task_reader_main(worker_id, hbm_gate, pma_raw_data_start, pma_vecinfo_start / 64,
                         online_in, pre_expand_out, edge_out_bs, edge_out_merge,
                         lock_req, unlock);
    }
}
