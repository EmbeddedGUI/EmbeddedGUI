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
# Speed up compilation: disable debug symbols, use -O0 (same as ui_designer)
COMPILE_FAST_FLAGS = ['COMPILE_DEBUG=', 'COMPILE_OPT_LEVEL=-O0']
# Retry count for intermittent crashes (e.g., race conditions in PC simulator threading)
RUN_RETRY_COUNT = 2

# Examples not suitable for runtime testing (headless/performance/test-only)
SKIP_LIST = ["HelloUnitTest", "HelloTest", "HelloPerformace", "HelloPerformance",
             "HelloDesigner", "HelloDesigner_temp"]


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


def get_example_basic_list():
    """Get list of HelloBasic sub-applications"""
    path = 'example/HelloBasic'
    app_list = []
    if not os.path.exists(path):
        return app_list

    for file in os.listdir(path):
        file_path = os.path.join(path, file)
        if os.path.isdir(file_path):
            app_list.append(file)

    return sorted(app_list)


def compile_app(app, app_sub=None, bits64=False, user_cflags=""):
    """Compile application with optional CFLAGS override.
    Uses per-app OBJDIR (no make clean needed).
    HelloBasic sub-apps use per-sub-app OBJDIR (APP_OBJ_SUFFIX=HelloBasic_<sub>).
    Returns True on success, False on failure.
    """
    # Clean before build to ensure fresh compilation with correct config.
    # Different sub-apps have different app_egui_config.h, and recording
    # needs a guaranteed correct executable.
    subprocess.run(['make', 'clean'], capture_output=True)

    # Build — always inject RECORDING_TEST since make clean guarantees fresh compilation
    recording_flag = '-DEGUI_CONFIG_RECORDING_TEST=1'
    combined_cflags = ('%s %s' % (recording_flag, user_cflags)).strip()
    cmd = ['make', '-j', 'APP=%s' % app, 'PORT=pc'] + COMPILE_FAST_FLAGS
    if app_sub:
        cmd.append('APP_SUB=%s' % app_sub)
    if bits64:
        cmd.append('BITS=64')
    cmd.append('USER_CFLAGS=%s' % combined_cflags)

    result = subprocess.run(cmd, capture_output=True)
    return result.returncode == 0


def run_app(app_name, output_subdir, timeout=DEFAULT_TIMEOUT, duration=RECORDING_DURATION, speed=RECORDING_SPEED):
    """Run app with recording mode, capture screenshot, verify exit.
    Retries on crash (access violation) since PC simulator has known
    race conditions between main thread and egui thread.
    Returns (success: bool, message: str).
    """
    frames_dir = os.path.join(SCREENSHOT_DIR, app_name, output_subdir)
    os.makedirs(frames_dir, exist_ok=True)

    if platform.system() == 'Windows':
        exe_path = 'output\\main.exe'
    else:
        exe_path = './output/main'

    resource_path = os.path.join('output', 'app_egui_resource_merge.bin')

    if not os.path.exists(exe_path):
        return False, "executable not found"

    # Run with recording: RECORDING_FPS fps, duration seconds, speed multiplier
    cmd = [exe_path, resource_path, '--record', frames_dir,
           str(RECORDING_FPS), str(duration), '--speed', str(speed)]

    last_error = ""
    for attempt in range(RUN_RETRY_COUNT):
        # Clean old frames before each attempt
        for old_frame in Path(frames_dir).glob("frame_*.*"):
            old_frame.unlink()

        try:
            result = subprocess.run(cmd, timeout=timeout, capture_output=True, text=True)
            if result.returncode != 0:
                stderr_msg = result.stderr.strip() if result.stderr else ""
                last_error = "exit code %d%s" % (result.returncode,
                                                  (": " + stderr_msg) if stderr_msg else "")
                if attempt < RUN_RETRY_COUNT - 1:
                    print("(retry %d/%d: %s)" % (attempt + 1, RUN_RETRY_COUNT - 1, last_error), end=" ")
                    continue
                return False, last_error
        except subprocess.TimeoutExpired:
            return False, "timeout after %ds" % timeout
        except Exception as e:
            return False, "run error: %s" % str(e)

        # Success - verify frames
        break

    # Verify frames were generated
    frame_files = sorted(Path(frames_dir).glob("frame_*.png"))
    if not frame_files:
        return False, "no frames generated"

    # Check frame files have reasonable size
    for f in frame_files:
        size = f.stat().st_size
        if size < 100:
            return False, "frame %s too small (%d bytes)" % (f.name, size)

    return True, "%d frames captured -> %s" % (len(frame_files), frames_dir)


def check_default_resolution(app, app_sub, bits64, explicit_timeout=None, speed=RECORDING_SPEED):
    """Test app at default resolution (no CFLAGS override).
    Apps auto-quit after all actions complete; RECORDING_DURATION is a safety timeout.
    Returns (success: bool, message: str).
    """
    app_name = "%s_%s" % (app, app_sub) if app_sub else app

    # Compile with default settings
    if not compile_app(app, app_sub, bits64):
        return False, "compile failed"

    # Timeout must be >= recording duration + margin to allow auto-quit
    timeout = max(RECORDING_DURATION + 5, explicit_timeout if explicit_timeout is not None else DEFAULT_TIMEOUT)
    return run_app(app_name, "default", timeout=timeout, duration=RECORDING_DURATION, speed=speed)



def capture_animation_frames(app_name, output_dir, fps=10, duration=5, speed=1):
    """Capture animation frames at higher FPS for regression verification.

    This is the programmatic API used by figmamake_regression.py.
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

    if platform.system() == 'Windows':
        exe_path = 'output\\main.exe'
    else:
        exe_path = './output/main'

    resource_path = os.path.join('output', 'app_egui_resource_merge.bin')

    if not os.path.exists(exe_path):
        return False, [], "executable not found"

    # Clean old frames
    for old_frame in Path(output_dir).glob("frame_*.*"):
        old_frame.unlink()

    cmd = [exe_path, resource_path, '--record', output_dir,
           str(fps), str(duration), '--speed', str(speed)]

    try:
        result = subprocess.run(cmd, timeout=duration + 10,
                                capture_output=True, text=True)
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


def run_full_check(bits64, speed=RECORDING_SPEED):
    """Run runtime check on all examples.
    Returns list of (app_name, success, message) tuples.
    """
    results = []
    app_sets = get_example_list()
    app_basic_sets = get_example_basic_list()

    print("Running with speed=%dx" % speed)

    # Calculate total
    total = 0
    for app in app_sets:
        if app in SKIP_LIST:
            continue
        if app == "HelloBasic":
            total += len(app_basic_sets)
        else:
            total += 1

    current = 0
    for app in app_sets:
        if app in SKIP_LIST:
            print("\nSkipping: %s" % app)
            continue

        if app == "HelloBasic":
            for app_sub in app_basic_sets:
                current += 1
                app_name = "HelloBasic_%s" % app_sub
                print("\n" + "=" * 60)
                print("[%d/%d] %s" % (current, total, app_name))
                print("=" * 60)

                success, msg = check_default_resolution(app, app_sub, bits64, speed=speed)
                status = "PASS" if success else "FAIL"
                print("  %s (%s)" % (status, msg))
                results.append((app_name, success, msg))
        else:
            current += 1
            print("\n" + "=" * 60)
            print("[%d/%d] %s" % (current, total, app))
            print("=" * 60)

            success, msg = check_default_resolution(app, None, bits64, speed=speed)
            status = "PASS" if success else "FAIL"
            print("  %s (%s)" % (status, msg))
            results.append((app, success, msg))

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
  %(prog)s --app HelloSimple                 Test single app
  %(prog)s --app HelloBasic --app-sub button Test HelloBasic sub-app
  %(prog)s --full-check                      Test all examples
        """
    )
    parser.add_argument('--app', type=str, help='Specific app to test')
    parser.add_argument('--app-sub', type=str, help='Sub-app for HelloBasic')
    parser.add_argument('--bits64', action='store_true', help='Build for 64-bit')
    parser.add_argument('--timeout', type=int, default=DEFAULT_TIMEOUT,
                        help='Run timeout in seconds (default: %d)' % DEFAULT_TIMEOUT)
    parser.add_argument('--keep-screenshots', action='store_true',
                        help='Keep screenshot files after testing')
    parser.add_argument('--full-check', action='store_true',
                        help='Test all example applications')
    parser.add_argument('--speed', type=int, default=RECORDING_SPEED,
                        help='Action speed multiplier (default: %d, 1=normal)' % RECORDING_SPEED)

    args = parser.parse_args()

    all_passed = True
    speed = args.speed

    if args.full_check:
        results = run_full_check(args.bits64, speed=speed)
        all_passed = print_summary(results)

    elif args.app:
        app_name = "%s_%s" % (args.app, args.app_sub) if args.app_sub else args.app
        print("=" * 60)
        print("Runtime Check: %s (speed=%dx)" % (app_name, speed))
        print("=" * 60)

        results = []

        success, msg = check_default_resolution(args.app, args.app_sub, args.bits64,
                                                 explicit_timeout=args.timeout,
                                                 speed=speed)
        results.append((app_name, success, msg))

        all_passed = print_summary(results)

        # Print screenshot locations for AI visual verification
        screenshot_base = os.path.join(SCREENSHOT_DIR, app_name)
        if os.path.exists(screenshot_base):
            print("\nScreenshots saved for visual verification:")
            for root, dirs, files in os.walk(screenshot_base):
                for f in sorted(files):
                    if f.endswith('.png'):
                        print("  %s" % os.path.join(root, f))

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
