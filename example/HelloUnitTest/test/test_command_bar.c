#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_command_bar.h"

#include "../../HelloCustomWidgets/input/command_bar/egui_view_command_bar.h"
#include "../../HelloCustomWidgets/input/command_bar/egui_view_command_bar.c"

static egui_view_command_bar_t test_bar;
static uint8_t changed_count;
static uint8_t last_index;

static const egui_view_command_bar_item_t g_items_0[] = {
        {"SV", "Save", EGUI_VIEW_COMMAND_BAR_TONE_ACCENT, 1, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"LK", "Locked", EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL, 0, 0, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"SH", "Share", EGUI_VIEW_COMMAND_BAR_TONE_SUCCESS, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"...", "Overflow", EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_OVERFLOW},
};

static const egui_view_command_bar_item_t g_items_1[] = {
        {"DS", "Disabled", EGUI_VIEW_COMMAND_BAR_TONE_WARNING, 0, 0, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"RV", "Review", EGUI_VIEW_COMMAND_BAR_TONE_SUCCESS, 1, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"DL", "Delete", EGUI_VIEW_COMMAND_BAR_TONE_DANGER, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"...", "Overflow", EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL, 0, 1, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_OVERFLOW},
};

static const egui_view_command_bar_item_t g_items_2[] = {
        {"A", "Alpha", EGUI_VIEW_COMMAND_BAR_TONE_ACCENT, 0, 0, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"B", "Beta", EGUI_VIEW_COMMAND_BAR_TONE_SUCCESS, 0, 0, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_NORMAL},
        {"...", "Overflow", EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL, 0, 0, EGUI_VIEW_COMMAND_BAR_ITEM_KIND_OVERFLOW},
};

static const egui_view_command_bar_snapshot_t g_snapshots[] = {
        {"Edit", "Page commands", "Canvas", "Save, share, or overflow", g_items_0, 4, 1},
        {"Review", "Review commands", "Build", "Review or delete", g_items_1, 4, 0},
        {"Locked", "Disabled commands", "Guard", "No commands active", g_items_2, 3, 2},
};

static const egui_view_command_bar_snapshot_t g_overflow_snapshots[] = {
        {"A", "A", "A", "A", g_items_0, 4, 0}, {"B", "B", "B", "B", g_items_0, 4, 1}, {"C", "C", "C", "C", g_items_0, 4, 2},
        {"D", "D", "D", "D", g_items_0, 4, 3}, {"E", "E", "E", "E", g_items_0, 4, 0}, {"F", "F", "F", "F", g_items_0, 4, 1},
        {"G", "G", "G", "G", g_items_0, 4, 2},
};

static const egui_view_command_bar_snapshot_t g_invalid_focus_snapshot = {
        "Warn", "Invalid focus", "Build", "Fallback to first enabled", g_items_1, 4, 9,
};

static void reset_changed_state(void)
{
    changed_count = 0;
    last_index = EGUI_VIEW_COMMAND_BAR_INDEX_NONE;
}

static void on_selection_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    changed_count++;
    last_index = index;
}

static void setup_bar(void)
{
    egui_view_command_bar_init(EGUI_VIEW_OF(&test_bar));
    egui_view_set_size(EGUI_VIEW_OF(&test_bar), 196, 88);
    egui_view_command_bar_set_snapshots(EGUI_VIEW_OF(&test_bar), g_snapshots, 3);
    egui_view_command_bar_set_on_selection_changed_listener(EGUI_VIEW_OF(&test_bar), on_selection_changed);
    reset_changed_state();
}

static void layout_bar(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 196;
    region.size.height = 88;
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

static void get_metrics(egui_view_command_bar_metrics_t *metrics)
{
    const egui_view_command_bar_snapshot_t *snapshot = egui_view_command_bar_get_snapshot(&test_bar);
    uint8_t item_count = snapshot == NULL ? 0 : egui_view_command_bar_clamp_item_count(snapshot->item_count);

    egui_view_command_bar_get_metrics(&test_bar, EGUI_VIEW_OF(&test_bar), snapshot, item_count, metrics);
}

static void test_command_bar_set_snapshots_clamp_and_resolve_default_index(void)
{
    setup_bar();

    egui_view_command_bar_set_snapshots(EGUI_VIEW_OF(&test_bar), g_overflow_snapshots, 7);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COMMAND_BAR_MAX_SNAPSHOTS, test_bar.snapshot_count);

    test_bar.current_snapshot = 5;
    test_bar.pressed_index = 2;
    egui_view_command_bar_set_snapshots(EGUI_VIEW_OF(&test_bar), g_snapshots, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_bar.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_command_bar_get_current_snapshot(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COMMAND_BAR_INDEX_NONE, test_bar.pressed_index);
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);

    egui_view_command_bar_set_snapshots(EGUI_VIEW_OF(&test_bar), &g_snapshots[2], 1);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));

    egui_view_command_bar_set_snapshots(EGUI_VIEW_OF(&test_bar), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_bar.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_command_bar_get_current_snapshot(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COMMAND_BAR_INDEX_NONE, test_bar.pressed_index);
}

static void test_command_bar_snapshot_and_index_guards_notify(void)
{
    setup_bar();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));

    egui_view_command_bar_set_current_index(EGUI_VIEW_OF(&test_bar), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, last_index);

    egui_view_command_bar_set_current_index(EGUI_VIEW_OF(&test_bar), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);

    egui_view_command_bar_set_current_index(EGUI_VIEW_OF(&test_bar), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);

    egui_view_command_bar_set_current_index(EGUI_VIEW_OF(&test_bar), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));

    test_bar.pressed_index = 0;
    egui_view_command_bar_set_current_snapshot(EGUI_VIEW_OF(&test_bar), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_command_bar_get_current_snapshot(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COMMAND_BAR_INDEX_NONE, test_bar.pressed_index);
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);

    egui_view_command_bar_set_current_snapshot(EGUI_VIEW_OF(&test_bar), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_command_bar_get_current_snapshot(EGUI_VIEW_OF(&test_bar)));

    egui_view_command_bar_set_current_snapshot(EGUI_VIEW_OF(&test_bar), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_command_bar_get_current_snapshot(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
}

static void test_command_bar_font_modes_listener_and_palette(void)
{
    setup_bar();

    egui_view_command_bar_set_font(EGUI_VIEW_OF(&test_bar), NULL);
    egui_view_command_bar_set_meta_font(EGUI_VIEW_OF(&test_bar), NULL);
    EGUI_TEST_ASSERT_TRUE(test_bar.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_bar.meta_font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    test_bar.pressed_index = 2;
    egui_view_command_bar_set_compact_mode(EGUI_VIEW_OF(&test_bar), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_bar.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COMMAND_BAR_INDEX_NONE, test_bar.pressed_index);

    test_bar.pressed_index = 1;
    egui_view_command_bar_set_disabled_mode(EGUI_VIEW_OF(&test_bar), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_bar.disabled_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COMMAND_BAR_INDEX_NONE, test_bar.pressed_index);

    egui_view_command_bar_set_disabled_mode(EGUI_VIEW_OF(&test_bar), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_bar.disabled_mode);

    egui_view_command_bar_set_palette(EGUI_VIEW_OF(&test_bar), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                      EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152), EGUI_COLOR_HEX(0x606162), EGUI_COLOR_HEX(0x707172),
                                      EGUI_COLOR_HEX(0x808182), EGUI_COLOR_HEX(0x909192), EGUI_COLOR_HEX(0xA0A1A2));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_bar.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_bar.section_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_bar.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_bar.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_bar.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, test_bar.accent_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x707172).full, test_bar.success_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full, test_bar.warning_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x909192).full, test_bar.danger_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0xA0A1A2).full, test_bar.neutral_color.full);
}

static void test_command_bar_touch_updates_selection_and_hit_testing(void)
{
    egui_view_command_bar_metrics_t metrics;
    egui_dim_t x0;
    egui_dim_t y0;
    egui_dim_t x1;
    egui_dim_t y1;
    egui_dim_t x2;
    egui_dim_t y2;
    egui_dim_t x3;
    egui_dim_t y3;

    setup_bar();
    layout_bar();
    get_metrics(&metrics);

    EGUI_TEST_ASSERT_TRUE(metrics.content_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.rail_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.footer_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.scope_region.size.width > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(4, metrics.visible_item_count);
    EGUI_TEST_ASSERT_TRUE(metrics.item_regions[0].size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.item_regions[1].size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.item_regions[2].size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.item_regions[3].size.width > 0);

    x0 = metrics.item_regions[0].location.x + metrics.item_regions[0].size.width / 2;
    y0 = metrics.item_regions[0].location.y + metrics.item_regions[0].size.height / 2;
    x1 = metrics.item_regions[1].location.x + metrics.item_regions[1].size.width / 2;
    y1 = metrics.item_regions[1].location.y + metrics.item_regions[1].size.height / 2;
    x2 = metrics.item_regions[2].location.x + metrics.item_regions[2].size.width / 2;
    y2 = metrics.item_regions[2].location.y + metrics.item_regions[2].size.height / 2;
    x3 = metrics.item_regions[3].location.x + metrics.item_regions[3].size.width / 2;
    y3 = metrics.item_regions[3].location.y + metrics.item_regions[3].size.height / 2;

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_command_bar_hit_item(&test_bar, EGUI_VIEW_OF(&test_bar), x0, y0));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_command_bar_hit_item(&test_bar, EGUI_VIEW_OF(&test_bar), x1, y1));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_command_bar_hit_item(&test_bar, EGUI_VIEW_OF(&test_bar), x2, y2));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_command_bar_hit_item(&test_bar, EGUI_VIEW_OF(&test_bar), x3, y3));
    EGUI_TEST_ASSERT_EQUAL_INT(
            EGUI_VIEW_COMMAND_BAR_INDEX_NONE,
            egui_view_command_bar_hit_item(&test_bar, EGUI_VIEW_OF(&test_bar), metrics.content_region.location.x, metrics.content_region.location.y));

    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, metrics.content_region.location.x, metrics.content_region.location.y));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x1, y1));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x2, y2));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_bar)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_bar.pressed_index);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x2, y2));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_bar)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COMMAND_BAR_INDEX_NONE, test_bar.pressed_index);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, last_index);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x3, y3));
    EGUI_TEST_ASSERT_EQUAL_INT(3, test_bar.pressed_index);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x3, y3));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(3, last_index);
}

static void test_command_bar_touch_cancel_and_disabled_or_view_disabled_ignore_input(void)
{
    egui_view_command_bar_metrics_t metrics;
    egui_dim_t x0;
    egui_dim_t y0;

    setup_bar();
    egui_view_command_bar_set_current_index(EGUI_VIEW_OF(&test_bar), 2);
    reset_changed_state();
    layout_bar();
    get_metrics(&metrics);

    x0 = metrics.item_regions[0].location.x + metrics.item_regions[0].size.width / 2;
    y0 = metrics.item_regions[0].location.y + metrics.item_regions[0].size.height / 2;

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x0, y0));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_bar)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_bar.pressed_index);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_CANCEL, x0, y0));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_bar)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COMMAND_BAR_INDEX_NONE, test_bar.pressed_index);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);

    egui_view_command_bar_set_disabled_mode(EGUI_VIEW_OF(&test_bar), 1);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x0, y0));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x0, y0));

    egui_view_command_bar_set_disabled_mode(EGUI_VIEW_OF(&test_bar), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_bar), 0);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x0, y0));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x0, y0));
}

static void test_command_bar_keyboard_navigation_and_guards(void)
{
    setup_bar();
    reset_changed_state();

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, last_index);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, changed_count);

    egui_view_command_bar_set_current_snapshot(EGUI_VIEW_OF(&test_bar), 2);
    reset_changed_state();
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_command_bar_get_current_index(EGUI_VIEW_OF(&test_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);

    egui_view_command_bar_set_disabled_mode(EGUI_VIEW_OF(&test_bar), 1);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_LEFT));
    egui_view_command_bar_set_disabled_mode(EGUI_VIEW_OF(&test_bar), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_bar), 0);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_LEFT));
}

static void test_command_bar_internal_helpers_cover_metrics_measurements_and_states(void)
{
    egui_view_command_bar_metrics_t metrics;
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);
    egui_color_t mixed = egui_view_command_bar_mix_disabled(sample);

    setup_bar();
    egui_view_command_bar_set_palette(EGUI_VIEW_OF(&test_bar), EGUI_COLOR_HEX(0x111111), EGUI_COLOR_HEX(0x222222), EGUI_COLOR_HEX(0x333333),
                                      EGUI_COLOR_HEX(0x444444), EGUI_COLOR_HEX(0x555555), EGUI_COLOR_HEX(0x666666), EGUI_COLOR_HEX(0x777777),
                                      EGUI_COLOR_HEX(0x888888), EGUI_COLOR_HEX(0x999999), EGUI_COLOR_HEX(0xAAAAAA));

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COMMAND_BAR_MAX_SNAPSHOTS, egui_view_command_bar_clamp_snapshot_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COMMAND_BAR_MAX_ITEMS, egui_view_command_bar_clamp_item_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_command_bar_text_len(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(6, egui_view_command_bar_text_len("Review"));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x666666).full, egui_view_command_bar_tone_color(&test_bar, EGUI_VIEW_COMMAND_BAR_TONE_ACCENT).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x777777).full, egui_view_command_bar_tone_color(&test_bar, EGUI_VIEW_COMMAND_BAR_TONE_SUCCESS).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x888888).full, egui_view_command_bar_tone_color(&test_bar, EGUI_VIEW_COMMAND_BAR_TONE_WARNING).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x999999).full, egui_view_command_bar_tone_color(&test_bar, EGUI_VIEW_COMMAND_BAR_TONE_DANGER).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0xAAAAAA).full, egui_view_command_bar_tone_color(&test_bar, EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x666666).full, egui_view_command_bar_tone_color(&test_bar, 99).full);
    EGUI_TEST_ASSERT_TRUE(egui_view_command_bar_get_snapshot(&test_bar) == &g_snapshots[0]);
    test_bar.current_snapshot = 9;
    EGUI_TEST_ASSERT_TRUE(egui_view_command_bar_get_snapshot(&test_bar) == NULL);
    test_bar.current_snapshot = 0;
    EGUI_TEST_ASSERT_TRUE(egui_view_command_bar_item_is_enabled(&test_bar, EGUI_VIEW_OF(&test_bar), &g_items_0[0]));
    EGUI_TEST_ASSERT_FALSE(egui_view_command_bar_item_is_enabled(&test_bar, EGUI_VIEW_OF(&test_bar), &g_items_0[1]));
    egui_view_command_bar_set_disabled_mode(EGUI_VIEW_OF(&test_bar), 1);
    EGUI_TEST_ASSERT_FALSE(egui_view_command_bar_item_is_enabled(&test_bar, EGUI_VIEW_OF(&test_bar), &g_items_0[0]));
    egui_view_command_bar_set_disabled_mode(EGUI_VIEW_OF(&test_bar), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_bar), 0);
    EGUI_TEST_ASSERT_FALSE(egui_view_command_bar_item_is_enabled(&test_bar, EGUI_VIEW_OF(&test_bar), &g_items_0[0]));
    egui_view_set_enable(EGUI_VIEW_OF(&test_bar), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_command_bar_resolve_default_index(&test_bar, EGUI_VIEW_OF(&test_bar), &g_snapshots[0], 4));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_command_bar_resolve_default_index(&test_bar, EGUI_VIEW_OF(&test_bar), &g_snapshots[1], 4));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_command_bar_resolve_default_index(&test_bar, EGUI_VIEW_OF(&test_bar), &g_snapshots[2], 3));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_command_bar_resolve_default_index(&test_bar, EGUI_VIEW_OF(&test_bar), &g_invalid_focus_snapshot, 4));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_command_bar_measure_scope_width(0, NULL, 30));
    EGUI_TEST_ASSERT_EQUAL_INT(40, egui_view_command_bar_measure_scope_width(0, "Build", 40));
    EGUI_TEST_ASSERT_EQUAL_INT(24, egui_view_command_bar_measure_scope_width(1, "Build", 24));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_command_bar_measure_pill_width(0, NULL, 30));
    EGUI_TEST_ASSERT_EQUAL_INT(32, egui_view_command_bar_measure_pill_width(0, "Edit", 40));
    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_command_bar_measure_pill_width(1, "Long", 20));
    EGUI_TEST_ASSERT_EQUAL_INT(53, egui_view_command_bar_measure_item_width(0, &g_items_0[0]));
    EGUI_TEST_ASSERT_EQUAL_INT(54, egui_view_command_bar_measure_item_width(0, &g_items_0[2]));
    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_command_bar_measure_item_width(1, &g_items_0[0]));
    EGUI_TEST_ASSERT_EQUAL_INT(18, egui_view_command_bar_measure_item_width(1, &g_items_0[2]));
    EGUI_TEST_ASSERT_EQUAL_INT(24, egui_view_command_bar_measure_item_width(0, &g_items_0[3]));
    EGUI_TEST_ASSERT_EQUAL_INT(18, egui_view_command_bar_measure_item_width(1, &g_items_0[3]));
    egui_view_command_bar_reset_metrics(&metrics);
    EGUI_TEST_ASSERT_EQUAL_INT(0, metrics.visible_item_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, metrics.item_regions[0].size.width);
    layout_bar();
    get_metrics(&metrics);
    EGUI_TEST_ASSERT_TRUE(metrics.content_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.header_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.rail_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.footer_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.scope_region.size.width > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(4, metrics.visible_item_count);
    EGUI_TEST_ASSERT_TRUE(metrics.item_regions[3].size.width > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(
            EGUI_VIEW_COMMAND_BAR_INDEX_NONE,
            egui_view_command_bar_hit_item(&test_bar, EGUI_VIEW_OF(&test_bar), metrics.content_region.location.x, metrics.content_region.location.y));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_command_bar_find_home_index(&test_bar, EGUI_VIEW_OF(&test_bar), 0));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_command_bar_find_home_index(&test_bar, EGUI_VIEW_OF(&test_bar), 1));
    egui_view_command_bar_set_compact_mode(EGUI_VIEW_OF(&test_bar), 1);
    get_metrics(&metrics);
    EGUI_TEST_ASSERT_EQUAL_INT(14, metrics.scope_region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(18, metrics.item_regions[0].size.height);
    egui_view_command_bar_set_current_snapshot(EGUI_VIEW_OF(&test_bar), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COMMAND_BAR_INDEX_NONE, egui_view_command_bar_find_home_index(&test_bar, EGUI_VIEW_OF(&test_bar), 0));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_COMMAND_BAR_INDEX_NONE, egui_view_command_bar_find_home_index(&test_bar, EGUI_VIEW_OF(&test_bar), 1));
    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, 68).full, mixed.full);
}

void test_command_bar_run(void)
{
    EGUI_TEST_SUITE_BEGIN(command_bar);
    EGUI_TEST_RUN(test_command_bar_set_snapshots_clamp_and_resolve_default_index);
    EGUI_TEST_RUN(test_command_bar_snapshot_and_index_guards_notify);
    EGUI_TEST_RUN(test_command_bar_font_modes_listener_and_palette);
    EGUI_TEST_RUN(test_command_bar_touch_updates_selection_and_hit_testing);
    EGUI_TEST_RUN(test_command_bar_touch_cancel_and_disabled_or_view_disabled_ignore_input);
    EGUI_TEST_RUN(test_command_bar_keyboard_navigation_and_guards);
    EGUI_TEST_RUN(test_command_bar_internal_helpers_cover_metrics_measurements_and_states);
    EGUI_TEST_SUITE_END();
}
