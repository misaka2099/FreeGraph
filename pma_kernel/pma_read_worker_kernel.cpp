#include <hls_stream.h>
#include <ap_int.h>
#include "data_type.h"
#include "etc/autopilot_ssdm_op.h"
#include "hls_print.h"
static const int READ_WORKERS = CHANNEL_NUM;

typedef ap_axiu<64, 8, 0, 0> READ_PKT_STREAM;

static void read_header(
    int pma_header_addr_in_byte,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &memory_access_stream,
    hls::stream<ap_axiu<512,0,0,0>> &data_from_mem,
    PMA_HEADER &pma_header
) {
#pragma HLS INLINE
    ap_uint<512> buffer[4];
#pragma HLS ARRAY_PARTITION variable=buffer complete

    MEMORY_INFO_ACCESS_COMMAND command = {};
    command.start = pma_header_addr_in_byte;
    command.pma_idx = 0;
    command.len = 256;
    command.command_type = MEMORY_READ;
    command.last = 0;
    memory_access_stream.write(command);

    for (int i = 0; i < 4; i++) {
#pragma HLS PIPELINE II=1
        buffer[i] = data_from_mem.read().data;
    }

    for (int j = 0; j < 16; j++) {
#pragma HLS UNROLL
        pma_header.vertex_range[j] = (int)buffer[0].range(32 * (j + 1) - 1, 32 * j);
        pma_header.edge_count[j] = (int)buffer[1].range(32 * (j + 1) - 1, 32 * j);
    }
}

static void read_seg(
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &memory_access_stream,
    hls::stream<ap_axiu<512,0,0,0>> &data_from_mem,
    int segment_buffer[SEGMENT_SIZE],
    int pma_data_base_addr,
    int seg_number
) {
#pragma HLS INLINE
    ap_uint<512> buffer[4];
#pragma HLS ARRAY_PARTITION variable=buffer complete

    MEMORY_INFO_ACCESS_COMMAND command = {};
    command.start = seg_number * SEGMENT_SIZE * (int)sizeof(int) + pma_data_base_addr;
    command.len = SEGMENT_SIZE * (int)sizeof(int);
    command.command_type = MEMORY_READ;
    command.last = 0;
    memory_access_stream.write(command);

    for (int i = 0; i < 4; i++) {
#pragma HLS PIPELINE II=1
        buffer[i] = data_from_mem.read().data;
    }

    for (int i = 0; i < 4; i++) {
#pragma HLS UNROLL
        for (int j = 0; j < 16; j++) {
#pragma HLS UNROLL
            segment_buffer[i * 16 + j] = (int)buffer[i].range(32 * (j + 1) - 1, 32 * j);
        }
    }
}

static void write_pkt(
    hls::stream<READ_PKT_STREAM> &out,
    ap_uint<64> data,
    ap_uint<8> user,
    ap_uint<1> last,
    ap_uint<8> keep
) {
#pragma HLS INLINE
    READ_PKT_STREAM pkt = {};
    pkt.data = data;
    pkt.user = user;
    pkt.last = last;
    pkt.keep = keep;
    pkt.strb = keep;
    out.write(pkt);
}

extern "C" {
void pma_read_worker_kernel(
    int worker_id,
    hls::stream<READ_COMMAND_STREAM> &cmd_stream,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access_ch,
    hls::stream<ap_axiu<512,0,0,0>> &data_from_mem_access_ch,
    hls::stream<READ_PKT_STREAM> &pkt_to_merger
) {
#pragma HLS INTERFACE s_axilite port=worker_id bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control
#pragma HLS INTERFACE axis port=cmd_stream
#pragma HLS INTERFACE axis port=req_to_mem_access_ch
#pragma HLS INTERFACE axis port=data_from_mem_access_ch
#pragma HLS INTERFACE axis port=pkt_to_merger

#pragma HLS STREAM variable=cmd_stream depth=16
#pragma HLS STREAM variable=req_to_mem_access_ch depth=16
#pragma HLS STREAM variable=data_from_mem_access_ch depth=16
#pragma HLS STREAM variable=pkt_to_merger depth=128

    const ap_uint<8> KEEP64 = 0xFF;

    while (true) {
        READ_COMMAND_STREAM cmd = cmd_stream.read();
        if (cmd.data == PMA_STOP && cmd.last == 1) {
            MEMORY_INFO_ACCESS_COMMAND finish_cmd = {};
            finish_cmd.command_type = MEMORY_FINISH;
            finish_cmd.pma_idx = 0;
            finish_cmd.start = 0;
            finish_cmd.len = 0;
            finish_cmd.last = 1;
            req_to_mem_access_ch.write(finish_cmd);
            break;
        }

        int mode = (int)cmd.data.range(31, 0);

        if (mode == PMA_GET_OUT_EDGE) {
            int src = (int)cmd.data.range(63, 32);
            int pma_idx = src / VERTEX_PER_PMA;
            if (pma_idx % READ_WORKERS != worker_id) {
                write_pkt(pkt_to_merger, 0, 0, 1, 0);
                continue;
            }

            MEMORY_INFO_ACCESS_COMMAND access_command = {};
            access_command.command_type = VEC_INFO_READ;
            access_command.pma_idx = pma_idx;
            access_command.last = 0;
            req_to_mem_access_ch.write(access_command);
            ap_wait();
            VEC_INFO vec_info = data_from_mem_access_ch.read().data.range(63, 0);

            if (vec_info == 0) {
                write_pkt(pkt_to_merger, 0, 0xFF, 1, KEEP64);
                write_pkt(pkt_to_merger, 0, 0, 1, 0);
                continue;
            }

            PMA_HEADER pma_header;
            int header_start = (int)vec_info.range(31, 0);
            read_header(header_start, req_to_mem_access_ch, data_from_mem_access_ch, pma_header);

            int local_src = src % VERTEX_PER_PMA;
            int seg_start = (local_src == 0) ? 0 : pma_header.vertex_range[local_src - 1];
            int seg_end = pma_header.vertex_range[local_src];
            int pma_data_base_addr = header_start + (int)sizeof(PMA_HEADER);
            int segment_buffer[SEGMENT_SIZE];

            for (int s = seg_start; s < seg_end; s++) {
                read_seg(req_to_mem_access_ch, data_from_mem_access_ch, segment_buffer, pma_data_base_addr, s);
                for (int j = 0; j < SEGMENT_SIZE; j++) {
                    #pragma HLS PIPELINE II=1
                    int neighbor = segment_buffer[j];
                    if (neighbor == UNUSED_ICON) break;
                    write_pkt(pkt_to_merger, (ap_uint<32>)neighbor, 0xFF, 0, KEEP64);
                }
            }
            write_pkt(pkt_to_merger, 0, 0xFF, 1, KEEP64);
            write_pkt(pkt_to_merger, 0, 0, 1, 0);
        } else if (mode == PMA_GET_ALL_OUT_EDGE_COO || mode == PMA_GET_ALL_OUT_EDGE_CSR) {
            int pma_idx = worker_id;
            while (true) {
                MEMORY_INFO_ACCESS_COMMAND access_command = {};
                access_command.command_type = VEC_INFO_READ;
                access_command.pma_idx = pma_idx;
                access_command.last = 0;
                req_to_mem_access_ch.write(access_command);
                ap_wait();
                VEC_INFO vec_info = data_from_mem_access_ch.read().data.range(63, 0);
                if (vec_info == 0) 
                {
                    hls::print("Worker %d: No more PMA to read, total PMA count: \n", worker_id);
                    hls::print("%d\n", pma_idx);
                    break;
                }
                PMA_HEADER pma_header;
                int header_start = (int)vec_info.range(31, 0);
                read_header(header_start, req_to_mem_access_ch, data_from_mem_access_ch, pma_header);

                int pma_data_base_addr = header_start + (int)sizeof(PMA_HEADER);
                int segment_buffer[SEGMENT_SIZE];

                for (int local_v = 0; local_v < VERTEX_PER_PMA; local_v++) {
                    int seg_start = (local_v == 0) ? 0 : pma_header.vertex_range[local_v - 1];
                    int seg_end = pma_header.vertex_range[local_v];
                    int src = pma_idx * VERTEX_PER_PMA + local_v;
                    for (int s = seg_start; s < seg_end; s++) {
                        read_seg(req_to_mem_access_ch, data_from_mem_access_ch, segment_buffer, pma_data_base_addr, s);
                        for (int j = 0; j < SEGMENT_SIZE; j++) {
                            #pragma HLS PIPELINE II=1
                            int neighbor = segment_buffer[j];
                            if (neighbor == UNUSED_ICON) break;
                            ap_uint<64> pair = ((ap_uint<64>)(ap_uint<32>)src << 32) | (ap_uint<32>)neighbor;
                            write_pkt(pkt_to_merger, pair, 0xFF, 0, KEEP64);
                        }
                    }
                }

                write_pkt(pkt_to_merger, 0, 0xFF, 1, KEEP64);
                pma_idx += READ_WORKERS;
            }
            write_pkt(pkt_to_merger, 0, 0, 1, 0);
        } else if (mode == PMA_GET_EDGE_COUNT_RANGE) {
            int start_v = (int)cmd.data.range(63, 32);
            int end_v = (int)cmd.data.range(95, 64);
            if (end_v <= start_v) {
                write_pkt(pkt_to_merger, 0, 0, 0, KEEP64);
                write_pkt(pkt_to_merger, 0, 0, 1, 0);
                continue;
            }

            int last_v = end_v - 1;
            int pma_start = start_v / VERTEX_PER_PMA;
            int pma_end = last_v / VERTEX_PER_PMA;
            int total_sum = 0;
            bool find_empty = false;

            for (int pma_idx = worker_id; pma_idx <= pma_end; pma_idx += READ_WORKERS) {
                if (pma_idx < pma_start) continue;

                MEMORY_INFO_ACCESS_COMMAND access_command = {};
                access_command.command_type = VEC_INFO_READ;
                access_command.pma_idx = pma_idx;
                access_command.last = 0;
                req_to_mem_access_ch.write(access_command);
                ap_wait();
                VEC_INFO vec_info = data_from_mem_access_ch.read().data.range(63, 0);
                if (vec_info == 0) {

                    find_empty = true;
                    break;
                }

                PMA_HEADER pma_header;
                int header_start = (int)vec_info.range(31, 0);
                read_header(header_start, req_to_mem_access_ch, data_from_mem_access_ch, pma_header);

                int local_start = (pma_idx == pma_start) ? (start_v % VERTEX_PER_PMA) : 0;
                int local_end = (pma_idx == pma_end) ? (last_v % VERTEX_PER_PMA) : (VERTEX_PER_PMA - 1);
                for (int i = local_start; i <= local_end; i++) {
                    total_sum += pma_header.edge_count[i];
                }
            }
            
            write_pkt(pkt_to_merger, (ap_uint<64>)(ap_uint<32>)total_sum, find_empty ? 0 : 0xff, 0, KEEP64);
            write_pkt(pkt_to_merger, 0, 0, 1, 0);
        }
    }
}
}
