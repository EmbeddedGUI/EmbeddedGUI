#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_data_list_panel.h"

#include "../../HelloCustomWidgets/layout/data_list_panel/egui_view_data_list_panel.h"
#include "../../HelloCustomWidgets/layout/data_list_panel/egui_view_data_list_panel.c"

static egui_view_data_list_panel_t test_panel;
static uint8_t changed_count;
static uint8_t last_snapshot;
static uint8_t last_index;

static const egui_view_data_list_panel_item_t g_items_0[] = {
        {"NS", "Nightly sync", "Ops handoff", "18", "Blocked", EGUI_VIEW_DATA_LIST_PANEL_TONE_ACCENT, 1},
        {"EX", "Exports", "Awaiting review", "12", "Review", EGUI_VIEW_DATA_LIST_PANEL_TONE_WARNING, 0},
        {"AP", "Approvals", "Ready to send", "7", "Ready", EGUI_VIEW_DATA_LIST_PANEL_TONE_SUCCESS, 0},
        {"AR", "Archive", "Cold store", "3", "Muted", EGUI_VIEW_DATA_LIST_PANEL_TONE_NEUTRAL, 0},
};

static const egui_view_data_list_panel_item_t g_items_1[] = {
        {"TH", "Thumb pack", "Fresh uploads", "24", "Fresh", EGUI_VIEW_DATA_LIST_PANEL_TONE_SUCCESS, 1},
        {"MD", "Metadata", "Needs owner", "9", "Review", EGUI_VIEW_DATA_LIST_PANEL_TONE_WARNING, 0},
        {"VC", "Version diff", "Build mismatch", "5", "Watch", EGUI_VIEW_DATA_LIST_PANEL_TONE_ACCENT, 0},
        {"BK", "Backup set", "Cold copy", "2", "Muted", EGUI_VIEW_DATA_LIST_PANEL_TONE_NEUTRAL, 0},
};

static const egui_view_data_list_panel_item_t g_items_2[] = {
        {"RT", "Retention", "31 day hold", "31", "Watch", EGUI_VIEW_DATA_LIST_PANEL_TONE_WARNING, 1},
        {"CL", "Cleanup", "Queued purge", "8", "Ready", EGUI_VIEW_DATA_LIST_PANEL_TONE_SUCCESS, 0},
        {"RS", "Restore", "Pending owner", "4", "Review", EGUI_VIEW_DATA_LIST_PANEL_TONE_ACCENT, 0},
};

static const egui_view_data_list_panel_snapshot_t g_snapshots[] = {
        {"Sync queue", "Nightly review", "Ops rows stay aligned.", g_items_0, 4, 0},
        {"Recent", "Upload audit", "Fresh uploads stay visible.", g_items_1, 4, 1},
        {"Locked", "Retention review", "Read only rows stay calm.", g_items_2, 3, 2},
};

static const egui_view_data_list_panel_snapshot_t g_overflow_snapshots[] = {
        {"A", "A", "A", g_items_0, 4, 0}, {"B", "B", "B", g_items_1, 4, 1}, {"C", "C", "C", g_items_2, 3, 2},
        {"D", "D", "D", g_items_0, 4, 3}, {"E", "E", "E", g_items_1, 4, 0}, {"F", "F", "F", g_items_2, 3, 1},
};

static const egui_view_data_list_panel_snapshot_t g_invalid_focus_snapshot = {
        "Warn", "Invalid", "Fallback", g_items_2, 3, 9,
};

static void on_selection_changed(egui_view_t *self, uint8_t snapshot_index, uint8_t item_index)
{
    EGUI_UNUSED(self);
    changed_count++;
    last_snapshot = snapshot_index;
    last_index = item_index;
}

static void reset_listener_state(void)
{
    changed_count = 0;
    last_snapshot = EGUI_VIEW_DATA_LIST_PANEL_MAX_SNAPSHOTS;
    last_index = EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS;
}

static void setup_panel(void)
{
    egui_view_data_list_panel_init(EGUI_VIEW_OF(&test_panel));
    egui_view_set_size(EGUI_VIEW_OF(&test_panel), 196, 132);
    egui_view_data_list_panel_set_snapshots(EGUI_VIEW_OF(&test_panel), g_snapshots, 3);
    egui_view_data_list_panel_set_on_selection_changed_listener(EGUI_VIEW_OF(&test_panel), on_selection_changed);
    reset_listener_state();
}

static void layout_panel(egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(EGUI_VIEW_OF(&test_panel), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_panel)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_panel)->api->on_touch_event(EGUI_VIEW_OF(&test_panel), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_panel)->api->on_key_event(EGUI_VIEW_OF(&test_panel), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_panel)->api->on_key_event(EGUI_VIEW_OF(&test_panel), &event);
    return handled;
}

static void get_metrics(egui_view_data_list_panel_metrics_t *metrics)
{
    egui_view_data_list_panel_get_metrics(&test_panel, EGUI_VIEW_OF(&test_panel), metrics);
}

static void get_row_center(uint8_t index, egui_dim_t *x, egui_dim_t *y)
{
    egui_view_data_list_panel_metrics_t metrics;

    get_metrics(&metrics);
    *x = metrics.row_regions[index].location.x + metrics.row_regions[index].size.width / 2;
    *y = metrics.row_regions[index].location.y + metrics.row_regions[index].size.height / 2;
}

static void test_data_list_panel_set_snapshots_clamp_and_reset_focus(void)
{
    setup_panel();

    egui_view_data_list_panel_set_snapshots(EGUI_VIEW_OF(&test_panel), g_overflow_snapshots, 6);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DATA_LIST_PANEL_MAX_SNAPSHOTS, test_panel.snapshot_count);

    test_panel.current_snapshot = 2;
    test_panel.current_index = 2;
    test_panel.pressed_index = 1;
    egui_view_data_list_panel_set_snapshots(EGUI_VIEW_OF(&test_panel), &g_invalid_focus_snapshot, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_panel.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_data_list_panel_get_current_snapshot(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_data_list_panel_get_current_index(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS, test_panel.pressed_index);
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);
    EGUI_TEST_ASSERT_TRUE(egui_view_data_list_panel_get_snapshot(&test_panel) == &g_invalid_focus_snapshot);
    EGUI_TEST_ASSERT_TRUE(egui_view_data_list_panel_get_item(&test_panel) == &g_items_2[0]);

    egui_view_data_list_panel_set_snapshots(EGUI_VIEW_OF(&test_panel), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_panel.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_data_list_panel_get_current_snapshot(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_data_list_panel_get_current_index(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_NULL(egui_view_data_list_panel_get_snapshot(&test_panel));
    EGUI_TEST_ASSERT_NULL(egui_view_data_list_panel_get_item(&test_panel));
}

static void test_data_list_panel_snapshot_and_index_guards_notify(void)
{
    setup_panel();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_data_list_panel_get_current_snapshot(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_data_list_panel_get_current_index(EGUI_VIEW_OF(&test_panel)));

    egui_view_data_list_panel_set_current_index(EGUI_VIEW_OF(&test_panel), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_data_list_panel_get_current_index(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, last_snapshot);
    EGUI_TEST_ASSERT_EQUAL_INT(2, last_index);

    egui_view_data_list_panel_set_current_index(EGUI_VIEW_OF(&test_panel), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);

    egui_view_data_list_panel_set_current_index(EGUI_VIEW_OF(&test_panel), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_data_list_panel_get_current_index(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);

    test_panel.pressed_index = 1;
    egui_view_data_list_panel_set_current_snapshot(EGUI_VIEW_OF(&test_panel), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_data_list_panel_get_current_snapshot(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_data_list_panel_get_current_index(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS, test_panel.pressed_index);
    EGUI_TEST_ASSERT_EQUAL_INT(2, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, last_snapshot);
    EGUI_TEST_ASSERT_EQUAL_INT(1, last_index);

    egui_view_data_list_panel_set_current_snapshot(EGUI_VIEW_OF(&test_panel), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_data_list_panel_get_current_snapshot(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, changed_count);

    egui_view_data_list_panel_set_current_snapshot(EGUI_VIEW_OF(&test_panel), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_data_list_panel_get_current_snapshot(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_data_list_panel_get_current_index(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, last_snapshot);
    EGUI_TEST_ASSERT_EQUAL_INT(2, last_index);
}

static void test_data_list_panel_font_modes_palette_and_helpers(void)
{
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);

    setup_panel();

    egui_view_data_list_panel_set_font(EGUI_VIEW_OF(&test_panel), NULL);
    egui_view_data_list_panel_set_meta_font(EGUI_VIEW_OF(&test_panel), NULL);
    EGUI_TEST_ASSERT_TRUE(test_panel.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_panel.meta_font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    test_panel.pressed_index = 2;
    egui_view_data_list_panel_set_compact_mode(EGUI_VIEW_OF(&test_panel), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_panel.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_panel.pressed_index);
    egui_view_data_list_panel_set_compact_mode(EGUI_VIEW_OF(&test_panel), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_panel.compact_mode);

    egui_view_set_pressed(EGUI_VIEW_OF(&test_panel), true);
    test_panel.pressed_index = 1;
    egui_view_data_list_panel_set_read_only_mode(EGUI_VIEW_OF(&test_panel), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_panel.read_only_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS, test_panel.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_panel)->is_pressed);
    egui_view_data_list_panel_set_read_only_mode(EGUI_VIEW_OF(&test_panel), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_panel.read_only_mode);

    egui_view_data_list_panel_set_palette(EGUI_VIEW_OF(&test_panel), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                          EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152), EGUI_COLOR_HEX(0x606162), EGUI_COLOR_HEX(0x707172),
                                          EGUI_COLOR_HEX(0x808182), EGUI_COLOR_HEX(0x909192));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_panel.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_panel.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_panel.section_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_panel.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_panel.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, test_panel.accent_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x707172).full, test_panel.success_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full, test_panel.warning_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x909192).full, test_panel.neutral_color.full);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DATA_LIST_PANEL_MAX_SNAPSHOTS, egui_view_data_list_panel_clamp_snapshot_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS, egui_view_data_list_panel_clamp_item_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_data_list_panel_text_len(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(7, egui_view_data_list_panel_text_len("Nightly"));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, egui_view_data_list_panel_tone_color(&test_panel, EGUI_VIEW_DATA_LIST_PANEL_TONE_ACCENT).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x707172).full, egui_view_data_list_panel_tone_color(&test_panel, EGUI_VIEW_DATA_LIST_PANEL_TONE_SUCCESS).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full, egui_view_data_list_panel_tone_color(&test_panel, EGUI_VIEW_DATA_LIST_PANEL_TONE_WARNING).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x909192).full, egui_view_data_list_panel_tone_color(&test_panel, EGUI_VIEW_DATA_LIST_PANEL_TONE_NEUTRAL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, egui_view_data_list_panel_tone_color(&test_panel, 9).full);
    EGUI_TEST_ASSERT_EQUAL_INT(34, egui_view_data_list_panel_pill_width("18", 0, 24, 60));
    EGUI_TEST_ASSERT_EQUAL_INT(24, egui_view_data_list_panel_pill_width("Long", 1, 20, 24));
    EGUI_TEST_ASSERT_EQUAL_INT(57, egui_view_data_list_panel_footer_width("Blocked", 0, 60));
    EGUI_TEST_ASSERT_EQUAL_INT(38, egui_view_data_list_panel_footer_width("Muted", 1, 40));
    EGUI_TEST_ASSERT_EQUAL_INT(30, egui_view_data_list_panel_footer_width("LongFooter", 0, 30));
    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, 68).full, egui_view_data_list_panel_mix_disabled(sample).full);

    EGUI_TEST_ASSERT_TRUE(egui_view_data_list_panel_get_snapshot(&test_panel) == &g_snapshots[0]);
    EGUI_TEST_ASSERT_TRUE(egui_view_data_list_panel_get_item(&test_panel) == &g_items_0[0]);
    test_panel.current_index = 9;
    EGUI_TEST_ASSERT_NULL(egui_view_data_list_panel_get_item(&test_panel));
    test_panel.current_index = 0;
    test_panel.current_snapshot = 9;
    EGUI_TEST_ASSERT_NULL(egui_view_data_list_panel_get_snapshot(&test_panel));
}

static void test_data_list_panel_metrics_and_hit_testing(void)
{
    egui_view_data_list_panel_metrics_t metrics;
    egui_dim_t x0;
    egui_dim_t y0;
    egui_dim_t x3;
    egui_dim_t y3;

    setup_panel();
    layout_panel(196, 132);
    get_metrics(&metrics);

    EGUI_TEST_ASSERT_TRUE(metrics.content_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.row_regions[0].size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.row_regions[1].location.y > metrics.row_regions[0].location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(8, metrics.eyebrow_region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(11, metrics.title_region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(8, metrics.summary_region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(9, metrics.footer_region.size.height);

    get_row_center(0, &x0, &y0);
    get_row_center(3, &x3, &y3);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_data_list_panel_hit_index(&test_panel, EGUI_VIEW_OF(&test_panel), x0, y0));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_data_list_panel_hit_index(&test_panel, EGUI_VIEW_OF(&test_panel), x3, y3));
    EGUI_TEST_ASSERT_EQUAL_INT(
            EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS,
            egui_view_data_list_panel_hit_index(&test_panel, EGUI_VIEW_OF(&test_panel), metrics.content_region.location.x, metrics.content_region.location.y));

    egui_view_data_list_panel_set_compact_mode(EGUI_VIEW_OF(&test_panel), 1);
    egui_view_set_size(EGUI_VIEW_OF(&test_panel), 108, 80);
    layout_panel(108, 80);
    get_metrics(&metrics);
    EGUI_TEST_ASSERT_EQUAL_INT(9, metrics.title_region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(0, metrics.summary_region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(8, metrics.footer_region.size.height);
    EGUI_TEST_ASSERT_TRUE(metrics.row_regions[2].size.height > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.row_regions[2].location.y > metrics.row_regions[1].location.y);
}

static void test_data_list_panel_touch_updates_selection_and_guards(void)
{
    egui_dim_t x1;
    egui_dim_t y1;
    egui_dim_t x2;
    egui_dim_t y2;

    setup_panel();
    layout_panel(196, 132);
    get_row_center(1, &x1, &y1);
    get_row_center(2, &x2, &y2);

    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 0, 0));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x2, y2));
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_panel.pressed_index);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_panel)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x2, y2));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_data_list_panel_get_current_index(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, last_snapshot);
    EGUI_TEST_ASSERT_EQUAL_INT(2, last_index);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS, test_panel.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_panel)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x1, y1));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_panel.pressed_index);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_CANCEL, x1, y1));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_data_list_panel_get_current_index(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS, test_panel.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_panel)->is_pressed);

    egui_view_data_list_panel_set_read_only_mode(EGUI_VIEW_OF(&test_panel), 1);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x1, y1));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x1, y1));

    egui_view_data_list_panel_set_read_only_mode(EGUI_VIEW_OF(&test_panel), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_panel), 0);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x1, y1));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x1, y1));
}

static void test_data_list_panel_keyboard_navigation_and_guards(void)
{
    setup_panel();

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_data_list_panel_get_current_index(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_data_list_panel_get_current_index(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, last_snapshot);
    EGUI_TEST_ASSERT_EQUAL_INT(1, last_index);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_data_list_panel_get_current_index(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_data_list_panel_get_current_index(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_data_list_panel_get_current_index(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_data_list_panel_get_current_index(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_UP));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_data_list_panel_get_current_index(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, changed_count);

    egui_view_data_list_panel_set_current_snapshot(EGUI_VIEW_OF(&test_panel), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_data_list_panel_get_current_snapshot(EGUI_VIEW_OF(&test_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_data_list_panel_get_current_index(EGUI_VIEW_OF(&test_panel)));

    egui_view_data_list_panel_set_read_only_mode(EGUI_VIEW_OF(&test_panel), 1);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_RIGHT));
    egui_view_data_list_panel_set_read_only_mode(EGUI_VIEW_OF(&test_panel), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_panel), 0);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
}

void test_data_list_panel_run(void)
{
    EGUI_TEST_SUITE_BEGIN(data_list_panel);
    EGUI_TEST_RUN(test_data_list_panel_set_snapshots_clamp_and_reset_focus);
    EGUI_TEST_RUN(test_data_list_panel_snapshot_and_index_guards_notify);
    EGUI_TEST_RUN(test_data_list_panel_font_modes_palette_and_helpers);
    EGUI_TEST_RUN(test_data_list_panel_metrics_and_hit_testing);
    EGUI_TEST_RUN(test_data_list_panel_touch_updates_selection_and_guards);
    EGUI_TEST_RUN(test_data_list_panel_keyboard_navigation_and_guards);
    EGUI_TEST_SUITE_END();
}
