// Testbench for the full wave sort pipeline:
//   radix_sort_src (LSD radix, 24-bit src, stable) + per-segment
//   bitonic_segment_by_dst (dynamic max_size ≤ 256, sorted by dst).
// Verifies:
//   1. sorted: src non-decreasing after radix
//   2. stable: equal src keep their original relative order (radix)
//   3. complete: multiset of (src,dst,opcode) unchanged (no loss/dup)
//   4. full sort: within each src group (segment), dst non-decreasing
//      (segments ≤ 256 and > 8 are bitonic-sorted; others left as-is)
// Usage: g++ -O2 -I ../pma_kernel test_radix_sort.cpp -o test_radix_sort && ./test_radix_sort

#include "radix_sort.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
#include <map>
#include <tuple>

static bool check_sorted(int* buf, int n) {
    for (int i = 1; i < n; i++) {
        if (buf[i * 3] < buf[(i - 1) * 3]) return false;
    }
    return true;
}

// Run the same segment-scan pipeline as the kernel: after radix, split by src
// and bitonic-sort segments that qualify.
static void full_sort(int* buf, int* tmp, int n) {
    radix_sort_src(buf, tmp, n);
    int i = 0;
    while (i < n) {
        int src = buf[i * 3];
        int j = i + 1;
        while (j < n && buf[j * 3] == src) j++;
        if (j - i <= SEG_BITONIC_MAX && j - i > 8) {
            bitonic_segment_by_dst(buf, tmp, i, j);
        }
        i = j;
    }
}

// check: within each src group, dst non-decreasing (only if segment was sorted)
static bool check_dst_sorted(int* buf, int n) {
    int i = 0;
    while (i < n) {
        int src = buf[i * 3];
        int j = i + 1;
        while (j < n && buf[j * 3] == src) j++;
        int seg = j - i;
        if (seg <= SEG_BITONIC_MAX && seg > 8) {
            for (int k = i + 1; k < j; k++) {
                if (buf[k * 3 + 1] < buf[(k - 1) * 3 + 1]) return false;
            }
        }
        i = j;
    }
    return true;
}

int main() {
    srand(12345);
    int failures = 0;
    const int NTESTS = 200;

    for (int t = 0; t < NTESTS; t++) {
        int n = rand() % 5000 + 1;
        // src range: mix small (many duplicates) and large (24-bit limit)
        int src_range = (t % 3 == 0) ? 64 : (t % 3 == 1 ? 4096 : (1 << 24));

        // ---- generate ----
        std::vector<int> orig(3 * n);
        for (int i = 0; i < n; i++) {
            orig[3 * i + 0] = rand() % src_range;        // src
            orig[3 * i + 1] = rand() % 1000;             // dst
            orig[3 * i + 2] = rand() % 8;                // opcode
        }

        // ---- run full sort: radix(src) + per-segment bitonic(dst) ----
        std::vector<int> a = orig, b(3 * n, -1);
        full_sort(a.data(), b.data(), n);                // result in a

        // ---- check 1: sorted by src ----
        if (!check_sorted(a.data(), n)) {
            printf("TEST %d FAIL: not sorted (n=%d range=%d)\n", t, n, src_range);
            for (int i = 0; i < n; i++) printf("  [%d] src=%d\n", i, a[3 * i]);
            failures++;
            break;
        }

        // ---- check 1b: within each src group, dst non-decreasing ----
        // (only for segments that qualify for bitonic: >8 and <=256)
        if (!check_dst_sorted(a.data(), n)) {
            printf("TEST %d FAIL: dst not sorted within segment (n=%d range=%d)\n",
                   t, n, src_range);
            failures++;
            break;
        }

        // ---- check 3: multiset unchanged ----
        {
            std::map<std::tuple<int,int,int>, int> before, after;
            for (int i = 0; i < n; i++) {
                before[{orig[3*i+0], orig[3*i+1], orig[3*i+2]}]++;
                after[{a[3*i+0], a[3*i+1], a[3*i+2]}]++;
            }
            if (before != after) {
                printf("TEST %d FAIL: multiset changed (n=%d)\n", t, n);
                failures++;
                break;
            }
        }
    }

    // ---- radix-only stability tests (no segment bitonic: it intentionally
    //      reorders equal-src by dst, so stability is a radix-only property) ----
    for (int t = 0; t < 100; t++) {
        int n = rand() % 2000 + 1;
        int src_range = (t % 2 == 0) ? 64 : 4096;    // many duplicates
        std::vector<int> orig(3 * n);
        for (int i = 0; i < n; i++) {
            orig[3 * i + 0] = rand() % src_range;
            orig[3 * i + 1] = rand() % 1000;
            orig[3 * i + 2] = rand() % 8;
        }
        std::vector<int> a = orig, b(3 * n, -1);
        radix_sort_src(a.data(), b.data(), n);

        if (!check_sorted(a.data(), n)) {
            printf("STAB TEST %d FAIL: not sorted\n", t);
            failures++;
            break;
        }
        // stability: equal src keep original (dst,opcode) sequence
        std::map<int, std::vector<std::pair<int,int>>> seq;
        for (int i = 0; i < n; i++) {
            int s = orig[3 * i + 0];
            seq[s].push_back({orig[3 * i + 1], orig[3 * i + 2]});
        }
        std::map<int, size_t> pos;
        bool bad = false;
        for (int i = 0; i < n && !bad; i++) {
            int s = a[3 * i + 0];
            size_t p = pos[s]++;
            if (p >= seq[s].size() ||
                seq[s][p].first != a[3 * i + 1] ||
                seq[s][p].second != a[3 * i + 2]) {
                printf("STAB TEST %d FAIL: unstable at %d src=%d\n", t, i, s);
                bad = true;
                failures++;
            }
        }
        if (failures) break;
    }

    // ---- edge cases ----
    {
        // n = 0
        int a0[3] = {0}, b0[3] = {0};
        radix_sort_src(a0, b0, 0);
        // n = 1
        int a1[3] = {42, 7, 1}, b1[3] = {0,0,0};
        radix_sort_src(a1, b1, 1);
        if (a1[0] != 42 || a1[1] != 7 || a1[2] != 1) {
            printf("EDGE FAIL: n=1 changed\n");
            failures++;
        }
        // already sorted
        int a2[9] = {1,5,2, 2,8,1, 3,9,0}, b2[9] = {0};
        radix_sort_src(a2, b2, 3);
        if (!check_sorted(a2, 3)) { printf("EDGE FAIL: already-sorted\n"); failures++; }
        // reverse order
        int a3[9] = {3,9,0, 2,8,1, 1,5,2}, b3[9] = {0};
        radix_sort_src(a3, b3, 3);
        if (!check_sorted(a3, 3)) { printf("EDGE FAIL: reverse\n"); failures++; }
        // all equal src → stable
        int a4[9] = {5,3,0, 5,1,2, 5,2,1}, b4[9] = {0};
        radix_sort_src(a4, b4, 3);
        if (!(a4[1]==3 && a4[4]==1 && a4[7]==2)) {
            printf("EDGE FAIL: all-equal stability (got %d,%d,%d)\n", a4[1], a4[4], a4[7]);
            failures++;
        }
    }

    if (failures == 0) {
        printf("ALL %d FULL-SORT TESTS + 100 RADIX-STABILITY TESTS + EDGE CASES PASSED\n", NTESTS);
        return 0;
    }
    printf("FAILURES: %d\n", failures);
    return 1;
}
