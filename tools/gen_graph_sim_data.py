#!/usr/bin/env python3
"""
Generate test data for graph_sim_kernel.

Input:  a file with lines of "src dst"
Output: a file with lines of "src dst timestamp opcode"

Parameters:
  --input       Path to src-dst edge list
  --output      Output file (default: stdout)
  --batch-num   Number of incremental batches
  --batch-ratio Ratio of edges used as incremental updates (0.0 ~ 1.0)
  --seed        Random seed for reproducible sampling (without replacement)

Base graph edges (timestamp = -1) are the remaining edges not sampled
for incremental batches.
"""

import argparse
import random
import sys


def main():
    parser = argparse.ArgumentParser(description="Generate graph_sim test data")
    parser.add_argument("--input", "-i", required=True, help="Input edge list file (src dst per line)")
    parser.add_argument("--output", "-o", default=None, help="Output file (default: auto-generated from input name + params)")
    parser.add_argument("--batch-num", "-n", type=int, required=True, help="Number of incremental batches")
    parser.add_argument("--batch-ratio", "-r", type=float, required=True,
                        help="Fraction of edges to use as incremental updates (0.0 ~ 1.0)")
    parser.add_argument("--seed", "-s", type=int, default=42, help="Random seed")
    args = parser.parse_args()

    if not (0.0 < args.batch_ratio <= 1.0):
        print("Error: --batch-ratio must be in (0, 1]", file=sys.stderr)
        sys.exit(1)
    if args.batch_num < 1:
        print("Error: --batch-num must be >= 1", file=sys.stderr)
        sys.exit(1)

    # Read all edges
    edges = []
    with open(args.input) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            if len(parts) >= 2:
                edges.append((int(parts[0]), int(parts[1])))

    total = len(edges)
    if total == 0:
        print("Error: no edges in input file", file=sys.stderr)
        sys.exit(1)

    batch_edge_count = int(total * args.batch_ratio)
    if batch_edge_count < args.batch_num:
        print(f"Error: batch edges ({batch_edge_count}) < batch_num ({args.batch_num})", file=sys.stderr)
        sys.exit(1)

    # Random sampling without replacement
    rng = random.Random(args.seed)
    indices = list(range(total))
    rng.shuffle(indices)

    batch_indices = set(indices[:batch_edge_count])
    base_indices = set(indices[batch_edge_count:])

    # Assign batch edges to batches (round-robin after shuffle)
    batch_edges_list = [edges[i] for i in indices[:batch_edge_count]]
    rng.shuffle(batch_edges_list)

    batches = [[] for _ in range(args.batch_num)]
    for i, e in enumerate(batch_edges_list):
        batches[i % args.batch_num].append(e)

    if not args.output:
        import os
        base = os.path.splitext(os.path.basename(args.input))[0]
        args.output = f"{base}_b{args.batch_num}_r{args.batch_ratio}_s{args.seed}.txt"
    out_fh = open(args.output, "w")

    # Write base graph edges (timestamp = -1, opcode = 0)
    for i in sorted(base_indices):
        src, dst = edges[i]
        out_fh.write(f"{src} {dst} -1 0\n")

    # Write incremental batch edges (timestamp = batch_id, opcode = 0)
    # Sort by (src, dst) within each batch so edges to the same PMA arrive consecutively,
    # maximizing write-back cache hit rate in pma_insert_edge.
    for batch_id, batch in enumerate(batches):
        batch.sort(key=lambda e: (e[0], e[1]))
        for src, dst in batch:
            out_fh.write(f"{src} {dst} {batch_id} 0\n")

    out_fh.close()

    # Summary
    base_count = len(base_indices)
    print(f"Total edges: {total}", file=sys.stderr)
    print(f"Base graph (ts=-1): {base_count} ({100*base_count/total:.1f}%)", file=sys.stderr)
    print(f"Incremental batches: {args.batch_num}", file=sys.stderr)
    for bid, b in enumerate(batches):
        print(f"  batch {bid}: {len(b)} edges", file=sys.stderr)


if __name__ == "__main__":
    main()
