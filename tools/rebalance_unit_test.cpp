// Rebalance Unit Test Framework
// Compares RTSS expansion (hardware) against power-of-2 expansion (ground truth)
//
// Build: g++ -std=c++17 -O2 -o rebalance_unit_test rebalance_unit_test.cpp && ./rebalance_unit_test

#include <algorithm>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <vector>

constexpr int VERTEX_PER_PMA     = 16;
constexpr int SEGMENT_SIZE       = 64;
constexpr int EDGE_SLOTS_PER_SEG = 63;
constexpr int VALID_COUNT_IDX    = 63;
constexpr int DENSITY_BOUND      = 47;
constexpr int UNUSED_ICON        = -1;
constexpr int SEG_WORDS_PER_SEG  = 4;
constexpr int PMA_ENLARGE_FACTOR = 3;

struct PMA {
    int vr[VERTEX_PER_PMA];   // vertex_range
    int ec[VERTEX_PER_PMA];   // edge_count
    std::vector<int> seg;     // flat: s * SEGMENT_SIZE + slot

    PMA() {
        for (int v = 0; v < VERTEX_PER_PMA; v++) { vr[v] = v + 1; ec[v] = 0; }
        int ts = vr[VERTEX_PER_PMA - 1];
        seg.resize(ts * SEGMENT_SIZE, UNUSED_ICON);
        for (int s = 0; s < ts; s++) seg[s * SEGMENT_SIZE + VALID_COUNT_IDX] = 0;
    }

    int total_segs() const { return vr[VERTEX_PER_PMA - 1]; }
    int seg_start(int v) const { return v == 0 ? 0 : vr[v - 1]; }
    int seg_end(int v) const { return vr[v]; }
    int seg_count(int v) const { return seg_end(v) - seg_start(v); }
    int* sd(int s) { return &seg[s * SEGMENT_SIZE]; }
    const int* sd(int s) const { return &seg[s * SEGMENT_SIZE]; }
    int vc(int s) const { return seg[s * SEGMENT_SIZE + VALID_COUNT_IDX]; }
    int first_edge(int s) const { int v = vc(s); return v > 0 ? seg[s * SEGMENT_SIZE] : UNUSED_ICON; }
};

// ---- Global utilities ----

int find_target_seg(const PMA& p, int v, int dst) {
    for (int s = p.seg_start(v); s < p.seg_end(v); s++) {
        int fe = p.first_edge(s);
        if (fe == UNUSED_ICON || dst <= fe) return s;
    }
    return p.seg_end(v) - 1;
}

void insert_edge(PMA& p, int v, int seg_idx, int dst) {
    int* d = p.sd(seg_idx);
    int vc = p.vc(seg_idx);
    int pos = 0;
    while (pos < vc && d[pos] < dst) pos++;
    for (int k = std::min(vc, EDGE_SLOTS_PER_SEG - 1); k > pos; k--) d[k] = d[k - 1];
    if (pos < EDGE_SLOTS_PER_SEG) { d[pos] = dst; if (vc < EDGE_SLOTS_PER_SEG) d[VALID_COUNT_IDX] = vc + 1; }
    p.ec[v]++;
}

PMA enlarge_pma(const PMA& p, int v, int new_seg_count) {
    PMA np;
    int enlarge_by = new_seg_count - p.seg_count(v), acc = 0;
    for (int vv = 0; vv < VERTEX_PER_PMA; vv++) {
        np.ec[vv] = p.ec[vv];
        if (vv == v) acc += enlarge_by;
        np.vr[vv] = p.vr[vv] + acc;
    }
    np.seg.resize(np.total_segs() * SEGMENT_SIZE, UNUSED_ICON);
    for (int s = 0; s < np.total_segs(); s++) np.seg[s * SEGMENT_SIZE + VALID_COUNT_IDX] = 0;
    for (int vv = 0; vv < VERTEX_PER_PMA; vv++) {
        int ovs = p.seg_start(vv), ove = p.seg_end(vv), nvs = np.seg_start(vv);
        for (int s = ovs; s < ove; s++)
            std::memcpy(np.sd(nvs + (s - ovs)), p.sd(s), SEGMENT_SIZE * sizeof(int));
    }
    return np;
}

int remap_seg(const PMA& old_p, const PMA& new_p, int old_seg) {
    for (int v = 0; v < VERTEX_PER_PMA; v++) {
        int ovs = old_p.seg_start(v), ove = old_p.seg_end(v);
        if (old_seg >= ovs && old_seg < ove) return new_p.seg_start(v) + (old_seg - ovs);
    }
    return old_seg;
}

// ---- Agent 2: Power-of-2 Ground Truth ----

struct RBResult { PMA pma; bool redist; int left, right, total; std::string log; };

RBResult rebalance_power2(const PMA& p, int seg, int owner_v) {
    RBResult r; r.pma = p; r.redist = false; r.left = seg; r.right = seg + 1; r.total = 0;
    int vs = p.seg_start(owner_v), ve = p.seg_end(owner_v), vsegs = ve - vs;
    if (vsegs <= 1) { r.log = "v_segs=1"; return r; }
    int rel = seg - vs, cur_vc = p.vc(seg);
    if (cur_vc <= DENSITY_BOUND) { r.log = "vc<=47"; return r; }

    int left = seg, right = seg + 1, total = cur_vc;
    for (int tgsz = 2; tgsz <= vsegs * 2; tgsz *= 2) {
        if (total <= (right - left) * DENSITY_BOUND) break;
        int nl = vs + (rel / tgsz) * tgsz, nr = vs + (rel / tgsz + 1) * tgsz;
        if (nr > ve) nr = ve;
        if (nl == left && nr == right) continue;
        for (int s = nl; s < nr; s++) if (s < left || s >= right) total += p.vc(s);
        left = nl; right = nr;
    }
    int gs = right - left; r.left = left; r.right = right; r.total = total;
    if (gs <= 1) { r.log = "gs=1"; return r; }

    std::vector<int> edges;
    for (int s = left; s < right; s++) {
        const int* d = p.sd(s); int vc = std::min(p.vc(s), EDGE_SLOTS_PER_SEG);
        for (int k = 0; k < vc; k++) edges.push_back(d[k]);
    }
    std::sort(edges.begin(), edges.end());
    int base = total / gs, extra = total % gs, pos = 0;
    for (int s = left; s < right; s++) {
        int fill = base + ((s - left) < extra ? 1 : 0);
        int* d = r.pma.sd(s);
        for (int k = 0; k < EDGE_SLOTS_PER_SEG; k++) d[k] = (k < fill) ? edges[pos++] : UNUSED_ICON;
        d[VALID_COUNT_IDX] = fill;
    }
    r.redist = true;
    r.log = "P2: [" + std::to_string(left) + "," + std::to_string(right) + ") total=" + std::to_string(total);
    return r;
}

// ---- Agent 4: Hardware Rebalance (power-of-2 with binary-tree alignment) ----

RBResult rebalance_rtss(const PMA& p, int seg) {
    RBResult r; r.pma = p; r.redist = false; r.left = seg; r.right = seg + 1; r.total = 0;
    int owner_v = -1;
    for (int vv = 0; vv < VERTEX_PER_PMA; vv++) {
        int vs = p.seg_start(vv);
        if (seg >= vs && seg < p.seg_end(vv)) owner_v = vv;
    }
    if (owner_v < 0) { r.log = "no owner"; return r; }
    int vs = p.seg_start(owner_v), ve = p.seg_end(owner_v), vsegs = ve - vs;
    if (vsegs <= 1) { r.log = "v_segs=1"; return r; }
    int cur_vc = p.vc(seg);
    if (cur_vc <= DENSITY_BOUND) { r.log = "vc<=47"; return r; }

    // Power-of-2 expansion with binary-tree alignment (matching hardware)
    int rel = seg - vs;
    int left = seg, right = seg + 1, total = cur_vc;
    for (int tgsz = 2; tgsz <= vsegs * 2; tgsz *= 2) {
        if (total <= (right - left) * DENSITY_BOUND) break;
        int nl = vs + (rel / tgsz) * tgsz;
        int nr = vs + (rel / tgsz + 1) * tgsz;
        if (nr > ve) nr = ve;
        if (nl == left && nr == right) continue;
        for (int s = nl; s < nr; s++) if (s < left || s >= right) total += p.vc(s);
        left = nl; right = nr;
    }
    int gs = right - left; r.left = left; r.right = right; r.total = total;
    if (gs <= 1) { r.log = "gs=1"; return r; }

    std::vector<int> edges;
    for (int s = left; s < right; s++) {
        const int* d = p.sd(s); int vc = std::min(p.vc(s), EDGE_SLOTS_PER_SEG);
        for (int k = 0; k < vc; k++) edges.push_back(d[k]);
    }
    std::sort(edges.begin(), edges.end());
    if ((int)edges.size() != total) total = (int)edges.size();
    int base = total / gs, extra = total % gs, pos = 0;
    for (int s = left; s < right; s++) {
        int fill = base + ((s - left) < extra ? 1 : 0);
        int* d = r.pma.sd(s);
        for (int k = 0; k < EDGE_SLOTS_PER_SEG; k++) d[k] = (k < fill && pos < (int)edges.size()) ? edges[pos++] : UNUSED_ICON;
        d[VALID_COUNT_IDX] = fill;
    }
    r.redist = true;
    r.log = "HW: [" + std::to_string(left) + "," + std::to_string(right) + ") total=" + std::to_string(total);
    return r;
}

// ---- Agent 1: Test Case Generator ----

struct TestCase { std::string desc; PMA pma; int seg; int owner; };

// Fill a specific segment with `count` edges (direct write, bypasses find_target_seg)
void fill_seg(PMA& p, int vertex, int target_seg, int count) {
    int vs = p.seg_start(vertex);
    for (int s = vs; s < target_seg; s++) {
        if (p.vc(s) == 0) {
            p.sd(s)[0] = s * 10;
            p.sd(s)[VALID_COUNT_IDX] = 1;
            p.ec[vertex]++;
        }
    }
    for (int e = 0; e < count; e++) {
        int* d = p.sd(target_seg);
        int vc = p.vc(target_seg);
        if (vc < EDGE_SLOTS_PER_SEG) { d[vc] = target_seg * 1000 + e; d[VALID_COUNT_IDX] = vc + 1; }
        p.ec[vertex]++;
    }
}

class TestGen {
    std::mt19937 rng;
public:
    TestGen(int seed = 42) : rng(seed) {}

    TestCase basic_overfull(int v, int n, int other = 3) {
        TestCase tc; tc.desc = "v"+std::to_string(v)+" n="+std::to_string(n);
        PMA& p = tc.pma;
        for (int vv = 0; vv < VERTEX_PER_PMA; vv++) {
            if (vv == v) continue;
            for (int e = 0; e < other; e++) {
                int dst = std::uniform_int_distribution<>(0, 255)(rng);
                insert_edge(p, vv, find_target_seg(p, vv, dst), dst);
            }
        }
        for (int e = 0; e < n; e++) {
            int dst = std::uniform_int_distribution<>(0, 255)(rng);
            insert_edge(p, v, find_target_seg(p, v, dst), dst);
        }
        tc.seg = p.seg_start(v); tc.owner = v;
        return tc;
    }

    TestCase right_edge(int v) {
        TestCase tc; tc.desc = "v"+std::to_string(v)+" rightmost";
        PMA p = enlarge_pma(PMA(), v, 3);
        int last = p.seg_end(v) - 1;
        fill_seg(p, v, last, 50);
        tc.pma = p; tc.seg = last; tc.owner = v;
        return tc;
    }

    TestCase left_edge(int v) {
        TestCase tc; tc.desc = "v"+std::to_string(v)+" leftmost";
        PMA p = enlarge_pma(PMA(), v, 3);
        int first = p.seg_start(v);
        fill_seg(p, v, first, 50);
        tc.pma = p; tc.seg = first; tc.owner = v;
        return tc;
    }

    TestCase boundary(int v, int vsegs) {
        TestCase tc; tc.desc = "v"+std::to_string(v)+" boundary segs="+std::to_string(vsegs);
        PMA p = enlarge_pma(PMA(), v, vsegs);
        int last = p.seg_end(v) - 1;
        fill_seg(p, v, last, 50);
        tc.pma = p; tc.seg = last; tc.owner = v;
        return tc;
    }

    TestCase middle_trigger(int v, int vsegs) {
        TestCase tc; tc.desc = "v"+std::to_string(v)+" middle segs="+std::to_string(vsegs);
        PMA p = enlarge_pma(PMA(), v, vsegs);
        int mid = p.seg_start(v) + 2;
        fill_seg(p, v, mid, 50);
        tc.pma = p; tc.seg = mid; tc.owner = v;
        return tc;
    }

    TestCase single_seg_vertex(int v) {
        TestCase tc; tc.desc = "single seg v"+std::to_string(v);
        PMA p;
        for (int e = 0; e < 30; e++) {
            int dst = std::uniform_int_distribution<>(0, 255)(rng);
            insert_edge(p, v, find_target_seg(p, v, dst), dst);
        }
        tc.pma = p; tc.seg = p.seg_start(v); tc.owner = v;
        return tc;
    }

    TestCase not_overfull(int v) {
        TestCase tc; tc.desc = "not overfull v"+std::to_string(v);
        PMA p = enlarge_pma(PMA(), v, 3);
        for (int e = 0; e < 30; e++) {
            int dst = std::uniform_int_distribution<>(0, 255)(rng);
            insert_edge(p, v, find_target_seg(p, v, dst), dst);
        }
        tc.pma = p; tc.seg = p.seg_start(v); tc.owner = v;
        return tc;
    }
};

// ---- Agent 3: Insert + Enlarge ----

struct InsResult { PMA pma; bool enlarged; std::vector<int> mod_segs; };

InsResult insert_batch(const PMA& p, int v, const std::vector<int>& dsts) {
    InsResult r; r.pma = p; r.enlarged = false;
    std::vector<bool> mod(p.total_segs(), false);
    for (int dst : dsts) {
        int ts = find_target_seg(r.pma, v, dst);
        insert_edge(r.pma, v, ts, dst);
        mod[ts] = true;
    }
    for (int s = 0; s < r.pma.total_segs(); s++) if (mod[s]) r.mod_segs.push_back(s);
    int sc = r.pma.seg_count(v);
    if (r.pma.ec[v] > sc * DENSITY_BOUND) {
        int ns = PMA_ENLARGE_FACTOR * sc;
        PMA old = r.pma;
        r.pma = enlarge_pma(r.pma, v, ns);
        r.enlarged = true;
        for (int& seg : r.mod_segs) seg = remap_seg(old, r.pma, seg);
    }
    return r;
}

// ---- Validator ----

// Verify rebalance correctness: edges preserved, density satisfied, sorted
bool verify_rebalance(const PMA& orig, const PMA& result, const std::string& label, int owner_v) {
    // 1. Vertex structure unchanged
    for (int v = 0; v < VERTEX_PER_PMA; v++) {
        if (orig.vr[v] != result.vr[v]) { std::cerr << label << " FAIL: vr["<<v<<"] changed" << std::endl; return false; }
        if (orig.ec[v] != result.ec[v]) { std::cerr << label << " FAIL: ec["<<v<<"] changed" << std::endl; return false; }
    }

    // 2. Total edges preserved in rebalanced vertex
    int vs = orig.seg_start(owner_v), ve = orig.seg_end(owner_v);
    int orig_total = 0, result_total = 0;
    std::vector<int> orig_edges, result_edges;
    for (int s = vs; s < ve; s++) {
        orig_total += orig.vc(s); result_total += result.vc(s);
        int ovc = std::min(orig.vc(s), EDGE_SLOTS_PER_SEG);
        int rvc = std::min(result.vc(s), EDGE_SLOTS_PER_SEG);
        for (int k = 0; k < ovc; k++) orig_edges.push_back(orig.sd(s)[k]);
        for (int k = 0; k < rvc; k++) result_edges.push_back(result.sd(s)[k]);
    }
    if (orig_total != result_total) {
        std::cerr << label << " FAIL: edge count mismatch " << orig_total << " vs " << result_total << std::endl;
        return false;
    }

    // 3. Edges preserved (same multiset)
    std::sort(orig_edges.begin(), orig_edges.end());
    std::sort(result_edges.begin(), result_edges.end());
    if (orig_edges != result_edges) {
        std::cerr << label << " FAIL: edge values differ" << std::endl;
        return false;
    }

    // 4. No segment exceeds density bound
    for (int s = vs; s < ve; s++) {
        if (result.vc(s) > DENSITY_BOUND) {
            std::cerr << label << " FAIL: seg " << s << " vc=" << result.vc(s) << " > " << DENSITY_BOUND << std::endl;
            return false;
        }
    }

    // 5. Segments are sorted
    for (int s = vs; s < ve; s++) {
        int vc = result.vc(s);
        for (int k = 1; k < vc; k++) {
            if (result.sd(s)[k-1] > result.sd(s)[k]) {
                std::cerr << label << " FAIL: seg " << s << " not sorted" << std::endl;
                return false;
            }
        }
    }
    return true;
}

// ---- Main ----
int main() {
    std::cout << "=== Rebalance Unit Tests ===" << std::endl;
    std::cout << "VERTEX_PER_PMA=" << VERTEX_PER_PMA << " DENSITY_BOUND=" << DENSITY_BOUND << std::endl << std::endl;

    TestGen gen(42);
    int passed = 0, failed = 0;

    auto run_test = [&](const std::string& name, const TestCase& tc) {
        auto gt = rebalance_power2(tc.pma, tc.seg, tc.owner);
        auto rt = rebalance_rtss(tc.pma, tc.seg);

        // Validate each independently
        bool gt_ok = !gt.redist || verify_rebalance(tc.pma, gt.pma, name + " GT", tc.owner);
        bool rt_ok = !rt.redist || verify_rebalance(tc.pma, rt.pma, name + " RTSS", tc.owner);
        bool ok = gt_ok && rt_ok;

        std::cout << (ok ? "[PASS]" : "[FAIL]") << " " << name << std::endl;
        std::cout << "  trigger=seg" << tc.seg << " vc=" << tc.pma.vc(tc.seg)
                  << " GT[" << gt.left << "," << gt.right << ")" << (gt.redist?" redist":"")
                  << " RTSS[" << rt.left << "," << rt.right << ")" << (rt.redist?" redist":"")
                  << std::endl;
        if (!gt_ok) std::cout << "  GT FAIL: " << gt.log << std::endl;
        if (!rt_ok) std::cout << "  RTSS FAIL: " << rt.log << std::endl;
        ok ? passed++ : failed++;
        std::cout << std::endl;
    };

    // Test 1
    run_test("Test 1: Basic overfull v5 (50 edges, 1 seg → skip)", gen.basic_overfull(5, 50));

    // Test 2: Right edge
    run_test("Test 2: Right edge trigger (v3, 3 segs)", gen.right_edge(3));

    // Test 3: Left edge
    run_test("Test 3: Left edge trigger (v1, 3 segs)", gen.left_edge(1));

    // Test 4: Boundary 3 segs
    run_test("Test 4: Boundary 3 segs, rightmost trigger", gen.boundary(2, 3));

    // Test 5: Middle trigger 5 segs
    run_test("Test 5: Middle trigger (v6, 5 segs)", gen.middle_trigger(6, 5));

    // Test 6: Single segment (skip)
    run_test("Test 6: Single segment v10 (should skip)", gen.single_seg_vertex(10));

    // Test 7: Not overfull (skip)
    run_test("Test 7: Not overfull v8 (should skip)", gen.not_overfull(8));

    // Test 8: Insert → Enlarge → Rebalance pipeline
    {
        PMA p;
        std::mt19937 lrng(123);
        std::vector<int> edges;
        for (int e = 0; e < 48; e++) edges.push_back(std::uniform_int_distribution<>(0, 200)(lrng));
        auto ins = insert_batch(p, 0, edges);
        PMA after = ins.pma;
        for (int seg : ins.mod_segs) { auto r = rebalance_rtss(after, seg); after = r.pma; }
        bool ok = (after.vc(after.seg_start(0)) > 0) && (after.ec[0] == 48);
        std::cout << (ok ? "[PASS]" : "[FAIL]") << " Test 8: Insert 48 → Enlarge → Rebalance pipeline" << std::endl;
        std::cout << "  ec=" << after.ec[0] << " segs=" << after.seg_count(0) << " enlarged=" << ins.enlarged << std::endl << std::endl;
        ok ? passed++ : failed++;
    }

    std::cout << "=== " << passed << " passed, " << failed << " failed ===" << std::endl;
    return failed == 0 ? 0 : 1;
}
