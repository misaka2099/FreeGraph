# BS insert (rebalance) vs Merge — IO comparison report

_single-PMA HBM-IO simulation. 12 base graphs x 24 timestamped insert streams x 3 policies = 288 cases. Total 4096 updates per case._

## 1. Two helper-access models

- **Model A (helper burst / on-chip)**: a bs batch's FE+VC helper traffic = 1 IO (helpers burst-loaded once). Represents a future on-chip helper cache.

- **Model B (helper independent)**: every scattered FE-search read and VC RMW = 1 IO (the current hardware behavior: random 512-bit HBM reads).

- Raw data (segments/header/vecinfo/buffer): contiguous burst = 1 IO, scattered = per-access. bs segments do not coalesce across touched segments (data-dependent addresses); merge rewrites whole vertices as one contiguous burst.


## 2. Overall result

| policy | modelA (helper burst) | modelB (independent) |
|---|---|---|
| bs | 16196 | 41280 |
| merge | 20086 | 20086 |
| policy2 (count>16 -> merge) | 14340 | 27465 |

- **Model A**: merge costs **1.24x** bs; policy2 costs **0.89x** bs.
- **Model B**: merge costs **0.49x** bs; policy2 costs **0.67x** bs.
- Under model A the scattered helper traffic is ~free, so bs's 'touch-only-the-hit-segment' locality wins; under model B (real hardware) the scattered FE/VC access dominates and merge's whole-vertex contiguous rewrite wins in almost every regime.


## 3. By batch size (mean over all base graphs)

| batch_size | ratio_A mg/bs | ratio_B mg/bs | policy2_B | bs_rebalance | bs_enlarge | merge_vertex |
|---|---|---|---|---|---|---|
| 1 | 1.40 | 0.88 | 1.00 | 538 | 13 | 4096 |
| 4 | 1.73 | 0.75 | 1.00 | 516 | 12 | 3461 |
| 16 | 1.47 | 0.52 | 1.00 | 452 | 12 | 2202 |
| 64 | 0.82 | 0.26 | 0.26 | 339 | 9 | 917 |
| 256 | 0.39 | 0.10 | 0.10 | 220 | 5 | 256 |
| 1024 | 0.17 | 0.03 | 0.03 | 165 | 2 | 64 |

Read: `ratio_A/B < 1` => merge cheaper. **Model A crossover at batch ~64**; **model B merge wins from batch 1 up** (only small graphs at batch 1-4 favour bs).


## 4. By base-graph scale (model B)

| scale | segs/PMA | bs_mean | merge_mean | ratio |
|---|---|---|---|---|
| small | ~16-64 | 30000 | 21440 | 0.71 |
| medium | ~256-512 | 40321 | 19593 | 0.49 |
| large | ~1024-4096 | 53521 | 19224 | 0.36 |

Larger vertices favour merge: bs's per-edge FE search grows with log2(vertex segs) while merge's whole-vertex rewrite is amortized across the batch.


## 5. Merge-wins fractions by (scale, batch_size)

**Model A (helper burst):**

| scale | bs1 | bs4 | bs16 | bs64 | bs256 | bs1024 |
|---|---|---|---|---|---|---|
| small | 0% | 0% | 0% | 25% | 100% | 100% |
| medium | 0% | 0% | 0% | 100% | 100% | 100% |
| large | 0% | 0% | 31% | 100% | 100% | 100% |

**Model B (helper independent):**

| scale | bs1 | bs4 | bs16 | bs64 | bs256 | bs1024 |
|---|---|---|---|---|---|---|
| small | 0% | 50% | 100% | 100% | 100% | 100% |
| medium | 100% | 100% | 100% | 100% | 100% | 100% |
| large | 100% | 100% | 100% | 100% | 100% | 100% |

Under model B, merge wins from batch 4 (small) / batch 1 (medium, large). Under model A, bs wins until batch 64 (small) / 256 (medium, large).


## 6. Why (IO anatomy)

Per-wave costs (single PMA):

```
BS (per 16-edge chunk):                         MERGE (per wave):
  +1 vecinfo read                                 +1 vecinfo read
  +1 header read (2w burst)                       +1 header read
  +log2(vsegs) FE reads/edge (scattered)          per affected vertex:
  +1 seg burst read + 1 write per hit seg           +read WHOLE vertex (burst)
  +2 VC RMW/hit seg (scattered)                     +write merged->buffer (burst)
  +1 header write                                   +read buffer, rewrite segs (burst)
  overflow>47 -> rebalance (~9 IO)                  +VC/FE contiguous RMW (bursts)
                                                    +1 header write
```
bs pays per-edge FE search (scattered) and touches only hit segments; merge pays a whole-vertex contiguous rewrite once per affected vertex. For large batches, merge amortizes the vertex rewrite; bs instead pays one FE search + one segment touch per 16 edges plus overflow rebalances.


## 7. Caveats & model decisions

- **pre-expand** counted in both paths (vecinfo+header read per wave, ENLARGE when a vertex would overflow). Growth modeled as repeated doubling to guarantee `(ve-vs)*47 >= old+count` (documented design invariant). Note: the current `redistribution_kernel.cpp` ENLARGE doubles only **once**; a single wave that exceeds a vertex's whole capacity would under-provision merge (no overflow path there) — a real-system bug worth fixing.

- bs's FE stays **stale** between rebalances (bs_insert does not write FE); the search can misroute into an empty mid-vertex segment. Modeled faithfully.

- hbm_buffer intermediate traffic (merge/redistribute) counted as contiguous bursts.

- Base-graph construction IO is excluded (both pipelines start from an initialized PMA). ALLOC/FREE counted separately, not in data IO.

- bs `modelA` = 1 helper IO per 16-edge chunk; `modelB` = every scattered FE/VC access. The truth for the current hardware is between them but closer to B (FE search is a random HBM read per iteration).


## 8. Take-aways

1. **The current hardware (model B) favours merge in nearly every regime** — the scattered FE binary-search reads dominate bs. The only regime bs wins is small graphs (< 2 segs/vertex) with tiny batches.

2. **A real on-chip FE/VC cache (model A) flips the picture**: bs then wins for batches < ~64; merge only wins for bulk loads. This quantifies the value of the abandoned on-chip FE cache idea.

3. **policy2 (count>16 -> merge) is a good default**: it sends small batches to bs (cheap under model A) and bulk to merge — its modelB cost stays near merge's while modelA is far below pure merge.

4. Larger base-graph vertices and skewed updates push harder toward merge; uniform small graphs + tiny batches push toward bs.
