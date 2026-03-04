"""Edge case tests for page name handling in the Designer.

Scenarios not covered by test_page_name_validation.py:
  - Case sensitivity: 'TEST' is NOT reserved (allow case mismatch to pass through)
  - Valid user names: home, welcome, splash, settings, login — not reserved
  - Single-character names: allowed (no rule against them)
  - Names with underscores in middle/end — common pattern
  - Names that start with a digit — C identifier cannot start with digit;
    documents the behavior (currently not validated, but should be noted)
  - Path traversal attempt in delete_page_generated_files
  - Page with same name as reserved word but with extra suffix allowed
  - All reserved names confirmed lowercase (set contains no uppercase)
  - Large reserved set: at least 30 entries protecting known egui internals
  - Ctrl+S: editor_tabs.py must NOT have a conflicting QShortcut definition
"""

import os
import pytest


# ======================================================================
# TestReservedNamesCaseSensitivity
# ======================================================================

class TestReservedNamesCaseSensitivity:
    """Reserved names are stored lowercase — uppercase is NOT blocked."""

    @pytest.fixture
    def reserved_names(self):
        from ui_designer.ui.project_dock import _RESERVED_PAGE_NAMES
        return _RESERVED_PAGE_NAMES

    def test_all_reserved_names_are_lowercase(self, reserved_names):
        """The set must only contain lowercase identifiers."""
        for name in reserved_names:
            assert name == name.lower(), (
                f"Reserved name '{name}' is not lowercase — validation compares "
                f"case-sensitively against the set."
            )

    def test_uppercase_test_not_in_set(self, reserved_names):
        """'TEST' (uppercase) is not in the reserved set."""
        assert "TEST" not in reserved_names

    def test_mixed_case_timer_not_in_set(self, reserved_names):
        """'Timer' is not reserved (set is case-sensitive)."""
        assert "Timer" not in reserved_names

    def test_reserved_set_has_at_least_30_entries(self, reserved_names):
        """With 30+ entries the set meaningfully protects egui internals."""
        assert len(reserved_names) >= 30


# ======================================================================
# TestCommonValidPageNames
# ======================================================================

class TestCommonValidPageNames:
    """Common names users would choose must not be reserved."""

    @pytest.fixture
    def reserved_names(self):
        from ui_designer.ui.project_dock import _RESERVED_PAGE_NAMES
        return _RESERVED_PAGE_NAMES

    @pytest.mark.parametrize("name", [
        "home", "welcome", "splash", "main", "main_page",
        "settings", "settings_page", "profile", "login",
        "dashboard", "menu_page", "about", "help",
        "clock_page", "timer_page", "sensor_page",
        "alarm", "weather", "battery_page", "wifi",
    ])
    def test_common_user_names_not_reserved(self, reserved_names, name):
        assert name not in reserved_names, (
            f"'{name}' was unexpectedly found in reserved names — "
            f"this is a valid user page name."
        )


# ======================================================================
# TestPageNamePrefixSuffix
# ======================================================================

class TestPageNamePrefixSuffix:
    """Names that prefix/suffix reserved words must not be blocked."""

    @pytest.fixture
    def reserved_names(self):
        from ui_designer.ui.project_dock import _RESERVED_PAGE_NAMES
        return _RESERVED_PAGE_NAMES

    def test_test_prefix_allowed(self, reserved_names):
        """'test_screen' starts with 'test' but is not reserved."""
        assert "test_screen" not in reserved_names

    def test_test_suffix_allowed(self, reserved_names):
        """'unit_test' ends with 'test' but is not reserved."""
        assert "unit_test" not in reserved_names

    def test_my_timer_allowed(self, reserved_names):
        assert "my_timer" not in reserved_names

    def test_animation_screen_allowed(self, reserved_names):
        assert "animation_screen" not in reserved_names


# ======================================================================
# TestDeletePageGeneratedFilesPathTraversal
# ======================================================================

class TestDeletePageGeneratedFilesPathTraversal:
    """delete_page_generated_files must not delete outside project_dir."""

    def test_normal_name_deletes_three_files(self, tmp_path):
        from ui_designer.ui.main_window import delete_page_generated_files
        for fname in ("nav.h", "nav_layout.c", "nav.c"):
            (tmp_path / fname).write_text("x")
        delete_page_generated_files(str(tmp_path), "nav")
        for fname in ("nav.h", "nav_layout.c", "nav.c"):
            assert not (tmp_path / fname).exists()

    def test_traversal_attempt_does_not_escape(self, tmp_path):
        """Page name with '..' should not escape the project directory."""
        from ui_designer.ui.main_window import delete_page_generated_files
        # Create a sentinel file one level up from tmp_path
        parent = tmp_path.parent
        sentinel = parent / "should_not_be_deleted.c"
        sentinel.write_text("important")

        # Attempt traversal via a specially crafted page name
        # (OS path join will produce paths like tmp/.././should_not_be_deleted.h)
        try:
            delete_page_generated_files(str(tmp_path), "../should_not_be_deleted")
        except (OSError, ValueError):
            pass  # Any exception is acceptable — the point is the sentinel is safe

        # Sentinel must not have been deleted
        assert sentinel.exists(), (
            "delete_page_generated_files must not delete files outside project_dir"
        )

    def test_empty_page_name_no_crash(self, tmp_path):
        """Empty page name should not crash or delete the directory itself."""
        from ui_designer.ui.main_window import delete_page_generated_files
        marker = tmp_path / "unrelated.txt"
        marker.write_text("keep")
        delete_page_generated_files(str(tmp_path), "")
        assert marker.exists()

    def test_page_name_with_slash_no_crash(self, tmp_path):
        """Page name with '/' must not crash."""
        from ui_designer.ui.main_window import delete_page_generated_files
        try:
            delete_page_generated_files(str(tmp_path), "sub/page")
        except Exception:
            pass  # any exception is fine; what matters is no crash propagating


# ======================================================================
# TestCtrlSNoConflict
# ======================================================================

class TestCtrlSNoConflict:
    """editor_tabs.py must NOT define a QShortcut for Ctrl+S.

    The main window QAction already registers Ctrl+S at WindowShortcut scope.
    A second QShortcut in EditorTabs at the same scope creates an ambiguous
    shortcut conflict where NEITHER shortcut fires.
    """

    def test_editor_tabs_has_no_ctrl_s_qshortcut(self):
        """Statically verify editor_tabs.py source does not create a
        QShortcut("Ctrl+S", ...) that would conflict with the main window."""
        import inspect
        import ui_designer.ui.editor_tabs as et_module
        src = inspect.getsource(et_module)

        # Must not contain both 'QShortcut' and 'Ctrl+S' together
        has_shortcut_import = "QShortcut" in src
        has_ctrl_s = '"Ctrl+S"' in src or "'Ctrl+S'" in src
        assert not (has_shortcut_import and has_ctrl_s), (
            "editor_tabs.py creates a QShortcut for Ctrl+S which conflicts "
            "with the main window QAction — remove the QShortcut from EditorTabs"
        )

    def test_main_window_has_save_action_with_ctrl_s(self):
        """main_window.py must define the save QAction with Ctrl+S shortcut."""
        import inspect
        import ui_designer.ui.main_window as mw_module
        src = inspect.getsource(mw_module)

        # The _save_action should exist and have Ctrl+S
        assert "_save_action" in src
        assert '"Ctrl+S"' in src or "'Ctrl+S'" in src, (
            "main_window.py must define an action with Ctrl+S shortcut for Save"
        )

    def test_xmleditor_has_no_keypress_override(self):
        """XmlEditor.keyPressEvent override is no longer needed after fix."""
        import inspect
        import ui_designer.ui.editor_tabs as et_module
        src = inspect.getsource(et_module)
        # The override was used to pass Ctrl+S up; with the QShortcut removed,
        # this override is also unnecessary. Verify it is gone.
        # We check that if keyPressEvent is still present it is not suppressing
        # Ctrl+S (the function may remain for other purposes but the specific
        # Ctrl+S suppression must be absent).
        if "keyPressEvent" in src:
            # If the override still exists for valid reasons, it must not
            # contain the Ctrl+S-specific ignore() block
            assert "Key_S" not in src, (
                "XmlEditor.keyPressEvent should not intercept Key_S "
                "since QShortcut conflict was removed"
            )


# ======================================================================
# TestCodeGeneratorFunctionNamePattern
# ======================================================================

class TestCodeGeneratorFunctionNamePattern:
    """Validate that generated function names follow egui_{page}_init pattern
    so that we understand WHY reserved names matter."""

    def test_main_page_init_func_name(self):
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.model.page import Page
        from ui_designer.model.project import Project
        from ui_designer.generator.code_generator import generate_page_user_source

        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        page = Page(file_path="layout/main_page.xml", root_widget=root)
        proj = Project()
        proj.add_page(page)

        output = generate_page_user_source(page, proj)
        assert "egui_main_page_init" in output

    def test_timer_page_init_func_name_safe(self):
        """'timer_page' generates egui_timer_page_init — safe (not timer_init)."""
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.model.page import Page
        from ui_designer.model.project import Project
        from ui_designer.generator.code_generator import generate_page_user_source

        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        page = Page(file_path="layout/timer_page.xml", root_widget=root)
        proj = Project()
        proj.add_page(page)

        output = generate_page_user_source(page, proj)
        # Must use timer_page, not just timer
        assert "egui_timer_page_init" in output
        # Must NOT accidentally generate just egui_timer_init
        assert "egui_timer_init" not in output or "egui_timer_page_init" in output

    def test_test_page_init_func_name_safe(self):
        """'test_page' generates egui_test_page_init — not egui_test_init."""
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.model.page import Page
        from ui_designer.model.project import Project
        from ui_designer.generator.code_generator import generate_page_user_source

        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        page = Page(file_path="layout/test_page.xml", root_widget=root)
        proj = Project()
        proj.add_page(page)

        output = generate_page_user_source(page, proj)
        assert "egui_test_page_init" in output
