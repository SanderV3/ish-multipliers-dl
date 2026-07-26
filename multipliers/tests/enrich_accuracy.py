#!/usr/bin/env python3
"""
Enrich the AdaPT / TransAxx accuracy CSVs with Δ-from-baseline columns.

Appends `delta_top1` (and `delta_top5` when present) to the original CSV,
preserving row order. Original columns and order are untouched, so any
downstream code that reads the CSV by name keeps working.

Usage:
    python3 enrich_accuracy.py adapt/examples/results.csv
    python3 enrich_accuracy.py transaxx/examples/transaxx_results.csv
"""

import csv
import sys
from pathlib import Path


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__.strip())
    path = Path(sys.argv[1])
    rows = list(csv.DictReader(open(path)))
    if not rows:
        sys.exit(f"empty CSV: {path}")

    # Baseline = first row whose name is one of the exact variants.
    baseline = next(
        (r for r in rows if r["axx_mult"] in ("mul8s_acc", "mul8s_exact")),
        None,
    )
    if baseline is None:
        sys.exit("no baseline row (mul8s_acc / mul8s_exact) found")

    base_top1 = float(baseline["top1"])
    has_top5  = "top5" in baseline
    base_top5 = float(baseline["top5"]) if has_top5 else None

    # Build enriched rows in the original order.
    original_cols = list(rows[0].keys())
    new_cols      = ["delta_top1"] + (["delta_top5"] if has_top5 else [])
    out_cols      = original_cols + new_cols

    enriched = []
    for r in rows:
        new = dict(r)
        new["delta_top1"] = f"{float(r['top1']) - base_top1:+.4f}"
        if has_top5:
            new["delta_top5"] = f"{float(r['top5']) - base_top5:+.4f}"
        enriched.append(new)

    # Write back (overwrite in place).
    with open(path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=out_cols)
        writer.writeheader()
        writer.writerows(enriched)

    print(f"Wrote {path}  (+{len(new_cols)} columns: {', '.join(new_cols)})")

    # Echo a sorted summary so you can eyeball the result.
    print()
    enriched_sorted = sorted(enriched, key=lambda r: -float(r["delta_top1"]))
    if has_top5:
        print(f"{'axx_mult':<22} {'top1':>8} {'Δ top1':>10}   {'top5':>8} {'Δ top5':>10}")
        for r in enriched_sorted:
            print(f"{r['axx_mult']:<22} {float(r['top1']):>8.4f} {r['delta_top1']:>10}   "
                  f"{float(r['top5']):>8.4f} {r['delta_top5']:>10}")
    else:
        print(f"{'axx_mult':<22} {'top1':>8} {'Δ top1':>10}")
        for r in enriched_sorted:
            print(f"{r['axx_mult']:<22} {float(r['top1']):>8.4f} {r['delta_top1']:>10}")


if __name__ == "__main__":
    main()
