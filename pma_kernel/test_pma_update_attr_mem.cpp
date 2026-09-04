#include "data_type.h"
#include <hls_stream.h>
extern "C"
{
    void pma_update_attr(
        hls::stream<PMA_UPDATE_MESSAGE>& cmd_stream,
        hls::stream<VERTEX_ATTR_STREAM_TYPE>& data_stream,
        VERTEX_ATTR_STREAM_TYPE* attr_mem_ptr
    )
    {
        while (true)
        {
            PMA_UPDATE_MESSAGE cmd = cmd_stream.read();
            if (cmd.last == 1 && cmd.data == PMA_STOP)
            {
                break;
            }
            int src = cmd.data.range(31,0);
            VERTEX_ATTR_STREAM_TYPE new_attr = data_stream.read();
            attr_mem_ptr[src] = new_attr;
        }
    }
}