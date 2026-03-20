#include <hls_stream.h>
#include <ap_int.h>

#include "acc_data_types.h"
#include "acc_config.h"
#include "l1_api.h"

#ifndef LITTLE_KERNEL_DST_BUFFER_SIZE
#define LITTLE_KERNEL_DST_BUFFER_SIZE 65536
#endif

#ifndef BIG_KERNEL_DST_BUFFER_SIZE
#define BIG_KERNEL_DST_BUFFER_SIZE 524288
#endif

#ifndef SRC_BUFFER_SIZE
#define SRC_BUFFER_SIZE 4096
#endif

static inline bool dst_in_range(
    ap_uint<32> dst,
    ap_uint<32> base,
    ap_uint<32> size)
{
#pragma HLS INLINE
    // ap_uint<32> rel = dst - base;
    // return rel < size;
    return dst < base + size && dst >= base;
}

static inline void init_dummy_burst(edge_burst_dt &burst)
{
#pragma HLS INLINE
    for (int i = 0; i < NUM_EDGE_PER_BURST; ++i)
    {
#pragma HLS UNROLL
        burst.edges[i].src = 0;
        burst.edges[i].dst = (ap_uint<32>)ENDFLAG | (ap_uint<32>)0x80000000;
    }
}

static inline void emit_count(hls::stream<ap_axiu<32,0,0,0>> &count_stm, ap_uint<32> count)
{
#pragma HLS INLINE
    ap_axiu<32,0,0,0> pkt = {};
    pkt.data = count;
    pkt.keep = 0xF;
    pkt.strb = 0xF;
    pkt.last = 1;
    count_stm.write(pkt);
}

extern "C"
{
    static inline void pad_to_burst(
        edge_burst_dt &burst,
        hls::stream<edge_burst_dt> &out_stm,
        ap_uint<32> last_src,
        ap_uint<32> &fill,
        ap_uint<32> &edge_count,
        bool mark_src_msb)
    {
#pragma HLS INLINE
        while (fill != 0)
        {
            ap_uint<32> src = mark_src_msb ? (last_src | 0x80000000) : last_src;
            burst.edges[fill].src = src;
            burst.edges[fill].dst = (ap_uint<32>)ENDFLAG | (ap_uint<32>)0x80000000;
            fill++;
            edge_count++;
            if (fill == NUM_EDGE_PER_BURST)
            {
                write_to_stream(out_stm, burst);
                init_dummy_burst(burst);
                fill = 0;
            }
        }
    }

    void edgeReader(
        ap_uint<32> little_part_dst_offset,
        ap_uint<32> big_part_dst_offset,
        ap_uint<32> *coo_array,
        hls::stream<edge_burst_dt> &little_edge_burst_stm,
        hls::stream<edge_burst_dt> &big_edge_burst_stm,
        hls::stream<ap_axiu<32,0,0,0>> &little_edge_num_stm,
        hls::stream<ap_axiu<32,0,0,0>> &big_edge_num_stm
    )
    {
#pragma HLS INTERFACE m_axi port=coo_array offset=slave bundle=gmem0
#pragma HLS INTERFACE s_axilite port=coo_array bundle=control
#pragma HLS INTERFACE axis port=little_edge_burst_stm
#pragma HLS INTERFACE axis port=big_edge_burst_stm
#pragma HLS INTERFACE axis port=little_edge_num_stm
#pragma HLS INTERFACE axis port=big_edge_num_stm

#pragma HLS INTERFACE s_axilite port=little_part_dst_offset bundle=control
#pragma HLS INTERFACE s_axilite port=big_part_dst_offset bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

        const ap_uint<32> little_size = LITTLE_KERNEL_DST_BUFFER_SIZE;
        const ap_uint<32> big_size = BIG_KERNEL_DST_BUFFER_SIZE;

        edge_burst_dt little_burst;
        edge_burst_dt big_burst;
        init_dummy_burst(little_burst);
        init_dummy_burst(big_burst);

        ap_uint<32> little_edge_count = 0;
        ap_uint<32> big_edge_count = 0;
        ap_uint<32> little_fill = 0;
        ap_uint<32> big_fill = 0;
        ap_uint<32> little_last_src = 0;
        ap_uint<32> big_last_src = 0;
        bool little_has_edge = false;
        bool big_has_edge = false;

        ap_uint<32> idx = 0;
        while (true)
        {
#pragma HLS PIPELINE II=1
            ap_uint<32> src = coo_array[idx++];
            ap_uint<32> dst = coo_array[idx++];

                if (dst == (ap_uint<32>)ENDFLAG)
                {
                    break;
                }

                if (dst_in_range(dst, little_part_dst_offset, little_size))
                {
                    little_burst.edges[little_fill].src = src;
                    little_burst.edges[little_fill].dst = dst;
                    little_fill++;
                    little_edge_count++;
                    little_last_src = src;
                    little_has_edge = true;
                    if (little_fill == NUM_EDGE_PER_BURST)
                    {
                        write_to_stream(little_edge_burst_stm, little_burst);
                        init_dummy_burst(little_burst);
                        little_fill = 0;
                    }
                }
                else if (dst_in_range(dst, big_part_dst_offset, big_size))
                {
                    big_burst.edges[big_fill].src = src;
                    big_burst.edges[big_fill].dst = dst;
                    big_fill++;
                    big_edge_count++;
                    big_last_src = src;
                    big_has_edge = true;
                    if (big_fill == NUM_EDGE_PER_BURST)
                    {
                        write_to_stream(big_edge_burst_stm, big_burst);
                        init_dummy_burst(big_burst);
                        big_fill = 0;
                    }
                }
        }

        if (!little_has_edge)
        {
            for (int i = 0; i < NUM_EDGE_PER_BURST; ++i)
            {
#pragma HLS UNROLL
                little_burst.edges[i].src = 0x80000000;
                little_burst.edges[i].dst = (ap_uint<32>)ENDFLAG | (ap_uint<32>)0x80000000;
            }
            write_to_stream(little_edge_burst_stm, little_burst);
            little_edge_count = NUM_EDGE_PER_BURST;
        }
        else
        {
            pad_to_burst(little_burst, little_edge_burst_stm, little_last_src, little_fill, little_edge_count, true);
        }

        if (!big_has_edge)
        {
            for (int i = 0; i < NUM_EDGE_PER_BURST; ++i)
            {
#pragma HLS UNROLL
                big_burst.edges[i].src = 0x80000000;
                big_burst.edges[i].dst = (ap_uint<32>)ENDFLAG | (ap_uint<32>)0x80000000;
            }
            write_to_stream(big_edge_burst_stm, big_burst);
            big_edge_count = NUM_EDGE_PER_BURST;
        }
        else
        {
            pad_to_burst(big_burst, big_edge_burst_stm, big_last_src, big_fill, big_edge_count, true);
        }

        emit_count(little_edge_num_stm, little_edge_count);
        emit_count(big_edge_num_stm, big_edge_count);
    }
}