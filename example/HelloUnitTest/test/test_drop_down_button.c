#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_drop_down_button.h"

#include "../../HelloCustomWidgets/input/drop_down_button/egui_view_drop_down_button.h"
#include "../../HelloCustomWidgets/input/drop_down_button/egui_view_drop_down_button.c"

static egui_view_drop_down_button_t test_button;
static int click_count;

static const egui_view_drop_down_button_snapshot_t test_snapshots[] = {
        {"Standard", "SO", "Sort", "Sort by name", "v", EGUI_VIEW_DROP_DOWN_BUTTON_TONE_ACCENT, 1},
        {"Standard", "LY", "Layout", "Switch layout", "v", EGUI_VIEW_DROP_DOWN_BUTTON_TONE_NEUTRAL, 0},
        {"Standard", "TH", "Theme", "Choose theme", "v", EGUI_VIEW_DROP_DOWN_BUTTON_TONE_WARNING, 0},
};

static void on_button_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    click_count++;
}

static void setup_button(void)
{
    egui_view_drop_down_button_init(EGUI_VIEW_OF(&test_button));
    egui_view_set_size(EGUI_VIEW_OF(&test_button), 116, 76);
    egui_view_drop_down_button_set_snapshots(EGUI_VIEW_OF(&test_button), test_snapshots, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&test_button), on_button_click);
    click_count = 0;
}

static void layout_button(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 116;
    region.size.height = 76;
    egui_view_layout(EGUI_VIEW_OF(&test_button), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_button)->region_screen, &region);
}

static int send_touch(uint8_t type)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = 40;
    event.location.y = 40;
    return EGUI_VIEW_OF(&test_button)->api->on_touch_event(EGUI_VIEW_OF(&test_button), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_button)->api->on_key_event(EGUI_VIEW_OF(&test_button), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_button)->api->on_key_event(EGUI_VIEW_OF(&test_button), &event);
    return handled;
}

static void test_drop_down_button_snapshot_switching(void)
{
    setup_button();
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_drop_down_button_get_current_snapshot(EGUI_VIEW_OF(&test_button)));
    egui_view_drop_down_button_set_current_snapshot(EGUI_VIEW_OF(&test_button), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_drop_down_button_get_current_snapshot(EGUI_VIEW_OF(&test_button)));
    egui_view_drop_down_button_set_current_snapshot(EGUI_VIEW_OF(&test_button), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_drop_down_button_get_current_snapshot(EGUI_VIEW_OF(&test_button)));
}

static void test_drop_down_button_touch_click_listener(void)
{
    setup_button();
    layout_button();
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_EQUAL_INT(1, click_count);
    EGUI_TEST_ASSERT_FALSE(egui_view_get_pressed(EGUI_VIEW_OF(&test_button)));
}

static void test_drop_down_button_keyboard_enter_click_listener(void)
{
    setup_button();
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(1, click_count);
}

static void test_drop_down_button_read_only_ignores_input(void)
{
    setup_button();
    egui_view_drop_down_button_set_read_only_mode(EGUI_VIEW_OF(&test_button), 1);
    layout_button();
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(0, click_count);
}

static void test_drop_down_button_compact_mode_keeps_snapshot_state(void)
{
    setup_button();
    egui_view_drop_down_button_set_compact_mode(EGUI_VIEW_OF(&test_button), 1);
    egui_view_drop_down_button_set_current_snapshot(EGUI_VIEW_OF(&test_button), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_drop_down_button_get_current_snapshot(EGUI_VIEW_OF(&test_button)));
}

void test_drop_down_button_run(void)
{
    EGUI_TEST_SUITE_BEGIN(drop_down_button);
    EGUI_TEST_RUN(test_drop_down_button_snapshot_switching);
    EGUI_TEST_RUN(test_drop_down_button_touch_click_listener);
    EGUI_TEST_RUN(test_drop_down_button_keyboard_enter_click_listener);
    EGUI_TEST_RUN(test_drop_down_button_read_only_ignores_input);
    EGUI_TEST_RUN(test_drop_down_button_compact_mode_keeps_snapshot_state);
    EGUI_TEST_SUITE_END();
}
