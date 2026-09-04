#pragma once

#include "data_type.h"
#include <hls_stream.h>
#include <ap_int.h>

struct inserted_seg
{
    int old_vertex_relative_seg_number[16];
    int vertex_number[16];
};


void read_seg(
    // ap_uint<512>* hbm_ptr,
    hls::stream<memory_access_command> &memory_access_stream,
    hls::stream<ap_uint<512>> &data_from_mem,
    int segment_buffer[SEGMENT_SIZE],
    int pma_idx,
    int seg_number
)
{
    #pragma HLS INLINE
    // ap_uint<512>* segment_buffer_ptr = (ap_uint<512>*)(segment_buffer);
    ap_uint<512> buffer[4];
    // const int seg_addr_in_ap_512_pattern = (seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr) / 64;
    
    // const int seg_addr_in_ap_512_pattern = (seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr) >> 6; 
    memory_access_command command;
    command.pma_idx = pma_idx;
    command.start = seg_number;
    command.len = 1;
    command.command_type = SEG_READ;
    command.type = READ;
    // for (int i = 0 ; i < 4 ; i++)
    // {
    //     #pragma HLS PIPELINE II=1 
    //     // segment_buffer_ptr[i] 
    //     // = hbm_ptr[i + (seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr) / 64];
    //     buffer[i] = hbm_ptr[i + seg_addr_in_ap_512_pattern];
    // }
    memory_access_stream.write(command);
    #pragma HLS protocol fixed
    for (int i = 0 ; i < 4 ; i++)
    {
        #pragma HLS PIPELINE II=1
        buffer[i] = data_from_mem.read();
    }
    for (int i = 0; i < 4; i++) {
        #pragma HLS UNROLL
        for (int j = 0; j < 16; j++) {
            #pragma HLS UNROLL
            segment_buffer[i * 16 + j] = buffer[i].range(32 * (j + 1) - 1, 32 * j);
        }
    }    
    // int seg_byte_addr = pma_data_base_addr + seg_number * SEGMENT_SIZE * sizeof(int);
    // 计算段的起始索引（单位：ap_uint<512>块）
    // int seg_idx = seg_byte_addr / 64;
    // 直接memcpy
    // memcpy(segment_buffer, hbm_ptr + seg_byte_addr, SEGMENT_SIZE * sizeof(int));
}
void write_seg(
    // ap_uint<512>* hbm_ptr,
    hls::stream<memory_access_command> &memory_access_stream,
    hls::stream<ap_uint<512>> &data_to_mem,
    hls::stream<ap_uint<512>> &data_from_mem,
    int segment_buffer[SEGMENT_SIZE],
    int pma_idx,
    int seg_number
)
{
    #pragma HLS INLINE
    ap_uint<512> buffer[4];
    memory_access_command command;
    command.pma_idx = pma_idx;
    command.start = seg_number;
    command.len = 1;
    command.command_type = SEG_WRITE;
    command.type = WRITE;
    memory_access_stream.write(command);
    // ap_uint<512>* segment_buffer_ptr = (ap_uint<512>*)(segment_buffer);
    // ap_uint<512> buffer;
    // const int seg_addr_in_ap_512_pattern = (seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr) >> 6;
    for (int i = 0; i < 4; i++) {
        #pragma HLS UNROLL
        buffer[i] = 0;
        for (int j = 0; j < 16; j++) {
            #pragma HLS UNROLL
            buffer[i].range(32 * (j + 1) - 1, 32 * j) = segment_buffer[i * 16 + j];
        }
    }
    
    for (int i = 0 ; i < 4 ; i++)
    {
        #pragma HLS PIPELINE II=1
        data_to_mem.write(buffer[i]);
    }
    #pragma HLS protocol fixed
    // ap_uint<512> ack = data_from_mem.read();
    // const int seg_addr_in_ap_512_pattern = (seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr) >> 6;
    // for (int i = 0; i < 4; i++) {
    //     #pragma HLS PIPELINE II=1
    //     hbm_ptr[i + seg_addr_in_ap_512_pattern] = buffer[i];
    // }
    // for (int i = 0 ; i < SEGMENT_SIZE * sizeof(int) / 64 ; i++)
    // {
    //     #pragma HLS PIPELINE II=1 
    //     for (int j = 0 ; j < 16 ; j++)
    //     {
    //         #pragma HLS UNROLL
    //         buffer.range(32*(j+1) - 1, 32*j) = segment_buffer[i*16+j];
    //     }
    //     hbm_ptr[i + seg_addr_in_ap_512_pattern] = buffer;
        
    //     // hbm_ptr[i + (seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr) / 64] 
    //     // = segment_buffer_ptr[i] ;

    // }
    // int seg_byte_addr = pma_data_base_addr + seg_number * SEGMENT_SIZE * sizeof(int);
    // 计算段的起始索引（单位：ap_uint<512>块）
    // int seg_idx = seg_byte_addr / 64;
    // 直接memcpy
    // memcpy(hbm_ptr + seg_byte_addr, segment_buffer, SEGMENT_SIZE * sizeof(int));
}
void read_header(
    // ap_uint<512>* hbm_ptr,
    
    hls::stream<memory_access_command> &memory_access_stream,
    hls::stream<ap_uint<512>> &data_from_mem,
    PMA_HEADER& pma_header,
    int pma_idx
)
{
    #pragma HLS INLINE
    ap_uint<512> buffer[4];
    // for (int i = 0; i < 4; i++) {
    //     #pragma HLS PIPELINE II=1
    //     buffer[i] = hbm_ptr[pma_header_addr_in_512_patten + i];
    // }
    memory_access_command command;
    command.pma_idx = pma_idx;
    command.start = 0;
    command.len = 0;
    command.command_type = HEADER_READ;
    command.type = READ;
    memory_access_stream.write(command);
    #pragma HLS protocol fixed
    for (int i = 0 ; i < 4 ; i++)
    {
        #pragma HLS PIPELINE II=1
        buffer[i] = data_from_mem.read();
    }

    for (int j = 0; j < 16; j++) 
    {
        #pragma HLS UNROLL
        int val = buffer[0].range(32 * (j + 1) - 1, 32 * j);
        pma_header.vertex_range[j] = val;
    }
    for (int j = 0 ; j < 16 ; j++)
    {
        #pragma HLS UNROLL 
        int val = buffer[1].range(32 * (j + 1) - 1, 32 * j);
        pma_header.edge_count[j] = val;
    }

    // attr is not read 
}
void write_header(
    // ap_uint<512>* hbm_ptr,
    hls::stream<memory_access_command> &memory_access_stream,
    hls::stream<ap_uint<512>> &data_to_mem,
    hls::stream<ap_uint<512>> &data_from_mem,
    PMA_HEADER& pma_header,
    int pma_idx
)
{
    #pragma HLS INLINE
    ap_uint<512> buffer[4];
    memory_access_command command;
    command.pma_idx = pma_idx;
    command.start = 0;
    command.len = 0;
    command.command_type = HEADER_WRITE;
    command.type = WRITE;
    memory_access_stream.write(command);
    // 打包 vertex_range 到 buffer[0]
    for (int j = 0; j < 16; j++) {
        #pragma HLS UNROLL
        buffer[0].range(32 * (j + 1) - 1, 32 * j) = pma_header.vertex_range[j];
    }
    // 打包 edge_count 到 buffer[1]
    for (int j = 0; j < 16; j++) {
        #pragma HLS UNROLL
        buffer[1].range(32 * (j + 1) - 1, 32 * j) = pma_header.edge_count[j];
    }
    // 打包 attr 到 buffer[2] 和 buffer[3]
    for (int j = 0; j < 8; j++) {
        #pragma HLS UNROLL
        ap_uint<64> d = *reinterpret_cast<ap_uint<64>*>(&pma_header.attr[j]);
        buffer[2].range(64 * (j + 1) - 1, 64 * j) = d;
        d = *reinterpret_cast<ap_uint<64>*>(&pma_header.attr[j + 8]);
        buffer[3].range(64 * (j + 1) - 1, 64 * j) = d;
    }
    // for (int i = 0; i < 4; i++) {
    //     #pragma HLS PIPELINE II=1
    //     hbm_ptr[pma_header_addr_in_512_patten + i] = buffer[i];
    // }
    for (int i = 0 ; i < 4 ; i++)
    {
        data_to_mem.write(buffer[i]);
    }
    #pragma HLS protocol fixed
    // ap_uint<512> ack = data_from_mem.read();
}


static inline void delete_vertex_segments(
    hls::stream<memory_access_command> &memory_access_stream,
    hls::stream<ap_uint<512>> &data_to_mem,
    hls::stream<ap_uint<512>> &data_from_mem,
    int segment_buffer[SEGMENT_SIZE],
    int pma_data_base_addr,
    int low_seg,
    int high_seg)
{
    #pragma HLS INLINE
    for (int k = 0; k < SEGMENT_SIZE; k++)
    {
        segment_buffer[k] = UNUSED_ICON;
    }

    for (int s = low_seg; s <= high_seg; s++)
    {
        write_seg(memory_access_stream, data_to_mem, data_from_mem, segment_buffer, pma_data_base_addr, s);
    }
}

static inline void delete_edge_in_segment(int segment_buffer[SEGMENT_SIZE], int offset)
{
    #pragma HLS INLINE
    for (int k = offset; k < SEGMENT_SIZE - 1; k++)
    {
        #pragma HLS UNROLL
        segment_buffer[k] = segment_buffer[k + 1];
    }
    segment_buffer[SEGMENT_SIZE - 1] = UNUSED_ICON;
}

static inline void insert_edge_in_segment(int segment_buffer[SEGMENT_SIZE], int offset, int dst)
{
    #pragma HLS INLINE
    for (int k = SEGMENT_SIZE - 1; k > offset; k--)
    {
        #pragma HLS UNROLL
        segment_buffer[k] = segment_buffer[k - 1];
    }
    segment_buffer[offset] = dst;
}
