// bench_hbm_build.cpp — Pure CPU benchmark of add_base_graph_to_hbm.
// No FPGA, no xclbin. Measures PMA data structure construction time in host memory.
//
// Compile: g++ -std=c++17 -O2 -I../include -o bench_hbm_build bench_hbm_build.cpp
// Usage:   ./bench_hbm_build <graph_file>

#include "data_type.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

struct Operation { int src, dst; long long ts; int opcode; };

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <graph_file>" << std::endl;
        return 1;
    }
    std::string graph_path = argv[1];

    // ---- Parse input ----
    auto t0 = std::chrono::high_resolution_clock::now();
    std::ifstream f(graph_path);
    if (!f) { std::cerr << "FAILED to open " << graph_path << std::endl; return 1; }
    std::vector<Operation> all_ops;
    { int a,b,op; long long ts; while (f>>a>>b>>ts>>op) all_ops.push_back({a,b,ts,op}); }
    f.close();

    std::vector<Operation> base_ops;
    int max_vertex = -1;
    for (auto& o : all_ops) {
        if (o.opcode == PMA_UPDATE_INSERT_EDGE && o.ts == -1) {
            max_vertex = std::max({max_vertex, o.src, o.dst});
            base_ops.push_back(o);
        }
    }
    int vertex_num = max_vertex + 1;
    int total_pma  = (vertex_num + VERTEX_PER_PMA - 1) / VERTEX_PER_PMA;

    // ---- Build adjacency lists ----
    std::vector<std::vector<int>> BaseGraph(vertex_num);
    for (auto& o : base_ops) BaseGraph[o.src].push_back(o.dst);
    for (auto& nb : BaseGraph) std::sort(nb.begin(), nb.end());

    double t_parse = std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now() - t0).count();

    // ================================================================
    // TIMED: add_base_graph_to_hbm (CPU-only, no device sync)
    // ================================================================
    auto t_start = std::chrono::high_resolution_clock::now();

    // Simulate HBM buffer allocation (2 channels)
    std::vector<BYTE> hbm[2];
    hbm[0].resize((size_t)CHANNEL_CAPACITY);
    hbm[1].resize((size_t)CHANNEL_CAPACITY);
    std::vector<VEC_INFO> vi[2];
    vi[0].resize(MAX_PMA_NUM);
    vi[1].resize(MAX_PMA_NUM);
    int addr[2] = {0, 0};

    for (int pma_idx = 0; pma_idx < total_pma; pma_idx++) {
        int ch = pma_idx % 2;
        int* off = &addr[ch];

        PMA_HEADER header;
        std::memset(&header, 0, sizeof(header));
        std::vector<int> seg_data;

        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            int src = pma_idx * VERTEX_PER_PMA + v;
            std::vector<int> nb;
            if (src < (int)BaseGraph.size()) nb = BaseGraph[src];
            header.edge_count[v] = (int)nb.size();
            int last_end = (v == 0) ? 0 : header.vertex_range[v - 1];
            int segs = 1;
            if (!nb.empty()) { while (segs * 47 < (int)nb.size()) segs *= 4; }
            header.vertex_range[v] = last_end + segs;

            int base_n = segs > 0 ? (int)nb.size() / segs : 0;
            int extra  = segs > 0 ? (int)nb.size() % segs : 0;
            int ni = 0;
            for (int s = 0; s < segs; s++) {
                int cnt = base_n + (s < extra ? 1 : 0);
                for (int k = 0; k < cnt; k++) seg_data.push_back(nb[ni++]);
                for (int k = cnt; k < EDGE_SLOTS_PER_SEG; k++) seg_data.push_back(UNUSED_ICON);
                seg_data.push_back(cnt);
            }
        }

        int total_bytes = (int)(sizeof(header) + seg_data.size() * sizeof(int));
        int aligned = total_bytes;
        if (aligned % INIT_VEC_LEN_IN_BYTE)
            aligned += INIT_VEC_LEN_IN_BYTE - (aligned % INIT_VEC_LEN_IN_BYTE);

        std::memcpy(&hbm[ch][*off], &header, sizeof(header));
        std::memcpy(&hbm[ch][*off] + sizeof(header), seg_data.data(),
                   seg_data.size() * sizeof(int));
        vi[ch][pma_idx] = VEC_INFO((uint64_t(total_bytes) << 32) | uint64_t(*off));
        *off += aligned;
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    double t_build = std::chrono::duration<double>(t_end - t_start).count();

    // ---- Output ----
    std::cout << "graph=" << graph_path
              << " edges=" << base_ops.size()
              << " vertices=" << vertex_num
              << " pmas=" << total_pma
              << " ch0_bytes=" << addr[0]
              << " ch1_bytes=" << addr[1]
              << " parse_s=" << t_parse
              << " build_s=" << t_build
              << std::endl;

    return 0;
}
