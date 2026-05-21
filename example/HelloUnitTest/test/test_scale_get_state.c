#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_scale_get_state.h"

static egui_view_scale_t s_scale;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_scale, 0, sizeof(s_scale));
    egui_view_scale_init(EGUI_VIEW_OF(&s_scale), core);
}

static void test_scale_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scale_get_range_min(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT(100, (int)egui_view_scale_get_range_max(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scale_get_value(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_scale_get_major_tick_count(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_scale_get_minor_tick_count(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_scale_get_is_horizontal(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_scale_get_show_labels(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scale_get_show_indicator(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TRACK_BG.full, (int)egui_view_scale_get_tick_color(EGUI_VIEW_OF(&s_scale)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT.full, (int)egui_view_scale_get_label_color(EGUI_VIEW_OF(&s_scale)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_PRIMARY.full, (int)egui_view_scale_get_indicator_color(EGUI_VIEW_OF(&s_scale)).full);
    EGUI_TEST_ASSERT_NULL(egui_view_scale_get_font(EGUI_VIEW_OF(&s_scale)));
}

static void test_scale_get_state_after_setters(void)
{
    egui_color_t tick = {.full = 0x1234};
    egui_color_t label = {.full = 0x2345};
    egui_color_t indicator = {.full = 0x3456};

    setup();
    egui_view_scale_set_range(EGUI_VIEW_OF(&s_scale), -50, 150);
    egui_view_scale_set_value(EGUI_VIEW_OF(&s_scale), 75);
    egui_view_scale_set_ticks(EGUI_VIEW_OF(&s_scale), 8, 3);
    egui_view_scale_set_orientation(EGUI_VIEW_OF(&s_scale), 0);
    egui_view_scale_set_tick_color(EGUI_VIEW_OF(&s_scale), tick);
    egui_view_scale_set_label_color(EGUI_VIEW_OF(&s_scale), label);
    egui_view_scale_set_indicator_color(EGUI_VIEW_OF(&s_scale), indicator);
    egui_view_scale_show_labels(EGUI_VIEW_OF(&s_scale), 0);
    egui_view_scale_show_indicator(EGUI_VIEW_OF(&s_scale), 1);
    egui_view_scale_set_font(EGUI_VIEW_OF(&s_scale), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    EGUI_TEST_ASSERT_EQUAL_INT(-50, (int)egui_view_scale_get_range_min(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT(150, (int)egui_view_scale_get_range_max(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT(75, (int)egui_view_scale_get_value(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT(8, (int)egui_view_scale_get_major_tick_count(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_scale_get_minor_tick_count(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scale_get_is_horizontal(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)tick.full, (int)egui_view_scale_get_tick_color(EGUI_VIEW_OF(&s_scale)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)label.full, (int)egui_view_scale_get_label_color(EGUI_VIEW_OF(&s_scale)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)indicator.full, (int)egui_view_scale_get_indicator_color(EGUI_VIEW_OF(&s_scale)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scale_get_show_labels(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_scale_get_show_indicator(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_TRUE(egui_view_scale_get_font(EGUI_VIEW_OF(&s_scale)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_scale_set_font(EGUI_VIEW_OF(&s_scale), NULL);
    EGUI_TEST_ASSERT_NULL(egui_view_scale_get_font(EGUI_VIEW_OF(&s_scale)));
}

static void test_scale_get_state_value_clamp(void)
{
    setup();
    egui_view_scale_set_range(EGUI_VIEW_OF(&s_scale), 10, 20);

    egui_view_scale_set_value(EGUI_VIEW_OF(&s_scale), 30);
    EGUI_TEST_ASSERT_EQUAL_INT(20, (int)egui_view_scale_get_value(EGUI_VIEW_OF(&s_scale)));

    egui_view_scale_set_value(EGUI_VIEW_OF(&s_scale), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(10, (int)egui_view_scale_get_value(EGUI_VIEW_OF(&s_scale)));
}

static void test_scale_get_state_apply_params(void)
{
    static const egui_view_scale_params_t params = {
            .region = {{3, 4}, {50, 60}},
            .range_min = -10,
            .range_max = 90,
            .major_tick_count = 5,
    };

    setup();
    egui_view_scale_apply_params(EGUI_VIEW_OF(&s_scale), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(-10, (int)egui_view_scale_get_range_min(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT(90, (int)egui_view_scale_get_range_max(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_scale_get_major_tick_count(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT(50, (int)egui_view_get_width(EGUI_VIEW_OF(&s_scale)));
    EGUI_TEST_ASSERT_EQUAL_INT(60, (int)egui_view_get_height(EGUI_VIEW_OF(&s_scale)));
}

static void test_scale_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scale_get_range_min(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scale_get_range_max(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scale_get_value(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scale_get_major_tick_count(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scale_get_minor_tick_count(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scale_get_is_horizontal(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scale_get_tick_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scale_get_label_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scale_get_indicator_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scale_get_show_labels(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scale_get_show_indicator(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_scale_get_font(NULL));
}

void test_scale_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(scale_get_state);

    EGUI_TEST_RUN(test_scale_get_state_defaults);
    EGUI_TEST_RUN(test_scale_get_state_after_setters);
    EGUI_TEST_RUN(test_scale_get_state_value_clamp);
    EGUI_TEST_RUN(test_scale_get_state_apply_params);
    EGUI_TEST_RUN(test_scale_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
