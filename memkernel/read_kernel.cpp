#include <cstdio>
#include <hls_stream.h>
#include "data_type.h"  
#include "debug.h"
extern "C"
{
    void read_kernel(
        BYTE* hbm_ptr, 
        hls::stream<BYTE_STREAM>& data_out,
        // hls::stream<BYTE_STREAM>& tag_in,
        hls::stream<TAG_STREAM>& tag_vec_info_in, 
        hls::stream<COMMAND_STREAM>& ack_back_to_scheduler
    )
    {
        #pragma HLS INTERFACE s_axilite port=return
        #pragma HLS INTERFACE m_axi port=hbm_ptr offset=slave bundle=gmem0
        #pragma HLS INTERFACE axis port=data_out
        #pragma HLS INTERFACE axis port=tag_vec_info_in
        #pragma HLS INTERFACE axis port=ack_back_to_scheduler
        // #pragma HLS INTERFACE s_axilite port=return

        while (true)
        {
            HERE
            auto info = tag_vec_info_in.read();
            int start_pos = info.data.range(31, 0);
            int total_size = info.data.range(63, 32);
            int tag = info.data.range(71, 64);
            if (info.last == 1)
            {
                HERE
                // printf("READ KERNEL GET LAST TAG %d\n", tag);
                return;
            }
            HERE
            printf("READ KERNEL GET TAG %d, START POS %d, TOTAL SIZE %d\n", tag, start_pos, total_size);
            for (int i = 0; i < total_size; i++) 
            {
                #pragma HLS PIPELINE II=1
                BYTE data_byte = hbm_ptr[start_pos + i];
                BYTE_STREAM payload;
                payload.data = data_byte;
                payload.keep = -1;
                data_out.write(payload);
            }
            HERE
            COMMAND_STREAM ack;
            ack.data = tag;
            ack_back_to_scheduler.write(ack);
        }
    }
}