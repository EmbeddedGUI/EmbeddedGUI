#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_compass_get_state.h"

static egui_view_compass_t s_compass;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_compass, 0, sizeof(s_compass));
    egui_view_compass_init(EGUI_VIEW_OF(&s_compass), core);
}

static void test_compass_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_compass_get_heading(EGUI_VIEW_OF(&s_compass)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_compass_get_show_degree(EGUI_VIEW_OF(&s_compass)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_STROKE_WIDTH, (int)egui_view_compass_get_stroke_width(EGUI_VIEW_OF(&s_compass)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT.full, (int)egui_view_compass_get_dial_color(EGUI_VIEW_OF(&s_compass)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_DANGER.full, (int)egui_view_compass_get_north_color(EGUI_VIEW_OF(&s_compass)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TRACK_BG.full, (int)egui_view_compass_get_needle_color(EGUI_VIEW_OF(&s_compass)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_WHITE.full, (int)egui_view_compass_get_text_color(EGUI_VIEW_OF(&s_compass)).full);
    EGUI_TEST_ASSERT_TRUE(egui_view_compass_get_font(EGUI_VIEW_OF(&s_compass)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
}

static void test_compass_get_state_after_setters(void)
{
    egui_color_t dial = {.full = 0x1234};
    egui_color_t north = {.full = 0x2345};
    egui_color_t needle = {.full = 0x3456};
    egui_color_t text = {.full = 0x4567};

    setup();
    egui_view_compass_set_heading(EGUI_VIEW_OF(&s_compass), 450);
    egui_view_compass_set_show_degree(EGUI_VIEW_OF(&s_compass), 0);
    egui_view_compass_set_stroke_width(EGUI_VIEW_OF(&s_compass), 5);
    egui_view_compass_set_dial_color(EGUI_VIEW_OF(&s_compass), dial);
    egui_view_compass_set_north_color(EGUI_VIEW_OF(&s_compass), north);
    egui_view_compass_set_needle_color(EGUI_VIEW_OF(&s_compass), needle);
    egui_view_compass_set_text_color(EGUI_VIEW_OF(&s_compass), text);
    egui_view_compass_set_font(EGUI_VIEW_OF(&s_compass), NULL);

    EGUI_TEST_ASSERT_EQUAL_INT(90, (int)egui_view_compass_get_heading(EGUI_VIEW_OF(&s_compass)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_compass_get_show_degree(EGUI_VIEW_OF(&s_compass)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_compass_get_stroke_width(EGUI_VIEW_OF(&s_compass)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)dial.full, (int)egui_view_compass_get_dial_color(EGUI_VIEW_OF(&s_compass)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)north.full, (int)egui_view_compass_get_north_color(EGUI_VIEW_OF(&s_compass)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)needle.full, (int)egui_view_compass_get_needle_color(EGUI_VIEW_OF(&s_compass)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)text.full, (int)egui_view_compass_get_text_color(EGUI_VIEW_OF(&s_compass)).full);
    EGUI_TEST_ASSERT_NULL(egui_view_compass_get_font(EGUI_VIEW_OF(&s_compass)));

    egui_view_compass_set_heading(EGUI_VIEW_OF(&s_compass), -1);
    EGUI_TEST_ASSERT_EQUAL_INT(359, (int)egui_view_compass_get_heading(EGUI_VIEW_OF(&s_compass)));
}

static void test_compass_get_state_apply_params(void)
{
    static const egui_view_compass_params_t params = {
            .region = {{3, 4}, {50, 60}},
            .heading = 275,
    };

    setup();
    egui_view_compass_apply_params(EGUI_VIEW_OF(&s_compass), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(275, (int)egui_view_compass_get_heading(EGUI_VIEW_OF(&s_compass)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_compass)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_compass)));
    EGUI_TEST_ASSERT_EQUAL_INT(50, (int)egui_view_get_width(EGUI_VIEW_OF(&s_compass)));
    EGUI_TEST_ASSERT_EQUAL_INT(60, (int)egui_view_get_height(EGUI_VIEW_OF(&s_compass)));
}

static void test_compass_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_compass_get_heading(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_compass_get_show_degree(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_compass_get_stroke_width(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_compass_get_dial_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_compass_get_north_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_compass_get_needle_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_compass_get_text_color(NULL).full);
    EGUI_TEST_ASSERT_NULL(egui_view_compass_get_font(NULL));
}

void test_compass_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(compass_get_state);

    EGUI_TEST_RUN(test_compass_get_state_defaults);
    EGUI_TEST_RUN(test_compass_get_state_after_setters);
    EGUI_TEST_RUN(test_compass_get_state_apply_params);
    EGUI_TEST_RUN(test_compass_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
