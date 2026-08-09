#!/bin/bash
export XCL_EMULATION_MODE=hw_emu
export XILINX_XRT=/opt/xilinx/xrt
export LD_LIBRARY_PATH=/opt/xilinx/xrt/lib:$LD_LIBRARY_PATH
cd /home/sail/lyf/pma_demo/stream/.claude/worktrees/merge-chunk-protection
./host_merge_bs ./hw_emu_merge_bs.xclbin mixed_pma_ops.txt > run_p2_direct.log 2>&1
echo "HOST_EXIT=$?"
