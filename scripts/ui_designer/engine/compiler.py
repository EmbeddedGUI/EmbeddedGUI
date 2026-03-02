"""Compiler engine for EmbeddedGUI Designer - manages build and exe process."""

import os
import re
import shlex
import shutil
import subprocess
import sys
import time

from PyQt5.QtCore import QThread, pyqtSignal

from .designer_bridge import DesignerBridge


class BuildConfig:
    """Cached build configuration extracted from ``make V=1 --dry-run``.

    Keeps gcc command templates in sync with the Makefile automatically.
    Invalidates when any watched Makefile changes (by mtime).
    """

    def __init__(self):
        self.compile_cmd_prefix = ""   # "gcc -D... -O0 -Wall ... -Iexample/... -Isrc -Iporting/..."
        self.link_cmd = ""             # full link command string
        self.compile_cmd_args = []     # pre-split for shell=False
        self.link_cmd_args = []        # pre-split for shell=False
        self.src_to_obj = {}           # "main_page.c" -> ("example/.../main_page.c", "output/obj/.../main_page.o")
        self._makefile_mtimes = {}     # path -> mtime for invalidation

    def is_valid(self, project_root):
        """Return False if any watched Makefile has been modified."""
        for path, mtime in self._makefile_mtimes.items():
            full = os.path.join(project_root, path)
            try:
                if os.path.getmtime(full) != mtime:
                    return False
            except OSError:
                return False
        return bool(self._makefile_mtimes)

    @staticmethod
    def extract(project_root, app_name):
        """Run ``make V=1 --dry-run`` and parse gcc commands.

        Returns a populated BuildConfig, or None on failure.
        """
        try:
            result = subprocess.run(
                ["make", "V=1", "--dry-run", "--always-make", "main.exe",
                 f"APP={app_name}", "PORT=designer",
                 "COMPILE_DEBUG=", "COMPILE_OPT_LEVEL=-O0"],
                cwd=project_root,
                capture_output=True, text=True, timeout=30,
            )
            if result.returncode != 0:
                return None
        except (subprocess.TimeoutExpired, FileNotFoundError):
            return None

        cfg = BuildConfig()
        lines = result.stdout.splitlines()

        for line in lines:
            line = line.strip()
            if not line.startswith("gcc "):
                continue

            if " -c " in line:
                # Compile command: gcc {flags} -c {src} -o {obj}
                m = re.match(r"(gcc .+?)\s+-c\s+(\S+)\s+-o\s+(\S+)", line)
                if m:
                    if not cfg.compile_cmd_prefix:
                        cfg.compile_cmd_prefix = m.group(1)
                    src_path = m.group(2)
                    obj_path = m.group(3)
                    basename = os.path.basename(src_path)
                    cfg.src_to_obj[basename] = (src_path, obj_path)

            elif " -o output/main.exe " in line or " -o output\\main.exe " in line:
                # Link command
                cfg.link_cmd = line

        if not cfg.compile_cmd_prefix or not cfg.link_cmd:
            return None

        # Pre-split for shell=False (avoids cmd.exe overhead ~9ms/call)
        try:
            cfg.compile_cmd_args = shlex.split(cfg.compile_cmd_prefix, posix=True)
            cfg.link_cmd_args = shlex.split(cfg.link_cmd, posix=True)
        except ValueError:
            return None  # Malformed command, fall back to make

        # Record Makefile mtimes for invalidation
        makefile_paths = [
            "Makefile",
            os.path.join("porting", "designer", "Makefile.base"),
            os.path.join("example", app_name, "build.mk"),
        ]
        for p in makefile_paths:
            full = os.path.join(project_root, p)
            try:
                cfg._makefile_mtimes[p] = os.path.getmtime(full)
            except OSError:
                pass

        return cfg


class CompileWorker(QThread):
    """Background worker thread for compilation.

    Signals:
        finished(success, message, old_process): Emitted when compile+start completes
        log(message): Emitted for logging during compile
    """

    finished = pyqtSignal(bool, str, object)  # success, message, old_process
    log = pyqtSignal(str, str)  # message, msg_type

    def __init__(self, compiler, code=None, files_dict=None, parent=None):
        super().__init__(parent)
        self.compiler = compiler
        self.code = code
        self.files_dict = files_dict
        self._old_process = None

    def run(self):
        """Execute compile_and_run in background thread."""
        import time
        t0 = time.time()

        # Write code — multi-file or single-file
        t1 = time.time()
        written = []
        if self.files_dict:
            written = self.compiler.write_project_files(self.files_dict)
        elif self.code:
            self.compiler.write_uicode(self.code)
            written = ["uicode.c"]
        t2 = time.time()
        if written:
            self.log.emit(f"Write files: {(t2-t1)*1000:.0f}ms ({len(written)} changed: {', '.join(written)})", "info")
        else:
            self.log.emit(f"Write files: {(t2-t1)*1000:.0f}ms (no changes)", "info")

        # Compile
        use_fast = self.compiler._build_config is not None and written
        self.log.emit(f"Compiling ({'fast' if use_fast else 'make'})...", "info")
        success, output = self.compiler.compile()
        t3 = time.time()
        self.log.emit(f"Compile completed: {(t3-t2)*1000:.0f}ms", "info")

        if not success:
            self.finished.emit(False, f"Compilation failed:\n{output}", None)
            return

        # Copy and start new exe via bridge
        self.log.emit("Starting headless preview...", "info")
        ok, msg, _ = self.compiler._copy_and_start()
        t4 = time.time()
        self.log.emit(f"Copy and start: {(t4-t3)*1000:.0f}ms", "info")

        if not ok:
            self.finished.emit(False, msg, None)
            return

        self.log.emit(f"Total compile time: {(t4-t0)*1000:.0f}ms", "success")
        self.finished.emit(True, f"Build #{self.compiler._compile_count} OK", None)


class PrecompileWorker(QThread):
    """Background worker for precompilation (compile only, no exe start)."""

    finished = pyqtSignal(bool, str)  # success, message

    def __init__(self, compiler, parent=None):
        super().__init__(parent)
        self.compiler = compiler

    def run(self):
        """Execute compile in background thread."""
        success, output = self.compiler.compile()
        if success:
            self.finished.emit(True, "Precompile OK")
        else:
            self.finished.emit(False, output)


class CompilerEngine:
    """Manages incremental compilation and exe process lifecycle.

    Uses double-buffering: the exe always runs from a copy (designer_run_0.exe
    or designer_run_1.exe) so that make can freely overwrite output/main.exe
    while the old preview is still running. This eliminates flash on recompile.
    """

    def __init__(self, project_root, app_name="HelloDesigner"):
        self.project_root = project_root
        self.app_name = app_name
        self.process = None  # kept for backward compat, not used by bridge
        self.bridge = DesignerBridge()
        self._compile_count = 0
        self._run_index = 0  # alternates 0/1 for double buffering
        self._screen_width = 240
        self._screen_height = 320
        self._build_config = None  # BuildConfig cache for fast compile
        self._last_changed_files = []  # tracks files changed in last write

        # Clean up any stale processes from previous abnormal exits
        self._cleanup_stale_processes()

    def _cleanup_stale_processes(self):
        """Kill any leftover designer_run_*.exe processes from previous sessions."""
        if sys.platform != "win32":
            return

        exe_names = ["designer_run_0.exe", "designer_run_1.exe"]

        try:
            # Try using taskkill command (always available on Windows)
            for exe_name in exe_names:
                try:
                    subprocess.run(
                        ["taskkill", "/F", "/IM", exe_name],
                        capture_output=True,
                        timeout=5,
                    )
                except (subprocess.TimeoutExpired, FileNotFoundError):
                    pass

            # Also try to remove the exe files
            for i in range(2):
                path = self._run_exe_path(i)
                try:
                    if os.path.exists(path):
                        os.remove(path)
                except OSError:
                    pass  # File may still be locked briefly after kill

        except Exception:
            pass  # Don't fail startup due to cleanup issues

    @property
    def uicode_path(self):
        return os.path.join(self.project_root, "example", self.app_name, "uicode.c")

    @property
    def app_dir(self):
        return os.path.join(self.project_root, "example", self.app_name)

    @property
    def exe_path(self):
        return os.path.join(self.project_root, "output", "main.exe")

    def _run_exe_path(self, index):
        return os.path.join(self.project_root, "output", f"designer_run_{index}.exe")

    def write_uicode(self, code):
        """Write generated C code to uicode.c."""
        with open(self.uicode_path, "w", encoding="utf-8") as f:
            f.write(code)
        self._last_changed_files = ["uicode.c"]

    def write_project_files(self, files_dict):
        """Write multiple generated C files to the app directory.

        Respects file ownership categories:
          - GENERATED_ALWAYS / GENERATED_PRESERVED: always write (with
            user code preservation for preserved files)
          - USER_OWNED: only write if file does not exist (skeleton)

        Handles both old-style dict[str, str] and new-style
        dict[str, tuple[str, str]] (content, category) formats.

        Only writes files if content has changed, to avoid triggering
        unnecessary recompilation due to timestamp changes.

        Args:
            files_dict: dict[str, str | tuple[str, str]] mapping
                filename to content (or (content, category))

        Returns:
            list[str]: List of filenames that were actually written (changed)
        """
        from ..generator.user_code_preserver import preserve_user_code
        from ..generator.code_generator import (
            GENERATED_ALWAYS, GENERATED_PRESERVED, USER_OWNED,
        )

        app_dir = self.app_dir
        os.makedirs(app_dir, exist_ok=True)
        written = []
        for filename, value in files_dict.items():
            # Support both (content, category) tuples and plain strings
            if isinstance(value, tuple):
                content, category = value
            else:
                content = value
                category = GENERATED_ALWAYS

            filepath = os.path.join(app_dir, filename)

            # User-owned files: skip if already exists
            if category == USER_OWNED and os.path.isfile(filepath):
                continue

            # Preserve USER CODE regions for preserved files
            if category == GENERATED_PRESERVED:
                content = preserve_user_code(filepath, content)

            # Only write if content changed (preserve timestamp for unchanged)
            if os.path.exists(filepath):
                try:
                    with open(filepath, "r", encoding="utf-8") as f:
                        existing = f.read()
                    if existing == content:
                        continue  # Skip unchanged file
                except (IOError, UnicodeDecodeError):
                    pass  # Write anyway if read fails
            with open(filepath, "w", encoding="utf-8") as f:
                f.write(content)
            written.append(filename)
        self._last_changed_files = [f for f in written if f.endswith(".c")]
        return written

    def compile(self, changed_files=None):
        """Run incremental compilation. Returns (success, output).

        Uses a two-tier strategy:
        1. Fast path: if libegui.a exists and BuildConfig is cached, invoke gcc
           directly for only the changed files, then link. Skips make overhead.
        2. Slow path: fall back to make for full builds or when fast path fails.

        The BuildConfig is extracted from ``make V=1 --dry-run`` after the first
        successful make build, keeping gcc flags in sync with the Makefile.

        Args:
            changed_files: list of changed .c basenames (e.g. ["uicode.c"]).
                If None, uses self._last_changed_files from write_project_files().
        """
        if changed_files is None:
            changed_files = self._last_changed_files

        lib_path = os.path.join(self.project_root, "output", "libegui.a")
        can_fast = (
            changed_files
            and os.path.exists(lib_path)
            and self._build_config is not None
            and self._build_config.is_valid(self.project_root)
            and all(f in self._build_config.src_to_obj for f in changed_files)
        )

        if can_fast:
            success, output = self._fast_compile(changed_files)
            if success:
                self._compile_count += 1
                return success, output
            # Fast path failed — fall through to make

        return self._make_compile()

    def _make_compile(self):
        """Full make-based compilation (slow path). Also refreshes BuildConfig."""
        # Delete stale output/main.exe to force re-link.  The exe is shared
        # across all apps, so switching apps (e.g. 240x320 → 320x480) can
        # leave a stale binary that make considers up-to-date.
        exe = os.path.join(self.project_root, "output", "main.exe")
        try:
            if os.path.exists(exe):
                os.remove(exe)
        except OSError:
            pass

        try:
            result = subprocess.run(
                ["make", "-j", "main.exe", f"APP={self.app_name}",
                 "PORT=designer",
                 "COMPILE_DEBUG=", "COMPILE_OPT_LEVEL=-O0"],
                cwd=self.project_root,
                capture_output=True, text=True, timeout=60,
            )
            self._compile_count += 1
            success = result.returncode == 0
            output = result.stdout + result.stderr

            # After successful make, extract build config for future fast compiles
            if success:
                cfg = BuildConfig.extract(self.project_root, self.app_name)
                if cfg:
                    self._build_config = cfg

            return success, output
        except subprocess.TimeoutExpired:
            return False, "Compilation timed out"
        except FileNotFoundError:
            return False, "make not found in PATH"

    def _fast_compile(self, changed_files):
        """Direct gcc compilation bypassing make. Returns (success, output).

        Only compiles the changed .c files (in parallel), then re-links all
        app objects against the pre-built libegui.a.
        Uses shell=False with pre-split args to avoid cmd.exe overhead.
        """
        cfg = self._build_config
        outputs = []

        # Step 1: compile changed files in parallel
        procs = []
        for basename in changed_files:
            src, obj = cfg.src_to_obj[basename]
            args = cfg.compile_cmd_args + ["-c", src, "-o", obj]
            try:
                p = subprocess.Popen(
                    args, shell=False,
                    cwd=self.project_root,
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                )
                procs.append(p)
            except OSError as e:
                return False, f"Fast compile failed: {e}"

        # Wait for all compiles
        for p in procs:
            try:
                stdout, stderr = p.communicate(timeout=30)
                outputs.append(stdout.decode(errors="replace") + stderr.decode(errors="replace"))
                if p.returncode != 0:
                    # Kill remaining
                    for q in procs:
                        try:
                            q.kill()
                        except OSError:
                            pass
                    return False, "".join(outputs)
            except subprocess.TimeoutExpired:
                p.kill()
                return False, "Fast compile timed out"

        # Step 2: link
        try:
            r = subprocess.run(
                cfg.link_cmd_args, shell=False,
                cwd=self.project_root,
                capture_output=True, text=True, timeout=30,
            )
            outputs.append(r.stdout + r.stderr)
            if r.returncode != 0:
                return False, "".join(outputs)
        except (subprocess.TimeoutExpired, FileNotFoundError) as e:
            return False, f"Fast link failed: {e}"

        return True, "".join(outputs)

    def stop_exe(self):
        """Stop the running exe process."""
        self.bridge.stop()
        self.process = None

    def _copy_and_start(self):
        """Copy main.exe to a run slot and start it via bridge.
        Returns (success, message, None).

        Uses seamless swap: starts the new process first (old bridge still
        serves frames during startup), then stops the old one.
        """
        if not os.path.exists(self.exe_path):
            return False, f"Exe not found: {self.exe_path}", None

        # Pick the OTHER slot (not the one currently running)
        new_index = 1 - self._run_index
        new_path = self._run_exe_path(new_index)

        try:
            # Hard link is near-instant vs copy (~5ms savings)
            if os.path.exists(new_path):
                os.remove(new_path)
            try:
                os.link(self.exe_path, new_path)
            except OSError:
                # Fall back to copy if hard link fails (cross-device, etc.)
                shutil.copy2(self.exe_path, new_path)
        except OSError as e:
            return False, f"Failed to copy exe: {e}", None

        try:
            resource_path = os.path.join(
                self.project_root, "output", "app_egui_resource_merge.bin"
            )
            # Start new bridge first (old one still running, serves frames)
            new_bridge = DesignerBridge()
            new_bridge.start(new_path, resource_path,
                             self._screen_width, self._screen_height)
            # New process ready — now stop old one
            old_bridge = self.bridge
            self.bridge = new_bridge
            self._run_index = new_index
            old_bridge.stop()
            return True, "Started headless preview", None
        except Exception as e:
            return False, f"Failed to start bridge: {e}", None

    def set_screen_size(self, width, height):
        """Set screen dimensions for bridge communication.

        If dimensions change, invalidates the build config cache so the
        next compile goes through a full make rebuild.  This ensures that
        egui_port_designer.c (which sizes its framebuffer from
        app_egui_config.h) is recompiled with the new resolution.
        """
        if width != self._screen_width or height != self._screen_height:
            self._build_config = None
        self._screen_width = width
        self._screen_height = height

    def get_frame(self):
        """Get current frame as RGB888 bytes, or None if not running."""
        if self.bridge.is_running:
            try:
                return self.bridge.render()
            except (ConnectionError, RuntimeError, OSError, BrokenPipeError):
                return None
        return None

    def inject_touch(self, action, x, y):
        """Forward touch event to the running exe."""
        if self.bridge.is_running:
            try:
                self.bridge.inject_touch(action, x, y)
            except (ConnectionError, OSError, BrokenPipeError):
                pass

    def inject_key(self, key_type, code):
        """Forward key event to the running exe."""
        if self.bridge.is_running:
            try:
                self.bridge.inject_key(key_type, code)
            except (ConnectionError, OSError, BrokenPipeError):
                pass

    def compile_and_run(self, code):
        """Full cycle: write code -> compile -> start bridge -> return.

        Returns (success, message, None).
        """
        # Write code
        self.write_uicode(code)

        # Compile
        success, output = self.compile()
        if not success:
            return False, f"Compilation failed:\n{output}", None

        # Copy new exe and start bridge
        ok, msg, _ = self._copy_and_start()
        if not ok:
            return False, msg, None

        return True, f"Build #{self._compile_count} OK", None

    def compile_and_run_async(self, code, callback, files_dict=None):
        """Asynchronous version - runs compile in background thread.

        Args:
            code: Generated C code to compile (single-file mode, can be None)
            callback: Function(success, message, old_process) called when done
            files_dict: dict of filename->content (multi-file mode)

        Returns:
            CompileWorker: The worker thread (caller should keep reference)
        """
        worker = CompileWorker(self, code=code, files_dict=files_dict)
        worker.finished.connect(callback)
        worker.start()
        return worker

    def is_exe_ready(self):
        """Check if main.exe exists."""
        return os.path.exists(self.exe_path)

    def precompile_async(self, callback):
        """Start background precompilation (compile only, no exe start).

        Args:
            callback: Function(success, message) called when done

        Returns:
            PrecompileWorker: The worker thread
        """
        worker = PrecompileWorker(self)
        worker.finished.connect(callback)
        worker.start()
        return worker

    def cleanup(self):
        """Cleanup on exit."""
        self.stop_exe()
        # Clean up run copies
        for i in range(2):
            path = self._run_exe_path(i)
            try:
                if os.path.exists(path):
                    os.remove(path)
            except OSError:
                pass
