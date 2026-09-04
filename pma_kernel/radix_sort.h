#ifndef RADIX_SORT_H
#define RADIX_SORT_H

#include <ap_int.h>

// ---- Radix LSD sort (by src, 24 bits, 6 passes × 4 bits) ----
// Edges are (src, dst, opcode) triplets in buf[3i .. 3i+2]; sorted by src only.
//   src = pma<<4 | vertex, so sorting by src groups edges by PMA then vertex.
//
// LSD radix: sort the least-significant digit first, each pass STABLE
// (histogram → prefix sum → in-order reorder), so later (higher) passes
// preserve the lower-digit order. After 6 passes the array is sorted by the
// full 24-bit src.
//
// Ping-pong: even passes read buf_a → write buf_b, odd read buf_b → write
// buf_a. After 6 passes (pass 5 is odd) the result is back in buf_a.
//
// Note: n is a runtime value; src values must fit in 24 bits (i.e. max vertex
// index < 2^24 = 16M, matching host MAX_PMA_NUM * VERTEX_PER_PMA).

static void radix_pass(int* src_buf, int* dst_buf, int n, int shift) {
    int hist[16];
    #pragma HLS ARRAY_PARTITION variable=hist complete
    for (int b = 0; b < 16; b++) hist[b] = 0;

    // histogram of the 4-bit digit
    for (int i = 0; i < n; i++) {
        #pragma HLS PIPELINE II=1
        int src = src_buf[i * 3];
        hist[(src >> shift) & 0xF]++;
    }

    // prefix sum → per-bucket write offsets
    int off[16];
    #pragma HLS ARRAY_PARTITION variable=off complete
    int acc = 0;
    for (int b = 0; b < 16; b++) { off[b] = acc; acc += hist[b]; }

    // stable reorder: scan in original order, each element goes to its
    // bucket's next slot → equal keys keep their relative order
    for (int i = 0; i < n; i++) {
        #pragma HLS PIPELINE II=1
        int src = src_buf[i * 3];
        int bucket = (src >> shift) & 0xF;
        int pos = off[bucket];
        dst_buf[pos * 3 + 0] = src_buf[i * 3 + 0];
        dst_buf[pos * 3 + 1] = src_buf[i * 3 + 1];
        dst_buf[pos * 3 + 2] = src_buf[i * 3 + 2];
        off[bucket]++;
    }
}

static void radix_sort_src(int* buf_a, int* buf_b, int n) {
    for (int pass = 0; pass < 6; pass++) {
        if ((pass & 1) == 0) radix_pass(buf_a, buf_b, n, pass * 4);
        else                 radix_pass(buf_b, buf_a, n, pass * 4);
    }
    // pass 5 (odd) writes buf_a → result in buf_a
}

// ---- Radix LSD sort by (src, dst) lexicographic ascending ----
// key = (src << 32) | dst (64-bit): src 优先, dst 其次 → 字典序。
// 8-bit radix, 8 passes (shift 0, 8, ..., 56)。Edges are (src, dst, opcode)
// triplets in buf[3i..3i+2]; the full triplet moves together.
// Ping-pong: after 8 passes (pass 7 is odd) the result is back in buf_a.
static void radix_pass_8bit(int* src_buf, int* dst_buf, int n, int shift) {
    int hist[256];
    #pragma HLS ARRAY_PARTITION variable=hist complete
    for (int b = 0; b < 256; b++) hist[b] = 0;

    // histogram of the 8-bit digit of the 64-bit (src<<32)|dst key
    for (int i = 0; i < n; i++) {
        #pragma HLS PIPELINE II=1
        ap_uint<64> key = ((ap_uint<64>)(ap_uint<32>)src_buf[i * 3] << 32) | (ap_uint<32>)src_buf[i * 3 + 1];
        hist[(int)((key >> shift) & 0xFF)]++;
    }

    int off[256];
    #pragma HLS ARRAY_PARTITION variable=off complete
    int acc = 0;
    for (int b = 0; b < 256; b++) { off[b] = acc; acc += hist[b]; }

    // stable reorder
    for (int i = 0; i < n; i++) {
        #pragma HLS PIPELINE II=1
        ap_uint<64> key = ((ap_uint<64>)(ap_uint<32>)src_buf[i * 3] << 32) | (ap_uint<32>)src_buf[i * 3 + 1];
        int bucket = (int)((key >> shift) & 0xFF);
        int pos = off[bucket];
        dst_buf[pos * 3 + 0] = src_buf[i * 3 + 0];
        dst_buf[pos * 3 + 1] = src_buf[i * 3 + 1];
        dst_buf[pos * 3 + 2] = src_buf[i * 3 + 2];
        off[bucket]++;
    }
}

static void sort_src_dst(int* buf_a, int* buf_b, int n) {
    // ---- 子层启用 (提前终止): 高位字节全 0 的 pass 是恒等操作, 直接跳过 ----
    // 实际 pass 数 = ceil(bits(max_key)/8), 即只排有意义的低字节, 高字节全 0
    // 的 pass 不跑。对中小图 (src < 2^16 / 2^12) 省 2-4 个 pass (~2n+256 每个)。
    // 四舍五入到偶数 pass 保持 ping-pong 结果落回 buf_a (调用方约定)。
    ap_uint<64> max_key = 0;
    for (int i = 0; i < n; i++) {
        #pragma HLS PIPELINE II=1
        ap_uint<64> key = ((ap_uint<64>)(ap_uint<32>)buf_a[i * 3] << 32) | (ap_uint<32>)buf_a[i * 3 + 1];
        if (key > max_key) max_key = key;
    }
    int passes = 0;
    for (int shift = 0; shift < 64; shift += 8) {
        #pragma HLS UNROLL
        if (((max_key >> shift) & 0xFF) != 0) passes = shift / 8 + 1;
    }
    if ((passes & 1) != 0) passes++;          // 偶数 pass → 结果在 buf_a
    if (passes > 8) passes = 8;               // 防御: max_key 64-bit, 最多 8 pass

    for (int pass = 0; pass < passes; pass++) {
        if ((pass & 1) == 0) radix_pass_8bit(buf_a, buf_b, n, pass * 8);
        else                 radix_pass_8bit(buf_b, buf_a, n, pass * 8);
    }
    // 最后 pass 为奇数索引 → 写 buf_a → 结果在 buf_a
}

// Packed variant used by task_reader.  The legacy int-triplet sorter above is
// retained for the standalone insertion experiment, while the production
// wave buffer stores src/dst as 32-bit fields and opcode as a 4-bit field.
static void radix_pass_8bit_packed(
    wave_edge_t* src_buf, wave_edge_t* dst_buf, int n, int shift) {
    int hist[256];
    #pragma HLS ARRAY_PARTITION variable=hist complete
    for (int b = 0; b < 256; b++) hist[b] = 0;

    for (int i = 0; i < n; i++) {
        #pragma HLS PIPELINE II=1
        ap_uint<64> key = ((ap_uint<64>)src_buf[i].src << 32) | src_buf[i].dst;
        hist[(int)((key >> shift) & 0xFF)]++;
    }

    int off[256];
    #pragma HLS ARRAY_PARTITION variable=off complete
    int acc = 0;
    for (int b = 0; b < 256; b++) { off[b] = acc; acc += hist[b]; }

    for (int i = 0; i < n; i++) {
        #pragma HLS PIPELINE II=1
        ap_uint<64> key = ((ap_uint<64>)src_buf[i].src << 32) | src_buf[i].dst;
        int bucket = (int)((key >> shift) & 0xFF);
        int pos = off[bucket];
        dst_buf[pos] = src_buf[i];
        off[bucket]++;
    }
}

static void sort_src_dst_packed(
    wave_edge_t* buf_a, wave_edge_t* buf_b, int n) {
    ap_uint<64> max_key = 0;
    for (int i = 0; i < n; i++) {
        #pragma HLS PIPELINE II=1
        ap_uint<64> key = ((ap_uint<64>)buf_a[i].src << 32) | buf_a[i].dst;
        if (key > max_key) max_key = key;
    }
    int passes = 0;
    for (int shift = 0; shift < 64; shift += 8) {
        #pragma HLS UNROLL
        if (((max_key >> shift) & 0xFF) != 0) passes = shift / 8 + 1;
    }
    if (passes & 1) passes++;
    if (passes > 8) passes = 8;
    for (int pass = 0; pass < passes; pass++) {
        if ((pass & 1) == 0)
            radix_pass_8bit_packed(buf_a, buf_b, n, pass * 8);
        else
            radix_pass_8bit_packed(buf_b, buf_a, n, pass * 8);
    }
}

// ---- In-segment bitonic sort by dst (for merge-insert groundwork) ----
// Sorts edges buf[3i..3i+2] for i in [seg_start, seg_end) by dst ascending,
// using tmp (≥ 256×3 ints) as scratch. max_size = segment size rounded up to
// the next power of two (dynamic, ≤ 256); unused slots are padded with
// dst = INT_MAX sentinels which sink to the end and are dropped on copy-back.
// Segments larger than 256 are left untouched (caller skips or falls back).
// Bitonic networks are NOT stable — irrelevant for dst ordering (duplicates
// are interchangeable).
#define SEG_BITONIC_MAX 256

static void bitonic_segment_by_dst(int* buf, int* tmp, int seg_start, int seg_end) {
    int n = seg_end - seg_start;
    if (n < 2) return;                       // 0/1 element already sorted

    int max_size = 1;
    while (max_size < n) max_size <<= 1;
    if (max_size > SEG_BITONIC_MAX) return;  // oversized segment: skip

    // copy segment into tmp + pad with dst sentinels
    for (int i = 0; i < max_size; i++) {
        #pragma HLS PIPELINE II=1
        if (i < n) {
            tmp[i * 3 + 0] = buf[(seg_start + i) * 3 + 0];
            tmp[i * 3 + 1] = buf[(seg_start + i) * 3 + 1];
            tmp[i * 3 + 2] = buf[(seg_start + i) * 3 + 2];
        } else {
            tmp[i * 3 + 0] = 0x7FFFFFFF;     // src sentinel
            tmp[i * 3 + 1] = 0x7FFFFFFF;     // dst sentinel → sinks to end
            tmp[i * 3 + 2] = 0;
        }
    }

    // classic bitonic sorting network on the dst field
    for (int k = 2; k <= max_size; k <<= 1) {
        for (int j = k >> 1; j > 0; j >>= 1) {
            for (int i = 0; i < max_size; i++) {
                #pragma HLS PIPELINE II=1
                int ixj = i ^ j;
                if (ixj > i) {
                    bool do_swap;
                    if ((i & k) == 0) do_swap = (tmp[i * 3 + 1] > tmp[ixj * 3 + 1]);
                    else              do_swap = (tmp[i * 3 + 1] < tmp[ixj * 3 + 1]);
                    if (do_swap) {
                        int t0 = tmp[i * 3 + 0], t1 = tmp[i * 3 + 1], t2 = tmp[i * 3 + 2];
                        tmp[i * 3 + 0] = tmp[ixj * 3 + 0];
                        tmp[i * 3 + 1] = tmp[ixj * 3 + 1];
                        tmp[i * 3 + 2] = tmp[ixj * 3 + 2];
                        tmp[ixj * 3 + 0] = t0; tmp[ixj * 3 + 1] = t1; tmp[ixj * 3 + 2] = t2;
                    }
                }
            }
        }
    }

    // copy back (valid elements only, sentinels dropped)
    for (int i = 0; i < n; i++) {
        #pragma HLS PIPELINE II=1
        buf[(seg_start + i) * 3 + 0] = tmp[i * 3 + 0];
        buf[(seg_start + i) * 3 + 1] = tmp[i * 3 + 1];
        buf[(seg_start + i) * 3 + 2] = tmp[i * 3 + 2];
    }
}

#endif // RADIX_SORT_H
