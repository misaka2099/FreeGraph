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

// Allocation constants
#define ALLOC_GRANULARITY 1024  // = INIT_VEC_LEN_IN_BYTE
#define PMA_DATA_SIZE (240 * 1024 * 1024)
#define TOTAL_BLOCKS (PMA_DATA_SIZE / ALLOC_GRANULARITY)
#define BITMAP_WORD_BITS 512
#define BITMAP_WORDS ((TOTAL_BLOCKS + BITMAP_WORD_BITS - 1) / BITMAP_WORD_BITS)

// ---- Cache (local to insert_rebalance_phase) ----
#define CACHE_LINES 64
#define MAX_HELPER_WORDS 256

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

// ---- Bitmap operations (local BRAM) ----

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

static int bm_alloc(ap_uint<512>* bm, int &cursor, int blocks)
{
    #pragma HLS INLINE off
    int found = -1;
    for (int i = cursor; i < TOTAL_BLOCKS - blocks; i++) {
        #pragma HLS PIPELINE II=1
        if (found == -1 && bm_is_free(bm, i, blocks)) found = i;
    }
    if (found == -1) {
        for (int i = 0; i < cursor && i < TOTAL_BLOCKS - blocks; i++) {
            #pragma HLS PIPELINE II=1
            if (found == -1 && bm_is_free(bm, i, blocks)) found = i;
        }
    }
    if (found != -1) {
        bm_set(bm, found, blocks);
        cursor = found + blocks;
        if (cursor >= TOTAL_BLOCKS) cursor = 0;
    }
    return found;
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

// ---- Binary search helper ----

static inline int density_capacity(int seg_cnt) { return seg_cnt * 47; }

void sub_search(int* helper, int lo, int hi, int dst, int &result)
{
    #pragma HLS INLINE
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

// ---- Phase 1: BS Worker ----
// Reads header+helper directly from HBM via m_axi, does binary search

void bs_worker(
    ap_uint<512>* hbm,
    int data_off, int vi_wo,
    hls::stream<PMA_UPDATE_MESSAGE>& edge_in,
    hls::stream<int>& cnt_in,
    hls::stream<pma_idx_info>& info_in,
    hls::stream<bs_batch_t>& bs_out)
{
    while (true) {
        auto sig = info_in.read();
        if (sig.command_type == INSERT_STOP) {
            bs_batch_t s; s.pma_idx = -1; s.count = 0; bs_out.write(s); break;
        }
        if (sig.command_type == INSERT_FLUSH_CACHE) {
            bs_batch_t f; f.pma_idx = -2; f.count = 0; bs_out.write(f); continue;
        }

        int pma = sig.pma_idx;
        int cnt = cnt_in.read();

        // Read VEC_INFO
        VEC_INFO vi = vi_read(hbm, vi_wo, pma);
        if (vi == 0) {
            // Empty PMA: 16 vertices × 1 segment each, vertex v = segment v
            // No binary search needed — target_seg = local_src
            bs_batch_t res;
            res.pma_idx = pma; res.op_mask = 0; res.count = cnt;
            for (int i = 0; i < cnt; i++) {
                #pragma HLS PIPELINE II=1
                PMA_UPDATE_MESSAGE e = edge_in.read();
                res.dst[i] = e.dst;
                res.local_src[i] = e.src & 0xF;
                res.target_seg[i] = e.src & 0xF;  // vertex v → segment v
            }
            bs_out.write(res);
            continue;
        }

        int start_byte = (int)vi.range(31, 0);
        int byte_off   = start_byte - data_off;

        // Read header
        ap_uint<512> hdr[HEADER_WORDS];
        hbm_read_words(hbm, byte_off, HEADER_WORDS, hdr);
        int total_segs = (int)hdr[0].range(15 * 32 + 31, 15 * 32);

        // Read helper
        int hlp_words = (total_segs + HELPER_ENTRIES_PER_WORD - 1) / HELPER_ENTRIES_PER_WORD;
        if (hlp_words > MAX_HELPER_WORDS) hlp_words = MAX_HELPER_WORDS;
        ap_uint<512> hlp_buf[MAX_HELPER_WORDS];
        #pragma HLS bind_storage variable=hlp_buf type=RAM_2P impl=BRAM
        hbm_read_words(hbm, byte_off + HEADER_BYTES, hlp_words, hlp_buf);

        // Unpack bs_helper
        int bs_helper[10240];
        #pragma HLS bind_storage variable=bs_helper type=RAM_2P impl=BRAM
        for (int w = 0; w < hlp_words; w++) {
            #pragma HLS PIPELINE II=1
            ap_uint<512> hw = hlp_buf[w];
            for (int s = 0; s < 8; s++) {
                int seg = w * 8 + s;
                if (seg < total_segs)
                    bs_helper[seg] = (int)hw.range(s * 64 + 31, s * 64);
            }
        }

        // Extract vertex_range from header
        int vr[VERTEX_PER_PMA];
        for (int v = 0; v < VERTEX_PER_PMA; v++)
            #pragma HLS UNROLL
            vr[v] = (int)hdr[0].range(v * 32 + 31, v * 32);

        // Process edges
        bs_batch_t res;
        res.pma_idx = pma; res.op_mask = 0; res.count = cnt;

        for (int i = 0; i < cnt; i++) {
            #pragma HLS PIPELINE II=1
            PMA_UPDATE_MESSAGE e = edge_in.read();
            res.dst[i] = e.dst;
            res.local_src[i] = e.src & 0xF;
            int op = (int)e.opcode;
            if (op == PMA_UPDATE_INSERT_EDGE || op == PMA_UPDATE_DELETE_EDGE) {
                int ls = e.src & 0xF;
                int lo = (ls == 0) ? 0 : vr[ls - 1];
                int hi = vr[ls];
                int rel = -1;
                sub_search(bs_helper, lo, hi, e.dst, rel);
                if (rel == -1) rel = 0;
                res.target_seg[i] = rel + lo;
                if (op == PMA_UPDATE_DELETE_EDGE) res.op_mask[i] = 1;
            } else {
                res.target_seg[i] = 0;
            }
        }
        bs_out.write(res);
    }
}

// ---- Phase 2+3: Insert + Rebalance (owns header+helper cache) ----

void insert_rebalance_phase(
    ap_uint<512>* hbm,
    int data_off, int vi_wo, int bm_wo,
    hls::stream<bs_batch_t>& bs0, hls::stream<bs_batch_t>& bs1,
    hls::stream<bs_batch_t>& bs2, hls::stream<bs_batch_t>& bs3,
    hls::stream<COMMAND_STREAM>& ack_out)
{
    // --- Local cache: header + helper ---
    ap_uint<512> ch[CACHE_LINES][HEADER_WORDS];
    #pragma HLS bind_storage variable=ch type=ram_2p impl=BRAM
    ap_uint<512> chp[CACHE_LINES][MAX_HELPER_WORDS];
    #pragma HLS bind_storage variable=chp type=ram_2p impl=URAM
    ap_uint<64>   cvi[CACHE_LINES];
    #pragma HLS bind_storage variable=cvi type=ram_1p impl=BRAM
    ap_uint<CACHE_LINES> cvalid = 0;
    int cpma[CACHE_LINES];
    int chpw[CACHE_LINES];
    int ctseg[CACHE_LINES];
    for (int i = 0; i < CACHE_LINES; i++) cpma[i] = -1;

    // --- Bitmap ---
    ap_uint<512> bm[BITMAP_WORDS];
    #pragma HLS bind_storage variable=bm type=ram_2p impl=BRAM
    int cursor = 0;
    // Init from HBM
    for (int i = 0; i < BITMAP_WORDS; i++)
        #pragma HLS PIPELINE II=1
        bm[i] = hbm[bm_wo + i];

    ap_uint<512> empty_vertex_range = 0;
    for (int v = 0; v < VERTEX_PER_PMA; v++)
        empty_vertex_range.range(v * 32 + 31, v * 32) = v + 1;

    int stop_cnt = 0, flush_cnt = 0;
    while (stop_cnt < BS_WORKER_NUM) {
        bs_batch_t b;
        bool got = false;
        if (!got && !bs0.empty()) { b = bs0.read(); got = true; }
        if (!got && !bs1.empty()) { b = bs1.read(); got = true; }
        if (!got && !bs2.empty()) { b = bs2.read(); got = true; }
        if (!got && !bs3.empty()) { b = bs3.read(); got = true; }
        if (!got) { ap_wait(); continue; }

        if (b.pma_idx == -1) { stop_cnt++; continue; }
        if (b.pma_idx == -2) { flush_cnt++; continue; }

        int pma = b.pma_idx;
        int line = ((pma % CACHE_LINES) + CACHE_LINES) % CACHE_LINES;

        // --- Load cache (init PMA if first access) ---
        bool hit = cvalid[line] && cpma[line] == pma;
        if (!hit) {
            cvalid[line] = 0;
            VEC_INFO vi = vi_read(hbm, vi_wo, pma);

            if (vi != 0) {
                // Existing PMA: load header + helper from HBM
                int sbyte = (int)vi.range(31, 0);
                int boff  = sbyte - data_off;
                hbm_read_words(hbm, boff, HEADER_WORDS, ch[line]);
                int tseg = (int)ch[line][0].range(15 * 32 + 31, 15 * 32);
                int hpw  = (tseg + 7) / 8;
                if (hpw > 0) hbm_read_words(hbm, boff + HEADER_BYTES, hpw, chp[line]);
                cvi[line] = vi; cvalid[line] = 1; cpma[line] = pma;
                chpw[line] = hpw; ctseg[line] = tseg;
            } else {
                // Empty PMA (first access): alloc + init in HBM, then process edges below
                int isegs = 16, ihlp = (isegs + 7) / 8;
                int ibytes = (HEADER_WORDS + ihlp + isegs * SEG_WORDS_PER_SEG) * 64;
                int iblocks = (ibytes + ALLOC_GRANULARITY - 1) / ALLOC_GRANULARITY;
                int blk = bm_alloc(bm, cursor, iblocks);
                int nstart = blk * ALLOC_GRANULARITY;

                // Write header
                ap_uint<512> hdr[HEADER_WORDS];
                hdr[0] = empty_vertex_range; hdr[1] = 0; hdr[2] = 0; hdr[3] = 0;
                hbm_write_words(hbm, nstart, HEADER_WORDS, hdr);

                // Write helper (all UNUSED_ICON = -1)
                ap_uint<512> hlp[MAX_HELPER_WORDS];
                for (int w = 0; w < ihlp; w++) hlp[w] = (ap_uint<512>)-1;
                hbm_write_words(hbm, nstart + HEADER_BYTES, ihlp, hlp);

                // Write empty segments
                ap_uint<512> es = (ap_uint<512>)-1; es.range(511, 480) = 0;
                for (int s = 0; s < isegs; s++) {
                    int so = nstart + HEADER_BYTES + ihlp * 64 + s * SEG_BYTES;
                    for (int w = 0; w < SEG_WORDS_PER_SEG; w++)
                        hbm[so / 64 + w] = (w == 3) ? es : (ap_uint<512>)-1;
                }

                VEC_INFO nvi = 0;
                nvi.range(31, 0) = (ap_uint<32>)(nstart + data_off);
                nvi.range(63, 32) = (ap_uint<32>)ibytes;
                vi_write(hbm, vi_wo, pma, nvi);

                // Fill cache
                for (int w = 0; w < HEADER_WORDS; w++) ch[line][w] = hdr[w];
                for (int w = 0; w < ihlp; w++) chp[line][w] = hlp[w];
                cvi[line] = nvi; cvalid[line] = 1; cpma[line] = pma;
                chpw[line] = ihlp; ctseg[line] = isegs;
            }
        }

        int tseg  = ctseg[line];
        int hpw   = chpw[line];
        int sbyte = (int)cvi[line].range(31, 0);
        int boff  = sbyte - data_off;

        // Read edge_count + vertex_range from cache
        int ec[VERTEX_PER_PMA], vr[VERTEX_PER_PMA];
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            vr[v] = (int)ch[line][0].range(v * 32 + 31, v * 32);
            ec[v] = (int)ch[line][1].range(v * 32 + 31, v * 32);
        }

        // --- Process segment modifications ---
        int ec_delta[VERTEX_PER_PMA];
        #pragma HLS ARRAY_PARTITION variable=ec_delta complete
        ap_uint<16> ins_mask = 0;
        for (int v = 0; v < VERTEX_PER_PMA; v++) ec_delta[v] = 0;

        int seg_list[16], seg_cnt = 0;
        #pragma HLS ARRAY_PARTITION variable=seg_list complete

        for (int i = 0; i < b.count; i++) {
            #pragma HLS PIPELINE II=1
            int ts = b.target_seg[i], ls = b.local_src[i];
            bool del = b.op_mask[i];
            if (del) ec_delta[ls]--;
            else { ec_delta[ls]++; ins_mask[ls] = 1; }

            bool found = false;
            for (int j = 0; j < seg_cnt; j++) if (seg_list[j] == ts) { found = true; break; }
            if (!found && seg_cnt < 16) seg_list[seg_cnt++] = ts;
        }

        ap_uint<512> seg_buf[SEG_WORDS_PER_SEG];
        #pragma HLS ARRAY_PARTITION variable=seg_buf complete

        for (int si = 0; si < seg_cnt; si++) {
            int seg = seg_list[si];
            int seg_bo = boff + HEADER_BYTES + hpw * 64 + seg * SEG_BYTES;

            // Read segment
            hbm_read_words(hbm, seg_bo, SEG_WORDS_PER_SEG, seg_buf);

            // Unpack
            int lb[SEGMENT_SIZE];
            #pragma HLS ARRAY_PARTITION variable=lb complete
            for (int w = 0; w < SEG_WORDS_PER_SEG; w++)
                #pragma HLS UNROLL
                for (int k = 0; k < 16; k++)
                    lb[w * 16 + k] = (int)seg_buf[w].range(32 * (k + 1) - 1, 32 * k);

            int valid = lb[VALID_COUNT_IDX];

            int ins_dst[16], ins_n = 0;
            int del_dst[16], del_n = 0;
            #pragma HLS ARRAY_PARTITION variable=ins_dst complete
            #pragma HLS ARRAY_PARTITION variable=del_dst complete
            for (int i = 0; i < b.count; i++) {
                #pragma HLS UNROLL
                if (b.target_seg[i] == seg) {
                    if (b.op_mask[i]) del_dst[del_n++] = b.dst[i];
                    else              ins_dst[ins_n++] = b.dst[i];
                }
            }

            // Filter deletes
            int tmp[EDGE_SLOTS_PER_SEG];
            #pragma HLS ARRAY_PARTITION variable=tmp complete
            int w = 0, d = 0;
            for (int r = 0; r < valid; r++) {
                #pragma HLS PIPELINE II=1
                if (d < del_n && lb[r] == del_dst[d]) d++;
                else tmp[w++] = lb[r];
            }
            int nvalid = w;

            // Merge inserts
            int s = 0, n = 0, ii = 0;
            while (s < nvalid && ii < ins_n) {
                #pragma HLS PIPELINE II=1
                if (tmp[s] <= ins_dst[ii]) lb[n++] = tmp[s++];
                else lb[n++] = ins_dst[ii++];
            }
            while (s < nvalid) lb[n++] = tmp[s++];
            while (ii < ins_n) lb[n++] = ins_dst[ii++];
            for (int k = n; k < EDGE_SLOTS_PER_SEG; k++) lb[k] = UNUSED_ICON;
            lb[VALID_COUNT_IDX] = n;

            // Pack and write back
            for (int ww = 0; ww < SEG_WORDS_PER_SEG; ww++) {
                #pragma HLS UNROLL
                seg_buf[ww] = 0;
                for (int k = 0; k < 16; k++)
                    seg_buf[ww].range(32 * (k + 1) - 1, 32 * k) = (ap_uint<32>)lb[ww * 16 + k];
            }
            hbm_write_words(hbm, seg_bo, SEG_WORDS_PER_SEG, seg_buf);

            // Update helper in cache
            int hw_idx = seg / 8, hw_slot = seg % 8;
            ap_uint<64> hv = 0;
            hv.range(31, 0)  = (ap_uint<32>)lb[0];
            hv.range(63, 32) = (ap_uint<32>)n;
            chp[line][hw_idx].range(hw_slot * 64 + 63, hw_slot * 64) = hv;
        }

        // Write-back helper to HBM
        int hlp_bo = boff + HEADER_BYTES;
        hbm_write_words(hbm, hlp_bo, hpw, chp[line]);

        // Update edge_count in cache
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            ec[v] += ec_delta[v];
            ch[line][1].range(v * 32 + 31, v * 32) = (ap_uint<32>)ec[v];
        }

        // --- Density check + enlarge ---
        bool enlarged = false;
        int esegs[VERTEX_PER_PMA];
        for (int v = 0; v < VERTEX_PER_PMA; v++) {
            #pragma HLS UNROLL
            esegs[v] = 0;
            int vs = (v == 0) ? 0 : vr[v - 1];
            int ve = vr[v], sc = ve - vs;
            if (ec[v] > density_capacity(sc)) {
                enlarged = true;
                esegs[v] = PMA_ENLARGE_FACTOR * sc;
            }
        }

        int work_bo = boff;  // HBM byte offset of current PMA

        if (enlarged) {
            int nvr[VERTEX_PER_PMA];
            int acc = 0;
            for (int v = 0; v < VERTEX_PER_PMA; v++) {
                #pragma HLS PIPELINE II=1
                acc += esegs[v];
                nvr[v] = vr[v] + acc;
            }
            int ntseg = nvr[VERTEX_PER_PMA - 1];
            int nhpw  = (ntseg + 7) / 8;
            int npwords = HEADER_WORDS + nhpw + ntseg * SEG_WORDS_PER_SEG;
            int npbytes = npwords * 64;
            int nblocks = (npbytes + ALLOC_GRANULARITY - 1) / ALLOC_GRANULARITY;

            int nblk = bm_alloc(bm, cursor, nblocks);
            int nstart = nblk * ALLOC_GRANULARITY;

            int ohpw = (tseg + 7) / 8;
            int oseg_base = boff + HEADER_BYTES + ohpw * 64;
            int nseg_base = nstart + HEADER_BYTES + nhpw * 64;

            // Move segments (backward: high vertex → low vertex)
            ap_uint<512> seg_tmp[SEG_WORDS_PER_SEG];
            for (int v = VERTEX_PER_PMA - 1; v >= 0; v--) {
                int ovs = (v == 0) ? 0 : vr[v - 1];
                int ove = vr[v], ocnt = ove - ovs;
                int nvs = (v == 0) ? 0 : nvr[v - 1];
                for (int seg = ocnt - 1; seg >= 0; seg--) {
                    #pragma HLS PIPELINE off
                    int ooff = oseg_base + (ovs + seg) * SEG_BYTES;
                    int noff = nseg_base + (nvs + seg) * SEG_BYTES;
                    hbm_read_words(hbm, ooff, SEG_WORDS_PER_SEG, seg_tmp);
                    hbm_write_words(hbm, noff, SEG_WORDS_PER_SEG, seg_tmp);
                }
            }

            // Fill extra segments
            ap_uint<512> es = (ap_uint<512>)-1; es.range(511, 480) = 0;
            for (int v = 0; v < VERTEX_PER_PMA; v++) {
                int ovs = (v == 0) ? 0 : vr[v - 1];
                int ove = vr[v], ocnt = ove - ovs;
                int nvs = (v == 0) ? 0 : nvr[v - 1];
                int nve = nvr[v], extra = nve - nvs - ocnt;
                for (int s = 0; s < extra; s++) {
                    int foff = nseg_base + (nvs + ocnt + s) * SEG_BYTES;
                    for (int w = 0; w < SEG_WORDS_PER_SEG; w++)
                        hbm[foff / 64 + w] = (w == 3) ? es : (ap_uint<512>)-1;
                }
            }

            // Rebuild helper
            ap_uint<512> nhlp[MAX_HELPER_WORDS];
            for (int w = 0; w < nhpw; w++) nhlp[w] = (ap_uint<512>)-1;
            for (int v = 0; v < VERTEX_PER_PMA; v++) {
                int ovs = (v == 0) ? 0 : vr[v - 1], ove = vr[v], ocnt = ove - ovs;
                int nvs = (v == 0) ? 0 : nvr[v - 1];
                for (int s = 0; s < ocnt; s++) {
                    int oseg = ovs + s, nseg = nvs + s;
                    int ow = oseg / 8, os = oseg % 8, nw = nseg / 8, ns = nseg % 8;
                    ap_uint<64> v = chp[line][ow].range(os * 64 + 63, os * 64);
                    nhlp[nw].range(ns * 64 + 63, ns * 64) = v;
                }
            }

            // Free old
            int obytes = (HEADER_WORDS + ohpw + tseg * SEG_WORDS_PER_SEG) * 64;
            int oblocks = (obytes + ALLOC_GRANULARITY - 1) / ALLOC_GRANULARITY;
            bm_reset(bm, boff / ALLOC_GRANULARITY, oblocks);

            // Write new header + helper
            ap_uint<512> nhdr[HEADER_WORDS];
            nhdr[0] = 0;
            for (int v = 0; v < VERTEX_PER_PMA; v++)
                nhdr[0].range(v * 32 + 31, v * 32) = nvr[v];
            nhdr[1] = ch[line][1]; nhdr[2] = ch[line][2]; nhdr[3] = ch[line][3];
            hbm_write_words(hbm, nstart, HEADER_WORDS, nhdr);
            hbm_write_words(hbm, nstart + HEADER_BYTES, nhpw, nhlp);

            // Update VEC_INFO
            VEC_INFO nvi = 0;
            nvi.range(31, 0) = (ap_uint<32>)(nstart + data_off);
            nvi.range(63, 32) = (ap_uint<32>)npbytes;
            vi_write(hbm, vi_wo, pma, nvi);

            // Update cache
            for (int w = 0; w < HEADER_WORDS; w++) ch[line][w] = nhdr[w];
            for (int w = 0; w < nhpw; w++) chp[line][w] = nhlp[w];
            cvi[line] = nvi; ctseg[line] = ntseg; chpw[line] = nhpw;

            work_bo = nstart;
        }

        // Write back final header
        hbm_write_words(hbm, work_bo, HEADER_WORDS, ch[line]);

        // Ack
        COMMAND_STREAM ack; ack.data = pma; ack.keep = 0xF; ack.strb = 0xF; ack.user = 0; ack.last = 0;
        ack_out.write(ack);
    }

    // Write bitmap back to HBM
    for (int i = 0; i < BITMAP_WORDS; i++)
        #pragma HLS PIPELINE II=1
        hbm[bm_wo + i] = bm[i];
}

// ---- process_cmd (unchanged) ----

void process_cmd(
    hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
    hls::stream<int> cmd_count_stream[BS_WORKER_NUM],
    hls::stream<PMA_UPDATE_MESSAGE> cmd_batch_stream[BS_WORKER_NUM],
    hls::stream<pma_idx_info> pma_info_stream[BS_WORKER_NUM]
)
{
    printf("PROCESS_CMD\n");
    int next_worker = 0;
    while(true)
    {
        if (cmd_stream.empty()) { ap_wait(); continue; }
        PMA_UPDATE_MESSAGE cmd_batch[MAX_CMD_NUM];
        #pragma HLS ARRAY_PARTITION variable=cmd_batch complete
        int cmd_count = 0;
        bool done = false;
        PMA_UPDATE_MESSAGE first = cmd_stream.read();
        switch (first.opcode)
        {
            case PMA_UPDATE_STOP:
                if (first.last == 1) {
                    pma_idx_info info; info.pma_idx = 0; info.command_type = INSERT_STOP;
                    for (int i = 0; i < BS_WORKER_NUM; i++) pma_info_stream[i].write(info);
                    return;
                }
                break;
            case PMA_UPDATE_FLUSH_CACHE:
            {
                pma_idx_info info; info.pma_idx = first.src >> 4; info.command_type = INSERT_FLUSH_CACHE;
                for (int i = 0; i < BS_WORKER_NUM; i++) pma_info_stream[i].write(info);
                break;
            }
            default:
            {
                cmd_batch[cmd_count++] = first;
                if (first.last == 1) done = true;
                for (int i = 1; i < MAX_CMD_NUM; i++) {
                    if (done) break;
                    PMA_UPDATE_MESSAGE cmd = cmd_stream.read();
                    cmd_batch[i] = cmd; cmd_count++;
                    if (cmd.last == 1) done = true;
                }
                pma_idx_info info; info.pma_idx = cmd_batch[0].src >> 4; info.command_type = INSERT_GOING;
                pma_info_stream[next_worker].write(info);

                PMA_UPDATE_MESSAGE edge_info[MAX_CMD_NUM];
                #pragma HLS ARRAY_PARTITION variable=edge_info complete
                for (int i = 0; i < cmd_count; i++) edge_info[i] = cmd_batch[i];
                for (int i = cmd_count; i < MAX_CMD_NUM; i++) {
                    edge_info[i].src = 0x7FFFFFFF; edge_info[i].dst = 0x7FFFFFFF;
                    edge_info[i].opcode = (PMA_UPDATE_OPCODE)0x7FFFFFFF;
                }
                #define BCS(a,b) do { \
                    PMA_UPDATE_MESSAGE _ta = edge_info[a], _tb = edge_info[b]; \
                    bool _gt = ((int)_ta.src > (int)_tb.src) || \
                        ((int)_ta.src == (int)_tb.src && ((int)_ta.dst > (int)_tb.dst || \
                        ((int)_ta.dst == (int)_tb.dst && (int)_ta.opcode > (int)_tb.opcode))); \
                    if (_gt) { edge_info[a] = _tb; edge_info[b] = _ta; } \
                } while(0)
                BCS( 0, 1); BCS( 2, 3); BCS( 4, 5); BCS( 6, 7);
                BCS( 8, 9); BCS(10,11); BCS(12,13); BCS(14,15);
                BCS( 0, 2); BCS( 1, 3); BCS( 4, 6); BCS( 5, 7);
                BCS( 8,10); BCS( 9,11); BCS(12,14); BCS(13,15);
                BCS( 1, 2); BCS( 5, 6); BCS( 9,10); BCS(13,14);
                BCS( 0, 4); BCS( 1, 5); BCS( 2, 6); BCS( 3, 7);
                BCS( 8,12); BCS( 9,13); BCS(10,14); BCS(11,15);
                BCS( 2, 4); BCS( 3, 5); BCS(10,12); BCS(11,13);
                BCS( 1, 2); BCS( 3, 4); BCS( 5, 6);
                BCS( 9,10); BCS(11,12); BCS(13,14);
                BCS( 0, 8); BCS( 1, 9); BCS( 2,10); BCS( 3,11);
                BCS( 4,12); BCS( 5,13); BCS( 6,14); BCS( 7,15);
                BCS( 4, 8); BCS( 5, 9); BCS( 6,10); BCS( 7,11);
                BCS( 2, 4); BCS( 3, 5); BCS( 6, 8); BCS( 7, 9);
                BCS(10,12); BCS(11,13);
                BCS( 1, 2); BCS( 3, 4); BCS( 5, 6); BCS( 7, 8);
                BCS( 9,10); BCS(11,12); BCS(13,14);
                #undef BCS
                cmd_count_stream[next_worker].write(cmd_count);
                for (int i = 0; i < cmd_count ; i++)
                    cmd_batch_stream[next_worker].write(edge_info[i]);
                next_worker = (next_worker + 1) % BS_WORKER_NUM;
                break;
            }
        }
    }
}

// ---- Top-level ----

extern "C"
{
    void pma_insert_edge(
        ap_uint<512>* hbm_bs0,    // BS worker 0 read-only
        ap_uint<512>* hbm_bs1,    // BS worker 1 read-only
        ap_uint<512>* hbm_bs2,    // BS worker 2 read-only
        ap_uint<512>* hbm_bs3,    // BS worker 3 read-only
        ap_uint<512>* hbm_rw,     // insert+rebalance read-write
        int pma_raw_data_start,
        int pma_vecinfo_start,
        int pma_bm_start,

        hls::stream<PMA_UPDATE_MESSAGE> &cmd_stream,
        hls::stream<COMMAND_STREAM> &ack_back_to_router
    )
    {
        #pragma HLS INTERFACE m_axi port=hbm_bs0 offset=slave bundle=gmem0
        #pragma HLS INTERFACE m_axi port=hbm_bs1 offset=slave bundle=gmem1
        #pragma HLS INTERFACE m_axi port=hbm_bs2 offset=slave bundle=gmem2
        #pragma HLS INTERFACE m_axi port=hbm_bs3 offset=slave bundle=gmem3
        #pragma HLS INTERFACE m_axi port=hbm_rw  offset=slave bundle=gmem4
        #pragma HLS INTERFACE s_axilite port=hbm_bs0 bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_bs1 bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_bs2 bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_bs3 bundle=control
        #pragma HLS INTERFACE s_axilite port=hbm_rw  bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_raw_data_start bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_vecinfo_start bundle=control
        #pragma HLS INTERFACE s_axilite port=pma_bm_start bundle=control
        #pragma HLS INTERFACE s_axilite port=return bundle=control
        #pragma HLS INTERFACE axis port=cmd_stream
        #pragma HLS INTERFACE axis port=ack_back_to_router

        hls::stream<int> cmd_count_stream[BS_WORKER_NUM];
        #pragma HLS STREAM variable=cmd_count_stream depth=8
        hls::stream<PMA_UPDATE_MESSAGE> cmd_batch_stream[BS_WORKER_NUM];
        #pragma HLS STREAM variable=cmd_batch_stream depth=32
        hls::stream<pma_idx_info> pma_info_stream[BS_WORKER_NUM];
        #pragma HLS STREAM variable=pma_info_stream depth=8

        hls::stream<bs_batch_t> bs_out[BS_WORKER_NUM];
        #pragma HLS STREAM variable=bs_out depth=8

        #pragma HLS DATAFLOW

        process_cmd(cmd_stream, cmd_count_stream, cmd_batch_stream, pma_info_stream);

        bs_worker(hbm_bs0, pma_raw_data_start, pma_vecinfo_start / 64,
                  cmd_batch_stream[0], cmd_count_stream[0], pma_info_stream[0], bs_out[0]);

        bs_worker(hbm_bs1, pma_raw_data_start, pma_vecinfo_start / 64,
                  cmd_batch_stream[1], cmd_count_stream[1], pma_info_stream[1], bs_out[1]);

        bs_worker(hbm_bs2, pma_raw_data_start, pma_vecinfo_start / 64,
                  cmd_batch_stream[2], cmd_count_stream[2], pma_info_stream[2], bs_out[2]);

        bs_worker(hbm_bs3, pma_raw_data_start, pma_vecinfo_start / 64,
                  cmd_batch_stream[3], cmd_count_stream[3], pma_info_stream[3], bs_out[3]);

        insert_rebalance_phase(hbm_rw, pma_raw_data_start, pma_vecinfo_start / 64, pma_bm_start / 64,
                               bs_out[0], bs_out[1], bs_out[2], bs_out[3], ack_back_to_router);
    }
}
