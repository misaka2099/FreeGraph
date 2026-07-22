#include <hls_stream.h>
#include <ap_int.h>
#include "data_type.h"
#include "etc/autopilot_ssdm_op.h"

static const int READ_WORKERS = CHANNEL_NUM;

static void send_stop(
    hls::stream<READ_COMMAND_STREAM> &worker_cmd_0,
    hls::stream<READ_COMMAND_STREAM> &worker_cmd_1,
    hls::stream<READ_COMMAND_STREAM> &worker_cmd_2,
    hls::stream<READ_COMMAND_STREAM> &merger_cmd
) {
    #pragma HLS INLINE
    READ_COMMAND_STREAM stop_cmd = {};
    stop_cmd.data = PMA_STOP;
    stop_cmd.last = 1;
    worker_cmd_0.write(stop_cmd);
    worker_cmd_1.write(stop_cmd);
    worker_cmd_2.write(stop_cmd);
    merger_cmd.write(stop_cmd);
}

extern "C" {
void pma_read_router_kernel(
    hls::stream<READ_COMMAND_STREAM> &cmd_in,
    hls::stream<READ_COMMAND_STREAM> &worker_cmd_0,
    hls::stream<READ_COMMAND_STREAM> &worker_cmd_1,
    hls::stream<READ_COMMAND_STREAM> &worker_cmd_2,
    hls::stream<READ_COMMAND_STREAM> &merger_cmd
) {
#pragma HLS INTERFACE axis port=cmd_in
#pragma HLS INTERFACE axis port=worker_cmd_0
#pragma HLS INTERFACE axis port=worker_cmd_1
#pragma HLS INTERFACE axis port=worker_cmd_2
#pragma HLS INTERFACE axis port=merger_cmd

#pragma HLS STREAM variable=cmd_in depth=16
#pragma HLS STREAM variable=worker_cmd_0 depth=16
#pragma HLS STREAM variable=worker_cmd_1 depth=16
#pragma HLS STREAM variable=worker_cmd_2 depth=16
#pragma HLS STREAM variable=merger_cmd depth=64

    while (true) {
        READ_COMMAND_STREAM cmd = cmd_in.read();

        if (cmd.data == PMA_STOP && cmd.last == 1) {
            send_stop(worker_cmd_0, worker_cmd_1, worker_cmd_2, merger_cmd);
            break;
        }
        #pragma HLS protocol fixed
        ap_wait();
        worker_cmd_0.write(cmd);
        worker_cmd_1.write(cmd);
        worker_cmd_2.write(cmd);
        merger_cmd.write(cmd);
    }
}
}
