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
#define MAX_INFLIGHT_NUM 16
#endif

#define ROUTER_WORKERS CHANNEL_NUM

// ---- Pool: per-PMA slots for unsent messages ----

struct PoolSlot {
    int  pma_idx;       // -1 = free
    int  worker_id;
    int  q_head;
    int  q_tail;
    int  q_count;
    PMA_UPDATE_MESSAGE queue[POOL_QUEUE_DEPTH];
};

// ============================================================
// Pool slot ops
// ============================================================

static int slot_find(PoolSlot p[POOL_SIZE], int pma) {
    #pragma HLS INLINE
    for (int i = 0; i < POOL_SIZE; i++) {
        if (p[i].pma_idx == pma && p[i].q_count < POOL_QUEUE_DEPTH)
            return i;
    }
    for (int i = 0; i < POOL_SIZE; i++) {
        if (p[i].pma_idx == pma) return i;
    }
    return -1;
}

static int slot_find_shortest(PoolSlot p[POOL_SIZE], int pma) {
    #pragma HLS INLINE
    int best = -1;
    int best_len = POOL_QUEUE_DEPTH + 1;
    for (int i = 0; i < POOL_SIZE; i++) {
        if (p[i].pma_idx == pma && p[i].q_count > 0 && p[i].q_count < best_len) {
            best = i;
            best_len = p[i].q_count;
        }
    }
    return best;
}

static int slot_alloc(PoolSlot p[POOL_SIZE], int pma, int wid) {
    #pragma HLS INLINE
    for (int i = 0; i < POOL_SIZE; i++) {
        if (p[i].pma_idx == -1) {
            p[i].pma_idx   = pma;
            p[i].worker_id = wid;
            p[i].q_head    = 0;
            p[i].q_tail    = 0;
            p[i].q_count   = 0;
            return i;
        }
    }
    return -1;
}

static bool slot_enq(PoolSlot p[POOL_SIZE], int s, PMA_UPDATE_MESSAGE m) {
    #pragma HLS INLINE
    if (p[s].q_count >= POOL_QUEUE_DEPTH) return false;
    p[s].queue[p[s].q_head] = m;
    p[s].q_head = (p[s].q_head + 1) % POOL_QUEUE_DEPTH;
    p[s].q_count++;
    return true;
}

static PMA_UPDATE_MESSAGE slot_deq(PoolSlot p[POOL_SIZE], int s) {
    #pragma HLS INLINE
    PMA_UPDATE_MESSAGE m = p[s].queue[p[s].q_tail];
    p[s].q_tail = (p[s].q_tail + 1) % POOL_QUEUE_DEPTH;
    p[s].q_count--;
    if (p[s].q_count == 0)
    {
        p[s].pma_idx = -1;
    }
    return m;
}

// ============================================================
// In-flight table ops (PMA-level, not slot-level)
// ============================================================

static bool inflight_check(int t[MAX_INFLIGHT_NUM], int pma) {
    #pragma HLS INLINE
    for (int i = 0; i < MAX_INFLIGHT_NUM; i++) {
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
    PoolSlot p[POOL_SIZE], int s,
    hls::stream<PMA_UPDATE_MESSAGE> &c0,
    hls::stream<PMA_UPDATE_MESSAGE> &c1,
    hls::stream<PMA_UPDATE_MESSAGE> &c2,
    hls::stream<PMA_UPDATE_MESSAGE> &c3)
{
    #pragma HLS INLINE
    int n = p[s].q_count;
    if (n == 0) {
        p[s].pma_idx = -1;
        return false;
    }
    int wid = p[s].worker_id;
    int batch = (n < MAX_CMD_NUM) ? n : MAX_CMD_NUM;
    for (int i = 0; i < batch; i++) {
        #pragma HLS PIPELINE II=1
        PMA_UPDATE_MESSAGE m = slot_deq(p, s);
        m.last = (i == batch - 1) ? 1 : 0;
        if (wid == 0) c0.write(m);
        else if (wid == 1) c1.write(m);
        else if (wid == 2) c2.write(m);
        else c3.write(m);
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
    PoolSlot p[POOL_SIZE],
    int t[MAX_INFLIGHT_NUM],
    int for_pma)
{
    #pragma HLS INLINE
    int best = -1;
    int best_len = POOL_QUEUE_DEPTH + 1;
    for (int i = 0; i < POOL_SIZE; i++) {
        int ev_pma = p[i].pma_idx;
        if (ev_pma == -1 || ev_pma == for_pma) continue;
        if (inflight_check(t, ev_pma)) continue;
        if (p[i].q_count < best_len) {
            best = i;
            best_len = p[i].q_count;
        }
    }
    return best;
}

// ============================================================
// Step 3 helper: longest non-in_flight PMA queue.
// ============================================================

static int step3_find_best(
    PoolSlot p[POOL_SIZE],
    int t[MAX_INFLIGHT_NUM],
    int min_len)
{
    #pragma HLS INLINE
    int best = -1;
    int best_len = min_len - 1;
    for (int i = 0; i < POOL_SIZE; i++) {
        if (p[i].pma_idx == -1 || p[i].q_count == 0) continue;
        if (inflight_check(t, p[i].pma_idx)) continue;
        if (p[i].q_count > best_len) {
            best = i;
            best_len = p[i].q_count;
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
            pool[i].pma_idx   = -1;
            pool[i].q_head    = 0;
            pool[i].q_tail    = 0;
            pool[i].q_count   = 0;
        }

        int in_flight[MAX_INFLIGHT_NUM];
        #pragma HLS ARRAY_PARTITION variable=in_flight complete
        for (int i = 0; i < MAX_INFLIGHT_NUM; i++) in_flight[i] = -1;
        int in_flight_count = 0;

        while (true) {

            // ==================================================
            // Step 1: Poll ACKs — only remove from in_flight.
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
            // Step 2: Read one message, enqueue.
            // Spins within this cycle if pool full / eviction
            // needed.  Handles its own ACK polling + dispatch
            // for the eviction target only.
            // ==================================================
            PMA_UPDATE_MESSAGE msg;
            if (cmd_from_generator.read_nb(msg)) {

                if (msg.opcode == PMA_UPDATE_STOP) {
                    // hls::print("[ROUTER] STOP in_flight=%d\n", in_flight_count);

                    // Flush all pending messages.
                    // For each non-empty slot: drain completely,
                    // dispatching batches and waiting for ACKs.
                    for (int i = 0; i < POOL_SIZE; i++) {
                        if (pool[i].pma_idx == -1 || pool[i].q_count == 0)
                            continue;
                        int pma_i = pool[i].pma_idx;
                        while (pool[i].q_count > 0) {
                            // Wait until PMA not in_flight and table has room
                            
                            #pragma HLS PIPELINE off
                            while (inflight_check(in_flight, pma_i)
                                   || in_flight_count >= MAX_INFLIGHT_NUM) {
                                
                                #pragma HLS PIPELINE off
                                // hls::print("[ROUTER] STOP wait pma=%d\n", pma_i);
                                // hls::print("[ROUTER] STOP wait cnt=%d\n", in_flight_count);
                                poll_ack(in_flight, in_flight_count,
                                    ack_from_insert_edge_1);
                                poll_ack(in_flight, in_flight_count,
                                    ack_from_insert_edge_2);
                                poll_ack(in_flight, in_flight_count,
                                    ack_from_insert_edge_3);
                                poll_ack(in_flight, in_flight_count,
                                    ack_from_insert_edge_4);
                                ap_wait();
                            }
                            if (slot_flush_one_batch(pool, i,
                                cmd_to_insert_edge_1, cmd_to_insert_edge_2,
                                cmd_to_insert_edge_3, cmd_to_insert_edge_4))
                                inflight_add(in_flight, in_flight_count, pma_i);
                        }
                        // Slot fully drained; mark it free.
                        pool[i].pma_idx   = -1;
                        pool[i].q_head    = 0;
                        pool[i].q_tail    = 0;
                    }

                    // Drain remaining in_flight.
                    // All slots were drained by the for loop above;
                    // just wait for the last ACKs to arrive.
                    while (in_flight_count > 0) {
                        poll_ack(in_flight, in_flight_count,
                            ack_from_insert_edge_1);
                        poll_ack(in_flight, in_flight_count,
                            ack_from_insert_edge_2);
                        poll_ack(in_flight, in_flight_count,
                            ack_from_insert_edge_3);
                        poll_ack(in_flight, in_flight_count,
                            ack_from_insert_edge_4);
                        ap_wait();
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
                    // ---- Normal edge ----
                    int pma = msg.src / VERTEX_PER_PMA;
                    int wid = pma % ROUTER_WORKERS;
                    int slot = slot_find(pool, pma);

                    if (slot == -1) {
                        slot = slot_alloc(pool, pma, wid);
                    } else if (pool[slot].q_count >= POOL_QUEUE_DEPTH) {
                        int ns = slot_alloc(pool, pma, wid);
                        if (ns != -1) slot = ns;
                    }

                    // Spin until message is enqueued.
                    bool enqueued = false;
                    while (!enqueued) {

                        // ---- Still no slot: evict ----
                        while (slot == -1) {
                            int ev = slot_evict(pool, in_flight, pma);
                            if (ev == -1) {
                                // All PMAs in flight: poll ACKs to free slots
                                poll_ack(in_flight, in_flight_count,
                                    ack_from_insert_edge_1);
                                poll_ack(in_flight, in_flight_count,
                                    ack_from_insert_edge_2);
                                poll_ack(in_flight, in_flight_count,
                                    ack_from_insert_edge_3);
                                poll_ack(in_flight, in_flight_count,
                                    ack_from_insert_edge_4);
                                ap_wait();
                                slot = slot_find(pool, pma);
                                if (slot == -1 || pool[slot].q_count >= POOL_QUEUE_DEPTH) {
                                    int ns = slot_alloc(pool, pma, wid);
                                    if (ns != -1) slot = ns;
                                }
                                continue;
                            }

                            // Drain the eviction slot completely.
                            int ev_pma = pool[ev].pma_idx;
                            while (pool[ev].q_count > 0) {
                                while (inflight_check(in_flight, ev_pma)
                                       || in_flight_count >= MAX_INFLIGHT_NUM) {
                                    poll_ack(in_flight, in_flight_count,
                                        ack_from_insert_edge_1);
                                    poll_ack(in_flight, in_flight_count,
                                        ack_from_insert_edge_2);
                                    ap_wait();
                                }
                                if (slot_flush_one_batch(pool, ev,
                                    cmd_to_insert_edge_1, cmd_to_insert_edge_2,
                                    cmd_to_insert_edge_3, cmd_to_insert_edge_4))
                                    inflight_add(in_flight, in_flight_count, ev_pma);
                            }
                            // Slot empty — reuse
                            pool[ev].pma_idx   = pma;
                            pool[ev].worker_id = wid;
                            pool[ev].q_head    = 0;
                            pool[ev].q_tail    = 0;
                            pool[ev].q_count   = 0;
                            slot = ev;
                        }

                        // ---- Enqueue ----
                        if (slot_enq(pool, slot, msg)) {
                            enqueued = true;
                        } else {
                            int ns = slot_alloc(pool, pma, wid);
                            if (ns != -1) { slot = ns; continue; }
                            int ev = slot_evict(pool, in_flight, pma);
                            if (ev == -1) {
                                poll_ack(in_flight, in_flight_count,
                                    ack_from_insert_edge_1);
                                poll_ack(in_flight, in_flight_count,
                                    ack_from_insert_edge_2);
                                poll_ack(in_flight, in_flight_count,
                                    ack_from_insert_edge_3);
                                poll_ack(in_flight, in_flight_count,
                                    ack_from_insert_edge_4);
                                ap_wait();
                                continue;
                            }
                            int ev_pma = pool[ev].pma_idx;
                            while (pool[ev].q_count > 0) {
                                #pragma HLS PIPELINE off
                                while (inflight_check(in_flight, ev_pma)
                                       || in_flight_count >= MAX_INFLIGHT_NUM) {
                                    poll_ack(in_flight, in_flight_count,
                                        ack_from_insert_edge_1);
                                    poll_ack(in_flight, in_flight_count,
                                        ack_from_insert_edge_2);
                                    ap_wait();
                                }
                                #pragma HLS PIPELINE off
                                if (slot_flush_one_batch(pool, ev,
                                    cmd_to_insert_edge_1, cmd_to_insert_edge_2,
                                    cmd_to_insert_edge_3, cmd_to_insert_edge_4))
                                    inflight_add(in_flight, in_flight_count, ev_pma);
                            }
                            pool[ev].pma_idx   = pma;
                            pool[ev].worker_id = wid;
                            pool[ev].q_head    = 0;
                            pool[ev].q_tail    = 0;
                            pool[ev].q_count   = 0;
                            slot = ev;
                        }
                    }
                }
            }

            // ==================================================
            // Step 3: Dispatch one batch — longest ready PMA.
            // Only if in_flight table has room.
            // ==================================================
            {
                int best = step3_find_best(pool, in_flight, 1);
                if (best != -1 && in_flight_count < MAX_INFLIGHT_NUM) {
                    int pma = pool[best].pma_idx;
                    if (slot_flush_one_batch(pool, best,
                        cmd_to_insert_edge_1, cmd_to_insert_edge_2,
                        cmd_to_insert_edge_3, cmd_to_insert_edge_4))
                        inflight_add(in_flight, in_flight_count, pma);
                }
            }
        }
    }
}
