// regraph_read_host — 读管线 + ReGraph 计算核集成测试 host
//
// 只使用:
//   读管线:  pma_read_worker_kernel × CHANNEL_NUM (直接由 edgeReader 驱动, 无 router/merger)
//   ReGraph: edgeReader(转换) → littleKernelScatterGather → kernelLittleGSMerger
//            → kernelApply → kernelHBMWrapper
//
// 图全部作为 base graph 由 host 直接构建为 PMA 结构写入 HBM (含 FE/VC helper 区),
// 然后 edgeReader 通过读管线把整图以 COO 流式读出, 转换成 ReGraph edge burst,
// 驱动 BFS 计算链执行一遍。
//
// 用法: regraph_read_host <xclbin> <graph> [src_vertex]
//   图文件格式: "src dst" 或 "src dst timestamp opcode" (全部边都当作 base graph)

#include "data_type.h"
#include "deprecated/xrt.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
#include <xrt/xrt_bo.h>
#include <xrt/xrt_device.h>
#include <xrt/xrt_kernel.h>

// 与 host.cpp 一致的 HBM 布局常量 (per channel)
constexpr size_t PMA_DATA_SIZE   = 220 * 1024 * 1024;  // 必须与内核一致
constexpr size_t VECINFO_SIZE    =  10 * 1024 * 1024;
constexpr size_t CHANNEL_TOTAL   = PMA_DATA_SIZE + VECINFO_SIZE;
constexpr int    PMA_RAW_DATA_START  = 0;
constexpr int    PMA_VECINFO_START   = static_cast<int>(PMA_DATA_SIZE);

// BFS 属性常量 (与 ReGraph acc_udfs/bfs 一致)
constexpr uint32_t MAX_PROP          = 0x7FFFFFFE;  // INT_MAX - 1
constexpr uint32_t VERTEX_ACTIVE     = 0x80000000;
constexpr uint32_t SRC_VERTEX_ACTIVE = VERTEX_ACTIVE | 0x1;
// 单趟 BFS 后, 源顶点的直接出邻居属性应为 preprocess(0x80000001) = 0x80000002
constexpr uint32_t REACHED_PROP      = 0x80000002;

// ReGraph little GS 的 dst 缓冲 (每个顶点 4B prop, 每 512-bit word 16 顶点)。
// 大小由 -DLITTLE_KERNEL_DST_BUFFER_SIZE 决定 (makefile 从 global_para.mk 传入),
// 增大它即可支持更大的图 (代价: gather URAM = 32B × 顶点数)。
#ifndef LITTLE_KERNEL_DST_BUFFER_SIZE
#define LITTLE_KERNEL_DST_BUFFER_SIZE 65536
#endif
constexpr size_t DST_PROP_WORDS  = LITTLE_KERNEL_DST_BUFFER_SIZE >> 4;  // apply/wrapper 写回的 prop word 数
constexpr size_t DST_PROP_BYTES  = DST_PROP_WORDS * 64;
constexpr int    MAX_GS_VERTEX   = LITTLE_KERNEL_DST_BUFFER_SIZE;

struct Operation
{
    int src;
    int dst;
};

static std::vector<std::vector<int>> build_adjacency(const std::string& graph_path,
                                                     int& total_edges)
{
    std::ifstream graph_file(graph_path);
    if (!graph_file.is_open())
    {
        std::cerr << "Failed to open graph file: " << graph_path << std::endl;
        exit(1);
    }

    std::vector<std::vector<int>> adjacency;
    total_edges = 0;
    std::string line;
    while (std::getline(graph_file, line))
    {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        int src, dst;
        long long ts;
        int opcode;
        if (iss >> src >> dst >> ts >> opcode)
        {
            // 4 列动态格式: 本测试把全部边都当作 base graph
        }
        else
        {
            std::istringstream iss2(line);
            if (!(iss2 >> src >> dst)) continue;
        }
        if (src < 0 || dst < 0) continue;
        if (src >= static_cast<int>(adjacency.size())) adjacency.resize(src + 1);
        if (dst >= static_cast<int>(adjacency.size())) adjacency.resize(dst + 1);
        adjacency[src].push_back(dst);
        total_edges++;
    }
    graph_file.close();
    for (auto& neighbors : adjacency) std::sort(neighbors.begin(), neighbors.end());
    return adjacency;
}

static const std::vector<int>& empty_neighbors()
{
    static const std::vector<int> empty;
    return empty;
}

// 将整图构建为 PMA 结构 (header + FE/VC helper 区 + segments) 写入 HBM。
// PMA 按 pma_idx % CHANNEL_NUM 轮转分配到各通道。
static void build_base_graph_in_hbm(const std::vector<std::vector<int>>& adjacency,
                                    std::vector<xrt::bo>& ch_bufs,
                                    std::vector<VEC_INFO*>& vec_info_ptrs)
{
    const int vertex_num = static_cast<int>(adjacency.size());
    const int total_pma_num = (vertex_num + VERTEX_PER_PMA - 1) / VERTEX_PER_PMA;
    if (total_pma_num > MAX_PMA_NUM)
    {
        std::cerr << "PMA count overflow: required=" << total_pma_num
                  << ", MAX_PMA_NUM=" << MAX_PMA_NUM << std::endl;
        exit(1);
    }

    std::vector<BYTE*> hbm_base(CHANNEL_NUM);
    for (int ch = 0; ch < CHANNEL_NUM; ++ch) hbm_base[ch] = ch_bufs[ch].map<BYTE*>();
    for (int ch = 0; ch < CHANNEL_NUM; ++ch)
    {
        std::fill_n(vec_info_ptrs[ch], MAX_PMA_NUM, 0);
    }

    std::vector<int> current_offset(CHANNEL_NUM, 0);

    for (int pma_idx = 0; pma_idx < total_pma_num; ++pma_idx)
    {
        int channel = pma_idx % CHANNEL_NUM;
        BYTE* base_addr = hbm_base[channel];

        PMA_HEADER header = {};
        std::vector<int> segment_data;
        int total_segs = 0;

        for (int i = 0; i < VERTEX_PER_PMA; ++i)
        {
            int src = pma_idx * VERTEX_PER_PMA + i;
            const std::vector<int>& neighbors =
                (src < vertex_num) ? adjacency[src] : empty_neighbors();

            header.edge_count[i] = static_cast<int>(neighbors.size());
            header.attr[i] = 0;

            int segs = 1;
            if (neighbors.size() > 0)
            {
                while (segs * 47 < static_cast<int>(neighbors.size())) segs *= 4;
            }
            header.vertex_range[i] = total_segs + segs;

            int base_num = segs > 0 ? static_cast<int>(neighbors.size()) / segs : 0;
            int extra_num = segs > 0 ? static_cast<int>(neighbors.size()) % segs : 0;
            int neighbor_idx = 0;
            for (int s = 0; s < segs; ++s)
            {
                int valid_num = base_num + (s < extra_num ? 1 : 0);
                for (int k = 0; k < valid_num; ++k) segment_data.push_back(neighbors[neighbor_idx++]);
                for (int k = valid_num; k < EDGE_SLOTS_PER_SEG; ++k) segment_data.push_back(UNUSED_ICON);
                segment_data.push_back(valid_num);  // VALID_COUNT_IDX
            }
            total_segs += segs;
        }

        // FE/VC helper 区 (SoA): FE 1 entry/segment + VC 1 entry/segment, 16 entries/word
        int fe_words = fe_words_for(total_segs);
        int vc_words = vc_words_for(total_segs);
        int hlp_words = fe_words + vc_words;
        std::vector<ap_uint<512>> helper(hlp_words, 0);
        for (int s = 0; s < total_segs; ++s)
        {
            // FE entry: 该 segment 的第一条真实边 (空 segment 为 -1)
            int fe_value = UNUSED_ICON;
            int vc_value = 0;
            int seg_base_idx = s * SEGMENT_SIZE;
            if (segment_data[seg_base_idx] != UNUSED_ICON) fe_value = segment_data[seg_base_idx];
            vc_value = segment_data[seg_base_idx + VALID_COUNT_IDX];

            int fe_word = s / FE_SOA_ENTRIES_PER_WORD, fe_slot = s % FE_SOA_ENTRIES_PER_WORD;
            int vc_word = s / VC_SOA_ENTRIES_PER_WORD, vc_slot = s % VC_SOA_ENTRIES_PER_WORD;
            helper[fe_word].range(fe_slot * 32 + 31, fe_slot * 32) = (ap_uint<32>)fe_value;
            helper[vc_word].range(vc_slot * 32 + 31, vc_slot * 32) = (ap_uint<32>)vc_value;
        }

        int total_len = static_cast<int>(sizeof(PMA_HEADER)) + hlp_words * 64
                      + static_cast<int>(segment_data.size() * sizeof(int));
        int aligned_len = total_len;
        if (aligned_len % INIT_VEC_LEN_IN_BYTE != 0)
            aligned_len += (INIT_VEC_LEN_IN_BYTE - (aligned_len % INIT_VEC_LEN_IN_BYTE));
        if (current_offset[channel] + aligned_len > static_cast<int>(CHANNEL_CAPACITY))
        {
            std::cerr << "HBM capacity overflow on channel " << channel
                      << " (offset=" << current_offset[channel] << ")" << std::endl;
            exit(1);
        }

        memcpy(base_addr + current_offset[channel], &header, sizeof(PMA_HEADER));
        for (int w = 0; w < hlp_words; ++w)
        {
            memcpy(base_addr + current_offset[channel] + sizeof(PMA_HEADER) + w * 64,
                   &helper[w], 64);
        }
        memcpy(base_addr + current_offset[channel] + sizeof(PMA_HEADER) + hlp_words * 64,
               segment_data.data(), segment_data.size() * sizeof(int));

        vec_info_ptrs[channel][pma_idx] =
            (ap_uint<64>(total_len) << 32) | (ap_uint<32>)current_offset[channel];
        current_offset[channel] += aligned_len;
    }

    for (int ch = 0; ch < CHANNEL_NUM; ++ch) ch_bufs[ch].sync(XCL_BO_SYNC_BO_TO_DEVICE);
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <xclbin> <graph> [src_vertex]" << std::endl;
        return 1;
    }
    std::string xclbin_name = argv[1];
    std::string graph_path = argv[2];
    int src_vertex = (argc > 3) ? std::stoi(argv[3]) : 0;

    // ---- 1. 读取图, 构建邻接表 ----
    int total_edges = 0;
    auto adjacency = build_adjacency(graph_path, total_edges);
    int num_vertices = static_cast<int>(adjacency.size());
    std::cout << "Graph: vertices=" << num_vertices << " edges=" << total_edges << std::endl;
    if (num_vertices > MAX_GS_VERTEX)
    {
        std::cerr << "Graph too large: " << num_vertices
                  << " vertices > little GS dst buffer " << MAX_GS_VERTEX << std::endl;
        return 1;
    }
    if (src_vertex >= num_vertices)
    {
        std::cerr << "src_vertex " << src_vertex << " out of range" << std::endl;
        return 1;
    }

    // ---- 2. 打开设备 ----
    auto xclbin = xrt::xclbin(xclbin_name);
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin(xclbin);
    std::cout << "device name: " << device.get_info<xrt::info::device::name>() << std::endl;

    // ---- 3. 创建内核 ----
    std::vector<xrt::kernel> read_worker_kernels;
    for (int i = 1; i <= CHANNEL_NUM; ++i)
    {
        read_worker_kernels.emplace_back(device, uuid,
            "pma_read_worker_kernel:{pma_read_worker_kernel_" + std::to_string(i) + "}");
    }
    auto edge_reader_kernel = xrt::kernel(device, uuid, "edgeReader:{edgeReader_1}");
    auto gs_kernel = xrt::kernel(device, uuid, "littleKernelScatterGather:{littleKernelScatterGather_1}");
    // auto merger_kernel = xrt::kernel(device, uuid, "kernelLittleGSMerger:{kernelLittleGSMerger_1}");
    auto apply_kernel = xrt::kernel(device, uuid, "kernelApply:{kernelApply_1}");
    auto hbm_wrapper_kernel = xrt::kernel(device, uuid, "kernelHBMWrapper:{kernelHBMWrapper_1}");

    // ---- 4. 分配 buffer ----
    std::vector<xrt::bo> ch_bufs;
    for (int ch = 0; ch < CHANNEL_NUM; ++ch)
    {
        ch_bufs.emplace_back(device, CHANNEL_TOTAL, read_worker_kernels[ch].group_id(1));
    }
    auto bo_edge_count = xrt::bo(device, 16, edge_reader_kernel.group_id(0));
    // src_prop 由 wrapper 按 src 窗口读取 (每窗口 256 word)。
    // GS scatter 会预取当前窗口+1 个窗口, 所以多留一个窗口避免越界。
    size_t src_prop_words = ((num_vertices + 15) / 16 / 256 + 1) * 256;
    // new_prop / vertex_prop 是 apply 的写回缓冲, 大小 = 分区 (dst buffer) 大小
    auto bo_src_prop = xrt::bo(device, src_prop_words * 64, hbm_wrapper_kernel.group_id(0));
    auto bo_new_prop = xrt::bo(device, DST_PROP_BYTES, hbm_wrapper_kernel.group_id(0));
    auto bo_vertex_prop = xrt::bo(device, DST_PROP_BYTES, apply_kernel.group_id(0));

    std::vector<VEC_INFO*> vec_info_ptrs(CHANNEL_NUM);
    for (int ch = 0; ch < CHANNEL_NUM; ++ch)
    {
        vec_info_ptrs[ch] =
            reinterpret_cast<VEC_INFO*>(ch_bufs[ch].map<BYTE*>() + PMA_VECINFO_START);
    }

    // ---- 5. 构建 base graph (PMA 布局) ----
    auto build_start = std::chrono::high_resolution_clock::now();
    build_base_graph_in_hbm(adjacency, ch_bufs, vec_info_ptrs);
    auto build_end = std::chrono::high_resolution_clock::now();
    double build_seconds = std::chrono::duration<double>(build_end - build_start).count();
    std::cout << "[T] base graph built in HBM (" << (num_vertices + 15) / 16
              << " PMAs) in " << build_seconds << " s" << std::endl;

    // ---- 6. 初始化顶点属性 (BFS: 仅 src_vertex 活跃) ----
    auto init_props = [&](xrt::bo& bo, int vertex_count) {
        uint32_t* props = bo.map<uint32_t*>();
        std::fill_n(props, vertex_count, MAX_PROP);
        props[src_vertex] = SRC_VERTEX_ACTIVE;
        bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    };
    init_props(bo_src_prop, src_prop_words * 16);
    init_props(bo_new_prop, DST_PROP_WORDS * 16);
    init_props(bo_vertex_prop, DST_PROP_WORDS * 16);

    // ---- 7. 设置内核参数 ----
    std::vector<xrt::run> run_read_workers;
    for (int ch = 0; ch < CHANNEL_NUM; ++ch)
    {
        run_read_workers.emplace_back(read_worker_kernels[ch]);
        run_read_workers[ch].set_arg(0, ch);
        run_read_workers[ch].set_arg(1, ch_bufs[ch]);
        run_read_workers[ch].set_arg(2, PMA_RAW_DATA_START);
        run_read_workers[ch].set_arg(3, PMA_VECINFO_START);
    }
    auto run_edge_reader = xrt::run(edge_reader_kernel);
    run_edge_reader.set_arg(0, bo_edge_count);
    run_edge_reader.set_arg(1, num_vertices);

    auto run_gs = xrt::run(gs_kernel);
    run_gs.set_arg(2, 0);  // part_dst_offset = 0 (单分区, 全图)

    auto run_apply = xrt::run(apply_kernel);
    run_apply.set_arg(0, bo_vertex_prop);
    run_apply.set_arg(1, 1u);  // num_dense_paritions
    run_apply.set_arg(2, 0u);  // num_sparse_paritions
    run_apply.set_arg(3, 0u);  // arg_reg

    auto run_hbm_wrapper = xrt::run(hbm_wrapper_kernel);
    run_hbm_wrapper.set_arg(0, bo_src_prop);
    run_hbm_wrapper.set_arg(1, bo_new_prop);
    run_hbm_wrapper.set_arg(2, 1u);  // num_dense_paritions
    run_hbm_wrapper.set_arg(3, 0u);  // num_sparse_paritions

    // auto run_merger = xrt::run(merger_kernel);

    // ---- 8. 启动 ----
    // 读 worker 长驻, 由 edgeReader 的 STOP 结束;
    // merger 是无限循环内核 (不退出), host 只 start 不 wait。
    for (auto& r : run_read_workers) r.start();
    run_hbm_wrapper.start();
    run_apply.start();
    // run_merger.start();
    run_gs.start();

    auto chain_start = std::chrono::high_resolution_clock::now();
    run_edge_reader.start();
    std::cout << "[T] edgeReader started" << std::endl;

    // ---- 9. 等待 ----
    run_edge_reader.wait();
    auto read_end = std::chrono::high_resolution_clock::now();
    std::cout << "[T] edgeReader done" << std::endl;

    run_gs.wait();
    std::cout << "[T] GS done" << std::endl;
    run_apply.wait();
    std::cout << "[T] apply done" << std::endl;
    run_hbm_wrapper.wait();
    std::cout << "[T] hbm wrapper done" << std::endl;
    auto compute_end = std::chrono::high_resolution_clock::now();

    for (auto& r : run_read_workers) r.wait();
    std::cout << "[T] read workers done" << std::endl;

    double read_seconds = std::chrono::duration<double>(read_end - chain_start).count();
    double compute_seconds = std::chrono::duration<double>(compute_end - chain_start).count();

    // ---- 10. 验证 ----
    bo_edge_count.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    uint64_t* count_ptr = bo_edge_count.map<uint64_t*>();
    uint64_t read_edge_count = count_ptr[0];
    uint64_t sent_edge_count = count_ptr[1];
    std::cout << "Read pipeline edge count: " << read_edge_count
              << " (expected " << total_edges << "), sent to GS: " << sent_edge_count << std::endl;

    bool count_match = (read_edge_count == static_cast<uint64_t>(total_edges));

    bo_vertex_prop.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    uint32_t* props = bo_vertex_prop.map<uint32_t*>();

    std::unordered_set<int> expected_reached;
    if (src_vertex < static_cast<int>(adjacency.size()))
    {
        for (int dst : adjacency[src_vertex]) expected_reached.insert(dst);
    }
    std::vector<int> reached;
    for (int v = 0; v < num_vertices; ++v)
    {
        if (v == src_vertex) continue;
        if (props[v] == REACHED_PROP) reached.push_back(v);
    }
    bool props_match = (reached.size() == expected_reached.size());
    for (int v : reached)
    {
        if (!expected_reached.count(v)) props_match = false;
    }
    std::cout << "BFS single-pass check: " << (props_match ? "MATCH" : "MISMATCH")
              << " — reached " << reached.size() << " of " << expected_reached.size()
              << " expected out-neighbors of v" << src_vertex << std::endl;

    // ---- 11. 报告 ----
    double read_mteps = static_cast<double>(read_edge_count) / read_seconds / 1e6;
    std::cout << "======================================================" << std::endl;
    std::cout << "Base graph build (host): " << build_seconds << " s" << std::endl;
    std::cout << "Read pipeline (edgeReader): " << read_seconds * 1000.0
              << " ms, " << read_edge_count << " edges, " << read_mteps << " MTEPS" << std::endl;
    std::cout << "End-to-end (read + BFS compute): " << compute_seconds * 1000.0
              << " ms (" << compute_seconds - read_seconds << " s compute)" << std::endl;
    std::cout << "RESULT: " << ((count_match && props_match) ? "PASS" : "FAIL") << std::endl;
    std::cout << "======================================================" << std::endl;

    return (count_match && props_match) ? 0 : 1;
}
