// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
/***************************** Include Files *********************************/
#include "xpma_insert_edge.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XPma_insert_edge_CfgInitialize(XPma_insert_edge *InstancePtr, XPma_insert_edge_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XPma_insert_edge_Start(XPma_insert_edge *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_insert_edge_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_AP_CTRL) & 0x80;
    XPma_insert_edge_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XPma_insert_edge_IsDone(XPma_insert_edge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_insert_edge_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XPma_insert_edge_IsIdle(XPma_insert_edge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_insert_edge_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XPma_insert_edge_IsReady(XPma_insert_edge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_insert_edge_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XPma_insert_edge_Continue(XPma_insert_edge *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_insert_edge_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_AP_CTRL) & 0x80;
    XPma_insert_edge_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_AP_CTRL, Data | 0x10);
}

void XPma_insert_edge_EnableAutoRestart(XPma_insert_edge *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_insert_edge_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XPma_insert_edge_DisableAutoRestart(XPma_insert_edge *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_insert_edge_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_AP_CTRL, 0);
}

void XPma_insert_edge_Set_hbm_gate(XPma_insert_edge *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_insert_edge_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_HBM_GATE_DATA, (u32)(Data));
    XPma_insert_edge_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_HBM_GATE_DATA + 4, (u32)(Data >> 32));
}

u64 XPma_insert_edge_Get_hbm_gate(XPma_insert_edge *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_insert_edge_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_HBM_GATE_DATA);
    Data += (u64)XPma_insert_edge_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_HBM_GATE_DATA + 4) << 32;
    return Data;
}

void XPma_insert_edge_Set_hbm_bs(XPma_insert_edge *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_insert_edge_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_HBM_BS_DATA, (u32)(Data));
    XPma_insert_edge_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_HBM_BS_DATA + 4, (u32)(Data >> 32));
}

u64 XPma_insert_edge_Get_hbm_bs(XPma_insert_edge *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_insert_edge_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_HBM_BS_DATA);
    Data += (u64)XPma_insert_edge_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_HBM_BS_DATA + 4) << 32;
    return Data;
}

void XPma_insert_edge_Set_hbm_rw(XPma_insert_edge *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_insert_edge_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_HBM_RW_DATA, (u32)(Data));
    XPma_insert_edge_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_HBM_RW_DATA + 4, (u32)(Data >> 32));
}

u64 XPma_insert_edge_Get_hbm_rw(XPma_insert_edge *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_insert_edge_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_HBM_RW_DATA);
    Data += (u64)XPma_insert_edge_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_HBM_RW_DATA + 4) << 32;
    return Data;
}

void XPma_insert_edge_Set_hbm_red(XPma_insert_edge *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_insert_edge_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_HBM_RED_DATA, (u32)(Data));
    XPma_insert_edge_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_HBM_RED_DATA + 4, (u32)(Data >> 32));
}

u64 XPma_insert_edge_Get_hbm_red(XPma_insert_edge *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_insert_edge_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_HBM_RED_DATA);
    Data += (u64)XPma_insert_edge_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_HBM_RED_DATA + 4) << 32;
    return Data;
}

void XPma_insert_edge_Set_pma_raw_data_start(XPma_insert_edge *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_insert_edge_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_PMA_RAW_DATA_START_DATA, Data);
}

u32 XPma_insert_edge_Get_pma_raw_data_start(XPma_insert_edge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_insert_edge_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_PMA_RAW_DATA_START_DATA);
    return Data;
}

void XPma_insert_edge_Set_pma_vecinfo_start(XPma_insert_edge *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_insert_edge_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_PMA_VECINFO_START_DATA, Data);
}

u32 XPma_insert_edge_Get_pma_vecinfo_start(XPma_insert_edge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XPma_insert_edge_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_PMA_VECINFO_START_DATA);
    return Data;
}

void XPma_insert_edge_InterruptGlobalEnable(XPma_insert_edge *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_insert_edge_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_GIE, 1);
}

void XPma_insert_edge_InterruptGlobalDisable(XPma_insert_edge *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_insert_edge_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_GIE, 0);
}

void XPma_insert_edge_InterruptEnable(XPma_insert_edge *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XPma_insert_edge_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_IER);
    XPma_insert_edge_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_IER, Register | Mask);
}

void XPma_insert_edge_InterruptDisable(XPma_insert_edge *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XPma_insert_edge_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_IER);
    XPma_insert_edge_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_IER, Register & (~Mask));
}

void XPma_insert_edge_InterruptClear(XPma_insert_edge *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XPma_insert_edge_WriteReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_ISR, Mask);
}

u32 XPma_insert_edge_InterruptGetEnabled(XPma_insert_edge *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XPma_insert_edge_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_IER);
}

u32 XPma_insert_edge_InterruptGetStatus(XPma_insert_edge *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XPma_insert_edge_ReadReg(InstancePtr->Control_BaseAddress, XPMA_INSERT_EDGE_CONTROL_ADDR_ISR);
}

