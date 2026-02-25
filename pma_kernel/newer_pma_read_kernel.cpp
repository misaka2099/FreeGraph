#include "data_type.h"
#include <ap_utils.h>
#include "debug.h"
#include <cstdio>
#include <cstring>
#include <hls_stream.h>
#include <ap_int.h>

struct READ_COMMAND 
{
    int src_idx;
    int direction;
    int last;
    int mode;
};

void read_header(
    int start_addr,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &memory_access_stream,
    hls::stream<ap_axiu<512,0,0,0>> &data_from_mem_access,
    PMA_HEADER &pma_header);

static void distributor(
    hls::stream<READ_COMMAND_STREAM> &cmd_in,
    hls::stream<READ_COMMAND> &worker_0_cmd,
    hls::stream<READ_COMMAND> &worker_1_cmd,
    hls::stream<READ_COMMAND> &worker_2_cmd,
    hls::stream<READ_COMMAND> &worker_3_cmd,
    hls::stream<READ_COMMAND> &worker_4_cmd,
    hls::stream<READ_COMMAND> &worker_5_cmd,
    hls::stream<READ_COMMAND> &worker_6_cmd,
    hls::stream<READ_COMMAND> &worker_7_cmd,
    hls::stream<READ_COMMAND> &merger_cmd
)
{
    while (true)
    {
        READ_COMMAND_STREAM cmd = cmd_in.read();
        
        if (cmd.data == PMA_STOP && cmd.last == 1)
        {
            READ_COMMAND stop_cmd = {0, 0, 1, 0};
            worker_0_cmd.write(stop_cmd);
            worker_1_cmd.write(stop_cmd);
            worker_2_cmd.write(stop_cmd);
            worker_3_cmd.write(stop_cmd);
            worker_4_cmd.write(stop_cmd);
            worker_5_cmd.write(stop_cmd);
            worker_6_cmd.write(stop_cmd);
            worker_7_cmd.write(stop_cmd);
            merger_cmd.write(stop_cmd);
            break;
        }

        ap_uint<32> opcode = cmd.data.range(31,0);
        ap_uint<1> direction = OUT_DIRECTION;
        int mode = -1;
        if (opcode == PMA_GET_ALL_IN_EDGE_COO || opcode == PMA_GET_IN_EDGE || opcode == PMA_GET_ALL_IN_EDGE_CSR)
        {
            direction = IN_DIRECTION;
        }

        if (opcode == PMA_GET_IN_EDGE || opcode == PMA_GET_OUT_EDGE) mode = 0;
        else if (opcode == PMA_GET_ALL_IN_EDGE_COO || opcode == PMA_GET_ALL_OUT_EDGE_COO) mode = 1;
        else if (opcode == PMA_GET_ALL_IN_EDGE_CSR || opcode == PMA_GET_ALL_OUT_EDGE_CSR) mode = 2;
        else continue;

        if (mode == 0) // Single Query
        {
            int src = cmd.data.range(63,32);
            int pma_idx = src / VERTEX_PER_PMA;
            READ_COMMAND rc = {src, (int)direction, 0, mode};
            
            switch (pma_idx % 8)
            {
                case 0: worker_0_cmd.write(rc); break;
                case 1: worker_1_cmd.write(rc); break;
                case 2: worker_2_cmd.write(rc); break;
                case 3: worker_3_cmd.write(rc); break;
                case 4: worker_4_cmd.write(rc); break;
                case 5: worker_5_cmd.write(rc); break;
                case 6: worker_6_cmd.write(rc); break;
                case 7: worker_7_cmd.write(rc); break;
            }
            
            merger_cmd.write(rc);
        }
        else // Scan All
        {
            // Send global command to merger first
            READ_COMMAND global_rc = {0, (int)direction, 0, mode};
            merger_cmd.write(global_rc);

            for (int pma_idx = 0; pma_idx < MAX_PMA_NUM; pma_idx++)
            {
                int src = pma_idx * VERTEX_PER_PMA; // Base src for PMA
                READ_COMMAND worker_rc = {src, (int)direction, 0, mode};
                switch (pma_idx % 8)
                {
                    case 0: worker_0_cmd.write(worker_rc); break;
                    case 1: worker_1_cmd.write(worker_rc); break;
                    case 2: worker_2_cmd.write(worker_rc); break;
                    case 3: worker_3_cmd.write(worker_rc); break;
                    case 4: worker_4_cmd.write(worker_rc); break;
                    case 5: worker_5_cmd.write(worker_rc); break;
                    case 6: worker_6_cmd.write(worker_rc); break;
                    case 7: worker_7_cmd.write(worker_rc); break;
                }
            }
        }
    }
}

static void consume_one_packet(
    hls::stream<ap_axiu<64,0,0,0>> &in_stream, 
    hls::stream<ap_axiu<64,0,0,0>> &out_stream)
{
    #pragma HLS INLINE
    while(true)
    {
        #pragma HLS PIPELINE II=1
        ap_axiu<64,0,0,0> pkt = in_stream.read();
        if (pkt.last) break;
        out_stream.write(pkt);
    }
}

static void merger_coo(
    hls::stream<READ_COMMAND> &cmd_in,
    hls::stream<ap_axiu<64,0,0,0>> &in0,
    hls::stream<ap_axiu<64,0,0,0>> &in1,
    hls::stream<ap_axiu<64,0,0,0>> &in2,
    hls::stream<ap_axiu<64,0,0,0>> &in3,
    hls::stream<ap_axiu<64,0,0,0>> &in4,
    hls::stream<ap_axiu<64,0,0,0>> &in5,
    hls::stream<ap_axiu<64,0,0,0>> &in6,
    hls::stream<ap_axiu<64,0,0,0>> &in7,
    hls::stream<ap_axiu<64,0,0,0>> &out
)
{
    while (true)
    {
        READ_COMMAND cmd = cmd_in.read();
        if (cmd.last) break;

        if (cmd.mode == 0) // Single
        {
            int pma_idx = cmd.src_idx / VERTEX_PER_PMA;
            switch (pma_idx % 8)
            {
                case 0: consume_one_packet(in0, out); break;
                case 1: consume_one_packet(in1, out); break;
                case 2: consume_one_packet(in2, out); break;
                case 3: consume_one_packet(in3, out); break;
                case 4: consume_one_packet(in4, out); break;
                case 5: consume_one_packet(in5, out); break;
                case 6: consume_one_packet(in6, out); break;
                case 7: consume_one_packet(in7, out); break;
            }
        }
        else // Scan
        {
            for (int pma_idx = 0; pma_idx < MAX_PMA_NUM; pma_idx++)
            {
                switch (pma_idx % 8)
                {
                    case 0: consume_one_packet(in0, out); break;
                    case 1: consume_one_packet(in1, out); break;
                    case 2: consume_one_packet(in2, out); break;
                    case 3: consume_one_packet(in3, out); break;
                    case 4: consume_one_packet(in4, out); break;
                    case 5: consume_one_packet(in5, out); break;
                    case 6: consume_one_packet(in6, out); break;
                    case 7: consume_one_packet(in7, out); break;
                }
            }
        }
    }
    // Global End
    out.write({.data = (ap_uint<64>)-1, .keep = 0xFF, .strb = 0xFF, .last = 1});
}

// Simple merger for other types can be added similarly

static VEC_INFO fetch_vec_info(
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_stream,
    hls::stream<VEC_INFO_STREAM> &vec_stream,
    ap_uint<1> direction,
    int pma_idx)
{
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

static int read_edge(
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
        return 0;
    }

    ap_uint<512> segment_buffer[4];
    int csr_count = 0;
    MEMORY_INFO_ACCESS_COMMAND req_cmd = {};
    req_cmd.command_type = MEMORY_READ;
    req_cmd.start = edge_start;
    req_cmd.len = aligned_len;
    req_cmd.last = 0;
    req_to_mem_access.write(req_cmd);

    for (int seg = 0; seg < src_seg; ++seg)
    {
        #pragma HLS protocol fixed
        for (int i = 0; i < 4; ++i)
        {
            #pragma HLS PIPELINE II=1
            auto pkt = data_from_mem_access.read();
            segment_buffer[i] = pkt.data;
        }
        for (int i = 0; i < SEGMENT_SIZE; ++i)
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
            else if (mode == 1)
            {
                ap_axiu<64,0,0,0> coo_pkt = {};
                coo_pkt.data = ((ap_uint<64>)src << 32) | (ap_uint<32>)neighbor;
                coo_pkt.keep = (ap_uint<8>)0xFF;
                coo_pkt.strb = (ap_uint<8>)0xFF;
                coo_pkt.last = 0;
                coo_stream.write(coo_pkt);
            }
            else
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
    return csr_count;
}

void read_header(
    int start_addr,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &memory_access_stream,
    hls::stream<ap_axiu<512,0,0,0>> &data_from_mem_access,
    PMA_HEADER &pma_header)
{
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

void read_worker(
    hls::stream<READ_COMMAND>& cmd_stream,
    hls::stream<ap_axiu<32,0,0,0>> &data_to_nei_query,
    hls::stream<ap_axiu<64,0,0,0>> &data_to_coo_stream,
    hls::stream<ap_axiu<32,0,0,0>> &data_to_csr_col_stream,
    hls::stream<ap_axiu<32,0,0,0>> &data_to_csr_row_ptr_stream,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access_ch,
    hls::stream<VEC_INFO_STREAM> &vec_info_back_stream_ch,
    hls::stream<ap_axiu<512,0,0,0>> &data_from_mem_access_ch
)
{
    while (true)
    {
        READ_COMMAND cmd = cmd_stream.read();
        if (cmd.last) break;

        int pma_idx = cmd.src_idx / VERTEX_PER_PMA;
        VEC_INFO vec_info = fetch_vec_info(req_to_mem_access_ch, vec_info_back_stream_ch, cmd.direction, pma_idx);
        
        if (vec_info == 0) 
        {
            if (cmd.mode == 0) data_to_nei_query.write({.data=(ap_uint<32>)-1, .keep=0xF, .strb=0xF, .last=1});
            continue;
        }

        int header_start = vec_info.range(31,0); 
        PMA_HEADER pma_header;
        read_header(header_start, req_to_mem_access_ch, data_from_mem_access_ch, pma_header);

        if (cmd.mode == 0) // Single
        {
             read_edge(cmd.src_idx, pma_header, header_start, 
                data_to_nei_query, data_to_coo_stream, data_to_csr_col_stream, 
                req_to_mem_access_ch, data_from_mem_access_ch, 0);

             // Sending dummy marker to allow merger to proceed
             data_to_coo_stream.write({.data=0, .keep=0, .strb=0, .last=1});
        }
        else if (cmd.mode == 1) // COO Scan
        {
            for (int local_v = 0; local_v < VERTEX_PER_PMA; ++local_v)
            {
                int s = pma_idx * VERTEX_PER_PMA + local_v;
                read_edge(s, pma_header, header_start, 
                   data_to_nei_query, data_to_coo_stream, data_to_csr_col_stream, 
                   req_to_mem_access_ch, data_from_mem_access_ch, 1);
            }
            // Send chunk terminator for this PMA
            data_to_coo_stream.write({.data=0, .keep=0, .strb=0, .last=1});
        }
        else if (cmd.mode == 2) // CSR Scan
        {
            // Simplified: logic for CSR row pointers needs state management outside this scope or passed in
            // For now, assume simple CSR edge streaming
             for (int local_v = 0; local_v < VERTEX_PER_PMA; ++local_v)
            {
                int s = pma_idx * VERTEX_PER_PMA + local_v;
                read_edge(s, pma_header, header_start, 
                   data_to_nei_query, data_to_coo_stream, data_to_csr_col_stream, 
                   req_to_mem_access_ch, data_from_mem_access_ch, 2);
            }
            // Send chunk terminator
            data_to_coo_stream.write({.data=0, .keep=0, .strb=0, .last=1});
        }
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
        hls::stream<ap_axiu<512,0,0,0>> &data_from_mem_access_ch2,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access_ch3,
        hls::stream<VEC_INFO_STREAM> &vec_info_back_stream_ch3,
        hls::stream<ap_axiu<512,0,0,0>> &data_from_mem_access_ch3,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access_ch4,
        hls::stream<VEC_INFO_STREAM> &vec_info_back_stream_ch4,
        hls::stream<ap_axiu<512,0,0,0>> &data_from_mem_access_ch4,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access_ch5,
        hls::stream<VEC_INFO_STREAM> &vec_info_back_stream_ch5,
        hls::stream<ap_axiu<512,0,0,0>> &data_from_mem_access_ch5,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access_ch6,
        hls::stream<VEC_INFO_STREAM> &vec_info_back_stream_ch6,
        hls::stream<ap_axiu<512,0,0,0>> &data_from_mem_access_ch6,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access_ch7,
        hls::stream<VEC_INFO_STREAM> &vec_info_back_stream_ch7,
        hls::stream<ap_axiu<512,0,0,0>> &data_from_mem_access_ch7,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access_ch8,
        hls::stream<VEC_INFO_STREAM> &vec_info_back_stream_ch8,
        hls::stream<ap_axiu<512,0,0,0>> &data_from_mem_access_ch8)
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
        #pragma HLS INTERFACE axis port=req_to_mem_access_ch3
        #pragma HLS INTERFACE axis port=vec_info_back_stream_ch3
        #pragma HLS INTERFACE axis port=data_from_mem_access_ch3
        #pragma HLS INTERFACE axis port=req_to_mem_access_ch4
        #pragma HLS INTERFACE axis port=vec_info_back_stream_ch4
        #pragma HLS INTERFACE axis port=data_from_mem_access_ch4
        #pragma HLS INTERFACE axis port=req_to_mem_access_ch5
        #pragma HLS INTERFACE axis port=vec_info_back_stream_ch5
        #pragma HLS INTERFACE axis port=data_from_mem_access_ch5
        #pragma HLS INTERFACE axis port=req_to_mem_access_ch6
        #pragma HLS INTERFACE axis port=vec_info_back_stream_ch6
        #pragma HLS INTERFACE axis port=data_from_mem_access_ch6
        #pragma HLS INTERFACE axis port=req_to_mem_access_ch7
        #pragma HLS INTERFACE axis port=vec_info_back_stream_ch7
        #pragma HLS INTERFACE axis port=data_from_mem_access_ch7
        #pragma HLS INTERFACE axis port=req_to_mem_access_ch8
        #pragma HLS INTERFACE axis port=vec_info_back_stream_ch8
        #pragma HLS INTERFACE axis port=data_from_mem_access_ch8

        hls::stream<READ_COMMAND> worker_cmd_0;
        hls::stream<READ_COMMAND> worker_cmd_1;
        hls::stream<READ_COMMAND> worker_cmd_2;
        hls::stream<READ_COMMAND> worker_cmd_3;
        hls::stream<READ_COMMAND> worker_cmd_4;
        hls::stream<READ_COMMAND> worker_cmd_5;
        hls::stream<READ_COMMAND> worker_cmd_6;
        hls::stream<READ_COMMAND> worker_cmd_7;
        
        #pragma HLS STREAM variable=worker_cmd_0 depth=16
        #pragma HLS STREAM variable=worker_cmd_1 depth=16
        #pragma HLS STREAM variable=worker_cmd_2 depth=16
        #pragma HLS STREAM variable=worker_cmd_3 depth=16
        #pragma HLS STREAM variable=worker_cmd_4 depth=16
        #pragma HLS STREAM variable=worker_cmd_5 depth=16
        #pragma HLS STREAM variable=worker_cmd_6 depth=16
        #pragma HLS STREAM variable=worker_cmd_7 depth=16
        
        hls::stream<READ_COMMAND> merger_cmd;
        #pragma HLS STREAM variable=merger_cmd depth=64

        hls::stream<ap_axiu<64,0,0,0>> coo_stream_0;
        hls::stream<ap_axiu<64,0,0,0>> coo_stream_1;
        hls::stream<ap_axiu<64,0,0,0>> coo_stream_2;
        hls::stream<ap_axiu<64,0,0,0>> coo_stream_3;
        hls::stream<ap_axiu<64,0,0,0>> coo_stream_4;
        hls::stream<ap_axiu<64,0,0,0>> coo_stream_5;
        hls::stream<ap_axiu<64,0,0,0>> coo_stream_6;
        hls::stream<ap_axiu<64,0,0,0>> coo_stream_7;
        
        #pragma HLS STREAM variable=coo_stream_0 depth=128
        #pragma HLS STREAM variable=coo_stream_1 depth=128
        #pragma HLS STREAM variable=coo_stream_2 depth=128
        #pragma HLS STREAM variable=coo_stream_3 depth=128
        #pragma HLS STREAM variable=coo_stream_4 depth=128
        #pragma HLS STREAM variable=coo_stream_5 depth=128
        #pragma HLS STREAM variable=coo_stream_6 depth=128
        #pragma HLS STREAM variable=coo_stream_7 depth=128
        
        // Dummy streams for unused outputs in read_worker
        hls::stream<ap_axiu<32,0,0,0>> d_nei_0, d_col_0, d_row_0;
        hls::stream<ap_axiu<32,0,0,0>> d_nei_1, d_col_1, d_row_1;
        hls::stream<ap_axiu<32,0,0,0>> d_nei_2, d_col_2, d_row_2;
        hls::stream<ap_axiu<32,0,0,0>> d_nei_3, d_col_3, d_row_3;
        hls::stream<ap_axiu<32,0,0,0>> d_nei_4, d_col_4, d_row_4;
        hls::stream<ap_axiu<32,0,0,0>> d_nei_5, d_col_5, d_row_5;
        hls::stream<ap_axiu<32,0,0,0>> d_nei_6, d_col_6, d_row_6;
        hls::stream<ap_axiu<32,0,0,0>> d_nei_7, d_col_7, d_row_7;

        #pragma HLS DATAFLOW
        distributor(cmd_stream, 
            worker_cmd_0, worker_cmd_1, worker_cmd_2, worker_cmd_3,
            worker_cmd_4, worker_cmd_5, worker_cmd_6, worker_cmd_7, 
            merger_cmd);
            
        read_worker(worker_cmd_0, d_nei_0, coo_stream_0, d_col_0, d_row_0, req_to_mem_access_ch1, vec_info_back_stream_ch1, data_from_mem_access_ch1);
        read_worker(worker_cmd_1, d_nei_1, coo_stream_1, d_col_1, d_row_1, req_to_mem_access_ch2, vec_info_back_stream_ch2, data_from_mem_access_ch2);
        read_worker(worker_cmd_2, d_nei_2, coo_stream_2, d_col_2, d_row_2, req_to_mem_access_ch3, vec_info_back_stream_ch3, data_from_mem_access_ch3);
        read_worker(worker_cmd_3, d_nei_3, coo_stream_3, d_col_3, d_row_3, req_to_mem_access_ch4, vec_info_back_stream_ch4, data_from_mem_access_ch4);
        read_worker(worker_cmd_4, d_nei_4, coo_stream_4, d_col_4, d_row_4, req_to_mem_access_ch5, vec_info_back_stream_ch5, data_from_mem_access_ch5);
        read_worker(worker_cmd_5, d_nei_5, coo_stream_5, d_col_5, d_row_5, req_to_mem_access_ch6, vec_info_back_stream_ch6, data_from_mem_access_ch6);
        read_worker(worker_cmd_6, d_nei_6, coo_stream_6, d_col_6, d_row_6, req_to_mem_access_ch7, vec_info_back_stream_ch7, data_from_mem_access_ch7);
        read_worker(worker_cmd_7, d_nei_7, coo_stream_7, d_col_7, d_row_7, req_to_mem_access_ch8, vec_info_back_stream_ch8, data_from_mem_access_ch8);
        
        merger_coo(merger_cmd, 
            coo_stream_0, coo_stream_1, coo_stream_2, coo_stream_3,
            coo_stream_4, coo_stream_5, coo_stream_6, coo_stream_7, 
            data_to_coo_stream);
    }
}