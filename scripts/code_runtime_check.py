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
import shutil
import signal
import tempfile
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
HELLO_VIRTUAL_SNAPSHOT_SETTLE_MS = SLOW_HOST_RETRY_SNAPSHOT_SETTLE_MS
HELLO_VIRTUAL_SNAPSHOT_STABLE_CYCLES = SLOW_HOST_RETRY_SNAPSHOT_STABLE_CYCLES
HELLO_VIRTUAL_SNAPSHOT_MAX_WAIT_MS = SLOW_HOST_RETRY_SNAPSHOT_MAX_WAIT_MS
# Speed up compilation: disable debug symbols, use -O0 (same as ui_designer)
COMPILE_FAST_FLAGS = ['COMPILE_DEBUG=', 'COMPILE_OPT_LEVEL=-O0']
# Retry count for intermittent crashes (e.g., race conditions in PC simulator threading)
RUN_RETRY_COUNT = 2
COMPILE_RETRY_COUNT = 2
COMPILE_RETRY_DELAY_S = 1.0
RUN_EXCEPTION_RETRY_DELAY_S = 0.5
FILE_OP_RETRY_COUNT = 20
FILE_OP_RETRY_DELAY_S = 0.1
PROCESS_TERMINATE_GRACE_S = 5
RUNTIME_FAIL_MARKERS = ("[RUNTIME_CHECK_FAIL]",)
FRAME_LABEL_PATTERN = re.compile(r"PERF_FRAME:(frame_\d+\.png):([A-Za-z0-9_.-]+)")
FRAME_LABEL_MANIFEST = "recording_frame_labels.json"
PRIMARY_FRAME_PATTERN = re.compile(r"^frame_(\d+)\.png$")
DISPLAY_FRAME_PATTERN = re.compile(r"^frame_(\d+)_disp(\d+)\.png$")
SHUTDOWN_BEGIN_PATTERN = re.compile(r"\[SHUTDOWN_CHECK\]\s+begin_shutdown\b")
SHUTDOWN_QUEUE_PATTERN = re.compile(
    r"\[SHUTDOWN_CHECK\]\s+display=(\d+)\s+queue_capacity=(\d+)\s+posted=(\d+)(?:\s+retries=(\d+))?(?:\s+max_retry_burst=(\d+))?\s+rejected=(\d+)\s+wait_timeouts=(\d+)\s+peak_queue=(\d+)\s+pending=(\d+)(?:\s+inflight=(\d+))?(?:\s+max_queue_wait_ms=(\d+)\s+max_queue_wait_ctx=([^\s]+))?(?:\s+max_exec_time_ms=(\d+)\s+max_exec_time_ctx=([^\s]+))?\b"
)
SHUTDOWN_THREADS_PATTERN = re.compile(r"\[SHUTDOWN_CHECK\]\s+core_threads_stopped=(\d+)\b")
SHUTDOWN_CLEANUP_PATTERN = re.compile(r"\[SHUTDOWN_CHECK\]\s+sdl_cleanup\s+primary_window=(\d+)\s+extra_windows=(\d+)\b")
SHUTDOWN_DEINIT_PATTERN = re.compile(r"\[SHUTDOWN_CHECK\]\s+deinit_done\b")
MAX_CORE_TASK_RETRIES = -1
MAX_SINGLE_POST_CORE_TASK_RETRIES = -1
FAIL_ON_FULL_CORE_TASK_QUEUE = False
MAX_SHUTDOWN_QUEUE_WAIT_MS = -1
MAX_SHUTDOWN_EXEC_TIME_MS = -1
CUSTOM_WIDGETS_REPO = "https://github.com/EmbeddedGUI/EmbeddedGUI_Widgets"
FULL_CHECK_OPTIONAL_APPS = {}
MULTI_DISPLAY_APPS = ("HelloMultiDisplay", "HelloMultiDisplayHetero")
EXPECTED_RUNTIME_FRAME_LABELS = {
    ("HelloMultiDisplay", None): (
        "initial",
        "after_disp0_next",
        "after_disp1_next",
        "after_finish",
    ),
    ("HelloMultiDisplayHetero", None): (
        "initial",
        "after_main_drag_1",
        "after_main_drag_2",
        "after_disp1_click",
        "after_main_restore",
    ),
}
RUNTIME_SELF_CHECK_SUMMARIES = {
    ("HelloMultiDisplay", None): "checks=click-isolation(primary),concurrent-activity-anims(primary/sub)",
    ("HelloMultiDisplayHetero", None): "checks=sub-tick-continuity,sub-click-reset",
}
MAX_DISPLAY_COUNT_PATTERN = re.compile(r"^\s*#\s*define\s+EGUI_CONFIG_MAX_DISPLAY_COUNT\s+(\d+)\b", re.MULTILINE)

# Examples not suitable for runtime testing (headless/performance/test-only)
SKIP_LIST = ["HelloUnitTest", "HelloTest", "HelloPerformance", "HelloPerformance",
             "HelloDesigner", "HelloDesigner_temp"]
SUB_APP_ROOTS = {
    "HelloBasic": "example/HelloBasic",
    "HelloVirtual": "example/HelloVirtual",
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


def get_recording_popen_kwargs():
    kwargs = get_windows_hidden_run_kwargs().copy()
    if platform.system() == 'Windows':
        create_new_process_group = getattr(subprocess, 'CREATE_NEW_PROCESS_GROUP', 0)
        if create_new_process_group:
            kwargs['creationflags'] = kwargs.get('creationflags', 0) | create_new_process_group
    else:
        kwargs['start_new_session'] = True
    return kwargs


def read_output_file(output_file):
    output_file.flush()
    output_file.seek(0)
    data = output_file.read()
    if isinstance(data, bytes):
        return data.decode('utf-8', errors='replace')
    return data


def terminate_process_tree(proc):
    if proc.poll() is not None:
        return

    if platform.system() == 'Windows':
        try:
            subprocess.run(
                ['taskkill', '/F', '/T', '/PID', str(proc.pid)],
                capture_output=True,
                text=True,
                timeout=PROCESS_TERMINATE_GRACE_S,
            )
        except Exception:
            try:
                proc.kill()
            except Exception:
                pass
    else:
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except Exception:
            try:
                proc.kill()
            except Exception:
                pass

    try:
        proc.wait(timeout=PROCESS_TERMINATE_GRACE_S)
    except Exception:
        try:
            proc.kill()
        except Exception:
            pass


def run_process_capture_output(cmd, timeout, **popen_kwargs):
    with tempfile.TemporaryFile() as stdout_file, tempfile.TemporaryFile() as stderr_file:
        proc = subprocess.Popen(cmd, stdout=stdout_file, stderr=stderr_file, **popen_kwargs)
        try:
            returncode = proc.wait(timeout=timeout)
        except subprocess.TimeoutExpired as exc:
            terminate_process_tree(proc)
            stdout = read_output_file(stdout_file)
            stderr = read_output_file(stderr_file)
            raise subprocess.TimeoutExpired(cmd, timeout, output=stdout, stderr=stderr) from exc
        except BaseException:
            terminate_process_tree(proc)
            raise

        stdout = read_output_file(stdout_file)
        stderr = read_output_file(stderr_file)
        return subprocess.CompletedProcess(cmd, returncode, stdout, stderr)


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


def tune_recording_params_for_app(app_name, snapshot_settle_ms, snapshot_stable_cycles, snapshot_max_wait_ms):
    if not app_name.startswith("HelloVirtual"):
        return snapshot_settle_ms, snapshot_stable_cycles, snapshot_max_wait_ms

    return (
        max(snapshot_settle_ms, HELLO_VIRTUAL_SNAPSHOT_SETTLE_MS),
        max(snapshot_stable_cycles, HELLO_VIRTUAL_SNAPSHOT_STABLE_CYCLES),
        max(snapshot_max_wait_ms, HELLO_VIRTUAL_SNAPSHOT_MAX_WAIT_MS),
    )


def get_config_max_display_count(app, app_sub=None):
    for config_path in get_config_paths(app, app_sub):
        if not config_path.exists():
            continue

        match = MAX_DISPLAY_COUNT_PATTERN.search(config_path.read_text(encoding='utf-8'))
        if match is not None:
            value = int(match.group(1))
            if value > 0:
                return value

    return 1


def get_expected_runtime_display_ids(app, app_sub=None):
    return tuple(range(get_config_max_display_count(app, app_sub)))


def get_expected_runtime_frame_labels(app, app_sub=None):
    return EXPECTED_RUNTIME_FRAME_LABELS.get((app, app_sub)) or EXPECTED_RUNTIME_FRAME_LABELS.get((app, None))


def get_runtime_self_check_summary(app, app_sub=None):
    return RUNTIME_SELF_CHECK_SUMMARIES.get((app, app_sub)) or RUNTIME_SELF_CHECK_SUMMARIES.get((app, None), "")


def parse_frame_label_matches(capture_output):
    return FRAME_LABEL_PATTERN.findall(capture_output)


def validate_recording_frames(frames_dir, expected_display_ids=(0,)):
    frame_dir = Path(frames_dir)
    frame_files = sorted(frame_dir.glob("frame_*.png"))
    if not frame_files:
        return False, "no frames generated"

    primary_frames = {}
    display_frames = {}

    for frame_path in frame_files:
        size = frame_path.stat().st_size
        if size < 100:
            return False, "frame %s too small (%d bytes)" % (frame_path.name, size)

        primary_match = PRIMARY_FRAME_PATTERN.match(frame_path.name)
        if primary_match is not None:
            primary_frames[primary_match.group(1)] = frame_path
            continue

        display_match = DISPLAY_FRAME_PATTERN.match(frame_path.name)
        if display_match is not None:
            frame_index = display_match.group(1)
            display_id = int(display_match.group(2))
            display_frames.setdefault(display_id, {})[frame_index] = frame_path

    if not primary_frames:
        return False, "no primary display frames generated"

    primary_indices = sorted(primary_frames.keys())
    for display_id in expected_display_ids:
        if display_id == 0:
            continue

        display_frame_map = display_frames.get(display_id, {})
        missing_indices = [frame_index for frame_index in primary_indices if frame_index not in display_frame_map]
        if missing_indices:
            return False, "missing display %d frames: %s" % (display_id, ", ".join("frame_%s_disp%d.png" % (index, display_id) for index in missing_indices[:5]))

    if len(expected_display_ids) > 1:
        display_summaries = ["disp0:%d" % len(primary_frames)]
        for display_id in expected_display_ids:
            if display_id == 0:
                continue
            display_summaries.append("disp%d:%d" % (display_id, len(display_frames.get(display_id, {}))))
        return True, "%s frames captured -> %s" % (", ".join(display_summaries), frames_dir)

    return True, "%d primary frames captured -> %s" % (len(primary_frames), frames_dir)


def validate_recording_frame_labels(frames_dir, label_matches, app=None, app_sub=None):
    expected_labels = get_expected_runtime_frame_labels(app, app_sub)
    if not expected_labels:
        return True, ""

    if not label_matches:
        return False, "missing labeled snapshots: expected %s" % ", ".join(expected_labels)

    frames_dir = Path(frames_dir)
    frames_by_name = {path.name: path for path in sorted(frames_dir.glob("frame_*.png"))}
    actual_labels = []
    missing_frames = []

    for frame_name, label in label_matches:
        if frame_name not in frames_by_name:
            missing_frames.append(frame_name)
            continue
        actual_labels.append(label)

    if missing_frames:
        return False, "labeled snapshots referenced missing frames: %s" % ", ".join(sorted(set(missing_frames)))

    if tuple(actual_labels) != tuple(expected_labels):
        return False, "snapshot labels mismatch: expected [%s], got [%s]" % (", ".join(expected_labels), ", ".join(actual_labels))

    return True, "stages=%s" % ", ".join(actual_labels)


def validate_shutdown_markers(capture_output, expected_display_ids=(0,), app=None, app_sub=None):
    EGUI_UNUSED = app_sub
    if app not in MULTI_DISPLAY_APPS:
        return True, ""

    begin_match = SHUTDOWN_BEGIN_PATTERN.search(capture_output)
    queue_matches = list(SHUTDOWN_QUEUE_PATTERN.finditer(capture_output))
    threads_match = SHUTDOWN_THREADS_PATTERN.search(capture_output)
    cleanup_match = SHUTDOWN_CLEANUP_PATTERN.search(capture_output)
    deinit_match = SHUTDOWN_DEINIT_PATTERN.search(capture_output)

    if begin_match is None:
        return False, "missing shutdown begin marker"
    if not queue_matches:
        return False, "missing core task queue metrics"
    if threads_match is None:
        return False, "missing core thread shutdown marker"
    if cleanup_match is None:
        return False, "missing SDL cleanup marker"
    if deinit_match is None:
        return False, "missing deinit completion marker"

    queue_positions = tuple(match.start() for match in queue_matches)
    ordered_positions = (begin_match.start(),) + queue_positions + (
        threads_match.start(),
        cleanup_match.start(),
        deinit_match.start(),
    )
    if ordered_positions != tuple(sorted(ordered_positions)):
        return False, "shutdown marker order mismatch"

    expected_thread_count = len(expected_display_ids)
    expected_extra_windows = max(0, expected_thread_count - 1)
    actual_thread_count = int(threads_match.group(1))
    actual_primary_window = int(cleanup_match.group(1))
    actual_extra_windows = int(cleanup_match.group(2))

    if actual_thread_count != expected_thread_count:
        return False, "shutdown thread count mismatch: expected %d, got %d" % (expected_thread_count, actual_thread_count)
    if actual_primary_window != 1:
        return False, "shutdown primary window count mismatch: expected 1, got %d" % actual_primary_window
    if actual_extra_windows != expected_extra_windows:
        return False, "shutdown extra window count mismatch: expected %d, got %d" % (expected_extra_windows, actual_extra_windows)

    queue_metrics_by_display = {}
    for match in queue_matches:
        display_id = int(match.group(1))
        queue_metrics_by_display[display_id] = {
            "queue_capacity": int(match.group(2)),
            "posted": int(match.group(3)),
            "retries": int(match.group(4) or 0),
            "max_retry_burst": int(match.group(5) or 0),
            "rejected": int(match.group(6)),
            "wait_timeouts": int(match.group(7)),
            "peak_queue": int(match.group(8)),
            "pending": int(match.group(9)),
            "inflight": int(match.group(10) or 0),
            "max_queue_wait_ms": int(match.group(11) or 0),
            "max_queue_wait_ctx": match.group(12) or "none",
            "max_exec_time_ms": int(match.group(13) or 0),
            "max_exec_time_ctx": match.group(14) or "none",
        }

    missing_displays = [display_id for display_id in expected_display_ids if display_id not in queue_metrics_by_display]
    if missing_displays:
        return False, "missing core task queue metrics for displays: %s" % ", ".join(str(display_id) for display_id in missing_displays)

    queue_summary_parts = []
    for display_id in expected_display_ids:
        metrics = queue_metrics_by_display[display_id]
        if metrics["queue_capacity"] <= 0:
            return False, "invalid queue capacity for display %d: %d" % (display_id, metrics["queue_capacity"])
        if metrics["rejected"] != 0:
            return False, "core task rejected on display %d: %d" % (display_id, metrics["rejected"])
        if metrics["wait_timeouts"] != 0:
            return False, "core task wait timeout on display %d: %d" % (display_id, metrics["wait_timeouts"])
        if metrics["pending"] != 0:
            return False, "core task pending on display %d at shutdown: %d" % (display_id, metrics["pending"])
        if metrics["inflight"] != 0:
            return False, "core task still inflight on display %d at shutdown: %d" % (display_id, metrics["inflight"])
        if MAX_CORE_TASK_RETRIES >= 0 and metrics["retries"] > MAX_CORE_TASK_RETRIES:
            return False, "core task retries exceeded on display %d: %d > %d" % (
                display_id,
                metrics["retries"],
                MAX_CORE_TASK_RETRIES,
            )
        if MAX_SINGLE_POST_CORE_TASK_RETRIES >= 0 and metrics["max_retry_burst"] > MAX_SINGLE_POST_CORE_TASK_RETRIES:
            return False, "core task max single-post retries exceeded on display %d: %d > %d" % (
                display_id,
                metrics["max_retry_burst"],
                MAX_SINGLE_POST_CORE_TASK_RETRIES,
            )
        if FAIL_ON_FULL_CORE_TASK_QUEUE and metrics["peak_queue"] >= metrics["queue_capacity"]:
            return False, "core task queue saturated on display %d: peak=%d/%d" % (
                display_id,
                metrics["peak_queue"],
                metrics["queue_capacity"],
            )
        if MAX_SHUTDOWN_QUEUE_WAIT_MS >= 0 and metrics["max_queue_wait_ms"] > MAX_SHUTDOWN_QUEUE_WAIT_MS:
            return False, "core task queue wait exceeded on display %d: %dms > %dms (%s)" % (
                display_id,
                metrics["max_queue_wait_ms"],
                MAX_SHUTDOWN_QUEUE_WAIT_MS,
                metrics["max_queue_wait_ctx"],
            )
        if MAX_SHUTDOWN_EXEC_TIME_MS >= 0 and metrics["max_exec_time_ms"] > MAX_SHUTDOWN_EXEC_TIME_MS:
            return False, "core task exec time exceeded on display %d: %dms > %dms (%s)" % (
                display_id,
                metrics["max_exec_time_ms"],
                MAX_SHUTDOWN_EXEC_TIME_MS,
                metrics["max_exec_time_ctx"],
            )

        queue_summary_parts.append(
            "disp%d(posted=%d,retries=%d,max_retry_burst=%d,peak=%d/%d,queue_wait=%d@%s,exec=%d@%s)" % (
                display_id,
                metrics["posted"],
                metrics["retries"],
                metrics["max_retry_burst"],
                metrics["peak_queue"],
                metrics["queue_capacity"],
                metrics["max_queue_wait_ms"],
                metrics["max_queue_wait_ctx"],
                metrics["max_exec_time_ms"],
                metrics["max_exec_time_ctx"],
            )
        )

    return True, "shutdown=begin->queue[%s]->threads:%d->cleanup:%d+%d->deinit" % (
        ",".join(queue_summary_parts),
        actual_thread_count,
        actual_primary_window,
        actual_extra_windows,
    )


def write_frame_label_manifest(frames_dir, label_matches):
    frames_dir = Path(frames_dir)
    manifest_path = frames_dir / FRAME_LABEL_MANIFEST
    if manifest_path.exists():
        manifest_path.unlink()

    if not label_matches:
        return

    frames_by_name = {path.name: path for path in sorted(frames_dir.glob("frame_*.png"))}
    entries = []
    missing_frames = []
    for frame_name, label in label_matches:
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
                          snapshot_max_wait_ms, expected_display_ids=(0,), app=None, app_sub=None):
    frames_dir = Path(frames_dir)
    frames_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = frames_dir / FRAME_LABEL_MANIFEST
    popen_kwargs = get_recording_popen_kwargs()
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
                result = run_process_capture_output(cmd, timeout=profile["timeout"], **popen_kwargs)
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

            frames_ok, frames_message = validate_recording_frames(frames_dir, expected_display_ids=expected_display_ids)
            if frames_ok:
                label_matches = parse_frame_label_matches(combined_output)
                labels_ok, labels_message = validate_recording_frame_labels(frames_dir, label_matches, app=app, app_sub=app_sub)
                if not labels_ok:
                    last_error = labels_message
                    break
                shutdown_ok, shutdown_message = validate_shutdown_markers(
                    combined_output,
                    expected_display_ids=expected_display_ids,
                    app=app,
                    app_sub=app_sub,
                )
                if not shutdown_ok:
                    last_error = shutdown_message
                    break
                write_frame_label_manifest(frames_dir, label_matches)
                message_parts = [frames_message]
                if labels_message:
                    message_parts.append(labels_message)
                self_check_message = get_runtime_self_check_summary(app, app_sub)
                if self_check_message:
                    message_parts.append(self_check_message)
                if shutdown_message:
                    message_parts.append(shutdown_message)
                return True, "; ".join(message_parts)

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
    EGUI_UNUSED = app  # compatibility placeholder
    EGUI_UNUSED = category
    return sorted(app_subs)


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
    elif scope == "multi-display":
        selected_apps = [app for app in MULTI_DISPLAY_APPS if app in app_sets and app not in SKIP_LIST and app not in skipped]
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
    isolated_obj_suffix = get_runtime_obj_suffix(
        app,
        app_sub=app_sub,
        bits64=bits64,
        user_cflags=user_cflags,
        recording_test=recording_test,
    )

    # Always inject RECORDING_TEST into the build signature so cached outputs stay isolated.
    recording_flag = '-DEGUI_CONFIG_FUNCTION_RECORDING_TEST=%d' % (1 if recording_test else 0)
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

    last_result = None
    for attempt in range(COMPILE_RETRY_COUNT):
        result = subprocess.run(cmd, cwd=ROOT_DIR, capture_output=True, text=True)
        last_result = result
        if result.returncode == 0:
            register_runtime_build_output_dir(build_output_dir)
            return True

        if attempt < COMPILE_RETRY_COUNT - 1:
            shutil.rmtree(build_output_dir, ignore_errors=True)
            print("(compile retry %d/%d: %s)" % (attempt + 1, COMPILE_RETRY_COUNT - 1, format_app_name(app, app_sub)), end=" ")
            time.sleep(COMPILE_RETRY_DELAY_S)

    # If a shared object-root build still fails, retry once with a fully isolated obj dir.
    if objroot_path is not None or app_obj_suffix != isolated_obj_suffix:
        fallback_cmd = ['make', get_make_job_arg(make_jobs), 'APP=%s' % app, 'PORT=pc'] + COMPILE_FAST_FLAGS
        if app_sub:
            fallback_cmd.append('APP_SUB=%s' % app_sub)
        if bits64:
            fallback_cmd.append('BITS=64')
        fallback_cmd.append('USER_CFLAGS=%s' % combined_cflags)
        fallback_cmd.append('OUTPUT_PATH=%s' % Path(build_output_dir).as_posix())
        fallback_cmd.append('APP_OBJ_SUFFIX=%s' % isolated_obj_suffix)
        shutil.rmtree(build_output_dir, ignore_errors=True)
        print("(compile fallback isolated-obj: %s)" % format_app_name(app, app_sub), end=" ")
        result = subprocess.run(fallback_cmd, cwd=ROOT_DIR, capture_output=True, text=True)
        last_result = result
        if result.returncode == 0:
            register_runtime_build_output_dir(build_output_dir)
            return True

    if last_result is not None:
        stderr_lines = (last_result.stderr or "").splitlines()[-10:]
        stdout_lines = (last_result.stdout or "").splitlines()[-10:]
        detail_lines = stderr_lines if stderr_lines else stdout_lines
        if detail_lines:
            print("\n[runtime-check] compile failure tail:")
            for line in detail_lines:
                print(line)
    return False


def run_app(app_name, output_subdir, timeout=DEFAULT_TIMEOUT, duration=RECORDING_DURATION,
            speed=RECORDING_SPEED, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
            clock_scale=RECORDING_CLOCK_SCALE,
            snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
            snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS,
            build_output_dir=None, app=None, app_sub=None):
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

    snapshot_settle_ms, snapshot_stable_cycles, snapshot_max_wait_ms = tune_recording_params_for_app(
        app_name, snapshot_settle_ms, snapshot_stable_cycles, snapshot_max_wait_ms
    )
    return run_recording_capture(exe_path, resource_path, frames_dir, timeout, duration,
                                 speed, snapshot_settle_ms, clock_scale,
                                 snapshot_stable_cycles, snapshot_max_wait_ms,
                                 expected_display_ids=get_expected_runtime_display_ids(app or app_name, app_sub),
                                 app=app or app_name,
                                 app_sub=app_sub)


def check_default_resolution(app, app_sub, bits64, explicit_timeout=None,
                             speed=RECORDING_SPEED, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
                             clock_scale=RECORDING_CLOCK_SCALE,
                             snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
                             snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS,
                             make_jobs=None, app_obj_suffix=None, objroot_path=None,
                             user_cflags=""):
    """Test app at default resolution (no CFLAGS override).
    Apps auto-quit after all actions complete; RECORDING_DURATION is a safety timeout.
    Returns (success: bool, message: str).
    """
    app_name = format_app_name(app, app_sub)
    skip_reason = get_runtime_skip_reason(app, app_sub)

    if skip_reason:
        return True, "skipped: %s" % skip_reason

    build_output_dir = get_runtime_build_output_dir(app, app_sub=app_sub, bits64=bits64, user_cflags=user_cflags)

    if not compile_app(
        app,
        app_sub,
        bits64,
        user_cflags=user_cflags,
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
                   build_output_dir=build_output_dir,
                   app=app,
                   app_sub=app_sub)



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
                     make_jobs=None, app_obj_suffix=None, objroot_path=None,
                     user_cflags=""):
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
        user_cflags=user_cflags,
    )
    return app_name, success, msg


def retry_failed_runtime_cases_serial(results, case_specs, bits64, explicit_timeout=None,
                                      speed=RECORDING_SPEED, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
                                      clock_scale=RECORDING_CLOCK_SCALE,
                                      snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
                                      snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS,
                                      user_cflags=""):
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
        case_info = build_runtime_case_info(app, app_sub, bits64, user_cflags=user_cflags)
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
            user_cflags=user_cflags,
        )
        status = "PASS" if success else "FAIL"
        print("  %s (%s)" % (status, msg))
        results[index] = (app_name, success, msg)

    return results


def run_runtime_case_batch(case_specs, bits64, explicit_timeout=None, jobs=0,
                           speed=RECORDING_SPEED, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
                           clock_scale=RECORDING_CLOCK_SCALE,
                           snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
                           snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS,
                           user_cflags=""):
    total = len(case_specs)
    if total == 0:
        return []

    parallel_jobs = resolve_parallel_jobs(jobs, total)
    make_jobs = resolve_make_job_count(parallel_jobs)
    case_infos = [build_runtime_case_info(app, app_sub, bits64, user_cflags=user_cflags) for app, app_sub in case_specs]

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
                user_cflags=user_cflags,
            )
            status = "PASS" if success else "FAIL"
            print("  %s (%s)" % (status, msg))
            results.append((app_name, success, msg))
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
            user_cflags=user_cflags,
        )

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
                user_cflags=user_cflags,
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
                    user_cflags,
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
                user_cflags,
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
        user_cflags=user_cflags,
    )


def run_full_check(bits64, speed=RECORDING_SPEED, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
                   clock_scale=RECORDING_CLOCK_SCALE,
                   snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
                   snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS,
                   jobs=0, user_cflags=""):
    """Run runtime check on all examples.
    Returns list of (app_name, success, message) tuples.
    """
    app_sets = get_example_list()
    sub_app_sets = build_sub_app_sets()
    skipped_apps = set()

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
        user_cflags=user_cflags,
    )


def run_scope_check(scope, bits64, explicit_timeout=None,
                    speed=RECORDING_SPEED, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
                    clock_scale=RECORDING_CLOCK_SCALE,
                    snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
                    snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS,
                    jobs=0, shard_count=1, shard_index=1, user_cflags=""):
    app_sets = get_example_list()
    sub_app_sets = build_sub_app_sets()
    skipped_apps = set()

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
        user_cflags=user_cflags,
    )


def run_sub_app_check(app, app_subs, bits64, explicit_timeout=None,
                      speed=RECORDING_SPEED, snapshot_settle_ms=RECORDING_SNAPSHOT_SETTLE_MS,
                      clock_scale=RECORDING_CLOCK_SCALE,
                      snapshot_stable_cycles=RECORDING_SNAPSHOT_STABLE_CYCLES,
                      snapshot_max_wait_ms=RECORDING_SNAPSHOT_MAX_WAIT_MS,
                      jobs=0, user_cflags=""):
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
        user_cflags=user_cflags,
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
  %(prog)s --app HelloBasic --app-sub button         Test one HelloBasic sub-app
  %(prog)s --app HelloVirtual --app-sub virtual_grid Test one HelloVirtual sub-app
  %(prog)s --scope multi-display --jobs 2            Test multi-display examples only
  %(prog)s --scope standard --jobs 2                 Test standard runtime examples only
  %(prog)s --scope basic --shard-count 3 --shard-index 1 --jobs 2
  %(prog)s --full-check                             Test all examples
        """
    )
    parser.add_argument('--app', type=str,
                        help='Specific app to test. For HelloBasic/HelloVirtual/HelloSizeAnalysis without --app-sub, tests all sub-apps.')
    parser.add_argument('--app-sub', type=str,
                        help='Single sub-app for HelloBasic/HelloVirtual/HelloSizeAnalysis. If omitted, all sub-apps are tested.')
    parser.add_argument('--category', type=str, help=argparse.SUPPRESS)
    parser.add_argument('--bits64', action='store_true', help='Build for 64-bit')
    parser.add_argument('--timeout', type=int, default=DEFAULT_TIMEOUT,
                        help='Run timeout in seconds (default: %d)' % DEFAULT_TIMEOUT)
    parser.add_argument('--keep-screenshots', action='store_true',
                        help='Keep screenshot files after testing')
    parser.add_argument('--full-check', action='store_true',
                        help='Test all example applications')
    parser.add_argument('--scope', choices=['standard', 'multi-display', 'basic', 'virtual', 'size-analysis'],
                        help='Run one runtime case family only, useful for CI sharding')
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
    parser.add_argument('--user-cflags', type=str, default='',
                        help='Append raw USER_CFLAGS to runtime builds (for config override / A-B probe)')
    parser.add_argument('--queue-capacity-probe', type=int, default=0,
                        help='Convenience wrapper for -DEGUI_PORT_PC_CORE_TASK_QUEUE_CAPACITY=N (0=disabled)')
    parser.add_argument('--queue-stress-probe', type=int, default=0,
                        help='Convenience wrapper for -DEGUI_MULTI_DISPLAY_CORE_TASK_STRESS_BURST_COUNT=N (0=disabled)')
    parser.add_argument('--queue-post-retry-probe', type=int, default=-1,
                        help='Convenience wrapper for -DEGUI_PORT_PC_CORE_TASK_POST_RETRY_COUNT=N (-1=disabled, 0=disable retries)')
    parser.add_argument('--queue-post-retry-delay-probe', type=int, default=-1,
                        help='Convenience wrapper for -DEGUI_PORT_PC_CORE_TASK_POST_RETRY_DELAY_MS=N (-1=disabled, 0=no delay)')
    parser.add_argument('--queue-stress-post-gap-probe', type=int, default=-1,
                        help='Convenience wrapper for -DEGUI_MULTI_DISPLAY_CORE_TASK_STRESS_POST_GAP_MS=N (-1=disabled, 0=no gap)')
    parser.add_argument('--max-core-task-retries', type=int, default=-1,
                        help='Fail runtime if any display exceeds this retry count (-1=disabled)')
    parser.add_argument('--max-single-post-core-task-retries', type=int, default=-1,
                        help='Fail runtime if any display exceeds this single-post retry burst (-1=disabled)')
    parser.add_argument('--fail-on-full-core-task-queue', action='store_true',
                        help='Fail runtime if any display peak_queue reaches queue_capacity')
    parser.add_argument('--max-shutdown-queue-wait-ms', type=int, default=-1,
                        help='Fail runtime if any display shutdown max_queue_wait_ms exceeds this threshold (-1=disabled)')
    parser.add_argument('--max-shutdown-exec-time-ms', type=int, default=-1,
                        help='Fail runtime if any display shutdown max_exec_time_ms exceeds this threshold (-1=disabled)')

    args = parser.parse_args()

    all_passed = True
    speed = args.speed
    clock_scale = args.clock_scale
    snapshot_settle_ms = args.snapshot_settle_ms
    snapshot_stable_cycles = args.snapshot_stable_cycles
    snapshot_max_wait_ms = args.snapshot_max_wait_ms
    user_cflags = args.user_cflags.strip()

    if args.queue_capacity_probe < 0:
        print("Error: --queue-capacity-probe must be >= 0")
        sys.exit(1)
    if args.queue_stress_probe < 0:
        print("Error: --queue-stress-probe must be >= 0")
        sys.exit(1)
    if args.queue_post_retry_probe < -1:
        print("Error: --queue-post-retry-probe must be >= -1")
        sys.exit(1)
    if args.queue_post_retry_delay_probe < -1:
        print("Error: --queue-post-retry-delay-probe must be >= -1")
        sys.exit(1)
    if args.queue_stress_post_gap_probe < -1:
        print("Error: --queue-stress-post-gap-probe must be >= -1")
        sys.exit(1)
    if args.max_core_task_retries < -1:
        print("Error: --max-core-task-retries must be >= -1")
        sys.exit(1)
    if args.max_single_post_core_task_retries < -1:
        print("Error: --max-single-post-core-task-retries must be >= -1")
        sys.exit(1)
    if args.max_shutdown_queue_wait_ms < -1:
        print("Error: --max-shutdown-queue-wait-ms must be >= -1")
        sys.exit(1)
    if args.max_shutdown_exec_time_ms < -1:
        print("Error: --max-shutdown-exec-time-ms must be >= -1")
        sys.exit(1)
    if args.queue_capacity_probe > 0:
        user_cflags = ("%s -DEGUI_PORT_PC_CORE_TASK_QUEUE_CAPACITY=%d" % (user_cflags, args.queue_capacity_probe)).strip()
    if args.queue_stress_probe > 0:
        user_cflags = ("%s -DEGUI_MULTI_DISPLAY_CORE_TASK_STRESS_BURST_COUNT=%d" % (user_cflags, args.queue_stress_probe)).strip()
    if args.queue_post_retry_probe >= 0:
        user_cflags = ("%s -DEGUI_PORT_PC_CORE_TASK_POST_RETRY_COUNT=%d" % (user_cflags, args.queue_post_retry_probe)).strip()
    if args.queue_post_retry_delay_probe >= 0:
        user_cflags = ("%s -DEGUI_PORT_PC_CORE_TASK_POST_RETRY_DELAY_MS=%d" % (user_cflags, args.queue_post_retry_delay_probe)).strip()
    if args.queue_stress_post_gap_probe >= 0:
        user_cflags = ("%s -DEGUI_MULTI_DISPLAY_CORE_TASK_STRESS_POST_GAP_MS=%d" % (user_cflags, args.queue_stress_post_gap_probe)).strip()

    global MAX_CORE_TASK_RETRIES
    global MAX_SINGLE_POST_CORE_TASK_RETRIES
    global FAIL_ON_FULL_CORE_TASK_QUEUE
    global MAX_SHUTDOWN_QUEUE_WAIT_MS
    global MAX_SHUTDOWN_EXEC_TIME_MS
    MAX_CORE_TASK_RETRIES = args.max_core_task_retries
    MAX_SINGLE_POST_CORE_TASK_RETRIES = args.max_single_post_core_task_retries
    FAIL_ON_FULL_CORE_TASK_QUEUE = args.fail_on_full_core_task_queue
    MAX_SHUTDOWN_QUEUE_WAIT_MS = args.max_shutdown_queue_wait_ms
    MAX_SHUTDOWN_EXEC_TIME_MS = args.max_shutdown_exec_time_ms

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
            jobs=args.jobs,
            user_cflags=user_cflags,
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
                jobs=args.jobs,
                shard_count=args.shard_count,
                shard_index=args.shard_index,
                user_cflags=user_cflags,
            )
        except ValueError as exc:
            print("Error: %s" % exc)
            sys.exit(1)
        all_passed = print_summary(results)

    elif args.app:
        if args.app == "HelloCustomWidgets" or args.category:
            print("Error: HelloCustomWidgets has moved to the standalone repository:")
            print("  %s" % CUSTOM_WIDGETS_REPO)
            sys.exit(1)

        results = []
        run_all_sub_apps = args.app in SUB_APP_ROOTS and not args.app_sub
        if run_all_sub_apps:
            sub_apps = filter_sub_apps(args.app, get_example_sub_list(args.app), args.category)
            if not sub_apps:
                print("Error: no %s sub-apps found" % args.app)
                sys.exit(1)

            print("=" * 60)
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
                user_cflags=user_cflags,
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
                                                     snapshot_max_wait_ms=snapshot_max_wait_ms,
                                                     user_cflags=user_cflags)
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
