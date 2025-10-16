#include "data_type.h"
#include "hls_stream.h"

void flush_buffer(
    int buffer_idx,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_pma,
    ap_uint<128> msg_buffer[MAX_COMMAND_BUFFER_NUM][MAX_COMMAND_BUFFER_LENGTH],
    ap_uint<4> *count_arr
)
{
    ap_uint<4> c = count_arr[buffer_idx];
    for (int j = 0 ; j < c; j++) {
        #pragma HLS PIPELINE II=1
        PMA_UPDATE_MESSAGE tmp;
        tmp.data = msg_buffer[buffer_idx][j];
        tmp.last = (j == c - 1) ? 1 : 0;
        cmd_to_pma.write(tmp);
    }
    count_arr[buffer_idx] = 0;
}
extern "C"
{
    void update_message_router(
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_from_generator,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_pma,
        hls::stream<CLEAR_ICON> &clear_message_from_generator
    )
    {
        #pragma HLS INTERFACE axis port=cmd_from_generator
        #pragma HLS INTERFACE axis port=cmd_to_pma
        #pragma HLS INTERFACE axis port=clear_message_from_generator
        #pragma HLS INTERFACE ap_ctrl_none port=return
        ap_uint<128> msg_buffer[MAX_COMMAND_BUFFER_NUM][MAX_COMMAND_BUFFER_LENGTH];
        #pragma HLS BIND_STORAGE variable=msg_buffer type=RAM_2P impl=BRAM
        ap_uint<4> count[MAX_COMMAND_BUFFER_NUM]; 
        // for (int i = 0 ; i < MAX_COMMAND_BUFFER_NUM ; i++)
        // {
        //     #pragma HLS UNROLL
        //     for (int j = 0 ; j < MAX_COMMAND_BUFFER_LENGTH ; j++)
        //     {
        //         #pragma HLS UNROLL
        //         msg_buffer[i][j] = 0;
        //     }
        //     count[i] = 0;
        // }

        while (true)
        {
            
            CLEAR_ICON clear_icon;
            bool res = clear_message_from_generator.read_nb(clear_icon);
            if (res)
            {
                for (int i = 0 ; i < MAX_COMMAND_BUFFER_NUM ; i++)
                    flush_buffer(
                        i,
                        cmd_to_pma,
                        msg_buffer,
                        count
                    );
            }
            PMA_UPDATE_MESSAGE cmd;
            auto is_cmd = cmd_from_generator.read_nb(cmd);
            if (!is_cmd)
            {
                continue;
            }
            ap_uint<4> hash_idx = (cmd.data.range(31,0) / VERTEX_PER_PMA) % (MAX_COMMAND_BUFFER_NUM / 2);
            auto cmd_data = cmd.data;
            if (cmd_data[98] == OUT_DIRECTION)
            {
                hash_idx += (MAX_COMMAND_BUFFER_NUM / 2);
            }
            if (count[hash_idx] < MAX_COMMAND_BUFFER_LENGTH)
            {
                msg_buffer[hash_idx][count[hash_idx]] = cmd_data;
                count[hash_idx]++;
            }
            else  
            {
                //flush
                flush_buffer(
                    hash_idx,
                    cmd_to_pma,
                    msg_buffer,
                    count
                );
            }
        }
    }
}