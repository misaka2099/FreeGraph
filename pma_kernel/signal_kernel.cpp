#include "data_type.h"
#include "debug.h"
#include <hls_stream.h>
extern "C"
{
    void signal_kernel(
        int signal,
        hls::stream<COMMAND_STREAM> &signal_to_alloc_free,
        hls::stream<COMMAND_STREAM> &signal_to_mem_access
    )
    {
        #pragma HLS INTERFACE axis port=signal_to_alloc_free
        #pragma HLS INTERFACE s_axilite port=signal 
        #pragma HLS INTERFACE axis port=signal_to_mem_access
        signal_to_alloc_free.write(COMMAND_STREAM{.data = signal});
        if (signal == MEM_STOP)
            signal_to_mem_access.write(COMMAND_STREAM{.data = signal});
    }
}