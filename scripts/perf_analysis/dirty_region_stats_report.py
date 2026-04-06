#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Generate Markdown and CSV reports from DIRTY_REGION_STATS logs.

Usage:
    python scripts/perf_analysis/main.py dirty-region-report \
        --input textinput=perf_output/dirty_region_logs/textinput.log \
        --input number_picker=perf_output/dirty_region_logs/number_picker.log \
        --output-prefix perf_output/dirty_region_stats
"""

import argparse
import csv
import re
import sys
from datetime import datetime
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent
DEFAULT_OUTPUT_PREFIX = PROJECT_ROOT / "perf_output" / "dirty_region_stats"

DIRTY_REGION_PATTERN = re.compile(
    r"DIRTY_REGION_STATS:"
    r"frame=(?P<frame>\d+),"
    r"regions=(?P<regions>\d+),"
    r"dirty_area=(?P<dirty_area>\d+),"
    r"screen_area=(?P<screen_area>\d+),"
    r"pfb_tiles=(?P<pfb_tiles>\d+),"
    r"total_frames=(?P<total_frames>\d+),"
    r"total_dirty_area=(?P<total_dirty_area>\d+),"
    r"total_pfb_tiles=(?P<total_pfb_tiles>\d+)"
)


def parse_args():
    parser = argparse.ArgumentParser(
        description="Parse DIRTY_REGION_STATS logs and generate Markdown/CSV reports."
    )
    parser.add_argument(
        "--input",
        action="append",
        required=True,
        help="Input log path, or label=path for a custom source name. Repeat for multiple logs.",
    )
    parser.add_argument(
        "--output-prefix",
        default=str(DEFAULT_OUTPUT_PREFIX),
        help="Output path prefix. Generates <prefix>.md and <prefix>.csv",
    )
    parser.add_argument(
        "--title",
        default="EmbeddedGUI Dirty Region Stats Report",
        help="Markdown report title.",
    )
    return parser.parse_args()


def parse_input_spec(spec):
    if "=" in spec:
        label, path_str = spec.split("=", 1)
        label = label.strip()
        path = Path(path_str.strip())
        if label:
            return label, path

    path = Path(spec.strip())
    return path.stem, path


def parse_log_entries(path):
    entries = []
    text = read_log_text(path)
    for line in text.splitlines():
        match = DIRTY_REGION_PATTERN.search(line)
        if not match:
            continue

        entry = {key: int(value) for key, value in match.groupdict().items()}
        entry["dirty_ratio_percent"] = calc_percent(entry["dirty_area"], entry["screen_area"])
        entry["is_full_refresh"] = entry["dirty_area"] >= entry["screen_area"]
        entries.append(entry)

    return entries


def read_log_text(path):
    raw = path.read_bytes()
    encodings = ("utf-8-sig", "utf-8", "utf-16", "utf-16-le", "utf-16-be", "gbk")

    for encoding in encodings:
        try:
            text = raw.decode(encoding)
        except UnicodeDecodeError:
            continue

        normalized_text = text.replace("\x00", "")
        if "DIRTY_REGION_STATS" in normalized_text:
            return normalized_text

    return raw.decode("utf-8", errors="ignore").replace("\x00", "")


def calc_percent(numerator, denominator):
    if denominator == 0:
        return 0.0
    return numerator * 100.0 / denominator


def avg(values):
    if not values:
        return 0.0
    return sum(values) / float(len(values))


def summarize_entries(entries):
    partial_entries = [entry for entry in entries if not entry["is_full_refresh"]]
    full_entries = [entry for entry in entries if entry["is_full_refresh"]]
    last_entry = entries[-1]

    summary = {
        "frame_count": len(entries),
        "full_refresh_frames": len(full_entries),
        "partial_frames": len(partial_entries),
        "screen_area": last_entry["screen_area"],
        "avg_dirty_area": avg([entry["dirty_area"] for entry in entries]),
        "avg_dirty_ratio_percent": avg([entry["dirty_ratio_percent"] for entry in entries]),
        "avg_pfb_tiles": avg([entry["pfb_tiles"] for entry in entries]),
        "avg_partial_dirty_area": avg([entry["dirty_area"] for entry in partial_entries]),
        "avg_partial_dirty_ratio_percent": avg([entry["dirty_ratio_percent"] for entry in partial_entries]),
        "avg_partial_pfb_tiles": avg([entry["pfb_tiles"] for entry in partial_entries]),
        "last_total_dirty_area": last_entry["total_dirty_area"],
        "last_total_pfb_tiles": last_entry["total_pfb_tiles"],
        "best_partial_entry": None,
        "worst_partial_entry": None,
    }

    if partial_entries:
        summary["best_partial_entry"] = min(
            partial_entries, key=lambda entry: (entry["dirty_area"], entry["pfb_tiles"], entry["frame"])
        )
        summary["worst_partial_entry"] = max(
            partial_entries, key=lambda entry: (entry["dirty_area"], entry["pfb_tiles"], entry["frame"])
        )

    return summary


def generate_markdown_report(title, sources):
    lines = []
    lines.append(f"# {title}")
    lines.append("")
    lines.append(f"- Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    lines.append(f"- Source count: {len(sources)}")
    lines.append("")
    lines.append("| Source | Frames | Partial Frames | Avg Partial Dirty Area | Avg Partial Dirty % | Avg Partial PFB Tiles | Best Partial Frame | Best Partial Dirty % |")
    lines.append("|--------|--------|----------------|------------------------|---------------------|-----------------------|--------------------|----------------------|")

    for source in sources:
        summary = source["summary"]
        best_entry = summary["best_partial_entry"]
        best_frame = f"{best_entry['frame']}" if best_entry else "-"
        best_ratio = f"{best_entry['dirty_ratio_percent']:.2f}%" if best_entry else "-"
        lines.append(
            f"| {source['label']} | {summary['frame_count']} | {summary['partial_frames']} | "
            f"{summary['avg_partial_dirty_area']:.2f} | {summary['avg_partial_dirty_ratio_percent']:.2f}% | "
            f"{summary['avg_partial_pfb_tiles']:.2f} | {best_frame} | {best_ratio} |"
        )

    for source in sources:
        summary = source["summary"]
        best_entry = summary["best_partial_entry"]
        worst_entry = summary["worst_partial_entry"]

        lines.append("")
        lines.append(f"## {source['label']}")
        lines.append("")
        lines.append(f"- Log: `{source['path']}`")
        lines.append(f"- Screen area: {summary['screen_area']}")
        lines.append(
            f"- Frames: {summary['frame_count']} total, {summary['full_refresh_frames']} full refresh, {summary['partial_frames']} partial refresh"
        )
        lines.append(
            f"- Partial averages: dirty area {summary['avg_partial_dirty_area']:.2f}, dirty ratio {summary['avg_partial_dirty_ratio_percent']:.2f}%, pfb tiles {summary['avg_partial_pfb_tiles']:.2f}"
        )
        lines.append(
            f"- Cumulative totals: dirty area {summary['last_total_dirty_area']}, pfb tiles {summary['last_total_pfb_tiles']}"
        )
        if best_entry is not None:
            lines.append(
                f"- Best partial frame: frame {best_entry['frame']}, dirty area {best_entry['dirty_area']}, dirty ratio {best_entry['dirty_ratio_percent']:.2f}%, pfb tiles {best_entry['pfb_tiles']}"
            )
        if worst_entry is not None:
            lines.append(
                f"- Worst partial frame: frame {worst_entry['frame']}, dirty area {worst_entry['dirty_area']}, dirty ratio {worst_entry['dirty_ratio_percent']:.2f}%, pfb tiles {worst_entry['pfb_tiles']}"
            )
        lines.append("")
        lines.append("| Frame | Regions | Dirty Area | Dirty % | PFB Tiles | Full Refresh |")
        lines.append("|-------|---------|------------|---------|-----------|--------------|")
        for entry in source["entries"]:
            lines.append(
                f"| {entry['frame']} | {entry['regions']} | {entry['dirty_area']} | {entry['dirty_ratio_percent']:.2f}% | {entry['pfb_tiles']} | "
                f"{'yes' if entry['is_full_refresh'] else 'no'} |"
            )

    lines.append("")
    return "\n".join(lines)


def to_display_path(path):
    try:
        return path.relative_to(PROJECT_ROOT)
    except ValueError:
        return path


def write_csv_report(path, sources):
    fieldnames = [
        "source",
        "frame",
        "regions",
        "dirty_area",
        "screen_area",
        "dirty_ratio_percent",
        "pfb_tiles",
        "is_full_refresh",
        "total_frames",
        "total_dirty_area",
        "total_pfb_tiles",
    ]

    with open(path, "w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for source in sources:
            for entry in source["entries"]:
                writer.writerow(
                    {
                        "source": source["label"],
                        "frame": entry["frame"],
                        "regions": entry["regions"],
                        "dirty_area": entry["dirty_area"],
                        "screen_area": entry["screen_area"],
                        "dirty_ratio_percent": f"{entry['dirty_ratio_percent']:.4f}",
                        "pfb_tiles": entry["pfb_tiles"],
                        "is_full_refresh": int(entry["is_full_refresh"]),
                        "total_frames": entry["total_frames"],
                        "total_dirty_area": entry["total_dirty_area"],
                        "total_pfb_tiles": entry["total_pfb_tiles"],
                    }
                )


def main():
    args = parse_args()

    sources = []
    for spec in args.input:
        label, path = parse_input_spec(spec)
        path = path if path.is_absolute() else PROJECT_ROOT / path

        if not path.exists():
            print(f"ERROR: input log not found: {path}")
            sys.exit(1)

        entries = parse_log_entries(path)
        if not entries:
            print(f"ERROR: no DIRTY_REGION_STATS lines found in: {path}")
            sys.exit(2)

        sources.append(
            {
                "label": label,
                "path": to_display_path(path),
                "entries": entries,
                "summary": summarize_entries(entries),
            }
        )

    output_prefix = Path(args.output_prefix)
    if not output_prefix.is_absolute():
        output_prefix = PROJECT_ROOT / output_prefix
    output_prefix.parent.mkdir(parents=True, exist_ok=True)

    markdown_path = output_prefix.with_suffix(".md")
    csv_path = output_prefix.with_suffix(".csv")

    markdown_report = generate_markdown_report(args.title, sources)
    with open(markdown_path, "w", encoding="utf-8") as f:
        f.write(markdown_report)

    write_csv_report(csv_path, sources)

    print(f"Markdown report saved to: {markdown_path}")
    print(f"CSV report saved to: {csv_path}")
    print(f"Parsed sources: {len(sources)}")


if __name__ == "__main__":
    main()
