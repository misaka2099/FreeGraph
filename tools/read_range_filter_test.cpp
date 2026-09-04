// read_range_filter_test — 验证 pma_read_worker 的 COO 范围过滤逻辑
// (160 位命令: opcode | src_start | src_end | dst_start | dst_end, -1 = 该维度不限)
//
// 模拟 worker 的遍历逻辑 (pma_lo/pma_hi 按 worker 序列对齐 + vertex 边界过滤 +
// dst 范围过滤), 与期望 (直接过滤原始边集) 对比。
//
// 编译: g++ -std=c++17 -O2 tools/read_range_filter_test.cpp -o tools/read_range_filter_test
// 运行: ./tools/read_range_filter_test

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

#define VERTEX_PER_PMA 16
#define CHANNEL_NUM 3

struct Edge {
    int src;
    int dst;
    bool operator==(const Edge& o) const { return src == o.src && dst == o.dst; }
};

// ---- 模拟 worker COO 范围遍历 (与 pma_read_worker_kernel.cpp 逻辑一致) ----
// graph: 每个 src 的邻居列表 (模拟 PMA segment 内容)
// worker_id, src_start/src_end/dst_start/dst_end: 命令字段 (-1 = 不限)
static std::vector<Edge> worker_coo_range(const std::vector<std::vector<int>>& graph,
                                          int worker_id,
                                          int src_start, int src_end,
                                          int dst_start, int dst_end) {
    const int READ_WORKERS = CHANNEL_NUM;
    bool all_src = (src_start == -1 || src_end == -1);
    bool all_dst = (dst_start == -1 || dst_end == -1);
    int total_pma = (int)((graph.size() + VERTEX_PER_PMA - 1) / VERTEX_PER_PMA);

    int pma_lo, pma_hi;
    if (all_src) {
        pma_lo = worker_id;
        pma_hi = -1;   // 哨兵: vec_info==0 终止 (用 total_pma 模拟)
    } else {
        int first_pma = src_start / VERTEX_PER_PMA;
        pma_lo = first_pma;
        int rem = pma_lo % READ_WORKERS;
        if (rem != worker_id) pma_lo += ((worker_id - rem) + READ_WORKERS) % READ_WORKERS;
        if (pma_lo < worker_id) pma_lo = worker_id;
        pma_hi = (src_end - 1) / VERTEX_PER_PMA;
    }

    std::vector<Edge> out;
    for (int pma_idx = pma_lo; ; pma_idx += READ_WORKERS) {
        if (!all_src && pma_idx > pma_hi) break;
        if (pma_idx >= total_pma) break;   // vec_info == 0
        for (int local_v = 0; local_v < VERTEX_PER_PMA; local_v++) {
            int src = pma_idx * VERTEX_PER_PMA + local_v;
            if (!all_src && (src < src_start || src >= src_end)) continue;
            if (src >= (int)graph.size()) continue;
            for (int neighbor : graph[src]) {
                if (all_dst || (neighbor >= dst_start && neighbor < dst_end)) {
                    out.push_back({src, neighbor});
                }
            }
        }
    }
    return out;
}

// 期望: 直接过滤原始边集
static std::vector<Edge> expected_filter(const std::vector<std::vector<int>>& graph,
                                         int worker_id,
                                         int src_start, int src_end,
                                         int dst_start, int dst_end) {
    bool all_src = (src_start == -1 || src_end == -1);
    bool all_dst = (dst_start == -1 || dst_end == -1);
    std::vector<Edge> out;
    for (int src = 0; src < (int)graph.size(); src++) {
        int pma = src / VERTEX_PER_PMA;
        if (pma % CHANNEL_NUM != worker_id) continue;   // worker 按 PMA 序列分配 (pma%3)
        if (!all_src && (src < src_start || src >= src_end)) continue;
        for (int neighbor : graph[src]) {
            if (all_dst || (neighbor >= dst_start && neighbor < dst_end)) {
                out.push_back({src, neighbor});
            }
        }
    }
    return out;
}

static int g_failures = 0;

static void check_case(const std::string& name,
                       const std::vector<std::vector<int>>& graph,
                       int worker_id, int src_start, int src_end,
                       int dst_start, int dst_end) {
    auto got = worker_coo_range(graph, worker_id, src_start, src_end, dst_start, dst_end);
    auto exp = expected_filter(graph, worker_id, src_start, src_end, dst_start, dst_end);
    std::sort(got.begin(), got.end(), [](const Edge& a, const Edge& b) {
        if (a.src != b.src) return a.src < b.src;
        return a.dst < b.dst;
    });
    std::sort(exp.begin(), exp.end(), [](const Edge& a, const Edge& b) {
        if (a.src != b.src) return a.src < b.src;
        return a.dst < b.dst;
    });
    bool ok = (got == exp);
    if (!ok) {
        std::cout << "  [FAIL] " << name << " (worker=" << worker_id
                  << " src=[" << src_start << "," << src_end << ") dst=["
                  << dst_start << "," << dst_end << ")): got " << got.size()
                  << " edges, expected " << exp.size() << std::endl;
        g_failures++;
    } else {
        std::cout << "  [OK] " << name << " (" << got.size() << " edges)" << std::endl;
    }
}

int main() {
    // 图: 60 个顶点 (4 个 PMA), 每个 src 有若干邻居 (含重复 + 各种 dst 值)
    std::vector<std::vector<int>> graph(60);
    std::srand(12345);
    for (int src = 0; src < 60; src++) {
        int n = 1 + std::rand() % 10;
        for (int k = 0; k < n; k++) graph[src].push_back(std::rand() % 60);
    }

    std::cout << "=== COO 范围过滤验证 ===" << std::endl;

    // 1. 全图遍历 (都 -1)
    for (int w = 0; w < CHANNEL_NUM; w++) {
        check_case("全图 worker" + std::to_string(w), graph, w, -1, -1, -1, -1);
    }

    // 2. 只有 dst 范围 (src 不限)
    check_case("dst=[10,30) worker0", graph, 0, -1, -1, 10, 30);
    check_case("dst=[0,15) worker1", graph, 1, -1, -1, 0, 15);
    check_case("dst 空范围 worker2", graph, 2, -1, -1, 100, 200);

    // 3. 只有 src 范围 (dst 不限)
    check_case("src=[0,32) worker0", graph, 0, 0, 32, -1, -1);
    check_case("src=[16,48) worker1", graph, 1, 16, 48, -1, -1);
    check_case("src 空范围 worker2", graph, 2, 60, 90, -1, -1);

    // 4. src + dst 组合
    check_case("src=[8,40) dst=[5,35) worker0", graph, 0, 8, 40, 5, 35);
    check_case("src=[0,16) dst=[20,50) worker2", graph, 2, 0, 16, 20, 50);

    // 5. 边界: src_end 超出实际顶点数
    check_case("src=[40,100) worker1", graph, 1, 40, 100, -1, -1);
    // 6. 边界: src 范围跨 PMA 边界 (16 的倍数)
    check_case("src=[15,17) worker0 (跨 PMA)", graph, 0, 15, 17, -1, -1);
    check_case("src=[15,17) worker1 (跨 PMA)", graph, 1, 15, 17, -1, -1);
    check_case("src=[15,17) worker2 (跨 PMA)", graph, 2, 15, 17, -1, -1);
    // 7. 边界: src_start 非对齐 (PMA 对齐逻辑)
    check_case("src=[17,35) worker1", graph, 1, 17, 35, -1, -1);
    check_case("src=[5,20) worker2", graph, 2, 5, 20, -1, -1);
    // 8. 单侧 -1: src_start=-1, src_end=32 (src 全遍历语义)
    check_case("src 起点开放 [-1,32) worker0", graph, 0, -1, 32, -1, -1);
    // 9. 单侧 -1: dst_start=10, dst_end=-1 (dst 全遍历语义)
    check_case("dst 终点开放 [10,-1) worker1", graph, 1, -1, -1, 10, -1);

    std::cout << (g_failures == 0 ? "ALL PASS" : "FAILURES: " + std::to_string(g_failures))
              << std::endl;
    return g_failures == 0 ? 0 : 1;
}
