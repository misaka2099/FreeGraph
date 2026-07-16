#include "data_type.h"
#include "hls_stream.h"

#define ROUTER_WORKERS CHANNEL_NUM

extern "C"
{
    void update_message_router_hbm(
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_from_generator,
        int* task_buf_0,
        int* task_buf_1,
        int* task_buf_2,
        int* task_buf_3,
        hls::stream<WHOLE_ACK> &ack_to_generator
    )
    {
        #pragma HLS INTERFACE axis port=cmd_from_generator
        #pragma HLS INTERFACE m_axi port=task_buf_0 offset=slave bundle=gmem0 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=task_buf_1 offset=slave bundle=gmem1 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=task_buf_2 offset=slave bundle=gmem2 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=task_buf_3 offset=slave bundle=gmem3 max_write_burst_length=256
        #pragma HLS INTERFACE axis port=ack_to_generator

        #pragma HLS INTERFACE s_axilite port=task_buf_0 bundle=control
        #pragma HLS INTERFACE s_axilite port=task_buf_1 bundle=control
        #pragma HLS INTERFACE s_axilite port=task_buf_2 bundle=control
        #pragma HLS INTERFACE s_axilite port=task_buf_3 bundle=control
        #pragma HLS INTERFACE s_axilite port=return bundle=control

        #pragma HLS STREAM variable=cmd_from_generator depth=16
        #pragma HLS STREAM variable=ack_to_generator depth=16

        int wr_cnt[ROUTER_WORKERS];
        #pragma HLS ARRAY_PARTITION variable=wr_cnt complete
        for (int w = 0; w < ROUTER_WORKERS; w++) {
            #pragma HLS UNROLL
            wr_cnt[w] = 0;
        }

        while (true) {
            #pragma HLS PIPELINE II=1
            PMA_UPDATE_MESSAGE msg;
            if (cmd_from_generator.read_nb(msg)) {

                if (msg.opcode == PMA_UPDATE_STOP) {
                    // Write count + sentinel to each worker's HBM buffer
                    // Layout: [0]=count, [1][2]=reserved, [3+]= {src,dst,opcode}, sentinel
                    for (int w = 0; w < ROUTER_WORKERS; w++) {
                        #pragma HLS UNROLL
                        int cnt = wr_cnt[w];
                        int pos = 3 + cnt * 3;  // sentinel position
                        if (w == 0) {
                            task_buf_0[0] = cnt;
                            task_buf_0[1] = 0;
                            task_buf_0[2] = 0;
                            task_buf_0[pos + 0] = -1;
                            task_buf_0[pos + 1] = 0;
                            task_buf_0[pos + 2] = 0;
                        } else if (w == 1) {
                            task_buf_1[0] = cnt;
                            task_buf_1[1] = 0;
                            task_buf_1[2] = 0;
                            task_buf_1[pos + 0] = -1;
                            task_buf_1[pos + 1] = 0;
                            task_buf_1[pos + 2] = 0;
                        } else if (w == 2) {
                            task_buf_2[0] = cnt;
                            task_buf_2[1] = 0;
                            task_buf_2[2] = 0;
                            task_buf_2[pos + 0] = -1;
                            task_buf_2[pos + 1] = 0;
                            task_buf_2[pos + 2] = 0;
                        } else {
                            task_buf_3[0] = cnt;
                            task_buf_3[1] = 0;
                            task_buf_3[2] = 0;
                            task_buf_3[pos + 0] = -1;
                            task_buf_3[pos + 1] = 0;
                            task_buf_3[pos + 2] = 0;
                        }
                    }
                    // ACK generator
                    WHOLE_ACK pkt;
                    pkt.data = 1;
                    pkt.keep = (ap_uint<1>)1;
                    pkt.strb = (ap_uint<1>)1;
                    pkt.last = 1;
                    ack_to_generator.write(pkt);
                    break;

                } else {
                    int pma = msg.src / VERTEX_PER_PMA;
                    int wid = pma % ROUTER_WORKERS;
                    int pos = 3 + wr_cnt[wid] * 3;

                    // Direct write to per-worker HBM (no pooling)
                    if (wid == 0) {
                        task_buf_0[pos + 0] = msg.src;
                        task_buf_0[pos + 1] = msg.dst;
                        task_buf_0[pos + 2] = (int)msg.opcode;
                    } else if (wid == 1) {
                        task_buf_1[pos + 0] = msg.src;
                        task_buf_1[pos + 1] = msg.dst;
                        task_buf_1[pos + 2] = (int)msg.opcode;
                    } else if (wid == 2) {
                        task_buf_2[pos + 0] = msg.src;
                        task_buf_2[pos + 1] = msg.dst;
                        task_buf_2[pos + 2] = (int)msg.opcode;
                    } else {
                        task_buf_3[pos + 0] = msg.src;
                        task_buf_3[pos + 1] = msg.dst;
                        task_buf_3[pos + 2] = (int)msg.opcode;
                    }
                    wr_cnt[wid]++;
                }
            }
        }
    }
}
