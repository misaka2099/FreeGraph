#!/bin/bash
# 指定 GATE_POLICY 重建 task_reader + 链接 + 跑 hw_emu。
# 用法: bash run_policy.sh <policy> [graph]
#   policy: 0=全走bs 1=全走merge 2=新边数>16走merge(默认)
# 前置: host_merge_bs 已编译, 其他 .xo 已构建。
set -eo pipefail
POLICY="${1:-2}"
GRAPH="${2:-mixed_pma_ops.txt}"

export XILINX_VITIS=/home/tools/Vitis/2022.2
export XILINX_VIVADO=/home/tools/Vivado/2022.2
export XILINX_VITIS_HLS=/home/tools/Vitis_HLS/2022.2
export XILINX_XRT=/opt/xilinx/xrt
export PATH=/home/tools/Vitis/2022.2/bin:/home/tools/Vivado/2022.2/bin:$PATH
export LD_LIBRARY_PATH=/opt/xilinx/xrt/lib:$LD_LIBRARY_PATH

cd /home/sail/lyf/pma_demo/stream/.claude/worktrees/merge-chunk-protection
# 强制重建 task_reader (GATE_POLICY 是编译期开关, make 不知道策略变了)
rm -f pma_kernel/task_reader_kernel.xo pma_kernel/task_reader_kernel.xo.compile_summary
echo "=== rebuild with GATE_POLICY=${POLICY} ==="
make merge_bs TARGET=hw_emu PLATFORM=/opt/xilinx/platforms/xilinx_u280_gen3x16_xdma_1_202211_1/xilinx_u280_gen3x16_xdma_1_202211_1.xpfm GATE_POLICY=${POLICY} 2>&1 | tee build_p${POLICY}_hwemu.log
echo "=== run policy=${POLICY} graph=${GRAPH} ==="
export XCL_EMULATION_MODE=hw_emu
stdbuf -oL ./host_merge_bs ./hw_emu_merge_bs.xclbin ${GRAPH} 2>&1 | tee run_p${POLICY}_$(basename ${GRAPH%.*}).log
echo "=== done policy=${POLICY} ==="
