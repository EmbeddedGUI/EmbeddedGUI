#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_persona_group.h"

#include "../../HelloCustomWidgets/display/persona_group/egui_view_persona_group.h"
#include "../../HelloCustomWidgets/display/persona_group/egui_view_persona_group.c"

static egui_view_persona_group_t test_group;
static uint8_t changed_count;
static uint8_t last_snapshot;
static uint8_t last_index;

static const egui_view_persona_group_item_t g_items_0[] = {
        {"LM", "Lena", "Design", EGUI_VIEW_PERSONA_GROUP_TONE_ACCENT, EGUI_VIEW_PERSONA_GROUP_PRESENCE_LIVE, 1},
        {"AR", "Arun", "Ops", EGUI_VIEW_PERSONA_GROUP_TONE_SUCCESS, EGUI_VIEW_PERSONA_GROUP_PRESENCE_BUSY, 0},
        {"MY", "Maya", "QA", EGUI_VIEW_PERSONA_GROUP_TONE_WARNING, EGUI_VIEW_PERSONA_GROUP_PRESENCE_AWAY, 0},
        {"JN", "Jin", "Content", EGUI_VIEW_PERSONA_GROUP_TONE_NEUTRAL, EGUI_VIEW_PERSONA_GROUP_PRESENCE_IDLE, 0},
};

static const egui_view_persona_group_item_t g_items_1[] = {
        {"SO", "Sora", "Lead", EGUI_VIEW_PERSONA_GROUP_TONE_SUCCESS, EGUI_VIEW_PERSONA_GROUP_PRESENCE_LIVE, 1},
        {"IV", "Ivy", "PM", EGUI_VIEW_PERSONA_GROUP_TONE_ACCENT, EGUI_VIEW_PERSONA_GROUP_PRESENCE_BUSY, 0},
        {"TE", "Teo", "QA", EGUI_VIEW_PERSONA_GROUP_TONE_WARNING, EGUI_VIEW_PERSONA_GROUP_PRESENCE_AWAY, 0},
        {"AL", "Ali", "Docs", EGUI_VIEW_PERSONA_GROUP_TONE_NEUTRAL, EGUI_VIEW_PERSONA_GROUP_PRESENCE_IDLE, 0},
};

static const egui_view_persona_group_item_t g_items_2[] = {
        {"MB", "Mina", "Archive", EGUI_VIEW_PERSONA_GROUP_TONE_NEUTRAL, EGUI_VIEW_PERSONA_GROUP_PRESENCE_IDLE, 1},
        {"KO", "Kora", "QA", EGUI_VIEW_PERSONA_GROUP_TONE_WARNING, EGUI_VIEW_PERSONA_GROUP_PRESENCE_AWAY, 0},
        {"YU", "Yuri", "Restore", EGUI_VIEW_PERSONA_GROUP_TONE_ACCENT, EGUI_VIEW_PERSONA_GROUP_PRESENCE_LIVE, 0},
};

static const egui_view_persona_group_snapshot_t g_snapshots[] = {
        {"Design squad", "Design review", "Design team", g_items_0, 4, 0, 2},
        {"Ops desk", "Ops handoff", "Ops desk", g_items_1, 4, 1, 1},
        {"Archive", "Archive sweep", "Archive", g_items_2, 3, 2, 0},
};

static const egui_view_persona_group_snapshot_t g_overflow_snapshots[] = {
        {"A", "A", "A", g_items_0, 4, 0, 0}, {"B", "B", "B", g_items_0, 4, 1, 1}, {"C", "C", "C", g_items_1, 4, 2, 2},
        {"D", "D", "D", g_items_1, 4, 3, 3}, {"E", "E", "E", g_items_2, 3, 0, 0}, {"F", "F", "F", g_items_2, 3, 1, 0},
};

static const egui_view_persona_group_snapshot_t g_invalid_focus_snapshot = {
        "Warn", "Invalid", "Fallback", g_items_2, 3, 9, 0,
};

static void on_focus_changed(egui_view_t *self, uint8_t snapshot_index, uint8_t item_index)
{
    EGUI_UNUSED(self);
    changed_count++;
    last_snapshot = snapshot_index;
    last_index = item_index;
}

static void reset_listener_state(void)
{
    changed_count = 0;
    last_snapshot = EGUI_VIEW_PERSONA_GROUP_MAX_SNAPSHOTS;
    last_index = EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS;
}

static void setup_group(void)
{
    egui_view_persona_group_init(EGUI_VIEW_OF(&test_group));
    egui_view_set_size(EGUI_VIEW_OF(&test_group), 194, 114);
    egui_view_persona_group_set_snapshots(EGUI_VIEW_OF(&test_group), g_snapshots, 3);
    egui_view_persona_group_set_on_focus_changed_listener(EGUI_VIEW_OF(&test_group), on_focus_changed);
    reset_listener_state();
}

static void layout_group(egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(EGUI_VIEW_OF(&test_group), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_group)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_group)->api->on_touch_event(EGUI_VIEW_OF(&test_group), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_group)->api->on_key_event(EGUI_VIEW_OF(&test_group), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_group)->api->on_key_event(EGUI_VIEW_OF(&test_group), &event);
    return handled;
}

static void get_metrics(egui_view_persona_group_metrics_t *metrics)
{
    egui_view_persona_group_get_metrics(&test_group, EGUI_VIEW_OF(&test_group), metrics);
}

static void get_avatar_center(uint8_t index, egui_dim_t *x, egui_dim_t *y)
{
    egui_view_persona_group_metrics_t metrics;

    get_metrics(&metrics);
    *x = metrics.avatar_regions[index].location.x + metrics.avatar_regions[index].size.width / 2;
    *y = metrics.avatar_regions[index].location.y + metrics.avatar_regions[index].size.height / 2;
}

static void test_persona_group_set_snapshots_clamp_and_reset_focus(void)
{
    setup_group();

    egui_view_persona_group_set_snapshots(EGUI_VIEW_OF(&test_group), g_overflow_snapshots, 6);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PERSONA_GROUP_MAX_SNAPSHOTS, test_group.snapshot_count);

    test_group.current_snapshot = 2;
    test_group.current_index = 2;
    test_group.pressed_index = 1;
    egui_view_persona_group_set_snapshots(EGUI_VIEW_OF(&test_group), &g_invalid_focus_snapshot, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_group.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_persona_group_get_current_snapshot(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_persona_group_get_current_index(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS, test_group.pressed_index);
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);
    EGUI_TEST_ASSERT_TRUE(egui_view_persona_group_get_snapshot(&test_group) == &g_invalid_focus_snapshot);

    egui_view_persona_group_set_snapshots(EGUI_VIEW_OF(&test_group), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_group.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_persona_group_get_current_snapshot(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_persona_group_get_current_index(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_NULL(egui_view_persona_group_get_snapshot(&test_group));
    EGUI_TEST_ASSERT_NULL(egui_view_persona_group_get_item(&test_group));
}

static void test_persona_group_snapshot_and_index_guards_notify(void)
{
    setup_group();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_persona_group_get_current_snapshot(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_persona_group_get_current_index(EGUI_VIEW_OF(&test_group)));

    egui_view_persona_group_set_current_index(EGUI_VIEW_OF(&test_group), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_persona_group_get_current_index(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, last_snapshot);
    EGUI_TEST_ASSERT_EQUAL_INT(2, last_index);

    egui_view_persona_group_set_current_index(EGUI_VIEW_OF(&test_group), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);

    egui_view_persona_group_set_current_index(EGUI_VIEW_OF(&test_group), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_persona_group_get_current_index(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);

    test_group.pressed_index = 1;
    egui_view_persona_group_set_current_snapshot(EGUI_VIEW_OF(&test_group), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_persona_group_get_current_snapshot(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_persona_group_get_current_index(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS, test_group.pressed_index);
    EGUI_TEST_ASSERT_EQUAL_INT(2, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, last_snapshot);
    EGUI_TEST_ASSERT_EQUAL_INT(1, last_index);

    egui_view_persona_group_set_current_snapshot(EGUI_VIEW_OF(&test_group), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_persona_group_get_current_snapshot(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, changed_count);

    egui_view_persona_group_set_current_snapshot(EGUI_VIEW_OF(&test_group), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_persona_group_get_current_snapshot(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_persona_group_get_current_index(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, last_snapshot);
    EGUI_TEST_ASSERT_EQUAL_INT(2, last_index);
}

static void test_persona_group_font_modes_palette_and_helpers(void)
{
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);

    setup_group();

    egui_view_persona_group_set_font(EGUI_VIEW_OF(&test_group), NULL);
    egui_view_persona_group_set_meta_font(EGUI_VIEW_OF(&test_group), NULL);
    EGUI_TEST_ASSERT_TRUE(test_group.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_group.meta_font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    test_group.pressed_index = 2;
    egui_view_persona_group_set_compact_mode(EGUI_VIEW_OF(&test_group), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_group.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_group.pressed_index);
    egui_view_persona_group_set_compact_mode(EGUI_VIEW_OF(&test_group), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_group.compact_mode);

    egui_view_set_pressed(EGUI_VIEW_OF(&test_group), true);
    test_group.pressed_index = 1;
    egui_view_persona_group_set_read_only_mode(EGUI_VIEW_OF(&test_group), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_group.read_only_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS, test_group.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_group)->is_pressed);
    egui_view_persona_group_set_read_only_mode(EGUI_VIEW_OF(&test_group), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_group.read_only_mode);

    egui_view_persona_group_set_palette(EGUI_VIEW_OF(&test_group), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                        EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152), EGUI_COLOR_HEX(0x606162), EGUI_COLOR_HEX(0x707172),
                                        EGUI_COLOR_HEX(0x808182), EGUI_COLOR_HEX(0x909192));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_group.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_group.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_group.section_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_group.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_group.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, test_group.accent_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x707172).full, test_group.success_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full, test_group.warning_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x909192).full, test_group.neutral_color.full);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PERSONA_GROUP_MAX_SNAPSHOTS, egui_view_persona_group_clamp_snapshot_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS, egui_view_persona_group_clamp_item_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_persona_group_text_len(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_persona_group_text_len("Lena"));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x707172).full, egui_view_persona_group_presence_color(&test_group, EGUI_VIEW_PERSONA_GROUP_PRESENCE_LIVE).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full, egui_view_persona_group_presence_color(&test_group, EGUI_VIEW_PERSONA_GROUP_PRESENCE_BUSY).full);
    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(EGUI_COLOR_HEX(0x808182), EGUI_COLOR_HEX(0x606162), 38).full,
                               egui_view_persona_group_presence_color(&test_group, EGUI_VIEW_PERSONA_GROUP_PRESENCE_AWAY).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x909192).full, egui_view_persona_group_presence_color(&test_group, EGUI_VIEW_PERSONA_GROUP_PRESENCE_IDLE).full);
    EGUI_TEST_ASSERT_EQUAL_INT(54, egui_view_persona_group_footer_width("Design", 0, 60));
    EGUI_TEST_ASSERT_EQUAL_INT(34, egui_view_persona_group_footer_width("Team", 1, 40));
    EGUI_TEST_ASSERT_EQUAL_INT(30, egui_view_persona_group_footer_width("Long", 0, 30));
    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, 66).full, egui_view_persona_group_mix_disabled(sample).full);

    EGUI_TEST_ASSERT_TRUE(egui_view_persona_group_get_snapshot(&test_group) == &g_snapshots[0]);
    EGUI_TEST_ASSERT_TRUE(egui_view_persona_group_get_item(&test_group) == &g_items_0[0]);
    test_group.current_index = 9;
    EGUI_TEST_ASSERT_NULL(egui_view_persona_group_get_item(&test_group));
    test_group.current_index = 0;
    test_group.current_snapshot = 9;
    EGUI_TEST_ASSERT_NULL(egui_view_persona_group_get_snapshot(&test_group));
}

static void test_persona_group_metrics_and_hit_testing(void)
{
    egui_view_persona_group_metrics_t metrics;
    egui_dim_t x0;
    egui_dim_t y0;
    egui_dim_t x3;
    egui_dim_t y3;
    egui_dim_t overflow_x;
    egui_dim_t overflow_y;

    setup_group();
    layout_group(194, 114);
    get_metrics(&metrics);

    EGUI_TEST_ASSERT_TRUE(metrics.content_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.avatar_regions[0].size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.avatar_regions[1].location.x > metrics.avatar_regions[0].location.x);
    EGUI_TEST_ASSERT_TRUE(metrics.overflow_region.size.width > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(5, metrics.bubble_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PERSONA_GROUP_STANDARD_HEADER_HEIGHT, metrics.eyebrow_region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PERSONA_GROUP_STANDARD_TITLE_HEIGHT, metrics.title_region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PERSONA_GROUP_STANDARD_FOOTER_HEIGHT, metrics.footer_region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(8, metrics.role_region.size.height);

    get_avatar_center(0, &x0, &y0);
    get_avatar_center(3, &x3, &y3);
    overflow_x = metrics.overflow_region.location.x + metrics.overflow_region.size.width / 2;
    overflow_y = metrics.overflow_region.location.y + metrics.overflow_region.size.height / 2;

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_persona_group_hit_index(&test_group, EGUI_VIEW_OF(&test_group), x0, y0));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_persona_group_hit_index(&test_group, EGUI_VIEW_OF(&test_group), x3, y3));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS,
                               egui_view_persona_group_hit_index(&test_group, EGUI_VIEW_OF(&test_group), overflow_x, overflow_y));
    EGUI_TEST_ASSERT_EQUAL_INT(
            EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS,
            egui_view_persona_group_hit_index(&test_group, EGUI_VIEW_OF(&test_group), metrics.content_region.location.x, metrics.content_region.location.y));

    egui_view_persona_group_set_compact_mode(EGUI_VIEW_OF(&test_group), 1);
    egui_view_set_size(EGUI_VIEW_OF(&test_group), 106, 76);
    layout_group(106, 76);
    get_metrics(&metrics);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PERSONA_GROUP_COMPACT_TITLE_HEIGHT, metrics.title_region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PERSONA_GROUP_COMPACT_FOOTER_HEIGHT, metrics.footer_region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(0, metrics.role_region.size.height);
    EGUI_TEST_ASSERT_TRUE(metrics.avatar_regions[2].size.width > 0);
}

static void test_persona_group_touch_updates_focus_and_guards(void)
{
    egui_dim_t x1;
    egui_dim_t y1;
    egui_dim_t x2;
    egui_dim_t y2;

    setup_group();
    layout_group(194, 114);
    get_avatar_center(1, &x1, &y1);
    get_avatar_center(2, &x2, &y2);

    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 0, 0));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x2, y2));
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_group.pressed_index);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_group)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x2, y2));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_persona_group_get_current_index(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, last_snapshot);
    EGUI_TEST_ASSERT_EQUAL_INT(2, last_index);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS, test_group.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_group)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x1, y1));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_group.pressed_index);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_CANCEL, x1, y1));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_persona_group_get_current_index(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS, test_group.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_group)->is_pressed);

    egui_view_persona_group_set_read_only_mode(EGUI_VIEW_OF(&test_group), 1);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x1, y1));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x1, y1));

    egui_view_persona_group_set_read_only_mode(EGUI_VIEW_OF(&test_group), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_group), 0);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x1, y1));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x1, y1));
}

static void test_persona_group_keyboard_navigation_and_guards(void)
{
    setup_group();

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_persona_group_get_current_index(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_persona_group_get_current_index(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, last_snapshot);
    EGUI_TEST_ASSERT_EQUAL_INT(1, last_index);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_persona_group_get_current_index(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_persona_group_get_current_index(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_persona_group_get_current_index(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_persona_group_get_current_index(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_UP));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_persona_group_get_current_index(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, changed_count);

    egui_view_persona_group_set_current_snapshot(EGUI_VIEW_OF(&test_group), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_persona_group_get_current_snapshot(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_persona_group_get_current_index(EGUI_VIEW_OF(&test_group)));

    egui_view_persona_group_set_read_only_mode(EGUI_VIEW_OF(&test_group), 1);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_RIGHT));
    egui_view_persona_group_set_read_only_mode(EGUI_VIEW_OF(&test_group), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_group), 0);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
}

void test_persona_group_run(void)
{
    EGUI_TEST_SUITE_BEGIN(persona_group);
    EGUI_TEST_RUN(test_persona_group_set_snapshots_clamp_and_reset_focus);
    EGUI_TEST_RUN(test_persona_group_snapshot_and_index_guards_notify);
    EGUI_TEST_RUN(test_persona_group_font_modes_palette_and_helpers);
    EGUI_TEST_RUN(test_persona_group_metrics_and_hit_testing);
    EGUI_TEST_RUN(test_persona_group_touch_updates_focus_and_guards);
    EGUI_TEST_RUN(test_persona_group_keyboard_navigation_and_guards);
    EGUI_TEST_SUITE_END();
}
