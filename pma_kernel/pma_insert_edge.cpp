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
#define MAX_READ_REQUEST 5
#define MAX_WRITE_REQUEST 3
#define MAX_MEMORY_REQUEST 4
#define MAX_PMA_IDX_INFO_REQUEST 5
#define MAX_PMA_ADDR_REQUEST 2


#define MAX_PIPELINE_NUM 4
struct InsertPos {
    int idx;      // 插入索引
    int val;      // 查找结果值
};
struct inserted_seg
{
    int old_vertex_relative_seg_number[16];
    int vertex_number[16];
    ap_uint<1> has_insert;
};

struct INTRA_COMMAND_STREAM
{
    ap_uint<32> data;
    ap_uint<1> last;
};

void distributor(
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_stream,    
    hls::stream<int> cmd_count_stream[MAX_PIPELINE_NUM],
    hls::stream<ap_uint<128>> cmd_batch_stream[MAX_PIPELINE_NUM],
    hls::stream<pma_idx_info> pma_info_request[MAX_PIPELINE_NUM][MAX_PMA_IDX_INFO_REQUEST]
)
{
    while (true)
    {
        if (cmd_stream.empty())
        {
            ap_wait();
            continue;
        }

        ap_uint<128> cmd_batch[MAX_CMD_NUM];
        #pragma HLS ARRAY_PARTITION variable=cmd_batch complete
        int cmd_count = 0;
        bool is_stop = false;
        bool last_flag = false;

        for (int i = 0; i < MAX_CMD_NUM; i++)
        {
            #pragma HLS PIPELINE II=1
            auto cmd = cmd_stream.read();
            if (cmd.data == PMA_STOP)
            {
                if (cmd.last == 1)
                {
                    is_stop = true;
                    last_flag = true;
                    break;
                }
                continue;
            }
            cmd_batch[cmd_count] = cmd.data;
            cmd_count++;
            if (cmd.last == 1)
            {
                last_flag = true;
                break;
            }
        }

        if (is_stop)
        {
            pma_idx_info stop_info;
            stop_info.last = 1;
            stop_info.pma_idx = 0;
            for (int lane = 0; lane < MAX_PIPELINE_NUM; lane++)
            {
                #pragma HLS UNROLL
                for (int i = 0; i < MAX_PMA_IDX_INFO_REQUEST; i++)
                {
                    #pragma HLS UNROLL
                    pma_info_request[lane][i].write(stop_info);
                }
            }
            break;
        }

        int pma_idx = cmd_batch[0].range(31,0) / VERTEX_PER_PMA;
        int lane = pma_idx % MAX_PIPELINE_NUM;

        pma_idx_info info;
        info.pma_idx = pma_idx;
        info.last = 0;
        for (int i = 0; i < MAX_PMA_IDX_INFO_REQUEST; i++)
        {
            #pragma HLS UNROLL
            pma_info_request[lane][i].write(info);
        }

        cmd_count_stream[lane].write(cmd_count);
        for (int i = 0; i < cmd_count; i++)
        {
            #pragma HLS PIPELINE II=1
            cmd_batch_stream[lane].write(cmd_batch[i]);
        }

        if (last_flag)
        {
            // 这里不额外处理；终止由显式 PMA_STOP 驱动
        }
    }
}
void ack_merger(
    hls::stream<INTRA_COMMAND_STREAM>& ack_from_lane_0,
    hls::stream<INTRA_COMMAND_STREAM>& ack_from_lane_1,
    hls::stream<INTRA_COMMAND_STREAM>& ack_from_lane_2,
    hls::stream<INTRA_COMMAND_STREAM>& ack_from_lane_3,
    hls::stream<COMMAND_STREAM>& ack_back_to_router
)
{
    bool done_lane[4] = {false, false, false, false};
    while (true)
    {
        #pragma HLS PIPELINE II=1
        COMMAND_STREAM ack;
        INTRA_COMMAND_STREAM lane_ack;
        if (!done_lane[0] && ack_from_lane_0.read_nb(lane_ack))
        {
            if (lane_ack.last == 1)
                done_lane[0] = true;
            else
            {
                ack.data = lane_ack.data;
                ack.keep = 0;
                ack.strb = 0;
                ack.user = 0;
                ack.id = 0;
                ack.dest = 0;
                ack.last = 0;
                ack_back_to_router.write(ack);
            }
        }
        if (!done_lane[1] && ack_from_lane_1.read_nb(lane_ack))
        {
            if (lane_ack.last == 1)
                done_lane[1] = true;
            else
            {
                ack.data = lane_ack.data;
                ack.keep = 0;
                ack.strb = 0;
                ack.user = 0;
                ack.id = 0;
                ack.dest = 0;
                ack.last = 0;
                ack_back_to_router.write(ack);
            }
        }
        if (!done_lane[2] && ack_from_lane_2.read_nb(lane_ack))
        {
            if (lane_ack.last == 1)
                done_lane[2] = true;
            else
            {
                ack.data = lane_ack.data;
                ack.keep = 0;
                ack.strb = 0;
                ack.user = 0;
                ack.id = 0;
                ack.dest = 0;
                ack.last = 0;
                ack_back_to_router.write(ack);
            }
        }
        if (!done_lane[3] && ack_from_lane_3.read_nb(lane_ack))
        {
            if (lane_ack.last == 1)
                done_lane[3] = true;
            else
            {
                ack.data = lane_ack.data;
                ack.keep = 0;
                ack.strb = 0;
                ack.user = 0;
                ack.id = 0;
                ack.dest = 0;
                ack.last = 0;
                ack_back_to_router.write(ack);
            }
        }

        if (done_lane[0] && done_lane[1] && done_lane[2] && done_lane[3])
            break;
    }
}
static inline int density_capacity_from_seg_count(int seg_count)
{
    return (seg_count * SEGMENT_SIZE * 3) >> 2;
}

void read_seg(
    // ap_uint<512>* hbm_ptr,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &memory_access_stream,
    hls::stream<ap_uint<512>> &data_from_mem,
    int segment_buffer[SEGMENT_SIZE],
    int pma_data_base_addr,
    int seg_number
)
{
    #pragma HLS INLINE
    ap_uint<512> buffer[4];
    MEMORY_INFO_ACCESS_COMMAND command = {};
    
    command.start = seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr;;
    command.len =  SEGMENT_SIZE * sizeof(int);
    command.command_type = MEMORY_READ;
    command.last = 0;
    memory_access_stream.write(command);
    #pragma HLS protocol fixed
    for (int i = 0 ; i < 4 ; i++)
    {
        #pragma HLS PIPELINE II=1
        buffer[i] = data_from_mem.read();
    }
    for (int i = 0; i < 4; i++) {
        // #pragma HLS UNROLL
        for (int j = 0; j < 16; j++) {
            #pragma HLS UNROLL
            segment_buffer[i * 16 + j] = buffer[i].range(32 * (j + 1) - 1, 32 * j);
        }
    }
}
void write_seg(
    // ap_uint<512>* hbm_ptr,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &memory_access_stream,
    hls::stream<ap_uint<512>> &data_to_mem,
    hls::stream<ap_uint<512>> &data_from_mem,
    int segment_buffer[SEGMENT_SIZE],
    int pma_data_base_addr,
    int seg_number
)
{
    #pragma HLS INLINE
    ap_uint<512> buffer[4];
    MEMORY_INFO_ACCESS_COMMAND command = {};
    command.start = seg_number * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr;;
    command.len = SEGMENT_SIZE * sizeof(int);
    command.command_type = MEMORY_WRITE;
    command.last = 0;
    memory_access_stream.write(command);
    for (int i = 0; i < 4; i++) {
        // #pragma HLS UNROLL
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
}
void read_header(
    // ap_uint<512>* hbm_ptr,
    
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &memory_access_stream,
    hls::stream<ap_uint<512>> &data_from_mem,
    PMA_HEADER& pma_header,
    int pma_header_addr_in_byte
)
{
    #pragma HLS INLINE
    ap_uint<512> buffer[4];
    MEMORY_INFO_ACCESS_COMMAND command = {};
    command.start = pma_header_addr_in_byte;
    command.len = 256;
    command.command_type = MEMORY_READ;
    command.last = 0;
    memory_access_stream.write(command);
    #pragma HLS protocol fixed
    for (int i = 0 ; i < 4 ; i++)
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
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &memory_access_stream,
    hls::stream<ap_uint<512>> &data_to_mem,
    hls::stream<ap_uint<512>> &data_from_mem,
    PMA_HEADER& pma_header,
    int pma_header_addr_in_byte
)
{
    #pragma HLS INLINE
    ap_uint<512> buffer[4];
    MEMORY_INFO_ACCESS_COMMAND command = {};
    // command.src = pma_idx * VERTEX_PER_PMA;
    command.start = pma_header_addr_in_byte;
    command.len = sizeof(PMA_HEADER);
    command.command_type = MEMORY_WRITE;
    command.last = 0;
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
    for (int i = 0 ; i < 4 ; i++)
    {
        data_to_mem.write(buffer[i]);
    }
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
        #pragma HLS BIND_STORAGE variable=edge_info type=RAM_2P impl=URAM

        // 收集边信息
        for (int i = 0; i < cmd_count; i++) {
            #pragma HLS PIPELINE II=1
            auto cmd = cmd_batch_stream.read();
            edge_info[i] = 0;
            edge_info[i].range(127,96) = cmd.range(31,0);  // src
            edge_info[i].range(95,64)  = cmd.range(63,32); // dst
            edge_info[i].range(63,32)  = cmd.range(127,96); // Include opcode region
        }
        SORT_EDGES:
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
    hls::stream<MEMORY_INFO_ACCESS_COMMAND>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_to_mem,
    hls::stream<ap_uint<512>>& data_from_mem,
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
            MEMORY_INFO_ACCESS_COMMAND x = {};
            // x.src = 0;
            x.start = 0;
            x.len = 0;
            x.command_type = MEMORY_READ;
            x.last = 1;
            memory_access_stream.write(x);
            data_to_mem.write(0);

            break;
        }
        int pma_idx = info.pma_idx;
        VEC_INFO vec;
        MEMORY_INFO_ACCESS_COMMAND access_command = {};
        access_command.command_type = VEC_INFO_READ;
        access_command.pma_idx = pma_idx;
        access_command.last = 0;
        memory_access_stream.write(access_command);
        #pragma HLS protocol fixed
        ap_wait();
        vec = vec_info_from_accesser.read();        
        PMA_HEADER header;
        read_header(memory_access_stream, data_from_mem, header, vec.range(31,0));
        // pma_addr.write(vec);
        for (int i = 0 ; i < MAX_PMA_ADDR_REQUEST ; i++)
        {
            pma_addr[i].write(vec);
        }
        header_out.write(header);
    }
}
void get_bs_helper(
    PMA_HEADER& header,
    int src,
    int*bs_helper,
    int &cache_size,
    int pma_data_base_addr,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_from_mem
)
{
    #pragma HLS INLINE off
    int local_src = src % VERTEX_PER_PMA;
    int low_seg = local_src == 0 ? 0 : header.vertex_range[local_src - 1];
    int high_seg = header.vertex_range[local_src];
    cache_size = high_seg - low_seg;
    int segment_buffer[SEGMENT_SIZE];
    for (int i = 0 ; i < cache_size ; i++)
    {
        #pragma HLS PIPELINE off
        read_seg(
            memory_access_stream,
            data_from_mem,
            segment_buffer,
            pma_data_base_addr,
            low_seg + i
        );
        bs_helper[i] = segment_buffer[0];
    }
}
void sub_search(
    int* cache_bs_helper,
    int cache_size, //num of int
    int dst,
    int &searched_seg_number
)
{
    #pragma HLS INLINE off
    int low = 0;
    int high = cache_size - 1;
    searched_seg_number = -1;
    while (low <= high) {
        #pragma HLS PIPELINE off
        int mid = (low + high) / 2;
        if (cache_bs_helper[mid] == dst) {
            searched_seg_number = mid;
            return;
        } else if (cache_bs_helper[mid] < dst) {
            low = mid + 1;
            searched_seg_number = mid; // Record the last position where cache_bs_helper[mid] < src
        } else {
            high = mid - 1;
        }
    }
}


void search_and_insert_cu(
    // ap_uint<512>* hbm_ptr,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_to_mem,
    hls::stream<ap_uint<512>>& data_from_mem,
    
    hls::stream<PMA_HEADER> &pma_header_in,
    hls::stream<PMA_HEADER> &pma_header_out,
    hls::stream<VEC_INFO> &pma_addr,
    hls::stream<ap_uint<128>> &edge_info_stream,
    hls::stream<int> &edge_count,
    hls::stream<pma_idx_info> &pma_idx,
    hls::stream<inserted_seg>& inserted_pos
)
{
    printf("BINARY_SEARCH_AND_INSERT\n");
    while (true)
    {
        auto signal = pma_idx.read();
        if (signal.last == 1)
        {
            MEMORY_INFO_ACCESS_COMMAND x = {};
            // x.src = 0;
            x.start = 0;
            x.len = 0;
            x.command_type = MEMORY_READ;
            x.last = 1;
            memory_access_stream.write(x);
            data_to_mem.write(0);
            break;
        }
        inserted_seg inserted_seg_info;
        for (int i = 0 ; i < 16 ; i++)
        {
            inserted_seg_info.old_vertex_relative_seg_number[i] = -1;
            inserted_seg_info.vertex_number[i] = -1;
        }
        inserted_seg_info.has_insert = 0;
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

        for (int i = 0 ; i < SEGMENT_SIZE ; i++) storage[i] = -1;

        int bs_helper[1024];
        #pragma HLS BIND_STORAGE variable=bs_helper type=RAM_2P impl=URAM
        int bs_helper_size = 0;
        ap_uint<64> insert_ops[16];
        int insert_ops_count = 0;
        ap_uint<64> delete_edge_ops[16];
        int delete_edge_ops_count = 0;
        ap_uint<64> delete_vertex_ops[16];
        int delete_vertex_ops_count = 0;
        for (int i = 0 ; i < count ; i++)
        {
            auto edge_info = edge_info_stream.read();
            switch (edge_info.range(63,32).to_uint()) 
            {
                case PMA_INSERT_EDGE:
                    insert_ops[insert_ops_count++] = edge_info.range(127,64);
                    break;
                case PMA_DELETE_EDGE:
                    delete_edge_ops[delete_edge_ops_count++] = edge_info.range(127,64);
                    break;
                case PMA_DELETE_VERTEX:
                    delete_vertex_ops[delete_vertex_ops_count++] = edge_info.range(127,64);
                    break;
                default:
                    break;
            }
        }
        inserted_seg_info.has_insert = (insert_ops_count > 0) ? ap_uint<1>(1) : ap_uint<1>(0);
        for (int i = 0 ; i < delete_vertex_ops_count; i++)
        {
            #pragma HLS PIPELINE off
            #pragma HLS LOOP_FLATTEN off
            int src = delete_vertex_ops[i].range(63,32);
            int dst = delete_vertex_ops[i].range(31,0);

            int local_src = src % VERTEX_PER_PMA;
            int low_seg = local_src == 0 ? 0 : header.vertex_range[local_src - 1];
            int high_seg = header.vertex_range[local_src];              
            // Prepare empty buffer
            for(int k=0; k<SEGMENT_SIZE; k++) storage[k] = UNUSED_ICON;
            // Iterate through segments
            for (int s = low_seg; s < high_seg; s++)
            {
                // Write to mem
                write_seg(memory_access_stream, data_to_mem, data_from_mem, storage, pma_data_base_addr, s);
            }
            
            // Update header state
            header.edge_count[local_src] = 0;
        }
        int last_bs_cached_src = -1;
        for (int i = 0 ; i < insert_ops_count; i++)
        {
            #pragma HLS PIPELINE off
            #pragma HLS LOOP_FLATTEN off
            int src = insert_ops[i].range(63,32);
            int dst = insert_ops[i].range(31,0);

            int local_src = src % VERTEX_PER_PMA;
            if (last_bs_cached_src != src)
            {
                get_bs_helper(header, src, bs_helper, bs_helper_size, pma_data_base_addr, memory_access_stream, data_from_mem);
                last_bs_cached_src = src;
            }
            int searched_seg_number = -1;
            sub_search(bs_helper, bs_helper_size, dst, searched_seg_number);
            if (searched_seg_number == -1) searched_seg_number = 0;//insert at the first segment and the first element.
            inserted_seg_info.old_vertex_relative_seg_number[i] = searched_seg_number;
            inserted_seg_info.vertex_number[i] = local_src;
        }
        ap_uint<32> delete_segs[16];
        for (int i = 0 ; i < delete_edge_ops_count; i++)
        {
            #pragma HLS PIPELINE off
            #pragma HLS LOOP_FLATTEN off
            int src = delete_edge_ops[i].range(63,32);
            int dst = delete_edge_ops[i].range(31,0);

            int local_src = src % VERTEX_PER_PMA;
            if (last_bs_cached_src != src)
            {
                get_bs_helper(header, src, bs_helper, bs_helper_size, pma_data_base_addr, memory_access_stream, data_from_mem);
                last_bs_cached_src = src;
            }
            int searched_seg_number = -1;
            sub_search(bs_helper, bs_helper_size, dst, searched_seg_number);
            delete_segs[i] = searched_seg_number;
        }
        for (int i = 0 ; i < delete_edge_ops_count ; i++)
        {
            #pragma HLS PIPELINE off
            #pragma HLS LOOP_FLATTEN off
            int src = delete_edge_ops[i].range(63,32);
            int dst = delete_edge_ops[i].range(31,0);

            int local_src = src % VERTEX_PER_PMA;
            int low_seg = local_src == 0 ? 0 : header.vertex_range[local_src - 1];
            int target_seg = delete_segs[i] + low_seg;
            if (current_cached_id != target_seg) {
                if (is_dirty && current_cached_id != -1) {
                    write_seg(memory_access_stream, data_to_mem, data_from_mem, storage, pma_data_base_addr, current_cached_id);
                }
                read_seg(memory_access_stream, data_from_mem, storage, pma_data_base_addr, target_seg);
                current_cached_id = target_seg;
            }
            int found_idx_for_del = -1;
            for (int j = 0; j < SEGMENT_SIZE; j++) {
                #pragma HLS LOOP_TRIPCOUNT min=0 max=64
                int storage_val = storage[j];
                if (storage_val == dst) {
                    found_idx_for_del = j;
                    break;
                }
            }
            if (found_idx_for_del == -1) continue; // Edge not found, skip deletion
            for (int j = found_idx_for_del + 1; j < SEGMENT_SIZE; j++)
            {
                storage[j-1] = storage[j];
            }
            storage[SEGMENT_SIZE - 1] = UNUSED_ICON;
            is_dirty = true;
            header.edge_count[local_src]--;
        }
        for (int i = 0 ; i < insert_ops_count ; i++)
        {
            #pragma HLS PIPELINE off
            #pragma HLS LOOP_FLATTEN off
            int src = insert_ops[i].range(63,32);
            int dst = insert_ops[i].range(31,0);

            int local_src = src % VERTEX_PER_PMA;
            int low_seg = local_src == 0 ? 0 : header.vertex_range[local_src - 1];
            int target_seg = inserted_seg_info.old_vertex_relative_seg_number[i] + low_seg;
            if (current_cached_id != target_seg) {
                if (is_dirty && current_cached_id != -1) {
                    write_seg(memory_access_stream, data_to_mem, data_from_mem, storage, pma_data_base_addr, current_cached_id);
                }
                read_seg(memory_access_stream, data_from_mem, storage, pma_data_base_addr, target_seg);
                current_cached_id = target_seg;
            }
            int found_idx_for_insert = -1;
            for (int j = 0; j < SEGMENT_SIZE; j++) {
                #pragma HLS LOOP_TRIPCOUNT min=0 max=64
                int storage_val = storage[j];
                if (storage_val >= dst) {
                    found_idx_for_insert = j;
                    break;
                }
                if (storage_val == UNUSED_ICON) {
                    found_idx_for_insert = j;
                    break;
                }
            }
            for (int j = SEGMENT_SIZE - 1; j > found_idx_for_insert; j--)
            {
                if (j == 0) break;
                storage[j] = storage[j - 1];
            }
            storage[found_idx_for_insert] = dst;
            is_dirty = true;
            header.edge_count[local_src]++;
        }
        
        if (is_dirty && current_cached_id != -1) {
            write_seg(memory_access_stream, data_to_mem, data_from_mem, storage, pma_data_base_addr, current_cached_id);
            is_dirty = false;
        }
        pma_header_out.write(header);
        inserted_pos.write(inserted_seg_info);
    }
}

int valid_count(
    int seg_start, int seg_end, 
    hls::stream<MEMORY_INFO_ACCESS_COMMAND>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_from_mem, 
    int pma_data_base_addr
)
{
    #pragma HLS INLINE
    int total_segs = seg_end - seg_start;
    if (total_segs <= 0) return 0;

    MEMORY_INFO_ACCESS_COMMAND command;
    command.start = seg_start * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr;
    command.len =  total_segs * SEGMENT_SIZE * sizeof(int);
    command.command_type = MEMORY_READ;
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


void redistribute(
    int read_seg_start, int read_seg_end,
    int write_seg_start, int write_seg_end,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND>& memory_access_stream,
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
    hls::stream<MEMORY_INFO_ACCESS_COMMAND>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_to_mem,
    hls::stream<ap_uint<512>>& data_from_mem,
    hls::stream<PMA_HEADER>& old_pma_header_in,
    hls::stream<PMA_HEADER>& pma_header_in,
    hls::stream<VEC_INFO>& old_pma_addr_in,
    hls::stream<VEC_INFO>& pma_addr_in,
    hls::stream<pma_idx_info>& pma_idx,
    hls::stream<ap_uint<1>>& need_valid_count_in,
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
            MEMORY_INFO_ACCESS_COMMAND x = {};
            x.pma_idx = 0;
            x.start = 0;
            x.len = 0;
            x.command_type = MEMORY_READ;
            x.last = 1;
            memory_access_stream.write(x);
            data_to_mem.write(0);
            MEMORY_ALLOC_FREE_REQUEST_INTERNAL finish_req = 0;
            finish_req.range(67,64) = REQ_TYPE_FINISH;
            free_request_stream.write(finish_req);
            break;
        }
        ap_uint<1> need_valid_count = need_valid_count_in.read();
        PMA_HEADER pma_header = pma_header_in.read();
        PMA_HEADER old_pma_header = old_pma_header_in.read();

        VEC_INFO pma_addr = pma_addr_in.read();
        VEC_INFO old_pma_addr = old_pma_addr_in.read();

        auto to_check = inserted_seg_info.read();
        bool must_rebalance = (need_valid_count == 0) || (to_check.has_insert == 1);
        if (!must_rebalance)
        {
            COMMAND_STREAM ack = {};
            ack.data = signal.pma_idx;
            ack.last = 0;
            ack_back_to_router.write(ack);
            continue;
        }
        // int seg_count = pma_header.vertex_range[15];

        const int pma_data_base_addr = pma_addr.range(31,0) + sizeof(PMA_HEADER);
        const int old_pma_data_base_addr = old_pma_addr.range(31,0) + sizeof(PMA_HEADER);
        // const int pma_data_size_bytes = pma_addr.range(63,32) - sizeof(PMA_HEADER);

        if (need_valid_count == 0)
        {
            // Enlarged
            for (int v = 0 ; v < VERTEX_PER_PMA ; v++)
            {
                int old_seg_start = (v == 0) ? 0 : old_pma_header.vertex_range[v-1];
                int old_seg_end = old_pma_header.vertex_range[v];
                int new_seg_start = (v == 0) ? 0 : pma_header.vertex_range[v-1];
                int new_seg_end = pma_header.vertex_range[v];

                int total_valid_count = old_pma_header.edge_count[v];
                
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

                int seg_start = (v == 0) ? 0 : pma_header.vertex_range[v-1];
                int seg_end = pma_header.vertex_range[v];
                int seg_num = seg_end - seg_start; // 当前vertex的segment数
                int total_valid_count = 0;
                int seg_idx = relative_seg_num;
                total_valid_count = valid_count(seg_idx + seg_start, seg_idx + seg_start+ 1, memory_access_stream, data_from_mem, pma_data_base_addr);
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
                        total_valid_count += valid_count(left, last_left, memory_access_stream, data_from_mem, pma_data_base_addr);
                    }
                    else if (right > last_right) {
                        total_valid_count += valid_count(last_right, right, memory_access_stream, data_from_mem, pma_data_base_addr);
                    }

                    max_edge = density_capacity_from_seg_count(right - left);
                    if (total_valid_count <= max_edge)
                    {
                        redistribute(left, right, left, right,
                            memory_access_stream, data_to_mem, data_from_mem,
                            pma_data_base_addr, pma_data_base_addr,
                            total_valid_count);
                        break;
                    }
                    last_left = left;
                    last_right = right;
                }
            }
        }
        
        COMMAND_STREAM ack = {};
        ack.data = signal.pma_idx;
        ack.last = 0;
        ack_back_to_router.write(ack);
    }
}   

void move_and_change_header(
    // ap_uint<512>* hbm_ptr,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND>& memory_access_stream,
    // hls::stream<COMMAND>& cmd_to_mem,
    hls::stream<ap_uint<512>>& data_to_mem,
    hls::stream<ap_uint<512>>& data_from_mem,
    // VEC_INFO* pos_addr,
    // VEC_INFO* neg_addr,
    hls::stream<PMA_HEADER>& header_stream,
    hls::stream<PMA_HEADER>& old_header_stream_out,
    hls::stream<PMA_HEADER>& new_header_stream,

    hls::stream<VEC_INFO>& pma_addr_in,
    hls::stream<VEC_INFO>& old_pma_addr_out,
    hls::stream<VEC_INFO>& pma_addr_out,

    hls::stream<VEC_INFO>& new_alloc_addr,

    hls::stream<MEMORY_ALLOC_FREE_REQUEST_INTERNAL>& alloc_request_stream,
    hls::stream<ap_uint<1>>& need_valid_count_out,

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
            MEMORY_INFO_ACCESS_COMMAND x = {};
            x.pma_idx = 0;
            x.start = 0;
            x.len = 0;
            x.command_type = MEMORY_READ;
            x.last = 1;
            memory_access_stream.write(x);
            data_to_mem.write(0);

            // cmd_to_mem.write(MEM_STOP);

            MEMORY_ALLOC_FREE_REQUEST_INTERNAL finish_req = 0;
            finish_req.range(67,64) = REQ_TYPE_FINISH;
            alloc_request_stream.write(finish_req);
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
            MEMORY_ALLOC_FREE_REQUEST_INTERNAL alloc_req = 0;
            alloc_req.range(67,64) = REQ_TYPE_ALLOC;
            alloc_req.range(64,64) = 0; // Alloc
            int current_size = old_addr.range(63,32);
            int new_size = current_size + total_enlarge_segs * SEGMENT_SIZE * 4;
            alloc_req.range(63,32) = new_size;
            alloc_req.range(31,0) = 0;
            alloc_request_stream.write(alloc_req);
            #pragma HLS protocol fixed
            ap_wait();
            new_addr = new_alloc_addr.read();

            MEMORY_INFO_ACCESS_COMMAND access_command = {};
            access_command.command_type = VEC_INFO_WRITE;
            access_command.pma_idx = pma_index;
            access_command.last = 0;
            access_command.start = new_addr.range(31,0);
            access_command.len = new_addr.range(63,32); 
            memory_access_stream.write(access_command);
            // Copy logic moved to revalance_cu
        }
        else  
        {
            new_addr = old_addr;
        }
        int bias = (int)new_addr.range(31,0);
        write_header(memory_access_stream, data_to_mem, data_from_mem, header, bias);

        #pragma HLS protocol fixed
        old_pma_addr_out.write(old_addr);
        pma_addr_out.write(new_addr);
        old_header_stream_out.write(old_header);
        new_header_stream.write(header);
        need_valid_count_out.write(enlarged ? ap_uint<1>(0) : ap_uint<1>(1));
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

void merge_lane_alloc_free_requests(
    hls::stream<MEMORY_ALLOC_FREE_REQUEST_INTERNAL> alloc_request_stream[MAX_PIPELINE_NUM],
    hls::stream<MEMORY_ALLOC_FREE_REQUEST_INTERNAL>& free_request_stream,
    hls::stream<VEC_INFO_STREAM>& vec_info_back_stream,
    hls::stream<VEC_INFO> lane_vec_info_back_stream[MAX_PIPELINE_NUM],
    hls::stream<MEMORY_ALLOC_FREE_REQUEST>& out
)
{
    ap_uint<2> lane_fifo[64];
    #pragma HLS BIND_STORAGE variable=lane_fifo type=RAM_2P impl=BRAM
    int fifo_head = 0;
    int fifo_tail = 0;
    bool alloc_done[MAX_PIPELINE_NUM] = {false, false, false, false};
    bool free_done = false;

    while (true)
    {
        bool progress = false;

        for (int lane = 0; lane < MAX_PIPELINE_NUM; lane++)
        {
            if (!alloc_done[lane])
            {
                MEMORY_ALLOC_FREE_REQUEST_INTERNAL req;
                if (alloc_request_stream[lane].read_nb(req))
                {
                    progress = true;
                    if (req.range(67, 64) == REQ_TYPE_FINISH)
                    {
                        alloc_done[lane] = true;
                    }
                    else
                    {
                        MEMORY_ALLOC_FREE_REQUEST out_req;
                        out_req.data = req;
                        out_req.last = 0;
                        out.write(out_req);
                        lane_fifo[fifo_tail] = lane;
                        fifo_tail = (fifo_tail + 1) & 63;
                    }
                }
            }

            if (!free_done)
            {
                MEMORY_ALLOC_FREE_REQUEST_INTERNAL req;
                if (free_request_stream.read_nb(req))
                {
                    progress = true;
                    if (req.range(67, 64) == REQ_TYPE_FINISH)
                    {
                        free_done = true;
                    }
                    else
                    {
                        MEMORY_ALLOC_FREE_REQUEST out_req;
                        out_req.data = req;
                        out_req.last = 0;
                        out.write(out_req);
                    }
                }
            }
        }

        if (!vec_info_back_stream.empty() && fifo_head != fifo_tail)
        {
            progress = true;
            VEC_INFO_STREAM back_info = vec_info_back_stream.read();
            ap_uint<2> lane = lane_fifo[fifo_head];
            fifo_head = (fifo_head + 1) & 63;
            lane_vec_info_back_stream[lane].write(back_info.data);
        }

        if (alloc_done[0] && alloc_done[1] && alloc_done[2] && alloc_done[3] &&
            free_done &&
            fifo_head == fifo_tail)
        {
            break;
        }

        if (!progress)
        {
            ap_wait();
        }
    }
}

void relay_pma_idx(
    hls::stream<pma_idx_info>& in,
    hls::stream<pma_idx_info>& out
)
{
    while (true)
    {
        pma_idx_info info = in.read();
        out.write(info);
        if (info.last == 1) break;
    }
}

void merge_rebalance_inputs(
    hls::stream<PMA_HEADER> old_header_in[MAX_PIPELINE_NUM],
    hls::stream<PMA_HEADER> new_header_in[MAX_PIPELINE_NUM],
    hls::stream<VEC_INFO> old_addr_in[MAX_PIPELINE_NUM],
    hls::stream<VEC_INFO> new_addr_in[MAX_PIPELINE_NUM],
    hls::stream<pma_idx_info> pma_idx_in[MAX_PIPELINE_NUM],
    hls::stream<inserted_seg> inserted_in[MAX_PIPELINE_NUM],
    hls::stream<ap_uint<1>> need_valid_count_in[MAX_PIPELINE_NUM],
    hls::stream<PMA_HEADER>& old_header_out,
    hls::stream<PMA_HEADER>& new_header_out,
    hls::stream<VEC_INFO>& old_addr_out,
    hls::stream<VEC_INFO>& new_addr_out,
    hls::stream<pma_idx_info>& pma_idx_out,
    hls::stream<inserted_seg>& inserted_out,
    hls::stream<ap_uint<1>>& need_valid_count_out
)
{
    bool done[MAX_PIPELINE_NUM] = {false, false, false, false};
    int done_cnt = 0;
    while (done_cnt < MAX_PIPELINE_NUM)
    {
        bool progress = false;
        for (int lane = 0; lane < MAX_PIPELINE_NUM; lane++)
        {
            if (done[lane]) continue;
            pma_idx_info info;
            if (pma_idx_in[lane].read_nb(info))
            {
                progress = true;
                if (info.last == 1)
                {
                    done[lane] = true;
                    done_cnt++;
                    continue;
                }
                pma_idx_out.write(info);
                old_header_out.write(old_header_in[lane].read());
                new_header_out.write(new_header_in[lane].read());
                old_addr_out.write(old_addr_in[lane].read());
                new_addr_out.write(new_addr_in[lane].read());
                inserted_out.write(inserted_in[lane].read());
                need_valid_count_out.write(need_valid_count_in[lane].read());
            }
        }
        if (!progress)
        {
            ap_wait();
        }
    }
    pma_idx_info stop = {};
    stop.pma_idx = 0;
    stop.last = 1;
    pma_idx_out.write(stop);
}

void stop_unused_memory_channel(
    hls::stream<MEMORY_INFO_ACCESS_COMMAND>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_to_mem,
    hls::stream<ap_uint<512>>& data_from_mem
)
{
    MEMORY_INFO_ACCESS_COMMAND x = {};
    x.start = 0;
    x.len = 0;
    x.command_type = MEMORY_READ;
    x.last = 1;
    memory_access_stream.write(x);
    data_to_mem.write(0);
    ap_uint<512> dummy_from_mem;
    data_from_mem.read_nb(dummy_from_mem);
}

void edge_pipeline_lane(
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> memory_access_stream[MAX_MEMORY_REQUEST],
    hls::stream<ap_uint<512>> data_to_mem[MAX_MEMORY_REQUEST],
    hls::stream<ap_uint<512>> data_from_mem[MAX_MEMORY_REQUEST],
    hls::stream<VEC_INFO>& vec_info_from_accesser,
    hls::stream<pma_idx_info> pma_idx_info_request[MAX_PMA_IDX_INFO_REQUEST],
    hls::stream<int>& cmd_count_stream,
    hls::stream<ap_uint<128>>& cmd_batch_stream,
    hls::stream<PMA_HEADER>& old_pma_header_to_rebalance,
    hls::stream<PMA_HEADER>& new_pma_header,
    hls::stream<VEC_INFO>& old_pma_addr_to_rebalance,
    hls::stream<VEC_INFO>& pma_addr_to_rebalance,
    hls::stream<pma_idx_info>& rebalance_idx_out,
    hls::stream<inserted_seg>& inserted_seg_out,
    hls::stream<ap_uint<1>>& need_valid_count_out,
    hls::stream<VEC_INFO>& vec_info_back_stream,
    hls::stream<MEMORY_ALLOC_FREE_REQUEST_INTERNAL>& alloc_request_stream
)
{
    hls::stream<PMA_HEADER> pma_header_request;
    hls::stream<PMA_HEADER> header_from_insert;
    hls::stream<VEC_INFO> pma_addr_request[MAX_PMA_ADDR_REQUEST];
    hls::stream<ap_uint<128>> edge_info_stream;
    hls::stream<int> edge_count_stream;
    #pragma HLS STREAM variable=pma_header_request depth=4
    #pragma HLS STREAM variable=header_from_insert depth=4
    #pragma HLS STREAM variable=pma_addr_request depth=4
    #pragma HLS STREAM variable=edge_info_stream depth=16
    #pragma HLS STREAM variable=edge_count_stream depth=16

    #pragma HLS DATAFLOW

    sort_cu(
        cmd_count_stream,
        cmd_batch_stream,
        edge_info_stream,
        edge_count_stream,
        pma_idx_info_request[4]
    );

    head_read(
        memory_access_stream[0],
        data_to_mem[0],
        data_from_mem[0],
        vec_info_from_accesser,
        pma_idx_info_request[0],
        pma_header_request,
        pma_addr_request
    );

    search_and_insert_cu(
        memory_access_stream[1],
        data_to_mem[1],
        data_from_mem[1],
        pma_header_request,
        header_from_insert,
        pma_addr_request[0],
        edge_info_stream,
        edge_count_stream,
        pma_idx_info_request[3],
        inserted_seg_out
    );

    move_and_change_header(
        memory_access_stream[2],
        data_to_mem[2],
        data_from_mem[2],
        header_from_insert,
        old_pma_header_to_rebalance,
        new_pma_header,
        pma_addr_request[1],
        old_pma_addr_to_rebalance,
        pma_addr_to_rebalance,
        vec_info_back_stream,
        alloc_request_stream,
        need_valid_count_out,
        pma_idx_info_request[1]
    );

    relay_pma_idx(pma_idx_info_request[2], rebalance_idx_out);
}


void mem_adapter(
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> request[MAX_PIPELINE_NUM][MAX_MEMORY_REQUEST],
    hls::stream<ap_uint<512>> data_to_mem[MAX_PIPELINE_NUM][MAX_MEMORY_REQUEST],
    hls::stream<ap_uint<512>> data_from_mem[MAX_PIPELINE_NUM][MAX_MEMORY_REQUEST],
    hls::stream<VEC_INFO> vec_info_to_app[MAX_PIPELINE_NUM],

    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access,
    hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_mem_access,
    hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_mem_access
)
{
    ap_uint<MAX_PIPELINE_NUM * MAX_MEMORY_REQUEST> finished_bitmap = -1;
    while (true)
    {
        #pragma HLS PIPELINE II=1
        bool all_finished = (finished_bitmap == 0);
        if (all_finished) break;

        // 统一处理所有memory和vec_info请求
        for (int lane = 0; lane < MAX_PIPELINE_NUM; lane++) {
            for (int i = 0; i < MAX_MEMORY_REQUEST; i++) {
                int flat_idx = lane * MAX_MEMORY_REQUEST + i;
                if (finished_bitmap[flat_idx] == 0) continue;
                MEMORY_INFO_ACCESS_COMMAND request_info;
                if (request[lane][i].read_nb(request_info)) {
                    if (request_info.last == 1) {
                        finished_bitmap[flat_idx] = 0;
                        if (!data_to_mem[lane][i].empty()) {
                            auto dummy = data_to_mem[lane][i].read();
                        }
                    } else {
                        MEMORY_INFO_ACCESS_COMMAND cmd = request_info;
                        if (cmd.command_type == MEMORY_READ) {
                            req_to_mem_access.write(cmd);
                            #pragma HLS PROTOCOL fixed
                            ap_wait();
                            for (int j = 0; j < cmd.len / 64; j++) {
                                #pragma HLS PIPELINE II=1
                                ap_axiu<512, 0, 0, 0> packet = data_from_mem_access.read();
                                data_from_mem[lane][i].write(packet.data);
                            }
                        } else if (cmd.command_type == MEMORY_WRITE) {
                            req_to_mem_access.write(cmd);
                            #pragma HLS PROTOCOL fixed
                            ap_wait();
                            for (int j = 0; j < cmd.len / 64; j++) {
                                #pragma HLS PIPELINE II=1
                                ap_uint<512> data = data_to_mem[lane][i].read();
                                ap_axiu<512, 0, 0, 0> packet;
                                packet.data = data;
                                packet.last = (j == cmd.len / 64 - 1) ? 1 : 0;
                                data_to_mem_access.write(packet);
                            }
                        } else if (cmd.command_type == VEC_INFO_READ) {
                            req_to_mem_access.write(cmd);
                            #pragma HLS protocol fixed
                            ap_wait();
                            VEC_INFO vec_pkt = data_from_mem_access.read().data.range(63,0);
                            #pragma HLS PROTOCOL fixed
                            ap_wait();
                            vec_info_to_app[lane].write(vec_pkt);
                        } else if (cmd.command_type == VEC_INFO_WRITE) {
                            req_to_mem_access.write(cmd);
                        }
                    }
                }
            }
        }
        ap_wait();
    }
}

extern "C"
{
    void pma_insert_edge(
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
        hls::stream<MEMORY_ALLOC_FREE_REQUEST> &req_to_mem_alloc,
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
        // #pragma HLS INTERFACE axis port=vec_info_from_mem_access


        #pragma HLS STREAM variable=cmd_stream depth=16
        #pragma HLS STREAM variable=req_to_mem_alloc depth=16
        #pragma HLS STREAM variable=vec_info_back_stream depth=4
        #pragma HLS STREAM variable=ack_back_to_router depth=1

        #pragma HLS STREAM variable=req_to_mem_access depth=16
        #pragma HLS STREAM variable=data_to_mem_access depth=16
        #pragma HLS STREAM variable=data_from_mem_access depth=16
        // #pragma HLS STREAM variable=vec_info_from_mem_access depth=4
       
        hls::stream<pma_idx_info> pma_idx_info_request[MAX_PIPELINE_NUM][MAX_PMA_IDX_INFO_REQUEST];
        hls::stream<VEC_INFO> pma_addr_request[MAX_PIPELINE_NUM][MAX_PMA_ADDR_REQUEST];

        hls::stream<int> cmd_count_stream[MAX_PIPELINE_NUM];
        hls::stream<ap_uint<128>> cmd_batch_stream[MAX_PIPELINE_NUM];
        hls::stream<VEC_INFO> alloc_vec_info_stream[MAX_PIPELINE_NUM];
        hls::stream<MEMORY_ALLOC_FREE_REQUEST_INTERNAL> alloc_request_stream[MAX_PIPELINE_NUM];
        hls::stream<MEMORY_ALLOC_FREE_REQUEST_INTERNAL> free_request_stream;
        hls::stream<VEC_INFO> vec_info_to_app[MAX_PIPELINE_NUM];
        hls::stream<PMA_HEADER> old_pma_header_to_rebalance[MAX_PIPELINE_NUM];
        hls::stream<PMA_HEADER> new_pma_header_to_rebalance[MAX_PIPELINE_NUM];
        hls::stream<VEC_INFO> old_pma_addr_to_rebalance[MAX_PIPELINE_NUM];
        hls::stream<VEC_INFO> new_pma_addr_to_rebalance[MAX_PIPELINE_NUM];
        hls::stream<pma_idx_info> rebalance_idx_stream[MAX_PIPELINE_NUM];
        hls::stream<inserted_seg> inserted_seg_to_rebalance[MAX_PIPELINE_NUM];
        hls::stream<ap_uint<1>> need_valid_count_to_rebalance[MAX_PIPELINE_NUM];

        hls::stream<PMA_HEADER> merged_old_pma_header;
        hls::stream<PMA_HEADER> merged_new_pma_header;
        hls::stream<VEC_INFO> merged_old_pma_addr;
        hls::stream<VEC_INFO> merged_new_pma_addr;
        hls::stream<pma_idx_info> merged_rebalance_idx;
        hls::stream<inserted_seg> merged_inserted_seg;
        hls::stream<ap_uint<1>> merged_need_valid_count;

        hls::stream<MEMORY_INFO_ACCESS_COMMAND> memory_access_stream[MAX_PIPELINE_NUM][MAX_MEMORY_REQUEST];
        hls::stream<ap_uint<512>> data_to_mem[MAX_PIPELINE_NUM][MAX_MEMORY_REQUEST];
        hls::stream<ap_uint<512>> data_from_mem[MAX_PIPELINE_NUM][MAX_MEMORY_REQUEST];

        #pragma HLS STREAM variable=cmd_stream depth=16
        #pragma HLS STREAM variable=req_to_mem_alloc depth=16
        #pragma HLS STREAM variable=vec_info_back_stream depth=4
        #pragma HLS STREAM variable=ack_back_to_router depth=1
        #pragma HLS STREAM variable=req_to_mem_access depth=16
        #pragma HLS STREAM variable=data_to_mem_access depth=16
        #pragma HLS STREAM variable=data_from_mem_access depth=16
        #pragma HLS STREAM variable=pma_idx_info_request depth=16
        #pragma HLS STREAM variable=pma_addr_request depth=16
        #pragma HLS STREAM variable=cmd_count_stream depth=16
        #pragma HLS STREAM variable=cmd_batch_stream depth=16
        #pragma HLS STREAM variable=alloc_vec_info_stream depth=4
        #pragma HLS STREAM variable=alloc_request_stream depth=16
        #pragma HLS STREAM variable=free_request_stream depth=16
        #pragma HLS STREAM variable=vec_info_to_app depth=4
        #pragma HLS STREAM variable=old_pma_header_to_rebalance depth=4
        #pragma HLS STREAM variable=new_pma_header_to_rebalance depth=4
        #pragma HLS STREAM variable=old_pma_addr_to_rebalance depth=4
        #pragma HLS STREAM variable=new_pma_addr_to_rebalance depth=4
        #pragma HLS STREAM variable=rebalance_idx_stream depth=4
        #pragma HLS STREAM variable=inserted_seg_to_rebalance depth=4
        #pragma HLS STREAM variable=need_valid_count_to_rebalance depth=4
        #pragma HLS STREAM variable=merged_old_pma_header depth=4
        #pragma HLS STREAM variable=merged_new_pma_header depth=4
        #pragma HLS STREAM variable=merged_old_pma_addr depth=4
        #pragma HLS STREAM variable=merged_new_pma_addr depth=4
        #pragma HLS STREAM variable=merged_rebalance_idx depth=4
        #pragma HLS STREAM variable=merged_inserted_seg depth=4
        #pragma HLS STREAM variable=merged_need_valid_count depth=4
        #pragma HLS STREAM variable=memory_access_stream depth=16
        #pragma HLS STREAM variable=data_to_mem depth=16
        #pragma HLS STREAM variable=data_from_mem depth=16
        #pragma HLS DATAFLOW
        
        
        mem_adapter(
            memory_access_stream, data_to_mem, data_from_mem,  vec_info_to_app,
            req_to_mem_access, data_to_mem_access, data_from_mem_access
        );

        merge_lane_alloc_free_requests(
            alloc_request_stream,
            free_request_stream,
            vec_info_back_stream,
            alloc_vec_info_stream,
            req_to_mem_alloc
        );

        distributor(
            cmd_stream,
            cmd_count_stream,
            cmd_batch_stream,
            pma_idx_info_request
        );

        edge_pipeline_lane(
            memory_access_stream[0],
            data_to_mem[0],
            data_from_mem[0],
            vec_info_to_app[0],
            pma_idx_info_request[0],
            cmd_count_stream[0],
            cmd_batch_stream[0],
            old_pma_header_to_rebalance[0],
            new_pma_header_to_rebalance[0],
            old_pma_addr_to_rebalance[0],
            new_pma_addr_to_rebalance[0],
            rebalance_idx_stream[0],
            inserted_seg_to_rebalance[0],
            need_valid_count_to_rebalance[0],
            alloc_vec_info_stream[0],
            alloc_request_stream[0]
        );

        edge_pipeline_lane(
            memory_access_stream[1],
            data_to_mem[1],
            data_from_mem[1],
            vec_info_to_app[1],
            pma_idx_info_request[1],
            cmd_count_stream[1],
            cmd_batch_stream[1],
            old_pma_header_to_rebalance[1],
            new_pma_header_to_rebalance[1],
            old_pma_addr_to_rebalance[1],
            new_pma_addr_to_rebalance[1],
            rebalance_idx_stream[1],
            inserted_seg_to_rebalance[1],
            need_valid_count_to_rebalance[1],
            alloc_vec_info_stream[1],
            alloc_request_stream[1]
        );

        edge_pipeline_lane(
            memory_access_stream[2],
            data_to_mem[2],
            data_from_mem[2],
            vec_info_to_app[2],
            pma_idx_info_request[2],
            cmd_count_stream[2],
            cmd_batch_stream[2],
            old_pma_header_to_rebalance[2],
            new_pma_header_to_rebalance[2],
            old_pma_addr_to_rebalance[2],
            new_pma_addr_to_rebalance[2],
            rebalance_idx_stream[2],
            inserted_seg_to_rebalance[2],
            need_valid_count_to_rebalance[2],
            alloc_vec_info_stream[2],
            alloc_request_stream[2]
        );

        edge_pipeline_lane(
            memory_access_stream[3],
            data_to_mem[3],
            data_from_mem[3],
            vec_info_to_app[3],
            pma_idx_info_request[3],
            cmd_count_stream[3],
            cmd_batch_stream[3],
            old_pma_header_to_rebalance[3],
            new_pma_header_to_rebalance[3],
            old_pma_addr_to_rebalance[3],
            new_pma_addr_to_rebalance[3],
            rebalance_idx_stream[3],
            inserted_seg_to_rebalance[3],
            need_valid_count_to_rebalance[3],
            alloc_vec_info_stream[3],
            alloc_request_stream[3]
        );

        merge_rebalance_inputs(
            old_pma_header_to_rebalance,
            new_pma_header_to_rebalance,
            old_pma_addr_to_rebalance,
            new_pma_addr_to_rebalance,
            rebalance_idx_stream,
            inserted_seg_to_rebalance,
            need_valid_count_to_rebalance,
            merged_old_pma_header,
            merged_new_pma_header,
            merged_old_pma_addr,
            merged_new_pma_addr,
            merged_rebalance_idx,
            merged_inserted_seg,
            merged_need_valid_count
        );

        rebalance_cu(
            memory_access_stream[0][3],
            data_to_mem[0][3],
            data_from_mem[0][3],
            merged_old_pma_header,
            merged_new_pma_header,
            merged_old_pma_addr,
            merged_new_pma_addr,
            merged_rebalance_idx,
            merged_need_valid_count,
            merged_inserted_seg,
            ack_back_to_router,
            free_request_stream
        );

        stop_unused_memory_channel(memory_access_stream[1][3], data_to_mem[1][3], data_from_mem[1][3]);
        stop_unused_memory_channel(memory_access_stream[2][3], data_to_mem[2][3], data_from_mem[2][3]);
        stop_unused_memory_channel(memory_access_stream[3][3], data_to_mem[3][3], data_from_mem[3][3]);
    }
}