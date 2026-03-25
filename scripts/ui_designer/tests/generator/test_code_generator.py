"""Tests for ui_designer.generator.code_generator module."""

import pytest

from ui_designer.model.widget_model import WidgetModel, BackgroundModel, AnimationModel
from ui_designer.model.page import Page
from ui_designer.model.project import Project
from ui_designer.generator.code_generator import (
    _simple_init_func,
    _bg_macro_name,
    _gen_bg_param_init,
    _upper_guard,
    _gen_widget_init_lines,
    generate_page_header,
    generate_page_layout_source,
    generate_page_user_source,
    generate_uicode_header,
    generate_uicode_source,
    generate_all_files,
    GENERATED_ALWAYS,
    GENERATED_PRESERVED,
    USER_OWNED,
)


# ── Fixture helpers ───────────────────────────────────────────────


def _make_page(name="main_page", root=None):
    """Create a Page with the given name and optional root widget."""
    if root is None:
        root = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
    return Page(file_path=f"layout/{name}.xml", root_widget=root)


def _make_project(pages=None, page_mode="easy_page", startup="main_page"):
    """Create a minimal Project."""
    proj = Project(screen_width=240, screen_height=320, app_name="TestApp")
    proj.page_mode = page_mode
    proj.startup_page = startup
    if pages:
        for p in pages:
            proj.add_page(p)
    return proj


def _make_bg(bg_type="solid", color="EGUI_COLOR_WHITE", alpha="EGUI_ALPHA_100",
             radius=0, stroke_width=0, stroke_color="EGUI_COLOR_BLACK",
             stroke_alpha="EGUI_ALPHA_100"):
    """Create a BackgroundModel with specified properties."""
    bg = BackgroundModel()
    bg.bg_type = bg_type
    bg.color = color
    bg.alpha = alpha
    bg.radius = radius
    bg.stroke_width = stroke_width
    bg.stroke_color = stroke_color
    bg.stroke_alpha = stroke_alpha
    return bg


# ======================================================================
# TestHelpers
# ======================================================================


class TestHelpers:
    """Tests for internal helper functions."""

    def test_simple_init_func_label(self):
        result = _simple_init_func("label")
        assert result == "egui_view_label_init"

    def test_simple_init_func_button(self):
        result = _simple_init_func("button")
        assert result == "egui_view_button_init"

    def test_simple_init_func_unknown(self):
        result = _simple_init_func("unknown")
        assert result == ""

    def test_bg_macro_name_solid(self):
        assert _bg_macro_name("solid") == "SOLID"

    def test_bg_macro_name_round_rectangle(self):
        assert _bg_macro_name("round_rectangle") == "ROUND_RECTANGLE"

    def test_bg_macro_name_round_rectangle_corners(self):
        assert _bg_macro_name("round_rectangle_corners") == "ROUND_RECTANGLE_CORNERS"

    def test_bg_macro_name_circle(self):
        assert _bg_macro_name("circle") == "CIRCLE"

    def test_bg_macro_name_unknown_fallback(self):
        assert _bg_macro_name("xxx") == "SOLID"

    def test_upper_guard(self):
        assert _upper_guard("main_page") == "_MAIN_PAGE_H_"

    def test_gen_bg_param_init_solid(self):
        bg = _make_bg(bg_type="solid", color="EGUI_COLOR_WHITE", alpha="EGUI_ALPHA_100")
        result = _gen_bg_param_init("my_param", bg)
        assert "EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID" in result
        assert "my_param" in result
        assert "EGUI_COLOR_WHITE" in result
        assert "EGUI_ALPHA_100" in result

    def test_gen_bg_param_init_round_rectangle(self):
        bg = _make_bg(bg_type="round_rectangle", radius=8)
        result = _gen_bg_param_init("rr_param", bg)
        assert "EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE" in result
        assert "rr_param" in result
        assert "8" in result

    def test_gen_bg_param_init_solid_with_stroke(self):
        bg = _make_bg(bg_type="solid", stroke_width=2,
                      stroke_color="EGUI_COLOR_RED", stroke_alpha="EGUI_ALPHA_100")
        result = _gen_bg_param_init("stroke_param", bg)
        assert "EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID_STROKE" in result
        assert "stroke_param" in result
        assert "2" in result
        assert "EGUI_COLOR_RED" in result


# ======================================================================
# TestGenWidgetInitLines
# ======================================================================


class TestGenWidgetInitLines:
    """Tests for _gen_widget_init_lines."""

    def test_label_basic(self):
        w = WidgetModel("label", name="lbl", x=10, y=20, width=100, height=30)
        w.properties["text"] = "Hello"
        w.properties["align_type"] = "EGUI_ALIGN_LEFT"  # non-default to trigger setter
        lines = _gen_widget_init_lines(w)
        text = "\n".join(lines)
        assert "egui_view_label_init" in text
        assert "set_position" in text
        assert "set_size" in text
        assert "set_text" in text
        assert '"Hello"' in text
        assert "set_align_type" in text
        assert "set_font" in text
        assert "set_font_color" in text

    def test_label_with_string_ref(self):
        w = WidgetModel("label", name="lbl", x=0, y=0, width=100, height=30)
        w.properties["text"] = "@string/hello"
        lines = _gen_widget_init_lines(w)
        text = "\n".join(lines)
        assert "egui_i18n_get(EGUI_STR_HELLO)" in text

    def test_button_init_lines(self):
        w = WidgetModel("button", name="btn", x=0, y=0, width=80, height=40)
        w.properties["text"] = "Click"
        lines = _gen_widget_init_lines(w)
        text = "\n".join(lines)
        assert "egui_view_button_init" in text
        assert "set_text" in text
        assert '"Click"' in text

    def test_image_init_lines(self):
        w = WidgetModel("image", name="img", x=0, y=0, width=64, height=64)
        w.properties["image_file"] = "star.png"
        w.properties["image_format"] = "rgb565"
        w.properties["image_alpha"] = "4"
        lines = _gen_widget_init_lines(w)
        text = "\n".join(lines)
        assert "egui_view_image_set_image" in text
        assert "egui_res_image_star_rgb565_4" in text

    def test_image_null_when_no_file(self):
        w = WidgetModel("image", name="img", x=0, y=0, width=64, height=64)
        w.properties["image_file"] = ""
        lines = _gen_widget_init_lines(w)
        text = "\n".join(lines)
        assert "set_image" not in text

    def test_linearlayout_init_lines(self):
        w = WidgetModel("linearlayout", name="ll", x=0, y=0, width=200, height=300)
        w.properties["align_type"] = "EGUI_ALIGN_LEFT"  # non-default to trigger setter
        lines = _gen_widget_init_lines(w)
        text = "\n".join(lines)
        assert "set_align_type" in text
        assert "EGUI_ALIGN_LEFT" in text

    def test_linearlayout_horizontal(self):
        w = WidgetModel("linearlayout", name="ll", x=0, y=0, width=200, height=100)
        w.properties["orientation"] = "horizontal"
        lines = _gen_widget_init_lines(w)
        text = "\n".join(lines)
        assert "set_orientation" in text
        assert "1" in text

    def test_switch_checked(self):
        w = WidgetModel("switch", name="sw", x=0, y=0, width=60, height=30)
        w.properties["is_checked"] = True
        lines = _gen_widget_init_lines(w)
        text = "\n".join(lines)
        assert "set_checked" in text

    def test_progress_bar_value(self):
        w = WidgetModel("progress_bar", name="pb", x=0, y=0, width=200, height=20)
        w.properties["value"] = 75
        lines = _gen_widget_init_lines(w)
        text = "\n".join(lines)
        assert "set_process" in text
        assert "75" in text

    def test_widget_with_margin(self):
        w = WidgetModel("label", name="lbl", x=0, y=0, width=100, height=30)
        w.margin = 5
        lines = _gen_widget_init_lines(w)
        text = "\n".join(lines)
        assert "set_margin_all" in text
        assert "5" in text

    def test_widget_with_on_click(self):
        w = WidgetModel("button", name="btn", x=0, y=0, width=80, height=40)
        w.on_click = "my_cb"
        lines = _gen_widget_init_lines(w)
        text = "\n".join(lines)
        assert "set_on_click_listener" in text
        assert "my_cb" in text


# ======================================================================
# TestGeneratePageHeader
# ======================================================================


class TestGeneratePageHeader:
    """Tests for generate_page_header."""

    def _make_simple_page_and_project(self):
        root = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
        label = WidgetModel("label", name="my_label", x=10, y=10, width=100, height=30)
        root.add_child(label)
        page = _make_page("main_page", root)
        proj = _make_project([page])
        return page, proj

    def test_header_guard(self):
        page, proj = self._make_simple_page_and_project()
        output = generate_page_header(page, proj)
        assert "#ifndef _MAIN_PAGE_H_" in output
        assert "#define _MAIN_PAGE_H_" in output
        assert "#endif /* _MAIN_PAGE_H_ */" in output

    def test_header_struct_definition(self):
        page, proj = self._make_simple_page_and_project()
        output = generate_page_header(page, proj)
        assert "egui_page_base_t base;" in output
        assert "egui_view_group_t root_group;" in output
        assert "egui_view_label_t my_label;" in output

    def test_header_user_code_regions(self):
        page, proj = self._make_simple_page_and_project()
        output = generate_page_header(page, proj)
        assert "// USER CODE BEGIN includes" in output
        assert "// USER CODE END includes" in output
        assert "// USER CODE BEGIN user_fields" in output
        assert "// USER CODE END user_fields" in output
        assert "// USER CODE BEGIN declarations" in output
        assert "// USER CODE END declarations" in output

    def test_header_function_declarations(self):
        page, proj = self._make_simple_page_and_project()
        output = generate_page_header(page, proj)
        assert "egui_main_page_layout_init" in output
        assert "egui_main_page_init" in output

    def test_header_extern_c(self):
        page, proj = self._make_simple_page_and_project()
        output = generate_page_header(page, proj)
        assert "#ifdef __cplusplus" in output
        assert 'extern "C" {' in output

    def test_header_includes_generated_page_fields_before_user_code_region(self):
        page, proj = self._make_simple_page_and_project()
        page.user_fields = [
            {"name": "counter", "type": "int", "default": "0"},
            {"name": "buffer", "type": "uint8_t[16]"},
        ]

        output = generate_page_header(page, proj)

        assert "    // Page fields (auto-generated from Designer metadata)" in output
        assert "    int counter;" in output
        assert "    uint8_t buffer[16];" in output
        assert output.index("    int counter;") < output.index("    // USER CODE BEGIN user_fields")

    def test_header_skips_invalid_or_conflicting_page_fields(self):
        page, proj = self._make_simple_page_and_project()
        page.user_fields = [
            {"name": "my_label", "type": "int"},
            {"name": "bad-name", "type": "int"},
            {"name": "counter", "type": "int"},
            {"name": "counter", "type": "uint32_t"},
        ]

        output = generate_page_header(page, proj)

        assert "    int my_label;" not in output
        assert "bad-name" not in output
        assert "    int counter;" not in output
        assert "    uint32_t counter;" not in output

    def test_header_contains_page_timer_members_and_helpers(self):
        page, proj = self._make_simple_page_and_project()
        page.timers = [
            {
                "name": "refresh_timer",
                "callback": "tick_refresh",
                "delay_ms": "500",
                "period_ms": "1000",
                "auto_start": True,
            }
        ]

        output = generate_page_header(page, proj)

        assert "    egui_timer_t refresh_timer;" in output
        assert "void egui_main_page_timers_init(egui_page_base_t *self);" in output
        assert "void egui_main_page_timers_start_auto(egui_page_base_t *self);" in output
        assert "void egui_main_page_timers_stop(egui_page_base_t *self);" in output


# ======================================================================
# TestGeneratePageLayoutSource
# ======================================================================


class TestGeneratePageLayoutSource:
    """Tests for generate_page_layout_source."""

    def _make_page_with_children(self):
        root = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
        label = WidgetModel("label", name="title", x=10, y=10, width=100, height=30)
        label.properties["text"] = "Title"
        root.add_child(label)
        page = _make_page("main_page", root)
        proj = _make_project([page])
        return page, proj

    def test_layout_source_auto_generated_header(self):
        page, proj = self._make_page_with_children()
        output = generate_page_layout_source(page, proj)
        assert "DO NOT EDIT" in output

    def test_layout_source_init_calls(self):
        page, proj = self._make_page_with_children()
        output = generate_page_layout_source(page, proj)
        assert "egui_view_group_init" in output
        assert "egui_view_label_init" in output

    def test_layout_source_hierarchy(self):
        page, proj = self._make_page_with_children()
        output = generate_page_layout_source(page, proj)
        assert "egui_view_group_add_child" in output
        assert "local->root_group" in output
        assert "local->title" in output

    def test_layout_source_background(self):
        root = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
        root.background = _make_bg(bg_type="solid", color="EGUI_COLOR_BLUE")
        page = _make_page("main_page", root)
        proj = _make_project([page])
        output = generate_page_layout_source(page, proj)
        assert "egui_background_color_t" in output
        assert "EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID" in output

    def test_layout_source_i18n_includes(self):
        root = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
        label = WidgetModel("label", name="lbl", x=0, y=0, width=100, height=30)
        label.properties["text"] = "@string/greeting"
        root.add_child(label)
        page = _make_page("main_page", root)
        proj = _make_project([page])
        output = generate_page_layout_source(page, proj)
        assert '#include "egui_strings.h"' in output

    def test_layout_source_page_root_add_view(self):
        page, proj = self._make_page_with_children()
        output = generate_page_layout_source(page, proj)
        assert "egui_page_base_add_view" in output

    def test_layout_source_initializes_scalar_page_fields_and_skips_arrays(self):
        page, proj = self._make_page_with_children()
        page.user_fields = [
            {"name": "counter", "type": "int", "default": "7"},
            {"name": "title_text", "type": "char *", "default": "\"ready\""},
            {"name": "buffer", "type": "uint8_t[16]", "default": "{0}"},
        ]

        output = generate_page_layout_source(page, proj)

        assert "    // Initialize page fields" in output
        assert "    local->counter = 7;" in output
        assert '    local->title_text = "ready";' in output
        assert "local->buffer =" not in output

    def test_layout_source_skips_invalid_page_field_initializers(self):
        page, proj = self._make_page_with_children()
        page.user_fields = [
            {"name": "title", "type": "int", "default": "1"},
            {"name": "bad-name", "type": "int", "default": "2"},
            {"name": "counter", "type": "int", "default": "3"},
            {"name": "counter", "type": "uint32_t", "default": "4"},
        ]

        output = generate_page_layout_source(page, proj)

        assert "local->title =" not in output
        assert "bad-name" not in output
        assert "local->counter =" not in output

    def test_layout_source_emits_page_timer_helpers(self):
        page, proj = self._make_page_with_children()
        page.timers = [
            {
                "name": "refresh_timer",
                "callback": "tick_refresh",
                "delay_ms": "500",
                "period_ms": "1000",
                "auto_start": True,
            }
        ]

        output = generate_page_layout_source(page, proj)

        assert "extern void tick_refresh(egui_timer_t *timer);" in output
        assert "void egui_main_page_timers_init(egui_page_base_t *self)" in output
        assert "egui_timer_init_timer(&local->refresh_timer, (void *)local, tick_refresh);" in output
        assert "void egui_main_page_timers_start_auto(egui_page_base_t *self)" in output
        assert "egui_timer_start_timer(&local->refresh_timer, 500, 1000);" in output
        assert "void egui_main_page_timers_stop(egui_page_base_t *self)" in output
        assert "egui_timer_stop_timer(&local->refresh_timer);" in output


# ======================================================================
# TestGeneratePageUserSource
# ======================================================================


class TestGeneratePageUserSource:
    """Tests for generate_page_user_source."""

    def _make_simple(self):
        root = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
        page = _make_page("main_page", root)
        proj = _make_project([page])
        return page, proj

    def test_user_source_on_open(self):
        page, proj = self._make_simple()
        output = generate_page_user_source(page, proj)
        assert "egui_main_page_on_open" in output

    def test_user_source_layout_init_call(self):
        page, proj = self._make_simple()
        output = generate_page_user_source(page, proj)
        assert "egui_main_page_layout_init" in output

    def test_user_source_vtable(self):
        page, proj = self._make_simple()
        output = generate_page_user_source(page, proj)
        assert "EGUI_VIEW_API_TABLE_NAME" in output

    def test_user_source_contains_user_code_regions(self):
        page, proj = self._make_simple()
        output = generate_page_user_source(page, proj)

        assert "// USER CODE BEGIN includes" in output
        assert "// USER CODE BEGIN callbacks" in output
        assert "// USER CODE BEGIN on_open" in output
        assert "// USER CODE BEGIN on_close" in output
        assert "// USER CODE BEGIN on_key_pressed" in output
        assert "// USER CODE BEGIN init" in output

    def test_user_source_wires_timer_helpers_and_callback_stubs(self):
        page, proj = self._make_simple()
        page.timers = [
            {
                "name": "refresh_timer",
                "callback": "tick_refresh",
                "delay_ms": "500",
                "period_ms": "1000",
                "auto_start": True,
            }
        ]

        output = generate_page_user_source(page, proj)

        assert "static void tick_refresh(egui_timer_t *timer)" in output
        assert "egui_main_page_timers_start_auto(self);" in output
        assert "egui_main_page_timers_stop(self);" in output
        assert "egui_main_page_timers_init(self);" in output


# ======================================================================
# TestGenerateUicode
# ======================================================================


class TestGenerateUicode:
    """Tests for generate_uicode_header and generate_uicode_source."""

    def test_uicode_header_page_enum(self):
        page = _make_page("main_page")
        proj = _make_project([page])
        output = generate_uicode_header(proj)
        assert "PAGE_MAIN_PAGE = 0" in output
        assert "PAGE_COUNT" in output

    def test_uicode_header_multi_page(self):
        p1 = _make_page("main_page")
        p2 = _make_page("settings")
        proj = _make_project([p1, p2])
        output = generate_uicode_header(proj)
        assert "PAGE_MAIN_PAGE = 0" in output
        assert "PAGE_SETTINGS = 1" in output
        assert "PAGE_COUNT = 2" in output

    def test_uicode_source_easy_page(self):
        page = _make_page("main_page")
        proj = _make_project([page], page_mode="easy_page")
        output = generate_uicode_source(proj)
        assert "union page_array" in output
        assert "switch (page_index)" in output
        assert "egui_main_page_init" in output

    def test_uicode_source_activity(self):
        page = _make_page("main_page")
        proj = _make_project([page], page_mode="activity")
        output = generate_uicode_source(proj)
        assert "main_page_activity" in output
        assert "egui_activity_t base" in output
        assert "main_page_activity_on_create" in output

    def test_uicode_startup_page_index(self):
        p1 = _make_page("page_a")
        p2 = _make_page("page_b")
        proj = _make_project([p1, p2], startup="page_b")
        output = generate_uicode_source(proj)
        # startup is page_b which is index 1
        assert "current_index = 1" in output
        assert "uicode_switch_page(1)" in output


# ======================================================================
# TestGenerateAllFiles
# ======================================================================


class TestGenerateAllFiles:
    """Tests for generate_all_files."""

    def test_all_files_single_page(self):
        root = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
        page = _make_page("main_page", root)
        proj = _make_project([page])
        files = generate_all_files(proj)
        assert "main_page.h" in files
        assert "main_page_layout.c" in files
        assert "main_page.c" in files
        assert "uicode.h" in files
        assert "uicode.c" in files
        assert "app_egui_config.h" in files

    def test_all_files_categories(self):
        root = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
        page = _make_page("main_page", root)
        proj = _make_project([page])
        files = generate_all_files(proj)
        _, cat_h = files["main_page.h"]
        assert cat_h == GENERATED_PRESERVED
        _, cat_layout = files["main_page_layout.c"]
        assert cat_layout == GENERATED_ALWAYS
        _, cat_c = files["main_page.c"]
        assert cat_c == USER_OWNED
        _, cat_uicode_h = files["uicode.h"]
        assert cat_uicode_h == GENERATED_ALWAYS

    def test_all_files_multi_page(self):
        p1 = _make_page("main_page")
        p2 = _make_page("settings")
        proj = _make_project([p1, p2])
        files = generate_all_files(proj)
        assert "main_page.h" in files
        assert "main_page_layout.c" in files
        assert "main_page.c" in files
        assert "settings.h" in files
        assert "settings_layout.c" in files
        assert "settings.c" in files

    def test_all_files_with_i18n(self):
        root = WidgetModel("group", name="root_group", x=0, y=0, width=240, height=320)
        page = _make_page("main_page", root)
        proj = _make_project([page])
        # Add string catalog with strings
        from ui_designer.model.string_resource import StringResourceCatalog
        cat = StringResourceCatalog()
        cat.set("app_name", "My App", "")
        cat.set("greeting", "Hello", "")
        proj.string_catalog = cat
        files = generate_all_files(proj)
        assert "egui_strings.h" in files
        assert "egui_strings.c" in files


# ── TestAnimationCodeGen ─────────────────────────────────────────


class TestAnimationCodeGen:
    """Animation code generation in header and layout source."""

    def test_header_contains_animation_struct(self):
        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        lbl = WidgetModel("label", name="title", x=10, y=10, width=100, height=30)
        anim = AnimationModel()
        anim.anim_type = "alpha"
        anim.duration = 500
        anim.params = {"from_alpha": "0", "to_alpha": "255"}
        lbl.animations.append(anim)
        root.add_child(lbl)
        page = _make_page("anim_page", root)
        proj = _make_project([page])
        header = generate_page_header(page, proj)
        assert "egui_animation_alpha_t anim_title_alpha;" in header

    def test_layout_contains_animation_params(self):
        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        lbl = WidgetModel("label", name="title", x=10, y=10, width=100, height=30)
        anim = AnimationModel()
        anim.anim_type = "alpha"
        anim.duration = 500
        anim.interpolator = "bounce"
        anim.params = {"from_alpha": "0", "to_alpha": "255"}
        lbl.animations.append(anim)
        root.add_child(lbl)
        page = _make_page("anim_page", root)
        proj = _make_project([page])
        layout = generate_page_layout_source(page, proj)
        assert "EGUI_ANIMATION_ALPHA_PARAMS_INIT(anim_title_alpha_params, 0, 255);" in layout
        assert "egui_interpolator_bounce_t anim_title_alpha_interpolator;" in layout
        assert "egui_animation_alpha_init(" in layout
        assert "egui_animation_duration_set(" in layout
        assert "egui_animation_target_view_set(" in layout
        assert "egui_animation_start(" in layout

    def test_layout_translate_animation(self):
        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        btn = WidgetModel("button", name="btn1", x=0, y=0, width=80, height=40)
        anim = AnimationModel()
        anim.anim_type = "translate"
        anim.duration = 1000
        anim.interpolator = "linear"
        anim.repeat_count = -1
        anim.repeat_mode = "reverse"
        anim.params = {"from_x": "0", "to_x": "100", "from_y": "0", "to_y": "0"}
        btn.animations.append(anim)
        root.add_child(btn)
        page = _make_page("move_page", root)
        proj = _make_project([page])
        layout = generate_page_layout_source(page, proj)
        assert "EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_btn1_translate_params, 0, 100, 0, 0);" in layout
        assert "egui_animation_repeat_count_set(" in layout
        assert "EGUI_ANIMATION_REPEAT_MODE_REVERSE" in layout

    def test_no_auto_start(self):
        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        lbl = WidgetModel("label", name="lbl", x=0, y=0, width=100, height=30)
        anim = AnimationModel()
        anim.anim_type = "alpha"
        anim.auto_start = False
        anim.params = {"from_alpha": "0", "to_alpha": "255"}
        lbl.animations.append(anim)
        root.add_child(lbl)
        page = _make_page("no_start", root)
        proj = _make_project([page])
        layout = generate_page_layout_source(page, proj)
        assert "egui_animation_start(" not in layout


# ======================================================================
# TestEventCallbackCodeGen
# ======================================================================


class TestEventCallbackCodeGen:
    """Tests for event callback code generation."""

    def test_slider_event_forward_declaration(self):
        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        slider = WidgetModel("slider", name="vol_slider", x=0, y=0, width=200, height=30)
        slider.events = {"onValueChanged": "on_volume_changed"}
        root.add_child(slider)
        page = _make_page("event_page", root)
        proj = _make_project([page])
        layout = generate_page_layout_source(page, proj)
        assert "extern void on_volume_changed(egui_view_t *self, uint8_t value);" in layout

    def test_slider_event_listener_registration(self):
        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        slider = WidgetModel("slider", name="vol_slider", x=0, y=0, width=200, height=30)
        slider.events = {"onValueChanged": "on_volume_changed"}
        root.add_child(slider)
        page = _make_page("event_page", root)
        proj = _make_project([page])
        layout = generate_page_layout_source(page, proj)
        assert "egui_view_slider_set_on_value_changed_listener(" in layout
        assert "on_volume_changed);" in layout

    def test_switch_event_code_gen(self):
        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        sw = WidgetModel("switch", name="sw1", x=0, y=0, width=60, height=30)
        sw.events = {"onCheckedChanged": "on_switch_changed"}
        root.add_child(sw)
        page = _make_page("sw_page", root)
        proj = _make_project([page])
        layout = generate_page_layout_source(page, proj)
        assert "extern void on_switch_changed(egui_view_t *self, int is_checked);" in layout
        assert "egui_view_switch_set_on_checked_listener(" in layout

    def test_no_event_no_declaration(self):
        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        slider = WidgetModel("slider", name="s1", x=0, y=0, width=200, height=30)
        root.add_child(slider)
        page = _make_page("no_event", root)
        proj = _make_project([page])
        layout = generate_page_layout_source(page, proj)
        assert "event callbacks" not in layout
        assert "set_on_value_changed_listener" not in layout

    def test_init_lines_contain_event_listener(self):
        slider = WidgetModel("slider", name="vol", x=10, y=20, width=200, height=30)
        slider.events = {"onValueChanged": "on_vol"}
        lines = _gen_widget_init_lines(slider)
        joined = "\n".join(lines)
        assert "egui_view_slider_set_on_value_changed_listener(" in joined
        assert "on_vol);" in joined


# ======================================================================
# TestCodeQualityOptimizations
# ======================================================================


class TestCodeQualityOptimizations:
    """Tests for generated code quality improvements."""

    def test_default_values_skipped(self):
        """Setter calls for default values should be omitted."""
        w = WidgetModel("slider", name="s1", x=0, y=0, width=200, height=30)
        # value=0 is the default for slider
        w.properties["value"] = 0
        lines = _gen_widget_init_lines(w)
        text = "\n".join(lines)
        assert "set_value" not in text

    def test_non_default_values_emitted(self):
        """Setter calls for non-default values should be present."""
        w = WidgetModel("slider", name="s1", x=0, y=0, width=200, height=30)
        w.properties["value"] = 128
        lines = _gen_widget_init_lines(w)
        text = "\n".join(lines)
        assert "set_value" in text
        assert "128" in text

    def test_widget_name_comment(self):
        """Each widget init block should have a name comment."""
        w = WidgetModel("label", name="title_lbl", x=0, y=0, width=100, height=30)
        lines = _gen_widget_init_lines(w)
        assert any("// title_lbl (label)" in l for l in lines)

    def test_label_default_align_skipped(self):
        """Label with default align_type should not emit set_align_type."""
        w = WidgetModel("label", name="lbl", x=0, y=0, width=100, height=30)
        # align_type default is EGUI_ALIGN_CENTER
        lines = _gen_widget_init_lines(w)
        text = "\n".join(lines)
        assert "set_align_type" not in text


# ======================================================================
# TestParamOnlyKindWidgets
# Regression tests for: KeyError: 'func' when code_gen kind is
# "param_only" or "param" (no function call, used only for struct init).
# ======================================================================


class TestParamOnlyKindWidgets:
    """Widgets with 'param_only'/'param' code_gen kinds must not raise KeyError."""

    def _make_page_with(self, widget):
        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        root.add_child(widget)
        page = _make_page("test_page", root)
        proj = _make_project([page])
        return page, proj

    def test_scale_widget_no_key_error(self):
        """Scale has param_only/param entries - must not raise KeyError."""
        w = WidgetModel("scale", name="sc1", x=0, y=0, width=200, height=40)
        w.properties["min_value"] = 0
        w.properties["max_value"] = 100
        w.properties["major_count"] = 5
        w.properties["value"] = 60
        page, proj = self._make_page_with(w)
        # Must not raise
        output = generate_page_layout_source(page, proj)
        assert "egui_view_scale_init" in output
        assert "set_value" in output

    def test_table_widget_no_key_error(self):
        """Table has param_only entries - must not raise KeyError."""
        w = WidgetModel("table", name="tbl1", x=0, y=0, width=200, height=160)
        w.properties["col_count"] = 3
        w.properties["row_count"] = 4
        w.properties["row_height"] = 40
        page, proj = self._make_page_with(w)
        output = generate_page_layout_source(page, proj)
        assert "egui_view_table_init" in output

    def test_mini_calendar_widget_no_key_error(self):
        """MiniCalendar has param_only entries - must not raise KeyError."""
        w = WidgetModel("mini_calendar", name="cal1", x=0, y=0, width=200, height=200)
        w.properties["year"] = 2026
        w.properties["month"] = 3
        page, proj = self._make_page_with(w)
        output = generate_page_layout_source(page, proj)
        assert "egui_view_mini_calendar_init" in output

    def test_gen_widget_init_lines_scale_no_key_error(self):
        """_gen_widget_init_lines with scale must not raise KeyError."""
        w = WidgetModel("scale", name="sc2", x=10, y=10, width=180, height=30)
        # Should not raise
        lines = _gen_widget_init_lines(w)
        assert any("egui_view_scale_init" in l for l in lines)

    def test_multiple_param_kind_widgets_on_same_page(self):
        """Multiple param_only widgets on one page - all must succeed."""
        root = WidgetModel("group", name="root", x=0, y=0, width=240, height=320)
        scale = WidgetModel("scale", name="sc", x=0, y=0, width=200, height=30)
        table = WidgetModel("table", name="tbl", x=0, y=100, width=200, height=160)
        cal = WidgetModel("mini_calendar", name="cal", x=0, y=270, width=200, height=200)
        root.add_child(scale)
        root.add_child(table)
        root.add_child(cal)
        page = _make_page("multi_param", root)
        proj = _make_project([page])
        # Must not raise
        output = generate_page_layout_source(page, proj)
        assert "egui_view_scale_init" in output
        assert "egui_view_table_init" in output
        assert "egui_view_mini_calendar_init" in output
