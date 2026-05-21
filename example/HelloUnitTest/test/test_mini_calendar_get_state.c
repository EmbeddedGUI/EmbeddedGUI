#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_mini_calendar_get_state.h"

static egui_view_mini_calendar_t s_calendar;
static uint8_t s_selected_day;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_calendar, 0, sizeof(s_calendar));
    s_selected_day = 0;
    egui_view_mini_calendar_init(EGUI_VIEW_OF(&s_calendar), core);
}

static void on_date_selected(egui_view_t *self, uint8_t day)
{
    EGUI_UNUSED(self);
    s_selected_day = day;
}

static void test_mini_calendar_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT(2026, (int)egui_view_mini_calendar_get_year(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_mini_calendar_get_month(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_mini_calendar_get_day(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mini_calendar_get_today(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mini_calendar_get_pressed_day(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_mini_calendar_get_focused_day(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mini_calendar_get_first_day_of_week(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_TRUE(strcmp("S", egui_view_mini_calendar_get_weekday_label(EGUI_VIEW_OF(&s_calendar), 0)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("M", egui_view_mini_calendar_get_weekday_label(EGUI_VIEW_OF(&s_calendar), 1)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("S", egui_view_mini_calendar_get_weekday_label(EGUI_VIEW_OF(&s_calendar), 6)) == 0);
    EGUI_TEST_ASSERT_NULL(egui_view_mini_calendar_get_on_date_selected_listener(EGUI_VIEW_OF(&s_calendar)));
}

static void test_mini_calendar_get_state_after_setters(void)
{
    static const char *const labels[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    setup();
    egui_view_mini_calendar_set_date(EGUI_VIEW_OF(&s_calendar), 2024, 2, 29);
    egui_view_mini_calendar_set_today(EGUI_VIEW_OF(&s_calendar), 15);
    egui_view_mini_calendar_set_first_day_of_week(EGUI_VIEW_OF(&s_calendar), 1);
    egui_view_mini_calendar_set_weekday_labels(EGUI_VIEW_OF(&s_calendar), labels);
    egui_view_mini_calendar_set_on_date_selected_listener(EGUI_VIEW_OF(&s_calendar), on_date_selected);

    EGUI_TEST_ASSERT_EQUAL_INT(2024, (int)egui_view_mini_calendar_get_year(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_mini_calendar_get_month(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_EQUAL_INT(29, (int)egui_view_mini_calendar_get_day(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_EQUAL_INT(29, (int)egui_view_mini_calendar_get_focused_day(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_EQUAL_INT(15, (int)egui_view_mini_calendar_get_today(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_mini_calendar_get_first_day_of_week(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_TRUE(egui_view_mini_calendar_get_weekday_label(EGUI_VIEW_OF(&s_calendar), 0) == labels[0]);
    EGUI_TEST_ASSERT_TRUE(egui_view_mini_calendar_get_weekday_label(EGUI_VIEW_OF(&s_calendar), 6) == labels[6]);
    EGUI_TEST_ASSERT_TRUE(egui_view_mini_calendar_get_on_date_selected_listener(EGUI_VIEW_OF(&s_calendar)) == on_date_selected);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)s_selected_day);
}

static void test_mini_calendar_get_state_default_labels_follow_first_day(void)
{
    setup();
    egui_view_mini_calendar_set_first_day_of_week(EGUI_VIEW_OF(&s_calendar), 1);

    EGUI_TEST_ASSERT_TRUE(strcmp("M", egui_view_mini_calendar_get_weekday_label(EGUI_VIEW_OF(&s_calendar), 0)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("S", egui_view_mini_calendar_get_weekday_label(EGUI_VIEW_OF(&s_calendar), 5)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("S", egui_view_mini_calendar_get_weekday_label(EGUI_VIEW_OF(&s_calendar), 6)) == 0);

    {
        static const char *const labels[] = {"0", "1", "2", "3", "4", "5", "6"};

        egui_view_mini_calendar_set_weekday_labels(EGUI_VIEW_OF(&s_calendar), labels);
        EGUI_TEST_ASSERT_TRUE(egui_view_mini_calendar_get_weekday_label(EGUI_VIEW_OF(&s_calendar), 0) == labels[0]);
        EGUI_TEST_ASSERT_TRUE(egui_view_mini_calendar_get_weekday_label(EGUI_VIEW_OF(&s_calendar), 6) == labels[6]);

        egui_view_mini_calendar_set_weekday_labels(EGUI_VIEW_OF(&s_calendar), NULL);
        EGUI_TEST_ASSERT_TRUE(strcmp("M", egui_view_mini_calendar_get_weekday_label(EGUI_VIEW_OF(&s_calendar), 0)) == 0);
        EGUI_TEST_ASSERT_TRUE(strcmp("S", egui_view_mini_calendar_get_weekday_label(EGUI_VIEW_OF(&s_calendar), 6)) == 0);
    }
}

static void test_mini_calendar_get_state_apply_params(void)
{
    static const egui_view_mini_calendar_params_t params = {
            .region = {{3, 4}, {140, 120}},
            .year = 2031,
            .month = 13,
            .day = 32,
    };

    setup();
    egui_view_mini_calendar_apply_params(EGUI_VIEW_OF(&s_calendar), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_EQUAL_INT(140, (int)egui_view_get_width(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_EQUAL_INT(120, (int)egui_view_get_height(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_EQUAL_INT(2031, (int)egui_view_mini_calendar_get_year(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_EQUAL_INT(13, (int)egui_view_mini_calendar_get_month(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_EQUAL_INT(32, (int)egui_view_mini_calendar_get_day(EGUI_VIEW_OF(&s_calendar)));
    EGUI_TEST_ASSERT_EQUAL_INT(32, (int)egui_view_mini_calendar_get_focused_day(EGUI_VIEW_OF(&s_calendar)));
}

static void test_mini_calendar_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mini_calendar_get_year(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mini_calendar_get_month(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mini_calendar_get_day(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mini_calendar_get_today(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mini_calendar_get_pressed_day(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mini_calendar_get_focused_day(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mini_calendar_get_first_day_of_week(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_mini_calendar_get_weekday_label(NULL, 0));
    EGUI_TEST_ASSERT_NULL(egui_view_mini_calendar_get_weekday_label(EGUI_VIEW_OF(&s_calendar), 7));
    EGUI_TEST_ASSERT_NULL(egui_view_mini_calendar_get_on_date_selected_listener(NULL));
}

void test_mini_calendar_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(mini_calendar_get_state);

    EGUI_TEST_RUN(test_mini_calendar_get_state_defaults);
    EGUI_TEST_RUN(test_mini_calendar_get_state_after_setters);
    EGUI_TEST_RUN(test_mini_calendar_get_state_default_labels_follow_first_day);
    EGUI_TEST_RUN(test_mini_calendar_get_state_apply_params);
    EGUI_TEST_RUN(test_mini_calendar_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
