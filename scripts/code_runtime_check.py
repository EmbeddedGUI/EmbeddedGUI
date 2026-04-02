#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Runtime verification for EmbeddedGUI examples.
Tests that applications run correctly and produce visual output.
"""

import os
import sys
import subprocess
import platform
import argparse
import shutil
from pathlib import Path

SCREENSHOT_DIR = "runtime_check_output"
DEFAULT_TIMEOUT = 15
RECORDING_FPS = 2
# Safety timeout duration (seconds). Apps auto-quit after all actions complete,
# so this is just a fallback. Set generous enough for worst case.
RECORDING_DURATION = 30
# Speed multiplier for action intervals (1 = normal speed)
# With snapshot-driven recording, actions auto-quit after completion,
# so speed acceleration is no longer needed.
RECORDING_SPEED = 1
RECORDING_CLOCK_SCALE = 6
RECORDING_SNAPSHOT_SETTLE_MS = 0
RECORDING_SNAPSHOT_STABLE_CYCLES = 1
RECORDING_SNAPSHOT_MAX_WAIT_MS = 1500
SLOW_HOST_RETRY_SNAPSHOT_SETTLE_MS = 120
SLOW_HOST_RETRY_SNAPSHOT_STABLE_CYCLES = 2
SLOW_HOST_RETRY_SNAPSHOT_MAX_WAIT_MS = 3000
SLOW_HOST_RETRY_TIMEOUT_MARGIN = 10
# Speed up compilation: disable debug symbols, use -O0 (same as ui_designer)
COMPILE_FAST_FLAGS = ['COMPILE_DEBUG=', 'COMPILE_OPT_LEVEL=-O0']
# Retry count for intermittent crashes (e.g., race conditions in PC simulator threading)
RUN_RETRY_COUNT = 2
RUNTIME_FAIL_MARKERS = ("[RUNTIME_CHECK_FAIL]",)
FULL_CHECK_OPTIONAL_APPS = {
    "HelloCustomWidgets": "requested by --skip-custom-widgets",
}

# Examples not suitable for runtime testing (headless/performance/test-only)
SKIP_LIST = ["HelloUnitTest", "HelloTest", "HelloPerformance", "HelloPerformance",
             "HelloDesigner", "HelloDesigner_temp"]
SUB_APP_ROOTS = {
    "HelloBasic": "example/HelloBasic",
    "HelloVirtual": "example/HelloVirtual",
    "HelloCustomWidgets": "example/HelloCustomWidgets",
}

WINDOWS_RUNTIME_SKIP_SET = {
}


def get_windows_hidden_run_kwargs():
    if platform.system() != 'Windows':
        return {}

    # SDL recording on Windows can hang indefinitely when launched with
    # CREATE_NO_WINDOW / SW_HIDE. Keep the default spawn behavior so CI/local
    # runtime checks remain stable.
    return {}


def build_recording_cmd(exe_path, resource_path, frames_dir, duration, speed, snapshot_settle_ms, clock_scale, snapshot_stable_cycles, snapshot_max_wait_ms):
    return [
        str(exe_path),
        str(resource_path),
        '--record',
        str(frames_dir),
        str(RECORDING_FPS),
        str(duration),
        '--speed',
        str(speed),
        '--clock-scale',
        str(clock_scale),
        '--snapshot-settle-ms',
        str(snapshot_settle_ms),
        '--snapshot-stable-cycles',
        str(snapshot_stable_cycles),
        '--snapshot-max-wait-ms',
        str(snapshot_max_wait_ms),
        '--headless',
    ]


def build_recording_profiles(timeout, duration, speed, snapshot_settle_ms, clock_scale, snapshot_stable_cycles, snapshot_max_wait_ms):
    default_profile = {
        "label": "default",
        "timeout": timeout,
        "duration": duration,
        "speed": speed,
        "snapshot_settle_ms": snapshot_settle_ms,
        "clock_scale": clock_scale,
        "snapshot_stable_cycles": snapshot_stable_cycles,
        "snapshot_max_wait_ms": snapshot_max_wait_ms,
    }
    conservative_profile = {
        "label": "slow-host",
        "timeout": max(timeout, duration + SLOW_HOST_RETRY_TIMEOUT_MARGIN),
        "duration": duration,
        "speed": speed,
        "snapshot_settle_ms": max(snapshot_settle_ms, SLOW_HOST_RETRY_SNAPSHOT_SETTLE_MS),
        "clock_scale": clock_scale,
        "snapshot_stable_cycles": max(snapshot_stable_cycles, SLOW_HOST_RETRY_SNAPSHOT_STABLE_CYCLES),
        "snapshot_max_wait_ms": max(snapshot_max_wait_ms, SLOW_HOST_RETRY_SNAPSHOT_MAX_WAIT_MS),
    }

    comparable_keys = ("timeout", "duration", "speed", "snapshot_settle_ms", "clock_scale", "snapshot_stable_cycles", "snapshot_max_wait_ms")
    if all(conservative_profile[key] == default_profile[key] for key in comparable_keys):
        return [default_profile]

    return [default_profile, conservative_profile]


def validate_recording_frames(frames_dir):
    frame_files = sorted(Path(frames_dir).glob("frame_*.png"))
    if not frame_files:
        return False, "no frames generated"

    for frame_path in frame_files:
        size = frame_path.stat().st_size
        if size < 100:
            return False, "frame %s too small (%d bytes)" % (frame_path.name, size)

    return True, "%d frames captured -> %s" % (len(frame_files), frames_dir)


def run_recording_capture(exe_path, resource_path, frames_dir, timeout, duration, speed,
                          snapshot_settle_ms, clock_scale, snapshot_stable_cycles,
                          snapshot_max_wait_ms):
    frames_dir = Path(frames_dir)
    frames_dir.mkdir(parents=True, exist_ok=True)
    hidden_kwargs = get_windows_hidden_run_kwargs()
    profiles = build_recording_profiles(timeout, duration, speed, snapshot_settle_ms,
                                        clock_scale, snapshot_stable_cycles,
                                        snapshot_max_wait_ms)
    last_error = ""

    for profile_index, profile in enumerate(profiles):
        cmd = build_recording_cmd(
            exe_path,
            resource_path,
            frames_dir,
            profile["duration"],
            profile["speed"],
            profile["snapshot_settle_ms"],
            profile["clock_scale"],
            profile["snapshot_stable_cycles"],
            profile["snapshot_max_wait_ms"],
        )

        for old_frame in frames_dir.glob("frame_*.*"):
            old_frame.unlink()

        for attempt in range(RUN_RETRY_COUNT):
            try:
                result = subprocess.run(cmd, timeout=profile["timeout"], capture_output=True, text=True, **hidden_kwargs)
                if result.returncode != 0:
                    stderr_msg = result.stderr.strip() if result.stderr else ""
                    last_error = "exit code %d%s" % (result.returncode, (": " + stderr_msg) if stderr_msg else "")
                    if attempt < RUN_RETRY_COUNT - 1:
                        print("(retry %d/%d: %s)" % (attempt + 1, RUN_RETRY_COUNT - 1, last_error), end=" ")
                        continue
                    break
            except subprocess.TimeoutExpired:
                last_error = "timeout after %ds" % profile["timeout"]
                break
            except Exception as e:
                last_error = "run error: %s" % str(e)
                break

            combined_output = "%s\n%s" % ((result.stdout or ""), (result.stderr or ""))
            marker_line = ""
            for marker in RUNTIME_FAIL_MARKERS:
                if marker in combined_output:
                    marker_line = marker
                    for line in combined_output.splitlines():
                        if marker in line:
                            marker_line = line.strip()
                            break
                    last_error = "runtime self-check failed: %s" % marker_line
                    break

            if marker_line:
                break

            frames_ok, frames_message = validate_recording_frames(frames_dir)
            if frames_ok:
                return True, frames_message

            last_error = frames_message
            break

        if profile_index < len(profiles) - 1:
            print("(retry %s mode: %s)" % (profiles[profile_index + 1]["label"], last_error), end=" ")

    return False, last_error


def get_example_list():
    """Get list of all example applications"""
    path = 'example'
    app_list = []
    if not os.path.exists(path):
        return app_list

    for file in os.listdir(path):
        file_path = os.path.join(path, file)
        if os.path.isdir(file_path):
            app_list.append(file)

    return sorted(app_list)


def get_example_sub_list(app):
    """Get list of sub-applications for apps that use APP_SUB."""
    path = SUB_APP_ROOTS.get(app)
    app_list = []
    if not path or not os.path.exists(path):
        return app_list

    if app == "HelloCustomWidgets":
        for root, dirs, files in os.walk(path):
            dirs[:] = [name for name in dirs if not name.startswith('.') and not name.startswith('__')]
            if 'test.c' not in files:
                continue

            rel_path = os.path.relpath(root, path).replace('\\', '/')
            if rel_path == '.':
                continue
            app_list.append(rel_path)

        return sorted(app_list)

    for file in os.listdir(path):
        file_path = os.path.join(path, file)
        if os.path.isdir(file_path):
            app_list.append(file)

    return sorted(app_list)


def filter_sub_apps(app, app_subs, category=None):
    """Filter sub-app list for app-specific grouping."""
    if not category or app != "HelloCustomWidgets":
        return sorted(app_subs)

    prefix = category.strip().strip('/\\')
    if not prefix:
        return sorted(app_subs)

    prefix = prefix.replace('\\', '/')
    return sorted([app_sub for app_sub in app_subs if app_sub.startswith(prefix + '/')])


def format_app_name(app, app_sub=None):
    """Build a stable runtime output name for app/app_sub."""
    if not app_sub:
        return app

    normalized_sub = app_sub.replace('\\', '_').replace('/', '_')
    return "%s_%s" % (app, normalized_sub)


def get_runtime_skip_reason(app, app_sub=None):
    """Return a skip reason for known platform-specific runtime issues."""
    if platform.system() == 'Windows' and (app, app_sub) in WINDOWS_RUNTIME_SKIP_SET:
        return "known Windows subprocess recording hang"
    return None


def compile_app(app, app_sub=None, bits64=False, user_cflags="", recording_test=True):
    """Compile application with optional CFLAGS override.
    Uses per-app OBJDIR (no make clean needed).
    HelloBasic/HelloVirtual sub-apps use per-sub-app OBJDIR.
    Returns True on success, False on failure.
    """
    # Clean before build to ensure fresh compilation with correct config.
    # Different sub-apps have different app_egui_config.h, and recording
    # needs a guaranteed correct executable.
    subprocess.run(['make', 'clean'], capture_output=True)

    # Build — always inject RECORDING_TEST since make clean guarantees fresh compilation
    recording_flag = '-DEGUI_CONFIG_RECORDING_TEST=%d' % (1 if recording_test else 0)
    combined_cflags = ('%s %s' % (recording_flag, user_cflags)).strip()
    cmd = ['make', '-j', 'APP=%s' % app, 'PORT=pc'] + COMPILE_FAST_FLAGS
    if app_sub:
        cmd.append('APP_SUB=%s' % app_sub)
    if bits64:
        cmd.append('BITS=64')
    cmd.append('USER_CFLAGS=%s' % combined_cflags)

    result = subprocess.run(cmd, capture_output=True)
    return result.returncode == 0


def run_app(app_name, output_subdir, timeout=DEFAULT_TIMEOUT, duration=RECORDING_DURATION,
            speed=RECORDING_SPEED, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
            clock_scale=RECORDING_CLOCK_SCALE,
            snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
            snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS):
    """Run app with recording mode, capture screenshot, verify exit.
    Retries on crash (access violation) since PC simulator has known
    race conditions between main thread and egui thread.
    Returns (success: bool, message: str).
    """
    frames_dir = os.path.join(SCREENSHOT_DIR, app_name, output_subdir)
    os.makedirs(frames_dir, exist_ok=True)

    root_dir = Path.cwd()

    if platform.system() == 'Windows':
        exe_path = root_dir / 'output' / 'main.exe'
    else:
        exe_path = root_dir / 'output' / 'main'

    resource_path = root_dir / 'output' / 'app_egui_resource_merge.bin'

    if not exe_path.exists():
        return False, "executable not found"

    return run_recording_capture(exe_path, resource_path, frames_dir, timeout, duration,
                                 speed, snapshot_settle_ms, clock_scale,
                                 snapshot_stable_cycles, snapshot_max_wait_ms)


def check_default_resolution(app, app_sub, bits64, explicit_timeout=None,
                             speed=RECORDING_SPEED, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
                             clock_scale=RECORDING_CLOCK_SCALE,
                             snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
                             snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS):
    """Test app at default resolution (no CFLAGS override).
    Apps auto-quit after all actions complete; RECORDING_DURATION is a safety timeout.
    Returns (success: bool, message: str).
    """
    app_name = format_app_name(app, app_sub)
    skip_reason = get_runtime_skip_reason(app, app_sub)

    if skip_reason:
        return True, "skipped: %s" % skip_reason

    # Compile with default settings
    if not compile_app(app, app_sub, bits64):
        return False, "compile failed"

    # Timeout must be >= recording duration + margin to allow auto-quit
    timeout = max(RECORDING_DURATION + 5, explicit_timeout if explicit_timeout is not None else DEFAULT_TIMEOUT)
    return run_app(app_name, "default", timeout=timeout, duration=RECORDING_DURATION,
                   speed=speed, snapshot_settle_ms=snapshot_settle_ms,
                   clock_scale=clock_scale,
                   snapshot_stable_cycles=snapshot_stable_cycles,
                   snapshot_max_wait_ms=snapshot_max_wait_ms)



def capture_animation_frames(app_name, output_dir, fps=10, duration=5,
                             speed=1, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
                             clock_scale=RECORDING_CLOCK_SCALE,
                             snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
                             snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS):
    """Capture animation frames at higher FPS for regression verification.

    This is the programmatic API used by the external Designer regression tooling.
    Captures at higher FPS to get animation keyframes (0%, 50%, 100%).

    Args:
        app_name: App name (e.g., "HelloBattery")
        output_dir: Directory to write frame PNGs
        fps: Frames per second (default 10 for animation capture)
        duration: Recording duration in seconds
        speed: Speed multiplier (1 = normal speed for accurate animation timing)

    Returns:
        (success: bool, frames: list[str], message: str)
    """
    os.makedirs(output_dir, exist_ok=True)

    root_dir = Path.cwd()

    if platform.system() == 'Windows':
        exe_path = root_dir / 'output' / 'main.exe'
    else:
        exe_path = root_dir / 'output' / 'main'

    resource_path = root_dir / 'output' / 'app_egui_resource_merge.bin'

    if not exe_path.exists():
        return False, [], "executable not found"

    # Clean old frames
    for old_frame in Path(output_dir).glob("frame_*.*"):
        old_frame.unlink()

    cmd = [str(exe_path), str(resource_path), '--record', output_dir,
           str(fps), str(duration), '--speed', str(speed),
           '--clock-scale', str(clock_scale),
           '--snapshot-settle-ms', str(snapshot_settle_ms),
           '--snapshot-stable-cycles', str(snapshot_stable_cycles),
           '--snapshot-max-wait-ms', str(snapshot_max_wait_ms), '--headless']

    try:
        result = subprocess.run(cmd, timeout=duration + 10,
                                capture_output=True, text=True, **get_windows_hidden_run_kwargs())
        if result.returncode != 0:
            stderr_msg = result.stderr.strip() if result.stderr else ""
            return False, [], "exit code %d: %s" % (result.returncode, stderr_msg)
    except subprocess.TimeoutExpired:
        return False, [], "timeout after %ds" % (duration + 10)
    except Exception as e:
        return False, [], "run error: %s" % str(e)

    # Collect frames
    frame_files = sorted(str(f) for f in Path(output_dir).glob("frame_*.png"))
    if not frame_files:
        return False, [], "no frames generated"

    return True, frame_files, "%d frames at %d fps" % (len(frame_files), fps)


def extract_keyframes(frame_files, keyframe_pcts=None):
    """Extract keyframes at specified percentages from a frame list.

    Args:
        frame_files: Sorted list of frame file paths
        keyframe_pcts: List of percentages [0, 50, 100] (default)

    Returns:
        dict: {pct: frame_path} e.g., {0: "frame_0000.png", 50: "frame_0025.png", 100: "frame_0049.png"}
    """
    if keyframe_pcts is None:
        keyframe_pcts = [0, 50, 100]

    if not frame_files:
        return {}

    n = len(frame_files)
    result = {}
    for pct in keyframe_pcts:
        idx = min(int(pct / 100.0 * (n - 1)), n - 1)
        result[pct] = frame_files[idx]

    return result


def run_full_check(bits64, speed=RECORDING_SPEED, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
                   clock_scale=RECORDING_CLOCK_SCALE,
                   snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
                   snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS,
                   skip_custom_widgets=False):
    """Run runtime check on all examples.
    Returns list of (app_name, success, message) tuples.
    """
    results = []
    app_sets = get_example_list()
    sub_app_sets = {app: get_example_sub_list(app) for app in SUB_APP_ROOTS}
    skipped_apps = set()

    if skip_custom_widgets:
        skipped_apps.add("HelloCustomWidgets")

    print("Running with speed=%dx" % speed)

    # Calculate total
    total = 0
    for app in app_sets:
        if app in SKIP_LIST or app in skipped_apps:
            continue
        if app in SUB_APP_ROOTS:
            total += len(sub_app_sets.get(app, []))
        else:
            total += 1

    current = 0
    for app in app_sets:
        if app in SKIP_LIST:
            print("\nSkipping: %s" % app)
            continue
        if app in skipped_apps:
            print("\nSkipping: %s (%s)" % (app, FULL_CHECK_OPTIONAL_APPS[app]))
            continue

        if app in SUB_APP_ROOTS:
            for app_sub in sub_app_sets.get(app, []):
                current += 1
                app_name = format_app_name(app, app_sub)
                print("\n" + "=" * 60)
                print("[%d/%d] %s" % (current, total, app_name))
                print("=" * 60)

                success, msg = check_default_resolution(
                    app,
                    app_sub,
                    bits64,
                    speed=speed,
                    snapshot_settle_ms=snapshot_settle_ms,
                    clock_scale=clock_scale,
                    snapshot_stable_cycles=snapshot_stable_cycles,
                    snapshot_max_wait_ms=snapshot_max_wait_ms,
                )
                status = "PASS" if success else "FAIL"
                print("  %s (%s)" % (status, msg))
                results.append((app_name, success, msg))
        else:
            current += 1
            print("\n" + "=" * 60)
            print("[%d/%d] %s" % (current, total, app))
            print("=" * 60)

            success, msg = check_default_resolution(
                app,
                None,
                bits64,
                speed=speed,
                snapshot_settle_ms=snapshot_settle_ms,
                clock_scale=clock_scale,
                snapshot_stable_cycles=snapshot_stable_cycles,
                snapshot_max_wait_ms=snapshot_max_wait_ms,
            )
            status = "PASS" if success else "FAIL"
            print("  %s (%s)" % (status, msg))
            results.append((app, success, msg))

    return results


def run_sub_app_check(app, app_subs, bits64, explicit_timeout=None,
                      speed=RECORDING_SPEED, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
                      clock_scale=RECORDING_CLOCK_SCALE,
                      snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
                      snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS):
    """Run runtime check for a batch of APP_SUB examples under one app."""
    results = []
    total = len(app_subs)
    for index, app_sub in enumerate(app_subs, start=1):
        app_name = format_app_name(app, app_sub)
        print("\n" + "=" * 60)
        print("[%d/%d] %s (speed=%dx)" % (index, total, app_name, speed))
        print("=" * 60)

        success, msg = check_default_resolution(
            app,
            app_sub,
            bits64,
            explicit_timeout=explicit_timeout,
            speed=speed,
            snapshot_settle_ms=snapshot_settle_ms,
            clock_scale=clock_scale,
            snapshot_stable_cycles=snapshot_stable_cycles,
            snapshot_max_wait_ms=snapshot_max_wait_ms,
        )
        results.append((app_name, success, msg))

    return results


def print_summary(results):
    """Print test result summary"""
    print("\n" + "=" * 60)
    print("RUNTIME CHECK SUMMARY")
    print("=" * 60)

    passed = 0
    failed = 0
    for name, success, msg in results:
        if success:
            print("  [PASS] %s - %s" % (name, msg))
            passed += 1
        else:
            print("  [FAIL] %s - %s" % (name, msg))
            failed += 1

    print("-" * 60)
    print("Total: %d passed, %d failed, %d total" % (passed, failed, passed + failed))

    if failed > 0:
        print("Result: FAILED")
    else:
        print("Result: ALL PASSED")

    print("=" * 60)
    return failed == 0


def main():
    parser = argparse.ArgumentParser(
        description='Runtime verification for EmbeddedGUI examples',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s --app HelloSimple                          Test single app
  %(prog)s --app HelloVirtual                        Test all HelloVirtual sub-apps
  %(prog)s --app HelloBasic --app-sub button         Test one HelloBasic sub-app
  %(prog)s --app HelloVirtual --app-sub virtual_grid Test one HelloVirtual sub-app
  %(prog)s --app HelloCustomWidgets --category input Test HelloCustomWidgets category
  %(prog)s --full-check --skip-custom-widgets        Test all examples except HelloCustomWidgets
  %(prog)s --full-check                             Test all examples
        """
    )
    parser.add_argument('--app', type=str,
                        help='Specific app to test. For HelloBasic/HelloVirtual/HelloCustomWidgets without --app-sub, tests all sub-apps.')
    parser.add_argument('--app-sub', type=str,
                        help='Single sub-app for HelloBasic/HelloVirtual/HelloCustomWidgets. If omitted, all sub-apps are tested.')
    parser.add_argument('--category', type=str, help='HelloCustomWidgets category filter (e.g. input)')
    parser.add_argument('--bits64', action='store_true', help='Build for 64-bit')
    parser.add_argument('--timeout', type=int, default=DEFAULT_TIMEOUT,
                        help='Run timeout in seconds (default: %d)' % DEFAULT_TIMEOUT)
    parser.add_argument('--keep-screenshots', action='store_true',
                        help='Keep screenshot files after testing')
    parser.add_argument('--full-check', action='store_true',
                        help='Test all example applications')
    parser.add_argument('--skip-custom-widgets', action='store_true',
                        help='Exclude HelloCustomWidgets from --full-check to keep CI/runtime sweeps shorter')
    parser.add_argument('--speed', type=int, default=RECORDING_SPEED,
                        help='Action speed multiplier (default: %d, 1=normal)' % RECORDING_SPEED)
    parser.add_argument('--clock-scale', type=int, default=RECORDING_CLOCK_SCALE,
                        help='Recording clock acceleration factor (default: %d)' % RECORDING_CLOCK_SCALE)
    parser.add_argument('--snapshot-settle-ms', type=int, default=RECORDING_SNAPSHOT_SETTLE_MS,
                        help='Snapshot settle time in ms (default: %d)' % RECORDING_SNAPSHOT_SETTLE_MS)
    parser.add_argument('--snapshot-stable-cycles', type=int, default=RECORDING_SNAPSHOT_STABLE_CYCLES,
                        help='Required unchanged frame cycles before snapshot (default: %d)' % RECORDING_SNAPSHOT_STABLE_CYCLES)
    parser.add_argument('--snapshot-max-wait-ms', type=int, default=RECORDING_SNAPSHOT_MAX_WAIT_MS,
                        help='Snapshot force-capture timeout in ms (default: %d)' % RECORDING_SNAPSHOT_MAX_WAIT_MS)

    args = parser.parse_args()

    all_passed = True
    speed = args.speed
    clock_scale = args.clock_scale
    snapshot_settle_ms = args.snapshot_settle_ms
    snapshot_stable_cycles = args.snapshot_stable_cycles
    snapshot_max_wait_ms = args.snapshot_max_wait_ms

    if args.full_check:
        results = run_full_check(
            args.bits64,
            speed=speed,
            clock_scale=clock_scale,
            snapshot_settle_ms=snapshot_settle_ms,
            snapshot_stable_cycles=snapshot_stable_cycles,
            snapshot_max_wait_ms=snapshot_max_wait_ms,
            skip_custom_widgets=args.skip_custom_widgets,
        )
        all_passed = print_summary(results)

    elif args.app:
        if args.category and args.app != "HelloCustomWidgets":
            print("Error: --category is only supported with --app HelloCustomWidgets")
            sys.exit(1)

        results = []
        run_all_sub_apps = args.app in SUB_APP_ROOTS and not args.app_sub
        if run_all_sub_apps:
            sub_apps = filter_sub_apps(args.app, get_example_sub_list(args.app), args.category)
            if not sub_apps:
                category_suffix = " (category=%s)" % args.category if args.category else ""
                print("Error: no %s sub-apps found%s" % (args.app, category_suffix))
                sys.exit(1)

            print("=" * 60)
            if args.category:
                print("Runtime Check: %s category=%s (speed=%dx)" % (args.app, args.category, speed))
            else:
                print("Runtime Check: %s all sub-apps (speed=%dx)" % (args.app, speed))
            print("=" * 60)

            results = run_sub_app_check(
                args.app,
                sub_apps,
                args.bits64,
                explicit_timeout=args.timeout,
                speed=speed,
                clock_scale=clock_scale,
                snapshot_settle_ms=snapshot_settle_ms,
                snapshot_stable_cycles=snapshot_stable_cycles,
                snapshot_max_wait_ms=snapshot_max_wait_ms,
            )
        else:
            app_name = format_app_name(args.app, args.app_sub)
            print("=" * 60)
            print("Runtime Check: %s (speed=%dx)" % (app_name, speed))
            print("=" * 60)

            success, msg = check_default_resolution(args.app, args.app_sub, args.bits64,
                                                     explicit_timeout=args.timeout,
                                                     speed=speed,
                                                     clock_scale=clock_scale,
                                                     snapshot_settle_ms=snapshot_settle_ms,
                                                     snapshot_stable_cycles=snapshot_stable_cycles,
                                                     snapshot_max_wait_ms=snapshot_max_wait_ms)
            results.append((app_name, success, msg))

        all_passed = print_summary(results)

        # Print screenshot locations for AI visual verification
        print("\nScreenshot directories for visual verification:")
        for result_name, success, _ in results:
            screenshot_base = os.path.join(SCREENSHOT_DIR, result_name)
            if not success or not os.path.exists(screenshot_base):
                continue
            print("  %s" % screenshot_base)

    else:
        parser.print_help()
        sys.exit(0)

    # Cleanup if not keeping screenshots
    if not args.keep_screenshots and not all_passed:
        # Keep screenshots on failure for debugging
        print("\nScreenshots retained for debugging (use --keep-screenshots to always keep)")

    sys.exit(0 if all_passed else 1)


if __name__ == '__main__':
    main()
