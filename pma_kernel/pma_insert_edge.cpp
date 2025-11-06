#include "data_type.h"
#include <cstring>
#include <cxxabi.h>
#include <hls_stream.h>
#include "debug.h"
#include "pma.h"
#include <ap_utils.h>

#define MAX_REQUEST 10
struct InsertPos {
    int idx;      // 插入索引
    int val;      // 查找结果值
};
const int max_cache_size = MAX_PMA_NUM * sizeof(int) * 64;


void process_cmd(
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
    hls::stream<int> &cmd_count_stream,
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_batch_stream,
    hls::stream<PMA_UPDATE_MESSAGE> &first_cmd_to_header_read_info
)
{
    while(true)
    {
        PMA_UPDATE_MESSAGE cmd_batch[MAX_CMD_NUM];
        #pragma HLS ARRAY_PARTITION variable=cmd_batch complete
        int cmd_count = 0;

        // 读取直到 last=1
        for (int i = 0; i < MAX_CMD_NUM; i++) {
            #pragma HLS PIPELINE II=1
            cmd_batch[i] = cmd_stream.read();
            cmd_count++;
            if (cmd_batch[i].last == 1) {
                break;
            }
        }

        // 将批次数据和数量写入输出流
        cmd_count_stream.write(cmd_count);
        for (int i = 0; i < cmd_count; i++) {
            #pragma HLS PIPELINE II=1
            cmd_batch_stream.write(cmd_batch[i]);
        }
        if (cmd_batch[0].data == PMA_STOP)
        {
            break;
        }
    }
}

void sort_cu(
    hls::stream<int> cmd_count_stream,
    hls::stream<PMA_UPDATE_MESSAGE> cmd_batch_stream,
    hls::stream<ap_uint<64>> edge_info_stream,
    hls::stream<int> &edge_count
)
{
    while (true)
    {
        int cmd_count = cmd_count_stream.read();
        ap_uint<64> edge_info[MAX_CMD_NUM];
        #pragma HLS ARRAY_PARTITION variable=edge_info complete

        // 收集边信息
        for (int i = 0; i < cmd_count; i++) {
            #pragma HLS PIPELINE II=1
            auto cmd = cmd_batch_stream.read();
            if (cmd.data == PMA_STOP)
            {
                return;
            }
            edge_info[i].range(63,32) = cmd.data.range(31,0); // src
            edge_info[i].range(31,0)  = cmd.data.range(63,32); // dst
        }
        SORT_EDGES:
        for (int i = 1; i < cmd_count; ++i) {
            #pragma HLS LOOP_TRIPCOUNT min=0 max=MAX_CMD_NUM
            ap_uint<64> key = edge_info[i];
            int j = i - 1;
            SHIFT_LOOP:
            while (j >= 0 && edge_info[j] > key) {
                #pragma HLS LOOP_TRIPCOUNT min=0 max=MAX_CMD_NUM
                #pragma HLS PIPELINE II=1
                edge_info[j + 1] = edge_info[j];
                j--;
            }
            edge_info[j + 1] = key;
        }
        edge_count.write(cmd_count);
        for (int i = 0; i < cmd_count ; i++)
        {
            edge_info_stream.write(edge_info[i]);
        }
    }
}

void head_read(
    VEC_INFO* pos_pma_addrs,
    VEC_INFO* neg_pma_addrs,
    hls::stream<PMA_UPDATE_MESSAGE> &first_cmd_to_header_read_info,
    hls::stream<PMA_HEADER>& header_out,
    hls::stream<VEC_INFO>& request_addr,
    hls::stream<VEC_INFO>& pma_addr,
    hls::stream<ap_uint<512>>& memory_chunk
)
{
    while (true)
    {
        auto cmd = first_cmd_to_header_read_info.read();
        if (cmd.data == PMA_STOP)
        {
            break;
        }
        int pma_idx = cmd.data.range(31,0) / VERTEX_PER_PMA;
        VEC_INFO vec;
        if (cmd.data.range(98,98) == IN_DIRECTION)
        {
            vec = neg_pma_addrs[pma_idx];
        }
        else
        {
            vec = pos_pma_addrs[pma_idx];
        }
        pma_addr.write(vec);
        vec.range(63,32) = sizeof(PMA_HEADER);
        
        PMA_HEADER header;
        ap_uint<512>* header_ptr = (ap_uint<512>*)&header;
        request_addr.write(vec);
        for (int i = 0 ; i < sizeof(PMA_HEADER) / 64; i++)
        {
            ap_uint<512> chunk = memory_chunk.read();
            header_ptr[i] = chunk;
        }
        header_out.write(header);
    }
}
void binary_search_cu(
    hls::stream<PMA_HEADER> &header_info_in_stream,
    hls::stream<VEC_INFO> &pma_addr,
    hls::stream<ap_uint<512>> &memory_chunk,
    hls::stream<VEC_INFO> &vec_info_to_mem,
    hls::stream<ap_uint<64>> &edge_info_stream,
    hls::stream<int> &edge_count,
    hls::stream<InsertPos> &insert_info,
    hls::stream<int> &insert_count,
    hls::stream<VEC_INFO> &new_alloc_addr
)
{
    while (true)
    {
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
                #pragma HLS HLS bind_storage variable=RAM_2P type=bram
                
                VEC_INFO request;
                const int chunks_per_segment = (SEGMENT_SIZE * sizeof(int) * 8) / 512; // 结果应为 4
                request.range(31,0) = pma_data_base_addr + mid * SEGMENT_SIZE * sizeof(int);
                request.range(63,32) = SEGMENT_SIZE * sizeof(int);
                vec_info_to_mem.write(request);
                ap_uint<512>* storage_ptr = (ap_uint<512>*)storage;
                READ_SEGMENT_LOOP:
                for (int k = 0; k < chunks_per_segment; k++) {
                    #pragma HLS PIPELINE II=1
                    ap_wait();
                    storage_ptr[k] = memory_chunk.read();
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
                header.vertex_range[local_src]++;
            }
            insert_info.write(to_insert);
        }
        insert_count.write(find_count);
        int need_extra_seg = 0;
        for (int v = 0 ; v < VERTEX_PER_PMA ; v++)
        {
            int seg_start = v == 0 ? 0 : header.vertex_range[v-1];
            int seg_end = header.vertex_range[v];
            int max_edge = (seg_end - seg_start) * SEGMENT_SIZE * DENSITY_BOUND;
            if (max_edge > header.edge_count[v])
            {
                need_extra_seg += seg_end - seg_start;
            }
        }

        if (need_extra_seg)
        {
            VEC_INFO vec_info;
            vec_info.range(31,0) = 0;//alloc 只看长度
            vec_info.range(63,32) = pma_data_base_addr + need_extra_seg * sizeof(BYTE) * SEGMENT_SIZE;
            new_alloc_addr.write(vec_info);
        }
    }
}


void rebalance_cu(
    hls::stream<PMA_HEADER>& pma_header_in,
    hls::stream<VEC_INFO>& pma_addr_in,
    hls::stream<int>& pma_idx,
    hls::stream<COMMAND_STREAM>& ack_back,

    hls::stream<VEC_INFO>& read_req_out,         // 输出: 发送读段请求
    hls::stream<ap_uint<512>>& read_data_in,     // 输入: 接收段数据
    hls::stream<VEC_INFO>& write_req_out,        // 输出: 发送写段请求
    hls::stream<ap_uint<512>>& write_data_out   // 输出: 发送要写入的段数据
)
{
    PMA_HEADER pma_header = pma_header_in.read();
    VEC_INFO pma_addr = pma_addr_in.read();


    const int pma_data_base_addr = pma_addr.range(31,0) + sizeof(PMA_HEADER);
    const int pma_data_size_bytes = pma_addr.range(63,32) - sizeof(PMA_HEADER);
    const int pma_data_size_ints = pma_data_size_bytes / sizeof(int);
    const int num_chunks_total = (pma_data_size_bytes + 63) / 64;
    const int INTS_PER_CHUNK = 512 / 32;
    
    int valid_data_buffer[INTS_PER_CHUNK];
    #pragma HLS ARRAY_PARTITION variable=valid_data_buffer complete
    for (int k = 0 ; k < INTS_PER_CHUNK ; k++)
    {
        #pragma HLS UNROLL 
        valid_data_buffer[k] = UNUSED_ICON;
    }
    //PACK_PHASE:
    int pack_ptr_ints = pma_data_size_ints;
    int buffer_count = INTS_PER_CHUNK - 1;
    PACK_MAIN_LOOP:
    for (int i = num_chunks_total - 1; i >= 0; --i) {
        #pragma HLS LOOP_TRIPCOUNT min=1 max=4096
        VEC_INFO read_req;
        read_req.range(31,0) = pma_data_base_addr + i * 64;
        read_req.range(63,32) = 64;
        read_req_out.write(read_req);

        ap_uint<512> chunk_data = read_data_in.read();
        int* chunk_data_ints = (int*)(&chunk_data);
        
        PACK_INNER_LOOP:
        for (int j = INTS_PER_CHUNK - 1; j >= 0; --j) {
            #pragma HLS PIPELINE II=1
            if (chunk_data_ints[j] != UNUSED_ICON) {
                valid_data_buffer[buffer_count] = chunk_data_ints[j];
                buffer_count--;
                if (buffer_count == -1) {
                    pack_ptr_ints -= INTS_PER_CHUNK;
                    VEC_INFO write_req;
                    write_req.range(31,0) = pma_data_base_addr + pack_ptr_ints * sizeof(int);
                    write_req.range(63,32) = 64;
                    write_req_out.write(write_req);
                    
                    ap_uint<512> write_chunk;
                    int* write_chunk_ints = (int*)(&write_chunk);
                    for(int k=0; k<INTS_PER_CHUNK; ++k) {
                        #pragma HLS UNROLL
                        write_chunk_ints[k] = valid_data_buffer[k];
                    }
                    for (int k = 0 ; k < INTS_PER_CHUNK ; k++)
                    {
                        #pragma HLS UNROLL 
                        valid_data_buffer[k] = UNUSED_ICON;
                    }
                    write_data_out.write(write_chunk);
                    buffer_count = INTS_PER_CHUNK - 1;
                }
            }
        }
    }
    // 处理缓冲区中剩余的数据
    if (buffer_count < INTS_PER_CHUNK - 1) {
        int element_num = INTS_PER_CHUNK - buffer_count - 1;
        pack_ptr_ints -= element_num;
        VEC_INFO write_req;
        write_req.range(31,0) = pma_data_base_addr + pack_ptr_ints * sizeof(int);
        write_req.range(63,32) = element_num * sizeof(int);
        write_req_out.write(write_req);

        ap_uint<512> write_chunk;
        int* write_chunk_ints = (int*)(&write_chunk);
        for(int k=0; k < element_num; ++k) {
            #pragma HLS UNROLL
            write_chunk_ints[k] = valid_data_buffer[buffer_count + 1 + k];
        }
        write_data_out.write(write_chunk);
    }
    //SPARSE_PHASE: 
    int current_read_ptr_ints = pack_ptr_ints;

    for (int v = 0 ; v < VERTEX_PER_PMA ; v++)
    {
        #pragma HLS LOOP_TRIPCOUNT min=16 max=16
        int seg_start_idx = (v == 0) ? 0 : pma_header.vertex_range[v - 1];
        int seg_end_idx = pma_header.vertex_range[v];
        int seg_count = seg_end_idx - seg_start_idx;
        if (seg_count == 0) continue;

        int edge_num = pma_header.edge_count[v];
        // if (edge_num == 0) continue;

        int base = edge_num / seg_count;
        int extra = edge_num % seg_count;

        REBALANCE_SEG_LOOP:
        for (int s = 0; s < seg_count; ++s) {
            #pragma HLS LOOP_TRIPCOUNT min=0 max=64
            int num_to_move = base + (s < extra ? 1 : 0);
            int target_seg_start_addr = pma_data_base_addr + (seg_start_idx + s) * SEGMENT_SIZE * sizeof(int);
            const int CHUNKS_PER_SEGMENT = SEGMENT_SIZE * sizeof(int) / 64;
            // if (num_to_move == 0) continue;
            if (num_to_move == 0)
            {
                VEC_INFO write_req;
                write_req.range(31,0) = target_seg_start_addr;
                write_req.range(63,32) = SEGMENT_SIZE * sizeof(int);
                write_req_out.write(write_req);

                ap_uint<512> unused_chunk;
                int* unused_chunk_ints = (int*)(&unused_chunk);
                for(int i=0; i<INTS_PER_CHUNK; ++i) {
                    #pragma HLS UNROLL
                    unused_chunk_ints[i] = UNUSED_ICON;
                }
                for (int k = 0; k < CHUNKS_PER_SEGMENT; ++k) {
                    #pragma HLS PIPELINE II=1
                    write_data_out.write(unused_chunk);
                }
                continue;
            }

            
            // 从紧缩区域读取数据并写到新的位置
            // 这是一个简化的内存到内存移动，实际可能需要一个中间CU来处理
            VEC_INFO read_req;
            read_req.range(31,0) = pma_data_base_addr + current_read_ptr_ints * sizeof(int);
            read_req.range(63,32) = num_to_move * sizeof(int);
            read_req_out.write(read_req);

            ap_uint<2048> segment_buffer;
            int* seg_int_ptr = (int*)(&segment_buffer);
            
            ap_uint<2048> valid_data;
            for (int k = 0 ; k < (num_to_move * sizeof(int) + 63) / 64 ; k++)
            {
                ap_uint<512>* chunk_ptr = (ap_uint<512>*)(&valid_data);
                chunk_ptr[k] = read_data_in.read();
            }
            int intra_seg_ptr = 0;
            for (;intra_seg_ptr < num_to_move; intra_seg_ptr++)
            {
                int *data_int_ptr = (int*)(&valid_data);
                seg_int_ptr[intra_seg_ptr] = data_int_ptr[intra_seg_ptr];
            }
            for (; intra_seg_ptr < SEGMENT_SIZE ; intra_seg_ptr++)
            {
                seg_int_ptr[intra_seg_ptr] = UNUSED_ICON;
            }

            VEC_INFO write_req;
            write_req.range(31,0) = target_seg_start_addr;
            write_req.range(63,32) = SEGMENT_SIZE * sizeof(int);
            write_req_out.write(write_req);
            ap_uint<512>* chunk_ptr = (ap_uint<512>*)(&segment_buffer);
            MOVE_DATA_CHUNKS:
            for (int k = 0; k < CHUNKS_PER_SEGMENT; ++k) {
                #pragma HLS PIPELINE II=1
                auto data = chunk_ptr[k];
                write_data_out.write(data);
            }
            current_read_ptr_ints += num_to_move;
        }
    }
    int idx = pma_idx.read();
    ack_back.write({.data=idx});
}   


void insert_element_cu(
    hls::stream<InsertPos> &insert_info,
    hls::stream<int> &insert_count,
    hls::stream<VEC_INFO> &pma_addr,
    hls::stream<PMA_HEADER>& pma_header,
    hls::stream<VEC_INFO>& read_req_out,         // 输出: 发送读段请求
    hls::stream<ap_uint<512>>& read_data_in,     // 输入: 接收段数据
    hls::stream<VEC_INFO>& write_req_out,        // 输出: 发送写段请求
    hls::stream<ap_uint<512>>& write_data_out,   // 输出: 发送要写入的段数据
    hls::stream<COMMAND>& cmd_to_mem,
    hls::stream<VEC_INFO>& new_alloc_addr,
    hls::stream<PMA_HEADER>& pma_header_out,
    hls::stream<VEC_INFO>& pma_addr_out
)
{
    while (true)
    {
        int count = insert_count.read();
        if (count == 0) { // 假设 count=0 是结束信号
            break;
        }
        VEC_INFO pma_base_addr = pma_addr.read();
        PMA_HEADER header = pma_header.read();
        pma_header_out.write(header);
        pma_addr_out.write(pma_base_addr);
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
            int seg_start = v == 0 ? 0 : header.vertex_range[v-1];
            int seg_end = header.vertex_range[v];
            int max_edge = (seg_end - seg_start) * SEGMENT_SIZE * DENSITY_BOUND;
            if (max_edge > header.edge_count[v])
            {
                need_extra_seg += seg_end - seg_start;
            }
        }

        if (need_extra_seg)
        {
            VEC_INFO vec_info;
            vec_info.range(31,0) = 0;//alloc 只看长度
            vec_info.range(63,32) = pma_base_addr.range(63,32) + need_extra_seg * sizeof(BYTE) * SEGMENT_SIZE;
            new_alloc_addr.write(vec_info);
        }
        
        
        // #pragma HLS ARRAY_PARTITION variable=pos_batch complete
        // int tmp_seg[SEGMENT_SIZE];
        
        int current_seg_idx = -1;
        const int BITS_PER_SEG = SEGMENT_SIZE * 32;
        int one_seg_updates = 0;
        const int chunks_per_segment = BITS_PER_SEG / 512; // 结果为 4
        ap_uint<BITS_PER_SEG> tmp_seg;
        ap_uint<512>* tmp_seg_ptr = (ap_uint<512>*)(&tmp_seg);
        PROCESS_INSERT_BATCH:
        for (int k = 0; k < count; k++)
        {
            #pragma HLS LOOP_TRIPCOUNT min=0 max=MAX_CMD_NUM
            #pragma HLS LOOP_TRIPCOUNT min=0 max=MAX_CMD_NUM
            InsertPos pos = pos_batch[k];
            int insert_abs_idx = pos.idx + 1;
            int target_seg_idx = insert_abs_idx / SEGMENT_SIZE;
            if (target_seg_idx != current_seg_idx) 
            {
                // a. 如果之前有段被修改，先写回
                one_seg_updates = 0;
                if (current_seg_idx != -1) 
                {
                    VEC_INFO write_req;
                    write_req.range(31,0) = pma_base_addr.range(31,0) + current_seg_idx * SEGMENT_SIZE * sizeof(int); 
                    write_req.range(63,32) = SEGMENT_SIZE * sizeof(int);
                    write_req_out.write(write_req);
                    for (int i = 0; i < chunks_per_segment; i++) 
                    {
                        #pragma HLS PIPELINE II=1
                        write_data_out.write(tmp_seg_ptr[i]);
                    }
                }
                current_seg_idx = target_seg_idx;
                VEC_INFO read_req;
                read_req.range(31,0) = pma_base_addr.range(31,0) + current_seg_idx * SEGMENT_SIZE * sizeof(int);
                read_req.range(63,32) = SEGMENT_SIZE * sizeof(int);
                read_req_out.write(read_req);
                READ_NEW_SEG:
                for (int i = 0; i < chunks_per_segment; i++) {
                    #pragma HLS PIPELINE II=1
                    ap_wait();
                    tmp_seg_ptr[i] = read_data_in.read();
                }
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
        VEC_INFO write_req;
        write_req.range(31,0) = pma_base_addr.range(31,0) + current_seg_idx * sizeof(int) * SEGMENT_SIZE;
        write_req.range(63,32) = SEGMENT_SIZE * sizeof(int);
        write_req_out.write(write_req);
        for (int i = 0; i < chunks_per_segment; i++) 
        {
            #pragma HLS PIPELINE II=1
            write_data_out.write(tmp_seg_ptr[i]);
        }
    }
}
void write_vec_info(
    hls::stream<VEC_INFO>& vec_info_stream,
    hls::stream<int>& pma_idx,
    hls::stream<int>& direction,
    VEC_INFO *pos_addr,
    VEC_INFO *neg_addr
)
{
    while(true)
    {
        //处理退出逻辑
        auto vec_info = vec_info_stream.read();
        auto dir = direction.read();
        int idx = pma_idx.read();
        if (dir == OUT_DIRECTION)
            pos_addr[idx] = vec_info;
        else if (dir == IN_DIRECTION)
            neg_addr[idx] = vec_info;
    }
}
void move_and_change_header(
    hls::stream<PMA_HEADER>& header_stream,
    hls::stream<PMA_HEADER>& new_header_stream,
    hls::stream<VEC_INFO>& new_alloc_addr,
    hls::stream<COMMAND>& to_free_cmd,
    hls::stream<VEC_INFO>& pma_addr_in,
    hls::stream<VEC_INFO>& pma_addr_out,
    hls::stream<VEC_INFO>& to_free_pos,
    hls::stream<VEC_INFO>& updated_pma_addr_to_mem,
    hls::stream<int>& direction_in,
    hls::stream<int>& direction_out,
    hls::stream<int>& pma_index_in,
    hls::stream<int>& pma_idx_out,

    hls::stream<VEC_INFO>& mem_read_req,
    hls::stream<ap_uint<512>>& mem_read_data,
    hls::stream<VEC_INFO>& mem_write_req,
    hls::stream<ap_uint<512>>& mem_write_data
)
{
    while (true)
    {
        auto header = header_stream.read();
        int direction = direction_in.read();
        int pma_index = pma_index_in.read();
        int enlarge_segs[16];
        for (int i = 0 ; i < 16 ; i++)
        {
            #pragma HLS UNROLL
            enlarge_segs[i] = 0;
        }
        bool enlarged = false;
        for (int v = 0 ; v < VERTEX_PER_PMA ; v++)
        {
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
            total_enlarge_segs += enlarge_segs[i];
            header.vertex_range[i] += total_enlarge_segs;
        }
        VEC_INFO old_addr = pma_addr_in.read();
        VEC_INFO new_addr;
        if (enlarged)
        {
            new_addr = new_alloc_addr.read();
            updated_pma_addr_to_mem.write(new_addr);
            updated_pma_addr_to_mem.write(new_addr);
            direction_out.write(direction);
            pma_idx_out.write(pma_index);
            to_free_pos.write(old_addr);

            mem_read_req.write(old_addr);
            VEC_INFO write_addr;
            write_addr.range(31,0) = new_addr.range(31,0);
            write_addr.range(63,32) = old_addr.range(63,32);
            mem_write_req.write(write_addr);
            //分配时候按照1024对齐的因此不会有余数
            for (int i = 0 ; i < old_addr.range(63,32) / 64 ; i++)
            {
                auto data = mem_read_data.read();
                ap_wait();
                mem_write_data.write(data);
            }
            //to be done move data
            to_free_cmd.write(MEM_FREE);
            to_free_pos.write(new_addr);
        }
        else  
        {
            new_addr = old_addr;
        }

        pma_addr_out.write(new_addr);
        new_header_stream.write(header);
    }
}
void send_alloc_free_request(
    hls::stream<VEC_INFO>& request_in,
    hls::stream<COMMAND>& cmd_in,
    hls::stream<VEC_INFO_STREAM>& request_to_mem,
    hls::stream<COMMAND_STREAM>& cmd_to_mem
)
{
    while (true)
    {
        auto cmd = cmd_in.read();
        if (cmd == PMA_STOP)
        {
            break;
        }
        // Send the request to memory
        auto request_info = request_in.read();
        request_to_mem.write({.data = request_info});
        cmd_to_mem.write({.data = cmd});
    }
}
void receive_alloc_request(
    hls::stream<VEC_INFO_STREAM>& request_back,
    hls::stream<VEC_INFO>& pma_addr_back
)
{
    while(true)
    {
        auto request_info = request_back.read();
        pma_addr_back.write(request_info.data);
    }
}
void read_vec_info(
    BYTE* hbm_ptr,
    VEC_INFO* pos_pma_addrs,
    VEC_INFO* neg_pma_addrs,
    hls::stream<PMA_UPDATE_MESSAGE>& first_cmd_stream,
    hls::stream<VEC_INFO>& vec_info_stream
)
{
    
    int cache[max_cache_size / sizeof(int)];
    #pragma HLS bind_storage variable=cache type=ram_t2p impl=uram
    while (true)
    {
        auto cmd = first_cmd_stream.read();
        if (cmd.data == PMA_STOP)
        {
            break;
        }
        int pma_idx = cmd.data.range(31,0) / VERTEX_PER_PMA;
        int direction = cmd.data.range(98,98);
        VEC_INFO out;
        if (direction == IN_DIRECTION)
        {
            out = neg_pma_addrs[pma_idx];
        }
        else
        {
            out = pos_pma_addrs[pma_idx];
        }
        bool cacheable = false;
        int start = out.range(31,0);
        int len = out.range(63,32);
        vec_info_stream.write(out);
        
    }
}




void read_hbm(
    BYTE* hbm_ptr,
    hls::stream<VEC_INFO> vec_info_stream[MAX_REQUEST],
    hls::stream<ap_uint<512>> memory_chunk[MAX_REQUEST]
)
{
    int idx = 0;
    VEC_INFO vec_info;
    ap_uint<512> data;
    while (true)
    {
        idx = (idx + 1) % MAX_REQUEST;
        bool request = vec_info_stream[idx].read_nb(vec_info);
        if (!request) continue;
        if (vec_info == PMA_STOP)
        {
            break;
        }
        int start = vec_info.range(31,0);
        int len = vec_info.range(63,32);
        int num_chunks = (len + 63) / 64;
        for (int i = 0; i < num_chunks; i++)
        {
            #pragma HLS PIPELINE II=1
            memcpy(&data, hbm_ptr + start + i * 64, 64);
            memory_chunk[idx].write(data);
        }
    }
}

void write_hbm(
    BYTE* hbm_ptr,
    hls::stream<VEC_INFO> vec_info_stream[MAX_REQUEST],
    hls::stream<ap_uint<512>> memory_chunk[MAX_REQUEST]
)
{
    int idx = 0;
    VEC_INFO vec_info;
    ap_uint<512> data;
    while (true)
    {
        idx = (idx + 1) % MAX_REQUEST;
        bool request = vec_info_stream[idx].read_nb(vec_info);
        if (!request) continue;
        if (vec_info == PMA_STOP)
        {
            break;
        }
        int start = vec_info.range(31,0);
        int len = vec_info.range(63,32);
        int num_chunks = (len) / 64;
        int extra = len % 64;
        ap_uint<512> data;
        ap_uint<512>* hbm_chunk_ptr = (ap_uint<512>*)(hbm_ptr + start);
        for (int i = 0; i < num_chunks; i++)
        {
            #pragma HLS PIPELINE II=1
            data = memory_chunk[idx].read();
            hbm_chunk_ptr[i] = data;
            // memcpy(hbm_ptr + start + i * 64, &data, 64);
        }
        if (extra)
        {
            data = memory_chunk[idx].read();
            memcpy(hbm_ptr + start + num_chunks * 64, &data, extra);
        }

    }
}


extern "C"
{
    void pma_insert_edge(
        VEC_INFO* pos_pma_addrs,
        VEC_INFO* neg_pma_addrs,
        BYTE* hbm_ptr,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
        hls::stream<COMMAND_STREAM> &ack_back_to_router,
        //mem operation
        hls::stream<VEC_INFO_STREAM> &vec_info_to_mem_stream,
        hls::stream<COMMAND_STREAM> &cmd_to_mem_stream,
        hls::stream<VEC_INFO_STREAM> &vec_info_back_stream
    )
    {
        #pragma HLS INTERFACE m_axi port=pos_pma_addrs offset=slave bundle=gmem0
        #pragma HLS INTERFACE m_axi port=neg_pma_addrs offset=slave bundle=gmem1
        #pragma HLS INTERFACE m_axi port=hbm_ptr offset=slave bundle=gmem2
        #pragma HLS INTERFACE axis port=cmd_stream
        #pragma HLS INTERFACE axis port=ack_back_to_router
        #pragma HLS INTERFACE axis port=vec_info_to_mem_stream
        #pragma HLS INTERFACE axis port=cmd_to_mem_stream
        #pragma HLS INTERFACE axis port=vec_info_back_stream
        #pragma HLS DATAFLOW
        read_hbm();
        read_hbm();
        read_vec_info();
        write_vec_info();
        send_alloc_free_request();
        receive_alloc_request();
        process_cmd();
        sort_cu();
        head_read();
        binary_search_cu();
        insert_element_cu();
        move_and_change_header();
        rebalance_cu();
    }
}