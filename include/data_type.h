#pragma once 
#define ATTR_CACHEABLE 

#define AP_INT_MAX_W 2048 
#include "ap_int.h"
#include "ap_axi_sdata.h"
// typedef  ap_uint<8> BYTE_STREAM;
typedef ap_axiu<8,0,0,0> VERTEX_ACK;
typedef ap_axiu<8,0,0,0> WHOLE_ACK;
#define CHANNEL_CAPACITY (248*1024*1024)      // PMA data region, managed by bitmap
#define VEC_INFO_SIZE   (16 * 1024 * 1024)     // vecinfo table at tail of channel
// #define CHANNEL_CAPACITY (64*1024*1024)
#ifndef PMA_PARALLEL_CHANNEL_NUM
#define PMA_PARALLEL_CHANNEL_NUM 2
#endif
#define CHANNEL_NUM PMA_PARALLEL_CHANNEL_NUM
typedef ap_axiu<8,0,0,0> BYTE_STREAM ;
typedef ap_axiu<32,3,0,0> COMMAND_STREAM;
typedef ap_uint<32> COMMAND;
typedef ap_uint<8> BYTE;
// typedef double ATTR_STREAM;
#define PMA_STOP 0
#define PMA_GET_OUT_EDGE 3
#define PMA_GET_IN_EDGE 4 
#define PMA_GET_ALL_OUT_EDGE_COO 17
// #define PMA_GET_ALL_IN_EDGE_COO 18
#define PMA_GET_ALL_OUT_EDGE_CSR 19
// #define PMA_GET_ALL_IN_EDGE_CSR 20
#define PMA_GET_EDGE_COUNT_RANGE 21
#define INIT_VEC_LEN_IN_BYTE 1024
//64 elements per segment, last element (idx 63) = valid_count
#define SEGMENT_SIZE 64
#define EDGE_SLOTS_PER_SEG 63  // SEGMENT_SIZE - 1, valid_count occupies the last slot
#define VALID_COUNT_IDX 63
#define DENSITY_BOUND 0.75
//0-31 start address
//32-63 length
typedef ap_uint<64> VEC_INFO;
typedef ap_axiu<64, 3, 0, 0> VEC_INFO_STREAM;
//0-31: start position
//32-63: length
// typedef ap_uint<64> ACCESS_POS ;
#define VERTEX_PER_PMA 16
#define UNUSED_ICON -1
#define COMMAND_BUFFER_SIZE 8
#define WRITE_KERNEL_BUFFER_SIZE 64
#define STOP_ICON 
typedef ap_axiu<72, 0, 0, 0> TAG_STREAM; // 64 VECINFO + 8tag 
#define READER_CHUNK_BYTES 512
// (0,31) src (32,63) dst or (32, 95) attr (96,97) type(insert vertex, insert edge, change attr) (98)direction()
// typedef ap_axiu<128, 0, 0, 0> PMA_UPDATE_MESSAGE; 

enum PMA_UPDATE_OPCODE
{
    PMA_UPDATE_INSERT_EDGE = 0,
    PMA_UPDATE_DELETE_EDGE = 1,
    PMA_UPDATE_INSERT_VERTEX = 2,
    PMA_UPDATE_DELETE_VERTEX = 3,
    PMA_UPDATE_STOP = 4,
    PMA_UPDATE_FLUSH_CACHE = 5,
};

enum CONTROL_TYPE
{
    normal_flag = 0,
    exit_flag = 1,
    clear_probe = 2,
    synchronize_flag = 3,
    flush_flag = 4,         // per-batch drain + ACK, workers stay alive
    locked_flag = 5,        // PMA locked by rebalance, edge needs reprocess
    empty_pma_flag = 6      // PMA not yet allocated, needs allocation first
};
struct PMA_UPDATE_MESSAGE
{
    int src;
    int dst;
    PMA_UPDATE_OPCODE opcode;
    CONTROL_TYPE flags;
};
#define MAX_PMA_NUM 1000000

// ============================================================
// 3-Kernel Insert Architecture Types
// ============================================================

// Segment: 16 ints = 64B = one HBM burst, all 16 slots hold edges (UNUSED_ICON for empty)
#define SEG_INT_SIZE 16
#define SEG_BYTES (SEG_INT_SIZE * (int)sizeof(int))  // 64
#define HEADER_WORDS 2
// bs_kernel → insert_kernel: one message per edge, carries search result
struct BS_OUT_MSG {
    int seg_idx;   // target segment index (found by bs search)
    int src;       // source vertex
    int dst;       // destination vertex
    PMA_UPDATE_OPCODE opcode;    // PMA_UPDATE_OPCODE
    CONTROL_TYPE flags;     // bit0: reprocess, bit1: last_in_batch
};

// insert_kernel → rebalance_kernel
struct REBALANCE_REQ {
    int src;    // src to rebalance
    int seg_idx;  // seg id in vertex src
    VEC_INFO old_addr;   // vecinfo for the vertex, used for rebalance decision
    CONTROL_TYPE flags;      // bit0: last (stop)
};

// insert_kernel → router: reprocessed messages go back for re-dispatch

// bs_helper cache entry per PMA
struct BS_CACHE_ENTRY {
    int pma_idx;
    int vertex_range[VERTEX_PER_PMA];  // cached range for invalidation check
    int bs_data[10240];                // per-segment first-edge values
    bool valid;
};


// #define OUT_DIRECTION 0
// #define IN_DIRECTION 1
typedef ap_axiu<96,0,0,0> READ_COMMAND_STREAM;
typedef ap_axiu<64,0,0,0> COO_STREAM;
typedef ap_axiu<32,0,0,0> EDGE_CNT_STREAM;
typedef  ap_axiu<8,0,0,0> CLEAR_ICON;



#define VERTEX_ATTR_SIZE_IN_BYTE 1
struct alignas(64)PMA_HEADER
{
    int vertex_range[VERTEX_PER_PMA];//every vertex in PMA's end segment [)
    int edge_count[VERTEX_PER_PMA];//every vertex in PMA's edge count
    // ap_uint<64> attr[VERTEX_PER_PMA];//every vertex in PMA's attr
};
typedef ap_uint<512> memory_chunk; 



#define MAX_COMMAND_BUFFER_LENGTH 16
#define MAX_COMMAND_BUFFER_NUM 128
#define MAX_CMD_NUM 16

// --- Pyramid Cache Configuration ---
#define LARGE_LINES   2
#define MEDIUM_LINES  4
#define SMALL_LINES   16
#define LARGE_SIZE    8192
#define MEDIUM_SIZE   4096
#define SMALL_SIZE    1024
#define TOTAL_CACHE_LINES (LARGE_LINES + MEDIUM_LINES + SMALL_LINES)  // 22
#define PHYS_CACHE_LINES 8

#define LARGE_START   0
#define LARGE_END     LARGE_LINES
#define MEDIUM_START  LARGE_LINES
#define MEDIUM_END    (LARGE_LINES + MEDIUM_LINES)
#define SMALL_START   (LARGE_LINES + MEDIUM_LINES)
#define SMALL_END     TOTAL_CACHE_LINES

// Profile table
// Router / Worker config
#define ROUTER_WORKERS          1

// #define LOAD_BITMAP 19
enum control_type
{
    INSERT_STOP = 0,
    INSERT_FLUSH_CACHE = 1,
    INSERT_GOING = 2
};
struct pma_idx_info
{
    int pma_idx;
    control_type command_type;
    int last;
};
//range(31,0) start
//range(63,32) len
//range(64): alloc or free 0->alloc 1->free
typedef ap_axiu<96,32,0,0> MEMORY_ALLOC_FREE_REQUEST;

typedef ap_uint<72> MEMORY_ALLOC_FREE_REQUEST_INTERNAL;


enum PMA_INFO_ACCESS_TYPE 
{
    PMA_HEADER_READ = 0,
    PMA_HEADER_WRITE = 1,
    PMA_SEGMENT_READ = 2,
    PMA_SEGMENT_WRITE = 3,
    PMA_FINISH = 4,
    PMA_EXTEND = 5,
    PMA_BS_HELPER_READ = 6,
    PMA_HEADER_READ_HINTED = 7,  // PMA_HEADER_READ + edge_count hint in seg_start
    PMA_FLUSH = 8,
    PMA_VALID_COUNT_READ = 9     // return sum of valid_count over seg_start..seg_start+seg_len
};
struct PMA_INFO_ACCESS_COMMAND
{
    PMA_INFO_ACCESS_TYPE command_type;
    int pma_idx;
    int seg_start;
    int seg_len;
};

enum MEMORY_INFO_ACCESS_TYPE
{
    MEMORY_READ = 0,
    MEMORY_WRITE = 1,
    VEC_INFO_READ = 2,
    VEC_INFO_WRITE = 3,
    MEMORY_FINISH = 4,
    VEC_INFO_ALLOC = 5,
    VEC_INFO_FREE = 6,
    ALLOC_FINISH = 7,
    PMA_IDX_MAX_QUERY = 8
    // HEADER_WRITE = 4,
    // HEADER_READ = 5,
    // SEG_WRITE = 6,
    // SEG_READ = 7
};
struct MEMORY_INFO_ACCESS_COMMAND
{
    MEMORY_INFO_ACCESS_TYPE command_type;
    // int src;
    
    int pma_idx;
    int start;
    int len;
    int last;
};

// enum memory_access_type
// {
//     READ,
//     WRITE,
//     FINISH
// };

// struct memory_access_command
// {
//     int pma_idx;
//     int start;
//     int len;
//     MEMORY_INFO_ACCESS_TYPE command_type;
//     memory_access_type type;
// };
#define REQ_TYPE_ALLOC 0
#define REQ_TYPE_FREE 1
#define REQ_TYPE_FINISH 2
#define REQ_TYPE_LOAD_BITMAP 3
#define NUM_MEM_PARTITIONS 4
#define CHANNELS_PER_PARTITION 1

// SEARCH_RESULT_STREAM flags
#define SEARCH_FLAG_EXISTS 1
#define SEARCH_FLAG_FOUND 2


// // Little kernel setup 
// const int LITTLE_KERNEL_NUM=1;
// const int LITTLE_KERNEL_DST_BUFFER_SIZE=65536;
// //Big kernel setup 
// const int BIG_KERNEL_NUM=1;
// const int BIG_KERNEL_DST_BUFFER_SIZE=524288;

// //the size of destination buffer; patition size should be aligned with 8K size to improve the utilization of URAMs.
// const int PARTITION_SIZE=LITTLE_KERNEL_DST_BUFFER_SIZE;
// //the size of one ping or pong buffer
// const int SRC_BUFFER_SIZE=4096; //4096 vertices 16*2 KB per source vertex buffer for a scatter PE// 64 BRAMs per scatter PE
// const int LOG2_SRC_BUFFER_SIZE=12; // #4096 vertices 16*2 KB per source vertex buffer for a scatter PE// 64 BRAMs per scatter PE

#define PMA_ENLARGE_FACTOR 3