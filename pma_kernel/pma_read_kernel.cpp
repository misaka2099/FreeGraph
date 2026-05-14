#include "data_type.h"
#include "common_function.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <hls_stream.h>
#include <ap_int.h>
#include "debug.h"
#include "etc/autopilot_ssdm_op.h"
#include "hls_burst_maxi.h"
#include "hls_print.h"

#ifndef READ_WORKERS
#define READ_WORKERS CHANNEL_NUM
#endif

struct READ_CMD {
    int src_idx;
    int end_src_idx;
    int mode;
    int last;
};

struct READ_PKT {
    ap_uint<64> data;
    ap_uint<8>  user;
    ap_uint<1>  last;
    ap_uint<8>  keep;
};

// =========================================================================
// distributor
// =========================================================================
static void distributor(
    hls::stream<READ_COMMAND_STREAM>& cmd_in,
    hls::stream<READ_CMD>& worker_0_cmd,
    hls::stream<READ_CMD>& worker_1_cmd,
    hls::stream<READ_CMD>& merger_cmd)
{
    #pragma HLS INLINE off
    while (true) {
        READ_COMMAND_STREAM cmd = cmd_in.read();
        if (cmd.data == PMA_STOP && cmd.last == 1) {
            READ_CMD stop = {};
            stop.last = 1;
            worker_0_cmd.write(stop);
            worker_1_cmd.write(stop);
            merger_cmd.write(stop);
            break;
        }

        int mode = (int)cmd.data.range(31, 0);

        if (mode == PMA_GET_OUT_EDGE) {
            int src = (int)cmd.data.range(63, 32);
            READ_CMD rc = {};
            rc.src_idx = src;
            rc.mode = mode;
            worker_0_cmd.write(rc);
            worker_1_cmd.write(rc);
            merger_cmd.write(rc);
        } else if (mode == PMA_GET_ALL_OUT_EDGE_COO || mode == PMA_GET_ALL_OUT_EDGE_CSR) {
            READ_CMD rc = {};
            rc.mode = mode;
            worker_0_cmd.write(rc);
            worker_1_cmd.write(rc);
            merger_cmd.write(rc);
        } else if (mode == PMA_GET_EDGE_COUNT_RANGE) {
            READ_CMD rc = {};
            rc.src_idx     = (int)cmd.data.range(63, 32);
            rc.end_src_idx = (int)cmd.data.range(95, 64);
            rc.mode = mode;
            worker_0_cmd.write(rc);
            worker_1_cmd.write(rc);
            merger_cmd.write(rc);
        }
    }
}

// =========================================================================
// write_pkt helper
// =========================================================================
static void write_pkt(
    hls::stream<READ_PKT>& out,
    ap_uint<64> data, ap_uint<8> user,
    ap_uint<1> last, ap_uint<8> keep)
{
    #pragma HLS INLINE
    READ_PKT pkt = {};
    pkt.data = data;
    pkt.user = user;
    pkt.last = last;
    pkt.keep = keep;
    out.write(pkt);
}

// =========================================================================
// read_worker: direct MAXI access.  16-int segments, 2-word headers.
//   max_pma_idx from vec_info_base[63:0]; vecinfo table at base+64.
// =========================================================================
template<int WORKER_ID>
static void read_worker(
    hls::stream<READ_CMD>& cmd_stream,
    hls::stream<READ_PKT>& pkt_out,
    ap_uint<512>* hbm_ptr,
    int vec_info_base)
{
    #pragma HLS INLINE off
    printf("READ_WORKER[%d] START\n", WORKER_ID);
    const ap_uint<8> KEEP64 = 0xFF;
    int vi_table = vec_info_base + 64;

    while (true) {
        READ_CMD cmd = cmd_stream.read();
        if (cmd.last) break;

        if (cmd.mode == PMA_GET_OUT_EDGE) {
            int src = cmd.src_idx;
            int pma_idx = src / VERTEX_PER_PMA;
            if (pma_idx % READ_WORKERS != WORKER_ID) {
                write_pkt(pkt_out, 0, 0, 1, 0);
                continue;
            }

            int vi_word = vi_table / 64 + pma_idx / 8;
            int vi_shift = (pma_idx % 8) * 64;
            ap_uint<512> vi_pkt = hbm_ptr[vi_word];
            VEC_INFO vi = vi_pkt.range(vi_shift + 63, vi_shift);

            if (vi == 0) {
                write_pkt(pkt_out, 0, 0xFF, 1, KEEP64);
                write_pkt(pkt_out, 0, 0, 1, 0);
                continue;
            }

            int pma_start = (int)vi.range(31, 0);
            int hdr_word = pma_start / 64;

            ap_uint<512> hdr[HEADER_WORDS];
            for (int j = 0; j < HEADER_WORDS; j++)
                hdr[j] = hbm_ptr[hdr_word + j];

            int local_src = src & 0xF;
            int seg_start = (local_src == 0) ? 0
                : (int)hdr[0].range(32 * local_src - 1, 32 * (local_src - 1));
            int seg_end = (int)hdr[0].range(32 * (local_src + 1) - 1, 32 * local_src);

            for (int s = seg_start; s < seg_end; s++) {
                ap_uint<512> seg = hbm_ptr[hdr_word + HEADER_WORDS + s];
                for (int k = 0; k < SEG_INT_SIZE; k++) {
                    #pragma HLS UNROLL
                    ap_uint<32> dst = seg.range(32*(k+1)-1, 32*k);
                    if (dst != (ap_uint<32>)UNUSED_ICON)
                        write_pkt(pkt_out, dst, 0xFF, 0, KEEP64);
                }
            }
            write_pkt(pkt_out, 0, 0xFF, 1, KEEP64);
            write_pkt(pkt_out, 0, 0, 1, 0);
            continue;
        }

        if (cmd.mode == PMA_GET_ALL_OUT_EDGE_COO || cmd.mode == PMA_GET_ALL_OUT_EDGE_CSR) {
            ap_uint<512> max_word = hbm_ptr[vec_info_base / 64];
            int max_pma_idx = (int)max_word.range(63, 0);

            for (int pma_idx = WORKER_ID; pma_idx <= max_pma_idx; pma_idx += READ_WORKERS) {
                int vi_word = vi_table / 64 + pma_idx / 8;
                int vi_shift = (pma_idx % 8) * 64;
                ap_uint<512> vi_pkt = hbm_ptr[vi_word];
                VEC_INFO vi = vi_pkt.range(vi_shift + 63, vi_shift);
                if (vi == 0) continue;

                int pma_start = (int)vi.range(31, 0);
                int hdr_word = pma_start / 64;

                ap_uint<512> hdr[HEADER_WORDS];
                for (int j = 0; j < HEADER_WORDS; j++)
                    hdr[j] = hbm_ptr[hdr_word + j];

                for (int v = 0; v < VERTEX_PER_PMA; v++) {
                    int vs = (v == 0) ? 0 : (int)hdr[0].range(32*v - 1, 32*(v-1));
                    int ve = (int)hdr[0].range(32*(v+1) - 1, 32*v);
                    int v_src = (pma_idx << 4) | v;

                    for (int s = vs; s < ve; s++) {
                        ap_uint<512> seg = hbm_ptr[hdr_word + HEADER_WORDS + s];
                        for (int k = 0; k < SEG_INT_SIZE; k++) {
                            #pragma HLS UNROLL
                            ap_uint<32> dst = seg.range(32*(k+1)-1, 32*k);
                            if (dst != (ap_uint<32>)UNUSED_ICON) {
                                ap_uint<64> pair = ((ap_uint<64>)(ap_uint<32>)v_src << 32)
                                                   | (ap_uint<32>)dst;
                                write_pkt(pkt_out, pair, 0xFF, 0, KEEP64);
                            }
                        }
                    }
                }
                write_pkt(pkt_out, 0, 0xFF, 1, KEEP64);
            }
            write_pkt(pkt_out, 0, 0, 1, 0);
            continue;
        }

        if (cmd.mode == PMA_GET_EDGE_COUNT_RANGE) {
            int start_v = cmd.src_idx;
            int end_v = cmd.end_src_idx;
            if (end_v <= start_v) {
                write_pkt(pkt_out, 0, 0, 0, KEEP64);
                write_pkt(pkt_out, 0, 0, 1, 0);
                continue;
            }

            int last_v = end_v - 1;
            int pma_start_p = start_v / VERTEX_PER_PMA;
            int pma_end_p   = last_v / VERTEX_PER_PMA;
            int total_sum = 0;
            bool find_empty = false;

            for (int pma_idx = WORKER_ID; pma_idx <= pma_end_p; pma_idx += READ_WORKERS) {
                if (pma_idx < pma_start_p) continue;

                int vi_word = vi_table / 64 + pma_idx / 8;
                int vi_shift = (pma_idx % 8) * 64;
                ap_uint<512> vi_pkt = hbm_ptr[vi_word];
                VEC_INFO vi = vi_pkt.range(vi_shift + 63, vi_shift);
                if (vi == 0) { find_empty = true; break; }

                int pa = (int)vi.range(31, 0);
                int hw = pa / 64;
                ap_uint<512> hdr[HEADER_WORDS];
                for (int j = 0; j < HEADER_WORDS; j++)
                    hdr[j] = hbm_ptr[hw + j];

                int ls = (pma_idx == pma_start_p) ? (start_v % VERTEX_PER_PMA) : 0;
                int le = (pma_idx == pma_end_p)   ? (last_v  % VERTEX_PER_PMA) : (VERTEX_PER_PMA - 1);
                for (int i = ls; i <= le; i++)
                    total_sum += (int)hdr[1].range(32*(i+1)-1, 32*i);
            }

            write_pkt(pkt_out, (ap_uint<64>)(ap_uint<32>)total_sum,
                      find_empty ? (ap_uint<8>)0 : (ap_uint<8>)0xFF, 0, KEEP64);
            write_pkt(pkt_out, 0, 0, 1, 0);
        }
    }
    printf("READ_WORKER[%d] DONE\n", WORKER_ID);
}

// =========================================================================
// merger_unified
// =========================================================================
static void merger_unified(
    hls::stream<READ_CMD>& cmd_in,
    hls::stream<READ_PKT>& in0,
    hls::stream<READ_PKT>& in1,
    hls::stream<ap_axiu<64,0,0,0>>& out_coo,
    hls::stream<ap_axiu<32,0,0,0>>& out_edge_count)
{
    #pragma HLS INLINE off
    printf("READ_MERGER START\n");

    while (true) {
        READ_CMD cmd = cmd_in.read();
        if (cmd.last) break;

        int mode = cmd.mode;
        bool done[READ_WORKERS];
        #pragma HLS ARRAY_PARTITION variable=done complete
        for (int i = 0; i < READ_WORKERS; i++) {
            #pragma HLS UNROLL
            done[i] = false;
        }
        int done_count = 0, rr = 0, total_edge_count = 0;
        bool count_end = false;

        while (done_count < READ_WORKERS) {
            #pragma HLS PIPELINE II=1
            if (done[rr]) { rr = (rr + 1) % READ_WORKERS; continue; }

            READ_PKT pkt = (rr == 0) ? in0.read() : in1.read();

            if (pkt.keep == 0 && pkt.last == 1) {
                done[rr] = true; done_count++;
                rr = (rr + 1) % READ_WORKERS;
                continue;
            }
            if (pkt.keep != 0 && pkt.last == 1) { rr = (rr + 1) % READ_WORKERS; continue; }
            if (pkt.keep == 0) continue;

            if (mode == PMA_GET_ALL_OUT_EDGE_COO || mode == PMA_GET_ALL_OUT_EDGE_CSR) {
                ap_axiu<64,0,0,0> o = {};
                o.data = pkt.data;
                o.keep = 0xFF; o.strb = 0xFF;
                out_coo.write(o);
            } else if (mode == PMA_GET_EDGE_COUNT_RANGE) {
                total_edge_count += (int)pkt.data.range(31, 0);
                if (pkt.user == 0) count_end = true;
            }
        }

        if (mode == PMA_GET_ALL_OUT_EDGE_COO || mode == PMA_GET_ALL_OUT_EDGE_CSR) {
            ap_axiu<64,0,0,0> o = {};
            o.data = (ap_uint<64>)-1;
            o.keep = 0xFF; o.strb = 0xFF;
            o.last = 1;
            out_coo.write(o);
        } else if (mode == PMA_GET_EDGE_COUNT_RANGE) {
            ap_axiu<32,0,0,0> o = {};
            o.data = (ap_uint<32>)total_edge_count;
            o.keep = 0xF; o.strb = 0xF;
            o.last = count_end ? (ap_uint<1>)1 : (ap_uint<1>)0;
            out_edge_count.write(o);
        }
    }
    printf("READ_MERGER DONE\n");
}

// =========================================================================
// top-level
// =========================================================================
extern "C"
{
void pma_read_kernel(
    hls::stream<READ_COMMAND_STREAM>& cmd_stream,
    hls::stream<ap_axiu<64,0,0,0>>& data_to_coo_stream,
    hls::stream<ap_axiu<32,0,0,0>>& coo_edge_count_stream,
    ap_uint<512>* hbm_ptr_0,
    ap_uint<512>* hbm_ptr_1,
    int vec_info_base)
{
    #pragma HLS INTERFACE axis port=cmd_stream
    #pragma HLS INTERFACE axis port=data_to_coo_stream
    #pragma HLS INTERFACE axis port=coo_edge_count_stream
    #pragma HLS INTERFACE m_axi port=hbm_ptr_0 offset=slave bundle=gmem0
    #pragma HLS INTERFACE m_axi port=hbm_ptr_1 offset=slave bundle=gmem1
    #pragma HLS INTERFACE s_axilite port=hbm_ptr_0 
    #pragma HLS INTERFACE s_axilite port=hbm_ptr_1 
    #pragma HLS INTERFACE s_axilite port=vec_info_base
    #pragma HLS INTERFACE s_axilite port=return 

    #pragma HLS DATAFLOW

    hls::stream<READ_CMD> worker_cmd_0;
    hls::stream<READ_CMD> worker_cmd_1;
    #pragma HLS STREAM variable=worker_cmd_0 depth=16
    #pragma HLS STREAM variable=worker_cmd_1 depth=16

    hls::stream<READ_CMD> merger_cmd;
    #pragma HLS STREAM variable=merger_cmd depth=64

    hls::stream<READ_PKT> u_stream_0;
    hls::stream<READ_PKT> u_stream_1;
    #pragma HLS STREAM variable=u_stream_0 depth=128
    #pragma HLS STREAM variable=u_stream_1 depth=128

    distributor(cmd_stream, worker_cmd_0, worker_cmd_1, merger_cmd);

    read_worker<0>(worker_cmd_0, u_stream_0, hbm_ptr_0, vec_info_base);
    read_worker<1>(worker_cmd_1, u_stream_1, hbm_ptr_1, vec_info_base);

    merger_unified(merger_cmd, u_stream_0, u_stream_1,
                   data_to_coo_stream, coo_edge_count_stream);
}
}
