// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#include "xparameters.h"
#include "xredistribution_kernel.h"

extern XRedistribution_kernel_Config XRedistribution_kernel_ConfigTable[];

XRedistribution_kernel_Config *XRedistribution_kernel_LookupConfig(u16 DeviceId) {
	XRedistribution_kernel_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XREDISTRIBUTION_KERNEL_NUM_INSTANCES; Index++) {
		if (XRedistribution_kernel_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XRedistribution_kernel_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XRedistribution_kernel_Initialize(XRedistribution_kernel *InstancePtr, u16 DeviceId) {
	XRedistribution_kernel_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XRedistribution_kernel_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XRedistribution_kernel_CfgInitialize(InstancePtr, ConfigPtr);
}

#endif

