# FreeGraph

FreeGraph supports incremental edge insertions and graph reads on an Alveo U280 FPGA. The host applies update batches, reads the resulting graph through the FPGA read pipeline, and compares it with a CPU reference.

## Requirements

- Linux with GNU Make and a C++17 compiler
- Xilinx Vitis and Vitis HLS 2022.2
- XRT and the `xilinx_u280_gen3x16_xdma_1_202211_1` platform
- An Alveo U280 with a compatible shell for hardware execution

The supplied connectivity configuration targets U280 HBM. Using another board requires adapting the connectivity configuration and memory mapping.

## Environment

Replace the paths below with your local installation paths, then run all build and execution commands from the repository root.

```bash
export XILINX_VITIS=/path/to/Vitis/2022.2
export XILINX_VITIS_HLS=/path/to/Vitis_HLS/2022.2
export XILINX_XRT=/path/to/xrt
export PLATFORM=/path/to/xilinx_u280_gen3x16_xdma_1_202211_1.xpfm

source "$XILINX_VITIS/settings64.sh"
source "$XILINX_XRT/setup.sh"
export VPP="$XILINX_VITIS/bin/v++"
```

## Input

Set `DATASET` to an operation file:

```bash
export DATASET=/path/to/operations.txt
```

Each line contains four whitespace-separated integers, without a header:

```text
src dst timestamp opcode
```

Vertex IDs start at zero. Timestamp `-1` identifies initial graph edges; nonnegative timestamps identify insertion batches. Place initial graph edges first, then updates in timestamp order, with each batch contiguous. Opcode `1` inserts one edge.
New vertices are introduced implicitly by edge endpoints.
Example file:

```text
0 1 -1 1
1 2 -1 1
0 2 0 1
2 3 1 1
```

## Hardware Emulation

```bash
make TARGET=hw_emu 
emconfigutil --platform "$PLATFORM"
XCL_EMULATION_MODE=hw_emu ./host ./hw_emu_freegraph.xclbin "$DATASET"
```
## Hardware Execution

```bash
make TARGET=hw 
unset XCL_EMULATION_MODE
./host ./hw_freegraph.xclbin "$DATASET"
```

The build produces `host` and `<TARGET>_freegraph.xclbin`. To build only the host, run `make host`. To build only the FPGA image, run `make xclbin TARGET=hw`, or select `TARGET=hw_emu` for emulation.

## Verification

The default invocation exercises both FPGA updates and FPGA reads. The host reports `update` and `read` timings for each batch and compares the graph against its CPU reference.

For debugging, append `--host-verify` to read back HBM and verify the graph on the CPU instead of using the FPGA read pipeline:

```bash
./host ./hw_freegraph.xclbin "$DATASET" --host-verify
```
