#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_shortcut_recorder.h"

#include "../../HelloCustomWidgets/input/shortcut_recorder/egui_view_shortcut_recorder.h"
#include "../../HelloCustomWidgets/input/shortcut_recorder/egui_view_shortcut_recorder.c"

static egui_view_shortcut_recorder_t test_recorder;
static uint8_t changed_count;
static uint8_t last_part;
static uint8_t last_preset;

static const egui_view_shortcut_recorder_preset_t test_presets[] = {
        {"Search files", "Workspace", EGUI_KEY_CODE_F, 1, 1},
        {"Command bar", "Quick command", EGUI_KEY_CODE_P, 1, 1},
        {"Pin focus", "One tap", EGUI_KEY_CODE_1, 0, 1},
};

static void on_changed(egui_view_t *self, uint8_t part, uint8_t preset_index)
{
    EGUI_UNUSED(self);
    changed_count++;
    last_part = part;
    last_preset = preset_index;
}

static void setup_recorder(void)
{
    egui_view_shortcut_recorder_init(EGUI_VIEW_OF(&test_recorder));
    egui_view_set_size(EGUI_VIEW_OF(&test_recorder), 160, 96);
    egui_view_shortcut_recorder_set_presets(EGUI_VIEW_OF(&test_recorder), test_presets, 3);
    egui_view_shortcut_recorder_set_binding(EGUI_VIEW_OF(&test_recorder), EGUI_KEY_CODE_K, 0, 1);
    egui_view_shortcut_recorder_set_on_changed_listener(EGUI_VIEW_OF(&test_recorder), on_changed);
    changed_count = 0;
    last_part = EGUI_VIEW_SHORTCUT_RECORDER_PART_NONE;
    last_preset = 0;
}

static void layout_recorder(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 160;
    region.size.height = 96;
    egui_view_layout(EGUI_VIEW_OF(&test_recorder), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_recorder)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_recorder)->api->on_touch_event(EGUI_VIEW_OF(&test_recorder), &event);
}

static int send_key(uint8_t key_code, uint8_t is_shift, uint8_t is_ctrl)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    event.is_shift = is_shift ? 1 : 0;
    event.is_ctrl = is_ctrl ? 1 : 0;
    handled |= EGUI_VIEW_OF(&test_recorder)->api->on_key_event(EGUI_VIEW_OF(&test_recorder), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_recorder)->api->on_key_event(EGUI_VIEW_OF(&test_recorder), &event);
    return handled;
}

static void test_shortcut_recorder_enter_listening_and_capture_binding(void)
{
    setup_recorder();

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER, 0, 0));
    EGUI_TEST_ASSERT_TRUE(egui_view_shortcut_recorder_is_listening(EGUI_VIEW_OF(&test_recorder)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_P, 1, 1));
    EGUI_TEST_ASSERT_FALSE(egui_view_shortcut_recorder_is_listening(EGUI_VIEW_OF(&test_recorder)));
    EGUI_TEST_ASSERT_TRUE(egui_view_shortcut_recorder_has_binding(EGUI_VIEW_OF(&test_recorder)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_KEY_CODE_P, egui_view_shortcut_recorder_get_key_code(EGUI_VIEW_OF(&test_recorder)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_shortcut_recorder_get_is_shift(EGUI_VIEW_OF(&test_recorder)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_shortcut_recorder_get_is_ctrl(EGUI_VIEW_OF(&test_recorder)));
    EGUI_TEST_ASSERT_TRUE(changed_count >= 2);
}

static void test_shortcut_recorder_tab_and_apply_preset(void)
{
    setup_recorder();

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB, 0, 0));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET, egui_view_shortcut_recorder_get_current_part(EGUI_VIEW_OF(&test_recorder)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_DOWN, 0, 0));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_shortcut_recorder_get_current_preset(EGUI_VIEW_OF(&test_recorder)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_SPACE, 0, 0));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_KEY_CODE_P, egui_view_shortcut_recorder_get_key_code(EGUI_VIEW_OF(&test_recorder)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_shortcut_recorder_get_is_shift(EGUI_VIEW_OF(&test_recorder)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_shortcut_recorder_get_is_ctrl(EGUI_VIEW_OF(&test_recorder)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET, last_part);
    EGUI_TEST_ASSERT_EQUAL_INT(1, last_preset);
}

static void test_shortcut_recorder_touch_preset_and_clear_binding(void)
{
    egui_region_t region;

    setup_recorder();
    layout_recorder();
    EGUI_TEST_ASSERT_TRUE(egui_view_shortcut_recorder_get_part_region(EGUI_VIEW_OF(&test_recorder), EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET, 2, &region));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_KEY_CODE_1, egui_view_shortcut_recorder_get_key_code(EGUI_VIEW_OF(&test_recorder)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_shortcut_recorder_get_is_ctrl(EGUI_VIEW_OF(&test_recorder)));

    EGUI_TEST_ASSERT_TRUE(egui_view_shortcut_recorder_get_part_region(EGUI_VIEW_OF(&test_recorder), EGUI_VIEW_SHORTCUT_RECORDER_PART_CLEAR, 0, &region));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_FALSE(egui_view_shortcut_recorder_has_binding(EGUI_VIEW_OF(&test_recorder)));
}

static void test_shortcut_recorder_escape_cancels_listening(void)
{
    setup_recorder();

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER, 0, 0));
    EGUI_TEST_ASSERT_TRUE(egui_view_shortcut_recorder_is_listening(EGUI_VIEW_OF(&test_recorder)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ESCAPE, 0, 0));
    EGUI_TEST_ASSERT_FALSE(egui_view_shortcut_recorder_is_listening(EGUI_VIEW_OF(&test_recorder)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_KEY_CODE_K, egui_view_shortcut_recorder_get_key_code(EGUI_VIEW_OF(&test_recorder)));
}

static void test_shortcut_recorder_compact_and_read_only_ignore_input(void)
{
    egui_region_t region;

    setup_recorder();
    egui_view_shortcut_recorder_set_compact_mode(EGUI_VIEW_OF(&test_recorder), 1);
    layout_recorder();
    EGUI_TEST_ASSERT_TRUE(egui_view_shortcut_recorder_get_part_region(EGUI_VIEW_OF(&test_recorder), EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD, 0, &region));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER, 0, 0));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + 6, region.location.y + 6));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + 6, region.location.y + 6));

    setup_recorder();
    egui_view_shortcut_recorder_set_read_only_mode(EGUI_VIEW_OF(&test_recorder), 1);
    layout_recorder();
    EGUI_TEST_ASSERT_TRUE(egui_view_shortcut_recorder_get_part_region(EGUI_VIEW_OF(&test_recorder), EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD, 0, &region));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_TAB, 0, 0));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + 6, region.location.y + 6));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + 6, region.location.y + 6));
}

static void test_shortcut_recorder_regions_follow_state(void)
{
    egui_region_t region;

    setup_recorder();
    layout_recorder();
    EGUI_TEST_ASSERT_TRUE(egui_view_shortcut_recorder_get_part_region(EGUI_VIEW_OF(&test_recorder), EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD, 0, &region));
    EGUI_TEST_ASSERT_TRUE(region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(egui_view_shortcut_recorder_get_part_region(EGUI_VIEW_OF(&test_recorder), EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET, 0, &region));
    EGUI_TEST_ASSERT_TRUE(egui_view_shortcut_recorder_get_part_region(EGUI_VIEW_OF(&test_recorder), EGUI_VIEW_SHORTCUT_RECORDER_PART_CLEAR, 0, &region));

    egui_view_shortcut_recorder_clear_binding(EGUI_VIEW_OF(&test_recorder));
    EGUI_TEST_ASSERT_FALSE(egui_view_shortcut_recorder_get_part_region(EGUI_VIEW_OF(&test_recorder), EGUI_VIEW_SHORTCUT_RECORDER_PART_CLEAR, 0, &region));
}

void test_shortcut_recorder_run(void)
{
    EGUI_TEST_SUITE_BEGIN(shortcut_recorder);
    EGUI_TEST_RUN(test_shortcut_recorder_enter_listening_and_capture_binding);
    EGUI_TEST_RUN(test_shortcut_recorder_tab_and_apply_preset);
    EGUI_TEST_RUN(test_shortcut_recorder_touch_preset_and_clear_binding);
    EGUI_TEST_RUN(test_shortcut_recorder_escape_cancels_listening);
    EGUI_TEST_RUN(test_shortcut_recorder_compact_and_read_only_ignore_input);
    EGUI_TEST_RUN(test_shortcut_recorder_regions_follow_state);
    EGUI_TEST_SUITE_END();
}
