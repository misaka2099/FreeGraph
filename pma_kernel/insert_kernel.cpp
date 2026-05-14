#include "data_type.h"
#include "common_function.h"
#include <cstdio>
#include <cstring>
#include <hls_stream.h>
#include <ap_utils.h>
#include "hls_burst_maxi.h"
#include "hls_print.h"

#define INSERT_WORKER_NUM 2

static void insert_worker(
    hls::stream<BS_OUT_MSG>& in,
    hls::stream<REBALANCE_REQ>& rebalance_out,
    hls::stream<PMA_UPDATE_MESSAGE>& reprocess_out,
    ap_uint<512>* hbm_ptr,
    int vec_info_base)
{
    #pragma HLS INLINE off
    while (true) {
        BS_OUT_MSG operation = in.read();
        if (operation.flags == exit_flag) {
            REBALANCE_REQ sr = {};
            sr.flags = exit_flag;
            rebalance_out.write(sr);
            PMA_UPDATE_MESSAGE sa = {};
            sa.flags = exit_flag;
            reprocess_out.write(sa);
            break;
        }

        // Clear probe: forward back to router unchanged.
        if (operation.flags == clear_probe) {
            PMA_UPDATE_MESSAGE probe = {};
            probe.flags = clear_probe;
            reprocess_out.write(probe);
            continue;
        }

        // locked_flag: PMA locked by rebalance, reprocess the edge
        if (operation.flags == locked_flag) {
            PMA_UPDATE_MESSAGE r = {};
            r.src = operation.src; r.dst = operation.dst;
            r.opcode = operation.opcode; r.flags = normal_flag;
            reprocess_out.write(r);
            continue;
        }

        // empty_pma_flag: PMA not yet allocated, reprocess + request allocation
        if (operation.flags == empty_pma_flag) {
            PMA_UPDATE_MESSAGE r = {};
            r.src = operation.src; r.dst = operation.dst;
            r.opcode = operation.opcode; r.flags = normal_flag;
            reprocess_out.write(r);

            REBALANCE_REQ req = {};
            req.src = operation.src;
            req.seg_idx = 0;
            req.old_addr = 0;
            req.flags = normal_flag;
            rebalance_out.write(req);
            continue;
        }

        int pma_idx = operation.src >> 4;
        int vi_word = vec_info_base / 64 + pma_idx / 8;
        int vi_shift = (pma_idx % 8) * 64;
        ap_uint<512> vi_pkt = hbm_ptr[vi_word];
        VEC_INFO vi = vi_pkt.range(vi_shift + 63, vi_shift);

        int pma_start = (int)vi.range(31, 0);
        int hdr_word = pma_start / 64;
        PMA_HEADER header;
        {
            ap_uint<512> hdr[HEADER_WORDS];
            for (int j = 0; j < HEADER_WORDS; j++) {
                #pragma HLS PIPELINE II=1
                hdr[j] = hbm_ptr[hdr_word + j];
            }
            for (int i = 0; i < 16; i++) {
                #pragma HLS UNROLL
                header.vertex_range[i] = (int)hdr[0].range(32*(i+1)-1, 32*i);
                header.edge_count[i]  = (int)hdr[1].range(32*(i+1)-1, 32*i);
            }
        }

        int seg_addr = hdr_word + HEADER_WORDS + operation.seg_idx;
        ap_uint<512> seg = hbm_ptr[seg_addr];

        bool any_rebalance = false;
        if (operation.opcode == PMA_UPDATE_INSERT_EDGE) {
            int pos = 0, vc = 0;
            for (int k = 0; k < SEG_INT_SIZE; k++) {
                #pragma HLS UNROLL
                ap_uint<32> val = seg.range(32*(k+1)-1, 32*k);
                if (val != (ap_uint<32>)UNUSED_ICON) {
                    vc++;
                    if (val < (ap_uint<32>)operation.dst) pos = k + 1;
                }
            }
            if (vc >= SEG_INT_SIZE) {
                any_rebalance = true;
                PMA_UPDATE_MESSAGE r = {};
                r.src = operation.src; r.dst = operation.dst;
                r.opcode = operation.opcode; r.flags = normal_flag;
                reprocess_out.write(r);
            } else {
                ap_uint<512> res = seg;
                for (int k = SEG_INT_SIZE - 1; k > pos; k--)
                    res.range(32*(k+1)-1, 32*k) = res.range(32*k-1, 32*(k-1));
                res.range(32*(pos+1)-1, 32*pos) = (ap_uint<32>)operation.dst;
                hbm_ptr[seg_addr] = res;
                header.edge_count[operation.src & 0xF]++;
            }
        } else {
            int del_pos = -1;
            for (int k = 0; k < SEG_INT_SIZE; k++) {
                #pragma HLS UNROLL
                if (seg.range(32*(k+1)-1, 32*k) == (ap_uint<32>)operation.dst) del_pos = k;
            }
            if (del_pos >= 0) {
                ap_uint<512> res = seg;
                for (int k = del_pos; k < SEG_INT_SIZE - 1; k++)
                    res.range(32*(k+1)-1, 32*k) = res.range(32*(k+2)-1, 32*(k+1));
                res.range(32*SEG_INT_SIZE-1, 32*(SEG_INT_SIZE-1)) = (ap_uint<32>)UNUSED_ICON;
                hbm_ptr[seg_addr] = res;
                header.edge_count[operation.src & 0xF]--;
            }
        }

        if (any_rebalance) {
            vi_pkt.range(vi_shift + 63, vi_shift) = (ap_uint<64>)(-1);
            hbm_ptr[vi_word] = vi_pkt;

            REBALANCE_REQ req = {};
            req.src = operation.src;
            req.seg_idx = operation.seg_idx;
            req.old_addr = vi;
            req.flags = normal_flag;
            rebalance_out.write(req);
        } else {
            ap_uint<512> hdr[HEADER_WORDS] = {};
            for (int i = 0; i < 16; i++) {
                #pragma HLS UNROLL
                hdr[0].range(32*(i+1)-1, 32*i) = (ap_uint<32>)header.vertex_range[i];
                hdr[1].range(32*(i+1)-1, 32*i) = (ap_uint<32>)header.edge_count[i];
            }
            for (int j = 0; j < HEADER_WORDS; j++) {
                #pragma HLS PIPELINE II=1
                hbm_ptr[hdr_word + j] = hdr[j];
            }
        }
    }
}

extern "C"
{
void insert_kernel(
    hls::stream<BS_OUT_MSG>& cmd_stream,
    hls::stream<REBALANCE_REQ>& rebalance_out,
    hls::stream<PMA_UPDATE_MESSAGE>& reprocess_out,
    ap_uint<512>* hbm_ptr_0,
    ap_uint<512>* hbm_ptr_1,
    int vec_info_base)
{
    #pragma HLS INTERFACE axis port=cmd_stream
    #pragma HLS INTERFACE axis port=rebalance_out
    #pragma HLS INTERFACE axis port=reprocess_out
    #pragma HLS INTERFACE m_axi port=hbm_ptr_0 offset=slave bundle=gmem0
    #pragma HLS INTERFACE m_axi port=hbm_ptr_1 offset=slave bundle=gmem1
    #pragma HLS INTERFACE s_axilite port=vec_info_base 
    #pragma HLS INTERFACE s_axilite port=return 
    #pragma HLS DATAFLOW

    hls::stream<BS_OUT_MSG> worker_cmd[INSERT_WORKER_NUM];
    #pragma HLS STREAM variable=worker_cmd depth=32
    hls::stream<REBALANCE_REQ> worker_req[INSERT_WORKER_NUM];
    #pragma HLS STREAM variable=worker_req depth=8
    hls::stream<PMA_UPDATE_MESSAGE> worker_reproc[INSERT_WORKER_NUM];
    #pragma HLS STREAM variable=worker_reproc depth=16

    dispatch<BS_OUT_MSG, INSERT_WORKER_NUM>(cmd_stream, worker_cmd);

    ap_uint<512>* hbm_ptrs[INSERT_WORKER_NUM] = { hbm_ptr_0, hbm_ptr_1 };
    for (int w = 0; w < INSERT_WORKER_NUM; w++) {
        #pragma HLS UNROLL
        insert_worker(worker_cmd[w], worker_req[w], worker_reproc[w],
                      hbm_ptrs[w], vec_info_base);
    }

    merger<REBALANCE_REQ, INSERT_WORKER_NUM>(worker_req, rebalance_out);
    merger<PMA_UPDATE_MESSAGE, INSERT_WORKER_NUM>(worker_reproc, reprocess_out);
}
}
