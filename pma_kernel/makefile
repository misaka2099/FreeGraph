VPP ?= v++

PLATFORM ?= /opt/xilinx/platforms/xilinx_u280_gen3x16_xdma_1_202211_1/xilinx_u280_gen3x16_xdma_1_202211_1.xpfm
INCLUDES := -I/home/sail/lyf/pma_demo/stream/include
TARGET ?= sw_emu
# KERNELS = update_message_generator update_message_router pma_insert_edge pma_insert_vertex pma_read_router_kernel pma_read_worker_kernel pma_read_merger_kernel check_kernel
KERNELS = update_message_generator update_message_router update_message_router_stream pma_insert_edge pma_read_router_kernel pma_read_worker_kernel pma_read_merger_kernel check_kernel bfs_kernel task_reader_kernel bs_insert_kernel redistribution_kernel merge_pipeline

XO_FILES := $(patsubst %,%.xo,$(KERNELS))

.PHONY: all clean help
all: $(XO_FILES)
INCLUDE_HEADERS := /home/sail/lyf/pma_demo/stream/include/data_type.h /home/sail/lyf/pma_demo/stream/include/debug.h

%.xo: %.cpp $(INCLUDE_HEADERS)
	@echo "---- Building kernel $* -> $@ (target=$(TARGET)) ----"
	$(VPP) -c -g -t $(TARGET) $(INCLUDES) --save-temps --platform $(PLATFORM) -k $* -o $@ $<
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