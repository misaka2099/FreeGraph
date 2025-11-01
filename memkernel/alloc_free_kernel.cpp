#include "data_type.h"
#include "hls_stream.h"
#include <algorithm>
struct seg_node
{
    int left;
    int right;
    int left_max;
    int right_max;
    int max_free;
    int lazy; // -1: no op, 0: set to used, 1: set to free
};

const int MAX_LEN = 64*1024*1024;
const int UNIT_BYTE = 1024;
const int MAX_DEPTH = (64);
seg_node storage[(MAX_LEN / UNIT_BYTE) * 2 + 10];
void push_down(int node_idx)
{
    if (storage[node_idx].lazy == -1) return; // no op
    int val = storage[node_idx].lazy; // 使用当前节点的 lazy 标记

    if (storage[node_idx].left == storage[node_idx].right) {
        storage[node_idx].lazy = -1;
        return;
    }


    if (val == 1)
    {
        storage[node_idx * 2].lazy = 1;
        storage[node_idx * 2 ].left_max = storage[node_idx * 2].right_max = storage[node_idx * 2].max_free = storage[node_idx * 2].right - storage[node_idx * 2].left + 1; 
        storage[node_idx * 2 + 1].lazy = 1;
        storage[node_idx * 2 + 1].left_max = storage[node_idx * 2 + 1].right_max = storage[node_idx * 2 + 1].max_free = storage[node_idx * 2 + 1].right - storage[node_idx * 2 + 1].left + 1;
    }
    else if (val == 0)
    {
        storage[node_idx * 2].lazy = 0;
        storage[node_idx * 2 ].left_max = storage[node_idx * 2].right_max = storage[node_idx * 2].max_free = 0; 
        storage[node_idx * 2 + 1].lazy = 0;
        storage[node_idx * 2 + 1].left_max = storage[node_idx * 2 + 1].right_max = storage[node_idx * 2 + 1].max_free = 0;
    }
    storage[node_idx].lazy = -1;
}
void push_up(int node_idx)
{
    storage[node_idx].left_max = storage[node_idx * 2].left_max;
    if (storage[node_idx * 2].left_max == storage[node_idx * 2].right - storage[node_idx * 2].left + 1)
        storage[node_idx].left_max += storage[node_idx * 2 + 1].left_max;
    storage[node_idx].right_max = storage[node_idx * 2 + 1].right_max;
    if (storage[node_idx * 2 + 1].right_max == storage[node_idx * 2 + 1].right - storage[node_idx * 2 + 1].left + 1)
        storage[node_idx].right_max += storage[node_idx * 2].right_max;
    int mid_max = storage[node_idx * 2].right_max + storage[node_idx * 2 + 1].left_max;
    storage[node_idx].max_free = std::max(storage[node_idx * 2].max_free, storage[node_idx * 2 + 1].max_free);
    storage[node_idx].max_free = std::max(storage[node_idx].max_free, mid_max);
    // storage[node_idx].lazy = -1;
}   
struct stack_frame
{
    int l;
    int r;
    int cur_node;
    int stage;
};
void modify_range(int l, int r, int val)
{
    stack_frame frame_stack[MAX_DEPTH];
    int top = 0;
    top++;
    frame_stack[top] = {l, r, 1, 1};
    
    while (top != 0)
    {
        stack_frame &current = frame_stack[top];
        int cur_l = current.l;
        int cur_r = current.r;
        int cur_node = current.cur_node;
        int cur_stage = current.stage;
        if (cur_stage == 1)
        {
            if (cur_l == storage[cur_node].left && cur_r == storage[cur_node].right)
            {
                if (val == 1)
                {
                    storage[cur_node].lazy = 1;
                    storage[cur_node].left_max = storage[cur_node].right_max = storage[cur_node].max_free = storage[cur_node].right - storage[cur_node].left + 1;
                }
                else if (val == 0)
                {
                    storage[cur_node].lazy = 0;
                    storage[cur_node].left_max = storage[cur_node].right_max = storage[cur_node].max_free = 0;
                }
                top--;
                continue;
            }
            if (storage[cur_node].lazy != -1)
            {
                push_down(cur_node);
            }
            int mid = (storage[cur_node].left + storage[cur_node].right) / 2;
            current.stage = 2;
            if (cur_r <= mid)
            {
                frame_stack[top].stage = 2;
                top++;
                frame_stack[top] = {cur_l, cur_r, cur_node * 2, 1};
            }
            else if (cur_l > mid)
            {
                frame_stack[top].stage = 2;
                top++;
                frame_stack[top] = {cur_l, cur_r, cur_node * 2 + 1, 1};
            }
            else  
            {
                frame_stack[top].stage = 2;
                top++;
                frame_stack[top] = {cur_l, mid, cur_node * 2, 1};
                top++;
                frame_stack[top] = {mid + 1, cur_r, cur_node * 2 + 1, 1};
            }
        }
        else  
        {
            push_up(cur_node);
            top--;
        }
    }
}
void alloc(int len, int &start)
{
    int cur_idx = 1;
    bool complete = false;
    start = -1;
    while (!complete)
    {
        if (storage[cur_idx].max_free < len)
        {
            break;
        }
        push_down(cur_idx );
        if (storage[cur_idx * 2].max_free >= len)
        {
            cur_idx = cur_idx * 2;
        }
        else if (storage[cur_idx * 2 + 1].max_free >= len)
        {
            cur_idx = cur_idx * 2 + 1;
        }
        else if (storage[2 * cur_idx].right_max + storage[cur_idx * 2 + 1].left_max >= len)
        {
            start = storage[cur_idx * 2].right - storage[cur_idx * 2].right_max + 1;
            complete = true;
        }
    }   
    if (complete)
    {
        modify_range(start, start + len - 1, 0);

        int temp_idx = cur_idx;
        while (temp_idx > 1) {
            temp_idx /= 2;
            push_up(temp_idx);
        }
    }
}
void dealloc(int start, int len)
{
    modify_range(start, start + len - 1, 1);    
}
//[]
void init(int len)
{
    storage[1].left = 0;
    storage[1].right = len - 1;
    storage[1].left_max = len;
    storage[1].right_max = len;
    storage[1].max_free = len;
    storage[1].lazy = -1;
    for (int i = 2 ; i <= len * 2 - 1 ; i++)
    {
        int father_left = storage[i / 2].left;
        int father_right = storage[i / 2].right;
        int mid = (father_left + father_right) / 2;
        if (i % 2 == 0) { // 左子节点
            storage[i].left = father_left;
            storage[i].right = mid;
        } else { // 右子节点
            storage[i].left = mid + 1;
            storage[i].right = father_right;
        }
        storage[i].left_max = storage[i].right_max = storage[i].max_free = storage[i].right - storage[i].left + 1;
        storage[i].lazy = -1;
    }
}

extern "C"
{
    void alloc_free_kernel(
        hls::stream<COMMAND_STREAM> cmd,
        hls::stream<VEC_INFO_STREAM> vec_info_in,
        hls::stream<VEC_INFO_STREAM> vec_info_back
    )
    {
        #pragma HLS INTERFACE axis port=cmd
        #pragma HLS INTERFACE axis port=vec_info_in
        #pragma HLS INTERFACE axis port=vec_info_back
        init(CHANNEL_CAPACITY * CHANNEL_NUM / INIT_VEC_LEN_IN_BYTE);
        while (true)
        {
            auto command = cmd.read();
            auto vec_info = vec_info_in.read();
            VEC_INFO back_info;
            if (command.data == MEM_ALLOC)
            {
                int request_len = vec_info.data.range(63,32);
                int start;
                alloc(request_len, start);
                back_info.range(31,0) = start;
                back_info.range(63,32) = request_len;
            }
            else if (command.data == MEM_FREE)
            {
                int start = vec_info.data.range(31,0);
                int len = vec_info.data.range(63,32);
                dealloc(start, len);
                back_info= 0;
            }
            vec_info_back.write({.data=back_info});
        }
    }
}