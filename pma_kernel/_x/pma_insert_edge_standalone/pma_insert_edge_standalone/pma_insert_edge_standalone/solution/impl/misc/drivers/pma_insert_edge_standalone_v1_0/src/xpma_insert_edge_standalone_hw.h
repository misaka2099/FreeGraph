// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
// control
// 0x00 : Control signals
//        bit 0  - ap_start (Read/Write/COH)
//        bit 1  - ap_done (Read)
//        bit 2  - ap_idle (Read)
//        bit 3  - ap_ready (Read/COR)
//        bit 4  - ap_continue (Read/Write/SC)
//        bit 7  - auto_restart (Read/Write)
//        bit 9  - interrupt (Read)
//        others - reserved
// 0x04 : Global Interrupt Enable Register
//        bit 0  - Global Interrupt Enable (Read/Write)
//        others - reserved
// 0x08 : IP Interrupt Enable Register (Read/Write)
//        bit 0 - enable ap_done interrupt (Read/Write)
//        bit 1 - enable ap_ready interrupt (Read/Write)
//        others - reserved
// 0x0c : IP Interrupt Status Register (Read/TOW)
//        bit 0 - ap_done (Read/TOW)
//        bit 1 - ap_ready (Read/TOW)
//        others - reserved
// 0x10 : Data signal of hbm_gate
//        bit 31~0 - hbm_gate[31:0] (Read/Write)
// 0x14 : Data signal of hbm_gate
//        bit 31~0 - hbm_gate[63:32] (Read/Write)
// 0x18 : reserved
// 0x1c : Data signal of hbm_bs
//        bit 31~0 - hbm_bs[31:0] (Read/Write)
// 0x20 : Data signal of hbm_bs
//        bit 31~0 - hbm_bs[63:32] (Read/Write)
// 0x24 : reserved
// 0x28 : Data signal of hbm_rw
//        bit 31~0 - hbm_rw[31:0] (Read/Write)
// 0x2c : Data signal of hbm_rw
//        bit 31~0 - hbm_rw[63:32] (Read/Write)
// 0x30 : reserved
// 0x34 : Data signal of hbm_red
//        bit 31~0 - hbm_red[31:0] (Read/Write)
// 0x38 : Data signal of hbm_red
//        bit 31~0 - hbm_red[63:32] (Read/Write)
// 0x3c : reserved
// 0x40 : Data signal of hbm_buffer_for_red
//        bit 31~0 - hbm_buffer_for_red[31:0] (Read/Write)
// 0x44 : Data signal of hbm_buffer_for_red
//        bit 31~0 - hbm_buffer_for_red[63:32] (Read/Write)
// 0x48 : reserved
// 0x4c : Data signal of hbm_merge
//        bit 31~0 - hbm_merge[31:0] (Read/Write)
// 0x50 : Data signal of hbm_merge
//        bit 31~0 - hbm_merge[63:32] (Read/Write)
// 0x54 : reserved
// 0x58 : Data signal of hbm_merge_buffer
//        bit 31~0 - hbm_merge_buffer[31:0] (Read/Write)
// 0x5c : Data signal of hbm_merge_buffer
//        bit 31~0 - hbm_merge_buffer[63:32] (Read/Write)
// 0x60 : reserved
// 0x64 : Data signal of task_buffer
//        bit 31~0 - task_buffer[31:0] (Read/Write)
// 0x68 : Data signal of task_buffer
//        bit 31~0 - task_buffer[63:32] (Read/Write)
// 0x6c : reserved
// 0x70 : Data signal of pma_raw_data_start
//        bit 31~0 - pma_raw_data_start[31:0] (Read/Write)
// 0x74 : reserved
// 0x78 : Data signal of pma_vecinfo_start
//        bit 31~0 - pma_vecinfo_start[31:0] (Read/Write)
// 0x7c : reserved
// 0x80 : Data signal of edge_count
//        bit 31~0 - edge_count[31:0] (Read/Write)
// 0x84 : reserved
// 0x88 : Data signal of worker_id
//        bit 31~0 - worker_id[31:0] (Read/Write)
// 0x8c : reserved
// 0x90 : Data signal of bitmap_offset
//        bit 31~0 - bitmap_offset[31:0] (Read/Write)
// 0x94 : reserved
// (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_ADDR_AP_CTRL                 0x00
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_ADDR_GIE                     0x04
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_ADDR_IER                     0x08
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_ADDR_ISR                     0x0c
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_ADDR_HBM_GATE_DATA           0x10
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_BITS_HBM_GATE_DATA           64
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_ADDR_HBM_BS_DATA             0x1c
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_BITS_HBM_BS_DATA             64
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_ADDR_HBM_RW_DATA             0x28
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_BITS_HBM_RW_DATA             64
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_ADDR_HBM_RED_DATA            0x34
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_BITS_HBM_RED_DATA            64
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_ADDR_HBM_BUFFER_FOR_RED_DATA 0x40
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_BITS_HBM_BUFFER_FOR_RED_DATA 64
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_ADDR_HBM_MERGE_DATA          0x4c
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_BITS_HBM_MERGE_DATA          64
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_ADDR_HBM_MERGE_BUFFER_DATA   0x58
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_BITS_HBM_MERGE_BUFFER_DATA   64
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_ADDR_TASK_BUFFER_DATA        0x64
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_BITS_TASK_BUFFER_DATA        64
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_ADDR_PMA_RAW_DATA_START_DATA 0x70
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_BITS_PMA_RAW_DATA_START_DATA 32
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_ADDR_PMA_VECINFO_START_DATA  0x78
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_BITS_PMA_VECINFO_START_DATA  32
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_ADDR_EDGE_COUNT_DATA         0x80
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_BITS_EDGE_COUNT_DATA         32
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_ADDR_WORKER_ID_DATA          0x88
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_BITS_WORKER_ID_DATA          32
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_ADDR_BITMAP_OFFSET_DATA      0x90
#define XPMA_INSERT_EDGE_STANDALONE_CONTROL_BITS_BITMAP_OFFSET_DATA      32

