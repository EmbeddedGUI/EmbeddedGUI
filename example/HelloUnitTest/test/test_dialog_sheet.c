#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_dialog_sheet.h"

#include "../../HelloCustomWidgets/feedback/dialog_sheet/egui_view_dialog_sheet.h"
#include "../../HelloCustomWidgets/feedback/dialog_sheet/egui_view_dialog_sheet.c"

static egui_view_dialog_sheet_t test_sheet;
static uint8_t changed_count;
static uint8_t last_action;

static const egui_view_dialog_sheet_snapshot_t g_snapshots[] = {
        {"Sync issue", "Reconnect account?", "Resume sync for review.", "Reconnect", "Later", "Sync", "Queue paused", EGUI_VIEW_DIALOG_SHEET_TONE_WARNING, 1, 1,
         EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY},
        {"Delete draft", "Delete unfinished layout?", "Remove local draft.", "Delete", "Cancel", "Draft", "Cannot undo", EGUI_VIEW_DIALOG_SHEET_TONE_ERROR, 1,
         1, EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY},
        {"Template", "Apply starter scene?", "Load base panels.", "Apply", NULL, "Template", "Saved", EGUI_VIEW_DIALOG_SHEET_TONE_ACCENT, 0, 0,
         EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY},
        {"Publishing", "Send build to review?", "Share build now.", "Send", NULL, "Review", "Ready", EGUI_VIEW_DIALOG_SHEET_TONE_SUCCESS, 0, 1,
         EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY},
};

static const egui_view_dialog_sheet_snapshot_t g_overflow_snapshots[] = {
        {"A", "A", "A", "A", "B", "A", "A", EGUI_VIEW_DIALOG_SHEET_TONE_ACCENT, 1, 1, EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY},
        {"B", "B", "B", "B", "C", "B", "B", EGUI_VIEW_DIALOG_SHEET_TONE_SUCCESS, 1, 1, EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY},
        {"C", "C", "C", "C", "D", "C", "C", EGUI_VIEW_DIALOG_SHEET_TONE_WARNING, 1, 1, EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY},
        {"D", "D", "D", "D", "E", "D", "D", EGUI_VIEW_DIALOG_SHEET_TONE_ERROR, 1, 1, EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY},
        {"E", "E", "E", "E", "F", "E", "E", EGUI_VIEW_DIALOG_SHEET_TONE_NEUTRAL, 1, 1, EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY},
        {"F", "F", "F", "F", "G", "F", "F", EGUI_VIEW_DIALOG_SHEET_TONE_ACCENT, 1, 1, EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY},
        {"G", "G", "G", "G", "H", "G", "G", EGUI_VIEW_DIALOG_SHEET_TONE_SUCCESS, 1, 1, EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY},
};

static const egui_view_dialog_sheet_snapshot_t g_secondary_only_snapshot = {
        "Warn",
        "Secondary only",
        "Fallback keeps the right action.",
        NULL,
        "Later",
        "Queue",
        "",
        EGUI_VIEW_DIALOG_SHEET_TONE_WARNING,
        1,
        1,
        EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY,
};

static const egui_view_dialog_sheet_snapshot_t g_no_action_snapshot = {
        "Lock", "Read only", "No actions remain.", NULL, NULL, "Locked", "", EGUI_VIEW_DIALOG_SHEET_TONE_NEUTRAL, 0, 0, EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY,
};

static const egui_view_dialog_sheet_snapshot_t g_invalid_secondary_snapshot = {
        "Warn",
        "Invalid secondary",
        "Secondary flag without label.",
        "Retry",
        NULL,
        "Sync",
        "",
        EGUI_VIEW_DIALOG_SHEET_TONE_WARNING,
        1,
        1,
        EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY,
};

static void reset_changed_state(void)
{
    changed_count = 0;
    last_action = EGUI_VIEW_DIALOG_SHEET_ACTION_NONE;
}

static void on_action_changed(egui_view_t *self, uint8_t action_index)
{
    EGUI_UNUSED(self);
    changed_count++;
    last_action = action_index;
}

static void setup_sheet(void)
{
    egui_view_dialog_sheet_init(EGUI_VIEW_OF(&test_sheet));
    egui_view_set_size(EGUI_VIEW_OF(&test_sheet), 196, 132);
    egui_view_dialog_sheet_set_snapshots(EGUI_VIEW_OF(&test_sheet), g_snapshots, 4);
    egui_view_dialog_sheet_set_on_action_changed_listener(EGUI_VIEW_OF(&test_sheet), on_action_changed);
    reset_changed_state();
}

static void layout_sheet(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 196;
    region.size.height = 132;
    egui_view_layout(EGUI_VIEW_OF(&test_sheet), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_sheet)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_sheet)->api->on_touch_event(EGUI_VIEW_OF(&test_sheet), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_sheet)->api->on_key_event(EGUI_VIEW_OF(&test_sheet), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_sheet)->api->on_key_event(EGUI_VIEW_OF(&test_sheet), &event);
    return handled;
}

static void get_metrics(egui_view_dialog_sheet_metrics_t *metrics)
{
    const egui_view_dialog_sheet_snapshot_t *snapshot = egui_view_dialog_sheet_get_snapshot(&test_sheet);
    uint8_t show_secondary;
    uint8_t show_close;

    EGUI_TEST_ASSERT_NOT_NULL(snapshot);
    show_secondary = egui_view_dialog_sheet_has_secondary(snapshot);
    show_close = snapshot->show_close && !test_sheet.compact_mode && !test_sheet.locked_mode;
    egui_view_dialog_sheet_get_metrics(&test_sheet, EGUI_VIEW_OF(&test_sheet), show_secondary, show_close, metrics);
}

static void test_dialog_sheet_set_snapshots_clamp_and_reset_state(void)
{
    setup_sheet();

    egui_view_dialog_sheet_set_snapshots(EGUI_VIEW_OF(&test_sheet), g_overflow_snapshots, 7);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_MAX_SNAPSHOTS, test_sheet.snapshot_count);

    test_sheet.current_snapshot = 5;
    test_sheet.current_action = EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY;
    test_sheet.pressed_action = EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY;
    egui_view_dialog_sheet_set_snapshots(EGUI_VIEW_OF(&test_sheet), g_snapshots, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_sheet.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_dialog_sheet_get_current_snapshot(EGUI_VIEW_OF(&test_sheet)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY, egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&test_sheet)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_NONE, test_sheet.pressed_action);
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);

    egui_view_dialog_sheet_set_snapshots(EGUI_VIEW_OF(&test_sheet), &g_no_action_snapshot, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_NONE, egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&test_sheet)));

    egui_view_dialog_sheet_set_snapshots(EGUI_VIEW_OF(&test_sheet), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_sheet.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_dialog_sheet_get_current_snapshot(EGUI_VIEW_OF(&test_sheet)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_NONE, egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&test_sheet)));
}

static void test_dialog_sheet_snapshot_and_action_guards_notify(void)
{
    setup_sheet();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_dialog_sheet_get_current_snapshot(EGUI_VIEW_OF(&test_sheet)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY, egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&test_sheet)));

    egui_view_dialog_sheet_set_current_action(EGUI_VIEW_OF(&test_sheet), EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY, egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&test_sheet)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY, last_action);

    egui_view_dialog_sheet_set_current_action(EGUI_VIEW_OF(&test_sheet), EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY);
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);

    reset_changed_state();
    test_sheet.pressed_action = EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY;
    egui_view_dialog_sheet_set_current_snapshot(EGUI_VIEW_OF(&test_sheet), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_dialog_sheet_get_current_snapshot(EGUI_VIEW_OF(&test_sheet)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY, egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&test_sheet)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_NONE, test_sheet.pressed_action);
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);

    egui_view_dialog_sheet_set_current_snapshot(EGUI_VIEW_OF(&test_sheet), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_dialog_sheet_get_current_snapshot(EGUI_VIEW_OF(&test_sheet)));

    egui_view_dialog_sheet_set_snapshots(EGUI_VIEW_OF(&test_sheet), &g_secondary_only_snapshot, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY, egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&test_sheet)));
    reset_changed_state();
    egui_view_dialog_sheet_set_current_action(EGUI_VIEW_OF(&test_sheet), EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY, egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&test_sheet)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);

    egui_view_dialog_sheet_set_snapshots(EGUI_VIEW_OF(&test_sheet), &g_no_action_snapshot, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_NONE, egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&test_sheet)));
    egui_view_dialog_sheet_set_current_action(EGUI_VIEW_OF(&test_sheet), EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_NONE, egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&test_sheet)));
}

static void test_dialog_sheet_font_modes_listener_and_palette(void)
{
    setup_sheet();

    egui_view_dialog_sheet_set_font(EGUI_VIEW_OF(&test_sheet), NULL);
    egui_view_dialog_sheet_set_meta_font(EGUI_VIEW_OF(&test_sheet), NULL);
    EGUI_TEST_ASSERT_TRUE(test_sheet.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_sheet.meta_font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_dialog_sheet_set_compact_mode(EGUI_VIEW_OF(&test_sheet), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_sheet.compact_mode);

    test_sheet.pressed_action = EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY;
    egui_view_dialog_sheet_set_locked_mode(EGUI_VIEW_OF(&test_sheet), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_sheet.locked_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_NONE, test_sheet.pressed_action);

    egui_view_dialog_sheet_set_locked_mode(EGUI_VIEW_OF(&test_sheet), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_sheet.locked_mode);

    egui_view_dialog_sheet_set_palette(EGUI_VIEW_OF(&test_sheet), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                       EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152), EGUI_COLOR_HEX(0x606162), EGUI_COLOR_HEX(0x707172),
                                       EGUI_COLOR_HEX(0x808182), EGUI_COLOR_HEX(0x909192), EGUI_COLOR_HEX(0xA0A1A2));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_sheet.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_sheet.overlay_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_sheet.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_sheet.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_sheet.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, test_sheet.accent_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x707172).full, test_sheet.success_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full, test_sheet.warning_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x909192).full, test_sheet.error_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0xA0A1A2).full, test_sheet.neutral_color.full);
}

static void test_dialog_sheet_touch_updates_action_and_hit_testing(void)
{
    egui_view_dialog_sheet_metrics_t metrics;
    egui_dim_t primary_x;
    egui_dim_t primary_y;
    egui_dim_t secondary_x;
    egui_dim_t secondary_y;

    setup_sheet();
    layout_sheet();
    get_metrics(&metrics);

    EGUI_TEST_ASSERT_TRUE(metrics.sheet_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.sheet_region.size.height > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.close_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.primary_action_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.secondary_action_region.size.width > 0);

    primary_x = metrics.primary_action_region.location.x + metrics.primary_action_region.size.width / 2;
    primary_y = metrics.primary_action_region.location.y + metrics.primary_action_region.size.height / 2;
    secondary_x = metrics.secondary_action_region.location.x + metrics.secondary_action_region.size.width / 2;
    secondary_y = metrics.secondary_action_region.location.y + metrics.secondary_action_region.size.height / 2;

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY,
                               egui_view_dialog_sheet_hit_action(&test_sheet, EGUI_VIEW_OF(&test_sheet), primary_x, primary_y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY,
                               egui_view_dialog_sheet_hit_action(&test_sheet, EGUI_VIEW_OF(&test_sheet), secondary_x, secondary_y));
    EGUI_TEST_ASSERT_EQUAL_INT(
            EGUI_VIEW_DIALOG_SHEET_ACTION_NONE,
            egui_view_dialog_sheet_hit_action(&test_sheet, EGUI_VIEW_OF(&test_sheet), metrics.sheet_region.location.x, metrics.sheet_region.location.y));

    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, metrics.sheet_region.location.x, metrics.sheet_region.location.y));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, secondary_x, secondary_y));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_sheet)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY, test_sheet.pressed_action);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, secondary_x, secondary_y));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_sheet)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_NONE, test_sheet.pressed_action);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY, egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&test_sheet)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY, last_action);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, primary_x, primary_y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, primary_x, primary_y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY, egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&test_sheet)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY, last_action);
}

static void test_dialog_sheet_touch_cancel_and_locked_or_disabled_ignore_input(void)
{
    egui_view_dialog_sheet_metrics_t metrics;
    egui_dim_t primary_x;
    egui_dim_t primary_y;

    setup_sheet();
    egui_view_dialog_sheet_set_current_action(EGUI_VIEW_OF(&test_sheet), EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY);
    reset_changed_state();
    layout_sheet();
    get_metrics(&metrics);

    primary_x = metrics.primary_action_region.location.x + metrics.primary_action_region.size.width / 2;
    primary_y = metrics.primary_action_region.location.y + metrics.primary_action_region.size.height / 2;

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, primary_x, primary_y));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_sheet)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY, test_sheet.pressed_action);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_CANCEL, primary_x, primary_y));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_sheet)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_NONE, test_sheet.pressed_action);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY, egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&test_sheet)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);

    egui_view_dialog_sheet_set_locked_mode(EGUI_VIEW_OF(&test_sheet), 1);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, primary_x, primary_y));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, primary_x, primary_y));

    egui_view_dialog_sheet_set_locked_mode(EGUI_VIEW_OF(&test_sheet), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_sheet), 0);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, primary_x, primary_y));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, primary_x, primary_y));
}

static void test_dialog_sheet_keyboard_navigation_and_guards(void)
{
    setup_sheet();
    reset_changed_state();

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY, egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&test_sheet)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY, last_action);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY, egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&test_sheet)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY, egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&test_sheet)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY, egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&test_sheet)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY, egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&test_sheet)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, changed_count);

    egui_view_dialog_sheet_set_current_snapshot(EGUI_VIEW_OF(&test_sheet), 2);
    reset_changed_state();
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY, egui_view_dialog_sheet_get_current_action(EGUI_VIEW_OF(&test_sheet)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);

    egui_view_dialog_sheet_set_locked_mode(EGUI_VIEW_OF(&test_sheet), 1);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_LEFT));
    egui_view_dialog_sheet_set_locked_mode(EGUI_VIEW_OF(&test_sheet), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_sheet), 0);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_LEFT));
}

static void test_dialog_sheet_internal_helpers_cover_tone_glyph_metrics_and_regions(void)
{
    egui_view_dialog_sheet_metrics_t metrics;
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);
    egui_color_t mixed = egui_view_dialog_sheet_mix_disabled(sample);
    egui_region_t region;

    setup_sheet();
    egui_view_dialog_sheet_set_palette(EGUI_VIEW_OF(&test_sheet), EGUI_COLOR_HEX(0x111111), EGUI_COLOR_HEX(0x222222), EGUI_COLOR_HEX(0x333333),
                                       EGUI_COLOR_HEX(0x444444), EGUI_COLOR_HEX(0x555555), EGUI_COLOR_HEX(0x666666), EGUI_COLOR_HEX(0x777777),
                                       EGUI_COLOR_HEX(0x888888), EGUI_COLOR_HEX(0x999999), EGUI_COLOR_HEX(0xAAAAAA));

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_MAX_SNAPSHOTS, egui_view_dialog_sheet_clamp_snapshot_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_dialog_sheet_text_len(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_dialog_sheet_text_len("Retry"));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x666666).full, egui_view_dialog_sheet_tone_color(&test_sheet, EGUI_VIEW_DIALOG_SHEET_TONE_ACCENT).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x777777).full, egui_view_dialog_sheet_tone_color(&test_sheet, EGUI_VIEW_DIALOG_SHEET_TONE_SUCCESS).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x888888).full, egui_view_dialog_sheet_tone_color(&test_sheet, EGUI_VIEW_DIALOG_SHEET_TONE_WARNING).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x999999).full, egui_view_dialog_sheet_tone_color(&test_sheet, EGUI_VIEW_DIALOG_SHEET_TONE_ERROR).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0xAAAAAA).full, egui_view_dialog_sheet_tone_color(&test_sheet, EGUI_VIEW_DIALOG_SHEET_TONE_NEUTRAL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x666666).full, egui_view_dialog_sheet_tone_color(&test_sheet, 99).full);
    EGUI_TEST_ASSERT_TRUE(strcmp("i", egui_view_dialog_sheet_tone_glyph(EGUI_VIEW_DIALOG_SHEET_TONE_ACCENT)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("+", egui_view_dialog_sheet_tone_glyph(EGUI_VIEW_DIALOG_SHEET_TONE_SUCCESS)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("!", egui_view_dialog_sheet_tone_glyph(EGUI_VIEW_DIALOG_SHEET_TONE_WARNING)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("x", egui_view_dialog_sheet_tone_glyph(EGUI_VIEW_DIALOG_SHEET_TONE_ERROR)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("o", egui_view_dialog_sheet_tone_glyph(EGUI_VIEW_DIALOG_SHEET_TONE_NEUTRAL)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("i", egui_view_dialog_sheet_tone_glyph(99)) == 0);
    EGUI_TEST_ASSERT_TRUE(egui_view_dialog_sheet_get_snapshot(&test_sheet) == &g_snapshots[0]);
    test_sheet.current_snapshot = 9;
    EGUI_TEST_ASSERT_TRUE(egui_view_dialog_sheet_get_snapshot(&test_sheet) == NULL);
    test_sheet.current_snapshot = 0;
    EGUI_TEST_ASSERT_TRUE(egui_view_dialog_sheet_has_primary(&g_snapshots[0]));
    EGUI_TEST_ASSERT_TRUE(egui_view_dialog_sheet_has_secondary(&g_snapshots[0]));
    EGUI_TEST_ASSERT_FALSE(egui_view_dialog_sheet_has_secondary(&g_invalid_secondary_snapshot));
    EGUI_TEST_ASSERT_FALSE(egui_view_dialog_sheet_has_primary(&g_no_action_snapshot));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY,
                               egui_view_dialog_sheet_normalize_action(&g_snapshots[1], EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY,
                               egui_view_dialog_sheet_normalize_action(&g_invalid_secondary_snapshot, EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY,
                               egui_view_dialog_sheet_normalize_action(&g_secondary_only_snapshot, EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DIALOG_SHEET_ACTION_NONE,
                               egui_view_dialog_sheet_normalize_action(&g_no_action_snapshot, EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_dialog_sheet_pill_width(NULL, 0, 40));
    EGUI_TEST_ASSERT_EQUAL_INT(28, egui_view_dialog_sheet_pill_width("Go", 0, 40));
    EGUI_TEST_ASSERT_EQUAL_INT(24, egui_view_dialog_sheet_pill_width("Long", 1, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_dialog_sheet_button_width(NULL, 0, 40));
    EGUI_TEST_ASSERT_EQUAL_INT(34, egui_view_dialog_sheet_button_width("Go", 0, 40));
    EGUI_TEST_ASSERT_EQUAL_INT(24, egui_view_dialog_sheet_button_width("Long", 1, 24));
    egui_view_dialog_sheet_zero_region(&region);
    EGUI_TEST_ASSERT_EQUAL_INT(0, region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(0, region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(0, region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(0, region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, 68).full, mixed.full);

    layout_sheet();
    get_metrics(&metrics);
    EGUI_TEST_ASSERT_TRUE(metrics.sheet_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.sheet_region.size.height > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.close_region.size.width > 0);

    egui_view_dialog_sheet_set_current_snapshot(EGUI_VIEW_OF(&test_sheet), 2);
    get_metrics(&metrics);
    EGUI_TEST_ASSERT_TRUE(metrics.footer_text_region.size.width > 0);

    egui_view_dialog_sheet_set_compact_mode(EGUI_VIEW_OF(&test_sheet), 1);
    get_metrics(&metrics);
    EGUI_TEST_ASSERT_EQUAL_INT(0, metrics.close_region.size.width);
}

void test_dialog_sheet_run(void)
{
    EGUI_TEST_SUITE_BEGIN(dialog_sheet);
    EGUI_TEST_RUN(test_dialog_sheet_set_snapshots_clamp_and_reset_state);
    EGUI_TEST_RUN(test_dialog_sheet_snapshot_and_action_guards_notify);
    EGUI_TEST_RUN(test_dialog_sheet_font_modes_listener_and_palette);
    EGUI_TEST_RUN(test_dialog_sheet_touch_updates_action_and_hit_testing);
    EGUI_TEST_RUN(test_dialog_sheet_touch_cancel_and_locked_or_disabled_ignore_input);
    EGUI_TEST_RUN(test_dialog_sheet_keyboard_navigation_and_guards);
    EGUI_TEST_RUN(test_dialog_sheet_internal_helpers_cover_tone_glyph_metrics_and_regions);
    EGUI_TEST_SUITE_END();
}
