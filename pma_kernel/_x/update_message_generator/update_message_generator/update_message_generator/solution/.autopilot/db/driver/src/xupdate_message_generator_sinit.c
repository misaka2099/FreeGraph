// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#include "xparameters.h"
#include "xupdate_message_generator.h"

extern XUpdate_message_generator_Config XUpdate_message_generator_ConfigTable[];

XUpdate_message_generator_Config *XUpdate_message_generator_LookupConfig(u16 DeviceId) {
	XUpdate_message_generator_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XUPDATE_MESSAGE_GENERATOR_NUM_INSTANCES; Index++) {
		if (XUpdate_message_generator_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XUpdate_message_generator_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XUpdate_message_generator_Initialize(XUpdate_message_generator *InstancePtr, u16 DeviceId) {
	XUpdate_message_generator_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XUpdate_message_generator_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XUpdate_message_generator_CfgInitialize(InstancePtr, ConfigPtr);
}

#endif

