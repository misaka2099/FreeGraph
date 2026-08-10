// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#include "xparameters.h"
#include "xpma_insert_vertex.h"

extern XPma_insert_vertex_Config XPma_insert_vertex_ConfigTable[];

XPma_insert_vertex_Config *XPma_insert_vertex_LookupConfig(u16 DeviceId) {
	XPma_insert_vertex_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XPMA_INSERT_VERTEX_NUM_INSTANCES; Index++) {
		if (XPma_insert_vertex_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XPma_insert_vertex_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XPma_insert_vertex_Initialize(XPma_insert_vertex *InstancePtr, u16 DeviceId) {
	XPma_insert_vertex_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XPma_insert_vertex_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XPma_insert_vertex_CfgInitialize(InstancePtr, ConfigPtr);
}

#endif

