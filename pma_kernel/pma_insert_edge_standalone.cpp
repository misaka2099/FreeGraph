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

#define BS_WORKER_NUM 4
#define HEADER_WORDS ((int)(sizeof(PMA_HEADER) / 64))
#define SEG_WORDS_PER_SEG (SEGMENT_SIZE * (int)sizeof(int) / 64)
#define HEADER_BYTES (HEADER_WORDS * 64)
#define SEG_BYTES (SEG_WORDS_PER_SEG * 64)
// Max segments per on-chip chunk, used to size pma_buffer / seg_buf_rd / onchip_buffer.
// 1024 segments × 4 words = 4096 × 512-bit words = 256 KiB on-chip.

// Helper SoA layout constants:
//   first_edge area: 1 entry per HELPER_FE_GRANULARITY segments → 16 entries per 512-bit word
//   valid_count area: 1 entry per segment → 16 entries per 512-bit word
// No more per-segment AoE — reduced HBM footprint and burst-friendly.
static const int HELPER_FE_GRANULARITY = 16;   // 1 first_edge manages 16 segments
static const int FE_SOA_ENTRIES_PER_WORD = 16;  // SoA: 16 × 32-bit per 512-bit word
static const int VC_SOA_ENTRIES_PER_WORD = 16;  // SoA: 16 × 32-bit per 512-bit word
// Empty helper words: first_edge = all UNUSED_ICON, valid_count = all 0
static const ap_uint<512> EMPTY_FE_WORD = ~ap_uint<512>(0);  // all UNUSED_ICON
static const ap_uint<512> EMPTY_VC_WORD = 0;                  // all zero

// Max segments per on-chip chunk — sizes pma_buffer / seg_buf_rd / onchip_buffer.
static const int MAX_ONCHIP_SEG = 1024;
static const int MAX_ONCHIP_SEG_WORDS = MAX_ONCHIP_SEG * SEG_WORDS_PER_SEG;  // 4096 words
static const int VC_CHUNK_ENTRIES = MAX_ONCHIP_SEG;   // aligned with segment chunk
static const int VC_CHUNK_WORDS   = MAX_ONCHIP_SEG / VC_SOA_ENTRIES_PER_WORD;  // 64 words
static const int FE_CHUNK_ENTRIES = MAX_ONCHIP_SEG / HELPER_FE_GRANULARITY;  // 64 entries per chunk
static const int FE_CHUNK_WORDS   = FE_CHUNK_ENTRIES / FE_SOA_ENTRIES_PER_WORD;  // 4 words

// Full-PMA first_edge cache: worst case 256MB HBM channel → 1M segs → 65536 fe entries
// 256MB / 4B / SEGMENT_SIZE(64) / HELPER_FE_GRANULARITY(16) = 65536 entries = 256KB URAM
static const int FE_CACHE_ENTRIES = 65536;
static const int FE_CACHE_WORDS   = FE_CACHE_ENTRIES / FE_SOA_ENTRIES_PER_WORD;  // 4096 words

// Full-PMA valid_count cache: 1 entry per segment, 1M segs max → 4MB URAM
// 256MB / 256B per seg = 1,048,576 segs = 1M entries × 4B = 4MB
static const int VC_CACHE_ENTRIES = 1048576;
static const int VC_CACHE_WORDS   = VC_CACHE_ENTRIES / VC_SOA_ENTRIES_PER_WORD;  // 65536 words
// Forwarded header info: pma_gate → bs_worker → insert_phase
// Eliminates redundant VEC_INFO + header HBM reads in downstream stages
struct fwd_header_t {
    int start_byte;              // absolute HBM byte offset of PMA data
    ap_uint<512> range_word;     // header[0]: vertex ranges [15:0] + total_segs[511:480]
    ap_uint<512> edge_count_word;// header[1]: per-vertex edge counts
};

// Rebalance task: insert_phase → redistribution_phase
// Carries forwarded header to avoid redundant VEC_INFO/header HBM reads
struct rebalance_task_t {
    int pma;                     // PMA index
    int payload;                 // segment index (>=0), -1=STOP, -2=ACK marker
    int start_byte;              // forwarded: PMA absolute HBM byte offset
    ap_uint<512> range_word;     // forwarded: header[0] — vertex ranges + total_segs
};

static const ap_uint<512> EMPTY_SEG_ROM[4] = {
    ~ap_uint<512>(0),            // all ones
    ~ap_uint<512>(0),            // all ones
    ~ap_uint<512>(0),            // all ones
    (~ap_uint<512>(0)) >> 32     // bits[511:480]=0, rest all ones
};

// Pre-computed header[0] for empty PMA: 16 vertices × 1 seg, total_segs=16
static const ap_uint<512> EMPTY_PMA_HDR0 =
    (ap_uint<512>(16) << 480) |
    (ap_uint<512>(15) << 448) |
    (ap_uint<512>(14) << 416) |
    (ap_uint<512>(13) << 384) |
    (ap_uint<512>(12) << 352) |
    (ap_uint<512>(11) << 320) |
    (ap_uint<512>(10) << 288) |
    (ap_uint<512>( 9) << 256) |
    (ap_uint<512>( 8) << 224) |
    (ap_uint<512>( 7) << 192) |
    (ap_uint<512>( 6) << 160) |
    (ap_uint<512>( 5) << 128) |
    (ap_uint<512>( 4) <<  96) |
    (ap_uint<512>( 3) <<  64) |
    (ap_uint<512>( 2) <<  32) |
    (ap_uint<512>( 1) <<   0);

// ---- HBM access helpers (m_axi) ----

static void hbm_read_words(ap_uint<512>* hbm_ptr, int byte_off, int words, ap_uint<512>* buf)
{
    int base = byte_off / 64;
    for (int w = 0; w < words; w++) {
        #pragma HLS PIPELINE II=1
        buf[w] = hbm_ptr[base + w];
    }
}

static void hbm_write_words(ap_uint<512>* hbm_ptr, int byte_off, int words, ap_uint<512>* buf)
{
    int base = byte_off / 64;
    for (int w = 0; w < words; w++) {
        #pragma HLS PIPELINE II=1
        hbm_ptr[base + w] = buf[w];
    }
}

// ---- SoA helper layout helpers ----
// first_edge area: 1 entry per HELPER_FE_GRANULARITY (16) segments
// valid_count area: 1 entry per segment
// Both areas: 16 × 32-bit entries per 512-bit word
static inline int fe_words_for(int segs) {
    int fe_entries = (segs + HELPER_FE_GRANULARITY - 1) / HELPER_FE_GRANULARITY;
    return (fe_entries + FE_SOA_ENTRIES_PER_WORD - 1) / FE_SOA_ENTRIES_PER_WORD;
}
static inline int vc_words_for(int segs) {
    return (segs + VC_SOA_ENTRIES_PER_WORD - 1) / VC_SOA_ENTRIES_PER_WORD;
}
static inline int hlp_total_words(int segs) { return fe_words_for(segs) + vc_words_for(segs); }

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

// ---- Binary search helper ----

static inline int density_capacity(int seg_cnt) { return seg_cnt * 47; }

void sub_search(int* helper, int lo, int hi, int dst, int &result)
{
    #pragma HLS INLINE off
    int low = lo, high = hi - 1;
    result = -1;
    while (low <= high) {
        #pragma HLS PIPELINE off
        int mid = (low + high) / 2;
        if (helper[mid] == dst) { result = mid - lo; return; }
        else if (helper[mid] < dst) { low = mid + 1; result = mid - lo; }
        else { high = mid - 1; }
    }
}

// ---- Segment Insert (pure compute, no HBM access) ----
// Processes one segment: extract edges, insert new dsts, pack back.
// Designed for multiple parallel instances (INLINE off).

void seg_insert(
    ap_uint<512> seg_in[SEG_WORDS_PER_SEG],
    int dsts[16],
    int dst_cnt,
    ap_uint<512> seg_out[SEG_WORDS_PER_SEG],
    int &new_valid,
    bool &overflow
) {
    #pragma HLS INLINE off

    int valid = (int)seg_in[3].range(511, 480);

    // Extract existing edges into edge_buf
    int edge_buf[EDGE_SLOTS_PER_SEG];
    // #pragma HLS ARRAY_PARTITION variable=edge_buf complete
    for (int k = 0; k < EDGE_SLOTS_PER_SEG; k++) {
        #pragma HLS UNROLL
        int wi = k / 16, slot = k % 16;
        edge_buf[k] = (k < valid) ? (int)seg_in[wi].range(slot * 32 + 31, slot * 32) : UNUSED_ICON;
    }

    // Insert each new edge using binary search for O(log N) position finding
    // edge_buf[0..valid-1] is sorted ascending — binary search replaces 48-way
    // parallel comparator+popcount with ~6 sequential comparisons, saving LUTs
    // and shortening the critical path at negligible latency cost (pma_gate
    // HBM access dominates the pipeline anyway).
    for (int ei = 0; ei < dst_cnt; ei++) {
        #pragma HLS PIPELINE II=2
        int dst = dsts[ei];
        if (valid >= EDGE_SLOTS_PER_SEG) break;

        // Binary search for insertion position
        int lo = 0, hi = valid;
        while (lo < hi) {
            #pragma HLS PIPELINE II=1
            int mid = (lo + hi) / 2;
            if (edge_buf[mid] < dst)
                lo = mid + 1;
            else
                hi = mid;
        }
        int pos = lo;

        // Shift elements right to make room
        for (int k = valid; k > pos; k--) {
            // #pragma HLS UNROLL
            edge_buf[k] = edge_buf[k - 1];
        }
        edge_buf[pos] = dst;
        valid++;
    }

    overflow   = (valid > 47);
    new_valid  = valid;

    // Pack back into seg_out (UNUSED_ICON padding)
    for (int k = 0; k < EDGE_SLOTS_PER_SEG; k++) {
        #pragma HLS UNROLL
        int wi = k / 16, slot = k % 16;
        int val = (k < valid) ? edge_buf[k] : UNUSED_ICON;
        seg_out[wi].range(slot * 32 + 31, slot * 32) = (ap_uint<32>)val;
    }
    seg_out[3].range(511, 480) = (ap_uint<32>)valid;
}

// ---- pma_gate helper: handle existing (non-empty) PMA ----
// Extracted to a separate function to prevent HLS if-conversion
// from speculating alloc/free/m_axi writes when vi==0.
static void pma_gate_handle_existing(
    ap_uint<512>* hbm_gate,
    int pma_raw_data_start,
    int pma_vecinfo_start,
    int pma_idx,
    VEC_INFO vi,
    PMA_UPDATE_MESSAGE* cmd_batch,
    int cmd_count,
    ap_uint<512>* pma_buffer,
    hls::stream<ap_axiu<96,0,0,0>>& alloc_cmd_stream,
    hls::stream<ap_axiu<64,0,0,0>>& alloc_rsp_stream,
    hls::stream<fwd_header_t>& hdr_fwd)
{
    ap_uint<512> range_word = hbm_gate[(int)(vi.range(31,0) - pma_raw_data_start) / 64];
    ap_uint<512> edge_count_word = hbm_gate[(int)(vi.range(31,0) - pma_raw_data_start) / 64 + 1];
    int delta_edge[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    int delta_range[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    bool enlarge = false;
    int total_delta = 0;
    for (int i = 0 ; i < cmd_count ; i++)
    {
        #pragma HLS PIPELINE II=1
        PMA_UPDATE_MESSAGE cmd = cmd_batch[i];
        int local_src = cmd.src & 0xF;
        delta_edge[local_src]++;
    }
    for (int i = 0 ; i < 16 ; i++)
    {
        #pragma HLS PIPELINE II=1
        if (delta_edge[i] == 0) continue;
        int old_count = (int)edge_count_word.range(i * 32 + 31, i * 32);
        int new_count = old_count + delta_edge[i];
        int seg_start = (i == 0) ? 0 : (int)range_word.range((i - 1) * 32 + 31, (i - 1) * 32);
        int seg_end = (int)range_word.range(i * 32 + 31, i * 32);

        int seg_len = seg_end - seg_start;
        int max_edge = density_capacity(seg_len);
        if (new_count > max_edge)
        {
            // Tiered growth: ×4 for small vertices (fast ramp-up),
            // ×2 for large vertices (limit wasted space)
            delta_range[i] = seg_len;
            total_delta += delta_range[i];
            enlarge = true;
        }
    }
    if (enlarge)
    {
        int old_tseg = (int)range_word.range(15 * 32 + 31, 15 * 32);
        int ntseg = old_tseg + total_delta;
        int nfe_words = fe_words_for(ntseg);
        int nvc_words = vc_words_for(ntseg);
        int nhlp      = nfe_words + nvc_words;
        int npwords   = HEADER_WORDS + nhlp + ntseg * SEG_WORDS_PER_SEG;
        int npbytes   = npwords * 64;

        // ==== Step 1: send alloc request for new space ====
        ap_axiu<96,0,0,0> acmd2;
        acmd2.data.range(31, 0)  = 0;
        acmd2.data.range(63, 32) = npbytes;
        acmd2.data.range(95, 64) = 1;  // opcode=ALLOC
        acmd2.last = 0; acmd2.keep = 0xFFF;
        alloc_cmd_stream.write(acmd2);
        #pragma HLS protocol fixed

        // ==== Step 2: compute new header + old layout (while alloc in flight) ====
        ap_uint<512> old_range_word = range_word;
        int acc = 0;
        for (int i = 0 ; i < 16 ; i++)
        {
            #pragma HLS PIPELINE II=1
            acc += delta_range[i];
            int old_vr = (int)range_word.range(i * 32 + 31, i * 32);
            int new_vr = old_vr + acc;
            range_word.range(i * 32 + 31, i * 32) = new_vr;
        }

        int old_boff     = (int)vi.range(31, 0) - pma_raw_data_start;
        int old_hlp      = hlp_total_words(old_tseg);
        int old_seg_base = (old_boff + HEADER_BYTES + old_hlp * 64) / 64;

        // ==== Step 3: receive new address ====
        ap_wait();
        ap_axiu<64,0,0,0> arsp2 = alloc_rsp_stream.read();
        int nstart = (int)arsp2.data.range(31, 0);

        // ==== Step 4: write new header to new location ====
        {
            ap_uint<512> new_hdr[HEADER_WORDS];
            new_hdr[0] = range_word;
            new_hdr[1] = edge_count_word;
            new_hdr[2] = 0; new_hdr[3] = 0;
            int new_abs_base = nstart / 64;
            for (int w = 0; w < HEADER_WORDS; w++) {
                #pragma HLS PIPELINE II=1
                hbm_gate[new_abs_base + w] = new_hdr[w];
            }
        }

        // ==== Step 5: chunk-based segment transfer + helper update ====
        int nseg_base      = nstart / 64 + HEADER_WORDS + nhlp;
        int fe_hbm_base    = nstart / 64 + HEADER_WORDS;
        int vc_hbm_base    = fe_hbm_base + nfe_words;

        // 5a: init SoA helper on new HBM: first_edge=UNUSED_ICON, valid_count=0
        for (int w = 0; w < nfe_words; w++) {
            #pragma HLS PIPELINE II=1
            hbm_gate[fe_hbm_base + w] = EMPTY_FE_WORD;
        }
        for (int w = 0; w < nvc_words; w++) {
            #pragma HLS PIPELINE II=1
            hbm_gate[vc_hbm_base + w] = EMPTY_VC_WORD;
        }

        // 5b: per-vertex transfer with chunk protection
        for (int v = 0; v < 16; v++) {
            int ovs = (v == 0) ? 0 : (int)old_range_word.range((v - 1) * 32 + 31, (v - 1) * 32);
            int ove = (int)old_range_word.range(v * 32 + 31, v * 32);
            int nvs = (v == 0) ? 0 : (int)range_word.range((v - 1) * 32 + 31, (v - 1) * 32);
            int nve = (int)range_word.range(v * 32 + 31, v * 32);
            int ocnt = ove - ovs;
            int ncnt = nve - nvs;

            // Transfer old segments in MAX_ONCHIP_SEG chunks
            for (int chunk_off = 0; chunk_off < ocnt; chunk_off += MAX_ONCHIP_SEG) {
                int chunk_segs = ocnt - chunk_off;
                if (chunk_segs > MAX_ONCHIP_SEG) chunk_segs = MAX_ONCHIP_SEG;
                int chunk_words = chunk_segs * SEG_WORDS_PER_SEG;

                // burst-read from old HBM
                int old_base = old_seg_base + (ovs + chunk_off) * SEG_WORDS_PER_SEG;
                for (int i = 0; i < chunk_words; i++) {
                    #pragma HLS PIPELINE II=1
                    pma_buffer[i] = hbm_gate[old_base + i];
                }

                // burst-write to new HBM
                int new_base = nseg_base + (nvs + chunk_off) * SEG_WORDS_PER_SEG;
                for (int i = 0; i < chunk_words; i++) {
                    #pragma HLS PIPELINE II=1
                    hbm_gate[new_base + i] = pma_buffer[i];
                }

                // SoA helper RMW: valid_count area + first_edge area (separate HBM regions)
                int seg_start  = nvs + chunk_off;
                int seg_end    = nvs + chunk_off + chunk_segs;

                // --- RMW valid_count area: 1 entry per segment ---
                int vc_word_start = seg_start / VC_SOA_ENTRIES_PER_WORD;
                int vc_word_end   = (seg_end + VC_SOA_ENTRIES_PER_WORD - 1) / VC_SOA_ENTRIES_PER_WORD;
                int vc_word_count = vc_word_end - vc_word_start;
                ap_uint<512> vc_buf[VC_CHUNK_WORDS];
                #pragma HLS bind_storage variable=vc_buf type=RAM_2P impl=BRAM
                for (int w = 0; w < vc_word_count; w++) {
                    #pragma HLS PIPELINE II=1
                    vc_buf[w] = hbm_gate[vc_hbm_base + vc_word_start + w];
                }
                for (int s = 0; s < chunk_segs; s++) {
                    #pragma HLS PIPELINE II=1
                    int nseg  = seg_start + s;
                    int local_word = nseg / VC_SOA_ENTRIES_PER_WORD - vc_word_start;
                    int local_slot = nseg % VC_SOA_ENTRIES_PER_WORD;
                    int pb = s * SEG_WORDS_PER_SEG;
                    vc_buf[local_word].range(local_slot * 32 + 31, local_slot * 32)
                        = pma_buffer[pb + 3].range(511, 480);  // valid_count
                }
                for (int w = 0; w < vc_word_count; w++) {
                    #pragma HLS PIPELINE II=1
                    hbm_gate[vc_hbm_base + vc_word_start + w] = vc_buf[w];
                }

                // --- RMW first_edge area: 1 entry per 16-segment group ---
                int fe_group_start = seg_start / HELPER_FE_GRANULARITY;
                int fe_group_end   = (seg_end + HELPER_FE_GRANULARITY - 1) / HELPER_FE_GRANULARITY;
                int fe_word_start  = fe_group_start / FE_SOA_ENTRIES_PER_WORD;
                int fe_word_end    = (fe_group_end + FE_SOA_ENTRIES_PER_WORD - 1) / FE_SOA_ENTRIES_PER_WORD;
                int fe_word_count  = fe_word_end - fe_word_start;
                ap_uint<512> fe_buf[FE_CHUNK_WORDS];  // at most ceil(1024/16/16)=4 words
                #pragma HLS bind_storage variable=fe_buf type=RAM_2P impl=BRAM
                for (int w = 0; w < fe_word_count; w++) {
                    #pragma HLS PIPELINE II=1
                    fe_buf[w] = hbm_gate[fe_hbm_base + fe_word_start + w];
                }
                for (int s = 0; s < chunk_segs; s++) {
                    #pragma HLS PIPELINE II=1
                    int nseg = seg_start + s;
                    int group_idx = nseg / HELPER_FE_GRANULARITY;
                    // only set first_edge for the first segment in each group
                    // (segments are sorted by first_edge within each vertex, so the
                    //  first segment in the group has the minimum first_edge)
                    if (nseg % HELPER_FE_GRANULARITY == 0) {
                        int local_word = group_idx / FE_SOA_ENTRIES_PER_WORD - fe_word_start;
                        int local_slot = group_idx % FE_SOA_ENTRIES_PER_WORD;
                        int pb = s * SEG_WORDS_PER_SEG;
                        fe_buf[local_word].range(local_slot * 32 + 31, local_slot * 32)
                            = pma_buffer[pb].range(31, 0);  // first_edge
                    }
                }
                for (int w = 0; w < fe_word_count; w++) {
                    #pragma HLS PIPELINE II=1
                    hbm_gate[fe_hbm_base + fe_word_start + w] = fe_buf[w];
                }
            }

            // Write empty segments for expanded portion (if any)
            // Helper already EMPTY_FE_WORD/EMPTY_VC_WORD from 5a — no update needed.
            int extra = ncnt - ocnt;
            if (extra > 0) {
                int empty_base = nseg_base + (nvs + ocnt) * SEG_WORDS_PER_SEG;
                for (int i = 0; i < extra; i++) {
                    #pragma HLS PIPELINE II=1
                    int off = i * SEG_WORDS_PER_SEG;
                    hbm_gate[empty_base + off + 0] = EMPTY_SEG_ROM[0];
                    hbm_gate[empty_base + off + 1] = EMPTY_SEG_ROM[1];
                    hbm_gate[empty_base + off + 2] = EMPTY_SEG_ROM[2];
                    hbm_gate[empty_base + off + 3] = EMPTY_SEG_ROM[3];
                }
            }
        }

        // ==== Step 6: free old address + update vecinfo ====
        {
            int obytes = (HEADER_WORDS + old_hlp + old_tseg * SEG_WORDS_PER_SEG) * 64;
            ap_axiu<96,0,0,0> fcmd;
            fcmd.data.range(31, 0)  = old_boff;
            fcmd.data.range(63, 32) = obytes;
            fcmd.data.range(95, 64) = 2;  // opcode=FREE
            fcmd.last = 0; fcmd.keep = 0xFFF;
            alloc_cmd_stream.write(fcmd);
        }

        VEC_INFO nvi = 0;
        nvi.range(31, 0)  = (ap_uint<32>)(nstart + pma_raw_data_start);
        nvi.range(63, 32) = (ap_uint<32>)npbytes;
        vi_write(hbm_gate, pma_vecinfo_start / 64, pma_idx, nvi);

        // Forward NEW header to bs_worker
        {
            fwd_header_t fh;
            fh.start_byte = nstart + pma_raw_data_start;
            fh.range_word = range_word;
            fh.edge_count_word = edge_count_word;
            hdr_fwd.write(fh);
        }
    } else {
        // No enlarge: forward ORIGINAL header to bs_worker
        fwd_header_t fh;
        fh.start_byte = (int)vi.range(31, 0);
        fh.range_word = range_word;
        fh.edge_count_word = edge_count_word;
        hdr_fwd.write(fh);
    }
}

void pma_gate(
    ap_uint<512> *hbm_gate,
    int pma_raw_data_start,
    int pma_vecinfo_start,
    int bitmap_offset,
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_in,
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_out,
    hls::stream<ap_axiu<96,0,0,0>>& alloc_cmd_stream,
    hls::stream<ap_axiu<64,0,0,0>>& alloc_rsp_stream,
    hls::stream<ap_axiu<ALLOC_BITMAP_STORAGE_WORD_BITS,0,0,0>>& bitmap_init,
    hls::stream<fwd_header_t>& hdr_fwd)
{
    ap_uint<512> pma_buffer[4096];
    #pragma HLS bind_storage variable=pma_buffer type=RAM_2P impl=URAM

    // ---- Init bitmap: read from hbm_gate, send to alloc_access ----
    // Runs once before processing any commands. Uses hbm_gate (this function's
    // only m_axi), so no DATAFLOW conflict with pre-DATAFLOW sequential code.
    int bm_word_off = bitmap_offset / 64;
    for (int i = 0; i < ALLOC_BITMAP_WORDS; i++) {
        // #pragma HLS PIPELINE II=1
        ap_axiu<ALLOC_BITMAP_STORAGE_WORD_BITS,0,0,0> w;
        w.data = hbm_gate[bm_word_off + i];
        w.last = 0;
        w.keep = -1;
        bitmap_init.write(w);
    }

    PMA_UPDATE_MESSAGE cmd_batch[MAX_CMD_NUM];

    while (true)
    {
        if (cmd_in.empty()) { ap_wait(); continue; }
        PMA_UPDATE_MESSAGE first = cmd_in.read();
        if (first.opcode == PMA_UPDATE_STOP && first.last == 1) {
            cmd_out.write(first);
            // Send STOP to alloc_access
            ap_axiu<96,0,0,0> scmd;
            scmd.data.range(95, 64) = 0;  // opcode=STOP
            scmd.last = 1; scmd.keep = 0xFFF;
            alloc_cmd_stream.write(scmd);
            return;
        }
        cmd_batch[0] = first;
        int cmd_count = 1;
        int pma_idx = first.src >> 4;
        bool done = (first.last == 1);
        for (int i = 1 ; i < MAX_CMD_NUM && !done; i++) {
            if (done) break;
            PMA_UPDATE_MESSAGE cmd = cmd_in.read();
            cmd_batch[i] = cmd; cmd_count++;
            done = (cmd.last == 1);
        }

        // check empty pma
        VEC_INFO vi = vi_read(hbm_gate, pma_vecinfo_start / 64, pma_idx);
        // hls::print("GATE vi=%d\n", (unsigned)(int)vi.range(31,0));
        if (vi == 0)
        {
            // Initialize empty PMA: header + SoA helper + 16 segments
            int isegs    = VERTEX_PER_PMA;                              // 16 segments
            int ife_words = fe_words_for(isegs);                        // first_edge area
            int ivc_words = vc_words_for(isegs);                        // valid_count area
            int ihlp      = ife_words + ivc_words;                      // total helper words

            // Alloc new PMA space
            int ibytes = (HEADER_WORDS + ihlp + isegs * SEG_WORDS_PER_SEG) * 64;
            ap_axiu<96,0,0,0> acmd;
            acmd.data.range(31, 0)  = 0;        // start=0 for ALLOC
            acmd.data.range(63, 32) = ibytes;   // request bytes
            acmd.data.range(95, 64) = 1;        // opcode=ALLOC
            acmd.last = 0; acmd.keep = 0xFFF;
            alloc_cmd_stream.write(acmd);
            #pragma HLS protocol fixed

            // ---- Build PMA in pma_buffer from ROM WHILE alloc is in flight ----
            // Header: from ROM
            pma_buffer[0] = EMPTY_PMA_HDR0;
            pma_buffer[1] = 0;
            pma_buffer[2] = 0;
            pma_buffer[3] = 0;

            // Helper SoA: first_edge area (all UNUSED_ICON) + valid_count area (all 0)
            int hlp_off = HEADER_WORDS;
            for (int w = 0; w < ife_words; w++) {
                #pragma HLS PIPELINE II=1
                pma_buffer[hlp_off + w] = EMPTY_FE_WORD;
            }
            for (int w = 0; w < ivc_words; w++) {
                #pragma HLS PIPELINE II=1
                pma_buffer[hlp_off + ife_words + w] = EMPTY_VC_WORD;
            }

            // Segments: 16 × EMPTY_SEG_ROM
            int seg_off = HEADER_WORDS + ihlp;
            for (int i = 0; i < isegs * SEG_WORDS_PER_SEG; i += SEG_WORDS_PER_SEG) {
                #pragma HLS PIPELINE II=1
                pma_buffer[seg_off + i + 0] = EMPTY_SEG_ROM[0];
                pma_buffer[seg_off + i + 1] = EMPTY_SEG_ROM[1];
                pma_buffer[seg_off + i + 2] = EMPTY_SEG_ROM[2];
                pma_buffer[seg_off + i + 3] = EMPTY_SEG_ROM[3];
            }

            // Alloc latency hidden: wait for response, then write to HBM
            ap_wait();
            ap_axiu<64,0,0,0> arsp = alloc_rsp_stream.read();
            int nstart = (int)arsp.data.range(31, 0);

            int write_start_in_512_pattern = nstart / 64;
            int ibody_words = HEADER_WORDS + ihlp + isegs * SEG_WORDS_PER_SEG;
            for (int i = 0; i < ibody_words; i++) {
                #pragma HLS PIPELINE II=1
                hbm_gate[write_start_in_512_pattern + i] = pma_buffer[i];
            }

            // Update VEC_INFO so subsequent accesses see the PMA
            VEC_INFO nvi = 0;
            nvi.range(31, 0)  = (ap_uint<32>)(nstart + pma_raw_data_start);
            nvi.range(63, 32) = (ap_uint<32>)ibytes;
            vi_write(hbm_gate, pma_vecinfo_start / 64, pma_idx, nvi);

            // Forward NEW header to bs_worker (post-init)
            {
                fwd_header_t fh;
                fh.start_byte = nstart + pma_raw_data_start;
                fh.range_word = pma_buffer[0];  // hdr0: vertex ranges
                fh.edge_count_word = 0;          // hdr1: all edge counts zero
                hdr_fwd.write(fh);
            }
        }
        //check pma enlargement
        else
        {
            // hls::print("GATE else enter\n");
            pma_gate_handle_existing(hbm_gate, pma_raw_data_start, pma_vecinfo_start,
                                     pma_idx, vi, cmd_batch, cmd_count, pma_buffer,
                                     alloc_cmd_stream, alloc_rsp_stream, hdr_fwd);
        }
        for (int i = 0 ; i < cmd_count; i++)
        {
            cmd_out.write(cmd_batch[i]);
        }
    }
}

// ---- Phase 1: BS Worker ----
// Reads header+helper directly from HBM via m_axi, does binary search

void bs_worker(
    ap_uint<512>* hbm,
    int data_off,
    hls::stream<fwd_header_t>& hdr_fwd,
    hls::stream<PMA_UPDATE_MESSAGE>& edge_in,
    hls::stream<bs_batch_t>& bs_out
)
{
    // Buffers declared outside while for HLS lifetime analysis
    PMA_UPDATE_MESSAGE edges[MAX_CMD_NUM];
    int vr[VERTEX_PER_PMA];
    #pragma HLS ARRAY_PARTITION variable=vr complete

    // first_edge_cache: holds ALL fe entries for the PMA (worst-case 256KB URAM)
    int first_edge_cache[FE_CACHE_ENTRIES];
    #pragma HLS bind_storage variable=first_edge_cache type=RAM_2P impl=URAM

    while (true) {
        // Read first edge to determine batch type and pma_idx
        // #pragma HLS PIPELINE II=1
        PMA_UPDATE_MESSAGE first = edge_in.read();
        if (first.opcode == PMA_UPDATE_STOP && first.last == 1) {
            bs_batch_t s; s.pma_idx = -1; s.count = 0; bs_out.write(s); break;
        }

        int pma = first.src >> 4;
        // Read remaining edges until last flag; count = position of last
        edges[0] = first;
        int cnt = 1;
        if (first.last != 1) {
            for (int i = 1; i < MAX_CMD_NUM; i++) {
                #pragma HLS PIPELINE II=1
                edges[i] = edge_in.read();
                cnt++;
                if (edges[i].last == 1) break;
            }
        }

        // Read forwarded header from pma_gate (avoids VEC_INFO + header HBM reads)
        fwd_header_t fh = hdr_fwd.read();
        int byte_off = fh.start_byte - data_off;
        int hbase = byte_off / 64;
        int total_segs = (int)fh.range_word.range(15 * 32 + 31, 15 * 32);

        // Extract vertex_range from forwarded header
        for (int v = 0; v < VERTEX_PER_PMA; v++)
            #pragma HLS UNROLL
            vr[v] = (int)fh.range_word.range(v * 32 + 31, v * 32);

        int fe_hbm_base = hbase + HEADER_WORDS;
        bs_batch_t res;
        res.pma_idx = pma; res.op_mask = 0; res.count = cnt;
        res.start_byte = fh.start_byte;
        res.range_word = fh.range_word;
        res.edge_count_word = fh.edge_count_word;

        // Pre-fill dst / local_src for all edges
        for (int i = 0; i < MAX_CMD_NUM; i++) {
            #pragma HLS PIPELINE II=1
            if (i < cnt) {
                res.dst[i]       = edges[i].dst;
                res.local_src[i] = edges[i].src & 0xF;
                res.target_group_start[i] = 0;
            }
        }

        // One burst: read ALL first_edge entries for this PMA once
        // Burst-read fe words → extract inline, no intermediate buffer
        int total_fe_entries = (total_segs + HELPER_FE_GRANULARITY - 1) / HELPER_FE_GRANULARITY;
        int total_fe_words   = fe_words_for(total_segs);

        for (int w = 0; w < total_fe_words; w++) {
            #pragma HLS PIPELINE II=1
            ap_uint<512> fe_word = hbm[fe_hbm_base + w];
            for (int s = 0; s < FE_SOA_ENTRIES_PER_WORD; s++) {
                #pragma HLS UNROLL
                int entry_idx = w * FE_SOA_ENTRIES_PER_WORD + s;
                if (entry_idx < total_fe_entries)
                    first_edge_cache[entry_idx] = (int)fe_word.range(s * 32 + 31, s * 32);
            }
        }

        // Search each edge against first_edge_cache — UNROLLed over cmd
        for (int i = 0; i < MAX_CMD_NUM; i++) {
            #pragma HLS UNROLL
            if (i < cnt) {
                int op = (int)edges[i].opcode;
                int v  = res.local_src[i];
                int vs = (v == 0) ? 0 : vr[v - 1];
                int ve = vr[v];
                int dst = edges[i].dst;

                int fe_entry_start = vs / HELPER_FE_GRANULARITY;
                int fe_entries     = (ve - vs + HELPER_FE_GRANULARITY - 1)
                                        / HELPER_FE_GRANULARITY;

                // Binary search on fe cache slice → target group
                int group_rel = -1;
                sub_search(&first_edge_cache[fe_entry_start], 0, fe_entries, dst, group_rel);
                if (group_rel == -1) group_rel = 0;
                res.target_group_start[i] = vs + group_rel * HELPER_FE_GRANULARITY;
            }
        }
        bs_out.write(res);
    }
}

// ---- Phase 2+3: Insert (segment modification + enlarge) ----
// Pushes modified segments to task_fifo for deferred redistribution
// ACKs are sent by redistribution_phase after all tasks are processed

void insert_phase(
    ap_uint<512>* hbm,
    int data_off,
    hls::stream<bs_batch_t>& bs_in,
    hls::stream<rebalance_task_t>& task_fifo,
    hls::stream<ap_uint<32>>& redist_ack_in,
    hls::stream<ap_uint<32>>& ack_out)
{
    int pending_cnt = 0;

    while (true) {
        if (!redist_ack_in.empty()) {
            ap_uint<32> ack_val = redist_ack_in.read();
            int ack_pma = (int)ack_val;
            if (ack_pma != -1) {
                pending_cnt--;
                ack_out.write((ap_uint<32>)ack_pma);
            }
        }

        if (bs_in.empty()) { ap_wait(); continue; }
        bs_batch_t b = bs_in.read();
        if (b.pma_idx == -1) { break; }

        int pma = b.pma_idx;
        int boff = b.start_byte - data_off;
        int hbase = boff / 64;
        int total_segs = (int)b.range_word.range(15 * 32 + 31, 15 * 32);

        int fe_words = fe_words_for(total_segs);
        int vc_words = vc_words_for(total_segs);
        int vc_hbm_base = hbase + HEADER_WORDS + fe_words;
        int total_hlp_words = fe_words + vc_words;

        // Vertex ranges and edge counts from forwarded header
        int ec[VERTEX_PER_PMA], vr[VERTEX_PER_PMA];
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            vr[v] = (int)b.range_word.range(v * 32 + 31, v * 32);
            ec[v] = (int)b.edge_count_word.range(v * 32 + 31, v * 32);
        }

        int ec_delta[VERTEX_PER_PMA];
        for (int v = 0; v < VERTEX_PER_PMA; v++) ec_delta[v] = 0;

        // --- Fine search: target_group_start → exact segment ---
        // Each group covers up to 16 segments, bounded by vertex end.
        // Group read is cached — if multiple edges share the same group, read once.
        ap_uint<512> group_buf[HELPER_FE_GRANULARITY * SEG_WORDS_PER_SEG];
        #pragma HLS bind_storage variable=group_buf type=RAM_2P impl=BRAM
        int exact_seg[16];
        int cached_group = -1, cached_group_count = 0;

        for (int i = 0; i < b.count; i++) {
            int tg = b.target_group_start[i];
            int v  = b.local_src[i];
            int vs = (v == 0) ? 0 : vr[v - 1];
            int ve = vr[v];
            int group_end = tg + HELPER_FE_GRANULARITY;
            if (group_end > ve) group_end = ve;
            int gcnt = group_end - tg;
            ec_delta[v]++;

            // Burst-read group if not cached
            if (tg != cached_group) {
                int group_base = (boff + HEADER_BYTES + total_hlp_words * 64
                                  + tg * SEG_BYTES) / 64;
                for (int w = 0; w < gcnt * SEG_WORDS_PER_SEG; w++) {
                    #pragma HLS PIPELINE II=1
                    group_buf[w] = hbm[group_base + w];
                }
                cached_group = tg;
                cached_group_count = gcnt;
            }

            // Search: rightmost non-empty segment with first_edge <= dst
            int dst = b.dst[i];
            int seg_rel = 0;
            bool found_nonempty = false;
            for (int s = 0; s < cached_group_count; s++) {
                int seg_fe = (int)group_buf[s * SEG_WORDS_PER_SEG].range(31, 0);
                if (seg_fe != UNUSED_ICON) {
                    found_nonempty = true;
                    if (seg_fe <= dst) seg_rel = s;
                }
            }
            if (!found_nonempty) seg_rel = 0;
            exact_seg[i] = tg + seg_rel;
        }

        // --- Build seg_list from exact_seg (dedup) ---
        int seg_list[16], seg_cnt = 0;
        for (int i = 0; i < b.count; i++) {
            int ts = exact_seg[i];
            bool found = false;
            for (int j = 0; j < seg_cnt; j++) {
                if (seg_list[j] == ts) { found = true; break; }
            }
            if (!found && seg_cnt < 16) seg_list[seg_cnt++] = ts;
        }

        // --- Phase A: burst-read all modified segments ---
        ap_uint<512> seg_buf[16][SEG_WORDS_PER_SEG];
        #pragma HLS bind_storage variable=seg_buf type=RAM_2P impl=BRAM
        #pragma HLS ARRAY_PARTITION variable=seg_buf complete dim=2
        int seg_bases[16];
        #pragma HLS ARRAY_PARTITION variable=seg_bases complete

        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS PIPELINE II=SEG_WORDS_PER_SEG
            int seg = seg_list[si];
            int base = (boff + HEADER_BYTES + total_hlp_words * 64 + seg * SEG_BYTES) / 64;
            seg_bases[si] = base;
            for (int w = 0; w < SEG_WORDS_PER_SEG; w++) {
                #pragma HLS UNROLL
                seg_buf[si][w] = hbm[base + w];
            }
        }

        // --- Phase A.5: distribute edges to per-segment dst arrays ---
        int seg_dsts[16][16];
        #pragma HLS ARRAY_PARTITION variable=seg_dsts complete dim=2
        int seg_dst_cnt[16];
        #pragma HLS ARRAY_PARTITION variable=seg_dst_cnt complete
        for (int i = 0; i < 16; i++) seg_dst_cnt[i] = 0;

        for (int i = 0; i < b.count; i++) {
            int ts = exact_seg[i];
            for (int si = 0; si < seg_cnt; si++) {
                #pragma HLS UNROLL
                if (seg_list[si] == ts) {
                    seg_dsts[si][seg_dst_cnt[si]++] = b.dst[i];
                }
            }
        }

        // --- Phase B: parallel seg_insert ---
        int seg_valid_buf[16];
        #pragma HLS ARRAY_PARTITION variable=seg_valid_buf complete
        bool seg_overflow[16];
        for (int i = 0; i < 16; i++) seg_overflow[i] = false;

        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS UNROLL factor=4
            seg_insert(seg_buf[si], seg_dsts[si], seg_dst_cnt[si],
                       seg_buf[si], seg_valid_buf[si], seg_overflow[si]);
        }

        // --- Phase C: write back segments + update VC helper ---
        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS PIPELINE II=SEG_WORDS_PER_SEG
            int base = seg_bases[si];
            int seg  = seg_list[si];
            for (int w = 0; w < SEG_WORDS_PER_SEG; w++) {
                #pragma HLS UNROLL
                hbm[base + w] = seg_buf[si][w];
            }
            // Update SoA valid_count on HBM (RMW per segment)
            int vc_word_idx = seg / VC_SOA_ENTRIES_PER_WORD;
            int vc_slot     = seg % VC_SOA_ENTRIES_PER_WORD;
            ap_uint<512> vc_word = hbm[vc_hbm_base + vc_word_idx];
            vc_word.range(vc_slot * 32 + 31, vc_slot * 32) = (ap_uint<32>)seg_valid_buf[si];
            hbm[vc_hbm_base + vc_word_idx] = vc_word;
        }

        // Update edge_count + write back header
        ap_uint<512> new_edge_count = b.edge_count_word;
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            ec[v] += ec_delta[v];
            new_edge_count.range(v * 32 + 31, v * 32) = (ap_uint<32>)ec[v];
        }
        int wb_base = boff / 64;
        hbm[wb_base + 0] = b.range_word;
        hbm[wb_base + 1] = new_edge_count;
        hbm[wb_base + 2] = 0;
        hbm[wb_base + 3] = 0;

        // Push SEGMENT_REBALANCE tasks for overflowed segments
        bool has_rebalance = false;
        for (int si = 0; si < seg_cnt; si++) {
            if (!seg_overflow[si]) continue;
            rebalance_task_t t;
            t.pma        = pma;
            t.payload    = seg_list[si];
            t.start_byte = b.start_byte;
            t.range_word = b.range_word;
            task_fifo.write(t);
            has_rebalance = true;
        }

        if (has_rebalance) {
            rebalance_task_t ack_task;
            ack_task.pma        = pma;
            ack_task.payload    = -2;
            ack_task.start_byte = 0;
            ack_task.range_word = 0;
            task_fifo.write(ack_task);
            pending_cnt++;
        } else {
            ack_out.write((ap_uint<32>)pma);
        }
    }

    rebalance_task_t stop_task;
    stop_task.pma        = 0;
    stop_task.payload    = -1;
    stop_task.start_byte = 0;
    stop_task.range_word = 0;
    task_fifo.write(stop_task);

    while (pending_cnt > 0) {
        if (!redist_ack_in.empty()) {
            ap_uint<32> ack_val = redist_ack_in.read();
            int ack_pma = (int)ack_val;
            if (ack_pma != -1) {
                pending_cnt--;
                ack_out.write((ap_uint<32>)ack_pma);
            }
        }
    }
    ack_out.write((ap_uint<32>)-1);
}


// Power-of-2 range finder for redistribution.
// Given an overflow segment `overflow_seg` in vertex [vertex_start, vertex_end),
// expand the range by doubling (power-of-2 alignment) until total edges fit
// the density bound.  Uses pre-loaded on-chip vc_cache — no HBM access.
//
// Output: left, right = segment range [left, right), total = sum of valid_counts.
void find_redistribution_range(
    int vc_cache[VC_CACHE_ENTRIES],
    int vertex_start, int vertex_end,
    int overflow_seg,
    int &left, int &right, int &total)
{
    int vertex_segments = vertex_end - vertex_start;
    int relative_position = overflow_seg - vertex_start;

    left  = overflow_seg;
    right = overflow_seg + 1;
    total = vc_cache[overflow_seg];

    for (int group_size = 2; group_size <= vertex_segments * 2; group_size *= 2) {
        if (total <= density_capacity(right - left)) break;

        int new_left  = vertex_start + (relative_position / group_size) * group_size;
        int new_right = vertex_start + ((relative_position / group_size) + 1) * group_size;
        if (new_right > vertex_end) new_right = vertex_end;
        if (new_left == left && new_right == right) continue;

        // Add valid_counts from newly included segments — on-chip cache, no HBM access
        for (int seg = new_left; seg < left; seg++) {
            #pragma HLS PIPELINE II=1
            total += vc_cache[seg];
        }
        for (int seg = right; seg < new_right; seg++) {
            #pragma HLS PIPELINE II=1
            total += vc_cache[seg];
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
        int segs_in_chunk = chunk_words / SEG_WORDS_PER_SEG;
        int max_edge_words = (segs_in_chunk * EDGE_SLOTS_PER_SEG + 15) / 16;

        // Initialize edge_buf to all UNUSED_ICON (all 1s = -1 in every slot)
        for (int i = 0; i < max_edge_words; i++) {
            #pragma HLS PIPELINE II=1
            edge_buf[i] = ~ap_uint<512>(0);
        }

        int edge_cnt = 0;
        for (int s = 0; s < segs_in_chunk; s++) {
            int seg_base = s * SEG_WORDS_PER_SEG;
            // valid_count lives in word 3, bits [511:480]
            int vc = (int)onchip_buffer[seg_base + 3].range(511, 480);
            if (vc > EDGE_SLOTS_PER_SEG) vc = EDGE_SLOTS_PER_SEG;

            for (int k = 0; k < vc; k++) {
                // No PIPELINE — RMW on same edge_buf word across consecutive
                // iterations is resolved by HLS with safe II; on-chip access
                // is not the bottleneck (HBM burst dominates).
                int wi = seg_base + k / 16;
                int si = k % 16;
                int edge = (int)onchip_buffer[wi].range(si * 32 + 31, si * 32);
                int word_idx = edge_cnt / 16;
                int slot     = edge_cnt % 16;
                edge_buf[word_idx].range(slot * 32 + 31, slot * 32) = (ap_uint<32>)edge;
                edge_cnt++;
            }
        }

        // ==== Phase 3: burst-write edge_buf → hbm_buffer ====
        int edge_words = (edge_cnt + 15) / 16;
        int hbm_wo = total_edges / 16;  // 512-bit word offset in hbm_buffer
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
void redistribute_edge_to_hbm(
    ap_uint<512>* hbm, ap_uint<512>* hbm_buffer,
    int start_word, int end_word, int total_edges,
    int group_left,              // absolute segment index of group start
    int vc_hbm_base,             // valid_count area HBM base (word offset)
    int fe_hbm_base,             // first_edge area HBM base (word offset)
    int vc_cache[VC_CACHE_ENTRIES],
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
        int edge_words = (chunk_edges + 15) / 16;
        int hbm_edge_wo = edge_start / 16;
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
                // Read edge from edge_buf: src position in flat edge array = edge_start + pos,
                // subtract hbm_edge_wo to get word index within the chunk's view of edge_buf
                int src_word = (edge_start + pos) / 16 - hbm_edge_wo;
                int src_slot = (edge_start + pos) % 16;
                int edge = (int)edge_buf[src_word].range(src_slot * 32 + 31, src_slot * 32);
                // Write to onchip_buffer at correct segment position
                int dst_wi = s * SEG_WORDS_PER_SEG + k / 16;
                int dst_si = k % 16;
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

        // ==== Phase 5: update VC (on-chip cache + HBM) and FE (HBM) ====
        int seg_chunk_start = group_left + chunk_start;

        // 5a: update vc_cache and VC HBM (RMW per word)
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
                // Update on-chip cache
                vc_cache[global_seg] = fill;
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

        // 5b: update FE HBM (RMW, min per 16-segment group)
        {
            int fe_group_start = seg_chunk_start / HELPER_FE_GRANULARITY;
            int fe_group_end   = (seg_chunk_start + chunk_segs + HELPER_FE_GRANULARITY - 1)
                                 / HELPER_FE_GRANULARITY;
            int fe_word_start  = fe_group_start / FE_SOA_ENTRIES_PER_WORD;
            int fe_word_end    = (fe_group_end + FE_SOA_ENTRIES_PER_WORD - 1)
                                 / FE_SOA_ENTRIES_PER_WORD;
            int fe_word_count  = fe_word_end - fe_word_start;
            // Reuse edge_buf as temp fe buffer
            for (int w = 0; w < fe_word_count; w++) {
                #pragma HLS PIPELINE II=1
                edge_buf[w] = hbm[fe_hbm_base + fe_word_start + w];
            }
            for (int s = 0; s < chunk_segs; s++) {
                #pragma HLS PIPELINE II=1
                int global_seg = seg_chunk_start + s;
                int group_idx  = global_seg / HELPER_FE_GRANULARITY;
                // Only update for the first segment in each group (minimum first_edge)
                if (global_seg % HELPER_FE_GRANULARITY == 0) {
                    int local_word = group_idx / FE_SOA_ENTRIES_PER_WORD - fe_word_start;
                    int local_slot = group_idx % FE_SOA_ENTRIES_PER_WORD;
                    ap_uint<32> seg_fe = onchip_buffer[s * SEG_WORDS_PER_SEG].range(31, 0);
                    edge_buf[local_word].range(local_slot * 32 + 31, local_slot * 32) = seg_fe;
                }
            }
            for (int w = 0; w < fe_word_count; w++) {
                #pragma HLS PIPELINE II=1
                hbm[fe_hbm_base + fe_word_start + w] = edge_buf[w];
            }
        }
    }
}
void redistribution_phase(
    ap_uint<512>* hbm,
    ap_uint<512>* hbm_edge_buffer,
    int data_off,
    hls::stream<rebalance_task_t>& task_fifo,
    hls::stream<ap_uint<32>>& redist_ack_out)
{
    // On-chip buffers — declared outside while for HLS lifetime analysis
    ap_uint<512> onchip_buffer[MAX_ONCHIP_SEG_WORDS];
    #pragma HLS bind_storage variable=onchip_buffer type=RAM_2P impl=URAM
    ap_uint<512> edge_buffer[MAX_ONCHIP_SEG_WORDS];
    #pragma HLS bind_storage variable=edge_buffer type=RAM_2P impl=URAM

    // Full-PMA valid_count cache: burst-read once per PMA, reused across rebalance tasks
    // Updated inline by redistribute_edge_to_hbm, so always in sync with HBM
    int vc_cache[VC_CACHE_ENTRIES];
    #pragma HLS bind_storage variable=vc_cache type=RAM_2P impl=URAM
    int cached_pma_boff = -1;  // which PMA's vc is currently cached

    while (true) {
        rebalance_task_t task = task_fifo.read();
        int payload = task.payload;
        int pm  = task.pma;
        if (payload == -1) {  // STOP
            ap_uint<32> stop_ack = (ap_uint<32>)(-1);
            redist_ack_out.write(stop_ack);
            break;
        }
        if (payload == -2) {  // ACK marker: notify insert_phase
            redist_ack_out.write((ap_uint<32>)pm);
            continue;
        }

        // Use forwarded header from insert_phase (no VEC_INFO/header HBM reads)
        int pma_boff = task.start_byte - data_off;
        int hbase = pma_boff / 64;
        int total_segs = (int)task.range_word.range(15 * 32 + 31, 15 * 32);
        if (total_segs == 0) continue;
        int fe_words = fe_words_for(total_segs);
        int fe_hbm_base = hbase + HEADER_WORDS;
        int vc_hbm_base = hbase + HEADER_WORDS + fe_words;
        int total_hlp    = fe_words + vc_words_for(total_segs);

        // Burst-read vc into cache only on PMA switch (multiple rebalance tasks
        // may target the same PMA; vc_cache is kept in sync by redistribute)
        if (pma_boff != cached_pma_boff) {
            int total_vc_words = vc_words_for(total_segs);
            for (int w = 0; w < total_vc_words; w++) {
                #pragma HLS PIPELINE II=1
                ap_uint<512> vc_word = hbm[vc_hbm_base + w];
                for (int s = 0; s < VC_SOA_ENTRIES_PER_WORD; s++) {
                    #pragma HLS UNROLL
                    int entry_idx = w * VC_SOA_ENTRIES_PER_WORD + s;
                    if (entry_idx < total_segs)
                        vc_cache[entry_idx] = (int)vc_word.range(s * 32 + 31, s * 32);
                }
            }
            cached_pma_boff = pma_boff;
        }

        // Vertex ranges from forwarded header
        int vertex_range[VERTEX_PER_PMA];
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            vertex_range[v] = (int)task.range_word.range(v * 32 + 31, v * 32);
        }

        // SEGMENT_REBALANCE: find owner vertex, then power-of-2 group via find_redistribution_range
        int overflow_seg = payload;
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
        if (vertex_end - vertex_start <= 1) continue;

        int left, right, total;
        find_redistribution_range(vc_cache,
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
        redistribute_edge_to_hbm(hbm, hbm_edge_buffer,
            segment_base, segment_base + segment_words, collected_edges,
            left, vc_hbm_base, fe_hbm_base, vc_cache,
            onchip_buffer, edge_buffer);
    }
}

// ---- Task Reader: replaces generator + router ----
// Reads edges from per-worker HBM task buffer, groups by PMA into batches.
// Two-phase design: burst-read chunks into local BRAM, then process from BRAM
// to avoid data-dependent break that prevents HLS burst inference.

#define STANDALONE_POOL_SIZE 16
#define POOL_SLOT_CAPACITY 256  // max edges per slot (never flush mid-way)
#define TASK_BURST_CHUNK 256    // edges per burst chunk

// Lock table: small associative in-flight tracker (like router's in_flight)
// Only stores PMAs that are currently locked — works for any PMA index.
#define LOCK_TABLE_SIZE 16
struct LockTable {
    int pmas[LOCK_TABLE_SIZE];   // -1 = free
    int count;
};
static void lock_init(LockTable &lt) {
    for (int i = 0; i < LOCK_TABLE_SIZE; i++) {
        lt.pmas[i] = -1;
    }
    lt.count = 0;
}
static bool lock_is_locked(LockTable &lt, int pma) {
    for (int i = 0; i < LOCK_TABLE_SIZE; i++) {
        if (lt.pmas[i] == pma) return true;
    }
    return false;
}
static void lock_acquire(LockTable &lt, int pma) {
    for (int i = 0; i < LOCK_TABLE_SIZE; i++) {
        if (lt.pmas[i] == -1) {
            lt.pmas[i] = pma;
            lt.count++;
            return;
        }
    }
}
static void lock_release(LockTable &lt, int pma) {
    for (int i = 0; i < LOCK_TABLE_SIZE; i++) {
        if (lt.pmas[i] == pma) {
            lt.pmas[i] = -1;
            lt.count--;
        }
    }
}


static void task_reader(
    int* task_buffer,
    int edge_count,
    LockTable &lock_tbl,
    hls::stream<ap_uint<32>>& ack_in,
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_out,
    hls::stream<PMA_UPDATE_MESSAGE>& online_in
) {
    // ---- Pool: ring buffers, same pattern as router ----
    struct Slot {
        int pma;   // -1 = free
        int head;  // enqueue position
        int tail;  // dequeue position
        int count; // edges in queue
        PMA_UPDATE_MESSAGE edges[POOL_SLOT_CAPACITY];
    };
    Slot pool[STANDALONE_POOL_SIZE];
    #pragma HLS bind_storage variable=pool type=RAM_2P impl=BRAM

    // Tag arrays — small, let HLS decide placement (no partition)
    int pool_pma[STANDALONE_POOL_SIZE];
    int pool_cnt[STANDALONE_POOL_SIZE];

    for (int i = 0; i < STANDALONE_POOL_SIZE; i++) {
        pool[i].pma = -1; pool[i].head = 0; pool[i].tail = 0; pool[i].count = 0;
        pool_pma[i] = -1; pool_cnt[i] = 0;
    }

    // Slot ops — no INLINE (let HLS decide); sequential scan, no UNROLL needed
    auto slot_find = [&](int pma) -> int {
        for (int i = 0; i < STANDALONE_POOL_SIZE; i++) {
            if (pool_pma[i] == pma && pool_cnt[i] < POOL_SLOT_CAPACITY) return i;
        }
        for (int i = 0; i < STANDALONE_POOL_SIZE; i++) {
            if (pool_pma[i] == pma) return i;
        }
        return -1;
    };
    auto slot_alloc = [&](int pma) -> int {
        int free_slot = -1;
        for (int i = 0; i < STANDALONE_POOL_SIZE; i++) {
            if (pool_pma[i] == -1) free_slot = i;
        }
        if (free_slot != -1) {
            pool_pma[free_slot] = pma; pool_cnt[free_slot] = 0;
            pool[free_slot].pma = pma; pool[free_slot].head = 0;
            pool[free_slot].tail = 0; pool[free_slot].count = 0;
        }
        return free_slot;
    };
    auto slot_enq = [&](int s, PMA_UPDATE_MESSAGE m) {
        pool[s].edges[pool[s].head] = m;
        pool[s].head = (pool[s].head + 1) % POOL_SLOT_CAPACITY;
        pool[s].count++;
        pool_cnt[s] = pool[s].count;
    };
    auto slot_flush_one_batch = [&](int s, hls::stream<PMA_UPDATE_MESSAGE>& out) {
        int n = pool[s].count;
        if (n == 0) { pool_pma[s] = -1; return; }
        int batch = (n < MAX_CMD_NUM) ? n : MAX_CMD_NUM;
        for (int i = 0; i < batch; i++) {
            #pragma HLS PIPELINE II=1
            PMA_UPDATE_MESSAGE m = pool[s].edges[pool[s].tail];
            pool[s].tail = (pool[s].tail + 1) % POOL_SLOT_CAPACITY;
            m.last = (i == batch - 1) ? 1 : 0;
            out.write(m);
        }
        pool[s].count -= batch;
        pool_cnt[s] = pool[s].count;
        if (pool[s].count == 0) {
            pool_pma[s] = -1; pool[s].pma = -1;
        }
    };
    auto slot_evict = [&](int for_pma) -> int {
        int best = -1;
        for (int i = 0; i < STANDALONE_POOL_SIZE; i++) {
            int ev_pma = pool_pma[i];
            if (ev_pma == -1 || ev_pma == for_pma) continue;
            if (lock_is_locked(lock_tbl, ev_pma)) continue;
            if (best == -1 || pool_cnt[i] > 0) best = i;
        }
        return best;
    };

    // Burst-read buffer
    int burst_buf[TASK_BURST_CHUNK * 3];
    #pragma HLS bind_storage variable=burst_buf type=RAM_2P impl=BRAM

    int processed = 0;
    int base = 0;
    int live_edges = edge_count;  // grows with online updates
    bool online_done = false;
    bool sentinel_hit = false;
    
    lock_init(lock_tbl);
    while ((base < live_edges || !online_done) && !sentinel_hit) {
        // ---- Poll online updates, append to task HBM ----
        while   (!online_in.empty() && !online_done) {
            PMA_UPDATE_MESSAGE om = online_in.read();
            if (om.opcode == PMA_UPDATE_STOP) {
                online_done = true;  // any STOP terminates online stream
            } else {
                int wp = live_edges * 3;
                task_buffer[wp]     = om.src;
                task_buffer[wp + 1] = om.dst;
                task_buffer[wp + 2] = (int)om.opcode;
                live_edges++;
            }
        }

        if (base >= live_edges) continue;  // wait for more data

        int chunk = live_edges - base;
        if (chunk > TASK_BURST_CHUNK) chunk = TASK_BURST_CHUNK;

        // Phase A: burst-read chunk
        int m_axi_off = base * 3;
        int words = chunk * 3;
        for (int j = 0; j < words; j++) {
            #pragma HLS PIPELINE II=1
            burst_buf[j] = task_buffer[m_axi_off + j];
        }

        // Phase B: process from BRAM with router-style enqueue+retry
        for (int j = 0; j < chunk && !sentinel_hit; j++) {

            // Phase-independent: poll ACKs
            while (!ack_in.empty()) {
                ap_uint<32> val = ack_in.read();
                int ack_pma = (int)val;
                if (ack_pma != -1) lock_release(lock_tbl, ack_pma);
            }

            int src = burst_buf[j * 3];
            int dst = burst_buf[j * 3 + 1];
            int op  = burst_buf[j * 3 + 2];
            if (src == -1) { sentinel_hit = true; break; }

            int pma = src / VERTEX_PER_PMA;
            int slot = slot_find(pma);
            if (slot == -1) slot = slot_alloc(pma);

            // If slot is full, flush one batch (wait for lock first)
            if (slot != -1 && pool_cnt[slot] >= MAX_CMD_NUM) {
                while (lock_is_locked(lock_tbl, pma)) {
                    while (!ack_in.empty()) {
                        ap_uint<32> val = ack_in.read();
                        int ap = (int)val;
                        if (ap != -1) lock_release(lock_tbl, ap);
                    }
                }
                slot_flush_one_batch(slot, cmd_out);
                lock_acquire(lock_tbl, pma);
                // After flush, slot might be freed (if empty). If still used, find/create.
                if (pool_pma[slot] != pma) slot = slot_alloc(pma);
            }

            // If no slot (pool full), evict with retry
            if (slot == -1) {
                int ev = slot_evict(pma);
                if (ev != -1) {
                    int ev_pma = pool_pma[ev];
                    while (lock_is_locked(lock_tbl, ev_pma)) {
                        while (!ack_in.empty()) {
                            ap_uint<32> val = ack_in.read();
                            int ap = (int)val;
                            if (ap != -1) lock_release(lock_tbl, ap);
                        }
                    }
                    // Drain victim completely
                    while (pool_cnt[ev] > 0) {
                        slot_flush_one_batch(ev, cmd_out);
                    }
                    lock_acquire(lock_tbl, ev_pma);
                    // Reuse slot
                    pool[ev].pma = pma; pool[ev].head = 0;
                    pool[ev].tail = 0; pool[ev].count = 0;
                    pool_pma[ev] = pma; pool_cnt[ev] = 0;
                    slot = ev;
                }
            }

            // Enqueue edge (with retry if slot still unavailable)
            if (slot == -1) continue; // shouldn't happen; edge lost as last resort

            PMA_UPDATE_MESSAGE msg;
            msg.src = src; msg.dst = dst;
            msg.opcode = (PMA_UPDATE_OPCODE)op; msg.last = 0;
            slot_enq(slot, msg);
            processed++;
        }
        base += chunk;
    }

    // ---- Drain: flush all remaining slots with lock sync ----
    bool any_left = true;
    while (any_left) {
        while (!ack_in.empty()) {
            ap_uint<32> val = ack_in.read();
            int ack_pma = (int)val;
            if (ack_pma != -1) lock_release(lock_tbl, ack_pma);
        }
        any_left = false;
        for (int j = 0; j < STANDALONE_POOL_SIZE; j++) {
            if (pool_cnt[j] == 0) continue;
            int pma = pool_pma[j];
            if (lock_is_locked(lock_tbl, pma)) { any_left = true; continue; }
            lock_acquire(lock_tbl, pma);
            slot_flush_one_batch(j, cmd_out);
            if (pool_cnt[j] > 0) any_left = true;
        }
    }

    // DEBUG: write processed count to HBM
    task_buffer[edge_count * 3 + 3] = processed;

    // Send STOP
    PMA_UPDATE_MESSAGE stop_msg;
    stop_msg.src = -1; stop_msg.dst = -1;
    stop_msg.opcode = PMA_UPDATE_STOP; stop_msg.last = 1;
    cmd_out.write(stop_msg);
}



// ---- Top-level ----

extern "C"
{
    void pma_insert_edge_standalone(
        ap_uint<512>* hbm_gate,
        ap_uint<512>* hbm_bs,     // BS worker read-only
        ap_uint<512>* hbm_rw,     // insert phase read-write
        ap_uint<512>* hbm_red,    // redistribution read-write (separate port)
        ap_uint<512>* hbm_buffer_for_red,  // edge buffer for collect/redistribute
        int* task_buffer,          // per-worker edge task buffer
        int pma_raw_data_start,
        int pma_vecinfo_start,
        int edge_count,            // number of edges in this batch
        int bitmap_offset,         // byte offset in hbm_gate for bitmap
        hls::stream<ap_axiu<96,0,0,0>>& alloc_cmd_stream,
        hls::stream<ap_axiu<64,0,0,0>>& alloc_rsp_stream,
        hls::stream<ap_axiu<ALLOC_BITMAP_STORAGE_WORD_BITS,0,0,0>>& bitmap_init,  // bitmap → alloc_access
        hls::stream<PMA_UPDATE_MESSAGE>& online_in    // online updates from router
    )
    {
        #pragma HLS INTERFACE m_axi port=hbm_gate offset=slave bundle=gate max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=hbm_bs   offset=slave bundle=bs max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=hbm_rw   offset=slave bundle=insert max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=hbm_red  offset=slave bundle=rebalance max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=task_buffer offset=slave bundle=taskbuffer max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=hbm_buffer_for_red offset=slave bundle=edgebuf max_read_burst_length=256 max_write_burst_length=256

        #pragma HLS INTERFACE s_axilite port=hbm_gate bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_bs   bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_rw   bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_red  bundle=control
        #pragma HLS INTERFACE s_axilite port=task_buffer bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_buffer_for_red bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_raw_data_start bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_vecinfo_start bundle=control
        #pragma HLS INTERFACE s_axilite port=edge_count bundle=control
        #pragma HLS INTERFACE s_axilite port=bitmap_offset bundle=control
        #pragma HLS INTERFACE s_axilite port=return bundle=control
        #pragma HLS INTERFACE axis port=alloc_cmd_stream
        #pragma HLS INTERFACE axis port=alloc_rsp_stream
        #pragma HLS INTERFACE axis port=bitmap_init
        #pragma HLS INTERFACE axis port=online_in

        hls::stream<PMA_UPDATE_MESSAGE> cmd_from_reader;
        #pragma HLS STREAM variable=cmd_from_reader depth=16

        hls::stream<PMA_UPDATE_MESSAGE> cmd_batch_stream;
        #pragma HLS STREAM variable=cmd_batch_stream depth=16

        hls::stream<bs_batch_t> bs_out;
        #pragma HLS STREAM variable=bs_out depth=8

        hls::stream<rebalance_task_t> task_fifo;
        #pragma HLS STREAM variable=task_fifo depth=64

        hls::stream<ap_uint<32>> redist_ack_stream;
        #pragma HLS STREAM variable=redist_ack_stream depth=16

        hls::stream<ap_uint<32>> dummy_ack_stream;
        #pragma HLS STREAM variable=dummy_ack_stream depth=16

        hls::stream<fwd_header_t> hdr_fwd;
        #pragma HLS STREAM variable=hdr_fwd depth=4

        // Per-PMA lock table (small associative set, works for any PMA index)
        LockTable lock_tbl;

        #pragma HLS DATAFLOW

        task_reader(task_buffer, edge_count, lock_tbl, dummy_ack_stream, cmd_from_reader, online_in);

        pma_gate(hbm_gate, pma_raw_data_start, pma_vecinfo_start,
                 bitmap_offset,
                 cmd_from_reader, cmd_batch_stream,
                 alloc_cmd_stream, alloc_rsp_stream, bitmap_init, hdr_fwd);

        bs_worker(hbm_bs, pma_raw_data_start,
                  hdr_fwd,
                  cmd_batch_stream, bs_out);

        insert_phase(hbm_rw, pma_raw_data_start,
                     bs_out,
                     task_fifo, redist_ack_stream, dummy_ack_stream);

        redistribution_phase(hbm_red, hbm_buffer_for_red, pma_raw_data_start,
                            task_fifo, redist_ack_stream);

    }
}
