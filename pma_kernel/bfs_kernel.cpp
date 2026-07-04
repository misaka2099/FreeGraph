#include "data_type.h"
#include <cstdio>
#include <cstring>
#include <hls_stream.h>
#include "debug.h"
#include <ap_utils.h>
#include "hls_burst_maxi.h"
#include "hls_print.h"

#define BFS_MAX_VERTICES 131072  // 128K vertices

extern "C"
{
    void bfs_kernel(
        int bfs_start,
        int *bfs_number_in,
        int *bfs_number_out,
        int graph_size,
        int max_rounds,
        int last,
        hls::stream<READ_COMMAND_STREAM>& read_command_stream,
        hls::stream<ap_axiu<64,0,0,0>> &data_to_coo_stream
    )
    {
        #pragma HLS INTERFACE s_axilite port=bfs_start bundle=control
        #pragma HLS INTERFACE m_axi port=bfs_number_in  offset=slave bundle=gmem0
        #pragma HLS INTERFACE m_axi port=bfs_number_out offset=slave bundle=gmem1
        #pragma HLS INTERFACE s_axilite port=bfs_number_in  bundle=control
        #pragma HLS INTERFACE s_axilite port=bfs_number_out bundle=control
        #pragma HLS INTERFACE s_axilite port=graph_size bundle=control
        #pragma HLS INTERFACE s_axilite port=max_rounds bundle=control
        #pragma HLS INTERFACE s_axilite port=last bundle=control
        #pragma HLS INTERFACE s_axilite port=return bundle=control
        #pragma HLS INTERFACE axis port=read_command_stream
        #pragma HLS INTERFACE axis port=data_to_coo_stream

        #pragma HLS STREAM variable=read_command_stream depth=8
        #pragma HLS STREAM variable=data_to_coo_stream    depth=128

        int buf_level[BFS_MAX_VERTICES];     // cumulative BFS levels
        #pragma HLS bind_storage variable=buf_level type=RAM_1P impl=URAM
        int buf_even[BFS_MAX_VERTICES];      // double buffer: even rounds
        #pragma HLS bind_storage variable=buf_even type=RAM_1P impl=URAM
        int buf_odd[BFS_MAX_VERTICES];       // double buffer: odd rounds
        #pragma HLS bind_storage variable=buf_odd type=RAM_1P impl=URAM

        int num_vertices = graph_size;
        if (num_vertices > BFS_MAX_VERTICES) num_vertices = BFS_MAX_VERTICES;

        // ---- Init: load bfs_number_in into buf_level ----
    init_level:
        for (int i = 0; i < num_vertices; i++) {
            #pragma HLS PIPELINE II=1
            buf_level[i] = bfs_number_in[i];
        }

        // Set initial frontier: bfs_start is level 1 if not pre-set
        if (buf_level[bfs_start] == 0)
            buf_level[bfs_start] = 1;

        // Clear double buffers
    init_bufs:
        for (int i = 0; i < num_vertices; i++) {
            #pragma HLS PIPELINE II=1
            buf_even[i] = 0;
            buf_odd[i]  = 0;
        }

        // ---- Multi-round BFS ----
        int round = 0;
        bool active = true;

    bfs_rounds:
        while (active && round < max_rounds) {
            round++;

            // Select read/write buffers for this round
            // Odd rounds:  read from buf_odd,  write to buf_even
            // Even rounds: read from buf_even, write to buf_odd
            int *rd_buf = (round & 1) ? buf_odd : buf_even;
            int *wr_buf = (round & 1) ? buf_even : buf_odd;

            // Determine current frontier level
            // Round 1 propagates from level 1 → level 2
            // Round r propagates from level r → level r+1
            int cur_level = round;

            // On round 1, the initial frontier is set from buf_level
            if (round == 1) {
                for (int i = 0; i < num_vertices; i++) {
                    #pragma HLS PIPELINE II=1
                    rd_buf[i] = (buf_level[i] == cur_level) ? cur_level : 0;
                }
            }

            // Send PMA_GET_ALL_OUT_EDGE_COO
            {
                READ_COMMAND_STREAM cmd;
                cmd.data.range(31, 0)  = PMA_GET_ALL_OUT_EDGE_COO;
                cmd.data.range(63, 32) = 0;
                cmd.data.range(95, 64) = 0;
                cmd.last = 0;
                read_command_stream.write(cmd);
            }

            // Process edge stream: propagate from frontier to neighbors
            bool any_update = false;
        process_edges:
            while (true) {
                #pragma HLS PIPELINE off
                ap_axiu<64, 0, 0, 0> pkt = data_to_coo_stream.read();
                if (pkt.last && pkt.data == (ap_uint<64>)-1) break;

                int src = (int)(pkt.data.range(63, 32));
                int dst = (int)(pkt.data.range(31, 0));

                if (src < num_vertices && dst < num_vertices) {
                    // Only propagate from vertices in current frontier
                    if (rd_buf[src] == cur_level) {
                        if (buf_level[dst] == 0) {
                            buf_level[dst] = cur_level + 1;
                            wr_buf[dst] = cur_level + 1;  // active next round
                            any_update = true;
                        }
                    }
                }
            }

            // Check stop conditions
            if (!any_update) {
                active = false;
            }

            // Prepare rd_buf for next round (copy wr_buf into it)
            if (active) {
                for (int i = 0; i < num_vertices; i++) {
                    #pragma HLS PIPELINE II=1
                    rd_buf[i] = 0;
                }
                // wr_buf already has next-round frontier; swap roles on next iteration
                // The next iteration's rd_buf = current wr_buf (via round parity swap)
                // No explicit copy needed — parity handles it
            }
        }

        // ---- Send PMA_STOP only on final round ----
        if (last == 1) {
            READ_COMMAND_STREAM stop;
            stop.data.range(31, 0)  = PMA_STOP;
            stop.data.range(63, 32) = 0;
            stop.data.range(95, 64) = 0;
            stop.last = 1;
            read_command_stream.write(stop);
        }

        // ---- Write final BFS levels to output ----
    write_out:
        for (int i = 0; i < num_vertices; i++) {
            #pragma HLS PIPELINE II=1
            bfs_number_out[i] = buf_level[i];
        }
    }
}
