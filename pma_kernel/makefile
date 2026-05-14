VPP ?= v++

PLATFORM ?= /xilinx/platforms/xilinx_u280_gen3x16_xdma_1_202211_1/xilinx_u280_gen3x16_xdma_1_202211_1.xpfm
INCLUDES := -I/home/sail/lyf/pma_demo/stream/include
TARGET ?= sw_emu
# 3-kernel insert architecture + unified read kernel
KERNELS = update_message_generator update_message_router bs_kernel insert_kernel rebalance_kernel pma_read_kernel check_kernel

XO_FILES := $(patsubst %,%.xo,$(KERNELS))

.PHONY: all clean help
all: $(XO_FILES)
INCLUDE_HEADERS := /home/sail/lyf/pma_demo/stream/include/data_type.h /home/sail/lyf/pma_demo/stream/include/common_function.h /home/sail/lyf/pma_demo/stream/include/debug.h

%.xo: %.cpp $(INCLUDE_HEADERS)
	@echo "---- Building kernel $* -> $@ (target=$(TARGET)) ----"
	$(VPP) -c -g -t $(TARGET) $(INCLUDES) --save-temps --platform $(PLATFORM) -k $* -o $@ $< 
	@echo "---- done: $@ ----"
clean:
	@echo "Cleaning .xo and build artifacts..."
	@rm -f *.xo
	@rm -rf _x
	@rm v++_*
	@rm *_summary
	@echo "done."