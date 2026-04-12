#include "data_type.h"
#include "debug.h"
#include "etc/autopilot_ssdm_op.h"
#include "hls_stream.h"
#include <cstdio>
#include <cstring>
#include <ap_utils.h>

const int extra_mem_needed = VERTEX_PER_PMA * SEGMENT_SIZE * sizeof(int);

static const PMA_HEADER kDefaultVertexHeader = {
    {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static void process_cmds_direct(
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
    hls::stream<MEMORY_ALLOC_FREE_REQUEST> &req_to_mem_alloc,
    hls::stream<VEC_INFO_STREAM> &vec_info_back_stream,
    hls::stream<VERTEX_ACK> &ack_back_to_router,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access,
    hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_mem_access
)
{
    ap_uint<512> empty_word = 0;
    for (int i = 0; i < 16; i++)
    {
        #pragma HLS UNROLL
        empty_word.range((i + 1) * 32 - 1, i * 32) = UNUSED_ICON;
    }

    while (true)
    {
        #pragma HLS PIPELINE II=1
        PMA_UPDATE_MESSAGE cmd = cmd_stream.read();
        if (cmd.data == PMA_STOP)
        {
            ack_back_to_router.write({.data = 1});
            if (cmd.last == 1)
            {
                MEMORY_ALLOC_FREE_REQUEST finish_alloc;
                finish_alloc.data = 0;
                finish_alloc.data.range(67,64) = REQ_TYPE_FINISH;
                finish_alloc.last = 1;
                req_to_mem_alloc.write(finish_alloc);

                MEMORY_INFO_ACCESS_COMMAND finish_mem;
                finish_mem.command_type = MEMORY_FINISH;
                finish_mem.pma_idx = 0;
                finish_mem.start = 0;
                finish_mem.len = 0;
                finish_mem.last = 1;
                req_to_mem_access.write(finish_mem);
                break;
            }
            continue;
        }

        MEMORY_ALLOC_FREE_REQUEST req;
        req.data = 0;
        req.data.range(31,0) = 0;
        req.data.range(63,32) = sizeof(PMA_HEADER) + extra_mem_needed;
        req.data.range(67,64) = REQ_TYPE_ALLOC;
        req.last = 0;
        req_to_mem_alloc.write(req);
        #pragma HLS PROTOCOL fixed
        ap_wait();
        VEC_INFO vec_info = vec_info_back_stream.read().data;
        int pma_idx = cmd.data.range(31,0) / VERTEX_PER_PMA;
        int start_addr = vec_info.range(31,0);

        MEMORY_INFO_ACCESS_COMMAND vec_cmd;
        vec_cmd.command_type = VEC_INFO_WRITE;
        vec_cmd.pma_idx = pma_idx;
        vec_cmd.start = start_addr;
        vec_cmd.len = vec_info.range(63,32);
        vec_cmd.last = 0;
        req_to_mem_access.write(vec_cmd);

        MEMORY_INFO_ACCESS_COMMAND header_cmd;
        header_cmd.command_type = MEMORY_WRITE;
        header_cmd.pma_idx = pma_idx;
        header_cmd.start = start_addr;
        header_cmd.len = 256;
        header_cmd.last = 0;
        req_to_mem_access.write(header_cmd);

        {
            ap_uint<512> buffer[4];
            for (int j = 0; j < 16; j++)
            {
                #pragma HLS UNROLL
                buffer[0].range(32 * (j + 1) - 1, 32 * j) = kDefaultVertexHeader.vertex_range[j];
            }
            for (int j = 0; j < 16; j++)
            {
                #pragma HLS UNROLL
                buffer[1].range(32 * (j + 1) - 1, 32 * j) = kDefaultVertexHeader.edge_count[j];
            }
            for (int j = 0; j < 8; j++)
            {
                #pragma HLS UNROLL
                ap_uint<64> d = *reinterpret_cast<const ap_uint<64>*>(&kDefaultVertexHeader.attr[j]);
                buffer[2].range(64 * (j + 1) - 1, 64 * j) = d;
                d = *reinterpret_cast<const ap_uint<64>*>(&kDefaultVertexHeader.attr[j + 8]);
                buffer[3].range(64 * (j + 1) - 1, 64 * j) = d;
            }
            for (int i = 0; i < 4; i++)
            {
                #pragma HLS PIPELINE II=1
                ap_axiu<512, 0, 0, 0> pkt;
                pkt.data = buffer[i];
                pkt.last = (i == 3) ? 1 : 0;
                data_to_mem_access.write(pkt);
            }
        }

        MEMORY_INFO_ACCESS_COMMAND data_cmd;
        data_cmd.command_type = MEMORY_WRITE;
        data_cmd.pma_idx = pma_idx;
        data_cmd.start = start_addr + 256;
        data_cmd.len = 4096;
        data_cmd.last = 0;
        req_to_mem_access.write(data_cmd);

        for (int i = 0 ; i < 64 ; i++)
        {
            #pragma HLS PIPELINE II=1
            ap_axiu<512, 0, 0, 0> pkt;
            pkt.data = empty_word;
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
        hls::stream<MEMORY_ALLOC_FREE_REQUEST> &req_to_mem_alloc,
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

        process_cmds_direct(
            cmd_stream,
            req_to_mem_alloc,
            vec_info_back_stream,
            ack_back_to_router,
            req_to_mem_access,
            data_to_mem_access
        );

    }
}