#!/usr/bin/env python3
"""Generate a single-batch ops file (all ts=0) where some PMAs get <=16 new
edges (bs path under GATE_POLICY=2) and others get >16 (merge path), so the
count>16 routing exercises BOTH paths. Format: 'src dst ts opcode' per line.
"""
import random, sys

def main():
    seed = int(sys.argv[1]) if len(sys.argv) > 1 else 7
    out = sys.argv[2] if len(sys.argv) > 2 else "mixed_pma_ops.txt"
    random.seed(seed)
    lines = []
    total = 0
    # 64 PMAs -> 1024 vertices. Per-PMA TOTAL edge count decides the path
    # under GATE_POLICY=2: total <=16 -> bs, >16 -> merge.
    for pma in range(64):
        vbase = pma * 16
        if pma % 2 == 0:
            # small PMA: 1..15 edges total (1-2 vertices touched) -> bs
            c = random.randint(1, 15)
            v = random.randrange(16)
            for _ in range(c):
                dst = random.randint(0, 1023)
                if dst == vbase + v:
                    dst = (dst + 1) % 1024
                lines.append(f"{vbase + v} {dst} 0 0")
                total += 1
        else:
            # large PMA: 40..150 edges total, spread over several vertices -> merge
            for v in random.sample(range(16), 8):
                for _ in range(random.randint(5, 20)):
                    dst = random.randint(0, 1023)
                    if dst == vbase + v:
                        dst = (dst + 1) % 1024
                    lines.append(f"{vbase + v} {dst} 0 0")
                    total += 1
    random.shuffle(lines)
    with open(out, "w") as f:
        f.write("\n".join(lines) + "\n")
    print(f"wrote {total} edges to {out} (64 PMAs, alternating small/large)")

if __name__ == "__main__":
    main()
