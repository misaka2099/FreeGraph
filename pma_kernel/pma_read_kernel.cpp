#include "data_type.h"
#include <ap_utils.h>
#include "debug.h"
#include "etc/autopilot_ssdm_op.h"
#include <cstdio>
#include <cstring>
#include <hls_stream.h>
#include <ap_int.h>

void read_header(
    int start_addr,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &memory_access_stream,
    hls::stream<ap_axiu<512,0,0,0>> &data_from_mem_access,
    PMA_HEADER &pma_header);

VEC_INFO fetch_vec_info(
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_stream,
    hls::stream<VEC_INFO_STREAM> &vec_stream,
    ap_uint<1> direction,
    int pma_idx)
{
    #pragma HLS INLINE
    MEMORY_INFO_ACCESS_COMMAND cmd = {};
    cmd.command_type = (direction == OUT_DIRECTION) ? POS_VEC_INFO_READ : NEG_VEC_INFO_READ;
    cmd.pma_idx = pma_idx;
    cmd.start = 0;
    cmd.len = 0;
    cmd.last = 0;
    req_stream.write(cmd);
    #pragma HLS protocol fixed
    ap_wait();
    VEC_INFO_STREAM vec_pkt = vec_stream.read();
    return vec_pkt.data;
}

int read_edge(
    int src,
    PMA_HEADER &pma_header,
    int header_start,
    hls::stream<ap_axiu<32,0,0,0>> &nei_stream,
    hls::stream<ap_axiu<64,0,0,0>> &coo_stream,
    hls::stream<ap_axiu<32,0,0,0>> &csr_col_stream,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access,
    hls::stream<ap_axiu<512,0,0,0>> &data_from_mem_access,
    int mode)
{
    #pragma HLS INLINE
    int local_src = src % VERTEX_PER_PMA;
    int start_seg_offset = local_src == 0 ? 0 : pma_header.vertex_range[local_src - 1];
    int src_seg = pma_header.vertex_range[local_src] - start_seg_offset;
    int edge_start = header_start + sizeof(PMA_HEADER) + start_seg_offset * sizeof(int) * SEGMENT_SIZE;
    int edge_len = src_seg * sizeof(int) * SEGMENT_SIZE;
    
    int aligned_len = (edge_len + 63) & ~0x3F;
    if (aligned_len == 0)
    {
        if (mode == 0)
        {
            ap_axiu<32,0,0,0> nei_pkt = {};
            nei_pkt.data = (ap_uint<32>)(-1);
            nei_pkt.keep = (ap_uint<4>)0xF;
            nei_pkt.strb = (ap_uint<4>)0xF;
            nei_pkt.last = 1;
            nei_stream.write(nei_pkt);
        }
        if (mode == 1)
        {
            // ap_axiu<64,0,0,0> coo_pkt = {};
            // coo_pkt.data = ((ap_uint<64>)src << 32) | (ap_uint<32>)(-1);
            // coo_pkt.keep = (ap_uint<8>)0xFF;
            // coo_pkt.strb = (ap_uint<8>)0xFF;
            // coo_pkt.last = 1;
            // coo_stream.write(coo_pkt);
        }
        if (mode == 2)
        {
            // ap_axiu<32,0,0,0> csr_pkt = {};
            // csr_pkt.data = (ap_uint<32>)-1;
            // csr_pkt.keep = (ap_uint<8>)0xFF;
            // csr_pkt.strb = (ap_uint<8>)0xFF;
            // csr_pkt.last = 1;
            // csr_col_stream.write(csr_pkt);
        }
        return 0;
    }

    int first_word = edge_start >> 6;
    int word_count = aligned_len >> 6;
    ap_uint<512> segment_buffer[4];
    int csr_count = 0;
    MEMORY_INFO_ACCESS_COMMAND req_cmd = {};
    req_cmd.command_type = MEMORY_READ;
    req_cmd.start = edge_start;
    req_cmd.len = aligned_len;
    req_cmd.last = 0;
    req_to_mem_access.write(req_cmd);
    #pragma HLS protocol fixed
    ap_wait();
    for (int seg = 0 ; seg < src_seg ; seg++)
    {
        // #pragma HLS PIPELINE II=1
        for (int i = 0 ; i < 4 ; i++)
        {
            #pragma HLS PIPELINE II=1
            auto pkt = data_from_mem_access.read();
            segment_buffer[i] = pkt.data;
        }
        for (int i = 0 ; i < SEGMENT_SIZE; i++)
        {
            #pragma HLS PIPELINE II=1
            int *segment_ptr = (int*)&segment_buffer;
            int neighbor = segment_ptr[i];
            if (neighbor == UNUSED_ICON)
            {
                break;
            }
            if (mode == 0)
            {
                ap_axiu<32,0,0,0> nei_pkt = {};
                nei_pkt.data = (ap_uint<32>)neighbor;
                nei_pkt.keep = (ap_uint<4>)0xF;
                nei_pkt.strb = (ap_uint<4>)0xF;
                nei_pkt.last = 0;
                nei_stream.write(nei_pkt);
            }
            if (mode == 1)
            {
                ap_axiu<64,0,0,0> coo_pkt = {};
                coo_pkt.data = ((ap_uint<64>)src << 32) | (ap_uint<32>)neighbor;
                coo_pkt.keep = (ap_uint<8>)0xFF;
                coo_pkt.strb = (ap_uint<8>)0xFF;
                coo_pkt.last = 0;
                coo_stream.write(coo_pkt);
            }
            if (mode == 2)
            {
                ap_axiu<32,0,0,0> csr_col_pkt = {};
                csr_col_pkt.data = ((ap_uint<32>)neighbor);
                csr_col_pkt.keep = (ap_uint<8>)0xFF;
                csr_col_pkt.strb = (ap_uint<8>)0xFF;
                csr_col_pkt.last = 0;
                csr_col_stream.write(csr_col_pkt);
                csr_count++;
            }
        }
    }

    if (mode == 0)
    {
        ap_axiu<32,0,0,0> nei_end = {};
        nei_end.data = (ap_uint<32>)(-1);
        nei_end.keep = (ap_uint<4>)0xF;
        nei_end.strb = (ap_uint<4>)0xF;
        nei_end.last = 1;
        nei_stream.write(nei_end);
    }
    if (mode == 1)
    {
        // ap_axiu<64,0,0,0> coo_end = {};
        // coo_end.data = ((ap_uint<64>)src << 32) | (ap_uint<32>)(-1);
        // coo_end.keep = (ap_uint<8>)0xFF;
        // coo_end.strb = (ap_uint<8>)0xFF;
        // coo_end.last = 1;
        // coo_stream.write(coo_end);
    }
    if (mode == 2)
    {
        // ap_axiu<64,0,0,0> csr_summary = {};
        // csr_summary.data = ((ap_uint<64>)csr_row_start << 32) | (ap_uint<32>)(csr_row_start + csr_count);
        // csr_summary.keep = (ap_uint<8>)0xFF;
        // csr_summary.strb = (ap_uint<8>)0xFF;
        // csr_summary.last = 1;
        // csr_col_stream.write(csr_summary);
    }
    return csr_count;
}

static void send_empty_neighbor(hls::stream<ap_axiu<32,0,0,0>> &nei_stream)
{
    #pragma HLS INLINE
    ap_axiu<32,0,0,0> nei_pkt = {};
    nei_pkt.data = (ap_uint<32>)(-1);
    nei_pkt.keep = (ap_uint<4>)0xF;
    nei_pkt.strb = (ap_uint<4>)0xF;
    nei_pkt.last = 1;
    nei_stream.write(nei_pkt);
}

static bool load_pma_header(
    int pma_idx,
    ap_uint<1> direction,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_stream,
    hls::stream<VEC_INFO_STREAM> &vec_stream,
    hls::stream<ap_axiu<512,0,0,0>> &data_stream,
    PMA_HEADER &pma_header,
    int &header_start)
{
    #pragma HLS INLINE
    VEC_INFO vec_info = fetch_vec_info(req_stream, vec_stream, direction, pma_idx);
    if (vec_info == 0)
    {
        return false;
    }
    header_start = vec_info.range(31,0);
    read_header(header_start, req_stream, data_stream, pma_header);
    return true;
}

void read_header(
    int start_addr,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &memory_access_stream,
    hls::stream<ap_axiu<512,0,0,0>> &data_from_mem_access,
    PMA_HEADER &pma_header)

{
    #pragma HLS INLINE
    MEMORY_INFO_ACCESS_COMMAND header_cmd = {};
    header_cmd.command_type = MEMORY_READ;
    header_cmd.start = start_addr;
    header_cmd.len = sizeof(PMA_HEADER);
    header_cmd.last = 0;
    memory_access_stream.write(header_cmd);
    const int header_word_count = sizeof(PMA_HEADER) >> 6;
    ap_uint<512>* header_ptr = reinterpret_cast<ap_uint<512>*>(&pma_header);
    #pragma HLS protocol fixed
    ap_wait();
    for (int i = 0; i < header_word_count; ++i)
    {
        #pragma HLS PIPELINE II=1
        auto pkt = data_from_mem_access.read();
        header_ptr[i] = pkt.data;
    }
}
extern "C"
{
    void pma_read_kernel(
        hls::stream<READ_COMMAND_STREAM> &cmd_stream,
        hls::stream<ap_axiu<32,0,0,0>> &data_to_nei_query,
        hls::stream<ap_axiu<64,0,0,0>> &data_to_coo_stream,
        hls::stream<ap_axiu<32,0,0,0>> &data_to_csr_col_stream,
        hls::stream<ap_axiu<32,0,0,0>> &data_to_csr_row_ptr_stream,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access_ch1,
        hls::stream<VEC_INFO_STREAM> &vec_info_back_stream_ch1,
        hls::stream<ap_axiu<512,0,0,0>> &data_from_mem_access_ch1,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access_ch2,
        hls::stream<VEC_INFO_STREAM> &vec_info_back_stream_ch2,
        hls::stream<ap_axiu<512,0,0,0>> &data_from_mem_access_ch2
    )
    {
        #pragma HLS INTERFACE axis port=cmd_stream
        #pragma HLS INTERFACE axis port=data_to_nei_query
        #pragma HLS INTERFACE axis port=data_to_coo_stream
        #pragma HLS INTERFACE axis port=data_to_csr_col_stream
        #pragma HLS INTERFACE axis port=data_to_csr_row_ptr_stream
        #pragma HLS INTERFACE axis port=req_to_mem_access_ch1
        #pragma HLS INTERFACE axis port=vec_info_back_stream_ch1
        #pragma HLS INTERFACE axis port=data_from_mem_access_ch1
        #pragma HLS INTERFACE axis port=req_to_mem_access_ch2
        #pragma HLS INTERFACE axis port=vec_info_back_stream_ch2
        #pragma HLS INTERFACE axis port=data_from_mem_access_ch2
        while (true)
        {
            READ_COMMAND_STREAM cmd = cmd_stream.read();
            if (cmd.data == PMA_STOP && cmd.last == 1) 
            {
                break;
            }
            ap_uint<32> opcode = cmd.data.range(31,0);
            ap_uint<1> direction = OUT_DIRECTION;
            int mode = -1;
            if (opcode == PMA_GET_ALL_IN_EDGE_COO || opcode == PMA_GET_IN_EDGE || opcode == PMA_GET_ALL_IN_EDGE_CSR)
            {
                direction = IN_DIRECTION;
            }
            if (opcode == PMA_GET_IN_EDGE || opcode == PMA_GET_OUT_EDGE)
            {
                mode = 0;
            }
            else if (opcode == PMA_GET_ALL_IN_EDGE_COO || opcode == PMA_GET_ALL_OUT_EDGE_COO)
            {
                mode = 1;
            }
            else if (opcode == PMA_GET_ALL_IN_EDGE_CSR || opcode == PMA_GET_ALL_OUT_EDGE_CSR)
            {
                mode = 2;
            }
            else
            {
                continue;
            }

            #pragma HLS protocol fixed
            ap_wait();

            if (mode == 0)
            {
                int src = cmd.data.range(63,32);
                int pma_idx = src / VERTEX_PER_PMA;
                if ((pma_idx & 1) == 0)
                {
                    PMA_HEADER pma_header;
                    int header_start = 0;
                    if (!load_pma_header(pma_idx, direction, req_to_mem_access_ch1, vec_info_back_stream_ch1, data_from_mem_access_ch1, pma_header, header_start))
                    {
                        send_empty_neighbor(data_to_nei_query);
                        continue;
                    }
                    read_edge(src, pma_header, header_start, data_to_nei_query, data_to_coo_stream, data_to_csr_col_stream,
                        req_to_mem_access_ch1, data_from_mem_access_ch1, 0);
                }
                else
                {
                    PMA_HEADER pma_header;
                    int header_start = 0;
                    if (!load_pma_header(pma_idx, direction, req_to_mem_access_ch2, vec_info_back_stream_ch2, data_from_mem_access_ch2, pma_header, header_start))
                    {
                        send_empty_neighbor(data_to_nei_query);
                        continue;
                    }
                    read_edge(src, pma_header, header_start, data_to_nei_query, data_to_coo_stream, data_to_csr_col_stream,
                        req_to_mem_access_ch2, data_from_mem_access_ch2, 0);
                }
            }
            else if (mode == 1)
            {
                for (int pma_idx = 0; pma_idx < MAX_PMA_NUM; ++pma_idx)
                {
                    if ((pma_idx & 1) == 0)
                    {
                        PMA_HEADER pma_header;
                        int header_start = 0;
                        if (!load_pma_header(pma_idx, direction, req_to_mem_access_ch1, vec_info_back_stream_ch1, data_from_mem_access_ch1, pma_header, header_start))
                        {
                            continue;
                        }
                        for (int local_v = 0; local_v < VERTEX_PER_PMA; ++local_v)
                        {
                            int src = pma_idx * VERTEX_PER_PMA + local_v;
                            read_edge(src, pma_header, header_start, data_to_nei_query, data_to_coo_stream, data_to_csr_col_stream,
                                req_to_mem_access_ch1, data_from_mem_access_ch1, 1);
                        }
                    }
                    else
                    {
                        PMA_HEADER pma_header;
                        int header_start = 0;
                        if (!load_pma_header(pma_idx, direction, req_to_mem_access_ch2, vec_info_back_stream_ch2, data_from_mem_access_ch2, pma_header, header_start))
                        {
                            continue;
                        }
                        for (int local_v = 0; local_v < VERTEX_PER_PMA; ++local_v)
                        {
                            int src = pma_idx * VERTEX_PER_PMA + local_v;
                            read_edge(src, pma_header, header_start, data_to_nei_query, data_to_coo_stream, data_to_csr_col_stream,
                                req_to_mem_access_ch2, data_from_mem_access_ch2, 1);
                        }
                    }
                }

                ap_axiu<64,0,0,0> coo_end = {};
                coo_end.data = (ap_uint<64>)(-1);
                coo_end.keep = (ap_uint<8>)0xFF;
                coo_end.strb = (ap_uint<8>)0xFF;
                coo_end.last = 1;
                data_to_coo_stream.write(coo_end);
            }
            else
            {
                ap_uint<32> csr_offset = 0;
                for (int pma_idx = 0; pma_idx < MAX_PMA_NUM; ++pma_idx)
                {
                    if ((pma_idx & 1) == 0)
                    {
                        PMA_HEADER pma_header;
                        int header_start = 0;
                        if (!load_pma_header(pma_idx, direction, req_to_mem_access_ch1, vec_info_back_stream_ch1, data_from_mem_access_ch1, pma_header, header_start))
                        {
                            continue;
                        }
                        for (int local_v = 0; local_v < VERTEX_PER_PMA; ++local_v)
                        {
                            int src = pma_idx * VERTEX_PER_PMA + local_v;
                            ap_axiu<32,0,0,0> row_ptr_pkt = {};
                            row_ptr_pkt.data = csr_offset;
                            row_ptr_pkt.keep = (ap_uint<8>)0xFF;
                            row_ptr_pkt.strb = (ap_uint<8>)0xFF;
                            row_ptr_pkt.last = 0;
                            data_to_csr_row_ptr_stream.write(row_ptr_pkt);
                            int emitted = read_edge(src, pma_header, header_start, data_to_nei_query, data_to_coo_stream, data_to_csr_col_stream,
                                req_to_mem_access_ch1, data_from_mem_access_ch1, 2);
                            csr_offset += emitted;
                        }
                    }
                    else
                    {
                        PMA_HEADER pma_header;
                        int header_start = 0;
                        if (!load_pma_header(pma_idx, direction, req_to_mem_access_ch2, vec_info_back_stream_ch2, data_from_mem_access_ch2, pma_header, header_start))
                        {
                            continue;
                        }
                        for (int local_v = 0; local_v < VERTEX_PER_PMA; ++local_v)
                        {
                            int src = pma_idx * VERTEX_PER_PMA + local_v;
                            ap_axiu<32,0,0,0> row_ptr_pkt = {};
                            row_ptr_pkt.data = csr_offset;
                            row_ptr_pkt.keep = (ap_uint<8>)0xFF;
                            row_ptr_pkt.strb = (ap_uint<8>)0xFF;
                            row_ptr_pkt.last = 0;
                            data_to_csr_row_ptr_stream.write(row_ptr_pkt);
                            int emitted = read_edge(src, pma_header, header_start, data_to_nei_query, data_to_coo_stream, data_to_csr_col_stream,
                                req_to_mem_access_ch2, data_from_mem_access_ch2, 2);
                            csr_offset += emitted;
                        }
                    }
                }

                ap_axiu<32,0,0,0> row_ptr_end = {};
                row_ptr_end.data = csr_offset;
                row_ptr_end.keep = (ap_uint<8>)0xFF;
                row_ptr_end.strb = (ap_uint<8>)0xFF;
                row_ptr_end.last = 1;
                data_to_csr_row_ptr_stream.write(row_ptr_end);

                ap_axiu<32,0,0,0> csr_col_end = {};
                csr_col_end.data = (ap_uint<32>)(-1);
                csr_col_end.keep = (ap_uint<8>)0xFF;
                csr_col_end.strb = (ap_uint<8>)0xFF;
                csr_col_end.last = 1;
                data_to_csr_col_stream.write(csr_col_end);
            }
            
            // if (cmd.data.range(31,0) == PMA_GET_OUT_EDGE || cmd.data.range(31,0) == PMA_GET_IN_EDGE)
            // {
            //     int src = cmd.data.range(63,32);
            //     int pma_idx = src / VERTEX_PER_PMA;
            //     if ((pma_idx % 2) == 0)
            //     {
            //         VEC_INFO vec_info = fetch_vec_info(req_to_mem_access_ch1, vec_info_back_stream_ch1, direction, pma_idx);
            //         #pragma HLS protocol fixed
            //         ap_wait(); 
            //         int start_addr = vec_info.range(31,0);
            //         if (vec_info == 0)
            //         {
            //             data_to_nei_query.write({.data=-1});
            //             // break;
            //             continue;
            //         }
            //         PMA_HEADER pma_header;
            //         read_header(start_addr, req_to_mem_access_ch1, data_from_mem_access_ch1, pma_header);
            //         read_edge(
            //             src,
            //             pma_header,
            //             vec_info.range(31,0),
            //             data_to_nei_query,
            //             data_to_coo_stream,
            //             data_to_csr_col_stream,
            //             req_to_mem_access_ch1,
            //             data_from_mem_access_ch1,
            //             0
            //         );
            //     }
            //     else
            //     {
            //         VEC_INFO vec_info = fetch_vec_info(req_to_mem_access_ch2, vec_info_back_stream_ch2, direction, pma_idx);
            //         #pragma HLS protocol fixed
            //         ap_wait(); 
            //         int start_addr = vec_info.range(31,0);
            //         if (vec_info == 0)
            //         {
            //             data_to_nei_query.write({.data=-1});
            //             // break;
                        
            //             continue;
            //         }
            //         PMA_HEADER pma_header;
            //         read_header(start_addr, req_to_mem_access_ch2, data_from_mem_access_ch2, pma_header);
            //         read_edge(
            //             src,
            //             pma_header,
            //             vec_info.range(31,0),
            //             data_to_nei_query,
            //             data_to_coo_stream,
            //             data_to_csr_col_stream,
            //             req_to_mem_access_ch2,
            //             data_from_mem_access_ch2,
            //             0
            //         );
            //     }
            // }
            // else if (cmd.data.range(31,0) == PMA_GET_ALL_IN_EDGE_COO || cmd.data.range(31,0) == PMA_GET_ALL_OUT_EDGE_COO)
            // {
            //     for (int pma_idx = 0; pma_idx < MAX_PMA_NUM; pma_idx++)
            //     {
            //         if ((pma_idx % 2) == 0)
            //         {
            //             VEC_INFO vec_info = fetch_vec_info(req_to_mem_access_ch1, vec_info_back_stream_ch1, direction, pma_idx);
            //             #pragma HLS protocol fixed
            //             ap_wait(); 
            //             if (vec_info == 0)
            //             {
            //                 // break;
            //                 continue;
            //             }
            //             PMA_HEADER pma_header;
            //             read_header(vec_info.range(31,0), req_to_mem_access_ch1, data_from_mem_access_ch1, pma_header);
            //             for (int local_v = 0; local_v < VERTEX_PER_PMA; local_v++)
            //             {
            //                 int src = pma_idx * VERTEX_PER_PMA + local_v;
            //                 read_edge(src, pma_header, vec_info.range(31,0), data_to_nei_query,
            //                     data_to_coo_stream,
            //                     data_to_csr_col_stream,
            //                     req_to_mem_access_ch1,
            //                     data_from_mem_access_ch1,
            //                     1);
            //             }
            //         }
            //         else
            //         {
            //             VEC_INFO vec_info = fetch_vec_info(req_to_mem_access_ch2, vec_info_back_stream_ch2, direction, pma_idx);
            //             // #pragma HLS protocol fixed
            //             // ap_wait();
            //             if (vec_info == 0)
            //             {
            //                 // break;
            //                 continue;
            //             }
            //             PMA_HEADER pma_header;
            //             read_header(vec_info.range(31,0), req_to_mem_access_ch2, data_from_mem_access_ch2, pma_header);
            //             for (int local_v = 0; local_v < VERTEX_PER_PMA; local_v++)
            //             {
            //                 int src = pma_idx * VERTEX_PER_PMA + local_v;
            //                 read_edge(src, pma_header, vec_info.range(31,0), data_to_nei_query,
            //                     data_to_coo_stream,
            //                     data_to_csr_col_stream,
            //                     req_to_mem_access_ch2,
            //                     data_from_mem_access_ch2,
            //                     1);
            //             }
            //         }
            //     }
            //     data_to_coo_stream.write({.data=-1, .last=1});
            // }
            // else if (cmd.data.range(31,0) == PMA_GET_ALL_IN_EDGE_CSR || cmd.data.range(31,0) == PMA_GET_ALL_OUT_EDGE_CSR)
            // {
            //     ap_uint<32> csr_offset = 0;
            //     for (int pma_idx = 0; pma_idx < MAX_PMA_NUM; ++pma_idx)
            //     {
            //         if ((pma_idx % 2) == 0)
            //         {
            //             VEC_INFO vec_info = fetch_vec_info(req_to_mem_access_ch1, vec_info_back_stream_ch1, direction, pma_idx);
            //             #pragma HLS protocol fixed
            //             ap_wait(); 
            //             if (vec_info == 0)
            //             {
            //                 // break;
                            
            //                 continue;
            //             }
            //             PMA_HEADER pma_header;
            //             int start_addr = vec_info.range(31,0);
            //             read_header(start_addr, req_to_mem_access_ch1, data_from_mem_access_ch1, pma_header);
            //             for (int local_v = 0; local_v < VERTEX_PER_PMA; local_v++)
            //             {
            //                 int src = pma_idx * VERTEX_PER_PMA + local_v;
            //                 ap_axiu<32,0,0,0> row_ptr_pkt = {};
            //                 row_ptr_pkt.data = csr_offset;
            //                 row_ptr_pkt.keep = (ap_uint<8>)0xFF;
            //                 row_ptr_pkt.strb = (ap_uint<8>)0xFF;
            //                 row_ptr_pkt.last = 0;
            //                 data_to_csr_row_ptr_stream.write(row_ptr_pkt);
            //                 int emitted = read_edge(src, pma_header, start_addr, data_to_nei_query,
            //                     data_to_coo_stream,
            //                     data_to_csr_col_stream,
            //                     req_to_mem_access_ch1,
            //                     data_from_mem_access_ch1,
            //                     2);
            //                 csr_offset += emitted;
            //             }
            //         }
            //         else
            //         {
            //             VEC_INFO vec_info = fetch_vec_info(req_to_mem_access_ch2, vec_info_back_stream_ch2, direction, pma_idx);
            //             // #pragma HLS protocol fixed
            //             // ap_wait();
            //             if (vec_info == 0)
            //             {
            //                 // break;
            //                 continue;
            //             }
            //             PMA_HEADER pma_header;
            //             int start_addr = vec_info.range(31,0);
            //             read_header(start_addr, req_to_mem_access_ch2, data_from_mem_access_ch2, pma_header);
            //             for (int local_v = 0; local_v < VERTEX_PER_PMA; local_v++)
            //             {
            //                 int src = pma_idx * VERTEX_PER_PMA + local_v;
            //                 ap_axiu<32,0,0,0> row_ptr_pkt = {};
            //                 row_ptr_pkt.data = csr_offset;
            //                 row_ptr_pkt.keep = (ap_uint<8>)0xFF;
            //                 row_ptr_pkt.strb = (ap_uint<8>)0xFF;
            //                 row_ptr_pkt.last = 0;
            //                 data_to_csr_row_ptr_stream.write(row_ptr_pkt);
            //                 int emitted = read_edge(src, pma_header, start_addr, data_to_nei_query,
            //                     data_to_coo_stream,
            //                     data_to_csr_col_stream,
            //                     req_to_mem_access_ch2,
            //                     data_from_mem_access_ch2,
            //                     2);
            //                 csr_offset += emitted;
            //             }
            //         }
            //     }
            //     ap_axiu<32,0,0,0> row_ptr_end = {};
            //     row_ptr_end.data = csr_offset;
            //     row_ptr_end.keep = (ap_uint<8>)0xFF;
            //     row_ptr_end.strb = (ap_uint<8>)0xFF;
            //     row_ptr_end.last = 1;
            //     data_to_csr_row_ptr_stream.write(row_ptr_end);
                
            //     data_to_csr_col_stream.write({.data=-1, .last=1});
            // }
            // else if (cmd.data.range(31,0) == PMA_GET_ATTR)
            // {
            //     int vertex = cmd.data.range(63,32);
            //     int pma_idx = vertex / VERTEX_PER_PMA;
            //     MemChannelSet channel = select_channel(
            //         pma_idx,
            //         req_to_mem_access_ch1,
            //         vec_info_back_stream_ch1,
            //         data_from_mem_access_ch1,
            //         req_to_mem_access_ch2,
            //         vec_info_back_stream_ch2,
            //         data_from_mem_access_ch2);
            //     VEC_INFO vec_info = fetch_vec_info(channel.req, channel.vec, OUT_DIRECTION, pma_idx);
            //     if (vec_info == 0) // empty PMA
            //     {
            //         data_to_attr_query.write({.data=-1});
            //         break;
            //     }
            //     PMA_HEADER pma_header;
            //     int start_addr =
            // switch (cmd.data.range(31,0)) 
            // {
            //     case PMA_GET_OUT_EDGE:
            //     {
            //         int src = cmd.data.range(63,32);
            //         int pma_idx = src / VERTEX_PER_PMA;
            //         MemChannelSet channel = select_channel(
            //             pma_idx,
            //             req_to_mem_access_ch1,
            //             vec_info_back_stream_ch1,
            //             data_from_mem_access_ch1,
            //             req_to_mem_access_ch2,
            //             vec_info_back_stream_ch2,
            //             data_from_mem_access_ch2);
            //         VEC_INFO vec_info = fetch_vec_info(channel.req, channel.vec, OUT_DIRECTION, pma_idx);
            //         int start_addr = vec_info.range(31,0);
            //         if (vec_info == 0) // empty PMA
            //         {
            //             data_to_nei_query.write({.data=-1});
            //             break;
            //         }
            //         PMA_HEADER pma_header;
            //         read_header(start_addr, channel.req, channel.data, pma_header);
            //         read_edge(
            //             src,
            //             pma_header,
            //             vec_info.range(31,0),
            //             data_to_nei_query,
            //             data_to_coo_stream,
            //             data_to_csr_col_stream,
            //             channel.req,
            //             channel.data,
            //             0
            //         );
            //         break;
            //     }
            //     case PMA_GET_IN_EDGE:
            //     {
            //         int src = cmd.data.range(63,32);
            //         int pma_idx = src / VERTEX_PER_PMA;
            //         MemChannelSet channel = select_channel(
            //             pma_idx,
            //             req_to_mem_access_ch1,
            //             vec_info_back_stream_ch1,
            //             data_from_mem_access_ch1,
            //             req_to_mem_access_ch2,
            //             vec_info_back_stream_ch2,
            //             data_from_mem_access_ch2);
            //         VEC_INFO vec_info = fetch_vec_info(channel.req, channel.vec, IN_DIRECTION, pma_idx);
            //         int start_addr = vec_info.range(31,0);
            //         if (vec_info == 0) // empty PMA
            //         {
            //             data_to_nei_query.write({.data=-1});
            //             break;
            //         }
            //         PMA_HEADER pma_header;
            //         read_header(start_addr, channel.req, channel.data, pma_header);
                    
            //         read_edge(
            //             src,
            //             pma_header,
            //             vec_info.range(31,0),
            //             data_to_nei_query,
            //             data_to_coo_stream,
            //             data_to_csr_col_stream,
            //             channel.req,
            //             channel.data,
            //             0
            //         );
            //         break;
            //     }
            //     case PMA_GET_ATTR:
            //     {
            //         int vertex = cmd.data.range(63,32);
            //         int pma_idx = vertex / VERTEX_PER_PMA;
            //         MemChannelSet channel = select_channel(
            //             pma_idx,
            //             req_to_mem_access_ch1,
            //             vec_info_back_stream_ch1,
            //             data_from_mem_access_ch1,
            //             req_to_mem_access_ch2,
            //             vec_info_back_stream_ch2,
            //             data_from_mem_access_ch2);
            //         VEC_INFO vec_info = fetch_vec_info(channel.req, channel.vec, OUT_DIRECTION, pma_idx);
            //         if (vec_info == 0) // empty PMA
            //         {
            //             data_to_attr_query.write({.data=-1});
            //             break;
            //         }
            //         PMA_HEADER pma_header;
            //         int start_addr = vec_info.range(31,0);
            //         read_header(start_addr, channel.req, channel.data, pma_header);
            //         double attr = pma_header.attr[vertex % VERTEX_PER_PMA];
            //         data_to_attr_query.write({.data=attr});
            //         break;
            //     }
            //     case PMA_GET_ALL_OUT_EDGE_COO:
            //     {
            //         for (int pma_idx = 0; pma_idx < MAX_PMA_NUM; pma_idx++)
            //         {
            //             MemChannelSet channel = select_channel(
            //                 pma_idx,
            //                 req_to_mem_access_ch1,
            //                 vec_info_back_stream_ch1,
            //                 data_from_mem_access_ch1,
            //                 req_to_mem_access_ch2,
            //                 vec_info_back_stream_ch2,
            //                 data_from_mem_access_ch2);
            //             VEC_INFO vec_info = fetch_vec_info(channel.req, channel.vec, OUT_DIRECTION, pma_idx);
            //             if (vec_info == 0) break;
            //             PMA_HEADER pma_header;
            //             read_header(vec_info.range(31,0), channel.req, channel.data, pma_header);
            //             for (int local_v = 0; local_v < VERTEX_PER_PMA; local_v++)
            //             {
            //                 int src = pma_idx * VERTEX_PER_PMA + local_v;
            //                 read_edge(src, pma_header, vec_info.range(31,0), data_to_nei_query,
            //                     data_to_coo_stream,
            //                     data_to_csr_col_stream,
            //                     channel.req,
            //                     channel.data,
            //                     1);
            //             }
            //         }
            //         data_to_coo_stream.write({.data=-1, .last=1});
            //         break;
            //     }
            //     case PMA_GET_ALL_IN_EDGE_COO:
            //     {
            //         for (int pma_idx = 0; pma_idx < MAX_PMA_NUM; pma_idx++)
            //         {
            //             MemChannelSet channel = select_channel(
            //                 pma_idx,
            //                 req_to_mem_access_ch1,
            //                 vec_info_back_stream_ch1,
            //                 data_from_mem_access_ch1,
            //                 req_to_mem_access_ch2,
            //                 vec_info_back_stream_ch2,
            //                 data_from_mem_access_ch2);
            //             VEC_INFO vec_info = fetch_vec_info(channel.req, channel.vec, IN_DIRECTION, pma_idx);
            //             if (vec_info == 0) break;
            //             PMA_HEADER pma_header;
            //             read_header(vec_info.range(31,0), channel.req, channel.data, pma_header);
            //             for (int local_v = 0; local_v < VERTEX_PER_PMA; local_v++)
            //             {
            //                 int src = pma_idx * VERTEX_PER_PMA + local_v;
            //                 read_edge(src, pma_header, vec_info.range(31,0), data_to_nei_query,
            //                     data_to_coo_stream,
            //                     data_to_csr_col_stream,
            //                     channel.req,
            //                     channel.data,
            //                     1);
            //             }
            //         }
            //         data_to_coo_stream.write({.data=-1, .last=1});
            //         break;
            //     }
            //     case PMA_GET_ALL_OUT_EDGE_CSR:
            //     {
            //         ap_uint<32> csr_offset = 0;
            //         for (int pma_idx = 0; pma_idx < MAX_PMA_NUM; pma_idx++)
            //         {
            //             MemChannelSet channel = select_channel(
            //                 pma_idx,
            //                 req_to_mem_access_ch1,
            //                 vec_info_back_stream_ch1,
            //                 data_from_mem_access_ch1,
            //                 req_to_mem_access_ch2,
            //                 vec_info_back_stream_ch2,
            //                 data_from_mem_access_ch2);
            //             VEC_INFO vec_info = fetch_vec_info(channel.req, channel.vec, OUT_DIRECTION, pma_idx);
            //             if (vec_info == 0) break;
            //             PMA_HEADER pma_header;
            //             read_header(vec_info.range(31,0), channel.req, channel.data, pma_header);
            //             for (int local_v = 0; local_v < VERTEX_PER_PMA; local_v++)
            //             {
            //                 int src = pma_idx * VERTEX_PER_PMA + local_v;
            //                 // 输出row_ptr for this vertex
            //                 ap_axiu<32,0,0,0> row_ptr_pkt = {};
            //                 row_ptr_pkt.data = csr_offset;
            //                 row_ptr_pkt.keep = (ap_uint<8>)0xFF;
            //                 row_ptr_pkt.strb = (ap_uint<8>)0xFF;
            //                 row_ptr_pkt.last = 0;
            //                 data_to_csr_row_ptr_stream.write(row_ptr_pkt);
            //                 int emitted = read_edge(src, pma_header, vec_info.range(31,0), data_to_nei_query, data_to_coo_stream, data_to_csr_col_stream, channel.req, channel.data, 2);
            //                 csr_offset += emitted;
            //             }
            //         }
            //         // 输出最后一个row_ptr[n]
            //         ap_axiu<32,0,0,0> row_ptr_end = {};
            //         row_ptr_end.data = csr_offset;
            //         row_ptr_end.keep = (ap_uint<8>)0xFF;
            //         row_ptr_end.strb = (ap_uint<8>)0xFF;
            //         row_ptr_end.last = 1;
            //         data_to_csr_row_ptr_stream.write(row_ptr_end);
            //         data_to_csr_col_stream.write({.data=-1, .last=1});
            //         break;
            //     }
            //     case PMA_GET_ALL_IN_EDGE_CSR:
            //     {
            //         ap_uint<32> csr_offset = 0;
            //         for (int pma_idx = 0; pma_idx < MAX_PMA_NUM; pma_idx++)
            //         {
            //             MemChannelSet channel = select_channel(
            //                 pma_idx,
            //                 req_to_mem_access_ch1,
            //                 vec_info_back_stream_ch1,
            //                 data_from_mem_access_ch1,
            //                 req_to_mem_access_ch2,
            //                 vec_info_back_stream_ch2,
            //                 data_from_mem_access_ch2);
            //             VEC_INFO vec_info = fetch_vec_info(channel.req, channel.vec, IN_DIRECTION, pma_idx);
            //             if (vec_info == 0) break;
            //             PMA_HEADER pma_header;
            //             read_header(vec_info.range(31,0), channel.req, channel.data, pma_header);
            //             for (int local_v = 0; local_v < VERTEX_PER_PMA; local_v++)
            //             {
            //                 int src = pma_idx * VERTEX_PER_PMA + local_v;
            //                 // 输出row_ptr for this vertex
            //                 ap_axiu<32,0,0,0> row_ptr_pkt = {};
            //                 row_ptr_pkt.data = csr_offset;
            //                 row_ptr_pkt.keep = (ap_uint<8>)0xFF;
            //                 row_ptr_pkt.strb = (ap_uint<8>)0xFF;
            //                 row_ptr_pkt.last = 0;
            //                 data_to_csr_row_ptr_stream.write(row_ptr_pkt);
            //                 int emitted = read_edge(src, pma_header, vec_info.range(31,0), data_to_nei_query, data_to_coo_stream, data_to_csr_col_stream, channel.req, channel.data, 2);
            //                 csr_offset += emitted;
            //             }
            //         }
            //         // 输出最后一个row_ptr[n]
            //         ap_axiu<32,0,0,0> row_ptr_end = {};
            //         row_ptr_end.data = csr_offset;
            //         row_ptr_end.keep = (ap_uint<8>)0xFF;
            //         row_ptr_end.strb = (ap_uint<8>)0xFF;
            //         row_ptr_end.last = 1;
            //         data_to_csr_row_ptr_stream.write(row_ptr_end);
            //         data_to_csr_col_stream.write({.data=-1, .last=1});
            //         break;
            //     }
                // case NONE:
                // {
                //     // vec_info_from_mem.read();
                //     // data_to_mem_stream.write({.data=0});
                //     break;
                // }
            // }
        }
    }
}
