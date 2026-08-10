// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef XMERGE_PIPELINE_H
#define XMERGE_PIPELINE_H

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
#include "xmerge_pipeline_hw.h"

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
} XMerge_pipeline_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XMerge_pipeline;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XMerge_pipeline_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XMerge_pipeline_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XMerge_pipeline_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XMerge_pipeline_ReadReg(BaseAddress, RegOffset) \
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
int XMerge_pipeline_Initialize(XMerge_pipeline *InstancePtr, u16 DeviceId);
XMerge_pipeline_Config* XMerge_pipeline_LookupConfig(u16 DeviceId);
int XMerge_pipeline_CfgInitialize(XMerge_pipeline *InstancePtr, XMerge_pipeline_Config *ConfigPtr);
#else
int XMerge_pipeline_Initialize(XMerge_pipeline *InstancePtr, const char* InstanceName);
int XMerge_pipeline_Release(XMerge_pipeline *InstancePtr);
#endif

void XMerge_pipeline_Start(XMerge_pipeline *InstancePtr);
u32 XMerge_pipeline_IsDone(XMerge_pipeline *InstancePtr);
u32 XMerge_pipeline_IsIdle(XMerge_pipeline *InstancePtr);
u32 XMerge_pipeline_IsReady(XMerge_pipeline *InstancePtr);
void XMerge_pipeline_Continue(XMerge_pipeline *InstancePtr);
void XMerge_pipeline_EnableAutoRestart(XMerge_pipeline *InstancePtr);
void XMerge_pipeline_DisableAutoRestart(XMerge_pipeline *InstancePtr);

void XMerge_pipeline_Set_hbm_pma(XMerge_pipeline *InstancePtr, u64 Data);
u64 XMerge_pipeline_Get_hbm_pma(XMerge_pipeline *InstancePtr);
void XMerge_pipeline_Set_hbm_buffer(XMerge_pipeline *InstancePtr, u64 Data);
u64 XMerge_pipeline_Get_hbm_buffer(XMerge_pipeline *InstancePtr);
void XMerge_pipeline_Set_pma_raw_data_start(XMerge_pipeline *InstancePtr, u32 Data);
u32 XMerge_pipeline_Get_pma_raw_data_start(XMerge_pipeline *InstancePtr);
void XMerge_pipeline_Set_pma_vecinfo_start(XMerge_pipeline *InstancePtr, u32 Data);
u32 XMerge_pipeline_Get_pma_vecinfo_start(XMerge_pipeline *InstancePtr);

void XMerge_pipeline_InterruptGlobalEnable(XMerge_pipeline *InstancePtr);
void XMerge_pipeline_InterruptGlobalDisable(XMerge_pipeline *InstancePtr);
void XMerge_pipeline_InterruptEnable(XMerge_pipeline *InstancePtr, u32 Mask);
void XMerge_pipeline_InterruptDisable(XMerge_pipeline *InstancePtr, u32 Mask);
void XMerge_pipeline_InterruptClear(XMerge_pipeline *InstancePtr, u32 Mask);
u32 XMerge_pipeline_InterruptGetEnabled(XMerge_pipeline *InstancePtr);
u32 XMerge_pipeline_InterruptGetStatus(XMerge_pipeline *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
