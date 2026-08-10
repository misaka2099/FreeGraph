#include "data_type.h"
#include <cstdio>
#include <cstring>
#include <cxxabi.h>
#include <hls_stream.h>
#include "debug.h"
#include <ap_utils.h>
#include "etc/autopilot_ssdm_op.h"
#include "hls_burst_maxi.h"
#include "hls_print.h"

// Helper SoA layout: FE (first_edge) 1:1 per segment, VC (valid_count) 1:1 per
// segment. Word-count formulas live in data_type.h.
// Empty helper words: first_edge = all UNUSED_ICON, valid_count = all 0
static const ap_uint<512> EMPTY_FE_WORD = ~ap_uint<512>(0);  // all UNUSED_ICON
static const ap_uint<512> EMPTY_VC_WORD = 0;

// Max segments per on-chip chunk — sizes onchip_buffer / edge_buffer.
// 1024 segments × 4 words = 4096 × 512-bit words = 256 KiB on-chip.
static const int MAX_ONCHIP_SEG = 1024;
static const int MAX_ONCHIP_SEG_WORDS = MAX_ONCHIP_SEG * SEG_WORDS_PER_SEG;

// Empty segment ROM: 4 words × 512 bits, replaces make_empty_seg()
// Word 0-2: all ones (UNUSED_ICON for all 16 edge slots)
// Word 3:   all ones except bits[511:480]=0 (valid_count=0)
static const ap_uint<512> EMPTY_SEG_ROM[4] = {
    ~ap_uint<512>(0),            // all ones
    ~ap_uint<512>(0),            // all ones
    ~ap_uint<512>(0),            // all ones
    (~ap_uint<512>(0)) >> 32     // bits[511:480]=0, rest all ones
};

// ---- VEC_INFO access ----

static VEC_INFO vi_read(ap_uint<512>* hbm, int vi_word_off, int pma_idx)
{
    int wi = pma_idx / 8, si = pma_idx % 8;
    ap_uint<512> w = hbm[vi_word_off + wi];
    VEC_INFO vi;
    vi.range(63, 0) = w.range(si * 64 + 63, si * 64);
    return vi;
}

static void vi_write(ap_uint<512>* hbm, int vi_word_off, int pma_idx, VEC_INFO vi)
{
    int wi = pma_idx / 8, si = pma_idx % 8;
    ap_uint<512> w = hbm[vi_word_off + wi];
    w.range(si * 64 + 63, si * 64) = vi.range(63, 0);
    hbm[vi_word_off + wi] = w;
}

// Read a valid_count entry (1:1 per segment, 16 entries per 512-bit word)
// directly from HBM (used by find_redistribution_range and redistribution_phase).
static inline int hbm_vc_read(ap_uint<512>* hbm, int vc_hbm_base, int seg)
{
    int vc_word_off = vc_hbm_base + seg / VC_SOA_ENTRIES_PER_WORD;
    int vc_slot     = seg % VC_SOA_ENTRIES_PER_WORD;
    ap_uint<512> vc_word = hbm[vc_word_off];
    return (int)vc_word.range(vc_slot * 32 + 31, vc_slot * 32);
}

static inline int density_capacity(int seg_cnt) { return seg_cnt * 47; }

// ---- Alloc command helpers (inline alloc_merger) ----
// redistribution 是唯一 alloc 源 (pre_expand ENLARGE/INIT + rebalance ENLARGE),
// 所以不再需要独立 merger 进程: 直接把命令包装成 ap_axiu 写 alloc_access,
// ALLOC (opcode=1) 同步读回 response; FREE (opcode=2) fire-and-forget。

static ap_uint<64> alloc_send_rsp(
    ap_uint<96> cmd,
    hls::stream<ap_axiu<96,0,0,0>>& alloc_cmd_stream,
    hls::stream<ap_axiu<64,0,0,0>>& alloc_rsp_stream)
{
    ap_axiu<96,0,0,0> acmd;
    acmd.data = cmd;
    acmd.last = 0;
    acmd.keep = 0xFFF;
    alloc_cmd_stream.write(acmd);
    ap_wait();  // barrier: 确保请求已发出 (CLAUDE.md)
    ap_axiu<64,0,0,0> arsp = alloc_rsp_stream.read();
    return arsp.data.range(63, 0);
}

static void alloc_send(
    ap_uint<96> cmd,
    hls::stream<ap_axiu<96,0,0,0>>& alloc_cmd_stream)
{
    ap_axiu<96,0,0,0> acmd;
    acmd.data = cmd;
    acmd.last = 0;
    acmd.keep = 0xFFF;
    alloc_cmd_stream.write(acmd);
    ap_wait();
}

// ---- Bitmap init (preamble) ----
// Reads the allocation bitmap from hbm_red and streams it to alloc_access
// (which counts ALLOC_BITMAP_WORDS packets). Runs once before the main loop.
static void bitmap_init_reader(
    ap_uint<512>* hbm,
    int bitmap_offset,
    hls::stream<ap_axiu<ALLOC_BITMAP_STORAGE_WORD_BITS,0,0,0>>& bitmap_init)
{
    int bm_word_off = bitmap_offset / 64;
    for (int i = 0; i < ALLOC_BITMAP_WORDS; i++) {
        #pragma HLS PIPELINE II=1
        ap_axiu<ALLOC_BITMAP_STORAGE_WORD_BITS,0,0,0> w;
        w.data = hbm[bm_word_off + i];
        w.last = 0;
        w.keep = -1;
        bitmap_init.write(w);
    }
}

// Power-of-2 range finder for redistribution.
// Given an overflow segment `overflow_seg` in vertex [vertex_start, vertex_end),
// expand the range by doubling (power-of-2 alignment) until total edges fit
// the density bound.  Reads valid_counts directly from HBM.
//
// Output: left, right = segment range [left, right), total = sum of valid_counts.
void find_redistribution_range(
    ap_uint<512>* hbm,
    int vc_hbm_base,
    int vertex_start, int vertex_end,
    int overflow_seg,
    int &left, int &right, int &total)
{
    int vertex_segments = vertex_end - vertex_start;
    int relative_position = overflow_seg - vertex_start;

    left  = overflow_seg;
    right = overflow_seg + 1;
    total = hbm_vc_read(hbm, vc_hbm_base, overflow_seg);

    for (int group_size = 2; group_size <= vertex_segments * 2; group_size *= 2) {
        if (total <= density_capacity(right - left)) break;

        int new_left  = vertex_start + (relative_position / group_size) * group_size;
        int new_right = vertex_start + ((relative_position / group_size) + 1) * group_size;
        if (new_right > vertex_end) new_right = vertex_end;
        if (new_left == left && new_right == right) continue;

        // Add valid_counts from newly included segments — direct HBM reads
        for (int seg = new_left; seg < left; seg++) {
            #pragma HLS PIPELINE II=1
            total += hbm_vc_read(hbm, vc_hbm_base, seg);
        }
        for (int seg = right; seg < new_right; seg++) {
            #pragma HLS PIPELINE II=1
            total += hbm_vc_read(hbm, vc_hbm_base, seg);
        }

        left = new_left;
        right = new_right;
    }
}

// Collect valid edges from a range of PMA segments into hbm_buffer.
// Each segment = SEG_WORDS_PER_SEG (4) × 512-bit words = 64 ints.
// Only the first valid_count edges are extracted; UNUSED_ICON slots are skipped.
//
// Uses two on-chip buffers to enable burst on both HBM read and write:
//   onchip_buffer — burst-read  raw segments from HBM
//   edge_buf      — init → pack valid edges → burst-write to hbm_buffer
void collect_edge_to_buffer(
    ap_uint<512>* hbm, ap_uint<512>* hbm_buffer,
    int start_word, int end_word, int &total_edges,
    ap_uint<512> onchip_buffer[MAX_ONCHIP_SEG_WORDS],
    ap_uint<512> edge_buf[MAX_ONCHIP_SEG_WORDS]
)
{
    total_edges = 0;
    int total_words = end_word - start_word;

    // Process words in chunks that fit in onchip_buffer
    for (int chunk_off = 0; chunk_off < total_words; chunk_off += MAX_ONCHIP_SEG_WORDS) {
        int chunk_words = total_words - chunk_off;
        if (chunk_words > MAX_ONCHIP_SEG_WORDS) chunk_words = MAX_ONCHIP_SEG_WORDS;

        // ==== Phase 1: burst-read raw segments from HBM → onchip_buffer ====
        // Simple counted loop — no conditional on the memory access path,
        // addresses are monotonically increasing → HLS infers burst.
        for (int i = 0; i < chunk_words; i++) {
            #pragma HLS PIPELINE II=1
            onchip_buffer[i] = hbm[start_word + chunk_off + i];
        }

        // ==== Phase 2: init edge_buf + pack valid edges (all on-chip) ====
        int segs_in_chunk = chunk_words >> 2;  // / SEG_WORDS_PER_SEG (4)
        int max_edge_words = ((segs_in_chunk * EDGE_SLOTS_PER_SEG + 15) >> 4) + 1;

        // Initialize edge_buf to all UNUSED_ICON (all 1s = -1 in every slot)
        for (int i = 0; i < max_edge_words; i++) {
            #pragma HLS PIPELINE II=1
            edge_buf[i] = ~ap_uint<512>(0);
        }

        // Edges are packed with GLOBAL edge indices so chunks stay contiguous
        // across the 16-slot word boundary: edge #(total_edges + edge_cnt)
        // lands at word (total_edges + edge_cnt) >> 4, slot ... & 0xF, relative
        // to hbm_wo = total_edges >> 4. Without this, chunk N overwrites the
        // tail of chunk N-1's last word when the cumulative edge count is not
        // a multiple of 16.
        int hbm_wo    = total_edges >> 4;
        int edge_cnt  = 0;
        for (int s = 0; s < segs_in_chunk; s++) {
            int seg_base = s << 2;  // s * SEG_WORDS_PER_SEG (4)
            // valid_count lives in word 3, bits [511:480]
            int vc = (int)onchip_buffer[seg_base + 3].range(511, 480);
            if (vc > EDGE_SLOTS_PER_SEG) vc = EDGE_SLOTS_PER_SEG;

            for (int k = 0; k < vc; k++) {
                // No PIPELINE — RMW on same edge_buf word across consecutive
                // iterations is resolved by HLS with safe II; on-chip access
                // is not the bottleneck (HBM burst dominates).
                int wi = seg_base + (k >> 4);
                int si = k & 0xF;
                int edge = (int)onchip_buffer[wi].range(si * 32 + 31, si * 32);
                int global_edge = total_edges + edge_cnt;
                int word_idx = (global_edge >> 4) - hbm_wo;
                int slot     = global_edge & 0xF;
                edge_buf[word_idx].range(slot * 32 + 31, slot * 32) = (ap_uint<32>)edge;
                edge_cnt++;
            }
        }

        // ==== Phase 3: burst-write edge_buf → hbm_buffer ====
        int edge_words = ((total_edges + edge_cnt + 15) >> 4) - hbm_wo;
        for (int i = 0; i < edge_words; i++) {
            #pragma HLS PIPELINE II=1
            hbm_buffer[hbm_wo + i] = edge_buf[i];
        }
        total_edges += edge_cnt;
    }
}

// Redistribute edges evenly across a group of segments.
// Called AFTER collect_edge_to_buffer: edges are already extracted into hbm_buffer
// as a flat packed array. Reads edges from hbm_buffer, redistributes with the
// base+extra pattern (first 'extra' segments get one more edge), and writes
// properly formed segments to hbm.
//
// Processing is chunked: each chunk handles up to MAX_ONCHIP_SEG segments,
// reading only the edges needed for those segments and burst-writing the result.
// VC (1:1 per segment) and FE (1:1 per segment) SoA helper entries are updated
// inline for each chunk.
void redistribute_edge_from_hbm(
    ap_uint<512>* hbm, ap_uint<512>* hbm_buffer,
    int start_word, int end_word, int total_edges,
    int edge_offset,             // starting edge index in hbm_buffer
    int group_left,              // absolute segment index of group start
    int vc_hbm_base,             // valid_count area HBM base (word offset)
    int fe_hbm_base,             // first_edge area HBM base (word offset)
    ap_uint<512> onchip_buffer[MAX_ONCHIP_SEG_WORDS],
    ap_uint<512> edge_buf[MAX_ONCHIP_SEG_WORDS]
)
{
    int total_words = end_word - start_word;
    int total_segs  = total_words / SEG_WORDS_PER_SEG;

    // Distribution: base edges per segment, first 'extra' segments get +1
    int base  = total_edges / total_segs;
    int extra = total_edges % total_segs;

    // Cumulative edges before segment s: s * base + min(s, extra)
    int max_chunk_segs = MAX_ONCHIP_SEG_WORDS / SEG_WORDS_PER_SEG;  // 1024

    // Process output segments in chunks
    for (int chunk_start = 0; chunk_start < total_segs; chunk_start += max_chunk_segs) {
        int chunk_segs = total_segs - chunk_start;
        if (chunk_segs > max_chunk_segs) chunk_segs = max_chunk_segs;
        int chunk_words = chunk_segs * SEG_WORDS_PER_SEG;

        // Edge range for this chunk
        int edge_start = chunk_start * base + (chunk_start < extra ? chunk_start : extra);
        int edge_end   = (chunk_start + chunk_segs) * base + ((chunk_start + chunk_segs) < extra ? (chunk_start + chunk_segs) : extra);
        int chunk_edges = edge_end - edge_start;

        // ==== Phase 1: init onchip_buffer to EMPTY_SEG for this chunk ====
        for (int i = 0; i < chunk_words; i += SEG_WORDS_PER_SEG) {
            #pragma HLS PIPELINE II=1
            onchip_buffer[i + 0] = EMPTY_SEG_ROM[0];
            onchip_buffer[i + 1] = EMPTY_SEG_ROM[1];
            onchip_buffer[i + 2] = EMPTY_SEG_ROM[2];
            onchip_buffer[i + 3] = EMPTY_SEG_ROM[3];
        }

        // ==== Phase 2: burst-read edges from hbm_buffer → edge_buf ====
        // edge_words must cover the full word range: the starting word may be
        // partially consumed (edge_offset + edge_start not a multiple of 16),
        // so the read length is ceil((start_mod + chunk_edges) / 16), not
        // ceil(chunk_edges / 16) — otherwise the tail words are garbage.
        int hbm_edge_wo = (edge_offset + edge_start) >> 4;
        int start_mod   = (edge_offset + edge_start) & 0xF;
        int edge_words  = ((start_mod + chunk_edges + 15) >> 4);
        for (int i = 0; i < edge_words; i++) {
            #pragma HLS PIPELINE II=1
            edge_buf[i] = hbm_buffer[hbm_edge_wo + i];
        }

        // ==== Phase 3: pack edges into onchip_buffer with base+extra distribution ====
        int pos = 0;
        for (int s = 0; s < chunk_segs; s++) {
            int fill = base + ((chunk_start + s) < extra ? 1 : 0);
            for (int k = 0; k < fill; k++) {
                #pragma HLS PIPELINE II=1
                int global_pos = edge_offset + edge_start + pos;
                int src_word = (global_pos >> 4) - hbm_edge_wo;
                int src_slot = global_pos & 0xF;
                int edge = (int)edge_buf[src_word].range(src_slot * 32 + 31, src_slot * 32);
                int dst_wi = s * SEG_WORDS_PER_SEG + (k >> 4);
                int dst_si = k & 0xF;
                onchip_buffer[dst_wi].range(dst_si * 32 + 31, dst_si * 32) = (ap_uint<32>)edge;
                pos++;
            }
            // Set valid_count for this output segment
            onchip_buffer[s * SEG_WORDS_PER_SEG + 3].range(511, 480) = (ap_uint<32>)fill;
        }

        // ==== Phase 4: burst-write onchip_buffer → hbm ====
        int hbm_seg_wo = start_word + chunk_start * SEG_WORDS_PER_SEG;
        for (int i = 0; i < chunk_words; i++) {
            #pragma HLS PIPELINE II=1
            hbm[hbm_seg_wo + i] = onchip_buffer[i];
        }

        // ==== Phase 5: update VC (HBM RMW) and FE (HBM RMW) ====
        int seg_chunk_start = group_left + chunk_start;

        // 5a: update VC HBM (RMW per word)
        {
            int vc_word_start = seg_chunk_start / VC_SOA_ENTRIES_PER_WORD;
            int vc_word_end   = (seg_chunk_start + chunk_segs + VC_SOA_ENTRIES_PER_WORD - 1)
                                / VC_SOA_ENTRIES_PER_WORD;
            int vc_word_count = vc_word_end - vc_word_start;
            // Reuse edge_buf as temp vc buffer
            for (int w = 0; w < vc_word_count; w++) {
                #pragma HLS PIPELINE II=1
                edge_buf[w] = hbm[vc_hbm_base + vc_word_start + w];
            }
            for (int s = 0; s < chunk_segs; s++) {
                #pragma HLS PIPELINE II=1
                int global_seg = seg_chunk_start + s;
                int fill = base + ((chunk_start + s) < extra ? 1 : 0);
                // Update VC HBM buffer
                int local_word = (global_seg / VC_SOA_ENTRIES_PER_WORD) - vc_word_start;
                int local_slot = global_seg % VC_SOA_ENTRIES_PER_WORD;
                edge_buf[local_word].range(local_slot * 32 + 31, local_slot * 32)
                    = (ap_uint<32>)fill;
            }
            for (int w = 0; w < vc_word_count; w++) {
                #pragma HLS PIPELINE II=1
                hbm[vc_hbm_base + vc_word_start + w] = edge_buf[w];
            }
        }

        // 5b: update FE HBM (RMW, 1:1 — every segment gets its own first_edge;
        // empty segments keep EMPTY_FE_WORD = UNUSED_ICON)
        {
            int fe_word_start = seg_chunk_start / FE_SOA_ENTRIES_PER_WORD;
            int fe_word_end   = (seg_chunk_start + chunk_segs + FE_SOA_ENTRIES_PER_WORD - 1)
                                / FE_SOA_ENTRIES_PER_WORD;
            int fe_word_count = fe_word_end - fe_word_start;
            // Reuse edge_buf as temp fe buffer
            for (int w = 0; w < fe_word_count; w++) {
                #pragma HLS PIPELINE II=1
                edge_buf[w] = hbm[fe_hbm_base + fe_word_start + w];
            }
            for (int s = 0; s < chunk_segs; s++) {
                #pragma HLS PIPELINE II=1
                int global_seg = seg_chunk_start + s;
                int local_word = global_seg / FE_SOA_ENTRIES_PER_WORD - fe_word_start;
                int local_slot = global_seg % FE_SOA_ENTRIES_PER_WORD;
                // first_edge of the output segment (word 0 bits[31:0]); empty
                // segments contain EMPTY_SEG_ROM → all ones = UNUSED_ICON
                ap_uint<32> seg_fe = onchip_buffer[s * SEG_WORDS_PER_SEG].range(31, 0);
                edge_buf[local_word].range(local_slot * 32 + 31, local_slot * 32) = seg_fe;
            }
            for (int w = 0; w < fe_word_count; w++) {
                #pragma HLS PIPELINE II=1
                hbm[fe_hbm_base + fe_word_start + w] = edge_buf[w];
            }
        }
    }
}

// ---- ENLARGE: owner vertex over capacity — grow the PMA ----
// Collects all edges from the old PMA into hbm_buffer (separate HBM bank for
// read/write separation), allocates new space, then redistributes edges
// per-vertex to the new PMA. Uses the existing collect_edge_to_buffer +
// redistribute_edge_from_hbm pipeline so hbm (read old) and hbm_buffer
// (intermediate storage) operate on independent m_axi ports.
static void pma_redist_enlarge(
    ap_uint<512>* hbm,
    ap_uint<512>* hbm_buffer,
    int data_off, int vi_wo,
    int pma_idx,
    int start_byte,               // absolute start byte of the CURRENT PMA
    ap_uint<512> range_word,      // current header[0] (vertex ranges + total_segs)
    int owner_vertex,
    int vertex_segs,
    hls::stream<ap_axiu<96,0,0,0>>& alloc_cmd,
    hls::stream<ap_axiu<64,0,0,0>>& alloc_rsp,
    ap_uint<512> onchip_buffer[MAX_ONCHIP_SEG_WORDS],
    ap_uint<512> edge_buffer[MAX_ONCHIP_SEG_WORDS],
    int &new_start_byte,          // out: absolute start byte of the new PMA
    ap_uint<512> &new_range_word) // out: new header[0]
{
    int delta = (vertex_segs <= 4) ? 3 * vertex_segs : vertex_segs;
    int old_tseg = (int)range_word.range(15 * 32 + 31, 15 * 32);
    int ntseg = old_tseg + delta;

    int nfe_words = fe_words_for(ntseg);
    int nvc_words = vc_words_for(ntseg);
    int nhlp      = nfe_words + nvc_words;
    int npwords   = HEADER_WORDS + nhlp + ntseg * SEG_WORDS_PER_SEG;
    int npbytes   = npwords * 64;

    int pma_boff  = start_byte - data_off;
    int hbase     = pma_boff / 64;
    ap_uint<512> edge_count_word = hbm[hbase + 1];

    // Old PMA segment range
    int old_hlp      = hlp_total_words(old_tseg);
    int old_seg_base = (pma_boff + HEADER_BYTES + old_hlp * 64) / 64;

    // ---- Step 1: collect all edges from old PMA → hbm_buffer ----
    int collected_edges = 0;
    collect_edge_to_buffer(hbm, hbm_buffer,
        old_seg_base, old_seg_base + old_tseg * SEG_WORDS_PER_SEG,
        collected_edges,
        onchip_buffer, edge_buffer);

    // ---- Step 2: allocate new space ----
    ap_uint<96> acmd = 0;
    acmd.range(31, 0)  = 0;                       // start=0 for ALLOC
    acmd.range(63, 32) = (ap_uint<32>)npbytes;    // request bytes
    acmd.range(95, 64) = 1;                       // opcode=ALLOC
    ap_uint<64> arsp = alloc_send_rsp(acmd, alloc_cmd, alloc_rsp);
    int nstart = (int)arsp.range(31, 0);
    int nabs_base = nstart / 64;

    // ---- Step 3: new range_word — owner vertex grows by delta, later ranges shift ----
    new_range_word = range_word;
    for (int v = owner_vertex; v < VERTEX_PER_PMA; v++) {
        #pragma HLS UNROLL
        int old_vr = (int)range_word.range(v * 32 + 31, v * 32);
        new_range_word.range(v * 32 + 31, v * 32) = (ap_uint<32>)(old_vr + delta);
    }

    // ---- Step 4: write new header to new location ----
    {
        ap_uint<512> new_hdr[HEADER_WORDS];
        new_hdr[0] = new_range_word;
        new_hdr[1] = edge_count_word;   // unchanged by enlarge
        new_hdr[2] = 0;
        new_hdr[3] = 0;
        for (int w = 0; w < HEADER_WORDS; w++) {
            #pragma HLS PIPELINE II=1
            hbm[nabs_base + w] = new_hdr[w];
        }
    }

    // ---- Step 5: init SoA helper areas (FE=UNUSED_ICON, VC=0) ----
    int fe_hbm_base = nabs_base + HEADER_WORDS;
    int vc_hbm_base = fe_hbm_base + nfe_words;
    for (int w = 0; w < nfe_words; w++) {
        #pragma HLS PIPELINE II=1
        hbm[fe_hbm_base + w] = EMPTY_FE_WORD;
    }
    for (int w = 0; w < nvc_words; w++) {
        #pragma HLS PIPELINE II=1
        hbm[vc_hbm_base + w] = EMPTY_VC_WORD;
    }

    // ---- Step 6: per-vertex redistribute from hbm_buffer → new PMA ----
    int nseg_base   = nabs_base + HEADER_WORDS + nhlp;
    int edge_cursor = 0;

    for (int v = 0; v < VERTEX_PER_PMA; v++) {
        int old_ec = (int)edge_count_word.range(v * 32 + 31, v * 32);
        int nvs = (v == 0) ? 0 : (int)new_range_word.range((v - 1) * 32 + 31, (v - 1) * 32);
        int nve = (int)new_range_word.range(v * 32 + 31, v * 32);
        int ncnt = nve - nvs;

        if (old_ec == 0) {
            // SoA helper already EMPTY from step 5; just write clean segments.
            int empty_base = nseg_base + nvs * SEG_WORDS_PER_SEG;
            for (int i = 0; i < ncnt; i++) {
                #pragma HLS PIPELINE II=1
                int off = i * SEG_WORDS_PER_SEG;
                hbm[empty_base + off + 0] = EMPTY_SEG_ROM[0];
                hbm[empty_base + off + 1] = EMPTY_SEG_ROM[1];
                hbm[empty_base + off + 2] = EMPTY_SEG_ROM[2];
                hbm[empty_base + off + 3] = EMPTY_SEG_ROM[3];
            }
            continue;
        }

        int seg_start_word = nseg_base + nvs * SEG_WORDS_PER_SEG;
        int seg_end_word   = seg_start_word + ncnt * SEG_WORDS_PER_SEG;

        redistribute_edge_from_hbm(
            hbm, hbm_buffer,
            seg_start_word, seg_end_word,
            old_ec,
            edge_cursor,   // edge_offset in hbm_buffer
            nvs,           // group_left (absolute segment index)
            vc_hbm_base, fe_hbm_base,
            onchip_buffer, edge_buffer);

        edge_cursor += old_ec;
    }

    // ---- Step 7: free old PMA (fire-and-forget) ----
    {
        int obytes = (HEADER_WORDS + old_hlp + old_tseg * SEG_WORDS_PER_SEG) * 64;
        ap_uint<96> fcmd = 0;
        fcmd.range(31, 0)  = (ap_uint<32>)pma_boff;
        fcmd.range(63, 32) = (ap_uint<32>)obytes;
        fcmd.range(95, 64) = 2;                   // opcode=FREE
        alloc_send(fcmd, alloc_cmd);
    }
    ap_wait();  // barrier: FREE 必须排在 VEC_INFO 更新之前 (CLAUDE.md)

    // ---- Step 8: update VEC_INFO + report new location ----
    VEC_INFO nvi = 0;
    nvi.range(31, 0)  = (ap_uint<32>)(nstart + data_off);
    nvi.range(63, 32) = (ap_uint<32>)npbytes;
    vi_write(hbm, vi_wo, pma_idx, nvi);

    new_start_byte = nstart + data_off;
}

// ---- Pre-expanded growth (profile-driven, runs BEFORE the edge batch) ----
// Grows the PMA so a batch of `count` insertions fits without runtime ENLARGE.
//   vi == 0  (INIT):   allocate a fresh PMA with 1 + ceil(count[v]/47) segments
//                      per vertex. No collect (no old data exists), no FREE.
//   vi != 0  (ENLARGE): move the whole PMA; every vertex gains
//                      ceil(count[v]/47) segments (per-vertex precise), edges
//                      re-distributed by redistribute_edge_from_hbm. Old space
//                      is FREEd.
static void pma_pre_expand(
    ap_uint<512>* hbm, ap_uint<512>* hbm_buffer,
    int data_off, int vi_wo, int pma_idx, int count[VERTEX_PER_PMA],
    hls::stream<ap_axiu<96,0,0,0>>& alloc_cmd,
    hls::stream<ap_axiu<64,0,0,0>>& alloc_rsp,
    ap_uint<512> onchip_buffer[MAX_ONCHIP_SEG_WORDS],
    ap_uint<512> edge_buffer[MAX_ONCHIP_SEG_WORDS])
{
    VEC_INFO vi = vi_read(hbm, vi_wo, pma_idx);
    bool is_init = (vi == 0);

    int old_tseg = VERTEX_PER_PMA;                   // INIT: 16 segs (1 per vertex)
    ap_uint<512> old_range_word = 0;
    ap_uint<512> old_ec_word = 0;
    int pma_boff = 0;
    if (!is_init) {
        int start = (int)vi.range(31, 0);
        pma_boff = start - data_off;
        int hbase = pma_boff / 64;
        old_range_word = hbm[hbase];
        old_ec_word    = hbm[hbase + 1];
        old_tseg = (int)old_range_word.range(15 * 32 + 31, 15 * 32);
    }
    int old_hlp = hlp_total_words(old_tseg);

    // New layout: vertex v gets 2x segments when it must grow (capacity doubled),
    // otherwise keeps old segment count. INIT (vi==0) allocates 1 + ceil(count/47).
    // ntseg accumulates in the same loop computing the range.
    int ntseg = 0;
    int nfe_words = 0, nvc_words = 0, nhlp = 0, npwords = 0, npbytes = 0;

    // ---- Step 1: per-vertex grow decision + new vertex_range (cumulative) ----
    // 按 vertex 粒度判断: 现有边数 + 本次插入数 > 现有容量 (旧 segment × 47)
    // → 该 vertex 需要扩容 (segment 数 2 倍翻倍)。
    // 所有 vertex 都不需要扩容 → 直接返回 (调用处照常发 ACK, 跳过 alloc/collect/
    // redistribute/FREE 整套开销)。INIT 必须 alloc (新 PMA 无空间)。
    ap_uint<512> new_range_word = 0;
    bool need_expand = is_init;
    {
        int cum = 0;
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS PIPELINE II=1
            int old_cnt = 1;
            int old_edges = 0;
            if (!is_init) {
                int vs = (v == 0) ? 0 : (int)old_range_word.range((v - 1) * 32 + 31, (v - 1) * 32);
                int ve = (int)old_range_word.range(v * 32 + 31, v * 32);
                old_cnt = ve - vs;
                old_edges = (int)old_ec_word.range(v * 32 + 31, v * 32);   // header.edge_count[v]
            }
            int new_cnt = old_cnt;
            if (is_init) {
                // 新 PMA: segment 数基于该 vertex 的插入边数 (vc 计数)。
                // 1 段容量 47 — count ≤ 47 保持 1 段 (不浪费); 超过则
                // ceil(count/47) 向上取 2 的幂 (与扩容翻倍语义一致)。
                int need = (count[v] + 46) / 47;   // ceil(count/47): 最小段数 (count=0 → 0)
                int p = 1;
                while (p < need) p <<= 1;
                new_cnt = (need <= 1) ? 1 : p;     // 至少 1 段, 2 的幂
            } else if (old_edges + count[v] > old_cnt * 47) {
                // 需扩容: 至少翻倍, 重复翻倍直到 (ve-vs)*47 >= old+count。
                // 原实现只翻倍一次 — 单个 wave 对某 vertex 的插入量超过其整个
                // 容量时扩容不足 (倾斜 dst 全打到一个热顶点就会发生), merge
                // 没有 overflow 兜底路径会写出 vc>47 的段 (bs_vs_merge 实验
                // 发现, 2026-08-11)。与 INIT 分支的 2 的幂语义保持一致。
                new_cnt = old_cnt * 2;
                need_expand = true;
                while (new_cnt * 47 < old_edges + count[v]) {
                    new_cnt *= 2;
                }
            }
            cum += new_cnt;
            new_range_word.range(v * 32 + 31, v * 32) = (ap_uint<32>)cum;
        }
        ntseg = cum;
    }
    if (!need_expand) {
        // 无需扩容: 直接返回 (redistribution_phase 调用处仍发 pre_ack)
        return;
    }
    nfe_words = fe_words_for(ntseg);
    nvc_words = vc_words_for(ntseg);
    nhlp = nfe_words + nvc_words;
    npwords = HEADER_WORDS + nhlp + ntseg * SEG_WORDS_PER_SEG;
    npbytes = npwords * 64;

    // ---- Step 2: allocate new space ----
    ap_uint<96> acmd = 0;
    acmd.range(31, 0)  = 0;
    acmd.range(63, 32) = (ap_uint<32>)npbytes;
    acmd.range(95, 64) = 1;                          // opcode=ALLOC
    ap_uint<64> arsp = alloc_send_rsp(acmd, alloc_cmd, alloc_rsp);
    int nstart = (int)arsp.range(31, 0);
    int nabs_base = nstart / 64;

    // ---- Step 3: write new header (edge counts unchanged) ----
    {
        ap_uint<512> new_hdr[HEADER_WORDS];
        new_hdr[0] = new_range_word;
        if (is_init) new_hdr[1] = 0;
        else         new_hdr[1] = old_ec_word;
        new_hdr[2] = 0;
        new_hdr[3] = 0;
        for (int w = 0; w < HEADER_WORDS; w++) {
            #pragma HLS PIPELINE II=1
            hbm[nabs_base + w] = new_hdr[w];
        }
    }

    // ---- Step 4: init SoA helper areas (FE=UNUSED_ICON, VC=0) ----
    int fe_hbm_base = nabs_base + HEADER_WORDS;
    int vc_hbm_base = fe_hbm_base + nfe_words;
    for (int w = 0; w < nfe_words; w++) {
        #pragma HLS PIPELINE II=1
        hbm[fe_hbm_base + w] = EMPTY_FE_WORD;
    }
    for (int w = 0; w < nvc_words; w++) {
        #pragma HLS PIPELINE II=1
        hbm[vc_hbm_base + w] = EMPTY_VC_WORD;
    }

    int nseg_base = nabs_base + HEADER_WORDS + nhlp;

    if (is_init) {
        // ---- INIT: no old data — just write empty segments ----
        for (int s = 0; s < ntseg; s++) {
            #pragma HLS PIPELINE II=1
            int off = s * SEG_WORDS_PER_SEG;
            hbm[nseg_base + off + 0] = EMPTY_SEG_ROM[0];
            hbm[nseg_base + off + 1] = EMPTY_SEG_ROM[1];
            hbm[nseg_base + off + 2] = EMPTY_SEG_ROM[2];
            hbm[nseg_base + off + 3] = EMPTY_SEG_ROM[3];
        }
    } else {
        // ---- ENLARGE: collect old edges → redistribute to new layout ----
        int old_seg_base = (pma_boff + HEADER_BYTES + old_hlp * 64) / 64;
        int collected_edges = 0;
        collect_edge_to_buffer(hbm, hbm_buffer,
            old_seg_base, old_seg_base + old_tseg * SEG_WORDS_PER_SEG,
            collected_edges, onchip_buffer, edge_buffer);

        int edge_cursor = 0;
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            int old_ec = (int)old_ec_word.range(v * 32 + 31, v * 32);
            int nvs = (v == 0) ? 0 : (int)new_range_word.range((v - 1) * 32 + 31, (v - 1) * 32);
            int nve = (int)new_range_word.range(v * 32 + 31, v * 32);
            int ncnt = nve - nvs;

            if (old_ec == 0) {
                int empty_base = nseg_base + nvs * SEG_WORDS_PER_SEG;
                for (int i = 0; i < ncnt; i++) {
                    #pragma HLS PIPELINE II=1
                    int off = i * SEG_WORDS_PER_SEG;
                    hbm[empty_base + off + 0] = EMPTY_SEG_ROM[0];
                    hbm[empty_base + off + 1] = EMPTY_SEG_ROM[1];
                    hbm[empty_base + off + 2] = EMPTY_SEG_ROM[2];
                    hbm[empty_base + off + 3] = EMPTY_SEG_ROM[3];
                }
                continue;
            }

            int seg_start_word = nseg_base + nvs * SEG_WORDS_PER_SEG;
            int seg_end_word   = seg_start_word + ncnt * SEG_WORDS_PER_SEG;

            redistribute_edge_from_hbm(hbm, hbm_buffer,
                seg_start_word, seg_end_word, old_ec, edge_cursor,
                nvs, vc_hbm_base, fe_hbm_base,
                onchip_buffer, edge_buffer);

            edge_cursor += old_ec;
        }

        // ---- FREE old PMA (fire-and-forget) ----
        {
            int obytes = (HEADER_WORDS + old_hlp + old_tseg * SEG_WORDS_PER_SEG) * 64;
            ap_uint<96> fcmd = 0;
            fcmd.range(31, 0)  = (ap_uint<32>)pma_boff;
            fcmd.range(63, 32) = (ap_uint<32>)obytes;
            fcmd.range(95, 64) = 2;                // opcode=FREE
            alloc_send(fcmd, alloc_cmd);
        }
        ap_wait();  // barrier: FREE 排在 VEC_INFO 更新之前 (CLAUDE.md)
    }

    // ---- Step 5: update VEC_INFO ----
    VEC_INFO nvi = 0;
    nvi.range(31, 0)  = (ap_uint<32>)(nstart + data_off);
    nvi.range(63, 32) = (ap_uint<32>)npbytes;
    vi_write(hbm, vi_wo, pma_idx, nvi);
}

// ---- Redistribution main loop ----
// Task types (in payload[31:0]):
//   -1 (0xFFFFFFFF): STOP — also signals alloc_access via alloc_cmd_stream
//   -2 (0xFFFFFFFE): ACK marker
//   seg (>=0):       overflow segment — owner vertex checked:
//                    vertex_total > capacity → ENLARGE (alloc new space, move PMA)
//                    else → power-of-2 group expansion + collect/redistribute
//
// pre_expand_in has priority: task_reader is blocked waiting for the ACK, so
// pre-expands must be serviced before rebalance tasks. This is what keeps the
// cross-kernel "先扩后插" invariant: bs_insert's overflow task for PMA X can
// only arrive after task_reader dispatched X's edges, which only happens after
// this kernel's pre_ack for X.
void redistribution_phase(
    ap_uint<512>* hbm,
    ap_uint<512>* hbm_edge_buffer,
    int data_off, int vi_wo,
    hls::stream<rebalance_task_t>& task_fifo,
    hls::stream<ap_uint<32>>& redist_ack_out,
    hls::stream<pre_expand_cmd_t>& pre_expand_in,
    hls::stream<ap_uint<32>>& pre_ack_out,
    hls::stream<ap_axiu<96,0,0,0>>& alloc_cmd_stream,
    hls::stream<ap_axiu<64,0,0,0>>& alloc_rsp_stream)
{
    // Single shared instance of the chunked helpers for all callers
    // (pma_pre_expand, pma_redist_enlarge, rebalance path)
    #pragma HLS allocation function instances=collect_edge_to_buffer limit=1
    #pragma HLS allocation function instances=redistribute_edge_from_hbm limit=1

    // On-chip buffers — declared outside while for HLS lifetime analysis
    ap_uint<512> onchip_buffer[MAX_ONCHIP_SEG_WORDS];
    #pragma HLS bind_storage variable=onchip_buffer type=RAM_2P impl=URAM
    ap_uint<512> edge_buffer[MAX_ONCHIP_SEG_WORDS];
    #pragma HLS bind_storage variable=edge_buffer type=RAM_2P impl=URAM

    // Enlarge remap state: when ENLARGE moves a PMA, the remaining tasks of the
    // same batch still carry the pre-enlarge forwarded header. They are remapped
    // to the new layout (segments at/after the enlarged vertex's old end shift
    // by delta). Cleared by the trailing -2 ACK marker of the batch.
    int remap_pma = -1;
    int remap_old_start = 0;
    int remap_boundary = 0;      // old vertex_end of the enlarged vertex
    int remap_delta = 0;         // segments added by the enlarge (0 = no state)
    int remap_new_start = 0;     // new absolute start byte
    ap_uint<512> remap_range_word = 0;

    while (true) {
        // Pre-expansion commands have priority: task_reader is blocked waiting
        // for the ACK, so they must be serviced before rebalance tasks.
        if (!pre_expand_in.empty()) {
            pre_expand_cmd_t cmd = pre_expand_in.read();
            pma_pre_expand(hbm, hbm_edge_buffer, data_off, vi_wo,
                           cmd.pma, cmd.count,
                           alloc_cmd_stream, alloc_rsp_stream,
                           onchip_buffer, edge_buffer);
            pre_ack_out.write((ap_uint<32>)cmd.pma);
            continue;
        }
        if (task_fifo.empty()) { ap_wait(); continue; }
        rebalance_task_t task = task_fifo.read();
        int payload = task.payload;
        int pm  = task.pma;
        if (payload == -1) {  // STOP
            // Signal alloc_access that redistribution is done with allocations
            ap_axiu<96,0,0,0> scmd;
            scmd.data = 0;              // opcode=STOP (bits [95:64] = 0)
            scmd.last = 1;
            scmd.keep = 0xFFF;
            alloc_cmd_stream.write(scmd);
            ap_uint<32> stop_ack = (ap_uint<32>)(-1);
            redist_ack_out.write(stop_ack);
            break;
        }
        if (payload == -2) {  // ACK marker: batch done → enlarge remap state stale
            if (pm == remap_pma) remap_delta = 0;
            redist_ack_out.write((ap_uint<32>)pm);
            continue;
        }

        // Remap tasks that predate an ENLARGE of the same PMA
        int task_start_byte = task.start_byte;
        ap_uint<512> task_range_word = task.range_word;
        int task_seg = payload;
        if (remap_delta > 0 && pm == remap_pma && task_start_byte == remap_old_start) {
            task_start_byte = remap_new_start;
            task_range_word = remap_range_word;
            if (task_seg >= remap_boundary) task_seg += remap_delta;
        } else if (pm != remap_pma || task_start_byte != remap_old_start) {
            remap_delta = 0;   // different layout context → drop stale state
        }

        // Use the (possibly remapped) forwarded header — no VEC_INFO/header HBM reads
        int pma_boff = task_start_byte - data_off;
        int hbase = pma_boff / 64;
        int total_segs = (int)task_range_word.range(15 * 32 + 31, 15 * 32);
        if (total_segs == 0) continue;
        int fe_words = fe_words_for(total_segs);
        int fe_hbm_base = hbase + HEADER_WORDS;
        int vc_hbm_base = fe_hbm_base + fe_words;
        int total_hlp    = fe_words + vc_words_for(total_segs);

        // Vertex ranges from forwarded header
        int vertex_range[VERTEX_PER_PMA];
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            vertex_range[v] = (int)task_range_word.range(v * 32 + 31, v * 32);
        }

        // Find owner vertex of the overflow segment
        int overflow_seg = task_seg;
        int owner_vertex = -1;
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            int vertex_start = (v == 0) ? 0 : vertex_range[v - 1];
            if (overflow_seg >= vertex_start && overflow_seg < vertex_range[v])
                owner_vertex = v;
        }
        if (owner_vertex < 0) continue;

        int vertex_start = (owner_vertex == 0) ? 0 : vertex_range[owner_vertex - 1];
        int vertex_end   = vertex_range[owner_vertex];
        int vertex_segs  = vertex_end - vertex_start;

        // Vertex total edges (direct HBM VC reads) → ENLARGE vs redistribute decision
        int vertex_total = 0;
        for (int s = vertex_start; s < vertex_end; s++) {
            #pragma HLS PIPELINE II=1
            vertex_total += hbm_vc_read(hbm, vc_hbm_base, s);
        }

        if (vertex_total > density_capacity(vertex_segs)) {
            // ENLARGE: whole vertex over capacity → grow the PMA
            int new_start = 0;
            ap_uint<512> new_range = 0;
            pma_redist_enlarge(hbm, hbm_edge_buffer, data_off, vi_wo, pm,
                               task_start_byte, task_range_word,
                               owner_vertex, vertex_segs,
                               alloc_cmd_stream, alloc_rsp_stream,
                               onchip_buffer, edge_buffer,
                               new_start, new_range);

            // Record layout change so remaining same-batch tasks get remapped
            remap_pma = pm;
            remap_old_start = task_start_byte;
            remap_boundary = vertex_end;
            remap_delta = (vertex_segs <= 4) ? 3 * vertex_segs : vertex_segs;
            remap_new_start = new_start;
            remap_range_word = new_range;
        } else {
            if (vertex_segs <= 1) continue;

            // SEGMENT_REBALANCE: power-of-2 group expansion around overflow segment
            int left, right, total;
            find_redistribution_range(hbm, vc_hbm_base,
                                      vertex_start, vertex_end, overflow_seg,
                                      left, right, total);

            // --- Redistribute group [left, right) via collect + redistribute ---
            int group_size     = right - left;
            int segment_words  = group_size * SEG_WORDS_PER_SEG;
            int segment_base   = (pma_boff + HEADER_BYTES + total_hlp * 64 + left * SEG_BYTES) / 64;

            // 1) Collect valid edges from segments [left, right) → hbm_edge_buffer
            int collected_edges = 0;
            collect_edge_to_buffer(hbm, hbm_edge_buffer,
                segment_base, segment_base + segment_words, collected_edges,
                onchip_buffer, edge_buffer);

            // 2) Redistribute edges evenly → hbm (also updates VC + FE helpers inline)
            redistribute_edge_from_hbm(hbm, hbm_edge_buffer,
                segment_base, segment_base + segment_words, collected_edges,
                0,  // edge_offset: read from start of hbm_buffer
                left, vc_hbm_base, fe_hbm_base,
                onchip_buffer, edge_buffer);
        }
    }
}

// ---- Top-level ----

extern "C"
{
    void redistribution_kernel(
        ap_uint<512>* hbm_red,
        ap_uint<512>* hbm_buffer_for_red,
        int pma_raw_data_start,
        int pma_vecinfo_start,
        int bitmap_offset,
        hls::stream<pre_expand_cmd_t>& pre_expand_in,
        hls::stream<rebalance_task_t>& task_in,
        hls::stream<ap_uint<32>>& pre_ack_out,
        hls::stream<ap_uint<32>>& redist_ack_out,
        hls::stream<ap_axiu<96,0,0,0>>& alloc_cmd_stream,
        hls::stream<ap_axiu<64,0,0,0>>& alloc_rsp_stream,
        hls::stream<ap_axiu<ALLOC_BITMAP_STORAGE_WORD_BITS,0,0,0>>& bitmap_init)
    {
        #pragma HLS INTERFACE m_axi port=hbm_red offset=slave bundle=gmem5 max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=hbm_buffer_for_red offset=slave bundle=gmem8 max_read_burst_length=256 max_write_burst_length=256

        #pragma HLS INTERFACE s_axilite port=hbm_red bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_buffer_for_red bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_raw_data_start bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_vecinfo_start bundle=control
        #pragma HLS INTERFACE s_axilite port=bitmap_offset bundle=control
        #pragma HLS INTERFACE s_axilite port=return bundle=control
        #pragma HLS INTERFACE axis port=pre_expand_in
        #pragma HLS INTERFACE axis port=task_in
        #pragma HLS INTERFACE axis port=pre_ack_out
        #pragma HLS INTERFACE axis port=redist_ack_out
        #pragma HLS INTERFACE axis port=alloc_cmd_stream
        #pragma HLS INTERFACE axis port=alloc_rsp_stream
        #pragma HLS INTERFACE axis port=bitmap_init

        // 单进程 (无 DATAFLOW): bitmap_init 前置 + redistribution 主循环。
        // bitmap 位于 hbm_red 同一 HBM bank (host 把 bitmap 写在 PMA bank 的
        // bitmap_offset 处), 复用 hbm_red 端口读, 无需额外 m_axi。
        bitmap_init_reader(hbm_red, bitmap_offset, bitmap_init);

        redistribution_phase(hbm_red, hbm_buffer_for_red,
                             pma_raw_data_start, pma_vecinfo_start / 64,
                             task_in, redist_ack_out,
                             pre_expand_in, pre_ack_out,
                             alloc_cmd_stream, alloc_rsp_stream);
    }
}
