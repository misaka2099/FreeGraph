#include "data_type.h"
#include "deprecated/xrt.h"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <xrt/xrt_device.h>
#include <xrt/xrt_kernel.h>
#include <xrt/xrt_bo.h>
#include <unordered_set>
#include <fstream>
#include <random>
bool final_match = true;
const bool kBaseGraphReadOnlyTest = false;
const bool kVertexOnlyTest = false;
const bool kShuffleSameTimestampOps = false; // Set to true to enable shuffling of operations with the same timestamp
const bool kMismatchfileOutput = false; // Set to true to output mismatch details to files
void print_pma(
    int pma_idx,
    VEC_INFO* pos_info,
    BYTE* storage
)
{
    VEC_INFO vec_info = pos_info[pma_idx];
    int start = vec_info.range(31,0);
    int len =  vec_info.range(63,32);
    std::cout << "PMA " << pma_idx << " ADDR START FROM " << start << " LEN " << len << std::endl; 
    PMA_HEADER header;
    memcpy(&header, storage + start, sizeof(PMA_HEADER));
    int *pma_data = (int*)(storage + sizeof(PMA_HEADER) + start);
    for (int i = 0 ; i < VERTEX_PER_PMA ; i++)
    {
        int src = pma_idx * 16  + i;
        int edge_count = header.edge_count[i];
        int seg_start = i == 0 ? 0 : header.vertex_range[i-1];
        int seg_end = header.vertex_range[i];
        std::cout << "vertex: " << src << " has " << edge_count << " neighbors" << std::endl;
        std::cout << "vertex " << src << " range from " << seg_start << " to " << seg_end << std::endl; 
        for (int j = seg_start ; j < seg_end ; j ++)
        {
            for (int k = 0 ; k < SEGMENT_SIZE ; k++)
            {
                std::cout <<pma_data[j*SEGMENT_SIZE + k] << " ";
            }
        }
        std::cout << std::endl;
    }
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <xclbin> <graph> [device_index=1]" << std::endl;
        return 1;
    }


    std::string xclbin_name = argv[1];
    std::string graph_path = argv[2];
    auto start_init = std::chrono::high_resolution_clock::now();
    
    std::fstream graph_file(graph_path);
    if (!graph_file.is_open()) {
        std::cerr << "Failed to open graph file: " << graph_path << std::endl;
        return 1;
    }

    struct Operation
    {
        int src;
        int dst;
        long long timestamp;
        int opcode;
    };

    std::vector<Operation> all_ops;
    int a = -1, b = -1, op = PMA_UPDATE_INSERT_EDGE;
    long long ts = 0;

    // 输入格式: src dst timestamp opcode  (timestamp == -1 → base graph)
    while (graph_file >> a >> b >> ts >> op)
        all_ops.push_back({a, b, ts, op});
    graph_file.close();

    // Separate base graph (ts == -1) from update operations (ts >= 0)
    std::vector<Operation> base_ops, operations;
    for (const auto& opr : all_ops) {
        if (opr.timestamp == -1) base_ops.push_back(opr);
        else                     operations.push_back(opr);
    }
    std::cout << "Base graph edges: " << base_ops.size()
              << ", Update edges: " << operations.size() << std::endl;

    // Helper: build adjacency list from operations
    auto build_graph = [](const std::vector<Operation>& ops, int& edge_count) -> std::vector<std::vector<int>> {
        std::vector<std::vector<int>> g;
        auto ensure = [&g](int v) {
            if (v >= static_cast<int>(g.size())) g.resize(v + 1);
        };
        edge_count = 0;
        for (const auto& opr : ops) {
            switch (opr.opcode) {
                case PMA_UPDATE_INSERT_EDGE:
                    ensure(opr.src); ensure(opr.dst);
                    g[opr.src].push_back(opr.dst); edge_count++;
                    break;
                case PMA_UPDATE_DELETE_EDGE:
                    ensure(opr.src); ensure(opr.dst);
                    if (opr.src < static_cast<int>(g.size())) {
                        auto& nbrs = g[opr.src];
                        nbrs.erase(std::remove(nbrs.begin(), nbrs.end(), opr.dst), nbrs.end());
                    }
                    edge_count--;
                    break;
                case PMA_UPDATE_DELETE_VERTEX:
                    if (opr.src >= 0) {
                        ensure(opr.src);
                        if (opr.src < static_cast<int>(g.size())) g[opr.src].clear();
                    }
                    break;
                default: break;
            }
        }
        for (auto& nbrs : g) {
            std::sort(nbrs.begin(), nbrs.end());
            nbrs.erase(std::unique(nbrs.begin(), nbrs.end()), nbrs.end());
        }
        return g;
    };

    int base_edge_count = 0;
    std::vector<std::vector<int>> BaseGraph = build_graph(base_ops, base_edge_count);
    int base_max_vertex = static_cast<int>(BaseGraph.size()) - 1;
    std::cout << "BaseGraph: vertices=" << BaseGraph.size() << ", edges=" << base_edge_count << std::endl;

    int full_edge_count = 0;
    std::vector<std::vector<int>> Graph = build_graph(all_ops, full_edge_count);
    std::cout << "Full graph: vertices=" << Graph.size() << ", edges=" << full_edge_count << std::endl;

    int max_vertex_idx = static_cast<int>(Graph.size()) - 1;
    size_t buffer_len = std::max<size_t>(1, (base_edge_count + operations.size()) * 2 + Graph.size() + 1024);
    int static_max_vertex = max_vertex_idx;

    size_t max_edge_batch = 0;
    size_t current_edge_batch = 0;
    long long current_edge_ts = 0;
    bool first_edge_batch = true;
    for (const auto &op : operations)
    {
        // if (op.opcode == PMA_UPDATE_INSERT_VERTEX)
        // {
        //     continue;
        // }
        if (first_edge_batch || op.timestamp != current_edge_ts)
        {
            max_edge_batch = std::max(max_edge_batch, current_edge_batch);
            current_edge_batch = 0;
            current_edge_ts = op.timestamp;
            first_edge_batch = false;
        }
        current_edge_batch++;
    }
    max_edge_batch = std::max(max_edge_batch, current_edge_batch);
    std::cout << "Max edge batch count: " << max_edge_batch << std::endl;

    std::cout << "Operations read from the file: " << operations.size() << std::endl;

    auto end_init = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_init = end_init - start_init;
    std::cout << "Initialization (File Read & Graph Build) time: " << elapsed_init.count() << " s" << std::endl;

    auto xclbin = xrt::xclbin(xclbin_name);
    auto device = xrt::device(0);

    auto uuid = device.load_xclbin(xclbin);
    std::cout << "device name:     " << device.get_info<xrt::info::device::name>() << "\n";
    std::cout << "device bdf:      " << device.get_info<xrt::info::device::bdf>() << "\n";

    // auto alloc_access_kernel_1 = xrt::kernel(device, uuid, "alloc_access_kernel:{alloc_access_kernel_1}");
    // auto alloc_access_kernel_2 = xrt::kernel(device, uuid, "alloc_access_kernel:{alloc_access_kernel_2}");
    // xrt::kernel alloc_access_kernels[CHANNEL_NUM] = {
    //     alloc_access_kernel_1,
    //     alloc_access_kernel_2
    // };
    // alloc_access & mem_access removed — kernels use direct m_axi
    auto pma_read_router_kernel = xrt::kernel(device, uuid, "pma_read_router_kernel:{pma_read_router_kernel_1}");
    std::vector<xrt::kernel> pma_read_worker_kernels;
    pma_read_worker_kernels.reserve(CHANNEL_NUM);
    for (int i = 1; i <= CHANNEL_NUM; ++i)
    {
        pma_read_worker_kernels.emplace_back(device, uuid,
            "pma_read_worker_kernel:{pma_read_worker_kernel_" + std::to_string(i) + "}");
    }
    auto pma_read_merger_kernel = xrt::kernel(device, uuid, "pma_read_merger_kernel:{pma_read_merger_kernel_1}");
    auto update_message_generator_kernel = xrt::kernel(device, uuid, "update_message_generator");
    auto update_message_router_kernel = xrt::kernel(device, uuid, "update_message_router");
    auto try_add_kernel = [&](std::vector<xrt::kernel> &kernels, const std::string &name) -> bool {
        try {
            kernels.emplace_back(device, uuid, name);
            return true;
        } catch (const std::runtime_error &e) {
            std::string msg = e.what();
            if (msg.find("No compute units matching") != std::string::npos) {
                return false;
            }
            throw;
        }
    };

    // std::vector<xrt::kernel> pma_insert_vertex_kernels;
    // pma_insert_vertex_kernels.reserve(CHANNEL_NUM);
    // for (int i = 1; i <= CHANNEL_NUM; ++i)
    // {
    //     if (!try_add_kernel(pma_insert_vertex_kernels, "pma_insert_vertex:{pma_insert_vertex_" + std::to_string(i) + "}")) {
    //         break;
    //     }
    // }
    std::vector<xrt::kernel> pma_insert_edge_kernels;
    pma_insert_edge_kernels.reserve(CHANNEL_NUM);
    for (int i = 1; i <= CHANNEL_NUM; ++i)
    {
        if (!try_add_kernel(pma_insert_edge_kernels, "pma_insert_edge:{pma_insert_edge_" + std::to_string(i) + "}")) {
            break;
        }
    }
    auto check_kernel = xrt::kernel(device, uuid, "check_kernel");
    std::cout << "[DBG] L233: Kernels loaded successfully." << std::endl;
    std::cout << "[DBG] L234: insert_edge=" << pma_insert_edge_kernels.size()
              << " read_worker=" << pma_read_worker_kernels.size()
              << " channels=" << CHANNEL_NUM << std::endl;
    if (pma_read_worker_kernels.size() < static_cast<size_t>(CHANNEL_NUM) ||
        // pma_insert_vertex_kernels.size() < static_cast<size_t>(CHANNEL_NUM) ||
        pma_insert_edge_kernels.size() < static_cast<size_t>(CHANNEL_NUM))
    {
        std::cerr << "Kernel loading incomplete for read/insert CUs; aborting before launch." << std::endl;
        std::cerr << "Loaded read workers: " << pma_read_worker_kernels.size()
                //   << ", insert vertex kernels: " << pma_insert_vertex_kernels.size()
                  << ", insert edge kernels: " << pma_insert_edge_kernels.size() << std::endl;
        return 1;
    }
    // alloc & mem runs removed — using direct m_axi
    std::cout << "Launching kernels..." << std::endl;
    auto run_pma_read_router_kernel = xrt::run(pma_read_router_kernel);
    std::vector<xrt::run> run_pma_read_worker_kernels;
    run_pma_read_worker_kernels.reserve(pma_read_worker_kernels.size());
    for (auto &k : pma_read_worker_kernels)
    {
        run_pma_read_worker_kernels.emplace_back(k);
    }
    auto run_pma_read_merger_kernel = xrt::run(pma_read_merger_kernel);
    auto run_update_message_router_kernel = xrt::run(update_message_router_kernel);
    std::cout << "Update message router kernel is launched..." << std::endl;
    // std::vector<xrt::run> run_pma_insert_vertex_kernels;
    // run_pma_insert_vertex_kernels.reserve(pma_insert_vertex_kernels.size());

    // for (auto &k : pma_insert_vertex_kernels)
    // {
    //     run_pma_insert_vertex_kernels.emplace_back(k);
    // }
    std::cout << "PMA insert vertex kernels are launched..." << std::endl;
    std::vector<xrt::run> run_pma_insert_edge_kernels;
    run_pma_insert_edge_kernels.reserve(pma_insert_edge_kernels.size());
    for (auto &k : pma_insert_edge_kernels)
    {
        run_pma_insert_edge_kernels.emplace_back(k);
    }
    std::cout << "PMA insert edge kernels are launched..." << std::endl;
    std::cout << "Kernels are created..." << std::endl;

    auto start_buffer = std::chrono::high_resolution_clock::now();
    // === m_axi HBM layout per channel ===
    constexpr size_t PMA_DATA_SIZE   = 230 * 1024 * 1024;
    constexpr size_t VECINFO_SIZE    =  10 * 1024 * 1024;
    constexpr size_t BM_SIZE         =   6 * 1024 * 1024;
    constexpr size_t CHANNEL_TOTAL   = PMA_DATA_SIZE + VECINFO_SIZE + BM_SIZE;
    int pma_raw_data_start = 0;
    int pma_vecinfo_start  = PMA_DATA_SIZE;
    int pma_bm_start       = PMA_DATA_SIZE + VECINFO_SIZE;

    // --- One BO per channel — all 6 m_axi ports share the same bank now ---
    std::vector<xrt::bo> ch_bufs; ch_bufs.reserve(CHANNEL_NUM);
    for (int ch = 0; ch < CHANNEL_NUM; ++ch) {
        auto& k = pma_insert_edge_kernels[ch];
        std::cout << "[DBG] L290: ch=" << ch
                  << " gid0=" << k.group_id(0) << " gid1=" << k.group_id(1)
                  << " gid2=" << k.group_id(2) << " gid3=" << k.group_id(3)
                  << " gid4=" << k.group_id(4) << std::endl;
        ch_bufs.emplace_back(device, CHANNEL_TOTAL, k.group_id(0));
    }

    std::cout << "[DBG] L286: HBM layout — PMA_DATA=" << PMA_DATA_SIZE/1024/1024
              << "MB VECINFO=" << VECINFO_SIZE/1024/1024
              << "MB BM=" << BM_SIZE/1024/1024 << "MB"
              << " CH_TOTAL=" << CHANNEL_TOTAL/1024/1024 << "MB" << std::endl;
    std::cout << "[DBG] L286: Offsets — raw=" << pma_raw_data_start
              << " vi=" << pma_vecinfo_start
              << " bm=" << pma_bm_start << std::endl;

    // --- Map host pointers (all point to same HBM region, different offsets) ---
    std::vector<BYTE*> hbm_base_addresses; hbm_base_addresses.reserve(CHANNEL_NUM);
    for (int ch = 0; ch < CHANNEL_NUM; ++ch)
        hbm_base_addresses.push_back(ch_bufs[ch].map<BYTE*>());

    std::vector<VEC_INFO*> vec_info_ptrs; vec_info_ptrs.reserve(CHANNEL_NUM);
    for (int ch = 0; ch < CHANNEL_NUM; ++ch) {
        vec_info_ptrs.push_back(reinterpret_cast<VEC_INFO*>(hbm_base_addresses[ch] + pma_vecinfo_start));
        std::fill_n(vec_info_ptrs[ch], MAX_PMA_NUM, 0);
    }

    std::vector<BYTE*> bitmap_ptrs; bitmap_ptrs.reserve(CHANNEL_NUM);
    for (int ch = 0; ch < CHANNEL_NUM; ++ch)
        bitmap_ptrs.push_back(hbm_base_addresses[ch] + pma_bm_start);

    std::cout << "[DBG] L327: hbm_base_addresses mapped, ch0=" << (void*)hbm_base_addresses[0] << " vi=" << (void*)vec_info_ptrs[0] << " bm=" << (void*)bitmap_ptrs[0] << std::endl;
    // Generator buffers
    const size_t kDefaultMaxEdgeBuffer = 1000000;
    size_t edge_buffer_len = std::max<size_t>(1, std::min(max_edge_batch, kDefaultMaxEdgeBuffer));
    if (edge_buffer_len < max_edge_batch) {
        std::cout << "Update buffer length capped per-device: " << edge_buffer_len
                  << " (original " << max_edge_batch << ")" << std::endl;
    } else {
        std::cout << "Update buffer length: " << edge_buffer_len << std::endl;
    }

    auto bo_src = xrt::bo(device, sizeof(int) * edge_buffer_len, update_message_generator_kernel.group_id(1));
    auto bo_dst = xrt::bo(device, sizeof(int) * edge_buffer_len, update_message_generator_kernel.group_id(2));
    auto bo_opcode = xrt::bo(device, sizeof(int) * edge_buffer_len, update_message_generator_kernel.group_id(3));
    std::cout << "Buffers for edge operations allocated." << std::endl;
    int* src_ptr = bo_src.map<int*>();
    int* dst_ptr = bo_dst.map<int*>();
    int* opcode_ptr = bo_opcode.map<int*>();

    // 边数据按阶段写入，初始不填充

    std::cout << "[DBG] L335: Allocating COO buffers, buffer_len=" << buffer_len << " (" << (sizeof(int)*buffer_len/1024/1024) << "MB each)" << std::endl;
    auto bo_coo_src = xrt::bo(device, sizeof(int) * buffer_len, check_kernel.group_id(0));
    auto bo_coo_dst = xrt::bo(device, sizeof(int) * buffer_len, check_kernel.group_id(1));
    int* coo_src_ptr = bo_coo_src.map<int*>();
    int* coo_dst_ptr = bo_coo_dst.map<int*>();
    auto bo_num_edges = xrt::bo(device, sizeof(int), check_kernel.group_id(2));

    int* bo_num_edges_ptr = bo_num_edges.map<int*>();

    // bitmap_ptrs now set up in m_axi buffer section above
    for (int i = 0; i < CHANNEL_NUM; ++i)
    {
      // bitmap now lives inside the m_axi buffer at offset pma_bm_start — no separate allocation
    }

    auto end_buffer = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_buffer = end_buffer - start_buffer;
    std::cout << "Buffer allocation and initialization time: " << elapsed_buffer.count() << " s" << std::endl;

    std::cout << "Buffers allocated and initialized." << std::endl;

    // --- Set kernel arguments for m_axi ports ---
    for (int ch = 0; ch < CHANNEL_NUM; ++ch) {
        auto& run = run_pma_insert_edge_kernels[ch];
        int a = 0;
        run.set_arg(a++, ch_bufs[ch]);
        run.set_arg(a++, ch_bufs[ch]);
        run.set_arg(a++, ch_bufs[ch]);
    std::cout << "[DBG] L373: insert_edge kernel args set (5 ports + 3 offsets) for " << CHANNEL_NUM << " channels" << std::endl;
        run.set_arg(a++, ch_bufs[ch]);
        run.set_arg(a++, ch_bufs[ch]);
        run.set_arg(a++, pma_raw_data_start);
        run.set_arg(a++, pma_vecinfo_start);
        run.set_arg(a++, pma_bm_start);
    }
    for (int ch = 0; ch < static_cast<int>(run_pma_read_worker_kernels.size()); ++ch) {
        auto& run = run_pma_read_worker_kernels[ch];
    std::cout << "[DBG] L381: read_worker kernel args set (1 port + 2 offsets) for " << run_pma_read_worker_kernels.size() << " workers" << std::endl;
        int a = 0;
        run.set_arg(a++, ch);
        run.set_arg(a++, ch_bufs[ch]);
        run.set_arg(a++, pma_raw_data_start);
        run.set_arg(a++, pma_vecinfo_start);
    }

    auto verify_graph = [&](const std::vector<std::vector<int>>& expected_graph, int current_max_vertex, const std::string& label, bool last) -> double {
        std::cout << "Verifying " << label << "..." << std::endl;
        int check_vertex_range = std::max(current_max_vertex, base_max_vertex) + 1;
        std::vector<std::vector<int>> Updated_Graph(check_vertex_range);

        auto run_check_kernel = xrt::run(check_kernel);
        run_check_kernel.set_arg(0, bo_coo_src);
        run_check_kernel.set_arg(1, bo_coo_dst);
        run_check_kernel.set_arg(2, bo_num_edges);
    std::cout << "[DBG] L397: generator buffers — edge_buf_len=" << edge_buffer_len << " coo_buf_len=" << buffer_len << std::endl;
        run_check_kernel.set_arg(3, check_vertex_range);
        run_check_kernel.set_arg(4, 3);
        run_check_kernel.set_arg(5, 0);
        std::cout << "Starting check kernel for " << label << " with max vertex " << check_vertex_range - 1 << "..." << std::endl;

        // ---- kernel-only timing ----
        auto k_start = std::chrono::high_resolution_clock::now();

        run_check_kernel.start();
        run_check_kernel.wait();

        bo_num_edges.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        int edge_num = bo_num_edges_ptr[0];
        std::cout << "Check kernel returned edge count: " << edge_num << std::endl;

        run_check_kernel.set_arg(4, 1);
        run_check_kernel.set_arg(5, last);
        run_check_kernel.start();
        run_check_kernel.wait();

        bo_coo_src.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        bo_coo_dst.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

        auto k_end = std::chrono::high_resolution_clock::now();
        double kernel_time = std::chrono::duration<double>(k_end - k_start).count();
        // ---- end kernel-only timing ----

        // Build Updated_Graph from COO arrays
        for (int i = 0; i < check_vertex_range; ++i) Updated_Graph[i].clear();
        for (int id = 0; id < static_cast<int>(buffer_len); ++id) {
            int s = coo_src_ptr[id];
            int d = coo_dst_ptr[id];
            if (s == -1 || d == -1) break;
            if (s < 0 || s >= check_vertex_range) continue;
            Updated_Graph[s].push_back(d);
        }
        for (int i = 0; i < check_vertex_range; ++i) {
            std::sort(Updated_Graph[i].begin(), Updated_Graph[i].end());
            Updated_Graph[i].erase(std::unique(Updated_Graph[i].begin(), Updated_Graph[i].end()), Updated_Graph[i].end());
        }
        bool all_match = true;
        std::string clean_label = label;
        int cmp_end = check_vertex_range - 1;
        if (kMismatchfileOutput) {
            std::replace(clean_label.begin(), clean_label.end(), ' ', '_');
            std::ofstream output_file("mismatch_" + clean_label + ".txt");
            for (int i = 0; i <= cmp_end; ++i)
            {
                const std::vector<int>& expected_neighbors = (i < (int)expected_graph.size()) ? expected_graph[i] : std::vector<int>();

                if (expected_neighbors != Updated_Graph[i]) {
                    output_file << "MISMATCH AT VERTEX " << i << std::endl;
                    for (auto v : expected_neighbors) output_file << "EXPECTED EDGE: " << i << " -> " << v << std::endl;
                    for (auto v : Updated_Graph[i]) output_file << "ACTUAL EDGE: " << i << " -> " << v << std::endl;
                    all_match = false;
                }
            }
        }
        else {
            for (int i = 0; i <= cmp_end; ++i)
            {
                const std::vector<int>& expected_neighbors = (i < (int)expected_graph.size()) ? expected_graph[i] : std::vector<int>();

                if (expected_neighbors != Updated_Graph[i]) {
                    all_match = false;
                    break;
                }
            }
        }
        
        if (all_match) {
            std::cout << label << " matches perfectly!" << std::endl;
        } else {
            std::cout << label << " does not match. "<< std::endl;
            final_match = false;
            if (kMismatchfileOutput) {
                std::cout << "See mismatch_" << clean_label << ".txt for details." << std::endl;
            }
        }
        return kernel_time;
    };

    auto add_base_graph_to_hbm = [&](int vertex_num, int edge_num) -> bool
    {
        auto start_cpu_preprocess = std::chrono::high_resolution_clock::now();
        const int total_pma_num = (vertex_num + VERTEX_PER_PMA - 1) / VERTEX_PER_PMA;
        if (total_pma_num > MAX_PMA_NUM)
        {
            std::cerr << "PMA count overflow: required=" << total_pma_num
                      << ", MAX_PMA_NUM=" << MAX_PMA_NUM << std::endl;
            return false;
        }
        std::cout << "[DBG] L488: PMA init start, total_pma_num=" << total_pma_num << " vertex_num=" << vertex_num << std::endl;
        std::vector<std::vector<int>> adj_pos(vertex_num + 1); // src -> dst
        if (!BaseGraph.empty())
        {
            adj_pos = BaseGraph;
            if (adj_pos.size() < static_cast<size_t>(vertex_num + 1))
            {
                adj_pos.resize(vertex_num + 1);
            }
        }
        // std::vector<std::vector<int>> adj_neg(vertex_num+1); // dst -> src

        // Base graph currently empty; no edges to preload.

        // 2. 对邻居进行排序
        for(auto& neighbors : adj_pos) std::sort(neighbors.begin(), neighbors.end());
        // for (int i = 0 ; i < vertex_num; ++i)
        // {
        //     std::cout << "Vertex " << i << " has " << adj_pos[i].size() << " neighbors." << std::endl;
        //     for (int j = 0; j < adj_pos[i].size(); ++j) 
        //     {
        //         std::cout<< adj_pos[i][j] << std::endl;
        //     }
        // }
        // for(auto& neighbors : adj_neg) std::sort(neighbors.begin(), neighbors.end());

        int current_addr_offset[CHANNEL_NUM];
        for (int i = 0; i < CHANNEL_NUM; ++i)
        {
            current_addr_offset[i] = 0;
        }
        // std::ofstream debug_pma("debug_pma_content.txt");

        // 3. 构建 PMA 结构
        for (int pma_idx = 0; pma_idx < total_pma_num; ++pma_idx)
        {
            BYTE* current_base_addr = hbm_base_addresses[pma_idx % CHANNEL_NUM];
            int* current_offset = &current_addr_offset[pma_idx % CHANNEL_NUM];
            int channel = pma_idx % CHANNEL_NUM;
            // POS Direction
            {
                PMA_HEADER pos_header = {0};
                std::vector<int> pos_data;
                
                for (int i = 0 ; i < VERTEX_PER_PMA; i++)
                {
                    int src = i + pma_idx * VERTEX_PER_PMA;
                    std::vector<int> neighbors;
                    if (src < vertex_num) {
                        neighbors = adj_pos[src];
                    }

                    pos_header.edge_count[i] = neighbors.size();
                    pos_header.attr[i] = 0;

                    int last_pos_range = i == 0 ? 0 : pos_header.vertex_range[i-1];
                    int total_segments = 1;
                    // Match kernel: density_capacity=seg_count*47, enlarge=4x
                    if (neighbors.size() > 0) {
                        int segs = 1;
                        while (segs * 47 < (int)neighbors.size())
                            segs *= 4;
                        total_segments = segs;
                    }
                    pos_header.vertex_range[i] = last_pos_range + total_segments;

                    // Fill data
                    int base_num = 0;
                    int extra_num = 0;
                    if (total_segments > 0) {
                        base_num = neighbors.size() / total_segments;
                        extra_num = neighbors.size() % total_segments;
                    }
                    
                    int neighbor_idx = 0;
                    for (int s = 0; s < total_segments; ++s) {
                        int valid_num = base_num + (s < extra_num ? 1 : 0);
                        for (int k = 0; k < valid_num; ++k) {
                            pos_data.push_back(neighbors[neighbor_idx++]);
                        }
                        for (int k = valid_num; k < EDGE_SLOTS_PER_SEG; ++k) {
                            pos_data.push_back(UNUSED_ICON);
                        }
                        // Slot VALID_COUNT_IDX (63) = valid_count
                        pos_data.push_back(valid_num);
                    }
                }
                
                // Write to HBM
                int total_len = sizeof(PMA_HEADER) + pos_data.size() * sizeof(int);
                int aligned_len = total_len;
                if (aligned_len % INIT_VEC_LEN_IN_BYTE != 0) aligned_len += (INIT_VEC_LEN_IN_BYTE - (aligned_len % INIT_VEC_LEN_IN_BYTE));
                if (*current_offset < 0 || static_cast<size_t>(*current_offset) + static_cast<size_t>(aligned_len) > static_cast<size_t>(CHANNEL_CAPACITY))
                {
                    std::cerr << "HBM capacity overflow on channel " << channel
                              << ": offset=" << *current_offset
                              << ", write_bytes=" << aligned_len
                              << ", capacity=" << CHANNEL_CAPACITY << std::endl;
                    return false;
                }

                memcpy(current_base_addr + *current_offset, &pos_header, sizeof(PMA_HEADER));
                memcpy(current_base_addr + *current_offset + sizeof(PMA_HEADER), pos_data.data(), pos_data.size() * sizeof(int));

                // pos_vec_info_ptr[pma_idx] = (ap_uint<64>(total_len) << 32) | *current_offset;
                vec_info_ptrs[channel][pma_idx] = (ap_uint<64>(total_len) << 32) | *current_offset;
                
                *current_offset += aligned_len;
                // Align to 64 bytes
                // if (current_addr_offset % 64 != 0) current_addr_offset += (64 - (current_addr_offset % 64));
            }

            // NEG Direction
            
        }

        //
        std::cout << "Base graph PMA construction completed. PMA info:" << std::endl;
        // for (int i = 0 ; i < 10 ; i++)
        // {
        //     int corresponding_channel = i % CHANNEL_NUM;
        //     if (vec_info_ptrs[i % CHANNEL_NUM][i] == 0) break;
        //     int start = vec_info_ptrs[i % CHANNEL_NUM][i].range(31,0);
        //     int len =  vec_info_ptrs[i % CHANNEL_NUM][i].range(63,32);
        //     std::cout << "PMA " << i << " ADDR START FROM " << start << " LEN " << len << std::endl; 
        //     PMA_HEADER header;
        //     memcpy(&header, hbm_base_addresses[corresponding_channel] + start, sizeof(PMA_HEADER));
        //     for (int src = 0 ; src < VERTEX_PER_PMA; src++)
        //     {
        //         int edge_count = header.edge_count[src];
        //         int seg_start = src == 0 ? 0 : header.vertex_range[src-1];
        //         int seg_end = header.vertex_range[src];
        //         std::cout << "vertex: " << src << " has " << edge_count << " neighbors" << std::endl;
        //         for (int j = seg_start ; j < seg_end ; j ++)
        //         {
        //             for (int k = 0 ; k < SEGMENT_SIZE ; k++)
        //             {
        //                 int neighbor = *((int*)(hbm_base_addresses[corresponding_channel] + start + sizeof(PMA_HEADER) + (j * SEGMENT_SIZE + k) * sizeof(int)));
        //                 if (neighbor == UNUSED_ICON)
        //                 {
        //                     break;
        //                 }
        //                 std::cout << neighbor << " ";
        //             }
        //         }
        //     }
        //         std::cout << std::endl;
        // }
        //
        // Update bitmap
        for (int ch = 0; ch < CHANNEL_NUM; ++ch)
        {
    std::cout << "[DBG] L640: PMA init complete, max offset per ch: " << current_addr_offset[0] << ", " << (CHANNEL_NUM>1?current_addr_offset[1]:0) << std::endl;
            int total_blocks = (current_addr_offset[ch] + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
            int full_bytes = total_blocks / 8;
            int rem_bits = total_blocks % 8;
            memset(bitmap_ptrs[ch], 0xFF, full_bytes);
            if (rem_bits > 0)
                bitmap_ptrs[ch][full_bytes] = (BYTE)((1 << rem_bits) - 1);
        }

        // Update mem_access kernel arg 2: total PMA count
        for (int i = 0; i < CHANNEL_NUM; ++i)

        // Sync modified buffers to device
        for (int ch = 0; ch < CHANNEL_NUM; ++ch) { ch_bufs[ch].sync(XCL_BO_SYNC_BO_TO_DEVICE); }

        auto end_cpu_preprocess = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_cpu_preprocess = end_cpu_preprocess - start_cpu_preprocess;
        std::cout << "add_base_graph_to_hbm: " << total_pma_num << " PMAs, "
                  << elapsed_cpu_preprocess.count() << " s" << std::endl;

        return true;
    };

    // bo_valid_array.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::cout << "Buffers synchronized to device." << std::endl;

    std::cout << "Static vertex count: " << (static_max_vertex + 1) << std::endl;
    // std::cout << "Static edge count: " << base_edges << std::endl;
    // std::cout << "Incremental new vertex count: " << incremental_new_vertex_count << std::endl;
    // std::cout << "Incremental edge/vertex ops (non-insert-vertex): " << incremental_edges << std::endl;
    std::cout << "Expected max vertex after update: " << (max_vertex_idx + 1) << std::endl;

    auto load_edge_batch = [&](const std::vector<Operation>& batch_ops) -> int {
        size_t edge_idx = 0;
        for (const auto &op : batch_ops)
        {
            // if (op.opcode == PMA_UPDATE_INSERT_VERTEX)
            // {
            //     continue;
            // }
    std::cout << "[DBG] L680: Starting edge update batches, total_ops=" << operations.size() << std::endl;
            if (edge_idx >= edge_buffer_len)
            {
                std::cerr << "Edge buffer overflow in batch load." << std::endl;
                return -1;
            }
            src_ptr[edge_idx] = op.src;
            dst_ptr[edge_idx] = op.dst;
            // std::cout << "Loading edge: " << op.src << " -> " << op.dst << " with opcode " << op.opcode << std::endl;
            opcode_ptr[edge_idx] = op.opcode;
            ++edge_idx;
        }
        if (edge_idx > 0)
        {
            bo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
            bo_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
            bo_opcode.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        }
        return static_cast<int>(edge_idx);
    };

    auto run_update_batch = [&]( int edge_count, const std::string& label, bool last) -> double {
        auto run_update_message_generator_kernel = xrt::run(update_message_generator_kernel);
        std::cout << "Update message generator kernel is launched..." << std::endl;
                      // add_vertex_num
        run_update_message_generator_kernel.set_arg(0, edge_count);                               // delete_edge_num
        run_update_message_generator_kernel.set_arg(1, bo_src);                              // src
        run_update_message_generator_kernel.set_arg(2, bo_dst);                              // dst
        run_update_message_generator_kernel.set_arg(3, bo_opcode);                           // opcode
        run_update_message_generator_kernel.set_arg(4, static_cast<int>(last));             // last flag

        auto phase_start = std::chrono::high_resolution_clock::now();

        // Router and Consumer kernels are already running (Keep Alive).
        // Only start the Generator for this batch.
        run_update_message_generator_kernel.start();

        // Generator waits for ACK from Router internally, so when it finishes, 
        // the batch is guaranteed to be processed by downstream kernels.
        run_update_message_generator_kernel.wait();

        auto phase_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = phase_end - phase_start;
        std::cout << label << "edges=" << edge_count
                  << ", time=" << elapsed.count() << " s" << std::endl;
        return elapsed.count();
    };

    // 1. Load base graph into HBM (ts == -1 edges) BEFORE starting kernels
    if (base_max_vertex >= 0) {
        if (!add_base_graph_to_hbm(base_max_vertex + 1, base_edge_count)) {
            std::cerr << "Failed to load base graph into HBM." << std::endl;
            return 1;
        }
    } else {
    std::cout << "[DBG] L734: All kernels starting (insert, router, read, merger)..." << std::endl;
        std::cout << "No base graph (ts==-1), skipping PMA preload." << std::endl;
    }

    // 2. Start Long-Running (Keep Alive) Kernels
    for (auto &r : run_pma_insert_edge_kernels) { r.start(); }
    run_update_message_router_kernel.start();
    for (auto &run_worker : run_pma_read_worker_kernels) { run_worker.start(); }
    run_pma_read_merger_kernel.start();
    run_pma_read_router_kernel.start();

    bo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_opcode.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    // Start from BaseGraph — FPGA has base edges in HBM
    std::vector<std::vector<int>> runtime_graph = BaseGraph;
    auto ensure_runtime_vertex = [&](int v)
    {
        if (v < 0) return;
        if (v >= static_cast<int>(runtime_graph.size()))
        {
            runtime_graph.resize(v + 1);
        }
    };
    auto normalize_neighbors = [&](int v)
    {
        if (v < 0 || v >= static_cast<int>(runtime_graph.size())) return;
        auto &nbrs = runtime_graph[v];
        std::sort(nbrs.begin(), nbrs.end());
        nbrs.erase(std::unique(nbrs.begin(), nbrs.end()), nbrs.end());
    };

    double base_phase_s = 0;
    double incremental_phase_s = 0;
    std::mt19937 rng(static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

    // Timing CSV output
    std::string graph_stem = graph_path;
    {
        auto slash = graph_stem.find_last_of("/\\");
        if (slash != std::string::npos) graph_stem = graph_stem.substr(slash + 1);
        auto dot = graph_stem.find_last_of('.');
        if (dot != std::string::npos) graph_stem = graph_stem.substr(0, dot);
    }
    std::ofstream timing_log(graph_stem + "_batch_timing.csv");
    timing_log << "batch,edges,update_time_s,read_time_s" << std::endl;
    int batch_idx = 0;

    auto send_update_stop_only = [&]() {
        auto run_update_message_generator_kernel = xrt::run(update_message_generator_kernel);
        run_update_message_generator_kernel.set_arg(0, 0); // add_vertex_num
        run_update_message_generator_kernel.set_arg(1, 0); // add_edge_num
        run_update_message_generator_kernel.set_arg(2, 0); // delete_vertex_num
        run_update_message_generator_kernel.set_arg(3, 0); // delete_edge_num
        run_update_message_generator_kernel.set_arg(4, bo_src);
        run_update_message_generator_kernel.set_arg(5, bo_dst);
        run_update_message_generator_kernel.set_arg(6, bo_opcode);
        run_update_message_generator_kernel.set_arg(7, 1); // last flag
        run_update_message_generator_kernel.start();
        run_update_message_generator_kernel.wait();
    };
    if (kBaseGraphReadOnlyTest)
    {
        if (!Graph.empty())
        {
            verify_graph(Graph, max_vertex_idx, "Base_Graph", true);
        }
        else
        {
            std::cout << "Base graph verification skipped (no vertices)." << std::endl;
        }
        // No incremental updates in this mode; send an explicit stop to update pipeline.
        send_update_stop_only();
    }
    else
    {
        for (size_t idx = 0; idx < operations.size(); )
        {
            long long current_ts = operations[idx].timestamp;
            std::vector<Operation> batch_ops;
            while (idx < operations.size() && operations[idx].timestamp == current_ts)
            {
                batch_ops.push_back(operations[idx]);
                ++idx;
            }

            if (batch_ops.size() > 1 && kShuffleSameTimestampOps)
            {
                std::shuffle(batch_ops.begin(), batch_ops.end(), rng); // shuffle the whole operation tuple (src, dst, opcode) within same timestamp
            }

            // int new_vertex_count = 0;
            // for (const auto &op : batch_ops)
            // {
            //     if (op.opcode == PMA_UPDATE_INSERT_VERTEX)
            //     {
            //         new_vertex_count += 1;
            //     }
            // }

            int edge_count = load_edge_batch(batch_ops);
            if (edge_count < 0)
            {
                break;
            }
            if (kVertexOnlyTest)
            {
                edge_count = 0;
            }

            bool last = (idx == operations.size());
            double update_time = run_update_batch(edge_count, "ts_" + std::to_string(current_ts), last);
            incremental_phase_s += update_time;

            std::cout << "Waiting 5s after update batch..." << std::endl;
            std::this_thread::sleep_for(std::chrono::microseconds(100));

            std::cout << "prepareing verification result on CPU for timestamp " << current_ts << "..." << std::endl;
            std::vector<int> dirty_vertices;
            dirty_vertices.reserve(batch_ops.size());
            for (const auto &op : batch_ops)
            {
                switch (op.opcode)
                {
                    // case PMA_UPDATE_INSERT_VERTEX:
                    //     ensure_runtime_vertex(op.src >= 0 ? op.src : op.dst);
                    //     break;
                    case PMA_UPDATE_INSERT_EDGE:
                        if (kVertexOnlyTest) break;
                        ensure_runtime_vertex(op.src);
                        ensure_runtime_vertex(op.dst);
                        if (op.src >= 0 && op.src < static_cast<int>(runtime_graph.size()))
                        {
                            runtime_graph[op.src].push_back(op.dst);
                            dirty_vertices.push_back(op.src);
                        }
                        break;
                    case PMA_UPDATE_DELETE_EDGE:
                        if (kVertexOnlyTest) break;
                        ensure_runtime_vertex(op.src);
                        ensure_runtime_vertex(op.dst);
                        if (op.src >= 0 && op.src < static_cast<int>(runtime_graph.size()))
                        {
                            auto &nbrs = runtime_graph[op.src];
                            nbrs.erase(std::remove(nbrs.begin(), nbrs.end(), op.dst), nbrs.end());
                            dirty_vertices.push_back(op.src);
                        }
                        break;
                    case PMA_UPDATE_DELETE_VERTEX:
                        if (kVertexOnlyTest) break;
                        ensure_runtime_vertex(op.src);
                        if (op.src >= 0 && op.src < static_cast<int>(runtime_graph.size()))
                        {
                            runtime_graph[op.src].clear();
                            dirty_vertices.push_back(op.src);
                        }
                        // for (auto &nbrs : runtime_graph)
                        // {
                        //     nbrs.erase(std::remove(nbrs.begin(), nbrs.end(), op.src), nbrs.end());
                        // }
                        break;
                    default:
                        break;
                }
            }

            std::sort(dirty_vertices.begin(), dirty_vertices.end());
            dirty_vertices.erase(std::unique(dirty_vertices.begin(), dirty_vertices.end()), dirty_vertices.end());

            for (int v : dirty_vertices)
            {
                normalize_neighbors(v);
            }
            std::cout << "CPU-side graph updated for timestamp " << current_ts << ". Verifying results..." << std::endl;
            int current_max_vertex = static_cast<int>(runtime_graph.size()) - 1;
            if (!runtime_graph.empty())
            {
                double read_time = verify_graph(runtime_graph, current_max_vertex,
                                                "after_ts_" + std::to_string(current_ts),
                                                idx == operations.size());
                std::cout << "BATCH[" << batch_idx << "] ts=" << current_ts
                          << " edges=" << edge_count
                          << " update=" << update_time << "s"
                          << " read=" << read_time << "s" << std::endl;
                timing_log << batch_idx << "," << edge_count << ","
                           << update_time << "," << read_time << std::endl;

                // ---- Dump PMA of first mismatched vertex ----
                std::string mis_file = "mismatch_after_ts_" + std::to_string(current_ts) + ".txt";
                std::ifstream mf(mis_file);
                if (mf.is_open()) {
                    std::string line;
                    int first_vtx = -1;
                    while (std::getline(mf, line)) {
                        if (line.rfind("MISMATCH AT VERTEX ", 0) == 0) {
                            first_vtx = std::stoi(line.substr(20));
                            break;
                        }
                    }
                    mf.close();
                    if (first_vtx >= 0) {
                        int pma_idx = first_vtx / VERTEX_PER_PMA;
                        int ch = pma_idx % CHANNEL_NUM;

                        // Sync HBM + vecinfo back from device
        for (int ch = 0; ch < CHANNEL_NUM; ++ch) { ch_bufs[ch].sync(XCL_BO_SYNC_BO_TO_DEVICE); }

                        VEC_INFO vi = vec_info_ptrs[ch][pma_idx];
                        int start = (int)vi.range(31, 0);
                        int len   = (int)vi.range(63, 32);
                        std::cout << "=== DUMP PMA " << pma_idx << " (ch=" << ch
                                  << ") after ts=" << current_ts
                                  << " start=" << start << " len=" << len << " ===" << std::endl;
                        print_pma(pma_idx, vec_info_ptrs[ch], hbm_base_addresses[ch]);
                        std::cout << "=== END PMA " << pma_idx << " ===" << std::endl;
                    }
                }
                batch_idx++;
            }
        }
    }

    std::cout << "Waiting for long-running kernels to finish..." << std::endl;
    run_update_message_router_kernel.wait();
    for (auto &r : run_pma_insert_edge_kernels)
    {
        r.wait();
    }
    // for (auto &r : run_pma_insert_vertex_kernels)
    // {
    //     r.wait();
    // }

    std::cout << "GRAPH UPDATED. total_time=" << (base_phase_s + incremental_phase_s)
              << " s" << std::endl;

    if (!Graph.empty())
    // {
    //     verify_graph(Graph, max_vertex_idx, "Final_Full_Graph");
    // }
    // else
    // {
    //     std::cout << "Full Graph verification skipped (no vertices)." << std::endl;
    // }

    std::cout << "Waiting service kernels to drain and exit by finish protocol..." << std::endl;
    // alloc_access & mem_access removed
    run_pma_read_router_kernel.wait();
    for (auto &run_worker : run_pma_read_worker_kernels)
    {
        run_worker.wait();
    }
    run_pma_read_merger_kernel.wait();

    std::cout << "Dumping device PMA state (vecinfo + HBM) ..." << std::endl;
    for (int ch = 0; ch < CHANNEL_NUM; ++ch) {
        ch_bufs[ch].sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    }
    // all buffers synced via insert_bufs[].rw above

    std::cout << "First 100 vecinfo entries (owner channel + value):" << std::endl;
    int printed_pma = 0;
    for (int pma_idx = 0; pma_idx < 100; ++pma_idx)
    {
        int ch = pma_idx % CHANNEL_NUM;
        VEC_INFO vec = vec_info_ptrs[ch][pma_idx];
        int start = vec.range(31, 0);
        int len = vec.range(63, 32);
        std::cout << "PMA " << pma_idx
                  << " (ch=" << ch << ") vecinfo start=" << start
                  << " len=" << len << std::endl;

        if (vec != 0)
        {
            print_pma(pma_idx, vec_info_ptrs[ch], hbm_base_addresses[ch]);
            printed_pma++;
        }
    }

    std::cout << "\nVecInfo cross-channel view (first 32 PMA idx):" << std::endl;
    for (int pma_idx = 0; pma_idx < 32; ++pma_idx)
    {
        std::cout << "PMA " << pma_idx << ": ";
        for (int ch = 0; ch < CHANNEL_NUM; ++ch)
        {
            VEC_INFO vec = vec_info_ptrs[ch][pma_idx];
            int start = vec.range(31, 0);
            int len = vec.range(63, 32);
            std::cout << "[ch=" << ch << " start=" << start << " len=" << len << "]";
            if (ch != CHANNEL_NUM - 1)
            {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
    }

    std::cout << "Printed PMA payload for " << printed_pma
              << " non-zero vecinfo entries (within first 100 PMAs)." << std::endl;
    if (final_match)
    {
        std::cout << "FINAL RESULT: MATCH!" << std::endl;
    }
    else
    {
        std::cout << "FINAL RESULT: MISMATCH!" << std::endl;
    }
    return 0;
}