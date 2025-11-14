#include "data_type.h"
#include "debug.h"
#include "hls_stream.h"
#include <cstdio>
#include <cstring>
#include <ap_utils.h>

const int extra_mem_needed = VERTEX_PER_PMA * SEGMENT_SIZE * sizeof(int);
void read_cmd(
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
    hls::stream<ap_uint<128>> &cmd_to_alloc,
    hls::stream<ap_uint<128>> &cmd_to_header,
    hls::stream<ap_uint<128>> &cmd_to_writer
)
{
    printf("read_cmd\n");
    READ_CMD:
    while(true) {
        #pragma HLS PIPELINE II=1
        PMA_UPDATE_MESSAGE cmd = cmd_stream.read();
        cmd_to_alloc.write(cmd.data);
        cmd_to_header.write(cmd.data);
        cmd_to_writer.write(cmd.data);
        if (cmd.data == PMA_STOP && cmd.last == 1) {
            break;
        }
    }
}
void send_alloc_request(
    hls::stream<VEC_INFO_STREAM> &vec_info_to_mem_stream,
    hls::stream<COMMAND_STREAM> &cmd_to_mem_stream,
    hls::stream<ap_uint<128>> &cmd_in_stream
)
{
    
    printf("send_alloc_request\n");
    SEND_ALLOC_REQUEST:
    while (true)
    {
        #pragma HLS PIPELINE II=1
        if (cmd_in_stream.empty()) {
            ap_wait();
            continue;
        }
        auto cmd = cmd_in_stream.read();
        if (cmd == PMA_STOP) {
            break;
        }
        VEC_INFO vec;
        vec.range(31,0) = 0;
        vec.range(63,32) = sizeof(PMA_HEADER) + extra_mem_needed;
        vec_info_to_mem_stream.write({.data=vec});
        cmd_to_mem_stream.write({.data = MEM_ALLOC});
    }

}

void process_header(
    
    hls::stream<ap_uint<128>> &cmd_in_stream,
    hls::stream<PMA_HEADER>& header_out
)
{
    printf("process_header\n");
    INSERT_VERTEX:
    while (true)
    {
        #pragma HLS PIPELINE II=1
        if (cmd_in_stream.empty()) {
            // 如果流为空，则等待一个周期，避免阻塞
            ap_wait();
            continue;
        }
        auto cmd = cmd_in_stream.read();
        if (cmd == PMA_STOP)
        {
            break;
        }
        PMA_HEADER header;
        for (int i = 0 ; i < VERTEX_PER_PMA ; i++)
        {
            #pragma HLS UNROLL
            int local_src = i;
            header.edge_count[local_src] = 0;
            header.attr[local_src] = 0;
            header.vertex_range[local_src] = i + 1;
        }
        header_out.write(header);
    }
}


void write_hbm(
    VEC_INFO* pos_pma_addrs,
    VEC_INFO* neg_pma_addrs,
    BYTE* hbm_ptr,
    hls::stream<ap_uint<128>>& cmd_in,
    hls::stream<VEC_INFO_STREAM> &vec_info_back_stream,
    hls::stream<PMA_HEADER>& header_in,
    hls::stream<COMMAND_STREAM>& ack_back_to_router
)
{
    
    printf("write_hbm\n");
    WRITE_HBM:
    while(true)
    {
        #pragma HLS PIPELINE II=1
        auto cmd = cmd_in.read();
        if (cmd == PMA_STOP)
        {
            ack_back_to_router.write({.data = 1});
            break;
        }        
        VEC_INFO vec_info = vec_info_back_stream.read().data;
        PMA_HEADER header = header_in.read();
        int pma_idx = cmd.range(31,0) / VERTEX_PER_PMA;
        int direction = cmd.range(98,98);
        // printf("PROCESSING PMA IDX %d START %d, LEN %d \n",pma_idx, int(vec_info.range(31,0)), int(vec_info.range(63,32)));
       
        if (direction == OUT_DIRECTION)
        {
            pos_pma_addrs[pma_idx] = vec_info;
        }
        else 
        {
            neg_pma_addrs[pma_idx] = vec_info;
        }
        int start_addr = vec_info.range(31,0);
        int len = vec_info.range(63,32);
        // memcpy(hbm_ptr + start_addr, &header, sizeof(PMA_HEADER));
        PMA_HEADER* write_header = (PMA_HEADER*)hbm_ptr;
        write_header[0] = header;
        WRITE_EMPTY_RAW_PMA:
        for (int i = 0 ; i < 16 * SEGMENT_SIZE  ; i++)
        {
            #pragma HLS PIPELINE II=1
            // 直接在 hbm_ptr 上计算地址，然后进行类型转换和解引用
            // 这能确保每次写入都使用了正确的、完整的偏移地址
            int* target_ptr = (int*)(hbm_ptr + start_addr + sizeof(PMA_HEADER) + i * 4);
            *target_ptr = (int)UNUSED_ICON;
        }
    }
}
extern "C"
{
    //一次插入 16 顶点
    void pma_insert_vertex(
        VEC_INFO* pos_pma_addrs,
        VEC_INFO* neg_pma_addrs,
        BYTE* hbm_ptr,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
        //mem operation
        hls::stream<VEC_INFO_STREAM> &vec_info_to_mem_stream,
        hls::stream<COMMAND_STREAM> &cmd_to_mem_stream,
        hls::stream<VEC_INFO_STREAM> &vec_info_back_stream,
        hls::stream<COMMAND_STREAM> &ack_back_to_router
    )
    {
        #pragma HLS INTERFACE m_axi port=pos_pma_addrs offset=slave bundle=gmem0 depth=512
        #pragma HLS INTERFACE m_axi port=neg_pma_addrs offset=slave bundle=gmem1 depth=512
        #pragma HLS INTERFACE m_axi port=hbm_ptr offset=slave bundle=gmem2 depth=512
        #pragma HLS INTERFACE axis port=cmd_stream
        #pragma HLS INTERFACE axis port=vec_info_to_mem_stream
        #pragma HLS INTERFACE axis port=cmd_to_mem_stream
        #pragma HLS INTERFACE axis port=vec_info_back_stream
        #pragma HLS INTERFACE axis port=ack_back_to_router


        hls::stream<ap_uint<128>> cmd_to_alloc("cmd_to_alloc");
        hls::stream<ap_uint<128>> cmd_to_header("cmd_to_header");
        hls::stream<ap_uint<128>> cmd_to_writer("cmd_to_writer");
        hls::stream<PMA_HEADER> header_to_writer("header_to_writer");

        #pragma HLS STREAM variable=cmd_to_alloc depth=8
        #pragma HLS STREAM variable=cmd_to_header depth=8
        #pragma HLS STREAM variable=cmd_to_writer depth=8
        #pragma HLS STREAM variable=header_to_writer depth=8


        #pragma HLS DATAFLOW
        read_cmd(cmd_stream, cmd_to_alloc, cmd_to_header, cmd_to_writer);
        
        // 2. 并行处理
        send_alloc_request(vec_info_to_mem_stream, cmd_to_mem_stream, cmd_to_alloc);
        process_header(cmd_to_header, header_to_writer);
        
        // 3. 唯一的接收者和最终写入
        write_hbm(pos_pma_addrs, neg_pma_addrs, hbm_ptr, 
                  cmd_to_writer, vec_info_back_stream, header_to_writer, ack_back_to_router);

    }
}