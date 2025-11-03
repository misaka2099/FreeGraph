#include "data_type.h"
#include <cstdio>
#include <hls_stream.h>
#include "debug.h"
extern "C"
{
    void update_message_generator(
        int add_vertex_num,
        int add_edge_num,
        int *src,
        int *dst,
        int *src_to_change,
        double *attr_to_change,
        int src_attr_change_num,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_router
        // hls::stream<CLEAR_ICON> &clear_message_to_router
    )
    {
        #pragma HLS INTERFACE s_axilite port=add_vertex_num
        #pragma HLS INTERFACE s_axilite port=add_edge_num
        #pragma HLS INTERFACE m_axi port=src bundle=gmem0
        #pragma HLS INTERFACE m_axi port=dst bundle=gmem1
        #pragma HLS INTERFACE m_axi port=src_to_change bundle=gmem2
        #pragma HLS INTERFACE m_axi port=attr_to_change bundle=gmem3
        #pragma HLS INTERFACE s_axilite port=src_attr_change_num
        #pragma HLS INTERFACE s_axilite port=return
        #pragma HLS INTERFACE axis port=cmd_to_router
        // #pragma HLS INTERFACE axis port=clear_message_to_router
        // static int total_vertex_num_in = 0;
        //(66,66) for direction (65,64) for command type (63,32) for dst (31,0) for src
        // static int total_vertex_num_out = 0;
        // HERE
        for (int i = 0 ; i < add_vertex_num ; i++)
        {
            #pragma HLS PIPELINE II=1
            PMA_UPDATE_MESSAGE msg;
            msg.data.range(31,0) = i;
            msg.data.range(63,32) = 0;
            msg.data.range(97,96) = PMA_INSERT_VERTEX;
            msg.data.range(98,98) = IN_DIRECTION; //default in direction
            cmd_to_router.write(msg);
        }
        // HERE
        for (int i = 0 ; i < add_vertex_num ; i++)
        {
            #pragma HLS PIPELINE II=1
            PMA_UPDATE_MESSAGE msg;
            msg.data.range(31,0) = i;
            msg.data.range(63,32) = 0;
            msg.data.range(97,96) = PMA_INSERT_VERTEX;
            msg.data.range(98,98) = OUT_DIRECTION; //default in direction
            cmd_to_router.write(msg);
        }
        //get batch update info vertex update + edge_update
        HERE
        cmd_to_router.write({.data = -1, .last = 0});
        // clear_message_to_router.write({.data = 111});
        for (int i = 0 ; i < add_edge_num ; i++)
        {
            #pragma HLS PIPELINE II=1
            PMA_UPDATE_MESSAGE msg;
            // printf("INSERTING EDGE FROM %d TO %d\n", src[i], dst[i]);
            // msg.data.range(31,0) = src[i];
            // msg.data.range(63,32) = dst[i];
            msg.data.range(31,0) = dst[i];
            msg.data.range(63,32) = src[i];
            msg.data.range(97,96) = PMA_INSERT_EDGE;
            msg.data.range(98,98) = IN_DIRECTION; //default in direction
            cmd_to_router.write(msg);
        }
        HERE
        for (int i = 0 ; i < add_edge_num ; i++)
        {
            #pragma HLS PIPELINE II=1
            PMA_UPDATE_MESSAGE msg;
            msg.data.range(31,0) = src[i];
            msg.data.range(63,32) = dst[i];
            msg.data.range(97,96) = PMA_INSERT_EDGE;
            msg.data.range(98,98) = OUT_DIRECTION; //default in direction
            cmd_to_router.write(msg);
        }
        HERE
        // for (int i = 0 ; i < src_attr_change_num ; i++)
        // {
        //     PMA_UPDATE_MESSAGE msg;
        //     msg.data.range(31,0) = src_to_change[i];
        //     double attr_bits = attr_to_change[i];
        //     msg.data.range(95,32) = attr_bits;
        //     msg.data.range(97,96) = PMA_CHANGE_ATTR;
        //     msg.data.range(98,98) = IN_DIRECTION; //default in direction
        //     cmd_to_router.write(msg);
        // }
        // // HERE
        // for (int i = 0 ; i < src_attr_change_num ; i++)
        // {
        //     PMA_UPDATE_MESSAGE msg;
        //     msg.data.range(31,0) = src_to_change[i];
        //     double attr_bits = attr_to_change[i];
        //     msg.data.range(95,32) = attr_bits;
        //     msg.data.range(97,96) = PMA_CHANGE_ATTR;
        //     msg.data.range(98,98) = OUT_DIRECTION; //default in direction
        //     cmd_to_router.write(msg);
        // }
        // //send to pma_kernel
        HERE
        cmd_to_router.write({.data = -1, .last = 1});
    }
}