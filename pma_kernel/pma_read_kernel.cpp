#include "data_type.h"
#include "debug.h"
#include <cstdio>
#include <hls_stream.h>
#include <ap_int.h> 
#define NONE 123

void read_header(
    int header_start,
    hls::stream<VEC_INFO_STREAM> &vec_info_stream,
    hls::stream<COMMAND_STREAM> &cmd_to_mem_stream,
    hls::stream<BYTE_STREAM> &data_from_mem_stream,
    PMA_HEADER &pma_header
)
{
    VEC_INFO_STREAM header_vec_info;
    cmd_to_mem_stream.write({.data=MEM_READ});
    header_vec_info.data.range(31,0) = header_start;
    header_vec_info.data.range(63,32) = sizeof(PMA_HEADER);
    vec_info_stream.write(header_vec_info);
    for (int i = 0 ; i < sizeof(PMA_HEADER); i++)
    {
        // printf("ITERATION %d\n", i);
        BYTE_STREAM byte;
        data_from_mem_stream.read(byte);
        ((BYTE*)&pma_header)[i] = byte.data;
    }
}
void read_edge(
    int src,
    PMA_HEADER &pma_header,
    int header_start,
    hls::stream<VEC_INFO_STREAM> &vec_info_stream,
    hls::stream<COMMAND_STREAM> &cmd_to_mem_stream,
    hls::stream<BYTE_STREAM> &data_from_mem_stream,
    hls::stream<ap_axiu<32,0,0,0>> &data_to_nei_query
)
{
    int local_src = src % VERTEX_PER_PMA;
    int start_seg_offset = local_src == 0 ? 0 : pma_header.vertex_range[local_src - 1];
    int src_seg = pma_header.vertex_range[local_src] - start_seg_offset;
    int edge_start = header_start + sizeof(PMA_HEADER) + start_seg_offset * sizeof(int) * SEGMENT_SIZE;
    int edge_len = src_seg * sizeof(int) * SEGMENT_SIZE;
    VEC_INFO_STREAM edge_vec_info;
    edge_vec_info.data.range(31,0) = edge_start;
    edge_vec_info.data.range(63,32) = edge_len;
    vec_info_stream.write(edge_vec_info);
    cmd_to_mem_stream.write({.data=MEM_READ});
    // printf("Reading edges for vertex %d: start=%d, length=%d bytes\n", src, edge_start, edge_len);
    // show_header(pma_header);
    printf("EDGE COUNT: %d\n", pma_header.edge_count[local_src]);
    for (int i = 0 ; i < edge_len / sizeof(int);  i++)
    {
        int dst;
        BYTE_STREAM byte;
        for (int j = 0 ; j < sizeof(int); j++)
        {
            data_from_mem_stream.read(byte);
            ((BYTE*)&dst)[j] = byte.data;
        }
        // data_from_mem_stream.read(byte);
        if (dst == -1)
        {
            continue;
        }
        data_to_nei_query.write({.data=dst});
    }
    data_to_nei_query.write({.data=-1});
}
extern "C"
{
    void pma_read_kernel(
        VEC_INFO* pos_pma_addrs,
        VEC_INFO* neg_pma_addrs,
        hls::stream<READ_COMMAND_STREAM> &cmd_stream,
        hls::stream<ap_axiu<32,0,0,0>> &data_to_nei_query,
        hls::stream<ap_axiu<64,0,0,0>> &data_to_attr_query,
        //memory
        hls::stream<VEC_INFO_STREAM> &vec_info_stream,
        hls::stream<COMMAND_STREAM> &cmd_to_mem_stream,
        hls::stream<BYTE_STREAM> &data_from_mem_stream,
        hls::stream<VEC_INFO_STREAM> &vec_info_from_mem,
        hls::stream<BYTE_STREAM> &data_to_mem_stream
        // hls::stream<COMMAND_STREAM> &stop_signal
    )
    {
        #pragma HLS INTERFACE m_axi port=pos_pma_addrs offset=slave bundle=gmem0
        #pragma HLS INTERFACE m_axi port=neg_pma_addrs offset=slave bundle=gmem1
        #pragma HLS INTERFACE axis port=cmd_stream
        #pragma HLS INTERFACE axis port=data_to_nei_query
        #pragma HLS INTERFACE axis port=data_to_attr_query
        #pragma HLS INTERFACE axis port=vec_info_stream
        #pragma HLS INTERFACE axis port=cmd_to_mem_stream   
        #pragma HLS INTERFACE axis port=data_to_mem_stream
        #pragma HLS INTERFACE axis port=data_from_mem_stream
        #pragma HLS INTERFACE axis port=vec_info_from_mem
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
                    if (vec_info == 0) // empty PMA
                    {
                        data_to_nei_query.write({.data=-1});
                        break;
                    }
                    PMA_HEADER pma_header;
                    HERE
                    // read header
                    read_header(
                        vec_info.range(31,0),
                        vec_info_stream,
                        cmd_to_mem_stream,
                        data_from_mem_stream,
                        pma_header
                    );
                    // printf("READING OUT EDGE FOR VERTEX %d\n", src);
                    // show_header(pma_header);
                    // HERE
                    // send edge data
                    read_edge(
                        src,
                        pma_header,
                        vec_info.range(31,0),
                        vec_info_stream,
                        cmd_to_mem_stream,
                        data_from_mem_stream,
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
                    if (vec_info == 0) // empty PMA
                    {
                        data_to_nei_query.write({.data=-1});
                        break;
                    }
                    PMA_HEADER pma_header;
                    // read header
                    read_header(
                        vec_info.range(31,0),
                        vec_info_stream,
                        cmd_to_mem_stream,
                        data_from_mem_stream,
                        pma_header
                    );
                    // send edge data
                    read_edge(
                        src,
                        pma_header,
                        vec_info.range(31,0),
                        vec_info_stream,
                        cmd_to_mem_stream,
                        data_from_mem_stream,
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
                    // read header
                    read_header(
                        vec_info.range(31,0),
                        vec_info_stream,
                        cmd_to_mem_stream,
                        data_from_mem_stream,
                        pma_header
                    );
                    double attr = pma_header.attr[vertex % VERTEX_PER_PMA];
                    data_to_attr_query.write({.data=attr});
                    break;
                }
                case NONE:
                {
                    vec_info_from_mem.read();
                    data_to_mem_stream.write({.data=0});
                    break;
                }
            }
        }
    }
}
