// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
/***************************** Include Files *********************************/
#include "xupdate_message_generator.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XUpdate_message_generator_CfgInitialize(XUpdate_message_generator *InstancePtr, XUpdate_message_generator_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XUpdate_message_generator_Start(XUpdate_message_generator *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_AP_CTRL) & 0x80;
    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XUpdate_message_generator_IsDone(XUpdate_message_generator *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XUpdate_message_generator_IsIdle(XUpdate_message_generator *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XUpdate_message_generator_IsReady(XUpdate_message_generator *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XUpdate_message_generator_Continue(XUpdate_message_generator *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_AP_CTRL) & 0x80;
    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_AP_CTRL, Data | 0x10);
}

void XUpdate_message_generator_EnableAutoRestart(XUpdate_message_generator *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XUpdate_message_generator_DisableAutoRestart(XUpdate_message_generator *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_AP_CTRL, 0);
}

void XUpdate_message_generator_Set_add_vertex_num(XUpdate_message_generator *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_ADD_VERTEX_NUM_DATA, Data);
}

u32 XUpdate_message_generator_Get_add_vertex_num(XUpdate_message_generator *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_ADD_VERTEX_NUM_DATA);
    return Data;
}

void XUpdate_message_generator_Set_add_edge_num(XUpdate_message_generator *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_ADD_EDGE_NUM_DATA, Data);
}

u32 XUpdate_message_generator_Get_add_edge_num(XUpdate_message_generator *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_ADD_EDGE_NUM_DATA);
    return Data;
}

void XUpdate_message_generator_Set_delete_vertex_num(XUpdate_message_generator *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_DELETE_VERTEX_NUM_DATA, Data);
}

u32 XUpdate_message_generator_Get_delete_vertex_num(XUpdate_message_generator *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_DELETE_VERTEX_NUM_DATA);
    return Data;
}

void XUpdate_message_generator_Set_delete_edge_num(XUpdate_message_generator *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_DELETE_EDGE_NUM_DATA, Data);
}

u32 XUpdate_message_generator_Get_delete_edge_num(XUpdate_message_generator *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_DELETE_EDGE_NUM_DATA);
    return Data;
}

void XUpdate_message_generator_Set_src(XUpdate_message_generator *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_SRC_DATA, (u32)(Data));
    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_SRC_DATA + 4, (u32)(Data >> 32));
}

u64 XUpdate_message_generator_Get_src(XUpdate_message_generator *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_SRC_DATA);
    Data += (u64)XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_SRC_DATA + 4) << 32;
    return Data;
}

void XUpdate_message_generator_Set_dst(XUpdate_message_generator *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_DST_DATA, (u32)(Data));
    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_DST_DATA + 4, (u32)(Data >> 32));
}

u64 XUpdate_message_generator_Get_dst(XUpdate_message_generator *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_DST_DATA);
    Data += (u64)XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_DST_DATA + 4) << 32;
    return Data;
}

void XUpdate_message_generator_Set_opcode(XUpdate_message_generator *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_OPCODE_DATA, (u32)(Data));
    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_OPCODE_DATA + 4, (u32)(Data >> 32));
}

u64 XUpdate_message_generator_Get_opcode(XUpdate_message_generator *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_OPCODE_DATA);
    Data += (u64)XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_OPCODE_DATA + 4) << 32;
    return Data;
}

void XUpdate_message_generator_Set_last(XUpdate_message_generator *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_LAST_DATA, Data);
}

u32 XUpdate_message_generator_Get_last(XUpdate_message_generator *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_LAST_DATA);
    return Data;
}

void XUpdate_message_generator_InterruptGlobalEnable(XUpdate_message_generator *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_GIE, 1);
}

void XUpdate_message_generator_InterruptGlobalDisable(XUpdate_message_generator *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_GIE, 0);
}

void XUpdate_message_generator_InterruptEnable(XUpdate_message_generator *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_IER);
    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_IER, Register | Mask);
}

void XUpdate_message_generator_InterruptDisable(XUpdate_message_generator *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_IER);
    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_IER, Register & (~Mask));
}

void XUpdate_message_generator_InterruptClear(XUpdate_message_generator *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XUpdate_message_generator_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_ISR, Mask);
}

u32 XUpdate_message_generator_InterruptGetEnabled(XUpdate_message_generator *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_IER);
}

u32 XUpdate_message_generator_InterruptGetStatus(XUpdate_message_generator *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XUpdate_message_generator_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_GENERATOR_CONTROL_ADDR_ISR);
}

