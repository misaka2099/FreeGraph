// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
/***************************** Include Files *********************************/
#include "xupdate_message_router_stream.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XUpdate_message_router_stream_CfgInitialize(XUpdate_message_router_stream *InstancePtr, XUpdate_message_router_stream_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XUpdate_message_router_stream_Start(XUpdate_message_router_stream *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XUpdate_message_router_stream_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_ROUTER_STREAM_CONTROL_ADDR_AP_CTRL) & 0x80;
    XUpdate_message_router_stream_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_ROUTER_STREAM_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XUpdate_message_router_stream_IsDone(XUpdate_message_router_stream *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XUpdate_message_router_stream_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_ROUTER_STREAM_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XUpdate_message_router_stream_IsIdle(XUpdate_message_router_stream *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XUpdate_message_router_stream_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_ROUTER_STREAM_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XUpdate_message_router_stream_IsReady(XUpdate_message_router_stream *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XUpdate_message_router_stream_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_ROUTER_STREAM_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XUpdate_message_router_stream_Continue(XUpdate_message_router_stream *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XUpdate_message_router_stream_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_ROUTER_STREAM_CONTROL_ADDR_AP_CTRL) & 0x80;
    XUpdate_message_router_stream_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_ROUTER_STREAM_CONTROL_ADDR_AP_CTRL, Data | 0x10);
}

void XUpdate_message_router_stream_EnableAutoRestart(XUpdate_message_router_stream *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XUpdate_message_router_stream_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_ROUTER_STREAM_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XUpdate_message_router_stream_DisableAutoRestart(XUpdate_message_router_stream *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XUpdate_message_router_stream_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_ROUTER_STREAM_CONTROL_ADDR_AP_CTRL, 0);
}

void XUpdate_message_router_stream_InterruptGlobalEnable(XUpdate_message_router_stream *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XUpdate_message_router_stream_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_ROUTER_STREAM_CONTROL_ADDR_GIE, 1);
}

void XUpdate_message_router_stream_InterruptGlobalDisable(XUpdate_message_router_stream *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XUpdate_message_router_stream_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_ROUTER_STREAM_CONTROL_ADDR_GIE, 0);
}

void XUpdate_message_router_stream_InterruptEnable(XUpdate_message_router_stream *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XUpdate_message_router_stream_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_ROUTER_STREAM_CONTROL_ADDR_IER);
    XUpdate_message_router_stream_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_ROUTER_STREAM_CONTROL_ADDR_IER, Register | Mask);
}

void XUpdate_message_router_stream_InterruptDisable(XUpdate_message_router_stream *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XUpdate_message_router_stream_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_ROUTER_STREAM_CONTROL_ADDR_IER);
    XUpdate_message_router_stream_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_ROUTER_STREAM_CONTROL_ADDR_IER, Register & (~Mask));
}

void XUpdate_message_router_stream_InterruptClear(XUpdate_message_router_stream *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XUpdate_message_router_stream_WriteReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_ROUTER_STREAM_CONTROL_ADDR_ISR, Mask);
}

u32 XUpdate_message_router_stream_InterruptGetEnabled(XUpdate_message_router_stream *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XUpdate_message_router_stream_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_ROUTER_STREAM_CONTROL_ADDR_IER);
}

u32 XUpdate_message_router_stream_InterruptGetStatus(XUpdate_message_router_stream *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XUpdate_message_router_stream_ReadReg(InstancePtr->Control_BaseAddress, XUPDATE_MESSAGE_ROUTER_STREAM_CONTROL_ADDR_ISR);
}

