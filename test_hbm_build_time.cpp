// test_hbm_build_time.cpp
// Measure CPU time of add_base_graph_to_hbm: parse graph → build PMA → sync to device.
//
// Usage: ./test_hbm_build_time <xclbin> <graph_file>

#include "data_type.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <numeric>
#include <vector>

#include "xrt/xrt_bo.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"

struct Operation { int src, dst; long long ts; int opcode; };

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <xclbin> <graph_file>" << std::endl;
        return 1;
    }
    std::string xclbin_name = argv[1];
    std::string graph_path  = argv[2];

    // ---- Parse input ----
    auto t_parse_start = std::chrono::high_resolution_clock::now();
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
    double t_parse = std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now() - t_parse_start).count();

    // ---- Build BaseGraph ----
    auto t_build_start = std::chrono::high_resolution_clock::now();
    std::vector<std::vector<int>> BaseGraph(vertex_num);
    for (auto& o : base_ops) BaseGraph[o.src].push_back(o.dst);
    double t_graph = std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now() - t_build_start).count();

    std::cout << "Base: " << base_ops.size() << " edges, "
              << vertex_num << " vertices, " << total_pma << " PMAs" << std::endl;
    std::cout << "Parse: " << t_parse << " s, Graph build: " << t_graph << " s" << std::endl;

    // ---- Device + kernels ----
    auto device = xrt::device(0);
    auto uuid   = device.load_xclbin(xclbin_name);

    std::vector<xrt::kernel> mem_kernels, alloc_kernels;
    for (int i = 1; i <= CHANNEL_NUM; i++) {
        mem_kernels.emplace_back(device, uuid,
            "mem_access_kernel:{mem_access_kernel_" + std::to_string(i) + "}");
        alloc_kernels.emplace_back(device, uuid,
            "alloc_access_kernel:{alloc_access_kernel_" + std::to_string(i) + "}");
    }

    // ---- Allocate BOs ----
    std::vector<xrt::bo> bo_hbm, bo_vi, bo_bm;
    BYTE*  hbm_ptr[CHANNEL_NUM];
    VEC_INFO* vi_ptr[CHANNEL_NUM];
    BYTE*  bm_ptr[CHANNEL_NUM];

    for (int i = 0; i < CHANNEL_NUM; i++) {
        bo_hbm.emplace_back(device, sizeof(BYTE) * (size_t)CHANNEL_CAPACITY,
                            mem_kernels[i].group_id(0));
        hbm_ptr[i] = bo_hbm[i].map<BYTE*>();
        std::memset(hbm_ptr[i], 0, (size_t)CHANNEL_CAPACITY);

        bo_vi.emplace_back(device, sizeof(VEC_INFO) * MAX_PMA_NUM,
                           mem_kernels[i].group_id(1));
        vi_ptr[i] = bo_vi[i].map<VEC_INFO*>();
        std::memset(vi_ptr[i], 0, sizeof(VEC_INFO) * MAX_PMA_NUM);

        bo_bm.emplace_back(device,
            sizeof(BYTE) * ((size_t)CHANNEL_CAPACITY * CHANNEL_NUM / INIT_VEC_LEN_IN_BYTE / 8),
            alloc_kernels[i].group_id(0));
        bm_ptr[i] = bo_bm[i].map<BYTE*>();
        std::memset(bm_ptr[i], 0, bo_bm[i].size());
    }

    // ================================================================
    // TIMED: add_base_graph_to_hbm
    // ================================================================
    std::cout << "\n=== add_base_graph_to_hbm ===" << std::endl;
    auto t_hbm_start = std::chrono::high_resolution_clock::now();

    {
        int addr[CHANNEL_NUM] = {0, 0};

        for (int pma_idx = 0; pma_idx < total_pma; pma_idx++) {
            int ch = pma_idx % CHANNEL_NUM;
            int* off = &addr[ch];

            PMA_HEADER header;
            std::memset(&header, 0, sizeof(header));
            std::vector<int> seg_data;

            for (int v = 0; v < VERTEX_PER_PMA; v++) {
                int src = pma_idx * VERTEX_PER_PMA + v;
                std::vector<int> nb;
                if (src < (int)BaseGraph.size()) {
                    nb = BaseGraph[src];
                    std::sort(nb.begin(), nb.end());
                }
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

            std::memcpy(hbm_ptr[ch] + *off, &header, sizeof(header));
            std::memcpy(hbm_ptr[ch] + *off + sizeof(header), seg_data.data(),
                       seg_data.size() * sizeof(int));
            vi_ptr[ch][pma_idx] = VEC_INFO((uint64_t(total_bytes) << 32) | uint64_t(*off));
            *off += aligned;
        }

        // Bitmap init
        for (int ch = 0; ch < CHANNEL_NUM; ch++) {
            int blocks = (addr[ch] + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
            int full = blocks / 8, rem = blocks % 8;
            std::memset(bm_ptr[ch], 0xFF, full);
            if (rem) bm_ptr[ch][full] = (BYTE)((1 << rem) - 1);
        }
    }

    auto t_hbm_built = std::chrono::high_resolution_clock::now();
    double t_build = std::chrono::duration<double>(t_hbm_built - t_hbm_start).count();

    // ---- Sync to device ----
    for (int ch = 0; ch < CHANNEL_NUM; ch++) {
        bo_hbm[ch].sync(XCL_BO_SYNC_BO_TO_DEVICE);
        bo_vi[ch].sync(XCL_BO_SYNC_BO_TO_DEVICE);
        bo_bm[ch].sync(XCL_BO_SYNC_BO_TO_DEVICE);
    }

    auto t_hbm_end = std::chrono::high_resolution_clock::now();
    double t_sync = std::chrono::duration<double>(t_hbm_end - t_hbm_built).count();
    double t_total = std::chrono::duration<double>(t_hbm_end - t_hbm_start).count();

    // ---- Output ----
    std::cout << "  PMA build  : " << t_build << " s" << std::endl;
    std::cout << "  BO sync    : " << t_sync  << " s" << std::endl;
    std::cout << "  Total HBM  : " << t_total << " s" << std::endl;
    std::cout << "=== HBM_BUILD_TIME graph=" << graph_path
              << " pmas=" << total_pma
              << " edges=" << base_ops.size()
              << " build_s=" << t_build
              << " sync_s=" << t_sync
              << " total_s=" << t_total
              << " ===" << std::endl;

    return 0;
}
