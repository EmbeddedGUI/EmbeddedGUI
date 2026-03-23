#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
GIF Recorder for EmbeddedGUI examples
Automatically records GIF demos for each example application
"""

import os
import sys
import subprocess
import shutil
import argparse
import platform
from pathlib import Path

# Configuration
DEFAULT_FPS = 10
DEFAULT_DURATION = 5  # seconds
OUTPUT_GIF_DIR = "doc/source/images/examples"
TEMP_FRAMES_DIR = "output/frames"
DEFAULT_RECORDING_CLOCK_SCALE = 6
DEFAULT_SNAPSHOT_SETTLE_MS = 0
DEFAULT_SNAPSHOT_STABLE_CYCLES = 1
DEFAULT_SNAPSHOT_MAX_WAIT_MS = 1500

# Default skip list - examples not suitable for demo
DEFAULT_SKIP_LIST = ["HelloUnitTest", "HelloTest", "HelloPerformance"]
SUB_APP_ROOTS = {
    "HelloBasic": "example/HelloBasic",
    "HelloVirtual": "example/HelloVirtual",
}


def get_windows_hidden_run_kwargs():
    if platform.system() != 'Windows':
        return {}

    return {}


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

    for file in os.listdir(path):
        file_path = os.path.join(path, file)
        if os.path.isdir(file_path):
            app_list.append(file)

    return sorted(app_list)


def format_app_name(app, app_sub=None):
    """Build a stable output name for app/app_sub."""
    if not app_sub:
        return app
    return f"{app}_{app_sub}"


def compile_app(app, app_sub=None, bits64=False):
    """Compile specified application"""
    print(f"Compiling: {app}" + (f"/{app_sub}" if app_sub else ""))

    # Clean first - need to pass APP to avoid build.mk include error
    cmd = ['make', 'clean', f'APP={app}']
    if app_sub:
        cmd.append(f'APP_SUB={app_sub}')
    result = subprocess.run(cmd, capture_output=True)
    if result.returncode != 0:
        print(f"Clean failed: {result.stderr.decode()}")
        return False

    # Build
    cmd = ['make', '-j']
    cmd.append(f'APP={app}')
    cmd.append('PORT=pc')
    if app_sub:
        cmd.append(f'APP_SUB={app_sub}')
    if bits64:
        cmd.append('BITS=64')

    result = subprocess.run(cmd, capture_output=True)
    if result.returncode != 0:
        print(f"Build failed: {result.stderr.decode()}")
        return False

    return True


def run_and_record(app_name, fps=DEFAULT_FPS, duration=DEFAULT_DURATION,
                   clock_scale=DEFAULT_RECORDING_CLOCK_SCALE,
                   snapshot_settle_ms=DEFAULT_SNAPSHOT_SETTLE_MS,
                   snapshot_stable_cycles=DEFAULT_SNAPSHOT_STABLE_CYCLES,
                   snapshot_max_wait_ms=DEFAULT_SNAPSHOT_MAX_WAIT_MS):
    """Run application and record frames"""
    frames_dir = os.path.join(TEMP_FRAMES_DIR, app_name)
    os.makedirs(frames_dir, exist_ok=True)

    # Determine executable path
    if platform.system() == 'Windows':
        exe_path = 'output/main.exe'
    else:
        exe_path = 'output/main'

    if not os.path.exists(exe_path):
        print(f"Executable not found: {exe_path}")
        return False

    resource_path = 'output/app_egui_resource_merge.bin'

    # Run with recording enabled
    cmd = [
        exe_path,
        resource_path,
        '--record',
        frames_dir,
        str(fps),
        str(duration),
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
    print(f"Running: {' '.join(cmd)}")

    # Set environment for headless display if needed (Linux)
    env = os.environ.copy()
    if platform.system() == 'Linux' and 'DISPLAY' not in env:
        print("Warning: No DISPLAY set. SDL requires a graphical environment.")
        print("Try: export DISPLAY=:0 or run from a desktop session")

    try:
        # Add extra time for startup/shutdown
        timeout_sec = duration + 10
        result = subprocess.run(
            cmd,
            timeout=timeout_sec,
            capture_output=True,
            text=True,
            env=env,
            **get_windows_hidden_run_kwargs(),
        )
        if result.stdout:
            print(result.stdout)
        if result.stderr:
            print(f"stderr: {result.stderr}")
    except subprocess.TimeoutExpired:
        print(f"Warning: {app_name} timeout after {timeout_sec}s")
        return False
    except Exception as e:
        print(f"Error running {app_name}: {e}")
        return False

    # Check if frames were generated
    frame_files = list(Path(frames_dir).glob("frame_*.png"))
    if not frame_files:
        frame_files = list(Path(frames_dir).glob("frame_*.bmp"))
    if not frame_files:
        print(f"No frames generated for {app_name}")
        return False

    print(f"Generated {len(frame_files)} frames")
    return True


def frames_to_gif(app_name, fps=DEFAULT_FPS):
    """Convert frame sequence to GIF using FFmpeg or Pillow"""
    frames_dir = os.path.join(TEMP_FRAMES_DIR, app_name)
    output_gif = os.path.join(OUTPUT_GIF_DIR, f"{app_name}.gif")

    os.makedirs(OUTPUT_GIF_DIR, exist_ok=True)

    # Check if frames exist
    frame_ext = 'png' if os.path.exists(os.path.join(frames_dir, 'frame_0000.png')) else 'bmp'
    frame_pattern = os.path.join(frames_dir, f'frame_%04d.{frame_ext}')
    if not os.path.exists(os.path.join(frames_dir, f'frame_0000.{frame_ext}')):
        print(f"No frames found in {frames_dir}")
        return None

    # Try FFmpeg first (better quality)
    if shutil.which('ffmpeg'):
        print("Using FFmpeg for GIF generation...")
        cmd = [
            'ffmpeg', '-y',
            '-framerate', str(fps),
            '-i', frame_pattern,
            '-vf', f'fps={fps},split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse',
            output_gif
        ]
        try:
            subprocess.run(cmd, check=True, capture_output=True)
            print(f"Generated: {output_gif}")
            return output_gif
        except subprocess.CalledProcessError as e:
            print(f"FFmpeg failed: {e.stderr.decode()}")
            # Fall through to Pillow

    # Fallback to Pillow
    try:
        from PIL import Image
        print("Using Pillow for GIF generation...")

        frames = []
        frame_files = sorted(Path(frames_dir).glob(f"frame_*.{frame_ext}"))

        for frame_file in frame_files:
            img = Image.open(frame_file)
            # Convert to RGB if needed
            if img.mode != 'RGB':
                img = img.convert('RGB')
            frames.append(img)

        if frames:
            # Calculate duration per frame in milliseconds
            duration_per_frame = 1000 // fps
            frames[0].save(
                output_gif,
                save_all=True,
                append_images=frames[1:],
                duration=duration_per_frame,
                loop=0,
                optimize=True
            )
            print(f"Generated: {output_gif}")
            return output_gif
        else:
            print("No frames to convert")
            return None

    except ImportError:
        print("Error: Neither FFmpeg nor Pillow available")
        print("Install FFmpeg or run: pip install Pillow")
        return None


def cleanup_frames(app_name):
    """Remove temporary frame files"""
    frames_dir = os.path.join(TEMP_FRAMES_DIR, app_name)
    if os.path.exists(frames_dir):
        shutil.rmtree(frames_dir, ignore_errors=True)


def record_single_app(app, app_sub=None, fps=DEFAULT_FPS, duration=DEFAULT_DURATION,
                      bits64=False, keep_frames=False,
                      clock_scale=DEFAULT_RECORDING_CLOCK_SCALE,
                      snapshot_settle_ms=DEFAULT_SNAPSHOT_SETTLE_MS,
                      snapshot_stable_cycles=DEFAULT_SNAPSHOT_STABLE_CYCLES,
                      snapshot_max_wait_ms=DEFAULT_SNAPSHOT_MAX_WAIT_MS):
    """Record single application"""
    app_name = format_app_name(app, app_sub)
    print(f"\n{'='*60}")
    print(f"Recording: {app_name}")
    print(f"{'='*60}")

    # Compile
    if not compile_app(app, app_sub, bits64):
        return (app_name, None, "compile failed")

    # Record
    if not run_and_record(app_name, fps, duration, clock_scale, snapshot_settle_ms, snapshot_stable_cycles, snapshot_max_wait_ms):
        cleanup_frames(app_name)
        return (app_name, None, "recording failed")

    # Convert to GIF
    gif_path = frames_to_gif(app_name, fps)

    # Cleanup frames
    if not keep_frames:
        cleanup_frames(app_name)

    if gif_path:
        return (app_name, gif_path, "success")
    else:
        return (app_name, None, "gif conversion failed")


def record_all_apps(fps=DEFAULT_FPS, duration=DEFAULT_DURATION, skip_list=None,
                    bits64=False, keep_frames=False,
                    clock_scale=DEFAULT_RECORDING_CLOCK_SCALE,
                    snapshot_settle_ms=DEFAULT_SNAPSHOT_SETTLE_MS,
                    snapshot_stable_cycles=DEFAULT_SNAPSHOT_STABLE_CYCLES,
                    snapshot_max_wait_ms=DEFAULT_SNAPSHOT_MAX_WAIT_MS):
    """Record all example applications"""
    if skip_list is None:
        skip_list = DEFAULT_SKIP_LIST.copy()

    results = []
    app_sets = get_example_list()
    sub_app_sets = {app: get_example_sub_list(app) for app in SUB_APP_ROOTS}

    # Calculate total count
    total = 0
    for app in app_sets:
        if app in skip_list:
            continue
        if app in SUB_APP_ROOTS:
            for app_sub in sub_app_sets.get(app, []):
                if format_app_name(app, app_sub) not in skip_list:
                    total += 1
        else:
            total += 1

    current = 0
    for app in app_sets:
        if app in skip_list:
            print(f"\nSkipping: {app}")
            continue

        if app in SUB_APP_ROOTS:
            for app_sub in sub_app_sets.get(app, []):
                full_name = format_app_name(app, app_sub)
                if full_name in skip_list:
                    print(f"\nSkipping: {full_name}")
                    continue

                current += 1
                print(f"\n[{current}/{total}] Processing {full_name}")
                result = record_single_app(
                    app,
                    app_sub,
                    fps,
                    duration,
                    bits64,
                    keep_frames,
                    clock_scale,
                    snapshot_settle_ms,
                    snapshot_stable_cycles,
                    snapshot_max_wait_ms,
                )
                results.append(result)
        else:
            current += 1
            print(f"\n[{current}/{total}] Processing {app}")
            result = record_single_app(
                app,
                None,
                fps,
                duration,
                bits64,
                keep_frames,
                clock_scale,
                snapshot_settle_ms,
                snapshot_stable_cycles,
                snapshot_max_wait_ms,
            )
            results.append(result)

    return results


def record_sub_apps(app, fps=DEFAULT_FPS, duration=DEFAULT_DURATION, bits64=False, keep_frames=False,
                    clock_scale=DEFAULT_RECORDING_CLOCK_SCALE,
                    snapshot_settle_ms=DEFAULT_SNAPSHOT_SETTLE_MS,
                    snapshot_stable_cycles=DEFAULT_SNAPSHOT_STABLE_CYCLES,
                    snapshot_max_wait_ms=DEFAULT_SNAPSHOT_MAX_WAIT_MS):
    """Record all sub-apps for a specific app that uses APP_SUB."""
    sub_apps = get_example_sub_list(app)
    results = []

    for index, app_sub in enumerate(sub_apps, start=1):
        print(f"\n[{index}/{len(sub_apps)}] Processing {app}/{app_sub}")
        result = record_single_app(
            app,
            app_sub,
            fps,
            duration,
            bits64,
            keep_frames,
            clock_scale,
            snapshot_settle_ms,
            snapshot_stable_cycles,
            snapshot_max_wait_ms,
        )
        results.append(result)

    return results


def print_summary(results):
    """Print recording summary"""
    print(f"\n{'='*60}")
    print("Recording Summary")
    print(f"{'='*60}")

    success = 0
    failed = 0
    for name, path, status in results:
        if status == "success":
            print(f"  [OK] {name} -> {path}")
            success += 1
        else:
            print(f"  [FAIL] {name}: {status}")
            failed += 1

    print(f"\nTotal: {success} succeeded, {failed} failed")


def main():
    parser = argparse.ArgumentParser(
        description='Record GIF demos for EmbeddedGUI examples',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s --app HelloSimple              Record single app
  %(prog)s --app HelloVirtual             Record all HelloVirtual sub-apps
  %(prog)s --app HelloBasic --app-sub anim  Record HelloBasic sub-app
  %(prog)s --app HelloVirtual --app-sub virtual_stage_basic  Record HelloVirtual sub-app
  %(prog)s --app HelloVirtual --app-sub virtual_stage_basic --bits64  Record HelloVirtual sub-app in 64-bit
  %(prog)s --all                          Record all (skips test examples)
  %(prog)s --all --no-skip                Record all including test examples
  %(prog)s --list                         List all available examples
        """
    )
    parser.add_argument('--app', type=str,
                        help='Specific app to record. For HelloBasic/HelloVirtual without --app-sub, records all sub-apps.')
    parser.add_argument('--app-sub', type=str,
                        help='Single sub-app for HelloBasic/HelloVirtual. If omitted, all sub-apps are recorded.')
    parser.add_argument('--fps', type=int, default=DEFAULT_FPS, help=f'Recording FPS (default: {DEFAULT_FPS})')
    parser.add_argument('--duration', type=int, default=DEFAULT_DURATION,
                        help=f'Recording duration in seconds (default: {DEFAULT_DURATION})')
    parser.add_argument('--all', action='store_true', help='Record all examples')
    parser.add_argument('--no-skip', action='store_true', help='Do not skip test examples')
    parser.add_argument('--skip', nargs='+', default=[], help='Additional apps to skip')
    parser.add_argument('--bits64', action='store_true', help='Build for 64-bit')
    parser.add_argument('--keep-frames', action='store_true', help='Keep intermediate BMP frames')
    parser.add_argument('--list', action='store_true', help='List all available examples')
    parser.add_argument('--clock-scale', type=int, default=DEFAULT_RECORDING_CLOCK_SCALE,
                        help=f'Recording clock acceleration factor (default: {DEFAULT_RECORDING_CLOCK_SCALE})')
    parser.add_argument('--snapshot-settle-ms', type=int, default=DEFAULT_SNAPSHOT_SETTLE_MS,
                        help=f'Snapshot settle time in ms (default: {DEFAULT_SNAPSHOT_SETTLE_MS})')
    parser.add_argument('--snapshot-stable-cycles', type=int, default=DEFAULT_SNAPSHOT_STABLE_CYCLES,
                        help=f'Unchanged frame cycles required before snapshot (default: {DEFAULT_SNAPSHOT_STABLE_CYCLES})')
    parser.add_argument('--snapshot-max-wait-ms', type=int, default=DEFAULT_SNAPSHOT_MAX_WAIT_MS,
                        help=f'Snapshot force-capture timeout in ms (default: {DEFAULT_SNAPSHOT_MAX_WAIT_MS})')

    args = parser.parse_args()

    # List mode
    if args.list:
        print("Available examples:")
        for app in get_example_list():
            skip_marker = " (skipped by default)" if app in DEFAULT_SKIP_LIST else ""
            print(f"  - {app}{skip_marker}")
            if app in SUB_APP_ROOTS:
                for sub in get_example_sub_list(app):
                    print(f"      - {sub}")
        return

    # Record all
    if args.all:
        skip_list = [] if args.no_skip else DEFAULT_SKIP_LIST.copy()
        skip_list.extend(args.skip)
        results = record_all_apps(
            args.fps,
            args.duration,
            skip_list,
            args.bits64,
            args.keep_frames,
            args.clock_scale,
            args.snapshot_settle_ms,
            args.snapshot_stable_cycles,
            args.snapshot_max_wait_ms,
        )
        print_summary(results)

    # Record single app
    elif args.app:
        if args.app in SUB_APP_ROOTS and not args.app_sub:
            results = record_sub_apps(
                args.app,
                args.fps,
                args.duration,
                args.bits64,
                args.keep_frames,
                args.clock_scale,
                args.snapshot_settle_ms,
                args.snapshot_stable_cycles,
                args.snapshot_max_wait_ms,
            )
            print_summary(results)
        else:
            result = record_single_app(args.app, args.app_sub, args.fps, args.duration,
                                       args.bits64, args.keep_frames, args.clock_scale, args.snapshot_settle_ms,
                                       args.snapshot_stable_cycles, args.snapshot_max_wait_ms)
            print_summary([result])

    else:
        parser.print_help()


if __name__ == '__main__':
    main()
