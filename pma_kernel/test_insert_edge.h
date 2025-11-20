#pragma once 
#include "data_type.h"
#include <hls_stream.h>
extern "C"{
    void pma_insert_edge(
        VEC_INFO* pos_pma_addrs_read,
        VEC_INFO* pos_pma_addrs_write,
        VEC_INFO* neg_pma_addrs_read,
        VEC_INFO* neg_pma_addrs_write,
        ap_uint<512>* hbm_ptr_0,
        ap_uint<512>* hbm_ptr_1,
        ap_uint<512>* hbm_ptr_2,
        ap_uint<512>* hbm_ptr_3,
        ap_uint<512>* hbm_ptr_4,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
        hls::stream<VEC_INFO_STREAM> &vec_info_to_mem_stream,
        hls::stream<COMMAND_STREAM> &cmd_to_mem_stream,
        hls::stream<VEC_INFO_STREAM> &vec_info_back_stream,
        hls::stream<COMMAND_STREAM> &ack_back_to_router
    );
}