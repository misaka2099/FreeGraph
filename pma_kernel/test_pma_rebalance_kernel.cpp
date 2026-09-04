#include "data_type.h"
#include <cstdio>
#include <hls_stream.h>
#include <ap_utils.h>
#include "etc/autopilot_ssdm_op.h"

static inline int valid_count(
    int seg_start,
    int seg_end,
    hls::stream<memory_access_command> &memory_access_stream,
    hls::stream<ap_uint<512>> &data_from_mem,
    int pma_data_base_addr)
{
    #pragma HLS INLINE
    int total_segs = seg_end - seg_start;
    if (total_segs <= 0) return 0;

    memory_access_command command;
    command.addr_in_byte_patten = seg_start * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr;
    command.len_in_byte_patten = total_segs * SEGMENT_SIZE * sizeof(int);
    command.type = READ;
    memory_access_stream.write(command);

    int count = 0;
    int total_words = total_segs * 4;
    for (int i = 0; i < total_words; i++)
    {
        #pragma HLS PIPELINE II=1
        ap_uint<512> val = data_from_mem.read();
        for (int j = 0; j < 16; j++)
        {
            #pragma HLS UNROLL
            int item = val.range(32 * (j + 1) - 1, 32 * j);
            if (item != UNUSED_ICON) count++;
        }
    }
    return count;
}

static inline void redistribute(
    int read_seg_start,
    int read_seg_end,
    int write_seg_start,
    int write_seg_end,
    hls::stream<memory_access_command> &memory_access_stream,
    hls::stream<ap_uint<512>> &data_to_mem,
    hls::stream<ap_uint<512>> &data_from_mem,
    int read_pma_data_base_addr,
    int write_pma_data_base_addr,
    int total_valid_count)
{
    int total_segs = write_seg_end - write_seg_start;
    int base_per_seg = total_valid_count / total_segs;
    int extra = total_valid_count % total_segs;

    int current_write_fill = 0;
    int read_seg_idx = read_seg_start;
    int write_seg_idx = write_seg_start;

    int read_buffer[SEGMENT_SIZE];
    int write_buffer[SEGMENT_SIZE];
    #pragma HLS BIND_STORAGE variable=read_buffer type=RAM_2P impl=BRAM
    #pragma HLS BIND_STORAGE variable=write_buffer type=RAM_2P impl=BRAM

    int read_ptr = SEGMENT_SIZE;

    for (int k = 0; k < SEGMENT_SIZE; k++) write_buffer[k] = -1;

    while (write_seg_idx < write_seg_end)
    {
        int target_fill = base_per_seg + ((write_seg_idx - write_seg_start) < extra ? 1 : 0);

        while (current_write_fill < target_fill)
        {
            if (read_ptr == SEGMENT_SIZE)
            {
                if (read_seg_idx < read_seg_end)
                {
                    read_seg(memory_access_stream, data_from_mem, read_buffer, read_pma_data_base_addr, read_seg_idx++);
                    read_ptr = 0;
                }
                else
                {
                    break;
                }
            }

            if (read_buffer[read_ptr] != UNUSED_ICON)
            {
                write_buffer[current_write_fill++] = read_buffer[read_ptr];
            }
            read_ptr++;
        }

        write_seg(memory_access_stream, data_to_mem, data_from_mem, write_buffer, write_pma_data_base_addr, write_seg_idx++);

        current_write_fill = 0;
        for (int k = 0; k < SEGMENT_SIZE; k++) write_buffer[k] = -1;
    }
}

static void rebalance_cu(
    hls::stream<memory_access_command> &memory_access_stream,
    hls::stream<ap_uint<512>> &data_to_mem,
    hls::stream<ap_uint<512>> &data_from_mem,
    hls::stream<PMA_HEADER> &old_pma_header_in,
    hls::stream<PMA_HEADER> &pma_header_in,
    hls::stream<VEC_INFO> &old_pma_addr_in,
    hls::stream<VEC_INFO> &pma_addr_in,
    hls::stream<pma_idx_info> &pma_idx,
    hls::stream<inserted_seg> &inserted_seg_info,
    hls::stream<COMMAND_STREAM> &ack_back_to_router,
    hls::stream<MEMORY_ALLOC_FREE_REQUEST_INTERNAL> &free_request_stream)
{
    printf("REBALANCE\n");
    while (true)
    {
        auto signal = pma_idx.read();
        if (signal.last == 1)
        {
            memory_access_command x;
            x.addr_in_byte_patten = 0;
            x.len_in_byte_patten = 0;
            x.type = FINISH;
            memory_access_stream.write(x);
            data_to_mem.write(0);
            MEMORY_ALLOC_FREE_REQUEST_INTERNAL finish_req = 0;
            finish_req.range(67, 64) = REQ_TYPE_FINISH;
            free_request_stream.write(finish_req);
            break;
        }

        PMA_HEADER pma_header = pma_header_in.read();
        PMA_HEADER old_pma_header = old_pma_header_in.read();

        VEC_INFO pma_addr = pma_addr_in.read();
        VEC_INFO old_pma_addr = old_pma_addr_in.read();

        auto to_check = inserted_seg_info.read();

        const int pma_data_base_addr = pma_addr.range(31, 0) + sizeof(PMA_HEADER);
        const int old_pma_data_base_addr = old_pma_addr.range(31, 0) + sizeof(PMA_HEADER);

        if (pma_data_base_addr != old_pma_data_base_addr)
        {
            for (int v = 0; v < VERTEX_PER_PMA; v++)
            {
                int old_seg_start = (v == 0) ? 0 : old_pma_header.vertex_range[v - 1];
                int old_seg_end = old_pma_header.vertex_range[v];
                int new_seg_start = (v == 0) ? 0 : pma_header.vertex_range[v - 1];
                int new_seg_end = pma_header.vertex_range[v];

                int total_valid_count = valid_count(old_seg_start, old_seg_end,
                                                    memory_access_stream, data_from_mem,
                                                    old_pma_data_base_addr);

                redistribute(old_seg_start, old_seg_end, new_seg_start, new_seg_end,
                             memory_access_stream, data_to_mem, data_from_mem,
                             old_pma_data_base_addr, pma_data_base_addr, total_valid_count);
            }
            MEMORY_ALLOC_FREE_REQUEST_INTERNAL free_command = 0;
            free_command.range(67, 64) = REQ_TYPE_FREE;
            free_command.range(63, 0) = old_pma_addr.range(63, 0);
            free_command.range(64, 64) = 1;
            free_request_stream.write(free_command);
        }
        else
        {
            for (int i = 0; i < 16; i++)
            {
                if (to_check.vertex_number[i] == -1) continue;

                int v = to_check.vertex_number[i];
                int relative_seg_num = to_check.old_vertex_relative_seg_number[i];

                int seg_start = (v == 0) ? 0 : pma_header.vertex_range[v - 1];
                int seg_end = pma_header.vertex_range[v];
                int seg_num = seg_end - seg_start;
                int total_valid_count = 0;
                int seg_idx = relative_seg_num;
                total_valid_count = valid_count(seg_idx + seg_start,
                                                seg_idx + seg_start + 1,
                                                memory_access_stream, data_from_mem,
                                                pma_data_base_addr);
                int last_left = seg_idx + seg_start;
                int last_right = seg_idx + seg_start + 1;

                int max_edge = SEGMENT_SIZE * DENSITY_BOUND;
                if (total_valid_count < max_edge) continue;
                for (int cur_seg_num = 2; cur_seg_num <= seg_num; cur_seg_num *= 2)
                {
                    int left = seg_start + int(seg_idx / cur_seg_num) * cur_seg_num;
                    int right = seg_start + (int(seg_idx / cur_seg_num) + 1) * cur_seg_num;
                    if (left < last_left)
                    {
                        total_valid_count += valid_count(left, last_left,
                                                         memory_access_stream, data_from_mem,
                                                         pma_data_base_addr);
                    }
                    else if (right > last_right)
                    {
                        total_valid_count += valid_count(last_right, right,
                                                         memory_access_stream, data_from_mem,
                                                         pma_data_base_addr);
                    }

                    max_edge = (right - left) * SEGMENT_SIZE * DENSITY_BOUND;
                    if (total_valid_count <= max_edge)
                    {
                        redistribute(left, right, left, right,
                                     memory_access_stream, data_to_mem, data_from_mem,
                                     pma_data_base_addr, pma_data_base_addr, total_valid_count);
                        break;
                    }
                    last_left = left;
                    last_right = right;
                }
            }
        }

        ack_back_to_router.write({.data = signal.pma_idx});
    }
}

extern "C"
{
    void pma_rebalance_kernel(
        hls::stream<PMA_HEADER> &pma_header_in,
        hls::stream<VEC_INFO> &pma_addr_in,
        hls::stream<inserted_seg> &inserted_seg_info,
        hls::stream<pma_idx_info> &pma_idx_in,
        hls::stream<MEMORY_ALLOC_FREE_REQUEST_INTERNAL> &alloc_request_stream,
        hls::stream<MEMORY_ALLOC_FREE_REQUEST> &req_to_mem_alloc,
        hls::stream<VEC_INFO_STREAM> &vec_info_back_stream,
        hls::stream<COMMAND_STREAM> &ack_back_to_router,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_mem_access,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_mem_access,
        hls::stream<VEC_INFO_STREAM> &vec_info_from_mem_access)
    {
        #pragma HLS INTERFACE axis port=pma_header_in
        #pragma HLS INTERFACE axis port=pma_addr_in
        #pragma HLS INTERFACE axis port=inserted_seg_info
        #pragma HLS INTERFACE axis port=pma_idx_in
        #pragma HLS INTERFACE axis port=alloc_request_stream
        #pragma HLS INTERFACE axis port=req_to_mem_alloc
        #pragma HLS INTERFACE axis port=vec_info_back_stream
        #pragma HLS INTERFACE axis port=ack_back_to_router
        #pragma HLS INTERFACE axis port=req_to_mem_access
        #pragma HLS INTERFACE axis port=data_to_mem_access
        #pragma HLS INTERFACE axis port=data_from_mem_access
        #pragma HLS INTERFACE axis port=vec_info_from_mem_access

       
       
        #pragma HLS DATAFLOW

        
    }
}
