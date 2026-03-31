#include "data_type.h"
#include "deprecated/xrt.h"
#include "graph.h"
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
#include "ReGraph/host/graph_loader/graph.h"
#include "ReGraph/host/graph_loader/graph_loader.h"
#include "ReGraph/host/host_config/host_data_types.h"
#include "ReGraph/host/preprocess/graph_preprocess.h"
#include "ReGraph/host/preprocess/partition_schedule.h"
#include "ReGraph/host/verification/verify.h"

size_t round_up(size_t value, size_t align) {
    return (value + align - 1) & ~(align - 1);
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
    
    
  
    Graph *gptr = createGraph(graph_path, "");
    CSR *csr = createCsr(gptr);
    std::vector<std::vector<int>> Graph;

    initializeProperty(csr);

    partition_container_dt partition_container = partitionGraph(csr);
    partition_container.num_dense_partitions = 1;
    schedulePartitions(partition_container);
    int max_print = 16;
    std::cout << "[DBG] vProps[0.." << (max_print - 1) << "]: ";
    for (int i = 0; i < csr->vertexNum && i < max_print; ++i) {
        std::cout << csr->vProps[i] << " ";
    }
    for (int i = 0; i < partition_container.num_partitions && i < max_print; ++i) {
        std::cout << "\n[DBG] Partition " << i << " edge count: " << partition_container.P[i].num_edges;
        std::cout << "\n[DBG] Partition " << i << " first few edges: ";
        for (int j = 0; j < partition_container.P[i].edge_array_host.size() / 2 && j < max_print; ++j) {
            uint src = partition_container.P[i].edge_array_host[2 * j];
            uint dst = partition_container.P[i].edge_array_host[2 * j + 1];
            std::cout << "(" << src << "," << dst << ") ";
        }
    }

    
    auto xclbin = xrt::xclbin(xclbin_name);
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin(xclbin);

    auto dummy_kernel = xrt::kernel(device, uuid, "dummyKernel:{dummyKernel_1}");
    std::vector<xrt::kernel> little_kernels;
    little_kernels.reserve(LITTLE_KERNEL_NUM);
    for (int i = 0; i < LITTLE_KERNEL_NUM; ++i) {
        const std::string cu_id = std::to_string(i + 1);
        const std::string name = "littleKernelScatterGather:{littleKernelScatterGather_" + cu_id + "}";
        little_kernels.emplace_back(device, uuid, name);
    }

    auto hbm_kernel = xrt::kernel(device, uuid, "kernelHBMWrapper:{kernelHBMWrapper_1}");
    auto apply_kernel = xrt::kernel(device, uuid, "kernelApply:{kernelApply_1}");



    
    std::cout << "Kernels loaded successfully." << std::endl;
    std::cout << "[DBG] Kernels are created..." << std::endl;

    auto start_buffer = std::chrono::high_resolution_clock::now();
    const size_t prop_size = partition_container.vertex_property.size();
    std::cout << "[DBG] Property size: " << prop_size << std::endl;
    const size_t prop_bytes = round_up(prop_size * sizeof(uint), 64);
    std::cout << "[DBG] Edge buffers will be allocated per-partition using XRT BOs." << std::endl;
    auto bo_src_prop_1 = xrt::bo(device, prop_bytes, hbm_kernel.group_id(0));
    auto bo_new_prop_1 = xrt::bo(device, prop_bytes, hbm_kernel.group_id(1));
    
    auto bo_apply_src_prop = xrt::bo(device, prop_bytes, apply_kernel.group_id(0));
    
    auto src_prop_1 = bo_src_prop_1.map<uint *>();
    auto new_prop_1 = bo_new_prop_1.map<uint *>();
    
    std::fill(new_prop_1, new_prop_1 + prop_size, 0);
    
    std::copy(partition_container.vertex_property.begin(),
              partition_container.vertex_property.end(),
              src_prop_1);
    auto apply_src_prop = bo_apply_src_prop.map<uint *>();
    std::copy(partition_container.vertex_property.begin(),
              partition_container.vertex_property.end(),
              apply_src_prop);
    for (size_t i = 0; i < 8; ++i) {
        std::cout << "Initial property [" << i << "]: " << src_prop_1[i] << std::endl;
    }
    std::cout << "[DBG] Property buffers allocated and initialized..." << std::endl;
    std::cout << "[DBG] Buffers allocated and initialized." << std::endl;
   std::cout << "Buffers synchronized to device." << std::endl;

   for (uint i = 0; i < partition_container.num_dense_partitions; i++) {
        for (uint subpart_id = 0; subpart_id < partition_container.DP[i].num_subpartitions; subpart_id++) {
            uint part_dst_offset = partition_container.DP[i].subP[subpart_id].dst_offset;
            const uint krnl_id = partition_container.DP[i].subP[subpart_id].kernel_id;
                std::cout << "[DBG] Partition " << i << " Subpart " << subpart_id
                        << " dst_offset=" << part_dst_offset
                        << " kernel_id=" << krnl_id << std::endl;
            for (auto &edge: partition_container.DP[i].subP[subpart_id].edge_array_host) {
                std::cout << "[DBG] edge " << edge << std::endl;
            }
        }
    }
   int num_super_step =5;
    bool use_a_as_src = true;
    
    bo_src_prop_1.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_new_prop_1.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_apply_src_prop.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    // run_littile_gs_merger_kernel.start();
    for (int super_step = 0; super_step < num_super_step; super_step++) {
        std::cout << "Super step " << super_step << std::endl;
        auto &src_prop_bo_1 = use_a_as_src ? bo_src_prop_1 : bo_new_prop_1;
        auto &dst_prop_bo_1 = use_a_as_src ? bo_new_prop_1 : bo_src_prop_1;
        auto *src_prop_ptr = use_a_as_src ? src_prop_1 : new_prop_1;

        std::cout << "[DBG] before kernel sync, sample src properties: ";
        for (size_t i = 0; i < 8 && i < prop_size; ++i) {
            std::cout << src_prop_bo_1.map<uint*>()[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "[DBG] before kernel sync, sample dst properties: ";
        for (size_t i = 0; i < 8 && i < prop_size; ++i) {
            std::cout << dst_prop_bo_1.map<uint*>()[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "[DBG] before kernel sync, sample src properties: ";
        for (size_t i = 0; i < 8 && i < prop_size; ++i) {
            std::cout << src_prop_ptr[i] << " ";
        }
        std::cout << std::endl;
        auto run_hbm = xrt::run(hbm_kernel);
        run_hbm.set_arg(0, src_prop_bo_1);
        run_hbm.set_arg(1, dst_prop_bo_1);
        run_hbm.set_arg(2, static_cast<uint>(partition_container.num_dense_partitions));
        run_hbm.set_arg(3, static_cast<uint>(partition_container.num_sparse_partitions));

        auto run_apply = xrt::run(apply_kernel);
        run_apply.set_arg(0, bo_apply_src_prop);
        run_apply.set_arg(1, static_cast<uint>(partition_container.num_dense_partitions));
        run_apply.set_arg(2, static_cast<uint>(partition_container.num_sparse_partitions));
        run_apply.set_arg(3, 0u);
        auto run_dummy = xrt::run(dummy_kernel);
        run_dummy.set_arg(0, 0);
        auto run_little = xrt::run(little_kernels[0]);
        run_little.set_arg(2, 0);
        // For each subpartition allocate an XRT BO, set args and start little kernels (mimic host_original behavior)
        std::vector<xrt::bo> part_bos;
        std::vector<xrt::run> little_runs;

        std::copy(src_prop_ptr, src_prop_ptr + prop_size, apply_src_prop);
        bo_apply_src_prop.sync(XCL_BO_SYNC_BO_TO_DEVICE);

        // Start global kernels
        run_hbm.start();
        run_apply.start();
        
        run_dummy.start();
        run_little.start();
        // for (uint i = 0; i < partition_container.num_dense_partitions; i++) {
        //     for (uint subpart_id = 0; subpart_id < partition_container.DP[i].num_subpartitions; subpart_id++) {
        //         uint krnl_id = partition_container.DP[i].subP[subpart_id].kernel_id;
        //         auto &edge_vec = partition_container.DP[i].subP[subpart_id].edge_array_host;
        //         // allocate BO on the same group as the target little kernel
        //         part_bos.emplace_back(device, edge_vec.size() * sizeof(uint), little_kernels[krnl_id].group_id(0));
        //         auto &bo = part_bos.back();
        //         auto ptr = bo.map<uint *>();
        //         std::copy(edge_vec.begin(), edge_vec.end(), ptr);
        //         bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);

        //         little_runs.emplace_back(little_kernels[krnl_id]);
        //         auto &run_l = little_runs.back();
        //         // run_l.set_arg(0, bo);
        //         // run_l.set_arg(1, static_cast<uint>(partition_container.DP[i].subP[subpart_id].num_edges));
        //         run_l.set_arg(2, static_cast<uint>(partition_container.DP[i].subP[subpart_id].dst_offset));
        //         run_l.start();
        //         std::cout << "[DBG] littleKernelScatterGather start CU " << krnl_id << std::endl;
        //     }
        // }


        run_little.wait();
        std::cout << "[DBG] little kernel(s) done" << std::endl;

        run_apply.wait();
        run_hbm.wait();
        run_dummy.wait();
        dst_prop_bo_1.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        // Print property values after this super step
        {
            auto *cur_prop = dst_prop_bo_1.map<uint *>();
            std::cout << "Property after superstep " << super_step << ": ";
            for (size_t i = 0; i < 8; ++i) {
                std::cout << cur_prop[i] << " ";
            }
            std::cout << std::endl;
        }
        use_a_as_src = !use_a_as_src;
    }
    // run_big_merger.wait();
    // run_little_merger.wait();
    // 6) Sync results
    bo_new_prop_1.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    std::cout << "Result sample (new_prop_1[0..7]): ";
    for (int i = 0; i < 8 && i < static_cast<int>(prop_size); ++i) {
        std::cout << new_prop_1[i] << " ";
    }
    std::cout << std::endl;
    verifyResultsXrt(partition_container, csr, new_prop_1);
    return 0;
}