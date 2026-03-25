"""Tests for ui_designer.model.resource_usage."""

from ui_designer.model.project import Project
from ui_designer.model.resource_usage import (
    collect_page_resource_usages,
    collect_project_resource_usages,
    find_resource_usages,
    rewrite_project_resource_references,
    rewrite_project_string_references,
)
from ui_designer.model.widget_model import WidgetModel


class TestResourceUsage:
    def test_collect_page_resource_usages_tracks_all_resource_property_types(self):
        project = Project(app_name="UsageDemo")
        page = project.create_new_page("main_page")

        image = WidgetModel("image", name="hero")
        image.properties["image_file"] = "hero.png"
        page.root_widget.add_child(image)

        label = WidgetModel("label", name="title")
        label.properties["font_file"] = "demo.ttf"
        label.properties["font_text_file"] = "chars.txt"
        page.root_widget.add_child(label)

        usages = collect_page_resource_usages(page)
        summary = {(entry.resource_type, entry.resource_name, entry.widget_name, entry.property_name) for entry in usages}

        assert summary == {
            ("image", "hero.png", "hero", "image_file"),
            ("font", "demo.ttf", "title", "font_file"),
            ("text", "chars.txt", "title", "font_text_file"),
        }

    def test_collect_project_resource_usages_groups_entries_by_resource_name(self):
        project = Project(app_name="UsageDemo")
        main_page = project.create_new_page("main_page")
        detail_page = project.create_new_page("detail_page")

        label_a = WidgetModel("label", name="title")
        label_a.properties["font_file"] = "demo.ttf"
        main_page.root_widget.add_child(label_a)

        label_b = WidgetModel("label", name="subtitle")
        label_b.properties["font_file"] = "demo.ttf"
        detail_page.root_widget.add_child(label_b)

        image = WidgetModel("image", name="hero")
        image.properties["image_file"] = "hero.png"
        detail_page.root_widget.add_child(image)

        usage_index = collect_project_resource_usages(project)
        font_usages = usage_index[("font", "demo.ttf")]
        image_usages = find_resource_usages(project, "image", "hero.png")

        assert [(entry.page_name, entry.widget_name) for entry in font_usages] == [
            ("main_page", "title"),
            ("detail_page", "subtitle"),
        ]
        assert [(entry.page_name, entry.widget_name, entry.property_name) for entry in image_usages] == [
            ("detail_page", "hero", "image_file"),
        ]

    def test_rewrite_project_resource_references_updates_all_matching_widgets(self):
        project = Project(app_name="RewriteDemo")
        main_page = project.create_new_page("main_page")
        detail_page = project.create_new_page("detail_page")

        label_a = WidgetModel("label", name="title")
        label_a.properties["font_file"] = "demo.ttf"
        label_a.properties["font_text_file"] = "chars.txt"
        main_page.root_widget.add_child(label_a)

        label_b = WidgetModel("label", name="subtitle")
        label_b.properties["font_file"] = "demo.ttf"
        label_b.properties["font_text_file"] = "chars.txt"
        detail_page.root_widget.add_child(label_b)

        untouched = WidgetModel("label", name="caption")
        untouched.properties["font_file"] = "demo.ttf"
        untouched.properties["font_text_file"] = "other.txt"
        detail_page.root_widget.add_child(untouched)

        touched_pages, rewrite_count = rewrite_project_resource_references(project, "text", "chars.txt", "chars_new.txt")

        assert [page.name for page in touched_pages] == ["main_page", "detail_page"]
        assert rewrite_count == 2
        assert label_a.properties["font_text_file"] == "chars_new.txt"
        assert label_b.properties["font_text_file"] == "chars_new.txt"
        assert untouched.properties["font_text_file"] == "other.txt"

    def test_rewrite_project_resource_references_can_clear_references(self):
        project = Project(app_name="RewriteDemo")
        page = project.create_new_page("main_page")

        image = WidgetModel("image", name="hero")
        image.properties["image_file"] = "missing.png"
        page.root_widget.add_child(image)

        touched_pages, rewrite_count = rewrite_project_resource_references(project, "image", "missing.png", "")

        assert [entry.name for entry in touched_pages] == ["main_page"]
        assert rewrite_count == 1
        assert image.properties["image_file"] == ""

    def test_collect_and_rewrite_string_references(self):
        project = Project(app_name="StringUsageDemo")
        main_page = project.create_new_page("main_page")
        detail_page = project.create_new_page("detail_page")

        title = WidgetModel("label", name="title")
        title.properties["text"] = "@string/greeting"
        main_page.root_widget.add_child(title)

        subtitle = WidgetModel("label", name="subtitle")
        subtitle.properties["text"] = "@string/greeting"
        detail_page.root_widget.add_child(subtitle)

        usages = find_resource_usages(project, "string", "greeting")
        assert [(entry.page_name, entry.widget_name, entry.property_name) for entry in usages] == [
            ("main_page", "title", "text"),
            ("detail_page", "subtitle", "text"),
        ]

        touched_pages, rewrite_count = rewrite_project_string_references(
            project,
            "greeting",
            replacement_text="Hello",
        )

        assert [page.name for page in touched_pages] == ["main_page", "detail_page"]
        assert rewrite_count == 2
        assert title.properties["text"] == "Hello"
        assert subtitle.properties["text"] == "Hello"

    def test_rewrite_string_references_to_new_key(self):
        project = Project(app_name="StringRenameDemo")
        page = project.create_new_page("main_page")

        title = WidgetModel("label", name="title")
        title.properties["text"] = "@string/greeting"
        page.root_widget.add_child(title)

        touched_pages, rewrite_count = rewrite_project_string_references(
            project,
            "greeting",
            new_key="salutation",
        )

        assert [item.name for item in touched_pages] == ["main_page"]
        assert rewrite_count == 1
        assert title.properties["text"] == "@string/salutation"
