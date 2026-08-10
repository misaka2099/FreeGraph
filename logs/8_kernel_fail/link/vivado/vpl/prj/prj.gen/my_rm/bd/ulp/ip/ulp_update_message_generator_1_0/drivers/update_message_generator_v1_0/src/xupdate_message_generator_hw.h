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
// 0x10 : Data signal of add_vertex_num
//        bit 31~0 - add_vertex_num[31:0] (Read/Write)
// 0x14 : reserved
// 0x18 : Data signal of add_edge_num
//        bit 31~0 - add_edge_num[31:0] (Read/Write)
// 0x1c : reserved
// 0x20 : Data signal of delete_vertex_num
//        bit 31~0 - delete_vertex_num[31:0] (Read/Write)
// 0x24 : reserved
// 0x28 : Data signal of delete_edge_num
//        bit 31~0 - delete_edge_num[31:0] (Read/Write)
// 0x2c : reserved
// 0x30 : Data signal of src
//        bit 31~0 - src[31:0] (Read/Write)
// 0x34 : Data signal of src
//        bit 31~0 - src[63:32] (Read/Write)
// 0x38 : reserved
// 0x3c : Data signal of dst
//        bit 31~0 - dst[31:0] (Read/Write)
// 0x40 : Data signal of dst
//        bit 31~0 - dst[63:32] (Read/Write)
// 0x44 : reserved
// 0x48 : Data signal of opcode
//        bit 31~0 - opcode[31:0] (Read/Write)
// 0x4c : Data signal of opcode
//        bit 31~0 - opcode[63:32] (Read/Write)
// 0x50 : reserved
// 0x54 : Data signal of last
//        bit 31~0 - last[31:0] (Read/Write)
// 0x58 : reserved
// (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

#define XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_AP_CTRL                0x00
#define XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_GIE                    0x04
#define XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_IER                    0x08
#define XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_ISR                    0x0c
#define XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_ADD_VERTEX_NUM_DATA    0x10
#define XUPDATE_MESSAGE_GENERATOR_CONTROL_BITS_ADD_VERTEX_NUM_DATA    32
#define XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_ADD_EDGE_NUM_DATA      0x18
#define XUPDATE_MESSAGE_GENERATOR_CONTROL_BITS_ADD_EDGE_NUM_DATA      32
#define XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_DELETE_VERTEX_NUM_DATA 0x20
#define XUPDATE_MESSAGE_GENERATOR_CONTROL_BITS_DELETE_VERTEX_NUM_DATA 32
#define XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_DELETE_EDGE_NUM_DATA   0x28
#define XUPDATE_MESSAGE_GENERATOR_CONTROL_BITS_DELETE_EDGE_NUM_DATA   32
#define XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_SRC_DATA               0x30
#define XUPDATE_MESSAGE_GENERATOR_CONTROL_BITS_SRC_DATA               64
#define XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_DST_DATA               0x3c
#define XUPDATE_MESSAGE_GENERATOR_CONTROL_BITS_DST_DATA               64
#define XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_OPCODE_DATA            0x48
#define XUPDATE_MESSAGE_GENERATOR_CONTROL_BITS_OPCODE_DATA            64
#define XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_LAST_DATA              0x54
#define XUPDATE_MESSAGE_GENERATOR_CONTROL_BITS_LAST_DATA              32

