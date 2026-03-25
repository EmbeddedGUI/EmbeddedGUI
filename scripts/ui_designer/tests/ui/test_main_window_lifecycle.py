"""Lifecycle smoke tests for MainWindow shutdown behavior."""

from __future__ import annotations

import os
import subprocess
import sys
import textwrap
from pathlib import Path


def test_main_window_close_smoke_with_active_timers():
    repo_root = Path(__file__).resolve().parents[4]
    script = textwrap.dedent(
        f"""
        import os
        import shutil
        import sys
        import tempfile
        import time
        from pathlib import Path

        os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

        repo_root = Path({repr(str(repo_root))})
        sys.path.insert(0, str(repo_root / "scripts"))

        from PyQt5.QtWidgets import QApplication

        from ui_designer.model.project import Project
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.ui.main_window import MainWindow


        def create_sdk_root(root: Path):
            (root / "src").mkdir(parents=True)
            (root / "porting" / "designer").mkdir(parents=True)
            (root / "Makefile").write_text("all:\\n", encoding="utf-8")


        class DisabledCompiler:
            def can_build(self):
                return False

            def is_preview_running(self):
                return False

            def stop_exe(self):
                return None

            def cleanup(self):
                return None

            def get_build_error(self):
                return "preview disabled for smoke"

            def set_screen_size(self, width, height):
                return None

            def is_exe_ready(self):
                return False


        app = QApplication.instance() or QApplication([])
        temp_root = Path(tempfile.mkdtemp(prefix="ui_designer_close_smoke_", dir=str(repo_root.parent)))
        try:
            for index in range(12):
                sdk_root = temp_root / f"sdk_{{index}}"
                project_dir = temp_root / f"project_{{index}}"
                create_sdk_root(sdk_root)

                project = Project(screen_width=240, screen_height=320, app_name=f"SmokeDemo{{index}}")
                project.sdk_root = str(sdk_root)
                project.project_dir = str(project_dir)
                page = project.create_new_page("main_page")
                page.root_widget.add_child(WidgetModel("label", name="field_label"))
                page.root_widget.add_child(WidgetModel("button", name="field_button"))
                project.save(str(project_dir))

                window = MainWindow(str(sdk_root))
                window._recreate_compiler = lambda _window=window: setattr(_window, "compiler", DisabledCompiler())
                window._trigger_compile = lambda: None
                window._open_loaded_project(project, str(project_dir), preferred_sdk_root=str(sdk_root), silent=True)
                window.widget_tree.filter_edit.setText("field")
                window._compile_timer.start(10)
                window._regen_timer.start(10)
                window._project_watch_timer.start(10)
                window.close()
                window.deleteLater()
                app.sendPostedEvents()
                app.processEvents()
                time.sleep(0.03)
                app.sendPostedEvents()
                app.processEvents()
        finally:
            shutil.rmtree(temp_root, ignore_errors=True)
        """
    )

    env = os.environ.copy()
    env.setdefault("QT_QPA_PLATFORM", "offscreen")
    result = subprocess.run(
        [sys.executable, "-c", script],
        stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        env=env,
        timeout=60,
    )

    assert result.returncode == 0, f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}"
