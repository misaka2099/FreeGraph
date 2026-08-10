// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
/***************************** Include Files *********************************/
#include "xpma_read_worker_kernel.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XPma_read_worker_kernel_CfgInitialize(XPma_read_worker_kernel *InstancePtr, XPma_read_worker_kernel_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XPma_read_worker_kernel_Start(XPma_read_worker_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_read_worker_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XPma_read_worker_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XPma_read_worker_kernel_IsDone(XPma_read_worker_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_read_worker_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XPma_read_worker_kernel_IsIdle(XPma_read_worker_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_read_worker_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XPma_read_worker_kernel_IsReady(XPma_read_worker_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_read_worker_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XPma_read_worker_kernel_Continue(XPma_read_worker_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_read_worker_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XPma_read_worker_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_AP_CTRL, Data | 0x10);
}

void XPma_read_worker_kernel_EnableAutoRestart(XPma_read_worker_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_read_worker_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XPma_read_worker_kernel_DisableAutoRestart(XPma_read_worker_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_read_worker_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_AP_CTRL, 0);
}

void XPma_read_worker_kernel_Set_worker_id(XPma_read_worker_kernel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_read_worker_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_WORKER_ID_DATA, Data);
}

u32 XPma_read_worker_kernel_Get_worker_id(XPma_read_worker_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_read_worker_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_WORKER_ID_DATA);
    return Data;
}

void XPma_read_worker_kernel_Set_hbm_ptr(XPma_read_worker_kernel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_read_worker_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_HBM_PTR_DATA, (u32)(Data));
    XPma_read_worker_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_HBM_PTR_DATA + 4, (u32)(Data >> 32));
}

u64 XPma_read_worker_kernel_Get_hbm_ptr(XPma_read_worker_kernel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_read_worker_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_HBM_PTR_DATA);
    Data += (u64)XPma_read_worker_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_HBM_PTR_DATA + 4) << 32;
    return Data;
}

void XPma_read_worker_kernel_Set_pma_raw_data_start(XPma_read_worker_kernel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_read_worker_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_PMA_RAW_DATA_START_DATA, Data);
}

u32 XPma_read_worker_kernel_Get_pma_raw_data_start(XPma_read_worker_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_read_worker_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_PMA_RAW_DATA_START_DATA);
    return Data;
}

void XPma_read_worker_kernel_Set_pma_vecinfo_start(XPma_read_worker_kernel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_read_worker_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_PMA_VECINFO_START_DATA, Data);
}

u32 XPma_read_worker_kernel_Get_pma_vecinfo_start(XPma_read_worker_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_read_worker_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_PMA_VECINFO_START_DATA);
    return Data;
}

void XPma_read_worker_kernel_InterruptGlobalEnable(XPma_read_worker_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_read_worker_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_GIE, 1);
}

void XPma_read_worker_kernel_InterruptGlobalDisable(XPma_read_worker_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_read_worker_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_GIE, 0);
}

void XPma_read_worker_kernel_InterruptEnable(XPma_read_worker_kernel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XPma_read_worker_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_IER);
    XPma_read_worker_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_IER, Register | Mask);
}

void XPma_read_worker_kernel_InterruptDisable(XPma_read_worker_kernel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XPma_read_worker_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_IER);
    XPma_read_worker_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_IER, Register & (~Mask));
}

void XPma_read_worker_kernel_InterruptClear(XPma_read_worker_kernel *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_read_worker_kernel_WriteReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_ISR, Mask);
}

u32 XPma_read_worker_kernel_InterruptGetEnabled(XPma_read_worker_kernel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XPma_read_worker_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_IER);
}

u32 XPma_read_worker_kernel_InterruptGetStatus(XPma_read_worker_kernel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XPma_read_worker_kernel_ReadReg(InstancePtr->Control_BaseAddress, XPMA_READ_WORKER_KERNEL_CONTROL_ADDR_ISR);
}

