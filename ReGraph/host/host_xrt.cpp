#include "graph.h"
#include "graph_loader.h"
#include "graph_preprocess.h"
#include "host_config.h"
#include "partition_schedule.h"
#include "verification/verify.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <xrt/xrt_bo.h>
#include <xrt/xrt_device.h>
#include <xrt/xrt_kernel.h>

namespace {

size_t round_up(size_t value, size_t align) {
    return (value + align - 1) & ~(align - 1);
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <xclbin> <graph> [num_dense_partitions]" << std::endl;
        return 1;
    }

    std::string xclbin_name = argv[1];
    std::string graph_path = argv[2];

    std::string path_graph_dataset = "";

    // 1) Read graph and preprocess
    auto start_init = std::chrono::high_resolution_clock::now();

    Graph *gptr = createGraph(graph_path, path_graph_dataset);
    CSR *csr = createCsr(gptr);

// #if VERTEX_REORDER_ENABLE == 1
    // reorderGraph(csr);
// #endif

    initializeProperty(csr);

    size_t nonzero_props = 0;
    const int max_print = std::min(16, csr->vertexNum);
    std::cout << "[DBG] vProps[0.." << (max_print - 1) << "]: ";
    for (int i = 0; i < csr->vertexNum && i < max_print; ++i) {
        std::cout << csr->vProps[i] << " ";
        if (csr->vProps[i] != 0) {
            nonzero_props++;
        }
    }
    for (int i = max_print; i < csr->vertexNum; ++i) {
        if (csr->vProps[i] != 0) {
            nonzero_props++;
        }
    }
    std::cout << "| nonzero=" << nonzero_props << std::endl;

    partition_container_dt partition_container = partitionGraph(csr);

    int num_dense = 1;
    if (argc > 3) {
        num_dense = std::atoi(argv[3]);
    }
    partition_container.num_dense_partitions = num_dense;

    schedulePartitions(partition_container);

    auto end_init = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_init = end_init - start_init;
    std::cout << "Initialization time: " << elapsed_init.count() << " s" << std::endl;

    // 2) Instantiate kernels
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin(xclbin_name);

    auto edge_reader_kernel = xrt::kernel(device, uuid, "edgeReader:{edgeReader_1}");
    std::vector<xrt::kernel> little_kernels;
    little_kernels.reserve(LITTLE_KERNEL_NUM);
    for (int i = 0; i < LITTLE_KERNEL_NUM; ++i) {
        const std::string cu_id = std::to_string(i + 1);
        const std::string name = "littleKernelScatterGather:{littleKernelScatterGather_" + cu_id + "}";
        little_kernels.emplace_back(device, uuid, name);
    }

    std::vector<xrt::kernel> big_kernels;
    big_kernels.reserve(BIG_KERNEL_NUM);
    for (int i = 0; i < BIG_KERNEL_NUM; ++i) {
        const std::string cu_id = std::to_string(i + 1);
        const std::string name = "bigKernelScatterGather:{bigKernelScatterGather_" + cu_id + "}";
        big_kernels.emplace_back(device, uuid, name);
    }
    auto little_merger_kernel = xrt::kernel(device, uuid, "kernelLittleGSMerger:{kernelLittleGSMerger_1}");
    auto big_merger_kernel = xrt::kernel(device, uuid, "kernelBigGSMerger:{kernelBigGSMerger_1}");
    auto hbm_kernel = xrt::kernel(device, uuid, "kernelHBMWrapper:{kernelHBMWrapper_1}");
    auto apply_kernel = xrt::kernel(device, uuid, "kernelApply:{kernelApply_1}");

    std::cout << "Kernels instantiated successfully." << std::endl;
    std::cout << "[DBG] edgeReader arg2 group_id=" << edge_reader_kernel.group_id(2)
              << " LITTLE_KERNEL_DST_BUFFER_SIZE=" << LITTLE_KERNEL_DST_BUFFER_SIZE
              << " BIG_KERNEL_DST_BUFFER_SIZE=" << BIG_KERNEL_DST_BUFFER_SIZE << std::endl;
    // 3) Create buffers
    const size_t prop_size = partition_container.vertex_property.size();
    const size_t prop_bytes = round_up(prop_size * sizeof(uint), 64);

    auto bo_src_prop_1 = xrt::bo(device, prop_bytes, hbm_kernel.group_id(0));
    auto bo_src_prop_2 = xrt::bo(device, prop_bytes, hbm_kernel.group_id(1));
    auto bo_new_prop_1 = xrt::bo(device, prop_bytes, hbm_kernel.group_id(2));
    auto bo_new_prop_2 = xrt::bo(device, prop_bytes, hbm_kernel.group_id(3));

    auto bo_apply_src_prop = xrt::bo(device, prop_bytes, apply_kernel.group_id(0));

    auto src_prop_1 = bo_src_prop_1.map<uint *>();
    auto src_prop_2 = bo_src_prop_2.map<uint *>();
    auto new_prop_1 = bo_new_prop_1.map<uint *>();
    auto new_prop_2 = bo_new_prop_2.map<uint *>();

    std::fill(new_prop_1, new_prop_1 + prop_size, 0);
    std::fill(new_prop_2, new_prop_2 + prop_size, 0);

    std::copy(partition_container.vertex_property.begin(),
              partition_container.vertex_property.end(),
              src_prop_1);
    std::copy(partition_container.vertex_property.begin(),
              partition_container.vertex_property.end(),
              src_prop_2);

    auto apply_src_prop = bo_apply_src_prop.map<uint *>();
    std::copy(partition_container.vertex_property.begin(),
              partition_container.vertex_property.end(),
              apply_src_prop);

    bo_src_prop_1.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_src_prop_2.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_new_prop_1.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_new_prop_2.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_apply_src_prop.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    // 4) Set kernel arguments
    auto run_hbm = xrt::run(hbm_kernel);
    run_hbm.set_arg(0, bo_src_prop_1);
    run_hbm.set_arg(1, bo_src_prop_2);
    run_hbm.set_arg(2, bo_new_prop_1);
    run_hbm.set_arg(3, bo_new_prop_2);
    run_hbm.set_arg(4, static_cast<uint>(partition_container.num_dense_partitions));
    run_hbm.set_arg(5, static_cast<uint>(partition_container.num_sparse_partitions));

    auto run_apply = xrt::run(apply_kernel);
    run_apply.set_arg(0, bo_apply_src_prop);
    run_apply.set_arg(1, static_cast<uint>(partition_container.num_dense_partitions));
    run_apply.set_arg(2, static_cast<uint>(partition_container.num_sparse_partitions));
    run_apply.set_arg(3, 0u);

    auto run_edge_reader = xrt::run(edge_reader_kernel);
    auto run_little_merger = xrt::run(little_merger_kernel);
    auto run_big_merger = xrt::run(big_merger_kernel);

    std::cout << "[DBG] buffer initialed!" << std::endl;
    // 5) Compute: start long-running kernels once
    // ap_ctrl_none kernels are free-running in this flow; start them once here.
    // run_hbm.start();
    // run_apply.start();
    if (LITTLE_KERNEL_NUM > 0) {
        run_little_merger.start();
    }
    if (BIG_KERNEL_NUM > 0) {
        run_big_merger.start();
    }

    auto run_edge_reader_partition = [&](std::vector<xrt::bo> &edge_reader_bos,
                                         std::vector<xrt::run> &edge_reader_runs,
                                         std::vector<xrt::run> &little_runs,
                                         std::vector<xrt::run> &big_runs,
                                         const std::vector<uint, aligned_allocator<uint>> &edge_array_host,
                                         uint little_dst_offset,
                                         uint big_dst_offset,
                                         uint little_kernel_id,
                                         uint big_kernel_id) {
        const size_t pair_count = edge_array_host.size() / 2;
        std::cout << "[DBG] edge_array_host pairs=" << pair_count
                  << " little_dst_offset=" << little_dst_offset
                  << " big_dst_offset=" << big_dst_offset << std::endl;
        for (size_t i = 0; i < 4 && (i * 2 + 1) < edge_array_host.size(); ++i) {
            std::cout << "[DBG] pair[" << i << "] src=" << edge_array_host[i * 2]
                      << " dst=" << edge_array_host[i * 2 + 1] << std::endl;
        }
        size_t little_in_range = 0;
        size_t big_in_range = 0;
        for (size_t i = 0; i + 1 < edge_array_host.size(); i += 2) {
            uint dst = edge_array_host[i + 1];
            if (dst >= little_dst_offset && dst < little_dst_offset + LITTLE_KERNEL_DST_BUFFER_SIZE) {
                little_in_range++;
            }
            if (dst >= big_dst_offset && dst < big_dst_offset + BIG_KERNEL_DST_BUFFER_SIZE) {
                big_in_range++;
            }
        }
        std::cout << "[DBG] in_range little=" << little_in_range
                  << " big=" << big_in_range << std::endl;

        std::vector<uint, aligned_allocator<uint>> coo_array(edge_array_host.begin(), edge_array_host.end());

        edge_reader_bos.emplace_back(device, coo_array.size() * sizeof(uint), edge_reader_kernel.group_id(2));
        auto &bo_coo = edge_reader_bos.back();
        auto coo_ptr = bo_coo.map<uint *>();
        std::copy(coo_array.begin(), coo_array.end(), coo_ptr);

        std::cout << "[DBG] coo_ptr pairs (first 4): ";
        const size_t dbg_pairs = std::min<size_t>(4, coo_array.size() / 2);
        for (size_t i = 0; i < dbg_pairs; ++i) {
            std::cout << "(" << coo_ptr[i * 2] << "," << coo_ptr[i * 2 + 1] << ") ";
        }
        if (coo_array.size() >= 2) {
            size_t last_idx = coo_array.size() / 2 - 1;
            std::cout << "| last=(" << coo_ptr[last_idx * 2] << "," << coo_ptr[last_idx * 2 + 1] << ")";
        }
        std::cout << std::endl;
        
        bo_coo.sync(XCL_BO_SYNC_BO_TO_DEVICE);

        edge_reader_runs.emplace_back(edge_reader_kernel);
        auto &edge_run = edge_reader_runs.back();
        edge_run.set_arg(0, little_dst_offset);
        edge_run.set_arg(1, big_dst_offset);
        edge_run.set_arg(2, bo_coo);

        std::cout << "[DBG] start kernels for partition" << std::endl;
        if (
            // LITTLE_KERNEL_NUM > 0 &&
             little_kernel_id < little_kernels.size()) {
            little_runs.emplace_back(little_kernels[little_kernel_id]);
            auto &run_little = little_runs.back();
            run_little.set_arg(0, little_dst_offset);
            run_little.start();
            std::cout << "[DBG] littleKernelScatterGather start" << std::endl;
        }
        if (
            // BIG_KERNEL_NUM > 0 && 
            big_kernel_id < big_kernels.size()) {
            big_runs.emplace_back(big_kernels[big_kernel_id]);
            auto &run_big = big_runs.back();
            run_big.set_arg(0, big_dst_offset);
            run_big.start();
            std::cout << "[DBG] bigKernelScatterGather start" << std::endl;
        }
        edge_run.start();
        std::cout << "[DBG] edgeReader start" << std::endl;
    };

    const uint invalid_dst_offset = 0x80000000u;
    int num_super_step = 5;
    bool use_a_as_src = true;
    for (int super_step = 0; super_step < num_super_step; super_step++) {
        std::cout << "Super step " << super_step << std::endl;
        auto &src_prop_bo_1 = use_a_as_src ? bo_src_prop_1 : bo_new_prop_1;
        auto &src_prop_bo_2 = use_a_as_src ? bo_src_prop_2 : bo_new_prop_2;
        auto &dst_prop_bo_1 = use_a_as_src ? bo_new_prop_1 : bo_src_prop_1;
        auto &dst_prop_bo_2 = use_a_as_src ? bo_new_prop_2 : bo_src_prop_2;
        auto *src_prop_ptr = use_a_as_src ? src_prop_1 : new_prop_1;

        run_hbm.set_arg(0, src_prop_bo_1);
        run_hbm.set_arg(1, src_prop_bo_2);
        run_hbm.set_arg(2, dst_prop_bo_1);
        run_hbm.set_arg(3, dst_prop_bo_2);
        std::copy(src_prop_ptr, src_prop_ptr + prop_size, apply_src_prop);
        bo_apply_src_prop.sync(XCL_BO_SYNC_BO_TO_DEVICE);

        run_hbm.start();
        run_apply.start();

        std::vector<xrt::bo> edge_reader_bos;
        std::vector<xrt::run> edge_reader_runs;
        std::vector<xrt::run> little_runs;
        std::vector<xrt::run> big_runs;

        for (uint i = 0; i < partition_container.num_dense_partitions; i++) {
            for (uint subpart_id = 0; subpart_id < partition_container.DP[i].num_subpartitions; subpart_id++) {
                uint part_dst_offset = partition_container.DP[i].subP[subpart_id].dst_offset;
                const uint krnl_id = partition_container.DP[i].subP[subpart_id].kernel_id;
                run_edge_reader_partition(edge_reader_bos,
                                          edge_reader_runs,
                                          little_runs,
                                          big_runs,
                                          partition_container.DP[i].subP[subpart_id].edge_array_host,
                                          part_dst_offset,
                                          invalid_dst_offset,
                                          krnl_id,
                                          UINT32_MAX);
            }
        }

        for (uint i = 0; i < partition_container.num_sparse_partitions; i++) {
            for (uint subpart_id = 0; subpart_id < partition_container.SP[i].num_subpartitions; subpart_id++) {
                uint part_dst_offset = partition_container.SP[i].subP[subpart_id].dst_offset;
                const uint krnl_id = partition_container.SP[i].subP[subpart_id].kernel_id;
                const uint big_kernel_id = krnl_id - LITTLE_KERNEL_NUM;
                run_edge_reader_partition(edge_reader_bos,
                                          edge_reader_runs,
                                          little_runs,
                                          big_runs,
                                          partition_container.SP[i].subP[subpart_id].edge_array_host,
                                          invalid_dst_offset,
                                          part_dst_offset,
                                          UINT32_MAX,
                                          big_kernel_id);
            }
        }

        for (auto &run : little_runs) {
            run.wait();
            std::cout << "[DBG] littleKernelScatterGather done" << std::endl;
        }
        for (auto &run : big_runs) {
            run.wait();
            std::cout << "[DBG] bigKernelScatterGather done" << std::endl;
        }
        for (auto &run : edge_reader_runs) {
            run.wait();
            std::cout << "[DBG] edgeReader done" << std::endl;
        }

        run_apply.wait();
        run_hbm.wait();

        dst_prop_bo_1.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        dst_prop_bo_2.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        use_a_as_src = !use_a_as_src;
    }
    // run_big_merger.wait();
    // run_little_merger.wait();
    // 6) Sync results
    bo_new_prop_1.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    bo_new_prop_2.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    std::cout << "Result sample (new_prop_1[0..7]): ";
    for (int i = 0; i < 8 && i < static_cast<int>(prop_size); ++i) {
        std::cout << new_prop_1[i] << " ";
    }
    std::cout << std::endl;

    verifyResultsXrt(partition_container, csr, new_prop_1);

    return 0;
}
