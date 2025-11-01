#include <cstdio>
#include <hls_stream.h>
#include "data_type.h"
#include "debug.h"



// void connect(
//     hls::stream<COMMAND_STREAM> &cmd_from_app,
//     hls::stream<VEC_INFO_STREAM> &vec_from_app,
//     hls::stream<VEC_INFO_STREAM> &vec_to_app,
//     hls::stream<VEC_INFO_STREAM> &vec_to_alloc,
//     hls::stream<VEC_INFO_STREAM> &vec_to_free,
//     hls::stream<VEC_INFO_STREAM> &vec_from_mem
// )
// {
//     if (cmd_from_app.empty() == false && vec_from_app.empty() == false)
//     {
//         auto cmd = cmd_from_app.read();
//         auto info = vec_from_app.read();
//         if (cmd.data == MEM_ALLOC)
//         {
//             vec_to_alloc.write(info);
//             auto info_back = vec_from_mem.read();
//             vec_to_app.write(info_back);
//         }
//         else if (cmd.data == MEM_FREE)
//         {
//             vec_to_free.write(info);
//         }
//     }
// }
extern "C"
{
    void app_mem_middle_ware(
        hls::stream<COMMAND_STREAM> &cmd_from_app_1,
        hls::stream<VEC_INFO_STREAM> &vec_info_from_app_1,
        hls::stream<VEC_INFO_STREAM> &vec_info_to_app_1,

        hls::stream<COMMAND_STREAM> &cmd_from_app_2,
        hls::stream<VEC_INFO_STREAM> &vec_info_from_app_2,
        hls::stream<VEC_INFO_STREAM> &vec_info_to_app_2,

        hls::stream<VEC_INFO_STREAM> &vec_info_to_alloc,
        hls::stream<VEC_INFO_STREAM> &vec_info_to_free,
        hls::stream<VEC_INFO_STREAM> &vec_info_from_mem,


        hls::stream<COMMAND_STREAM> &stop_signal
    )
    {
        #pragma HLS INTERFACE axis port=cmd_from_app_1
        #pragma HLS INTERFACE axis port=vec_info_from_app_1
        #pragma HLS INTERFACE axis port=vec_info_to_app_1
        #pragma HLS INTERFACE axis port=cmd_from_app_2
        #pragma HLS INTERFACE axis port=vec_info_from_app_2
        #pragma HLS INTERFACE axis port=vec_info_to_app_2
        #pragma HLS INTERFACE axis port=vec_info_to_alloc
        #pragma HLS INTERFACE axis port=vec_info_to_free
        #pragma HLS INTERFACE axis port=vec_info_from_mem
        #pragma HLS INTERFACE axis port=stop_signal
        bool alloc_app_1 = false;
        bool alloc_app_2 = false;
        while (true)
        {
            // Process commands from app 1
            if (stop_signal.empty() == false)
            {
                stop_signal.read();
                vec_info_to_alloc.write({.data=0, .last=1});
                vec_info_to_free.write({.data=0, .last=1});
                HERE
                break;
            }
            COMMAND_STREAM cmd;
            VEC_INFO_STREAM info;
            if (alloc_app_1 || alloc_app_2)
            {
                bool is_back = false;
                VEC_INFO_STREAM info_back;
                is_back = vec_info_from_mem.read_nb(info_back);
                if (is_back)
                {
                    if (alloc_app_1)
                    {
                        vec_info_to_app_1.write(info_back);
                        alloc_app_1 = false;
                    }
                    else if (alloc_app_2)
                    {
                        vec_info_to_app_2.write(info_back);
                        alloc_app_2 = false;
                    }
                }
            }
            if (cmd_from_app_1.empty() == false && vec_info_from_app_1.empty() == false && alloc_app_1 == false && alloc_app_2 == false)
            {
                cmd = cmd_from_app_1.read();
                info = vec_info_from_app_1.read();
                if (cmd.data == MEM_ALLOC)
                {
                    vec_info_to_alloc.write(info);
                    alloc_app_1 = true;
                }
                else if (cmd.data == MEM_FREE)
                {
                    vec_info_to_free.write(info);
                }
            }
            if (cmd_from_app_2.empty() == false && vec_info_from_app_2.empty() == false && alloc_app_2 == false && alloc_app_1 == false)
            {
                cmd = cmd_from_app_2.read();
                info = vec_info_from_app_2.read();
                if (cmd.data == MEM_ALLOC)
                {
                    vec_info_to_alloc.write(info);
                    alloc_app_2 = true;
                }
                else if (cmd.data == MEM_FREE)
                {
                    vec_info_to_free.write(info);
                }
            }
            // connect(
            //     cmd_from_app_1,
            //     vec_info_from_app_1,
            //     vec_info_to_app_1,
            //     vec_info_to_alloc,
            //     vec_info_to_free,
            //     vec_info_from_mem
            // );
            // connect(
            //     cmd_from_app_2,
            //     vec_info_from_app_2,
            //     vec_info_to_app_2,
            //     vec_info_to_alloc,
            //     vec_info_to_free,
            //     vec_info_from_mem
            // );
        }
    }
}