"""Tests for ui_designer.model.diagnostics."""

from ui_designer.model.diagnostics import analyze_page, analyze_selection
from ui_designer.model.page import Page
from ui_designer.model.resource_catalog import ResourceCatalog
from ui_designer.model.string_resource import StringResourceCatalog
from ui_designer.model.widget_model import WidgetModel


class TestPageDiagnostics:
    def test_analyze_page_reports_invalid_duplicate_bounds_and_missing_resource(self, tmp_path):
        page = Page.create_default("main_page", screen_width=240, screen_height=320)

        invalid = WidgetModel("label", name="bad-name", x=8, y=8, width=60, height=20)
        duplicate_a = WidgetModel("label", name="dup_name", x=20, y=40, width=60, height=20)
        duplicate_b = WidgetModel("label", name="dup_name", x=230, y=40, width=30, height=20)
        missing = WidgetModel("image", name="missing_image", x=16, y=80, width=48, height=48)
        missing.properties["image_file"] = "ghost.png"

        page.root_widget.add_child(invalid)
        page.root_widget.add_child(duplicate_a)
        page.root_widget.add_child(duplicate_b)
        page.root_widget.add_child(missing)

        catalog = ResourceCatalog()
        catalog.add_image("ghost.png")
        resource_dir = tmp_path / "resources"
        (resource_dir / "images").mkdir(parents=True)

        entries = analyze_page(page, resource_catalog=catalog, source_resource_dir=str(resource_dir))

        codes = [entry.code for entry in entries]
        assert codes.count("invalid_name") == 1
        assert codes.count("duplicate_name") == 2
        assert codes.count("bounds") == 1
        assert codes.count("missing_resource") == 1
        assert any(entry.code == "missing_resource" and "missing on disk" in entry.message for entry in entries)
        assert all(entry.page_name == "main_page" for entry in entries)
        missing_entry = next(entry for entry in entries if entry.code == "missing_resource")
        assert missing_entry.resource_type == "image"
        assert missing_entry.resource_name == "ghost.png"
        assert missing_entry.property_name == "image_file"

    def test_analyze_page_reports_invalid_page_fields(self):
        page = Page.create_default("main_page", screen_width=240, screen_height=320)
        title = WidgetModel("label", name="title", x=8, y=8, width=60, height=20)
        page.root_widget.add_child(title)
        page.user_fields = [
            {"name": "title", "type": "int"},
            {"name": "counter", "type": "int"},
            {"name": "counter", "type": "uint32_t"},
            {"name": "bad-name", "type": "int"},
        ]

        entries = analyze_page(page)

        codes = [entry.code for entry in entries]
        assert codes.count("page_field_conflict") == 1
        assert codes.count("page_field_duplicate_name") == 2
        assert codes.count("page_field_invalid_name") == 1
        assert any("auto-generated page member" in entry.message for entry in entries)
        assert any("already exists in this page" in entry.message for entry in entries)

    def test_analyze_page_reports_invalid_page_timers(self):
        page = Page.create_default("main_page", screen_width=240, screen_height=320)
        title = WidgetModel("label", name="title", x=8, y=8, width=60, height=20)
        page.root_widget.add_child(title)
        page.user_fields = [{"name": "counter", "type": "int"}]
        page.timers = [
            {"name": "title", "callback": "tick_title", "delay_ms": "1000", "period_ms": "1000"},
            {"name": "refresh_timer", "callback": "", "delay_ms": "1000", "period_ms": "1000"},
        ]

        entries = analyze_page(page)

        codes = [entry.code for entry in entries]
        assert codes.count("page_timer_conflict") == 1
        assert codes.count("page_timer_missing_callback") == 1
        assert any("callback function name" in entry.message for entry in entries)

    def test_analyze_page_reports_callback_signature_conflicts(self):
        page = Page.create_default("main_page", screen_width=240, screen_height=320)
        button = WidgetModel("button", name="confirm_button", x=8, y=8, width=80, height=28)
        slider = WidgetModel("slider", name="volume_slider", x=8, y=48, width=120, height=24)
        button.on_click = "on_shared_action"
        slider.events["onValueChanged"] = "on_shared_action"
        page.root_widget.add_child(button)
        page.root_widget.add_child(slider)

        entries = analyze_page(page)

        assert [entry.code for entry in entries] == ["callback_signature_conflict"]
        assert "on_shared_action" in entries[0].message
        assert "confirm_button.onClick" in entries[0].message
        assert "volume_slider.onValueChanged" in entries[0].message

    def test_analyze_page_reports_missing_string_key_references(self):
        page = Page.create_default("main_page", screen_width=240, screen_height=320)
        title = WidgetModel("label", name="title", x=8, y=8, width=60, height=20)
        title.properties["text"] = "@string/missing_key"
        page.root_widget.add_child(title)

        string_catalog = StringResourceCatalog()
        string_catalog.set("greeting", "Hello")

        entries = analyze_page(page, string_catalog=string_catalog)

        assert [entry.code for entry in entries] == ["missing_string_resource"]
        assert "missing_key" in entries[0].message
        assert entries[0].resource_type == "string"
        assert entries[0].resource_name == "missing_key"
        assert entries[0].property_name == "text"


class TestSelectionDiagnostics:
    def test_analyze_selection_reports_locked_hidden_and_layout_managed(self):
        layout_parent = WidgetModel("linearlayout", name="layout_parent", x=0, y=0, width=240, height=80)
        managed = WidgetModel("label", name="managed_widget", x=12, y=8, width=80, height=20)
        managed.designer_locked = True
        managed.designer_hidden = True
        layout_parent.add_child(managed)

        entries = analyze_selection([managed])

        codes = [entry.code for entry in entries]
        assert codes == [
            "selection_locked",
            "selection_hidden",
            "selection_layout_managed",
        ]
        assert entries[0].severity == "info"
        assert "canvas drag and resize are disabled" in entries[0].message
        assert "canvas hit testing" in entries[1].message
        assert "layout-managed by linearlayout" in entries[2].message
