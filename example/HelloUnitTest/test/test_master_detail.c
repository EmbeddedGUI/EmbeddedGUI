#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_master_detail.h"

#include "../../HelloCustomWidgets/layout/master_detail/egui_view_master_detail.h"
#include "../../HelloCustomWidgets/layout/master_detail/egui_view_master_detail.c"

static egui_view_master_detail_t test_master_detail;
static uint8_t changed_count;
static uint8_t last_index;

static const egui_view_master_detail_item_t g_items[] = {
        {"FI", "Files", "12 docs", "Workspace", "Files library", "Updated 3m ago", "Pinned drafts stay ready", "Shared notes stay close", "Open",
         EGUI_VIEW_MASTER_DETAIL_TONE_ACCENT, 1},
        {"RV", "Review", "4 tasks", "Review", "Review queue", "Awaiting handoff", "Approve copy and visuals", "Escalate blockers fast", "Assign",
         EGUI_VIEW_MASTER_DETAIL_TONE_WARNING, 0},
        {"MB", "Members", "7 people", "People", "Member roster", "2 active now", "Leads and notes stay aligned", "Roles remain easy to scan", "Invite",
         EGUI_VIEW_MASTER_DETAIL_TONE_SUCCESS, 0},
        {"AR", "Archive", "1 lock", "Archive", "Archive shelf", "Retention 30 days", "Past work stays searchable", "Restore before final purge", "Archive",
         EGUI_VIEW_MASTER_DETAIL_TONE_NEUTRAL, 0},
};

static const egui_view_master_detail_item_t g_overflow_items[] = {
        {"A", "A", "1", "A", "A", "A", "A", "A", "A", EGUI_VIEW_MASTER_DETAIL_TONE_ACCENT, 0},
        {"B", "B", "2", "B", "B", "B", "B", "B", "B", EGUI_VIEW_MASTER_DETAIL_TONE_SUCCESS, 0},
        {"C", "C", "3", "C", "C", "C", "C", "C", "C", EGUI_VIEW_MASTER_DETAIL_TONE_WARNING, 0},
        {"D", "D", "4", "D", "D", "D", "D", "D", "D", EGUI_VIEW_MASTER_DETAIL_TONE_NEUTRAL, 0},
        {"E", "E", "5", "E", "E", "E", "E", "E", "E", EGUI_VIEW_MASTER_DETAIL_TONE_ACCENT, 0},
        {"F", "F", "6", "F", "F", "F", "F", "F", "F", EGUI_VIEW_MASTER_DETAIL_TONE_SUCCESS, 0},
};

static void on_selection_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    changed_count++;
    last_index = index;
}

static void reset_listener_state(void)
{
    changed_count = 0;
    last_index = EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS;
}

static void setup_master_detail(void)
{
    egui_view_master_detail_init(EGUI_VIEW_OF(&test_master_detail));
    egui_view_set_size(EGUI_VIEW_OF(&test_master_detail), 194, 96);
    egui_view_master_detail_set_items(EGUI_VIEW_OF(&test_master_detail), g_items, 4);
    egui_view_master_detail_set_on_selection_changed_listener(EGUI_VIEW_OF(&test_master_detail), on_selection_changed);
    reset_listener_state();
}

static void layout_master_detail(egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(EGUI_VIEW_OF(&test_master_detail), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_master_detail)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_master_detail)->api->on_touch_event(EGUI_VIEW_OF(&test_master_detail), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_master_detail)->api->on_key_event(EGUI_VIEW_OF(&test_master_detail), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_master_detail)->api->on_key_event(EGUI_VIEW_OF(&test_master_detail), &event);
    return handled;
}

static void get_metrics(egui_view_master_detail_metrics_t *metrics)
{
    egui_view_master_detail_get_metrics(&test_master_detail, EGUI_VIEW_OF(&test_master_detail), metrics);
}

static void get_row_center(uint8_t index, egui_dim_t *x, egui_dim_t *y)
{
    egui_view_master_detail_metrics_t metrics;

    get_metrics(&metrics);
    *x = metrics.row_regions[index].location.x + metrics.row_regions[index].size.width / 2;
    *y = metrics.row_regions[index].location.y + metrics.row_regions[index].size.height / 2;
}

static void test_master_detail_set_items_clamp_and_listener_guards(void)
{
    setup_master_detail();

    test_master_detail.current_index = 9;
    test_master_detail.pressed_index = 2;
    egui_view_master_detail_set_items(EGUI_VIEW_OF(&test_master_detail), g_overflow_items, 6);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS, egui_view_master_detail_get_item_count(EGUI_VIEW_OF(&test_master_detail)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_master_detail_get_current_index(EGUI_VIEW_OF(&test_master_detail)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS, test_master_detail.pressed_index);
    EGUI_TEST_ASSERT_NOT_NULL(egui_view_master_detail_get_current_item(&test_master_detail));

    egui_view_master_detail_set_current_index(EGUI_VIEW_OF(&test_master_detail), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_master_detail_get_current_index(EGUI_VIEW_OF(&test_master_detail)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(3, last_index);

    egui_view_master_detail_set_current_index(EGUI_VIEW_OF(&test_master_detail), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);

    egui_view_master_detail_set_current_index(EGUI_VIEW_OF(&test_master_detail), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_master_detail_get_current_index(EGUI_VIEW_OF(&test_master_detail)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);

    test_master_detail.current_index = 9;
    test_master_detail.pressed_index = 1;
    egui_view_master_detail_set_items(EGUI_VIEW_OF(&test_master_detail), g_items, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_master_detail_get_item_count(EGUI_VIEW_OF(&test_master_detail)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_master_detail_get_current_index(EGUI_VIEW_OF(&test_master_detail)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS, test_master_detail.pressed_index);

    egui_view_master_detail_set_items(EGUI_VIEW_OF(&test_master_detail), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_master_detail_get_item_count(EGUI_VIEW_OF(&test_master_detail)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_master_detail_get_current_index(EGUI_VIEW_OF(&test_master_detail)));
    EGUI_TEST_ASSERT_NULL(egui_view_master_detail_get_current_item(&test_master_detail));
}

static void test_master_detail_font_modes_palette_and_helpers(void)
{
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);

    setup_master_detail();

    egui_view_master_detail_set_font(EGUI_VIEW_OF(&test_master_detail), NULL);
    egui_view_master_detail_set_meta_font(EGUI_VIEW_OF(&test_master_detail), NULL);
    EGUI_TEST_ASSERT_TRUE(test_master_detail.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_master_detail.meta_font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    test_master_detail.pressed_index = 1;
    egui_view_master_detail_set_compact_mode(EGUI_VIEW_OF(&test_master_detail), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_master_detail.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_master_detail.pressed_index);
    egui_view_master_detail_set_compact_mode(EGUI_VIEW_OF(&test_master_detail), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_master_detail.compact_mode);

    egui_view_set_pressed(EGUI_VIEW_OF(&test_master_detail), true);
    test_master_detail.pressed_index = 2;
    egui_view_master_detail_set_read_only_mode(EGUI_VIEW_OF(&test_master_detail), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_master_detail.read_only_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS, test_master_detail.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_master_detail)->is_pressed);
    egui_view_master_detail_set_read_only_mode(EGUI_VIEW_OF(&test_master_detail), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_master_detail.read_only_mode);

    egui_view_master_detail_set_palette(EGUI_VIEW_OF(&test_master_detail), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                        EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152), EGUI_COLOR_HEX(0x606162), EGUI_COLOR_HEX(0x707172),
                                        EGUI_COLOR_HEX(0x808182), EGUI_COLOR_HEX(0x909192));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_master_detail.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_master_detail.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_master_detail.section_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_master_detail.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_master_detail.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, test_master_detail.accent_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x707172).full, test_master_detail.success_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full, test_master_detail.warning_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x909192).full, test_master_detail.neutral_color.full);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS, egui_view_master_detail_clamp_item_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_master_detail_text_len(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(6, egui_view_master_detail_text_len("Review"));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full,
                               egui_view_master_detail_tone_color(&test_master_detail, EGUI_VIEW_MASTER_DETAIL_TONE_ACCENT).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x707172).full,
                               egui_view_master_detail_tone_color(&test_master_detail, EGUI_VIEW_MASTER_DETAIL_TONE_SUCCESS).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full,
                               egui_view_master_detail_tone_color(&test_master_detail, EGUI_VIEW_MASTER_DETAIL_TONE_WARNING).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x909192).full,
                               egui_view_master_detail_tone_color(&test_master_detail, EGUI_VIEW_MASTER_DETAIL_TONE_NEUTRAL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, egui_view_master_detail_tone_color(&test_master_detail, 99).full);
    EGUI_TEST_ASSERT_EQUAL_INT(22, egui_view_master_detail_footer_width(NULL, 0, 50));
    EGUI_TEST_ASSERT_EQUAL_INT(42, egui_view_master_detail_footer_width("Open", 0, 60));
    EGUI_TEST_ASSERT_EQUAL_INT(34, egui_view_master_detail_footer_width("Open", 1, 40));
    EGUI_TEST_ASSERT_EQUAL_INT(30, egui_view_master_detail_footer_width("Long", 0, 30));
    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, 68).full, egui_view_master_detail_mix_disabled(sample).full);
}

static void test_master_detail_metrics_and_hit_testing(void)
{
    egui_view_master_detail_metrics_t metrics;
    egui_dim_t x0;
    egui_dim_t y0;
    egui_dim_t x3;
    egui_dim_t y3;

    setup_master_detail();
    layout_master_detail(194, 96);
    get_metrics(&metrics);

    EGUI_TEST_ASSERT_TRUE(metrics.content_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.content_region.size.height > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.master_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.detail_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.row_regions[1].location.y > metrics.row_regions[0].location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(1, metrics.body_line_count);

    get_row_center(0, &x0, &y0);
    get_row_center(3, &x3, &y3);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_master_detail_hit_index(&test_master_detail, EGUI_VIEW_OF(&test_master_detail), x0, y0));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_master_detail_hit_index(&test_master_detail, EGUI_VIEW_OF(&test_master_detail), x3, y3));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS,
                               egui_view_master_detail_hit_index(&test_master_detail, EGUI_VIEW_OF(&test_master_detail), metrics.detail_region.location.x,
                                                                 metrics.detail_region.location.y));

    EGUI_TEST_ASSERT_TRUE(egui_view_master_detail_get_current_item(&test_master_detail) == &g_items[0]);
    test_master_detail.current_index = 9;
    EGUI_TEST_ASSERT_NULL(egui_view_master_detail_get_current_item(&test_master_detail));
    test_master_detail.current_index = 0;

    egui_view_master_detail_set_compact_mode(EGUI_VIEW_OF(&test_master_detail), 1);
    egui_view_set_size(EGUI_VIEW_OF(&test_master_detail), 108, 80);
    layout_master_detail(108, 80);
    get_metrics(&metrics);
    EGUI_TEST_ASSERT_EQUAL_INT(28, metrics.master_region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(14, metrics.row_regions[0].size.height);
    EGUI_TEST_ASSERT_TRUE(metrics.detail_region.location.x > metrics.master_region.location.x);
}

static void test_master_detail_touch_selects_and_guards(void)
{
    egui_dim_t x1;
    egui_dim_t y1;
    egui_dim_t x2;
    egui_dim_t y2;

    setup_master_detail();
    layout_master_detail(194, 96);
    get_row_center(1, &x1, &y1);
    get_row_center(2, &x2, &y2);

    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 0, 0));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x2, y2));
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_master_detail.pressed_index);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_master_detail)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x2, y2));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_master_detail_get_current_index(EGUI_VIEW_OF(&test_master_detail)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, last_index);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS, test_master_detail.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_master_detail)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x1, y1));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_master_detail.pressed_index);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_CANCEL, x1, y1));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_master_detail_get_current_index(EGUI_VIEW_OF(&test_master_detail)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS, test_master_detail.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_master_detail)->is_pressed);

    egui_view_master_detail_set_read_only_mode(EGUI_VIEW_OF(&test_master_detail), 1);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x1, y1));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x1, y1));

    egui_view_master_detail_set_read_only_mode(EGUI_VIEW_OF(&test_master_detail), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_master_detail), 0);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x1, y1));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x1, y1));
}

static void test_master_detail_keyboard_navigation_and_guards(void)
{
    setup_master_detail();

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_master_detail_get_current_index(EGUI_VIEW_OF(&test_master_detail)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_master_detail_get_current_index(EGUI_VIEW_OF(&test_master_detail)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, last_index);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_master_detail_get_current_index(EGUI_VIEW_OF(&test_master_detail)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_master_detail_get_current_index(EGUI_VIEW_OF(&test_master_detail)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_master_detail_get_current_index(EGUI_VIEW_OF(&test_master_detail)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_master_detail_get_current_index(EGUI_VIEW_OF(&test_master_detail)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_UP));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_master_detail_get_current_index(EGUI_VIEW_OF(&test_master_detail)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, changed_count);

    egui_view_master_detail_set_read_only_mode(EGUI_VIEW_OF(&test_master_detail), 1);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_master_detail_get_current_index(EGUI_VIEW_OF(&test_master_detail)));

    egui_view_master_detail_set_read_only_mode(EGUI_VIEW_OF(&test_master_detail), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_master_detail), 0);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
}

void test_master_detail_run(void)
{
    EGUI_TEST_SUITE_BEGIN(master_detail);
    EGUI_TEST_RUN(test_master_detail_set_items_clamp_and_listener_guards);
    EGUI_TEST_RUN(test_master_detail_font_modes_palette_and_helpers);
    EGUI_TEST_RUN(test_master_detail_metrics_and_hit_testing);
    EGUI_TEST_RUN(test_master_detail_touch_selects_and_guards);
    EGUI_TEST_RUN(test_master_detail_keyboard_navigation_and_guards);
    EGUI_TEST_SUITE_END();
}
