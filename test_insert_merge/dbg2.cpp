#define main test_main
#include "test.cpp"
#undef main
int main() {
    int B = 400000, N = 200000, batch = 64;
    int V = B + N;
    int S = max(16, (int)ceil(1.3 * V / 48.0));
    vector<int> vals(V);
    for (int i = 0; i < V; i++) vals[i] = i;
    std::mt19937 rng(20260807);
    shuffle(vals.begin(), vals.end(), rng);
    vector<int> expected(V);
    for (int i = 0; i < V; i++) expected[i] = i;
    // 只跑 bs, 定位 572481
    pma p(S);
    vector<int> base(vals.begin(), vals.begin() + B);
    sort(base.begin(), base.end());
    p.bulk_load_sorted_cap(base, 47);
    vector<int> scratch;
    int num_waves = (N + batch - 1) / batch;
    for (int w = 0; w < num_waves; w++) {
        int start = w * batch;
        int cnt = min(batch, N - start);
        wave_bs_insert(p, (int*)&vals[B + start], cnt);
        if (w % 500 == 0) { cerr << "wave " << w << "\n"; }
    }
    vector<int> a; pma_collect_sorted(p, a);
    size_t inv = 0;
    for (size_t i = 1; i < a.size(); i++)
        if (a[i - 1] >= a[i]) { inv = i; break; }
    cout << "first inversion at " << inv << " : " << a[inv - 1] << " >= " << a[inv] << "\n";
    // 找 572481 在哪个段
    for (int s = 0; s < S; s++) {
        if (p.seg_vc[s] == 0) continue;
        for (int i = 0; i < p.seg_vc[s]; i++)
            if (abs(p.segments[s].data[i] - 572481) <= 1)
                cout << "seg " << s << " fe=" << p.seg_fe[s] << " vc=" << p.seg_vc[s]
                     << " has " << p.segments[s].data[i] << " at idx " << i << "\n";
    }
    return 0;
}
