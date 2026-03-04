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

import os
import copy

from PyQt5.QtWidgets import (
    QMainWindow, QWidget, QVBoxLayout,
    QAction, QActionGroup, QFileDialog, QStatusBar,
    QMessageBox, QScrollArea, QDockWidget, QMenu,
    QApplication, QDialog, QStackedWidget, QToolBar, QInputDialog,
)
from PyQt5.QtCore import Qt, QTimer, QSize
from PyQt5.QtGui import QIcon

from qfluentwidgets import TabBar, TabCloseButtonDisplayMode

from .widget_tree import WidgetTreePanel
from .property_panel import PropertyPanel
from .preview_panel import PreviewPanel, MODE_VERTICAL, MODE_HORIZONTAL, MODE_HIDDEN
from .editor_tabs import EditorTabs, MODE_DESIGN, MODE_SPLIT, MODE_CODE
from .project_dock import ProjectExplorerDock
from .resource_panel import ResourcePanel
from .app_selector import AppSelectorDialog
from .welcome_page import WelcomePage
from .debug_panel import DebugPanel
from ..model.widget_model import WidgetModel
from ..model.project import Project
from ..model.page import Page
from ..model.config import get_config
from ..model.undo_manager import UndoManager
from ..generator.code_generator import generate_all_files, generate_all_files_preserved, generate_uicode
from ..generator.resource_config_generator import ResourceConfigGenerator
from ..engine.compiler import CompilerEngine
from ..engine.layout_engine import compute_layout, compute_page_layout
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
        self.project_root = project_root
        self.app_name = app_name
        self.project = None
        self.compiler = CompilerEngine(project_root, app_name)
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

        self._init_ui()
        self._init_menus()
        self._init_toolbar()
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
        self.addDockWidget(Qt.LeftDockWidgetArea, self.project_dock)
        self.project_dock.setMinimumWidth(180)
        # self.project_dock.setMaximumWidth(320)

        # ── Central area: Stacked Widget (Welcome / Editor) ──
        self._central_stack = QStackedWidget()

        # Welcome page (index 0)
        self._welcome_page = WelcomePage()
        self._welcome_page.open_recent.connect(self._open_app)
        self._welcome_page.new_project.connect(self._new_project)
        self._welcome_page.open_app.connect(self._open_app_dialog)
        self._central_stack.addWidget(self._welcome_page)

        # Editor container (index 1)
        editor_container = QWidget()
        editor_layout = QVBoxLayout(editor_container)
        editor_layout.setContentsMargins(0, 0, 0, 0)
        editor_layout.setSpacing(0)

        # Page tab bar (qfluentwidgets — movable, closable, scrollable)
        self.page_tab_bar = TabBar()
        self.page_tab_bar.setMovable(True)
        self.page_tab_bar.setTabsClosable(True)
        self.page_tab_bar.setScrollable(True)
        self.page_tab_bar.setAddButtonVisible(False)  # Hide the "+" button
        self.page_tab_bar.setCloseButtonDisplayMode(
            TabCloseButtonDisplayMode.ON_HOVER
        )
        self.page_tab_bar.setTabMaximumWidth(180)
        self.page_tab_bar.setTabShadowEnabled(False)
        self.page_tab_bar.setFixedHeight(40)
        self.page_tab_bar.tabCloseRequested.connect(self._on_page_tab_closed)
        self.page_tab_bar.currentChanged.connect(self._on_page_tab_changed)
        self.page_tab_bar.setContextMenuPolicy(Qt.CustomContextMenu)
        self.page_tab_bar.customContextMenuRequested.connect(
            self._show_tab_context_menu
        )

        editor_layout.addWidget(self.page_tab_bar)

        self.preview_panel = PreviewPanel(screen_width=240, screen_height=320)
        self.editor_tabs = EditorTabs(self.preview_panel)
        editor_layout.addWidget(self.editor_tabs, 1)

        self._central_stack.addWidget(editor_container)

        self.setCentralWidget(self._central_stack)

        # ── Right dock: Widget Tree ──
        self.tree_dock = QDockWidget("Widget Tree", self)
        self.tree_dock.setAllowedAreas(Qt.AllDockWidgetAreas)
        self.widget_tree = WidgetTreePanel()
        self.tree_dock.setWidget(self.widget_tree)
        self.tree_dock.setMinimumWidth(180)
        self.addDockWidget(Qt.RightDockWidgetArea, self.tree_dock)

        # ── Right dock: Properties ──
        self.props_dock = QDockWidget("Properties", self)
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
        self.res_dock.setAllowedAreas(Qt.AllDockWidgetAreas)
        self.res_panel = ResourcePanel()
        self.res_dock.setWidget(self.res_panel)
        self.res_dock.setMinimumWidth(200)
        self.addDockWidget(Qt.LeftDockWidgetArea, self.res_dock)
        # Stack below project explorer
        self.splitDockWidget(self.project_dock, self.res_dock, Qt.Vertical)

        # ── Bottom dock: Debug Output ──
        self.debug_dock = QDockWidget("Debug Output", self)
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

    def _init_menus(self):
        menubar = self.menuBar()

        # ── File menu ──
        file_menu = menubar.addMenu("File")

        new_action = QAction("New Project", self)
        new_action.setShortcut("Ctrl+N")
        new_action.triggered.connect(self._new_project)
        file_menu.addAction(new_action)

        open_app_action = QAction("Open App...", self)
        open_app_action.setShortcut("Ctrl+Shift+O")
        open_app_action.triggered.connect(self._open_app_dialog)
        file_menu.addAction(open_app_action)

        open_action = QAction("Open Project File...", self)
        open_action.setShortcut("Ctrl+O")
        open_action.triggered.connect(self._open_project)
        file_menu.addAction(open_action)

        # Recent Apps submenu
        self._recent_menu = file_menu.addMenu("Recent Apps")
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
        """Update the Recent Apps submenu."""
        self._recent_menu.clear()
        recent = self._config.recent_apps
        if not recent:
            action = QAction("(No recent apps)", self)
            action.setEnabled(False)
            self._recent_menu.addAction(action)
            return

        for app_name, egui_root in recent[:10]:
            action = QAction(f"{app_name}", self)
            action.setToolTip(egui_root)
            action.triggered.connect(
                lambda checked, a=app_name, r=egui_root: self._open_app(a, r)
            )
            self._recent_menu.addAction(action)

    def _open_app_dialog(self):
        """Show dialog to select and open an app."""
        dialog = AppSelectorDialog(self, self.project_root)
        if dialog.exec_() == QDialog.Accepted:
            app_name = dialog.selected_app
            egui_root = dialog.egui_root
            if app_name and egui_root:
                self._open_app(app_name, egui_root)

    def _open_app(self, app_name, egui_root):
        """Open a specific app.

        Args:
            app_name: Name of the app (e.g., "HelloDesigner")
            egui_root: Path to EmbeddedGUI root directory
        """
        # Update paths
        self.project_root = egui_root
        self.app_name = app_name

        # Update compiler
        self.compiler = CompilerEngine(egui_root, app_name)

        # Find project file (.egui)
        app_dir = os.path.join(egui_root, "example", app_name)
        eui_file = os.path.join(app_dir, f"{app_name}.egui")

        if os.path.isfile(eui_file):
            try:
                self.project = Project.load(eui_file)
                self.project.egui_root = egui_root
                self._project_dir = app_dir
            except Exception as e:
                QMessageBox.warning(
                    self, "Warning",
                    f"Failed to load {app_name}.egui:\n{e}\n\n"
                    "Creating a new project."
                )
                self._create_new_project_for_app(app_name, egui_root, app_dir)
        else:
            # Create new project
            self._create_new_project_for_app(app_name, egui_root, app_dir)

        # Update config
        self._config.egui_root = egui_root
        self._config.last_app = app_name
        self._config.last_project_path = os.path.abspath(eui_file) if os.path.isfile(eui_file) else ""
        self._config.add_recent_app(app_name, egui_root)
        self._config.save()
        self._update_recent_menu()

        # Reset undo manager for the new project
        self._undo_manager = UndoManager()

        # Update UI - switch to editor view
        self._show_editor()
        self._selected_widget = None
        self._apply_project()
        self._update_window_title()

        # Switch to startup page
        startup = self.project.get_startup_page()
        if startup:
            self._switch_page(startup.name)
        elif self.project.pages:
            self._switch_page(self.project.pages[0].name)

        self._trigger_compile()
        self.statusBar().showMessage(f"Opened app: {app_name}")

    def _create_new_project_for_app(self, app_name, egui_root, app_dir):
        """Create a new project for the given app."""
        WidgetModel.reset_counter()

        # Try to read screen dimensions from app_egui_config.h
        screen_w, screen_h = 240, 320
        config_h = os.path.join(app_dir, "app_egui_config.h")
        if os.path.isfile(config_h):
            try:
                with open(config_h, "r", encoding="utf-8") as f:
                    content = f.read()
                import re
                m = re.search(r"EGUI_CONFIG_SCEEN_WIDTH\s+(\d+)", content)
                if m:
                    screen_w = int(m.group(1))
                m = re.search(r"EGUI_CONFIG_SCEEN_HEIGHT\s+(\d+)", content)
                if m:
                    screen_h = int(m.group(1))
            except Exception:
                pass

        self.project = Project(
            screen_width=screen_w,
            screen_height=screen_h,
            app_name=app_name
        )
        self.project.egui_root = egui_root
        self.project.create_new_page("main_page")
        self._project_dir = app_dir

    def _update_window_title(self):
        """Update window title with current app name and dirty indicator."""
        title = f"EmbeddedGUI Designer - {self.app_name}"
        if self._project_dir:
            title += f" [{self._project_dir}]"
        if self._undo_manager.is_any_dirty():
            title += " *"
        self.setWindowTitle(title)

    def _new_project(self):
        """Create a new project - shows app selection dialog first."""
        dialog = AppSelectorDialog(self, self.project_root)
        dialog.setWindowTitle("New Project - Select App")
        if dialog.exec_() != QDialog.Accepted:
            return

        app_name = dialog.selected_app
        egui_root = dialog.egui_root
        if not app_name or not egui_root:
            return

        # Check if project file already exists
        app_dir = os.path.join(egui_root, "example", app_name)
        eui_file = os.path.join(app_dir, f"{app_name}.egui")
        if os.path.isfile(eui_file):
            reply = QMessageBox.question(
                self, "Project Exists",
                f"A project already exists in '{app_name}'.\n\n"
                "Do you want to overwrite it with a new project?",
                QMessageBox.Yes | QMessageBox.No,
                QMessageBox.No
            )
            if reply != QMessageBox.Yes:
                return

        # Update paths
        self.project_root = egui_root
        self.app_name = app_name
        self.compiler = CompilerEngine(egui_root, app_name)

        # Create new project
        self._create_new_project_for_app(app_name, egui_root, app_dir)

        # Update config
        self._config.egui_root = egui_root
        self._config.last_app = app_name
        self._config.last_project_path = os.path.abspath(eui_file)
        self._config.add_recent_app(app_name, egui_root)
        self._config.save()
        self._update_recent_menu()

        # Reset undo manager for the new project
        self._undo_manager = UndoManager()

        # Update UI - switch to editor view
        self._show_editor()
        self._selected_widget = None
        self._apply_project()
        self._update_window_title()
        self._switch_page("main_page")
        self._trigger_compile()
        self.statusBar().showMessage(f"Created new project for: {app_name}")

    def _open_project(self):
        path, _ = QFileDialog.getOpenFileName(
            self, "Open Project", "",
            "EmbeddedGUI Projects (*.egui);;All Files (*.*)"
        )
        if not path:
            return
        try:
            self.project = Project.load(path)
            self._project_dir = os.path.dirname(os.path.abspath(path))

            # Try to infer egui_root from path (project is in example/<app>/)
            if not self.project.egui_root:
                # Path structure: .../example/<app>/{app}.egui
                app_dir = self._project_dir
                example_dir = os.path.dirname(app_dir)
                if os.path.basename(example_dir) == "example":
                    self.project.egui_root = os.path.dirname(example_dir)
                    self.project_root = self.project.egui_root

            # Update app_name and compiler if egui_root is valid
            if self.project.egui_root and self.project.app_name:
                self.app_name = self.project.app_name
                self.project_root = self.project.egui_root
                self.compiler = CompilerEngine(self.project_root, self.app_name)

                # Update config
                self._config.egui_root = self.project.egui_root
                self._config.last_app = self.app_name
                self._config.last_project_path = os.path.abspath(path)
                self._config.add_recent_app(self.app_name, self.project.egui_root)
                self._config.save()
                self._update_recent_menu()

            # Reset undo manager for the new project
            self._undo_manager = UndoManager()

            # Switch to editor view
            self._show_editor()
            self._selected_widget = None
            self._apply_project()
            self._update_window_title()

            # Switch to startup page
            startup = self.project.get_startup_page()
            if startup:
                self._switch_page(startup.name)
            elif self.project.pages:
                self._switch_page(self.project.pages[0].name)
            self._trigger_compile()
            self.statusBar().showMessage(f"Opened: {path}")
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to open project:\n{e}")

    def _save_project(self):
        if self.project is None:
            self.statusBar().showMessage("No project to save")
            return

        # Flush any pending XML edits into the model before saving
        self._flush_pending_xml()

        # Default to app directory if not set
        if not self._project_dir and self.project_root and self.app_name:
            self._project_dir = os.path.join(self.project_root, "example", self.app_name)

        if self._project_dir:
            os.makedirs(self._project_dir, exist_ok=True)
            self.project.save(self._project_dir)

            # Generate C code files with user code preservation
            files = generate_all_files_preserved(
                self.project, self._project_dir, backup=True,
            )
            for filename, content in files.items():
                filepath = os.path.join(self._project_dir, filename)
                with open(filepath, "w", encoding="utf-8") as f:
                    f.write(content)

            self._undo_manager.mark_all_saved()
            self._update_window_title()
            n = len(files)
            self.statusBar().showMessage(
                f"Saved: {self._project_dir} ({n} code file(s) updated)"
            )
        else:
            self._save_project_as()

    def _save_project_as(self):
        if self.project is None:
            self.statusBar().showMessage("No project to save")
            return

        path = QFileDialog.getExistingDirectory(
            self, "Save Project To Directory"
        )
        if path:
            self.project.save(path)
            self._project_dir = path
            self._undo_manager.mark_all_saved()
            self._update_window_title()
            self.statusBar().showMessage(f"Saved: {path}")

    def _close_project(self):
        """Close current project and return to welcome page."""
        if self.project is None:
            # Already no project, just show welcome page
            self._show_welcome_page()
            return

        # Ask to save changes only if there are unsaved modifications
        if self._undo_manager.is_any_dirty():
            reply = QMessageBox.question(
                self, "Close Project",
                "There are unsaved changes. Do you want to save before closing?",
                QMessageBox.Save | QMessageBox.Discard | QMessageBox.Cancel,
                QMessageBox.Save
            )

            if reply == QMessageBox.Cancel:
                return
            elif reply == QMessageBox.Save:
                self._save_project()

        # Stop any running exe
        self.compiler.stop_exe()

        # Clear project state
        self.project = None
        self._project_dir = None
        self._selected_widget = None
        self._current_page = None
        self._undo_manager = UndoManager()

        # Clear UI panels
        self.page_tab_bar.clear()
        self.widget_tree.set_project(None)
        self.property_panel.set_widget(None)
        self.preview_panel.set_widgets([])
        self.preview_panel.clear_background_image()
        self.project_dock.set_project(None)

        # Show welcome page
        self._show_welcome_page()
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
        # Load project-level custom widget plugins
        if self._project_dir:
            from ..model.widget_registry import WidgetRegistry
            custom_dir = os.path.join(self._project_dir, ".eguiproject", "custom_widgets")
            WidgetRegistry.instance().load_custom_widgets(custom_dir)

        self.project_dock.set_project(self.project)
        self.preview_panel.update_screen_size(self.project.screen_width, self.project.screen_height)
        self.compiler.set_screen_size(self.project.screen_width, self.project.screen_height)
        self.page_tab_bar.clear()

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

        # Start background precompile if exe doesn't exist
        self._start_precompile()

    def _start_precompile(self):
        """Start background precompile if exe doesn't exist."""
        if not self.project:
            return
        if self._precompile_worker is not None and self._precompile_worker.isRunning():
            return
        if not self.compiler.is_exe_ready():
            self.statusBar().showMessage("Background compiling...")
            self.debug_panel.log_action("Starting background precompile...")
            self.debug_panel.log_cmd(f"make -j main.exe APP={self.app_name} PORT=designer COMPILE_DEBUG= COMPILE_OPT_LEVEL=-O0")
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
        # Fallback for unsaved projects: use example/<app>/resource under project root
        return os.path.join(self.project_root, "example", self.app_name, "resource")

    def _get_eguiproject_resource_dir(self):
        """Compute the .eguiproject/resources/ path for the current project.

        This is the authoritative directory for all source resource files.
        Used by the resource panel for browsing and importing.
        """
        if self._project_dir:
            return os.path.join(self._project_dir, ".eguiproject", "resources")
        return os.path.join(
            self.project_root, "example", self.app_name,
            ".eguiproject", "resources"
        )

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
        print("[DEBUG] Resource imported, starting regen timer...")
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

    def _generate_resources(self, silent=False):
        """Run the resource generation pipeline.

        Steps:
        1. Sync .eguiproject/resources/ -> resource/src/
        2. Generate app_resource_config.json from layout XML (ResourceConfigGenerator)
        3. Run app_resource_generate.py to produce C source files

        Args:
            silent: If True, suppress warning dialogs (used for auto-trigger).
        """
        print(f"[DEBUG] _generate_resources called, silent={silent}")
        res_dir = self._get_resource_dir()
        eguiproject_res_dir = self._get_eguiproject_resource_dir()
        src_dir = os.path.join(res_dir, "src") if res_dir else ""
        print(f"[DEBUG] res_dir={res_dir}, eguiproject_res_dir={eguiproject_res_dir}")

        # Check that we have source files in .eguiproject/resources/
        if not eguiproject_res_dir or not os.path.isdir(eguiproject_res_dir):
            if not silent:
                QMessageBox.warning(
                    self, "Error",
                    "No .eguiproject/resources/ directory found.\n"
                    "Please import resources first."
                )
            return

        # Step 0: Sync .eguiproject/resources/ -> resource/src/
        if self.project:
            project_dir = self._project_dir or os.path.join(
                self.project_root, "example", self.app_name
            )
            self.project.sync_resources_to_src(project_dir)
            print("[DEBUG] Synced .eguiproject/resources/ -> resource/src/")

        # Step 1: Generate app_resource_config.json from widget properties
        if self.project:
            try:
                gen = ResourceConfigGenerator()
                gen.generate_and_save(self.project, src_dir)
                print("[DEBUG] app_resource_config.json generated from XML")
            except Exception as e:
                print(f"[DEBUG] ResourceConfigGenerator failed: {e}")
                if not silent:
                    QMessageBox.warning(
                        self, "Error",
                        f"Failed to generate resource config:\n{e}"
                    )
                return

        # Step 2: Run app_resource_generate.py
        gen_script = os.path.join(
            self.project_root, "scripts", "tools", "app_resource_generate.py"
        )
        if not os.path.isfile(gen_script):
            if not silent:
                QMessageBox.warning(
                    self, "Error",
                    f"Cannot find resource generator:\n{gen_script}"
                )
            return

        import subprocess, sys
        output_dir = os.path.join(self.project_root, "output")
        os.makedirs(output_dir, exist_ok=True)

        cmd = [
            sys.executable, gen_script,
            "-r", res_dir,
            "-o", output_dir,
            "-f", "true",
        ]
        print(f"[DEBUG] Running: {' '.join(cmd)}")
        self.statusBar().showMessage("Generating resources...")
        try:
            result = subprocess.run(
                cmd, capture_output=True, text=True, timeout=120,
                cwd=self.project_root,
            )
            if result.returncode == 0:
                self._resources_need_regen = False
                self.statusBar().showMessage("Resource generation completed.")
            else:
                err = result.stderr or result.stdout or "Unknown error"
                if not silent:
                    QMessageBox.warning(
                        self, "Resource Generation Failed",
                        f"Return code {result.returncode}:\n{err[:2000]}"
                    )
                self.statusBar().showMessage("Resource generation FAILED.")
        except Exception as e:
            if not silent:
                QMessageBox.warning(
                    self, "Error", f"Failed to run resource generator:\n{e}"
                )
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

        res_dir = self._get_resource_dir()
        eguiproject_res_dir = self._get_eguiproject_resource_dir()
        src_dir = os.path.join(res_dir, "src") if res_dir else ""

        if not eguiproject_res_dir or not os.path.isdir(eguiproject_res_dir):
            return

        # Sync .eguiproject/resources/ -> resource/src/ for pipeline
        if self.project:
            project_dir = self._project_dir or os.path.join(
                self.project_root, "example", self.app_name
            )
            self.project.sync_resources_to_src(project_dir)

        # Step 1: Generate app_resource_config.json from widget properties
        try:
            gen = ResourceConfigGenerator()
            gen.generate_and_save(self.project, src_dir)
        except Exception as e:
            self.debug_panel.log_error(f"Resource config generation failed: {e}")
            return

        # Step 2: Run app_resource_generate.py
        gen_script = os.path.join(
            self.project_root, "scripts", "tools", "app_resource_generate.py"
        )
        if not os.path.isfile(gen_script):
            return

        import subprocess, sys
        output_dir = os.path.join(self.project_root, "output")
        os.makedirs(output_dir, exist_ok=True)

        cmd = [
            sys.executable, gen_script,
            "-r", res_dir,
            "-o", output_dir,
            "-f", "true",
        ]
        try:
            result = subprocess.run(
                cmd, capture_output=True, text=True, timeout=120,
                cwd=self.project_root,
            )
            if result.returncode == 0:
                self._resources_need_regen = False
                self.debug_panel.log_info("Resources generated successfully")
            else:
                err = result.stderr or result.stdout or "Unknown error"
                self.debug_panel.log_error(f"Resource generation failed (rc={result.returncode})")
                self.debug_panel.log_compile_output(False, err[:2000])
        except Exception as e:
            self.debug_panel.log_error(f"Resource generation error: {e}")

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
            self.page_tab_bar.clear()

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
        if self.auto_compile:
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
        self.debug_panel.log_cmd(f"make -j main.exe APP={self.app_name} PORT=designer COMPILE_DEBUG= COMPILE_OPT_LEVEL=-O0")

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
            self.preview_panel.status_label.setText("FAILED")
            # Show debug dock on compile failure
            self.debug_dock.show()
            self.debug_dock.raise_()

    def _try_embed_exe(self):
        """Legacy - headless rendering replaces window embedding."""
        pass

    def _stop_exe(self):
        self.preview_panel.stop_rendering()
        self.compiler.stop_exe()
        self.preview_panel.status_label.setText("Preview stopped")

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
        self._config.save()

        self.preview_panel.stop_rendering()
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
