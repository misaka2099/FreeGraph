#include "data_type.h"
#include <hls_stream.h>
extern "C"
{
    void command_router(
        hls::stream<COMMAND_STREAM> &cmd_from_gas,
        hls::stream<COMMAND_STREAM> &cmd_from_update,
        hls::stream<COMMAND_STREAM> &cmd_to_cached_pma
    )
    {
        while (true)
        {
            COMMAND_STREAM cmd;
            if (!cmd_from_update.empty())
            {
                cmd = cmd_from_update.read();
                cmd_to_cached_pma.write(cmd);
            }
            if (!cmd_from_gas.empty())
            {
                cmd = cmd_from_gas.read();
                cmd_to_cached_pma.write(cmd);
            }
            if (cmd.data == PMA_STOP)
            {
                
                break;
            }
        }
        return;
    }
}