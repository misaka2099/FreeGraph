#include <hls_stream.h>
#include <ap_int.h>
#include <ap_axi_sdata.h>

#include "acc_data_types.h"
#include "acc_config.h"
#include "etc/autopilot_ssdm_op.h"
#include "l1_api.h"
#include "data_type.h"

#ifndef SRC_BUFFER_SIZE
#define SRC_BUFFER_SIZE 4096
#endif

// Read pipeline worker count — must match pma_read_worker_kernel instances in
// regraph_read_system.cfg (CHANNEL_NUM from data_type.h).
static const int READ_WORKERS = CHANNEL_NUM;

// Same packet type as pma_read_worker_kernel's pkt_to_merger port.
typedef ap_axiu<64, 8, 0, 0> READ_PKT_STREAM;

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

// edgeReader drives the PMA read pipeline directly (no read router, no read
// merger): it broadcasts commands to all READ_WORKERS and merges their packet
// streams itself, converting the COO stream into ReGraph edge bursts.
//
// Merging strategy — per-PMA round robin:
//   Each worker streams its own PMAs in order (pma_idx = worker_id, +READ_WORKERS),
//   delimited by per-PMA end markers (keep!=0, last==1) and terminated by a
//   done marker (keep==0, last==1). Draining one PMA per worker in round-robin
//   order yields a globally src-sorted edge stream, which the ReGraph GS
//   kernel's src-property ping-pong prefetch relies on.
//
// Edge count protocol:
//   Phase 1 requests PMA_GET_EDGE_COUNT_RANGE per SRC_BUFFER_SIZE chunk, sums
//   the workers' partial counts, and computes the exact padding needed to keep
//   every 8-edge burst within one src buffer. The count sent to the GS kernel
//   (total + padding + one dummy burst) is emitted as exactly that many bursts
//   in phase 2, so the GS kernel never blocks or over-reads.
extern "C"
{
    void edgeReader(
        ap_uint<64> *edge_count_out,
        int num_vertices,
        hls::stream<edge_burst_dt> &little_edge_burst_stm,
        hls::stream<ap_axiu<32,0,0,0>> &little_edge_num_stm,
        hls::stream<READ_COMMAND_STREAM> &worker_cmd_0,
        hls::stream<READ_COMMAND_STREAM> &worker_cmd_1,
        hls::stream<READ_COMMAND_STREAM> &worker_cmd_2,
        hls::stream<READ_PKT_STREAM> &worker_pkt_0,
        hls::stream<READ_PKT_STREAM> &worker_pkt_1,
        hls::stream<READ_PKT_STREAM> &worker_pkt_2
    )
    {
    #pragma HLS INTERFACE m_axi port=edge_count_out offset=slave bundle=gmem0
    #pragma HLS INTERFACE s_axilite port=edge_count_out bundle=control
    #pragma HLS INTERFACE s_axilite port=num_vertices bundle=control
    #pragma HLS INTERFACE s_axilite port=return bundle=control
    #pragma HLS INTERFACE axis port=little_edge_burst_stm
    #pragma HLS INTERFACE axis port=little_edge_num_stm
    #pragma HLS INTERFACE axis port=worker_cmd_0
    #pragma HLS INTERFACE axis port=worker_cmd_1
    #pragma HLS INTERFACE axis port=worker_cmd_2
    #pragma HLS INTERFACE axis port=worker_pkt_0
    #pragma HLS INTERFACE axis port=worker_pkt_1
    #pragma HLS INTERFACE axis port=worker_pkt_2

    #pragma HLS STREAM variable=worker_cmd_0 depth=16
    #pragma HLS STREAM variable=worker_cmd_1 depth=16
    #pragma HLS STREAM variable=worker_cmd_2 depth=16
    #pragma HLS STREAM variable=worker_pkt_0 depth=128
    #pragma HLS STREAM variable=worker_pkt_1 depth=128
    #pragma HLS STREAM variable=worker_pkt_2 depth=128
    #pragma HLS STREAM variable=little_edge_burst_stm depth=16
    #pragma HLS STREAM variable=little_edge_num_stm depth=4

        // ---------------- Phase 1: edge counting + padding budget ----------------
        // Request the edge count per SRC_BUFFER_SIZE chunk so that the padding
        // needed at src-buffer boundaries can be accounted for exactly.
        int num_buffers = (num_vertices + SRC_BUFFER_SIZE - 1) / SRC_BUFFER_SIZE;
        ap_uint<32> total_edge_count = 0;
        ap_uint<32> padding_sum = 0;

        for (int b = 0; b < num_buffers; b++)
        {
            int chunk_start = b * SRC_BUFFER_SIZE;
            int chunk_end = chunk_start + SRC_BUFFER_SIZE;
            if (chunk_end > num_vertices) chunk_end = num_vertices;

            READ_COMMAND_STREAM count_cmd = {};
            count_cmd.data.range(31, 0)  = PMA_GET_EDGE_COUNT_RANGE;
            count_cmd.data.range(63, 32) = chunk_start;   // src_start
            count_cmd.data.range(95, 64) = chunk_end;     // src_end
            count_cmd.data.range(127, 96)  = (ap_uint<32>)-1;  // dst_start: 不限
            count_cmd.data.range(159, 128) = (ap_uint<32>)-1;  // dst_end: 不限
            count_cmd.last = 0;
            count_cmd.keep = 0xFF;
            count_cmd.strb = 0xFF;
            worker_cmd_0.write(count_cmd);
            worker_cmd_1.write(count_cmd);
            worker_cmd_2.write(count_cmd);
            #pragma HLS protocol fixed
            ap_wait();

            // Each worker answers with exactly one count packet (partial sum)
            // followed by one done marker.
            ap_uint<32> chunk_count = 0;
            READ_PKT_STREAM count_pkt_0 = worker_pkt_0.read();
            chunk_count += count_pkt_0.data.range(31, 0);
            ap_wait();
            READ_PKT_STREAM done_pkt_0 = worker_pkt_0.read();
            (void)done_pkt_0;
            ap_wait();
            READ_PKT_STREAM count_pkt_1 = worker_pkt_1.read();
            chunk_count += count_pkt_1.data.range(31, 0);
            ap_wait();
            READ_PKT_STREAM done_pkt_1 = worker_pkt_1.read();
            (void)done_pkt_1;
            ap_wait();
            READ_PKT_STREAM count_pkt_2 = worker_pkt_2.read();
            chunk_count += count_pkt_2.data.range(31, 0);
            ap_wait();
            READ_PKT_STREAM done_pkt_2 = worker_pkt_2.read();
            (void)done_pkt_2;

            total_edge_count += chunk_count;
            ap_uint<32> chunk_remainder = chunk_count % 8;
            if (chunk_remainder != 0) padding_sum += (8 - chunk_remainder);
        }

        // count_to_send: real edges + boundary/final padding + one extra dummy
        // burst (keeps the GS kernel's burst counter exact in all cases).
        ap_uint<32> count_to_send = total_edge_count + padding_sum + 8;

        // Forward the (8-aligned) count to the GS kernel before any burst.
        ap_axiu<32,0,0,0> edge_num_pkt = {};
        edge_num_pkt.data = count_to_send;
        edge_num_pkt.keep = 0xF;
        edge_num_pkt.strb = 0xF;
        edge_num_pkt.last = 1;
        little_edge_num_stm.write(edge_num_pkt);

        // Report counts to the host (verification + performance accounting).
        edge_count_out[0] = (ap_uint<64>)total_edge_count;
        edge_count_out[1] = (ap_uint<64>)count_to_send;

        // ---------------- Phase 2: stream all edges as COO ----------------
        // 全图遍历: src/dst 范围都是 -1 (160 位命令格式)
        READ_COMMAND_STREAM coo_cmd = {};
        coo_cmd.data.range(31, 0)  = PMA_GET_ALL_OUT_EDGE_COO;
        coo_cmd.data.range(63, 32) = (ap_uint<32>)-1;   // src_start: 不限
        coo_cmd.data.range(95, 64) = (ap_uint<32>)-1;   // src_end: 不限
        coo_cmd.data.range(127, 96)  = (ap_uint<32>)-1; // dst_start: 不限
        coo_cmd.data.range(159, 128) = (ap_uint<32>)-1; // dst_end: 不限
        coo_cmd.last = 0;
        coo_cmd.keep = 0xFF;
        coo_cmd.strb = 0xFF;
        worker_cmd_0.write(coo_cmd);
        worker_cmd_1.write(coo_cmd);
        worker_cmd_2.write(coo_cmd);
        #pragma HLS protocol fixed
        ap_wait();

        bool worker_done[READ_WORKERS];
        for (int w = 0; w < READ_WORKERS; w++)
        {
    #pragma HLS UNROLL
            worker_done[w] = false;
        }
        int done_count = 0;
        int worker_idx = 0;

        edge_burst_dt burst;
        init_dummy_burst(burst);
        int fill_count = 0;
        ap_uint<32> emitted_count = 0;
        ap_uint<32> prev_src_buffer = (ap_uint<32>)0xFFFFFFFF;
        ap_uint<32> last_real_src = 0;

        // Per-PMA round-robin merge of the three worker streams.
        // 注意: 循环边界 (done_count) 是运行时的, 不能加 #pragma HLS PIPELINE II=1 —
        // HLS 会把 pipeline 解释成 complete unroll, 可变边界无法展开 → II violation
        // (worker_idx 存储/加载循环依赖 + %3 的 36-cycle srem), 综合出的硬件退出不可靠。
        while (done_count < READ_WORKERS)
        {
            if (worker_done[worker_idx])
            {
                worker_idx = (worker_idx + 1) % READ_WORKERS;
                continue;
            }

            READ_PKT_STREAM pkt;
            if      (worker_idx == 0) pkt = worker_pkt_0.read();
            else if (worker_idx == 1) pkt = worker_pkt_1.read();
            else                      pkt = worker_pkt_2.read();

            if (pkt.keep == 0 && pkt.last == 1)
            {
                // Done marker: this worker has no more PMAs.
                worker_done[worker_idx] = true;
                done_count++;
                worker_idx = (worker_idx + 1) % READ_WORKERS;
                continue;
            }
            if (pkt.keep != 0 && pkt.last == 1)
            {
                // Per-PMA end marker: move to the next worker's next PMA.
                worker_idx = (worker_idx + 1) % READ_WORKERS;
                continue;
            }

            // Data packet: (src << 32) | dst
            ap_uint<32> src = pkt.data.range(63, 32);
            ap_uint<32> dst = pkt.data.range(31, 0);
            ap_uint<32> current_src_buffer = src / SRC_BUFFER_SIZE;

            // Pad the in-flight burst to 8 at a src-buffer boundary so no burst
            // straddles two buffers (the GS scatter reads props per burst head).
            if (current_src_buffer != prev_src_buffer && fill_count != 0)
            {
                int pad_num = 8 - fill_count;
                for (int p = 0; p < 8; p++)
                {
    #pragma HLS UNROLL
                    if (p < pad_num)
                    {
                        burst.edges[fill_count + p].src = last_real_src | 0x80000000;
                        burst.edges[fill_count + p].dst =
                            (ap_uint<32>)ENDFLAG | (ap_uint<32>)0x80000000;
                    }
                }
                little_edge_burst_stm.write(burst);
                init_dummy_burst(burst);
                emitted_count += pad_num;
                fill_count = 0;
            }

            burst.edges[fill_count].src = src;
            burst.edges[fill_count].dst = dst;
            last_real_src = src;
            prev_src_buffer = current_src_buffer;
            fill_count++;
            emitted_count++;

            if (fill_count == NUM_EDGE_PER_BURST)
            {
                little_edge_burst_stm.write(burst);
                init_dummy_burst(burst);
                fill_count = 0;
            }
        }

        // Final pad to the burst boundary, then dummy edges up to count_to_send.
        if (fill_count != 0)
        {
            int pad_num = 8 - fill_count;
            for (int p = 0; p < 8; p++)
            {
    #pragma HLS UNROLL
                if (p < pad_num)
                {
                    burst.edges[fill_count + p].src = last_real_src | 0x80000000;
                    burst.edges[fill_count + p].dst =
                        (ap_uint<32>)ENDFLAG | (ap_uint<32>)0x80000000;
                }
            }
            little_edge_burst_stm.write(burst);
            init_dummy_burst(burst);
            emitted_count += pad_num;
            fill_count = 0;
        }
        while (emitted_count < count_to_send)
        {
            burst.edges[fill_count].src = last_real_src | 0x80000000;
            burst.edges[fill_count].dst = (ap_uint<32>)ENDFLAG | (ap_uint<32>)0x80000000;
            fill_count++;
            emitted_count++;
            if (fill_count == NUM_EDGE_PER_BURST)
            {
                little_edge_burst_stm.write(burst);
                init_dummy_burst(burst);
                fill_count = 0;
            }
        }

        // Terminate the read pipeline: workers exit on PMA_STOP with last=1.
        READ_COMMAND_STREAM stop_cmd = {};
        stop_cmd.data = (ap_uint<160>)PMA_STOP;
        stop_cmd.last = 1;
        stop_cmd.keep = 0xFF;
        stop_cmd.strb = 0xFF;
        worker_cmd_0.write(stop_cmd);
        worker_cmd_1.write(stop_cmd);
        worker_cmd_2.write(stop_cmd);
        #pragma HLS protocol fixed
        ap_wait();
    }
}
