// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef XUPDATE_MESSAGE_GENERATOR_H
#define XUPDATE_MESSAGE_GENERATOR_H

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
#include "xupdate_message_generator_hw.h"

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
} XUpdate_message_generator_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XUpdate_message_generator;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XUpdate_message_generator_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XUpdate_message_generator_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XUpdate_message_generator_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XUpdate_message_generator_ReadReg(BaseAddress, RegOffset) \
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
int XUpdate_message_generator_Initialize(XUpdate_message_generator *InstancePtr, u16 DeviceId);
XUpdate_message_generator_Config* XUpdate_message_generator_LookupConfig(u16 DeviceId);
int XUpdate_message_generator_CfgInitialize(XUpdate_message_generator *InstancePtr, XUpdate_message_generator_Config *ConfigPtr);
#else
int XUpdate_message_generator_Initialize(XUpdate_message_generator *InstancePtr, const char* InstanceName);
int XUpdate_message_generator_Release(XUpdate_message_generator *InstancePtr);
#endif

void XUpdate_message_generator_Start(XUpdate_message_generator *InstancePtr);
u32 XUpdate_message_generator_IsDone(XUpdate_message_generator *InstancePtr);
u32 XUpdate_message_generator_IsIdle(XUpdate_message_generator *InstancePtr);
u32 XUpdate_message_generator_IsReady(XUpdate_message_generator *InstancePtr);
void XUpdate_message_generator_Continue(XUpdate_message_generator *InstancePtr);
void XUpdate_message_generator_EnableAutoRestart(XUpdate_message_generator *InstancePtr);
void XUpdate_message_generator_DisableAutoRestart(XUpdate_message_generator *InstancePtr);

void XUpdate_message_generator_Set_op_num(XUpdate_message_generator *InstancePtr, u32 Data);
u32 XUpdate_message_generator_Get_op_num(XUpdate_message_generator *InstancePtr);
void XUpdate_message_generator_Set_src(XUpdate_message_generator *InstancePtr, u64 Data);
u64 XUpdate_message_generator_Get_src(XUpdate_message_generator *InstancePtr);
void XUpdate_message_generator_Set_dst(XUpdate_message_generator *InstancePtr, u64 Data);
u64 XUpdate_message_generator_Get_dst(XUpdate_message_generator *InstancePtr);
void XUpdate_message_generator_Set_opcode(XUpdate_message_generator *InstancePtr, u64 Data);
u64 XUpdate_message_generator_Get_opcode(XUpdate_message_generator *InstancePtr);
void XUpdate_message_generator_Set_last(XUpdate_message_generator *InstancePtr, u32 Data);
u32 XUpdate_message_generator_Get_last(XUpdate_message_generator *InstancePtr);

void XUpdate_message_generator_InterruptGlobalEnable(XUpdate_message_generator *InstancePtr);
void XUpdate_message_generator_InterruptGlobalDisable(XUpdate_message_generator *InstancePtr);
void XUpdate_message_generator_InterruptEnable(XUpdate_message_generator *InstancePtr, u32 Mask);
void XUpdate_message_generator_InterruptDisable(XUpdate_message_generator *InstancePtr, u32 Mask);
void XUpdate_message_generator_InterruptClear(XUpdate_message_generator *InstancePtr, u32 Mask);
u32 XUpdate_message_generator_InterruptGetEnabled(XUpdate_message_generator *InstancePtr);
u32 XUpdate_message_generator_InterruptGetStatus(XUpdate_message_generator *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
