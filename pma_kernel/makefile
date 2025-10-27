VPP ?= v++

PLATFORM ?= /xilinx/platforms/xilinx_u280_gen3x16_xdma_1_202211_1/xilinx_u280_gen3x16_xdma_1_202211_1.xpfm
INCLUDES := -I/home/sail/lyf/pma_demo/stream/include
TARGET ?= sw_emu
KERNELS = update_message_generator update_message_router pma_update_kernel pma_read_kernel check_kernel stop_kernel

XO_FILES := $(patsubst %,%.xo,$(KERNELS))

.PHONY: all clean help
all: $(XO_FILES)
%.xo: %.cpp
	@echo "---- Building kernel $* -> $@ (target=$(TARGET)) ----"
	$(VPP) -c -g -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) -k $* -o $@ $<
	@echo "---- done: $@ ----"
pma_update_kernel.xo: pma_update_kernel.cpp pma.cpp 
	@echo "---- Building pma_update_kernel -> $@ ----"
	$(VPP) -c -g -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) -k pma_update_kernel -o $@ pma_update_kernel.cpp pma.cpp
	@echo "---- done: $@ ----"
clean:
	@echo "Cleaning .xo and build artifacts..."
	@rm -f *.xo
	@rm -rf _x
	@rm v++_*
	@rm *_summary
	@rm *.info
	@echo "done."