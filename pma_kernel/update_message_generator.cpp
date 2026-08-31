#include "data_type.h"
#include <cstdio>
#include <hls_stream.h>
#include "debug.h"
extern "C"
{
    void update_message_generator(
        int op_num,
        int *src,
        int *dst,
        int *opcode,
        int last,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_router,
        hls::stream<WHOLE_ACK> &ack_from_router)
    {
        #pragma HLS INTERFACE s_axilite port=op_num
        #pragma HLS INTERFACE m_axi port=src bundle=gmem0
        #pragma HLS INTERFACE m_axi port=dst bundle=gmem1
        #pragma HLS INTERFACE s_axilite port=return
        #pragma HLS INTERFACE axis port=cmd_to_router depth=64
        #pragma HLS INTERFACE axis port=ack_from_router depth=64

        for (int i = 0; i < op_num; i++) {
            #pragma HLS PIPELINE II=1
            PMA_UPDATE_MESSAGE msg{};
            msg.src = src[i];
            msg.dst = dst[i];
            switch (opcode[i]) {
            case PMA_UPDATE_INSERT_EDGE: msg.opcode = PMA_UPDATE_INSERT_EDGE; break;
            case PMA_UPDATE_DELETE_EDGE: msg.opcode = PMA_UPDATE_DELETE_EDGE; break;
            default: msg.opcode = PMA_UPDATE_INSERT_EDGE; break;
            }
            msg.last = 0;
            cmd_to_router.write(msg);
        }

        // Control records use synthetic PMA bases 0,16,32. The router uses
        // exactly the same src>>4 demux for data and control records.
        ap_uint<3> ack_mask = 0;
        PMA_UPDATE_OPCODE barrier = last ? PMA_UPDATE_STOP : PMA_UPDATE_FLUSH_CACHE;
        for (int worker = 0; worker < CHANNEL_NUM; worker++) {
            PMA_UPDATE_MESSAGE ctrl{};
            ctrl.src = worker * VERTEX_PER_PMA;
            ctrl.dst = -1;
            ctrl.opcode = barrier;
            ctrl.last = last;
            cmd_to_router.write(ctrl);
        }

        while (ack_mask != ((1 << CHANNEL_NUM) - 1)) {
            WHOLE_ACK ack_pkt = ack_from_router.read();
            unsigned worker = (unsigned)ack_pkt.data;
            if (worker < CHANNEL_NUM) ack_mask[worker] = 1;
        }
    }
}