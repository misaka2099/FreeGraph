// #include "deprecated/xrt.h"
#include "xrt/xrt_bo.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"
int main(int argc, char**argv)
{
    std::string xclbin_name = argv[1];
    auto xclbin = xrt::xclbin(xclbin_name);
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin(xclbin);

    auto test_burst_read = xrt::kernel(device, uuid, "test_cast_burst_read");
    auto bo_in = xrt::bo(device,sizeof(int)*16, test_burst_read.group_id(0));
    auto bo_out = xrt::bo(device,sizeof(int)*16, test_burst_read.group_id(1));
    auto bo_in_ptr = bo_in.map<int*>();
    auto bo_out_ptr = bo_out.map<int*>();
    for (int i = 0 ; i < 16 ; i++)
    {
        bo_in_ptr[i] = i;
    }
    bo_in.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_out.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    auto run = xrt::run(test_burst_read);
    run.set_arg(0, bo_in);
    run.set_arg(1, bo_out);
    run.start();
    run.wait();
    bo_out.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    for (int i = 0 ; i < 16 ; i++)
    {
        std::cout << bo_out_ptr[i] << " ";
    }
    return 0;
}