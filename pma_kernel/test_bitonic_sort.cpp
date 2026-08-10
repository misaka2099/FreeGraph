// test_bitonic_sort.cpp - Test suite for bitonic_sort
// Compile: g++ -std=c++11 -DBITONIC_MAX_SIZE=256 -o test_bitonic_sort test_bitonic_sort.cpp
// Run:     ./test_bitonic_sort

#include <algorithm>
#include <random>
#include <cstdio>
#include <cstring>
#include <climits>
#include <cstdlib>

// === Bitonic Sort Implementation (self-contained, no HLS) ===

#ifndef BITONIC_MAX_SIZE
#define BITONIC_MAX_SIZE 256
#endif

struct INNER_TASK {
    int src;
    int dst;
    int opcode;
};

static bool operator<(const INNER_TASK& a, const INNER_TASK& b) {
    return a.src < b.src;
}

// Compare and swap two elements in direction dir.
// dir=true  -> ascending (smaller src first)
// dir=false -> descending (larger src first)
static void compare_swap(INNER_TASK tasks[], int i, int j, bool dir) {
    bool need_swap = (dir == true) ? (tasks[i].src > tasks[j].src)
                                   : (tasks[i].src < tasks[j].src);
    if (need_swap) {
        INNER_TASK tmp = tasks[i];
        tasks[i] = tasks[j];
        tasks[j] = tmp;
    }
}

// Bitonic merge: merge a bitonic sequence of 'count' elements starting at 'low'
// into a monotonic sequence in direction 'dir'.
static void bitonic_merge(INNER_TASK tasks[], int low, int count, bool dir) {
    if (count <= 1) return;
    int half = count / 2;
    for (int i = low; i < low + half; i++) {
        compare_swap(tasks, i, i + half, dir);
    }
    bitonic_merge(tasks, low, half, dir);
    bitonic_merge(tasks, low + half, half, dir);
}

// Bitonic sort: sort 'count' elements of tasks[] by src ascending.
// Elements at positions [count, BITONIC_MAX_SIZE) must already hold
// INT_MAX sentinel values (or this function will pad them).
void bitonic_sort(INNER_TASK tasks[], int count) {
    // Pad remaining slots with INT_MAX sentinels
    for (int i = count; i < BITONIC_MAX_SIZE; i++) {
        tasks[i].src = INT_MAX;
    }

    // Build bitonic sequence then merge
    // For each step size k = 2, 4, 8, ..., BITONIC_MAX_SIZE
    for (int k = 2; k <= BITONIC_MAX_SIZE; k *= 2) {
        // For each sub-sequence of size k, first half is ascending, second half descending
        for (int j = k / 2; j > 0; j /= 2) {
            for (int i = 0; i < BITONIC_MAX_SIZE; i++) {
                int ixj = i ^ j;
                if (ixj > i) {
                    // Determine direction: ascending for first half of each k-block
                    bool dir = ((i & k) == 0);  // true for first half of block
                    compare_swap(tasks, i, ixj, dir);
                }
            }
        }
    }
}


// === Test Framework ===

static int total_tests = 0;
static int passed_tests = 0;

// Check that the multiset of (dst, opcode) pairs is the same between
// sorted_result[lo..hi] and expected[lo..hi] (both have the same src value).
// We do this by counting occurrences of each (dst, opcode) pair.
static bool check_equal_src_group(const INNER_TASK* sorted_result,
                                  const INNER_TASK* expected,
                                  int lo, int hi) {
    // Use simple frequency counting since dst/opcode ranges are small
    static const int MAX_DST = 65536;
    static const int MAX_OPCODE = 16;
    // Stack-allocate a small frequency table; memset is fine here
    unsigned char freq_sorted[MAX_DST][MAX_OPCODE];
    unsigned char freq_expected[MAX_DST][MAX_OPCODE];
    memset(freq_sorted, 0, sizeof(freq_sorted));
    memset(freq_expected, 0, sizeof(freq_expected));

    for (int i = lo; i <= hi; i++) {
        int dst = sorted_result[i].dst;
        int op = sorted_result[i].opcode;
        if (dst >= 0 && dst < MAX_DST && op >= 0 && op < MAX_OPCODE) {
            freq_sorted[dst][op]++;
        }
        dst = expected[i].dst;
        op = expected[i].opcode;
        if (dst >= 0 && dst < MAX_DST && op >= 0 && op < MAX_OPCODE) {
            freq_expected[dst][op]++;
        }
    }
    return memcmp(freq_sorted, freq_expected, sizeof(freq_sorted)) == 0;
}

// Verify that bitonic_sort output matches expected:
//   1. src values are non-decreasing (ascending order)
//   2. For each group of equal src values, the multiset of (dst, opcode) matches
//   3. Padding positions count..BITONIC_MAX_SIZE-1 have src == INT_MAX
static void verify_match(const INNER_TASK* sorted, const INNER_TASK* expected,
                         int count, const char* test_name) {
    total_tests++;
    bool pass = true;

    // Check that src values are non-decreasing
    for (int i = 1; i < count; i++) {
        if (sorted[i].src < sorted[i-1].src) {
            if (pass) {
                printf("  FAIL [%s] src not in ascending order:\n", test_name);
            }
            pass = false;
            printf("    pos %d: src=%d < pos %d: src=%d\n",
                   i, sorted[i].src, i-1, sorted[i-1].src);
        }
    }

    // For each group of equal src values, verify multiset of (dst, opcode) matches
    int group_start = 0;
    while (group_start < count) {
        int group_end = group_start;
        while (group_end + 1 < count && sorted[group_end + 1].src == sorted[group_start].src) {
            group_end++;
        }

        if (!check_equal_src_group(sorted, expected, group_start, group_end)) {
            if (pass) {
                printf("  FAIL [%s] mismatch within equal-src group:\n", test_name);
            }
            pass = false;
            printf("    src=%d at positions %d..%d: (dst, opcode) multiset mismatch\n",
                   sorted[group_start].src, group_start, group_end);
            // Print details for the first few mismatches in this group
            int detail_count = 0;
            for (int i = group_start; i <= group_end && detail_count < 10; i++, detail_count++) {
                printf("      got pos %d: (src=%d, dst=%d, opcode=%d)\n",
                       i, sorted[i].src, sorted[i].dst, sorted[i].opcode);
            }
        }
        group_start = group_end + 1;
    }

    // Check padding positions count..BITONIC_MAX_SIZE-1 are INT_MAX
    for (int i = count; i < BITONIC_MAX_SIZE; i++) {
        if (sorted[i].src != INT_MAX) {
            if (pass) {
                printf("  FAIL [%s] padding corruption at position %d:\n", test_name, i);
            }
            pass = false;
            printf("    pos %d: src=%d (expected INT_MAX=%d)\n", i, sorted[i].src, INT_MAX);
        }
    }

    if (pass) {
        passed_tests++;
    }
}

// Run a single test: generate count random tasks, sort with bitonic_sort,
// compare against std::sort ground truth.
static void test_random(int count, std::mt19937& rng, const char* label) {
    // Generate random tasks
    INNER_TASK original[BITONIC_MAX_SIZE];
    INNER_TASK expected[BITONIC_MAX_SIZE];

    std::uniform_int_distribution<int> src_dist(0, 1000000);
    std::uniform_int_distribution<int> dst_dist(0, 65535);
    std::uniform_int_distribution<int> opcode_dist(0, 15);

    for (int i = 0; i < count; i++) {
        int src = src_dist(rng);
        original[i].src = src;
        original[i].dst = dst_dist(rng);
        original[i].opcode = opcode_dist(rng);
    }

    // Ground truth: copy and use std::sort
    std::copy(original, original + count, expected);
    std::sort(expected, expected + count,
              [](const INNER_TASK& a, const INNER_TASK& b) { return a.src < b.src; });

    // Run bitonic sort on original
    bitonic_sort(original, count);

    // Verify
    char test_name[128];
    snprintf(test_name, sizeof(test_name), "%s (count=%d)", label, count);
    verify_match(original, expected, count, test_name);
}

// Test with specific data (not random)
static void test_specific(INNER_TASK* input, int count, const char* label) {
    INNER_TASK copy[BITONIC_MAX_SIZE];
    std::copy(input, input + count, copy);
    std::sort(copy, copy + count,
              [](const INNER_TASK& a, const INNER_TASK& b) { return a.src < b.src; });

    bitonic_sort(input, count);

    char test_name[128];
    snprintf(test_name, sizeof(test_name), "%s (count=%d)", label, count);
    verify_match(input, copy, count, test_name);
}

// === Main ===

int main() {
    printf("=== Bitonic Sort Test Suite ===\n");
    printf("BITONIC_MAX_SIZE = %d\n\n", BITONIC_MAX_SIZE);

    std::mt19937 rng(42);  // fixed seed for reproducibility

    // --- Random tests for various sizes ---
    printf("--- Random Tests ---\n");
    for (int count : {1, 2, 3, 7, 15, 31, 50, 100, 128, 200, 255, 256}) {
        test_random(count, rng, "random");
    }

    // --- Edge case: single element ---
    printf("\n--- Edge Cases ---\n");
    {
        INNER_TASK data[1];
        data[0].src = 500; data[0].dst = 100; data[0].opcode = 7;
        test_specific(data, 1, "single element");
    }

    // --- Edge case: all same src value ---
    {
        INNER_TASK data[256];
        for (int i = 0; i < 256; i++) {
            data[i].src = 42;
            data[i].dst = i;
            data[i].opcode = i % 16;
        }
        test_specific(data, 256, "all same src");
    }

    // --- Edge case: already sorted ascending ---
    {
        INNER_TASK data[256];
        for (int i = 0; i < 256; i++) {
            data[i].src = i * 100;
            data[i].dst = i;
            data[i].opcode = i % 8;
        }
        test_specific(data, 256, "already sorted asc");
    }

    // --- Edge case: already sorted descending ---
    {
        INNER_TASK data[256];
        for (int i = 0; i < 256; i++) {
            data[i].src = (255 - i) * 100;
            data[i].dst = i;
            data[i].opcode = i % 8;
        }
        test_specific(data, 256, "already sorted desc");
    }

    // --- Edge case: reverse sorted (worst case) ---
    {
        INNER_TASK data[256];
        for (int i = 0; i < 256; i++) {
            data[i].src = 1000000 - i * 100;
            data[i].dst = i;
            data[i].opcode = i % 4;
        }
        test_specific(data, 256, "reverse sorted");
    }

    // --- Edge case: many duplicates ---
    {
        INNER_TASK data[256];
        int src_vals[] = {10, 20, 30, 40, 50};
        for (int i = 0; i < 256; i++) {
            data[i].src = src_vals[i % 5];
            data[i].dst = i;
            data[i].opcode = (i * 7) % 16;
        }
        test_specific(data, 256, "many duplicates (5 unique values)");
    }

    // --- Edge case: full range, sparse ---
    {
        INNER_TASK data[256];
        for (int i = 0; i < 256; i++) {
            data[i].src = i * 3906;  // roughly spread across 0..1M
            data[i].dst = 256 - i;
            data[i].opcode = i;
        }
        test_specific(data, 256, "spread across full range");
    }

    // --- Edge case: count = BITONIC_MAX_SIZE (already padded) ---
    {
        INNER_TASK data[256];
        for (int i = 0; i < 256; i++) {
            data[i].src = (i * 997) % 1000000;  // pseudo-random
            data[i].dst = i * 2;
            data[i].opcode = i ^ 0xAA;
        }
        test_specific(data, 256, "max size 256 random");
    }

    // --- Repeatability: same seed gives same result ---
    printf("\n--- Repeatability Check ---\n");
    {
        std::mt19937 rng_a(12345);
        std::mt19937 rng_b(12345);
        INNER_TASK data_a[256];
        INNER_TASK data_b[256];
        for (int i = 0; i < 100; i++) {
            int src = std::uniform_int_distribution<int>(0, 1000000)(rng_a);
            data_a[i].src = src; data_a[i].dst = 0; data_a[i].opcode = 0;
            // same for rng_b
            src = std::uniform_int_distribution<int>(0, 1000000)(rng_b);
            data_b[i].src = src; data_b[i].dst = 0; data_b[i].opcode = 0;
        }
        bitonic_sort(data_a, 100);
        bitonic_sort(data_b, 100);
        bool match = true;
        for (int i = 0; i < BITONIC_MAX_SIZE; i++) {
            if (data_a[i].src != data_b[i].src) {
                match = false;
                break;
            }
        }
        total_tests++;
        if (match) {
            printf("  PASS [repeatability] same seed produces identical result\n");
            passed_tests++;
        } else {
            printf("  FAIL [repeatability] same seed produced different results\n");
        }
    }

    // === Summary ===
    printf("\n=== Results: %d / %d tests passed ===\n", passed_tests, total_tests);
    return (passed_tests == total_tests) ? 0 : 1;
}
