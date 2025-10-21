#include "ap_axi_sdata.h"
#include "data_type.h"
#include <cstdio>
#include <hls_stream.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include "debug.h"
extern "C"
{
void test_kernel(
    int *int_in,
    double *double_in,
    int *int_out,
    double *double_out,
    int len_int,
    int len_double,
    hls::stream<BYTE_STREAM> &to_write_data,
    hls::stream<BYTE_STREAM> &to_receive_data,
    hls::stream<COMMAND_STREAM> &request,
    hls::stream<VEC_INFO_STREAM> &vec_info_to_mem,
    hls::stream<VEC_INFO_STREAM> &vec_info_from_mem
)
{
    #pragma HLS INTERFACE m_axi port=int_in offset=slave bundle=gmem0
    #pragma HLS INTERFACE m_axi port=double_in offset=slave bundle=gmem1
    #pragma HLS INTERFACE m_axi port=int_out offset=slave bundle=gmem2
    #pragma HLS INTERFACE m_axi port=double_out offset=slave bundle=gmem3
    #pragma HLS INTERFACE axis port=request
    #pragma HLS stream depth=16 variable=request
    #pragma HLS INTERFACE axis port=to_write_data
    #pragma HLS stream depth=16 variable=to_write_data
    #pragma HLS INTERFACE axis port=to_receive_data
    #pragma HLS stream depth=16 variable=to_receive_data
    #pragma HLS INTERFACE s_axilite port=len_int
    #pragma HLS INTERFACE s_axilite port=len_double
    #pragma HLS INTERFACE s_axilite port=return

    VEC_INFO info;
    COMMAND_STREAM cmd;
    int total_int_size = len_int * sizeof(int);
    int total_double_size = len_double * sizeof(double);
    info.range(31, 0) = CHANNEL_CAPACITY - total_int_size / 2;
    info.range(63, 32) = total_int_size;
    HERE
    request.write({.data=MEM_WRITE});
    vec_info_to_mem.write({.data=info});
    WRTIE_LOOP_1:
    for (int i = 0 ; i < len_int ; i++)
    {
        #pragma HLS PIPELINE II=1
        BYTE* p = (BYTE*)&int_in[i];
        for (int j = 0; j < (int)sizeof(int); j++)
        {
            #pragma HLS PIPELINE II=1
            BYTE_STREAM payload;
            payload.data = p[j];
            payload.keep = -1;
            payload.strb = 0;
            payload.last = (i == len_int - 1 && j == (int)sizeof(int) - 1) ? 1 : 0;
            to_write_data.write(payload);
        }
    }
    HERE
    request.write({.data=MEM_READ});
    vec_info_to_mem.write({.data=info});
    READ_LOOP_1:
    for (int i = 0 ; i < len_int ; i++)
    {
        #pragma HLS PIPELINE II=1
        BYTE* p = (BYTE*)&int_out[i];
        // printf("ITERATION %d: TEST_READ\n", i);
        for (int j = 0; j < (int)sizeof(int); j++)
        {
            #pragma HLS PIPELINE II=1
            BYTE_STREAM data = to_receive_data.read();
            p[j] = data.data;
        }
    }

    HERE
    WRITE_LOOP_2:
    info.range(31, 0) = CHANNEL_CAPACITY - total_double_size / 2;
    info.range(63, 32) = total_double_size;
    request.write({.data=MEM_WRITE});
    vec_info_to_mem.write({.data=info});
    for (int i = 0 ; i < len_double ; i++)
    {
        #pragma HLS PIPELINE II=1
        BYTE* p = (BYTE*)&double_in[i];
        for (int j = 0; j < (int)sizeof(double); j++)
        {
            #pragma HLS PIPELINE II=1
            BYTE_STREAM payload;
            payload.data = p[j];
            payload.keep = -1;
            payload.strb = 0;
            payload.last = (i == len_double - 1 && j == (int)sizeof(double) - 1) ? 1 : 0;
            to_write_data.write(payload);
        }
    }
    HERE
    // 按字节（int8）从流读取到 double_out
    request.write({.data=MEM_READ});
    vec_info_to_mem.write({.data=info});
    READ_LOOP_2:
    for (int i = 0 ; i < len_double ; i++)
    {
        #pragma HLS PIPELINE II=1
        BYTE* p = (BYTE*)&double_out[i];
        for (int j = 0; j < (int)sizeof(double); j++)
        {
            #pragma HLS PIPELINE II=1
            BYTE_STREAM data = to_receive_data.read();
            p[j] = data.data;
        }
    }
    HERE

    info.range(31, 0) = 0;
    info.range(63, 32) = 114514;
    request.write({.data=MEM_ALLOC});
    vec_info_to_mem.write({.data=info});
    auto info_back_1 = vec_info_from_mem.read().data;
    int alloc_start_pos = info_back_1.range(31, 0);
    int alloc_total_size = info_back_1.range(63, 32);
    printf("ALLOCATED POS %d, SIZE %d\n", alloc_start_pos, alloc_total_size);
    HERE
    info.range(31,0) = 0;
    info.range(63,32) = 114514;
    request.write({.data=MEM_ALLOC});
    vec_info_to_mem.write({.data=info});
    auto info_back_2 = vec_info_from_mem.read().data;
    alloc_start_pos = info_back_2.range(31, 0);
    alloc_total_size = info_back_2.range(63, 32);
    printf("ALLOCATED POS %d, SIZE %d\n", alloc_start_pos, alloc_total_size);
    HERE
    
    request.write({.data=MEM_FREE});
    vec_info_to_mem.write({.data=info_back_1});
    HERE
    info.range(31, 0) = 0;
    info.range(63, 32) = 114514;

    request.write({.data=MEM_ALLOC});
    vec_info_to_mem.write({.data=info});
    auto info_back_3 = vec_info_from_mem.read().data;
    alloc_start_pos = info_back_3.range(31, 0);
    alloc_total_size = info_back_3.range(63, 32);
    printf("ALLOCATED POS %d, SIZE %d\n", alloc_start_pos, alloc_total_size);
    


    request.write({.data=MEM_FREE});
    vec_info_to_mem.write({.data=info_back_2});
    HERE

    request.write({.data=MEM_FREE});
    vec_info_to_mem.write({.data=info_back_3});
    HERE



    request.write({.data=MEM_STOP});
    vec_info_to_mem.write({.data=0, .last=1});
    HERE
}
}