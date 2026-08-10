// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
/***************************** Include Files *********************************/
#include "xmerge_pipeline.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XMerge_pipeline_CfgInitialize(XMerge_pipeline *InstancePtr, XMerge_pipeline_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XMerge_pipeline_Start(XMerge_pipeline *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMerge_pipeline_ReadReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_AP_CTRL) & 0x80;
    XMerge_pipeline_WriteReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XMerge_pipeline_IsDone(XMerge_pipeline *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMerge_pipeline_ReadReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XMerge_pipeline_IsIdle(XMerge_pipeline *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMerge_pipeline_ReadReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XMerge_pipeline_IsReady(XMerge_pipeline *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMerge_pipeline_ReadReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XMerge_pipeline_Continue(XMerge_pipeline *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMerge_pipeline_ReadReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_AP_CTRL) & 0x80;
    XMerge_pipeline_WriteReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_AP_CTRL, Data | 0x10);
}

void XMerge_pipeline_EnableAutoRestart(XMerge_pipeline *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMerge_pipeline_WriteReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XMerge_pipeline_DisableAutoRestart(XMerge_pipeline *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMerge_pipeline_WriteReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_AP_CTRL, 0);
}

void XMerge_pipeline_Set_hbm_pma(XMerge_pipeline *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMerge_pipeline_WriteReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_HBM_PMA_DATA, (u32)(Data));
    XMerge_pipeline_WriteReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_HBM_PMA_DATA + 4, (u32)(Data >> 32));
}

u64 XMerge_pipeline_Get_hbm_pma(XMerge_pipeline *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMerge_pipeline_ReadReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_HBM_PMA_DATA);
    Data += (u64)XMerge_pipeline_ReadReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_HBM_PMA_DATA + 4) << 32;
    return Data;
}

void XMerge_pipeline_Set_hbm_buffer(XMerge_pipeline *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMerge_pipeline_WriteReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_HBM_BUFFER_DATA, (u32)(Data));
    XMerge_pipeline_WriteReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_HBM_BUFFER_DATA + 4, (u32)(Data >> 32));
}

u64 XMerge_pipeline_Get_hbm_buffer(XMerge_pipeline *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMerge_pipeline_ReadReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_HBM_BUFFER_DATA);
    Data += (u64)XMerge_pipeline_ReadReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_HBM_BUFFER_DATA + 4) << 32;
    return Data;
}

void XMerge_pipeline_Set_pma_raw_data_start(XMerge_pipeline *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMerge_pipeline_WriteReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_PMA_RAW_DATA_START_DATA, Data);
}

u32 XMerge_pipeline_Get_pma_raw_data_start(XMerge_pipeline *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMerge_pipeline_ReadReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_PMA_RAW_DATA_START_DATA);
    return Data;
}

void XMerge_pipeline_Set_pma_vecinfo_start(XMerge_pipeline *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMerge_pipeline_WriteReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_PMA_VECINFO_START_DATA, Data);
}

u32 XMerge_pipeline_Get_pma_vecinfo_start(XMerge_pipeline *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMerge_pipeline_ReadReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_PMA_VECINFO_START_DATA);
    return Data;
}

void XMerge_pipeline_InterruptGlobalEnable(XMerge_pipeline *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMerge_pipeline_WriteReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_GIE, 1);
}

void XMerge_pipeline_InterruptGlobalDisable(XMerge_pipeline *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMerge_pipeline_WriteReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_GIE, 0);
}

void XMerge_pipeline_InterruptEnable(XMerge_pipeline *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XMerge_pipeline_ReadReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_IER);
    XMerge_pipeline_WriteReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_IER, Register | Mask);
}

void XMerge_pipeline_InterruptDisable(XMerge_pipeline *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XMerge_pipeline_ReadReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_IER);
    XMerge_pipeline_WriteReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_IER, Register & (~Mask));
}

void XMerge_pipeline_InterruptClear(XMerge_pipeline *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMerge_pipeline_WriteReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_ISR, Mask);
}

u32 XMerge_pipeline_InterruptGetEnabled(XMerge_pipeline *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XMerge_pipeline_ReadReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_IER);
}

u32 XMerge_pipeline_InterruptGetStatus(XMerge_pipeline *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XMerge_pipeline_ReadReg(InstancePtr->Control_BaseAddress, XMERGE_PIPELINE_CONTROL_ADDR_ISR);
}

