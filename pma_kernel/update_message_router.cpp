#include "data_type.h"
#include "debug.h"
#include "debug.h"
#include "hls_stream.h"
#include <cstdio>
// static int task_tag = 0;
void flush_buffer_aux(
    int buffer_idx,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_pma,
    ap_uint<128> msg_buffer[MAX_COMMAND_BUFFER_NUM][MAX_COMMAND_BUFFER_LENGTH],
    int *count_arr
)
{
    int c = count_arr[buffer_idx];
    for (int j = 0 ; j < c; j++) {
        #pragma HLS PIPELINE II=1
        PMA_UPDATE_MESSAGE tmp;
        tmp.data = msg_buffer[buffer_idx][j];
        tmp.last = (j == c - 1) ? 1 : 0;
        cmd_to_pma.write(tmp);
        printf("Flushing buffer %d, command type: %d, SRC = %d, DST = %d\n", buffer_idx, int(tmp.data.range(97,96)), 
        int(tmp.data.range(31,0)), int(tmp.data.range(63,32)));
    }
    count_arr[buffer_idx] = 0;
}
// void check_ack(
//     hls::stream<COMMAND_STREAM> &ack_from_pma_1,
//     hls::stream<COMMAND_STREAM> &ack_from_pma_2,
//     int* processing_pma_idx
// )
// {
//     COMMAND_STREAM ack_val_1, ack_val_2;
//     auto ack_1 = ack_from_pma_1.read_nb(ack_val_1);
//     auto ack_2 = ack_from_pma_2.read_nb(ack_val_2);
//     if (ack_1)
//     {
//         processing_pma_idx[0] = -1;
//         printf("ack received from pma 1\n");
//     }
//     if (ack_2)
//     {
//         processing_pma_idx[1] = -1;
//         printf("ack received from pma 2\n");
//     }
//     // printf("prossessing_pma_idx[0]: %d, processing_pma_idx[1]: %d\n", processing_pma_idx[0], processing_pma_idx[1]);
// }

// void flush_buffer(
//     int buffer_idx,
//     hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_pma_1,
//     hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_pma_2,
//     hls::stream<COMMAND_STREAM> &ack_from_pma_1,
//     hls::stream<COMMAND_STREAM> &ack_from_pma_2,
//     ap_uint<128> msg_buffer[MAX_COMMAND_BUFFER_NUM][MAX_COMMAND_BUFFER_LENGTH],
//     int *processing_pma_idx,
//     int *count_arr
// )
// {
//     if (count_arr[buffer_idx] == 0)
//     {
//         return;
//     }
//     bool complete = false;
//     int pma_idx = msg_buffer[buffer_idx][0].range(31,0) / VERTEX_PER_PMA;
//     while (!complete)
//     {
//         // HERE
//         check_ack(ack_from_pma_1, ack_from_pma_2, processing_pma_idx);
//         for (int i = 0 ; i < MAX_WORKERS ; i++)
//         {
//             // HERE
//             if (processing_pma_idx[i] == -1)
//             {
//                 bool not_conflict = true;
//                 for (int w = 0; w < MAX_WORKERS ; w++)
//                 {
//                     if (processing_pma_idx[w] == pma_idx)
//                     {
//                         // HERE
//                         not_conflict = false;
//                         break;
//                     }
//                 }
//                 if (not_conflict)
//                 {
//                     // HERE
//                     for (int debug = 0 ; debug < MAX_WORKERS ; debug++)
//                     {
//                         printf("processing_pma_idx[%d] = %d\n", debug, processing_pma_idx[debug]);
//                     }
//                     if (i == 0)
//                     {
//                         printf("Assigned PMA IDX %d to worker %d\n", pma_idx, i);
//                         flush_buffer_aux(
//                             buffer_idx,
//                             cmd_to_pma_1,
//                             msg_buffer,
//                             count_arr,
//                             0
//                         );
//                     }
//                     else  
//                     {
//                         printf("Assigned PMA IDX %d to worker %d\n", pma_idx, i);
//                         flush_buffer_aux(
//                             buffer_idx,
//                             cmd_to_pma_2,
//                             msg_buffer,
//                             count_arr,
//                             1
//                         );
//                     }
//                     processing_pma_idx[i] = pma_idx;
//                     complete = true;
//                     break;
//                 }
//             }
//         }
//     }
// }
extern "C"
{
    void update_message_router(
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_from_generator,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_1,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_vertex_2,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_1,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_2
        // hls::stream<CLEAR_ICON> &clear_message_from_generator
    )
    {
        #pragma HLS INTERFACE axis port=cmd_from_generator
        #pragma HLS INTERFACE axis port=cmd_to_insert_vertex_1
        #pragma HLS INTERFACE axis port=cmd_to_insert_vertex_2
        #pragma HLS INTERFACE axis port=cmd_to_insert_edge_1
        #pragma HLS INTERFACE axis port=cmd_to_insert_edge_2
        // #pragma HLS INTERFACE axis port=clear_message_from_generator
        // int processing_pma_idx[MAX_WORKERS] = {-1};
        // #pragma HLS ARRAY_PARTITION variable=processing_pma_idx complete
        // for (int i = 0 ; i < MAX_WORKERS ; i++)
        // {
        //     #pragma HLS UNROLL
        //     processing_pma_idx[i] = -1;
        // }
        ap_uint<128> msg_buffer[MAX_COMMAND_BUFFER_NUM][MAX_COMMAND_BUFFER_LENGTH];
        #pragma HLS BIND_STORAGE variable=msg_buffer type=RAM_2P impl=URAM
        int count[MAX_COMMAND_BUFFER_NUM]; 
        #pragma HLS ARRAY_PARTITION variable=count complete
        for (int i = 0 ; i < MAX_COMMAND_BUFFER_NUM ; i++)
        {
            // #pragma HLS UNROLL
            for (int j = 0 ; j < MAX_COMMAND_BUFFER_LENGTH ; j++)
            {
                #pragma HLS UNROLL
                msg_buffer[i][j] = 0;
            }
            count[i] = 0;
        }
        int stage = 0;
        PMA_UPDATE_MESSAGE msg;
        bool quit = false;
        while (true)
        {
            switch (stage)
            {
                case 0://insert vertex
                {
                    if(cmd_from_generator.read_nb(msg))
                    {
                        if (msg.data == 0)
                        {
                            stage++;
                        }
                        int src = msg.data.range(31,0);  
                        if (src % 2 == 0) {
                            cmd_to_insert_vertex_1.write(msg);
                        } else {
                            cmd_to_insert_vertex_2.write(msg);
                        }
                    }
                    break;
                }
                case 1:
                {
                    if(cmd_from_generator.read_nb(msg))
                    {
                        if (msg.data == 0)
                        {
                            stage++;
                        }
                        int src = msg.data.range(31,0);  
                        int pma_idx = src / VERTEX_PER_PMA;
                        int hash = pma_idx % (MAX_COMMAND_BUFFER_NUM / 2);
                        if (msg.data.range(98,98) == OUT_DIRECTION)
                        {
                            hash += MAX_COMMAND_BUFFER_NUM / 2;
                        }
                        if (count[hash] == 0)
                        {
                            msg_buffer[hash][count[hash]] = msg.data;
                            count[hash]++;
                        }
                        else if (count[hash] < 16)
                        {
                            if (msg_buffer[hash][0].range(31,0) / VERTEX_PER_PMA == msg.data.range(31,0) / VERTEX_PER_PMA)
                            {
                                msg_buffer[hash][count[hash]] = msg.data;
                                count[hash]++;
                            }
                            else  
                            {
                                if (hash % 2 == 0)
                                    flush_buffer_aux(hash, cmd_to_insert_edge_1, msg_buffer, count);
                                else  
                                    flush_buffer_aux(hash, cmd_to_insert_edge_2, msg_buffer, count);
                            }
                        }
                        else  
                        {
                            if (hash % 2 == 0)
                                flush_buffer_aux(hash, cmd_to_insert_edge_1, msg_buffer, count);
                            else  
                                flush_buffer_aux(hash, cmd_to_insert_edge_2, msg_buffer, count);
                        }
                    }
                }
                case 2:
                {
                    quit = true;
                    break;
                }
            }
            if (quit)
            {
                break;
            }
        }
    }
}