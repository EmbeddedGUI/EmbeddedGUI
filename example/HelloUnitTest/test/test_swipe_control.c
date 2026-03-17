#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_swipe_control.h"

#include "../../HelloCustomWidgets/input/swipe_control/egui_view_swipe_control.h"
#include "../../HelloCustomWidgets/input/swipe_control/egui_view_swipe_control.c"

static egui_view_swipe_control_t test_swipe_control;

static const egui_view_swipe_control_item_t unit_item = {
        "Mail", "Inbox row", "Single row with reveal actions", "Ready", EGUI_COLOR_HEX(0xE9F4FF), EGUI_COLOR_HEX(0x2563EB)};
static const egui_view_swipe_control_action_t unit_start_action = {"Pin", "Keep", EGUI_COLOR_HEX(0x0F766E), EGUI_COLOR_HEX(0xFFFFFF)};
static const egui_view_swipe_control_action_t unit_end_action = {"Delete", "Remove", EGUI_COLOR_HEX(0xDC2626), EGUI_COLOR_HEX(0xFFFFFF)};

static void setup_swipe_control(void)
{
    egui_view_swipe_control_init(EGUI_VIEW_OF(&test_swipe_control));
    egui_view_set_size(EGUI_VIEW_OF(&test_swipe_control), 160, 92);
    egui_view_swipe_control_set_item(EGUI_VIEW_OF(&test_swipe_control), &unit_item);
    egui_view_swipe_control_set_actions(EGUI_VIEW_OF(&test_swipe_control), &unit_start_action, &unit_end_action);
    egui_view_swipe_control_set_reveal_state(EGUI_VIEW_OF(&test_swipe_control), EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE);
}

static void layout_swipe_control(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 160;
    region.size.height = 92;
    egui_view_layout(EGUI_VIEW_OF(&test_swipe_control), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_swipe_control)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_swipe_control)->api->on_touch_event(EGUI_VIEW_OF(&test_swipe_control), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_swipe_control)->api->on_key_event(EGUI_VIEW_OF(&test_swipe_control), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_swipe_control)->api->on_key_event(EGUI_VIEW_OF(&test_swipe_control), &event);
    return handled;
}

static void test_swipe_control_default_state(void)
{
    setup_swipe_control();
    EGUI_TEST_ASSERT_TRUE(egui_view_swipe_control_get_item(EGUI_VIEW_OF(&test_swipe_control)) != NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE, egui_view_swipe_control_get_reveal_state(EGUI_VIEW_OF(&test_swipe_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE, egui_view_swipe_control_get_current_part(EGUI_VIEW_OF(&test_swipe_control)));
}

static void test_swipe_control_keyboard_reveal_cycle(void)
{
    setup_swipe_control();
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SWIPE_CONTROL_REVEAL_START, egui_view_swipe_control_get_reveal_state(EGUI_VIEW_OF(&test_swipe_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SWIPE_CONTROL_PART_START_ACTION, egui_view_swipe_control_get_current_part(EGUI_VIEW_OF(&test_swipe_control)));

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SWIPE_CONTROL_REVEAL_END, egui_view_swipe_control_get_reveal_state(EGUI_VIEW_OF(&test_swipe_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SWIPE_CONTROL_PART_END_ACTION, egui_view_swipe_control_get_current_part(EGUI_VIEW_OF(&test_swipe_control)));

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ESCAPE));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE, egui_view_swipe_control_get_reveal_state(EGUI_VIEW_OF(&test_swipe_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE, egui_view_swipe_control_get_current_part(EGUI_VIEW_OF(&test_swipe_control)));
}

static void test_swipe_control_tab_cycles_parts(void)
{
    setup_swipe_control();
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SWIPE_CONTROL_PART_START_ACTION, egui_view_swipe_control_get_current_part(EGUI_VIEW_OF(&test_swipe_control)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SWIPE_CONTROL_PART_END_ACTION, egui_view_swipe_control_get_current_part(EGUI_VIEW_OF(&test_swipe_control)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE, egui_view_swipe_control_get_current_part(EGUI_VIEW_OF(&test_swipe_control)));
}

static void test_swipe_control_touch_swipe_reveals_actions(void)
{
    egui_region_t region;

    setup_swipe_control();
    layout_swipe_control();
    EGUI_TEST_ASSERT_TRUE(egui_view_swipe_control_get_part_region(EGUI_VIEW_OF(&test_swipe_control), EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE, &region));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(
            send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, region.location.x + region.size.width / 2 + 24, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2 + 24, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SWIPE_CONTROL_REVEAL_START, egui_view_swipe_control_get_reveal_state(EGUI_VIEW_OF(&test_swipe_control)));

    egui_view_swipe_control_set_reveal_state(EGUI_VIEW_OF(&test_swipe_control), EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE);
    layout_swipe_control();
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(
            send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, region.location.x + region.size.width / 2 - 24, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2 - 24, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SWIPE_CONTROL_REVEAL_END, egui_view_swipe_control_get_reveal_state(EGUI_VIEW_OF(&test_swipe_control)));
}

static void test_swipe_control_part_region_exposes_visible_action(void)
{
    egui_region_t region;

    setup_swipe_control();
    layout_swipe_control();
    EGUI_TEST_ASSERT_FALSE(egui_view_swipe_control_get_part_region(EGUI_VIEW_OF(&test_swipe_control), EGUI_VIEW_SWIPE_CONTROL_PART_START_ACTION, &region));

    egui_view_swipe_control_set_reveal_state(EGUI_VIEW_OF(&test_swipe_control), EGUI_VIEW_SWIPE_CONTROL_REVEAL_START);
    layout_swipe_control();
    EGUI_TEST_ASSERT_TRUE(egui_view_swipe_control_get_part_region(EGUI_VIEW_OF(&test_swipe_control), EGUI_VIEW_SWIPE_CONTROL_PART_START_ACTION, &region));
    EGUI_TEST_ASSERT_TRUE(region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(region.size.height > 0);
}

static void test_swipe_control_surface_tap_closes_reveal(void)
{
    egui_region_t region;

    setup_swipe_control();
    egui_view_swipe_control_set_reveal_state(EGUI_VIEW_OF(&test_swipe_control), EGUI_VIEW_SWIPE_CONTROL_REVEAL_START);
    layout_swipe_control();
    EGUI_TEST_ASSERT_TRUE(egui_view_swipe_control_get_part_region(EGUI_VIEW_OF(&test_swipe_control), EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE, &region));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE, egui_view_swipe_control_get_reveal_state(EGUI_VIEW_OF(&test_swipe_control)));
}

static void test_swipe_control_compact_and_read_only_ignore_input(void)
{
    egui_region_t region;

    setup_swipe_control();
    egui_view_swipe_control_set_compact_mode(EGUI_VIEW_OF(&test_swipe_control), 1);
    layout_swipe_control();
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_TRUE(egui_view_swipe_control_get_part_region(EGUI_VIEW_OF(&test_swipe_control), EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE, &region));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));

    setup_swipe_control();
    egui_view_swipe_control_set_read_only_mode(EGUI_VIEW_OF(&test_swipe_control), 1);
    layout_swipe_control();
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_TRUE(egui_view_swipe_control_get_part_region(EGUI_VIEW_OF(&test_swipe_control), EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE, &region));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
}

void test_swipe_control_run(void)
{
    EGUI_TEST_SUITE_BEGIN(swipe_control);
    EGUI_TEST_RUN(test_swipe_control_default_state);
    EGUI_TEST_RUN(test_swipe_control_keyboard_reveal_cycle);
    EGUI_TEST_RUN(test_swipe_control_tab_cycles_parts);
    EGUI_TEST_RUN(test_swipe_control_touch_swipe_reveals_actions);
    EGUI_TEST_RUN(test_swipe_control_part_region_exposes_visible_action);
    EGUI_TEST_RUN(test_swipe_control_surface_tap_closes_reveal);
    EGUI_TEST_RUN(test_swipe_control_compact_and_read_only_ignore_input);
    EGUI_TEST_SUITE_END();
}
