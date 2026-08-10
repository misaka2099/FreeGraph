// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef XPMA_READ_MERGER_KERNEL_H
#define XPMA_READ_MERGER_KERNEL_H

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
#include "xpma_read_merger_kernel_hw.h"

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
} XPma_read_merger_kernel_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XPma_read_merger_kernel;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XPma_read_merger_kernel_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XPma_read_merger_kernel_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XPma_read_merger_kernel_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XPma_read_merger_kernel_ReadReg(BaseAddress, RegOffset) \
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
int XPma_read_merger_kernel_Initialize(XPma_read_merger_kernel *InstancePtr, u16 DeviceId);
XPma_read_merger_kernel_Config* XPma_read_merger_kernel_LookupConfig(u16 DeviceId);
int XPma_read_merger_kernel_CfgInitialize(XPma_read_merger_kernel *InstancePtr, XPma_read_merger_kernel_Config *ConfigPtr);
#else
int XPma_read_merger_kernel_Initialize(XPma_read_merger_kernel *InstancePtr, const char* InstanceName);
int XPma_read_merger_kernel_Release(XPma_read_merger_kernel *InstancePtr);
#endif

void XPma_read_merger_kernel_Start(XPma_read_merger_kernel *InstancePtr);
u32 XPma_read_merger_kernel_IsDone(XPma_read_merger_kernel *InstancePtr);
u32 XPma_read_merger_kernel_IsIdle(XPma_read_merger_kernel *InstancePtr);
u32 XPma_read_merger_kernel_IsReady(XPma_read_merger_kernel *InstancePtr);
void XPma_read_merger_kernel_Continue(XPma_read_merger_kernel *InstancePtr);
void XPma_read_merger_kernel_EnableAutoRestart(XPma_read_merger_kernel *InstancePtr);
void XPma_read_merger_kernel_DisableAutoRestart(XPma_read_merger_kernel *InstancePtr);


void XPma_read_merger_kernel_InterruptGlobalEnable(XPma_read_merger_kernel *InstancePtr);
void XPma_read_merger_kernel_InterruptGlobalDisable(XPma_read_merger_kernel *InstancePtr);
void XPma_read_merger_kernel_InterruptEnable(XPma_read_merger_kernel *InstancePtr, u32 Mask);
void XPma_read_merger_kernel_InterruptDisable(XPma_read_merger_kernel *InstancePtr, u32 Mask);
void XPma_read_merger_kernel_InterruptClear(XPma_read_merger_kernel *InstancePtr, u32 Mask);
u32 XPma_read_merger_kernel_InterruptGetEnabled(XPma_read_merger_kernel *InstancePtr);
u32 XPma_read_merger_kernel_InterruptGetStatus(XPma_read_merger_kernel *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
