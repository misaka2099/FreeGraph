#include <cstdio>
#include <hls_stream.h>
#include "data_type.h"

const int TOTAL_BLOCKS = (CHANNEL_CAPACITY / INIT_VEC_LEN_IN_BYTE);
const int BITMAP_SIZE = (TOTAL_BLOCKS + 7) / 8;
const int BITMAP_WORD_BITS = 512;
const int BITMAP_WORD_BYTES = BITMAP_WORD_BITS / 8;
const int BITMAP_WORDS = (TOTAL_BLOCKS + BITMAP_WORD_BITS - 1) / BITMAP_WORD_BITS;

static bool get_bit(int idx, ap_uint<BITMAP_WORD_BITS>* bitmap)
{
    #pragma HLS INLINE
    int word_idx = idx / BITMAP_WORD_BITS;
    int bit_idx = idx % BITMAP_WORD_BITS;
    return bitmap[word_idx][bit_idx] == 1;
}

static void set_bit(int idx, ap_uint<BITMAP_WORD_BITS>* bitmap)
{
    #pragma HLS INLINE
    int word_idx = idx / BITMAP_WORD_BITS;
    int bit_idx = idx % BITMAP_WORD_BITS;
    bitmap[word_idx][bit_idx] = 1;
}

static void clear_bit(int idx, ap_uint<BITMAP_WORD_BITS>* bitmap)
{
    #pragma HLS INLINE
    int word_idx = idx / BITMAP_WORD_BITS;
    int bit_idx = idx % BITMAP_WORD_BITS;
    bitmap[word_idx][bit_idx] = 0;
}

static void set_bits(int start, int len, ap_uint<BITMAP_WORD_BITS>* bitmap)
{
    #pragma HLS INLINE
    for (int i = start; i < start + len; i++)
    {
        #pragma HLS LOOP_TRIPCOUNT min=1 max=4096
        set_bit(i, bitmap);
    }
}

static bool is_free(int start, int len, int &first_set, ap_uint<BITMAP_WORD_BITS>* bitmap)
{
    #pragma HLS INLINE
    for (int i = start; i < start + len; i++)
    {
        #pragma HLS LOOP_TRIPCOUNT min=1 max=4096
        if (get_bit(i, bitmap))
        {
            first_set = i;
            return false;
        }
    }
    return true;
}

static void allocate(int &start, int len, ap_uint<BITMAP_WORD_BITS>* bitmap)
{
    #pragma HLS INLINE
    start = -1;
    if (len == 0) return;
    int first_set = -1;
    for (int i = 0 ; i < TOTAL_BLOCKS ; i++)
    {
        #pragma HLS LOOP_TRIPCOUNT min=1 max=256000
        if (is_free(i, len, first_set, bitmap))
        {
            start = i;
            set_bits(start, len, bitmap);
            return;
        }
        else
        {
            i = first_set;
        }
    }
}

static void reset_bits(int start, int len, ap_uint<BITMAP_WORD_BITS>* bitmap)
{
    #pragma HLS INLINE
    for (int i = start; i < start + len; i++)
    {
        #pragma HLS LOOP_TRIPCOUNT min=1 max=4096
        clear_bit(i, bitmap);
    }
}

static void deallocate(int start, int len, ap_uint<BITMAP_WORD_BITS>* bitmap)
{
    #pragma HLS INLINE
    if (len == 0) return;
    reset_bits(start, len, bitmap);
}

static void alloc_free_worker(
    hls::stream<COMMAND_STREAM>& signal_stream,
    hls::stream<MEMORY_ALLOC_FREE_REQUEST>& req_vertex_in,
    hls::stream<MEMORY_ALLOC_FREE_REQUEST>& req_edge_in,
    hls::stream<VEC_INFO_STREAM>& vec_info_to_vertex,
    hls::stream<VEC_INFO_STREAM>& vec_info_to_edge,
    BYTE* bm_in_hbm
)
{
    #pragma HLS INTERFACE m_axi port=bm_in_hbm offset=slave bundle=gmem0

    ap_uint<BITMAP_WORD_BITS> storage[BITMAP_WORDS];
    #pragma HLS bind_storage variable=storage type=ram_t2p impl=bram

    for (int i = 0; i < BITMAP_WORDS; i++)
    {
        #pragma HLS PIPELINE II=1
        storage[i] = 0;
    }

    while (true)
    {
        #pragma HLS PIPELINE II=2
        MEMORY_ALLOC_FREE_REQUEST pending_info;
        COMMAND_STREAM control_signal;
        bool from_vertex = false;
        bool valid_req = false;
        
        if (signal_stream.read_nb(control_signal))
        {
            if (control_signal.data == MEM_STOP)
            {
                break;
            }
            else if (control_signal.data == REQ_TYPE_LOAD_BITMAP)
            {
                for (int i = 0 ; i < BITMAP_WORDS ; i++)
                {
                    #pragma HLS PIPELINE II=1
                    ap_uint<BITMAP_WORD_BITS> word = 0;
                    for (int b = 0; b < BITMAP_WORD_BYTES; ++b)
                    {
                        int byte_idx = i * BITMAP_WORD_BYTES + b;
                        BYTE v = (byte_idx < BITMAP_SIZE) ? bm_in_hbm[byte_idx] : (BYTE)0;
                        word.range((b + 1) * 8 - 1, b * 8) = v;
                    }
                    storage[i] = word;
                }
            }
        }
        else if (req_vertex_in.read_nb(pending_info))
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
            continue;
        }
        if (pending_info.last == 1)
        {
            break;
        }

        VEC_INFO back_info = 0;
        auto req_internal = pending_info.data;
        int req_type = req_internal.range(67, 64);

        if (req_type == REQ_TYPE_ALLOC)
        {
            int req_bytes = pending_info.data.range(63,32);
            int len_segs = (req_bytes + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
            int start_seg;
            allocate(start_seg, len_segs, storage);
            if (start_seg == -1)
            {
                perror("ERROR INSUFFICIENT MEMORY POOL\n");
            }
            int start_bytes = start_seg * INIT_VEC_LEN_IN_BYTE;
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
        else if (req_type == REQ_TYPE_FREE)
        {
            int start_bytes = pending_info.data.range(31,0);
            int len_bytes = pending_info.data.range(63,32);
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
        hls::stream<COMMAND_STREAM>& signal_stream,
        hls::stream<MEMORY_ALLOC_FREE_REQUEST>& req_from_vertex,
        hls::stream<MEMORY_ALLOC_FREE_REQUEST>& req_from_edge,
        hls::stream<VEC_INFO_STREAM>& vec_info_to_vertex,
        hls::stream<VEC_INFO_STREAM>& vec_info_to_edge
    )
    {
        #pragma HLS INTERFACE m_axi port=bm_in_hbm offset=slave bundle=gmem0
        #pragma HLS INTERFACE axis port=signal_stream
        #pragma HLS INTERFACE axis port=req_from_vertex
        #pragma HLS INTERFACE axis port=req_from_edge
        #pragma HLS INTERFACE axis port=vec_info_to_vertex
        #pragma HLS INTERFACE axis port=vec_info_to_edge
        #pragma HLS STREAM variable=signal_stream depth=4
        #pragma HLS STREAM variable=req_from_vertex depth=16
        #pragma HLS STREAM variable=req_from_edge depth=16
        #pragma HLS STREAM variable=vec_info_to_vertex depth=4
        #pragma HLS STREAM variable=vec_info_to_edge depth=4

        #pragma HLS DATAFLOW

        alloc_free_worker(signal_stream, req_from_vertex, req_from_edge, vec_info_to_vertex, vec_info_to_edge, bm_in_hbm);
    }
}
