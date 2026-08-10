# BS insert vs Merge — IO comparison summary

Rows: 288 cases (12 PMA x 24 insert streams x 3 policies).

## Overall means

| metric | modelA (helper burst) | modelB (independent) |
|---|---|---|
| bs mean IO | 16196 | 41280 |
| merge mean IO | 20086 | 20086 |
| merge/bs | 1.24 | 0.49 |

## By batch size (mean merge/bs ratio; <1 => merge cheaper)

| batch_size | ratio_A (merge/bs) | ratio_B (merge/bs) | bs_enlarge_mean | bs_rebalance_mean | merge_vertex_mean |
|---|---|---|---|---|---|
| 1 | 1.40 | 0.88 | 12.5 | 538.3 | 4096.0 |
| 4 | 1.73 | 0.75 | 12.3 | 515.7 | 3460.8 |
| 16 | 1.47 | 0.52 | 11.5 | 452.3 | 2201.5 |
| 64 | 0.82 | 0.26 | 8.7 | 339.4 | 917.0 |
| 256 | 0.39 | 0.10 | 4.5 | 219.7 | 256.0 |
| 1024 | 0.17 | 0.03 | 1.9 | 165.4 | 64.0 |

## By base-graph scale (mean, modelB independent)

| scale | bs_mean | merge_mean | ratio |
|---|---|---|---|
| small | 30000 | 21440 | 0.71 |
| medium | 40321 | 19593 | 0.49 |
| large | 53521 | 19224 | 0.36 |

## By update skew (mean, modelB)

| update_dist | bs_mean | merge_mean | ratio |
|---|---|---|---|
| uniform | 41508 | 21150 | 0.51 |
| skewed | 41053 | 19021 | 0.46 |

## ModelB (independent helper) crossover: merge wins when ratio < 1

Grouped by (scale, update_dist, dst_dist):

| scale | update | dst | merge_wins_A | merge_wins_B | mean_ratio_B |
|---|---|---|---|---|---|
| large | skewed | skewed | 0.67 | 1.00 | 0.28 |
| large | skewed | uniform | 0.50 | 1.00 | 0.30 |
| large | uniform | skewed | 0.54 | 1.00 | 0.30 |
| large | uniform | uniform | 0.50 | 1.00 | 0.34 |
| medium | skewed | skewed | 0.50 | 1.00 | 0.39 |
| medium | skewed | uniform | 0.50 | 1.00 | 0.38 |
| medium | uniform | skewed | 0.50 | 1.00 | 0.40 |
| medium | uniform | uniform | 0.50 | 1.00 | 0.43 |
| small | skewed | skewed | 0.42 | 0.83 | 0.55 |
| small | skewed | uniform | 0.42 | 0.83 | 0.49 |
| small | uniform | skewed | 0.33 | 0.67 | 0.64 |
| small | uniform | uniform | 0.33 | 0.67 | 0.59 |

## ModelB: merge-wins fraction by (scale, batch_size)

| scale | bs1 | bs4 | bs16 | bs64 | bs256 | bs1024 |
|---|---|---|---|---|---|---|
| small | 0% | 50% | 100% | 100% | 100% | 100% |
| medium | 100% | 100% | 100% | 100% | 100% | 100% |
| large | 100% | 100% | 100% | 100% | 100% | 100% |
