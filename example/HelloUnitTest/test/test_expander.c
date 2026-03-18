#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_expander.h"

#include "../../HelloCustomWidgets/layout/expander/egui_view_expander.h"
#include "../../HelloCustomWidgets/layout/expander/egui_view_expander.c"

static egui_view_expander_t test_expander;
static uint8_t g_selection_count;
static uint8_t g_last_selection_index;
static uint8_t g_expanded_count;
static uint8_t g_last_expanded_index;
static uint8_t g_last_expanded_value;

static const egui_view_expander_item_t g_items[] = {
        {"WORK", "Workspace policy", "Ready", "Pinned groups stay open", "Draft rules stay visible", "Always on", EGUI_VIEW_EXPANDER_TONE_ACCENT, 1},
        {"SYNC", "Sync rules", "3 rules", "Metered uploads wait for Wi-Fi", "Night copy stays local", "Queue review", EGUI_VIEW_EXPANDER_TONE_SUCCESS, 0},
        {"RELEASE", "Release notes", "Hold", "Pilot warnings stay staged", "Manual signoff closes it", "Manual hold", EGUI_VIEW_EXPANDER_TONE_WARNING, 1},
};

static const egui_view_expander_item_t g_overflow_items[] = {
        {"A", "Alpha", "1", "A", "A", "Open", EGUI_VIEW_EXPANDER_TONE_ACCENT, 0},  {"B", "Beta", "2", "B", "B", "Open", EGUI_VIEW_EXPANDER_TONE_SUCCESS, 0},
        {"C", "Gamma", "3", "C", "C", "Open", EGUI_VIEW_EXPANDER_TONE_WARNING, 0}, {"D", "Delta", "4", "D", "D", "Open", EGUI_VIEW_EXPANDER_TONE_NEUTRAL, 0},
        {"E", "Echo", "5", "E", "E", "Open", EGUI_VIEW_EXPANDER_TONE_ACCENT, 0},
};

static void on_selection_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    g_selection_count++;
    g_last_selection_index = index;
}

static void on_expanded_changed(egui_view_t *self, uint8_t index, uint8_t expanded)
{
    EGUI_UNUSED(self);
    g_expanded_count++;
    g_last_expanded_index = index;
    g_last_expanded_value = expanded;
}

static void reset_listener_state(void)
{
    g_selection_count = 0;
    g_last_selection_index = EGUI_VIEW_EXPANDER_INDEX_NONE;
    g_expanded_count = 0;
    g_last_expanded_index = EGUI_VIEW_EXPANDER_INDEX_NONE;
    g_last_expanded_value = 0xFF;
}

static void setup_expander(void)
{
    egui_view_expander_init(EGUI_VIEW_OF(&test_expander));
    egui_view_set_size(EGUI_VIEW_OF(&test_expander), 194, 110);
    egui_view_expander_set_items(EGUI_VIEW_OF(&test_expander), g_items, 3);
    egui_view_expander_set_on_selection_changed_listener(EGUI_VIEW_OF(&test_expander), on_selection_changed);
    egui_view_expander_set_on_expanded_changed_listener(EGUI_VIEW_OF(&test_expander), on_expanded_changed);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&test_expander), 1);
#endif
    reset_listener_state();
}

static void layout_expander(egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(EGUI_VIEW_OF(&test_expander), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_expander)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_expander)->api->on_touch_event(EGUI_VIEW_OF(&test_expander), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_expander)->api->on_key_event(EGUI_VIEW_OF(&test_expander), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_expander)->api->on_key_event(EGUI_VIEW_OF(&test_expander), &event);
    return handled;
}

static void get_metrics(egui_view_expander_metrics_t *metrics)
{
    expander_get_metrics(&test_expander, EGUI_VIEW_OF(&test_expander), metrics);
}

static void get_header_center(uint8_t index, egui_dim_t *x, egui_dim_t *y)
{
    egui_view_expander_metrics_t metrics;

    get_metrics(&metrics);
    *x = metrics.header_regions[index].location.x + metrics.header_regions[index].size.width / 2;
    *y = metrics.header_regions[index].location.y + metrics.header_regions[index].size.height / 2;
}

static void test_expander_set_items_clamp_and_listener_guards(void)
{
    setup_expander();

    test_expander.current_index = 7;
    test_expander.expanded_index = 8;
    test_expander.pressed_index = 2;
    egui_view_expander_set_items(EGUI_VIEW_OF(&test_expander), g_overflow_items, 5);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_EXPANDER_MAX_ITEMS, egui_view_expander_get_item_count(EGUI_VIEW_OF(&test_expander)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_expander_get_current_index(EGUI_VIEW_OF(&test_expander)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_expander_get_expanded_index(EGUI_VIEW_OF(&test_expander)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_EXPANDER_INDEX_NONE, test_expander.pressed_index);
    EGUI_TEST_ASSERT_TRUE(expander_get_current_item(&test_expander) == &g_overflow_items[0]);

    egui_view_expander_set_current_index(EGUI_VIEW_OF(&test_expander), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_expander_get_current_index(EGUI_VIEW_OF(&test_expander)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selection_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_last_selection_index);

    egui_view_expander_set_current_index(EGUI_VIEW_OF(&test_expander), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selection_count);

    egui_view_expander_set_current_index(EGUI_VIEW_OF(&test_expander), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_expander_get_current_index(EGUI_VIEW_OF(&test_expander)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_selection_count);
    EGUI_TEST_ASSERT_EQUAL_INT(3, g_last_selection_index);

    egui_view_expander_set_expanded_index(EGUI_VIEW_OF(&test_expander), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_expander_get_expanded_index(EGUI_VIEW_OF(&test_expander)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_expanded_count);
    EGUI_TEST_ASSERT_EQUAL_INT(3, g_last_expanded_index);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_expanded_value);

    egui_view_expander_set_expanded_index(EGUI_VIEW_OF(&test_expander), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_expanded_count);

    egui_view_expander_set_items(EGUI_VIEW_OF(&test_expander), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_expander_get_item_count(EGUI_VIEW_OF(&test_expander)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_expander_get_current_index(EGUI_VIEW_OF(&test_expander)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_EXPANDER_INDEX_NONE, egui_view_expander_get_expanded_index(EGUI_VIEW_OF(&test_expander)));
    EGUI_TEST_ASSERT_TRUE(expander_get_current_item(&test_expander) == NULL);
}

static void test_expander_font_palette_and_helpers(void)
{
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);

    setup_expander();

    egui_view_expander_set_font(EGUI_VIEW_OF(&test_expander), NULL);
    egui_view_expander_set_meta_font(EGUI_VIEW_OF(&test_expander), NULL);
    EGUI_TEST_ASSERT_TRUE(test_expander.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_expander.meta_font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    test_expander.pressed_index = 1;
    egui_view_expander_set_compact_mode(EGUI_VIEW_OF(&test_expander), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_expander.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_expander.pressed_index);
    egui_view_expander_set_compact_mode(EGUI_VIEW_OF(&test_expander), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_expander.compact_mode);

    test_expander.pressed_index = 2;
    egui_view_expander_set_read_only_mode(EGUI_VIEW_OF(&test_expander), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_expander.read_only_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_EXPANDER_INDEX_NONE, test_expander.pressed_index);
    egui_view_expander_set_read_only_mode(EGUI_VIEW_OF(&test_expander), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_expander.read_only_mode);

    egui_view_expander_set_palette(EGUI_VIEW_OF(&test_expander), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                   EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152), EGUI_COLOR_HEX(0x606162), EGUI_COLOR_HEX(0x707172),
                                   EGUI_COLOR_HEX(0x808182), EGUI_COLOR_HEX(0x909192));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_expander.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_expander.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_expander.section_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_expander.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_expander.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, test_expander.accent_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x707172).full, test_expander.success_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full, test_expander.warning_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x909192).full, test_expander.neutral_color.full);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_EXPANDER_MAX_ITEMS, expander_clamp_item_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(0, expander_text_len(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(6, expander_text_len("Review"));
    EGUI_TEST_ASSERT_EQUAL_INT(47, expander_meta_width("Ready", 0, 80));
    EGUI_TEST_ASSERT_EQUAL_INT(20, expander_meta_width("A", 1, 80));
    EGUI_TEST_ASSERT_EQUAL_INT(20, expander_pill_width(NULL, 0, 20, 80));
    EGUI_TEST_ASSERT_EQUAL_INT(28, expander_pill_width("Open", 1, 12, 28));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, expander_tone_color(&test_expander, EGUI_VIEW_EXPANDER_TONE_ACCENT).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x707172).full, expander_tone_color(&test_expander, EGUI_VIEW_EXPANDER_TONE_SUCCESS).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full, expander_tone_color(&test_expander, EGUI_VIEW_EXPANDER_TONE_WARNING).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x909192).full, expander_tone_color(&test_expander, EGUI_VIEW_EXPANDER_TONE_NEUTRAL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, expander_tone_color(&test_expander, 99).full);
    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, 68).full, expander_mix_disabled(sample).full);
}

static void test_expander_metrics_and_hit_testing(void)
{
    egui_view_expander_metrics_t metrics;
    egui_dim_t x0;
    egui_dim_t y0;
    egui_dim_t x2;
    egui_dim_t y2;

    setup_expander();
    layout_expander(194, 110);
    get_metrics(&metrics);
    EGUI_TEST_ASSERT_TRUE(metrics.content_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.content_region.size.height > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.header_regions[1].location.y > metrics.header_regions[0].location.y);
    EGUI_TEST_ASSERT_TRUE(metrics.body_regions[0].size.height > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(metrics.body_regions[0].size.height, metrics.body_height);
    EGUI_TEST_ASSERT_EQUAL_INT(0, metrics.body_regions[1].size.height);

    get_header_center(0, &x0, &y0);
    get_header_center(2, &x2, &y2);
    EGUI_TEST_ASSERT_EQUAL_INT(0, expander_hit_index(&test_expander, EGUI_VIEW_OF(&test_expander), x0, y0));
    EGUI_TEST_ASSERT_EQUAL_INT(2, expander_hit_index(&test_expander, EGUI_VIEW_OF(&test_expander), x2, y2));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_EXPANDER_INDEX_NONE,
                               expander_hit_index(&test_expander, EGUI_VIEW_OF(&test_expander), metrics.body_regions[0].location.x + 2,
                                                  metrics.body_regions[0].location.y + metrics.body_regions[0].size.height / 2));

    EGUI_TEST_ASSERT_TRUE(expander_get_current_item(&test_expander) == &g_items[0]);
    test_expander.current_index = 8;
    EGUI_TEST_ASSERT_TRUE(expander_get_current_item(&test_expander) == NULL);
    test_expander.current_index = 0;

    egui_view_expander_set_compact_mode(EGUI_VIEW_OF(&test_expander), 1);
    egui_view_set_size(EGUI_VIEW_OF(&test_expander), 108, 76);
    layout_expander(108, 76);
    get_metrics(&metrics);
    EGUI_TEST_ASSERT_TRUE(metrics.header_regions[0].size.height <= EGUI_VIEW_EXPANDER_COMPACT_HEADER_HEIGHT);
    EGUI_TEST_ASSERT_TRUE(metrics.body_regions[0].size.height <= EGUI_VIEW_EXPANDER_COMPACT_BODY_HEIGHT);
}

static void test_expander_touch_toggle_and_cancel(void)
{
    egui_dim_t x0;
    egui_dim_t y0;
    egui_dim_t x1;
    egui_dim_t y1;

    setup_expander();
    layout_expander(194, 110);
    get_header_center(0, &x0, &y0);
    get_header_center(1, &x1, &y1);

    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 0, 0));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x1, y1));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_expander.pressed_index);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_expander)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x1, y1));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_expander_get_current_index(EGUI_VIEW_OF(&test_expander)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_expander_get_expanded_index(EGUI_VIEW_OF(&test_expander)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selection_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_selection_index);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_expanded_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_expanded_index);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_expanded_value);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_EXPANDER_INDEX_NONE, test_expander.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_expander)->is_pressed);

    get_header_center(1, &x1, &y1);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x1, y1));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x1, y1));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_EXPANDER_INDEX_NONE, egui_view_expander_get_expanded_index(EGUI_VIEW_OF(&test_expander)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selection_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_expanded_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_expanded_index);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_last_expanded_value);

    get_header_center(0, &x0, &y0);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x0, y0));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_expander.pressed_index);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_CANCEL, x0, y0));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_EXPANDER_INDEX_NONE, test_expander.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_expander)->is_pressed);
}

static void test_expander_keyboard_navigation_and_guards(void)
{
    setup_expander();

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_expander_get_current_index(EGUI_VIEW_OF(&test_expander)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selection_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_selection_index);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_expander_get_current_index(EGUI_VIEW_OF(&test_expander)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_selection_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_expander_get_current_index(EGUI_VIEW_OF(&test_expander)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, g_selection_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_UP));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_expander_get_current_index(EGUI_VIEW_OF(&test_expander)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, g_selection_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_EXPANDER_INDEX_NONE, egui_view_expander_get_expanded_index(EGUI_VIEW_OF(&test_expander)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_expanded_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_last_expanded_index);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_last_expanded_value);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_SPACE));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_expander_get_expanded_index(EGUI_VIEW_OF(&test_expander)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_expanded_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_last_expanded_index);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_expanded_value);

    egui_view_expander_set_read_only_mode(EGUI_VIEW_OF(&test_expander), 1);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_expander_get_current_index(EGUI_VIEW_OF(&test_expander)));

    egui_view_expander_set_read_only_mode(EGUI_VIEW_OF(&test_expander), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_expander), 0);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
}

static void test_expander_read_only_disabled_and_empty_guards(void)
{
    egui_dim_t x0;
    egui_dim_t y0;

    setup_expander();
    layout_expander(194, 110);
    get_header_center(0, &x0, &y0);

    egui_view_expander_set_read_only_mode(EGUI_VIEW_OF(&test_expander), 1);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x0, y0));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x0, y0));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_expander_get_current_index(EGUI_VIEW_OF(&test_expander)));

    egui_view_expander_set_read_only_mode(EGUI_VIEW_OF(&test_expander), 0);
    egui_view_expander_set_items(EGUI_VIEW_OF(&test_expander), NULL, 0);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x0, y0));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_DOWN));

    egui_view_expander_set_items(EGUI_VIEW_OF(&test_expander), g_items, 3);
    egui_view_set_enable(EGUI_VIEW_OF(&test_expander), 0);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x0, y0));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x0, y0));
}

void test_expander_run(void)
{
    EGUI_TEST_SUITE_BEGIN(expander);
    EGUI_TEST_RUN(test_expander_set_items_clamp_and_listener_guards);
    EGUI_TEST_RUN(test_expander_font_palette_and_helpers);
    EGUI_TEST_RUN(test_expander_metrics_and_hit_testing);
    EGUI_TEST_RUN(test_expander_touch_toggle_and_cancel);
    EGUI_TEST_RUN(test_expander_keyboard_navigation_and_guards);
    EGUI_TEST_RUN(test_expander_read_only_disabled_and_empty_guards);
    EGUI_TEST_SUITE_END();
}
