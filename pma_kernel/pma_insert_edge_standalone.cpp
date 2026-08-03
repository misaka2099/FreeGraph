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

// ---- Helper SoA layout (new: FE 1:1 per segment, VC 1:1 per segment) ----
//   first_edge area: 1 entry per segment (= first_edge, 32-bit), 16 entries/word
//   valid_count area: 1 entry per segment (= valid_count, 32-bit), 16 entries/word
//   hlp_total_words(segs) = fe_words_for(segs) + vc_words_for(segs)
// Word-count formulas live in data_type.h (fe_words_for / vc_words_for / hlp_total_words).
// Empty helper words: first_edge = all UNUSED_ICON, valid_count = all 0
static const ap_uint<512> EMPTY_FE_WORD = ~ap_uint<512>(0);  // all UNUSED_ICON
static const ap_uint<512> EMPTY_VC_WORD = 0;

// Max segments per on-chip chunk — sizes onchip_buffer / edge_buffer.
// 1024 segments × 4 words = 4096 × 512-bit words = 256 KiB on-chip.
static const int MAX_ONCHIP_SEG = 1024;
static const int MAX_ONCHIP_SEG_WORDS = MAX_ONCHIP_SEG * SEG_WORDS_PER_SEG;

// Forwarded header info: pma_gate → bs_worker → insert_phase
// Eliminates redundant VEC_INFO + header HBM reads in downstream stages
struct fwd_header_t {
    int start_byte;              // absolute HBM byte offset of PMA data
    int pma_idx;                 // PMA index
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

// Pre-expand command: task_reader → redistribution_phase
// Sent BEFORE a PMA's edge segment is flushed. Per-vertex insert counts (from
// the sorted-wave scan) let redistribution grow each vertex of the PMA
// up-front so inserts fit without runtime ENLARGE. INIT (vi==0) is handled as
// a special case: allocate fresh PMA, no collect, no FREE.
struct pre_expand_cmd_t {
    int pma;                     // PMA index (global)
    int count[VERTEX_PER_PMA];   // edges per local vertex for this PMA this wave
};

// ALLOC_BITMAP_WORDS defined in data_type.h (shared with alloc_access_kernel)

// File-scope helper: ALL_ONES and EMPTY_SEG with valid_count=0
// Empty segment ROM: 4 words × 512 bits, replaces make_empty_seg()
// Word 0-2: all ones (UNUSED_ICON for all 16 edge slots)
// Word 3:   all ones except bits[511:480]=0 (valid_count=0)
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

// Read a valid_count entry (1:1 per segment, 16 entries per 512-bit word)
// directly from HBM (used by find_redistribution_range and redistribution_phase).
static inline int hbm_vc_read(ap_uint<512>* hbm, int vc_hbm_base, int seg)
{
    int vc_word_off = vc_hbm_base + seg / VC_SOA_ENTRIES_PER_WORD;
    int vc_slot     = seg % VC_SOA_ENTRIES_PER_WORD;
    ap_uint<512> vc_word = hbm[vc_word_off];
    return (int)vc_word.range(vc_slot * 32 + 31, vc_slot * 32);
}

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

static inline int density_capacity(int seg_cnt) { return seg_cnt * 47; }

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

// ---- Alloc command merger ----
// pma_gate (INIT) and redistribution_phase (ENLARGE) both need the single
// external alloc_access port. This process round-robins the two internal
// request/response stream pairs and forwards to the external ap_axiu streams.
//
// The single STOP is deferred until BOTH consumers have finished (each writes
// an opcode=0 marker), because alloc_access terminates on the first STOP while
// redistribution may still need ENLARGE allocations afterwards. FREE (opcode=2)
// is fire-and-forget (no response); only ALLOC (opcode=1) gets a response.

static void alloc_merger(
    hls::stream<ap_uint<96>>& gate_alloc_cmd,
    hls::stream<ap_uint<64>>& gate_alloc_rsp,
    hls::stream<ap_uint<96>>& redist_alloc_cmd,
    hls::stream<ap_uint<64>>& redist_alloc_rsp,
    hls::stream<ap_axiu<96,0,0,0>>& alloc_cmd_stream,
    hls::stream<ap_axiu<64,0,0,0>>& alloc_rsp_stream)
{
    bool gate_done = false;
    bool redist_done = false;

    while (true) {
        bool served = false;

        // pma_gate side
        if (!gate_alloc_cmd.empty()) {
            ap_uint<96> cmd = gate_alloc_cmd.read();
            int opcode = (int)cmd.range(95, 64);
            if (opcode == 0) {
                gate_done = true;               // STOP marker from pma_gate
            } else {
                ap_axiu<96,0,0,0> acmd;
                acmd.data = cmd;
                acmd.last = 0;
                acmd.keep = 0xFFF;
                alloc_cmd_stream.write(acmd);
                ap_wait();
                if (opcode == 1) {              // ALLOC → wait for response
                    ap_axiu<64,0,0,0> arsp = alloc_rsp_stream.read();
                    gate_alloc_rsp.write(arsp.data.range(63, 0));
                }
            }
            served = true;
        }

        // redistribution side
        if (!redist_alloc_cmd.empty()) {
            ap_uint<96> cmd = redist_alloc_cmd.read();
            int opcode = (int)cmd.range(95, 64);
            if (opcode == 0) {
                redist_done = true;             // STOP marker from redistribution
            } else {
                ap_axiu<96,0,0,0> acmd;
                acmd.data = cmd;
                acmd.last = 0;
                acmd.keep = 0xFFF;
                alloc_cmd_stream.write(acmd);
                ap_wait();
                if (opcode == 1) {              // ALLOC → wait for response
                    ap_axiu<64,0,0,0> arsp = alloc_rsp_stream.read();
                    redist_alloc_rsp.write(arsp.data.range(63, 0));
                }
            }
            served = true;
        }

        // Both consumers done → forward the single STOP and terminate
        if (gate_done && redist_done) {
            ap_axiu<96,0,0,0> scmd;
            scmd.data.range(95, 64) = 0;        // opcode=STOP
            scmd.last = 1;
            scmd.keep = 0xFFF;
            alloc_cmd_stream.write(scmd);
            return;
        }

        if (!served) ap_wait();
    }
}

// ---- pma_gate helper: handle existing (non-empty) PMA ----
// Extracted to a separate function to prevent HLS if-conversion
// from speculating alloc/free/m_axi writes when vi==0.
// ENLARGE no longer happens here — overflow is handled by redistribution_phase.
// This function only forwards the existing header to bs_worker.
static void pma_gate_handle_existing(
    ap_uint<512>* hbm_gate,
    int pma_raw_data_start,
    int pma_idx,
    VEC_INFO vi,
    PMA_UPDATE_MESSAGE* cmd_batch,
    int cmd_count,
    ap_uint<512>* pma_buffer,
    hls::stream<ap_uint<96>>& gate_alloc_cmd,
    hls::stream<ap_uint<64>>& gate_alloc_rsp,
    hls::stream<fwd_header_t>& hdr_fwd)
{
    ap_uint<512> range_word = hbm_gate[(int)(vi.range(31,0) - pma_raw_data_start) / 64];
    ap_uint<512> edge_count_word = hbm_gate[(int)(vi.range(31,0) - pma_raw_data_start) / 64 + 1];
    fwd_header_t fh;
    fh.start_byte = (int)vi.range(31, 0);
    fh.pma_idx    = pma_idx;
    fh.range_word = range_word;
    fh.edge_count_word = edge_count_word;
    hdr_fwd.write(fh);
}

void pma_gate(
    ap_uint<512> *hbm_gate,
    int pma_raw_data_start,
    int pma_vecinfo_start,
    int bitmap_offset,
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_in,
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_out,
    hls::stream<ap_uint<96>>& gate_alloc_cmd,
    hls::stream<ap_uint<64>>& gate_alloc_rsp,
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
        #pragma HLS PIPELINE II=1
        ap_axiu<ALLOC_BITMAP_STORAGE_WORD_BITS,0,0,0> w;
        w.data = hbm_gate[bm_word_off + i];
        w.last = 0;
        w.keep = -1;
        bitmap_init.write(w);
    }

    while (true)
    {
        if (cmd_in.empty()) { ap_wait(); continue; }
        PMA_UPDATE_MESSAGE first = cmd_in.read();
        if (first.opcode == PMA_UPDATE_STOP && first.last == 1) {
            cmd_out.write(first);
            // Send STOP marker to alloc_merger (forwarded once both consumers done)
            ap_uint<96> scmd = 0;               // opcode=STOP
            gate_alloc_cmd.write(scmd);
            return;
        }
        PMA_UPDATE_MESSAGE cmd_batch[MAX_CMD_NUM];
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
        VEC_INFO vi = vi_read(hbm_gate, pma_vecinfo_start, pma_idx);
        // hls::print("GATE vi=%d\n", (unsigned)(int)vi.range(31,0));
        if (vi == 0)
        {
            // Initialize empty PMA in pma_buffer: header + SoA helper + 16 segments
            int isegs = VERTEX_PER_PMA;  // 16 vertices, 1 segment each
            int ife_words = fe_words_for(isegs);   // first_edge area (1 entry per segment)
            int ivc_words = vc_words_for(isegs);   // valid_count area (1 entry per segment)
            int ihlp      = ife_words + ivc_words; // = hlp_total_words(isegs)

            // Alloc new PMA space
            int ibytes = (HEADER_WORDS + ihlp + isegs * SEG_WORDS_PER_SEG) * 64;
            ap_uint<96> acmd = 0;
            acmd.range(31, 0)  = 0;        // start=0 for ALLOC
            acmd.range(63, 32) = (ap_uint<32>)ibytes;   // request bytes
            acmd.range(95, 64) = 1;        // opcode=ALLOC
            gate_alloc_cmd.write(acmd);

            // ---- Build PMA in pma_buffer from ROM WHILE alloc is in flight ----
            // (nstart not needed until write-back)

            // Header: from ROM
            pma_buffer[0] = EMPTY_PMA_HDR0;     // vertex_range: 1 seg per vertex, total=16
            pma_buffer[1] = 0;                   // edge_count all zero
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
            ap_uint<64> arsp = gate_alloc_rsp.read();
            int nstart = (int)arsp.range(31, 0);

            int write_start_in_512_pattern = nstart / 64;
            int ibody_words = HEADER_WORDS + ihlp + isegs * SEG_WORDS_PER_SEG;
            for (int i = 0; i < ibody_words; i++)
            {
                #pragma HLS PIPELINE II=1
                hbm_gate[write_start_in_512_pattern + i] = pma_buffer[i];
            }

            // Update VEC_INFO so subsequent accesses see the PMA
            VEC_INFO nvi = 0;
            nvi.range(31, 0)  = (ap_uint<32>)(nstart + pma_raw_data_start);
            nvi.range(63, 32) = (ap_uint<32>)ibytes;
            vi_write(hbm_gate, pma_vecinfo_start, pma_idx, nvi);

            // Forward NEW header to bs_worker (post-init)
            {
                fwd_header_t fh;
                fh.start_byte = nstart + pma_raw_data_start;
                fh.pma_idx    = pma_idx;
                fh.range_word = pma_buffer[0];  // hdr0: vertex ranges
                fh.edge_count_word = 0;          // hdr1: all edge counts zero
                hdr_fwd.write(fh);
            }
        }
        //check pma enlargement
        else
        {
            // hls::print("GATE else enter\n");
            pma_gate_handle_existing(hbm_gate, pma_raw_data_start,
                                     pma_idx, vi, cmd_batch, cmd_count, pma_buffer,
                                     gate_alloc_cmd, gate_alloc_rsp, hdr_fwd);
        }
        for (int i = 0 ; i < cmd_count; i++)
        {
            cmd_out.write(cmd_batch[i]);
        }
    }
}

// ---- Phase 1: BS Worker ----
// Reads ONLY the first_edge area from HBM (1 entry per segment) and binary
// searches directly for the exact target segment of each edge.

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

        // Extract vertex_range from forwarded header
        for (int v = 0; v < VERTEX_PER_PMA; v++)
            #pragma HLS UNROLL
            vr[v] = (int)fh.range_word.range(v * 32 + 31, v * 32);

        // FE area base (word offset): 1 entry per segment, 16 entries per 512-bit word
        int fe_hbm_base = hbase + HEADER_WORDS;

        // Process edges — direct HBM binary search on the per-segment first_edge
        // entries: result IS the exact segment (no fine search needed)
        bs_batch_t res;
        res.pma_idx = pma; res.op_mask = 0; res.count = cnt;
        res.start_byte = fh.start_byte;
        res.range_word = fh.range_word;
        res.edge_count_word = fh.edge_count_word;

        for (int i = 0; i < MAX_CMD_NUM; i++) {
            #pragma HLS PIPELINE II=1
            if (i < cnt) {
                PMA_UPDATE_MESSAGE e = edges[i];
                res.dst[i] = e.dst;
                res.local_src[i] = e.src & 0xF;
                int op = (int)e.opcode;
                if (op == PMA_UPDATE_INSERT_EDGE /* || op == PMA_UPDATE_DELETE_EDGE */) {
                    int ls = e.src & 0xF;
                    int vs = (ls == 0) ? 0 : vr[ls - 1];
                    int ve = vr[ls];
                    // Direct HBM binary search for FE entry (1 entry per segment,
                    // 16 entries per 512-bit word). Result is the relative segment
                    // offset within this vertex → absolute = vs + seg_rel.
                    int lo = 0, hi = ve - vs - 1;
                    int seg_rel = -1;
                    while (lo <= hi) {
                        #pragma HLS PIPELINE off
                        int mid = (lo + hi) / 2;
                        int fe_idx = vs + mid;
                        int fe_word_off = fe_hbm_base + fe_idx / FE_SOA_ENTRIES_PER_WORD;
                        int fe_slot = fe_idx % FE_SOA_ENTRIES_PER_WORD;
                        ap_uint<512> fe_word = hbm[fe_word_off];
                        int fe_val = (int)fe_word.range(fe_slot * 32 + 31, fe_slot * 32);
                        if (fe_val == e.dst) { seg_rel = mid; break; }
                        else if (fe_val < e.dst) { lo = mid + 1; seg_rel = mid; }
                        else { hi = mid - 1; }
                    }
                    if (seg_rel == -1) seg_rel = 0;
                    res.target_seg[i] = vs + seg_rel;
                } else {
                    res.target_seg[i] = 0;
                }
            }
        }
        bs_out.write(res);
    }
}

// ---- Phase 2+3: Insert (segment modification) ----
// bs_worker already resolved the EXACT target segment (FE 1:1 search), so no
// fine search is needed here. Modified segments are read, patched via seg_insert,
// written back, and their valid_count is RMW-updated in the SoA VC area.
// Overflowed segments are pushed to task_fifo for deferred redistribution.
// ACKs are sent by redistribution_phase after all tasks are processed.

void insert_phase(
    ap_uint<512>* hbm,
    int data_off, int vi_wo,
    hls::stream<bs_batch_t>& bs_in,
    hls::stream<rebalance_task_t>& task_fifo,
    hls::stream<ap_uint<32>>& redist_ack_in,
    hls::stream<ap_uint<32>>& ack_out)
{
    int pending_cnt = 0;

    while (true) {
        // #pragma HLS PIPELINE II=1

        // --- read_nb: poll redistribution ACK every cycle ---
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

        // Use forwarded header from pma_gate → bs_worker
        // (no VEC_INFO read, no header HBM read — only segments + VC are touched)
        int boff = b.start_byte - data_off;
        int hbase = boff / 64;
        int tseg = (int)b.range_word.range(15 * 32 + 31, 15 * 32);
        int fe_words = fe_words_for(tseg);
        int vc_words = vc_words_for(tseg);
        int total_hlp_words = fe_words + vc_words;
        int vc_hbm_base = hbase + HEADER_WORDS + fe_words;

        // Vertex ranges and edge counts from forwarded header
        int ec[VERTEX_PER_PMA], vr[VERTEX_PER_PMA];
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            vr[v] = (int)b.range_word.range(v * 32 + 31, v * 32);
            ec[v] = (int)b.edge_count_word.range(v * 32 + 31, v * 32);
        }

        // --- Process segment modifications ---
        int ec_delta[VERTEX_PER_PMA];
        // #pragma HLS ARRAY_PARTITION variable=ec_delta complete
        for (int v = 0; v < VERTEX_PER_PMA; v++) ec_delta[v] = 0;

        int seg_list[16], seg_cnt = 0;
        // #pragma HLS ARRAY_PARTITION variable=seg_list complete
        bool seg_overflow[16];
        // #pragma HLS ARRAY_PARTITION variable=seg_overflow complete
        for (int i = 0; i < 16; i++) seg_overflow[i] = false;

        // bs_worker already resolved the EXACT segment → dedup only
        for (int i = 0; i < b.count; i++) {
            #pragma HLS PIPELINE II=1
            int ts = b.target_seg[i], ls = b.local_src[i];
            ec_delta[ls]++;

            bool found = false;
            for (int j = 0; j < seg_cnt; j++) if (seg_list[j] == ts) { found = true; break; }
            if (!found && seg_cnt < 16) seg_list[seg_cnt++] = ts;
        }

        // --- Phase A: burst-read all modified segments ---
        ap_uint<512> seg_buf[16][SEG_WORDS_PER_SEG];
        // #pragma HLS ARRAY_PARTITION variable=seg_buf complete dim=2
        #pragma HLS bind_storage variable=seg_buf type=RAM_2P impl=BRAM
        int seg_bases[16];
        // #pragma HLS ARRAY_PARTITION variable=seg_bases complete
        int seg_valid_buf[16];
        // #pragma HLS ARRAY_PARTITION variable=seg_valid_buf complete

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

        // --- Phase A.5: distribute edges in batch to per-segment dst arrays ---
        int  seg_dsts[16][16];
        // #pragma HLS ARRAY_PARTITION variable=seg_dsts complete dim=2
        int  seg_dst_cnt[16];
        #pragma HLS ARRAY_PARTITION variable=seg_dst_cnt complete
        for (int i = 0; i < 16; i++) seg_dst_cnt[i] = 0;

        for (int i = 0; i < b.count; i++) {
            #pragma HLS PIPELINE II=1
            int ts = b.target_seg[i];
            for (int si = 0; si < seg_cnt; si++) {
                #pragma HLS UNROLL
                if (seg_list[si] == ts) {
                    seg_dsts[si][seg_dst_cnt[si]++] = b.dst[i];
                }
            }
        }

        // --- Phase B: parallel seg_insert ---
        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS UNROLL factor=4
            seg_insert(seg_buf[si], seg_dsts[si], seg_dst_cnt[si],
                       seg_buf[si], seg_valid_buf[si], seg_overflow[si]);
        }

        // --- Phase C: burst-write all modified segments + VC RMW ---
        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS PIPELINE II=SEG_WORDS_PER_SEG
            int base = seg_bases[si];
            int seg  = seg_list[si];
            for (int w = 0; w < SEG_WORDS_PER_SEG; w++) {
                #pragma HLS UNROLL
                hbm[base + w] = seg_buf[si][w];
            }
            // RMW this segment's valid_count entry in the SoA VC area (1:1 per segment)
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
        hbm[wb_base + 0] = b.range_word;       // header[0]: vertex ranges (unchanged)
        hbm[wb_base + 1] = new_edge_count;     // header[1]: updated edge counts
        hbm[wb_base + 2] = 0;
        hbm[wb_base + 3] = 0;

        // Push SEGMENT_REBALANCE tasks for overflowed segments
        bool has_rebalance = false;
        for (int si = 0; si < seg_cnt; si++) {
            #pragma HLS PIPELINE II=1
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
            // ACK marker → redistribution → redist_ack → forwarded upstream
            rebalance_task_t ack_task;
            ack_task.pma        = pma;
            ack_task.payload    = -2;
            ack_task.start_byte = 0;
            ack_task.range_word = 0;
            task_fifo.write(ack_task);
            pending_cnt++;
        } else {
            // No rebalance: ACK upstream directly
            ack_out.write((ap_uint<32>)pma);
        }
    }

    // --- STOP: send STOP to redistribution, then drain pending table ---
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

    // Send STOP ACK upstream
    ack_out.write((ap_uint<32>)-1);
}

// ---- process_cmd (unchanged) ----

// void process_cmd(
//     hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
//     hls::stream<int> &cmd_count_stream,
//     hls::stream<PMA_UPDATE_MESSAGE> &cmd_batch_stream
//     // hls::stream<pma_idx_info> pma_info_stream
// )
// {
//     printf("PROCESS_CMD\n");
//     // int next_worker = 0;
//     while(true)
//     {
//         if (cmd_stream.empty()) { ap_wait(); continue; }
//         PMA_UPDATE_MESSAGE cmd_batch[MAX_CMD_NUM];
//         #pragma HLS ARRAY_PARTITION variable=cmd_batch complete
//         int cmd_count = 0;
//         bool done = false;
//         PMA_UPDATE_MESSAGE first = cmd_stream.read();
//         // switch (first.opcode)
//         // {
//             // case PMA_UPDATE_STOP:
//             //     if (first.last == 1) {
//             //         pma_idx_info info; info.pma_idx = 0; info.command_type = INSERT_STOP;
//             //         // for (int i = 0; i < BS_WORKER_NUM; i++) pma_info_stream[i].write(info);
//             //         pma_info_stream.write(info);
//             //         return;
//             //     }
//             //     break;
//             // case PMA_UPDATE_FLUSH_CACHE:
//             // {
//             //     pma_idx_info info; info.pma_idx = first.src >> 4; info.command_type = INSERT_FLUSH_CACHE;
//             //     // for (int i = 0; i < BS_WORKER_NUM; i++) pma_info_stream[i].write(info);
//             //     pma_info_stream.write(info);
//             //     break;
//             // }
//             // default:
//         // {

//         cmd_batch[cmd_count++] = first;
//         if (first.last == 1) done = true;
//         for (int i = 1; i < MAX_CMD_NUM; i++) {
//             if (done) break;
//             PMA_UPDATE_MESSAGE cmd = cmd_stream.read();
//             cmd_batch[i] = cmd; cmd_count++;
//             if (cmd.last == 1) done = true;
//         }
//         // pma_idx_info info; info.pma_idx = cmd_batch[0].src >> 4; info.command_type = INSERT_GOING;
//         // pma_info_stream[next_worker].write(info);
//         // pma_info_stream.write(info);

//         PMA_UPDATE_MESSAGE edge_info[MAX_CMD_NUM];
//         #pragma HLS ARRAY_PARTITION variable=edge_info complete
//         for (int i = 0; i < cmd_count; i++) edge_info[i] = cmd_batch[i];
//         for (int i = cmd_count; i < MAX_CMD_NUM; i++) {
//             edge_info[i].src = 0x7FFFFFFF; edge_info[i].dst = 0x7FFFFFFF;
//             edge_info[i].opcode = (PMA_UPDATE_OPCODE)0x7FFFFFFF;
//         }
//         #define BCS(a,b) do { \
//             PMA_UPDATE_MESSAGE _ta = edge_info[a], _tb = edge_info[b]; \
//             bool _gt = ((int)_ta.src > (int)_tb.src) || \
//                 ((int)_ta.src == (int)_tb.src && ((int)_ta.dst > (int)_tb.dst || \
//                 ((int)_ta.dst == (int)_tb.dst && (int)_ta.opcode > (int)_tb.opcode))); \
//             if (_gt) { edge_info[a] = _tb; edge_info[b] = _ta; } \
//         } while(0)
//         BCS( 0, 1); BCS( 2, 3); BCS( 4, 5); BCS( 6, 7);
//         BCS( 8, 9); BCS(10,11); BCS(12,13); BCS(14,15);
//         BCS( 0, 2); BCS( 1, 3); BCS( 4, 6); BCS( 5, 7);
//         BCS( 8,10); BCS( 9,11); BCS(12,14); BCS(13,15);
//         BCS( 1, 2); BCS( 5, 6); BCS( 9,10); BCS(13,14);
//         BCS( 0, 4); BCS( 1, 5); BCS( 2, 6); BCS( 3, 7);
//         BCS( 8,12); BCS( 9,13); BCS(10,14); BCS(11,15);
//         BCS( 2, 4); BCS( 3, 5); BCS(10,12); BCS(11,13);
//         BCS( 1, 2); BCS( 3, 4); BCS( 5, 6);
//         BCS( 9,10); BCS(11,12); BCS(13,14);
//         BCS( 0, 8); BCS( 1, 9); BCS( 2,10); BCS( 3,11);
//         BCS( 4,12); BCS( 5,13); BCS( 6,14); BCS( 7,15);
//         BCS( 4, 8); BCS( 5, 9); BCS( 6,10); BCS( 7,11);
//         BCS( 2, 4); BCS( 3, 5); BCS( 6, 8); BCS( 7, 9);
//         BCS(10,12); BCS(11,13);
//         BCS( 1, 2); BCS( 3, 4); BCS( 5, 6); BCS( 7, 8);
//         BCS( 9,10); BCS(11,12); BCS(13,14);
//         #undef BCS
//         // cmd_count_stream[next_worker].write(cmd_count);
//         // cmd_count_stream.write(cmd_count);
//         for (int i = 0; i < cmd_count ; i++)
//             // cmd_batch_stream[next_worker].write(edge_info[i]);
//             cmd_batch_stream.write(edge_info[i]);
//         if (first.opcode == PMA_UPDATE_STOP && first.last == 1) {
//             return;
//         }
//         // next_worker = (next_worker + 1) % BS_WORKER_NUM;
//         // }
//         // }
//     }
// }

// ---- Phase 4: Redistribution (deferred rebalance) ----
// Task types (in payload[31:0]):
//   -1 (0xFFFFFFFF): STOP — also signals alloc_merger via redist_alloc_cmd
//   -2 (0xFFFFFFFE): ACK marker
//   seg (>=0):       overflow segment — owner vertex checked:
//                    vertex_total > capacity → ENLARGE (alloc new space, move PMA)
//                    else → power-of-2 group expansion + collect/redistribute

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
    hls::stream<ap_uint<96>>& redist_alloc_cmd,
    hls::stream<ap_uint<64>>& redist_alloc_rsp,
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
    redist_alloc_cmd.write(acmd);
    ap_wait();
    ap_uint<64> arsp = redist_alloc_rsp.read();
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
        redist_alloc_cmd.write(fcmd);
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
    hls::stream<ap_uint<96>>& alloc_cmd,
    hls::stream<ap_uint<64>>& alloc_rsp,
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

    // New layout: vertex v gets old_count + ceil(count[v]/47) segments.
    // ntseg accumulates in Step 2 (same loop computes the range).
    int ntseg = 0;
    int nfe_words = 0, nvc_words = 0, nhlp = 0, npwords = 0, npbytes = 0;

    // ---- Step 1: new vertex_range (cumulative) + total segments ----
    ap_uint<512> new_range_word = 0;
    {
        int cum = 0;
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS PIPELINE II=1
            int old_cnt = 1;
            if (!is_init) {
                int vs = (v == 0) ? 0 : (int)old_range_word.range((v - 1) * 32 + 31, (v - 1) * 32);
                int ve = (int)old_range_word.range(v * 32 + 31, v * 32);
                old_cnt = ve - vs;
            }
            int extra = (count[v] + 46) / 47;        // ceil(count[v]/47)
            cum += old_cnt + extra;
            new_range_word.range(v * 32 + 31, v * 32) = (ap_uint<32>)cum;
        }
        ntseg = cum;
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
    alloc_cmd.write(acmd);
    ap_wait();
    ap_uint<64> arsp = alloc_rsp.read();
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
            alloc_cmd.write(fcmd);
        }
        ap_wait();  // barrier: FREE 排在 VEC_INFO 更新之前 (CLAUDE.md)
    }

    // ---- Step 5: update VEC_INFO ----
    VEC_INFO nvi = 0;
    nvi.range(31, 0)  = (ap_uint<32>)(nstart + data_off);
    nvi.range(63, 32) = (ap_uint<32>)npbytes;
    vi_write(hbm, vi_wo, pma_idx, nvi);
}

void redistribution_phase(
    ap_uint<512>* hbm,
    ap_uint<512>* hbm_edge_buffer,
    int data_off, int vi_wo,
    hls::stream<rebalance_task_t>& task_fifo,
    hls::stream<ap_uint<32>>& redist_ack_out,
    hls::stream<ap_uint<96>>& redist_alloc_cmd,
    hls::stream<ap_uint<64>>& redist_alloc_rsp,
    hls::stream<pre_expand_cmd_t>& pre_expand_in,
    hls::stream<ap_uint<32>>& pre_ack_out)
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
                           redist_alloc_cmd, redist_alloc_rsp,
                           onchip_buffer, edge_buffer);
            pre_ack_out.write((ap_uint<32>)cmd.pma);
            continue;
        }
        if (task_fifo.empty()) { ap_wait(); continue; }
        rebalance_task_t task = task_fifo.read();
        int payload = task.payload;
        int pm  = task.pma;
        if (payload == -1) {  // STOP
            // Signal alloc_merger that redistribution is done with allocations
            ap_uint<96> stop_cmd = 0;   // opcode=STOP
            redist_alloc_cmd.write(stop_cmd);
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
                               redist_alloc_cmd, redist_alloc_rsp,
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

// ---- Task Reader: replaces generator + router ----
// Reads edges from per-worker HBM task buffer, groups by PMA into batches.
// Two-phase design: burst-read chunks into local BRAM, then process from BRAM
// to avoid data-dependent break that prevents HLS burst inference.

#define TASK_BURST_CHUNK 256    // edges per offline burst chunk
#define WAVE_CAPACITY 8192      // max edges per wave (barrier closes the wave)

#include "radix_sort.h"

static void task_reader(
    int* task_buffer,
    int edge_count,
    int worker_id,
    hls::stream<ap_uint<32>>& ack_in,
    hls::stream<PMA_UPDATE_MESSAGE>& cmd_out,
    hls::stream<PMA_UPDATE_MESSAGE>& online_in,
    hls::stream<pre_expand_cmd_t>& pre_expand_out,
    hls::stream<ap_uint<32>>& pre_ack_in
) {

    // ---- Wave buffer: edges of the current wave (before processing) ----
    // Online edges and offline chunks both land here; a barrier (FLUSH_CACHE /
    // STOP / offline exhausted / buffer full) closes the wave. The wave is then
    // radix-sorted by src (= pma<<4 | vertex) so per-PMA segments become
    // contiguous and per-vertex insert counts fall out of a linear scan —
    // no separate profile table, no pool.
    int wave_buffer[WAVE_CAPACITY * 3];
    #pragma HLS bind_storage variable=wave_buffer type=RAM_2P impl=BRAM
    // Radix ping-pong partner (result ends up back in wave_buffer)
    int radix_tmp[WAVE_CAPACITY * 3];
    #pragma HLS bind_storage variable=radix_tmp type=RAM_2P impl=BRAM

    int processed = 0;
    int offline_base = 0;
    bool offline_done = false;
    bool online_done = false;

    while (true) {
        // ================= 收波 =================
        // Sources: online stream (edges + FLUSH/STOP barriers) and offline
        // task_buffer chunks. A wave closes on: FLUSH_CACHE (immediate update),
        // STOP (final), offline exhausted, or wave buffer full.
        int wave_cnt = 0;
        bool wave_closed = false;
        while (!wave_closed) {
            bool did_work = false;

            // online 优先（应收尽收，不积压）
            if (!online_done && !online_in.empty()) {
                PMA_UPDATE_MESSAGE om = online_in.read();
                did_work = true;
                if (om.opcode == PMA_UPDATE_STOP) {
                    online_done = true;          // 终止信号（非 barrier，offline 继续收）
                } else if (om.opcode == PMA_UPDATE_FLUSH_CACHE) {
                    wave_closed = true;          // barrier：立即更新
                } else {
                    wave_buffer[wave_cnt * 3 + 0] = om.src;
                    wave_buffer[wave_cnt * 3 + 1] = om.dst;
                    wave_buffer[wave_cnt * 3 + 2] = (int)om.opcode;
                    wave_cnt++;
                    if (wave_cnt >= WAVE_CAPACITY) wave_closed = true;
                }
            }

            // offline chunk（burst 读入 wave_buffer）
            if (!offline_done && offline_base < edge_count && wave_cnt < WAVE_CAPACITY) {
                int chunk = edge_count - offline_base;
                if (chunk > TASK_BURST_CHUNK) chunk = TASK_BURST_CHUNK;
                if (wave_cnt + chunk > WAVE_CAPACITY) chunk = WAVE_CAPACITY - wave_cnt;
                int m_axi_off = offline_base * 3;
                int dst_off = wave_cnt * 3;
                for (int j = 0; j < chunk * 3; j++) {
                    #pragma HLS PIPELINE II=1
                    wave_buffer[dst_off + j] = task_buffer[m_axi_off + j];
                }
                // sentinel 检测（edge_count 不精确时的兼容）
                int added = 0;
                for (int j = 0; j < chunk; j++) {
                    #pragma HLS PIPELINE II=1
                    if (wave_buffer[dst_off + j * 3] == -1) { offline_done = true; break; }
                    added++;
                }
                wave_cnt += added;
                offline_base += chunk;
                if (offline_base >= edge_count) offline_done = true;
                if (wave_cnt >= WAVE_CAPACITY) wave_closed = true;
                did_work = true;
            }

            // 两端完成 → 收波结束
            if (offline_done && online_done) wave_closed = true;
            if (!did_work) ap_wait();
        }

        // ================= 波处理 =================
        // 全排序：
        //   1. radix 按 src (= pma<<4|vertex) → 每 PMA 的边连续成段，段内 vertex 连续
        //   2. 段内 bitonic 按 dst（≤256）→ 每 vertex 的插入边 dst 有序
        // 排序后：
        //   - per-vertex 插入计数 = 线性扫描（profile 免费）→ pre_expand 精确
        //   - 分发 = 逐段处理（无 pool / 无锁：段内串行 + 逐批等 ACK）
        //   - 段内 dst 有序 → 为 merge 插入铺路
        radix_sort_src(wave_buffer, radix_tmp, wave_cnt);

        int i = 0;
        while (i < wave_cnt) {
            int pma = wave_buffer[i * 3] / VERTEX_PER_PMA;
            int j = i + 1;
            while (j < wave_cnt && wave_buffer[j * 3] / VERTEX_PER_PMA == pma) j++;

            // 段内按 dst 排序（≤256 才排；>256 跳过 → insert 二分插入兜底）
            if (j - i <= SEG_BITONIC_MAX && j - i > 8) {
                bitonic_segment_by_dst(wave_buffer, radix_tmp, i, j);
            }

            // per-vertex 计数（段内 vertex 连续）
            int vc[VERTEX_PER_PMA];
            #pragma HLS ARRAY_PARTITION variable=vc complete
            for (int v = 0; v < VERTEX_PER_PMA; v++) vc[v] = 0;
            for (int k = i; k < j; k++) {
                #pragma HLS PIPELINE II=1
                vc[wave_buffer[k * 3] & 0xF]++;
            }

            // 预分配（per-vertex 精确计数）→ 等 redistribution 完成扩容
            pre_expand_cmd_t cmd;
            cmd.pma = pma;
            for (int v = 0; v < VERTEX_PER_PMA; v++) cmd.count[v] = vc[v];
            pre_expand_out.write(cmd);
            ap_wait();  // barrier: cmd 发出后再读 ACK
            while (true) {
                ap_uint<32> pre_ack = pre_ack_in.read();
                if ((int)pre_ack == pma) break;
            }

            // flush 段：16 条/批，逐批等 insert 完成（ACK）
            int k = i;
            while (k < j) {
                int batch = j - k;
                if (batch > MAX_CMD_NUM) batch = MAX_CMD_NUM;
                for (int b = 0; b < batch; b++) {
                    #pragma HLS PIPELINE II=1
                    PMA_UPDATE_MESSAGE msg;
                    msg.src = wave_buffer[(k + b) * 3];
                    msg.dst = wave_buffer[(k + b) * 3 + 1];
                    msg.opcode = (PMA_UPDATE_OPCODE)wave_buffer[(k + b) * 3 + 2];
                    msg.last = (b == batch - 1) ? 1 : 0;
                    cmd_out.write(msg);
                }
                k += batch;
                ap_wait();  // barrier: batch 发出后再读 ACK
                while (true) {
                    ap_uint<32> val = ack_in.read();
                    if ((int)val == pma) break;
                }
            }
            processed += j - i;
            i = j;
        }

        // 终止：offline 与 online 都结束
        if (offline_done && online_done) break;
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
        int worker_id,             // worker index (this kernel handles pma % CHANNEL_NUM == worker_id)
        int bitmap_offset,         // byte offset in hbm_gate for bitmap
        hls::stream<ap_axiu<96,0,0,0>>& alloc_cmd_stream,
        hls::stream<ap_axiu<64,0,0,0>>& alloc_rsp_stream,
        hls::stream<ap_axiu<ALLOC_BITMAP_STORAGE_WORD_BITS,0,0,0>>& bitmap_init,  // bitmap → alloc_access
        hls::stream<PMA_UPDATE_MESSAGE>& online_in    // online updates from router
    )
    {
        #pragma HLS INTERFACE m_axi port=hbm_gate offset=slave bundle=gmem6 max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=hbm_bs   offset=slave bundle=gmem0 max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=hbm_rw   offset=slave bundle=gmem4 max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=hbm_red  offset=slave bundle=gmem5 max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=hbm_buffer_for_red offset=slave bundle=gmem8 max_read_burst_length=256 max_write_burst_length=256
        #pragma HLS INTERFACE m_axi port=task_buffer offset=slave bundle=gmem7 max_read_burst_length=256 max_write_burst_length=256

        #pragma HLS INTERFACE s_axilite port=hbm_gate bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_bs   bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_rw   bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_red  bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_buffer_for_red bundle=control
        #pragma HLS INTERFACE s_axilite port=task_buffer bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_raw_data_start bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_vecinfo_start bundle=control
        #pragma HLS INTERFACE s_axilite port=edge_count bundle=control
        #pragma HLS INTERFACE s_axilite port=worker_id bundle=control
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

        // Pre-expand command/ACK streams: task_reader → redistribution
        hls::stream<pre_expand_cmd_t> pre_expand_stream;
        #pragma HLS STREAM variable=pre_expand_stream depth=8
        hls::stream<ap_uint<32>> pre_ack_stream;
        #pragma HLS STREAM variable=pre_ack_stream depth=8

        // Internal alloc command/response streams (ap_uint, merged by alloc_merger)
        hls::stream<ap_uint<96>> gate_alloc_cmd;
        #pragma HLS STREAM variable=gate_alloc_cmd depth=8
        hls::stream<ap_uint<64>> gate_alloc_rsp;
        #pragma HLS STREAM variable=gate_alloc_rsp depth=8
        hls::stream<ap_uint<96>> redist_alloc_cmd;
        #pragma HLS STREAM variable=redist_alloc_cmd depth=8
        hls::stream<ap_uint<64>> redist_alloc_rsp;
        #pragma HLS STREAM variable=redist_alloc_rsp depth=8

        #pragma HLS DATAFLOW

        task_reader(task_buffer, edge_count, worker_id, dummy_ack_stream, cmd_from_reader, online_in,
                    pre_expand_stream, pre_ack_stream);

        // pma_gate handles bitmap init internally (reads hbm_gate, sends to alloc_access)
        // before entering its main command-processing loop.
        // Forwards header to bs_worker via hdr_fwd to avoid redundant HBM reads.
        pma_gate(hbm_gate, pma_raw_data_start, pma_vecinfo_start / 64,
                 bitmap_offset,
                 cmd_from_reader, cmd_batch_stream,
                 gate_alloc_cmd, gate_alloc_rsp, bitmap_init, hdr_fwd);

        bs_worker(hbm_bs, pma_raw_data_start,
                  hdr_fwd,
                  cmd_batch_stream, bs_out);

        insert_phase(hbm_rw, pma_raw_data_start, pma_vecinfo_start / 64,
                     bs_out,
                     task_fifo, redist_ack_stream, dummy_ack_stream);

        redistribution_phase(hbm_red, hbm_buffer_for_red,
                             pma_raw_data_start, pma_vecinfo_start / 64,
                             task_fifo, redist_ack_stream,
                             redist_alloc_cmd, redist_alloc_rsp,
                             pre_expand_stream, pre_ack_stream);

        alloc_merger(gate_alloc_cmd, gate_alloc_rsp,
                     redist_alloc_cmd, redist_alloc_rsp,
                     alloc_cmd_stream, alloc_rsp_stream);

        // No post-DATAFLOW bitmap write-back — host reconstructs bitmap from data
    }
}
