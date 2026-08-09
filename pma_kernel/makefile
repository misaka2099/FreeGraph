VPP ?= v++

PLATFORM ?= /opt/xilinx/platforms/xilinx_u280_gen3x16_xdma_1_202211_1/xilinx_u280_gen3x16_xdma_1_202211_1.xpfm
INCLUDES := -I$(abspath ../include)
TARGET ?= sw_emu
# Gate 路由策略 (data_type.h decide_merge_path): 0=全走 bs, 1=全走 merge, 2=新边数>16 走 merge
GATE_POLICY ?= 2
# KERNELS = update_message_generator update_message_router pma_insert_edge pma_insert_vertex pma_read_router_kernel pma_read_worker_kernel pma_read_merger_kernel check_kernel
KERNELS = update_message_generator update_message_router update_message_router_stream pma_insert_edge pma_read_router_kernel pma_read_worker_kernel pma_read_merger_kernel check_kernel bfs_kernel task_reader_kernel bs_insert_kernel redistribution_kernel merge_pipeline

XO_FILES := $(patsubst %,%.xo,$(KERNELS))

.PHONY: all clean help
all: $(XO_FILES)
INCLUDE_HEADERS := $(abspath ../include/data_type.h) $(abspath ../include/debug.h)

%.xo: %.cpp $(INCLUDE_HEADERS)
	@echo "---- Building kernel $* -> $@ (target=$(TARGET)) ----"
	$(VPP) -c -g -t $(TARGET) $(INCLUDES) --save-temps --platform $(PLATFORM) -k $* -o $@ $<
	@echo "---- done: $@ ----"

# task_reader: gate 路由策略编译开关 (data_type.h GATE_POLICY)
#   GATE_POLICY=0 全走 bs, =1 全走 merge, =2 该 PMA 新边数>16 走 merge (默认)
task_reader_kernel.xo: task_reader_kernel.cpp $(INCLUDE_HEADERS)
	@echo "---- Building task_reader_kernel -> $@ (target=$(TARGET), GATE_POLICY=$(GATE_POLICY)) ----"
	$(VPP) -c -g -t $(TARGET) $(INCLUDES) --save-temps --platform $(PLATFORM) -k task_reader_kernel -DGATE_POLICY=$(GATE_POLICY) -o $@ $<
	@echo "---- done: $@ ----"

# check_kernel with stall profiling
check_kernel.xo: check_kernel.cpp $(INCLUDE_HEADERS)
	@echo "---- Building kernel check_kernel -> $@ (target=$(TARGET), stall profile) ----"
	$(VPP) -c -g -t $(TARGET) $(INCLUDES) --save-temps --platform $(PLATFORM) -k check_kernel --profile.stall check_kernel:all -o $@ $<
	@echo "---- done: $@ ----"

clean:
	@echo "Cleaning .xo and build artifacts..."
	@rm -f *.xo
	@rm -rf _x
	@rm v++_*
	@rm *_summary
	@echo "done."