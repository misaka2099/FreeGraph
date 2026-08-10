// test_read_latency.cpp
// Minimal host: build PMA → GAS query → measure cmd-to-first-data latency distribution.
// Reads overlap test data format: src dst timestamp opcode (opcode=6 for GAS vertices).
//
// Usage: ./test_read_latency <xclbin> <graph_file>

#include "data_type.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

#include "xrt/xrt_bo.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"

struct Operation { int src, dst, ts, opcode; };

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <xclbin> <graph_file>" << std::endl;
        return 1;
    }
    std::string xclbin_name = argv[1];
    std::string graph_path  = argv[2];

    // ---- Parse input ----
    std::ifstream f(graph_path);
    if (!f) { std::cerr << "FAILED to open " << graph_path << std::endl; return 1; }
    std::vector<Operation> all_ops;
    { int a,b,ts,op; while (f>>a>>b>>ts>>op) all_ops.push_back({a,b,ts,op}); }
    f.close();

    std::vector<Operation> base_ops, gas_ops;
    int max_vertex = -1;
    for (auto& o : all_ops) {
        if (o.opcode == PMA_UPDATE_INSERT_EDGE) {
            max_vertex = std::max({max_vertex, o.src, o.dst});
            if (o.ts == -1) base_ops.push_back(o);
        } else if (o.opcode == PMA_UPDATE_GAS_QUERY) {
            max_vertex = std::max(max_vertex, o.src);
            gas_ops.push_back(o);
        }
    }
    int vertex_num = max_vertex + 1;
    int total_pma  = (vertex_num + VERTEX_PER_PMA - 1) / VERTEX_PER_PMA;
    int ng = (int)gas_ops.size();
    std::cout << "Base: " << base_ops.size() << "  GAS vertices: " << ng
              << "  PMAs: " << total_pma << std::endl;
    if (ng == 0) { std::cerr << "No GAS vertices in input" << std::endl; return 1; }

    // Build BaseGraph
    std::vector<std::vector<int>> BaseGraph(vertex_num);
    for (auto& o : base_ops) BaseGraph[o.src].push_back(o.dst);

    // ---- Device + xclbin ----
    auto device = xrt::device(0);
    auto uuid   = device.load_xclbin(xclbin_name);

    // ---- Kernels ----
    std::vector<xrt::kernel> mem_kernels, alloc_kernels, insert_kernels, read_worker_kernels;
    for (int i = 1; i <= CHANNEL_NUM; i++) {
        mem_kernels.emplace_back(device, uuid,
            "mem_access_kernel:{mem_access_kernel_" + std::to_string(i) + "}");
        alloc_kernels.emplace_back(device, uuid,
            "alloc_access_kernel:{alloc_access_kernel_" + std::to_string(i) + "}");
        insert_kernels.emplace_back(device, uuid,
            "pma_insert_edge:{pma_insert_edge_" + std::to_string(i) + "}");
        read_worker_kernels.emplace_back(device, uuid,
            "pma_read_worker_kernel:{pma_read_worker_kernel_" + std::to_string(i) + "}");
    }
    auto router_kernel = xrt::kernel(device, uuid, "update_message_router");
    auto check_kernel  = xrt::kernel(device, uuid, "check_kernel");
    auto read_router   = xrt::kernel(device, uuid, "pma_read_router_kernel:{pma_read_router_kernel_1}");
    auto read_merger   = xrt::kernel(device, uuid, "pma_read_merger_kernel:{pma_read_merger_kernel_1}");

    // ---- BOs ----
    std::vector<xrt::bo> bo_hbm, bo_vi, bo_bm;
    BYTE*  hbm_ptr[CHANNEL_NUM];
    VEC_INFO*  vi_ptr[CHANNEL_NUM];
    BYTE*   bm_ptr[CHANNEL_NUM];

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

    const int kBufWords = 65536;
    int coo_src_words = std::max(kBufWords, ng);
    size_t coo_dst_sz = sizeof(int) * std::max<size_t>(kBufWords, (size_t)base_ops.size() / 4 + 65536);
    auto bo_coo_src = xrt::bo(device, coo_src_words * sizeof(int), check_kernel.group_id(0));
    auto bo_coo_dst = xrt::bo(device, coo_dst_sz, check_kernel.group_id(1));
    auto bo_num_edg = xrt::bo(device, sizeof(int),         check_kernel.group_id(2));
    auto coo_src_ptr = bo_coo_src.map<int*>();
    auto coo_dst_ptr = bo_coo_dst.map<int*>();

    // ---- Create runs ----
    std::vector<xrt::run> run_alloc, run_mem, run_insert, run_rd_worker;
    for (int i = 0; i < CHANNEL_NUM; i++) {
        run_alloc.emplace_back(alloc_kernels[i]);
        run_mem.emplace_back(mem_kernels[i]);
        run_insert.emplace_back(insert_kernels[i]);
        run_rd_worker.emplace_back(read_worker_kernels[i]);
    }
    auto run_router    = xrt::run(router_kernel);
    auto run_rd_router = xrt::run(read_router);
    auto run_rd_merger = xrt::run(read_merger);

    // ---- Set kernel args ----
    for (int i = 0; i < CHANNEL_NUM; i++) {
        run_mem[i].set_arg(0, bo_hbm[i]);
        run_mem[i].set_arg(1, bo_vi[i]);
        run_mem[i].set_arg(2, 0);
    }
    for (int i = 0; i < CHANNEL_NUM; i++) {
        run_alloc[i].set_arg(0, bo_bm[i]);
        run_alloc[i].set_arg(1, true);
    }
    for (int i = 0; i < CHANNEL_NUM; i++)
        run_rd_worker[i].set_arg(0, i);

    // ================================================================
    // add_base_graph_to_hbm
    // ================================================================
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

        for (int ch = 0; ch < CHANNEL_NUM; ch++) {
            int blocks = (addr[ch] + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
            int full = blocks / 8, rem = blocks % 8;
            std::memset(bm_ptr[ch], 0xFF, full);
            if (rem) bm_ptr[ch][full] = (BYTE)((1 << rem) - 1);
        }
        for (int i = 0; i < CHANNEL_NUM; i++)
            run_mem[i].set_arg(2, total_pma);
        for (int ch = 0; ch < CHANNEL_NUM; ch++) {
            bo_hbm[ch].sync(XCL_BO_SYNC_BO_TO_DEVICE);
            bo_vi[ch].sync(XCL_BO_SYNC_BO_TO_DEVICE);
            bo_bm[ch].sync(XCL_BO_SYNC_BO_TO_DEVICE);
        }
    }

    // ---- Start keep-alive kernels ----
    for (auto& r : run_insert)    r.start();
    run_router.start();
    for (auto& r : run_rd_worker) r.start();
    run_rd_merger.start();
    run_rd_router.start();
    for (auto& r : run_mem)       r.start();
    for (auto& r : run_alloc)     r.start();

    // ---- Load GAS vertices into coo_src ----
    for (int i = 0; i < ng; i++) coo_src_ptr[i] = gas_ops[i].src;
    bo_coo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    // ---- Run check_kernel mode=2 ----
    auto chk_run = xrt::run(check_kernel);
    chk_run.set_arg(0, bo_coo_src);
    chk_run.set_arg(1, bo_coo_dst);
    chk_run.set_arg(2, bo_num_edg);
    chk_run.set_arg(3, ng);
    chk_run.set_arg(4, 2);   // mode=2: GAS query
    chk_run.set_arg(5, 1);   // last=1

    auto t0 = std::chrono::high_resolution_clock::now();
    chk_run.start();
    chk_run.wait();
    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed_ms = std::chrono::duration<double>(t1 - t0).count() * 1000;

    // ---- Read per-vertex latencies from coo_src ----
    bo_coo_src.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    std::vector<int> lat_cycles(ng);
    for (int i = 0; i < ng; i++) lat_cycles[i] = coo_src_ptr[i];
    std::sort(lat_cycles.begin(), lat_cycles.end());

    int min_c = lat_cycles[0];
    int p50  = lat_cycles[ng * 50 / 100];
    int p90  = lat_cycles[ng * 90 / 100];
    int p99  = lat_cycles[ng * 99 / 100];
    int p999 = lat_cycles[std::min(ng - 1, ng * 999 / 1000)];
    int max_c = lat_cycles[ng - 1];

    auto to_ns = [](int c) { return c * 1000.0 / 150.0; };  // cycles → ns @150MHz
    long long avg_c = std::accumulate(lat_cycles.begin(), lat_cycles.end(), 0LL) / ng;

    // ---- Derive output filename from graph path ----
    std::string graph_basename = graph_path;
    auto slash = graph_basename.find_last_of("/\\");
    if (slash != std::string::npos) graph_basename = graph_basename.substr(slash + 1);
    std::string out_file = graph_basename + "_read_latency.txt";
    std::ofstream lf(out_file);
    if (!lf) { std::cerr << "WARNING: cannot write " << out_file << std::endl; }

    auto emit = [&](const std::string& s) {
        std::cout << s << std::endl;
        if (lf) lf << s << std::endl;
    };

    emit("\n=== READ LATENCY DISTRIBUTION (" + std::to_string(ng) + " samples, "
         + std::to_string(elapsed_ms) + "ms total) ===");
    emit("  min    = " + std::to_string(min_c) + " cyc  (" + std::to_string(to_ns(min_c)) + " ns)");
    emit("  p50    = " + std::to_string(p50)  + " cyc  (" + std::to_string(to_ns(p50))  + " ns)");
    emit("  p90    = " + std::to_string(p90)  + " cyc  (" + std::to_string(to_ns(p90))  + " ns)");
    emit("  p99    = " + std::to_string(p99)  + " cyc  (" + std::to_string(to_ns(p99))  + " ns)");
    emit("  p99.9  = " + std::to_string(p999) + " cyc  (" + std::to_string(to_ns(p999)) + " ns)");
    emit("  max    = " + std::to_string(max_c) + " cyc  (" + std::to_string(to_ns(max_c)) + " ns)");
    emit("  avg    = " + std::to_string(avg_c) + " cyc  (" + std::to_string(to_ns((int)avg_c)) + " ns)");
    emit("=== END_READ_LATENCY ===");
    lf.close();

    return 0;
}
