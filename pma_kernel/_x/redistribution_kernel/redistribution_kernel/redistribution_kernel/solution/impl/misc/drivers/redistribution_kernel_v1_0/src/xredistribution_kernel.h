// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef XREDISTRIBUTION_KERNEL_H
#define XREDISTRIBUTION_KERNEL_H

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
#include "xredistribution_kernel_hw.h"

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
} XRedistribution_kernel_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XRedistribution_kernel;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XRedistribution_kernel_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XRedistribution_kernel_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XRedistribution_kernel_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XRedistribution_kernel_ReadReg(BaseAddress, RegOffset) \
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
int XRedistribution_kernel_Initialize(XRedistribution_kernel *InstancePtr, u16 DeviceId);
XRedistribution_kernel_Config* XRedistribution_kernel_LookupConfig(u16 DeviceId);
int XRedistribution_kernel_CfgInitialize(XRedistribution_kernel *InstancePtr, XRedistribution_kernel_Config *ConfigPtr);
#else
int XRedistribution_kernel_Initialize(XRedistribution_kernel *InstancePtr, const char* InstanceName);
int XRedistribution_kernel_Release(XRedistribution_kernel *InstancePtr);
#endif

void XRedistribution_kernel_Start(XRedistribution_kernel *InstancePtr);
u32 XRedistribution_kernel_IsDone(XRedistribution_kernel *InstancePtr);
u32 XRedistribution_kernel_IsIdle(XRedistribution_kernel *InstancePtr);
u32 XRedistribution_kernel_IsReady(XRedistribution_kernel *InstancePtr);
void XRedistribution_kernel_Continue(XRedistribution_kernel *InstancePtr);
void XRedistribution_kernel_EnableAutoRestart(XRedistribution_kernel *InstancePtr);
void XRedistribution_kernel_DisableAutoRestart(XRedistribution_kernel *InstancePtr);

void XRedistribution_kernel_Set_hbm_red(XRedistribution_kernel *InstancePtr, u64 Data);
u64 XRedistribution_kernel_Get_hbm_red(XRedistribution_kernel *InstancePtr);
void XRedistribution_kernel_Set_hbm_buffer_for_red(XRedistribution_kernel *InstancePtr, u64 Data);
u64 XRedistribution_kernel_Get_hbm_buffer_for_red(XRedistribution_kernel *InstancePtr);
void XRedistribution_kernel_Set_pma_raw_data_start(XRedistribution_kernel *InstancePtr, u32 Data);
u32 XRedistribution_kernel_Get_pma_raw_data_start(XRedistribution_kernel *InstancePtr);
void XRedistribution_kernel_Set_pma_vecinfo_start(XRedistribution_kernel *InstancePtr, u32 Data);
u32 XRedistribution_kernel_Get_pma_vecinfo_start(XRedistribution_kernel *InstancePtr);
void XRedistribution_kernel_Set_bitmap_offset(XRedistribution_kernel *InstancePtr, u32 Data);
u32 XRedistribution_kernel_Get_bitmap_offset(XRedistribution_kernel *InstancePtr);

void XRedistribution_kernel_InterruptGlobalEnable(XRedistribution_kernel *InstancePtr);
void XRedistribution_kernel_InterruptGlobalDisable(XRedistribution_kernel *InstancePtr);
void XRedistribution_kernel_InterruptEnable(XRedistribution_kernel *InstancePtr, u32 Mask);
void XRedistribution_kernel_InterruptDisable(XRedistribution_kernel *InstancePtr, u32 Mask);
void XRedistribution_kernel_InterruptClear(XRedistribution_kernel *InstancePtr, u32 Mask);
u32 XRedistribution_kernel_InterruptGetEnabled(XRedistribution_kernel *InstancePtr);
u32 XRedistribution_kernel_InterruptGetStatus(XRedistribution_kernel *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
