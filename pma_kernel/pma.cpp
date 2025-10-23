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
    int target_seg = low_seg - 1;

    out_idx = left - 1;
    out_val = -1;
    while (low_seg <= high_seg)
    {
        int mid = low_seg + (high_seg - low_seg) / 2;
        int seg_start_addr = mid * SEGMENT_SIZE;

        int minimum = INT_MAX;
        int maximum = INT_MIN;
        int max_idx = 0;
        bool find = false;
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
            target_seg = mid;
            find = true;
        }
        
        if (find) break;
    }
    return;
}
void insert_edge(int src, int dst, int* pma_data, PMA_HEADER& pma_header)
{
    int local_src = src % VERTEX_PER_PMA;
    int left = local_src == 0 ? 0 : pma_header.vertex_range[local_src - 1] * SEGMENT_SIZE;
    int right = pma_header.vertex_range[local_src] * SEGMENT_SIZE ;
    int out_idx, out_val;
    binary_search(dst, left, right, pma_data, out_idx, out_val);
    if (out_val == dst)
    {
        return;
    }
    int to_move_pos = -1;
    for (int i = out_idx + 1; i < pma_header.vertex_range[local_src] * SEGMENT_SIZE; i++)
    {
        #pragma HLS PIPELINE II=1
        #pragma HLS LOOP_TRIPCOUNT min=0 max=4096
        if (pma_data[i] == UNUSED_ICON)
        {
            to_move_pos = i;
            break;
        }
    }
    for (int i = to_move_pos; i > out_idx + 1; i--)
    {
        #pragma HLS PIPELINE II=1
        #pragma HLS LOOP_TRIPCOUNT min=0 max=4096
        pma_data[i] = pma_data[i - 1];
    }
    pma_data[out_idx+1] = dst;
    pma_header.edge_count[local_src]++;
}
void rebalance(int* pma_data, PMA_HEADER& pma_header)
{
    
    // for (int i = 0 ; i < VERTEX_PER_PMA; i++)
    // {
    //     // HERE
    //     int start = i == 0 ? 0 : pma_header.vertex_range[i - 1] * SEGMENT_SIZE;
    //     int end = pma_header.vertex_range[i] * SEGMENT_SIZE;
    //     int seg_count = (end - start) / SEGMENT_SIZE;
    //     // printf("REBALANCE VERTEX %d FROM %d TO %d, SEG COUNT %d\n", i, start, end, seg_count);
    //     if (seg_count == 0) continue;
    //     int pack_ptr = start;
    //     for (int j = start ; j < end ; j++)
    //     {
    //         if (pma_data[j] != UNUSED_ICON)
    //         {
    //             pma_data[pack_ptr++] = pma_data[j];
    //         }
    //     }
    //     // HERE
    //     int edge_num = (pack_ptr - start);
    //     int base = edge_num / seg_count;
    //     int extra = edge_num % seg_count;
    //     if (edge_num == 0) continue;
    //     // HERE
    //     for (int s = seg_count - 1 ; s >= 0 ; s--)
    //     {
    //         int cur_seg_valid = base + (s < extra ? 1 : 0);
    //         int cur_seg_start = start + s * SEGMENT_SIZE;
    //         for (int k = cur_seg_start + cur_seg_valid -1 ; k >= cur_seg_start ; k--)
    //         {
    //             --pack_ptr;
    //             int tmp = pma_data[pack_ptr];
    //             pma_data[pack_ptr] = UNUSED_ICON;
    //             pma_data[k] = tmp;
    //         }
    //     }
    // }
    int end = pma_header.vertex_range[VERTEX_PER_PMA - 1] * SEGMENT_SIZE;
    int pack_ptr = end;
    //把所有数据放在最后
    for (int i = end - 1 ; i >= 0 ; i--)
    {
        if (pma_data[i] != UNUSED_ICON)
        {
            pack_ptr--;
            pma_data[pack_ptr] = pma_data[i];
            pma_data[i] = UNUSED_ICON;
        }
    }
    for (int i = 0 ; i < VERTEX_PER_PMA ; i++)
    {
        int seg_start = (i == 0) ? 0 : pma_header.vertex_range[i - 1] * SEGMENT_SIZE;
        int seg_end = pma_header.vertex_range[i] * SEGMENT_SIZE;
        int seg_count = (seg_end - seg_start) / SEGMENT_SIZE;
        if (seg_count == 0) continue;
        int edge_num = pma_header.edge_count[i];
        int base = edge_num / seg_count;
        int extra = edge_num % seg_count;
        if (edge_num == 0) continue;
        for (int s = 0 ; s < seg_count ; s++)
        {
            int cur_seg_valid = base + (s < extra ? 1 : 0);
            int cur_seg_start = seg_start + s * SEGMENT_SIZE;
            for (int k = cur_seg_start; k < cur_seg_start + cur_seg_valid; k++)
            {
                int tmp = pma_data[pack_ptr];
                pma_data[pack_ptr] = UNUSED_ICON;
                ++pack_ptr;
                pma_data[k] = tmp;
            }
        }
    }
}
