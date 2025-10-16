#include <hls_stream.h>
#include <ap_int.h> 
#include "data_type.h"
extern "C"
{
    void check_kernel(
        int *src,
        int *dst,
        double *vertex_attr,
        int vertex_to_check,
        hls::stream<READ_COMMAND_STREAM> &cmd_to_read,
        hls::stream<int> &data_to_check,
        hls::stream<double> &attr_to_check
    )
    {
        #pragma HLS INTERFACE m_axi port=src offset=slave bundle=gmem0
        #pragma HLS INTERFACE m_axi port=dst offset=slave bundle=gmem1
        #pragma HLS INTERFACE m_axi port=vertex_attr offset=slave bundle=gmem2
        #pragma HLS INTERFACE s_axilite port=src bundle=control
        #pragma HLS INTERFACE s_axilite port=dst bundle=control
        #pragma HLS INTERFACE s_axilite port=vertex_attr bundle=control
        #pragma HLS INTERFACE s_axilite port=vertex_to_check bundle=control
        #pragma HLS INTERFACE s_axilite port=return bundle=control
        int idx = 0;
        for (int i = 0 ; i < vertex_to_check ; i++)
        {
            READ_COMMAND_STREAM cmd;
            cmd.data.range(31,0) = PMA_GET_OUT_EDGE;
            cmd.data.range(63,32) = src[i];
            cmd_to_read.write(cmd);
            int tmp_dst = 0;
            while (tmp_dst != -1)
            {
                tmp_dst = data_to_check.read();
                dst[idx] = tmp_dst;
                idx++;
            }
        }
    }
}