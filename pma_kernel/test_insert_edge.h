#pragma once 
#include "data_type.h"
#include <hls_stream.h>
extern "C"{
    void pma_insert_edge(
        VEC_INFO* pos_pma_addrs,
        VEC_INFO* neg_pma_addrs,
        BYTE* hbm_ptr_0,
        BYTE* hbm_ptr_1,
        BYTE* hbm_ptr_2,
        BYTE* hbm_ptr_3,
        BYTE* hbm_ptr_4,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
        //mem operation
        hls::stream<VEC_INFO_STREAM> &vec_info_to_mem_stream,
        hls::stream<COMMAND_STREAM> &cmd_to_mem_stream,
        hls::stream<VEC_INFO_STREAM> &vec_info_back_stream,
        hls::stream<COMMAND_STREAM> &ack_back_to_router
    );
}