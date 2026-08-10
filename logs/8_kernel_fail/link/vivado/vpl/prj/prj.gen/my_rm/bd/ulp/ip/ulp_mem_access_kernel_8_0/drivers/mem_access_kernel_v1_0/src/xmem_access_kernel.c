// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
/***************************** Include Files *********************************/
#include "xmem_access_kernel.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XMem_access_kernel_CfgInitialize(XMem_access_kernel *InstancePtr, XMem_access_kernel_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XMem_access_kernel_Start(XMem_access_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMem_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XMem_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XMem_access_kernel_IsDone(XMem_access_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMem_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XMem_access_kernel_IsIdle(XMem_access_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMem_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XMem_access_kernel_IsReady(XMem_access_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMem_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XMem_access_kernel_Continue(XMem_access_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMem_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XMem_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_AP_CTRL, Data | 0x10);
}

void XMem_access_kernel_EnableAutoRestart(XMem_access_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMem_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XMem_access_kernel_DisableAutoRestart(XMem_access_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMem_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_AP_CTRL, 0);
}

void XMem_access_kernel_Set_hbm_ptr(XMem_access_kernel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMem_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_HBM_PTR_DATA, (u32)(Data));
    XMem_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_HBM_PTR_DATA + 4, (u32)(Data >> 32));
}

u64 XMem_access_kernel_Get_hbm_ptr(XMem_access_kernel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMem_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_HBM_PTR_DATA);
    Data += (u64)XMem_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_HBM_PTR_DATA + 4) << 32;
    return Data;
}

void XMem_access_kernel_Set_pma_addrs(XMem_access_kernel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMem_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_PMA_ADDRS_DATA, (u32)(Data));
    XMem_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_PMA_ADDRS_DATA + 4, (u32)(Data >> 32));
}

u64 XMem_access_kernel_Get_pma_addrs(XMem_access_kernel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMem_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_PMA_ADDRS_DATA);
    Data += (u64)XMem_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_PMA_ADDRS_DATA + 4) << 32;
    return Data;
}

void XMem_access_kernel_InterruptGlobalEnable(XMem_access_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMem_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_GIE, 1);
}

void XMem_access_kernel_InterruptGlobalDisable(XMem_access_kernel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMem_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_GIE, 0);
}

void XMem_access_kernel_InterruptEnable(XMem_access_kernel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XMem_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_IER);
    XMem_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_IER, Register | Mask);
}

void XMem_access_kernel_InterruptDisable(XMem_access_kernel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XMem_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_IER);
    XMem_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_IER, Register & (~Mask));
}

void XMem_access_kernel_InterruptClear(XMem_access_kernel *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMem_access_kernel_WriteReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_ISR, Mask);
}

u32 XMem_access_kernel_InterruptGetEnabled(XMem_access_kernel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XMem_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_IER);
}

u32 XMem_access_kernel_InterruptGetStatus(XMem_access_kernel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XMem_access_kernel_ReadReg(InstancePtr->Control_BaseAddress, XMEM_ACCESS_KERNEL_CONTROL_ADDR_ISR);
}

