// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
/***************************** Include Files *********************************/
#include "xalloc_access_kernel.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XAlloc_access_kernel_CfgInitialize(XAlloc_access_kernel *InstancePtr, XAlloc_access_kernel_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XAlloc_access_kernel_Start(XAlloc_access_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XAlloc_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XALLOC_ACCESS_KERNEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XAlloc_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XALLOC_ACCESS_KERNEL_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XAlloc_access_kernel_IsDone(XAlloc_access_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XAlloc_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XALLOC_ACCESS_KERNEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XAlloc_access_kernel_IsIdle(XAlloc_access_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XAlloc_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XALLOC_ACCESS_KERNEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XAlloc_access_kernel_IsReady(XAlloc_access_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XAlloc_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XALLOC_ACCESS_KERNEL_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XAlloc_access_kernel_Continue(XAlloc_access_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XAlloc_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XALLOC_ACCESS_KERNEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XAlloc_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XALLOC_ACCESS_KERNEL_CONTROL_ADDR_AP_CTRL, Data | 0x10);
}

void XAlloc_access_kernel_EnableAutoRestart(XAlloc_access_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XAlloc_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XALLOC_ACCESS_KERNEL_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XAlloc_access_kernel_DisableAutoRestart(XAlloc_access_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XAlloc_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XALLOC_ACCESS_KERNEL_CONTROL_ADDR_AP_CTRL, 0);
}

void XAlloc_access_kernel_InterruptGlobalEnable(XAlloc_access_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XAlloc_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XALLOC_ACCESS_KERNEL_CONTROL_ADDR_GIE, 1);
}

void XAlloc_access_kernel_InterruptGlobalDisable(XAlloc_access_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XAlloc_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XALLOC_ACCESS_KERNEL_CONTROL_ADDR_GIE, 0);
}

void XAlloc_access_kernel_InterruptEnable(XAlloc_access_kernel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XAlloc_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XALLOC_ACCESS_KERNEL_CONTROL_ADDR_IER);
    XAlloc_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XALLOC_ACCESS_KERNEL_CONTROL_ADDR_IER, Register | Mask);
}

void XAlloc_access_kernel_InterruptDisable(XAlloc_access_kernel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XAlloc_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XALLOC_ACCESS_KERNEL_CONTROL_ADDR_IER);
    XAlloc_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XALLOC_ACCESS_KERNEL_CONTROL_ADDR_IER, Register & (~Mask));
}

void XAlloc_access_kernel_InterruptClear(XAlloc_access_kernel *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XAlloc_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XALLOC_ACCESS_KERNEL_CONTROL_ADDR_ISR, Mask);
}

u32 XAlloc_access_kernel_InterruptGetEnabled(XAlloc_access_kernel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XAlloc_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XALLOC_ACCESS_KERNEL_CONTROL_ADDR_IER);
}

u32 XAlloc_access_kernel_InterruptGetStatus(XAlloc_access_kernel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XAlloc_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XALLOC_ACCESS_KERNEL_CONTROL_ADDR_ISR);
}

