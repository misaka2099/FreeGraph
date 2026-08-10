// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#include "xparameters.h"
#include "xmerge_pipeline.h"

extern XMerge_pipeline_Config XMerge_pipeline_ConfigTable[];

XMerge_pipeline_Config *XMerge_pipeline_LookupConfig(u16 DeviceId) {
	XMerge_pipeline_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XMERGE_PIPELINE_NUM_INSTANCES; Index++) {
		if (XMerge_pipeline_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XMerge_pipeline_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XMerge_pipeline_Initialize(XMerge_pipeline *InstancePtr, u16 DeviceId) {
	XMerge_pipeline_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XMerge_pipeline_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XMerge_pipeline_CfgInitialize(InstancePtr, ConfigPtr);
}

#endif

