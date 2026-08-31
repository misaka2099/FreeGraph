#include "data_type.h"
#include <hls_stream.h>
#include "etc/autopilot_ssdm_op.h"

// Free-running transport only. Every command, including FLUSH/STOP, is
// routed by src>>4; generator emits control records with src 0,16,32.
static inline ap_uint<2> channel_from_pma(pma_idx_u_t pma)
{
    #pragma HLS INLINE
    // 4 == 1 (mod 3), so summing the 2-bit base-4 digits gives pma % 3.
    // Keep this combinational: an HLS % operator creates a long divider
    // pipeline which is unnecessary for this one-cycle routing decision.
    ap_uint<3> p0 = (ap_uint<3>)pma.range(1, 0) + (ap_uint<3>)pma.range(3, 2);
    ap_uint<3> p1 = (ap_uint<3>)pma.range(5, 4) + (ap_uint<3>)pma.range(7, 6);
    ap_uint<3> p2 = (ap_uint<3>)pma.range(9, 8) + (ap_uint<3>)pma.range(11, 10);
    ap_uint<3> p3 = (ap_uint<3>)pma.range(13, 12) + (ap_uint<3>)pma.range(15, 14);
    ap_uint<3> p4 = (ap_uint<3>)pma.range(17, 16) + (ap_uint<3>)pma.range(19, 18);
    ap_uint<4> q0 = (ap_uint<4>)p0 + (ap_uint<4>)p1;
    ap_uint<4> q1 = (ap_uint<4>)p2 + (ap_uint<4>)p3;
    ap_uint<5> sum = (ap_uint<5>)q0 + (ap_uint<5>)q1 + (ap_uint<5>)p4;
    if (sum == 0 || sum == 3 || sum == 6 || sum == 9 || sum == 12 ||
        sum == 15 || sum == 18 || sum == 21 || sum == 24 ||
        sum == 27 || sum == 30)
        return 0;
    if (sum == 1 || sum == 4 || sum == 7 || sum == 10 || sum == 13 ||
        sum == 16 || sum == 19 || sum == 22 || sum == 25 || sum == 28)
        return 1;
    return 2;
}

static void route_commands(hls::stream<PMA_UPDATE_MESSAGE>& in,
                           hls::stream<PMA_UPDATE_MESSAGE>& out0,
                           hls::stream<PMA_UPDATE_MESSAGE>& out1,
                           hls::stream<PMA_UPDATE_MESSAGE>& out2)
{
    while (true) {
        #pragma HLS PIPELINE off
        PMA_UPDATE_MESSAGE msg = in.read();
        ap_uint<2> ch = channel_from_pma((pma_idx_u_t)(((ap_uint<32>)msg.src) >> 4));
        if (ch == 0) out0.write(msg);
        else if (ch == 1) out1.write(msg);
        else out2.write(msg);
    }
}

static void merge_acks(hls::stream<WHOLE_ACK>& in0,
                       hls::stream<WHOLE_ACK>& in1,
                       hls::stream<WHOLE_ACK>& in2,
                       hls::stream<WHOLE_ACK>& out)
{
    unsigned rr = 0;
    while (true) {
        #pragma HLS PIPELINE off
        WHOLE_ACK ack;
        bool got = false;
        if (rr == 0) got = in0.read_nb(ack);
        else if (rr == 1) got = in1.read_nb(ack);
        else got = in2.read_nb(ack);
        rr = (rr == 2) ? 0 : rr + 1;
        if (got) out.write(ack);
        else ap_wait();
    }
}

extern "C" void update_message_router_stream(
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_from_generator,
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_to_worker_0,
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_to_worker_1,
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_to_worker_2,
    hls::stream<WHOLE_ACK>& ack_from_worker_0,
    hls::stream<WHOLE_ACK>& ack_from_worker_1,
    hls::stream<WHOLE_ACK>& ack_from_worker_2,
    hls::stream<WHOLE_ACK>& ack_to_generator)
{
    #pragma HLS INTERFACE axis port=cmd_from_generator
    #pragma HLS INTERFACE axis port=cmd_to_worker_0
    #pragma HLS INTERFACE axis port=cmd_to_worker_1
    #pragma HLS INTERFACE axis port=cmd_to_worker_2
    #pragma HLS INTERFACE axis port=ack_from_worker_0
    #pragma HLS INTERFACE axis port=ack_from_worker_1
    #pragma HLS INTERFACE axis port=ack_from_worker_2
    #pragma HLS INTERFACE axis port=ack_to_generator
    #pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS DATAFLOW
    route_commands(cmd_from_generator, cmd_to_worker_0, cmd_to_worker_1, cmd_to_worker_2);
    merge_acks(ack_from_worker_0, ack_from_worker_1, ack_from_worker_2, ack_to_generator);
}
