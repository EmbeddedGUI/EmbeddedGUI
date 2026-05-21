#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_chart_get_state.h"

static egui_view_chart_line_t s_line;
static egui_view_chart_bar_t s_bar;

static void setup_line(void)
{
    memset(&s_line, 0, sizeof(s_line));
    egui_view_chart_line_init(EGUI_VIEW_OF(&s_line), uicode_get_core());
}

static void setup_bar(void)
{
    memset(&s_bar, 0, sizeof(s_bar));
    egui_view_chart_bar_init(EGUI_VIEW_OF(&s_bar), uicode_get_core());
}

static void test_chart_line_get_state_defaults(void)
{
    setup_line();

    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_chart_line_get_line_width(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_chart_line_get_point_radius(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)s_line.axis_base.clip_margin);
}

static void test_chart_line_get_state_after_setters(void)
{
    setup_line();
    egui_view_chart_line_set_line_width(EGUI_VIEW_OF(&s_line), 4);
    egui_view_chart_line_set_point_radius(EGUI_VIEW_OF(&s_line), 6);

    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_chart_line_get_line_width(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_chart_line_get_point_radius(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT(10, (int)s_line.axis_base.clip_margin);
}

static void test_chart_line_get_state_apply_params(void)
{
    static const egui_view_chart_line_params_t params = {
            .region = {{3, 4}, {80, 60}},
    };

    setup_line();
    egui_view_chart_line_apply_params(EGUI_VIEW_OF(&s_line), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT(80, (int)egui_view_get_width(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT(60, (int)egui_view_get_height(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_chart_line_get_line_width(EGUI_VIEW_OF(&s_line)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_chart_line_get_point_radius(EGUI_VIEW_OF(&s_line)));
}

static void test_chart_bar_get_state_defaults(void)
{
    setup_bar();

    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_chart_bar_get_bar_gap(EGUI_VIEW_OF(&s_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)s_bar.axis_base.clip_margin);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)s_bar.axis_base.ab.axis_x.is_categorical);
}

static void test_chart_bar_get_state_after_setter(void)
{
    setup_bar();
    egui_view_chart_bar_set_bar_gap(EGUI_VIEW_OF(&s_bar), 7);

    EGUI_TEST_ASSERT_EQUAL_INT(7, (int)egui_view_chart_bar_get_bar_gap(EGUI_VIEW_OF(&s_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(9, (int)s_bar.axis_base.clip_margin);
}

static void test_chart_bar_get_state_apply_params(void)
{
    static const egui_view_chart_bar_params_t params = {
            .region = {{5, 6}, {90, 70}},
    };

    setup_bar();
    egui_view_chart_bar_apply_params(EGUI_VIEW_OF(&s_bar), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_get_x(EGUI_VIEW_OF(&s_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_get_y(EGUI_VIEW_OF(&s_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(90, (int)egui_view_get_width(EGUI_VIEW_OF(&s_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(70, (int)egui_view_get_height(EGUI_VIEW_OF(&s_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_chart_bar_get_bar_gap(EGUI_VIEW_OF(&s_bar)));
}

static void test_chart_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_chart_line_get_line_width(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_chart_line_get_point_radius(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_chart_bar_get_bar_gap(NULL));
}

void test_chart_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(chart_get_state);

    EGUI_TEST_RUN(test_chart_line_get_state_defaults);
    EGUI_TEST_RUN(test_chart_line_get_state_after_setters);
    EGUI_TEST_RUN(test_chart_line_get_state_apply_params);
    EGUI_TEST_RUN(test_chart_bar_get_state_defaults);
    EGUI_TEST_RUN(test_chart_bar_get_state_after_setter);
    EGUI_TEST_RUN(test_chart_bar_get_state_apply_params);
    EGUI_TEST_RUN(test_chart_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
