#include "data_type.h"
#include "deprecated/xrt.h"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <xrt/xrt_device.h>
#include <xrt/xrt_kernel.h>
#include <xrt/xrt_bo.h>
#include <unordered_set>
#include <fstream>
#include <random>

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
    for (int i = 0 ; i < CHANNEL_NUM ; i++)
    {
        int src = pma_idx * 16  + i;
        int edge_count = header.edge_count[i];
        int seg_start = i == 0 ? 0 : header.vertex_range[i-1];
        int seg_end = header.vertex_range[i];
        std::cout << "vertex: " << src << " has " << edge_count << " neighbors" << std::endl;
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
        std::cerr << "Usage: " << argv[0] << " <xclbin> <graph>" << std::endl;
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

    std::vector<Operation> operations;
    int a = -1, b = -1, op = PMA_INSERT_EDGE;
    long long ts = 0;

    // 输入格式: src dst timestamp opcode
    while (graph_file >> a >> b >> ts >> op)
    {
        operations.push_back({a, b, ts, op});
    }
    graph_file.close();

    // // 拆分操作: 顶点插入单独计数，其余操作进入消息队列
    std::vector<Operation> edge_ops;
    size_t vertex_insert_count = 0;
    for (const auto &opr : operations)
    {
        if (opr.opcode == PMA_INSERT_VERTEX)
        {
            vertex_insert_count++;
        }
        else
        {
            edge_ops.push_back(opr);
        }
    }

    // 基于操作序列构建期望图，用于后验验证
    std::vector<std::vector<int>> Graph;
    auto ensure_vertex = [&](int v)
    {
        if (v < 0) return;
        if (v >= static_cast<int>(Graph.size()))
        {
            Graph.resize(v + 1);
        }
    };
    int full_edge_count = 0;
    for (const auto &opr : operations)
    {
        switch (opr.opcode)
        {
            case PMA_INSERT_VERTEX:
                // 插入顶点不需要 src/dst，但如果提供了则确保容量
                if (opr.src >= 0)
                {
                    ensure_vertex(opr.src);
                }
                else if (opr.dst >= 0)
                {
                    ensure_vertex(opr.dst);
                }
                else
                {
                    Graph.resize(Graph.size() + 1);
                }
                break;
            case PMA_INSERT_EDGE:
                ensure_vertex(opr.src);
                ensure_vertex(opr.dst);
                Graph[opr.src].push_back(opr.dst);
                full_edge_count++;
                break;
            case PMA_DELETE_EDGE:
                ensure_vertex(opr.src);
                ensure_vertex(opr.dst);
                if (opr.src < static_cast<int>(Graph.size()))
                {
                    auto &nbrs = Graph[opr.src];
                    nbrs.erase(std::remove(nbrs.begin(), nbrs.end(), opr.dst), nbrs.end());
                }
                full_edge_count--;
                break;
            case PMA_DELETE_VERTEX:
                // 删除顶点需要指定 src (或 dst)，两者取其一
                if (opr.src >= 0)
                {
                    ensure_vertex(opr.src);
                    if (opr.src < static_cast<int>(Graph.size()))
                    {
                        Graph[opr.src].clear();
                    }
                    // for (auto &nbrs : Graph)
                    // {
                    //     nbrs.erase(std::remove(nbrs.begin(), nbrs.end(), opr.src), nbrs.end());
                    // }
                }
                break;
            default:
                break;
        }
    }

    for (auto &neighbors : Graph) {
        std::sort(neighbors.begin(), neighbors.end());
        neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
    }

    int max_vertex_idx = static_cast<int>(Graph.size()) - 1;
    // Avoid O(V^2) host/device allocation. Verification buffers only need to cover
    // expected COO/CSR payload scale, which is bounded by operation/edge volume.
    size_t buffer_len = std::max<size_t>(1, edge_ops.size() * 2 + Graph.size() + 1024);
    int static_max_vertex = max_vertex_idx;
    // BaseGraph置空，所有操作都作为增量在FPGA上执行
    std::vector<std::vector<int>> BaseGraph;

    std::cout << "Operations read from the file: " << operations.size() << std::endl;

    auto end_init = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_init = end_init - start_init;
    std::cout << "Initialization (File Read & Graph Build) time: " << elapsed_init.count() << " s" << std::endl;

    auto xclbin = xrt::xclbin(xclbin_name);
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin(xclbin);

    // auto alloc_access_kernel_1 = xrt::kernel(device, uuid, "alloc_access_kernel:{alloc_access_kernel_1}");
    // auto alloc_access_kernel_2 = xrt::kernel(device, uuid, "alloc_access_kernel:{alloc_access_kernel_2}");
    // xrt::kernel alloc_access_kernels[CHANNEL_NUM] = {
    //     alloc_access_kernel_1,
    //     alloc_access_kernel_2
    // };
    std::vector<xrt::kernel> alloc_access_kernels;
    alloc_access_kernels.reserve(CHANNEL_NUM);
    for (int i = 1; i <= CHANNEL_NUM; ++i)
    {
        try {
            alloc_access_kernels.emplace_back(device, uuid, "alloc_access_kernel:{alloc_access_kernel_" + std::to_string(i) + "}");
        } catch (const std::runtime_error &e) {
            std::string msg = e.what();
            if (msg.find("No compute units matching") != std::string::npos) {
                break;
            }
        }
    }
    // auto mem_access_kernel_1 = xrt::kernel(device, uuid, "mem_access_kernel:{mem_access_kernel_1}");
    // auto mem_access_kernel_2 = xrt::kernel(device, uuid, "mem_access_kernel:{mem_access_kernel_2}");
    // xrt::kernel mem_access_kernels[CHANNEL_NUM] = {
    //     mem_access_kernel_1,
    //     mem_access_kernel_2
    // };
    std::vector<xrt::kernel> mem_access_kernels;
    mem_access_kernels.reserve(CHANNEL_NUM);
    for (int i = 1; i <= CHANNEL_NUM; ++i)
    {
        try {
            mem_access_kernels.emplace_back(device, uuid, "mem_access_kernel:{mem_access_kernel_" + std::to_string(i) + "}");
        } catch (const std::runtime_error &e) {
            std::string msg = e.what();
            if (msg.find("No compute units matching") != std::string::npos) {
                break;
            }
        }
    }
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

    std::vector<xrt::kernel> pma_insert_vertex_kernels;
    pma_insert_vertex_kernels.reserve(CHANNEL_NUM);
    for (int i = 1; i <= CHANNEL_NUM; ++i)
    {
        if (!try_add_kernel(pma_insert_vertex_kernels, "pma_insert_vertex:{pma_insert_vertex_" + std::to_string(i) + "}")) {
            break;
        }
    }
    std::vector<xrt::kernel> pma_insert_edge_kernels;
    pma_insert_edge_kernels.reserve(CHANNEL_NUM);
    for (int i = 1; i <= CHANNEL_NUM; ++i)
    {
        if (!try_add_kernel(pma_insert_edge_kernels, "pma_insert_edge:{pma_insert_edge_" + std::to_string(i) + "}")) {
            break;
        }
    }
    auto check_kernel = xrt::kernel(device, uuid, "check_kernel");
    auto signal_kernel = xrt::kernel(device, uuid, "signal_kernel:{signal_kernel_1}");
    std::cout << "Kernels loaded successfully." << std::endl;
    std::cout << "Loaded alloc kernels: " << alloc_access_kernels.size()
              << ", mem kernels: " << mem_access_kernels.size() << std::endl;
    if (alloc_access_kernels.size() < static_cast<size_t>(CHANNEL_NUM) ||
        mem_access_kernels.size() < static_cast<size_t>(CHANNEL_NUM))
    {
        std::cerr << "Kernel loading incomplete; aborting before launch to avoid out-of-range access." << std::endl;
        return 1;
    }
    std::vector<xrt::run> run_alloc_kernels;
    run_alloc_kernels.reserve(CHANNEL_NUM);
    std::vector<xrt::run> run_mem_access_kernels;
    run_mem_access_kernels.reserve(CHANNEL_NUM);
    std::cout << "Launching kernels..." << std::endl;
    for (size_t i = 0; i < static_cast<size_t>(CHANNEL_NUM); ++i)
    {
        std::cout << "LOOP " << i << std::endl;
        run_alloc_kernels.emplace_back(alloc_access_kernels[i]);
        run_mem_access_kernels.emplace_back(mem_access_kernels[i]);
    }
    std::cout << "Memory access kernels are launched..." << std::endl;
    // auto run_alloc_access_kernel = xrt::run(alloc_access_kernel_1);
    // auto run_alloc_access_kernel_2 = xrt::run(alloc_access_kernel_2);
    // auto run_mem_access_kernel_1 = xrt::run(mem_access_kernel_1);
    // auto run_mem_access_kernel_2 = xrt::run(mem_access_kernel_2);
    auto run_pma_read_router_kernel = xrt::run(pma_read_router_kernel);
    std::vector<xrt::run> run_pma_read_worker_kernels;
    run_pma_read_worker_kernels.reserve(pma_read_worker_kernels.size());
    for (auto &k : pma_read_worker_kernels)
    {
        run_pma_read_worker_kernels.emplace_back(k);
    }
    auto run_pma_read_merger_kernel = xrt::run(pma_read_merger_kernel);
    std::cout << "Memory access kernels are launched..." << std::endl;
    auto run_update_message_router_kernel = xrt::run(update_message_router_kernel);
    std::cout << "Update message router kernel is launched..." << std::endl;
    std::vector<xrt::run> run_pma_insert_vertex_kernels;
    run_pma_insert_vertex_kernels.reserve(pma_insert_vertex_kernels.size());

    for (auto &k : pma_insert_vertex_kernels)
    {
        run_pma_insert_vertex_kernels.emplace_back(k);
    }
    std::cout << "PMA insert vertex kernels are launched..." << std::endl;
    std::vector<xrt::run> run_pma_insert_edge_kernels;
    run_pma_insert_edge_kernels.reserve(pma_insert_edge_kernels.size());
    for (auto &k : pma_insert_edge_kernels)
    {
        run_pma_insert_edge_kernels.emplace_back(k);
    }
    std::cout << "PMA insert edge kernels are launched..." << std::endl;
    auto run_signal_kernel = xrt::run(signal_kernel);
    std::cout << "Kernels are created..." << std::endl;

    auto start_buffer = std::chrono::high_resolution_clock::now();

    std::vector<xrt::bo> bo_hbm_base_addr;
    std::vector<xrt::bo> bo_vec_info;
    bo_hbm_base_addr.reserve(CHANNEL_NUM);
    bo_vec_info.reserve(CHANNEL_NUM);
    for (int i = 0 ; i < CHANNEL_NUM; ++i)
    {
        bo_hbm_base_addr.emplace_back(device, sizeof(BYTE) * (size_t)CHANNEL_CAPACITY, mem_access_kernels[i].group_id(0));
        bo_vec_info.emplace_back(device, sizeof(VEC_INFO) * MAX_PMA_NUM,
                                 mem_access_kernels[i].group_id(1));
    }
    // auto bo_valid_array = xrt::bo(device, sizeof(BYTE) * ((size_t)CHANNEL_CAPACITY * CHANNEL_NUM / 8), allocate_kernel.group_id(0));


    std::vector<BYTE*> hbm_base_addresses;
    hbm_base_addresses.reserve(CHANNEL_NUM);
    for (auto &bo : bo_hbm_base_addr)
    {
        hbm_base_addresses.push_back(bo.map<BYTE*>());
    }
    // BYTE* valid_array = bo_valid_array.map<BYTE*>();

    // 初始化 HBM 和有效性数组
    // std::fill_n(hbm_base_address, CHANNEL_CAPACITY * CHANNEL_NUM, -2);

    // std::fill_n(valid_array, CHANNEL_CAPACITY / 8, 0);

    // auto bo_pos_vec_info = xrt::bo(device, sizeof(VEC_INFO) * MAX_PMA_NUM, mem_access_kernel_1.group_id(CHANNEL_NUM));
    // auto pos_vec_info_ptr = bo_pos_vec_info.map<VEC_INFO*>();
    std::vector<VEC_INFO*> vec_info_ptrs;
    vec_info_ptrs.reserve(CHANNEL_NUM);
    for (auto &bo : bo_vec_info)
    {
        vec_info_ptrs.push_back(bo.map<VEC_INFO*>());
        std::fill_n(vec_info_ptrs.back(), MAX_PMA_NUM, 0);
    }

    // 修改 bo_src 和 bo_dst 的分配，确保使用 update_message_generator_kernel 的 group_id
    size_t edge_buffer_len = std::max<size_t>(edge_ops.size(), 1);
    printf("edge_size %zu\n", edge_ops.size());
    auto bo_src = xrt::bo(device, sizeof(int) * edge_buffer_len, update_message_generator_kernel.group_id(4));
    auto bo_dst = xrt::bo(device, sizeof(int) * edge_buffer_len, update_message_generator_kernel.group_id(5));
    auto bo_opcode = xrt::bo(device, sizeof(int) * edge_buffer_len, update_message_generator_kernel.group_id(6));

    int* src_ptr = bo_src.map<int*>();
    int* dst_ptr = bo_dst.map<int*>();
    int* opcode_ptr = bo_opcode.map<int*>();

    // 边数据按阶段写入，初始不填充

    auto bo_check_src = xrt::bo(device, sizeof(int) * std::max<int>(max_vertex_idx + 10, 1), check_kernel.group_id(0));
    auto bo_check_dst = xrt::bo(device, sizeof(int) * buffer_len, check_kernel.group_id(1));
    auto check_src = bo_check_src.map<int*>();
    auto check_dst = bo_check_dst.map<int*>();

    auto bo_coo_src = xrt::bo(device, sizeof(int) * buffer_len, check_kernel.group_id(2));
    auto bo_coo_dst = xrt::bo(device, sizeof(int) * buffer_len, check_kernel.group_id(3));
    int* coo_src_ptr = bo_coo_src.map<int*>();
    int* coo_dst_ptr = bo_coo_dst.map<int*>();
    auto bo_row_ptr = xrt::bo(device, sizeof(int) * std::max<int>(max_vertex_idx + 10, 1), check_kernel.group_id(4));
    auto bo_col_idx = xrt::bo(device, sizeof(int) * buffer_len, check_kernel.group_id(5));
    auto bo_num_vertices = xrt::bo(device, sizeof(int), check_kernel.group_id(6));
    auto bo_num_edges = xrt::bo(device, sizeof(int), check_kernel.group_id(7));

    // Map num vertices/edges so host can read returned counts from check_kernel
    int* bo_num_vertices_ptr = bo_num_vertices.map<int*>();
    int* bo_num_edges_ptr = bo_num_edges.map<int*>();

    std::vector<xrt::bo> bo_bitmap;
    std::vector<BYTE*> bitmap_ptrs;
    bo_bitmap.reserve(CHANNEL_NUM);
    bitmap_ptrs.reserve(CHANNEL_NUM);
    for (int i = 0; i < CHANNEL_NUM; ++i)
    {
      bo_bitmap.emplace_back(device,
                             sizeof(BYTE) *
                                 ((size_t)CHANNEL_CAPACITY * CHANNEL_NUM /
                                  INIT_VEC_LEN_IN_BYTE / 8),
                             alloc_access_kernels[i].group_id(0));
      auto bitmap_ptr = bo_bitmap.back().map<BYTE *>();
      bitmap_ptrs.push_back(bitmap_ptr);
      std::fill_n(
          bitmap_ptr,
          (size_t)CHANNEL_CAPACITY * CHANNEL_NUM / 8 / INIT_VEC_LEN_IN_BYTE, 0);
    }

    auto end_buffer = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_buffer = end_buffer - start_buffer;
    std::cout << "Buffer allocation and initialization time: " << elapsed_buffer.count() << " s" << std::endl;

    std::cout << "Buffers allocated and initialized." << std::endl;

    // Mem Access Kernel Args
    for (int i = 0; i < CHANNEL_NUM; ++i)
    {
        // run_mem_access_kernel_1.set_arg(i, bo_hbm_base_addr[i]);
        run_mem_access_kernels[i].set_arg(0, bo_hbm_base_addr[i]);
        run_mem_access_kernels[i].set_arg(1, bo_vec_info[i]);
    }
    // run_mem_access_kernel_1.set_arg(CHANNEL_NUM, bo_pos_vec_info);

    for (int i = 0; i < CHANNEL_NUM; ++i)
    {
        // run_alloc_access_kernel.set_arg(i, bo_bitmap[i]);
        run_alloc_kernels[i].set_arg(0, bo_bitmap[i]);
    }

    auto verify_graph = [&](const std::vector<std::vector<int>>& expected_graph, int current_max_vertex, const std::string& label) {
        auto start_verify = std::chrono::high_resolution_clock::now();
        std::cout << "Verifying " << label << "..." << std::endl;
        std::vector<std::vector<int>> Updated_Graph(current_max_vertex + 1);
        
        for (int i = 0 ; i < current_max_vertex + 1; i++)
        {
            check_src[i] = i;
        }
        std::fill(check_dst, check_dst + buffer_len, -1);
        
        bo_check_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        bo_check_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        
        auto run_check_kernel = xrt::run(check_kernel);
        run_check_kernel.set_arg(0, bo_check_src);
        run_check_kernel.set_arg(1, bo_check_dst);
        run_check_kernel.set_arg(2, bo_coo_src);
        run_check_kernel.set_arg(3, bo_coo_dst);
        run_check_kernel.set_arg(4, bo_row_ptr);
        run_check_kernel.set_arg(5, bo_col_idx);
        run_check_kernel.set_arg(6, bo_num_vertices);
        run_check_kernel.set_arg(7, bo_num_edges);
        run_check_kernel.set_arg(8, current_max_vertex + 1);
        // run_check_kernel.set_arg(9, 1); // COO mode for read kernel test
        run_check_kernel.set_arg(9, 3); 
        run_check_kernel.set_arg(10, 0);
        
        for (int i = 0; i < static_cast<int>(run_pma_read_worker_kernels.size()); ++i)
        {
            run_pma_read_worker_kernels[i].set_arg(0, i);
        }

        for (auto &run_worker : run_pma_read_worker_kernels)
        {
            run_worker.start();
        }
        run_pma_read_merger_kernel.start();
        run_pma_read_router_kernel.start();
        run_check_kernel.start();
        run_check_kernel.wait();

        bo_num_edges.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        int edge_num = bo_num_edges_ptr[0];
        std::cout << "Check kernel returned edge count: " << edge_num << std::endl;

        run_check_kernel.set_arg(9, 1); 
        run_check_kernel.set_arg(10, 1);
        run_check_kernel.start();
        run_check_kernel.wait();
        run_pma_read_router_kernel.wait();
        for (auto &run_worker : run_pma_read_worker_kernels)
        {
            run_worker.wait();
        }
        run_pma_read_merger_kernel.wait();

        // Retrieve edge count and COO arrays written back by check_kernel
        bo_num_edges.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        bo_coo_src.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        bo_coo_dst.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        // int returned_edge_count = bo_num_edges_ptr[0];

        // std::cout << "Check kernel finished for " << label << ". Returned edge count: " << returned_edge_count << std::endl;

        // Build Updated_Graph from COO arrays
        for (int i = 0; i <= current_max_vertex; ++i) Updated_Graph[i].clear();
        for (int id = 0;id < static_cast<int>(buffer_len); ++id) {
            int s = coo_src_ptr[id];
            int d = coo_dst_ptr[id];
            // 以src或dst为-1作为分割终止
            if (s == -1 || d == -1) break;
            if (s < 0 || s > current_max_vertex) continue;
            Updated_Graph[s].push_back(d);
        }
        for (int i = 0; i <= current_max_vertex; ++i) {
            std::sort(Updated_Graph[i].begin(), Updated_Graph[i].end());
            Updated_Graph[i].erase(std::unique(Updated_Graph[i].begin(), Updated_Graph[i].end()), Updated_Graph[i].end());
        }
        
        std::string clean_label = label;
        std::replace(clean_label.begin(), clean_label.end(), ' ', '_');
        std::ofstream output_file("mismatch_" + clean_label + ".txt");
        
        bool all_match = true;
        for (int i = 0; i <= current_max_vertex; ++i) {
             const std::vector<int>& expected_neighbors = (i < expected_graph.size()) ? expected_graph[i] : std::vector<int>();
             
            if (expected_neighbors != Updated_Graph[i]) {
                output_file << "MISMATCH AT VERTEX " << i << std::endl;
                for (auto v : expected_neighbors)
                {
                    output_file << "EXPECTED EDGE: " << i << " -> " << v << std::endl;
                }
                for (auto v : Updated_Graph[i])
                {
                    output_file << "ACTUAL EDGE: " << i << " -> " << v << std::endl;
                }
                all_match = false;
            }
        }
        
        if (all_match) {
            std::cout << label << " matches perfectly!" << std::endl;
        } else {
            std::cout << label << " does not match. See mismatch_" << clean_label << ".txt for details." << std::endl;
        }
        auto end_verify = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_verify = end_verify - start_verify;
        std::cout << "Verification time for " << label << ": " << elapsed_verify.count() << " s" << std::endl;
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
                    if (neighbors.size() > 0) {
                        total_segments = (neighbors.size() + SEGMENT_SIZE * DENSITY_BOUND - 1) / (SEGMENT_SIZE * DENSITY_BOUND);
                        int power_of_2 = 1;
                        while (power_of_2 < total_segments) {
                            power_of_2 *= 2;
                        }
                        total_segments = power_of_2;
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
                        for (int k = valid_num; k < SEGMENT_SIZE; ++k) {
                            pos_data.push_back(UNUSED_ICON);
                        }
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
        for (int i = 0 ; i < 10 ; i++)
        {
            int corresponding_channel = i % CHANNEL_NUM;
            if (vec_info_ptrs[i % CHANNEL_NUM][i] == 0) break;
            int start = vec_info_ptrs[i % CHANNEL_NUM][i].range(31,0);
            int len =  vec_info_ptrs[i % CHANNEL_NUM][i].range(63,32);
            std::cout << "PMA " << i << " ADDR START FROM " << start << " LEN " << len << std::endl; 
            PMA_HEADER header;
            memcpy(&header, hbm_base_addresses[corresponding_channel] + start, sizeof(PMA_HEADER));
            for (int src = 0 ; src < VERTEX_PER_PMA; src++)
            {
                int edge_count = header.edge_count[src];
                int seg_start = src == 0 ? 0 : header.vertex_range[src-1];
                int seg_end = header.vertex_range[src];
                std::cout << "vertex: " << src << " has " << edge_count << " neighbors" << std::endl;
                for (int j = seg_start ; j < seg_end ; j ++)
                {
                    for (int k = 0 ; k < SEGMENT_SIZE ; k++)
                    {
                        int neighbor = *((int*)(hbm_base_addresses[corresponding_channel] + start + sizeof(PMA_HEADER) + (j * SEGMENT_SIZE + k) * sizeof(int)));
                        if (neighbor == UNUSED_ICON)
                        {
                            break;
                        }
                        std::cout << neighbor << " ";
                    }
                }
            }
                std::cout << std::endl;
        }
        //
        // Update bitmap
        for (int ch = 0; ch < CHANNEL_NUM; ++ch)
        {
            int total_blocks = (current_addr_offset[ch] + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
            int full_bytes = total_blocks / 8;
            int rem_bits = total_blocks % 8;
            memset(bitmap_ptrs[ch], 0xFF, full_bytes);
            if (rem_bits > 0)
            {
                bitmap_ptrs[ch][full_bytes] = (BYTE)((1 << rem_bits) - 1);
            }
        }

        auto end_cpu_preprocess = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_cpu_preprocess = end_cpu_preprocess - start_cpu_preprocess;
        std::cout << "CPU preprocessing time in add_base_graph_to_hbm: " << elapsed_cpu_preprocess.count() << " s" << std::endl;

        for (auto &bo : bo_bitmap)
        {
            bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        }
        std::cout << "base bitmap inserted to device." << std::endl;
        
        run_signal_kernel.set_arg(0, LOAD_BITMAP);
        run_signal_kernel.start();
        run_signal_kernel.wait();
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
            if (op.opcode == PMA_INSERT_VERTEX)
            {
                continue;
            }
            if (edge_idx >= edge_buffer_len)
            {
                std::cerr << "Edge buffer overflow in batch load." << std::endl;
                return -1;
            }
            src_ptr[edge_idx] = op.src;
            dst_ptr[edge_idx] = op.dst;
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

    auto run_update_batch = [&](int new_vertex_count, int edge_count, const std::string& label, bool last) -> double {
        if (edge_count == 0 && new_vertex_count == 0) {
            std::cout << label << ": vertices=0, edges=0, time=0 s" << std::endl;
            return 0;
        }
        auto run_update_message_generator_kernel = xrt::run(update_message_generator_kernel);
        std::cout << "Update message generator kernel is launched..." << std::endl;
    
        run_update_message_generator_kernel.set_arg(0, new_vertex_count);                   // add_vertex_num
        run_update_message_generator_kernel.set_arg(1, edge_count);                         // add_edge_num
        run_update_message_generator_kernel.set_arg(2, 0);                                  // delete_vertex_num
        run_update_message_generator_kernel.set_arg(3, 0);                                  // delete_edge_num
        run_update_message_generator_kernel.set_arg(4, bo_src);                              // src
        run_update_message_generator_kernel.set_arg(5, bo_dst);                              // dst
        run_update_message_generator_kernel.set_arg(6, bo_opcode);                           // opcode
        run_update_message_generator_kernel.set_arg(7, static_cast<int>(last));             // last flag

        auto phase_start = std::chrono::high_resolution_clock::now();

        // Router and Consumer kernels are already running (Keep Alive).
        // Only start the Generator for this batch.
        run_update_message_generator_kernel.start();

        // Generator waits for ACK from Router internally, so when it finishes, 
        // the batch is guaranteed to be processed by downstream kernels.
        run_update_message_generator_kernel.wait();

        auto phase_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = phase_end - phase_start;
        std::cout << label << ": vertices=" << new_vertex_count << ", edges=" << edge_count
                  << ", time=" << elapsed.count() << " s" << std::endl;
        return elapsed.count();
    };

    // 1. Start Long-Running (Keep Alive) Kernels ONCE before the batch loop
    for (auto &r : run_pma_insert_vertex_kernels)
    {
        r.start();
    }
    for (auto &r : run_pma_insert_edge_kernels)
    {
        r.start();
    }
    run_update_message_router_kernel.start();


    // run_mem_access_kernel_1.start();
    // run_alloc_access_kernel.start();
    for (auto &r : run_mem_access_kernels)
    {
        r.start();
    }
    for (auto &r : run_alloc_kernels)
    {
        r.start();
    }
    // run_app_mem_middle_ware_kernel.start();
    // run_memalloc_kernel.start();
    

    if (!add_base_graph_to_hbm(static_max_vertex + 1, full_edge_count))
    {
        std::cerr << "Failed to build/load base graph into HBM due to capacity/index overflow." << std::endl;
        return 1;
    }
    std::cout << "Base graph loaded into HBM." << std::endl;
    // bo_pos_vec_info.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_opcode.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    for (auto &bo : bo_hbm_base_addr)
    {
        bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    }
    for (auto &bo : bo_vec_info)
    {
        bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    }
    for (auto &bo : bo_bitmap)
    {
        bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    }

    std::vector<std::vector<int>> runtime_graph;
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

    for (size_t idx = 0; idx < operations.size(); )
    {
        long long current_ts = operations[idx].timestamp;
        std::vector<Operation> batch_ops;
        while (idx < operations.size() && operations[idx].timestamp == current_ts)
        {
            batch_ops.push_back(operations[idx]);
            ++idx;
        }

        if (batch_ops.size() > 1)
        {
            std::shuffle(batch_ops.begin(), batch_ops.end(), rng); // shuffle the whole operation tuple (src, dst, opcode) within same timestamp
        }

        int new_vertex_count = 0;
        for (const auto &op : batch_ops)
        {
            if (op.opcode == PMA_INSERT_VERTEX)
            {
                new_vertex_count += 1;
            }
        }

        int edge_count = load_edge_batch(batch_ops);
        if (edge_count < 0)
        {
            break;
        }

        bool last = (idx == operations.size());
        incremental_phase_s += run_update_batch(new_vertex_count, edge_count, "ts_" + std::to_string(current_ts), last);

        for (const auto &op : batch_ops)

        {
            switch (op.opcode)
            {
                case PMA_INSERT_VERTEX:
                    ensure_runtime_vertex(op.src >= 0 ? op.src : op.dst);
                    break;
                case PMA_INSERT_EDGE:
                    ensure_runtime_vertex(op.src);
                    ensure_runtime_vertex(op.dst);
                    if (op.src >= 0 && op.src < static_cast<int>(runtime_graph.size()))
                    {
                        runtime_graph[op.src].push_back(op.dst);
                        normalize_neighbors(op.src);
                    }
                    break;
                case PMA_DELETE_EDGE:
                    ensure_runtime_vertex(op.src);
                    ensure_runtime_vertex(op.dst);
                    if (op.src >= 0 && op.src < static_cast<int>(runtime_graph.size()))
                    {
                        auto &nbrs = runtime_graph[op.src];
                        nbrs.erase(std::remove(nbrs.begin(), nbrs.end(), op.dst), nbrs.end());
                    }
                    break;
                case PMA_DELETE_VERTEX:
                    ensure_runtime_vertex(op.src);
                    if (op.src >= 0 && op.src < static_cast<int>(runtime_graph.size()))
                    {
                        runtime_graph[op.src].clear();
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

        int current_max_vertex = static_cast<int>(runtime_graph.size()) - 1;
        if (!runtime_graph.empty())
        {
            verify_graph(runtime_graph, current_max_vertex, "after_ts_" + std::to_string(current_ts));
        }
    }

    std::cout << "Waiting for long-running kernels to finish..." << std::endl;
    run_update_message_router_kernel.wait();
    for (auto &r : run_pma_insert_edge_kernels)
    {
        r.wait();
    }
    for (auto &r : run_pma_insert_vertex_kernels)
    {
        r.wait();
    }

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

    run_signal_kernel.set_arg(0, MEM_STOP);
    run_signal_kernel.start();
    run_signal_kernel.wait();
    std::cout << "Stop signal sent to memory kernel." <<  std::endl;
    // run_alloc_access_kernel.wait();
    // run_mem_access_kernel_1.wait();
    for (auto &r : run_alloc_kernels)
    {
        r.wait();
    }
    for (auto &r : run_mem_access_kernels)
    {
        r.wait();
    }
    return 0;
}