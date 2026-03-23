#include <hls_stream.h>
#include <ap_int.h>

#include "acc_data_types.h"
#include "acc_config.h"
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
        // ap_uint<32> big_part_dst_offset,
        // ap_uint<32> *coo_array,
        int last,
        hls::stream<edge_burst_dt> &little_edge_burst_stm,
        // hls::stream<edge_burst_dt> &big_edge_burst_stm,
        hls::stream<ap_axiu<32,0,0,0>> &little_edge_num_stm,
        // hls::stream<ap_axiu<32,0,0,0>> &big_edge_num_stm,

        hls::stream<READ_COMMAND_STREAM> &little_cmd_stm,
        hls::stream<ap_axiu<32,0,0,0>> &little_coo_edge_num_stm,
        hls::stream<ap_axiu<64,0,0,0>> &coo_array_stm
        // hls::stream<READ_COMMAND_STREAM> &big_cmd_stm
    )
    {
<<<<<<< HEAD:ReGraph/acc_template/edge_reader/edge_reader.cpp
    // #pragma HLS INTERFACE m_axi port=coo_array offset=slave bundle=gmem0
    // #pragma HLS INTERFACE s_axilite port=coo_array bundle=control
    #pragma HLS INTERFACE axis port=little_edge_burst_stm
    #pragma HLS INTERFACE s_axilite port=last bundle=control
    #pragma HLS INTERFACE axis port=big_edge_burst_stm
    #pragma HLS INTERFACE axis port=little_edge_num_stm
    #pragma HLS INTERFACE axis port=big_edge_num_stm
    #pragma HLS INTERFACE axis port=little_coo_edge_num_stm
    #pragma HLS INTERFACE axis port=coo_array_stm

    #pragma HLS INTERFACE s_axilite port=little_part_dst_offset bundle=control
    #pragma HLS INTERFACE s_axilite port=big_part_dst_offset bundle=control
    #pragma HLS INTERFACE s_axilite port=return bundle=control
    #pragma HLS INTERFACE axis port=little_cmd_stm
=======
// #pragma HLS INTERFACE m_axi port=coo_array offset=slave bundle=gmem0
// #pragma HLS INTERFACE s_axilite port=coo_array bundle=control
#pragma HLS INTERFACE axis port=little_edge_burst_stm
// #pragma HLS INTERFACE axis port=big_edge_burst_stm
#pragma HLS INTERFACE axis port=little_edge_num_stm
// #pragma HLS INTERFACE axis port=big_edge_num_stm
#pragma HLS INTERFACE axis port=little_coo_edge_num_stm
#pragma HLS INTERFACE axis port=coo_array_stm
#pragma HLS INTERFACE axis port=little_cmd_stm

#pragma HLS INTERFACE s_axilite port=last bundle=control
#pragma HLS INTERFACE s_axilite port=little_part_dst_offset bundle=control
// #pragma HLS INTERFACE s_axilite port=big_part_dst_offset bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control
>>>>>>> 3aebe2e (to speed up):ReGraph/acc_template/edge_reader/edgeReader.cpp
    // #pragma HLS INTERFACE axis port=big_cmd_stm

        const ap_uint<32> little_size = LITTLE_KERNEL_DST_BUFFER_SIZE;
        const ap_uint<32> big_size = BIG_KERNEL_DST_BUFFER_SIZE;

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

<<<<<<< HEAD:ReGraph/acc_template/edge_reader/edge_reader.cpp
        // READ_COMMAND_STREAM cmd;
        // cmd.data = PMA_GET_ALL_OUT_EDGE_COO;
        // cmd.last = 0;
        // little_cmd_stm.write(cmd);

/*
        // 第一步：先请求边数目
        READ_COMMAND_STREAM count_cmd = {};
        count_cmd.data.range(31, 0)  = PMA_GET_EDGE_COUNT_RANGE;
        count_cmd.data.range(63, 32) = 0;           // start_v = 0
        count_cmd.data.range(95, 64) = 0xFFFFFFFF;  // end_v = 全量
        count_cmd.last = 0;
        little_cmd_stm.write(count_cmd);

        // 等 pma_read_kernel 把边总数写到 coo_edge_count_stream
        auto little_edge_num = little_coo_edge_num_stm.read();
        little_edge_num_stm.write(little_edge_num);  // 转发给 little GS kernel

        // 第二步：再请求 COO 边数据
        READ_COMMAND_STREAM coo_cmd = {};
        coo_cmd.data = PMA_GET_ALL_OUT_EDGE_COO;
        coo_cmd.last = 0;
        little_cmd_stm.write(coo_cmd);

        //little_edge_num = little_coo_edge_num_stm.read(); 
        little_edge_num_stm.write(little_coo_edge_num_stm.read());

        ap_uint<32> idx = 0;
        while (true)
=======
        READ_COMMAND_STREAM cmd;
        cmd.data = PMA_GET_EDGE_COUNT_RANGE;
        cmd.last = 0;
        little_cmd_stm.write(cmd);
        #pragma HLS protocol fiexed
        ap_wait();
        auto little_edge_num = little_coo_edge_num_stm.read();
        ap_wait();
        // compute aligned count (NUM_EDGE_PER_BURST alignment) and emit before bursts
        ap_uint<32> little_num = little_edge_num.data;
        // mark whether we will produce any little bursts
        little_has_edge = (little_num != 0);
        // ap_uint<32> big_num = 0;
        // big_has_edge = big_num != 0;
        ap_uint<32> aligned_num = ((little_num + (NUM_EDGE_PER_BURST - 1)) / NUM_EDGE_PER_BURST) * NUM_EDGE_PER_BURST;
        emit_count(little_edge_num_stm, aligned_num);
        // big unused for small graph runs
        // emit_count(big_edge_num_stm, 0);
        
        // READ_COMMAND_STREAM cmd;
        cmd.data = PMA_GET_ALL_OUT_EDGE_COO;
        cmd.last = 0;
        little_cmd_stm.write(cmd);
        
        // For small-graph flow: upstream gives the number of little edges.
        // Read exactly that many COOs and pack them into little bursts.
        int last_src = -1;
        for (ap_uint<32> i = 0; i < little_num; ++i)
>>>>>>> 3aebe2e (to speed up):ReGraph/acc_template/edge_reader/edgeReader.cpp
        {
#pragma HLS PIPELINE II=1
            auto single_edge = coo_array_stm.read();
            ap_uint<64> single_edge_data = single_edge.data;
            int src = single_edge_data.range(63,32);
            int dst = single_edge_data.range(31,0);

            little_burst.edges[little_fill].src = src;
            little_burst.edges[little_fill].dst = dst;
            last_src = src;
            little_fill++;
            little_edge_count++;
            little_last_src = src;
            if (little_fill == NUM_EDGE_PER_BURST)
            {
                write_to_stream(little_edge_burst_stm, little_burst);
                init_dummy_burst(little_burst);
                little_fill = 0;
            }
        }


        if (little_num == 0)
        {
            // no edges: emit a single dummy burst
            for (int i = 0; i < NUM_EDGE_PER_BURST; ++i)
            {
#pragma HLS UNROLL
                little_burst.edges[i].src = last_src;
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

//         if (big_num == 0)
//         {
//             for (int i = 0; i < NUM_EDGE_PER_BURST; ++i)
//             {
// #pragma HLS UNROLL
//                 big_burst.edges[i].src = 0x00000000;
//                 big_burst.edges[i].dst = (ap_uint<32>)ENDFLAG | (ap_uint<32>)0x80000000;
//             }
//             write_to_stream(big_edge_burst_stm, big_burst);
//             big_edge_count = NUM_EDGE_PER_BURST;
//         }
//         else if (0)
//         {
//             pad_to_burst(big_burst, big_edge_burst_stm, big_last_src, big_fill, big_edge_count, true);
//         }

        // emit_count(little_edge_num_stm, little_edge_count);
        emit_count(big_edge_num_stm, big_edge_count);
*/

        // 直接请求 COO 边数据
        READ_COMMAND_STREAM coo_cmd = {};
        coo_cmd.data = PMA_GET_ALL_OUT_EDGE_COO;
        coo_cmd.last = 0;
        little_cmd_stm.write(coo_cmd);

        while (true)
        {
#pragma HLS PIPELINE II=1
            auto single_edge = coo_array_stm.read();

            if (single_edge.last == 1)
            {
                break;
            }

            ap_uint<64> single_edge_data = single_edge.data;
            int src = single_edge_data.range(63,32);
            int dst = single_edge_data.range(31,0);

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

        // send stop commands to reader kernels
        READ_COMMAND_STREAM stop_pkt = {};
        stop_pkt.data = (ap_uint<64>)PMA_STOP;
        stop_pkt.keep = (ap_uint<8>)0xFF;
        stop_pkt.strb = (ap_uint<8>)0xFF;
        stop_pkt.last = last;
        little_cmd_stm.write(stop_pkt);
    }
}