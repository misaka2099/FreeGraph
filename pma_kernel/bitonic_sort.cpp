#ifndef BITONIC_SORT_H
#define BITONIC_SORT_H

// ---- Configuration ----
// TASK_BURST_CHUNK = 256 in the PMA kernel, so 256 is the next power of 2
#define BITONIC_MAX_SIZE 256
// Sentinel: must be larger than any real src value
#define TASK_SENTINEL 0x7FFFFFFF

// ---- Bitonic Sort ----
// Sorts tasks[0..count-1] by src ascending using a bitonic sorting network.
// INNER_TASK must be defined before including this file.
// Tasks[count..BITONIC_MAX_SIZE-1] are padded with sentinel values.
// After sorting, valid elements occupy positions 0..count-1 and the sentinels
// are at positions count..BITONIC_MAX_SIZE-1 (guaranteed by ascending order).
void bitonic_sort(INNER_TASK tasks[], int count) {
    #pragma HLS INLINE off

    // Pad unused positions with sentinel values.  Variable bound loop —
    // use PIPELINE (not UNROLL) since count is not compile-time constant.
    for (int i = count; i < BITONIC_MAX_SIZE; i++) {
        #pragma HLS PIPELINE II=1
        tasks[i].src    = TASK_SENTINEL;
        tasks[i].dst    = TASK_SENTINEL;
        tasks[i].opcode = TASK_SENTINEL;
    }

    // Classic bitonic sorting network.
    //
    //   k  – size of the bitonic sequence being built (doubles each phase)
    //   j  – distance between compared elements (halves within a phase)
    //   i  – element index; pair (i, i^j) is compared when i^j > i
    //
    // Direction: (i & k) == 0  → ascending (lower src first)
    //            (i & k) != 0  → descending (higher src first)
    //
    // All comparisions within a (k, j) pair are independent, so the inner
    // loop can be pipelined at II=1.
    for (int k = 2; k <= BITONIC_MAX_SIZE; k *= 2) {
        for (int j = k / 2; j > 0; j /= 2) {
            for (int i = 0; i < BITONIC_MAX_SIZE; i++) {
                #pragma HLS PIPELINE II=1

                int ixj = i ^ j;
                if (ixj > i) {
                    INNER_TASK &a = tasks[i];
                    INNER_TASK &b = tasks[ixj];

                    bool do_swap;
                    if ((i & k) == 0) {
                        // ascending half: smaller src should be at lower index
                        do_swap = (a.src > b.src);
                    } else {
                        // descending half: larger src should be at lower index
                        do_swap = (a.src < b.src);
                    }

                    if (do_swap) {
                        INNER_TASK tmp = a;
                        a = b;
                        b = tmp;
                    }
                }
            }
        }
    }
}

#endif // BITONIC_SORT_H
