#include "data_type.h"
#include "debug.h"
#include "hls_stream.h"
#include <cstdio>
#include <cstring>
#include <ap_utils.h>

const int extra_mem_needed = VERTEX_PER_PMA * SEGMENT_SIZE * sizeof(int);
void read_cmd(
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_alloc,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_header,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_writer
)
{
    printf("read_cmd\n");
    READ_CMD:
    while(true) {
        #pragma HLS PIPELINE II=1
        PMA_UPDATE_MESSAGE cmd = cmd_stream.read();
        cmd_to_alloc.write(cmd);
        cmd_to_header.write(cmd);
        cmd_to_writer.write(cmd);
        if (cmd.opcode == PMA_UPDATE_STOP && cmd.last == 1) {
            break;
        }
    }
}
void send_alloc_request(
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_stream,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_in_stream
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
        if (cmd.opcode == PMA_UPDATE_STOP) 
        {
            if (cmd.last == 1)
            {
                MEMORY_INFO_ACCESS_COMMAND finish_req = {};
                finish_req.command_type = ALLOC_FINISH;
                finish_req.last = 1;
                req_to_mem_stream.write(finish_req);
                break;
            }
            continue;
        }
        MEMORY_INFO_ACCESS_COMMAND req;
        // req.data = 0;
        // req.data.range(31,0) = 0;
        // req.data.range(63,32) = sizeof(PMA_HEADER) + extra_mem_needed;
        // req.data.range(67,64) = REQ_TYPE_ALLOC;
        req.command_type = VEC_INFO_ALLOC;
        req.start = 0;
        req.len = sizeof(PMA_HEADER) + extra_mem_needed;
        req.last = 0;
        req_to_mem_stream.write(req);
    }

}

void process_header(
    
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_in_stream,
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
        if (cmd.opcode == PMA_UPDATE_STOP)
        {
            if (cmd.last == 1)
                break;
            continue;
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
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_in,
    hls::stream<VEC_INFO_STREAM> &vec_info_back_stream,
    hls::stream<PMA_HEADER>& header_in,
    hls::stream<VERTEX_ACK>& ack_back_to_router,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access,
    hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_mem_access
)
{
    
    printf("write_hbm\n");
    ap_uint<512> wide_data;//32 * 16 = 512
    for (int i = 0; i < 16; i++) 
    {
        #pragma HLS UNROLL
        wide_data.range((i+1)*32-1, i*32) = UNUSED_ICON;
    }
    WRITE_HBM:
    while(true)
    {
        auto cmd = cmd_in.read();
        if (cmd.opcode == PMA_UPDATE_STOP)
        {
            ack_back_to_router.write({.data = 1});
            if (cmd.last == 1)
            {
                
                MEMORY_INFO_ACCESS_COMMAND finish_cmd = {};
                finish_cmd.last = 1;
                finish_cmd.command_type = MEMORY_FINISH;
                req_to_mem_access.write(finish_cmd);
                break;
            }
            continue;  
        }        
        VEC_INFO vec_info = vec_info_back_stream.read().data;
        PMA_HEADER header = header_in.read();
        int pma_idx = cmd.src / VERTEX_PER_PMA;
        // printf("PROCESSING PMA IDX %d START %d, LEN %d \n",pma_idx, int(vec_info.range(31,0)), int(vec_info.range(63,32)));
       
        // 1. Update VEC_INFO
        MEMORY_INFO_ACCESS_COMMAND vec_cmd;
        // if (direction == OUT_DIRECTION)
        //     vec_cmd.command_type = POS_VEC_INFO_WRITE;
        // else 
        //     vec_cmd.command_type = NEG_VEC_INFO_WRITE;
        vec_cmd.command_type = VEC_INFO_WRITE; // single direction
        vec_cmd.pma_idx = pma_idx;
        vec_cmd.start = vec_info.range(31,0);
        vec_cmd.len = vec_info.range(63,32);
        vec_cmd.last = 0; 
        req_to_mem_access.write(vec_cmd);

        int start_addr = vec_info.range(31,0);
        int len = vec_info.range(63,32);
        const int pma_header_addr_in_512_patten = start_addr >> 6;
        
        // 2. Write Header
        MEMORY_INFO_ACCESS_COMMAND header_cmd;
        header_cmd.command_type = MEMORY_WRITE;
        header_cmd.pma_idx = pma_idx;
        header_cmd.start = start_addr;
        header_cmd.len = 256; // 4 * 64 bytes
        header_cmd.last = 0;
        req_to_mem_access.write(header_cmd);

        // memcpy(hbm_ptr + start_addr, &header, sizeof(PMA_HEADER));
        {
            ap_uint<512> buffer[4];
            // 打包 vertex_range 到 buffer[0]
            for (int j = 0; j < 16; j++) {
                #pragma HLS UNROLL
                buffer[0].range(32 * (j + 1) - 1, 32 * j) = header.vertex_range[j];
            }
            // 打包 edge_count 到 buffer[1]
            for (int j = 0; j < 16; j++) {
                #pragma HLS UNROLL
                buffer[1].range(32 * (j + 1) - 1, 32 * j) = header.edge_count[j];
            }
            // 打包 attr 到 buffer[2] 和 buffer[3]
            for (int j = 0; j < 8; j++) {
                #pragma HLS UNROLL
                ap_uint<64> d = *reinterpret_cast<ap_uint<64>*>(&header.attr[j]);
                buffer[2].range(64 * (j + 1) - 1, 64 * j) = d;
                d = *reinterpret_cast<ap_uint<64>*>(&header.attr[j + 8]);
                buffer[3].range(64 * (j + 1) - 1, 64 * j) = d;
            }
            for (int i = 0; i < 4; i++) {
                #pragma HLS PIPELINE II=1
                ap_axiu<512, 0, 0, 0> pkt;
                pkt.data = buffer[i];
                pkt.last = (i == 3) ? 1 : 0;
                data_to_mem_access.write(pkt);
            }
        }
        
        // 3. Write Empty Raw PMA
        MEMORY_INFO_ACCESS_COMMAND data_cmd;
        data_cmd.command_type = MEMORY_WRITE;
        data_cmd.pma_idx = pma_idx;
        data_cmd.start = start_addr + 256; // 4 * 64 bytes offset
        data_cmd.len = 4096; // 64 * 64 bytes -> 64 chunks
        data_cmd.last = 0;
        req_to_mem_access.write(data_cmd);

        // 16 * 64 / 16
        WRITE_EMPTY_RAW_PMA:
        for (int i = 0 ; i < 64 ; i++)
        {
            #pragma HLS PIPELINE II=1
            ap_axiu<512, 0, 0, 0> pkt;
            pkt.data = wide_data;
            pkt.last = (i == 63) ? 1 : 0;
            data_to_mem_access.write(pkt);
        }   
    }
}
extern "C"
{
    //一次插入 16 顶点
    void pma_insert_vertex(
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
        //mem operation
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_alloc,
        hls::stream<VEC_INFO_STREAM> &vec_info_back_stream,
        hls::stream<VERTEX_ACK> &ack_back_to_router,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_mem_access
    )
    {
        #pragma HLS INTERFACE axis port=cmd_stream
        #pragma HLS INTERFACE axis port=req_to_mem_alloc
        #pragma HLS INTERFACE axis port=vec_info_back_stream
        #pragma HLS INTERFACE axis port=ack_back_to_router
        #pragma HLS INTERFACE axis port=req_to_mem_access
        #pragma HLS INTERFACE axis port=data_to_mem_access

        #pragma HLS STREAM variable= cmd_stream depth=16
        #pragma HLS STREAM variable=req_to_mem_alloc depth=16
        #pragma HLS STREAM variable=vec_info_back_stream depth=16
        #pragma HLS STREAM variable=ack_back_to_router depth=1
        #pragma HLS STREAM variable=req_to_mem_access depth=16
        #pragma HLS STREAM variable=data_to_mem_access depth=16

        hls::stream<PMA_UPDATE_MESSAGE> cmd_to_alloc("cmd_to_alloc");
        hls::stream<PMA_UPDATE_MESSAGE> cmd_to_header("cmd_to_header");
        hls::stream<PMA_UPDATE_MESSAGE> cmd_to_writer("cmd_to_writer");
        hls::stream<PMA_HEADER> header_to_writer("header_to_writer");

        #pragma HLS STREAM variable=cmd_to_alloc depth=8
        #pragma HLS STREAM variable=cmd_to_header depth=8
        #pragma HLS STREAM variable=cmd_to_writer depth=8
        #pragma HLS STREAM variable=header_to_writer depth=8


        #pragma HLS DATAFLOW
        read_cmd(cmd_stream, cmd_to_alloc, cmd_to_header, cmd_to_writer);
        
        // 2. 并行处理
        send_alloc_request(req_to_mem_alloc, cmd_to_alloc);
        process_header(cmd_to_header, header_to_writer);
        
        // 3. 唯一的接收者和最终写入
        write_hbm(
                  cmd_to_writer, vec_info_back_stream, header_to_writer, ack_back_to_router,
                  req_to_mem_access, data_to_mem_access);

    }
}