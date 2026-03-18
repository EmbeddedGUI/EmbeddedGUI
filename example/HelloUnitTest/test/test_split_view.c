#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_split_view.h"

#include "../../HelloCustomWidgets/layout/split_view/egui_view_split_view.h"
#include "../../HelloCustomWidgets/layout/split_view/egui_view_split_view.c"

static egui_view_split_view_t test_split_view;
static uint8_t g_selection_count;
static uint8_t g_last_selection_index;
static uint8_t g_pane_state_count;
static uint8_t g_last_pane_expanded;

static const egui_view_split_view_item_t g_items[] = {
        {"OV", "Overview", "8", "Workspace", "Overview board", "Updated 2m ago", "Pinned modules stay visible", "Status cards stay aligned", "Open",
         EGUI_VIEW_SPLIT_VIEW_TONE_ACCENT, 1},
        {"FI", "Files", "12", "Library", "Files library", "Synced 5m ago", "Shared assets stay within reach", "Recent exports stay sorted", "Browse",
         EGUI_VIEW_SPLIT_VIEW_TONE_ACCENT, 0},
        {"RV", "Review", "4", "Review", "Review queue", "Awaiting signoff", "Approvals stay on one rail", "Escalations remain visible", "Assign",
         EGUI_VIEW_SPLIT_VIEW_TONE_WARNING, 0},
        {"AR", "Archive", "1", "Archive", "Archive shelf", "Retention 30 days", "Older work stays tucked away", "Restore before final purge", "Store",
         EGUI_VIEW_SPLIT_VIEW_TONE_NEUTRAL, 0},
};

static const egui_view_split_view_item_t g_overflow_items[] = {
        {"A", "Alpha", "1", "A", "Alpha", "A", "A", "A", "Open", EGUI_VIEW_SPLIT_VIEW_TONE_ACCENT, 0},
        {"B", "Beta", "2", "B", "Beta", "B", "B", "B", "Open", EGUI_VIEW_SPLIT_VIEW_TONE_SUCCESS, 0},
        {"C", "Gamma", "3", "C", "Gamma", "C", "C", "C", "Open", EGUI_VIEW_SPLIT_VIEW_TONE_WARNING, 0},
        {"D", "Delta", "4", "D", "Delta", "D", "D", "D", "Open", EGUI_VIEW_SPLIT_VIEW_TONE_NEUTRAL, 0},
        {"E", "Echo", "5", "E", "Echo", "E", "E", "E", "Open", EGUI_VIEW_SPLIT_VIEW_TONE_ACCENT, 0},
        {"F", "Foxtrot", "6", "F", "Foxtrot", "F", "F", "F", "Open", EGUI_VIEW_SPLIT_VIEW_TONE_SUCCESS, 0},
};

static void on_selection_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    g_selection_count++;
    g_last_selection_index = index;
}

static void on_pane_state_changed(egui_view_t *self, uint8_t expanded)
{
    EGUI_UNUSED(self);
    g_pane_state_count++;
    g_last_pane_expanded = expanded;
}

static void reset_listener_state(void)
{
    g_selection_count = 0;
    g_last_selection_index = EGUI_VIEW_SPLIT_VIEW_INDEX_NONE;
    g_pane_state_count = 0;
    g_last_pane_expanded = 0xFF;
}

static void setup_split_view(void)
{
    egui_view_split_view_init(EGUI_VIEW_OF(&test_split_view));
    egui_view_set_size(EGUI_VIEW_OF(&test_split_view), 194, 104);
    egui_view_split_view_set_items(EGUI_VIEW_OF(&test_split_view), g_items, 4);
    egui_view_split_view_set_on_selection_changed_listener(EGUI_VIEW_OF(&test_split_view), on_selection_changed);
    egui_view_split_view_set_on_pane_state_changed_listener(EGUI_VIEW_OF(&test_split_view), on_pane_state_changed);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&test_split_view), 1);
#endif
    reset_listener_state();
}

static void layout_split_view(egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(EGUI_VIEW_OF(&test_split_view), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_split_view)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_split_view)->api->on_touch_event(EGUI_VIEW_OF(&test_split_view), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_split_view)->api->on_key_event(EGUI_VIEW_OF(&test_split_view), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_split_view)->api->on_key_event(EGUI_VIEW_OF(&test_split_view), &event);
    return handled;
}

static void get_metrics(egui_view_split_view_metrics_t *metrics)
{
    sv_get_metrics(&test_split_view, EGUI_VIEW_OF(&test_split_view), metrics);
}

static void get_row_center(uint8_t index, egui_dim_t *x, egui_dim_t *y)
{
    egui_view_split_view_metrics_t metrics;

    get_metrics(&metrics);
    *x = metrics.rows[index].location.x + metrics.rows[index].size.width / 2;
    *y = metrics.rows[index].location.y + metrics.rows[index].size.height / 2;
}

static void get_toggle_center(egui_dim_t *x, egui_dim_t *y)
{
    egui_view_split_view_metrics_t metrics;

    get_metrics(&metrics);
    *x = metrics.toggle.location.x + metrics.toggle.size.width / 2;
    *y = metrics.toggle.location.y + metrics.toggle.size.height / 2;
}

static void test_split_view_set_items_clamp_and_listener_guards(void)
{
    setup_split_view();

    test_split_view.current_index = 9;
    test_split_view.pressed_index = 2;
    test_split_view.pressed_toggle = 1;
    egui_view_split_view_set_items(EGUI_VIEW_OF(&test_split_view), g_overflow_items, 6);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_VIEW_MAX_ITEMS, egui_view_split_view_get_item_count(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_split_view_get_current_index(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_VIEW_INDEX_NONE, test_split_view.pressed_index);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_split_view.pressed_toggle);
    EGUI_TEST_ASSERT_TRUE(sv_get_current_item(&test_split_view) == &g_overflow_items[0]);

    egui_view_split_view_set_current_index(EGUI_VIEW_OF(&test_split_view), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_split_view_get_current_index(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selection_count);
    EGUI_TEST_ASSERT_EQUAL_INT(3, g_last_selection_index);

    egui_view_split_view_set_current_index(EGUI_VIEW_OF(&test_split_view), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selection_count);

    egui_view_split_view_set_current_index(EGUI_VIEW_OF(&test_split_view), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_split_view_get_current_index(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selection_count);

    test_split_view.current_index = 8;
    egui_view_split_view_set_items(EGUI_VIEW_OF(&test_split_view), g_items, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_split_view_get_item_count(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_split_view_get_current_index(EGUI_VIEW_OF(&test_split_view)));

    egui_view_split_view_set_items(EGUI_VIEW_OF(&test_split_view), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_split_view_get_item_count(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_split_view_get_current_index(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_TRUE(sv_get_current_item(&test_split_view) == NULL);
}

static void test_split_view_font_palette_helpers_and_pane_listener(void)
{
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);

    setup_split_view();

    egui_view_split_view_set_font(EGUI_VIEW_OF(&test_split_view), NULL);
    egui_view_split_view_set_meta_font(EGUI_VIEW_OF(&test_split_view), NULL);
    EGUI_TEST_ASSERT_TRUE(test_split_view.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_split_view.meta_font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    test_split_view.pressed_index = 1;
    egui_view_split_view_set_compact_mode(EGUI_VIEW_OF(&test_split_view), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_split_view.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_split_view.pressed_index);
    egui_view_split_view_set_compact_mode(EGUI_VIEW_OF(&test_split_view), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_split_view.compact_mode);

    egui_view_set_pressed(EGUI_VIEW_OF(&test_split_view), true);
    test_split_view.pressed_index = 2;
    test_split_view.pressed_toggle = 1;
    egui_view_split_view_set_read_only_mode(EGUI_VIEW_OF(&test_split_view), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_split_view.read_only_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_VIEW_INDEX_NONE, test_split_view.pressed_index);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_split_view.pressed_toggle);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_split_view)->is_pressed);
    egui_view_split_view_set_read_only_mode(EGUI_VIEW_OF(&test_split_view), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_split_view.read_only_mode);

    egui_view_split_view_set_palette(EGUI_VIEW_OF(&test_split_view), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                     EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152), EGUI_COLOR_HEX(0x606162), EGUI_COLOR_HEX(0x707172),
                                     EGUI_COLOR_HEX(0x808182), EGUI_COLOR_HEX(0x909192));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_split_view.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_split_view.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_split_view.section_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_split_view.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_split_view.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, test_split_view.accent_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x707172).full, test_split_view.success_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full, test_split_view.warning_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x909192).full, test_split_view.neutral_color.full);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_VIEW_MAX_ITEMS, sv_clamp_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(0, sv_text_len(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(6, sv_text_len("Review"));
    EGUI_TEST_ASSERT_EQUAL_INT(28, sv_meta_width("LongLabel", 0));
    EGUI_TEST_ASSERT_EQUAL_INT(20, sv_meta_width("LongLabel", 1));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, sv_tone_color(&test_split_view, EGUI_VIEW_SPLIT_VIEW_TONE_ACCENT).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x707172).full, sv_tone_color(&test_split_view, EGUI_VIEW_SPLIT_VIEW_TONE_SUCCESS).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full, sv_tone_color(&test_split_view, EGUI_VIEW_SPLIT_VIEW_TONE_WARNING).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x909192).full, sv_tone_color(&test_split_view, EGUI_VIEW_SPLIT_VIEW_TONE_NEUTRAL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, sv_tone_color(&test_split_view, 99).full);
    EGUI_TEST_ASSERT_EQUAL_INT(22, sv_footer_width(NULL, 0, 50));
    EGUI_TEST_ASSERT_EQUAL_INT(42, sv_footer_width("Open", 0, 60));
    EGUI_TEST_ASSERT_EQUAL_INT(34, sv_footer_width("Open", 1, 40));
    EGUI_TEST_ASSERT_EQUAL_INT(30, sv_footer_width("Long", 0, 30));
    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, 68).full, sv_mix_disabled(sample).full);

    egui_view_split_view_set_pane_expanded(EGUI_VIEW_OF(&test_split_view), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_pane_state_count);
    egui_view_split_view_set_pane_expanded(EGUI_VIEW_OF(&test_split_view), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_split_view_get_pane_expanded(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_pane_state_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_last_pane_expanded);
    egui_view_split_view_toggle_pane(EGUI_VIEW_OF(&test_split_view));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_split_view_get_pane_expanded(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_pane_state_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_pane_expanded);
}

static void test_split_view_metrics_and_hit_testing(void)
{
    egui_view_split_view_metrics_t metrics;
    egui_dim_t row0_x;
    egui_dim_t row0_y;
    egui_dim_t row3_x;
    egui_dim_t row3_y;
    egui_dim_t toggle_x;
    egui_dim_t toggle_y;

    setup_split_view();
    layout_split_view(194, 104);
    get_metrics(&metrics);

    EGUI_TEST_ASSERT_TRUE(metrics.content.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.content.size.height > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(SV_STD_EXPANDED_W, metrics.pane.size.width);
    EGUI_TEST_ASSERT_TRUE(metrics.detail.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.rows[1].location.y > metrics.rows[0].location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(SV_STD_ROW_H, metrics.rows[0].size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(1, metrics.show_secondary_body);
    EGUI_TEST_ASSERT_EQUAL_INT(1, metrics.show_meta);
    EGUI_TEST_ASSERT_EQUAL_INT(1, metrics.draw_detail);
    EGUI_TEST_ASSERT_TRUE(metrics.title.size.width > 0);

    get_row_center(0, &row0_x, &row0_y);
    get_row_center(3, &row3_x, &row3_y);
    get_toggle_center(&toggle_x, &toggle_y);
    EGUI_TEST_ASSERT_EQUAL_INT(0, sv_hit_index(&test_split_view, EGUI_VIEW_OF(&test_split_view), row0_x, row0_y));
    EGUI_TEST_ASSERT_EQUAL_INT(3, sv_hit_index(&test_split_view, EGUI_VIEW_OF(&test_split_view), row3_x, row3_y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_VIEW_INDEX_NONE,
                               sv_hit_index(&test_split_view, EGUI_VIEW_OF(&test_split_view), metrics.detail.location.x + 2, metrics.detail.location.y + 2));
    EGUI_TEST_ASSERT_EQUAL_INT(1, sv_hit_toggle(&test_split_view, EGUI_VIEW_OF(&test_split_view), toggle_x, toggle_y));
    EGUI_TEST_ASSERT_EQUAL_INT(0,
                               sv_hit_toggle(&test_split_view, EGUI_VIEW_OF(&test_split_view), metrics.detail.location.x + 2, metrics.detail.location.y + 2));

    EGUI_TEST_ASSERT_TRUE(sv_get_current_item(&test_split_view) == &g_items[0]);
    test_split_view.current_index = 9;
    EGUI_TEST_ASSERT_TRUE(sv_get_current_item(&test_split_view) == NULL);
    test_split_view.current_index = 0;

    egui_view_split_view_set_compact_mode(EGUI_VIEW_OF(&test_split_view), 1);
    egui_view_split_view_set_pane_expanded(EGUI_VIEW_OF(&test_split_view), 0);
    egui_view_set_size(EGUI_VIEW_OF(&test_split_view), 108, 74);
    layout_split_view(108, 74);
    get_metrics(&metrics);
    EGUI_TEST_ASSERT_EQUAL_INT(SV_COMPACT_COLLAPSED_W, metrics.pane.size.width);
    EGUI_TEST_ASSERT_TRUE(metrics.rows[0].size.height > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.rows[0].size.height <= SV_COMPACT_ROW_H);
    EGUI_TEST_ASSERT_EQUAL_INT(0, metrics.show_secondary_body);
    EGUI_TEST_ASSERT_TRUE(metrics.detail.location.x > metrics.pane.location.x);
}

static void test_split_view_touch_toggle_select_and_cancel(void)
{
    egui_dim_t toggle_x;
    egui_dim_t toggle_y;
    egui_dim_t row1_x;
    egui_dim_t row1_y;
    egui_dim_t row2_x;
    egui_dim_t row2_y;

    setup_split_view();
    layout_split_view(194, 104);
    get_toggle_center(&toggle_x, &toggle_y);
    get_row_center(1, &row1_x, &row1_y);
    get_row_center(2, &row2_x, &row2_y);

    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 0, 0));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, toggle_x, toggle_y));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_split_view.pressed_toggle);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_VIEW_INDEX_NONE, test_split_view.pressed_index);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_split_view)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, toggle_x, toggle_y));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_split_view_get_pane_expanded(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_pane_state_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_last_pane_expanded);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_split_view.pressed_toggle);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_split_view)->is_pressed);

    get_row_center(1, &row1_x, &row1_y);
    get_row_center(2, &row2_x, &row2_y);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, row2_x, row2_y));
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_split_view.pressed_index);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, row2_x, row2_y));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_split_view_get_current_index(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selection_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_last_selection_index);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_VIEW_INDEX_NONE, test_split_view.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_split_view)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, row1_x, row1_y));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_split_view.pressed_index);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_CANCEL, row1_x, row1_y));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_split_view_get_current_index(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selection_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SPLIT_VIEW_INDEX_NONE, test_split_view.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_split_view)->is_pressed);
}

static void test_split_view_keyboard_navigation_and_guards(void)
{
    setup_split_view();

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_split_view_get_pane_expanded(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_pane_state_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_last_pane_expanded);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_split_view_get_pane_expanded(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_pane_state_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_pane_expanded);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_split_view_get_current_index(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selection_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_selection_index);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_split_view_get_current_index(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_selection_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_split_view_get_current_index(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, g_selection_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_split_view_get_current_index(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, g_selection_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_UP));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_split_view_get_current_index(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, g_selection_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_split_view_get_pane_expanded(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, g_pane_state_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_SPACE));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_split_view_get_pane_expanded(EGUI_VIEW_OF(&test_split_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, g_pane_state_count);

    egui_view_split_view_set_read_only_mode(EGUI_VIEW_OF(&test_split_view), 1);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_split_view_get_pane_expanded(EGUI_VIEW_OF(&test_split_view)));

    egui_view_split_view_set_read_only_mode(EGUI_VIEW_OF(&test_split_view), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_split_view), 0);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
}

static void test_split_view_read_only_disabled_and_empty_guards(void)
{
    egui_dim_t toggle_x;
    egui_dim_t toggle_y;
    egui_dim_t row0_x;
    egui_dim_t row0_y;

    setup_split_view();
    layout_split_view(194, 104);
    get_toggle_center(&toggle_x, &toggle_y);
    get_row_center(0, &row0_x, &row0_y);

    egui_view_split_view_set_read_only_mode(EGUI_VIEW_OF(&test_split_view), 1);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, toggle_x, toggle_y));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, toggle_x, toggle_y));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, row0_x, row0_y));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_DOWN));

    egui_view_split_view_set_read_only_mode(EGUI_VIEW_OF(&test_split_view), 0);
    egui_view_split_view_set_items(EGUI_VIEW_OF(&test_split_view), NULL, 0);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, toggle_x, toggle_y));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_RIGHT));

    egui_view_split_view_set_items(EGUI_VIEW_OF(&test_split_view), g_items, 4);
    egui_view_set_enable(EGUI_VIEW_OF(&test_split_view), 0);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, row0_x, row0_y));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, row0_x, row0_y));
}

void test_split_view_run(void)
{
    EGUI_TEST_SUITE_BEGIN(split_view);
    EGUI_TEST_RUN(test_split_view_set_items_clamp_and_listener_guards);
    EGUI_TEST_RUN(test_split_view_font_palette_helpers_and_pane_listener);
    EGUI_TEST_RUN(test_split_view_metrics_and_hit_testing);
    EGUI_TEST_RUN(test_split_view_touch_toggle_select_and_cancel);
    EGUI_TEST_RUN(test_split_view_keyboard_navigation_and_guards);
    EGUI_TEST_RUN(test_split_view_read_only_disabled_and_empty_guards);
    EGUI_TEST_SUITE_END();
}
