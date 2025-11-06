#include "data_type.h"
#include <cstdio>
#include <sstream>
#include "pma.h"
#include "debug.h"
//[)
//left and right stand for seg idx
void binary_search(int val, int left, int right, int *storage, int &out_idx, int& out_val)
{


    int low_seg = left / SEGMENT_SIZE;
    int high_seg = (right) / SEGMENT_SIZE - 1;

    out_idx = left - 1;
    out_val = -1;
    BINARY_SEARCH:
    while (low_seg <= high_seg)
    {
        int mid = low_seg + (high_seg - low_seg) / 2;
        int seg_start_addr = mid * SEGMENT_SIZE;

        int minimum = INT_MAX;
        int maximum = INT_MIN;
        int max_idx = 0;
        bool find = false;
        INTRA_SEG_ITERATION:
        for (int i = 0 ; i < SEGMENT_SIZE ; i++)
        {
            #pragma HLS PIPELINE II=1
            #pragma HLS LOOP_TRIPCOUNT min=0 max=64
            // 使用局部索引 i
            if (storage[i+mid*SEGMENT_SIZE] != UNUSED_ICON)
            {
                int storage_val = storage[i + seg_start_addr];
                maximum = std::max(maximum, storage_val);
                minimum = std::min(minimum, storage_val);
                if (storage_val == val)
                { 
                    out_idx = mid * SEGMENT_SIZE + i;
                    out_val = val;
                    return;
                }
                if (storage_val < val)
                {
                    out_idx = mid * SEGMENT_SIZE + i;
                    out_val = storage_val;
                }
                else if (storage_val > val)
                { 
                    break; 
                }
            }
        }

        if (maximum < val)
        {
            low_seg = mid + 1;
        }
        if (minimum > val)
        {
            high_seg = mid - 1;
        }
        if (minimum <= val && maximum >= val)
        {
            find = true;
        }
        
        if (find) break;
    }
    return;
}
void insert_edge(int dst, int* pma_data, PMA_HEADER& pma_header, int find_pos)
{
    // int local_src = src % VERTEX_PER_PMA;
    // int left = local_src == 0 ? 0 : pma_header.vertex_range[local_src - 1] * SEGMENT_SIZE;
    // int right = pma_header.vertex_range[local_src] * SEGMENT_SIZE ;
    // int out_idx, out_val;
    // binary_search(dst, left, right, pma_data, out_idx, out_val);
    // if (out_val == dst)
    // {
    //     HERE
    //     printf("Edge %d -> %d already exists. Skipping insertion.\n", src, dst);
    //     return;
    // }


    const int BITS_PER_SEG = SEGMENT_SIZE * 32;  // 2048
    int seg_idx = find_pos / SEGMENT_SIZE;
    int seg_start = seg_idx * SEGMENT_SIZE;
    int insert_offset = find_pos - seg_start;

    ap_uint<BITS_PER_SEG> seg_data = 0;
    LOAD_SEG:
    for (int i = 0; i < SEGMENT_SIZE; i++)
    {
        #pragma HLS PIPELINE II=1
        seg_data.range((i+1)*32 - 1, i*32) = pma_data[seg_start + i];
    }

    int shift_start = insert_offset + 1;  // 从插入后第一个位置开始移
    ap_uint<BITS_PER_SEG> shifted_data = seg_data;
    ap_uint<BITS_PER_SEG> mask = (ap_uint<BITS_PER_SEG>(1) << (insert_offset * 32)) - 1;
    seg_data &= mask;
    shifted_data &= ~mask;
    shifted_data <<= 32;
    seg_data |= shifted_data;
    seg_data.range((insert_offset + 1) * 32 + 31, (insert_offset + 1) * 32) = dst;
}
void rebalance(int* pma_data, PMA_HEADER& pma_header)
{
    int end = -1;
    FIND_LAST:
    for (int i = 0 ; i < VERTEX_PER_PMA ; i++)
    {
        #pragma HLS PIPELINE II = 1
        if (pma_header.vertex_range[i] == UNUSED_ICON)
        {
            break;
        }
        end = pma_header.vertex_range[i] * SEGMENT_SIZE;
    }
    if (end == -1)  
    {
        return;
    }
    int pack_ptr = end;
    //把所有数据放在最后
    
    REBALANCE_PACK:
    for (int i = end - 1 ; i >= 0 ; i--)
    {
        #pragma HLS PIPELINE II = 1
        if (pma_data[i] != UNUSED_ICON)
        {
            pack_ptr--;
            pma_data[pack_ptr] = pma_data[i];
            pma_data[i] = UNUSED_ICON;
        }
    }
    REBALANCING:
    for (int i = 0 ; i < VERTEX_PER_PMA ; i++)
    {
        if (pma_header.vertex_range[i] == UNUSED_ICON)
        {
            break;
        }
        int seg_start = (i == 0) ? 0 : pma_header.vertex_range[i - 1] * SEGMENT_SIZE;
        int seg_end = pma_header.vertex_range[i] * SEGMENT_SIZE;
        int seg_count = (seg_end - seg_start) / SEGMENT_SIZE;
        if (seg_count == 0) continue;
        int edge_num = pma_header.edge_count[i];
        int base = edge_num / seg_count;
        int extra = edge_num % seg_count;
        if (edge_num == 0) continue;
        REBALANCE_SEG_AUX:
        #pragma HLS LOOP_TRIPCOUNT min=0 max=64
        for (int s = 0 ; s < seg_count ; s++)
        {
            int cur_seg_valid = base + (s < extra ? 1 : 0);
            int cur_seg_start = seg_start + s * SEGMENT_SIZE;
            for (int k = cur_seg_start; k < cur_seg_start + cur_seg_valid; k++)
            {
                #pragma HLS PIPELINE II = 1
                int tmp = pma_data[pack_ptr];
                pma_data[pack_ptr] = UNUSED_ICON;
                ++pack_ptr;
                pma_data[k] = tmp;
            }
        }
    }
    
}
