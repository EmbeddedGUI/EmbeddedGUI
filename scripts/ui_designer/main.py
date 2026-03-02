"""EmbeddedGUI Visual UI Designer - Entry Point.

Usage:
    python -m ui_designer.main [--project FILE] [--app NAME] [--root DIR]

    Or from project root:
    python scripts/ui_designer/main.py [--project FILE] [--app NAME] [--root DIR]
"""

import sys
import os
import argparse

# Suppress QFluentWidgets Pro promotion tip during import
import io
import contextlib

def _suppress_qfluentwidgets_tip():
    """Import qfluentwidgets while suppressing the promotion tip."""
    with contextlib.redirect_stdout(io.StringIO()):
        import qfluentwidgets
    return qfluentwidgets

_suppress_qfluentwidgets_tip()

# Ensure scripts/ is in path so package imports work
_script_dir = os.path.dirname(os.path.abspath(__file__))
_scripts_dir = os.path.normpath(os.path.join(_script_dir, ".."))
if _scripts_dir not in sys.path:
    sys.path.insert(0, _scripts_dir)


def main():
    parser = argparse.ArgumentParser(description="EmbeddedGUI Visual UI Designer")
    parser.add_argument(
        "--project", "-p",
        help="Open an existing project (.egui file or directory containing one)",
        default=None,
    )
    parser.add_argument(
        "--app",
        help="Target APP name (default: from config or HelloDesigner)",
        default=None,
    )
    parser.add_argument(
        "--root",
        help="EmbeddedGUI project root directory",
        default=None,
    )
    args = parser.parse_args()

    # Import PyQt5
    try:
        from PyQt5.QtWidgets import QApplication
    except ImportError:
        print("Error: PyQt5 is required. Install it with:")
        print("  pip install PyQt5")
        sys.exit(1)

    from ui_designer.ui.main_window import MainWindow
    from ui_designer.model.project import Project
    from ui_designer.model.config import get_config
    from ui_designer.ui.theme import apply_theme
    from ui_designer.model.widget_registry import WidgetRegistry

    # Load config
    config = get_config()

    # Determine project root (priority: args > config > auto-detect)
    if args.root:
        project_root = os.path.abspath(args.root)
    elif config.egui_root and os.path.isdir(config.egui_root):
        project_root = config.egui_root
    else:
        # Auto-detect: go up from scripts/ui_designer/ to project root
        project_root = os.path.normpath(os.path.join(_script_dir, "..", ".."))

    if not os.path.exists(os.path.join(project_root, "Makefile")):
        print(f"Error: Cannot find Makefile in {project_root}")
        print("Please run from the EmbeddedGUI project root or use --root option.")
        sys.exit(1)

    # WidgetRegistry loads custom_widgets/ plugins automatically on first access.
    # Ensure the registry is initialized before the UI starts.
    WidgetRegistry.instance()

    # Determine app name (priority: args > config > default)
    if args.app:
        app_name = args.app
    elif config.last_app:
        app_name = config.last_app
    else:
        app_name = "HelloDesigner"

    # Update config with current paths
    config.egui_root = project_root
    config.last_app = app_name
    config.save()

    app = QApplication(sys.argv)
    app.setApplicationName("EmbeddedGUI Designer")

    # Apply theme from config
    apply_theme(app, config.theme)

    # Apply font size (saved or default 9pt)
    font_pt = getattr(config, 'font_size_px', 0) or 9
    app.setStyleSheet(app.styleSheet() + f"\n* {{ font-size: {font_pt}pt; }}")

    window = MainWindow(project_root, app_name=app_name)

    # Open project if specified on command line
    if args.project:
        project_path = os.path.abspath(args.project)
        try:
            project = Project.load(project_path)
            project.egui_root = project_root
            window.project = project
            window._project_dir = os.path.dirname(project_path)
            window._show_editor()
            window._apply_project()
            window._update_window_title()
            # Switch to startup page
            startup = project.get_startup_page()
            if startup:
                window._switch_page(startup.name)
            elif project.pages:
                window._switch_page(project.pages[0].name)
        except Exception as e:
            print(f"Warning: Failed to load project: {e}")
            # Show welcome page on error
    else:
        # Try to auto-load last project (supports paths outside example/)
        project_file = None
        project_dir = None

        # Priority 1: last_project_path from config (any location)
        if config.last_project_path and os.path.isfile(config.last_project_path):
            project_file = config.last_project_path
            project_dir = os.path.dirname(project_file)
        else:
            # Priority 2: fall back to example/{app_name}
            app_dir = os.path.join(project_root, "example", app_name)
            if os.path.isdir(app_dir):
                found_file, found = Project._find_project_file(app_dir)
                if found:
                    project_file = found_file
                    project_dir = app_dir

        if project_file:
            try:
                project = Project.load(project_file)
                project.egui_root = project_root
                window.project = project
                window._project_dir = project_dir
                window._show_editor()
                window._apply_project()
                window._update_window_title()
                # Switch to startup page
                startup = project.get_startup_page()
                if startup:
                    window._switch_page(startup.name)
                elif project.pages:
                    window._switch_page(project.pages[0].name)
            except Exception as e:
                print(f"Warning: Failed to load last project: {e}")
                # Keep welcome page visible on error
        # If no project file exists, welcome page stays visible (default)

    window.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
