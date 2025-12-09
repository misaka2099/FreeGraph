#include "data_type.h"
#include <hls_stream.h>
void get_header(
    ap_uint<512>* hbm_ptr,
    int idx,
    PMA_HEADER& pos_pma_header,
    PMA_HEADER& neg_pma_header,
    VEC_INFO* pos_pma_addrs,
    VEC_INFO* neg_pma_addrs 
)
{
    int pos_pma_addr = pos_pma_addrs[idx].range(31,0);
    int neg_pma_addr = neg_pma_addrs[idx].range(31,0);
    // read pos pma header
    for (int i = 0 ; i < sizeof(PMA_HEADER) / 64 ; i++)
    {
        ap_uint<512>* pma_header_ptr = (ap_uint<512>*) (&pos_pma_header);
        pma_header_ptr[i] = hbm_ptr[(pos_pma_addr + i * 64) / 64];
    }
    // read neg pma header
    for (int i = 0 ; i < sizeof(PMA_HEADER) / 64 ; i++)
    {
        ap_uint<512>* pma_header_ptr = (ap_uint<512>*) (&neg_pma_header);
        pma_header_ptr[i] = hbm_ptr[(neg_pma_addr + i * 64) / 64];
    }
}
void write_header(
    ap_uint<512>* hbm_ptr,
    int pma_idx,
    PMA_HEADER& pos_pma_header,
    PMA_HEADER& neg_pma_header,
    VEC_INFO* pos_pma_addrs,
    VEC_INFO* neg_pma_addrs 
)
{
    int pos_pma_addr = pos_pma_addrs[pma_idx].range(31,0);
    int neg_pma_addr = neg_pma_addrs[pma_idx].range(31,0);
    // write pos pma header
    for (int i = 0 ; i < sizeof(PMA_HEADER) / 64 ; i++)
    {
        ap_uint<512>* pma_header_ptr = (ap_uint<512>*) (&pos_pma_header);
        hbm_ptr[(pos_pma_addr + i * 64) / 64] = pma_header_ptr[i];
    }
    // write neg pma header
    for (int i = 0 ; i < sizeof(PMA_HEADER) / 64 ; i++)
    {
        ap_uint<512>* pma_header_ptr = (ap_uint<512>*) (&neg_pma_header);
        hbm_ptr[(neg_pma_addr + i * 64) / 64] = pma_header_ptr[i];
    }
}
void update_header(
    PMA_HEADER& pos_pma_header,
    PMA_HEADER& neg_pma_header,
    int vertex_num,
    hls::stream<VERTEX_ATTR_STREAM_TYPE>& data_stream
)
{
    vertex_num = vertex_num % VERTEX_PER_PMA;
    auto attr = data_stream.read();
    pos_pma_header.attr[vertex_num] = attr.data;
    neg_pma_header.attr[vertex_num] = attr.data;
}
extern "C"
{
    void pma_update_attr(
        hls::stream<PMA_UPDATE_MESSAGE>& cmd_stream,
        hls::stream<VERTEX_ATTR_STREAM_TYPE>& data_stream,
        ap_uint<512>* hbm_ptr,
        VEC_INFO* pos_pma_addrs,
        VEC_INFO* neg_pma_addrs,
        hls::stream<VEC_INFO_STREAM>& vec_info_to_mem_stream,
        hls::stream<VEC_INFO_STREAM>& vec_info_from_mem_stream,
        hls::stream<COMMAND_STREAM>& cmd_to_mem_stream
    )
    {
        PMA_HEADER pos_pma;
        PMA_HEADER neg_pma;
        int cur_pma_idx = -1;
        while (true)
        {
            auto cmd = cmd_stream.read();
            if (cmd.last == 1 && cmd.data == PMA_STOP)
            {
                write_header(hbm_ptr, cur_pma_idx, pos_pma, neg_pma, pos_pma_addrs, neg_pma_addrs);
                break;
            }
            int pma_idx = cmd.data.range(31,0) / VERTEX_PER_PMA;
            if (cur_pma_idx != pma_idx)
            {
                if (cur_pma_idx != -1)
                {
                    write_header(hbm_ptr, cur_pma_idx, pos_pma, neg_pma, pos_pma_addrs, neg_pma_addrs);
                }
                cur_pma_idx = pma_idx;
                get_header(hbm_ptr, pma_idx, pos_pma, neg_pma, pos_pma_addrs, neg_pma_addrs);
            }
            int vertex_num = cmd.data.range(31,0);
            update_header(pos_pma, neg_pma, vertex_num, data_stream);
        }
    }
}