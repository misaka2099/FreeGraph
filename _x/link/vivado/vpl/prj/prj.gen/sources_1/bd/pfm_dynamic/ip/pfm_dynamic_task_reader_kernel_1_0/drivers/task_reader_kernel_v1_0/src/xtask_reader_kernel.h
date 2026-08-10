// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef XTASK_READER_KERNEL_H
#define XTASK_READER_KERNEL_H

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
#include "xtask_reader_kernel_hw.h"

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
} XTask_reader_kernel_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XTask_reader_kernel;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XTask_reader_kernel_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XTask_reader_kernel_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XTask_reader_kernel_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XTask_reader_kernel_ReadReg(BaseAddress, RegOffset) \
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
int XTask_reader_kernel_Initialize(XTask_reader_kernel *InstancePtr, u16 DeviceId);
XTask_reader_kernel_Config* XTask_reader_kernel_LookupConfig(u16 DeviceId);
int XTask_reader_kernel_CfgInitialize(XTask_reader_kernel *InstancePtr, XTask_reader_kernel_Config *ConfigPtr);
#else
int XTask_reader_kernel_Initialize(XTask_reader_kernel *InstancePtr, const char* InstanceName);
int XTask_reader_kernel_Release(XTask_reader_kernel *InstancePtr);
#endif

void XTask_reader_kernel_Start(XTask_reader_kernel *InstancePtr);
u32 XTask_reader_kernel_IsDone(XTask_reader_kernel *InstancePtr);
u32 XTask_reader_kernel_IsIdle(XTask_reader_kernel *InstancePtr);
u32 XTask_reader_kernel_IsReady(XTask_reader_kernel *InstancePtr);
void XTask_reader_kernel_Continue(XTask_reader_kernel *InstancePtr);
void XTask_reader_kernel_EnableAutoRestart(XTask_reader_kernel *InstancePtr);
void XTask_reader_kernel_DisableAutoRestart(XTask_reader_kernel *InstancePtr);

void XTask_reader_kernel_Set_worker_id(XTask_reader_kernel *InstancePtr, u32 Data);
u32 XTask_reader_kernel_Get_worker_id(XTask_reader_kernel *InstancePtr);

void XTask_reader_kernel_InterruptGlobalEnable(XTask_reader_kernel *InstancePtr);
void XTask_reader_kernel_InterruptGlobalDisable(XTask_reader_kernel *InstancePtr);
void XTask_reader_kernel_InterruptEnable(XTask_reader_kernel *InstancePtr, u32 Mask);
void XTask_reader_kernel_InterruptDisable(XTask_reader_kernel *InstancePtr, u32 Mask);
void XTask_reader_kernel_InterruptClear(XTask_reader_kernel *InstancePtr, u32 Mask);
u32 XTask_reader_kernel_InterruptGetEnabled(XTask_reader_kernel *InstancePtr);
u32 XTask_reader_kernel_InterruptGetStatus(XTask_reader_kernel *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
