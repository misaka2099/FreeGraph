#include "data_type.h"
#include "debug.h"
#include "debug.h"
#include "hls_stream.h"
#include <cstdio>

void flush_buffer(
    int buffer_idx,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_pma,
    ap_uint<128> msg_buffer[MAX_COMMAND_BUFFER_NUM][MAX_COMMAND_BUFFER_LENGTH],
    int *count_arr
)
{
    int c = count_arr[buffer_idx];
    for (int j = 0 ; j < c; j++) {
        #pragma HLS PIPELINE II=1
        PMA_UPDATE_MESSAGE tmp;
        tmp.data = msg_buffer[buffer_idx][j];
        tmp.last = (j == c - 1) ? 1 : 0;
        cmd_to_pma.write(tmp);
        // printf("Flushing buffer %d, command type: %d\n", buffer_idx, int(tmp.data.range(97,96)));
        // printf("SRC =%d, DST=%d\n", int(tmp.data.range(31,0)), int(tmp.data.range(63,32)));
    }
    count_arr[buffer_idx] = 0;
}
extern "C"
{
    void update_message_router(
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_from_generator,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_pma
        // hls::stream<CLEAR_ICON> &clear_message_from_generator
    )
    {
        #pragma HLS INTERFACE axis port=cmd_from_generator
        #pragma HLS INTERFACE axis port=cmd_to_pma
        // #pragma HLS INTERFACE axis port=clear_message_from_generator
        
        ap_uint<128> msg_buffer[MAX_COMMAND_BUFFER_NUM][MAX_COMMAND_BUFFER_LENGTH];
        #pragma HLS BIND_STORAGE variable=msg_buffer type=RAM_2P impl=BRAM
        int count[MAX_COMMAND_BUFFER_NUM]; 
        for (int i = 0 ; i < MAX_COMMAND_BUFFER_NUM ; i++)
        {
            #pragma HLS UNROLL
            for (int j = 0 ; j < MAX_COMMAND_BUFFER_LENGTH ; j++)
            {
                #pragma HLS UNROLL
                msg_buffer[i][j] = 0;
            }
            count[i] = 0;
        }

        while (true)
        {
            
            // bool res = clear_message_from_generator.read_nb(clear_icon);
            
            PMA_UPDATE_MESSAGE cmd;
            auto is_cmd = cmd_from_generator.read_nb(cmd);
            if (!is_cmd)
            {
                continue;
            }
            bool res = (cmd.data == -1);
            if (res)
            {
                HERE
                for (int i = 0 ; i < MAX_COMMAND_BUFFER_NUM ; i++)
                {
                    flush_buffer(
                        i,
                        cmd_to_pma,
                        msg_buffer,
                        count
                    );
                }
                if (cmd.last == 1)
                {
                    HERE
                    cmd_to_pma.write({.data=PMA_STOP, .last=1});
                    return;
                }
                continue;   
            }
            // printf("SRC =%d, DST=%d, TYPE=%d\n", int(cmd.data.range(31,0)), int(cmd.data.range(63,32)), int(cmd.data.range(97,96)));
            int hash_idx = (cmd.data.range(31,0) / VERTEX_PER_PMA) % (MAX_COMMAND_BUFFER_NUM / 2);
            // printf("HASH IDX BEFORE DIRECTION CHECK: %d\n", (int)hash_idx);
            auto cmd_data = cmd.data;
            int pma_index = cmd_data.range(31,0) / VERTEX_PER_PMA;
            // HERE
            if (cmd_data[98] == OUT_DIRECTION)
            {
                hash_idx += (MAX_COMMAND_BUFFER_NUM / 2);
            }
            // printf("HASH IDX AFTER DIRECTION CHECK: %d\n", (int)hash_idx);
            if (count[hash_idx] < MAX_COMMAND_BUFFER_LENGTH)
            {
                // Ensure that all commands with the same PMA index are in the same buffer
                
                if (count[hash_idx] > 0 && (msg_buffer[hash_idx][0].range(31,0) / VERTEX_PER_PMA) != pma_index)
                {
                    HERE
                    // Flush the buffer if the PMA index does not match
                    flush_buffer(
                        hash_idx,
                        cmd_to_pma,
                        msg_buffer,
                        count
                    );
                }
                msg_buffer[hash_idx][count[hash_idx]] = cmd_data;
                count[hash_idx]++;
                // printf("count of buffer %d: %d\n", (int)hash_idx, (int)count[hash_idx]);
            }
            else  
            {
                // HERE
                //flush
                flush_buffer(
                    hash_idx,
                    cmd_to_pma,
                    msg_buffer,
                    count
                );
                msg_buffer[hash_idx][0] = cmd_data;
                count[hash_idx]++;
            }
        }
    }
}