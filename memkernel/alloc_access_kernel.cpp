#include <cstdio>
#include <hls_stream.h>
#include "data_type.h"

// Bitmap constants from data_type.h (ALLOC_BITMAP_STORAGE_WORD_BITS / ALLOC_BITMAP_TOTAL_BLOCKS / WORDS)
typedef ap_uint<ALLOC_BITMAP_STORAGE_WORD_BITS> bitmap_word_t;

// ---- Alloc algorithm (unchanged) ----

static void set_bits(int start, int len, bitmap_word_t* bitmap)
{
    #pragma HLS INLINE off
    int end = start + len;
    int i = start;

    while (i < end)
    {
        int word_idx = i / ALLOC_BITMAP_STORAGE_WORD_BITS;
        int word_start = word_idx * ALLOC_BITMAP_STORAGE_WORD_BITS;
        int word_end = word_start + ALLOC_BITMAP_STORAGE_WORD_BITS;

        int s = i - word_start;
        int e = ((end < word_end) ? end : word_end) - word_start;
        int width = e - s;

        bitmap_word_t mask = 0;
        if (width <= 0) {
        } else if (width >= ALLOC_BITMAP_STORAGE_WORD_BITS) {
            mask = ~((bitmap_word_t)0);
        } else {
            bitmap_word_t one = (bitmap_word_t)1;
            bitmap_word_t bits = (one << width) - (bitmap_word_t)1;
            mask = bits << s;
        }

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
        int word_idx = i / ALLOC_BITMAP_STORAGE_WORD_BITS;
        int word_start = word_idx * ALLOC_BITMAP_STORAGE_WORD_BITS;
        int word_end = word_start + ALLOC_BITMAP_STORAGE_WORD_BITS;
        bitmap_word_t cur = bitmap[word_idx];

        int s = i - word_start;
        int e = ((end < word_end) ? end : word_end) - word_start;
        int width = e - s;

        bitmap_word_t mask = 0;
        if (width <= 0) {
        } else if (width >= ALLOC_BITMAP_STORAGE_WORD_BITS) {
            mask = ~((bitmap_word_t)0);
        } else {
            bitmap_word_t one = (bitmap_word_t)1;
            bitmap_word_t bits = (one << width) - (bitmap_word_t)1;
            mask = bits << s;
        }

        bitmap_word_t tmp = cur & mask;
        if (tmp != 0)
        {
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
    if (len <= 0 || len > ALLOC_BITMAP_TOTAL_BLOCKS) return;

    const int max_start = ALLOC_BITMAP_TOTAL_BLOCKS - len;
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
            if (next_cursor >= ALLOC_BITMAP_TOTAL_BLOCKS) next_cursor = 0;
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
            if (next_cursor >= ALLOC_BITMAP_TOTAL_BLOCKS) next_cursor = 0;
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
        int word_idx = i / ALLOC_BITMAP_STORAGE_WORD_BITS;
        int word_start = word_idx * ALLOC_BITMAP_STORAGE_WORD_BITS;
        int word_end = word_start + ALLOC_BITMAP_STORAGE_WORD_BITS;

        int s = i - word_start;
        int e = ((end < word_end) ? end : word_end) - word_start;
        int width = e - s;

        bitmap_word_t mask = 0;
        if (width <= 0) {
        } else if (width >= ALLOC_BITMAP_STORAGE_WORD_BITS) {
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

// ---- Worker: protocol matching insert_edge ----
// Bitmap initialization comes from external worker via stream (not m_axi).
// No write-back on STOP — host can reconstruct bitmap from data.

static void alloc_free_worker(
    hls::stream<ap_axiu<96,0,0,0>>& cmd_stream,
    hls::stream<ap_axiu<64,0,0,0>>& rsp_stream,
    hls::stream<ap_axiu<ALLOC_BITMAP_STORAGE_WORD_BITS,0,0,0>>& bitmap_init
)
{
    bitmap_word_t storage[ALLOC_BITMAP_WORDS];
    #pragma HLS bind_storage variable=storage type=ram_t2p impl=bram
    #pragma HLS DEPENDENCE variable=storage inter false

    // Init bitmap from stream (sent by pma_gate at start of DATAFLOW)
    for (int i = 0 ; i < ALLOC_BITMAP_WORDS ; i++)
    {
        // #pragma HLS PIPELINE II=1
        ap_axiu<ALLOC_BITMAP_STORAGE_WORD_BITS,0,0,0> w = bitmap_init.read();
        storage[i] = w.data;
    }

    int next_fit_cursor = 0;

    while (true)
    {
        ap_axiu<96,0,0,0> cmd = cmd_stream.read();

        int opcode    = (int)cmd.data.range(95, 64);
        int start_byte = (int)cmd.data.range(31, 0);
        int len_byte   = (int)cmd.data.range(63, 32);

        if (cmd.last == 1 || opcode == 0) {
            // STOP: no write-back — host reconstructs bitmap from data
            break;
        }
        else if (opcode == 1) {
            // ALLOC: len_byte = requested bytes, return start_byte
            int len_segs = (len_byte + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
            int start_seg;
            allocate_next_fit(start_seg, len_segs, next_fit_cursor, storage);

            ap_axiu<64,0,0,0> rsp;
            rsp.last = 0;
            rsp.keep = 0xFF;
            if (start_seg >= 0) {
                int alloc_start = start_seg * INIT_VEC_LEN_IN_BYTE;
                rsp.data.range(31, 0)  = (ap_uint<32>)alloc_start;
                rsp.data.range(63, 32) = (ap_uint<32>)len_byte;
            } else {
                rsp.data.range(31, 0)  = (ap_uint<32>)-1;
                rsp.data.range(63, 32) = 0;
            }
            rsp_stream.write(rsp);
        }
        else if (opcode == 2) {
            // FREE: start_byte, len_byte
            int start_seg = start_byte / INIT_VEC_LEN_IN_BYTE;
            int len_segs  = (len_byte + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
            deallocate(start_seg, len_segs, storage);
            // fire-and-forget: no response
        }
    }
}

// ---- Top-level ----
// No m_axi port — bitmap received via stream from worker, no write-back.

extern "C"
{
    void alloc_access_kernel(
        hls::stream<ap_axiu<ALLOC_BITMAP_STORAGE_WORD_BITS,0,0,0>>& bitmap_init,
        hls::stream<ap_axiu<96,0,0,0>>& cmd_stream,
        hls::stream<ap_axiu<64,0,0,0>>& rsp_stream
    )
    {
        #pragma HLS INTERFACE s_axilite port=return bundle=control
        #pragma HLS INTERFACE axis port=bitmap_init
        #pragma HLS INTERFACE axis port=cmd_stream
        #pragma HLS INTERFACE axis port=rsp_stream

        #pragma HLS STREAM variable=bitmap_init depth=16
        #pragma HLS STREAM variable=cmd_stream depth=16
        #pragma HLS STREAM variable=rsp_stream depth=4

        #pragma HLS DATAFLOW

        alloc_free_worker(cmd_stream, rsp_stream, bitmap_init);
    }
}
