"""Shared fixtures for ui_designer tests."""

import os
import sys
import pytest

# Ensure the scripts directory is on sys.path so `ui_designer` package imports work
_TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
_UI_DESIGNER_DIR = os.path.dirname(_TESTS_DIR)
_SCRIPTS_DIR = os.path.dirname(_UI_DESIGNER_DIR)
if _SCRIPTS_DIR not in sys.path:
    sys.path.insert(0, _SCRIPTS_DIR)

TEST_DATA_DIR = os.path.join(_TESTS_DIR, "test_data")


@pytest.fixture
def test_data_dir():
    """Path to the test_data/ directory with static fixtures."""
    return TEST_DATA_DIR


@pytest.fixture(autouse=True)
def reset_widget_counter():
    """Reset WidgetModel._counter before each test to avoid state leaks."""
    from ui_designer.model.widget_model import WidgetModel
    WidgetModel.reset_counter()
    yield
    WidgetModel.reset_counter()


@pytest.fixture
def simple_label():
    """Create a basic label widget."""
    from ui_designer.model.widget_model import WidgetModel
    return WidgetModel("label", name="test_label", x=10, y=20, width=100, height=30)


@pytest.fixture
def simple_button():
    """Create a basic button widget."""
    from ui_designer.model.widget_model import WidgetModel
    return WidgetModel("button", name="test_button", x=0, y=0, width=80, height=40)


@pytest.fixture
def simple_image():
    """Create a basic image widget with an image file set."""
    from ui_designer.model.widget_model import WidgetModel
    w = WidgetModel("image", name="test_image", x=0, y=0, width=64, height=64)
    w.properties["image_file"] = "star.png"
    w.properties["image_format"] = "rgb565"
    w.properties["image_alpha"] = "4"
    return w


@pytest.fixture
def container_group():
    """Create a group container with two children."""
    from ui_designer.model.widget_model import WidgetModel
    group = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
    label = WidgetModel("label", name="child_label", x=10, y=10, width=100, height=30)
    button = WidgetModel("button", name="child_button", x=10, y=50, width=100, height=40)
    group.add_child(label)
    group.add_child(button)
    return group


@pytest.fixture
def linearlayout_vertical():
    """Create a vertical LinearLayout with 3 children."""
    from ui_designer.model.widget_model import WidgetModel
    layout = WidgetModel("linearlayout", name="ll_v", x=0, y=0, width=200, height=300)
    layout.properties["orientation"] = "vertical"
    layout.properties["align_type"] = "EGUI_ALIGN_CENTER"
    for i in range(3):
        child = WidgetModel("label", name=f"item_{i}", x=0, y=0, width=100, height=40)
        layout.add_child(child)
    return layout


@pytest.fixture
def linearlayout_horizontal():
    """Create a horizontal LinearLayout with 3 children."""
    from ui_designer.model.widget_model import WidgetModel
    layout = WidgetModel("linearlayout", name="ll_h", x=0, y=0, width=300, height=100)
    layout.properties["orientation"] = "horizontal"
    layout.properties["align_type"] = "EGUI_ALIGN_CENTER"
    for i in range(3):
        child = WidgetModel("label", name=f"item_{i}", x=0, y=0, width=80, height=30)
        layout.add_child(child)
    return layout


@pytest.fixture
def simple_page():
    """Create a Page with a root group containing one label."""
    from ui_designer.model.widget_model import WidgetModel
    from ui_designer.model.page import Page
    root = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
    label = WidgetModel("label", name="my_label", x=10, y=10, width=100, height=30)
    label.properties["text"] = "Hello"
    root.add_child(label)
    page = Page(file_path="layout/main_page.xml", root_widget=root)
    return page


@pytest.fixture
def simple_project(simple_page):
    """Create a Project with one page."""
    from ui_designer.model.project import Project
    proj = Project(screen_width=240, screen_height=320, app_name="TestApp")
    proj.add_page(simple_page)
    proj.startup_page = "main_page"
    return proj


@pytest.fixture
def multi_page_project():
    """Create a Project with two pages for multi-page testing."""
    from ui_designer.model.widget_model import WidgetModel
    from ui_designer.model.page import Page
    from ui_designer.model.project import Project

    proj = Project(screen_width=240, screen_height=320, app_name="MultiPageApp")

    root1 = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
    label1 = WidgetModel("label", name="title", x=10, y=10, width=220, height=30)
    label1.properties["text"] = "Page One"
    root1.add_child(label1)
    page1 = Page(file_path="layout/main_page.xml", root_widget=root1)

    root2 = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
    btn = WidgetModel("button", name="back_btn", x=10, y=10, width=100, height=40)
    btn.properties["text"] = "Back"
    btn.on_click = "on_back_click"
    root2.add_child(btn)
    page2 = Page(file_path="layout/settings.xml", root_widget=root2)

    proj.add_page(page1)
    proj.add_page(page2)
    proj.startup_page = "main_page"
    return proj


@pytest.fixture
def string_catalog():
    """Create a StringResourceCatalog with default + Chinese locale."""
    from ui_designer.model.string_resource import StringResourceCatalog
    cat = StringResourceCatalog()
    cat.set("app_name", "My App", "")
    cat.set("greeting", "Hello", "")
    cat.set("app_name", "My App ZH", "zh")
    cat.set("greeting", "Ni Hao", "zh")
    return cat


@pytest.fixture
def tmp_project_dir(tmp_path):
    """Create a temporary directory structure simulating a project dir."""
    return tmp_path
