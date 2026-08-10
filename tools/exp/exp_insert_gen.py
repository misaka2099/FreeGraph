#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
exp_insert_gen.py — 生成带时间戳的分批插入流 (JSON)

用于 BS insert vs Merge 的 HBM-IO 仿真实验。
生成 24 个插入流 (2 update_dist × 2 dst_dist × 6 batch_size)。

用法:
    python3 exp_insert_gen.py --all --out DIR [--seed N] [--max-dst N]
    python3 exp_insert_gen.py --update-dist {uniform,skewed} --dst-dist {uniform,skewed} \
                              --batch-size N [--total N] [--seed N] [--max-dst N] [--out DIR]

语义:
    - src  ∈ 0..15 (顶点号, VERTEX_PER_PMA=16)
    - dst  ∈ [0, max_dst)
    - 时间戳语义: batch 按 time 严格递增; t 时刻的 batch 只依赖 time ≤ t 的数据。
      每个 batch 独立采样 (无跨 batch 状态), 因此天然满足该约束。
    - 批内按 (src, dst) 升序排序, 模拟 task_reader 的 radix sort 输出
      (仿真依赖每顶点 dst 有序)。
    - 允许重复 (src, dst) 多边, 绝不去重。

随机种子说明:
    每个文件用 fresh random.Random(seed) 生成, config.seed 即所用种子。
    因此固定 --seed 时, 同参数维度 (update_dist, dst_dist) 的不同 batch_size
    文件共享同一随机流, 只是分批方式不同 —— 作为 batch_size 对比的控制变量。
"""

import argparse
import bisect
import json
import math
import os
import random
import sys

# ---------------------------- 固定常量 ----------------------------

VERTEX_PER_PMA = 16                 # 顶点数, src 取值 0..15
VERTEX_INDICES = list(range(VERTEX_PER_PMA))
HOT_VERTICES = {3, 7, 11}           # skewed update_dist 的热顶点集合
DEFAULT_MAX_DST = 65536             # dst 默认取值范围 [0, max_dst)
DEFAULT_TOTAL_UPDATES = 4096        # 默认总更新边数
BATCH_SIZES = [1, 4, 16, 64, 256, 1024]
UPDATE_DISTS = ["uniform", "skewed"]
DST_DISTS = ["uniform", "skewed"]
ZIPF_S = 1.5                        # dst 幂律 (Zipf) 分布的 s 参数, s≈1.5 强左偏
DEFAULT_SEED = 42

# 默认输出目录: tools/exp/data/insert (相对脚本文件定位, 与 CWD 无关)
DEFAULT_OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                               "data", "insert")


# ---------------------------- 分布工具 ----------------------------

def src_weights(update_dist):
    """返回 16 个顶点的采样权重 (random.choices 的相对权重)。"""
    if update_dist == "uniform":
        # 16 顶点等概率命中
        return [1.0] * VERTEX_PER_PMA
    if update_dist == "skewed":
        # 热顶点 {3,7,11} 合计 ~70%, 其余 13 个顶点平分剩余 30%
        hot_weight = 0.70 / len(HOT_VERTICES)
        cold_weight = 0.30 / (VERTEX_PER_PMA - len(HOT_VERTICES))
        return [hot_weight if v in HOT_VERTICES else cold_weight
                for v in VERTEX_INDICES]
    raise ValueError("unknown update_dist: %s" % update_dist)


_zipf_cdf_cache = {}


def zipf_cdf(s, max_dst):
    """p(k) ∝ 1/k^s, k = 1..max_dst, 返回累计分布列表 (长度 max_dst)。"""
    key = (s, max_dst)
    if key in _zipf_cdf_cache:
        return _zipf_cdf_cache[key]
    weights = [1.0 / (k ** s) for k in range(1, max_dst + 1)]
    total = sum(weights)
    cdf = []
    acc = 0.0
    for w in weights:
        acc += w / total
        cdf.append(acc)
    cdf[-1] = 1.0                    # 浮点求和兜底, 保证末项 = 1
    _zipf_cdf_cache[key] = cdf
    return cdf


def make_dst_sampler(rng, dst_dist, max_dst):
    """返回一个 dst 采样闭包 (无参数, 返回 [0, max_dst) 内的 int)。"""
    if dst_dist == "uniform":
        return lambda: rng.randrange(max_dst)
    if dst_dist == "skewed":
        # 逆 CDF 采样: r = U[0,1), 找最小 k 使 CDF(k) >= r, 则 dst = k-1
        cdf = zipf_cdf(ZIPF_S, max_dst)

        def sample_skewed():
            k = bisect.bisect_left(cdf, rng.random()) + 1  # k ∈ 1..max_dst
            return k - 1                                    # dst ∈ 0..max_dst-1

        return sample_skewed
    raise ValueError("unknown dst_dist: %s" % dst_dist)


# ---------------------------- 生成逻辑 ----------------------------

def generate_batches(rng, update_dist, dst_dist, batch_size,
                     total_updates, max_dst):
    """按时间戳顺序生成 total_updates 条边, 每批 batch_size 条 (最后一批可不满)。"""
    weights = src_weights(update_dist)
    sample_dst = make_dst_sampler(rng, dst_dist, max_dst)
    num_batches = math.ceil(total_updates / batch_size)
    batches = []
    for t in range(num_batches):
        # 最后一批可能不满 (仅当 total_updates 不是 batch_size 整数倍时)
        n_this = min(batch_size, total_updates - t * batch_size)
        edges = []
        for _ in range(n_this):
            src = rng.choices(VERTEX_INDICES, weights=weights)[0]
            dst = sample_dst()
            edges.append({"src": src, "dst": dst})
        # 批内按 (src, dst) 升序排序, 模拟 task_reader radix sort 的 per-PMA runs
        edges.sort(key=lambda e: (e["src"], e["dst"]))
        batches.append({"time": t, "edges": edges})
    return batches


def build_document(seed, update_dist, dst_dist, batch_size,
                   total_updates, max_dst):
    """用独立 RNG 生成一个完整的插入流文档。"""
    rng = random.Random(seed)
    name = "upd_%s_dst_%s_bs%d" % (update_dist, dst_dist, batch_size)
    batches = generate_batches(rng, update_dist, dst_dist, batch_size,
                               total_updates, max_dst)
    return {
        "name": name,
        "config": {
            "update_dist": update_dist,
            "dst_dist": dst_dist,
            "batch_size": batch_size,
            "total_updates": total_updates,
            "max_dst": max_dst,
            "seed": seed,
        },
        "batches": batches,
    }


# ---------------------------- 校验 ----------------------------

def validate_document(data, batch_size, total_updates, max_dst):
    """校验一个文档, 返回 (ok: bool, problems: list[str])。"""
    problems = []
    batches = data["batches"]

    # 1) 全文件边数 == total_updates
    total_edges = sum(len(b["edges"]) for b in batches)
    if total_edges != total_updates:
        problems.append("total_edges=%d != total_updates=%d"
                        % (total_edges, total_updates))

    # 2) time 严格递增 0..nb-1
    times = [b["time"] for b in batches]
    if times != list(range(len(batches))):
        problems.append("times not 0..nb-1: %r" % (times,))

    # 3) 每批边数 / src / dst 范围 / 批内 (src, dst) 升序
    for i, b in enumerate(batches):
        n = len(b["edges"])
        if i < len(batches) - 1 and n != batch_size:
            problems.append("batch[%d] has %d edges != batch_size=%d"
                            % (i, n, batch_size))
        prev = None
        for e in b["edges"]:
            if not (0 <= e["src"] < VERTEX_PER_PMA):
                problems.append("batch[%d] src out of range: %r" % (i, e["src"]))
            if not (0 <= e["dst"] < max_dst):
                problems.append("batch[%d] dst out of range: %r" % (i, e["dst"]))
            key = (e["src"], e["dst"])
            if prev is not None and key < prev:
                problems.append("batch[%d] not sorted by (src,dst): %r before %r"
                                % (i, prev, key))
            prev = key

    return len(problems) == 0, problems


# ---------------------------- 主流程 ----------------------------

def main():
    parser = argparse.ArgumentParser(
        description="生成带时间戳的分批插入流 (BS vs Merge HBM-IO 仿真)")
    parser.add_argument("--all", action="store_true",
                        help="生成完整矩阵 2×2×6=24 个文件")
    parser.add_argument("--out", default=DEFAULT_OUT_DIR,
                        help="输出目录 (默认 %s)" % DEFAULT_OUT_DIR)
    parser.add_argument("--seed", type=int, default=DEFAULT_SEED,
                        help="随机种子 (默认 %d)" % DEFAULT_SEED)
    parser.add_argument("--max-dst", type=int, default=DEFAULT_MAX_DST,
                        help="dst 取值范围上界 [0, max_dst) (默认 %d)" % DEFAULT_MAX_DST)
    parser.add_argument("--update-dist", choices=UPDATE_DISTS,
                        help="顶点更新分布 (uniform/skewed)")
    parser.add_argument("--dst-dist", choices=DST_DISTS,
                        help="dst 值分布 (uniform/skewed)")
    parser.add_argument("--batch-size", type=int, choices=BATCH_SIZES,
                        help="每批边数 (1/4/16/64/256/1024)")
    parser.add_argument("--total", type=int, default=DEFAULT_TOTAL_UPDATES,
                        help="总更新边数 (默认 %d)" % DEFAULT_TOTAL_UPDATES)
    args = parser.parse_args()

    if args.max_dst <= 0:
        parser.error("--max-dst 必须为正整数")
    if args.total <= 0:
        parser.error("--total 必须为正整数")

    # 组装待生成的 (update_dist, dst_dist, batch_size) 列表
    combos = []
    if args.all:
        for ud in UPDATE_DISTS:
            for dd in DST_DISTS:
                for bs in BATCH_SIZES:
                    combos.append((ud, dd, bs))
    else:
        if args.update_dist is None or args.dst_dist is None or args.batch_size is None:
            parser.error("单文件模式需要 --update-dist, --dst-dist, --batch-size "
                         "(或用 --all 生成完整矩阵)")
        combos = [(args.update_dist, args.dst_dist, args.batch_size)]

    os.makedirs(args.out, exist_ok=True)

    # 生成 + 校验
    rows = []           # (filename, batch_size, num_batches, total_edges, status)
    any_fail = False
    for ud, dd, bs in combos:
        doc = build_document(args.seed, ud, dd, bs,
                             args.total, args.max_dst)
        filename = "%s.json" % doc["name"]
        out_path = os.path.join(args.out, filename)
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(doc, f, indent=2)
            f.write("\n")

        ok, problems = validate_document(doc, bs, args.total, args.max_dst)
        num_batches = len(doc["batches"])
        total_edges = sum(len(b["edges"]) for b in doc["batches"])
        status = "PASS" if ok else "FAIL"
        if not ok:
            any_fail = True
            for p in problems:
                print("  [%s] %s" % (filename, p))
        rows.append((filename, bs, num_batches, total_edges, status))

    # 打印汇总表
    col_w = max(len(r[0]) for r in rows) if rows else 0
    header = (filename.ljust(col_w), "batch_size", "num_batches", "total_edges", "status")
    print("  %s  %10s  %11s  %11s  %s"
          % (header[0], header[1], header[2], header[3], header[4]))
    for filename, bs, nb, te, status in rows:
        print("  %s  %10d  %11d  %11d  %s"
              % (filename.ljust(col_w), bs, nb, te, status))

    if any_fail:
        print("\n校验失败: 存在 FAIL 文件")
        sys.exit(1)
    print("\n全部 %d 个文件校验 PASS" % len(rows))


if __name__ == "__main__":
    main()
