#include "data_type.h"
#include <cstdio>
#include <cstring>
#include <cxxabi.h>
#include <hls_stream.h>
#include "debug.h"
#include "pma.h"
#include <ap_utils.h>
#include "hls_burst_maxi.h"
#define MAX_READ_REQUEST 5
#define MAX_WRITE_REQUEST 3
// #define MAX_ALLOC 1 
// #define MAX_FREE 1
#define MAX_PMA_IDX_INFO_REQUEST 6
#define MAX_PMA_ADDR_REQUEST 3
// #define MAX_PMA_HEADER_REQUEST 3
struct pma_idx_info
{
    int pma_idx;
    int direction;
    int last;
};
struct InsertPos {
    int idx;      // 插入索引
    int val;      // 查找结果值
};

void process_cmd(
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
    hls::stream<int> &cmd_count_stream,
    hls::stream<ap_uint<128>> &cmd_batch_stream,
    hls::stream<pma_idx_info> pma_info_request[MAX_PMA_IDX_INFO_REQUEST]
)
{
    printf("PROCESS_CMD\n");
    while(true)
    {
        if (cmd_stream.empty()) {
            ap_wait();
            continue;
        }
        ap_uint<128> cmd_batch[MAX_CMD_NUM];
        #pragma HLS ARRAY_PARTITION variable=cmd_batch complete
        int cmd_count = 0;
        // 读取直到 last=1
        for (int i = 0; i < MAX_CMD_NUM; i++) {
            #pragma HLS PIPELINE II=1
            auto cmd = cmd_stream.read();
            if (cmd.data == PMA_STOP && cmd.last == 1) 
            {
                pma_idx_info info;
                info.last = 1;
                for (int i = 0 ; i < MAX_PMA_IDX_INFO_REQUEST ; i++)
                {
                    #pragma HLS UNROLL
                    pma_info_request[i].write(info);
                }
                return;
            }
            cmd_batch[i] = cmd.data;
            cmd_count++;
            if (cmd.last == 1) {
                break;
            }
        }
        pma_idx_info info;
        info.direction = cmd_batch[0].range(98,98);
        info.pma_idx = cmd_batch[0].range(31,0) / VERTEX_PER_PMA;
        
        for (int i = 0 ; i < MAX_PMA_IDX_INFO_REQUEST ; i++)
        {
            #pragma HLS UNROLL
            pma_info_request[i].write(info);
        }
        // 将批次数据和数量写入输出流
        cmd_count_stream.write(cmd_count);
        for (int i = 0; i < cmd_count; i++) {
            #pragma HLS PIPELINE II=1
            cmd_batch_stream.write(cmd_batch[i]);
        }
    }
}

void sort_cu(
    hls::stream<int>& cmd_count_stream,
    hls::stream<ap_uint<128>>& cmd_batch_stream,
    hls::stream<ap_uint<64>>& edge_info_stream,
    hls::stream<int> &edge_count,
    hls::stream<pma_idx_info> &for_exit_signal 
)
{
    printf("SORT\n");
    while (true)
    {
        auto exit_signal = for_exit_signal.read();
        if (exit_signal.last == 1)
        {
            break;
        }
        int cmd_count = cmd_count_stream.read();
        ap_uint<64> edge_info[MAX_CMD_NUM];
        #pragma HLS ARRAY_PARTITION variable=edge_info complete

        // 收集边信息
        for (int i = 0; i < cmd_count; i++) {
            #pragma HLS PIPELINE II=1
            auto cmd = cmd_batch_stream.read();
            if (cmd == PMA_STOP)
            {
                return;
            }
            edge_info[i].range(63,32) = cmd.range(31,0); // src
            edge_info[i].range(31,0)  = cmd.range(63,32); // dst
        }
        SORT_EDGES:
        // 使用排序网络对 edge_info 排序
        for (int stage = 0; stage < 4; stage++) { // log2(16) = 4
            for (int step = 0; step < 16; step++) {
                #pragma HLS UNROLL
                int idx1 = step;
                int idx2 = step ^ (1 << stage);
                if (idx2 > idx1 && idx2 < cmd_count) {
                    if (edge_info[idx1] > edge_info[idx2]) {
                        ap_uint<64> temp = edge_info[idx1];
                        edge_info[idx1] = edge_info[idx2];
                        edge_info[idx2] = temp;
                    }
                }
            }
        }
        edge_count.write(cmd_count);
        for (int i = 0; i < cmd_count ; i++)
        {
            #pragma HLS PIPELINE II=1
            edge_info_stream.write(edge_info[i]);
        }
    }
}

void head_read(
    BYTE* hbm_ptr,
    VEC_INFO* pos_pma_addrs,
    VEC_INFO* neg_pma_addrs,
    hls::stream<pma_idx_info> &pma_idx_info_request,
    hls::stream<PMA_HEADER>& header_out,
    hls::stream<VEC_INFO> pma_addr[MAX_PMA_ADDR_REQUEST]
)
{
    printf("HEAD_READ\n");
    while (true)
    {
        auto info = pma_idx_info_request.read();
        if (info.last == 1)
        {
            break;
        }
        int pma_idx = info.pma_idx;
        int direction = info.direction;
        VEC_INFO vec;
        if (direction == IN_DIRECTION)
        {
            vec = neg_pma_addrs[pma_idx];
        }
        else
        {
            vec = pos_pma_addrs[pma_idx];
        }
        for (int i = 0 ; i < MAX_PMA_ADDR_REQUEST ; i++)
        {
            #pragma HLS UNROLL
            pma_addr[i].write(vec);
        }
        
        PMA_HEADER header;
        // memcpy(&header, hbm_ptr + int(vec.range(31,0)), sizeof(PMA_HEADER));
        {
            ap_uint<512>* hbm_wide = (ap_uint<512>*)(hbm_ptr + int(vec.range(31,0)));
            ap_uint<512>* header_wide = (ap_uint<512>*)&header;
            for (int i = 0; i < 4; i++) { // 256 / 64 = 4
                #pragma HLS UNROLL
                header_wide[i] = hbm_wide[i];
            }
        }
        header_out.write(header);
    }
}
void binary_search_cu(
    BYTE* hbm_ptr,
    hls::stream<PMA_HEADER> &header_info_in_stream,
    hls::stream<PMA_HEADER> &header_after_modify,
    hls::stream<VEC_INFO> &pma_addr,
    hls::stream<ap_uint<64>> &edge_info_stream,
    hls::stream<int> &edge_count,
    hls::stream<InsertPos> &insert_info,
    hls::stream<int> &insert_count,
    hls::stream<pma_idx_info> &for_exit_signal
)
{
    printf("BINARY_SEARCH\n");
    while (true)
    {
        auto exit_signal = for_exit_signal.read();
        if (exit_signal.last == 1)
        {
            break;
        }
        auto header = header_info_in_stream.read();
        auto addr = pma_addr.read();
        int count = edge_count.read();
        int pma_data_base_addr = addr.range(31,0) + sizeof(PMA_HEADER);
        int find_count = 0;
        for (int i = 0 ; i < count ; i++)
        {
            auto edge_info = edge_info_stream.read();
            int src = edge_info.range(63,32);
            int dst = edge_info.range(31,0);
            int local_src = src % VERTEX_PER_PMA;

            int low_seg = local_src == 0 ? 0 : header.vertex_range[local_src - 1];
            int high_seg = header.vertex_range[local_src];
            
            int out_idx = low_seg * SEGMENT_SIZE - 1;
            int out_val = -1;
            bool find = false;
            BINARY_SEARCH:
            while (low_seg <= high_seg)
            {
                int mid = low_seg + (high_seg - low_seg) / 2;
                int seg_start_addr = mid * SEGMENT_SIZE;

                int minimum = INT_MAX;
                int maximum = INT_MIN;
                int max_idx = 0;
                int storage[SEGMENT_SIZE];
                
                // memcpy(storage, hbm_ptr + pma_data_base_addr + mid * SEGMENT_SIZE * sizeof(int), sizeof(storage));
                {
                    
                    ap_uint<512>* hbm_wide = (ap_uint<512>*)(hbm_ptr + pma_data_base_addr + mid * SEGMENT_SIZE * sizeof(int));
                    ap_uint<512>* storage_wide = (ap_uint<512>*)storage;
                    const int seg_chunks = sizeof(storage) / 64; // SEGMENT_SIZE*4 / 64
                    for (int i = 0; i < seg_chunks; i++) {
                        #pragma HLS UNROLL
                        storage_wide[i] = hbm_wide[i];
                    }
                }
                bool match_perfect = false;
                INTRA_SEG_ITERATION:
                for (int j = 0 ; j < SEGMENT_SIZE ; j++)
                {
                    #pragma HLS PIPELINE II=1
                    #pragma HLS LOOP_TRIPCOUNT min=0 max=64
                    int storage_val = storage[j];
                    if (storage_val != UNUSED_ICON)
                    {
                        maximum = std::max(maximum, storage_val);
                        minimum = std::min(minimum, storage_val);
                        if (storage_val == dst)
                        { 
                            out_idx = mid * SEGMENT_SIZE + j;
                            out_val = dst;
                            match_perfect = true;
                            break;
                        }
                        if (storage_val < dst)
                        {
                            out_idx = mid * SEGMENT_SIZE + j;
                            out_val = storage_val;
                        }
                        else if (storage_val > dst)
                        { 
                            break; 
                        }
                    }
                }
                if (match_perfect)
                {
                    break;
                }
                if (maximum < dst)
                {
                    low_seg = mid + 1;
                }
                 else if (minimum > dst)
                {
                    high_seg = mid - 1;
                }
                else
                {
                    find = true;
                }
                
                if (find) break;
            }
            InsertPos to_insert;
            if (out_val != dst)
            {
                find_count++;
                to_insert.idx = out_idx;
                to_insert.val = dst;
                header.edge_count[local_src]++;
                insert_info.write(to_insert);
            }
        }
        insert_count.write(find_count);
        header_after_modify.write(header);
    }
}


void rebalance_cu(
    BYTE* hbm_ptr,
    hls::stream<PMA_HEADER>& pma_header_in,
    hls::stream<VEC_INFO>& pma_addr_in,
    hls::stream<pma_idx_info>& pma_idx,
    hls::stream<COMMAND_STREAM>& ack_back_to_router
)
{
            
    int unused_data[SEGMENT_SIZE];
    for (int i = 0; i < SEGMENT_SIZE; i++) {
        #pragma HLS UNROLL
        unused_data[i] = UNUSED_ICON;
    }
    HERE
    printf("REBALANCE\n");
    while (true)
    {
        auto signal = pma_idx.read();
        if (signal.last == 1)
        {
            break;
        }
        PMA_HEADER pma_header = pma_header_in.read();
        VEC_INFO pma_addr = pma_addr_in.read();
        int seg_count = pma_header.vertex_range[15];

        const int pma_data_base_addr = pma_addr.range(31,0) + sizeof(PMA_HEADER);
        const int pma_data_size_bytes = pma_addr.range(63,32) - sizeof(PMA_HEADER);
        
        int pack_buffer[SEGMENT_SIZE];
        int collect_buffer[SEGMENT_SIZE];
        for (int k = 0 ; k < SEGMENT_SIZE ; k++)
        {
            #pragma HLS UNROLL 
            pack_buffer[k] = UNUSED_ICON;
        }
        int pack_ptr_ints = seg_count * SEGMENT_SIZE - 1;
        int buffer_count = SEGMENT_SIZE - 1;
        HERE
        PACK_MAIN_LOOP:
        for (int i = seg_count - 1; i >= 0; --i) {
            #pragma HLS LOOP_TRIPCOUNT min=1 max=4096
            // memcpy(collect_buffer, hbm_ptr + pma_data_base_addr + i * SEGMENT_SIZE * sizeof(int), sizeof(collect_buffer));
            ap_uint<512>* wide_hbm_ptr = (ap_uint<512>*)(hbm_ptr + pma_data_base_addr + i * SEGMENT_SIZE * sizeof(int));
            ap_uint<512>* wide_collect_buffer = (ap_uint<512>*) collect_buffer;
            WRITE_COLLECT_BUFFER:
            for (int j = 0 ; j < sizeof(collect_buffer) / 64 ; j++)
            {
                #pragma HLS UNROLL
                wide_collect_buffer[j] = wide_hbm_ptr[j];
            }
            PACK_INNER_LOOP:
            for (int j = SEGMENT_SIZE - 1; j >= 0; --j) {
                #pragma HLS PIPELINE II=1
                if (collect_buffer[j] != UNUSED_ICON) {
                    pack_buffer[buffer_count] = collect_buffer[j];
                    buffer_count--;
                    pack_ptr_ints--;
                    if (buffer_count == -1) {
                        // memcpy(hbm_ptr + pma_data_base_addr + (pack_ptr_ints + 1) * sizeof(int), pack_buffer, sizeof(pack_buffer));
                        ap_uint<512>* wide_write_back_hbm = (ap_uint<512>*)(hbm_ptr + pma_data_base_addr + (pack_ptr_ints + 1) * sizeof(int));
                        ap_uint<512>* wide_pack_buffer = (ap_uint<512>*) pack_buffer;
                        for (int k = 0 ; k < sizeof(pack_buffer) / 64 ; k++)
                        {
                            wide_write_back_hbm[k] = wide_pack_buffer[k];
                        }
                        buffer_count = SEGMENT_SIZE - 1;
                    }
                }
            }
            // for (int j = 0 ; j < SEGMENT_SIZE ; j++)
            // {
            //     #pragma HLS PIPELINE II=1
            //     collect_buffer[j] = UNUSED_ICON;
            // }
            // HERE
            ap_uint<512>* wide_hbm_write_back_read = (ap_uint<512>*)(hbm_ptr + pma_data_base_addr + i * SEGMENT_SIZE * sizeof(int));
            ap_uint<512>* wide_unused_data = (ap_uint<512>*) unused_data;
            for (int j = 0 ; j < sizeof(unused_data) / 64 ; j++)
            {
                #pragma HLS UNROLL
                wide_hbm_write_back_read[j] = wide_unused_data[j];
            }
            // HERE
            // memcpy(hbm_ptr + pma_data_base_addr + i * SEGMENT_SIZE * sizeof(int), unused_data,  sizeof(collect_buffer));
            
        }
        HERE
        if (buffer_count < SEGMENT_SIZE - 1) {
            int element_num = SEGMENT_SIZE - buffer_count - 1;
            //这里不能使用burst 覆盖否则后面数据被覆盖掉了
            // memcpy(hbm_ptr + pma_data_base_addr + (pack_ptr_ints + 1) * sizeof(int), pack_buffer + buffer_count + 1, element_num * sizeof(int));
            int* dst_ptr = (int*)(hbm_ptr + pma_data_base_addr + (pack_ptr_ints + 1) * sizeof(int));
            int* src_ptr = pack_buffer + buffer_count + 1;
            for (int i = 0; i < element_num; i++) {
                #pragma HLS PIPELINE II=1
                dst_ptr[i] = src_ptr[i];
            }
        }
        HERE
        int current_read_ptr_ints = pack_ptr_ints + 1;



        
        for (int v = 0 ; v < VERTEX_PER_PMA ; v++)
        {
            #pragma HLS LOOP_TRIPCOUNT min=64 max=64
            int seg_start_idx = (v == 0) ? 0 : pma_header.vertex_range[v - 1];
            int seg_end_idx = pma_header.vertex_range[v];
            int seg_count = seg_end_idx - seg_start_idx;
            if (seg_count == 0) continue;
            int edge_num = pma_header.edge_count[v];
            if (edge_num == 0) continue;
            int base = edge_num / seg_count;
            int extra = edge_num % seg_count;

            REBALANCE_SEG_LOOP:
            for (int s = 0; s < seg_count; ++s) {
                #pragma HLS LOOP_TRIPCOUNT min=0 max=64
                int num_to_move = base + (s < extra ? 1 : 0);
                int target_seg_start_addr = pma_data_base_addr + (seg_start_idx + s) * SEGMENT_SIZE * sizeof(int);

                if (num_to_move == 0)
                {
                    // memcpy(hbm_ptr + target_seg_start_addr, unused_data, sizeof(unused_data));
                    {
                        ap_uint<512>* hbm_wide = (ap_uint<512>*)(hbm_ptr + target_seg_start_addr);
                        ap_uint<512>* unused_wide = (ap_uint<512>*)unused_data;
                        const int seg_chunks = sizeof(unused_data) / 64; // SEGMENT_SIZE*4 / 64
                        for (int i = 0; i < seg_chunks; i++) {
                            #pragma HLS UNROLL
                            hbm_wide[i] = unused_wide[i];
                        }
                    }
                    continue;
                }

                int read_addr = pma_data_base_addr + current_read_ptr_ints * sizeof(int);
                int segment_buffer[SEGMENT_SIZE];
                // memcpy(segment_buffer, hbm_ptr + read_addr, num_to_move * sizeof(int));
                {
                    ap_uint<512>* hbm_wide = (ap_uint<512>*)(hbm_ptr + read_addr);
                    ap_uint<512>* seg_wide = (ap_uint<512>*)segment_buffer;
                    const int seg_chunks = SEGMENT_SIZE * sizeof(int) / 64; // SEGMENT_SIZE*4 / 64
                    for (int i = 0; i < seg_chunks; i++) {
                        #pragma HLS UNROLL
                        seg_wide[i] = hbm_wide[i];
                    }
                }
                for (int i = num_to_move; i < SEGMENT_SIZE; i++) {
                    // #pragma HLS UNROLL
                    #pragma HLS PIPELINE II=1
                    segment_buffer[i] = UNUSED_ICON;
                }

                // memcpy(hbm_ptr + target_seg_start_addr, segment_buffer, sizeof(segment_buffer));
                {
                    ap_uint<512>* hbm_wide = (ap_uint<512>*)(hbm_ptr + target_seg_start_addr);
                    ap_uint<512>* seg_wide = (ap_uint<512>*)segment_buffer;
                    const int seg_chunks = sizeof(segment_buffer) / 64; // SEGMENT_SIZE*4 / 64
                    for (int i = 0; i < seg_chunks; i++) {
                        #pragma HLS UNROLL
                        hbm_wide[i] = seg_wide[i];
                    }
                }
                //防止覆盖
                // memcpy(hbm_ptr + read_addr, unused_data, num_to_move * sizeof(int));
                {
                    int* dst_ptr = (int*)(hbm_ptr + read_addr);
                    int* src_ptr = unused_data;
                    for (int i = 0; i < num_to_move; i++) {
                        #pragma HLS PIPELINE II=1
                        dst_ptr[i] = src_ptr[i];
                    }  
                }
                current_read_ptr_ints += num_to_move;
            }
        }
        ack_back_to_router.write({.data=signal.pma_idx});
    }
}   


void insert_element_cu(
    BYTE* hbm_ptr,
    hls::stream<InsertPos> &insert_info,
    hls::stream<int> &insert_count,
    hls::stream<VEC_INFO> &pma_addr,
    hls::stream<PMA_HEADER>& pma_header,
    hls::stream<PMA_HEADER>& pma_header_out,
    hls::stream<pma_idx_info>& for_exit_signal
)
{
    printf("INSERT_ELEMENT\n");
    while (true)
    {
        auto exit_signal = for_exit_signal.read();
        if (exit_signal.last == 1)
        {
            break;
        }
        int count = insert_count.read();
        VEC_INFO pma_base_addr = pma_addr.read();
        PMA_HEADER header = pma_header.read();
        InsertPos pos_batch[MAX_CMD_NUM];
        for (int k = 0 ; k < count ; k++)
        {
            #pragma HLS LOOP_TRIPCOUNT min=0 max=MAX_CMD_NUM
            InsertPos pos = insert_info.read();
            pos_batch[k] = pos;
        }
        int need_extra_seg = 0;
        for (int v = 0 ; v < VERTEX_PER_PMA ; v++)
        {
            #pragma HLS PIPELINE II=1
            int seg_start = v == 0 ? 0 : header.vertex_range[v-1];
            int seg_end = header.vertex_range[v];
            int max_edge = (seg_end - seg_start) * SEGMENT_SIZE * DENSITY_BOUND;
            if (max_edge < header.edge_count[v])
            {
                need_extra_seg += seg_end - seg_start;
            }
        }
        
        int current_seg_idx = -1;
        const int BITS_PER_SEG = SEGMENT_SIZE * 32;
        int one_seg_updates = 0;
        // const int chunks_per_segment = BITS_PER_SEG / 512; // 结果为 4
        ap_uint<BITS_PER_SEG> tmp_seg;
        PROCESS_INSERT_BATCH:
        for (int k = 0; k < count; k++)
        {
            #pragma HLS LOOP_TRIPCOUNT min=0 max=MAX_CMD_NUM
            InsertPos pos = pos_batch[k];
            int insert_abs_idx = pos.idx + 1;
            int target_seg_idx = insert_abs_idx / SEGMENT_SIZE;
            if (target_seg_idx != current_seg_idx) 
            {
                one_seg_updates = 0;
                if (current_seg_idx != -1) 
                {
                    // memcpy(hbm_ptr + (int)pma_base_addr.range(31,0) + sizeof(PMA_HEADER) + current_seg_idx * SEGMENT_SIZE * sizeof(int), &tmp_seg, sizeof(tmp_seg));
                    ap_uint<512>* hbm_wide = (ap_uint<512>*)(hbm_ptr + (int)pma_base_addr.range(31,0) + sizeof(PMA_HEADER) + current_seg_idx * SEGMENT_SIZE * sizeof(int));
                    ap_uint<512>* seg_wide = (ap_uint<512>*)&tmp_seg;
                    const int seg_chunks = sizeof(tmp_seg) / 64; // SEGMENT_SIZE*4 / 64
                    for (int i = 0; i < seg_chunks; i++) {
                        #pragma HLS UNROLL
                        hbm_wide[i] = seg_wide[i];
                    }
                }
                current_seg_idx = target_seg_idx;
                {
                    ap_uint<512>* hbm_wide = (ap_uint<512>*)(hbm_ptr + (int)pma_base_addr.range(31,0) + sizeof(PMA_HEADER) + current_seg_idx * SEGMENT_SIZE * sizeof(int));
                    ap_uint<512>* seg_wide = (ap_uint<512>*)&tmp_seg;
                    const int seg_chunks = sizeof(tmp_seg) / 64; // SEGMENT_SIZE*4 / 64
                    for (int i = 0; i < seg_chunks; i++) {
                        #pragma HLS UNROLL
                        seg_wide[i] = hbm_wide[i];
                    }
                }
                // memcpy(&tmp_seg, hbm_ptr + (int)pma_base_addr.range(31,0) + sizeof(PMA_HEADER) + current_seg_idx * SEGMENT_SIZE * sizeof(int), sizeof(tmp_seg));
            }
            
            int insert_offset_in_seg = (insert_abs_idx % SEGMENT_SIZE) + one_seg_updates;
            ap_uint<BITS_PER_SEG> shifted_data = tmp_seg;
            ap_uint<BITS_PER_SEG> mask = (ap_uint<BITS_PER_SEG>(1) << (insert_offset_in_seg * 32)) - 1;
            tmp_seg &= mask;
            shifted_data &= ~mask;
            shifted_data <<= 32;
            tmp_seg |= shifted_data;
            tmp_seg.range((insert_offset_in_seg) * 32 + 31, (insert_offset_in_seg) * 32) = pos.val;
            one_seg_updates++;
        }
        // memcpy(hbm_ptr + (int)pma_base_addr.range(31,0) + sizeof(PMA_HEADER) + current_seg_idx * sizeof(int) * SEGMENT_SIZE, &tmp_seg, sizeof(tmp_seg));
        {
            ap_uint<512>* hbm_wide = (ap_uint<512>*)(hbm_ptr + (int)pma_base_addr.range(31,0) + sizeof(PMA_HEADER) + current_seg_idx * sizeof(int) * SEGMENT_SIZE);
            ap_uint<512>* seg_wide = (ap_uint<512>*)&tmp_seg;
            const int seg_chunks = sizeof(tmp_seg) / 64; // SEGMENT_SIZE*4 / 64
            for (int i = 0; i < seg_chunks; i++) {
                #pragma HLS UNROLL
                hbm_wide[i] = seg_wide[i];
            }
        }
        pma_header_out.write(header);
    }
}

void move_and_change_header(
    BYTE* hbm_ptr,
    VEC_INFO* pos_addr,
    VEC_INFO* neg_addr,
    hls::stream<PMA_HEADER>& header_stream,
    hls::stream<PMA_HEADER>& new_header_stream,

    hls::stream<VEC_INFO>& pma_addr_in,
    hls::stream<VEC_INFO>& pma_addr_out,

    hls::stream<VEC_INFO_STREAM>& new_alloc_addr,
    hls::stream<VEC_INFO_STREAM>& vec_info_to_mem,
    hls::stream<COMMAND_STREAM>& cmd_to_mem,

    hls::stream<pma_idx_info>& pma_idx_info_in
)
{
    printf("MOVE_AND_CHANGE_HEADER\n");
    while (true)
    {
        auto pma_info = pma_idx_info_in.read();
        if (pma_info.last == 1)
        {
            HERE
            break;
        }
        auto header = header_stream.read();
        int direction = pma_info.direction;
        int pma_index = pma_info.pma_idx;
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
            int max_edge = (seg_end - seg_start) * SEGMENT_SIZE * DENSITY_BOUND;
            if (max_edge < header.edge_count[v])
            {
                enlarged = true;
                enlarge_segs[v] = seg_end - seg_start;
            }
        }
        int total_enlarge_segs = 0;
        for (int i = 0 ; i < VERTEX_PER_PMA ; i++)
        {
            #pragma HLS PIPELINE II=1
            total_enlarge_segs += enlarge_segs[i];
            header.vertex_range[i] += total_enlarge_segs;
        }
        VEC_INFO old_addr = pma_addr_in.read();
        VEC_INFO new_addr;
        if (enlarged)
        {
            HERE
            cmd_to_mem.write({.data=MEM_ALLOC});
            VEC_INFO to_enlarge;
            to_enlarge.range(31,0) = 0; //alloc 只看长度
            to_enlarge.range(63,32) = int(old_addr.range(63,32)) + total_enlarge_segs * SEGMENT_SIZE * sizeof(int);
            vec_info_to_mem.write({.data = to_enlarge});
            // ap_wait();
            new_addr = new_alloc_addr.read().data;
            if (direction == IN_DIRECTION)
            {
                neg_addr[pma_index] = new_addr;
            }
            else  
            {
                pos_addr[pma_index] = new_addr;
            }
            // memmove(hbm_ptr + (int)new_addr.range(31,0),  hbm_ptr + (int)old_addr.range(31,0), (int)old_addr.range(63,32));
            int src_addr = (int)old_addr.range(31,0);
            int dst_addr = (int)new_addr.range(31,0);
            int move_bytes = (int)old_addr.range(63,32);
            int move_chunks = move_bytes / 64; // 每块64字节
            
            ap_uint<512>* src_ptr = (ap_uint<512>*)(hbm_ptr + src_addr);
            ap_uint<512>* dst_ptr = (ap_uint<512>*)(hbm_ptr + dst_addr);

            for (int i = 0; i < move_chunks; i++) {
                #pragma HLS PIPELINE II=1
                dst_ptr[i] = src_ptr[i];
            }
            
            cmd_to_mem.write({.data = MEM_FREE});
            vec_info_to_mem.write({.data=old_addr});
            int new_data_start = (int)new_addr.range(31,0) + (int)old_addr.range(63,32);
            // int unused_segment[SEGMENT_SIZE];
            ap_uint<512> wide_unused;
            for (int i = 0; i < 16; i++) {
                #pragma HLS UNROLL
                wide_unused.range((i+1)*32-1, i*32) = UNUSED_ICON;
            }

            // for (int i = 0; i < SEGMENT_SIZE; i++) {
            //     #pragma HLS UNROLL
            //     unused_segment[i] = UNUSED_ICON;
            // }
            HERE
            for (int s = 0; s < total_enlarge_segs; s++) {
                #pragma HLS PIPELINE
                int segment_start_addr = new_data_start + s * SEGMENT_SIZE * sizeof(int);
                ap_uint<512>* hbm_wide = (ap_uint<512>*)(hbm_ptr + segment_start_addr);
                for (int k = 0; k < SEGMENT_SIZE / 16; k++) {
                    #pragma HLS UNROLL
                    hbm_wide[k] = wide_unused;
                }
            }

        }
        else  
        {
            new_addr = old_addr;
        }
        // memcpy(hbm_ptr + (int)new_addr.range(31,0), &header, sizeof(PMA_HEADER));
        HERE
        ap_uint<512>* hbm_wide = (ap_uint<512>*)(hbm_ptr + (int)new_addr.range(31,0));
        ap_uint<512>* header_wide = (ap_uint<512>*)&header;
        for (int i = 0; i < 4; i++) { // 256 / 64 = 4
            #pragma HLS UNROLL
            hbm_wide[i] = header_wide[i];
        }
        pma_addr_out.write(new_addr);
        new_header_stream.write(header);
        HERE
    }
}

extern "C"
{
    void pma_insert_edge(
        VEC_INFO* pos_pma_addrs,
        VEC_INFO* neg_pma_addrs,
        BYTE* hbm_ptr_0,
        BYTE* hbm_ptr_1,
        BYTE* hbm_ptr_2,
        BYTE* hbm_ptr_3,
        BYTE* hbm_ptr_4,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
        hls::stream<VEC_INFO_STREAM> &vec_info_to_mem_stream,
        hls::stream<COMMAND_STREAM> &cmd_to_mem_stream,
        hls::stream<VEC_INFO_STREAM> &vec_info_back_stream,
        hls::stream<COMMAND_STREAM> &ack_back_to_router
    )
    {
        #pragma HLS INTERFACE m_axi port=pos_pma_addrs offset=slave bundle=gmem0
        #pragma HLS INTERFACE m_axi port=neg_pma_addrs offset=slave bundle=gmem1
        #pragma HLS INTERFACE m_axi port=hbm_ptr_0 offset=slave bundle=gmem2
        #pragma HLS INTERFACE m_axi port=hbm_ptr_1 offset=slave bundle=gmem3
        #pragma HLS INTERFACE m_axi port=hbm_ptr_2 offset=slave bundle=gmem4
        #pragma HLS INTERFACE m_axi port=hbm_ptr_3 offset=slave bundle=gmem5
        #pragma HLS INTERFACE m_axi port=hbm_ptr_4 offset=slave bundle=gmem6
        #pragma HLS INTERFACE axis port=cmd_stream
        #pragma HLS INTERFACE axis port=vec_info_to_mem_stream
        #pragma HLS INTERFACE axis port=cmd_to_mem_stream
        #pragma HLS INTERFACE axis port=vec_info_back_stream
        #pragma HLS INTERFACE axis port=ack_back_to_router
       
        hls::stream<pma_idx_info> pma_idx_info_request[MAX_PMA_IDX_INFO_REQUEST];
        hls::stream<VEC_INFO> pma_addr_request[MAX_PMA_ADDR_REQUEST];
        hls::stream<PMA_HEADER> pma_header_request("pma_header_request");

        hls::stream<int> cmd_count_stream("cmd_count_stream");
        hls::stream<ap_uint<128>> cmd_batch_stream("cmd_batch_stream");

        hls::stream<ap_uint<64>> edge_info_stream("edge_info_stream");
        hls::stream<int> edge_count_stream("edge_count_stream");

        hls::stream<PMA_HEADER> header_from_bs("header_from_bs");
        hls::stream<PMA_HEADER> header_from_insert("header_from_insert");
        hls::stream<InsertPos> insert_info_stream("insert_info_stream");
        hls::stream<int> insert_count_stream("insert_count_stream");

        hls::stream<PMA_HEADER> new_pma_header("new_pma_header");
        hls::stream<VEC_INFO> pma_addr_to_rebalance("pma_addr_to_rebalance");
        // #pragma HLS STREAM variable=pma_idx_info_request depth=16
        // #pragma HLS STREAM variable=pma_addr_request depth=16
        // #pragma HLS STREAM variable=pma_header_request depth=16
        // #pragma HLS STREAM variable=cmd_count_stream depth=16
        #pragma HLS STREAM variable=cmd_batch_stream depth=16
        #pragma HLS STREAM variable=edge_info_stream depth=16
        // #pragma HLS STREAM variable=edge_count_stream depth=16
        // #pragma HLS STREAM variable=header_from_bs depth=16
        // #pragma HLS STREAM variable=header_from_insert depth=16
        #pragma HLS STREAM variable=insert_info_stream depth=16
        // #pragma HLS STREAM variable=insert_count_stream depth=16
        // #pragma HLS STREAM variable=new_pma_header depth=16
        // #pragma HLS STREAM variable=pma_addr_to_rebalance depth=16


        #pragma HLS DATAFLOW
        
        process_cmd(
            cmd_stream,
            cmd_count_stream,
            cmd_batch_stream,
            pma_idx_info_request
        );
        
        sort_cu(
            cmd_count_stream,
            cmd_batch_stream,
            edge_info_stream,
            edge_count_stream,
            pma_idx_info_request[5]
        );

        
        head_read(
            hbm_ptr_0,
            pos_pma_addrs, 
            neg_pma_addrs, 
            pma_idx_info_request[0],
            pma_header_request,
            pma_addr_request
        );

        binary_search_cu(
            hbm_ptr_1,
            pma_header_request,
            header_from_bs,
            pma_addr_request[0],
            edge_info_stream,
            edge_count_stream,
            insert_info_stream,
            insert_count_stream,
            pma_idx_info_request[3]
        );

        
        insert_element_cu(
            hbm_ptr_2,
            insert_info_stream,
            insert_count_stream,
            pma_addr_request[1],
            header_from_bs,
            header_from_insert,
            pma_idx_info_request[4]
        );

        
        move_and_change_header(
            hbm_ptr_3,
            pos_pma_addrs,
            neg_pma_addrs,
            header_from_insert,
            new_pma_header, 

            pma_addr_request[2],
            pma_addr_to_rebalance, 

            vec_info_back_stream,
            vec_info_to_mem_stream,
            cmd_to_mem_stream,
            pma_idx_info_request[1]
        );
        
        rebalance_cu(
            hbm_ptr_4,
            new_pma_header,
            pma_addr_to_rebalance,
            pma_idx_info_request[2],
            ack_back_to_router
        );
    }
}