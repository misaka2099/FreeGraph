#include <cstdio>
#include <cstring>
#include <algorithm>
#include <hls_stream.h>
#include <ap_int.h>
#include "data_type.h"
#include "debug.h"
#include "etc/autopilot_ssdm_op.h"

template<int D>
struct internal_axis {
    ap_uint<D> data;
    ap_uint<D/8> keep;
    ap_uint<D/8> find_end;
    ap_uint<1> last;
};

struct READ_COMMAND 
{
    int src_idx;
    int end_src_idx;
    int last;
    int mode;
};

static const int READ_WORKERS = 2;

void read_header(
    // ap_uint<512>* hbm_ptr,
    int pma_header_addr_in_byte,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &memory_access_stream,
    hls::stream<ap_axiu<512,0,0,0>> &data_from_mem,
    PMA_HEADER& pma_header
)
{
    #pragma HLS INLINE
    ap_uint<512> buffer[4];
    MEMORY_INFO_ACCESS_COMMAND command = {};
    command.start = pma_header_addr_in_byte;
    command.pma_idx = 0;
    command.len = 256;
    command.command_type = MEMORY_READ;
    command.last = 0;
    memory_access_stream.write(command);
    #pragma HLS protocol fixed
    for (int i = 0 ; i < 4 ; i++)
    {
        #pragma HLS PIPELINE II=1
        buffer[i] = data_from_mem.read().data;
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


void read_seg(
    // ap_uint<512>* hbm_ptr,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &memory_access_stream,
    hls::stream<ap_axiu<512,0,0,0>> &data_from_mem,
    int segment_buffer[SEGMENT_SIZE],
    int pma_data_base_addr,
    int seg_number
)
{
    #pragma HLS INLINE
    // ap_uint<512>* segment_buffer_ptr = (ap_uint<512>*)(segment_buffer);
    ap_uint<512> buffer[4];
    MEMORY_INFO_ACCESS_COMMAND command = {};
    
    command.start = seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr;;
    command.len =  SEGMENT_SIZE * sizeof(int);
    command.command_type = MEMORY_READ;
    command.last = 0;
    memory_access_stream.write(command);
    #pragma HLS protocol fixed
    for (int i = 0 ; i < 4 ; i++)
    {
        #pragma HLS PIPELINE II=1
        buffer[i] = data_from_mem.read().data;
    }
    for (int i = 0; i < 4; i++) {
        #pragma HLS UNROLL
        for (int j = 0; j < 16; j++) {
            #pragma HLS UNROLL
            segment_buffer[i * 16 + j] = buffer[i].range(32 * (j + 1) - 1, 32 * j);
        }
    }    
}
static void distributor(
    hls::stream<READ_COMMAND_STREAM> &cmd_in,
    hls::stream<READ_COMMAND> &worker_0_cmd,
    hls::stream<READ_COMMAND> &worker_1_cmd,
    hls::stream<READ_COMMAND> &merger_cmd
)
{
    while (true)
    {
        READ_COMMAND_STREAM cmd = cmd_in.read();
        
        if (cmd.data == PMA_STOP && cmd.last == 1)
        {
            // READ_COMMAND stop_cmd = {0, 1, 0};
            READ_COMMAND stop_cmd = {};
            stop_cmd.last = 1;
            stop_cmd.mode = 0;
            stop_cmd.src_idx = 0;
            stop_cmd.end_src_idx = 0;
            worker_0_cmd.write(stop_cmd);
            worker_1_cmd.write(stop_cmd);
            merger_cmd.write(stop_cmd);
            break;
        }

        ap_uint<32> opcode = cmd.data.range(31,0);
        #pragma HLS protocol fixed
        ap_wait();
        if (opcode == PMA_GET_OUT_EDGE) // Single Query
        {
            int src = cmd.data.range(63,32);
            int pma_idx = src / VERTEX_PER_PMA;
            // READ_COMMAND rc = {src, 0, mode};
            READ_COMMAND rc = {};
            rc.src_idx = src;
            rc.end_src_idx = 0;
            rc.mode = opcode;
            rc.last = 0;
            
            // switch (pma_idx % READ_WORKERS)
            // {
            worker_0_cmd.write(rc); 
            worker_1_cmd.write(rc);
            
            merger_cmd.write(rc);
        }
        else if (opcode == PMA_GET_ALL_OUT_EDGE_COO || opcode == PMA_GET_ALL_OUT_EDGE_CSR) // Scan All
        {
            // Send global command to merger first
            // READ_COMMAND global_rc = {0, 0, mode};
            // READ_COMMAND global_rc = {};
            // global_rc.src_idx = 0;
            // global_rc.end_src_idx = 0;
            // global_rc.mode = mode;
            // global_rc.last = 0;
            // merger_cmd.write(global_rc);

            // for (int pma_idx = 0; pma_idx < MAX_PMA_NUM; pma_idx++)
            // {
            //     int src = pma_idx * VERTEX_PER_PMA; // Base src for PMA
            //     // READ_COMMAND worker_rc = {src, 0, mode};
            //     READ_COMMAND worker_rc = {};
            //     worker_rc.src_idx = src;
            //     worker_rc.end_src_idx = 0;
            //     worker_rc.mode = mode;
            //     worker_rc.last = 0;
            //     switch (pma_idx % READ_WORKERS)
            //     {
            //         case 0: worker_0_cmd.write(worker_rc); break;
            //         case 1: worker_1_cmd.write(worker_rc); break;
            //     }
            // }
            READ_COMMAND rc = {};
            rc.mode = opcode;
            rc.last = 0;
            // send one command to each worker
            worker_0_cmd.write(rc);
            worker_1_cmd.write(rc);

            // merger also receives one command
            merger_cmd.write(rc);
        }
        else if (opcode == PMA_GET_EDGE_COUNT_RANGE) // Edge count for a vertex id range
        {
            // Read second packet from cmd stream which carries the end vertex (host must send two cmds)
            
            // ap_uint<32> start_v = cmd.data.range(63,32);
            // ap_uint<32> end_v = cmd.data.range(95,64);

            // READ_COMMAND global_rc = {};
            // global_rc.src_idx = start_v;
            // global_rc.end_src_idx = end_v;
            // global_rc.mode = mode;
            // global_rc.last = 0;
            // merger_cmd.write(global_rc);

            // int pma_start = start_v / VERTEX_PER_PMA;
            // int pma_end = end_v / VERTEX_PER_PMA;
            // for (int pma_idx = pma_start; pma_idx <= pma_end; ++pma_idx) {
            //     int chunk_start = (pma_idx == pma_start) ? (int)start_v : (pma_idx * VERTEX_PER_PMA);
            //     int chunk_end = (pma_idx == pma_end) ? (int)end_v : (pma_idx * VERTEX_PER_PMA + VERTEX_PER_PMA - 1);
            //     READ_COMMAND worker_rc = {};
            //     worker_rc.src_idx = chunk_start;
            //     worker_rc.end_src_idx = chunk_end;
            //     worker_rc.mode = mode;
            //     worker_rc.last = 0;
            //     switch (pma_idx % READ_WORKERS)
            //     {
            //         case 0: worker_0_cmd.write(worker_rc); break;
            //         case 1: worker_1_cmd.write(worker_rc); break;
            //     }
            // }
            ap_uint<32> start_v = cmd.data.range(63,32);
            ap_uint<32> end_v = cmd.data.range(95,64);

            READ_COMMAND rc = {};
            rc.src_idx = start_v;
            rc.end_src_idx = end_v;
            rc.mode = opcode;
            rc.last = 0;
            // send same command to both workers
            worker_0_cmd.write(rc);
            worker_1_cmd.write(rc);
            merger_cmd.write(rc);
        }
        else
        {
            continue;
        }
    }
}

// static inline void emit_count_packet(hls::stream<ap_axiu<32,0,0,0>> &out, ap_uint<32> count)
// {
//     ap_axiu<32,0,0,0> pkt = {};
//     pkt.data = count;
//     pkt.keep = 0xF;
//     pkt.strb = 0xF;
//     pkt.last = 1;
//     out.write(pkt);
// }


static void merger_unified(
    hls::stream<READ_COMMAND> &cmd_in,
    hls::stream<internal_axis<64>> &in0,
    hls::stream<internal_axis<64>> &in1,
    // hls::stream<ap_axiu<32,0,0,0>> &out_nei,
    hls::stream<ap_axiu<64,0,0,0>> &out_coo,
    hls::stream<ap_axiu<32,0,0,0>> &out_edge_count
    // hls::stream<ap_axiu<32,0,0,0>> &out_csr_col,
    // hls::stream<ap_axiu<32,0,0,0>> &out_csr_row
)
{
    while (true)
    {
        READ_COMMAND cmd = cmd_in.read();
        if (cmd.last) break;

        int total_edge_count = 0;
        bool worker_done[READ_WORKERS];
        #pragma HLS ARRAY_PARTITION variable=worker_done complete
        worker_done[0] = false;
        worker_done[1] = false;

        int worker_idx = 0;

        bool count_end = false;
        while (!(worker_done[0] && worker_done[1]))
        {
            #pragma HLS PIPELINE II=1
            internal_axis<64> pkt;

            if (worker_idx == 0)
            {
                if (worker_done[0]) {
                    worker_idx = 1;
                    continue;
                }
                pkt = in0.read();
            }
            else
            {
                if (worker_done[1]) {
                    worker_idx = 0;
                    continue;
                }
                pkt = in1.read();
            }


            bool is_last = pkt.last;

            // terminator packet
            if (pkt.keep == 0 && is_last)
            {
                worker_done[worker_idx] = true;
                worker_idx ^= 1;
                continue;
            }

            //last pkt dose not carry data, just control info, so we check last before processing data
            else if (pkt.keep != 0 && is_last)
            {
                worker_idx ^= 1;
                continue;
            }


            else if (pkt.keep != 0)
            {
                if (cmd.mode == PMA_GET_ALL_OUT_EDGE_COO)   // COO
                {
                    ap_axiu<64,0,0,0> o = {};
                    o.data = pkt.data;
                    o.keep = 0xFF;
                    o.strb = 0xFF;
                    o.last = 0;

                    out_coo.write(o);
                }
                else if (cmd.mode == PMA_GET_EDGE_COUNT_RANGE)  // edge count
                {
                    total_edge_count += pkt.data;
                    if (pkt.find_end == 0)
                    {
                        count_end = true;
                    }
                }
            }

        }

        // Append Terminator for Scan modes where workers just send chunks
        if (cmd.mode == PMA_GET_ALL_OUT_EDGE_COO) // COO
        {
            out_coo.write({.data = (ap_uint<64>)-1, .keep = 0xFF, .strb = 0xFF, .last = 1});
        }
        if (cmd.mode == PMA_GET_EDGE_COUNT_RANGE)
        {
            ap_axiu<32,0,0,0> o = {};
            o.data = total_edge_count;
            o.keep = 0xFF;
            o.strb = 0xFF;
            o.last = count_end;
            out_edge_count.write(o);
        }
    }
}

template<int WORKER_ID>
static void read_worker(
    hls::stream<READ_COMMAND>& cmd_stream,
    hls::stream<internal_axis<64>> &unified_out_stream,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access_ch,
    // hls::stream<VEC_INFO_STREAM> &vec_info_back_stream_ch,
    hls::stream<ap_axiu<512,0,0,0>> &data_from_mem_access_ch
)
{
    while (true)
    {
        READ_COMMAND cmd = cmd_stream.read();
        if (cmd.last) break;

        // Dummy packets for control
        internal_axis<64> pkt_finish = {};
        pkt_finish.data=0; pkt_finish.keep=0; pkt_finish.find_end=0; pkt_finish.last=1;
        internal_axis<64> pkt_separator = {};
        pkt_separator.data = 0;
        pkt_separator.keep = 0xff;
        pkt_separator.find_end = 0;
        pkt_separator.last = 1;
        switch (cmd.mode) // Single
        {
            case (PMA_GET_OUT_EDGE):
            {
                int src = cmd.src_idx;
                int local_src = src % VERTEX_PER_PMA;
                int pma_idx = src / VERTEX_PER_PMA;
                if (pma_idx % READ_WORKERS != WORKER_ID) {
                    unified_out_stream.write(pkt_finish);
                    break;
                }
                VEC_INFO vec_info;
                MEMORY_INFO_ACCESS_COMMAND access_command = {};
                access_command.command_type = VEC_INFO_READ;
                access_command.pma_idx = pma_idx;
                access_command.last = 0;
                req_to_mem_access_ch.write(access_command);
                #pragma HLS protocol fixed
                ap_wait();
                vec_info = data_from_mem_access_ch.read().data.range(63,0);       
                if (vec_info == 0)
                {
                    unified_out_stream.write(pkt_separator);
                    unified_out_stream.write(pkt_finish);
                    break;
                }
                
                int header_start = vec_info.range(31,0);

                PMA_HEADER pma_header;
                read_header(header_start,
                            req_to_mem_access_ch,
                            data_from_mem_access_ch,
                            pma_header);
                int vertex_range_start = local_src == 0 ? 0 : pma_header.vertex_range[local_src - 1];
                int vertex_range_end = pma_header.vertex_range[local_src];
                int segment_buffer[SEGMENT_SIZE]; 
                int pma_data_base_addr = header_start + sizeof(PMA_HEADER);
                for (int i = vertex_range_start; i < vertex_range_end; ++i)
                {
                    read_seg(
                        req_to_mem_access_ch,
                        data_from_mem_access_ch,
                        segment_buffer,
                        pma_data_base_addr,
                        i
                    );
                    for (int j = 0; j < SEGMENT_SIZE; ++j)
                    {
                        #pragma HLS PIPELINE II=1
                        int neighbor = segment_buffer[j];
                        if (neighbor == UNUSED_ICON)
                        {
                            break;
                        }
                        internal_axis<64> out_pkt = {};
                        out_pkt.data = (ap_uint<32>)neighbor;
                        out_pkt.keep = (ap_uint<8>)0xFF;
                        out_pkt.find_end = (ap_uint<8>)0xFF;
                        out_pkt.last = 0;
                        unified_out_stream.write(out_pkt);
                    }
                }
                unified_out_stream.write(pkt_separator);
                unified_out_stream.write(pkt_finish);
                break;
            }
            case PMA_GET_ALL_OUT_EDGE_CSR:
            case PMA_GET_ALL_OUT_EDGE_COO:
            {
                int pma_idx = WORKER_ID;

                while (true)
                {
                    VEC_INFO vec_info;
                    MEMORY_INFO_ACCESS_COMMAND access_command = {};
                    access_command.command_type = VEC_INFO_READ;
                    access_command.pma_idx = pma_idx;
                    access_command.last = 0;
                    req_to_mem_access_ch.write(access_command);
                    #pragma HLS protocol fixed
                    ap_wait();
                    vec_info = data_from_mem_access_ch.read().data.range(63,0);    
                    if (vec_info == 0)
                    {
                        break;
                    }
                    int header_start = vec_info.range(31,0);

                    PMA_HEADER pma_header;

                    read_header(header_start,
                                req_to_mem_access_ch,
                                data_from_mem_access_ch,
                                pma_header);
                    int segment_buffer[SEGMENT_SIZE]; 
                    for (int local_v = 0;
                        local_v < VERTEX_PER_PMA;
                        ++local_v)
                    {
                        int vertex_range_start = local_v == 0 ? 0 : pma_header.vertex_range[local_v - 1];
                        int vertex_range_end = pma_header.vertex_range[local_v];
                        int src = pma_idx * VERTEX_PER_PMA + local_v;
                        int pma_data_base_addr = header_start + sizeof(PMA_HEADER);
                        for (int i = vertex_range_start; i < vertex_range_end; ++i)
                        {
                            read_seg(
                                req_to_mem_access_ch,
                                data_from_mem_access_ch,
                                segment_buffer,
                                pma_data_base_addr,
                                i
                            );
                            for (int j = 0; j < SEGMENT_SIZE; ++j)
                            {
                                #pragma HLS PIPELINE II=1
                                int neighbor = segment_buffer[j];
                                if (neighbor == UNUSED_ICON)
                                {
                                    break;
                                }
                                internal_axis<64> out_pkt = {};
                                out_pkt.data = ((ap_uint<64>)src << 32) | (ap_uint<32>)neighbor;
                                out_pkt.keep = (ap_uint<8>)0xFF;
                                out_pkt.find_end = (ap_uint<8>)0xFF;
                                out_pkt.last = 0;
                                unified_out_stream.write(out_pkt);
                            }
                        }
                    }

                    unified_out_stream.write(pkt_separator);

                    pma_idx += READ_WORKERS;
                }

                unified_out_stream.write(pkt_finish);
                break;
            }
            case PMA_GET_EDGE_COUNT_RANGE:
            {
                int pma_idx = WORKER_ID;

                int total_sum = 0;
                int start_v = cmd.src_idx;
                int end_v   = cmd.end_src_idx;

                int pma_start = start_v / VERTEX_PER_PMA;
                int pma_end   = end_v   / VERTEX_PER_PMA;
                bool find_empty = false;
                for (int idx = pma_idx ; idx <= pma_end; idx += READ_WORKERS)
                {
                    if (idx < pma_start) {
                        continue;
                    }
                    VEC_INFO vec_info;
                    MEMORY_INFO_ACCESS_COMMAND access_command = {};
                    access_command.command_type = VEC_INFO_READ;
                    access_command.pma_idx = idx;
                    access_command.last = 0;
                    req_to_mem_access_ch.write(access_command);
                    #pragma HLS protocol fixed
                    ap_wait();
                    vec_info = data_from_mem_access_ch.read().data.range(63,0);   
                    if (vec_info == 0)
                    {
                        find_empty = true;
                        break;
                    }
                    int header_start = vec_info.range(31,0);

                    
                    PMA_HEADER pma_header;

                    read_header(header_start,
                        req_to_mem_access_ch,
                        data_from_mem_access_ch,
                        pma_header
                    );
                    int local_start = (idx == pma_start) ? (start_v % VERTEX_PER_PMA) : 0;
                    int local_end   = (idx == pma_end)   ? (end_v   % VERTEX_PER_PMA) : (VERTEX_PER_PMA - 1);

                    for (int i = local_start; i <= local_end; ++i)
                    {
                        total_sum += pma_header.edge_count[i];
                    }
                }

                internal_axis<64> cnt_pkt = {};
                cnt_pkt.data = total_sum;
                cnt_pkt.keep = 0xFF;
                cnt_pkt.find_end = find_empty ? 0 : 0xFF; // If we find an empty PMA, it means all subsequent PMAs are also empty, so we can set keep to 0 to indicate the count is not valid
                cnt_pkt.last = 0;

                unified_out_stream.write(cnt_pkt);

                unified_out_stream.write(pkt_finish);
                break;
            }
        }
        
    }
}

extern "C"
{
    void pma_read_kernel(
        hls::stream<READ_COMMAND_STREAM> &cmd_stream,
        // hls::stream<ap_axiu<32,0,0,0>> &data_to_nei_query,
        hls::stream<ap_axiu<64,0,0,0>> &data_to_coo_stream,
        // hls::stream<ap_axiu<32,0,0,0>> &data_to_csr_col_stream,
        // hls::stream<ap_axiu<32,0,0,0>> &data_to_csr_row_ptr_stream,
        hls::stream<ap_axiu<32,0,0,0>> &coo_edge_count_stream,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access_ch1,
        // hls::stream<VEC_INFO_STREAM> &vec_info_back_stream_ch1,
        hls::stream<ap_axiu<512,0,0,0>> &data_from_mem_access_ch1,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access_ch2,
        // hls::stream<VEC_INFO_STREAM> &vec_info_back_stream_ch2,
        hls::stream<ap_axiu<512,0,0,0>> &data_from_mem_access_ch2
    )
    {
        #pragma HLS INTERFACE axis port=cmd_stream
        // #pragma HLS INTERFACE axis port=data_to_nei_query
        #pragma HLS INTERFACE axis port=data_to_coo_stream
        // #pragma HLS INTERFACE axis port=data_to_csr_col_stream
        // #pragma HLS INTERFACE axis port=data_to_csr_row_ptr_stream
        #pragma HLS INTERFACE axis port=coo_edge_count_stream
        #pragma HLS INTERFACE axis port=req_to_mem_access_ch1
        // #pragma HLS INTERFACE axis port=vec_info_back_stream_ch1
        #pragma HLS INTERFACE axis port=data_from_mem_access_ch1
        #pragma HLS INTERFACE axis port=req_to_mem_access_ch2
        // #pragma HLS INTERFACE axis port=vec_info_back_stream_ch2
        #pragma HLS INTERFACE axis port=data_from_mem_access_ch2

        #pragma HLS STREAM variable=cmd_stream depth=16
        #pragma HLS STREAM variable=data_to_coo_stream depth=16
        #pragma HLS STREAM variable=coo_edge_count_stream depth=16
        #pragma HLS STREAM variable=req_to_mem_access_ch1 depth=16
        // #pragma HLS STREAM variable=vec_info_back_stream_ch1 depth=16
        #pragma HLS STREAM variable=data_from_mem_access_ch1 depth=16
        #pragma HLS STREAM variable=req_to_mem_access_ch2 depth=16
        // #pragma HLS STREAM variable=vec_info_back_stream_ch2 depth=16
        #pragma HLS STREAM variable=data_from_mem_access_ch2 depth=16
        hls::stream<READ_COMMAND> worker_cmd_0;
        hls::stream<READ_COMMAND> worker_cmd_1;
        
        #pragma HLS STREAM variable=worker_cmd_0 depth=16
        #pragma HLS STREAM variable=worker_cmd_1 depth=16
        
        hls::stream<READ_COMMAND> merger_cmd;
        #pragma HLS STREAM variable=merger_cmd depth=64
        

        // Unified 64-bit internal streams
        hls::stream<internal_axis<64>> u_stream_0;
        hls::stream<internal_axis<64>> u_stream_1;
        
        #pragma HLS STREAM variable=u_stream_0 depth=128
        #pragma HLS STREAM variable=u_stream_1 depth=128

        #pragma HLS DATAFLOW
        distributor(cmd_stream, 
            worker_cmd_0, worker_cmd_1,
            merger_cmd);
            
        read_worker<0>(worker_cmd_0, u_stream_0, req_to_mem_access_ch1, data_from_mem_access_ch1);
        read_worker<1>(worker_cmd_1, u_stream_1, req_to_mem_access_ch2, data_from_mem_access_ch2);
        
        merger_unified(merger_cmd, 
            u_stream_0, u_stream_1,
            // data_to_nei_query,
            data_to_coo_stream,
            coo_edge_count_stream
            // data_to_csr_col_stream,
            // data_to_csr_row_ptr_stream
        );
    }
}