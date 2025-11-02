#include <cstdio>
#include <cstring>
#include <hls_stream.h>
#include <ap_int.h>
#include "data_type.h"
#include "debug.h"
#include "pma.h"
#define MAX_CMD_NUM 16
#define MAX_TMP_PMA_NUM 1024


extern "C"
{
    void pma_update_kernel(
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
        VEC_INFO* pos_pma_addrs,
        VEC_INFO* neg_pma_addrs,
        BYTE *hbm_ptr,
        //for memory manage
        hls::stream<VEC_INFO_STREAM> &vec_info_stream,
        hls::stream<COMMAND_STREAM> &cmd_to_mem_stream,
        // hls::stream<BYTE_STREAM> &data_to_mem_stream,
        // hls::stream<BYTE_STREAM> &data_from_mem_stream,
        hls::stream<VEC_INFO_STREAM> &pma_addr_back_stream,
        hls::stream<COMMAND_STREAM> &ack_back_to_router
        // hls::stream<COMMAND_STREAM> &stop_signal
    )   
    {
        #pragma HLS INTERFACE axis port=cmd_stream
        #pragma HLS INTERFACE axis port=vec_info_stream
        #pragma HLS INTERFACE axis port=cmd_to_mem_stream   
        // #pragma HLS INTERFACE axis port=data_to_mem_stream
        // #pragma HLS INTERFACE axis port=data_from_mem_stream
        #pragma HLS INTERFACE axis port=pma_addr_back_stream
        #pragma HLS INTERFACE m_axi port=pos_pma_addrs offset=slave bundle=gmem0
        #pragma HLS INTERFACE m_axi port=neg_pma_addrs offset=slave bundle=gmem1
        #pragma HLS INTERFACE m_axi port=hbm_ptr offset=slave bundle=gmem2
        #pragma HLS INTERFACE axis port=ack_back_to_router
        // #pragma HLS INTERFACE axis port=stop_signal
        int cmd_count = 0;
        PMA_UPDATE_MESSAGE cmd[MAX_CMD_NUM] = {0};
        // VEC_INFO pos_pma_addrs[MAX_PMA_NUM];//对于超大图这一部分可以放到HBM中，片上作为二级缓存
        // VEC_INFO neg_pma_addrs[MAX_PMA_NUM];
        int tmp_storge[SEGMENT_SIZE * MAX_TMP_PMA_NUM];
        const int TMP_BYTES = (int)sizeof(int) * SEGMENT_SIZE * MAX_TMP_PMA_NUM;
        bool direction = 0;//0 for out edge, 1 for in edge
        PMA_HEADER pma_headers; 
        // memset(pos_pma_addrs, 0, sizeof(VEC_INFO) * MAX_PMA_NUM);
        // memset(neg_pma_addrs, 0, sizeof(VEC_INFO) * MAX_PMA_NUM);
        while (true)
        {    
            //get batch update info vertex update + edge_update
            cmd_count = 0;
            HERE
            READ_COMMAND:
            for (int i = 0 ; i < MAX_CMD_NUM ; i++)
            {
                cmd[i].data = 0;
            }
            for (int i = 0 ; i < MAX_CMD_NUM; i++)
            {
                cmd[i] = cmd_stream.read();
                // HERE
                cmd_count++;
                if (cmd[i].last == 1)
                {
                    break;
                }
            }
            if (cmd_count == 1 && cmd[0].data == PMA_STOP)
            {
                HERE
                break;
            }
            int pma_idx = cmd[0].data.range(31,0) / VERTEX_PER_PMA;
            printf("PMA UPDATE KERNEL RECEIVED %d COMMANDS NO %d\n", cmd_count, pma_idx);
            for (int i = 0 ; i < cmd_count ; i++)
            {
                printf("WORK WITH COMMAND %d TYPE %d, SRC %d, DST %d, TAG %d\n",i, int(cmd[i].data.range(97,96)), int(cmd[i].data.range(31,0)), int(cmd[i].data.range(63,32)), int(cmd[i].data.range(123,100)));
            }
            // HERE
            direction = cmd[0].data.range(98,98);
            // read_header from HBM
            int header_start = 0;
            int pma_len_in_byte = 0;
            bool in_mem = false;
            if (direction == IN_DIRECTION)
            {
                if (neg_pma_addrs[pma_idx] != 0)
                {
                    // HERE
                    in_mem = true;
                    header_start = neg_pma_addrs[pma_idx].range(31,0);
                    pma_len_in_byte = neg_pma_addrs[pma_idx].range(63,32);
                }
            }
            else
            {
                if (pos_pma_addrs[pma_idx] != 0)
                {
                    // HERE
                    in_mem = true;
                    header_start = pos_pma_addrs[pma_idx].range(31,0);
                    pma_len_in_byte = pos_pma_addrs[pma_idx].range(63,32);
                }
            }  
            // HERE
            VEC_INFO_STREAM vec_info;
            if (in_mem)
            {
                memcpy(&pma_headers, hbm_ptr + header_start, sizeof(PMA_HEADER));
            }
            else  
            {
                INIT_HEADER:
                for (int i = 0 ; i < VERTEX_PER_PMA ; i++)
                {
                    #pragma HLS UNROLL
                    pma_headers.vertex_range[i] = UNUSED_ICON;
                    pma_headers.edge_count[i] = 0;
                    pma_headers.attr[i] = 0;
                }
            }
            //pre allocate memory 
            int extra_mem_needed = 0;
            int enlarged_segs[VERTEX_PER_PMA] = {0};
            memset(enlarged_segs, 0, sizeof(int) * VERTEX_PER_PMA);
            INSERT_VERTEX:
            for (int i = 0 ; i < cmd_count; i++)
            {
                if (cmd[i].data.range(97,96) == PMA_INSERT_VERTEX)
                {
                    // HERE
                    //allocate memory for vertex
                    extra_mem_needed += SEGMENT_SIZE * sizeof(int);
                    int src = cmd[i].data.range(31,0);
                    int local_src = src % VERTEX_PER_PMA;
                    // enlarged_segs[local_src]++;
                    pma_headers.edge_count[local_src] = 0;
                    pma_headers.attr[local_src] = 0;
                    if (local_src == 0)
                    {
                        pma_headers.vertex_range[local_src] = 1;
                    }
                    //强依赖前面一个顶点的信息， 必须按照顺序插入！！！
                    else  
                    {
                        pma_headers.vertex_range[local_src] = pma_headers.vertex_range[local_src - 1] + 1;
                    }

                    // printf("Vertex %d inserted, range %d\n", src, pma_headers.vertex_range[local_src]);
                }
            }
            // show_header(pma_headers);
            // HERE
            
            // HERE
            //read data from memory
            if (in_mem)
            {
                // printf("IN MEM READ\n");
                memset(tmp_storge, UNUSED_ICON, sizeof(int) * SEGMENT_SIZE * MAX_TMP_PMA_NUM);
                memcpy(tmp_storge, hbm_ptr + header_start + sizeof(PMA_HEADER), pma_len_in_byte - sizeof(PMA_HEADER));
            }
            else  
            {
                // printf("NOT IN MEM INIT\n");
                memset(tmp_storge, UNUSED_ICON, sizeof(int) * SEGMENT_SIZE * MAX_TMP_PMA_NUM);
            }
            if (in_mem)
            {
                vec_info.data.range(31,0) = header_start;
                vec_info.data.range(63,32) = pma_len_in_byte;
                // 顺序必须是 vec_info -> command，避免死锁/错配
                vec_info_stream.write(vec_info);
                COMMAND_STREAM cfree; cfree.data = MEM_FREE; cfree.last = 0;
                cmd_to_mem_stream.write(cfree);
            }
            //process data
            int new_pma_len = pma_len_in_byte + extra_mem_needed + sizeof(PMA_HEADER);
            int new_header_start = 0;
            vec_info.data.range(31,0) = new_header_start;
            vec_info.data.range(63,32) = new_pma_len;
            // 顺序必须是 vec_info -> command
            vec_info_stream.write(vec_info);
            COMMAND_STREAM calloc; calloc.data = MEM_ALLOC; calloc.last = 0;
            cmd_to_mem_stream.write(calloc);
            pma_addr_back_stream.read(vec_info);
            // HERE
            new_header_start = vec_info.data.range(31,0);
            if (direction == IN_DIRECTION)
            {
                // printf("IN DIRECTION\n");
                neg_pma_addrs[pma_idx].range(31,0) = new_header_start;
                neg_pma_addrs[pma_idx].range(63,32) = new_pma_len;
            }
            else
            {
                // printf("OUT DIRECTION\n");
                pos_pma_addrs[pma_idx].range(31,0) = new_header_start;
                pos_pma_addrs[pma_idx].range(63,32) = new_pma_len;
            }
            /*
                1. update pma header
                2. insert vertex done before
                3. insert edge
                4. rebalance  
            */
            //update pma header
            // HERE
            int total_extra_segs = 0;

            //insert edge 反向信息由调用者保证
            printf("INSERT EDGE FOR DIRECTION %d PMA IDX %d WORKER %d TOTAL CMDS %d\n ",direction, int(cmd[0].data.range(31,0) / VERTEX_PER_PMA), int(cmd[0].data.range(99,99)), int(cmd_count));
            INSERT_EDGE:
            for (int i = 0 ; i < cmd_count; i++)
            {
                if (cmd[i].data.range(97,96) == PMA_INSERT_EDGE)
                {
                    int src = cmd[i].data.range(31,0);
                    int dst = cmd[i].data.range(63,32);
                    printf("INSERT WITH COMMAND %d TYPE %d, SRC %d, DST %d, TAG %d, WORKER %d\n",i, int(cmd[i].data.range(97,96)), int(cmd[i].data.range(31,0)), int(cmd[i].data.range(63,32)), int(cmd[i].data.range(123,100)), int(cmd[i].data.range(99,99)));
                    insert_edge(src, dst, tmp_storge, pma_headers);
                }
            }


            STASTISTIC_ENLARGE_SEGS:
            for (int i = 0 ; i < VERTEX_PER_PMA ; i++)
            {
                if (pma_headers.vertex_range[i] == UNUSED_ICON)
                {
                    continue;
                }
                int max_edge;
                int seg_count;
                if (i == 0)
                {
                    seg_count = pma_headers.vertex_range[i];
                }
                else 
                {
                    seg_count = pma_headers.vertex_range[i] - pma_headers.vertex_range[i - 1];
                }
                max_edge = (seg_count * SEGMENT_SIZE) * DENSITY_BOUND;
                if (pma_headers.edge_count[i] > max_edge)
                {
                    extra_mem_needed += seg_count * SEGMENT_SIZE * sizeof(int);
                    enlarged_segs[i] += seg_count;
                }
            }
            ENLARGE_SEGS:
            for (int i = 0; i < VERTEX_PER_PMA; i++) 
            {
                // HERE
                if (pma_headers.vertex_range[i] == UNUSED_ICON) 
                {
                    break;
                }

                // Update the vertex range to include the original segments and the extra segments
                total_extra_segs += enlarged_segs[i];
                pma_headers.vertex_range[i] += total_extra_segs;
            }
            rebalance(tmp_storge, pma_headers);
            CHANGE_ATTR:
            for (int i = 0 ; i < cmd_count; i++)
            {
                if (cmd[i].data.range(97,96) == PMA_CHANGE_ATTR)
                {
                    int vertex = cmd[i].data.range(31,0);
                    double attr;
                    ap_uint<64> attr_bits = cmd[i].data.range(95,32);
                    memcpy(&attr, &attr_bits, sizeof(double));
                    int local_vertex = vertex % VERTEX_PER_PMA;
                    pma_headers.attr[local_vertex] = attr;
                }
            }
            memcpy(hbm_ptr + new_header_start, &pma_headers, sizeof(PMA_HEADER));
            memcpy(hbm_ptr + new_header_start + sizeof(PMA_HEADER), tmp_storge, new_pma_len - sizeof(PMA_HEADER));
            ack_back_to_router.write({.data = 1});
            printf("PMA UPDATE DONE FOR PMA IDX %d, DIRECTION %d\n", pma_idx, int(direction));
            // HERE
        }
    }
}