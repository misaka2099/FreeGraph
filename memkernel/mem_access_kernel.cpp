#include "ap_axi_sdata.h"
#include "data_type.h"
#include "debug.h"
#include "etc/autopilot_ssdm_op.h"
#include "hls_stream.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
const int MEM_CHANNELS = 2;
const int WORKER_NUM = (MEM_CHANNELS + 1);
enum vec_info_access_type
{
    READ,
    WRITE
};
struct vec_info_access_command 
{
    int pma_idx;
    // ap_uint<1> direction; // 0: out, 1: in
    VEC_INFO vec_info;
    vec_info_access_type command_type;
};
void vec_info_merger(
    VEC_INFO* pos_pma_addrs,
    // VEC_INFO* neg_pma_addrs,
    hls::stream<vec_info_access_command> vec_info_to_merger[MEM_CHANNELS],
    hls::stream<VEC_INFO> vec_info_from_merger[MEM_CHANNELS],
    hls::stream<COMMAND> &stop_signal
)
{
    bool stop_requested = false;
    while (true)
    {
        #pragma HLS PIPELINE II=1
        if (!stop_requested)
        {
            COMMAND cmd;
            if (stop_signal.read_nb(cmd) && cmd == MEM_STOP)
            {
                stop_requested = true;
            }
        }
        bool pending_work = false;
        for (int i = 0; i < MEM_CHANNELS; ++i)
        {
            if (!vec_info_to_merger[i].empty())
            {
                pending_work = true;
                break;
            }
        }
        if (stop_requested && !pending_work)
        {
            break;
        }
        vec_info_access_command cmd;
        bool handled = false;
        for (int i = 0; i < MEM_CHANNELS; ++i)
        {
            if (vec_info_to_merger[i].read_nb(cmd))
            {
                handled = true;
                if (cmd.command_type == WRITE)
                {
                    // Direction is ignored; always use positive (OUT) table
                    pos_pma_addrs[cmd.pma_idx] = cmd.vec_info;
                }
                else
                {
                    VEC_INFO vec_info = pos_pma_addrs[cmd.pma_idx];
                    vec_info_from_merger[i].write(vec_info);
                }
                break;
            }
        }
        if (!handled)
        {
            continue;
        }
    }
}
void process_req(
    ap_uint<512>* hbm_ptr,
    hls::stream<vec_info_access_command> &vec_info_to_merger,
    hls::stream<VEC_INFO> &vec_info_from_merger,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_vertex,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_edge,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_read,
    hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_vertex,
    hls::stream<ap_axiu<512,0,0, 0>> &data_from_edge,
    hls::stream<ap_axiu<512,0,0, 0>> &data_to_edge,
    hls::stream<ap_axiu<512,0,0, 0>> &data_to_read,
    // hls::stream<VEC_INFO_STREAM>& vec_info_to_edge,
    // hls::stream<VEC_INFO_STREAM>& vec_info_to_read,
    hls::stream<COMMAND> &stop_signal
)
{
    bool stop_requested = false;
    while (true)
    {
        #pragma HLS PIPELINE II=1
        if (!stop_requested)
        {
            COMMAND cmd_stop;
            if (stop_signal.read_nb(cmd_stop) && cmd_stop == MEM_STOP)
            {
                stop_requested = true;
            }
        }
        bool no_inflight_reqs = req_from_vertex.empty() && req_from_edge.empty() && req_from_read.empty();
        if (stop_requested && no_inflight_reqs)
        {
            break;
        }
        MEMORY_INFO_ACCESS_COMMAND cmd;
        int turn = -1;
        if (req_from_vertex.read_nb(cmd))      turn = 0;
        else if (req_from_edge.read_nb(cmd))   turn = 1;
        else if (req_from_read.read_nb(cmd))   turn = 2;
        else continue;
        switch (cmd.command_type) 
        {
            case MEMORY_WRITE:
            {
                int start = cmd.start;
                int len = cmd.len;
                //512bit -> 64byte
                for (int i = 0 ; i < len / 64 ; i++)
                {
                    #pragma HLS PIPELINE II=1
                    ap_axiu<512, 0, 0, 0> pkt;
                    if (turn == 0)
                        pkt = data_from_vertex.read();
                    else
                        pkt = data_from_edge.read();
                    // else
                    //     pkt = data_from_edge.read();
                    hbm_ptr[(start / 64) + i] = pkt.data;
                }
                break;
            }
            case MEMORY_READ:
            {
                int start = cmd.start;
                int len = cmd.len;
                for (int i = 0 ; i < len / 64 ; i++)
                {
                    #pragma HLS PIPELINE II=1
                    auto pkt = hbm_ptr[(start / 64) + i];
                    ap_axiu<512,0,0,0> out_pkt;
                    out_pkt.data = pkt;
                    out_pkt.last = (i == (len / 64 - 1)) ? 1 : 0;
                    if (turn == 1)
                        data_to_edge.write(out_pkt);
                    else if (turn == 2)
                        data_to_read.write(out_pkt);
                }
                break;
            }
            case VEC_INFO_WRITE:
            {
                vec_info_access_command to_write;
                int pma_idx = cmd.pma_idx;
                to_write.pma_idx = pma_idx;
                // to_write.direction = OUT_DIRECTION;
                to_write.command_type = WRITE;
                VEC_INFO vec;
                vec.range(31,0) = cmd.start;
                vec.range(63,32) = cmd.len;
                to_write.vec_info = vec;
                vec_info_to_merger.write(to_write);
                break;
            }
            case VEC_INFO_READ:
            {
                vec_info_access_command to_read;
                int pma_idx = cmd.pma_idx;
                to_read.pma_idx = pma_idx;
                // to_read.direction = OUT_DIRECTION;
                to_read.command_type = READ;
                vec_info_to_merger.write(to_read);
                #pragma HLS protocol fixed
                ap_wait();
                VEC_INFO vec_info_pkt = vec_info_from_merger.read();
                if (turn == 1)
                    data_to_edge.write({.data = vec_info_pkt});
                else if (turn == 2)
                    data_to_read.write({.data = vec_info_pkt});
                break;
            }
            default:
                break;
        }
        
    }
}

void exit_handle(
    hls::stream<COMMAND_STREAM>& stop_signal,
    hls::stream<COMMAND> signal_to_worker[WORKER_NUM]
)
{
    while (true) {
        COMMAND_STREAM cmd;
        if (stop_signal.read_nb(cmd)) {
            if (cmd.data != MEM_STOP) continue;
            for (int i = 0; i < WORKER_NUM; i++) 
            {
                COMMAND stop;
                stop = MEM_STOP;
                signal_to_worker[i].write(stop);
            }
            break;
        }
    }
}
extern "C"
{
    void mem_access_kernel(
        ap_uint<512>* hbm_ptr_1,
        ap_uint<512>* hbm_ptr_2,
        VEC_INFO* pos_pma_addrs,
        // VEC_INFO* neg_pma_addrs,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_vertex_1,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_vertex_2,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_edge_1,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_edge_2,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_read_1,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_read_2,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_vertex_1,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_vertex_2,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_edge_1,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_edge_2,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_edge_1,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_edge_2,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_read_1,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_read_2,
        // hls::stream<VEC_INFO_STREAM>& vec_info_to_edge_1,
        // hls::stream<VEC_INFO_STREAM>& vec_info_to_edge_2,
        // hls::stream<VEC_INFO_STREAM>& vec_info_to_read_1,
        // hls::stream<VEC_INFO_STREAM>& vec_info_to_read_2,
        hls::stream<COMMAND_STREAM>& signal_stream
    )
    {
        #pragma HLS INTERFACE m_axi port=hbm_ptr_1 offset=slave bundle=gmem1
        #pragma HLS INTERFACE m_axi port=hbm_ptr_2 offset=slave bundle=gmem2
        #pragma HLS INTERFACE m_axi port=hbm_ptr_2 offset=slave bundle=gmem2
        #pragma HLS INTERFACE m_axi port=pos_pma_addrs offset=slave bundle=gmem17
        // #pragma HLS INTERFACE m_axi port=neg_pma_addrs offset=slave bundle=gmem17
        
        #pragma HLS INTERFACE s_axilite port=hbm_ptr_1 bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_ptr_2 bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_ptr_2 bundle=control
        #pragma HLS INTERFACE s_axilite port=pos_pma_addrs bundle=control
        // #pragma HLS INTERFACE s_axilite port=neg_pma_addrs bundle=control
        #pragma HLS INTERFACE s_axilite port=return bundle=control

        #pragma HLS INTERFACE axis port=req_from_vertex_1 depth=4
        #pragma HLS INTERFACE axis port=req_from_vertex_2 depth=4
        #pragma HLS INTERFACE axis port=req_from_edge_1 depth=4
        #pragma HLS INTERFACE axis port=req_from_edge_2 depth=4
        #pragma HLS INTERFACE axis port=req_from_read_1 depth=4
        #pragma HLS INTERFACE axis port=req_from_read_2 depth=4
        #pragma HLS INTERFACE axis port=data_from_vertex_1 depth=16
        #pragma HLS INTERFACE axis port=data_from_vertex_2 depth=16
        #pragma HLS INTERFACE axis port=data_from_edge_1 depth=16
        #pragma HLS INTERFACE axis port=data_from_edge_2 depth=16
        #pragma HLS INTERFACE axis port=data_to_edge_1 depth=16
        #pragma HLS INTERFACE axis port=data_to_edge_2 depth=16
        #pragma HLS INTERFACE axis port=data_to_read_1 depth=16
        #pragma HLS INTERFACE axis port=data_to_read_2 depth=16
        // #pragma HLS INTERFACE axis port=vec_info_to_edge_1 depth=4
        // #pragma HLS INTERFACE axis port=vec_info_to_edge_2 depth=4
        // #pragma HLS INTERFACE axis port=vec_info_to_read_1 depth=4
        // #pragma HLS INTERFACE axis port=vec_info_to_read_2 depth=4
        #pragma HLS INTERFACE axis port=signal_stream
        #pragma HLS DATAFLOW 
        ap_uint<512>* hbm_ptrs[MEM_CHANNELS] = {
            hbm_ptr_1, hbm_ptr_2
        };
        hls::stream<MEMORY_INFO_ACCESS_COMMAND>* req_from_vertex[MEM_CHANNELS] = {
            &req_from_vertex_1, &req_from_vertex_2
        };
        hls::stream<MEMORY_INFO_ACCESS_COMMAND>* req_from_edge[MEM_CHANNELS] = {
            &req_from_edge_1, &req_from_edge_2
        };
        hls::stream<MEMORY_INFO_ACCESS_COMMAND>* req_from_read[MEM_CHANNELS] = {
            &req_from_read_1, &req_from_read_2
        };
        hls::stream<ap_axiu<512, 0, 0, 0>>* data_from_vertex[MEM_CHANNELS] = {
            &data_from_vertex_1, &data_from_vertex_2
        };
        hls::stream<ap_axiu<512, 0, 0, 0>>* data_from_edge[MEM_CHANNELS] = {
            &data_from_edge_1, &data_from_edge_2
        };
        hls::stream<ap_axiu<512, 0, 0, 0>>* data_to_edge[MEM_CHANNELS] = {
            &data_to_edge_1, &data_to_edge_2
        };
        hls::stream<ap_axiu<512, 0, 0, 0>>* data_to_read[MEM_CHANNELS] = {
            &data_to_read_1, &data_to_read_2
        };
        // hls::stream<VEC_INFO_STREAM>* vec_info_to_edge[MEM_CHANNELS] = {
        //     &vec_info_to_edge_1, &vec_info_to_edge_2
        // };
        // hls::stream<VEC_INFO_STREAM>* vec_info_to_read[MEM_CHANNELS] = {
        //     &vec_info_to_read_1, &vec_info_to_read_2
        // };

        hls::stream<vec_info_access_command> vec_info_to_merger[MEM_CHANNELS];
        #pragma HLS stream variable=vec_info_to_merger depth=32
        hls::stream<VEC_INFO> vec_info_from_merger[MEM_CHANNELS];
        #pragma HLS stream variable=vec_info_from_merger depth=32
        hls::stream<COMMAND> signal_to_worker[WORKER_NUM];
        #pragma HLS stream variable=signal_to_worker depth=2

        exit_handle(signal_stream, signal_to_worker);

        for (int i = 0; i < MEM_CHANNELS; ++i)
        {
            #pragma HLS UNROLL
            process_req(
                hbm_ptrs[i],
                vec_info_to_merger[i],
                vec_info_from_merger[i],
                *req_from_vertex[i],
                *req_from_edge[i],
                *req_from_read[i],
                *data_from_vertex[i],
                *data_from_edge[i],
                *data_to_edge[i],
                *data_to_read[i],
                // *vec_info_to_edge[i],
                // *vec_info_to_read[i],
                signal_to_worker[i]
            );
        }

        vec_info_merger(
            pos_pma_addrs,
            // neg_pma_addrs,
            vec_info_to_merger,
            vec_info_from_merger,
            signal_to_worker[MEM_CHANNELS]
        );
    }
}