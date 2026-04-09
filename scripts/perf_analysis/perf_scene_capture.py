#!/usr/bin/env python3
"""Capture HelloPerformance scenes and compose a contact sheet."""

from __future__ import annotations

import argparse
import json
import math
import platform
import re
import subprocess
import sys
import textwrap
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont, ImageOps


SCRIPT_DIR = Path(__file__).resolve().parent
SCRIPTS_ROOT = SCRIPT_DIR.parent
PROJECT_ROOT = SCRIPTS_ROOT.parent
PERF_OUTPUT_DIR = PROJECT_ROOT / "perf_output"
DEFAULT_RESULTS_PATH = PERF_OUTPUT_DIR / "perf_results.json"
DEFAULT_FRAMES_DIR = PERF_OUTPUT_DIR / "perf_scene_frames"
DEFAULT_SHEET_PATH = PERF_OUTPUT_DIR / "perf_scenes.png"
DEFAULT_INDEX_PATH = PERF_OUTPUT_DIR / "perf_scenes_index.json"

SCENE_PATTERN = re.compile(r"PERF_SCENE:(\w+)")
FRAME_PATTERN = re.compile(r"PERF_FRAME:(frame_\d+\.png):(\w+)")
REQUIRED_CFLAGS = "-DEGUI_CONFIG_RECORDING_TEST=1"
BUILD_TARGET = "perf_scene_capture"
BUILD_OBJ_SUFFIX = "HelloPerformance_perf_scene_capture"

BACKGROUND_COLOR = "#f4f6f8"
CARD_COLOR = "#ffffff"
CARD_BORDER_COLOR = "#d7dde2"
TITLE_COLOR = "#15202b"
SUBTITLE_COLOR = "#5b6670"
HEADER_COLOR = "#243447"
HEADER_TEXT_COLOR = "#ffffff"
TIME_COLOR = "#0f766e"
LABEL_COLOR = "#1f2933"
IMAGE_LANCZOS = getattr(Image, "Resampling", Image).LANCZOS


if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))
if str(SCRIPTS_ROOT) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_ROOT))

import code_runtime_check as runtime_check
from perf_to_doc import _get_categorized_tests


@dataclass
class SceneEntry:
    name: str
    category: str
    time_ms: float | None
    frame_path: Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Capture HelloPerformance scenes and build a contact sheet")
    parser.add_argument("--results", default=str(DEFAULT_RESULTS_PATH), help="Path to perf_results.json")
    parser.add_argument("--profile", default="", help="Profile name to read from perf_results.json")
    parser.add_argument("--output", default=str(DEFAULT_SHEET_PATH), help="Output PNG path for the contact sheet")
    parser.add_argument("--index", default=str(DEFAULT_INDEX_PATH), help="Output JSON path for the scene index")
    parser.add_argument("--frames-dir", default=str(DEFAULT_FRAMES_DIR), help="Directory for captured frame PNGs")
    parser.add_argument("--columns", type=int, default=5, help="Number of columns in the contact sheet")
    parser.add_argument("--tile-width", type=int, default=160, help="Rendered tile width in pixels")
    parser.add_argument("--timeout", type=int, default=120, help="Capture command timeout in seconds")
    parser.add_argument("--duration", type=int, default=90, help="Recording duration in seconds")
    parser.add_argument("--clock-scale", type=int, default=runtime_check.RECORDING_CLOCK_SCALE, help="Recording clock scale")
    parser.add_argument("--snapshot-settle-ms", type=int, default=runtime_check.RECORDING_SNAPSHOT_SETTLE_MS, help="Snapshot settle delay")
    parser.add_argument("--snapshot-stable-cycles", type=int, default=runtime_check.RECORDING_SNAPSHOT_STABLE_CYCLES, help="Stable frame cycles before snapshot")
    parser.add_argument("--snapshot-max-wait-ms", type=int, default=runtime_check.RECORDING_SNAPSHOT_MAX_WAIT_MS, help="Snapshot max wait")
    parser.add_argument("--filter", default="", help="Comma-separated substrings used to filter the final sheet")
    return parser.parse_args()


def get_windows_hidden_run_kwargs() -> dict:
    if platform.system() != "Windows":
        return {}

    return {}


def read_perf_results(path: Path, profile_name: str, keyword_filter: str) -> tuple[dict[str, dict], str, str]:
    if not path.exists():
        return {}, "unknown", "unknown"

    data = json.loads(path.read_text(encoding="utf-8"))
    profiles = data.get("profiles", {})
    if not profiles:
        return {}, data.get("git_commit", "unknown"), profile_name or "unknown"

    if profile_name:
        if profile_name not in profiles:
            raise ValueError(f"profile not found in results: {profile_name}")
        selected_profile = profile_name
    else:
        selected_profile = next(iter(profiles.keys()))

    results = dict(profiles[selected_profile])
    if keyword_filter.strip():
        keywords = [item.strip().upper() for item in keyword_filter.split(",") if item.strip()]
        results = {
            name: value
            for name, value in results.items()
            if any(keyword in name.upper() for keyword in keywords)
        }

    return results, data.get("git_commit", "unknown"), selected_profile


def get_git_commit() -> str:
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--short", "HEAD"],
            cwd=PROJECT_ROOT,
            capture_output=True,
            text=True,
            check=True,
        )
    except Exception:
        return "unknown"
    return (result.stdout or "").strip() or "unknown"


def flatten_categories(available_tests: set[str]) -> tuple[list[str], dict[str, str]]:
    ordered_names: list[str] = []
    category_map: dict[str, str] = {}
    for category_name, tests in _get_categorized_tests(available_tests):
        for name in tests:
            ordered_names.append(name)
            category_map[name] = category_name
    return ordered_names, category_map


def build_capture_app() -> None:
    print("Building HelloPerformance capture app...")

    clean_result = subprocess.run(["make", "clean"], cwd=PROJECT_ROOT, capture_output=True, text=True)
    if clean_result.returncode != 0:
        raise RuntimeError("make clean failed before scene capture build")

    cmd = ["make", "-j", "APP=HelloPerformance", "PORT=pc", f"TARGET={BUILD_TARGET}", f"APP_OBJ_SUFFIX={BUILD_OBJ_SUFFIX}"]
    cmd.extend(runtime_check.COMPILE_FAST_FLAGS)
    cmd.append(f"USER_CFLAGS={REQUIRED_CFLAGS}")

    result = subprocess.run(cmd, cwd=PROJECT_ROOT, capture_output=True, text=True)
    if result.returncode != 0:
        combined = ((result.stdout or "") + "\n" + (result.stderr or "")).strip()
        raise RuntimeError("scene capture build failed\n" + combined[-4000:])


def run_capture(frames_dir: Path, args: argparse.Namespace) -> tuple[list[str], dict[str, Path]]:
    frames_dir.mkdir(parents=True, exist_ok=True)
    for old_frame in frames_dir.glob("frame_*.png"):
        old_frame.unlink()

    if platform.system() == "Windows":
        exe_path = PROJECT_ROOT / "output" / f"{BUILD_TARGET}.exe"
    else:
        exe_path = PROJECT_ROOT / "output" / BUILD_TARGET

    if not exe_path.exists():
        raise RuntimeError(f"capture executable not found: {exe_path}")

    resource_path = PROJECT_ROOT / "output" / "app_egui_resource_merge.bin"
    if not resource_path.exists():
        raise RuntimeError(f"resource file not found: {resource_path}")

    cmd = [
        str(exe_path),
        str(resource_path),
        "--record",
        str(frames_dir),
        str(runtime_check.RECORDING_FPS),
        str(args.duration),
        "--speed",
        "1",
        "--clock-scale",
        str(args.clock_scale),
        "--snapshot-settle-ms",
        str(args.snapshot_settle_ms),
        "--snapshot-stable-cycles",
        str(args.snapshot_stable_cycles),
        "--snapshot-max-wait-ms",
        str(args.snapshot_max_wait_ms),
        "--headless",
    ]

    print("Running scene capture...")
    result = subprocess.run(
        cmd,
        cwd=PROJECT_ROOT,
        timeout=args.timeout,
        capture_output=True,
        text=True,
        **get_windows_hidden_run_kwargs(),
    )
    if result.returncode != 0:
        combined = ((result.stdout or "") + "\n" + (result.stderr or "")).strip()
        raise RuntimeError("scene capture run failed\n" + combined[-4000:])

    combined_output = f"{result.stdout or ''}\n{result.stderr or ''}"
    scene_names = SCENE_PATTERN.findall(combined_output)
    frame_paths = sorted(frames_dir.glob("frame_*.png"))

    if not scene_names:
        raise RuntimeError("no PERF_SCENE markers found in capture output")
    if not frame_paths:
        raise RuntimeError("no capture frames were generated")

    validate_frames(frame_paths)
    frame_map = build_frame_map(scene_names, frame_paths, combined_output)
    return scene_names, frame_map


def frames_have_same_pixels(path_a: Path, path_b: Path) -> bool:
    with Image.open(path_a) as image_a, Image.open(path_b) as image_b:
        return image_a.size == image_b.size and image_a.tobytes() == image_b.tobytes()


def normalize_frame_paths(scene_names: list[str], frame_paths: list[Path]) -> list[Path]:
    scene_count = len(scene_names)
    frame_count = len(frame_paths)

    if frame_count == scene_count:
        return frame_paths

    if frame_count == scene_count + 2 and frames_have_same_pixels(frame_paths[0], frame_paths[1]) and frames_have_same_pixels(frame_paths[-1], frame_paths[-2]):
        print("Normalizing capture frames: dropping leading and trailing duplicate snapshots.")
        return frame_paths[1:-1]

    if frame_count == scene_count + 1:
        if frames_have_same_pixels(frame_paths[0], frame_paths[1]):
            print("Normalizing capture frames: dropping leading duplicate snapshot.")
            return frame_paths[1:]
        if frames_have_same_pixels(frame_paths[-1], frame_paths[-2]):
            print("Normalizing capture frames: dropping trailing duplicate snapshot.")
            return frame_paths[:-1]

    return frame_paths


def build_frame_map(scene_names: list[str], frame_paths: list[Path], capture_output: str) -> dict[str, Path]:
    explicit_map = parse_explicit_frame_map(frame_paths, capture_output)
    if explicit_map:
        missing = [name for name in scene_names if name not in explicit_map]
        if missing:
            raise RuntimeError("missing PERF_FRAME markers for scenes: " + ", ".join(missing))
        print("Using explicit PERF_FRAME markers for scene-to-frame mapping.")
        return explicit_map

    normalized_paths = normalize_frame_paths(scene_names, frame_paths)
    if len(normalized_paths) != len(scene_names):
        raise RuntimeError(f"scene/frame count mismatch: {len(scene_names)} scenes vs {len(normalized_paths)} frames")

    return {name: frame for name, frame in zip(scene_names, normalized_paths)}


def parse_explicit_frame_map(frame_paths: list[Path], capture_output: str) -> dict[str, Path]:
    matches = FRAME_PATTERN.findall(capture_output)
    if not matches:
        return {}

    frames_by_name = {path.name: path for path in frame_paths}
    scene_frames: dict[str, list[Path]] = {}
    missing_frames: list[str] = []

    for frame_name, scene_name in matches:
        frame_path = frames_by_name.get(frame_name)
        if frame_path is None:
            missing_frames.append(frame_name)
            continue
        scene_frames.setdefault(scene_name, []).append(frame_path)

    if missing_frames:
        raise RuntimeError("PERF_FRAME markers referenced unknown files: " + ", ".join(sorted(set(missing_frames))))

    explicit_map: dict[str, Path] = {}
    for scene_name, scene_frame_paths in scene_frames.items():
        if len(scene_frame_paths) > 1:
            print(f"Scene {scene_name} produced {len(scene_frame_paths)} labeled snapshots; using the first one.")
        explicit_map[scene_name] = scene_frame_paths[0]

    return explicit_map


def validate_frames(frame_paths: list[Path]) -> None:
    expected_size = None
    for frame_path in frame_paths:
        if frame_path.stat().st_size < 100:
            raise RuntimeError(f"frame too small: {frame_path}")

        with Image.open(frame_path) as image:
            if expected_size is None:
                expected_size = image.size
            elif image.size != expected_size:
                raise RuntimeError(f"inconsistent frame size detected: {frame_path.name}")


def map_scenes(
    scene_names: list[str],
    frame_map: dict[str, Path],
    perf_results: dict[str, dict],
    keyword_filter: str,
) -> tuple[list[SceneEntry], list[str], list[str]]:
    available_tests = set(scene_names)

    if keyword_filter.strip():
        keywords = [item.strip().upper() for item in keyword_filter.split(",") if item.strip()]
        available_tests = {
            name
            for name in available_tests
            if any(keyword in name.upper() for keyword in keywords)
        }

    ordered_names, category_map = flatten_categories(available_tests)

    missing = [name for name in ordered_names if name not in frame_map]
    if missing:
        raise RuntimeError("missing frames for scenes: " + ", ".join(missing))

    entries = [
        SceneEntry(
            name=name,
            category=category_map[name],
            time_ms=perf_results.get(name, {}).get("time_ms") if perf_results else None,
            frame_path=frame_map[name],
        )
        for name in ordered_names
    ]

    extra_names = [name for name in scene_names if name not in available_tests]
    missing_timing_names = [name for name in ordered_names if name not in perf_results]
    return entries, extra_names, missing_timing_names


def load_font(size: int, bold: bool = False) -> ImageFont.ImageFont:
    candidates = []
    if platform.system() == "Windows":
        if bold:
            candidates.extend(
                [
                    Path("C:/Windows/Fonts/segoeuib.ttf"),
                    Path("C:/Windows/Fonts/consolab.ttf"),
                    Path("C:/Windows/Fonts/arialbd.ttf"),
                ]
            )
        else:
            candidates.extend(
                [
                    Path("C:/Windows/Fonts/segoeui.ttf"),
                    Path("C:/Windows/Fonts/consola.ttf"),
                    Path("C:/Windows/Fonts/arial.ttf"),
                ]
            )

    for candidate in candidates:
        if candidate.exists():
            return ImageFont.truetype(str(candidate), size=size)

    return ImageFont.load_default()


def wrap_case_name(name: str, width: int) -> list[str]:
    words = name.split("_")
    lines: list[str] = []
    current = ""
    max_chars = max(10, width // 9)

    for word in words:
        token = word if not current else "_" + word
        if len(current) + len(token) <= max_chars:
            current += token
            continue
        if current:
            lines.append(current)
        current = word

    if current:
        lines.append(current)

    wrapped: list[str] = []
    for line in lines:
        wrapped.extend(textwrap.wrap(line, width=max_chars, break_long_words=True, break_on_hyphens=False) or [""])
    return wrapped[:3]


def measure_multiline_height(draw: ImageDraw.ImageDraw, lines: list[str], font: ImageFont.ImageFont, spacing: int) -> int:
    if not lines:
        return 0
    bbox = draw.multiline_textbbox((0, 0), "\n".join(lines), font=font, spacing=spacing)
    return bbox[3] - bbox[1]


def build_contact_sheet(
    entries: list[SceneEntry],
    output_path: Path,
    git_commit: str,
    perf_results_commit: str,
    profile_name: str,
    columns: int,
    tile_width: int,
) -> None:
    if not entries:
        raise RuntimeError("no scene entries to compose")
    if columns <= 0:
        raise RuntimeError("columns must be greater than zero")

    with Image.open(entries[0].frame_path) as sample:
        tile_height = max(1, int(round(tile_width * sample.height / sample.width)))

    title_font = load_font(30, bold=True)
    subtitle_font = load_font(16, bold=False)
    category_font = load_font(20, bold=True)
    label_font = load_font(14, bold=True)
    time_font = load_font(14, bold=False)

    outer_margin = 24
    column_gap = 16
    row_gap = 16
    label_spacing = 4
    card_padding = 8
    section_gap = 26
    category_height = 38
    category_content_gap = 12
    card_width = tile_width + card_padding * 2
    label_height = 58
    card_height = tile_height + label_height + card_padding * 2
    sheet_width = outer_margin * 2 + columns * card_width + (columns - 1) * column_gap

    grouped: list[tuple[str, list[SceneEntry]]] = []
    current_category = None
    current_items: list[SceneEntry] = []
    for entry in entries:
        if entry.category != current_category:
            if current_items:
                grouped.append((current_category, current_items))
            current_category = entry.category
            current_items = [entry]
        else:
            current_items.append(entry)
    if current_items:
        grouped.append((current_category, current_items))

    title_height = 90
    total_height = outer_margin + title_height
    for _, category_entries in grouped:
        rows = math.ceil(len(category_entries) / columns)
        total_height += category_height + category_content_gap + rows * card_height + max(0, rows - 1) * row_gap + section_gap
    total_height += outer_margin - section_gap

    sheet = Image.new("RGB", (sheet_width, total_height), BACKGROUND_COLOR)
    draw = ImageDraw.Draw(sheet)

    draw.text((outer_margin, outer_margin), "HelloPerformance Scene Contact Sheet", fill=TITLE_COLOR, font=title_font)
    if perf_results_commit == git_commit:
        subtitle = f"capture/perf commit {git_commit} | profile {profile_name} | generated {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}"
    elif perf_results_commit != "unknown":
        subtitle = (
            f"capture commit {git_commit} | stale perf commit {perf_results_commit} "
            f"(timings hidden) | profile {profile_name}"
        )
    else:
        subtitle = f"capture commit {git_commit} | scene-only sheet | profile {profile_name}"
    draw.text((outer_margin, outer_margin + 40), subtitle, fill=SUBTITLE_COLOR, font=subtitle_font)

    y = outer_margin + title_height
    for category_name, category_entries in grouped:
        draw.rounded_rectangle((outer_margin, y, sheet_width - outer_margin, y + category_height), radius=10, fill=HEADER_COLOR)
        draw.text((outer_margin + 14, y + 8), f"{category_name} ({len(category_entries)})", fill=HEADER_TEXT_COLOR, font=category_font)
        y += category_height + category_content_gap

        for index, entry in enumerate(category_entries):
            row = index // columns
            col = index % columns
            card_x = outer_margin + col * (card_width + column_gap)
            card_y = y + row * (card_height + row_gap)

            draw.rounded_rectangle(
                (card_x, card_y, card_x + card_width, card_y + card_height),
                radius=12,
                fill=CARD_COLOR,
                outline=CARD_BORDER_COLOR,
                width=1,
            )

            with Image.open(entry.frame_path) as frame:
                preview = ImageOps.contain(frame.convert("RGB"), (tile_width, tile_height), IMAGE_LANCZOS)
            preview_x = card_x + card_padding + (tile_width - preview.width) // 2
            preview_y = card_y + card_padding + (tile_height - preview.height) // 2
            sheet.paste(preview, (preview_x, preview_y))

            label_x = card_x + card_padding + 6
            label_y = card_y + card_padding + tile_height + 6
            name_lines = wrap_case_name(entry.name, tile_width - 12)
            draw.multiline_text((label_x, label_y), "\n".join(name_lines), fill=LABEL_COLOR, font=label_font, spacing=label_spacing)

            name_height = measure_multiline_height(draw, name_lines, label_font, label_spacing)
            time_text = f"{entry.time_ms:.3f} ms" if entry.time_ms is not None else "scene only"
            draw.text((label_x, label_y + name_height + 6), time_text, fill=TIME_COLOR, font=time_font)

        rows = math.ceil(len(category_entries) / columns)
        y += rows * card_height + max(0, rows - 1) * row_gap + section_gap

    output_path.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(output_path)


def format_output_path(path: Path) -> str:
    try:
        return str(path.relative_to(PROJECT_ROOT)).replace("\\", "/")
    except ValueError:
        return str(path)


def write_index(
    index_path: Path,
    entries: list[SceneEntry],
    git_commit: str,
    perf_results_commit: str,
    profile_name: str,
    frames_dir: Path,
    sheet_path: Path,
) -> None:
    payload = {
        "timestamp": datetime.now().isoformat(),
        "git_commit": git_commit,
        "perf_results_commit": perf_results_commit,
        "profile": profile_name,
        "frames_dir": format_output_path(frames_dir),
        "sheet_path": format_output_path(sheet_path),
        "scene_count": len(entries),
        "timed_scene_count": sum(1 for entry in entries if entry.time_ms is not None),
        "scenes": [
            {
                "name": entry.name,
                "category": entry.category,
                "time_ms": entry.time_ms,
                "frame": format_output_path(entry.frame_path),
            }
            for entry in entries
        ],
    }
    index_path.parent.mkdir(parents=True, exist_ok=True)
    index_path.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def main() -> int:
    args = parse_args()
    results_path = Path(args.results)
    frames_dir = Path(args.frames_dir)
    output_path = Path(args.output)
    index_path = Path(args.index)

    perf_results, perf_results_commit, profile_name = read_perf_results(results_path, args.profile, args.filter)
    git_commit = get_git_commit()
    if perf_results and perf_results_commit != git_commit:
        print(
            "Warning: perf_results.json commit "
            f"{perf_results_commit} does not match current source commit {git_commit}; "
            "captured scenes will be kept, but timing labels will be omitted."
        )
        perf_results = {}

    build_capture_app()
    scene_names, frame_map = run_capture(frames_dir, args)
    entries, extra_names, missing_timing_names = map_scenes(scene_names, frame_map, perf_results, args.filter)
    if not entries:
        raise RuntimeError("no matching scenes after applying filters")

    build_contact_sheet(entries, output_path, git_commit, perf_results_commit, profile_name, args.columns, args.tile_width)
    write_index(index_path, entries, git_commit, perf_results_commit, profile_name, frames_dir, output_path)

    print(f"Scene sheet saved: {output_path}")
    print(f"Scene index saved: {index_path}")
    print(f"Captured scenes: {len(entries)}")
    if extra_names:
        print("Skipped scenes not present in the final sheet: " + ", ".join(extra_names))
    if missing_timing_names:
        print("Scenes without timing labels: " + ", ".join(missing_timing_names))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
