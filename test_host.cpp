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
    std::vector<std::unordered_set<int>> Graph;
    int a,b;
    std::vector<std::pair<int,int>> edges;
    edges.resize(64);
    Graph.resize(64);
    int max_vertex = 64;
    for (int i = 0 ; i < 64 ; i++)
    {
        edges[i] = {i, (i+1)%64};
        Graph[i].insert((i+1)%64);
    }
    // First, calculate max_vertex from base edges (static edges)

    // Then, calculate new vertices from incremental edges
    // Debug: Print edges read from the file
    // for (const auto& edge : edges) {
    //     std::cout << edge.first << " -> " << edge.second << std::endl;
    // }

    // // Debug: Print max_vertex value
    // std::cout << "Max vertex: " << max_vertex << std::endl;
    auto xclbin_name = argv[1];
    auto xclbin = xrt::xclbin(xclbin_name);
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin(xclbin);
    int buffer_len = 1000;
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
    auto signal_kernel = xrt::kernel(device, uuid, "signal_kernel");
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
    auto run_signal_kernel = xrt::run(signal_kernel);
    std::cout << "Kernels are created..." << std::endl;

    auto bo_hbm_base_addr = xrt::bo(device, sizeof(BYTE) * CHANNEL_CAPACITY * CHANNEL_NUM, pma_insert_vertex_1.group_id(2));
    // auto bo_valid_array = xrt::bo(device, sizeof(BYTE) * (CHANNEL_CAPACITY * CHANNEL_NUM / 8), allocate_kernel.group_id(0));


    BYTE* hbm_base_address = bo_hbm_base_addr.map<BYTE*>();
    // BYTE* valid_array = bo_valid_array.map<BYTE*>();

    // 初始化 HBM 和有效性数组
    // std::fill_n(hbm_base_address, CHANNEL_CAPACITY * CHANNEL_NUM, -2);

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
    // Additional BOs for check kernel (COO/CSR outputs and metadata)
    auto bo_coo_src = xrt::bo(device, sizeof(int) * buffer_len, check_kernel.group_id(3));
    auto bo_coo_dst = xrt::bo(device, sizeof(int) * buffer_len, check_kernel.group_id(4));
    auto bo_row_ptr = xrt::bo(device, sizeof(int) * (max_vertex + 10), check_kernel.group_id(5));
    auto bo_col_idx = xrt::bo(device, sizeof(int) * buffer_len, check_kernel.group_id(6));
    auto bo_num_vertices = xrt::bo(device, sizeof(int), check_kernel.group_id(7));
    auto bo_num_edges = xrt::bo(device, sizeof(int), check_kernel.group_id(8));

    auto coo_src_ptr = bo_coo_src.map<int*>();
    auto coo_dst_ptr = bo_coo_dst.map<int*>();
    auto row_ptr_ptr = bo_row_ptr.map<int*>();
    auto col_idx_ptr = bo_col_idx.map<int*>();
    auto num_vertices_ptr = bo_num_vertices.map<int*>();
    auto num_edges_ptr = bo_num_edges.map<int*>();

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
    run_pma_insert_edge_1.set_arg(2, bo_hbm_base_addr);

    run_pma_insert_edge_2.set_arg(0, bo_pos_vec_info);
    run_pma_insert_edge_2.set_arg(1, bo_neg_vec_info );
    run_pma_insert_edge_2.set_arg(2, bo_hbm_base_addr);
    bo_pos_vec_info.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_neg_vec_info.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_hbm_base_addr.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::cout << "Buffers synchronized to device." << std::endl;

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

    auto run_phase = [&](size_t edge_start_idx, size_t edge_count, int new_vertex_count, const std::string& label, bool last) -> long long {
        if (edge_count == 0 && new_vertex_count == 0) {
            std::cout << label << ": vertices=0, edges=0, time=0 ms" << std::endl;
            return 0;
        }

        load_edges(edge_start_idx, edge_count);

        run_update_message_generator_kernel.set_arg(0, new_vertex_count);
        run_update_message_generator_kernel.set_arg(1, static_cast<int>(edge_count));
        run_update_message_generator_kernel.set_arg(2, bo_src);
        run_update_message_generator_kernel.set_arg(3, bo_dst);
        // set the `last` flag for update_message_generator kernel (0 for not-last, 1 for last batch)
        run_update_message_generator_kernel.set_arg(4, static_cast<int>(last));

        auto phase_start = std::chrono::high_resolution_clock::now();

        // Only start the generator per phase; router and insert kernels are persistent and started once before phases
        run_update_message_generator_kernel.start();

        // wait for generator to finish sending this batch
        run_update_message_generator_kernel.wait();

        // If this is the final batch, wait for router and insert kernels to finish (stop's propagated via last==1)
        if (last) {
            run_update_message_router_kernel.wait();
            run_pma_insert_edge_1.wait();
            run_pma_insert_edge_2.wait();
            run_pma_insert_vertex_1.wait();
            run_pma_insert_vertex_2.wait();
        }

        auto phase_end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(phase_end - phase_start).count();
        std::cout << label << ": vertices=" << new_vertex_count << ", edges=" << edge_count
                  << ", time=" << elapsed << " ms" << std::endl;
        return elapsed;
    };

    run_alloc_free_kernel.start();
    run_app_mem_middle_ware_kernel.start();

    // Start router and PMA insert kernels once so they remain online; stop timing is determined by last==1
    run_pma_insert_vertex_1.start();
    run_pma_insert_vertex_2.start();
    run_pma_insert_edge_1.start();
    run_pma_insert_edge_2.start();
    run_update_message_router_kernel.start();

    // Split into two batches: first batch inserts edges [0..31] and 32 new vertices,
    // second (final) batch inserts edges [32..63] and the remaining 32 new vertices
    run_phase(0, 32, 32, "batch 0-31", false);
    run_phase(32, 32, 32, "batch 32-63", true);

    std::vector<std::unordered_set<int>> Updated_Graph(max_vertex + 1);
    for (int i = 0 ; i < max_vertex + 1; i++)
    {
        check_src[i] = i;
    }
    std::fill(check_dst, check_dst + buffer_len, -1);
    // sync inputs & outputs to device
    bo_check_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_check_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_vertex_attr.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_coo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_coo_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_row_ptr.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_col_idx.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_num_vertices.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_num_edges.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    // set kernel args (match check_kernel signature)
    run_check_kernel.set_arg(0, bo_check_src);       // int *src
    run_check_kernel.set_arg(1, bo_check_dst);       // int *dst
    run_check_kernel.set_arg(2, bo_vertex_attr);     // double *vertex_attr
    run_check_kernel.set_arg(3, bo_coo_src);        // int *coo_src
    run_check_kernel.set_arg(4, bo_coo_dst);        // int *coo_dst
    run_check_kernel.set_arg(5, bo_row_ptr);        // int *row_ptr
    run_check_kernel.set_arg(6, bo_col_idx);        // int *col_idx
    run_check_kernel.set_arg(7, bo_num_vertices);   // int *num_vertices
    run_check_kernel.set_arg(8, bo_num_edges);      // int *num_edges
    run_check_kernel.set_arg(9, max_vertex + 1);    // vertex_to_check
    run_check_kernel.set_arg(10, 0);                // attr_vertex_to_check
    run_check_kernel.set_arg(11, 0);                // mode (0: check)
    run_check_kernel.set_arg(12, 1);                // last: set 1 to signal read kernel to stop after this check

    // start read kernel and perform 3 checks: COO (mode=1), CSR (mode=2), final CHECK (mode=0, last=1)
    run_pma_read_kernel.start();

    // --- 1) COO check (mode=1), do not stop read kernel (last=0) ---
    std::fill(coo_src_ptr, coo_src_ptr + buffer_len, -1);
    std::fill(coo_dst_ptr, coo_dst_ptr + buffer_len, -1);
    num_edges_ptr[0] = 0;
    bo_coo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_coo_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_num_edges.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    run_check_kernel.set_arg(11, 1); // COO
    run_check_kernel.set_arg(12, 0); // last=0 -> keep read kernel online
    run_check_kernel.start();
    run_check_kernel.wait();

    bo_coo_src.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    bo_coo_dst.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    bo_num_edges.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    int coo_num_edges = num_edges_ptr[0];
    std::cout << "COO check finished, num_edges=" << coo_num_edges << std::endl;
    // dump COO
    std::fstream coo_file_run("coo_output_run.txt", std::ios::out);
    for (int e = 0; e < coo_num_edges && e < buffer_len; ++e) coo_file_run << coo_src_ptr[e] << " " << coo_dst_ptr[e] << "\n";
    coo_file_run.close();

    // --- 2) CSR check (mode=2), do not stop read kernel (last=0) ---
    std::fill(row_ptr_ptr, row_ptr_ptr + (max_vertex + 10), -1);
    std::fill(col_idx_ptr, col_idx_ptr + buffer_len, -1);
    num_vertices_ptr[0] = 0;
    num_edges_ptr[0] = 0;
    bo_row_ptr.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_col_idx.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_num_vertices.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_num_edges.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    run_check_kernel.set_arg(11, 2); // CSR
    run_check_kernel.set_arg(12, 0); // last=0
    run_check_kernel.start();
    run_check_kernel.wait();

    bo_row_ptr.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    bo_col_idx.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    bo_num_vertices.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    bo_num_edges.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    int csr_num_vertices = num_vertices_ptr[0];
    int csr_num_edges = num_edges_ptr[0];
    std::cout << "CSR check finished, num_vertices=" << csr_num_vertices << " num_edges=" << csr_num_edges << std::endl;
    // dump CSR
    std::fstream csr_file_run("csr_output_run.txt", std::ios::out);
    csr_file_run << "num_vertices " << csr_num_vertices << " num_edges " << csr_num_edges << "\n";
    for (int i = 0; i <= csr_num_vertices && i < (max_vertex + 10); ++i) csr_file_run << row_ptr_ptr[i] << " ";
    csr_file_run << "\n";
    for (int i = 0; i < csr_num_edges && i < buffer_len; ++i) csr_file_run << col_idx_ptr[i] << " ";
    csr_file_run << std::endl;
    csr_file_run.close();

    // --- 3) Final full check (mode=0) and stop read kernel (last=1) ---
    std::fill(check_dst, check_dst + buffer_len, -1);
    run_check_kernel.set_arg(11, 0); // check mode
    run_check_kernel.set_arg(12, 1); // last=1 -> signal read kernel to stop
    run_check_kernel.start();
    run_check_kernel.wait();

    // wait for read kernel to finish after last==1
    run_pma_read_kernel.wait();

    // retrieve outputs of the final check
    bo_check_dst.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    bo_num_vertices.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    bo_num_edges.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    std::cout << "Final check kernel finished." << std::endl;
    // sync COO/CSR buffers for final inspection
    bo_coo_src.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    bo_coo_dst.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    bo_row_ptr.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    bo_col_idx.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    // Read COO/CSR metadata
    int num_vertices = num_vertices_ptr[0];
    int num_edges = num_edges_ptr[0];
    std::cout << "COO/CSR outputs (final): num_vertices=" << num_vertices << " num_edges=" << num_edges << std::endl;
    int idx = 0;
    for (int i = 0; i < max_vertex + 1; i++) 
    {
        while (idx < buffer_len && check_dst[idx] != -1) {
            Updated_Graph[i].insert(check_dst[idx]);
            idx++;
        }
        idx++; // 跳过分隔符 -1
    }
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

    // --- Validate COO format ---
    std::vector<std::pair<int,int>> coo_edges_from_device;
    coo_edges_from_device.reserve(std::max(0, num_edges));
    for (int e = 0; e < num_edges && e < buffer_len; ++e) {
        int s = coo_src_ptr[e];
        int d = coo_dst_ptr[e];
        coo_edges_from_device.emplace_back(s, d);
    }
    std::vector<std::unordered_set<int>> coo_graph(num_vertices);
    for (auto &pr : coo_edges_from_device) {
        if (pr.first >= 0 && pr.first < (int)coo_graph.size())
            coo_graph[pr.first].insert(pr.second);
    }
    bool coo_ok = true;
    for (size_t v = 0; v < Updated_Graph.size() && v < coo_graph.size(); ++v) {
        if (Updated_Graph[v] != coo_graph[v]) {
            coo_ok = false;
            output_file << "COO MISMATCH AT VERTEX " << v << std::endl;
        }
    }
    if (coo_ok) std::cout << "COO output matches Updated_Graph." << std::endl;
    else std::cout << "COO output mismatch detected (see updated_graph.txt)." << std::endl;

    // dump COO to file for inspection
    std::fstream coo_file("coo_output.txt", std::ios::out);
    for (auto &pr : coo_edges_from_device) {
        coo_file << pr.first << " " << pr.second << "\n";
    }
    coo_file.close();

    // --- Validate CSR format ---
    std::vector<int> row_ptr(num_vertices + 1, 0);
    for (int i = 0; i <= num_vertices && i < (max_vertex + 10); ++i) {
        row_ptr[i] = row_ptr_ptr[i];
    }
    std::vector<int> col_idx(num_edges);
    for (int i = 0; i < num_edges && i < buffer_len; ++i) col_idx[i] = col_idx_ptr[i];
    std::vector<std::unordered_set<int>> csr_graph(num_vertices);
    for (int v = 0; v < num_vertices; ++v) {
        int s = row_ptr[v];
        int e = (v + 1 <= num_vertices) ? row_ptr[v + 1] : s;
        if (s < 0) s = 0;
        if (e < s) e = s;
        for (int idx2 = s; idx2 < e && idx2 < (int)col_idx.size(); ++idx2) {
            csr_graph[v].insert(col_idx[idx2]);
        }
    }
    bool csr_ok = true;
    for (size_t v = 0; v < Updated_Graph.size() && v < csr_graph.size(); ++v) {
        if (Updated_Graph[v] != csr_graph[v]) {
            csr_ok = false;
            output_file << "CSR MISMATCH AT VERTEX " << v << std::endl;
        }
    }
    if (csr_ok) std::cout << "CSR output matches Updated_Graph." << std::endl;
    else std::cout << "CSR output mismatch detected (see updated_graph.txt)." << std::endl;

    // dump CSR to file
    std::fstream csr_file("csr_output.txt", std::ios::out);
    csr_file << "num_vertices " << num_vertices << " num_edges " << num_edges << "\n";
    csr_file << "row_ptr:\n";
    for (int i = 0; i <= num_vertices; ++i) row_ptr[i] ? csr_file << row_ptr[i] << " " : csr_file << row_ptr[i] << " ";
    csr_file << "\ncol_idx:\n";
    for (int i = 0; i < num_edges; ++i) csr_file << col_idx[i] << " ";
    csr_file << std::endl;
    csr_file.close();
    run_signal_kernel.set_arg(1, MEM_STOP);
    run_signal_kernel.start();
    run_signal_kernel.wait();
    std::cout << "Stop signal sent to memory kernel." <<  std::endl;

    run_alloc_free_kernel.wait();
    run_app_mem_middle_ware_kernel.wait();
    return 0;
}