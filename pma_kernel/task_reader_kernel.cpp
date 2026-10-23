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
#ifdef HW_EMU
#undef PMA_LOCK_SIZE
#define PMA_LOCK_SIZE 256
#endif
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

// Internal centralized-lock protocol.
// bit 32: try-acquire (1 = grant only when currently unlocked, 0 = force acquire)
// bit 33: stop manager after all completions have been consumed
typedef ap_uint<34> lock_req_t;


// ---- 锁表管理进程 (独立 DATAFLOW 阶段) ----
// 职责: ① 轮询 3 个 ack 流 (pre_ack_in / ack_in_bs / ack_in_merge), 解锁锁表,
//       并把解锁事件回传给主进程 (unlock_out); ② 接受主进程的锁请求 (lock_req_in)。
// 与主进程解耦是本设计的死锁修复核心: 主进程阻塞在 edge/pre_expand 写时, 本进程
// 依然消费 ack — 下游 (insert_phase / redistribution) 不会被 ack 反压卡死,
// 主进程的阻塞写因此是安全的, 无需任何 write_nb。
// 协议:
//   lock_req_in: pma_idx (≥0) = 加锁请求; -1 = 主进程收尾 (所有锁已清) → 退出
//   unlock_out:  已解锁的 pma_idx (与 ack 一一对应)
//   ack == -1 (STOP 收尾标记) 转为 stop_done 事件, 不访问锁表。
// 深度不变量: unlock_out 深度 (256) 必须 ≥ 主进程阻塞期间可能到达的在飞 ack 数上界
//   ≈ edge FIFO (2 批) + bs_out (8) + 3 个 ack FIFO (16×3) + task_in (64) ≈ 122。
//   若增大上述任一 FIFO 深度, 需同步放大 unlock_out。
static void lock_manager(
    hls::stream<ap_uint<32>>& pre_ack_in,
    hls::stream<ap_uint<32>>& ack_in_bs,
    hls::stream<ap_uint<32>>& ack_in_merge,
    hls::stream<lock_req_t>& lock_req_in,
    hls::stream<ap_uint<1>>& lock_rsp_out,
    hls::stream<ap_uint<1>>& wave_done_out,
    hls::stream<ap_uint<1>>& stop_done_out,
    hls::stream<ap_uint<1>>& manager_done_out,
    // hls::stream<ap_uint<32>>& completion_out,
    int worker_id)
{
    // This is the only authoritative PMA lock state.  ACK handling and
    // check-and-set requests are serialized so each iteration performs at most
    // one packed-byte RMW; this removes the old multi-write carried dependency.
    uint8_t pma_lock[PMA_LOCK_SIZE];
    #pragma HLS bind_storage variable=pma_lock type=RAM_2P impl=BRAM

    // This table is now the authoritative state.  Explicit initialization is
    // mandatory: a try-lock before a defined zero value can reject every PMA
    // forever, because no downstream ACK exists yet to clear the bit.
    LOCK_TABLE_INIT:
    for (int i = 0; i < PMA_LOCK_SIZE; i++) {
        #pragma HLS PIPELINE II=1
        pma_lock[i] = 0;
    }

    unsigned ack_rr = 0;
    int lock_num = 0;
    bool stop_manager = false;
    while (true) {
#pragma HLS PIPELINE II=1
        bool busy = false;
        bool have_table_op = false;
        bool check_busy = false;
        // bool is_completion = false;
        // bool try_acquire = false;
        bool set_lock = false; // 1 for set 0 for reset
        int pma_idx = 0;
        ap_uint<32> v;
        // A lock query must be answered before consuming another ACK.  The
        // previous ACK-first else-if chain could starve lock_req_in whenever a
        // completion stream stayed non-empty; task_reader then hung forever at
        // lock_rsp_in.empty() (the observed 189-loop stall).  ACKs are served
        // with round-robin arbitration whenever no query is pending.
        if (!lock_req_in.empty()) {
            lock_req_t req = lock_req_in.read();
            if (req[33]) {
                stop_manager = true;
            } else if (!stop_manager) {
                pma_idx = (int)req.range(31, 0);
                have_table_op = true;
                set_lock = true;
                if (req[32]) { check_busy = true; }
            }
            busy = true;
        } else if (ack_rr == 0 && pre_ack_in.read_nb(v)) {
            if ((int)v != -1) {
                pma_idx = (int)v;
                have_table_op = true;
                set_lock = false;
            }
            busy = true;
        } else if (ack_rr == 1 && ack_in_bs.read_nb(v)) {
            if ((int)v == CACHE_WAVE_ACK) {
                wave_done_out.write((ap_uint<1>)0);
            } else if ((int)v == -1) {
                stop_done_out.write((ap_uint<1>)0);
            } else if ((int)v != -1) {
                pma_idx = (int)v;
                have_table_op = true;
                set_lock = false;
            }
            busy = true;
        } else if (ack_rr == 2 && ack_in_merge.read_nb(v)) {
            if ((int)v == CACHE_WAVE_ACK) {
                wave_done_out.write((ap_uint<1>)1);
            } else if ((int)v == -1) {
                stop_done_out.write((ap_uint<1>)1);
            } else if ((int)v != -1) {
                pma_idx = (int)v;
                have_table_op = true;
                set_lock = false;
            }
            busy = true;
        }
        ack_rr = (ack_rr + 1) % 3;
        if (stop_manager && lock_num == 0) {
            manager_done_out.write((ap_uint<1>)1);
            break;
        }
        if (have_table_op) {
            int local_pma = pma_idx / CHANNEL_NUM;
            int lock_word = local_pma / 8;
            int lock_bit = local_pma % 8;
            uint8_t mask = (uint8_t)(1 << lock_bit);

            // A single syntactic load/store pair prevents mutually-exclusive
            // ACK/request branches from being counted as concurrent BRAM ports.
            uint8_t old_word = pma_lock[lock_word];
            bool locked = (old_word & mask) != 0;
            if (check_busy) {
                lock_rsp_out.write((ap_uint<1>)locked);
                continue;
            }
            uint8_t new_word = set_lock ?  (uint8_t)(old_word | mask) : (uint8_t)(old_word & ~mask);
            if (set_lock && !locked) {
                pma_lock[lock_word] = new_word;
                lock_num++;
                ap_wait();
            } else if (!set_lock && locked) {
                pma_lock[lock_word] = new_word;
                lock_num--;
                ap_wait();
            }

        }
        if (!busy) ap_wait();
    }
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

// Read the two header words needed by the analytical Update Path Selection
// policy: current valid edge count E and allocated segment count S.
static void gate_read_pma_stats(
    ap_uint<512>* hbm_gate, int data_off, int vi_wo, int pma_idx,
    int &current_edges, int &segment_count)
{
    int wi = pma_idx / 8, si = pma_idx % 8;
    ap_uint<512> w = hbm_gate[vi_wo + wi];
    VEC_INFO vi;
    vi.range(63, 0) = w.range(si * 64 + 63, si * 64);
    if (vi == 0) {
        current_edges = 0;
        segment_count = 0;
        return;
    }
    int hbase = ((int)vi.range(31, 0) - data_off) / 64;
    ap_uint<512> range_word = hbm_gate[hbase];
    ap_uint<512> edge_count_word = hbm_gate[hbase + 1];
    segment_count = (int)range_word.range(15 * 32 + 31, 15 * 32);
    current_edges = 0;
    for (int v = 0; v < VERTEX_PER_PMA; v++) {
        #pragma HLS UNROLL
        current_edges += (int)edge_count_word.range(v * 32 + 31, v * 32);
    }
}

static bool gate_needs_pre_expand(
    ap_uint<512>* hbm_gate, int data_off, int vi_wo, int pma_idx,
    const int new_counts[VERTEX_PER_PMA])
{
    int wi = pma_idx / 8, si = pma_idx % 8;
    ap_uint<512> vw = hbm_gate[vi_wo + wi];
    VEC_INFO vi;
    vi.range(63, 0) = vw.range(si * 64 + 63, si * 64);
    // A zero VEC_INFO denotes an unallocated PMA.  It has no readable header;
    // redistribution must run its INIT path before any update is dispatched.
    if (vi == 0) return true;

    int hbase = ((int)vi.range(31, 0) - data_off) / 64;
    ap_uint<512> range_word = hbm_gate[hbase];
    ap_uint<512> edge_count_word = hbm_gate[hbase + 1];
    for (int v = 0; v < VERTEX_PER_PMA; v++) {
        int vs = v == 0 ? 0 : (int)range_word.range((v - 1) * 32 + 31, (v - 1) * 32);
        int ve = (int)range_word.range(v * 32 + 31, v * 32);
        int current = (int)edge_count_word.range(v * 32 + 31, v * 32);
        if (current + new_counts[v] > (ve - vs) * 47) return true;
    }
    return false;
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
bool is_lock(hls::stream<lock_req_t>& lock_req_out, hls::stream<ap_uint<1>>& lock_rsp_in, int pma_idx) {
    #pragma HLS INLINE
    lock_req_t req;
    req.range(31, 0) = (ap_uint<32>)pma_idx;
    req[33] = 0; // stop_manager
    req[32] = 1; // check_busy
    lock_req_out.write(req);
    ap_wait();
    bool locked = (bool)lock_rsp_in.read();
    return locked;
}
void set_lock(hls::stream<lock_req_t>& lock_req_out, int pma_idx)
{
    #pragma HLS INLINE
    lock_req_t req;
    req.range(31, 0) = (ap_uint<32>)pma_idx;
    req[33] = 0; // stop_manager
    req[32] = 0; // check_busy
    lock_req_out.write(req);
    ap_wait();
}

static inline bs_update_token_t make_bs_token(const PMA_UPDATE_MESSAGE &m)
{
    #pragma HLS INLINE
    bs_update_token_t t;
    t.src = (ap_uint<32>)m.src;
    t.dst = (ap_uint<32>)m.dst;
    t.opcode = (ap_uint<4>)m.opcode;
    t.last = (ap_uint<1>)m.last;
    return t;
}

static inline void send_router_ack(hls::stream<WHOLE_ACK>& ack_to_router, int worker_id)
{
    #pragma HLS INLINE
    WHOLE_ACK ack = {};
    ack.data = worker_id;
    ack.keep = -1;
    ack.strb = -1;
    ack.last = 1;
    ack_to_router.write(ack);
}

// Drain the external AXIS input independently from the wave-processing state
// machine.  The main task-reader loop spends long intervals sorting a wave,
// waiting for downstream acknowledgements, and accessing HBM.  If it owns the
// external stream directly, those intervals deassert online_in_TREADY and can
// leave a FLUSH command stuck at the router output.  This relay keeps the
// external stream flowing and preserves command order in a bounded FIFO.
static void receive_online_commands(
    hls::stream<PMA_UPDATE_MESSAGE>& online_in,
    hls::stream<PMA_UPDATE_MESSAGE>& online_fifo)
{
    bool stop = false;
    while (!stop) {
        #pragma HLS PIPELINE II=1
        PMA_UPDATE_MESSAGE msg = online_in.read();
        online_fifo.write(msg);
        if (msg.opcode == PMA_UPDATE_STOP)
            stop = true;
    }
}

static void task_reader_main(
    int worker_id,
    ap_uint<512>* hbm_gate,
    int pma_raw_data_start,
    int pma_vecinfo_start,
    hls::stream<PMA_UPDATE_MESSAGE>& online_in,
    hls::stream<pre_expand_cmd_t>& pre_expand_out,
    hls::stream<bs_update_token_t>& edge_out_bs,
    hls::stream<PMA_UPDATE_MESSAGE>& edge_out_merge,
    hls::stream<lock_req_t>& lock_req_out,
    hls::stream<ap_uint<1>>& lock_rsp_in,
    hls::stream<ap_uint<1>>& wave_done_in,
    hls::stream<ap_uint<1>>& stop_done_in,
    hls::stream<ap_uint<1>>& manager_done_in,
    hls::stream<WHOLE_ACK>& ack_to_router
) {

    // ---- Wave buffer: edges of the current wave (before processing) ----
    // Online edges land here; wave closes on wave full (8192) / STOP / FLUSH.
    // The wave is radix-sorted by src (= pma<<4 | vertex) so per-PMA segments
    // become contiguous; per-vertex counts fall out of a linear scan.
    wave_edge_t wave_buffer[WAVE_CAPACITY];
    wave_edge_t radix_tmp[WAVE_CAPACITY];  // sort_src_dst 的 ping-pong 伙伴
    ap_uint<14> seperations[WAVE_CAPACITY];
    ap_uint<14> seperation_cursors[WAVE_CAPACITY];
    bool pre_expand_checked[WAVE_CAPACITY];
    ap_uint<512> separation_counts[WAVE_CAPACITY];
    bool merge_path[WAVE_CAPACITY];          // 每 PMA 路由决策: true=merge, false=bs
    #pragma HLS bind_storage variable=radix_tmp type=RAM_2P impl=URAM
    #pragma HLS bind_storage variable=seperation_cursors type=RAM_2P impl=BRAM
    #pragma HLS bind_storage variable=seperations type=RAM_2P impl=BRAM
    #pragma HLS bind_storage variable=merge_path type=RAM_2P impl=BRAM
    #pragma HLS bind_storage variable=pre_expand_checked type=RAM_2P impl=BRAM
    #pragma HLS bind_storage variable=separation_counts type=RAM_2P impl=URAM
    #pragma HLS bind_storage variable=wave_buffer type=RAM_2P impl=URAM
    bool online_done = false;
    bool flush = false;
    // int inflight_pma_count = 0;
    while (true)
    {
        // ===== 收集阶段: online 收进 wave_buffer, 收满 8192 / FLUSH / STOP 即退出 =====
        // 收满 8192 直接阻塞 (wave_buffer 只有一份, 处理完当前波才能继续收)。
        int wave_count = 0;
        COLLECT_EDGE_LOOP:
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
                wave_buffer[wave_count].src = msg.src;
                wave_buffer[wave_count].dst = msg.dst;
                wave_buffer[wave_count].opcode = msg.opcode;
                wave_count++;
            }
        }
        ap_wait();

        if (wave_count > 0)
        {
            // ===== 排序: src, dst 字典序升序 (排序后同 PMA 连续成段, 段内 dst 升序) =====
            sort_src_dst_packed(wave_buffer, radix_tmp, wave_count);
            int current_pma = -1;
            int current_vc[16];
            #pragma HLS ARRAY_PARTITION variable=current_vc complete
            int seperation_count = 0;
            for (int i = 0 ; i < wave_count; i++)
            {
                int src = wave_buffer[i].src;
                int pma_idx = src >> 4;
                if (pma_idx != current_pma)
                {
                    if (current_pma != -1)
                    {
                        seperations[seperation_count] = i;
                        // Analytical Update Path Selection (design.tex), not a
                        // fixed update-count threshold.
                        int pma_edges = 0;
                        for (int v = 0; v < 16; v++) {
                            #pragma HLS UNROLL
                            pma_edges += current_vc[v];
                        }
                        int current_edges = 0, segment_count = 0;
                        gate_read_pma_stats(hbm_gate, pma_raw_data_start,
                                            pma_vecinfo_start, current_pma,
                                            current_edges, segment_count);
                        merge_path[seperation_count] = decide_merge_path(
                            current_pma, pma_edges, current_edges, segment_count);
                        seperation_count++;
                        // 阻塞写安全: 锁表管理进程独立消费 ack, 本进程的写阻塞不再
                        // 引发 ack 反压闭环 (旧架构死锁的根因, 见 lock_manager 注释)。
                        ap_uint<512> count_word = 0;
                        for (int v = 0; v < VERTEX_PER_PMA; v++) {
                            #pragma HLS UNROLL
                            count_word.range(v * 32 + 31, v * 32) = current_vc[v];
                        }
                        separation_counts[seperation_count - 1] = count_word;
                        pre_expand_checked[seperation_count - 1] = false;
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
                int current_edges = 0, segment_count = 0;
                gate_read_pma_stats(hbm_gate, pma_raw_data_start,
                                    pma_vecinfo_start, current_pma,
                                    current_edges, segment_count);
                merge_path[seperation_count] = decide_merge_path(
                    current_pma, pma_edges, current_edges, segment_count);
                seperation_count++;
                ap_uint<512> count_word = 0;
                for (int v = 0; v < VERTEX_PER_PMA; v++) {
                    #pragma HLS UNROLL
                    count_word.range(v * 32 + 31, v * 32) = current_vc[v];
                }
                separation_counts[seperation_count - 1] = count_word;
                pre_expand_checked[seperation_count - 1] = false;
            }
            // ===== dispatch: 等 pre_ack 解锁后按 PMA 发边批 =====
            // 发送粒度由 gate 决策决定 (profile scan 已算好 merge_path[i]):
            //   merge → 整 PMA 一次发到 edge_out_merge (last=1 在 PMA 末边)
            //   bs    → 16 边一批发到 edge_out_bs (last=1 在每批末边), 一批后跳下一 PMA。
            // 循环逐 PMA 轮询: 某 PMA 被锁 (在飞) 时跳过, 继续发其他 PMA 的批, 保持
            // pipeline 有 cmd 可处理; 锁由管理器解锁事件释放后下一轮继续该 PMA 剩余批。
            int remaining = wave_count;

            // Select deterministic per-route cache-admission candidates.
            pma_idx_t bs_top_pma[BS_CACHE_HINTS_PER_WAVE];
            ap_uint<14> bs_top_value[BS_CACHE_HINTS_PER_WAVE];
            pma_idx_t merge_top_pma[MERGE_CACHE_HINTS_PER_WAVE];
            ap_uint<14> merge_top_value[MERGE_CACHE_HINTS_PER_WAVE];
            #pragma HLS ARRAY_PARTITION variable=bs_top_pma complete
            #pragma HLS ARRAY_PARTITION variable=bs_top_value complete
            #pragma HLS ARRAY_PARTITION variable=merge_top_pma complete
            #pragma HLS ARRAY_PARTITION variable=merge_top_value complete
            for (int k = 0; k < BS_CACHE_HINTS_PER_WAVE; k++) {
                #pragma HLS UNROLL
                bs_top_pma[k] = -1;
                bs_top_value[k] = -1;
            }
            for (int k = 0; k < MERGE_CACHE_HINTS_PER_WAVE; k++) {
                #pragma HLS UNROLL
                merge_top_pma[k] = -1;
                merge_top_value[k] = -1;
            }
            for (int i = 0; i < seperation_count; i++) {
                ap_uint<14> begin = i == 0 ? ap_uint<14>(0) : seperations[i - 1];
                int value = seperations[i] - begin;
                pma_idx_u_t pma = wave_buffer[begin].src >> 4;
                if (merge_path[i]) {
                    for (int k = 0; k < MERGE_CACHE_HINTS_PER_WAVE; k++) {
                        if (value > merge_top_value[k] ||
                            (value == merge_top_value[k] &&
                             (merge_top_pma[k] < 0 || pma < merge_top_pma[k]))) {
                            for (int s = MERGE_CACHE_HINTS_PER_WAVE - 1; s > k; s--) {
                                merge_top_value[s] = merge_top_value[s - 1];
                                merge_top_pma[s] = merge_top_pma[s - 1];
                            }
                            merge_top_value[k] = value;
                            merge_top_pma[k] = pma;
                            break;
                        }
                    }
                } else {
                    for (int k = 0; k < BS_CACHE_HINTS_PER_WAVE; k++) {
                        if (value > bs_top_value[k] ||
                            (value == bs_top_value[k] &&
                             (bs_top_pma[k] < 0 || pma < bs_top_pma[k]))) {
                            for (int s = BS_CACHE_HINTS_PER_WAVE - 1; s > k; s--) {
                                bs_top_value[s] = bs_top_value[s - 1];
                                bs_top_pma[s] = bs_top_pma[s - 1];
                            }
                            bs_top_value[k] = value;
                            bs_top_pma[k] = pma;
                            break;
                        }
                    }
                }
            }
            PMA_UPDATE_MESSAGE cache_cmd;
            cache_cmd.last = 1;
            cache_cmd.opcode = PMA_UPDATE_CACHE_HINT;
            for (int k = 0; k < BS_CACHE_HINTS_PER_WAVE; k++) {
                if (bs_top_pma[k] < 0) continue;
                cache_cmd.src = bs_top_pma[k];
                cache_cmd.dst = bs_top_value[k];
                edge_out_bs.write(make_bs_token(cache_cmd));
            }
            for (int k = 0; k < MERGE_CACHE_HINTS_PER_WAVE; k++) {
                if (merge_top_pma[k] < 0) continue;
                cache_cmd.src = merge_top_pma[k];
                cache_cmd.dst = merge_top_value[k];
                edge_out_merge.write(cache_cmd);
            }

            seperation_cursors[0] = 0;
            for (int i = 1 ; i < seperation_count; i++)
            {
                seperation_cursors[i] = seperations[i - 1];
            }
            while (remaining > 0)
            {
                // poll_completions(completion_in, inflight_pma_count);
                for (int i = 0; i < seperation_count; i++)
                {
                    if (seperation_cursors[i] >= seperations[i]) continue;
                    pma_idx_u_t pma_idx = wave_buffer[seperation_cursors[i]].src >> 4;
                    if (!pre_expand_checked[i]) {
                        // The same round-robin loop performs the gate check.
                        // A busy PMA is skipped; other separations continue.
                        if (is_lock(lock_req_out, lock_rsp_in, pma_idx)) continue;
                        pre_expand_cmd_t pre_cmd;
                        pre_cmd.pma = pma_idx;
                        int profile_counts[VERTEX_PER_PMA];
                        #pragma HLS ARRAY_PARTITION variable=profile_counts complete
                        ap_uint<512> count_word = separation_counts[i];
                        for (int v = 0; v < VERTEX_PER_PMA; v++) {
                            #pragma HLS UNROLL
                            profile_counts[v] = (int)count_word.range(v * 32 + 31, v * 32);
                            pre_cmd.count[v] = profile_counts[v];
                        }
                        bool need_expand = gate_needs_pre_expand(
                            hbm_gate, pma_raw_data_start, pma_vecinfo_start,
                            pma_idx, profile_counts);
                        pre_expand_checked[i] = true;
                        if (need_expand) {
                            set_lock(lock_req_out, pma_idx);
                            ap_wait();
                            pre_expand_out.write(pre_cmd);
                            continue;
                        }
                    }
                    // Check the manager-owned state, then mark this batch in
                    // flight before exposing any edge to the downstream path.
                    // if (!acquire_lock(lock_req_out, lock_rsp_in, completion_in,
                    //                   inflight_pma_count, pma_idx, true)) continue;
                    if (is_lock(lock_req_out, lock_rsp_in, pma_idx)) continue;
                    set_lock(lock_req_out, pma_idx);
                    // Strict cross-process order: lock commit -> edges.
                    ap_wait();
                    ap_uint<14> start = seperation_cursors[i];
                    ap_uint<14> end = seperations[i];
                    ap_uint<14> bs_end = start + ap_uint<14>(BS_MAX_CMD_NUM);
                    ap_uint<14> chunk_end = merge_path[i]
                        ? end : (bs_end < end ? bs_end : end);
                    bool to_merge = merge_path[i];
                    for (int j = start; j < chunk_end; j++)
                    {
                        int src = wave_buffer[j].src;
                        int dst = wave_buffer[j].dst;
                        int opcode = (int)wave_buffer[j].opcode;
                        PMA_UPDATE_MESSAGE msg;
                        msg.src = src; msg.dst = dst; msg.opcode = (PMA_UPDATE_OPCODE)opcode; msg.last = (j == chunk_end - 1);
                        if (to_merge) edge_out_merge.write(msg);
                        else          edge_out_bs.write(make_bs_token(msg));
                        // edge_out_bs.write(msg);
                    }
                    remaining -= (chunk_end - start);
                    seperation_cursors[i] = chunk_end;
                }
            }

            // Ordered wave barrier: consumers flush/age caches only after all
            // preceding updates, then ACK(-2) through the existing paths.
            PMA_UPDATE_MESSAGE wave_end;
            wave_end.src = -1;
            wave_end.dst = 0;
            wave_end.opcode = PMA_UPDATE_WAVE_END;
            wave_end.last = 1;
            edge_out_bs.write(make_bs_token(wave_end));
            edge_out_merge.write(wave_end);
            bool bs_done = false;
            bool merge_done = false;
            while (!bs_done || !merge_done) {
                ap_uint<1> route = wave_done_in.read();
                if (route == 0) bs_done = true;
                else            merge_done = true;
            }
        }
        ap_wait();
        // FLUSH is complete only here: for a non-empty wave both BS and merge
        // have returned CACHE_WAVE_ACK, which also means BS has drained all
        // redistribution work.  An empty worker has no state from this batch
        // to drain and can acknowledge immediately.
        if (flush) {
            send_router_ack(ack_to_router, worker_id);
            flush = false;
        }
        if (online_done) break;
        // 收尾 ACK (最后一波的批可能仍在飞) — 否则 inflight 不降 → 死循环
        // poll_completions(completion_in, inflight_pma_count);
        // if (online_done && inflight_pma_count == 0) break;
        // 全部处理完但未结束: 空转等 online (下一轮顶部继续收波)
        // if (wave_count == 0) ap_wait();
    }
    // while (inflight_pma_count > 0) {
    //     poll_completions(completion_in, inflight_pma_count);
    //     ap_wait();
    // }
    // Send STOP（所有批完成后最后执行）— 两条路径都要收, 各自收尾退出。
    // bs_insert 收到后经 redistribution 收尾 (redistribution 也由它带 STOP);
    // merge_pipeline 收到后直接 ack(-1) 退出 (不再与 redistribution 交互)。
    PMA_UPDATE_MESSAGE stop_msg;
    stop_msg.src = -1; stop_msg.dst = -1;
    stop_msg.opcode = PMA_UPDATE_STOP; stop_msg.last = 1;
    edge_out_bs.write(make_bs_token(stop_msg));
    edge_out_merge.write(stop_msg);
    // Wait until both downstream paths have really stopped.  The BS stop ACK
    // is emitted only after redistribution has forwarded STOP to alloc_access,
    // so this pair covers the whole worker pipeline rather than merely the
    // task_reader input boundary.
    bool bs_stopped = false;
    bool merge_stopped = false;
    while (!bs_stopped || !merge_stopped) {
        ap_uint<1> route = stop_done_in.read();
        if (route == 0) bs_stopped = true;
        else            merge_stopped = true;
    }

    // All real ACKs have now been consumed.  Stop the lock manager and wait
    // for its explicit completion before acknowledging STOP to the router.
    lock_req_t stop_req = 0;
    stop_req[33] = 1;
    while (!lock_req_out.write_nb(stop_req)) { ap_wait(); }
    (void)manager_done_in.read();
    send_router_ack(ack_to_router, worker_id);
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
        hls::stream<bs_update_token_t>& edge_out_bs,
        hls::stream<PMA_UPDATE_MESSAGE>& edge_out_merge,
        hls::stream<ap_uint<32>>& pre_ack_in,
        hls::stream<ap_uint<32>>& ack_in_bs,
        hls::stream<ap_uint<32>>& ack_in_merge,
        hls::stream<WHOLE_ACK>& ack_to_router)
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
        #pragma HLS INTERFACE axis port=ack_to_router

        // 内部通道: lock_req (main → manager, 深度 16 足够: main 提交慢, manager 消费快)
        //           unlock (manager → main, 深度 256: 主进程阻塞期间的 ack 缓冲垫, 见
        //           lock_manager 注释的深度不变量)
        hls::stream<lock_req_t> lock_req;
        // Front-end command buffer.  A batch in this design is at most one
        // wave plus its barrier; 64 entries covers the small-batch case that
        // exposed the router/reader handshake issue while retaining ordering.
        hls::stream<PMA_UPDATE_MESSAGE> online_fifo;
        hls::stream<ap_uint<1>> lock_rsp;
        hls::stream<ap_uint<1>> wave_done;
        hls::stream<ap_uint<1>> stop_done;
        hls::stream<ap_uint<1>> manager_done;
        // hls::stream<ap_uint<32>> completion;
        #pragma HLS STREAM variable=lock_req depth=16
        #pragma HLS STREAM variable=online_fifo depth=64
        #pragma HLS BIND_STORAGE variable=online_fifo type=fifo impl=bram
        #pragma HLS STREAM variable=lock_rsp depth=16
        #pragma HLS STREAM variable=wave_done depth=4
        #pragma HLS STREAM variable=stop_done depth=4
        #pragma HLS STREAM variable=manager_done depth=2
        // #pragma HLS STREAM variable=completion depth=256

        #pragma HLS DATAFLOW

        receive_online_commands(online_in, online_fifo);
        lock_manager(pre_ack_in, ack_in_bs, ack_in_merge, lock_req, lock_rsp,
                     wave_done, stop_done, manager_done, worker_id);
        task_reader_main(worker_id, hbm_gate, pma_raw_data_start, pma_vecinfo_start / 64,
                         online_fifo, pre_expand_out, edge_out_bs, edge_out_merge,
                         lock_req, lock_rsp, wave_done, stop_done, manager_done,
                         ack_to_router);
    }
}
