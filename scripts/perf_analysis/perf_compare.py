#!/usr/bin/env python3
"""Compare two perf JSON files and output a markdown table."""

import json
import sys
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[2]


def resolve_input(path_text: str, default_path: Path) -> Path:
    path = Path(path_text) if path_text else default_path
    if path.is_absolute():
        return path
    return PROJECT_ROOT / path

def main():
    baseline_path = resolve_input(
        sys.argv[1] if len(sys.argv) > 1 else "",
        PROJECT_ROOT / "perf_output" / "perf_baseline_999bb88.json",
    )
    current_path = resolve_input(
        sys.argv[2] if len(sys.argv) > 2 else "",
        PROJECT_ROOT / "perf_output" / "perf_results.json",
    )

    with open(baseline_path, "r", encoding="utf-8") as baseline_file:
        b = json.load(baseline_file)["profiles"]["cortex-m3"]
    with open(current_path, "r", encoding="utf-8") as current_file:
        c = json.load(current_file)["profiles"]["cortex-m3"]

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
