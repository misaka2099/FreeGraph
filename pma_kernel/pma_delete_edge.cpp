#include "data_type.h"
#include <hls_stream.h>
#include <ap_utils.h>
#include <cstdio>
#include <climits>

#define MAX_PMA_IDX_INFO_REQUEST 6
#define MAX_READ_REQUEST 5
#define MAX_WRITE_REQUEST 3
#define MAX_MEMORY_REQUEST 2

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
struct VEC_INFO_ACCESS 
{
    memory_access_type command_type;
    ap_uint<1> direction;
    int pma_idx;
};
// process command: read batches from cmd_stream (PMA_UPDATE_MESSAGE) and output count + batch
void process_cmd_delete(
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
    hls::stream<int> &cmd_count_stream,
    hls::stream<ap_uint<128>> &cmd_batch_stream,
    hls::stream<pma_idx_info> pma_info_request[MAX_PMA_IDX_INFO_REQUEST]
)
{
    printf("DELETE_PROCESS_CMD\n");
    while(true)
    {
        if (cmd_stream.empty()) { ap_wait(); continue; }
        ap_uint<128> cmd_batch[MAX_CMD_NUM];
        #pragma HLS ARRAY_PARTITION variable=cmd_batch complete
        int cmd_count = 0;
        for (int i = 0; i < MAX_CMD_NUM; i++) {
            #pragma HLS PIPELINE II=1
            PMA_UPDATE_MESSAGE cmd = cmd_stream.read();
            if (cmd.data == PMA_STOP) {
                pma_idx_info info;
                info.last = cmd.last;
                for (int k = 0 ; k < MAX_PMA_IDX_INFO_REQUEST ; k++) pma_info_request[k].write(info);
                if (cmd.last == 1) return;
            }
            cmd_batch[i] = cmd.data;
            cmd_batch[i][127] = 1;
            cmd_count++;
            if (cmd.last == 1) break;
        }
        pma_idx_info info;
        info.direction = cmd_batch[0].range(98,98);
        info.pma_idx = cmd_batch[0].range(31,0) / VERTEX_PER_PMA;
        for (int k = 0 ; k < MAX_PMA_IDX_INFO_REQUEST ; k++) pma_info_request[k].write(info);
        cmd_count_stream.write(cmd_count);
        for (int i = 0; i < cmd_count; i++) {
            #pragma HLS PIPELINE II=1
            cmd_batch_stream.write(cmd_batch[i]);
        }
    }
}


// Binary search and immediate-delete: find target, then left-shift across segments to fill gap, write changed segments, update header
// memory access types and command
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
    ap_uint<512> buffer[4];
    // for (int i = 0; i < 4; i++) {
    //     #pragma HLS PIPELINE II=1
    //     buffer[i] = hbm_ptr[pma_header_addr_in_512_patten + i];
    // }
    memory_access_command command;
    command.addr_in_byte_patten = pma_header_addr_in_byte;
    command.len_in_byte_patten =  sizeof(PMA_HEADER);
    command.type = READ;
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

    // attr是double，共16个double，每个double 8字节
    for (int j = 0; j < 8; j++) {
        #pragma HLS UNROLL
        // 第3个buffer存前8个double
        ap_uint<64> d = buffer[2].range(64 * (j + 1) - 1, 64 * j);
        pma_header.attr[j] = *reinterpret_cast<double*>(&d);
        // 第4个buffer存后8个double
        d = buffer[3].range(64 * (j + 1) - 1, 64 * j);
        pma_header.attr[j + 8] = *reinterpret_cast<double*>(&d);
    }
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

void hbm_access_unit(
    hls::stream<memory_access_command> request[MAX_MEMORY_REQUEST],
    hls::stream<ap_uint<512>> data_in[MAX_MEMORY_REQUEST],
    hls::stream<ap_uint<512>> data_out[MAX_MEMORY_REQUEST],
    ap_uint<512>* hbm_ptr
)
{
    // bool finished[MAX_MEMORY_REQUEST];
    memory_access_command request_info;
    int addr_in_512_pattern = 0;
    int len_in_512_pattern  = 0;
    // for (int i = 0 ; i < MAX_MEMORY_REQUEST ; i++) finished[i] = false;
    ap_uint<MAX_MEMORY_REQUEST> finished_bitmap = -1;
    int idx = 0;
    while (
        finished_bitmap != 0
    )
    {
        if(request[idx].read_nb(request_info))
        {
            if (request_info.type == FINISH)
            {
                finished_bitmap[MAX_MEMORY_REQUEST + idx] = 0;
                auto dummy = data_in[idx].read();
                continue;
            }
            addr_in_512_pattern = request_info.addr_in_byte_patten >> 6;
            len_in_512_pattern = request_info.len_in_byte_patten >> 6;
            if (request_info.type == READ)
            {
                for (int i = 0 ; i < len_in_512_pattern ; i++)
                {
                    auto x = hbm_ptr[addr_in_512_pattern + i];
                    data_out[idx].write(x);
                }
            }
            else if (request_info.type == WRITE)
            {
                for (int i = 0 ; i < len_in_512_pattern ; i++)
                {
                    auto x = data_in[idx].read();
                    hbm_ptr[addr_in_512_pattern + i] = x;
                }
                // data_out[worker_num][idx].write(WRITE_ACK);
            }
        }
        idx = (idx + 1);
        idx = idx % MAX_MEMORY_REQUEST;
    }
}

void head_read_delete(
    hls::stream<memory_access_command> &memory_access_stream,
    hls::stream<ap_uint<512>> &data_from_mem,
    hls::stream<ap_uint<512>> &data_to_mem,
    hls::stream<VEC_INFO_ACCESS> &vec_info_access_command,
    VEC_INFO* pos_pma_addrs,
    VEC_INFO* neg_pma_addrs,
    hls::stream<pma_idx_info> &pma_idx_info_request,
    hls::stream<PMA_HEADER>& header_out,
    hls::stream<VEC_INFO> &pma_addr_out
)
{
    printf("DELETE_HEAD_READ\n");
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
        int direction = info.direction;
        VEC_INFO vec = (direction == OUT_DIRECTION) ? pos_pma_addrs[pma_idx] : neg_pma_addrs[pma_idx];
        PMA_HEADER header;
        int bias = int(vec.range(31,0));
        read_header(memory_access_stream, data_from_mem, header, bias);
        // fanout pma_addr
        pma_addr_out.write(vec);
        header_out.write(header);
    }
}

void binary_search_delete(
    hls::stream<memory_access_command> &memory_access_stream,
    hls::stream<ap_uint<512>> &data_to_mem,
    hls::stream<ap_uint<512>> &data_from_mem,
    hls::stream<PMA_HEADER> &header_info_in_stream,
    hls::stream<VEC_INFO> &pma_addr,
    hls::stream<ap_uint<64>> &edge_info_stream,
    hls::stream<int> &edge_count,
    hls::stream<pma_idx_info> &for_exit_signal,
    hls::stream<int> &ack_stream
)
{
    printf("BINARY_SEARCH_DELETE_IMMEDIATE\n");
    while (true)
    {
        auto exit_signal = for_exit_signal.read();
        // if (exit_signal.last == 1) break;
        if (exit_signal.last == 1)
        {
            memory_access_command x;
            x.addr_in_byte_patten = 0;
            x.len_in_byte_patten = 0;
            x.type = FINISH;
            memory_access_stream.write(x);
            data_to_mem.write(0);
            break;
        }
        auto header = header_info_in_stream.read();
        auto addr = pma_addr.read();
        int count = edge_count.read();
        int pma_data_base_addr = addr.range(31,0) + sizeof(PMA_HEADER);
        int pma_idx = exit_signal.pma_idx;
        for (int i = 0; i < count; i++)
        {
            auto edge_info = edge_info_stream.read();
            int src = edge_info.range(63,32);
            int dst = edge_info.range(31,0);
            int local_src = src % VERTEX_PER_PMA;
            int seg_start = local_src == 0 ? 0 : header.vertex_range[local_src - 1];
            int seg_end = header.vertex_range[local_src] - 1;
            int low_seg = seg_start;
            int high_seg = seg_end;
            bool deleted = false;
            // binary-search across segments
            while (low_seg <= high_seg)
            {
                int mid = low_seg + (high_seg - low_seg) / 2;
                int storage[SEGMENT_SIZE];
                for (int t = 0; t < SEGMENT_SIZE; t++) storage[t] = UNUSED_ICON;
                read_seg(memory_access_stream, data_from_mem, storage, pma_data_base_addr, mid);
                int minimum = INT_MAX;
                int maximum = INT_MIN;
                bool match_perfect = false;
                int found_offset = -1;
                for (int j = 0; j < SEGMENT_SIZE; j++) {
                    #pragma HLS PIPELINE II=1
                    int storage_val = storage[j];
                    if (storage_val != UNUSED_ICON) 
                    {
                        maximum = std::max(maximum, storage_val);
                        minimum = std::min(minimum, storage_val);
                        if (storage_val == dst) 
                        {
                            found_offset = j;
                            match_perfect = true;
                            break;
                        }
                        if (storage_val < dst) 
                        {
                            // continue
                        } 
                        else 
                        {
                            break;
                        }
                    }
                }
                if (match_perfect) 
                {
                    // deleting in last segment, simple local shift
                    for (int k = found_offset; k < SEGMENT_SIZE - 1; k++) 
                    {
                        #pragma HLS PIPELINE II=1
                        storage[k] = storage[k + 1];
                    }
                    storage[SEGMENT_SIZE - 1] = UNUSED_ICON;
                    write_seg(memory_access_stream, data_to_mem, data_from_mem, storage, pma_data_base_addr, mid);
                    
                    header.edge_count[local_src]--;
                    deleted = true;
                    break;
                }
                if (maximum < dst) low_seg = mid + 1;
                else if (minimum > dst) high_seg = mid - 1;
                else break; // target not in this pmA
            }
            // if not found, nothing to do
        }
        // Write back header using memory streams: header at addr.range(31,0)
        write_header(memory_access_stream, data_to_mem, data_from_mem, header, int(addr.range(31,0)));
        // ack this pma index processed
        ack_stream.write(pma_idx);
    }
}

// ack sender to router
void ack_sender(hls::stream<int> &ack_stream, hls::stream<COMMAND_STREAM> &ack_to_router) {
    while (true) {
        int idx = ack_stream.read();
        if (idx == -1) {
            // forward stop
            ack_to_router.write({.data = (ap_uint<32>)-1, .last = 1});
            break;
        }
        COMMAND_STREAM out;
        out.data = idx;
        out.last = 0;
        ack_to_router.write(out);
    }
}

extern "C" {
    void pma_delete_edge(
        VEC_INFO* pos_pma_addrs,
        VEC_INFO* neg_pma_addrs,
        ap_uint<512>* hbm_ptr,
        hls::stream<PMA_UPDATE_MESSAGE>& cmd_stream,
        hls::stream<COMMAND_STREAM>& ack_to_router
    ) {
        #pragma HLS INTERFACE m_axi port=pos_pma_addrs offset=slave bundle=gmem0 depth=50
        #pragma HLS INTERFACE m_axi port=neg_pma_addrs offset=slave bundle=gmem1 depth=50
        #pragma HLS INTERFACE m_axi port=hbm_ptr offset=slave bundle=gmem2 depth=50 max_widen_bitwidth=512
        #pragma HLS INTERFACE axis port=cmd_stream
        #pragma HLS INTERFACE axis port=ack_to_router

        // streams
        hls::stream<int> cmd_count_stream;
        hls::stream<ap_uint<128>> cmd_batch_stream;
        hls::stream<ap_uint<64>> edge_info_stream;
        hls::stream<int> edge_count_stream;
        hls::stream<PMA_HEADER> header_stream;
        hls::stream<VEC_INFO> pma_addr_stream;
        hls::stream<pma_idx_info> pma_idx_info_request[MAX_PMA_IDX_INFO_REQUEST];
        hls::stream<int> ack_stream;

        #pragma HLS STREAM variable=cmd_count_stream depth=4
        #pragma HLS STREAM variable=cmd_batch_stream depth=16
        #pragma HLS STREAM variable=edge_info_stream depth=16
        #pragma HLS STREAM variable=edge_count_stream depth=4
        #pragma HLS STREAM variable=header_stream depth=4
        #pragma HLS STREAM variable=pma_addr_stream depth=4
        #pragma HLS STREAM variable=ack_stream depth=4

        // memory access streams
        hls::stream<memory_access_command> memory_access_stream;
        hls::stream<ap_uint<512>> data_to_mem;
        hls::stream<ap_uint<512>> data_from_mem;
        #pragma HLS STREAM variable=memory_access_stream depth=16
        #pragma HLS STREAM variable=data_to_mem depth=16
        #pragma HLS STREAM variable=data_from_mem depth=16

        #pragma HLS DATAFLOW
        // hbm access unit services memory requests
        hbm_access_unit(memory_access_stream, data_to_mem, data_from_mem, hbm_ptr);

        process_cmd_delete(cmd_stream, cmd_count_stream, cmd_batch_stream, pma_idx_info_request);
        head_read_delete(memory_access_stream, data_from_mem, pos_pma_addrs, neg_pma_addrs, pma_idx_info_request[0], header_stream, pma_addr_stream);
        binary_search_delete(memory_access_stream, data_to_mem, data_from_mem, header_stream, pma_addr_stream, edge_info_stream, edge_count_stream, pma_idx_info_request[3], ack_stream);
        ack_sender(ack_stream, ack_to_router);
    }
}
