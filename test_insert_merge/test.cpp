#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <queue>
#include <fstream>
#include <cmath>
#include <climits>
#include <iomanip>
#include <map>
using namespace std;

struct seg {
    int data[64];
    seg() { for (int i = 0; i < 64; i++) data[i] = -1; }
};

class pma {
public:
    vector<seg> segments;
    vector<int> seg_vc;
    vector<int> seg_fe;
    pma(int nseg = 1) {
        segments.resize(nseg);
        seg_vc.resize(nseg);
        seg_fe.resize(nseg);
        for (int i = 0; i < nseg; i++) seg_vc[i] = 0;
        for (int i = 0; i < nseg; i++) seg_fe[i] = -1;
    }

    void rebalance(int modified_seg) {
        int total_vc = 0;
        total_vc += seg_vc[modified_seg];
        if (total_vc < 48) return;
        int vertex_segments = seg_vc.size();

        int left = modified_seg;
        int right = modified_seg + 1;

        for (int group_size = 2; group_size <= vertex_segments * 2; group_size *= 2) {
            if (total_vc <= (right - left) * 48) break;

            int new_left = (modified_seg / group_size) * group_size;
            int new_right = ((modified_seg / group_size) + 1) * group_size;
            if (new_right > vertex_segments) new_right = vertex_segments;
            if (new_left == left && new_right == right) continue;

            for (int seg = new_left; seg < left; seg++) {
                #pragma HLS PIPELINE II=1
                total_vc += seg_vc[seg];
            }
            for (int seg = right; seg < new_right; seg++) {
                #pragma HLS PIPELINE II=1
                total_vc += seg_vc[seg];
            }

            left = new_left;
            right = new_right;
        }

        vector<int> tmp_edges;
        for (int seg = left; seg < right; seg++) {
            for (int i = 0; i < seg_vc[seg]; i++) tmp_edges.push_back(segments[seg].data[i]);
        }

        int base_fill = total_vc / (right - left);
        int extra_fill = total_vc % (right - left);
        int tmp_cursor = 0;
        for (int seg = left; seg < right; seg++) {
            int fill = base_fill + ((seg - left) < extra_fill ? 1 : 0);
            seg_vc[seg] = fill;
            seg_fe[seg] = tmp_edges[tmp_cursor];
            for (int i = 0; i < fill; i++) segments[seg].data[i] = tmp_edges[tmp_cursor++];
            for (int i = fill; i < 64; i++) segments[seg].data[i] = -1;
        }
    }

    void insert_edge(int edge, int& inserted_segs) {
        int seg = 0;
        {
            int vs = 0;
            int ve = seg_fe.size();
            int lo = 0, hi = ve - vs - 1;
            while (lo <= hi) {
                int mid = (lo + hi) / 2;
                int fe_val = seg_fe[mid];
                if (fe_val == -1) { hi = mid - 1; }    // 空段 (fe=-1): 不选它, 向左
                else if (fe_val == edge) { seg = mid; break; }
                else if (fe_val < edge) { lo = mid + 1; seg = mid; }
                else { hi = mid - 1; }
            }
            if (seg == -1) seg = 0;
        }
        inserted_segs = seg;
        int insert_idx = -1;
        for (int i = 0; i < seg_vc[seg]; i++) {
            if (segments[seg].data[i] > edge) break;
            else insert_idx = i;
        }
        for (int i = seg_vc[seg]; i > insert_idx + 1; i--) segments[seg].data[i] = segments[seg].data[i - 1];
        segments[seg].data[insert_idx + 1] = edge;
        seg_vc[seg]++;
        if (insert_idx + 1 == 0) seg_fe[seg] = edge;
    }

    void insert_batch(int* edges, int count) {
        int inserted_segs[16];
        int loops = count / 16 + (count % 16 != 0 ? 1 : 0);
        int remain_tasks = count;
        for (int l = 0; l < loops; l++) {
            int batch_size = (remain_tasks > 16) ? 16 : remain_tasks;
            remain_tasks -= batch_size;
            for (int i = 0; i < batch_size; i++) insert_edge(edges[l * 16 + i], inserted_segs[i]);
            for (int i = 0; i < batch_size; i++) rebalance(inserted_segs[i]);
        }
    }

    // Bulk load a fully sorted array into segments sequentially (cap 每段最多 cap 条, 默认 64)
    void bulk_load_sorted_cap(const vector<int>& data, int cap) {
        size_t n = data.size();
        size_t idx = 0;
        for (size_t s = 0; s < segments.size(); s++) {
            int fill = 0;
            if (idx < n) {
                fill = (int)min<size_t>(cap, n - idx);
                seg_fe[s] = data[idx];
            } else {
                fill = 0;
                seg_fe[s] = -1;
            }
            seg_vc[s] = fill;
            for (int i = 0; i < fill; i++) segments[s].data[i] = data[idx++];
            for (int i = fill; i < 64; i++) segments[s].data[i] = -1;
            if (idx >= n) {
                for (size_t t = s + 1; t < segments.size(); t++) {
                    seg_vc[t] = 0; seg_fe[t] = -1;
                    for (int i = 0; i < 64; i++) segments[t].data[i] = -1;
                }
                break;
            }
        }
    }

    void bulk_load_sorted(const vector<int>& data) { bulk_load_sorted_cap(data, 64); }

    // Merge multiple sorted arrays into this PMA efficiently (k-way merge then bulk load)
    void merge_sorted_arrays(const vector<vector<int>>& parts) {
        size_t total = 0;
        for (const auto& p : parts) total += p.size();
        vector<int> merged;
        merged.reserve(total);
        struct Node { int val; int ai; int idx; };
        struct Cmp { bool operator()(const Node& a, const Node& b) const { return a.val > b.val; } };
        std::priority_queue<Node, vector<Node>, Cmp> pq;
        for (int i = 0; i < (int)parts.size(); i++) if (!parts[i].empty()) pq.push({parts[i][0], i, 0});
        while (!pq.empty()) {
            Node n = pq.top(); pq.pop();
            merged.push_back(n.val);
            int ai = n.ai; int ni = n.idx + 1;
            if (ni < (int)parts[ai].size()) pq.push({parts[ai][ni], ai, ni});
        }
        bulk_load_sorted(merged);
    }

    long long total_count() const {
        long long t = 0;
        for (int v : seg_vc) t += v;
        return t;
    }
};

// =====================================================================
//  波次处理路径 (wave = 一批到达的边; 真实系统中波先被 radix 排序)
// =====================================================================

// 收集 PMA 的逻辑有序数组 (segment 按值分区时全局有序)
static void pma_collect_sorted(const pma& p, vector<int>& out) {
    out.clear();
    for (size_t s = 0; s < p.segments.size(); s++)
        for (int i = 0; i < p.seg_vc[s]; i++) out.push_back(p.segments[s].data[i]);
}

// ---- Path A: bs+insert (逐边 FE 二分 + 段内扫描移位 + rebalance) ----
// 波内容为原始到达顺序 (未排序)。这是当前系统的行为: 排序白付, 插入不利用有序性。
static void wave_bs_insert(pma& p, int* wave, int cnt) {
    p.insert_batch(wave, cnt);
}

// ---- Path B: 全量重建, k-way 堆归并 (每段一个有序 slice + 波) ----
static void wave_rebuild_kway(pma& p, const vector<int>& wave) {
    vector<vector<int>> parts;
    parts.reserve(p.segments.size() + 1);
    for (size_t s = 0; s < p.segments.size(); s++)
        if (p.seg_vc[s] > 0)
            parts.emplace_back(p.segments[s].data, p.segments[s].data + p.seg_vc[s]);
    parts.push_back(wave);
    p.merge_sorted_arrays(parts);
}

// ---- Path C: 全量重建, 线性二路归并 (PMA 逻辑数组全局有序, 直接与波归并) ----
static void wave_rebuild_linear(pma& p, const vector<int>& wave) {
    vector<int> existing;
    pma_collect_sorted(p, existing);
    vector<int> merged(existing.size() + wave.size());
    std::merge(existing.begin(), existing.end(), wave.begin(), wave.end(), merged.begin());
    p.bulk_load_sorted_cap(merged, 64);
}

// ---- Path D: 段级增量归并 (sorted command 最优形态: 只归并命中的段, 不重建) ----
// 波必须已排序。利用 FE 数组的值分区:
//   upper[s] = 段 s 之后第一个非空段的 fe (或 INT_MAX)
//   段 s 的范围 = [upper[s-1], upper[s]) (空段的范围为空, 归并到相邻段)
// 波中落在该范围的边是连续切片, 与段内已有数据二路归并。
// 溢出 (合并结果 > 64) 时只重建 [s, S) 尾部 (线性归并 + cap=47 铺开), 分区自适应。
static void wave_seg_merge(pma& p, const vector<int>& wave) {
    const int S = (int)p.segments.size();
    const int W = (int)wave.size();

    // upper[s] = s 右侧第一个非空段的 fe
    vector<int> upper(S, INT_MAX);
    int nxt = INT_MAX;
    for (int s = S - 1; s >= 0; s--) {
        upper[s] = nxt;
        if (p.seg_fe[s] != -1) nxt = p.seg_fe[s];
    }

    int wpos = 0;
    for (int s = 0; s < S; s++) {
        if (wpos >= W) break;
        int hi_s = upper[s];
        int ws = wpos;
        while (wpos < W && wave[wpos] < hi_s) wpos++;
        if (wpos == ws) continue;                      // 本段范围无新边
        int vc = p.seg_vc[s];
        vector<int> merged;
        merged.reserve(vc + (wpos - ws));
        std::merge(p.segments[s].data, p.segments[s].data + vc,
                   wave.begin() + ws, wave.begin() + wpos,
                   back_inserter(merged));
        if ((int)merged.size() > 64) {
            // 局部重建: 段 s 溢出 → 重建 [s, S)。
            // 内容 = 本次合并结果 merged (段 s 旧数据 + 切片, 不能丢)
            //       + 剩余波 [wpos, W)   (merged.max < upper[s] <= wave[wpos] → 拼接有序)
            //       + [s+1, S) 的现有数据 (值 >= upper[s] → 有序)
            // 三部分两两有序, 先拼 rest 再与 old_tail 线性归并, 按 cap=47 铺回 [s, S)。
            vector<int> rest;
            rest.reserve(merged.size() + (W - wpos));
            rest.insert(rest.end(), merged.begin(), merged.end());
            rest.insert(rest.end(), wave.begin() + wpos, wave.end());
            vector<int> old_tail;
            for (int t = s + 1; t < S; t++)
                for (int i = 0; i < p.seg_vc[t]; i++) old_tail.push_back(p.segments[t].data[i]);
            vector<int> tail;
            tail.reserve(old_tail.size() + rest.size());
            std::merge(old_tail.begin(), old_tail.end(), rest.begin(), rest.end(),
                       back_inserter(tail));
            size_t idx = 0;
            for (int t = s; t < S; t++) {
                int fill = 0;
                if (idx < tail.size()) {
                    fill = (int)min<size_t>(47, tail.size() - idx);
                    p.seg_fe[t] = tail[idx];
                } else {
                    p.seg_fe[t] = -1;
                }
                p.seg_vc[t] = fill;
                for (int i = 0; i < fill; i++) p.segments[t].data[i] = tail[idx++];
                for (int i = fill; i < 64; i++) p.segments[t].data[i] = -1;
            }
            return;   // 尾部已全部处理完
        }
        for (int i = 0; i < (int)merged.size(); i++) p.segments[s].data[i] = merged[i];
        for (int i = (int)merged.size(); i < 64; i++) p.segments[s].data[i] = -1;
        p.seg_vc[s] = (int)merged.size();
        p.seg_fe[s] = merged.empty() ? -1 : merged[0];
    }
    // 第二阶段: 延迟 rebalance。波内绝不能中途 rebalance — 它改变段边界,
    // 会使剩余波的切片与旧边界错位 (值落入 [旧边界, 新边界) 空隙, 放错段)。
    // 全部 merge 完成后再统一 rebalance (re-split 保持组内有序, 不破坏全局序)。
    for (int s = 0; s < S; s++)
        if (p.seg_vc[s] >= 48) p.rebalance(s);
}

// =====================================================================
//  实验框架
// =====================================================================

using clk = std::chrono::steady_clock;
static long long ms_of(clk::time_point a, clk::time_point b) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(b - a).count();
}

// 跑一条路径的完整波次实验, 返回耗时 ms; 结束时校验 PMA 内容 == expected
static long long run_path(
    const string& path,
    int B,                       // 预加载 base 边数 (0 = 空 PMA 起步)
    int N,                       // 波次插入总边数
    int batch_size,              // 每波大小
    int S,                       // segment 数
    const vector<int>& wave_vals,// 长度 B+N: 前 B 个是 base (乱序), 后 N 个是波的到达序列
    const vector<int>& expected  // 期望最终全量有序数组
) {
    pma p(S);
    if (B > 0) {
        vector<int> base(wave_vals.begin(), wave_vals.begin() + B);
        sort(base.begin(), base.end());          // bulk_load 要求有序输入
        p.bulk_load_sorted_cap(base, 47);
    }

    int num_waves = (N + batch_size - 1) / batch_size;
    vector<int> scratch;   // 排序变体的临时缓冲

    auto t0 = clk::now();
    for (int w = 0; w < num_waves; w++) {
        int start = w * batch_size;
        int cnt = min(batch_size, N - start);
        const int* wv = &wave_vals[B + start];
        if (path == "bs") {
            wave_bs_insert(p, (int*)wv, cnt);
        } else if (path == "bs_sorted") {
            scratch.assign(wv, wv + cnt);
            sort(scratch.begin(), scratch.end());
            wave_bs_insert(p, scratch.data(), cnt);
        } else if (path == "seg_merge") {
            scratch.assign(wv, wv + cnt);
            sort(scratch.begin(), scratch.end());
            wave_seg_merge(p, scratch);
        } else if (path == "rebuild_kway") {
            scratch.assign(wv, wv + cnt);
            sort(scratch.begin(), scratch.end());
            wave_rebuild_kway(p, scratch);
        } else if (path == "rebuild_linear") {
            scratch.assign(wv, wv + cnt);
            sort(scratch.begin(), scratch.end());
            wave_rebuild_linear(p, scratch);
        } else {
            cerr << "unknown path " << path << "\n";
            return -1;
        }
    }
    auto t1 = clk::now();

    vector<int> actual;
    pma_collect_sorted(p, actual);
    if (actual.size() != expected.size() ||
        !equal(actual.begin(), actual.end(), expected.begin())) {
        size_t mm = 0;
        while (mm < actual.size() && mm < expected.size() && actual[mm] == expected[mm]) mm++;
        cerr << "FAIL path=" << path << " B=" << B << " N=" << N
             << " batch=" << batch_size << " S=" << S
             << " got " << actual.size() << " expected " << expected.size()
             << " first_mismatch=" << mm;
        if (mm > 0) {
            cerr << "  actual[mm-2..+2]=" << actual[mm - 1] << "," << actual[mm] << ","
                 << (mm + 1 < actual.size() ? to_string(actual[mm + 1]) : "x") << ","
                 << (mm + 2 < actual.size() ? to_string(actual[mm + 2]) : "x")
                 << "  expected=" << expected[mm] << ",";
        }
        cerr << "\n";
        return -1;
    }
    return ms_of(t0, t1);
}

// 主扫掠: N × batch × S × 路径, 输出 CSV + 交叉点分析
// 每个 (case, batch, path) 只跑一次; 结果存表后统一做交叉点分析。
static void run_wave_sweep() {
    // 用例: (预加载 B, 插入 N)
    vector<pair<int, int>> cases = {
        {0, 200000},
        {0, 800000},
        {0, 3200000},
        {400000, 200000},   // 预加载 2N 的 base, 模拟已有图上的增量
    };
    vector<int> batch_sizes = {64, 512, 4096, 32768, 262144};
    const double REBUILD_MAX_OPS = 5e9;    // 全量重建路径的操作数护栏 (避免 O(total)/波 × 多波 太慢)

    ofstream out("wave_sweep.csv");
    out << "B,N,total_v,batch,num_waves,S,path,ms\n";
    cout << fixed << setprecision(1);

    for (const auto& c : cases) {
        int B = c.first, N = c.second;
        int V = B + N;
        int S = max(16, (int)ceil(1.3 * V / 48.0));

        // 值空间 [0, V): 前 B 个为 base, 后 N 个为波的到达序列
        vector<int> vals(V);
        for (int i = 0; i < V; i++) vals[i] = i;
        std::mt19937 rng(20260807);
        shuffle(vals.begin(), vals.end(), rng);
        vector<int> expected(V);
        for (int i = 0; i < V; i++) expected[i] = i;

        cout << "\n=== case B=" << B << " N=" << N << " (V=" << V << ") S=" << S << " ===\n" << flush;

        // 结果表: [batch][path] -> ms
        map<int, map<string, long long>> table;
        for (int batch : batch_sizes) {
            if (batch > N) continue;
            int num_waves = (N + batch - 1) / batch;
            vector<string> paths = {"bs", "bs_sorted"};
            // seg_merge: 空 PMA 冷启动 (B=0) 时值分区尚未建立, 每波都会集中到边界段
            // → 逐波局部重建, 代价 ≈ 每波 O(S + 已填充量)。非真实系统行为, 超护栏跳过。
            if (num_waves * (double)(S + V) <= REBUILD_MAX_OPS) paths.push_back("seg_merge");
            // rebuild 路径操作数估计: 每波 ~3V (linear) 或 ~V(2+log2 S) (kway)
            // 注意: 必须先转 double, num_waves*V 在 int 里会溢出 (3.2M×6250 = 2e10 > 2^31)
            if (num_waves * 3.0 * V <= REBUILD_MAX_OPS) paths.push_back("rebuild_linear");
            if ((double)num_waves * V * (2.0 + log2((double)S)) <= REBUILD_MAX_OPS) paths.push_back("rebuild_kway");

            cout << "  batch=" << setw(6) << batch << " waves=" << setw(6) << num_waves;
            for (const string& path : paths) {
                long long ms = run_path(path, B, N, batch, S, vals, expected);
                if (ms < 0) { cerr << "  path " << path << " FAILED\n"; continue; }
                table[batch][path] = ms;
                cout << "  " << setw(13) << path << "=" << setw(7) << ms << "ms";
                out << B << "," << N << "," << V << "," << batch << "," << num_waves << ","
                    << S << "," << path << "," << ms << "\n";
                out.flush();
            }
            cout << "\n" << flush;
        }

        // ---- 交叉点分析 (用表, 不再重跑) ----
        for (int batch : batch_sizes) {
            if (batch > N) continue;
            auto& row = table[batch];
            if (!row.count("bs")) continue;
            long long t_bs = row["bs"];
            long long t_seg = row.count("seg_merge") ? row["seg_merge"] : -1;
            long long t_lin = row.count("rebuild_linear") ? row["rebuild_linear"] : -1;
            string flags;
            if (t_seg >= 0 && t_seg < t_bs) flags += "  <-- seg_merge 胜出";
            if (t_lin >= 0 && t_lin < t_bs) flags += "  <-- rebuild_linear 胜出";
            cout << "  [crossover] batch=" << setw(6) << batch
                 << " bs=" << setw(7) << t_bs << "ms"
                 << " seg_merge=" << setw(7) << (t_seg >= 0 ? to_string(t_seg) : "-") << "ms"
                 << " rebuild_lin=" << setw(7) << (t_lin >= 0 ? to_string(t_lin) : "-") << "ms"
                 << flags << "\n" << flush;
        }
        // 解析预测: 交叉点 batch* ≈ S / log2(S)
        double pred = S / log2((double)S);
        cout << "  [model] S/log2(S) = " << (long long)pred
             << "  (预测 seg_merge 从 ~此批大小起优于 bs)\n" << flush;
    }
    out.close();
    cout << "\nCSV written to wave_sweep.csv\n";
}

int main() {
    // ---- 原始 sanity 测试 (空 PMA 起步, bs+insert, 校验) ----
    const int N = 480000;
    pma my_pma(10000);
    int *edges = new int[N];
    for (int i = 0; i < N; i++) edges[i] = i;

    auto t_start = clk::now();
    shuffle(edges, edges + N, std::default_random_engine(0));
    auto t_after_shuffle = clk::now();

    my_pma.insert_batch(edges, N);
    auto t_after_insert = clk::now();

    sort(edges, edges + N);
    vector<int> actual;
    actual.reserve(N);
    long long total = 0;
    for (size_t s = 0; s < my_pma.segments.size(); s++) {
        int vc = my_pma.seg_vc[s];
        total += vc;
        for (int j = 0; j < vc; j++) actual.push_back(my_pma.segments[s].data[j]);
    }

    if (total != N) { cerr << "FAIL: total=" << total << " expected=" << N << "\n"; return 2; }
    bool correct = equal(actual.begin(), actual.end(), edges);
    cout << "Sanity (N=" << N << "): insert=" << ms_of(t_after_shuffle, t_after_insert)
         << "ms  " << (correct ? "PASS" : "FAIL") << "\n";
    delete[] edges;

    // ---- 波次扫掠实验 ----
    run_wave_sweep();
    return correct ? 0 : 1;
}
