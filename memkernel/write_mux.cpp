#include <cstdio>
#include <hls_stream.h>
#include "data_type.h"
#include "debug.h"
#include "debug.h"
#include <ap_int.h>

#define IDLE 0
#define DECODE_AND_DISPATCH 1


extern "C"
{
    void write_mux(
        hls::stream<VEC_INFO_STREAM> &vec_info_stream,
        hls::stream<VEC_INFO_STREAM> &vec_info_to_mem_1,
        hls::stream<VEC_INFO_STREAM> &vec_info_to_mem_2,
        // hls::stream<VEC_INFO_STREAM> vec_info_to_mem[2],
        hls::stream<BYTE_STREAM> &data_from_app,
        hls::stream<BYTE_STREAM> &data_to_mem_1,
        hls::stream<BYTE_STREAM> &data_to_mem_2,
        // hls::stream<BYTE_STREAM> data_to_mem[2],
        hls::stream<COMMAND_STREAM> &ack_from_mem_1,
        hls::stream<COMMAND_STREAM> &ack_from_mem_2,
        // hls::stream<COMMAND_STREAM> ack_from_mem[2],
        hls::stream<COMMAND_STREAM> &ack_to_scheduler
    )
    {
        #pragma HLS INTERFACE s_axilite port=return
        #pragma HLS INTERFACE axis port=vec_info_stream
        #pragma HLS INTERFACE axis port=vec_info_to_mem_1 
        #pragma HLS INTERFACE axis port=vec_info_to_mem_2
        #pragma HLS INTERFACE axis port=data_from_app
        #pragma HLS INTERFACE axis port=data_to_mem_1
        #pragma HLS INTERFACE axis port=data_to_mem_2
        #pragma HLS INTERFACE axis port=ack_from_mem_1 
        #pragma HLS INTERFACE axis port=ack_from_mem_2
        #pragma HLS INTERFACE axis port=ack_to_scheduler

        #pragma HLS STREAM variable=vec_info_stream depth=64
        #pragma HLS STREAM variable=vec_info_to_mem_1 depth=64
        #pragma HLS STREAM variable=vec_info_to_mem_2 depth=64
        #pragma HLS STREAM variable=data_from_app depth=64
        #pragma HLS STREAM variable=data_to_mem_1 depth=64
        #pragma HLS STREAM variable=data_to_mem_2 depth=64
        #pragma HLS STREAM variable=ack_from_mem_1 depth=64
        #pragma HLS STREAM variable=ack_from_mem_2 depth=64
        #pragma HLS STREAM variable=ack_to_scheduler depth=64
        static int state = IDLE;
        VEC_INFO_STREAM current_job;
        while (true)
        {
            #pragma HLS PIPELINE II=1
            COMMAND_STREAM ack1, ack2;
            bool has_ack1 = ack_from_mem_1.read_nb(ack1);
            bool has_ack2 = ack_from_mem_2.read_nb(ack2);
            if (has_ack1) {
                // HERE
                ack_to_scheduler.write({.data=0});
            }
            if (has_ack2) {
                // HERE
                ack_to_scheduler.write({.data=1});
            }
             switch (state) {
                case IDLE: {
                    if (vec_info_stream.read_nb(current_job)) {
                        state = DECODE_AND_DISPATCH;
                    }
                    if (current_job.last == 1)
                    {
                        // HERE
                        vec_info_to_mem_1.write({.last=1});
                        vec_info_to_mem_2.write({.last=1});
                        return;
                    }
                    break;
                }
                case DECODE_AND_DISPATCH: {
                    // HERE
                    int start_pos = current_job.data.range(31, 0);
                    int total_len = current_job.data.range(63, 32);
                    int current_pos = start_pos;
                    int remaining_len = total_len;
                    // This loop breaks down the request into non-crossing chunks
                    // It will execute multiple times if the write crosses channels
                    while (remaining_len > 0) {
                        // HERE
                        int channel_idx = current_pos / CHANNEL_CAPACITY;
                        int channel_start_addr = channel_idx * CHANNEL_CAPACITY;
                        int channel_end_addr = channel_start_addr + CHANNEL_CAPACITY;
                        
                        int chunk_start_addr_abs = current_pos;
                        int chunk_start_addr_rel = chunk_start_addr_abs % CHANNEL_CAPACITY;
                        
                        // Calculate how much can be written in the current channel
                        int len_in_this_chunk = channel_end_addr - chunk_start_addr_abs;
                        if (len_in_this_chunk > remaining_len) {
                            len_in_this_chunk = remaining_len;
                        }

                        // Create and send the header for this chunk
                        VEC_INFO header;
                        header.range(31, 0) = chunk_start_addr_rel;
                        header.range(63, 32) = len_in_this_chunk;
                        // std::cout << "Dispatching write chunk: start_pos=" << chunk_start_addr_abs 
                        //           << " (rel=" << chunk_start_addr_rel << "), len=" << len_in_this_chunk 
                        //           << ", channel=" << channel_idx << std::endl;
                        // HERE
                        if (channel_idx == 0) {
                            vec_info_to_mem_1.write({.data=header});
                        } else { // channel_idx == 1
                            vec_info_to_mem_2.write({.data=header});
                        }
                        // vec_info_to_mem[channel_idx].write({.data=header});
                        // Stream the payload for this chunk
                        for (int i = 0; i < len_in_this_chunk; i++) {
                            #pragma HLS PIPELINE
                            BYTE_STREAM data = data_from_app.read();
                            // printf("ITERATION %d: data=%02x\n", i, data.data);
                            if (channel_idx == 0) {
                                // HERE
                                data_to_mem_1.write(data);
                            } else { // channel_idx == 1
                                data_to_mem_2.write(data);
                            }
                            // data_to_mem[channel_idx].write(data);
                        }

                        // Update counters for the next iteration of the while loop
                        remaining_len -= len_in_this_chunk;
                        current_pos += len_in_this_chunk;
                    }
                    // HERE
                    // All chunks dispatched, return to IDLE
                    state = IDLE;
                    break;
                }
            }
        }
    }
}