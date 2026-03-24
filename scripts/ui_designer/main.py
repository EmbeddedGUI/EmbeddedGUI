"""EmbeddedGUI Visual UI Designer entry point."""

from __future__ import annotations

import argparse
import contextlib
import io
import os
import sys


def _suppress_qfluentwidgets_tip():
    """Import qfluentwidgets while suppressing the promotion tip."""
    with contextlib.redirect_stdout(io.StringIO()):
        import qfluentwidgets
    return qfluentwidgets


_suppress_qfluentwidgets_tip()

# Ensure scripts/ is in path so package imports work.
_script_dir = os.path.dirname(os.path.abspath(__file__))
_scripts_dir = os.path.normpath(os.path.join(_script_dir, ".."))
if _scripts_dir not in sys.path:
    sys.path.insert(0, _scripts_dir)


def _parse_args():
    parser = argparse.ArgumentParser(description="EmbeddedGUI Visual UI Designer")
    parser.add_argument(
        "--project",
        "-p",
        help="Open an existing project (.egui file or directory containing one)",
        default=None,
    )
    parser.add_argument(
        "--app",
        help="Target APP name (default: from config or HelloDesigner)",
        default=None,
    )
    parser.add_argument(
        "--sdk-root",
        "--root",
        dest="sdk_root",
        help="EmbeddedGUI SDK root directory",
        default=None,
    )
    return parser.parse_args()


def main():
    args = _parse_args()

    try:
        from PyQt5.QtWidgets import QApplication
        from PyQt5.QtCore import QTimer
    except ImportError:
        print("Error: PyQt5 is required. Install it with:")
        print("  pip install PyQt5")
        sys.exit(1)

    from ui_designer.model.config import get_config
    from ui_designer.model.widget_registry import WidgetRegistry
    from ui_designer.model.workspace import find_sdk_root, normalize_path
    from ui_designer.ui.main_window import MainWindow
    from ui_designer.ui.theme import apply_theme

    config = get_config()
    cli_project = normalize_path(args.project)
    sdk_root = find_sdk_root(
        cli_sdk_root=args.sdk_root,
        configured_sdk_root=config.sdk_root or config.egui_root,
        project_path=cli_project,
    )
    app_name = args.app or config.last_app or "HelloDesigner"

    if sdk_root:
        config.sdk_root = sdk_root
        config.egui_root = sdk_root
        config.save()

    WidgetRegistry.instance()

    app = QApplication(sys.argv)
    app.setApplicationName("EmbeddedGUI Designer")
    apply_theme(app, config.theme)

    font_pt = getattr(config, "font_size_px", 0) or 9
    app.setStyleSheet(app.styleSheet() + f"\n* {{ font-size: {font_pt}pt; }}")

    window = MainWindow(sdk_root, app_name=app_name)

    project_to_open = ""
    preferred_sdk_root = sdk_root
    if cli_project:
        project_to_open = cli_project
    elif config.last_project_path and os.path.exists(config.last_project_path):
        project_to_open = normalize_path(config.last_project_path)
        preferred_sdk_root = preferred_sdk_root or config.sdk_root or config.egui_root

    if project_to_open:
        try:
            window._open_project_path(project_to_open, preferred_sdk_root=preferred_sdk_root, silent=not bool(cli_project))
        except Exception as exc:
            print(f"Warning: Failed to load project: {exc}")

    window.show()
    if not project_to_open and not sdk_root:
        QTimer.singleShot(0, window.maybe_prompt_initial_sdk_setup)
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
