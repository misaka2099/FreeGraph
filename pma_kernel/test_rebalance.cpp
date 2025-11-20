#include <cstring>
#include <iostream>
#include <thread>
#include "data_type.h"
int storage[20*SEGMENT_SIZE];
// PMA_HEADER old_header;
PMA_HEADER new_header;
void init()
{
    for (int i = 0 ; i < 16 ; i++)
    {
        // old_header.vertex_range[i] = i;
        new_header.vertex_range[i] = i + 1;
        // old_header.edge_count[i] = 48;
        new_header.edge_count[i] = 48;
    }
    for (int i = 0 ; i < 16 ; i++)
    {
        for (int j = 0 ; j < 48 ; j++)
        {
            storage[i*SEGMENT_SIZE+j] = j;
        }
        for (int k = 48 ; k < SEGMENT_SIZE ; k++)
        {
            storage[i*SEGMENT_SIZE+k] = UNUSED_ICON;
        }
    }
}
void print_pma()
{
    for (int i = 0 ; i < 16 ; i++)
    {
        int s = (i == 0)?  0: new_header.vertex_range[i-1];
        int e = new_header.vertex_range[i];
        std::cout << "VERTEX " << i << std::endl;
        for (int j = s*SEGMENT_SIZE; j < e*SEGMENT_SIZE ; j++)
        {
            std::cout << storage[j] << " ";
        }
        std::cout << std::endl;
    }
}


int valid_count(int seg_start, int seg_end)
{
    int total_count = 0;
    for (int i = seg_start * SEGMENT_SIZE ; i < seg_end * SEGMENT_SIZE ; i++)
    {
        if (storage[i] != UNUSED_ICON)
        total_count++;
    }
    return total_count;
}
void redistribute(int left, int right)
{
    if (left + 1 == right) return;
    //pack_phase
    int seg_count = right - left;
    int pack_ptr = left * SEGMENT_SIZE;
    for (int i = left * SEGMENT_SIZE ; i < right * SEGMENT_SIZE ; i++)
    {
        if (storage[i] != UNUSED_ICON)
        {
            int tmp = storage[i];
            storage[i] = UNUSED_ICON;
            storage[pack_ptr] = tmp;
            pack_ptr++;
        }
    }
    int count = pack_ptr - left * SEGMENT_SIZE;
    int base = count / seg_count;
    int extra_seg = count % seg_count;
    int sparse_ptr = pack_ptr - 1;
    //sparse phase
    for (int s = right - 1 ; s >= left ; s--)
    {
        int to_fill = base + ((s - left < extra_seg)? 1: 0);
        for (int j = to_fill + s *SEGMENT_SIZE - 1; j >= s*SEGMENT_SIZE ; j--)
        {
            int tmp = storage[sparse_ptr];
            storage[sparse_ptr] = UNUSED_ICON;
            storage[j] = tmp;
            sparse_ptr--;
        }
    }
}
void rebalance(int src, int insert_pos)
{
    int s = (src == 0) ? 0 : new_header.vertex_range[src-1];
    int e = new_header.vertex_range[src];
    int seg_start = s;
    int seg_end = e;
    int seg_num = seg_end - seg_start; // 当前vertex的segment数


    int seg_idx = (insert_pos / SEGMENT_SIZE); // 插入点所在segment编号

    for (int cur_seg_num = 1 ; cur_seg_num <= seg_num ; cur_seg_num *= 2)
    {
        int left = seg_start + int(seg_idx / cur_seg_num) * cur_seg_num;
        int right = seg_start + (int(seg_idx / cur_seg_num) + 1) * cur_seg_num;
        int valid = valid_count(left, right);
        int max_edge = (right - left) * SEGMENT_SIZE * DENSITY_BOUND; 
        if (valid <= max_edge)
        {
            redistribute(left, right);
            return;
        }
    }
}
void enlarge_seg(int src)
{
    int old_start = (src == 0) ? 0 : new_header.vertex_range[src-1];
    int old_end = new_header.vertex_range[src];
    int old_seg_num = old_end - old_start;
    int new_seg_num = old_seg_num * 2;
    int new_end = old_start + new_seg_num;
    int move_count = (new_header.vertex_range[15] - old_end) * SEGMENT_SIZE;
    int move_bytes = move_count * sizeof(int);
    if (move_count > 0) {
        memmove(
            storage + new_end * SEGMENT_SIZE,
            storage + old_end * SEGMENT_SIZE,
            move_bytes
        );
    }
    for (int i = 15; i > src; i--) {
        new_header.vertex_range[i] += new_seg_num - old_seg_num;
    }
    new_header.vertex_range[src] = new_end;
    for (int i = old_end * SEGMENT_SIZE; i < new_end * SEGMENT_SIZE; i++) {
        storage[i] = UNUSED_ICON;
    }
}
void insert_edge(int src, int dst)
{
    int s = (src == 0)? 0 : new_header.vertex_range[src-1];
    int e = new_header.vertex_range[src];
    int pos = s * SEGMENT_SIZE;
    for (int i = s * SEGMENT_SIZE ; i < e * SEGMENT_SIZE ; i++)
    {
        if (storage[i] == UNUSED_ICON)
        {
            continue;
        }
        if (storage[i] == dst)
        {
            return;
        }
        else if (storage[i] > dst)
        {
            break;
        }
        else if (storage[i] < dst)
        {
            pos = i;
        }
    }
    for (int j = (pos + SEGMENT_SIZE - 1) / SEGMENT_SIZE * SEGMENT_SIZE - 1 ; j > pos + 1; j--)
    {
        storage[j] = storage[j-1];
    }
    storage[pos + 1] = dst;
    new_header.edge_count[src]++;
    int max_edge = (new_header.vertex_range[src] - (src == 0 ? 0 : new_header.vertex_range[src-1])) * DENSITY_BOUND * SEGMENT_SIZE;
    if (new_header.edge_count[src] > max_edge)
    {
        enlarge_seg(src);
    }
    rebalance(src, pos);
}


int main()
{
    init();
    insert_edge(0, 1000);
    insert_edge(0, 2000);
    for (int i = 0 ; i < 30 ; i++)
    {
        insert_edge(0, i+1000);
    }
    print_pma();
    return 0;
}