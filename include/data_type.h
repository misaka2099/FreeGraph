#pragma once 
#define ATTR_CACHEABLE 

#define AP_INT_MAX_W 2048 
#include "ap_int.h"
#include "ap_axi_sdata.h"
// typedef  ap_uint<8> BYTE_STREAM;
typedef ap_axiu<8,0,0,0> VERTEX_ACK;
typedef ap_axiu<8,0,0,0> WHOLE_ACK;
#define CHANNEL_CAPACITY (250*1024*1024)
// #define CHANNEL_CAPACITY (64*1024*1024)
#ifndef PMA_PARALLEL_CHANNEL_NUM
#define PMA_PARALLEL_CHANNEL_NUM 3
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

// Allocator bitmap: shared between alloc_access_kernel and pma_insert_edge
// TOTAL_BLOCKS = CHANNEL_CAPACITY / INIT_VEC_LEN_IN_BYTE = 250*1024*1024 / 1024 = 256000
// BITMAP_WORDS = ceil(TOTAL_BLOCKS / BITMAP_STORAGE_WORD_BITS)
#define ALLOC_BITMAP_STORAGE_WORD_BITS 512
#define ALLOC_BITMAP_TOTAL_BLOCKS (CHANNEL_CAPACITY / INIT_VEC_LEN_IN_BYTE)
#define ALLOC_BITMAP_WORDS ((ALLOC_BITMAP_TOTAL_BLOCKS + ALLOC_BITMAP_STORAGE_WORD_BITS - 1) / ALLOC_BITMAP_STORAGE_WORD_BITS)
//64 elements per segment, last element (idx 63) = valid_count
#define SEGMENT_SIZE 64
#define EDGE_SLOTS_PER_SEG 63  // SEGMENT_SIZE - 1, valid_count occupies the last slot
#define VALID_COUNT_IDX 63
#define DENSITY_BOUND 0.75
//0-31 start address
//32-63 length
typedef ap_uint<64> VEC_INFO;
typedef ap_axiu<64, 3, 0, 0> VEC_INFO_STREAM;
typedef ap_axiu<256, 0, 0, 0> SEARCH_RESULT_STREAM;
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
    PMA_UPDATE_GAS_QUERY = 6,     // GAS vertex query: src = vertex to query
};
struct PMA_UPDATE_MESSAGE
{
    int src;
    int dst;
    PMA_UPDATE_OPCODE opcode;
    int last;
};
#define MAX_PMA_NUM 1000000

// #define OUT_DIRECTION 0
// #define IN_DIRECTION 1
// 读命令格式 (5×32 = 160 位):
//   [31:0]    opcode
//   [63:32]   src_start    (含)
//   [95:64]   src_end      (不含)
//   [127:96]  dst_start    (含)
//   [159:128] dst_end      (不含)
// 范围语义: -1 = 该维度不限 (全遍历)。COO 指令两维度都 -1 = 全图遍历;
// src 为 -1 仅 dst 限定时 = 遍历所有 src 顶点、只输出 dst 在范围内的边。
typedef ap_axiu<160,0,0,0> READ_COMMAND_STREAM;
typedef  ap_axiu<8,0,0,0> CLEAR_ICON;



#define VERTEX_ATTR_SIZE_IN_BYTE 16
typedef ap_uint<64> VERTEX_ATTR_TYPE; 
struct VERTEX_ATTR_STREAM_TYPE {
    VERTEX_ATTR_TYPE data;
    ap_uint<1> last;
};
#ifdef ATTR_CACHEABLE
struct alignas(64)PMA_HEADER
{
    int vertex_range[VERTEX_PER_PMA];//every vertex in PMA's end segment [)
    int edge_count[VERTEX_PER_PMA];//every vertex in PMA's edge count
    VERTEX_ATTR_TYPE attr[VERTEX_PER_PMA];//every vertex in PMA's attr
};
#else
struct alignas(64)PMA_HEADER
{
    int vertex_range[VERTEX_PER_PMA];//every vertex in PMA's end segment [)
    int edge_count[VERTEX_PER_PMA];//every vertex in PMA's edge count
    ap_uint<64> attr[VERTEX_PER_PMA];//every vertex in PMA's attr
};
#endif
typedef ap_uint<512> memory_chunk; 

struct INTRA_PMA_UPDATE_MESSAGE
{
    ap_uint<128> data;
    ap_uint<1> last;
};



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
#define ROUTER_WORKERS          CHANNEL_NUM

// #define LOAD_BITMAP 19
// enum control_type
// {
//     INSERT_STOP = 0,
//     INSERT_FLUSH_CACHE = 1,
//     INSERT_GOING = 2
// };
// struct pma_idx_info
// {
//     int pma_idx;
//     control_type command_type;
//     int boff;      // PMA byte offset relative to data_off (from pma_gate, no VEC_INFO re-read)
//     int last;
// };
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

#define PMA_ENLARGE_FACTOR 4

// --- Helper Array Layout (per PMA, after header) ---
// SoA layout: FE area + VC area
//   first_edge area: 1 entry per segment (= first_edge, 32-bit)
//                    16 entries per 512-bit word
//   valid_count area: 1 entry per segment (= valid_count, 32-bit)
//                     16 entries per 512-bit word
static const int FE_SOA_ENTRIES_PER_WORD = 16;
static const int VC_SOA_ENTRIES_PER_WORD = 16;

// Deprecated AoS constants kept for reference:
#define HELPER_ENTRIES_PER_WORD 8   // old AoS: 8 × 64-bit entries per word
#define HELPER_FIRST_MASK  0x00000000FFFFFFFFUL
#define HELPER_VALID_SHIFT 32
#define HELPER_WORDS(total_segs) (((total_segs) + HELPER_ENTRIES_PER_WORD - 1) / HELPER_ENTRIES_PER_WORD)

// SoA helper word counts
static inline int fe_words_for(int segs) {
    return (segs + FE_SOA_ENTRIES_PER_WORD - 1) / FE_SOA_ENTRIES_PER_WORD;
}
static inline int vc_words_for(int segs) {
    return (segs + VC_SOA_ENTRIES_PER_WORD - 1) / VC_SOA_ENTRIES_PER_WORD;
}
static inline int hlp_total_words(int segs) { return fe_words_for(segs) + vc_words_for(segs); }

// Total PMA words: header + helper (FE+VC) + segments
static inline int pma_total_words_for(int segs) { return 4 + hlp_total_words(segs) + segs * 4; }

// --- Three-Phase Pipeline Types ---

// process_cmd → pma_gate: single batch stream (replaces per-worker distribution)
struct cmd_batch_full_t {
    int pma_idx;
    // control_type command_type;
    int cmd_count;
    PMA_UPDATE_MESSAGE cmds[MAX_CMD_NUM];
};

// Phase 1→2: Binary search result for one batch (up to 16 edges)
// Forwarded header from pma_gate avoids redundant HBM reads in insert_phase
struct bs_batch_t {
    int pma_idx;
    int start_byte;              // forwarded: PMA absolute HBM byte offset
    ap_uint<512> range_word;     // forwarded: header[0] — vertex ranges + total_segs
    ap_uint<512> edge_count_word;// forwarded: header[1] — per-vertex edge counts
    int target_seg[16];          // absolute segment index for each op
    int dst[16];                 // destination vertex
    int local_src[16];           // source vertex within PMA (0..15)
    ap_uint<16> op_mask;         // bit i: 0=insert_edge, 1=delete_edge
    int count;                   // number of ops (1..16)
};

// Phase 2→3: Modification summary for rebalance decision
struct insert_summary_t {
    int pma_idx;
    int edge_count_delta[16];      // net +inserts -deletes per vertex
    ap_uint<16> inserted_vertex_mask; // which vertices had insert operations
    bool any_modification;
};

// Header / segment word & byte counts (shared across bs_insert & redistribution kernels)
#define HEADER_WORDS ((int)(sizeof(PMA_HEADER) / 64))
#define SEG_WORDS_PER_SEG (SEGMENT_SIZE * (int)sizeof(int) / 64)
#define HEADER_BYTES (HEADER_WORDS * 64)
#define SEG_BYTES (SEG_WORDS_PER_SEG * 64)

// Pre-expand command: task_reader → redistribution (per-PMA growth request).
// Sent BEFORE a PMA's edge segment is flushed; redistribution grows the PMA
// (INIT allocate / ENLARGE) so inserts fit without runtime ENLARGE.
struct pre_expand_cmd_t {
    int pma;                     // PMA index (global)
    int count[VERTEX_PER_PMA];   // edges per local vertex for this PMA this wave
};

// Rebalance task: bs_insert → redistribution (overflowed segment / ACK / STOP).
// Carries forwarded header to avoid redundant VEC_INFO/header HBM reads.
// payload: seg (>=0) overflow segment, -1 = STOP, -2 = ACK marker.
struct rebalance_task_t {
    int pma;                     // PMA index
    int payload;                 // segment index (>=0), -1=STOP, -2=ACK marker
    int start_byte;              // forwarded: PMA absolute HBM byte offset
    ap_uint<512> range_word;     // forwarded: header[0] — vertex ranges + total_segs
};