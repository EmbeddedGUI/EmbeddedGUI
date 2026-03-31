#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Performance regression check for EmbeddedGUI using QEMU ARM emulation.

Builds HelloPerformance for each CPU profile, runs in QEMU with deterministic
instruction counting (-icount shift=0), parses structured PERF_RESULT output,
and generates JSON + Markdown reports.

Usage:
    python scripts/code_perf_check.py --full-check
    python scripts/code_perf_check.py --profile cortex-m3
    python scripts/code_perf_check.py --threshold 15
"""

import os
import sys
import json
import re
import subprocess
import argparse
import platform
import shutil
from datetime import datetime
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
PROJECT_ROOT = SCRIPT_DIR.parent
PROFILES_FILE = SCRIPT_DIR / "perf_cpu_profiles.json"
OUTPUT_DIR = PROJECT_ROOT / "perf_output"
RESULTS_FILE = OUTPUT_DIR / "perf_results.json"
REPORT_FILE = OUTPUT_DIR / "perf_report.md"
SCENE_SHEET_FILE = OUTPUT_DIR / "perf_scenes.png"
SCENE_INDEX_FILE = OUTPUT_DIR / "perf_scenes_index.json"

DEFAULT_TIMEOUT = 300
DEFAULT_THRESHOLD = 10


def find_qemu_executable():
    """Find qemu-system-arm executable."""
    import shutil

    exe_name = "qemu-system-arm.exe" if platform.system() == "Windows" else "qemu-system-arm"

    # 1. Check QEMU_PATH environment variable
    qemu_path = os.environ.get("QEMU_PATH")
    if qemu_path:
        exe = Path(qemu_path) / exe_name
        if exe.exists():
            return str(exe)

    # 2. Check PATH
    qemu = shutil.which("qemu-system-arm")
    if qemu:
        return qemu

    # 3. Check common Windows installation directories
    if platform.system() == "Windows":
        candidates = [
            Path("C:/Program Files/qemu") / exe_name,
            Path("C:/Program Files (x86)/qemu") / exe_name,
            Path(os.environ.get("ProgramFiles", "C:/Program Files")) / "qemu" / exe_name,
            Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "qemu" / exe_name,
        ]
        for candidate in candidates:
            if candidate.exists():
                return str(candidate)

    return "qemu-system-arm"  # fallback, let subprocess raise FileNotFoundError


def load_profiles():
    """Load CPU profile configurations."""
    with open(PROFILES_FILE, "r") as f:
        data = json.load(f)
    return data["profiles"], data.get("defaults", {}), data.get("pfb_configs", {}), data.get("spi_configs", [])


def get_git_commit():
    """Get current git commit hash."""
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--short", "HEAD"],
            capture_output=True, text=True, cwd=PROJECT_ROOT
        )
        return result.stdout.strip()
    except Exception:
        return "unknown"


def build_for_profile(profile_name, profile_config, clean=False, extra_cflags=None):
    """Build HelloPerformance for a specific CPU profile.

    extra_cflags is passed via USER_CFLAGS on the make command line so that
    app_egui_config.h is never modified during testing.
    """
    cpu_arch = profile_config["cpu_arch"]
    print(f"\n{'='*60}")
    print(f"Building for {profile_name} (cpu_arch={cpu_arch})")
    print(f"{'='*60}")

    if clean:
        subprocess.run(['make', 'clean'], capture_output=True, cwd=PROJECT_ROOT)

    # Build — pass config overrides via USER_CFLAGS, never touch app_egui_config.h
    build_cmd = ['make', 'all', 'APP=HelloPerformance', 'PORT=qemu', f'CPU_ARCH={cpu_arch}']
    if extra_cflags:
        build_cmd.append(f'USER_CFLAGS={extra_cflags}')
    result = subprocess.run(
        build_cmd, cwd=PROJECT_ROOT,
        capture_output=True, text=True, timeout=120
    )

    if result.returncode != 0:
        print(f"  BUILD FAILED for {profile_name}")
        print(f"  stderr: {result.stderr[-500:]}")
        return False

    # Copy external resource binary to output/ for QEMU semihosting access
    res_bin_src = PROJECT_ROOT / "example" / "HelloPerformance" / "resource" / "app_egui_resource_merge.bin"
    res_bin_dst = PROJECT_ROOT / "output" / "app_egui_resource_merge.bin"
    if res_bin_src.exists() and not res_bin_dst.exists():
        shutil.copy(res_bin_src, res_bin_dst)

    print(f"  Build OK")
    return True


def run_qemu(profile_name, profile_config, timeout):
    """Run benchmark in QEMU and capture output.

    Uses streaming output parsing to terminate early when PERF_COMPLETE is detected.
    """
    machine = profile_config["qemu_machine"]
    cpu = profile_config["qemu_cpu"]
    elf_path = PROJECT_ROOT / "output" / "main.elf"

    if not elf_path.exists():
        print(f"  ELF not found: {elf_path}")
        return None

    qemu_cmd = [
        find_qemu_executable(),
        "-machine", machine,
        "-cpu", cpu,
        "-nographic",
        "-semihosting-config", "enable=on,target=native",
        "-icount", "shift=0",
        "-kernel", str(elf_path),
    ]

    print(f"  Running QEMU: machine={machine} cpu={cpu}")

    try:
        import time
        import threading
        proc = subprocess.Popen(
            qemu_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, cwd=PROJECT_ROOT
        )
        output_lines = []
        completed = False

        def reader_thread():
            nonlocal completed
            for line in proc.stdout:
                output_lines.append(line)
                # Print PERF_RESULT lines in real-time for progress feedback
                if "PERF_RESULT:" in line:
                    print(f"    {line.rstrip()}")
                if "PERF_COMPLETE" in line:
                    completed = True
                    break

        t = threading.Thread(target=reader_thread, daemon=True)
        t.start()
        t.join(timeout=timeout)

        if not completed:
            print(f"  QEMU TIMEOUT after {timeout}s")

        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()

        return "".join(output_lines)
    except FileNotFoundError:
        print(f"  ERROR: qemu-system-arm not found. Install QEMU first.")
        return None


def parse_perf_results(output):
    """Parse PERF_RESULT and PERF_SKIP lines from QEMU output."""
    results = {}
    skipped = []
    pattern = re.compile(r"PERF_RESULT:(\w+):(\d+)\.(\d+)")
    skip_pattern = re.compile(r"PERF_SKIP:(\w+)")

    for line in output.split("\n"):
        match = pattern.search(line)
        if match:
            name = match.group(1)
            time_ms = float(f"{match.group(2)}.{match.group(3)}")
            results[name] = {"time_ms": time_ms}
            continue

        skip_match = skip_pattern.search(line)
        if skip_match:
            skipped.append(skip_match.group(1))

    complete = "PERF_COMPLETE" in output
    return results, complete, skipped


def generate_markdown_report(all_results, threshold, git_commit, skipped=None):
    """Generate Markdown performance report grouped by category."""
    profiles = sorted(all_results.keys())
    profile_name = profiles[0] if profiles else "cortex-m3"
    results = all_results.get(profile_name, {})

    # Reuse CATEGORY_GROUPS from perf_to_doc for consistent ordering.
    # Fall back to sorted keys if categories are unavailable.
    try:
        sys.path.insert(0, str(SCRIPT_DIR))
        from perf_to_doc import _get_categorized_tests
        categorized = _get_categorized_tests(set(results.keys()))
    except Exception:
        categorized = [("All Tests", sorted(results.keys()))]

    lines = []
    lines.append("# EmbeddedGUI Performance Report")
    lines.append("")
    lines.append(f"- Commit: `{git_commit}`")
    lines.append(f"- Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    lines.append(f"- Profile: {profile_name}")
    lines.append(f"- Regression threshold: {threshold}%")
    lines.append("")

    try:
        if SCENE_SHEET_FILE.exists() and SCENE_INDEX_FILE.exists():
            scene_index = json.loads(SCENE_INDEX_FILE.read_text(encoding="utf-8"))
            if scene_index.get("git_commit") == git_commit and scene_index.get("profile") == profile_name:
                lines.append("## Scene Contact Sheet")
                lines.append("")
                lines.append("QEMU provides the timing data. The contact sheet below is rendered with the PC simulator for scene reference.")
                lines.append("")
                lines.append(f"![Performance Scenes]({SCENE_SHEET_FILE.name})")
                lines.append("")
    except Exception:
        pass

    for cat_name, tests in categorized:
        lines.append(f"## {cat_name}")
        lines.append("")
        lines.append("| Test Case | Time (ms) |")
        lines.append("|-----------|-----------|")
        for test in tests:
            t = results[test]["time_ms"]
            lines.append(f"| {test} | {t:.3f} |")
        lines.append("")

    if skipped:
        lines.append("## Skipped Tests")
        lines.append("")
        lines.append("The following tests were skipped (disabled at compile time):")
        lines.append("")
        for name in sorted(skipped):
            lines.append(f"- {name}")

    return "\n".join(lines)

# ============================================================================
# PFB Matrix Testing
# ============================================================================

PFB_MATRIX_RESULTS_FILE = OUTPUT_DIR / "pfb_matrix_results.json"
PFB_MATRIX_REPORT_FILE = OUTPUT_DIR / "pfb_matrix_report.md"
SPI_MATRIX_RESULTS_FILE = OUTPUT_DIR / "spi_matrix_results.json"
SPI_MATRIX_REPORT_FILE = OUTPUT_DIR / "spi_matrix_report.md"
APP_CONFIG_PATH = PROJECT_ROOT / "example" / "HelloPerformance" / "app_egui_config.h"

# Common extra defines injected via USER_CFLAGS for all QEMU perf builds.
# EGUI_TEST_CONFIG_IMAGE_565=1 enables image alpha tests (no flash limit on QEMU).
QEMU_BASE_CFLAGS = "-DEGUI_TEST_CONFIG_IMAGE_565=1"


def merge_extra_cflags(user_extra_cflags=None):
    parts = [QEMU_BASE_CFLAGS]
    if user_extra_cflags:
        parts.append(user_extra_cflags)
    return " ".join(part for part in parts if part)


def read_screen_size():
    """Read EGUI_CONFIG_SCEEN_WIDTH/HEIGHT from app_egui_config.h.

    Falls back per-dimension to the library defaults if one side is omitted.
    """
    default_w = 240
    default_h = 320
    try:
        text = APP_CONFIG_PATH.read_text(encoding="utf-8")
        w = h = None
        for line in text.splitlines():
            m = re.match(r"\s*#\s*define\s+EGUI_CONFIG_SCEEN_WIDTH\s+(\d+)", line)
            if m:
                w = int(m.group(1))
            m = re.match(r"\s*#\s*define\s+EGUI_CONFIG_SCEEN_HEIGHT\s+(\d+)", line)
            if m:
                h = int(m.group(1))
        return (w if w is not None else default_w, h if h is not None else default_h)
    except Exception:
        pass
    return default_w, default_h


def run_pfb_matrix(profile_name, profile_config, pfb_configs, timeout):
    """Run performance tests across all PFB configurations."""
    matrix_results = {}

    total = len(pfb_configs)
    for idx, (pfb_name, pfb_cfg) in enumerate(pfb_configs.items(), 1):
        pfb_w = pfb_cfg["pfb_width"]
        pfb_h = pfb_cfg["pfb_height"]
        desc = pfb_cfg.get("description", f"{pfb_w}x{pfb_h}")

        print(f"\n[{idx}/{total}] PFB config: {pfb_name} ({pfb_w}x{pfb_h}) - {desc}")

        # Pass PFB dimensions via USER_CFLAGS — never modify app_egui_config.h
        extra_cflags = (
            f"{merge_extra_cflags()}"
            f" -DEGUI_CONFIG_PFB_WIDTH={pfb_w}"
            f" -DEGUI_CONFIG_PFB_HEIGHT={pfb_h}"
        )

        try:
            # Build
            if not build_for_profile(f"{profile_name}/{pfb_name}", profile_config,
                                     clean=True, extra_cflags=extra_cflags):
                print(f"  SKIP: build failed for PFB {pfb_name}")
                continue

            # Run QEMU
            output = run_qemu(f"{profile_name}/{pfb_name}", profile_config, timeout)
            if output is None:
                print(f"  SKIP: QEMU failed for PFB {pfb_name}")
                continue

            # Parse
            results, complete, skipped = parse_perf_results(output)
            if not results:
                print(f"  SKIP: no results for PFB {pfb_name}")
                continue

            if not complete:
                print(f"  WARNING: PERF_COMPLETE not found")

            print(f"  Parsed {len(results)} test results")
            matrix_results[pfb_name] = {
                "pfb_width": pfb_w,
                "pfb_height": pfb_h,
                "results": results,
            }
        except Exception as e:
            print(f"  ERROR: {e}")

    return matrix_results


def generate_pfb_matrix_report(matrix_results, profile_name, git_commit):
    """Generate Markdown report for PFB matrix test."""
    configs = list(matrix_results.keys())
    if not configs:
        return "# PFB Matrix Report\n\nNo results collected.\n"

    # Collect all test names
    all_tests = set()
    for cfg_data in matrix_results.values():
        all_tests.update(cfg_data["results"].keys())
    all_tests = sorted(all_tests)

    lines = []
    lines.append("# EmbeddedGUI PFB Size Performance Matrix")
    lines.append("")
    lines.append(f"- Profile: {profile_name}")
    lines.append(f"- Commit: `{git_commit}`")
    lines.append(f"- Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    lines.append("")

    # Header
    header = "| Test Case |"
    separator = "|-----------|"
    for cfg_name in configs:
        d = matrix_results[cfg_name]
        w, h = d["pfb_width"], d["pfb_height"]
        header += f" {cfg_name} ({w}x{h}) |"
        separator += "------------|"
    lines.append(header)
    lines.append(separator)

    # Find the "default" config for relative comparison
    default_cfg = "default" if "default" in configs else configs[0]

    # Rows
    for test in all_tests:
        row = f"| {test} |"
        default_time = None
        if test in matrix_results.get(default_cfg, {}).get("results", {}):
            default_time = matrix_results[default_cfg]["results"][test]["time_ms"]

        for cfg_name in configs:
            results = matrix_results[cfg_name].get("results", {})
            if test in results:
                t = results[test]["time_ms"]
                row += f" {t:.3f} |"
            else:
                row += " - |"
        lines.append(row)

    lines.append("")

    # Summary: best PFB config per test
    lines.append("## Best PFB Config Per Test")
    lines.append("")
    lines.append("| Test Case | Best Config | Time (ms) | vs Default |")
    lines.append("|-----------|-------------|-----------|------------|")

    for test in all_tests:
        best_cfg = None
        best_time = float("inf")
        default_time = None

        for cfg_name in configs:
            results = matrix_results[cfg_name].get("results", {})
            if test in results:
                t = results[test]["time_ms"]
                if t < best_time:
                    best_time = t
                    best_cfg = cfg_name
                if cfg_name == default_cfg:
                    default_time = t

        if best_cfg:
            if default_time and default_time > 0:
                change = ((best_time - default_time) / default_time) * 100
                lines.append(f"| {test} | {best_cfg} | {best_time:.3f} | {change:+.1f}% |")
            else:
                lines.append(f"| {test} | {best_cfg} | {best_time:.3f} | - |")

    return "\n".join(lines)


def run_spi_matrix(profile_name, profile_config, spi_configs, timeout):
    """Run performance tests across SPI/buffer configurations."""
    matrix_results = {}

    total = len(spi_configs)
    for idx, spi_cfg in enumerate(spi_configs, 1):
        cfg_name = spi_cfg["name"]
        spi_mhz = spi_cfg.get("spi_mhz", 0)
        buf_count = spi_cfg.get("buffer_count", 1)
        pfb_w = spi_cfg.get("pfb_width", None)
        pfb_h = spi_cfg.get("pfb_height", None)
        desc = spi_cfg.get("desc", cfg_name)

        print(f"\n[{idx}/{total}] SPI config: {cfg_name} - {desc}")

        # Build extra_cflags from SPI/buffer settings — never modify app_egui_config.h
        parts = [merge_extra_cflags()]
        if spi_mhz > 0:
            parts.append(f"-DQEMU_SPI_SPEED_MHZ={spi_mhz}")
        if buf_count > 1:
            parts.append(f"-DEGUI_CONFIG_PFB_BUFFER_COUNT={buf_count}")
        if pfb_w is not None and pfb_h is not None:
            parts.append(f"-DEGUI_CONFIG_PFB_WIDTH={pfb_w}")
            parts.append(f"-DEGUI_CONFIG_PFB_HEIGHT={pfb_h}")
        extra_cflags = " ".join(parts)

        try:
            # Build
            if not build_for_profile(f"{profile_name}/{cfg_name}", profile_config,
                                     clean=True, extra_cflags=extra_cflags):
                print(f"  SKIP: build failed for {cfg_name}")
                continue

            # Run QEMU
            output = run_qemu(f"{profile_name}/{cfg_name}", profile_config, timeout)
            if output is None:
                print(f"  SKIP: QEMU failed for {cfg_name}")
                continue

            # Parse
            results, complete, skipped = parse_perf_results(output)
            if not results:
                print(f"  SKIP: no results for {cfg_name}")
                continue

            if not complete:
                print(f"  WARNING: PERF_COMPLETE not found")

            print(f"  Parsed {len(results)} test results")
            matrix_results[cfg_name] = {
                "spi_mhz": spi_mhz,
                "buffer_count": buf_count,
                "results": results,
            }
        except Exception as e:
            print(f"  ERROR: {e}")

    return matrix_results


def generate_spi_matrix_report(matrix_results, profile_name, git_commit):
    """Generate Markdown report for SPI matrix test."""
    SCREEN_W, SCREEN_H = read_screen_size()
    BYTES_PER_PIXEL = 2  # RGB565
    frame_bytes = SCREEN_W * SCREEN_H * BYTES_PER_PIXEL

    configs = list(matrix_results.keys())
    if not configs:
        return "# SPI Matrix Report\n\nNo results collected.\n"

    all_tests = set()
    for cfg_data in matrix_results.values():
        all_tests.update(cfg_data["results"].keys())
    all_tests = sorted(all_tests)

    lines = []
    lines.append("# EmbeddedGUI SPI Transfer Simulation Matrix")
    lines.append("")
    lines.append(f"- Profile: {profile_name}")
    lines.append(f"- Commit: `{git_commit}`")
    lines.append(f"- Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    lines.append("")

    # Theoretical SPI transfer performance section
    lines.append("## Theoretical SPI Transfer Performance")
    lines.append("")
    lines.append(f"Screen: {SCREEN_W}x{SCREEN_H} px, RGB565 ({BYTES_PER_PIXEL} bytes/px), full frame = {frame_bytes:,} bytes ({frame_bytes/1024:.1f} KB)")
    lines.append("")
    lines.append("All SPI tests use **dual buffer (DMA double-buffering)**:")
    lines.append("CPU renders into buffer A while DMA transfers buffer B, then they swap.")
    lines.append("Effective frame time = **max(CPU render time, SPI transfer time)**.")
    lines.append("")
    lines.append("| SPI Clock | Bit Rate | Full Frame Transfer | Max FPS (SPI-limited) |")
    lines.append("|-----------|----------|--------------------|-----------------------|")
    for spi_mhz in (16, 64):
        byte_rate = spi_mhz * 1_000_000 / 8
        transfer_ms = frame_bytes / byte_rate * 1000
        max_fps = 1000.0 / transfer_ms
        lines.append(f"| SPI{spi_mhz}M ({spi_mhz} MHz) | {spi_mhz} Mbps = {byte_rate/1e6:.0f} MB/s | {transfer_ms:.1f} ms | ~{max_fps:.1f} FPS |")
    lines.append("")
    lines.append("> Note: actual test timing depends on the dirty-region area redrawn per test case.")
    lines.append("")

    # Header
    header = "| Test Case |"
    separator = "|-----------|"
    for cfg_name in configs:
        d = matrix_results[cfg_name]
        spi = d["spi_mhz"]
        buf = d["buffer_count"]
        label = f"SPI{spi}M x{buf}buf" if spi > 0 else "CPU only"
        header += f" {label} |"
        separator += "------------|"
    lines.append(header)
    lines.append(separator)

    for test in all_tests:
        row = f"| {test} |"
        for cfg_name in configs:
            results = matrix_results[cfg_name].get("results", {})
            if test in results:
                t = results[test]["time_ms"]
                row += f" {t:.3f} |"
            else:
                row += " - |"
        lines.append(row)

    lines.append("")

    # Summary: SPI overhead vs CPU-only
    cpu_only_cfg = None
    for cfg_name in configs:
        if matrix_results[cfg_name]["spi_mhz"] == 0:
            cpu_only_cfg = cfg_name
            break

    spi_cfgs = [c for c in configs if matrix_results[c]["spi_mhz"] > 0]

    if cpu_only_cfg and spi_cfgs:
        lines.append("## SPI Bottleneck Analysis (vs CPU-only)")
        lines.append("")
        lines.append(f"Effective time = max(CPU render, SPI transfer). "
                     f"Rows where SPI dominates are marked with the SPI overhead.")
        lines.append("")
        hdr = "| Test Case | CPU only (ms) |"
        sep = "|-----------|---------------|"
        for c in spi_cfgs:
            d = matrix_results[c]
            label = f"SPI{d['spi_mhz']}M x{d['buffer_count']}buf (ms)"
            hdr += f" {label} | SPI overhead |"
            sep += "-----------|--------------|"
        lines.append(hdr)
        lines.append(sep)

        for test in all_tests:
            cpu_time = None
            if test in matrix_results[cpu_only_cfg]["results"]:
                cpu_time = matrix_results[cpu_only_cfg]["results"][test]["time_ms"]
            if cpu_time is None:
                continue
            row = f"| {test} | {cpu_time:.3f} |"
            dominated = False
            for c in spi_cfgs:
                spi_res = matrix_results[c]["results"]
                if test in spi_res:
                    t = spi_res[test]["time_ms"]
                    overhead = t - cpu_time
                    overhead_str = f"+{overhead:.3f}" if overhead > 0.001 else "~0"
                    row += f" {t:.3f} | {overhead_str} |"
                    if overhead > 0.001:
                        dominated = True
                else:
                    row += " - | - |"
            if dominated:
                lines.append(row)

    return "\n".join(lines)


def _do_pfb_matrix(profiles, pfb_configs, args, timeout):
    """Run PFB matrix test and save results. Returns True on success."""
    if not pfb_configs:
        print("  [SKIP] No pfb_configs defined in perf_cpu_profiles.json")
        return True

    profile_name = args.profile or "cortex-m3"
    if profile_name not in profiles:
        print(f"  ERROR: Unknown profile for PFB matrix: {profile_name}")
        return False

    git_commit = get_git_commit()
    profile_config = profiles[profile_name]

    print(f"PFB Matrix Test: profile={profile_name}, {len(pfb_configs)} configs")
    matrix_results = run_pfb_matrix(profile_name, profile_config, pfb_configs, timeout)

    matrix_data = {
        "timestamp": datetime.now().isoformat(),
        "git_commit": git_commit,
        "profile": profile_name,
        "pfb_matrix": {k: v for k, v in matrix_results.items()},
    }
    with open(PFB_MATRIX_RESULTS_FILE, "w") as f:
        json.dump(matrix_data, f, indent=2)
    print(f"  PFB matrix results saved to: {PFB_MATRIX_RESULTS_FILE}")

    report = generate_pfb_matrix_report(matrix_results, profile_name, git_commit)
    with open(PFB_MATRIX_REPORT_FILE, "w", encoding="utf-8") as f:
        f.write(report)
    print(f"  PFB matrix report saved to: {PFB_MATRIX_REPORT_FILE}")

    success = len(matrix_results) == len(pfb_configs)
    print(f"  PFB Matrix: {len(matrix_results)}/{len(pfb_configs)} configs succeeded")
    return success


def _do_spi_matrix(profiles, spi_configs, args, timeout):
    """Run SPI matrix test and save results. Returns True on success."""
    if not spi_configs:
        print("  [SKIP] No spi_configs defined in perf_cpu_profiles.json")
        return True

    profile_name = args.profile or "cortex-m3"
    if profile_name not in profiles:
        print(f"  ERROR: Unknown profile for SPI matrix: {profile_name}")
        return False

    git_commit = get_git_commit()
    profile_config = profiles[profile_name]

    print(f"SPI Matrix Test: profile={profile_name}, {len(spi_configs)} configs")
    matrix_results = run_spi_matrix(profile_name, profile_config, spi_configs, timeout)

    matrix_data = {
        "timestamp": datetime.now().isoformat(),
        "git_commit": git_commit,
        "profile": profile_name,
        "spi_matrix": {k: v for k, v in matrix_results.items()},
    }
    with open(SPI_MATRIX_RESULTS_FILE, "w") as f:
        json.dump(matrix_data, f, indent=2)
    print(f"  SPI matrix results saved to: {SPI_MATRIX_RESULTS_FILE}")

    report = generate_spi_matrix_report(matrix_results, profile_name, git_commit)
    with open(SPI_MATRIX_REPORT_FILE, "w", encoding="utf-8") as f:
        f.write(report)
    print(f"  SPI matrix report saved to: {SPI_MATRIX_REPORT_FILE}")

    success = len(matrix_results) == len(spi_configs)
    print(f"  SPI Matrix: {len(matrix_results)}/{len(spi_configs)} configs succeeded")
    return success


def run_scene_capture(profile_name, keyword_filter=None, timeout=180):
    """Capture HelloPerformance scene renders and compose a contact sheet."""
    script_path = SCRIPT_DIR / "perf_scene_capture.py"
    if not script_path.exists():
        print(f"  ERROR: scene capture script not found: {script_path}")
        return False

    cmd = [
        sys.executable,
        str(script_path),
        "--results",
        str(RESULTS_FILE),
        "--profile",
        profile_name,
        "--timeout",
        str(max(120, timeout)),
    ]
    if keyword_filter:
        cmd.extend(["--filter", keyword_filter])

    try:
        result = subprocess.run(
            cmd,
            cwd=PROJECT_ROOT,
            capture_output=True,
            text=True,
            timeout=max(timeout + 120, 300),
        )
    except subprocess.TimeoutExpired:
        print("  ERROR: scene capture timed out")
        return False

    if result.stdout:
        print(result.stdout.rstrip())
    if result.stderr:
        print(result.stderr.rstrip())

    if result.returncode != 0:
        print(f"  ERROR: scene capture failed with exit code {result.returncode}")
        return False

    return True


def _generate_perf_documentation():
    """Generate all performance documentation reports."""
    try:
        import perf_to_doc
        perf_to_doc.generate_perf_report()
        perf_to_doc.generate_pfb_matrix_report()
        perf_to_doc.generate_spi_matrix_report()
    except ImportError:
        print("Warning: perf_to_doc module not found, skipping documentation generation")
    except Exception as e:
        raise Exception(f"Failed to generate performance documentation: {e}")


def main():
    parser = argparse.ArgumentParser(
        description="EmbeddedGUI QEMU Performance Regression Check"
    )
    parser.add_argument("--full-check", action="store_true",
                        help="Run cortex-m3 perf + PFB matrix + SPI matrix")
    parser.add_argument("--profile", type=str,
                        help="Run specific profile (e.g., cortex-m3)")
    parser.add_argument("--threshold", type=float, default=DEFAULT_THRESHOLD,
                        help=f"Regression threshold %% (default: {DEFAULT_THRESHOLD})")
    parser.add_argument("--timeout", type=int, default=DEFAULT_TIMEOUT,
                        help=f"QEMU timeout seconds (default: {DEFAULT_TIMEOUT})")
    parser.add_argument("--pfb-matrix", action="store_true",
                        help="Run PFB size matrix test (varies PFB dimensions)")
    parser.add_argument("--spi-matrix", action="store_true",
                        help="Run SPI transfer simulation matrix test (varies buffer count)")
    parser.add_argument("--clean", action="store_true",
                        help="Run make clean before building (default: incremental build)")
    parser.add_argument("--filter", type=str,
                        help="Filter test results by keyword (e.g., RECT,CIRCLE)")
    parser.add_argument("--with-scenes", action="store_true",
                        help="Capture HelloPerformance renders and generate a scene contact sheet")
    parser.add_argument("--extra-cflags", type=str,
                        help="Append extra USER_CFLAGS for A/B config testing (e.g. -DMY_MACRO=0)")
    parser.add_argument("--doc", action="store_true",
                        help="Generate documentation (Markdown reports and charts) after tests complete")
    args = parser.parse_args()

    profiles, defaults, pfb_configs, spi_configs = load_profiles()
    threshold = args.threshold or defaults.get("regression_threshold_percent", DEFAULT_THRESHOLD)
    timeout = args.timeout or defaults.get("timeout_seconds", DEFAULT_TIMEOUT)

    # Determine which profiles to run
    if args.profile:
        if args.profile not in profiles:
            print(f"Unknown profile: {args.profile}")
            print(f"Available: {', '.join(profiles.keys())}")
            sys.exit(1)
        run_profiles = {args.profile: profiles[args.profile]}
    else:
        # Default and --full-check: run cortex-m3 only.
        # QEMU -icount shift=0 counts instructions uniformly, so different
        # cortex-m profiles produce nearly identical results — multi-profile
        # perf comparison is not meaningful under instruction counting.
        run_profiles = {"cortex-m3": profiles["cortex-m3"]}

    # Create output directory
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    # Standalone PFB Matrix mode (--pfb-matrix without --full-check)
    if args.pfb_matrix and not args.full_check:
        ok = _do_pfb_matrix(profiles, pfb_configs, args, timeout)
        sys.exit(0 if ok else 2)

    # Standalone SPI Matrix mode (--spi-matrix without --full-check)
    if args.spi_matrix and not args.full_check:
        ok = _do_spi_matrix(profiles, spi_configs, args, timeout)
        sys.exit(0 if ok else 2)

    git_commit = get_git_commit()
    all_results = {}
    failed_profiles = []

    total = len(run_profiles)
    all_skipped = set()
    for idx, (name, config) in enumerate(run_profiles.items(), 1):
        print(f"\n[{idx}/{total}] Profile: {name} - {config['description']}")

        need_clean = args.clean

        # Build — enable all image tests via USER_CFLAGS (no flash limit on QEMU)
        if not build_for_profile(name, config, clean=need_clean,
                                 extra_cflags=merge_extra_cflags(args.extra_cflags)):
            failed_profiles.append(name)
            continue

        # Run QEMU
        output = run_qemu(name, config, timeout)
        if output is None:
            failed_profiles.append(name)
            continue

        # Parse results
        results, complete, skipped = parse_perf_results(output)
        all_skipped.update(skipped)
        if not results:
            print(f"  WARNING: No PERF_RESULT lines found in output")
            print(f"  Output (last 500 chars): {output[-500:]}")
            failed_profiles.append(name)
            continue

        if not complete:
            print(f"  WARNING: PERF_COMPLETE not found (may have timed out)")

        # Apply filter if specified
        if args.filter:
            keywords = [k.strip().upper() for k in args.filter.split(",")]
            results = {k: v for k, v in results.items()
                       if any(kw in k.upper() for kw in keywords)}

        print(f"  Parsed {len(results)} test results")

        all_results[name] = results

    # Save results JSON
    results_data = {
        "timestamp": datetime.now().isoformat(),
        "git_commit": git_commit,
        "threshold_percent": threshold,
        "profiles": all_results,
    }
    with open(RESULTS_FILE, "w") as f:
        json.dump(results_data, f, indent=2)
    print(f"\nResults saved to: {RESULTS_FILE}")

    scenes_failed = False
    # Generate scene capture if --with-scenes or --doc is specified
    if (args.with_scenes or args.doc) and all_results:
        first_profile_name = next(iter(all_results.keys()))
        print(f"\n{'='*60}")
        print("Scene Capture")
        print(f"{'='*60}")
        if not run_scene_capture(first_profile_name, args.filter, timeout=max(timeout, 180)):
            scenes_failed = True

    # Generate Markdown report
    report = generate_markdown_report(
        all_results, threshold, git_commit, skipped=all_skipped
    )
    with open(REPORT_FILE, "w", encoding="utf-8") as f:
        f.write(report)
    print(f"Report saved to: {REPORT_FILE}")

    # Print summary
    print(f"\n{'='*60}")
    print(f"Performance Check Summary")
    print(f"{'='*60}")
    print(f"  Profiles tested: {len(all_results)}")
    print(f"  Failed profiles: {len(failed_profiles)}")
    if failed_profiles:
        print(f"    {', '.join(failed_profiles)}")

    matrix_failed = False

    # In full-check mode, also run PFB and SPI matrix tests
    if args.full_check:
        print(f"\n{'='*60}")
        print(f"PFB Matrix Test")
        print(f"{'='*60}")
        if not _do_pfb_matrix(profiles, pfb_configs, args, timeout):
            matrix_failed = True

        print(f"\n{'='*60}")
        print(f"SPI Matrix Test")
        print(f"{'='*60}")
        if not _do_spi_matrix(profiles, spi_configs, args, timeout):
            matrix_failed = True

    # Generate documentation from performance results (if --doc flag is set)
    if args.doc:
        print(f"\n{'='*60}")
        print(f"Generating Documentation")
        print(f"{'='*60}")
        try:
            _generate_perf_documentation()

            # Copy scene capture image to documentation directory
            doc_img_dir = PROJECT_ROOT / "doc" / "source" / "performance" / "images"
            doc_img_dir.mkdir(parents=True, exist_ok=True)

            if SCENE_SHEET_FILE.exists():
                import shutil
                dest_scene = doc_img_dir / "perf_scenes.png"
                shutil.copy2(SCENE_SHEET_FILE, dest_scene)
                print(f"  Scene capture copied to: {dest_scene}")

            print("Documentation updated successfully")
        except Exception as e:
            print(f"Warning: Failed to generate documentation: {e}")
            import traceback
            traceback.print_exc()

    if failed_profiles or matrix_failed or scenes_failed:
        print(f"\nWARNING: some tests failed to build/run")
        sys.exit(2)
    else:
        print(f"\nPASSED")
        sys.exit(0)


if __name__ == "__main__":
    main()
