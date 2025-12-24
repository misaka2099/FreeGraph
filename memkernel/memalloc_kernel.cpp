#include <cstdio>
#include <hls_stream.h>
#include "data_type.h"
#include "debug.h"

#define MAX_WORKERS 4
const int TOTAL_BLOCKS = (CHANNEL_CAPACITY / INIT_VEC_LEN_IN_BYTE * CHANNEL_NUM );

void set(int start, int len, BYTE* bitmap)
{
    #pragma HLS INLINE off
    for (int i = start; i < start + len; i++)
    {
        bitmap[i / 8] |= ((BYTE)1 << (i % 8));
    }
}


bool is_free(int start, int len, int &first_set , BYTE* bitmap)
{
    #pragma HLS INLINE off
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
    #pragma HLS INLINE off
    start = -1;
    if (len == 0) return;
    int first_set = -1;
    for (int i = 0 ; i < TOTAL_BLOCKS ; i++)
    {
        #pragma HLS LOOP_TRIPCOUNT min=1 max=TOTAL_BLOCKS
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
    #pragma HLS INLINE off
    for (int i = start; i < start + len; i++)
    {
        bitmap[i / 8] &= ~((BYTE)1 << (i % 8));
    }
}
void deallocate(int start, int len, BYTE* bitmap)
{
    #pragma HLS INLINE off
    if (len == 0) return;
    reset(start, len, bitmap);
}


extern "C"
{
    void memalloc_kernel(
        hls::stream<COMMAND_STREAM> &cmd_from_app_1,
        hls::stream<VEC_INFO_STREAM> &vec_info_from_app_1,
        hls::stream<VEC_INFO_STREAM> &vec_info_to_app_1,

        hls::stream<COMMAND_STREAM> &cmd_from_app_2,
        hls::stream<VEC_INFO_STREAM> &vec_info_from_app_2,
        hls::stream<VEC_INFO_STREAM> &vec_info_to_app_2,

        
        hls::stream<COMMAND_STREAM> &cmd_from_app_3,
        hls::stream<VEC_INFO_STREAM> &vec_info_from_app_3,
        hls::stream<VEC_INFO_STREAM> &vec_info_to_app_3,

        hls::stream<COMMAND_STREAM> &cmd_from_app_4,
        hls::stream<VEC_INFO_STREAM> &vec_info_from_app_4,
        hls::stream<VEC_INFO_STREAM> &vec_info_to_app_4,

        // hls::stream<VEC_INFO_STREAM> &vec_info_to_mem,
        // hls::stream<COMMAND_STREAM> &cmd_to_mem,
        // hls::stream<VEC_INFO_STREAM> &vec_info_from_mem,


        hls::stream<COMMAND_STREAM> &signal,

        BYTE* bm_in_hbm
    )
    {
        #pragma HLS INTERFACE axis port=cmd_from_app_1
        #pragma HLS INTERFACE axis port=vec_info_from_app_1
        #pragma HLS INTERFACE axis port=vec_info_to_app_1
        #pragma HLS INTERFACE axis port=cmd_from_app_2
        #pragma HLS INTERFACE axis port=vec_info_from_app_2
        #pragma HLS INTERFACE axis port=vec_info_to_app_2

        
        #pragma HLS INTERFACE axis port=cmd_from_app_3
        #pragma HLS INTERFACE axis port=vec_info_from_app_3
        #pragma HLS INTERFACE axis port=vec_info_to_app_3
        #pragma HLS INTERFACE axis port=cmd_from_app_4
        #pragma HLS INTERFACE axis port=vec_info_from_app_4
        #pragma HLS INTERFACE axis port=vec_info_to_app_4

        #pragma HLS INTERFACE m_axi port=bm_in_hbm offset=slave bundle=gmem0 
        // #pragma HLS INTERFACE axis port=vec_info_to_mem
        // #pragma HLS INTERFACE axis port=cmd_to_mem
        // #pragma HLS INTERFACE axis port=vec_info_from_mem
        #pragma HLS INTERFACE axis port=signal
        // bool alloc_app_1 = false;
        // bool alloc_app_2 = false;
        // bool free_app_1 = false;
        // bool free_app_2 = false;
        bool alloc_app[4];
        bool free_app[4];
        for (int i = 0 ; i < 4 ; i++)
        {
            alloc_app[i] = false;
            free_app[i] = false;
        }
        // bool stopping = false;
        int next_worker_ptr = 0;
        BYTE storage[(CHANNEL_CAPACITY / INIT_VEC_LEN_IN_BYTE * CHANNEL_NUM)];
        #pragma HLS bind_storage variable=storage type=ram_t2p impl=uram
        // memset(storage, 0, sizeof(storage));
        for (int i = 0; i < (CHANNEL_CAPACITY / INIT_VEC_LEN_IN_BYTE * CHANNEL_NUM ); i++) {
            #pragma HLS PIPELINE II=1
            storage[i] = 0;
        }
        while (true)
        {
            // 进入停止流程：不再接收新请求，只清空在途请求
            #pragma HLS PIPELINE II = 1
            if (signal.empty() == false)
            {
                auto x = signal.read();
                if (x.data == MEM_STOP)
                    break;
                else if (x.data == LOAD_BITMAP)
                {
                    for (int i = 0; i < (CHANNEL_CAPACITY  / INIT_VEC_LEN_IN_BYTE * CHANNEL_NUM); i++) {
                        #pragma HLS PIPELINE II=1
                        storage[i] = bm_in_hbm[i];
                    }
                }
            }


            bool process_in_this_cycle = false;

            switch (next_worker_ptr) 
            {
                case 0:
                {
                    process_in_this_cycle = cmd_from_app_1.empty() == false && vec_info_from_app_1.empty() == false;
                    break;
                }
                case 1:
                {
                    process_in_this_cycle = cmd_from_app_2.empty() == false && vec_info_from_app_2.empty() == false ;
                    break;
                }
                case 2:
                {
                    process_in_this_cycle = cmd_from_app_3.empty() == false && vec_info_from_app_3.empty() == false;
                    break;
                }
                case 3:
                {
                    process_in_this_cycle = cmd_from_app_4.empty() == false && vec_info_from_app_4.empty() == false;
                    break;
                }
            }
            if (process_in_this_cycle)
            {
                switch (next_worker_ptr) 
                {
                    case 0:
                    {
                        VEC_INFO_STREAM info = vec_info_from_app_1.read();
                        COMMAND_STREAM cmd = cmd_from_app_1.read();
                        int start = info.data.range(31,0) / INIT_VEC_LEN_IN_BYTE;
                        int req_bytes = info.data.range(63,32);
                        int len = (req_bytes + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
                        
                        if (cmd.data == MEM_ALLOC)
                        {
                            allocate(start, len, storage);
                            VEC_INFO back_info;
                            back_info.range(31,0) = start * INIT_VEC_LEN_IN_BYTE;
                            back_info.range(63,32) = req_bytes;
                            vec_info_to_app_1.write({.data=back_info});
                            // HERE
                        }
                        else if (cmd.data == MEM_FREE)
                        {
                            deallocate(start, len, storage);
                        }
                        break;
                    }
                    case 1:
                    {
                        VEC_INFO_STREAM info = vec_info_from_app_2.read();
                        COMMAND_STREAM cmd = cmd_from_app_2.read();
                        int start = info.data.range(31,0) / INIT_VEC_LEN_IN_BYTE;
                        int req_bytes = info.data.range(63,32);
                        int len = (req_bytes + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
                        
                        if (cmd.data == MEM_ALLOC)
                        {
                            // HERE
                            allocate(start, len, storage);
                            VEC_INFO back_info;
                            back_info.range(31,0) = start * INIT_VEC_LEN_IN_BYTE;
                            back_info.range(63,32) = req_bytes;
                            vec_info_to_app_2.write({.data=back_info});
                        }
                        else if (cmd.data == MEM_FREE)
                        {
                            // HERE
                            deallocate(start, len, storage);
                        }
                        break;
                    }
                    case 2:
                    {
                        VEC_INFO_STREAM info = vec_info_from_app_3.read();
                        COMMAND_STREAM cmd = cmd_from_app_3.read();
                        int start = info.data.range(31,0) / INIT_VEC_LEN_IN_BYTE;
                        int req_bytes = info.data.range(63,32);
                        int len = (req_bytes + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
                        
                        if (cmd.data == MEM_ALLOC)
                        {
                            // HERE
                            allocate(start, len, storage);
                            VEC_INFO back_info;
                            back_info.range(31,0) = start * INIT_VEC_LEN_IN_BYTE;
                            back_info.range(63,32) = req_bytes;
                            vec_info_to_app_3.write({.data=back_info});
                        }
                        else if (cmd.data == MEM_FREE)
                        {
                            // HERE
                            deallocate(start, len, storage);
                        }
                        break;
                    }
                    case 3:
                    {
                        VEC_INFO_STREAM info = vec_info_from_app_4.read();
                        COMMAND_STREAM cmd = cmd_from_app_4.read();
                        int start = info.data.range(31,0) / INIT_VEC_LEN_IN_BYTE;
                        int req_bytes = info.data.range(63,32);
                        int len = (req_bytes + INIT_VEC_LEN_IN_BYTE - 1) / INIT_VEC_LEN_IN_BYTE;
                        
                        if (cmd.data == MEM_ALLOC)
                        {
                            // HERE
                            allocate(start, len, storage);
                            VEC_INFO back_info;
                            back_info.range(31,0) = start * INIT_VEC_LEN_IN_BYTE;
                            back_info.range(63,32) = req_bytes;
                            vec_info_to_app_4.write({.data=back_info});
                        }
                        else if (cmd.data == MEM_FREE)
                        {
                            // HERE
                            deallocate(start, len, storage);
                        }
                        break;
                    }
                }
            }
            next_worker_ptr  = (next_worker_ptr + 1) % MAX_WORKERS;
        }
    }
}