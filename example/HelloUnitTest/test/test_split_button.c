#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_split_button.h"

#include "../../HelloCustomWidgets/input/split_button/egui_view_split_button.h"
#include "../../HelloCustomWidgets/input/split_button/egui_view_split_button.c"

static egui_view_split_button_t test_button;
static uint8_t changed_count;
static uint8_t last_part;

static const egui_view_split_button_snapshot_t g_snapshots[] = {
        {"Save draft", "SV", "Save", "Run save or open more", EGUI_VIEW_SPLIT_BUTTON_TONE_ACCENT, 1, 1, 1, EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY},
        {"Share handoff", "SH", "Share", "Send fast or choose route", EGUI_VIEW_SPLIT_BUTTON_TONE_ACCENT, 0, 1, 1, EGUI_VIEW_SPLIT_BUTTON_PART_MENU},
        {"Export file", "EX", "Export", "Export PDF or pick format", EGUI_VIEW_SPLIT_BUTTON_TONE_WARNING, 0, 0, 1, EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY},
        {"Locked", "LK", "Lock", "No actions available", EGUI_VIEW_SPLIT_BUTTON_TONE_NEUTRAL, 0, 0, 0, EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY},
};

static const egui_view_split_button_snapshot_t g_overflow_snapshots[] = {
        {"A", "A", "A", "A", EGUI_VIEW_SPLIT_BUTTON_TONE_ACCENT, 0, 1, 1, EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY},
        {"B", "B", "B", "B", EGUI_VIEW_SPLIT_BUTTON_TONE_ACCENT, 0, 1, 1, EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY},
        {"C", "C", "C", "C", EGUI_VIEW_SPLIT_BUTTON_TONE_ACCENT, 0, 1, 1, EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY},
        {"D", "D", "D", "D", EGUI_VIEW_SPLIT_BUTTON_TONE_ACCENT, 0, 1, 1, EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY},
        {"E", "E", "E", "E", EGUI_VIEW_SPLIT_BUTTON_TONE_ACCENT, 0, 1, 1, EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY},
        {"F", "F", "F", "F", EGUI_VIEW_SPLIT_BUTTON_TONE_ACCENT, 0, 1, 1, EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY},
        {"G", "G", "G", "G", EGUI_VIEW_SPLIT_BUTTON_TONE_ACCENT, 0, 1, 1, EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY},
};

static void reset_changed_state(void)
{
    changed_count = 0;
    last_part = EGUI_VIEW_SPLIT_BUTTON_PART_NONE;
}

static void on_part_changed(egui_view_t *self, uint8_t part)
{
    EGUI_UNUSED(self);
    changed_count++;
    last_part = part;
}

static void setup_button(void)
{
    egui_view_split_button_init(EGUI_VIEW_OF(&test_button));
    egui_view_set_size(EGUI_VIEW_OF(&test_button), 116, 88);
    egui_view_split_button_set_snapshots(EGUI_VIEW_OF(&test_button), g_snapshots, 4);
    egui_view_split_button_set_on_part_changed_listener(EGUI_VIEW_OF(&test_button), on_part_changed);
    reset_changed_state();
}

static void layout_button(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 116;
    region.size.height = 88;
    egui_view_layout(EGUI_VIEW_OF(&test_button), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_button)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
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

static void test_split_button_set_snapshots_clamp_and_resolve_default_part(void)
{
    setup_button();

    egui_view_split_button_set_snapshots(EGUI_VIEW_OF(&test_button), g_overflow_snapshots, 7);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_MAX_SNAPSHOTS, test_button.snapshot_count);

    test_button.current_snapshot = 5;
    egui_view_split_button_set_snapshots(EGUI_VIEW_OF(&test_button), g_snapshots, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_button.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_split_button_get_current_snapshot(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));

    egui_view_split_button_set_snapshots(EGUI_VIEW_OF(&test_button), g_snapshots, 4);
    egui_view_split_button_set_current_snapshot(EGUI_VIEW_OF(&test_button), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_MENU, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));

    egui_view_split_button_set_current_snapshot(EGUI_VIEW_OF(&test_button), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_MENU, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));

    egui_view_split_button_set_current_snapshot(EGUI_VIEW_OF(&test_button), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_NONE, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));
}

static void test_split_button_snapshot_and_part_guards(void)
{
    setup_button();

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));
    egui_view_split_button_set_current_snapshot(EGUI_VIEW_OF(&test_button), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_split_button_get_current_snapshot(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_MENU, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));

    egui_view_split_button_set_current_snapshot(EGUI_VIEW_OF(&test_button), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_split_button_get_current_snapshot(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);

    egui_view_split_button_set_current_part(EGUI_VIEW_OF(&test_button), EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY, last_part);

    egui_view_split_button_set_current_part(EGUI_VIEW_OF(&test_button), EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY);
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);

    egui_view_split_button_set_current_snapshot(EGUI_VIEW_OF(&test_button), 2);
    reset_changed_state();
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_MENU, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));
    egui_view_split_button_set_current_part(EGUI_VIEW_OF(&test_button), EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_MENU, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);
}

static void test_split_button_font_modes_and_palette(void)
{
    setup_button();

    egui_view_split_button_set_font(EGUI_VIEW_OF(&test_button), NULL);
    egui_view_split_button_set_meta_font(EGUI_VIEW_OF(&test_button), NULL);
    EGUI_TEST_ASSERT_TRUE(test_button.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_button.meta_font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    test_button.pressed_part = EGUI_VIEW_SPLIT_BUTTON_PART_MENU;
    egui_view_split_button_set_compact_mode(EGUI_VIEW_OF(&test_button), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_button.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_NONE, test_button.pressed_part);

    test_button.pressed_part = EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY;
    egui_view_split_button_set_disabled_mode(EGUI_VIEW_OF(&test_button), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_button.disabled_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_NONE, test_button.pressed_part);

    egui_view_split_button_set_palette(EGUI_VIEW_OF(&test_button), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                       EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152), EGUI_COLOR_HEX(0x606162), EGUI_COLOR_HEX(0x707172),
                                       EGUI_COLOR_HEX(0x808182), EGUI_COLOR_HEX(0x909192));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_button.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_button.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_button.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_button.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_button.accent_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, test_button.success_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x707172).full, test_button.warning_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full, test_button.danger_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x909192).full, test_button.neutral_color.full);
}

static void test_split_button_touch_switches_part_and_notifies(void)
{
    egui_view_split_button_metrics_t metrics;
    const egui_view_split_button_snapshot_t *snapshot;
    egui_dim_t primary_x;
    egui_dim_t primary_y;
    egui_dim_t menu_x;
    egui_dim_t menu_y;

    setup_button();
    egui_view_split_button_set_current_snapshot(EGUI_VIEW_OF(&test_button), 1);
    reset_changed_state();
    layout_button();
    snapshot = egui_view_split_button_get_snapshot(&test_button);
    egui_view_split_button_get_metrics(&test_button, EGUI_VIEW_OF(&test_button), snapshot, &metrics);

    primary_x = metrics.primary_region.location.x + metrics.primary_region.size.width / 2;
    primary_y = metrics.primary_region.location.y + metrics.primary_region.size.height / 2;
    menu_x = metrics.menu_region.location.x + metrics.menu_region.size.width / 2;
    menu_y = metrics.menu_region.location.y + metrics.menu_region.size.height / 2;

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY,
                               egui_view_split_button_hit_part(&test_button, EGUI_VIEW_OF(&test_button), primary_x, primary_y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_MENU, egui_view_split_button_hit_part(&test_button, EGUI_VIEW_OF(&test_button), menu_x, menu_y));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, primary_x, primary_y));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_button)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY, test_button.pressed_part);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, primary_x, primary_y));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_button)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_NONE, test_button.pressed_part);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY, last_part);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, menu_x, menu_y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_MENU, test_button.pressed_part);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, menu_x, menu_y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_MENU, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_MENU, last_part);
}

static void test_split_button_touch_cancel_clears_pressed_state(void)
{
    egui_view_split_button_metrics_t metrics;
    const egui_view_split_button_snapshot_t *snapshot;
    egui_dim_t primary_x;
    egui_dim_t primary_y;

    setup_button();
    layout_button();
    snapshot = egui_view_split_button_get_snapshot(&test_button);
    egui_view_split_button_get_metrics(&test_button, EGUI_VIEW_OF(&test_button), snapshot, &metrics);

    primary_x = metrics.primary_region.location.x + metrics.primary_region.size.width / 2;
    primary_y = metrics.primary_region.location.y + metrics.primary_region.size.height / 2;

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, primary_x, primary_y));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_button)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY, test_button.pressed_part);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_CANCEL, primary_x, primary_y));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_button)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_NONE, test_button.pressed_part);
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);
}

static void test_split_button_keyboard_navigation_and_fallback(void)
{
    setup_button();

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_MENU, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_MENU, last_part);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_MENU, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_MENU, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, changed_count);

    egui_view_split_button_set_current_snapshot(EGUI_VIEW_OF(&test_button), 2);
    reset_changed_state();
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_MENU, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_MENU, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_MENU, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);
}

static void test_split_button_disabled_mode_ignores_input(void)
{
    egui_view_split_button_metrics_t metrics;
    const egui_view_split_button_snapshot_t *snapshot;
    egui_dim_t primary_x;
    egui_dim_t primary_y;

    setup_button();
    egui_view_split_button_set_disabled_mode(EGUI_VIEW_OF(&test_button), 1);
    layout_button();
    snapshot = egui_view_split_button_get_snapshot(&test_button);
    egui_view_split_button_get_metrics(&test_button, EGUI_VIEW_OF(&test_button), snapshot, &metrics);
    primary_x = metrics.primary_region.location.x + metrics.primary_region.size.width / 2;
    primary_y = metrics.primary_region.location.y + metrics.primary_region.size.height / 2;

    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, primary_x, primary_y));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, primary_x, primary_y));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY, egui_view_split_button_get_current_part(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);
}

void test_split_button_run(void)
{
    EGUI_TEST_SUITE_BEGIN(split_button);
    EGUI_TEST_RUN(test_split_button_set_snapshots_clamp_and_resolve_default_part);
    EGUI_TEST_RUN(test_split_button_snapshot_and_part_guards);
    EGUI_TEST_RUN(test_split_button_font_modes_and_palette);
    EGUI_TEST_RUN(test_split_button_touch_switches_part_and_notifies);
    EGUI_TEST_RUN(test_split_button_touch_cancel_clears_pressed_state);
    EGUI_TEST_RUN(test_split_button_keyboard_navigation_and_fallback);
    EGUI_TEST_RUN(test_split_button_disabled_mode_ignores_input);
    EGUI_TEST_SUITE_END();
}
