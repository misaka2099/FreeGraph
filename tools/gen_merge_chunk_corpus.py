#!/usr/bin/env python3
"""Generate a deterministic test corpus for tools/test_merge_chunk.cpp.

The corpus exercises merge_pipeline.cpp's per-vertex chunked merge:
  existing segments (S segments, each up to 47 valid edges) + new_edges
  two-pointer merge -> redistribute base+extra across segments.

Format (free-form whitespace, one case after another):
    S nidx
    vc[0..S-1]               (S ints)
    seg0 edges (vc[0] ints)
    seg1 edges (vc[1] ints)
    ...
    seg(S-1) edges
    new_edges (nidx ints, sorted)

Every emitted case is re-validated against the PMA invariants the harness
enforces; a candidate that fails is retried (fresh RNG draws) a few times and
otherwise discarded.  Deterministic: a fixed master seed drives per-case RNGs.

Run:  python3 tools/gen_merge_chunk_corpus.py
Writes: tools/merge_chunk_corpus.txt
"""

import os
import random
import sys

# ---- constants (must mirror PMA / harness) ----
MAX_VC = 47                # per-segment valid-count cap (PMA invariant)
CAP_PER_SEG = 47           # pre-expand capacity: S * 47
CHUNK = 1024               # MAX_ONCHIP_SEG (block size)
MAX_NEW_EDGES = 8192       # kernel new_edges array bound
INT_NEAR_MAX = 2**31 - 5   # near-INT_MAX test values
EDGE_HI_DEFAULT = 2**28    # recommended edge value ceiling

OUT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                        "merge_chunk_corpus.txt")
SEED_BASE = 0x5EEDC0DE
TARGET_CASES = 500
MAX_RETRIES = 30


def validate(existing, new_edges):
    """True iff the case satisfies every PMA invariant the harness checks.

    Mirror of test_merge_chunk.cpp run_case() input validation:
      (a) each segment sorted, vc <= 47
      (b) vertex-wide global order (segment s max <= segment s+1 min)
      (c) old_edges + nidx <= S*47  (pre-expand capacity)
      (d) new_edges sorted
    """
    S = len(existing)
    old_edges = 0
    last_max = -1
    for es in existing:
        if len(es) > MAX_VC:
            return False
        for k in range(1, len(es)):
            if es[k] < es[k - 1]:
                return False
        if es:
            if es[0] < last_max:
                return False
            last_max = es[-1]
            old_edges += len(es)
    for k in range(1, len(new_edges)):
        if new_edges[k] < new_edges[k - 1]:
            return False
    if old_edges + len(new_edges) > S * CAP_PER_SEG:
        return False
    return True


# ---- value / segment construction helpers ----
def gen_sorted_pool(rng, n, lo, hi, dup_prob=0.0):
    """n non-decreasing values in [lo,hi]; dup_prob ~ repeats the prev value."""
    pool = []
    prev = None
    for _ in range(n):
        if prev is not None and dup_prob > 0 and rng.random() < dup_prob:
            pool.append(prev)
        else:
            prev = rng.randint(lo, hi)
            pool.append(prev)
    pool.sort()
    return pool


def distribute_pool(pool, S, rng, empty_prob=0.0, max_vc=MAX_VC):
    """Slice a sorted pool into S segments keeping global order + per-seg cap.

    Guarantees: each segment <= max_vc edges, pool kept fully sorted across
    segment boundaries (front(s+1) >= back(s)).  Returns list of lists, or
    None if the pool cannot fit (pool > S*max_vc).
    """
    n = len(pool)
    if S == 0:
        return [] if n == 0 else None
    if n > S * max_vc:
        return None
    counts = [0] * S
    remaining = n
    for s in range(S):
        if remaining == 0:
            break
        segs_left = S - s
        max_take = min(max_vc, remaining)
        min_take = max(1, remaining - (segs_left - 1) * max_vc)
        if rng.random() < empty_prob and remaining <= (segs_left - 1) * max_vc:
            counts[s] = 0
        else:
            counts[s] = rng.randint(min_take, max_take)
            remaining -= counts[s]
    assert remaining == 0, "pool must fit"
    segs = []
    p = 0
    for c in counts:
        segs.append(pool[p:p + c])
        p += c
    assert p == n
    return segs


def gen_new_edges(rng, nidx, lo, hi, dup_prob=0.0, values=None):
    """nidx non-decreasing new_edges in [lo,hi] (or drawn from `values`)."""
    if nidx <= 0:
        return []
    edges = []
    prev = None
    for _ in range(nidx):
        if values is not None:
            v = values[rng.randrange(len(values))]
        else:
            if prev is not None and dup_prob > 0 and rng.random() < dup_prob:
                v = prev
            else:
                v = rng.randint(lo, hi)
                prev = v
        edges.append(v)
    edges.sort()
    return edges


# ---- generic case builder ----
def build_generic(rng, S, nidx, pool_lo, pool_hi, new_lo, new_hi, empty_prob,
                  max_vc=MAX_VC, new_dup_prob=0.0, pool_dup_prob=0.0,
                  force_capacity=False, new_values=None, fill47=False,
                  old_override=None, old_max=None):
    """Build (existing, new_edges).  Returns None if not constructible."""
    capacity = S * CAP_PER_SEG
    nidx = max(0, min(nidx, capacity, MAX_NEW_EDGES))
    max_old = capacity - nidx
    if force_capacity:
        old = max_old
    elif old_override is not None:
        old = min(max(0, old_override), max_old)
    else:
        hi = max_old if old_max is None else min(max_old, old_max)
        old = rng.randint(0, hi)

    pool = gen_sorted_pool(rng, old, pool_lo, pool_hi, dup_prob=pool_dup_prob)

    if fill47:
        # pack as many exactly-47 segments as possible, spread the rest
        segs = [[] for _ in range(S)]
        full_count = min(old // MAX_VC, S)
        p = 0
        for s in range(full_count):
            segs[s] = pool[p:p + MAX_VC]
            p += MAX_VC
        rem = old - p
        if rem > 0:
            if S - full_count <= 0:
                return None
            sub = distribute_pool(pool[p:], S - full_count, rng,
                                  empty_prob=empty_prob, max_vc=max_vc)
            if sub is None:
                return None
            for s in range(S - full_count):
                segs[full_count + s] = sub[s]
    else:
        segs = distribute_pool(pool, S, rng, empty_prob=empty_prob, max_vc=max_vc)
        if segs is None:
            return None

    if new_values is not None:
        new_edges = gen_new_edges(rng, nidx, new_lo, new_hi, values=new_values)
    else:
        new_edges = gen_new_edges(rng, nidx, new_lo, new_hi, dup_prob=new_dup_prob)
    return segs, new_edges


# ---- per-category builders ----
def builder_chunk_boundary(S):
    """S exactly around a 1024-seg block boundary."""
    def b(rng):
        capacity = S * CAP_PER_SEG
        nidx = rng.randint(0, min(4000, capacity))
        empty_prob = 0.6 if S >= 3072 else 0.25
        return build_generic(rng, S, nidx, 0, EDGE_HI_DEFAULT, 0, EDGE_HI_DEFAULT,
                             empty_prob, new_dup_prob=rng.choice([0.0, 0.4]),
                             pool_dup_prob=rng.choice([0.0, 0.2]), old_max=80000)
    return b


def builder_empty_chunks(S, empty_chunks, nidx_hi=3000):
    """Whole 1024-seg chunks left completely empty; others populated."""
    def b(rng):
        capacity = S * CAP_PER_SEG
        nidx = rng.randint(0, min(nidx_hi, capacity))
        empty_set = set()
        for c in empty_chunks:
            for s in range(c * CHUNK, min(S, (c + 1) * CHUNK)):
                empty_set.add(s)
        fillable = [s for s in range(S) if s not in empty_set]
        max_old = min(capacity - nidx, len(fillable) * MAX_VC)
        old = rng.randint(0, max_old)
        pool = gen_sorted_pool(rng, old, 0, EDGE_HI_DEFAULT,
                               dup_prob=rng.choice([0.0, 0.2]))
        segs = [[] for _ in range(S)]
        sub = distribute_pool(pool, len(fillable), rng, empty_prob=0.2, max_vc=MAX_VC)
        if sub is None:
            return None
        for s, v in zip(fillable, sub):
            segs[s] = v
        new_edges = gen_new_edges(rng, nidx, 0, EDGE_HI_DEFAULT,
                                  dup_prob=rng.choice([0.0, 0.3]))
        return segs, new_edges
    return b


def builder_vc47():
    """Many segments packed to exactly vc=47 (at full capacity)."""
    def b(rng):
        S = rng.randint(8, 512)
        capacity = S * CAP_PER_SEG
        nidx = rng.randint(0, min(2000, capacity))
        return build_generic(rng, S, nidx, 0, EDGE_HI_DEFAULT, 0, EDGE_HI_DEFAULT,
                             empty_prob=0.0, fill47=True, force_capacity=True)
    return b


def builder_vc47_notfull():
    """Several exactly-47 segments but not at full capacity."""
    def b(rng):
        S = rng.randint(8, 512)
        capacity = S * CAP_PER_SEG
        nidx = rng.randint(0, min(1000, capacity))
        max_old = capacity - nidx
        return build_generic(rng, S, nidx, 0, EDGE_HI_DEFAULT, 0, EDGE_HI_DEFAULT,
                             empty_prob=0.0, fill47=True,
                             old_override=rng.randint(int(max_old * 0.7), max_old))
    return b


def builder_dup_new_internal():
    """Repeated values inside new_edges."""
    def b(rng):
        S = rng.randint(16, 2000)
        capacity = S * CAP_PER_SEG
        nidx = rng.randint(100, min(4000, capacity))
        return build_generic(rng, S, nidx, 0, 1000, 0, 1000, empty_prob=0.2,
                             new_dup_prob=0.9)
    return b


def builder_dup_existing_internal():
    """Repeated values inside existing segments."""
    def b(rng):
        S = rng.randint(16, 2000)
        capacity = S * CAP_PER_SEG
        nidx = rng.randint(0, min(2000, capacity))
        max_old = capacity - nidx
        return build_generic(rng, S, nidx, 0, 1000, 0, 1000, empty_prob=0.2,
                             pool_dup_prob=0.9,
                             old_override=rng.randint(max_old // 3, max_old))
    return b


def builder_new_equals_existing():
    """new_edges drawn from the exact existing edge values."""
    def b(rng):
        S = rng.randint(16, 2000)
        capacity = S * CAP_PER_SEG
        nidx = rng.randint(1, min(4000, capacity))
        max_old = capacity - nidx
        old = rng.randint(max_old // 3, max_old)
        pool = gen_sorted_pool(rng, old, 0, EDGE_HI_DEFAULT, dup_prob=0.2)
        segs = distribute_pool(pool, S, rng, empty_prob=0.2)
        if segs is None:
            return None
        flat = [e for es in segs for e in es]
        if not flat:
            flat = [0]
        new_edges = gen_new_edges(rng, nidx, 0, EDGE_HI_DEFAULT, values=flat)
        return segs, new_edges
    return b


def builder_new_less_than_existing():
    def b(rng):
        S = rng.randint(16, 2000)
        capacity = S * CAP_PER_SEG
        nidx = rng.randint(1, min(4000, capacity))
        max_old = capacity - nidx
        return build_generic(rng, S, nidx, 1000, 3000, 0, 999, empty_prob=0.15,
                             old_override=rng.randint(max_old // 3, max_old))
    return b


def builder_new_greater_than_existing():
    def b(rng):
        S = rng.randint(16, 2000)
        capacity = S * CAP_PER_SEG
        nidx = rng.randint(1, min(4000, capacity))
        max_old = capacity - nidx
        return build_generic(rng, S, nidx, 0, 1000, 5000, 9000, empty_prob=0.15,
                             old_override=rng.randint(max_old // 3, max_old))
    return b


def builder_interleaved():
    """new and existing values drawn from the same range -> interleaved."""
    def b(rng):
        S = rng.randint(16, 2000)
        capacity = S * CAP_PER_SEG
        nidx = rng.randint(1, min(4000, capacity))
        max_old = capacity - nidx
        return build_generic(rng, S, nidx, 0, 100000, 0, 100000, empty_prob=0.2,
                             old_override=rng.randint(max_old // 3, max_old),
                             new_dup_prob=0.3)
    return b


def builder_nidx0():
    """No new edges: pure redistribution of existing edges."""
    def b(rng):
        S = rng.choice([rng.randint(1, 100),
                        rng.randint(100, 2048),
                        rng.randint(2048, 5000)])
        return build_generic(rng, S, 0, 0, EDGE_HI_DEFAULT, 0, EDGE_HI_DEFAULT,
                             empty_prob=rng.choice([0.0, 0.3, 0.7]),
                             pool_dup_prob=rng.choice([0.0, 0.2]))
    return b


def builder_nidx_large():
    """nidx in [7000, 8192]."""
    def b(rng):
        S = rng.randint(200, 5000)
        capacity = S * CAP_PER_SEG
        nidx = rng.randint(7000, min(MAX_NEW_EDGES, capacity))
        return build_generic(rng, S, nidx, 0, EDGE_HI_DEFAULT, 0, EDGE_HI_DEFAULT,
                             empty_prob=0.5, old_max=20000)
    return b


def builder_full_capacity():
    """total edges == S*47 exactly."""
    def b(rng):
        S = rng.choice([4, 8, 16, 32, 64, 100, 128, 200])
        capacity = S * CAP_PER_SEG
        nidx = rng.randint(0, min(MAX_NEW_EDGES, capacity))
        return build_generic(rng, S, nidx, 0, EDGE_HI_DEFAULT, 0, EDGE_HI_DEFAULT,
                             empty_prob=0.0, force_capacity=True)
    return b


def builder_s1():
    """Single-segment vertex."""
    def b(rng):
        S = 1
        capacity = CAP_PER_SEG
        nidx = rng.randint(0, capacity)
        return build_generic(rng, S, nidx, 0, EDGE_HI_DEFAULT, 0, EDGE_HI_DEFAULT,
                             empty_prob=0.0, new_dup_prob=0.3)
    return b


def builder_near_intmax():
    """Edge values close to INT_MAX (2^31-5)."""
    def b(rng):
        S = rng.randint(16, 2000)
        capacity = S * CAP_PER_SEG
        nidx = rng.randint(0, min(3000, capacity))
        hi = INT_NEAR_MAX
        return build_generic(rng, S, nidx, hi - 100000, hi, hi - 50000, hi,
                             empty_prob=rng.choice([0.0, 0.3]),
                             new_dup_prob=rng.choice([0.0, 0.3]),
                             pool_dup_prob=rng.choice([0.0, 0.2]))
    return b


def builder_random_large():
    """Random big vertices: S in [2000,5000], nidx in [1000,8000]."""
    def b(rng):
        S = rng.randint(2000, 5000)
        capacity = S * CAP_PER_SEG
        nidx = rng.randint(1000, min(MAX_NEW_EDGES, capacity))
        return build_generic(rng, S, nidx, 0, EDGE_HI_DEFAULT, 0, EDGE_HI_DEFAULT,
                             empty_prob=rng.choice([0.4, 0.6, 0.8]),
                             new_dup_prob=0.2, pool_dup_prob=0.1, old_max=12000)
    return b


def builder_random():
    """Generic random fill: mix of vertex sizes / densities."""
    def b(rng):
        S = rng.choice([rng.randint(1, 100),
                        rng.randint(100, 1024),
                        rng.randint(1024, 2048),
                        rng.randint(2048, 5000)])
        capacity = S * CAP_PER_SEG
        nidx = rng.randint(0, min(MAX_NEW_EDGES, capacity))
        if rng.random() < 0.03:
            hi = INT_NEAR_MAX
            pool_lo, new_lo = hi - 200000, hi - 200000
        else:
            hi, pool_lo, new_lo = EDGE_HI_DEFAULT, 0, 0
        empty_prob = rng.choice([0.3, 0.5, 0.7, 0.85])
        old_max = 12000 if S >= 2048 else None
        return build_generic(rng, S, nidx, pool_lo, hi, new_lo, hi, empty_prob,
                             new_dup_prob=rng.choice([0.0, 0.3]),
                             pool_dup_prob=rng.choice([0.0, 0.2]), old_max=old_max)
    return b


# ---- plan assembly ----
def build_plan():
    plan = []
    # S crossing 1024-seg block boundaries (3 cases each)
    for S in [1023, 1024, 1025, 2047, 2048, 2049, 3072, 4096, 5000]:
        for _ in range(3):
            plan.append(("chunk_boundary_S%d" % S, builder_chunk_boundary(S)))
    # sparse vertices: whole 1024-seg chunks empty
    plan.append(("sparse_empty_chunk", builder_empty_chunks(3000, [1])))       # chunks 0+2 populated, chunk1 empty
    plan.append(("sparse_empty_chunk", builder_empty_chunks(3000, [0, 2])))    # only chunk1 populated
    plan.append(("sparse_empty_chunk", builder_empty_chunks(2048, [0])))
    plan.append(("sparse_empty_chunk", builder_empty_chunks(2048, [1])))
    plan.append(("sparse_empty_chunk", builder_empty_chunks(1024, [0])))
    plan.append(("sparse_empty_chunk", builder_empty_chunks(5000, [1, 3])))    # populated chunks 0,2,4
    # vc=47 full segments
    plan.append(("vc47_full", builder_vc47()))
    plan.append(("vc47_full", builder_vc47()))
    plan.append(("vc47_full", builder_vc47_notfull()))
    plan.append(("vc47_full", builder_vc47_notfull()))
    # duplicates
    plan.append(("dup_new_internal", builder_dup_new_internal()))
    plan.append(("dup_new_internal", builder_dup_new_internal()))
    plan.append(("dup_new_internal", builder_dup_new_internal()))
    plan.append(("dup_existing_internal", builder_dup_existing_internal()))
    plan.append(("dup_existing_internal", builder_dup_existing_internal()))
    plan.append(("dup_existing_internal", builder_dup_existing_internal()))
    plan.append(("new_equals_existing", builder_new_equals_existing()))
    plan.append(("new_equals_existing", builder_new_equals_existing()))
    plan.append(("new_equals_existing", builder_new_equals_existing()))
    # new_edges relations
    plan.append(("new_all_less", builder_new_less_than_existing()))
    plan.append(("new_all_less", builder_new_less_than_existing()))
    plan.append(("new_all_less", builder_new_less_than_existing()))
    plan.append(("new_all_greater", builder_new_greater_than_existing()))
    plan.append(("new_all_greater", builder_new_greater_than_existing()))
    plan.append(("new_all_greater", builder_new_greater_than_existing()))
    plan.append(("interleaved", builder_interleaved()))
    plan.append(("interleaved", builder_interleaved()))
    plan.append(("interleaved", builder_interleaved()))
    # nidx extremes
    plan.append(("nidx0", builder_nidx0()))
    plan.append(("nidx0", builder_nidx0()))
    plan.append(("nidx0", builder_nidx0()))
    plan.append(("nidx0", builder_nidx0()))
    plan.append(("nidx_large", builder_nidx_large()))
    plan.append(("nidx_large", builder_nidx_large()))
    plan.append(("nidx_large", builder_nidx_large()))
    plan.append(("nidx_large", builder_nidx_large()))
    # full capacity == S*47
    plan.append(("full_capacity", builder_full_capacity()))
    plan.append(("full_capacity", builder_full_capacity()))
    plan.append(("full_capacity", builder_full_capacity()))
    plan.append(("full_capacity", builder_full_capacity()))
    # single segment
    plan.append(("S1", builder_s1()))
    plan.append(("S1", builder_s1()))
    plan.append(("S1", builder_s1()))
    plan.append(("S1", builder_s1()))
    # near-INT_MAX values
    plan.append(("near_intmax", builder_near_intmax()))
    plan.append(("near_intmax", builder_near_intmax()))
    plan.append(("near_intmax", builder_near_intmax()))
    plan.append(("near_intmax", builder_near_intmax()))
    # random big vertices
    for _ in range(40):
        plan.append(("random_large", builder_random_large()))
    # generic random fill to reach TARGET_CASES
    for _ in range(382):
        plan.append(("random_fill", builder_random()))
    return plan


def make_rng(case_index):
    """Deterministic per-case RNG (distinct seed per case)."""
    x = (SEED_BASE ^ (case_index * 0x9E3779B1)) & 0xFFFFFFFF
    x ^= x >> 16
    x = (x * 0x7FEB352D) & 0xFFFFFFFF
    x ^= x >> 15
    x = (x * 0x846CA68B) & 0xFFFFFFFF
    x ^= x >> 16
    return random.Random(x)


def main():
    plan = build_plan()
    cases = []                # (category, existing, new_edges)
    counts = {}
    discarded = 0
    attempt = 0

    queue = list(plan)
    while len(cases) < TARGET_CASES and queue:
        cat, builder = queue.pop(0)
        rng = make_rng(attempt)
        attempt += 1
        got = None
        for _ in range(MAX_RETRIES):
            res = builder(rng)
            if res is None:
                continue
            existing, new_edges = res
            if validate(existing, new_edges):
                got = (existing, new_edges)
                break
        if got is None:
            discarded += 1
            continue
        cases.append((cat, got[0], got[1]))
        counts[cat] = counts.get(cat, 0) + 1

    # extra fill if a few category cases got discarded
    while len(cases) < TARGET_CASES and attempt < len(plan) + 500:
        rng = make_rng(attempt)
        attempt += 1
        got = None
        for _ in range(MAX_RETRIES):
            res = builder_random()(rng)
            if res is None:
                continue
            existing, new_edges = res
            if validate(existing, new_edges):
                got = (existing, new_edges)
                break
        if got is None:
            discarded += 1
            continue
        cases.append(("random_fill_extra", got[0], got[1]))
        counts["random_fill_extra"] = counts.get("random_fill_extra", 0) + 1

    # ---- write corpus ----
    with open(OUT_PATH, "w") as f:
        for cat, existing, new_edges in cases:
            S = len(existing)
            nidx = len(new_edges)
            f.write("%d %d\n" % (S, nidx))
            f.write(" ".join(str(len(es)) for es in existing) + "\n")
            for es in existing:
                if es:
                    f.write(" ".join(str(e) for e in es) + "\n")
                else:
                    f.write("\n")
            if new_edges:
                f.write(" ".join(str(e) for e in new_edges) + "\n")
            else:
                f.write("\n")

    size_mb = os.path.getsize(OUT_PATH) / (1024 * 1024)

    # ---- report (stderr) ----
    sys.stderr.write("corpus cases generated: %d\n" % len(cases))
    sys.stderr.write("discarded (failed validation after retries): %d\n" % discarded)
    sys.stderr.write("corpus file: %s (%.1f MB)\n" % (OUT_PATH, size_mb))
    for cat in sorted(counts):
        sys.stderr.write("  %-24s %4d\n" % (cat, counts[cat]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
