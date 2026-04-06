VPP ?= v++
include ./ReGraph/global_para.mk
PLATFORM ?= /xilinx/platforms/xilinx_u280_gen3x16_xdma_1_202211_1/xilinx_u280_gen3x16_xdma_1_202211_1.xpfm
INCLUDES := -I/home/sail/lyf/pma_demo/stream/include
TARGET ?= sw_emu
MEMKERNEL_DIR := ./memkernel
PMA_KERNEL_DIR := ./pma_kernel
REGRAPH_KERNEL_DIR := ./ReGraph/xokernels
MEM_XO_FILES := $(addprefix $(MEMKERNEL_DIR)/, alloc_access_kernel.xo mem_access_kernel.xo)
LIVE_GRAPH_PMA_XO_FILES := $(addprefix $(PMA_KERNEL_DIR)/, update_message_generator.xo update_message_router.xo pma_insert_vertex.xo pma_insert_edge.xo pma_read_router_kernel.xo pma_read_worker_kernel.xo pma_read_merger_kernel.xo check_kernel.xo signal_kernel.xo)
WHOLE_PMA_XO_FILES := $(addprefix $(PMA_KERNEL_DIR)/, update_message_generator.xo update_message_router.xo pma_insert_vertex.xo pma_insert_edge.xo pma_read_router_kernel.xo pma_read_worker_kernel.xo pma_read_merger_kernel.xo signal_kernel.xo)
REGRAPH_XO_FILES := $(wildcard $(REGRAPH_KERNEL_DIR)/*.xo)
XCLBIN_FILE := $(TARGET)_test_all.xclbin
WHOLE_SYSTEM_XCLBIN := $(TARGET)_whole_system.xclbin
WHOLE_SYSTEM_XO_FILE := $(MEM_XO_FILES) $(WHOLE_PMA_XO_FILES) $(REGRAPH_XO_FILES)
CONFIG_FILES := system.cfg profile.cfg
all: subdirs $(XCLBIN_FILE)

# Compilation-time algorithm selection (default bfs). Usage: `make whole_host ALGO=pr` or `ALGO=cc`
ALGO ?= bfs
ifeq ($(ALGO),pr)
ALGO_CODE = 1
else ifeq ($(ALGO),cc)
ALGO_CODE = 2
else
ALGO_CODE = 0
endif
WHOLE_HOST_INCLUDES := -I$(XILINX_XRT)/include -I$(XILINX_VITIS_HLS)/include \
	-IReGraph/acc_udfs/$(ALGO) -IReGraph -IReGraph/acc_template -IReGraph/acc_template/common -IReGraph/utils/common/includes -IReGraph/utils/common/includes/xcl2 $(shell find ReGraph/host -type d | sed 's|^|-I|g' | tr '\n' ' ')

# Pass important ReGraph configuration macros to host compiler
WHOLE_HOST_DEFINES := -DLITTLE_KERNEL_NUM=$(LITTLE_KERNEL_NUM) -DBIG_KERNEL_NUM=$(BIG_KERNEL_NUM) \
	-DPARTITION_SIZE=$(PARTITION_SIZE) -DSRC_BUFFER_SIZE=$(SRC_BUFFER_SIZE) \
	-DLOG2_SRC_BUFFER_SIZE=$(LOG2_SRC_BUFFER_SIZE) -DLITTLE_KERNEL_DST_BUFFER_SIZE=$(LITTLE_KERNEL_DST_BUFFER_SIZE) \
	-DBIG_KERNEL_DST_BUFFER_SIZE=$(BIG_KERNEL_DST_BUFFER_SIZE) -DHAVE_APPLY=$(HAVE_APPLY) \
	-DVERTEX_REORDER_ENABLE=$(VERTEX_REORDER_ENABLE) -DALGO_CODE=$(ALGO_CODE)
subdirs:
	$(MAKE) -C memkernel all TARGET=$(TARGET)
	$(MAKE) -C pma_kernel all TARGET=$(TARGET)

# Host source list: all ReGraph/host .cpp except files that define main
HOST_SRCS := $(shell find ReGraph/host -name "*.cpp" | grep -v -E 'host_xrt.cpp|host.cpp|host_original.cpp|test_partition.cpp' | tr '\n' ' ')
# Add xcl2 implementation so xcl:: symbols resolve
HOST_SRCS += ReGraph/utils/common/includes/xcl2/xcl2.cpp

# Add algorithm-specific host helpers (dataPrepare, etc.) from acc_udfs
UDFS_SRCS := $(shell if [ -d ReGraph/acc_udfs/$(ALGO) ]; then find ReGraph/acc_udfs/$(ALGO) -name "*.cpp" | tr '\n' ' '; fi)

ALL_XO_FILES = $(MEM_XO_FILES) $(LIVE_GRAPH_PMA_XO_FILES)
WHOLE_XO_FILES =$(MEM_XO_FILES) $(PMA_XO_FILES) $(REGRAPH_XO_FILES)
$(XCLBIN_FILE): $(ALL_XO_FILES)
	@echo "---- Linking kernels to create $(XCLBIN_FILE) ----"
	$(VPP) -l -g -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) --config system.cfg --config profile.cfg -o $@ $^
	@echo "---- Link done: $@ ----"

$(WHOLE_SYSTEM_XCLBIN): $(WHOLE_XO_FILES)
	@echo "---- Linking whole system kernels to create $(WHOLE_SYSTEM_XCLBIN) ----"
	$(VPP) -l -g -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) --config whole_system.cfg --config profile.cfg -o $@ $(WHOLE_SYSTEM_XO_FILE)
	@echo "---- Link done: $@ ----"
DUMMY_XCLBIN := dummy.xclbin
DUMMT_KERNEL_XO := $(wildcard ReGraph/dummykernels/*.xo)
$(DUMMY_XCLBIN): $(DUMMT_KERNEL_XO)
	@echo "---- Linking dummy kernel to create $(DUMMY_XCLBIN) ----"
	$(VPP) -l -g -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) --config dummy.cfg -o $@ $^
	@echo "---- Link done: $@ ----"
whole_system: $(WHOLE_SYSTEM_XCLBIN)
	$(MAKE) -C memkernel all TARGET=$(TARGET)
	$(MAKE) -C pma_kernel all TARGET=$(TARGET)
	$(MAKE) -C ReGraph -f makekernel.mk all TARGET=$(TARGET) WHOLE_HOST_DEFINES="$(WHOLE_HOST_DEFINES)" ALGO=$(ALGO)
	$(MAKE) $(WHOLE_SYSTEM_XCLBIN)
whole_host: whole_host.cpp
	g++ -std=c++17 $(INCLUDES) $(WHOLE_HOST_INCLUDES) $(WHOLE_HOST_DEFINES) -L${XILINX_XRT}/lib -o $@ whole_host.cpp $(HOST_SRCS) $(UDFS_SRCS) -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g
dummy_system:
	$(MAKE) -C ReGraph -f makedummy.mk all TARGET=$(TARGET) WHOLE_HOST_DEFINES="$(WHOLE_HOST_DEFINES)" ALGO=$(ALGO)
	$(MAKE) $(DUMMY_XCLBIN)
host: host.cpp
	g++ -std=c++17 $(INCLUDES) -I${XILINX_XRT}/include -L${XILINX_XRT}/lib -I${XILINX_VITIS_HLS}/include -o $@ $^ -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g
real_host: real_host.cpp
	g++ -std=c++17 $(INCLUDES) -I${XILINX_XRT}/include -L${XILINX_XRT}/lib -I${XILINX_VITIS_HLS}/include -o $@ $^ -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g
test_host: test_host.cpp
	g++ -std=c++17 $(INCLUDES) -I${XILINX_XRT}/include -L${XILINX_XRT}/lib -I${XILINX_VITIS_HLS}/include -o $@ $^ -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g
dummy_host: dummy_host.cpp
	g++ -std=c++17 $(INCLUDES) $(WHOLE_HOST_INCLUDES) $(WHOLE_HOST_DEFINES) -L${XILINX_XRT}/lib -o $@ dummy_host.cpp $(HOST_SRCS) $(UDFS_SRCS) -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g
test_read_kernel: test_read_kernel.cpp
	g++ -std=c++17 $(INCLUDES) -I${XILINX_XRT}/include -L${XILINX_XRT}/lib -I${XILINX_VITIS_HLS}/include -o $@ $^ -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g
clean:
	$(MAKE) -C memkernel clean
	$(MAKE) -C pma_kernel clean
	$(MAKE) -C ReGraph -f makekernel.mk clean
	rm -f $(XCLBIN_FILE) host
	rm -rf _x/link