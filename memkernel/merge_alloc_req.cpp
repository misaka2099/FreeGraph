#include <cstdio>
#include <hls_stream.h>
#include "data_type.h"
#include "debug.h"
#include <ap_utils.h>
extern "C"
{
    void merge_alloc_req(
        hls::stream<COMMAND_STREAM>& signal_stream,
        hls::stream<MEMORY_ALLOC_FREE_REQUEST>& req_from_vertex,
        hls::stream<MEMORY_ALLOC_FREE_REQUEST>& req_from_edge,
        hls::stream<MEMORY_ALLOC_FREE_REQUEST>& req_to_mem,
        hls::stream<VEC_INFO_STREAM>& vec_info_from_mem,
        hls::stream<VEC_INFO_STREAM>& vec_info_to_vertex,
        hls::stream<VEC_INFO_STREAM>& vec_info_to_edge
    )
    {
        #pragma HLS INTERFACE axis port=signal_stream
        #pragma HLS INTERFACE axis port=req_from_vertex
        #pragma HLS INTERFACE axis port=req_from_edge
        #pragma HLS INTERFACE axis port=req_to_mem
        #pragma HLS INTERFACE axis port=vec_info_from_mem
        #pragma HLS INTERFACE axis port=vec_info_to_vertex
        #pragma HLS INTERFACE axis port=vec_info_to_edge


        bool vertex_alloc_pending = false;
        bool vertex_free_pending = false;
        bool edge_alloc_pending = false;
        bool edge_free_pending = false;

        bool stopping = false;
        int next_worker = 0; // 0: vertex, 1: edge

        while (true)
        {
            #pragma HLS PIPELINE II=1
            
            // 1. Signal Handling
            if (!stopping && !signal_stream.empty())
            {
                auto control_signal = signal_stream.read();
                if (control_signal.data == MEM_STOP)
                    stopping = true;
                else if (control_signal.data == LOAD_BITMAP)
                {
                    MEMORY_ALLOC_FREE_REQUEST_INTERNAL req = 0;
                    req.range(67, 64) = REQ_TYPE_LOAD_BITMAP;
                    MEMORY_ALLOC_FREE_REQUEST out_req;
                    out_req.data = req;
                    out_req.last = 0;
                    req_to_mem.write(out_req);

                    #pragma HLS protocol fixed
                    ap_wait();
                    vec_info_from_mem.read();  
                }
            }

            // 2. Check in-flight
            bool in_flight = vertex_alloc_pending || vertex_free_pending || 
                             edge_alloc_pending || edge_free_pending;

            // 3. Handle Responses
            if (in_flight)
            {
                VEC_INFO_STREAM info_back;
                if (vec_info_from_mem.read_nb(info_back))
                {
                    if (vertex_alloc_pending)
                    {
                        vec_info_to_vertex.write(info_back);
                        vertex_alloc_pending = false;
                    }
                    else if (edge_alloc_pending)
                    {
                        vec_info_to_edge.write(info_back);
                        edge_alloc_pending = false;
                    }
                    else if (vertex_free_pending) vertex_free_pending = false;
                    else if (edge_free_pending) edge_free_pending = false;
                }
            }

            // 4. Stopping Logic
            if (stopping)
            {
                if (!in_flight)
                {
                    MEMORY_ALLOC_FREE_REQUEST term; term.data = 0; term.last = 1;
                    req_to_mem.write(term);
                    break;
                }
                continue;
            }

            // 5. Scheduling
            bool process_in_this_cycle = false;
            if (next_worker == 0) process_in_this_cycle = !req_from_vertex.empty() && !in_flight;
            else if (next_worker == 1) process_in_this_cycle = !req_from_edge.empty() && !in_flight;

            if (process_in_this_cycle)
            {
                MEMORY_ALLOC_FREE_REQUEST req_pkg;
                if (next_worker == 0) req_pkg = req_from_vertex.read();
                else req_pkg = req_from_edge.read();

                MEMORY_ALLOC_FREE_REQUEST_INTERNAL req_data = req_pkg.data;
                int type = req_data.range(67, 64);
                
                req_to_mem.write(req_pkg);

                if (type == REQ_TYPE_ALLOC)
                {
                    if (next_worker == 0) vertex_alloc_pending = true;
                    else edge_alloc_pending = true;
                }
                else if (type == REQ_TYPE_FREE)
                {
                    if (next_worker == 0) vertex_free_pending = true;
                    else edge_free_pending = true;
                }
            }
            
            next_worker = (next_worker + 1) % 2;
        }
    }
}