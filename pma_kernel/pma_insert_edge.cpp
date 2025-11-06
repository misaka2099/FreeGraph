#include "data_type.h"
#include <cstring>
#include <cxxabi.h>
#include <hls_stream.h>
#include "debug.h"
#include "pma.h"
#include <ap_utils.h>


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





void insert_element(
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
        write_req.range(31,0) = pma_base_addr.range(31,0) + current_seg_idx * sizeof(int);
        write_req.range(63,32) = SEGMENT_SIZE * SEGMENT_SIZE * sizeof(int);
        write_req_out.write(write_req);
        for (int i = 0; i < chunks_per_segment; i++) 
        {
            #pragma HLS PIPELINE II=1
            write_data_out.write(tmp_seg_ptr[i]);
        }
    }
}
void vec_info_writer(
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
    hls::stream<int>& pma_idx_out
)
{
    while (true)
    {
        auto header = header_stream.read();
        int direction = direction_in.read();
        int pma_index = pma_index_in.read();
        int enlarge_segs[16];
        bool enlarged = false;
        for (int v = 0 ; v < VERTEX_PER_PMA ; v++)
        {
            int seg_start = v == 0 ? 0 : header.vertex_range[v-1];
            int seg_end = header.vertex_range[v];
            int max_edge = (seg_end - seg_start) * SEGMENT_SIZE * DENSITY_BOUND;
            if (max_edge > header.edge_count[v])
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
        if (enlarged)
        {
            VEC_INFO old_addr = pma_addr_in.read();
            VEC_INFO new_addr = new_alloc_addr.read();
            pma_addr_out.write(new_addr);
            updated_pma_addr_to_mem.write(new_addr);
            direction_out.write(direction);
            pma_idx_out.write(pma_index);
            to_free_pos.write(old_addr);
            

            //to be done move data
        }


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
    hls::stream<VEC_INFO>& vec_info_stream,
    hls::stream<ap_uint<512>>& memory_chunk
)
{
    while (true)
    {
        auto vec_info = vec_info_stream.read();
        if (vec_info == PMA_STOP)
        {
            break;
        }
        int start = vec_info.range(31,0);
        int len = vec_info.range(63,32);
        int num_chunks = (len + 63) / 64;
        ap_uint<512> data;
        for (int i = 0; i < num_chunks; i++)
        {
            #pragma HLS PIPELINE II=1
            memcpy(&data, hbm_ptr + start + i * 64, 64);
            memory_chunk.write(data);
        }
    }
}

void write_hbm(
    BYTE* hbm_ptr,
    hls::stream<VEC_INFO>& vec_info_stream,
    hls::stream<ap_uint<512>>& memory_chunk
)
{
    while (true)
    {
        auto vec_info = vec_info_stream.read();
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
            data = memory_chunk.read();
            hbm_chunk_ptr[i] = data;
            // memcpy(hbm_ptr + start + i * 64, &data, 64);
        }
        if (extra)
        {
            data = memory_chunk.read();
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
      
            // int cmd_count = 0;
            // PMA_UPDATE_MESSAGE cmd[MAX_CMD_NUM] = {0};
            // #pragma HLS ARRAY_PARTITION variable=cmd complete
            // const int TMP_BYTES = sizeof(int) * SEGMENT_SIZE * MAX_PMA_NUM;
            // static int tmp_storage[TMP_BYTES / sizeof(int)];
            
            // while (true)
            // {
            //     cmd_count = 0;
            //     for (int i = 0 ; i < MAX_CMD_NUM; i++)
            //     {
            //         #pragma HLS PIPELINE II = 1
            //         cmd[i] = cmd_stream.read();
            //         // HERE
            //         cmd_count++;
            //         if (cmd[i].last == 1)
            //         {
            //             break;
            //         }
            //     }
            //     if (cmd_count == 1 && cmd[0].data == PMA_STOP)
            //     {
            //         HERE
            //         break;
            //     }
            //     int pma_idx = cmd[0].data.range(31,0) / VERTEX_PER_PMA;
            //     int direction = cmd[0].data.range(98,98);
            //     int header_start = 0;
            //     int pma_len_in_byte = 0;
            //     if (direction == IN_DIRECTION)
            //     {
            //         if (neg_pma_addrs[pma_idx] != 0)
            //         {
            //             header_start = neg_pma_addrs[pma_idx].range(31,0);
            //             pma_len_in_byte = neg_pma_addrs[pma_idx].range(63,32);
            //         }
            //     }
            //     else
            //     {
            //         if (pos_pma_addrs[pma_idx] != 0)
            //         {
            //             header_start = pos_pma_addrs[pma_idx].range(31,0);
            //             pma_len_in_byte = pos_pma_addrs[pma_idx].range(63,32);
            //         }
            //     }  
            //     VEC_INFO_STREAM vec_info;
            //     PMA_HEADER header;
            //     memcpy(&header, hbm_ptr + header_start, sizeof(PMA_HEADER));
            //     ap_uint<64> edge_info[MAX_CMD_NUM];
            //     #pragma HLS ARRAY_PARTITION variable=batch_src complete
            //     #pragma HLS ARRAY_PARTITION variable=batch_dst complete

            //     // 收集所有要插入的 edge 到数组 便于排序
            //     for (int i = 0; i < cmd_count; ++i) 
            //     {
            //         #pragma HLS PIPELINE II=1
            //         edge_info[i].range(63,32) = cmd[i].data.range(31,0);
            //         edge_info[i].range(31,0) = cmd[i].data.range(63,32);
            //     }
            //     for (int i = 0; i < cmd_count; ++i) {
            //         #pragma HLS PIPELINE II=1
            //         edge_info[i].range(63,32) = cmd[i].data.range(31,0); // src
            //         edge_info[i].range(31,0)  = cmd[i].data.range(63,32); // dst
            //     }

            //     // 插入排序：按 src 升序；src 相同时按 dst 升序（适合 MAX_CMD_NUM 小的情况）
            //     SORT_EDGES:
            //     for (int i = 1; i < cmd_count; ++i) {
            //         #pragma HLS LOOP_TRIPCOUNT min=0 max=MAX_CMD_NUM
            //         ap_uint<64> key = edge_info[i];
            //         int j = i - 1;
            //         SHIFT_LOOP:
            //         while (j >= 0 && edge_info[j] > key) {
            //             #pragma HLS LOOP_TRIPCOUNT min=0 max=MAX_CMD_NUM
            //             #pragma HLS PIPELINE II=1
            //             edge_info[j + 1] = edge_info[j];
            //             j--;
            //         }
            //         edge_info[j + 1] = key;
            //     }
                
            //     int* storage;
            //     bool cacheable = false;
            //     int data_start = header_start + sizeof(PMA_HEADER);
            //     int total_ints = pma_len_in_byte / sizeof(int);
            //     if (total_ints * sizeof(int) <= TMP_BYTES) {
            //         cacheable = true;
            //         memcpy(tmp_storage, hbm_ptr + data_start, pma_len_in_byte);
            //         storage = tmp_storage;
            //     } else {
            //         storage = (int*)(hbm_ptr + data_start);
            //     }
            //     InsertPos pos_info[MAX_CMD_NUM];
            //     for (int i = 0 ; i < cmd_count ; i++)
            //     {
            //         int src = edge_info[i].range(63,32);
            //         int dst = edge_info[i].range(31,0);
            //         int local_src = src % VERTEX_PER_PMA;
            //         int left = (local_src == 0 ? 0 : header.vertex_range[local_src - 1]);
            //         int right = header.vertex_range[local_src];
            //         binary_search(dst, left, right, storage, cacheable, addr_fifo, data_fifo, pos_info[i].idx, pos_info[i].val);
            //         pos_info[i].exists = (pos_info[i].val == dst);
            //     }

            //     if (cacheable) {
            //         memcpy(hbm_ptr + data_start, tmp_storage, pma_len_in_byte);
            //     }
            //     memcpy(hbm_ptr + header_start, &header, sizeof(PMA_HEADER));

            //     // 发送ACK（已有）
            //     ack_back_to_router.write({.data=pma_idx});
            // }
    }
}