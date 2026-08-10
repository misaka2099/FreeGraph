// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#include "xparameters.h"
#include "xbfs_kernel.h"

extern XBfs_kernel_Config XBfs_kernel_ConfigTable[];

XBfs_kernel_Config *XBfs_kernel_LookupConfig(u16 DeviceId) {
	XBfs_kernel_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XBFS_KERNEL_NUM_INSTANCES; Index++) {
		if (XBfs_kernel_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XBfs_kernel_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XBfs_kernel_Initialize(XBfs_kernel *InstancePtr, u16 DeviceId) {
	XBfs_kernel_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XBfs_kernel_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XBfs_kernel_CfgInitialize(InstancePtr, ConfigPtr);
}

#endif

