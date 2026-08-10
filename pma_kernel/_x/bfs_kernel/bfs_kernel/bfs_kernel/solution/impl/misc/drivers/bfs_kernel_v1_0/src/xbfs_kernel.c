// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
/***************************** Include Files *********************************/
#include "xbfs_kernel.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XBfs_kernel_CfgInitialize(XBfs_kernel *InstancePtr, XBfs_kernel_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XBfs_kernel_Start(XBfs_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBfs_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XBfs_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XBfs_kernel_IsDone(XBfs_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBfs_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XBfs_kernel_IsIdle(XBfs_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBfs_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XBfs_kernel_IsReady(XBfs_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBfs_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XBfs_kernel_Continue(XBfs_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBfs_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XBfs_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_AP_CTRL, Data | 0x10);
}

void XBfs_kernel_EnableAutoRestart(XBfs_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBfs_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XBfs_kernel_DisableAutoRestart(XBfs_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBfs_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_AP_CTRL, 0);
}

void XBfs_kernel_Set_bfs_start(XBfs_kernel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBfs_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_BFS_START_DATA, Data);
}

u32 XBfs_kernel_Get_bfs_start(XBfs_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBfs_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_BFS_START_DATA);
    return Data;
}

void XBfs_kernel_Set_bfs_number_in(XBfs_kernel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBfs_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_BFS_NUMBER_IN_DATA, (u32)(Data));
    XBfs_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_BFS_NUMBER_IN_DATA + 4, (u32)(Data >> 32));
}

u64 XBfs_kernel_Get_bfs_number_in(XBfs_kernel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBfs_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_BFS_NUMBER_IN_DATA);
    Data += (u64)XBfs_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_BFS_NUMBER_IN_DATA + 4) << 32;
    return Data;
}

void XBfs_kernel_Set_bfs_number_out(XBfs_kernel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBfs_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_BFS_NUMBER_OUT_DATA, (u32)(Data));
    XBfs_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_BFS_NUMBER_OUT_DATA + 4, (u32)(Data >> 32));
}

u64 XBfs_kernel_Get_bfs_number_out(XBfs_kernel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBfs_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_BFS_NUMBER_OUT_DATA);
    Data += (u64)XBfs_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_BFS_NUMBER_OUT_DATA + 4) << 32;
    return Data;
}

void XBfs_kernel_Set_graph_size(XBfs_kernel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBfs_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_GRAPH_SIZE_DATA, Data);
}

u32 XBfs_kernel_Get_graph_size(XBfs_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBfs_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_GRAPH_SIZE_DATA);
    return Data;
}

void XBfs_kernel_Set_max_rounds(XBfs_kernel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBfs_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_MAX_ROUNDS_DATA, Data);
}

u32 XBfs_kernel_Get_max_rounds(XBfs_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBfs_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_MAX_ROUNDS_DATA);
    return Data;
}

void XBfs_kernel_Set_last(XBfs_kernel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBfs_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_LAST_DATA, Data);
}

u32 XBfs_kernel_Get_last(XBfs_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBfs_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_LAST_DATA);
    return Data;
}

void XBfs_kernel_InterruptGlobalEnable(XBfs_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBfs_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_GIE, 1);
}

void XBfs_kernel_InterruptGlobalDisable(XBfs_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBfs_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_GIE, 0);
}

void XBfs_kernel_InterruptEnable(XBfs_kernel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XBfs_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_IER);
    XBfs_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_IER, Register | Mask);
}

void XBfs_kernel_InterruptDisable(XBfs_kernel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XBfs_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_IER);
    XBfs_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_IER, Register & (~Mask));
}

void XBfs_kernel_InterruptClear(XBfs_kernel *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBfs_kernel_WriteReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_ISR, Mask);
}

u32 XBfs_kernel_InterruptGetEnabled(XBfs_kernel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XBfs_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_IER);
}

u32 XBfs_kernel_InterruptGetStatus(XBfs_kernel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XBfs_kernel_ReadReg(InstancePtr->Control_BaseAddress, XBFS_KERNEL_CONTROL_ADDR_ISR);
}

