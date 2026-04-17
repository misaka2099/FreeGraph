#include <cstdlib>
#include <iostream>

#include "data_type.h"
#include "hls_stream.h"

extern "C" {
void alloc_access_kernel(
	BYTE* bm_in_hbm,
	bool init_from_hbm,
	hls::stream<MEMORY_ALLOC_FREE_REQUEST>& req_from_vertex,
	hls::stream<MEMORY_ALLOC_FREE_REQUEST>& req_from_edge,
	hls::stream<VEC_INFO_STREAM>& vec_info_to_vertex,
	hls::stream<VEC_INFO_STREAM>& vec_info_to_edge);
}

static MEMORY_ALLOC_FREE_REQUEST make_alloc_req(int req_bytes) {
	MEMORY_ALLOC_FREE_REQUEST req = {};
	req.data = 0;
	req.data.range(67, 64) = REQ_TYPE_ALLOC;
	req.data.range(63, 32) = req_bytes;
	req.data.range(31, 0) = 0;
	req.last = 0;
	return req;
}

static MEMORY_ALLOC_FREE_REQUEST make_free_req(int start_bytes, int len_bytes) {
	MEMORY_ALLOC_FREE_REQUEST req = {};
	req.data = 0;
	req.data.range(67, 64) = REQ_TYPE_FREE;
	req.data.range(63, 32) = len_bytes;
	req.data.range(31, 0) = start_bytes;
	req.last = 0;
	return req;
}

static MEMORY_ALLOC_FREE_REQUEST make_finish_req() {
	MEMORY_ALLOC_FREE_REQUEST req = {};
	req.data = 0;
	req.data.range(67, 64) = REQ_TYPE_FINISH;
	req.last = 0;
	return req;
}

static void expect_alloc(
	hls::stream<VEC_INFO_STREAM>& out,
	int expected_start,
	int expected_len,
	const char* tag,
	int& errors) {
	if (out.empty()) {
		std::cerr << "[FAIL] " << tag << ": no response" << std::endl;
		errors++;
		return;
	}
	VEC_INFO_STREAM pkt = out.read();
	int got_start = pkt.data.range(31, 0);
	int got_len = pkt.data.range(63, 32);
	if (got_start != expected_start || got_len != expected_len) {
		std::cerr << "[FAIL] " << tag << ": expect(start=" << expected_start
				  << ", len=" << expected_len << "), got(start=" << got_start
				  << ", len=" << got_len << ")" << std::endl;
		errors++;
	} else {
		std::cout << "[PASS] " << tag << std::endl;
	}
}

int main() {
	hls::stream<MEMORY_ALLOC_FREE_REQUEST> req_from_vertex;
	hls::stream<MEMORY_ALLOC_FREE_REQUEST> req_from_edge;
	hls::stream<VEC_INFO_STREAM> vec_info_to_vertex;
	hls::stream<VEC_INFO_STREAM> vec_info_to_edge;

	static BYTE bm_in_hbm[(CHANNEL_CAPACITY / INIT_VEC_LEN_IN_BYTE + 7) / 8];
	for (int i = 0; i < (int)((CHANNEL_CAPACITY / INIT_VEC_LEN_IN_BYTE + 7) / 8); i++) {
		bm_in_hbm[i] = 0;
	}

	int errors = 0;

	// Vertex alloc 1 segment -> start 0.
	req_from_vertex.write(make_alloc_req(INIT_VEC_LEN_IN_BYTE));
	// Edge alloc 2 segments -> start 1024.
	req_from_edge.write(make_alloc_req(2 * INIT_VEC_LEN_IN_BYTE));
	// Free first vertex segment [0,1024).
	req_from_vertex.write(make_free_req(0, INIT_VEC_LEN_IN_BYTE));
	// Next-fit alloc 1 segment should prefer cursor progression -> start 3072.
	req_from_edge.write(make_alloc_req(INIT_VEC_LEN_IN_BYTE));

	// Terminate both request streams.
	req_from_vertex.write(make_finish_req());
	req_from_edge.write(make_finish_req());

	alloc_access_kernel(
		bm_in_hbm,
		false,
		req_from_vertex,
		req_from_edge,
		vec_info_to_vertex,
		vec_info_to_edge);

	expect_alloc(
		vec_info_to_vertex,
		0,
		INIT_VEC_LEN_IN_BYTE,
		"vertex alloc #1",
		errors);
	expect_alloc(
		vec_info_to_edge,
		INIT_VEC_LEN_IN_BYTE,
		2 * INIT_VEC_LEN_IN_BYTE,
		"edge alloc #1",
		errors);
	expect_alloc(
		vec_info_to_edge,
		3 * INIT_VEC_LEN_IN_BYTE,
		INIT_VEC_LEN_IN_BYTE,
		"edge alloc #2 next-fit",
		errors);

	if (!vec_info_to_vertex.empty()) {
		std::cerr << "[FAIL] unexpected extra vertex responses" << std::endl;
		errors++;
	}
	if (!vec_info_to_edge.empty()) {
		std::cerr << "[FAIL] unexpected extra edge responses" << std::endl;
		errors++;
	}

	if (errors != 0) {
		std::cerr << "alloc_access_kernel test FAILED, errors=" << errors << std::endl;
		return 1;
	}

	std::cout << "alloc_access_kernel test PASSED" << std::endl;
	return 0;
}
