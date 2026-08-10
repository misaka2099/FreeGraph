// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
/***************************** Include Files *********************************/
#include "xcheck_kernel.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XCheck_kernel_CfgInitialize(XCheck_kernel *InstancePtr, XCheck_kernel_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XCheck_kernel_Start(XCheck_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XCheck_kernel_ReadReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XCheck_kernel_WriteReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XCheck_kernel_IsDone(XCheck_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XCheck_kernel_ReadReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XCheck_kernel_IsIdle(XCheck_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XCheck_kernel_ReadReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XCheck_kernel_IsReady(XCheck_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XCheck_kernel_ReadReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XCheck_kernel_Continue(XCheck_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XCheck_kernel_ReadReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XCheck_kernel_WriteReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_AP_CTRL, Data | 0x10);
}

void XCheck_kernel_EnableAutoRestart(XCheck_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XCheck_kernel_WriteReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XCheck_kernel_DisableAutoRestart(XCheck_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XCheck_kernel_WriteReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_AP_CTRL, 0);
}

void XCheck_kernel_Set_coo_src(XCheck_kernel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XCheck_kernel_WriteReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_COO_SRC_DATA, (u32)(Data));
    XCheck_kernel_WriteReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_COO_SRC_DATA + 4, (u32)(Data >> 32));
}

u64 XCheck_kernel_Get_coo_src(XCheck_kernel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XCheck_kernel_ReadReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_COO_SRC_DATA);
    Data += (u64)XCheck_kernel_ReadReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_COO_SRC_DATA + 4) << 32;
    return Data;
}

void XCheck_kernel_Set_coo_dst(XCheck_kernel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XCheck_kernel_WriteReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_COO_DST_DATA, (u32)(Data));
    XCheck_kernel_WriteReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_COO_DST_DATA + 4, (u32)(Data >> 32));
}

u64 XCheck_kernel_Get_coo_dst(XCheck_kernel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XCheck_kernel_ReadReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_COO_DST_DATA);
    Data += (u64)XCheck_kernel_ReadReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_COO_DST_DATA + 4) << 32;
    return Data;
}

void XCheck_kernel_Set_num_edges(XCheck_kernel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XCheck_kernel_WriteReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_NUM_EDGES_DATA, (u32)(Data));
    XCheck_kernel_WriteReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_NUM_EDGES_DATA + 4, (u32)(Data >> 32));
}

u64 XCheck_kernel_Get_num_edges(XCheck_kernel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XCheck_kernel_ReadReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_NUM_EDGES_DATA);
    Data += (u64)XCheck_kernel_ReadReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_NUM_EDGES_DATA + 4) << 32;
    return Data;
}

void XCheck_kernel_Set_vertex_to_check(XCheck_kernel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XCheck_kernel_WriteReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_VERTEX_TO_CHECK_DATA, Data);
}

u32 XCheck_kernel_Get_vertex_to_check(XCheck_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XCheck_kernel_ReadReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_VERTEX_TO_CHECK_DATA);
    return Data;
}

void XCheck_kernel_Set_mode(XCheck_kernel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XCheck_kernel_WriteReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_MODE_DATA, Data);
}

u32 XCheck_kernel_Get_mode(XCheck_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XCheck_kernel_ReadReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_MODE_DATA);
    return Data;
}

void XCheck_kernel_Set_last(XCheck_kernel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XCheck_kernel_WriteReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_LAST_DATA, Data);
}

u32 XCheck_kernel_Get_last(XCheck_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XCheck_kernel_ReadReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_LAST_DATA);
    return Data;
}

void XCheck_kernel_InterruptGlobalEnable(XCheck_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XCheck_kernel_WriteReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_GIE, 1);
}

void XCheck_kernel_InterruptGlobalDisable(XCheck_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XCheck_kernel_WriteReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_GIE, 0);
}

void XCheck_kernel_InterruptEnable(XCheck_kernel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XCheck_kernel_ReadReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_IER);
    XCheck_kernel_WriteReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_IER, Register | Mask);
}

void XCheck_kernel_InterruptDisable(XCheck_kernel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XCheck_kernel_ReadReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_IER);
    XCheck_kernel_WriteReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_IER, Register & (~Mask));
}

void XCheck_kernel_InterruptClear(XCheck_kernel *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XCheck_kernel_WriteReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_ISR, Mask);
}

u32 XCheck_kernel_InterruptGetEnabled(XCheck_kernel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XCheck_kernel_ReadReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_IER);
}

u32 XCheck_kernel_InterruptGetStatus(XCheck_kernel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XCheck_kernel_ReadReg(InstancePtr->Control_BaseAddress, XCHECK_KERNEL_CONTROL_ADDR_ISR);
}

