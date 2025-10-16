#include "data_type.h"
#include <hls_stream.h>
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
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_router,
        hls::stream<CLEAR_ICON> &clear_message_to_router
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
        #pragma HLS INTERFACE axis port=clear_message_to_router
        static int total_vertex_num_in = 0;
        //(66,66) for direction (65,64) for command type (63,32) for dst (31,0) for src
        static int total_vertex_num_out = 0;
        for (int i = 0 ; i < add_vertex_num ; i++)
        {
            PMA_UPDATE_MESSAGE msg;
            msg.data.range(31,0) = total_vertex_num_in++;
            msg.data.range(63,32) = 0;
            msg.data.range(97,96) = PMA_INSERT_VERTEX;
            msg.data.range(98,98) = IN_DIRECTION; //default in direction
        }
        for (int i = 0 ; i < add_vertex_num ; i++)
        {
            PMA_UPDATE_MESSAGE msg;
            msg.data.range(31,0) = total_vertex_num_out++;
            msg.data.range(63,32) = 0;
            msg.data.range(97,96) = PMA_INSERT_VERTEX;
            msg.data.range(98,98) = OUT_DIRECTION; //default in direction
        }
        //get batch update info vertex update + edge_update
        for (int i = 0 ; i < add_edge_num ; i++)
        {
            PMA_UPDATE_MESSAGE msg;
            msg.data.range(31,0) = src[i];
            msg.data.range(63,32) = dst[i];
            msg.data.range(97,96) = PMA_INSERT_EDGE;
            msg.data.range(98,98) = IN_DIRECTION; //default in direction
            cmd_to_router.write(msg);
        }
        for (int i = 0 ; i < add_edge_num ; i++)
        {
            PMA_UPDATE_MESSAGE msg;
            msg.data.range(31,0) = src[i];
            msg.data.range(63,32) = dst[i];
            msg.data.range(97,96) = PMA_INSERT_EDGE;
            msg.data.range(98,98) = OUT_DIRECTION; //default in direction
            cmd_to_router.write(msg);
        }
        for (int i = 0 ; i < src_attr_change_num ; i++)
        {
            PMA_UPDATE_MESSAGE msg;
            msg.data.range(31,0) = src_to_change[i];
            double attr_bits = attr_to_change[i];
            msg.data.range(95,32) = attr_bits;
            msg.data.range(97,96) = PMA_CHANGE_ATTR;
            msg.data.range(98,98) = IN_DIRECTION; //default in direction
            cmd_to_router.write(msg);
        }
        for (int i = 0 ; i < src_attr_change_num ; i++)
        {
            PMA_UPDATE_MESSAGE msg;
            msg.data.range(31,0) = src_to_change[i];
            double attr_bits = attr_to_change[i];
            msg.data.range(95,32) = attr_bits;
            msg.data.range(97,96) = PMA_CHANGE_ATTR;
            msg.data.range(98,98) = OUT_DIRECTION; //default in direction
            cmd_to_router.write(msg);
        }
        //send to pma_kernel
        clear_message_to_router.write({.data = 111});
    }
}