// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#include "xparameters.h"
#include "xupdate_message_router.h"

extern XUpdate_message_router_Config XUpdate_message_router_ConfigTable[];

XUpdate_message_router_Config *XUpdate_message_router_LookupConfig(u16 DeviceId) {
	XUpdate_message_router_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XUPDATE_MESSAGE_ROUTER_NUM_INSTANCES; Index++) {
		if (XUpdate_message_router_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XUpdate_message_router_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XUpdate_message_router_Initialize(XUpdate_message_router *InstancePtr, u16 DeviceId) {
	XUpdate_message_router_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XUpdate_message_router_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XUpdate_message_router_CfgInitialize(InstancePtr, ConfigPtr);
}

#endif

