#include <hls_stream.h>
#include <string.h>
#include "acc_config.h"
#include "l1_api.h"
#include "acc_data_types.h"
#include <ap_axi_sdata.h>

#include "hbm_wrapper.h"

// REGRAPH_ALL_KERNEL_ID_TEXT
// REGRAPH_LITTLE_KERNEL_ID_TEXT
// REGRAPH_BIG_KERNEL_ID_TEXT
// REGRAPH_ALL_KERNEL_ID_TEXT REGRAPH_ALL_KERNEL_ID_TEXT
// REGRAPH_LITTLE_KERNEL_ID_ARRAY REGRAPH_LITTLE_KERNEL_GLOBAL_ID_TEXT REGRAPH_LITTLE_KERNEL_ID_TEXT
// REGRAPH_BIG_KERNEL_ID_ARRAY REGRAPH_BIG_KERNEL_GLOBAL_ID_TEXT REGRAPH_BIG_KERNEL_ID_TEXT
// REGRAPH_ALL_KERNEL_ID_TEXT

extern "C" {
    void  kernelHBMWrapper(
        ap_uint<512>                    *src_prop_1,
        ap_uint<512>                    *src_prop_2,


        ap_uint<512>                    *new_prop_1,
        ap_uint<512>                    *new_prop_2,


		uint num_dense_paritions,
		uint num_sparse_paritions,


        hls::stream<l_ppb_request_pkt>    &l_ppb_request_stm_1,


        hls::stream<l_ppb_response_pkt>   &l_ppb_response_stm_1,


        hls::stream<b_cacheline_request_pkt>    &b_cacheline_request_stm_1,


        hls::stream<b_cacheline_response_pkt>   &b_cacheline_response_stm_1,

		hls::stream<write_burst_pkt>      		&prop_write_burst_stm

    )
{

	#pragma HLS INTERFACE m_axi port=src_prop_1 offset=slave bundle = gmem1
	#pragma HLS INTERFACE m_axi port=src_prop_2 offset=slave bundle = gmem2
	
	#pragma HLS INTERFACE m_axi port=new_prop_1 offset=slave bundle = gmem1
	#pragma HLS INTERFACE m_axi port=new_prop_2 offset=slave bundle = gmem2

	#pragma HLS INTERFACE s_axilite port=src_prop_1  bundle=control
	#pragma HLS INTERFACE s_axilite port=src_prop_2  bundle=control
	
	#pragma HLS INTERFACE s_axilite port=new_prop_1  bundle=control
	#pragma HLS INTERFACE s_axilite port=new_prop_2  bundle=control

	#pragma HLS INTERFACE s_axilite port=num_dense_paritions  bundle=control
	#pragma HLS INTERFACE s_axilite port=num_sparse_paritions  bundle=control
	#pragma HLS INTERFACE s_axilite port=return  bundle=control



#pragma HLS DATAFLOW

	hls::stream<write_burst_dt>      write_burst_stm;
#pragma HLS stream variable=write_burst_stm depth=2

	hls::stream<write_burst_dt>      l_write_burst_stm_local;
#pragma HLS stream variable=l_write_burst_stm_local depth=2

	hls::stream<write_burst_dt>      b_write_burst_stm_local;
#pragma HLS stream variable=b_write_burst_stm_local depth=2

	littleKernelReadMemory<0>(0, src_prop_1, num_dense_paritions, l_ppb_request_stm_1, l_ppb_response_stm_1);

	bigKernelReadMemory<0>(0, src_prop_2, num_sparse_paritions, b_cacheline_request_stm_1, b_cacheline_response_stm_1);


	write_out(
			new_prop_1,
			new_prop_2,
			prop_write_burst_stm			
		);
}
}
