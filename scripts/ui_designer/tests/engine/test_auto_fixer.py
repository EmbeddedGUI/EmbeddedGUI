"""Tests for auto_fixer engine."""

import pytest

from ui_designer.engine.auto_fixer import AutoFixer, FixReport
from ui_designer.model.widget_registry import WidgetRegistry
from ui_designer.model.widget_model import WidgetModel

import os

# Ensure custom widgets are loaded
_loaded = False
def _ensure_widgets():
    global _loaded
    if not _loaded:
        cw_dir = os.path.normpath(
            os.path.join(os.path.dirname(__file__), "..", "..", "custom_widgets")
        )
        if os.path.isdir(cw_dir):
            WidgetRegistry.instance().load_custom_widgets(cw_dir)
        _loaded = True


@pytest.fixture(autouse=True)
def setup_registry():
    _ensure_widgets()
    WidgetModel.reset_counter()


class TestFixReport:
    """Test FixReport data structure."""

    def test_empty_report(self):
        report = FixReport()
        assert report.auto_fixed == []
        assert report.warnings == []
        assert report.to_dict()["auto_fixed"] == []

    def test_add_fix(self):
        report = FixReport()
        report.add_fix("label_height", "label_1", "height 10 -> 16")
        assert len(report.auto_fixed) == 1
        assert report.auto_fixed[0]["rule"] == "label_height"

    def test_add_warning(self):
        report = FixReport()
        report.add_warning("overlap", "btn_1", "overlaps with btn_2")
        assert len(report.warnings) == 1


class TestLabelHeightFix:
    """Test label height auto-fix."""

    def test_label_too_short(self):
        label = WidgetModel("label", "lbl", width=100, height=8)
        label.properties["font"] = "&egui_res_font_montserrat_14_4"
        fixer = AutoFixer()
        report = fixer.fix_widgets([label])
        assert label.height >= 14
        assert len(report.auto_fixed) == 1

    def test_label_adequate_height(self):
        label = WidgetModel("label", "lbl", width=100, height=20)
        label.properties["font"] = "&egui_res_font_montserrat_14_4"
        fixer = AutoFixer()
        report = fixer.fix_widgets([label])
        assert label.height == 20
        assert len(report.auto_fixed) == 0


class TestContainerOverflowFix:
    """Test container overflow auto-fix."""

    def test_child_exceeds_parent(self):
        parent = WidgetModel("group", "grp", width=100, height=50)
        child = WidgetModel("label", "lbl", x=10, y=10, width=50, height=60)
        parent.add_child(child)
        fixer = AutoFixer()
        report = fixer.fix_widgets([parent])
        assert parent.height >= 70
        assert len(report.auto_fixed) >= 1

    def test_no_overflow(self):
        parent = WidgetModel("group", "grp", width=100, height=100)
        child = WidgetModel("label", "lbl", x=10, y=10, width=50, height=20)
        parent.add_child(child)
        fixer = AutoFixer()
        report = fixer.fix_widgets([parent])
        assert parent.height == 100


class TestOverlapDetection:
    """Test overlap detection (warning only)."""

    def test_overlapping_siblings(self):
        parent = WidgetModel("group", "grp", width=200, height=200)
        child1 = WidgetModel("label", "lbl1", x=10, y=10, width=80, height=30)
        child2 = WidgetModel("label", "lbl2", x=50, y=20, width=80, height=30)
        parent.add_child(child1)
        parent.add_child(child2)
        fixer = AutoFixer()
        report = fixer.fix_widgets([parent])
        assert len(report.warnings) >= 1
        assert "overlap" in report.warnings[0]["rule"]

    def test_no_overlap(self):
        parent = WidgetModel("group", "grp", width=200, height=200)
        child1 = WidgetModel("label", "lbl1", x=10, y=10, width=40, height=30)
        child2 = WidgetModel("label", "lbl2", x=60, y=10, width=40, height=30)
        parent.add_child(child1)
        parent.add_child(child2)
        fixer = AutoFixer()
        report = fixer.fix_widgets([parent])
        assert len(report.warnings) == 0


class TestIdConflictFix:
    """Test duplicate ID auto-fix."""

    def test_duplicate_ids(self):
        w1 = WidgetModel("label", "my_label", width=100, height=20)
        w2 = WidgetModel("label", "my_label", width=100, height=20)
        fixer = AutoFixer()
        report = fixer.fix_widgets([w1, w2])
        assert w1.name != w2.name
        assert len(report.auto_fixed) >= 1

    def test_unique_ids(self):
        w1 = WidgetModel("label", "label_a", width=100, height=20)
        w2 = WidgetModel("label", "label_b", width=100, height=20)
        fixer = AutoFixer()
        report = fixer.fix_widgets([w1, w2])
        id_fixes = [f for f in report.auto_fixed if f["rule"] == "id_conflict"]
        assert len(id_fixes) == 0


class TestImageCropFix:
    """Test image resize auto-fix."""

    def test_image_missing_resize(self):
        img = WidgetModel("image", "img1", width=100, height=100)
        img.properties["image_file"] = "icon.png"
        img.properties["image_resize"] = ""
        fixer = AutoFixer()
        report = fixer.fix_widgets([img])
        assert img.properties.get("image_resize") != ""
        assert len(report.auto_fixed) >= 1

    def test_image_with_resize(self):
        img = WidgetModel("image", "img1", width=100, height=100)
        img.properties["image_file"] = "icon.png"
        img.properties["image_resize"] = "EGUI_IMAGE_RESIZE_SCALE"
        fixer = AutoFixer()
        report = fixer.fix_widgets([img])
        resize_fixes = [f for f in report.auto_fixed if f["rule"] == "image_crop"]
        assert len(resize_fixes) == 0
