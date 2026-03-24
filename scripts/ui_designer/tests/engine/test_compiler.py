"""Tests for CompilerEngine and BuildConfig."""

import os
from unittest.mock import MagicMock, patch

import pytest

from ui_designer.engine.compiler import BuildConfig, CompilerEngine


MAKE_DRYRUN_OUTPUT = """\
echo Compiling  : "example/HelloDesigner_1/main_page.c"
gcc -DEGUI_APP=\\"HelloDesigner_1\\" -DEGUI_PORT=EGUI_PORT_TYPE_PC -O0 -Wall -std=c99 -Iexample/HelloDesigner_1 -Isrc -Iporting/designer -c example/HelloDesigner_1/main_page.c  -o output/obj/example/HelloDesigner_1/main_page.o
echo Compiling  : "example/HelloDesigner_1/uicode.c"
gcc -DEGUI_APP=\\"HelloDesigner_1\\" -DEGUI_PORT=EGUI_PORT_TYPE_PC -O0 -Wall -std=c99 -Iexample/HelloDesigner_1 -Isrc -Iporting/designer -c example/HelloDesigner_1/uicode.c  -o output/obj/example/HelloDesigner_1/uicode.o
echo Compiling  : "porting/designer/main.c"
gcc -DEGUI_APP=\\"HelloDesigner_1\\" -DEGUI_PORT=EGUI_PORT_TYPE_PC -O0 -Wall -std=c99 -Iexample/HelloDesigner_1 -Isrc -Iporting/designer -c porting/designer/main.c  -o output/obj/porting/designer/main.o
echo Linking    : "output/main.exe"
gcc -DEGUI_APP=\\"HelloDesigner_1\\" -O0 -Wall -std=c99 -o output/main.exe output/obj/example/HelloDesigner_1/main_page.o output/obj/example/HelloDesigner_1/uicode.o output/obj/porting/designer/main.o output/libegui.a -lpthread
echo Building   : "output/main.exe"
"""


class TestBuildConfigExtract:
    def _make_result(self, stdout="", returncode=0):
        result = MagicMock()
        result.stdout = stdout
        result.stderr = ""
        result.returncode = returncode
        return result

    @patch("subprocess.run")
    @patch("os.path.getmtime", return_value=1000.0)
    def test_extract_parses_compile_commands(self, mock_mtime, mock_run):
        mock_run.return_value = self._make_result(MAKE_DRYRUN_OUTPUT)
        cfg = BuildConfig.extract("/project", "HelloDesigner_1", "example")

        assert cfg is not None
        assert "gcc" in cfg.compile_cmd_prefix
        assert "-O0" in cfg.compile_cmd_prefix
        assert "-Isrc" in cfg.compile_cmd_prefix

    @patch("subprocess.run")
    @patch("os.path.getmtime", return_value=1000.0)
    def test_extract_parses_src_to_obj_mapping(self, mock_mtime, mock_run):
        mock_run.return_value = self._make_result(MAKE_DRYRUN_OUTPUT)
        cfg = BuildConfig.extract("/project", "HelloDesigner_1", "example")

        assert "main_page.c" in cfg.src_to_obj
        assert "uicode.c" in cfg.src_to_obj
        assert "main.c" in cfg.src_to_obj
        src, obj = cfg.src_to_obj["uicode.c"]
        assert src == "example/HelloDesigner_1/uicode.c"
        assert obj == "output/obj/example/HelloDesigner_1/uicode.o"

    @patch("subprocess.run")
    @patch("os.path.getmtime", return_value=1000.0)
    def test_extract_parses_link_command(self, mock_mtime, mock_run):
        mock_run.return_value = self._make_result(MAKE_DRYRUN_OUTPUT)
        cfg = BuildConfig.extract("/project", "HelloDesigner_1", "example")

        assert "output/main.exe" in cfg.link_cmd
        assert "libegui.a" in cfg.link_cmd
        assert "-lpthread" in cfg.link_cmd

    @patch("subprocess.run")
    @patch("os.path.getmtime", return_value=1000.0)
    def test_extract_generates_presplit_args(self, mock_mtime, mock_run):
        mock_run.return_value = self._make_result(MAKE_DRYRUN_OUTPUT)
        cfg = BuildConfig.extract("/project", "HelloDesigner_1", "example")

        assert isinstance(cfg.compile_cmd_args, list)
        assert cfg.compile_cmd_args[0] == "gcc"
        assert isinstance(cfg.link_cmd_args, list)
        assert cfg.link_cmd_args[0] == "gcc"

    @patch("subprocess.run")
    def test_extract_returns_none_on_make_failure(self, mock_run):
        mock_run.return_value = self._make_result("error", returncode=1)
        assert BuildConfig.extract("/project", "HelloDesigner_1", "example") is None

    @patch("subprocess.run")
    def test_extract_returns_none_on_empty_output(self, mock_run):
        mock_run.return_value = self._make_result("")
        assert BuildConfig.extract("/project", "HelloDesigner_1", "example") is None

    @patch("subprocess.run", side_effect=FileNotFoundError)
    def test_extract_returns_none_when_make_missing(self, mock_run):
        assert BuildConfig.extract("/project", "HelloDesigner_1", "example") is None


class TestBuildConfigIsValid:
    def test_valid_when_mtimes_match(self, tmp_path):
        cfg = BuildConfig()
        makefile = tmp_path / "Makefile"
        makefile.write_text("test")
        cfg._makefile_mtimes = {"Makefile": os.path.getmtime(str(makefile))}
        assert cfg.is_valid(str(tmp_path))

    def test_invalid_when_mtime_changes(self, tmp_path):
        cfg = BuildConfig()
        makefile = tmp_path / "Makefile"
        makefile.write_text("test")
        cfg._makefile_mtimes = {"Makefile": os.path.getmtime(str(makefile)) - 1}
        assert not cfg.is_valid(str(tmp_path))

    def test_invalid_when_file_missing(self, tmp_path):
        cfg = BuildConfig()
        cfg._makefile_mtimes = {"missing": 1.0}
        assert not cfg.is_valid(str(tmp_path))

    def test_invalid_when_no_mtimes(self, tmp_path):
        cfg = BuildConfig()
        assert not cfg.is_valid(str(tmp_path))


class TestCompilerFastPath:
    def _make_engine(self, tmp_path):
        engine = CompilerEngine.__new__(CompilerEngine)
        engine.project_root = str(tmp_path)
        engine.app_dir = str(tmp_path / "example" / "TestApp")
        engine.app_name = "TestApp"
        engine.app_root_arg = "example"
        engine._app_root_error = ""
        engine._compile_count = 0
        engine._last_changed_files = []
        engine._build_config = None
        engine._screen_width = 240
        engine._screen_height = 320
        engine._last_runtime_error = ""
        engine.bridge = MagicMock()
        return engine

    def _make_config(self, tmp_path):
        cfg = BuildConfig()
        cfg.compile_cmd_prefix = "gcc -O0 -Wall -Isrc"
        cfg.link_cmd = "gcc -o output/main.exe output/obj/uicode.o output/libegui.a"
        cfg.compile_cmd_args = ["gcc", "-O0", "-Wall", "-Isrc"]
        cfg.link_cmd_args = ["gcc", "-o", "output/main.exe", "output/obj/uicode.o", "output/libegui.a"]
        cfg.src_to_obj = {
            "uicode.c": ("example/TestApp/uicode.c", "output/obj/uicode.o"),
        }
        makefile = tmp_path / "Makefile"
        makefile.write_text("test")
        cfg._makefile_mtimes = {"Makefile": os.path.getmtime(str(makefile))}
        return cfg

    @patch("subprocess.run")
    @patch("subprocess.Popen")
    def test_fast_path_used_when_config_valid(self, mock_popen, mock_run, tmp_path):
        engine = self._make_engine(tmp_path)
        engine._build_config = self._make_config(tmp_path)
        engine._last_changed_files = ["uicode.c"]
        (tmp_path / "output").mkdir()
        (tmp_path / "output" / "libegui.a").write_bytes(b"lib")

        fake_proc = MagicMock()
        fake_proc.communicate.return_value = (b"", b"")
        fake_proc.returncode = 0
        mock_popen.return_value = fake_proc
        mock_run.return_value = MagicMock(returncode=0, stdout="", stderr="")

        success, _ = engine.compile()
        assert success
        assert mock_popen.call_count == 1
        assert mock_popen.call_args[1]["shell"] is False
        assert mock_run.call_args[1]["shell"] is False

    @patch("subprocess.run")
    def test_falls_back_to_make_without_config(self, mock_run, tmp_path):
        engine = self._make_engine(tmp_path)
        engine._last_changed_files = ["uicode.c"]
        mock_run.return_value = MagicMock(returncode=0, stdout="ok\n", stderr="")

        success, _ = engine.compile()
        assert success
        assert mock_run.call_args_list[0][0][0][0] == "make"

    @patch("subprocess.run")
    def test_falls_back_to_make_when_no_libegui(self, mock_run, tmp_path):
        engine = self._make_engine(tmp_path)
        engine._build_config = self._make_config(tmp_path)
        engine._last_changed_files = ["uicode.c"]
        mock_run.return_value = MagicMock(returncode=0, stdout="ok\n", stderr="")

        success, _ = engine.compile()
        assert success
        assert mock_run.call_args_list[0][0][0][0] == "make"

    @patch("subprocess.run")
    def test_falls_back_when_unknown_file(self, mock_run, tmp_path):
        engine = self._make_engine(tmp_path)
        engine._build_config = self._make_config(tmp_path)
        engine._last_changed_files = ["other.c"]
        (tmp_path / "output").mkdir()
        (tmp_path / "output" / "libegui.a").write_bytes(b"lib")
        mock_run.return_value = MagicMock(returncode=0, stdout="ok\n", stderr="")

        success, _ = engine.compile()
        assert success
        assert mock_run.call_args_list[0][0][0][0] == "make"

    @patch("subprocess.run")
    @patch("subprocess.Popen")
    def test_fast_compile_failure_falls_back_to_make(self, mock_popen, mock_run, tmp_path):
        engine = self._make_engine(tmp_path)
        engine._build_config = self._make_config(tmp_path)
        engine._last_changed_files = ["uicode.c"]
        (tmp_path / "output").mkdir()
        (tmp_path / "output" / "libegui.a").write_bytes(b"lib")

        fake_proc = MagicMock()
        fake_proc.communicate.return_value = (b"error\n", b"")
        fake_proc.returncode = 1
        fake_proc.kill = MagicMock()
        mock_popen.return_value = fake_proc
        mock_run.side_effect = [
            MagicMock(returncode=0, stdout="ok\n", stderr=""),
            MagicMock(returncode=0, stdout=MAKE_DRYRUN_OUTPUT, stderr=""),
        ]

        with patch("os.path.getmtime", return_value=1000.0):
            success, _ = engine.compile()
        assert success

    @patch("subprocess.run")
    def test_make_compile_extracts_config(self, mock_run, tmp_path):
        engine = self._make_engine(tmp_path)
        make_result = MagicMock(returncode=0, stdout="ok\n", stderr="")
        dryrun_result = MagicMock(returncode=0, stdout=MAKE_DRYRUN_OUTPUT, stderr="")
        mock_run.side_effect = [make_result, dryrun_result]

        with patch("os.path.getmtime", return_value=1000.0):
            success, _ = engine._make_compile()

        assert success
        assert engine._build_config is not None
        assert "uicode.c" in engine._build_config.src_to_obj

    def test_write_uicode_sets_changed(self, tmp_path):
        engine = self._make_engine(tmp_path)
        os.makedirs(engine.app_dir, exist_ok=True)
        engine.write_uicode("int main() {}")
        assert engine._last_changed_files == ["uicode.c"]
        assert os.path.isfile(os.path.join(engine.app_dir, "uicode.c"))


class TestCompilerRuntime:
    def _make_engine(self, tmp_path):
        engine = CompilerEngine.__new__(CompilerEngine)
        engine.project_root = str(tmp_path)
        engine.app_dir = str(tmp_path / "app")
        engine.app_name = "TestApp"
        engine.app_root_arg = "example"
        engine._app_root_error = ""
        engine._compile_count = 0
        engine._screen_width = 240
        engine._screen_height = 320
        engine._last_runtime_error = ""
        engine.bridge = MagicMock()
        return engine

    def test_validate_preview_reports_bridge_error(self, tmp_path):
        engine = self._make_engine(tmp_path)
        engine.bridge.is_running = False
        ready, err = engine.validate_preview()
        assert not ready
        assert "not running" in err

    def test_validate_preview_accepts_correct_frame(self, tmp_path):
        engine = self._make_engine(tmp_path)
        engine.bridge.is_running = True
        engine.bridge.render.return_value = bytes(240 * 320 * 3)
        ready, err = engine.validate_preview()
        assert ready
        assert err == ""

    def test_get_last_runtime_error_prefers_app_root_error(self, tmp_path):
        engine = self._make_engine(tmp_path)
        engine._app_root_error = "External app must be on the same drive as the SDK root"
        assert "same drive" in engine.get_last_runtime_error()
        assert engine.can_build() is False
        assert "same drive" in engine.get_build_error()

    def test_init_records_cross_drive_error(self, tmp_path):
        with patch("ui_designer.engine.compiler.compute_make_app_root_arg", side_effect=ValueError("cross drive")):
            with patch.object(CompilerEngine, "_cleanup_stale_processes"):
                engine = CompilerEngine(str(tmp_path), str(tmp_path / "app"), "TestApp")
        assert engine.get_last_runtime_error() == "cross drive"
        assert engine.get_build_error() == "cross drive"
        assert not engine.is_exe_ready()
