// Standalone test: Batcher odd-even mergesort for PMA_UPDATE_MESSAGE
// Compile: g++ -std=c++11 -O2 test_sort.cpp -o test_sort && ./test_sort

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <algorithm>

// Mirror data_type.h
#define MAX_CMD_NUM 16

enum PMA_UPDATE_OPCODE {
    INSERT_EDGE = 0,
    DELETE_EDGE = 1,
    INSERT_VERTEX = 2,
    DELETE_VERTEX = 3,
    STOP = 4,
    FLUSH_CACHE = 5
};

struct PMA_UPDATE_MESSAGE {
    int src;
    int dst;
    PMA_UPDATE_OPCODE opcode;
    int last;
};

// ---- Comparison: a > b in (src, dst, opcode) order ----
static inline bool msg_gt(const PMA_UPDATE_MESSAGE &a, const PMA_UPDATE_MESSAGE &b) {
    if (a.src != b.src) return a.src > b.src;
    if (a.dst != b.dst) return a.dst > b.dst;
    return (int)a.opcode > (int)b.opcode;
}

// ---- Compare-exchange: if a > b, swap ----
static inline void cmp_swap(PMA_UPDATE_MESSAGE &a, PMA_UPDATE_MESSAGE &b) {
    if (msg_gt(a, b)) {
        PMA_UPDATE_MESSAGE t = a; a = b; b = t;
    }
}

// ---- Ref: original insertion sort ----
static void insertion_sort(PMA_UPDATE_MESSAGE arr[], int n) {
    for (int i = 1; i < n; i++) {
        PMA_UPDATE_MESSAGE key = arr[i];
        int j = i - 1;
        while (j >= 0 && msg_gt(arr[j], key)) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

// ---- Batcher odd-even mergesort for n=16 (standard iterative algorithm) ----
static void batcher_sort16(PMA_UPDATE_MESSAGE arr[16]) {
    int n = 16;
    for (int p = 1; p < n; p <<= 1) {
        for (int k = p; k >= 1; k >>= 1) {
            for (int j = k % p; j + k < n; j += 2 * k) {
                for (int i = 0; i < k; i++) {
                    int a = i + j;
                    int b = a + k;
                    if (a / (2 * p) == b / (2 * p)) {
                        cmp_swap(arr[a], arr[b]);
                    }
                }
            }
        }
    }
}

// ---- HLS-ready: fully unrolled Batcher network for 16 elements ----
// arr[16] must be register-mapped (#pragma HLS ARRAY_PARTITION complete)
// Non-active positions (i >= n) must be pre-filled with MAX sentinel.
static void batcher_sort16_unrolled(PMA_UPDATE_MESSAGE arr[16]) {
    // p=1 k=1
    cmp_swap(arr[ 0], arr[ 1]);
    cmp_swap(arr[ 2], arr[ 3]);
    cmp_swap(arr[ 4], arr[ 5]);
    cmp_swap(arr[ 6], arr[ 7]);
    cmp_swap(arr[ 8], arr[ 9]);
    cmp_swap(arr[10], arr[11]);
    cmp_swap(arr[12], arr[13]);
    cmp_swap(arr[14], arr[15]);
    // p=2 k=2
    cmp_swap(arr[ 0], arr[ 2]);
    cmp_swap(arr[ 1], arr[ 3]);
    cmp_swap(arr[ 4], arr[ 6]);
    cmp_swap(arr[ 5], arr[ 7]);
    cmp_swap(arr[ 8], arr[10]);
    cmp_swap(arr[ 9], arr[11]);
    cmp_swap(arr[12], arr[14]);
    cmp_swap(arr[13], arr[15]);
    // p=2 k=1
    cmp_swap(arr[ 1], arr[ 2]);
    cmp_swap(arr[ 5], arr[ 6]);
    cmp_swap(arr[ 9], arr[10]);
    cmp_swap(arr[13], arr[14]);
    // p=4 k=4
    cmp_swap(arr[ 0], arr[ 4]);
    cmp_swap(arr[ 1], arr[ 5]);
    cmp_swap(arr[ 2], arr[ 6]);
    cmp_swap(arr[ 3], arr[ 7]);
    cmp_swap(arr[ 8], arr[12]);
    cmp_swap(arr[ 9], arr[13]);
    cmp_swap(arr[10], arr[14]);
    cmp_swap(arr[11], arr[15]);
    // p=4 k=2
    cmp_swap(arr[ 2], arr[ 4]);
    cmp_swap(arr[ 3], arr[ 5]);
    cmp_swap(arr[10], arr[12]);
    cmp_swap(arr[11], arr[13]);
    // p=4 k=1
    cmp_swap(arr[ 1], arr[ 2]);
    cmp_swap(arr[ 3], arr[ 4]);
    cmp_swap(arr[ 5], arr[ 6]);
    cmp_swap(arr[ 9], arr[10]);
    cmp_swap(arr[11], arr[12]);
    cmp_swap(arr[13], arr[14]);
    // p=8 k=8
    cmp_swap(arr[ 0], arr[ 8]);
    cmp_swap(arr[ 1], arr[ 9]);
    cmp_swap(arr[ 2], arr[10]);
    cmp_swap(arr[ 3], arr[11]);
    cmp_swap(arr[ 4], arr[12]);
    cmp_swap(arr[ 5], arr[13]);
    cmp_swap(arr[ 6], arr[14]);
    cmp_swap(arr[ 7], arr[15]);
    // p=8 k=4
    cmp_swap(arr[ 4], arr[ 8]);
    cmp_swap(arr[ 5], arr[ 9]);
    cmp_swap(arr[ 6], arr[10]);
    cmp_swap(arr[ 7], arr[11]);
    // p=8 k=2
    cmp_swap(arr[ 2], arr[ 4]);
    cmp_swap(arr[ 3], arr[ 5]);
    cmp_swap(arr[ 6], arr[ 8]);
    cmp_swap(arr[ 7], arr[ 9]);
    cmp_swap(arr[10], arr[12]);
    cmp_swap(arr[11], arr[13]);
    // p=8 k=1
    cmp_swap(arr[ 1], arr[ 2]);
    cmp_swap(arr[ 3], arr[ 4]);
    cmp_swap(arr[ 5], arr[ 6]);
    cmp_swap(arr[ 7], arr[ 8]);
    cmp_swap(arr[ 9], arr[10]);
    cmp_swap(arr[11], arr[12]);
    cmp_swap(arr[13], arr[14]);
}

// ---- Generic Batcher mergesort for n <= 16 (pad + unrolled network) ----
static void batcher_sort(PMA_UPDATE_MESSAGE arr[], int n) {
    PMA_UPDATE_MESSAGE pad[16];
    for (int i = 0; i < n; i++) pad[i] = arr[i];
    // Pad with MAX sentinel so they sink to the end
    for (int i = n; i < 16; i++) {
        pad[i].src = 0x7FFFFFFF;
        pad[i].dst = 0x7FFFFFFF;
        pad[i].opcode = (PMA_UPDATE_OPCODE)0x7FFFFFFF;
        pad[i].last = 0;
    }
    batcher_sort16_unrolled(pad);
    for (int i = 0; i < n; i++) arr[i] = pad[i];
}

// ---- Test: verify sorting correctness against reference ----
static bool test_one(int n, int seed) {
    PMA_UPDATE_MESSAGE arr1[16], arr2[16];
    srand(seed);
    for (int i = 0; i < n; i++) {
        arr1[i].src = rand() % 256;
        arr1[i].dst = rand() % 256;
        arr1[i].opcode = (rand() & 1) ? DELETE_EDGE : INSERT_EDGE;
        arr1[i].last = (i == n - 1) ? 1 : 0;
    }
    memcpy(arr2, arr1, sizeof(arr1));

    insertion_sort(arr1, n);
    batcher_sort(arr2, n);

    for (int i = 0; i < n; i++) {
        if (arr1[i].src != arr2[i].src ||
            arr1[i].dst != arr2[i].dst ||
            arr1[i].opcode != arr2[i].opcode) {
            printf("MISMATCH at i=%d: ref=(%d,%d,%d) batcher=(%d,%d,%d)\n",
                   i, arr1[i].src, arr1[i].dst, (int)arr1[i].opcode,
                   arr2[i].src, arr2[i].dst, (int)arr2[i].opcode);
            return false;
        }
    }
    return true;
}

// ---- Dump unrolled Batcher network for HLS ----
static void dump_network16() {
    int n = 16;
    int total = 0;
    for (int p = 1; p < n; p <<= 1) {
        for (int k = p; k >= 1; k >>= 1) {
            printf("    // p=%d k=%d\n", p, k);
            for (int j = k % p; j + k < n; j += 2 * k) {
                for (int i = 0; i < k; i++) {
                    int a = i + j;
                    int b = a + k;
                    if (a / (2 * p) == b / (2 * p)) {
                        printf("    cmp_swap(arr[%2d], arr[%2d]);\n", a, b);
                        total++;
                    }
                }
            }
        }
    }
    printf("    // total cmp_swap ops: %d (unrolled)\n", total);
}

int main() {
    dump_network16();
    // Run correctness + perf test (iterative version)
    int total = 0, fail = 0;
    for (int n = 1; n <= 16; n++) {
        for (int seed = 0; seed < 10000; seed++) {
            total++;
            if (!test_one(n, seed)) {
                fail++;
                if (fail >= 5) { printf("Too many failures.\n"); return 1; }
            }
        }
    }
    printf("Tested %d cases, %d failures.\n", total, fail);
    if (fail == 0) printf("ALL PASSED\n");

    // Quick perf test
    PMA_UPDATE_MESSAGE perf_arr[16];
    for (int i = 0; i < 16; i++) {
        perf_arr[i].src = rand() % 256;
        perf_arr[i].dst = rand() % 256;
        perf_arr[i].opcode = (rand() & 1) ? DELETE_EDGE : INSERT_EDGE;
    }
    const int N = 1000000;
    volatile int sink;
    int start = clock();
    for (int k = 0; k < N; k++) {
        PMA_UPDATE_MESSAGE tmp[16];
        memcpy(tmp, perf_arr, sizeof(tmp));
        batcher_sort16(tmp);
        sink += tmp[0].src;
    }
    printf("Batcher: %d sorts in %d ms\n", N, (int)((clock() - start) * 1000 / CLOCKS_PER_SEC));
    return fail > 0;
}
