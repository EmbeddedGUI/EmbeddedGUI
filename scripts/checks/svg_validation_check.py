#!/usr/bin/env python3
"""Validate EmbeddedGUI SVG rendering against a large-viewport reference renderer."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
import xml.etree.ElementTree as ET
from dataclasses import asdict, dataclass
from pathlib import Path

import numpy as np
from PIL import Image, ImageChops, ImageDraw, ImageFont

SCRIPT_DIR = Path(__file__).resolve().parent
SCRIPTS_ROOT = SCRIPT_DIR.parent
ROOT_DIR = SCRIPTS_ROOT.parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))
if str(SCRIPTS_ROOT) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_ROOT))

import resvg_tool

import code_runtime_check as runtime_check


APP_NAME = "HelloSVGSpec"
OUTPUT_SUBDIR = "spec_parity"
FIXTURE_DIR = ROOT_DIR / "example" / APP_NAME / "fixtures"
MANIFEST_PATH = FIXTURE_DIR / "manifest.json"
CODEGEN_PATH = SCRIPT_DIR / "svg_case_codegen.py"
CODEGEN_OUTPUT_PATH = ROOT_DIR / "example" / APP_NAME / "svg_spec_cases_gen.h"
REFERENCE_CANVAS_WIDTH = 160
REFERENCE_CANVAS_HEIGHT = 160
DEFAULT_TIMEOUT = 35
DEFAULT_SUITES = ("image_svg", "canvas_active")
DEFAULT_FUZZY_MAX_DIFFERENCE = 0
DEFAULT_FUZZY_MAX_DIFF_PIXELS = 0
DEFAULT_MAX_MEAN_ABS = 2.5
DEFAULT_MAX_RMS = 10.0
DEFAULT_MAX_HOT_PIXEL_RATIO = 0.050
DEFAULT_REVIEW_MAX_BBOX_EDGE_DELTA = 4
DEFAULT_REVIEW_MAX_BBOX_AREA_DELTA_RATIO = 0.25
DEFAULT_HOT_PIXEL_DELTA = 16
EDGE_REFERENCE_TIMEOUT_SEC = 20
SVG_SPEC_RECORDING_WAIT_MS = 180
SVG_SPEC_REVIEW_RECORDING_WAIT_MS = 240
SVG_SPEC_RECORDING_OVERHEAD_MS = 8000
SVG_SPEC_RECORDING_CASE_BUDGET_MS = 400
DEFAULT_BROWSER_REFERENCE_COVERAGE_PHRASE = "strict parity stays covered by the default browser reference engine"
REVIEW_MODE_OFF = "off"
REVIEW_MODE_WARN = "warn"
REVIEW_MODE_FAIL = "fail"
PRIMARY_PARITY_MODE_LARGE_VIEWPORT = "large_viewport"
RENDER_BOX_SUMMARY_LIMIT = 10
OVERVIEW_MAX_COLUMNS = 4
OVERVIEW_CARD_WIDTH = 288
OVERVIEW_CARD_SPACING = 12
OVERVIEW_CARD_PADDING = 10
OVERVIEW_HEADER_HEIGHT = 92
OVERVIEW_THUMB_TOP = 74
OVERVIEW_CARD_BOTTOM_PADDING = 8
OVERVIEW_THUMB_WIDTH = 84
OVERVIEW_THUMB_HEIGHT = 84
OVERVIEW_CARD_HEIGHT = OVERVIEW_THUMB_TOP + OVERVIEW_THUMB_HEIGHT + OVERVIEW_CARD_BOTTOM_PADDING
OVERVIEW_REVIEW_COLUMNS = 3
OVERVIEW_REVIEW_ROWS = 4
OVERVIEW_REVIEW_CARD_WIDTH = 372
OVERVIEW_REVIEW_THUMB_WIDTH = 112
OVERVIEW_REVIEW_THUMB_HEIGHT = 112
OVERVIEW_REVIEW_CARD_HEIGHT = OVERVIEW_THUMB_TOP + OVERVIEW_REVIEW_THUMB_HEIGHT + OVERVIEW_CARD_BOTTOM_PADDING
OVERVIEW_MIN_CANONICAL_PREVIEW_AREA = 256
REVIEW_CANVAS_WIDTH = 320
REVIEW_CANVAS_HEIGHT = 320
REVIEW_PFB_WIDTH = 80
REVIEW_PFB_HEIGHT = 80
REVIEW_MIN_EDGE = 80
REVIEW_MARGIN = 0
REFERENCE_ENGINE_AUTO = "auto"
REFERENCE_ENGINE_RESVG = "resvg"
REFERENCE_ENGINE_EDGE = "edge"
REFERENCE_ENGINE_OPTIONS = (
    REFERENCE_ENGINE_AUTO,
    REFERENCE_ENGINE_RESVG,
    REFERENCE_ENGINE_EDGE,
)
REFERENCE_ENGINE_OVERRIDE_ALLOWED_ENGINES = (
    REFERENCE_ENGINE_RESVG,
    REFERENCE_ENGINE_EDGE,
)


def describe_resvg_unavailable() -> str:
    return resvg_tool.describe_resvg_missing(ROOT_DIR)


REFERENCE_ENGINE_OVERRIDE_ALLOWED_KEYS = frozenset(
    (
        "fuzzy_max_difference",
        "fuzzy_max_diff_pixels",
        "fuzzy_override_reason",
        "review_max_mean_abs",
        "review_max_rms",
        "review_max_hot_pixel_ratio",
        "review_max_bbox_edge_delta",
        "review_max_bbox_area_delta_ratio",
        "review_skip",
        "review_skip_reason",
    )
)
REVIEW_THRESHOLD_FIELDS = (
    ("review_max_mean_abs", "max_mean_abs", DEFAULT_MAX_MEAN_ABS),
    ("review_max_rms", "max_rms", DEFAULT_MAX_RMS),
    ("review_max_hot_pixel_ratio", "max_hot_pixel_ratio", DEFAULT_MAX_HOT_PIXEL_RATIO),
    ("review_max_bbox_edge_delta", "max_bbox_edge_delta", DEFAULT_REVIEW_MAX_BBOX_EDGE_DELTA),
    ("review_max_bbox_area_delta_ratio", "max_bbox_area_delta_ratio", DEFAULT_REVIEW_MAX_BBOX_AREA_DELTA_RATIO),
)
EDGE_CANDIDATES = (
    Path(r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"),
    Path(r"C:\Program Files\Microsoft\Edge\Application\msedge.exe"),
    Path(r"C:\Program Files\Google\Chrome\Application\chrome.exe"),
    Path(r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe"),
)
PLUTOSVG_SUPPORTED_ELEMENTS = frozenset(
    (
        "circle",
        "clipPath",
        "defs",
        "ellipse",
        "g",
        "image",
        "line",
        "linearGradient",
        "path",
        "polygon",
        "polyline",
        "radialGradient",
        "rect",
        "stop",
        "svg",
        "symbol",
        "use",
    )
)
PLUTOSVG_SUPPORTED_ATTRIBUTES = frozenset(
    (
        "clip-path",
        "clip-rule",
        "clipPathUnits",
        "color",
        "cx",
        "cy",
        "d",
        "display",
        "fill",
        "fill-opacity",
        "fill-rule",
        "fx",
        "fy",
        "gradientTransform",
        "gradientUnits",
        "height",
        "href",
        "id",
        "offset",
        "opacity",
        "points",
        "preserveAspectRatio",
        "r",
        "rx",
        "ry",
        "spreadMethod",
        "stop-color",
        "stop-opacity",
        "stroke",
        "stroke-dasharray",
        "stroke-dashoffset",
        "stroke-linecap",
        "stroke-linejoin",
        "stroke-miterlimit",
        "stroke-opacity",
        "stroke-width",
        "style",
        "transform",
        "viewBox",
        "visibility",
        "width",
        "x",
        "x1",
        "x2",
        "xlink:href",
        "y",
        "y1",
        "y2",
    )
)
PLUTOSVG_UNSUPPORTED_FEATURE_EXACT_REASONS = {
    "group-opacity": "current PlutoSVG integration does not isolate multi-child group opacity compositing",
}
PLUTOSVG_UNSUPPORTED_FEATURE_KEYWORD_REASONS = (
    ("overflow", "current PlutoSVG integration does not implement nested viewport overflow semantics"),
)
SVG_RAW_ATTRIBUTE_PATTERN = re.compile(r"([A-Za-z_:][\w:.-]*)\s*=")
XLINK_HREF_NAMESPACE = "{http://www.w3.org/1999/xlink}href"
EDGE_LARGE_VIEWPORT_PROBE_CACHE: dict[str, bool] = {}
try:
    NEAREST_RESAMPLING = Image.Resampling.NEAREST
    HIGH_QUALITY_RESAMPLING = Image.Resampling.LANCZOS
except AttributeError:
    NEAREST_RESAMPLING = Image.NEAREST
    HIGH_QUALITY_RESAMPLING = Image.ANTIALIAS


@dataclass(frozen=True)
class FrameMetrics:
    max_difference: int
    diff_pixels: int
    diff_pixel_ratio: float
    mean_abs: float
    rms: float
    hot_pixel_ratio: float


@dataclass(frozen=True)
class ReviewGeometryMetrics:
    available: bool
    reason: str
    reference_background: tuple[int, int, int] | None
    actual_background: tuple[int, int, int] | None
    reference_bbox: tuple[int, int, int, int] | None
    actual_bbox: tuple[int, int, int, int] | None
    bbox_edge_delta: int | None
    bbox_area_delta_ratio: float | None


@dataclass(frozen=True)
class ManifestData:
    cases: list[dict[str, object]]
    required_features: tuple[str, ...]


@dataclass
class RuntimeCodegenContext:
    cases: list[dict[str, object]]
    manifest_path: Path | None
    fixture_dir: Path | None
    trial_render_box_request: dict[str, object] | None
    temp_dir: tempfile.TemporaryDirectory | None


@dataclass(frozen=True)
class ImageCropBox:
    x: int
    y: int
    width: int
    height: int


@dataclass(frozen=True)
class OverviewDiffPreview:
    image: Image.Image
    diff_pixel_count: int
    hot_pixel_count: int
    max_difference: int


def resolve_make_jobs(requested_jobs: int) -> int | None:
    if requested_jobs > 0:
        return requested_jobs
    cpu_count = os.cpu_count() or 1
    return min(2, max(1, cpu_count))


def run_command(command: list[str], cwd: Path = ROOT_DIR) -> tuple[bool, str]:
    result = subprocess.run(command, cwd=cwd, capture_output=True, text=True)
    output = ((result.stdout or "") + "\n" + (result.stderr or "")).strip()
    return result.returncode == 0, output


def run_make_clean() -> tuple[bool, str]:
    return run_command(["make", "clean"])


def build_app(app: str, port: str, jobs: int) -> tuple[bool, str]:
    command = ["make", f"-j{resolve_make_jobs(jobs)}", "all", f"APP={app}", f"PORT={port}"]
    return run_command(command)


def parse_executed_suites(output: str) -> list[str]:
    suites = []
    prefix = "===== Suite: "
    suffix = " ====="

    for line in output.splitlines():
        if not line.startswith(prefix):
            continue
        suite_name = line[len(prefix):]
        if suite_name.endswith(suffix):
            suite_name = suite_name[:-len(suffix)]
        suite_name = suite_name.strip()
        if suite_name:
            suites.append(suite_name)

    return suites


def run_svg_unit_suites(suites: tuple[str, ...], jobs: int) -> tuple[bool, str]:
    ok, output = run_make_clean()
    if not ok:
        return False, "make clean failed for HelloUnitTest\n%s" % output

    ok, output = build_app("HelloUnitTest", "pc_test", jobs)
    if not ok:
        return False, "build failed for HelloUnitTest\n%s" % output

    suite_filter = ",".join(suites)
    ok, output = run_command([str(ROOT_DIR / "output" / "main.exe"), "--suite", suite_filter])
    if not ok:
        return False, "svg unit suites failed\n%s" % output

    executed_suites = parse_executed_suites(output)
    expected_suite_set = set(suites)
    executed_suite_set = set(executed_suites)
    if executed_suite_set != expected_suite_set:
        return False, "suite filter mismatch: expected %s got %s\n%s" % (
            ", ".join(suites),
            ", ".join(executed_suites) if executed_suites else "<none>",
            output,
        )

    return True, "supplemental internal suites passed: %s" % ", ".join(executed_suites)


def run_codegen(manifest_path: Path | None = None, fixture_dir: Path | None = None) -> None:
    command = [sys.executable, str(CODEGEN_PATH), "--output", str(CODEGEN_OUTPUT_PATH)]
    if manifest_path is not None:
        command.extend(["--manifest", str(manifest_path)])
    if fixture_dir is not None:
        command.extend(["--fixture-dir", str(fixture_dir)])
    result = subprocess.run(command, cwd=ROOT_DIR, capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(((result.stdout or "") + "\n" + (result.stderr or "")).strip() or "svg fixture codegen failed")


def normalize_reference_engine_overrides(entry: dict[str, object], case_id: str) -> dict[str, dict[str, object]]:
    raw_overrides = entry.get("reference_engine_overrides", {})
    if raw_overrides is None:
        raw_overrides = {}
    if not isinstance(raw_overrides, dict):
        raise RuntimeError("svg fixture manifest reference_engine_overrides must be an object: %s" % case_id)

    normalized: dict[str, dict[str, object]] = {}
    for raw_engine, raw_override in raw_overrides.items():
        engine = str(raw_engine).strip().lower()
        if engine not in REFERENCE_ENGINE_OVERRIDE_ALLOWED_ENGINES:
            raise RuntimeError("unsupported reference_engine_overrides key %s: %s" % (raw_engine, case_id))
        if not isinstance(raw_override, dict):
            raise RuntimeError("reference_engine_overrides.%s must be an object: %s" % (engine, case_id))
        unknown_keys = sorted(set(raw_override.keys()) - REFERENCE_ENGINE_OVERRIDE_ALLOWED_KEYS)
        if unknown_keys:
            raise RuntimeError(
                "unsupported reference_engine_overrides fields (%s): %s"
                % (", ".join(str(key) for key in unknown_keys), case_id)
            )

        override: dict[str, object] = {}
        for key in ("fuzzy_max_difference", "fuzzy_max_diff_pixels", "review_max_bbox_edge_delta"):
            if key not in raw_override:
                continue
            try:
                override[key] = int(raw_override[key])
            except (TypeError, ValueError) as exc:
                raise RuntimeError("reference_engine_overrides.%s.%s must be an integer: %s" % (engine, key, case_id)) from exc
        for key in (
            "review_max_mean_abs",
            "review_max_rms",
            "review_max_hot_pixel_ratio",
            "review_max_bbox_area_delta_ratio",
        ):
            if key not in raw_override:
                continue
            try:
                override[key] = float(raw_override[key])
            except (TypeError, ValueError) as exc:
                raise RuntimeError("reference_engine_overrides.%s.%s must be numeric: %s" % (engine, key, case_id)) from exc
        for key in ("fuzzy_override_reason", "review_skip_reason"):
            if key in raw_override:
                override[key] = str(raw_override[key]).strip()
        if "review_skip" in raw_override:
            review_skip = raw_override["review_skip"]
            if not isinstance(review_skip, bool):
                raise RuntimeError("reference_engine_overrides.%s.review_skip must be a boolean: %s" % (engine, case_id))
            override["review_skip"] = review_skip
        normalized[engine] = override
    return normalized


def build_review_threshold_overrides(
    case: dict[str, object],
    explicit_manifest_review_threshold_keys: set[str],
    reference_engine_override_fields: set[str],
) -> dict[str, dict[str, object]]:
    review_threshold_overrides: dict[str, dict[str, object]] = {}

    def register_review_threshold_override(
        threshold_key: str,
        active_value: float | int,
        default_value: float | int,
        origin: str,
    ) -> None:
        differs = active_value != default_value
        if isinstance(active_value, float) or isinstance(default_value, float):
            differs = abs(float(active_value) - float(default_value)) > 1e-9
        if not differs:
            return
        review_threshold_overrides[threshold_key] = {
            "active": active_value,
            "default": default_value,
            "origin": origin,
        }

    for manifest_key, report_key, default_value in REVIEW_THRESHOLD_FIELDS:
        active_value = case[manifest_key]
        if manifest_key in reference_engine_override_fields:
            origin = "reference_engine_override"
        elif manifest_key in explicit_manifest_review_threshold_keys:
            origin = "explicit_review_threshold"
        else:
            origin = "default"
        register_review_threshold_override(report_key, active_value, default_value, origin)
    return review_threshold_overrides


def refresh_case_threshold_metadata(case: dict[str, object], case_id: str) -> None:
    explicit_manifest_review_threshold_keys = {
        str(item)
        for item in case.get("manifest_explicit_review_threshold_keys", ())
        if str(item)
    }
    reference_engine_override_fields = {
        str(item)
        for item in case.get("reference_engine_override_fields", ())
        if str(item)
    }

    try:
        fuzzy_max_difference = int(case.get("fuzzy_max_difference", DEFAULT_FUZZY_MAX_DIFFERENCE))
        fuzzy_max_diff_pixels = int(case.get("fuzzy_max_diff_pixels", DEFAULT_FUZZY_MAX_DIFF_PIXELS))
        review_max_mean_abs = float(case.get("review_max_mean_abs", DEFAULT_MAX_MEAN_ABS))
        review_max_rms = float(case.get("review_max_rms", DEFAULT_MAX_RMS))
        review_max_hot_pixel_ratio = float(case.get("review_max_hot_pixel_ratio", DEFAULT_MAX_HOT_PIXEL_RATIO))
        review_max_bbox_edge_delta = int(case.get("review_max_bbox_edge_delta", DEFAULT_REVIEW_MAX_BBOX_EDGE_DELTA))
        review_max_bbox_area_delta_ratio = float(
            case.get("review_max_bbox_area_delta_ratio", DEFAULT_REVIEW_MAX_BBOX_AREA_DELTA_RATIO)
        )
    except (TypeError, ValueError) as exc:
        raise RuntimeError("svg fixture manifest thresholds must be numeric: %s" % case_id) from exc

    fuzzy_override_reason = str(case.get("fuzzy_override_reason", "")).strip()
    uses_default_fuzzy = (
        fuzzy_max_difference == DEFAULT_FUZZY_MAX_DIFFERENCE
        and fuzzy_max_diff_pixels == DEFAULT_FUZZY_MAX_DIFF_PIXELS
    )
    if fuzzy_max_difference < 0:
        raise RuntimeError("svg fixture manifest fuzzy_max_difference must be non-negative: %s" % case_id)
    if fuzzy_max_diff_pixels < 0:
        raise RuntimeError("svg fixture manifest fuzzy_max_diff_pixels must be non-negative: %s" % case_id)
    if uses_default_fuzzy:
        if fuzzy_override_reason:
            raise RuntimeError("fuzzy_override_reason is only valid for fuzzy override cases: %s" % case_id)
    elif not fuzzy_override_reason:
        raise RuntimeError("fuzzy override case must declare fuzzy_override_reason: %s" % case_id)

    if review_max_mean_abs < 0.0:
        raise RuntimeError("svg fixture manifest review_max_mean_abs must be non-negative: %s" % case_id)
    if review_max_rms < 0.0:
        raise RuntimeError("svg fixture manifest review_max_rms must be non-negative: %s" % case_id)
    if review_max_hot_pixel_ratio < 0.0 or review_max_hot_pixel_ratio > 1.0:
        raise RuntimeError("svg fixture manifest review_max_hot_pixel_ratio must be within [0, 1]: %s" % case_id)
    if review_max_bbox_edge_delta < 0:
        raise RuntimeError("svg fixture manifest review_max_bbox_edge_delta must be non-negative: %s" % case_id)
    if review_max_bbox_area_delta_ratio < 0.0 or review_max_bbox_area_delta_ratio > 1.0:
        raise RuntimeError("svg fixture manifest review_max_bbox_area_delta_ratio must be within [0, 1]: %s" % case_id)

    review_skip = case.get("review_skip", False)
    if not isinstance(review_skip, bool):
        raise RuntimeError("svg fixture manifest review_skip must be a boolean: %s" % case_id)
    review_skip_reason = str(case.get("review_skip_reason", "")).strip()
    if not review_skip and review_skip_reason:
        raise RuntimeError("review_skip_reason is only valid for review_skip cases: %s" % case_id)
    if review_skip and not review_skip_reason:
        raise RuntimeError("review_skip case must declare a non-empty review_skip_reason: %s" % case_id)
    effective_explicit_review_threshold_keys = set(explicit_manifest_review_threshold_keys)
    effective_explicit_review_threshold_keys.update(
        manifest_key for manifest_key, _, _ in REVIEW_THRESHOLD_FIELDS if manifest_key in reference_engine_override_fields
    )
    if review_skip and effective_explicit_review_threshold_keys:
        raise RuntimeError(
            "review_skip case cannot declare explicit review thresholds (%s): %s"
            % (", ".join(sorted(effective_explicit_review_threshold_keys)), case_id)
        )

    review_threshold_overrides = build_review_threshold_overrides(
        case,
        explicit_manifest_review_threshold_keys,
        reference_engine_override_fields,
    )

    case["uses_default_fuzzy"] = uses_default_fuzzy
    case["fuzzy_override_reason"] = fuzzy_override_reason
    case["fuzzy_max_difference"] = fuzzy_max_difference
    case["fuzzy_max_diff_pixels"] = fuzzy_max_diff_pixels
    case["review_max_mean_abs"] = review_max_mean_abs
    case["review_max_rms"] = review_max_rms
    case["review_max_hot_pixel_ratio"] = review_max_hot_pixel_ratio
    case["review_max_bbox_edge_delta"] = review_max_bbox_edge_delta
    case["review_max_bbox_area_delta_ratio"] = review_max_bbox_area_delta_ratio
    case["review_has_nondefault_thresholds"] = bool(review_threshold_overrides)
    case["review_threshold_override_keys"] = sorted(review_threshold_overrides.keys())
    case["review_threshold_overrides"] = review_threshold_overrides
    case["review_skip"] = review_skip
    case["review_skip_reason"] = review_skip_reason


def apply_reference_engine_case_overrides(cases: list[dict[str, object]], reference_engine: str) -> None:
    for case in cases:
        raw_overrides = case.get("reference_engine_overrides", {})
        if not isinstance(raw_overrides, dict):
            case["reference_engine_override_fields"] = []
            continue
        override = raw_overrides.get(reference_engine, {})
        if not isinstance(override, dict) or not override:
            case["reference_engine_override_fields"] = []
            continue
        for field, value in override.items():
            case[field] = value
        case["reference_engine_override_fields"] = sorted(str(field) for field in override.keys())
        refresh_case_threshold_metadata(case, str(case["id"]))


def format_fixed_viewport_parity_message(case_id: str, parity_result: dict[str, object]) -> str:
    metrics = dict(parity_result.get("metrics", {}))
    geometry = dict(parity_result.get("geometry", {}))
    reasons = [str(reason).strip() for reason in list(parity_result.get("reasons", [])) if str(reason).strip()]
    parts = [
        "%s large_viewport" % case_id,
        "max_diff=%d" % int(metrics.get("max_difference", 0)),
        "diff_pixels=%d" % int(metrics.get("diff_pixels", 0)),
        "diff_ratio=%.5f" % float(metrics.get("diff_pixel_ratio", 0.0)),
        "mean_abs=%.3f" % float(metrics.get("mean_abs", 0.0)),
        "rms=%.3f" % float(metrics.get("rms", 0.0)),
        "hot_ratio=%.5f" % float(metrics.get("hot_pixel_ratio", 0.0)),
    ]

    if bool(geometry.get("available", False)):
        bbox_edge_delta = geometry.get("bbox_edge_delta")
        bbox_area_delta_ratio = geometry.get("bbox_area_delta_ratio")
        if bbox_edge_delta is not None:
            parts.append("bbox_edge=%d" % int(bbox_edge_delta))
        if bbox_area_delta_ratio is not None:
            parts.append("bbox_area_ratio=%.5f" % float(bbox_area_delta_ratio))
    if str(parity_result.get("compare", "")).strip():
        parts.append("compare=%s" % str(parity_result["compare"]))
    if str(parity_result.get("diff", "")).strip():
        parts.append("diff=%s" % str(parity_result["diff"]))
    if reasons:
        parts.append("reasons=%s" % ",".join(reasons))
    return " ".join(parts)


def load_case_manifest() -> ManifestData:
    payload = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    cases = payload.get("cases", [])
    raw_required_features = payload.get("required_features", [])
    if not isinstance(cases, list) or not cases:
        raise RuntimeError("svg fixture manifest is empty")
    if raw_required_features is None:
        raw_required_features = []
    if not isinstance(raw_required_features, list):
        raise RuntimeError("svg fixture manifest required_features must be a list")

    seen_ids: set[str] = set()
    seen_required_features: set[str] = set()
    required_features: list[str] = []
    normalized_cases: list[dict[str, object]] = []
    for raw_feature in raw_required_features:
        feature = str(raw_feature).strip()
        if not feature:
            raise RuntimeError("svg fixture manifest contains an empty required feature tag")
        if feature in seen_required_features:
            raise RuntimeError("duplicate required feature tag: %s" % feature)
        seen_required_features.add(feature)
        required_features.append(feature)

    for entry in cases:
        if not isinstance(entry, dict):
            raise RuntimeError("svg fixture manifest contains a non-object case entry")
        case_id = str(entry.get("id", "")).strip()
        file_name = str(entry.get("file", "")).strip()
        title = str(entry.get("title", case_id)).strip() or case_id
        raw_features = entry.get("features", [])
        raw_render_size_sensitive = entry.get("render_size_sensitive", False)
        raw_render_size_reason = entry.get("render_size_reason", "")
        raw_shrink_locked = entry.get("shrink_locked", False)
        raw_shrink_lock_reason = entry.get("shrink_lock_reason", "")
        raw_fuzzy_max_difference = entry.get("fuzzy_max_difference", DEFAULT_FUZZY_MAX_DIFFERENCE)
        raw_fuzzy_max_diff_pixels = entry.get("fuzzy_max_diff_pixels", DEFAULT_FUZZY_MAX_DIFF_PIXELS)
        raw_fuzzy_override_reason = entry.get("fuzzy_override_reason", "")
        raw_review_max_mean_abs = entry.get("review_max_mean_abs", DEFAULT_MAX_MEAN_ABS)
        raw_review_max_rms = entry.get("review_max_rms", DEFAULT_MAX_RMS)
        raw_review_max_hot_pixel_ratio = entry.get("review_max_hot_pixel_ratio", DEFAULT_MAX_HOT_PIXEL_RATIO)
        raw_review_max_bbox_edge_delta = entry.get("review_max_bbox_edge_delta", DEFAULT_REVIEW_MAX_BBOX_EDGE_DELTA)
        raw_review_max_bbox_area_delta_ratio = entry.get("review_max_bbox_area_delta_ratio", DEFAULT_REVIEW_MAX_BBOX_AREA_DELTA_RATIO)
        review_skip = bool(entry.get("review_skip", False))
        review_skip_reason = str(entry.get("review_skip_reason", "")).strip()
        explicit_review_threshold_keys = [
            threshold_key
            for threshold_key in (
                "review_max_mean_abs",
                "review_max_rms",
                "review_max_hot_pixel_ratio",
                "review_max_bbox_edge_delta",
                "review_max_bbox_area_delta_ratio",
            )
            if threshold_key in entry
        ]
        render_width = int(entry.get("render_width", REFERENCE_CANVAS_WIDTH))
        render_height = int(entry.get("render_height", REFERENCE_CANVAS_HEIGHT))
        composite_x = int(entry.get("composite_x", 0))
        composite_y = int(entry.get("composite_y", 0))
        if not case_id or not file_name:
            raise RuntimeError("svg fixture manifest contains an incomplete case entry")
        reference_engine_overrides = normalize_reference_engine_overrides(entry, case_id)
        if case_id in seen_ids:
            raise RuntimeError("duplicate svg fixture id: %s" % case_id)
        if raw_features is None:
            raw_features = []
        if not isinstance(raw_features, list):
            raise RuntimeError("svg fixture manifest features must be a list: %s" % case_id)
        features: list[str] = []
        for raw_feature in raw_features:
            feature = str(raw_feature).strip()
            if not feature:
                raise RuntimeError("svg fixture manifest contains an empty feature tag: %s" % case_id)
            features.append(feature)
        if not isinstance(raw_render_size_sensitive, bool):
            raise RuntimeError("svg fixture manifest render_size_sensitive must be a boolean: %s" % case_id)
        render_size_sensitive = raw_render_size_sensitive
        render_size_reason = str(raw_render_size_reason).strip()
        if not isinstance(raw_shrink_locked, bool):
            raise RuntimeError("svg fixture manifest shrink_locked must be a boolean: %s" % case_id)
        shrink_locked = raw_shrink_locked
        shrink_lock_reason = str(raw_shrink_lock_reason).strip()
        if render_width <= 0 or render_height <= 0:
            raise RuntimeError("svg fixture manifest render size must be positive: %s" % case_id)
        fuzzy_max_difference = int(raw_fuzzy_max_difference)
        fuzzy_max_diff_pixels = int(raw_fuzzy_max_diff_pixels)
        fuzzy_override_reason = str(raw_fuzzy_override_reason).strip()
        uses_default_fuzzy = (
            fuzzy_max_difference == DEFAULT_FUZZY_MAX_DIFFERENCE
            and fuzzy_max_diff_pixels == DEFAULT_FUZZY_MAX_DIFF_PIXELS
        )
        if fuzzy_max_difference < 0:
            raise RuntimeError("svg fixture manifest fuzzy_max_difference must be non-negative: %s" % case_id)
        if fuzzy_max_diff_pixels < 0:
            raise RuntimeError("svg fixture manifest fuzzy_max_diff_pixels must be non-negative: %s" % case_id)
        if uses_default_fuzzy:
            if fuzzy_override_reason:
                raise RuntimeError("fuzzy_override_reason is only valid for fuzzy override cases: %s" % case_id)
        elif not fuzzy_override_reason:
            raise RuntimeError("fuzzy override case must declare fuzzy_override_reason: %s" % case_id)
        if composite_x < 0 or composite_y < 0:
            raise RuntimeError("svg fixture manifest composite offset must be non-negative: %s" % case_id)
        if composite_x + render_width > REFERENCE_CANVAS_WIDTH or composite_y + render_height > REFERENCE_CANVAS_HEIGHT:
            raise RuntimeError("svg fixture render box exceeds reference canvas: %s" % case_id)
        uses_default_render_box = (
            render_width == REFERENCE_CANVAS_WIDTH
            and render_height == REFERENCE_CANVAS_HEIGHT
            and composite_x == 0
            and composite_y == 0
        )
        if uses_default_render_box and not render_size_sensitive:
            raise RuntimeError(
                "cases that keep the default render box must declare render_size_sensitive: %s" % case_id
            )
        if render_size_sensitive:
            if not render_size_reason:
                raise RuntimeError(
                    "render-size-sensitive case must declare a non-empty render_size_reason: %s" % case_id
                )
            if render_width != REFERENCE_CANVAS_WIDTH or render_height != REFERENCE_CANVAS_HEIGHT:
                raise RuntimeError(
                    "render-size-sensitive case must keep the default render box: %s" % case_id
                )
            if composite_x != 0 or composite_y != 0:
                raise RuntimeError(
                    "render-size-sensitive case must keep the default composite offset: %s" % case_id
                )
        elif render_size_reason:
            raise RuntimeError(
                "render_size_reason is only valid for render-size-sensitive cases: %s" % case_id
            )
        if shrink_locked:
            if render_size_sensitive:
                raise RuntimeError(
                    "shrink_locked is only valid for non-render-size-sensitive cases: %s" % case_id
                )
            if uses_default_render_box:
                raise RuntimeError(
                    "shrink_locked case must already use a non-default render box: %s" % case_id
                )
            if not shrink_lock_reason:
                raise RuntimeError(
                    "shrink_locked case must declare a non-empty shrink_lock_reason: %s" % case_id
                )
        elif shrink_lock_reason:
            raise RuntimeError(
                "shrink_lock_reason is only valid for shrink_locked cases: %s" % case_id
            )
        try:
            review_max_mean_abs = float(raw_review_max_mean_abs)
            review_max_rms = float(raw_review_max_rms)
            review_max_hot_pixel_ratio = float(raw_review_max_hot_pixel_ratio)
            review_max_bbox_edge_delta = int(raw_review_max_bbox_edge_delta)
            review_max_bbox_area_delta_ratio = float(raw_review_max_bbox_area_delta_ratio)
        except (TypeError, ValueError) as exc:
            raise RuntimeError("svg fixture manifest review thresholds must be numeric: %s" % case_id) from exc
        if review_max_mean_abs < 0.0:
            raise RuntimeError("svg fixture manifest review_max_mean_abs must be non-negative: %s" % case_id)
        if review_max_rms < 0.0:
            raise RuntimeError("svg fixture manifest review_max_rms must be non-negative: %s" % case_id)
        if review_max_hot_pixel_ratio < 0.0 or review_max_hot_pixel_ratio > 1.0:
            raise RuntimeError("svg fixture manifest review_max_hot_pixel_ratio must be within [0, 1]: %s" % case_id)
        if review_max_bbox_edge_delta < 0:
            raise RuntimeError("svg fixture manifest review_max_bbox_edge_delta must be non-negative: %s" % case_id)
        if review_max_bbox_area_delta_ratio < 0.0 or review_max_bbox_area_delta_ratio > 1.0:
            raise RuntimeError("svg fixture manifest review_max_bbox_area_delta_ratio must be within [0, 1]: %s" % case_id)
        if not review_skip and review_skip_reason:
            raise RuntimeError("review_skip_reason is only valid for review_skip cases: %s" % case_id)
        if review_skip and not review_skip_reason:
            raise RuntimeError("review_skip case must declare a non-empty review_skip_reason: %s" % case_id)
        if review_skip and explicit_review_threshold_keys:
            raise RuntimeError(
                "review_skip case cannot declare explicit review thresholds (%s): %s"
                % (", ".join(explicit_review_threshold_keys), case_id)
            )

        review_threshold_overrides: dict[str, dict[str, object]] = {}

        def register_review_threshold_override(
            threshold_key: str,
            active_value: float | int,
            default_value: float | int,
            origin: str,
        ) -> None:
            differs = active_value != default_value
            if isinstance(active_value, float) or isinstance(default_value, float):
                differs = abs(float(active_value) - float(default_value)) > 1e-9
            if not differs:
                return
            review_threshold_overrides[threshold_key] = {
                "active": active_value,
                "default": default_value,
                "origin": origin,
            }

        register_review_threshold_override(
            "max_mean_abs",
            review_max_mean_abs,
            DEFAULT_MAX_MEAN_ABS,
            "explicit_review_threshold" if "review_max_mean_abs" in entry else "default",
        )
        register_review_threshold_override(
            "max_rms",
            review_max_rms,
            DEFAULT_MAX_RMS,
            "explicit_review_threshold" if "review_max_rms" in entry else "default",
        )
        register_review_threshold_override(
            "max_hot_pixel_ratio",
            review_max_hot_pixel_ratio,
            DEFAULT_MAX_HOT_PIXEL_RATIO,
            "explicit_review_threshold" if "review_max_hot_pixel_ratio" in entry else "default",
        )
        register_review_threshold_override(
            "max_bbox_edge_delta",
            review_max_bbox_edge_delta,
            DEFAULT_REVIEW_MAX_BBOX_EDGE_DELTA,
            "explicit_review_threshold" if "review_max_bbox_edge_delta" in entry else "default",
        )
        register_review_threshold_override(
            "max_bbox_area_delta_ratio",
            review_max_bbox_area_delta_ratio,
            DEFAULT_REVIEW_MAX_BBOX_AREA_DELTA_RATIO,
            "explicit_review_threshold" if "review_max_bbox_area_delta_ratio" in entry else "default",
        )
        fixture_path = FIXTURE_DIR / file_name
        if not fixture_path.exists():
            raise RuntimeError("missing svg fixture: %s" % fixture_path)
        seen_ids.add(case_id)
        normalized_cases.append(
            {
                "id": case_id,
                "title": title,
                "file": file_name,
                "path": fixture_path,
                "manifest_entry": json.loads(json.dumps(entry)),
                "features": tuple(features),
                "render_size_sensitive": render_size_sensitive,
                "render_size_reason": render_size_reason,
                "shrink_locked": shrink_locked,
                "shrink_lock_reason": shrink_lock_reason,
                "uses_default_render_box": uses_default_render_box,
                "render_width": render_width,
                "render_height": render_height,
                "composite_x": composite_x,
                "composite_y": composite_y,
                "manifest_render_width": render_width,
                "manifest_render_height": render_height,
                "manifest_composite_x": composite_x,
                "manifest_composite_y": composite_y,
                "trial_render_box_active": False,
                "uses_default_fuzzy": uses_default_fuzzy,
                "fuzzy_override_reason": fuzzy_override_reason,
                "fuzzy_max_difference": fuzzy_max_difference,
                "fuzzy_max_diff_pixels": fuzzy_max_diff_pixels,
                "max_mean_abs": float(entry.get("max_mean_abs", DEFAULT_MAX_MEAN_ABS)),
                "max_rms": float(entry.get("max_rms", DEFAULT_MAX_RMS)),
                "max_hot_pixel_ratio": float(entry.get("max_hot_pixel_ratio", DEFAULT_MAX_HOT_PIXEL_RATIO)),
                "manifest_explicit_review_threshold_keys": tuple(explicit_review_threshold_keys),
                "reference_engine_overrides": reference_engine_overrides,
                "reference_engine_override_fields": [],
                "review_max_mean_abs": review_max_mean_abs,
                "review_max_rms": review_max_rms,
                "review_max_hot_pixel_ratio": review_max_hot_pixel_ratio,
                "review_max_bbox_edge_delta": review_max_bbox_edge_delta,
                "review_max_bbox_area_delta_ratio": review_max_bbox_area_delta_ratio,
                "review_has_nondefault_thresholds": bool(review_threshold_overrides),
                "review_threshold_override_keys": sorted(review_threshold_overrides.keys()),
                "review_threshold_overrides": review_threshold_overrides,
                "review_skip": review_skip,
                "review_skip_reason": review_skip_reason,
            }
        )

    return ManifestData(cases=normalized_cases, required_features=tuple(required_features))


def summarize_case_features(cases: list[dict[str, object]]) -> list[str]:
    feature_set: set[str] = set()
    for case in cases:
        for feature in case.get("features", ()):
            feature_set.add(str(feature))
    return sorted(feature_set)


def build_feature_case_map(cases: list[dict[str, object]]) -> dict[str, list[str]]:
    feature_case_map: dict[str, list[str]] = {}
    for case in cases:
        case_id = str(case["id"])
        for raw_feature in case.get("features", ()):
            feature = str(raw_feature)
            feature_case_map.setdefault(feature, []).append(case_id)
    return {feature: sorted(case_ids) for feature, case_ids in sorted(feature_case_map.items())}


def validate_feature_contract(cases: list[dict[str, object]], required_features: tuple[str, ...]) -> tuple[list[str], list[str]]:
    covered_features = set(summarize_case_features(cases))
    required_feature_set = set(required_features)
    missing = [feature for feature in required_features if feature not in covered_features]
    unexpected = sorted(feature for feature in covered_features if feature not in required_feature_set)
    return missing, unexpected


def xml_local_name(name: str) -> str:
    if name.startswith("{"):
        return name.split("}", 1)[1]
    return name


def append_unique_reason(reasons: list[str], reason: str) -> None:
    if reason and reason not in reasons:
        reasons.append(reason)


def analyze_case_plutosvg_support(case: dict[str, object]) -> list[str]:
    reasons: list[str] = []
    raw_svg = Path(case["path"]).read_text(encoding="utf-8")

    raw_unsupported_attributes = sorted(
        {
            match.group(1)
            for match in SVG_RAW_ATTRIBUTE_PATTERN.finditer(raw_svg)
            if match.group(1) != "xmlns"
            and not match.group(1).startswith("xmlns:")
            and match.group(1) not in PLUTOSVG_SUPPORTED_ATTRIBUTES
        }
    )
    if raw_unsupported_attributes:
        append_unique_reason(
            reasons,
            "uses raw attributes unsupported by vendored PlutoSVG parser: %s" % ", ".join(raw_unsupported_attributes),
        )

    for raw_feature in case.get("features", ()):
        feature = str(raw_feature)
        exact_reason = PLUTOSVG_UNSUPPORTED_FEATURE_EXACT_REASONS.get(feature)
        if exact_reason:
            append_unique_reason(reasons, "feature %s: %s" % (feature, exact_reason))
            continue
        for keyword, keyword_reason in PLUTOSVG_UNSUPPORTED_FEATURE_KEYWORD_REASONS:
            if keyword in feature:
                append_unique_reason(reasons, "feature %s: %s" % (feature, keyword_reason))
                break

    root = ET.fromstring(raw_svg)
    id_map: dict[str, ET.Element] = {}
    unsupported_elements: set[str] = set()
    for element in root.iter():
        element_name = xml_local_name(element.tag)
        if element_name not in PLUTOSVG_SUPPORTED_ELEMENTS:
            unsupported_elements.add(element_name)
        element_id = element.attrib.get("id")
        if element_id:
            id_map[element_id] = element

        if element_name not in ("g", "svg", "symbol"):
            continue
        if "opacity" not in element.attrib:
            continue
        paint_child_count = sum(1 for child in list(element) if xml_local_name(child.tag) != "defs")
        if paint_child_count > 1:
            append_unique_reason(
                reasons,
                "uses multi-child %s opacity compositing that current PlutoSVG integration does not isolate" % element_name,
            )
            break

    if unsupported_elements:
        append_unique_reason(
            reasons,
            "uses SVG elements unsupported by vendored PlutoSVG parser: %s" % ", ".join(sorted(unsupported_elements)),
        )

    for element in root.iter():
        if xml_local_name(element.tag) != "use":
            continue
        href = element.attrib.get("href")
        if href is None:
            href = element.attrib.get(XLINK_HREF_NAMESPACE)
        if not href or not href.startswith("#"):
            continue
        referenced = id_map.get(href[1:])
        if referenced is None:
            continue
        referenced_name = xml_local_name(referenced.tag)
        if referenced_name in ("svg", "symbol") and ("width" in element.attrib or "height" in element.attrib):
            append_unique_reason(
                reasons,
                "uses <use> width/height viewport sizing for referenced <%s>, which current PlutoSVG integration ignores" % referenced_name,
            )
            break

    return reasons


def filter_plutosvg_supported_cases(cases: list[dict[str, object]]) -> tuple[list[dict[str, object]], list[dict[str, object]]]:
    active_cases: list[dict[str, object]] = []
    excluded_cases: list[dict[str, object]] = []
    for case in cases:
        reasons = analyze_case_plutosvg_support(case)
        if not reasons:
            active_cases.append(case)
            continue
        excluded_case = dict(case)
        excluded_case["library_support_reasons"] = tuple(reasons)
        excluded_cases.append(excluded_case)
    return active_cases, excluded_cases


def validate_case_focus(cases: list[dict[str, object]]) -> list[str]:
    drifted_cases: list[str] = []
    for case in cases:
        feature_count = len(case.get("features", ()))
        if feature_count != 1:
            drifted_cases.append("%s(%d)" % (case["id"], feature_count))
    return drifted_cases


def validate_feature_uniqueness(cases: list[dict[str, object]]) -> list[str]:
    duplicate_features: list[str] = []
    for feature, case_ids in build_feature_case_map(cases).items():
        if len(case_ids) != 1:
            duplicate_features.append("%s(%s)" % (feature, ", ".join(case_ids)))
    return duplicate_features


def build_render_box_payload_from_values(width: int, height: int, composite_x: int, composite_y: int) -> dict[str, int]:
    return {
        "width": int(width),
        "height": int(height),
        "x": int(composite_x),
        "y": int(composite_y),
    }


def build_case_render_box_payload(case: dict[str, object]) -> dict[str, int]:
    return build_render_box_payload_from_values(
        int(case["render_width"]),
        int(case["render_height"]),
        int(case["composite_x"]),
        int(case["composite_y"]),
    )


def build_manifest_render_box_payload(case: dict[str, object]) -> dict[str, int]:
    return build_render_box_payload_from_values(
        int(case.get("manifest_render_width", case["render_width"])),
        int(case.get("manifest_render_height", case["render_height"])),
        int(case.get("manifest_composite_x", case["composite_x"])),
        int(case.get("manifest_composite_y", case["composite_y"])),
    )


def build_default_large_viewport_box() -> dict[str, int]:
    return build_render_box_payload_from_values(
        REVIEW_CANVAS_WIDTH,
        REVIEW_CANVAS_HEIGHT,
        0,
        0,
    )


def build_primary_user_cflags(viewport_box: dict[str, object] | None = None) -> str:
    active_viewport_box = dict(viewport_box) if isinstance(viewport_box, dict) else build_default_large_viewport_box()
    return (
        "-DEGUI_CONFIG_SCREEN_WIDTH=%d "
        "-DEGUI_CONFIG_SCREEN_HEIGHT=%d "
        "-DEGUI_CONFIG_PFB_WIDTH=%d "
        "-DEGUI_CONFIG_PFB_HEIGHT=%d "
        "-DHELLO_SVG_SPEC_VIEWPORT_X=%d "
        "-DHELLO_SVG_SPEC_VIEWPORT_Y=%d "
        "-DHELLO_SVG_SPEC_VIEWPORT_WIDTH=%d "
        "-DHELLO_SVG_SPEC_VIEWPORT_HEIGHT=%d"
    ) % (
        REVIEW_CANVAS_WIDTH,
        REVIEW_CANVAS_HEIGHT,
        REVIEW_PFB_WIDTH,
        REVIEW_PFB_HEIGHT,
        int(active_viewport_box["x"]),
        int(active_viewport_box["y"]),
        int(active_viewport_box["width"]),
        int(active_viewport_box["height"]),
    )


def prepare_large_viewport_codegen_context(
    cases: list[dict[str, object]],
    case_filter_active: bool,
    trial_render_box_spec: str,
) -> RuntimeCodegenContext:
    trial_spec = trial_render_box_spec.strip()
    default_box = build_default_large_viewport_box()
    if trial_spec and not case_filter_active:
        raise RuntimeError("--trial-render-box requires --case with exactly one case id")
    if trial_spec and len(cases) != 1:
        raise RuntimeError("--trial-render-box requires exactly one selected case")
    if not case_filter_active and not trial_spec:
        active_cases: list[dict[str, object]] = []
        for case in cases:
            active_case = dict(case)
            active_case["path"] = Path(case["path"])
            active_case["render_width"] = int(default_box["width"])
            active_case["render_height"] = int(default_box["height"])
            active_case["composite_x"] = int(default_box["x"])
            active_case["composite_y"] = int(default_box["y"])
            active_case["uses_default_render_box"] = True
            active_case["trial_render_box_active"] = False
            active_cases.append(active_case)
        return RuntimeCodegenContext(
            cases=active_cases,
            manifest_path=None,
            fixture_dir=None,
            trial_render_box_request=None,
            temp_dir=None,
        )

    temp_dir = tempfile.TemporaryDirectory(prefix="svg_spec_large_viewport_codegen_")
    temp_root = Path(temp_dir.name)
    temp_manifest_path = temp_root / "manifest.json"
    temp_manifest_payload: dict[str, object] = {"required_features": [], "cases": []}
    active_cases: list[dict[str, object]] = []
    trial_render_box_request: dict[str, object] | None = None

    for case in cases:
        active_case = dict(case)
        manifest_entry = json.loads(json.dumps(case["manifest_entry"]))
        active_case["path"] = Path(case["path"])
        width = int(default_box["width"])
        height = int(default_box["height"])
        composite_x = int(default_box["x"])
        composite_y = int(default_box["y"])

        if trial_spec:
            width, height, composite_x, composite_y = parse_trial_render_box(
                trial_spec,
                composite_x,
                composite_y,
                REVIEW_CANVAS_WIDTH,
                REVIEW_CANVAS_HEIGHT,
            )
            trial_render_box_request = {
                "case_id": str(case["id"]),
                "spec": trial_spec,
                "manifest_render_box": build_manifest_render_box_payload(case),
                "trial_render_box": build_render_box_payload_from_values(width, height, composite_x, composite_y),
            }
            active_case["trial_render_box_active"] = True
        else:
            active_case["trial_render_box_active"] = False

        active_case["render_width"] = width
        active_case["render_height"] = height
        active_case["composite_x"] = composite_x
        active_case["composite_y"] = composite_y
        active_case["uses_default_render_box"] = (
            width == REVIEW_CANVAS_WIDTH
            and height == REVIEW_CANVAS_HEIGHT
            and composite_x == 0
            and composite_y == 0
        )
        temp_manifest_payload["cases"].append(manifest_entry)
        active_cases.append(active_case)

    temp_manifest_path.write_text(json.dumps(temp_manifest_payload, indent=2, ensure_ascii=False) + "\n", encoding="utf-8", newline="\n")
    return RuntimeCodegenContext(
        cases=active_cases,
        manifest_path=temp_manifest_path,
        fixture_dir=FIXTURE_DIR,
        trial_render_box_request=trial_render_box_request,
        temp_dir=temp_dir,
    )


def parse_trial_render_box(raw_value: str, default_x: int, default_y: int, canvas_width: int, canvas_height: int) -> tuple[int, int, int, int]:
    spec = raw_value.strip()
    if not spec:
        raise RuntimeError("trial render box must not be empty")

    size_part = spec
    composite_x = default_x
    composite_y = default_y
    if "@" in spec:
        size_part, offset_part = spec.split("@", 1)
        offsets = [token.strip() for token in offset_part.split(",")]
        if len(offsets) != 2:
            raise RuntimeError("trial render box offset must use WIDTHxHEIGHT@X,Y")
        try:
            composite_x = int(offsets[0])
            composite_y = int(offsets[1])
        except ValueError as exc:
            raise RuntimeError("trial render box offset must be integers") from exc

    normalized_size_part = size_part.strip().lower()
    dimensions = [token.strip() for token in normalized_size_part.split("x")]
    if len(dimensions) != 2:
        raise RuntimeError("trial render box must use WIDTHxHEIGHT or WIDTHxHEIGHT@X,Y")
    try:
        width = int(dimensions[0])
        height = int(dimensions[1])
    except ValueError as exc:
        raise RuntimeError("trial render box dimensions must be integers") from exc

    if width <= 0 or height <= 0:
        raise RuntimeError("trial render box dimensions must be positive")
    if composite_x < 0 or composite_y < 0:
        raise RuntimeError("trial render box composite offset must be non-negative")
    if composite_x + width > canvas_width or composite_y + height > canvas_height:
        raise RuntimeError("trial render box exceeds the %dx%d reference canvas" % (canvas_width, canvas_height))
    return width, height, composite_x, composite_y


def print_case_listing(cases: list[dict[str, object]]) -> None:
    for case in cases:
        features = ", ".join(str(feature) for feature in case.get("features", ())) or "-"
        render_box = "%dx%d@%d,%d" % (
            int(case["render_width"]),
            int(case["render_height"]),
            int(case["composite_x"]),
            int(case["composite_y"]),
        )
        sensitivity = " render-size-sensitive" if bool(case.get("render_size_sensitive", False)) else ""
        reason = ""
        if bool(case.get("render_size_sensitive", False)):
            reason = ' reason="%s"' % str(case.get("render_size_reason", ""))
        shrink_locked = " shrink-locked" if bool(case.get("shrink_locked", False)) else ""
        shrink_reason = ""
        if bool(case.get("shrink_locked", False)):
            shrink_reason = ' shrink_reason="%s"' % str(case.get("shrink_lock_reason", ""))
        fuzzy_override = ""
        if not bool(case.get("uses_default_fuzzy", True)):
            fuzzy_override = ' fuzzy=%d/%d fuzzy_reason="%s"' % (
                int(case.get("fuzzy_max_difference", DEFAULT_FUZZY_MAX_DIFFERENCE)),
                int(case.get("fuzzy_max_diff_pixels", DEFAULT_FUZZY_MAX_DIFF_PIXELS)),
                str(case.get("fuzzy_override_reason", "")),
            )
        print(
            "%s: %s [%s] box=%s%s%s%s%s%s"
            % (case["id"], case["title"], features, render_box, sensitivity, reason, shrink_locked, shrink_reason, fuzzy_override)
        )


def print_feature_case_map(cases: list[dict[str, object]]) -> None:
    for feature, case_ids in build_feature_case_map(cases).items():
        print("%s: %s" % (feature, ", ".join(case_ids)))


def print_render_size_sensitive_cases(cases: list[dict[str, object]]) -> None:
    for case in cases:
        if not bool(case.get("render_size_sensitive", False)):
            continue
        print("%s: %s" % (str(case["id"]), str(case.get("render_size_reason", ""))))


def print_shrink_locked_cases(cases: list[dict[str, object]]) -> None:
    for case in cases:
        if not bool(case.get("shrink_locked", False)):
            continue
        print("%s: %s" % (str(case["id"]), str(case.get("shrink_lock_reason", ""))))


def print_fuzzy_override_cases(cases: list[dict[str, object]]) -> None:
    for case in cases:
        if bool(case.get("uses_default_fuzzy", True)):
            continue
        print(
            "%s: max_diff=%d diff_pixels=%d reason=%s"
            % (
                str(case["id"]),
                int(case.get("fuzzy_max_difference", DEFAULT_FUZZY_MAX_DIFFERENCE)),
                int(case.get("fuzzy_max_diff_pixels", DEFAULT_FUZZY_MAX_DIFF_PIXELS)),
                str(case.get("fuzzy_override_reason", "")),
            )
        )


def print_default_render_box_cases(cases: list[dict[str, object]]) -> None:
    for case in cases:
        if not bool(case.get("uses_default_render_box", False)):
            continue
        label = str(case["id"])
        if bool(case.get("render_size_sensitive", False)):
            label = "%s: %s" % (label, str(case.get("render_size_reason", "")))
        print(label)


def get_render_box_area(case: dict[str, object]) -> int:
    return int(case["render_width"]) * int(case["render_height"])


def order_cases_by_render_box(cases: list[dict[str, object]]) -> list[dict[str, object]]:
    return sorted(
        cases,
        key=lambda case: (
            get_render_box_area(case),
            int(case["render_width"]),
            int(case["render_height"]),
            str(case["id"]),
        ),
        reverse=True,
    )


def print_largest_render_boxes(cases: list[dict[str, object]]) -> None:
    for case in order_cases_by_render_box(cases):
        area = get_render_box_area(case)
        print(
            "%s: %dx%d area=%d"
            % (str(case["id"]), int(case["render_width"]), int(case["render_height"]), area)
        )


def build_shrink_candidates(cases: list[dict[str, object]]) -> list[dict[str, object]]:
    return [
        case
        for case in order_cases_by_render_box(cases)
        if not bool(case.get("render_size_sensitive", False)) and not bool(case.get("shrink_locked", False))
    ]


def print_shrink_candidates(cases: list[dict[str, object]]) -> None:
    reference_area = REFERENCE_CANVAS_WIDTH * REFERENCE_CANVAS_HEIGHT
    for case in build_shrink_candidates(cases):
        area = get_render_box_area(case)
        features = ", ".join(str(feature) for feature in case.get("features", ())) or "-"
        print(
            "%s: %dx%d area=%d canvas=%.2f%% feature=%s"
            % (
                str(case["id"]),
                int(case["render_width"]),
                int(case["render_height"]),
                area,
                (100.0 * float(area)) / float(reference_area),
                features,
            )
        )


def build_render_box_summary_entry(case: dict[str, object]) -> dict[str, object]:
    return {
        "id": str(case["id"]),
        "width": int(case["render_width"]),
        "height": int(case["render_height"]),
        "area": get_render_box_area(case),
        "canvas_ratio": float(get_render_box_area(case)) / float(REFERENCE_CANVAS_WIDTH * REFERENCE_CANVAS_HEIGHT),
        "render_size_sensitive": bool(case.get("render_size_sensitive", False)),
        "render_size_reason": str(case.get("render_size_reason", "")),
        "shrink_locked": bool(case.get("shrink_locked", False)),
        "shrink_lock_reason": str(case.get("shrink_lock_reason", "")),
        "uses_default_fuzzy": bool(case.get("uses_default_fuzzy", True)),
        "fuzzy_max_difference": int(case.get("fuzzy_max_difference", DEFAULT_FUZZY_MAX_DIFFERENCE)),
        "fuzzy_max_diff_pixels": int(case.get("fuzzy_max_diff_pixels", DEFAULT_FUZZY_MAX_DIFF_PIXELS)),
        "fuzzy_override_reason": str(case.get("fuzzy_override_reason", "")),
        "features": list(case.get("features", ())),
    }


def build_render_box_summary(
    cases: list[dict[str, object]],
    default_fuzzy_max_difference: int = DEFAULT_FUZZY_MAX_DIFFERENCE,
    default_fuzzy_max_diff_pixels: int = DEFAULT_FUZZY_MAX_DIFF_PIXELS,
) -> dict[str, object]:
    ordered_cases = order_cases_by_render_box(cases)
    shrink_candidates = build_shrink_candidates(cases)
    fuzzy_overrides = [case for case in cases if not bool(case.get("uses_default_fuzzy", True))]
    return {
        "reference_canvas": {
            "width": REFERENCE_CANVAS_WIDTH,
            "height": REFERENCE_CANVAS_HEIGHT,
            "area": REFERENCE_CANVAS_WIDTH * REFERENCE_CANVAS_HEIGHT,
        },
        "default_fuzzy": {
            "max_difference": int(default_fuzzy_max_difference),
            "max_diff_pixels": int(default_fuzzy_max_diff_pixels),
        },
        "default_render_box_case_count": sum(1 for case in cases if bool(case.get("uses_default_render_box", False))),
        "render_size_sensitive_case_count": sum(1 for case in cases if bool(case.get("render_size_sensitive", False))),
        "shrink_locked_case_count": sum(1 for case in cases if bool(case.get("shrink_locked", False))),
        "fuzzy_override_case_count": len(fuzzy_overrides),
        "shrink_candidate_case_count": len(shrink_candidates),
        "largest_render_boxes": [build_render_box_summary_entry(case) for case in ordered_cases[:RENDER_BOX_SUMMARY_LIMIT]],
        "fuzzy_overrides": [build_render_box_summary_entry(case) for case in sorted(fuzzy_overrides, key=lambda case: str(case["id"]))],
        "largest_shrink_candidates": [
            build_render_box_summary_entry(case) for case in shrink_candidates[:RENDER_BOX_SUMMARY_LIMIT]
        ],
    }


def resolve_artifact_path(path_value: str) -> Path:
    path = Path(path_value)
    if path.is_absolute():
        return path
    return ROOT_DIR / path


def apply_active_fuzzy_thresholds(
    cases: list[dict[str, object]],
    default_fuzzy_max_difference: int,
    default_fuzzy_max_diff_pixels: int,
) -> None:
    for case in cases:
        if bool(case.get("uses_default_fuzzy", True)):
            case["fuzzy_max_difference"] = int(default_fuzzy_max_difference)
            case["fuzzy_max_diff_pixels"] = int(default_fuzzy_max_diff_pixels)
        else:
            case["fuzzy_max_difference"] = int(case.get("fuzzy_max_difference", DEFAULT_FUZZY_MAX_DIFFERENCE))
            case["fuzzy_max_diff_pixels"] = int(case.get("fuzzy_max_diff_pixels", DEFAULT_FUZZY_MAX_DIFF_PIXELS))


def load_overview_font(size: int) -> ImageFont.ImageFont:
    candidates = (
        "arial.ttf",
        "segoeui.ttf",
        "DejaVuSans.ttf",
    )
    for candidate in candidates:
        try:
            return ImageFont.truetype(candidate, size)
        except OSError:
            continue
    return ImageFont.load_default()


def clip_text(draw: ImageDraw.ImageDraw, font: ImageFont.ImageFont, text: str, max_width: int) -> str:
    if not text:
        return ""

    if draw.textbbox((0, 0), text, font=font)[2] <= max_width:
        return text

    ellipsis = "..."
    clipped = text
    while clipped:
        clipped = clipped[:-1]
        candidate = clipped.rstrip() + ellipsis
        if draw.textbbox((0, 0), candidate, font=font)[2] <= max_width:
            return candidate
    return ellipsis


def summarize_features(features: list[str]) -> str:
    if not features:
        return "-"
    if len(features) <= 2:
        return ", ".join(features)
    return ", ".join(features[:2]) + ", +%d" % (len(features) - 2)


def crop_render_box(image: Image.Image, render_box: dict[str, object]) -> Image.Image:
    x = int(render_box["x"])
    y = int(render_box["y"])
    width = int(render_box["width"])
    height = int(render_box["height"])
    return image.crop((x, y, x + width, y + height)).convert("RGB")


def fit_preview(image: Image.Image, target_width: int, target_height: int, preserve_pixel_edges: bool = False) -> Image.Image:
    canvas = Image.new("RGB", (target_width, target_height), "white")
    width = max(1, int(image.width))
    height = max(1, int(image.height))
    scale = min(target_width / float(width), target_height / float(height))
    resized_width = max(1, int(round(width * scale)))
    resized_height = max(1, int(round(height * scale)))
    if preserve_pixel_edges:
        resampling = NEAREST_RESAMPLING
    elif scale < 1.0:
        resampling = HIGH_QUALITY_RESAMPLING
    else:
        resampling = NEAREST_RESAMPLING
    resized = image.resize((resized_width, resized_height), resample=resampling)
    offset_x = (target_width - resized_width) // 2
    offset_y = (target_height - resized_height) // 2
    canvas.paste(resized, (offset_x, offset_y))
    return canvas


def resolve_overview_diff_hot_threshold(case: dict[str, object]) -> int:
    review = case.get("review")
    if isinstance(review, dict):
        thresholds = review.get("thresholds")
        if isinstance(thresholds, dict):
            try:
                return max(0, int(thresholds.get("hot_pixel_delta", DEFAULT_HOT_PIXEL_DELTA)))
            except (TypeError, ValueError):
                return DEFAULT_HOT_PIXEL_DELTA
    thresholds = case.get("thresholds")
    if isinstance(thresholds, dict):
        try:
            return max(0, int(thresholds.get("hot_pixel_delta", DEFAULT_HOT_PIXEL_DELTA)))
        except (TypeError, ValueError):
            return DEFAULT_HOT_PIXEL_DELTA
    return DEFAULT_HOT_PIXEL_DELTA


def build_diff_preview(
    reference_crop: Image.Image,
    actual_crop: Image.Image,
    hot_threshold: int = DEFAULT_HOT_PIXEL_DELTA,
) -> OverviewDiffPreview:
    reference_array = np.array(quantize_rgb565(reference_crop), dtype=np.int16)
    actual_array = np.array(quantize_rgb565(actual_crop), dtype=np.int16)
    diff_array = np.abs(reference_array - actual_array)
    per_pixel_max_diff = np.max(diff_array, axis=2)
    nonzero_mask = per_pixel_max_diff > 0
    active_mask = np.max(diff_array, axis=2) >= max(0, int(hot_threshold))
    base_gray = np.array(actual_crop.convert("L"), dtype=np.uint8)
    preview_gray = 255 - (((255 - base_gray).astype(np.uint16) * 28) // 100)
    preview = np.repeat(preview_gray[:, :, np.newaxis], 3, axis=2).astype(np.uint8)
    max_difference = int(np.max(per_pixel_max_diff)) if per_pixel_max_diff.size else 0
    if np.any(nonzero_mask):
        overlay = np.zeros_like(preview, dtype=np.uint8)
        preview_scale = float(max(1, max(int(hot_threshold), max_difference)))
        severity = np.clip(per_pixel_max_diff.astype(np.float32) / preview_scale, 0.0, 1.0)
        hot_severity = np.zeros(per_pixel_max_diff.shape, dtype=np.float32)
        if max_difference > max(0, int(hot_threshold)):
            hot_severity = np.clip(
                (per_pixel_max_diff.astype(np.float32) - float(max(0, int(hot_threshold)))) /
                float(max(1, max_difference - max(0, int(hot_threshold)))),
                0.0,
                1.0,
            )
        overlay[..., 0] = 255
        overlay[..., 1] = np.clip(224.0 - (severity * 176.0), 48.0, 224.0).astype(np.uint8)
        overlay[..., 2] = np.clip(132.0 - (severity * 132.0), 0.0, 132.0).astype(np.uint8)
        alpha = np.zeros(per_pixel_max_diff.shape, dtype=np.float32)
        alpha[nonzero_mask] = 0.06 + (0.16 * severity[nonzero_mask])
        alpha[active_mask] = np.maximum(alpha[active_mask], 0.52 + (0.24 * hot_severity[active_mask]))
        blended = (
            preview.astype(np.float32) * (1.0 - alpha[..., np.newaxis]) +
            overlay.astype(np.float32) * alpha[..., np.newaxis]
        )
        preview[nonzero_mask] = np.clip(blended[nonzero_mask], 0.0, 255.0).astype(np.uint8)
    return OverviewDiffPreview(
        image=Image.fromarray(preview, mode="RGB"),
        diff_pixel_count=int(np.count_nonzero(nonzero_mask)),
        hot_pixel_count=int(np.count_nonzero(active_mask)),
        max_difference=max_difference,
    )


def annotate_preview_badge(
    preview: Image.Image,
    label_lines: list[str],
    font: ImageFont.ImageFont,
    background: tuple[int, int, int],
    foreground: tuple[int, int, int],
) -> Image.Image:
    normalized_lines = [str(line).strip() for line in label_lines if str(line).strip()]
    if not normalized_lines:
        return preview

    annotated = preview.copy()
    draw = ImageDraw.Draw(annotated)
    text = "\n".join(normalized_lines)
    badge_padding_x = 4
    badge_padding_y = 2
    badge_left = 3
    badge_top = 3
    badge_box = draw.multiline_textbbox((0, 0), text, font=font, spacing=1)
    badge_width = (badge_box[2] - badge_box[0]) + badge_padding_x * 2
    badge_height = (badge_box[3] - badge_box[1]) + badge_padding_y * 2
    badge_right = min(annotated.width - 4, badge_left + badge_width)
    badge_bottom = min(annotated.height - 4, badge_top + badge_height)
    draw.rectangle((badge_left, badge_top, badge_right, badge_bottom), fill=background)
    draw.multiline_text(
        (badge_left + badge_padding_x, badge_top + badge_padding_y - badge_box[1]),
        text,
        fill=foreground,
        font=font,
        spacing=1,
    )
    return annotated


def should_use_review_overview_preview(case: dict[str, object]) -> bool:
    render_box = dict(case.get("render_box", {}))
    width = int(render_box.get("width", 0))
    height = int(render_box.get("height", 0))
    review = case.get("review")
    if not isinstance(review, dict):
        return False
    if not str(review.get("reference", "")).strip() or not str(review.get("actual", "")).strip():
        return False
    review_render_box = dict(review.get("render_box", {}))
    review_width = int(review_render_box.get("width", 0))
    review_height = int(review_render_box.get("height", 0))
    if review_width <= width or review_height <= height:
        return False
    return width > 0 and height > 0 and (width * height) < OVERVIEW_MIN_CANONICAL_PREVIEW_AREA


def resolve_overview_preview_case_assets(case: dict[str, object]) -> tuple[Path, Path, dict[str, object]]:
    if should_use_review_overview_preview(case):
        review = dict(case.get("review", {}))
        return (
            resolve_artifact_path(str(review["reference"])),
            resolve_artifact_path(str(review["actual"])),
            dict(review.get("render_box", {})),
        )
    return (
        resolve_artifact_path(str(case["reference"])),
        resolve_artifact_path(str(case["actual"])),
        dict(case.get("render_box", {})),
    )


def resolve_overview_columns(case_count: int) -> int:
    if case_count <= 1:
        return 1
    if case_count <= 9:
        return 3
    if case_count <= 24:
        return 4
    return OVERVIEW_MAX_COLUMNS


def build_overview_legend_text() -> str:
    return "Each card shows large-viewport parity renders: reference | actual | diff map (all RGB565 deltas; hot >=%d emphasized)" % DEFAULT_HOT_PIXEL_DELTA


def render_overview_sheet(
    output_path: Path,
    case_results: list[dict[str, object]],
    coverage_features: list[str],
    reference_engine: str,
    with_unit: bool,
    columns: int,
    card_width: int,
    card_height: int,
    thumb_width: int,
    thumb_height: int,
    title_text: str,
    subtitle_text: str,
    legend_text: str,
    status_field: str = "status",
) -> dict[str, object]:
    title_font = load_overview_font(18)
    subtitle_font = load_overview_font(13)
    body_font = load_overview_font(12)
    badge_font = load_overview_font(11)
    case_count = len(case_results)
    passed_count = 0
    failed_count = 0
    skipped_count = 0
    for case in case_results:
        sheet_status = str(case.get(status_field, case.get("status", "failed"))).strip().lower()
        if sheet_status == "passed":
            passed_count += 1
        elif sheet_status == "skipped":
            skipped_count += 1
        else:
            failed_count += 1
    rows = int(math.ceil(case_count / float(columns)))
    canvas_width = (
        OVERVIEW_CARD_PADDING * 2
        + columns * card_width
        + max(0, columns - 1) * OVERVIEW_CARD_SPACING
    )
    canvas_height = (
        OVERVIEW_HEADER_HEIGHT
        + OVERVIEW_CARD_PADDING
        + rows * card_height
        + max(0, rows - 1) * OVERVIEW_CARD_SPACING
        + OVERVIEW_CARD_PADDING
    )
    canvas = Image.new("RGB", (canvas_width, canvas_height), (245, 247, 250))
    draw = ImageDraw.Draw(canvas)
    metadata_parts = [
        "cases=%d" % case_count,
        "passed=%d" % passed_count,
        "failed=%d" % failed_count,
    ]
    if skipped_count:
        metadata_parts.append("skipped=%d" % skipped_count)
    metadata_parts.extend(
        [
            "features=%d" % len(coverage_features),
            "engine=%s" % reference_engine,
            "with_unit=%s" % ("yes" if with_unit else "no"),
        ]
    )
    metadata_line = " ".join(metadata_parts)
    legend_line = legend_text

    draw.text((OVERVIEW_CARD_PADDING, 8), title_text, fill=(20, 24, 28), font=title_font)
    draw.text((OVERVIEW_CARD_PADDING, 30), metadata_line, fill=(20, 24, 28), font=subtitle_font)
    draw.text((OVERVIEW_CARD_PADDING, 48), legend_line, fill=(20, 24, 28), font=subtitle_font)
    draw.text((OVERVIEW_CARD_PADDING, 66), subtitle_text, fill=(80, 88, 96), font=subtitle_font)

    for index, case in enumerate(case_results):
        column = index % columns
        row = index // columns
        card_x = OVERVIEW_CARD_PADDING + column * (card_width + OVERVIEW_CARD_SPACING)
        card_y = OVERVIEW_HEADER_HEIGHT + OVERVIEW_CARD_PADDING + row * (card_height + OVERVIEW_CARD_SPACING)
        card_bounds = (card_x, card_y, card_x + card_width - 1, card_y + card_height - 1)
        status = str(case.get(status_field, case.get("status", "failed"))).strip().lower()
        if status == "passed":
            status_color = (39, 135, 71)
        elif status == "skipped":
            status_color = (178, 124, 0)
        else:
            status_color = (184, 56, 56)
        draw.rectangle(card_bounds, fill=(255, 255, 255), outline=(216, 220, 228))
        draw.rectangle((card_x, card_y, card_x + card_width - 1, card_y + 4), fill=status_color)

        content_x = card_x + 8
        content_width = card_width - 16
        title_line = clip_text(draw, body_font, "%03d %s" % (index + 1, str(case.get("title", ""))), content_width)
        id_text = clip_text(draw, body_font, str(case.get("id", "")), content_width)
        feature_text_source = str(case.get("sheet_feature_text", "")).strip()
        if not feature_text_source:
            feature_text_source = summarize_features(list(case.get("features", [])))
        feature_text = clip_text(draw, body_font, feature_text_source, content_width)
        preview_reference_path, preview_actual_path, preview_render_box = resolve_overview_preview_case_assets(case)
        render_box = dict(case.get("render_box", {}))
        box_text = "box %dx%d@%d,%d %s" % (
            int(render_box.get("width", 0)),
            int(render_box.get("height", 0)),
            int(render_box.get("x", 0)),
            int(render_box.get("y", 0)),
            status.upper(),
        )
        box_text = clip_text(draw, body_font, box_text, content_width)

        draw.text((content_x, card_y + 10), title_line, fill=(20, 24, 28), font=body_font)
        draw.text((content_x, card_y + 26), id_text, fill=(56, 63, 70), font=body_font)
        draw.text((content_x, card_y + 42), feature_text, fill=(56, 63, 70), font=body_font)
        draw.text((content_x, card_y + 58), box_text, fill=status_color, font=body_font)

        reference_crop = quantize_rgb565(crop_render_box(Image.open(preview_reference_path), preview_render_box))
        actual_crop = quantize_rgb565(crop_render_box(Image.open(preview_actual_path), preview_render_box))
        hot_threshold = resolve_overview_diff_hot_threshold(case)
        diff_preview = build_diff_preview(reference_crop, actual_crop, hot_threshold)

        thumb_y = card_y + OVERVIEW_THUMB_TOP
        thumb_gap = max(8, (content_width - thumb_width * 3) // 2)
        thumb_x_positions = (
            content_x,
            content_x + thumb_width + thumb_gap,
            content_x + (thumb_width + thumb_gap) * 2,
        )
        preview_triptych = (
            annotate_preview_badge(
                fit_preview(reference_crop, thumb_width, thumb_height),
                ["REF"],
                badge_font,
                (34, 42, 56),
                (255, 255, 255),
            ),
            annotate_preview_badge(
                fit_preview(actual_crop, thumb_width, thumb_height),
                ["ACT"],
                badge_font,
                (34, 42, 56),
                (255, 255, 255),
            ),
            annotate_preview_badge(
                fit_preview(diff_preview.image, thumb_width, thumb_height, preserve_pixel_edges=True),
                [
                    "DIFF max%d" % diff_preview.max_difference,
                    "all%d hot%d" % (diff_preview.diff_pixel_count, diff_preview.hot_pixel_count),
                ],
                badge_font,
                (126, 22, 22),
                (255, 255, 255),
            ),
        )
        for thumb_x, preview in zip(thumb_x_positions, preview_triptych):
            canvas.paste(preview, (thumb_x, thumb_y))
            draw.rectangle(
                (thumb_x, thumb_y, thumb_x + thumb_width - 1, thumb_y + thumb_height - 1),
                outline=(216, 220, 228),
            )

    canvas.save(output_path)
    return {
        "path": str(output_path.relative_to(ROOT_DIR)),
        "title": title_text,
        "subtitle": subtitle_text,
        "legend": legend_text,
        "status_field": status_field,
        "case_count": case_count,
        "passed_count": passed_count,
        "failed_count": failed_count,
        "skipped_count": skipped_count,
        "layout": {
            "columns": columns,
            "rows": rows,
            "card_width": card_width,
            "card_height": card_height,
            "thumb_width": thumb_width,
            "thumb_height": thumb_height,
        },
    }


def build_page_key(page_role: str, page_index: int) -> str:
    return "%s:%d" % (page_role, page_index)


def resolve_page_slot_target_type(page_role: str) -> str:
    slot_target_types = {
        "primary_user_overview": "default_overview_slot",
        "all_case_overview": "all_overview_slot",
        "review_page": "review_page_slot",
        "review_skipped_page": "review_skipped_page_slot",
    }
    return slot_target_types.get(page_role, "page_slot")


def annotate_artifact_page(page: dict[str, object], page_role: str, page_bucket: str, page_index: int, page_count: int) -> None:
    page["page_role"] = page_role
    page["page_bucket"] = page_bucket
    page["page_index"] = page_index
    page["page_count"] = page_count
    page["page_number"] = page_index + 1
    page["page_key"] = build_page_key(page_role, page_index)
    page_label = str(page.get("title", "")).strip()
    if not page_label:
        if page_count > 1:
            page_label = "%s %d/%d" % (page_role, page_index + 1, page_count)
        else:
            page_label = page_role
    page["page_label"] = page_label


def collect_artifact_page_records(artifacts: dict[str, object]) -> list[dict[str, object]]:
    page_records: list[dict[str, object]] = []
    for artifact_key in ("overview_sheet", "overview_all_sheet"):
        page = artifacts.get(artifact_key)
        if not isinstance(page, dict):
            continue
        case_ids = page.get("case_ids", [])
        layout = page.get("layout", {})
        page_records.append(
            {
                "path": str(page.get("path", "")),
                "title": str(page.get("title", "")),
                "subtitle": str(page.get("subtitle", "")),
                "legend": str(page.get("legend", "")),
                "status_field": str(page.get("status_field", "")),
                "case_count": int(page.get("case_count", len(case_ids) if isinstance(case_ids, list) else 0)),
                "passed_count": int(page.get("passed_count", 0)),
                "failed_count": int(page.get("failed_count", 0)),
                "skipped_count": int(page.get("skipped_count", 0)),
                "page_role": str(page.get("page_role", "")),
                "page_bucket": str(page.get("page_bucket", "")),
                "page_index": int(page.get("page_index", 0)),
                "page_number": int(page.get("page_number", int(page.get("page_index", 0)) + 1)),
                "page_count": int(page.get("page_count", 0)),
                "page_key": str(page.get("page_key", "")),
                "page_label": str(page.get("page_label", page.get("title", ""))),
                "case_ids": [str(case_id) for case_id in case_ids] if isinstance(case_ids, list) else [],
                "layout": dict(layout) if isinstance(layout, dict) else {},
            }
        )
    for artifact_key in ("review_pages", "review_skipped_pages"):
        pages = artifacts.get(artifact_key, [])
        if not isinstance(pages, list):
            continue
        for page in pages:
            if not isinstance(page, dict):
                continue
            case_ids = page.get("case_ids", [])
            layout = page.get("layout", {})
            page_records.append(
                {
                    "path": str(page.get("path", "")),
                    "title": str(page.get("title", "")),
                    "subtitle": str(page.get("subtitle", "")),
                    "legend": str(page.get("legend", "")),
                    "status_field": str(page.get("status_field", "")),
                    "case_count": int(page.get("case_count", len(case_ids) if isinstance(case_ids, list) else 0)),
                    "passed_count": int(page.get("passed_count", 0)),
                    "failed_count": int(page.get("failed_count", 0)),
                    "skipped_count": int(page.get("skipped_count", 0)),
                    "page_role": str(page.get("page_role", "")),
                    "page_bucket": str(page.get("page_bucket", "")),
                    "page_index": int(page.get("page_index", 0)),
                    "page_number": int(page.get("page_number", int(page.get("page_index", 0)) + 1)),
                    "page_count": int(page.get("page_count", 0)),
                    "page_key": str(page.get("page_key", "")),
                    "page_label": str(page.get("page_label", page.get("title", ""))),
                    "case_ids": [str(case_id) for case_id in case_ids] if isinstance(case_ids, list) else [],
                    "layout": dict(layout) if isinstance(layout, dict) else {},
                }
            )
    return page_records


def build_compact_page_ref(page: dict[str, object]) -> dict[str, object]:
    case_ids = page.get("case_ids", [])
    layout = page.get("layout", {})
    compact_page = {
        "path": str(page.get("path", "")),
        "title": str(page.get("title", "")),
        "subtitle": str(page.get("subtitle", "")),
        "legend": str(page.get("legend", "")),
        "status_field": str(page.get("status_field", "")),
        "case_count": int(page.get("case_count", len(case_ids) if isinstance(case_ids, list) else 0)),
        "passed_count": int(page.get("passed_count", 0)),
        "failed_count": int(page.get("failed_count", 0)),
        "skipped_count": int(page.get("skipped_count", 0)),
        "page_role": str(page.get("page_role", "")),
        "page_bucket": str(page.get("page_bucket", "")),
        "page_index": int(page.get("page_index", 0)),
        "page_number": int(page.get("page_number", int(page.get("page_index", 0)) + 1)),
        "page_count": int(page.get("page_count", 0)),
        "page_key": str(page.get("page_key", "")),
        "page_label": str(page.get("page_label", page.get("title", ""))),
        "case_ids": [str(case_id) for case_id in case_ids] if isinstance(case_ids, list) else [],
        "layout": dict(layout) if isinstance(layout, dict) else {},
    }
    if "slot_count" in page:
        compact_page["slot_count"] = int(page.get("slot_count", 0))
    return compact_page


def build_case_result_index(case_results: list[dict[str, object]]) -> dict[str, dict[str, object]]:
    case_result_index: dict[str, dict[str, object]] = {}
    for case in case_results:
        case_id = str(case.get("id", ""))
        if not case_id:
            continue
        case_result_index[case_id] = case
    return case_result_index


def attach_page_slot_records(page_records: list[dict[str, object]], case_results: list[dict[str, object]]) -> None:
    case_result_index = build_case_result_index(case_results)
    for page_record in page_records:
        page_key = str(page_record.get("page_key", ""))
        page_path = str(page_record.get("path", ""))
        page_role = str(page_record.get("page_role", ""))
        page_bucket = str(page_record.get("page_bucket", ""))
        page_index = int(page_record.get("page_index", 0))
        page_number = int(page_record.get("page_number", page_index + 1))
        page_count = int(page_record.get("page_count", max(page_number, 1)))
        page_label = str(page_record.get("page_label", page_record.get("title", "")))
        case_ids = page_record.get("case_ids", [])
        if not isinstance(case_ids, list):
            case_ids = []

        slots: list[dict[str, object]] = []
        slot_target_type = resolve_page_slot_target_type(page_role)
        slot_case_count = len(case_ids)
        for slot_index, case_id in enumerate(case_ids):
            case_key = str(case_id)
            case = case_result_index.get(case_key, {})
            review = case.get("review")
            review_payload = review if isinstance(review, dict) else {}
            primary_target = case.get("primary_visual_target", review_payload.get("primary_visual_target", {}))
            slot_target = {
                "target_type": slot_target_type,
                "path": page_path,
                "page_key": page_key,
                "page_role": page_role,
                "page_bucket": page_bucket,
                "page_index": page_index,
                "page_number": page_number,
                "page_count": page_count,
                "page_label": page_label,
                "slot_index": slot_index,
                "slot_number": slot_index + 1,
                "case_count": slot_case_count,
            }
            is_primary_target = (
                isinstance(primary_target, dict)
                and str(primary_target.get("page_key", "")) == page_key
                and int(primary_target.get("slot_index", -1)) == slot_index
            )
            slot = {
                "case_id": case_key,
                "title": str(case.get("title", "")),
                "status": str(case.get("review_status", review_payload.get("status", ""))),
                "canonical_status": str(case.get("status", "")),
                "slot_index": slot_index,
                "slot_number": slot_index + 1,
                "is_actionable": bool(review_payload.get("is_actionable", False)),
                "skip_reason": str(case.get("review_skip_reason", review_payload.get("skip_reason", ""))),
                "target": slot_target,
                "is_primary_visual_target": is_primary_target,
            }
            if isinstance(primary_target, dict):
                slot["primary_visual_target"] = dict(primary_target)
            slots.append(slot)

        page_record["slot_count"] = len(slots)
        page_record["slots"] = slots
        page_record["slot_map"] = {
            str(slot.get("case_id", "")): dict(slot)
            for slot in slots
            if str(slot.get("case_id", ""))
        }


def build_artifact_page_index(artifacts: dict[str, object], case_results: list[dict[str, object]] | None = None) -> dict[str, object]:
    pages = collect_artifact_page_records(artifacts)
    if case_results is not None:
        attach_page_slot_records(pages, case_results)
    by_path: dict[str, dict[str, object]] = {}
    by_key: dict[str, dict[str, object]] = {}
    for page in pages:
        page_path = str(page.get("path", ""))
        if page_path:
            by_path[page_path] = build_compact_page_ref(page)
        page_key = str(page.get("page_key", ""))
        if page_key:
            by_key[page_key] = build_compact_page_ref(page)
    return {
        "pages": pages,
        "by_path": by_path,
        "by_key": by_key,
    }


def build_artifact_page_groups(page_index: dict[str, object]) -> dict[str, object]:
    by_role: dict[str, list[str]] = {}
    by_bucket: dict[str, list[str]] = {}
    resolved = {
        "by_role": {},
        "by_bucket": {},
    }
    pages = page_index.get("pages", [])
    if not isinstance(pages, list):
        return {
            "by_role": by_role,
            "by_bucket": by_bucket,
            "resolved": resolved,
        }

    for page in pages:
        if not isinstance(page, dict):
            continue
        page_key = str(page.get("page_key", ""))
        if not page_key:
            continue
        page_role = str(page.get("page_role", ""))
        page_bucket = str(page.get("page_bucket", ""))
        if page_role:
            by_role.setdefault(page_role, []).append(page_key)
        if page_bucket:
            by_bucket.setdefault(page_bucket, []).append(page_key)
    page_index_by_key = page_index.get("by_key", {})
    if isinstance(page_index_by_key, dict):
        resolved["by_role"] = build_page_group_entries(by_role, page_index_by_key)
        resolved["by_bucket"] = build_page_group_entries(by_bucket, page_index_by_key)
    return {
        "by_role": by_role,
        "by_bucket": by_bucket,
        "resolved": resolved,
    }


def resolve_page_refs(page_index_by_key: dict[str, dict[str, object]], page_keys: list[str]) -> list[dict[str, object]]:
    page_refs: list[dict[str, object]] = []
    for page_key in page_keys:
        page = page_index_by_key.get(page_key)
        if page is None:
            continue
        page_refs.append(build_compact_page_ref(page))
    return page_refs


def build_page_group_entries(
    page_groups: dict[str, list[str]],
    page_index_by_key: dict[str, dict[str, object]],
) -> dict[str, dict[str, object]]:
    entries: dict[str, dict[str, object]] = {}
    for group_key, page_keys in page_groups.items():
        normalized_page_keys = [str(page_key) for page_key in page_keys if str(page_key)]
        entries[group_key] = {
            "page_keys": normalized_page_keys,
            "pages": resolve_page_refs(page_index_by_key, normalized_page_keys),
            "page_count": len(normalized_page_keys),
        }
    return entries


def build_overview_artifacts(
    runtime_dir: Path,
    display_case_results: list[dict[str, object]],
    coverage_features: list[str],
    reference_engine: str,
    with_unit: bool,
) -> dict[str, object]:
    actionable_review_cases = [
        case_result
        for case_result in display_case_results
        if str(case_result.get("review_status", "")).strip().lower() != "skipped"
    ]
    skipped_review_cases: list[dict[str, object]] = []
    for case_result in display_case_results:
        if str(case_result.get("review_status", "")).strip().lower() != "skipped":
            continue
        skipped_case = dict(case_result)
        skip_reason = str(case_result.get("review_skip_reason", "")).strip()
        if skip_reason:
            skipped_case["sheet_feature_text"] = skip_reason
        skipped_review_cases.append(skipped_case)
    debug_skipped_review_cases = [
        case_result
        for case_result in skipped_review_cases
        if bool(case_result.get("review_skip_debug_included", False))
    ]
    covered_skipped_review_cases = [
        case_result
        for case_result in skipped_review_cases
        if not bool(case_result.get("review_skip_debug_included", False))
    ]
    all_cases_overview_cases: list[dict[str, object]] = []
    skipped_case_ids = {str(case.get("id", "")) for case in skipped_review_cases}
    skipped_case_map = {str(case.get("id", "")): case for case in skipped_review_cases}
    for case_result in display_case_results:
        case_id = str(case_result.get("id", ""))
        if case_id in skipped_case_ids:
            all_cases_overview_cases.append(dict(skipped_case_map[case_id]))
        else:
            all_cases_overview_cases.append(case_result)

    def collect_sheet_features(case_results: list[dict[str, object]]) -> list[str]:
        case_feature_set: set[str] = set()
        for case_result in case_results:
            for feature in list(case_result.get("features", [])):
                feature_name = str(feature).strip()
                if feature_name:
                    case_feature_set.add(feature_name)
        return [feature for feature in coverage_features if feature in case_feature_set]

    default_overview_cases = actionable_review_cases if actionable_review_cases else display_case_results
    total_skip_count = len(skipped_review_cases)
    debug_skip_count = len(debug_skipped_review_cases)
    covered_skip_count = len(covered_skipped_review_cases)
    if total_skip_count:
        if debug_skip_count:
            default_overview_subtitle = (
                "Default overview sheet for actionable large-viewport parity cases. %d unresolved skipped cases are routed to skipped debug pages; %d covered skips stay in the all-case sheet."
                % (debug_skip_count, covered_skip_count)
            )
            review_page_subtitle = (
                "Detail sheet for actionable large-viewport parity cases. %d unresolved skipped cases are routed to debug pages."
                % debug_skip_count
            )
            skipped_review_subtitle = "Detail sheet for %d unresolved large-viewport skips that still need manual follow-up." % debug_skip_count
        else:
            default_overview_subtitle = (
                "Default overview sheet for actionable large-viewport parity cases. %d skipped cases are structurally covered elsewhere and remain only in the all-case sheet."
                % covered_skip_count
            )
            review_page_subtitle = "Detail sheet for actionable large-viewport parity cases."
            skipped_review_subtitle = "Detail sheet for unresolved large-viewport skips kept for debugging only."
        all_cases_overview_subtitle = (
            "All-case overview sheet including %d skipped parity cases (%d unresolved, %d covered elsewhere) for coverage and debugging."
            % (total_skip_count, debug_skip_count, covered_skip_count)
        )
    else:
        default_overview_subtitle = "Default overview sheet for actionable large-viewport parity cases."
        review_page_subtitle = "Detail sheet for actionable large-viewport parity cases."
        all_cases_overview_subtitle = "All-case overview sheet for coverage and debugging."
        skipped_review_subtitle = "Detail sheet for skipped large-viewport parity cases kept for debugging only."
    overview_path = runtime_dir / "svg_validation_overview.png"
    overview_columns = resolve_overview_columns(len(default_overview_cases))
    compact_sheet = render_overview_sheet(
        overview_path,
        default_overview_cases,
        collect_sheet_features(default_overview_cases),
        reference_engine,
        with_unit,
        overview_columns,
        OVERVIEW_CARD_WIDTH,
        OVERVIEW_CARD_HEIGHT,
        OVERVIEW_THUMB_WIDTH,
        OVERVIEW_THUMB_HEIGHT,
        "SVG Validation Overview",
        default_overview_subtitle,
        build_overview_legend_text(),
        status_field="review_status",
    )
    compact_sheet["case_ids"] = [str(case.get("id", "")) for case in default_overview_cases]
    annotate_artifact_page(compact_sheet, "primary_user_overview", "actionable", 0, 1)

    all_cases_overview_path = runtime_dir / "svg_validation_overview_all_cases.png"
    all_cases_overview_columns = resolve_overview_columns(len(all_cases_overview_cases))
    all_cases_sheet = render_overview_sheet(
        all_cases_overview_path,
        all_cases_overview_cases,
        collect_sheet_features(all_cases_overview_cases),
        reference_engine,
        with_unit,
        all_cases_overview_columns,
        OVERVIEW_CARD_WIDTH,
        OVERVIEW_CARD_HEIGHT,
        OVERVIEW_THUMB_WIDTH,
        OVERVIEW_THUMB_HEIGHT,
        "SVG Validation All-Case Overview",
        all_cases_overview_subtitle,
        "%s; skipped cards replace the feature label with the clipped skip reason." % build_overview_legend_text(),
        status_field="review_status",
    )
    all_cases_sheet["case_ids"] = [str(case.get("id", "")) for case in all_cases_overview_cases]
    annotate_artifact_page(all_cases_sheet, "all_case_overview", "all_cases", 0, 1)

    review_dir = runtime_dir / "svg_validation_review_pages"
    shutil.rmtree(review_dir, ignore_errors=True)
    review_dir.mkdir(parents=True, exist_ok=True)
    review_cases_per_page = OVERVIEW_REVIEW_COLUMNS * OVERVIEW_REVIEW_ROWS
    review_pages: list[dict[str, object]] = []
    page_count = int(math.ceil(len(actionable_review_cases) / float(review_cases_per_page))) if actionable_review_cases else 0
    for page_index in range(page_count):
        page_start = page_index * review_cases_per_page
        page_cases = actionable_review_cases[page_start:page_start + review_cases_per_page]
        page_path = review_dir / ("svg_validation_review_%02d.png" % (page_index + 1))
        review_sheet = render_overview_sheet(
            page_path,
            page_cases,
            collect_sheet_features(page_cases),
            reference_engine,
            with_unit,
            OVERVIEW_REVIEW_COLUMNS,
            OVERVIEW_REVIEW_CARD_WIDTH,
            OVERVIEW_REVIEW_CARD_HEIGHT,
            OVERVIEW_REVIEW_THUMB_WIDTH,
            OVERVIEW_REVIEW_THUMB_HEIGHT,
            "SVG Validation Review Page %d/%d" % (page_index + 1, page_count),
            review_page_subtitle,
            build_overview_legend_text(),
            status_field="review_status",
        )
        review_sheet["case_ids"] = [str(case.get("id", "")) for case in page_cases]
        annotate_artifact_page(review_sheet, "review_page", "actionable", page_index, page_count)
        review_pages.append(review_sheet)

    skipped_review_dir: Path | None = None
    skipped_review_pages: list[dict[str, object]] = []
    skipped_page_count = int(math.ceil(len(debug_skipped_review_cases) / float(review_cases_per_page))) if debug_skipped_review_cases else 0
    if skipped_page_count:
        skipped_review_dir = runtime_dir / "svg_validation_review_skipped_pages"
        shutil.rmtree(skipped_review_dir, ignore_errors=True)
        skipped_review_dir.mkdir(parents=True, exist_ok=True)
        for page_index in range(skipped_page_count):
            page_start = page_index * review_cases_per_page
            page_cases = debug_skipped_review_cases[page_start:page_start + review_cases_per_page]
            page_path = skipped_review_dir / ("svg_validation_review_skipped_%02d.png" % (page_index + 1))
            skipped_sheet = render_overview_sheet(
                page_path,
                page_cases,
                collect_sheet_features(page_cases),
                reference_engine,
                with_unit,
                OVERVIEW_REVIEW_COLUMNS,
                OVERVIEW_REVIEW_CARD_WIDTH,
                OVERVIEW_REVIEW_CARD_HEIGHT,
                OVERVIEW_REVIEW_THUMB_WIDTH,
                OVERVIEW_REVIEW_THUMB_HEIGHT,
                "SVG Validation Skipped Review Page %d/%d" % (page_index + 1, skipped_page_count),
                skipped_review_subtitle,
                "%s; skipped cards also show the clipped skip reason." % build_overview_legend_text(),
                status_field="review_status",
            )
            skipped_sheet["case_ids"] = [str(case.get("id", "")) for case in page_cases]
            annotate_artifact_page(skipped_sheet, "review_skipped_page", "skipped", page_index, skipped_page_count)
            skipped_review_pages.append(skipped_sheet)

    artifacts = {
        "overview_image": compact_sheet["path"],
        "layout": compact_sheet["layout"],
        "overview_sheet": compact_sheet,
        "overview_mode": "actionable_review_cases" if actionable_review_cases else "all_cases_fallback",
        "overview_case_ids": [str(case.get("id", "")) for case in default_overview_cases],
        "overview_excluded_case_ids": [str(case.get("id", "")) for case in skipped_review_cases],
        "overview_all_image": all_cases_sheet["path"],
        "overview_all_layout": all_cases_sheet["layout"],
        "overview_all_sheet": all_cases_sheet,
        "overview_all_case_ids": [str(case.get("id", "")) for case in display_case_results],
        "review_pages_dir": str(review_dir.relative_to(ROOT_DIR)),
        "review_pages": review_pages,
        "legend": {
            "panels": ["reference", "actual", "diff_map"],
            "preview_mode": "large_viewport_parity_diff_map",
        },
        "overview_summary": {
            "default_case_count": len(default_overview_cases),
            "default_feature_count": len(collect_sheet_features(default_overview_cases)),
            "all_case_count": len(display_case_results),
            "all_feature_count": len(coverage_features),
        },
        "review_page_summary": {
            "actionable_case_count": len(actionable_review_cases),
            "actionable_page_count": len(review_pages),
            "actionable_case_ids": [str(case.get("id", "")) for case in actionable_review_cases],
            "skipped_case_count": len(skipped_review_cases),
            "covered_skipped_case_count": covered_skip_count,
            "covered_skipped_case_ids": [str(case.get("id", "")) for case in covered_skipped_review_cases],
            "unresolved_skipped_case_count": debug_skip_count,
            "unresolved_skipped_case_ids": [str(case.get("id", "")) for case in debug_skipped_review_cases],
            "skipped_page_count": len(skipped_review_pages),
            "skipped_case_ids": [str(case.get("id", "")) for case in skipped_review_cases],
        },
    }
    if skipped_review_dir is not None:
        artifacts["review_skipped_pages_dir"] = str(skipped_review_dir.relative_to(ROOT_DIR))
        artifacts["review_skipped_pages"] = skipped_review_pages
    return artifacts


def write_validation_report(
    runtime_dir: Path,
    reference_engine: str,
    reference_detail: str,
    coverage_features: list[str],
    required_features: tuple[str, ...],
    feature_case_map: dict[str, list[str]],
    render_box_summary: dict[str, object],
    trial_render_box_request: dict[str, object] | None,
    artifacts: dict[str, object],
    case_results: list[dict[str, object]],
    review_mode: str,
    review_failures: list[dict[str, object]],
    review_skips: list[dict[str, object]],
    with_unit: bool,
    unit_message: str | None,
) -> Path:
    report_path = runtime_dir / "svg_validation_report.json"
    page_index = build_artifact_page_index(artifacts, case_results)
    review_cases = build_review_case_summaries(case_results, artifacts)
    review_family_map = build_review_family_map(case_results, page_index)
    review_families = build_review_family_list(case_results, review_family_map)
    payload = {
        "app": APP_NAME,
        "reference_engine": reference_engine,
        "reference_detail": reference_detail,
        "coverage_features": coverage_features,
        "required_features": list(required_features),
        "feature_case_map": feature_case_map,
        "render_box_summary": render_box_summary,
        "trial_render_box_request": trial_render_box_request,
        "artifacts": artifacts,
        "review_summary": build_review_summary(review_mode, case_results, review_failures, review_skips),
        "review_case_groups": build_review_case_groups(review_cases),
        "review_families": review_families,
        "review_family_map": review_family_map,
        "review_family_groups": build_review_family_groups(review_families),
        "with_unit": with_unit,
        "unit_message": unit_message,
        "cases": case_results,
    }
    report_path.write_text(json.dumps(payload, indent=2, ensure_ascii=False) + "\n", encoding="utf-8", newline="\n")
    return report_path


def attach_review_page_refs(case_results: list[dict[str, object]], artifacts: dict[str, object]) -> None:
    page_refs: dict[str, dict[str, object]] = {}
    page_groups = (
        ("review_pages", "actionable"),
        ("review_skipped_pages", "skipped"),
    )
    for artifact_key, _bucket in page_groups:
        pages = artifacts.get(artifact_key, [])
        if not isinstance(pages, list):
            continue
        for page_index, page in enumerate(pages):
            if not isinstance(page, dict):
                continue
            page_path = str(page.get("path", ""))
            case_ids = page.get("case_ids", [])
            if not isinstance(case_ids, list):
                continue
            for page_slot_index, case_id in enumerate(case_ids):
                page_refs[str(case_id)] = {
                    "page_path": page_path,
                    "page_index": page_index,
                    "page_slot_index": page_slot_index,
                }

    for case in case_results:
        case_id = str(case.get("id", ""))
        page_ref = page_refs.get(case_id)
        if page_ref is None:
            continue
        case["review_page_path"] = str(page_ref["page_path"])
        case["review_page_index"] = int(page_ref["page_index"])
        case["review_page_slot_index"] = int(page_ref["page_slot_index"])
        review = case.get("review")
        if isinstance(review, dict):
            review["page_path"] = str(page_ref["page_path"])
            review["page_index"] = int(page_ref["page_index"])
            review["page_slot_index"] = int(page_ref["page_slot_index"])


def attach_overview_refs(case_results: list[dict[str, object]], artifacts: dict[str, object]) -> None:
    default_overview_path = str(artifacts.get("overview_image", ""))
    all_overview_path = str(artifacts.get("overview_all_image", ""))
    default_overview_case_ids = artifacts.get("overview_case_ids", [])
    all_overview_case_ids = artifacts.get("overview_all_case_ids", [])
    default_overview_index_map = {
        str(case_id): index
        for index, case_id in enumerate(default_overview_case_ids)
        if str(case_id)
    }
    all_overview_index_map = {
        str(case_id): index
        for index, case_id in enumerate(all_overview_case_ids)
        if str(case_id)
    }

    for case in case_results:
        case_id = str(case.get("id", ""))
        default_index = default_overview_index_map.get(case_id)
        all_index = all_overview_index_map.get(case_id)
        case["is_in_default_overview"] = default_index is not None
        if default_index is not None:
            case["default_overview_path"] = default_overview_path
            case["default_overview_slot_index"] = int(default_index)
        if all_index is not None:
            case["all_overview_path"] = all_overview_path
            case["all_overview_slot_index"] = int(all_index)
        review = case.get("review")
        if isinstance(review, dict):
            review["is_in_default_overview"] = default_index is not None
            if default_index is not None:
                review["default_overview_path"] = default_overview_path
                review["default_overview_slot_index"] = int(default_index)
            if all_index is not None:
                review["all_overview_path"] = all_overview_path
                review["all_overview_slot_index"] = int(all_index)


def build_page_slot_target(
    target_type: str,
    page_path: str,
    page_index: int,
    page_slot_index: int,
    fallback_page_role: str,
    fallback_page_bucket: str,
    page_index_by_path: dict[str, dict[str, object]],
) -> dict[str, object]:
    if not page_path:
        return {}

    page_record = page_index_by_path.get(page_path, {})
    page_role = str(page_record.get("page_role", fallback_page_role))
    page_bucket = str(page_record.get("page_bucket", fallback_page_bucket))
    target = {
        "target_type": target_type,
        "path": page_path,
        "page_key": str(page_record.get("page_key", build_page_key(page_role, page_index))),
        "page_role": page_role,
        "page_bucket": page_bucket,
        "page_index": int(page_index),
        "page_number": int(page_record.get("page_number", page_index + 1)),
        "page_count": int(page_record.get("page_count", max(page_index + 1, 1))),
        "page_label": str(page_record.get("page_label", page_record.get("title", ""))),
        "slot_index": int(page_slot_index),
        "slot_number": int(page_slot_index) + 1,
    }
    case_ids = page_record.get("case_ids", [])
    if isinstance(case_ids, list):
        target["case_count"] = len(case_ids)
    return target


def dedupe_visual_targets(targets: list[dict[str, object]]) -> list[dict[str, object]]:
    unique_targets: list[dict[str, object]] = []
    seen_keys: set[tuple[str, str, int]] = set()
    for target in targets:
        if not isinstance(target, dict) or not target:
            continue
        target_key = (
            str(target.get("target_type", "")),
            str(target.get("page_key", "")),
            int(target.get("slot_index", -1)),
        )
        if target_key in seen_keys:
            continue
        seen_keys.add(target_key)
        unique_targets.append(dict(target))
    return unique_targets


def dedupe_strings(values: list[str]) -> list[str]:
    unique_values: list[str] = []
    seen_values: set[str] = set()
    for value in values:
        normalized_value = str(value)
        if not normalized_value or normalized_value in seen_values:
            continue
        seen_values.add(normalized_value)
        unique_values.append(normalized_value)
    return unique_values


def normalize_review_family_key(case_id: str) -> str:
    tokens = [token for token in case_id.split("_") if token]
    if len(tokens) > 1:
        tokens = [token for index, token in enumerate(tokens) if not (token == "inset" and index > 0)]
    if len(tokens) >= 3 and tokens[-3:] == ["wide", "fractional", "scale"]:
        tokens = tokens[:-3]
    elif len(tokens) >= 2 and tokens[-2:] == ["fractional", "scale"]:
        tokens = tokens[:-2]
    if len(tokens) >= 2 and tokens[-1] == "nested" and tokens[0] != "nested":
        tokens = tokens[:-1]
    return "_".join(tokens)


def build_review_family_index(case_results: list[dict[str, object]]) -> dict[str, dict[str, object]]:
    family_index: dict[str, dict[str, object]] = {}
    for case in case_results:
        case_id = str(case.get("id", ""))
        if not case_id:
            continue
        family_key = normalize_review_family_key(case_id)
        family = family_index.setdefault(
            family_key,
            {
                "family_key": family_key,
                "case_ids": [],
                "actionable_case_ids": [],
                "skipped_case_ids": [],
            },
        )
        family["case_ids"].append(case_id)
        if str(case.get("review_status", "")).strip().lower() == "skipped":
            family["skipped_case_ids"].append(case_id)
        else:
            family["actionable_case_ids"].append(case_id)
    for family in family_index.values():
        family["case_count"] = len(family["case_ids"])
        family["actionable_case_count"] = len(family["actionable_case_ids"])
        family["skipped_case_count"] = len(family["skipped_case_ids"])
    return family_index


def attach_review_family_refs(case_results: list[dict[str, object]]) -> None:
    family_index = build_review_family_index(case_results)
    for case in case_results:
        case_id = str(case.get("id", ""))
        if not case_id:
            continue
        family_key = normalize_review_family_key(case_id)
        family = family_index.get(
            family_key,
            {
                "family_key": family_key,
                "case_ids": [case_id],
                "actionable_case_ids": [],
                "skipped_case_ids": [],
                "case_count": 1,
                "actionable_case_count": 0,
                "skipped_case_count": 0,
            },
        )
        related_case_ids = [str(item) for item in family.get("case_ids", []) if str(item) != case_id]
        related_actionable_case_ids = [str(item) for item in family.get("actionable_case_ids", []) if str(item) != case_id]
        related_skipped_case_ids = [str(item) for item in family.get("skipped_case_ids", []) if str(item) != case_id]
        case["review_family_key"] = family_key
        case["review_family_case_ids"] = [str(item) for item in family.get("case_ids", [])]
        case["review_related_case_ids"] = related_case_ids
        case["review_related_actionable_case_ids"] = related_actionable_case_ids
        case["review_related_skipped_case_ids"] = related_skipped_case_ids
        review = case.get("review")
        if isinstance(review, dict):
            review["family_key"] = family_key
            review["family_case_ids"] = [str(item) for item in family.get("case_ids", [])]
            review["related_case_ids"] = related_case_ids
            review["related_actionable_case_ids"] = related_actionable_case_ids
            review["related_skipped_case_ids"] = related_skipped_case_ids


def build_review_family_entrypoint(
    cases: list[dict[str, object]],
    visual_targets: list[dict[str, object]],
    page_index_by_key: dict[str, dict[str, object]],
    source_mode: str,
) -> dict[str, object]:
    compact_cases = [build_compact_review_case_ref(case) for case in cases]
    page_keys = dedupe_strings(
        [str(target.get("page_key", "")) for target in visual_targets if str(target.get("page_key", ""))]
    )
    return {
        "source_mode": source_mode,
        "case_ids": [str(case.get("id", "")) for case in compact_cases],
        "case_count": len(compact_cases),
        "visual_target_count": len(visual_targets),
        "page_keys": page_keys,
        "page_count": len(page_keys),
    }


def build_review_family_map(
    case_results: list[dict[str, object]],
    page_index: dict[str, object] | None = None,
) -> dict[str, dict[str, object]]:
    family_index = build_review_family_index(case_results)
    case_map = {str(case.get("id", "")): case for case in case_results if str(case.get("id", ""))}
    page_index_by_key = page_index.get("by_key", {}) if isinstance(page_index, dict) else {}
    family_map: dict[str, dict[str, object]] = {}
    for family_key, family in family_index.items():
        family_case_ids = [str(item) for item in family.get("case_ids", [])]
        actionable_case_ids = [str(item) for item in family.get("actionable_case_ids", [])]
        skipped_case_ids = [str(item) for item in family.get("skipped_case_ids", [])]
        family_cases = [case_map[case_id] for case_id in family_case_ids if case_id in case_map]
        actionable_cases = [case_map[case_id] for case_id in actionable_case_ids if case_id in case_map]
        skipped_cases = [case_map[case_id] for case_id in skipped_case_ids if case_id in case_map]
        primary_visual_targets = dedupe_visual_targets(
            [
                dict(case.get("primary_visual_target", {}))
                for case in family_cases
                if isinstance(case.get("primary_visual_target"), dict)
            ]
        )
        actionable_primary_visual_targets = dedupe_visual_targets(
            [
                dict(case.get("primary_visual_target", {}))
                for case in actionable_cases
                if isinstance(case.get("primary_visual_target"), dict)
            ]
        )
        skipped_primary_visual_targets = dedupe_visual_targets(
            [
                dict(case.get("primary_visual_target", {}))
                for case in skipped_cases
                if isinstance(case.get("primary_visual_target"), dict)
            ]
        )
        available_visual_targets = dedupe_visual_targets(
            [
                dict(target)
                for case in family_cases
                for target in (
                    case.get("visual_targets", {}).get("available", [])
                    if isinstance(case.get("visual_targets"), dict)
                    else []
                )
                if isinstance(target, dict)
            ]
        )
        preferred_review_cases = actionable_cases if actionable_cases else family_cases
        preferred_review_targets = actionable_primary_visual_targets if actionable_primary_visual_targets else primary_visual_targets
        preferred_review_source_mode = "actionable_cases" if actionable_cases else "all_cases_fallback"
        family_map[family_key] = {
            "family_key": family_key,
            "case_ids": family_case_ids,
            "cases": [
                build_compact_review_case_ref(case)
                for case in family_cases
            ],
            "actionable_case_ids": actionable_case_ids,
            "skipped_case_ids": skipped_case_ids,
            "actionable_cases": [
                build_compact_review_case_ref(case)
                for case in actionable_cases
            ],
            "skipped_cases": [
                build_compact_review_case_ref(case)
                for case in skipped_cases
            ],
            "primary_visual_targets": primary_visual_targets,
            "actionable_primary_visual_targets": actionable_primary_visual_targets,
            "skipped_primary_visual_targets": skipped_primary_visual_targets,
            "available_visual_targets": available_visual_targets,
            "primary_page_keys": dedupe_strings(
                [str(target.get("page_key", "")) for target in primary_visual_targets if str(target.get("page_key", ""))]
            ),
            "available_page_keys": dedupe_strings(
                [str(target.get("page_key", "")) for target in available_visual_targets if str(target.get("page_key", ""))]
            ),
            "entrypoints": {
                "primary_user_review": build_review_family_entrypoint(
                    preferred_review_cases,
                    preferred_review_targets,
                    page_index_by_key,
                    preferred_review_source_mode,
                ),
                "actionable_review": build_review_family_entrypoint(
                    actionable_cases,
                    actionable_primary_visual_targets,
                    page_index_by_key,
                    "actionable_cases",
                ),
                "skipped_review_debug": build_review_family_entrypoint(
                    skipped_cases,
                    skipped_primary_visual_targets,
                    page_index_by_key,
                    "skipped_cases",
                ),
                "all_family_debug": build_review_family_entrypoint(
                    family_cases,
                    available_visual_targets,
                    page_index_by_key,
                    "all_family_cases",
                ),
            },
            "case_count": int(family.get("case_count", 0)),
            "actionable_case_count": int(family.get("actionable_case_count", 0)),
            "skipped_case_count": int(family.get("skipped_case_count", 0)),
        }
    return family_map


def build_review_family_list(
    case_results: list[dict[str, object]],
    review_family_map: dict[str, dict[str, object]],
) -> list[dict[str, object]]:
    family_list: list[dict[str, object]] = []
    seen_family_keys: set[str] = set()
    for case in case_results:
        case_id = str(case.get("id", ""))
        family_key = str(case.get("review_family_key", normalize_review_family_key(case_id)))
        if not family_key or family_key in seen_family_keys:
            continue
        family = review_family_map.get(family_key)
        if family is None:
            continue
        seen_family_keys.add(family_key)
        family_list.append(dict(family))
    for family_key, family in review_family_map.items():
        if family_key in seen_family_keys:
            continue
        family_list.append(dict(family))
    return family_list


def build_review_family_group_entries(
    family_groups: dict[str, list[str]],
    review_family_map: dict[str, dict[str, object]],
) -> dict[str, dict[str, object]]:
    entries: dict[str, dict[str, object]] = {}
    for group_key, family_keys in family_groups.items():
        normalized_family_keys = [str(family_key) for family_key in family_keys if str(family_key)]
        entries[group_key] = {
            "family_keys": normalized_family_keys,
            "families": [
                dict(review_family_map[family_key])
                for family_key in normalized_family_keys
                if family_key in review_family_map
            ],
            "family_count": len(normalized_family_keys),
        }
    return entries


def build_review_family_groups(review_families: list[dict[str, object]]) -> dict[str, object]:
    groups = {
        "by_actionability": {},
        "by_preferred_source_mode": {},
        "by_preferred_page_key": {},
        "by_debug_page_key": {},
        "resolved": {},
    }
    review_family_map = {
        str(family.get("family_key", "")): family
        for family in review_families
        if str(family.get("family_key", ""))
    }
    for family in review_families:
        family_key = str(family.get("family_key", ""))
        if not family_key:
            continue
        actionable_case_count = int(family.get("actionable_case_count", 0))
        skipped_case_count = int(family.get("skipped_case_count", 0))
        if actionable_case_count and skipped_case_count:
            actionability_key = "mixed"
        elif actionable_case_count:
            actionability_key = "actionable_only"
        else:
            actionability_key = "skipped_only"
        groups["by_actionability"].setdefault(actionability_key, []).append(family_key)
        entrypoints = family.get("entrypoints", {})
        if isinstance(entrypoints, dict):
            preferred_entrypoint = entrypoints.get("primary_user_review", {})
            skipped_entrypoint = entrypoints.get("skipped_review_debug", {})
            if isinstance(preferred_entrypoint, dict):
                preferred_source_mode = str(preferred_entrypoint.get("source_mode", ""))
                if preferred_source_mode:
                    groups["by_preferred_source_mode"].setdefault(preferred_source_mode, []).append(family_key)
                preferred_page_keys = preferred_entrypoint.get("page_keys", [])
                if isinstance(preferred_page_keys, list):
                    for page_key in preferred_page_keys:
                        normalized_page_key = str(page_key)
                        if normalized_page_key:
                            groups["by_preferred_page_key"].setdefault(normalized_page_key, []).append(family_key)
            if isinstance(skipped_entrypoint, dict):
                skipped_page_keys = skipped_entrypoint.get("page_keys", [])
                if isinstance(skipped_page_keys, list):
                    for page_key in skipped_page_keys:
                        normalized_page_key = str(page_key)
                        if normalized_page_key:
                            groups["by_debug_page_key"].setdefault(normalized_page_key, []).append(family_key)
    groups["resolved"] = {
        "by_actionability": build_review_family_group_entries(groups["by_actionability"], review_family_map),
        "by_preferred_source_mode": build_review_family_group_entries(groups["by_preferred_source_mode"], review_family_map),
        "by_preferred_page_key": build_review_family_group_entries(groups["by_preferred_page_key"], review_family_map),
        "by_debug_page_key": build_review_family_group_entries(groups["by_debug_page_key"], review_family_map),
    }
    return groups


def build_compact_review_case_ref(case: dict[str, object]) -> dict[str, object]:
    review = case.get("review")
    review_payload = review if isinstance(review, dict) else {}
    return {
        "id": str(case.get("id", "")),
        "title": str(case.get("title", "")),
        "status": str(case.get("review_status", review_payload.get("status", ""))),
        "page_bucket": str(case.get("review_page_bucket", review_payload.get("page_bucket", ""))),
        "is_actionable": bool(review_payload.get("is_actionable", False)),
        "family_key": str(case.get("review_family_key", review_payload.get("family_key", ""))),
        "skip_reason": str(case.get("review_skip_reason", review_payload.get("skip_reason", ""))),
        "skip_classification": str(case.get("review_skip_classification", review_payload.get("skip_classification", ""))),
        "skip_is_unresolved": bool(case.get("review_skip_is_unresolved", review_payload.get("skip_is_unresolved", False))),
        "skip_covering_case_ids": [
            str(item)
            for item in case.get("review_skip_covering_case_ids", review_payload.get("skip_covering_case_ids", []))
            if str(item)
        ],
        "primary_visual_target": dict(case.get("primary_visual_target", review_payload.get("primary_visual_target", {}))),
    }


def attach_review_companion_refs(case_results: list[dict[str, object]]) -> None:
    case_map = {str(case.get("id", "")): case for case in case_results if str(case.get("id", ""))}
    for case in case_results:
        related_actionable_case_ids = [
            str(item)
            for item in case.get("review_related_actionable_case_ids", [])
            if str(item)
        ]
        related_skipped_case_ids = [
            str(item)
            for item in case.get("review_related_skipped_case_ids", [])
            if str(item)
        ]
        related_actionable_cases = [
            build_compact_review_case_ref(case_map[case_id])
            for case_id in related_actionable_case_ids
            if case_id in case_map
        ]
        related_skipped_cases = [
            build_compact_review_case_ref(case_map[case_id])
            for case_id in related_skipped_case_ids
            if case_id in case_map
        ]
        case["review_related_actionable_cases"] = related_actionable_cases
        case["review_related_skipped_cases"] = related_skipped_cases
        review = case.get("review")
        if isinstance(review, dict):
            review["related_actionable_cases"] = [dict(item) for item in related_actionable_cases]
            review["related_skipped_cases"] = [dict(item) for item in related_skipped_cases]


def classify_review_skip_case(case: dict[str, object]) -> dict[str, object]:
    review = case.get("review")
    review_payload = review if isinstance(review, dict) else {}
    review_status = str(case.get("review_status", review_payload.get("status", ""))).strip().lower()
    skip_reason = str(case.get("review_skip_reason", review_payload.get("skip_reason", ""))).strip()
    related_actionable_case_ids = [
        str(item)
        for item in case.get("review_related_actionable_case_ids", review_payload.get("related_actionable_case_ids", []))
        if str(item)
    ]
    related_actionable_cases = [
        dict(item)
        for item in case.get("review_related_actionable_cases", review_payload.get("related_actionable_cases", []))
        if isinstance(item, dict)
    ]

    if review_status != "skipped":
        return {
            "classification": "",
            "label": "",
            "is_unresolved": False,
            "covering_case_ids": [],
            "covering_cases": [],
            "debug_included": False,
        }

    if related_actionable_case_ids:
        return {
            "classification": "companion_covered",
            "label": "covered by related actionable companion cases",
            "is_unresolved": False,
            "covering_case_ids": related_actionable_case_ids,
            "covering_cases": related_actionable_cases,
            "debug_included": False,
        }
    if "canonical parity already covers the behavior" in skip_reason:
        return {
            "classification": "canonical_only",
            "label": "covered by canonical parity only",
            "is_unresolved": False,
            "covering_case_ids": [],
            "covering_cases": [],
            "debug_included": False,
        }
    if DEFAULT_BROWSER_REFERENCE_COVERAGE_PHRASE in skip_reason:
        return {
            "classification": "default_reference_covered",
            "label": "covered by the default browser reference engine",
            "is_unresolved": False,
            "covering_case_ids": [],
            "covering_cases": [],
            "debug_included": False,
        }
    return {
        "classification": "reference_divergence",
        "label": "reference divergence requires manual follow-up",
        "is_unresolved": True,
        "covering_case_ids": [],
        "covering_cases": [],
        "debug_included": True,
    }


def attach_review_skip_classification(case_results: list[dict[str, object]]) -> None:
    for case in case_results:
        classification = classify_review_skip_case(case)
        case["review_skip_classification"] = classification["classification"]
        case["review_skip_label"] = classification["label"]
        case["review_skip_is_unresolved"] = classification["is_unresolved"]
        case["review_skip_covering_case_ids"] = list(classification["covering_case_ids"])
        case["review_skip_covering_cases"] = [dict(item) for item in classification["covering_cases"]]
        case["review_skip_debug_included"] = classification["debug_included"]
        review = case.get("review")
        if isinstance(review, dict):
            review["skip_classification"] = classification["classification"]
            review["skip_label"] = classification["label"]
            review["skip_is_unresolved"] = classification["is_unresolved"]
            review["skip_covering_case_ids"] = list(classification["covering_case_ids"])
            review["skip_covering_cases"] = [dict(item) for item in classification["covering_cases"]]
            review["skip_debug_included"] = classification["debug_included"]


def attach_review_skip_refs(review_skips: list[dict[str, object]], case_results: list[dict[str, object]]) -> None:
    case_map = {str(case.get("id", "")): case for case in case_results if str(case.get("id", ""))}
    for review_skip in review_skips:
        case = case_map.get(str(review_skip.get("id", "")))
        if case is None:
            continue
        review_payload = case.get("review")
        review_skip["classification"] = str(case.get("review_skip_classification", ""))
        review_skip["label"] = str(case.get("review_skip_label", ""))
        review_skip["is_unresolved"] = bool(case.get("review_skip_is_unresolved", False))
        review_skip["covering_case_ids"] = [
            str(item)
            for item in case.get("review_skip_covering_case_ids", [])
            if str(item)
        ]
        review_skip["covering_cases"] = [
            dict(item)
            for item in case.get("review_skip_covering_cases", [])
            if isinstance(item, dict)
        ]
        if isinstance(review_payload, dict):
            review_skip["skip_reason"] = str(review_payload.get("skip_reason", review_skip.get("reason", "")))


def attach_primary_visual_targets(case_results: list[dict[str, object]], artifacts: dict[str, object]) -> None:
    page_index = build_artifact_page_index(artifacts)
    page_index_by_path = page_index["by_path"]

    for case in case_results:
        review = case.get("review")
        review_payload = review if isinstance(review, dict) else {}
        review_status = str(case.get("review_status", review_payload.get("status", ""))).strip().lower()
        default_overview_path = str(case.get("default_overview_path", review_payload.get("default_overview_path", "")))
        default_overview_slot_index = int(case.get("default_overview_slot_index", review_payload.get("default_overview_slot_index", 0)))
        default_overview_target = build_page_slot_target(
            "default_overview_slot",
            default_overview_path,
            0,
            default_overview_slot_index,
            "primary_user_overview",
            "actionable",
            page_index_by_path,
        )
        all_overview_path = str(case.get("all_overview_path", review_payload.get("all_overview_path", "")))
        all_overview_slot_index = int(case.get("all_overview_slot_index", review_payload.get("all_overview_slot_index", 0)))
        all_overview_target = build_page_slot_target(
            "all_overview_slot",
            all_overview_path,
            0,
            all_overview_slot_index,
            "all_case_overview",
            "all_cases",
            page_index_by_path,
        )
        review_page_path = str(case.get("review_page_path", review_payload.get("page_path", "")))
        review_page_index = int(case.get("review_page_index", review_payload.get("page_index", 0)))
        review_page_slot_index = int(case.get("review_page_slot_index", review_payload.get("page_slot_index", 0)))
        actionable_review_target: dict[str, object] = {}
        skipped_review_target: dict[str, object] = {}
        if review_status == "skipped":
            skipped_review_target = build_page_slot_target(
                "review_skipped_page_slot",
                review_page_path,
                review_page_index,
                review_page_slot_index,
                "review_skipped_page",
                "skipped",
                page_index_by_path,
            )
        else:
            actionable_review_target = build_page_slot_target(
                "review_page_slot",
                review_page_path,
                review_page_index,
                review_page_slot_index,
                "review_page",
                "actionable",
                page_index_by_path,
            )
        primary_target: dict[str, object]
        if review_status == "skipped":
            primary_target = dict(all_overview_target)
            if not primary_target:
                primary_target = dict(skipped_review_target)
        else:
            primary_target = dict(actionable_review_target)
            if not primary_target:
                primary_target = dict(default_overview_target)
        visual_targets = {
            "primary": dict(primary_target),
            "default_overview": dict(default_overview_target),
            "all_overview": dict(all_overview_target),
            "review_page": dict(actionable_review_target),
            "review_skipped_page": dict(skipped_review_target),
            "available": dedupe_visual_targets(
                [
                    primary_target,
                    default_overview_target,
                    all_overview_target,
                    actionable_review_target,
                    skipped_review_target,
                ]
            ),
        }
        case["primary_visual_target"] = primary_target
        case["visual_targets"] = visual_targets
        if isinstance(review, dict):
            review["primary_visual_target"] = dict(primary_target)
            review["visual_targets"] = dict(visual_targets)


def build_review_case_summaries(case_results: list[dict[str, object]], artifacts: dict[str, object]) -> list[dict[str, object]]:
    page_index = build_artifact_page_index(artifacts, case_results)
    page_index_by_path = page_index["by_path"]
    review_cases: list[dict[str, object]] = []
    for case in case_results:
        review = case.get("review")
        if not isinstance(review, dict):
            continue
        page_path = str(case.get("review_page_path", review.get("page_path", "")))
        page_record = page_index_by_path.get(page_path, {})
        review_cases.append(
            {
                "id": str(case.get("id", "")),
                "title": str(case.get("title", "")),
                "status": str(case.get("review_status", review.get("status", ""))),
                "page_bucket": str(case.get("review_page_bucket", review.get("page_bucket", ""))),
                "page_path": page_path,
                "page_index": int(case.get("review_page_index", review.get("page_index", 0))),
                "page_number": int(page_record.get("page_number", int(case.get("review_page_index", review.get("page_index", 0))) + 1)),
                "page_key": str(page_record.get("page_key", "")),
                "page_label": str(page_record.get("page_label", page_record.get("title", ""))),
                "page_slot_index": int(case.get("review_page_slot_index", review.get("page_slot_index", 0))),
                "page_slot_number": int(case.get("review_page_slot_index", review.get("page_slot_index", 0))) + 1,
                "is_in_default_overview": bool(case.get("is_in_default_overview", review.get("is_in_default_overview", False))),
                "default_overview_path": str(case.get("default_overview_path", review.get("default_overview_path", ""))),
                "default_overview_slot_index": case.get("default_overview_slot_index", review.get("default_overview_slot_index")),
                "all_overview_path": str(case.get("all_overview_path", review.get("all_overview_path", ""))),
                "all_overview_slot_index": case.get("all_overview_slot_index", review.get("all_overview_slot_index")),
                "family_key": str(case.get("review_family_key", review.get("family_key", ""))),
                "family_case_ids": [str(item) for item in case.get("review_family_case_ids", review.get("family_case_ids", []))],
                "related_case_ids": [str(item) for item in case.get("review_related_case_ids", review.get("related_case_ids", []))],
                "related_actionable_case_ids": [
                    str(item)
                    for item in case.get("review_related_actionable_case_ids", review.get("related_actionable_case_ids", []))
                ],
                "related_actionable_cases": [
                    dict(item)
                    for item in case.get("review_related_actionable_cases", review.get("related_actionable_cases", []))
                    if isinstance(item, dict)
                ],
                "related_skipped_case_ids": [
                    str(item)
                    for item in case.get("review_related_skipped_case_ids", review.get("related_skipped_case_ids", []))
                ],
                "related_skipped_cases": [
                    dict(item)
                    for item in case.get("review_related_skipped_cases", review.get("related_skipped_cases", []))
                    if isinstance(item, dict)
                ],
                "primary_visual_target": dict(case.get("primary_visual_target", review.get("primary_visual_target", {}))),
                "visual_targets": dict(case.get("visual_targets", review.get("visual_targets", {}))),
                "is_actionable": bool(review.get("is_actionable", False)),
                "reference": str(review.get("reference", "")),
                "actual": str(review.get("actual", "")),
                "compare": str(review.get("compare", "")),
                "diff": review.get("diff"),
                "override_audit_diff": review.get("override_audit_diff"),
                "has_nondefault_thresholds": bool(case.get("review_has_nondefault_thresholds", review.get("has_nondefault_thresholds", False))),
                "threshold_override_keys": [str(item) for item in case.get("review_threshold_override_keys", review.get("threshold_override_keys", []))],
                "threshold_overrides": dict(case.get("review_threshold_overrides", review.get("threshold_overrides", {}))),
                "would_fail_default_review": bool(review.get("would_fail_default_review", False)),
                "default_failure_reasons": [str(reason) for reason in review.get("default_failure_reasons", [])],
                "skip_reason": str(case.get("review_skip_reason", review.get("skip_reason", ""))),
                "skip_classification": str(case.get("review_skip_classification", review.get("skip_classification", ""))),
                "skip_is_unresolved": bool(case.get("review_skip_is_unresolved", review.get("skip_is_unresolved", False))),
                "skip_covering_case_ids": [
                    str(item)
                    for item in case.get("review_skip_covering_case_ids", review.get("skip_covering_case_ids", []))
                    if str(item)
                ],
                "skip_covering_cases": [
                    dict(item)
                    for item in case.get("review_skip_covering_cases", review.get("skip_covering_cases", []))
                    if isinstance(item, dict)
                ],
                "reasons": [str(reason) for reason in review.get("reasons", [])],
            }
        )
    return review_cases


def build_review_case_index(review_cases: list[dict[str, object]]) -> dict[str, dict[str, object]]:
    review_case_index: dict[str, dict[str, object]] = {}
    for review_case in review_cases:
        case_id = str(review_case.get("id", ""))
        if not case_id:
            continue
        review_case_index[case_id] = dict(review_case)
    return review_case_index


def build_review_case_groups(review_cases: list[dict[str, object]]) -> dict[str, object]:
    groups = {
        "by_status": {},
        "by_actionability": {},
        "by_page_bucket": {},
        "by_entrypoint": {},
        "by_skip_classification": {},
        "by_primary_target_type": {},
        "by_primary_page_key": {},
        "resolved": {},
    }
    for review_case in review_cases:
        case_id = str(review_case.get("id", ""))
        if not case_id:
            continue
        status_key = str(review_case.get("status", ""))
        if status_key:
            groups["by_status"].setdefault(status_key, []).append(case_id)
        actionability_key = "actionable" if bool(review_case.get("is_actionable", False)) else "skipped"
        groups["by_actionability"].setdefault(actionability_key, []).append(case_id)
        skip_classification = str(review_case.get("skip_classification", ""))
        if skip_classification:
            groups["by_skip_classification"].setdefault(skip_classification, []).append(case_id)
        page_bucket = str(review_case.get("page_bucket", ""))
        if page_bucket:
            groups["by_page_bucket"].setdefault(page_bucket, []).append(case_id)
        primary_target = review_case.get("primary_visual_target", {})
        if isinstance(primary_target, dict):
            primary_target_type = str(primary_target.get("target_type", ""))
            primary_page_key = str(primary_target.get("page_key", ""))
            if primary_target_type in ("review_page_slot", "default_overview_slot"):
                groups["by_entrypoint"].setdefault("actionable_review", []).append(case_id)
            elif primary_target_type in ("all_overview_slot", "review_skipped_page_slot"):
                groups["by_entrypoint"].setdefault("skipped_review_debug", []).append(case_id)
            if primary_target_type:
                groups["by_primary_target_type"].setdefault(primary_target_type, []).append(case_id)
            if primary_page_key:
                groups["by_primary_page_key"].setdefault(primary_page_key, []).append(case_id)
    review_case_map = build_review_case_index(review_cases)
    groups["resolved"] = {
        "by_status": build_review_case_group_entries(groups["by_status"], review_case_map),
        "by_actionability": build_review_case_group_entries(groups["by_actionability"], review_case_map),
        "by_page_bucket": build_review_case_group_entries(groups["by_page_bucket"], review_case_map),
        "by_entrypoint": build_review_case_group_entries(groups["by_entrypoint"], review_case_map),
        "by_skip_classification": build_review_case_group_entries(groups["by_skip_classification"], review_case_map),
        "by_primary_target_type": build_review_case_group_entries(groups["by_primary_target_type"], review_case_map),
        "by_primary_page_key": build_review_case_group_entries(groups["by_primary_page_key"], review_case_map),
    }
    return groups


def build_review_case_group_entries(
    case_groups: dict[str, list[str]],
    review_case_map: dict[str, dict[str, object]],
) -> dict[str, dict[str, object]]:
    entries: dict[str, dict[str, object]] = {}
    for group_key, case_ids in case_groups.items():
        normalized_case_ids = [str(case_id) for case_id in case_ids if str(case_id)]
        entries[group_key] = {
            "case_ids": normalized_case_ids,
            "case_count": len(normalized_case_ids),
        }
    return entries


def resolve_review_case_ids(review_case_groups: dict[str, object], group_key: str) -> list[str]:
    entrypoint_groups = review_case_groups.get("by_entrypoint", {})
    if not isinstance(entrypoint_groups, dict):
        return []
    case_ids = entrypoint_groups.get(group_key, [])
    if not isinstance(case_ids, list):
        return []
    return [str(case_id) for case_id in case_ids]


def build_entrypoint_review_case_ref(review_case: dict[str, object]) -> dict[str, object]:
    return {
        "id": str(review_case.get("id", "")),
        "title": str(review_case.get("title", "")),
        "status": str(review_case.get("status", "")),
        "page_bucket": str(review_case.get("page_bucket", "")),
        "is_actionable": bool(review_case.get("is_actionable", False)),
        "family_key": str(review_case.get("family_key", "")),
        "skip_reason": str(review_case.get("skip_reason", "")),
        "skip_classification": str(review_case.get("skip_classification", "")),
        "skip_is_unresolved": bool(review_case.get("skip_is_unresolved", False)),
        "skip_covering_case_ids": [str(item) for item in review_case.get("skip_covering_case_ids", []) if str(item)],
        "primary_visual_target": dict(review_case.get("primary_visual_target", {})),
        "related_actionable_cases": [
            dict(item) for item in review_case.get("related_actionable_cases", []) if isinstance(item, dict)
        ],
        "related_skipped_cases": [
            dict(item) for item in review_case.get("related_skipped_cases", []) if isinstance(item, dict)
        ],
    }


def resolve_review_case_refs(review_case_map: dict[str, dict[str, object]], case_ids: list[str]) -> list[dict[str, object]]:
    return [
        build_entrypoint_review_case_ref(review_case_map[case_id])
        for case_id in case_ids
        if case_id in review_case_map
    ]


def build_entrypoint_review_family_ref(review_family: dict[str, object]) -> dict[str, object]:
    return {
        "family_key": str(review_family.get("family_key", "")),
        "case_ids": [str(item) for item in review_family.get("case_ids", [])],
        "actionable_case_ids": [str(item) for item in review_family.get("actionable_case_ids", [])],
        "skipped_case_ids": [str(item) for item in review_family.get("skipped_case_ids", [])],
        "case_count": int(review_family.get("case_count", 0)),
        "actionable_case_count": int(review_family.get("actionable_case_count", 0)),
        "skipped_case_count": int(review_family.get("skipped_case_count", 0)),
        "primary_page_keys": [str(item) for item in review_family.get("primary_page_keys", [])],
        "available_page_keys": [str(item) for item in review_family.get("available_page_keys", [])],
    }


def resolve_review_family_keys(review_case_map: dict[str, dict[str, object]], case_ids: list[str]) -> list[str]:
    family_keys: list[str] = []
    seen_family_keys: set[str] = set()
    for case_id in case_ids:
        review_case = review_case_map.get(case_id)
        if not isinstance(review_case, dict):
            continue
        family_key = str(review_case.get("family_key", ""))
        if not family_key or family_key in seen_family_keys:
            continue
        seen_family_keys.add(family_key)
        family_keys.append(family_key)
    return family_keys


def resolve_review_family_refs(
    review_family_map: dict[str, dict[str, object]],
    family_keys: list[str],
) -> list[dict[str, object]]:
    return [
        build_entrypoint_review_family_ref(review_family_map[family_key])
        for family_key in family_keys
        if family_key in review_family_map
    ]


def build_artifact_entrypoints(
    page_index: dict[str, object],
    page_groups: dict[str, object],
    review_case_groups: dict[str, object],
    review_case_map: dict[str, dict[str, object]],
    review_family_map: dict[str, dict[str, object]],
    artifacts: dict[str, object],
    report_path: Path,
    summary_path: Path,
) -> dict[str, dict[str, object]]:
    overview_summary = artifacts.get("overview_summary", {})
    review_page_summary = artifacts.get("review_page_summary", {})
    review_runtime = artifacts.get("review_runtime", {})
    page_index_by_key = page_index["by_key"]
    role_groups = page_groups.get("by_role", {})
    primary_overview_keys = list(role_groups.get("primary_user_overview", []))
    all_overview_keys = list(role_groups.get("all_case_overview", []))
    review_page_keys = list(role_groups.get("review_page", []))
    skipped_review_page_keys = list(role_groups.get("review_skipped_page", []))
    actionable_case_ids = resolve_review_case_ids(review_case_groups, "actionable_review")
    skipped_case_ids = resolve_review_case_ids(review_case_groups, "skipped_review_debug")
    actionable_family_keys = resolve_review_family_keys(review_case_map, actionable_case_ids)
    skipped_family_keys = resolve_review_family_keys(review_case_map, skipped_case_ids)
    all_family_keys = dedupe_strings(actionable_family_keys + skipped_family_keys)
    return {
        "primary_user_review": {
            "overview_image": str(artifacts.get("overview_image", "")),
            "overview_page": page_index_by_key.get(build_page_key("primary_user_overview", 0)),
            "overview_page_key": primary_overview_keys[0] if primary_overview_keys else "",
            "review_pages_dir": str(artifacts.get("review_pages_dir", "")),
            "first_review_page": page_index_by_key.get(build_page_key("review_page", 0)),
            "review_page_keys": review_page_keys,
            "review_pages": resolve_page_refs(page_index_by_key, review_page_keys),
            "case_ids": actionable_case_ids,
            "cases": resolve_review_case_refs(review_case_map, actionable_case_ids),
            "family_keys": actionable_family_keys,
            "families": resolve_review_family_refs(review_family_map, actionable_family_keys),
            "overview_mode": str(artifacts.get("overview_mode", "")),
            "case_count": int(overview_summary.get("default_case_count", 0)),
            "family_count": len(actionable_family_keys),
            "review_page_count": int(review_page_summary.get("actionable_page_count", 0)),
        },
        "actionable_review": {
            "overview_image": str(artifacts.get("overview_image", "")),
            "overview_page": page_index_by_key.get(build_page_key("primary_user_overview", 0)),
            "overview_page_key": primary_overview_keys[0] if primary_overview_keys else "",
            "review_pages_dir": str(artifacts.get("review_pages_dir", "")),
            "first_review_page": page_index_by_key.get(build_page_key("review_page", 0)),
            "review_page_keys": review_page_keys,
            "review_pages": resolve_page_refs(page_index_by_key, review_page_keys),
            "case_ids": actionable_case_ids,
            "cases": resolve_review_case_refs(review_case_map, actionable_case_ids),
            "family_keys": actionable_family_keys,
            "families": resolve_review_family_refs(review_family_map, actionable_family_keys),
            "case_count": len(actionable_case_ids),
            "family_count": len(actionable_family_keys),
            "primary_target_types": ["review_page_slot", "default_overview_slot"],
        },
        "skipped_review_debug": {
            "overview_image": str(artifacts.get("overview_all_image", "")),
            "overview_page": page_index_by_key.get(build_page_key("all_case_overview", 0)),
            "overview_page_key": all_overview_keys[0] if all_overview_keys else "",
            "review_skipped_pages_dir": str(artifacts.get("review_skipped_pages_dir", "")),
            "first_skipped_review_page": page_index_by_key.get(build_page_key("review_skipped_page", 0)),
            "skipped_review_page_keys": skipped_review_page_keys,
            "skipped_review_pages": resolve_page_refs(page_index_by_key, skipped_review_page_keys),
            "case_ids": skipped_case_ids,
            "cases": resolve_review_case_refs(review_case_map, skipped_case_ids),
            "family_keys": skipped_family_keys,
            "families": resolve_review_family_refs(review_family_map, skipped_family_keys),
            "case_count": len(skipped_case_ids),
            "family_count": len(skipped_family_keys),
            "primary_target_types": ["all_overview_slot", "review_skipped_page_slot"],
        },
        "all_case_debug": {
            "overview_image": str(artifacts.get("overview_all_image", "")),
            "overview_page": page_index_by_key.get(build_page_key("all_case_overview", 0)),
            "overview_page_key": all_overview_keys[0] if all_overview_keys else "",
            "review_pages_dir": str(artifacts.get("review_pages_dir", "")),
            "first_review_page": page_index_by_key.get(build_page_key("review_page", 0)),
            "review_page_keys": review_page_keys,
            "review_pages": resolve_page_refs(page_index_by_key, review_page_keys),
            "actionable_case_ids": actionable_case_ids,
            "actionable_cases": resolve_review_case_refs(review_case_map, actionable_case_ids),
            "actionable_family_keys": actionable_family_keys,
            "actionable_families": resolve_review_family_refs(review_family_map, actionable_family_keys),
            "review_skipped_pages_dir": str(artifacts.get("review_skipped_pages_dir", "")),
            "first_skipped_review_page": page_index_by_key.get(build_page_key("review_skipped_page", 0)),
            "skipped_review_page_keys": skipped_review_page_keys,
            "skipped_review_pages": resolve_page_refs(page_index_by_key, skipped_review_page_keys),
            "skipped_case_ids": skipped_case_ids,
            "skipped_cases": resolve_review_case_refs(review_case_map, skipped_case_ids),
            "skipped_family_keys": skipped_family_keys,
            "skipped_families": resolve_review_family_refs(review_family_map, skipped_family_keys),
            "family_keys": all_family_keys,
            "families": resolve_review_family_refs(review_family_map, all_family_keys),
            "case_count": int(overview_summary.get("all_case_count", 0)),
            "family_count": len(all_family_keys),
            "skipped_case_count": int(review_page_summary.get("skipped_case_count", 0)),
            "skipped_page_count": int(review_page_summary.get("skipped_page_count", 0)),
        },
        "structured_reports": {
            "report": str(report_path.relative_to(ROOT_DIR)),
            "artifact_summary": str(summary_path.relative_to(ROOT_DIR)),
            "review_mode": str(review_runtime.get("mode", "")),
            "review_status": str(review_runtime.get("status", "")),
        },
    }


def write_artifact_summary(
    runtime_dir: Path,
    reference_engine: str,
    reference_detail: str,
    report_path: Path,
    artifacts: dict[str, object],
    review_summary: dict[str, object],
    case_results: list[dict[str, object]],
    with_unit: bool,
    unit_message: str | None,
) -> Path:
    summary_path = runtime_dir / "svg_validation_artifacts_summary.json"
    page_index = build_artifact_page_index(artifacts, case_results)
    page_groups = build_artifact_page_groups(page_index)
    review_cases = build_review_case_summaries(case_results, artifacts)
    review_case_map = build_review_case_index(review_cases)
    review_case_groups = build_review_case_groups(review_cases)
    review_family_map = build_review_family_map(case_results, page_index)
    review_families = build_review_family_list(case_results, review_family_map)
    payload = {
        "app": APP_NAME,
        "reference_engine": reference_engine,
        "reference_detail": reference_detail,
        "report": str(report_path.relative_to(ROOT_DIR)),
        "artifacts": artifacts,
        "review_summary": review_summary,
        "pages": page_index["pages"],
        "page_map": {
            "by_path": page_index["by_path"],
            "by_key": page_index["by_key"],
        },
        "page_groups": page_groups,
        "review_cases": review_cases,
        "review_case_map": review_case_map,
        "review_case_groups": review_case_groups,
        "review_families": review_families,
        "review_family_map": review_family_map,
        "review_family_groups": build_review_family_groups(review_families),
        "entrypoints": build_artifact_entrypoints(
            page_index,
            page_groups,
            review_case_groups,
            review_case_map,
            review_family_map,
            artifacts,
            report_path,
            summary_path,
        ),
        "with_unit": with_unit,
        "unit_message": unit_message,
    }
    summary_path.write_text(json.dumps(payload, indent=2, ensure_ascii=False) + "\n", encoding="utf-8", newline="\n")
    return summary_path


def print_validation_artifacts(report_path: Path, artifacts: dict[str, object]) -> None:
    print("Report: %s" % report_path.relative_to(ROOT_DIR))
    if artifacts.get("artifact_summary"):
        print("Artifact Summary: %s" % artifacts["artifact_summary"])
    print("Overview: %s" % artifacts["overview_image"])
    if artifacts.get("overview_all_image"):
        print("All Cases Overview: %s" % artifacts["overview_all_image"])
    print("Review Pages: %s" % artifacts["review_pages_dir"])
    if artifacts.get("review_skipped_pages_dir"):
        print("Skipped Review Pages: %s" % artifacts["review_skipped_pages_dir"])


def resolve_edge_path(explicit_path: str | None) -> Path:
    if explicit_path:
        candidate = Path(explicit_path)
        if candidate.exists():
            return candidate
        raise RuntimeError("edge path does not exist: %s" % candidate)

    for candidate in EDGE_CANDIDATES:
        if candidate.exists():
            return candidate

    raise RuntimeError("could not find Edge/Chrome executable for reference SVG rendering")


def resolve_reference_backend(requested_engine: str, explicit_edge_path: str | None) -> tuple[str, str]:
    if requested_engine == REFERENCE_ENGINE_AUTO:
        try:
            edge_path = resolve_edge_path(explicit_edge_path)
        except RuntimeError:
            edge_path = None
        if edge_path is not None and probe_edge_large_viewport_capture(edge_path):
            return REFERENCE_ENGINE_EDGE, str(edge_path)
        resvg_path = resvg_tool.find_resvg_binary(ROOT_DIR)
        if resvg_path is not None:
            if edge_path is not None:
                return REFERENCE_ENGINE_RESVG, "auto fallback: Edge/Chrome large-viewport SVG capture unavailable"
            return REFERENCE_ENGINE_RESVG, str(resvg_path)
        if edge_path is not None:
            raise RuntimeError(
                "Edge/Chrome is available but cannot capture large-viewport SVG references, and "
                f"{describe_resvg_unavailable()}"
            )
        raise RuntimeError(f"could not resolve reference renderer: neither Edge/Chrome nor {describe_resvg_unavailable()}")

    if requested_engine == REFERENCE_ENGINE_RESVG:
        resvg_path = resvg_tool.find_resvg_binary(ROOT_DIR)
        if resvg_path is None:
            raise RuntimeError(describe_resvg_unavailable())
        return REFERENCE_ENGINE_RESVG, str(resvg_path)

    edge_path = resolve_edge_path(explicit_edge_path)
    if not probe_edge_large_viewport_capture(edge_path):
        raise RuntimeError("Edge/Chrome cannot capture large-viewport SVG references in this environment")
    return REFERENCE_ENGINE_EDGE, str(edge_path)


def resolve_runtime_build_variant(bits64: bool, output_subdir: str, user_cflags: str = "") -> tuple[Path, str | None]:
    build_output_dir = runtime_check.get_runtime_build_output_dir(APP_NAME, bits64=bits64, user_cflags=user_cflags)
    if output_subdir == OUTPUT_SUBDIR and not user_cflags.strip():
        return build_output_dir, None

    variant_key = json.dumps(
        {
            "output_subdir": output_subdir,
            "user_cflags": runtime_check.normalize_user_cflags(user_cflags),
        },
        sort_keys=True,
        separators=(",", ":"),
    )
    variant_hash = hashlib.sha1(variant_key.encode("utf-8")).hexdigest()[:8]
    variant_build_output_dir = build_output_dir.parent / ("%s_%s" % (build_output_dir.name, variant_hash))
    variant_obj_suffix = "%s_%s" % (
        runtime_check.get_runtime_obj_suffix(APP_NAME, bits64=bits64, user_cflags=user_cflags),
        variant_hash,
    )
    return variant_build_output_dir, variant_obj_suffix


def compute_recording_duration_seconds(
    case_count: int,
    recording_wait_ms: int = SVG_SPEC_RECORDING_WAIT_MS,
) -> int:
    # Full-page 320x320 SVG cases are limited by real render/snapshot completion,
    # not only by the scripted WAIT interval. Keep a conservative per-case budget
    # so large parity sweeps do not terminate early before the tail cases render.
    per_case_budget_ms = max(int(recording_wait_ms), SVG_SPEC_RECORDING_CASE_BUDGET_MS)
    total_duration_ms = (max(1, case_count) * per_case_budget_ms) + SVG_SPEC_RECORDING_OVERHEAD_MS
    return max(runtime_check.RECORDING_DURATION, int(math.ceil(total_duration_ms / 1000.0)))


def capture_runtime_cases(
    bits64: bool,
    timeout: int,
    jobs: int,
    output_subdir: str,
    case_count: int,
    user_cflags: str = "",
    recording_wait_ms: int = SVG_SPEC_RECORDING_WAIT_MS,
) -> Path:
    build_output_dir, app_obj_suffix = resolve_runtime_build_variant(bits64, output_subdir, user_cflags)
    recording_duration = compute_recording_duration_seconds(case_count, recording_wait_ms=recording_wait_ms)
    if not runtime_check.compile_app(
        APP_NAME,
        bits64=bits64,
        user_cflags=user_cflags,
        build_output_dir=build_output_dir,
        make_jobs=resolve_make_jobs(jobs),
        app_obj_suffix=app_obj_suffix,
    ):
        raise RuntimeError("failed to compile %s" % APP_NAME)

    ok, message = runtime_check.run_app(
        APP_NAME,
        output_subdir,
        timeout=max(recording_duration + 5, timeout),
        duration=recording_duration,
        speed=runtime_check.RECORDING_SPEED,
        snapshot_settle_ms=runtime_check.RECORDING_SNAPSHOT_SETTLE_MS,
        clock_scale=runtime_check.RECORDING_CLOCK_SCALE,
        snapshot_stable_cycles=runtime_check.RECORDING_SNAPSHOT_STABLE_CYCLES,
        snapshot_max_wait_ms=runtime_check.RECORDING_SNAPSHOT_MAX_WAIT_MS,
        build_output_dir=build_output_dir,
    )
    if not ok:
        raise RuntimeError(message)

    print("[PASS] runtime capture: %s" % message)
    return ROOT_DIR / runtime_check.SCREENSHOT_DIR / APP_NAME / output_subdir


def load_runtime_case_frames(cases: list[dict[str, object]], runtime_dir: Path) -> tuple[dict[str, Path], list[str]]:
    manifest_path = runtime_dir / runtime_check.FRAME_LABEL_MANIFEST
    if not manifest_path.exists():
        raise RuntimeError("%s did not produce %s" % (APP_NAME, runtime_check.FRAME_LABEL_MANIFEST))

    payload = json.loads(manifest_path.read_text(encoding="utf-8"))
    entries = payload.get("entries", [])
    if not isinstance(entries, list) or not entries:
        raise RuntimeError("%s produced an empty frame label manifest" % APP_NAME)

    expected_ids = [str(case["id"]) for case in cases]
    expected_id_set = set(expected_ids)
    resolved_frames: dict[str, Path] = {}
    first_seen_order: list[str] = []

    for entry in entries:
        frame_name = str(entry.get("frame", "")).strip()
        label = str(entry.get("label", "")).strip()
        if label not in expected_id_set or label in resolved_frames:
            continue
        frame_path = runtime_dir / frame_name
        if not frame_path.exists():
            raise RuntimeError("%s labeled a missing frame: %s" % (APP_NAME, frame_name))
        resolved_frames[label] = frame_path
        first_seen_order.append(label)

    missing = [case_id for case_id in expected_ids if case_id not in resolved_frames]
    if missing:
        raise RuntimeError("%s missing labeled runtime frames: %s" % (APP_NAME, ", ".join(missing)))
    if first_seen_order != expected_ids:
        raise RuntimeError(
            "%s runtime frame order mismatch: expected %s got %s"
            % (APP_NAME, ", ".join(expected_ids), ", ".join(first_seen_order))
        )

    return resolved_frames, first_seen_order


def capture_fixed_viewport_case_results(
    cases: list[dict[str, object]],
    runtime_dir: Path,
    runtime_frames: dict[str, Path],
    reference_engine: str,
    reference_detail: str,
    hot_pixel_delta: int,
    output_subdir: str,
) -> tuple[list[dict[str, object]], list[dict[str, object]], list[dict[str, object]], dict[str, object]]:
    reference_dir = runtime_dir / "reference"
    compare_dir = runtime_dir / "compare"
    shutil.rmtree(reference_dir, ignore_errors=True)
    shutil.rmtree(compare_dir, ignore_errors=True)
    reference_dir.mkdir(parents=True, exist_ok=True)
    compare_dir.mkdir(parents=True, exist_ok=True)

    case_results: list[dict[str, object]] = []
    parity_failures: list[dict[str, object]] = []
    parity_skips: list[dict[str, object]] = []

    for case in cases:
        case_id = str(case["id"])
        review_skip_reason = ""
        review_page_bucket = "actionable"
        review_status = "passed"
        review_reasons: list[str] = []
        diff_path: str | None = None
        audit_diff_path: str | None = None

        try:
            reference_path = render_reference_image(
                case,
                reference_engine,
                reference_detail,
                reference_dir,
            )
        except RuntimeError as exc:
            raise RuntimeError("large viewport reference render failed for %s: %s" % (case_id, exc)) from exc

        crop_box = None
        metrics = compute_metrics(reference_path, runtime_frames[case_id], hot_pixel_delta, crop_box=crop_box)
        geometry = compute_review_geometry_metrics(reference_path, runtime_frames[case_id], crop_box=crop_box)
        compare_path = save_side_by_side(case_id, reference_path, runtime_frames[case_id], compare_dir, crop_box=crop_box)
        active_thresholds = build_case_review_threshold_payload(case, hot_pixel_delta)
        default_thresholds = build_default_review_threshold_payload(hot_pixel_delta)
        default_failure_reasons = evaluate_review_metrics_against_thresholds(metrics, geometry, default_thresholds)

        if bool(case.get("review_has_nondefault_thresholds", False)):
            audit_diff_path = str(
                save_diff(case_id, reference_path, runtime_frames[case_id], compare_dir, crop_box=crop_box).relative_to(ROOT_DIR)
            )

        if bool(case.get("review_skip", False)):
            review_status = "skipped"
            review_page_bucket = "skipped"
            review_skip_reason = str(case.get("review_skip_reason", "")).strip()
            parity_skips.append(
                {
                    "id": case_id,
                    "title": str(case["title"]),
                    "reason": review_skip_reason,
                    "compare": str(compare_path.relative_to(ROOT_DIR)),
                }
            )
        else:
            passed, review_reasons = evaluate_review_metrics(case, metrics, geometry)
            if not passed:
                review_status = "failed"
                if audit_diff_path is None:
                    diff_path = str(
                        save_diff(case_id, reference_path, runtime_frames[case_id], compare_dir, crop_box=crop_box).relative_to(ROOT_DIR)
                    )
                else:
                    diff_path = audit_diff_path
                parity_failures.append(
                    {
                        "id": case_id,
                        "title": str(case["title"]),
                        "metrics": asdict(metrics),
                        "thresholds": dict(active_thresholds),
                        "default_thresholds": dict(default_thresholds),
                        "geometry": asdict(geometry),
                        "has_nondefault_thresholds": bool(case.get("review_has_nondefault_thresholds", False)),
                        "threshold_override_keys": list(case.get("review_threshold_override_keys", [])),
                        "threshold_overrides": dict(case.get("review_threshold_overrides", {})),
                        "would_fail_default_review": bool(default_failure_reasons),
                        "default_failure_reasons": list(default_failure_reasons),
                        "reasons": review_reasons,
                        "compare": str(compare_path.relative_to(ROOT_DIR)),
                        "diff": diff_path,
                    }
                )

        parity_result = {
            "reference": str(reference_path.relative_to(ROOT_DIR)),
            "actual": str(runtime_frames[case_id].relative_to(ROOT_DIR)),
            "render_box": build_case_render_box_payload(case),
            "canvas": {
                "width": REVIEW_CANVAS_WIDTH,
                "height": REVIEW_CANVAS_HEIGHT,
            },
            "status": review_status,
            "page_bucket": review_page_bucket,
            "is_actionable": review_page_bucket == "actionable",
            "metrics": asdict(metrics),
            "thresholds": dict(active_thresholds),
            "default_thresholds": dict(default_thresholds),
            "geometry": asdict(geometry),
            "compare": str(compare_path.relative_to(ROOT_DIR)),
            "diff": diff_path,
            "override_audit_diff": audit_diff_path,
            "has_nondefault_thresholds": bool(case.get("review_has_nondefault_thresholds", False)),
            "threshold_override_keys": list(case.get("review_threshold_override_keys", [])),
            "threshold_overrides": dict(case.get("review_threshold_overrides", {})),
            "would_fail_default_review": bool(default_failure_reasons),
            "default_failure_reasons": list(default_failure_reasons),
            "reasons": review_reasons if review_status != "skipped" else [review_skip_reason],
            "skip_reason": review_skip_reason,
        }

        status = "passed" if review_status == "skipped" else review_status
        if review_status == "skipped":
            status_reason = "large viewport parity waived by review skip"
            status_message = format_review_skip_message(
                {
                    "id": case_id,
                    "reason": review_skip_reason,
                    "compare": parity_result["compare"],
                }
            )
        else:
            status_reason = "large viewport parity"
            status_message = format_fixed_viewport_parity_message(case_id, parity_result)

        case_results.append(
            {
                "id": case_id,
                "title": str(case["title"]),
                "features": list(case.get("features", ())),
                "render_size_sensitive": bool(case.get("render_size_sensitive", False)),
                "render_size_reason": str(case.get("render_size_reason", "")),
                "shrink_locked": bool(case.get("shrink_locked", False)),
                "shrink_lock_reason": str(case.get("shrink_lock_reason", "")),
                "uses_default_fuzzy": bool(case.get("uses_default_fuzzy", True)),
                "fuzzy_override_reason": str(case.get("fuzzy_override_reason", "")),
                "trial_render_box_active": bool(case.get("trial_render_box_active", False)),
                "render_box_source": "trial" if bool(case.get("trial_render_box_active", False)) else "large_viewport",
                "canonical_status": status,
                "canonical_message": status_message,
                "status": status,
                "status_reason": status_reason,
                "status_message": status_message,
                "reference": str(parity_result["reference"]),
                "actual": str(parity_result["actual"]),
                "render_box": build_case_render_box_payload(case),
                "manifest_render_box": build_manifest_render_box_payload(case),
                "metrics": asdict(metrics),
                "geometry": asdict(geometry),
                "thresholds": dict(active_thresholds),
                "default_thresholds": dict(default_thresholds),
                "compare": str(parity_result["compare"]),
                "diff": diff_path,
                "review_has_nondefault_thresholds": bool(case.get("review_has_nondefault_thresholds", False)),
                "review_threshold_override_keys": list(case.get("review_threshold_override_keys", [])),
                "review_threshold_overrides": dict(case.get("review_threshold_overrides", {})),
                "review": parity_result,
                "review_status": review_status,
                "review_page_bucket": review_page_bucket,
                "review_skip_reason": review_skip_reason,
                "primary_parity_mode": PRIMARY_PARITY_MODE_LARGE_VIEWPORT,
                "primary_parity_label": "large-viewport parity",
            }
        )

    review_runtime = {
        "runtime_dir": str(runtime_dir.relative_to(ROOT_DIR)),
        "output_subdir": output_subdir,
        "canvas": {
            "width": REVIEW_CANVAS_WIDTH,
            "height": REVIEW_CANVAS_HEIGHT,
        },
        "min_edge": REVIEW_MIN_EDGE,
        "margin": REVIEW_MARGIN,
        "compare_dir": str(compare_dir.relative_to(ROOT_DIR)),
        "total_case_count": len(case_results),
        "actionable_count": len(case_results) - len(parity_skips),
        "failure_count": len(parity_failures),
        "skip_count": len(parity_skips),
        "covered_skip_count": 0,
        "unresolved_skip_count": 0,
    }
    return case_results, parity_failures, parity_skips, review_runtime


def decode_process_output(stdout: bytes | str | None, stderr: bytes | str | None) -> str:
    chunks: list[str] = []
    for stream in (stdout, stderr):
        if not stream:
            continue
        if isinstance(stream, bytes):
            chunks.append(stream.decode("utf-8", errors="replace"))
        else:
            chunks.append(stream)
    return "\n".join(chunk for chunk in chunks if chunk).strip()


def run_edge_screenshot(
    edge_path: Path,
    page_url: str,
    output_path: Path,
    user_data_dir: Path,
    capture_width: int,
    capture_height: int,
    timeout_sec: int = EDGE_REFERENCE_TIMEOUT_SEC,
) -> tuple[bool, str]:
    command = [
        str(edge_path),
        "--headless",
        "--disable-gpu",
        "--hide-scrollbars",
        "--run-all-compositor-stages-before-draw",
        "--virtual-time-budget=3000",
        "--no-first-run",
        "--no-default-browser-check",
        "--disable-extensions",
        "--disable-background-networking",
        "--disable-component-update",
        "--user-data-dir=%s" % user_data_dir,
        "--window-size=%d,%d" % (capture_width, capture_height),
        "--screenshot=%s" % output_path,
        page_url,
    ]
    try:
        result = subprocess.run(command, cwd=ROOT_DIR, capture_output=True, text=False, timeout=timeout_sec)
    except subprocess.TimeoutExpired as exc:
        output = decode_process_output(exc.stdout, exc.stderr)
        return False, "timeout after %ss\n%s" % (timeout_sec, output or "<no output>")

    if result.returncode != 0 or not output_path.exists():
        output = decode_process_output(result.stdout, result.stderr)
        return False, output or "<no output>"
    return True, ""


def case_uses_full_page_reference_render(case: dict[str, object]) -> bool:
    return (
        int(case["render_width"]) == REVIEW_CANVAS_WIDTH
        and int(case["render_height"]) == REVIEW_CANVAS_HEIGHT
        and int(case["composite_x"]) == 0
        and int(case["composite_y"]) == 0
    )


def materialize_reference_fixture_svg(case: dict[str, object], reference_dir: Path) -> tuple[Path, str]:
    case_id = str(case["id"])
    svg_text = Path(case["path"]).read_text(encoding="utf-8")
    reference_svg_path = reference_dir / ("%s.svg" % case_id)
    reference_svg_path.write_text(svg_text, encoding="utf-8", newline="\n")
    return reference_svg_path, svg_text


def probe_edge_large_viewport_capture(edge_path: Path) -> bool:
    cache_key = str(edge_path.resolve())
    if cache_key in EDGE_LARGE_VIEWPORT_PROBE_CACHE:
        return EDGE_LARGE_VIEWPORT_PROBE_CACHE[cache_key]

    probe_svg = """<?xml version="1.0" encoding="utf-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="320" height="320" viewBox="0 0 320 320">
  <rect width="320" height="320" fill="#ffffff"/>
  <svg x="0" y="0" width="320" height="320" viewBox="0 0 64 64">
    <rect width="64" height="64" fill="#ffffff"/>
    <path fill="#ffd43b" d="M10 52 Q18 40 26 52 T42 52 T58 52 V62 H10 Z"/>
  </svg>
</svg>
"""
    supported = False
    with tempfile.TemporaryDirectory(prefix="svg_edge_probe_") as probe_dir:
        probe_root = Path(probe_dir)
        probe_svg_path = probe_root / "probe.svg"
        probe_capture_path = probe_root / "probe.png"
        probe_svg_path.write_text(probe_svg, encoding="utf-8", newline="\n")

        with tempfile.TemporaryDirectory(prefix="svg-edge-profile-", dir=str(probe_root)) as profile_dir:
            ok, _ = run_edge_screenshot(
                edge_path,
                probe_svg_path.resolve().as_uri(),
                probe_capture_path,
                Path(profile_dir),
                REVIEW_CANVAS_WIDTH,
                REVIEW_CANVAS_HEIGHT,
            )
        if ok and probe_capture_path.exists():
            captured = Image.open(probe_capture_path).convert("RGB")
            supported = captured.point(lambda value: 0 if value == 255 else 255).getbbox() is not None

    EDGE_LARGE_VIEWPORT_PROBE_CACHE[cache_key] = supported
    return supported


def render_edge_reference(case: dict[str, object], edge_path: Path, reference_dir: Path) -> Path:
    case_id = str(case["id"])
    if not case_uses_full_page_reference_render(case):
        raise RuntimeError("Edge/Chrome reference rendering only supports the default full-page large-viewport box; use resvg for --trial-render-box")

    reference_svg_path, _ = materialize_reference_fixture_svg(case, reference_dir)
    capture_path = reference_dir / ("%s_capture.png" % case_id)
    output_path = reference_dir / ("%s.png" % case_id)

    for path in (capture_path, output_path):
        if path.exists():
            path.unlink()

    with tempfile.TemporaryDirectory(prefix="svg-edge-", dir=str(reference_dir)) as profile_dir:
        user_data_dir = Path(profile_dir)
        ok, output = run_edge_screenshot(
            edge_path,
            reference_svg_path.resolve().as_uri(),
            capture_path,
            user_data_dir,
            REVIEW_CANVAS_WIDTH,
            REVIEW_CANVAS_HEIGHT,
        )
        if not ok:
            raise RuntimeError("failed to render reference for %s\n%s" % (case_id, output))

    captured = Image.open(capture_path).convert("RGB")
    if captured.width < REVIEW_CANVAS_WIDTH or captured.height < REVIEW_CANVAS_HEIGHT:
        raise RuntimeError(
            "failed to render reference for %s\nunexpected edge capture size %dx%d (need at least %dx%d)"
            % (case_id, captured.width, captured.height, REVIEW_CANVAS_WIDTH, REVIEW_CANVAS_HEIGHT)
        )
    captured = captured.crop((0, 0, REVIEW_CANVAS_WIDTH, REVIEW_CANVAS_HEIGHT))
    captured.save(output_path)
    return output_path


def render_resvg_reference(case: dict[str, object], reference_dir: Path) -> Path:
    case_id = str(case["id"])
    reference_svg_path, _ = materialize_reference_fixture_svg(case, reference_dir)
    raw_output_path = reference_dir / ("%s_raw.png" % case_id)
    output_path = reference_dir / ("%s.png" % case_id)
    for path in (raw_output_path, output_path):
        if path.exists():
            path.unlink()

    if case_uses_full_page_reference_render(case):
        resvg_tool.render_svg_to_png(reference_svg_path, raw_output_path, REVIEW_CANVAS_WIDTH, REVIEW_CANVAS_HEIGHT, ROOT_DIR)
        canvas = Image.new("RGBA", (REVIEW_CANVAS_WIDTH, REVIEW_CANVAS_HEIGHT), (255, 255, 255, 255))
        rendered = Image.open(raw_output_path).convert("RGBA")
        canvas.alpha_composite(rendered, (0, 0))
        canvas.convert("RGB").save(output_path)
        return output_path

    resvg_tool.render_svg_to_png(
        reference_svg_path,
        raw_output_path,
        int(case["render_width"]),
        int(case["render_height"]),
        ROOT_DIR,
    )
    canvas = Image.new("RGBA", (REVIEW_CANVAS_WIDTH, REVIEW_CANVAS_HEIGHT), (255, 255, 255, 255))
    rendered = Image.open(raw_output_path).convert("RGBA")
    canvas.alpha_composite(rendered, (int(case["composite_x"]), int(case["composite_y"])))
    canvas.convert("RGB").save(output_path)
    return output_path


def render_reference_image(
    case: dict[str, object],
    reference_engine: str,
    reference_detail: str,
    reference_dir: Path,
) -> Path:
    if reference_engine == REFERENCE_ENGINE_RESVG:
        return render_resvg_reference(case, reference_dir)
    return render_edge_reference(case, Path(reference_detail), reference_dir)


def quantize_rgb565(image: Image.Image) -> Image.Image:
    array = np.array(image.convert("RGB"), dtype=np.uint16)
    # Match the runtime RGB565 -> RGB888 expansion used by the SDL capture path.
    array[..., 0] = (array[..., 0] & 0xF8) | (array[..., 0] >> 5)
    array[..., 1] = (array[..., 1] & 0xFC) | (array[..., 1] >> 6)
    array[..., 2] = (array[..., 2] & 0xF8) | (array[..., 2] >> 5)
    return Image.fromarray(array.astype(np.uint8), mode="RGB")


def normalize_crop_box(crop_box: ImageCropBox | None, width: int, height: int) -> tuple[int, int, int, int] | None:
    if crop_box is None:
        return None

    x0 = int(crop_box.x)
    y0 = int(crop_box.y)
    crop_width = int(crop_box.width)
    crop_height = int(crop_box.height)
    if crop_width <= 0 or crop_height <= 0:
        raise RuntimeError("crop box must have positive dimensions")
    if x0 < 0 or y0 < 0 or x0 + crop_width > width or y0 + crop_height > height:
        raise RuntimeError("crop box exceeds image bounds")
    return x0, y0, x0 + crop_width, y0 + crop_height


def crop_image_array(image: np.ndarray, crop_box: ImageCropBox | None) -> np.ndarray:
    normalized = normalize_crop_box(crop_box, int(image.shape[1]), int(image.shape[0]))
    if normalized is None:
        return image
    x0, y0, x1, y1 = normalized
    return image[y0:y1, x0:x1]


def crop_pil_image(image: Image.Image, crop_box: ImageCropBox | None) -> Image.Image:
    normalized = normalize_crop_box(crop_box, image.width, image.height)
    if normalized is None:
        return image
    return image.crop(normalized)


def build_crop_box_from_payload(payload: dict[str, object] | None) -> ImageCropBox | None:
    if payload is None:
        return None
    return ImageCropBox(
        x=int(payload["x"]),
        y=int(payload["y"]),
        width=int(payload["width"]),
        height=int(payload["height"]),
    )


def load_quantized_image_pair(reference_path: Path, actual_path: Path, crop_box: ImageCropBox | None = None) -> tuple[np.ndarray, np.ndarray]:
    reference_image = np.array(quantize_rgb565(Image.open(reference_path)), dtype=np.uint8)
    actual_image = np.array(quantize_rgb565(Image.open(actual_path)), dtype=np.uint8)
    if reference_image.shape != actual_image.shape:
        raise RuntimeError(
            "image size mismatch for %s vs %s: %s != %s"
            % (reference_path.name, actual_path.name, reference_image.shape, actual_image.shape)
        )

    reference_image = crop_image_array(reference_image, crop_box)
    actual_image = crop_image_array(actual_image, crop_box)
    return reference_image, actual_image


def estimate_corner_background(image: np.ndarray) -> tuple[int, int, int] | None:
    if image.ndim != 3 or image.shape[2] != 3 or image.shape[0] <= 0 or image.shape[1] <= 0:
        return None
    height = int(image.shape[0])
    width = int(image.shape[1])
    corners = np.stack(
        (
            image[0, 0],
            image[0, width - 1],
            image[height - 1, 0],
            image[height - 1, width - 1],
        ),
        axis=0,
    )
    if not np.all(corners == corners[0]):
        return None
    return tuple(int(channel) for channel in corners[0])


def compute_foreground_bbox(image: np.ndarray, background: tuple[int, int, int]) -> tuple[int, int, int, int] | None:
    background_array = np.array(background, dtype=np.int16)
    diff = np.max(np.abs(image.astype(np.int16) - background_array), axis=2)
    foreground_y, foreground_x = np.nonzero(diff > 0)
    if foreground_x.size == 0 or foreground_y.size == 0:
        return None
    return (
        int(np.min(foreground_x)),
        int(np.min(foreground_y)),
        int(np.max(foreground_x)) + 1,
        int(np.max(foreground_y)) + 1,
    )


def bbox_is_full_image(bbox: tuple[int, int, int, int], width: int, height: int) -> bool:
    return bbox[0] == 0 and bbox[1] == 0 and bbox[2] == width and bbox[3] == height


def compute_bbox_edge_delta(reference_bbox: tuple[int, int, int, int], actual_bbox: tuple[int, int, int, int]) -> int:
    return max(abs(int(reference_bbox[index]) - int(actual_bbox[index])) for index in range(4))


def compute_bbox_area_delta_ratio(reference_bbox: tuple[int, int, int, int], actual_bbox: tuple[int, int, int, int]) -> float:
    reference_area = max(0, (int(reference_bbox[2]) - int(reference_bbox[0])) * (int(reference_bbox[3]) - int(reference_bbox[1])))
    actual_area = max(0, (int(actual_bbox[2]) - int(actual_bbox[0])) * (int(actual_bbox[3]) - int(actual_bbox[1])))
    denominator = max(reference_area, actual_area)
    if denominator <= 0:
        return 0.0
    return abs(float(actual_area - reference_area)) / float(denominator)


def compute_review_geometry_metrics(reference_path: Path, actual_path: Path, crop_box: ImageCropBox | None = None) -> ReviewGeometryMetrics:
    reference_image, actual_image = load_quantized_image_pair(reference_path, actual_path, crop_box)
    height = int(reference_image.shape[0])
    width = int(reference_image.shape[1])
    reference_background = estimate_corner_background(reference_image)
    actual_background = estimate_corner_background(actual_image)
    reference_bbox: tuple[int, int, int, int] | None = None
    actual_bbox: tuple[int, int, int, int] | None = None
    if reference_background is None or actual_background is None:
        return ReviewGeometryMetrics(
            available=False,
            reason="corner_background_unavailable",
            reference_background=reference_background,
            actual_background=actual_background,
            reference_bbox=None,
            actual_bbox=None,
            bbox_edge_delta=None,
            bbox_area_delta_ratio=None,
        )

    reference_bbox = compute_foreground_bbox(reference_image, reference_background)
    actual_bbox = compute_foreground_bbox(actual_image, actual_background)
    if reference_bbox is None or actual_bbox is None:
        return ReviewGeometryMetrics(
            available=False,
            reason="foreground_bbox_unavailable",
            reference_background=reference_background,
            actual_background=actual_background,
            reference_bbox=reference_bbox,
            actual_bbox=actual_bbox,
            bbox_edge_delta=None,
            bbox_area_delta_ratio=None,
        )
    if bbox_is_full_image(reference_bbox, width, height) or bbox_is_full_image(actual_bbox, width, height):
        return ReviewGeometryMetrics(
            available=False,
            reason="full_crop_foreground",
            reference_background=reference_background,
            actual_background=actual_background,
            reference_bbox=reference_bbox,
            actual_bbox=actual_bbox,
            bbox_edge_delta=None,
            bbox_area_delta_ratio=None,
        )

    return ReviewGeometryMetrics(
        available=True,
        reason="ok",
        reference_background=reference_background,
        actual_background=actual_background,
        reference_bbox=reference_bbox,
        actual_bbox=actual_bbox,
        bbox_edge_delta=compute_bbox_edge_delta(reference_bbox, actual_bbox),
        bbox_area_delta_ratio=compute_bbox_area_delta_ratio(reference_bbox, actual_bbox),
    )


def compute_metrics(reference_path: Path, actual_path: Path, hot_pixel_delta: int, crop_box: ImageCropBox | None = None) -> FrameMetrics:
    reference_image, actual_image = load_quantized_image_pair(reference_path, actual_path, crop_box)
    reference_image = reference_image.astype(np.float32)
    actual_image = actual_image.astype(np.float32)
    diff = np.abs(reference_image - actual_image)
    per_pixel_max_diff = np.max(diff, axis=2)
    diff_pixels = int(np.count_nonzero(per_pixel_max_diff > 0.0))
    total_pixels = int(per_pixel_max_diff.shape[0] * per_pixel_max_diff.shape[1])
    hot_pixel_ratio = float(np.mean(per_pixel_max_diff > float(hot_pixel_delta)))
    return FrameMetrics(
        max_difference=int(np.max(per_pixel_max_diff)),
        diff_pixels=diff_pixels,
        diff_pixel_ratio=(float(diff_pixels) / float(total_pixels)) if total_pixels else 0.0,
        mean_abs=float(np.mean(diff)),
        rms=float(np.sqrt(np.mean(np.square(diff)))),
        hot_pixel_ratio=hot_pixel_ratio,
    )


def save_side_by_side(label: str, left_path: Path, right_path: Path, output_dir: Path, crop_box: ImageCropBox | None = None) -> Path:
    left = crop_pil_image(quantize_rgb565(Image.open(left_path)), crop_box)
    right = crop_pil_image(quantize_rgb565(Image.open(right_path)), crop_box)
    canvas = Image.new("RGB", (left.width + right.width, max(left.height, right.height)), "white")
    canvas.paste(left, (0, 0))
    canvas.paste(right, (left.width, 0))

    output_path = output_dir / ("compare_%s.png" % label)
    canvas.save(output_path)
    return output_path


def save_diff(label: str, left_path: Path, right_path: Path, output_dir: Path, crop_box: ImageCropBox | None = None) -> Path:
    left = crop_pil_image(quantize_rgb565(Image.open(left_path)), crop_box)
    right = crop_pil_image(quantize_rgb565(Image.open(right_path)), crop_box)
    diff = ImageChops.difference(left, right)
    amplified = diff.point(lambda value: min(255, value * 6))
    output_path = output_dir / ("diff_%s.png" % label)
    amplified.save(output_path)
    return output_path


def build_case_review_threshold_payload(case: dict[str, object], hot_pixel_delta: int) -> dict[str, object]:
    return {
        "max_mean_abs": float(case["review_max_mean_abs"]),
        "max_rms": float(case["review_max_rms"]),
        "max_hot_pixel_ratio": float(case["review_max_hot_pixel_ratio"]),
        "max_bbox_edge_delta": int(case["review_max_bbox_edge_delta"]),
        "max_bbox_area_delta_ratio": float(case["review_max_bbox_area_delta_ratio"]),
        "hot_pixel_delta": int(hot_pixel_delta),
    }


def build_default_review_threshold_payload(hot_pixel_delta: int) -> dict[str, object]:
    return {
        "max_mean_abs": float(DEFAULT_MAX_MEAN_ABS),
        "max_rms": float(DEFAULT_MAX_RMS),
        "max_hot_pixel_ratio": float(DEFAULT_MAX_HOT_PIXEL_RATIO),
        "max_bbox_edge_delta": int(DEFAULT_REVIEW_MAX_BBOX_EDGE_DELTA),
        "max_bbox_area_delta_ratio": float(DEFAULT_REVIEW_MAX_BBOX_AREA_DELTA_RATIO),
        "hot_pixel_delta": int(hot_pixel_delta),
    }


def evaluate_review_metrics_against_thresholds(
    metrics: FrameMetrics,
    geometry: ReviewGeometryMetrics | None,
    thresholds: dict[str, object],
) -> list[str]:
    failures: list[str] = []
    if metrics.mean_abs > float(thresholds["max_mean_abs"]):
        failures.append("mean_abs=%.3f>%.3f" % (metrics.mean_abs, float(thresholds["max_mean_abs"])))
    if metrics.rms > float(thresholds["max_rms"]):
        failures.append("rms=%.3f>%.3f" % (metrics.rms, float(thresholds["max_rms"])))
    if metrics.hot_pixel_ratio > float(thresholds["max_hot_pixel_ratio"]):
        failures.append("hot_ratio=%.5f>%.5f" % (metrics.hot_pixel_ratio, float(thresholds["max_hot_pixel_ratio"])))
    if geometry is not None and geometry.available:
        if geometry.bbox_edge_delta is not None and geometry.bbox_edge_delta > int(thresholds["max_bbox_edge_delta"]):
            failures.append("bbox_edge=%d>%d" % (int(geometry.bbox_edge_delta), int(thresholds["max_bbox_edge_delta"])))
        if geometry.bbox_area_delta_ratio is not None and geometry.bbox_area_delta_ratio > float(thresholds["max_bbox_area_delta_ratio"]):
            failures.append("bbox_area_ratio=%.5f>%.5f" % (geometry.bbox_area_delta_ratio, float(thresholds["max_bbox_area_delta_ratio"])))
    return failures


def evaluate_review_metrics(case: dict[str, object], metrics: FrameMetrics, geometry: ReviewGeometryMetrics | None = None) -> tuple[bool, list[str]]:
    failures = evaluate_review_metrics_against_thresholds(metrics, geometry, build_case_review_threshold_payload(case, DEFAULT_HOT_PIXEL_DELTA))
    return (len(failures) == 0), failures


def format_review_failure_message(review_failure: dict[str, object]) -> str:
    reasons = [str(reason) for reason in review_failure.get("reasons", [])]
    reason_text = ", ".join(reasons) if reasons else "review metrics exceeded thresholds"
    suffix_parts = ["compare=%s" % str(review_failure["compare"])]
    diff_path = review_failure.get("diff")
    if diff_path:
        suffix_parts.append("diff=%s" % str(diff_path))
    return "review %s %s %s" % (
        str(review_failure["id"]),
        reason_text,
        " ".join(suffix_parts),
    )


def format_review_skip_message(review_skip: dict[str, object]) -> str:
    reason = str(review_skip.get("reason", "")).strip()
    if not reason:
        reason = "review skipped"
    classification = str(review_skip.get("classification", "")).strip()
    classification_text = (" [%s]" % classification) if classification else ""
    covering_case_ids = [str(item) for item in review_skip.get("covering_case_ids", []) if str(item)]
    covering_text = ""
    if covering_case_ids:
        covering_text = " covered_by=%s" % ",".join(covering_case_ids)
    return "review %s skipped%s: %s%s compare=%s" % (
        str(review_skip["id"]),
        classification_text,
        reason,
        covering_text,
        str(review_skip["compare"]),
    )


def build_review_skip_breakdown(case_results: list[dict[str, object]]) -> dict[str, dict[str, object]]:
    breakdown = {
        "companion_covered": {
            "case_count": 0,
            "case_ids": [],
        },
        "canonical_only": {
            "case_count": 0,
            "case_ids": [],
        },
        "default_reference_covered": {
            "case_count": 0,
            "case_ids": [],
        },
        "reference_divergence": {
            "case_count": 0,
            "case_ids": [],
        },
        "unclassified": {
            "case_count": 0,
            "case_ids": [],
        },
    }
    for case in case_results:
        review = case.get("review")
        review_payload = review if isinstance(review, dict) else {}
        review_status = str(case.get("review_status", review_payload.get("status", ""))).strip().lower()
        if review_status != "skipped":
            continue
        classification = str(case.get("review_skip_classification", review_payload.get("skip_classification", ""))).strip() or "unclassified"
        entry = breakdown.setdefault(classification, {"case_count": 0, "case_ids": []})
        case_id = str(case.get("id", ""))
        entry["case_count"] += 1
        entry["case_ids"].append(case_id)
    return breakdown


def build_review_summary(
    review_mode: str,
    case_results: list[dict[str, object]],
    review_failures: list[dict[str, object]],
    review_skips: list[dict[str, object]],
) -> dict[str, object]:
    status = "passed" if not review_failures else "failed"

    actionable_cases = [
        str(case.get("id", ""))
        for case in case_results
        if str(case.get("review_page_bucket", "")).strip().lower() == "actionable"
    ]
    override_audit = build_review_threshold_override_audit(case_results)
    skip_breakdown = build_review_skip_breakdown(case_results)
    covered_skip_cases = (
        list(skip_breakdown.get("companion_covered", {}).get("case_ids", [])) +
        list(skip_breakdown.get("canonical_only", {}).get("case_ids", [])) +
        list(skip_breakdown.get("default_reference_covered", {}).get("case_ids", []))
    )
    unresolved_skip_cases = list(skip_breakdown.get("reference_divergence", {}).get("case_ids", []))
    return {
        "mode": review_mode,
        "status": status,
        "actionable_count": len(actionable_cases),
        "actionable_cases": actionable_cases,
        "failure_count": len(review_failures),
        "failed_cases": [str(item["id"]) for item in review_failures],
        "skip_count": len(review_skips),
        "skipped_cases": [str(item["id"]) for item in review_skips],
        "covered_skip_count": len(covered_skip_cases),
        "covered_skip_cases": covered_skip_cases,
        "unresolved_skip_count": len(unresolved_skip_cases),
        "unresolved_skipped_cases": unresolved_skip_cases,
        "skip_breakdown": skip_breakdown,
        "threshold_override_audit": override_audit,
    }


def resolve_review_threshold_metric_value(
    metric_key: str,
    metrics: dict[str, object],
    geometry: dict[str, object],
) -> float | int | None:
    metric_value_map = {
        "max_mean_abs": metrics.get("mean_abs"),
        "max_rms": metrics.get("rms"),
        "max_hot_pixel_ratio": metrics.get("hot_pixel_ratio"),
        "max_bbox_edge_delta": geometry.get("bbox_edge_delta"),
        "max_bbox_area_delta_ratio": geometry.get("bbox_area_delta_ratio"),
    }
    value = metric_value_map.get(metric_key)
    if value is None:
        return None
    if metric_key in ("max_bbox_edge_delta",):
        return int(value)
    return float(value)


def compute_threshold_headroom(current_value: float | int | None, limit_value: float | int) -> dict[str, object]:
    if current_value is None:
        return {
            "current": None,
            "remaining": None,
            "remaining_ratio": None,
        }
    remaining = float(limit_value) - float(current_value)
    if abs(float(limit_value)) <= 1e-9:
        remaining_ratio: float | None = None
    else:
        remaining_ratio = remaining / float(limit_value)
    return {
        "current": current_value,
        "remaining": remaining,
        "remaining_ratio": remaining_ratio,
    }


def build_review_threshold_override_audit(case_results: list[dict[str, object]]) -> dict[str, object]:
    audit_cases: list[dict[str, object]] = []
    explicit_case_ids: list[str] = []
    inherited_case_ids: list[str] = []
    for case in case_results:
        review = case.get("review")
        if not isinstance(review, dict):
            continue
        threshold_overrides = case.get("review_threshold_overrides", review.get("threshold_overrides", {}))
        if not isinstance(threshold_overrides, dict) or not threshold_overrides:
            continue
        metrics = review.get("metrics", {})
        geometry = review.get("geometry", {})
        active_thresholds = review.get("thresholds", {})
        default_thresholds = review.get("default_thresholds", {})
        if not isinstance(metrics, dict) or not isinstance(geometry, dict) or not isinstance(active_thresholds, dict) or not isinstance(default_thresholds, dict):
            continue

        override_entries: list[dict[str, object]] = []
        closest_metric_key = ""
        closest_remaining_ratio: float | None = None
        uses_explicit_review_threshold = False
        uses_inherited_case_threshold = False

        for metric_key, override_payload in sorted(threshold_overrides.items()):
            if not isinstance(override_payload, dict):
                continue
            active_limit = active_thresholds.get(metric_key)
            default_limit = default_thresholds.get(metric_key)
            if active_limit is None or default_limit is None:
                continue
            origin = str(override_payload.get("origin", ""))
            if origin == "explicit_review_threshold":
                uses_explicit_review_threshold = True
            if origin == "inherited_case_threshold":
                uses_inherited_case_threshold = True
            current_value = resolve_review_threshold_metric_value(metric_key, metrics, geometry)
            headroom = compute_threshold_headroom(current_value, active_limit)
            remaining_ratio = headroom.get("remaining_ratio")
            if isinstance(remaining_ratio, float):
                if closest_remaining_ratio is None or remaining_ratio < closest_remaining_ratio:
                    closest_remaining_ratio = remaining_ratio
                    closest_metric_key = metric_key
            override_entries.append(
                {
                    "metric": metric_key,
                    "origin": origin,
                    "active_limit": active_limit,
                    "default_limit": default_limit,
                    "current": headroom.get("current"),
                    "remaining": headroom.get("remaining"),
                    "remaining_ratio": headroom.get("remaining_ratio"),
                }
            )

        case_id = str(case.get("id", ""))
        if uses_explicit_review_threshold:
            explicit_case_ids.append(case_id)
        if uses_inherited_case_threshold:
            inherited_case_ids.append(case_id)

        audit_cases.append(
            {
                "id": case_id,
                "title": str(case.get("title", "")),
                "review_status": str(case.get("review_status", review.get("status", ""))),
                "page_bucket": str(case.get("review_page_bucket", review.get("page_bucket", ""))),
                "compare": str(review.get("compare", "")),
                "diff": review.get("override_audit_diff", review.get("diff")),
                "threshold_override_keys": [str(item) for item in case.get("review_threshold_override_keys", review.get("threshold_override_keys", []))],
                "threshold_overrides": override_entries,
                "would_fail_default_review": bool(review.get("would_fail_default_review", False)),
                "default_failure_reasons": [str(item) for item in review.get("default_failure_reasons", [])],
                "closest_metric_key": closest_metric_key,
                "closest_remaining_ratio": closest_remaining_ratio,
            }
        )

    audit_cases.sort(key=lambda item: (0 if bool(item.get("would_fail_default_review", False)) else 1, str(item.get("id", ""))))
    return {
        "case_count": len(audit_cases),
        "case_ids": [str(item.get("id", "")) for item in audit_cases],
        "would_fail_default_count": sum(1 for item in audit_cases if bool(item.get("would_fail_default_review", False))),
        "would_fail_default_case_ids": [str(item.get("id", "")) for item in audit_cases if bool(item.get("would_fail_default_review", False))],
        "explicit_review_threshold_case_ids": explicit_case_ids,
        "inherited_case_threshold_case_ids": inherited_case_ids,
        "cases": audit_cases,
    }


def parse_csv_list(raw_value: str) -> tuple[str, ...]:
    return tuple(item.strip() for item in raw_value.split(",") if item.strip())


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Validate EmbeddedGUI SVG behavior against a large-viewport reference renderer",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Examples:\n"
            "  python scripts/checks/svg_validation_check.py\n"
            "  python scripts/checks/svg_validation_check.py --with-unit\n"
            "  python scripts/checks/svg_validation_check.py --case path_smooth_quad_fill --trial-render-box 56x56\n"
            "  python scripts/checks/svg_validation_check.py --edge-path \"C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe\"\n"
        ),
    )
    parser.add_argument("--with-unit", action="store_true", help="Also run supplemental internal HelloUnitTest SVG suites")
    parser.add_argument("--case", default="", help="Comma-separated SVG parity case ids to run")
    parser.add_argument("--trial-render-box", default="", help="Temporarily override the large-viewport image box for a single selected case using WIDTHxHEIGHT or WIDTHxHEIGHT@X,Y")
    parser.add_argument("--suite", default=",".join(DEFAULT_SUITES), help="Comma-separated internal HelloUnitTest suites used with --with-unit")
    parser.add_argument("--timeout", type=int, default=DEFAULT_TIMEOUT, help="Runtime timeout for HelloSVGSpec")
    parser.add_argument("--bits64", action="store_true", help="Build and run 64-bit binaries")
    parser.add_argument("--jobs", type=int, default=0, help="Parallel make jobs (0=auto)")
    parser.add_argument("--edge-path", default="", help="Explicit Edge/Chrome executable path")
    parser.add_argument(
        "--reference-engine",
        choices=list(REFERENCE_ENGINE_OPTIONS),
        default=REFERENCE_ENGINE_AUTO,
        help="Reference renderer backend (default: auto, prefer Edge/Chrome only when large-viewport SVG capture works, otherwise resvg)",
    )
    parser.add_argument("--output-subdir", default=OUTPUT_SUBDIR, help="Runtime output subdir under runtime_check_output/HelloSVGSpec")
    parser.add_argument(
        "--include-library-unsupported",
        action="store_true",
        help="Include cases that exceed the current vendored PlutoSVG support envelope",
    )
    parser.add_argument("--fuzzy-max-difference", type=int, default=DEFAULT_FUZZY_MAX_DIFFERENCE, help="Default WPT-style fuzzy maximum per-pixel channel difference")
    parser.add_argument("--fuzzy-max-diff-pixels", type=int, default=DEFAULT_FUZZY_MAX_DIFF_PIXELS, help="Default WPT-style fuzzy maximum differing pixel count")
    parser.add_argument("--max-mean-abs", type=float, default=DEFAULT_MAX_MEAN_ABS, help="Default large-viewport maximum mean absolute RGB difference")
    parser.add_argument("--max-rms", type=float, default=DEFAULT_MAX_RMS, help="Default large-viewport maximum RGB RMS difference")
    parser.add_argument("--max-hot-pixel-ratio", type=float, default=DEFAULT_MAX_HOT_PIXEL_RATIO, help="Default large-viewport maximum changed-pixel ratio")
    parser.add_argument("--hot-pixel-delta", type=int, default=DEFAULT_HOT_PIXEL_DELTA, help="RGB delta used for large-viewport changed-pixel ratio")
    parser.add_argument(
        "--review-mode",
        choices=[REVIEW_MODE_OFF, REVIEW_MODE_WARN, REVIEW_MODE_FAIL],
        default=REVIEW_MODE_WARN,
        help="Compatibility label for review-page reporting; primary large-viewport parity mismatches always fail",
    )
    parser.add_argument("--list-cases", action="store_true", help="Print the SVG parity case list and exit")
    parser.add_argument("--list-features", action="store_true", help="Print the covered SVG feature tags and exit")
    parser.add_argument("--list-feature-map", action="store_true", help="Print covered SVG feature tags and their case ids, then exit")
    parser.add_argument("--list-required-features", action="store_true", help="Print the required SVG feature contract tags and exit")
    parser.add_argument("--list-render-size-sensitive", action="store_true", help="Print cases that must keep the default render box and exit")
    parser.add_argument("--list-shrink-locked", action="store_true", help="Print cases that keep a non-default render box but are excluded from future shrink experiments and exit")
    parser.add_argument("--list-fuzzy-overrides", action="store_true", help="Print cases that use non-default fuzzy reftest tolerances and exit")
    parser.add_argument("--list-default-render-box", action="store_true", help="Print cases that still use the default render box and exit")
    parser.add_argument("--list-largest-render-boxes", action="store_true", help="Print cases ordered by render box area and exit")
    parser.add_argument(
        "--list-shrink-candidates",
        action="store_true",
        help="Print non-sensitive cases ordered by render box area for future shrink experiments and exit",
    )
    return parser.parse_args()


def filter_cases(cases: list[dict[str, object]], selected_case_ids: tuple[str, ...]) -> list[dict[str, object]]:
    if not selected_case_ids:
        return cases

    case_map = {str(case["id"]): case for case in cases}
    missing = [case_id for case_id in selected_case_ids if case_id not in case_map]
    if missing:
        raise RuntimeError("unknown svg parity cases: %s" % ", ".join(missing))

    selected_case_set: set[str] = set()
    for case_id in selected_case_ids:
        selected_case_set.add(case_id)

    return [case for case in cases if str(case["id"]) in selected_case_set]


def resolve_output_subdir(requested_output_subdir: str, case_filter_active: bool, cases: list[dict[str, object]]) -> str:
    output_subdir = requested_output_subdir.strip() or OUTPUT_SUBDIR

    if not case_filter_active or output_subdir != OUTPUT_SUBDIR:
        return output_subdir

    case_key = ",".join(str(case["id"]) for case in cases)
    case_hash = hashlib.sha1(case_key.encode("utf-8")).hexdigest()[:10]
    run_hash = hashlib.sha1(("%d:%d" % (os.getpid(), time.time_ns())).encode("utf-8")).hexdigest()[:8]
    return "%s_cases_%s_run_%s" % (OUTPUT_SUBDIR, case_hash, run_hash)


def main() -> int:
    args = parse_args()
    print("== SVG Standards Validation Check ==")

    try:
        manifest_data = load_case_manifest()
        cases = filter_cases(manifest_data.cases, parse_csv_list(args.case))
    except RuntimeError as exc:
        print("[FAIL] fixture setup: %s" % exc)
        return 1

    library_unsupported_cases: list[dict[str, object]] = []
    if not args.include_library_unsupported:
        try:
            cases, library_unsupported_cases = filter_plutosvg_supported_cases(cases)
        except (OSError, ET.ParseError) as exc:
            print("[FAIL] fixture setup: PlutoSVG support analysis failed: %s" % exc)
            return 1
        if library_unsupported_cases:
            print("PlutoSVG support filter excluded %d cases:" % len(library_unsupported_cases))
            for excluded_case in library_unsupported_cases:
                print("  - %s: %s" % (excluded_case["id"], "; ".join(str(item) for item in excluded_case["library_support_reasons"])))
        if not cases:
            print("[FAIL] fixture setup: no active SVG parity cases remain after PlutoSVG support filtering")
            return 1

    if args.list_cases:
        print_case_listing(cases)
        return 0

    coverage_features = summarize_case_features(cases)
    excluded_features = {
        str(feature)
        for excluded_case in library_unsupported_cases
        for feature in excluded_case.get("features", ())
    }
    active_required_features = tuple(feature for feature in manifest_data.required_features if feature not in excluded_features)
    focus_drift = validate_case_focus(manifest_data.cases)
    if focus_drift:
        print("[FAIL] fixture focus drift: each svg parity case must map to exactly one feature tag: %s" % ", ".join(focus_drift))
        return 1
    duplicate_features = validate_feature_uniqueness(manifest_data.cases)
    if duplicate_features:
        print("[FAIL] feature mapping drift: each svg feature tag must map to exactly one case: %s" % ", ".join(duplicate_features))
        return 1
    if args.list_features:
        for feature in coverage_features:
            print(feature)
        return 0

    if args.list_required_features:
        for feature in active_required_features:
            print(feature)
        return 0

    if args.list_render_size_sensitive:
        print_render_size_sensitive_cases(cases)
        return 0

    if args.list_shrink_locked:
        print_shrink_locked_cases(cases)
        return 0

    if args.list_fuzzy_overrides:
        print_fuzzy_override_cases(cases)
        return 0

    if args.list_default_render_box:
        print_default_render_box_cases(cases)
        return 0

    if args.list_largest_render_boxes:
        print_largest_render_boxes(cases)
        return 0

    if args.list_shrink_candidates:
        print_shrink_candidates(cases)
        return 0

    if args.list_feature_map:
        print_feature_case_map(cases)
        return 0

    case_filter_active = bool(args.case.strip())
    try:
        codegen_context = prepare_large_viewport_codegen_context(cases, case_filter_active, args.trial_render_box)
    except RuntimeError as exc:
        print("[FAIL] fixture setup: %s" % exc)
        return 1
    cases = codegen_context.cases

    try:
        run_codegen(codegen_context.manifest_path, codegen_context.fixture_dir)
    except RuntimeError as exc:
        print("[FAIL] fixture setup: %s" % exc)
        if codegen_context.temp_dir is not None:
            codegen_context.temp_dir.cleanup()
        return 1

    def execute_validation_run() -> int:
        try:
            reference_engine, reference_detail = resolve_reference_backend(args.reference_engine, args.edge_path or None)
        except RuntimeError as exc:
            print("[FAIL] reference renderer: %s" % exc)
            return 1

        apply_reference_engine_case_overrides(cases, reference_engine)
        apply_active_fuzzy_thresholds(cases, args.fuzzy_max_difference, args.fuzzy_max_diff_pixels)
        for case in cases:
            case["max_mean_abs"] = float(case.get("max_mean_abs", args.max_mean_abs))
            case["max_rms"] = float(case.get("max_rms", args.max_rms))
            case["max_hot_pixel_ratio"] = float(case.get("max_hot_pixel_ratio", args.max_hot_pixel_ratio))

        print("Reference renderer: %s (%s)" % (reference_engine, reference_detail))
        print("Cases: %s" % ", ".join(str(case["id"]) for case in cases))
        if coverage_features:
            print("Coverage tags (%d): %s" % (len(coverage_features), ", ".join(coverage_features)))
        output_subdir = resolve_output_subdir(args.output_subdir, case_filter_active, cases)
        missing_features: list[str] = []
        unexpected_features: list[str] = []
        if case_filter_active:
            print("Case filter active: coverage contract check skipped")
            print("Filtered output dir: %s" % output_subdir)
        else:
            missing_features, unexpected_features = validate_feature_contract(cases, active_required_features)
        if codegen_context.trial_render_box_request is not None:
            request = codegen_context.trial_render_box_request
            print(
                "Trial viewport box: %s %dx%d@%d,%d (manifest %dx%d@%d,%d)"
                % (
                    str(request["case_id"]),
                    int(request["trial_render_box"]["width"]),
                    int(request["trial_render_box"]["height"]),
                    int(request["trial_render_box"]["x"]),
                    int(request["trial_render_box"]["y"]),
                    int(request["manifest_render_box"]["width"]),
                    int(request["manifest_render_box"]["height"]),
                    int(request["manifest_render_box"]["x"]),
                    int(request["manifest_render_box"]["y"]),
                )
            )
        feature_case_map = build_feature_case_map(cases)
        render_box_summary = build_render_box_summary(cases, args.fuzzy_max_difference, args.fuzzy_max_diff_pixels)
        if missing_features:
            print("[FAIL] coverage requirements missing: %s" % ", ".join(missing_features))
            return 1
        if unexpected_features:
            print("[FAIL] uncovered contract drift: unexpected feature tags: %s" % ", ".join(unexpected_features))
            return 1

        primary_viewport_box = (
            dict(codegen_context.trial_render_box_request["trial_render_box"])
            if codegen_context.trial_render_box_request is not None
            else build_default_large_viewport_box()
        )

        try:
            runtime_dir = capture_runtime_cases(
                args.bits64,
                args.timeout,
                args.jobs,
                output_subdir,
                len(cases),
                build_primary_user_cflags(primary_viewport_box),
                recording_wait_ms=SVG_SPEC_REVIEW_RECORDING_WAIT_MS,
            )
            runtime_frames, runtime_order = load_runtime_case_frames(cases, runtime_dir)
        except RuntimeError as exc:
            print("[FAIL] runtime capture: %s" % exc)
            return 1

        print("Runtime frames: %s" % ", ".join(runtime_order))

        try:
            case_results, review_failures, review_skips, review_runtime = capture_fixed_viewport_case_results(
                cases,
                runtime_dir,
                runtime_frames,
                reference_engine,
                reference_detail,
                args.hot_pixel_delta,
                output_subdir,
            )
        except RuntimeError as exc:
            print("[FAIL] large viewport parity: %s" % exc)
            return 1

        display_case_results = [dict(case_result) for case_result in case_results]

        attach_review_family_refs(case_results)
        attach_review_companion_refs(case_results)
        attach_review_skip_classification(case_results)
        attach_review_skip_refs(review_skips, case_results)
        attach_review_family_refs(display_case_results)
        attach_review_companion_refs(display_case_results)
        attach_review_skip_classification(display_case_results)
        for case_result in case_results:
            message = str(case_result.get("status_message", case_result.get("canonical_message", "")))
            print("[PASS] %s" % message if str(case_result.get("status", "failed")) == "passed" else "[FAIL] %s" % message)
        all_passed = all(str(case_result.get("status", "failed")) == "passed" for case_result in case_results)
        artifacts = build_overview_artifacts(
            runtime_dir,
            display_case_results,
            coverage_features,
            reference_engine,
            args.with_unit,
        )
        artifacts["artifact_summary"] = str((runtime_dir / "svg_validation_artifacts_summary.json").relative_to(ROOT_DIR))
        artifacts["review_runtime"] = review_runtime
        artifacts["primary_parity_mode"] = PRIMARY_PARITY_MODE_LARGE_VIEWPORT
        artifacts["primary_parity_label"] = "large-viewport parity"
        review_summary = build_review_summary(args.review_mode, case_results, review_failures, review_skips)
        artifacts["review_runtime"]["mode"] = args.review_mode
        artifacts["review_runtime"]["status"] = str(review_summary["status"])
        artifacts["review_runtime"]["actionable_case_ids"] = list(review_summary["actionable_cases"])
        artifacts["review_runtime"]["skipped_case_ids"] = list(review_summary["skipped_cases"])
        artifacts["review_runtime"]["covered_skip_case_ids"] = list(review_summary["covered_skip_cases"])
        artifacts["review_runtime"]["unresolved_skip_case_ids"] = list(review_summary["unresolved_skipped_cases"])
        artifacts["review_runtime"]["failed_case_ids"] = list(review_summary["failed_cases"])
        artifacts["review_runtime"]["covered_skip_count"] = int(review_summary["covered_skip_count"])
        artifacts["review_runtime"]["unresolved_skip_count"] = int(review_summary["unresolved_skip_count"])
        artifacts["review_runtime"]["canonical_covered_by_review_count"] = 0
        artifacts["review_runtime"]["canonical_covered_by_skip_count"] = 0
        attach_overview_refs(case_results, artifacts)
        attach_review_page_refs(case_results, artifacts)
        attach_primary_visual_targets(case_results, artifacts)

        if review_failures:
            print("Large-viewport mismatches: %d" % len(review_failures))
            for review_failure in review_failures:
                print("[FAIL] %s" % format_review_failure_message(review_failure))
        else:
            print("Large-viewport mismatches: 0")
        print("Primary parity mode: large-viewport")
        print(
            "Actionable parity cases: %d/%d"
            % (
                int(review_summary["actionable_count"]),
                int(artifacts["overview_summary"]["all_case_count"]),
            )
        )
        if review_skips:
            print(
                "Large-viewport skips: %d total (%d companion-covered, %d canonical-only, %d default-reference-covered, %d unresolved)"
                % (
                    len(review_skips),
                    int(review_summary.get("skip_breakdown", {}).get("companion_covered", {}).get("case_count", 0)),
                    int(review_summary.get("skip_breakdown", {}).get("canonical_only", {}).get("case_count", 0)),
                    int(review_summary.get("skip_breakdown", {}).get("default_reference_covered", {}).get("case_count", 0)),
                    int(review_summary.get("unresolved_skip_count", 0)),
                )
            )
            for review_skip in review_skips:
                if bool(review_skip.get("is_unresolved", False)):
                    print("[INFO] %s" % format_review_skip_message(review_skip))
        review_override_audit = review_summary.get("threshold_override_audit", {})
        if isinstance(review_override_audit, dict) and int(review_override_audit.get("case_count", 0)) > 0:
            print(
                "Review threshold overrides: %d cases, %d would fail with defaults"
                % (
                    int(review_override_audit.get("case_count", 0)),
                    int(review_override_audit.get("would_fail_default_count", 0)),
                )
            )

        if not all_passed:
            report_path = write_validation_report(
                runtime_dir,
                reference_engine,
                reference_detail,
                coverage_features,
                manifest_data.required_features,
                feature_case_map,
                render_box_summary,
                codegen_context.trial_render_box_request,
                artifacts,
                case_results,
                args.review_mode,
                review_failures,
                review_skips,
                False,
                None,
            )
            write_artifact_summary(
                runtime_dir,
                reference_engine,
                reference_detail,
                report_path,
                artifacts,
                review_summary,
                case_results,
                False,
                None,
            )
            print_validation_artifacts(report_path, artifacts)
            print("Result: FAILED")
            return 1

        unit_message: str | None = None
        if args.with_unit:
            suites = parse_csv_list(args.suite)
            if not suites:
                print("[FAIL] no internal suites specified")
                return 1
            ok, message = run_svg_unit_suites(suites, args.jobs)
            unit_message = message
            print("[PASS] %s" % message if ok else "[FAIL] %s" % message)
            if not ok:
                report_path = write_validation_report(
                    runtime_dir,
                    reference_engine,
                    reference_detail,
                    coverage_features,
                    manifest_data.required_features,
                    feature_case_map,
                    render_box_summary,
                    codegen_context.trial_render_box_request,
                    artifacts,
                    case_results,
                    args.review_mode,
                    review_failures,
                    review_skips,
                    True,
                    unit_message,
                )
                write_artifact_summary(
                    runtime_dir,
                    reference_engine,
                    reference_detail,
                    report_path,
                    artifacts,
                    review_summary,
                    case_results,
                    True,
                    unit_message,
                )
                print_validation_artifacts(report_path, artifacts)
                return 1

        report_path = write_validation_report(
            runtime_dir,
            reference_engine,
            reference_detail,
            coverage_features,
            manifest_data.required_features,
            feature_case_map,
            render_box_summary,
            codegen_context.trial_render_box_request,
            artifacts,
            case_results,
            args.review_mode,
            review_failures,
            review_skips,
            args.with_unit,
            unit_message,
        )
        write_artifact_summary(
            runtime_dir,
            reference_engine,
            reference_detail,
            report_path,
            artifacts,
            review_summary,
            case_results,
            args.with_unit,
            unit_message,
        )
        print_validation_artifacts(report_path, artifacts)
        print("Result: ALL PASSED")
        return 0
    restore_error_message: str | None = None
    try:
        result = execute_validation_run()
    finally:
        if codegen_context.manifest_path is not None or codegen_context.fixture_dir is not None:
            try:
                run_codegen()
            except RuntimeError as exc:
                restore_error_message = str(exc)
        if codegen_context.temp_dir is not None:
            codegen_context.temp_dir.cleanup()

    if restore_error_message is not None:
        print("[FAIL] fixture restore: %s" % restore_error_message)
        return 1
    return result


if __name__ == "__main__":
    raise SystemExit(main())
