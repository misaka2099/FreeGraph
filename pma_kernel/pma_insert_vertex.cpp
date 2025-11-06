#include "data_type.h"
#include "hls_stream.h"
#include <cstring>


const int extra_mem_needed = VERTEX_PER_PMA * SEGMENT_SIZE * sizeof(int);
void read_cmd(
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_alloc,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_header,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_writer
)
{
    while(true) {
        PMA_UPDATE_MESSAGE cmd = cmd_stream.read();
        cmd_to_alloc.write(cmd);
        cmd_to_header.write(cmd);
        cmd_to_writer.write(cmd);
        if (cmd.data == PMA_STOP && cmd.last == 1) {
            break;
        }
    }
}
void send_alloc_request(
    hls::stream<VEC_INFO_STREAM> &vec_info_to_mem_stream,
    hls::stream<COMMAND_STREAM> &cmd_to_mem_stream,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_in_stream
)
{
    while (true)
    {
        PMA_UPDATE_MESSAGE cmd = cmd_in_stream.read();
        if (cmd.data == PMA_STOP && cmd.last == 1) {
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
    
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_in_stream,
    hls::stream<PMA_HEADER>& header_out
)
{
    INSERT_VERTEX:
    while (true)
    {
        auto cmd = cmd_in_stream.read();
        if (cmd.data == PMA_STOP && cmd.last == 1)
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
    hls::stream<COMMAND_STREAM> &ack_back_to_router,
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_in,
    hls::stream<VEC_INFO_STREAM> &vec_info_back_stream,
    hls::stream<PMA_HEADER>& header_in
)
{
    while(true)
    {
        PMA_UPDATE_MESSAGE cmd = cmd_in.read();
        if (cmd.data == PMA_STOP && cmd.last == 1)
        {
            ack_back_to_router.write({.data=1});    
            break;
        }
        VEC_INFO vec_info = vec_info_back_stream.read().data;
        PMA_HEADER header = header_in.read();
        int pma_idx = cmd.data.range(31,0) / VERTEX_PER_PMA;
        int direction = cmd.data.range(98,98);
        if (direction == OUT_DIRECTION)
        {
            pos_pma_addrs[pma_idx] = vec_info;
        }
        else 
        {
            neg_pma_addrs[pma_idx] = vec_info;
        }
        int start_addr = vec_info.range(0,31);
        int len = vec_info.range(63,32);
        memcpy(hbm_ptr + start_addr, &header, sizeof(PMA_HEADER));
        int* int_ptr = (int*)(hbm_ptr + start_addr + sizeof(PMA_HEADER));
        for (int i = 0 ; i < 16 * SEGMENT_SIZE  ; i++)
        {
            #pragma HLS PIPELINE II=1
            int_ptr[i] = (int)UNUSED_ICON;
        }
        ack_back_to_router.write({.data=1});    
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
        hls::stream<COMMAND_STREAM> &ack_back_to_router,
        //mem operation
        hls::stream<VEC_INFO_STREAM> &vec_info_to_mem_stream,
        hls::stream<COMMAND_STREAM> &cmd_to_mem_stream,
        hls::stream<VEC_INFO_STREAM> &vec_info_back_stream
    )
    {
        #pragma HLS INTERFACE m_axi port=pos_pma_addrs offset=slave bundle=gmem0
        #pragma HLS INTERFACE m_axi port=neg_pma_addrs offset=slave bundle=gmem1
        #pragma HLS INTERFACE m_axi port=hbm_ptr offset=slave bundle=gmem2
        #pragma HLS INTERFACE axis port=cmd_stream
        #pragma HLS INTERFACE axis port=ack_back_to_router
        #pragma HLS INTERFACE axis port=vec_info_to_mem_stream
        #pragma HLS INTERFACE axis port=cmd_to_mem_stream
        #pragma HLS INTERFACE axis port=vec_info_back_stream
        #pragma HLS DATAFLOW

        hls::stream<PMA_UPDATE_MESSAGE> cmd_to_alloc("cmd_to_alloc");
        hls::stream<PMA_UPDATE_MESSAGE> cmd_to_header("cmd_to_header");
        hls::stream<PMA_UPDATE_MESSAGE> cmd_to_writer("cmd_to_writer");
        hls::stream<PMA_HEADER> header_to_writer("header_to_writer");

        read_cmd(cmd_stream, cmd_to_alloc, cmd_to_header, cmd_to_writer);
        
        // 2. 并行处理
        send_alloc_request(vec_info_to_mem_stream, cmd_to_mem_stream, cmd_to_alloc);
        process_header(cmd_to_header, header_to_writer);
        
        // 3. 唯一的接收者和最终写入
        write_hbm(pos_pma_addrs, neg_pma_addrs, hbm_ptr, ack_back_to_router, 
                  cmd_to_writer, vec_info_back_stream, header_to_writer);

    }
}