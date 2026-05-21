#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_line_get_state.h"

static egui_view_line_t s_line;
static const egui_view_line_point_t s_points[] = {
        {0, 0},
        {10, 3},
        {20, 0},
};

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_line, 0, sizeof(s_line));
    egui_view_line_init(EGUI_VIEW_OF(&s_line), core);
}

static void test_line_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_line_get_points(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_line_get_point_count(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_STROKE_WIDTH, (int)egui_view_line_get_line_width(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_PRIMARY.full, (int)egui_view_line_get_line_color(EGUI_VIEW_OF(&s_line)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_line_get_use_round_cap(EGUI_VIEW_OF(&s_line)));
}

static void test_line_get_state_after_setters(void)
{
    egui_color_t color = {.full = 0x2345};

    setup();
    egui_view_line_set_points(EGUI_VIEW_OF(&s_line), s_points, EGUI_ARRAY_SIZE(s_points));
    egui_view_line_set_line_width(EGUI_VIEW_OF(&s_line), 4);
    egui_view_line_set_line_color(EGUI_VIEW_OF(&s_line), color);
    egui_view_line_set_use_round_cap(EGUI_VIEW_OF(&s_line), 1);

    EGUI_TEST_ASSERT_TRUE(egui_view_line_get_points(EGUI_VIEW_OF(&s_line)) == s_points);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ARRAY_SIZE(s_points), (int)egui_view_line_get_point_count(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_line_get_line_width(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)color.full, (int)egui_view_line_get_line_color(EGUI_VIEW_OF(&s_line)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_line_get_use_round_cap(EGUI_VIEW_OF(&s_line)));

    egui_view_line_set_points(EGUI_VIEW_OF(&s_line), NULL, 0);
    egui_view_line_set_use_round_cap(EGUI_VIEW_OF(&s_line), 0);

    EGUI_TEST_ASSERT_NULL(egui_view_line_get_points(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_line_get_point_count(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_line_get_use_round_cap(EGUI_VIEW_OF(&s_line)));
}

static void test_line_get_state_apply_params(void)
{
    static const egui_view_line_params_t params = {
            .region = {{1, 2}, {30, 12}},
            .line_width = 3,
            .line_color = {.full = 0x4567},
    };

    setup();
    egui_view_line_apply_params(EGUI_VIEW_OF(&s_line), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_line_get_line_width(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)params.line_color.full, (int)egui_view_line_get_line_color(EGUI_VIEW_OF(&s_line)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_get_x(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_get_y(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT(30, (int)egui_view_get_width(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT(12, (int)egui_view_get_height(EGUI_VIEW_OF(&s_line)));
}

static void test_line_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_line_get_points(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_line_get_point_count(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_line_get_line_width(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_line_get_line_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_line_get_use_round_cap(NULL));
}

void test_line_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(line_get_state);

    EGUI_TEST_RUN(test_line_get_state_defaults);
    EGUI_TEST_RUN(test_line_get_state_after_setters);
    EGUI_TEST_RUN(test_line_get_state_apply_params);
    EGUI_TEST_RUN(test_line_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
