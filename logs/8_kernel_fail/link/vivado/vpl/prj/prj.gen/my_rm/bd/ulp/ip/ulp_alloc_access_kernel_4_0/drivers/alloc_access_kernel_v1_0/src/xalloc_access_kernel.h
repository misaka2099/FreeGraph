// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef XALLOC_ACCESS_KERNEL_H
#define XALLOC_ACCESS_KERNEL_H

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
#include "xalloc_access_kernel_hw.h"

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
} XAlloc_access_kernel_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XAlloc_access_kernel;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XAlloc_access_kernel_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XAlloc_access_kernel_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XAlloc_access_kernel_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XAlloc_access_kernel_ReadReg(BaseAddress, RegOffset) \
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
int XAlloc_access_kernel_Initialize(XAlloc_access_kernel *InstancePtr, u16 DeviceId);
XAlloc_access_kernel_Config* XAlloc_access_kernel_LookupConfig(u16 DeviceId);
int XAlloc_access_kernel_CfgInitialize(XAlloc_access_kernel *InstancePtr, XAlloc_access_kernel_Config *ConfigPtr);
#else
int XAlloc_access_kernel_Initialize(XAlloc_access_kernel *InstancePtr, const char* InstanceName);
int XAlloc_access_kernel_Release(XAlloc_access_kernel *InstancePtr);
#endif

void XAlloc_access_kernel_Start(XAlloc_access_kernel *InstancePtr);
u32 XAlloc_access_kernel_IsDone(XAlloc_access_kernel *InstancePtr);
u32 XAlloc_access_kernel_IsIdle(XAlloc_access_kernel *InstancePtr);
u32 XAlloc_access_kernel_IsReady(XAlloc_access_kernel *InstancePtr);
void XAlloc_access_kernel_Continue(XAlloc_access_kernel *InstancePtr);
void XAlloc_access_kernel_EnableAutoRestart(XAlloc_access_kernel *InstancePtr);
void XAlloc_access_kernel_DisableAutoRestart(XAlloc_access_kernel *InstancePtr);

void XAlloc_access_kernel_Set_bm_in_hbm(XAlloc_access_kernel *InstancePtr, u64 Data);
u64 XAlloc_access_kernel_Get_bm_in_hbm(XAlloc_access_kernel *InstancePtr);
void XAlloc_access_kernel_Set_init_from_hbm(XAlloc_access_kernel *InstancePtr, u32 Data);
u32 XAlloc_access_kernel_Get_init_from_hbm(XAlloc_access_kernel *InstancePtr);

void XAlloc_access_kernel_InterruptGlobalEnable(XAlloc_access_kernel *InstancePtr);
void XAlloc_access_kernel_InterruptGlobalDisable(XAlloc_access_kernel *InstancePtr);
void XAlloc_access_kernel_InterruptEnable(XAlloc_access_kernel *InstancePtr, u32 Mask);
void XAlloc_access_kernel_InterruptDisable(XAlloc_access_kernel *InstancePtr, u32 Mask);
void XAlloc_access_kernel_InterruptClear(XAlloc_access_kernel *InstancePtr, u32 Mask);
u32 XAlloc_access_kernel_InterruptGetEnabled(XAlloc_access_kernel *InstancePtr);
u32 XAlloc_access_kernel_InterruptGetStatus(XAlloc_access_kernel *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
