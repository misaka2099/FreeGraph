VPP ?= v++

PLATFORM ?= /xilinx/platforms/xilinx_u280_gen3x16_xdma_1_202211_1/xilinx_u280_gen3x16_xdma_1_202211_1.xpfm
INCLUDES := -I /home/sail/lyf/pma_demo/stream/include -I $(CURDIR)/acc_template -I $(CURDIR)/acc_template/common \
			-I $(CURDIR)/acc_template/kernel_little_gs -I $(CURDIR)/acc_template/kernel_big_gs \
			-I $(CURDIR)/acc_udfs -I $(CURDIR)/acc_udfs/$(ALGO)
TARGET ?= sw_emu

# load global parameters (LITTLE/BIG buffer sizes etc.)
include global_para.mk

# Algorithm selection (default: bfs). Set via: `make ALGO=pr` or export ALGO.
ALGO ?= bfs

# If the selected algorithm provides a build.mk with compile-time options, include it.
ALG_BUILD_MK := $(CURDIR)/acc_udfs/$(ALGO)/build.mk
ifneq (,$(wildcard $(ALG_BUILD_MK)))
include $(ALG_BUILD_MK)
else
$(warning No build.mk found for algorithm '$(ALGO)' at $(ALG_BUILD_MK))
endif

# Normalize boolean flags from build.mk to 0/1 for passing to v++
$(eval HAVE_EDGE_PROP_VAL := $(if $(filter true,$(HAVE_EDGE_PROP)),1,0))
$(eval HAVE_UNSIGNED_PROP_VAL := $(if $(filter true,$(HAVE_UNSIGNED_PROP)),1,0))
$(eval HAVE_APPLY_VAL := $(if $(filter true,$(HAVE_APPLY)),1,0))
$(eval HAVE_APPLY_OUTDEG_VAL := $(if $(filter true,$(HAVE_APPLY_OUTDEG)),1,0))
$(eval HAVE_VERTEX_PROP_VAL := $(if $(filter true,$(HAVE_VERTEX_PROP)),1,0))

# pass defines to v++ so kernels see LITTLE/BIG macros
DEFINES := -D LITTLE_KERNEL_DST_BUFFER_SIZE=$(LITTLE_KERNEL_DST_BUFFER_SIZE) \
		   -D BIG_KERNEL_DST_BUFFER_SIZE=$(BIG_KERNEL_DST_BUFFER_SIZE) \
		   -D PARTITION_SIZE=$(PARTITION_SIZE) \
		   -D SRC_BUFFER_SIZE=$(SRC_BUFFER_SIZE) \
		   -D LOG2_SRC_BUFFER_SIZE=$(LOG2_SRC_BUFFER_SIZE) \
		   -D VERTEX_REORDER_ENABLE=$(VERTEX_REORDER_ENABLE) \
		   -D BIG_KERNEL_NUM=$(BIG_KERNEL_NUM) \
		   -D LITTLE_KERNEL_NUM=$(LITTLE_KERNEL_NUM) \
		   -D HAVE_EDGE_PROP=$(HAVE_EDGE_PROP_VAL) \
		   -D HAVE_UNSIGNED_PROP=$(HAVE_UNSIGNED_PROP_VAL) \
		   -D HAVE_APPLY=$(HAVE_APPLY_VAL) \
		   -D HAVE_APPLY_OUTDEG=$(HAVE_APPLY_OUTDEG_VAL) \
		   -D HAVE_VERTEX_PROP=$(HAVE_VERTEX_PROP_VAL) \
		   $(WHOLE_HOST_DEFINES)

# List of kernel symbols (extern "C" function names)
# KERNELS := edgeReader kernelApply kernelHBMWrapper kernelBigGSMerger kernelLittleGSMerger \
           bigKernelScatterGather littleKernelScatterGather

KERNELS := edgeReader kernelApply kernelHBMWrapper kernelLittleGSMerger \
        littleKernelScatterGather

# Map kernel symbol to source file path
edgeReader_SRC := acc_template/edge_reader/edge_reader.cpp
kernelApply_SRC := acc_template/kernel_apply/kernel_apply.cpp
kernelHBMWrapper_SRC := acc_template/kernel_hbm_wrapper/kernel_hbm_wrapper.cpp
# kernelBigGSMerger_SRC := acc_template/kernel_big_gs_merger/kernel_big_gs_merger.cpp
kernelLittleGSMerger_SRC := acc_template/kernel_little_gs_merger/kernel_little_gs_merger.cpp
# bigKernelScatterGather_SRC := acc_template/kernel_big_gs/kernel_scatter_gather.cpp
littleKernelScatterGather_SRC := acc_template/kernel_little_gs/kernel_scatter_gather.cpp
edgeReader_SRC := acc_template/edge_reader/edgeReader.cpp

XO_FILES := $(patsubst %,xokernels/%.xo,$(KERNELS))

.PHONY: all clean
all: $(XO_FILES)

xokernels/edgeReader.xo: $(edgeReader_SRC)
	@mkdir -p xokernels
	@echo "---- Building kernel edgeReader -> $@ (target=$(TARGET)) ----"
	$(VPP) -c -g $(DEFINES) -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) -k edgeReader -o $@ $<
	@echo "---- done: $@ ----"

xokernels/kernelApply.xo: $(kernelApply_SRC)
	@mkdir -p xokernels
	@echo "---- Building kernel kernelApply -> $@ (target=$(TARGET)) ----"
	$(VPP) -c -g $(DEFINES) -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) -k kernelApply -o $@ $<
	@echo "---- done: $@ ----"

xokernels/kernelHBMWrapper.xo: $(kernelHBMWrapper_SRC)
	@mkdir -p xokernels
	@echo "---- Building kernel kernelHBMWrapper -> $@ (target=$(TARGET)) ----"
	$(VPP) -c -g $(DEFINES) -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) -k kernelHBMWrapper -o $@ $<
	@echo "---- done: $@ ----"

# xokernels/kernelBigGSMerger.xo: $(kernelBigGSMerger_SRC)
# 	@mkdir -p xokernels
# 	@echo "---- Building kernel kernelBigGSMerger -> $@ (target=$(TARGET)) ----"
# 	$(VPP) -c -g $(DEFINES) -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) -k kernelBigGSMerger -o $@ $<
# 	@echo "---- done: $@ ----"

xokernels/kernelLittleGSMerger.xo: $(kernelLittleGSMerger_SRC)
	@mkdir -p xokernels
	@echo "---- Building kernel kernelLittleGSMerger -> $@ (target=$(TARGET)) ----"
	$(VPP) -c -g $(DEFINES) -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) -k kernelLittleGSMerger -o $@ $<
	@echo "---- done: $@ ----"

# xokernels/bigKernelScatterGather.xo: $(bigKernelScatterGather_SRC)
# 	@mkdir -p xokernels
# 	@echo "---- Building kernel bigKernelScatterGather -> $@ (target=$(TARGET)) ----"
# 	$(VPP) -c -g $(DEFINES) -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) -k bigKernelScatterGather -o $@ $<
# 	@echo "---- done: $@ ----"

xokernels/littleKernelScatterGather.xo: $(littleKernelScatterGather_SRC)
	@mkdir -p xokernels
	@echo "---- Building kernel littleKernelScatterGather -> $@ (target=$(TARGET)) ----"
	$(VPP) -c -g $(DEFINES) -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) -k littleKernelScatterGather -o $@ $<
	@echo "---- done: $@ ----"


xokernels/edgeReader.xo: $(edgeReader_SRC)
	@mkdir -p xokernels
	@echo "---- Building kernel edgeReader -> $@ (target=$(TARGET)) ----"
	$(VPP) -c -g $(DEFINES) -t $(TARGET) $(INCLUDES) --platform $(PLATFORM) -k edgeReader -o $@ $<
	@echo "---- done: $@ ----"


clean:
	@echo "Cleaning .xo and build artifacts in ReGraph..."
	@rm -f xokernels/*.xo
	@rm -rf _x
	@rm -rf v++_*
	@rm -f *_summary
	@echo "done."
