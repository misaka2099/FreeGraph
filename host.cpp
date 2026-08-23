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
// The split host supports only the ONLINE generator -> router -> worker path.
// USE_STANDALONE_MODE is supplied by the split-host build targets.

const bool kBaseGraphReadOnlyTest = false;
const bool kVertexOnlyTest = false;
const bool kShuffleSameTimestampOps = true; // Set to true to enable shuffling of operations with the same timestamp
const bool kMismatchfileOutput = true; // Set to true to output mismatch details to files
void print_pma(
    int pma_idx,
    VEC_INFO* pos_info,
    BYTE* storage
)
{
    VEC_INFO vec_info = pos_info[pma_idx];
    int start = vec_info.range(31,0);
    int len =  vec_info.range(63,32);
    PMA_HEADER header;
    memcpy(&header, storage + start, sizeof(PMA_HEADER));
    int total_segs = header.vertex_range[VERTEX_PER_PMA - 1];
    int fe_words = fe_words_for(total_segs);
    int vc_words = vc_words_for(total_segs);
    int hlp_words = fe_words + vc_words;  // = hlp_total_words(total_segs)
    // SoA layout: FE area (1 entry per segment, 16 entries/word) + VC area (same)
    ap_uint<512>* fe_ptr = (ap_uint<512>*)(storage + start + sizeof(PMA_HEADER));
    ap_uint<512>* vc_ptr = fe_ptr + fe_words;
    // Segment data: after header + helper
    int *pma_data = (int*)(storage + start + sizeof(PMA_HEADER) + hlp_words * 64);

    std::cout << "=== PMA " << pma_idx << " start=" << start << " len=" << len
              << " total_segs=" << total_segs << " hlp_words=" << hlp_words << " ===" << std::endl;

    // Print helper (SoA: FE and VC separate, 16 entries per 512-bit word)
    std::cout << "  Helper (first_edge, valid_count):" << std::endl;
    for (int s = 0; s < total_segs; s++) {
        int fe_word = s / FE_SOA_ENTRIES_PER_WORD, fe_slot = s % FE_SOA_ENTRIES_PER_WORD;
        int vc_word = s / VC_SOA_ENTRIES_PER_WORD, vc_slot = s % VC_SOA_ENTRIES_PER_WORD;
        int first_edge = (int)fe_ptr[fe_word].range(fe_slot * 32 + 31, fe_slot * 32);
        int vc         = (int)vc_ptr[vc_word].range(vc_slot * 32 + 31, vc_slot * 32);
        std::cout << "    seg" << s << ": fe=" << first_edge << " vc=" << vc;
        if ((s + 1) % 4 == 0 || s == total_segs - 1) std::cout << std::endl;
        else std::cout << " | ";
    }

    // Print vertices and their segments
    for (int i = 0 ; i < VERTEX_PER_PMA ; i++)
    {
        int src = pma_idx * VERTEX_PER_PMA + i;
        int ec = header.edge_count[i];
        int seg_start = i == 0 ? 0 : header.vertex_range[i-1];
        int seg_end = header.vertex_range[i];
        std::cout << "  v" << src << ": edges=" << ec << " segs=" << seg_start << ".." << seg_end-1 << "  [";
        for (int j = seg_start ; j < seg_end ; j ++)
        {
            int vc = pma_data[j * SEGMENT_SIZE + VALID_COUNT_IDX];
            std::cout << "(" << vc << "){";
            for (int k = 0 ; k < SEGMENT_SIZE ; k++)
                std::cout << pma_data[j * SEGMENT_SIZE + k] << (k+1 < SEGMENT_SIZE ? "," : "");
            std::cout << "} ";
        }
        std::cout << "]" << std::endl;
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
    const bool host_verify = (argc >= 4 && std::string(argv[3]) == "--host-verify");
    std::cout << "Update path: ONLINE (generator→router→worker)" << std::endl;
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
#ifndef USE_STANDALONE_MODE
    auto update_message_router_kernel = xrt::kernel(device, uuid, "update_message_router");
#endif
#ifdef USE_STANDALONE_MODE
    auto update_message_router_stream_kernel = xrt::kernel(device, uuid,
        "update_message_router_stream:{update_message_router_stream_1}");
#endif
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
#ifdef USE_STANDALONE_MODE
    // Split mode: 3 内核类型 per channel (task_reader / bs_insert / redistribution)
    std::vector<xrt::kernel> task_reader_kernels;
    std::vector<xrt::kernel> bs_insert_kernels;
    std::vector<xrt::kernel> redistribution_kernels;
    task_reader_kernels.reserve(CHANNEL_NUM);
    bs_insert_kernels.reserve(CHANNEL_NUM);
    redistribution_kernels.reserve(CHANNEL_NUM);
#if defined(USE_MERGE_BS_PATH)
    std::vector<xrt::kernel> merge_pipeline_kernels;
    merge_pipeline_kernels.reserve(CHANNEL_NUM);
#endif
    for (int i = 1; i <= CHANNEL_NUM; ++i) {
        if (!try_add_kernel(task_reader_kernels, "task_reader_kernel:{task_reader_kernel_" + std::to_string(i) + "}")) break;
#if defined(USE_MERGE_BS_PATH)
        if (!try_add_kernel(bs_insert_kernels, "bs_insert_kernel:{bs_insert_kernel_" + std::to_string(i) + "}")) break;
        if (!try_add_kernel(merge_pipeline_kernels, "merge_pipeline:{merge_pipeline_" + std::to_string(i) + "}")) break;
#elif defined(USE_MERGE_PATH)
        if (!try_add_kernel(bs_insert_kernels, "merge_pipeline:{merge_pipeline_" + std::to_string(i) + "}")) break;
#else
        if (!try_add_kernel(bs_insert_kernels, "bs_insert_kernel:{bs_insert_kernel_" + std::to_string(i) + "}")) break;
#endif
        if (!try_add_kernel(redistribution_kernels, "redistribution_kernel:{redistribution_kernel_" + std::to_string(i) + "}")) break;
    }
    // pma_insert_edge_kernels 别名 bs_insert (主 worker, 用于 HBM BO 的 group_id)
    pma_insert_edge_kernels = bs_insert_kernels;
#else
    for (int i = 1; i <= CHANNEL_NUM; ++i)
    {
        if (!try_add_kernel(pma_insert_edge_kernels, "pma_insert_edge:{pma_insert_edge_" + std::to_string(i) + "}")) {
            break;
        }
    }
#endif
    auto check_kernel = xrt::kernel(device, uuid, "check_kernel");
    // alloc_access kernels (bitmap allocator)
    std::vector<xrt::kernel> alloc_access_kernels;
    alloc_access_kernels.reserve(CHANNEL_NUM);
    for (int i = 1; i <= CHANNEL_NUM; ++i) {
        if (!try_add_kernel(alloc_access_kernels,
                "alloc_access_kernel:{alloc_access_kernel_" + std::to_string(i) + "}")) {
            break;
        }
    }
    std::cout << "[DBG] L233: Kernels loaded successfully." << std::endl;
#ifdef USE_STANDALONE_MODE
    std::cout << "[DBG] L234: task_reader=" << task_reader_kernels.size()
              << " bs_insert=" << bs_insert_kernels.size()
              << " redistribution=" << redistribution_kernels.size()
              << " read_worker=" << pma_read_worker_kernels.size()
              << " alloc_access=" << alloc_access_kernels.size()
              << " channels=" << CHANNEL_NUM << std::endl;
    if (pma_read_worker_kernels.size() < static_cast<size_t>(CHANNEL_NUM) ||
        task_reader_kernels.size() < static_cast<size_t>(CHANNEL_NUM) ||
        bs_insert_kernels.size() < static_cast<size_t>(CHANNEL_NUM) ||
        redistribution_kernels.size() < static_cast<size_t>(CHANNEL_NUM) ||
        alloc_access_kernels.size() < static_cast<size_t>(CHANNEL_NUM)
#if defined(USE_MERGE_BS_PATH)
        || merge_pipeline_kernels.size() < static_cast<size_t>(CHANNEL_NUM)
#endif
        )
    {
        std::cerr << "Kernel loading incomplete for read/insert/alloc CUs; aborting before launch." << std::endl;
        std::cerr << "Loaded read workers: " << pma_read_worker_kernels.size()
                  << ", task_reader: " << task_reader_kernels.size()
                  << ", bs_insert: " << bs_insert_kernels.size()
                  << ", redistribution: " << redistribution_kernels.size()
                  << ", alloc access kernels: " << alloc_access_kernels.size()
#if defined(USE_MERGE_BS_PATH)
                  << ", merge_pipeline: " << merge_pipeline_kernels.size()
#endif
                  << std::endl;
        return 1;
    }
#else
    std::cout << "[DBG] L234: insert_edge=" << pma_insert_edge_kernels.size()
              << " read_worker=" << pma_read_worker_kernels.size()
              << " alloc_access=" << alloc_access_kernels.size()
              << " channels=" << CHANNEL_NUM << std::endl;
    if (pma_read_worker_kernels.size() < static_cast<size_t>(CHANNEL_NUM) ||
        pma_insert_edge_kernels.size() < static_cast<size_t>(CHANNEL_NUM) ||
        alloc_access_kernels.size() < static_cast<size_t>(CHANNEL_NUM))
    {
        std::cerr << "Kernel loading incomplete for read/insert/alloc CUs; aborting before launch." << std::endl;
        std::cerr << "Loaded read workers: " << pma_read_worker_kernels.size()
                  << ", insert edge kernels: " << pma_insert_edge_kernels.size()
                  << ", alloc access kernels: " << alloc_access_kernels.size() << std::endl;
        return 1;
    }
#endif
    std::cout << "Launching kernels..." << std::endl;
    auto run_pma_read_router_kernel = xrt::run(pma_read_router_kernel);
    std::vector<xrt::run> run_pma_read_worker_kernels;
    run_pma_read_worker_kernels.reserve(pma_read_worker_kernels.size());
    for (auto &k : pma_read_worker_kernels)
    {
        run_pma_read_worker_kernels.emplace_back(k);
    }
    auto run_pma_read_merger_kernel = xrt::run(pma_read_merger_kernel);
#ifndef USE_STANDALONE_MODE
    auto run_update_message_router_kernel = xrt::run(update_message_router_kernel);
    std::cout << "Update message router kernel is launched..." << std::endl;
#endif
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
#ifdef USE_STANDALONE_MODE
    std::vector<xrt::run> run_task_reader_kernels;
    std::vector<xrt::run> run_redistribution_kernels;
    run_task_reader_kernels.reserve(task_reader_kernels.size());
    run_redistribution_kernels.reserve(redistribution_kernels.size());
    for (auto &k : task_reader_kernels) run_task_reader_kernels.emplace_back(k);
    for (auto &k : redistribution_kernels) run_redistribution_kernels.emplace_back(k);
#if defined(USE_MERGE_BS_PATH)
    std::vector<xrt::run> run_merge_pipeline_kernels;
    run_merge_pipeline_kernels.reserve(merge_pipeline_kernels.size());
    for (auto &k : merge_pipeline_kernels) run_merge_pipeline_kernels.emplace_back(k);
#endif
    std::cout << "task_reader / redistribution kernels are launched..." << std::endl;
#endif
    std::cout << "PMA insert edge kernels are launched..." << std::endl;
    std::vector<xrt::run> run_alloc_access_kernels;
    run_alloc_access_kernels.reserve(alloc_access_kernels.size());
    for (auto &k : alloc_access_kernels) {
        run_alloc_access_kernels.emplace_back(k);
    }
    std::cout << "alloc_access kernels are launched..." << std::endl;
    std::cout << "Kernels are created..." << std::endl;

    auto start_buffer = std::chrono::high_resolution_clock::now();
    // === m_axi HBM layout per channel ===
    constexpr size_t PMA_DATA_SIZE   = 440 * 1024 * 1024;  // 每 m_axi 端口 2 个 HBM 通道 (2×220MB)
    constexpr size_t VECINFO_SIZE    =  20 * 1024 * 1024;
    constexpr size_t BM_SIZE         =  12 * 1024 * 1024;
    constexpr size_t CHANNEL_TOTAL   = PMA_DATA_SIZE + VECINFO_SIZE + BM_SIZE;
    long long pma_raw_data_start = 0;
    long long pma_vecinfo_start  = PMA_DATA_SIZE;
    long long pma_bm_start       = PMA_DATA_SIZE + VECINFO_SIZE;
    std::vector<int> allocator_initial_segments(CHANNEL_NUM, 0);

    // --- One BO per channel — all 3 m_axi ports share the same bank ---
    std::vector<xrt::bo> ch_bufs; ch_bufs.reserve(CHANNEL_NUM);
    for (int ch = 0; ch < CHANNEL_NUM; ++ch) {
        auto& k = pma_insert_edge_kernels[ch];
        std::cout << "[DBG] L290: ch=" << ch
                  << " gid0=" << k.group_id(0) << " gid1=" << k.group_id(1) << std::endl;
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

    // Bitmap: standalone uses ch_bufs bitmap area (no separate BO);
    // non-standalone keeps separate bo_bitmap for alloc_access m_axi.
    std::vector<xrt::bo> bo_bitmap; bo_bitmap.reserve(CHANNEL_NUM);
    std::vector<BYTE*> bitmap_ptrs; bitmap_ptrs.reserve(CHANNEL_NUM);
#ifdef USE_STANDALONE_MODE
    // Standalone: bitmap lives in ch_bufs at offset pma_bm_start.
    // alloc_access gets initial bitmap via stream from insert_edge worker.
    for (int ch = 0; ch < CHANNEL_NUM; ++ch)
        bitmap_ptrs.push_back(hbm_base_addresses[ch] + pma_bm_start);
#else
    for (int ch = 0; ch < CHANNEL_NUM; ++ch) {
        bo_bitmap.emplace_back(device, BM_SIZE, alloc_access_kernels[ch].group_id(0));
    }
    for (int ch = 0; ch < CHANNEL_NUM; ++ch)
        bitmap_ptrs.push_back(bo_bitmap[ch].map<BYTE*>());
#endif

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

    // Generator buffers (src, dst, opcode) — used in both modes
    auto bo_src = xrt::bo(device, sizeof(int) * edge_buffer_len, update_message_generator_kernel.group_id(1));
    auto bo_dst = xrt::bo(device, sizeof(int) * edge_buffer_len, update_message_generator_kernel.group_id(2));
    auto bo_opcode = xrt::bo(device, sizeof(int) * edge_buffer_len, update_message_generator_kernel.group_id(3));
    std::cout << "Generator buffers for edge operations allocated ("
              << (sizeof(int)*edge_buffer_len/1024) << "KB each)." << std::endl;
    int* src_ptr = bo_src.map<int*>();
    int* dst_ptr = bo_dst.map<int*>();
    int* opcode_ptr = bo_opcode.map<int*>();

#ifdef USE_STANDALONE_MODE
    // Edge buffer for redistribution (collect_edge_to_buffer / redistribute_edge_from_hbm)
    // hbm_buffer_for_red 是 redistribution_kernel 的第 2 个 m_axi 端口 (group_id 1)
    constexpr size_t kEdgeBufWords = 4096;  // ≥ ceil(1024 * 63 / 16)
    constexpr size_t kEdgeBufBytes = kEdgeBufWords * 64;
    std::vector<xrt::bo> bo_edge_bufs;
    bo_edge_bufs.reserve(CHANNEL_NUM);
    for (int ch = 0; ch < CHANNEL_NUM; ++ch) {
        bo_edge_bufs.emplace_back(device, kEdgeBufBytes,
                                   XRT_BO_FLAGS_DEV_ONLY,
                                   redistribution_kernels[ch].group_id(1));
    }
#if defined(USE_MERGE_PATH) || defined(USE_MERGE_BS_PATH)
    // merge_pipeline 的归并缓冲 (hbm_buffer, 独立备用 bank HBM[10/11/12])
    std::vector<xrt::bo> bo_merge_bufs;
    bo_merge_bufs.reserve(CHANNEL_NUM);
    for (int ch = 0; ch < CHANNEL_NUM; ++ch) {
#if defined(USE_MERGE_BS_PATH)
        bo_merge_bufs.emplace_back(device, kEdgeBufBytes,
                                   XRT_BO_FLAGS_DEV_ONLY,
                                   merge_pipeline_kernels[ch].group_id(1));
#else
        bo_merge_bufs.emplace_back(device, kEdgeBufBytes,
                                   XRT_BO_FLAGS_DEV_ONLY,
                                   bs_insert_kernels[ch].group_id(1));
#endif
    }
#endif
    (void)edge_buffer_len;
#endif

    std::cout << "[DBG] L335: Allocating COO buffers, buffer_len=" << buffer_len << " (" << (sizeof(int)*buffer_len/1024/1024) << "MB each)" << std::endl;
    auto bo_coo_src = xrt::bo(device, sizeof(int) * buffer_len, check_kernel.group_id(0));
    auto bo_coo_dst = xrt::bo(device, sizeof(int) * buffer_len, check_kernel.group_id(1));
    int* coo_src_ptr = bo_coo_src.map<int*>();
    int* coo_dst_ptr = bo_coo_dst.map<int*>();
    auto bo_num_edges = xrt::bo(device, sizeof(int), check_kernel.group_id(2));

    int* bo_num_edges_ptr = bo_num_edges.map<int*>();

    // bitmap_ptrs now set up in m_axi buffer section above
    // for (int i = 0; i < CHANNEL_NUM; ++i)
    // {
      // bitmap now lives inside the m_axi buffer at offset pma_bm_start — no separate allocation
    // }

    auto end_buffer = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_buffer = end_buffer - start_buffer;
    std::cout << "Buffer allocation and initialization time: " << elapsed_buffer.count() << " s" << std::endl;

    std::cout << "Buffers allocated and initialized." << std::endl;

    // --- Set kernel arguments for m_axi ports ---
    for (int ch = 0; ch < CHANNEL_NUM; ++ch) {
#ifdef USE_STANDALONE_MODE
#if defined(USE_MERGE_BS_PATH)
        // bs_insert: hbm_bs, hbm_rw (同 channel bank)
        {
            auto& run = run_pma_insert_edge_kernels[ch];
            int a = 0;
            run.set_arg(a++, ch_bufs[ch]);  // hbm_bs
            run.set_arg(a++, ch_bufs[ch]);  // hbm_rw
            run.set_arg(a++, pma_raw_data_start);
            run.set_arg(a++, pma_vecinfo_start);
        }
        // merge_pipeline: hbm_pma, hbm_buffer (独立备用 bank)
        {
            auto& run = run_merge_pipeline_kernels[ch];
            int a = 0;
            run.set_arg(a++, ch_bufs[ch]);       // hbm_pma
            run.set_arg(a++, bo_merge_bufs[ch]); // hbm_buffer
            run.set_arg(a++, pma_raw_data_start);
            run.set_arg(a++, pma_vecinfo_start);
        }
        // task_reader: worker_id + gate m_axi (读 VEC_INFO/header/VC 供决策)
        {
            auto& run = run_task_reader_kernels[ch];
            int a = 0;
            run.set_arg(a++, ch);             // worker_id
            run.set_arg(a++, ch_bufs[ch]);    // hbm_gate (同 channel bank)
            run.set_arg(a++, pma_raw_data_start);
            run.set_arg(a++, pma_vecinfo_start);
        }
#else
        // 插入内核: bs_insert (hbm_bs/hbm_rw) 或 merge_pipeline (hbm_pma/hbm_buffer)
        {
            auto& run = run_pma_insert_edge_kernels[ch];
            int a = 0;
            run.set_arg(a++, ch_bufs[ch]);  // hbm_pma (merge) / hbm_bs (bs)
#ifdef USE_MERGE_PATH
            run.set_arg(a++, bo_merge_bufs[ch]);  // hbm_buffer (merge 归并缓冲, 独立 bank)
#else
            run.set_arg(a++, ch_bufs[ch]);  // hbm_rw (bs)
#endif
            run.set_arg(a++, pma_raw_data_start);
            run.set_arg(a++, pma_vecinfo_start);
        }
        // task_reader: 纯流, 只有 worker_id 标量
        {
            auto& run = run_task_reader_kernels[ch];
            run.set_arg(0, ch);             // worker_id
        }
#endif
        // redistribution: hbm_red, hbm_buffer_for_red + 标量
        {
            auto& run = run_redistribution_kernels[ch];
            int a = 0;
            run.set_arg(a++, ch_bufs[ch]);       // hbm_red
            run.set_arg(a++, bo_edge_bufs[ch]);  // hbm_buffer_for_red
            run.set_arg(a++, pma_raw_data_start);
            run.set_arg(a++, pma_vecinfo_start);
            run.set_arg(a++, pma_bm_start);      // bitmap_offset
            // pre_expand_in/task_in/pre_ack_out/redist_ack_out/alloc_cmd/rsp/bitmap_init: axis, no host set
        }
        std::cout << "[DBG] L373: split kernels args set for " << CHANNEL_NUM << " channels" << std::endl;
#else
        auto& run = run_pma_insert_edge_kernels[ch];
        int a = 0;
        run.set_arg(a++, ch_bufs[ch]);  // hbm_gate
        run.set_arg(a++, ch_bufs[ch]);  // hbm_bs
        run.set_arg(a++, ch_bufs[ch]);  // hbm_rw
        run.set_arg(a++, ch_bufs[ch]);  // hbm_red
        std::cout << "[DBG] L373: insert_edge kernel args set (4 ports + 2 offsets) for " << CHANNEL_NUM << " channels" << std::endl;
        run.set_arg(a++, pma_raw_data_start);
        run.set_arg(a++, pma_vecinfo_start);
#endif
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
    // Merger has no s_axilite args — edge count forwarded to check_kernel via stream

    // alloc_access kernel args
#ifdef USE_STANDALONE_MODE
    // alloc_access has one scalar control argument; all bitmap/cmd/rsp ports
    // remain AXIS connections in system_merge_bs.cfg.
#else
    // Online mode uses the same AXIS-only allocator interface.
#endif

    bool read_pipeline_started = false;
    auto ensure_read_pipeline_started = [&]() {
        if (read_pipeline_started) return;
        std::cout << "Starting read pipeline on first verification request..." << std::endl;
        for (auto &run_worker : run_pma_read_worker_kernels) run_worker.start();
        run_pma_read_merger_kernel.start();
        run_pma_read_router_kernel.start();
        read_pipeline_started = true;
    };

    auto verify_graph = [&](const std::vector<std::vector<int>>& expected_graph, int current_max_vertex, const std::string& label, bool last) -> double {
        std::cout << "Verifying " << label << "..." << std::endl;
        int check_vertex_range = std::max(current_max_vertex, base_max_vertex) + 1;
        std::vector<std::vector<int>> Updated_Graph(check_vertex_range);

        auto verify_start = std::chrono::high_resolution_clock::now();
        double kernel_time = 0.0;
        if (host_verify) {
            // Host-only verification: synchronize the authoritative PMA HBM
            // image and decode header/segment payloads directly on the CPU.
            // This deliberately does not start the pma_read/check pipeline.
            for (int ch = 0; ch < CHANNEL_NUM; ++ch)
                ch_bufs[ch].sync(XCL_BO_SYNC_BO_FROM_DEVICE);
            int pma_limit = (check_vertex_range + VERTEX_PER_PMA - 1) / VERTEX_PER_PMA;
            for (int pma_idx = 0; pma_idx < pma_limit; ++pma_idx) {
                int ch = pma_idx % CHANNEL_NUM;
                VEC_INFO vi = vec_info_ptrs[ch][pma_idx];
                int start = (int)vi.range(31, 0);
                int len = (int)vi.range(63, 32);
                if (len <= 0 || start < 0) continue;
                PMA_HEADER header;
                memcpy(&header, hbm_base_addresses[ch] + start, sizeof(PMA_HEADER));
                int total_segs = header.vertex_range[VERTEX_PER_PMA - 1];
                if (total_segs <= 0) continue;
                int hlp_words = fe_words_for(total_segs) + vc_words_for(total_segs);
                int* payload = reinterpret_cast<int*>(hbm_base_addresses[ch] + start
                    + sizeof(PMA_HEADER) + hlp_words * 64);
                for (int v = 0; v < VERTEX_PER_PMA; ++v) {
                    int src = pma_idx * VERTEX_PER_PMA + v;
                    if (src < 0 || src >= check_vertex_range) continue;
                    int seg_start = v == 0 ? 0 : header.vertex_range[v - 1];
                    int seg_end = header.vertex_range[v];
                    for (int s = seg_start; s < seg_end; ++s) {
                        int vc = payload[s * SEGMENT_SIZE + VALID_COUNT_IDX];
                        if (vc < 0) vc = 0;
                        if (vc > EDGE_SLOTS_PER_SEG) vc = EDGE_SLOTS_PER_SEG;
                        for (int k = 0; k < vc; ++k) {
                            int dst = payload[s * SEGMENT_SIZE + k];
                            if (dst != UNUSED_ICON) Updated_Graph[src].push_back(dst);
                        }
                    }
                }
            }
            auto verify_end = std::chrono::high_resolution_clock::now();
            // Continue through the common comparison/reporting path below.
            for (int i = 0; i < check_vertex_range; ++i) {
                std::sort(Updated_Graph[i].begin(), Updated_Graph[i].end());
                Updated_Graph[i].erase(std::unique(Updated_Graph[i].begin(), Updated_Graph[i].end()), Updated_Graph[i].end());
            }
            // The comparison/reporting block below owns final_match and
            // mismatch files; return timing after it completes.
            (void)verify_start;
            (void)verify_end;
        } else {
            ensure_read_pipeline_started();

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

        // mode=3: get total edge count via read pipeline (merger writes to bo_num_edges via m_axi)
        run_check_kernel.start();
        run_check_kernel.wait();

        bo_num_edges.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        int edge_num = bo_num_edges_ptr[0];
        std::cout << "Check kernel returned edge count: " << edge_num << std::endl;

        // mode=1: COO dump to verify graph content
        run_check_kernel.set_arg(4, 1);
        run_check_kernel.set_arg(5, last);
        run_check_kernel.start();
        run_check_kernel.wait();

        bo_coo_src.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        bo_coo_dst.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

        auto k_end = std::chrono::high_resolution_clock::now();
        kernel_time = std::chrono::duration<double>(k_end - k_start).count();
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
        return host_verify
            ? std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - verify_start).count()
            : kernel_time;
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

        long long current_addr_offset[CHANNEL_NUM];
        for (int i = 0; i < CHANNEL_NUM; ++i)
        {
            current_addr_offset[i] = 0;
        }
        // std::ofstream debug_pma("debug_pma_content.txt");

        // 3. 构建 PMA 结构
        for (int pma_idx = 0; pma_idx < total_pma_num; ++pma_idx)
        {
            BYTE* current_base_addr = hbm_base_addresses[pma_idx % CHANNEL_NUM];
            long long* current_offset = &current_addr_offset[pma_idx % CHANNEL_NUM];
            int channel = pma_idx % CHANNEL_NUM;
            // POS Direction
            {
                PMA_HEADER pos_header = {0};
                std::vector<int> fe_entries;
                std::vector<int> vc_entries;
                std::vector<int> segment_data;
                
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
                        fe_entries.push_back(valid_num > 0 ? neighbors[neighbor_idx] : UNUSED_ICON);
                        vc_entries.push_back(valid_num);
                        for (int k = 0; k < valid_num; ++k) {
                            segment_data.push_back(neighbors[neighbor_idx++]);
                        }
                        for (int k = valid_num; k < EDGE_SLOTS_PER_SEG; ++k) {
                            segment_data.push_back(UNUSED_ICON);
                        }
                        // Slot VALID_COUNT_IDX (63) = valid_count
                        segment_data.push_back(valid_num);
                    }
                }

                // FPGA layout is header + FE SoA + VC SoA + segment payload.
                // Pad FE/VC to complete 512-bit words (16 int entries/word).
                const int helper_words = (pos_header.vertex_range[VERTEX_PER_PMA - 1]
                                          + FE_SOA_ENTRIES_PER_WORD - 1)
                                         / FE_SOA_ENTRIES_PER_WORD;
                std::vector<int> pos_data;
                pos_data.reserve((helper_words * 2 * FE_SOA_ENTRIES_PER_WORD)
                                 + segment_data.size());
                fe_entries.resize(helper_words * FE_SOA_ENTRIES_PER_WORD, UNUSED_ICON);
                vc_entries.resize(helper_words * VC_SOA_ENTRIES_PER_WORD, 0);
                pos_data.insert(pos_data.end(), fe_entries.begin(), fe_entries.end());
                pos_data.insert(pos_data.end(), vc_entries.begin(), vc_entries.end());
                pos_data.insert(pos_data.end(), segment_data.begin(), segment_data.end());
                
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
                vec_info_ptrs[channel][pma_idx] = (ap_uint<64>(total_len) << 32) | (ap_uint<32>)*current_offset;
                
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
            long long total_blocks = (current_addr_offset[ch] + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
            allocator_initial_segments[ch] = static_cast<int>(
                total_blocks % ALLOC_BITMAP_TOTAL_BLOCKS);
            long long full_bytes = total_blocks / 8;
            int rem_bits = (int)(total_blocks % 8);
            memset(bitmap_ptrs[ch], 0xFF, full_bytes);
            if (rem_bits > 0)
                bitmap_ptrs[ch][full_bytes] = (BYTE)((1 << rem_bits) - 1);
        }

        // Update mem_access kernel arg 2: total PMA count
        for (int i = 0; i < CHANNEL_NUM; ++i)

        // Sync modified buffers to device
        for (int ch = 0; ch < CHANNEL_NUM; ++ch) { ch_bufs[ch].sync(XCL_BO_SYNC_BO_TO_DEVICE); }
#ifndef USE_STANDALONE_MODE
        for (int ch = 0; ch < CHANNEL_NUM; ++ch) { bo_bitmap[ch].sync(XCL_BO_SYNC_BO_TO_DEVICE); }
#endif

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

#ifdef USE_STANDALONE_MODE
    // --- Standalone (split): ONLINE path only ---
    //   边走 generator→router_stream→task_reader.online_in, 无 task_buffer/DIRECT。
    auto load_edge_batch = [&](const std::vector<Operation>& batch_ops) -> int {
        // ---- ONLINE path only: fill generator buffers ----
        int count = std::min((int)batch_ops.size(), (int)edge_buffer_len);
        for (int i = 0; i < count; i++) {
            src_ptr[i] = batch_ops[i].src;
            dst_ptr[i] = batch_ops[i].dst;
            opcode_ptr[i] = batch_ops[i].opcode;
        }
        std::cout << "[standalone] online edges: " << count << std::endl;
        return count;
    };

    // --- Standalone (split): start generator+router+3 类型内核+alloc_access, wait for completion ---
    auto run_update_batch = [&](int gen_edge_count, const std::string& label, bool last) -> double {
        auto phase_start = std::chrono::high_resolution_clock::now();

        // 参数已在初始化阶段 set (worker_id/offsets/bitmap_offset 每批不变), 无需重设。

        // Start generator + router stream for online updates.
        // 每批都启动: 中间批次 (last=0) generator 发 FLUSH_CACHE (router 广播 + ACK),
        // 最后批次 (last=1) 发 STOP (router 转发 + ACK) — 两者都被 ACK, 不会卡死。
        xrt::run run_generator(update_message_generator_kernel);
        xrt::run run_router_stream(update_message_router_stream_kernel);
        bo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        bo_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        bo_opcode.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        run_generator.set_arg(0, gen_edge_count);
        run_generator.set_arg(1, bo_src);
        run_generator.set_arg(2, bo_dst);
        run_generator.set_arg(3, bo_opcode);
        run_generator.set_arg(4, static_cast<int>(last));
        run_generator.start();
        run_router_stream.start();

        // Start alloc_access kernels, then task_reader + bs_insert (+merge) + redistribution
        for (auto &r : run_alloc_access_kernels) { r.start(); }
        for (auto &r : run_task_reader_kernels) { r.start(); }
        for (auto &r : run_pma_insert_edge_kernels) { r.start(); }
#if defined(USE_MERGE_BS_PATH)
        for (auto &r : run_merge_pipeline_kernels) { r.start(); }
#endif
        for (auto &r : run_redistribution_kernels) { r.start(); }

        // Wait for generator (finishes after sending STOP + receiving ACK from router)
        run_generator.wait();
        // Wait for router stream (exits after forwarding STOP to all workers + ACK)
        run_router_stream.wait();
        // Wait for workers (STOP 链: task_reader → bs_insert/merge_pipeline → redistribution 依次收尾)
        for (auto &r : run_task_reader_kernels) { r.wait(); }
        for (auto &r : run_pma_insert_edge_kernels) { r.wait(); }
#if defined(USE_MERGE_BS_PATH)
        for (auto &r : run_merge_pipeline_kernels) { r.wait(); }
#endif
        for (auto &r : run_redistribution_kernels) { r.wait(); }
        // Wait for alloc_access (receives STOP from redistribution)
        for (auto &r : run_alloc_access_kernels) { r.wait(); }

        auto phase_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = phase_end - phase_start;
        std::cout << label << " edges=" << gen_edge_count
                  << ", time=" << elapsed.count() << " s (split standalone)" << std::endl;
        return elapsed.count();
    };
#else
    // --- Original: load edges into generator buffers ---
    auto load_edge_batch = [&](const std::vector<Operation>& batch_ops) -> int {
        size_t edge_idx = 0;
        for (const auto &op : batch_ops)
        {
            // if (op.opcode == PMA_UPDATE_INSERT_VERTEX)
            // {
            //     continue;
            // }
            // std::cout << "[DBG] L680: Starting edge update batches, total_ops=" << operations.size() << std::endl;
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
#endif

    // 1. Load base graph into HBM (ts == -1 edges) BEFORE starting kernels
    if (base_max_vertex >= 0) {
        if (!add_base_graph_to_hbm(base_max_vertex + 1, base_edge_count)) {
            std::cerr << "Failed to load base graph into HBM." << std::endl;
            return 1;
        }
    } else {
    std::cout << "[DBG] L734: All kernels starting (insert, router, read, merger)..." << std::endl;
        std::cout << "No base graph (ts==-1), skipping PMA preload." << std::endl;
        // Zero-initialize bitmap: all blocks free since no PMA data loaded
        for (int ch = 0; ch < CHANNEL_NUM; ++ch) {
            memset(bitmap_ptrs[ch], 0, BM_SIZE);
        }
        for (int ch = 0; ch < CHANNEL_NUM; ++ch) { ch_bufs[ch].sync(XCL_BO_SYNC_BO_TO_DEVICE); }
#ifndef USE_STANDALONE_MODE
        for (int ch = 0; ch < CHANNEL_NUM; ++ch) { bo_bitmap[ch].sync(XCL_BO_SYNC_BO_TO_DEVICE); }
#endif
    }

    // Base PMAs are packed sequentially per channel.  The first free segment
    // is therefore the best initial next-fit cursor for that channel.
    for (int ch = 0; ch < static_cast<int>(run_alloc_access_kernels.size()); ++ch) {
        run_alloc_access_kernels[ch].set_arg(0, allocator_initial_segments[ch]);
    }

    // 2. Start Long-Running (Keep Alive) Kernels
#ifdef USE_STANDALONE_MODE
    // Standalone: insert_edge + alloc_access started per-batch (not keep-alive)
    // Read pipeline is started lazily by ensure_read_pipeline_started() when
    // verification actually requests a read.
#else
    for (auto &r : run_alloc_access_kernels) { r.start(); }
    for (auto &r : run_pma_insert_edge_kernels) { r.start(); }
    run_update_message_router_kernel.start();
    bo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_opcode.sync(XCL_BO_SYNC_BO_TO_DEVICE);
#endif

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

#ifndef USE_STANDALONE_MODE
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
#endif
#ifdef USE_STANDALONE_MODE
    // Standalone: 通过 generator→router_stream→worker 发 STOP (last=1)。
    // task_reader 的收波循环阻塞等 STOP — 任何模式下都必须发, 否则 worker 卡死。
    auto send_stop_to_workers = [&]() {
        auto run_g = xrt::run(update_message_generator_kernel);
        run_g.set_arg(0, 0); run_g.set_arg(1, bo_src);
        run_g.set_arg(2, bo_dst); run_g.set_arg(3, bo_opcode);
        run_g.set_arg(4, 1);  // last=1
        auto run_r = xrt::run(update_message_router_stream_kernel);
        run_g.start(); run_r.start();
        run_g.wait(); run_r.wait();
    };
#endif
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
#ifndef USE_STANDALONE_MODE
        send_update_stop_only();
#else
        // Standalone: 总是发 STOP 关闭 worker 的 online 流 —
        // task_reader 的收波循环 (online_in.read) 阻塞等 STOP, 不发则死锁。
        send_stop_to_workers();
#endif
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
                            first_vtx = std::stoi(line.substr(19));
                            break;
                        }
                    }
                    mf.close();
                    if (first_vtx >= 0) {
                        int pma_idx = first_vtx / VERTEX_PER_PMA;
                        int ch = pma_idx % CHANNEL_NUM;

                        // Sync HBM + vecinfo back from device
                        for (int ch = 0; ch < CHANNEL_NUM; ++ch) { ch_bufs[ch].sync(XCL_BO_SYNC_BO_FROM_DEVICE); }

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
#ifdef USE_STANDALONE_MODE
    // 无增量批次 (纯 base 图): else 分支的循环未执行, 未发过 STOP — 补发,
    // 否则 worker 的 task_reader 收波循环 (online_in.read) 永远等不到 STOP。
    if (operations.empty()) send_stop_to_workers();
#endif

    std::cout << "Waiting for long-running kernels to finish..." << std::endl;
#ifndef USE_STANDALONE_MODE
    run_update_message_router_kernel.wait();
    for (auto &r : run_pma_insert_edge_kernels)
    {
        r.wait();
    }
    for (auto &r : run_alloc_access_kernels)
    {
        r.wait();
    }
#else
    // Standalone: insert_edge + alloc_access already exited per-batch;
    // nothing extra to wait on here.
#endif
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
    if (read_pipeline_started) {
        run_pma_read_router_kernel.wait();
        for (auto &run_worker : run_pma_read_worker_kernels) run_worker.wait();
        run_pma_read_merger_kernel.wait();
    }

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
