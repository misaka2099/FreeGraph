// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#include "xparameters.h"
#include "xalloc_access_kernel.h"

extern XAlloc_access_kernel_Config XAlloc_access_kernel_ConfigTable[];

XAlloc_access_kernel_Config *XAlloc_access_kernel_LookupConfig(u16 DeviceId) {
	XAlloc_access_kernel_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XALLOC_ACCESS_KERNEL_NUM_INSTANCES; Index++) {
		if (XAlloc_access_kernel_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XAlloc_access_kernel_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XAlloc_access_kernel_Initialize(XAlloc_access_kernel *InstancePtr, u16 DeviceId) {
	XAlloc_access_kernel_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XAlloc_access_kernel_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XAlloc_access_kernel_CfgInitialize(InstancePtr, ConfigPtr);
}

#endif

