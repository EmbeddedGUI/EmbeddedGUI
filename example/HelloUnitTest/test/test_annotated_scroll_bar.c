#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_annotated_scroll_bar.h"

#include "../../HelloCustomWidgets/navigation/annotated_scroll_bar/egui_view_annotated_scroll_bar.h"
#include "../../HelloCustomWidgets/navigation/annotated_scroll_bar/egui_view_annotated_scroll_bar.c"

static egui_view_annotated_scroll_bar_t test_bar;

static const egui_view_annotated_scroll_bar_marker_t test_markers[] = {
        {"A", "Intro", 0, EGUI_COLOR_HEX(0x2563EB)},
        {"B", "Board", 180, EGUI_COLOR_HEX(0x2563EB)},
        {"C", "Review", 420, EGUI_COLOR_HEX(0x2563EB)},
        {"D", "Wrap", 780, EGUI_COLOR_HEX(0x2563EB)},
};

static void setup_bar(egui_dim_t content_length, egui_dim_t viewport_length, egui_dim_t offset, egui_dim_t small_change, egui_dim_t large_change)
{
    egui_view_annotated_scroll_bar_init(EGUI_VIEW_OF(&test_bar));
    egui_view_set_size(EGUI_VIEW_OF(&test_bar), 144, 108);
    egui_view_annotated_scroll_bar_set_markers(EGUI_VIEW_OF(&test_bar), test_markers, (uint8_t)(sizeof(test_markers) / sizeof(test_markers[0])));
    egui_view_annotated_scroll_bar_set_content_metrics(EGUI_VIEW_OF(&test_bar), content_length, viewport_length);
    egui_view_annotated_scroll_bar_set_step_size(EGUI_VIEW_OF(&test_bar), small_change, large_change);
    egui_view_annotated_scroll_bar_set_offset(EGUI_VIEW_OF(&test_bar), offset);
    egui_view_annotated_scroll_bar_set_current_part(EGUI_VIEW_OF(&test_bar), EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
}

static void layout_bar(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 144;
    region.size.height = 108;
    egui_view_layout(EGUI_VIEW_OF(&test_bar), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_bar)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_bar)->api->on_touch_event(EGUI_VIEW_OF(&test_bar), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_bar)->api->on_key_event(EGUI_VIEW_OF(&test_bar), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_bar)->api->on_key_event(EGUI_VIEW_OF(&test_bar), &event);
    return handled;
}

static void test_annotated_scroll_bar_clamps_metrics_and_regions(void)
{
    egui_region_t region;

    setup_bar(10, 5, 3, 0, 0);
    egui_view_annotated_scroll_bar_set_content_metrics(EGUI_VIEW_OF(&test_bar), 0, 0);
    egui_view_annotated_scroll_bar_set_offset(EGUI_VIEW_OF(&test_bar), 90);
    layout_bar();
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_annotated_scroll_bar_get_marker_count(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_annotated_scroll_bar_get_content_length(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_annotated_scroll_bar_get_viewport_length(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_annotated_scroll_bar_get_max_offset(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_annotated_scroll_bar_get_offset(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_TRUE(egui_view_annotated_scroll_bar_get_marker_region(EGUI_VIEW_OF(&test_bar), 3, &region));
    EGUI_TEST_ASSERT_FALSE(egui_view_annotated_scroll_bar_get_marker_region(EGUI_VIEW_OF(&test_bar), 4, &region));
}

static void test_annotated_scroll_bar_tab_cycles_parts(void)
{
    setup_bar(1000, 200, 180, 20, 120);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL, egui_view_annotated_scroll_bar_get_current_part(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_TRUE(egui_view_annotated_scroll_bar_handle_navigation_key(EGUI_VIEW_OF(&test_bar), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_INCREASE, egui_view_annotated_scroll_bar_get_current_part(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_TRUE(egui_view_annotated_scroll_bar_handle_navigation_key(EGUI_VIEW_OF(&test_bar), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_DECREASE, egui_view_annotated_scroll_bar_get_current_part(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_TRUE(egui_view_annotated_scroll_bar_handle_navigation_key(EGUI_VIEW_OF(&test_bar), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL, egui_view_annotated_scroll_bar_get_current_part(EGUI_VIEW_OF(&test_bar)));
}

static void test_annotated_scroll_bar_keyboard_navigation(void)
{
    setup_bar(1000, 200, 180, 20, 120);
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(200, egui_view_annotated_scroll_bar_get_offset(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_PLUS));
    EGUI_TEST_ASSERT_EQUAL_INT(320, egui_view_annotated_scroll_bar_get_offset(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_annotated_scroll_bar_get_offset(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(800, egui_view_annotated_scroll_bar_get_offset(EGUI_VIEW_OF(&test_bar)));
}

static void test_annotated_scroll_bar_touch_increase_button(void)
{
    egui_region_t region;

    setup_bar(1000, 200, 180, 20, 120);
    layout_bar();
    EGUI_TEST_ASSERT_TRUE(egui_view_annotated_scroll_bar_get_part_region(EGUI_VIEW_OF(&test_bar), EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_INCREASE, &region));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(200, egui_view_annotated_scroll_bar_get_offset(EGUI_VIEW_OF(&test_bar)));
}

static void test_annotated_scroll_bar_touch_marker_selects_offset(void)
{
    egui_region_t region;

    setup_bar(1000, 200, 180, 20, 120);
    layout_bar();
    EGUI_TEST_ASSERT_TRUE(egui_view_annotated_scroll_bar_get_marker_region(EGUI_VIEW_OF(&test_bar), 2, &region));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(420, egui_view_annotated_scroll_bar_get_offset(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_annotated_scroll_bar_get_active_marker(EGUI_VIEW_OF(&test_bar)));
}

static void test_annotated_scroll_bar_touch_drag_rail_reaches_end(void)
{
    egui_region_t rail_region;
    egui_dim_t x;
    egui_dim_t start_y;

    setup_bar(1000, 200, 180, 20, 120);
    layout_bar();
    EGUI_TEST_ASSERT_TRUE(egui_view_annotated_scroll_bar_get_part_region(EGUI_VIEW_OF(&test_bar), EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL, &rail_region));
    x = rail_region.location.x + rail_region.size.width / 2;
    start_y = rail_region.location.y + 2;
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, start_y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x, rail_region.location.y + rail_region.size.height - 1));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, rail_region.location.y + rail_region.size.height - 1));
    EGUI_TEST_ASSERT_EQUAL_INT(800, egui_view_annotated_scroll_bar_get_offset(EGUI_VIEW_OF(&test_bar)));
}

static void test_annotated_scroll_bar_marker_drag_switches_to_rail(void)
{
    egui_region_t marker_region;
    egui_region_t rail_region;
    egui_dim_t marker_x;
    egui_dim_t marker_y;
    egui_dim_t rail_x;
    egui_dim_t rail_end_y;

    setup_bar(1000, 200, 180, 20, 120);
    layout_bar();
    EGUI_TEST_ASSERT_TRUE(egui_view_annotated_scroll_bar_get_marker_region(EGUI_VIEW_OF(&test_bar), 1, &marker_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_annotated_scroll_bar_get_part_region(EGUI_VIEW_OF(&test_bar), EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL, &rail_region));

    marker_x = marker_region.location.x + marker_region.size.width / 2;
    marker_y = marker_region.location.y + marker_region.size.height / 2;
    rail_x = rail_region.location.x + rail_region.size.width / 2;
    rail_end_y = rail_region.location.y + rail_region.size.height - 1;

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, marker_x, marker_y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, rail_x, rail_end_y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, rail_x, rail_end_y));
    EGUI_TEST_ASSERT_EQUAL_INT(800, egui_view_annotated_scroll_bar_get_offset(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_annotated_scroll_bar_get_active_marker(EGUI_VIEW_OF(&test_bar)));
}

static void test_annotated_scroll_bar_read_only_and_compact_ignore_input(void)
{
    egui_region_t region;

    setup_bar(1000, 200, 180, 20, 120);
    egui_view_annotated_scroll_bar_set_read_only_mode(EGUI_VIEW_OF(&test_bar), 1);
    layout_bar();
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_TRUE(egui_view_annotated_scroll_bar_get_part_region(EGUI_VIEW_OF(&test_bar), EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_INCREASE, &region));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(180, egui_view_annotated_scroll_bar_get_offset(EGUI_VIEW_OF(&test_bar)));

    setup_bar(1000, 200, 180, 20, 120);
    egui_view_annotated_scroll_bar_set_compact_mode(EGUI_VIEW_OF(&test_bar), 1);
    layout_bar();
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_TRUE(egui_view_annotated_scroll_bar_get_part_region(EGUI_VIEW_OF(&test_bar), EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL, &region));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(180, egui_view_annotated_scroll_bar_get_offset(EGUI_VIEW_OF(&test_bar)));
}

void test_annotated_scroll_bar_run(void)
{
    EGUI_TEST_SUITE_BEGIN(annotated_scroll_bar);
    EGUI_TEST_RUN(test_annotated_scroll_bar_clamps_metrics_and_regions);
    EGUI_TEST_RUN(test_annotated_scroll_bar_tab_cycles_parts);
    EGUI_TEST_RUN(test_annotated_scroll_bar_keyboard_navigation);
    EGUI_TEST_RUN(test_annotated_scroll_bar_touch_increase_button);
    EGUI_TEST_RUN(test_annotated_scroll_bar_touch_marker_selects_offset);
    EGUI_TEST_RUN(test_annotated_scroll_bar_touch_drag_rail_reaches_end);
    EGUI_TEST_RUN(test_annotated_scroll_bar_marker_drag_switches_to_rail);
    EGUI_TEST_RUN(test_annotated_scroll_bar_read_only_and_compact_ignore_input);
    EGUI_TEST_SUITE_END();
}
