// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
/***************************** Include Files *********************************/
#include "xpma_insert_vertex.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XPma_insert_vertex_CfgInitialize(XPma_insert_vertex *InstancePtr, XPma_insert_vertex_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XPma_insert_vertex_Start(XPma_insert_vertex *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_insert_vertex_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_VERTEX_CONTROL_ADDR_AP_CTRL) & 0x80;
    XPma_insert_vertex_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_VERTEX_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XPma_insert_vertex_IsDone(XPma_insert_vertex *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_insert_vertex_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_VERTEX_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XPma_insert_vertex_IsIdle(XPma_insert_vertex *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_insert_vertex_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_VERTEX_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XPma_insert_vertex_IsReady(XPma_insert_vertex *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_insert_vertex_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_VERTEX_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XPma_insert_vertex_Continue(XPma_insert_vertex *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_insert_vertex_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_VERTEX_CONTROL_ADDR_AP_CTRL) & 0x80;
    XPma_insert_vertex_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_VERTEX_CONTROL_ADDR_AP_CTRL, Data | 0x10);
}

void XPma_insert_vertex_EnableAutoRestart(XPma_insert_vertex *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_insert_vertex_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_VERTEX_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XPma_insert_vertex_DisableAutoRestart(XPma_insert_vertex *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_insert_vertex_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_VERTEX_CONTROL_ADDR_AP_CTRL, 0);
}

void XPma_insert_vertex_InterruptGlobalEnable(XPma_insert_vertex *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_insert_vertex_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_VERTEX_CONTROL_ADDR_GIE, 1);
}

void XPma_insert_vertex_InterruptGlobalDisable(XPma_insert_vertex *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_insert_vertex_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_VERTEX_CONTROL_ADDR_GIE, 0);
}

void XPma_insert_vertex_InterruptEnable(XPma_insert_vertex *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XPma_insert_vertex_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_VERTEX_CONTROL_ADDR_IER);
    XPma_insert_vertex_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_VERTEX_CONTROL_ADDR_IER, Register | Mask);
}

void XPma_insert_vertex_InterruptDisable(XPma_insert_vertex *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XPma_insert_vertex_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_VERTEX_CONTROL_ADDR_IER);
    XPma_insert_vertex_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_VERTEX_CONTROL_ADDR_IER, Register & (~Mask));
}

void XPma_insert_vertex_InterruptClear(XPma_insert_vertex *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_insert_vertex_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_VERTEX_CONTROL_ADDR_ISR, Mask);
}

u32 XPma_insert_vertex_InterruptGetEnabled(XPma_insert_vertex *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XPma_insert_vertex_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_VERTEX_CONTROL_ADDR_IER);
}

u32 XPma_insert_vertex_InterruptGetStatus(XPma_insert_vertex *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XPma_insert_vertex_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_VERTEX_CONTROL_ADDR_ISR);
}

