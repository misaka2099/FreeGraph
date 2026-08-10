// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#include "xparameters.h"
#include "xcheck_kernel.h"

extern XCheck_kernel_Config XCheck_kernel_ConfigTable[];

XCheck_kernel_Config *XCheck_kernel_LookupConfig(u16 DeviceId) {
	XCheck_kernel_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XCHECK_KERNEL_NUM_INSTANCES; Index++) {
		if (XCheck_kernel_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XCheck_kernel_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XCheck_kernel_Initialize(XCheck_kernel *InstancePtr, u16 DeviceId) {
	XCheck_kernel_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XCheck_kernel_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XCheck_kernel_CfgInitialize(InstancePtr, ConfigPtr);
}

#endif

