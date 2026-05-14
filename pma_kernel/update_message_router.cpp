#include "data_type.h"
#include "common_function.h"
#include <cstdio>
#include <cstring>
#include <hls_stream.h>
#include <ap_utils.h>
#include "hls_print.h"

// =========================================================================
// collect: merge generator + reprocess. Generator priority.
//   flush_flag:  drain pipeline + send ACK, workers stay alive.
//   exit_flag:   drain pipeline + send exit_flag to workers + send ACK.
//   Drain sequence:
//     1) Stop forwarding generator messages.
//     2) Send probe (clear_probe) through pipeline.
//     3) When all workers return clear_probe, pipeline is empty.
//     4) flush_flag → send synchronize_flag (ACK), reset state.
//        exit_flag  → send exit_flag to workers, wait for it to return,
//                    then send synchronize_flag (ACK), reset state.
// =========================================================================
static void collect(
    hls::stream<PMA_UPDATE_MESSAGE>& from_generator,
    hls::stream<PMA_UPDATE_MESSAGE>& reprocess_in,
    hls::stream<PMA_UPDATE_MESSAGE>& to_pool)
{
    #pragma HLS INLINE off
    printf("ROUTER COLLECT START\n");
    bool draining = false;
    bool drain_exit = false;   // true=drain from exit_flag, false=drain from flush_flag
    bool probe_inflight = false;
    bool stop_sent = false;    // exit_flag sent to workers (only when drain_exit)
    bool probe_failed = false;

    while (true) {
        // Reprocess priority in normal mode: drain reprocess first to
        // prevent backpressure deadlock.  Only read generator when
        // reprocess is empty.
        if (!draining && !reprocess_in.empty()) {
            PMA_UPDATE_MESSAGE msg = reprocess_in.read();
            to_pool.write(msg);
            continue;
        }

        // Generator: read only when reprocess is empty.
        if (!draining && !from_generator.empty()) {
            PMA_UPDATE_MESSAGE msg = from_generator.read();
            if (msg.flags == exit_flag) {
                draining = true; drain_exit = true;
            } else if (msg.flags == flush_flag) {
                draining = true; drain_exit = false;
            } else {
                to_pool.write(msg);
            }
            continue;
        }

        // Drain phase: probe and wait for reprocess response.
        if (draining) {
            if (!stop_sent) {
                if (!probe_inflight) {
                    PMA_UPDATE_MESSAGE probe = {};
                    probe.flags = clear_probe;
                    to_pool.write(probe);
                    probe_inflight = true;
                    probe_failed = false;
                }

                if (!reprocess_in.empty()) {
                    PMA_UPDATE_MESSAGE msg = reprocess_in.read();

                    if (msg.flags == clear_probe) {
                        if (!probe_failed) {
                            if (drain_exit) {
                                PMA_UPDATE_MESSAGE stop = {};
                                stop.flags = exit_flag;
                                to_pool.write(stop);
                                stop_sent = true;
                            } else {
                                PMA_UPDATE_MESSAGE done = {};
                                done.flags = synchronize_flag;
                                to_pool.write(done);
                                draining = false; probe_inflight = false;
                                probe_failed = false;
                            }
                        } else {
                            probe_inflight = false; // resend probe
                        }
                    } else if (msg.flags == exit_flag) {
                        PMA_UPDATE_MESSAGE done = {};
                        done.flags = synchronize_flag;
                        to_pool.write(done);
                        draining = false; drain_exit = false; stop_sent = false;
                        probe_inflight = false; probe_failed = false;
                        continue;
                    } else {
                        to_pool.write(msg);
                        probe_failed = true;
                    }
                } else {
                    ap_wait();
                }
                continue;
            }

            // stop_sent (drain_exit only): wait for exit_flag from reprocess.
            if (!reprocess_in.empty()) {
                PMA_UPDATE_MESSAGE msg = reprocess_in.read();
                if (msg.flags == exit_flag) {
                    PMA_UPDATE_MESSAGE done = {};
                    done.flags = synchronize_flag;
                    to_pool.write(done);
                    draining = false; drain_exit = false; stop_sent = false;
                    probe_inflight = false; probe_failed = false;
                    continue;
                }
            }
            ap_wait();
            continue;
        }

        ap_wait();
    }
}

// =========================================================================
// dispatch_with_ack: route by src>>4 % N.
//   exit_flag:       broadcast to workers, stay alive for drain.
//   clear_probe:     broadcast probe, continue.
//   synchronize_flag: drain confirmed → send ACK, stay alive for next batch.
// =========================================================================
static void dispatch_with_ack(
    hls::stream<PMA_UPDATE_MESSAGE>& pool_stream,
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_to_bs,
    hls::stream<WHOLE_ACK>&          ack_to_generator)
{
    #pragma HLS INLINE off
    while (true) {
        if (pool_stream.empty()) { ap_wait(); continue; }
        PMA_UPDATE_MESSAGE msg = pool_stream.read();

        if (msg.flags == exit_flag) {
            cmd_to_bs.write(msg);
            continue;  // stay alive for drain phase
        }

        if (msg.flags == clear_probe) {
            cmd_to_bs.write(msg);
            continue;
        }

        if (msg.flags == synchronize_flag) {
            WHOLE_ACK ack = {};
            ack.data = -1;
            ack.keep = 0xFF;
            ack.strb = 0xFF;
            ack.user = 0;
            ack.last = 0;
            ack_to_generator.write(ack);
            continue;
        }

        cmd_to_bs.write(msg);
    }
}

// =========================================================================
// top-level
// =========================================================================
extern "C"
{
void update_message_router(
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_from_generator,
    hls::stream<PMA_UPDATE_MESSAGE>& reprocess_in,
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_to_bs,
    hls::stream<WHOLE_ACK>&          ack_to_generator)
{
    #pragma HLS INTERFACE axis port=cmd_from_generator
    #pragma HLS INTERFACE axis port=reprocess_in
    #pragma HLS INTERFACE axis port=cmd_to_bs
    #pragma HLS INTERFACE axis port=ack_to_generator
    #pragma HLS INTERFACE s_axilite port=return bundle=ctrl

    #pragma HLS DATAFLOW

    hls::stream<PMA_UPDATE_MESSAGE> pool_stream;
    #pragma HLS STREAM variable=pool_stream depth=128

    collect(cmd_from_generator, reprocess_in, pool_stream);
    dispatch_with_ack(pool_stream, cmd_to_bs, ack_to_generator);
}
}
