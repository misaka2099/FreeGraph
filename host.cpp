#include "data_type.h"
#include "deprecated/xrt.h"
#include <chrono>
#include <cstddef>
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
    for (int i = 0 ; i < 16 ; i++)
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
    std::fstream graph_file(graph_path);
    if (!graph_file.is_open()) {
        std::cerr << "Failed to open graph file: " << graph_path << std::endl;
        return 1;
    }
    std::vector<std::unordered_set<int>> Graph;
    int a,b;
    std::vector<std::pair<int,int>> edges;
    int max_vertex = -1;
    while (graph_file >> a >> b)
    {
        edges.emplace_back(a,b);
        if (a > max_vertex) max_vertex = a;
        if (b > max_vertex) max_vertex = b;
    }
    graph_file.close();
    Graph.resize(max_vertex + 1);
    for (auto& edge : edges)
    {
        Graph[edge.first].insert(edge.second);
    }
    int buffer_len = edges.size() + (max_vertex + 1);

    // std::random_device rd;
    // std::mt19937 g(rd());
    // std::shuffle(edges.begin(), edges.end(), g);
    // Debug: Print edges read from the file
    std::cout << "Edges read from the file:" << std::endl;
    // for (const auto& edge : edges) {
    //     std::cout << edge.first << " -> " << edge.second << std::endl;
    // }

    // // Debug: Print max_vertex value
    // std::cout << "Max vertex: " << max_vertex << std::endl;

    auto xclbin = xrt::xclbin(xclbin_name);
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin(xclbin);

    // auto allocate_kernel = xrt::kernel(device, uuid, "allocate_kernel");
    // auto free_kernel = xrt::kernel(device, uuid, "free_kernel");
    auto alloc_free_kernel = xrt::kernel(device, uuid, "alloc_free_kernel");
    auto app_mem_middle_ware_kernel = xrt::kernel(device, uuid, "app_mem_middle_ware");
    auto pma_read_kernel = xrt::kernel(device, uuid, "pma_read_kernel");
    auto update_message_generator_kernel = xrt::kernel(device, uuid, "update_message_generator");
    auto update_message_router_kernel = xrt::kernel(device, uuid, "update_message_router");
    auto pma_insert_vertex_1 = xrt::kernel(device, uuid, "pma_insert_vertex:{pma_insert_vertex_1}");
    auto pma_insert_vertex_2 = xrt::kernel(device, uuid, "pma_insert_vertex:{pma_insert_vertex_2}");
    auto pma_insert_edge_1 = xrt::kernel(device, uuid, "pma_insert_edge:{pma_insert_edge_1}");
    auto pma_insert_edge_2 = xrt::kernel(device, uuid, "pma_insert_edge:{pma_insert_edge_2}");
    auto check_kernel = xrt::kernel(device, uuid, "check_kernel");
    
    auto stop_kernel = xrt::kernel(device, uuid, "stop_kernel");
    std::cout << "Kernels loaded successfully." << std::endl;
    // auto run_allocate_kernel = xrt::run(allocate_kernel);
    // auto run_free_kernel = xrt::run(free_kernel);
    auto run_alloc_free_kernel = xrt::run(alloc_free_kernel);
    auto run_app_mem_middle_ware_kernel = xrt::run(app_mem_middle_ware_kernel);
    auto run_pma_read_kernel = xrt::run(pma_read_kernel);
    auto run_update_message_generator_kernel = xrt::run(update_message_generator_kernel);
    auto run_update_message_router_kernel = xrt::run(update_message_router_kernel);
    auto run_pma_insert_vertex_1 = xrt::run(pma_insert_vertex_1);
    auto run_pma_insert_vertex_2 = xrt::run(pma_insert_vertex_2);
    auto run_pma_insert_edge_1 = xrt::run(pma_insert_edge_1);
    auto run_pma_insert_edge_2 = xrt::run(pma_insert_edge_2);
    auto run_check_kernel = xrt::run(check_kernel);
    auto run_stop_kernel = xrt::run(stop_kernel);
    std::cout << "Kernels are created..." << std::endl;

    auto bo_hbm_base_addr = xrt::bo(device, sizeof(BYTE) * CHANNEL_CAPACITY * CHANNEL_NUM, pma_insert_vertex_1.group_id(2));
    // auto bo_valid_array = xrt::bo(device, sizeof(BYTE) * (CHANNEL_CAPACITY * CHANNEL_NUM / 8), allocate_kernel.group_id(0));


    BYTE* hbm_base_address = bo_hbm_base_addr.map<BYTE*>();
    // BYTE* valid_array = bo_valid_array.map<BYTE*>();

    // 初始化 HBM 和有效性数组
    std::fill_n(hbm_base_address, CHANNEL_CAPACITY * CHANNEL_NUM, -2);

    // std::fill_n(valid_array, CHANNEL_CAPACITY / 8, 0);

    auto bo_pos_vec_info = xrt::bo(device, sizeof(VEC_INFO) * MAX_PMA_NUM, pma_read_kernel.group_id(0));
    auto bo_neg_vec_info = xrt::bo(device, sizeof(VEC_INFO) * MAX_PMA_NUM, pma_read_kernel.group_id(1));
    auto pos_vec_info_ptr = bo_pos_vec_info.map<VEC_INFO*>();
    auto neg_vec_info_ptr = bo_neg_vec_info.map<VEC_INFO*>();
    std::fill_n(pos_vec_info_ptr, MAX_PMA_NUM, 0);
    std::fill_n(neg_vec_info_ptr, MAX_PMA_NUM, 0);

    // 修改 bo_src 和 bo_dst 的分配，确保使用 update_message_generator_kernel 的 group_id
    auto bo_src = xrt::bo(device, sizeof(int) * edges.size(), update_message_generator_kernel.group_id(2));
    auto bo_dst = xrt::bo(device, sizeof(int) * edges.size(), update_message_generator_kernel.group_id(3));

    int* src_ptr = bo_src.map<int*>();
    int* dst_ptr = bo_dst.map<int*>();

    int index = 0;
    for (const auto& edge : edges) {
        src_ptr[index] = edge.first;
        dst_ptr[index] = edge.second;
        index++;
    }

    auto bo_check_src = xrt::bo(device, sizeof(int) * (max_vertex + 10), check_kernel.group_id(0));
    auto bo_check_dst = xrt::bo(device, sizeof(int) * buffer_len, check_kernel.group_id(1));
    auto check_src = bo_check_src.map<int*>();
    auto check_dst = bo_check_dst.map<int*>();
    auto bo_vertex_attr = xrt::bo(device, sizeof(double) *  (max_vertex + 10), check_kernel.group_id(2));

    std::cout << "Buffers allocated and initialized." << std::endl;

    // run_allocate_kernel.set_arg(0, bo_valid_array);
    // run_free_kernel.set_arg(0, bo_valid_array);
    run_pma_read_kernel.set_arg(0, bo_pos_vec_info);
    run_pma_read_kernel.set_arg(1, bo_neg_vec_info);
    run_pma_read_kernel.set_arg(2, bo_hbm_base_addr);
    run_update_message_generator_kernel.set_arg(0, max_vertex+1);
    run_update_message_generator_kernel.set_arg(1, edges.size());
    run_update_message_generator_kernel.set_arg(2,bo_src);
    run_update_message_generator_kernel.set_arg(3,bo_dst);
    run_pma_insert_vertex_1.set_arg(0, bo_pos_vec_info);
    run_pma_insert_vertex_1.set_arg(1, bo_neg_vec_info);
    run_pma_insert_vertex_1.set_arg(2, bo_hbm_base_addr);
    run_pma_insert_vertex_2.set_arg(0, bo_pos_vec_info);
    run_pma_insert_vertex_2.set_arg(1, bo_neg_vec_info);
    run_pma_insert_vertex_2.set_arg(2, bo_hbm_base_addr);
    
    run_pma_insert_edge_1.set_arg(0, bo_pos_vec_info);
    run_pma_insert_edge_1.set_arg(1, bo_neg_vec_info);
    // run_pma_insert_edge_1.set_arg(2, bo_neg_vec_info);
    // run_pma_insert_edge_1.set_arg(3, bo_neg_vec_info);
    run_pma_insert_edge_1.set_arg(2, bo_hbm_base_addr);
    // run_pma_insert_edge_1.set_arg(5, bo_hbm_base_addr);
    // run_pma_insert_edge_1.set_arg(6, bo_hbm_base_addr);
    // run_pma_insert_edge_1.set_arg(7, bo_hbm_base_addr);
    // run_pma_insert_edge_1.set_arg(8, bo_hbm_base_addr);

    run_pma_insert_edge_2.set_arg(0, bo_pos_vec_info);
    run_pma_insert_edge_2.set_arg(1, bo_neg_vec_info );
    // run_pma_insert_edge_2.set_arg(2, bo_neg_vec_info);
    // run_pma_insert_edge_2.set_arg(3, bo_neg_vec_info);
    run_pma_insert_edge_2.set_arg(2, bo_hbm_base_addr);
    // run_pma_insert_edge_2.set_arg(5, bo_hbm_base_addr);
    // run_pma_insert_edge_2.set_arg(6, bo_hbm_base_addr);
    // run_pma_insert_edge_2.set_arg(7, bo_hbm_base_addr);
    // run_pma_insert_edge_2.set_arg(8, bo_hbm_base_addr);
    // bo_valid_array.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_pos_vec_info.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_neg_vec_info.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_hbm_base_addr.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::cout << "Buffers synchronized to device." << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();
    // run_allocate_kernel.start();
    // run_free_kernel.start();
    run_alloc_free_kernel.start();
    run_app_mem_middle_ware_kernel.start();
    run_pma_insert_vertex_1.start();
    run_pma_insert_vertex_2.start();
    run_pma_insert_edge_1.start();
    run_pma_insert_edge_2.start();
    run_update_message_router_kernel.start();
    run_update_message_generator_kernel.start();
    run_update_message_generator_kernel.wait();
    run_update_message_router_kernel.wait();
    run_pma_insert_edge_1.wait();
    run_pma_insert_edge_2.wait();
    run_pma_insert_vertex_1.wait();
    run_pma_insert_vertex_2.wait();
    auto end_time = std::chrono::high_resolution_clock::now();
    std::cout << "GRAPH UPDATED." << std::endl;

    std::vector<std::unordered_set<int>> Updated_Graph(max_vertex + 1);
    for (int i = 0 ; i < max_vertex + 1; i++)
    {
        check_src[i] = i;
        
        // std::cout << std::endl;
    }
    std::fill(check_dst, check_dst + buffer_len, -1);
    bo_check_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_check_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    run_check_kernel.set_arg(0, bo_check_src);
    run_check_kernel.set_arg(1, bo_check_dst);
    run_check_kernel.set_arg(3, max_vertex + 1);
    run_check_kernel.set_arg(4, 0);
    run_pma_read_kernel.start();
    run_check_kernel.start();
    run_check_kernel.wait();
    run_pma_read_kernel.wait();
    bo_check_dst.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    std::cout << "Check kernel finished." << std::endl;
    int idx = 0;
    for (int i = 0; i < max_vertex + 1; i++) 
    {
        while (idx < buffer_len && check_dst[idx] != -1) {
            Updated_Graph[i].insert(check_dst[idx]);
            idx++;
        }
        idx++; // 跳过分隔符 -1
    }
    // Compare Updated_Graph with the original Graph
    std::fstream output_file("updated_graph.txt", std::ios::out);
    if (!output_file.is_open()) {
        std::cerr << "Failed to open output file for updated graph." << std::endl;
        return 1;
    }
    bool all_match = true;
    for (size_t i = 0; i < Graph.size(); ++i) {
        if (Graph[i] != Updated_Graph[i]) {
            std::cout << "Mismatch found at vertex " << i << std::endl;
            output_file << "MISMATCH AT VERTEX " << i << std::endl;
            for (auto v : Graph[i])
            {
                output_file << "GRAPH EDGE: " << i << " -> " << v << std::endl;
            }
            for (auto v : Updated_Graph[i])
            {
                output_file << "UPDATED GRAPH EDGE: " << i << " -> " << v << std::endl;
            }
            all_match = false;
        }

    }
    if (all_match) {
        std::cout << "Graphs match perfectly!" << std::endl;
    } else {
        std::cout << "Graphs do not match. See updated_graph.txt for details." << std::endl;
    }
    run_stop_kernel.start();
    run_stop_kernel.wait();
    std::cout << "Stop signal sent to memory kernel." <<  std::endl;
    std::cout << "update edges " << edges.size() << " in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() 
              << " ms" << std::endl;

    // run_allocate_kernel.wait();
    // run_free_kernel.wait();
    run_alloc_free_kernel.wait();
    run_app_mem_middle_ware_kernel.wait();
    // bo_hbm_base_addr.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    // bo_pos_vec_info.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    // bo_neg_vec_info.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    // BYTE* storage = bo_hbm_base_addr.map<BYTE*>();
    // auto pos_info = bo_pos_vec_info.map<VEC_INFO*>();
    // auto neg_info = bo_neg_vec_info.map<VEC_INFO*>();
    // std::cout << "----------------POS_DIRECTION-----------------" << std::endl;
    // for (int i = 0 ; i < (max_vertex + VERTEX_PER_PMA - 1)/ VERTEX_PER_PMA ; i++)
    // {
    //     print_pma(i, pos_info, storage);
    // }
    // std::cout << "----------------NEG_DIRECTION-----------------" <<std::endl;
    //     for (int i = 0 ; i < (max_vertex + VERTEX_PER_PMA - 1)/ VERTEX_PER_PMA ; i++)
    // {
    //     print_pma(i, neg_info, storage);
    // }
    return 0;
}