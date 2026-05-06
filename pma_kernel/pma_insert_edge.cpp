#include "data_type.h"
#include <cstdio>
#include <cstring>
#include <cxxabi.h>
#include <hls_stream.h>
#include "debug.h" 
#include <ap_utils.h>
#include "etc/autopilot_ssdm_op.h"
#include "hls_burst_maxi.h"
#include "hls_print.h"

#define MAX_CACHE_SIZE 8192
#define CACHE_LINE_SIZE 8

#define MEM_REQUEST_NUM 2
//cache by addr
void add_timestamp(
    int now_op_cache_tag,
    ap_uint<32> cache_time_stamp[CACHE_LINE_SIZE]
)
{
    #pragma HLS INLINE
    ADD_TIMESTAMP:
    for (int i = 0 ; i < CACHE_LINE_SIZE ; i++)
    {
        cache_time_stamp[i]++;
    }
    cache_time_stamp[now_op_cache_tag] = 0;
}

void flush_1_cache(
    int to_flush_cache_id,
    ap_uint<CACHE_LINE_SIZE>& cache_valid,
    ap_uint<CACHE_LINE_SIZE>& cache_dirty,
    int cache_pma_idx[CACHE_LINE_SIZE],
    ap_uint<32> cache_old_len[CACHE_LINE_SIZE],
    ap_uint<32> cache_old_start[CACHE_LINE_SIZE],
    ap_uint<32> cache_new_len[CACHE_LINE_SIZE],
    ap_uint<512> cache_buffer[CACHE_LINE_SIZE][MAX_CACHE_SIZE],
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_alloc_access,
    hls::stream<VEC_INFO_STREAM> &vec_info_back_stream,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access,
    hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_mem_access
)
{
    if (cache_valid[to_flush_cache_id] && cache_dirty[to_flush_cache_id]) {
        int evict_pma_idx = cache_pma_idx[to_flush_cache_id];
        int to_write_start = -1;
        int to_write_len = -1;
        if (cache_new_len[to_flush_cache_id] > cache_old_len[to_flush_cache_id]) {
            auto new_len = cache_new_len[to_flush_cache_id];
            MEMORY_INFO_ACCESS_COMMAND alloc_req = {};
            alloc_req.command_type = VEC_INFO_ALLOC;
            alloc_req.pma_idx = evict_pma_idx;
            alloc_req.len = new_len;
            alloc_req.start = 0;
            alloc_req.last = 0;
            // hls::print("[pma] REQ_TO_ALLOC alloc pma_idx=%d\n", evict_pma_idx);
            req_to_alloc_access.write(alloc_req);

            #pragma HLS protocol fixed
            ap_wait();
            auto new_addr_stream = vec_info_back_stream.read();
            auto new_addr = new_addr_stream.data;

            auto old_start = cache_old_start[to_flush_cache_id];
            auto old_len = cache_old_len[to_flush_cache_id];
            if (old_len != 0) {
                MEMORY_INFO_ACCESS_COMMAND free_req = {};
                free_req.command_type = VEC_INFO_FREE;
                free_req.pma_idx = evict_pma_idx;
                free_req.len = old_len;
                free_req.start = old_start;
                free_req.last = 0;
                // hls::print("[pma] REQ_TO_ALLOC free pma_idx=%d\n", evict_pma_idx);
                req_to_alloc_access.write(free_req);
            }

            MEMORY_INFO_ACCESS_COMMAND vec_info_write_cmd = {};
            vec_info_write_cmd.command_type = VEC_INFO_WRITE;
            vec_info_write_cmd.pma_idx = evict_pma_idx;
            vec_info_write_cmd.start = new_addr.range(31,0);
            vec_info_write_cmd.len = new_addr.range(63,32);
            vec_info_write_cmd.last = 0;
            // hls::print("[pma] REQ_TO_MEM VEC_INFO_WRITE pma_idx=%d\n", evict_pma_idx);
            req_to_mem_access.write(vec_info_write_cmd);

            to_write_len = new_len;
            to_write_start = new_addr.range(31,0);
        } else {
            to_write_start = cache_old_start[to_flush_cache_id];
            to_write_len = cache_new_len[to_flush_cache_id];
        }

        MEMORY_INFO_ACCESS_COMMAND write_cmd = {};
        write_cmd.command_type = MEMORY_WRITE;
        write_cmd.pma_idx = evict_pma_idx;
        write_cmd.len = to_write_len;
        write_cmd.start = to_write_start;
        write_cmd.last = 0;
        // hls::print("[pma] REQ_TO_MEM MEMORY_WRITE pma_idx=%d\n", evict_pma_idx);
        req_to_mem_access.write(write_cmd);

        for (int j = 0 ; j < to_write_len / 64 ; j++) {
            #pragma HLS PIPELINE II = 1
            ap_axiu<512, 0, 0, 0> packet;
            packet.data = cache_buffer[to_flush_cache_id][j];
            packet.last = (j == to_write_len / 64 - 1) ? 1 : 0;
            // hls::print("[pma] DATA_TO_MEM packet pma_idx=%d\n", evict_pma_idx);
            data_to_mem_access.write(packet);
        }
        cache_dirty[to_flush_cache_id] = 0;
    }
}
void cache_mechanism(
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_alloc_access,
    hls::stream<VEC_INFO_STREAM> &vec_info_back_stream,
        
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access,
    hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_mem_access,
    hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_mem_access,

    hls::stream<ap_uint<512>> data_from_app[MEM_REQUEST_NUM],
    hls::stream<ap_uint<512>> data_to_app[MEM_REQUEST_NUM],
    hls::stream<PMA_INFO_ACCESS_COMMAND> req_from_app[MEM_REQUEST_NUM]
)
{
    #pragma HLS INTERFACE axis port=req_to_alloc_access
    #pragma HLS INTERFACE axis port=vec_info_back_stream
    #pragma HLS INTERFACE axis port=req_to_mem_access
    #pragma HLS INTERFACE axis port=data_to_mem_access
    #pragma HLS INTERFACE axis port=data_from_mem_access

    #pragma HLS STREAM variable=req_to_alloc_access depth=16
    #pragma HLS STREAM variable=vec_info_back_stream depth=4
    #pragma HLS STREAM variable=req_to_mem_access depth=16
    #pragma HLS STREAM variable=data_to_mem_access depth=16
    #pragma HLS STREAM variable=data_from_mem_access depth=16

    #pragma HLS STREAM variable=data_from_app depth=16
    #pragma HLS STREAM variable=data_to_app depth=16
    #pragma HLS STREAM variable=req_from_app depth=16
    ap_uint<512> cache_buffer[CACHE_LINE_SIZE][MAX_CACHE_SIZE];
    #pragma HLS bind_storage variable=cache_buffer type=ram_2p impl=URAM
    ap_uint<32> cache_time_stamp[CACHE_LINE_SIZE];
    #pragma HLS bind_storage variable=cache_time_stamp type=ram_1p impl=URAM
    // ap_uint<1> cache_valid[CACHE_LINE_SIZE];
    // ap_uint<1> cache_dirty[CACHE_LINE_SIZE];
    ap_uint<CACHE_LINE_SIZE> cache_valid = 0;
    ap_uint<CACHE_LINE_SIZE> cache_dirty = 0;
    ap_uint<32> cache_old_len[CACHE_LINE_SIZE];
    ap_uint<32> cache_old_start[CACHE_LINE_SIZE];
    ap_uint<32> cache_new_len[CACHE_LINE_SIZE];
    int cache_pma_idx[CACHE_LINE_SIZE];

    const int MAX_BS_HELPER_SIZE = (MAX_CACHE_SIZE - (int)(sizeof(PMA_HEADER) / 64)) / (SEGMENT_SIZE * (int)sizeof(int) / 64);
    ap_uint<CACHE_LINE_SIZE> bs_helper_valid = 0;
    int bs_helper_cache[CACHE_LINE_SIZE][MAX_BS_HELPER_SIZE];
    #pragma HLS bind_storage variable=bs_helper_cache type=ram_2p impl=URAM

    ap_uint<MEM_REQUEST_NUM> finish_flag = -1; // all bits set to 1, meaning all requests are not finished
    for (int i = 0 ; i < CACHE_LINE_SIZE ; i++)
    {
        cache_time_stamp[i] = 0;
        cache_old_len[i] = 0;
        cache_old_start[i] = 0;
        cache_new_len[i] = 0;
        cache_pma_idx[i] = -1;
    }
    ap_uint<512> empty_vertex_range = 0;
    for (int v = 0; v < VERTEX_PER_PMA; v++)
    {
        empty_vertex_range.range(v * 32 + 31, v * 32) = v + 1;
    }
    const ap_uint<512> empty_pma_content = -1;//UNUSED_INCON = -1 OTHER NEED TO BE INITIAL MANUALLY
    #pragma HLS bind_storage variable=empty_pma_content type=ram_1p impl=URAM
        const int HEADER_WORDS = (int)(sizeof(PMA_HEADER) / 64);
        const int SEG_WORDS_PER_SEG = SEGMENT_SIZE * (int)sizeof(int) / 64;

    // ---- Write-through state (transparent to binary_search / rebalance) ----
    int  wt_pma_idx = -1;          // active write-through PMA (-1 = none)
    int  wt_hbm_start = 0;         // absolute HBM start address
    int  wt_hbm_len   = 0;         // HBM length in bytes
    ap_uint<512> wt_header_buf[4]; // cached header (256 bytes)
    bool wt_header_dirty = false;  // header modified, needs HBM writeback

    while (true)
    {
        if (finish_flag == 0)
        {
            break;
        }

        //poll every req_from_app
        for (int i = 0 ; i < MEM_REQUEST_NUM ; i++)
        {
            //determine if pma hit/miss
            if (!finish_flag[i]) continue;
            if (req_from_app[i].empty()) continue;
            PMA_INFO_ACCESS_COMMAND cmd = req_from_app[i].read();
            int pma_idx = cmd.pma_idx;
            int seg_start = cmd.seg_start;
            int seg_len = cmd.seg_len;
            if (cmd.command_type == PMA_FINISH)
            {
                finish_flag[i] = 0; // mark this request as finished
                continue;
            }

            // === PMA_HEADER_READ_HINTED: carries batch size in seg_start ===
            // cache_mechanism uses this to decide write-through vs cache load.
            if (cmd.command_type == PMA_HEADER_READ_HINTED)
            {
                int hint_edge_count = cmd.seg_start;
                // Check if PMA is already in cache
                bool in_cache = false;
                for (int line = 0; line < CACHE_LINE_SIZE; line++) {
                    if (cache_valid[line] && cache_pma_idx[line] == pma_idx)
                        in_cache = true;
                }
                if (!in_cache) {
                    // Read vec_info to decide
                    MEMORY_INFO_ACCESS_COMMAND vcmd = {};
                    vcmd.command_type = VEC_INFO_READ;
                    vcmd.pma_idx = pma_idx;
                    vcmd.start = 0; vcmd.len = 0; vcmd.last = 0;
                    req_to_mem_access.write(vcmd);
                    #pragma HLS protocol fixed
                    ap_wait();
                    VEC_INFO vi = data_from_mem_access.read().data.range(63,0);
                    bool pma_exists = (vi != 0);

                    // Decision: write-through vs cache
                    // 1. PMA exists in HBM + batch < 4 → write-through
                    // 2. Otherwise → fall through to cache path (load or init)
                    if (pma_exists && hint_edge_count < 4) {
                        // Flush previous write-through header if switching
                        if (wt_pma_idx != -1 && wt_header_dirty) {
                            MEMORY_INFO_ACCESS_COMMAND whdr = {};
                            whdr.command_type = MEMORY_WRITE;
                            whdr.pma_idx = wt_pma_idx;
                            whdr.start = wt_hbm_start;
                            whdr.len = sizeof(PMA_HEADER);
                            whdr.last = 0;
                            req_to_mem_access.write(whdr);
                            for (int j = 0; j < 4; j++) {
                                #pragma HLS PIPELINE II=1
                                ap_axiu<512, 0, 0, 0> pkt;
                                pkt.data = wt_header_buf[j];
                                pkt.last = (j == 3) ? 1 : 0;
                                data_to_mem_access.write(pkt);
                            }
                            wt_header_dirty = false;
                        }
                        // Set up write-through state
                        wt_pma_idx = pma_idx;
                        wt_hbm_start = vi.range(31,0);
                        wt_hbm_len   = vi.range(63,32);
                        // Read header from HBM
                        MEMORY_INFO_ACCESS_COMMAND rcmd = {};
                        rcmd.command_type = MEMORY_READ;
                        rcmd.pma_idx = pma_idx;
                        rcmd.start = wt_hbm_start;
                        rcmd.len = sizeof(PMA_HEADER);
                        rcmd.last = 0;
                        req_to_mem_access.write(rcmd);
                        for (int j = 0; j < 4; j++) {
                            #pragma HLS PIPELINE II=1
                            wt_header_buf[j] = data_from_mem_access.read().data;
                        }
                        wt_header_dirty = false;
                        // Serve header to app
                        for (int j = 0; j < 4; j++) {
                            #pragma HLS PIPELINE II=1
                            data_to_app[i].write(wt_header_buf[j]);
                        }
                        // write-through state set up
                        continue; // skip cache path
                    }
                }
                // Fall through to cache path (PMA in cache, or large batch, or new PMA)
                // PMA_HEADER_READ_HINTED is treated same as PMA_HEADER_READ below
            }

            // === Write-through command handler ===
            if (wt_pma_idx == pma_idx) {
                if (cmd.command_type == PMA_HEADER_READ
                    || cmd.command_type == PMA_HEADER_READ_HINTED) {
                    for (int j = 0; j < 4; j++) {
                        #pragma HLS PIPELINE II=1
                        data_to_app[i].write(wt_header_buf[j]);
                    }
                } else if (cmd.command_type == PMA_SEGMENT_READ) {
                    int offset = wt_hbm_start + (int)sizeof(PMA_HEADER)
                                 + seg_start * SEGMENT_SIZE * (int)sizeof(int);
                    int len = seg_len * SEGMENT_SIZE * (int)sizeof(int);
                    MEMORY_INFO_ACCESS_COMMAND rcmd = {};
                    rcmd.command_type = MEMORY_READ;
                    rcmd.pma_idx = pma_idx;
                    rcmd.start = offset;
                    rcmd.len = len;
                    rcmd.last = 0;
                    req_to_mem_access.write(rcmd);
                    int words = len / 64;
                    for (int j = 0; j < words; j++) {
                        #pragma HLS PIPELINE II=1
                        data_to_app[i].write(data_from_mem_access.read().data);
                    }
                } else if (cmd.command_type == PMA_SEGMENT_WRITE) {
                    int offset = wt_hbm_start + (int)sizeof(PMA_HEADER)
                                 + seg_start * SEGMENT_SIZE * (int)sizeof(int);
                    int len = seg_len * SEGMENT_SIZE * (int)sizeof(int);
                    MEMORY_INFO_ACCESS_COMMAND wcmd = {};
                    wcmd.command_type = MEMORY_WRITE;
                    wcmd.pma_idx = pma_idx;
                    wcmd.start = offset;
                    wcmd.len = len;
                    wcmd.last = 0;
                    req_to_mem_access.write(wcmd);
                    int words = len / 64;
                    for (int j = 0; j < words; j++) {
                        #pragma HLS PIPELINE II=1
                        ap_axiu<512, 0, 0, 0> pkt;
                        pkt.data = data_from_app[i].read();
                        pkt.last = (j == words - 1) ? 1 : 0;
                        data_to_mem_access.write(pkt);
                    }
                } else if (cmd.command_type == PMA_HEADER_WRITE) {
                    for (int j = 0; j < 4; j++) {
                        #pragma HLS PIPELINE II=1
                        wt_header_buf[j] = data_from_app[i].read();
                    }
                    wt_header_dirty = true;
                } else if (cmd.command_type == PMA_BS_HELPER_READ) {
                    int total_segs = (int)wt_header_buf[0].range(15*32+31, 15*32);
                    int seg_words = SEGMENT_SIZE * (int)sizeof(int) / 64;
                    for (int s = 0; s < total_segs; s++) {
                        #pragma HLS PIPELINE II=1
                        int offset = wt_hbm_start + (int)sizeof(PMA_HEADER)
                                     + s * seg_words * 64;
                        MEMORY_INFO_ACCESS_COMMAND rcmd = {};
                        rcmd.command_type = MEMORY_READ;
                        rcmd.pma_idx = pma_idx;
                        rcmd.start = offset;
                        rcmd.len = sizeof(int);
                        rcmd.last = 0;
                        req_to_mem_access.write(rcmd);
                        #pragma HLS protocol fixed
                        ap_wait();
                        ap_uint<512> out = 0;
                        out.range(31,0) = data_from_mem_access.read().data.range(31,0);
                        data_to_app[i].write(out);
                    }
                } else if (cmd.command_type == PMA_EXTEND) {
                    // Read new vertex_range
                    ap_uint<512> new_range_packet = data_from_app[i].read();
                    int old_range[VERTEX_PER_PMA], new_range[VERTEX_PER_PMA];
                    for (int v = 0; v < VERTEX_PER_PMA; v++) {
                        #pragma HLS UNROLL
                        old_range[v] = (int)wt_header_buf[0].range(v * 32 + 31, v * 32);
                        new_range[v] = (int)new_range_packet.range(v * 32 + 31, v * 32);
                    }
                    int old_total_seg = old_range[VERTEX_PER_PMA - 1];
                    int new_total_seg = new_range[VERTEX_PER_PMA - 1];
                    int old_size = (int)sizeof(PMA_HEADER) + old_total_seg * SEGMENT_SIZE * (int)sizeof(int);
                    int new_size = (int)sizeof(PMA_HEADER) + new_total_seg * SEGMENT_SIZE * (int)sizeof(int);
                    int seg_words = SEGMENT_SIZE * (int)sizeof(int) / 64;

                    // Alloc new space
                    MEMORY_INFO_ACCESS_COMMAND alloc_cmd = {};
                    alloc_cmd.command_type = VEC_INFO_ALLOC;
                    alloc_cmd.pma_idx = pma_idx;
                    alloc_cmd.len = new_size;
                    alloc_cmd.start = 0;
                    alloc_cmd.last = 0;
                    req_to_alloc_access.write(alloc_cmd);
                    #pragma HLS protocol fixed
                    ap_wait();
                    auto new_addr_stream = vec_info_back_stream.read();
                    auto new_addr = new_addr_stream.data;
                    int new_hbm_start = new_addr.range(31,0);

                    // Write new vec_info
                    MEMORY_INFO_ACCESS_COMMAND vi_cmd = {};
                    vi_cmd.command_type = VEC_INFO_WRITE;
                    vi_cmd.pma_idx = pma_idx;
                    vi_cmd.start = new_hbm_start;
                    vi_cmd.len = new_size;
                    vi_cmd.last = 0;
                    req_to_mem_access.write(vi_cmd);

                    // Write header to new location
                    wt_header_buf[0] = new_range_packet;
                    MEMORY_INFO_ACCESS_COMMAND hdr_wcmd = {};
                    hdr_wcmd.command_type = MEMORY_WRITE;
                    hdr_wcmd.pma_idx = pma_idx;
                    hdr_wcmd.start = new_hbm_start;
                    hdr_wcmd.len = sizeof(PMA_HEADER);
                    hdr_wcmd.last = 0;
                    req_to_mem_access.write(hdr_wcmd);
                    for (int j = 0; j < 4; j++) {
                        #pragma HLS PIPELINE II=1
                        ap_axiu<512, 0, 0, 0> pkt;
                        pkt.data = wt_header_buf[j];
                        pkt.last = (j == 3) ? 1 : 0;
                        data_to_mem_access.write(pkt);
                    }

                    // Copy old segments to new location
                    for (int v = VERTEX_PER_PMA - 1; v >= 0; v--) {
                        int old_v_start = (v == 0) ? 0 : old_range[v - 1];
                        int old_v_end = old_range[v];
                        int new_v_start = (v == 0) ? 0 : new_range[v - 1];
                        int new_v_end = new_range[v];
                        // Copy existing segments from old HBM
                        for (int s = 0; s < old_v_end - old_v_start; s++) {
                            int old_off = wt_hbm_start + (int)sizeof(PMA_HEADER)
                                          + (old_v_start + s) * seg_words * 64;
                            int new_off = new_hbm_start + (int)sizeof(PMA_HEADER)
                                          + (new_v_start + s) * seg_words * 64;
                            MEMORY_INFO_ACCESS_COMMAND rcmd2 = {};
                            rcmd2.command_type = MEMORY_READ;
                            rcmd2.pma_idx = pma_idx;
                            rcmd2.start = old_off;
                            rcmd2.len = seg_words * 64;
                            rcmd2.last = 0;
                            req_to_mem_access.write(rcmd2);
                            MEMORY_INFO_ACCESS_COMMAND wcmd2 = {};
                            wcmd2.command_type = MEMORY_WRITE;
                            wcmd2.pma_idx = pma_idx;
                            wcmd2.start = new_off;
                            wcmd2.len = seg_words * 64;
                            wcmd2.last = 0;
                            req_to_mem_access.write(wcmd2);
                            for (int w = 0; w < seg_words; w++) {
                                #pragma HLS PIPELINE II=1
                                ap_axiu<512, 0, 0, 0> pkt;
                                pkt.data = data_from_mem_access.read().data;
                                pkt.last = (w == seg_words - 1) ? 1 : 0;
                                data_to_mem_access.write(pkt);
                            }
                        }
                        // Fill extra new segments with UNUSED_ICON
                        int extra_segs = (new_v_end - new_v_start) - (old_v_end - old_v_start);
                        for (int s = 0; s < extra_segs; s++) {
                            int new_off = new_hbm_start + (int)sizeof(PMA_HEADER)
                                          + (new_v_start + (old_v_end - old_v_start) + s) * seg_words * 64;
                            MEMORY_INFO_ACCESS_COMMAND wcmd3 = {};
                            wcmd3.command_type = MEMORY_WRITE;
                            wcmd3.pma_idx = pma_idx;
                            wcmd3.start = new_off;
                            wcmd3.len = seg_words * 64;
                            wcmd3.last = 0;
                            req_to_mem_access.write(wcmd3);
                            for (int w = 0; w < seg_words; w++) {
                                #pragma HLS PIPELINE II=1
                                ap_axiu<512, 0, 0, 0> pkt;
                                pkt.data = empty_pma_content;
                                pkt.last = (w == seg_words - 1) ? 1 : 0;
                                data_to_mem_access.write(pkt);
                            }
                        }
                    }

                    // Free old allocation
                    MEMORY_INFO_ACCESS_COMMAND free_cmd = {};
                    free_cmd.command_type = VEC_INFO_FREE;
                    free_cmd.pma_idx = pma_idx;
                    free_cmd.start = wt_hbm_start;
                    free_cmd.len = old_size;
                    free_cmd.last = 0;
                    req_to_alloc_access.write(free_cmd);

                    // Update write-through state
                    wt_hbm_start = new_hbm_start;
                    wt_hbm_len = new_size;
                    wt_header_dirty = false;
                }
                continue; // write-through handled
            }

            //determine if cache hit
            int cache_line = -1;
            for (int line = 0; line < CACHE_LINE_SIZE; line++)
            {
                #pragma HLS UNROLL
                if (cache_valid[line] && cache_pma_idx[line] == pma_idx)
                {
                    cache_line = line;
                }
            }

            //determine if cache is full, if full, evict one cache line (write back if dirty)
            int evict_line = -1;
            if (cache_line == -1)
            {
                for (int line = 0; line < CACHE_LINE_SIZE; line++)
                {
                    #pragma HLS UNROLL
                    if (!cache_valid[line])
                    {
                        evict_line = line;
                        break;
                    }
                }
                if (evict_line == -1)
                {
                    ap_uint<32> max_ts = 0;
                    for (int line = 0; line < CACHE_LINE_SIZE; line++)
                    {
                        #pragma HLS UNROLL
                        if (line == 0 || cache_time_stamp[line] > max_ts)
                        {
                            max_ts = cache_time_stamp[line];
                            evict_line = line;
                        }
                    }
                }
            }
            else
            {
                evict_line = cache_line;
            }

            //when evict, if extended, free the old pma, alloc for a new one 
            if (cache_line == -1 && evict_line != -1)
            {
                // flush using helper to avoid duplication
                flush_1_cache(evict_line, cache_valid, cache_dirty, cache_pma_idx,
                              cache_old_len, cache_old_start, cache_new_len,
                              cache_buffer, req_to_alloc_access, vec_info_back_stream,
                              req_to_mem_access, data_to_mem_access);
            }
            //determine if pma_exists in mem_access
            if (cache_line == -1)
            {
                MEMORY_INFO_ACCESS_COMMAND vec_info_read_cmd = {};
                vec_info_read_cmd.command_type = VEC_INFO_READ;
                vec_info_read_cmd.pma_idx = pma_idx;
                vec_info_read_cmd.start = 0;
                vec_info_read_cmd.len = 0;
                vec_info_read_cmd.last = 0;
                // hls::print("[pma] REQ_TO_MEM VEC_INFO_READ pma_idx=%d\n", pma_idx);
                req_to_mem_access.write(vec_info_read_cmd);
                #pragma HLS protocol fixed
                ap_wait();
                VEC_INFO vec_info = data_from_mem_access.read().data.range(63,0);
                bool pma_exists = (vec_info != 0);
                //if exists in mem_access, but not in cache, then read from mem_access and update cache
                if (pma_exists)
                {
                    MEMORY_INFO_ACCESS_COMMAND mem_read_cmd = {};
                    mem_read_cmd.command_type = MEMORY_READ;
                    mem_read_cmd.pma_idx = pma_idx;
                    mem_read_cmd.start = vec_info.range(31,0);
                    mem_read_cmd.len = vec_info.range(63,32);
                    mem_read_cmd.last = 0;
                    // hls::print("[pma] REQ_TO_MEM MEMORY_READ pma_idx=%d\n", pma_idx);
                    req_to_mem_access.write(mem_read_cmd);
                    for (int j = 0 ; j < mem_read_cmd.len / 64 ; j++)
                    {
                        #pragma HLS PIPELINE II=1
                        ap_axiu<512, 0, 0, 0> packet = data_from_mem_access.read();
                        cache_buffer[evict_line][j] = packet.data;
                    }
                    cache_valid[evict_line] = 1;
                    cache_dirty[evict_line] = 0;
                    cache_old_start[evict_line] = vec_info.range(31,0);
                    cache_old_len[evict_line] = vec_info.range(63,32);
                    cache_new_len[evict_line] = vec_info.range(63,32);
                    cache_pma_idx[evict_line] = pma_idx;
                    bs_helper_valid[evict_line] = 0;
                }
                //if not exists in mem_access, inital a brand new pma, update in the cache
                else
                {
                    cache_valid[evict_line] = 1;
                    cache_dirty[evict_line] = 1;
                    cache_old_start[evict_line] = 0;
                    cache_old_len[evict_line] = 0;
                    cache_new_len[evict_line] = sizeof(PMA_HEADER) + 16*SEGMENT_SIZE*sizeof(int); // initialize with max size for simplicity
                    cache_pma_idx[evict_line] = pma_idx;
                    bs_helper_valid[evict_line] = 0;
                    //write empty header to cache_buffer
                    ap_uint<512> header_buffer[4];
                    // for (int j = 0 ; j < 4 ; j++)
                    // {
                    //     #pragma HLS PIPELINE II=1
                    //     header_buffer[j] = ((ap_uint<512>*)(&empty_header))[j];
                    //     cache_buffer[evict_line][j] = header_buffer[j];
                    // }
                    cache_buffer[evict_line][0] = empty_vertex_range;
                    cache_buffer[evict_line][1] = 0;
                    cache_buffer[evict_line][2] = 0;
                    cache_buffer[evict_line][3] = 0;
                    for (int j = 4 ; j < 64 + 4 ; j++)
                    {
                        #pragma HLS PIPELINE II=1
                        cache_buffer[evict_line][j] = empty_pma_content;
                    }
                }
            }
            int active_line = (cache_line == -1) ? evict_line : cache_line;
            add_timestamp(active_line, cache_time_stamp);
            //req to read_a_pma
            if (cmd.command_type == PMA_HEADER_READ
                || cmd.command_type == PMA_HEADER_READ_HINTED)
            {
                for (int j = 0 ; j < (int)(sizeof(PMA_HEADER) / 64) ; j++)
                {
                    #pragma HLS PIPELINE II=1
                    data_to_app[i].write(cache_buffer[active_line][j]);
                }
            }
            else if (cmd.command_type == PMA_SEGMENT_READ)
            {
                int seg_word_start = (int)(sizeof(PMA_HEADER) / 64) + seg_start * (SEGMENT_SIZE * (int)sizeof(int) / 64);
                int seg_words = seg_len * (SEGMENT_SIZE * (int)sizeof(int) / 64);
                for (int j = 0 ; j < seg_words ; j++)
                {
                    #pragma HLS PIPELINE II=1
                    data_to_app[i].write(cache_buffer[active_line][seg_word_start + j]);
                }
            }
            //req to write_a_pma
            else if (cmd.command_type == PMA_HEADER_WRITE)
            {
                for (int j = 0 ; j < (int)(sizeof(PMA_HEADER) / 64) ; j++)
                {
                    #pragma HLS PIPELINE II=1
                    cache_buffer[active_line][j] = data_from_app[i].read();
                }
                cache_dirty[active_line] = 1;
             }
            else if (cmd.command_type == PMA_SEGMENT_WRITE)
            {
                int seg_word_start = (int)(sizeof(PMA_HEADER) / 64) + seg_start * (SEGMENT_SIZE * (int)sizeof(int) / 64);
                int seg_words = seg_len * (SEGMENT_SIZE * (int)sizeof(int) / 64);
                for (int j = 0 ; j < seg_words ; j++)
                {
                    #pragma HLS PIPELINE II=1
                    cache_buffer[active_line][seg_word_start + j] = data_from_app[i].read();
                }
                cache_dirty[active_line] = 1;
             }
            //req to extend a pma
            else if (cmd.command_type == PMA_EXTEND)
            {
                ap_uint<512> new_range_packet = data_from_app[i].read();
                int old_range[VERTEX_PER_PMA];
                int new_range[VERTEX_PER_PMA];
                for (int v = 0; v < VERTEX_PER_PMA; v++)
                {
                    #pragma HLS UNROLL
                    old_range[v] = (int)cache_buffer[active_line][0].range(v * 32 + 31, v * 32);
                    new_range[v] = (int)new_range_packet.range(v * 32 + 31, v * 32);
                }

                int seg_words_per_seg = SEGMENT_SIZE * (int)sizeof(int) / 64;
                int old_total_seg = old_range[VERTEX_PER_PMA - 1];
                int new_total_seg = new_range[VERTEX_PER_PMA - 1];
                int header_words = (int)(sizeof(PMA_HEADER) / 64);
                int old_total_words = header_words + old_total_seg * seg_words_per_seg;
                int new_total_words = header_words + new_total_seg * seg_words_per_seg;
                if (new_total_seg >= old_total_seg && new_total_words <= MAX_CACHE_SIZE)
                {
                    for (int v = VERTEX_PER_PMA - 1; v >= 0; v--)
                    {
                        int old_start = (v == 0) ? 0 : old_range[v - 1];
                        int old_end = old_range[v];
                        int old_seg_num = old_end - old_start;
                        int new_start = (v == 0) ? 0 : new_range[v - 1];
                        for (int s = old_seg_num - 1; s >= 0; s--)
                        {
                            for (int w = 0; w < seg_words_per_seg; w++)
                            {
                                int src_word_idx = header_words + (old_start + s) * seg_words_per_seg + w;
                                int dst_word_idx = header_words + (new_start + s) * seg_words_per_seg + w;
                                cache_buffer[active_line][dst_word_idx] = cache_buffer[active_line][src_word_idx];
                            }
                        }
                    }

                    for (int v = 0; v < VERTEX_PER_PMA; v++)
                    {
                        int old_start = (v == 0) ? 0 : old_range[v - 1];
                        int old_end = old_range[v];
                        int old_seg_num = old_end - old_start;
                        int new_start = (v == 0) ? 0 : new_range[v - 1];
                        int new_end = new_range[v];
                        int extra_seg_num = new_end - new_start - old_seg_num;
                        if (extra_seg_num > 0)
                        {
                            int fill_start_seg = new_start + old_seg_num;
                            for (int s = 0; s < extra_seg_num; s++)
                            {
                                for (int w = 0; w < seg_words_per_seg; w++)
                                {
                                    #pragma HLS PIPELINE II=1
                                    int fill_word_idx = header_words + (fill_start_seg + s) * seg_words_per_seg + w;
                                    cache_buffer[active_line][fill_word_idx] = empty_pma_content;
                                }
                            }
                        }
                    }

                    cache_buffer[active_line][0] = new_range_packet;
                    cache_new_len[active_line] = sizeof(PMA_HEADER) + new_total_seg * SEGMENT_SIZE * sizeof(int);
                    cache_dirty[active_line] = 1;
                    bs_helper_valid[active_line] = 0;
                }
            }
            else if (cmd.command_type == PMA_BS_HELPER_READ)
            {
                if (!bs_helper_valid[active_line])
                {
                    int total_segs = (int)cache_buffer[active_line][0].range(15 * 32 + 31, 15 * 32);
                    int header_words = (int)(sizeof(PMA_HEADER) / 64);
                    int seg_words_per_seg = SEGMENT_SIZE * (int)sizeof(int) / 64;
                    for (int s = 0; s < total_segs; s++)
                    {
                        int seg_word_start = header_words + s * seg_words_per_seg;
                        bs_helper_cache[active_line][s] = cache_buffer[active_line][seg_word_start].range(31, 0);
                    }
                    bs_helper_valid[active_line] = 1;
                }
                int total_segs = (int)cache_buffer[active_line][0].range(15 * 32 + 31, 15 * 32);
                for (int s = 0; s < total_segs; s++)
                {
                    #pragma HLS PIPELINE II=1
                    ap_uint<512> out = 0;
                    out.range(31, 0) = bs_helper_cache[active_line][s];
                    data_to_app[i].write(out);
                }
            }
            else if (cmd.command_type == PMA_FLUSH)
            {
                // Flush write-through header if dirty
                if (wt_pma_idx != -1 && wt_header_dirty) {
                    MEMORY_INFO_ACCESS_COMMAND whdr = {};
                    whdr.command_type = MEMORY_WRITE;
                    whdr.pma_idx = wt_pma_idx;
                    whdr.start = wt_hbm_start;
                    whdr.len = sizeof(PMA_HEADER);
                    whdr.last = 0;
                    req_to_mem_access.write(whdr);
                    for (int j = 0; j < 4; j++) {
                        #pragma HLS PIPELINE II=1
                        ap_axiu<512, 0, 0, 0> pkt;
                        pkt.data = wt_header_buf[j];
                        pkt.last = (j == 3) ? 1 : 0;
                        data_to_mem_access.write(pkt);
                    }
                    wt_header_dirty = false;
                }
                // Flush all dirty cache lines
                for (int line = 0; line < CACHE_LINE_SIZE; line++)
                {
                    if (cache_valid[line] && cache_dirty[line])
                    {
                        flush_1_cache(line, cache_valid, cache_dirty, cache_pma_idx,
                                      cache_old_len, cache_old_start, cache_new_len,
                                      cache_buffer, req_to_alloc_access, vec_info_back_stream,
                                      req_to_mem_access, data_to_mem_access);
                    }
                }
                data_to_app[1].write(1); // ack the flush command
            }
        }

    }
    //do return
    // Flush write-through header if dirty
    if (wt_pma_idx != -1 && wt_header_dirty) {
        MEMORY_INFO_ACCESS_COMMAND whdr = {};
        whdr.command_type = MEMORY_WRITE;
        whdr.pma_idx = wt_pma_idx;
        whdr.start = wt_hbm_start;
        whdr.len = sizeof(PMA_HEADER);
        whdr.last = 0;
        req_to_mem_access.write(whdr);
        for (int j = 0; j < 4; j++) {
            #pragma HLS PIPELINE II=1
            ap_axiu<512, 0, 0, 0> pkt;
            pkt.data = wt_header_buf[j];
            pkt.last = (j == 3) ? 1 : 0;
            data_to_mem_access.write(pkt);
        }
        wt_header_dirty = false;
    }
    // flush all dirty cache lines before exiting
    for (int line = 0; line < CACHE_LINE_SIZE; line++) {
        if (cache_valid[line] && cache_dirty[line]) {
            flush_1_cache(line, cache_valid, cache_dirty, cache_pma_idx,
                          cache_old_len, cache_old_start, cache_new_len,
                          cache_buffer, req_to_alloc_access, vec_info_back_stream,
                          req_to_mem_access, data_to_mem_access);
        }
    }

    data_to_app[1].write(1); // ack the shutdown
    MEMORY_INFO_ACCESS_COMMAND finish_cmd = {};
    finish_cmd.last = 1;
    finish_cmd.command_type = MEMORY_FINISH;
    // hls::print("[pma] REQ_TO_MEM FINISH pma_idx=%d\n", 0);
    req_to_mem_access.write(finish_cmd);
    finish_cmd.command_type = ALLOC_FINISH;
    // hls::print("[pma] REQ_TO_ALLOC FINISH pma_idx=%d\n", 0);
    req_to_alloc_access.write(finish_cmd);
}

void process_cmd(
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
    hls::stream<int> &cmd_count_stream,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_batch_stream,
    hls::stream<pma_idx_info> &pma_info_stream
)
{
    printf("PROCESS_CMD\n");
    while(true)
    {
        if (cmd_stream.empty()) {
            ap_wait();
            continue;
        }
        PMA_UPDATE_MESSAGE cmd_batch[MAX_CMD_NUM];
        #pragma HLS ARRAY_PARTITION variable=cmd_batch complete
        int cmd_count = 0;
        // read until last=1
        bool done = false;
        PMA_UPDATE_MESSAGE first = cmd_stream.read();
        switch (first.opcode) 
        {
            case PMA_UPDATE_STOP:
            {
                if (first.last == 1) {
                    pma_idx_info info;
                    info.pma_idx = 0;
                    info.command_type = INSERT_STOP;
                    pma_info_stream.write(info);
                    return;
                }
                break;
            }
            case PMA_UPDATE_FLUSH_CACHE:
            {
                pma_idx_info info;
                info.pma_idx = first.src / VERTEX_PER_PMA;
                info.command_type = INSERT_FLUSH_CACHE;
                pma_info_stream.write(info);
                break;
            }
            default:
            {
                cmd_batch[cmd_count++] = first;
                if (first.last == 1) {
                    done = true;
                }
                for (int i = 1; i < MAX_CMD_NUM; i++) 
                {
                    if (done) break;
                    PMA_UPDATE_MESSAGE cmd = cmd_stream.read();
                    cmd_batch[i] = cmd;
                    cmd_count++;
                    if (cmd.last == 1) {
                        done = true;
                    }
                }
                pma_idx_info info;
                info.pma_idx = cmd_batch[0].src / VERTEX_PER_PMA;
                info.command_type = INSERT_GOING;
                pma_info_stream.write(info);

                PMA_UPDATE_MESSAGE edge_info[MAX_CMD_NUM];
                #pragma HLS BIND_STORAGE variable=edge_info type=RAM_2P impl=BRAM

                // collect edges
                for (int i = 0; i < cmd_count; i++) {
                    #pragma HLS PIPELINE II=1
                    edge_info[i] = cmd_batch[i];
                }

                // stable insertion sort by src, dst, opcode
                SORT_EDGES:
                for (int i = 1; i < cmd_count; i++) {
                    PMA_UPDATE_MESSAGE key = edge_info[i];
                    int j = i - 1;
                    while (j >= 0 && (
                        (edge_info[j].src > key.src) ||
                        (edge_info[j].src == key.src && (edge_info[j].dst > key.dst 
                        || (edge_info[j].dst == key.dst && (int)edge_info[j].opcode > (int)key.opcode)))
                    )) {
                        edge_info[j + 1] = edge_info[j];
                        j--;
                    }
                    edge_info[j + 1] = key;
                }

                cmd_count_stream.write(cmd_count);
                for (int i = 0; i < cmd_count ; i++)
                {
                    #pragma HLS PIPELINE II=1
                    cmd_batch_stream.write(edge_info[i]);
                }
                break;
            }
        }
        
        
    }
}
void write_seg(
    hls::stream<PMA_INFO_ACCESS_COMMAND>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_to_mem,
    int* storage,
    int pma_idx,
    int seg_idx
)
{
    #pragma HLS INLINE
    PMA_INFO_ACCESS_COMMAND command = {};
    command.command_type = PMA_SEGMENT_WRITE;
    command.pma_idx = pma_idx;
    command.seg_start = seg_idx;
    command.seg_len = 1;
    // hls::print("[pma] MEMORY_ACCESS_SEND PMA_SEGMENT_WRITE pma_idx=%d\n", pma_idx);
    memory_access_stream.write(command);
    for (int j = 0; j < SEGMENT_SIZE * (int)sizeof(int) / 64; j++)
    {
        #pragma HLS PIPELINE II=1
        ap_uint<512> packet = 0;
        for (int k = 0; k < 16; k++)
        {
            #pragma HLS UNROLL
            packet.range(32 * (k + 1) - 1, 32 * k) = storage[j * 16 + k];
        }
        // hls::print("[pma] DATA_TO_MEM packet pma_idx=%d\n", pma_idx);
        data_to_mem.write(packet);
    }
}
void read_seg(
    hls::stream<PMA_INFO_ACCESS_COMMAND>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_from_mem,
    int* storage,
    int pma_idx,
    int seg_idx
)
{
    #pragma HLS INLINE
    PMA_INFO_ACCESS_COMMAND command = {};
    command.command_type = PMA_SEGMENT_READ;
    command.pma_idx = pma_idx;
    command.seg_start = seg_idx;
    command.seg_len = 1;
    // hls::print("[pma] MEMORY_ACCESS_SEND PMA_SEGMENT_READ pma_idx=%d\n", pma_idx);
    memory_access_stream.write(command);
    ap_wait();
    for (int j = 0; j < SEGMENT_SIZE * (int)sizeof(int) / 64; j++)
    {
        #pragma HLS PIPELINE II=1
        ap_uint<512> packet = 0;
        packet = data_from_mem.read();
        for (int k = 0; k < 16; k++)
        {
            #pragma HLS UNROLL
            storage[j * 16 + k] = packet.range(32 * (k + 1) - 1, 32 * k);
        }
    }
}
void write_header(
    hls::stream<PMA_INFO_ACCESS_COMMAND>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_to_mem,
    PMA_HEADER &header_to_write,
    int pma_idx
)
{
    #pragma HLS INLINE
    PMA_INFO_ACCESS_COMMAND header_write_cmd = {};
    header_write_cmd.command_type = PMA_HEADER_WRITE;
    header_write_cmd.pma_idx = pma_idx;
    header_write_cmd.seg_start = 0;
    header_write_cmd.seg_len = 0;
    // hls::print("[pma] MEMORY_ACCESS_SEND PMA_HEADER_WRITE pma_idx=%d\n", pma_idx);
    memory_access_stream.write(header_write_cmd);
    // pack PMA_HEADER into 4 x 512-bit words and write
    ap_uint<512> buf[(int)(sizeof(PMA_HEADER) / 64)];
    for (int i = 0; i < 16; i++) {
        #pragma HLS UNROLL
        buf[0].range(32 * (i + 1) - 1, 32 * i) = header_to_write.vertex_range[i];
        buf[1].range(32 * (i + 1) - 1, 32 * i) = header_to_write.edge_count[i];
    }
    for (int i = 0; i < 8; i++) {
        #pragma HLS UNROLL
        buf[2].range(64 * (i + 1) - 1, 64 * i) = header_to_write.attr[i];
        buf[3].range(64 * (i + 1) - 1, 64 * i) = header_to_write.attr[i + 8];
    }
    for (int j = 0 ; j < (int)(sizeof(PMA_HEADER) / 64) ; j++)
    {
        #pragma HLS PIPELINE II=1
        // hls::print("[pma] DATA_TO_MEM header_word pma_idx=%d\n", pma_idx);
        data_to_mem.write(buf[j]);
    }
}
void read_header(
    hls::stream<PMA_INFO_ACCESS_COMMAND>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_from_mem,
    PMA_HEADER &header_to_read,
    int pma_idx
)
{
    #pragma HLS INLINE
    PMA_INFO_ACCESS_COMMAND header_read_cmd = {};
    header_read_cmd.command_type = PMA_HEADER_READ;
    header_read_cmd.pma_idx = pma_idx;
    header_read_cmd.seg_start = 0;
    header_read_cmd.seg_len = 0;
    // hls::print("[pma] MEMORY_ACCESS_SEND PMA_HEADER_READ pma_idx=%d\n", pma_idx);
    memory_access_stream.write(header_read_cmd);
    ap_wait();
    // read 4 x 512-bit words and unpack into PMA_HEADER
    ap_uint<512> buf[(int)(sizeof(PMA_HEADER) / 64)];
    for (int j = 0 ; j < (int)(sizeof(PMA_HEADER) / 64) ; j++)
    {
        #pragma HLS PIPELINE II=1
        buf[j] = data_from_mem.read();
    }
    // unpack
    for (int i = 0; i < 16; i++) {
        #pragma HLS UNROLL
        header_to_read.vertex_range[i] = (int)buf[0].range(32 * (i + 1) - 1, 32 * i);
        header_to_read.edge_count[i] = (int)buf[1].range(32 * (i + 1) - 1, 32 * i);
    }
    for (int i = 0; i < 8; i++) {
        #pragma HLS UNROLL
        header_to_read.attr[i] = buf[2].range(64 * (i + 1) - 1, 64 * i);
        header_to_read.attr[i + 8] = buf[3].range(64 * (i + 1) - 1, 64 * i);
    }
}
void get_bs_helper(
    PMA_HEADER& header,
    int src,
    int*bs_helper,
    int &cache_size,
    hls::stream<PMA_INFO_ACCESS_COMMAND>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_from_mem
)
{
    #pragma HLS INLINE off
    int local_src = src % VERTEX_PER_PMA;
    int low_seg = local_src == 0 ? 0 : header.vertex_range[local_src - 1];
    int high_seg = header.vertex_range[local_src];
    cache_size = high_seg - low_seg;
    PMA_INFO_ACCESS_COMMAND command = {};
    command.command_type = PMA_SEGMENT_READ;
    // compute pma index from global src and include it in the segment read
    int pma_idx = src / VERTEX_PER_PMA;
    command.pma_idx = pma_idx;
    command.seg_start = low_seg;
    command.seg_len = cache_size;
    // hls::print("[pma] GET_BS_HELPER PMA_SEGMENT_READ pma_idx=%d\n", pma_idx);
    memory_access_stream.write(command);
    ap_wait();
    for (int i = 0 ; i < cache_size ; i++)
    {
        #pragma HLS PIPELINE off
        auto data = data_from_mem.read();
        bs_helper[i] = data.range(31,0);
        for (int j = 0 ; j < 3 ; j++)
        {
            data_from_mem.read();// discard the rest of a segment
        }
    }
}
void sub_search(
    int* cache_bs_helper,
    int seg_low,
    int seg_high,
    int dst,
    int &searched_seg_number
)
{
    #pragma HLS INLINE off
    int low = seg_low;
    int high = seg_high - 1;
    searched_seg_number = -1;
    while (low <= high) {
        #pragma HLS PIPELINE off
        int mid = (low + high) / 2;
        if (cache_bs_helper[mid] == dst) {
            searched_seg_number = mid - seg_low;
            return;
        } else if (cache_bs_helper[mid] < dst) {
            low = mid + 1;
            searched_seg_number = mid - seg_low;
        } else {
            high = mid - 1;
        }
    }
}

struct inserted_seg
{
    int old_vertex_relative_seg_number[16];
    int vertex_number[16];
};
void binary_search_insert_cu(
    hls::stream<PMA_INFO_ACCESS_COMMAND>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_to_mem,
    hls::stream<ap_uint<512>>& data_from_mem,
    hls::stream<PMA_UPDATE_MESSAGE> &edge_info_stream,
    hls::stream<int> &edge_count_in,
    hls::stream<pma_idx_info> &pma_idx_in,
    hls::stream<pma_idx_info> &pma_idx_out,
    hls::stream<PMA_HEADER> &header_to_rebalance,
    hls::stream<inserted_seg>& inserted_pos_to_rebalance
)
{
    printf("BINARY_SEARCH\n");
    while (true)
    {
        auto signal = pma_idx_in.read();
        if (signal.command_type == INSERT_FLUSH_CACHE)
        {
            pma_idx_out.write(signal);
            continue;
        }
        else if (signal.command_type == INSERT_STOP)
        {
            PMA_INFO_ACCESS_COMMAND x = {};
            x.seg_start = 0;
            x.seg_len = 0;
            x.command_type = PMA_FINISH;
            x.pma_idx = signal.pma_idx;
            // hls::print("[pma] BINARY_SEARCH FINISH pma_idx=%d\n", signal.pma_idx);
            memory_access_stream.write(x);
            pma_idx_out.write(signal);
            break;
        }

        PMA_HEADER header;
        int count = edge_count_in.read();
        // Send hinted header read: cache_mechanism uses seg_start as batch size hint
        {
            PMA_INFO_ACCESS_COMMAND hdr_cmd = {};
            hdr_cmd.command_type = PMA_HEADER_READ_HINTED;
            hdr_cmd.pma_idx = signal.pma_idx;
            hdr_cmd.seg_start = count;
            hdr_cmd.seg_len = 0;
            memory_access_stream.write(hdr_cmd);
            ap_wait();
            // unpack header (same as read_header helper)
            ap_uint<512> buf[(int)(sizeof(PMA_HEADER) / 64)];
            for (int j = 0; j < (int)(sizeof(PMA_HEADER) / 64); j++) {
                #pragma HLS PIPELINE II=1
                buf[j] = data_from_mem.read();
            }
            for (int i = 0; i < 16; i++) {
                #pragma HLS UNROLL
                header.vertex_range[i] = (int)buf[0].range(32 * (i + 1) - 1, 32 * i);
                header.edge_count[i] = (int)buf[1].range(32 * (i + 1) - 1, 32 * i);
            }
            for (int i = 0; i < 8; i++) {
                #pragma HLS UNROLL
                header.attr[i] = buf[2].range(64 * (i + 1) - 1, 64 * i);
                header.attr[i + 8] = buf[3].range(64 * (i + 1) - 1, 64 * i);
            }
        }

        inserted_seg inserted_seg_info;
        inserted_seg deleted_seg_info;
        for (int i = 0 ; i < 16 ; i++)
        {
            inserted_seg_info.old_vertex_relative_seg_number[i] = -1;
            inserted_seg_info.vertex_number[i] = -1;
            deleted_seg_info.old_vertex_relative_seg_number[i] = -1;
            deleted_seg_info.vertex_number[i] = -1;
        }

        int bs_helper[10240];
        #pragma HLS BIND_STORAGE variable=bs_helper type=RAM_2P impl=URAM
        // read all segment first-words from cache_mechanism via PMA_BS_HELPER_READ
        {
            PMA_INFO_ACCESS_COMMAND bs_cmd = {};
            bs_cmd.command_type = PMA_BS_HELPER_READ;
            bs_cmd.pma_idx = signal.pma_idx;
            bs_cmd.seg_start = 0;
            bs_cmd.seg_len = 0;
            memory_access_stream.write(bs_cmd);
            ap_wait();
            int total_segs = header.vertex_range[VERTEX_PER_PMA - 1];
            for (int s = 0; s < total_segs; s++)
            {
                #pragma HLS PIPELINE II=1
                bs_helper[s] = data_from_mem.read().range(31, 0);
            }
        }
        int insert_idx = 0;
        int delete_idx = 0;

        ap_uint<64> insert_ops[16];
        ap_uint<64> delete_edge_ops[16];
        ap_uint<64> delete_vertex_ops[16];
        int insert_ops_count = 0;
        int delete_edge_ops_count = 0;
        int delete_vertex_ops_count = 0;

        for (int i = 0 ; i < count ; i++)
        {
            #pragma HLS PIPELINE off
            PMA_UPDATE_MESSAGE edge_info = edge_info_stream.read();

            int opcode = (int)edge_info.opcode;
            int src = edge_info.src;
            int dst = edge_info.dst;
            if (opcode == PMA_UPDATE_INSERT_EDGE)
            {
                int local_src = src % VERTEX_PER_PMA;
                int low_seg = local_src == 0 ? 0 : header.vertex_range[local_src - 1];
                int high_seg = header.vertex_range[local_src];

                int searched_seg_number = -1;
                sub_search(bs_helper, low_seg, high_seg, dst, searched_seg_number);
                if (searched_seg_number == -1)
                {
                    searched_seg_number = 0;
                }
                inserted_seg_info.old_vertex_relative_seg_number[insert_idx] = searched_seg_number;
                inserted_seg_info.vertex_number[insert_idx] = local_src;
                ap_uint<64> payload = 0;
                payload.range(63,32) = (ap_uint<32>)src;
                payload.range(31,0) = (ap_uint<32>)dst;
                insert_ops[insert_ops_count++] = payload;
                insert_idx++;
            }
            else if (opcode == PMA_UPDATE_DELETE_EDGE)
            {
                int local_src = src % VERTEX_PER_PMA;
                int low_seg = local_src == 0 ? 0 : header.vertex_range[local_src - 1];
                int high_seg = header.vertex_range[local_src];

                int searched_seg_number = -1;
                sub_search(bs_helper, low_seg, high_seg, dst, searched_seg_number);
                if (searched_seg_number == -1)
                {
                    searched_seg_number = 0;
                }

                deleted_seg_info.old_vertex_relative_seg_number[delete_idx] = searched_seg_number;
                deleted_seg_info.vertex_number[delete_idx] = local_src;
                ap_uint<64> payload = 0;
                payload.range(63,32) = (ap_uint<32>)src;
                payload.range(31,0) = (ap_uint<32>)dst;
                delete_edge_ops[delete_edge_ops_count++] = payload;
                delete_idx++;
            }
            else if (opcode == PMA_UPDATE_DELETE_VERTEX)
            {
                ap_uint<64> payload = 0;
                payload.range(63,32) = src;
                payload.range(31,0) = dst;
                delete_vertex_ops[delete_vertex_ops_count++] = payload;
            }
        }

        int current_cached_id = -1;
        bool is_dirty = false;
        int storage[SEGMENT_SIZE];
        for (int i = 0 ; i < SEGMENT_SIZE ; i++)
        {
            #pragma HLS UNROLL
            storage[i] = UNUSED_ICON;
        }

        for (int i = 0 ; i < delete_vertex_ops_count; i++)
        {
            #pragma HLS PIPELINE off
            int src = delete_vertex_ops[i].range(63,32);
            int local_src = src % VERTEX_PER_PMA;
            int low_seg = local_src == 0 ? 0 : header.vertex_range[local_src - 1];
            int high_seg = header.vertex_range[local_src];
            // for (int s = low_seg; s < high_seg; s++)
            // {
            //     write_seg(memory_access_stream, data_to_mem, data_from_mem, storage, pma_data_base_addr, s);
            // }
            PMA_INFO_ACCESS_COMMAND command = {};
            command.command_type = PMA_SEGMENT_WRITE;
            command.pma_idx = signal.pma_idx;
            command.seg_start = low_seg;
            command.seg_len = high_seg - low_seg;
            // hls::print("[pma] PMA_SEGMENT_WRITE delete_vertex pma_idx=%d\n", signal.pma_idx);
            memory_access_stream.write(command);
            for (int s = low_seg; s < high_seg; s++)            
            {
                for (int k = 0; k < 4; k++)
                {
                    // hls::print("[pma] DATA_TO_MEM_FILL_UNUSED pma_idx=%d\n", signal.pma_idx);
                    data_to_mem.write(-1);//UNUSED ICON = -1 OTHERWISE NEED TO BE INITIAL MANUALLY
                }
            }
            header.edge_count[local_src] = 0;
        }

        for (int i = 0 ; i < delete_edge_ops_count ; i++)
        {
            #pragma HLS PIPELINE off
            int src = delete_edge_ops[i].range(63,32);
            int dst = delete_edge_ops[i].range(31,0);
            int local_src = src % VERTEX_PER_PMA;
            int low_seg = local_src == 0 ? 0 : header.vertex_range[local_src - 1];
            int target_seg = deleted_seg_info.old_vertex_relative_seg_number[i] + low_seg;
            if (target_seg < low_seg)
            {
                continue;
            }
            if (current_cached_id != target_seg)
            {
                if (is_dirty && current_cached_id != -1)
                {
                    write_seg(memory_access_stream, data_to_mem, storage, signal.pma_idx, current_cached_id);
                }
                read_seg(memory_access_stream, data_from_mem, storage, signal.pma_idx, target_seg);
                
                current_cached_id = target_seg;
                is_dirty = false;
            }
            int found_idx_for_del = -1;
            for (int j = 0; j < SEGMENT_SIZE; j++)
            {
                #pragma HLS LOOP_TRIPCOUNT min=0 max=64
                if (storage[j] == dst)
                {
                    found_idx_for_del = j;
                    break;
                }
            }
            if (found_idx_for_del == -1)
            {
                continue;
            }
            for (int j = found_idx_for_del + 1; j < SEGMENT_SIZE; j++)
            {
                storage[j - 1] = storage[j];
            }
            storage[SEGMENT_SIZE - 1] = UNUSED_ICON;
            is_dirty = true;
            header.edge_count[local_src]--;
        }

        for (int i = 0 ; i < insert_ops_count ; i++)
        {
            #pragma HLS PIPELINE off
            int src = insert_ops[i].range(63,32);
            int dst = insert_ops[i].range(31,0);
            int local_src = src % VERTEX_PER_PMA;
            int low_seg = local_src == 0 ? 0 : header.vertex_range[local_src - 1];
            int target_seg = inserted_seg_info.old_vertex_relative_seg_number[i] + low_seg;
            if (target_seg < low_seg)
            {
                continue;
            }
            if (current_cached_id != target_seg)
            {
                if (is_dirty && current_cached_id != -1)
                {
                    write_seg(memory_access_stream, data_to_mem, storage, signal.pma_idx, current_cached_id);
                }
                read_seg(memory_access_stream, data_from_mem, storage, signal.pma_idx, target_seg);
                current_cached_id = target_seg;
            }
            int found_idx_for_insert = -1;
            for (int j = 0; j < SEGMENT_SIZE; j++)
            {
                #pragma HLS LOOP_TRIPCOUNT min=0 max=64
                int storage_val = storage[j];
                if (storage_val >= dst || storage_val == UNUSED_ICON)
                {
                    found_idx_for_insert = j;
                    break;
                }
            }
            for (int j = SEGMENT_SIZE - 1; j > found_idx_for_insert; j--)
            {
                storage[j] = storage[j - 1];
            }
            storage[found_idx_for_insert] = dst;
            is_dirty = true;
            header.edge_count[local_src]++;
        }

        if (is_dirty && current_cached_id != -1)
        {
            write_seg(memory_access_stream, data_to_mem, storage, signal.pma_idx, current_cached_id);
            is_dirty = false;
        }
        // Forward the updated header to rebalance; do not let rebalance re-read stale header from cache.
        header_to_rebalance.write(header);
        // Ensure rebalance reads the updated header (edge_count/vertex_range) from cache.
        inserted_pos_to_rebalance.write(inserted_seg_info);
        pma_idx_out.write(signal);
    }
}

static inline int density_capacity_from_seg_count(int seg_count)
{
    #pragma HLS INLINE
    return (seg_count * SEGMENT_SIZE * 3) >> 2;
}

void redistribute(
    int seg_start, int seg_end,
    hls::stream<PMA_INFO_ACCESS_COMMAND>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_to_mem,
    hls::stream<ap_uint<512>>& data_from_mem,
    int total_valid_count,
    int pma_idx)
{
    int total_segs = seg_end - seg_start;
    // int capacity = total_segs * SEGMENT_SIZE;
    int tmp[8192*32];  // 在 HLS 中用静态大数组 + pragma BIND_STORAGE
    #pragma HLS BIND_STORAGE variable=tmp type=RAM_2P impl=URAM
    int tmp_idx = 0;

    // 1. 读取所有段，收集所有非 UNUSED 元素
    for (int s = seg_start; s < seg_end; s++) {
        int buf[SEGMENT_SIZE];
        read_seg(memory_access_stream, data_from_mem, buf, pma_idx, s);
        for (int k = 0; k < SEGMENT_SIZE; k++) {
            if (buf[k] != UNUSED_ICON) {
                tmp[tmp_idx++] = buf[k];
            }
        }
    }

    // 2. 均匀写入
    int base = total_valid_count / total_segs;
    int extra = total_valid_count % total_segs;
    int pos = 0;
    for (int w = seg_start; w < seg_end; w++) {
        int fill = base + ((w - seg_start) < extra ? 1 : 0);
        int wbuf[SEGMENT_SIZE];
        for (int k = 0; k < SEGMENT_SIZE; k++) {
            if (k < fill && pos < total_valid_count) {
                wbuf[k] = tmp[pos++];
            } else {
                wbuf[k] = UNUSED_ICON;
            }
        }
        write_seg(memory_access_stream, data_to_mem, wbuf, pma_idx, w);
    }
}

int valid_count(
    int seg_start, int seg_end, 
    hls::stream<PMA_INFO_ACCESS_COMMAND>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_from_mem,
    int pma_idx
)
{
    #pragma HLS INLINE
    int total_segs = seg_end - seg_start;
    if (total_segs <= 0) return 0;

    PMA_INFO_ACCESS_COMMAND command;
    command.pma_idx = pma_idx;
    command.seg_start = seg_start;
    command.seg_len = seg_end - seg_start;
    command.command_type = PMA_SEGMENT_READ;
    // hls::print("[pma] VALID_COUNT PMA_SEGMENT_READ pma_idx=%d\n", pma_idx);
    memory_access_stream.write(command);

    int count = 0;
    // Each segment has 4 * 512-bit words (assuming SEGMENT_SIZE=64)
    int total_words = total_segs * 4; 
    VALID_COUNT_LOOP:
    for (int i = 0 ; i < total_words ; i++)
    {
        #pragma HLS PIPELINE II=1
        ap_uint<512> val = data_from_mem.read();
        int local_count = 0;//important to avoid HLS report dependency on count variable

        for (int j = 0 ; j < 16 ; j++)
        {
            #pragma HLS UNROLL
            int item = val.range(32 * (j + 1) - 1, 32 * j);
            local_count += (item != UNUSED_ICON);
        }

        count += local_count;
    }
    return count;
}
void rebalance_cu(
    // ap_uint<512>* hbm_ptr,
    hls::stream<PMA_INFO_ACCESS_COMMAND>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_to_mem,
    hls::stream<ap_uint<512>>& data_from_mem,
    hls::stream<pma_idx_info>& pma_idx,
    hls::stream<PMA_HEADER>& header_from_bs,
    hls::stream<inserted_seg>& inserted_seg_info,
    hls::stream<COMMAND_STREAM>& ack_back_to_router
)
{
        
    printf("REBALANCE\n");
    while (true)
    {
        auto signal = pma_idx.read();
        if (signal.command_type == INSERT_FLUSH_CACHE)
        {
            PMA_INFO_ACCESS_COMMAND x = {};
            x.pma_idx = 0;
            x.seg_start = 0;
            x.seg_len = 0;
            x.command_type = PMA_FLUSH;
            memory_access_stream.write(x);
            ap_wait();
            data_from_mem.read();
            COMMAND_STREAM ack_pkt = {};
            ack_pkt.data = -1;
            ack_pkt.keep = (ap_uint<4>)0xF;
            ack_pkt.strb = (ap_uint<4>)0xF;
            ack_pkt.user = (ap_uint<3>)0;
            ack_pkt.last = 0;
            ack_back_to_router.write(ack_pkt);
            continue;
        }
        else if (signal.command_type == INSERT_STOP)
        {
            PMA_INFO_ACCESS_COMMAND x = {};
            x.pma_idx = 0;
            x.seg_start = 0;
            x.seg_len = 0;
            x.command_type = PMA_FINISH;
            // hls::print("[pma] REBALANCE FINISH pma_idx=%d\n", signal.pma_idx);
            memory_access_stream.write(x);
            ap_wait();
            data_from_mem.read();
            COMMAND_STREAM ack_pkt = {};
            ack_pkt.data = -1;
            ack_pkt.keep = (ap_uint<4>)0xF;
            ack_pkt.strb = (ap_uint<4>)0xF;
            ack_pkt.user = (ap_uint<3>)0;
            ack_pkt.last = 0;
            ack_back_to_router.write(ack_pkt);
            break;
        }
        PMA_HEADER header;
        #pragma HLS BIND_STORAGE variable=header type=RAM_2P impl=URAM
        header = header_from_bs.read();
        PMA_HEADER old_pma_header = header; // for later use in redistribution
        auto to_check = inserted_seg_info.read();
        // const int pma_data_size_bytes = pma_addr.range(63,32) - sizeof(PMA_HEADER);
        
        int enlarge_segs[16];
        for (int i = 0 ; i < 16 ; i++)
        {
            enlarge_segs[i] = 0;
        }
        bool enlarged = false;
        for (int v = 0 ; v < VERTEX_PER_PMA ; v++)
        {
            #pragma HLS PIPELINE II=2
            int seg_start = v == 0 ? 0 : header.vertex_range[v-1];
            int seg_end = header.vertex_range[v];
            // int max_edge = (seg_end - seg_start) * SEGMENT_SIZE * DENSITY_BOUND;
            int max_edge = density_capacity_from_seg_count(seg_end - seg_start);
            if (max_edge < header.edge_count[v])
            {
                enlarged = true;
                enlarge_segs[v] = 3 * (seg_end - seg_start);
            }
        }
        int total_enlarge_segs = 0;
        for (int i = 0 ; i < VERTEX_PER_PMA ; i++)
        {
            #pragma HLS PIPELINE II=1
            total_enlarge_segs += enlarge_segs[i];
            header.vertex_range[i] += total_enlarge_segs;
        }
        if (enlarged)
        {
            // Enlarged
            PMA_INFO_ACCESS_COMMAND extend_cmd = {};
            // int new_size = current_size + total_enlarge_segs * SEGMENT_SIZE * 4;
            extend_cmd.command_type = PMA_EXTEND;
            extend_cmd.pma_idx = signal.pma_idx;
            extend_cmd.seg_start = 0;
            extend_cmd.seg_len = 0;
            // hls::print("[pma] PMA_EXTEND pma_idx=%d\n", signal.pma_idx);
            memory_access_stream.write(extend_cmd);
            ap_uint<512> new_range_packet = 0;
            for (int v = 0 ; v < VERTEX_PER_PMA ; v++)
            {
                #pragma HLS PIPELINE II=1
                new_range_packet.range(v * 32 + 31, v * 32) = header.vertex_range[v];
            }
            // hls::print("[pma] DATA_TO_MEM new_range pma_idx=%d\n", signal.pma_idx);
            data_to_mem.write(new_range_packet);
            // for (int v = 0 ; v < VERTEX_PER_PMA ; v++)
            // {
            //     int seg_start = (v == 0) ? 0 : header.vertex_range[v-1];
            //     int seg_end = header.vertex_range[v];

            //     int total_valid_count = old_pma_header.edge_count[v];
                
            //     redistribute(seg_start, seg_end, 
            //         memory_access_stream, data_to_mem, data_from_mem, 
            //         total_valid_count, signal.pma_idx);
            // }
        }
        int last_vertex = -10;
        int last_reletive_seg = -10;
        for (int i = 0 ; i < 16 ; i++)//去除重复判定
        {
            if (
                to_check.old_vertex_relative_seg_number[i] == last_reletive_seg &&
                to_check.vertex_number[i] == last_vertex
            )
            {
                to_check.old_vertex_relative_seg_number[i] = -1;
                to_check.vertex_number[i] = -1;
            }
            else  
            {
                last_vertex = to_check.vertex_number[i];
                last_reletive_seg = to_check.old_vertex_relative_seg_number[i];
            }
        }
        for (int i = 0 ; i < 16 ; i++)
        {
            if (to_check.vertex_number[i] == -1) continue;
            int v = to_check.vertex_number[i];
            int relative_seg_num = to_check.old_vertex_relative_seg_number[i];

            int seg_start = (v == 0) ? 0 : header.vertex_range[v-1];
            int seg_end = header.vertex_range[v];
            int seg_num = seg_end - seg_start; // 当前vertex的segment数
            int total_valid_count = 0;
            int seg_idx = relative_seg_num;
            total_valid_count = valid_count(seg_idx + seg_start, seg_idx + seg_start+ 1, memory_access_stream, data_from_mem, signal.pma_idx);
            int last_left = seg_idx + seg_start;
            int last_right = seg_idx + seg_start + 1 ;
            int max_edge = density_capacity_from_seg_count(1);
            if (total_valid_count < max_edge) continue;
            for (int cur_seg_num = 2 ; cur_seg_num <= seg_num ; cur_seg_num *= 2)
            {
                int left = seg_start + int(seg_idx / cur_seg_num) * cur_seg_num;
                int right = seg_start + (int(seg_idx / cur_seg_num) + 1) * cur_seg_num;
                // int valid = valid_count(left, right);
                    if (left < last_left) {
                    total_valid_count += valid_count(left, last_left, memory_access_stream, data_from_mem, signal.pma_idx);
                }
                else if (right > last_right) {
                    total_valid_count += valid_count(last_right, right, memory_access_stream, data_from_mem, signal.pma_idx);
                }

                max_edge = density_capacity_from_seg_count(right - left);
                if (total_valid_count <= max_edge)
                {
                    redistribute(left, right,
                        memory_access_stream, data_to_mem, data_from_mem,
                        total_valid_count, signal.pma_idx);
                    break;
                }
                last_left = left;
                last_right = right;
            }
        }
        
        
        
        write_header(memory_access_stream, data_to_mem, header, signal.pma_idx);
        COMMAND_STREAM ack_pkt = {};
        ack_pkt.data = (ap_uint<32>)signal.pma_idx;
        ack_pkt.keep = (ap_uint<4>)0xF;
        ack_pkt.strb = (ap_uint<4>)0xF;
        ack_pkt.user = (ap_uint<3>)0;
        ack_pkt.last = 0;
        ack_back_to_router.write(ack_pkt);
    }
}   

extern "C"
{
    void pma_insert_edge(
        
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_alloc,
        hls::stream<VEC_INFO_STREAM> &vec_info_back_stream,
        hls::stream<COMMAND_STREAM> &ack_back_to_router,
        
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_mem_access,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_mem_access
    )
    {
        #pragma HLS INTERFACE axis port=cmd_stream
        #pragma HLS INTERFACE axis port=req_to_mem_alloc
        #pragma HLS INTERFACE axis port=vec_info_back_stream
        #pragma HLS INTERFACE axis port=ack_back_to_router

        #pragma HLS INTERFACE axis port=req_to_mem_access
        #pragma HLS INTERFACE axis port=data_to_mem_access
        #pragma HLS INTERFACE axis port=data_from_mem_access
        // Internal streams connecting the modules
        hls::stream<int> cmd_count_stream;
        #pragma HLS STREAM variable=cmd_count_stream depth=8
        hls::stream<PMA_UPDATE_MESSAGE> cmd_batch_stream;
        #pragma HLS STREAM variable=cmd_batch_stream depth=32
        hls::stream<pma_idx_info> pma_info_stream;
        #pragma HLS STREAM variable=pma_info_stream depth=8

        // memory request/data streams for modules (slots)
        hls::stream<PMA_INFO_ACCESS_COMMAND> req_from_app[MEM_REQUEST_NUM];
        hls::stream<ap_uint<512>> data_from_app[MEM_REQUEST_NUM];
        hls::stream<ap_uint<512>> data_to_app[MEM_REQUEST_NUM];
        // set stream depths for array elements
        #pragma HLS STREAM variable=req_from_app depth=8
        #pragma HLS STREAM variable=data_from_app depth=16
        #pragma HLS STREAM variable=data_to_app depth=16

        hls::stream<pma_idx_info> pma_idx_to_rebalance;
        #pragma HLS STREAM variable=pma_idx_to_rebalance depth=8
        hls::stream<PMA_HEADER> header_to_rebalance;
        #pragma HLS STREAM variable=header_to_rebalance depth=8
        hls::stream<inserted_seg> inserted_seg_stream;
        #pragma HLS STREAM variable=inserted_seg_stream depth=8

        #pragma HLS DATAFLOW

        // dispatch and connect modules via streams
        process_cmd(cmd_stream, cmd_count_stream, cmd_batch_stream, pma_info_stream);

        // Binary-search insert CU uses slot 0
        binary_search_insert_cu(req_from_app[0], data_from_app[0], data_to_app[0],
                                cmd_batch_stream, cmd_count_stream,
                                pma_info_stream, pma_idx_to_rebalance,
                                header_to_rebalance, inserted_seg_stream);

        // Rebalance CU uses slot 1
        rebalance_cu(req_from_app[1], data_from_app[1], data_to_app[1], 
                     pma_idx_to_rebalance, header_to_rebalance,
                     inserted_seg_stream, ack_back_to_router);

        // Cache mechanism bridges modules to external memory ports
        cache_mechanism(req_to_mem_alloc, vec_info_back_stream,
                        req_to_mem_access, data_to_mem_access, data_from_mem_access,
                        data_from_app, data_to_app, req_from_app);
    }
}