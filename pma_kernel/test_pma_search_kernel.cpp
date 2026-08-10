#include "data_type.h"
#include "host_config.h"
#include "pma_insert_edge_common.h"
#include <cstdio>
#include <climits>
#include <hls_stream.h>
#include <ap_utils.h>
#include "etc/autopilot_ssdm_op.h"
#define ENDEDGE 0xFFFFFFFF
#define MAX_SEARCH_WORKER 4
struct pma_insert_pos
{
    int src[16];
    int dst[16];
    int seg_idx[16];
    ap_uint<1> last;
};

void process_cmd(
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
    hls::stream<int> &cmd_count_stream,
    hls::stream<ap_uint<64>> cmd_to_worker_stream[MAX_SEARCH_WORKER]
)
{
    printf("PROCESS_CMD\n");
    while(true)
    {
        if (cmd_stream.empty()) {
            ap_wait();
            continue;
        }
        ap_uint<128> cmd_batch[MAX_CMD_NUM];
        #pragma HLS ARRAY_PARTITION variable=cmd_batch complete
        int cmd_count = 0;
        // 读取直到 last=1
        for (int i = 0; i < MAX_CMD_NUM; i++) {
            #pragma HLS PIPELINE II=1
            auto cmd = cmd_stream.read();
            if (cmd.data == PMA_STOP) 
            {
                if (cmd.last == 1)
                {
                    for (int i = 0 ; i < MAX_SEARCH_WORKER; i++)
                    {
                        cmd_to_worker_stream[i].write(ENDEDGE);    
                    }
                    cmd_count_stream.write(-1);
                    return;
                }
                continue;
            }
            cmd_batch[i] = cmd.data;
            cmd_count++;
            if (cmd.last == 1) {
                break;
            }
        }
        // 将命令分发给不同的搜索工作线程
        for (int i = 0; i < cmd_count; i++) {
            int worker_id = i % MAX_SEARCH_WORKER;
            cmd_to_worker_stream[worker_id].write(cmd_batch[i].range(63,0));
        }
        cmd_count_stream.write(cmd_count);
    }
}

void search(
    hls::stream<ap_uint<64>>& edge_info_stream,
    hls::stream<ap_uint<96>>& edge_insert_info_stream,
    hls::stream<memory_access_command> &memory_access_stream,
    hls::stream<ap_uint<512>> &data_from_mem
)
{
    int segment_buffer[SEGMENT_SIZE];
    #pragma HLS ARRAY_PARTITION variable=segment_buffer cyclic factor=4

    while (true)
    {
        #pragma HLS PIPELINE II=1
        auto edge_info = edge_info_stream.read();
        int src = edge_info.range(31, 0);
        int dst = edge_info.range(63, 32);
        if (edge_info == ENDEDGE) break;

        //get src corresponding header and get vertex range
        //base on the vertex range, binary search
        //the pivot is the first element of each segment
        //this function return <src,dst,found_seg_idx>
        

        PMA_HEADER header;
        int local_src = src % VERTEX_PER_PMA;
        int pma_idx = src / VERTEX_PER_PMA;
        
        read_header(
            memory_access_stream, 
            data_from_mem, 
            header, 
            pma_idx
        );
        int seg_begin = (local_src == 0) ? 0 : header.vertex_range[local_src - 1];
        int seg_end = header.vertex_range[local_src]; // exclusive

        // Binary search by each segment's first element.
        int found_seg_idx = -1;
        if (seg_begin < seg_end)
        {
            int begin = seg_begin;
            int end = seg_end;
            while ((begin + 1) < end)
            {
                int mid = (begin + end) >> 1;
                read_seg(
                    memory_access_stream,
                    data_from_mem,
                    segment_buffer,
                    pma_idx,
                    mid
                );        
                int first_val = segment_buffer[0];

                // Treat empty segment as +INF so it belongs to the right half.
                if (first_val == UNUSED_ICON)
                {
                    end = mid;
                }
                else if (first_val <= dst)
                {
                    begin = mid;
                }
                else
                {
                    end = mid;
                }
            }
            found_seg_idx = begin;
        }

        ap_uint<96> out = 0;
        out.range(31, 0) = src;
        out.range(63, 32) = dst;
        out.range(95, 64) = found_seg_idx;
        edge_insert_info_stream.write(out);
    }

    memory_access_command stop_cmd;
    stop_cmd.pma_idx = 0;
    stop_cmd.start = 0;
    stop_cmd.len = 0;
    stop_cmd.command_type = SEG_READ;
    stop_cmd.type = FINISH;
    memory_access_stream.write(stop_cmd);
}

void mem_adapter(
    hls::stream<memory_access_command> request[MAX_SEARCH_WORKER],
    hls::stream<ap_uint<512>> data_from_mem[MAX_SEARCH_WORKER],
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access,
    hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_mem_access
)
{
    ap_uint<MAX_SEARCH_WORKER> finished_bitmap = -1;
    int idx = 0;

    while (true)
    {
        #pragma HLS PIPELINE II=1
        if (finished_bitmap == 0) break;

        memory_access_command request_info;
        if (request[idx].read_nb(request_info))
        {
            if (request_info.type == FINISH)
            {
                finished_bitmap[idx] = 0;
            }
            else
            {
                MEMORY_INFO_ACCESS_COMMAND cmd;
                cmd.src = request_info.pma_idx * VERTEX_PER_PMA;
                cmd.start = request_info.start;
                cmd.len = request_info.len;
                cmd.command_type = request_info.command_type;
                cmd.last = 0;

                req_to_mem_access.write(cmd);
                #pragma HLS PROTOCOL fixed
                ap_wait();

                int len_in_512 = 0;
                if (request_info.command_type == HEADER_READ)
                    len_in_512 = 4;
                else if (request_info.command_type == SEG_READ)
                    len_in_512 = request_info.len * 4;

                for (int i = 0; i < len_in_512; i++)
                {
                    #pragma HLS PIPELINE II=1
                    ap_axiu<512, 0, 0, 0> packet = data_from_mem_access.read();
                    data_from_mem[idx].write(packet.data);
                }
            }
        }

        idx = (idx + 1) % MAX_SEARCH_WORKER;
        ap_wait();
    }
}
void merge_results(
    hls::stream<int> &cmd_count_stream,
    hls::stream<ap_uint<96>> worker_result_stream[MAX_SEARCH_WORKER],
    hls::stream<SEARCH_RESULT_STREAM> &search_result_stream
)
{
    while (true)
    {
        int cmd_count = cmd_count_stream.read();
        if (cmd_count < 0)
            break;

        for (int i = 0; i < cmd_count; i++)
        {
            #pragma HLS PIPELINE II=1
            int worker_id = i % MAX_SEARCH_WORKER;
            ap_uint<96> res = worker_result_stream[worker_id].read();

            SEARCH_RESULT_STREAM out_pkt = {};
            out_pkt.data = 0;
            out_pkt.data.range(95, 0) = res;
            out_pkt.keep = -1;
            out_pkt.strb = -1;
            out_pkt.last = (i == (cmd_count - 1)) ? 1 : 0;
            search_result_stream.write(out_pkt);
        }
    }

}
extern "C"
{
    void pma_search_kernel(
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
        hls::stream<SEARCH_RESULT_STREAM> &search_result_stream,
        hls::stream<PMA_HEADER> &pma_header_out,
        hls::stream<VEC_INFO> &pma_addr_out,
        hls::stream<pma_idx_info> &pma_idx_out,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_mem_access,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_mem_access,
        hls::stream<VEC_INFO_STREAM> &vec_info_from_mem_access)
    {
        #pragma HLS INTERFACE axis port=cmd_stream
        #pragma HLS INTERFACE axis port=search_result_stream
        #pragma HLS INTERFACE axis port=pma_header_out
        #pragma HLS INTERFACE axis port=pma_addr_out
        #pragma HLS INTERFACE axis port=pma_idx_out
        #pragma HLS INTERFACE axis port=req_to_mem_access
        #pragma HLS INTERFACE axis port=data_to_mem_access
        #pragma HLS INTERFACE axis port=data_from_mem_access
        #pragma HLS INTERFACE axis port=vec_info_from_mem_access

        hls::stream<int> cmd_count_stream("cmd_count_stream");
        #pragma HLS STREAM variable=cmd_count_stream depth=16

        hls::stream<ap_uint<64>> cmd_to_worker_stream[MAX_SEARCH_WORKER];
        #pragma HLS STREAM variable=cmd_to_worker_stream depth=32

        hls::stream<ap_uint<96>> worker_result_stream[MAX_SEARCH_WORKER];
        #pragma HLS STREAM variable=worker_result_stream depth=32

        hls::stream<memory_access_command> memory_access_stream[MAX_SEARCH_WORKER];
        hls::stream<ap_uint<512>> data_from_mem[MAX_SEARCH_WORKER];
        #pragma HLS STREAM variable=memory_access_stream depth=16
        #pragma HLS STREAM variable=data_from_mem depth=16

        #pragma HLS DATAFLOW
        process_cmd(cmd_stream, cmd_count_stream, cmd_to_worker_stream);

        for (int i = 0; i < MAX_SEARCH_WORKER; i++)
        {
            #pragma HLS UNROLL
            search(
                cmd_to_worker_stream[i],
                worker_result_stream[i],
                memory_access_stream[i],
                data_from_mem[i]
            );
        }

        mem_adapter(
            memory_access_stream,
            data_from_mem,
            req_to_mem_access,
            data_from_mem_access
        );

        merge_results(
            cmd_count_stream,
            worker_result_stream,
            search_result_stream
        );
        
    }
}
