#include "data_type.h"
#include "debug.h"
#include "hls_stream.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
struct seg_node
{
    int left;
    int right;
    int left_max;
    int right_max;
    int max_free;
    int lazy; // -1: no op, 0: set to used, 1: set to free
};

// const int MAX_LEN = 64*1024*1024;
// const int UNIT_BYTE = 1024;
const int MAX_DEPTH = (128);
void push_down(int node_idx, seg_node* storage)
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
void push_up(int node_idx, seg_node* storage)
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
void modify_range(int l, int r, int val, seg_node* storage)
{
    stack_frame frame_stack[MAX_DEPTH];
    int top = 0;
    // 初始将根节点任务压栈
    frame_stack[top++] = {l, r, 1, 1}; // 使用 top++ 更符合常规栈操作

    while (top > 0)
    {
        // 取出栈顶帧，注意这里是值拷贝，因为我们不需要修改栈中内容，而是决定下一步
        stack_frame current = frame_stack[--top]; 
        int cur_l = current.l;
        int cur_r = current.r;
        int cur_node = current.cur_node;
        int cur_stage = current.stage;

        // stage 1: 向下递归（入口阶段）
        if (cur_stage == 1)
        {
            // 检查：当前节点区间与目标修改区间是否无交集
            if (cur_r < storage[cur_node].left || cur_l > storage[cur_node].right)
            {
                continue; // 无交集，直接忽略此分支
            }

            // 检查：当前节点区间是否被目标修改区间完全覆盖
            if (cur_l <= storage[cur_node].left && cur_r >= storage[cur_node].right)
            {
                // 完全覆盖，直接应用 lazy tag 并停止下探
                if (val == 1)
                {
                    storage[cur_node].lazy = 1;
                    storage[cur_node].left_max = storage[cur_node].right_max = storage[cur_node].max_free = storage[cur_node].right - storage[cur_node].left + 1;
                }
                else // val == 0
                {
                    storage[cur_node].lazy = 0;
                    storage[cur_node].left_max = storage[cur_node].right_max = storage[cur_node].max_free = 0;
                }
                continue; // 不再深入
            }

            // 部分覆盖，需要继续下探
            // 1. 先下推当前节点的 lazy tag
            if (storage[cur_node].lazy != -1)
            {
                push_down(cur_node, storage);
            }

            // 2. 将当前节点的 push_up 任务（回溯帧）压栈
            frame_stack[top++] = {0, 0, cur_node, 2}; // l, r 无用，设为0

            // 3. 将子节点的处理任务压栈
            int mid = (storage[cur_node].left + storage[cur_node].right) / 2;

            // 先压右子节点，后压左子节点，保证左边先被处理
            if (cur_r > mid) { // 如果目标区间与右子节点有交集
                frame_stack[top++] = {cur_l, cur_r, cur_node * 2 + 1, 1};
            }
            if (cur_l <= mid) { // 如果目标区间与左子节点有交集
                frame_stack[top++] = {cur_l, cur_r, cur_node * 2, 1};
            }
        }
        // stage 2: 向上合并（出口阶段）
        else  
        {
            push_up(cur_node, storage);
        }
    }
}

void alloc(int len, int &start, seg_node* storage)
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
        // 叶子节点保护：避免访问不存在的子节点
        if (storage[cur_idx].left == storage[cur_idx].right) {
            if (len == 1 && storage[cur_idx].max_free >= 1) {
                start = storage[cur_idx].left;
                complete = true;
            } else {
                // 叶子无法满足更长的连续块
                break;
            }
            continue;
        }
        push_down(cur_idx , storage);
        if (storage[cur_idx * 2].max_free >= len)
        {
            cur_idx = cur_idx * 2;
        }
        else if (storage[cur_idx * 2 + 1].max_free >= len)
        {
            cur_idx = cur_idx * 2 + 1;
        }
        else {
            int cross = storage[2 * cur_idx].right_max + storage[cur_idx * 2 + 1].left_max;
            if (cross >= len)
            {
                start = storage[cur_idx * 2].right - storage[cur_idx * 2].right_max + 1;
                complete = true;
                break; // Exit loop once allocation is found
            }
            else
            {
                // 状态不一致，避免死循环
                printf("THIS SHOULD NEVER BE REACHED\n");
                break;
            }
        }
    }   
    if (complete)
    {
        modify_range(start, start + len - 1, 0, storage);
    }
}
void dealloc(int start, int len, seg_node* storage)
{
    modify_range(start, start + len - 1, 1, storage);    
}
//[]
void init(int len, seg_node* storage)
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
        storage[i].lazy = 1;
    }
}

extern "C"
{
    void alloc_free_kernel(
        hls::stream<COMMAND_STREAM>& cmd,
        hls::stream<VEC_INFO_STREAM>& vec_info_in,
        hls::stream<VEC_INFO_STREAM>& vec_info_back
    )
    {
        #pragma HLS INTERFACE axis port=cmd
        #pragma HLS INTERFACE axis port=vec_info_in
        #pragma HLS INTERFACE axis port=vec_info_back
        
        seg_node storage[(CHANNEL_CAPACITY * CHANNEL_NUM / INIT_VEC_LEN_IN_BYTE) * 2 + 10];
        #pragma HLS bind_storage variable=storage type=ram_t2p impl=uram
        memset(storage, 0, sizeof(storage));
        // 初始化
        init(CHANNEL_CAPACITY * CHANNEL_NUM / INIT_VEC_LEN_IN_BYTE, storage);
        
        while (true)
        {
            // 非阻塞获取 vec_info，拿到后再去阻塞读取 cmd，避免跨流死锁
            VEC_INFO_STREAM pending_info;
            bool has_info = false;

            if (!has_info) {
                VEC_INFO_STREAM tmp;
                if (vec_info_in.read_nb(tmp)) {
                    if (tmp.last == 1) {
                        break;
                    }
                    HERE
                    pending_info = tmp;
                    has_info = true;
                } else {
                    // 还没有 vec 信息，先继续尝试读取
                    continue;
                }
            }

            // 到这里说明已有 vec_info，可安全阻塞读 cmd
            COMMAND_STREAM command = cmd.read();

            VEC_INFO back_info = 0;
            if (command.data == MEM_ALLOC)
            {
                int req_bytes = pending_info.data.range(63,32);
                int len_segs = (req_bytes + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
                int start_seg;
                HERE
                alloc(len_segs, start_seg, storage);
                HERE
                // 返回按字节的起始地址与长度（失败时返回 0xFFFFFFFF 作为哨兵）
                if (start_seg == -1)
                {
                    perror("ERROR INSUFFICIENT MEMORY POOL\n");
                }
                int start_bytes;
                // if (start_seg < 0) {
                //     start_bytes = 0xFFFFFFFF;
                // } else {
                    start_bytes = start_seg * INIT_VEC_LEN_IN_BYTE;
                // }
                back_info.range(31,0) = start_bytes;
                back_info.range(63,32) = req_bytes;
            }
            else if (command.data == MEM_FREE)
            {
                int start_bytes = pending_info.data.range(31,0);
                int len_bytes = pending_info.data.range(63,32);
                int start_seg = start_bytes / INIT_VEC_LEN_IN_BYTE;
                int len_segs  = (len_bytes + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
                // 边界保护，避免越界
                // const int TOTAL_SEGS = CHANNEL_CAPACITY * CHANNEL_NUM / INIT_VEC_LEN_IN_BYTE;
                // if (start_bytes != 0xFFFFFFFF && start_seg >= 0 && len_segs > 0 && (start_seg + len_segs - 1) < TOTAL_SEGS) {
                HERE
                dealloc(start_seg, len_segs, storage);
                HERE
                // } else {
                //     printf("WARNING: skip free: start_bytes=%d len_bytes=%d (start_seg=%d len_segs=%d TOTAL_SEGS=%d)\n",
                //            start_bytes, len_bytes, start_seg, len_segs, TOTAL_SEGS);
                // }
                back_info = 0;
            }
            VEC_INFO_STREAM out; out.data = back_info; out.last = 0;
            vec_info_back.write(out);
            HERE
            // 清空本次缓存，处理下一对
            has_info = false;
        }
    }
}