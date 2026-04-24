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

    while (i < end)
    {
        #pragma HLS LOOP_TRIPCOUNT min=1 max=4096
        int word_idx = i / BITMAP_WORD_BITS;
        int word_start = word_idx * BITMAP_WORD_BITS;
        int word_end = word_start + BITMAP_WORD_BITS;
        bitmap_word_t cur = bitmap[word_idx];

        int s = i - word_start;
        int e = ((end < word_end) ? end : word_end) - word_start;

        for (int b = s; b < e; ++b)
        {
            #pragma HLS LOOP_TRIPCOUNT min=1 max=BITMAP_WORD_BITS
            cur[b] = 1;
        }

        bitmap[word_idx] = cur;
        i += (e - s);
    }
}

static bool is_free(int start, int len, int &first_set, bitmap_word_t* bitmap)
{
    #pragma HLS INLINE
    int end = start + len;
    int i = start;

    while (i < end)
    {
        #pragma HLS LOOP_TRIPCOUNT min=1 max=4096
        int word_idx = i / BITMAP_WORD_BITS;
        int word_start = word_idx * BITMAP_WORD_BITS;
        int word_end = word_start + BITMAP_WORD_BITS;
        bitmap_word_t cur = bitmap[word_idx];

        int s = i - word_start;
        int e = ((end < word_end) ? end : word_end) - word_start;

        bitmap_word_t mask = 0;
        for (int b = s; b < e; ++b)
        {
            #pragma HLS LOOP_TRIPCOUNT min=1 max=BITMAP_WORD_BITS
            mask[b] = 1;
        }

        if ((cur & mask) != 0)
        {
            for (int b = s; b < e; ++b)
            {
                #pragma HLS UNROLL
                if (cur[b])
                {
                    first_set = word_start + b;
                    return false;
                }
            }
        }

        i += (e - s);
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
        #pragma HLS LOOP_TRIPCOUNT min=1 max=4096
        int word_idx = i / BITMAP_WORD_BITS;
        int word_start = word_idx * BITMAP_WORD_BITS;
        int word_end = word_start + BITMAP_WORD_BITS;
        bitmap_word_t cur = bitmap[word_idx];

        int s = i - word_start;
        int e = ((end < word_end) ? end : word_end) - word_start;

        for (int b = s; b < e; ++b)
        {
            #pragma HLS LOOP_TRIPCOUNT min=1 max=BITMAP_WORD_BITS
            cur[b] = 0;
        }

        bitmap[word_idx] = cur;
        i += (e - s);
    }
}

static void deallocate(int start, int len, bitmap_word_t* bitmap)
{
    #pragma HLS INLINE off
    if (len == 0) return;
    reset_bits(start, len, bitmap);
}

static void alloc_free_worker(
    hls::stream<MEMORY_INFO_ACCESS_COMMAND>& req_vertex_in,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND>& req_edge_in,
    hls::stream<VEC_INFO_STREAM>& vec_info_to_vertex,
    hls::stream<VEC_INFO_STREAM>& vec_info_to_edge,
    BYTE* bm_in_hbm,
    bool init_from_hbm
)
{
    #pragma HLS INTERFACE m_axi port=bm_in_hbm offset=slave bundle=gmem0

    bitmap_word_t storage[BITMAP_WORDS];
    #pragma HLS bind_storage variable=storage type=ram_t2p impl=bram
    #pragma HLS DEPENDENCE variable=storage inter false

    if (init_from_hbm)
    {
        // Prefer full word loads from HBM to keep bitmap representation word-based.
        bitmap_word_t* bm_words = (bitmap_word_t*)bm_in_hbm;
        int full_words = BITMAP_SIZE / BITMAP_WORD_BYTES;
        int rem_bytes = BITMAP_SIZE % BITMAP_WORD_BYTES;

        for (int i = 0 ; i < full_words ; i++)
        {
            #pragma HLS PIPELINE II=1
            storage[i] = bm_words[i];
        }

        if (full_words < BITMAP_WORDS)
        {
            if (rem_bytes != 0)
            {
                bitmap_word_t tail_word = 0;
                for (int b = 0; b < rem_bytes; ++b)
                {
                    #pragma HLS PIPELINE II=1
                    int byte_idx = full_words * BITMAP_WORD_BYTES + b;
                    tail_word.range((b + 1) * 8 - 1, b * 8) = bm_in_hbm[byte_idx];
                }
                storage[full_words] = tail_word;
            }

            int zero_start = full_words + ((rem_bytes != 0) ? 1 : 0);
            for (int i = zero_start; i < BITMAP_WORDS; ++i)
            {
                #pragma HLS PIPELINE II=1
                storage[i] = 0;
            }
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

    bool finish_vertex = false;
    bool finish_edge = false;
    int next_fit_cursor = 0;

    while (true)
    {
        MEMORY_INFO_ACCESS_COMMAND pending_info;
        bool from_vertex = false;
        bool valid_req = false;
        
        if (req_vertex_in.read_nb(pending_info))
        {
            from_vertex = true;
            valid_req = true;
        }
        else if (req_edge_in.read_nb(pending_info))
        {
            from_vertex = false;
            valid_req = true;
        }
        if (!valid_req)
        {
            if (finish_vertex && finish_edge)
            {
                break;
            }
            continue;
        }

        MEMORY_INFO_ACCESS_TYPE req_type = pending_info.command_type;

        if (req_type == ALLOC_FINISH || pending_info.last == 1)
        {
            if (from_vertex)
            {
                finish_vertex = true;
            }
            else
            {
                finish_edge = true;
            }
            if (finish_vertex && finish_edge)
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
            if (from_vertex)
            {
                vec_info_to_vertex.write(out);
            }
            else
            {
                vec_info_to_edge.write(out);
            }
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
        BYTE* bm_in_hbm,
        bool init_from_hbm,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND>& req_from_vertex,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND>& req_from_edge,
        hls::stream<VEC_INFO_STREAM>& vec_info_to_vertex,
        hls::stream<VEC_INFO_STREAM>& vec_info_to_edge
    )
    {
        #pragma HLS INTERFACE m_axi port=bm_in_hbm offset=slave bundle=gmem0
        #pragma HLS INTERFACE s_axilite port=bm_in_hbm bundle=control
        #pragma HLS INTERFACE s_axilite port=init_from_hbm bundle=control
        #pragma HLS INTERFACE s_axilite port=return bundle=control
        #pragma HLS INTERFACE axis port=req_from_vertex
        #pragma HLS INTERFACE axis port=req_from_edge
        #pragma HLS INTERFACE axis port=vec_info_to_vertex
        #pragma HLS INTERFACE axis port=vec_info_to_edge
        #pragma HLS STREAM variable=req_from_vertex depth=16
        #pragma HLS STREAM variable=req_from_edge depth=16
        #pragma HLS STREAM variable=vec_info_to_vertex depth=4
        #pragma HLS STREAM variable=vec_info_to_edge depth=4

        #pragma HLS DATAFLOW

        alloc_free_worker(req_from_vertex, req_from_edge, vec_info_to_vertex, vec_info_to_edge, bm_in_hbm, init_from_hbm);
    }
}
