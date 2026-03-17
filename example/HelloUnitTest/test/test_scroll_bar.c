#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_scroll_bar.h"

#include "../../HelloCustomWidgets/input/scroll_bar/egui_view_scroll_bar.h"
#include "../../HelloCustomWidgets/input/scroll_bar/egui_view_scroll_bar.c"

static egui_view_scroll_bar_t test_scroll_bar;

static void setup_scroll_bar(egui_dim_t content_length, egui_dim_t viewport_length, egui_dim_t offset, egui_dim_t line_step, egui_dim_t page_step)
{
    egui_view_scroll_bar_init(EGUI_VIEW_OF(&test_scroll_bar));
    egui_view_set_size(EGUI_VIEW_OF(&test_scroll_bar), 112, 132);
    egui_view_scroll_bar_set_content_metrics(EGUI_VIEW_OF(&test_scroll_bar), content_length, viewport_length);
    egui_view_scroll_bar_set_step_size(EGUI_VIEW_OF(&test_scroll_bar), line_step, page_step);
    egui_view_scroll_bar_set_offset(EGUI_VIEW_OF(&test_scroll_bar), offset);
    egui_view_scroll_bar_set_current_part(EGUI_VIEW_OF(&test_scroll_bar), EGUI_VIEW_SCROLL_BAR_PART_THUMB);
}

static void layout_scroll_bar(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = x;
    region.location.y = y;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(EGUI_VIEW_OF(&test_scroll_bar), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_scroll_bar)->region_screen, &region);
}

static int send_touch_event(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_scroll_bar)->api->on_touch_event(EGUI_VIEW_OF(&test_scroll_bar), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_scroll_bar)->api->on_key_event(EGUI_VIEW_OF(&test_scroll_bar), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_scroll_bar)->api->on_key_event(EGUI_VIEW_OF(&test_scroll_bar), &event);
    return handled;
}

static void test_scroll_bar_tab_cycles_parts(void)
{
    setup_scroll_bar(800, 200, 120, 20, 80);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SCROLL_BAR_PART_THUMB, egui_view_scroll_bar_get_current_part(EGUI_VIEW_OF(&test_scroll_bar)));
    EGUI_TEST_ASSERT_TRUE(egui_view_scroll_bar_handle_navigation_key(EGUI_VIEW_OF(&test_scroll_bar), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SCROLL_BAR_PART_INCREASE, egui_view_scroll_bar_get_current_part(EGUI_VIEW_OF(&test_scroll_bar)));
    EGUI_TEST_ASSERT_TRUE(egui_view_scroll_bar_handle_navigation_key(EGUI_VIEW_OF(&test_scroll_bar), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SCROLL_BAR_PART_DECREASE, egui_view_scroll_bar_get_current_part(EGUI_VIEW_OF(&test_scroll_bar)));
    EGUI_TEST_ASSERT_TRUE(egui_view_scroll_bar_handle_navigation_key(EGUI_VIEW_OF(&test_scroll_bar), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SCROLL_BAR_PART_THUMB, egui_view_scroll_bar_get_current_part(EGUI_VIEW_OF(&test_scroll_bar)));
}

static void test_scroll_bar_thumb_keyboard_step(void)
{
    setup_scroll_bar(800, 200, 120, 20, 80);
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(140, egui_view_scroll_bar_get_offset(EGUI_VIEW_OF(&test_scroll_bar)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_UP));
    EGUI_TEST_ASSERT_EQUAL_INT(120, egui_view_scroll_bar_get_offset(EGUI_VIEW_OF(&test_scroll_bar)));
}

static void test_scroll_bar_plus_minus_page_step(void)
{
    setup_scroll_bar(800, 200, 120, 20, 80);
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_PLUS));
    EGUI_TEST_ASSERT_EQUAL_INT(200, egui_view_scroll_bar_get_offset(EGUI_VIEW_OF(&test_scroll_bar)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_MINUS));
    EGUI_TEST_ASSERT_EQUAL_INT(120, egui_view_scroll_bar_get_offset(EGUI_VIEW_OF(&test_scroll_bar)));
}

static void test_scroll_bar_home_end(void)
{
    setup_scroll_bar(800, 200, 120, 20, 80);
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_scroll_bar_get_offset(EGUI_VIEW_OF(&test_scroll_bar)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(600, egui_view_scroll_bar_get_offset(EGUI_VIEW_OF(&test_scroll_bar)));
}

static void test_scroll_bar_touch_decrease_button(void)
{
    egui_region_t region;

    setup_scroll_bar(800, 200, 120, 20, 80);
    layout_scroll_bar(10, 20, 112, 132);
    EGUI_TEST_ASSERT_TRUE(egui_view_scroll_bar_get_part_region(EGUI_VIEW_OF(&test_scroll_bar), EGUI_VIEW_SCROLL_BAR_PART_DECREASE, &region));
    EGUI_TEST_ASSERT_TRUE(
            send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(100, egui_view_scroll_bar_get_offset(EGUI_VIEW_OF(&test_scroll_bar)));
}

static void test_scroll_bar_touch_track_pages(void)
{
    egui_region_t track_region;
    egui_region_t thumb_region;
    egui_dim_t x;
    egui_dim_t y;

    setup_scroll_bar(800, 200, 120, 20, 80);
    layout_scroll_bar(10, 20, 112, 132);
    EGUI_TEST_ASSERT_TRUE(egui_view_scroll_bar_get_part_region(EGUI_VIEW_OF(&test_scroll_bar), EGUI_VIEW_SCROLL_BAR_PART_TRACK, &track_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_scroll_bar_get_part_region(EGUI_VIEW_OF(&test_scroll_bar), EGUI_VIEW_SCROLL_BAR_PART_THUMB, &thumb_region));
    x = track_region.location.x + track_region.size.width / 2;
    y = thumb_region.location.y + thumb_region.size.height + 6;
    if (y >= track_region.location.y + track_region.size.height)
    {
        y = track_region.location.y + track_region.size.height - 2;
    }

    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(200, egui_view_scroll_bar_get_offset(EGUI_VIEW_OF(&test_scroll_bar)));
}

static void test_scroll_bar_thumb_drag_reaches_end(void)
{
    egui_region_t track_region;
    egui_region_t thumb_region;
    egui_dim_t start_x;
    egui_dim_t start_y;
    egui_dim_t end_y;

    setup_scroll_bar(800, 200, 120, 20, 80);
    layout_scroll_bar(10, 20, 112, 132);
    EGUI_TEST_ASSERT_TRUE(egui_view_scroll_bar_get_part_region(EGUI_VIEW_OF(&test_scroll_bar), EGUI_VIEW_SCROLL_BAR_PART_TRACK, &track_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_scroll_bar_get_part_region(EGUI_VIEW_OF(&test_scroll_bar), EGUI_VIEW_SCROLL_BAR_PART_THUMB, &thumb_region));
    start_x = thumb_region.location.x + thumb_region.size.width / 2;
    start_y = thumb_region.location.y + thumb_region.size.height / 2;
    end_y = track_region.location.y + track_region.size.height - thumb_region.size.height / 2;

    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, start_x, start_y));
    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_MOVE, start_x, end_y));
    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_UP, start_x, end_y));
    EGUI_TEST_ASSERT_EQUAL_INT(600, egui_view_scroll_bar_get_offset(EGUI_VIEW_OF(&test_scroll_bar)));
}

static void test_scroll_bar_read_only_and_compact_ignore_input(void)
{
    egui_region_t decrease_region;

    setup_scroll_bar(800, 200, 120, 20, 80);
    egui_view_scroll_bar_set_read_only_mode(EGUI_VIEW_OF(&test_scroll_bar), 1);
    EGUI_TEST_ASSERT_FALSE(egui_view_scroll_bar_handle_navigation_key(EGUI_VIEW_OF(&test_scroll_bar), EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(120, egui_view_scroll_bar_get_offset(EGUI_VIEW_OF(&test_scroll_bar)));

    setup_scroll_bar(800, 200, 120, 20, 80);
    egui_view_scroll_bar_set_compact_mode(EGUI_VIEW_OF(&test_scroll_bar), 1);
    layout_scroll_bar(10, 20, 112, 132);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_TRUE(egui_view_scroll_bar_get_part_region(EGUI_VIEW_OF(&test_scroll_bar), EGUI_VIEW_SCROLL_BAR_PART_DECREASE, &decrease_region));
    EGUI_TEST_ASSERT_FALSE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, decrease_region.location.x + decrease_region.size.width / 2,
                                            decrease_region.location.y + decrease_region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(120, egui_view_scroll_bar_get_offset(EGUI_VIEW_OF(&test_scroll_bar)));
}

static void test_scroll_bar_clamps_metrics_and_offset(void)
{
    setup_scroll_bar(10, 5, 3, 0, 0);
    egui_view_scroll_bar_set_content_metrics(EGUI_VIEW_OF(&test_scroll_bar), 0, 0);
    egui_view_scroll_bar_set_offset(EGUI_VIEW_OF(&test_scroll_bar), 90);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_scroll_bar_get_content_length(EGUI_VIEW_OF(&test_scroll_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_scroll_bar_get_viewport_length(EGUI_VIEW_OF(&test_scroll_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_scroll_bar_get_max_offset(EGUI_VIEW_OF(&test_scroll_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_scroll_bar_get_offset(EGUI_VIEW_OF(&test_scroll_bar)));
}

void test_scroll_bar_run(void)
{
    EGUI_TEST_SUITE_BEGIN(scroll_bar);
    EGUI_TEST_RUN(test_scroll_bar_tab_cycles_parts);
    EGUI_TEST_RUN(test_scroll_bar_thumb_keyboard_step);
    EGUI_TEST_RUN(test_scroll_bar_plus_minus_page_step);
    EGUI_TEST_RUN(test_scroll_bar_home_end);
    EGUI_TEST_RUN(test_scroll_bar_touch_decrease_button);
    EGUI_TEST_RUN(test_scroll_bar_touch_track_pages);
    EGUI_TEST_RUN(test_scroll_bar_thumb_drag_reaches_end);
    EGUI_TEST_RUN(test_scroll_bar_read_only_and_compact_ignore_input);
    EGUI_TEST_RUN(test_scroll_bar_clamps_metrics_and_offset);
    EGUI_TEST_SUITE_END();
}
