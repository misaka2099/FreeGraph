#include "data_type.h"
#include "debug.h"
#include "hls_stream.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
void set(int start, int len, BYTE* bitmap)
{
    for (int i = start; i < start + len; i++)
    {
        bitmap[i / 8] |= ((BYTE)1 << (i % 8));
    }
}


bool is_free(int start, int len, int &first_set , BYTE* bitmap)
{
    for (int i = start; i < start + len; i++)
    {
        if (bitmap[i / 8] & (1 << (i % 8)))
        {
            first_set = i;
            return false;
        }
    }
    return true;
} 
void allocate(int &start, int len, BYTE* bitmap)
{
    start = -1;
    if (len == 0) return;
    int first_set = -1;
    for (int i = 0 ; i < CHANNEL_CAPACITY * CHANNEL_NUM ; i++)
    {
        if (is_free(i, len, first_set, bitmap))
        {
            start = i;
            set(start, len, bitmap);
            return;
        }
        else
        {
            i = first_set;
        }
    }
}


void reset(int start, int len, BYTE* bitmap)
{
    for (int i = start; i < start + len; i++)
    {
        bitmap[i / 8] &= ~((BYTE)1 << (i % 8));
    }
}
void deallocate(int start, int len, BYTE* bitmap)
{
    if (len == 0) return;
    reset(start, len, bitmap);
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
        
        BYTE storage[(CHANNEL_CAPACITY / INIT_VEC_LEN_IN_BYTE)];
        #pragma HLS bind_storage variable=storage type=ram_t2p impl=uram
        memset(storage, 0, sizeof(storage));
        // 初始化
        // init(CHANNEL_CAPACITY * CHANNEL_NUM / INIT_VEC_LEN_IN_BYTE, storage);
        
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
                    // HERE
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
                // HERE
                allocate(start_seg, len_segs , storage);
                // HERE
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
                // HERE
                deallocate(start_seg, len_segs, storage);
                // HERE
                // } else {
                //     printf("WARNING: skip free: start_bytes=%d len_bytes=%d (start_seg=%d len_segs=%d TOTAL_SEGS=%d)\n",
                //            start_bytes, len_bytes, start_seg, len_segs, TOTAL_SEGS);
                // }
                back_info = 0;
            }
            VEC_INFO_STREAM out; out.data = back_info; out.last = 0;
            vec_info_back.write(out);
            // HERE
            // 清空本次缓存，处理下一对
            has_info = false;
        }
    }
}