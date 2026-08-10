// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#include "xparameters.h"
#include "xbs_insert_kernel.h"

extern XBs_insert_kernel_Config XBs_insert_kernel_ConfigTable[];

XBs_insert_kernel_Config *XBs_insert_kernel_LookupConfig(u16 DeviceId) {
	XBs_insert_kernel_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XBS_INSERT_KERNEL_NUM_INSTANCES; Index++) {
		if (XBs_insert_kernel_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XBs_insert_kernel_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XBs_insert_kernel_Initialize(XBs_insert_kernel *InstancePtr, u16 DeviceId) {
	XBs_insert_kernel_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XBs_insert_kernel_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XBs_insert_kernel_CfgInitialize(InstancePtr, ConfigPtr);
}

#endif

