#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_analog_clock_get_state.h"

static egui_view_analog_clock_t s_clock;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_clock, 0, sizeof(s_clock));
    egui_view_analog_clock_init(EGUI_VIEW_OF(&s_clock), core);
}

static void test_analog_clock_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_analog_clock_get_hour(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_analog_clock_get_minute(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_analog_clock_get_second(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_analog_clock_get_show_second(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_analog_clock_get_show_ticks(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)(EGUI_THEME_STROKE_WIDTH * 2), (int)egui_view_analog_clock_get_hour_hand_width(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_STROKE_WIDTH, (int)egui_view_analog_clock_get_minute_hand_width(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_analog_clock_get_second_hand_width(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT.full, (int)egui_view_analog_clock_get_dial_color(EGUI_VIEW_OF(&s_clock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT.full, (int)egui_view_analog_clock_get_hour_color(EGUI_VIEW_OF(&s_clock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT.full, (int)egui_view_analog_clock_get_minute_color(EGUI_VIEW_OF(&s_clock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_DANGER.full, (int)egui_view_analog_clock_get_second_color(EGUI_VIEW_OF(&s_clock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TRACK_BG.full, (int)egui_view_analog_clock_get_tick_color(EGUI_VIEW_OF(&s_clock)).full);
}

static void test_analog_clock_get_state_after_setters(void)
{
    egui_color_t dial = {.full = 0x1234};
    egui_color_t hour = {.full = 0x2345};
    egui_color_t minute = {.full = 0x3456};
    egui_color_t second = {.full = 0x4567};
    egui_color_t tick = {.full = 0x5678};

    setup();
    egui_view_analog_clock_set_time(EGUI_VIEW_OF(&s_clock), 14, 70, 99);
    egui_view_analog_clock_show_second(EGUI_VIEW_OF(&s_clock), 0);
    egui_view_analog_clock_show_ticks(EGUI_VIEW_OF(&s_clock), 0);
    egui_view_analog_clock_set_hour_hand_width(EGUI_VIEW_OF(&s_clock), 6);
    egui_view_analog_clock_set_minute_hand_width(EGUI_VIEW_OF(&s_clock), 4);
    egui_view_analog_clock_set_second_hand_width(EGUI_VIEW_OF(&s_clock), 2);
    egui_view_analog_clock_set_dial_color(EGUI_VIEW_OF(&s_clock), dial);
    egui_view_analog_clock_set_hour_color(EGUI_VIEW_OF(&s_clock), hour);
    egui_view_analog_clock_set_minute_color(EGUI_VIEW_OF(&s_clock), minute);
    egui_view_analog_clock_set_second_color(EGUI_VIEW_OF(&s_clock), second);
    egui_view_analog_clock_set_tick_color(EGUI_VIEW_OF(&s_clock), tick);

    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_analog_clock_get_hour(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(59, (int)egui_view_analog_clock_get_minute(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(59, (int)egui_view_analog_clock_get_second(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_analog_clock_get_show_second(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_analog_clock_get_show_ticks(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_analog_clock_get_hour_hand_width(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_analog_clock_get_minute_hand_width(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_analog_clock_get_second_hand_width(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)dial.full, (int)egui_view_analog_clock_get_dial_color(EGUI_VIEW_OF(&s_clock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)hour.full, (int)egui_view_analog_clock_get_hour_color(EGUI_VIEW_OF(&s_clock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)minute.full, (int)egui_view_analog_clock_get_minute_color(EGUI_VIEW_OF(&s_clock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)second.full, (int)egui_view_analog_clock_get_second_color(EGUI_VIEW_OF(&s_clock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)tick.full, (int)egui_view_analog_clock_get_tick_color(EGUI_VIEW_OF(&s_clock)).full);
}

static void test_analog_clock_get_state_apply_params(void)
{
    static const egui_view_analog_clock_params_t params = {
            .region = {{3, 4}, {50, 60}},
            .hour = 23,
            .minute = 88,
            .second = 99,
    };

    setup();
    egui_view_analog_clock_apply_params(EGUI_VIEW_OF(&s_clock), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(11, (int)egui_view_analog_clock_get_hour(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(59, (int)egui_view_analog_clock_get_minute(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(59, (int)egui_view_analog_clock_get_second(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(50, (int)egui_view_get_width(EGUI_VIEW_OF(&s_clock)));
    EGUI_TEST_ASSERT_EQUAL_INT(60, (int)egui_view_get_height(EGUI_VIEW_OF(&s_clock)));
}

static void test_analog_clock_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_analog_clock_get_hour(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_analog_clock_get_minute(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_analog_clock_get_second(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_analog_clock_get_show_second(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_analog_clock_get_show_ticks(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_analog_clock_get_hour_hand_width(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_analog_clock_get_minute_hand_width(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_analog_clock_get_second_hand_width(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_analog_clock_get_dial_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_analog_clock_get_hour_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_analog_clock_get_minute_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_analog_clock_get_second_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_analog_clock_get_tick_color(NULL).full);
}

void test_analog_clock_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(analog_clock_get_state);

    EGUI_TEST_RUN(test_analog_clock_get_state_defaults);
    EGUI_TEST_RUN(test_analog_clock_get_state_after_setters);
    EGUI_TEST_RUN(test_analog_clock_get_state_apply_params);
    EGUI_TEST_RUN(test_analog_clock_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
