VPP ?= v++
PLATFORM ?= /xilinx/platforms/xilinx_u280_gen3x16_xdma_1_202211_1/xilinx_u280_gen3x16_xdma_1_202211_1.xpfm
TARGET ?= sw_emu

KERNEL_SRC := test_burst_read.cpp
KERNEL_XO := test_burst_read.xo
KERNEL_XCLBIN := test_burst_read.xclbin

all: $(KERNEL_XCLBIN)

$(KERNEL_XO): $(KERNEL_SRC)
	$(VPP) -g -c -t $(TARGET) --platform $(PLATFORM) -k test_cast_burst_read -o $@ $<

$(KERNEL_XCLBIN): $(KERNEL_XO)
	$(VPP) -g -l -t $(TARGET) --platform $(PLATFORM) -o $@ $^

clean:
	rm -f $(KERNEL_XO) $(KERNEL_XCLBIN)

host: test_burst_read_host.cpp
	g++ -std=c++17 $(INCLUDES) -I${XILINX_XRT}/include -L${XILINX_XRT}/lib -I${XILINX_VITIS_HLS}/include -o $@ $^ -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g
