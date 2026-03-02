"""Tests for ui_designer.engine.layout_engine — alignment and layout computation."""

import pytest

from ui_designer.engine.layout_engine import (
    align_get_x_y,
    compute_layout,
    compute_page_layout,
    EGUI_ALIGN_HCENTER,
    EGUI_ALIGN_LEFT,
    EGUI_ALIGN_RIGHT,
    EGUI_ALIGN_VCENTER,
    EGUI_ALIGN_TOP,
    EGUI_ALIGN_BOTTOM,
    ALIGN_MAP,
    _layout_linearlayout_children,
)
from ui_designer.model.widget_model import WidgetModel
from ui_designer.model.page import Page
from ui_designer.model.project import Project


# ── TestAlignGetXY ───────────────────────────────────────────────


class TestAlignGetXY:
    """Test the low-level alignment helper."""

    def test_hcenter(self):
        x, y = align_get_x_y(200, 100, 80, 40, EGUI_ALIGN_HCENTER)
        assert x == (200 - 80) >> 1  # 60
        assert y == 0

    def test_left(self):
        x, y = align_get_x_y(200, 100, 80, 40, EGUI_ALIGN_LEFT)
        assert x == 0
        assert y == 0

    def test_right(self):
        x, y = align_get_x_y(200, 100, 80, 40, EGUI_ALIGN_RIGHT)
        assert x == 200 - 80  # 120
        assert y == 0

    def test_vcenter(self):
        x, y = align_get_x_y(200, 100, 80, 40, EGUI_ALIGN_VCENTER)
        assert x == 0
        assert y == (100 - 40) >> 1  # 30

    def test_top(self):
        x, y = align_get_x_y(200, 100, 80, 40, EGUI_ALIGN_TOP)
        assert x == 0
        assert y == 0

    def test_bottom(self):
        x, y = align_get_x_y(200, 100, 80, 40, EGUI_ALIGN_BOTTOM)
        assert x == 0
        assert y == 100 - 40  # 60

    def test_center_combined(self):
        """HCENTER | VCENTER should center both axes."""
        x, y = align_get_x_y(200, 100, 80, 40,
                              EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER)
        assert x == (200 - 80) >> 1  # 60
        assert y == (100 - 40) >> 1  # 30

    def test_right_bottom_combined(self):
        x, y = align_get_x_y(200, 100, 80, 40,
                              EGUI_ALIGN_RIGHT | EGUI_ALIGN_BOTTOM)
        assert x == 120
        assert y == 60

    def test_hcenter_integer_division(self):
        """Verify >>1 integer division (truncation, not rounding)."""
        # parent_w=201, child_w=80 => (201-80)>>1 = 121>>1 = 60
        x, _ = align_get_x_y(201, 100, 80, 40, EGUI_ALIGN_HCENTER)
        assert x == 60  # not 60.5

    def test_vcenter_integer_division(self):
        _, y = align_get_x_y(200, 101, 80, 40, EGUI_ALIGN_VCENTER)
        assert y == 30  # (101-40)>>1 = 61>>1 = 30

    def test_child_larger_than_parent_hcenter_returns_zero(self):
        x, _ = align_get_x_y(50, 100, 200, 40, EGUI_ALIGN_HCENTER)
        assert x == 0

    def test_child_larger_than_parent_right_returns_zero(self):
        x, _ = align_get_x_y(50, 100, 200, 40, EGUI_ALIGN_RIGHT)
        assert x == 0

    def test_child_larger_than_parent_vcenter_returns_zero(self):
        _, y = align_get_x_y(200, 20, 80, 100, EGUI_ALIGN_VCENTER)
        assert y == 0

    def test_child_larger_than_parent_bottom_returns_zero(self):
        _, y = align_get_x_y(200, 20, 80, 100, EGUI_ALIGN_BOTTOM)
        assert y == 0

    def test_align_map_center_value(self):
        """ALIGN_MAP['EGUI_ALIGN_CENTER'] should be HCENTER | VCENTER."""
        assert ALIGN_MAP["EGUI_ALIGN_CENTER"] == (EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER)


# ── TestLayoutLinearLayout ───────────────────────────────────────


class TestLayoutLinearLayout:
    """Test _layout_linearlayout_children for vertical and horizontal modes."""

    def _make_linearlayout(self, orientation, width, height, align_str, children_specs):
        """Helper: build a LinearLayout with children of given (w, h) tuples."""
        ll = WidgetModel("linearlayout", name="ll", x=0, y=0, width=width, height=height)
        ll.properties["orientation"] = orientation
        ll.properties["align_type"] = align_str
        for i, (cw, ch) in enumerate(children_specs):
            child = WidgetModel("label", name=f"c{i}", x=0, y=0, width=cw, height=ch)
            ll.add_child(child)
        return ll

    def test_vertical_center_3_items(self):
        """3 items (100x40 each) centered in 200x300 container."""
        ll = self._make_linearlayout("vertical", 200, 300, "EGUI_ALIGN_CENTER",
                                     [(100, 40), (100, 40), (100, 40)])
        _layout_linearlayout_children(ll)
        # total_child_width = 100, total_child_height = 120
        # base_x = (200 - 100) >> 1 = 50
        # base_y = (300 - 120) >> 1 = 90
        # Each child: child_x = align within total_child_width (hcenter of 100 in 100) = 0
        for i, child in enumerate(ll.children):
            assert child.display_x == 50
            assert child.display_y == 90 + i * 40

    def test_horizontal_center_3_items(self):
        """3 items (80x30 each) centered in 300x100 container."""
        ll = self._make_linearlayout("horizontal", 300, 100, "EGUI_ALIGN_CENTER",
                                     [(80, 30), (80, 30), (80, 30)])
        _layout_linearlayout_children(ll)
        # total_child_width = 240, total_child_height = 30
        # base_x = (300 - 240) >> 1 = 30
        # base_y = (100 - 30) >> 1 = 35
        for i, child in enumerate(ll.children):
            assert child.display_x == 30 + i * 80
            assert child.display_y == 35

    def test_vertical_left_align(self):
        ll = self._make_linearlayout("vertical", 200, 300, "EGUI_ALIGN_LEFT",
                                     [(80, 40), (120, 40)])
        _layout_linearlayout_children(ll)
        # total_child_width = max(80, 120) = 120, total_child_height = 80
        # LEFT => base_x = 0, VCENTER not set (no V flags) => base_y = 0
        # child_x for each: align within total_child_width using HMASK = LEFT => x=0
        assert ll.children[0].display_x == 0
        assert ll.children[1].display_x == 0
        assert ll.children[0].display_y == 0
        assert ll.children[1].display_y == 40

    def test_vertical_right_align(self):
        ll = self._make_linearlayout("vertical", 200, 300, "EGUI_ALIGN_RIGHT",
                                     [(80, 40), (120, 40)])
        _layout_linearlayout_children(ll)
        # total_child_width = 120, total_child_height = 80
        # RIGHT => base_x = 200 - 120 = 80
        # child[0]: align 80 in 120, RIGHT => child_x = 120-80 = 40 => display_x = 80+40 = 120
        # child[1]: align 120 in 120, RIGHT => child_x = 0 => display_x = 80+0 = 80
        assert ll.children[0].display_x == 120
        assert ll.children[1].display_x == 80

    def test_horizontal_top_align(self):
        ll = self._make_linearlayout("horizontal", 300, 200, "EGUI_ALIGN_TOP",
                                     [(80, 30), (80, 50)])
        _layout_linearlayout_children(ll)
        # total_child_width = 160, total_child_height = max(30, 50) = 50
        # TOP => base_y = 0, HCENTER not set => base_x = 0
        # child[0]: align 30 in 50, TOP => child_y = 0
        # child[1]: align 50 in 50, TOP => child_y = 0
        assert ll.children[0].display_y == 0
        assert ll.children[1].display_y == 0
        assert ll.children[0].display_x == 0
        assert ll.children[1].display_x == 80

    def test_horizontal_bottom_align(self):
        ll = self._make_linearlayout("horizontal", 300, 200, "EGUI_ALIGN_BOTTOM",
                                     [(80, 30), (80, 50)])
        _layout_linearlayout_children(ll)
        # total_child_height = 50
        # BOTTOM => base_y = 200 - 50 = 150
        # child[0]: align 30 in 50, BOTTOM => child_y = 50-30 = 20 => display_y = 150+20 = 170
        # child[1]: align 50 in 50, BOTTOM => child_y = 0 => display_y = 150
        assert ll.children[0].display_y == 170
        assert ll.children[1].display_y == 150

    def test_single_child_vertical(self):
        ll = self._make_linearlayout("vertical", 200, 200, "EGUI_ALIGN_CENTER",
                                     [(100, 40)])
        _layout_linearlayout_children(ll)
        assert ll.children[0].display_x == (200 - 100) >> 1  # 50
        assert ll.children[0].display_y == (200 - 40) >> 1   # 80

    def test_no_children_does_not_crash(self):
        ll = self._make_linearlayout("vertical", 200, 200, "EGUI_ALIGN_CENTER", [])
        _layout_linearlayout_children(ll)  # should not raise


# ── TestComputeLayout ────────────────────────────────────────────


class TestComputeLayout:
    """Test compute_layout and compute_page_layout with full widget trees."""

    def test_group_children_use_model_xy(self):
        """Group children should be positioned at parent_display + child.x/y."""
        root = WidgetModel("group", name="root", x=10, y=20, width=240, height=320)
        child = WidgetModel("label", name="lbl", x=5, y=15, width=100, height=30)
        root.add_child(child)
        page = Page(file_path="layout/test.xml", root_widget=root)
        compute_page_layout(page)
        assert root.display_x == 10
        assert root.display_y == 20
        assert child.display_x == 10 + 5  # 15
        assert child.display_y == 20 + 15  # 35

    def test_linearlayout_children_computed(self):
        """LinearLayout children should have computed positions, not model x/y."""
        root = WidgetModel("linearlayout", name="ll", x=0, y=0, width=200, height=200)
        root.properties["orientation"] = "vertical"
        root.properties["align_type"] = "EGUI_ALIGN_CENTER"
        c1 = WidgetModel("label", name="c1", x=99, y=99, width=100, height=40)
        c2 = WidgetModel("label", name="c2", x=99, y=99, width=100, height=40)
        root.add_child(c1)
        root.add_child(c2)
        page = Page(file_path="layout/test.xml", root_widget=root)
        compute_page_layout(page)
        # Model x/y (99) should be ignored; layout engine computes positions
        # total_child_height = 80, base_y = (200-80)>>1 = 60
        # total_child_width = 100, base_x = (200-100)>>1 = 50
        assert c1.display_x == 50
        assert c1.display_y == 60
        assert c2.display_x == 50
        assert c2.display_y == 100

    def test_nested_group_in_linearlayout(self):
        """Group nested inside LinearLayout uses layout-computed position."""
        ll = WidgetModel("linearlayout", name="ll", x=0, y=0, width=200, height=200)
        ll.properties["orientation"] = "vertical"
        ll.properties["align_type"] = "EGUI_ALIGN_CENTER"
        group = WidgetModel("group", name="g", x=0, y=0, width=100, height=80)
        inner = WidgetModel("label", name="inner", x=10, y=10, width=50, height=20)
        group.add_child(inner)
        ll.add_child(group)
        page = Page(file_path="layout/test.xml", root_widget=ll)
        compute_page_layout(page)
        # group: base_x = (200-100)>>1 = 50, base_y = (200-80)>>1 = 60
        assert group.display_x == 50
        assert group.display_y == 60
        # inner: group's abs position + inner's model x,y
        assert inner.display_x == 50 + 10  # 60
        assert inner.display_y == 60 + 10  # 70

    def test_compute_layout_with_project(self, simple_project):
        """compute_layout works on a Project object."""
        compute_layout(simple_project)
        root = simple_project.root_widgets[0]
        assert root.display_x == root.x
        assert root.display_y == root.y

    def test_compute_layout_none_project_no_crash(self):
        """compute_layout(None) should be a safe no-op."""
        compute_layout(None)

    def test_compute_page_layout_none_page_no_crash(self):
        """compute_page_layout(None) should be a safe no-op."""
        compute_page_layout(None)

    def test_compute_page_layout_empty_page_no_crash(self):
        """Page with no root_widget should be a safe no-op."""
        page = Page(file_path="layout/empty.xml", root_widget=None)
        compute_page_layout(page)
