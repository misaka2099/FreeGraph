VPP ?= v++

PLATFORM ?= /opt/xilinx/platforms/xilinx_u280_gen3x16_xdma_1_202211_1/xilinx_u280_gen3x16_xdma_1_202211_1.xpfm
INCLUDES := -I$(abspath ../include)
TARGET ?= sw_emu
# Add a compile-time switch for emulation-sized on-chip structures.
EMU_DEFINES := $(if $(filter hw_emu,$(TARGET)),-DHW_EMU,)
KERNELS := task_reader_kernel bs_insert_kernel merge_pipeline redistribution_kernel update_message_generator update_message_router_stream pma_read_router_kernel pma_read_worker_kernel pma_read_merger_kernel check_kernel
XO_FILES := $(patsubst %,%.xo,$(KERNELS))
.PHONY: all clean help
all: $(XO_FILES)
INCLUDE_HEADERS := $(abspath ../include/data_type.h) $(abspath ../include/debug.h) $(abspath ../include/gate_model_lut.h)

%.xo: %.cpp $(INCLUDE_HEADERS)
	@echo "---- Building kernel $* -> $@ (target=$(TARGET), defines=$(EMU_DEFINES)) ----"
	$(VPP) -c -g -t $(TARGET) $(INCLUDES) $(EMU_DEFINES) --save-temps --platform $(PLATFORM) -k $* -o $@ $<
	@echo "---- done: $@ ----"

check_kernel.xo: check_kernel.cpp $(INCLUDE_HEADERS)
	@echo "---- Building kernel check_kernel -> $@ (target=$(TARGET), defines=$(EMU_DEFINES)) ----"
	$(VPP) -c -g -t $(TARGET) $(INCLUDES) $(EMU_DEFINES) --save-temps --platform $(PLATFORM) -k check_kernel --profile.stall check_kernel:all -o $@ $<
	@echo "---- done: $@ ----"

clean:
	@echo "Cleaning .xo and build artifacts..."
	@rm -f *.xo
	@rm -rf _x
	@rm v++_*
	@rm *_summary
	@echo "done."
