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

#define MEM_REQUEST_NUM 2
#define MAX_CACHE_SIZE 8192  // kept for backward compat only

//cache by addr
void add_timestamp(
    int now_op_cache_tag,
    ap_uint<32> cache_time_stamp[TOTAL_CACHE_LINES]
)
{
    #pragma HLS INLINE
    ADD_TIMESTAMP:
    for (int i = 0 ; i < TOTAL_CACHE_LINES ; i++)
    {
        #pragma HLS PIPELINE II=1
        cache_time_stamp[i]++;
    }
    cache_time_stamp[now_op_cache_tag] = 0;
}

void flush_1_cache(
    int to_flush_cache_id,
    ap_uint<TOTAL_CACHE_LINES>& cache_valid,
    ap_uint<TOTAL_CACHE_LINES>& cache_dirty,
    int cache_pma_idx[TOTAL_CACHE_LINES],
    ap_uint<32> cache_old_len[TOTAL_CACHE_LINES],
    ap_uint<32> cache_old_start[TOTAL_CACHE_LINES],
    ap_uint<32> cache_new_len[TOTAL_CACHE_LINES],
    ap_uint<512> large_buf[LARGE_LINES][LARGE_SIZE],
    ap_uint<512> medium_buf[MEDIUM_LINES][MEDIUM_SIZE],
    ap_uint<512> small_buf[SMALL_LINES][SMALL_SIZE],
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
                req_to_alloc_access.write(free_req);
            }

            MEMORY_INFO_ACCESS_COMMAND vec_info_write_cmd = {};
            vec_info_write_cmd.command_type = VEC_INFO_WRITE;
            vec_info_write_cmd.pma_idx = evict_pma_idx;
            vec_info_write_cmd.start = new_addr.range(31,0);
            vec_info_write_cmd.len = new_addr.range(63,32);
            vec_info_write_cmd.last = 0;
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
        req_to_mem_access.write(write_cmd);

        int line = to_flush_cache_id;
        for (int j = 0 ; j < to_write_len / 64 ; j++) {
            #pragma HLS PIPELINE II = 1
            ap_axiu<512, 0, 0, 0> packet;
            if (line < LARGE_LINES) packet.data = large_buf[line][j];
            else if (line < MEDIUM_END) packet.data = medium_buf[line - MEDIUM_START][j];
            else packet.data = small_buf[line - SMALL_START][j];
            packet.last = (j == to_write_len / 64 - 1) ? 1 : 0;
            data_to_mem_access.write(packet);
        }
        cache_dirty[to_flush_cache_id] = 0;
    }
}
// Forward decl: tier-aware buffer helpers (defined after buffer arrays inside cache_mechanism)
// We use macros instead
#define READ_BUF_WORD(line, word) \
    ((line) < LARGE_LINES ? large_buf[(line)][word] : \
     (line) < MEDIUM_END ? medium_buf[(line) - MEDIUM_START][word] : \
     small_buf[(line) - SMALL_START][word])
#define WRITE_BUF_WORD(line, word, val) do { \
    if ((line) < LARGE_LINES) large_buf[(line)][word] = (val); \
    else if ((line) < MEDIUM_END) medium_buf[(line) - MEDIUM_START][word] = (val); \
    else small_buf[(line) - SMALL_START][word] = (val); \
} while(0)

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
    // ---- Pyramid Cache Buffers ----
    ap_uint<512> large_buf[LARGE_LINES][LARGE_SIZE];
    #pragma HLS bind_storage variable=large_buf type=ram_2p impl=URAM
    ap_uint<512> medium_buf[MEDIUM_LINES][MEDIUM_SIZE];
    #pragma HLS bind_storage variable=medium_buf type=ram_2p impl=URAM
    ap_uint<512> small_buf[SMALL_LINES][SMALL_SIZE];
    #pragma HLS bind_storage variable=small_buf type=ram_2p impl=URAM

    // ---- Unified Metadata (index 0..TOTAL-1: Large=0..1, Medium=2..9, Small=10..25) ----
    ap_uint<TOTAL_CACHE_LINES> cache_valid = 0;
    ap_uint<TOTAL_CACHE_LINES> cache_dirty = 0;
    ap_uint<32> cache_time_stamp[TOTAL_CACHE_LINES];
    #pragma HLS bind_storage variable=cache_time_stamp type=ram_1p impl=URAM
    ap_uint<32> cache_old_len[TOTAL_CACHE_LINES];
    ap_uint<32> cache_old_start[TOTAL_CACHE_LINES];
    ap_uint<32> cache_new_len[TOTAL_CACHE_LINES];
    int cache_pma_idx[TOTAL_CACHE_LINES];
    // Per-tier max words for buffer access
    static const int tier_max_words[TOTAL_CACHE_LINES] = {
        [0]=LARGE_SIZE, [1]=LARGE_SIZE,  // Large tier: lines 0-1
        [2]=MEDIUM_SIZE, [3]=MEDIUM_SIZE, [4]=MEDIUM_SIZE, [5]=MEDIUM_SIZE,  // Medium: 2-5
        [6]=SMALL_SIZE, [7]=SMALL_SIZE, [8]=SMALL_SIZE, [9]=SMALL_SIZE,
        [10]=SMALL_SIZE, [11]=SMALL_SIZE, [12]=SMALL_SIZE, [13]=SMALL_SIZE,
        [14]=SMALL_SIZE, [15]=SMALL_SIZE, [16]=SMALL_SIZE, [17]=SMALL_SIZE,
        [18]=SMALL_SIZE, [19]=SMALL_SIZE, [20]=SMALL_SIZE, [21]=SMALL_SIZE  // Small: 6-21
    };

    // ---- Tiered bs_helper caches ----
    #define LARGE_BS_MAX  ((LARGE_SIZE  - 4) / (SEGMENT_SIZE * (int)sizeof(int) / 64))
    #define MEDIUM_BS_MAX ((MEDIUM_SIZE - 4) / (SEGMENT_SIZE * (int)sizeof(int) / 64))
    #define SMALL_BS_MAX  ((SMALL_SIZE  - 4) / (SEGMENT_SIZE * (int)sizeof(int) / 64))
    ap_uint<TOTAL_CACHE_LINES> bs_helper_valid = 0;
    int large_bs_cache[LARGE_LINES][LARGE_BS_MAX];
    #pragma HLS bind_storage variable=large_bs_cache type=ram_2p impl=URAM
    int medium_bs_cache[MEDIUM_LINES][MEDIUM_BS_MAX];
    #pragma HLS bind_storage variable=medium_bs_cache type=ram_2p impl=URAM
    int small_bs_cache[SMALL_LINES][SMALL_BS_MAX];
    #pragma HLS bind_storage variable=small_bs_cache type=ram_2p impl=URAM

    // ---- Profile Table: track PMA access frequency across evictions ----
    int   profile_pma_idx[PROFILE_TABLE_SIZE];
    ap_uint<8>  profile_access_count[PROFILE_TABLE_SIZE];
    ap_uint<16> profile_last_time[PROFILE_TABLE_SIZE];
    bool  profile_promoted[PROFILE_TABLE_SIZE];
    #pragma HLS ARRAY_PARTITION variable=profile_pma_idx complete
    #pragma HLS ARRAY_PARTITION variable=profile_access_count complete
    #pragma HLS ARRAY_PARTITION variable=profile_last_time complete
    #pragma HLS ARRAY_PARTITION variable=profile_promoted complete
    ap_uint<16> profile_timestamp = 0;
    for (int p = 0; p < PROFILE_TABLE_SIZE; p++) {
        #pragma HLS UNROLL
        profile_pma_idx[p] = -1;
        profile_access_count[p] = 0;
        profile_last_time[p] = 0;
        profile_promoted[p] = false;
    }

    ap_uint<MEM_REQUEST_NUM> finish_flag = -1;
    for (int i = 0 ; i < TOTAL_CACHE_LINES ; i++)
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
    const ap_uint<512> empty_pma_content = -1;
    const int HEADER_WORDS = (int)(sizeof(PMA_HEADER) / 64);
    const int SEG_WORDS_PER_SEG = SEGMENT_SIZE * (int)sizeof(int) / 64;

    // ---- Write-through state (transparent to binary_search / rebalance) ----
    int  wt_pma_idx = -1;          // active write-through PMA (-1 = none)
    int  wt_hbm_start = 0;         // absolute HBM start address
    int  wt_hbm_len   = 0;         // HBM length in bytes
    ap_uint<512> wt_header_buf[4]; // cached header (256 bytes)
    bool wt_header_dirty = false;  // header modified, needs HBM writeback
    // Small bs_helper cache for write-through PMAs (avoids N× MEMORY_READ round-trips)
    #define WT_BS_CACHE_LINES 8
    int  wt_bs_pma_idx[WT_BS_CACHE_LINES];   // -1 = invalid
    int  wt_bs_data[WT_BS_CACHE_LINES][64];  // up to 64 segments per PMA
    int  wt_bs_segs[WT_BS_CACHE_LINES];       // number of valid segments in entry
    #pragma HLS BIND_STORAGE variable=wt_bs_data type=RAM_2P impl=BRAM
    for (int e = 0; e < WT_BS_CACHE_LINES; e++) wt_bs_pma_idx[e] = -1;

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
                // Update profile table: single unrolled scan for match + empty + LRU
                profile_timestamp++;
                int ps_match = -1, ps_empty = -1, ps_lru = -1;
                ap_uint<16> oldest = profile_timestamp;
                for (int p = 0; p < PROFILE_TABLE_SIZE; p++) {
                    #pragma HLS UNROLL
                    if (profile_pma_idx[p] == pma_idx) ps_match = p;
                    if (profile_pma_idx[p] == -1 && ps_empty == -1) ps_empty = p;
                    if (profile_last_time[p] < oldest) {
                        oldest = profile_last_time[p]; ps_lru = p;
                    }
                }
                int ps = ps_match;
                if (ps == -1) {
                    ps = (ps_empty != -1) ? ps_empty : ps_lru;
                    profile_pma_idx[ps] = pma_idx;
                    profile_access_count[ps] = 0;
                    if (ps_empty == -1) profile_promoted[ps] = false;
                }
                profile_access_count[ps]++;
                profile_last_time[ps] = profile_timestamp;
                // Check if PMA is already in cache
                bool in_cache = false;
                for (int line = 0; line < TOTAL_CACHE_LINES; line++) {
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
                    if (pma_exists && hint_edge_count < BYPASS_EDGE_THRESHOLD) {
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
                    // Check bs_helper cache
                    int cache_e = -1;
                    for (int e = 0; e < WT_BS_CACHE_LINES; e++) {
                        #pragma HLS UNROLL
                        if (wt_bs_pma_idx[e] == pma_idx) { cache_e = e; }
                    }
                    if (cache_e != -1) {
                        // Cache hit: pack 16 segs per word
                        int packed_words = (total_segs + 15) / 16;
                        for (int w = 0; w < packed_words; w++) {
                            #pragma HLS PIPELINE II=1
                            ap_uint<512> out = 0;
                            for (int slot = 0; slot < 16; slot++) {
                                int s = w * 16 + slot;
                                ap_uint<32> val = (s < total_segs)
                                    ? (ap_uint<32>)wt_bs_data[cache_e][s]
                                    : (ap_uint<32>)0xFFFFFFFF;
                                out.range(32 * (slot + 1) - 1, 32 * slot) = val;
                            }
                            data_to_app[i].write(out);
                        }
                    } else {
                        // Cache miss: read from HBM in chunks of 16, pack, send
                        int fill_e = -1;
                        for (int e = 0; e < WT_BS_CACHE_LINES; e++) {
                            #pragma HLS UNROLL
                            if (wt_bs_pma_idx[e] == -1) { fill_e = e; }
                        }
                        if (fill_e == -1) { fill_e = 0; }
                        wt_bs_pma_idx[fill_e] = pma_idx;
                        wt_bs_segs[fill_e] = total_segs;
                        int packed_words = (total_segs + 15) / 16;
                        for (int w = 0; w < packed_words; w++) {
                            #pragma HLS PIPELINE II=1
                            ap_uint<512> out = 0;
                            for (int slot = 0; slot < 16; slot++) {
                                int s = w * 16 + slot;
                                ap_uint<32> val;
                                if (s < total_segs) {
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
                                    val = (ap_uint<32>)data_from_mem_access.read().data.range(31,0);
                                    wt_bs_data[fill_e][s] = (int)val;
                                } else {
                                    val = (ap_uint<32>)0xFFFFFFFF;
                                }
                                out.range(32 * (slot + 1) - 1, 32 * slot) = val;
                            }
                            data_to_app[i].write(out);
                        }
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
                                if (w == 3) {
                                    ap_uint<512> val = empty_pma_content;
                                    val.range(511, 480) = 0;
                                    pkt.data = val;
                                } else {
                                    pkt.data = empty_pma_content;
                                }
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
                    // Invalidate bs_helper cache for this PMA
                    for (int e = 0; e < WT_BS_CACHE_LINES; e++) {
                        #pragma HLS UNROLL
                        if (wt_bs_pma_idx[e] == wt_pma_idx) wt_bs_pma_idx[e] = -1;
                    }
                } else if (cmd.command_type == PMA_VALID_COUNT_READ) {
                    // Sum valid_count over [seg_start, seg_start+seg_len) from HBM
                    // Single bulk read, then extract word 3 of each segment
                    int seg_words_per_seg = SEGMENT_SIZE * (int)sizeof(int) / 64;
                    int offset = wt_hbm_start + (int)sizeof(PMA_HEADER)
                                 + seg_start * seg_words_per_seg * 64;
                    int total_bytes = seg_len * seg_words_per_seg * 64;
                    MEMORY_INFO_ACCESS_COMMAND rcmd = {};
                    rcmd.command_type = MEMORY_READ;
                    rcmd.pma_idx = pma_idx;
                    rcmd.start = offset;
                    rcmd.len = total_bytes;
                    rcmd.last = 0;
                    req_to_mem_access.write(rcmd);
                    int total = 0;
                    int total_words = total_bytes / 64;
                    for (int j = 0; j < total_words; j++) {
                        #pragma HLS PIPELINE II=1
                        ap_uint<512> val = data_from_mem_access.read().data;
                        if ((j % 4) == 3) {
                            total += (int)val.range(511, 480);
                        }
                    }
                    ap_uint<512> out = 0;
                    out.range(31,0) = (ap_uint<32>)total;
                    data_to_app[i].write(out);
                }
                continue; // write-through handled
            }

            // === Cache lookup: search all lines (pipelined for timing) ===
            int cache_line = -1;
            for (int line = 0; line < TOTAL_CACHE_LINES; line++)
            {
                #pragma HLS PIPELINE II=1
                if (cache_valid[line] && cache_pma_idx[line] == pma_idx)
                    cache_line = line;
            }

            // === Eviction: per-tier LRU for cache miss ===
            int evict_line = -1;
            if (cache_line == -1)
            {
                // Read vec_info to select tier and find eviction target
                MEMORY_INFO_ACCESS_COMMAND vcmd = {};
                vcmd.command_type = VEC_INFO_READ;
                vcmd.pma_idx = pma_idx;
                vcmd.start = 0; vcmd.len = 0; vcmd.last = 0;
                req_to_mem_access.write(vcmd);
                #pragma HLS protocol fixed
                ap_wait();
                VEC_INFO vi = data_from_mem_access.read().data.range(63,0);
                bool pma_exists = (vi != 0);
                int pma_len_words = pma_exists ? (int)(vi.range(63,32)) / 64
                                   : (sizeof(PMA_HEADER) + 16*SEGMENT_SIZE*sizeof(int)) / 64;

                // Tier selection: size hard-constraint + profile heat upgrade
                int tier_start, tier_end;
                if (pma_len_words > MEDIUM_SIZE) {
                    // Must go to Large (size constraint)
                    tier_start = LARGE_START; tier_end = LARGE_END;
                } else if (pma_len_words > SMALL_SIZE) {
                    // Must go to Medium (size constraint)
                    tier_start = MEDIUM_START; tier_end = MEDIUM_END;
                } else {
                    // Small enough for any tier; check profile heat
                    int prof_slot = -1;
                    for (int p = 0; p < PROFILE_TABLE_SIZE; p++) {
                        #pragma HLS UNROLL
                        if (profile_pma_idx[p] == pma_idx) { prof_slot = p; }
                    }
                    bool promoted = (prof_slot != -1) && profile_promoted[prof_slot];
                    int access = (prof_slot != -1) ? (int)profile_access_count[prof_slot] : 0;
                    // Mark as promoted for future
                    if (prof_slot != -1 && access >= PROFILE_WARM_THRESHOLD)
                        profile_promoted[prof_slot] = true;
                    if (promoted && access >= PROFILE_HOT_THRESHOLD) {
                        tier_start = LARGE_START; tier_end = LARGE_END;
                    } else if (promoted && access >= PROFILE_WARM_THRESHOLD) {
                        tier_start = MEDIUM_START; tier_end = MEDIUM_END;
                    } else {
                        tier_start = SMALL_START; tier_end = SMALL_END;
                    }
                }

                // Find free or LRU line within selected tier
                for (int line = tier_start; line < tier_end; line++) {
                    #pragma HLS PIPELINE II=1
                    if (!cache_valid[line]) { evict_line = line; }
                }
                if (evict_line == -1) {
                    ap_uint<32> max_ts = 0;
                    for (int line = tier_start; line < tier_end; line++) {
                        #pragma HLS PIPELINE II=1
                        if (line == tier_start || cache_time_stamp[line] > max_ts) {
                            max_ts = cache_time_stamp[line]; evict_line = line;
                        }
                    }
                }

                // Flush evicted line if dirty
                flush_1_cache(evict_line, cache_valid, cache_dirty, cache_pma_idx,
                              cache_old_len, cache_old_start, cache_new_len,
                              large_buf, medium_buf, small_buf,
                              req_to_alloc_access, vec_info_back_stream,
                              req_to_mem_access, data_to_mem_access);
                bs_helper_valid[evict_line] = 0;

                // Load PMA into cache
                if (pma_exists) {
                    MEMORY_INFO_ACCESS_COMMAND rcmd = {};
                    rcmd.command_type = MEMORY_READ;
                    rcmd.pma_idx = pma_idx;
                    rcmd.start = vi.range(31,0);
                    rcmd.len = vi.range(63,32);
                    rcmd.last = 0;
                    req_to_mem_access.write(rcmd);
                    int words = rcmd.len / 64;
                    for (int j = 0; j < words; j++) {
                        #pragma HLS PIPELINE II=1
                        ap_axiu<512, 0, 0, 0> packet = data_from_mem_access.read();
                        WRITE_BUF_WORD(evict_line, j, packet.data);
                    }
                    cache_valid[evict_line] = 1;
                    cache_dirty[evict_line] = 0;
                    cache_old_start[evict_line] = vi.range(31,0);
                    cache_old_len[evict_line] = vi.range(63,32);
                    cache_new_len[evict_line] = vi.range(63,32);
                } else {
                    // Brand new PMA: init in Small tier
                    int init_segs = 16;
                    int init_words = HEADER_WORDS + init_segs * SEG_WORDS_PER_SEG;
                    cache_valid[evict_line] = 1;
                    cache_dirty[evict_line] = 1;
                    cache_old_start[evict_line] = 0;
                    cache_old_len[evict_line] = 0;
                    cache_new_len[evict_line] = init_words * 64;
                    WRITE_BUF_WORD(evict_line, 0, empty_vertex_range);
                    WRITE_BUF_WORD(evict_line, 1, (ap_uint<512>)0);
                    WRITE_BUF_WORD(evict_line, 2, (ap_uint<512>)0);
                    WRITE_BUF_WORD(evict_line, 3, (ap_uint<512>)0);
                    for (int j = 4; j < init_words; j++) {
                        #pragma HLS PIPELINE II=1
                        if ((j % 4) == 3) {
                            ap_uint<512> w = empty_pma_content;
                            w.range(511, 480) = 0;
                            WRITE_BUF_WORD(evict_line, j, w);
                        } else {
                            WRITE_BUF_WORD(evict_line, j, empty_pma_content);
                        }
                    }
                }
                cache_pma_idx[evict_line] = pma_idx;
            }
            else
            {
                evict_line = cache_line;
            }
            int active_line = (cache_line == -1) ? evict_line : cache_line;
            if (cache_line != -1) {
                // Cache hit: update LRU order (22-line loop)
                add_timestamp(active_line, cache_time_stamp);
            } else {
                // Cache miss: new PMA just loaded, only reset this line's timestamp
                cache_time_stamp[active_line] = 0;
            }
            //req to read_a_pma
            if (cmd.command_type == PMA_HEADER_READ
                || cmd.command_type == PMA_HEADER_READ_HINTED)
            {
                for (int j = 0 ; j < (int)(sizeof(PMA_HEADER) / 64) ; j++)
                {
                    #pragma HLS PIPELINE II=1
                    data_to_app[i].write(READ_BUF_WORD(active_line, j));
                }
            }
            else if (cmd.command_type == PMA_SEGMENT_READ)
            {
                int seg_word_start = (int)(sizeof(PMA_HEADER) / 64) + seg_start * (SEGMENT_SIZE * (int)sizeof(int) / 64);
                int seg_words = seg_len * (SEGMENT_SIZE * (int)sizeof(int) / 64);
                // Hoist tier check outside loop to reduce combinational depth
                if (active_line < LARGE_LINES) {
                    for (int j = 0; j < seg_words; j++) {
                        #pragma HLS PIPELINE II=1
                        data_to_app[i].write(large_buf[active_line][seg_word_start + j]);
                    }
                } else if (active_line < MEDIUM_END) {
                    for (int j = 0; j < seg_words; j++) {
                        #pragma HLS PIPELINE II=1
                        data_to_app[i].write(medium_buf[active_line - MEDIUM_START][seg_word_start + j]);
                    }
                } else {
                    for (int j = 0; j < seg_words; j++) {
                        #pragma HLS PIPELINE II=1
                        data_to_app[i].write(small_buf[active_line - SMALL_START][seg_word_start + j]);
                    }
                }
            }
            else if (cmd.command_type == PMA_VALID_COUNT_READ)
            {
                // Sum valid_count over [seg_start, seg_start+seg_len) directly from cache
                int total = 0;
                int seg_words_per_seg = SEGMENT_SIZE * (int)sizeof(int) / 64;
                int header_words = (int)(sizeof(PMA_HEADER) / 64);
                for (int s = 0; s < seg_len; s++) {
                    int seg_idx = seg_start + s;
                    int word_idx = header_words + seg_idx * seg_words_per_seg + 3;
                    total += (int)READ_BUF_WORD(active_line, word_idx).range(511, 480);
                }
                ap_uint<512> out = 0;
                out.range(31,0) = (ap_uint<32>)total;
                data_to_app[i].write(out);
            }
            //req to write_a_pma
            else if (cmd.command_type == PMA_HEADER_WRITE)
            {
                for (int j = 0 ; j < (int)(sizeof(PMA_HEADER) / 64) ; j++)
                {
                    #pragma HLS PIPELINE II=1
                    WRITE_BUF_WORD(active_line, j, data_from_app[i].read());
                }
                cache_dirty[active_line] = 1;
             }
            else if (cmd.command_type == PMA_SEGMENT_WRITE)
            {
                int seg_word_start = (int)(sizeof(PMA_HEADER) / 64) + seg_start * (SEGMENT_SIZE * (int)sizeof(int) / 64);
                int seg_words = seg_len * (SEGMENT_SIZE * (int)sizeof(int) / 64);
                // Hoist tier check outside loop to reduce combinational depth
                if (active_line < LARGE_LINES) {
                    for (int j = 0; j < seg_words; j++) {
                        #pragma HLS PIPELINE II=1
                        large_buf[active_line][seg_word_start + j] = data_from_app[i].read();
                    }
                } else if (active_line < MEDIUM_END) {
                    for (int j = 0; j < seg_words; j++) {
                        #pragma HLS PIPELINE II=1
                        medium_buf[active_line - MEDIUM_START][seg_word_start + j] = data_from_app[i].read();
                    }
                } else {
                    for (int j = 0; j < seg_words; j++) {
                        #pragma HLS PIPELINE II=1
                        small_buf[active_line - SMALL_START][seg_word_start + j] = data_from_app[i].read();
                    }
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
                    old_range[v] = (int)READ_BUF_WORD(active_line, 0).range(v * 32 + 31, v * 32);
                    new_range[v] = (int)new_range_packet.range(v * 32 + 31, v * 32);
                }

                int seg_words_per_seg = SEGMENT_SIZE * (int)sizeof(int) / 64;
                int old_total_seg = old_range[VERTEX_PER_PMA - 1];
                int new_total_seg = new_range[VERTEX_PER_PMA - 1];
                int header_words = (int)(sizeof(PMA_HEADER) / 64);
                int old_total_words = header_words + old_total_seg * seg_words_per_seg;
                int new_total_words = header_words + new_total_seg * seg_words_per_seg;
                if (new_total_seg >= old_total_seg && new_total_words <= tier_max_words[active_line])
                {
                    // ---- In-place extend (fits in current tier) ----
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
                                WRITE_BUF_WORD(active_line, dst_word_idx, READ_BUF_WORD(active_line, src_word_idx));
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
                                    if (w == 3) {
                                        ap_uint<512> val = empty_pma_content;
                                        val.range(511, 480) = 0;
                                        WRITE_BUF_WORD(active_line, fill_word_idx, val);
                                    } else {
                                        WRITE_BUF_WORD(active_line, fill_word_idx, empty_pma_content);
                                    }
                                }
                            }
                        }
                    }

                    WRITE_BUF_WORD(active_line, 0, new_range_packet);
                    cache_new_len[active_line] = sizeof(PMA_HEADER) + new_total_seg * SEGMENT_SIZE * sizeof(int);
                    cache_dirty[active_line] = 1;
                    bs_helper_valid[active_line] = 0;
                }
                else if (new_total_seg >= old_total_seg)
                {
                    // ---- Outgrew current tier: upgrade or HBM fallback ----
                    if (new_total_words > LARGE_SIZE) {
                        // ---- HBM fallback: too large for any cache tier ----
                        // Flush old data to HBM
                        flush_1_cache(active_line, cache_valid, cache_dirty, cache_pma_idx,
                                      cache_old_len, cache_old_start, cache_new_len,
                                      large_buf, medium_buf, small_buf,
                                      req_to_alloc_access, vec_info_back_stream,
                                      req_to_mem_access, data_to_mem_access);
                        int old_hbm_start = cache_old_start[active_line];
                        int old_hbm_len   = cache_old_len[active_line];
                        int old_size = (int)sizeof(PMA_HEADER) + old_total_seg * SEGMENT_SIZE * (int)sizeof(int);
                        int new_size = (int)sizeof(PMA_HEADER) + new_total_seg * SEGMENT_SIZE * (int)sizeof(int);

                        // Alloc new space
                        MEMORY_INFO_ACCESS_COMMAND alloc_cmd = {};
                        alloc_cmd.command_type = VEC_INFO_ALLOC;
                        alloc_cmd.pma_idx = pma_idx;
                        alloc_cmd.len = new_size;
                        alloc_cmd.start = 0; alloc_cmd.last = 0;
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

                        // Write header
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
                            pkt.data = (j == 0) ? new_range_packet
                                     : READ_BUF_WORD(active_line, j);
                            pkt.last = (j == 3) ? 1 : 0;
                            data_to_mem_access.write(pkt);
                        }

                        // Copy + shift segments from cache to HBM
                        for (int v = VERTEX_PER_PMA - 1; v >= 0; v--) {
                            int old_v_start = (v == 0) ? 0 : old_range[v - 1];
                            int old_v_end = old_range[v];
                            int new_v_start = (v == 0) ? 0 : new_range[v - 1];
                            int new_v_end = new_range[v];
                            for (int s = 0; s < old_v_end - old_v_start; s++) {
                                int new_off = new_hbm_start + (int)sizeof(PMA_HEADER)
                                              + (new_v_start + s) * seg_words_per_seg * 64;
                                MEMORY_INFO_ACCESS_COMMAND wcmd = {};
                                wcmd.command_type = MEMORY_WRITE;
                                wcmd.pma_idx = pma_idx;
                                wcmd.start = new_off;
                                wcmd.len = seg_words_per_seg * 64;
                                wcmd.last = 0;
                                req_to_mem_access.write(wcmd);
                                int old_word_base = header_words + (old_v_start + s) * seg_words_per_seg;
                                for (int w = 0; w < seg_words_per_seg; w++) {
                                    #pragma HLS PIPELINE II=1
                                    ap_axiu<512, 0, 0, 0> pkt;
                                    pkt.data = READ_BUF_WORD(active_line, old_word_base + w);
                                    pkt.last = (w == seg_words_per_seg - 1) ? 1 : 0;
                                    data_to_mem_access.write(pkt);
                                }
                            }
                            // Fill extra new segments
                            int extra = (new_v_end - new_v_start) - (old_v_end - old_v_start);
                            for (int s = 0; s < extra; s++) {
                                int new_off = new_hbm_start + (int)sizeof(PMA_HEADER)
                                              + (new_v_start + old_v_end - old_v_start + s) * seg_words_per_seg * 64;
                                MEMORY_INFO_ACCESS_COMMAND wcmd = {};
                                wcmd.command_type = MEMORY_WRITE;
                                wcmd.pma_idx = pma_idx; wcmd.start = new_off;
                                wcmd.len = seg_words_per_seg * 64; wcmd.last = 0;
                                req_to_mem_access.write(wcmd);
                                for (int w = 0; w < seg_words_per_seg; w++) {
                                    #pragma HLS PIPELINE II=1
                                    ap_axiu<512, 0, 0, 0> pkt;
                                    if (w == 3) {
                                        ap_uint<512> val = empty_pma_content;
                                        val.range(511, 480) = 0;
                                        pkt.data = val;
                                    } else {
                                        pkt.data = empty_pma_content;
                                    }
                                    pkt.last = (w == seg_words_per_seg - 1) ? 1 : 0;
                                    data_to_mem_access.write(pkt);
                                }
                            }
                        }

                        // Free old allocation
                        if (old_hbm_len > 0) {
                            MEMORY_INFO_ACCESS_COMMAND free_cmd = {};
                            free_cmd.command_type = VEC_INFO_FREE;
                            free_cmd.pma_idx = pma_idx;
                            free_cmd.start = old_hbm_start;
                            free_cmd.len = old_hbm_len;
                            free_cmd.last = 0;
                            req_to_alloc_access.write(free_cmd);
                        }

                        // Invalidate cache line — PMA now lives in HBM
                        cache_valid[active_line] = 0;
                        cache_pma_idx[active_line] = -1;
                        bs_helper_valid[active_line] = 0;
                    } else {
                        // ---- Tier upgrade: move to larger cache tier ----
                        int new_tier_start, new_tier_end;
                        if (new_total_words > MEDIUM_SIZE)
                            { new_tier_start = LARGE_START;  new_tier_end = LARGE_END; }
                        else
                            { new_tier_start = MEDIUM_START; new_tier_end = MEDIUM_END; }

                        // Find free line or evict LRU in target tier
                        int new_line = -1;
                        for (int line = new_tier_start; line < new_tier_end; line++) {
                            if (!cache_valid[line]) { new_line = line; break; }
                        }
                        if (new_line == -1) {
                            ap_uint<32> max_ts = 0;
                            for (int line = new_tier_start; line < new_tier_end; line++) {
                                if (line == new_tier_start || cache_time_stamp[line] > max_ts) {
                                    max_ts = cache_time_stamp[line]; new_line = line;
                                }
                            }
                            flush_1_cache(new_line, cache_valid, cache_dirty, cache_pma_idx,
                                          cache_old_len, cache_old_start, cache_new_len,
                                          large_buf, medium_buf, small_buf,
                                          req_to_alloc_access, vec_info_back_stream,
                                          req_to_mem_access, data_to_mem_access);
                            bs_helper_valid[new_line] = 0;
                        }

                        // Copy all old words from active_line to new_line
                        for (int w = 0; w < old_total_words; w++) {
                            #pragma HLS PIPELINE II=1
                            WRITE_BUF_WORD(new_line, w, READ_BUF_WORD(active_line, w));
                        }

                        // Segment shift in new_line
                        for (int v = VERTEX_PER_PMA - 1; v >= 0; v--) {
                            int old_start = (v == 0) ? 0 : old_range[v - 1];
                            int old_end = old_range[v];
                            int old_seg_num = old_end - old_start;
                            int new_start = (v == 0) ? 0 : new_range[v - 1];
                            for (int s = old_seg_num - 1; s >= 0; s--) {
                                for (int w = 0; w < seg_words_per_seg; w++) {
                                    int src = header_words + (old_start + s) * seg_words_per_seg + w;
                                    int dst = header_words + (new_start + s) * seg_words_per_seg + w;
                                    WRITE_BUF_WORD(new_line, dst, READ_BUF_WORD(new_line, src));
                                }
                            }
                        }

                        // Fill extra segments in new_line
                        for (int v = 0; v < VERTEX_PER_PMA; v++) {
                            int old_start = (v == 0) ? 0 : old_range[v - 1];
                            int old_end = old_range[v];
                            int old_seg_num = old_end - old_start;
                            int new_start = (v == 0) ? 0 : new_range[v - 1];
                            int new_end = new_range[v];
                            int extra_seg_num = new_end - new_start - old_seg_num;
                            if (extra_seg_num > 0) {
                                int fill_start_seg = new_start + old_seg_num;
                                for (int s = 0; s < extra_seg_num; s++) {
                                    for (int w = 0; w < seg_words_per_seg; w++) {
                                        #pragma HLS PIPELINE II=1
                                        int fwi = header_words + (fill_start_seg + s) * seg_words_per_seg + w;
                                        if (w == 3) {
                                            ap_uint<512> val = empty_pma_content;
                                            val.range(511, 480) = 0;
                                            WRITE_BUF_WORD(new_line, fwi, val);
                                        } else {
                                            WRITE_BUF_WORD(new_line, fwi, empty_pma_content);
                                        }
                                    }
                                }
                            }
                        }

                        // Update new_line metadata
                        WRITE_BUF_WORD(new_line, 0, new_range_packet);
                        cache_pma_idx[new_line] = pma_idx;
                        cache_old_start[new_line] = cache_old_start[active_line];
                        cache_old_len[new_line]   = cache_old_len[active_line];
                        cache_new_len[new_line]   = sizeof(PMA_HEADER) + new_total_seg * SEGMENT_SIZE * sizeof(int);
                        cache_valid[new_line] = 1;
                        cache_dirty[new_line] = 1;
                        cache_time_stamp[new_line] = 0;
                        bs_helper_valid[new_line] = 0;

                        // Release old line
                        cache_valid[active_line] = 0;
                        cache_pma_idx[active_line] = -1;
                        bs_helper_valid[active_line] = 0;
                    }
                }
            }
            else if (cmd.command_type == PMA_BS_HELPER_READ)
            {
                if (!bs_helper_valid[active_line])
                {
                    int total_segs = (int)READ_BUF_WORD(active_line, 0).range(15 * 32 + 31, 15 * 32);
                    int header_words = (int)(sizeof(PMA_HEADER) / 64);
                    int seg_words_per_seg = SEGMENT_SIZE * (int)sizeof(int) / 64;
                    for (int s = 0; s < total_segs; s++)
                    {
                        int seg_word_start = header_words + s * seg_words_per_seg;
                        int line = active_line;
                        ap_uint<512> val = READ_BUF_WORD(active_line, seg_word_start);
                        if (line < LARGE_LINES) large_bs_cache[line][s] = val.range(31,0);
                        else if (line < MEDIUM_END) medium_bs_cache[line - MEDIUM_START][s] = val.range(31,0);
                        else small_bs_cache[line - SMALL_START][s] = val.range(31,0);
                    }
                    bs_helper_valid[active_line] = 1;
                }
                int total_segs = (int)READ_BUF_WORD(active_line, 0).range(15 * 32 + 31, 15 * 32);
                // Pack 16 segments per 512-bit word, padding with -1
                int packed_words = (total_segs + 15) / 16;
                for (int w = 0; w < packed_words; w++)
                {
                    #pragma HLS PIPELINE II=1
                    ap_uint<512> out = 0;
                    int line = active_line;
                    for (int slot = 0; slot < 16; slot++) {
                        int s = w * 16 + slot;
                        ap_uint<32> val;
                        if (s < total_segs) {
                            if (line < LARGE_LINES) val = large_bs_cache[line][s];
                            else if (line < MEDIUM_END) val = medium_bs_cache[line - MEDIUM_START][s];
                            else val = small_bs_cache[line - SMALL_START][s];
                        } else {
                            val = (ap_uint<32>)0xFFFFFFFF;
                        }
                        out.range(32 * (slot + 1) - 1, 32 * slot) = val;
                    }
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
                for (int line = 0; line < TOTAL_CACHE_LINES; line++)
                {
                    if (cache_valid[line] && cache_dirty[line])
                    {
                        flush_1_cache(line, cache_valid, cache_dirty, cache_pma_idx,
                                      cache_old_len, cache_old_start, cache_new_len,
                                      large_buf, medium_buf, small_buf, req_to_alloc_access, vec_info_back_stream,
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
    for (int line = 0; line < TOTAL_CACHE_LINES; line++) {
        if (cache_valid[line] && cache_dirty[line]) {
            flush_1_cache(line, cache_valid, cache_dirty, cache_pma_idx,
                          cache_old_len, cache_old_start, cache_new_len,
                          large_buf, medium_buf, small_buf,
                          req_to_alloc_access, vec_info_back_stream,
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
                info.pma_idx = first.src >> 4;
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
                info.pma_idx = cmd_batch[0].src >> 4;
                info.command_type = INSERT_GOING;
                pma_info_stream.write(info);

                PMA_UPDATE_MESSAGE edge_info[MAX_CMD_NUM];
                #pragma HLS ARRAY_PARTITION variable=edge_info complete

                // collect edges + pad unused slots with MAX sentinel
                for (int i = 0; i < cmd_count; i++) {
                    #pragma HLS PIPELINE II=1
                    edge_info[i] = cmd_batch[i];
                }
                for (int i = cmd_count; i < MAX_CMD_NUM; i++) {
                    #pragma HLS PIPELINE II=1
                    edge_info[i].src = 0x7FFFFFFF;
                    edge_info[i].dst = 0x7FFFFFFF;
                    edge_info[i].opcode = (PMA_UPDATE_OPCODE)0x7FFFFFFF;
                }

                // Batcher odd-even mergesort: 10 stages, 63 cmp-swap ops
                #define BCS(a,b) do { \
                    PMA_UPDATE_MESSAGE _ta = edge_info[a], _tb = edge_info[b]; \
                    bool _gt = ((int)_ta.src > (int)_tb.src) || \
                        ((int)_ta.src == (int)_tb.src && ((int)_ta.dst > (int)_tb.dst || \
                        ((int)_ta.dst == (int)_tb.dst && (int)_ta.opcode > (int)_tb.opcode))); \
                    if (_gt) { edge_info[a] = _tb; edge_info[b] = _ta; } \
                } while(0)
                // p=1 k=1
                BCS( 0, 1); BCS( 2, 3); BCS( 4, 5); BCS( 6, 7);
                BCS( 8, 9); BCS(10,11); BCS(12,13); BCS(14,15);
                // p=2 k=2
                BCS( 0, 2); BCS( 1, 3); BCS( 4, 6); BCS( 5, 7);
                BCS( 8,10); BCS( 9,11); BCS(12,14); BCS(13,15);
                // p=2 k=1
                BCS( 1, 2); BCS( 5, 6); BCS( 9,10); BCS(13,14);
                // p=4 k=4
                BCS( 0, 4); BCS( 1, 5); BCS( 2, 6); BCS( 3, 7);
                BCS( 8,12); BCS( 9,13); BCS(10,14); BCS(11,15);
                // p=4 k=2
                BCS( 2, 4); BCS( 3, 5); BCS(10,12); BCS(11,13);
                // p=4 k=1
                BCS( 1, 2); BCS( 3, 4); BCS( 5, 6);
                BCS( 9,10); BCS(11,12); BCS(13,14);
                // p=8 k=8
                BCS( 0, 8); BCS( 1, 9); BCS( 2,10); BCS( 3,11);
                BCS( 4,12); BCS( 5,13); BCS( 6,14); BCS( 7,15);
                // p=8 k=4
                BCS( 4, 8); BCS( 5, 9); BCS( 6,10); BCS( 7,11);
                // p=8 k=2
                BCS( 2, 4); BCS( 3, 5); BCS( 6, 8); BCS( 7, 9);
                BCS(10,12); BCS(11,13);
                // p=8 k=1
                BCS( 1, 2); BCS( 3, 4); BCS( 5, 6); BCS( 7, 8);
                BCS( 9,10); BCS(11,12); BCS(13,14);
                #undef BCS

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
void sub_search(
    int* cache_bs_helper,
    int seg_low,
    int seg_high,
    int dst,
    int &searched_seg_number
)
{
    #pragma HLS INLINE
    int range = seg_high - seg_low;
    // Power-law graphs: most vertices have 1-4 segments → linear scan wins
    if (range <= 4) {
        searched_seg_number = range - 1;
        for (int s = seg_low; s < seg_high; s++) {
            #pragma HLS UNROLL
            if (cache_bs_helper[s] >= dst) {
                searched_seg_number = s - seg_low;
                return;
            }
        }
    } else {
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
        // Fire both header and bs_helper read requests before waiting,
        // so cache_mechanism can process them back-to-back.
        {
            PMA_INFO_ACCESS_COMMAND hdr_cmd = {};
            hdr_cmd.command_type = PMA_HEADER_READ_HINTED;
            hdr_cmd.pma_idx = signal.pma_idx;
            hdr_cmd.seg_start = count;
            hdr_cmd.seg_len = 0;
            memory_access_stream.write(hdr_cmd);
            // No ap_wait here — fire bs_helper request immediately
            PMA_INFO_ACCESS_COMMAND bs_cmd = {};
            bs_cmd.command_type = PMA_BS_HELPER_READ;
            bs_cmd.pma_idx = signal.pma_idx;
            bs_cmd.seg_start = 0;
            bs_cmd.seg_len = 0;
            memory_access_stream.write(bs_cmd);
            ap_wait(); // both requests committed, responses ready

            // unpack header
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
            #pragma HLS UNROLL
            inserted_seg_info.old_vertex_relative_seg_number[i] = -1;
            inserted_seg_info.vertex_number[i] = -1;
            deleted_seg_info.old_vertex_relative_seg_number[i] = -1;
            deleted_seg_info.vertex_number[i] = -1;
        }

        int bs_helper[10240];
        #pragma HLS BIND_STORAGE variable=bs_helper type=RAM_2P impl=URAM
        // bs_helper response: packed 16 segments per 512-bit word
        {
            int total_segs = header.vertex_range[VERTEX_PER_PMA - 1];
            int packed_words = (total_segs + 15) / 16;
            for (int w = 0; w < packed_words; w++)
            {
                #pragma HLS PIPELINE II=1
                ap_uint<512> word = data_from_mem.read();
                for (int slot = 0; slot < 16; slot++) {
                    int s = w * 16 + slot;
                    if (s < total_segs) {
                        bs_helper[s] = (int)word.range(32 * (slot + 1) - 1, 32 * slot);
                    }
                }
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
        // Parallel sub_search: collect args first, then search in batch
        int sub_dst[16];
        int sub_low[16];
        int sub_high[16];
        int sub_result[16];
        int sub_type[16]; // 0=insert, 1=delete_edge, -1=none
        int sub_count = 0;

        for (int i = 0 ; i < count ; i++)
        {
            #pragma HLS PIPELINE II=1
            PMA_UPDATE_MESSAGE edge_info = edge_info_stream.read();

            int opcode = (int)edge_info.opcode;
            int src = edge_info.src;
            int dst = edge_info.dst;
            if (opcode == PMA_UPDATE_INSERT_EDGE || opcode == PMA_UPDATE_DELETE_EDGE)
            {
                int local_src = src & 0xF;
                int low_seg = local_src == 0 ? 0 : header.vertex_range[local_src - 1];
                int high_seg = header.vertex_range[local_src];
                sub_dst[sub_count] = dst;
                sub_low[sub_count] = low_seg;
                sub_high[sub_count] = high_seg;
                sub_type[sub_count] = (opcode == PMA_UPDATE_INSERT_EDGE) ? 0 : 1;
                sub_count++;
            }
            if (opcode == PMA_UPDATE_INSERT_EDGE)
            {
                ap_uint<64> payload = 0;
                payload.range(63,32) = (ap_uint<32>)src;
                payload.range(31,0) = (ap_uint<32>)dst;
                insert_ops[insert_ops_count++] = payload;
            }
            else if (opcode == PMA_UPDATE_DELETE_EDGE)
            {
                ap_uint<64> payload = 0;
                payload.range(63,32) = (ap_uint<32>)src;
                payload.range(31,0) = (ap_uint<32>)dst;
                delete_edge_ops[delete_edge_ops_count++] = payload;
            }
            else if (opcode == PMA_UPDATE_DELETE_VERTEX)
            {
                ap_uint<64> payload = 0;
                payload.range(63,32) = src;
                payload.range(31,0) = dst;
                delete_vertex_ops[delete_vertex_ops_count++] = payload;
            }
        }

        // 16-way parallel sub_search
        for (int i = 0; i < 16; i++) {
            #pragma HLS UNROLL
            if (i < sub_count) {
                sub_result[i] = -1;
                sub_search(bs_helper, sub_low[i], sub_high[i], sub_dst[i], sub_result[i]);
                if (sub_result[i] == -1) sub_result[i] = 0;
            }
        }

        // Distribute results to insert/delete arrays
        int ii = 0, di = 0;
        for (int i = 0; i < sub_count; i++) {
            #pragma HLS PIPELINE II=1
            if (sub_type[i] == 0) {
                int src = insert_ops[ii].range(63,32);
                inserted_seg_info.old_vertex_relative_seg_number[ii] = sub_result[i];
                inserted_seg_info.vertex_number[ii] = src & 0xF;
                ii++;
            } else {
                int src = delete_edge_ops[di].range(63,32);
                deleted_seg_info.old_vertex_relative_seg_number[di] = sub_result[i];
                deleted_seg_info.vertex_number[di] = src & 0xF;
                di++;
            }
        }

        // ---- Pre-compute target segments for all ops (avoids re-computation in per-segment loops) ----
        int del_target_seg[16], del_lsrc[16], del_dst[16];
        for (int i = 0; i < delete_edge_ops_count; i++) {
            #pragma HLS UNROLL
            int src = delete_edge_ops[i].range(63,32);
            del_dst[i] = delete_edge_ops[i].range(31,0);
            del_lsrc[i] = src & 0xF;
            int low_seg = del_lsrc[i] == 0 ? 0 : header.vertex_range[del_lsrc[i] - 1];
            del_target_seg[i] = deleted_seg_info.old_vertex_relative_seg_number[i] + low_seg;
        }
        int ins_target_seg[16], ins_lsrc[16], ins_dst[16];
        for (int i = 0; i < insert_ops_count; i++) {
            #pragma HLS UNROLL
            int src = insert_ops[i].range(63,32);
            ins_dst[i] = insert_ops[i].range(31,0);
            ins_lsrc[i] = src & 0xF;
            int low_seg = ins_lsrc[i] == 0 ? 0 : header.vertex_range[ins_lsrc[i] - 1];
            ins_target_seg[i] = inserted_seg_info.old_vertex_relative_seg_number[i] + low_seg;
        }
        // Pre-compute delete_vertex segment ranges
        int dv_low_seg[16], dv_high_seg[16], dv_lsrc[16];
        for (int i = 0; i < delete_vertex_ops_count; i++) {
            #pragma HLS UNROLL
            int src = delete_vertex_ops[i].range(63,32);
            dv_lsrc[i] = src & 0xF;
            dv_low_seg[i] = dv_lsrc[i] == 0 ? 0 : header.vertex_range[dv_lsrc[i] - 1];
            dv_high_seg[i] = header.vertex_range[dv_lsrc[i]];
        }

        // Pre-scan: collect all unique segments from all ops
        int seg_list[16];
        int seg_count = 0;
        int seg_del_cnt[16];
        int seg_ins_cnt[16];
        int seg_dv_cnt[16];
        for (int i = 0; i < 16; i++) {
            #pragma HLS UNROLL
            seg_del_cnt[i] = 0; seg_ins_cnt[i] = 0; seg_dv_cnt[i] = 0;
        }

        for (int i = 0 ; i < delete_vertex_ops_count; i++) {
            for (int s = dv_low_seg[i]; s < dv_high_seg[i]; s++) {
                bool found = false;
                for (int j = 0; j < seg_count; j++) {
                    if (seg_list[j] == s) { found = true; seg_dv_cnt[j]++; break; }
                }
                if (!found) { seg_list[seg_count] = s; seg_dv_cnt[seg_count] = 1; seg_count++; }
                header.edge_count[dv_lsrc[i]] = 0;
            }
        }
        for (int i = 0 ; i < delete_edge_ops_count ; i++) {
            int ts = del_target_seg[i];
            bool found = false;
            for (int j = 0; j < seg_count; j++) {
                if (seg_list[j] == ts) { found = true; seg_del_cnt[j]++; break; }
            }
            if (!found) { seg_list[seg_count] = ts; seg_del_cnt[seg_count] = 1; seg_count++; }
        }
        for (int i = 0 ; i < insert_ops_count ; i++) {
            int ts = ins_target_seg[i];
            bool found = false;
            for (int j = 0; j < seg_count; j++) {
                if (seg_list[j] == ts) { found = true; seg_ins_cnt[j]++; break; }
            }
            if (!found) { seg_list[seg_count] = ts; seg_ins_cnt[seg_count] = 1; seg_count++; }
        }

        // Process each unique segment: read → delete → insert → write
        int seg_buf[SEGMENT_SIZE];
        int tmp[SEGMENT_SIZE];
        #pragma HLS ARRAY_PARTITION variable=tmp complete
        for (int si = 0; si < seg_count; si++) {
            // Pre-fill tmp with UNUSED_ICON (1 cycle via UNROLL)
            for (int k = 0; k < EDGE_SLOTS_PER_SEG; k++) {
                #pragma HLS UNROLL
                tmp[k] = UNUSED_ICON;
            }
            int seg = seg_list[si];
            read_seg(memory_access_stream, data_from_mem, seg_buf, signal.pma_idx, seg);

            // Delete vertex: clear segment to UNUSED
            if (seg_dv_cnt[si] > 0) {
                for (int e = 0; e < EDGE_SLOTS_PER_SEG; e++) {
                    #pragma HLS UNROLL
                    seg_buf[e] = UNUSED_ICON;
                }
                seg_buf[VALID_COUNT_IDX] = 0;
            }

            // Collect delete dst for this segment (pre-computed target_seg, O(1) comparison)
            int del_dst_local[16]; int del_cnt = 0;
            for (int i = 0 ; i < delete_edge_ops_count ; i++) {
                #pragma HLS UNROLL
                if (del_target_seg[i] == seg) {
                    del_dst_local[del_cnt++] = del_dst[i];
                    header.edge_count[del_lsrc[i]]--;
                }
            }
            // Collect insert dst for this segment
            int ins_dst_local[16]; int ins_cnt = 0;
            for (int i = 0 ; i < insert_ops_count ; i++) {
                #pragma HLS UNROLL
                if (ins_target_seg[i] == seg) {
                    ins_dst_local[ins_cnt++] = ins_dst[i];
                    header.edge_count[ins_lsrc[i]]++;
                }
            }

            // Merge: compact (delete) + 2-way merge (insert) → tmp
            {
                int valid = seg_buf[VALID_COUNT_IDX];
                // Pass 1: compact seg_buf by removing deleted edges
                int w = 0, d = 0;
                for (int r = 0; r < valid; r++) {
                    #pragma HLS PIPELINE II=1
                    if (d < del_cnt && seg_buf[r] == del_dst_local[d]) {
                        d++;
                    } else {
                        seg_buf[w++] = seg_buf[r];
                    }
                }
                int new_valid = w;

                // Pass 2: 2-way merge seg_buf[0..new_valid) + ins_dst → tmp (pre-filled with UNUSED)
                int s = 0, i = 0, n = 0;
                while (s < new_valid && i < ins_cnt) {
                    #pragma HLS PIPELINE II=1
                    if (seg_buf[s] <= ins_dst_local[i])
                        tmp[n++] = seg_buf[s++];
                    else
                        tmp[n++] = ins_dst_local[i++];
                }
                while (s < new_valid) {
                    #pragma HLS PIPELINE II=1
                    tmp[n++] = seg_buf[s++];
                }
                while (i < ins_cnt) {
                    #pragma HLS PIPELINE II=1
                    tmp[n++] = ins_dst_local[i++];
                }
                tmp[VALID_COUNT_IDX] = new_valid + ins_cnt;
            }
            write_seg(memory_access_stream, data_to_mem, tmp, signal.pma_idx, seg);
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
    return (seg_count * EDGE_SLOTS_PER_SEG * 3) >> 2;
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

    // 1. 读取所有段，收集所有非 UNUSED 元素 (skip valid_count at position 63)
    for (int s = seg_start; s < seg_end; s++) {
        int buf[SEGMENT_SIZE];
        read_seg(memory_access_stream, data_from_mem, buf, pma_idx, s);
        for (int k = 0; k < EDGE_SLOTS_PER_SEG; k++) {
            if (buf[k] != UNUSED_ICON) {
                tmp[tmp_idx++] = buf[k];
            }
        }
    }

    // 2. 均匀写入，同时设置 valid_count
    int base = total_valid_count / total_segs;
    int extra = total_valid_count % total_segs;
    int pos = 0;
    for (int w = seg_start; w < seg_end; w++) {
        int fill = base + ((w - seg_start) < extra ? 1 : 0);
        int wbuf[SEGMENT_SIZE];
        #pragma HLS ARRAY_PARTITION variable=wbuf complete
        // Pre-fill with UNUSED_ICON (1 cycle via UNROLL)
        for (int k = 0; k < EDGE_SLOTS_PER_SEG; k++) {
            #pragma HLS UNROLL
            wbuf[k] = UNUSED_ICON;
        }
        // Copy valid entries (no conditional per element)
        for (int k = 0; k < fill; k++) {
            #pragma HLS PIPELINE II=1
            wbuf[k] = tmp[pos++];
        }
        wbuf[VALID_COUNT_IDX] = fill;
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

    // Read all words: cache_mechanism returns seg_len * 4 words per PMA_SEGMENT_READ
    PMA_INFO_ACCESS_COMMAND command;
    command.pma_idx = pma_idx;
    command.seg_start = seg_start;
    command.seg_len = total_segs;
    command.command_type = PMA_SEGMENT_READ;
    memory_access_stream.write(command);

    int count = 0;
    int total_words = total_segs * 4;
    for (int i = 0 ; i < total_words ; i++)
    {
        #pragma HLS PIPELINE II=1
        ap_uint<512> val = data_from_mem.read();
        // Extract valid_count from word 3 within each segment (int 15 = bits 511:480)
        if ((i % 4) == 3) {
            count += (int)val.range(511, 480); // VALID_COUNT_IDX=63 → int 15 in word 3
        }
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
            #pragma HLS UNROLL
            enlarge_segs[i] = 0;
        }
        bool enlarged = false;
        for (int v = 0 ; v < VERTEX_PER_PMA ; v++)
        {
            #pragma HLS PIPELINE II=1
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