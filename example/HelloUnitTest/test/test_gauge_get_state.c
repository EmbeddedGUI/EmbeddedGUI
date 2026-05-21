#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_gauge_get_state.h"

static egui_view_gauge_t s_gauge;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_gauge, 0, sizeof(s_gauge));
    egui_view_gauge_init(EGUI_VIEW_OF(&s_gauge), core);
}

static void test_gauge_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_gauge_get_value(EGUI_VIEW_OF(&s_gauge)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TRACK_THICKNESS, (int)egui_view_gauge_get_stroke_width(EGUI_VIEW_OF(&s_gauge)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_gauge_get_needle_width(EGUI_VIEW_OF(&s_gauge)));
    EGUI_TEST_ASSERT_EQUAL_INT(150, (int)egui_view_gauge_get_start_angle(EGUI_VIEW_OF(&s_gauge)));
    EGUI_TEST_ASSERT_EQUAL_INT(240, (int)egui_view_gauge_get_sweep_angle(EGUI_VIEW_OF(&s_gauge)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TRACK_BG.full, (int)egui_view_gauge_get_bk_color(EGUI_VIEW_OF(&s_gauge)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_PRIMARY.full, (int)egui_view_gauge_get_progress_color(EGUI_VIEW_OF(&s_gauge)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_DANGER.full, (int)egui_view_gauge_get_needle_color(EGUI_VIEW_OF(&s_gauge)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT_PRIMARY.full, (int)egui_view_gauge_get_text_color(EGUI_VIEW_OF(&s_gauge)).full);
    EGUI_TEST_ASSERT_NULL(egui_view_gauge_get_font(EGUI_VIEW_OF(&s_gauge)));
}

static void test_gauge_get_state_after_setters(void)
{
    egui_color_t bk = {.full = 0x1234};
    egui_color_t progress = {.full = 0x2345};
    egui_color_t needle = {.full = 0x3456};
    egui_color_t text = {.full = 0x4567};

    setup();
    egui_view_gauge_set_value(EGUI_VIEW_OF(&s_gauge), 65);
    egui_view_gauge_set_stroke_width(EGUI_VIEW_OF(&s_gauge), 12);
    egui_view_gauge_set_needle_width(EGUI_VIEW_OF(&s_gauge), 5);
    egui_view_gauge_set_start_angle(EGUI_VIEW_OF(&s_gauge), 135);
    egui_view_gauge_set_sweep_angle(EGUI_VIEW_OF(&s_gauge), 270);
    egui_view_gauge_set_bk_color(EGUI_VIEW_OF(&s_gauge), bk);
    egui_view_gauge_set_progress_color(EGUI_VIEW_OF(&s_gauge), progress);
    egui_view_gauge_set_needle_color(EGUI_VIEW_OF(&s_gauge), needle);
    egui_view_gauge_set_text_color(EGUI_VIEW_OF(&s_gauge), text);
    egui_view_gauge_set_font(EGUI_VIEW_OF(&s_gauge), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    EGUI_TEST_ASSERT_EQUAL_INT(65, (int)egui_view_gauge_get_value(EGUI_VIEW_OF(&s_gauge)));
    EGUI_TEST_ASSERT_EQUAL_INT(12, (int)egui_view_gauge_get_stroke_width(EGUI_VIEW_OF(&s_gauge)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_gauge_get_needle_width(EGUI_VIEW_OF(&s_gauge)));
    EGUI_TEST_ASSERT_EQUAL_INT(135, (int)egui_view_gauge_get_start_angle(EGUI_VIEW_OF(&s_gauge)));
    EGUI_TEST_ASSERT_EQUAL_INT(270, (int)egui_view_gauge_get_sweep_angle(EGUI_VIEW_OF(&s_gauge)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)bk.full, (int)egui_view_gauge_get_bk_color(EGUI_VIEW_OF(&s_gauge)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)progress.full, (int)egui_view_gauge_get_progress_color(EGUI_VIEW_OF(&s_gauge)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)needle.full, (int)egui_view_gauge_get_needle_color(EGUI_VIEW_OF(&s_gauge)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)text.full, (int)egui_view_gauge_get_text_color(EGUI_VIEW_OF(&s_gauge)).full);
    EGUI_TEST_ASSERT_TRUE(egui_view_gauge_get_font(EGUI_VIEW_OF(&s_gauge)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_gauge_set_font(EGUI_VIEW_OF(&s_gauge), NULL);
    EGUI_TEST_ASSERT_NULL(egui_view_gauge_get_font(EGUI_VIEW_OF(&s_gauge)));
}

static void test_gauge_get_state_value_clamp(void)
{
    setup();

    egui_view_gauge_set_value(EGUI_VIEW_OF(&s_gauge), 255);
    EGUI_TEST_ASSERT_EQUAL_INT(100, (int)egui_view_gauge_get_value(EGUI_VIEW_OF(&s_gauge)));
}

static void test_gauge_get_state_apply_params(void)
{
    static const egui_view_gauge_params_t params = {
            .region = {{3, 4}, {50, 60}},
            .value = 75,
    };

    setup();
    egui_view_gauge_apply_params(EGUI_VIEW_OF(&s_gauge), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(75, (int)egui_view_gauge_get_value(EGUI_VIEW_OF(&s_gauge)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_gauge)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_gauge)));
    EGUI_TEST_ASSERT_EQUAL_INT(50, (int)egui_view_get_width(EGUI_VIEW_OF(&s_gauge)));
    EGUI_TEST_ASSERT_EQUAL_INT(60, (int)egui_view_get_height(EGUI_VIEW_OF(&s_gauge)));
}

static void test_gauge_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_gauge_get_value(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_gauge_get_stroke_width(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_gauge_get_needle_width(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_gauge_get_start_angle(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_gauge_get_sweep_angle(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_gauge_get_bk_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_gauge_get_progress_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_gauge_get_needle_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_gauge_get_text_color(NULL).full);
    EGUI_TEST_ASSERT_NULL(egui_view_gauge_get_font(NULL));
}

void test_gauge_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(gauge_get_state);

    EGUI_TEST_RUN(test_gauge_get_state_defaults);
    EGUI_TEST_RUN(test_gauge_get_state_after_setters);
    EGUI_TEST_RUN(test_gauge_get_state_value_clamp);
    EGUI_TEST_RUN(test_gauge_get_state_apply_params);
    EGUI_TEST_RUN(test_gauge_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
