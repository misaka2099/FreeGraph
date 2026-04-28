#include "ap_axi_sdata.h"
#include "data_type.h"
#include "debug.h"
#include "etc/autopilot_ssdm_op.h"
#include "hls_stream.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
void process_req(
    ap_uint<512>* hbm_ptr,
    VEC_INFO* pma_addrs,
    int max_pma_idx,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_edge,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_read,
    hls::stream<ap_axiu<512,0,0, 0>> &data_from_edge,
    hls::stream<ap_axiu<512,0,0, 0>> &data_to_edge,
    hls::stream<ap_axiu<512,0,0, 0>> &data_to_read
)
{
    bool finish_edge = false;
    bool finish_read = false;
    int currect_max_pma_idx = max_pma_idx;
    while (true)
    {
        if (finish_edge && finish_read)
        {
            break;
        }

        MEMORY_INFO_ACCESS_COMMAND cmd;
        int turn = -1;
        if (req_from_edge.read_nb(cmd))   turn = 1;
        else if (req_from_read.read_nb(cmd))   turn = 2;
        else continue;

        if (cmd.command_type == MEMORY_FINISH)
        {
            if (turn == 1)
                finish_edge = true;
            else if (turn == 2)
                finish_read = true;
            continue;
        }

        switch (cmd.command_type) 
        {
            case MEMORY_WRITE:
            {
                int start = cmd.start;
                int len = cmd.len;
                //512bit -> 64byte
                for (int i = 0 ; i < len / 64 ; i++)
                {
                    ap_axiu<512, 0, 0, 0> pkt;
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
                int pma_idx = cmd.pma_idx;
                VEC_INFO vec;
                vec.range(31,0) = cmd.start;
                vec.range(63,32) = cmd.len;
                pma_addrs[pma_idx] = vec;
                if (pma_idx > currect_max_pma_idx)
                {
                    currect_max_pma_idx = pma_idx;
                }
                break;
            }
            case VEC_INFO_READ:
            {
                int pma_idx = cmd.pma_idx;
                VEC_INFO vec_info = pma_addrs[pma_idx];
                if (turn == 1)
                    data_to_edge.write({.data = vec_info});
                else if (turn == 2)
                    data_to_read.write({.data = vec_info});
                break;
            }
            case PMA_IDX_MAX_QUERY:
            {
                if (turn == 2)
                {
                    ap_axiu<512,0,0,0> pkt;
                    pkt.data = currect_max_pma_idx;
                    pkt.last = 1;
                    data_to_read.write(pkt);
                }
                break;
            }
            default:
                break;
        }
        
    }
}

extern "C"
{
    void mem_access_kernel(
        ap_uint<512>* hbm_ptr,
        VEC_INFO* pma_addrs,
        int max_pma_idx,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_edge,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_from_read,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_edge,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_edge,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_read
    )
    {
        #pragma HLS INTERFACE m_axi port=hbm_ptr offset=slave bundle=gmem1
        #pragma HLS INTERFACE m_axi port=pma_addrs offset=slave bundle=gmem2
        // #pragma HLS INTERFACE m_axi port=neg_pma_addrs offset=slave bundle=gmem17
        
        #pragma HLS INTERFACE s_axilite port=hbm_ptr bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_addrs bundle=control
        // #pragma HLS INTERFACE s_axilite port=neg_pma_addrs bundle=control
        #pragma HLS INTERFACE s_axilite port=return bundle=control
        #pragma HLS INTERFACE s_axilite port=max_pma_idx bundle=control
        #pragma HLS INTERFACE axis port=req_from_edge
        #pragma HLS INTERFACE axis port=req_from_read
        #pragma HLS INTERFACE axis port=data_from_edge
        #pragma HLS INTERFACE axis port=data_to_edge
        #pragma HLS INTERFACE axis port=data_to_read
        #pragma HLS DATAFLOW 

        process_req(
            hbm_ptr,
            pma_addrs,
            max_pma_idx,
            req_from_edge,
            req_from_read,
            data_from_edge,
            data_to_edge,
            data_to_read
        );

    }
}