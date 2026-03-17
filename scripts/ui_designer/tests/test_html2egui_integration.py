"""Integration tests for html2egui_helper.py subcommands.

These tests call the actual script via subprocess to verify end-to-end behavior.
"""

import json
import os
import subprocess
import sys
import pytest

# Path to the helper script
SCRIPT_PATH = os.path.normpath(
    os.path.join(os.path.dirname(__file__), "..", "..", "html2egui_helper.py")
)
# EmbeddedGUI project root
EGUI_ROOT = os.path.normpath(
    os.path.join(os.path.dirname(__file__), "..", "..", "..")
)


def _run_helper(*cmd_args, check=True):
    """Run html2egui_helper.py with given arguments."""
    result = subprocess.run(
        [sys.executable, SCRIPT_PATH] + list(cmd_args),
        # Avoid inheriting a potentially invalid stdin handle on Windows CI,
        # which can raise WinError 6 during subprocess setup.
        stdin=subprocess.DEVNULL,
        capture_output=True,
        text=True,
        cwd=EGUI_ROOT,
        timeout=30,
    )
    if check and result.returncode != 0:
        raise RuntimeError(
            f"Command failed (rc={result.returncode}):\n"
            f"stdout: {result.stdout}\nstderr: {result.stderr}"
        )
    return result


@pytest.mark.integration
class TestScaffold:
    """Test scaffold subcommand."""

    def test_scaffold_creates_project_structure(self, tmp_path):
        app_name = "TestScaffoldApp"
        app_dir = tmp_path / "example" / app_name

        # Scaffold uses _find_egui_root() which needs Makefile + src/
        # We run from the real EGUI_ROOT but use --force to create in example/
        # Instead, test by calling scaffold on the real project root
        result = _run_helper(
            "scaffold", "--app", app_name,
            "--width", "320", "--height", "240", "--force",
        )

        real_app_dir = os.path.join(EGUI_ROOT, "example", app_name)
        try:
            assert os.path.isdir(real_app_dir)
            assert os.path.isfile(os.path.join(real_app_dir, f"{app_name}.egui"))
            assert os.path.isfile(os.path.join(real_app_dir, "app_egui_config.h"))
            assert os.path.isfile(os.path.join(real_app_dir, "build.mk"))
            assert os.path.isdir(os.path.join(real_app_dir, ".eguiproject", "layout"))
            assert os.path.isfile(
                os.path.join(real_app_dir, ".eguiproject", "layout", "main_page.xml")
            )
        finally:
            # Cleanup
            import shutil
            if os.path.isdir(real_app_dir):
                shutil.rmtree(real_app_dir, ignore_errors=True)

    def test_scaffold_multi_page(self):
        app_name = "TestMultiPageApp"
        result = _run_helper(
            "scaffold", "--app", app_name,
            "--width", "320", "--height", "240",
            "--pages", "home,settings,detail", "--force",
        )

        real_app_dir = os.path.join(EGUI_ROOT, "example", app_name)
        try:
            layout_dir = os.path.join(real_app_dir, ".eguiproject", "layout")
            assert os.path.isfile(os.path.join(layout_dir, "home.xml"))
            assert os.path.isfile(os.path.join(layout_dir, "settings.xml"))
            assert os.path.isfile(os.path.join(layout_dir, "detail.xml"))
        finally:
            import shutil
            if os.path.isdir(real_app_dir):
                shutil.rmtree(real_app_dir, ignore_errors=True)


@pytest.mark.integration
class TestGenerateCode:
    """Test generate-code subcommand."""

    def test_generate_code_from_scaffold(self):
        app_name = "TestGenCodeApp"

        # Step 1: Scaffold
        _run_helper(
            "scaffold", "--app", app_name,
            "--width", "240", "--height", "320", "--force",
        )

        real_app_dir = os.path.join(EGUI_ROOT, "example", app_name)
        try:
            # Step 2: Generate code
            result = _run_helper("generate-code", "--app", app_name)

            # Verify generated files
            assert os.path.isfile(os.path.join(real_app_dir, "main_page.h"))
            assert os.path.isfile(os.path.join(real_app_dir, "main_page_layout.c"))
            assert os.path.isfile(os.path.join(real_app_dir, "main_page.c"))
            assert os.path.isfile(os.path.join(real_app_dir, "uicode.h"))
            assert os.path.isfile(os.path.join(real_app_dir, "uicode.c"))

            # Verify resource config was generated
            rc_path = os.path.join(real_app_dir, "resource", "src", "app_resource_config.json")
            assert os.path.isfile(rc_path)
            with open(rc_path, "r") as f:
                data = json.load(f)
            assert "img" in data
            assert "font" in data
        finally:
            import shutil
            if os.path.isdir(real_app_dir):
                shutil.rmtree(real_app_dir, ignore_errors=True)


@pytest.mark.integration
class TestExtractText:
    """Test extract-text subcommand."""

    def test_extract_text_from_html(self, tmp_path):
        html_file = tmp_path / "test.html"
        html_file.write_text(
            '<div class="w-[320px] h-[240px]">'
            '<span>Hello World</span>'
            '<span>Test 123</span>'
            '</div>',
            encoding="utf-8",
        )

        result = _run_helper("extract-text", "--input", str(html_file))
        # Output should contain the extracted characters
        assert "H" in result.stdout
        assert "W" in result.stdout


@pytest.mark.integration
class TestExtractLayout:
    """Test extract-layout subcommand."""

    def test_extract_layout_from_html(self, tmp_path):
        html_file = tmp_path / "test.html"
        html_file.write_text(
            '<div class="w-[320px] h-[240px] bg-slate-900">'
            '  <div class="flex flex-col p-4">'
            '    <span class="text-white text-lg">Title</span>'
            '    <span class="text-slate-400 text-sm">Subtitle</span>'
            '  </div>'
            '</div>',
            encoding="utf-8",
        )

        result = _run_helper("extract-layout", "--input", str(html_file))
        # Output should be valid JSON
        data = json.loads(result.stdout)
        assert "screen" in data
        assert data["screen"]["width"] == 320
        assert data["screen"]["height"] == 240
