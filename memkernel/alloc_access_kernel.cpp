#include <cstdio>
#include <hls_stream.h>
#include "data_type.h"

// Configurable bitmap storage granularity (default: 512-bit per word).
#ifndef BITMAP_STORAGE_WORD_BITS
#define BITMAP_STORAGE_WORD_BITS 512
#endif

typedef ap_uint<BITMAP_STORAGE_WORD_BITS> bitmap_word_t;

const int TOTAL_BLOCKS = (CHANNEL_CAPACITY / INIT_VEC_LEN_IN_BYTE);
const int BITMAP_SIZE = (TOTAL_BLOCKS + 7) / 8;
const int BITMAP_WORD_BITS = BITMAP_STORAGE_WORD_BITS;
const int BITMAP_WORD_BYTES = BITMAP_WORD_BITS / 8;
const int BITMAP_WORDS = (TOTAL_BLOCKS + BITMAP_WORD_BITS - 1) / BITMAP_WORD_BITS;

static void set_bits(int start, int len, bitmap_word_t* bitmap)
{
    #pragma HLS INLINE off
    int end = start + len;
    int i = start;

    // Process at word granularity; build a mask per-word and do one RMW.
    while (i < end)
    {
        int word_idx = i / BITMAP_WORD_BITS;
        int word_start = word_idx * BITMAP_WORD_BITS;
        int word_end = word_start + BITMAP_WORD_BITS;

        int s = i - word_start;
        int e = ((end < word_end) ? end : word_end) - word_start;
        int width = e - s;

        bitmap_word_t mask = 0;
        if (width <= 0) {
            // nothing
        } else if (width >= BITMAP_WORD_BITS) {
            mask = ~((bitmap_word_t)0);
        } else {
            bitmap_word_t one = (bitmap_word_t)1;
            bitmap_word_t bits = (one << width) - (bitmap_word_t)1;
            mask = bits << s;
        }

        // read-modify-write: OR the mask
        bitmap_word_t cur = bitmap[word_idx];
        bitmap[word_idx] = cur | mask;
        i += width;
    }
}

static bool is_free(int start, int len, int &first_set, bitmap_word_t* bitmap)
{
    #pragma HLS INLINE
    int end = start + len;
    int i = start;

    while (i < end)
    {
        int word_idx = i / BITMAP_WORD_BITS;
        int word_start = word_idx * BITMAP_WORD_BITS;
        int word_end = word_start + BITMAP_WORD_BITS;
        bitmap_word_t cur = bitmap[word_idx];

        int s = i - word_start;
        int e = ((end < word_end) ? end : word_end) - word_start;
        int width = e - s;

        bitmap_word_t mask = 0;
        if (width <= 0) {
            // nothing
        } else if (width >= BITMAP_WORD_BITS) {
            mask = ~((bitmap_word_t)0);
        } else {
            bitmap_word_t one = (bitmap_word_t)1;
            bitmap_word_t bits = (one << width) - (bitmap_word_t)1;
            mask = bits << s;
        }

        bitmap_word_t tmp = cur & mask;
        if (tmp != 0)
        {
            // find first set bit inside [s, e)
            for (int b = s; b < e; ++b)
            {
                #pragma HLS PIPELINE II=1
                if (cur[b])
                {
                    first_set = word_start + b;
                    return false;
                }
            }
        }

        i += width;
    }
    return true;
}

static void allocate_next_fit(int &start, int len, int &next_cursor, bitmap_word_t* bitmap)
{
    #pragma HLS INLINE off
    start = -1;
    if (len <= 0 || len > TOTAL_BLOCKS) return;

    const int max_start = TOTAL_BLOCKS - len;
    int first_set = -1;

    int begin = next_cursor;
    if (begin > max_start) begin = 0;

    for (int i = begin; i <= max_start; i++)
    {
        #pragma HLS LOOP_TRIPCOUNT min=1 max=256000
        bool free = is_free(i, len, first_set, bitmap);
        if (free)
        {
            start = i;
            set_bits(start, len, bitmap);
            next_cursor = start + len;
            if (next_cursor >= TOTAL_BLOCKS) next_cursor = 0;
            return;
        }
        i = first_set;
    }

    for (int i = 0; i < begin; i++)
    {
        #pragma HLS LOOP_TRIPCOUNT min=1 max=256000
        if (i > max_start) break;
        bool free = is_free(i, len, first_set, bitmap);
        if (free)
        {
            start = i;
            set_bits(start, len, bitmap);
            next_cursor = start + len;
            if (next_cursor >= TOTAL_BLOCKS) next_cursor = 0;
            return;
        }
        i = first_set;
    }
}

static void reset_bits(int start, int len, bitmap_word_t* bitmap)
{
    #pragma HLS INLINE off
    int end = start + len;
    int i = start;

    while (i < end)
    {
        int word_idx = i / BITMAP_WORD_BITS;
        int word_start = word_idx * BITMAP_WORD_BITS;
        int word_end = word_start + BITMAP_WORD_BITS;

        int s = i - word_start;
        int e = ((end < word_end) ? end : word_end) - word_start;
        int width = e - s;

        bitmap_word_t mask = 0;
        if (width <= 0) {
            // nothing
        } else if (width >= BITMAP_WORD_BITS) {
            mask = ~((bitmap_word_t)0);
        } else {
            bitmap_word_t one = (bitmap_word_t)1;
            bitmap_word_t bits = (one << width) - (bitmap_word_t)1;
            mask = bits << s;
        }

        bitmap_word_t cur = bitmap[word_idx];
        bitmap[word_idx] = cur & (~mask);
        i += width;
    }
}

static void deallocate(int start, int len, bitmap_word_t* bitmap)
{
    #pragma HLS INLINE off
    if (len == 0) return;
    reset_bits(start, len, bitmap);
}

static void alloc_free_worker(
    hls::stream<MEMORY_INFO_ACCESS_COMMAND>& req_edge_in,
    hls::stream<VEC_INFO_STREAM>& vec_info_to_edge,
    bitmap_word_t* bm_in_hbm,
    bool init_from_hbm
)
{
    #pragma HLS INTERFACE m_axi port=bm_in_hbm offset=slave bundle=gmem0

    bitmap_word_t storage[BITMAP_WORDS];
    #pragma HLS bind_storage variable=storage type=ram_t2p impl=bram
    #pragma HLS DEPENDENCE variable=storage inter false

    if (init_from_hbm)
    {
        for (int i = 0 ; i < BITMAP_WORDS ; i++)
        {
            #pragma HLS PIPELINE II=1
            storage[i] = bm_in_hbm[i];
        }
    }
    else
    {
        for (int i = 0; i < BITMAP_WORDS; i++)
        {
            #pragma HLS PIPELINE II=1
            storage[i] = 0;
        }
    }

    bool finish = false;
    int next_fit_cursor = 0;

    while (true)
    {
        MEMORY_INFO_ACCESS_COMMAND pending_info;
        bool valid_req = false;
        
        if (req_edge_in.read_nb(pending_info))
        {
            valid_req = true;
        }
        if (!valid_req)
        {
            if (finish)
            {
                break;
            }
            continue;
        }

        MEMORY_INFO_ACCESS_TYPE req_type = pending_info.command_type;

        if (req_type == ALLOC_FINISH || pending_info.last == 1)
        {
            
            finish = true;
            if (finish)
            {
                break;
            }
            continue;
        }

        if (pending_info.last == 1)
        {
            break;
        }

        VEC_INFO back_info = 0;

        if (req_type == VEC_INFO_ALLOC)
        {
            int req_bytes = pending_info.len;
            int len_segs = (req_bytes + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
            int start_seg;
            allocate_next_fit(start_seg, len_segs, next_fit_cursor, storage);

            int reg_start_seg = start_seg;
            if (reg_start_seg == -1)
            {
                perror("ERROR INSUFFICIENT MEMORY POOL\n");
            }
            int start_bytes = reg_start_seg * INIT_VEC_LEN_IN_BYTE;
            back_info.range(31,0) = start_bytes;
            back_info.range(63,32) = req_bytes;

            VEC_INFO_STREAM out;
            out.data = back_info;
            out.last = 0;
            vec_info_to_edge.write(out); 
        }
        else if (req_type == VEC_INFO_FREE)
        {
            int start_bytes = pending_info.start;
            int len_bytes = pending_info.len;
            int start_seg = start_bytes / INIT_VEC_LEN_IN_BYTE;
            int len_segs  = (len_bytes + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
            deallocate(start_seg, len_segs, storage);
        }
    }
}

extern "C"
{
    void alloc_access_kernel(
        bitmap_word_t* bm_in_hbm,
        bool init_from_hbm,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND>& req_from_edge,
        hls::stream<VEC_INFO_STREAM>& vec_info_to_edge
    )
    {
        #pragma HLS INTERFACE m_axi port=bm_in_hbm offset=slave bundle=gmem0
        #pragma HLS INTERFACE s_axilite port=bm_in_hbm bundle=control
        #pragma HLS INTERFACE s_axilite port=init_from_hbm bundle=control
        #pragma HLS INTERFACE s_axilite port=return bundle=control
        #pragma HLS INTERFACE axis port=req_from_edge
        #pragma HLS INTERFACE axis port=vec_info_to_edge
        #pragma HLS STREAM variable=req_from_edge depth=16
        #pragma HLS STREAM variable=vec_info_to_edge depth=4

        #pragma HLS DATAFLOW

        alloc_free_worker(req_from_edge, vec_info_to_edge, bm_in_hbm, init_from_hbm);
    }
}
