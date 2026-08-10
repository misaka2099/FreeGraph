#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
exp_pma_gen.py — 生成已初始化的单 PMA 布局 JSON

用于 BS insert vs Merge 的 HBM-IO 仿真实验。
生成 12 个 PMA (2 vertex_dist × 2 edge_dist × 3 scale)，
每个文件描述一个已按 PMA "INIT" 规则初始化的单 PMA 布局
(vsegs / edge_count / fe / vc)，供 bs_vs_merge_sim.py 读取。

用法:
    python3 exp_pma_gen.py --all --out DIR [--seed N] [--max-dst N]
    python3 exp_pma_gen.py --vertex-dist {uniform,skewed} --edge-dist {uniform,skewed} \
                           --scale {small,medium,large} [--seed N] [--max-dst N] [--out DIR]

布局语义:
    - VERTEX_PER_PMA=16, 每顶点 dst ∈ [0, max_dst)
    - 每 segment 最多 47 条边 (EDGE_SLOTS_PER_SEG=63 × DENSITY_BOUND=0.75)
    - vsegs 由 INIT 公式决定: need = ceil(count/47); need<=1 → 1, 否则上取整到 2 的幂
    - 段内边按 base+extra 分配: base = count//segs, extra = count%segs;
      前 extra 段每段 base+1 条, 其余 base 条 (有序边数组的连续切片)
    - fe = 段内最小 dst, 空段 = -1; vc = 段内边数
    - 允许重复 dst, 绝不去重 (保留抽样原始值)

关于 "fe 严格递增" 与 "允许重复 dst" 的一致性:
    顶点边升序后按 base+extra 连续切片, 若某值重复跨段, 切片首元素 (fe) 就会重复。
    为保证校验规则 "顶点内 fe 严格递增" 恒成立, 生成时按段构造:
      1) 先从 edge_dist 分布采样 vs 个段起始候选值 → 排序 → 上推去重 → 得到严格递增的 fe;
      2) 每段首条边 = fe[s] (保证段内 min==fe), 其余边在 [fe[s], next_fe) 内采样。
    这样拼接出的边数组本身有序, 且 base+extra 切分恰好得到这些段 —— 输出布局与
    "排序后连续切片" 语义完全一致, 只是值的选取保证了段边界不重复。
    段内允许任意重复 (不做去重)。

随机种子说明:
    每个文件用 fresh random.Random(seed) 生成, config.seed 即所用种子。
    vertex 计数的幂律权重是手工计算的 (与 numpy 无关)。
    skewed 的段边界候选值用 1/x 密度连续逆变换采样 (小 dst 更频繁, 强烈左偏);
    若系统装了 numpy, 用独立的 numpy Generator (种子 = seed^常量) 提供均匀样本,
    否则回退 random.Random —— 两条路径分布相同, 仅随机流来源不同。
"""

import argparse
import json
import os
import random
import sys

# numpy 可选: 有则用它提供随机流, 无则回退纯 Python
try:
    import numpy as np
    HAVE_NUMPY = True
except ImportError:                      # pragma: no cover
    HAVE_NUMPY = False

# ---------------------------- 固定常量 ----------------------------

VERTEX_PER_PMA = 16                 # 每 PMA 覆盖 16 个顶点 (src 0..15)
VERTEX_INDICES = list(range(VERTEX_PER_PMA))
EDGE_SLOTS_PER_SEG = 63             # 每 segment 的 512-bit 槽位数 (16×32-bit entry)
DENSITY_BOUND = 0.75                # 密度上界 → 每段最多 47 条边
EDGE_CAP_PER_SEG = int(EDGE_SLOTS_PER_SEG * DENSITY_BOUND)   # 47

VERTEX_DISTS = ["uniform", "skewed"]
EDGE_DISTS = ["uniform", "skewed"]
SCALES = ["small", "medium", "large"]

# 每 scale 的顶点平均边数 (README §4: small~30-80 / medium~500-1500 / large~4000-12000)
SCALE_AVG_EDGES = {"small": 60, "medium": 1000, "large": 8000}

DEFAULT_MAX_DST = 65536             # dst 默认取值范围 [0, max_dst)
DEFAULT_SEED = 42

# 默认输出目录: tools/exp/data/pma (相对脚本文件定位, 与 CWD 无关)
DEFAULT_OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                               "data", "pma")


# ---------------------------- INIT / 切分工具 ----------------------------

def vsegs_for_count(count):
    """PMA INIT 公式: need = ceil(count/47); need<=1 → 1, 否则上取整到 2 的幂。

    空顶点 (count==0) 也占 1 段 (fe=-1, vc=0), 与 pma_gate 初始化一致。
    """
    if count <= 0:
        return 1
    need = (count + EDGE_CAP_PER_SEG - 1) // EDGE_CAP_PER_SEG   # ceil(count/47)
    if need <= 1:
        return 1
    # 上取整到 2 的幂: need=2→2, 3→4, 4→4, 5→8, ...
    return 1 << (need - 1).bit_length()


# ---------------------------- 分布工具 ----------------------------

def one_over_x_from_u(u, max_dst):
    """把 [0,1) 均匀样本 u 经 1/x 密度连续逆变换映射到 [0, max_dst)。

    x = max_dst^u ∈ [1, max_dst), 密度 ∝ 1/x → 小 dst 远更频繁 (强烈左偏)。
    """
    if max_dst <= 1:
        return 0
    x = float(max_dst) ** float(u)
    v = int(x)
    if v >= max_dst:
        v = max_dst - 1
    return max(v - 1, 0)


def one_over_x_in_range_from_u(u, lo, hi):
    """1/x 密度在 [lo, hi) 内的样本 (偏向左端 lo); 宽<=1 时只有 lo。"""
    span = hi - lo
    if span <= 1:
        return lo
    v = lo + int(float(span) ** float(u)) - 1
    if v < lo:
        v = lo
    if v >= hi:
        v = hi - 1
    return v


def make_boundary_sampler(rng, edge_dist, max_dst, seed):
    """返回无参闭包, 每次返回一个 [0, max_dst) 内的段起始候选值。

    uniform: 均匀整数。skewed: 1/x 密度 (强烈左偏, 小 dst 更频繁)。
    """
    if edge_dist == "uniform":
        return lambda: rng.randrange(max_dst)

    if edge_dist == "skewed":
        if HAVE_NUMPY:
            # 独立的 numpy Generator, 种子由 config seed 派生
            gen = np.random.default_rng((seed ^ 0x51ED270B) & 0xFFFFFFFF)
            return lambda: one_over_x_from_u(float(gen.random()), max_dst)
        return lambda: one_over_x_from_u(rng.random(), max_dst)

    raise ValueError("unknown edge_dist: %s" % edge_dist)


def gen_vertex_counts(rng, vertex_dist, scale):
    """生成 16 个顶点的初始边数 (列表, 长度 16)。

    uniform: 全部顶点接近 SCALE_AVG_EDGES[scale] (±10% 抖动)。
    skewed : 手工幂律权重 1/(rank+1), rank 0 最热 → 热顶点 ≈ 4-5x 均值;
             小规模下最冷顶点 (rank>=12) 允许少量留空 (空顶点少见)。
    """
    target = SCALE_AVG_EDGES[scale]
    counts = []
    if vertex_dist == "uniform":
        for _v in range(VERTEX_PER_PMA):
            jitter = rng.uniform(-0.1, 0.1)          # ±10% 抖动
            c = int(round(target * (1.0 + jitter)))
            counts.append(max(1, c))                  # 至少 1 条边
        return counts

    # skewed: 幂律权重 w_r = 1/(r+1), r=0..15
    weights = [1.0 / (r + 1) for r in range(VERTEX_PER_PMA)]
    total_weight = sum(weights)
    for v in range(VERTEX_PER_PMA):
        jitter = rng.uniform(0.85, 1.15)              # 轻微抖动, 避免过于整齐
        c = int(round(target * VERTEX_PER_PMA * weights[v] / total_weight * jitter))
        counts.append(max(1, c))                      # 热/小顶点默认至少 1 条
    # 小规模倾斜: 最冷顶点 (rank>=12) 允许少量为空 (概率小 → "rare")
    if scale == "small":
        for v in range(12, VERTEX_PER_PMA):
            if rng.random() < 0.2:
                counts[v] = 0
    return counts


def _sample_in_range(rng, lo, hi, edge_dist):
    """在 [lo, hi) 内采一个边值。uniform: 均匀; skewed: 段内 1/x 偏左。"""
    if hi - lo <= 1:
        return lo
    if edge_dist == "uniform":
        return rng.randrange(lo, hi)
    return one_over_x_in_range_from_u(rng.random(), lo, hi)


def gen_vertex_layout(rng, count_v, vs, max_dst, edge_dist, sample_boundary):
    """生成单个顶点的段起始值 fe 与段计数 vc (段大小按 base+extra)。

    空顶点 (count_v==0) → fe=[-1], vc=[0]。
    fe 通过 "采样→排序→上推" 保证严格递增 (排序数组连续切片的首元素)。
    返回 (fe_list, vc_list)。
    """
    if count_v <= 0:
        return [-1], [0]

    base = count_v // vs
    extra = count_v % vs

    # 1) 段起始候选值: vs 个采样 → 排序 → 上推去重 → 严格递增的 fe
    candidates = sorted(sample_boundary() for _ in range(vs))
    fe = []
    cur = -1
    for c in candidates:
        if c <= cur:
            c = cur + 1                     # 上推保证严格递增
        if c >= max_dst:
            c = max_dst - 1                 # 钳制到合法范围
        if c < 0:
            c = 0
        if c <= cur:                        # max_dst 过小无法递增时的兜底 (保持非降)
            c = cur
        fe.append(c)
        cur = c

    # 2) 填充各段: 首条边 = fe[s] (保证段内 min==fe), 其余在 [fe[s], next_fe) 内采样
    vc = []
    for s in range(vs):
        size = base + 1 if s < extra else base
        lo = fe[s]
        hi = fe[s + 1] if s + 1 < vs else max_dst
        if size <= 0:
            seg = []
        elif size == 1:
            seg = [lo]
        else:
            seg = [lo] + [_sample_in_range(rng, lo, hi, edge_dist)
                          for _ in range(size - 1)]
            seg.sort()                       # 段内升序 (段间天然有序: 本段 max < next fe)
        vc.append(len(seg))
    return fe, vc


# ---------------------------- 生成逻辑 ----------------------------

def build_document(seed, vertex_dist, edge_dist, scale, max_dst):
    """用独立 RNG 生成一个完整的单 PMA 布局文档。"""
    rng = random.Random(seed)
    name = "pma_%s_%s_%s" % (vertex_dist, edge_dist, scale)

    counts = gen_vertex_counts(rng, vertex_dist, scale)
    sample_boundary = make_boundary_sampler(rng, edge_dist, max_dst, seed)

    vsegs = []
    fe = []
    vc = []
    for v in range(VERTEX_PER_PMA):
        count_v = counts[v]
        vs = vsegs_for_count(count_v)
        fe_v, vc_v = gen_vertex_layout(rng, count_v, vs, max_dst,
                                       edge_dist, sample_boundary)
        vsegs.append(vs)
        fe.extend(fe_v)
        vc.extend(vc_v)

    return {
        "name": name,
        "pma": 0,
        "vertices": VERTEX_PER_PMA,
        "max_dst": max_dst,
        "config": {
            "vertex_dist": vertex_dist,
            "edge_dist": edge_dist,
            "scale": scale,
            "seed": seed,
        },
        "initial_layout": {
            "vsegs": vsegs,          # 每 vertex 段数, len=16
            "edge_count": counts,    # 每 vertex 初始边数, len=16
            "fe": fe,                # 每段 first_edge (空段=-1), len=sum(vsegs)
            "vc": vc,                # 每段有效边数, len=sum(vsegs)
        },
    }


# ---------------------------- 校验 ----------------------------

def validate_document(data):
    """校验布局文档, 返回 (ok: bool, problems: list[str])。"""
    problems = []
    layout = data["initial_layout"]
    vsegs = layout["vsegs"]
    edge_count = layout["edge_count"]
    fe = layout["fe"]
    vc = layout["vc"]
    max_dst = data["max_dst"]

    if data["vertices"] != VERTEX_PER_PMA:
        problems.append("vertices=%d != %d" % (data["vertices"], VERTEX_PER_PMA))
    if len(vsegs) != VERTEX_PER_PMA:
        problems.append("len(vsegs)=%d != %d" % (len(vsegs), VERTEX_PER_PMA))
    if len(edge_count) != VERTEX_PER_PMA:
        problems.append("len(edge_count)=%d != %d" % (len(edge_count), VERTEX_PER_PMA))
    total_segs = sum(vsegs)
    if len(fe) != total_segs:
        problems.append("len(fe)=%d != sum(vsegs)=%d" % (len(fe), total_segs))
    if len(vc) != total_segs:
        problems.append("len(vc)=%d != sum(vsegs)=%d" % (len(vc), total_segs))

    seg_idx = 0
    for v in range(VERTEX_PER_PMA):
        vs = vsegs[v]
        seg_vc = vc[seg_idx:seg_idx + vs]
        if sum(seg_vc) != edge_count[v]:
            problems.append("vertex %d: sum(vc)=%d != edge_count=%d"
                            % (v, sum(seg_vc), edge_count[v]))
        prev_fe = None
        for s in range(vs):
            vv = vc[seg_idx + s]
            if not (0 <= vv <= EDGE_CAP_PER_SEG):
                problems.append("vertex %d seg %d: vc=%d not in [0,%d]"
                                % (v, s, vv, EDGE_CAP_PER_SEG))
            f = fe[seg_idx + s]
            if f != -1:
                if not (0 <= f < max_dst):
                    problems.append("vertex %d seg %d: fe=%d out of [0,%d)"
                                    % (v, s, f, max_dst))
                if prev_fe is not None and f <= prev_fe:
                    problems.append("vertex %d: fe 不严格递增 (%d 在 %d 之后)"
                                    % (v, f, prev_fe))
                prev_fe = f
        seg_idx += vs

    return len(problems) == 0, problems


# ---------------------------- 主流程 ----------------------------

def main():
    parser = argparse.ArgumentParser(
        description="生成已初始化的单 PMA 布局 (BS vs Merge HBM-IO 仿真)")
    parser.add_argument("--all", action="store_true",
                        help="生成完整矩阵 2×2×3=12 个文件")
    parser.add_argument("--out", default=DEFAULT_OUT_DIR,
                        help="输出目录 (默认 %s)" % DEFAULT_OUT_DIR)
    parser.add_argument("--seed", type=int, default=DEFAULT_SEED,
                        help="随机种子 (默认 %d)" % DEFAULT_SEED)
    parser.add_argument("--max-dst", type=int, default=DEFAULT_MAX_DST,
                        help="dst 取值范围上界 [0, max_dst) (默认 %d)" % DEFAULT_MAX_DST)
    parser.add_argument("--vertex-dist", choices=VERTEX_DISTS,
                        help="顶点边数分布 (uniform/skewed)")
    parser.add_argument("--edge-dist", choices=EDGE_DISTS,
                        help="dst 值分布 (uniform/skewed)")
    parser.add_argument("--scale", choices=SCALES,
                        help="规模 (small/medium/large)")
    args = parser.parse_args()

    if args.max_dst <= 0:
        parser.error("--max-dst 必须为正整数")

    # 组装待生成的 (vertex_dist, edge_dist, scale) 列表
    combos = []
    if args.all:
        for vd in VERTEX_DISTS:
            for ed in EDGE_DISTS:
                for sc in SCALES:
                    combos.append((vd, ed, sc))
    else:
        if args.vertex_dist is None or args.edge_dist is None or args.scale is None:
            parser.error("单文件模式需要 --vertex-dist, --edge-dist, --scale "
                         "(或用 --all 生成完整矩阵)")
        combos = [(args.vertex_dist, args.edge_dist, args.scale)]

    os.makedirs(args.out, exist_ok=True)

    # 生成 + 校验
    rows = []           # (filename, total_segs, total_edges, status)
    any_fail = False
    for vd, ed, sc in combos:
        doc = build_document(args.seed, vd, ed, sc, args.max_dst)
        filename = "%s.json" % doc["name"]
        out_path = os.path.join(args.out, filename)
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(doc, f, indent=2)
            f.write("\n")

        ok, problems = validate_document(doc)
        total_segs = sum(doc["initial_layout"]["vsegs"])
        total_edges = sum(doc["initial_layout"]["edge_count"])
        status = "PASS" if ok else "FAIL"
        if not ok:
            any_fail = True
            for p in problems:
                print("  [%s] %s" % (filename, p))
        rows.append((filename, total_segs, total_edges, status))

    # 打印汇总表
    col_w = max(len(r[0]) for r in rows) if rows else 0
    print("  %s  %10s  %11s  %s" % ("filename".ljust(col_w),
                                    "total_segs", "total_edges", "status"))
    for filename, ts, te, status in rows:
        print("  %s  %10d  %11d  %s"
              % (filename.ljust(col_w), ts, te, status))

    if any_fail:
        print("\n校验失败: 存在 FAIL 文件")
        sys.exit(1)
    print("\n全部 %d 个文件校验 PASS" % len(rows))


if __name__ == "__main__":
    main()
