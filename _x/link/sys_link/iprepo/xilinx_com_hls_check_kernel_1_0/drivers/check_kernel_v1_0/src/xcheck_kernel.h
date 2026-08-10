// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.2 (64-bit)
// Tool Version Limit: 2019.12
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// ==============================================================
#ifndef XCHECK_KERNEL_H
#define XCHECK_KERNEL_H

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
#include "xcheck_kernel_hw.h"

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
} XCheck_kernel_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XCheck_kernel;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XCheck_kernel_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XCheck_kernel_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XCheck_kernel_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XCheck_kernel_ReadReg(BaseAddress, RegOffset) \
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
int XCheck_kernel_Initialize(XCheck_kernel *InstancePtr, u16 DeviceId);
XCheck_kernel_Config* XCheck_kernel_LookupConfig(u16 DeviceId);
int XCheck_kernel_CfgInitialize(XCheck_kernel *InstancePtr, XCheck_kernel_Config *ConfigPtr);
#else
int XCheck_kernel_Initialize(XCheck_kernel *InstancePtr, const char* InstanceName);
int XCheck_kernel_Release(XCheck_kernel *InstancePtr);
#endif

void XCheck_kernel_Start(XCheck_kernel *InstancePtr);
u32 XCheck_kernel_IsDone(XCheck_kernel *InstancePtr);
u32 XCheck_kernel_IsIdle(XCheck_kernel *InstancePtr);
u32 XCheck_kernel_IsReady(XCheck_kernel *InstancePtr);
void XCheck_kernel_Continue(XCheck_kernel *InstancePtr);
void XCheck_kernel_EnableAutoRestart(XCheck_kernel *InstancePtr);
void XCheck_kernel_DisableAutoRestart(XCheck_kernel *InstancePtr);

void XCheck_kernel_Set_coo_src(XCheck_kernel *InstancePtr, u64 Data);
u64 XCheck_kernel_Get_coo_src(XCheck_kernel *InstancePtr);
void XCheck_kernel_Set_coo_dst(XCheck_kernel *InstancePtr, u64 Data);
u64 XCheck_kernel_Get_coo_dst(XCheck_kernel *InstancePtr);
void XCheck_kernel_Set_num_edges(XCheck_kernel *InstancePtr, u64 Data);
u64 XCheck_kernel_Get_num_edges(XCheck_kernel *InstancePtr);
void XCheck_kernel_Set_vertex_to_check(XCheck_kernel *InstancePtr, u32 Data);
u32 XCheck_kernel_Get_vertex_to_check(XCheck_kernel *InstancePtr);
void XCheck_kernel_Set_mode(XCheck_kernel *InstancePtr, u32 Data);
u32 XCheck_kernel_Get_mode(XCheck_kernel *InstancePtr);
void XCheck_kernel_Set_last(XCheck_kernel *InstancePtr, u32 Data);
u32 XCheck_kernel_Get_last(XCheck_kernel *InstancePtr);

void XCheck_kernel_InterruptGlobalEnable(XCheck_kernel *InstancePtr);
void XCheck_kernel_InterruptGlobalDisable(XCheck_kernel *InstancePtr);
void XCheck_kernel_InterruptEnable(XCheck_kernel *InstancePtr, u32 Mask);
void XCheck_kernel_InterruptDisable(XCheck_kernel *InstancePtr, u32 Mask);
void XCheck_kernel_InterruptClear(XCheck_kernel *InstancePtr, u32 Mask);
u32 XCheck_kernel_InterruptGetEnabled(XCheck_kernel *InstancePtr);
u32 XCheck_kernel_InterruptGetStatus(XCheck_kernel *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
