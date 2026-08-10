// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#include "xparameters.h"
#include "xmem_access_kernel.h"

extern XMem_access_kernel_Config XMem_access_kernel_ConfigTable[];

XMem_access_kernel_Config *XMem_access_kernel_LookupConfig(u16 DeviceId) {
	XMem_access_kernel_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XMEM_ACCESS_KERNEL_NUM_INSTANCES; Index++) {
		if (XMem_access_kernel_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XMem_access_kernel_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XMem_access_kernel_Initialize(XMem_access_kernel *InstancePtr, u16 DeviceId) {
	XMem_access_kernel_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XMem_access_kernel_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XMem_access_kernel_CfgInitialize(InstancePtr, ConfigPtr);
}

#endif

