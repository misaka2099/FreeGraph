#include "data_type.h"
#include "deprecated/xrt.h"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <queue>
#include <string>
#include <vector>
#include <xrt/xrt_device.h>
#include <xrt/xrt_kernel.h>
#include <xrt/xrt_bo.h>
#include <fstream>
#include <thread>

// ---- CPU BFS ----
std::vector<int> cpu_bfs(const std::vector<std::vector<int>> &graph, int start) {
    int n = static_cast<int>(graph.size());
    std::vector<int> levels(n, 0);
    if (start < 0 || start >= n) return levels;
    std::queue<int> q;
    levels[start] = 1;
    q.push(start);
    while (!q.empty()) {
        int v = q.front(); q.pop();
        for (int neighbor : graph[v]) {
            if (levels[neighbor] == 0) {
                levels[neighbor] = levels[v] + 1;
                q.push(neighbor);
            }
        }
    }
    return levels;
}

int main(int argc, char** argv)
{
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <xclbin> <graph> <bfs_start> [max_rounds=100]" << std::endl;
        return 1;
    }

    std::string xclbin_name = argv[1];
    std::string graph_path = argv[2];
    int bfs_start = std::stoi(argv[3]);
    int max_rounds = (argc >= 5) ? std::stoi(argv[4]) : 100;

    auto start_init = std::chrono::high_resolution_clock::now();

    // ---- Load graph file ----
    std::fstream graph_file(graph_path);
    if (!graph_file.is_open()) {
        std::cerr << "Failed to open graph file: " << graph_path << std::endl;
        return 1;
    }

    struct Operation { int src, dst; long long timestamp; int opcode; };
    std::vector<Operation> all_ops;
    int a = -1, b = -1, op = PMA_UPDATE_INSERT_EDGE;
    long long ts = 0;
    while (graph_file >> a >> b >> ts >> op)
        all_ops.push_back({a, b, ts, op});
    graph_file.close();

    // Separate base graph (ts == -1) from updates (ts >= 0)
    std::vector<Operation> base_ops, update_ops;
    for (const auto& opr : all_ops) {
        if (opr.timestamp == -1) base_ops.push_back(opr);
        else                      update_ops.push_back(opr);
    }
    std::cout << "Base graph edges: " << base_ops.size()
              << ", Update edges: " << update_ops.size() << std::endl;

    // Compute max edge batch size
    size_t max_edge_batch = 0, cur_batch = 0;
    long long cur_ts = -1;
    bool first = true;
    for (const auto &opr : update_ops) {
        if (first || opr.timestamp != cur_ts) {
            max_edge_batch = std::max(max_edge_batch, cur_batch);
            cur_batch = 0;
            cur_ts = opr.timestamp;
            first = false;
        }
        cur_batch++;
    }
    max_edge_batch = std::max(max_edge_batch, cur_batch);
    std::cout << "Max edge batch: " << max_edge_batch << std::endl;

    // ---- Build CPU graph from base ops ----
    std::vector<std::vector<int>> cpu_graph;
    auto ensure_v = [&cpu_graph](int v) {
        if (v >= static_cast<int>(cpu_graph.size())) cpu_graph.resize(v + 1);
    };
    for (const auto& opr : base_ops) {
        if (opr.opcode == PMA_UPDATE_INSERT_EDGE) {
            ensure_v(opr.src); ensure_v(opr.dst);
            cpu_graph[opr.src].push_back(opr.dst);
        }
    }
    // Also ensure vertices from update ops
    for (const auto& opr : update_ops) {
        ensure_v(std::max(opr.src, opr.dst));
    }
    for (auto& nbrs : cpu_graph) {
        std::sort(nbrs.begin(), nbrs.end());
        nbrs.erase(std::unique(nbrs.begin(), nbrs.end()), nbrs.end());
    }
    int num_vertices = static_cast<int>(cpu_graph.size());
    std::cout << "Graph vertices: " << num_vertices << std::endl;

    auto end_init = std::chrono::high_resolution_clock::now();
    std::cout << "Init time: " << std::chrono::duration<double>(end_init - start_init).count() << " s" << std::endl;

    // ---- Load xclbin ----
    auto xclbin = xrt::xclbin(xclbin_name);
    auto device = xrt::device(1);
    auto uuid = device.load_xclbin(xclbin);
    std::cout << "device: " << device.get_info<xrt::info::device::name>() << "\n";

    // ---- Get kernels ----
    auto try_add = [&](std::vector<xrt::kernel> &kernels, const std::string &name) -> bool {
        try { kernels.emplace_back(device, uuid, name); return true; }
        catch (const std::runtime_error &e) {
            if (std::string(e.what()).find("No compute units matching") != std::string::npos) return false;
            throw;
        }
    };

    std::vector<xrt::kernel> alloc_kerns, mem_kerns, insert_edge_kerns, read_worker_kerns;
    for (int i = 1; i <= CHANNEL_NUM; ++i) {
        try_add(alloc_kerns,     "alloc_access_kernel:{alloc_access_kernel_"       + std::to_string(i) + "}");
        try_add(mem_kerns,       "mem_access_kernel:{mem_access_kernel_"           + std::to_string(i) + "}");
        read_worker_kerns.emplace_back(device, uuid, "pma_read_worker_kernel:{pma_read_worker_kernel_" + std::to_string(i) + "}");
        try_add(insert_edge_kerns, "pma_insert_edge:{pma_insert_edge_"             + std::to_string(i) + "}");
    }
    auto read_router_kern   = xrt::kernel(device, uuid, "pma_read_router_kernel:{pma_read_router_kernel_1}");
    auto read_merger_kern   = xrt::kernel(device, uuid, "pma_read_merger_kernel:{pma_read_merger_kernel_1}");
    auto gen_kern           = xrt::kernel(device, uuid, "update_message_generator");
    auto router_kern        = xrt::kernel(device, uuid, "update_message_router");
    auto bfs_kern           = xrt::kernel(device, uuid, "bfs_kernel:{bfs_kernel_1}");
    std::cout << "Kernels loaded." << std::endl;

    // ---- Create runs ----
    std::vector<xrt::run> run_alloc, run_mem, run_insert_edge, run_read_worker;
    for (int i = 0; i < CHANNEL_NUM; ++i) {
        run_alloc.emplace_back(alloc_kerns[i]);
        run_mem.emplace_back(mem_kerns[i]);
        run_insert_edge.emplace_back(insert_edge_kerns[i]);
        run_read_worker.emplace_back(read_worker_kerns[i]);
    }
    auto run_read_router = xrt::run(read_router_kern);
    auto run_read_merger = xrt::run(read_merger_kern);
    auto run_router      = xrt::run(router_kern);

    // ---- Allocate buffers ----
    auto t_buf = std::chrono::high_resolution_clock::now();

    // HBM + vec_info per channel
    std::cout << "DBG: Allocating HBM + vec_info (" << CHANNEL_NUM << " channels)..." << std::endl;
    std::cout << "DBG:   HBM size per channel: " << (sizeof(BYTE) * (size_t)CHANNEL_CAPACITY) << " bytes ("
              << (CHANNEL_CAPACITY / (1024*1024)) << " MB)" << std::endl;
    std::cout << "DBG:   vec_info size per channel: " << (sizeof(VEC_INFO) * MAX_PMA_NUM) << " bytes ("
              << (sizeof(VEC_INFO) * MAX_PMA_NUM / (1024*1024)) << " MB)" << std::endl;
    std::vector<xrt::bo> bo_hbm, bo_vi;
    std::vector<BYTE*>     hbm_ptrs;
    std::vector<VEC_INFO*> vi_ptrs;
    for (int i = 0; i < CHANNEL_NUM; ++i) {
        std::cout << "DBG:   ch" << i << " bo_hbm group_id=" << mem_kerns[i].group_id(0) << " ..." << std::endl;
        bo_hbm.emplace_back(device, sizeof(BYTE) * (size_t)CHANNEL_CAPACITY, mem_kerns[i].group_id(0));
        std::cout << "DBG:   ch" << i << " bo_hbm done, bo_vi group_id=" << mem_kerns[i].group_id(1) << " ..." << std::endl;
        bo_vi.emplace_back( device, sizeof(VEC_INFO) * MAX_PMA_NUM,       mem_kerns[i].group_id(1));
        std::cout << "DBG:   ch" << i << " bo_vi done, mapping..." << std::endl;
        hbm_ptrs.push_back(bo_hbm.back().map<BYTE*>());
        auto p = bo_vi.back().map<VEC_INFO*>();
        std::fill_n(p, MAX_PMA_NUM, 0);
        vi_ptrs.push_back(p);
    }
    std::cout << "DBG: HBM + vec_info done." << std::endl;

    // Bitmap
    std::cout << "DBG: Allocating bitmap..." << std::endl;
    size_t bm_bytes = sizeof(BYTE) * ((size_t)CHANNEL_CAPACITY * CHANNEL_NUM / INIT_VEC_LEN_IN_BYTE / 8);
    std::cout << "DBG:   bitmap size per channel: " << bm_bytes << " bytes" << std::endl;
    std::vector<xrt::bo> bo_bm;
    std::vector<BYTE*>   bm_ptrs;
    for (int i = 0; i < CHANNEL_NUM; ++i) {
        std::cout << "DBG:   ch" << i << " bo_bm group_id=" << alloc_kerns[i].group_id(0) << " ..." << std::endl;
        bo_bm.emplace_back(device, bm_bytes, alloc_kerns[i].group_id(0));
        std::cout << "DBG:   ch" << i << " bo_bm done, mapping..." << std::endl;
        auto p = bo_bm.back().map<BYTE*>();
        std::fill_n(p, bm_bytes, 0);
        bm_ptrs.push_back(p);
    }
    std::cout << "DBG: Bitmap done." << std::endl;

    // Edge update buffers (for generator)
    size_t edge_buf_len = std::max<size_t>(1, std::min(max_edge_batch, (size_t)1000000));
    std::cout << "DBG: Allocating edge buffers, len=" << edge_buf_len
              << " (" << (sizeof(int) * edge_buf_len) << " bytes each)..." << std::endl;
    std::cout << "DBG:   bo_src group_id=" << gen_kern.group_id(1) << " ..." << std::endl;
    auto bo_src    = xrt::bo(device, sizeof(int) * edge_buf_len, gen_kern.group_id(1));
    std::cout << "DBG:   bo_src done. bo_dst group_id=" << gen_kern.group_id(2) << " ..." << std::endl;
    auto bo_dst    = xrt::bo(device, sizeof(int) * edge_buf_len, gen_kern.group_id(2));
    std::cout << "DBG:   bo_dst done. bo_opcode group_id=" << gen_kern.group_id(3) << " ..." << std::endl;
    auto bo_opcode = xrt::bo(device, sizeof(int) * edge_buf_len, gen_kern.group_id(3));
    std::cout << "DBG:   edge buffers done, mapping..." << std::endl;
    int* src_ptr    = bo_src.map<int*>();
    int* dst_ptr    = bo_dst.map<int*>();
    int* opcode_ptr = bo_opcode.map<int*>();

    // BFS number buffers
    // bfs_kernel args: 0=bfs_start(s_axilite) 1=bfs_number_in(m_axi) 2=bfs_number_out(m_axi)
    //                  3=graph_size(s_axilite) 4=max_rounds(s_axilite)
    std::cout << "DBG: Allocating BFS number buffers, vertices=" << num_vertices
              << " (" << (sizeof(int) * num_vertices) << " bytes each)..." << std::endl;
    std::cout << "DBG:   bo_bfs_in group_id=" << bfs_kern.group_id(1) << " (arg 1) ..." << std::endl;
    auto bo_bfs_in  = xrt::bo(device, sizeof(int) * num_vertices, bfs_kern.group_id(1));
    std::cout << "DBG:   bo_bfs_in done. bo_bfs_out group_id=" << bfs_kern.group_id(2) << " (arg 2) ..." << std::endl;
    auto bo_bfs_out = xrt::bo(device, sizeof(int) * num_vertices, bfs_kern.group_id(2));
    std::cout << "DBG:   BFS buffers done, mapping..." << std::endl;
    int* bfs_in  = bo_bfs_in.map<int*>();
    int* bfs_out = bo_bfs_out.map<int*>();

    std::cout << "DBG: All buffers allocated." << std::endl;
    std::cout << "Buffer alloc time: "
              << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t_buf).count()
              << " s" << std::endl;

    // ---- Set kernel args ----
    for (int i = 0; i < CHANNEL_NUM; ++i) {
        run_mem[i].set_arg(0, bo_hbm[i]);
        run_mem[i].set_arg(1, bo_vi[i]);
        run_mem[i].set_arg(2, 0);
        run_alloc[i].set_arg(0, bo_bm[i]);
        run_alloc[i].set_arg(1, true);
        run_read_worker[i].set_arg(0, i);
    }

    // ---- Load base graph into HBM ----
    auto load_base_graph = [&]() -> bool {
        auto t0 = std::chrono::high_resolution_clock::now();
        const int total_pma = (num_vertices + VERTEX_PER_PMA - 1) / VERTEX_PER_PMA;
        if (total_pma > MAX_PMA_NUM) { std::cerr << "PMA overflow" << std::endl; return false; }

        std::vector<std::vector<int>> adj(num_vertices);
        for (const auto& opr : base_ops)
            if (opr.opcode == PMA_UPDATE_INSERT_EDGE && opr.src < num_vertices)
                adj[opr.src].push_back(opr.dst);
        for (auto& nbrs : adj) std::sort(nbrs.begin(), nbrs.end());

        int offsets[CHANNEL_NUM] = {0};
        for (int pma_idx = 0; pma_idx < total_pma; ++pma_idx) {
            int ch = pma_idx % CHANNEL_NUM;
            PMA_HEADER hdr = {0};
            std::vector<int> data;

            std::vector<int> empty_nbrs;
            for (int i = 0; i < VERTEX_PER_PMA; i++) {
                int src = i + pma_idx * VERTEX_PER_PMA;
                auto& nbrs = (src < num_vertices) ? adj[src] : empty_nbrs;

                hdr.edge_count[i] = nbrs.size();
                hdr.attr[i] = 0;

                int last_range = (i == 0) ? 0 : hdr.vertex_range[i-1];
                int total_segs = 1;
                if (!nbrs.empty()) {
                    int segs = 1;
                    while (segs * 47 < static_cast<int>(nbrs.size())) segs *= 4;
                    total_segs = segs;
                }
                hdr.vertex_range[i] = last_range + total_segs;

                int base_n = 0, extra_n = 0;
                if (total_segs > 0) {
                    base_n = nbrs.size() / total_segs;
                    extra_n = nbrs.size() % total_segs;
                }
                int ni = 0;
                for (int s = 0; s < total_segs; ++s) {
                    int vn = base_n + (s < extra_n ? 1 : 0);
                    for (int k = 0; k < vn; ++k) data.push_back(nbrs[ni++]);
                    for (int k = vn; k < EDGE_SLOTS_PER_SEG; ++k) data.push_back(UNUSED_ICON);
                    data.push_back(vn);
                }
            }

            int total_len = sizeof(PMA_HEADER) + data.size() * sizeof(int);
            int aligned = total_len;
            if (aligned % INIT_VEC_LEN_IN_BYTE) aligned += INIT_VEC_LEN_IN_BYTE - (aligned % INIT_VEC_LEN_IN_BYTE);
            if (offsets[ch] + aligned > CHANNEL_CAPACITY) { std::cerr << "HBM overflow ch" << ch << std::endl; return false; }

            memcpy(hbm_ptrs[ch] + offsets[ch], &hdr, sizeof(PMA_HEADER));
            memcpy(hbm_ptrs[ch] + offsets[ch] + sizeof(PMA_HEADER), data.data(), data.size() * sizeof(int));
            vi_ptrs[ch][pma_idx] = (ap_uint<64>(total_len) << 32) | offsets[ch];
            offsets[ch] += aligned;
        }

        for (int ch = 0; ch < CHANNEL_NUM; ++ch) {
            int blocks = (offsets[ch] + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
            memset(bm_ptrs[ch], 0xFF, blocks / 8);
            if (blocks % 8) bm_ptrs[ch][blocks / 8] = (BYTE)((1 << (blocks % 8)) - 1);
            run_mem[ch].set_arg(2, total_pma);
        }

        for (auto &b : bo_hbm) b.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        for (auto &b : bo_vi)  b.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        for (auto &b : bo_bm)  b.sync(XCL_BO_SYNC_BO_TO_DEVICE);

        std::cout << "Base graph: " << total_pma << " PMAs, "
                  << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count()
                  << " s" << std::endl;
        return true;
    };

    if (!load_base_graph()) { std::cerr << "Failed to load base graph." << std::endl; return 1; }

    // ---- Start all long-running kernels (insert pipeline, read pipeline, mem, alloc) ----
    for (auto &r : run_insert_edge)  { r.start(); }
    run_router.start();
    for (auto &r : run_mem)          { r.start(); }
    for (auto &r : run_alloc)        { r.start(); }
    for (auto &r : run_read_worker)  { r.start(); }
    run_read_merger.start();
    run_read_router.start();

    std::cout << "Service kernels started." << std::endl;

    // ==================== Helper lambdas ====================

    // Run a single BFS on FPGA, verify against CPU golden
    auto run_bfs_and_verify = [&](const std::string &label,
                                  const std::vector<std::vector<int>> &cpu_graph_state,
                                  bool is_last) -> double {
        // Compute CPU golden
        auto t0 = std::chrono::high_resolution_clock::now();
        auto golden = cpu_bfs(cpu_graph_state, bfs_start);
        double cpu_t = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();

        // Init bfs_number_in
        std::fill_n(bfs_in, num_vertices, 0);
        bfs_in[bfs_start] = 1;
        bo_bfs_in.sync(XCL_BO_SYNC_BO_TO_DEVICE);

        // Run BFS kernel
        auto run_bfs = xrt::run(bfs_kern);
        run_bfs.set_arg(0, bfs_start);
        run_bfs.set_arg(1, bo_bfs_in);
        run_bfs.set_arg(2, bo_bfs_out);
        run_bfs.set_arg(3, num_vertices);
        run_bfs.set_arg(4, max_rounds);
        run_bfs.set_arg(5, static_cast<int>(is_last));  // last flag: send PMA_STOP only on final round

        auto t1 = std::chrono::high_resolution_clock::now();
        run_bfs.start();
        run_bfs.wait();
        double fpga_t = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t1).count();

        bo_bfs_out.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

        // Compare
        int mismatches = 0, fpga_reached = 0, cpu_reached = 0;
        for (int i = 0; i < num_vertices; ++i) {
            if (bfs_out[i] > 0) fpga_reached++;
            if (golden[i] > 0)  cpu_reached++;
            if (bfs_out[i] != golden[i]) {
                if (mismatches < 10)
                    std::cout << "  MISMATCH v" << i << ": FPGA=" << bfs_out[i] << " CPU=" << golden[i] << std::endl;
                mismatches++;
            }
        }
        if (mismatches > 10) std::cout << "  ... +" << (mismatches - 10) << " more mismatches" << std::endl;

        std::cout << "[" << label << "] FPGA=" << fpga_t << "s CPU=" << cpu_t << "s"
                  << " reached=" << fpga_reached << "/" << cpu_reached
                  << " mismatches=" << mismatches
                  << (mismatches == 0 ? " MATCH" : " MISMATCH") << std::endl;

        return fpga_t;
    };

    // Load edge batch into generator buffers
    auto load_batch = [&](const std::vector<Operation> &batch) -> int {
        for (size_t i = 0; i < batch.size(); ++i) {
            src_ptr[i]    = batch[i].src;
            dst_ptr[i]    = batch[i].dst;
            opcode_ptr[i] = batch[i].opcode;
        }
        bo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        bo_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        bo_opcode.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        return static_cast<int>(batch.size());
    };

    // Run update batch through generator
    auto run_update = [&](int edge_count, bool last) -> double {
        auto run_gen = xrt::run(gen_kern);
        run_gen.set_arg(0, edge_count);
        run_gen.set_arg(1, bo_src);
        run_gen.set_arg(2, bo_dst);
        run_gen.set_arg(3, bo_opcode);
        run_gen.set_arg(4, static_cast<int>(last));
        auto t0 = std::chrono::high_resolution_clock::now();
        run_gen.start();
        run_gen.wait();
        return std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();
    };

    // Apply a batch of operations to CPU graph
    auto apply_to_cpu = [&](std::vector<std::vector<int>> &g, const std::vector<Operation> &batch) {
        for (const auto &opr : batch) {
            switch (opr.opcode) {
                case PMA_UPDATE_INSERT_EDGE:
                    if (opr.src >= 0 && opr.src < static_cast<int>(g.size()))
                        g[opr.src].push_back(opr.dst);
                    break;
                case PMA_UPDATE_DELETE_EDGE:
                    if (opr.src >= 0 && opr.src < static_cast<int>(g.size())) {
                        auto &nbrs = g[opr.src];
                        nbrs.erase(std::remove(nbrs.begin(), nbrs.end(), opr.dst), nbrs.end());
                    }
                    break;
                case PMA_UPDATE_DELETE_VERTEX:
                    if (opr.src >= 0 && opr.src < static_cast<int>(g.size()))
                        g[opr.src].clear();
                    break;
                default: break;
            }
        }
        for (auto &nbrs : g) {
            std::sort(nbrs.begin(), nbrs.end());
            nbrs.erase(std::unique(nbrs.begin(), nbrs.end()), nbrs.end());
        }
    };

    // ==================== Per-timestamp BFS loop ====================

    std::vector<std::vector<int>> runtime_graph = cpu_graph;
    double total_bfs_time = 0;

    std::cout << "\n========== BFS at each timestamp ==========\n" << std::endl;

    // Time 0: base graph (no updates applied yet)
    std::cout << "--- Timestamp: base (ts=-1) ---" << std::endl;
    total_bfs_time += run_bfs_and_verify("ts=base", runtime_graph, update_ops.empty());

    // Process update batches per timestamp
    for (size_t idx = 0; idx < update_ops.size(); ) {
        long long ts_val = update_ops[idx].timestamp;
        std::vector<Operation> batch;
        while (idx < update_ops.size() && update_ops[idx].timestamp == ts_val)
            batch.push_back(update_ops[idx++]);

        bool is_last = (idx == update_ops.size());

        std::cout << "\n--- Timestamp: " << ts_val << " (" << batch.size() << " ops) ---" << std::endl;

        // Apply updates on CPU
        apply_to_cpu(runtime_graph, batch);

        // Send updates to FPGA
        int n = load_batch(batch);
        double upd_t = run_update(n, is_last);
        std::cout << "  Update time: " << upd_t << " s" << std::endl;

        // Small delay to let HBM writes settle
        std::this_thread::sleep_for(std::chrono::microseconds(100));

        // Run BFS (read pipeline stays alive unless is_last)
        total_bfs_time += run_bfs_and_verify("ts=" + std::to_string(ts_val), runtime_graph, is_last);
    }

    // ==================== Summary ====================
    std::cout << "\n========== Summary ==========" << std::endl;
    std::cout << "Total BFS kernel time: " << total_bfs_time << " s" << std::endl;

    // ---- Wait for kernels ----
    std::cout << "Waiting for kernels to finish..." << std::endl;
    run_router.wait();
    for (auto &r : run_insert_edge)  { r.wait(); }
    for (auto &r : run_alloc)        { r.wait(); }
    for (auto &r : run_mem)          { r.wait(); }
    run_read_router.wait();
    for (auto &r : run_read_worker)  { r.wait(); }
    run_read_merger.wait();

    std::cout << "Done." << std::endl;
    return 0;
}
