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
// 0x10 : Data signal of hbm_bs
//        bit 31~0 - hbm_bs[31:0] (Read/Write)
// 0x14 : Data signal of hbm_bs
//        bit 31~0 - hbm_bs[63:32] (Read/Write)
// 0x18 : reserved
// 0x1c : Data signal of hbm_rw
//        bit 31~0 - hbm_rw[31:0] (Read/Write)
// 0x20 : Data signal of hbm_rw
//        bit 31~0 - hbm_rw[63:32] (Read/Write)
// 0x24 : reserved
// 0x28 : Data signal of pma_raw_data_start
//        bit 31~0 - pma_raw_data_start[31:0] (Read/Write)
// 0x2c : reserved
// 0x30 : Data signal of pma_vecinfo_start
//        bit 31~0 - pma_vecinfo_start[31:0] (Read/Write)
// 0x34 : reserved
// (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

#define XBS_INSERT_KERNEL_CONTROL_ADDR_AP_CTRL                 0x00
#define XBS_INSERT_KERNEL_CONTROL_ADDR_GIE                     0x04
#define XBS_INSERT_KERNEL_CONTROL_ADDR_IER                     0x08
#define XBS_INSERT_KERNEL_CONTROL_ADDR_ISR                     0x0c
#define XBS_INSERT_KERNEL_CONTROL_ADDR_HBM_BS_DATA             0x10
#define XBS_INSERT_KERNEL_CONTROL_BITS_HBM_BS_DATA             64
#define XBS_INSERT_KERNEL_CONTROL_ADDR_HBM_RW_DATA             0x1c
#define XBS_INSERT_KERNEL_CONTROL_BITS_HBM_RW_DATA             64
#define XBS_INSERT_KERNEL_CONTROL_ADDR_PMA_RAW_DATA_START_DATA 0x28
#define XBS_INSERT_KERNEL_CONTROL_BITS_PMA_RAW_DATA_START_DATA 32
#define XBS_INSERT_KERNEL_CONTROL_ADDR_PMA_VECINFO_START_DATA  0x30
#define XBS_INSERT_KERNEL_CONTROL_BITS_PMA_VECINFO_START_DATA  32

