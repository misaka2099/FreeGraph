#include <cstdio>
#include <hls_stream.h>
#include "data_type.h"
#include "debug.h"
#include <ap_utils.h>
#define MAX_WORKERS 4
extern "C"
{
    void app_mem_middle_ware(
        hls::stream<MEMORY_ALLOC_FREE_REQUEST> &req_from_app_1,
        hls::stream<VEC_INFO_STREAM> &vec_info_to_app_1,

        hls::stream<MEMORY_ALLOC_FREE_REQUEST> &req_from_app_2,
        hls::stream<VEC_INFO_STREAM> &vec_info_to_app_2,

        hls::stream<MEMORY_ALLOC_FREE_REQUEST> &req_from_app_3,
        hls::stream<VEC_INFO_STREAM> &vec_info_to_app_3,

        hls::stream<MEMORY_ALLOC_FREE_REQUEST> &req_from_app_4,
        hls::stream<VEC_INFO_STREAM> &vec_info_to_app_4,

        hls::stream<MEMORY_ALLOC_FREE_REQUEST> &vec_info_to_mem,
        hls::stream<VEC_INFO_STREAM> &vec_info_from_mem,

        hls::stream<COMMAND_STREAM> &signal
    )
    {
        #pragma HLS INTERFACE axis port=req_from_app_1
        #pragma HLS INTERFACE axis port=vec_info_to_app_1
        #pragma HLS INTERFACE axis port=req_from_app_2
        #pragma HLS INTERFACE axis port=vec_info_to_app_2
        #pragma HLS INTERFACE axis port=req_from_app_3
        #pragma HLS INTERFACE axis port=vec_info_to_app_3
        #pragma HLS INTERFACE axis port=req_from_app_4
        #pragma HLS INTERFACE axis port=vec_info_to_app_4

        #pragma HLS INTERFACE axis port=vec_info_to_mem
        #pragma HLS INTERFACE axis port=vec_info_from_mem
        #pragma HLS INTERFACE axis port=signal

        bool alloc_app[4];
        bool free_app[4];
        for (int i = 0 ; i < 4 ; i++)
        {
            alloc_app[i] = false;
            free_app[i] = false;
        }
        bool stopping = false;
        int next_worker_ptr = 0;
        while (true)
        {
            // 进入停止流程：不再接收新请求，只清空在途请求
            #pragma HLS PIPELINE II = 1
            if (!stopping && signal.empty() == false)
            {
                auto control_signal = signal.read();
                if (control_signal.data == MEM_STOP)
                    stopping = true;
                else if (control_signal.data == LOAD_BITMAP)
                {
                    // vec_info_to_mem.write({.data = 0, .last = 0});
                    MEMORY_ALLOC_FREE_REQUEST_INTERNAL req = 0;
                    req.range(67, 64) = REQ_TYPE_LOAD_BITMAP;
                    MEMORY_ALLOC_FREE_REQUEST out_req;
                    out_req.data = req;
                    out_req.last = 0;
                    vec_info_to_mem.write(out_req);
                    // cmd_to_mem.write({.data = LOAD_BITMAP, .last = 0});
                    #pragma HLS protocol fixed
                    ap_wait();
                    vec_info_from_mem.read();  
                }
            }

            // check in-flight transactions
            bool in_flight = alloc_app[0] || alloc_app[1] || alloc_app[2] || alloc_app[3] ||
                free_app[0] || free_app[1] || free_app[2] || free_app[3] ;
            if (in_flight)
            {
                VEC_INFO_STREAM info_back;
                bool is_back = vec_info_from_mem.read_nb(info_back);
                if (is_back)
                {
                    if (alloc_app[0])
                    {
                        // HERE
                        vec_info_to_app_1.write(info_back);
                        alloc_app[0] = false;
                    }
                    else if (alloc_app[1])
                    {
                        // HERE
                        vec_info_to_app_2.write(info_back);
                        alloc_app[1] = false;
                    }
                    else if (alloc_app[2])
                    {
                        // HERE
                        vec_info_to_app_3.write(info_back);
                        alloc_app[2] = false; // free 不回传给 app
                    }
                    else if (alloc_app[3])
                    {
                        // HERE
                        vec_info_to_app_4.write(info_back);
                        alloc_app[3] = false;
                    }
                    else if (free_app[0]) free_app[0] = false;
                    else if (free_app[1]) free_app[1] = false;
                    else if (free_app[2]) free_app[2] = false;
                    else if (free_app[3]) free_app[3] = false;
                }
            }

            if (stopping)
            {
                // HERE
                if (!in_flight)
                {
                    // HERE
                    MEMORY_ALLOC_FREE_REQUEST term; term.data = 0; term.last = 1;
                    vec_info_to_mem.write(term);
                    break;
                }
                continue;
            }

            bool process_in_this_cycle = false;
            // Round robin scheduling
            switch (next_worker_ptr) 
            {
                case 0: process_in_this_cycle = !req_from_app_1.empty() && !in_flight; break;
                case 1: process_in_this_cycle = !req_from_app_2.empty() && !in_flight; break;
                case 2: process_in_this_cycle = !req_from_app_3.empty() && !in_flight; break;
                case 3: process_in_this_cycle = !req_from_app_4.empty() && !in_flight; break;
            }
            if (process_in_this_cycle)
            {
                MEMORY_ALLOC_FREE_REQUEST req_pkg;
                switch (next_worker_ptr) 
                {
                    case 0: req_pkg = req_from_app_1.read(); break;
                    case 1: req_pkg = req_from_app_2.read(); break;
                    case 2: req_pkg = req_from_app_3.read(); break;
                    case 3: req_pkg = req_from_app_4.read(); break;
                }
                
                MEMORY_ALLOC_FREE_REQUEST_INTERNAL req = req_pkg.data;
                int type = req.range(67, 64);
                
                if (type == REQ_TYPE_ALLOC)
                {
                    vec_info_to_mem.write(req_pkg);
                    alloc_app[next_worker_ptr] = true;
                }
                else if (type == REQ_TYPE_FREE)
                {
                    vec_info_to_mem.write(req_pkg);
                    free_app[next_worker_ptr] = true;
                }
            }
            next_worker_ptr = (next_worker_ptr + 1) % MAX_WORKERS;
        }
    }
}