"""Tests for ui_designer.model.project.Project."""

import os

import pytest

from ui_designer.model.project import Project
from ui_designer.model.page import Page
from ui_designer.model.widget_model import WidgetModel
from ui_designer.model.workspace import normalize_path


class TestProjectDefaults:
    """Tests for default Project construction."""

    def test_create_defaults(self):
        proj = Project()
        assert proj.screen_width == 240
        assert proj.screen_height == 320
        assert proj.app_name == "HelloDesigner"
        assert proj.sdk_root == ""
        assert proj.egui_root == ""
        assert proj.project_dir == ""
        assert proj.page_mode == "easy_page"
        assert proj.startup_page == "main_page"
        assert proj.pages == []
        assert proj.resource_catalog is not None
        assert proj.string_catalog is not None


class TestPageManagement:
    """Tests for add_page, remove_page, get_page_by_name."""

    def test_add_page(self, simple_page):
        proj = Project()
        proj.add_page(simple_page)
        assert len(proj.pages) == 1
        assert proj.pages[0] is simple_page

    def test_remove_page(self, simple_page):
        proj = Project()
        proj.add_page(simple_page)
        assert len(proj.pages) == 1

        proj.remove_page(simple_page)
        assert len(proj.pages) == 0

    def test_get_page_by_name(self, simple_page):
        proj = Project()
        proj.add_page(simple_page)

        found = proj.get_page_by_name("main_page")
        assert found is simple_page

    def test_get_page_by_name_not_found(self, simple_page):
        proj = Project()
        proj.add_page(simple_page)

        found = proj.get_page_by_name("nonexistent_page")
        assert found is None


class TestStartupPage:
    """Tests for startup page resolution."""

    def test_get_startup_page(self, simple_project):
        startup = simple_project.get_startup_page()
        assert startup is not None
        assert startup.name == "main_page"

    def test_get_startup_page_fallback(self):
        proj = Project()
        proj.startup_page = "nonexistent_page"

        root = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
        fallback_page = Page(file_path="layout/fallback.xml", root_widget=root)
        proj.add_page(fallback_page)

        startup = proj.get_startup_page()
        assert startup is fallback_page


class TestCreateNewPage:
    """Tests for create_new_page."""

    def test_create_new_page(self):
        proj = Project(screen_width=320, screen_height=480)
        page = proj.create_new_page("settings")

        assert page is not None
        assert page.name == "settings"
        assert page.root_widget is not None
        assert page.root_widget.widget_type == "group"
        assert page.root_widget.width == 320
        assert page.root_widget.height == 480
        assert page in proj.pages

    def test_duplicate_page_copies_page_content(self):
        proj = Project(screen_width=320, screen_height=480)

        original = proj.create_new_page("settings")
        label = WidgetModel("label", name="title", x=12, y=18, width=180, height=32)
        label.properties["text"] = "Settings"
        original.root_widget.add_child(label)
        original.user_fields.append({"name": "counter", "type": "int", "default": 3})
        original.mockup_image_path = "mockup/settings.png"
        original.mockup_image_visible = False
        original.mockup_image_opacity = 0.5

        duplicated = proj.duplicate_page("settings", "settings_copy")

        assert duplicated is not original
        assert duplicated.name == "settings_copy"
        assert duplicated.file_path == "layout/settings_copy.xml"
        assert duplicated.dirty is True
        assert duplicated.root_widget is not original.root_widget
        assert len(duplicated.root_widget.children) == 1
        assert duplicated.root_widget.children[0].name == "title"
        assert duplicated.root_widget.children[0].properties["text"] == "Settings"
        assert duplicated.user_fields == [{"name": "counter", "type": "int", "default": "3"}]
        assert duplicated.mockup_image_path == "mockup/settings.png"
        assert duplicated.mockup_image_visible is False
        assert duplicated.mockup_image_opacity == 0.5
        assert duplicated in proj.pages


class TestRootWidgets:
    """Tests for root_widgets compatibility property."""

    def test_root_widgets(self, simple_project):
        widgets = simple_project.root_widgets
        assert len(widgets) == 1
        assert widgets[0].widget_type == "group"


class TestPathHelpers:
    """Tests for get_app_dir, get_resource_dir, get_eguiproject_dir."""

    def test_get_app_dir(self):
        proj = Project(app_name="TestApp")
        proj.egui_root = "/home/user/EmbeddedGUI"

        expected = os.path.join(normalize_path("/home/user/EmbeddedGUI"), "example", "TestApp")
        assert proj.get_app_dir() == expected

    def test_get_app_dir_empty_root(self):
        proj = Project(app_name="TestApp")
        assert proj.get_app_dir() == ""

    def test_get_resource_dir(self):
        proj = Project(app_name="TestApp")
        proj.egui_root = "/home/user/EmbeddedGUI"

        expected = os.path.join(normalize_path("/home/user/EmbeddedGUI"), "example", "TestApp", "resource")
        assert proj.get_resource_dir() == expected

    def test_get_eguiproject_dir(self):
        proj = Project(app_name="TestApp")
        proj.egui_root = "/home/user/EmbeddedGUI"

        expected = os.path.join(normalize_path("/home/user/EmbeddedGUI"), "example", "TestApp", ".eguiproject")
        assert proj.get_eguiproject_dir() == expected

    def test_get_app_dir_prefers_project_dir(self):
        proj = Project(app_name="TestApp")
        proj.project_dir = normalize_path("/workspace/TestApp")
        proj.egui_root = "/home/user/EmbeddedGUI"
        assert proj.get_app_dir() == normalize_path("/workspace/TestApp")


class TestGetAllWidgets:
    """Tests for get_all_widgets across multiple pages."""

    def test_get_all_widgets(self, multi_page_project):
        all_widgets = multi_page_project.get_all_widgets()

        # Page1: root_group + title label = 2
        # Page2: root_group + back_btn = 2
        # Total = 4
        assert len(all_widgets) == 4


class TestSaveLoad:
    """Tests for save/load file I/O."""

    @pytest.mark.integration
    def test_save_load_round_trip(self, tmp_path):
        proj = Project(screen_width=320, screen_height=480, app_name="RoundTripApp")
        proj.sdk_root = str(tmp_path / "sdk")
        proj.startup_page = "home"

        root1 = WidgetModel("group", name="root_group", x=0, y=0, width=320, height=480)
        label1 = WidgetModel("label", name="title", x=10, y=10, width=200, height=30)
        label1.properties["text"] = "Home Page"
        root1.add_child(label1)
        page1 = Page(file_path="layout/home.xml", root_widget=root1)

        root2 = WidgetModel("group", name="root_group", x=0, y=0, width=320, height=480)
        page2 = Page(file_path="layout/about.xml", root_widget=root2)

        proj.add_page(page1)
        proj.add_page(page2)

        project_dir = str(tmp_path / "RoundTripApp")
        proj.save(project_dir)

        loaded = Project.load(project_dir)

        assert loaded.screen_width == 320
        assert loaded.screen_height == 480
        assert loaded.app_name == "RoundTripApp"
        assert loaded.project_dir == normalize_path(project_dir)
        assert loaded.sdk_root == normalize_path(str(tmp_path / "sdk"))
        assert loaded.startup_page == "home"
        assert len(loaded.pages) == 2

        home_page = loaded.get_page_by_name("home")
        assert home_page is not None
        assert len(home_page.root_widget.children) == 1
        assert home_page.root_widget.children[0].properties["text"] == "Home Page"

    @pytest.mark.integration
    def test_save_creates_files(self, tmp_path):
        proj = Project(app_name="FileCheckApp")

        root = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
        page = Page(file_path="layout/main_page.xml", root_widget=root)
        proj.add_page(page)

        project_dir = str(tmp_path / "FileCheckApp")
        proj.save(project_dir)

        # Verify .egui project file exists
        egui_file = os.path.join(project_dir, "FileCheckApp.egui")
        assert os.path.isfile(egui_file)

        # Verify layout XML was created
        layout_xml = os.path.join(project_dir, ".eguiproject", "layout", "main_page.xml")
        assert os.path.isfile(layout_xml)
