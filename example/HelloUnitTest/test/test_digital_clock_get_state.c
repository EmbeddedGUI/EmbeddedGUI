#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_digital_clock_get_state.h"

static egui_view_digital_clock_t s_clock;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_clock, 0, sizeof(s_clock));
    egui_view_digital_clock_init(EGUI_VIEW_OF(&s_clock), core);
}

static void assert_time_text(const char *expected)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(expected, egui_view_digital_clock_get_time_text(EGUI_VIEW_OF(&s_clock))));
    EGUI_TEST_ASSERT_TRUE(egui_view_label_get_text(EGUI_VIEW_OF(&s_clock)) == egui_view_digital_clock_get_time_text(EGUI_VIEW_OF(&s_clock)));
}

static void test_digital_clock_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_digital_clock_get_hour(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_digital_clock_get_minute(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_digital_clock_get_second(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_digital_clock_get_format_24h(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_digital_clock_get_show_second(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_digital_clock_get_colon_blink(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_digital_clock_get_colon_visible(EGUI_VIEW_OF(&s_clock)));
    assert_time_text("00:00:00");
}

static void test_digital_clock_get_state_after_set_time(void)
{
    setup();
    egui_view_digital_clock_set_time(EGUI_VIEW_OF(&s_clock), 23, 5, 9);

    EGUI_TEST_ASSERT_EQUAL_INT(23, (int)egui_view_digital_clock_get_hour(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_digital_clock_get_minute(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(9, (int)egui_view_digital_clock_get_second(EGUI_VIEW_OF(&s_clock)));
    assert_time_text("23:05:09");
}

static void test_digital_clock_get_state_format_options(void)
{
    setup();
    egui_view_digital_clock_set_time(EGUI_VIEW_OF(&s_clock), 13, 7, 8);
    egui_view_digital_clock_set_show_second(EGUI_VIEW_OF(&s_clock), 0);

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_digital_clock_get_show_second(EGUI_VIEW_OF(&s_clock)));
    assert_time_text("13:07");

    egui_view_digital_clock_set_format(EGUI_VIEW_OF(&s_clock), 0);

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_digital_clock_get_format_24h(EGUI_VIEW_OF(&s_clock)));
    assert_time_text(" 1:07 PM");
}

static void test_digital_clock_get_state_colon_blink(void)
{
    setup();
    egui_view_digital_clock_set_time(EGUI_VIEW_OF(&s_clock), 9, 8, 7);
    egui_view_digital_clock_set_colon_blink(EGUI_VIEW_OF(&s_clock), 1);
    egui_view_digital_clock_set_colon_visible(EGUI_VIEW_OF(&s_clock), 0);

    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_digital_clock_get_colon_blink(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_digital_clock_get_colon_visible(EGUI_VIEW_OF(&s_clock)));
    assert_time_text("09 08 07");

    egui_view_digital_clock_set_colon_visible(EGUI_VIEW_OF(&s_clock), 1);

    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_digital_clock_get_colon_visible(EGUI_VIEW_OF(&s_clock)));
    assert_time_text("09:08:07");
}

static void test_digital_clock_get_state_apply_params(void)
{
    static const egui_view_label_params_t params = {
            .region = {{3, 4}, {70, 20}},
            .text = "ignored after next format",
    };

    setup();
    egui_view_digital_clock_apply_params(EGUI_VIEW_OF(&s_clock), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(70, (int)egui_view_get_width(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(20, (int)egui_view_get_height(EGUI_VIEW_OF(&s_clock)));

    egui_view_digital_clock_set_time(EGUI_VIEW_OF(&s_clock), 1, 2, 3);
    assert_time_text("01:02:03");
}

static void test_digital_clock_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_digital_clock_get_hour(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_digital_clock_get_minute(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_digital_clock_get_second(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_digital_clock_get_time_text(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_digital_clock_get_format_24h(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_digital_clock_get_show_second(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_digital_clock_get_colon_blink(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_digital_clock_get_colon_visible(NULL));
}

void test_digital_clock_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(digital_clock_get_state);

    EGUI_TEST_RUN(test_digital_clock_get_state_defaults);
    EGUI_TEST_RUN(test_digital_clock_get_state_after_set_time);
    EGUI_TEST_RUN(test_digital_clock_get_state_format_options);
    EGUI_TEST_RUN(test_digital_clock_get_state_colon_blink);
    EGUI_TEST_RUN(test_digital_clock_get_state_apply_params);
    EGUI_TEST_RUN(test_digital_clock_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
