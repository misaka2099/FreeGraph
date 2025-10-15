#pragma once  
#include "ap_int.h"
#include "ap_axi_sdata.h"
// typedef  ap_uint<8> BYTE_STREAM;
#define CHANNEL_CAPACITY (64*1024*1024)
#define CHANNEL_NUM 2
typedef ap_axiu<8,0,0,0> BYTE_STREAM ;
typedef ap_axiu<32,0,0,0> COMMAND_STREAM;
typedef ap_uint<32> COMMAND;
typedef ap_uint<8> BYTE;
// typedef double ATTR_STREAM;
#define PMA_STOP 13
#define PMA_INSERT_EDGE 1 
#define PMA_INSERT_VERTEX 2 
#define PMA_GET_OUT_EDGE 3
#define PMA_GET_IN_EDGE 4 
#define PMA_GET_ATTR 5 
#define PMA_CHANGE_ATTR 6 
#define MEM_WRITE 7
#define MEM_READ 8 
#define MEM_STOP 9
#define MEM_ALLOC 10
#define MEM_FREE 11
#define MEM_COPY 12
#define INIT_VEC_LEN_IN_BYTE 1024
//64 elements
#define SEGMENT_SIZE 64 
#define DENSITY_BOUND 0.75
//0-31 start address
//32-63 length
typedef ap_uint<64> VEC_INFO;
typedef ap_axiu<64, 0, 0, 0> VEC_INFO_STREAM;
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
typedef ap_axiu<67, 0, 0, 0> PMA_UPDATE_MESSAGE; // (0,31) src (32,63) dst (64,65) type(insert vertex, insert edge, change attr) (66)direction()
#define MAX_PMA_NUM 2048

struct PMA_HEADER
{
    int vertex_range[VERTEX_PER_PMA];//every vertex in PMA's end segment [)
    int edge_count[VERTEX_PER_PMA];//every vertex in PMA's edge count
    double attr[VERTEX_PER_PMA];//every vertex in PMA's attr
};