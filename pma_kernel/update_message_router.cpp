#include "data_type.h"
#include "debug.h"
#include "etc/autopilot_ssdm_op.h"
#include "hls_print.h"
#include "hls_stream.h"
#include <cstdio>

#ifndef MAX_INFLIGHT_NUM
#define MAX_INFLIGHT_NUM 16
#endif

#define ROUTER_WORKERS CHANNEL_NUM

void flush_buffer_aux(
    int buffer_idx,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_pma,
    PMA_UPDATE_MESSAGE msg_buffer[MAX_COMMAND_BUFFER_NUM][MAX_COMMAND_BUFFER_LENGTH],
    int *count_arr
)
{
    #pragma HLS INLINE 
    int c = count_arr[buffer_idx];
    for (int j = 0 ; j < c; j++) {
        #pragma HLS PIPELINE II=1
        PMA_UPDATE_MESSAGE tmp;
        tmp = msg_buffer[buffer_idx][j];
        tmp.last = (j == c - 1) ? 1 : 0;
        cmd_to_pma.write(tmp);
    }
    // hls::print("Flushed buffer with %d messages\n", c);
    count_arr[buffer_idx] = 0;
}

bool check_inflight(int *in_flight_arr, int pma_idx)
{
    #pragma HLS INLINE 
    bool found = false;
    for (int i = 0 ; i < MAX_INFLIGHT_NUM ; i++)
    {
        #pragma HLS PIPELINE II=1
        if (in_flight_arr[i] == pma_idx)
            found = true;
    }
    return found;
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
    if (ack_stream.read_nb(ack))
    {
        int pma_idx = ack.data;
        int find_idx = -1;
        for (int i = 0 ; i < MAX_INFLIGHT_NUM ; i++)
        {
            if (in_flight_arr[i] == pma_idx)
            {
                // in_flight_arr[i] = -1;
                // in_flight_count--;
                // // hls::print("infight removed, count %d\n", in_flight_count);
                // break;
                find_idx = i;
            }
        }
        if (find_idx != -1)
        {
            in_flight_arr[find_idx] = -1;
            in_flight_count--;
        }
    }
}

static void check_ack_back_array(
    int *in_flight_arr,
    int &in_flight_count,
    hls::stream<COMMAND_STREAM>& ack_from_insert_edge_1,
    hls::stream<COMMAND_STREAM>& ack_from_insert_edge_2
    // hls::stream<COMMAND_STREAM>& ack_from_insert_edge_3,
    // hls::stream<COMMAND_STREAM>& ack_from_insert_edge_4
)
{
    #pragma HLS INLINE
    update_inflight_from_ack(in_flight_arr, in_flight_count, ack_from_insert_edge_1);
    update_inflight_from_ack(in_flight_arr, in_flight_count, ack_from_insert_edge_2);
    // update_inflight_from_ack(in_flight_arr, in_flight_count, ack_from_insert_edge_3);
    // update_inflight_from_ack(in_flight_arr, in_flight_count, ack_from_insert_edge_4);
}

void add_inflight(int *in_flight_arr, int &in_flight_count, int pma_idx)
{
    for (int i  = 0 ; i < MAX_INFLIGHT_NUM ; i++)
    {
        if (in_flight_arr[i] == -1)
        {
            in_flight_arr[i] = pma_idx;
            in_flight_count++;
            // hls::print("infight added, count %d\n", in_flight_count);
            break;
        }
    }
}

void flush_and_add_inflight(
    int hash,
    int pma_idx,
    int *in_flight_arr,
    int &in_flight_count,
    hls::stream<COMMAND_STREAM>& ack_from_insert_edge_1,
    hls::stream<COMMAND_STREAM>& ack_from_insert_edge_2,
    // hls::stream<COMMAND_STREAM>& ack_from_insert_edge_3,
    // hls::stream<COMMAND_STREAM>& ack_from_insert_edge_4,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_1,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_2,
    // hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_3,
    // hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_4,
    PMA_UPDATE_MESSAGE msg_buffer[MAX_COMMAND_BUFFER_NUM][MAX_COMMAND_BUFFER_LENGTH],
    int *count_arr)
{
    while (check_inflight(in_flight_arr, pma_idx) || in_flight_count >= MAX_INFLIGHT_NUM)
    {
        check_ack_back_array(
            in_flight_arr,
            in_flight_count,
            ack_from_insert_edge_1,
            ack_from_insert_edge_2
            // ack_from_insert_edge_3,
            // ack_from_insert_edge_4
        );
        ap_wait();
    }

    if (count_arr[hash] > 0) {
        int worker = pma_idx % ROUTER_WORKERS;
        switch (worker)
        {
            case 0: flush_buffer_aux(hash, cmd_to_insert_edge_1, msg_buffer, count_arr); break;
            case 1: flush_buffer_aux(hash, cmd_to_insert_edge_2, msg_buffer, count_arr); break;
            // case 2: flush_buffer_aux(hash, cmd_to_insert_edge_3, msg_buffer, count_arr); break;
            // case 3: flush_buffer_aux(hash, cmd_to_insert_edge_4, msg_buffer, count_arr); break;
            default: break;
        }
        add_inflight(in_flight_arr, in_flight_count, pma_idx);
    }
}

extern "C"
{
    void update_message_router(
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_from_generator,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_1,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_2,
        // hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_3,
        // hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_4,
        hls::stream<COMMAND_STREAM> &ack_from_insert_edge_1,
        hls::stream<COMMAND_STREAM> &ack_from_insert_edge_2,
        // hls::stream<COMMAND_STREAM> &ack_from_insert_edge_3,
        // hls::stream<COMMAND_STREAM> &ack_from_insert_edge_4,
        hls::stream<WHOLE_ACK> &ack_to_generator
    )
    {
        #pragma HLS INTERFACE axis port=cmd_from_generator
        #pragma HLS INTERFACE axis port=cmd_to_insert_edge_1
        #pragma HLS INTERFACE axis port=cmd_to_insert_edge_2
        // #pragma HLS INTERFACE axis port=cmd_to_insert_edge_3
        // #pragma HLS INTERFACE axis port=cmd_to_insert_edge_4
        #pragma HLS INTERFACE axis port=ack_from_insert_edge_1
        #pragma HLS INTERFACE axis port=ack_from_insert_edge_2
        // #pragma HLS INTERFACE axis port=ack_from_insert_edge_3
        // #pragma HLS INTERFACE axis port=ack_from_insert_edge_4
        #pragma HLS INTERFACE axis port=ack_to_generator

        #pragma HLS STREAM variable=cmd_from_generator depth=16
        #pragma HLS STREAM variable=cmd_to_insert_edge_1 depth=16
        #pragma HLS STREAM variable=cmd_to_insert_edge_2 depth=16
        // #pragma HLS STREAM variable=cmd_to_insert_edge_3 depth=16
        // #pragma HLS STREAM variable=cmd_to_insert_edge_4 depth=16
        #pragma HLS STREAM variable=ack_from_insert_edge_1 depth=16
        #pragma HLS STREAM variable=ack_from_insert_edge_2 depth=16
        // #pragma HLS STREAM variable=ack_from_insert_edge_3 depth=16
        // #pragma HLS STREAM variable=ack_from_insert_edge_4 depth=16
        #pragma HLS STREAM variable=ack_to_generator depth=16

        PMA_UPDATE_MESSAGE msg_buffer[MAX_COMMAND_BUFFER_NUM][MAX_COMMAND_BUFFER_LENGTH];
        #pragma HLS BIND_STORAGE variable=msg_buffer type=RAM_2P impl=URAM
        int count[MAX_COMMAND_BUFFER_NUM];
        #pragma HLS ARRAY_PARTITION variable=count complete

        int in_flight[MAX_INFLIGHT_NUM];
        #pragma HLS ARRAY_PARTITION variable=in_flight complete
        for (int i = 0 ; i < MAX_INFLIGHT_NUM ; i++) in_flight[i] = -1;
        int in_flight_count = 0;
        // hls::print("in_flight_count %d\n", in_flight_count);
        for (int i = 0 ; i < MAX_COMMAND_BUFFER_NUM ; i++)
        {
            count[i] = 0;
        }

        while (true)
        { 
            check_ack_back_array(
                in_flight,
                in_flight_count,
                ack_from_insert_edge_1,
                ack_from_insert_edge_2
                // ack_from_insert_edge_3,
                // ack_from_insert_edge_4
            );
            ap_wait();
            PMA_UPDATE_MESSAGE msg;
            if(cmd_from_generator.read_nb(msg))
            {
                if (msg.opcode == PMA_UPDATE_STOP)
                {
                    for (int i = 0 ; i < MAX_COMMAND_BUFFER_NUM ; i++)
                    {
                        if (count[i] > 0)
                        {
                            flush_and_add_inflight(
                                i, msg_buffer[i][0].src / VERTEX_PER_PMA,
                                in_flight, in_flight_count,
                                ack_from_insert_edge_1,
                                ack_from_insert_edge_2,
                                // ack_from_insert_edge_3,
                                // ack_from_insert_edge_4,
                                cmd_to_insert_edge_1,
                                cmd_to_insert_edge_2,
                                // cmd_to_insert_edge_3,
                                // cmd_to_insert_edge_4,
                                msg_buffer, count
                            );
                        }
                    }
                    while (in_flight_count > 0)
                    {
                        check_ack_back_array(
                            in_flight,
                            in_flight_count,
                            ack_from_insert_edge_1,
                            ack_from_insert_edge_2
                            // ack_from_insert_edge_3,
                            // ack_from_insert_edge_4
                        );
                        ap_wait();
                    }
                    hls::print("in_flight_count after flush %d\n", in_flight_count);
                   
                    if (msg.last == 1)
                    {
                        msg.opcode = PMA_UPDATE_STOP;
                        cmd_to_insert_edge_1.write(msg);
                        cmd_to_insert_edge_2.write(msg);
                        #pragma HLS protocol fixed
                        auto ack_1 = ack_from_insert_edge_1.read();
                        auto ack_2 = ack_from_insert_edge_2.read();
                        #pragma HLS protocol fixed
                        WHOLE_ACK ack_pkt;
                        ack_pkt.data = 1;
                        ack_pkt.keep = (ap_uint<4>)0xF;
                        ack_pkt.strb = (ap_uint<4>)0xF;
                        ack_pkt.last = 1;
                        ack_to_generator.write(ack_pkt);
                        break;
                    }
                    else
                    {
                        msg.opcode = PMA_UPDATE_FLUSH_CACHE;
                        cmd_to_insert_edge_1.write(msg);
                        cmd_to_insert_edge_2.write(msg);
                        #pragma HLS protocol fixed
                        auto ack_1 = ack_from_insert_edge_1.read();
                        auto ack_2 = ack_from_insert_edge_2.read();
                        #pragma HLS protocol fixed
                        WHOLE_ACK ack_pkt;
                        ack_pkt.data = 1;
                        ack_pkt.keep = (ap_uint<4>)0xF;
                        ack_pkt.strb = (ap_uint<4>)0xF;
                        ack_pkt.last = 1;
                        ack_to_generator.write(ack_pkt);
                    }
                }
                else  
                {
                    
                    int src = msg.src;
                    int pma_idx = src / VERTEX_PER_PMA;
                    int hash = -1;
                    // ap_wait();
                    get_hash_value(pma_idx, hash);
                    // ap_wait();
                    if (count[hash] == 0)   
                    {
                        msg_buffer[hash][count[hash]] = msg;
                        count[hash]++;
                    }
                    else if (count[hash] < MAX_COMMAND_BUFFER_LENGTH)
                    {
                        if (msg_buffer[hash][0].src / VERTEX_PER_PMA == msg.src / VERTEX_PER_PMA)
                        {
                            msg_buffer[hash][count[hash]] = msg;
                            count[hash]++;
                        }
                        else
                        {
                            flush_and_add_inflight(
                                hash, msg_buffer[hash][0].src / VERTEX_PER_PMA,
                                in_flight, in_flight_count,
                                ack_from_insert_edge_1,
                                ack_from_insert_edge_2,
                                // ack_from_insert_edge_3,
                                // ack_from_insert_edge_4,
                                cmd_to_insert_edge_1,
                                cmd_to_insert_edge_2,
                                // cmd_to_insert_edge_3,
                                // cmd_to_insert_edge_4,
                                msg_buffer, count
                            );
                            msg_buffer[hash][count[hash]] = msg;
                            count[hash]++;
                        }
                    }
                    else
                    {
                        flush_and_add_inflight(
                            hash, msg_buffer[hash][0].src / VERTEX_PER_PMA,
                            in_flight, in_flight_count,
                            ack_from_insert_edge_1,
                            ack_from_insert_edge_2,
                            // ack_from_insert_edge_3,
                            // ack_from_insert_edge_4,
                            cmd_to_insert_edge_1,
                            cmd_to_insert_edge_2,
                            // cmd_to_insert_edge_3,
                            // cmd_to_insert_edge_4,
                            msg_buffer, count
                        );
                        msg_buffer[hash][count[hash]] = msg;
                        count[hash]++;
                    }
                }
            }
        }
    }
}
