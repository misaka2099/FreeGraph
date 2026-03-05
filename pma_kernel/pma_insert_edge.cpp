#include "data_type.h"
#include <cstdio>
#include <cstring>
#include <cxxabi.h>
#include <hls_stream.h>
#include "debug.h" 
#include <ap_utils.h>
#include "etc/autopilot_ssdm_op.h"
#include "hls_burst_maxi.h"
#define MAX_READ_REQUEST 5
#define MAX_WRITE_REQUEST 3
#define MAX_MEMORY_REQUEST 4
// #define MAX_ALLOC 1 
// #define MAX_FREE 1
#define MAX_PMA_IDX_INFO_REQUEST 5
#define MAX_PMA_ADDR_REQUEST 2
#define WRITE_ACK 114514
// #define MAX_PMA_WORKER 2
// #define MAX_PMA_HEADER_REQUEST 3

struct InsertPos {
    int idx;      // 插入索引
    int val;      // 查找结果值
};
struct inserted_seg
{
    int old_vertex_relative_seg_number[16];
    int vertex_number[16];
};
enum memory_access_type
{
    READ,
    WRITE,
    FINISH
};
struct memory_access_command
{
    int addr_in_byte_patten;
    int len_in_byte_patten;
    memory_access_type type;
};


struct INTRA_COMMAND_STREAM
{
    ap_uint<32> data;
    ap_uint<1> last;
};
struct VEC_INFO_ACCESS 
{
    memory_access_type command_type;
    // ap_uint<1> direction;
    int pma_idx;
};
void read_seg(
    // ap_uint<512>* hbm_ptr,
    hls::stream<memory_access_command> &memory_access_stream,
    hls::stream<ap_uint<512>> &data_from_mem,
    int segment_buffer[SEGMENT_SIZE],
    int pma_data_base_addr,
    int seg_number
)
{
    #pragma HLS INLINE
    // ap_uint<512>* segment_buffer_ptr = (ap_uint<512>*)(segment_buffer);
    ap_uint<512> buffer[4];
    // const int seg_addr_in_ap_512_pattern = (seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr) / 64;
    
    // const int seg_addr_in_ap_512_pattern = (seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr) >> 6; 
    memory_access_command command;
    command.addr_in_byte_patten = seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr;
    command.len_in_byte_patten =  SEGMENT_SIZE * sizeof(int);
    command.type = READ;
    // for (int i = 0 ; i < 4 ; i++)
    // {
    //     #pragma HLS PIPELINE II=1 
    //     // segment_buffer_ptr[i] 
    //     // = hbm_ptr[i + (seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr) / 64];
    //     buffer[i] = hbm_ptr[i + seg_addr_in_ap_512_pattern];
    // }
    memory_access_stream.write(command);
    #pragma HLS protocol fixed
    for (int i = 0 ; i < 4 ; i++)
    {
        #pragma HLS PIPELINE II=1
        buffer[i] = data_from_mem.read();
    }
    for (int i = 0; i < 4; i++) {
        #pragma HLS UNROLL
        for (int j = 0; j < 16; j++) {
            #pragma HLS UNROLL
            segment_buffer[i * 16 + j] = buffer[i].range(32 * (j + 1) - 1, 32 * j);
        }
    }    
    // int seg_byte_addr = pma_data_base_addr + seg_number * SEGMENT_SIZE * sizeof(int);
    // 计算段的起始索引（单位：ap_uint<512>块）
    // int seg_idx = seg_byte_addr / 64;
    // 直接memcpy
    // memcpy(segment_buffer, hbm_ptr + seg_byte_addr, SEGMENT_SIZE * sizeof(int));
}
void write_seg(
    // ap_uint<512>* hbm_ptr,
    hls::stream<memory_access_command> &memory_access_stream,
    hls::stream<ap_uint<512>> &data_to_mem,
    hls::stream<ap_uint<512>> &data_from_mem,
    int segment_buffer[SEGMENT_SIZE],
    int pma_data_base_addr,
    int seg_number
)
{
    #pragma HLS INLINE
    ap_uint<512> buffer[4];
    memory_access_command command;
    command.addr_in_byte_patten = seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr;
    command.len_in_byte_patten =  SEGMENT_SIZE * sizeof(int);
    command.type = WRITE;
    memory_access_stream.write(command);
    // ap_uint<512>* segment_buffer_ptr = (ap_uint<512>*)(segment_buffer);
    // ap_uint<512> buffer;
    // const int seg_addr_in_ap_512_pattern = (seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr) >> 6;
    for (int i = 0; i < 4; i++) {
        #pragma HLS UNROLL
        buffer[i] = 0;
        for (int j = 0; j < 16; j++) {
            #pragma HLS UNROLL
            buffer[i].range(32 * (j + 1) - 1, 32 * j) = segment_buffer[i * 16 + j];
        }
    }
    
    for (int i = 0 ; i < 4 ; i++)
    {
        #pragma HLS PIPELINE II=1
        data_to_mem.write(buffer[i]);
    }
    #pragma HLS protocol fixed
    // ap_uint<512> ack = data_from_mem.read();
    // const int seg_addr_in_ap_512_pattern = (seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr) >> 6;
    // for (int i = 0; i < 4; i++) {
    //     #pragma HLS PIPELINE II=1
    //     hbm_ptr[i + seg_addr_in_ap_512_pattern] = buffer[i];
    // }
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
    // int seg_byte_addr = pma_data_base_addr + seg_number * SEGMENT_SIZE * sizeof(int);
    // 计算段的起始索引（单位：ap_uint<512>块）
    // int seg_idx = seg_byte_addr / 64;
    // 直接memcpy
    // memcpy(hbm_ptr + seg_byte_addr, segment_buffer, SEGMENT_SIZE * sizeof(int));
}
void read_header(
    // ap_uint<512>* hbm_ptr,
    
    hls::stream<memory_access_command> &memory_access_stream,
    hls::stream<ap_uint<512>> &data_from_mem,
    PMA_HEADER& pma_header,
    int pma_header_addr_in_byte
)
{
    #pragma HLS INLINE
    // const int pma_header_addr_in_512_patten = pma_header_addr_in_byte >> 6;
    ap_uint<512> buffer[2];
    // for (int i = 0; i < 4; i++) {
    //     #pragma HLS PIPELINE II=1
    //     buffer[i] = hbm_ptr[pma_header_addr_in_512_patten + i];
    // }
    memory_access_command command;
    command.addr_in_byte_patten = pma_header_addr_in_byte;
    command.len_in_byte_patten =  128;
    command.type = READ;
    memory_access_stream.write(command);
    #pragma HLS protocol fixed
    for (int i = 0 ; i < 2 ; i++)
    {
        #pragma HLS PIPELINE II=1
        buffer[i] = data_from_mem.read();
    }

    for (int j = 0; j < 16; j++) 
    {
        #pragma HLS UNROLL
        int val = buffer[0].range(32 * (j + 1) - 1, 32 * j);
        pma_header.vertex_range[j] = val;
    }
    for (int j = 0 ; j < 16 ; j++)
    {
        #pragma HLS UNROLL 
        int val = buffer[1].range(32 * (j + 1) - 1, 32 * j);
        pma_header.edge_count[j] = val;
    }

    // attr is not read 
}
void write_header(
    // ap_uint<512>* hbm_ptr,
    hls::stream<memory_access_command> &memory_access_stream,
    hls::stream<ap_uint<512>> &data_to_mem,
    hls::stream<ap_uint<512>> &data_from_mem,
    PMA_HEADER& pma_header,
    int pma_header_addr_in_byte
)
{
    #pragma HLS INLINE
    ap_uint<512> buffer[4];
    memory_access_command command;
    command.addr_in_byte_patten = pma_header_addr_in_byte;
    command.len_in_byte_patten =  sizeof(PMA_HEADER);
    command.type = WRITE;
    memory_access_stream.write(command);
    // 打包 vertex_range 到 buffer[0]
    for (int j = 0; j < 16; j++) {
        #pragma HLS UNROLL
        buffer[0].range(32 * (j + 1) - 1, 32 * j) = pma_header.vertex_range[j];
    }
    // 打包 edge_count 到 buffer[1]
    for (int j = 0; j < 16; j++) {
        #pragma HLS UNROLL
        buffer[1].range(32 * (j + 1) - 1, 32 * j) = pma_header.edge_count[j];
    }
    // 打包 attr 到 buffer[2] 和 buffer[3]
    for (int j = 0; j < 8; j++) {
        #pragma HLS UNROLL
        ap_uint<64> d = *reinterpret_cast<ap_uint<64>*>(&pma_header.attr[j]);
        buffer[2].range(64 * (j + 1) - 1, 64 * j) = d;
        d = *reinterpret_cast<ap_uint<64>*>(&pma_header.attr[j + 8]);
        buffer[3].range(64 * (j + 1) - 1, 64 * j) = d;
    }
    const int pma_header_addr_in_512_patten = pma_header_addr_in_byte >> 6;
    // for (int i = 0; i < 4; i++) {
    //     #pragma HLS PIPELINE II=1
    //     hbm_ptr[pma_header_addr_in_512_patten + i] = buffer[i];
    // }
    for (int i = 0 ; i < 4 ; i++)
    {
        data_to_mem.write(buffer[i]);
    }
    #pragma HLS protocol fixed
    // ap_uint<512> ack = data_from_mem.read();
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
            // if (cmd.data == PMA_STOP && cmd.last == 1) 
            // {
            //     pma_idx_info info;
            //     info.last = 1;
            //     for (int i = 0 ; i < MAX_PMA_IDX_INFO_REQUEST ; i++)
            //     {
            //         pma_info_request[i].write(info);
            //     }
            //     return;
            // }
            if (cmd.data == PMA_STOP) 
            {
                if (cmd.last == 1)
                {
                    pma_idx_info info;
                    info.last = 1;
                    for (int i = 0 ; i < MAX_PMA_IDX_INFO_REQUEST ; i++)
                    {
                        pma_info_request[i].write(info);
                    }
                    return;
                }
                continue;
            }
            cmd_batch[i] = cmd.data;
            cmd_count++;
            if (cmd.last == 1) {
                break;
            }
        }
        pma_idx_info info;
        // info.direction = cmd_batch[0].range(98,98);
        info.pma_idx = cmd_batch[0].range(31,0) / VERTEX_PER_PMA;
        info.last = 0;
        
        for (int i = 0 ; i < MAX_PMA_IDX_INFO_REQUEST ; i++)
        {
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
    hls::stream<ap_uint<128>>& edge_info_stream,
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
        ap_uint<128> edge_info[MAX_CMD_NUM];
        #pragma HLS ARRAY_PARTITION variable=edge_info complete

        // 收集边信息
        for (int i = 0; i < cmd_count; i++) {
            #pragma HLS PIPELINE II=1
            auto cmd = cmd_batch_stream.read();
            // if (cmd == PMA_STOP)
            // {
            //     return;
            // }
            // edge_info format:
            // 127-96: src
            // 95-64: dst
            // 63-32: opcode (extended)
            // 31-0: reserved/padding
            edge_info[i] = 0;
            edge_info[i].range(127,96) = cmd.range(31,0);  // src
            edge_info[i].range(95,64)  = cmd.range(63,32); // dst
            edge_info[i].range(63,32)  = cmd.range(127,96); // Include opcode region
        }
        // for (int i = cmd_count ; i < MAX_CMD_NUM ; i++)
        // {
        //     edge_info[i] = 0xFFFFFFFFFFFFFFFF;
        // }
        SORT_EDGES:
        // 使用排序网络对 edge_info 排序
        // for (int stage = 0; stage < 4; stage++) { // log2(16) = 4
        //     for (int step = 0; step < 16; step++) {
        //         #pragma HLS UNROLL
        //         int idx1 = step;
        //         int idx2 = step ^ (1 << stage);
        //         if (idx2 > idx1) {
        //             if (edge_info[idx1] > edge_info[idx2]) {
        //                 ap_uint<64> temp = edge_info[idx1];
        //                 edge_info[idx1] = edge_info[idx2];
        //                 edge_info[idx2] = temp;
        //             }
        //         }
        //     }
        // }
        for (int i = 1; i < cmd_count; i++) {
            ap_uint<128> key = edge_info[i];
            int j = i - 1;
            while (j >= 0 && edge_info[j] > key) {
                edge_info[j + 1] = edge_info[j];
                j--;
            }
            edge_info[j + 1] = key;
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
    // ap_uint<512>* hbm_ptr,
    hls::stream<memory_access_command>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_to_mem,
    hls::stream<ap_uint<512>>& data_from_mem,
    // VEC_INFO* pos_pma_addrs,
    // VEC_INFO* neg_pma_addrs,
    hls::stream<VEC_INFO_ACCESS> &vec_info_access_command,
    hls::stream<VEC_INFO> &vec_info_from_accesser,

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
            memory_access_command x;
            x.addr_in_byte_patten = 0;
            x.len_in_byte_patten = 0;
            x.type = FINISH;
            memory_access_stream.write(x);
            data_to_mem.write(0);

            
            vec_info_access_command.write({.command_type=FINISH});
            break;
        }
        int pma_idx = info.pma_idx;
        // int direction = OUT_DIRECTION;
        VEC_INFO vec;
        VEC_INFO_ACCESS access_command;
        access_command.command_type = READ;
        // access_command.direction = OUT_DIRECTION;
        access_command.pma_idx = pma_idx;
        vec_info_access_command.write(access_command);
        #pragma HLS protocol fixed
        ap_wait();
        vec = vec_info_from_accesser.read();
        // if (direction == IN_DIRECTION)
        // {
        //     // vec = neg_pma_addrs[pma_idx];
        //     VEC_INFO_ACCESS access_command;
        //     access_command.command_type = READ;
        //     access_command.direction = IN_DIRECTION;
        //     vec_info_access_command.write(access_command);
        //     #pragma HLS protocol fixed
        //     vec = vec_info_from_accesser.read();
        // }
        // else
        // {
        //     // vec = pos_pma_addrs[pma_idx];
        //     VEC_INFO_ACCESS access_command;
        //     access_command.command_type = READ;
        //     access_command.direction = OUT_DIRECTION;
        //     vec_info_access_command.write(access_command);
        //     #pragma HLS protocol fixed
        //     vec = vec_info_from_accesser.read();
        // }
        
        PMA_HEADER header;
        int bias = int(vec.range(31,0));
        // memcpy(&header, hbm_ptr + bias, sizeof(PMA_HEADER));
        read_header(memory_access_stream, data_from_mem, header, bias);
        
        for (int i = 0 ; i < MAX_PMA_ADDR_REQUEST ; i++)
        {
            pma_addr[i].write(vec);
        }
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
void binary_search_and_insert_cu(
    // ap_uint<512>* hbm_ptr,
    hls::stream<memory_access_command>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_to_mem,
    hls::stream<ap_uint<512>>& data_from_mem,
    
    hls::stream<PMA_HEADER> &pma_header_in,
    hls::stream<PMA_HEADER> &pma_header_out,
    hls::stream<VEC_INFO> &pma_addr,
    hls::stream<ap_uint<128>> &edge_info_stream,
    hls::stream<int> &edge_count,
    hls::stream<pma_idx_info> &pma_idx,
    hls::stream<inserted_seg>& inserted_pos,
    hls::stream<MEMORY_ALLOC_FREE_REQUEST_INTERNAL>& alloc_request_stream
)
{
    printf("BINARY_SEARCH_AND_INSERT\n");
    while (true)
    {
        auto signal = pma_idx.read();
        if (signal.last == 1)
        {
            memory_access_command x;
            x.addr_in_byte_patten = 0;
            x.len_in_byte_patten = 0;
            x.type = FINISH;
            memory_access_stream.write(x);
            data_to_mem.write(0);
            MEMORY_ALLOC_FREE_REQUEST_INTERNAL finish_req = 0;
            finish_req.range(67,64) = REQ_TYPE_FINISH;
            alloc_request_stream.write(finish_req);
            break;
        }
        inserted_seg inserted_seg_info;
        for (int i = 0 ; i < 16 ; i++)
        {
            inserted_seg_info.old_vertex_relative_seg_number[i] = -1;
            inserted_seg_info.vertex_number[i] = -1;
        }
        auto header = pma_header_in.read();
        auto addr = pma_addr.read();
        int count = edge_count.read();
        int pma_data_base_addr = addr.range(31,0) + sizeof(PMA_HEADER);
        int find_count = 0;

        // Cache mechanism
        int current_cached_id = -1;
        bool is_dirty = false;
        int last_src = -1;
        int last_found_seg = -1;

        int storage[SEGMENT_SIZE];
        #pragma HLS ARRAY_PARTITION variable=storage complete

        for (int i = 0 ; i < SEGMENT_SIZE ; i++) storage[i] = -1;

        for (int i = 0 ; i < count ; i++)
        {
            auto edge_info = edge_info_stream.read();
            int src = edge_info.range(127,96);
            int dst = edge_info.range(95,64);
            int opcode = edge_info.range(63,32);
            int local_src = src % VERTEX_PER_PMA;

            int low_seg = local_src == 0 ? 0 : header.vertex_range[local_src - 1];
            int high_seg = header.vertex_range[local_src] - 1;

            if (src == last_src && last_found_seg != -1 && last_found_seg >= low_seg && last_found_seg <= high_seg) {
                low_seg = last_found_seg;
            }
            last_src = src;

            int out_idx = low_seg * SEGMENT_SIZE - 1;
            int out_val = -1;
            bool find = false;
            int found_idx_for_del = -1; 
            int found_seg_for_del = -1;

            if (opcode == PMA_DELETE_VERTEX)
            {
                if (current_cached_id != -1 && is_dirty) {
                   write_seg(memory_access_stream, data_to_mem, data_from_mem, storage, pma_data_base_addr, current_cached_id);
                   is_dirty = false;
                   current_cached_id = -1;  // Invalidate cache after flushing
                }
                
                // Prepare empty buffer
                for(int k=0; k<SEGMENT_SIZE; k++) storage[k] = UNUSED_ICON;

                // Iterate through segments
                for (int s = low_seg; s <= high_seg; s++)
                {
                    // Write to mem
                    write_seg(memory_access_stream, data_to_mem, data_from_mem, storage, pma_data_base_addr, s);
                }
                
                // Update header state
                header.edge_count[local_src] = 0;
                last_found_seg = -1;
                
                // Since we just cleared everything, logic:
                // If we accessed any segment, storage now holds UNUSED_ICONs which matches that segment.
                // We can set current_cached_id to the last segment we wrote to efficiently resume if needed,
                // BUT only if we actually wrote something (high_seg >= low_seg).
                if (high_seg >= low_seg) {
                   current_cached_id = high_seg;
                   // storage already has UNUSED_ICONs, so it matches high_seg content
                } else {
                   // No segments exist for this vertex possibly?
                   current_cached_id = -1;
                }
                is_dirty = false;
                
                continue;
            }

            BINARY_SEARCH:
            while (low_seg <= high_seg)
            {
                int mid = low_seg + (high_seg - low_seg) / 2;
                
                if (current_cached_id != mid) {
                    if (is_dirty) {
                        write_seg(memory_access_stream, data_to_mem, data_from_mem, storage, pma_data_base_addr, current_cached_id);
                        is_dirty = false;
                    }
                    read_seg(memory_access_stream, data_from_mem, storage, pma_data_base_addr, mid);
                    current_cached_id = mid;
                }

                int minimum = INT_MAX;
                int maximum = INT_MIN;
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
                            found_idx_for_del = j;
                            found_seg_for_del = mid;
                            break;
                            // already exists
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
            
            if (opcode == PMA_DELETE_EDGE)
            {
                if (found_seg_for_del != -1)
                {
                     if (current_cached_id != found_seg_for_del) {
                        if (is_dirty) {
                            write_seg(memory_access_stream, data_to_mem, data_from_mem, storage, pma_data_base_addr, current_cached_id);
                            is_dirty = false;
                        }
                        read_seg(memory_access_stream, data_from_mem, storage, pma_data_base_addr, found_seg_for_del);
                        current_cached_id = found_seg_for_del;
                     }

                     int offset = found_idx_for_del;
                     for (int k = offset; k < SEGMENT_SIZE - 1; k++)
                     {
                         #pragma HLS UNROLL
                         storage[k] = storage[k+1];
                     }
                     storage[SEGMENT_SIZE - 1] = UNUSED_ICON;
                     is_dirty = true;

                     header.edge_count[local_src]--;
                     last_found_seg = found_seg_for_del;
                }
            }
            else if (out_val != dst)
            {
                int target_idx = out_idx + 1;
                int target_seg = target_idx / SEGMENT_SIZE;
                int offset = target_idx % SEGMENT_SIZE;
                
                if (current_cached_id != target_seg) {
                    if (is_dirty) {
                        write_seg(memory_access_stream, data_to_mem, data_from_mem, storage, pma_data_base_addr, current_cached_id);
                        is_dirty = false;
                    }
                    read_seg(memory_access_stream, data_from_mem, storage, pma_data_base_addr, target_seg);
                    current_cached_id = target_seg;
                }
                
                // Shift elements to make space
                for (int k = SEGMENT_SIZE - 1; k > offset; k--)
                {
                    #pragma HLS UNROLL
                    storage[k] = storage[k-1];
                }
                storage[offset] = dst;
                
                is_dirty = true;
                // Write back immediately removed for write combining
                // write_seg(memory_access_stream, data_to_mem, data_from_mem, storage, pma_data_base_addr, target_seg);
                
                int seg_start = local_src == 0 ? 0 : header.vertex_range[local_src-1];
                inserted_seg_info.old_vertex_relative_seg_number[find_count] = target_seg - seg_start;
                inserted_seg_info.vertex_number[find_count] = local_src;
                find_count++;
                
                header.edge_count[local_src]++;
                last_found_seg = target_seg;
            } else {
                last_found_seg = current_cached_id;
            }
        }
        if (is_dirty && current_cached_id != -1) {
            write_seg(memory_access_stream, data_to_mem, data_from_mem, storage, pma_data_base_addr, current_cached_id);
            is_dirty = false;
        }
        int check_enlarge_segs[16];
        #pragma HLS ARRAY_PARTITION variable=check_enlarge_segs complete dim=1
        int check_total_enlarge = 0;
        for (int v = 0 ; v < 16 ; v++) { 
            #pragma HLS UNROLL
            check_enlarge_segs[v] = 0; 
        }
        for (int v = 0 ; v < VERTEX_PER_PMA ; v++)
        {
            #pragma HLS PIPELINE II=1
            int seg_start = v == 0 ? 0 : header.vertex_range[v-1];
            int seg_end = header.vertex_range[v];
            int max_edge = (seg_end - seg_start) * SEGMENT_SIZE * DENSITY_BOUND;
            if (max_edge < header.edge_count[v])
            {
                check_enlarge_segs[v] = 3 * (seg_end - seg_start);
            }
        }
        for(int v=0; v<VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            check_total_enlarge += check_enlarge_segs[v];
        }

        if (check_total_enlarge > 0)
        {
            MEMORY_ALLOC_FREE_REQUEST_INTERNAL alloc_req = 0;
            alloc_req.range(67,64) = REQ_TYPE_ALLOC;
            alloc_req.range(64,64) = 0; // Alloc
            int current_size = addr.range(63,32);
            int new_size = current_size + check_total_enlarge * SEGMENT_SIZE * 4; 
            alloc_req.range(63,32) = new_size;
            alloc_req.range(31,0) = 0; 
            alloc_request_stream.write(alloc_req);
        }

        pma_header_out.write(header);
        inserted_pos.write(inserted_seg_info);
    }
}



int valid_count(
    int seg_start, int seg_end, 
    hls::stream<memory_access_command>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_from_mem, 
    int pma_data_base_addr
)
{
    #pragma HLS INLINE
    int total_segs = seg_end - seg_start;
    if (total_segs <= 0) return 0;

    memory_access_command command;
    command.addr_in_byte_patten = seg_start * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr;
    command.len_in_byte_patten =  total_segs * SEGMENT_SIZE * sizeof(int);
    command.type = READ;
    memory_access_stream.write(command);

    int count = 0;
    // Each segment has 4 * 512-bit words (assuming SEGMENT_SIZE=64)
    int total_words = total_segs * 4; 
    
    for (int i = 0 ; i < total_words ; i++)
    {
        #pragma HLS PIPELINE II=1
        ap_uint<512> val = data_from_mem.read();
        for (int j = 0 ; j < 16 ; j++)
        {
            #pragma HLS UNROLL
            int item = val.range(32 * (j + 1) - 1, 32 * j);
            if (item != UNUSED_ICON) count++;
        }
    }
    return count;
}


void redistribute(
    int read_seg_start, int read_seg_end,
    int write_seg_start, int write_seg_end,
    hls::stream<memory_access_command>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_to_mem,
    hls::stream<ap_uint<512>>& data_from_mem,
    int read_pma_data_base_addr, 
    int write_pma_data_base_addr, 
    int total_valid_count)
{
    int total_segs = write_seg_end - write_seg_start;
    int base_per_seg = total_valid_count / total_segs;
    int extra = total_valid_count % total_segs;
    
    int current_write_fill = 0;
    int read_seg_idx = read_seg_start;
    int write_seg_idx = write_seg_start;
    
    int read_buffer[SEGMENT_SIZE];
    int write_buffer[SEGMENT_SIZE];
    #pragma HLS BIND_STORAGE variable=read_buffer type=RAM_2P impl=BRAM
    #pragma HLS BIND_STORAGE variable=write_buffer type=RAM_2P impl=BRAM
    
    int read_ptr = SEGMENT_SIZE; // Force read first
    
    // Initialize write buffer
    for(int k=0; k<SEGMENT_SIZE; k++) write_buffer[k] = -1;

    while(write_seg_idx < write_seg_end) {
        int target_fill = base_per_seg + ((write_seg_idx - write_seg_start) < extra ? 1 : 0);
        
        while(current_write_fill < target_fill) {
            if (read_ptr == SEGMENT_SIZE) {
                    if (read_seg_idx < read_seg_end) {
                        read_seg(memory_access_stream, data_from_mem, read_buffer, read_pma_data_base_addr, read_seg_idx++);
                        read_ptr = 0;
                    } else {
                        break; 
                    }
            }
            
            if (read_buffer[read_ptr] != UNUSED_ICON) {
                write_buffer[current_write_fill++] = read_buffer[read_ptr];
            }
            read_ptr++;
        }
        
        write_seg(memory_access_stream, data_to_mem, data_from_mem, write_buffer, write_pma_data_base_addr, write_seg_idx++);
        
        // Reset
        current_write_fill = 0;
        for(int k=0; k<SEGMENT_SIZE; k++) write_buffer[k] = -1;
    }
}

void rebalance_cu(
    // ap_uint<512>* hbm_ptr,
    hls::stream<memory_access_command>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_to_mem,
    hls::stream<ap_uint<512>>& data_from_mem,
    hls::stream<PMA_HEADER>& old_pma_header_in,
    hls::stream<PMA_HEADER>& pma_header_in,
    hls::stream<VEC_INFO>& old_pma_addr_in,
    hls::stream<VEC_INFO>& pma_addr_in,
    hls::stream<pma_idx_info>& pma_idx,
    hls::stream<inserted_seg>& inserted_seg_info,
    hls::stream<COMMAND_STREAM>& ack_back_to_router,
    hls::stream<MEMORY_ALLOC_FREE_REQUEST_INTERNAL>& free_request_stream
)
{
        
    printf("REBALANCE\n");
    while (true)
    {
        auto signal = pma_idx.read();
        if (signal.last == 1)
        {
            memory_access_command x;
            x.addr_in_byte_patten = 0;
            x.len_in_byte_patten = 0;
            x.type = FINISH;
            memory_access_stream.write(x);
            data_to_mem.write(0);
            MEMORY_ALLOC_FREE_REQUEST_INTERNAL finish_req = 0;
            finish_req.range(67,64) = REQ_TYPE_FINISH;
            free_request_stream.write(finish_req);
            // ack_back_to_router.write(pma_idx);
            break;
        }
        PMA_HEADER pma_header = pma_header_in.read();
        PMA_HEADER old_pma_header = old_pma_header_in.read();

        VEC_INFO pma_addr = pma_addr_in.read();
        VEC_INFO old_pma_addr = old_pma_addr_in.read();

        auto to_check = inserted_seg_info.read();
        // int seg_count = pma_header.vertex_range[15];

        const int pma_data_base_addr = pma_addr.range(31,0) + sizeof(PMA_HEADER);
        const int old_pma_data_base_addr = old_pma_addr.range(31,0) + sizeof(PMA_HEADER);
        // const int pma_data_size_bytes = pma_addr.range(63,32) - sizeof(PMA_HEADER);

        if (pma_data_base_addr != old_pma_data_base_addr)
        {
            // Enlarged
            for (int v = 0 ; v < VERTEX_PER_PMA ; v++)
            {
                int old_seg_start = (v == 0) ? 0 : old_pma_header.vertex_range[v-1];
                int old_seg_end = old_pma_header.vertex_range[v];
                int new_seg_start = (v == 0) ? 0 : pma_header.vertex_range[v-1];
                int new_seg_end = pma_header.vertex_range[v];

                int total_valid_count = valid_count(old_seg_start, old_seg_end, memory_access_stream, data_from_mem, old_pma_data_base_addr);
                
                redistribute(old_seg_start, old_seg_end, new_seg_start, new_seg_end, 
                    memory_access_stream, data_to_mem, data_from_mem, 
                    old_pma_data_base_addr, pma_data_base_addr, total_valid_count);
            }
            MEMORY_ALLOC_FREE_REQUEST_INTERNAL free_command = 0;
            free_command.range(67,64) = REQ_TYPE_FREE;
            free_command.range(63,0) = old_pma_addr.range(63,0);
            free_command.range(64,64) = 1; // free
            free_request_stream.write(free_command);
        }
        else
        {
            for (int i = 0 ; i < 16 ; i++)
            {
                if (to_check.vertex_number[i] == -1) continue;

                int v = to_check.vertex_number[i];
                int relative_seg_num = to_check.old_vertex_relative_seg_number[i];

                int seg_start = (v == 0) ? 0 : pma_header.vertex_range[v-1];
                int seg_end = pma_header.vertex_range[v];
                int seg_num = seg_end - seg_start; // 当前vertex的segment数
                int total_valid_count = 0;
                int seg_idx = relative_seg_num;
                total_valid_count = valid_count(seg_idx + seg_start, seg_idx + seg_start+ 1, memory_access_stream, data_from_mem, pma_data_base_addr);
                int last_left = seg_idx + seg_start;
                int last_right = seg_idx + seg_start + 1 ;


                int max_edge = SEGMENT_SIZE * DENSITY_BOUND;
                if (total_valid_count < max_edge) continue;
                for (int cur_seg_num = 2 ; cur_seg_num <= seg_num ; cur_seg_num *= 2)
                {
                    int left = seg_start + int(seg_idx / cur_seg_num) * cur_seg_num;
                    int right = seg_start + (int(seg_idx / cur_seg_num) + 1) * cur_seg_num;
                    // int valid = valid_count(left, right);
                    if (left < last_left) {
                        total_valid_count += valid_count(left, last_left, memory_access_stream, data_from_mem, pma_data_base_addr);
                    }
                    else if (right > last_right) {
                        total_valid_count += valid_count(last_right, right, memory_access_stream, data_from_mem, pma_data_base_addr);
                    }

                    max_edge = (right - left) * SEGMENT_SIZE * DENSITY_BOUND; 
                    if (total_valid_count <= max_edge)
                    {
                        redistribute(left, right, left, right, memory_access_stream, data_to_mem, data_from_mem, pma_data_base_addr, pma_data_base_addr, total_valid_count);
                        break;
                    }
                    last_left = left;
                    last_right = right;
                }
            }
        }
        
        ack_back_to_router.write({.data=signal.pma_idx});
    }
}   








void move_and_change_header(
    // ap_uint<512>* hbm_ptr,
    hls::stream<memory_access_command>& memory_access_stream,
    // hls::stream<COMMAND>& cmd_to_mem,
    hls::stream<ap_uint<512>>& data_to_mem,
    hls::stream<ap_uint<512>>& data_from_mem,
    // VEC_INFO* pos_addr,
    // VEC_INFO* neg_addr,
    hls::stream<VEC_INFO_ACCESS> &vec_info_access_command,
    hls::stream<VEC_INFO>& vec_info_to_accesser,
    hls::stream<PMA_HEADER>& header_stream,
    hls::stream<PMA_HEADER>& old_header_stream_out,
    hls::stream<PMA_HEADER>& new_header_stream,

    hls::stream<VEC_INFO>& pma_addr_in,
    hls::stream<VEC_INFO>& old_pma_addr_out,
    hls::stream<VEC_INFO>& pma_addr_out,

    hls::stream<VEC_INFO_STREAM>& new_alloc_addr,

    hls::stream<pma_idx_info>& pma_idx_info_in
)
{
    printf("MOVE_AND_CHANGE_HEADER\n");
    int unused_data[SEGMENT_SIZE];
    for (int i = 0 ; i < 64 ; i++)
    {
        unused_data[i] = UNUSED_ICON;
    }
    while (true)
    {
        auto pma_info = pma_idx_info_in.read();
        
        ap_uint<512> dummy_val;
        bool dummy_res = data_from_mem.read_nb(dummy_val);

        if (pma_info.last == 1)
        {
            HERE
            
            memory_access_command x;
            x.addr_in_byte_patten = 0;
            x.len_in_byte_patten = 0;
            x.type = FINISH;
            memory_access_stream.write(x);
            data_to_mem.write(0);

            // cmd_to_mem.write(MEM_STOP);

            vec_info_access_command.write({.command_type=FINISH});
            break;
        }
        auto header = header_stream.read();
        auto old_header = header;
        int pma_index = pma_info.pma_idx;
        int enlarge_segs[16];
        for (int i = 0 ; i < 16 ; i++)
        {
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
        VEC_INFO old_addr = pma_addr_in.read();
        printf("old addr start %d , len %d\n", int(old_addr.range(31,0)), int(old_addr.range(63,32)));
        VEC_INFO new_addr;
        ap_wait();
        if (enlarged)
        {
            HERE
            auto new_addr_stream = new_alloc_addr.read(); // Read from allocation result (which was requested by binary_search)
            new_addr = new_addr_stream.data;

            VEC_INFO_ACCESS access_command;
            access_command.command_type = WRITE;
            // access_command.direction = direction;
            access_command.pma_idx = pma_index;
            vec_info_access_command.write(access_command);
            vec_info_to_accesser.write(new_addr);
            // Copy logic moved to revalance_cu
        }
        else  
        {
            new_addr = old_addr;
        }
        int bias = (int)new_addr.range(31,0);
        // memcpy(hbm_ptr + bias, &header, sizeof(PMA_HEADER));
        write_header(memory_access_stream, data_to_mem, data_from_mem, header, bias);

        #pragma HLS protocol fixed
        old_pma_addr_out.write(old_addr);
        pma_addr_out.write(new_addr);
        old_header_stream_out.write(old_header);
        new_header_stream.write(header);
        HERE
    }
}

void merge_requests(
    hls::stream<MEMORY_ALLOC_FREE_REQUEST_INTERNAL>& in1, // alloc
    hls::stream<MEMORY_ALLOC_FREE_REQUEST_INTERNAL>& in2, // free
    hls::stream<MEMORY_ALLOC_FREE_REQUEST>& out
)
{
    bool finish1 = false;
    bool finish2 = false;
    while(true)
    {
        #pragma HLS PIPELINE II=1
        MEMORY_ALLOC_FREE_REQUEST_INTERNAL req;
        if (!finish1 && in1.read_nb(req))
        {
            if (req.range(67, 64) == REQ_TYPE_FINISH) {
                finish1 = true;
            } else {
                MEMORY_ALLOC_FREE_REQUEST out_req;
                out_req.data = req;
                out_req.last = 0;
                out.write(out_req);
            }
        }
        else if (!finish2 && in2.read_nb(req))
        {
            if (req.range(67, 64) == REQ_TYPE_FINISH) {
                finish2 = true;
            } else {
                MEMORY_ALLOC_FREE_REQUEST out_req;
                out_req.data = req;
                out_req.last = 0;
                out.write(out_req);
            }
        }

        if (finish1 && finish2) break;
    }
}


void mem_adapter(
    hls::stream<memory_access_command> request[MAX_MEMORY_REQUEST],
    hls::stream<ap_uint<512>> data_to_mem[MAX_MEMORY_REQUEST],
    hls::stream<ap_uint<512>> data_from_mem[MAX_MEMORY_REQUEST],
    hls::stream<VEC_INFO_ACCESS> vec_info_access_from_app[2],
    hls::stream<VEC_INFO>& vec_info_from_app,
    hls::stream<VEC_INFO> &vec_info_to_app,

    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access,
    hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_mem_access,
    hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_mem_access,
    hls::stream<VEC_INFO_STREAM> &vec_info_from_mem_access
)
{
    ap_uint<MAX_MEMORY_REQUEST> finished_bitmap = -1;
    bool vec_info_finished[2] = {false, false};
    int idx = 0;
    int vec_idx = 0;

    while (true)
    {
        #pragma HLS PIPELINE II=1
        bool all_finished = (finished_bitmap == 0) && vec_info_finished[0] && vec_info_finished[1];
        if (all_finished) break;

        // Process memory requests
        if (finished_bitmap != 0)
        {
            memory_access_command request_info;
            if (request[idx].read_nb(request_info))
            {
                if (request_info.type == FINISH)
                {
                    finished_bitmap[idx] = 0;
                    if (!data_to_mem[idx].empty()) {
                        auto dummy = data_to_mem[idx].read();
                    }
                }
                else
                {
                    MEMORY_INFO_ACCESS_COMMAND cmd;
                    cmd.pma_idx = 0;
                    cmd.start = request_info.addr_in_byte_patten;
                    cmd.len = request_info.len_in_byte_patten;
                    cmd.last = 0;

                    int len_in_512 = request_info.len_in_byte_patten >> 6;

                    if (request_info.type == READ)
                    {
                        cmd.command_type = MEMORY_READ;
                        req_to_mem_access.write(cmd);
                        #pragma HLS PROTOCOL fixed
                        ap_wait();
                        for (int i = 0; i < len_in_512; i++)
                        {
                            #pragma HLS PIPELINE II=1
                            ap_axiu<512, 0, 0, 0> packet = data_from_mem_access.read();
                            data_from_mem[idx].write(packet.data);
                        }
                    }
                    else if (request_info.type == WRITE)
                    {
                        cmd.command_type = MEMORY_WRITE;
                        req_to_mem_access.write(cmd);
                        #pragma HLS PROTOCOL fixed
                        ap_wait();
                        for (int i = 0; i < len_in_512; i++)
                        {
                            #pragma HLS PIPELINE II=1
                            ap_uint<512> data = data_to_mem[idx].read();
                            ap_axiu<512, 0, 0, 0> packet;
                            packet.data = data;
                            packet.last = (i == len_in_512 - 1) ? 1 : 0;
                            data_to_mem_access.write(packet);
                        }
                    }
                }
            }
            // Move to next memory request queue
            idx = (idx + 1) % MAX_MEMORY_REQUEST;
        }

        ap_wait();

        // Process vec info requests
        if (!vec_info_finished[vec_idx])
        {
            VEC_INFO_ACCESS vec_cmd;
            if (vec_info_access_from_app[vec_idx].read_nb(vec_cmd))
            {
                if (vec_cmd.command_type == FINISH)
                {
                    vec_info_finished[vec_idx] = true;
                }
                else
                {
                    MEMORY_INFO_ACCESS_COMMAND cmd;
                    cmd.pma_idx = vec_cmd.pma_idx;
                    cmd.last = 0;

                    if (vec_cmd.command_type == READ)
                    {
                        // if (vec_cmd.direction == OUT_DIRECTION)
                        //     cmd.command_type = POS_VEC_INFO_READ;
                        // else
                        //     cmd.command_type = NEG_VEC_INFO_READ;
                        cmd.command_type = VEC_INFO_READ;
                        
                        cmd.start = 0;
                        cmd.len = 0;
                        
                        req_to_mem_access.write(cmd);

                        #pragma HLS protocol fixed
                        ap_wait();
                        VEC_INFO_STREAM vec_pkt = vec_info_from_mem_access.read();
                        #pragma HLS PROTOCOL fixed
                        ap_wait();
                        vec_info_to_app.write(vec_pkt.data);
                    }
                    else // WRITE
                    {
                        VEC_INFO vec_info = vec_info_from_app.read();
                        // if (vec_cmd.direction == OUT_DIRECTION)
                        //     cmd.command_type = POS_VEC_INFO_WRITE;
                        // else
                        //     cmd.command_type = NEG_VEC_INFO_WRITE;
                        cmd.command_type = VEC_INFO_WRITE;
                        
                        cmd.start = vec_info.range(31,0); 
                        cmd.len = vec_info.range(63,32);
                        
                        req_to_mem_access.write(cmd);
                    }
                }
            }
        }
        vec_idx = (vec_idx + 1) % 2;
    }
}

extern "C"
{
    void pma_insert_edge(
        // VEC_INFO* pos_pma_addrs,
        // VEC_INFO* neg_pma_addrs,
        // ap_uint<512>* hbm_ptr,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
        hls::stream<MEMORY_ALLOC_FREE_REQUEST> &cmd_to_mem_stream,
        hls::stream<VEC_INFO_STREAM> &vec_info_back_stream,
        hls::stream<COMMAND_STREAM> &ack_back_to_router,
        
        hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_mem_access,
        hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_mem_access,
        hls::stream<VEC_INFO_STREAM> &vec_info_from_mem_access
    )
    {
        #pragma HLS INTERFACE axis port=cmd_stream
        #pragma HLS INTERFACE axis port=cmd_to_mem_stream
        #pragma HLS INTERFACE axis port=vec_info_back_stream
        #pragma HLS INTERFACE axis port=ack_back_to_router
        
        #pragma HLS INTERFACE axis port=req_to_mem_access
        #pragma HLS INTERFACE axis port=data_to_mem_access
        #pragma HLS INTERFACE axis port=data_from_mem_access
        #pragma HLS INTERFACE axis port=vec_info_from_mem_access
       
        hls::stream<pma_idx_info> pma_idx_info_request[MAX_PMA_IDX_INFO_REQUEST];
        hls::stream<VEC_INFO> pma_addr_request [MAX_PMA_ADDR_REQUEST];
        hls::stream<PMA_HEADER> pma_header_request ;

        hls::stream<int> cmd_count_stream ;
        hls::stream<ap_uint<128>> cmd_batch_stream ;

        hls::stream<ap_uint<128>> edge_info_stream ;
        hls::stream<int> edge_count_stream ;

        hls::stream<PMA_HEADER> header_from_insert ;

        hls::stream<PMA_HEADER> new_pma_header ;
        hls::stream<VEC_INFO> pma_addr_to_rebalance ;
        
        hls::stream<VEC_INFO> old_pma_addr_to_rebalance;
        hls::stream<PMA_HEADER> old_pma_header_to_rebalance;

        hls::stream<inserted_seg> inserted_seg_info ;

        hls::stream<MEMORY_ALLOC_FREE_REQUEST_INTERNAL> alloc_request_stream;
        hls::stream<MEMORY_ALLOC_FREE_REQUEST_INTERNAL> free_request_stream;
        #pragma HLS STREAM variable=alloc_request_stream depth=16
        #pragma HLS STREAM variable=free_request_stream depth=16
        #pragma HLS STREAM variable=old_pma_addr_to_rebalance depth=4
        #pragma HLS STREAM variable=old_pma_header_to_rebalance depth=4

        hls::stream<memory_access_command> memory_access_stream [MAX_MEMORY_REQUEST];
        hls::stream<ap_uint<512>> data_to_mem [MAX_MEMORY_REQUEST];
        hls::stream<ap_uint<512>> data_from_mem [MAX_MEMORY_REQUEST];
        #pragma HLS STREAM variable=memory_access_stream depth=16
        #pragma HLS STREAM variable=data_to_mem depth=16
        #pragma HLS STREAM variable=data_from_mem depth=16
        #pragma HLS STREAM variable=pma_idx_info_request depth=16
        #pragma HLS STREAM variable=pma_addr_request depth=16
        // #pragma HLS STREAM variable=pma_header_request depth=16
        // #pragma HLS STREAM variable=cmd_count_stream depth=16
        #pragma HLS STREAM variable=cmd_batch_stream depth=16
        #pragma HLS STREAM variable=edge_info_stream depth=16
        // #pragma HLS STREAM variable=edge_count_stream depth=16
        // #pragma HLS STREAM variable=header_from_bs depth=16
        // #pragma HLS STREAM variable=header_from_insert depth=16

        // #pragma HLS STREAM variable=insert_count_stream depth=16
        // #pragma HLS STREAM variable=new_pma_header depth=16
        // #pragma HLS STREAM variable=pma_addr_to_rebalance depth=16
        // hls::stream<INTRA_PMA_UPDATE_MESSAGE> cmd_from_dispatcher ;

        // hls::stream<VEC_INFO> alloc_free_vec_info_from_app ;
        // hls::stream<VEC_INFO> alloc_free_vec_info_to_app ;
        // hls::stream<COMMAND> alloc_free_cmd_from_app ;

        // hls::stream<INTRA_COMMAND_STREAM> ack_from_app ;

        hls::stream<VEC_INFO_ACCESS> vec_info_access_from_app [2];
        hls::stream<VEC_INFO> vec_info_from_app ;
        hls::stream<VEC_INFO> vec_info_to_app;
        #pragma HLS DATAFLOW
        
        
        mem_adapter(memory_access_stream, data_to_mem, data_from_mem, 
            vec_info_access_from_app, vec_info_from_app, vec_info_to_app,
            req_to_mem_access, data_to_mem_access, data_from_mem_access, vec_info_from_mem_access
            );
        // hbm_access_unit(memory_access_stream, data_to_mem, data_from_mem, hbm_ptr);

        merge_requests(alloc_request_stream, free_request_stream, cmd_to_mem_stream);

        // dispatch(cmd_stream, cmd_from_dispatcher);

        // alloc_free_sender(alloc_free_cmd_from_app, alloc_free_vec_info_from_app, alloc_free_vec_info_to_app,
        //     cmd_to_mem_stream, vec_info_to_mem_stream, vec_info_back_stream);

        // ack_backer(ack_back_to_router, ack_from_app);

        // vec_info_accesser(vec_info_access_from_app, vec_info_from_app, vec_info_to_app, pos_pma_addrs, neg_pma_addrs);
        
        process_cmd(
            cmd_stream,
            cmd_count_stream,
            cmd_batch_stream,
            pma_idx_info_request
        );
        // process_cmd(
        //     cmd_from_dispatcher[1],
        //     cmd_count_stream[1],
        //     cmd_batch_stream[1],
        //     pma_idx_info_request[1]
        // );
        
        sort_cu(
            cmd_count_stream,
            cmd_batch_stream,
            edge_info_stream,
            edge_count_stream,
            pma_idx_info_request[4]
        );
        // sort_cu(
        //     cmd_count_stream[1],
        //     cmd_batch_stream[1],
        //     edge_info_stream[1],
        //     edge_count_stream[1],
        //     pma_idx_info_request[1][5]
        // );
        
        head_read(
            // hbm_ptr_0,
            memory_access_stream[0],
            data_to_mem[0],
            data_from_mem[0],
            vec_info_access_from_app[0],
            vec_info_to_app, 
            pma_idx_info_request[0],
            pma_header_request,
            pma_addr_request
        );
        // head_read(
        //     // hbm_ptr_0,
        //     memory_access_stream[1][0],
        //     data_to_mem[1][0],
        //     data_from_mem[1][0],
        //     vec_info_access_from_app[1][0],
        //     vec_info_to_app[1],
        //     pma_idx_info_request[1][0],
        //     pma_header_request[1],
        //     pma_addr_request[1]
        // );

        binary_search_and_insert_cu(
            memory_access_stream[1],
            data_to_mem[1],
            data_from_mem[1],
            pma_header_request,
            header_from_insert,
            pma_addr_request[0],
            edge_info_stream,
            edge_count_stream,
            pma_idx_info_request[3],
            inserted_seg_info,
            alloc_request_stream
        );


        move_and_change_header(
            // hbm_ptr_3,
            
            memory_access_stream[2],
            data_to_mem[2],
            data_from_mem[2],
            vec_info_access_from_app[1],

            vec_info_from_app,
            header_from_insert,
            old_pma_header_to_rebalance,
            new_pma_header, 

            pma_addr_request[1],
            old_pma_addr_to_rebalance,
            pma_addr_to_rebalance, 

            // vec_info_back_stream,
            // vec_info_to_mem_stream,
            // cmd_to_mem_stream,
            vec_info_back_stream,
            pma_idx_info_request[1]
        );
        // move_and_change_header(
        // //     // hbm_ptr_3,
            
        //     memory_access_stream[1][3],
        //     data_to_mem[1][3],
        //     data_from_mem[1][3],
        //     vec_info_access_from_app[1][1],
        //     vec_info_from_app[1],
        //     header_from_insert[1],
        //     new_pma_header[1], 

        //     pma_addr_request[1][2],
        //     pma_addr_to_rebalance[1], 

        //     // vec_info_back_stream,
        //     // vec_info_to_mem_stream,
        //     // cmd_to_mem_stream,
        //     alloc_free_vec_info_to_app[1],
        //     alloc_free_vec_info_from_app[1],
        //     alloc_free_cmd_from_app[1],
        //     pma_idx_info_request[1][1]
        // );
        
        rebalance_cu(
            // hbm_ptr_4,
            
            memory_access_stream[3],
            data_to_mem[3],
            data_from_mem[3],
            old_pma_header_to_rebalance,
            new_pma_header,
            old_pma_addr_to_rebalance,
            pma_addr_to_rebalance,
            pma_idx_info_request[2],
            inserted_seg_info,
            ack_back_to_router,
            free_request_stream
        );

        // rebalance_cu(
        //     // hbm_ptr_4,
            
        //     memory_access_stream[1][4],
        //     data_to_mem[1][4],
        //     data_from_mem[1][4],
        //     new_pma_header[1],
        //     pma_addr_to_rebalance[1],
        //     pma_idx_info_request[1][2],
        //     inserted_seg_info[1],
        //     ack_from_app[1]
        // );
    }
}