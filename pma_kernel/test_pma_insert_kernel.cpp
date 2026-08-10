#include "data_type.h"
#include "pma_insert_edge_common.h"
#include <cstdio>
#include <hls_stream.h>
#include <ap_utils.h>

#define END_INSERT_CMD 0xFFFFFFFF

void get_seg(int src, int seg_idx, int seg_buffer[64])
{

}
void modify_seg(int seg_buffer[64], int opcode)
{

}
void write_seg(int src, int seg_idx, int seg_buffer[64])
{
    
}
void get_header(int header, PMA_HEADER&     pma_header)
{
    #pragma HLS INLINE

}
void write_header(
    int src, 
    PMA_HEADER&     pma_header
)
{
    #pragma HLS INLINE

}
void insert_element(
    //<src,dst,seg_idx,opcode>
    hls::stream<ap_uint<128>>& insert_vec_info_stream
)
{
    while (true)
    {
        #pragma HLS PIPELINE II=1
        auto insert_vec_info = insert_vec_info_stream.read();
        int src = insert_vec_info.range(31, 0);
        int dst = insert_vec_info.range(63, 32);
        int seg_idx = insert_vec_info.range(95, 64);
        int opcode = insert_vec_info.range(127, 96);
        if (insert_vec_info == END_INSERT_CMD) break;

        int seg_buffer[64];
        int pma_idx = src / 16;
        PMA_HEADER pma_header;
        get_header(pma_idx, pma_header);
        get_seg(src, seg_idx, seg_buffer);
        modify_seg(seg_buffer, opcode);
        write_seg(src, seg_idx, seg_buffer);
        write_header(pma_idx, pma_header);

        //get the corresponding header and vertex range of the segment idx
        //perform insertion based on the opcode (insert into the middle of the segment or insert a new segment)
    }
}
extern "C"
{
    void pma_insert_kernel(
        hls::stream<SEARCH_RESULT_STREAM> &search_result_stream,
        hls::stream<PMA_HEADER> &pma_header_in,
        hls::stream<VEC_INFO> &pma_addr_in,
        hls::stream<pma_idx_info> &pma_idx_in,
        hls::stream<MEMORY_ALLOC_FREE_REQUEST_INTERNAL> &alloc_request_stream,
        hls::stream<PMA_HEADER> &pma_header_out,
        hls::stream<VEC_INFO> &pma_addr_out,
        hls::stream<inserted_seg> &inserted_pos,
        hls::stream<pma_idx_info> &pma_idx_out,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_mem_access,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_mem_access)
    {
        #pragma HLS INTERFACE axis port=search_result_stream
        #pragma HLS INTERFACE axis port=pma_header_in
        #pragma HLS INTERFACE axis port=pma_addr_in
        #pragma HLS INTERFACE axis port=pma_idx_in
        #pragma HLS INTERFACE axis port=alloc_request_stream
        #pragma HLS INTERFACE axis port=pma_header_out
        #pragma HLS INTERFACE axis port=pma_addr_out
        #pragma HLS INTERFACE axis port=inserted_pos
        #pragma HLS INTERFACE axis port=pma_idx_out
        #pragma HLS INTERFACE axis port=req_to_mem_access
        #pragma HLS INTERFACE axis port=data_to_mem_access
        #pragma HLS INTERFACE axis port=data_from_mem_access

        hls::stream<memory_access_command> memory_access_stream;
        hls::stream<ap_uint<512>> data_to_mem;
        hls::stream<ap_uint<512>> data_from_mem;
        #pragma HLS STREAM variable=memory_access_stream depth=16
        #pragma HLS STREAM variable=data_to_mem depth=16
        #pragma HLS STREAM variable=data_from_mem depth=16

        #pragma HLS DATAFLOW

    }
}
