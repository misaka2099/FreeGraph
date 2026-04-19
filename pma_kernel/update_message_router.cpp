#include "data_type.h"
#include "debug.h"
#include "etc/autopilot_ssdm_op.h"
#include "hls_print.h"
#include "hls_stream.h"
#include <cstdio>

#ifndef MAX_INFLIGHT_NUM
#define MAX_INFLIGHT_NUM 32
#endif

#define ROUTER_WORKERS CHANNEL_NUM

void flush_buffer_aux(
    int buffer_idx,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_pma,
    ap_uint<128> msg_buffer[MAX_COMMAND_BUFFER_NUM][MAX_COMMAND_BUFFER_LENGTH],
    int *count_arr)
{
    #pragma HLS INLINE
    int c = count_arr[buffer_idx];
    for (int j = 0; j < c; j++) {
        #pragma HLS PIPELINE II=1
        PMA_UPDATE_MESSAGE tmp;
        tmp.data = msg_buffer[buffer_idx][j];
        tmp.last = (j == c - 1) ? 1 : 0;
        cmd_to_pma.write(tmp);
    }
    hls::print("Flushed buffer with %d messages\n", c);
    count_arr[buffer_idx] = 0;
}

bool check_inflight(int *in_flight_arr, int pma_idx)
{
    #pragma HLS INLINE
    for (int i = 0; i < MAX_INFLIGHT_NUM; i++) {
        if (in_flight_arr[i] == pma_idx) {
            return true;
        }
    }
    return false;
}

void get_hash_value(int pma_idx, int &hash_value)
{
    #pragma HLS INLINE
    ap_uint<32> x = (ap_uint<32>)pma_idx;
    x ^= (x >> 16);
    x *= (ap_uint<32>)2654435761u;
    hash_value = (int)(x % MAX_COMMAND_BUFFER_NUM);
}

static void update_inflight_from_ack(
    int *in_flight_arr,
    int &in_flight_count,
    hls::stream<COMMAND_STREAM> &ack_stream)
{
    #pragma HLS INLINE
    COMMAND_STREAM ack;
    if (ack_stream.read_nb(ack)) {
        int pma_idx = ack.data;
        for (int i = 0; i < MAX_INFLIGHT_NUM; i++) {
            if (in_flight_arr[i] == pma_idx) {
                in_flight_arr[i] = -1;
                in_flight_count--;
                hls::print("infight removed, count %d\n", in_flight_count);
                break;
            }
        }
    }
}

static void check_ack_back_array(
    int *in_flight_arr,
    int &in_flight_count,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_1,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_2,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_3,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_4,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_5,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_6,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_7,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_8)
{
    #pragma HLS INLINE
    update_inflight_from_ack(in_flight_arr, in_flight_count, ack_from_insert_edge_1);
    update_inflight_from_ack(in_flight_arr, in_flight_count, ack_from_insert_edge_2);
    update_inflight_from_ack(in_flight_arr, in_flight_count, ack_from_insert_edge_3);
    update_inflight_from_ack(in_flight_arr, in_flight_count, ack_from_insert_edge_4);
    update_inflight_from_ack(in_flight_arr, in_flight_count, ack_from_insert_edge_5);
    update_inflight_from_ack(in_flight_arr, in_flight_count, ack_from_insert_edge_6);
    update_inflight_from_ack(in_flight_arr, in_flight_count, ack_from_insert_edge_7);
    update_inflight_from_ack(in_flight_arr, in_flight_count, ack_from_insert_edge_8);
}

void add_inflight(int *in_flight_arr, int &in_flight_count, int pma_idx)
{
    for (int i = 0; i < MAX_INFLIGHT_NUM; i++) {
        if (in_flight_arr[i] == -1) {
            in_flight_arr[i] = pma_idx;
            in_flight_count++;
            hls::print("infight added, count %d\n", in_flight_count);
            break;
        }
    }
}

static void flush_edge_buffer_to_worker(
    int worker,
    int hash,
    ap_uint<128> msg_buffer[MAX_COMMAND_BUFFER_NUM][MAX_COMMAND_BUFFER_LENGTH],
    int *count_arr,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_1,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_2,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_3,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_4,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_5,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_6,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_7,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_8)
{
    #pragma HLS INLINE
    switch (worker) {
        case 0: flush_buffer_aux(hash, cmd_to_insert_edge_1, msg_buffer, count_arr); break;
        case 1: flush_buffer_aux(hash, cmd_to_insert_edge_2, msg_buffer, count_arr); break;
        case 2: flush_buffer_aux(hash, cmd_to_insert_edge_3, msg_buffer, count_arr); break;
        case 3: flush_buffer_aux(hash, cmd_to_insert_edge_4, msg_buffer, count_arr); break;
        case 4: flush_buffer_aux(hash, cmd_to_insert_edge_5, msg_buffer, count_arr); break;
        case 5: flush_buffer_aux(hash, cmd_to_insert_edge_6, msg_buffer, count_arr); break;
        case 6: flush_buffer_aux(hash, cmd_to_insert_edge_7, msg_buffer, count_arr); break;
        case 7: flush_buffer_aux(hash, cmd_to_insert_edge_8, msg_buffer, count_arr); break;
        default: break;
    }
}

static void route_vertex_to_worker(
    int worker,
    PMA_UPDATE_MESSAGE &msg,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_1,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_2,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_3,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_4,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_5,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_6,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_7,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_8)
{
    #pragma HLS INLINE
    switch (worker) {
        case 0: cmd_to_insert_vertex_1.write(msg); break;
        case 1: cmd_to_insert_vertex_2.write(msg); break;
        case 2: cmd_to_insert_vertex_3.write(msg); break;
        case 3: cmd_to_insert_vertex_4.write(msg); break;
        case 4: cmd_to_insert_vertex_5.write(msg); break;
        case 5: cmd_to_insert_vertex_6.write(msg); break;
        case 6: cmd_to_insert_vertex_7.write(msg); break;
        case 7: cmd_to_insert_vertex_8.write(msg); break;
        default: break;
    }
}

static void broadcast_vertex_stop(
    PMA_UPDATE_MESSAGE &msg,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_1,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_2,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_3,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_4,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_5,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_6,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_7,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_8)
{
    #pragma HLS INLINE
    cmd_to_insert_vertex_1.write(msg);
    cmd_to_insert_vertex_2.write(msg);
    cmd_to_insert_vertex_3.write(msg);
    cmd_to_insert_vertex_4.write(msg);
    cmd_to_insert_vertex_5.write(msg);
    cmd_to_insert_vertex_6.write(msg);
    cmd_to_insert_vertex_7.write(msg);
    cmd_to_insert_vertex_8.write(msg);
}

static void broadcast_edge_stop(
    PMA_UPDATE_MESSAGE &msg,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_1,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_2,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_3,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_4,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_5,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_6,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_7,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_8)
{
    #pragma HLS INLINE
    cmd_to_insert_edge_1.write(msg);
    cmd_to_insert_edge_2.write(msg);
    cmd_to_insert_edge_3.write(msg);
    cmd_to_insert_edge_4.write(msg);
    cmd_to_insert_edge_5.write(msg);
    cmd_to_insert_edge_6.write(msg);
    cmd_to_insert_edge_7.write(msg);
    cmd_to_insert_edge_8.write(msg);
}

static void poll_vertex_acks(
    bool acked[ROUTER_WORKERS],
    VERTEX_ACK &tmp,
    hls::stream<VERTEX_ACK> &ack_from_insert_vertex_1,
    hls::stream<VERTEX_ACK> &ack_from_insert_vertex_2,
    hls::stream<VERTEX_ACK> &ack_from_insert_vertex_3,
    hls::stream<VERTEX_ACK> &ack_from_insert_vertex_4,
    hls::stream<VERTEX_ACK> &ack_from_insert_vertex_5,
    hls::stream<VERTEX_ACK> &ack_from_insert_vertex_6,
    hls::stream<VERTEX_ACK> &ack_from_insert_vertex_7,
    hls::stream<VERTEX_ACK> &ack_from_insert_vertex_8)
{
    #pragma HLS INLINE
    if (!acked[0] && ack_from_insert_vertex_1.read_nb(tmp)) acked[0] = true;
    if (!acked[1] && ack_from_insert_vertex_2.read_nb(tmp)) acked[1] = true;
    if (!acked[2] && ack_from_insert_vertex_3.read_nb(tmp)) acked[2] = true;
    if (!acked[3] && ack_from_insert_vertex_4.read_nb(tmp)) acked[3] = true;
    if (!acked[4] && ack_from_insert_vertex_5.read_nb(tmp)) acked[4] = true;
    if (!acked[5] && ack_from_insert_vertex_6.read_nb(tmp)) acked[5] = true;
    if (!acked[6] && ack_from_insert_vertex_7.read_nb(tmp)) acked[6] = true;
    if (!acked[7] && ack_from_insert_vertex_8.read_nb(tmp)) acked[7] = true;
}

void flush_and_add_inflight(
    int hash,
    int pma_idx,
    int *in_flight_arr,
    int &in_flight_count,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_1,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_2,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_3,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_4,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_5,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_6,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_7,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_8,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_1,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_2,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_3,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_4,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_5,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_6,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_7,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_8,
    ap_uint<128> msg_buffer[MAX_COMMAND_BUFFER_NUM][MAX_COMMAND_BUFFER_LENGTH],
    int *count_arr)
{
    while (check_inflight(in_flight_arr, pma_idx) || in_flight_count >= MAX_INFLIGHT_NUM) {
        check_ack_back_array(
            in_flight_arr,
            in_flight_count,
            ack_from_insert_edge_1,
            ack_from_insert_edge_2,
            ack_from_insert_edge_3,
            ack_from_insert_edge_4,
            ack_from_insert_edge_5,
            ack_from_insert_edge_6,
            ack_from_insert_edge_7,
            ack_from_insert_edge_8);
    }

    if (count_arr[hash] > 0) {
        int worker = pma_idx % ROUTER_WORKERS;
        flush_edge_buffer_to_worker(
            worker,
            hash,
            msg_buffer,
            count_arr,
            cmd_to_insert_edge_1,
            cmd_to_insert_edge_2,
            cmd_to_insert_edge_3,
            cmd_to_insert_edge_4,
            cmd_to_insert_edge_5,
            cmd_to_insert_edge_6,
            cmd_to_insert_edge_7,
            cmd_to_insert_edge_8);
        add_inflight(in_flight_arr, in_flight_count, pma_idx);
    }
}

extern "C" {
void update_message_router(
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_from_generator,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_1,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_2,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_3,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_4,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_5,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_6,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_7,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_8,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_1,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_2,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_3,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_4,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_5,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_6,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_7,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_8,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_1,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_2,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_3,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_4,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_5,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_6,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_7,
    hls::stream<COMMAND_STREAM> &ack_from_insert_edge_8,
    hls::stream<VERTEX_ACK> &ack_from_insert_vertex_1,
    hls::stream<VERTEX_ACK> &ack_from_insert_vertex_2,
    hls::stream<VERTEX_ACK> &ack_from_insert_vertex_3,
    hls::stream<VERTEX_ACK> &ack_from_insert_vertex_4,
    hls::stream<VERTEX_ACK> &ack_from_insert_vertex_5,
    hls::stream<VERTEX_ACK> &ack_from_insert_vertex_6,
    hls::stream<VERTEX_ACK> &ack_from_insert_vertex_7,
    hls::stream<VERTEX_ACK> &ack_from_insert_vertex_8,
    hls::stream<WHOLE_ACK> &ack_to_generator)
{
    #pragma HLS INTERFACE axis port=cmd_from_generator
    #pragma HLS INTERFACE axis port=cmd_to_insert_vertex_1
    #pragma HLS INTERFACE axis port=cmd_to_insert_vertex_2
    #pragma HLS INTERFACE axis port=cmd_to_insert_vertex_3
    #pragma HLS INTERFACE axis port=cmd_to_insert_vertex_4
    #pragma HLS INTERFACE axis port=cmd_to_insert_vertex_5
    #pragma HLS INTERFACE axis port=cmd_to_insert_vertex_6
    #pragma HLS INTERFACE axis port=cmd_to_insert_vertex_7
    #pragma HLS INTERFACE axis port=cmd_to_insert_vertex_8
    #pragma HLS INTERFACE axis port=cmd_to_insert_edge_1
    #pragma HLS INTERFACE axis port=cmd_to_insert_edge_2
    #pragma HLS INTERFACE axis port=cmd_to_insert_edge_3
    #pragma HLS INTERFACE axis port=cmd_to_insert_edge_4
    #pragma HLS INTERFACE axis port=cmd_to_insert_edge_5
    #pragma HLS INTERFACE axis port=cmd_to_insert_edge_6
    #pragma HLS INTERFACE axis port=cmd_to_insert_edge_7
    #pragma HLS INTERFACE axis port=cmd_to_insert_edge_8
    #pragma HLS INTERFACE axis port=ack_from_insert_edge_1
    #pragma HLS INTERFACE axis port=ack_from_insert_edge_2
    #pragma HLS INTERFACE axis port=ack_from_insert_edge_3
    #pragma HLS INTERFACE axis port=ack_from_insert_edge_4
    #pragma HLS INTERFACE axis port=ack_from_insert_edge_5
    #pragma HLS INTERFACE axis port=ack_from_insert_edge_6
    #pragma HLS INTERFACE axis port=ack_from_insert_edge_7
    #pragma HLS INTERFACE axis port=ack_from_insert_edge_8
    #pragma HLS INTERFACE axis port=ack_from_insert_vertex_1
    #pragma HLS INTERFACE axis port=ack_from_insert_vertex_2
    #pragma HLS INTERFACE axis port=ack_from_insert_vertex_3
    #pragma HLS INTERFACE axis port=ack_from_insert_vertex_4
    #pragma HLS INTERFACE axis port=ack_from_insert_vertex_5
    #pragma HLS INTERFACE axis port=ack_from_insert_vertex_6
    #pragma HLS INTERFACE axis port=ack_from_insert_vertex_7
    #pragma HLS INTERFACE axis port=ack_from_insert_vertex_8
    #pragma HLS INTERFACE axis port=ack_to_generator

    #pragma HLS STREAM variable=cmd_from_generator depth=16
    #pragma HLS STREAM variable=cmd_to_insert_vertex_1 depth=16
    #pragma HLS STREAM variable=cmd_to_insert_vertex_2 depth=16
    #pragma HLS STREAM variable=cmd_to_insert_vertex_3 depth=16
    #pragma HLS STREAM variable=cmd_to_insert_vertex_4 depth=16
    #pragma HLS STREAM variable=cmd_to_insert_vertex_5 depth=16
    #pragma HLS STREAM variable=cmd_to_insert_vertex_6 depth=16
    #pragma HLS STREAM variable=cmd_to_insert_vertex_7 depth=16
    #pragma HLS STREAM variable=cmd_to_insert_vertex_8 depth=16
    #pragma HLS STREAM variable=cmd_to_insert_edge_1 depth=16
    #pragma HLS STREAM variable=cmd_to_insert_edge_2 depth=16
    #pragma HLS STREAM variable=cmd_to_insert_edge_3 depth=16
    #pragma HLS STREAM variable=cmd_to_insert_edge_4 depth=16
    #pragma HLS STREAM variable=cmd_to_insert_edge_5 depth=16
    #pragma HLS STREAM variable=cmd_to_insert_edge_6 depth=16
    #pragma HLS STREAM variable=cmd_to_insert_edge_7 depth=16
    #pragma HLS STREAM variable=cmd_to_insert_edge_8 depth=16
    #pragma HLS STREAM variable=ack_from_insert_edge_1 depth=16
    #pragma HLS STREAM variable=ack_from_insert_edge_2 depth=16
    #pragma HLS STREAM variable=ack_from_insert_edge_3 depth=16
    #pragma HLS STREAM variable=ack_from_insert_edge_4 depth=16
    #pragma HLS STREAM variable=ack_from_insert_edge_5 depth=16
    #pragma HLS STREAM variable=ack_from_insert_edge_6 depth=16
    #pragma HLS STREAM variable=ack_from_insert_edge_7 depth=16
    #pragma HLS STREAM variable=ack_from_insert_edge_8 depth=16
    #pragma HLS STREAM variable=ack_from_insert_vertex_1 depth=16
    #pragma HLS STREAM variable=ack_from_insert_vertex_2 depth=16
    #pragma HLS STREAM variable=ack_from_insert_vertex_3 depth=16
    #pragma HLS STREAM variable=ack_from_insert_vertex_4 depth=16
    #pragma HLS STREAM variable=ack_from_insert_vertex_5 depth=16
    #pragma HLS STREAM variable=ack_from_insert_vertex_6 depth=16
    #pragma HLS STREAM variable=ack_from_insert_vertex_7 depth=16
    #pragma HLS STREAM variable=ack_from_insert_vertex_8 depth=16
    #pragma HLS STREAM variable=ack_to_generator depth=16

    ap_uint<128> msg_buffer[MAX_COMMAND_BUFFER_NUM][MAX_COMMAND_BUFFER_LENGTH];
    #pragma HLS BIND_STORAGE variable=msg_buffer type=RAM_2P impl=URAM
    int count[MAX_COMMAND_BUFFER_NUM];
    #pragma HLS ARRAY_PARTITION variable=count complete

    int in_flight[MAX_INFLIGHT_NUM];
    for (int i = 0; i < MAX_INFLIGHT_NUM; i++) in_flight[i] = -1;
    int in_flight_count = 0;
    hls::print("in_flight_count %d\n", in_flight_count);

    for (int i = 0; i < MAX_COMMAND_BUFFER_NUM; i++) {
        for (int j = 0; j < MAX_COMMAND_BUFFER_LENGTH; j++) {
            #pragma HLS UNROLL
            msg_buffer[i][j] = 0;
        }
        count[i] = 0;
    }

    int stage = 0;
    PMA_UPDATE_MESSAGE msg;
    bool quit = false;
    while (true) {
        switch (stage) {
            case 0:
            {
                if (cmd_from_generator.read_nb(msg)) {
                    if (msg.data == -1) {
                        stage++;
                        msg.data = PMA_STOP;
                        broadcast_vertex_stop(
                            msg,
                            cmd_to_insert_vertex_1,
                            cmd_to_insert_vertex_2,
                            cmd_to_insert_vertex_3,
                            cmd_to_insert_vertex_4,
                            cmd_to_insert_vertex_5,
                            cmd_to_insert_vertex_6,
                            cmd_to_insert_vertex_7,
                            cmd_to_insert_vertex_8);
                        break;
                    }
                    int src = msg.data.range(31, 0);
                    int pma_idx = src / VERTEX_PER_PMA;
                    int worker = pma_idx % ROUTER_WORKERS;
                    route_vertex_to_worker(
                        worker,
                        msg,
                        cmd_to_insert_vertex_1,
                        cmd_to_insert_vertex_2,
                        cmd_to_insert_vertex_3,
                        cmd_to_insert_vertex_4,
                        cmd_to_insert_vertex_5,
                        cmd_to_insert_vertex_6,
                        cmd_to_insert_vertex_7,
                        cmd_to_insert_vertex_8);
                }
                break;
            }
            case 1:
            {
                bool acked[ROUTER_WORKERS];
                #pragma HLS ARRAY_PARTITION variable=acked complete
                for (int i = 0; i < ROUTER_WORKERS; ++i) {
                    #pragma HLS UNROLL
                    acked[i] = false;
                }
                VERTEX_ACK tmp;
                while (true) {
                    poll_vertex_acks(
                        acked,
                        tmp,
                        ack_from_insert_vertex_1,
                        ack_from_insert_vertex_2,
                        ack_from_insert_vertex_3,
                        ack_from_insert_vertex_4,
                        ack_from_insert_vertex_5,
                        ack_from_insert_vertex_6,
                        ack_from_insert_vertex_7,
                        ack_from_insert_vertex_8);

                    bool all_acked = true;
                    for (int i = 0; i < ROUTER_WORKERS; ++i) {
                        #pragma HLS UNROLL
                        all_acked = all_acked && acked[i];
                    }
                    if (all_acked) break;
                }
                stage++;
            }
            case 2:
            {
                check_ack_back_array(
                    in_flight,
                    in_flight_count,
                    ack_from_insert_edge_1,
                    ack_from_insert_edge_2,
                    ack_from_insert_edge_3,
                    ack_from_insert_edge_4,
                    ack_from_insert_edge_5,
                    ack_from_insert_edge_6,
                    ack_from_insert_edge_7,
                    ack_from_insert_edge_8);

                if (cmd_from_generator.read_nb(msg)) {
                    if (msg.data == -1) {
                        msg.data = PMA_STOP;
                        for (int i = 0; i < MAX_COMMAND_BUFFER_NUM; i++) {
                            if (count[i] > 0) {
                                flush_and_add_inflight(
                                    i,
                                    msg_buffer[i][0].range(31, 0) / VERTEX_PER_PMA,
                                    in_flight,
                                    in_flight_count,
                                    ack_from_insert_edge_1,
                                    ack_from_insert_edge_2,
                                    ack_from_insert_edge_3,
                                    ack_from_insert_edge_4,
                                    ack_from_insert_edge_5,
                                    ack_from_insert_edge_6,
                                    ack_from_insert_edge_7,
                                    ack_from_insert_edge_8,
                                    cmd_to_insert_edge_1,
                                    cmd_to_insert_edge_2,
                                    cmd_to_insert_edge_3,
                                    cmd_to_insert_edge_4,
                                    cmd_to_insert_edge_5,
                                    cmd_to_insert_edge_6,
                                    cmd_to_insert_edge_7,
                                    cmd_to_insert_edge_8,
                                    msg_buffer,
                                    count);
                            }
                        }

                        if (msg.last == 1) {
                            broadcast_edge_stop(
                                msg,
                                cmd_to_insert_edge_1,
                                cmd_to_insert_edge_2,
                                cmd_to_insert_edge_3,
                                cmd_to_insert_edge_4,
                                cmd_to_insert_edge_5,
                                cmd_to_insert_edge_6,
                                cmd_to_insert_edge_7,
                                cmd_to_insert_edge_8);
                            quit = true;
                            stage++;
                        } else {
                            stage = 0;
                        }

                        while (in_flight_count > 0) {
                            check_ack_back_array(
                                in_flight,
                                in_flight_count,
                                ack_from_insert_edge_1,
                                ack_from_insert_edge_2,
                                ack_from_insert_edge_3,
                                ack_from_insert_edge_4,
                                ack_from_insert_edge_5,
                                ack_from_insert_edge_6,
                                ack_from_insert_edge_7,
                                ack_from_insert_edge_8);
                        }

                        WHOLE_ACK ack_pkt;
                        ack_pkt.data = 1;
                        ack_pkt.keep = (ap_uint<4>)0xF;
                        ack_pkt.strb = (ap_uint<4>)0xF;
                        ack_pkt.last = 1;
                        ack_to_generator.write(ack_pkt);
                        break;
                    }

                    int src = msg.data.range(31, 0);
                    int pma_idx = src / VERTEX_PER_PMA;
                    int hash = -1;
                    // ap_wait();
                    get_hash_value(pma_idx, hash);
                    // ap_wait();

                    if (count[hash] == 0) {
                        msg_buffer[hash][count[hash]] = msg.data;
                        count[hash]++;
                    } else if (count[hash] < MAX_COMMAND_BUFFER_LENGTH) {
                        if (msg_buffer[hash][0].range(31, 0) / VERTEX_PER_PMA ==
                            msg.data.range(31, 0) / VERTEX_PER_PMA) {
                            msg_buffer[hash][count[hash]] = msg.data;
                            count[hash]++;
                        } else {
                            flush_and_add_inflight(
                                hash,
                                msg_buffer[hash][0].range(31, 0) / VERTEX_PER_PMA,
                                in_flight,
                                in_flight_count,
                                ack_from_insert_edge_1,
                                ack_from_insert_edge_2,
                                ack_from_insert_edge_3,
                                ack_from_insert_edge_4,
                                ack_from_insert_edge_5,
                                ack_from_insert_edge_6,
                                ack_from_insert_edge_7,
                                ack_from_insert_edge_8,
                                cmd_to_insert_edge_1,
                                cmd_to_insert_edge_2,
                                cmd_to_insert_edge_3,
                                cmd_to_insert_edge_4,
                                cmd_to_insert_edge_5,
                                cmd_to_insert_edge_6,
                                cmd_to_insert_edge_7,
                                cmd_to_insert_edge_8,
                                msg_buffer,
                                count);
                            msg_buffer[hash][count[hash]] = msg.data;
                            count[hash]++;
                        }
                    } else {
                        flush_and_add_inflight(
                            hash,
                            msg_buffer[hash][0].range(31, 0) / VERTEX_PER_PMA,
                            in_flight,
                            in_flight_count,
                            ack_from_insert_edge_1,
                            ack_from_insert_edge_2,
                            ack_from_insert_edge_3,
                            ack_from_insert_edge_4,
                            ack_from_insert_edge_5,
                            ack_from_insert_edge_6,
                            ack_from_insert_edge_7,
                            ack_from_insert_edge_8,
                            cmd_to_insert_edge_1,
                            cmd_to_insert_edge_2,
                            cmd_to_insert_edge_3,
                            cmd_to_insert_edge_4,
                            cmd_to_insert_edge_5,
                            cmd_to_insert_edge_6,
                            cmd_to_insert_edge_7,
                            cmd_to_insert_edge_8,
                            msg_buffer,
                            count);
                        msg_buffer[hash][count[hash]] = msg.data;
                        count[hash]++;
                    }
                }
                break;
            }
            case 3:
            {
                quit = true;
                break;
            }
        }
        if (quit) {
            break;
        }
    }

    hls::print("in_flight_count after end %d\n", in_flight_count);
}
}
