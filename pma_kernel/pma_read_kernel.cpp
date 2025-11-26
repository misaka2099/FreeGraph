#include "data_type.h"
#include "debug.h"
#include <cstdio>
#include <cstring>
#include <hls_stream.h>
#include <ap_int.h> 
#define NONE 123

void read_edge(
    int src,
    PMA_HEADER &pma_header,
    int header_start,
    BYTE *hbm_ptr,
    hls::stream<ap_axiu<32,0,0,0>> &data_to_nei_query
)
{
    int local_src = src % VERTEX_PER_PMA;
    int start_seg_offset = local_src == 0 ? 0 : pma_header.vertex_range[local_src - 1];
    int src_seg = pma_header.vertex_range[local_src] - start_seg_offset;
    int edge_start = header_start + sizeof(PMA_HEADER) + start_seg_offset * sizeof(int) * SEGMENT_SIZE;
    int edge_len = src_seg * sizeof(int) * SEGMENT_SIZE;
    
    int aligned_len = (edge_len + 63) & ~0x3F;
    // 使用 512bit 行缓冲，避免对小数组的整数组装 memcpy 触发 array transformation 冲突
    // int local_buffer[16];
    // #pragma HLS bind_storage variable=local_buffer type=ram_t2p impl=uram
    for (int i = 0 ; i < aligned_len ; i+= 64)
    {
        #pragma HLS PIPELINE II=1
        ap_uint<512> line = 0;
        memcpy(&line, hbm_ptr + edge_start + i, 64); // 读取 64 字节到本地缓冲区
        for (int j = 0 ; j < 16 ; j++)
        {
            #pragma HLS UNROLL
            int byte_off = i + j * 4;
            if (byte_off >= edge_len)
            {
                break;
            }
            ap_uint<32> w = line.range(32*(j+1)-1, 32*j);
            int neighbor = (int)w;
            if (neighbor == UNUSED_ICON)
            {
                continue;
            }
            data_to_nei_query.write({.data=neighbor});
        }
    }

    data_to_nei_query.write({.data=-1});
}

extern "C"
{
    void pma_read_kernel(
        VEC_INFO* pos_pma_addrs,
        VEC_INFO* neg_pma_addrs,
        BYTE *hbm_ptr,
        hls::stream<READ_COMMAND_STREAM> &cmd_stream,
        hls::stream<ap_axiu<32,0,0,0>> &data_to_nei_query,
        hls::stream<ap_axiu<64,0,0,0>> &data_to_attr_query
        //memory
        // hls::stream<VEC_INFO_STREAM> &vec_info_stream,
        // hls::stream<COMMAND_STREAM> &cmd_to_mem_stream,
        // hls::stream<BYTE_STREAM> &data_from_mem_stream,
        // hls::stream<VEC_INFO_STREAM> &vec_info_from_mem,
        // hls::stream<BYTE_STREAM> &data_to_mem_stream
        // hls::stream<COMMAND_STREAM> &stop_signal
    )
    {
        #pragma HLS INTERFACE m_axi port=pos_pma_addrs offset=slave bundle=gmem0
        #pragma HLS INTERFACE m_axi port=neg_pma_addrs offset=slave bundle=gmem1
        #pragma HLS INTERFACE axis port=cmd_stream
        #pragma HLS INTERFACE axis port=data_to_nei_query
        #pragma HLS INTERFACE axis port=data_to_attr_query
        // #pragma HLS INTERFACE axis port=vec_info_stream
        // #pragma HLS INTERFACE axis port=cmd_to_mem_stream   
        // #pragma HLS INTERFACE axis port=data_to_mem_stream
        // #pragma HLS INTERFACE axis port=data_from_mem_stream
        // #pragma HLS INTERFACE axis port=vec_info_from_mem
        // #pragma HLS INTERFACE axis port=stop_signal
        while (true)
        {
            READ_COMMAND_STREAM cmd = cmd_stream.read();
            // HERE
            if (cmd.data == PMA_STOP && cmd.last == 1) 
            {
                break;
            }
            switch (cmd.data.range(31,0)) 
            {
                case PMA_GET_OUT_EDGE:
                {
                    // HERE
                    int src = cmd.data.range(63,32);
                    int pma_idx = src / VERTEX_PER_PMA;
                    VEC_INFO vec_info;
                    vec_info = pos_pma_addrs[pma_idx];
                    int start_addr = vec_info.range(31,0);
                    int pma_len = vec_info.range(63,32);
                    if (vec_info == 0) // empty PMA
                    {
                        data_to_nei_query.write({.data=-1});
                        break;
                    }
                    PMA_HEADER pma_header;
                    HERE
                    // read header
                    // read_header(
                    //     vec_info.range(31,0),
                    //     vec_info_stream,
                    //     cmd_to_mem_stream,
                    //     data_from_mem_stream,
                    //     pma_header
                    // );
                    // printf("READING OUT EDGE FOR VERTEX %d\n", src);
                    // show_header(pma_header);
                    // HERE
                    // send edge data
                    // read_edge(
                    //     src,
                    //     pma_header,
                    //     vec_info.range(31,0),
                    //     vec_info_stream,
                    //     cmd_to_mem_stream,
                    //     data_from_mem_stream,
                    //     data_to_nei_query
                    // );
                    memcpy(&pma_header, hbm_ptr + start_addr, sizeof(PMA_HEADER));
                    
                    read_edge(
                        src,
                        pma_header,
                        vec_info.range(31,0),
                        hbm_ptr,
                        data_to_nei_query
                    );
                    HERE
                    break;
                }
                case PMA_GET_IN_EDGE:
                {
                    int src = cmd.data.range(63,32);
                    int pma_idx = src / VERTEX_PER_PMA;
                    VEC_INFO vec_info;
                    vec_info = neg_pma_addrs[pma_idx];
                    int start_addr = vec_info.range(31,0);
                    int pma_len = vec_info.range(63,32);
                    if (vec_info == 0) // empty PMA
                    {
                        data_to_nei_query.write({.data=-1});
                        break;
                    }
                    PMA_HEADER pma_header;
                    // read header
                    // read_header(
                    //     vec_info.range(31,0),
                    //     vec_info_stream,
                    //     cmd_to_mem_stream,
                    //     data_from_mem_stream,
                    //     pma_header
                    // );
                    // send edge data
                    // read_edge(
                    //     src,
                    //     pma_header,
                    //     vec_info.range(31,0),
                    //     vec_info_stream,
                    //     cmd_to_mem_stream,
                    //     data_from_mem_stream,
                    //     data_to_nei_query
                    // );
                    memcpy(&pma_header, hbm_ptr + start_addr, sizeof(PMA_HEADER));
                    
                    read_edge(
                        src,
                        pma_header,
                        vec_info.range(31,0),
                        hbm_ptr,
                        data_to_nei_query
                    );
                    break;
                }
                case PMA_GET_ATTR:
                {
                    int vertex = cmd.data.range(63,32);
                    int pma_idx = vertex / VERTEX_PER_PMA;
                    VEC_INFO vec_info;
                    vec_info = pos_pma_addrs[pma_idx];
                    if (vec_info == 0) // empty PMA
                    {
                        data_to_attr_query.write({.data=-1});
                    }
                    PMA_HEADER pma_header;
                    int start_addr = vec_info.range(31,0);
                    // read header
                    // read_header(
                    //     vec_info.range(31,0),
                    //     vec_info_stream,
                    //     cmd_to_mem_stream,
                    //     data_from_mem_stream,
                    //     pma_header
                    // );
                    memcpy(&pma_header, hbm_ptr + start_addr, sizeof(PMA_HEADER));
                    double attr = pma_header.attr[vertex % VERTEX_PER_PMA];
                    data_to_attr_query.write({.data=attr});
                    break;
                }
                // case NONE:
                // {
                //     // vec_info_from_mem.read();
                //     // data_to_mem_stream.write({.data=0});
                //     break;
                // }
            }
        }
    }
}
