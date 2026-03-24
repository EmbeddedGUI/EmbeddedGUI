"""Main window for EmbeddedGUI Designer — Android Studio-like IDE layout.

All panels (Project Explorer, Widget Tree, Properties) are QDockWidgets
that can be freely dragged, docked, and rearranged.  Page switching uses
a qfluentwidgets.TabBar strip above the central editor area.

Layout (default):
    ┌──────────┬────────────────────────┬───────────────┐
    │ Project  │  [page tabs bar]       │  Widget Tree  │
    │ Explorer │  Design/Split/Code     │───────────────│
    │          │                        │  Properties   │
    └──────────┴────────────────────────┴───────────────┘
"""

import copy
import os
import shutil

from PyQt5.QtWidgets import (
    QMainWindow, QWidget, QVBoxLayout,
    QAction, QActionGroup, QFileDialog, QStatusBar,
    QMessageBox, QScrollArea, QDockWidget, QMenu,
    QApplication, QDialog, QStackedWidget, QToolBar, QInputDialog, QProgressDialog,
)
from PyQt5.QtCore import Qt, QTimer, QSize, QByteArray
from PyQt5.QtGui import QIcon

from qfluentwidgets import TabBar, TabCloseButtonDisplayMode

from .widget_tree import WidgetTreePanel
from .property_panel import PropertyPanel
from .preview_panel import PreviewPanel, MODE_VERTICAL, MODE_HORIZONTAL, MODE_HIDDEN
from .editor_tabs import EditorTabs, MODE_DESIGN, MODE_SPLIT, MODE_CODE
from .project_dock import ProjectExplorerDock
from .resource_panel import ResourcePanel
from .app_selector import AppSelectorDialog
from .new_project_dialog import NewProjectDialog
from .welcome_page import WelcomePage
from .debug_panel import DebugPanel
from ..model.widget_model import WidgetModel
from ..model.project import Project
from ..model.page import Page
from ..model.config import get_config
from ..model.sdk_bootstrap import default_sdk_install_dir, ensure_sdk_downloaded
from ..model.workspace import find_sdk_root, infer_sdk_root_from_project_dir, is_valid_sdk_root, normalize_path, resolve_sdk_root_candidate
from ..model.undo_manager import UndoManager
from ..generator.code_generator import generate_all_files, generate_all_files_preserved, generate_uicode
from ..generator.resource_config_generator import ResourceConfigGenerator
from ..engine.compiler import CompilerEngine
from ..engine.layout_engine import compute_layout, compute_page_layout
from ..utils.scaffold import make_app_build_mk_content, make_app_config_h_content, make_empty_resource_config_content
from .theme import apply_theme


def delete_page_generated_files(project_dir, page_name):
    """Delete the three generated C files for a removed page.

    Removes {page_name}.h, {page_name}_layout.c, {page_name}.c from
    project_dir so they are no longer picked up by EGUI_CODE_SRC.
    Silently ignores missing files and permission errors.

    Only deletes files that resolve to paths strictly inside project_dir
    (path traversal via page_name like '../other_project/file' is blocked).
    """
    if not page_name or not project_dir:
        return
    project_real = os.path.realpath(project_dir)
    for suffix in (f"{page_name}.h", f"{page_name}_layout.c", f"{page_name}.c"):
        fpath = os.path.realpath(os.path.join(project_dir, suffix))
        # Safety check: only files directly inside project_real
        if not fpath.startswith(project_real + os.sep):
            continue
        try:
            if os.path.isfile(fpath):
                os.remove(fpath)
        except OSError:
            pass


class MainWindow(QMainWindow):
    """Main designer window with project explorer, editor, tree, and properties."""

    def __init__(self, project_root, app_name="HelloDesigner"):
        super().__init__()
        self._config = get_config()
        self.project_root = normalize_path(project_root)
        self.app_name = app_name
        self.project = None
        self.compiler = None
        self.auto_compile = self._config.auto_compile
        self._project_dir = None      # directory containing .egui project file
        self._selected_widget = None
        self._current_page = None      # currently-displayed Page object

        # Debounce timer for compile
        self._compile_timer = QTimer()
        self._compile_timer.setSingleShot(True)
        self._compile_timer.setInterval(500)
        self._compile_timer.timeout.connect(self._do_compile_and_run)

        # Timer to find and embed exe window after compile (legacy, kept for compat)
        self._embed_timer = QTimer()
        self._embed_timer.setSingleShot(True)
        self._embed_timer.setInterval(0)  # Immediate - no delay
        self._embed_timer.timeout.connect(self._try_embed_exe)

        # Debounce timer for resource generation
        self._regen_timer = QTimer()
        self._regen_timer.setSingleShot(True)
        self._regen_timer.setInterval(800)  # Wait 800ms after last change
        self._regen_timer.timeout.connect(lambda: self._generate_resources(silent=True))

        self._compile_worker = None
        self._precompile_worker = None
        self._syncing_tabs = False
        self._resources_need_regen = False
        self._pending_compile = False  # Track if compile needed after current one
        self._undo_manager = UndoManager()
        self._undoing = False  # True during undo/redo to suppress snapshot recording
        self._project_watch_snapshot = {}
        self._external_reload_pending = False

        self._project_watch_timer = QTimer()
        self._project_watch_timer.setInterval(1000)
        self._project_watch_timer.timeout.connect(self._poll_project_files)

        self._init_ui()
        self._init_menus()
        self._init_toolbar()
        self._apply_saved_window_state()
        # Start with welcome page (don't auto-create project)
        self._show_welcome_page()

    # ── UI Construction ────────────────────────────────────────────

    def _init_ui(self):
        self.setWindowTitle("EmbeddedGUI Designer")
        self.setMinimumSize(1100, 700)
        self.resize(1400, 800)

        # Allow docks to be nested and overlapped
        self.setDockNestingEnabled(True)

        # ── Left dock: Project Explorer ──
        self.project_dock = ProjectExplorerDock(self)
        self.project_dock.setObjectName("project_explorer_dock")
        self.addDockWidget(Qt.LeftDockWidgetArea, self.project_dock)
        self.project_dock.setMinimumWidth(180)
        # self.project_dock.setMaximumWidth(320)

        # ── Central area: Stacked Widget (Welcome / Editor) ──
        self._central_stack = QStackedWidget()

        # Welcome page (index 0)
        self._welcome_page = WelcomePage()
        self._welcome_page.open_recent.connect(self._open_recent_project)
        self._welcome_page.new_project.connect(self._new_project)
        self._welcome_page.open_project.connect(self._open_project)
        self._welcome_page.open_app.connect(self._open_app_dialog)
        self._welcome_page.set_sdk_root.connect(self._set_sdk_root)
        self._welcome_page.download_sdk.connect(self._download_sdk)
        self._central_stack.addWidget(self._welcome_page)

        # Editor container (index 1)
        editor_container = QWidget()
        editor_layout = QVBoxLayout(editor_container)
        editor_layout.setContentsMargins(0, 0, 0, 0)
        editor_layout.setSpacing(0)

        # Page tab bar (qfluentwidgets — movable, closable, scrollable)
        self.page_tab_bar = self._create_page_tab_bar()

        editor_layout.addWidget(self.page_tab_bar)

        self.preview_panel = PreviewPanel(screen_width=240, screen_height=320)
        self.editor_tabs = EditorTabs(self.preview_panel)
        editor_layout.addWidget(self.editor_tabs, 1)

        self._central_stack.addWidget(editor_container)

        self.setCentralWidget(self._central_stack)

        # ── Right dock: Widget Tree ──
        self.tree_dock = QDockWidget("Widget Tree", self)
        self.tree_dock.setObjectName("widget_tree_dock")
        self.tree_dock.setAllowedAreas(Qt.AllDockWidgetAreas)
        self.widget_tree = WidgetTreePanel()
        self.tree_dock.setWidget(self.widget_tree)
        self.tree_dock.setMinimumWidth(180)
        self.addDockWidget(Qt.RightDockWidgetArea, self.tree_dock)

        # ── Right dock: Properties ──
        self.props_dock = QDockWidget("Properties", self)
        self.props_dock.setObjectName("properties_dock")
        self.props_dock.setAllowedAreas(Qt.AllDockWidgetAreas)
        right_scroll = QScrollArea()
        right_scroll.setWidgetResizable(True)
        right_scroll.setMinimumWidth(260)
        self.property_panel = PropertyPanel()
        right_scroll.setWidget(self.property_panel)
        self.props_dock.setWidget(right_scroll)
        self.addDockWidget(Qt.RightDockWidgetArea, self.props_dock)

        # Stack the two right docks vertically by default
        self.splitDockWidget(self.tree_dock, self.props_dock, Qt.Vertical)

        # ── Left dock: Resources (independent panel) ──
        self.res_dock = QDockWidget("Resources", self)
        self.res_dock.setObjectName("resources_dock")
        self.res_dock.setAllowedAreas(Qt.AllDockWidgetAreas)
        self.res_panel = ResourcePanel()
        self.res_dock.setWidget(self.res_panel)
        self.res_dock.setMinimumWidth(200)
        self.addDockWidget(Qt.LeftDockWidgetArea, self.res_dock)
        # Stack below project explorer
        self.splitDockWidget(self.project_dock, self.res_dock, Qt.Vertical)

        # ── Bottom dock: Debug Output ──
        self.debug_dock = QDockWidget("Debug Output", self)
        self.debug_dock.setObjectName("debug_output_dock")
        self.debug_dock.setAllowedAreas(Qt.AllDockWidgetAreas)
        self.debug_panel = DebugPanel()
        self.debug_dock.setWidget(self.debug_panel)
        self.addDockWidget(Qt.BottomDockWidgetArea, self.debug_dock)

        # Status bar
        self.statusBar().showMessage("Ready")

        # ── Connect signals ──

        # Widget tree
        self.widget_tree.widget_selected.connect(self._on_widget_selected)
        self.widget_tree.tree_changed.connect(self._on_tree_changed)

        # Property panel
        self.property_panel.property_changed.connect(self._on_property_changed)

        # Preview panel
        self.preview_panel.widget_selected.connect(self._on_preview_widget_selected)
        self.preview_panel.widget_moved.connect(self._on_widget_moved)
        self.preview_panel.widget_resized.connect(self._on_widget_resized)
        self.preview_panel.widget_reordered.connect(self._on_widget_reordered)
        self.preview_panel.resource_dropped.connect(self._on_resource_dropped)
        self.preview_panel.drag_started.connect(self._on_drag_started)
        self.preview_panel.drag_finished.connect(self._on_drag_finished)
        self.preview_panel.runtime_failed.connect(self._on_preview_runtime_failed)

        # Editor tabs (Code → Design sync)
        self.editor_tabs.xml_changed.connect(self._on_xml_changed)
        self.editor_tabs.save_requested.connect(self._save_project)

        # Project explorer
        self.project_dock.page_selected.connect(self._on_page_selected)
        self.project_dock.page_added.connect(self._on_page_added)
        self.project_dock.page_removed.connect(self._on_page_removed)
        self.project_dock.page_renamed.connect(self._on_page_renamed)
        self.project_dock.startup_changed.connect(self._on_startup_changed)
        self.project_dock.page_mode_changed.connect(self._on_page_mode_changed)

        # Resource panel
        self.res_panel.resource_selected.connect(self._on_resource_selected)
        self.res_panel.resource_imported.connect(self._on_resource_imported)

    def _apply_stylesheet(self):
        pass  # Rely entirely on the global Fusion / Fluent theme

    def _apply_saved_window_state(self):
        geometry = (self._config.window_geometry or "").strip()
        if geometry:
            try:
                self.restoreGeometry(QByteArray.fromBase64(geometry.encode("ascii")))
            except Exception:
                pass

        state = (self._config.window_state or "").strip()
        if state:
            try:
                self.restoreState(QByteArray.fromBase64(state.encode("ascii")))
            except Exception:
                pass

    def _save_window_state_to_config(self):
        try:
            self._config.window_geometry = bytes(self.saveGeometry().toBase64()).decode("ascii")
            self._config.window_state = bytes(self.saveState().toBase64()).decode("ascii")
        except Exception:
            self._config.window_geometry = ""
            self._config.window_state = ""

    def _apply_sdk_root(self, path, status_message=""):
        path = normalize_path(path)
        if not path:
            return

        self.project_root = path
        self._config.sdk_root = path
        self._config.egui_root = path
        self._config.sdk_setup_prompted = True
        self._config.save()

        if self.project is not None:
            self.project.sdk_root = path
            self._recreate_compiler()
            self._update_compile_availability()
            if self.compiler is None or not self.compiler.can_build():
                reason = "SDK unavailable, compile preview disabled"
                if self.compiler is not None and self.compiler.get_build_error():
                    reason = self.compiler.get_build_error()
                self._switch_to_python_preview(reason)
            elif self.auto_compile:
                self._trigger_compile()

        self._welcome_page.refresh()
        if status_message:
            self.statusBar().showMessage(status_message)

    def _has_valid_sdk_root(self):
        return is_valid_sdk_root(self.project_root)

    def _recreate_compiler(self):
        if self.compiler is not None:
            self.compiler.cleanup()
            self.compiler = None

        if not self._has_valid_sdk_root() or not self._project_dir or not self.app_name:
            return

        self.compiler = CompilerEngine(self.project_root, self._project_dir, self.app_name)
        if self.project is not None:
            self.compiler.set_screen_size(self.project.screen_width, self.project.screen_height)

    def _update_compile_availability(self):
        can_compile = (
            self.project is not None
            and self.compiler is not None
            and self._has_valid_sdk_root()
            and self.compiler.can_build()
        )
        self._compile_action.setEnabled(can_compile)
        self.auto_compile_action.setEnabled(can_compile)
        self._stop_action.setEnabled(self.compiler is not None and self.compiler.is_preview_running())
        self._reload_project_action.setEnabled(self.project is not None and bool(self._project_dir))

    def _switch_to_python_preview(self, reason=""):
        if self._current_page is None:
            self.preview_panel.show_python_preview(None, reason)
            return
        self.preview_panel.show_python_preview(self._current_page, reason)

    def _refresh_python_preview(self, reason=""):
        if self.project is None or self._current_page is None:
            return
        if reason or self.preview_panel.is_python_preview_active() or self.compiler is None:
            self._switch_to_python_preview(reason)

    def _persist_current_project_to_config(self):
        self._config.last_app = self.app_name or self._config.last_app
        self._config.last_project_path = normalize_path(os.path.join(self._project_dir, f"{self.app_name}.egui")) if self._project_dir else ""
        if self._has_valid_sdk_root():
            self._config.sdk_root = self.project_root
            self._config.egui_root = self.project_root
        if self._config.last_project_path:
            self._config.add_recent_project(self._config.last_project_path, self.project_root, self.app_name)
        else:
            self._config.save()
        self._update_recent_menu()

    def _read_app_dimensions(self, app_dir):
        screen_w, screen_h = 240, 320
        config_h = os.path.join(app_dir, "app_egui_config.h")
        if not os.path.isfile(config_h):
            return screen_w, screen_h

        try:
            with open(config_h, "r", encoding="utf-8") as f:
                content = f.read()
            import re
            match = re.search(r"EGUI_CONFIG_SCEEN_WIDTH\s+(\d+)", content)
            if match:
                screen_w = int(match.group(1))
            match = re.search(r"EGUI_CONFIG_SCEEN_HEIGHT\s+(\d+)", content)
            if match:
                screen_h = int(match.group(1))
        except Exception:
            pass
        return screen_w, screen_h

    def _create_standard_project_model(self, app_name, sdk_root, project_dir):
        WidgetModel.reset_counter()
        screen_w, screen_h = self._read_app_dimensions(project_dir)
        project = Project(screen_width=screen_w, screen_height=screen_h, app_name=app_name)
        project.sdk_root = sdk_root
        project.project_dir = project_dir
        project.create_new_page("main_page")
        return project

    def _scaffold_project_directory(self, project_dir, app_name, screen_width, screen_height):
        os.makedirs(project_dir, exist_ok=True)
        resource_src_dir = os.path.join(project_dir, "resource", "src")
        os.makedirs(resource_src_dir, exist_ok=True)

        build_mk = os.path.join(project_dir, "build.mk")
        if not os.path.exists(build_mk):
            with open(build_mk, "w", encoding="utf-8") as f:
                f.write(make_app_build_mk_content(app_name))

        config_h = os.path.join(project_dir, "app_egui_config.h")
        if not os.path.exists(config_h):
            with open(config_h, "w", encoding="utf-8") as f:
                f.write(make_app_config_h_content(app_name, screen_width, screen_height))

        resource_cfg = os.path.join(resource_src_dir, "app_resource_config.json")
        if not os.path.exists(resource_cfg):
            with open(resource_cfg, "w", encoding="utf-8") as f:
                f.write(make_empty_resource_config_content())

    def _copy_project_sidecar_files(self, src_dir, dst_dir):
        if not src_dir or not os.path.isdir(src_dir) or normalize_path(src_dir) == normalize_path(dst_dir):
            return

        for rel_path in ("build.mk", "app_egui_config.h"):
            src_path = os.path.join(src_dir, rel_path)
            dst_path = os.path.join(dst_dir, rel_path)
            if os.path.isfile(src_path) and not os.path.exists(dst_path):
                shutil.copy2(src_path, dst_path)

        for rel_dir in (
            os.path.join(".eguiproject", "resources"),
            os.path.join(".eguiproject", "mockup"),
        ):
            src_path = os.path.join(src_dir, rel_dir)
            dst_path = os.path.join(dst_dir, rel_dir)
            if os.path.isdir(src_path):
                shutil.copytree(src_path, dst_path, dirs_exist_ok=True)

    def _build_project_watch_snapshot(self):
        snapshot = {}
        if not self._project_dir or not self.app_name:
            return snapshot

        def _add_path(path):
            path = normalize_path(path)
            if not path or not os.path.exists(path):
                return

            try:
                stat = os.stat(path)
            except OSError:
                return

            snapshot[path] = (
                1 if os.path.isdir(path) else 0,
                stat.st_mtime_ns,
                stat.st_size,
            )

            if not os.path.isdir(path):
                return

            try:
                entries = sorted(os.listdir(path))
            except OSError:
                return

            for name in entries:
                _add_path(os.path.join(path, name))

        project_file = os.path.join(self._project_dir, f"{self.app_name}.egui")
        eguiproject_dir = os.path.join(self._project_dir, ".eguiproject")
        watch_roots = [
            project_file,
            os.path.join(eguiproject_dir, "layout"),
            os.path.join(eguiproject_dir, "resources"),
            os.path.join(eguiproject_dir, "mockup"),
            os.path.join(eguiproject_dir, "custom_widgets"),
        ]
        for root in watch_roots:
            _add_path(root)
        return snapshot

    @staticmethod
    def _diff_project_watch_snapshot(old_snapshot, new_snapshot):
        changed = []
        all_paths = set(old_snapshot.keys()) | set(new_snapshot.keys())
        for path in sorted(all_paths):
            if old_snapshot.get(path) != new_snapshot.get(path):
                changed.append(path)
        return changed

    @staticmethod
    def _summarize_changed_paths(paths):
        if not paths:
            return ""

        labels = [os.path.basename(path) or path for path in paths[:3]]
        summary = ", ".join(labels)
        remaining = len(paths) - len(labels)
        if remaining > 0:
            summary += f" (+{remaining})"
        return summary

    def _refresh_project_watch_snapshot(self):
        if self.project is None or not self._project_dir:
            self._project_watch_timer.stop()
            self._project_watch_snapshot = {}
            self._external_reload_pending = False
            return

        self._project_watch_snapshot = self._build_project_watch_snapshot()
        self._external_reload_pending = False
        if not self._project_watch_timer.isActive():
            self._project_watch_timer.start()

    def _poll_project_files(self):
        if self.project is None or not self._project_dir:
            return

        if self._external_reload_pending:
            if self._undo_manager.is_any_dirty():
                return
            if self._compile_worker is not None and self._compile_worker.isRunning():
                return
            if self._precompile_worker is not None and self._precompile_worker.isRunning():
                return
            self._reload_project_from_disk(auto=True)
            return

        new_snapshot = self._build_project_watch_snapshot()
        if not self._project_watch_snapshot:
            self._project_watch_snapshot = new_snapshot
            return
        if new_snapshot == self._project_watch_snapshot:
            return

        changed_paths = self._diff_project_watch_snapshot(self._project_watch_snapshot, new_snapshot)
        self._project_watch_snapshot = new_snapshot
        summary = self._summarize_changed_paths(changed_paths)

        if self._undo_manager.is_any_dirty():
            self._external_reload_pending = True
            self.debug_panel.log_info(f"External project change detected while dirty: {summary or 'project files updated'}")
            self.statusBar().showMessage("External project changes detected. Save or reload from disk to sync.", 5000)
            return

        if self._compile_worker is not None and self._compile_worker.isRunning():
            self._external_reload_pending = True
            return

        if self._precompile_worker is not None and self._precompile_worker.isRunning():
            self._external_reload_pending = True
            self.statusBar().showMessage("External project changes detected. Reload will resume after background compile.", 4000)
            return

        self._reload_project_from_disk(auto=True, changed_paths=changed_paths)

    def _reload_project_from_disk(self, checked=False, auto=False, changed_paths=None):
        del checked
        if self.project is None or not self._project_dir:
            return False

        if self._undo_manager.is_any_dirty():
            reply = QMessageBox.question(
                self,
                "Reload Project",
                "Reload project files from disk and discard unsaved changes?",
                QMessageBox.Yes | QMessageBox.No,
                QMessageBox.No,
            )
            if reply != QMessageBox.Yes:
                return False

        current_page_name = self._current_page.name if self._current_page else ""

        try:
            project = Project.load(self._project_dir)
        except Exception as exc:
            self._external_reload_pending = True
            self.debug_panel.log_error(f"Project reload failed: {exc}")
            self.debug_dock.show()
            self.debug_dock.raise_()
            if auto:
                self.statusBar().showMessage(f"Project reload failed: {exc}", 6000)
            else:
                QMessageBox.critical(self, "Reload Project Failed", f"Failed to reload project:\n{exc}")
            return False

        self._open_loaded_project(project, self._project_dir, preferred_sdk_root=self.project_root, silent=True)
        if current_page_name and self.project and self.project.get_page_by_name(current_page_name):
            if self._current_page is None or self._current_page.name != current_page_name:
                self._switch_page(current_page_name)

        summary = self._summarize_changed_paths(changed_paths or [])
        if auto:
            self.debug_panel.log_info(f"Project reloaded from disk: {summary or 'external changes applied'}")
            self.statusBar().showMessage(f"Reloaded external changes: {summary or 'project updated'}", 5000)
        else:
            self.statusBar().showMessage("Project reloaded from disk", 4000)
        return True

    def _clear_editor_state(self):
        self.preview_panel.stop_rendering()
        self._project_watch_timer.stop()
        self._project_watch_snapshot = {}
        self._external_reload_pending = False
        self._selected_widget = None
        self._current_page = None
        self._clear_page_tabs()
        self.widget_tree.set_project(None)
        self.property_panel.set_widget(None)
        self.preview_panel.set_widgets([])
        self.preview_panel.clear_background_image()
        self.project_dock.set_project(None)

    def _open_loaded_project(self, project, project_dir, preferred_sdk_root="", silent=False):
        project_dir = normalize_path(project_dir)
        resolved_sdk_root = find_sdk_root(
            cli_sdk_root=preferred_sdk_root or project.sdk_root,
            configured_sdk_root=self._config.sdk_root or self.project_root,
            project_path=project_dir,
        )
        project.sdk_root = resolved_sdk_root or normalize_path(preferred_sdk_root or project.sdk_root)
        project.project_dir = project_dir

        self.project = project
        self._project_dir = project_dir
        self.project_root = project.sdk_root
        self.app_name = project.app_name
        self._undo_manager = UndoManager()
        self._recreate_compiler()
        self._show_editor()
        self._clear_editor_state()
        self._apply_project()
        self._update_window_title()
        self._persist_current_project_to_config()
        self._refresh_project_watch_snapshot()

        startup = project.get_startup_page()
        if startup:
            self._switch_page(startup.name)
        elif project.pages:
            self._switch_page(project.pages[0].name)

        if self.compiler is None or not self.compiler.can_build():
            reason = "SDK unavailable, compile preview disabled"
            if self.compiler is not None and self.compiler.get_build_error():
                reason = self.compiler.get_build_error()
            self._switch_to_python_preview(reason)
            self.statusBar().showMessage("Opened project in editing-only mode")
        else:
            self._trigger_compile()
            self.statusBar().showMessage(f"Opened: {project_dir}")

        if not project.sdk_root and not silent:
            QMessageBox.information(
                self,
                "SDK Root Missing",
                "The project opened successfully, but no valid EmbeddedGUI SDK root was found. Preview will use Python fallback until you set the SDK root.",
            )

    # ── View switching ─────────────────────────────────────────────

    def _show_welcome_page(self):
        """Show the welcome page (hide editor)."""
        self._central_stack.setCurrentIndex(0)
        self._welcome_page.refresh()
        self.setWindowTitle("EmbeddedGUI Designer")

        # Hide dock widgets when on welcome page
        self.project_dock.hide()
        self.res_dock.hide()
        self.tree_dock.hide()
        self.props_dock.hide()

    def _show_editor(self):
        """Show the editor (hide welcome page)."""
        self._central_stack.setCurrentIndex(1)

        # Show dock widgets
        self.project_dock.show()
        self.res_dock.show()
        self.tree_dock.show()
        self.props_dock.show()

    def _create_page_tab_bar(self):
        page_tab_bar = TabBar()
        page_tab_bar.setMovable(True)
        page_tab_bar.setTabsClosable(True)
        page_tab_bar.setScrollable(True)
        page_tab_bar.setAddButtonVisible(False)
        page_tab_bar.setCloseButtonDisplayMode(TabCloseButtonDisplayMode.ON_HOVER)
        page_tab_bar.setTabMaximumWidth(180)
        page_tab_bar.setTabShadowEnabled(False)
        page_tab_bar.setFixedHeight(40)
        page_tab_bar.tabCloseRequested.connect(self._on_page_tab_closed)
        page_tab_bar.currentChanged.connect(self._on_page_tab_changed)
        page_tab_bar.setContextMenuPolicy(Qt.CustomContextMenu)
        page_tab_bar.customContextMenuRequested.connect(self._show_tab_context_menu)
        return page_tab_bar

    def _replace_page_tab_bar(self):
        old_tab_bar = self.page_tab_bar
        parent = old_tab_bar.parentWidget()
        layout = parent.layout() if parent is not None else None
        index = layout.indexOf(old_tab_bar) if layout is not None else -1

        self.page_tab_bar = self._create_page_tab_bar()
        if layout is not None:
            if index < 0:
                layout.addWidget(self.page_tab_bar)
            else:
                layout.insertWidget(index, self.page_tab_bar)

        old_tab_bar.setParent(None)
        old_tab_bar.deleteLater()

    def _clear_page_tabs(self):
        try:
            self.page_tab_bar.clear()
        except RuntimeError:
            self._replace_page_tab_bar()

    def _init_menus(self):
        menubar = self.menuBar()

        # ── File menu ──
        file_menu = menubar.addMenu("File")

        new_action = QAction("New Project", self)
        new_action.setShortcut("Ctrl+N")
        new_action.triggered.connect(self._new_project)
        file_menu.addAction(new_action)

        open_app_action = QAction("Open SDK Example...", self)
        open_app_action.setShortcut("Ctrl+Shift+O")
        open_app_action.triggered.connect(self._open_app_dialog)
        file_menu.addAction(open_app_action)

        open_action = QAction("Open Project File...", self)
        open_action.setShortcut("Ctrl+O")
        open_action.triggered.connect(self._open_project)
        file_menu.addAction(open_action)

        download_sdk_action = QAction("Download SDK Copy...", self)
        download_sdk_action.triggered.connect(self._download_sdk)
        file_menu.addAction(download_sdk_action)

        set_sdk_root_action = QAction("Set SDK Root...", self)
        set_sdk_root_action.triggered.connect(self._set_sdk_root)
        file_menu.addAction(set_sdk_root_action)

        # Recent Projects submenu
        self._recent_menu = file_menu.addMenu("Recent Projects")
        self._update_recent_menu()

        file_menu.addSeparator()

        self._save_action = QAction("Save Project", self)
        self._save_action.setShortcut("Ctrl+S")
        self._save_action.triggered.connect(self._save_project)
        file_menu.addAction(self._save_action)

        save_as_action = QAction("Save As...", self)
        save_as_action.setShortcut("Ctrl+Shift+S")
        save_as_action.triggered.connect(self._save_project_as)
        file_menu.addAction(save_as_action)

        self._reload_project_action = QAction("Reload Project From Disk", self)
        self._reload_project_action.setShortcut("Ctrl+Shift+R")
        self._reload_project_action.triggered.connect(self._reload_project_from_disk)
        self._reload_project_action.setEnabled(False)
        file_menu.addAction(self._reload_project_action)

        file_menu.addSeparator()

        close_project_action = QAction("Close Project", self)
        close_project_action.setShortcut("Ctrl+W")
        close_project_action.triggered.connect(self._close_project)
        file_menu.addAction(close_project_action)

        file_menu.addSeparator()

        export_action = QAction("Export C Code...", self)
        export_action.setShortcut("Ctrl+E")
        export_action.triggered.connect(self._export_code)
        file_menu.addAction(export_action)

        file_menu.addSeparator()

        quit_action = QAction("Quit", self)
        quit_action.setShortcut("Ctrl+Q")
        quit_action.triggered.connect(self.close)
        file_menu.addAction(quit_action)

        # ── Edit menu ──
        edit_menu = menubar.addMenu("Edit")

        self._undo_action = QAction("Undo", self)
        self._undo_action.setShortcut("Ctrl+Z")
        self._undo_action.setEnabled(False)
        self._undo_action.triggered.connect(self._undo)
        edit_menu.addAction(self._undo_action)

        self._redo_action = QAction("Redo", self)
        self._redo_action.setShortcut("Ctrl+Shift+Z")
        self._redo_action.setEnabled(False)
        self._redo_action.triggered.connect(self._redo)
        edit_menu.addAction(self._redo_action)

        # ── Build menu ──
        build_menu = menubar.addMenu("Build")

        self._compile_action = QAction("Compile && Run", self)
        self._compile_action.setShortcut("F5")
        self._compile_action.triggered.connect(self._do_compile_and_run)
        build_menu.addAction(self._compile_action)

        self.auto_compile_action = QAction("Auto Compile", self)
        self.auto_compile_action.setCheckable(True)
        self.auto_compile_action.setChecked(True)
        self.auto_compile_action.toggled.connect(self._toggle_auto_compile)
        build_menu.addAction(self.auto_compile_action)

        self._stop_action = QAction("Stop Exe", self)
        self._stop_action.triggered.connect(self._stop_exe)
        build_menu.addAction(self._stop_action)

        build_menu.addSeparator()

        gen_res_action = QAction("Generate Resources", self)
        gen_res_action.setToolTip(
            "Run resource generation (app_resource_generate.py) to produce\n"
            "C source files from resource/src/ assets and config."
        )
        gen_res_action.triggered.connect(self._generate_resources)
        build_menu.addAction(gen_res_action)

        # ── View menu ──
        view_menu = menubar.addMenu("View")

        # Theme Submenu
        theme_menu = view_menu.addMenu("Theme")
        theme_group = QActionGroup(self)
        theme_group.setExclusive(True)

        self.theme_dark_action = QAction("Dark", self)
        self.theme_dark_action.setCheckable(True)
        self.theme_dark_action.setChecked(self._config.theme == 'dark')
        self.theme_dark_action.triggered.connect(lambda: self._set_theme('dark'))
        theme_group.addAction(self.theme_dark_action)
        theme_menu.addAction(self.theme_dark_action)

        self.theme_light_action = QAction("Light", self)
        self.theme_light_action.setCheckable(True)
        self.theme_light_action.setChecked(self._config.theme == 'light')
        self.theme_light_action.triggered.connect(lambda: self._set_theme('light'))
        theme_group.addAction(self.theme_light_action)
        theme_menu.addAction(self.theme_light_action)
        
        view_menu.addSeparator()

        font_size_action = QAction("Font Size...", self)
        font_size_action.triggered.connect(self._set_font_sizes)
        view_menu.addAction(font_size_action)

        view_menu.addSeparator()

        # Dock panel toggles
        view_menu.addAction(self.project_dock.toggleViewAction())
        view_menu.addAction(self.res_dock.toggleViewAction())
        view_menu.addAction(self.tree_dock.toggleViewAction())
        view_menu.addAction(self.props_dock.toggleViewAction())
        view_menu.addAction(self.debug_dock.toggleViewAction())
        view_menu.addSeparator()

        self._overlay_group = QActionGroup(self)
        self._overlay_group.setExclusive(True)
        self._overlay_mode_actions = {}  # mode -> QAction

        # Restore saved layout mode from config
        saved_mode = self._config.overlay_mode
        saved_flipped = self._config.overlay_flipped
        self.preview_panel.set_overlay_mode(saved_mode)
        self.preview_panel._flipped = saved_flipped
        self.preview_panel._apply_mode()

        mode_items = [
            ("Vertical", MODE_VERTICAL, "Ctrl+1"),
            ("Horizontal", MODE_HORIZONTAL, "Ctrl+2"),
            ("Overlay Only", MODE_HIDDEN, "Ctrl+3"),
        ]
        for label, mode, shortcut in mode_items:
            action = QAction(label, self)
            action.setCheckable(True)
            action.setShortcut(shortcut)
            if mode == saved_mode:
                action.setChecked(True)
            action.triggered.connect(
                lambda checked, m=mode: self._set_overlay_mode(m)
            )
            self._overlay_group.addAction(action)
            self._overlay_mode_actions[mode] = action
            view_menu.addAction(action)

        swap_action = QAction("Swap Preview/Overlay", self)
        swap_action.setShortcut("Ctrl+4")
        swap_action.triggered.connect(self._flip_overlay_layout)
        view_menu.addAction(swap_action)

        view_menu.addSeparator()

        zoom_in_action = QAction("Zoom In", self)
        zoom_in_action.setShortcut("Ctrl+=")
        zoom_in_action.triggered.connect(lambda: self.preview_panel.overlay.zoom_in())
        view_menu.addAction(zoom_in_action)

        zoom_out_action = QAction("Zoom Out", self)
        zoom_out_action.setShortcut("Ctrl+-")
        zoom_out_action.triggered.connect(lambda: self.preview_panel.overlay.zoom_out())
        view_menu.addAction(zoom_out_action)

        zoom_reset_action = QAction("Zoom Reset (100%)", self)
        zoom_reset_action.setShortcut("Ctrl+0")
        zoom_reset_action.triggered.connect(lambda: self.preview_panel.overlay.zoom_reset())
        view_menu.addAction(zoom_reset_action)

        view_menu.addSeparator()

        # ── Background Mockup submenu ──
        bg_menu = view_menu.addMenu("Background Mockup")

        load_bg_action = QAction("Load Mockup Image...", self)
        load_bg_action.triggered.connect(self._load_background_image)
        bg_menu.addAction(load_bg_action)

        self._toggle_bg_action = QAction("Show Mockup", self)
        self._toggle_bg_action.setCheckable(True)
        self._toggle_bg_action.setChecked(True)
        self._toggle_bg_action.setShortcut("Ctrl+M")
        self._toggle_bg_action.toggled.connect(self._toggle_background_image)
        bg_menu.addAction(self._toggle_bg_action)

        clear_bg_action = QAction("Clear Mockup Image", self)
        clear_bg_action.triggered.connect(self._clear_background_image)
        bg_menu.addAction(clear_bg_action)

        bg_menu.addSeparator()

        # Opacity sub-menu with preset values
        opacity_menu = bg_menu.addMenu("Opacity")
        self._opacity_group = QActionGroup(self)
        self._opacity_group.setExclusive(True)
        for pct in [10, 20, 30, 50, 70, 100]:
            act = QAction(f"{pct}%", self)
            act.setCheckable(True)
            if pct == 30:
                act.setChecked(True)
            act.triggered.connect(
                lambda checked, p=pct: self._set_background_opacity(p / 100.0)
            )
            self._opacity_group.addAction(act)
            opacity_menu.addAction(act)

    # ── Toolbar ────────────────────────────────────────────────────

    def _init_toolbar(self):
        tb = QToolBar("Main Toolbar", self)
        tb.setObjectName("main_toolbar")
        tb.setMovable(False)
        tb.setIconSize(QSize(16, 16))
        tb.setToolButtonStyle(Qt.ToolButtonTextBesideIcon)
        tb.setStyleSheet(
            "QToolBar { spacing: 2px; padding: 2px 4px; }"
            "QToolButton { padding: 3px 8px; border-radius: 3px; }"
            "QToolButton:hover { background: rgba(255,255,255,0.08); }"
            "QToolButton:pressed { background: rgba(255,255,255,0.15); }"
            "QToolButton:disabled { color: #666; }"
        )
        self.addToolBar(Qt.TopToolBarArea, tb)

        # File actions
        tb.addAction(self._save_action)

        tb.addSeparator()

        # Edit actions
        tb.addAction(self._undo_action)
        tb.addAction(self._redo_action)

        tb.addSeparator()

        # Build actions
        tb.addAction(self._compile_action)
        tb.addAction(self._stop_action)

        self._toolbar = tb
        self._update_compile_availability()

    # ── Theme ──────────────────────────────────────────────────────

    def _set_theme(self, theme):
        """Set the application theme and save to config."""
        apply_theme(QApplication.instance(), theme)
        self._config.theme = theme
        self._config.save()

    def _set_font_sizes(self):
        """Set a single font size for the entire UI."""
        current_size = self._config.font_size_px
        if not current_size or current_size <= 0:
            current_size = 9

        size, ok = QInputDialog.getInt(
            self, "Font Size", "Font size (pt):",
            value=current_size, min=6, max=48
        )
        if not ok:
            return

        # Apply via stylesheet (DPI-independent, overrides all widgets)
        app = QApplication.instance()
        base_ss = self._get_base_stylesheet(app)
        app.setStyleSheet(base_ss + f"\n* {{ font-size: {size}pt; }}")

        # Debug panel uses its own font
        self.debug_panel.set_output_font_size_pt(size)

        # Persist to config
        self._config.font_size_px = size
        self._config.save()
        self.statusBar().showMessage(f"Font size set to {size}pt (saved)")

    def _get_base_stylesheet(self, app):
        """Get the base stylesheet without any font-size override."""
        ss = app.styleSheet()
        # Remove any previous font-size override line we added
        import re
        ss = re.sub(r'\n\*\s*\{\s*font-size:\s*\d+pt;\s*\}', '', ss)
        return ss

    # ── Project operations ─────────────────────────────────────────

    def _update_recent_menu(self):
        """Update the Recent Projects submenu."""
        self._recent_menu.clear()
        recent = self._config.recent_projects
        if not recent:
            action = QAction("(No recent projects)", self)
            action.setEnabled(False)
            self._recent_menu.addAction(action)
            return

        for item in recent[:10]:
            project_path = item.get("project_path", "")
            sdk_root = item.get("sdk_root", "")
            display_name = item.get("display_name") or os.path.splitext(os.path.basename(project_path))[0]
            action = QAction(display_name, self)
            action.setToolTip(project_path)
            action.triggered.connect(
                lambda checked, p=project_path, r=sdk_root: self._open_recent_project(p, r)
            )
            self._recent_menu.addAction(action)

    def _open_app_dialog(self):
        """Show dialog to select and open an SDK example."""
        dialog = AppSelectorDialog(self, self.project_root)
        if dialog.exec_() != QDialog.Accepted:
            return

        entry = dialog.selected_entry
        sdk_root = normalize_path(dialog.egui_root)
        if not entry:
            return

        self.project_root = sdk_root
        self._config.sdk_root = sdk_root
        self._config.egui_root = sdk_root
        self._config.save()

        if entry.get("has_project"):
            try:
                self._open_project_path(entry.get("project_path", ""), preferred_sdk_root=sdk_root)
            except Exception as exc:
                QMessageBox.critical(self, "Error", f"Failed to open SDK example:\n{exc}")
            return

        try:
            self._import_legacy_example(entry, sdk_root)
        except Exception as exc:
            QMessageBox.critical(self, "Error", f"Failed to import legacy example:\n{exc}")

    def _set_sdk_root(self):
        path = QFileDialog.getExistingDirectory(self, "Select EmbeddedGUI SDK Root", self.project_root or "")
        if not path:
            return

        path = resolve_sdk_root_candidate(path)
        if not path:
            QMessageBox.warning(
                self,
                "Invalid SDK Root",
                "The selected directory does not contain a valid EmbeddedGUI SDK root.",
            )
            return

        self._apply_sdk_root(path, status_message=f"SDK root set to: {path}")

    def _download_sdk(self):
        target_dir = default_sdk_install_dir()
        progress = QProgressDialog(f"Preparing SDK download...\nTarget: {target_dir}", "Cancel", 0, 100, self)
        progress.setWindowTitle("Download EmbeddedGUI SDK")
        progress.setWindowModality(Qt.WindowModal)
        progress.setMinimumDuration(0)
        progress.setAutoClose(False)
        progress.setValue(0)

        def on_progress(message, percent):
            if progress.wasCanceled():
                raise RuntimeError("SDK download canceled by user")
            progress.setLabelText(message)
            if percent is not None:
                progress.setValue(max(0, min(100, percent)))
            QApplication.processEvents()

        try:
            sdk_root = ensure_sdk_downloaded(target_dir, progress_callback=on_progress)
        except Exception as exc:
            progress.close()
            QMessageBox.warning(
                self,
                "Download SDK Failed",
                "Failed to prepare an EmbeddedGUI SDK automatically.\n\n"
                f"Target location:\n{target_dir}\n\n"
                f"{exc}\n\n"
                "You can try again later, install git for clone fallback, or select an existing SDK root manually.",
            )
            return ""

        progress.setValue(100)
        progress.close()
        self._apply_sdk_root(sdk_root, status_message=f"SDK downloaded to: {sdk_root}")
        return sdk_root

    def maybe_prompt_initial_sdk_setup(self):
        if self._has_valid_sdk_root() or self._config.sdk_setup_prompted:
            return

        self._config.sdk_setup_prompted = True
        self._config.save()

        dialog = QMessageBox(self)
        dialog.setWindowTitle("Prepare EmbeddedGUI SDK")
        dialog.setIcon(QMessageBox.Information)
        dialog.setText("No EmbeddedGUI SDK was detected.")
        target_dir = default_sdk_install_dir()
        dialog.setInformativeText(
            "Designer can download a local SDK copy automatically.\n"
            f"Target location:\n{target_dir}\n\n"
            "It will try GitHub archive first, then Gitee archive, then Gitee git clone when git is available.\n"
            "You can also point Designer to an existing SDK root."
        )
        download_btn = dialog.addButton("Download SDK Automatically", QMessageBox.AcceptRole)
        select_btn = dialog.addButton("Select SDK Root...", QMessageBox.ActionRole)
        dialog.addButton("Skip for Now", QMessageBox.RejectRole)
        dialog.exec_()

        clicked = dialog.clickedButton()
        if clicked == download_btn:
            self._download_sdk()
        elif clicked == select_btn:
            self._set_sdk_root()

    def _open_recent_project(self, project_path, sdk_root=""):
        if not project_path:
            return
        try:
            self._open_project_path(project_path, preferred_sdk_root=sdk_root)
        except FileNotFoundError:
            reply = QMessageBox.question(
                self,
                "Recent Project Missing",
                f"The recent project path no longer exists:\n{project_path}\n\nRemove it from the recent project list?",
                QMessageBox.Yes | QMessageBox.No,
                QMessageBox.Yes,
            )
            if reply == QMessageBox.Yes:
                if self._config.remove_recent_project(project_path):
                    self._update_recent_menu()
                    self._welcome_page.refresh()
                self.statusBar().showMessage("Removed missing project from recent projects")
        except Exception as exc:
            QMessageBox.critical(self, "Error", f"Failed to open project:\n{exc}")

    def _import_legacy_example(self, entry, sdk_root):
        app_name = entry.get("app_name", "")
        app_dir = normalize_path(entry.get("app_dir", ""))
        project_path = normalize_path(os.path.join(app_dir, f"{app_name}.egui"))
        eguiproject_dir = os.path.join(app_dir, ".eguiproject")

        if os.path.exists(eguiproject_dir) and not os.path.isfile(project_path):
            QMessageBox.warning(
                self,
                "Legacy Example Conflict",
                "This example already contains a .eguiproject directory but has no .egui file. Please resolve the directory conflict manually before importing it into Designer.",
            )
            return

        project = self._create_standard_project_model(app_name, sdk_root, app_dir)
        self._scaffold_project_directory(app_dir, app_name, project.screen_width, project.screen_height)
        project.save(app_dir)
        self._open_project_path(project_path, preferred_sdk_root=sdk_root)

    def _update_window_title(self):
        """Update window title with current app name and dirty indicator."""
        title = f"EmbeddedGUI Designer - {self.app_name}"
        if self._project_dir:
            title += f" [{self._project_dir}]"
        if self._undo_manager.is_any_dirty():
            title += " *"
        self.setWindowTitle(title)

    def _default_new_project_parent_dir(self, sdk_root=""):
        sdk_root = normalize_path(sdk_root)
        if is_valid_sdk_root(sdk_root):
            return os.path.join(sdk_root, "example")

        if self._project_dir:
            return os.path.dirname(self._project_dir)

        last_project_path = normalize_path(self._config.last_project_path)
        if last_project_path:
            last_project_dir = last_project_path if os.path.isdir(last_project_path) else os.path.dirname(last_project_path)
            if last_project_dir:
                return os.path.dirname(last_project_dir) or last_project_dir

        return normalize_path(os.getcwd())

    def _new_project(self):
        """Create a new project in a dedicated app directory."""
        default_sdk_root = self.project_root if self._has_valid_sdk_root() else normalize_path(self._config.sdk_root)
        if default_sdk_root and not is_valid_sdk_root(default_sdk_root):
            default_sdk_root = ""
        default_parent_dir = self._default_new_project_parent_dir(default_sdk_root)
        dialog = NewProjectDialog(self, sdk_root=default_sdk_root, default_parent_dir=default_parent_dir)
        if dialog.exec_() != QDialog.Accepted:
            return

        sdk_root = normalize_path(dialog.sdk_root)
        project_dir = normalize_path(os.path.join(dialog.parent_dir, dialog.app_name))
        if os.path.isdir(project_dir) and os.listdir(project_dir):
            QMessageBox.warning(
                self,
                "Directory Conflict",
                f"The target directory already exists and is not empty:\n{project_dir}",
            )
            return

        os.makedirs(project_dir, exist_ok=True)
        project = Project(screen_width=dialog.screen_width, screen_height=dialog.screen_height, app_name=dialog.app_name)
        project.sdk_root = sdk_root
        project.project_dir = project_dir
        project.create_new_page("main_page")
        self._scaffold_project_directory(project_dir, dialog.app_name, dialog.screen_width, dialog.screen_height)
        project.save(project_dir)
        self._open_loaded_project(project, project_dir, preferred_sdk_root=sdk_root)
        self.statusBar().showMessage(f"Created project: {dialog.app_name}")

    def _open_project_path(self, path, preferred_sdk_root="", silent=False):
        path = normalize_path(path)
        if not path:
            raise FileNotFoundError("Project path is empty")
        if not os.path.exists(path):
            raise FileNotFoundError(path)

        project = Project.load(path)
        project_dir = path if os.path.isdir(path) else os.path.dirname(path)
        self._open_loaded_project(project, project_dir, preferred_sdk_root=preferred_sdk_root, silent=silent)

    def _open_project(self):
        path, _ = QFileDialog.getOpenFileName(
            self, "Open Project", "",
            "EmbeddedGUI Projects (*.egui);;All Files (*.*)"
        )
        if not path:
            return
        try:
            self._open_project_path(path)
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to open project:\n{e}")

    def _save_project_files(self, project_dir):
        self.project.project_dir = project_dir
        self.project.sdk_root = self.project_root
        self._scaffold_project_directory(project_dir, self.project.app_name, self.project.screen_width, self.project.screen_height)
        self.project.save(project_dir)

        files = generate_all_files_preserved(self.project, project_dir, backup=True)
        for filename, content in files.items():
            filepath = os.path.join(project_dir, filename)
            with open(filepath, "w", encoding="utf-8") as f:
                f.write(content)
        return files

    def _save_project(self):
        if self.project is None:
            self.statusBar().showMessage("No project to save")
            return

        self._flush_pending_xml()

        if not self._project_dir:
            self._save_project_as()
            return

        os.makedirs(self._project_dir, exist_ok=True)
        files = self._save_project_files(self._project_dir)
        self._recreate_compiler()
        self._undo_manager.mark_all_saved()
        self._persist_current_project_to_config()
        self._refresh_project_watch_snapshot()
        self._update_window_title()
        self._update_compile_availability()
        self.statusBar().showMessage(f"Saved: {self._project_dir} ({len(files)} code file(s) updated)")

    def _save_project_as(self):
        if self.project is None:
            self.statusBar().showMessage("No project to save")
            return

        path = QFileDialog.getExistingDirectory(self, "Save Project To Directory")
        if not path:
            return

        path = normalize_path(path)
        if path != normalize_path(self._project_dir) and os.path.isdir(path) and os.listdir(path):
            QMessageBox.warning(
                self,
                "Directory Conflict",
                f"The selected directory is not empty:\n{path}",
            )
            return

        old_project_dir = self._project_dir
        os.makedirs(path, exist_ok=True)
        self._copy_project_sidecar_files(old_project_dir, path)
        files = self._save_project_files(path)
        self._project_dir = path
        self.project.project_dir = path
        self._recreate_compiler()
        self._undo_manager.mark_all_saved()
        self._persist_current_project_to_config()
        self._refresh_project_watch_snapshot()
        self._update_window_title()
        self._update_compile_availability()
        self.statusBar().showMessage(f"Saved: {path} ({len(files)} code file(s) updated)")

    def _close_project(self):
        """Close current project and return to welcome page."""
        if self.project is None:
            self._show_welcome_page()
            return

        if self._undo_manager.is_any_dirty():
            reply = QMessageBox.question(
                self, "Close Project",
                "There are unsaved changes. Do you want to save before closing?",
                QMessageBox.Save | QMessageBox.Discard | QMessageBox.Cancel,
                QMessageBox.Save
            )

            if reply == QMessageBox.Cancel:
                return
            if reply == QMessageBox.Save:
                self._save_project()

        if self.compiler is not None:
            self.compiler.stop_exe()
            self.compiler.cleanup()
            self.compiler = None

        self.preview_panel.stop_rendering()
        self._project_watch_timer.stop()
        self._project_watch_snapshot = {}
        self._external_reload_pending = False
        self.project = None
        self._project_dir = None
        self._undo_manager = UndoManager()
        self._clear_editor_state()
        self._show_welcome_page()
        self._update_compile_availability()
        self.statusBar().showMessage("Project closed")

    def _export_code(self):
        """Export all generated C files to a directory, preserving user code."""
        if not self.project:
            return
        path = QFileDialog.getExistingDirectory(
            self, "Export C Code To Directory"
        )
        if not path:
            return
        files = generate_all_files_preserved(
            self.project, path, backup=True,
        )
        for filename, content in files.items():
            filepath = os.path.join(path, filename)
            with open(filepath, "w", encoding="utf-8") as f:
                f.write(content)
        self.statusBar().showMessage(
            f"Exported {len(files)} files to {path} (user code preserved)"
        )

    # ── Background Mockup Image ───────────────────────────────────

    def _load_background_image(self):
        """Load a mockup image for the current page."""
        if not self._current_page:
            QMessageBox.warning(self, "Warning", "No page is currently open.")
            return
        if not self._project_dir:
            QMessageBox.warning(self, "Warning", "Please save the project first.")
            return

        from PyQt5.QtGui import QPixmap
        path, _ = QFileDialog.getOpenFileName(
            self, "Load Mockup Image", "",
            "Images (*.png *.jpg *.jpeg *.bmp);;All Files (*.*)"
        )
        if not path:
            return
        pixmap = QPixmap(path)
        if pixmap.isNull():
            QMessageBox.warning(self, "Error", f"Failed to load image:\n{path}")
            return

        # Check if image size matches screen size
        sw = self.project.screen_width if self.project else 240
        sh = self.project.screen_height if self.project else 320
        if pixmap.width() != sw or pixmap.height() != sh:
            QMessageBox.information(
                self, "Image Size Mismatch",
                f"The mockup image size ({pixmap.width()}×{pixmap.height()}) "
                f"does not match the screen size ({sw}×{sh}).\n\n"
                f"The image will be scaled to {sw}×{sh} to fit the canvas."
            )
            pixmap = pixmap.scaled(sw, sh, Qt.IgnoreAspectRatio, Qt.SmoothTransformation)

        # Copy image to .eguiproject/mockup/
        eguiproject_dir = os.path.join(self._project_dir, ".eguiproject")
        mockup_dir = os.path.join(eguiproject_dir, "mockup")
        os.makedirs(mockup_dir, exist_ok=True)

        import shutil
        filename = os.path.basename(path)
        dest = os.path.join(mockup_dir, filename)
        # Handle name collision
        if os.path.abspath(path) != os.path.abspath(dest):
            base, ext = os.path.splitext(filename)
            counter = 1
            while os.path.isfile(dest):
                dest = os.path.join(mockup_dir, f"{base}_{counter}{ext}")
                filename = f"{base}_{counter}{ext}"
                counter += 1
            shutil.copy2(path, dest)

        # Store relative path (relative to .eguiproject/)
        rel_path = f"mockup/{filename}"
        self._current_page.mockup_image_path = rel_path
        self._current_page.mockup_image_visible = True
        self._toggle_bg_action.setChecked(True)

        # Apply to overlay
        self.preview_panel.set_background_image(pixmap)
        self._refresh_project_watch_snapshot()
        self.statusBar().showMessage(f"Mockup image loaded: {filename}")

    def _toggle_background_image(self, visible):
        """Toggle mockup image visibility."""
        if self._current_page:
            self._current_page.mockup_image_visible = visible
        self.preview_panel.set_background_image_visible(visible)

    def _clear_background_image(self):
        """Remove the mockup image from the current page."""
        if self._current_page and self._current_page.mockup_image_path:
            # Delete file from .eguiproject/mockup/
            if self._project_dir:
                eguiproject_dir = os.path.join(self._project_dir, ".eguiproject")
                full_path = os.path.join(eguiproject_dir, self._current_page.mockup_image_path)
                if os.path.isfile(full_path):
                    try:
                        os.remove(full_path)
                    except OSError:
                        pass
            self._current_page.mockup_image_path = ""
            self._current_page.mockup_image_visible = True
        self.preview_panel.clear_background_image()
        self._refresh_project_watch_snapshot()
        self.statusBar().showMessage("Mockup image cleared")

    def _set_background_opacity(self, opacity):
        """Set mockup image opacity."""
        if self._current_page:
            self._current_page.mockup_image_opacity = opacity
        self.preview_panel.set_background_image_opacity(opacity)

    def _apply_page_mockup(self):
        """Load and apply the current page's mockup image."""
        if not self._current_page:
            self.preview_panel.clear_background_image()
            return

        path = self._current_page.mockup_image_path
        if path and self._project_dir:
            eguiproject_dir = os.path.join(self._project_dir, ".eguiproject")
            full_path = os.path.join(eguiproject_dir, path)
            if os.path.isfile(full_path):
                from PyQt5.QtGui import QPixmap
                pixmap = QPixmap(full_path)
                if not pixmap.isNull():
                    # Scale to screen size if needed
                    sw = self.project.screen_width if self.project else 240
                    sh = self.project.screen_height if self.project else 320
                    if pixmap.width() != sw or pixmap.height() != sh:
                        pixmap = pixmap.scaled(sw, sh, Qt.IgnoreAspectRatio, Qt.SmoothTransformation)
                    self.preview_panel.set_background_image(pixmap)
                    self.preview_panel.set_background_image_visible(
                        self._current_page.mockup_image_visible
                    )
                    self.preview_panel.set_background_image_opacity(
                        self._current_page.mockup_image_opacity
                    )
                    self._toggle_bg_action.setChecked(
                        self._current_page.mockup_image_visible
                    )
                    # Update opacity menu checkmarks
                    target_pct = int(self._current_page.mockup_image_opacity * 100)
                    for act in self._opacity_group.actions():
                        act.setChecked(act.text() == f"{target_pct}%")
                    return
        self.preview_panel.clear_background_image()

    def _apply_project(self):
        """Refresh all panels from the current project."""
        if not self.project:
            return

        # Load project-level custom widget plugins
        if self._project_dir:
            from ..model.widget_registry import WidgetRegistry
            custom_dir = os.path.join(self._project_dir, ".eguiproject", "custom_widgets")
            WidgetRegistry.instance().load_custom_widgets(custom_dir)

        self.project_dock.set_project(self.project)
        self.preview_panel.update_screen_size(self.project.screen_width, self.project.screen_height)
        if self.compiler is not None:
            self.compiler.set_screen_size(self.project.screen_width, self.project.screen_height)
        self._clear_page_tabs()

        # Refresh resource panel with catalog
        # Resource panel uses .eguiproject/resources/ as the source directory
        eguiproject_res_dir = self._get_eguiproject_resource_dir()
        res_dir = self._get_resource_dir()  # resource/ for property panel
        catalog = self.project.resource_catalog if self.project else None
        self.res_panel.set_resource_catalog(catalog)
        self.res_panel.set_resource_dir(eguiproject_res_dir)
        self.property_panel.set_resource_dir(res_dir)
        self.property_panel.set_source_resource_dir(eguiproject_res_dir)
        self.property_panel.set_resource_catalog(catalog)

        # Refresh i18n string catalog
        string_catalog = self.project.string_catalog if self.project else None
        self.res_panel.set_string_catalog(string_catalog)
        # Feed string keys to property panel for @string/ completions
        if string_catalog:
            self.property_panel.set_string_keys(string_catalog.all_keys)
        else:
            self.property_panel.set_string_keys([])

        self._update_compile_availability()
        if self.compiler is not None:
            self._start_precompile()

    def _start_precompile(self):
        """Start background precompile if exe doesn't exist."""
        if not self.project:
            return
        if self.compiler is None or not self.compiler.can_build():
            reason = "SDK unavailable, compile preview disabled"
            if self.compiler is not None and self.compiler.get_build_error():
                reason = self.compiler.get_build_error()
            self._switch_to_python_preview(reason)
            return
        if self._precompile_worker is not None and self._precompile_worker.isRunning():
            return
        if not self.compiler.is_exe_ready():
            self.statusBar().showMessage("Background compiling...")
            self.debug_panel.log_action("Starting background precompile...")
            self.debug_panel.log_cmd(
                f"make -j main.exe APP={self.app_name} PORT=designer EGUI_APP_ROOT_PATH={self.compiler.app_root_arg} COMPILE_DEBUG= COMPILE_OPT_LEVEL=-O0"
            )
            self._precompile_worker = self.compiler.precompile_async(
                callback=self._on_precompile_done
            )

    def _on_precompile_done(self, success, message):
        """Callback when background precompile finishes."""
        self._precompile_worker = None
        if success:
            self.statusBar().showMessage("Ready (precompiled)", 3000)
            self.debug_panel.log_success("Background precompile completed")
        else:
            self.statusBar().showMessage("Precompile failed", 5000)
            self.debug_panel.log_error("Background precompile failed")
            self.debug_panel.log_compile_output(False, message)
            self.debug_dock.show()

    # ── Resource panel integration ──────────────────────────────────

    def _get_resource_dir(self):
        """Compute the resource directory path (resource/) for the current project.

        Used for the generation pipeline and for property_panel to scan
        generated fonts in resource/font/.
        """
        if self._project_dir:
            return os.path.join(self._project_dir, "resource")
        return ""

    def _get_eguiproject_resource_dir(self):
        """Compute the .eguiproject/resources/ path for the current project.

        This is the authoritative directory for all source resource files.
        Used by the resource panel for browsing and importing.
        """
        if self._project_dir:
            return os.path.join(self._project_dir, ".eguiproject", "resources")
        return ""

    def _get_eguiproject_images_dir(self):
        """Compute the .eguiproject/resources/images/ path.

        Authoritative directory for source image files.
        Used for Page XML image path resolution.
        """
        res_dir = self._get_eguiproject_resource_dir()
        return os.path.join(res_dir, "images") if res_dir else ""

    def _on_resource_selected(self, res_type, filename):
        """User selected/assigned a resource from the ResourcePanel."""
        if self._selected_widget is None:
            return
        if res_type == "image" and "image_file" in self._selected_widget.properties:
            self._selected_widget.properties["image_file"] = filename
        elif res_type == "font" and "font_file" in self._selected_widget.properties:
            self._selected_widget.properties["font_file"] = filename
        else:
            return
        self.property_panel.set_widget(self._selected_widget)
        self._on_model_changed()

    def _on_resource_imported(self):
        """Resource files were imported — sync catalog and auto-regenerate."""
        # Sync catalog from resource panel back to project
        if self.project:
            catalog = self.res_panel.get_resource_catalog()
            self.project.resource_catalog = catalog
            self.property_panel.set_resource_catalog(catalog)
            # Sync i18n string catalog
            self.project.string_catalog = self.res_panel.get_string_catalog()
            self.property_panel.set_string_keys(self.project.string_catalog.all_keys)
        self._resources_need_regen = True
        # Auto-trigger resource generation with debounce
        self._refresh_project_watch_snapshot()
        self._regen_timer.start()
        self.statusBar().showMessage("Resources changed, will regenerate...")

    def _on_resource_dropped(self, widget, res_type, filename):
        """Resource was dropped onto a widget in the preview overlay."""
        self._selected_widget = widget
        # Assign the resource filename to the matching property
        if res_type == "image" and "image_file" in widget.properties:
            widget.properties["image_file"] = filename
        elif res_type == "font" and "font_file" in widget.properties:
            widget.properties["font_file"] = filename
        self.property_panel.set_widget(widget)
        self._on_model_changed()

    def _run_resource_generation(self, silent=False):
        if not self.project or not self._project_dir:
            return False
        if not self._has_valid_sdk_root():
            if not silent:
                QMessageBox.warning(
                    self,
                    "SDK Root Missing",
                    "A valid EmbeddedGUI SDK root is required to run resource generation.",
                )
            self.debug_panel.log_error("Resource generation skipped: SDK root is missing or invalid")
            return False

        res_dir = self._get_resource_dir()
        eguiproject_res_dir = self._get_eguiproject_resource_dir()
        src_dir = os.path.join(res_dir, "src") if res_dir else ""
        if not res_dir or not eguiproject_res_dir or not os.path.isdir(eguiproject_res_dir):
            if not silent:
                QMessageBox.warning(
                    self,
                    "Error",
                    "No .eguiproject/resources directory found.\nPlease import resources first.",
                )
            return False

        self.project.sync_resources_to_src(self._project_dir)

        try:
            ResourceConfigGenerator().generate_and_save(self.project, src_dir)
        except Exception as exc:
            self.debug_panel.log_error(f"Resource config generation failed: {exc}")
            if not silent:
                QMessageBox.warning(self, "Error", f"Failed to generate resource config:\n{exc}")
            return False

        import subprocess
        import sys

        gen_script = os.path.join(self.project_root, "scripts", "tools", "app_resource_generate.py")
        if not os.path.isfile(gen_script):
            if not silent:
                QMessageBox.warning(self, "Error", f"Cannot find resource generator:\n{gen_script}")
            self.debug_panel.log_error(f"Resource generation skipped: missing generator {gen_script}")
            return False

        output_dir = os.path.join(self.project_root, "output")
        os.makedirs(output_dir, exist_ok=True)
        cmd = [
            sys.executable,
            gen_script,
            "-r",
            res_dir,
            "-o",
            output_dir,
            "-f",
            "true",
        ]
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=120,
                cwd=self.project_root,
            )
        except Exception as exc:
            self.debug_panel.log_error(f"Resource generation error: {exc}")
            if not silent:
                QMessageBox.warning(self, "Error", f"Failed to run resource generator:\n{exc}")
            return False

        if result.returncode != 0:
            err = result.stderr or result.stdout or "Unknown error"
            self.debug_panel.log_error(f"Resource generation failed (rc={result.returncode})")
            self.debug_panel.log_compile_output(False, err[:2000])
            if not silent:
                QMessageBox.warning(
                    self,
                    "Resource Generation Failed",
                    f"Return code {result.returncode}:\n{err[:2000]}",
                )
            return False

        self._resources_need_regen = False
        self.debug_panel.log_info("Resources generated successfully")
        return True

    def _generate_resources(self, silent=False):
        """Run the resource generation pipeline.

        Steps:
        1. Sync .eguiproject/resources/ -> resource/src/
        2. Generate app_resource_config.json from layout XML (ResourceConfigGenerator)
        3. Run app_resource_generate.py to produce C source files

        Args:
            silent: If True, suppress warning dialogs (used for auto-trigger).
        """
        self.statusBar().showMessage("Generating resources...")
        if self._run_resource_generation(silent=silent):
            self.statusBar().showMessage("Resource generation completed.")
        else:
            self.statusBar().showMessage("Resource generation FAILED.")

    def _ensure_resources_generated(self):
        """Generate app_resource_config.json from widget properties and run
        app_resource_generate.py if .eguiproject/resources/ exists.

        Called before each compile to ensure resource C files are up-to-date.
        Skips entirely when resources haven't changed since last generation.
        Runs silently — errors are logged to debug panel only.
        """
        if not self._resources_need_regen:
            return
        self._run_resource_generation(silent=True)

    # ── Page management ────────────────────────────────────────────

    def _switch_page(self, page_name):
        """Switch the editor to display a specific page."""
        if not self.project:
            return
        page = self.project.get_page_by_name(page_name)
        if page is None:
            return
        self._current_page = page
        self._selected_widget = None
        self.project_dock.set_current_page(page_name)

        # Initialize undo stack for this page if empty
        stack = self._undo_manager.get_stack(page_name)
        if not stack._history:
            stack.push(page.to_xml_string())
            stack.mark_saved()

        # Ensure page tab exists and is selected
        self._syncing_tabs = True
        self._ensure_page_tab(page_name)
        self.page_tab_bar.setCurrentTab(page_name)
        self._syncing_tabs = False

        # Update widget tree with this page's widgets
        # Create a shim so WidgetTreePanel.set_project() works
        self._page_shim = _PageProjectShim(page)
        self.widget_tree.set_project(self._page_shim)

        # Update preview & XML
        self._update_preview_overlay()
        self._sync_xml_to_editors()

        # Load mockup image for this page
        self._apply_page_mockup()

        # Trigger compile to show current page in preview
        self._trigger_compile()
        self._update_undo_actions()

    def _on_page_selected(self, page_name):
        """User clicked a page in the Project Explorer."""
        self._switch_page(page_name)

    def _on_page_added(self, page_name):
        """User requested a new page."""
        if not self.project:
            return
        self.project.create_new_page(page_name)
        self.project_dock.set_project(self.project)
        self._ensure_page_tab(page_name)
        self._switch_page(page_name)
        self._trigger_compile()

    def _on_page_removed(self, page_name):
        """User deleted a page."""
        if not self.project:
            return
        page = self.project.get_page_by_name(page_name)
        if page:
            self.project.remove_page(page)
            self._undo_manager.remove_stack(page_name)
            self.project_dock.set_project(self.project)
            self._remove_page_tab(page_name)
            # Delete generated files for the removed page so they are not
            # picked up by EGUI_CODE_SRC on the next build.
            if self._project_dir:
                delete_page_generated_files(self._project_dir, page_name)
            # Switch to another page
            if self.project.pages:
                self._switch_page(self.project.pages[0].name)
            self._trigger_compile()
            self._update_window_title()

    def _on_page_renamed(self, old_name, new_name):
        """User renamed a page."""
        if not self.project:
            return
        page = self.project.get_page_by_name(old_name)
        if page:
            page.file_path = f"layout/{new_name}.xml"
            # Update startup_page reference if needed
            if self.project.startup_page == old_name:
                self.project.startup_page = new_name
            self._undo_manager.rename_stack(old_name, new_name)
            self.project_dock.set_project(self.project)
            self._rename_page_tab(old_name, new_name)
            self._switch_page(new_name)

    def _on_startup_changed(self, page_name):
        """User changed the startup page."""
        if self.project:
            self.project.startup_page = page_name
            self.project_dock.set_project(self.project)
            self._trigger_compile()

    def _on_page_mode_changed(self, mode):
        """User switched between easy_page and activity mode."""
        if self.project:
            self.project.page_mode = mode
            self._trigger_compile()

    # ── Page tabs (qfluentwidgets TabBar) ─────────────────────────

    def _ensure_page_tab(self, page_name):
        """Add a tab for page_name if not already present. Returns the index."""
        for i in range(self.page_tab_bar.count()):
            if self.page_tab_bar.tabText(i) == page_name:
                return i
        # routeKey = page_name (unique per page)
        self.page_tab_bar.addTab(page_name, page_name, None)
        return self.page_tab_bar.count() - 1

    def _remove_page_tab(self, page_name):
        for i in range(self.page_tab_bar.count()):
            if self.page_tab_bar.tabText(i) == page_name:
                self.page_tab_bar.removeTab(i)
                return

    def _rename_page_tab(self, old_name, new_name):
        for i in range(self.page_tab_bar.count()):
            if self.page_tab_bar.tabText(i) == old_name:
                self.page_tab_bar.setTabText(i, new_name)
                return

    def _on_page_tab_changed(self, index):
        if self._syncing_tabs:
            return
        if index < 0:
            return
        page_name = self.page_tab_bar.tabText(index)
        if self._current_page and page_name == self._current_page.name:
            return
        self._switch_page(page_name)

    def _on_page_tab_closed(self, index):
        if index < 0:
            return
        page_name = self.page_tab_bar.tabText(index)
        self.page_tab_bar.removeTab(index)
        # If current page tab closed, switch to another open tab or fallback
        if self._current_page and self._current_page.name == page_name:
            if self.page_tab_bar.count() > 0:
                new_index = min(index, self.page_tab_bar.count() - 1)
                self.page_tab_bar.setCurrentIndex(new_index)
                self._on_page_tab_changed(new_index)
            elif self.project and self.project.pages:
                self._switch_page(self.project.pages[0].name)

    def _show_tab_context_menu(self, pos):
        index = -1
        for i in range(self.page_tab_bar.count()):
            r = self.page_tab_bar.tabRect(i)
            if r.contains(pos):
                index = i
                break

        menu = QMenu(self)

        if index >= 0:
            close_tab = menu.addAction("Close")
            close_others = menu.addAction("Close Others")
            close_all = menu.addAction("Close All")
        else:
            close_tab = close_others = close_all = None

        action = menu.exec_(self.page_tab_bar.mapToGlobal(pos))
        if action is None:
            return

        if action == close_tab and index >= 0:
            self._on_page_tab_closed(index)
        elif action == close_others and index >= 0:
            keep_name = self.page_tab_bar.tabText(index)
            names_to_remove = []
            for i in range(self.page_tab_bar.count()):
                n = self.page_tab_bar.tabText(i)
                if n != keep_name:
                    names_to_remove.append(n)
            for n in names_to_remove:
                self._remove_page_tab(n)
        elif action == close_all:
            self._clear_page_tabs()

    # ── Widget selection / editing ─────────────────────────────────

    def _on_widget_selected(self, widget):
        """Widget selected from tree panel."""
        self._selected_widget = widget
        self.property_panel.set_widget(widget)
        self.preview_panel.set_selected(widget)

    def _on_preview_widget_selected(self, widget):
        """Widget selected from preview panel overlay."""
        self._selected_widget = widget
        self.property_panel.set_widget(widget)

    def _on_widget_moved(self, widget, new_x, new_y):
        """Widget dragged on preview overlay."""
        if widget == self._selected_widget:
            self.property_panel.set_widget(widget)
        self._on_model_changed()

    def _on_widget_resized(self, widget, new_width, new_height):
        """Widget resized on preview overlay."""
        if widget == self._selected_widget:
            self.property_panel.set_widget(widget)
        self._on_model_changed()

    def _on_widget_reordered(self, widget, new_index):
        """Widget reordered within a layout container."""
        self.widget_tree.rebuild_tree()
        self._on_model_changed()

    def _on_tree_changed(self):
        """Widget tree structure changed (add/delete/reorder)."""
        self._on_model_changed()

    def _on_property_changed(self):
        """A property value was changed in the property panel."""
        self.widget_tree.rebuild_tree()
        self._on_model_changed()

    def _on_model_changed(self):
        """Common handler: model changed → record snapshot + update preview + XML + recompile."""
        if self._current_page and not self._undoing:
            xml = self._current_page.to_xml_string()
            stack = self._undo_manager.get_stack(self._current_page.name)
            stack.push(xml)
        self._update_preview_overlay()
        self._sync_xml_to_editors()
        self._trigger_compile()
        self._update_undo_actions()
        self._update_window_title()

    # ── Undo / Redo ─────────────────────────────────────────────────

    def _undo(self):
        if not self._current_page:
            return
        stack = self._undo_manager.get_stack(self._current_page.name)
        xml = stack.undo()
        if xml is not None:
            self._apply_xml_snapshot(xml)

    def _redo(self):
        if not self._current_page:
            return
        stack = self._undo_manager.get_stack(self._current_page.name)
        xml = stack.redo()
        if xml is not None:
            self._apply_xml_snapshot(xml)

    def _apply_xml_snapshot(self, xml):
        """Restore page state from an XML snapshot without recording a new undo entry."""
        self._undoing = True
        try:
            images_dir = self._get_eguiproject_images_dir()
            src_dir = images_dir if images_dir else None
            new_page = Page.from_xml_string(xml, self._current_page.file_path, src_dir=src_dir)
            self._current_page.root_widget = new_page.root_widget
            self._current_page.user_fields = new_page.user_fields
            # Refresh UI
            self._page_shim = _PageProjectShim(self._current_page)
            self.widget_tree.set_project(self._page_shim)
            self._selected_widget = None
            self.property_panel.set_widget(None)
            self._update_preview_overlay()
            self._sync_xml_to_editors()
            self._trigger_compile()
        finally:
            self._undoing = False
        self._update_undo_actions()
        self._update_window_title()

    def _update_undo_actions(self):
        """Enable/disable Undo and Redo menu actions based on stack state."""
        if self._current_page:
            stack = self._undo_manager.get_stack(self._current_page.name)
            self._undo_action.setEnabled(stack.can_undo())
            self._redo_action.setEnabled(stack.can_redo())
        else:
            self._undo_action.setEnabled(False)
            self._redo_action.setEnabled(False)

    def _on_drag_started(self):
        """Preview drag/resize began — start undo batch."""
        if self._current_page:
            stack = self._undo_manager.get_stack(self._current_page.name)
            stack.begin_batch()

    def _on_drag_finished(self):
        """Preview drag/resize ended — commit undo batch."""
        if self._current_page:
            xml = self._current_page.to_xml_string()
            stack = self._undo_manager.get_stack(self._current_page.name)
            stack.end_batch(xml)
            self._update_undo_actions()
            self._update_window_title()

    # ── XML bidirectional sync ─────────────────────────────────────

    def _sync_xml_to_editors(self):
        """Push current page XML to the code editors (Design → Code)."""
        if not self._current_page:
            return
        try:
            xml_text = self._current_page.to_xml_string()
            self.editor_tabs.set_xml_text(xml_text)
        except Exception:
            pass

    def _on_xml_changed(self, xml_text):
        """User edited XML in the code editor (Code → Design)."""
        if not self._current_page:
            return
        try:
            images_dir = self._get_eguiproject_images_dir()
            src_dir = images_dir if images_dir else None
            new_page = Page.from_xml_string(xml_text, self._current_page.file_path, src_dir=src_dir)
            self._current_page.root_widget = new_page.root_widget
            self._current_page.user_fields = new_page.user_fields

            if not self._undoing:
                stack = self._undo_manager.get_stack(self._current_page.name)
                stack.push(xml_text)

            # Refresh tree and preview (without re-syncing XML back)
            self._page_shim = _PageProjectShim(self._current_page)
            self.widget_tree.set_project(self._page_shim)
            self._update_preview_overlay()
            self._trigger_compile()
            self._update_undo_actions()
            self._update_window_title()
        except Exception:
            # XML parse error — ignore until user fixes it
            pass

    # ── Preview / Compile ──────────────────────────────────────────

    def _update_preview_overlay(self):
        """Update the preview overlay with current page widgets."""
        if self._current_page:
            compute_page_layout(self._current_page)
            widgets = self._current_page.get_all_widgets()
            self.preview_panel.set_widgets(widgets)
            self.preview_panel.set_selected(self._selected_widget)
            if self.compiler is None or not self.compiler.can_build() or self.preview_panel.is_python_preview_active():
                reason = ""
                if self.compiler is None:
                    reason = "SDK unavailable, compile preview disabled"
                elif not self.compiler.can_build():
                    reason = self.compiler.get_build_error()
                self._refresh_python_preview(reason)

    def _set_overlay_mode(self, mode):
        self.preview_panel.set_overlay_mode(mode)
        # Persist layout config
        self._config.overlay_mode = mode
        self._config.save()
        # Sync menu checkmarks
        act = self._overlay_mode_actions.get(mode)
        if act:
            act.setChecked(True)

    def _flip_overlay_layout(self):
        """Swap preview/overlay and persist the flipped state."""
        self.preview_panel.flip_layout()
        self._config.overlay_flipped = self.preview_panel._flipped
        self._config.save()

    def _toggle_auto_compile(self, enabled):
        self.auto_compile = enabled

    def _trigger_compile(self):
        """Trigger a debounced compile."""
        if not self.auto_compile:
            return
        if self.compiler is None:
            self._refresh_python_preview("SDK unavailable, compile preview disabled")
            return
        self._compile_timer.start()

    def _flush_pending_xml(self):
        """Flush any pending XML edits from the editor into the model."""
        if self.editor_tabs._parse_timer.isActive():
            self.editor_tabs._parse_timer.stop()
            self.editor_tabs._emit_xml_changed()

    def _do_compile_and_run(self):
        """Execute compile and run cycle (async, multi-file)."""
        if not self.project:
            return
        if self.compiler is None or not self.compiler.can_build():
            reason = "SDK unavailable, compile preview disabled"
            if self.compiler is not None and self.compiler.get_build_error():
                reason = self.compiler.get_build_error()
            self._switch_to_python_preview(reason)
            self.statusBar().showMessage("Compile preview unavailable")
            return
        if self._compile_worker is not None and self._compile_worker.isRunning():
            # Mark that we need to recompile after current one finishes
            self._pending_compile = True
            return
        # Wait for precompile to finish to avoid conflicts
        if self._precompile_worker is not None and self._precompile_worker.isRunning():
            self.statusBar().showMessage("Waiting for background compile...")
            self.debug_panel.log_info("Waiting for background compile to finish...")
            self._pending_compile = True
            return

        self._pending_compile = False

        # Always use the latest editor content
        self._flush_pending_xml()

        self.statusBar().showMessage("Compiling...")
        self.preview_panel.status_label.setText("Compiling...")

        self.debug_panel.log_action("Starting compile and run...")
        self.debug_panel.log_info(f"Generating code for {len(self.project.pages)} page(s)")

        # Generate resource config + resource C files if needed
        self._ensure_resources_generated()

        # Temporarily set startup_page to current page for preview
        original_startup = self.project.startup_page
        if self._current_page:
            self.project.startup_page = self._current_page.name

        files = generate_all_files(self.project)

        # Restore original startup_page
        self.project.startup_page = original_startup

        self.debug_panel.log_info(f"Generated {len(files)} file(s): {', '.join(files.keys())}")
        self.debug_panel.log_cmd(
            f"make -j main.exe APP={self.app_name} PORT=designer EGUI_APP_ROOT_PATH={self.compiler.app_root_arg} COMPILE_DEBUG= COMPILE_OPT_LEVEL=-O0"
        )

        self._compile_worker = self.compiler.compile_and_run_async(
            code=None,
            callback=self._on_compile_finished,
            files_dict=files,
        )
        # Connect log signal for detailed timing info
        self._compile_worker.log.connect(self._on_compile_log)

    def _on_compile_log(self, message, msg_type):
        """Handle log messages from compile worker."""
        self.debug_panel.log(message, msg_type)

    def _on_compile_finished(self, success, message, old_process):
        """Callback when background compilation completes."""
        # Update debug panel with compile output
        self.debug_panel.log_compile_output(success, message)

        # Check if we need to recompile due to pending changes
        if self._pending_compile:
            self._pending_compile = False
            self._trigger_compile()

        if success:
            self.statusBar().showMessage(message)
            self.preview_panel.status_label.setText(f"OK - {message}")
            # Start headless frame rendering
            self.preview_panel.start_rendering(self.compiler)
            self.debug_panel.log_action("Headless preview started")
        else:
            self.statusBar().showMessage("Compile FAILED - see Debug Output")
            if self.compiler is not None:
                self.compiler.stop_exe()
            self._switch_to_python_preview(message.splitlines()[0] if message else "Compile failed")
            # Show debug dock on compile failure
            self.debug_dock.show()
            self.debug_dock.raise_()
        self._update_compile_availability()

    def _on_preview_runtime_failed(self, reason):
        if self.compiler is not None:
            self.compiler.stop_exe()
        self.debug_panel.log_error(reason or "Headless preview stopped responding")
        self.debug_dock.show()
        self.debug_dock.raise_()
        self._switch_to_python_preview(reason or "Headless preview stopped responding")
        self._update_compile_availability()

    def _try_embed_exe(self):
        """Legacy - headless rendering replaces window embedding."""
        pass

    def _stop_exe(self):
        self.preview_panel.stop_rendering()
        if self.compiler is not None:
            self.compiler.stop_exe()
        self.preview_panel.status_label.setText("Preview stopped")
        self._update_compile_availability()

    def closeEvent(self, event):
        if self.project and self._undo_manager.is_any_dirty():
            reply = QMessageBox.question(
                self, "Unsaved Changes",
                "There are unsaved changes. Do you want to save before closing?",
                QMessageBox.Save | QMessageBox.Discard | QMessageBox.Cancel,
                QMessageBox.Save
            )
            if reply == QMessageBox.Cancel:
                event.ignore()
                return
            elif reply == QMessageBox.Save:
                self._save_project()

        # Save config
        self._config.auto_compile = self.auto_compile
        self._config.overlay_mode = self.preview_panel._mode
        self._config.overlay_flipped = self.preview_panel._flipped
        self._save_window_state_to_config()
        if self._has_valid_sdk_root():
            self._config.sdk_root = self.project_root
            self._config.egui_root = self.project_root
        self._config.save()

        self.preview_panel.stop_rendering()
        if self.compiler is not None:
            self.compiler.cleanup()
        event.accept()


class _PageProjectShim:
    """Shim that makes a Page look like a Project for WidgetTreePanel.

    WidgetTreePanel expects ``project.root_widgets`` to be a list.
    This adapter provides that interface for a single page.
    """

    def __init__(self, page):
        self._page = page

    @property
    def root_widgets(self):
        if self._page and self._page.root_widget:
            return [self._page.root_widget]
        return []
