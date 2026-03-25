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
import re
import shutil

from PyQt5.QtWidgets import (
    QMainWindow, QWidget, QVBoxLayout,
    QAction, QActionGroup, QFileDialog, QStatusBar,
    QMessageBox, QScrollArea, QDockWidget, QMenu,
    QApplication, QDialog, QStackedWidget, QToolBar, QInputDialog, QProgressDialog,
)
from PyQt5.QtCore import Qt, QTimer, QSize, QByteArray, QSignalBlocker
from PyQt5.QtGui import QIcon

from qfluentwidgets import TabBar, TabCloseButtonDisplayMode

from .widget_tree import WidgetTreePanel
from .property_panel import PropertyPanel
from .preview_panel import PreviewPanel, MODE_VERTICAL, MODE_HORIZONTAL, MODE_HIDDEN
from .editor_tabs import EditorTabs, MODE_DESIGN, MODE_SPLIT, MODE_CODE
from .project_dock import ProjectExplorerDock
from .resource_panel import ResourcePanel
from .history_panel import HistoryPanel
from .diagnostics_panel import DiagnosticsPanel
from .animations_panel import AnimationsPanel
from .page_fields_panel import PageFieldsPanel
from .page_timers_panel import PageTimersPanel
from .app_selector import AppSelectorDialog
from .new_project_dialog import NewProjectDialog
from .welcome_page import WelcomePage
from .debug_panel import DebugPanel
from ..model.widget_model import WidgetModel
from ..model.project import Project
from ..model.page import Page
from ..model.config import get_config
from ..model.sdk_bootstrap import AUTO_DOWNLOAD_STRATEGY_TEXT, default_sdk_install_dir, describe_sdk_source, ensure_sdk_downloaded
from ..model.workspace import (
    find_sdk_root,
    is_valid_sdk_root,
    normalize_path,
    resolve_available_sdk_root,
    resolve_sdk_root_candidate,
)
from ..model.resource_binding import assign_resource_to_widget
from ..model.resource_usage import (
    collect_project_resource_usages,
    rewrite_project_resource_references,
    rewrite_project_string_references,
)
from ..model.selection_state import SelectionState
from ..model.diagnostics import analyze_page, analyze_project_callback_conflicts, analyze_selection, sort_diagnostic_entries
from ..model.undo_manager import UndoManager
from ..generator.code_generator import (
    collect_page_callback_stubs,
    generate_all_files,
    generate_all_files_preserved,
    generate_page_user_source,
    generate_uicode,
    render_page_callback_stub,
)
from ..generator.user_code_preserver import compute_source_hash, embed_source_hash, read_existing_file
from ..generator.resource_config_generator import ResourceConfigGenerator
from ..engine.compiler import CompilerEngine
from ..engine.layout_engine import compute_layout, compute_page_layout
from ..utils.scaffold import make_app_build_mk_content, make_app_config_h_content, make_empty_resource_config_content
from .theme import apply_theme
from .widgets.page_navigator import PageNavigator, PAGE_TEMPLATES


_DETACHED_WORKERS = set()


def _release_detached_worker(worker):
    _DETACHED_WORKERS.discard(worker)
    try:
        worker.deleteLater()
    except Exception:
        pass


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


def _callback_definition_exists(content, callback_name):
    if not content or not callback_name:
        return False
    pattern = rf"^\s*(?:static\s+)?void\s+{re.escape(callback_name)}\s*\("
    return re.search(pattern, content, re.MULTILINE) is not None


def _resolve_page_callback_target(page, callback_name, signature):
    for callback in collect_page_callback_stubs(page):
        if callback.get("name") == callback_name:
            return callback
    kind = "timer" if "egui_timer_t" in (signature or "") else "view"
    return {
        "kind": kind,
        "name": callback_name,
        "signature": signature,
    }


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
        self._selection_state = SelectionState()
        self._current_page = None      # currently-displayed Page object
        self._clipboard_payload = None
        self._paste_serial = 0
        self._async_generation = 0
        self._is_closing = False

        # Debounce timer for compile
        self._compile_timer = QTimer(self)
        self._compile_timer.setSingleShot(True)
        self._compile_timer.setInterval(500)
        self._compile_timer.timeout.connect(self._do_compile_and_run)

        # Timer to find and embed exe window after compile (legacy, kept for compat)
        self._embed_timer = QTimer(self)
        self._embed_timer.setSingleShot(True)
        self._embed_timer.setInterval(0)  # Immediate - no delay
        self._embed_timer.timeout.connect(self._try_embed_exe)

        # Debounce timer for resource generation
        self._regen_timer = QTimer(self)
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
        self._active_batch_source = ""
        self._project_watch_snapshot = {}
        self._external_reload_pending = False

        self._project_watch_timer = QTimer(self)
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

        self.page_nav_dock = QDockWidget("Page Navigator", self)
        self.page_nav_dock.setObjectName("page_navigator_dock")
        self.page_nav_dock.setAllowedAreas(Qt.LeftDockWidgetArea | Qt.RightDockWidgetArea)
        self.page_navigator = PageNavigator()
        self.page_nav_dock.setWidget(self.page_navigator)
        self.page_nav_dock.setMinimumWidth(180)
        self.addDockWidget(Qt.LeftDockWidgetArea, self.page_nav_dock)
        self.splitDockWidget(self.project_dock, self.page_nav_dock, Qt.Vertical)

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
        self.preview_panel.set_show_grid(self._config.show_grid)
        self.preview_panel.set_grid_size(self._config.grid_size)
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
        # Stack below the page navigator
        self.splitDockWidget(self.page_nav_dock, self.res_dock, Qt.Vertical)

        # ── Bottom dock: Debug Output ──
        self.debug_dock = QDockWidget("Debug Output", self)
        self.debug_dock.setObjectName("debug_output_dock")
        self.debug_dock.setAllowedAreas(Qt.AllDockWidgetAreas)
        self.debug_panel = DebugPanel()
        self.debug_dock.setWidget(self.debug_panel)
        self.addDockWidget(Qt.BottomDockWidgetArea, self.debug_dock)

        self.history_dock = QDockWidget("History", self)
        self.history_dock.setObjectName("history_dock")
        self.history_dock.setAllowedAreas(Qt.AllDockWidgetAreas)
        self.history_panel = HistoryPanel()
        self.history_dock.setWidget(self.history_panel)
        self.addDockWidget(Qt.BottomDockWidgetArea, self.history_dock)
        self.tabifyDockWidget(self.debug_dock, self.history_dock)

        self.diagnostics_dock = QDockWidget("Diagnostics", self)
        self.diagnostics_dock.setObjectName("diagnostics_dock")
        self.diagnostics_dock.setAllowedAreas(Qt.AllDockWidgetAreas)
        self.diagnostics_panel = DiagnosticsPanel()
        self.diagnostics_dock.setWidget(self.diagnostics_panel)
        self.addDockWidget(Qt.BottomDockWidgetArea, self.diagnostics_dock)
        self.tabifyDockWidget(self.history_dock, self.diagnostics_dock)

        self.animations_dock = QDockWidget("Animations", self)
        self.animations_dock.setObjectName("animations_dock")
        self.animations_dock.setAllowedAreas(Qt.AllDockWidgetAreas)
        self.animations_panel = AnimationsPanel()
        self.animations_dock.setWidget(self.animations_panel)
        self.addDockWidget(Qt.RightDockWidgetArea, self.animations_dock)
        self.tabifyDockWidget(self.props_dock, self.animations_dock)

        self.page_fields_dock = QDockWidget("Page Fields", self)
        self.page_fields_dock.setObjectName("page_fields_dock")
        self.page_fields_dock.setAllowedAreas(Qt.AllDockWidgetAreas)
        self.page_fields_panel = PageFieldsPanel()
        self.page_fields_dock.setWidget(self.page_fields_panel)
        self.addDockWidget(Qt.RightDockWidgetArea, self.page_fields_dock)
        self.tabifyDockWidget(self.animations_dock, self.page_fields_dock)

        self.page_timers_dock = QDockWidget("Page Timers", self)
        self.page_timers_dock.setObjectName("page_timers_dock")
        self.page_timers_dock.setAllowedAreas(Qt.AllDockWidgetAreas)
        self.page_timers_panel = PageTimersPanel()
        self.page_timers_dock.setWidget(self.page_timers_panel)
        self.addDockWidget(Qt.RightDockWidgetArea, self.page_timers_dock)
        self.tabifyDockWidget(self.page_fields_dock, self.page_timers_dock)

        # Status bar
        self.statusBar().showMessage("Ready")

        # ── Connect signals ──

        # Widget tree
        self.widget_tree.selection_changed.connect(self._on_tree_selection_changed)
        self.widget_tree.widget_selected.connect(self._on_widget_selected)
        self.widget_tree.tree_changed.connect(self._on_tree_changed)
        self.widget_tree.feedback_message.connect(self._on_widget_tree_feedback_message)

        # Property panel
        self.property_panel.property_changed.connect(self._on_property_changed)
        self.property_panel.resource_imported.connect(self._on_resource_imported)
        self.property_panel.validation_message.connect(self._on_property_validation_message)
        self.property_panel.user_code_requested.connect(self._on_user_code_requested)

        # Preview panel
        self.preview_panel.selection_changed.connect(self._on_preview_selection_changed)
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
        self.project_dock.page_duplicated.connect(self._on_page_duplicated)
        self.project_dock.page_removed.connect(self._on_page_removed)
        self.project_dock.page_renamed.connect(self._on_page_renamed)
        self.project_dock.startup_changed.connect(self._on_startup_changed)
        self.project_dock.page_mode_changed.connect(self._on_page_mode_changed)

        self.page_navigator.page_selected.connect(self._on_page_selected)
        self.page_navigator.page_copy_requested.connect(self._duplicate_page_from_navigator)
        self.page_navigator.page_delete_requested.connect(self._on_page_removed)
        self.page_navigator.page_add_requested.connect(self._on_page_add_from_template)

        # Resource panel
        self.res_panel.resource_selected.connect(self._on_resource_selected)
        self.res_panel.resource_renamed.connect(self._on_resource_renamed)
        self.res_panel.resource_deleted.connect(self._on_resource_deleted)
        self.res_panel.string_key_renamed.connect(self._on_string_key_renamed)
        self.res_panel.string_key_deleted.connect(self._on_string_key_deleted)
        self.diagnostics_panel.diagnostic_activated.connect(self._on_diagnostic_requested)
        self.diagnostics_panel.copy_requested.connect(self._copy_diagnostics_summary)
        self.animations_panel.animations_changed.connect(self._on_widget_animations_changed)
        self.page_fields_panel.fields_changed.connect(self._on_page_fields_changed)
        self.page_fields_panel.validation_message.connect(self._on_property_validation_message)
        self.page_fields_panel.user_code_section_requested.connect(self._on_page_user_code_section_requested)
        self.page_timers_panel.timers_changed.connect(self._on_page_timers_changed)
        self.page_timers_panel.validation_message.connect(self._on_property_validation_message)
        self.page_timers_panel.user_code_requested.connect(self._on_user_code_requested)
        self.res_panel.resource_imported.connect(self._on_resource_imported)
        self.res_panel.feedback_message.connect(self._on_resource_feedback_message)
        self.res_panel.usage_activated.connect(self._on_resource_usage_activated)

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

    def _bump_async_generation(self):
        self._async_generation += 1
        self._pending_compile = False

    def _stop_background_timers(self):
        for timer in (
            self._compile_timer,
            self._embed_timer,
            self._regen_timer,
            self._project_watch_timer,
        ):
            timer.stop()

    @staticmethod
    def _disconnect_worker_signals(worker):
        if worker is None:
            return
        for signal_name in ("finished", "log"):
            signal = getattr(worker, signal_name, None)
            if signal is None:
                continue
            try:
                signal.disconnect()
            except Exception:
                pass

    def _detach_worker(self, worker):
        if worker is None:
            return
        self._disconnect_worker_signals(worker)
        _DETACHED_WORKERS.add(worker)
        try:
            worker.finished.connect(lambda *args, _worker=worker: _release_detached_worker(_worker))
        except Exception:
            _release_detached_worker(worker)

    def _cleanup_worker_ref(self, worker, attr_name):
        if worker is None:
            return
        if getattr(self, attr_name, None) is worker:
            setattr(self, attr_name, None)
        if worker in _DETACHED_WORKERS:
            _release_detached_worker(worker)
            return
        try:
            worker.deleteLater()
        except Exception:
            pass

    def _shutdown_worker(self, worker, attr_name, wait_ms=200):
        if worker is None:
            return
        self._disconnect_worker_signals(worker)
        try:
            worker.requestInterruption()
        except Exception:
            pass
        still_running = False
        try:
            still_running = worker.isRunning()
        except Exception:
            still_running = False
        if still_running:
            try:
                still_running = not worker.wait(wait_ms)
            except Exception:
                still_running = True
        if still_running:
            self._detach_worker(worker)
            if getattr(self, attr_name, None) is worker:
                setattr(self, attr_name, None)
            return
        self._cleanup_worker_ref(worker, attr_name)

    def _shutdown_async_activity(self, wait_ms=200):
        self._stop_background_timers()
        self.preview_panel.stop_rendering()
        self._shutdown_worker(self._compile_worker, "_compile_worker", wait_ms=wait_ms)
        self._shutdown_worker(self._precompile_worker, "_precompile_worker", wait_ms=wait_ms)

    def _describe_sdk_source(self, path=""):
        sdk_root = normalize_path(path or self.project_root)
        if not sdk_root or not is_valid_sdk_root(sdk_root):
            return ""
        return describe_sdk_source(sdk_root)

    def _format_sdk_status_message(self, prefix, path=""):
        sdk_root = normalize_path(path or self.project_root)
        sdk_source = self._describe_sdk_source(sdk_root)
        if sdk_root and sdk_source:
            return f"{prefix}: {sdk_root} [{sdk_source}]"
        if sdk_root:
            return f"{prefix}: {sdk_root}"
        return prefix

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
            self._bump_async_generation()
            self._shutdown_async_activity()
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
        self._update_edit_actions()

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
        self._stop_background_timers()
        self.preview_panel.stop_rendering()
        self._project_watch_snapshot = {}
        self._external_reload_pending = False
        self._active_batch_source = ""
        self._selected_widget = None
        self._selection_state.clear()
        self._current_page = None
        self._clear_page_tabs()
        self.widget_tree.set_project(None)
        self.property_panel.set_selection([])
        self.preview_panel.set_widgets([])
        self.preview_panel.set_selection([])
        self.preview_panel.clear_background_image()
        self.project_dock.set_project(None)
        self.page_navigator.set_pages({})
        self.page_navigator.set_current_page("")
        self.history_panel.clear()
        self.diagnostics_panel.clear()
        self.animations_panel.clear()
        self.page_fields_panel.clear()
        self.page_timers_panel.clear()
        self._update_edit_actions()

    def _open_loaded_project(self, project, project_dir, preferred_sdk_root="", silent=False):
        project_dir = normalize_path(project_dir)
        self._bump_async_generation()
        self._shutdown_async_activity()
        resolved_sdk_root = find_sdk_root(
            cli_sdk_root=preferred_sdk_root or project.sdk_root,
            configured_sdk_root=self._active_sdk_root(),
            project_path=project_dir,
            extra_candidates=[default_sdk_install_dir()],
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
            self.statusBar().showMessage(f"Opened project in editing-only mode: {reason}")
        else:
            self._trigger_compile()
            sdk_source = self._describe_sdk_source(project.sdk_root)
            if sdk_source:
                self.statusBar().showMessage(f"Opened: {project_dir} | SDK: {sdk_source}")
            else:
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
        self.page_nav_dock.hide()
        self.res_dock.hide()
        self.tree_dock.hide()
        self.props_dock.hide()
        self.animations_dock.hide()
        self.history_dock.hide()
        self.diagnostics_dock.hide()
        self.page_fields_dock.hide()
        self.page_timers_dock.hide()

    def _show_editor(self):
        """Show the editor (hide welcome page)."""
        self._central_stack.setCurrentIndex(1)

        # Show dock widgets
        self.project_dock.show()
        self.page_nav_dock.show()
        self.res_dock.show()
        self.tree_dock.show()
        self.props_dock.show()
        self.animations_dock.show()
        self.history_dock.show()
        self.diagnostics_dock.show()
        self.page_fields_dock.show()
        self.page_timers_dock.show()

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

        edit_menu.addSeparator()

        self._copy_action = QAction("Copy", self)
        self._copy_action.setShortcut("Ctrl+C")
        self._copy_action.triggered.connect(self._copy_selection)
        edit_menu.addAction(self._copy_action)

        self._cut_action = QAction("Cut", self)
        self._cut_action.setShortcut("Ctrl+X")
        self._cut_action.triggered.connect(self._cut_selection)
        edit_menu.addAction(self._cut_action)

        self._paste_action = QAction("Paste", self)
        self._paste_action.setShortcut("Ctrl+V")
        self._paste_action.triggered.connect(self._paste_selection)
        edit_menu.addAction(self._paste_action)

        self._duplicate_action = QAction("Duplicate", self)
        self._duplicate_action.setShortcut("Ctrl+D")
        self._duplicate_action.triggered.connect(self._duplicate_selection)
        edit_menu.addAction(self._duplicate_action)

        self._delete_action = QAction("Delete", self)
        self._delete_action.setShortcut("Del")
        self._delete_action.triggered.connect(self._delete_selection)
        edit_menu.addAction(self._delete_action)

        arrange_menu = menubar.addMenu("Arrange")

        self._align_left_action = QAction("Align Left", self)
        self._align_left_action.triggered.connect(lambda: self._align_selection("left"))
        arrange_menu.addAction(self._align_left_action)

        self._align_right_action = QAction("Align Right", self)
        self._align_right_action.triggered.connect(lambda: self._align_selection("right"))
        arrange_menu.addAction(self._align_right_action)

        self._align_top_action = QAction("Align Top", self)
        self._align_top_action.triggered.connect(lambda: self._align_selection("top"))
        arrange_menu.addAction(self._align_top_action)

        self._align_bottom_action = QAction("Align Bottom", self)
        self._align_bottom_action.triggered.connect(lambda: self._align_selection("bottom"))
        arrange_menu.addAction(self._align_bottom_action)

        self._align_hcenter_action = QAction("Align Horizontal Center", self)
        self._align_hcenter_action.triggered.connect(lambda: self._align_selection("hcenter"))
        arrange_menu.addAction(self._align_hcenter_action)

        self._align_vcenter_action = QAction("Align Vertical Center", self)
        self._align_vcenter_action.triggered.connect(lambda: self._align_selection("vcenter"))
        arrange_menu.addAction(self._align_vcenter_action)

        arrange_menu.addSeparator()

        self._distribute_h_action = QAction("Distribute Horizontally", self)
        self._distribute_h_action.triggered.connect(lambda: self._distribute_selection("horizontal"))
        arrange_menu.addAction(self._distribute_h_action)

        self._distribute_v_action = QAction("Distribute Vertically", self)
        self._distribute_v_action.triggered.connect(lambda: self._distribute_selection("vertical"))
        arrange_menu.addAction(self._distribute_v_action)

        arrange_menu.addSeparator()

        self._bring_front_action = QAction("Bring to Front", self)
        self._bring_front_action.triggered.connect(self._move_selection_to_front)
        arrange_menu.addAction(self._bring_front_action)

        self._send_back_action = QAction("Send to Back", self)
        self._send_back_action.triggered.connect(self._move_selection_to_back)
        arrange_menu.addAction(self._send_back_action)

        arrange_menu.addSeparator()

        self._toggle_lock_action = QAction("Toggle Lock", self)
        self._toggle_lock_action.triggered.connect(self._toggle_selection_locked)
        arrange_menu.addAction(self._toggle_lock_action)

        self._toggle_hide_action = QAction("Toggle Hide", self)
        self._toggle_hide_action.triggered.connect(self._toggle_selection_hidden)
        arrange_menu.addAction(self._toggle_hide_action)

        # ── Build menu ──
        build_menu = menubar.addMenu("Build")

        self._compile_action = QAction("Compile && Run", self)
        self._compile_action.setShortcut("F5")
        self._compile_action.triggered.connect(self._do_compile_and_run)
        build_menu.addAction(self._compile_action)

        self.auto_compile_action = QAction("Auto Compile", self)
        self.auto_compile_action.setCheckable(True)
        self.auto_compile_action.setChecked(self.auto_compile)
        self.auto_compile_action.toggled.connect(self._toggle_auto_compile)
        build_menu.addAction(self.auto_compile_action)

        self._stop_action = QAction("Stop Exe", self)
        self._stop_action.triggered.connect(self._stop_exe)
        build_menu.addAction(self._stop_action)

        build_menu.addSeparator()

        gen_res_action = QAction("Generate Resources", self)
        gen_res_action.setToolTip(
            "Run resource generation (app_resource_generate.py) to produce\n"
            "C source files from .eguiproject/resources/ assets and widget config."
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
        view_menu.addAction(self.page_nav_dock.toggleViewAction())
        view_menu.addAction(self.res_dock.toggleViewAction())
        view_menu.addAction(self.tree_dock.toggleViewAction())
        view_menu.addAction(self.props_dock.toggleViewAction())
        view_menu.addAction(self.animations_dock.toggleViewAction())
        view_menu.addAction(self.page_fields_dock.toggleViewAction())
        view_menu.addAction(self.page_timers_dock.toggleViewAction())
        view_menu.addAction(self.history_dock.toggleViewAction())
        view_menu.addAction(self.diagnostics_dock.toggleViewAction())
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

        self._show_grid_action = QAction("Show Grid", self)
        self._show_grid_action.setCheckable(True)
        self._show_grid_action.setChecked(self.preview_panel.show_grid())
        self._show_grid_action.toggled.connect(self._set_show_grid)
        view_menu.addAction(self._show_grid_action)

        grid_menu = view_menu.addMenu("Grid Size")
        self._grid_size_group = QActionGroup(self)
        self._grid_size_group.setExclusive(True)
        self._grid_size_actions = {}
        for size in (0, 4, 8, 12, 16, 24):
            action = QAction("No Snap" if size == 0 else f"{size}px", self)
            action.setCheckable(True)
            if size == self.preview_panel.grid_size():
                action.setChecked(True)
            action.triggered.connect(lambda checked, s=size: self._set_grid_size(s))
            self._grid_size_group.addAction(action)
            self._grid_size_actions[size] = action
            grid_menu.addAction(action)

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
        tb.addAction(self._copy_action)
        tb.addAction(self._paste_action)

        tb.addSeparator()

        # Build actions
        tb.addAction(self._compile_action)
        tb.addAction(self._stop_action)

        self._toolbar = tb
        self._update_compile_availability()
        self._update_edit_actions()

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
            sdk_root = resolve_available_sdk_root(
                item.get("sdk_root", ""),
                cached_sdk_root=default_sdk_install_dir(),
            )
            display_name = item.get("display_name") or os.path.splitext(os.path.basename(project_path))[0]
            project_exists = bool(project_path) and os.path.exists(project_path)
            action_label = display_name if project_exists else f"[Missing] {display_name}"
            action = QAction(action_label, self)
            tooltip = project_path
            if not project_exists:
                tooltip = f"{project_path}\nProject path is missing. Selecting it will offer to remove the stale entry."
            action.setToolTip(tooltip)
            action.triggered.connect(
                lambda checked, p=project_path, r=sdk_root: self._open_recent_project(p, r)
            )
            self._recent_menu.addAction(action)

    def _open_app_dialog(self):
        """Show dialog to select and open an SDK example."""
        dialog = AppSelectorDialog(self, self._active_sdk_root(), on_download_sdk=self._download_sdk)
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
        path = QFileDialog.getExistingDirectory(self, "Select EmbeddedGUI SDK Root", self._active_sdk_root() or "")
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

        self._apply_sdk_root(path, status_message=self._format_sdk_status_message("SDK root set to", path))

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
                f"Automatic setup order: {AUTO_DOWNLOAD_STRATEGY_TEXT}\n\n"
                f"{exc}\n\n"
                "You can try again later, install git for clone fallback, or select an existing SDK root manually.",
            )
            return ""

        progress.setValue(100)
        progress.close()
        self._apply_sdk_root(sdk_root, status_message=self._format_sdk_status_message("SDK downloaded to", sdk_root))
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
            f"Automatic setup order: {AUTO_DOWNLOAD_STRATEGY_TEXT}\n"
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
        dirty_pages = set(self._undo_manager.dirty_pages())
        self.project_dock.set_dirty_pages(dirty_pages)
        self.page_navigator.set_dirty_pages(dirty_pages)
        for i in range(self.page_tab_bar.count()):
            page_name = self._page_tab_name(i)
            self.page_tab_bar.setTabText(i, self._page_tab_label(page_name, dirty_pages))

        title = f"EmbeddedGUI Designer - {self.app_name}"
        if self._project_dir:
            title += f" [{self._project_dir}]"
        if dirty_pages:
            title += " *"
        self.setWindowTitle(title)
        self._update_history_panel()
        self._update_diagnostics_panel()

    def _update_history_panel(self):
        if self._current_page is None:
            self.history_panel.clear()
            return

        stack = self._undo_manager.get_stack(self._current_page.name)
        dirty_source = stack.current_label() if stack.is_dirty() else ""
        self.history_panel.set_history(
            self._current_page.name,
            stack.history_entries(),
            dirty=stack.is_dirty(),
            dirty_source=dirty_source,
            can_undo=stack.can_undo(),
            can_redo=stack.can_redo(),
        )

    def _update_diagnostics_panel(self):
        if not hasattr(self, "diagnostics_panel"):
            return
        if self._current_page is None:
            self.diagnostics_panel.clear()
            return

        resource_dir = self._get_eguiproject_resource_dir()
        catalog = self.project.resource_catalog if self.project is not None else None
        string_catalog = self.project.string_catalog if self.project is not None else None
        entries = analyze_page(
            self._current_page,
            resource_catalog=catalog,
            string_catalog=string_catalog,
            source_resource_dir=resource_dir,
        )
        entries.extend(analyze_project_callback_conflicts(self.project))
        entries.extend(analyze_selection(self._selection_state.widgets))
        self.diagnostics_panel.set_entries(sort_diagnostic_entries(entries))

    def _copy_diagnostics_summary(self):
        if not hasattr(self, "diagnostics_panel") or not self.diagnostics_panel.has_entries():
            self.statusBar().showMessage("No diagnostics to copy.", 3000)
            return

        QApplication.clipboard().setText(self.diagnostics_panel.summary_text())
        self.statusBar().showMessage("Copied diagnostics summary.", 3000)

    def _update_resource_usage_panel(self):
        if not hasattr(self, "res_panel"):
            return
        if self.project is None:
            self.res_panel.set_resource_usage_index({})
            self.res_panel.set_usage_page_context("")
            return
        self.res_panel.set_resource_usage_index(collect_project_resource_usages(self.project))
        current_page_name = self._current_page.name if self._current_page is not None else ""
        self.res_panel.set_usage_page_context(current_page_name)

    def _collect_codegen_blockers(self):
        if self.project is None:
            return []

        resource_dir = self._get_eguiproject_resource_dir()
        catalog = self.project.resource_catalog if self.project is not None else None
        string_catalog = self.project.string_catalog if self.project is not None else None
        entries = []
        for page in self.project.pages:
            entries.extend(
                entry
                for entry in analyze_page(
                    page,
                    resource_catalog=catalog,
                    string_catalog=string_catalog,
                    source_resource_dir=resource_dir,
                )
                if entry.severity == "error"
            )
        entries.extend(
            entry
            for entry in analyze_project_callback_conflicts(self.project)
            if entry.severity == "error"
        )
        return sort_diagnostic_entries(entries)

    def _format_codegen_blocker_summary(self, entries, limit=5):
        entries = list(entries or [])
        lines = []
        for entry in entries[:limit]:
            scope = entry.page_name or "project"
            if entry.widget_name:
                scope = f"{scope}/{entry.widget_name}"
            lines.append(f"- {scope}: {entry.message}")
        remaining = max(0, len(entries) - limit)
        if remaining:
            lines.append(f"- ... and {remaining} more issue(s)")
        return "\n".join(lines)

    def _ensure_codegen_preflight(self, action_name, show_dialog=False, switch_to_python_preview=False):
        blockers = self._collect_codegen_blockers()
        if not blockers:
            return True

        summary = self._format_codegen_blocker_summary(blockers)
        self.debug_panel.log_error(f"{action_name} blocked by diagnostics ({len(blockers)} error(s))")
        if summary:
            self.debug_panel.log_error(summary)
        self.diagnostics_dock.show()
        self.diagnostics_dock.raise_()

        if switch_to_python_preview:
            self._switch_to_python_preview("Compile blocked by diagnostics")

        self.statusBar().showMessage(f"{action_name} blocked: {len(blockers)} error(s) in diagnostics.", 5000)

        if show_dialog:
            QMessageBox.warning(
                self,
                f"{action_name} Blocked",
                f"{action_name} blocked by diagnostics ({len(blockers)} error(s)).\n\n{summary}",
            )
        return False

    def _find_widget_in_page(self, page, widget_name):
        if page is None or not widget_name:
            return None
        for widget in page.get_all_widgets():
            if widget.name == widget_name:
                return widget
        return None

    def _active_sdk_root(self):
        return resolve_available_sdk_root(
            self.project_root,
            self._config.sdk_root,
            self._config.egui_root,
            cached_sdk_root=default_sdk_install_dir(),
        )

    def _nearest_existing_directory(self, path=""):
        candidate = normalize_path(path)
        if not candidate:
            return ""
        while candidate and not os.path.exists(candidate):
            parent = os.path.dirname(candidate)
            if not parent or parent == candidate:
                return ""
            candidate = parent
        if os.path.isfile(candidate):
            candidate = os.path.dirname(candidate)
        return candidate if os.path.isdir(candidate) else ""

    def _default_new_project_parent_dir(self, sdk_root=""):
        sdk_root = normalize_path(sdk_root) or self._active_sdk_root()
        if is_valid_sdk_root(sdk_root):
            return os.path.join(sdk_root, "example")

        if self._project_dir:
            existing_dir = self._nearest_existing_directory(os.path.dirname(self._project_dir))
            if existing_dir:
                return existing_dir

        last_project_path = normalize_path(self._config.last_project_path)
        if last_project_path:
            existing_dir = self._nearest_existing_directory(os.path.dirname(last_project_path))
            if existing_dir:
                return existing_dir

        return normalize_path(os.getcwd())

    def _default_open_project_dir(self):
        if self._project_dir:
            existing_dir = self._nearest_existing_directory(self._project_dir)
            if existing_dir:
                return existing_dir

        last_project_path = normalize_path(self._config.last_project_path)
        if last_project_path:
            existing_dir = self._nearest_existing_directory(last_project_path)
            if existing_dir:
                return existing_dir

        sdk_root = self._active_sdk_root()
        if sdk_root:
            return os.path.join(sdk_root, "example")

        return normalize_path(os.getcwd())

    def _default_save_project_as_dir(self):
        if self._project_dir:
            existing_dir = self._nearest_existing_directory(os.path.dirname(self._project_dir))
            if existing_dir:
                return existing_dir
        return self._default_new_project_parent_dir()

    def _has_directory_conflict(self, path, *, allow_current=False):
        path = normalize_path(path)
        if not path:
            return False
        if allow_current and path == normalize_path(self._project_dir):
            return False
        return os.path.exists(path)

    def _show_directory_conflict(self, path, message):
        QMessageBox.warning(
            self,
            "Directory Conflict",
            f"{message}:\n{normalize_path(path)}",
        )

    def _default_export_code_dir(self):
        if self._project_dir:
            existing_dir = self._nearest_existing_directory(self._project_dir)
            if existing_dir:
                return existing_dir
        return self._default_open_project_dir()

    def _default_mockup_open_dir(self):
        if self._current_page and self._current_page.mockup_image_path and self._project_dir:
            mockup_path = os.path.join(self._project_dir, ".eguiproject", self._current_page.mockup_image_path)
            mockup_dir = normalize_path(os.path.dirname(mockup_path))
            if os.path.isdir(mockup_dir):
                return mockup_dir

        if self._project_dir:
            mockup_dir = os.path.join(self._project_dir, ".eguiproject", "mockup")
            if os.path.isdir(mockup_dir):
                return normalize_path(mockup_dir)
            existing_dir = self._nearest_existing_directory(self._project_dir)
            if existing_dir:
                return existing_dir

        return self._default_open_project_dir()

    def _new_project(self):
        """Create a new project in a dedicated app directory."""
        default_sdk_root = self._active_sdk_root()
        default_parent_dir = self._default_new_project_parent_dir(default_sdk_root)
        dialog = NewProjectDialog(self, sdk_root=default_sdk_root, default_parent_dir=default_parent_dir)
        if dialog.exec_() != QDialog.Accepted:
            return

        sdk_root = normalize_path(dialog.sdk_root)
        project_dir = normalize_path(os.path.join(dialog.parent_dir, dialog.app_name))
        if self._has_directory_conflict(project_dir):
            self._show_directory_conflict(project_dir, "The target directory already exists")
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
            self, "Open Project", self._default_open_project_dir(),
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
        self._bump_async_generation()
        self._shutdown_async_activity()
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

        path = QFileDialog.getExistingDirectory(self, "Save Project To Directory", self._default_save_project_as_dir())
        if not path:
            return

        path = normalize_path(path)
        if self._has_directory_conflict(path, allow_current=True):
            self._show_directory_conflict(path, "The selected directory already exists")
            return

        old_project_dir = self._project_dir
        os.makedirs(path, exist_ok=True)
        self._copy_project_sidecar_files(old_project_dir, path)
        files = self._save_project_files(path)
        self._project_dir = path
        self.project.project_dir = path
        self._bump_async_generation()
        self._shutdown_async_activity()
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

        self._bump_async_generation()
        self._shutdown_async_activity()
        if self.compiler is not None:
            self.compiler.stop_exe()
            self.compiler.cleanup()
            self.compiler = None

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
            self, "Export C Code To Directory", self._default_export_code_dir()
        )
        if not path:
            return
        self._flush_pending_xml()
        self._update_diagnostics_panel()
        if not self._ensure_codegen_preflight("Export", show_dialog=True, switch_to_python_preview=False):
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
            self, "Load Mockup Image", self._default_mockup_open_dir(),
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
        self._set_background_toggle_state(True)

        # Apply to overlay
        self.preview_panel.set_background_image(pixmap)
        self.preview_panel.set_background_image_visible(True)
        self.preview_panel.set_background_image_opacity(self._current_page.mockup_image_opacity)
        self._refresh_project_watch_snapshot()
        self._record_page_state_change(update_preview=False, trigger_compile=False)
        self.statusBar().showMessage(f"Mockup image loaded: {filename}")

    def _toggle_background_image(self, visible):
        """Toggle mockup image visibility."""
        if self._current_page:
            if self._current_page.mockup_image_visible == visible:
                self.preview_panel.set_background_image_visible(visible)
                return
            self._current_page.mockup_image_visible = visible
        self.preview_panel.set_background_image_visible(visible)
        self._record_page_state_change(update_preview=False, trigger_compile=False, source="mockup visibility")

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
        self._set_background_toggle_state(True)
        self._refresh_project_watch_snapshot()
        self._record_page_state_change(update_preview=False, trigger_compile=False)
        self.statusBar().showMessage("Mockup image cleared")

    def _set_background_opacity(self, opacity):
        """Set mockup image opacity."""
        if self._current_page:
            if self._current_page.mockup_image_opacity == opacity:
                self.preview_panel.set_background_image_opacity(opacity)
                return
            self._current_page.mockup_image_opacity = opacity
        self.preview_panel.set_background_image_opacity(opacity)
        self._record_page_state_change(update_preview=False, trigger_compile=False, source="mockup opacity")

    def _apply_page_mockup(self):
        """Load and apply the current page's mockup image."""
        if not self._current_page:
            self.preview_panel.clear_background_image()
            self._set_background_toggle_state(True)
            return

        path = self._current_page.mockup_image_path
        self._set_background_toggle_state(self._current_page.mockup_image_visible)
        self._sync_background_opacity_actions(self._current_page.mockup_image_opacity)
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
                    return
        self.preview_panel.clear_background_image()

    def _set_background_toggle_state(self, visible):
        blocker = QSignalBlocker(self._toggle_bg_action)
        self._toggle_bg_action.setChecked(visible)
        del blocker

    def _sync_background_opacity_actions(self, opacity):
        target_pct = int(opacity * 100)
        for act in self._opacity_group.actions():
            act.setChecked(act.text() == f"{target_pct}%")

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
        self._refresh_page_navigator()

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
        self._update_resource_usage_panel()

        # Refresh i18n string catalog
        string_catalog = self.project.string_catalog if self.project else None
        self.res_panel.set_string_catalog(string_catalog)
        # Feed string keys to property panel for @string/ completions
        if string_catalog:
            self.property_panel.set_string_keys(string_catalog.all_keys)
        else:
            self.property_panel.set_string_keys([])

        self._update_compile_availability()
        self._update_edit_actions()
        if self.compiler is not None:
            self._start_precompile()

    def _start_precompile(self):
        """Start background precompile if exe doesn't exist."""
        if not self.project:
            return
        if self._is_closing:
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
            generation = self._async_generation
            worker = self.compiler.precompile_async(
                callback=lambda success, message: self._on_precompile_done(worker, generation, success, message)
            )
            self._precompile_worker = worker

    def _on_precompile_done(self, worker, generation, success, message):
        """Callback when background precompile finishes."""
        self._cleanup_worker_ref(worker, "_precompile_worker")
        if self._is_closing or generation != self._async_generation:
            return
        if success:
            self.statusBar().showMessage("Ready (precompiled)", 3000)
            self.debug_panel.log_success("Background precompile completed")
        else:
            self.statusBar().showMessage("Precompile failed", 5000)
            self.debug_panel.log_error("Background precompile failed")
            self.debug_panel.log_compile_output(False, message)
            self.debug_dock.show()

    def _refresh_page_navigator(self):
        if not self.project:
            self.page_navigator.set_pages({})
            self.page_navigator.set_current_page("")
            return

        self.page_navigator.set_screen_size(self.project.screen_width, self.project.screen_height)
        self.page_navigator.set_pages({page.name: page for page in self.project.pages})

        current_name = ""
        if self._current_page and self.project.get_page_by_name(self._current_page.name):
            current_name = self._current_page.name
        self.page_navigator.set_current_page(current_name)

    def _make_unique_page_name(self, base_name):
        candidate = (base_name or "page").strip().replace(" ", "_").replace(".xml", "")
        if not candidate:
            candidate = "page"
        if not self.project:
            return candidate

        existing = {page.name for page in self.project.pages}
        if candidate not in existing:
            return candidate

        match = re.match(r"^(.*?)(?:_(\d+))?$", candidate)
        stem = candidate
        suffix = 2
        if match:
            stem = match.group(1) or candidate
            if match.group(2):
                suffix = int(match.group(2)) + 1

        while f"{stem}_{suffix}" in existing:
            suffix += 1
        return f"{stem}_{suffix}"

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
        target = self._primary_selected_widget()
        if target is None:
            return
        if not assign_resource_to_widget(target, res_type, filename):
            return
        self.property_panel.set_selection(self._selected_widgets(), self._primary_selected_widget())
        self._update_resource_usage_panel()
        self._on_model_changed(source=f"{res_type} resource assignment")

    def _on_resource_renamed(self, res_type, old_name, new_name):
        """Update widget references after a resource file was renamed."""
        touched_pages = self._rewrite_resource_references(res_type, old_name, new_name)
        self._finalize_resource_reference_change(touched_pages, source=f"{res_type} resource rename")

    def _on_resource_deleted(self, res_type, filename):
        """Clear widget references after a resource file was deleted."""
        touched_pages = self._rewrite_resource_references(res_type, filename, "")
        self._finalize_resource_reference_change(touched_pages, source=f"{res_type} resource delete")

    def _on_string_key_deleted(self, key, replacement_text):
        """Rewrite widget text references after a string key was deleted."""
        touched_pages, _ = rewrite_project_string_references(
            self.project,
            key,
            replacement_text=replacement_text,
        )
        self._finalize_resource_reference_change(touched_pages, source="string key delete")

    def _on_string_key_renamed(self, old_key, new_key):
        """Rewrite widget text references after a string key was renamed."""
        touched_pages, _ = rewrite_project_string_references(
            self.project,
            old_key,
            new_key=new_key,
        )
        self._finalize_resource_reference_change(touched_pages, source="string key rename")

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
        self._update_resource_usage_panel()
        self._update_diagnostics_panel()
        self._resources_need_regen = True
        # Auto-trigger resource generation with debounce
        self._refresh_project_watch_snapshot()
        self._regen_timer.start()
        current_message = self.statusBar().currentMessage()
        if not current_message.startswith("Updated resources in "):
            self.statusBar().showMessage("Resources changed, will regenerate...")

    def _on_resource_feedback_message(self, message):
        if message:
            self.statusBar().showMessage(message, 5000)

    def _on_resource_usage_activated(self, page_name, widget_name):
        if not self.project or not page_name or not widget_name:
            return
        if self._current_page is None or self._current_page.name != page_name:
            self._switch_page(page_name)
        target_page = self.project.get_page_by_name(page_name)
        target_widget = self._find_widget_in_page(target_page, widget_name)
        if target_widget is not None:
            self._set_selection([target_widget], primary=target_widget, sync_tree=True, sync_preview=True)
            self.statusBar().showMessage(f"Focused resource usage: {page_name}/{widget_name}.", 4000)

    def _rewrite_resource_references(self, res_type, old_name, new_name):
        """Rewrite matching resource filename references across all project pages."""
        touched_pages, _ = rewrite_project_resource_references(self.project, res_type, old_name, new_name)
        return touched_pages

    def _finalize_resource_reference_change(self, touched_pages, source="resource reference update"):
        """Record dirty state and refresh current-page UI after resource ref changes."""
        if not touched_pages:
            return

        current_page_changed = False
        for page in touched_pages:
            stack = self._undo_manager.get_stack(page.name)
            stack.push(page.to_xml_string(), label=source or "resource reference update")
            self.page_navigator.refresh_thumbnail(page.name)
            if page is self._current_page:
                current_page_changed = True

        if current_page_changed:
            self.widget_tree.rebuild_tree()
            self.widget_tree.set_selected_widgets(self._selection_state.widgets, self._selection_state.primary)
            if self._selection_state.primary is not None:
                try:
                    self.property_panel.set_selection(self._selection_state.widgets, self._selection_state.primary)
                except RuntimeError:
                    pass
            self._update_preview_overlay()
            self._sync_xml_to_editors()

        self._update_resource_usage_panel()
        self._update_diagnostics_panel()
        self._update_undo_actions()
        self._update_window_title()
        if source:
            page_count = len(touched_pages)
            noun = "page" if page_count == 1 else "pages"
            self.statusBar().showMessage(f"Updated resources in {page_count} {noun}: {source}.", 4000)

    def _on_resource_dropped(self, widget, res_type, filename):
        """Resource was dropped onto a widget in the preview overlay."""
        self._set_selection([widget], primary=widget, sync_tree=True, sync_preview=False)
        if not assign_resource_to_widget(widget, res_type, filename):
            return
        self.property_panel.set_selection(self._selection_state.widgets, self._selection_state.primary)
        self._update_resource_usage_panel()
        self._on_model_changed(source=f"{res_type} resource drop")

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
        self.res_panel.set_usage_page_context(page_name)
        self._clear_selection(sync_tree=True, sync_preview=True)
        self.project_dock.set_current_page(page_name)
        self.page_navigator.set_current_page(page_name)

        # Initialize undo stack for this page if empty
        stack = self._undo_manager.get_stack(page_name)
        if not stack._history:
            stack.push(page.to_xml_string(), label="Loaded page")
            if not page.dirty:
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
        self.page_fields_panel.set_page(page)
        self.page_timers_panel.set_page(page)

        # Update preview & XML
        self._update_preview_overlay()
        self._sync_xml_to_editors()

        # Load mockup image for this page
        self._apply_page_mockup()

        # Trigger compile to show current page in preview
        self._trigger_compile()
        self._update_undo_actions()
        self._update_edit_actions()
        self._update_window_title()

    def _on_page_selected(self, page_name):
        """User clicked a page in the Project Explorer."""
        self._switch_page(page_name)

    def _on_widget_animations_changed(self, animations):
        widget = self._primary_selected_widget()
        if widget is None:
            return
        widget.animations = list(animations or [])
        self._record_page_state_change(update_preview=False, trigger_compile=True, source="widget animations edit")

    def _on_page_fields_changed(self, fields):
        if self._current_page is None:
            return
        self._current_page.user_fields = list(fields or [])
        self._record_page_state_change(update_preview=False, trigger_compile=True, source="page fields edit")

    def _on_page_timers_changed(self, timers):
        if self._current_page is None:
            return
        self._current_page.timers = list(timers or [])
        self._record_page_state_change(update_preview=False, trigger_compile=True, source="page timers edit")

    def _page_user_source_path(self, page):
        if page is None or not self._project_dir:
            return ""
        return os.path.join(self._project_dir, f"{page.name}.c")

    def _generate_page_user_source_content(self, page):
        content = generate_page_user_source(page, self.project)
        return embed_source_hash(content, compute_source_hash(content))

    def _insert_callback_stub_into_user_source(self, content, page, callback_name, signature):
        if not content or not callback_name or _callback_definition_exists(content, callback_name):
            return content, False

        target = _resolve_page_callback_target(page, callback_name, signature)
        stub = render_page_callback_stub(
            page,
            target.get("name", ""),
            target.get("signature", ""),
            kind=target.get("kind", "view"),
        )
        if not stub:
            return content, False

        begin_marker = "// USER CODE BEGIN callbacks"
        end_marker = "// USER CODE END callbacks"
        begin_index = content.find(begin_marker)
        end_index = content.find(end_marker)
        if begin_index < 0 or end_index < begin_index:
            return content, False

        body_start = content.find("\n", begin_index)
        if body_start < 0 or body_start >= end_index:
            return content, False

        callback_body = content[body_start + 1:end_index]
        if callback_body.strip():
            new_body = callback_body.rstrip() + "\n\n" + stub + "\n"
        else:
            new_body = stub + "\n"
        updated = content[:body_start + 1] + new_body + content[end_index:]
        return updated, True

    def _ensure_page_user_source_ready(self, page, callback_name="", signature=""):
        filepath = self._page_user_source_path(page)
        if not filepath:
            return "", False, False

        existing_content = read_existing_file(filepath)
        if existing_content is None:
            content = self._generate_page_user_source_content(page)
        else:
            content = existing_content

        inserted = False
        if callback_name:
            content, inserted = self._insert_callback_stub_into_user_source(content, page, callback_name, signature)

        should_write = existing_content is None or content != existing_content
        if should_write:
            os.makedirs(os.path.dirname(filepath), exist_ok=True)
            with open(filepath, "w", encoding="utf-8") as f:
                f.write(content)

        has_callback = not callback_name or _callback_definition_exists(content, callback_name)
        return filepath, should_write or inserted, has_callback

    def _open_path_in_default_app(self, path):
        if not path or not os.path.exists(path):
            return False

        import subprocess
        import sys

        try:
            if os.name == "nt":
                os.startfile(os.path.normpath(path))
            elif sys.platform == "darwin":
                subprocess.Popen(["open", path])
            else:
                subprocess.Popen(["xdg-open", path])
            return True
        except Exception as exc:
            self.debug_panel.log_error(f"Failed to open user source: {exc}")
            return False

    def _open_page_user_source(self, callback_name="", signature="", section_name=""):
        if self.project is None or self._current_page is None:
            self.statusBar().showMessage("No active page available for user code.", 5000)
            return
        if not self._project_dir:
            self.statusBar().showMessage("Save the project first to create user source files.", 5000)
            return

        self._flush_pending_xml()

        filepath, updated, has_callback = self._ensure_page_user_source_ready(
            self._current_page,
            callback_name or "",
            signature or "",
        )
        if not filepath:
            self.statusBar().showMessage("Unable to resolve the page user source file.", 5000)
            return
        if updated:
            self._refresh_project_watch_snapshot()

        if not self._open_path_in_default_app(filepath):
            QMessageBox.warning(self, "Open User Code", f"Failed to open:\n{filepath}")
            return

        if callback_name and has_callback:
            self.statusBar().showMessage(
                f"Opened user code: {self._current_page.name}.c ({callback_name}).",
                5000,
            )
            return
        if callback_name:
            self.statusBar().showMessage(
                f"Opened user code: {self._current_page.name}.c. Add '{callback_name}' manually if needed.",
                5000,
            )
            return
        if section_name:
            self.statusBar().showMessage(
                f"Opened user code: {self._current_page.name}.c ({section_name}).",
                5000,
            )
            return
        self.statusBar().showMessage(f"Opened user code: {self._current_page.name}.c.", 5000)

    def _on_user_code_requested(self, callback_name, signature):
        self._open_page_user_source(callback_name=callback_name, signature=signature)

    def _on_page_user_code_section_requested(self, section_name):
        self._open_page_user_source(section_name=section_name or "")

    def _on_diagnostic_requested(self, page_name, widget_name):
        if not self.project:
            return

        diagnostic_entry = self.diagnostics_panel.current_activated_entry() if hasattr(self, "diagnostics_panel") else None

        target_page_name = page_name or (self._current_page.name if self._current_page is not None else "")
        if not target_page_name:
            return

        page = self.project.get_page_by_name(target_page_name)
        if page is None:
            return

        if self._current_page is None or self._current_page.name != target_page_name:
            self._switch_page(target_page_name)
            page = self._current_page

        if not widget_name:
            return

        widget = self._find_widget_in_page(page, widget_name)
        if widget is None:
            self.statusBar().showMessage(f"Diagnostic target not found: {target_page_name}/{widget_name}", 4000)
            return

        self._set_selection([widget], primary=widget, sync_tree=True, sync_preview=True)
        if diagnostic_entry is not None and getattr(diagnostic_entry, "resource_type", "") and getattr(diagnostic_entry, "resource_name", ""):
            self.res_dock.show()
            self.res_panel._select_resource_item(diagnostic_entry.resource_type, diagnostic_entry.resource_name)
            self.statusBar().showMessage(
                f"Opened diagnostic resource check: {diagnostic_entry.resource_type}/{diagnostic_entry.resource_name}.",
                4000,
            )

    def _on_page_added(self, page_name):
        """User requested a new page."""
        if not self.project:
            return
        self.project.create_new_page(page_name)
        self.project_dock.set_project(self.project)
        self._refresh_page_navigator()
        self._ensure_page_tab(page_name)
        self._switch_page(page_name)
        self._trigger_compile()

    def _on_page_duplicated(self, source_name, page_name):
        """User requested duplicating an existing page."""
        if not self.project:
            return
        self.project.duplicate_page(source_name, page_name)
        self.project_dock.set_project(self.project)
        self._refresh_page_navigator()
        self._ensure_page_tab(page_name)
        self._switch_page(page_name)
        self._trigger_compile()

    def _on_page_removed(self, page_name):
        """User deleted a page."""
        if not self.project:
            return
        page = self.project.get_page_by_name(page_name)
        if page:
            was_current = self._current_page is not None and self._current_page.name == page_name
            self.project.remove_page(page)
            self._undo_manager.remove_stack(page_name)
            self.project_dock.set_project(self.project)
            self._refresh_page_navigator()
            self._remove_page_tab(page_name)
            # Delete generated files for the removed page so they are not
            # picked up by EGUI_CODE_SRC on the next build.
            if self._project_dir:
                delete_page_generated_files(self._project_dir, page_name)
            if was_current and self.project.pages:
                self._switch_page(self.project.pages[0].name)
            elif not self.project.pages:
                self._current_page = None
                self._clear_selection(sync_tree=True, sync_preview=True)
                self.widget_tree.set_project(None)
                self.preview_panel.set_widgets([])
                self.preview_panel.set_selection([])
                self.page_navigator.set_current_page("")
            elif self._current_page:
                self.project_dock.set_current_page(self._current_page.name)
                self.page_navigator.set_current_page(self._current_page.name)
                self._update_preview_overlay()
            self._trigger_compile()
            self._update_window_title()
            self._update_edit_actions()

    def _on_page_renamed(self, old_name, new_name):
        """User renamed a page."""
        if not self.project:
            return
        page = self.project.get_page_by_name(old_name)
        if page:
            was_current = self._current_page is not None and self._current_page.name == old_name
            page.file_path = f"layout/{new_name}.xml"
            # Update startup_page reference if needed
            if self.project.startup_page == old_name:
                self.project.startup_page = new_name
            self._undo_manager.rename_stack(old_name, new_name)
            self.project_dock.set_project(self.project)
            self._refresh_page_navigator()
            self._rename_page_tab(old_name, new_name)
            if was_current:
                self._switch_page(new_name)
            elif self._current_page:
                self.project_dock.set_current_page(self._current_page.name)
                self.page_navigator.set_current_page(self._current_page.name)
                self._trigger_compile()
                self._update_edit_actions()

    def _duplicate_page_from_navigator(self, page_name):
        if not self.project or not page_name:
            return
        self._on_page_duplicated(page_name, self._make_unique_page_name(f"{page_name}_copy"))

    def _on_page_add_from_template(self, template_key, anchor_page_name):
        del anchor_page_name
        if not self.project or template_key not in PAGE_TEMPLATES:
            return

        page_name = self._make_unique_page_name(f"{template_key}_page")
        template = PAGE_TEMPLATES[template_key]
        page = self.project.create_new_page(page_name)
        page.dirty = True

        existing_names = {widget.name for widget in page.get_all_widgets() if widget.name}
        root_widget = page.root_widget
        if root_widget is not None:
            for spec in template.get("widgets", []):
                widget = WidgetModel(
                    spec.get("type", "label"),
                    name=spec.get("name"),
                    x=spec.get("x", 0),
                    y=spec.get("y", 0),
                    width=spec.get("w", 100),
                    height=spec.get("h", 40),
                )
                widget.name = self._make_unique_widget_name(widget.name, existing_names=existing_names)
                if "text" in spec and "text" in widget.properties:
                    widget.properties["text"] = spec["text"]
                root_widget.add_child(widget)

        self.project_dock.set_project(self.project)
        self._refresh_page_navigator()
        self._ensure_page_tab(page_name)
        self._switch_page(page_name)

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

    def _page_tab_name(self, index):
        item = self.page_tab_bar.tabItem(index)
        if item is not None and hasattr(item, "routeKey"):
            return item.routeKey()
        return self.page_tab_bar.tabText(index).rstrip("*")

    def _page_tab_label(self, page_name, dirty_pages=None):
        if dirty_pages is None:
            dirty_pages = set(self._undo_manager.dirty_pages())
        else:
            dirty_pages = set(dirty_pages)
        return f"{page_name}*" if page_name in dirty_pages else page_name

    def _ensure_page_tab(self, page_name):
        """Add a tab for page_name if not already present. Returns the index."""
        for i in range(self.page_tab_bar.count()):
            if self._page_tab_name(i) == page_name:
                return i
        # routeKey = page_name (unique per page)
        self.page_tab_bar.addTab(page_name, self._page_tab_label(page_name), None)
        return self.page_tab_bar.count() - 1

    def _remove_page_tab(self, page_name):
        for i in range(self.page_tab_bar.count()):
            if self._page_tab_name(i) == page_name:
                self.page_tab_bar.removeTab(i)
                return

    def _rename_page_tab(self, old_name, new_name):
        for i in range(self.page_tab_bar.count()):
            if self._page_tab_name(i) == old_name:
                item = self.page_tab_bar.tabItem(i)
                if item is not None and hasattr(item, "setRouteKey"):
                    item.setRouteKey(new_name)
                self.page_tab_bar.setTabText(i, self._page_tab_label(new_name))
                return

    def _on_page_tab_changed(self, index):
        if self._syncing_tabs:
            return
        if index < 0:
            return
        page_name = self._page_tab_name(index)
        if self._current_page and page_name == self._current_page.name:
            return
        self._switch_page(page_name)

    def _on_page_tab_closed(self, index):
        if index < 0:
            return
        page_name = self._page_tab_name(index)
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
            keep_name = self._page_tab_name(index)
            names_to_remove = []
            for i in range(self.page_tab_bar.count()):
                n = self._page_tab_name(i)
                if n != keep_name:
                    names_to_remove.append(n)
            for n in names_to_remove:
                self._remove_page_tab(n)
        elif action == close_all:
            self._clear_page_tabs()

    # ── Widget selection / editing ─────────────────────────────────

    def _set_selection(self, widgets=None, primary=None, sync_tree=True, sync_preview=True):
        self._selection_state.set_widgets(widgets or [], primary=primary)
        self._selected_widget = self._selection_state.primary
        self.property_panel.set_selection(self._selection_state.widgets, self._selection_state.primary)
        self.animations_panel.set_selection(self._selection_state.widgets, self._selection_state.primary)
        if sync_tree:
            self.widget_tree.set_selected_widgets(self._selection_state.widgets, self._selection_state.primary)
        if sync_preview:
            self.preview_panel.set_selection(self._selection_state.widgets, self._selection_state.primary)
        self._update_edit_actions()
        self._update_diagnostics_panel()
        self._show_selection_feedback()

    def _clear_selection(self, sync_tree=True, sync_preview=True):
        self._set_selection([], primary=None, sync_tree=sync_tree, sync_preview=sync_preview)

    def _selected_widgets(self):
        widgets = self._selection_state.widgets
        if widgets:
            return widgets
        if self._selected_widget is not None:
            return [self._selected_widget]
        return []

    def _primary_selected_widget(self):
        return self._selection_state.primary or self._selected_widget

    def _selection_feedback_message(self):
        widgets = [widget for widget in self._selection_state.widgets if widget is not None]
        if not widgets:
            return ""

        if len(widgets) == 1:
            widget = widgets[0]
            parts = []
            if getattr(widget, "designer_locked", False):
                parts.append("locked")
            if getattr(widget, "designer_hidden", False):
                parts.append("hidden")
            if self._parent_uses_layout(widget.parent):
                parts.append(f"layout-managed by {widget.parent.widget_type}")
            if not parts:
                return ""
            return f"Selection note: {widget.name} is " + ", ".join(parts) + "."

        issues = []
        locked_count = sum(1 for widget in widgets if getattr(widget, "designer_locked", False))
        hidden_count = sum(1 for widget in widgets if getattr(widget, "designer_hidden", False))
        layout_count = sum(1 for widget in widgets if self._parent_uses_layout(widget.parent))
        if locked_count:
            noun = "widget" if locked_count == 1 else "widgets"
            issues.append(f"{locked_count} locked {noun}")
        if hidden_count:
            noun = "widget" if hidden_count == 1 else "widgets"
            issues.append(f"{hidden_count} hidden {noun}")
        if layout_count:
            noun = "widget" if layout_count == 1 else "widgets"
            issues.append(f"{layout_count} layout-managed {noun}")
        if not issues:
            return ""
        return "Selection note: current selection includes " + ", ".join(issues) + "."

    def _show_selection_feedback(self):
        message = self._selection_feedback_message()
        if message:
            self.statusBar().showMessage(message, 5000)

    def _existing_widget_names(self, existing_names=None):
        if existing_names is not None:
            return existing_names
        if not self._current_page:
            return set()
        return {widget.name for widget in self._current_page.get_all_widgets() if widget.name}

    def _make_unique_widget_name(self, base_name, existing_names=None):
        candidate = (base_name or "widget").strip().replace(" ", "_")
        if not candidate:
            candidate = "widget"

        existing_names = self._existing_widget_names(existing_names)
        if candidate not in existing_names:
            existing_names.add(candidate)
            return candidate

        match = re.match(r"^(.*?)(?:_(\d+))?$", candidate)
        stem = candidate
        suffix = 2
        if match:
            stem = match.group(1) or candidate
            if match.group(2):
                suffix = int(match.group(2)) + 1

        while f"{stem}_{suffix}" in existing_names:
            suffix += 1

        resolved = f"{stem}_{suffix}"
        existing_names.add(resolved)
        return resolved

    def _rename_widget_subtree_uniquely(self, widget, existing_names):
        if widget is None:
            return
        widget.name = self._make_unique_widget_name(widget.name or widget.widget_type, existing_names=existing_names)
        for child in widget.children:
            self._rename_widget_subtree_uniquely(child, existing_names)

    def _top_level_selected_widgets(self, widgets=None, exclude_root=True):
        widgets = [widget for widget in (widgets or self._selected_widgets()) if widget is not None]
        if not widgets:
            return []

        selected_ids = {id(widget) for widget in widgets}
        result = []
        root_widget = self._current_page.root_widget if self._current_page else None

        for widget in widgets:
            if exclude_root and widget is root_widget:
                continue
            parent = widget.parent
            skip = False
            while parent is not None:
                if id(parent) in selected_ids:
                    skip = True
                    break
                parent = parent.parent
            if not skip:
                result.append(widget)
        return result

    def _parent_uses_layout(self, parent):
        if parent is None:
            return False
        return WidgetModel._get_type_info(parent.widget_type).get("layout_func") is not None

    def _shared_selection_parent(self, widgets=None):
        widgets = [widget for widget in (widgets or []) if widget is not None]
        if not widgets:
            return None
        parent = widgets[0].parent
        if parent is None:
            return None
        for widget in widgets[1:]:
            if widget.parent is not parent:
                return None
        if self._parent_uses_layout(parent):
            return None
        return parent

    def _default_paste_parent(self):
        if not self._current_page:
            return None

        primary = self._primary_selected_widget()
        if primary is not None:
            if primary.is_container:
                return primary
            if primary.parent is not None and primary.parent.is_container:
                return primary.parent

        root_widget = self._current_page.root_widget
        if root_widget is not None and root_widget.is_container:
            return root_widget
        return None

    def _selected_widget_payload(self):
        widgets = self._top_level_selected_widgets()
        if not widgets:
            return None
        return {
            "widgets": [copy.deepcopy(widget.to_dict()) for widget in widgets],
        }

    def _locked_widget_summary(self, count):
        noun = "widget" if count == 1 else "widgets"
        return f"{count} locked {noun}"

    def _show_selection_action_blocked(self, action, reason):
        self.statusBar().showMessage(f"Cannot {action}: {reason}.", 4000)

    def _deletable_selected_widgets(self):
        widgets = self._top_level_selected_widgets()
        deletable = [widget for widget in widgets if not getattr(widget, "designer_locked", False)]
        locked_count = len(widgets) - len(deletable)
        return deletable, locked_count

    def _paste_widget_payload(self, payload):
        if not self._current_page or not payload:
            return []

        parent = self._default_paste_parent()
        if parent is None:
            return []

        widgets_data = payload.get("widgets", [])
        if not widgets_data:
            return []

        self._paste_serial += 1
        offset = self.preview_panel.grid_size() or 12
        offset *= self._paste_serial
        existing_names = self._existing_widget_names()
        use_offset = not self._parent_uses_layout(parent)

        pasted_widgets = []
        for widget_data in widgets_data:
            widget = WidgetModel.from_dict(copy.deepcopy(widget_data))
            self._rename_widget_subtree_uniquely(widget, existing_names)
            if use_offset:
                widget.x += offset
                widget.y += offset
                widget.display_x = widget.x
                widget.display_y = widget.y
            parent.add_child(widget)
            pasted_widgets.append(widget)

        self.widget_tree.rebuild_tree()
        self._set_selection(pasted_widgets, primary=pasted_widgets[-1], sync_tree=True, sync_preview=True)
        self._record_page_state_change(source="clipboard paste")
        return pasted_widgets

    def _copy_selection(self):
        payload = self._selected_widget_payload()
        if not payload:
            return
        self._clipboard_payload = payload
        self._paste_serial = 0
        self._update_edit_actions()
        self.statusBar().showMessage(f"Copied {len(payload['widgets'])} widget(s)", 3000)

    def _cut_selection(self):
        deletable_widgets, locked_count = self._deletable_selected_widgets()
        if not deletable_widgets:
            if locked_count:
                self.statusBar().showMessage(f"Cannot cut selection: {self._locked_widget_summary(locked_count)}.", 4000)
            return
        payload = {
            "widgets": [copy.deepcopy(widget.to_dict()) for widget in deletable_widgets],
        }
        if not payload:
            return
        self._clipboard_payload = payload
        self._paste_serial = 0
        deleted_count, skipped_locked = self._delete_selection()
        if deleted_count:
            message = f"Cut {deleted_count} widget(s)"
            if skipped_locked:
                message += f"; skipped {self._locked_widget_summary(skipped_locked)}"
            self.statusBar().showMessage(message, 3000)

    def _paste_selection(self):
        pasted_widgets = self._paste_widget_payload(self._clipboard_payload)
        if pasted_widgets:
            self.statusBar().showMessage(f"Pasted {len(pasted_widgets)} widget(s)", 3000)

    def _duplicate_selection(self):
        payload = self._selected_widget_payload()
        if not payload:
            return
        duplicated_widgets = self._paste_widget_payload(payload)
        if duplicated_widgets:
            self.statusBar().showMessage(f"Duplicated {len(duplicated_widgets)} widget(s)", 3000)

    def _delete_selection(self):
        widgets, locked_count = self._deletable_selected_widgets()
        if not widgets:
            if locked_count:
                self.statusBar().showMessage(f"Cannot delete selection: {self._locked_widget_summary(locked_count)}.", 4000)
            return 0, locked_count

        for widget in widgets:
            if widget.parent is not None:
                widget.parent.remove_child(widget)

        self.widget_tree.rebuild_tree()
        self._clear_selection(sync_tree=True, sync_preview=True)
        self._record_page_state_change(source="widget delete")
        message = f"Deleted {len(widgets)} widget(s)"
        if locked_count:
            message += f"; skipped {self._locked_widget_summary(locked_count)}"
        self.statusBar().showMessage(message, 3000)
        return len(widgets), locked_count

    def _align_selection(self, mode):
        selected_widgets = self._top_level_selected_widgets()
        widgets = [widget for widget in selected_widgets if not getattr(widget, "designer_locked", False)]
        if len(widgets) < 2:
            if len(selected_widgets) >= 2:
                self._show_selection_action_blocked("align selection", "locked widgets leave fewer than 2 editable widgets")
            return
        if self._shared_selection_parent(widgets) is None:
            self._show_selection_action_blocked("align selection", "selected widgets do not share the same free-position parent")
            return

        primary = self._primary_selected_widget()
        if primary not in widgets:
            primary = widgets[-1]

        for widget in widgets:
            if widget is primary:
                continue
            if mode == "left":
                widget.x = primary.x
            elif mode == "right":
                widget.x = primary.x + primary.width - widget.width
            elif mode == "top":
                widget.y = primary.y
            elif mode == "bottom":
                widget.y = primary.y + primary.height - widget.height
            elif mode == "hcenter":
                widget.x = primary.x + (primary.width - widget.width) // 2
            elif mode == "vcenter":
                widget.y = primary.y + (primary.height - widget.height) // 2
            widget.display_x = widget.x
            widget.display_y = widget.y

        self._record_page_state_change(source=f"align {mode}")

    def _distribute_selection(self, axis):
        selected_widgets = self._top_level_selected_widgets()
        widgets = [widget for widget in selected_widgets if not getattr(widget, "designer_locked", False)]
        if len(widgets) < 3:
            if len(selected_widgets) >= 3:
                self._show_selection_action_blocked("distribute selection", "locked widgets leave fewer than 3 editable widgets")
            return
        if self._shared_selection_parent(widgets) is None:
            self._show_selection_action_blocked("distribute selection", "selected widgets do not share the same free-position parent")
            return

        key_name = "x" if axis == "horizontal" else "y"
        size_name = "width" if axis == "horizontal" else "height"
        widgets = sorted(widgets, key=lambda widget: getattr(widget, key_name))
        first = widgets[0]
        last = widgets[-1]
        inner_widgets = widgets[1:-1]
        if not inner_widgets:
            return

        start_edge = getattr(first, key_name) + getattr(first, size_name)
        end_edge = getattr(last, key_name)
        available = end_edge - start_edge
        used = sum(getattr(widget, size_name) for widget in inner_widgets)
        gap_count = len(widgets) - 1
        if gap_count <= 0:
            return
        gap = (available - used) // gap_count if available > used else 0

        cursor = start_edge + gap
        for widget in inner_widgets:
            setattr(widget, key_name, cursor)
            widget.display_x = widget.x
            widget.display_y = widget.y
            cursor += getattr(widget, size_name) + gap

        self._record_page_state_change(source=f"distribute {axis}")

    def _move_selection_to_front(self):
        selected_widgets = self._top_level_selected_widgets()
        widgets = [widget for widget in selected_widgets if not getattr(widget, "designer_locked", False)]
        if not widgets:
            if selected_widgets:
                self._show_selection_action_blocked("bring to front", "all selected widgets are locked")
            return
        grouped = {}
        for widget in widgets:
            if widget.parent is None:
                continue
            grouped.setdefault(id(widget.parent), (widget.parent, []) )[1].append(widget)

        for parent, children in grouped.values():
            ordered = [child for child in parent.children if child in children]
            remaining = [child for child in parent.children if child not in children]
            parent.children = remaining + ordered

        self.widget_tree.rebuild_tree()
        self._set_selection(widgets, primary=self._primary_selected_widget(), sync_tree=True, sync_preview=True)
        self._record_page_state_change(source="bring to front")

    def _move_selection_to_back(self):
        selected_widgets = self._top_level_selected_widgets()
        widgets = [widget for widget in selected_widgets if not getattr(widget, "designer_locked", False)]
        if not widgets:
            if selected_widgets:
                self._show_selection_action_blocked("send to back", "all selected widgets are locked")
            return
        grouped = {}
        for widget in widgets:
            if widget.parent is None:
                continue
            grouped.setdefault(id(widget.parent), (widget.parent, []) )[1].append(widget)

        for parent, children in grouped.values():
            ordered = [child for child in parent.children if child in children]
            remaining = [child for child in parent.children if child not in children]
            parent.children = ordered + remaining

        self.widget_tree.rebuild_tree()
        self._set_selection(widgets, primary=self._primary_selected_widget(), sync_tree=True, sync_preview=True)
        self._record_page_state_change(source="send to back")

    def _set_selection_flag(self, field_name):
        widgets = [widget for widget in self._selected_widgets() if self._current_page and widget is not self._current_page.root_widget]
        if not widgets:
            return
        new_value = not all(bool(getattr(widget, field_name, False)) for widget in widgets)
        for widget in widgets:
            setattr(widget, field_name, new_value)
        primary = self._primary_selected_widget()
        self.widget_tree.rebuild_tree()
        self._set_selection(widgets, primary=primary, sync_tree=True, sync_preview=True)
        label = "designer lock" if field_name == "designer_locked" else "designer visibility"
        self._record_page_state_change(trigger_compile=False, source=label)

    def _toggle_selection_locked(self):
        self._set_selection_flag("designer_locked")

    def _toggle_selection_hidden(self):
        self._set_selection_flag("designer_hidden")

    def _update_edit_actions(self):
        if not hasattr(self, "_copy_action"):
            return

        selected_widgets = self._top_level_selected_widgets()
        selectable_widgets = [widget for widget in selected_widgets if not getattr(widget, "designer_locked", False)]
        has_selection = bool(selected_widgets)
        has_deletable_selection = bool(selectable_widgets)
        has_project = self._current_page is not None
        can_paste = has_project and self._clipboard_payload is not None and self._default_paste_parent() is not None
        can_align = len(selectable_widgets) >= 2 and self._shared_selection_parent(selectable_widgets) is not None
        can_distribute = len(selectable_widgets) >= 3 and self._shared_selection_parent(selectable_widgets) is not None

        self._copy_action.setEnabled(has_selection)
        self._cut_action.setEnabled(has_deletable_selection)
        self._paste_action.setEnabled(can_paste)
        self._duplicate_action.setEnabled(has_selection)
        self._delete_action.setEnabled(has_deletable_selection)

        self._align_left_action.setEnabled(can_align)
        self._align_right_action.setEnabled(can_align)
        self._align_top_action.setEnabled(can_align)
        self._align_bottom_action.setEnabled(can_align)
        self._align_hcenter_action.setEnabled(can_align)
        self._align_vcenter_action.setEnabled(can_align)
        self._distribute_h_action.setEnabled(can_distribute)
        self._distribute_v_action.setEnabled(can_distribute)
        self._bring_front_action.setEnabled(has_selection)
        self._send_back_action.setEnabled(has_selection)
        self._toggle_lock_action.setEnabled(has_selection)
        self._toggle_hide_action.setEnabled(has_selection)

    def _on_tree_selection_changed(self, widgets, primary):
        self._set_selection(widgets, primary=primary, sync_tree=False, sync_preview=True)

    def _on_preview_selection_changed(self, widgets, primary):
        self._set_selection(widgets, primary=primary, sync_tree=True, sync_preview=False)

    def _on_widget_selected(self, widget):
        """Widget selected from tree panel."""
        self._set_selection([widget] if widget is not None else [], primary=widget, sync_tree=False, sync_preview=True)

    def _on_preview_widget_selected(self, widget):
        """Widget selected from preview panel overlay."""
        self._set_selection([widget] if widget is not None else [], primary=widget, sync_tree=True, sync_preview=False)

    def _on_widget_moved(self, widget, new_x, new_y):
        """Widget dragged on preview overlay."""
        self._active_batch_source = "canvas move"
        if widget == self._selection_state.primary:
            self.property_panel.set_selection(self._selection_state.widgets, self._selection_state.primary)
        self._on_model_changed(source="canvas move")

    def _on_widget_resized(self, widget, new_width, new_height):
        """Widget resized on preview overlay."""
        self._active_batch_source = "canvas resize"
        if widget == self._selection_state.primary:
            self.property_panel.set_selection(self._selection_state.widgets, self._selection_state.primary)
        self._on_model_changed(source="canvas resize")

    def _on_widget_reordered(self, widget, new_index):
        """Widget reordered within a layout container."""
        self.widget_tree.rebuild_tree()
        self.widget_tree.set_selected_widgets(self._selection_state.widgets, self._selection_state.primary)
        self._on_model_changed(source="layout reorder")

    def _on_tree_changed(self):
        """Widget tree structure changed (add/delete/reorder)."""
        self.widget_tree.set_selected_widgets(self._selection_state.widgets, self._selection_state.primary)
        self._on_model_changed(source="widget tree change")

    def _on_widget_tree_feedback_message(self, message):
        if message:
            self.statusBar().showMessage(message, 5000)

    def _on_property_changed(self):
        """A property value was changed in the property panel."""
        self.widget_tree.rebuild_tree()
        self.widget_tree.set_selected_widgets(self._selection_state.widgets, self._selection_state.primary)
        self.animations_panel.refresh()
        self._on_model_changed(source="property edit")

    def _on_property_validation_message(self, message):
        if message:
            self.statusBar().showMessage(message, 5000)

    def _on_model_changed(self, source=""):
        """Common handler: model changed → record snapshot + update preview + XML + recompile."""
        self._record_page_state_change(source=source)

    def _format_page_change_message(self, source):
        if not source or self._current_page is None:
            return ""
        return f"Changed {self._current_page.name}: {source}."

    def _record_page_state_change(self, update_preview=True, trigger_compile=True, source=""):
        """Record the current page snapshot and refresh dependent UI state."""
        if self._current_page and not self._undoing:
            xml = self._current_page.to_xml_string()
            stack = self._undo_manager.get_stack(self._current_page.name)
            stack.push(xml, label=source or "property edit")
        if update_preview:
            self._update_preview_overlay()
        self._sync_xml_to_editors()
        self._update_resource_usage_panel()
        if trigger_compile:
            self._trigger_compile()
        self._update_undo_actions()
        self._update_window_title()
        message = self._format_page_change_message(source)
        if message and not self._undoing:
            self.statusBar().showMessage(message, 3000)

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
            self._apply_page_state(self._current_page, new_page)
            # Refresh UI
            self._page_shim = _PageProjectShim(self._current_page)
            self.widget_tree.set_project(self._page_shim)
            self.page_fields_panel.set_page(self._current_page)
            self.page_timers_panel.set_page(self._current_page)
            self._clear_selection(sync_tree=True, sync_preview=True)
            self._update_preview_overlay()
            self._apply_page_mockup()
            self._sync_xml_to_editors()
            self._update_resource_usage_panel()
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
            self._active_batch_source = ""
            stack = self._undo_manager.get_stack(self._current_page.name)
            stack.begin_batch()

    def _on_drag_finished(self):
        """Preview drag/resize ended — commit undo batch."""
        if self._current_page:
            xml = self._current_page.to_xml_string()
            stack = self._undo_manager.get_stack(self._current_page.name)
            stack.end_batch(xml, label=self._active_batch_source or "canvas drag")
            self._active_batch_source = ""
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
            self._apply_page_state(self._current_page, new_page)

            if not self._undoing:
                stack = self._undo_manager.get_stack(self._current_page.name)
                stack.push(xml_text, label="xml edit")

            # Refresh tree and preview (without re-syncing XML back)
            self._page_shim = _PageProjectShim(self._current_page)
            self.widget_tree.set_project(self._page_shim)
            self._clear_selection(sync_tree=True, sync_preview=True)
            self._update_preview_overlay()
            self._apply_page_mockup()
            self._update_resource_usage_panel()
            self._trigger_compile()
            self._update_undo_actions()
            self._update_window_title()
            if not self._undoing:
                self.statusBar().showMessage(f"Changed {self._current_page.name}: xml edit.", 3000)
        except Exception:
            # XML parse error — ignore until user fixes it
            pass

    # ── Preview / Compile ──────────────────────────────────────────

    def _apply_page_state(self, target_page, source_page):
        """Copy all serializable page state from source to target."""
        target_page.root_widget = source_page.root_widget
        target_page.user_fields = source_page.user_fields
        target_page.timers = source_page.timers
        target_page.mockup_image_path = source_page.mockup_image_path
        target_page.mockup_image_visible = source_page.mockup_image_visible
        target_page.mockup_image_opacity = source_page.mockup_image_opacity

    def _update_preview_overlay(self):
        """Update the preview overlay with current page widgets."""
        if self._current_page:
            compute_page_layout(self._current_page)
            widgets = self._current_page.get_all_widgets()
            self.preview_panel.set_widgets(widgets)
            self.preview_panel.set_selection(self._selection_state.widgets, self._selection_state.primary)
            self.page_navigator.refresh_thumbnail(self._current_page.name)
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

    def _set_show_grid(self, show):
        self.preview_panel.set_show_grid(show)
        self._config.show_grid = bool(show)
        self._config.save()

    def _set_grid_size(self, size):
        self.preview_panel.set_grid_size(size)
        self._config.grid_size = int(size)
        self._config.save()
        action = self._grid_size_actions.get(int(size))
        if action is not None:
            action.setChecked(True)

    def _toggle_auto_compile(self, enabled):
        self.auto_compile = enabled

    def _trigger_compile(self):
        """Trigger a debounced compile."""
        if self._is_closing:
            return
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
        if self._is_closing:
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
        self._update_diagnostics_panel()
        if not self._ensure_codegen_preflight("Compile preview", show_dialog=False, switch_to_python_preview=True):
            return

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

        generation = self._async_generation
        worker = self.compiler.compile_and_run_async(
            code=None,
            callback=lambda success, message, old_process: self._on_compile_finished(worker, generation, success, message, old_process),
            files_dict=files,
        )
        self._compile_worker = worker
        # Connect log signal for detailed timing info
        worker.log.connect(lambda message, msg_type: self._on_compile_log(worker, generation, message, msg_type))

    def _on_compile_log(self, worker, generation, message, msg_type):
        """Handle log messages from compile worker."""
        if self._is_closing or generation != self._async_generation or worker is not self._compile_worker:
            return
        self.debug_panel.log(message, msg_type)

    def _on_compile_finished(self, worker, generation, success, message, old_process):
        """Callback when background compilation completes."""
        del old_process
        self._cleanup_worker_ref(worker, "_compile_worker")
        if self._is_closing or generation != self._async_generation:
            return
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
        if self._is_closing:
            return
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
        self._stop_background_timers()
        self.preview_panel.stop_rendering()
        if self.compiler is not None:
            self.compiler.stop_exe()
        self.preview_panel.status_label.setText("Preview stopped")
        self._update_compile_availability()

    def closeEvent(self, event):
        self._is_closing = True
        if self.project and self._undo_manager.is_any_dirty():
            reply = QMessageBox.question(
                self, "Unsaved Changes",
                "There are unsaved changes. Do you want to save before closing?",
                QMessageBox.Save | QMessageBox.Discard | QMessageBox.Cancel,
                QMessageBox.Save
            )
            if reply == QMessageBox.Cancel:
                self._is_closing = False
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

        self._bump_async_generation()
        self._shutdown_async_activity(wait_ms=500)
        self.widget_tree.shutdown()
        if self.compiler is not None:
            self.compiler.cleanup()
            self.compiler = None
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
