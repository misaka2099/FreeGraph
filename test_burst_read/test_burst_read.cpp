#include <ap_int.h>
extern "C"
{
    void test_cast_burst_read(
        int *in,
        int *out
    )
    {
        #pragma HLS INTERFACE m_axi port=in bundle=gmem0
        #pragma HLS INTERFACE m_axi port=out bundle=gmem1
        // for (int i = 0 ; i < 16 ; i++)
        // {
        //     out[i] = in[i] + 1;
        // }
        ap_uint<512>* wide_hbm_ptr = (ap_uint<512>*)in;
        int buffer[16];
        ap_uint<512>* buffer_ptr = (ap_uint<512>*) buffer;
        buffer_ptr[0] = wide_hbm_ptr[0];
        for (int i = 0 ; i < 16 ; i++)
        {
            buffer[i]++;
        }
        ap_uint<512>* out_ptr = (ap_uint<512>*) out;
        out_ptr[0] = buffer_ptr[0];
    }
}