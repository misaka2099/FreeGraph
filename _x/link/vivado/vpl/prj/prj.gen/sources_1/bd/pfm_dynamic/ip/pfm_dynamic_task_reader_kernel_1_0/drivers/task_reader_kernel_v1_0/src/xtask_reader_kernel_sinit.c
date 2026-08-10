// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#include "xparameters.h"
#include "xtask_reader_kernel.h"

extern XTask_reader_kernel_Config XTask_reader_kernel_ConfigTable[];

XTask_reader_kernel_Config *XTask_reader_kernel_LookupConfig(u16 DeviceId) {
	XTask_reader_kernel_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XTASK_READER_KERNEL_NUM_INSTANCES; Index++) {
		if (XTask_reader_kernel_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XTask_reader_kernel_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XTask_reader_kernel_Initialize(XTask_reader_kernel *InstancePtr, u16 DeviceId) {
	XTask_reader_kernel_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XTask_reader_kernel_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XTask_reader_kernel_CfgInitialize(InstancePtr, ConfigPtr);
}

#endif

