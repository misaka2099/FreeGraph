#include "data_type.h"
#include <cstdio>
#include <hls_stream.h>
#include <ap_int.h>
#include "debug.h"
extern "C"
{
    void memlock(
        hls::stream<VEC_INFO_STREAM> &vec_info_stream,
        hls::stream<COMMAND_STREAM> &cmd_stream,
        hls::stream<COMMAND_STREAM> &ack_write_stream,
        hls::stream<COMMAND_STREAM> &ack_read_stream,
        hls::stream<COMMAND_STREAM> &ack_alloc_stream,
        hls::stream<COMMAND_STREAM> &ack_free_stream,
        hls::stream<VEC_INFO_STREAM>& vec_to_write_stream,
        hls::stream<VEC_INFO_STREAM>& vec_to_read_stream,
        hls::stream<VEC_INFO_STREAM>& vec_to_alloc_stream,
        hls::stream<VEC_INFO_STREAM>& vec_to_free_stream
    )
    {
        #pragma HLS INTERFACE axis port=vec_info_stream
        #pragma HLS INTERFACE axis port=cmd_stream
        #pragma HLS INTERFACE axis port=ack_write_stream
        #pragma HLS INTERFACE axis port=ack_read_stream
        #pragma HLS INTERFACE axis port=ack_alloc_stream
        #pragma HLS INTERFACE axis port=ack_free_stream
        #pragma HLS INTERFACE axis port=vec_to_write_stream
        #pragma HLS INTERFACE axis port=vec_to_read_stream
        #pragma HLS INTERFACE axis port=vec_to_alloc_stream
        #pragma HLS INTERFACE axis port=vec_to_free_stream
        #pragma HLS INTERFACE s_axilite port=return
        static int write_counter[CHANNEL_NUM] = {0};
        static int read_counter[CHANNEL_NUM] = {0};
        static int alloc_counter = {0};
        static int free_counter = {0};
        static int cmd_buffer[COMMAND_BUFFER_SIZE] = {-1};
        static int cmd_number = 0;
        for (int i = 0 ; i < COMMAND_BUFFER_SIZE; i++)
        {
            cmd_buffer[i] = -1;
        }
        static VEC_INFO vec_buffer[COMMAND_BUFFER_SIZE];
        // int cur_buffer_size = 0;
        static int buffer_ptr = 0;
        // static VEC_INFO info;
        int cmd = 0;
        while (true)
        {
            // HERE
            if (cmd_buffer[buffer_ptr] == -1 && !vec_info_stream.empty() && !cmd_stream.empty() )
            {
                // HERE
                int tmp_cmd = cmd_stream.read().data;
                VEC_INFO info = vec_info_stream.read().data;
                // cur_buffer_size++;
                vec_buffer[buffer_ptr] = info;
                cmd_buffer[buffer_ptr] = tmp_cmd;
                buffer_ptr = (buffer_ptr + 1) % COMMAND_BUFFER_SIZE;
                // printf("CUR  buffer ptr = %d, cmd_number = %d\n", buffer_ptr, cmd_number);
                cmd_number++;
            }
            COMMAND_STREAM ack_w, ack_r, ack_a, ack_f;
            bool has_ack_w = ack_write_stream.read_nb(ack_w);
            bool has_ack_r = ack_read_stream.read_nb(ack_r);
            bool has_ack_a = ack_alloc_stream.read_nb(ack_a);
            bool has_ack_f = ack_free_stream.read_nb(ack_f);
            if (has_ack_w) {
                // HERE
                write_counter[ack_w.data]--;
                // printf("Channel %d write counter: %d\n", int(ack_w.data), write_counter[ack_w.data]);
                // HERE
            }
            if (has_ack_r) {
                // HERE
                read_counter[ack_r.data]--;
                // printf("Channel %d read counter: %d\n", int(ack_r.data), read_counter[ack_r.data]);
            }
            if (has_ack_a) {
                // HERE
                alloc_counter--;
            }
            if (has_ack_f) {
                // HERE
                free_counter--;
                // printf("Channel %d free counter: %d\n", int(ack_f.data), free_counter[ack_f.data]);
            }
            // HERE
            for (int i = 0 ; i < COMMAND_BUFFER_SIZE; i++)
            {
                // HERE
                
                cmd = cmd_buffer[(buffer_ptr+i) % COMMAND_BUFFER_SIZE];
                VEC_INFO info = vec_buffer[(buffer_ptr+i) % COMMAND_BUFFER_SIZE];
                if (cmd == -1) continue;
                // printf("cmd = %d, cmd_number = %d\n", cmd, cmd_number); 
                if (cmd == MEM_WRITE)
                {
                    // HERE
                    int start_pos = info.range(31, 0);
                    int len = info.range(63, 32);
                    int channel_start = start_pos / CHANNEL_CAPACITY;
                    int channel_end = (start_pos + len - 1) / CHANNEL_CAPACITY;
                    // don't busy-wait here; if cannot proceed, skip this slot so
                    // the outer loop can handle acks and other slots.
                    bool to_continue = false;
                    for (int i = channel_start; i <= channel_end; ++i) {
                        if (read_counter[i] > 0  || write_counter[i] >= 8) {
                            to_continue = true;
                            break;
                        }
                    }
                    if (to_continue) {
                        continue;
                    }
                    // if (read_counter[channel] > 0  || write_counter[channel] >= 8) {
                    //     // cannot issue now, try other buffer entries
                    //     continue;
                    // }
                    // HERE
                    vec_to_write_stream.write({.data=info});
                    // printf("CHANNEL START %d, CHANNEL END %d\n", channel_start, channel_end);
                    for (int i = channel_start; i <= channel_end; ++i) {
                        write_counter[i] ++;
                        // printf("Channel %d write counter: %d\n", i, write_counter[i]);
                    }
                    // write_counter[channel] ++;
                    // HERE
                    cmd_buffer[(buffer_ptr+i) % COMMAND_BUFFER_SIZE] = -1;
                    // cur_buffer_size--;
                    cmd_number--;
                }
                else if (cmd == MEM_READ)
                {
                    // HERE
                    int start_pos = info.range(31, 0);
                    int len = info.range(63, 32);
                    int channel_start = start_pos / CHANNEL_CAPACITY;
                    int channel_end = (start_pos + len - 1) / CHANNEL_CAPACITY;
                    // int channel = start_pos / CHANNEL_CAPACITY;

                    bool to_continue = false;
                    for (int i = channel_start; i <= channel_end; ++i) {
                        if (write_counter[i] > 0  || read_counter[i] >= 8) {
                            to_continue = true;
                            // HERE
                            break;
                        }
                    }
                    if (to_continue) {
                        continue;
                    }
                    // if (write_counter[channel] > 0  || read_counter[channel] >= 8 || free_counter[channel] > 0) {
                    //     // cannot issue now, try other buffer entries
                    //     continue;
                    // }
                    // HERE
                    vec_to_read_stream.write({.data=info});
                    // read_counter[channel] ++;
                    for (int i = channel_start; i <= channel_end; ++i) {
                        read_counter[i] ++;
                    }
                    
                    // HERE
                    
                    cmd_buffer[(buffer_ptr+i) % COMMAND_BUFFER_SIZE] = -1;
                    // cur_buffer_size--;
                    cmd_number--;
                }
                else if (cmd == MEM_ALLOC)
                {
                    // HERE
                    if (free_counter > 0)
                    {
                        continue;
                    }
                    vec_to_alloc_stream.write({.data=info});
                    alloc_counter++;                   
                    cmd_buffer[(buffer_ptr+i) % COMMAND_BUFFER_SIZE] = -1;
                    // cur_buffer_size--;
                    cmd_number--;
                }
                else if (cmd == MEM_FREE)
                {
                    // HERE
                    int start_pos = info.range(31, 0);
                    int channel = start_pos / CHANNEL_CAPACITY;
                    if (write_counter[channel] > 0 || read_counter[channel] > 0 || alloc_counter > 0) {
                        // cannot issue now, try other buffer entries
                        continue;
                    }
                    vec_to_free_stream.write({.data=info});
                    free_counter++;                    
                    cmd_buffer[(buffer_ptr+i) % COMMAND_BUFFER_SIZE] = -1;
                    // cur_buffer_size--;
                    cmd_number--;
                }
                else if (cmd == MEM_STOP)
                {
                    if (cmd_number != 1) {
                        continue;
                    }
                    // HERE
                    vec_to_alloc_stream.write({.last = 1});
                    vec_to_free_stream.write({.last = 1});
                    vec_to_read_stream.write({.last = 1});
                    vec_to_write_stream.write({.last = 1});
                    return;
                }
            }
        }
    }
}