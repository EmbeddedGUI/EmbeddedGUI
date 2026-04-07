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
import concurrent.futures
import hashlib
import json
import re
import time
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
ROOT_DIR = SCRIPT_DIR.parent
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
COMPILE_RETRY_COUNT = 2
COMPILE_RETRY_DELAY_S = 1.0
RUN_EXCEPTION_RETRY_DELAY_S = 0.5
FILE_OP_RETRY_COUNT = 20
FILE_OP_RETRY_DELAY_S = 0.1
RUNTIME_FAIL_MARKERS = ("[RUNTIME_CHECK_FAIL]",)
FRAME_LABEL_PATTERN = re.compile(r"PERF_FRAME:(frame_\d+\.png):([A-Za-z0-9_.-]+)")
FRAME_LABEL_MANIFEST = "recording_frame_labels.json"
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
    "HelloSizeAnalysis": "example/HelloSizeAnalysis",
}
SUB_APP_FAMILY_APPS = tuple(SUB_APP_ROOTS.keys())
RUNTIME_BUILD_ROOT = Path("output") / "rt"
LAST_BUILD_OUTPUT_DIR = None

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


def write_frame_label_manifest(frames_dir, capture_output):
    frames_dir = Path(frames_dir)
    manifest_path = frames_dir / FRAME_LABEL_MANIFEST
    if manifest_path.exists():
        manifest_path.unlink()

    matches = FRAME_LABEL_PATTERN.findall(capture_output)
    if not matches:
        return

    frames_by_name = {path.name: path for path in sorted(frames_dir.glob("frame_*.png"))}
    entries = []
    missing_frames = []
    for frame_name, label in matches:
        if frame_name not in frames_by_name:
            missing_frames.append(frame_name)
            continue
        entries.append({"frame": frame_name, "label": label})

    if not entries:
        return

    manifest_path.write_text(json.dumps({"entries": entries}, ensure_ascii=False, indent=2), encoding='utf-8')

    if missing_frames:
        print("[runtime-check] PERF_FRAME markers referenced missing frames: %s" % ", ".join(sorted(set(missing_frames))))


def unlink_with_retries(path):
    last_error = None

    for attempt in range(FILE_OP_RETRY_COUNT):
        try:
            path.unlink()
            return
        except FileNotFoundError:
            return
        except PermissionError as exc:
            last_error = exc
        except OSError as exc:
            last_error = exc

        if attempt < FILE_OP_RETRY_COUNT - 1:
            time.sleep(FILE_OP_RETRY_DELAY_S)

    if last_error is not None:
        raise last_error


def wait_for_file_ready(path):
    path = Path(path)
    last_size = -1
    stable_count = 0

    for attempt in range(FILE_OP_RETRY_COUNT):
        try:
            size = path.stat().st_size
            if size > 0 and size == last_size:
                stable_count += 1
                if stable_count >= 1:
                    return True
            else:
                stable_count = 0
                last_size = size
        except FileNotFoundError:
            pass
        except OSError:
            pass

        if attempt < FILE_OP_RETRY_COUNT - 1:
            time.sleep(FILE_OP_RETRY_DELAY_S)

    return False


def run_recording_capture(exe_path, resource_path, frames_dir, timeout, duration, speed,
                          snapshot_settle_ms, clock_scale, snapshot_stable_cycles,
                          snapshot_max_wait_ms):
    frames_dir = Path(frames_dir)
    frames_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = frames_dir / FRAME_LABEL_MANIFEST
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
            unlink_with_retries(old_frame)
        if manifest_path.exists():
            unlink_with_retries(manifest_path)

        if not wait_for_file_ready(exe_path):
            last_error = "executable not ready: %s" % exe_path
            break

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
                if attempt < RUN_RETRY_COUNT - 1:
                    print("(retry %d/%d: %s)" % (attempt + 1, RUN_RETRY_COUNT - 1, last_error), end=" ")
                    time.sleep(RUN_EXCEPTION_RETRY_DELAY_S)
                    continue
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
                write_frame_label_manifest(frames_dir, combined_output)
                return True, frames_message

            last_error = frames_message
            break

        if profile_index < len(profiles) - 1:
            print("(retry %s mode: %s)" % (profiles[profile_index + 1]["label"], last_error), end=" ")

    return False, last_error


def get_example_list():
    """Get list of all example applications"""
    path = ROOT_DIR / 'example'
    app_list = []
    if not path.exists():
        return app_list

    for file in os.listdir(path):
        file_path = path / file
        if file_path.is_dir() and (file_path / 'build.mk').exists():
            app_list.append(file)

    return sorted(app_list)


def get_example_sub_list(app):
    """Get list of sub-applications for apps that use APP_SUB."""
    root = SUB_APP_ROOTS.get(app)
    app_list = []
    if not root:
        return app_list
    path = ROOT_DIR / root
    if not path.exists():
        return app_list

    if app == "HelloCustomWidgets":
        for root, dirs, files in os.walk(path):
            dirs[:] = [name for name in dirs if not name.startswith('.') and not name.startswith('__')]
            if 'test.c' not in files:
                continue

            rel_path = os.path.relpath(root, str(path)).replace('\\', '/')
            if rel_path == '.':
                continue
            app_list.append(rel_path)

        return sorted(app_list)

    for file in os.listdir(path):
        file_path = path / file
        if file_path.is_dir() and (file_path / 'app_egui_config.h').exists():
            app_list.append(file)

    return sorted(app_list)


def get_config_paths(app, app_sub=None):
    config_paths = []
    if app_sub:
        config_paths.append(ROOT_DIR / "example" / app / app_sub / "app_egui_config.h")
    config_paths.append(ROOT_DIR / "example" / app / "app_egui_config.h")
    return config_paths


def get_config_path(app, app_sub=None):
    for config_path in get_config_paths(app, app_sub):
        if config_path.exists():
            return config_path
    return None


def get_config_hash(app, app_sub=None):
    config_path = get_config_path(app, app_sub)
    if config_path is None:
        return "default"
    return hashlib.sha256(config_path.read_bytes()).hexdigest()[:12]


def filter_sub_apps(app, app_subs, category=None):
    """Filter sub-app list for app-specific grouping."""
    if not category or app != "HelloCustomWidgets":
        return sorted(app_subs)

    prefix = category.strip().strip('/\\')
    if not prefix:
        return sorted(app_subs)

    prefix = prefix.replace('\\', '/')
    return sorted([app_sub for app_sub in app_subs if app_sub.startswith(prefix + '/')])


def build_sub_app_sets():
    return {app: get_example_sub_list(app) for app in SUB_APP_ROOTS}


def build_standard_app_list(app_sets, skipped_apps=None):
    skipped = set(skipped_apps or [])
    return [
        app for app in app_sets
        if app not in SKIP_LIST and app not in SUB_APP_FAMILY_APPS and app not in skipped
    ]


def expand_runtime_cases(app, sub_app_sets):
    if app in SUB_APP_ROOTS:
        return [(app, app_sub) for app_sub in sub_app_sets.get(app, [])]
    return [(app, None)]


def build_runtime_case_specs(scope, app_sets, sub_app_sets, skipped_apps=None):
    skipped = set(skipped_apps or [])
    if scope == "standard":
        selected_apps = build_standard_app_list(app_sets, skipped)
    elif scope == "basic":
        selected_apps = ["HelloBasic"]
    elif scope == "virtual":
        selected_apps = ["HelloVirtual"]
    elif scope == "size-analysis":
        selected_apps = ["HelloSizeAnalysis"]
    elif scope == "full":
        selected_apps = [app for app in app_sets if app not in SKIP_LIST and app not in skipped]
    else:
        raise ValueError("unknown runtime scope: %s" % scope)

    case_specs = []
    for app in selected_apps:
        case_specs.extend(expand_runtime_cases(app, sub_app_sets))
    return case_specs


def apply_case_shard(case_specs, shard_count, shard_index):
    if shard_count <= 1:
        return case_specs

    if shard_index < 1 or shard_index > shard_count:
        raise ValueError("--shard-index must be in [1, --shard-count]")

    return [case for index, case in enumerate(case_specs) if index % shard_count == (shard_index - 1)]


def format_app_name(app, app_sub=None):
    """Build a stable runtime output name for app/app_sub."""
    if not app_sub:
        return app

    normalized_sub = app_sub.replace('\\', '_').replace('/', '_')
    return "%s_%s" % (app, normalized_sub)


def normalize_user_cflags(user_cflags):
    return " ".join(user_cflags.split())


def get_runtime_build_signature(app, app_sub=None, bits64=False, user_cflags="", recording_test=True):
    """Return a short stable signature for one runtime build config."""
    normalized_user_cflags = normalize_user_cflags(user_cflags)
    signature_payload = {
        "app": app,
        "app_sub": app_sub or "",
        "bits64": bool(bits64),
        "recording_test": bool(recording_test),
        "user_cflags": normalized_user_cflags,
        "compile_fast_flags": COMPILE_FAST_FLAGS,
    }
    return hashlib.sha1(
        json.dumps(signature_payload, sort_keys=True, separators=(',', ':')).encode('utf-8')
    ).hexdigest()[:10]


def get_runtime_build_output_dir(app, app_sub=None, bits64=False, user_cflags="", recording_test=True):
    """Return the short per-config OUTPUT_PATH used for runtime builds."""
    signature = get_runtime_build_signature(
        app,
        app_sub=app_sub,
        bits64=bits64,
        user_cflags=user_cflags,
        recording_test=recording_test,
    )
    return RUNTIME_BUILD_ROOT / signature


def get_runtime_objroot_path():
    return ROOT_DIR / "output"


def get_runtime_shared_obj_suffix(app, app_sub=None, bits64=False, user_cflags="", recording_test=True):
    if app not in SUB_APP_FAMILY_APPS:
        return None

    scope_signature = hashlib.sha1(
        json.dumps(
            {
                "bits64": bool(bits64),
                "recording_test": bool(recording_test),
                "user_cflags": normalize_user_cflags(user_cflags),
            },
            sort_keys=True,
            separators=(",", ":"),
        ).encode("utf-8")
    ).hexdigest()[:8]
    return "rt_%s_cfg_%s_%s" % (app.lower(), get_config_hash(app, app_sub), scope_signature)


def get_runtime_obj_suffix(app, app_sub=None, bits64=False, user_cflags="", recording_test=True):
    signature = get_runtime_build_signature(
        app,
        app_sub=app_sub,
        bits64=bits64,
        user_cflags=user_cflags,
        recording_test=recording_test,
    )
    return "rt_%s" % signature


def register_runtime_build_output_dir(build_output_dir):
    global LAST_BUILD_OUTPUT_DIR

    build_output_path = Path(build_output_dir)
    if not build_output_path.is_absolute():
        build_output_path = ROOT_DIR / build_output_path
    LAST_BUILD_OUTPUT_DIR = build_output_path


def resolve_runtime_build_output_dir(build_output_dir=None):
    if build_output_dir is not None:
        build_output_path = Path(build_output_dir)
        return build_output_path if build_output_path.is_absolute() else (ROOT_DIR / build_output_path)
    if LAST_BUILD_OUTPUT_DIR is not None:
        return LAST_BUILD_OUTPUT_DIR
    return ROOT_DIR / "output"


def get_runtime_executable_path(build_output_dir):
    if platform.system() == 'Windows':
        return build_output_dir / 'main.exe'
    return build_output_dir / 'main'


def get_runtime_resource_path(build_output_dir):
    return build_output_dir / 'app_egui_resource_merge.bin'


def get_auto_parallel_jobs():
    cpu_count = os.cpu_count() or 1
    if cpu_count <= 1:
        return 1
    return min(4, max(2, cpu_count // 2))


def resolve_parallel_jobs(requested_jobs, total_cases):
    if total_cases <= 1:
        return 1
    if requested_jobs is None or requested_jobs <= 0:
        requested_jobs = get_auto_parallel_jobs()
    return max(1, min(total_cases, requested_jobs))


def resolve_make_job_count(parallel_jobs):
    if parallel_jobs <= 1:
        return None
    cpu_count = os.cpu_count() or 1
    return max(1, cpu_count // parallel_jobs)


def get_make_job_arg(make_jobs):
    if make_jobs is None:
        return '-j'
    return '-j%d' % make_jobs


def get_runtime_skip_reason(app, app_sub=None):
    """Return a skip reason for known platform-specific runtime issues."""
    if platform.system() == 'Windows' and (app, app_sub) in WINDOWS_RUNTIME_SKIP_SET:
        return "known Windows subprocess recording hang"
    return None


def compile_app(app, app_sub=None, bits64=False, user_cflags="", recording_test=True,
                build_output_dir=None, make_jobs=None, app_obj_suffix=None, objroot_path=None):
    """Compile application with optional CFLAGS override.
    Uses a per-config OUTPUT_PATH so runtime sweeps can reuse build cache safely.
    Returns True on success, False on failure.
    """
    if build_output_dir is None:
        build_output_dir = get_runtime_build_output_dir(
            app,
            app_sub=app_sub,
            bits64=bits64,
            user_cflags=user_cflags,
            recording_test=recording_test,
        )
    if app_obj_suffix is None:
        app_obj_suffix = get_runtime_obj_suffix(
            app,
            app_sub=app_sub,
            bits64=bits64,
            user_cflags=user_cflags,
            recording_test=recording_test,
        )

    # Always inject RECORDING_TEST into the build signature so cached outputs stay isolated.
    recording_flag = '-DEGUI_CONFIG_RECORDING_TEST=%d' % (1 if recording_test else 0)
    combined_cflags = ('%s %s' % (recording_flag, user_cflags)).strip()
    cmd = ['make', get_make_job_arg(make_jobs), 'APP=%s' % app, 'PORT=pc'] + COMPILE_FAST_FLAGS
    if app_sub:
        cmd.append('APP_SUB=%s' % app_sub)
    if bits64:
        cmd.append('BITS=64')
    cmd.append('USER_CFLAGS=%s' % combined_cflags)
    cmd.append('OUTPUT_PATH=%s' % Path(build_output_dir).as_posix())
    cmd.append('APP_OBJ_SUFFIX=%s' % app_obj_suffix)
    if objroot_path is not None:
        cmd.append('OBJROOT_PATH=%s' % Path(objroot_path).as_posix())

    for attempt in range(COMPILE_RETRY_COUNT):
        result = subprocess.run(cmd, cwd=ROOT_DIR, capture_output=True, text=True)
        if result.returncode == 0:
            register_runtime_build_output_dir(build_output_dir)
            return True

        if attempt < COMPILE_RETRY_COUNT - 1:
            print("(compile retry %d/%d: %s)" % (attempt + 1, COMPILE_RETRY_COUNT - 1, format_app_name(app, app_sub)), end=" ")
            time.sleep(COMPILE_RETRY_DELAY_S)
    return False


def run_app(app_name, output_subdir, timeout=DEFAULT_TIMEOUT, duration=RECORDING_DURATION,
            speed=RECORDING_SPEED, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
            clock_scale=RECORDING_CLOCK_SCALE,
            snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
            snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS,
            build_output_dir=None):
    """Run app with recording mode, capture screenshot, verify exit.
    Retries on crash (access violation) since PC simulator has known
    race conditions between main thread and egui thread.
    Returns (success: bool, message: str).
    """
    frames_dir = ROOT_DIR / SCREENSHOT_DIR / app_name / output_subdir
    os.makedirs(frames_dir, exist_ok=True)
    build_output_dir = resolve_runtime_build_output_dir(build_output_dir)
    exe_path = get_runtime_executable_path(build_output_dir)
    resource_path = get_runtime_resource_path(build_output_dir)

    if not exe_path.exists():
        return False, "executable not found"

    return run_recording_capture(exe_path, resource_path, frames_dir, timeout, duration,
                                 speed, snapshot_settle_ms, clock_scale,
                                 snapshot_stable_cycles, snapshot_max_wait_ms)


def check_default_resolution(app, app_sub, bits64, explicit_timeout=None,
                             speed=RECORDING_SPEED, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
                             clock_scale=RECORDING_CLOCK_SCALE,
                             snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
                             snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS,
                             make_jobs=None, app_obj_suffix=None, objroot_path=None):
    """Test app at default resolution (no CFLAGS override).
    Apps auto-quit after all actions complete; RECORDING_DURATION is a safety timeout.
    Returns (success: bool, message: str).
    """
    app_name = format_app_name(app, app_sub)
    skip_reason = get_runtime_skip_reason(app, app_sub)

    if skip_reason:
        return True, "skipped: %s" % skip_reason

    build_output_dir = get_runtime_build_output_dir(app, app_sub=app_sub, bits64=bits64)

    if not compile_app(
        app,
        app_sub,
        bits64,
        build_output_dir=build_output_dir,
        make_jobs=make_jobs,
        app_obj_suffix=app_obj_suffix,
        objroot_path=objroot_path,
    ):
        return False, "compile failed"

    # Timeout must be >= recording duration + margin to allow auto-quit
    timeout = max(RECORDING_DURATION + 5, explicit_timeout if explicit_timeout is not None else DEFAULT_TIMEOUT)
    return run_app(app_name, "default", timeout=timeout, duration=RECORDING_DURATION,
                   speed=speed, snapshot_settle_ms=snapshot_settle_ms,
                   clock_scale=clock_scale,
                   snapshot_stable_cycles=snapshot_stable_cycles,
                   snapshot_max_wait_ms=snapshot_max_wait_ms,
                   build_output_dir=build_output_dir)



def capture_animation_frames(app_name, output_dir, fps=10, duration=5,
                             speed=1, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
                             clock_scale=RECORDING_CLOCK_SCALE,
                             snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
                             snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS,
                             build_output_dir=None):
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
    build_output_dir = resolve_runtime_build_output_dir(build_output_dir)
    exe_path = get_runtime_executable_path(build_output_dir)
    resource_path = get_runtime_resource_path(build_output_dir)

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


def print_case_header(index, total, app_name, speed):
    print("\n" + "=" * 60)
    print("[%d/%d] %s (speed=%dx)" % (index, total, app_name, speed))
    print("=" * 60)


def build_runtime_case_info(app, app_sub, bits64, user_cflags="", recording_test=True):
    return {
        "app": app,
        "app_sub": app_sub,
        "name": format_app_name(app, app_sub),
        "shared_obj_suffix": get_runtime_shared_obj_suffix(
            app,
            app_sub=app_sub,
            bits64=bits64,
            user_cflags=user_cflags,
            recording_test=recording_test,
        ),
        "objroot_path": get_runtime_objroot_path(),
    }


def run_runtime_case(app, app_sub, bits64, explicit_timeout=None,
                     speed=RECORDING_SPEED, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
                     clock_scale=RECORDING_CLOCK_SCALE,
                     snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
                     snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS,
                     make_jobs=None, app_obj_suffix=None, objroot_path=None):
    app_name = format_app_name(app, app_sub)
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
        make_jobs=make_jobs,
        app_obj_suffix=app_obj_suffix,
        objroot_path=objroot_path,
    )
    return app_name, success, msg


def retry_failed_runtime_cases_serial(results, case_specs, bits64, explicit_timeout=None,
                                      speed=RECORDING_SPEED, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
                                      clock_scale=RECORDING_CLOCK_SCALE,
                                      snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
                                      snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS):
    normalized_results = []
    failed_indexes = []
    for index, item in enumerate(results):
        if item is None:
            app, app_sub = case_specs[index]
            item = (format_app_name(app, app_sub), False, "missing runtime result")
        normalized_results.append(item)
        if not item[1]:
            failed_indexes.append(index)

    results = normalized_results
    if not failed_indexes:
        return results

    print("Retrying %d failed runtime cases serially for stability" % len(failed_indexes))
    for retry_index, index in enumerate(failed_indexes, start=1):
        app, app_sub = case_specs[index]
        case_info = build_runtime_case_info(app, app_sub, bits64)
        app_name = case_info["name"]
        print_case_header(retry_index, len(failed_indexes), "%s [serial retry]" % app_name, speed)
        _, success, msg = run_runtime_case(
            app,
            app_sub,
            bits64,
            explicit_timeout=explicit_timeout,
            speed=speed,
            snapshot_settle_ms=snapshot_settle_ms,
            clock_scale=clock_scale,
            snapshot_stable_cycles=snapshot_stable_cycles,
            snapshot_max_wait_ms=snapshot_max_wait_ms,
            make_jobs=1,
            app_obj_suffix=case_info["shared_obj_suffix"],
            objroot_path=case_info["objroot_path"] if case_info["shared_obj_suffix"] else None,
        )
        status = "PASS" if success else "FAIL"
        print("  %s (%s)" % (status, msg))
        results[index] = (app_name, success, msg)

    return results


def run_runtime_case_batch(case_specs, bits64, explicit_timeout=None, jobs=0,
                           speed=RECORDING_SPEED, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
                           clock_scale=RECORDING_CLOCK_SCALE,
                           snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
                           snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS):
    total = len(case_specs)
    if total == 0:
        return []

    parallel_jobs = resolve_parallel_jobs(jobs, total)
    make_jobs = resolve_make_job_count(parallel_jobs)
    case_infos = [build_runtime_case_info(app, app_sub, bits64) for app, app_sub in case_specs]

    if parallel_jobs <= 1:
        results = []
        for index, case_info in enumerate(case_infos, start=1):
            app = case_info["app"]
            app_sub = case_info["app_sub"]
            app_name = case_info["name"]
            print_case_header(index, total, app_name, speed)
            _, success, msg = run_runtime_case(
                app,
                app_sub,
                bits64,
                explicit_timeout=explicit_timeout,
                speed=speed,
                snapshot_settle_ms=snapshot_settle_ms,
                clock_scale=clock_scale,
                snapshot_stable_cycles=snapshot_stable_cycles,
                snapshot_max_wait_ms=snapshot_max_wait_ms,
                make_jobs=make_jobs,
                app_obj_suffix=case_info["shared_obj_suffix"],
                objroot_path=case_info["objroot_path"] if case_info["shared_obj_suffix"] else None,
            )
            status = "PASS" if success else "FAIL"
            print("  %s (%s)" % (status, msg))
            results.append((app_name, success, msg))
        return results

    print("Running %d runtime cases with jobs=%d, make -j%d, speed=%dx" % (total, parallel_jobs, make_jobs, speed))
    results = [None] * total
    with concurrent.futures.ThreadPoolExecutor(max_workers=parallel_jobs) as executor:
        future_to_case = {}
        completed = 0
        grouped_indexes = {}
        for index, case_info in enumerate(case_infos):
            shared_suffix = case_info["shared_obj_suffix"]
            if shared_suffix:
                grouped_indexes.setdefault(shared_suffix, []).append(index)
            else:
                grouped_indexes.setdefault("__direct__%d" % index, []).append(index)

        direct_indexes = []
        for group_key in sorted(grouped_indexes):
            indexes = grouped_indexes[group_key]
            if group_key.startswith("__direct__"):
                direct_indexes.extend(indexes)
                continue

            seed_index = indexes[0]
            seed_case = case_infos[seed_index]
            completed += 1
            print("[%d/%d] %s WARMUP" % (completed, total, seed_case["name"]))
            _, success, msg = run_runtime_case(
                seed_case["app"],
                seed_case["app_sub"],
                bits64,
                explicit_timeout=explicit_timeout,
                speed=speed,
                snapshot_settle_ms=snapshot_settle_ms,
                clock_scale=clock_scale,
                snapshot_stable_cycles=snapshot_stable_cycles,
                snapshot_max_wait_ms=snapshot_max_wait_ms,
                make_jobs=make_jobs,
                app_obj_suffix=seed_case["shared_obj_suffix"],
                objroot_path=seed_case["objroot_path"],
            )
            results[seed_index] = (seed_case["name"], success, msg)
            status = "PASS" if success else "FAIL"
            print("[%d/%d] %s %s (%s)" % (completed, total, seed_case["name"], status, msg))

            if not success:
                for index in indexes[1:]:
                    case_info = case_infos[index]
                    results[index] = (case_info["name"], False, "skipped after warmup failure")
                continue

                for index in indexes[1:]:
                    case_info = case_infos[index]
                    future = executor.submit(
                        run_runtime_case,
                        case_info["app"],
                    case_info["app_sub"],
                    bits64,
                    explicit_timeout,
                    speed,
                    snapshot_settle_ms,
                    clock_scale,
                    snapshot_stable_cycles,
                    snapshot_max_wait_ms,
                    make_jobs,
                    case_info["shared_obj_suffix"],
                    case_info["objroot_path"],
                    )
                    future_to_case[future] = index

        for index in direct_indexes:
            case_info = case_infos[index]
            future = executor.submit(
                run_runtime_case,
                case_info["app"],
                case_info["app_sub"],
                bits64,
                explicit_timeout,
                speed,
                snapshot_settle_ms,
                clock_scale,
                snapshot_stable_cycles,
                snapshot_max_wait_ms,
                make_jobs,
                None,
                None,
            )
            future_to_case[future] = index

        for future in concurrent.futures.as_completed(future_to_case):
            index = future_to_case[future]
            case_info = case_infos[index]
            app_name = case_info["name"]
            try:
                _, success, msg = future.result()
            except Exception as exc:
                success = False
                msg = "unexpected error: %s" % exc
            results[index] = (app_name, success, msg)
            completed += 1
            status = "PASS" if success else "FAIL"
            print("[%d/%d] %s %s (%s)" % (completed, total, app_name, status, msg))

    return retry_failed_runtime_cases_serial(
        results,
        case_specs,
        bits64,
        explicit_timeout=explicit_timeout,
        speed=speed,
        snapshot_settle_ms=snapshot_settle_ms,
        clock_scale=clock_scale,
        snapshot_stable_cycles=snapshot_stable_cycles,
        snapshot_max_wait_ms=snapshot_max_wait_ms,
    )


def run_full_check(bits64, speed=RECORDING_SPEED, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
                   clock_scale=RECORDING_CLOCK_SCALE,
                   snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
                   snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS,
                   skip_custom_widgets=False, jobs=0):
    """Run runtime check on all examples.
    Returns list of (app_name, success, message) tuples.
    """
    app_sets = get_example_list()
    sub_app_sets = build_sub_app_sets()
    skipped_apps = set()

    if skip_custom_widgets:
        skipped_apps.add("HelloCustomWidgets")

    print("Running with speed=%dx" % speed)
    for app in app_sets:
        if app in SKIP_LIST:
            print("\nSkipping: %s" % app)
        elif app in skipped_apps:
            print("\nSkipping: %s (%s)" % (app, FULL_CHECK_OPTIONAL_APPS[app]))

    case_specs = build_runtime_case_specs("full", app_sets, sub_app_sets, skipped_apps=skipped_apps)

    return run_runtime_case_batch(
        case_specs,
        bits64,
        jobs=jobs,
        speed=speed,
        snapshot_settle_ms=snapshot_settle_ms,
        clock_scale=clock_scale,
        snapshot_stable_cycles=snapshot_stable_cycles,
        snapshot_max_wait_ms=snapshot_max_wait_ms,
    )


def run_scope_check(scope, bits64, explicit_timeout=None,
                    speed=RECORDING_SPEED, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
                    clock_scale=RECORDING_CLOCK_SCALE,
                    snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
                    snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS,
                    skip_custom_widgets=False, jobs=0, shard_count=1, shard_index=1):
    app_sets = get_example_list()
    sub_app_sets = build_sub_app_sets()
    skipped_apps = set()
    if skip_custom_widgets:
        skipped_apps.add("HelloCustomWidgets")

    case_specs = build_runtime_case_specs(scope, app_sets, sub_app_sets, skipped_apps=skipped_apps)
    case_specs = apply_case_shard(case_specs, shard_count, shard_index)

    print("=" * 60)
    print("Runtime Check Scope: %s shard=%d/%d speed=%dx cases=%d" % (scope, shard_index, shard_count, speed, len(case_specs)))
    print("=" * 60)

    return run_runtime_case_batch(
        case_specs,
        bits64,
        explicit_timeout=explicit_timeout,
        jobs=jobs,
        speed=speed,
        snapshot_settle_ms=snapshot_settle_ms,
        clock_scale=clock_scale,
        snapshot_stable_cycles=snapshot_stable_cycles,
        snapshot_max_wait_ms=snapshot_max_wait_ms,
    )


def run_sub_app_check(app, app_subs, bits64, explicit_timeout=None,
                      speed=RECORDING_SPEED, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
                      clock_scale=RECORDING_CLOCK_SCALE,
                      snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
                      snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS,
                      jobs=0):
    """Run runtime check for a batch of APP_SUB examples under one app."""
    case_specs = [(app, app_sub) for app_sub in app_subs]
    return run_runtime_case_batch(
        case_specs,
        bits64,
        explicit_timeout=explicit_timeout,
        jobs=jobs,
        speed=speed,
        snapshot_settle_ms=snapshot_settle_ms,
        clock_scale=clock_scale,
        snapshot_stable_cycles=snapshot_stable_cycles,
        snapshot_max_wait_ms=snapshot_max_wait_ms,
    )


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
  %(prog)s --app HelloSizeAnalysis                   Test all HelloSizeAnalysis sub-apps
  %(prog)s --app HelloBasic --app-sub button         Test one HelloBasic sub-app
  %(prog)s --app HelloVirtual --app-sub virtual_grid Test one HelloVirtual sub-app
  %(prog)s --app HelloCustomWidgets --category input Test HelloCustomWidgets category
  %(prog)s --scope standard --jobs 2                 Test standard runtime examples only
  %(prog)s --scope basic --shard-count 3 --shard-index 1 --jobs 2
  %(prog)s --full-check --skip-custom-widgets        Test all examples except HelloCustomWidgets
  %(prog)s --full-check                             Test all examples
        """
    )
    parser.add_argument('--app', type=str,
                        help='Specific app to test. For HelloBasic/HelloVirtual/HelloCustomWidgets/HelloSizeAnalysis without --app-sub, tests all sub-apps.')
    parser.add_argument('--app-sub', type=str,
                        help='Single sub-app for HelloBasic/HelloVirtual/HelloCustomWidgets/HelloSizeAnalysis. If omitted, all sub-apps are tested.')
    parser.add_argument('--category', type=str, help='HelloCustomWidgets category filter (e.g. input)')
    parser.add_argument('--bits64', action='store_true', help='Build for 64-bit')
    parser.add_argument('--timeout', type=int, default=DEFAULT_TIMEOUT,
                        help='Run timeout in seconds (default: %d)' % DEFAULT_TIMEOUT)
    parser.add_argument('--keep-screenshots', action='store_true',
                        help='Keep screenshot files after testing')
    parser.add_argument('--full-check', action='store_true',
                        help='Test all example applications')
    parser.add_argument('--scope', choices=['standard', 'basic', 'virtual', 'size-analysis'],
                        help='Run one runtime case family only, useful for CI sharding')
    parser.add_argument('--skip-custom-widgets', action='store_true',
                        help='Exclude HelloCustomWidgets from --full-check to keep CI/runtime sweeps shorter')
    parser.add_argument('--jobs', type=int, default=0,
                        help='Parallel runtime cases for batch/full checks (default: auto, 0=auto)')
    parser.add_argument('--shard-count', type=int, default=1,
                        help='Split scoped runtime cases into N shards')
    parser.add_argument('--shard-index', type=int, default=1,
                        help='1-based shard index used with --shard-count')
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

    if args.full_check and args.scope:
        print("Error: --full-check cannot be combined with --scope")
        sys.exit(1)

    if args.app and args.scope:
        print("Error: --app cannot be combined with --scope")
        sys.exit(1)

    if args.scope is None and args.shard_count != 1:
        print("Error: --shard-count requires --scope")
        sys.exit(1)

    if args.scope is None and args.shard_index != 1:
        print("Error: --shard-index requires --scope")
        sys.exit(1)

    if args.scope is not None and args.shard_count < 1:
        print("Error: --shard-count must be >= 1")
        sys.exit(1)

    if args.scope is not None and args.shard_count == 1 and args.shard_index != 1:
        print("Error: --shard-index must be 1 when --shard-count is 1")
        sys.exit(1)

    if args.full_check:
        results = run_full_check(
            args.bits64,
            speed=speed,
            clock_scale=clock_scale,
            snapshot_settle_ms=snapshot_settle_ms,
            snapshot_stable_cycles=snapshot_stable_cycles,
            snapshot_max_wait_ms=snapshot_max_wait_ms,
            skip_custom_widgets=args.skip_custom_widgets,
            jobs=args.jobs,
        )
        all_passed = print_summary(results)

    elif args.scope:
        try:
            results = run_scope_check(
                args.scope,
                args.bits64,
                explicit_timeout=args.timeout,
                speed=speed,
                clock_scale=clock_scale,
                snapshot_settle_ms=snapshot_settle_ms,
                snapshot_stable_cycles=snapshot_stable_cycles,
                snapshot_max_wait_ms=snapshot_max_wait_ms,
                skip_custom_widgets=args.skip_custom_widgets,
                jobs=args.jobs,
                shard_count=args.shard_count,
                shard_index=args.shard_index,
            )
        except ValueError as exc:
            print("Error: %s" % exc)
            sys.exit(1)
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
                jobs=args.jobs,
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
            screenshot_base = ROOT_DIR / SCREENSHOT_DIR / result_name
            if not success or not screenshot_base.exists():
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
