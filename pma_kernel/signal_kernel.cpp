#include "data_type.h"
#include "debug.h"
#include <hls_stream.h>
extern "C"
{
    void signal_kernel(
        int signal,
        hls::stream<COMMAND_STREAM> &signal_to_alloc_free_1,
        hls::stream<COMMAND_STREAM> &signal_to_alloc_free_2,
        hls::stream<COMMAND_STREAM> &signal_to_mem_access_1,
        hls::stream<COMMAND_STREAM> &signal_to_mem_access_2
    )
    {
        #pragma HLS INTERFACE s_axilite port=signal 
        #pragma HLS INTERFACE axis port=signal_to_alloc_free_1
        #pragma HLS INTERFACE axis port=signal_to_alloc_free_2
        #pragma HLS INTERFACE axis port=signal_to_mem_access_1
        #pragma HLS INTERFACE axis port=signal_to_mem_access_2
        signal_to_alloc_free_1.write(COMMAND_STREAM{.data = signal});
        signal_to_alloc_free_2.write(COMMAND_STREAM{.data = signal});
        if (signal == MEM_STOP)
        {
            signal_to_mem_access_1.write(COMMAND_STREAM{.data = signal});
            signal_to_mem_access_2.write(COMMAND_STREAM{.data = signal});

        }
    }
}