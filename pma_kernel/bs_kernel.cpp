#include "data_type.h"
#include "common_function.h"
#include <cstdio>
#include <cstring>
#include <hls_stream.h>
#include <ap_utils.h>
#include "hls_burst_maxi.h"
#include "hls_print.h"

#ifndef BS_WORKER_NUM
#define BS_WORKER_NUM 2
#endif
#define BS_CACHE_SLOTS 16

// =========================================================================
// worker unit
// =========================================================================
static int sub_search_16seg(
    int bs_helper[10240], int low, int high, int key)
{
    #pragma HLS INLINE
    int range = high - low;
    if (range <= 4) {
        for (int s = high - 1; s >= low; s--) {
            #pragma HLS PIPELINE II=1
            if (bs_helper[s] <= key) return s;
        }
        return low;
    }
    while (low < high) {
        #pragma HLS PIPELINE II=2
        int mid = (low + high) >> 1;
        if (bs_helper[mid] <= key) low = mid + 1;
        else high = mid;
    }
    return (low > 0) ? low - 1 : 0;
}

static void build_bs_helper(
    PMA_HEADER& header,
    ap_uint<512>* hbm_ptr,
    int hdr_word_base,
    int bs_helper[10240])
{
    #pragma HLS INLINE off
    int total_segs = header.vertex_range[VERTEX_PER_PMA - 1];
    for (int s = 0; s < total_segs; s++) {
        #pragma HLS PIPELINE II=1
        ap_uint<512> seg = hbm_ptr[hdr_word_base + HEADER_WORDS + s];
        int first = (int)seg.range(31, 0);
        bs_helper[s] = (first == UNUSED_ICON) ? 0x7FFFFFFF : first;
    }
}

static bool bs_cache_check(
    BS_CACHE_ENTRY& entry, int pma_idx, PMA_HEADER& header)
{
    #pragma HLS INLINE
    if (!entry.valid || entry.pma_idx != pma_idx) return false;
    for (int v = 0; v < VERTEX_PER_PMA; v++) {
        #pragma HLS UNROLL
        if (entry.vertex_range[v] != header.vertex_range[v]) return false;
    }
    return true;
}

static void bs_worker_unit(
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_in,
    hls::stream<BS_OUT_MSG>& out,
    ap_uint<512>* hbm_ptr,
    int vec_info_base
)
{
    #pragma HLS INLINE off

    BS_CACHE_ENTRY bs_cache[BS_CACHE_SLOTS];
    #pragma HLS BIND_STORAGE variable=bs_cache type=ram_2p impl=BRAM
    for (int i = 0; i < BS_CACHE_SLOTS; i++) {
        #pragma HLS UNROLL
        bs_cache[i].valid = false; bs_cache[i].pma_idx = -1;
    }
    int cache_fill = 0;

    while (true) {
        PMA_UPDATE_MESSAGE msg = cmd_in.read();

        if (msg.flags == exit_flag) {
            BS_OUT_MSG stop = {};
            stop.flags = exit_flag;
            out.write(stop);
            break;
        }

        // Clear probe: forward unchanged.
        if (msg.flags == clear_probe) {
            BS_OUT_MSG probe = {};
            probe.flags = clear_probe;
            out.write(probe);
            continue;
        }

        int pma_idx = msg.src >> 4;

        int vi_word = vec_info_base / 64 + pma_idx / 8;
        int vi_shift = (pma_idx % 8) * 64;
        ap_uint<512> vi_pkt = hbm_ptr[vi_word];
        VEC_INFO vi = vi_pkt.range(vi_shift + 63, vi_shift);

        if (vi == (ap_uint<64>)(-1)) {
            BS_OUT_MSG o = {};
            o.src = msg.src; o.dst = msg.dst;
            o.opcode = msg.opcode;
            o.flags = locked_flag;
            out.write(o);
            continue;
        }

        if (vi == 0) {
            BS_OUT_MSG o = {};
            o.src = msg.src; o.dst = msg.dst;
            o.opcode = msg.opcode;
            o.flags = empty_pma_flag;
            out.write(o);
            continue;
        }

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
                header.vertex_range[i] = (int)hdr[0].range(32 * (i + 1) - 1, 32 * i);
                header.edge_count[i]  = (int)hdr[1].range(32 * (i + 1) - 1, 32 * i);
            }
        }

        int slot = -1;
        for (int s = 0; s < BS_CACHE_SLOTS; s++) {
            #pragma HLS PIPELINE II=1
            if (bs_cache[s].valid && bs_cache[s].pma_idx == pma_idx) { slot = s; break; }
        }
        if (slot >= 0 && !bs_cache_check(bs_cache[slot], pma_idx, header)) {
            bs_cache[slot].valid = false; slot = -1;
        }
        if (slot < 0) {
            slot = cache_fill;
            cache_fill = (cache_fill + 1) % BS_CACHE_SLOTS;
            BS_CACHE_ENTRY& e = bs_cache[slot];
            e.pma_idx = pma_idx;
            for (int v = 0; v < VERTEX_PER_PMA; v++) {
                #pragma HLS UNROLL
                e.vertex_range[v] = header.vertex_range[v];
            }
            build_bs_helper(header, hbm_ptr, hdr_word, e.bs_data);
            e.valid = true;
        }

        BS_OUT_MSG o = {};
        o.src = msg.src; o.dst = msg.dst;
        o.opcode = msg.opcode;
        o.flags = normal_flag;

        int ls = msg.src & 0xF;
        int ss = (ls == 0) ? 0 : header.vertex_range[ls - 1];
        int se = header.vertex_range[ls];
        o.seg_idx = sub_search_16seg(bs_cache[slot].bs_data, ss, se, msg.dst);
        
        out.write(o);
    }
}


// =========================================================================
// top-level: bs_kernel
// =========================================================================
extern "C"
{
void bs_kernel(
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_stream,
    hls::stream<BS_OUT_MSG>& to_insert,
    ap_uint<512>* hbm_ptr_0,
    ap_uint<512>* hbm_ptr_1,
    int vec_info_base)
{
    #pragma HLS INTERFACE axis port=cmd_stream
    #pragma HLS INTERFACE axis port=to_insert
    #pragma HLS INTERFACE m_axi port=hbm_ptr_0 offset=slave bundle=gmem0
    #pragma HLS INTERFACE m_axi port=hbm_ptr_1 offset=slave bundle=gmem1
    #pragma HLS INTERFACE s_axilite port=hbm_ptr_0
    #pragma HLS INTERFACE s_axilite port=hbm_ptr_1
    #pragma HLS INTERFACE s_axilite port=vec_info_base
    #pragma HLS INTERFACE s_axilite port=return

    #pragma HLS DATAFLOW

    hls::stream<PMA_UPDATE_MESSAGE> worker_cmd[BS_WORKER_NUM];
    #pragma HLS STREAM variable=worker_cmd depth=16
    hls::stream<BS_OUT_MSG> worker_out[BS_WORKER_NUM];
    #pragma HLS STREAM variable=worker_out depth=32

    dispatch<PMA_UPDATE_MESSAGE, BS_WORKER_NUM>(cmd_stream, worker_cmd);

    bs_worker_unit(worker_cmd[0], worker_out[0], hbm_ptr_0, vec_info_base);
    bs_worker_unit(worker_cmd[1], worker_out[1], hbm_ptr_1, vec_info_base);

    merger<BS_OUT_MSG, BS_WORKER_NUM>(worker_out, to_insert);
}
}
