#include <hls_stream.h>
#include <ap_int.h>
#include "data_type.h"
#include "etc/autopilot_ssdm_op.h"
#include "hls_print.h"

#define HEADER_WORDS ((int)(sizeof(PMA_HEADER) / 64))
#define SEG_WORDS_PER_SEG (SEGMENT_SIZE * (int)sizeof(int) / 64)
#define SEG_BYTES (SEG_WORDS_PER_SEG * 64)

static const int READ_WORKERS = CHANNEL_NUM;

typedef ap_axiu<64, 8, 0, 0> READ_PKT_STREAM;

static void hbm_read_words_simple(ap_uint<512>* hbm, int byte_off, int words, ap_uint<512>* buf)
{
    int base = byte_off / 64;
    for (int w = 0; w < words; w++) {
        #pragma HLS PIPELINE II=1
        buf[w] = hbm[base + w];
    }
}

static VEC_INFO vi_read_simple(ap_uint<512>* hbm, int vi_wo, int pma_idx)
{
    int wi = pma_idx / 8, si = pma_idx % 8;
    ap_uint<512> w = hbm[vi_wo + wi];
    VEC_INFO vi;
    vi.range(63, 0) = w.range(si * 64 + 63, si * 64);
    return vi;
}

static void read_header(
    ap_uint<512>* hbm,
    int header_byte_off,
    PMA_HEADER &pma_header
) {
    #pragma HLS INLINE
    ap_uint<512> buffer[HEADER_WORDS];
    #pragma HLS ARRAY_PARTITION variable=buffer complete
    hbm_read_words_simple(hbm, header_byte_off, HEADER_WORDS, buffer);

    for (int j = 0; j < 16; j++) {
        #pragma HLS UNROLL
        pma_header.vertex_range[j] = (int)buffer[0].range(32 * (j + 1) - 1, 32 * j);
        pma_header.edge_count[j] = (int)buffer[1].range(32 * (j + 1) - 1, 32 * j);
    }
}

static void read_seg(
    ap_uint<512>* hbm,
    int segment_buffer[SEGMENT_SIZE],
    int seg_byte_off
) {
    #pragma HLS INLINE
    ap_uint<512> buffer[SEG_WORDS_PER_SEG];
    #pragma HLS ARRAY_PARTITION variable=buffer complete
    hbm_read_words_simple(hbm, seg_byte_off, SEG_WORDS_PER_SEG, buffer);

    for (int i = 0; i < SEG_WORDS_PER_SEG; i++) {
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
    ap_uint<512>* hbm_ptr,
    int pma_raw_data_start,
    int pma_vecinfo_start,
    hls::stream<READ_COMMAND_STREAM> &cmd_stream,
    hls::stream<READ_PKT_STREAM> &pkt_to_merger
) {
    #pragma HLS INTERFACE s_axilite port=worker_id bundle=control
    #pragma HLS INTERFACE m_axi port=hbm_ptr offset=slave bundle=gmem0
    #pragma HLS INTERFACE s_axilite port=hbm_ptr bundle=control
    #pragma HLS INTERFACE s_axilite port=pma_raw_data_start bundle=control
    #pragma HLS INTERFACE s_axilite port=pma_vecinfo_start bundle=control
    #pragma HLS INTERFACE s_axilite port=return bundle=control
    #pragma HLS INTERFACE axis port=cmd_stream
    #pragma HLS INTERFACE axis port=pkt_to_merger

    #pragma HLS STREAM variable=cmd_stream depth=16
    #pragma HLS STREAM variable=pkt_to_merger depth=128

    const ap_uint<8> KEEP64 = 0xFF;
    int vi_wo = pma_vecinfo_start / 64;

    while (true) {
        READ_COMMAND_STREAM cmd = cmd_stream.read();
        if (cmd.data == PMA_STOP && cmd.last == 1) {
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

            VEC_INFO vec_info = vi_read_simple(hbm_ptr, vi_wo, pma_idx);

            if (vec_info == 0) {
                write_pkt(pkt_to_merger, 0, 0xFF, 1, KEEP64);
                write_pkt(pkt_to_merger, 0, 0, 1, 0);
                continue;
            }

            PMA_HEADER pma_header;
            int header_start = (int)vec_info.range(31, 0);
            int header_byte_off = header_start - pma_raw_data_start;
            read_header(hbm_ptr, header_byte_off, pma_header);

            int local_src = src % VERTEX_PER_PMA;
            int seg_start = (local_src == 0) ? 0 : pma_header.vertex_range[local_src - 1];
            int seg_end = pma_header.vertex_range[local_src];
            int pma_data_base = header_start + (int)sizeof(PMA_HEADER);
            int segment_buffer[SEGMENT_SIZE];

            for (int s = seg_start; s < seg_end; s++) {
                int seg_byte_off = pma_data_base + s * SEG_BYTES - pma_raw_data_start;
                read_seg(hbm_ptr, segment_buffer, seg_byte_off);
                int cnt = segment_buffer[VALID_COUNT_IDX];
                for (int j = 0; j < cnt; j++) {
                    #pragma HLS PIPELINE II=1
                    write_pkt(pkt_to_merger, (ap_uint<32>)segment_buffer[j], 0xFF, 0, KEEP64);
                }
            }
            write_pkt(pkt_to_merger, 0, 0xFF, 1, KEEP64);
            write_pkt(pkt_to_merger, 0, 0, 1, 0);
        } else if (mode == PMA_GET_ALL_OUT_EDGE_COO || mode == PMA_GET_ALL_OUT_EDGE_CSR) {
            for (int pma_idx = worker_id; ; pma_idx += READ_WORKERS) {
                VEC_INFO vec_info = vi_read_simple(hbm_ptr, vi_wo, pma_idx);
                if (vec_info == 0) break;

                PMA_HEADER pma_header;
                int header_start = (int)vec_info.range(31, 0);
                int header_byte_off = header_start - pma_raw_data_start;
                read_header(hbm_ptr, header_byte_off, pma_header);

                int pma_data_base = header_start + (int)sizeof(PMA_HEADER);
                int segment_buffer[SEGMENT_SIZE];

                for (int local_v = 0; local_v < VERTEX_PER_PMA; local_v++) {
                    int s_start = (local_v == 0) ? 0 : pma_header.vertex_range[local_v - 1];
                    int s_end = pma_header.vertex_range[local_v];
                    int src = pma_idx * VERTEX_PER_PMA + local_v;
                    for (int s = s_start; s < s_end; s++) {
                        int seg_byte_off = pma_data_base + s * SEG_BYTES - pma_raw_data_start;
                        read_seg(hbm_ptr, segment_buffer, seg_byte_off);
                        int cnt = segment_buffer[VALID_COUNT_IDX];
                        for (int j = 0; j < cnt; j++) {
                            #pragma HLS PIPELINE II=1
                            int neighbor = segment_buffer[j];
                            ap_uint<64> pair = ((ap_uint<64>)(ap_uint<32>)src << 32) | (ap_uint<32>)neighbor;
                            write_pkt(pkt_to_merger, pair, 0xFF, 0, KEEP64);
                        }
                    }
                }
                write_pkt(pkt_to_merger, 0, 0xFF, 1, KEEP64);
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

                VEC_INFO vec_info = vi_read_simple(hbm_ptr, vi_wo, pma_idx);
                if (vec_info == 0) {
                    find_empty = true;
                    break;
                }

                PMA_HEADER pma_header;
                int header_start = (int)vec_info.range(31, 0);
                int header_byte_off = header_start - pma_raw_data_start;
                read_header(hbm_ptr, header_byte_off, pma_header);

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
