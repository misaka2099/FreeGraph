// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
/***************************** Include Files *********************************/
#include "xredistribution_kernel.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XRedistribution_kernel_CfgInitialize(XRedistribution_kernel *InstancePtr, XRedistribution_kernel_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XRedistribution_kernel_Start(XRedistribution_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XRedistribution_kernel_ReadReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XRedistribution_kernel_WriteReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XRedistribution_kernel_IsDone(XRedistribution_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XRedistribution_kernel_ReadReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XRedistribution_kernel_IsIdle(XRedistribution_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XRedistribution_kernel_ReadReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XRedistribution_kernel_IsReady(XRedistribution_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XRedistribution_kernel_ReadReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XRedistribution_kernel_Continue(XRedistribution_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XRedistribution_kernel_ReadReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XRedistribution_kernel_WriteReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_AP_CTRL, Data | 0x10);
}

void XRedistribution_kernel_EnableAutoRestart(XRedistribution_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XRedistribution_kernel_WriteReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XRedistribution_kernel_DisableAutoRestart(XRedistribution_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XRedistribution_kernel_WriteReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_AP_CTRL, 0);
}

void XRedistribution_kernel_Set_hbm_red(XRedistribution_kernel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XRedistribution_kernel_WriteReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_HBM_RED_DATA, (u32)(Data));
    XRedistribution_kernel_WriteReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_HBM_RED_DATA + 4, (u32)(Data >> 32));
}

u64 XRedistribution_kernel_Get_hbm_red(XRedistribution_kernel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XRedistribution_kernel_ReadReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_HBM_RED_DATA);
    Data += (u64)XRedistribution_kernel_ReadReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_HBM_RED_DATA + 4) << 32;
    return Data;
}

void XRedistribution_kernel_Set_hbm_buffer_for_red(XRedistribution_kernel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XRedistribution_kernel_WriteReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_HBM_BUFFER_FOR_RED_DATA, (u32)(Data));
    XRedistribution_kernel_WriteReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_HBM_BUFFER_FOR_RED_DATA + 4, (u32)(Data >> 32));
}

u64 XRedistribution_kernel_Get_hbm_buffer_for_red(XRedistribution_kernel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XRedistribution_kernel_ReadReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_HBM_BUFFER_FOR_RED_DATA);
    Data += (u64)XRedistribution_kernel_ReadReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_HBM_BUFFER_FOR_RED_DATA + 4) << 32;
    return Data;
}

void XRedistribution_kernel_Set_pma_raw_data_start(XRedistribution_kernel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XRedistribution_kernel_WriteReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_PMA_RAW_DATA_START_DATA, Data);
}

u32 XRedistribution_kernel_Get_pma_raw_data_start(XRedistribution_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XRedistribution_kernel_ReadReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_PMA_RAW_DATA_START_DATA);
    return Data;
}

void XRedistribution_kernel_Set_pma_vecinfo_start(XRedistribution_kernel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XRedistribution_kernel_WriteReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_PMA_VECINFO_START_DATA, Data);
}

u32 XRedistribution_kernel_Get_pma_vecinfo_start(XRedistribution_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XRedistribution_kernel_ReadReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_PMA_VECINFO_START_DATA);
    return Data;
}

void XRedistribution_kernel_Set_bitmap_offset(XRedistribution_kernel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XRedistribution_kernel_WriteReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_BITMAP_OFFSET_DATA, Data);
}

u32 XRedistribution_kernel_Get_bitmap_offset(XRedistribution_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XRedistribution_kernel_ReadReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_BITMAP_OFFSET_DATA);
    return Data;
}

void XRedistribution_kernel_InterruptGlobalEnable(XRedistribution_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XRedistribution_kernel_WriteReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_GIE, 1);
}

void XRedistribution_kernel_InterruptGlobalDisable(XRedistribution_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XRedistribution_kernel_WriteReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_GIE, 0);
}

void XRedistribution_kernel_InterruptEnable(XRedistribution_kernel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XRedistribution_kernel_ReadReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_IER);
    XRedistribution_kernel_WriteReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_IER, Register | Mask);
}

void XRedistribution_kernel_InterruptDisable(XRedistribution_kernel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XRedistribution_kernel_ReadReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_IER);
    XRedistribution_kernel_WriteReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_IER, Register & (~Mask));
}

void XRedistribution_kernel_InterruptClear(XRedistribution_kernel *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XRedistribution_kernel_WriteReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_ISR, Mask);
}

u32 XRedistribution_kernel_InterruptGetEnabled(XRedistribution_kernel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XRedistribution_kernel_ReadReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_IER);
}

u32 XRedistribution_kernel_InterruptGetStatus(XRedistribution_kernel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XRedistribution_kernel_ReadReg(InstancePtr->Control_BaseAddress, XREDISTRIBUTION_KERNEL_CONTROL_ADDR_ISR);
}

