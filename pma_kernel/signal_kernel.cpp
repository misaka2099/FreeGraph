#include "data_type.h"
#include "debug.h"
#include <hls_stream.h>
extern "C"
{
    void signal_kernel(
        int signal,
        hls::stream<COMMAND_STREAM> &signal_to_alloc_free_1,
        hls::stream<COMMAND_STREAM> &signal_to_alloc_free_2,
        hls::stream<COMMAND_STREAM> &signal_to_alloc_free_3,
        hls::stream<COMMAND_STREAM> &signal_to_alloc_free_4,
        hls::stream<COMMAND_STREAM> &signal_to_alloc_free_5,
        hls::stream<COMMAND_STREAM> &signal_to_alloc_free_6,
        hls::stream<COMMAND_STREAM> &signal_to_alloc_free_7,
        hls::stream<COMMAND_STREAM> &signal_to_alloc_free_8,
        hls::stream<COMMAND_STREAM> &signal_to_mem_access
    )
    {
        #pragma HLS INTERFACE axis port=signal_to_alloc_free_1
        #pragma HLS INTERFACE axis port=signal_to_alloc_free_2
        #pragma HLS INTERFACE axis port=signal_to_alloc_free_3
        #pragma HLS INTERFACE axis port=signal_to_alloc_free_4
        #pragma HLS INTERFACE axis port=signal_to_alloc_free_5
        #pragma HLS INTERFACE axis port=signal_to_alloc_free_6
        #pragma HLS INTERFACE axis port=signal_to_alloc_free_7
        #pragma HLS INTERFACE axis port=signal_to_alloc_free_8
        #pragma HLS INTERFACE s_axilite port=signal 
        #pragma HLS INTERFACE axis port=signal_to_mem_access
        signal_to_alloc_free_1.write(COMMAND_STREAM{.data = signal});
        signal_to_alloc_free_2.write(COMMAND_STREAM{.data = signal});
        signal_to_alloc_free_3.write(COMMAND_STREAM{.data = signal});
        signal_to_alloc_free_4.write(COMMAND_STREAM{.data = signal});
        signal_to_alloc_free_5.write(COMMAND_STREAM{.data = signal});
        signal_to_alloc_free_6.write(COMMAND_STREAM{.data = signal});
        signal_to_alloc_free_7.write(COMMAND_STREAM{.data = signal});
        signal_to_alloc_free_8.write(COMMAND_STREAM{.data = signal});
        if (signal == MEM_STOP)
            signal_to_mem_access.write(COMMAND_STREAM{.data = signal});
    }
}