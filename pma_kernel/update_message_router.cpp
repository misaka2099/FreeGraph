#include "data_type.h"
#include "debug.h"
#include "etc/autopilot_ssdm_op.h"
#include "hls_print.h"
#include "hls_stream.h"
#include <cstdio>

#ifndef POOL_SIZE
#define POOL_SIZE 32
#endif
#ifndef POOL_QUEUE_DEPTH
#define POOL_QUEUE_DEPTH 64
#endif
#ifndef MAX_INFLIGHT_NUM
#define MAX_INFLIGHT_NUM 32
#endif

#define ROUTER_WORKERS CHANNEL_NUM

#define POLL_ALL_ACKS(t, cnt, a0, a1, a2, a3) do { \
    poll_ack(t, cnt, a0); poll_ack(t, cnt, a1); \
    poll_ack(t, cnt, a2); poll_ack(t, cnt, a3); \
} while(0)
#define POLL_ALL_ACKS_WAIT(t, cnt, a0, a1, a2, a3) do { \
    POLL_ALL_ACKS(t, cnt, a0, a1, a2, a3); \
} while(0)

// ---- Phase-based dispatch ----
#define MIN_ACCUMULATE_TARGET    8
#define MAX_ACCUMULATE_TARGET    128
#define INITIAL_ACCUMULATE_TARGET 32
#define DISPATCH_THRESHOLD       2
#define EVICTION_EARLY_DISPATCH  3

enum RouterPhase { ACCUMULATE, DISPATCH };

struct WorkerBest {
    int slot;      // -1 = none
    int q_count;
};

// ---- Pool: per-PMA slots for unsent messages ----
// Hot fields in registers for 1-cycle parallel scan; cold queue in URAM.

struct PoolSlot {
    int  q_head;
    int  q_tail;
    PMA_UPDATE_MESSAGE queue[POOL_QUEUE_DEPTH];
};
// Register tag arrays — fully partitioned for 1-cycle parallel compare
int   pool_tag[POOL_SIZE];
int   pool_wid[POOL_SIZE];
int pool_qcnt[POOL_SIZE];

#define POOL_QCNT_INIT 0
#define POOL_TAG_FREE  -1

// ============================================================
// Pool slot ops
// ============================================================

static int slot_find(int tag[POOL_SIZE], int qc[POOL_SIZE], int pma) {
    #pragma HLS INLINE
    // Priority 1: slot with matching PMA that has room
    for (int i = 0; i < POOL_SIZE; i++) {
        #pragma HLS UNROLL
        if (tag[i] == pma && qc[i] < POOL_QUEUE_DEPTH) return i;
    }
    // Priority 2: any matching slot (may be full — caller will alloc new one)
    for (int i = 0; i < POOL_SIZE; i++) {
        #pragma HLS UNROLL
        if (tag[i] == pma) return i;
    }
    return -1;
}

static int slot_alloc(int tag[POOL_SIZE], int wid_arr[POOL_SIZE],
                      int qc[POOL_SIZE], int pma, int wid) {
    #pragma HLS INLINE
    // Phase 1: find any free slot (32-way parallel, last match wins — no cross-iteration dependency)
    int free_slot = -1;
    for (int i = 0; i < POOL_SIZE; i++) {
        #pragma HLS UNROLL
        if (tag[i] == POOL_TAG_FREE) free_slot = i;
    }
    // Phase 2: allocate only that one slot
    if (free_slot != -1) {
        tag[free_slot]     = pma;
        wid_arr[free_slot] = wid;
        qc[free_slot]      = 0;
    }
    return free_slot;
}

static bool slot_enq(PoolSlot p[POOL_SIZE], int qc[POOL_SIZE], int s, PMA_UPDATE_MESSAGE m) {
    #pragma HLS INLINE
    if (qc[s] >= POOL_QUEUE_DEPTH) return false;
    p[s].queue[p[s].q_head] = m;
    p[s].q_head = (p[s].q_head + 1) % POOL_QUEUE_DEPTH;
    qc[s]++;
    return true;
}

static PMA_UPDATE_MESSAGE slot_deq(PoolSlot p[POOL_SIZE], int tag[POOL_SIZE],
                                    int qc[POOL_SIZE], int s) {
    #pragma HLS INLINE
    PMA_UPDATE_MESSAGE m = p[s].queue[p[s].q_tail];
    p[s].q_tail = (p[s].q_tail + 1) % POOL_QUEUE_DEPTH;
    qc[s]--;
    if (qc[s] == 0) {
        tag[s] = POOL_TAG_FREE;
    }
    return m;
}

// ============================================================
// In-flight table ops (PMA-level, not slot-level)
// ============================================================

static bool inflight_check(int t[MAX_INFLIGHT_NUM], int pma) {
    #pragma HLS INLINE
    for (int i = 0; i < MAX_INFLIGHT_NUM; i++) {
        #pragma HLS UNROLL
        if (t[i] == pma) return true;
    }
    return false;
}

static void inflight_add(int t[MAX_INFLIGHT_NUM], int &cnt, int pma) {
    #pragma HLS INLINE
    // Prevent duplicate: if already tracked, don't add again.
    if (inflight_check(t, pma)) return;
    if (cnt >= MAX_INFLIGHT_NUM) return;
    for (int i = 0; i < MAX_INFLIGHT_NUM; i++) {
        #pragma HLS UNROLL
        if (t[i] == -1) {
            t[i] = pma;
            cnt++;
            return;
        }
    }
}

static void inflight_remove(int t[MAX_INFLIGHT_NUM], int &cnt, int pma) {
    #pragma HLS INLINE
    for (int i = 0; i < MAX_INFLIGHT_NUM; i++) {
        #pragma HLS UNROLL
        if (t[i] == pma) {
            t[i] = -1;
            cnt--;
            return;
        }
    }
    // PMA not found in table — table/count is now out of sync.
    // hls::print("[ROUTER] RM FAIL pma=%d\n", pma);
    return;
}

// ============================================================
// Flush at most MAX_CMD_NUM messages from one slot.
// Returns true if messages were actually sent.
// ============================================================

static bool slot_flush_one_batch(
    PoolSlot p[POOL_SIZE], int tag[POOL_SIZE], int wid_arr[POOL_SIZE],
    int qc[POOL_SIZE], int s,
    hls::stream<PMA_UPDATE_MESSAGE> &c0,
    hls::stream<PMA_UPDATE_MESSAGE> &c1,
    hls::stream<PMA_UPDATE_MESSAGE> &c2,
    hls::stream<PMA_UPDATE_MESSAGE> &c3)
{
    #pragma HLS INLINE
    int n = qc[s];
    if (n == 0) {
        tag[s] = POOL_TAG_FREE;
        return false;
    }
    int wid = wid_arr[s];
    int batch = (n < MAX_CMD_NUM) ? n : MAX_CMD_NUM;
    for (int i = 0; i < batch; i++) {
        #pragma HLS PIPELINE II=1
        PMA_UPDATE_MESSAGE m = slot_deq(p, tag, qc, s);
        m.last = (i == batch - 1) ? 1 : 0;
        if      (wid == 0) c0.write(m);
        else if (wid == 1) c1.write(m);
        else if (wid == 2) c2.write(m);
        else               c3.write(m);
    }
    return true;
}

// ============================================================
// Step 1: Poll ACKs — only remove from in_flight, no dispatch.
// ============================================================

static void poll_ack(
    int t[MAX_INFLIGHT_NUM], int &t_cnt,
    hls::stream<COMMAND_STREAM> &ack_st)
{
    #pragma HLS INLINE
    COMMAND_STREAM ack;
    if (ack_st.read_nb(ack))
    {
        inflight_remove(t, t_cnt, ack.data);
    }
}

// ============================================================
// Eviction candidate: non-in_flight PMA, shortest queue.
// ============================================================

static int slot_evict(
    int tag[POOL_SIZE], int qc[POOL_SIZE],
    int t[MAX_INFLIGHT_NUM],
    int for_pma)
{
    #pragma HLS INLINE
    int best = -1;
    int best_len = POOL_QUEUE_DEPTH + 1;
    for (int i = 0; i < POOL_SIZE; i++) {
        #pragma HLS PIPELINE II=1
        int ev_pma = tag[i];
        if (ev_pma == POOL_TAG_FREE || ev_pma == for_pma) continue;
        if (inflight_check(t, ev_pma)) continue;
        if (qc[i] < best_len) {
            best = i;
            best_len = qc[i];
        }
    }
    return best;
}

// ============================================================
// Drain a victim slot and reuse it for a new PMA.
// Returns the victim slot index (now assigned to the new PMA).
// Increments eviction_count.
// ============================================================

static int slot_evict_and_reuse(
    PoolSlot p[POOL_SIZE], int tag[POOL_SIZE], int wid_arr[POOL_SIZE],
    int qc[POOL_SIZE], int t[MAX_INFLIGHT_NUM], int &t_cnt,
    int ev, int new_pma, int new_wid,
    int &eviction_cnt,
    hls::stream<PMA_UPDATE_MESSAGE> &c0,
    hls::stream<PMA_UPDATE_MESSAGE> &c1,
    hls::stream<PMA_UPDATE_MESSAGE> &c2,
    hls::stream<PMA_UPDATE_MESSAGE> &c3,
    hls::stream<COMMAND_STREAM> &a0,
    hls::stream<COMMAND_STREAM> &a1,
    hls::stream<COMMAND_STREAM> &a2,
    hls::stream<COMMAND_STREAM> &a3)
{
    int ev_pma = tag[ev];
    while (qc[ev] > 0) {
        #pragma HLS PIPELINE off
        while (inflight_check(t, ev_pma) || t_cnt >= MAX_INFLIGHT_NUM) {
            #pragma HLS PIPELINE off
            POLL_ALL_ACKS_WAIT(t, t_cnt, a0, a1, a2, a3);
        }
        if (slot_flush_one_batch(p, tag, wid_arr, qc, ev, c0, c1, c2, c3))
            inflight_add(t, t_cnt, ev_pma);
    }
    eviction_cnt++;
    tag[ev]      = new_pma;
    wid_arr[ev]  = new_wid;
    p[ev].q_head = 0;
    p[ev].q_tail = 0;
    qc[ev]       = 0;
    return ev;
}

// ============================================================
// Rescan best slot for a specific worker.
// Only called when wb[w].slot is invalidated (consumed or stale).
// ============================================================

static void rescan_wb(
    int tag[POOL_SIZE], int wid_arr[POOL_SIZE], int qc[POOL_SIZE],
    int t[MAX_INFLIGHT_NUM],
    int w,
    WorkerBest wb[ROUTER_WORKERS])
{
    #pragma HLS INLINE
    int best = -1;
    int best_len = DISPATCH_THRESHOLD - 1;
    for (int i = 0; i < POOL_SIZE; i++) {
        #pragma HLS PIPELINE II=1
        if (tag[i] == POOL_TAG_FREE || qc[i] < DISPATCH_THRESHOLD) continue;
        if (wid_arr[i] != w) continue;
        if (inflight_check(t, tag[i])) continue;
        if (qc[i] > best_len) {
            best = i;
            best_len = qc[i];
        }
    }
    wb[w].slot = best;
    wb[w].q_count = (best != -1) ? best_len : 0;
}

// ============================================================
// Starvation scan: find any non-empty, non-inflight slot
// regardless of threshold. Used as fallback when no slot
// meets DISPATCH_THRESHOLD.
// ============================================================

static int rescan_any(
    int tag[POOL_SIZE], int wid_arr[POOL_SIZE], int qc[POOL_SIZE],
    int t[MAX_INFLIGHT_NUM],
    int w)
{
    #pragma HLS INLINE
    int best = -1;
    int best_len = 0;
    for (int i = 0; i < POOL_SIZE; i++) {
        #pragma HLS PIPELINE II=1
        if (tag[i] == POOL_TAG_FREE || qc[i] == 0) continue;
        if (wid_arr[i] != w) continue;
        if (inflight_check(t, tag[i])) continue;
        if (qc[i] > best_len) {
            best = i;
            best_len = qc[i];
        }
    }
    return best;
}

// ============================================================
// Main kernel
// ============================================================

extern "C"
{
    void update_message_router(
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_from_generator,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_1,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_2,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_3,
        hls::stream<PMA_UPDATE_MESSAGE> &cmd_to_insert_edge_4,
        hls::stream<COMMAND_STREAM> &ack_from_insert_edge_1,
        hls::stream<COMMAND_STREAM> &ack_from_insert_edge_2,
        hls::stream<COMMAND_STREAM> &ack_from_insert_edge_3,
        hls::stream<COMMAND_STREAM> &ack_from_insert_edge_4,
        hls::stream<WHOLE_ACK> &ack_to_generator
    )
    {
        #pragma HLS INTERFACE axis port=cmd_from_generator
        #pragma HLS INTERFACE axis port=cmd_to_insert_edge_1
        #pragma HLS INTERFACE axis port=cmd_to_insert_edge_2
        #pragma HLS INTERFACE axis port=cmd_to_insert_edge_3
        #pragma HLS INTERFACE axis port=cmd_to_insert_edge_4
        #pragma HLS INTERFACE axis port=ack_from_insert_edge_1
        #pragma HLS INTERFACE axis port=ack_from_insert_edge_2
        #pragma HLS INTERFACE axis port=ack_from_insert_edge_3
        #pragma HLS INTERFACE axis port=ack_from_insert_edge_4
        #pragma HLS INTERFACE axis port=ack_to_generator

        #pragma HLS STREAM variable=cmd_from_generator depth=16
        #pragma HLS STREAM variable=cmd_to_insert_edge_1 depth=16
        #pragma HLS STREAM variable=cmd_to_insert_edge_2 depth=16
        #pragma HLS STREAM variable=cmd_to_insert_edge_3 depth=16
        #pragma HLS STREAM variable=cmd_to_insert_edge_4 depth=16
        #pragma HLS STREAM variable=ack_from_insert_edge_1 depth=16
        #pragma HLS STREAM variable=ack_from_insert_edge_2 depth=16
        #pragma HLS STREAM variable=ack_from_insert_edge_3 depth=16
        #pragma HLS STREAM variable=ack_from_insert_edge_4 depth=16
        #pragma HLS STREAM variable=ack_to_generator depth=16

        PoolSlot pool[POOL_SIZE];
        #pragma HLS BIND_STORAGE variable=pool type=RAM_2P impl=URAM
        for (int i = 0; i < POOL_SIZE; i++) {
            #pragma HLS UNROLL
            pool[i].q_head = 0;
            pool[i].q_tail = 0;
            pool_tag[i]  = POOL_TAG_FREE;
            pool_wid[i]  = 0;
            pool_qcnt[i] = 0;
        }

        int in_flight[MAX_INFLIGHT_NUM];
        #pragma HLS ARRAY_PARTITION variable=in_flight complete
        for (int i = 0; i < MAX_INFLIGHT_NUM; i++) in_flight[i] = -1;
        int in_flight_count = 0;

        // ---- Phase state ----
        RouterPhase phase = ACCUMULATE;
        WorkerBest wb[ROUTER_WORKERS];
        #pragma HLS ARRAY_PARTITION variable=wb complete
        for (int w = 0; w < ROUTER_WORKERS; w++) {
            wb[w].slot = -1;
            wb[w].q_count = 0;
        }
        int accumulate_target = INITIAL_ACCUMULATE_TARGET;
        int accumulated = 0;
        int eviction_count = 0;
        int dispatched = 0;

        while (true) {

            // ==================================================
            // Phase-independent: Poll ACKs from all workers.
            // ==================================================
            {
                poll_ack(in_flight, in_flight_count,
                    ack_from_insert_edge_1);
                poll_ack(in_flight, in_flight_count,
                    ack_from_insert_edge_2);
                poll_ack(in_flight, in_flight_count,
                    ack_from_insert_edge_3);
                poll_ack(in_flight, in_flight_count,
                    ack_from_insert_edge_4);
            }

            // ==================================================
            // ACCUMULATE phase: read, STOP/enqueue, transition.
            // Only read from generator during ACCUMULATE to
            // avoid silently dropping messages during DISPATCH.
            // ==================================================
            if (phase == ACCUMULATE) {

                PMA_UPDATE_MESSAGE msg;
                if (cmd_from_generator.read_nb(msg)) {

                    if (msg.opcode == PMA_UPDATE_STOP) {
                        // ---- STOP received: round-robin flush all slots ----
                        // Dispatch 1 batch per ready slot per iteration to
                        // keep all workers busy concurrently.
                        bool any_left;
                        do {
                            any_left = false;
                            for (int i = 0; i < POOL_SIZE; i++) {
                                #pragma HLS PIPELINE II=1
                                if (pool_tag[i] == -1 || pool_qcnt[i] == 0)
                                    continue;
                                int pma_i = pool_tag[i];
                                if (inflight_check(in_flight, pma_i)
                                    || in_flight_count >= MAX_INFLIGHT_NUM) {
                                    any_left = true;
                                    continue;
                                }
                                if (slot_flush_one_batch(pool, pool_tag, pool_wid, pool_qcnt, i,
                                    cmd_to_insert_edge_1, cmd_to_insert_edge_2, cmd_to_insert_edge_3, cmd_to_insert_edge_4))
                                    inflight_add(in_flight, in_flight_count, pma_i);
                                if (pool_qcnt[i] > 0) any_left = true;
                                else {
                                    pool_tag[i] = -1;
                                    pool[i].q_head = 0;
                                    pool[i].q_tail = 0;
                                }
                            }
                            if (any_left) {
                                POLL_ALL_ACKS_WAIT(in_flight, in_flight_count, ack_from_insert_edge_1, ack_from_insert_edge_2, ack_from_insert_edge_3, ack_from_insert_edge_4);
                            }
                        } while (any_left);

                        // Drain remaining in_flight (all slots emptied above).
                        while (in_flight_count > 0) {
                            POLL_ALL_ACKS_WAIT(in_flight, in_flight_count, ack_from_insert_edge_1, ack_from_insert_edge_2, ack_from_insert_edge_3, ack_from_insert_edge_4);
                        }

                        if (msg.last == 1) {
                            msg.opcode = PMA_UPDATE_STOP;
                            cmd_to_insert_edge_1.write(msg);
                            cmd_to_insert_edge_2.write(msg);
                            cmd_to_insert_edge_3.write(msg);
                            cmd_to_insert_edge_4.write(msg);
                            #pragma HLS protocol fixed
                            auto a1 = ack_from_insert_edge_1.read();
                            auto a2 = ack_from_insert_edge_2.read();
                            auto a3 = ack_from_insert_edge_3.read();
                            auto a4 = ack_from_insert_edge_4.read();
                            #pragma HLS protocol fixed
                            WHOLE_ACK pkt;
                            pkt.data = 1;
                            pkt.keep = (ap_uint<4>)0xF;
                            pkt.strb = (ap_uint<4>)0xF;
                            pkt.last = 1;
                            ack_to_generator.write(pkt);
                            break;
                        } else {
                            msg.opcode = PMA_UPDATE_FLUSH_CACHE;
                            cmd_to_insert_edge_1.write(msg);
                            cmd_to_insert_edge_2.write(msg);
                            cmd_to_insert_edge_3.write(msg);
                            cmd_to_insert_edge_4.write(msg);
                            #pragma HLS protocol fixed
                            auto a1 = ack_from_insert_edge_1.read();
                            auto a2 = ack_from_insert_edge_2.read();
                            auto a3 = ack_from_insert_edge_3.read();
                            auto a4 = ack_from_insert_edge_4.read();
                            #pragma HLS protocol fixed
                            WHOLE_ACK pkt;
                            pkt.data = 1;
                            pkt.keep = (ap_uint<4>)0xF;
                            pkt.strb = (ap_uint<4>)0xF;
                            pkt.last = 1;
                            ack_to_generator.write(pkt);
                        }

                    } else {
                        // ---- Normal edge: enqueue ----
                    int pma = msg.src / VERTEX_PER_PMA;
                    int wid = pma % ROUTER_WORKERS;
                    int slot = slot_find(pool_tag, pool_qcnt, pma);

                    if (slot == -1) {
                        slot = slot_alloc(pool_tag, pool_wid, pool_qcnt, pma, wid);
                    }

                    // Enqueue with retry limit: after N failed ACK polls,
                    // force a DISPATCH round to drain in_flight, then retry.
                    #define ENQUEUE_RETRY_LIMIT 5
                    bool enqueued = false;
                    while (!enqueued) {

                        // If slot is full, flush one batch from it (no duplicate slot)
                        if (slot != -1 && pool_qcnt[slot] >= POOL_QUEUE_DEPTH) {
                            while (inflight_check(in_flight, pma) || in_flight_count >= MAX_INFLIGHT_NUM) {
                                POLL_ALL_ACKS_WAIT(in_flight, in_flight_count, ack_from_insert_edge_1, ack_from_insert_edge_2, ack_from_insert_edge_3, ack_from_insert_edge_4);
                            }
                            if (slot_flush_one_batch(pool, pool_tag, pool_wid, pool_qcnt, slot,
                                cmd_to_insert_edge_1, cmd_to_insert_edge_2, cmd_to_insert_edge_3, cmd_to_insert_edge_4))
                                inflight_add(in_flight, in_flight_count, pma);
                            eviction_count++;
                        }

                        // Try to obtain a slot via eviction, with limited retries
                        int retries = 0;
                        while (slot == -1 && retries < ENQUEUE_RETRY_LIMIT) {
                            int ev = slot_evict(pool_tag, pool_qcnt, in_flight, pma);
                            if (ev == -1) {
                                poll_ack(in_flight, in_flight_count,
                                    ack_from_insert_edge_1);
                                poll_ack(in_flight, in_flight_count,
                                    ack_from_insert_edge_2);
                                poll_ack(in_flight, in_flight_count,
                                    ack_from_insert_edge_3);
                                poll_ack(in_flight, in_flight_count,
                                    ack_from_insert_edge_4);
                                retries++;
                                slot = slot_find(pool_tag, pool_qcnt, pma);
                                if (slot == -1) {
                                    int ns = slot_alloc(pool_tag, pool_wid, pool_qcnt, pma, wid);
                                    if (ns != -1) slot = ns;
                                }
                                continue;
                            }
                            slot = slot_evict_and_reuse(
                                pool, pool_tag, pool_wid, pool_qcnt,
                                in_flight, in_flight_count,
                                ev, pma, wid, eviction_count,
                                cmd_to_insert_edge_1, cmd_to_insert_edge_2,
                                cmd_to_insert_edge_3, cmd_to_insert_edge_4,
                                ack_from_insert_edge_1, ack_from_insert_edge_2,
                                ack_from_insert_edge_3, ack_from_insert_edge_4);
                            retries = 0; // eviction succeeded, reset
                        }

                        if (slot == -1) {
                            // Retry limit hit: force a passive dispatch round
                            for (int w = 0; w < ROUTER_WORKERS; w++) {
                                #pragma HLS UNROLL
                                if (in_flight_count >= MAX_INFLIGHT_NUM) continue;
                                int s = -1;
                                int best = 0;
                                for (int j = 0; j < POOL_SIZE; j++) {
                                    #pragma HLS PIPELINE II=1
                                    if (pool_tag[j] == -1 || pool_qcnt[j] == 0) continue;
                                    if (pool_wid[j] != w) continue;
                                    if (inflight_check(in_flight, pool_tag[j])) continue;
                                    if (pool_qcnt[j] > best) { best = pool_qcnt[j]; s = j; }
                                }
                                if (s != -1) {
                                    if (slot_flush_one_batch(pool, pool_tag, pool_wid, pool_qcnt, s,
                                        cmd_to_insert_edge_1, cmd_to_insert_edge_2, cmd_to_insert_edge_3, cmd_to_insert_edge_4))
                                        inflight_add(in_flight, in_flight_count, pool_tag[s]);
                                    dispatched++;
                                }
                            }
                            // Try to get a slot again after dispatch
                            slot = slot_find(pool_tag, pool_qcnt, pma);
                            if (slot == -1)
                                slot = slot_alloc(pool_tag, pool_wid, pool_qcnt, pma, wid);
                            continue;
                        }

                        if (slot_enq(pool, pool_qcnt, slot, msg)) {
                            enqueued = true;
                            accumulated++;
                        }
                        // If slot_enq fails (shouldn't happen with flush+alloc above),
                        // loop back and retry
                    }
                    #undef ENQUEUE_RETRY_LIMIT
                }
                }

                // ---- Phase transition: ACCUMULATE → DISPATCH ----
                if (accumulated >= accumulate_target
                    || eviction_count >= EVICTION_EARLY_DISPATCH) {
                    phase = DISPATCH;
                    dispatched = 0;
                }

            } else {
                // ==============================================
                // DISPATCH phase: send one batch per worker.
                // Use wb for O(1) best-slot lookup; rescan only
                // when wb is invalidated.
                // ==============================================
                bool any = false;
                for (int w = 0; w < ROUTER_WORKERS; w++) {
                    #pragma HLS UNROLL
                    if (in_flight_count >= MAX_INFLIGHT_NUM) continue;

                    // Validate or rebuild wb for this worker
                    if (wb[w].slot != -1) {
                        int s = wb[w].slot;
                        if (pool_tag[s] == -1
                            || pool_wid[s] != w
                            || inflight_check(in_flight, pool_tag[s])
                            || pool_qcnt[s] < DISPATCH_THRESHOLD) {
                            wb[w].slot = -1;
                        }
                    }
                    if (wb[w].slot == -1) {
                        rescan_wb(pool_tag, pool_wid, pool_qcnt,in_flight, w, wb);
                    }

                    if (wb[w].slot != -1) {
                        int s = wb[w].slot;
                        int pma = pool_tag[s];
                        int batch = ((int)pool_qcnt[s] < MAX_CMD_NUM)
                                    ? (int)pool_qcnt[s] : MAX_CMD_NUM;
                        if (slot_flush_one_batch(pool, pool_tag, pool_wid, pool_qcnt,s,
                            cmd_to_insert_edge_1, cmd_to_insert_edge_2, cmd_to_insert_edge_3, cmd_to_insert_edge_4)) {
                            inflight_add(in_flight, in_flight_count, pma);
                            dispatched += batch;
                            any = true;
                        }
                        wb[w].slot = -1;
                    }
                }

                // Starvation fix: if nothing dispatched and pool has
                // non-inflight messages, force dispatch one regardless
                // of threshold.
                if (!any) {
                    for (int w = 0; w < ROUTER_WORKERS; w++) {
                        #pragma HLS UNROLL
                        if (in_flight_count >= MAX_INFLIGHT_NUM) continue;
                        int s = rescan_any(pool_tag, pool_wid, pool_qcnt,in_flight, w);
                        if (s != -1) {
                            int pma = pool_tag[s];
                            int batch = ((int)pool_qcnt[s] < MAX_CMD_NUM)
                                        ? (int)pool_qcnt[s] : MAX_CMD_NUM;
                            if (slot_flush_one_batch(pool, pool_tag, pool_wid, pool_qcnt,s,
                                cmd_to_insert_edge_1, cmd_to_insert_edge_2, cmd_to_insert_edge_3, cmd_to_insert_edge_4)) {
                                inflight_add(in_flight, in_flight_count, pma);
                                dispatched += batch;
                                any = true;
                            }
                            break;
                        }
                    }
                }

                // ---- Phase transition: DISPATCH → ACCUMULATE ----
                if (!any) {
                    // Adapt accumulate_target based on dispatch success
                    if (dispatched >= accumulate_target * 3 / 4) {
                        if (accumulate_target < MAX_ACCUMULATE_TARGET)
                            accumulate_target *= 2;
                        if (accumulate_target > MAX_ACCUMULATE_TARGET)
                            accumulate_target = MAX_ACCUMULATE_TARGET;
                    } else if (dispatched < accumulate_target / 2) {
                        if (accumulate_target > MIN_ACCUMULATE_TARGET)
                            accumulate_target /= 2;
                        if (accumulate_target < MIN_ACCUMULATE_TARGET)
                            accumulate_target = MIN_ACCUMULATE_TARGET;
                    }
                    accumulated = 0;
                    eviction_count = 0;
                    for (int w = 0; w < ROUTER_WORKERS; w++) {
                        #pragma HLS UNROLL
                        wb[w].slot = -1;
                        wb[w].q_count = 0;
                    }
                    phase = ACCUMULATE;
                }
            }
        }
    }
}
