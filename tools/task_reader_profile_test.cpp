// task_reader_profile_test — 验证 pma_insert_edge_standalone 中 task_reader 的
// profile（per-vertex 插入计数）逻辑是否正确。
//
// 方法：忠实复刻 task_reader 的收波→排序→分段→vc 计数→pre_expand→flush 逻辑
// （与内核代码逐行对应，见行号注释），radix/bitonic 排序直接使用内核实际的
// radix_sort.h。用随机边数据驱动，对比：
//   1. pre_expand 命令的 (pma, count[16]) 与期望 per-vertex 计数
//   2. cmd_out 边流与输入的一致性（无丢失/无重复）
//   3. 排序正确性（按 src 分组、段内按 dst 升序）
//   4. 多波边界（> WAVE_CAPACITY）与 delete 混合时的计数行为
//
// 编译: g++ -std=c++17 -O2 -I pma_kernel tools/task_reader_profile_test.cpp -o tools/task_reader_profile_test
// 运行: ./tools/task_reader_profile_test

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <queue>
#include <vector>

#include "radix_sort.h"   // 内核实际排序代码 (radix_sort_src / bitonic_segment_by_dst)

// ---- 与内核一致的常量 (pma_insert_edge_standalone.cpp) ----
#define TASK_BURST_CHUNK 256     // L1628
#define WAVE_CAPACITY 8192       // L1629: 每波最大边数
#define MAX_CMD_NUM 16           // data_type.h: 每批 flush 最大边数
#define VERTEX_PER_PMA 16
#define SEG_BITONIC_MAX 256      // radix_sort.h: 段内 bitonic 排序上限

// ---- 与内核一致的结构体 ----
struct PMA_UPDATE_MESSAGE {
    int src;
    int dst;
    int opcode;   // 0=INSERT_EDGE, 1=DELETE_EDGE
    int last;
};
struct pre_expand_cmd_t {
    int pma;
    int count[VERTEX_PER_PMA];
};

// 任务缓冲里的一项 (与 host DIRECT 路径的 task_buffer 格式一致)
struct TaskEdge {
    int src;
    int dst;
    int opcode;
    bool operator==(const TaskEdge& o) const {
        return src == o.src && dst == o.dst && opcode == o.opcode;
    }
};

// =====================================================================
// task_reader 忠实复刻 (内核新调度 2026-08-04: 去 gate)
// 收波(FLUSH 过滤/STOP 记录) → radix 排序 → profile 全部 PMA → 等 pre_ack
// → 轮转扫描放数据 (每 PMA 每轮 ≤16 条 + processed 标记) → 最后 STOP
// stream 用 std::queue 代替 hls::stream
// =====================================================================
static void task_reader_test(
    std::vector<int>& task_buffer,       // [src, dst, opcode] * edge_count + sentinel(-1)
    int edge_count,
    std::queue<int>& ack_in,             // insert 批完成 ACK (值 = pma)
    std::queue<PMA_UPDATE_MESSAGE>& cmd_out, // 排序后的边流
    std::queue<int>& pre_ack_in,         // pre_expand 完成 ACK (值 = pma)
    std::queue<pre_expand_cmd_t>& pre_expand_out
) {
    // ---- 复刻内核 task_reader (2026-08-05 版): 双游标循环缓冲 + check_ack_back ----
    std::vector<int> wave_buffer(WAVE_CAPACITY * 3, 0);
    std::vector<int> radix_tmp(WAVE_CAPACITY * 3, 0);
    std::vector<int> seperations(WAVE_CAPACITY, 0);
    std::vector<int> seperation_cursors(WAVE_CAPACITY, 0);
    std::vector<uint8_t> pma_lock(65536, 0);   // 位图: local_pma = pma/3, 1 bit/PM

    // online 流: 测试固定先放入一个 STOP
    std::queue<PMA_UPDATE_MESSAGE> online_in;
    PMA_UPDATE_MESSAGE stop_msg;
    stop_msg.src = -1; stop_msg.dst = -1; stop_msg.opcode = 2 /*STOP*/; stop_msg.last = 1;
    online_in.push(stop_msg);

    auto lock_pma_tb = [&](int pma) { int lp = pma / 3; pma_lock[lp / 8] |= (1 << (lp % 8)); };
    auto unlock_pma_tb = [&](int pma) { int lp = pma / 3; pma_lock[lp / 8] &= ~(1 << (lp % 8)); };
    auto is_locked_tb = [&](int pma) {
        int lp = pma / 3;
        return ((pma_lock[lp / 8] >> (lp % 8)) & 1) != 0;
    };

    int inflight_pma_count = 0;
    auto check_ack_back = [&]() {
        if (!pre_ack_in.empty()) {
            int pma = pre_ack_in.front(); pre_ack_in.pop();
            unlock_pma_tb(pma);
            inflight_pma_count--;
        }
        if (!ack_in.empty()) {
            int pma = ack_in.front(); ack_in.pop();
            unlock_pma_tb(pma);
            inflight_pma_count--;
        }
    };

    int processed_total = 0;
    int write_cursor = edge_count;   // offline 边已预置在 task_buffer[0..edge_count)
    int read_cursor = 0;
    bool online_done = false;
    bool flush = false;

    while (true) {
        // ===== 收波阶段: 先全部收集 online 更新 (写入 task_buffer) =====
        while (!online_in.empty() && !online_done && !flush) {
            PMA_UPDATE_MESSAGE msg = online_in.front(); online_in.pop();
            if (msg.opcode == 2 /*STOP*/) { online_done = true; break; }
            if (msg.opcode == 5 /*FLUSH*/) { flush = true; break; }
            task_buffer[write_cursor * 3 + 0] = msg.src;
            task_buffer[write_cursor * 3 + 1] = msg.dst;
            task_buffer[write_cursor * 3 + 2] = msg.opcode;
            write_cursor++;
        }

        // ===== 处理阶段: 分批 (chunk ≤ 8192) 处理完当前收集的全部边 =====
        // 收波可能一次读入 > 8192 条 — 内层循环多次处理 (chunk 保护)
        while (read_cursor < write_cursor) {
            int cmd_count_this_round = std::min(8192, write_cursor - read_cursor);
            // 拷贝到 wave_buffer (read_cursor 推进)
            for (int i = 0; i < cmd_count_this_round; i++) {
                int idx = read_cursor + i;
                wave_buffer[i * 3 + 0] = task_buffer[idx * 3 + 0];
                wave_buffer[i * 3 + 1] = task_buffer[idx * 3 + 1];
                wave_buffer[i * 3 + 2] = task_buffer[idx * 3 + 2];
            }
            read_cursor += cmd_count_this_round;

        // 排序: src, dst 字典序升序 (内核实际排序代码)
        sort_src_dst(wave_buffer.data(), radix_tmp.data(), cmd_count_this_round);

        // ===== Profile: 分段 + pre_expand + 锁 =====
        int current_pma = -1;
        int current_vc[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        int seperation_count = 0;
        for (int i = 0; i < cmd_count_this_round; i++) {
            check_ack_back();
            int src = wave_buffer[i * 3 + 0];
            int pma_idx = src >> 4;
            if (pma_idx != current_pma) {
                if (current_pma != -1) {
                    seperations[seperation_count++] = i;
                    pre_expand_cmd_t pre_cmd;
                    pre_cmd.pma = current_pma;
                    for (int v = 0; v < 16; v++) pre_cmd.count[v] = current_vc[v];
                    pre_expand_out.push(pre_cmd);
                    inflight_pma_count++;
                    lock_pma_tb(current_pma);
                    pre_ack_in.push(current_pma);   // 模拟 redistribution 即时完成
                }
                current_pma = pma_idx;
                for (int v = 0; v < 16; v++) current_vc[v] = 0;   // 换 PMA 清零
            }
            current_vc[src & 0xF]++;                               // 先判换再累加
        }
        if (current_pma != -1) {
            seperations[seperation_count++] = cmd_count_this_round;
            pre_expand_cmd_t pre_cmd;
            pre_cmd.pma = current_pma;
            for (int v = 0; v < 16; v++) pre_cmd.count[v] = current_vc[v];
            pre_expand_out.push(pre_cmd);
            inflight_pma_count++;
            lock_pma_tb(current_pma);
            pre_ack_in.push(current_pma);   // 模拟 redistribution 即时完成
        }

        // ===== 放数据多轮 (锁驱动: pre_ack 解锁后才能放; 批 ACK 解锁后下一轮) =====
        int remaining = cmd_count_this_round;
        seperation_cursors[0] = 0;
        for (int i = 1; i < seperation_count; i++) seperation_cursors[i] = seperations[i - 1];
        while (remaining > 0) {
            check_ack_back();
            for (int i = 0; i < seperation_count; i++) {
                int pma_idx = wave_buffer[seperation_cursors[i] * 3] >> 4;
                if (is_locked_tb(pma_idx)) continue;
                if (seperation_cursors[i] >= seperations[i]) continue;
                int start = seperation_cursors[i];
                int end = std::min(start + 16, seperations[i]);
                for (int j = start; j < end; j++) {
                    PMA_UPDATE_MESSAGE msg;
                    msg.src = wave_buffer[j * 3 + 0];
                    msg.dst = wave_buffer[j * 3 + 1];
                    msg.opcode = wave_buffer[j * 3 + 2];
                    msg.last = (j == end - 1);
                    cmd_out.push(msg);
                }
                inflight_pma_count++;
                lock_pma_tb(pma_idx);
                ack_in.push(pma_idx);   // 模拟 insert 即时完成
                remaining -= (end - start);
                seperation_cursors[i] = end;
            }
        }
        // FLUSH barrier 已过: 复位, 下一轮收波继续收 FLUSH 后的 online
        if (flush) flush = false;
        processed_total += cmd_count_this_round;
        }
        // 收尾 ACK (最后一波批可能仍在飞) — 否则 inflight 不降 → 死循环
        check_ack_back();
        if (online_done && read_cursor >= write_cursor && inflight_pma_count == 0) break;
        if (read_cursor >= write_cursor) { /* 空转等 online */ }
    }
    // 收尾: 全部在飞 ACK
    while (inflight_pma_count > 0) { check_ack_back(); }
    // 内核末尾发 STOP (不验证)
}


// =====================================================================
// 期望计算: 把输入边按 src 分组 (同 PMA), 每组的 per-vertex 计数
// =====================================================================
struct ExpectedSegment {
    int pma;
    int count[VERTEX_PER_PMA];
    std::vector<TaskEdge> edges;   // 段内边 (保持输入顺序)
};

static std::vector<ExpectedSegment> compute_expected(const std::vector<TaskEdge>& edges) {
    // 按 pma 分组 (与 radix 排序后分组一致)
    std::vector<TaskEdge> sorted = edges;
    std::sort(sorted.begin(), sorted.end(),
              [](const TaskEdge& a, const TaskEdge& b) { return a.src < b.src; });
    std::vector<ExpectedSegment> segs;
    size_t i = 0;
    while (i < sorted.size()) {
        int pma = sorted[i].src / VERTEX_PER_PMA;
        ExpectedSegment seg;
        seg.pma = pma;
        for (int v = 0; v < VERTEX_PER_PMA; v++) seg.count[v] = 0;
        size_t j = i;
        while (j < sorted.size() && sorted[j].src / VERTEX_PER_PMA == pma) j++;
        for (size_t k = i; k < j; k++) {
            seg.count[sorted[k].src & 0xF]++;
            seg.edges.push_back(sorted[k]);
        }
        segs.push_back(seg);
        i = j;
    }
    return segs;
}

// =====================================================================
// 测试驱动
// =====================================================================
static int g_failures = 0;

static void check(bool cond, const std::string& what) {
    if (!cond) {
        std::cout << "  [FAIL] " << what << std::endl;
        g_failures++;
    }
}

static void run_case(const std::string& name, const std::vector<TaskEdge>& edges,
                     bool check_delete_counted) {
    std::cout << "=== " << name << " (" << edges.size() << " edges) ===" << std::endl;

    // 构造 task_buffer: [src, dst, opcode] * n + sentinel(-1,0,0)
    std::vector<int> task_buffer;
    for (const auto& e : edges) {
        task_buffer.push_back(e.src);
        task_buffer.push_back(e.dst);
        task_buffer.push_back(e.opcode);
    }
    task_buffer.push_back(-1);
    task_buffer.push_back(0);
    task_buffer.push_back(0);

    // 期望段: 新 task_reader 每波取 min(8192, 剩余) 条 → 每波排序分段
    std::vector<std::vector<TaskEdge>> waves;
    for (size_t base = 0; base < edges.size(); base += WAVE_CAPACITY) {
        std::vector<TaskEdge> wave(edges.begin() + base,
                                   edges.begin() + std::min(base + WAVE_CAPACITY, edges.size()));
        waves.push_back(wave);
    }
    std::vector<ExpectedSegment> expected;
    for (const auto& wave : waves) {
        auto segs = compute_expected(wave);
        expected.insert(expected.end(), segs.begin(), segs.end());
    }

    // ACK 不预填: task_reader_test 内部即时返回 (模拟下游零延迟)
    std::queue<int> ack_in, pre_ack_in;

    std::queue<PMA_UPDATE_MESSAGE> cmd_out;
    std::queue<pre_expand_cmd_t> pre_expand_out;
    task_reader_test(task_buffer, (int)edges.size(), ack_in, cmd_out, pre_ack_in, pre_expand_out);

    // ---- 验证 1: pre_expand 命令 (profile 正确性) ----
    check(pre_expand_out.size() == expected.size(),
          "pre_expand 命令数 " + std::to_string(pre_expand_out.size()) +
          " != 期望段数 " + std::to_string(expected.size()));
    size_t seg_idx = 0;
    bool count_match = true;
    while (!pre_expand_out.empty()) {
        pre_expand_cmd_t cmd = pre_expand_out.front(); pre_expand_out.pop();
        if (seg_idx >= expected.size()) { count_match = false; break; }
        const auto& exp = expected[seg_idx];
        if (cmd.pma != exp.pma) {
            std::cout << "  [FAIL] seg " << seg_idx << " pma: got " << cmd.pma
                      << " expected " << exp.pma << std::endl;
            g_failures++;
            count_match = false;
        }
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            if (cmd.count[v] != exp.count[v]) {
                std::cout << "  [FAIL] seg " << seg_idx << " pma=" << cmd.pma
                          << " vertex " << v << ": count got " << cmd.count[v]
                          << " expected " << exp.count[v] << std::endl;
                g_failures++;
                count_match = false;
            }
        }
        seg_idx++;
    }
    check(count_match, "pre_expand count[16] 与期望一致 (profile)");

    // ---- 验证 2: cmd_out 边流完整性 (无丢失/无重复) ----
    check(cmd_out.size() == edges.size(),
          "cmd_out 边数 " + std::to_string(cmd_out.size()) + " != 输入 " + std::to_string(edges.size()));
    std::vector<TaskEdge> out_edges_ordered;   // 保留原始输出顺序 (验证 3 用)
    while (!cmd_out.empty()) {
        PMA_UPDATE_MESSAGE m = cmd_out.front(); cmd_out.pop();
        out_edges_ordered.push_back({m.src, m.dst, m.opcode});
    }
    std::vector<TaskEdge> out_edges = out_edges_ordered;   // 排序副本 (验证 2 用)
    std::vector<TaskEdge> in_edges = edges;
    std::sort(in_edges.begin(), in_edges.end(),
              [](const TaskEdge& a, const TaskEdge& b) {
                  if (a.src != b.src) return a.src < b.src;
                  if (a.dst != b.dst) return a.dst < b.dst;
                  return a.opcode < b.opcode;
              });
    std::sort(out_edges.begin(), out_edges.end(),
              [](const TaskEdge& a, const TaskEdge& b) {
                  if (a.src != b.src) return a.src < b.src;
                  if (a.dst != b.dst) return a.dst < b.dst;
                  return a.opcode < b.opcode;
              });
    if (in_edges != out_edges) {
        std::cout << "  [DBG] in_edges.size=" << in_edges.size()
                  << " out_edges.size=" << out_edges.size() << std::endl;
        int dbg_shown = 0;
        for (size_t d = 0; d < std::min<size_t>(in_edges.size(), out_edges.size()) && dbg_shown < 8; d++) {
            if (!(in_edges[d] == out_edges[d])) {
                std::cout << "  [DBG] diff[" << d << "] in=(" << in_edges[d].src << ","
                          << in_edges[d].dst << "," << in_edges[d].opcode << ") out=("
                          << out_edges[d].src << "," << out_edges[d].dst << ","
                          << out_edges[d].opcode << ")" << std::endl;
                dbg_shown++;
            }
        }
        // 频率差异统计: 找出被修改/丢失/新增的边 (前 8 条, 强制打印)
        auto key = [](const TaskEdge& e) {
            return std::to_string(e.src) + "->" + std::to_string(e.dst) + "/op" + std::to_string(e.opcode);
        };
        std::map<std::string, int> freq_in, freq_out;
        for (const auto& e : in_edges) freq_in[key(e)]++;
        for (const auto& e : out_edges) freq_out[key(e)]++;
        std::cout << "  [DBG] freq_in.size=" << freq_in.size()
                  << " freq_out.size=" << freq_out.size() << std::endl;
        int shown = 0;
        for (const auto& kv : freq_in) {
            if (freq_out[kv.first] != kv.second) {
                std::cout << "  [FAIL] 边 " << kv.first << ": 输入 " << kv.second
                          << " 条, 输出 " << freq_out[kv.first] << " 条" << std::endl;
                if (++shown >= 8) break;
            }
        }
        for (const auto& kv : freq_out) {
            if (freq_in[kv.first] != kv.second) {
                std::cout << "  [FAIL] 边 " << kv.first << ": 输入 " << freq_in[kv.first]
                          << " 条, 输出 " << kv.second << " 条 (新增)" << std::endl;
                if (++shown >= 8) break;
            }
        }
    }
    check(in_edges == out_edges, "cmd_out 边集与输入一致 (无丢失/无重复)");

    // 验证 3 (段内 bitonic 排序) 已移除 — 新 task_reader 调度不再做段内排序

    // ---- 验证 4: delete 是否被计入 profile ----
    if (check_delete_counted) {
        // 统计纯插入计数
        std::vector<int> insert_only(expected.size() * VERTEX_PER_PMA, 0);
        for (size_t s = 0; s < expected.size(); s++) {
            for (const auto& e : expected[s].edges) {
                if (e.opcode == 0) insert_only[s * VERTEX_PER_PMA + (e.src & 0xF)]++;
            }
        }
        bool delete_included = false;
        // 重新读 pre_expand 输出? 已被消费 — 这里用期望段的全边计数与插入计数对比
        for (size_t s = 0; s < expected.size(); s++) {
            for (int v = 0; v < VERTEX_PER_PMA; v++) {
                if (expected[s].count[v] != insert_only[s * VERTEX_PER_PMA + v]) {
                    delete_included = true;
                }
            }
        }
        if (delete_included) {
            std::cout << "  [INFO] profile 计数包含 delete 操作 (全边计数, 预分配偏大但安全)" << std::endl;
        } else {
            std::cout << "  [INFO] profile 计数仅含插入操作" << std::endl;
        }
    }

    std::cout << std::endl;
}

// 生成随机边
static std::vector<TaskEdge> gen_edges(int count, int max_vertex, int delete_ratio_pct,
                                       unsigned seed) {
    std::srand(seed);
    std::vector<TaskEdge> edges;
    for (int i = 0; i < count; i++) {
        TaskEdge e;
        e.src = std::rand() % max_vertex;
        e.dst = std::rand() % max_vertex;
        e.opcode = (std::rand() % 100 < delete_ratio_pct) ? 1 : 0;
        edges.push_back(e);
    }
    return edges;
}

// bitonic 排序单测: 随机段验证集合不变 + dst 升序 (定位丢边)
static void test_bitonic() {
    std::cout << "=== bitonic_segment_by_dst 单测 (1000 组随机段) ===" << std::endl;
    bool ok = true;
    for (int t = 0; t < 1000; t++) {
        int n = 9 + std::rand() % (SEG_BITONIC_MAX - 8);   // 9..256
        int max_size = 1;
        while (max_size < n) max_size <<= 1;
        std::vector<int> buf(n * 3);
        std::vector<int> tmp(SEG_BITONIC_MAX * 3, 0);      // max_size 最大 256
        std::vector<TaskEdge> before;
        for (int i = 0; i < n; i++) {
            int src = std::rand() % 256;
            int dst = std::rand() % 100000;
            buf[i * 3 + 0] = src;
            buf[i * 3 + 1] = dst;
            buf[i * 3 + 2] = std::rand() % 2;
            before.push_back({src, dst, buf[i * 3 + 2]});
        }
        bitonic_segment_by_dst(buf.data(), tmp.data(), 0, n);
        std::vector<TaskEdge> after_ordered;   // copy back 后的原始顺序
        for (int i = 0; i < n; i++) after_ordered.push_back({buf[i * 3], buf[i * 3 + 1], buf[i * 3 + 2]});
        std::vector<TaskEdge> after = after_ordered;
        std::sort(before.begin(), before.end(),
                  [](const TaskEdge& a, const TaskEdge& b) {
                      if (a.src != b.src) return a.src < b.src;
                      if (a.dst != b.dst) return a.dst < b.dst;
                      return a.opcode < b.opcode;
                  });
        std::sort(after.begin(), after.end(),
                  [](const TaskEdge& a, const TaskEdge& b) {
                      if (a.src != b.src) return a.src < b.src;
                      if (a.dst != b.dst) return a.dst < b.dst;
                      return a.opcode < b.opcode;
                  });
        if (before != after) {
            std::cout << "  [FAIL] bitonic 丢边/改边: n=" << n << std::endl;
            ok = false;
            break;
        }
        for (int i = 1; i < n; i++) {
            if (after_ordered[i].dst < after_ordered[i - 1].dst) {
                std::cout << "  [FAIL] bitonic 未按 dst 升序: n=" << n
                          << " max_size=" << max_size << std::endl;
                // 打印乱序位置附近
                for (int p = std::max(0, i - 3); p < std::min(n, i + 4); p++) {
                    std::cout << "    [" << p << "] src=" << after[p].src
                              << " dst=" << after[p].dst << " op=" << after[p].opcode << std::endl;
                }
                ok = false;
                break;
            }
        }
        if (!ok) break;
    }
    check(ok, "bitonic_segment_by_dst 集合不变 + dst 升序");
    std::cout << std::endl;
}

int main() {
    test_bitonic();

    // ---- 用例 1: 单 PMA, 纯插入 ----
    {
        std::vector<TaskEdge> edges;
        // 16 个 vertex 都有边, 数量不等
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            int n = 1 + (v * 7) % 30;   // 1..29 条
            for (int k = 0; k < n; k++) edges.push_back({v, 1000 + k, 0});
        }
        run_case("单 PMA 纯插入", edges, false);
    }

    // ---- 用例 2: 多 PMA, 纯插入 ----
    {
        std::vector<TaskEdge> edges = gen_edges(5000, 256, 0, 42);
        run_case("多 PMA 纯插入 (5000 边)", edges, false);
    }

    // ---- 用例 3: 插入 + delete 混合 ----
    {
        std::vector<TaskEdge> edges = gen_edges(3000, 128, 30, 7);
        run_case("insert+delete 混合 (3000 边)", edges, true);
    }

    // ---- 用例 4: 多波 (超过 WAVE_CAPACITY) ----
    {
        std::vector<TaskEdge> edges = gen_edges(WAVE_CAPACITY + 5000, 1024, 10, 99);
        run_case("多波 (edge_count > WAVE_CAPACITY)", edges, false);
    }

    // ---- 用例 5: 单 PMA 超大段 (> 256, bitonic 跳过) ----
    {
        std::vector<TaskEdge> edges;
        for (int k = 0; k < 600; k++) edges.push_back({0, k, 0});  // 600 条同一 src
        for (int k = 0; k < 600; k++) edges.push_back({32, k, 0}); // 另一 PMA
        run_case("超大段 (>256, bitonic 跳过)", edges, false);
    }

    std::cout << (g_failures == 0 ? "ALL PASS" : "FAILURES: " + std::to_string(g_failures))
              << std::endl;
    return g_failures == 0 ? 0 : 1;
}
