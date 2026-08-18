VPP ?= v++
PLATFORM ?= /opt/xilinx/platforms/xilinx_u280_gen3x16_xdma_1_202211_1/xilinx_u280_gen3x16_xdma_1_202211_1.xpfm
INCLUDES := -I$(abspath include)
TARGET ?= sw_emu
# Gate 路由策略 (data_type.h decide_merge_path): 0=全走 bs, 1=全走 merge, 2=新边数>16 走 merge
GATE_POLICY ?= 2
MEMKERNEL_DIR := ./memkernel
PMA_KERNEL_DIR := ./pma_kernel
REGRAPH_KERNEL_DIR := ./ReGraph/xokernels
MEM_XO_FILES := $(addprefix $(MEMKERNEL_DIR)/, alloc_access_kernel.xo)
# LIVE_GRAPH_PMA_XO_FILES := $(addprefix $(PMA_KERNEL_DIR)/, update_message_generator.xo update_message_router.xo pma_insert_vertex.xo pma_insert_edge.xo pma_read_router_kernel.xo pma_read_worker_kernel.xo pma_read_merger_kernel.xo check_kernel.xo)
# WHOLE_PMA_XO_FILES := $(addprefix $(PMA_KERNEL_DIR)/, update_message_generator.xo update_message_router.xo pma_insert_vertex.xo pma_insert_edge.xo pma_read_router_kernel.xo pma_read_worker_kernel.xo pma_read_merger_kernel.xo)
LIVE_GRAPH_PMA_XO_FILES := $(addprefix $(PMA_KERNEL_DIR)/, update_message_generator.xo update_message_router.xo  pma_insert_edge.xo pma_read_router_kernel.xo pma_read_worker_kernel.xo pma_read_merger_kernel.xo check_kernel.xo)
WHOLE_PMA_XO_FILES := $(addprefix $(PMA_KERNEL_DIR)/, update_message_generator.xo update_message_router.xo  pma_insert_edge.xo pma_read_router_kernel.xo pma_read_worker_kernel.xo pma_read_merger_kernel.xo)

REGRAPH_XO_FILES := $(wildcard $(REGRAPH_KERNEL_DIR)/*.xo)
XCLBIN_FILE := $(TARGET)_test_all.xclbin
WHOLE_SYSTEM_XCLBIN := $(TARGET)_whole_system.xclbin
WHOLE_SYSTEM_XO_FILE := $(MEM_XO_FILES) $(WHOLE_PMA_XO_FILES) $(REGRAPH_XO_FILES)
CONFIG_FILES := system.cfg
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
	$(VPP) -l -g -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) --kernel_frequency=150 --save-temps --config system.cfg -o $@ $^
	@echo "---- Link done: $@ ----"

BFS_PMA_XO_FILES := $(addprefix $(PMA_KERNEL_DIR)/, update_message_generator.xo update_message_router.xo pma_insert_edge.xo pma_read_router_kernel.xo pma_read_worker_kernel.xo pma_read_merger_kernel.xo bfs_kernel.xo)
BFS_ALL_XO_FILES = $(MEM_XO_FILES) $(BFS_PMA_XO_FILES)
BFS_XCLBIN := bfs_$(TARGET).xclbin

# ---- Split mode: pma_insert_edge_standalone 拆成 3 内核 (task_reader / bs_insert / redistribution), online-only ----
SPLIT_PMA_XO_FILES := $(addprefix $(PMA_KERNEL_DIR)/, task_reader_kernel.xo bs_insert_kernel.xo redistribution_kernel.xo update_message_generator.xo update_message_router_stream.xo pma_read_router_kernel.xo pma_read_worker_kernel.xo pma_read_merger_kernel.xo check_kernel.xo)
SPLIT_ALL_XO := $(MEM_XO_FILES) $(SPLIT_PMA_XO_FILES)
SPLIT_XCLBIN := $(TARGET)_split.xclbin
split: subdirs_split $(SPLIT_XCLBIN)
subdirs_split:
	$(MAKE) -C memkernel all TARGET=$(TARGET)
	$(MAKE) -C pma_kernel task_reader_kernel.xo bs_insert_kernel.xo redistribution_kernel.xo TARGET=$(TARGET)
	$(MAKE) -C pma_kernel update_message_generator.xo update_message_router_stream.xo pma_read_router_kernel.xo pma_read_worker_kernel.xo pma_read_merger_kernel.xo check_kernel.xo TARGET=$(TARGET)
$(SPLIT_XCLBIN): $(SPLIT_ALL_XO)
	@echo "---- Linking split system to create $(SPLIT_XCLBIN) ----"
	$(VPP) -l -g -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) --kernel_frequency=150 --save-temps --config system_split.cfg -o $@ $^
	@echo "---- Split link done: $@ ----"

# ---- Merge mode: 插入流程用 merge 归并替换 bs (task_reader / merge_pipeline / redistribution) ----
MERGE_PMA_XO_FILES := $(addprefix $(PMA_KERNEL_DIR)/, task_reader_kernel.xo merge_pipeline.xo redistribution_kernel.xo update_message_generator.xo update_message_router_stream.xo pma_read_router_kernel.xo pma_read_worker_kernel.xo pma_read_merger_kernel.xo check_kernel.xo)
MERGE_ALL_XO := $(MEM_XO_FILES) $(MERGE_PMA_XO_FILES)
MERGE_XCLBIN := $(TARGET)_merge.xclbin
merge: subdirs_merge $(MERGE_XCLBIN)
subdirs_merge:
	$(MAKE) -C memkernel all TARGET=$(TARGET)
	$(MAKE) -C pma_kernel task_reader_kernel.xo merge_pipeline.xo redistribution_kernel.xo TARGET=$(TARGET)
	$(MAKE) -C pma_kernel update_message_generator.xo update_message_router_stream.xo pma_read_router_kernel.xo pma_read_worker_kernel.xo pma_read_merger_kernel.xo check_kernel.xo TARGET=$(TARGET)
$(MERGE_XCLBIN): $(MERGE_ALL_XO)
	@echo "---- Linking merge system to create $(MERGE_XCLBIN) ----"
	$(VPP) -l -g -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) --kernel_frequency=150 --save-temps --config system_merge.cfg -o $@ $^
	@echo "---- Merge link done: $@ ----"

# ---- Merge+BS 整合系统: task_reader gate 路由 (decide_merge_path), bs+merge 双路径 ----
# 三个测试策略用 GATE_POLICY 编译开关 (0=全 bs, 1=全 merge, 2=新边数>16 走 merge, 默认)。
# 旧 system_merge.cfg / system_split.cfg 已被整合系统取代: task_reader 现在双 edge 流,
# 单流 cfg 不再可用。
MERGE_BS_PMA_XO_FILES := $(addprefix $(PMA_KERNEL_DIR)/, task_reader_kernel.xo bs_insert_kernel.xo merge_pipeline.xo redistribution_kernel.xo update_message_generator.xo update_message_router_stream.xo pma_read_router_kernel.xo pma_read_worker_kernel.xo pma_read_merger_kernel.xo check_kernel.xo)
MERGE_BS_ALL_XO := $(MEM_XO_FILES) $(MERGE_BS_PMA_XO_FILES)
MERGE_BS_XCLBIN := $(TARGET)_merge_bs.xclbin
merge_bs: subdirs_merge_bs $(MERGE_BS_XCLBIN)
subdirs_merge_bs:
	$(MAKE) -C memkernel alloc_access_kernel.xo TARGET=$(TARGET)
	$(MAKE) -C pma_kernel task_reader_kernel.xo bs_insert_kernel.xo merge_pipeline.xo redistribution_kernel.xo TARGET=$(TARGET) GATE_POLICY=$(GATE_POLICY)
	$(MAKE) -C pma_kernel update_message_generator.xo update_message_router_stream.xo pma_read_router_kernel.xo pma_read_worker_kernel.xo pma_read_merger_kernel.xo check_kernel.xo TARGET=$(TARGET)
$(MERGE_BS_XCLBIN): $(MERGE_BS_ALL_XO)
	@echo "---- Linking merge+bs system to create $(MERGE_BS_XCLBIN) ----"
	$(VPP) -l -g -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) --kernel_frequency=150 --save-temps --config system_merge_bs.cfg -o $@ $^
	@echo "---- Merge+bs link done: $@ ----"
host_merge_bs: host.cpp
	g++ -std=c++17 -DUSE_STANDALONE_MODE -DUSE_MERGE_BS_PATH $(INCLUDES) -I${XILINX_XRT}/include -L${XILINX_XRT}/lib -I${XILINX_VITIS_HLS}/include -o $@ $^ -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g
bfs: subdirs_bfs $(BFS_XCLBIN)
subdirs_bfs:
	$(MAKE) -C memkernel all TARGET=$(TARGET)
	$(MAKE) -C pma_kernel all TARGET=$(TARGET)
$(BFS_XCLBIN): $(BFS_ALL_XO_FILES)
	@echo "---- Linking BFS system to create $(BFS_XCLBIN) ----"
	$(VPP) -l -g -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) --kernel_frequency=150 --save-temps --config bfs_system.cfg -o $@ $^
	@echo "---- BFS link done: $@ ----"

$(WHOLE_SYSTEM_XCLBIN): $(WHOLE_XO_FILES)
	@echo "---- Linking whole system kernels to create $(WHOLE_SYSTEM_XCLBIN) ----"
	$(VPP) -l -g -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) --config whole_system.cfg -o $@ $(WHOLE_SYSTEM_XO_FILE)
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
host_standalone: host.cpp
	g++ -std=c++17 -DUSE_STANDALONE_MODE $(INCLUDES) -I${XILINX_XRT}/include -L${XILINX_XRT}/lib -I${XILINX_VITIS_HLS}/include -o $@ $^ -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g
host_merge: host.cpp
	g++ -std=c++17 -DUSE_STANDALONE_MODE -DUSE_MERGE_PATH $(INCLUDES) -I${XILINX_XRT}/include -L${XILINX_XRT}/lib -I${XILINX_VITIS_HLS}/include -o $@ $^ -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g
real_host: real_host.cpp
	g++ -std=c++17 $(INCLUDES) -I${XILINX_XRT}/include -L${XILINX_XRT}/lib -I${XILINX_VITIS_HLS}/include -o $@ $^ -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g
test_host: test_host.cpp
	g++ -std=c++17 $(INCLUDES) -I${XILINX_XRT}/include -L${XILINX_XRT}/lib -I${XILINX_VITIS_HLS}/include -o $@ $^ -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g
dummy_host: dummy_host.cpp
	g++ -std=c++17 $(INCLUDES) $(WHOLE_HOST_INCLUDES) $(WHOLE_HOST_DEFINES) -L${XILINX_XRT}/lib -o $@ dummy_host.cpp $(HOST_SRCS) $(UDFS_SRCS) -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g
test_read_kernel: test_read_kernel.cpp
	g++ -std=c++17 $(INCLUDES) -I${XILINX_XRT}/include -L${XILINX_XRT}/lib -I${XILINX_VITIS_HLS}/include -o $@ $^ -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g

# ---- Overlap Test Host ----
overlap_test_host: test_overlap_host.cpp
	g++ -std=c++17 $(INCLUDES) -I${XILINX_XRT}/include -L${XILINX_XRT}/lib -I${XILINX_VITIS_HLS}/include -o $@ $^ -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g

# ---- Read Latency Test Host ----
test_read_latency: test_read_latency.cpp
	g++ -std=c++17 $(INCLUDES) -I${XILINX_XRT}/include -L${XILINX_XRT}/lib -I${XILINX_VITIS_HLS}/include -o $@ $^ -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g

# ---- HBM Build Time Host ----
test_hbm_build_time: test_hbm_build_time.cpp
	g++ -std=c++17 $(INCLUDES) -I${XILINX_XRT}/include -L${XILINX_XRT}/lib -I${XILINX_VITIS_HLS}/include -o $@ $^ -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g

# ---- BFS Host ----
bfs_host: bfs_host.cpp
	g++ -std=c++17 $(INCLUDES) -I${XILINX_XRT}/include -L${XILINX_XRT}/lib -I${XILINX_VITIS_HLS}/include -o $@ $^ -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g

# ---- Read pipeline + ReGraph compute (regraph_read) ----
# 只含读管线 + ReGraph 计算链: 无 router/merger/insert/check。
# edgeReader 直接驱动 CHANNEL_NUM 个 pma_read_worker_kernel。
READ_REGRAPH_XO_FILES := $(PMA_KERNEL_DIR)/pma_read_worker_kernel.xo \
	$(REGRAPH_KERNEL_DIR)/edgeReader.xo $(REGRAPH_KERNEL_DIR)/littleKernelScatterGather.xo \
	$(REGRAPH_KERNEL_DIR)/kernelLittleGSMerger.xo $(REGRAPH_KERNEL_DIR)/kernelApply.xo \
	$(REGRAPH_KERNEL_DIR)/kernelHBMWrapper.xo
READ_REGRAPH_XCLBIN := $(TARGET)_regraph_read.xclbin
regraph_read: subdirs_regraph_read $(READ_REGRAPH_XCLBIN)
subdirs_regraph_read:
	$(MAKE) -C pma_kernel pma_read_worker_kernel.xo TARGET=$(TARGET)
	$(MAKE) -C ReGraph -f makekernel.mk xokernels/edgeReader.xo xokernels/littleKernelScatterGather.xo xokernels/kernelLittleGSMerger.xo xokernels/kernelApply.xo xokernels/kernelHBMWrapper.xo TARGET=$(TARGET) WHOLE_HOST_DEFINES="$(WHOLE_HOST_DEFINES)" ALGO=$(ALGO)
$(READ_REGRAPH_XCLBIN): $(READ_REGRAPH_XO_FILES)
	@echo "---- Linking read-pipeline + ReGraph system to create $@ ----"
	$(VPP) -l -g -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) --kernel_frequency=150 --save-temps --config regraph_read_system.cfg -o $@ $^
	@echo "---- Link done: $@ ----"
regraph_read_host: regraph_read_host.cpp
	g++ -std=c++17 $(INCLUDES) -I${XILINX_XRT}/include -L${XILINX_XRT}/lib -I${XILINX_VITIS_HLS}/include -DLITTLE_KERNEL_DST_BUFFER_SIZE=$(LITTLE_KERNEL_DST_BUFFER_SIZE) -o $@ $^ -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl -g

clean:
	$(MAKE) -C memkernel clean
	$(MAKE) -C pma_kernel clean
	$(MAKE) -C ReGraph -f makekernel.mk clean
	rm -f $(XCLBIN_FILE) host $(OVERLAP_XCLBIN) overlap_test_host
	rm -rf _x/link