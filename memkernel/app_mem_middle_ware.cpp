#include <hls_stream.h>
#include "data_type.h"
extern "C"
{
    void app_mem_middle_ware(
        hls::stream<COMMAND_STREAM> &cmd_from_app_1,
        hls::stream<COMMAND_STREAM> &cmd_from_app_2,
        hls::stream<COMMAND_STREAM> &cmd_to_mem,
        hls::stream<BYTE_STREAM> &data_from_app_1,
        hls::stream<BYTE_STREAM> &data_from_app_2,
        hls::stream<BYTE_STREAM> &data_to_mem,
        hls::stream<BYTE_STREAM> &data_from_mem,
        hls::stream<BYTE_STREAM> &data_to_app_1,
        hls::stream<BYTE_STREAM> &data_to_app_2,
        hls::stream<VEC_INFO_STREAM> &vec_info_from_app_1,
        hls::stream<VEC_INFO_STREAM> &vec_info_from_app_2,
        hls::stream<VEC_INFO_STREAM> &vec_info_from_mem,
        hls::stream<VEC_INFO_STREAM> &vec_info_to_app_1,
        hls::stream<VEC_INFO_STREAM> &vec_info_to_app_2,
        hls::stream<VEC_INFO_STREAM> &vec_info_to_mem
    )
    {
        #pragma HLS INTERFACE axis port=cmd_from_app_1
        #pragma HLS INTERFACE axis port=cmd_from_app_2
        #pragma HLS INTERFACE axis port=cmd_to_mem
        #pragma HLS INTERFACE axis port=data_from_app_1
        #pragma HLS INTERFACE axis port=data_from_app_2
        #pragma HLS INTERFACE axis port=data_to_mem
        #pragma HLS INTERFACE axis port=data_from_mem
        #pragma HLS INTERFACE axis port=data_to_app_1
        #pragma HLS INTERFACE axis port=data_to_app_2
        #pragma HLS INTERFACE axis port=vec_info_from_app_1
        #pragma HLS INTERFACE axis port=vec_info_from_app_2
        #pragma HLS INTERFACE axis port=vec_info_from_mem
        #pragma HLS INTERFACE axis port=vec_info_to_app_1
        #pragma HLS INTERFACE axis port=vec_info_to_app_2
        #pragma HLS INTERFACE axis port=vec_info_to_mem
        #pragma HLS INTERFACE ap_ctrl_none port=return
        while (true)
        {
            // Process commands from app 1
            if (!cmd_from_app_1.empty())
            {
                COMMAND_STREAM cmd = cmd_from_app_1.read();
                cmd_to_mem.write(cmd);
                if (cmd.data == MEM_WRITE)
                {
                    auto vec_info = vec_info_from_app_1.read();
                    vec_info_to_mem.write(vec_info);
                    for (int i = 0 ; i < vec_info.data.range(63,32) ; i++)
                    {
                        auto byte = data_from_app_1.read();
                        data_to_mem.write(byte);
                    }
                }
                else if (cmd.data == MEM_READ)
                {
                    auto vec_info = vec_info_from_app_1.read();
                    vec_info_to_mem.write(vec_info);
                    for (int i = 0 ; i < vec_info.data.range(63,32) ; i++)
                    {
                        auto byte = data_from_mem.read();
                        data_to_app_1.write(byte);
                    }
                }
                else if (cmd.data == MEM_ALLOC)
                {
                    auto vec_info = vec_info_from_app_1.read();
                    vec_info_to_mem.write(vec_info);
                    auto back = vec_info_from_mem.read();
                    vec_info_to_app_1.write(back);
                }
                else if (cmd.data == MEM_FREE)
                {
                    auto vec_info = vec_info_from_app_1.read();
                    vec_info_to_mem.write(vec_info);
                }
                else if (cmd.data == MEM_STOP)
                {
                    break;
                }
            }

            // Process commands from app 2
            if (!cmd_from_app_2.empty())
            {
                COMMAND_STREAM cmd = cmd_from_app_2.read();
                cmd_to_mem.write(cmd);
                if (cmd.data == MEM_WRITE)
                {
                    auto vec_info = vec_info_from_app_2.read();
                    vec_info_to_mem.write(vec_info);
                    for (int i = 0 ; i < vec_info.data.range(63,32) ; i++)
                    {
                        auto byte = data_from_app_2.read();
                        data_to_mem.write(byte);
                    }
                }
                else if (cmd.data == MEM_READ)
                {
                    auto vec_info = vec_info_from_app_2.read();
                    vec_info_to_mem.write(vec_info);
                    for (int i = 0 ; i < vec_info.data.range(63,32) ; i++)
                    {
                        auto byte = data_from_mem.read();
                        data_to_app_2.write(byte);
                    }
                }
                else if (cmd.data == MEM_ALLOC)
                {
                    auto vec_info = vec_info_from_app_2.read();
                    vec_info_to_mem.write(vec_info);
                    auto back = vec_info_from_mem.read();
                    vec_info_to_app_2.write(back);
                }
                else if (cmd.data == MEM_FREE)
                {
                    auto vec_info = vec_info_from_app_2.read();
                    vec_info_to_mem.write(vec_info);
                }
                else if (cmd.data == MEM_STOP)
                {
                    break;
                }
            }
        }
    }
}