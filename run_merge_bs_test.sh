#!/bin/bash
# 运行整合系统 (merge+bs) 的 hw_emu 验证。用法:
#   bash run_merge_bs_test.sh <policy> <graph>
#     policy: 0=全走bs 1=全走merge 2=新边数>16走merge(默认)
#     graph : mixed_pma_ops.txt 或 rmat_8_12_ops.mtx 等
# 前置: make merge_bs TARGET=hw_emu 已构建, host_merge_bs 已编译。
set -eo pipefail
POLICY="${1:-2}"
GRAPH="${2:-mixed_pma_ops.txt}"
XCLBIN="hw_emu_merge_bs.xclbin"

export XILINX_VITIS=/home/tools/Vitis/2022.2
export XILINX_VIVADO=/home/tools/Vivado/2022.2
export XILINX_VITIS_HLS=/home/tools/Vitis_HLS/2022.2
export XILINX_XRT=/opt/xilinx/xrt
export PATH=/home/tools/Vitis/2022.2/bin:/home/tools/Vivado/2022.2/bin:$PATH
export LD_LIBRARY_PATH=/opt/xilinx/xrt/lib:$LD_LIBRARY_PATH
export XCL_EMULATION_MODE=hw_emu

echo "=== policy=${POLICY} graph=${GRAPH} xclbin=${XCLBIN} ==="
stdbuf -oL ./host_merge_bs ./${XCLBIN} ${GRAPH} 2>&1 | tee "run_p${POLICY}_$(basename ${GRAPH%.*}).log"
