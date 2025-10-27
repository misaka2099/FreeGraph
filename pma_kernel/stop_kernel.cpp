#include "data_type.h"
#include "debug.h"
#include <hls_stream.h>
extern "C"
{
    void stop_kernel(
        hls::stream<COMMAND_STREAM> &stop_signal_to_mem
    )
    {
        #pragma HLS INTERFACE axis port=stop_signal_to_mem
        stop_signal_to_mem.write(COMMAND_STREAM{.data = MEM_STOP});
    }
}