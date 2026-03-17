#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_calendar_view.h"

#include "../../HelloCustomWidgets/input/calendar_view/egui_view_calendar_view.h"
#include "../../HelloCustomWidgets/input/calendar_view/egui_view_calendar_view.c"

static egui_view_calendar_view_t test_calendar_view;

static uint8_t calendar_view_test_day_of_week(uint16_t year, uint8_t month, uint8_t day)
{
    static const int offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    uint16_t y = year;

    if (month < 3)
    {
        y--;
    }
    return (uint8_t)((y + y / 4 - y / 100 + y / 400 + offsets[month - 1] + day) % 7);
}

static void setup_calendar_view(uint16_t year, uint8_t month, uint8_t start_day, uint8_t end_day)
{
    egui_view_calendar_view_init(EGUI_VIEW_OF(&test_calendar_view));
    egui_view_set_size(EGUI_VIEW_OF(&test_calendar_view), 196, 144);
    egui_view_calendar_view_set_display_month(EGUI_VIEW_OF(&test_calendar_view), year, month);
    egui_view_calendar_view_set_range(EGUI_VIEW_OF(&test_calendar_view), year, month, start_day, end_day);
}

static void layout_calendar_view(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = x;
    region.location.y = y;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(EGUI_VIEW_OF(&test_calendar_view), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_calendar_view)->region_screen, &region);
}

static int send_touch_event(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_calendar_view)->api->on_touch_event(EGUI_VIEW_OF(&test_calendar_view), &event);
}

static int send_key_event(uint8_t type, uint8_t key_code)
{
    egui_key_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.key_code = key_code;
    return EGUI_VIEW_OF(&test_calendar_view)->api->on_key_event(EGUI_VIEW_OF(&test_calendar_view), &event);
}

static void get_day_center(uint8_t day, egui_dim_t *x, egui_dim_t *y)
{
    egui_region_t grid_region;
    uint8_t first_dow = calendar_view_test_day_of_week(egui_view_calendar_view_get_display_year(EGUI_VIEW_OF(&test_calendar_view)),
                                                       egui_view_calendar_view_get_display_month(EGUI_VIEW_OF(&test_calendar_view)), 1);
    uint8_t start_cell = (uint8_t)((first_dow - egui_view_calendar_view_get_first_day_of_week(EGUI_VIEW_OF(&test_calendar_view)) + 7) % 7);
    uint8_t pos = (uint8_t)(start_cell + day - 1);
    uint8_t col = (uint8_t)(pos % 7);
    uint8_t row = (uint8_t)(pos / 7);
    egui_dim_t cell_w;
    egui_dim_t cell_h;

    EGUI_TEST_ASSERT_TRUE(egui_view_calendar_view_get_part_region(EGUI_VIEW_OF(&test_calendar_view), EGUI_VIEW_CALENDAR_VIEW_PART_GRID, &grid_region));
    cell_w = grid_region.size.width / 7;
    cell_h = grid_region.size.height / 6;
    *x = grid_region.location.x + col * cell_w + cell_w / 2;
    *y = grid_region.location.y + row * cell_h + cell_h / 2;
}

static void test_calendar_view_tab_cycles_parts(void)
{
    setup_calendar_view(2026, 3, 9, 11);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_CALENDAR_VIEW_PART_GRID, egui_view_calendar_view_get_current_part(EGUI_VIEW_OF(&test_calendar_view)));
    EGUI_TEST_ASSERT_TRUE(egui_view_calendar_view_handle_navigation_key(EGUI_VIEW_OF(&test_calendar_view), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_CALENDAR_VIEW_PART_PREV, egui_view_calendar_view_get_current_part(EGUI_VIEW_OF(&test_calendar_view)));
    EGUI_TEST_ASSERT_TRUE(egui_view_calendar_view_handle_navigation_key(EGUI_VIEW_OF(&test_calendar_view), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_CALENDAR_VIEW_PART_NEXT, egui_view_calendar_view_get_current_part(EGUI_VIEW_OF(&test_calendar_view)));
    EGUI_TEST_ASSERT_TRUE(egui_view_calendar_view_handle_navigation_key(EGUI_VIEW_OF(&test_calendar_view), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_CALENDAR_VIEW_PART_GRID, egui_view_calendar_view_get_current_part(EGUI_VIEW_OF(&test_calendar_view)));
}

static void test_calendar_view_keyboard_range_commit(void)
{
    setup_calendar_view(2026, 3, 9, 11);
    EGUI_TEST_ASSERT_FALSE(egui_view_calendar_view_get_editing_range(EGUI_VIEW_OF(&test_calendar_view)));
    EGUI_TEST_ASSERT_TRUE(egui_view_calendar_view_handle_navigation_key(EGUI_VIEW_OF(&test_calendar_view), EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_TRUE(egui_view_calendar_view_get_editing_range(EGUI_VIEW_OF(&test_calendar_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(11, egui_view_calendar_view_get_start_day(EGUI_VIEW_OF(&test_calendar_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(11, egui_view_calendar_view_get_end_day(EGUI_VIEW_OF(&test_calendar_view)));

    EGUI_TEST_ASSERT_TRUE(egui_view_calendar_view_handle_navigation_key(EGUI_VIEW_OF(&test_calendar_view), EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_TRUE(egui_view_calendar_view_handle_navigation_key(EGUI_VIEW_OF(&test_calendar_view), EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(11, egui_view_calendar_view_get_start_day(EGUI_VIEW_OF(&test_calendar_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(13, egui_view_calendar_view_get_end_day(EGUI_VIEW_OF(&test_calendar_view)));

    EGUI_TEST_ASSERT_TRUE(egui_view_calendar_view_handle_navigation_key(EGUI_VIEW_OF(&test_calendar_view), EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_FALSE(egui_view_calendar_view_get_editing_range(EGUI_VIEW_OF(&test_calendar_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(11, egui_view_calendar_view_get_start_day(EGUI_VIEW_OF(&test_calendar_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(13, egui_view_calendar_view_get_end_day(EGUI_VIEW_OF(&test_calendar_view)));
}

static void test_calendar_view_plus_minus_shift_display_month(void)
{
    setup_calendar_view(2026, 3, 9, 11);
    EGUI_TEST_ASSERT_TRUE(egui_view_calendar_view_handle_navigation_key(EGUI_VIEW_OF(&test_calendar_view), EGUI_KEY_CODE_PLUS));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_calendar_view_get_display_month(EGUI_VIEW_OF(&test_calendar_view)));
    EGUI_TEST_ASSERT_TRUE(egui_view_calendar_view_handle_navigation_key(EGUI_VIEW_OF(&test_calendar_view), EGUI_KEY_CODE_MINUS));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_calendar_view_get_display_month(EGUI_VIEW_OF(&test_calendar_view)));
}

static void test_calendar_view_touch_two_taps_commits_range(void)
{
    egui_dim_t x1;
    egui_dim_t y1;
    egui_dim_t x2;
    egui_dim_t y2;

    setup_calendar_view(2026, 3, 9, 11);
    layout_calendar_view(10, 20, 196, 144);
    get_day_center(9, &x1, &y1);
    get_day_center(13, &x2, &y2);

    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, x1, y1));
    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_UP, x1, y1));
    EGUI_TEST_ASSERT_TRUE(egui_view_calendar_view_get_editing_range(EGUI_VIEW_OF(&test_calendar_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(9, egui_view_calendar_view_get_start_day(EGUI_VIEW_OF(&test_calendar_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(9, egui_view_calendar_view_get_end_day(EGUI_VIEW_OF(&test_calendar_view)));

    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, x2, y2));
    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_UP, x2, y2));
    EGUI_TEST_ASSERT_FALSE(egui_view_calendar_view_get_editing_range(EGUI_VIEW_OF(&test_calendar_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(9, egui_view_calendar_view_get_start_day(EGUI_VIEW_OF(&test_calendar_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(13, egui_view_calendar_view_get_end_day(EGUI_VIEW_OF(&test_calendar_view)));
}

static void test_calendar_view_touch_prev_button_changes_month(void)
{
    egui_region_t prev_region;

    setup_calendar_view(2026, 3, 9, 11);
    layout_calendar_view(10, 20, 196, 144);
    EGUI_TEST_ASSERT_TRUE(egui_view_calendar_view_get_part_region(EGUI_VIEW_OF(&test_calendar_view), EGUI_VIEW_CALENDAR_VIEW_PART_PREV, &prev_region));
    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, prev_region.location.x + prev_region.size.width / 2,
                                           prev_region.location.y + prev_region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_UP, prev_region.location.x + prev_region.size.width / 2,
                                           prev_region.location.y + prev_region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_calendar_view_get_display_month(EGUI_VIEW_OF(&test_calendar_view)));
}

static void test_calendar_view_set_range_clamps_order(void)
{
    setup_calendar_view(2026, 3, 9, 11);
    egui_view_calendar_view_set_range(EGUI_VIEW_OF(&test_calendar_view), 2026, 2, 29, 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2026, egui_view_calendar_view_get_selection_year(EGUI_VIEW_OF(&test_calendar_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_calendar_view_get_selection_month(EGUI_VIEW_OF(&test_calendar_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_calendar_view_get_start_day(EGUI_VIEW_OF(&test_calendar_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(28, egui_view_calendar_view_get_end_day(EGUI_VIEW_OF(&test_calendar_view)));
}

static void test_calendar_view_read_only_and_compact_ignore_interaction(void)
{
    egui_region_t grid_region;

    setup_calendar_view(2026, 3, 9, 11);
    egui_view_calendar_view_set_read_only_mode(EGUI_VIEW_OF(&test_calendar_view), 1);
    EGUI_TEST_ASSERT_FALSE(egui_view_calendar_view_handle_navigation_key(EGUI_VIEW_OF(&test_calendar_view), EGUI_KEY_CODE_LEFT));

    setup_calendar_view(2026, 3, 9, 11);
    egui_view_calendar_view_set_compact_mode(EGUI_VIEW_OF(&test_calendar_view), 1);
    layout_calendar_view(10, 20, 196, 144);
    EGUI_TEST_ASSERT_FALSE(egui_view_calendar_view_handle_navigation_key(EGUI_VIEW_OF(&test_calendar_view), EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_FALSE(egui_view_calendar_view_get_part_region(EGUI_VIEW_OF(&test_calendar_view), EGUI_VIEW_CALENDAR_VIEW_PART_GRID, &grid_region));
    EGUI_TEST_ASSERT_FALSE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_LEFT));
}

void test_calendar_view_run(void)
{
    EGUI_TEST_SUITE_BEGIN(calendar_view);
    EGUI_TEST_RUN(test_calendar_view_tab_cycles_parts);
    EGUI_TEST_RUN(test_calendar_view_keyboard_range_commit);
    EGUI_TEST_RUN(test_calendar_view_plus_minus_shift_display_month);
    EGUI_TEST_RUN(test_calendar_view_touch_two_taps_commits_range);
    EGUI_TEST_RUN(test_calendar_view_touch_prev_button_changes_month);
    EGUI_TEST_RUN(test_calendar_view_set_range_clamps_order);
    EGUI_TEST_RUN(test_calendar_view_read_only_and_compact_ignore_interaction);
    EGUI_TEST_SUITE_END();
}
