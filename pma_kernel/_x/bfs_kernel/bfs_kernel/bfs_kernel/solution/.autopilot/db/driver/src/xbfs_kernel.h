// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef XBFS_KERNEL_H
#define XBFS_KERNEL_H

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
#include "xbfs_kernel_hw.h"

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
} XBfs_kernel_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XBfs_kernel;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XBfs_kernel_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XBfs_kernel_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XBfs_kernel_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XBfs_kernel_ReadReg(BaseAddress, RegOffset) \
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
int XBfs_kernel_Initialize(XBfs_kernel *InstancePtr, u16 DeviceId);
XBfs_kernel_Config* XBfs_kernel_LookupConfig(u16 DeviceId);
int XBfs_kernel_CfgInitialize(XBfs_kernel *InstancePtr, XBfs_kernel_Config *ConfigPtr);
#else
int XBfs_kernel_Initialize(XBfs_kernel *InstancePtr, const char* InstanceName);
int XBfs_kernel_Release(XBfs_kernel *InstancePtr);
#endif

void XBfs_kernel_Start(XBfs_kernel *InstancePtr);
u32 XBfs_kernel_IsDone(XBfs_kernel *InstancePtr);
u32 XBfs_kernel_IsIdle(XBfs_kernel *InstancePtr);
u32 XBfs_kernel_IsReady(XBfs_kernel *InstancePtr);
void XBfs_kernel_Continue(XBfs_kernel *InstancePtr);
void XBfs_kernel_EnableAutoRestart(XBfs_kernel *InstancePtr);
void XBfs_kernel_DisableAutoRestart(XBfs_kernel *InstancePtr);

void XBfs_kernel_Set_bfs_start(XBfs_kernel *InstancePtr, u32 Data);
u32 XBfs_kernel_Get_bfs_start(XBfs_kernel *InstancePtr);
void XBfs_kernel_Set_bfs_number_in(XBfs_kernel *InstancePtr, u64 Data);
u64 XBfs_kernel_Get_bfs_number_in(XBfs_kernel *InstancePtr);
void XBfs_kernel_Set_bfs_number_out(XBfs_kernel *InstancePtr, u64 Data);
u64 XBfs_kernel_Get_bfs_number_out(XBfs_kernel *InstancePtr);
void XBfs_kernel_Set_graph_size(XBfs_kernel *InstancePtr, u32 Data);
u32 XBfs_kernel_Get_graph_size(XBfs_kernel *InstancePtr);
void XBfs_kernel_Set_max_rounds(XBfs_kernel *InstancePtr, u32 Data);
u32 XBfs_kernel_Get_max_rounds(XBfs_kernel *InstancePtr);
void XBfs_kernel_Set_last(XBfs_kernel *InstancePtr, u32 Data);
u32 XBfs_kernel_Get_last(XBfs_kernel *InstancePtr);

void XBfs_kernel_InterruptGlobalEnable(XBfs_kernel *InstancePtr);
void XBfs_kernel_InterruptGlobalDisable(XBfs_kernel *InstancePtr);
void XBfs_kernel_InterruptEnable(XBfs_kernel *InstancePtr, u32 Mask);
void XBfs_kernel_InterruptDisable(XBfs_kernel *InstancePtr, u32 Mask);
void XBfs_kernel_InterruptClear(XBfs_kernel *InstancePtr, u32 Mask);
u32 XBfs_kernel_InterruptGetEnabled(XBfs_kernel *InstancePtr);
u32 XBfs_kernel_InterruptGetStatus(XBfs_kernel *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
