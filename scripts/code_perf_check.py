#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Performance regression check for EmbeddedGUI using QEMU ARM emulation.

Builds HelloPerformace for each CPU profile, runs in QEMU with deterministic
instruction counting (-icount shift=0), parses structured PERF_RESULT output,
compares against baseline, and generates JSON + Markdown reports.

Usage:
    python scripts/code_perf_check.py --full-check
    python scripts/code_perf_check.py --profile cortex-m3
    python scripts/code_perf_check.py --update-baseline
    python scripts/code_perf_check.py --threshold 15
"""

import os
import sys
import json
import re
import subprocess
import argparse
import platform
from datetime import datetime
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
PROJECT_ROOT = SCRIPT_DIR.parent
PROFILES_FILE = SCRIPT_DIR / "perf_cpu_profiles.json"
OUTPUT_DIR = PROJECT_ROOT / "perf_output"
BASELINE_FILE = OUTPUT_DIR / "baseline.json"
RESULTS_FILE = OUTPUT_DIR / "perf_results.json"
REPORT_FILE = OUTPUT_DIR / "perf_report.md"

DEFAULT_TIMEOUT = 300
DEFAULT_THRESHOLD = 10


def find_qemu_executable():
    """Find qemu-system-arm executable."""
    # 1. Check QEMU_PATH environment variable
    qemu_path = os.environ.get("QEMU_PATH")
    if qemu_path:
        exe_name = "qemu-system-arm.exe" if platform.system() == "Windows" else "qemu-system-arm"
        exe = Path(qemu_path) / exe_name
        if exe.exists():
            return str(exe)

    # 2. Check PATH
    import shutil
    qemu = shutil.which("qemu-system-arm")
    if qemu:
        return qemu

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


def build_for_profile(profile_name, profile_config, clean=False):
    """Build HelloPerformace for a specific CPU profile."""
    cpu_arch = profile_config["cpu_arch"]
    print(f"\n{'='*60}")
    print(f"Building for {profile_name} (cpu_arch={cpu_arch})")
    print(f"{'='*60}")

    if clean:
        clean_cmd = f"make clean"
        subprocess.run(clean_cmd, shell=True, cwd=PROJECT_ROOT,
                       capture_output=True)

    # Build
    build_cmd = f"make all APP=HelloPerformace PORT=qemu CPU_ARCH={cpu_arch}"
    result = subprocess.run(
        build_cmd, shell=True, cwd=PROJECT_ROOT,
        capture_output=True, text=True, timeout=120
    )

    if result.returncode != 0:
        print(f"  BUILD FAILED for {profile_name}")
        print(f"  stderr: {result.stderr[-500:]}")
        return False

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
        proc = subprocess.Popen(
            qemu_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, cwd=PROJECT_ROOT
        )
        output_lines = []
        start_time = time.time()
        completed = False

        for line in proc.stdout:
            output_lines.append(line)
            # Print PERF_RESULT lines in real-time for progress feedback
            if "PERF_RESULT:" in line:
                print(f"    {line.rstrip()}")
            if "PERF_COMPLETE" in line:
                completed = True
                break
            if time.time() - start_time > timeout:
                print(f"  QEMU TIMEOUT after {timeout}s")
                break

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
            integer_part = int(match.group(2))
            decimal_part = int(match.group(3))
            time_ms = float(f"{integer_part}.{decimal_part}")
            results[name] = {"time_ms": time_ms}
            continue

        skip_match = skip_pattern.search(line)
        if skip_match:
            skipped.append(skip_match.group(1))

    complete = "PERF_COMPLETE" in output
    return results, complete, skipped


def compare_with_baseline(current, baseline, threshold):
    """Compare current results with baseline, return regression info."""
    regressions = []

    for test_name, current_data in current.items():
        if test_name in baseline:
            base_time = baseline[test_name]["time_ms"]
            curr_time = current_data["time_ms"]

            if base_time > 0:
                change_pct = ((curr_time - base_time) / base_time) * 100
            else:
                change_pct = 0.0

            current_data["baseline_ms"] = base_time
            current_data["change_pct"] = round(change_pct, 1)
            current_data["regression"] = change_pct > threshold
            if current_data["regression"]:
                regressions.append(test_name)
        else:
            current_data["baseline_ms"] = None
            current_data["change_pct"] = None
            current_data["regression"] = False

    return regressions


def generate_markdown_report(all_results, baseline, threshold, git_commit, skipped=None):
    """Generate Markdown performance report."""
    profiles = sorted(all_results.keys())
    all_tests = set()
    for profile_results in all_results.values():
        all_tests.update(profile_results.keys())
    all_tests = sorted(all_tests)

    lines = []
    lines.append("# EmbeddedGUI Performance Report")
    lines.append("")
    lines.append(f"- Commit: `{git_commit}`")
    lines.append(f"- Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    lines.append(f"- Regression threshold: {threshold}%")
    lines.append("")

    # Header
    header = "| Test Case |"
    separator = "|-----------|"
    for p in profiles:
        short = p.replace("cortex-", "").upper()
        header += f" {short} (ms) |"
        separator += "----------|"
    if baseline:
        header += " vs Baseline |"
        separator += "-------------|"
    lines.append(header)
    lines.append(separator)

    # Rows
    has_regression = False
    for test in all_tests:
        row = f"| {test} |"
        worst_change = None

        for p in profiles:
            if test in all_results.get(p, {}):
                t = all_results[p][test]["time_ms"]
                row += f" {t:.3f} |"

                change = all_results[p][test].get("change_pct")
                if change is not None:
                    if worst_change is None or change > worst_change:
                        worst_change = change
            else:
                row += " - |"

        if baseline and worst_change is not None:
            if worst_change > threshold:
                row += f" +{worst_change:.1f}% :warning: |"
                has_regression = True
            elif worst_change < -threshold:
                row += f" {worst_change:.1f}% :rocket: |"
            else:
                row += f" {worst_change:+.1f}% :white_check_mark: |"
        elif baseline:
            row += " NEW |"

        lines.append(row)

    lines.append("")
    if has_regression:
        lines.append("> :warning: Performance regression detected!")
    else:
        lines.append("> :white_check_mark: No performance regressions.")

    # Skipped tests section
    if skipped:
        lines.append("")
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

APP_CONFIG_PATH = PROJECT_ROOT / "example" / "HelloPerformace" / "app_egui_config.h"
PFB_MATRIX_RESULTS_FILE = OUTPUT_DIR / "pfb_matrix_results.json"
PFB_MATRIX_REPORT_FILE = OUTPUT_DIR / "pfb_matrix_report.md"
SPI_MATRIX_RESULTS_FILE = OUTPUT_DIR / "spi_matrix_results.json"
SPI_MATRIX_REPORT_FILE = OUTPUT_DIR / "spi_matrix_report.md"


def read_app_config():
    """Read and return the original app_egui_config.h content."""
    with open(APP_CONFIG_PATH, "r", encoding="utf-8") as f:
        return f.read()


def write_app_config(content):
    """Write content to app_egui_config.h."""
    with open(APP_CONFIG_PATH, "w", encoding="utf-8") as f:
        f.write(content)


def patch_app_config_pfb(original_content, pfb_width, pfb_height):
    """Patch app_egui_config.h with specific PFB dimensions.

    Inserts PFB defines right after the opening extern C block.
    """
    pfb_defines = (
        f"#define EGUI_CONFIG_PFB_WIDTH {pfb_width}\n"
        f"#define EGUI_CONFIG_PFB_HEIGHT {pfb_height}\n"
    )

    # Remove any existing PFB defines
    lines = original_content.split("\n")
    filtered = [l for l in lines
                if "EGUI_CONFIG_PFB_WIDTH" not in l
                and "EGUI_CONFIG_PFB_HEIGHT" not in l]
    content = "\n".join(filtered)

    # Insert after the extern C opening brace
    marker = 'extern "C" {\n#endif'
    if marker in content:
        content = content.replace(marker, marker + "\n\n" + pfb_defines)
    else:
        # Fallback: insert after first #define
        first_define = content.find("#define ")
        if first_define >= 0:
            line_end = content.find("\n", first_define)
            content = content[:line_end+1] + "\n" + pfb_defines + content[line_end+1:]

    return content


def patch_app_config_image_tests(original_content):
    """Patch app_egui_config.h to enable all image alpha test cases for QEMU.

    QEMU has no flash size limit, so we can enable all alpha variants.
    """
    image_defines = (
        "#define EGUI_TEST_CONFIG_IMAGE_565   1\n"
        "#define EGUI_TEST_CONFIG_IMAGE_565_1 1\n"
        "#define EGUI_TEST_CONFIG_IMAGE_565_2 1\n"
        "#define EGUI_TEST_CONFIG_IMAGE_565_4 1\n"
        "#define EGUI_TEST_CONFIG_IMAGE_565_8 1\n"
    )

    # Remove any existing image test defines
    lines = original_content.split("\n")
    filtered = [l for l in lines if "EGUI_TEST_CONFIG_IMAGE_565" not in l]
    content = "\n".join(filtered)

    # Insert after the extern C opening brace
    marker = 'extern "C" {\n#endif'
    if marker in content:
        content = content.replace(marker, marker + "\n\n" + image_defines)
    else:
        first_define = content.find("#define ")
        if first_define >= 0:
            line_end = content.find("\n", first_define)
            content = content[:line_end+1] + "\n" + image_defines + content[line_end+1:]

    return content


def run_pfb_matrix(profile_name, profile_config, pfb_configs, timeout):
    """Run performance tests across all PFB configurations."""
    original_config = read_app_config()
    matrix_results = {}

    total = len(pfb_configs)
    for idx, (pfb_name, pfb_cfg) in enumerate(pfb_configs.items(), 1):
        pfb_w = pfb_cfg["pfb_width"]
        pfb_h = pfb_cfg["pfb_height"]
        desc = pfb_cfg.get("description", f"{pfb_w}x{pfb_h}")

        print(f"\n[{idx}/{total}] PFB config: {pfb_name} ({pfb_w}x{pfb_h}) - {desc}")

        # Patch config
        patched = patch_app_config_pfb(original_config, pfb_w, pfb_h)
        write_app_config(patched)

        try:
            # Build
            if not build_for_profile(f"{profile_name}/{pfb_name}", profile_config, clean=True):
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

    # Restore original config
    write_app_config(original_config)
    print(f"\nRestored original app_egui_config.h")

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


def patch_app_config_spi(original_content, spi_mhz, buffer_count, pfb_width=None, pfb_height=None):
    """Patch app_egui_config.h with SPI speed and buffer count settings.

    Optionally also sets PFB dimensions.
    """
    defines = ""
    if spi_mhz > 0:
        defines += f"#define QEMU_SPI_SPEED_MHZ {spi_mhz}\n"
    if buffer_count > 1:
        defines += f"#define EGUI_CONFIG_PFB_BUFFER_COUNT {buffer_count}\n"

    # Remove any existing SPI/buffer defines
    lines = original_content.split("\n")
    filtered = [l for l in lines
                if "QEMU_SPI_SPEED_MHZ" not in l
                and "EGUI_CONFIG_PFB_BUFFER_COUNT" not in l]

    # Also handle PFB dimensions if specified
    if pfb_width is not None and pfb_height is not None:
        filtered = [l for l in filtered
                    if "EGUI_CONFIG_PFB_WIDTH" not in l
                    and "EGUI_CONFIG_PFB_HEIGHT" not in l]
        defines += f"#define EGUI_CONFIG_PFB_WIDTH {pfb_width}\n"
        defines += f"#define EGUI_CONFIG_PFB_HEIGHT {pfb_height}\n"

    content = "\n".join(filtered)

    if not defines:
        return content

    # Insert after the extern C opening brace
    marker = 'extern "C" {\n#endif'
    if marker in content:
        content = content.replace(marker, marker + "\n\n" + defines)
    else:
        first_define = content.find("#define ")
        if first_define >= 0:
            line_end = content.find("\n", first_define)
            content = content[:line_end+1] + "\n" + defines + content[line_end+1:]

    return content


def run_spi_matrix(profile_name, profile_config, spi_configs, timeout):
    """Run performance tests across SPI/buffer configurations."""
    original_config = read_app_config()
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

        # Patch config
        patched = patch_app_config_spi(original_config, spi_mhz, buf_count, pfb_w, pfb_h)
        write_app_config(patched)

        try:
            # Build
            if not build_for_profile(f"{profile_name}/{cfg_name}", profile_config, clean=True):
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

    # Restore original config
    write_app_config(original_config)
    print(f"\nRestored original app_egui_config.h")

    return matrix_results


def generate_spi_matrix_report(matrix_results, profile_name, git_commit):
    """Generate Markdown report for SPI matrix test."""
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

    # Find baseline (first config, typically no_spi)
    baseline_cfg = configs[0]

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

    # Summary: speedup from multi-buffering
    if len(configs) >= 2:
        single_buf_cfg = None
        for cfg_name in configs:
            d = matrix_results[cfg_name]
            if d["spi_mhz"] > 0 and d["buffer_count"] == 1:
                single_buf_cfg = cfg_name
                break

        if single_buf_cfg:
            lines.append("## Multi-Buffer Speedup vs Single Buffer")
            lines.append("")
            lines.append("| Test Case | Single Buf (ms) | Best Multi-Buf | Time (ms) | Speedup |")
            lines.append("|-----------|----------------|----------------|-----------|---------|")

            for test in all_tests:
                single_time = None
                if test in matrix_results[single_buf_cfg]["results"]:
                    single_time = matrix_results[single_buf_cfg]["results"][test]["time_ms"]

                if single_time is None or single_time <= 0:
                    continue

                best_cfg = None
                best_time = single_time
                for cfg_name in configs:
                    d = matrix_results[cfg_name]
                    if d["spi_mhz"] > 0 and d["buffer_count"] > 1:
                        if test in d["results"]:
                            t = d["results"][test]["time_ms"]
                            if t < best_time:
                                best_time = t
                                best_cfg = cfg_name

                if best_cfg:
                    speedup = ((single_time - best_time) / single_time) * 100
                    lines.append(f"| {test} | {single_time:.3f} | {best_cfg} | {best_time:.3f} | -{speedup:.1f}% |")

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="EmbeddedGUI QEMU Performance Regression Check"
    )
    parser.add_argument("--full-check", action="store_true",
                        help="Run all CPU profiles")
    parser.add_argument("--profile", type=str,
                        help="Run specific profile (e.g., cortex-m3)")
    parser.add_argument("--update-baseline", action="store_true",
                        help="Save current results as new baseline")
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
    elif args.full_check:
        run_profiles = profiles
    else:
        # Default: run cortex-m3 only
        run_profiles = {"cortex-m3": profiles["cortex-m3"]}

    # Load baseline if exists
    baseline = {}
    if BASELINE_FILE.exists():
        with open(BASELINE_FILE, "r") as f:
            baseline_data = json.load(f)
            baseline = baseline_data.get("profiles", {})

    # Create output directory
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    # PFB Matrix mode
    if args.pfb_matrix:
        if not pfb_configs:
            print("ERROR: No pfb_configs defined in perf_cpu_profiles.json")
            sys.exit(1)

        profile_name = args.profile or "cortex-m3"
        if profile_name not in profiles:
            print(f"Unknown profile: {profile_name}")
            sys.exit(1)

        git_commit = get_git_commit()
        profile_config = profiles[profile_name]

        print(f"PFB Matrix Test: profile={profile_name}, {len(pfb_configs)} configs")
        matrix_results = run_pfb_matrix(
            profile_name, profile_config, pfb_configs, timeout
        )

        # Save matrix results
        matrix_data = {
            "timestamp": datetime.now().isoformat(),
            "git_commit": git_commit,
            "profile": profile_name,
            "pfb_matrix": {k: v for k, v in matrix_results.items()},
        }
        with open(PFB_MATRIX_RESULTS_FILE, "w") as f:
            json.dump(matrix_data, f, indent=2)
        print(f"\nMatrix results saved to: {PFB_MATRIX_RESULTS_FILE}")

        # Generate matrix report
        report = generate_pfb_matrix_report(matrix_results, profile_name, git_commit)
        with open(PFB_MATRIX_REPORT_FILE, "w", encoding="utf-8") as f:
            f.write(report)
        print(f"Matrix report saved to: {PFB_MATRIX_REPORT_FILE}")

        print(f"\nPFB Matrix Test Complete: {len(matrix_results)}/{len(pfb_configs)} configs succeeded")
        sys.exit(0 if len(matrix_results) == len(pfb_configs) else 2)

    # SPI Matrix mode
    if args.spi_matrix:
        if not spi_configs:
            print("ERROR: No spi_configs defined in perf_cpu_profiles.json")
            sys.exit(1)

        profile_name = args.profile or "cortex-m3"
        if profile_name not in profiles:
            print(f"Unknown profile: {profile_name}")
            sys.exit(1)

        git_commit = get_git_commit()
        profile_config = profiles[profile_name]

        print(f"SPI Matrix Test: profile={profile_name}, {len(spi_configs)} configs")
        matrix_results = run_spi_matrix(
            profile_name, profile_config, spi_configs, timeout
        )

        # Save matrix results
        matrix_data = {
            "timestamp": datetime.now().isoformat(),
            "git_commit": git_commit,
            "profile": profile_name,
            "spi_matrix": {k: v for k, v in matrix_results.items()},
        }
        with open(SPI_MATRIX_RESULTS_FILE, "w") as f:
            json.dump(matrix_data, f, indent=2)
        print(f"\nMatrix results saved to: {SPI_MATRIX_RESULTS_FILE}")

        # Generate matrix report
        report = generate_spi_matrix_report(matrix_results, profile_name, git_commit)
        with open(SPI_MATRIX_REPORT_FILE, "w", encoding="utf-8") as f:
            f.write(report)
        print(f"Matrix report saved to: {SPI_MATRIX_REPORT_FILE}")

        print(f"\nSPI Matrix Test Complete: {len(matrix_results)}/{len(spi_configs)} configs succeeded")
        sys.exit(0 if len(matrix_results) == len(spi_configs) else 2)

    git_commit = get_git_commit()
    all_results = {}
    all_regressions = []
    failed_profiles = []

    # Enable all image alpha tests for QEMU (no flash size limit)
    original_config = read_app_config()
    patched_config = patch_app_config_image_tests(original_config)
    write_app_config(patched_config)
    print("Enabled all image alpha test cases for QEMU")

    total = len(run_profiles)
    all_skipped = set()
    for idx, (name, config) in enumerate(run_profiles.items(), 1):
        print(f"\n[{idx}/{total}] Profile: {name} - {config['description']}")

        # Build
        if not build_for_profile(name, config, clean=args.clean):
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

        # Compare with baseline
        if name in baseline:
            regs = compare_with_baseline(results, baseline[name], threshold)
            if regs:
                all_regressions.extend([(name, r) for r in regs])
                print(f"  REGRESSIONS: {', '.join(regs)}")
            else:
                print(f"  No regressions")

        all_results[name] = results

    # Restore original config
    write_app_config(original_config)
    print("Restored original app_egui_config.h")

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

    # Generate Markdown report
    report = generate_markdown_report(
        all_results, baseline, threshold, git_commit, skipped=all_skipped
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
    print(f"  Regressions: {len(all_regressions)}")
    for profile, test in all_regressions:
        r = all_results[profile][test]
        print(f"    [{profile}] {test}: {r['baseline_ms']:.1f}ms -> {r['time_ms']:.1f}ms ({r['change_pct']:+.1f}%)")

    # Update baseline if requested
    if args.update_baseline:
        with open(BASELINE_FILE, "w") as f:
            json.dump(results_data, f, indent=2)
        print(f"\nBaseline updated: {BASELINE_FILE}")

    # Exit code
    if all_regressions:
        print(f"\nFAILED: {len(all_regressions)} regression(s) detected")
        sys.exit(1)
    elif failed_profiles:
        print(f"\nWARNING: {len(failed_profiles)} profile(s) failed to build/run")
        sys.exit(2)
    else:
        print(f"\nPASSED: No regressions detected")
        sys.exit(0)


if __name__ == "__main__":
    main()
