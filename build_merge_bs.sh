#!/bin/bash
# Build integrated merge+bs system for hw_emu. Run from the worktree root.
export XILINX_VITIS=/home/tools/Vitis/2022.2
export XILINX_VIVADO=/home/tools/Vivado/2022.2
export XILINX_VITIS_HLS=/home/tools/Vitis_HLS/2022.2
export XILINX_XRT=/opt/xilinx/xrt
export PATH=/home/tools/Vitis/2022.2/bin:/home/tools/Vivado/2022.2/bin:$PATH
export LD_LIBRARY_PATH=/opt/xilinx/xrt/lib:$LD_LIBRARY_PATH
cd /home/sail/lyf/pma_demo/stream/.claude/worktrees/merge-chunk-protection
make merge_bs TARGET=hw_emu PLATFORM=/opt/xilinx/platforms/xilinx_u280_gen3x16_xdma_1_202211_1/xilinx_u280_gen3x16_xdma_1_202211_1.xpfm 2>&1 | tee build_merge_bs_hwemu.log
echo "BUILD_EXIT=${PIPESTATUS[0]}"
