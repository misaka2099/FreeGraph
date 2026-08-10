// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
/***************************** Include Files *********************************/
#include "xtask_reader_kernel.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XTask_reader_kernel_CfgInitialize(XTask_reader_kernel *InstancePtr, XTask_reader_kernel_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XTask_reader_kernel_Start(XTask_reader_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XTask_reader_kernel_ReadReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XTask_reader_kernel_WriteReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XTask_reader_kernel_IsDone(XTask_reader_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XTask_reader_kernel_ReadReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XTask_reader_kernel_IsIdle(XTask_reader_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XTask_reader_kernel_ReadReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XTask_reader_kernel_IsReady(XTask_reader_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XTask_reader_kernel_ReadReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XTask_reader_kernel_Continue(XTask_reader_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XTask_reader_kernel_ReadReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XTask_reader_kernel_WriteReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_AP_CTRL, Data | 0x10);
}

void XTask_reader_kernel_EnableAutoRestart(XTask_reader_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XTask_reader_kernel_WriteReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XTask_reader_kernel_DisableAutoRestart(XTask_reader_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XTask_reader_kernel_WriteReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_AP_CTRL, 0);
}

void XTask_reader_kernel_Set_worker_id(XTask_reader_kernel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XTask_reader_kernel_WriteReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_WORKER_ID_DATA, Data);
}

u32 XTask_reader_kernel_Get_worker_id(XTask_reader_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XTask_reader_kernel_ReadReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_WORKER_ID_DATA);
    return Data;
}

void XTask_reader_kernel_InterruptGlobalEnable(XTask_reader_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XTask_reader_kernel_WriteReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_GIE, 1);
}

void XTask_reader_kernel_InterruptGlobalDisable(XTask_reader_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XTask_reader_kernel_WriteReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_GIE, 0);
}

void XTask_reader_kernel_InterruptEnable(XTask_reader_kernel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XTask_reader_kernel_ReadReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_IER);
    XTask_reader_kernel_WriteReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_IER, Register | Mask);
}

void XTask_reader_kernel_InterruptDisable(XTask_reader_kernel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XTask_reader_kernel_ReadReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_IER);
    XTask_reader_kernel_WriteReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_IER, Register & (~Mask));
}

void XTask_reader_kernel_InterruptClear(XTask_reader_kernel *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XTask_reader_kernel_WriteReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_ISR, Mask);
}

u32 XTask_reader_kernel_InterruptGetEnabled(XTask_reader_kernel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XTask_reader_kernel_ReadReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_IER);
}

u32 XTask_reader_kernel_InterruptGetStatus(XTask_reader_kernel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XTask_reader_kernel_ReadReg(InstancePtr->Control_BaseAddress, XTASK_READER_KERNEL_CONTROL_ADDR_ISR);
}

