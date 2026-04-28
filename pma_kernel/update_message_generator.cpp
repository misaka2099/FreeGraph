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
        // int *src_to_change,
        // double *attr_to_change,
        // int src_attr_change_num,
        int last,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_router,
        hls::stream<WHOLE_ACK> &ack_from_router
        // hls::stream<VERTEX_ATTR_STREAM_TYPE> &attr_to_pma
        // hls::stream<CLEAR_ICON> &clear_message_to_router
    )
    {
        #pragma HLS INTERFACE s_axilite port=op_num
        #pragma HLS INTERFACE m_axi port=src bundle=gmem0
        #pragma HLS INTERFACE m_axi port=dst bundle=gmem1
        // #pragma HLS INTERFACE m_axi port=src_to_change bundle=gmem2
        // #pragma HLS INTERFACE m_axi port=attr_to_change bundle=gmem3
        // #pragma HLS INTERFACE s_axilite port=src_attr_change_num
        #pragma HLS INTERFACE s_axilite port=return
        #pragma HLS INTERFACE axis port=cmd_to_router depth=64
        #pragma HLS INTERFACE axis port=ack_from_router depth=64
       
        for (int i = 0 ; i < op_num ; i++)
        {
            #pragma HLS PIPELINE II=1
            PMA_UPDATE_MESSAGE msg{};
            msg.src = src[i];
            msg.dst = dst[i];
            // msg.opcode = opcode[i]; // opcode stored in high 32 bits
            switch (opcode[i]) 
            {
                case PMA_UPDATE_INSERT_EDGE:
                    msg.opcode = PMA_UPDATE_INSERT_EDGE;
                    break;
                case PMA_UPDATE_DELETE_EDGE:
                    msg.opcode = PMA_UPDATE_DELETE_EDGE;
                    break;
                case PMA_UPDATE_DELETE_VERTEX:
                    msg.opcode = PMA_UPDATE_DELETE_VERTEX;
                    break;
                default:
                    msg.opcode = PMA_UPDATE_INSERT_EDGE; // default to insert edge if unknown opcode
            }
            msg.last = last;
            cmd_to_router.write(msg);
        }
        HERE
        cmd_to_router.write({.src=-1, .dst=-1, .opcode = PMA_UPDATE_STOP, .last = last});
        // If this is the final batch, wait for ACK from router indicating completion
        WHOLE_ACK ack_pkt;
        // blocking read to ensure router processed the whole batch
        ack_from_router.read(ack_pkt);
        // optionally we could check ack_pkt.data
    }
}