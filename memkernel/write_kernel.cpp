#include "ap_int.h"
#include "data_type.h"
#include "debug.h"
#include <cstdio>
#include <cstring>
#include <hls_stream.h>
extern "C"
{
    void write_kernel(BYTE* hbm_ptr, 
        hls::stream<BYTE_STREAM>& data_in, 
        hls::stream<VEC_INFO_STREAM>& vec_info_in, 
        hls::stream<COMMAND_STREAM>& ack_back_to_scheduler)
    {
        #pragma HLS INTERFACE s_axilite port=return
        #pragma HLS INTERFACE m_axi port=hbm_ptr offset=slave bundle=gmem0
        #pragma HLS INTERFACE axis port=data_in
        #pragma HLS INTERFACE axis port=vec_info_in
        #pragma HLS INTERFACE axis port=ack_back_to_scheduler
        #pragma HLS INTERFACE s_axilite port=return
        while (true)
        {
            auto info = vec_info_in.read();
            // HERE
            int start_pos = info.data.range(31, 0);
            int total_size = info.data.range(63, 32);
            if (info.last == 1)
            {
                // HERE
                // printf("WRITE KERNEL GET LAST TAG %d\n", tag);
                return;
            }
            // std::cout << "Write request: start_pos=" << start_pos << ", total_size=" << total_size << std::endl;
            for (int i = 0; i < total_size; i++) 
            {
                #pragma HLS PIPELINE II=1
                // HERE
                BYTE_STREAM payload = data_in.read();
                // printf("ITERATION %d: data=%02x\n", i, payload.data);
                hbm_ptr[start_pos + i] = payload.data;
            }
            COMMAND_STREAM ack;
            ack.data = 1;
            ack_back_to_scheduler.write(ack);
            // HERE
        }
    }
}