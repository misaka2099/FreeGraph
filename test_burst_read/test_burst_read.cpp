#include "etc/autopilot_ssdm_op.h"
#include <ap_int.h>
#include <hls_stream.h>
#define MAX_MEMORY_REQUEST 5
enum memory_access_type
{
    READ,
    WRITE,
    FINISH
};
struct memory_access_command
{
    int addr_in_byte_patten;
    int len_in_byte_patten;
    memory_access_type type;
};
void hbm_access_unit(
    hls::stream<memory_access_command> request[MAX_MEMORY_REQUEST],
    hls::stream<ap_uint<512>> data_in[MAX_MEMORY_REQUEST],
    hls::stream<ap_uint<512>> data_out[MAX_MEMORY_REQUEST],
    ap_uint<512>* hbm_ptr
)
{
    bool finished[MAX_MEMORY_REQUEST];
    memory_access_command request_info;
    int addr_in_512_pattern = 0;
    int len_in_512_pattern  = 0;
    for (int i = 0 ; i < MAX_MEMORY_REQUEST ; i++) finished[i] = false;
    int idx = 0;
    while (
        !finished[0] || !finished[1] || !finished[2] || !finished[3] || !finished[4]
    )
    {
        if(request[idx].read_nb(request_info))
        {
            if (request_info.type == FINISH)
            {
                finished[idx] = true;
                ap_wait();
                auto dummy = data_in[idx].read();
                continue;
            }
            addr_in_512_pattern = request_info.addr_in_byte_patten >> 6;
            len_in_512_pattern = request_info.len_in_byte_patten >> 6;
            if (request_info.type == READ)
            {
                for (int i = 0 ; i < len_in_512_pattern ; i++)
                {
                    auto x = hbm_ptr[addr_in_512_pattern + i];
                    data_out[idx].write(x);
                }
            }
            else if (request_info.type == WRITE)
            {
                for (int i = 0 ; i < len_in_512_pattern ; i++)
                {
                    auto x = data_in[idx].read();
                    hbm_ptr[addr_in_512_pattern + i] = x;
                }
            }
        }
        idx = (idx + 1);
        idx = idx % MAX_MEMORY_REQUEST;
        // if (!is_request) continue;
        
    }
}
template <int N>
void add(
    hls::stream<memory_access_command>& memory_access_stream,
    hls::stream<ap_uint<512>>& data_to_mem,
    hls::stream<ap_uint<512>>& data_from_mem
)
{
    struct memory_access_command x;
    x.addr_in_byte_patten = 64*(N);
    x.len_in_byte_patten = 64;
    x.type = READ;
    memory_access_stream.write(x);
    ap_wait();
    ap_uint<512> data = data_from_mem.read();

    for (int i = 0 ; i < 16 ; i++)
    {
        int to_add = data.range(32*(i+1)-1, 32*(i));
        to_add += N;
        data.range(32*(i+1)-1, 32*(i)) = to_add;
    }

    x.addr_in_byte_patten = 64*(N);
    x.len_in_byte_patten = 64;
    x.type = WRITE;
    memory_access_stream.write(x);
    data_to_mem.write(data);
    
    ap_wait();
    x.addr_in_byte_patten = 0;
    x.len_in_byte_patten = 0;
    x.type = FINISH;
    memory_access_stream.write(x);
    data_to_mem.write(0);
}
extern "C"
{
    void test_hbm_access_unit(
        ap_uint<512>* hbm_ptr
    )
    {
        #pragma HLS INTERFACE m_axi port=hbm_ptr bundle=gmem0
        
        hls::stream<memory_access_command> memory_access_stream[MAX_MEMORY_REQUEST];
        hls::stream<ap_uint<512>> data_to_mem[MAX_MEMORY_REQUEST];
        hls::stream<ap_uint<512>> data_from_mem[MAX_MEMORY_REQUEST];
        #pragma HLS STREAM variable=memory_access_stream depth=16
        #pragma HLS STREAM variable=data_to_mem depth=16
        #pragma HLS STREAM variable=data_from_mem depth=16
        #pragma HLS DATAFLOW
        hbm_access_unit(memory_access_stream, data_to_mem, data_from_mem, hbm_ptr);
        add<0>(memory_access_stream[0], data_to_mem[0], data_from_mem[0]);
        add<1>(memory_access_stream[1], data_to_mem[1], data_from_mem[1]);
        add<2>(memory_access_stream[2], data_to_mem[2], data_from_mem[2]);
        add<3>(memory_access_stream[3], data_to_mem[3], data_from_mem[3]);
        add<4>(memory_access_stream[4], data_to_mem[4], data_from_mem[4]);
    }
}