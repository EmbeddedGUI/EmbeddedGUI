"""Tests for CompilerEngine and BuildConfig."""

import os
import time
import pytest
from unittest.mock import patch, MagicMock

from ui_designer.engine.compiler import BuildConfig, CompilerEngine


# ---------------------------------------------------------------------------
# BuildConfig.extract tests
# ---------------------------------------------------------------------------

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
    """Test BuildConfig.extract parsing of make --dry-run output."""

    def _make_result(self, stdout="", returncode=0):
        r = MagicMock()
        r.stdout = stdout
        r.stderr = ""
        r.returncode = returncode
        return r

    @patch("subprocess.run")
    @patch("os.path.getmtime", return_value=1000.0)
    def test_extract_parses_compile_commands(self, mock_mtime, mock_run):
        mock_run.return_value = self._make_result(MAKE_DRYRUN_OUTPUT)
        cfg = BuildConfig.extract("/project", "HelloDesigner_1")

        assert cfg is not None
        assert "gcc" in cfg.compile_cmd_prefix
        assert "-O0" in cfg.compile_cmd_prefix
        assert "-Isrc" in cfg.compile_cmd_prefix

    @patch("subprocess.run")
    @patch("os.path.getmtime", return_value=1000.0)
    def test_extract_parses_src_to_obj_mapping(self, mock_mtime, mock_run):
        mock_run.return_value = self._make_result(MAKE_DRYRUN_OUTPUT)
        cfg = BuildConfig.extract("/project", "HelloDesigner_1")

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
        cfg = BuildConfig.extract("/project", "HelloDesigner_1")

        assert "output/main.exe" in cfg.link_cmd
        assert "libegui.a" in cfg.link_cmd
        assert "-lpthread" in cfg.link_cmd

    @patch("subprocess.run")
    @patch("os.path.getmtime", return_value=1000.0)
    def test_extract_generates_presplit_args(self, mock_mtime, mock_run):
        mock_run.return_value = self._make_result(MAKE_DRYRUN_OUTPUT)
        cfg = BuildConfig.extract("/project", "HelloDesigner_1")

        assert isinstance(cfg.compile_cmd_args, list)
        assert cfg.compile_cmd_args[0] == "gcc"
        assert "-O0" in cfg.compile_cmd_args
        assert isinstance(cfg.link_cmd_args, list)
        assert cfg.link_cmd_args[0] == "gcc"
        assert "output/main.exe" in cfg.link_cmd_args

    @patch("subprocess.run")
    def test_extract_returns_none_on_make_failure(self, mock_run):
        mock_run.return_value = self._make_result("error", returncode=1)
        cfg = BuildConfig.extract("/project", "HelloDesigner_1")
        assert cfg is None

    @patch("subprocess.run")
    def test_extract_returns_none_on_empty_output(self, mock_run):
        mock_run.return_value = self._make_result("")
        cfg = BuildConfig.extract("/project", "HelloDesigner_1")
        assert cfg is None

    @patch("subprocess.run", side_effect=FileNotFoundError)
    def test_extract_returns_none_when_make_missing(self, mock_run):
        cfg = BuildConfig.extract("/project", "HelloDesigner_1")
        assert cfg is None


# ---------------------------------------------------------------------------
# BuildConfig.is_valid tests
# ---------------------------------------------------------------------------

class TestBuildConfigIsValid:
    def test_valid_when_mtimes_match(self, tmp_path):
        cfg = BuildConfig()
        f = tmp_path / "Makefile"
        f.write_text("test")
        cfg._makefile_mtimes = {"Makefile": os.path.getmtime(str(f))}
        assert cfg.is_valid(str(tmp_path))

    def test_invalid_when_mtime_changes(self, tmp_path):
        cfg = BuildConfig()
        f = tmp_path / "Makefile"
        f.write_text("test")
        cfg._makefile_mtimes = {"Makefile": os.path.getmtime(str(f)) - 1}
        assert not cfg.is_valid(str(tmp_path))

    def test_invalid_when_file_missing(self, tmp_path):
        cfg = BuildConfig()
        cfg._makefile_mtimes = {"nonexistent": 1000.0}
        assert not cfg.is_valid(str(tmp_path))

    def test_invalid_when_no_mtimes(self, tmp_path):
        cfg = BuildConfig()
        assert not cfg.is_valid(str(tmp_path))


# ---------------------------------------------------------------------------
# CompilerEngine.compile fast path tests
# ---------------------------------------------------------------------------

class TestCompilerFastPath:
    def _make_engine(self, tmp_path):
        engine = CompilerEngine.__new__(CompilerEngine)
        engine.project_root = str(tmp_path)
        engine.app_name = "TestApp"
        engine._compile_count = 0
        engine._last_changed_files = []
        engine._build_config = None
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
        # Create a fake Makefile for mtime tracking
        mk = tmp_path / "Makefile"
        mk.write_text("test")
        cfg._makefile_mtimes = {"Makefile": os.path.getmtime(str(mk))}
        return cfg

    @patch("subprocess.run")
    @patch("subprocess.Popen")
    def test_fast_path_used_when_config_valid(self, mock_popen, mock_run, tmp_path):
        engine = self._make_engine(tmp_path)
        engine._build_config = self._make_config(tmp_path)
        engine._last_changed_files = ["uicode.c"]

        # Create libegui.a
        (tmp_path / "output").mkdir()
        (tmp_path / "output" / "libegui.a").write_bytes(b"lib")

        # Mock Popen for compile step
        fake_proc = MagicMock()
        fake_proc.communicate.return_value = (b"", b"")
        fake_proc.returncode = 0
        mock_popen.return_value = fake_proc

        # Mock run for link step
        mock_run.return_value = MagicMock(returncode=0, stdout="", stderr="")

        success, output = engine.compile()
        assert success
        # Popen called for compile with shell=False and arg list
        assert mock_popen.call_count == 1
        popen_args = mock_popen.call_args
        assert popen_args[1].get("shell") is False
        assert isinstance(popen_args[0][0], list)
        # run called for link with shell=False and arg list
        assert mock_run.call_count == 1
        assert mock_run.call_args[1].get("shell") is False
        assert isinstance(mock_run.call_args[0][0], list)

    @patch("subprocess.run")
    def test_falls_back_to_make_without_config(self, mock_run, tmp_path):
        engine = self._make_engine(tmp_path)
        engine._build_config = None
        engine._last_changed_files = ["uicode.c"]

        mock_run.return_value = MagicMock(
            returncode=0, stdout="ok\n", stderr=""
        )

        success, _ = engine.compile()
        assert success
        # Should have called make (not shell=True)
        call_args = mock_run.call_args_list[0]
        assert call_args[0][0][0] == "make"

    @patch("subprocess.run")
    def test_falls_back_to_make_when_no_libegui(self, mock_run, tmp_path):
        engine = self._make_engine(tmp_path)
        engine._build_config = self._make_config(tmp_path)
        engine._last_changed_files = ["uicode.c"]
        # No libegui.a exists

        mock_run.return_value = MagicMock(
            returncode=0, stdout="ok\n", stderr=""
        )

        success, _ = engine.compile()
        assert success
        call_args = mock_run.call_args_list[0]
        assert call_args[0][0][0] == "make"

    @patch("subprocess.run")
    def test_falls_back_when_unknown_file(self, mock_run, tmp_path):
        engine = self._make_engine(tmp_path)
        engine._build_config = self._make_config(tmp_path)
        engine._last_changed_files = ["new_file.c"]  # not in src_to_obj

        (tmp_path / "output").mkdir()
        (tmp_path / "output" / "libegui.a").write_bytes(b"lib")

        mock_run.return_value = MagicMock(
            returncode=0, stdout="ok\n", stderr=""
        )

        success, _ = engine.compile()
        assert success
        call_args = mock_run.call_args_list[0]
        assert call_args[0][0][0] == "make"

    @patch("subprocess.run")
    @patch("subprocess.Popen")
    def test_fast_compile_failure_falls_back_to_make(self, mock_popen, mock_run, tmp_path):
        engine = self._make_engine(tmp_path)
        engine._build_config = self._make_config(tmp_path)
        engine._last_changed_files = ["uicode.c"]

        (tmp_path / "output").mkdir()
        (tmp_path / "output" / "libegui.a").write_bytes(b"lib")

        # Popen (fast compile) fails
        fake_proc = MagicMock()
        fake_proc.communicate.return_value = (b"error\n", b"")
        fake_proc.returncode = 1
        fake_proc.kill = MagicMock()
        mock_popen.return_value = fake_proc

        # run (make fallback) succeeds, then dry-run for config
        mock_run.side_effect = [
            MagicMock(returncode=0, stdout="ok\n", stderr=""),  # make
            MagicMock(returncode=0, stdout=MAKE_DRYRUN_OUTPUT, stderr=""),  # dry-run
        ]

        with patch("os.path.getmtime", return_value=1000.0):
            success, _ = engine.compile()
        assert success

    @patch("subprocess.run")
    def test_make_compile_extracts_config(self, mock_run, tmp_path):
        engine = self._make_engine(tmp_path)
        engine._build_config = None
        engine._last_changed_files = []

        make_result = MagicMock(returncode=0, stdout="ok\n", stderr="")
        dryrun_result = MagicMock(
            returncode=0, stdout=MAKE_DRYRUN_OUTPUT, stderr=""
        )
        mock_run.side_effect = [make_result, dryrun_result]

        with patch("os.path.getmtime", return_value=1000.0):
            success, _ = engine._make_compile()

        assert success
        assert engine._build_config is not None
        assert "uicode.c" in engine._build_config.src_to_obj


# ---------------------------------------------------------------------------
# write_project_files sets _last_changed_files
# ---------------------------------------------------------------------------

class TestWriteTracksChangedFiles:
    def test_write_uicode_sets_changed(self, tmp_path):
        engine = CompilerEngine.__new__(CompilerEngine)
        engine.project_root = str(tmp_path)
        engine.app_name = "TestApp"
        engine._last_changed_files = []

        uicode = tmp_path / "example" / "TestApp" / "uicode.c"
        uicode.parent.mkdir(parents=True)

        engine.write_uicode("int main() {}")
        assert engine._last_changed_files == ["uicode.c"]
