#include <hls_stream.h>
#include <ap_int.h>
#include "data_type.h"

typedef ap_axiu<64, 8, 0, 0> READ_PKT_STREAM;
static const int READ_WORKERS = 4;

extern "C" {
void pma_read_merger_kernel(
    hls::stream<READ_COMMAND_STREAM> &merger_cmd_stream,
    hls::stream<READ_PKT_STREAM> &worker0_pkt_stream,
    hls::stream<READ_PKT_STREAM> &worker1_pkt_stream,
    hls::stream<READ_PKT_STREAM> &worker2_pkt_stream,
    hls::stream<READ_PKT_STREAM> &worker3_pkt_stream,
    hls::stream<ap_axiu<64,0,0,0>> &data_to_coo_stream,
    hls::stream<ap_axiu<32,0,0,0>> &coo_edge_count_stream
) {
#pragma HLS INTERFACE axis port=merger_cmd_stream
#pragma HLS INTERFACE axis port=worker0_pkt_stream
#pragma HLS INTERFACE axis port=worker1_pkt_stream
#pragma HLS INTERFACE axis port=worker2_pkt_stream
#pragma HLS INTERFACE axis port=worker3_pkt_stream
#pragma HLS INTERFACE axis port=data_to_coo_stream
#pragma HLS INTERFACE axis port=coo_edge_count_stream

#pragma HLS STREAM variable=merger_cmd_stream depth=64
#pragma HLS STREAM variable=worker0_pkt_stream depth=128
#pragma HLS STREAM variable=worker1_pkt_stream depth=128
#pragma HLS STREAM variable=worker2_pkt_stream depth=128
#pragma HLS STREAM variable=worker3_pkt_stream depth=128
#pragma HLS STREAM variable=data_to_coo_stream depth=16
#pragma HLS STREAM variable=coo_edge_count_stream depth=16

    while (true) {
        READ_COMMAND_STREAM cmd = merger_cmd_stream.read();
        if (cmd.data == PMA_STOP && cmd.last == 1) {
            break;
        }

        int mode = (int)cmd.data.range(31, 0);
        bool done0 = false;
        bool done1 = false;
        bool done2 = false;
        bool done3 = false;
        int rr = 0;
        int total_edge_count = 0;
        bool count_end = false;

        while (!(done0 && done1 && done2 && done3)) {
#pragma HLS PIPELINE II=1
            READ_PKT_STREAM pkt;
            if (rr == 0) {
                if (done0) { rr = 1; continue; }
                pkt = worker0_pkt_stream.read();
            } else if (rr == 1) {
                if (done1) { rr = 2; continue; }
                pkt = worker1_pkt_stream.read();
            } else if (rr == 2) {
                if (done2) { rr = 3; continue; }
                pkt = worker2_pkt_stream.read();
            } else {
                if (done3) { rr = 0; continue; }
                pkt = worker3_pkt_stream.read();
            }

            if (pkt.keep == 0 && pkt.last == 1) {
                if (rr == 0) done0 = true;
                else if (rr == 1) done1 = true;
                else if (rr == 2) done2 = true;
                else done3 = true;
                rr = (rr + 1) % READ_WORKERS;
                continue;
            }

            else if (pkt.keep != 0 && pkt.last == 1) {
                rr = (rr + 1) % READ_WORKERS;
                continue;
            }

            else if (pkt.keep != 0) {
                if (mode == PMA_GET_ALL_OUT_EDGE_COO) {
                    ap_axiu<64,0,0,0> o = {};
                    o.data = pkt.data;
                    o.keep = 0xFF;
                    o.strb = 0xFF;
                    o.last = 0;
                    data_to_coo_stream.write(o);
                } else if (mode == PMA_GET_EDGE_COUNT_RANGE) {
                    total_edge_count += (int)pkt.data.range(31, 0);
                    if (pkt.user == 0) count_end = true;
                }
            }
        }

        if (mode == PMA_GET_ALL_OUT_EDGE_COO) {
            ap_axiu<64,0,0,0> o = {};
            o.data = (ap_uint<64>)-1;
            o.keep = 0xFF;
            o.strb = 0xFF;
            o.last = 1;
            data_to_coo_stream.write(o);
        } else if (mode == PMA_GET_EDGE_COUNT_RANGE) {
            ap_axiu<32,0,0,0> o = {};
            o.data = (ap_uint<32>)total_edge_count;
            o.keep = 0xF;
            o.strb = 0xF;
            o.last = count_end ? 1 : 0;
            coo_edge_count_stream.write(o);
        }
    }
}
}
