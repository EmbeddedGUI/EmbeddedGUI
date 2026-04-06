#include "egui.h"
#include "test/egui_test.h"
#include "test_compact_text.h"

#include "widget/egui_view_button.h"
#include "widget/egui_view_chart_line.h"
#include "widget/egui_view_chart_pie.h"
#include "widget/egui_view_checkbox.h"
#include "widget/egui_view_circular_progress_bar.h"
#include "core/egui_canvas_compact.h"
#include "widget/egui_view_gauge.h"
#include "widget/egui_view_image_button.h"
#include "widget/egui_view_radio_button.h"
#include "widget/egui_view_toggle_button.h"

EGUI_VIEW_BUTTON_PARAMS_INIT_SIMPLE(test_button_simple_params, 0, 0, 96, 28, "Run");
EGUI_VIEW_IMAGE_BUTTON_PARAMS_INIT(test_image_button_params, 4, 5, 72, 48, NULL);

static void test_compact_text_supports_basic_ascii_words(void)
{
    egui_canvas_compact_text_layout_t layout;

    EGUI_TEST_ASSERT_TRUE(egui_canvas_compact_text_is_supported("Settings"));
    EGUI_TEST_ASSERT_TRUE(egui_canvas_compact_text_is_supported("Run 9+"));
    EGUI_TEST_ASSERT_TRUE(egui_canvas_compact_text_is_supported("75%"));
    EGUI_TEST_ASSERT_TRUE(egui_canvas_compact_text_measure("Favorite", 108, 30, &layout));
    EGUI_TEST_ASSERT_TRUE(egui_canvas_compact_text_measure("75%", 48, 16, &layout));
    EGUI_TEST_ASSERT_TRUE(layout.scale > 0);
    EGUI_TEST_ASSERT_TRUE(layout.width > 0);
    EGUI_TEST_ASSERT_TRUE(layout.height > 0);
}

static void test_compact_text_rejects_non_ascii_sequences(void)
{
    EGUI_TEST_ASSERT_FALSE(egui_canvas_compact_text_is_supported("\xE4\xB8\xAD"));
}

static void test_compact_number_supports_basic_numeric_tokens(void)
{
    egui_canvas_compact_number_layout_t layout;

    EGUI_TEST_ASSERT_TRUE(egui_canvas_compact_number_is_supported("75%"));
    EGUI_TEST_ASSERT_TRUE(egui_canvas_compact_number_is_supported("99+"));
    EGUI_TEST_ASSERT_FALSE(egui_canvas_compact_number_is_supported("A1"));
    EGUI_TEST_ASSERT_TRUE(egui_canvas_compact_number_measure("75%", 48, 16, &layout));
    EGUI_TEST_ASSERT_TRUE(layout.scale > 0);
    EGUI_TEST_ASSERT_TRUE(layout.width > 0);
    EGUI_TEST_ASSERT_TRUE(layout.height > 0);
}

static void test_button_simple_params_default_font_is_null(void)
{
    egui_view_button_t button;

    EGUI_TEST_ASSERT_NULL(test_button_simple_params.font);

    egui_view_button_init_with_params(EGUI_VIEW_OF(&button), &test_button_simple_params);
    EGUI_TEST_ASSERT_NULL(button.base.font);
}

static void test_image_button_init_default_font_is_null(void)
{
    egui_view_image_button_t button;

    egui_view_image_button_init(EGUI_VIEW_OF(&button));
    EGUI_TEST_ASSERT_NULL(button.font);
}

static void test_image_button_apply_params_copies_region_and_image(void)
{
    egui_view_image_button_t button;

    egui_view_image_button_init(EGUI_VIEW_OF(&button));
    egui_view_image_button_apply_params(EGUI_VIEW_OF(&button), &test_image_button_params);

    EGUI_TEST_ASSERT_EQUAL_INT(4, EGUI_VIEW_OF(&button)->region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(5, EGUI_VIEW_OF(&button)->region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(72, EGUI_VIEW_OF(&button)->region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(48, EGUI_VIEW_OF(&button)->region.size.height);
    EGUI_TEST_ASSERT_NULL(button.base.image);
}

static void test_toggle_button_init_default_font_is_null(void)
{
    egui_view_toggle_button_t button;

    egui_view_toggle_button_init(EGUI_VIEW_OF(&button));
    EGUI_TEST_ASSERT_NULL(button.font);
}

static void test_circular_progress_bar_init_default_font_is_null(void)
{
    egui_view_circular_progress_bar_t progress_bar;

    egui_view_circular_progress_bar_init(EGUI_VIEW_OF(&progress_bar));
    EGUI_TEST_ASSERT_NULL(progress_bar.font);
}

static void test_gauge_init_default_font_is_null(void)
{
    egui_view_gauge_t gauge;

    egui_view_gauge_init(EGUI_VIEW_OF(&gauge));
    EGUI_TEST_ASSERT_NULL(gauge.font);
}

static void test_checkbox_init_default_font_is_null(void)
{
    egui_view_checkbox_t checkbox;

    egui_view_checkbox_init(EGUI_VIEW_OF(&checkbox));
    EGUI_TEST_ASSERT_NULL(checkbox.font);
}

static void test_radio_button_init_default_font_is_null(void)
{
    egui_view_radio_button_t radio_button;

    egui_view_radio_button_init(EGUI_VIEW_OF(&radio_button));
    EGUI_TEST_ASSERT_NULL(radio_button.font);
}

static void test_chart_pie_init_default_font_is_null(void)
{
    egui_view_chart_pie_t chart_pie;

    egui_view_chart_pie_init(EGUI_VIEW_OF(&chart_pie));
    EGUI_TEST_ASSERT_NULL(chart_pie.font);
    EGUI_TEST_ASSERT_NOT_NULL(chart_pie.text_ops);
}

static void test_chart_line_init_default_font_is_null(void)
{
    egui_view_chart_line_t chart_line;

    egui_view_chart_line_init(EGUI_VIEW_OF(&chart_line));
    EGUI_TEST_ASSERT_NULL(chart_line.axis_base.ab.font);
    EGUI_TEST_ASSERT_NOT_NULL(chart_line.axis_base.ab.text_ops);
}

void test_compact_text_run(void)
{
    EGUI_TEST_SUITE_BEGIN(compact_text);
    EGUI_TEST_RUN(test_compact_text_supports_basic_ascii_words);
    EGUI_TEST_RUN(test_compact_text_rejects_non_ascii_sequences);
    EGUI_TEST_RUN(test_compact_number_supports_basic_numeric_tokens);
    EGUI_TEST_RUN(test_button_simple_params_default_font_is_null);
    EGUI_TEST_RUN(test_image_button_init_default_font_is_null);
    EGUI_TEST_RUN(test_image_button_apply_params_copies_region_and_image);
    EGUI_TEST_RUN(test_toggle_button_init_default_font_is_null);
    EGUI_TEST_RUN(test_circular_progress_bar_init_default_font_is_null);
    EGUI_TEST_RUN(test_gauge_init_default_font_is_null);
    EGUI_TEST_RUN(test_checkbox_init_default_font_is_null);
    EGUI_TEST_RUN(test_radio_button_init_default_font_is_null);
    EGUI_TEST_RUN(test_chart_pie_init_default_font_is_null);
    EGUI_TEST_RUN(test_chart_line_init_default_font_is_null);
    EGUI_TEST_SUITE_END();
}
