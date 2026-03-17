#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_pips_pager.h"

#include "../../HelloCustomWidgets/navigation/pips_pager/egui_view_pips_pager.h"
#include "../../HelloCustomWidgets/navigation/pips_pager/egui_view_pips_pager.c"

static egui_view_pips_pager_t test_pager;

static void setup_pager(uint8_t total_count, uint8_t current_index, uint8_t visible_count)
{
    egui_view_pips_pager_init(EGUI_VIEW_OF(&test_pager));
    egui_view_set_size(EGUI_VIEW_OF(&test_pager), 136, 82);
    egui_view_pips_pager_set_page_metrics(EGUI_VIEW_OF(&test_pager), total_count, current_index, visible_count);
    egui_view_pips_pager_set_current_part(EGUI_VIEW_OF(&test_pager), EGUI_VIEW_PIPS_PAGER_PART_PIP);
}

static void layout_pager(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 136;
    region.size.height = 82;
    egui_view_layout(EGUI_VIEW_OF(&test_pager), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_pager)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_pager)->api->on_touch_event(EGUI_VIEW_OF(&test_pager), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_pager)->api->on_key_event(EGUI_VIEW_OF(&test_pager), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_pager)->api->on_key_event(EGUI_VIEW_OF(&test_pager), &event);
    return handled;
}

static void test_pips_pager_clamps_metrics_and_regions(void)
{
    egui_region_t region;

    setup_pager(3, 9, 0);
    layout_pager();
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_pips_pager_get_total_count(EGUI_VIEW_OF(&test_pager)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_pips_pager_get_current_index(EGUI_VIEW_OF(&test_pager)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_pips_pager_get_visible_count(EGUI_VIEW_OF(&test_pager)));
    EGUI_TEST_ASSERT_TRUE(egui_view_pips_pager_get_part_region(EGUI_VIEW_OF(&test_pager), EGUI_VIEW_PIPS_PAGER_PART_PIP, 2, &region));
    EGUI_TEST_ASSERT_FALSE(egui_view_pips_pager_get_part_region(EGUI_VIEW_OF(&test_pager), EGUI_VIEW_PIPS_PAGER_PART_PIP, 3, &region));
}

static void test_pips_pager_tab_cycles_parts(void)
{
    setup_pager(8, 3, 5);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PIPS_PAGER_PART_PIP, egui_view_pips_pager_get_current_part(EGUI_VIEW_OF(&test_pager)));
    EGUI_TEST_ASSERT_TRUE(egui_view_pips_pager_handle_navigation_key(EGUI_VIEW_OF(&test_pager), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PIPS_PAGER_PART_NEXT, egui_view_pips_pager_get_current_part(EGUI_VIEW_OF(&test_pager)));
    EGUI_TEST_ASSERT_TRUE(egui_view_pips_pager_handle_navigation_key(EGUI_VIEW_OF(&test_pager), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PIPS_PAGER_PART_PREVIOUS, egui_view_pips_pager_get_current_part(EGUI_VIEW_OF(&test_pager)));
    EGUI_TEST_ASSERT_TRUE(egui_view_pips_pager_handle_navigation_key(EGUI_VIEW_OF(&test_pager), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PIPS_PAGER_PART_PIP, egui_view_pips_pager_get_current_part(EGUI_VIEW_OF(&test_pager)));
}

static void test_pips_pager_keyboard_navigation(void)
{
    setup_pager(8, 3, 5);
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_pips_pager_get_current_index(EGUI_VIEW_OF(&test_pager)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_pips_pager_get_current_index(EGUI_VIEW_OF(&test_pager)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(7, egui_view_pips_pager_get_current_index(EGUI_VIEW_OF(&test_pager)));
}

static void test_pips_pager_plus_minus_steps_pages(void)
{
    setup_pager(8, 3, 5);
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_PLUS));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_pips_pager_get_current_index(EGUI_VIEW_OF(&test_pager)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_MINUS));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_pips_pager_get_current_index(EGUI_VIEW_OF(&test_pager)));
}

static void test_pips_pager_touch_previous_next(void)
{
    egui_region_t region;

    setup_pager(8, 3, 5);
    layout_pager();
    EGUI_TEST_ASSERT_TRUE(egui_view_pips_pager_get_part_region(EGUI_VIEW_OF(&test_pager), EGUI_VIEW_PIPS_PAGER_PART_PREVIOUS, 0, &region));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_pips_pager_get_current_index(EGUI_VIEW_OF(&test_pager)));

    EGUI_TEST_ASSERT_TRUE(egui_view_pips_pager_get_part_region(EGUI_VIEW_OF(&test_pager), EGUI_VIEW_PIPS_PAGER_PART_NEXT, 0, &region));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_pips_pager_get_current_index(EGUI_VIEW_OF(&test_pager)));
}

static void test_pips_pager_touch_pip_selects_visible_page(void)
{
    egui_region_t region;

    setup_pager(8, 4, 5);
    layout_pager();
    EGUI_TEST_ASSERT_TRUE(egui_view_pips_pager_get_part_region(EGUI_VIEW_OF(&test_pager), EGUI_VIEW_PIPS_PAGER_PART_PIP, 5, &region));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_pips_pager_get_current_index(EGUI_VIEW_OF(&test_pager)));
}

static void test_pips_pager_read_only_and_compact_ignore_input(void)
{
    egui_region_t region;

    setup_pager(8, 3, 5);
    egui_view_pips_pager_set_read_only_mode(EGUI_VIEW_OF(&test_pager), 1);
    layout_pager();
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_TRUE(egui_view_pips_pager_get_part_region(EGUI_VIEW_OF(&test_pager), EGUI_VIEW_PIPS_PAGER_PART_NEXT, 0, &region));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_pips_pager_get_current_index(EGUI_VIEW_OF(&test_pager)));

    setup_pager(8, 3, 5);
    egui_view_pips_pager_set_compact_mode(EGUI_VIEW_OF(&test_pager), 1);
    layout_pager();
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_TRUE(egui_view_pips_pager_get_part_region(EGUI_VIEW_OF(&test_pager), EGUI_VIEW_PIPS_PAGER_PART_NEXT, 0, &region));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_pips_pager_get_current_index(EGUI_VIEW_OF(&test_pager)));
}

void test_pips_pager_run(void)
{
    EGUI_TEST_SUITE_BEGIN(pips_pager);
    EGUI_TEST_RUN(test_pips_pager_clamps_metrics_and_regions);
    EGUI_TEST_RUN(test_pips_pager_tab_cycles_parts);
    EGUI_TEST_RUN(test_pips_pager_keyboard_navigation);
    EGUI_TEST_RUN(test_pips_pager_plus_minus_steps_pages);
    EGUI_TEST_RUN(test_pips_pager_touch_previous_next);
    EGUI_TEST_RUN(test_pips_pager_touch_pip_selects_visible_page);
    EGUI_TEST_RUN(test_pips_pager_read_only_and_compact_ignore_input);
    EGUI_TEST_SUITE_END();
}
