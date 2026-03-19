#!/usr/bin/env python3
"""Compare two perf JSON files and output a markdown table."""
import json
import sys

def main():
    baseline_path = sys.argv[1] if len(sys.argv) > 1 else "perf_output/perf_baseline_999bb88.json"
    current_path = sys.argv[2] if len(sys.argv) > 2 else "perf_output/perf_results.json"

    b = json.load(open(baseline_path))["profiles"]["cortex-m3"]
    c = json.load(open(current_path))["profiles"]["cortex-m3"]

    header = "| Test | Baseline(ms) | Current(ms) | Delta(ms) | Change% |"
    sep    = "|------|-------------|-------------|-----------|---------|"
    print(header)
    print(sep)

    total_b, total_c = 0.0, 0.0
    significant = []
    for k in sorted(b.keys()):
        bv = b[k]["time_ms"]
        cv = c[k]["time_ms"]
        total_b += bv
        total_c += cv
        d = cv - bv
        pct = d / bv * 100 if bv else 0
        flag = " **" if abs(pct) > 2.0 else ""
        print(f"| {k:<28} | {bv:>11.3f} | {cv:>11.3f} | {d:>+9.3f} | {pct:>+7.1f}% |{flag}")
        if abs(pct) > 2.0:
            significant.append((k, bv, cv, pct))

    d = total_c - total_b
    pct = d / total_b * 100
    print(sep)
    print(f"| {'TOTAL':<28} | {total_b:>11.3f} | {total_c:>11.3f} | {d:>+9.3f} | {pct:>+7.1f}% |")

    print("\n### Significant changes (>2%):")
    for k, bv, cv, pct in sorted(significant, key=lambda x: x[3]):
        print(f"- **{k}**: {bv:.3f} -> {cv:.3f} ms ({pct:+.1f}%)")

if __name__ == "__main__":
    main()
