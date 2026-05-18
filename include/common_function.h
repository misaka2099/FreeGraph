#pragma once
#include "data_type.h"
#include <hls_stream.h>
#include <ap_utils.h>

// =========================================================================
// dispatch: route by src>>4 % N.
//   flags==exit_flag: exit → broadcast to all workers & exit.
//   normal:           route by src>>4.
// Requires T to have fields: int src, CONTROL_TYPE flags
// =========================================================================
template <typename T, int N>
void dispatch(
    hls::stream<T>& cmd_stream,
    hls::stream<T> worker_cmd[N])
{
    #pragma HLS INLINE off
    while (true) {
        if (cmd_stream.empty()) { ap_wait(); continue; }
        T first = cmd_stream.read();

        if (first.flags == exit_flag) {
            for (int w = 0; w < N; w++) {
                #pragma HLS UNROLL
                worker_cmd[w].write(first);
            }
            break;
        }

        int w = (first.src >> 4) % N;
        worker_cmd[w].write(first);
    }
}

// =========================================================================
// merger: non-blocking poll from N workers → one output.
//   exit_flag: each worker signals done → N collected → 1 forwarded.
//   normal:    forwarded immediately.
// Requires T to have field: CONTROL_TYPE flags
// =========================================================================
template <typename T, int N>
void merger(
    hls::stream<T> worker_out[N],
    hls::stream<T>& to_next)
{
    ap_uint<N> finished = -1;

    while (finished != 0) {
        for (int i = 0; i < N; i++) {
            T msg;
            if (worker_out[i].read_nb(msg)) {
                if (msg.flags == exit_flag) {
                    finished[i] = 0;
                } else {
                    to_next.write(msg);
                }
            }
        }
    }

    T end_msg;
    end_msg.flags = exit_flag;
    to_next.write(end_msg);
}
