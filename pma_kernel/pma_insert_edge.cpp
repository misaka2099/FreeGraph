#include "data_type.h"
#include <cstdio>
#include <cstring>
#include <cxxabi.h>
#include <hls_stream.h>
#include "debug.h" 
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
struct inserted_seg
{
    int old_vertex_relative_seg_number[16];
    int vertex_number[16];
};

void read_seg(
    BYTE* hbm_ptr,
    int segment_buffer[SEGMENT_SIZE],
    int pma_data_base_addr,
    int seg_number
)
{
    #pragma HLS INLINE
    // ap_uint<512>* segment_buffer_ptr = (ap_uint<512>*)(segment_buffer);
    ap_uint<512> buffer;
    // const int seg_addr_in_ap_512_pattern = (seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr) / 64;
    
    // const int seg_addr_in_ap_512_pattern = (seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr) >> 6;    
    // for (int i = 0 ; i < SEGMENT_SIZE * sizeof(int) / 64 ; i++)
    // {
    //     #pragma HLS PIPELINE II=1 
    //     // segment_buffer_ptr[i] 
    //     // = hbm_ptr[i + (seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr) / 64];
    //     buffer = hbm_ptr[i + seg_addr_in_ap_512_pattern];
    //     for (int j = 0 ; j < 16 ; j++)
    //     {
    //         #pragma HLS UNROLL
    //         segment_buffer[i*16+j] = buffer.range(32*(j+1) - 1, 32*j);
    //     }
    // }
    int seg_byte_addr = pma_data_base_addr + seg_number * SEGMENT_SIZE * sizeof(int);
    // 计算段的起始索引（单位：ap_uint<512>块）
    // int seg_idx = seg_byte_addr / 64;
    // 直接memcpy
    memcpy(segment_buffer, hbm_ptr + seg_byte_addr, SEGMENT_SIZE * sizeof(int));
}
void write_seg(
    BYTE* hbm_ptr,
    int segment_buffer[SEGMENT_SIZE],
    int pma_data_base_addr,
    int seg_number
)
{
    #pragma HLS INLINE
    // ap_uint<512>* segment_buffer_ptr = (ap_uint<512>*)(segment_buffer);
    // ap_uint<512> buffer;
    // const int seg_addr_in_ap_512_pattern = (seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr) >> 6;
        
    // for (int i = 0 ; i < SEGMENT_SIZE * sizeof(int) / 64 ; i++)
    // {
    //     #pragma HLS PIPELINE II=1 
    //     for (int j = 0 ; j < 16 ; j++)
    //     {
    //         #pragma HLS UNROLL
    //         buffer.range(32*(j+1) - 1, 32*j) = segment_buffer[i*16+j];
    //     }
    //     hbm_ptr[i + seg_addr_in_ap_512_pattern] = buffer;
        
    //     // hbm_ptr[i + (seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr) / 64] 
    //     // = segment_buffer_ptr[i] ;

    // }
    int seg_byte_addr = pma_data_base_addr + seg_number * SEGMENT_SIZE * sizeof(int);
    // 计算段的起始索引（单位：ap_uint<512>块）
    // int seg_idx = seg_byte_addr / 64;
    // 直接memcpy
    memcpy(hbm_ptr + seg_byte_addr, segment_buffer, SEGMENT_SIZE * sizeof(int));
}
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
            cmd_batch[i][127] = 1;
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
        int bias = int(vec.range(31,0));
        memcpy(&header, hbm_ptr + bias, sizeof(PMA_HEADER));
        // {
        //     ap_uint<512>* hbm_wide = (ap_uint<512>*)(hbm_ptr + int(vec.range(31,0)));
        // ap_uint<512>* header_wide = (ap_uint<512>*)&header;
        // for (int i = 0; i < 4; i++) { // 256 / 64 = 4
        //     #pragma HLS UNROLL
        //     header_wide[i] = hbm_ptr[i + int(vec.range(31,0)) / 64];
        // }
        // }
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
    hls::stream<pma_idx_info> &for_exit_signal,
    hls::stream<inserted_seg>& inserted_pos
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
        inserted_seg inserted_seg_info;
        for (int i = 0 ; i < 16 ; i++)
        {
            #pragma HLS UNROLL
            inserted_seg_info.old_vertex_relative_seg_number[i] = -1;
            inserted_seg_info.vertex_number[i] = -1;
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
                    
                    // ap_uint<512>* hbm_wide = (ap_uint<512>*)(hbm_ptr + pma_data_base_addr + mid * SEGMENT_SIZE * sizeof(int));
                    // ap_uint<512>* storage_wide = (ap_uint<512>*)storage;
                    // const int seg_chunks = sizeof(storage) / 64; // SEGMENT_SIZE*4 / 64
                    // for (int i = 0; i < seg_chunks; i++) {
                    //     #pragma HLS UNROLL
                    //     storage_wide[i] = hbm_ptr[i + (pma_data_base_addr + mid * SEGMENT_SIZE * sizeof(int)) / 64];
                    // }
                    read_seg(hbm_ptr, storage, pma_data_base_addr, mid);
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
                inserted_seg_info.old_vertex_relative_seg_number[find_count] = (out_idx + 1) / SEGMENT_SIZE;
                inserted_seg_info.vertex_number[find_count] = local_src;
                find_count++;
                to_insert.idx = out_idx;
                to_insert.val = dst;
                header.edge_count[local_src]++;
                insert_info.write(to_insert);
            }
        }
        insert_count.write(find_count);
        header_after_modify.write(header);
        inserted_pos.write(inserted_seg_info);
    }
}


int valid_count(int seg_start, int seg_end, BYTE* hbm_ptr, int pma_data_base_addr)
{
    #pragma HLS INLINE
    int count = 0;
    int segment_buffer[SEGMENT_SIZE];
    for (int  i = seg_start ; i < seg_end ; i++)
    {
        // for (int j = 0 ; j < SEGMENT_SIZE * sizeof(int) / 64 ; j++)
        // {
        //     #pragma HLS UNROLL 
        //     ap_uint<512>* segment_buffer_ptr = (ap_uint<512>*)segment_buffer;
        //     segment_buffer_ptr[j] = hbm_ptr[(pma_data_base_addr + i * SEGMENT_SIZE * sizeof(int)) / 64];
        // }
        read_seg(hbm_ptr, segment_buffer, pma_data_base_addr, i);
        for (int j = 0 ; j < 64 ; j++)
        {
            #pragma HLS PIPELINE II=1
            if (segment_buffer[j] != UNUSED_ICON) count++;
        }
    }
    return count;
}


void redistribute(int seg_start, int seg_end, BYTE* hbm_ptr, int pma_data_base_addr)
{
    HERE
    int valid_buffer[SEGMENT_SIZE];
    for (int i = 0 ; i < SEGMENT_SIZE ; i++)
    {
        valid_buffer[i] = UNUSED_ICON;
    }
    int collect_buffer[SEGMENT_SIZE];
    int pack_ptr = 0;
    int pack_buffer_ptr = 0;
    int complete_seg = 0;
    int seg_count = seg_end - seg_start;
    for (int i = seg_start ; i < seg_end ; i++)
    {
        // ap_uint<512>* collect_buffer_ptr = (ap_uint<512>*)(collect_buffer);
        // for (int j = 0 ; j < SEGMENT_SIZE * sizeof(int) / 64 ; j++)
        // {
        //     #pragma HLS UNROLL
        //     collect_buffer_ptr[j] = 
        //     hbm_ptr[(pma_data_base_addr + i * SEGMENT_SIZE * sizeof(int)) / 64 + j];
        // }
        read_seg(hbm_ptr, collect_buffer,  pma_data_base_addr, i);
        for (int j = 0 ; j < SEGMENT_SIZE ; j++)
        {
            if (collect_buffer[j] != UNUSED_ICON)
            {
                valid_buffer[pack_buffer_ptr] = collect_buffer[j];
                pack_buffer_ptr++;
                pack_ptr++;
                if (pack_buffer_ptr == SEGMENT_SIZE)
                {
                    // ap_uint<512>* valid_buffer_ptr = (ap_uint<512>*)(valid_buffer);
                    // for (int k = 0 ; k < SEGMENT_SIZE * sizeof(int) / 64 ; k++)
                    // {
                    //     hbm_ptr[(pma_data_base_addr + (complete_seg) * SEGMENT_SIZE * sizeof(int)) / 64 + k]
                    //         = valid_buffer_ptr[k];
                    // }
                    write_seg(hbm_ptr, valid_buffer, pma_data_base_addr, complete_seg);
                    pack_buffer_ptr = 0;
                    complete_seg++;
                }
            }
        }
    }
    
    pack_buffer_ptr --;
    int write_back_buffer[SEGMENT_SIZE];
    int base_num = pack_ptr / seg_count;
    int extra_num = pack_ptr % seg_count;
    for (int s = seg_end - 1 ; s >= seg_start ; s--)
    {
        for (int i = 0 ; i < SEGMENT_SIZE ; i++)
        {
            write_back_buffer[i] = UNUSED_ICON;
        }
        int valid_num = base_num + (s < extra_num? 1:0);
        for (int i = valid_num - 1 ; i >= 0 ; i--)
        {
            if (pack_buffer_ptr == -1)
            {
                complete_seg--;
                // ap_uint<512>* valid_buffer_ptr = (ap_uint<512>*)(valid_buffer);
                // for (int k = 0 ; k < SEGMENT_SIZE * sizeof(int) / 64 ; k++)
                // {
                //     valid_buffer_ptr[k] 
                //         = hbm_ptr[(pma_data_base_addr + (complete_seg) * SEGMENT_SIZE * sizeof(int)) / 64 + k];
                // }
                read_seg(hbm_ptr, valid_buffer, pma_data_base_addr, complete_seg);
                pack_buffer_ptr = SEGMENT_SIZE - 1;
            }
            write_back_buffer[i] = valid_buffer[pack_buffer_ptr];
            pack_buffer_ptr--;
        }
        
        // for (int i = 0 ; i < SEGMENT_SIZE * sizeof(int) / 64 ; i++)
        // {
        //     ap_uint<512>* write_back_buffer_ptr = (ap_uint<512>*) write_back_buffer;
        //     hbm_ptr[(s * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr) / 64 + i] = write_back_buffer_ptr[i];
        // }
        write_seg(hbm_ptr, write_back_buffer, pma_data_base_addr, s);
    }
}

void rebalance_cu(
    BYTE* hbm_ptr,
    hls::stream<PMA_HEADER>& pma_header_in,
    hls::stream<VEC_INFO>& pma_addr_in,
    hls::stream<pma_idx_info>& pma_idx,
    hls::stream<inserted_seg>& inserted_seg_info,
    hls::stream<COMMAND_STREAM>& ack_back_to_router
)
{
        
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
        auto to_check = inserted_seg_info.read();
        int seg_count = pma_header.vertex_range[15];

        const int pma_data_base_addr = pma_addr.range(31,0) + sizeof(PMA_HEADER);
        const int pma_data_size_bytes = pma_addr.range(63,32) - sizeof(PMA_HEADER);

        
        for (int i = 0 ; i < 16 ; i++)
        {
            if (to_check.vertex_number[i] == -1) continue;

            int v = to_check.vertex_number[i];
            int relative_seg_num = to_check.old_vertex_relative_seg_number[i];

            int seg_start = (v == 0) ? 0 : pma_header.vertex_range[v-1];
            int seg_end = pma_header.vertex_range[v];
            int seg_num = seg_end - seg_start; // 当前vertex的segment数
            int total_valid_count = 0;
            int seg_idx = seg_start + relative_seg_num;
            total_valid_count = valid_count(seg_idx, seg_idx + 1, hbm_ptr, pma_data_base_addr);
            int last_left = seg_idx;
            int last_right = seg_idx + 1;


            int max_edge = SEGMENT_SIZE * DENSITY_BOUND;
            if (total_valid_count < max_edge) continue;
            for (int cur_seg_num = 2 ; cur_seg_num <= seg_num ; cur_seg_num *= 2)
            {
                int left = seg_start + int(seg_idx / cur_seg_num) * cur_seg_num;
                int right = seg_start + (int(seg_idx / cur_seg_num) + 1) * cur_seg_num;
                // int valid = valid_count(left, right);
                if (left < last_left) {
                    total_valid_count += valid_count(left, last_left, hbm_ptr, pma_data_base_addr);
                }
                else if (right > last_right) {
                    total_valid_count += valid_count(last_right, right, hbm_ptr, pma_data_base_addr);
                }

                max_edge = (right - left) * SEGMENT_SIZE * DENSITY_BOUND; 
                if (total_valid_count <= max_edge)
                {
                    redistribute(left, right, hbm_ptr, pma_data_base_addr);
                    continue;
                }
                last_left = left;
                last_right = right;
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
        int pma_data_start = pma_base_addr.range(31,0) + sizeof(PMA_HEADER);
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
                    // ap_uint<512>* hbm_wide = (ap_uint<512>*)(hbm_ptr + (int)pma_base_addr.range(31,0) + sizeof(PMA_HEADER) + current_seg_idx * SEGMENT_SIZE * sizeof(int));
                    // ap_uint<512>* seg_wide = (ap_uint<512>*)&tmp_seg;
                    // const int seg_chunks = sizeof(tmp_seg) / 64; // SEGMENT_SIZE*4 / 64
                    // for (int i = 0; i < seg_chunks; i++) {
                    //     #pragma HLS UNROLL
                    //     hbm_ptr[i + (pma_data_start + current_seg_idx * SEGMENT_SIZE * sizeof(int)) / 64] = seg_wide[i];
                    // }
                    write_seg(hbm_ptr, (int*)(&tmp_seg), pma_data_start, current_seg_idx);
                }
                current_seg_idx = target_seg_idx;
                // {
                //     ap_uint<512>* hbm_wide = (ap_uint<512>*)(hbm_ptr + (int)pma_base_addr.range(31,0) + sizeof(PMA_HEADER) + current_seg_idx * SEGMENT_SIZE * sizeof(int));
                    // ap_uint<512>* seg_wide = (ap_uint<512>*)&tmp_seg;
                    // const int seg_chunks = sizeof(tmp_seg) / 64; // SEGMENT_SIZE*4 / 64
                    // for (int i = 0; i < seg_chunks; i++) {
                    //     #pragma HLS UNROLL
                    //     seg_wide[i] = hbm_ptr[i + (pma_data_start + current_seg_idx * SEGMENT_SIZE * sizeof(int)) / 64];
                    // }
                    read_seg(hbm_ptr, (int*)(&tmp_seg), pma_data_start, current_seg_idx);
                // }
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
        // {
        //     ap_uint<512>* hbm_wide = (ap_uint<512>*)(hbm_ptr + (int)pma_base_addr.range(31,0) + sizeof(PMA_HEADER) + current_seg_idx * sizeof(int) * SEGMENT_SIZE);
            // ap_uint<512>* seg_wide = (ap_uint<512>*)&tmp_seg;
            // const int seg_chunks = sizeof(tmp_seg) / 64; // SEGMENT_SIZE*4 / 64
            // for (int i = 0; i < seg_chunks; i++) {
            //     #pragma HLS UNROLL
            //     hbm_ptr[i + (pma_data_start + current_seg_idx * SEGMENT_SIZE * sizeof(int) / 64)] = seg_wide[i];
            // }
            write_seg(hbm_ptr, (int*)(&tmp_seg), pma_data_start, current_seg_idx);
        // }
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
    int unused_data[SEGMENT_SIZE];
    for (int i = 0 ; i < 64 ; i++)
    {
        #pragma HLS UNROLL
        unused_data[i] = UNUSED_ICON;
    }
    while (true)
    {
        auto pma_info = pma_idx_info_in.read();
        if (pma_info.last == 1)
        {
            HERE
            break;
        }
        auto header = header_stream.read();
        auto old_header = header;
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

            for (int i = 0 ; i < VERTEX_PER_PMA ; i++)
            {
                #pragma HLS PIPELINE II=1
                int seg_start = i == 0 ? 0 : old_header.vertex_range[i - 1];
                int seg_end = old_header.vertex_range[i];
                int new_seg_start = (i == 0) ? 0 : header.vertex_range[i - 1];
                int new_seg_end = header.vertex_range[i];
                int old_data_start = (int)old_addr.range(31,0) + sizeof(PMA_HEADER);
                int new_data_start = (int)new_addr.range(31,0) + sizeof(PMA_HEADER);
                
                for (int s1 = seg_start, s2=new_seg_start; s1 < seg_end; s1++,s2++)
                {
                    // int old_idx = old_data_start / 64 + (s1 * SEGMENT_SIZE * sizeof(int)) / 64;
                    // int new_idx = new_data_start / 64 + (s2 * SEGMENT_SIZE * sizeof(int)) / 64;
                    
                    int old_idx = (old_data_start << 6) + ((s1 * SEGMENT_SIZE * sizeof(int)) << 6);
                    int new_idx = (new_data_start << 6) + ((s2 * SEGMENT_SIZE * sizeof(int)) << 6);
                    // 复制一个段（SEGMENT_SIZE*4字节=256字节=4个ap_uint<512>）
                    // ap_uint<512> temp_buffer[4];
                    // #pragma HLS ARRAY_PARTITION variable=temp_buffer complete
                    
                    // // Burst读取：一次性读取整个段
                    // READ_BURST: 
                    // for (int k = 0; k < SEGMENT_SIZE * sizeof(int) / 64; k++) {
                    //     #pragma HLS UNROLL
                    //     temp_buffer[k] = hbm_ptr[old_idx + k];
                    // }
                    
                    // // Burst写入：一次性写入整个段
                    // WRITE_BURST: 
                    // for (int k = 0; k < SEGMENT_SIZE * sizeof(int) / 64; k++) {
                    //     #pragma HLS UNROLL
                    //     hbm_ptr[new_idx + k] = temp_buffer[k];
                    // }
                    int seg_buffer[SEGMENT_SIZE];
                    read_seg(hbm_ptr, seg_buffer, old_data_start, s1);
                    write_seg(hbm_ptr, seg_buffer, new_data_start, s2);
                }
                for (int s = new_seg_start + (seg_end - seg_start) ; s < new_seg_end; s++)
                {
                    // int idx =  new_data_start / 64 + (s * SEGMENT_SIZE * sizeof(int)) / 64;
                    // for (int k = 0; k < SEGMENT_SIZE * sizeof(int) / 64; k++) {
                    //     #pragma HLS UNROLL
                    //     hbm_ptr[idx + k] = unused_data;
                    // }
                    write_seg(hbm_ptr, unused_data, new_data_start, s);
                }

            }
            // {
            //     int src_addr = (int)old_addr.range(31,0);
            //     int dst_addr = (int)new_addr.range(31,0);
            //     int move_bytes = (int)old_addr.range(63,32);
            //     int move_chunks = move_bytes / 64; // 每块64字节
                
            //     ap_uint<512>* src_ptr = (ap_uint<512>*)(hbm_ptr + src_addr);
            //     ap_uint<512>* dst_ptr = (ap_uint<512>*)(hbm_ptr + dst_addr);

            //     for (int i = 0; i < move_chunks; i++) {
            //         #pragma HLS PIPELINE II=1
            //         dst_ptr[i] = src_ptr[i];
            //     }
            // }
            
            cmd_to_mem.write({.data = MEM_FREE});
            vec_info_to_mem.write({.data=old_addr});
            int new_data_start = (int)new_addr.range(31,0) + (int)old_addr.range(63,32);
            // int unused_segment[SEGMENT_SIZE];


            // for (int i = 0; i < SEGMENT_SIZE; i++) {
            //     #pragma HLS UNROLL
            //     unused_segment[i] = UNUSED_ICON;
            // }

        }
        else  
        {
            new_addr = old_addr;
        }
        int bias = (int)new_addr.range(31,0);
        memcpy(hbm_ptr + bias, &header, sizeof(PMA_HEADER));
        // HERE
        // {
            // ap_uint<512>* hbm_wide = (ap_uint<512>*)(hbm_ptr + (int)new_addr.range(31,0));
        //     ap_uint<512>* header_wide = (ap_uint<512>*)&header;
        //     for (int i = 0; i < 4; i++) { // 256 / 64 = 4
        //         #pragma HLS PIPELINE
        //         hbm_ptr[i +  (int)new_addr.range(31,0) / 64] = header_wide[i];
        //     }
        // }
        pma_addr_out.write(new_addr);
        new_header_stream.write(header);
        HERE
    }
}

extern "C"
{
    void pma_insert_edge(
        VEC_INFO* pos_pma_addrs_read,
        VEC_INFO* pos_pma_addrs_write,
        VEC_INFO* neg_pma_addrs_read,
        VEC_INFO* neg_pma_addrs_write,
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
        #pragma HLS INTERFACE m_axi port=pos_pma_addrs_read offset=slave bundle=gmem0 depth=50
        #pragma HLS INTERFACE m_axi port=pos_pma_addrs_write offset=slave bundle=gmem0 depth=50
        #pragma HLS INTERFACE m_axi port=neg_pma_addrs_read offset=slave bundle=gmem1 depth=50 max_widen_bitwidth=512
        #pragma HLS INTERFACE m_axi port=neg_pma_addrs_write offset=slave bundle=gmem1 depth=50 max_widen_bitwidth=512
        #pragma HLS INTERFACE m_axi port=hbm_ptr_0 offset=slave bundle=gmem2 depth=50 max_widen_bitwidth=512 
        #pragma HLS INTERFACE m_axi port=hbm_ptr_1 offset=slave bundle=gmem3 depth=50 max_widen_bitwidth=512
        #pragma HLS INTERFACE m_axi port=hbm_ptr_2 offset=slave bundle=gmem4 depth=50 max_widen_bitwidth=512 
        #pragma HLS INTERFACE m_axi port=hbm_ptr_3 offset=slave bundle=gmem5 depth=50 max_widen_bitwidth=512 
        #pragma HLS INTERFACE m_axi port=hbm_ptr_4 offset=slave bundle=gmem6 depth=50 max_widen_bitwidth=512 
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

        hls::stream<inserted_seg> inserted_seg_info("inserted_seg_info");
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
            pos_pma_addrs_read, 
            neg_pma_addrs_read, 
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
            pma_idx_info_request[3],
            inserted_seg_info
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
            pos_pma_addrs_write,
            neg_pma_addrs_write,
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
            inserted_seg_info,
            ack_back_to_router
        );
    }
}