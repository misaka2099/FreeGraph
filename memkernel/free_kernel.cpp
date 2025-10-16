#include "debug.h"
#include "memory_manage.h"
#include "data_type.h"
#include <hls_stream.h>
extern "C"
{
    void free_kernel(
        BYTE* bm_ptr,
        hls::stream<VEC_INFO_STREAM> &vec_info_stream,
        hls::stream<COMMAND_STREAM> &ack_stream
    )
    {
        #pragma HLS INTERFACE s_axilite port=return
        #pragma HLS INTERFACE m_axi port=bm_ptr offset=slave bundle=gmem0
        #pragma HLS INTERFACE axis port=vec_info_stream
        #pragma HLS INTERFACE axis port=ack_stream
        while (true)
        {
            auto info = vec_info_stream.read();
            int start_pos = info.data.range(31, 0);
            int total_size = info.data.range(63, 32);
            if (info.last == 1)
            {
                HERE
                // printf("FREE KERNEL GET LAST TAG %d\n", tag);
                return;
            }
            int start_page = start_pos / INIT_VEC_LEN_IN_BYTE;
            int len_in_pages = total_size / INIT_VEC_LEN_IN_BYTE;
            if (total_size % INIT_VEC_LEN_IN_BYTE != 0) 
            {
                len_in_pages++;
            }
            deallocate(start_page, len_in_pages, bm_ptr);
            COMMAND_STREAM ack;
            ack.data = 1;
            ack_stream.write(ack);
        }
    }
}