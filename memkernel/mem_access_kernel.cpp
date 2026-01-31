#include "ap_axi_sdata.h"
#include "data_type.h"
#include "debug.h"
#include "etc/autopilot_ssdm_op.h"
#include "hls_stream.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
const int WORKER_NUM = (2+1);
enum vec_info_access_type
{
    READ,
    WRITE
};
struct vec_info_access_command 
{
    int pma_idx;
    ap_uint<1> direction; // 0: out, 1: in
    VEC_INFO vec_info;
    vec_info_access_type command_type;
};
void vec_info_merger(
    VEC_INFO* pos_pma_addrs,
    VEC_INFO* neg_pma_addrs,
    hls::stream<vec_info_access_command> &vec_info_to_merger_1,
    hls::stream<VEC_INFO> &vec_info_from_merger_1,
    hls::stream<vec_info_access_command> &vec_info_to_merger_2,
    hls::stream<VEC_INFO> &vec_info_from_merger_2,
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
        bool pending_work = !(vec_info_to_merger_1.empty() && vec_info_to_merger_2.empty());
        if (stop_requested && !pending_work)
        {
            break;
        }
        vec_info_access_command cmd;
        if (vec_info_to_merger_1.read_nb(cmd))
        {
            if (cmd.command_type == WRITE)
            {
                if (cmd.direction == OUT_DIRECTION)
                {
                    pos_pma_addrs[cmd.pma_idx] = cmd.vec_info;
                }
                else
                {
                    neg_pma_addrs[cmd.pma_idx] = cmd.vec_info;
                }
            }
            else // READ
            {
                VEC_INFO vec_info;
                if (cmd.direction == OUT_DIRECTION)
                {
                    vec_info = pos_pma_addrs[cmd.pma_idx];
                }
                else
                {
                    vec_info = neg_pma_addrs[cmd.pma_idx];
                }
                vec_info_from_merger_1.write(vec_info);
            }
        }
        else if (vec_info_to_merger_2.read_nb(cmd))
        {
            if (cmd.command_type == WRITE)
            {
                if (cmd.direction == OUT_DIRECTION)
                {
                    pos_pma_addrs[cmd.pma_idx] = cmd.vec_info;
                }
                else
                {
                    neg_pma_addrs[cmd.pma_idx] = cmd.vec_info;
                }
            }
            else // READ
            {
                VEC_INFO vec_info;
                if (cmd.direction == OUT_DIRECTION)
                {
                    vec_info = pos_pma_addrs[cmd.pma_idx];
                }
                else
                {
                    vec_info = neg_pma_addrs[cmd.pma_idx];
                }
                vec_info_from_merger_2.write(vec_info);
            }
        }
    }
}
void process_req(
    ap_uint<512>* hbm_ptr,
    hls::stream<vec_info_access_command> &vec_info_to_merger,
    hls::stream<VEC_INFO> &vec_info_from_merger,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_vertex,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_edge,
    hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_vertex,
    hls::stream<ap_axiu<512,0,0, 0>> &data_from_edge,
    hls::stream<ap_axiu<512,0,0, 0>> &data_to_edge,
    hls::stream<VEC_INFO_STREAM>& vec_info_to_edge,
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
        bool no_inflight_reqs = req_from_vertex.empty() && req_from_edge.empty();
        if (stop_requested && no_inflight_reqs)
        {
            break;
        }
        MEMORY_INFO_ACCESS_COMMAND cmd;
        int turn = 0;
        if (req_from_vertex.read_nb(cmd))  turn = 0;
        else if (req_from_edge.read_nb(cmd)) turn = 1;
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
                    if (turn == 0)
                    {
                        // data_to_vertex.write(out_pkt);
                    }
                    else
                        data_to_edge.write(out_pkt);
                }
                break;
            }
            case POS_VEC_INFO_WRITE:
            {
                vec_info_access_command to_write;
                int pma_idx = cmd.pma_idx;
                to_write.pma_idx = pma_idx;
                to_write.direction = OUT_DIRECTION;
                to_write.command_type = WRITE;
                VEC_INFO vec;
                vec.range(31,0) = cmd.start;
                vec.range(63,32) = cmd.len;
                to_write.vec_info = vec;
                vec_info_to_merger.write(to_write);
                break;
            }
            case NEG_VEC_INFO_WRITE:
            {
                vec_info_access_command to_write;
                int pma_idx = cmd.pma_idx;
                to_write.pma_idx = pma_idx;
                to_write.direction = IN_DIRECTION;
                to_write.command_type = WRITE;
                VEC_INFO vec;
                vec.range(31,0) = cmd.start;
                vec.range(63,32) = cmd.len;
                to_write.vec_info = vec;
                vec_info_to_merger.write(to_write);
                break;
            }
            case POS_VEC_INFO_READ:
            {
                vec_info_access_command to_read;
                int pma_idx = cmd.pma_idx;
                to_read.pma_idx = pma_idx;
                to_read.direction = OUT_DIRECTION;
                to_read.command_type = READ;
                vec_info_to_merger.write(to_read);
                #pragma HLS protocol fixed
                ap_wait();
                VEC_INFO vec_info_pkt = vec_info_from_merger.read();
                if (turn == 1)
                    vec_info_to_edge.write({.data = vec_info_pkt});
                break;
            }
            case NEG_VEC_INFO_READ:
            {
                vec_info_access_command to_read;
                int pma_idx = cmd.pma_idx;
                to_read.pma_idx = pma_idx;
                to_read.direction = IN_DIRECTION;
                to_read.command_type = READ;
                vec_info_to_merger.write(to_read);
                #pragma HLS protocol fixed
                ap_wait();
                VEC_INFO vec_info_pkt = vec_info_from_merger.read();
                if (turn == 1)
                    vec_info_to_edge.write({.data = vec_info_pkt});
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
        // ap_uint<512>* hbm_ptr_3,
        // ap_uint<512>* hbm_ptr_4,
        // ap_uint<512>* hbm_ptr_5,
        // ap_uint<512>* hbm_ptr_6,
        // ap_uint<512>* hbm_ptr_7,
        // ap_uint<512>* hbm_ptr_8,
        VEC_INFO* pos_pma_addrs,
        VEC_INFO* neg_pma_addrs,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_vertex_1,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_vertex_2,
        // hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_vertex_3,
        // hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_vertex_4,
        // hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_vertex_5,
        // hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_vertex_6,
        // hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_vertex_7,
        // hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_vertex_8,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_edge_1,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_edge_2,   
        // hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_edge_3,
        // hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_edge_4,
        // hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_edge_5,
        // hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_edge_6,
        // hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_edge_7,
        // hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_edge_8,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_vertex_1,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_vertex_2,

        hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_edge_1,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_edge_2,
        
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_edge_1,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_edge_2,
        hls::stream<VEC_INFO_STREAM>& vec_info_to_edge_1,
        hls::stream<VEC_INFO_STREAM>& vec_info_to_edge_2,
        hls::stream<COMMAND_STREAM>& signal_stream
    )
    {
        #pragma HLS INTERFACE m_axi port=hbm_ptr_1 offset=slave bundle=gmem1
        #pragma HLS INTERFACE m_axi port=hbm_ptr_2 offset=slave bundle=gmem2
        #pragma HLS INTERFACE m_axi port=pos_pma_addrs offset=slave bundle=gmem3
        #pragma HLS INTERFACE m_axi port=neg_pma_addrs offset=slave bundle=gmem3
        
        #pragma HLS INTERFACE s_axilite port=hbm_ptr_1 bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_ptr_2 bundle=control
        #pragma HLS INTERFACE s_axilite port=pos_pma_addrs bundle=control
        #pragma HLS INTERFACE s_axilite port=neg_pma_addrs bundle=control
        #pragma HLS INTERFACE s_axilite port=return bundle=control

        #pragma HLS INTERFACE axis port=req_from_vertex_1
        #pragma HLS INTERFACE axis port=req_from_vertex_2
        #pragma HLS INTERFACE axis port=req_from_edge_1
        #pragma HLS INTERFACE axis port=req_from_edge_2
        
        #pragma HLS INTERFACE axis port=data_from_vertex_1
        #pragma HLS INTERFACE axis port=data_from_vertex_2
        
        #pragma HLS INTERFACE axis port=data_from_edge_1
        #pragma HLS INTERFACE axis port=data_from_edge_2
        
        #pragma HLS INTERFACE axis port=data_to_edge_1
        #pragma HLS INTERFACE axis port=data_to_edge_2
        #pragma HLS INTERFACE axis port=vec_info_to_edge_1
        #pragma HLS INTERFACE axis port=vec_info_to_edge_2
        #pragma HLS INTERFACE axis port=signal_stream
        #pragma HLS DATAFLOW 
        hls::stream<vec_info_access_command> vec_info_to_merger_1;
        #pragma HLS stream variable=vec_info_to_merger_1 depth=32
        hls::stream<VEC_INFO> vec_info_from_merger_1;
        #pragma HLS stream variable=vec_info_from_merger_1 depth=32
        hls::stream<vec_info_access_command> vec_info_to_merger_2;
        #pragma HLS stream variable=vec_info_to_merger_2 depth=32
        hls::stream<VEC_INFO> vec_info_from_merger_2;
        #pragma HLS stream variable=vec_info_from_merger_2 depth=32
        hls::stream<COMMAND> signal_to_worker[WORKER_NUM];
        #pragma HLS stream variable=signal_to_worker depth=2

        exit_handle(signal_stream, signal_to_worker);

        process_req(
            hbm_ptr_1,
            vec_info_to_merger_1,
            vec_info_from_merger_1,
            req_from_vertex_1,
            req_from_edge_1,
            data_from_vertex_1,
            data_from_edge_1,
            data_to_edge_1,
            vec_info_to_edge_1,
            signal_to_worker[0]
        );

        process_req(
            hbm_ptr_2,
            vec_info_to_merger_2,
            vec_info_from_merger_2,
            req_from_vertex_2,
            req_from_edge_2,
            data_from_vertex_2,
            data_from_edge_2,
            data_to_edge_2,
            vec_info_to_edge_2,
            signal_to_worker[1]
        );

        vec_info_merger(
            pos_pma_addrs,
            neg_pma_addrs,
            vec_info_to_merger_1,
            vec_info_from_merger_1,
            vec_info_to_merger_2,
            vec_info_from_merger_2,
            signal_to_worker[2]
        );
    }
}