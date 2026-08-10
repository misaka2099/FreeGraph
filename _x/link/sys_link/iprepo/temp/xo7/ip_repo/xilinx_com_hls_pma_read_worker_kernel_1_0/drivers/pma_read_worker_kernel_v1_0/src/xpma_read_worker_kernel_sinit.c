// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#include "xparameters.h"
#include "xpma_read_worker_kernel.h"

extern XPma_read_worker_kernel_Config XPma_read_worker_kernel_ConfigTable[];

XPma_read_worker_kernel_Config *XPma_read_worker_kernel_LookupConfig(u16 DeviceId) {
	XPma_read_worker_kernel_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XPMA_READ_WORKER_KERNEL_NUM_INSTANCES; Index++) {
		if (XPma_read_worker_kernel_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XPma_read_worker_kernel_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XPma_read_worker_kernel_Initialize(XPma_read_worker_kernel *InstancePtr, u16 DeviceId) {
	XPma_read_worker_kernel_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XPma_read_worker_kernel_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XPma_read_worker_kernel_CfgInitialize(InstancePtr, ConfigPtr);
}

#endif

