#include <cstdio>
#include <hls_stream.h>
#include "data_type.h"

const int TOTAL_BLOCKS = (CHANNEL_CAPACITY / INIT_VEC_LEN_IN_BYTE * CHANNELS_PER_PARTITION);
const int BITMAP_SIZE = (TOTAL_BLOCKS + 7) / 8;
const int MEM_PARTITIONS = 2;
const int NUM_ALLOC_STREAMS = 2;

struct MERGED_ALLOC_REQUEST
{
    ap_uint<96> data;
    ap_uint<1> last;
    ap_uint<1> target_vertex;
};

static void set_bits(int start, int len, BYTE* bitmap)
{
    #pragma HLS INLINE off
    for (int i = start; i < start + len; i++)
    {
        #pragma HLS UNROLL factor=1
        bitmap[i / 8] |= ((BYTE)1 << (i % 8));
    }
}

static bool is_free(int start, int len, int &first_set, BYTE* bitmap)
{
    #pragma HLS INLINE off
    for (int i = start; i < start + len; i++)
    {
        #pragma HLS UNROLL factor=1
        if (bitmap[i / 8] & (1 << (i % 8)))
        {
            first_set = i;
            return false;
        }
    }
    return true;
}

static void allocate(int &start, int len, BYTE* bitmap)
{
    #pragma HLS INLINE off
    start = -1;
    if (len == 0) return;
    int first_set = -1;
    for (int i = 0 ; i < TOTAL_BLOCKS ; i++)
    {
        #pragma HLS UNROLL factor=1
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

static void reset_bits(int start, int len, BYTE* bitmap)
{
    #pragma HLS INLINE off
    for (int i = start; i < start + len; i++)
    {
        #pragma HLS UNROLL factor=1
        bitmap[i / 8] &= ~((BYTE)1 << (i % 8));
    }
}

static void deallocate(int start, int len, BYTE* bitmap)
{
    #pragma HLS INLINE off
    if (len == 0) return;
    reset_bits(start, len, bitmap);
}

static void alloc_free_worker_tagged(
    hls::stream<MERGED_ALLOC_REQUEST>& req_in,
    hls::stream<VEC_INFO_STREAM>& vec_info_to_vertex,
    hls::stream<VEC_INFO_STREAM>& vec_info_to_edge,
    BYTE* bm_in_hbm
)
{
    #pragma HLS INTERFACE m_axi port=bm_in_hbm offset=slave bundle=gmem0

    BYTE storage[BITMAP_SIZE];
    #pragma HLS bind_storage variable=storage type=ram_t2p impl=uram

    for (int i = 0; i < BITMAP_SIZE; i++)
    {
        #pragma HLS PIPELINE II=1
        storage[i] = 0;
    }

    while (true)
    {
        #pragma HLS PIPELINE II=2
        MERGED_ALLOC_REQUEST pending_info;
        if (!req_in.read_nb(pending_info))
        {
            continue;
        }
        if (pending_info.last == 1)
        {
            break;
        }

        VEC_INFO back_info = 0;
        MEMORY_ALLOC_FREE_REQUEST_INTERNAL req_internal = pending_info.data;
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
            if (pending_info.target_vertex)
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
        else if (req_type == REQ_TYPE_LOAD_BITMAP)
        {
            for (int i = 0 ; i < BITMAP_SIZE ; i++)
            {
                #pragma HLS PIPELINE II=1
                #pragma HLS UNROLL factor=1
                storage[i] = bm_in_hbm[i];
            }
        }
    }
}

static void broadcast_control(
    hls::stream<COMMAND_STREAM>& signal_stream,
    hls::stream<COMMAND> control_out[MEM_PARTITIONS]
)
{
    while (true)
    {
        #pragma HLS PIPELINE II=1
        COMMAND_STREAM control_signal;
        if (!signal_stream.read_nb(control_signal))
        {
            continue;
        }
        for (int i = 0; i < MEM_PARTITIONS; ++i)
        {
            #pragma HLS UNROLL
            control_out[i].write(control_signal.data);
        }
        if (control_signal.data == MEM_STOP)
        {
            break;
        }
    }
}

static void merge_alloc_req_pair(
    hls::stream<COMMAND>& control_stream,
    hls::stream<MEMORY_ALLOC_FREE_REQUEST>& req_from_vertex,
    hls::stream<MEMORY_ALLOC_FREE_REQUEST>& req_from_edge,
    hls::stream<MERGED_ALLOC_REQUEST>& req_to_mem
)
{
    bool stopping = false;

    while (true)
    {
        #pragma HLS PIPELINE II=1

        if (!stopping && !control_stream.empty())
        {
            auto control_signal = control_stream.read();
            if (control_signal == MEM_STOP)
            {
                MERGED_ALLOC_REQUEST term;
                term.data = 0;
                term.last = 1;
                term.target_vertex = 0;
                req_to_mem.write(term);
                stopping = true;
            }
            else if (control_signal == LOAD_BITMAP)
            {
                MEMORY_ALLOC_FREE_REQUEST_INTERNAL req = 0;
                req.range(67, 64) = REQ_TYPE_LOAD_BITMAP;
                MERGED_ALLOC_REQUEST out_req;
                out_req.data = req;
                out_req.last = 0;
                out_req.target_vertex = 0;
                req_to_mem.write(out_req);
            }
        }

        if (stopping)
        {
            break;
        }

        
        if (!req_from_vertex.empty())
        {
            auto req_pkg = req_from_vertex.read();
            MERGED_ALLOC_REQUEST out;
            out.data = req_pkg.data;
            out.last = req_pkg.last;
            out.target_vertex = 1;
            req_to_mem.write(out);
            continue;
        }
        else if (!req_from_edge.empty())
        {
            auto req_pkg = req_from_edge.read();
            MERGED_ALLOC_REQUEST out;
            out.data = req_pkg.data;
            out.last = req_pkg.last;
            out.target_vertex = 0;
            req_to_mem.write(out);
            continue;
        }
        
    }
}

extern "C"
{
    void alloc_access_kernel(
        BYTE* bm_in_hbm_1,
        BYTE* bm_in_hbm_2,
        hls::stream<COMMAND_STREAM>& signal_stream,
        hls::stream<MEMORY_ALLOC_FREE_REQUEST>& req_from_vertex_1,
        hls::stream<MEMORY_ALLOC_FREE_REQUEST>& req_from_vertex_2,
        hls::stream<MEMORY_ALLOC_FREE_REQUEST>& req_from_edge_1,
        hls::stream<MEMORY_ALLOC_FREE_REQUEST>& req_from_edge_2,
        hls::stream<VEC_INFO_STREAM>& vec_info_to_vertex_1,
        hls::stream<VEC_INFO_STREAM>& vec_info_to_vertex_2,
        hls::stream<VEC_INFO_STREAM>& vec_info_to_edge_1,
        hls::stream<VEC_INFO_STREAM>& vec_info_to_edge_2
    )
    {
        #pragma HLS INTERFACE m_axi port=bm_in_hbm_1 offset=slave bundle=gmem0
        #pragma HLS INTERFACE m_axi port=bm_in_hbm_2 offset=slave bundle=gmem1
        #pragma HLS INTERFACE axis port=signal_stream
        #pragma HLS INTERFACE axis port=req_from_vertex_1
        #pragma HLS INTERFACE axis port=req_from_vertex_2
        #pragma HLS INTERFACE axis port=req_from_edge_1
        #pragma HLS INTERFACE axis port=req_from_edge_2
        #pragma HLS INTERFACE axis port=vec_info_to_vertex_1
        #pragma HLS INTERFACE axis port=vec_info_to_vertex_2
        #pragma HLS INTERFACE axis port=vec_info_to_edge_1
        #pragma HLS INTERFACE axis port=vec_info_to_edge_2
        #pragma HLS STREAM variable=signal_stream depth=4
        #pragma HLS STREAM variable=req_from_vertex_1 depth=16
        #pragma HLS STREAM variable=req_from_vertex_2 depth=16
        #pragma HLS STREAM variable=req_from_edge_1 depth=16
        #pragma HLS STREAM variable=req_from_edge_2 depth=16
        #pragma HLS STREAM variable=vec_info_to_vertex_1 depth=4
        #pragma HLS STREAM variable=vec_info_to_vertex_2 depth=4
        #pragma HLS STREAM variable=vec_info_to_edge_1 depth=4
        #pragma HLS STREAM variable=vec_info_to_edge_2 depth=4

        #pragma HLS DATAFLOW

        hls::stream<COMMAND> control_to_merge[MEM_PARTITIONS];
        #pragma HLS stream variable=control_to_merge depth=4

        hls::stream<MERGED_ALLOC_REQUEST> req_to_mem[MEM_PARTITIONS];
        #pragma HLS stream variable=req_to_mem depth=32

        BYTE* bm_ptrs[MEM_PARTITIONS] = {
            bm_in_hbm_1, bm_in_hbm_2
        };

        hls::stream<VEC_INFO_STREAM>* vec_info_to_vertex[NUM_ALLOC_STREAMS] = {
            &vec_info_to_vertex_1, &vec_info_to_vertex_2
        };
        hls::stream<VEC_INFO_STREAM>* vec_info_to_edge[NUM_ALLOC_STREAMS] = {
            &vec_info_to_edge_1, &vec_info_to_edge_2
        };

        broadcast_control(signal_stream, control_to_merge);

        merge_alloc_req_pair(control_to_merge[0], req_from_vertex_1, req_from_edge_1, req_to_mem[0]);
        merge_alloc_req_pair(control_to_merge[1], req_from_vertex_2, req_from_edge_2, req_to_mem[1]);


        for (int i = 0; i < MEM_PARTITIONS; ++i)
        {
            #pragma HLS UNROLL
            alloc_free_worker_tagged(
                req_to_mem[i],
                *vec_info_to_vertex[i],
                *vec_info_to_edge[i],
                bm_ptrs[i]
            );
        }
    }
}
