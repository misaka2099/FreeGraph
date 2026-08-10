// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef XPMA_INSERT_EDGE_STANDALONE_H
#define XPMA_INSERT_EDGE_STANDALONE_H

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
#include "xpma_insert_edge_standalone_hw.h"

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
} XPma_insert_edge_standalone_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XPma_insert_edge_standalone;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XPma_insert_edge_standalone_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XPma_insert_edge_standalone_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XPma_insert_edge_standalone_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XPma_insert_edge_standalone_ReadReg(BaseAddress, RegOffset) \
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
int XPma_insert_edge_standalone_Initialize(XPma_insert_edge_standalone *InstancePtr, u16 DeviceId);
XPma_insert_edge_standalone_Config* XPma_insert_edge_standalone_LookupConfig(u16 DeviceId);
int XPma_insert_edge_standalone_CfgInitialize(XPma_insert_edge_standalone *InstancePtr, XPma_insert_edge_standalone_Config *ConfigPtr);
#else
int XPma_insert_edge_standalone_Initialize(XPma_insert_edge_standalone *InstancePtr, const char* InstanceName);
int XPma_insert_edge_standalone_Release(XPma_insert_edge_standalone *InstancePtr);
#endif

void XPma_insert_edge_standalone_Start(XPma_insert_edge_standalone *InstancePtr);
u32 XPma_insert_edge_standalone_IsDone(XPma_insert_edge_standalone *InstancePtr);
u32 XPma_insert_edge_standalone_IsIdle(XPma_insert_edge_standalone *InstancePtr);
u32 XPma_insert_edge_standalone_IsReady(XPma_insert_edge_standalone *InstancePtr);
void XPma_insert_edge_standalone_Continue(XPma_insert_edge_standalone *InstancePtr);
void XPma_insert_edge_standalone_EnableAutoRestart(XPma_insert_edge_standalone *InstancePtr);
void XPma_insert_edge_standalone_DisableAutoRestart(XPma_insert_edge_standalone *InstancePtr);

void XPma_insert_edge_standalone_Set_hbm_gate(XPma_insert_edge_standalone *InstancePtr, u64 Data);
u64 XPma_insert_edge_standalone_Get_hbm_gate(XPma_insert_edge_standalone *InstancePtr);
void XPma_insert_edge_standalone_Set_hbm_bs(XPma_insert_edge_standalone *InstancePtr, u64 Data);
u64 XPma_insert_edge_standalone_Get_hbm_bs(XPma_insert_edge_standalone *InstancePtr);
void XPma_insert_edge_standalone_Set_hbm_rw(XPma_insert_edge_standalone *InstancePtr, u64 Data);
u64 XPma_insert_edge_standalone_Get_hbm_rw(XPma_insert_edge_standalone *InstancePtr);
void XPma_insert_edge_standalone_Set_hbm_red(XPma_insert_edge_standalone *InstancePtr, u64 Data);
u64 XPma_insert_edge_standalone_Get_hbm_red(XPma_insert_edge_standalone *InstancePtr);
void XPma_insert_edge_standalone_Set_hbm_buffer_for_red(XPma_insert_edge_standalone *InstancePtr, u64 Data);
u64 XPma_insert_edge_standalone_Get_hbm_buffer_for_red(XPma_insert_edge_standalone *InstancePtr);
void XPma_insert_edge_standalone_Set_hbm_merge(XPma_insert_edge_standalone *InstancePtr, u64 Data);
u64 XPma_insert_edge_standalone_Get_hbm_merge(XPma_insert_edge_standalone *InstancePtr);
void XPma_insert_edge_standalone_Set_hbm_merge_buffer(XPma_insert_edge_standalone *InstancePtr, u64 Data);
u64 XPma_insert_edge_standalone_Get_hbm_merge_buffer(XPma_insert_edge_standalone *InstancePtr);
void XPma_insert_edge_standalone_Set_task_buffer(XPma_insert_edge_standalone *InstancePtr, u64 Data);
u64 XPma_insert_edge_standalone_Get_task_buffer(XPma_insert_edge_standalone *InstancePtr);
void XPma_insert_edge_standalone_Set_pma_raw_data_start(XPma_insert_edge_standalone *InstancePtr, u32 Data);
u32 XPma_insert_edge_standalone_Get_pma_raw_data_start(XPma_insert_edge_standalone *InstancePtr);
void XPma_insert_edge_standalone_Set_pma_vecinfo_start(XPma_insert_edge_standalone *InstancePtr, u32 Data);
u32 XPma_insert_edge_standalone_Get_pma_vecinfo_start(XPma_insert_edge_standalone *InstancePtr);
void XPma_insert_edge_standalone_Set_edge_count(XPma_insert_edge_standalone *InstancePtr, u32 Data);
u32 XPma_insert_edge_standalone_Get_edge_count(XPma_insert_edge_standalone *InstancePtr);
void XPma_insert_edge_standalone_Set_worker_id(XPma_insert_edge_standalone *InstancePtr, u32 Data);
u32 XPma_insert_edge_standalone_Get_worker_id(XPma_insert_edge_standalone *InstancePtr);
void XPma_insert_edge_standalone_Set_bitmap_offset(XPma_insert_edge_standalone *InstancePtr, u32 Data);
u32 XPma_insert_edge_standalone_Get_bitmap_offset(XPma_insert_edge_standalone *InstancePtr);

void XPma_insert_edge_standalone_InterruptGlobalEnable(XPma_insert_edge_standalone *InstancePtr);
void XPma_insert_edge_standalone_InterruptGlobalDisable(XPma_insert_edge_standalone *InstancePtr);
void XPma_insert_edge_standalone_InterruptEnable(XPma_insert_edge_standalone *InstancePtr, u32 Mask);
void XPma_insert_edge_standalone_InterruptDisable(XPma_insert_edge_standalone *InstancePtr, u32 Mask);
void XPma_insert_edge_standalone_InterruptClear(XPma_insert_edge_standalone *InstancePtr, u32 Mask);
u32 XPma_insert_edge_standalone_InterruptGetEnabled(XPma_insert_edge_standalone *InstancePtr);
u32 XPma_insert_edge_standalone_InterruptGetStatus(XPma_insert_edge_standalone *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
