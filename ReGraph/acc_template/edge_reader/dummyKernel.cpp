#include <hls_stream.h>
#include <ap_int.h>

#include "acc_data_types.h"
#include "acc_config.h"
#include "ap_axi_sdata.h"
#include "etc/autopilot_ssdm_op.h"
#include "l1_api.h"
#include "data_type.h"

#ifndef LITTLE_KERNEL_DST_BUFFER_SIZE
#define LITTLE_KERNEL_DST_BUFFER_SIZE 65536
#endif

#ifndef BIG_KERNEL_DST_BUFFER_SIZE
#define BIG_KERNEL_DST_BUFFER_SIZE 524288
#endif

#ifndef SRC_BUFFER_SIZE
#define SRC_BUFFER_SIZE 4096
#endif


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

    void dummyKernel(
        ap_uint<32> little_part_dst_offset,
        // ap_uint<32> big_part_dst_offset,
        // ap_uint<32> *coo_array,
        hls::stream<edge_burst_dt> &little_edge_burst_stm,
        // hls::stream<edge_burst_dt> &big_edge_burst_stm,
        hls::stream<ap_axiu<32,0,0,0>> &little_edge_num_stm
        // hls::stream<READ_COMMAND_STREAM> &big_cmd_stm
    )
    {
    // #pragma HLS INTERFACE m_axi port=coo_array offset=slave bundle=gmem0
    // #pragma HLS INTERFACE s_axilite port=coo_array bundle=control
    #pragma HLS INTERFACE axis port=little_edge_burst_stm
    // #pragma HLS INTERFACE axis port=big_edge_burst_stm
    #pragma HLS INTERFACE axis port=little_edge_num_stm
    // #pragma HLS INTERFACE axis port=big_edge_num_stm

    #pragma HLS INTERFACE s_axilite port=little_part_dst_offset bundle=control
    // #pragma HLS INTERFACE s_axilite port=big_part_dst_offset bundle=control
    #pragma HLS INTERFACE s_axilite port=return bundle=control
    // #pragma HLS INTERFACE axis port=big_cmd_stm

        const ap_uint<32> little_size = LITTLE_KERNEL_DST_BUFFER_SIZE;
        const ap_uint<32> big_size = BIG_KERNEL_DST_BUFFER_SIZE;
        ap_uint<32> prev_src_buffer = (ap_uint<32>)0xFFFFFFFF;

        edge_burst_dt little_burst;
        edge_burst_dt big_burst;
        init_dummy_burst(little_burst);
        init_dummy_burst(big_burst);
        ap_wait();
        ap_uint<32> little_edge_count = 0;
        ap_uint<32> big_edge_count = 0;
        ap_uint<32> little_fill = 0;
        ap_uint<32> big_fill = 0;
        ap_uint<32> little_last_src = 0;
        ap_uint<32> big_last_src = 0;
        ap_uint<32> little_prev_src = (ap_uint<32>)0xFFFFFFFF;
        ap_uint<32> big_prev_src = (ap_uint<32>)0xFFFFFFFF;
        bool little_has_edge = false;
        bool big_has_edge = false;

        // READ_COMMAND_STREAM cmd;
        // cmd.data = PMA_GET_ALL_OUT_EDGE_COO;
        // cmd.last = 0;
        // little_cmd_stm.write(cmd);


        // 第一步：先请求边数目
         // 等 pma_read_kernel 把边总数写到 coo_edge_count_stream
        ap_axiu<32,0,0,0> little_edge_num = {10};

        // 向上8对齐边数量后再转发给 little GS kernel
        ap_axiu<32,0,0,0> aligned_edge_num = little_edge_num;
        ap_uint<32> cnt = aligned_edge_num.data;
        // cnt = (cnt + (ap_uint<32>)7) & ~((ap_uint<32>)7);
        cnt = (cnt / 8 + 1) * 8;
        aligned_edge_num.data = 24;
        
        // 第二步：再请求 COO 边数据
        READ_COMMAND_STREAM coo_cmd = {};
        coo_cmd.data = PMA_GET_ALL_OUT_EDGE_COO;
        coo_cmd.last = 0;
        ap_uint<32> idx = 0;
        int last_src = 0;
        ap_axiu<64, 0, 0, 0> coo_array_stm[] = {
            (ap_axiu<64,0,0,0>){(ap_uint<64>(0) << 32) | ap_uint<64>(1)},
            (ap_axiu<64,0,0,0>){(ap_uint<64>(1) << 32) | ap_uint<64>(2)},
            (ap_axiu<64,0,0,0>){(ap_uint<64>(1) << 32) | ap_uint<64>(3)},
            (ap_axiu<64,0,0,0>){(ap_uint<64>(2) << 32) | ap_uint<64>(4)},
            (ap_axiu<64,0,0,0>){(ap_uint<64>(3) << 32) | ap_uint<64>(2)},
            (ap_axiu<64,0,0,0>){(ap_uint<64>(3) << 32) | ap_uint<64>(4)},
            (ap_axiu<64,0,0,0>){(ap_uint<64>(4) << 32) | ap_uint<64>(9)},
            (ap_axiu<64,0,0,0>){(ap_uint<64>(9) << 32) | ap_uint<64>(10)},
            (ap_axiu<64,0,0,0>){(ap_uint<64>(9) << 32) | ap_uint<64>(5000)},
            (ap_axiu<64,0,0,0>){(ap_uint<64>(9) << 5000) | ap_uint<64>(5001)}

        };
        for (int i = 0 ; i < little_edge_num.data; i++)
        {
        #pragma HLS PIPELINE II=1

            auto single_edge = coo_array_stm[i];
            ap_uint<64> single_edge_data = single_edge.data;

            ap_uint<32> src = single_edge_data.range(63,32);
            ap_uint<32> dst = single_edge_data.range(31,0);

            ap_uint<32> current_src_buffer = src / SRC_BUFFER_SIZE;

            if (current_src_buffer != prev_src_buffer && little_fill != 0)
            {
                // padding 到 8（不加 mask！和 host 一样）
                while (little_fill != 0)
                {
                    little_burst.edges[little_fill].src = little_last_src;
                    little_burst.edges[little_fill].dst =
                        (ap_uint<32>)ENDFLAG | (ap_uint<32>)0x80000000;

                    little_fill++;
                    little_edge_count++;

                    if (little_fill == NUM_EDGE_PER_BURST)
                    {
                        write_to_stream(little_edge_burst_stm, little_burst);
                        init_dummy_burst(little_burst);
                        little_fill = 0;
                    }
                }
            }

            // 正常写 edge
            little_burst.edges[little_fill].src = src;
            little_burst.edges[little_fill].dst = dst;

            little_last_src = src;
            prev_src_buffer = current_src_buffer;

            little_fill++;
            little_edge_count++;

            if (little_fill == NUM_EDGE_PER_BURST)
            {
                write_to_stream(little_edge_burst_stm, little_burst);
                init_dummy_burst(little_burst);
                little_fill = 0;
            }
        }

        if (little_edge_num.data % 8 == 0)
        {
            // no edges: emit a single dummy burst
            for (int i = 0; i < NUM_EDGE_PER_BURST; ++i)
            {
#pragma HLS UNROLL
                little_burst.edges[i].src = last_src | 0x80000000;
                little_burst.edges[i].dst = (ap_uint<32>)ENDFLAG | (ap_uint<32>)0x80000000;
            }
            write_to_stream(little_edge_burst_stm, little_burst);
            little_edge_count = NUM_EDGE_PER_BURST;
        }
        else
        {
            // there were edges: pad the last partial burst (if any) to NUM_EDGE_PER_BURST
            pad_to_burst(little_burst, little_edge_burst_stm, little_last_src, little_fill, little_edge_count, true);
        }
        little_edge_num_stm.write(aligned_edge_num);  // 转发给 little GS kernel（已向上8对齐）

    }
}