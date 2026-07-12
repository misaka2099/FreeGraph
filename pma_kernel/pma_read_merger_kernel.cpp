#include <hls_stream.h>
#include <ap_int.h>
#include "data_type.h"

typedef ap_axiu<64, 8, 0, 0> READ_PKT_STREAM;
static const int READ_WORKERS = CHANNEL_NUM;

extern "C" {
void pma_read_merger_kernel(
    hls::stream<READ_COMMAND_STREAM> &merger_cmd_stream,
    hls::stream<READ_PKT_STREAM> &worker0_pkt_stream,
    hls::stream<READ_PKT_STREAM> &worker1_pkt_stream,
    hls::stream<READ_PKT_STREAM> &worker2_pkt_stream,
    hls::stream<READ_PKT_STREAM> &worker3_pkt_stream,
    hls::stream<ap_axiu<64,0,0,0>> &data_to_coo_stream,
    int* edge_count_ptr
) {
#pragma HLS INTERFACE axis port=merger_cmd_stream
#pragma HLS INTERFACE axis port=worker0_pkt_stream
#pragma HLS INTERFACE axis port=worker1_pkt_stream
#pragma HLS INTERFACE axis port=worker2_pkt_stream
#pragma HLS INTERFACE axis port=worker3_pkt_stream
#pragma HLS INTERFACE axis port=data_to_coo_stream
#pragma HLS INTERFACE m_axi port=edge_count_ptr offset=slave bundle=gmem0
#pragma HLS INTERFACE s_axilite port=edge_count_ptr bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

#pragma HLS STREAM variable=merger_cmd_stream depth=64
#pragma HLS STREAM variable=worker0_pkt_stream depth=128
#pragma HLS STREAM variable=worker1_pkt_stream depth=128
#pragma HLS STREAM variable=worker2_pkt_stream depth=128
#pragma HLS STREAM variable=worker3_pkt_stream depth=128
#pragma HLS STREAM variable=data_to_coo_stream depth=16

    while (true) {
        READ_COMMAND_STREAM cmd = merger_cmd_stream.read();
        if (cmd.data == PMA_STOP && cmd.last == 1) {
            break;
        }

        int mode = (int)cmd.data.range(31, 0);
        bool done[READ_WORKERS];
#pragma HLS ARRAY_PARTITION variable=done complete
        for (int i = 0; i < READ_WORKERS; i++) {
#pragma HLS UNROLL
            done[i] = false;
        }
        int done_count = 0;
        int rr = 0;
        int total_edge_count = 0;

        while (done_count < READ_WORKERS) {
#pragma HLS PIPELINE II=1
            if (done[rr]) {
                rr = (rr + 1) % READ_WORKERS;
                continue;
            }

            READ_PKT_STREAM pkt;
            if      (rr == 0) pkt = worker0_pkt_stream.read();
            else if (rr == 1) pkt = worker1_pkt_stream.read();
            else if (rr == 2) pkt = worker2_pkt_stream.read();
            else              pkt = worker3_pkt_stream.read();

            if (pkt.keep == 0 && pkt.last == 1) {
                done[rr] = true;
                done_count++;
                rr = (rr + 1) % READ_WORKERS;
                continue;
            }

            if (pkt.keep != 0 && pkt.last == 1) {
                rr = (rr + 1) % READ_WORKERS;
                continue;
            }

            if (pkt.keep != 0) {
                if (mode == PMA_GET_ALL_OUT_EDGE_COO || mode == PMA_GET_OUT_EDGE) {
                    ap_axiu<64,0,0,0> o = {};
                    o.data = pkt.data;
                    o.keep = 0xFF;
                    o.strb = 0xFF;
                    o.last = 0;
                    data_to_coo_stream.write(o);
                } else if (mode == PMA_GET_EDGE_COUNT_RANGE) {
                    total_edge_count += (int)pkt.data.range(31, 0);
                }
            }
        }

        if (mode == PMA_GET_ALL_OUT_EDGE_COO || mode == PMA_GET_OUT_EDGE) {
            ap_axiu<64,0,0,0> o = {};
            o.data = (ap_uint<64>)-1;
            o.keep = 0xFF;
            o.strb = 0xFF;
            o.last = 1;
            data_to_coo_stream.write(o);
        } else if (mode == PMA_GET_EDGE_COUNT_RANGE) {
            *edge_count_ptr = total_edge_count;
            ap_axiu<64,0,0,0> o = {};
            o.data = (ap_uint<64>)total_edge_count;
            o.keep = 0xFF;
            o.strb = 0xFF;
            o.last = 1;
            data_to_coo_stream.write(o);
        }
    }
}
}
