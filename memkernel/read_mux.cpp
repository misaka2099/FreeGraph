#include <cstdio>
#include <hls_stream.h>
#include "data_type.h"
#include <ap_int.h>
#include "debug.h"
#define IDLE 0
#define DECODE 1
#define COLLECTION_AND_DATA_TRANSFER 2
#define PARTITION_COMMAND 3
#define ACK_BACK 4
const int MAX_INFLIGHT_REQS = 16;
const int MAX_CHUNKS_PER_REQ = 16;
bool check_for_commit(
    int ack_count[MAX_INFLIGHT_REQS][MAX_CHUNKS_PER_REQ],
    int commit_req_id
)
{
    #pragma HLS PIPELINE
    for (int i = 0; i < MAX_CHUNKS_PER_REQ; i++) {
        if (ack_count[commit_req_id][i] > 0) {
            return false;
        }
    }
    return true;
}

extern "C"
{
    void read_mux(
        hls::stream<VEC_INFO_STREAM> &vec_info_stream,
        // hls::stream<VEC_INFO_STREAM> &vec_info_to_mem_1,
        // hls::stream<VEC_INFO_STREAM> &vec_info_to_mem_2,
        // hls::stream<VEC_INFO_STREAM> vec_info_to_mem[2],

        hls::stream<BYTE_STREAM> &data_to_app,
        hls::stream<BYTE_STREAM> &data_from_mem_1,
        hls::stream<BYTE_STREAM> &data_from_mem_2,
        // hls::stream<BYTE_STREAM> data_from_mem[2],
        hls::stream<COMMAND_STREAM> &ack_from_mem_1,
        hls::stream<COMMAND_STREAM> &ack_from_mem_2,
        // hls::stream<COMMAND_STREAM> ack_from_mem[2],
        hls::stream<COMMAND_STREAM> &ack_to_scheduler,
        // hls::stream<BYTE_STREAM> tag_to_mem[2]
        hls::stream<TAG_STREAM> &tag_vecinfo_to_mem_1,
        hls::stream<TAG_STREAM> &tag_vecinfo_to_mem_2
        // hls::stream<BYTE_STREAM> &tag_to_mem_1,
        // hls::stream<BYTE_STREAM> &tag_to_mem_2
    )
    {
        #pragma HLS INTERFACE s_axilite port=return
        #pragma HLS INTERFACE axis port=vec_info_stream
        #pragma HLS INTERFACE axis port=data_to_app
        #pragma HLS INTERFACE axis port=data_from_mem_1
        #pragma HLS INTERFACE axis port=data_from_mem_2
        #pragma HLS INTERFACE axis port=ack_from_mem_1
        #pragma HLS INTERFACE axis port=ack_from_mem_2
        #pragma HLS INTERFACE axis port=ack_to_scheduler
        #pragma HLS INTERFACE axis port=tag_vecinfo_to_mem_1
        #pragma HLS INTERFACE axis port=tag_vecinfo_to_mem_2

        #pragma HLS STREAM variable=vec_info_stream depth=64
        #pragma HLS STREAM variable=data_to_app depth=64
        #pragma HLS STREAM variable=data_from_mem_1 depth=64
        #pragma HLS STREAM variable=data_from_mem_2 depth=64
        #pragma HLS STREAM variable=ack_from_mem_1 depth=64
        #pragma HLS STREAM variable=ack_from_mem_2 depth=64
        #pragma HLS STREAM variable=ack_to_scheduler depth=64
        #pragma HLS STREAM variable=tag_vecinfo_to_mem_1 depth=64
        #pragma HLS STREAM variable=tag_vecinfo_to_mem_2 depth=64
        static int state = IDLE;
        VEC_INFO_STREAM current_job;
        int tag_num = 0;
        int cur_start_addr = 0;
        int cur_length = 0;
        int cur_chunk_channel = -1;
        int remain_lenth = 0; 
        int ack_count = 0;
        bool used[2] = {0, 0};
        while (true)
        {
            switch (state)
            {
                case IDLE:
                {
                    // HERE
                    if (!vec_info_stream.empty())
                    {
                        vec_info_stream.read(current_job);
                        // HERE
                        state = DECODE;
                    }
                    break;
                }
                case DECODE:
                {
                    // HERE
                    if (current_job.last == 1)
                    {
                        tag_vecinfo_to_mem_1.write({.last = 1});
                        tag_vecinfo_to_mem_2.write({.last = 1});
                        return;
                    }
                    cur_start_addr = current_job.data.range(31, 0);
                    cur_length = current_job.data.range(63, 32);
                    remain_lenth = cur_length;
                    state = PARTITION_COMMAND;
                    tag_num = 0;
                    ack_count = 0;
                    for (int i = 0; i < 2; i++)
                    {
                        used[i] = 0;
                    }
                    break;
                }
                case PARTITION_COMMAND:
                {
                    // HERE
                    if (remain_lenth > 0)
                    {
                        int chunk_size = (remain_lenth > READER_CHUNK_BYTES) ? READER_CHUNK_BYTES : remain_lenth;
                        TAG_STREAM chunk;
                        if (cur_start_addr / CHANNEL_CAPACITY != (cur_start_addr + chunk_size - 1) / CHANNEL_CAPACITY)
                        {
                            chunk_size = CHANNEL_CAPACITY - (cur_start_addr % CHANNEL_CAPACITY);
                        }
                        cur_length = chunk_size;
                        // send offset relative to channel local base
                        chunk.data.range(31, 0) = cur_start_addr % CHANNEL_CAPACITY;
                        chunk.data.range(63, 32) = chunk_size;
                        chunk.data.range(71, 64) = tag_num;
                        chunk.last = 0;
                        int channel_id = cur_start_addr / CHANNEL_CAPACITY;
                        // save channel for this chunk before advancing cur_start_addr
                        cur_chunk_channel = channel_id;
                        remain_lenth -= chunk_size;
                        // printf("REMAINNING LENGTH %d\n", remain_lenth);
                        tag_num++;
                        ack_count++;
                        // printf("ACK COUNT %d\n", ack_count);
                        if (channel_id == 0)
                        {
                            tag_vecinfo_to_mem_1.write(chunk);
                            used[0] = 1;
                        }
                        else  
                        {
                            tag_vecinfo_to_mem_2.write(chunk);
                            used[1] = 1;
                        }
                        state = COLLECTION_AND_DATA_TRANSFER;
                    }
                    else  
                    {
                        // HERE
                        state = ACK_BACK;
                    }
                    break;
                }
                case COLLECTION_AND_DATA_TRANSFER:
                {
                    // HERE
                    // use saved chunk channel (do not recompute from cur_start_addr which was advanced)
                    int channel_id = cur_chunk_channel;
                    if (channel_id == 0)
                    {
                      for (int i = 0; i < cur_length; i++) {
                        auto y = data_from_mem_1.read();
                        data_to_app.write(y);
                      }
                      auto ack = ack_from_mem_1.read();
                      ack_count--;
                    //   printf("GOT ACK FROM MEM 1, REMAINING COUNT %d\n", ack_count);
                    }
                    else
                    {
                      for (int i = 0; i < cur_length; i++) {
                        auto y = data_from_mem_2.read();
                        data_to_app.write(y);
                      }
                      auto ack = ack_from_mem_2.read();
                      ack_count--;
                    //   printf("GOT ACK FROM MEM 2, REMAINING COUNT %d\n", ack_count);

                    }
                    cur_start_addr += cur_length;
                    state = PARTITION_COMMAND;
                    break;
                }
                case ACK_BACK:
                {
                    // HERE
                    if (ack_count == 0)
                    {
                        COMMAND_STREAM ack;
                        for (int i = 0 ; i < 2; i++)
                        {
                            if (used[i])
                            {
                                // printf("SENDING ACK BACK %d\n", i);
                                ack.data = i;
                                ack_to_scheduler.write(ack);
                            }
                        }
                        // HERE
                        state = IDLE;
                    }
                    else  
                    {
                        printf("WAITING FOR ACK COUNT %d\n", ack_count);
                    }
                    break;
                }
            }
        }
    }
}