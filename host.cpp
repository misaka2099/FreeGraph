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
    //这一部分是在构建验证的答案
    while (graph_file >> a >> b)
    {
        edges.emplace_back(a,b);
        if (a > max_vertex) max_vertex = a;
        if (b > max_vertex) max_vertex = b;
    }
    
    Graph.resize(max_vertex + 1);
    for (size_t i = 0; i < edges.size(); ++i) {
        Graph[edges[i].first].insert(edges[i].second);
    }
    int buffer_len = edges.size() + (max_vertex + 1);
    //
    graph_file.close();

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(edges.begin(), edges.end(), g);

    // 将边拆分为"基线构图阶段"和"增量插入阶段"
    const double incremental_fraction = 0.01; // 默认增量插入占 1%
    size_t incremental_edges = edges.empty() ? 0 : std::max<size_t>(1, static_cast<size_t>(edges.size() * incremental_fraction));
    if (incremental_edges > edges.size()) {
        incremental_edges = edges.size();
    }
    size_t base_edges = edges.size() >= incremental_edges ? edges.size() - incremental_edges : 0;

    // First, calculate max_vertex from base edges (static edges)
    int static_max_vertex = -1;
    for (size_t i = 0; i < base_edges; ++i) {
        if (edges[i].first > static_max_vertex) static_max_vertex = edges[i].first;
        if (edges[i].second > static_max_vertex) static_max_vertex = edges[i].second;
    }

    // Then, calculate new vertices from incremental edges
    std::unordered_set<int> base_vertices;
    for (int i = 0; i <= static_max_vertex; ++i) base_vertices.insert(i);
    std::unordered_set<int> incremental_vertices;
    for (size_t i = base_edges; i < edges.size(); ++i) {
        incremental_vertices.insert(edges[i].first);
        incremental_vertices.insert(edges[i].second);
    }
    std::vector<int> new_vertices;
    for (int v : incremental_vertices) {
        if (base_vertices.find(v) == base_vertices.end()) {
            new_vertices.push_back(v);
        }
    }
    int incremental_new_vertex_count = max_vertex - static_max_vertex;

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
    // auto memalloc_kernel = xrt::kernel(device, uuid, "memalloc_kernel");
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
    auto signal_kernel = xrt::kernel(device, uuid, "signal_kernel");
    std::cout << "Kernels loaded successfully." << std::endl;
    // auto run_allocate_kernel = xrt::run(allocate_kernel);
    // auto run_free_kernel = xrt::run(free_kernel);
    auto run_alloc_free_kernel = xrt::run(alloc_free_kernel);
    auto run_app_mem_middle_ware_kernel = xrt::run(app_mem_middle_ware_kernel);
    // auto run_memalloc_kernel = xrt::run(memalloc_kernel);
    auto run_pma_read_kernel = xrt::run(pma_read_kernel);
    auto run_update_message_generator_kernel = xrt::run(update_message_generator_kernel);
    auto run_update_message_router_kernel = xrt::run(update_message_router_kernel);
    auto run_pma_insert_vertex_1 = xrt::run(pma_insert_vertex_1);
    auto run_pma_insert_vertex_2 = xrt::run(pma_insert_vertex_2);
    auto run_pma_insert_edge_1 = xrt::run(pma_insert_edge_1);
    auto run_pma_insert_edge_2 = xrt::run(pma_insert_edge_2);
    auto run_check_kernel = xrt::run(check_kernel);
    auto run_signal_kernel = xrt::run(signal_kernel);
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
    printf("edge_size %d\n", int(edges.size()));
    auto bo_src = xrt::bo(device, sizeof(int) * edges.size(), update_message_generator_kernel.group_id(2));
    auto bo_dst = xrt::bo(device, sizeof(int) * edges.size(), update_message_generator_kernel.group_id(3));

    int* src_ptr = bo_src.map<int*>();
    int* dst_ptr = bo_dst.map<int*>();

    // 边数据按阶段写入，初始不填充

    auto bo_check_src = xrt::bo(device, sizeof(int) * (max_vertex + 10), check_kernel.group_id(0));
    auto bo_check_dst = xrt::bo(device, sizeof(int) * buffer_len, check_kernel.group_id(1));
    auto check_src = bo_check_src.map<int*>();
    auto check_dst = bo_check_dst.map<int*>();
    auto bo_vertex_attr = xrt::bo(device, sizeof(double) *  (max_vertex + 10), check_kernel.group_id(2));
    auto bo_bitmap = xrt::bo(device, sizeof(BYTE) * (CHANNEL_CAPACITY * CHANNEL_NUM / 8), alloc_free_kernel.group_id(3));
    auto bitmap_ptr = bo_bitmap.map<BYTE*>();
    std::fill_n(bitmap_ptr, CHANNEL_CAPACITY * CHANNEL_NUM / 8 / INIT_VEC_LEN_IN_BYTE, 0);
    std::cout << "Buffers allocated and initialized." << std::endl;

    // run_allocate_kernel.set_arg(0, bo_valid_array);
    // run_free_kernel.set_arg(0, bo_valid_array);
    run_pma_read_kernel.set_arg(0, bo_pos_vec_info);
    run_pma_read_kernel.set_arg(1, bo_neg_vec_info);
    run_pma_read_kernel.set_arg(2, bo_hbm_base_addr);
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

    run_alloc_free_kernel.set_arg(3, bo_bitmap);
    // run_pma_insert_edge_2.set_arg(5, bo_hbm_base_addr);
    // run_pma_insert_edge_2.set_arg(6, bo_hbm_base_addr);
    // run_pma_insert_edge_2.set_arg(7, bo_hbm_base_addr);
    // run_pma_insert_edge_2.set_arg(8, bo_hbm_base_addr);

    auto add_base_graph_to_hbm = [&](int vertex_num, int edge_num) ->void
    {
        std::vector<ap_uint<64>> sorted_edges;
        std::vector<std::vector<int>> adj_pos(vertex_num+1); // src -> dst
        std::vector<std::vector<int>> adj_neg(vertex_num+1); // dst -> src

        for (size_t i = 0; i < edge_num; ++i) {
            int u = edges[i].first;
            int v = edges[i].second;
            if (u < vertex_num && v < vertex_num) {
                adj_pos[u].push_back(v);
                adj_neg[v].push_back(u);
            }
        }

        // 2. 对邻居进行排序
        for(auto& neighbors : adj_pos) std::sort(neighbors.begin(), neighbors.end());
        for(auto& neighbors : adj_neg) std::sort(neighbors.begin(), neighbors.end());

        int current_addr_offset = 0;

        // 3. 构建 PMA 结构
        for (int pma_idx = 0; pma_idx < (vertex_num + VERTEX_PER_PMA - 1) / VERTEX_PER_PMA; ++pma_idx) 
        {
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
                memcpy(hbm_base_address + current_addr_offset, &pos_header, sizeof(PMA_HEADER));
                memcpy(hbm_base_address + current_addr_offset + sizeof(PMA_HEADER), pos_data.data(), pos_data.size() * sizeof(int));
                
                int total_len = sizeof(PMA_HEADER) + pos_data.size() * sizeof(int);
                pos_vec_info_ptr[pma_idx] = (ap_uint<64>(total_len) << 32) | current_addr_offset;
                
                current_addr_offset += total_len;
                // Align to 64 bytes
                if (current_addr_offset % 64 != 0) current_addr_offset += (64 - (current_addr_offset % 64));
            }

            // NEG Direction
            {
                PMA_HEADER neg_header = {0};
                std::vector<int> neg_data;
                
                for (int i = 0 ; i < VERTEX_PER_PMA; i++)
                {
                    int src = i + pma_idx * VERTEX_PER_PMA;
                    std::vector<int> neighbors;
                    if (src < vertex_num) {
                        neighbors = adj_neg[src];
                    }

                    neg_header.edge_count[i] = neighbors.size();
                    neg_header.attr[i] = 0;

                    int last_pos_range = i == 0 ? 0 : neg_header.vertex_range[i-1];
                    int total_segments = 1;
                    if (neighbors.size() > 0) {
                        total_segments = (neighbors.size() + SEGMENT_SIZE * DENSITY_BOUND - 1) / (SEGMENT_SIZE * DENSITY_BOUND);
                    }
                    neg_header.vertex_range[i] = last_pos_range + total_segments;

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
                            neg_data.push_back(neighbors[neighbor_idx++]);
                        }
                        for (int k = valid_num; k < SEGMENT_SIZE; ++k) {
                            neg_data.push_back(UNUSED_ICON);
                        }
                    }
                }
                
                // Write to HBM
                memcpy(hbm_base_address + current_addr_offset, &neg_header, sizeof(PMA_HEADER));
                memcpy(hbm_base_address + current_addr_offset + sizeof(PMA_HEADER), neg_data.data(), neg_data.size() * sizeof(int));
                
                int total_len = sizeof(PMA_HEADER) + neg_data.size() * sizeof(int);
                neg_vec_info_ptr[pma_idx] = (ap_uint<64>(total_len) << 32) | current_addr_offset;
                
                current_addr_offset += total_len;
                // Align to 64 bytes
                if (current_addr_offset % 64 != 0) current_addr_offset += (64 - (current_addr_offset % 64));
            }
        }
        // Update bitmap
        int total_blocks = (current_addr_offset + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
        int full_bytes = total_blocks / 8;
        int remaining_bits = total_blocks % 8;
        memset(bitmap_ptr, 0xFF, full_bytes);
        
        if (remaining_bits > 0) {
            bitmap_ptr[full_bytes] |= (BYTE)((1 << remaining_bits) - 1);
        }
        bo_bitmap.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        
        run_signal_kernel.set_arg(1, LOAD_BITMAP);
        run_signal_kernel.start();
        run_signal_kernel.wait();
    };

    // bo_valid_array.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::cout << "Buffers synchronized to device." << std::endl;

    std::cout << "Static vertex count: " << (static_max_vertex + 1) << std::endl;
    std::cout << "Static edge count: " << base_edges << std::endl;
    std::cout << "Incremental new vertex count: " << incremental_new_vertex_count << std::endl;
    std::cout << "Incremental edge count: " << incremental_edges << std::endl;
    std::cout << "Total max vertex after update: " << (max_vertex + 1) << std::endl;

    auto load_edges = [&](size_t start_idx, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            src_ptr[i] = edges[start_idx + i].first;
            dst_ptr[i] = edges[start_idx + i].second;
        }
        if (count > 0) {
            bo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
            bo_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        }
    };

    auto run_phase = [&](size_t edge_start_idx, size_t edge_count, int new_vertex_count, const std::string& label) -> long long {
        if (edge_count == 0 && new_vertex_count == 0) {
            std::cout << label << ": vertices=0, edges=0, time=0 ms" << std::endl;
            return 0;
        }

        load_edges(edge_start_idx, edge_count);

        run_update_message_generator_kernel.set_arg(0, new_vertex_count);
        run_update_message_generator_kernel.set_arg(1, static_cast<int>(edge_count));
        run_update_message_generator_kernel.set_arg(2, bo_src);
        run_update_message_generator_kernel.set_arg(3, bo_dst);

        auto phase_start = std::chrono::high_resolution_clock::now();

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

        auto phase_end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(phase_end - phase_start).count();
        std::cout << label << ": vertices=" << new_vertex_count << ", edges=" << edge_count
                  << ", time=" << elapsed << " ms" << std::endl;
        return elapsed;
    };


    run_alloc_free_kernel.start();
    run_app_mem_middle_ware_kernel.start();
    // run_memalloc_kernel.start();
    
    add_base_graph_to_hbm(static_max_vertex + 1, base_edges);
    
    bo_pos_vec_info.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_neg_vec_info.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_hbm_base_addr.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_bitmap.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    // long long base_phase_ms = run_phase(0, base_edges, static_max_vertex + 1, "Base graph build");
    long long base_phase_ms = 0;
    long long incremental_phase_ms = run_phase(base_edges, incremental_edges, incremental_new_vertex_count, "Incremental edge insert");
    std::cout << "GRAPH UPDATED. total_time=" << (base_phase_ms + incremental_phase_ms)
              << " ms" << std::endl;

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
    run_check_kernel.set_arg(2, bo_vertex_attr);
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
    // run_stop_kernel.start();
    // run_stop_kernel.wait();
    run_signal_kernel.set_arg(1, MEM_STOP);
    run_signal_kernel.start();
    run_signal_kernel.wait();
    std::cout << "Stop signal sent to memory kernel." <<  std::endl;
    std::cout << "update edges " << edges.size() << " base_time " << base_phase_ms
              << " ms incremental_time " << incremental_phase_ms << " ms total "
              << (base_phase_ms + incremental_phase_ms) << " ms" << std::endl;

    // run_allocate_kernel.wait();
    // run_free_kernel.wait();
    run_alloc_free_kernel.wait();
    run_app_mem_middle_ware_kernel.wait();
    // run_memalloc_kernel.wait();
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