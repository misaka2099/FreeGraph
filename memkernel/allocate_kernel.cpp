#include "data_type.h"
#include <cstdio>
#include <hls_stream.h>
#include "debug.h"
#include "memory_manage.h"
extern "C"
{
    void allocate_kernel(
        BYTE* bm_ptr,
        hls::stream<VEC_INFO_STREAM> &vec_info_stream,
        hls::stream<COMMAND_STREAM> &ack_stream,
        hls::stream<VEC_INFO_STREAM> &vec_info_back
    )
    {
        #pragma HLS INTERFACE s_axilite port=return
        #pragma HLS INTERFACE m_axi port=bm_ptr offset=slave bundle=gmem0
        #pragma HLS INTERFACE axis port=vec_info_stream
        #pragma HLS INTERFACE axis port=ack_stream
        #pragma HLS INTERFACE s_axilite port=return
        while (true)
        {
            auto info = vec_info_stream.read();
            int total_size = info.data.range(63, 32);
            
            int start_pos;
            int len_in_pages = total_size / INIT_VEC_LEN_IN_BYTE;
            if (info.last == 1)
            {
                HERE
                // printf("FREE KERNEL GET LAST TAG %d\n", tag);
                return;
            }
            if (total_size % INIT_VEC_LEN_IN_BYTE != 0) 
            {
                len_in_pages++;
            }
            allocate(start_pos, len_in_pages, bm_ptr);
            // printf("LEN IN PAGES %d\n", len_in_pages);
            // printf("ALLOCATED POS %d, SIZE %d\n", start_pos*INIT_VEC_LEN_IN_BYTE, total_size);
            COMMAND_STREAM ack;
            ack.data = 1;
            ack_stream.write(ack);
            vec_info_back.write({.data=start_pos*INIT_VEC_LEN_IN_BYTE | ((VEC_INFO)total_size << 32)});
        }
    }
}