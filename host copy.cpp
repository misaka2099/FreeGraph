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

bool final_match = true;
const bool kBaseGraphReadOnlyTest = false;
const bool kVertexOnlyTest = false;
const bool kShuffleSameTimestampOps = false;
const bool kMismatchfileOutput = true;

// HBM memory layout per channel (must fit within U280 256MB pseudo-channel):
//   [0, PMA_DATA_SIZE): PMA data region
//   [VECINFO_OFFSET, VECINFO_OFFSET + VECINFO_TOTAL_SIZE): max_pma_idx(64B) + vecinfo table
//   [BITMAP_OFFSET, ...): allocation bitmap
constexpr size_t PMA_DATA_SIZE      = CHANNEL_CAPACITY;
constexpr size_t VECINFO_OFFSET     = PMA_DATA_SIZE;
constexpr size_t VECINFO_TOTAL_SIZE = 64 + MAX_PMA_NUM * (int)sizeof(VEC_INFO);
constexpr size_t BITMAP_OFFSET      = VECINFO_OFFSET + VECINFO_TOTAL_SIZE;
constexpr size_t BITMAP_SIZE        = (PMA_DATA_SIZE / INIT_VEC_LEN_IN_BYTE + 7) / 8;
constexpr size_t TOTAL_CHANNEL_SIZE = BITMAP_OFFSET + BITMAP_SIZE;

void print_pma(int pma_idx, VEC_INFO* pos_info, BYTE* storage)
{
    VEC_INFO vec_info = pos_info[pma_idx];
    int start = vec_info.range(31, 0);
    int len   = vec_info.range(63, 32);
    std::cout << "PMA " << pma_idx << " ADDR START FROM " << start << " LEN " << len << std::endl;
    PMA_HEADER header;
    memcpy(&header, storage + start, sizeof(PMA_HEADER));
    int total_segs = header.vertex_range[VERTEX_PER_PMA - 1];
    int hlp_words = hlp_total_words(total_segs);
    int* pma_data = (int*)(storage + sizeof(PMA_HEADER) + hlp_words * 64 + start);
    for (int i = 0; i < VERTEX_PER_PMA; i++) {
        int src = pma_idx * VERTEX_PER_PMA + i;
        int edge_count = header.edge_count[i];
        int seg_start  = (i == 0) ? 0 : header.vertex_range[i - 1];
        int seg_end    = header.vertex_range[i];
        std::cout << "vertex: " << src << " has " << edge_count << " neighbors" << std::endl;
        std::cout << "vertex " << src << " range from " << seg_start << " to " << seg_end << std::endl;
        for (int j = seg_start; j < seg_end; j++) {
            for (int k = 0; k < SEG_INT_SIZE; k++) {
                std::cout << pma_data[j * SEG_INT_SIZE + k] << " ";
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
    std::string graph_path  = argv[2];
    auto start_init = std::chrono::high_resolution_clock::now();

    std::fstream graph_file(graph_path);
    if (!graph_file.is_open()) {
        std::cerr << "Failed to open graph file: " << graph_path << std::endl;
        return 1;
    }

    struct Operation { int src; int dst; long long timestamp; int opcode; };
    std::vector<Operation> operations;
    int a = -1, b = -1, op = PMA_UPDATE_INSERT_EDGE;
    long long ts = 0;
    while (graph_file >> a >> b >> ts >> op)
        operations.push_back({a, b, ts, op});
    graph_file.close();

    // Build expected graph for verification
    std::vector<std::vector<int>> Graph;
    auto ensure_vertex = [&](int v) {
        if (v < 0) return;
        if (v >= static_cast<int>(Graph.size())) Graph.resize(v + 1);
    };
    int full_edge_count = 0;
    for (const auto& opr : operations) {
        switch (opr.opcode) {
            case PMA_UPDATE_INSERT_EDGE:
                ensure_vertex(opr.src); ensure_vertex(opr.dst);
                Graph[opr.src].push_back(opr.dst);
                full_edge_count++;
                break;
            case PMA_UPDATE_DELETE_EDGE:
                ensure_vertex(opr.src); ensure_vertex(opr.dst);
                if (opr.src < static_cast<int>(Graph.size())) {
                    auto& nbrs = Graph[opr.src];
                    nbrs.erase(std::remove(nbrs.begin(), nbrs.end(), opr.dst), nbrs.end());
                }
                full_edge_count--;
                break;
            case PMA_UPDATE_DELETE_VERTEX:
                if (opr.src >= 0) {
                    ensure_vertex(opr.src);
                    if (opr.src < static_cast<int>(Graph.size())) Graph[opr.src].clear();
                }
                break;
            default: break;
        }
    }
    for (auto& neighbors : Graph) {
        std::sort(neighbors.begin(), neighbors.end());
        neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
    }

    int max_vertex_idx = static_cast<int>(Graph.size()) - 1;
    size_t buffer_len = std::max<size_t>(1, operations.size() * 2 + Graph.size() + 1024);
    int static_max_vertex = max_vertex_idx;
    std::vector<std::vector<int>> BaseGraph = Graph;

    // Max edge batch
    size_t max_edge_batch = 0, current_edge_batch = 0;
    long long current_edge_ts = 0;
    bool first_edge_batch = true;
    for (const auto& op : operations) {
        if (first_edge_batch || op.timestamp != current_edge_ts) {
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

    // =========================================================================
    // Load xclbin and kernels
    // =========================================================================
    auto xclbin = xrt::xclbin(xclbin_name);
    auto device = xrt::device(0);
    auto uuid   = device.load_xclbin(xclbin);
    std::cout << "device name: " << device.get_info<xrt::info::device::name>() << "\n";
    std::cout << "device bdf:  " << device.get_info<xrt::info::device::bdf>() << "\n";

    auto try_add_kernel = [&](std::vector<xrt::kernel>& kernels, const std::string& name) -> bool {
        try {
            kernels.emplace_back(device, uuid, name);
            return true;
        } catch (const std::runtime_error& e) {
            std::string msg = e.what();
            if (msg.find("No compute units matching") != std::string::npos) return false;
            throw;
        }
    };

    auto update_message_generator_kernel = xrt::kernel(device, uuid, "update_message_generator:{update_message_generator_1}");
    auto update_message_router_kernel    = xrt::kernel(device, uuid, "update_message_router:{update_message_router_1}");
    auto bs_kernel       = xrt::kernel(device, uuid, "bs_kernel:{bs_kernel_1}");
    auto insert_kernel   = xrt::kernel(device, uuid, "insert_kernel:{insert_kernel_1}");
    auto rebalance_kernel = xrt::kernel(device, uuid, "rebalance_kernel:{rebalance_kernel_1}");
    auto pma_read_kernel = xrt::kernel(device, uuid, "pma_read_kernel:{pma_read_kernel_1}");
    auto check_kernel    = xrt::kernel(device, uuid, "check_kernel:{check_kernel_1}");
    std::cout << "Kernels loaded successfully." << std::endl;

    // =========================================================================
    // Allocate HBM BOs for PMA data + vecinfo + bitmap (2 channels)
    // =========================================================================
    auto start_buffer = std::chrono::high_resolution_clock::now();

    auto bo_hbm_0 = xrt::bo(device, TOTAL_CHANNEL_SIZE, bs_kernel.group_id(2));  // hbm_ptr_0 → HBM[0]
    auto bo_hbm_1 = xrt::bo(device, TOTAL_CHANNEL_SIZE, bs_kernel.group_id(3));  // hbm_ptr_1 → HBM[1]
    BYTE* hbm_ptr_0 = bo_hbm_0.map<BYTE*>();
    BYTE* hbm_ptr_1 = bo_hbm_1.map<BYTE*>();
    std::fill_n(hbm_ptr_0, TOTAL_CHANNEL_SIZE, 0);
    std::fill_n(hbm_ptr_1, TOTAL_CHANNEL_SIZE, 0);

    int vec_info_base = (int)VECINFO_OFFSET;
    int bitmap_base   = (int)BITMAP_OFFSET;

    // Edge buffers for generator
    const size_t kDefaultMaxEdgeBuffer = 1000000;
    size_t edge_buffer_len = std::max<size_t>(1, std::min(max_edge_batch, kDefaultMaxEdgeBuffer));
    std::cout << "Update buffer length: " << edge_buffer_len << std::endl;

    auto bo_src    = xrt::bo(device, sizeof(int) * edge_buffer_len, update_message_generator_kernel.group_id(1));
    auto bo_dst    = xrt::bo(device, sizeof(int) * edge_buffer_len, update_message_generator_kernel.group_id(2));
    auto bo_opcode = xrt::bo(device, sizeof(int) * edge_buffer_len, update_message_generator_kernel.group_id(3));
    int* src_ptr    = bo_src.map<int*>();
    int* dst_ptr    = bo_dst.map<int*>();
    int* opcode_ptr = bo_opcode.map<int*>();

    // COO / verification buffers for check_kernel
    auto bo_coo_src  = xrt::bo(device, sizeof(int) * buffer_len, check_kernel.group_id(0));
    auto bo_coo_dst  = xrt::bo(device, sizeof(int) * buffer_len, check_kernel.group_id(1));
    auto bo_num_edges = xrt::bo(device, sizeof(int), check_kernel.group_id(2));
    int* coo_src_ptr     = bo_coo_src.map<int*>();
    int* coo_dst_ptr     = bo_coo_dst.map<int*>();
    int* bo_num_edges_ptr = bo_num_edges.map<int*>();

    auto end_buffer = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_buffer = end_buffer - start_buffer;
    std::cout << "Buffer allocation and initialization time: " << elapsed_buffer.count() << " s" << std::endl;

    // =========================================================================
    // add_base_graph_to_hbm (updated for SEG_INT_SIZE, no attr)
    // =========================================================================
    auto add_base_graph_to_hbm = [&](int vertex_num, int /*edge_num*/) -> bool {
        auto start_cpu = std::chrono::high_resolution_clock::now();
        const int total_pma_num = (vertex_num + VERTEX_PER_PMA - 1) / VERTEX_PER_PMA;
        if (total_pma_num > MAX_PMA_NUM) {
            std::cerr << "PMA count overflow: required=" << total_pma_num
                      << ", MAX_PMA_NUM=" << MAX_PMA_NUM << std::endl;
            return false;
        }
        std::vector<std::vector<int>> adj_pos(vertex_num + 1);
        if (!BaseGraph.empty()) {
            adj_pos = BaseGraph;
            if (adj_pos.size() < static_cast<size_t>(vertex_num + 1))
                adj_pos.resize(vertex_num + 1);
        }
        for (auto& neighbors : adj_pos)
            std::sort(neighbors.begin(), neighbors.end());

        int current_offset[CHANNEL_NUM] = {0, 0};
        BYTE* hbm_ptrs[CHANNEL_NUM] = {hbm_ptr_0, hbm_ptr_1};

        for (int pma_idx = 0; pma_idx < total_pma_num; pma_idx++) {
            int ch  = pma_idx % CHANNEL_NUM;
            BYTE* base = hbm_ptrs[ch];
            int* off   = &current_offset[ch];

            PMA_HEADER pos_header = {};
            std::vector<int> pos_data;

            for (int i = 0; i < VERTEX_PER_PMA; i++) {
                int src = i + pma_idx * VERTEX_PER_PMA;
                std::vector<int> neighbors;
                if (src < vertex_num) neighbors = adj_pos[src];

                pos_header.edge_count[i] = (int)neighbors.size();

                int last_range = (i == 0) ? 0 : pos_header.vertex_range[i - 1];
                int total_segs = 1;
                if (!neighbors.empty()) {
                    // capacity = segs * 12 (SEG_INT_SIZE * 0.75)
                    total_segs = ((int)neighbors.size() + 11) / 12;
                    int power_of_2 = 1;
                    while (power_of_2 < total_segs) power_of_2 *= 2;
                    total_segs = power_of_2;
                }
                pos_header.vertex_range[i] = last_range + total_segs;

                int base_num = 0, extra_num = 0;
                if (total_segs > 0) {
                    base_num  = (int)neighbors.size() / total_segs;
                    extra_num = (int)neighbors.size() % total_segs;
                }
                int nidx = 0;
                for (int s = 0; s < total_segs; s++) {
                    int valid_num = base_num + (s < extra_num ? 1 : 0);
                    for (int k = 0; k < valid_num; k++)
                        pos_data.push_back(neighbors[nidx++]);
                    for (int k = valid_num; k < SEG_INT_SIZE; k++)
                        pos_data.push_back(UNUSED_ICON);
                }
            }

            int total_len = (int)sizeof(PMA_HEADER) + (int)pos_data.size() * (int)sizeof(int);
            int aligned_len = total_len;
            if (aligned_len % INIT_VEC_LEN_IN_BYTE != 0)
                aligned_len += (INIT_VEC_LEN_IN_BYTE - (aligned_len % INIT_VEC_LEN_IN_BYTE));

            if (*off < 0 || static_cast<size_t>(*off) + aligned_len > PMA_DATA_SIZE) {
                std::cerr << "HBM capacity overflow on channel " << ch << std::endl;
                return false;
            }

            memcpy(base + *off, &pos_header, sizeof(PMA_HEADER));
            memcpy(base + *off + sizeof(PMA_HEADER), pos_data.data(), pos_data.size() * sizeof(int));

            // Write vecinfo at VECINFO_OFFSET + 64 + pma_idx * sizeof(VEC_INFO)
            VEC_INFO vi = ((ap_uint<64>)total_len << 32) | (ap_uint<32>)(*off);
            size_t vi_byte = VECINFO_OFFSET + 64 + (size_t)pma_idx * sizeof(VEC_INFO);
            memcpy(base + vi_byte, &vi, sizeof(VEC_INFO));

            *off += aligned_len;
        }

        // Write max_pma_idx at VECINFO_OFFSET
        for (int ch = 0; ch < CHANNEL_NUM; ch++) {
            BYTE* base = hbm_ptrs[ch];
            // Find max pma_idx for this channel
            int ch_max_pma = 0;
            for (int p = ch; p < total_pma_num; p += CHANNEL_NUM)
                ch_max_pma = p;
            VEC_INFO max_val = ch_max_pma;
            memcpy(base + VECINFO_OFFSET, &max_val, sizeof(VEC_INFO));

            // Init bitmap: mark blocks up to current_offset as used
            int total_blocks = (current_offset[ch] + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
            int full_bytes = total_blocks / 8;
            int rem_bits   = total_blocks % 8;
            memset(base + BITMAP_OFFSET, 0xFF, full_bytes);
            if (rem_bits > 0)
                base[BITMAP_OFFSET + full_bytes] = (BYTE)((1 << rem_bits) - 1);
        }

        auto end_cpu = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end_cpu - start_cpu;
        std::cout << "CPU preprocessing time in add_base_graph_to_hbm: " << elapsed.count() << " s" << std::endl;
        std::cout << "Base graph PMA construction completed." << std::endl;
        return true;
    };

    // =========================================================================
    // verify_graph
    // =========================================================================
    auto verify_graph = [&](const std::vector<std::vector<int>>& expected_graph,
                            int current_max_vertex, const std::string& label, bool last) {
        std::cout << "Verifying " << label << "..." << std::endl;
        std::vector<std::vector<int>> Updated_Graph(current_max_vertex + 1);

        auto run_check = xrt::run(check_kernel);
        run_check.set_arg(0, bo_coo_src);
        run_check.set_arg(1, bo_coo_dst);
        run_check.set_arg(2, bo_num_edges);
        run_check.set_arg(3, current_max_vertex + 1);
        run_check.set_arg(4, 3); // mode = edge count first
        run_check.set_arg(5, 0);
        std::cout << "Starting check kernel for " << label << " with max vertex " << current_max_vertex << "..." << std::endl;

        // Phase 1: edge count
        auto t1 = std::chrono::high_resolution_clock::now();
        run_check.start();
        run_check.wait();
        auto t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> edge_kernel_time = t2 - t1;

        bo_num_edges.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        int edge_num = bo_num_edges_ptr[0];
        std::cout << "Check kernel returned edge count: " << edge_num
                  << " | edge_count kernel time: " << edge_kernel_time.count() << " s" << std::endl;

        // Phase 2: COO
        run_check.set_arg(4, 1); // mode = COO
        run_check.set_arg(5, (int)last);
        auto t3 = std::chrono::high_resolution_clock::now();
        run_check.start();
        run_check.wait();
        auto t4 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> vertex_kernel_time = t4 - t3;
        std::cout << "Vertex check kernel time: " << vertex_kernel_time.count() << " s" << std::endl;

        bo_coo_src.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        bo_coo_dst.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

        for (int i = 0; i <= current_max_vertex; ++i) Updated_Graph[i].clear();
        for (size_t id = 0; id < buffer_len; ++id) {
            int s = coo_src_ptr[id];
            int d = coo_dst_ptr[id];
            if (s == -1 || d == -1) break;
            if (s < 0 || s > current_max_vertex) continue;
            Updated_Graph[s].push_back(d);
        }
        for (int i = 0; i <= current_max_vertex; ++i) {
            std::sort(Updated_Graph[i].begin(), Updated_Graph[i].end());
            Updated_Graph[i].erase(std::unique(Updated_Graph[i].begin(), Updated_Graph[i].end()),
                                   Updated_Graph[i].end());
        }

        bool all_match = true;
        std::string clean_label = label;
        if (kMismatchfileOutput) {
            std::replace(clean_label.begin(), clean_label.end(), ' ', '_');
            std::ofstream output_file("mismatch_" + clean_label + ".txt");
            for (int i = 0; i <= current_max_vertex; ++i) {
                const std::vector<int>& expected =
                    (i < (int)expected_graph.size()) ? expected_graph[i] : std::vector<int>();
                if (expected != Updated_Graph[i]) {
                    output_file << "MISMATCH AT VERTEX " << i << std::endl;
                    for (auto v : expected)  output_file << "EXPECTED EDGE: " << i << " -> " << v << std::endl;
                    for (auto v : Updated_Graph[i]) output_file << "ACTUAL EDGE: " << i << " -> " << v << std::endl;
                    all_match = false;
                }
            }
        } else {
            for (int i = 0; i <= current_max_vertex; ++i) {
                const std::vector<int>& expected =
                    (i < (int)expected_graph.size()) ? expected_graph[i] : std::vector<int>();
                if (expected != Updated_Graph[i]) { all_match = false; break; }
            }
        }

        if (all_match) std::cout << label << " matches perfectly!" << std::endl;
        else {
            std::cout << label << " does not match." << std::endl;
            final_match = false;
            if (kMismatchfileOutput)
                std::cout << "See mismatch_" << clean_label << ".txt for details." << std::endl;
        }
    };

    // =========================================================================
    // Set kernel arguments for long-running kernels
    // =========================================================================
    auto run_bs       = xrt::run(bs_kernel);
    auto run_insert   = xrt::run(insert_kernel);
    auto run_rebalance = xrt::run(rebalance_kernel);
    auto run_read     = xrt::run(pma_read_kernel);
    auto run_router   = xrt::run(update_message_router_kernel);

    // bs_kernel: hbm_ptr_0(2), hbm_ptr_1(3), vec_info_base(4)
    run_bs.set_arg(2, bo_hbm_0);
    run_bs.set_arg(3, bo_hbm_1);
    run_bs.set_arg(4, vec_info_base);

    // insert_kernel: hbm_ptr_0(3), hbm_ptr_1(4), vec_info_base(5)
    run_insert.set_arg(3, bo_hbm_0);
    run_insert.set_arg(4, bo_hbm_1);
    run_insert.set_arg(5, vec_info_base);

    // rebalance_kernel: hbm_ptr_0(1), hbm_ptr_1(2), bitmap_base(3), vec_info_base(4)
    run_rebalance.set_arg(1, bo_hbm_0);
    run_rebalance.set_arg(2, bo_hbm_1);
    run_rebalance.set_arg(3, bitmap_base);
    run_rebalance.set_arg(4, vec_info_base);

    // pma_read_kernel: hbm_ptr_0(3), hbm_ptr_1(4), vec_info_base(5)
    run_read.set_arg(3, bo_hbm_0);
    run_read.set_arg(4, bo_hbm_1);
    run_read.set_arg(5, vec_info_base);

    // =========================================================================
    // Sync initial HBM data, then start long-running kernels
    // =========================================================================
    bo_hbm_0.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_hbm_1.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_opcode.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    // Start long-running kernels
    run_router.start();
    run_bs.start();
    run_insert.start();
    run_rebalance.start();
    run_read.start();
    std::cout << "Long-running kernels started." << std::endl;

    // =========================================================================
    // Edge batch functions
    // =========================================================================
    auto load_edge_batch = [&](const std::vector<Operation>& batch_ops) -> int {
        size_t edge_idx = 0;
        for (const auto& op : batch_ops) {
            if (edge_idx >= edge_buffer_len) {
                std::cerr << "Edge buffer overflow in batch load." << std::endl;
                return -1;
            }
            src_ptr[edge_idx]    = op.src;
            dst_ptr[edge_idx]    = op.dst;
            opcode_ptr[edge_idx] = op.opcode;
            ++edge_idx;
        }
        if (edge_idx > 0) {
            bo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
            bo_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
            bo_opcode.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        }
        return (int)edge_idx;
    };

    auto run_update_batch = [&](int edge_count, const std::string& label, bool last) -> double {
        auto run_gen = xrt::run(update_message_generator_kernel);
        run_gen.set_arg(0, edge_count);
        run_gen.set_arg(1, bo_src);
        run_gen.set_arg(2, bo_dst);
        run_gen.set_arg(3, bo_opcode);
        run_gen.set_arg(4, (int)(last ? exit_flag : flush_flag));

        auto phase_start = std::chrono::high_resolution_clock::now();
        run_gen.start();
        run_gen.wait();
        auto phase_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = phase_end - phase_start;
        std::cout << label << "edges=" << edge_count << ", time=" << elapsed.count() << " s" << std::endl;
        return elapsed.count();
    };

    // =========================================================================
    // Process operations
    // =========================================================================
    if (kBaseGraphReadOnlyTest) {
        if (!Graph.empty())
            verify_graph(Graph, max_vertex_idx, "Base_Graph", true);
        else
            std::cout << "Base graph verification skipped (no vertices)." << std::endl;
        // Send stop only
        auto run_gen = xrt::run(update_message_generator_kernel);
        run_gen.set_arg(0, 0);
        run_gen.set_arg(1, bo_src);
        run_gen.set_arg(2, bo_dst);
        run_gen.set_arg(3, bo_opcode);
        run_gen.set_arg(4, (int)exit_flag);
        run_gen.start();
        run_gen.wait();
    } else {
        std::vector<std::vector<int>> runtime_graph;
        auto ensure_runtime_vertex = [&](int v) {
            if (v < 0) return;
            if (v >= static_cast<int>(runtime_graph.size())) runtime_graph.resize(v + 1);
        };
        auto normalize_neighbors = [&](int v) {
            if (v < 0 || v >= static_cast<int>(runtime_graph.size())) return;
            auto& nbrs = runtime_graph[v];
            std::sort(nbrs.begin(), nbrs.end());
            nbrs.erase(std::unique(nbrs.begin(), nbrs.end()), nbrs.end());
        };

        double incremental_phase_s = 0;
        std::mt19937 rng((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());

        for (size_t idx = 0; idx < operations.size(); ) {
            long long current_ts = operations[idx].timestamp;
            std::vector<Operation> batch_ops;
            while (idx < operations.size() && operations[idx].timestamp == current_ts) {
                batch_ops.push_back(operations[idx]);
                ++idx;
            }

            if (batch_ops.size() > 1 && kShuffleSameTimestampOps)
                std::shuffle(batch_ops.begin(), batch_ops.end(), rng);

            int edge_count = load_edge_batch(batch_ops);
            if (edge_count < 0) break;
            if (kVertexOnlyTest) edge_count = 0;

            bool last = (idx == operations.size());
            incremental_phase_s += run_update_batch(edge_count, "ts_" + std::to_string(current_ts), last);

            // Update CPU-side graph
            std::vector<int> dirty_vertices;
            dirty_vertices.reserve(batch_ops.size());
            for (const auto& op : batch_ops) {
                switch (op.opcode) {
                    case PMA_UPDATE_INSERT_EDGE:
                        if (kVertexOnlyTest) break;
                        ensure_runtime_vertex(op.src); ensure_runtime_vertex(op.dst);
                        if (op.src >= 0 && op.src < static_cast<int>(runtime_graph.size())) {
                            runtime_graph[op.src].push_back(op.dst);
                            dirty_vertices.push_back(op.src);
                        }
                        break;
                    case PMA_UPDATE_DELETE_EDGE:
                        if (kVertexOnlyTest) break;
                        ensure_runtime_vertex(op.src); ensure_runtime_vertex(op.dst);
                        if (op.src >= 0 && op.src < static_cast<int>(runtime_graph.size())) {
                            auto& nbrs = runtime_graph[op.src];
                            nbrs.erase(std::remove(nbrs.begin(), nbrs.end(), op.dst), nbrs.end());
                            dirty_vertices.push_back(op.src);
                        }
                        break;
                    case PMA_UPDATE_DELETE_VERTEX:
                        if (kVertexOnlyTest) break;
                        ensure_runtime_vertex(op.src);
                        if (op.src >= 0 && op.src < static_cast<int>(runtime_graph.size())) {
                            runtime_graph[op.src].clear();
                            dirty_vertices.push_back(op.src);
                        }
                        break;
                    default: break;
                }
            }

            std::sort(dirty_vertices.begin(), dirty_vertices.end());
            dirty_vertices.erase(std::unique(dirty_vertices.begin(), dirty_vertices.end()),
                                 dirty_vertices.end());
            for (int v : dirty_vertices) normalize_neighbors(v);

            int current_max_vertex = static_cast<int>(runtime_graph.size()) - 1;
            if (!runtime_graph.empty())
                verify_graph(runtime_graph, current_max_vertex,
                             "after_ts_" + std::to_string(current_ts), last);
        }
    }

    // =========================================================================
    // Wait for long-running kernels to finish
    // =========================================================================
    std::cout << "Waiting for long-running kernels to finish..." << std::endl;
    run_router.wait();
    run_bs.wait();
    run_insert.wait();
    run_rebalance.wait();
    run_read.wait();

    // =========================================================================
    // Dump device PMA state
    // =========================================================================
    std::cout << "Dumping device PMA state (vecinfo + HBM) ..." << std::endl;
    bo_hbm_0.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    bo_hbm_1.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    BYTE* hbm_ptrs[CHANNEL_NUM] = {hbm_ptr_0, hbm_ptr_1};
    VEC_INFO* vecinfo_ptrs[CHANNEL_NUM];
    for (int ch = 0; ch < CHANNEL_NUM; ch++)
        vecinfo_ptrs[ch] = reinterpret_cast<VEC_INFO*>(hbm_ptrs[ch] + VECINFO_OFFSET + 64);

    std::cout << "First 100 vecinfo entries:" << std::endl;
    int printed_pma = 0;
    for (int pma_idx = 0; pma_idx < 100; ++pma_idx) {
        int ch = pma_idx % CHANNEL_NUM;
        VEC_INFO vec = vecinfo_ptrs[ch][pma_idx];
        int start = vec.range(31, 0);
        int len   = vec.range(63, 32);
        std::cout << "PMA " << pma_idx << " (ch=" << ch << ") vecinfo start=" << start << " len=" << len << std::endl;
        if (vec != 0) {
            print_pma(pma_idx, vecinfo_ptrs[ch], hbm_ptrs[ch]);
            printed_pma++;
        }
    }

    std::cout << "\nVecInfo cross-channel view (first 32 PMA idx):" << std::endl;
    for (int pma_idx = 0; pma_idx < 32; ++pma_idx) {
        std::cout << "PMA " << pma_idx << ": ";
        for (int ch = 0; ch < CHANNEL_NUM; ++ch) {
            VEC_INFO vec = vecinfo_ptrs[ch][pma_idx];
            std::cout << "[ch=" << ch << " start=" << vec.range(31, 0) << " len=" << vec.range(63, 32) << "] ";
        }
        std::cout << std::endl;
    }

    std::cout << "Printed PMA payload for " << printed_pma << " non-zero vecinfo entries." << std::endl;
    if (final_match) std::cout << "FINAL RESULT: MATCH!" << std::endl;
    else             std::cout << "FINAL RESULT: MISMATCH!" << std::endl;
    return 0;
}
