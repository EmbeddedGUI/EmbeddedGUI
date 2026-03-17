#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_flip_view.h"

#include "../../HelloCustomWidgets/navigation/flip_view/egui_view_flip_view.h"
#include "../../HelloCustomWidgets/navigation/flip_view/egui_view_flip_view.c"

static egui_view_flip_view_t test_flip_view;

static const egui_view_flip_view_item_t unit_items[] = {
        {"One", "North deck", "Primary hero card", "Step 1", EGUI_COLOR_HEX(0xE4F0FF), EGUI_COLOR_HEX(0x2563EB)},
        {"Two", "South deck", "Secondary hero card", "Step 2", EGUI_COLOR_HEX(0xE8F5EE), EGUI_COLOR_HEX(0x0F766E)},
        {"Three", "Review deck", "Third hero card", "Step 3", EGUI_COLOR_HEX(0xF8ECDC), EGUI_COLOR_HEX(0xD97706)},
        {"Four", "Archive deck", "Fourth hero card", "Step 4", EGUI_COLOR_HEX(0xF3E8FF), EGUI_COLOR_HEX(0x8B5CF6)},
};

static void setup_flip_view(uint8_t item_count, uint8_t current_index)
{
    egui_view_flip_view_init(EGUI_VIEW_OF(&test_flip_view));
    egui_view_set_size(EGUI_VIEW_OF(&test_flip_view), 150, 88);
    egui_view_flip_view_set_items(EGUI_VIEW_OF(&test_flip_view), unit_items, item_count, current_index);
    egui_view_flip_view_set_current_part(EGUI_VIEW_OF(&test_flip_view), EGUI_VIEW_FLIP_VIEW_PART_SURFACE);
}

static void layout_flip_view(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 150;
    region.size.height = 88;
    egui_view_layout(EGUI_VIEW_OF(&test_flip_view), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_flip_view)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_flip_view)->api->on_touch_event(EGUI_VIEW_OF(&test_flip_view), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_flip_view)->api->on_key_event(EGUI_VIEW_OF(&test_flip_view), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_flip_view)->api->on_key_event(EGUI_VIEW_OF(&test_flip_view), &event);
    return handled;
}

static void test_flip_view_clamps_items_and_current_index(void)
{
    setup_flip_view(3, 9);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_flip_view_get_item_count(EGUI_VIEW_OF(&test_flip_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_flip_view_get_current_index(EGUI_VIEW_OF(&test_flip_view)));
    EGUI_TEST_ASSERT_TRUE(egui_view_flip_view_get_current_item(EGUI_VIEW_OF(&test_flip_view)) != NULL);
}

static void test_flip_view_tab_cycles_parts(void)
{
    setup_flip_view(4, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_FLIP_VIEW_PART_SURFACE, egui_view_flip_view_get_current_part(EGUI_VIEW_OF(&test_flip_view)));
    EGUI_TEST_ASSERT_TRUE(egui_view_flip_view_handle_navigation_key(EGUI_VIEW_OF(&test_flip_view), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_FLIP_VIEW_PART_NEXT, egui_view_flip_view_get_current_part(EGUI_VIEW_OF(&test_flip_view)));
    EGUI_TEST_ASSERT_TRUE(egui_view_flip_view_handle_navigation_key(EGUI_VIEW_OF(&test_flip_view), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS, egui_view_flip_view_get_current_part(EGUI_VIEW_OF(&test_flip_view)));
    EGUI_TEST_ASSERT_TRUE(egui_view_flip_view_handle_navigation_key(EGUI_VIEW_OF(&test_flip_view), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_FLIP_VIEW_PART_SURFACE, egui_view_flip_view_get_current_part(EGUI_VIEW_OF(&test_flip_view)));
}

static void test_flip_view_keyboard_navigation(void)
{
    setup_flip_view(4, 1);
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_flip_view_get_current_index(EGUI_VIEW_OF(&test_flip_view)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_flip_view_get_current_index(EGUI_VIEW_OF(&test_flip_view)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_flip_view_get_current_index(EGUI_VIEW_OF(&test_flip_view)));
}

static void test_flip_view_plus_minus_steps_items(void)
{
    setup_flip_view(4, 2);
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_PLUS));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_flip_view_get_current_index(EGUI_VIEW_OF(&test_flip_view)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_MINUS));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_flip_view_get_current_index(EGUI_VIEW_OF(&test_flip_view)));
}

static void test_flip_view_touch_previous_next(void)
{
    egui_region_t region;

    setup_flip_view(4, 2);
    layout_flip_view();

    EGUI_TEST_ASSERT_TRUE(egui_view_flip_view_get_part_region(EGUI_VIEW_OF(&test_flip_view), EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS, &region));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_flip_view_get_current_index(EGUI_VIEW_OF(&test_flip_view)));

    EGUI_TEST_ASSERT_TRUE(egui_view_flip_view_get_part_region(EGUI_VIEW_OF(&test_flip_view), EGUI_VIEW_FLIP_VIEW_PART_NEXT, &region));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_flip_view_get_current_index(EGUI_VIEW_OF(&test_flip_view)));
}

static void test_flip_view_surface_region_exists(void)
{
    egui_region_t region;

    setup_flip_view(4, 1);
    layout_flip_view();
    EGUI_TEST_ASSERT_TRUE(egui_view_flip_view_get_part_region(EGUI_VIEW_OF(&test_flip_view), EGUI_VIEW_FLIP_VIEW_PART_SURFACE, &region));
    EGUI_TEST_ASSERT_TRUE(region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(region.size.height > 0);
}

static void test_flip_view_compact_and_read_only_ignore_input(void)
{
    egui_region_t region;

    setup_flip_view(4, 1);
    egui_view_flip_view_set_read_only_mode(EGUI_VIEW_OF(&test_flip_view), 1);
    layout_flip_view();
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_TRUE(egui_view_flip_view_get_part_region(EGUI_VIEW_OF(&test_flip_view), EGUI_VIEW_FLIP_VIEW_PART_NEXT, &region));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_flip_view_get_current_index(EGUI_VIEW_OF(&test_flip_view)));

    setup_flip_view(4, 1);
    egui_view_flip_view_set_compact_mode(EGUI_VIEW_OF(&test_flip_view), 1);
    layout_flip_view();
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_TRUE(egui_view_flip_view_get_part_region(EGUI_VIEW_OF(&test_flip_view), EGUI_VIEW_FLIP_VIEW_PART_NEXT, &region));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_flip_view_get_current_index(EGUI_VIEW_OF(&test_flip_view)));
}

void test_flip_view_run(void)
{
    EGUI_TEST_SUITE_BEGIN(flip_view);
    EGUI_TEST_RUN(test_flip_view_clamps_items_and_current_index);
    EGUI_TEST_RUN(test_flip_view_tab_cycles_parts);
    EGUI_TEST_RUN(test_flip_view_keyboard_navigation);
    EGUI_TEST_RUN(test_flip_view_plus_minus_steps_items);
    EGUI_TEST_RUN(test_flip_view_touch_previous_next);
    EGUI_TEST_RUN(test_flip_view_surface_region_exists);
    EGUI_TEST_RUN(test_flip_view_compact_and_read_only_ignore_input);
    EGUI_TEST_SUITE_END();
}