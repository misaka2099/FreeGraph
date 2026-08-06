#include "data_type.h"
#include <hls_stream.h>

// Simple stream router: reads from generator, forwards to per-worker streams.
// No ACKs, no HBM — pure stream demux by PMA -> channel mapping.

extern "C"
{
    void update_message_router_stream(
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_from_generator,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_worker_0,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_worker_1,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_worker_2,
        hls::stream<WHOLE_ACK> &ack_to_generator
    )
    {
        #pragma HLS INTERFACE axis port=cmd_from_generator
        #pragma HLS INTERFACE axis port=cmd_to_worker_0
        #pragma HLS INTERFACE axis port=cmd_to_worker_1
        #pragma HLS INTERFACE axis port=cmd_to_worker_2
        #pragma HLS INTERFACE axis port=ack_to_generator
        #pragma HLS INTERFACE s_axilite port=return bundle=control

        #pragma HLS STREAM variable=cmd_from_generator depth=64
        #pragma HLS STREAM variable=cmd_to_worker_0 depth=16
        #pragma HLS STREAM variable=cmd_to_worker_1 depth=16
        #pragma HLS STREAM variable=cmd_to_worker_2 depth=16
        #pragma HLS STREAM variable=ack_to_generator depth=4

        while (true) {
            #pragma HLS PIPELINE II=1
            PMA_UPDATE_MESSAGE msg = cmd_from_generator.read();

            if (msg.opcode == PMA_UPDATE_STOP && msg.last == 1) {
                cmd_to_worker_0.write(msg);
                cmd_to_worker_1.write(msg);
                cmd_to_worker_2.write(msg);
                WHOLE_ACK ack; ack.data = 0; ack.last = 1;
                ack_to_generator.write(ack);
                return;
            }

            if (msg.opcode == PMA_UPDATE_FLUSH_CACHE) {
                // 批间 barrier: 广播给所有 worker (各 worker 关闭当前波), ACK 回 generator,
                // 不 return (继续服务后续批次)。
                cmd_to_worker_0.write(msg);
                cmd_to_worker_1.write(msg);
                cmd_to_worker_2.write(msg);
                WHOLE_ACK ack; ack.data = 0; ack.last = 1;
                ack_to_generator.write(ack);
                continue;
            }

            int channel = (msg.src / VERTEX_PER_PMA) % CHANNEL_NUM;
            if (channel == 0)      cmd_to_worker_0.write(msg);
            else if (channel == 1) cmd_to_worker_1.write(msg);
            else                   cmd_to_worker_2.write(msg);
        }
    }
}
