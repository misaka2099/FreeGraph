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
// 0x10 : Data signal of bm_in_hbm
//        bit 31~0 - bm_in_hbm[31:0] (Read/Write)
// 0x14 : Data signal of bm_in_hbm
//        bit 31~0 - bm_in_hbm[63:32] (Read/Write)
// 0x18 : reserved
// 0x1c : Data signal of init_from_hbm
//        bit 0  - init_from_hbm[0] (Read/Write)
//        others - reserved
// 0x20 : reserved
// (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

#define XALLOC_ACCESS_KERNEL_CONTROL_ADDR_AP_CTRL            0x00
#define XALLOC_ACCESS_KERNEL_CONTROL_ADDR_GIE                0x04
#define XALLOC_ACCESS_KERNEL_CONTROL_ADDR_IER                0x08
#define XALLOC_ACCESS_KERNEL_CONTROL_ADDR_ISR                0x0c
#define XALLOC_ACCESS_KERNEL_CONTROL_ADDR_BM_IN_HBM_DATA     0x10
#define XALLOC_ACCESS_KERNEL_CONTROL_BITS_BM_IN_HBM_DATA     64
#define XALLOC_ACCESS_KERNEL_CONTROL_ADDR_INIT_FROM_HBM_DATA 0x1c
#define XALLOC_ACCESS_KERNEL_CONTROL_BITS_INIT_FROM_HBM_DATA 1

