VPP ?= v++

PLATFORM ?= /xilinx/platforms/xilinx_u280_gen3x16_xdma_1_202211_1/xilinx_u280_gen3x16_xdma_1_202211_1.xpfm
INCLUDES := -I/home/sail/lyf/pma_demo/stream/include
TARGET ?= sw_emu
MEMKERNEL_DIR := ./memkernel
PMA_KERNEL_DIR := ./pma_kernel
REGRAPH_KERNEL_DIR := ./ReGraph/xokernels
MEM_XO_FILES := $(addprefix $(MEMKERNEL_DIR)/, alloc_access_kernel.xo mem_access_kernel.xo)
PMA_XO_FILES := $(addprefix $(PMA_KERNEL_DIR)/, update_message_generator.xo update_message_router.xo pma_insert_vertex.xo pma_insert_edge.xo pma_read_kernel.xo check_kernel.xo signal_kernel.xo)
REGRAPH_XO_FILES := $(wildcard $(REGRAPH_KERNEL_DIR)/*.xo)
XCLBIN_FILE := $(TARGET)_test_all.xclbin
WHOLE_SYSTEM_XCLBIN := $(TARGET)_whole_system.xclbin
WHOLE_SYSTEM_XO_FILE := $(MEM_XO_FILES) $(PMA_XO_FILES) $(REGRAPH_XO_FILES)
CONFIG_FILES := system.cfg profile.cfg
all: subdirs $(XCLBIN_FILE)

subdirs:
	$(MAKE) -C memkernel all TARGET=$(TARGET)
	$(MAKE) -C pma_kernel all TARGET=$(TARGET)

ALL_XO_FILES = $(MEM_XO_FILES) $(PMA_XO_FILES)
$(XCLBIN_FILE): $(ALL_XO_FILES)
	@echo "---- Linking kernels to create $(XCLBIN_FILE) ----"
	$(VPP) -l -g -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) --config system.cfg --config profile.cfg -o $@ $^
	@echo "---- Link done: $@ ----"

$(WHOLE_SYSTEM_XCLBIN):
	@echo "---- Linking whole system kernels to create $(WHOLE_SYSTEM_XCLBIN) ----"
	$(VPP) -l -g -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) --config whole_system.cfg --config profile.cfg -o $@ $(WHOLE_SYSTEM_XO_FILE)
	@echo "---- Link done: $@ ----"

whole_system: 
	$(MAKE) -C memkernel all TARGET=$(TARGET)
	$(MAKE) -C pma_kernel all TARGET=$(TARGET)
	$(MAKE) -C ReGraph -f makekernel.mk all TARGET=$(TARGET)
	$(MAKE) $(WHOLE_SYSTEM_XCLBIN)


host: host.cpp
	g++ -std=c++17 $(INCLUDES) -I${XILINX_XRT}/include -L${XILINX_XRT}/lib -I${XILINX_VITIS_HLS}/include -o $@ $^ -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g
real_host: real_host.cpp
	g++ -std=c++17 $(INCLUDES) -I${XILINX_XRT}/include -L${XILINX_XRT}/lib -I${XILINX_VITIS_HLS}/include -o $@ $^ -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g
test_host: test_host.cpp
	g++ -std=c++17 $(INCLUDES) -I${XILINX_XRT}/include -L${XILINX_XRT}/lib -I${XILINX_VITIS_HLS}/include -o $@ $^ -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g

clean:
	$(MAKE) -C memkernel clean
	$(MAKE) -C pma_kernel clean
	rm -f $(XCLBIN_FILE) host
	rm -rf _x/link