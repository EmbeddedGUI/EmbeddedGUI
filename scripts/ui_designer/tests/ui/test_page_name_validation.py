"""Tests for page name validation in the Designer.

Covers:
  - Reserved page names that conflict with egui internal module symbols
    (e.g. 'test' → egui_test_init conflict, 'timer' → egui_timer_init conflict)
  - The _RESERVED_PAGE_NAMES set in project_dock is consistent and covers
    known problematic names.
  - Code generation for a page named after a near-reserved pattern does NOT
    accidentally conflict when the name is correctly guarded.
"""

import pytest


class TestReservedPageNames:
    """Tests for _RESERVED_PAGE_NAMES in project_dock."""

    @pytest.fixture
    def reserved_names(self):
        from ui_designer.ui.project_dock import _RESERVED_PAGE_NAMES
        return _RESERVED_PAGE_NAMES

    # ── core problematic names ──────────────────────────────────

    def test_test_is_reserved(self, reserved_names):
        """'test' conflicts with src/test/egui_test.h egui_test_init(void)."""
        assert "test" in reserved_names

    def test_timer_is_reserved(self, reserved_names):
        """'timer' conflicts with egui_timer_init."""
        assert "timer" in reserved_names

    def test_animation_is_reserved(self, reserved_names):
        assert "animation" in reserved_names

    def test_view_is_reserved(self, reserved_names):
        assert "view" in reserved_names

    def test_core_is_reserved(self, reserved_names):
        assert "core" in reserved_names

    def test_canvas_is_reserved(self, reserved_names):
        assert "canvas" in reserved_names

    def test_font_is_reserved(self, reserved_names):
        assert "font" in reserved_names

    def test_image_is_reserved(self, reserved_names):
        assert "image" in reserved_names

    def test_input_is_reserved(self, reserved_names):
        assert "input" in reserved_names

    def test_mask_is_reserved(self, reserved_names):
        assert "mask" in reserved_names

    def test_toast_is_reserved(self, reserved_names):
        assert "toast" in reserved_names

    def test_dialog_is_reserved(self, reserved_names):
        assert "dialog" in reserved_names

    def test_activity_is_reserved(self, reserved_names):
        assert "activity" in reserved_names

    # ── valid names should NOT be reserved ───────────────────

    def test_main_page_not_reserved(self, reserved_names):
        """'main_page' is the default page name and must be allowed."""
        assert "main_page" not in reserved_names

    def test_settings_page_not_reserved(self, reserved_names):
        assert "settings_page" not in reserved_names

    def test_home_not_reserved(self, reserved_names):
        assert "home" not in reserved_names

    def test_dashboard_not_reserved(self, reserved_names):
        assert "dashboard" not in reserved_names

    def test_test_page_not_reserved(self, reserved_names):
        """'test_page' is a valid name (suffix '_page' avoids conflict)."""
        assert "test_page" not in reserved_names

    def test_timer_screen_not_reserved(self, reserved_names):
        """'timer_screen' is a valid compound name."""
        assert "timer_screen" not in reserved_names


class TestPageNameGeneratesConflictingSymbol:
    """Confirm that page 'test' generates egui_test_init which conflicts.

    This validates WHY we need the reserved list — the generated function name
    would be egui_{page_name}_init, and 'egui_test_init' is already declared
    with a different signature in src/test/egui_test.h.
    """

    def test_page_init_func_name_follows_pattern(self):
        """Generated init func follows egui_{page}_init pattern."""
        from ui_designer.generator.code_generator import generate_page_user_source
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.model.page import Page
        from ui_designer.model.project import Project

        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        # Use a safe name to verify the pattern
        page = Page(file_path="layout/my_screen.xml", root_widget=root)
        proj = Project(screen_width=240, screen_height=320, app_name="TestApp")
        proj.add_page(page)

        output = generate_page_user_source(page, proj)
        # The user source defines egui_my_screen_init(egui_page_base_t *self)
        assert "egui_my_screen_init(egui_page_base_t *self)" in output

    def test_conflicting_pattern_for_test_page(self):
        """Page named 'test' would generate egui_test_init(egui_page_base_t *self)
        which conflicts with src/test/egui_test.h egui_test_init(void).
        This confirms the reserved name is necessary.
        """
        from ui_designer.generator.code_generator import generate_page_user_source
        from ui_designer.model.widget_model import WidgetModel
        from ui_designer.model.page import Page
        from ui_designer.model.project import Project

        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        page = Page(file_path="layout/test.xml", root_widget=root)
        proj = Project(screen_width=240, screen_height=320, app_name="TestApp")
        proj.add_page(page)

        output = generate_page_user_source(page, proj)
        # This function signature conflicts with egui_test.h
        assert "egui_test_init(egui_page_base_t *self)" in output
