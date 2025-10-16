#include "data_type.h"
#include "hls_stream.h"

void flush_buffer(
    int buffer_idx,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_pma,
    ap_uint<67> msg_buffer[16][16],
    ap_uint<4> &count
)
{
    for (int j = 0 ; j < count; j++)
    {
        #pragma HLS PIPELINE II=1
        #pragma HLS LOOP_TRIPCOUNT min=0 max=16
        if (j == count - 1)
        {
            PMA_UPDATE_MESSAGE tmp;
            tmp.data = msg_buffer[buffer_idx][j];
            tmp.last = 1;
            cmd_to_pma.write(tmp);
            // cmd_to_pma.write({.data=msg_buffer[i][j], .last = 1});
        }
        else
        {
            PMA_UPDATE_MESSAGE tmp;
            tmp.data = msg_buffer[buffer_idx][j];
            tmp.last = 0;
            cmd_to_pma.write(tmp);
            // cmd_to_pma.write({.data=msg_buffer[i][j], .last = 0});
        }
    }
    count = 0;
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

        static ap_uint<67> msg_buffer[16][16];
        #pragma HLS BIND_STORAGE variable=msg_buffer type=RAM_2P impl=BRAM
        static ap_uint<4> count[16]; 
        #pragma HLS BIND_STORAGE variable=count type=RAM_1P impl=BRAM
        for (int i = 0 ; i < 16 ; i++)
        {
            #pragma HLS UNROLL
            for (int j = 0 ; j < 16 ; j++)
            {
                #pragma HLS UNROLL
                msg_buffer[i][j] = 0;
            }
            count[i] = 0;
        }

        while (true)
        {
            
            CLEAR_ICON clear_icon;
            bool res = clear_message_from_generator.read_nb(clear_icon);
            if (res)
            {
                for (int i = 0 ; i < 16 ; i++)
                    flush_buffer(
                        i,
                        cmd_to_pma,
                        msg_buffer,
                        count[i]
                    );
            }
            PMA_UPDATE_MESSAGE cmd;
            auto is_cmd = cmd_from_generator.read_nb(cmd);
            if (!is_cmd)
            {
                continue;
            }
            ap_uint<4> hash_idx = (cmd.data.range(31,0) / VERTEX_PER_PMA) % 8;
            if (cmd.data[98] == OUT_DIRECTION)
            {
                hash_idx += 8;
            }
            if (count[hash_idx] < 16)
            {
                msg_buffer[hash_idx][count[hash_idx]] = cmd.data;
                count[hash_idx]++;
            }
            else  
            {
                //flush
                flush_buffer(
                    hash_idx,
                    cmd_to_pma,
                    msg_buffer,
                    count[hash_idx]
                );
            }
        }
    }
}