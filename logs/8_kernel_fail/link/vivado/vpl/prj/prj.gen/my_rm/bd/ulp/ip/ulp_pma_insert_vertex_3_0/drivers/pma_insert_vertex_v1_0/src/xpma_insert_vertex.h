// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef XPMA_INSERT_VERTEX_H
#define XPMA_INSERT_VERTEX_H

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
#include "xpma_insert_vertex_hw.h"

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
} XPma_insert_vertex_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XPma_insert_vertex;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XPma_insert_vertex_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XPma_insert_vertex_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XPma_insert_vertex_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XPma_insert_vertex_ReadReg(BaseAddress, RegOffset) \
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
int XPma_insert_vertex_Initialize(XPma_insert_vertex *InstancePtr, u16 DeviceId);
XPma_insert_vertex_Config* XPma_insert_vertex_LookupConfig(u16 DeviceId);
int XPma_insert_vertex_CfgInitialize(XPma_insert_vertex *InstancePtr, XPma_insert_vertex_Config *ConfigPtr);
#else
int XPma_insert_vertex_Initialize(XPma_insert_vertex *InstancePtr, const char* InstanceName);
int XPma_insert_vertex_Release(XPma_insert_vertex *InstancePtr);
#endif

void XPma_insert_vertex_Start(XPma_insert_vertex *InstancePtr);
u32 XPma_insert_vertex_IsDone(XPma_insert_vertex *InstancePtr);
u32 XPma_insert_vertex_IsIdle(XPma_insert_vertex *InstancePtr);
u32 XPma_insert_vertex_IsReady(XPma_insert_vertex *InstancePtr);
void XPma_insert_vertex_Continue(XPma_insert_vertex *InstancePtr);
void XPma_insert_vertex_EnableAutoRestart(XPma_insert_vertex *InstancePtr);
void XPma_insert_vertex_DisableAutoRestart(XPma_insert_vertex *InstancePtr);


void XPma_insert_vertex_InterruptGlobalEnable(XPma_insert_vertex *InstancePtr);
void XPma_insert_vertex_InterruptGlobalDisable(XPma_insert_vertex *InstancePtr);
void XPma_insert_vertex_InterruptEnable(XPma_insert_vertex *InstancePtr, u32 Mask);
void XPma_insert_vertex_InterruptDisable(XPma_insert_vertex *InstancePtr, u32 Mask);
void XPma_insert_vertex_InterruptClear(XPma_insert_vertex *InstancePtr, u32 Mask);
u32 XPma_insert_vertex_InterruptGetEnabled(XPma_insert_vertex *InstancePtr);
u32 XPma_insert_vertex_InterruptGetStatus(XPma_insert_vertex *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
