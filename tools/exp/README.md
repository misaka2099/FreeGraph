# BS Insert vs Merge — IO 仿真实验

对比 PMA 图加速器中 **bs insert (rebalance)** 管线与 **merge** 管线在不同
base graph / 更新模式 / batch 大小下的 **HBM IO 次数**。

## 1. 两条管线（基于 2026-08-11 源码）

### BS insert 路径（`bs_insert_kernel.cpp` + `redistribution_kernel.cpp`）

一次 wave（一个 PMA 的 B 条新边）的处理：

1. **pre-expand**（redistribution，每条路径都要付）：
   - `vi_read` 1 次散乱读 + header 2 字连续读（1 burst）
   - 若任一 vertex 现有边 + 新边 > 段数×47 → **ENLARGE**：alloc 新空间、写新 header、
     init FE/VC 区、collect 旧 PMA 全部边到 buffer、per-vertex redistribute 写回、
     FREE 旧 PMA、写 VEC_INFO
2. **bs_worker**（每 ≤16 边一批）：
   - `vi_read` 1 + header 读 1 burst
   - 每条边对 FE 数组**二分搜索**目标段：每次迭代 1 次**散乱随机** 512-bit 读，
     迭代次数 ≈ log₂(该顶点段数)
3. **insert_phase**（每批）：
   - burst 读每个命中段（4 字连续，但段间地址数据依赖 → **段间不合并**，每段 1 burst）
   - 片上 `seg_insert`（新边排入段内）
   - burst 写回命中段 + **VC RMW**（每段 1 读 1 写，散乱）
   - header 写回 1 burst
4. **段溢出 >47** → rebalance：
   - 读整顶点 VC（连续范围，1 burst）
   - power-of-2 找 redistribution 范围（读该范围 VC）
   - collect 范围段到 buffer（读段 + 写 buffer）
   - redistribute（读 buffer + 写段 + VC/FE 连续 RMW）
   - 若顶点总边数 > 段数×47 → ENLARGE（整 PMA 搬移）

### Merge 路径（`merge_pipeline.cpp`）

一次 wave（整个 PMA 的边一次下发）：

1. **pre-expand**（同上，保证容量）
2. **merge_phase**（每 PMA 批一次）：
   - `vi_read` 1 + header 读 1 burst（**整 PMA 摊销，不是每 16 边**）
   - 对**每个受影响 vertex**（≥1 条新边）：
     - burst 读**整个顶点所有段**到片上（连续，1 burst / 1024 段分块）
     - 片上双指针归并（现有 + 新边）→ 结果写 hbm_buffer（连续，1 burst）
     - `redistribute_edge_from_hbm`：读 buffer、**整顶点段写回**、VC/FE 连续 RMW
     - **即使只插 1 条边，也整顶点重写**
   - header.edge_count 写回 1 burst

### 关键差异

| | BS | Merge |
|---|---|---|
| 访问粒度 | 只碰命中段 | 整顶点 |
| FE 二分搜索 | 每边散乱读 ~log₂(vsegs) 次 | 无 |
| header/vi 读 | 每 16 边一批付一次 | 整 PMA 一次 |
| VC/FE 更新 | 命中段散乱 RMW | 整顶点连续 RMW |
| 段写回 | 每命中段 4 字 burst（不跨段合并） | 整顶点连续 burst |

## 2. IO 计数规则

所有计数以 **512-bit word** 为访问粒度。

**Raw data（段/header/VEC_INFO/buffer）规则**：
- 连续地址的 burst → 计 **1 次 IO**
- 散乱单字访问 → 每次计 **1 次 IO**
  - 注意：bs 的段访问**段间不合并**（HLS 对数据依赖地址不能跨迭代合并 burst），
    merge 整顶点段访问是编译期确定范围的连续 burst。

**Helper（FE/VC）两种模型**（用户指定，主要影响 bs insert 的散乱 helper 访问）：

- **Model A — helper burst（模拟片上缓存）**：bs 每批的 helper 流量计 **1 次 IO**
  （FE 区整体 burst 加载到片上，FE 搜索 + VC RMW 全部命中片上）。
  merge/rebalance/enlarge 的 FE/VC 更新是连续范围，按 raw data 规则计 burst。
- **Model B — helper independent**：bs 的每个散乱 FE 搜索读 + VC RMW 读写各自计 1 次 IO
  （`fe_read += Σ迭代次数`，`vc += 2×命中段数`）。连续范围仍按 burst。

**其他**：ALLOC/FREE 单独计（`alloc_roundtrip` / `free`），不混入数据 IO。

## 3. 数据格式

### PMA 文件（`data/pma/<name>.json`）— 一个已初始化的 PMA

```json
{
  "name": "vdist_uniform_edgeskew_medium",
  "pma": 0,
  "vertices": 16,
  "max_dst": 65536,
  "config": { "vertex_dist": "uniform", "edge_dist": "skewed", "scale": "medium", "seed": 42 },
  "initial_layout": {
    "vsegs": [2, 4, 2, ...],         // 每 vertex 段数（len 16，INIT 公式：pow2(ceil(count/47))，最小 1）
    "edge_count": [60, 120, ...],    // 每 vertex 初始边数
    "fe":  [5, 40, ...],             // 每段 first_edge（段内最小 dst；空段 = -1），len = sum(vsegs)
    "vc":  [40, 20, ...]             // 每段有效边数，len = sum(vsegs)
  }
}
```

初始布局按 redistribution 的 base+extra 方式生成：顶点边排序后均匀切成 vsegs 段，
前 `extra` 段每段 `base+1` 条，其余 `base` 条。

### 插入流文件（`data/insert/<name>.json`）— 带时间戳的分批插入

```json
{
  "name": "upd_skew_dst_uniform_bs16",
  "config": { "update_dist": "skewed", "dst_dist": "uniform", "batch_size": 16,
              "total_updates": 4096, "max_dst": 65536, "seed": 7 },
  "batches": [
    { "time": 0, "edges": [ {"src": 3, "dst": 512}, {"src": 7, "dst": 128}, ... ] },
    { "time": 1, "edges": [ ... ] },
    ...
  ]
}
```

- `src` ∈ 0..15（顶点号），`dst` ∈ [0, max_dst)
- **时间戳语义**：batch 按 `time` 严格递增；模拟 t 时刻只能看到 time ≤ t 的 batch，
  不能看到 t+1 的数据（模拟在线流式、wave barrier）。
- 每 batch 恰好 `batch_size` 条边（最后一批可不满）。

## 4. 参数矩阵

### Base graph（生成器 `exp_pma_gen.py`）

| 维度 | 取值 |
|---|---|
| vertex_dist | uniform（16 顶点边数接近） / skewed（Zipf，少数热顶点） |
| edge_dist（dst 值分布 → 决定哪段热） | uniform / skewed |
| scale | small（~30-80 边/顶点，1-2 段）/ medium（~500-1500，12-32 段）/ large（~4000-12000，86-256 段） |

共 2×2×3 = **12 个 PMA**。

### 更新流（生成器 `exp_insert_gen.py`）

| 维度 | 取值 |
|---|---|
| update_dist | uniform（各顶点均匀命中）/ skewed（少数热顶点） |
| dst_dist | uniform / skewed |
| batch_size | 1, 4, 16, 64, 256, 1024 |
| total_updates | 固定 4096（batch 数 = 4096/batch_size） |

共 2×2×6 = **24 个插入流**。

### 对比策略

- **bs**：所有 batch 走 bs insert 路径（GATE_POLICY=0 语义）
- **merge**：所有 batch 走 merge（GATE_POLICY=1 语义）
- （可选）**policy2**：batch 边数 >16 → merge，否则 bs（GATE_POLICY=2 默认语义）

## 5. 模拟器输出

每个 (PMA, insert 流, 策略) 输出：
- IO 总数：Model A（helper burst）与 Model B（helper independent）
- IO 分解：seg 读写、FE/VC、header、VEC_INFO、buffer、alloc/free
- 统计：rebalance 次数、enlarge 次数、bs 批数、merge 触及顶点数

汇总到 `results/`，供分析 bs vs merge 的分界（何时 merge 摊销了整顶点重写、
何时 bs 的散乱 helper 搜索成为瓶颈）。

## 6. 复现

```bash
# 生成数据
python3 tools/exp/exp_pma_gen.py --all --out tools/exp/data/pma
python3 tools/exp/exp_insert_gen.py --all --out tools/exp/data/insert
# 跑全部 288 case（12 PMA × 24 插入流 × 3 策略，~90s）
python3 tools/exp/exp_run.py
# 生成报告（读 results/io_results.csv，不重跑仿真）
python3 tools/exp/exp_analyze.py
```

输出：
- `results/io_results.csv` — 每 case 的 bs/merge/policy2 IO（两种模型）+ 分解
- `results/summary.md` — 按 batch size / scale / 更新倾斜度的聚合（exp_run 自动生成）
- `results/report.md` — 完整分析报告（exp_analyze 生成）

## 7. 结果摘要（2026-08-11 运行，seed=42）

| 模型 | bs 平均 IO | merge 平均 IO | policy2 平均 IO | merge/bs |
|---|---|---|---|---|
| Model A (helper burst) | 16036 | 20086 | 14284 | 1.25× |
| Model B (helper independent) | 40577 | 20086 | 27174 | **0.50×** |

- **当前硬件行为（Model B，FE 搜索散乱读）几乎全场景 merge 更优**——bs 的
  每边 log2(vsegs) 次 FE 散乱读主导成本。唯一 bs 胜出的场景是小型图（顶点 <2 段）
  + 极小 batch。
- **若实现片上 FE/VC 缓存（Model A）则反转**：batch <64 时 bs 更优，merge 只在
  大批量时胜出。这量化了被放弃的片上 FE 缓存方案的价值。
- **policy2（count>16→merge，当前默认）是好选择**：Model B 下贴近 merge，Model A 下
  远低于纯 merge。
- Model A 的分界 batch ~64；Model B 从 batch 1 起 merge 就赢（仅 small 图 bs1-4 例外）。
- 详见 `results/report.md`。
