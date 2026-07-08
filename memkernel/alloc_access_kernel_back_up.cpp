#include <hls_stream.h>
#include <ap_int.h>
#include "data_type.h"

// ---- Bitmap compile-time constants ----
#define ALLOC_GRANULARITY 1024  // = INIT_VEC_LEN_IN_BYTE
#define BITMAP_WORD_BITS  512
#define MAX_TOTAL_BLOCKS  262144  // upper bound: 256 MB / 1024
#define MAX_BITMAP_WORDS  ((MAX_TOTAL_BLOCKS + BITMAP_WORD_BITS - 1) / BITMAP_WORD_BITS)  // 513

// ---- Bitmap operations (local BRAM, copied from pma_insert_edge) ----

static bool bm_is_free(ap_uint<512>* bm, int start, int len)
{
    for (int off = 0; off < len; ) {
        #pragma HLS PIPELINE II=1
        int wi = (start + off) / BITMAP_WORD_BITS;
        int bi = (start + off) % BITMAP_WORD_BITS;
        int chk = len - off;
        if (chk > BITMAP_WORD_BITS - bi) chk = BITMAP_WORD_BITS - bi;
        ap_uint<512> mask = (((ap_uint<512>)1 << chk) - 1) << bi;
        if ((bm[wi] & mask) != 0) return false;
        off += chk;
    }
    return true;
}

static void bm_set(ap_uint<512>* bm, int start, int len)
{
    for (int off = 0; off < len; ) {
        #pragma HLS PIPELINE II=1
        int wi = (start + off) / BITMAP_WORD_BITS;
        int bi = (start + off) % BITMAP_WORD_BITS;
        int sl = len - off;
        if (sl > BITMAP_WORD_BITS - bi) sl = BITMAP_WORD_BITS - bi;
        ap_uint<512> mask = (((ap_uint<512>)1 << sl) - 1) << bi;
        bm[wi] |= mask;
        off += sl;
    }
}

static void bm_reset(ap_uint<512>* bm, int start, int len)
{
    for (int off = 0; off < len; ) {
        #pragma HLS PIPELINE II=1
        int wi = (start + off) / BITMAP_WORD_BITS;
        int bi = (start + off) % BITMAP_WORD_BITS;
        int rl = len - off;
        if (rl > BITMAP_WORD_BITS - bi) rl = BITMAP_WORD_BITS - bi;
        ap_uint<512> mask = (((ap_uint<512>)1 << rl) - 1) << bi;
        bm[wi] &= ~mask;
        off += rl;
    }
}

static int bm_alloc(ap_uint<512>* bm, int &cursor, int blocks, int total_blocks)
{
    #pragma HLS INLINE off
    int found = -1;
    int limit = total_blocks - blocks;
    if (limit <= 0) { printf("bm_alloc: BAD limit=%d tb=%d blk=%d\n", limit, total_blocks, blocks); return -1; }
    for (int i = cursor; i < limit; i++) {
        #pragma HLS PIPELINE II=1
        #pragma HLS LOOP_TRIPCOUNT min=0 max=262144
        if (found == -1 && bm_is_free(bm, i, blocks)) found = i;
    }
    if (found == -1) {
        int limit2 = (cursor < limit) ? limit : cursor;
        for (int i = 0; i < cursor && i < limit; i++) {
            #pragma HLS PIPELINE II=1
            #pragma HLS LOOP_TRIPCOUNT min=0 max=262144
            if (found == -1 && bm_is_free(bm, i, blocks)) found = i;
        }
    }
    if (found != -1) {
        bm_set(bm, found, blocks);
        cursor = found + blocks;
        if (cursor >= total_blocks) cursor = 0;
    }
    return found;
}

// ---- Main alloc worker ----

extern "C" {
void alloc_access_kernel(
    ap_uint<512>* hbm_ptr,
    int bm_wo,
    int total_blocks,
    int bitmap_words,
    hls::stream<ap_axiu<96,0,0,0>>& cmd_stream,
    hls::stream<ap_axiu<64,0,0,0>>& rsp_stream
)
{
    #pragma HLS INTERFACE m_axi port=hbm_ptr offset=slave bundle=gmem0
    #pragma HLS INTERFACE s_axilite port=hbm_ptr bundle=control
    #pragma HLS INTERFACE s_axilite port=bm_wo bundle=control
    #pragma HLS INTERFACE s_axilite port=total_blocks bundle=control
    #pragma HLS INTERFACE s_axilite port=bitmap_words bundle=control
    #pragma HLS INTERFACE s_axilite port=return bundle=control
    #pragma HLS INTERFACE axis port=cmd_stream
    #pragma HLS INTERFACE axis port=rsp_stream

    #pragma HLS STREAM variable=cmd_stream depth=16
    #pragma HLS STREAM variable=rsp_stream depth=4

    // Local bitmap
    ap_uint<512> bm[MAX_BITMAP_WORDS];
    #pragma HLS bind_storage variable=bm type=ram_2p impl=BRAM
    #pragma HLS DEPENDENCE variable=bm inter false

    int cursor = 0;

    printf("ALLOC_ACCESS: total_blocks=%d bitmap_words=%d bm_wo=%d\n",
           total_blocks, bitmap_words, bm_wo);

    // Init bitmap from HBM
    for (int i = 0; i < bitmap_words; i++) {
        #pragma HLS PIPELINE II=1
        bm[i] = hbm_ptr[bm_wo + i];
    }
    printf("ALLOC_ACCESS: bitmap[0] = 0x%016llx%016llx\n",
           (unsigned long long)(bm[0].range(511,256)),
           (unsigned long long)(bm[0].range(255,0)));

    // Command loop
    bool stop = false;
    while (!stop) {
        ap_axiu<96,0,0,0> cmd = cmd_stream.read();

        int opcode    = (int)cmd.data.range(95, 64);
        int start_byte = (int)cmd.data.range(31, 0);
        int len_byte   = (int)cmd.data.range(63, 32);
        printf("ALLOC_ACCESS: cmd op=%d start=%d len=%d last=%d\n",
               opcode, start_byte, len_byte, (int)cmd.last);

        if (cmd.last == 1 || opcode == 0) {
            // STOP
            stop = true;
        } else if (opcode == 1) {
            // ALLOC: start_byte=0, len_byte=requested bytes
            int blocks = (len_byte + ALLOC_GRANULARITY - 1) / ALLOC_GRANULARITY;
            printf("ALLOC_ACCESS: ALLOC blocks=%d cursor=%d\n", blocks, cursor);
            int blk = bm_alloc(bm, cursor, blocks, total_blocks);
            printf("ALLOC_ACCESS: ALLOC result blk=%d\n", blk);

            ap_axiu<64,0,0,0> rsp;
            rsp.last = 0;
            rsp.keep = 0xFF;
            if (blk >= 0) {
                int alloc_start = blk * ALLOC_GRANULARITY;
                rsp.data.range(31, 0)  = (ap_uint<32>)alloc_start;
                rsp.data.range(63, 32) = (ap_uint<32>)len_byte;
            } else {
                rsp.data.range(31, 0)  = (ap_uint<32>)-1;
                rsp.data.range(63, 32) = 0;
            }
            rsp_stream.write(rsp);
        } else if (opcode == 2) {
            // FREE: start_byte=address, len_byte=bytes to free
            int start_blk = start_byte / ALLOC_GRANULARITY;
            int blocks = (len_byte + ALLOC_GRANULARITY - 1) / ALLOC_GRANULARITY;
            bm_reset(bm, start_blk, blocks);
            // fire-and-forget: no response
        }
    }

    // Write bitmap back to HBM
    for (int i = 0; i < bitmap_words; i++) {
        #pragma HLS PIPELINE II=1
        hbm_ptr[bm_wo + i] = bm[i];
    }

    // Send STOP ack
    ap_axiu<64,0,0,0> rsp;
    rsp.data = 0;
    rsp.last = 0;
    rsp.keep = 0xFF;
    rsp_stream.write(rsp);
}
}
