// #include "data_type.h"
#include <cstring>
#include <iostream>

#define SEGMENT_SIZE 64
#define UNUSED_ICON -1
#define DENSITY_BOUND 0.75
int storage[SEGMENT_SIZE * 1024];
void read_seg(int *collect_buffer, int seg_num)
{
    memcpy(collect_buffer, storage + seg_num * SEGMENT_SIZE, SEGMENT_SIZE * sizeof(int));
}
void write_seg(int *valid_buffer, int seg_number)
{
    memcpy(storage + seg_number * SEGMENT_SIZE, valid_buffer, SEGMENT_SIZE * sizeof(int));
}
void redistribute(int seg_start, int seg_end)
{
    int valid_buffer[SEGMENT_SIZE*512];
    for (int i = 0 ; i < SEGMENT_SIZE ; i++)
    {
        valid_buffer[i] = UNUSED_ICON;
    }
    int collect_buffer[SEGMENT_SIZE];
    for (int i = 0 ; i < SEGMENT_SIZE ; i++)
    {
        collect_buffer[i] = -1;
    }
    int pack_ptr = 0;
    int pack_buffer_ptr = 0;
    int complete_seg = 0;
    int seg_count = seg_end - seg_start;
    for (int i = seg_start ; i < seg_end ; i++)
    {
        // ap_uint<512>* collect_buffer_ptr = (ap_uint<512>*)(collect_buffer);
        // for (int j = 0 ; j < SEGMENT_SIZE * sizeof(int) / 64 ; j++)
        // {
        //     #pragma HLS UNROLL
        //     collect_buffer_ptr[j] = 
        //     hbm_ptr[(pma_data_base_addr + i * SEGMENT_SIZE * sizeof(int)) / 64 + j];
        // }
        read_seg(collect_buffer, i);
        for (int j = 0 ; j < SEGMENT_SIZE ; j++)
        {
            if (collect_buffer[j] != UNUSED_ICON)
            {
                valid_buffer[pack_buffer_ptr] = collect_buffer[j];
                pack_buffer_ptr++;
                pack_ptr++;
                if (pack_ptr % SEGMENT_SIZE == 0)
                {
                    complete_seg++;
                }
                if (pack_buffer_ptr == 512*SEGMENT_SIZE)
                {
                    // ap_uint<512>* valid_buffer_ptr = (ap_uint<512>*)(valid_buffer);
                    // for (int k = 0 ; k < SEGMENT_SIZE * sizeof(int) / 64 ; k++)
                    // {
                    //     hbm_ptr[(pma_data_base_addr + (complete_seg) * SEGMENT_SIZE * sizeof(int)) / 64 + k]
                    //         = valid_buffer_ptr[k];
                    // }
                    for (int k = 0 ; k < 512 ; k++)
                    {
                        write_seg(valid_buffer+k*SEGMENT_SIZE, seg_start + k);
                    }
                    pack_buffer_ptr = 0;
                }
            }
        }
    }
    
    pack_buffer_ptr --;
    int write_back_buffer[SEGMENT_SIZE];
    for (int i = 0 ; i < SEGMENT_SIZE ; i++)
    {
        write_back_buffer[i] = -1;
    }
    int base_num = pack_ptr / seg_count;
    int extra_num = pack_ptr % seg_count;
    for (int s = seg_end - 1 ; s >= seg_start ; s--)
    {
        for (int i = 0 ; i < SEGMENT_SIZE ; i++)
        {
            write_back_buffer[i] = UNUSED_ICON;
        }
        int valid_num = base_num + ((s - seg_start) < extra_num? 1:0);
        for (int i = valid_num - 1 ; i >= 0 ; i--)
        {
            if (pack_buffer_ptr == -1)
            {
                complete_seg-=512;
                // ap_uint<512>* valid_buffer_ptr = (ap_uint<512>*)(valid_buffer);
                // for (int k = 0 ; k < SEGMENT_SIZE * sizeof(int) / 64 ; k++)
                // {
                //     valid_buffer_ptr[k] 
                //         = hbm_ptr[(pma_data_base_addr + (complete_seg) * SEGMENT_SIZE * sizeof(int)) / 64 + k];
                // }
                for (int k = 0 ; k < 512 ; k++)
                {
                   read_seg(valid_buffer+k*SEGMENT_SIZE, seg_start + k);
                }
            pack_buffer_ptr = 512 * SEGMENT_SIZE - 1;
            }
            write_back_buffer[i] = valid_buffer[pack_buffer_ptr];
            valid_buffer[pack_buffer_ptr] = UNUSED_ICON;
            pack_buffer_ptr--;
        }
        
        // for (int i = 0 ; i < SEGMENT_SIZE * sizeof(int) / 64 ; i++)
        // {
        //     ap_uint<512>* write_back_buffer_ptr = (ap_uint<512>*) write_back_buffer;
        //     hbm_ptr[(s * SEGMENT_SIZE * sizeof(int) + pma_data_base_addr) / 64 + i] = write_back_buffer_ptr[i];
        // }
        // write_seg(valid_buffer + complete_seg + seg_start, complete_seg + seg_start);
        write_seg(write_back_buffer, s);
    }
}
int main()
{
    for (int i = 0 ; i < SEGMENT_SIZE * 1024 ; i++)
    {
        storage[i] = UNUSED_ICON;
    }
    int idx = 0;
    for (int i = 0 ; i < 600 ; i++)
    {
        for (int j = 0 ; j < SEGMENT_SIZE; j++)
        {
            storage[i * SEGMENT_SIZE + j] = idx;
            idx++;
        }
    }
    redistribute(0, 1024);
    for (int i = 0 ; i < SEGMENT_SIZE * 1024 ; i++)
    {
        std::cout << storage[i] << " ";
        if (i % SEGMENT_SIZE == SEGMENT_SIZE - 1)
            std::cout << std::endl;
    }
    return 0;
}