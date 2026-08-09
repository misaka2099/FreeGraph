// Pure-C verification harness for merge_pipeline's chunked merge algorithm.
//
// Mirrors merge_pipeline.cpp merge_phase per-vertex logic:
//   - chunked read of vertex segments (CHUNK_SEGS = MAX_ONCHIP_SEG = 1024)
//   - per-chunk two-pointer merge (existing from on-chip buffer, new from new_edges)
//   - empty chunk skip (new_edges held across empty chunks)
//   - trailing new_edges appended after all chunks
//   - redistribute base+extra across segments; VC/FE consistency
//
// Reference: sorted union of existing + new edges.
//
// Corpus format (free-form whitespace):
//   S nidx
//   vc[0..S-1]                      (S ints, each 0..63)
//   seg0 edges (vc[0] ints), seg1 edges (vc[1] ints), ... (S lines/values)
//   new_edges (nidx ints, sorted)
// Existing segment edges must be sorted; new_edges must be sorted.
//
// Usage:
//   test_merge_chunk <corpus.txt>
//   test_merge_chunk --fuzz <N>      (N random internally-generated cases)
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>

static const int CHUNK_SEGS = 1024;            // = MAX_ONCHIP_SEG
static const int EDGE_SLOTS_PER_SEG = 63;      // physical max per segment word
static const int UNUSED_ICON = -1;

// ---- chunked merge (logical mirror of merge_pipeline.cpp) ----
// existing_segs[s] = sorted vector of valid edges in segment s
// new_edges = sorted vector
// returns merged sorted edge list (what the kernel writes to hbm_buffer)
static std::vector<int> chunked_merge(
    const std::vector<std::vector<int>>& existing_segs,
    const std::vector<int>& new_edges)
{
    int S = (int)existing_segs.size();
    int nidx = (int)new_edges.size();
    std::vector<int> merged;
    merged.reserve(64 * S + nidx);
    int n = 0;

    for (int chunk_start = 0; chunk_start < S; chunk_start += CHUNK_SEGS) {
        int chunk_segs = std::min(CHUNK_SEGS, S - chunk_start);

        int seg_idx = -1;
        int seg_rem = 0;
        int slot = 0;
        int e_cur = 0;
        bool have_e = false;

        while (true) {
            // load next existing edge when needed
            if (!have_e) {
                if (seg_rem > 0) {
                    const std::vector<int>& es = existing_segs[chunk_start + seg_idx];
                    e_cur = es[slot];
                    slot++; seg_rem--;
                    have_e = true;
                } else {
                    seg_idx++;
                    while (seg_idx < chunk_segs) {
                        const std::vector<int>& es = existing_segs[chunk_start + seg_idx];
                        int vc = (int)es.size();
                        if (vc > EDGE_SLOTS_PER_SEG) vc = EDGE_SLOTS_PER_SEG;
                        if (vc > 0) {
                            seg_rem = vc; slot = 0;
                            e_cur = es[slot];
                            slot = 1; seg_rem--;
                            have_e = true;
                            break;
                        }
                        seg_idx++;
                    }
                }
            }
            // chunk's existing edges exhausted -> next chunk (new_edges held)
            if (!have_e) break;

            // pick smaller
            int pick;
            if (n >= nidx) { pick = e_cur; have_e = false; }
            else if (new_edges[n] < e_cur) { pick = new_edges[n++]; }
            else { pick = e_cur; have_e = false; }
            merged.push_back(pick);
        }
    }
    // trailing: remaining new_edges are all >= every existing edge
    while (n < nidx) { merged.push_back(new_edges[n++]); }
    return merged;
}

// ---- reference: sorted union ----
static std::vector<int> reference_union(
    const std::vector<std::vector<int>>& existing_segs,
    const std::vector<int>& new_edges)
{
    std::vector<int> all;
    for (const auto& es : existing_segs) all.insert(all.end(), es.begin(), es.end());
    all.insert(all.end(), new_edges.begin(), new_edges.end());
    std::sort(all.begin(), all.end());
    return all;
}

// ---- redistribute base+extra across S segments (mirror of redistribute_edge_from_hbm) ----
// Returns false on any violation. Fills per-segment valid_count and first_edge.
static bool check_redistribute(const std::vector<int>& merged, int S,
                               std::vector<int>& out_vc, std::vector<int>& out_fe)
{
    out_vc.assign(S, 0);
    out_fe.assign(S, UNUSED_ICON);
    if (S == 0) return merged.empty();

    int total = (int)merged.size();
    int base = total / S;
    int extra = total % S;

    for (int s = 0; s < S; s++) {
        int fill = base + (s < extra ? 1 : 0);
        if (fill > EDGE_SLOTS_PER_SEG) return false;          // capacity overflow
        int off = s * base + (s < extra ? s : extra);         // edge offset of segment s
        out_vc[s] = fill;
        if (fill > 0) {
            out_fe[s] = merged[off];
            for (int k = 1; k < fill; k++)                    // segment must stay sorted
                if (merged[off + k] < merged[off + k - 1]) return false;
        }
    }
    // full consumption: last offset == total
    return (S * base + (S < extra ? S : extra)) == total;
}

// ---- run one case; returns true on pass ----
static bool run_case(const std::vector<std::vector<int>>& existing,
                     const std::vector<int>& new_edges, int case_no, bool verbose)
{
    int S = (int)existing.size();
    int nidx = (int)new_edges.size();
    std::string why;

    // input validation — merge 算法依赖 PMA 前置条件:
    //   (a) 每段内部有序, vc <= 63
    //   (b) 顶点内 segment 按序全局有序 (PMA 不变式)
    //   (c) existing + new <= S*47 (pre-expand 容量保证, 内核不会出现超容)
    // 违反任一条视为坏输入, 跳过算法校验 (内核在真实 PMA 上不会遇到)。
    bool input_ok = true;
    int last_max = -1;
    int old_edges = 0;
    for (int s = 0; s < S && input_ok; s++) {
        if ((int)existing[s].size() > EDGE_SLOTS_PER_SEG) input_ok = false;
        for (size_t k = 1; k < existing[s].size(); k++)
            if (existing[s][k] < existing[s][k - 1]) input_ok = false;
        if (!existing[s].empty()) {
            if (existing[s].front() < last_max) input_ok = false;   // 全局有序
            last_max = existing[s].back();
            old_edges += (int)existing[s].size();
        }
    }
    for (int k = 1; k < nidx; k++)
        if (new_edges[k] < new_edges[k - 1]) input_ok = false;
    if (old_edges + nidx > S * 47) input_ok = false;

    bool ok = input_ok;
    if (!input_ok) why = "bad input (violates PMA invariant: global order / vc / capacity)";

    if (ok) {
        std::vector<int> merged = chunked_merge(existing, new_edges);
        std::vector<int> ref = reference_union(existing, new_edges);
        if (merged != ref) {
            ok = false; why = "merged != sorted union";
        }
        std::vector<int> out_vc, out_fe;
        if (ok && !check_redistribute(merged, S, out_vc, out_fe)) {
            ok = false; why = "redistribute violation";
        }
        if (ok && (int)merged.size() > S * 47) { ok = false; why = "capacity > S*47"; }
        if (ok && S > 0) {
            // FE consistency: filled segment's first_edge == its first edge
            for (int s = 0; s < S; s++) {
                if (out_vc[s] > 0) {
                    int off = 0;
                    int base = (int)merged.size() / S, extra = (int)merged.size() % S;
                    off = s * base + (s < extra ? s : extra);
                    if (out_fe[s] != merged[off]) { ok = false; why = "FE mismatch"; break; }
                }
            }
        }
        if (!ok && verbose) {
            printf("  case %d FAIL: %s (S=%d nidx=%d)\n", case_no, why.c_str(), S, nidx);
        }
    }
    return ok;
}

// ---- deterministic PRNG for fuzz mode ----
static uint32_t rng_state = 0x12345678;
static uint32_t rng_next() {
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state >> 8;
}
static int rng_choice(int lo, int hi) { return lo + (int)(rng_next() % (uint32_t)(hi - lo + 1)); }

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <corpus.txt> | --fuzz <N>\n", argv[0]);
        return 2;
    }
    if (std::string(argv[1]) == "--fuzz") {
        int N = argc > 2 ? atoi(argv[2]) : 1000;
        int fails = 0;
        for (int c = 0; c < N; c++) {
            int S = rng_choice(1, 2500);
            // 全局有序的 existing 边 (顶点内 segment 按序有序是算法前提):
            // 生成一个排序池, 按连续切片分给各段, 每段 vc<=47, 随机留空段。
            // 容量: existing + new <= S*47 (pre-expand 保证)。
            int max_total = S * 47;
            int nidx = rng_choice(0, std::min(8192, max_total));
            int budget = rng_choice(0, max_total - nidx);
            std::vector<int> pool(budget);
            for (int i = 0; i < budget; i++) pool[i] = rng_choice(0, 500000);
            std::sort(pool.begin(), pool.end());
            std::vector<std::vector<int>> existing(S);
            int p = 0;
            for (int s = 0; s < S; s++) {
                int take = 0;
                if (p < (int)pool.size()) {
                    int rem = std::min(47, (int)pool.size() - p);
                    take = (rng_next() % 5 == 0) ? 0 : rng_choice(1, rem);  // ~20% 空段
                }
                existing[s].insert(existing[s].end(), pool.begin() + p, pool.begin() + p + take);
                p += take;
            }
            std::vector<int> new_edges(nidx);
            for (int k = 0; k < nidx; k++) new_edges[k] = rng_choice(0, 500000);
            std::sort(new_edges.begin(), new_edges.end());
            if (!run_case(existing, new_edges, c, true)) fails++;
        }
        printf("Fuzz: %d cases, %d failed\n", N, fails);
        return fails ? 1 : 0;
    }

    FILE* f = fopen(argv[1], "r");
    if (!f) { fprintf(stderr, "cannot open %s\n", argv[1]); return 2; }

    int cases = 0, fails = 0;
    while (true) {
        int S, nidx;
        if (fscanf(f, "%d %d", &S, &nidx) != 2) break;
        // 语料格式: 先 S 个 vc 值, 再各段边, 再 nidx 个 new_edges。
        std::vector<int> vc_arr(S);
        for (int s = 0; s < S; s++) fscanf(f, "%d", &vc_arr[s]);
        std::vector<std::vector<int>> existing(S);
        for (int s = 0; s < S; s++) {
            existing[s].resize(vc_arr[s]);
            for (int k = 0; k < vc_arr[s]; k++) fscanf(f, "%d", &existing[s][k]);
        }
        std::vector<int> new_edges(nidx);
        for (int k = 0; k < nidx; k++) fscanf(f, "%d", &new_edges[k]);
        cases++;
        if (!run_case(existing, new_edges, cases, true)) fails++;
    }
    fclose(f);
    printf("Corpus: %d cases, %d failed\n", cases, fails);
    return fails ? 1 : 0;
}
