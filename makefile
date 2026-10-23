# Full graph framework: incremental updates plus graph/neighbor reads.
.DEFAULT_GOAL := all
.DELETE_ON_ERROR:

VPP ?= /home/tools/Vitis/2022.2/bin/v++
CXX ?= g++
TARGET ?= hw
PLATFORM ?= /opt/xilinx/platforms/xilinx_u280_gen3x16_xdma_1_202211_1/xilinx_u280_gen3x16_xdma_1_202211_1.xpfm
XILINX_XRT ?= /opt/xilinx/xrt
XILINX_VITIS_HLS ?= /home/tools/Vitis_HLS/2022.2
CXXFLAGS ?= -O2 -g

ifeq ($(filter $(TARGET),hw hw_emu sw_emu),)
$(error TARGET must be hw, hw_emu or sw_emu)
endif

BUILD_DIR := build/framework
BUILD_CONFIG := $(BUILD_DIR)/.config
XCLBIN := $(TARGET)_freegraph.xclbin
CONFIG := system_merge_bs.cfg
PMA_KERNELS := task_reader_kernel bs_insert_kernel merge_pipeline redistribution_kernel \
               update_message_generator update_message_router_stream \
               pma_read_router_kernel pma_read_worker_kernel pma_read_merger_kernel check_kernel
XO_FILES := memkernel/alloc_access_kernel.xo $(addprefix pma_kernel/,$(addsuffix .xo,$(PMA_KERNELS)))
HEADERS := $(wildcard include/*.h include/*.hpp pma_kernel/*.h pma_kernel/*.hpp memkernel/*.h memkernel/*.hpp)
HOST_DEFINES := -DUSE_STANDALONE_MODE -DUSE_MERGE_BS_PATH -DHOST_SKIP_AP_CTRL_NONE_ROUTER
EMU_DEFINES := $(if $(filter hw_emu,$(TARGET)),-DHW_EMU,) -DREDISTRIBUTION_BS_PORTS=1

.PHONY: all xclbin clean help FORCE
all: host xclbin
xclbin: $(XCLBIN)

host: host.cpp $(HEADERS) makefile
	$(CXX) -std=c++17 $(CPPFLAGS) $(CXXFLAGS) $(HOST_DEFINES) -Iinclude \
		-I$(XILINX_XRT)/include -I$(XILINX_VITIS_HLS)/include $< -o $@ \
		$(LDFLAGS) -L$(XILINX_XRT)/lib -Wl,-rpath,$(XILINX_XRT)/lib -lxrt_core -lxrt_coreutil -lOpenCL -pthread -lrt -ldl

# Update the stamp only when build settings change. This prevents shared
# subdirectory .xo files from being reused for a different target.
$(BUILD_CONFIG): FORCE
	mkdir -p $(BUILD_DIR)
	@printf '%s\n' 'TARGET=$(TARGET)' 'PLATFORM=$(PLATFORM)' 'EMU_DEFINES=$(EMU_DEFINES)' 'VPP=$(VPP)' > $@.tmp
	@cmp -s $@.tmp $@ || cp $@.tmp $@
	@rm -f $@.tmp

$(XO_FILES): %.xo: %.cpp $(HEADERS) $(BUILD_CONFIG) makefile memkernel/makefile pma_kernel/makefile
	$(MAKE) -C $(@D) $(@F) -W $(*F).cpp VPP="$(VPP)" TARGET=$(TARGET) \
		PLATFORM=$(PLATFORM) EMU_DEFINES="$(EMU_DEFINES)"

$(XCLBIN): $(XO_FILES) $(CONFIG) makefile
	mkdir -p $(BUILD_DIR)/$(TARGET)/link
	cd $(BUILD_DIR)/$(TARGET)/link && $(VPP) -l -g -t $(TARGET) --platform $(PLATFORM) \
		--kernel_frequency=150 --save-temps --config $(abspath $(CONFIG)) \
		-o $(abspath $@) $(abspath $(XO_FILES))

help:
	@echo 'make                         Build full read/write host and hardware xclbin'
	@echo 'make host                    Build the full-framework host'
	@echo 'make xclbin TARGET=hw_emu     Build the hardware-emulation image'
	@echo 'Run: ./host $(XCLBIN) <graph.mtx>'
	@echo 'Graph verification uses the FPGA read pipeline by default.'
	@echo 'For hw_emu, set XCL_EMULATION_MODE=hw_emu before running.'

clean:
	rm -f host $(XO_FILES) $(addsuffix _freegraph.xclbin,hw hw_emu sw_emu)
	rm -rf build/framework
