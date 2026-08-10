// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
/***************************** Include Files *********************************/
#include "xpma_read_merger_kernel.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XPma_read_merger_kernel_CfgInitialize(XPma_read_merger_kernel *InstancePtr, XPma_read_merger_kernel_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XPma_read_merger_kernel_Start(XPma_read_merger_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_read_merger_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_MERGER_KERNEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XPma_read_merger_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_MERGER_KERNEL_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XPma_read_merger_kernel_IsDone(XPma_read_merger_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_read_merger_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_MERGER_KERNEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XPma_read_merger_kernel_IsIdle(XPma_read_merger_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_read_merger_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_MERGER_KERNEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XPma_read_merger_kernel_IsReady(XPma_read_merger_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_read_merger_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_MERGER_KERNEL_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XPma_read_merger_kernel_Continue(XPma_read_merger_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_read_merger_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_MERGER_KERNEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XPma_read_merger_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_MERGER_KERNEL_CONTROL_ADDR_AP_CTRL, Data | 0x10);
}

void XPma_read_merger_kernel_EnableAutoRestart(XPma_read_merger_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_read_merger_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_MERGER_KERNEL_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XPma_read_merger_kernel_DisableAutoRestart(XPma_read_merger_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_read_merger_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_MERGER_KERNEL_CONTROL_ADDR_AP_CTRL, 0);
}

void XPma_read_merger_kernel_InterruptGlobalEnable(XPma_read_merger_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_read_merger_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_MERGER_KERNEL_CONTROL_ADDR_GIE, 1);
}

void XPma_read_merger_kernel_InterruptGlobalDisable(XPma_read_merger_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_read_merger_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_MERGER_KERNEL_CONTROL_ADDR_GIE, 0);
}

void XPma_read_merger_kernel_InterruptEnable(XPma_read_merger_kernel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XPma_read_merger_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_MERGER_KERNEL_CONTROL_ADDR_IER);
    XPma_read_merger_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_MERGER_KERNEL_CONTROL_ADDR_IER, Register | Mask);
}

void XPma_read_merger_kernel_InterruptDisable(XPma_read_merger_kernel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XPma_read_merger_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_MERGER_KERNEL_CONTROL_ADDR_IER);
    XPma_read_merger_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_MERGER_KERNEL_CONTROL_ADDR_IER, Register & (~Mask));
}

void XPma_read_merger_kernel_InterruptClear(XPma_read_merger_kernel *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_read_merger_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_MERGER_KERNEL_CONTROL_ADDR_ISR, Mask);
}

u32 XPma_read_merger_kernel_InterruptGetEnabled(XPma_read_merger_kernel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XPma_read_merger_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_MERGER_KERNEL_CONTROL_ADDR_IER);
}

u32 XPma_read_merger_kernel_InterruptGetStatus(XPma_read_merger_kernel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XPma_read_merger_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_MERGER_KERNEL_CONTROL_ADDR_ISR);
}

