// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef XUPDATE_MESSAGE_ROUTER_H
#define XUPDATE_MESSAGE_ROUTER_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/
#ifndef __linux__
#include "xil_types.h"
#include "xil_assert.h"
#include "xstatus.h"
#include "xil_io.h"
#else
#include <stdint.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stddef.h>
#endif
#include "xupdate_message_router_hw.h"

/**************************** Type Definitions ******************************/
#ifdef __linux__
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#else
typedef struct {
    u16 DeviceId;
    u64 Control_BaseAddress;
} XUpdate_message_router_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XUpdate_message_router;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XUpdate_message_router_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XUpdate_message_router_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XUpdate_message_router_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XUpdate_message_router_ReadReg(BaseAddress, RegOffset) \
    *(volatile u32*)((BaseAddress) + (RegOffset))

#define Xil_AssertVoid(expr)    assert(expr)
#define Xil_AssertNonvoid(expr) assert(expr)

#define XST_SUCCESS             0
#define XST_DEVICE_NOT_FOUND    2
#define XST_OPEN_DEVICE_FAILED  3
#define XIL_COMPONENT_IS_READY  1
#endif

/************************** Function Prototypes *****************************/
#ifndef __linux__
int XUpdate_message_router_Initialize(XUpdate_message_router *InstancePtr, u16 DeviceId);
XUpdate_message_router_Config* XUpdate_message_router_LookupConfig(u16 DeviceId);
int XUpdate_message_router_CfgInitialize(XUpdate_message_router *InstancePtr, XUpdate_message_router_Config *ConfigPtr);
#else
int XUpdate_message_router_Initialize(XUpdate_message_router *InstancePtr, const char* InstanceName);
int XUpdate_message_router_Release(XUpdate_message_router *InstancePtr);
#endif

void XUpdate_message_router_Start(XUpdate_message_router *InstancePtr);
u32 XUpdate_message_router_IsDone(XUpdate_message_router *InstancePtr);
u32 XUpdate_message_router_IsIdle(XUpdate_message_router *InstancePtr);
u32 XUpdate_message_router_IsReady(XUpdate_message_router *InstancePtr);
void XUpdate_message_router_Continue(XUpdate_message_router *InstancePtr);
void XUpdate_message_router_EnableAutoRestart(XUpdate_message_router *InstancePtr);
void XUpdate_message_router_DisableAutoRestart(XUpdate_message_router *InstancePtr);


void XUpdate_message_router_InterruptGlobalEnable(XUpdate_message_router *InstancePtr);
void XUpdate_message_router_InterruptGlobalDisable(XUpdate_message_router *InstancePtr);
void XUpdate_message_router_InterruptEnable(XUpdate_message_router *InstancePtr, u32 Mask);
void XUpdate_message_router_InterruptDisable(XUpdate_message_router *InstancePtr, u32 Mask);
void XUpdate_message_router_InterruptClear(XUpdate_message_router *InstancePtr, u32 Mask);
u32 XUpdate_message_router_InterruptGetEnabled(XUpdate_message_router *InstancePtr);
u32 XUpdate_message_router_InterruptGetStatus(XUpdate_message_router *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
