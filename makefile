VPP ?= v++

PLATFORM ?= /xilinx/platforms/xilinx_u280_gen3x16_xdma_1_202211_1/xilinx_u280_gen3x16_xdma_1_202211_1.xpfm
INCLUDES := -I /home/sail/lyf/pma_demo/stream/include
TARGET ?= sw_emu
MEMKERNEL_DIR := ./memkernel
PMA_KERNEL_DIR := ./pma_kernel
MEM_XO_FILES := $(addprefix $(MEMKERNEL_DIR)/, memlock.xo read_mux.xo write_mux.xo  write_kernel.xo read_kernel.xo allocate_kernel.xo free_kernel.xo app_mem_middle_ware.xo)
PMA_XO_FILES := $(addprefix $(PMA_KERNEL_DIR)/, update_message_generator.xo update_message_router.xo pma_update_kernel.xo pma_read_kernel.xo check_kernel.xo)

XCLBIN_FILE := $(TARGET)_test_all.xclbin
all: $(XCLBIN_FILE)
ALL_XO_FILES = $(MEM_XO_FILES) $(PMA_XO_FILES)
$(XCLBIN_FILE): $(ALL_XO_FILES)
	@echo "---- Linking kernels to create $(XCLBIN_FILE) ----"
	$(VPP) -l -g -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) --config system_connect.cfg --config $(MEMKERNEL_DIR)/connectivity.cfg --config $(PMA_KERNEL_DIR)/connectivity.cfg -o $@ $^
	@echo "---- Link done: $@ ----"
