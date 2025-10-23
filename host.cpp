#include "data_type.h"
#include <iostream>
#include <string>
#include <xrt/xrt_device.h>
#include <xrt/xrt_kernel.h>
#include <xrt/xrt_bo.h>
int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <xclbin> <graph>" << std::endl;
        return 1;
    }

    std::string xclbin_name = argv[1];
    // std::string graph_path = argv[2];
    auto xclbin = xrt::xclbin(xclbin_name);
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin(xclbin);

    auto write_kernel_1 = xrt::kernel(device, uuid, "write_kernel:{write_kernel_1}");
    auto write_kernel_2 = xrt::kernel(device, uuid, "write_kernel:{write_kernel_2}");
    auto read_kernel_1 = xrt::kernel(device, uuid, "read_kernel:{read_kernel_1}");
    auto read_kernel_2 = xrt::kernel(device, uuid, "read_kernel:{read_kernel_2}");
    auto read_mux = xrt::kernel(device, uuid, "read_mux");
    auto write_mux = xrt::kernel(device, uuid, "write_mux");
    auto memlock = xrt::kernel(device, uuid, "memlock");
    auto allocate_kernel = xrt::kernel(device, uuid, "allocate_kernel");
    auto free_kernel = xrt::kernel(device, uuid, "free_kernel");
    auto app_mem_middle_ware_kernel = xrt::kernel(device, uuid, "app_mem_middle_ware");
    auto pma_read_kernel = xrt::kernel(device, uuid, "pma_read_kernel");
    auto update_message_generator_kernel = xrt::kernel(device, uuid, "update_message_generator");
    auto update_message_router_kernel = xrt::kernel(device, uuid, "update_message_router");
    auto pma_update_kernel = xrt::kernel(device, uuid, "pma_update_kernel");

    auto check_kernel = xrt::kernel(device, uuid, "check_kernel");
    
    std::cout << "Kernels loaded successfully." << std::endl;
    auto run_write_kernel_1 = xrt::run(write_kernel_1);
    auto run_write_kernel_2 = xrt::run(write_kernel_2);
    auto run_read_kernel_1 = xrt::run(read_kernel_1);
    auto run_read_kernel_2 = xrt::run(read_kernel_2);
    auto run_read_mux = xrt::run(read_mux);
    auto run_write_mux = xrt::run(write_mux);
    auto run_memlock = xrt::run(memlock);
    auto run_allocate_kernel = xrt::run(allocate_kernel);
    auto run_free_kernel = xrt::run(free_kernel);
    auto run_app_mem_middle_ware_kernel = xrt::run(app_mem_middle_ware_kernel);
    auto run_pma_read_kernel = xrt::run(pma_read_kernel);
    auto run_update_message_generator_kernel = xrt::run(update_message_generator_kernel);
    auto run_update_message_router_kernel = xrt::run(update_message_router_kernel);
    auto run_pma_update_kernel = xrt::run(pma_update_kernel);
    auto run_check_kernel = xrt::run(check_kernel);

    std::cout << "Kernels are running..." << std::endl;

    auto bo_hbm_base_addr_1 = xrt::bo(device, sizeof(BYTE) * CHANNEL_CAPACITY, write_kernel_1.group_id(0));
    auto bo_hbm_base_addr_2 = xrt::bo(device, sizeof(BYTE) * CHANNEL_CAPACITY, write_kernel_2.group_id(0));
    auto bo_valid_array = xrt::bo(device, sizeof(BYTE) * (CHANNEL_CAPACITY / 8), allocate_kernel.group_id(0));

    BYTE* hbm_base_address_1 = bo_hbm_base_addr_1.map<BYTE*>();
    BYTE* hbm_base_address_2 = bo_hbm_base_addr_2.map<BYTE*>();
    BYTE* valid_array = bo_valid_array.map<BYTE*>();

    // 初始化 HBM 和有效性数组
    std::fill_n(hbm_base_address_1, CHANNEL_CAPACITY, 0);
    std::fill_n(hbm_base_address_2, CHANNEL_CAPACITY, 0);
    std::fill_n(valid_array, CHANNEL_CAPACITY / 8, 0);

    auto bo_pos_vec_info = xrt::bo(device, sizeof(VEC_INFO) * MAX_PMA_NUM, pma_read_kernel.group_id(0));
    auto bo_neg_vec_info = xrt::bo(device, sizeof(VEC_INFO) * MAX_PMA_NUM, pma_read_kernel.group_id(1));
    auto pos_vec_info_ptr = bo_pos_vec_info.map<VEC_INFO*>();
    auto neg_vec_info_ptr = bo_neg_vec_info.map<VEC_INFO*>();
    std::fill_n(pos_vec_info_ptr, MAX_PMA_NUM, 0);
    std::fill_n(neg_vec_info_ptr, MAX_PMA_NUM, 0);
    auto bo_src = xrt::bo(device, sizeof(int) * 1024, check_kernel.group_id(0));
    auto bo_dst = xrt::bo(device, sizeof(int) * 1024, check_kernel.group_id(1));

    auto bo_check_src = xrt::bo(device, sizeof(int) * 1024, check_kernel.group_id(0));
    auto bo_check_dst = xrt::bo(device, sizeof(int) * 1024, check_kernel.group_id(1));
    auto check_src = bo_check_src.map<int*>();
    auto check_dst = bo_check_dst.map<int*>();
    auto bo_vertex_attr = xrt::bo(device, sizeof(double) * 1024, check_kernel.group_id(2));

    for (int i = 0 ; i < 1024 ; i++)
        bo_src.map<int*>()[i] = i;
    for (int i =  0 ; i < 1024 ; i++)
        bo_dst.map<int*>()[i] = 1023-i;

    std::cout << "Buffers allocated and initialized." << std::endl;

    run_write_kernel_1.set_arg(0, bo_hbm_base_addr_1);
    run_write_kernel_2.set_arg(0, bo_hbm_base_addr_2);
    run_read_kernel_1.set_arg(0, bo_hbm_base_addr_1);
    run_read_kernel_2.set_arg(0, bo_hbm_base_addr_2);
    run_allocate_kernel.set_arg(0, bo_valid_array);
    run_free_kernel.set_arg(0, bo_valid_array);
    run_pma_read_kernel.set_arg(0, bo_pos_vec_info);
    run_pma_read_kernel.set_arg(1, bo_neg_vec_info);
    run_update_message_generator_kernel.set_arg(0, 1024);
    run_update_message_generator_kernel.set_arg(1, 1024);
    run_update_message_generator_kernel.set_arg(2,bo_src);
    run_update_message_generator_kernel.set_arg(3,bo_dst);
    run_pma_update_kernel.set_arg(1, bo_pos_vec_info);
    run_pma_update_kernel.set_arg(2, bo_neg_vec_info);
    bo_hbm_base_addr_1.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_hbm_base_addr_2.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_valid_array.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_pos_vec_info.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_neg_vec_info.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::cout << "Buffers synchronized to device." << std::endl;

    run_read_kernel_1.start();
    run_read_kernel_2.start();
    run_write_kernel_1.start();
    run_write_kernel_2.start();
    run_read_mux.start();
    run_write_mux.start();
    run_allocate_kernel.start();
    run_free_kernel.start();
    run_memlock.start();
    run_app_mem_middle_ware_kernel.start();
    run_pma_update_kernel.start();
    run_update_message_router_kernel.start();
    run_update_message_generator_kernel.start();
    run_update_message_generator_kernel.wait();
    run_update_message_router_kernel.wait();
    run_pma_update_kernel.wait();
    run_pma_read_kernel.start();

    for (int i = 0 ; i < 1024 ; i++)
    {
        check_src[0] = i;
        std::fill(check_dst, check_dst + 1024, -1);
        bo_check_src.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        bo_check_dst.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        run_check_kernel.set_arg(0, bo_check_src);
        run_check_kernel.set_arg(1, bo_check_dst);
        run_check_kernel.set_arg(3,1);
        run_check_kernel.start();
        run_check_kernel.wait();
        bo_check_dst.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        for (int j = 0 ; j < 1024 ; j++)
        {
            if (check_dst[j] == -1) break;
            std::cout << check_dst[j] << " ";
        }
        std::cout << std::endl;
    }
    return 0;
}