// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
/***************************** Include Files *********************************/
#include "xbs_insert_kernel.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XBs_insert_kernel_CfgInitialize(XBs_insert_kernel *InstancePtr, XBs_insert_kernel_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XBs_insert_kernel_Start(XBs_insert_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBs_insert_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XBs_insert_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XBs_insert_kernel_IsDone(XBs_insert_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBs_insert_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XBs_insert_kernel_IsIdle(XBs_insert_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBs_insert_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XBs_insert_kernel_IsReady(XBs_insert_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBs_insert_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XBs_insert_kernel_Continue(XBs_insert_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBs_insert_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XBs_insert_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_AP_CTRL, Data | 0x10);
}

void XBs_insert_kernel_EnableAutoRestart(XBs_insert_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBs_insert_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XBs_insert_kernel_DisableAutoRestart(XBs_insert_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBs_insert_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_AP_CTRL, 0);
}

void XBs_insert_kernel_Set_hbm_bs(XBs_insert_kernel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBs_insert_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_HBM_BS_DATA, (u32)(Data));
    XBs_insert_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_HBM_BS_DATA + 4, (u32)(Data >> 32));
}

u64 XBs_insert_kernel_Get_hbm_bs(XBs_insert_kernel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBs_insert_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_HBM_BS_DATA);
    Data += (u64)XBs_insert_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_HBM_BS_DATA + 4) << 32;
    return Data;
}

void XBs_insert_kernel_Set_hbm_rw(XBs_insert_kernel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBs_insert_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_HBM_RW_DATA, (u32)(Data));
    XBs_insert_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_HBM_RW_DATA + 4, (u32)(Data >> 32));
}

u64 XBs_insert_kernel_Get_hbm_rw(XBs_insert_kernel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBs_insert_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_HBM_RW_DATA);
    Data += (u64)XBs_insert_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_HBM_RW_DATA + 4) << 32;
    return Data;
}

void XBs_insert_kernel_Set_pma_raw_data_start(XBs_insert_kernel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBs_insert_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_PMA_RAW_DATA_START_DATA, Data);
}

u32 XBs_insert_kernel_Get_pma_raw_data_start(XBs_insert_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBs_insert_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_PMA_RAW_DATA_START_DATA);
    return Data;
}

void XBs_insert_kernel_Set_pma_vecinfo_start(XBs_insert_kernel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBs_insert_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_PMA_VECINFO_START_DATA, Data);
}

u32 XBs_insert_kernel_Get_pma_vecinfo_start(XBs_insert_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBs_insert_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_PMA_VECINFO_START_DATA);
    return Data;
}

void XBs_insert_kernel_InterruptGlobalEnable(XBs_insert_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBs_insert_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_GIE, 1);
}

void XBs_insert_kernel_InterruptGlobalDisable(XBs_insert_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBs_insert_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_GIE, 0);
}

void XBs_insert_kernel_InterruptEnable(XBs_insert_kernel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XBs_insert_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_IER);
    XBs_insert_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_IER, Register | Mask);
}

void XBs_insert_kernel_InterruptDisable(XBs_insert_kernel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XBs_insert_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_IER);
    XBs_insert_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_IER, Register & (~Mask));
}

void XBs_insert_kernel_InterruptClear(XBs_insert_kernel *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBs_insert_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_ISR, Mask);
}

u32 XBs_insert_kernel_InterruptGetEnabled(XBs_insert_kernel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XBs_insert_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_IER);
}

u32 XBs_insert_kernel_InterruptGetStatus(XBs_insert_kernel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XBs_insert_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBS_INSERT_KERNEL_CONTROL_ADDR_ISR);
}

