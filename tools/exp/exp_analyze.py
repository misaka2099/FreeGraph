#!/usr/bin/env python3
"""Read results/io_results.csv and write results/report.md with the full
bs-vs-merge IO comparison narrative, including the default policy2."""

import csv
import os

HERE = os.path.dirname(os.path.abspath(__file__))
CSV = os.path.join(HERE, "results", "io_results.csv")
OUT = os.path.join(HERE, "results", "report.md")


def load():
    with open(CSV) as f:
        return list(csv.DictReader(f))


def mean(xs):
    xs = list(xs)
    return sum(xs) / len(xs) if xs else float("nan")


def main():
    rows = load()
    L = []
    A = L.append

    A("# BS insert (rebalance) vs Merge — IO comparison report\n")
    A("_single-PMA HBM-IO simulation. 12 base graphs x 24 timestamped insert streams x "
      "3 policies = 288 cases. Total 4096 updates per case._\n")

    A("## 1. Two helper-access models\n")
    A("- **Model A (helper burst / on-chip)**: a bs batch's FE+VC helper traffic = 1 IO "
      "(helpers burst-loaded once). Represents a future on-chip helper cache.\n")
    A("- **Model B (helper independent)**: every scattered FE-search read and VC RMW = 1 IO "
      "(the current hardware behavior: random 512-bit HBM reads).\n")
    A("- Raw data (segments/header/vecinfo/buffer): contiguous burst = 1 IO, scattered = "
      "per-access. bs segments do not coalesce across touched segments (data-dependent "
      "addresses); merge rewrites whole vertices as one contiguous burst.\n")

    A("\n## 2. Overall result\n")
    def agg(pol):
        return (mean(float(r[pol + "_modelA"]) for r in rows),
                mean(float(r[pol + "_modelB"]) for r in rows))
    bA, bB = agg("bs")
    mA, mB = agg("merge")
    pA, pB = agg("policy2")
    A("| policy | modelA (helper burst) | modelB (independent) |\n|---|---|---|")
    A("| bs | %.0f | %.0f |" % (bA, bB))
    A("| merge | %.0f | %.0f |" % (mA, mB))
    A("| policy2 (count>16 -> merge) | %.0f | %.0f |" % (pA, pB))
    A("")
    A("- **Model A**: merge costs **%.2fx** bs; policy2 costs **%.2fx** bs."
      % (mA / bA, pA / bA))
    A("- **Model B**: merge costs **%.2fx** bs; policy2 costs **%.2fx** bs."
      % (mB / bB, pB / bB))
    A("- Under model A the scattered helper traffic is ~free, so bs's "
      "'touch-only-the-hit-segment' locality wins; under model B (real hardware) the "
      "scattered FE/VC access dominates and merge's whole-vertex contiguous rewrite "
      "wins in almost every regime.\n")

    A("\n## 3. By batch size (mean over all base graphs)\n")
    A("| batch_size | ratio_A mg/bs | ratio_B mg/bs | policy2_B | bs_rebalance | bs_enlarge | merge_vertex |")
    A("|---|---|---|---|---|---|---|")
    for bs in (1, 4, 16, 64, 256, 1024):
        rs = [r for r in rows if r["batch_size"] == str(bs)]
        ra = mean(float(r["ratio_merge_over_bs_A"]) for r in rs)
        rb = mean(float(r["ratio_merge_over_bs_B"]) for r in rs)
        rp = mean(float(r["policy2_modelB"]) / float(r["bs_modelB"]) for r in rs)
        en = mean(float(r["bs_enlarges"]) for r in rs)
        re_ = mean(float(r["bs_rebalances"]) for r in rs)
        mv = mean(float(r["merge_vertices"]) for r in rs)
        A("| %d | %.2f | %.2f | %.2f | %.0f | %.0f | %.0f |" % (bs, ra, rb, rp, re_, en, mv))
    A("")
    A("Read: `ratio_A/B < 1` => merge cheaper. **Model A crossover at batch ~64**; "
      "**model B merge wins from batch 1 up** (only small graphs at batch 1-4 favour bs).\n")

    A("\n## 4. By base-graph scale (model B)\n")
    A("| scale | segs/PMA | bs_mean | merge_mean | ratio |")
    A("|---|---|---|---|---|")
    for sc, segs in (("small", "~16-64"), ("medium", "~256-512"), ("large", "~1024-4096")):
        rs = [r for r in rows if r["scale"] == sc]
        b = mean(float(r["bs_modelB"]) for r in rs)
        m = mean(float(r["merge_modelB"]) for r in rs)
        A("| %s | %s | %.0f | %.0f | %.2f |" % (sc, segs, b, m, m / b))
    A("")
    A("Larger vertices favour merge: bs's per-edge FE search grows with log2(vertex segs) "
      "while merge's whole-vertex rewrite is amortized across the batch.\n")

    A("\n## 5. Merge-wins fractions by (scale, batch_size)\n")
    A("**Model A (helper burst):**\n")
    A("| scale | bs1 | bs4 | bs16 | bs64 | bs256 | bs1024 |")
    A("|---|---|---|---|---|---|---|")
    for sc in ("small", "medium", "large"):
        c = []
        for bs in (1, 4, 16, 64, 256, 1024):
            rs = [r for r in rows if r["scale"] == sc and r["batch_size"] == str(bs)]
            w = sum(1 for r in rs if float(r["ratio_merge_over_bs_A"]) < 1.0) / len(rs)
            c.append("%d%%" % int(round(100 * w)))
        A("| %s | %s |" % (sc, " | ".join(c)))
    A("\n**Model B (helper independent):**\n")
    A("| scale | bs1 | bs4 | bs16 | bs64 | bs256 | bs1024 |")
    A("|---|---|---|---|---|---|---|")
    for sc in ("small", "medium", "large"):
        c = []
        for bs in (1, 4, 16, 64, 256, 1024):
            rs = [r for r in rows if r["scale"] == sc and r["batch_size"] == str(bs)]
            w = sum(1 for r in rs if float(r["ratio_merge_over_bs_B"]) < 1.0) / len(rs)
            c.append("%d%%" % int(round(100 * w)))
        A("| %s | %s |" % (sc, " | ".join(c)))
    A("")
    A("Under model B, merge wins from batch 4 (small) / batch 1 (medium, large). "
      "Under model A, bs wins until batch 64 (small) / 256 (medium, large).\n")

    A("\n## 6. Why (IO anatomy)\n")
    A("Per-wave costs (single PMA):\n")
    A("```")
    A("BS (per 16-edge chunk):                         MERGE (per wave):")
    A("  +1 vecinfo read                                 +1 vecinfo read")
    A("  +1 header read (2w burst)                       +1 header read")
    A("  +log2(vsegs) FE reads/edge (scattered)          per affected vertex:")
    A("  +1 seg burst read + 1 write per hit seg           +read WHOLE vertex (burst)")
    A("  +2 VC RMW/hit seg (scattered)                     +write merged->buffer (burst)")
    A("  +1 header write                                   +read buffer, rewrite segs (burst)")
    A("  overflow>47 -> rebalance (~9 IO)                  +VC/FE contiguous RMW (bursts)")
    A("                                                    +1 header write")
    A("```")
    A("bs pays per-edge FE search (scattered) and touches only hit segments; "
      "merge pays a whole-vertex contiguous rewrite once per affected vertex. "
      "For large batches, merge amortizes the vertex rewrite; bs instead pays "
      "one FE search + one segment touch per 16 edges plus overflow rebalances.\n")

    A("\n## 7. Caveats & model decisions\n")
    A("- **pre-expand** counted in both paths (vecinfo+header read per wave, ENLARGE "
      "when a vertex would overflow). Growth modeled as repeated doubling to guarantee "
      "`(ve-vs)*47 >= old+count` (documented design invariant). Note: the current "
      "`redistribution_kernel.cpp` ENLARGE doubles only **once**; a single wave that "
      "exceeds a vertex's whole capacity would under-provision merge (no overflow path "
      "there) — a real-system bug worth fixing.\n")
    A("- bs's FE stays **stale** between rebalances (bs_insert does not write FE); the "
      "search can misroute into an empty mid-vertex segment. Modeled faithfully.\n")
    A("- hbm_buffer intermediate traffic (merge/redistribute) counted as contiguous bursts.\n")
    A("- Base-graph construction IO is excluded (both pipelines start from an initialized "
      "PMA). ALLOC/FREE counted separately, not in data IO.\n")
    A("- bs `modelA` = 1 helper IO per 16-edge chunk; `modelB` = every scattered FE/VC "
      "access. The truth for the current hardware is between them but closer to B "
      "(FE search is a random HBM read per iteration).\n")

    A("\n## 8. Take-aways\n")
    A("1. **The current hardware (model B) favours merge in nearly every regime** — "
      "the scattered FE binary-search reads dominate bs. The only regime bs wins is "
      "small graphs (< 2 segs/vertex) with tiny batches.\n")
    A("2. **A real on-chip FE/VC cache (model A) flips the picture**: bs then wins for "
      "batches < ~64; merge only wins for bulk loads. This quantifies the value of the "
      "abandoned on-chip FE cache idea.\n")
    A("3. **policy2 (count>16 -> merge) is a good default**: it sends small batches to "
      "bs (cheap under model A) and bulk to merge — its modelB cost stays near merge's "
      "while modelA is far below pure merge.\n")
    A("4. Larger base-graph vertices and skewed updates push harder toward merge; "
      "uniform small graphs + tiny batches push toward bs.\n")

    with open(OUT, "w") as f:
        f.write("\n".join(L))
    print("wrote", OUT)


if __name__ == "__main__":
    main()
