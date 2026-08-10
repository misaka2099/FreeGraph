#!/usr/bin/env python3
"""BS insert (rebalance) vs Merge pipeline — HBM IO simulator.

Faithfully replays the two insertion pipelines of the PMA graph accelerator at
the level needed to count HBM IO, given one initialized PMA and a timestamped
batch insert stream. Produces per-case IO totals under two helper-access models.

IO model (see README.md):
  * Raw data (segments / header / VEC_INFO / hbm_buffer):
      contiguous burst  -> 1 IO
      scattered access  -> 1 IO each
      bs segment reads/writes do NOT coalesce across touched segments
      (addresses come from a data-dependent array -> HLS cannot merge).
  * Helpers (FE / VC):
      Model A (helper-burst):  a bs batch's helper traffic costs 1 IO total
          (helpers burst-loaded once onto chip; FE search + VC RMW hit on-chip).
      Model B (helper-independent): every scattered helper word read/write = 1 IO.
      Contiguous helper ranges (rebalance / merge / enlarge) count as bursts in
      both models.
  * ALLOC/FREE counted separately (not mixed into data IO).

Timestamps: batches are processed strictly in order; batch at time t only sees
edges with time <= t (no lookahead) — mirrors the online wave barrier.
"""

import json
import math
import sys
import bisect
import os

# ---- Fixed hardware constants (data_type.h / kernels) ----
VERTEX_PER_PMA = 16
EDGE_SLOTS_PER_SEG = 63
SEG_CAPACITY = 47            # rebalance when segment valid_count > 47 (63*0.75)
SEG_WORDS = 4                # 512-bit words per segment
HEADER_WORDS = 4
FE_ENTRIES_PER_WORD = 16     # SoA: 16 x 32-bit first_edge per 512-bit word
VC_ENTRIES_PER_WORD = 16     # SoA: 16 x 32-bit valid_count per 512-bit word
MAX_ONCHIP_SEG = 1024        # on-chip chunk size (merge / redistribution)
MAX_CMD = 16                 # bs batch size (task_reader -> bs_insert)

def ceil_div(a, b):
    return (a + b - 1) // b

def fe_words_for(segs):
    return ceil_div(segs, FE_ENTRIES_PER_WORD)

def vc_words_for(segs):
    return ceil_div(segs, VC_ENTRIES_PER_WORD)


class IO:
    """Per-run HBM IO counters. 'runs' = contiguous bursts; 'words' = scattered
    single 512-bit accesses (used by the independent-helper model)."""

    def __init__(self):
        self.vecinfo_read = 0
        self.vecinfo_write = 0
        self.header_read = 0
        self.header_write = 0
        self.seg_read = 0            # bursts
        self.seg_write = 0           # bursts
        self.vc_read_runs = 0
        self.vc_write_runs = 0
        self.fe_read_runs = 0
        self.fe_write_runs = 0
        self.vc_read_words = 0       # scattered helper words
        self.vc_write_words = 0
        self.fe_read_words = 0       # scattered helper words
        self.fe_write_words = 0
        self.buf_read = 0            # hbm_buffer bursts
        self.buf_write = 0
        self.alloc = 0               # ALLOC round-trips (cmd + rsp)
        self.free = 0
        # statistics
        self.bs_batches = 0
        self.rebalances = 0
        self.enlarges = 0
        self.merge_vertices = 0

    def totals(self):
        """Return (modelA_burst, modelB_independent) total data IO."""
        base = (self.vecinfo_read + self.vecinfo_write + self.header_read +
                self.header_write + self.seg_read + self.seg_write +
                self.buf_read + self.buf_write)
        helper_runs = (self.vc_read_runs + self.vc_write_runs +
                       self.fe_read_runs + self.fe_write_runs)
        helper_scatter = (self.vc_read_words + self.vc_write_words +
                          self.fe_read_words + self.fe_write_words)
        model_burst = base + helper_runs + self.bs_batches   # 1 helper IO per bs batch
        model_indep = base + helper_runs + helper_scatter
        return model_burst, model_indep

    def merge(self, o):
        for k, v in o.__dict__.items():
            setattr(self, k, getattr(self, k) + v)

    def summary(self):
        a, b = self.totals()
        return {
            "io_modelA_burst": a, "io_modelB_indep": b,
            "seg_read": self.seg_read, "seg_write": self.seg_write,
            "fe_read_runs": self.fe_read_runs, "fe_write_runs": self.fe_write_runs,
            "fe_read_words": self.fe_read_words, "fe_write_words": self.fe_write_words,
            "vc_read_runs": self.vc_read_runs, "vc_write_runs": self.vc_write_runs,
            "vc_read_words": self.vc_read_words, "vc_write_words": self.vc_write_words,
            "header_read": self.header_read, "header_write": self.header_write,
            "vecinfo_read": self.vecinfo_read, "vecinfo_write": self.vecinfo_write,
            "buf_read": self.buf_read, "buf_write": self.buf_write,
            "alloc": self.alloc, "free": self.free,
            "bs_batches": self.bs_batches, "rebalances": self.rebalances,
            "enlarges": self.enlarges, "merge_vertices": self.merge_vertices,
        }


class PMAState:
    """Simulated on-HBM state of one PMA.

    Faithful tracking:
      * vsegs[v]      — segment count per vertex
      * edge_count[v] — total edges per vertex (header.edge_count)
      * vc[seg]       — on-HBM valid_count per segment (SoA)
      * fe_hbm[seg]   — on-HBM first_edge per segment. NOTE: bs insert does NOT
                        write FE (matches bs_insert_kernel.cpp), so fe_hbm goes
                        stale after bs inserts and is only refreshed by
                        rebalance / merge / enlarge.
      * seg_edges[seg]— the actual sorted edge dst values in the segment
                        (used to collect edges for rebalance / merge).
    """

    def __init__(self, layout, max_dst=65536):
        self.max_dst = max_dst
        self.vsegs = list(layout["vsegs"])
        self.edge_count = list(layout["edge_count"])
        self.vc = list(layout["vc"])
        self.fe_hbm = list(layout["fe"])
        self.seg_edges = self._synthesize_edges()
        assert sum(self.vsegs) == len(self.vc) == len(self.fe_hbm) == len(self.seg_edges)
        self.check()

    # ---- index helpers ----
    def vseg_range(self, v):
        vs = sum(self.vsegs[:v])
        return vs, vs + self.vsegs[v]

    def total_segs(self):
        return sum(self.vsegs)

    # ---- initial edge reconstruction ----
    # Given only per-segment fe/vc, synthesize the actual edge values so the
    # sim can collect/merge/re-distribute them later. Each segment gets vc
    # strictly-increasing values spanning [fe, next_boundary) with min == fe,
    # so fe_hbm stays exact for the very first searches.
    def _synthesize_edges(self):
        seg_edges = []
        fe = self.fe_hbm
        for v in range(VERTEX_PER_PMA):
            vs, ve = self.vseg_range(v)
            for s in range(vs, ve):
                vc_s = self.vc[s]
                fe_s = fe[s]
                if vc_s <= 0:
                    seg_edges.append([])
                    continue
                # upper bound: next fe within the vertex, else next vertex's
                # first fe if greater, else max_dst
                ub = self.max_dst
                if s + 1 < ve and fe[s + 1] > fe_s:
                    ub = fe[s + 1]
                else:
                    for v2 in range(v + 1, VERTEX_PER_PMA):
                        vs2, _ = self.vseg_range(v2)
                        if fe[vs2] > fe_s:
                            ub = fe[vs2]
                            break
                span = max(ub - fe_s, vc_s)
                edges = [fe_s + (j * span) // vc_s for j in range(vc_s)]
                seg_edges.append(edges)
        return seg_edges

    # ---- consistency check (debug) ----
    def check(self):
        vs = 0
        for v in range(VERTEX_PER_PMA):
            ve = vs + self.vsegs[v]
            vsum = sum(self.vc[vs:ve])
            esum = sum(len(e) for e in self.seg_edges[vs:ve])
            assert vsum == esum == self.edge_count[v], (v, vsum, esum, self.edge_count[v])
            for s in range(vs, ve):
                assert self.vc[s] == len(self.seg_edges[s]), (s,)
                assert self.vc[s] <= SEG_CAPACITY, (s, self.vc[s])
                # sorted (non-decreasing, duplicates allowed)
                for i in range(1, len(self.seg_edges[s])):
                    assert self.seg_edges[s][i - 1] <= self.seg_edges[s][i]
            vs = ve

    # ---- layout mutations ----
    def set_vertex_segs(self, v, new_vsegs):
        self.vsegs[v] = new_vsegs

    def _resplit_vertex(self, v, merged):
        """Split `merged` (sorted edge list) base+extra across vsegs[v] and
        update vc / fe_hbm / seg_edges / edge_count."""
        vs, ve = self.vseg_range(v)
        vcnt = ve - vs
        total = len(merged)
        base = total // vcnt
        extra = total % vcnt
        idx = 0
        for i in range(vcnt):
            s = vs + i
            fill = base + (1 if i < extra else 0)
            sl = merged[idx:idx + fill]
            idx += fill
            self.vc[s] = fill
            self.seg_edges[s] = sl
            self.fe_hbm[s] = sl[0] if fill else -1
        self.edge_count[v] = total


# ============================================================================
# BS path — faithful replay of bs_insert_kernel + redistribution_kernel
# ============================================================================

def bs_fe_search(state, v, dst):
    """Replay bs_worker's FE binary search. Returns (target_seg, num_FE_reads)."""
    vs, ve = state.vseg_range(v)
    lo, hi = 0, ve - vs - 1
    seg_rel = -1
    reads = 0
    while lo <= hi:
        mid = (lo + hi) // 2
        reads += 1
        fe_val = state.fe_hbm[vs + mid]
        if fe_val == dst:
            seg_rel = mid
            break
        elif fe_val < dst:
            lo = mid + 1
            seg_rel = mid
        else:
            hi = mid - 1
    if seg_rel == -1:
        seg_rel = 0
    return vs + seg_rel, reads


def bs_insert_chunk(state, edges, io):
    """One bs batch (<=16 edges) through bs_worker + insert_phase.

    edges: list of (local_src, dst). Returns list of overflowed segments."""
    io.bs_batches += 1
    io.vecinfo_read += 1                      # bs_worker vi_read (1 scattered word)
    io.header_read += 1                       # bs_worker header (2 consecutive words)

    # --- bs_worker: FE binary search per edge ---
    results = []
    per_seg_dsts = {}
    for (ls, dst) in edges:
        seg, reads = bs_fe_search(state, ls, dst)
        io.fe_read_words += reads             # scattered; feeds independent model only
        results.append((seg, dst))
        per_seg_dsts.setdefault(seg, []).append(dst)

    touched = sorted(per_seg_dsts.keys())

    # --- insert_phase Phase A: burst-read touched segments (no cross-seg coalesce) ---
    io.seg_read += len(touched)
    # --- VC RMW (scattered per touched seg) ---
    io.vc_read_words += len(touched)
    io.vc_write_words += len(touched)

    # --- on-chip seg_insert: update seg_edges + vc ---
    for seg, dsts in per_seg_dsts.items():
        se = state.seg_edges[seg]
        for d in sorted(dsts):
            bisect.insort(se, d)
        state.vc[seg] = len(se)
        # NOTE: FE is NOT written here — bs_insert_kernel.cpp does not touch the
        # FE area on insert, so fe_hbm stays stale until the next rebalance.

    # --- Phase C: burst-write segments ---
    io.seg_write += len(touched)

    # --- header write back ---
    io.header_write += 1
    for (ls, _dst) in edges:
        state.edge_count[ls] += 1

    overflow = [seg for seg in touched if state.vc[seg] > SEG_CAPACITY]
    return overflow


def find_redistribution_range(state, vs, ve, overflow_seg):
    """Replay redistribution's power-of-2 range finder (uses on-HBM VC)."""
    rel = overflow_seg - vs
    left, right = overflow_seg, overflow_seg + 1
    total = state.vc[overflow_seg]
    vertex_segs = ve - vs
    group_size = 2
    while group_size <= vertex_segs * 2:
        if total <= (right - left) * SEG_CAPACITY:
            break
        new_left = vs + (rel // group_size) * group_size
        new_right = min(vs + ((rel // group_size) + 1) * group_size, ve)
        if new_left == left and new_right == right:
            group_size *= 2
            continue
        total += sum(state.vc[new_left:left]) + sum(state.vc[right:new_right])
        left, right = new_left, new_right
        group_size *= 2
    return left, right


def enlarge_pma(state, io, new_vsegs):
    """Whole-PMA move (pre-expand ENLARGE or rebalance ENLARGE)."""
    io.enlarges += 1
    io.alloc += 1
    io.header_write += 1                      # new header (4 words)
    io.fe_write_runs += 1                     # init FE area (contiguous)
    io.vc_write_runs += 1                     # init VC area (contiguous)

    old_vsegs = state.vsegs
    old_total = sum(old_vsegs)
    io.seg_read += ceil_div(old_total, MAX_ONCHIP_SEG)      # collect old PMA
    io.buf_write += ceil_div(old_total, MAX_ONCHIP_SEG)

    old_edges_by_v = [[] for _ in range(VERTEX_PER_PMA)]
    vs = 0
    for v in range(VERTEX_PER_PMA):
        ve = vs + old_vsegs[v]
        for s in range(vs, ve):
            old_edges_by_v[v].extend(state.seg_edges[s])
        vs = ve

    # per-vertex redistribute into new layout
    old_ec = list(state.edge_count)
    for v in range(VERTEX_PER_PMA):
        vcnt = new_vsegs[v]
        if old_ec[v] == 0:
            io.seg_write += 1                 # empty segments (1 contiguous run)
            continue
        nchunks = ceil_div(vcnt, MAX_ONCHIP_SEG)
        io.buf_read += nchunks
        io.seg_write += nchunks
        io.vc_read_runs += nchunks
        io.vc_write_runs += nchunks
        io.fe_read_runs += nchunks
        io.fe_write_runs += nchunks

    io.free += 1
    io.vecinfo_write += 1

    # --- apply new layout ---
    state.vsegs = list(new_vsegs)
    state.vc = [0] * sum(new_vsegs)
    state.fe_hbm = [-1] * sum(new_vsegs)
    state.seg_edges = [[] for _ in range(sum(new_vsegs))]
    for v in range(VERTEX_PER_PMA):
        state.edge_count[v] = old_ec[v]
        if old_ec[v] > 0:
            state._resplit_vertex(v, sorted(old_edges_by_v[v]))


def rebalance_segment(state, io, overflow_seg):
    """redistribution_phase handling of one overflowed segment."""
    # find owner vertex
    owner = -1
    vs = 0
    for v in range(VERTEX_PER_PMA):
        ve = vs + state.vsegs[v]
        if vs <= overflow_seg < ve:
            owner = v
            break
        vs = ve
    if owner < 0:
        return

    vs, ve = state.vseg_range(owner)
    vertex_segs = ve - vs
    io.vc_read_runs += 1                      # read ALL vertex VC (contiguous range)
    vertex_total = sum(state.vc[vs:ve])

    if vertex_total > vertex_segs * SEG_CAPACITY:
        # ENLARGE whole PMA
        new_vsegs = []
        for v in range(VERTEX_PER_PMA):
            if v == owner:
                new_vsegs.append(state.vsegs[v] * 2)
            else:
                new_vsegs.append(state.vsegs[v])
        enlarge_pma(state, io, new_vsegs)
        return

    if vertex_segs <= 1:
        return

    io.rebalances += 1
    left, right = find_redistribution_range(state, vs, ve, overflow_seg)
    io.vc_read_runs += 1                      # range VC read (contiguous)

    group_size = right - left
    # collect edges from group (sort: bs's stale-FE search can misroute an edge
    # into an empty mid-vertex segment, breaking cross-segment order; sorting
    # here models the redistribution result without changing the IO count —
    # base+extra counts are order-independent, only first-edge values change)
    group_edges = []
    for s in range(left, right):
        group_edges.extend(state.seg_edges[s])
    group_edges.sort()
    total_edges = len(group_edges)

    nchunks = ceil_div(group_size, MAX_ONCHIP_SEG)
    io.seg_read += nchunks                    # collect: read group segments
    io.buf_write += nchunks                   # collect: write hbm_buffer
    io.buf_read += nchunks                    # redistribute: read hbm_buffer
    io.seg_write += nchunks                   # redistribute: write segments
    io.vc_read_runs += nchunks
    io.vc_write_runs += nchunks
    io.fe_read_runs += nchunks
    io.fe_write_runs += nchunks

    # base+extra re-split over the SAME group range
    base = total_edges // group_size
    extra = total_edges % group_size
    idx = 0
    for i in range(group_size):
        s = left + i
        fill = base + (1 if i < extra else 0)
        sl = group_edges[idx:idx + fill]
        idx += fill
        state.vc[s] = fill
        state.seg_edges[s] = sl
        state.fe_hbm[s] = sl[0] if fill else -1


def pre_expand(state, count, io):
    """redistribution pma_pre_expand for one wave. count[v] = new edges/vertex.

    Base graph is preloaded (vi != 0), so only the ENLARGE path is possible."""
    io.vecinfo_read += 1
    io.header_read += 1                       # read old layout (2 words)

    need_expand = False
    for v in range(VERTEX_PER_PMA):
        if state.edge_count[v] + count[v] > state.vsegs[v] * SEG_CAPACITY:
            need_expand = True
            break
    if not need_expand:
        return

    new_vsegs = []
    for v in range(VERTEX_PER_PMA):
        if state.edge_count[v] + count[v] > state.vsegs[v] * SEG_CAPACITY:
            # repeated doubling until (ve-vs)*47 >= old+count — the documented
            # count-aware invariant. (real code doubles ONCE; insufficient when
            # a single wave exceeds the vertex's whole capacity — see README)
            nc = state.vsegs[v]
            while state.edge_count[v] + count[v] > nc * SEG_CAPACITY:
                nc *= 2
            new_vsegs.append(nc)
        else:
            new_vsegs.append(state.vsegs[v])
    enlarge_pma(state, io, new_vsegs)


def run_bs(state, batches):
    """Run the full bs pipeline over a timestamped batch list."""
    io = IO()
    for batch in batches:
        edges = batch["edges"]
        count = [0] * VERTEX_PER_PMA
        for (ls, _dst) in edges:
            count[ls] += 1
        pre_expand(state, count, io)
        # bs: process in chunks of MAX_CMD, rebalancing overflowed segs per chunk
        for i in range(0, len(edges), MAX_CMD):
            chunk = edges[i:i + MAX_CMD]
            overflow = bs_insert_chunk(state, chunk, io)
            for seg in overflow:
                rebalance_segment(state, io, seg)
    return io


# ============================================================================
# Merge path — faithful replay of merge_pipeline merge_phase
# ============================================================================

def merge_wave(state, edges, io):
    """One whole-PMA batch through merge_phase."""
    io.vecinfo_read += 1
    io.header_read += 1

    per_v = {}
    for (ls, dst) in edges:
        per_v.setdefault(ls, []).append(dst)

    header_changed = False
    for v in sorted(per_v):
        dsts = sorted(per_v[v])
        vs, ve = state.vseg_range(v)
        vcnt = ve - vs
        if vcnt <= 0:
            continue
        io.merge_vertices += 1
        nchunks = ceil_div(vcnt, MAX_ONCHIP_SEG)

        io.seg_read += nchunks                # burst-read ALL vertex segments
        existing = []
        for s in range(vs, ve):
            existing.extend(state.seg_edges[s])
        merged = sorted(existing + dsts)

        io.buf_write += 1                     # write merged edges to hbm_buffer (1 run)
        # redistribute_edge_from_hbm over the whole vertex
        io.buf_read += nchunks
        io.seg_write += nchunks
        io.vc_read_runs += nchunks
        io.vc_write_runs += nchunks
        io.fe_read_runs += nchunks
        io.fe_write_runs += nchunks

        state._resplit_vertex(v, merged)
        header_changed = True

    if header_changed:
        io.header_write += 1                  # header.edge_count write back


def run_merge(state, batches):
    io = IO()
    for batch in batches:
        edges = batch["edges"]
        count = [0] * VERTEX_PER_PMA
        for (ls, _dst) in edges:
            count[ls] += 1
        pre_expand(state, count, io)
        merge_wave(state, edges, io)
    return io


# ============================================================================
# Policy-2 (default GATE_POLICY): count>16 -> merge, else bs
# ============================================================================

def run_policy2(state, batches):
    io = IO()
    for batch in batches:
        edges = batch["edges"]
        count = [0] * VERTEX_PER_PMA
        for (ls, _dst) in edges:
            count[ls] += 1
        pre_expand(state, count, io)
        if len(edges) > 16:
            merge_wave(state, edges, io)
        else:
            overflow = bs_insert_chunk(state, edges, io)
            for seg in overflow:
                rebalance_segment(state, io, seg)
    return io


# ============================================================================
# Loading & CLI
# ============================================================================

def load_pma(path):
    with open(path) as f:
        d = json.load(f)
    return d

def load_inserts(path):
    """Load insert stream; convert edge dicts {src,dst} -> (src, dst) tuples."""
    with open(path) as f:
        d = json.load(f)
    batches = []
    for b in d["batches"]:
        batches.append({"time": b["time"],
                        "edges": [(e["src"], e["dst"]) for e in b["edges"]]})
    return batches

POLICIES = {"bs": run_bs, "merge": run_merge, "policy2": run_policy2}


def run_case(pma_file, insert_file, policy):
    pma = load_pma(pma_file)
    batches = load_inserts(insert_file)
    state = PMAState(pma["initial_layout"], max_dst=pma.get("max_dst", 65536))
    run = POLICIES[policy]
    io = run(state, batches)
    state.check()
    return io


def main():
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument("pma_file")
    ap.add_argument("insert_file")
    ap.add_argument("--policy", choices=list(POLICIES), default="bs")
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args()

    io = run_case(args.pma_file, args.insert_file, args.policy)
    if args.json:
        print(json.dumps(io.summary()))
    else:
        s = io.summary()
        print("== IO summary ==")
        for k, v in s.items():
            print(f"  {k:22s} {v}")
        print(f"  modelA_burst={s['io_modelA_burst']}  modelB_indep={s['io_modelB_indep']}")


if __name__ == "__main__":
    main()
