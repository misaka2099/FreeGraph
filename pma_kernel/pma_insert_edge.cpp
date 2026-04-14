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
#define MAX_MEMORY_REQUEST 5
#define MAX_PMA_IDX_INFO_REQUEST 5
#define MAX_PMA_ADDR_REQUEST 1
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

struct INTRA_COMMAND_STREAM
{
    ap_uint<32> data;
    ap_uint<1> last;
};

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
                    info.pma_idx = 0;
                    info.last = 1;
                    pma_info_stream.write(info);
                    cmd_count_stream.write(-1);
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
        pma_info_stream.write(info);
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
    hls::stream<int> &edge_count
)
{
    printf("SORT\n");
    while (true)
    {
        int cmd_count = cmd_count_stream.read();
        if (cmd_count == -1)
        {
            break;
        }
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

    hls::stream<pma_idx_info> &pma_idx_info_in,
    hls::stream<pma_idx_info> &pma_idx_info_out,
    hls::stream<PMA_HEADER>& header_out,
    hls::stream<VEC_INFO> &pma_addr
)
{
    printf("HEAD_READ\n");
    while (true)
    {
        auto info = pma_idx_info_in.read();
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
            pma_idx_info_out.write(info);
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
        pma_addr.write(vec);
        header_out.write(header);
        pma_idx_info_out.write(info);
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


void binary_search_cu(
    hls::stream<MEMORY_INFO_ACCESS_COMMAND>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_to_mem,
    hls::stream<ap_uint<512>>& data_from_mem,
    hls::stream<PMA_HEADER> &pma_header_in,
    hls::stream<PMA_HEADER> &pma_header_out,
    hls::stream<VEC_INFO> &pma_addr_in,
    hls::stream<VEC_INFO> &pma_addr_out,
    hls::stream<ap_uint<128>> &edge_info_stream,
    hls::stream<ap_uint<128>> &edge_info_to_insert,
    hls::stream<int> &edge_count_in,
    hls::stream<int> &edge_count_out,
    hls::stream<pma_idx_info> &pma_idx_in,
    hls::stream<pma_idx_info> &pma_idx_out,
    hls::stream<inserted_seg>& inserted_pos_to_insert,
    hls::stream<inserted_seg>& deleted_pos
)
{
    printf("BINARY_SEARCH\n");
    while (true)
    {
        auto signal = pma_idx_in.read();
        if (signal.last == 1)
        {
            MEMORY_INFO_ACCESS_COMMAND x = {};
            x.start = 0;
            x.len = 0;
            x.command_type = MEMORY_READ;
            x.last = 1;
            memory_access_stream.write(x);
            data_to_mem.write(0);
            pma_idx_out.write(signal);
            break;
        }

        auto header = pma_header_in.read();
        auto addr = pma_addr_in.read();
        int count = edge_count_in.read();
        int pma_data_base_addr = addr.range(31,0) + sizeof(PMA_HEADER);

        pma_header_out.write(header);
        pma_addr_out.write(addr);
        edge_count_out.write(count);

        inserted_seg inserted_seg_info;
        inserted_seg deleted_seg_info;
        for (int i = 0 ; i < 16 ; i++)
        {
            inserted_seg_info.old_vertex_relative_seg_number[i] = -1;
            inserted_seg_info.vertex_number[i] = -1;
            deleted_seg_info.old_vertex_relative_seg_number[i] = -1;
            deleted_seg_info.vertex_number[i] = -1;
        }

        int bs_helper[10240 ];
        #pragma HLS BIND_STORAGE variable=bs_helper type=RAM_2P impl=URAM
        int bs_helper_size = 0;
        int last_cached_src = -1;
        int insert_idx = 0;
        int delete_idx = 0;

        for (int i = 0 ; i < count ; i++)
        {
            #pragma HLS PIPELINE off
            auto edge_info = edge_info_stream.read();
            edge_info_to_insert.write(edge_info);

            int opcode = edge_info.range(63,32).to_uint();
            if (opcode == PMA_INSERT_EDGE || opcode == PMA_DELETE_EDGE)
            {
                ap_uint<64> payload = edge_info.range(127,64);
                int src = payload.range(63,32);
                int dst = payload.range(31,0);
                int local_src = src % VERTEX_PER_PMA;

                if (last_cached_src != src)
                {
                    get_bs_helper(header, src, bs_helper, bs_helper_size, pma_data_base_addr, memory_access_stream, data_from_mem);
                    last_cached_src = src;
                }

                int searched_seg_number = -1;
                sub_search(bs_helper, bs_helper_size, dst, searched_seg_number);
                if (searched_seg_number == -1)
                {
                    searched_seg_number = 0;
                }

                if (opcode == PMA_INSERT_EDGE)
                {
                    inserted_seg_info.old_vertex_relative_seg_number[insert_idx] = searched_seg_number;
                    inserted_seg_info.vertex_number[insert_idx] = local_src;
                    insert_idx++;
                }
                else
                {
                    deleted_seg_info.old_vertex_relative_seg_number[delete_idx] = searched_seg_number;
                    deleted_seg_info.vertex_number[delete_idx] = local_src;
                    delete_idx++;
                }
            }
        }

        inserted_pos_to_insert.write(inserted_seg_info);
        deleted_pos.write(deleted_seg_info);
        pma_idx_out.write(signal);
    }
}

void insert_cu(
    hls::stream<MEMORY_INFO_ACCESS_COMMAND>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_to_mem,
    hls::stream<ap_uint<512>>& data_from_mem,
    hls::stream<PMA_HEADER> &pma_header_in,
    hls::stream<PMA_HEADER> &pma_header_out,
    hls::stream<VEC_INFO> &pma_addr_in,
    hls::stream<VEC_INFO> &pma_addr_out,
    hls::stream<ap_uint<128>> &edge_info_stream,
    hls::stream<int> &edge_count,
    hls::stream<pma_idx_info> &pma_idx_in,
    hls::stream<pma_idx_info> &pma_idx_out,
    hls::stream<inserted_seg>& inserted_pos,
    hls::stream<inserted_seg>& inserted_pos_to_rebalance,
    hls::stream<inserted_seg>& deleted_pos
)
{
    printf("INSERT\n");
    while (true)
    {
        auto signal = pma_idx_in.read();
        if (signal.last == 1)
        {
            MEMORY_INFO_ACCESS_COMMAND x = {};
            x.start = 0;
            x.len = 0;
            x.command_type = MEMORY_READ;
            x.last = 1;
            memory_access_stream.write(x);
            data_to_mem.write(0);
            pma_idx_out.write(signal);
            break;
        }

        inserted_seg inserted_seg_info = inserted_pos.read();
        inserted_seg deleted_seg_info = deleted_pos.read();
        auto header = pma_header_in.read();
        auto addr = pma_addr_in.read();
        int count = edge_count.read();
        int pma_data_base_addr = addr.range(31,0) + sizeof(PMA_HEADER);

        ap_uint<64> insert_ops[16];
        ap_uint<64> delete_edge_ops[16];
        ap_uint<64> delete_vertex_ops[16];
        int insert_ops_count = 0;
        int delete_edge_ops_count = 0;
        int delete_vertex_ops_count = 0;

        for (int i = 0 ; i < count ; i++)
        {
            #pragma HLS PIPELINE off
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

        int current_cached_id = -1;
        bool is_dirty = false;
        int storage[SEGMENT_SIZE];
        for (int i = 0 ; i < SEGMENT_SIZE ; i++)
        {
            storage[i] = UNUSED_ICON;
        }

        for (int i = 0 ; i < delete_vertex_ops_count; i++)
        {
            #pragma HLS PIPELINE off
            int src = delete_vertex_ops[i].range(63,32);
            int local_src = src % VERTEX_PER_PMA;
            int low_seg = local_src == 0 ? 0 : header.vertex_range[local_src - 1];
            int high_seg = header.vertex_range[local_src];
            for (int k = 0; k < SEGMENT_SIZE; k++)
            {
                storage[k] = UNUSED_ICON;
            }
            for (int s = low_seg; s < high_seg; s++)
            {
                write_seg(memory_access_stream, data_to_mem, data_from_mem, storage, pma_data_base_addr, s);
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
                    write_seg(memory_access_stream, data_to_mem, data_from_mem, storage, pma_data_base_addr, current_cached_id);
                }
                read_seg(memory_access_stream, data_from_mem, storage, pma_data_base_addr, target_seg);
                current_cached_id = target_seg;
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
                    write_seg(memory_access_stream, data_to_mem, data_from_mem, storage, pma_data_base_addr, current_cached_id);
                }
                read_seg(memory_access_stream, data_from_mem, storage, pma_data_base_addr, target_seg);
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
            write_seg(memory_access_stream, data_to_mem, data_from_mem, storage, pma_data_base_addr, current_cached_id);
            is_dirty = false;
        }

        pma_header_out.write(header);
        pma_addr_out.write(addr);
        inserted_pos_to_rebalance.write(inserted_seg_info);
        pma_idx_out.write(signal);
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
        
        ack_back_to_router.write({.data=signal.pma_idx});
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

    hls::stream<VEC_INFO_STREAM>& new_alloc_addr,

    hls::stream<MEMORY_ALLOC_FREE_REQUEST_INTERNAL>& alloc_request_stream,

    hls::stream<pma_idx_info>& pma_idx_info_in,
    hls::stream<pma_idx_info>& pma_idx_info_out
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
            pma_idx_info_out.write(pma_info);
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
            auto new_addr_stream = new_alloc_addr.read(); // Read from allocation result (which was requested by binary_search)
            new_addr = new_addr_stream.data;

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
        pma_idx_info_out.write(pma_info);
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
    MEMORY_ALLOC_FREE_REQUEST finish_req = {};
    finish_req.data.range(67, 64) = REQ_TYPE_FINISH;
    finish_req.last = 1;
    out.write(finish_req);
}


void mem_adapter(
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> request[MAX_MEMORY_REQUEST],
    hls::stream<ap_uint<512>> data_to_mem[MAX_MEMORY_REQUEST],
    hls::stream<ap_uint<512>> data_from_mem[MAX_MEMORY_REQUEST],
    hls::stream<VEC_INFO> &vec_info_to_app,

    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access,
    hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_mem_access,
    hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_mem_access
)
{
    ap_uint<MAX_MEMORY_REQUEST> finished_bitmap = -1;
    int idx = 0;
    while (true)
    {
        #pragma HLS PIPELINE II=1
        bool all_finished = (finished_bitmap == 0);
        if (all_finished) break;

        // 统一处理所有memory和vec_info请求
        for (int i = 0; i < MAX_MEMORY_REQUEST; i++) {
            if (finished_bitmap[i] == 0) continue;
            MEMORY_INFO_ACCESS_COMMAND request_info;
            if (request[i].read_nb(request_info)) {
                if (request_info.last == 1) {
                    finished_bitmap[i] = 0;
                    if (!data_to_mem[i].empty()) {
                        auto dummy = data_to_mem[i].read();
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
                            data_from_mem[i].write(packet.data);
                        }
                    } else if (cmd.command_type == MEMORY_WRITE) {
                        req_to_mem_access.write(cmd);
                        #pragma HLS PROTOCOL fixed
                        ap_wait();
                        for (int j = 0; j < cmd.len / 64; j++) {
                            #pragma HLS PIPELINE II=1
                            ap_uint<512> data = data_to_mem[i].read();
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
                        vec_info_to_app.write(vec_pkt);
                    } else if (cmd.command_type == VEC_INFO_WRITE) {
                        req_to_mem_access.write(cmd);
                    }
                }
            }
        }
        ap_wait();
    }
    MEMORY_INFO_ACCESS_COMMAND finish_cmd = {};
    finish_cmd.last = 1;
    finish_cmd.command_type = MEMORY_FINISH;
    req_to_mem_access.write(finish_cmd);
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
       
        hls::stream<pma_idx_info> pma_idx_from_cmd;
        hls::stream<pma_idx_info> pma_idx_after_head;
        hls::stream<VEC_INFO> pma_addr_request;
        hls::stream<PMA_HEADER> pma_header_request ;

        hls::stream<int> cmd_count_stream ;
        hls::stream<ap_uint<128>> cmd_batch_stream ;

        hls::stream<ap_uint<128>> edge_info_stream ;
        hls::stream<int> edge_count_stream ;

        hls::stream<PMA_HEADER> header_from_insert ;
        hls::stream<PMA_HEADER> header_after_bs ;
        hls::stream<PMA_HEADER> header_after_insert ;

        hls::stream<PMA_HEADER> new_pma_header ;
        hls::stream<VEC_INFO> pma_addr_to_rebalance ;
        hls::stream<VEC_INFO> pma_addr_after_bs ;
        hls::stream<VEC_INFO> pma_addr_after_insert ;
        
        hls::stream<VEC_INFO> old_pma_addr_to_rebalance;
        hls::stream<PMA_HEADER> old_pma_header_to_rebalance;

        hls::stream<inserted_seg> inserted_seg_info ;
        hls::stream<inserted_seg> inserted_seg_for_rebalance ;
        hls::stream<inserted_seg> deleted_seg_info ;

        hls::stream<ap_uint<128>> edge_info_after_bs ;
        hls::stream<int> edge_count_after_bs ;
        hls::stream<pma_idx_info> pma_idx_after_bs ;
        hls::stream<pma_idx_info> pma_idx_after_insert ;
        hls::stream<pma_idx_info> pma_idx_to_rebalance;

        hls::stream<MEMORY_ALLOC_FREE_REQUEST_INTERNAL> alloc_request_stream;
        hls::stream<MEMORY_ALLOC_FREE_REQUEST_INTERNAL> free_request_stream;
        #pragma HLS STREAM variable=alloc_request_stream depth=16
        #pragma HLS STREAM variable=free_request_stream depth=16
        #pragma HLS STREAM variable=old_pma_addr_to_rebalance depth=4
        #pragma HLS STREAM variable=old_pma_header_to_rebalance depth=4
        #pragma HLS STREAM variable=header_after_bs depth=4
        #pragma HLS STREAM variable=header_after_insert depth=4
        #pragma HLS STREAM variable=pma_addr_after_bs depth=4
        #pragma HLS STREAM variable=pma_addr_after_insert depth=4
        #pragma HLS STREAM variable=deleted_seg_info depth=4
        #pragma HLS STREAM variable=edge_info_after_bs depth=16
        #pragma HLS STREAM variable=edge_count_after_bs depth=16
        #pragma HLS STREAM variable=pma_idx_after_bs depth=4
        #pragma HLS STREAM variable=pma_idx_after_insert depth=4

        hls::stream<MEMORY_INFO_ACCESS_COMMAND> memory_access_stream [MAX_MEMORY_REQUEST];
        hls::stream<ap_uint<512>> data_to_mem [MAX_MEMORY_REQUEST];
        hls::stream<ap_uint<512>> data_from_mem [MAX_MEMORY_REQUEST];
        #pragma HLS STREAM variable=memory_access_stream depth=16
        #pragma HLS STREAM variable=data_to_mem depth=16
        #pragma HLS STREAM variable=data_from_mem depth=16
        #pragma HLS STREAM variable=pma_idx_from_cmd depth=16
        #pragma HLS STREAM variable=pma_idx_after_head depth=16
        #pragma HLS STREAM variable=pma_addr_request depth=16
        #pragma HLS STREAM variable=cmd_batch_stream depth=16
        #pragma HLS STREAM variable=edge_info_stream depth=16
        #pragma HLS STREAM variable=inserted_seg_for_rebalance depth=4
        hls::stream<VEC_INFO> vec_info_from_app ;
        hls::stream<VEC_INFO> vec_info_to_app;
        #pragma HLS DATAFLOW
        
        
        mem_adapter(
            memory_access_stream, data_to_mem, data_from_mem,  vec_info_to_app,
            req_to_mem_access, data_to_mem_access, data_from_mem_access
        );

        merge_requests(alloc_request_stream, free_request_stream, req_to_mem_alloc);

        process_cmd(
            cmd_stream,
            cmd_count_stream,
            cmd_batch_stream,
            pma_idx_from_cmd
        );
        
        sort_cu(
            cmd_count_stream,
            cmd_batch_stream,
            edge_info_stream,
            edge_count_stream
        );
        
        head_read(
            memory_access_stream[0],
            data_to_mem[0],
            data_from_mem[0],
            vec_info_to_app, 
            pma_idx_from_cmd,
            pma_idx_after_head,
            pma_header_request,
            pma_addr_request
        );

        binary_search_cu(
            memory_access_stream[1],
            data_to_mem[1],
            data_from_mem[1],
            pma_header_request,
            header_after_bs,
            pma_addr_request,
            pma_addr_after_bs,
            edge_info_stream,
            edge_info_after_bs,
            edge_count_stream,
            edge_count_after_bs,
            pma_idx_after_head,
            pma_idx_after_bs,
            inserted_seg_info,
            deleted_seg_info
        );

        insert_cu(
            memory_access_stream[2],
            data_to_mem[2],
            data_from_mem[2],
            header_after_bs,
            header_after_insert,
            pma_addr_after_bs,
            pma_addr_after_insert,
            edge_info_after_bs,
            edge_count_after_bs,
            pma_idx_after_bs,
            pma_idx_after_insert,
            inserted_seg_info,
            inserted_seg_for_rebalance,
            deleted_seg_info
        );

        move_and_change_header(
            memory_access_stream[3],
            data_to_mem[3],
            data_from_mem[3],
            header_after_insert,
            old_pma_header_to_rebalance,
            new_pma_header, 

            pma_addr_after_insert,
            old_pma_addr_to_rebalance,
            pma_addr_to_rebalance, 

            // vec_info_back_stream,
            // vec_info_to_mem_stream,
            // cmd_to_mem_stream,
            vec_info_back_stream,
            alloc_request_stream,
            pma_idx_after_insert,
            pma_idx_to_rebalance
        );
        
        rebalance_cu(
            // hbm_ptr_4,
            memory_access_stream[4],
            data_to_mem[4],
            data_from_mem[4],
            old_pma_header_to_rebalance,
            new_pma_header,
            old_pma_addr_to_rebalance,
            pma_addr_to_rebalance,
            pma_idx_to_rebalance,
            inserted_seg_for_rebalance,
            ack_back_to_router,
            free_request_stream
        );
    }
}