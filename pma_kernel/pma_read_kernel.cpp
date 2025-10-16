#include "data_type.h"
#include <hls_stream.h>
#include <ap_int.h> 


void read_header(
    int header_start,
    hls::stream<VEC_INFO_STREAM> &vec_info_stream,
    hls::stream<COMMAND_STREAM> &cmd_to_mem_stream,
    hls::stream<BYTE_STREAM> &data_from_mem_stream,
    PMA_HEADER &pma_header
)
{
    VEC_INFO_STREAM header_vec_info;
    header_vec_info.data.range(31,0) = header_start;
    header_vec_info.data.range(63,32) = sizeof(PMA_HEADER);
    vec_info_stream.write(header_vec_info);
    for (int i = 0 ; i < sizeof(PMA_HEADER); i++)
    {
        BYTE_STREAM byte;
        data_from_mem_stream.read(byte);
        ((BYTE*)&pma_header)[i] = byte.data;
    }
}
void read_edge(
    int src,
    PMA_HEADER &pma_header,
    hls::stream<VEC_INFO_STREAM> &vec_info_stream,
    hls::stream<COMMAND_STREAM> &cmd_to_mem_stream,
    hls::stream<BYTE_STREAM> &data_from_mem_stream,
    hls::stream<ap_axiu<32,0,0,0>> &data_to_nei_query
)
{
    int local_src = src % VERTEX_PER_PMA;
    int src_seg = pma_header.vertex_range[local_src] - (local_src == 0 ? 0 : pma_header.vertex_range[local_src - 1]);
    int edge_start = sizeof(PMA_HEADER) + src_seg * SEGMENT_SIZE * sizeof(int);
    int edge_len = pma_header.edge_count[local_src] * sizeof(int);
    VEC_INFO_STREAM edge_vec_info;
    edge_vec_info.data.range(31,0) = edge_start;
    edge_vec_info.data.range(63,32) = edge_len;
    vec_info_stream.write(edge_vec_info);
    for (int i = 0 ; i < edge_len;  i++)
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
    )
    {
        while (true)
        {
            READ_COMMAND_STREAM cmd = cmd_stream.read();
            if (cmd.data == PMA_STOP && cmd.last == 1) 
            {
                break;
            }
            switch (cmd.data.range(31,0)) 
            {
                case PMA_GET_OUT_EDGE:
                {
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
                        vec_info_stream,
                        cmd_to_mem_stream,
                        data_from_mem_stream,
                        data_to_nei_query
                    );
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
            }
        }
    }
}
