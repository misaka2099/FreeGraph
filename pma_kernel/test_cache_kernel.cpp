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
#include <iostream>

static void print_cache_line(ap_uint<512>* line_buf, int header_words, int seg_words_per_seg, int /*total_seg*/, int pma_idx)
{
    printf("--------------------------------------------------\n");
    printf("[cache][dump] PMA idx=%d\n", pma_idx);
    ap_uint<512> header_word = line_buf[0];
    unsigned new_range[VERTEX_PER_PMA];
    printf("[cache][dump] PMA new_range:");
    for (int h = 0; h < VERTEX_PER_PMA; ++h) {
        new_range[h] = (unsigned)header_word.range(h*32+31, h*32);
        printf(" %u", new_range[h]);
    }
    printf("\n");

    // For each source vertex, iterate segments in its range and print src->dst pairs
    // for (int src = 0; src < VERTEX_PER_PMA; ++src) {
    //     unsigned seg_start = (src == 0) ? 0 : new_range[src-1];
    //     unsigned seg_end = new_range[src];
    //     for (unsigned s = seg_start; s < seg_end; ++s) {
    //         int first_word_idx = header_words + s * seg_words_per_seg;
    //         for (int w = 0; w < seg_words_per_seg; ++w) {
    //             ap_uint<512> big = line_buf[first_word_idx + w];
    //             for (int b = 0; b < 16; ++b) {
    //                 unsigned dst = (unsigned)big.range(b*32+31, b*32);
    //                 printf("[cache][dump] edge src=%d dst=%u\n", src, dst);
    //             }
    //         }
    //     }
    // }
}

#define MAX_CACHE_SIZE 8192
#define CACHE_LINE_SIZE 8

#define MEM_REQUEST_NUM 2

void add_timestamp(
    int now_op_cache_tag,
    ap_uint<32> cache_time_stamp[CACHE_LINE_SIZE]
)
{
    #pragma HLS INLINE
    for (int i = 0 ; i < CACHE_LINE_SIZE ; i++)
    {
        if (i != now_op_cache_tag)
        {
            cache_time_stamp[i]++;
        }
    }
    cache_time_stamp[now_op_cache_tag] = 0;
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
    #pragma HLS bind_storage variable=cache_buffer type=ram_2p impl=BRAM
    ap_uint<32> cache_time_stamp[CACHE_LINE_SIZE];
    #pragma HLS bind_storage variable=cache_time_stamp type=ram_1p impl=BRAM
    // ap_uint<1> cache_valid[CACHE_LINE_SIZE];
    // ap_uint<1> cache_dirty[CACHE_LINE_SIZE];
    ap_uint<CACHE_LINE_SIZE> cache_valid = 0;
    ap_uint<CACHE_LINE_SIZE> cache_dirty = 0;
    ap_uint<32> cache_old_len[CACHE_LINE_SIZE];
    ap_uint<32> cache_old_start[CACHE_LINE_SIZE];
    ap_uint<32> cache_new_len[CACHE_LINE_SIZE];
    int cache_pma_idx[CACHE_LINE_SIZE];

    ap_uint<MEM_REQUEST_NUM> finish_flag = -1; // all bits set to 1, meaning all requests are not finished
    for (int i = 0 ; i < CACHE_LINE_SIZE ; i++)
    {
        cache_time_stamp[i] = 0;
        cache_old_len[i] = 0;
        cache_old_start[i] = 0;
        cache_new_len[i] = 0;
        cache_pma_idx[i] = -1;
    }
    const PMA_HEADER empty_header = {
        .vertex_range = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16},
        .edge_count =   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        .attr = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
    };
    #pragma HLS bind_storage variable=empty_header type=ram_1p impl=URAM

    const ap_uint<512> empty_pma_content = -1;//UNUSED_INCON = -1 OTHER NEED TO BE INITIAL MANUALLY
    #pragma HLS bind_storage variable=empty_pma_content type=ram_1p impl=URAM
        const int HEADER_WORDS = (int)(sizeof(PMA_HEADER) / 64);
        const int SEG_WORDS_PER_SEG = SEGMENT_SIZE * (int)sizeof(int) / 64;
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
                if (cache_dirty[evict_line])
                {
                    int evict_pma_idx = cache_pma_idx[evict_line];
                    printf("[cache] Evicting line=%d pma_idx=%d old_start=%u old_len=%u new_len=%u\n", evict_line, evict_pma_idx, (unsigned)cache_old_start[evict_line], (unsigned)cache_old_len[evict_line], (unsigned)cache_new_len[evict_line]);
                    int to_write_start = -1;
                    int to_write_len = -1;
                    if (cache_new_len[evict_line] > cache_old_len[evict_line])
                    {
                        auto new_len = cache_new_len[evict_line];
                        printf("[cache] Request VEC_INFO_ALLOC pma_idx=%d len=%u\n", evict_pma_idx, (unsigned)new_len);
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
                        printf("[cache] Alloc returned for pma_idx=%d start=%u len=%u\n", evict_pma_idx, (unsigned)new_addr.range(31,0), (unsigned)new_addr.range(63,32));

                        auto old_start = cache_old_start[evict_line];
                        auto old_len = cache_old_len[evict_line];

                        if (old_len != 0)
                        {
                            MEMORY_INFO_ACCESS_COMMAND free_req = {};
                            free_req.command_type = VEC_INFO_FREE;
                            free_req.pma_idx = evict_pma_idx;
                            free_req.len = old_len;
                            free_req.start = old_start;
                            free_req.last = 0;
                            printf("[cache] Request VEC_INFO_FREE pma_idx=%d start=%u len=%u\n", evict_pma_idx, (unsigned)old_start, (unsigned)old_len);
                            req_to_alloc_access.write(free_req);
                        }
                        
                        MEMORY_INFO_ACCESS_COMMAND vec_info_write_cmd = {};
                        vec_info_write_cmd.command_type = VEC_INFO_WRITE;
                        vec_info_write_cmd.pma_idx = evict_pma_idx;
                        vec_info_write_cmd.start = new_addr.range(31,0);
                        vec_info_write_cmd.len = new_addr.range(63,32);
                        vec_info_write_cmd.last = 0;
                        printf("[cache] Request VEC_INFO_WRITE pma_idx=%d start=%u len=%u\n", evict_pma_idx, (unsigned)vec_info_write_cmd.start, (unsigned)vec_info_write_cmd.len);
                        req_to_mem_access.write(vec_info_write_cmd);

                        to_write_len = new_len;
                        to_write_start = new_addr.range(31,0);
                    }
                    else
                    {
                        to_write_start = cache_old_start[evict_line];
                        to_write_len = cache_new_len[evict_line];
                    }

                    MEMORY_INFO_ACCESS_COMMAND write_cmd = {};
                    write_cmd.command_type = MEMORY_WRITE;
                    write_cmd.pma_idx = evict_pma_idx;
                    write_cmd.len = to_write_len;
                    write_cmd.start = to_write_start;
                    write_cmd.last = 0;
                    printf("[cache] Request MEMORY_WRITE pma_idx=%d start=%u len=%u\n", evict_pma_idx, (unsigned)write_cmd.start, (unsigned)write_cmd.len);
                    req_to_mem_access.write(write_cmd);
                    for (int j = 0 ; j < to_write_len / 64 ; j++)
                    {
                        #pragma HLS PIPELINE II = 1
                        ap_axiu<512, 0, 0, 0> packet;
                        packet.data = cache_buffer[evict_line][j];
                        packet.last = (j == to_write_len / 64 - 1) ? 1 : 0;
                        data_to_mem_access.write(packet);
                    }
                }
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
                req_to_mem_access.write(vec_info_read_cmd);
                #pragma HLS protocol fixed
                ap_wait();
                VEC_INFO vec_info = data_from_mem_access.read().data.range(63,0);
                printf("[cache] VEC_INFO_READ reply for pma_idx=%d vec_info.start=%u len=%u\n", pma_idx, (unsigned)vec_info.range(31,0), (unsigned)vec_info.range(63,32));
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
                    printf("[cache] Request MEMORY_READ pma_idx=%d start=%u len=%u\n", pma_idx, (unsigned)mem_read_cmd.start, (unsigned)mem_read_cmd.len);
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
                    {
                        // read total segments directly from header (last entry in new_range)
                        ap_uint<512> header_word = cache_buffer[evict_line][0];
                        int total_seg = (int)header_word.range((VERTEX_PER_PMA-1)*32+31, (VERTEX_PER_PMA-1)*32);
                        printf("[cache] PRINT PMA AFTER READ EXIST PMA CACHE LANE %d\n", evict_line);
                        print_cache_line(&cache_buffer[evict_line][0], HEADER_WORDS, SEG_WORDS_PER_SEG, total_seg, pma_idx);
                    }
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
                    //write empty header to cache_buffer
                    ap_uint<512> header_buffer[4];
                    for (int j = 0 ; j < 4 ; j++)
                    {
                        #pragma HLS PIPELINE II=1
                        header_buffer[j] = ((ap_uint<512>*)(&empty_header))[j];
                        cache_buffer[evict_line][j] = header_buffer[j];
                    }
                    for (int j = 4 ; j < 64 + 4 ; j++)
                    {
                        #pragma HLS PIPELINE II=1
                        cache_buffer[evict_line][j] = empty_pma_content;
                    }
                    {
                        // read total segments directly from header (last entry in new_range)
                        ap_uint<512> header_word = cache_buffer[evict_line][0];
                        int total_seg = (int)header_word.range((VERTEX_PER_PMA-1)*32+31, (VERTEX_PER_PMA-1)*32);
                        printf("[cache] PRINT PMA AFTER INITIALIZATION CACHE LANE %d\n", evict_line);
                        print_cache_line(&cache_buffer[evict_line][0], HEADER_WORDS, SEG_WORDS_PER_SEG, total_seg, pma_idx);
                    }
                }
            }
            int active_line = (cache_line == -1) ? evict_line : cache_line;
            add_timestamp(active_line, cache_time_stamp);
            //req to read_a_pma
            if (cmd.command_type == PMA_HEADER_READ)
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
                    cache_dirty[active_line] = 1;
                }
                cache_dirty[active_line] = 1;
                {
                    ap_uint<512> header_word = cache_buffer[active_line][0];
                    int total_seg = (int)header_word.range((VERTEX_PER_PMA-1)*32+31, (VERTEX_PER_PMA-1)*32);
                    printf("[cache] PRINT PMA AFTER HEADER WRITE CACHE LANE %d\n", evict_line);
                    print_cache_line(&cache_buffer[active_line][0], HEADER_WORDS, SEG_WORDS_PER_SEG, total_seg, pma_idx);
                }
            }
            else if (cmd.command_type == PMA_SEGMENT_WRITE)
            {
                int seg_word_start = (int)(sizeof(PMA_HEADER) / 64) + seg_start * (SEGMENT_SIZE * (int)sizeof(int) / 64);
                int seg_words = seg_len * (SEGMENT_SIZE * (int)sizeof(int) / 64);
                for (int j = 0 ; j < seg_words ; j++)
                {
                    #pragma HLS PIPELINE II=1
                    cache_buffer[active_line][seg_word_start + j] = data_from_app[i].read();
                    cache_dirty[active_line] = 1;
                }
                cache_dirty[active_line] = 1;
                {
                    ap_uint<512> header_word = cache_buffer[active_line][0];
                    int total_seg = (int)header_word.range((VERTEX_PER_PMA-1)*32+31, (VERTEX_PER_PMA-1)*32);
                    printf("[cache] PRINT PMA AFTER SEGMENT WRITE CACHE LANE %d\n", evict_line);
                    print_cache_line(&cache_buffer[active_line][0], HEADER_WORDS, SEG_WORDS_PER_SEG, total_seg, pma_idx);
                }
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
                printf("[cache] Extend request for pma_idx=%d old_total_seg=%d new_total_seg=%d\n", pma_idx, old_total_seg, new_total_seg);
                for (int v = 0; v < VERTEX_PER_PMA; ++v)
                {
                    int seg_start = (v == 0) ? 0 : new_range[v-1];
                    int seg_end = new_range[v];
                    for (int s = seg_start; s < seg_end; s++)
                    {
                        printf("[cache] PMA seg v=%d seg=%d neighbors:", v, s);
                        int first_word_idx = header_words + s * seg_words_per_seg;
                        for (int w = 0; w < seg_words_per_seg; ++w)
                        {
                            ap_uint<512> dst_big_word = cache_buffer[active_line][first_word_idx + w];
                            for (int b = 0; b < 16; ++b)
                            {
                                unsigned neighbor = (unsigned)dst_big_word.range(b*32+31, b*32);
                                printf(" %u", neighbor);
                            }
                        }
                        printf("\n");
                    }
                }
                if (new_total_seg >= old_total_seg && new_total_words <= MAX_CACHE_SIZE)
                {
                    printf("[cache] Extending pma_idx=%d old_total_seg=%d new_total_seg=%d\n", pma_idx, old_total_seg, new_total_seg);
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
                                #pragma HLS PIPELINE II=1
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
                    printf("[cache] Extend done in cache for pma_idx=%d new_total_seg=%d\n", pma_idx, new_total_seg);
                    printf("[cache] PRINT PMA AFTER EXTEND CACHE LANE %d\n", evict_line);
                    print_cache_line(&cache_buffer[active_line][0], HEADER_WORDS, seg_words_per_seg, new_total_seg, pma_idx);
                }
            }
        }

    }
    // flush all dirty cache lines before exiting
    for (int line = 0; line < CACHE_LINE_SIZE; line++) {
        if (cache_valid[line] && cache_dirty[line]) {
            int evict_pma_idx = cache_pma_idx[line];
            int to_write_start = -1;
            int to_write_len = -1;
            if (cache_new_len[line] > cache_old_len[line]) {
                auto new_len = cache_new_len[line];
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

                auto old_start = cache_old_start[line];
                auto old_len = cache_old_len[line];
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
                to_write_start = cache_old_start[line];
                to_write_len = cache_new_len[line];
            }

            MEMORY_INFO_ACCESS_COMMAND write_cmd = {};
            write_cmd.command_type = MEMORY_WRITE;
            write_cmd.pma_idx = evict_pma_idx;
            write_cmd.len = to_write_len;
            write_cmd.start = to_write_start;
            write_cmd.last = 0;
            req_to_mem_access.write(write_cmd);

            for (int j = 0 ; j < to_write_len / 64 ; j++) {
                #pragma HLS PIPELINE II = 1
                ap_axiu<512, 0, 0, 0> packet;
                packet.data = cache_buffer[line][j];
                packet.last = (j == to_write_len / 64 - 1) ? 1 : 0;
                data_to_mem_access.write(packet);
            }
            cache_dirty[line] = 0;
        }
    }

    // now inform memory model to finish
    MEMORY_INFO_ACCESS_COMMAND finish_cmd = {};
    finish_cmd.last = 1;
    finish_cmd.command_type = MEMORY_FINISH;
    req_to_mem_access.write(finish_cmd);
    req_to_alloc_access.write(finish_cmd);
}