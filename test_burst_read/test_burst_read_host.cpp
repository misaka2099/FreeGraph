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

    auto test_burst_read = xrt::kernel(device, uuid, "test_hbm_access_unit");
    auto bo_hbm = xrt::bo(device,sizeof(int)*16*5, test_burst_read.group_id(0));
    auto bo_hbm_ptr = bo_hbm.map<int*>();
    for (int i = 0 ; i < 80 ; i++)
    {
        bo_hbm_ptr[i] = i;
    }
    bo_hbm.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    auto run = xrt::run(test_burst_read);
    run.set_arg(0, bo_hbm);
    run.start();
    run.wait();
    bo_hbm.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    for (int i = 0 ; i < 80 ; i++)
    {
        std::cout << bo_hbm_ptr[i] << " ";
    }
    return 0;
}