// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef XBS_INSERT_KERNEL_H
#define XBS_INSERT_KERNEL_H

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
#include "xbs_insert_kernel_hw.h"

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
} XBs_insert_kernel_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XBs_insert_kernel;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XBs_insert_kernel_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XBs_insert_kernel_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XBs_insert_kernel_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XBs_insert_kernel_ReadReg(BaseAddress, RegOffset) \
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
int XBs_insert_kernel_Initialize(XBs_insert_kernel *InstancePtr, u16 DeviceId);
XBs_insert_kernel_Config* XBs_insert_kernel_LookupConfig(u16 DeviceId);
int XBs_insert_kernel_CfgInitialize(XBs_insert_kernel *InstancePtr, XBs_insert_kernel_Config *ConfigPtr);
#else
int XBs_insert_kernel_Initialize(XBs_insert_kernel *InstancePtr, const char* InstanceName);
int XBs_insert_kernel_Release(XBs_insert_kernel *InstancePtr);
#endif

void XBs_insert_kernel_Start(XBs_insert_kernel *InstancePtr);
u32 XBs_insert_kernel_IsDone(XBs_insert_kernel *InstancePtr);
u32 XBs_insert_kernel_IsIdle(XBs_insert_kernel *InstancePtr);
u32 XBs_insert_kernel_IsReady(XBs_insert_kernel *InstancePtr);
void XBs_insert_kernel_Continue(XBs_insert_kernel *InstancePtr);
void XBs_insert_kernel_EnableAutoRestart(XBs_insert_kernel *InstancePtr);
void XBs_insert_kernel_DisableAutoRestart(XBs_insert_kernel *InstancePtr);

void XBs_insert_kernel_Set_hbm_bs(XBs_insert_kernel *InstancePtr, u64 Data);
u64 XBs_insert_kernel_Get_hbm_bs(XBs_insert_kernel *InstancePtr);
void XBs_insert_kernel_Set_hbm_rw(XBs_insert_kernel *InstancePtr, u64 Data);
u64 XBs_insert_kernel_Get_hbm_rw(XBs_insert_kernel *InstancePtr);
void XBs_insert_kernel_Set_pma_raw_data_start(XBs_insert_kernel *InstancePtr, u32 Data);
u32 XBs_insert_kernel_Get_pma_raw_data_start(XBs_insert_kernel *InstancePtr);
void XBs_insert_kernel_Set_pma_vecinfo_start(XBs_insert_kernel *InstancePtr, u32 Data);
u32 XBs_insert_kernel_Get_pma_vecinfo_start(XBs_insert_kernel *InstancePtr);

void XBs_insert_kernel_InterruptGlobalEnable(XBs_insert_kernel *InstancePtr);
void XBs_insert_kernel_InterruptGlobalDisable(XBs_insert_kernel *InstancePtr);
void XBs_insert_kernel_InterruptEnable(XBs_insert_kernel *InstancePtr, u32 Mask);
void XBs_insert_kernel_InterruptDisable(XBs_insert_kernel *InstancePtr, u32 Mask);
void XBs_insert_kernel_InterruptClear(XBs_insert_kernel *InstancePtr, u32 Mask);
u32 XBs_insert_kernel_InterruptGetEnabled(XBs_insert_kernel *InstancePtr);
u32 XBs_insert_kernel_InterruptGetStatus(XBs_insert_kernel *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
