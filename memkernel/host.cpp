#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <hls_stream.h>
#include "data_type.h"
#include "xrt/xrt_bo.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"
#include <fstream>
#include <unordered_set>
#define HERE std::cout << __FILE__ << ":" << __LINE__ << std::endl;
int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <xclbin>" << std::endl;
        return 1;
    }

    std::string xclbin_name = argv[1];

    // 加载设备和 xclbin
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin(xclbin_name);
    auto write_kernel_1 = xrt::kernel(device, uuid, "write_kernel:{write_kernel_1}");
    auto write_kernel_2 = xrt::kernel(device, uuid, "write_kernel:{write_kernel_2}");
    auto read_kernel_1 = xrt::kernel(device, uuid, "read_kernel:{read_kernel_1}");
    auto read_kernel_2 = xrt::kernel(device, uuid, "read_kernel:{read_kernel_2}");
    auto read_mux = xrt::kernel(device, uuid, "read_mux");
    auto write_mux = xrt::kernel(device, uuid, "write_mux");
    auto memlock = xrt::kernel(device, uuid, "memlock");
    auto allocate_kernel = xrt::kernel(device, uuid, "allocate_kernel");
    auto free_kernel = xrt::kernel(device, uuid, "free_kernel");
    auto test_kernel = xrt::kernel(device, uuid, "test_kernel");


    auto run_write_kernel_1 = xrt::run(write_kernel_1);
    auto run_write_kernel_2 = xrt::run(write_kernel_2);
    auto run_read_kernel_1 = xrt::run(read_kernel_1);
    auto run_read_kernel_2 = xrt::run(read_kernel_2);
    auto run_read_mux = xrt::run(read_mux);
    auto run_write_mux = xrt::run(write_mux);
    auto run_memlock = xrt::run(memlock);
    auto run_allocate_kernel = xrt::run(allocate_kernel);
    auto run_free_kernel = xrt::run(free_kernel);   
    auto run_test_kernel = xrt::run(test_kernel);
    
    HERE
    // 分配 HBM 和有效性数组
    auto bo_hbm_base_address_1 = xrt::bo(device, sizeof(BYTE) * CHANNEL_CAPACITY, write_kernel_1.group_id(0));
    auto bo_hbm_base_address_2 = xrt::bo(device, sizeof(BYTE) * CHANNEL_CAPACITY, write_kernel_2.group_id(0));
    auto bo_valid_array = xrt::bo(device, sizeof(BYTE) * (CHANNEL_CAPACITY / 8), allocate_kernel.group_id(0));

    BYTE* hbm_base_address_1 = bo_hbm_base_address_1.map<BYTE*>();
    BYTE* hbm_base_address_2 = bo_hbm_base_address_2.map<BYTE*>();
    BYTE* valid_array = bo_valid_array.map<BYTE*>();

    // 初始化 HBM 和有效性数组
    std::fill_n(hbm_base_address_1, CHANNEL_CAPACITY, 0);
    std::fill_n(hbm_base_address_2, CHANNEL_CAPACITY, 0);
    std::fill_n(valid_array, CHANNEL_CAPACITY / 8, 0);
    HERE
    bo_hbm_base_address_1.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_hbm_base_address_2.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_valid_array.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    HERE
    // 设置内核参数
    run_write_kernel_1.set_arg(0, bo_hbm_base_address_1);
    run_write_kernel_2.set_arg(0, bo_hbm_base_address_2);
    run_read_kernel_1.set_arg(0, bo_hbm_base_address_1);
    run_read_kernel_2.set_arg(0, bo_hbm_base_address_2);
    run_allocate_kernel.set_arg(0, bo_valid_array);
    run_free_kernel.set_arg(0, bo_valid_array);

    HERE
    int int_len = 1024;
    int double_len = 512;
    auto bo_int_in = xrt::bo(device, sizeof(int) * int_len, test_kernel.group_id(0));
    auto bo_double_in = xrt::bo(device, sizeof(double) * double_len, test_kernel.group_id(1));
    auto bo_int_out = xrt::bo(device, sizeof(int) * int_len, test_kernel.group_id(2));
    auto bo_double_out = xrt::bo(device, sizeof(double) * double_len, test_kernel.group_id(3));

    int* int_in_ptr = bo_int_in.map<int*>();
    double* double_in_ptr = bo_double_in.map<double*>();
    int* int_out_ptr = bo_int_out.map<int*>();
    double* double_out_ptr = bo_double_out.map<double*>();
    HERE
    for (int i = 0 ; i < int_len ; i++)
    {
        int_in_ptr[i] = i;
    }
    for (int i = 0 ; i < double_len ; i++)
    {
        double_in_ptr[i] = i + 1.5;
    }
    std::fill_n(int_out_ptr, int_len, 0);
    std::fill_n(double_out_ptr, double_len, 0); 
    HERE
    bo_int_in.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_double_in.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_int_out.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_double_out.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    HERE
    run_test_kernel.set_arg(0, bo_int_in);
    run_test_kernel.set_arg(1, bo_double_in);
    run_test_kernel.set_arg(2, bo_int_out);
    run_test_kernel.set_arg(3, bo_double_out);
    run_test_kernel.set_arg(4, int_len);
    run_test_kernel.set_arg(5, double_len);
    HERE

    run_write_kernel_1.start();
    run_write_kernel_2.start();
    run_read_kernel_1.start();
    run_read_kernel_2.start();
    run_read_mux.start();
    run_write_mux.start();
    run_memlock.start();
    run_allocate_kernel.start();
    run_free_kernel.start();
    std::cout << "Starting test_kernel..." << std::endl;
    std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();
    run_test_kernel.start();
    run_test_kernel.wait();
    std::chrono::high_resolution_clock::time_point end_time = std::chrono::high_resolution_clock::now();
    std::cout << "test_kernel finished." << std::endl;
    bo_int_out.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    bo_double_out.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    //check result
    bool all_pass = true;
    bo_double_out.read(double_out_ptr);
    for (int i = 0 ; i < double_len ; i++)
    {
        if (double_out_ptr[i] != double_in_ptr[i])
        {
            std::cout << "double_out[" << i << "] = " << double_out_ptr[i] << ", expected " << double_in_ptr[i] << std::endl;
            all_pass = false;
            break;
        }
    }
    bo_int_out.read(int_out_ptr);
    for (int i = 0 ; i < int_len ; i++)
    {
        if (int_out_ptr[i] != int_in_ptr[i])
        {
            std::cout << "int_out[" << i << "] = " << int_out_ptr[i] << ", expected " << int_in_ptr[i] << std::endl;
            all_pass = false;
            break;
        }
    }
    if (all_pass)
    {
        std::cout << "All results are correct!" << std::endl;
    }
    else
    {
        std::cout << "Some results are incorrect!" << std::endl;
    }
    run_write_kernel_1.wait();
    run_write_kernel_2.wait();
    run_read_kernel_1.wait();
    run_read_kernel_2.wait();
    run_read_mux.wait();
    run_write_mux.wait();
    run_memlock.wait();
    run_allocate_kernel.wait();
    run_free_kernel.wait();
    return 0;
}