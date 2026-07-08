#include <cstdio>
#include <hls_stream.h>
#include <ap_int.h> 
#include "data_type.h"
#include "debug.h"
#include "etc/autopilot_ssdm_op.h"
extern "C"
{
    void check_kernel(
        int *coo_src,
        int *coo_dst,
        int *num_edges,
        int vertex_to_check,
        int mode,  // 0: check, 1: COO, 2: CSR
        int last,
        hls::stream<READ_COMMAND_STREAM> &cmd_to_read,
        hls::stream<ap_axiu<64,0,0,0>> &data_to_coo_stream
    )
    {
        #pragma HLS INTERFACE m_axi port=coo_src offset=slave bundle=gmem0
        #pragma HLS INTERFACE m_axi port=coo_dst offset=slave bundle=gmem1
        #pragma HLS INTERFACE m_axi port=num_edges offset=slave bundle=gmem2
        #pragma HLS INTERFACE axis port=cmd_to_read depth=16
        #pragma HLS INTERFACE axis port=data_to_coo_stream depth=16
        #pragma HLS INTERFACE s_axilite port=coo_src
        #pragma HLS INTERFACE s_axilite port=coo_dst 
        #pragma HLS INTERFACE s_axilite port=num_edges 
        #pragma HLS INTERFACE s_axilite port=vertex_to_check 
        #pragma HLS INTERFACE s_axilite port=mode 
        #pragma HLS INTERFACE s_axilite port=return 

        if (mode == 1) {  // COO mode — scan all vertices
            READ_COMMAND_STREAM cmd;
            cmd.data = PMA_GET_ALL_OUT_EDGE_COO;
            cmd.last = 0;
            #pragma HLS protocol fixed
            ap_wait();
            cmd_to_read.write(cmd);

            int idx = 0;
            while (true)
            {
                ap_axiu<64,0,0,0> pkt = data_to_coo_stream.read();
                ap_uint<64> data = pkt.data;
                int src_val = (int)(data >> 32);
                int dst_val = (int)(data & 0xFFFFFFFF);
                coo_src[idx] = src_val;
                coo_dst[idx] = dst_val;
                idx++;
                if (pkt.last)
                {
                    break;
                }
            }
        } else if (mode == 2) {  // GAS — query vertex_to_check vertices, output only dst[] with -1 separators
            int out_idx = 0;
            for (int v = 0; v < vertex_to_check; v++) {
                int vid = coo_src[v];  // vertex ID to query (coo_src reused as latency output)

                READ_COMMAND_STREAM cmd;
                cmd.data.range(31,0)  = PMA_GET_OUT_EDGE;
                cmd.data.range(63,32) = vid;
                cmd.data.range(95,64) = 0;
                cmd.last = 0;
                #pragma HLS protocol fixed
                ap_wait();
                cmd_to_read.write(cmd);

                // ---- measure cmd-to-first-data latency for this vertex ----
                {
                    ap_uint<32> cnt = 0;
                    while (data_to_coo_stream.empty()) {
                        #pragma HLS PIPELINE II=1
                        cnt++;
                    }
                    coo_src[v] = (int)cnt;  // overwrite: latency in cycles (150MHz → ×6.67ns)
                }

                // Read dst values for this vertex until terminator
                while (true) {
                    ap_axiu<64,0,0,0> pkt = data_to_coo_stream.read();
                    if (pkt.last) break;  // terminator — end of this vertex
                    ap_uint<64> data = pkt.data;
                    coo_dst[out_idx] = (int)(data & 0xFFFFFFFF);  // dst only
                    out_idx++;
                }

                // -1 separator between vertices
                coo_dst[out_idx] = -1;
                out_idx++;
            }
        } else if (mode == 3)
        {
            READ_COMMAND_STREAM count_cmd;
            count_cmd.data = PMA_GET_EDGE_COUNT_RANGE;
            count_cmd.data.range(63,32) = 0;
            count_cmd.data.range(95,64) = vertex_to_check;

            count_cmd.last = 0;
            cmd_to_read.write(count_cmd);
            #pragma HLS protocol fixed
            ap_wait();
            // Read completion packet from merger via data_to_coo_stream;
            // edge count has already been written to *num_edges by merger via m_axi.
            ap_axiu<64,0,0,0> completion = data_to_coo_stream.read();
        }
        if (last)
            cmd_to_read.write({.data = PMA_STOP, .last = 1});
    }
}