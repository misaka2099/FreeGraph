#!/usr/bin/env python3
"""Run the bs-vs-merge IO sweep across generated PMA x insert-stream combos.

Produces results/io_results.csv (every case x policy, both helper models plus
breakdown) and a human-readable results/summary.md aggregating by batch size.
"""

import os
import csv
import glob
import json
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from bs_vs_merge_sim import run_bs, run_merge, run_policy2, IO

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.join(HERE, "data")
PMA_DIR = os.path.join(DATA, "pma")
INSERT_DIR = os.path.join(DATA, "insert")
RESULT_DIR = os.path.join(HERE, "results")

POLICIES = {"bs": run_bs, "merge": run_merge, "policy2": run_policy2}


def one_case(pma_file, insert_file, policy_name):
    import bs_vs_merge_sim as S
    pma = S.load_pma(pma_file)
    batches = S.load_inserts(insert_file)
    state = S.PMAState(pma["initial_layout"], max_dst=pma.get("max_dst", 65536))
    io = POLICIES[policy_name](state, batches)
    state.check()
    return io.summary()


def main():
    os.makedirs(RESULT_DIR, exist_ok=True)
    pma_files = sorted(glob.glob(os.path.join(PMA_DIR, "*.json")))
    ins_files = sorted(glob.glob(os.path.join(INSERT_DIR, "*.json")))
    if not pma_files:
        print("No PMA files under", PMA_DIR); sys.exit(1)
    if not ins_files:
        print("No insert files under", INSERT_DIR); sys.exit(1)

    rows = []
    for pf in pma_files:
        pma_name = os.path.basename(pf)[:-5]
        pcfg = json.load(open(pf))["config"]
        for inf in ins_files:
            ins_name = os.path.basename(inf)[:-5]
            icfg = json.load(open(inf))["config"]
            row = {
                "pma": pma_name,
                "vertex_dist": pcfg["vertex_dist"],
                "edge_dist": pcfg["edge_dist"],
                "scale": pcfg["scale"],
                "insert": ins_name,
                "update_dist": icfg["update_dist"],
                "dst_dist": icfg["dst_dist"],
                "batch_size": icfg["batch_size"],
            }
            for pol in ("bs", "merge", "policy2"):
                s = one_case(pf, inf, pol)
                row[pol + "_modelA"] = s["io_modelA_burst"]
                row[pol + "_modelB"] = s["io_modelB_indep"]
                if pol == "bs":
                    row["bs_rebalances"] = s["rebalances"]
                    row["bs_enlarges"] = s["enlarges"]
                    row["bs_seg_read"] = s["seg_read"]
                    row["bs_seg_write"] = s["seg_write"]
                    row["bs_fe_read_words"] = s["fe_read_words"]
                    row["bs_vc_read_words"] = s["vc_read_words"]
                if pol == "merge":
                    row["merge_vertices"] = s["merge_vertices"]
                    row["merge_seg_read"] = s["seg_read"]
                    row["merge_seg_write"] = s["seg_write"]
                    row["merge_enlarges"] = s["enlarges"]
            row["ratio_merge_over_bs_A"] = _ratio(row["merge_modelA"], row["bs_modelA"])
            row["ratio_merge_over_bs_B"] = _ratio(row["merge_modelB"], row["bs_modelB"])
            row["ratio_bs_over_merge_B"] = _ratio(row["bs_modelB"], row["merge_modelB"])
            rows.append(row)

    csv_path = os.path.join(RESULT_DIR, "io_results.csv")
    with open(csv_path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        for r in rows:
            w.writerow(r)
    print("wrote", csv_path, "rows:", len(rows))

    # ---- aggregations for summary ----
    agg_by_batch = {}
    for r in rows:
        key = r["batch_size"]
        agg_by_batch.setdefault(key, []).append(r)

    lines = []
    lines.append("# BS insert vs Merge — IO comparison summary\n")
    lines.append("Rows: %d cases (12 PMA x 24 insert streams x 3 policies).\n" % len(rows))

    lines.append("## Overall means\n")
    bsA = sum(r["bs_modelA"] for r in rows) / len(rows)
    bsB = sum(r["bs_modelB"] for r in rows) / len(rows)
    mgA = sum(r["merge_modelA"] for r in rows) / len(rows)
    mgB = sum(r["merge_modelB"] for r in rows) / len(rows)
    lines.append("| metric | modelA (helper burst) | modelB (independent) |")
    lines.append("|---|---|---|")
    lines.append("| bs mean IO | %.0f | %.0f |" % (bsA, bsB))
    lines.append("| merge mean IO | %.0f | %.0f |" % (mgA, mgB))
    lines.append("| merge/bs | %.2f | %.2f |" % (mgA / bsA, mgB / bsB))
    lines.append("")

    lines.append("## By batch size (mean merge/bs ratio; <1 => merge cheaper)\n")
    lines.append("| batch_size | ratio_A (merge/bs) | ratio_B (merge/bs) | bs_enlarge_mean | bs_rebalance_mean | merge_vertex_mean |")
    lines.append("|---|---|---|---|---|---|")
    for bs in sorted(agg_by_batch):
        rs = agg_by_batch[bs]
        ra = sum(r["ratio_merge_over_bs_A"] for r in rs) / len(rs)
        rb = sum(r["ratio_merge_over_bs_B"] for r in rs) / len(rs)
        en = sum(r["bs_enlarges"] for r in rs) / len(rs)
        re = sum(r["bs_rebalances"] for r in rs) / len(rs)
        mv = sum(r["merge_vertices"] for r in rs) / len(rs)
        lines.append("| %d | %.2f | %.2f | %.1f | %.1f | %.1f |" % (bs, ra, rb, en, re, mv))
    lines.append("")

    lines.append("## By base-graph scale (mean, modelB independent)\n")
    lines.append("| scale | bs_mean | merge_mean | ratio |")
    lines.append("|---|---|---|---|")
    for sc in ("small", "medium", "large"):
        rs = [r for r in rows if r["scale"] == sc]
        b = sum(r["bs_modelB"] for r in rs) / len(rs)
        m = sum(r["merge_modelB"] for r in rs) / len(rs)
        lines.append("| %s | %.0f | %.0f | %.2f |" % (sc, b, m, m / b))
    lines.append("")

    lines.append("## By update skew (mean, modelB)\n")
    lines.append("| update_dist | bs_mean | merge_mean | ratio |")
    lines.append("|---|---|---|---|")
    for ud in ("uniform", "skewed"):
        rs = [r for r in rows if r["update_dist"] == ud]
        b = sum(r["bs_modelB"] for r in rs) / len(rs)
        m = sum(r["merge_modelB"] for r in rs) / len(rs)
        lines.append("| %s | %.0f | %.0f | %.2f |" % (ud, b, m, m / b))
    lines.append("")

    # ---- crossover analysis: where does merge beat bs (modelB) ----
    lines.append("## ModelB (independent helper) crossover: merge wins when ratio < 1\n")
    lines.append("Grouped by (scale, update_dist, dst_dist):\n")
    lines.append("| scale | update | dst | merge_wins_A | merge_wins_B | mean_ratio_B |")
    lines.append("|---|---|---|---|---|---|")
    groups = {}
    for r in rows:
        g = (r["scale"], r["update_dist"], r["dst_dist"])
        groups.setdefault(g, []).append(r)
    for g in sorted(groups):
        rs = groups[g]
        wa = sum(1 for r in rs if r["ratio_merge_over_bs_A"] < 1.0) / len(rs)
        wb = sum(1 for r in rs if r["ratio_merge_over_bs_B"] < 1.0) / len(rs)
        mb = sum(r["ratio_merge_over_bs_B"] for r in rs) / len(rs)
        lines.append("| %s | %s | %s | %.2f | %.2f | %.2f |" % (g[0], g[1], g[2], wa, wb, mb))
    lines.append("")

    # per-batch-size breakdown of winners under modelB for each scale
    lines.append("## ModelB: merge-wins fraction by (scale, batch_size)\n")
    lines.append("| scale | bs1 | bs4 | bs16 | bs64 | bs256 | bs1024 |")
    lines.append("|---|---|---|---|---|---|---|")
    for sc in ("small", "medium", "large"):
        cell = []
        for bs in (1, 4, 16, 64, 256, 1024):
            rs = [r for r in rows if r["scale"] == sc and r["batch_size"] == bs]
            wb = sum(1 for r in rs if r["ratio_merge_over_bs_B"] < 1.0) / len(rs)
            cell.append("%.0f%%" % (100 * wb))
        lines.append("| %s | %s |" % (sc, " | ".join(cell)))
    lines.append("")

    summary_path = os.path.join(RESULT_DIR, "summary.md")
    with open(summary_path, "w") as f:
        f.write("\n".join(lines))
    print("wrote", summary_path)
    print("\n".join(lines))


def _ratio(a, b):
    return (a / b) if b else float("nan")


if __name__ == "__main__":
    main()
