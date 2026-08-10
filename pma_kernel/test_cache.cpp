#include "data_type.h"
#include <cstdio>
#include <cstring>
#include <hls_stream.h>
#include <thread>
#include <map>
#include <vector>
#include <chrono>
#include <iostream>
#include <cassert>
#include <atomic>
#include <cstdint>
#include <mutex>
#define MAX_CACHE_SIZE 8192
#define CACHE_LINE_SIZE 8

#define MEM_REQUEST_NUM 2
// Forward declare the kernel function implemented in test_cache_kernel.cpp
void cache_mechanism(
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_alloc_access,
    hls::stream<VEC_INFO_STREAM> &vec_info_back_stream,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access,
    hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_mem_access,
    hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_mem_access,
    hls::stream<ap_uint<512>> data_from_app[MEM_REQUEST_NUM],
    hls::stream<ap_uint<512>> data_to_app[MEM_REQUEST_NUM],
    hls::stream<PMA_INFO_ACCESS_COMMAND> req_from_app[MEM_REQUEST_NUM]
);

struct TestState {
    std::atomic<bool> header_read_ok{false};
    std::atomic<bool> seg_read_ok{false};
    std::atomic<int> seg_val0{0};
    std::atomic<int> seg_val1{0};
    std::atomic<int> seg_val2{0};
    std::atomic<int> seg_val3{0};
    std::atomic<bool> extend_ok{false};
    std::atomic<int> after_extend_word0{0};
    std::atomic<bool> alloc_occurred{false};
    std::atomic<bool> memory_write_occurred{false};
    std::atomic<int> memory_write_words{0};
    std::atomic<int> dump_request{-1};
    std::atomic<bool> dump_done{false};
};

struct MemShared {
    std::mutex mtx;
    std::map<int, std::vector<ap_uint<512>>> storage; // keyed by pma_idx
    std::map<int, ap_uint<64>> vec_info_map;
};

static void print_pma_from_shared(MemShared *mem_shared, int pma_idx) {
    std::lock_guard<std::mutex> lock(mem_shared->mtx);
    printf("[main] READ-MEM PMA %d\n", pma_idx);
    if (mem_shared->vec_info_map.find(pma_idx) == mem_shared->vec_info_map.end()) {
        printf("[main] no vec_info for pma_idx=%d\n", pma_idx);
        if (mem_shared->storage.find(pma_idx) == mem_shared->storage.end()) {
            printf("[main] no storage for pma_idx=%d\n", pma_idx);
            return;
        }
        const auto &vec = mem_shared->storage.at(pma_idx);
        printf("[main] stored words: %zu\n", vec.size());
        return;
    }
    ap_uint<64> v = mem_shared->vec_info_map[pma_idx];
    unsigned start = (unsigned)v.range(31,0);
    unsigned len = (unsigned)v.range(63,32);
    printf("PMA %d ADDR START FROM %u LEN %u\n", pma_idx, start, len);
    const auto it = mem_shared->storage.find(pma_idx);
    if (it == mem_shared->storage.end()) { printf("[main] no storage vector for pma_idx=%d\n", pma_idx); return; }
    const auto &vec = it->second;
    int header_words = (int)(sizeof(PMA_HEADER) / 64);
    int word_start = start / 64;
    std::vector<char> hdr_bytes(header_words * 64, 0);
    for (int wi = 0; wi < header_words; wi++) {
        int idx = word_start + wi;
        if (idx < 0 || idx >= (int)vec.size()) continue;
        ap_uint<512> chunk = vec[idx];
        for (int part = 0; part < 8; part++) {
            ap_uint<64> p = chunk.range((part+1)*64-1, part*64);
            unsigned long long val = (unsigned long long)p;
            std::memcpy(&hdr_bytes[wi*64 + part*8], &val, 8);
        }
    }
    PMA_HEADER header = {};
    std::memcpy(&header, hdr_bytes.data(), sizeof(PMA_HEADER));
    for (int i = 0 ; i < VERTEX_PER_PMA ; i++)
    {
        int local_vid = i;
        int global_vid = pma_idx * VERTEX_PER_PMA  + i;
        int edge_count = header.edge_count[i];
        int seg_start = i == 0 ? 0 : header.vertex_range[i-1];
        int seg_end = header.vertex_range[i];
        printf("vertex: local=%d global=%d has %d neighbors (segments %d..%d)\n", local_vid, global_vid, edge_count, seg_start, seg_end-1);
        int seg_words_per_seg = SEGMENT_SIZE * (int)sizeof(int) / 64; // words per segment
        for (int j = seg_start ; j < seg_end ; j ++)
        {
            int seg_word_start = word_start + header_words + j * seg_words_per_seg;
            if (seg_word_start < 0 || seg_word_start + seg_words_per_seg > (int)vec.size()) {
                printf("  segment %d: <no data in mem_model>\n", j);
                continue;
            }
            for (int widx = 0; widx < seg_words_per_seg; widx++) {
                ap_uint<512> w = vec[seg_word_start + widx];
                for (int k = 0; k < 16; k++) {
                    int val = (int)w.range(32*(k+1)-1, 32*k);
                    printf("%d ", val);
                }
            }
            printf("\n");
        }
    }
}

static void mem_model(
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_alloc_access,
    hls::stream<VEC_INFO_STREAM> &vec_info_back_stream,
    hls::stream<MEMORY_INFO_ACCESS_COMMAND> &req_to_mem_access,
    hls::stream<ap_axiu<512, 0, 0, 0>> &data_to_mem_access,
    hls::stream<ap_axiu<512, 0, 0, 0>> &data_from_mem_access,
    MemShared *mem_shared,
    TestState *test
)
{
    using namespace std::chrono_literals;
    int alloc_counter = 1;
    bool running = true;
    while (running)
    {
        if (!req_to_alloc_access.empty())
        {
            auto cmd = req_to_alloc_access.read();
            switch (cmd.command_type)
            {
                case VEC_INFO_ALLOC: {
                    ap_uint<64> v = 0;
                    ap_uint<32> start = (ap_uint<32>)(alloc_counter * 0x1000);
                    ap_uint<32> len = (ap_uint<32>)cmd.len;
                    v.range(31,0) = start;
                    v.range(63,32) = len;
                    {
                        std::lock_guard<std::mutex> lock(mem_shared->mtx);
                        mem_shared->vec_info_map[cmd.pma_idx] = v;
                    }
                    ap_axiu<64,3,0,0> resp;
                    resp.data = v;
                    resp.last = 1;
                    vec_info_back_stream.write(resp);
                    printf("[mem_model] VEC_INFO_ALLOC for pma_idx=%d -> start=%u len=%u\n", cmd.pma_idx, (unsigned)start, (unsigned)len);
                    if (test) test->alloc_occurred.store(true);
                    alloc_counter++;
                    break; }
                case VEC_INFO_FREE: {
                    {
                        std::lock_guard<std::mutex> lock(mem_shared->mtx);
                        mem_shared->vec_info_map.erase(cmd.pma_idx);
                        mem_shared->storage.erase(cmd.pma_idx);
                    }
                    printf("[mem_model] VEC_INFO_FREE pma_idx=%d\n", cmd.pma_idx);
                    break; }
                case ALLOC_FINISH: {
                    printf("[mem_model] ALLOC_FINISH for pma_idx=%d\n", cmd.pma_idx);
                    break; }
                case MEMORY_FINISH: {
                    running = false;
                    break; }
                default: break;
            }
            continue;
        }

        if (!req_to_mem_access.empty())
        {
            auto cmd = req_to_mem_access.read();
            switch (cmd.command_type)
            {
                case VEC_INFO_READ: {
                    ap_uint<64> v = 0;
                    {
                        std::lock_guard<std::mutex> lock(mem_shared->mtx);
                        auto it = mem_shared->vec_info_map.find(cmd.pma_idx);
                        if (it != mem_shared->vec_info_map.end()) v = it->second;
                    }
                    ap_axiu<512,0,0,0> pkt = {};
                    pkt.data.range(63,0) = v;
                    pkt.last = 1;
                    data_from_mem_access.write(pkt);
                    printf("[mem_model] VEC_INFO_READ pma_idx=%d -> %u,%u\n", cmd.pma_idx, (unsigned)v.range(31,0), (unsigned)v.range(63,32));
                    break; }
                case MEMORY_READ: {
                    int words = cmd.len / 64;
                    std::vector<ap_uint<512>> copy;
                    copy.reserve(words);
                    {
                        std::lock_guard<std::mutex> lock(mem_shared->mtx);
                        auto &vec = mem_shared->storage[cmd.pma_idx];
                        if ((int)vec.size() < words + (cmd.start/64)) vec.resize(words + (cmd.start/64), 0);
                        int base = cmd.start/64;
                        for (int j = 0; j < words; j++) copy.push_back(vec[base + j]);
                    }
                    for (int j = 0; j < words; j++) {
                        ap_axiu<512,0,0,0> pkt = {};
                        pkt.data = copy[j];
                        pkt.last = (j == words-1) ? 1 : 0;
                        data_from_mem_access.write(pkt);
                    }
                    printf("[mem_model] MEMORY_READ pma_idx=%d words=%d\n", cmd.pma_idx, words);
                    break; }
                case MEMORY_WRITE: {
                    int words = cmd.len / 64;
                    std::vector<ap_uint<512>> incoming;
                    incoming.reserve(words);
                    for (int j = 0; j < words; j++) {
                        ap_axiu<512,0,0,0> pkt = data_to_mem_access.read();
                        incoming.push_back(pkt.data);
                    }
                    {
                        std::lock_guard<std::mutex> lock(mem_shared->mtx);
                        auto &vec = mem_shared->storage[cmd.pma_idx];
                        if ((int)vec.size() < words + (cmd.start/64)) vec.resize(words + (cmd.start/64), 0);
                        int base = cmd.start/64;
                        for (int j = 0; j < words; j++) vec[base + j] = incoming[j];
                    }
                    printf("[mem_model] MEMORY_WRITE pma_idx=%d words=%d\n", cmd.pma_idx, words);
                    if (test) { test->memory_write_occurred.store(true); test->memory_write_words.store(words); }
                    break; }
                case VEC_INFO_WRITE: {
                    {
                        // record vec_info (start/len) into shared memory map
                        ap_uint<64> v = 0;
                        v.range(31,0) = (ap_uint<32>)cmd.start;
                        v.range(63,32) = (ap_uint<32>)cmd.len;
                        {
                            std::lock_guard<std::mutex> lock(mem_shared->mtx);
                            mem_shared->vec_info_map[cmd.pma_idx] = v;
                        }
                        printf("[mem_model] VEC_INFO_WRITE pma_idx=%d start=%u len=%u\n", cmd.pma_idx, (unsigned)cmd.start, (unsigned)cmd.len);
                    }
                    break; }
                case MEMORY_FINISH: {
                    running = false; break; }
                default: break;
            }
            continue;
        }

        std::this_thread::sleep_for(50us);
    }

    printf("[mem_model] exiting\n");
}
static void write_header_slot(int slot, int pma_idx,
    hls::stream<ap_uint<512>> data_from_app[MEM_REQUEST_NUM],
    hls::stream<PMA_INFO_ACCESS_COMMAND> req_from_app[MEM_REQUEST_NUM],
    const PMA_HEADER &hdr)
{
    const ap_uint<512>* wptr = (const ap_uint<512>*)(&hdr);
    int header_words = (int)(sizeof(PMA_HEADER)/64);
    for (int j = 0; j < header_words; j++) data_from_app[slot].write(wptr[j]);
    PMA_INFO_ACCESS_COMMAND cmd = {}; cmd.command_type = PMA_HEADER_WRITE; cmd.pma_idx = pma_idx;
    req_from_app[slot].write(cmd);
}

static PMA_HEADER read_header_slot(int slot,
    hls::stream<ap_uint<512>> data_to_app[MEM_REQUEST_NUM],
    hls::stream<PMA_INFO_ACCESS_COMMAND> req_from_app[MEM_REQUEST_NUM],
    int pma_idx)
{
    PMA_INFO_ACCESS_COMMAND cmd = {}; cmd.command_type = PMA_HEADER_READ; cmd.pma_idx = pma_idx;
    req_from_app[slot].write(cmd);
    int header_words = (int)(sizeof(PMA_HEADER)/64);
    ap_uint<512> buf[8];
    for (int j = 0; j < header_words; j++) buf[j] = data_to_app[slot].read();
    PMA_HEADER out = {};
    std::memcpy(&out, buf, sizeof(PMA_HEADER));
    return out;
}

static void write_segment_slot(int slot, int pma_idx, int seg_start, int seg_len,
    hls::stream<ap_uint<512>> data_from_app[MEM_REQUEST_NUM],
    hls::stream<PMA_INFO_ACCESS_COMMAND> req_from_app[MEM_REQUEST_NUM],
    const std::vector<int> &vals)
{
    // pack SEGMENT_SIZE * seg_len ints into 512-bit words (16 ints per word)
    int ints_per_word = 16;
    int seg_words = seg_len * (SEGMENT_SIZE / ints_per_word);
    for (int w = 0; w < seg_words; w++) {
        ap_uint<512> word = 0;
        for (int k = 0; k < ints_per_word; k++) {
            int idx = w*ints_per_word + k;
            ap_uint<32> v = 0;
            if (idx < (int)vals.size()) v = (ap_uint<32>)vals[idx]; else v = (ap_uint<32>)(-1);
            word.range(32*(k+1)-1, 32*k) = v;
        }
        data_from_app[slot].write(word);
    }
    PMA_INFO_ACCESS_COMMAND cmd = {}; cmd.command_type = PMA_SEGMENT_WRITE; cmd.pma_idx = pma_idx; cmd.seg_start = seg_start; cmd.seg_len = seg_len;
    req_from_app[slot].write(cmd);
}

static std::vector<int> read_segment_slot(int slot, int pma_idx, int seg_start, int seg_len,
    hls::stream<ap_uint<512>> data_to_app[MEM_REQUEST_NUM],
    hls::stream<PMA_INFO_ACCESS_COMMAND> req_from_app[MEM_REQUEST_NUM])
{
    PMA_INFO_ACCESS_COMMAND cmdr = {}; cmdr.command_type = PMA_SEGMENT_READ; cmdr.pma_idx = pma_idx; cmdr.seg_start = seg_start; cmdr.seg_len = seg_len;
    req_from_app[slot].write(cmdr);
    int seg_words = seg_len * (SEGMENT_SIZE * (int)sizeof(int) / 64);
    std::vector<int> out;
    out.reserve(seg_words * 16);
    for (int w = 0; w < seg_words; w++) {
        ap_uint<512> got = data_to_app[slot].read();
        for (int k = 0; k < 16; k++) {
            int v = (int)got.range(32*(k+1)-1, 32*k);
            out.push_back(v);
        }
    }
    return out;
}

static void evict_by_writing_many(int slot, int start_pma,
    hls::stream<ap_uint<512>> data_from_app[MEM_REQUEST_NUM],
    hls::stream<PMA_INFO_ACCESS_COMMAND> req_from_app[MEM_REQUEST_NUM])
{
    PMA_HEADER tmp = {};
    for (int i = 0; i < VERTEX_PER_PMA; i++) { tmp.vertex_range[i] = i+1; tmp.edge_count[i] = 0; tmp.attr[i] = 0; }
    // write CACHE_LINE_SIZE+2 distinct PMAs to force eviction
    for (int p = start_pma; p < start_pma + CACHE_LINE_SIZE + 2; p++) {
        const ap_uint<512>* wptr = (const ap_uint<512>*)(&tmp);
        for (int j = 0; j < (int)(sizeof(PMA_HEADER)/64); j++) data_from_app[slot].write(wptr[j]);
        PMA_INFO_ACCESS_COMMAND cmd = {}; cmd.command_type = PMA_HEADER_WRITE; cmd.pma_idx = p;
        req_from_app[slot].write(cmd);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

int main()
{
    using namespace std::chrono_literals;
    printf("test_cache: start\n");

    hls::stream<MEMORY_INFO_ACCESS_COMMAND> req_to_alloc_access;
    hls::stream<VEC_INFO_STREAM> vec_info_back_stream;

    hls::stream<MEMORY_INFO_ACCESS_COMMAND> req_to_mem_access;
    hls::stream<ap_axiu<512, 0, 0, 0>> data_to_mem_access;
    hls::stream<ap_axiu<512, 0, 0, 0>> data_from_mem_access;

    hls::stream<ap_uint<512>> data_from_app[MEM_REQUEST_NUM];
    hls::stream<ap_uint<512>> data_to_app[MEM_REQUEST_NUM];
    hls::stream<PMA_INFO_ACCESS_COMMAND> req_from_app[MEM_REQUEST_NUM];

    // test state shared between threads
    TestState test;
    // test-side shared memory for mem_model
    MemShared mem_shared;
    // start memory model thread (pass MemShared and TestState pointers)
    std::thread mem_thread(mem_model, std::ref(req_to_alloc_access), std::ref(vec_info_back_stream), std::ref(req_to_mem_access), std::ref(data_to_mem_access), std::ref(data_from_mem_access), &mem_shared, &test);

    // start kernel thread
    std::thread kernel_thread(cache_mechanism, std::ref(req_to_alloc_access), std::ref(vec_info_back_stream), std::ref(req_to_mem_access), std::ref(data_to_mem_access), std::ref(data_from_mem_access), data_from_app, data_to_app, req_from_app);

    // small helper header
    PMA_HEADER hdr = {};
    for (int i = 0; i < VERTEX_PER_PMA; i++) { hdr.vertex_range[i] = i+1; hdr.edge_count[i] = 0; hdr.attr[i] = 0; }

    // --- Begin required tests ---
    bool test1_ok = true;
    bool test2_ok = true;
    bool test3_ok = true;
    bool test4_ok = true;

    // Test 1: write 1 vertex (vertex 0) 64 edges 0..63 then read back and check header
    printf("Test1: write vertex 0 edges 0..63\n");
    {
        std::vector<int> vals(SEGMENT_SIZE);
        for (int i = 0; i < SEGMENT_SIZE; i++) vals[i] = i;
        write_segment_slot(0, 1, 0, 1, data_from_app, req_from_app, vals);
        std::this_thread::sleep_for(20ms);

        // update header.edge_count[0]
        PMA_HEADER hdr_now = read_header_slot(0, data_to_app, req_from_app, 1);
        hdr_now.edge_count[0] = SEGMENT_SIZE;
        write_header_slot(0, 1, data_from_app, req_from_app, hdr_now);
        std::this_thread::sleep_for(10ms);

        PMA_HEADER hdr_check = read_header_slot(0, data_to_app, req_from_app, 1);
        if (hdr_check.edge_count[0] != SEGMENT_SIZE) {
            printf("  header edge_count[0] mismatch: %d\n", hdr_check.edge_count[0]);
            test1_ok = false;
        }

        auto got = read_segment_slot(0, 1, 0, 1, data_to_app, req_from_app);
        for (int i = 0; i < SEGMENT_SIZE; i++) {
            printf("  got[%d]=%d\n", i, got[i]);
            if (got[i] != vals[i]) { test1_ok = false; printf("  seg val mismatch at %d got=%d expected=%d\n", i, got[i], vals[i]); break; }
        }

        // force eviction to cause writeback
        test.memory_write_occurred.store(false);
        evict_by_writing_many(0, 200, data_from_app, req_from_app);
        std::this_thread::sleep_for(50ms);
        if (!test.memory_write_occurred.load()) { printf("  warning: no memory write observed after eviction\n"); }
    }

    // read and print PMA 1 from mem directly after Test1
    print_pma_from_shared(&mem_shared, 1);

    // Test 2: fill PMA with 16 vertices, each 64 edges, then read back
    printf("Test2: fill PMA (16 vertices * 64 edges)\n");
    {
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            std::vector<int> vals(SEGMENT_SIZE);
            for (int i = 0; i < SEGMENT_SIZE; i++) vals[i] = v*1000 + i;
            write_segment_slot(0, 1, v, 1, data_from_app, req_from_app, vals);
            std::this_thread::sleep_for(5ms);
            PMA_HEADER hdr_now = read_header_slot(0, data_to_app, req_from_app, 1);
            hdr_now.edge_count[v] = SEGMENT_SIZE;
            write_header_slot(0, 1, data_from_app, req_from_app, hdr_now);
            std::this_thread::sleep_for(2ms);
        }

        // read back and verify
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            auto got = read_segment_slot(0, 1, v, 1, data_to_app, req_from_app);
            for (int i = 0; i < SEGMENT_SIZE; i++) {
                int expect = v*1000 + i;
                if (got[i] != expect) { test2_ok = false; printf("  seg mismatch v=%d idx=%d got=%d exp=%d\n", v, i, got[i], expect); break; }
                printf("  vertex %d got[%d]=%d\n", v, i, got[i]);
            }
        }

        // force eviction to flush PMA
        test.memory_write_occurred.store(false);
        evict_by_writing_many(0, 400, data_from_app, req_from_app);
        std::this_thread::sleep_for(50ms);
        if (!test.memory_write_occurred.load()) { printf("  warning: no memory write observed after eviction\n"); }
    }

    // read and print PMA 1 from mem directly after Test2
    print_pma_from_shared(&mem_shared, 1);

    // Test 3: extend even vertices 0,2,4,...14 by 1 segment each (sequentially), verify header and new segment is empty
    printf("Test3: extend even vertices by 1 segment each\n");
    std::vector<int> added_segments;
    for (int t = 0; t < VERTEX_PER_PMA; t += 2) {
        PMA_HEADER old_hdr = read_header_slot(0, data_to_app, req_from_app, 1);
        int old_end = old_hdr.vertex_range[t];
        // compute new_range
        ap_uint<512> new_range_packet = 0;
        int new_range[VERTEX_PER_PMA];
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            if (v < t) new_range[v] = old_hdr.vertex_range[v];
            else new_range[v] = old_hdr.vertex_range[v] + 1;
            new_range_packet.range(32*(v+1)-1, 32*v) = (ap_uint<32>)new_range[v];
        }
        data_from_app[0].write(new_range_packet);
        PMA_INFO_ACCESS_COMMAND cmd = {}; cmd.command_type = PMA_EXTEND; cmd.pma_idx = 1; req_from_app[0].write(cmd);
        std::this_thread::sleep_for(20ms);

        PMA_HEADER after = read_header_slot(0, data_to_app, req_from_app, 1);
        // verify header updated
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            if (after.vertex_range[v] != new_range[v]) { test3_ok = false; printf("  header.range mismatch at v=%d got=%d exp=%d\n", v, after.vertex_range[v], new_range[v]); }
            printf("  after extend vertex %d range=%d\n", v, after.vertex_range[v]);
        }
        // newly added segment index is old_end
        added_segments.push_back(old_end);
        // read new segment and verify it's empty (all-ones => -1)
        auto newseg = read_segment_slot(0, 1, old_end, 1, data_to_app, req_from_app);
        if ((int)newseg[0] != -1) { test3_ok = false; printf("  new segment not empty at seg=%d val0=%d\n", old_end, newseg[0]); }
    }

    // read and print PMA 1 from mem directly after Test3
    print_pma_from_shared(&mem_shared, 1);

    // Test 4: write into newly added segments and verify content
    printf("Test4: write into newly added segments and verify\n");
    for (size_t idx = 0; idx < added_segments.size(); idx++) {
        int seg = added_segments[idx];
        int v = idx * 2; // mapping in our sequence
        std::vector<int> vals(SEGMENT_SIZE);
        for (int i = 0; i < SEGMENT_SIZE; i++) vals[i] = 200000 + v*1000 + i;
        write_segment_slot(0, 1, seg, 1, data_from_app, req_from_app, vals);
        std::this_thread::sleep_for(5ms);
        // read back
        auto got = read_segment_slot(0, 1, seg, 1, data_to_app, req_from_app);
        for (int i = 0; i < SEGMENT_SIZE; i++) {
            if (got[i] != vals[i]) { test4_ok = false; printf("  newseg mismatch seg=%d i=%d got=%d exp=%d\n", seg, i, got[i], vals[i]); break; }
                printf("  new seg %d got[%d]=%d\n", seg, i, got[i]);
        }
    }

    // request mem_model to dump PMA 1 for visibility after Test4
    // ensure cache contents are written back before dumping: trigger eviction and wait for mem write
    test.memory_write_occurred.store(false);
    evict_by_writing_many(0, 600, data_from_app, req_from_app);
    for (int _w2 = 0; _w2 < 200; _w2++) { if (test.memory_write_occurred.load()) break; std::this_thread::sleep_for(5ms); }
    if (!test.memory_write_occurred.load()) { printf("  warning: no memory write observed after eviction before dump\n"); }

    print_pma_from_shared(&mem_shared, 1);

    // finish both request slots (tell kernel to stop)
    PMA_INFO_ACCESS_COMMAND finish = {}; finish.command_type = PMA_FINISH; 
    req_from_app[0].write(finish);
    req_from_app[1].write(finish);

    // wait for kernel to exit
    kernel_thread.join();

    // notify mem_model to finish (drain remaining memory requests)
    MEMORY_INFO_ACCESS_COMMAND mem_fin = {}; mem_fin.command_type = MEMORY_FINISH;
    req_to_alloc_access.write(mem_fin);
    req_to_mem_access.write(mem_fin);

    // drain any leftover streams to avoid HLS leftover warnings
    while (!vec_info_back_stream.empty()) { (void)vec_info_back_stream.read(); }
    while (!data_to_mem_access.empty()) { (void)data_to_mem_access.read(); }
    while (!data_from_mem_access.empty()) { (void)data_from_mem_access.read(); }
    for (int s = 0; s < MEM_REQUEST_NUM; s++) {
        while (!data_to_app[s].empty()) { (void)data_to_app[s].read(); }
        while (!data_from_app[s].empty()) { (void)data_from_app[s].read(); }
        while (!req_from_app[s].empty()) { (void)req_from_app[s].read(); }
    }

    // wait for mem thread to exit
    mem_thread.join();

    printf("\n===== DETAILED TEST SUMMARY =====\n");
    printf("Test1 (single vertex 64 edges): %s\n", test1_ok?"PASS":"FAIL");
    printf("Test2 (full PMA 16x64): %s\n", test2_ok?"PASS":"FAIL");
    printf("Test3 (extend evens): %s\n", test3_ok?"PASS":"FAIL");
    printf("Test4 (new seg writes): %s\n", test4_ok?"PASS":"FAIL");

    if (test1_ok && test2_ok && test3_ok && test4_ok) {
        printf("ALL TESTS PASS\n");
        return 0;
    } else {
        printf("SOME TESTS FAILED\n");
        return 1;
    }
}