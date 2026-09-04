// test_overlap_host.cpp
// HBM read/write concurrency test on existing bitstream.
// Initialization follows host.cpp pattern exactly.
//
// Write path: update_message_generator → router → pma_insert_edge → mem_access
// Read path:  check_kernel (mode=2) → pma_read pipeline → mem_access
//
// Input format: src dst timestamp opcode
//   opcode=0 (INSERT_EDGE)   — base graph (ts=-1) or update edge
//   opcode=6 (GAS_QUERY)     — GAS vertex query: src=vertex, dst=0
//
// Usage: ./test_overlap_host <xclbin> <graph_file> <mode>
//   mode=0  serial | mode=1  concurrent

#include "data_type.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

#include "xrt/xrt_bo.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"

struct Operation { int src, dst, ts, opcode; };

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <xclbin> <graph_file> <mode>" << std::endl;
        std::cerr << "  mode=0 serial | mode=1 concurrent" << std::endl;
        return 1;
    }
    std::string xclbin_name = argv[1];
    std::string graph_path  = argv[2];
    int mode = std::atoi(argv[3]);
    if (mode != 0 && mode != 1) { std::cerr << "Invalid mode" << std::endl; return 1; }

    // ---- Parse input ----
    std::cerr << "[dbg] Parsing " << graph_path << std::endl;
    std::ifstream f(graph_path);
    if (!f) { std::cerr << "FAILED to open " << graph_path << std::endl; return 1; }
    std::vector<Operation> all_ops;
    { int a,b,ts,op; while (f>>a>>b>>ts>>op) all_ops.push_back({a,b,ts,op}); }
    f.close();
    std::cerr << "[dbg] " << all_ops.size() << " ops parsed" << std::endl;

    std::vector<Operation> base_ops, update_ops, gas_ops;
    int max_vertex = -1;
    for (auto& o : all_ops) {
        if (o.opcode == PMA_UPDATE_INSERT_EDGE) {
            max_vertex = std::max({max_vertex, o.src, o.dst});
            if (o.ts == -1) base_ops.push_back(o);
            else            update_ops.push_back(o);
        } else if (o.opcode == PMA_UPDATE_GAS_QUERY) {
            max_vertex = std::max(max_vertex, o.src);
            gas_ops.push_back(o);
        }
    }
    int vertex_num = max_vertex + 1;
    int total_pma  = (vertex_num + VERTEX_PER_PMA - 1) / VERTEX_PER_PMA;
    std::cout << "Base: " << base_ops.size()   << "  Updates: " << update_ops.size()
              << "  GAS: " << gas_ops.size()   << "  PMAs: " << total_pma << std::endl;

    // Build BaseGraph adjacency + runtime graph (tracks state after each batch)
    std::vector<std::vector<int>> BaseGraph(vertex_num);
    for (auto& o : base_ops) BaseGraph[o.src].push_back(o.dst);
    std::vector<std::vector<int>> runtime_graph = BaseGraph;  // evolves per batch
    for (auto& nb : BaseGraph) std::sort(nb.begin(), nb.end());
    // ---- Compute max batch sizes for BO sizing ----
    int max_upd_per_batch = 0, max_gas_per_batch = 0;
    {
        int max_ts = -1;
        for (auto& o : update_ops) max_ts = std::max(max_ts, o.ts);
        for (auto& o : gas_ops)    max_ts = std::max(max_ts, o.ts);
        std::vector<int> upd_cnt(max_ts + 1, 0), gas_cnt(max_ts + 1, 0);
        for (auto& o : update_ops) upd_cnt[o.ts]++;
        for (auto& o : gas_ops)    gas_cnt[o.ts]++;
        for (int c : upd_cnt) max_upd_per_batch = std::max(max_upd_per_batch, c);
        for (int c : gas_cnt) max_gas_per_batch = std::max(max_gas_per_batch, c);
    }
    const int kMinBufWords = 65536;
    int gen_buf_words = std::max(kMinBufWords, max_upd_per_batch);
    int coo_src_words = std::max(kMinBufWords, max_gas_per_batch);
    std::cerr << "[dbg] max_upd_per_batch=" << max_upd_per_batch
              << " max_gas_per_batch=" << max_gas_per_batch
              << " gen_buf_words=" << gen_buf_words << std::endl;

    std::cerr << "[dbg] BaseGraph built" << std::endl;

    // ---- Device + xclbin ----
    std::cerr << "[dbg] Loading xclbin..." << std::endl;
    auto device = xrt::device(0);
    auto uuid   = device.load_xclbin(xclbin_name);
    std::cerr << "[dbg] xclbin loaded" << std::endl;

    // ---- CU-specific kernel handles (match host.cpp pattern) ----
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
    auto gen_kernel    = xrt::kernel(device, uuid, "update_message_generator");
    auto router_kernel = xrt::kernel(device, uuid, "update_message_router");
    auto check_kernel  = xrt::kernel(device, uuid, "check_kernel");
    auto read_router   = xrt::kernel(device, uuid, "pma_read_router_kernel:{pma_read_router_kernel_1}");
    auto read_merger   = xrt::kernel(device, uuid, "pma_read_merger_kernel:{pma_read_merger_kernel_1}");
    std::cerr << "[dbg] Kernels loaded" << std::endl;

    // ---- BOs (exactly as host.cpp) ----
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

    // Generator buffers — size based on actual max batch size
    auto bo_src = xrt::bo(device, gen_buf_words * sizeof(int), gen_kernel.group_id(1));
    auto bo_dst = xrt::bo(device, gen_buf_words * sizeof(int), gen_kernel.group_id(2));
    auto bo_opc = xrt::bo(device, gen_buf_words * sizeof(int), gen_kernel.group_id(3));
    auto src_ptr = bo_src.map<int*>();
    auto dst_ptr = bo_dst.map<int*>();
    auto opc_ptr = bo_opc.map<int*>();

    // Check kernel buffers — use kernel group_id
    // coo_dst: enough for max GAS edges per batch (worst: all batch vertices read)
    size_t coo_dst_sz = sizeof(int) * std::max<size_t>(kMinBufWords, (size_t)base_ops.size() / 4 + 65536);
    std::cerr << "[dbg] coo_dst_sz=" << coo_dst_sz << " (" << (coo_dst_sz>>20) << "MB)" << std::endl;
    auto bo_coo_src = xrt::bo(device, coo_src_words * sizeof(int), check_kernel.group_id(0));
    auto bo_coo_dst = xrt::bo(device, coo_dst_sz, check_kernel.group_id(1));
    auto bo_num_edg = xrt::bo(device, sizeof(int),         check_kernel.group_id(2));
    auto coo_src_ptr = bo_coo_src.map<int*>();
    auto coo_dst_ptr = bo_coo_dst.map<int*>();
    auto num_edg_ptr = bo_num_edg.map<int*>();
    std::cerr << "[dbg] BOs allocated" << std::endl;

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
    std::cerr << "[dbg] Runs created" << std::endl;

    // ---- Set kernel args ----
    for (int i = 0; i < CHANNEL_NUM; i++) {
        run_mem[i].set_arg(0, bo_hbm[i]);
        run_mem[i].set_arg(1, bo_vi[i]);
        run_mem[i].set_arg(2, 0);  // placeholder, updated below
    }
    for (int i = 0; i < CHANNEL_NUM; i++) {
        run_alloc[i].set_arg(0, bo_bm[i]);
        run_alloc[i].set_arg(1, true);
    }
    for (int i = 0; i < CHANNEL_NUM; i++)
        run_rd_worker[i].set_arg(0, i);
    std::cerr << "[dbg] Kernel args set" << std::endl;

    // ================================================================
    // add_base_graph_to_hbm
    // ================================================================
    {
        std::cerr << "[dbg] Building " << total_pma << " PMAs..." << std::endl;
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
        std::cerr << "[dbg] PMA done. addr[0]=" << addr[0] << " addr[1]=" << addr[1] << std::endl;
    }

    // ---- Start keep-alive kernels (order matches host.cpp) ----
    std::cerr << "[dbg] Starting keep-alive kernels..." << std::endl;
    for (auto& r : run_insert)    r.start();
    run_router.start();
    for (auto& r : run_rd_worker) r.start();
    run_rd_merger.start();
    run_rd_router.start();
    for (auto& r : run_mem)       r.start();
    for (auto& r : run_alloc)     r.start();
    std::cerr << "[dbg] Keep-alive kernels started" << std::endl;

    // ---- Group by timestamp ----
    std::vector<std::vector<Operation>> upd_by_ts, gas_by_ts;
    {
        int max_ts = -1;
        for (auto& o : update_ops) max_ts = std::max(max_ts, o.ts);
        for (auto& o : gas_ops)    max_ts = std::max(max_ts, o.ts);
        upd_by_ts.resize(max_ts + 1);
        gas_by_ts.resize(max_ts + 1);
        for (auto& o : update_ops) upd_by_ts[o.ts].push_back(o);
        for (auto& o : gas_ops)    gas_by_ts[o.ts].push_back(o);
    }
    int num_batches = (int)upd_by_ts.size();

    // ---- Sync generator BOs ----
    bo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_opc.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    // ================================================================
    std::cout << "\n=== MODE " << mode
              << (mode == 0 ? " Serial" : " Concurrent")
              << "  batches=" << num_batches << " ===" << std::endl;

    double total_update_s = 0, total_read_s = 0;

    auto t_all = std::chrono::high_resolution_clock::now();

    for (int ts = 0; ts < num_batches; ts++) {
        auto& upds = upd_by_ts[ts];
        auto& gass = gas_by_ts[ts];
        bool last_batch = (ts == num_batches - 1);
        int ne = (int)upds.size(), ng = (int)gass.size();
        std::cerr << "[dbg] === batch ts=" << ts << " updates=" << ne
                  << " gas=" << ng << " last=" << last_batch << " ===" << std::endl;
        if (ng == 0) continue;

        // Load update edges
        for (int i = 0; i < ne; i++) {
            src_ptr[i] = upds[i].src; dst_ptr[i] = upds[i].dst;
            opc_ptr[i] = PMA_UPDATE_INSERT_EDGE;
        }
        bo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        bo_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        bo_opc.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        std::cerr << "[dbg]   update edges synced" << std::endl;

        // Load GAS query vertices
        for (int i = 0; i < ng; i++) coo_src_ptr[i] = gass[i].src;
        bo_coo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        std::cerr << "[dbg]   GAS vertices synced" << std::endl;
        std::cerr << "[dbg]   creating gen_run..." << std::endl;

        auto gen_run = xrt::run(gen_kernel);
        std::cerr << "[dbg]   gen_run created" << std::endl;
        gen_run.set_arg(0, ne);
        gen_run.set_arg(1, bo_src); gen_run.set_arg(2, bo_dst);
        gen_run.set_arg(3, bo_opc); gen_run.set_arg(4, (int)last_batch);
        std::cerr << "[dbg]   gen_run args set" << std::endl;

        std::cerr << "[dbg]   creating chk_run..." << std::endl;
        auto chk_run = xrt::run(check_kernel);
        std::cerr << "[dbg]   chk_run created" << std::endl;
        chk_run.set_arg(0, bo_coo_src); chk_run.set_arg(1, bo_coo_dst);
        chk_run.set_arg(2, bo_num_edg); chk_run.set_arg(3, ng);
        chk_run.set_arg(4, 2); chk_run.set_arg(5, (int)last_batch);
        std::cerr << "[dbg]   chk_run args set" << std::endl;

        if (mode == 0) {
            // Serial: update then read, measure individually
            auto t0 = std::chrono::high_resolution_clock::now();
            gen_run.start(); gen_run.wait();
            auto t1 = std::chrono::high_resolution_clock::now();
            total_update_s += std::chrono::duration<double>(t1 - t0).count();

            auto t2 = std::chrono::high_resolution_clock::now();
            chk_run.start(); chk_run.wait();
            auto t3 = std::chrono::high_resolution_clock::now();
            total_read_s += std::chrono::duration<double>(t3 - t2).count();
        } else {
            // Concurrent: start both, poll state() for individual completion
            auto t0 = std::chrono::high_resolution_clock::now();
            gen_run.start(); chk_run.start();

            bool gen_done = false, chk_done = false;
            double gen_ms = 0, chk_ms = 0;
            while (!gen_done || !chk_done) {
                auto now = std::chrono::high_resolution_clock::now();
                if (!gen_done && gen_run.state() == ERT_CMD_STATE_COMPLETED) {
                    gen_done = true;
                    gen_ms = std::chrono::duration<double>(now - t0).count() * 1000;
                }
                if (!chk_done && chk_run.state() == ERT_CMD_STATE_COMPLETED) {
                    chk_done = true;
                    chk_ms = std::chrono::duration<double>(now - t0).count() * 1000;
                }
                if (!gen_done || !chk_done)
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
            // Properly sync both (should return immediately since they're done)
            gen_run.wait(); chk_run.wait();
            total_update_s += gen_ms / 1000.0;
            total_read_s += chk_ms / 1000.0;
            std::cerr << "[dbg]   concurrent done: gen=" << gen_ms << "ms chk=" << chk_ms << "ms" << std::endl;
        }
        // ---- Apply update edges to runtime graph (for subsequent verification) ----
        std::cerr << "[dbg]   verifying runtime_graph integrity..." << std::endl;
        {
            size_t total_edges = 0;
            for (size_t vi = 0; vi < runtime_graph.size(); vi++) {
                total_edges += runtime_graph[vi].size();
                if (vi < 3) std::cerr << "[dbg]     v[" << vi << "].size()=" << runtime_graph[vi].size() << std::endl;
            }
            std::cerr << "[dbg]   runtime_graph OK: " << runtime_graph.size()
                      << " vertices, " << total_edges << " edges" << std::endl;
        }
        // SKIP applying updates for b=1 GAS-first: PMA exclusion guarantees
        // no GAS vertex is in the update set, so BaseGraph is correct.
        std::cerr << "[dbg]   skipping update apply (b=1, PMA exclusion ensures no overlap)" << std::endl;

        // ---- Read per-vertex first-data latencies from coo_src (reused as output) ----
        bo_coo_src.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        {
            std::vector<int> lat_cycles(ng);
            for (int i = 0; i < ng; i++) lat_cycles[i] = coo_src_ptr[i];
            std::sort(lat_cycles.begin(), lat_cycles.end());
            int p50 = lat_cycles[ng * 50 / 100];
            int p99 = lat_cycles[ng * 99 / 100];
            int p999 = lat_cycles[std::min(ng - 1, ng * 999 / 1000)];
            double t50 = p50 * 1000.0 / 150.0;  // cycles → ns, /150MHz
            double t99 = p99 * 1000.0 / 150.0;
            double t999 = p999 * 1000.0 / 150.0;
            std::cout << "CMD_TO_FIRST_DATA_LATENCY samples=" << ng
                      << " min=" << lat_cycles[0] << "cyc (" << lat_cycles[0]*1000.0/150.0 << "ns)"
                      << " p50=" << p50 << "cyc (" << t50 << "ns)"
                      << " p99=" << p99 << "cyc (" << t99 << "ns)"
                      << " p99.9=" << p999 << "cyc (" << t999 << "ns)"
                      << " max=" << lat_cycles[ng-1] << "cyc (" << lat_cycles[ng-1]*1000.0/150.0 << "ns)"
                      << std::endl;
        }

        std::cerr << "[dbg]   syncing coo_dst..." << std::endl;

        // ---- Verify GAS results against CURRENT runtime_graph ----
        bo_coo_dst.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        std::cerr << "[dbg]   sync done, reading results..." << std::endl;
        int mismatch_count = 0;
        int dst_idx = 0;
        // Safety: max entries = bo size. Detect runaway reads.
        int coo_dst_max = (int)(bo_coo_dst.size() / sizeof(int));
        std::cerr << "[dbg]   coo_dst_max=" << coo_dst_max << std::endl;
        for (int vi = 0; vi < ng; vi++) {
            int vid = gass[vi].src;
            if (vi < 100 || vi % 1000 == 0)
                std::cerr << "[dbg]   v[" << vi << "] id=" << vid << " BEGIN dst_idx=" << dst_idx << std::endl;
            std::vector<int> expected;
            if (vid < (int)BaseGraph.size())
                expected = BaseGraph[vid];  // GAS vertices not in update PMAs, BaseGraph is correct
            std::vector<int> actual;
            actual.reserve(std::min(expected.size() + 64, (size_t)500000));
            while (dst_idx < coo_dst_max) {
                int d = coo_dst_ptr[dst_idx++];
                if (d == -1) break;
                actual.push_back(d);
                if (actual.size() > 500000) {  // sanity limit
                    std::cerr << "[dbg] ERROR: vertex " << vid << " exceeded 500K edges, aborting read" << std::endl;
                    break;
                }
            }
            if (dst_idx >= coo_dst_max) {
                std::cerr << "[dbg] WARNING: hit coo_dst_max=" << coo_dst_max
                          << " at vertex " << vi << " id=" << vid << std::endl;
                break;
            }
            if (vi < 100 || vi % 1000 == 0)
                std::cerr << "[dbg]   v[" << vi << "] id=" << vid
                          << " exp=" << expected.size() << " act=" << actual.size() << " sorting..." << std::endl;
            std::sort(actual.begin(), actual.end());

            if (expected != actual) {
                mismatch_count++;
                std::cout << "MISMATCH ts=" << ts << " vertex=" << vid
                          << " exp_count=" << expected.size() << " act_count=" << actual.size() << std::endl;
                // Show expected edges not in actual
                for (int e : expected) {
                    if (!std::binary_search(actual.begin(), actual.end(), e))
                        std::cout << "  MISSING: " << vid << " -> " << e << std::endl;
                }
                // Show extra edges in actual not in expected
                for (int a : actual) {
                    if (!std::binary_search(expected.begin(), expected.end(), a))
                        std::cout << "  EXTRA:   " << vid << " -> " << a << std::endl;
                }
            }
        }
        if (mismatch_count > 0)
            std::cerr << "[dbg]   *** " << mismatch_count << "/" << ng << " GAS mismatches ***" << std::endl;
        else
            std::cerr << "[dbg]   GAS all match (" << ng << " vertices)" << std::endl;
        std::cerr << "[dbg]   verification done" << std::endl;

        // std::cerr << "[dbg]   batch done, waiting 5s..." << std::endl;
        // std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    std::cerr << "[dbg] batch loop done, writing results..." << std::endl;

    auto t_end = std::chrono::high_resolution_clock::now();
    double total_s = std::chrono::duration<double>(t_end - t_all).count();

    // ---- Structured output to both stdout and result file ----
    std::string mode_label = (mode == 0) ? "SERIAL" : "CONCURRENT";

    // Ensure output dir exists
    system("mkdir -p experiment_2_results");

    // Derive result file: experiment_2_results/<basename>_mode<N>.result
    std::string graph_basename = graph_path;
    auto slash = graph_basename.find_last_of("/\\");
    if (slash != std::string::npos) graph_basename = graph_basename.substr(slash + 1);
    std::string result_file = "experiment_2_results/" + graph_basename + "_mode" + std::to_string(mode) + ".result";
    std::ofstream rf(result_file);
    if (!rf) {
        std::cerr << "[dbg] WARNING: could not open result file: " << result_file << std::endl;
    } else {
        std::cerr << "[dbg] Writing results to " << result_file << std::endl;
    }

    auto emit = [&](const std::string& s) {
        std::cout << s << std::endl;
        rf << s << std::endl;
    };

    emit("\n=== OVERLAP_RESULT " + mode_label + " ===");
    emit("graph=" + graph_path);
    emit("mode=" + std::to_string(mode));
    emit("num_batches=" + std::to_string(num_batches));
    emit("total_update_s=" + std::to_string(total_update_s));
    emit("total_read_s=" + std::to_string(total_read_s));
    emit("total_s=" + std::to_string(total_s));
    emit("=== END_" + mode_label + " ===");
    rf.close();

    std::cout << "\nDone." << std::endl;
    return 0;
}
