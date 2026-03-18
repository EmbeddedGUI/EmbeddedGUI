#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_mini_calendar.h"

static egui_view_mini_calendar_t test_calendar;
static uint8_t g_selected_count;
static uint8_t g_last_day;

static uint8_t days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint8_t d = days[month - 1];

    if (month == 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)))
    {
        d = 29;
    }
    return d;
}

static uint8_t day_of_week(uint16_t year, uint8_t month, uint8_t day)
{
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    uint16_t y = year;

    if (month < 3)
    {
        y--;
    }
    return (uint8_t)((y + y / 4 - y / 100 + y / 400 + t[month - 1] + day) % 7);
}

static void on_date_selected(egui_view_t *self, uint8_t day)
{
    EGUI_UNUSED(self);
    g_selected_count++;
    g_last_day = day;
}

static void setup_calendar(void)
{
    egui_view_mini_calendar_init(EGUI_VIEW_OF(&test_calendar));
    egui_view_set_size(EGUI_VIEW_OF(&test_calendar), 210, 160);
    egui_view_mini_calendar_set_date(EGUI_VIEW_OF(&test_calendar), 2026, 3, 5);
    egui_view_mini_calendar_set_on_date_selected_listener(EGUI_VIEW_OF(&test_calendar), on_date_selected);
    g_selected_count = 0;
    g_last_day = 0;
}

static void layout_calendar(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 210;
    region.size.height = 160;
    egui_view_layout(EGUI_VIEW_OF(&test_calendar), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_calendar)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_calendar)->api->on_touch_event(EGUI_VIEW_OF(&test_calendar), &event);
}

static void get_day_center(uint8_t day, egui_dim_t *x, egui_dim_t *y)
{
    egui_dim_t cell_w = EGUI_VIEW_OF(&test_calendar)->region.size.width / 7;
    egui_dim_t header_h = EGUI_VIEW_OF(&test_calendar)->region.size.height / 8;
    egui_dim_t cell_h = (EGUI_VIEW_OF(&test_calendar)->region.size.height - header_h * 2) / 6;
    uint8_t start_col = (day_of_week(test_calendar.year, test_calendar.month, 1) - test_calendar.first_day_of_week + 7) % 7;
    uint8_t total_days = days_in_month(test_calendar.year, test_calendar.month);
    uint8_t pos;
    uint8_t col;
    uint8_t row;

    EGUI_TEST_ASSERT_TRUE(day >= 1);
    EGUI_TEST_ASSERT_TRUE(day <= total_days);

    pos = (uint8_t)(start_col + day - 1);
    col = (uint8_t)(pos % 7);
    row = (uint8_t)(pos / 7);

    *x = EGUI_VIEW_OF(&test_calendar)->region_screen.location.x + col * cell_w + cell_w / 2;
    *y = EGUI_VIEW_OF(&test_calendar)->region_screen.location.y + header_h * 2 + row * cell_h + cell_h / 2;
}

static void test_mini_calendar_release_requires_same_day(void)
{
    egui_dim_t x7 = 0;
    egui_dim_t y7 = 0;
    egui_dim_t x8 = 0;
    egui_dim_t y8 = 0;

    setup_calendar();
    layout_calendar();
    get_day_center(7, &x7, &y7);
    get_day_center(8, &x8, &y8);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x7, y7));
    EGUI_TEST_ASSERT_EQUAL_INT(7, test_calendar.pressed_day);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_calendar)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x8, y8));
    EGUI_TEST_ASSERT_EQUAL_INT(7, test_calendar.pressed_day);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_calendar)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x8, y8));
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_calendar.day);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_selected_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_calendar.pressed_day);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x8, y8));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x7, y7));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x8, y8));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_calendar)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x8, y8));
    EGUI_TEST_ASSERT_EQUAL_INT(8, test_calendar.day);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selected_count);
    EGUI_TEST_ASSERT_EQUAL_INT(8, g_last_day);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_calendar.pressed_day);
}

void test_mini_calendar_run(void)
{
    EGUI_TEST_SUITE_BEGIN(mini_calendar);
    EGUI_TEST_RUN(test_mini_calendar_release_requires_same_day);
    EGUI_TEST_SUITE_END();
}
