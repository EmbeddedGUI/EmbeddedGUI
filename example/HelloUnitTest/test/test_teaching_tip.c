#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_teaching_tip.h"

#include "../../HelloCustomWidgets/feedback/teaching_tip/egui_view_teaching_tip.h"
#include "../../HelloCustomWidgets/feedback/teaching_tip/egui_view_teaching_tip.c"

static egui_view_teaching_tip_t test_tip;
static uint8_t changed_count;
static uint8_t last_part;

static const egui_view_teaching_tip_snapshot_t g_snapshots[] = {
        {"Quick filters", "Coachmark", "Pin today view", "Keep ship dates nearby.", "Pin tip", "Later", "Below target", EGUI_VIEW_TEACHING_TIP_TONE_ACCENT,
         EGUI_VIEW_TEACHING_TIP_PLACEMENT_BOTTOM, 1, 1, 1, 1, EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, -18},
        {"Cmd palette", "Shortcut", "Press slash to search", "Jump to commands fast.", "Got it", "Tips", "Placement above target",
         EGUI_VIEW_TEACHING_TIP_TONE_SUCCESS, EGUI_VIEW_TEACHING_TIP_PLACEMENT_TOP, 1, 1, 1, 1, EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, 22},
        {"Quick filters", "", "Tip hidden", "Tap target to reopen", "", "", "", EGUI_VIEW_TEACHING_TIP_TONE_NEUTRAL, EGUI_VIEW_TEACHING_TIP_PLACEMENT_BOTTOM, 0,
         0, 0, 0, EGUI_VIEW_TEACHING_TIP_PART_TARGET, 0},
};

static const egui_view_teaching_tip_snapshot_t g_overflow_snapshots[] = {
        {"A", "A", "A", "A", "A", "B", "A", EGUI_VIEW_TEACHING_TIP_TONE_ACCENT, EGUI_VIEW_TEACHING_TIP_PLACEMENT_BOTTOM, 1, 1, 1, 1,
         EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, 0},
        {"B", "B", "B", "B", "B", "C", "B", EGUI_VIEW_TEACHING_TIP_TONE_SUCCESS, EGUI_VIEW_TEACHING_TIP_PLACEMENT_TOP, 1, 1, 1, 1,
         EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, 0},
        {"C", "C", "C", "C", "C", "", "C", EGUI_VIEW_TEACHING_TIP_TONE_WARNING, EGUI_VIEW_TEACHING_TIP_PLACEMENT_BOTTOM, 1, 1, 0, 1,
         EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, 0},
        {"D", "", "D", "D", "", "", "", EGUI_VIEW_TEACHING_TIP_TONE_NEUTRAL, EGUI_VIEW_TEACHING_TIP_PLACEMENT_BOTTOM, 0, 0, 0, 0,
         EGUI_VIEW_TEACHING_TIP_PART_TARGET, 0},
        {"E", "E", "E", "E", "E", "F", "E", EGUI_VIEW_TEACHING_TIP_TONE_ACCENT, EGUI_VIEW_TEACHING_TIP_PLACEMENT_BOTTOM, 1, 1, 1, 1,
         EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, 0},
        {"F", "F", "F", "F", "F", "G", "F", EGUI_VIEW_TEACHING_TIP_TONE_SUCCESS, EGUI_VIEW_TEACHING_TIP_PLACEMENT_TOP, 1, 1, 1, 1,
         EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, 0},
        {"G", "G", "G", "G", "G", "H", "G", EGUI_VIEW_TEACHING_TIP_TONE_WARNING, EGUI_VIEW_TEACHING_TIP_PLACEMENT_BOTTOM, 1, 1, 1, 1,
         EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, 0},
};

static const egui_view_teaching_tip_snapshot_t g_secondary_only_snapshot = {
        "Warn",
        "Fallback",
        "Secondary only",
        "Fallback keeps the right action.",
        NULL,
        "Later",
        "",
        EGUI_VIEW_TEACHING_TIP_TONE_WARNING,
        EGUI_VIEW_TEACHING_TIP_PLACEMENT_BOTTOM,
        1,
        0,
        1,
        0,
        EGUI_VIEW_TEACHING_TIP_PART_PRIMARY,
        0,
};

static const egui_view_teaching_tip_snapshot_t g_close_only_snapshot = {
        "Hint",
        "Close only",
        "Dismiss helper",
        "Only the close affordance remains.",
        NULL,
        NULL,
        "",
        EGUI_VIEW_TEACHING_TIP_TONE_ACCENT,
        EGUI_VIEW_TEACHING_TIP_PLACEMENT_BOTTOM,
        1,
        0,
        0,
        1,
        EGUI_VIEW_TEACHING_TIP_PART_PRIMARY,
        0,
};

static const egui_view_teaching_tip_snapshot_t g_closed_snapshot = {
        "Quick filters",
        "",
        "Tip hidden",
        "Tap target to reopen",
        "",
        "",
        "",
        EGUI_VIEW_TEACHING_TIP_TONE_NEUTRAL,
        EGUI_VIEW_TEACHING_TIP_PLACEMENT_BOTTOM,
        0,
        0,
        0,
        0,
        EGUI_VIEW_TEACHING_TIP_PART_PRIMARY,
        0,
};

static void reset_changed_state(void)
{
    changed_count = 0;
    last_part = EGUI_VIEW_TEACHING_TIP_PART_NONE;
}

static void on_part_changed(egui_view_t *self, uint8_t part)
{
    EGUI_UNUSED(self);
    changed_count++;
    last_part = part;
}

static void setup_tip(void)
{
    egui_view_teaching_tip_init(EGUI_VIEW_OF(&test_tip));
    egui_view_set_size(EGUI_VIEW_OF(&test_tip), 196, 132);
    egui_view_teaching_tip_set_snapshots(EGUI_VIEW_OF(&test_tip), g_snapshots, 3);
    egui_view_teaching_tip_set_on_part_changed_listener(EGUI_VIEW_OF(&test_tip), on_part_changed);
    reset_changed_state();
}

static void layout_tip(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 196;
    region.size.height = 132;
    egui_view_layout(EGUI_VIEW_OF(&test_tip), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_tip)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_tip)->api->on_touch_event(EGUI_VIEW_OF(&test_tip), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_tip)->api->on_key_event(EGUI_VIEW_OF(&test_tip), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_tip)->api->on_key_event(EGUI_VIEW_OF(&test_tip), &event);
    return handled;
}

static void get_metrics(egui_view_teaching_tip_metrics_t *metrics)
{
    const egui_view_teaching_tip_snapshot_t *snapshot = egui_view_teaching_tip_get_snapshot(&test_tip);

    EGUI_TEST_ASSERT_NOT_NULL(snapshot);
    egui_view_teaching_tip_get_metrics(&test_tip, EGUI_VIEW_OF(&test_tip), snapshot, metrics);
}

static void get_region_center(egui_region_t *region, egui_dim_t *x, egui_dim_t *y)
{
    *x = region->location.x + region->size.width / 2;
    *y = region->location.y + region->size.height / 2;
}

static void test_teaching_tip_set_snapshots_clamp_and_default_part(void)
{
    setup_tip();

    egui_view_teaching_tip_set_snapshots(EGUI_VIEW_OF(&test_tip), g_overflow_snapshots, 7);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_MAX_SNAPSHOTS, test_tip.snapshot_count);

    test_tip.current_snapshot = 5;
    test_tip.current_part = EGUI_VIEW_TEACHING_TIP_PART_CLOSE;
    test_tip.pressed_part = EGUI_VIEW_TEACHING_TIP_PART_PRIMARY;
    egui_view_teaching_tip_set_snapshots(EGUI_VIEW_OF(&test_tip), g_snapshots, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_tip.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_teaching_tip_get_current_snapshot(EGUI_VIEW_OF(&test_tip)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_NONE, test_tip.pressed_part);

    egui_view_teaching_tip_set_snapshots(EGUI_VIEW_OF(&test_tip), &g_secondary_only_snapshot, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));

    egui_view_teaching_tip_set_snapshots(EGUI_VIEW_OF(&test_tip), &g_close_only_snapshot, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_CLOSE, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));

    egui_view_teaching_tip_set_snapshots(EGUI_VIEW_OF(&test_tip), &g_closed_snapshot, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_TARGET, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));

    egui_view_teaching_tip_set_snapshots(EGUI_VIEW_OF(&test_tip), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_tip.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_teaching_tip_get_current_snapshot(EGUI_VIEW_OF(&test_tip)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_NONE, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));
}

static void test_teaching_tip_snapshot_and_part_guards(void)
{
    setup_tip();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_teaching_tip_get_current_snapshot(EGUI_VIEW_OF(&test_tip)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));

    egui_view_teaching_tip_set_current_part(EGUI_VIEW_OF(&test_tip), EGUI_VIEW_TEACHING_TIP_PART_SECONDARY);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);

    egui_view_teaching_tip_set_current_part(EGUI_VIEW_OF(&test_tip), EGUI_VIEW_TEACHING_TIP_PART_NONE);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));

    test_tip.pressed_part = EGUI_VIEW_TEACHING_TIP_PART_CLOSE;
    egui_view_teaching_tip_set_current_snapshot(EGUI_VIEW_OF(&test_tip), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_teaching_tip_get_current_snapshot(EGUI_VIEW_OF(&test_tip)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_NONE, test_tip.pressed_part);

    egui_view_teaching_tip_set_current_snapshot(EGUI_VIEW_OF(&test_tip), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_teaching_tip_get_current_snapshot(EGUI_VIEW_OF(&test_tip)));

    egui_view_teaching_tip_set_snapshots(EGUI_VIEW_OF(&test_tip), &g_secondary_only_snapshot, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));
    egui_view_teaching_tip_set_current_part(EGUI_VIEW_OF(&test_tip), EGUI_VIEW_TEACHING_TIP_PART_PRIMARY);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));

    egui_view_teaching_tip_set_snapshots(EGUI_VIEW_OF(&test_tip), &g_closed_snapshot, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_TARGET, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));
    egui_view_teaching_tip_set_current_part(EGUI_VIEW_OF(&test_tip), EGUI_VIEW_TEACHING_TIP_PART_PRIMARY);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_TARGET, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));
}

static void test_teaching_tip_font_palette_and_internal_helpers(void)
{
    uint8_t parts[4];
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);
    egui_color_t mixed = egui_view_teaching_tip_mix_disabled(sample);

    setup_tip();

    egui_view_teaching_tip_set_font(EGUI_VIEW_OF(&test_tip), NULL);
    egui_view_teaching_tip_set_meta_font(EGUI_VIEW_OF(&test_tip), NULL);
    EGUI_TEST_ASSERT_TRUE(test_tip.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_tip.meta_font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_teaching_tip_set_compact_mode(EGUI_VIEW_OF(&test_tip), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_tip.compact_mode);
    egui_view_teaching_tip_set_compact_mode(EGUI_VIEW_OF(&test_tip), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_tip.compact_mode);

    egui_view_teaching_tip_set_read_only_mode(EGUI_VIEW_OF(&test_tip), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_tip.read_only_mode);
    egui_view_teaching_tip_set_read_only_mode(EGUI_VIEW_OF(&test_tip), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_tip.read_only_mode);

    egui_view_teaching_tip_set_palette(EGUI_VIEW_OF(&test_tip), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                       EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152), EGUI_COLOR_HEX(0x606162), EGUI_COLOR_HEX(0x707172),
                                       EGUI_COLOR_HEX(0x808182), EGUI_COLOR_HEX(0x909192));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_tip.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_tip.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_tip.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_tip.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_tip.accent_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, test_tip.success_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x707172).full, test_tip.warning_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full, test_tip.neutral_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x909192).full, test_tip.shadow_color.full);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_MAX_SNAPSHOTS, egui_view_teaching_tip_clamp_snapshot_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_teaching_tip_text_len(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_teaching_tip_text_len("Later"));
    EGUI_TEST_ASSERT_FALSE(egui_view_teaching_tip_has_text(NULL));
    EGUI_TEST_ASSERT_FALSE(egui_view_teaching_tip_has_text(""));
    EGUI_TEST_ASSERT_TRUE(egui_view_teaching_tip_has_text("Tip"));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, egui_view_teaching_tip_tone_color(&test_tip, EGUI_VIEW_TEACHING_TIP_TONE_ACCENT).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, egui_view_teaching_tip_tone_color(&test_tip, EGUI_VIEW_TEACHING_TIP_TONE_SUCCESS).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x707172).full, egui_view_teaching_tip_tone_color(&test_tip, EGUI_VIEW_TEACHING_TIP_TONE_WARNING).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full, egui_view_teaching_tip_tone_color(&test_tip, EGUI_VIEW_TEACHING_TIP_TONE_NEUTRAL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, egui_view_teaching_tip_tone_color(&test_tip, 99).full);
    EGUI_TEST_ASSERT_TRUE(egui_view_teaching_tip_get_snapshot(&test_tip) == &g_snapshots[0]);
    test_tip.current_snapshot = 9;
    EGUI_TEST_ASSERT_TRUE(egui_view_teaching_tip_get_snapshot(&test_tip) == NULL);
    test_tip.current_snapshot = 0;

    EGUI_TEST_ASSERT_TRUE(egui_view_teaching_tip_part_is_enabled(&test_tip, EGUI_VIEW_OF(&test_tip), &g_snapshots[0], EGUI_VIEW_TEACHING_TIP_PART_TARGET));
    EGUI_TEST_ASSERT_TRUE(egui_view_teaching_tip_part_is_enabled(&test_tip, EGUI_VIEW_OF(&test_tip), &g_snapshots[0], EGUI_VIEW_TEACHING_TIP_PART_PRIMARY));
    EGUI_TEST_ASSERT_TRUE(egui_view_teaching_tip_part_is_enabled(&test_tip, EGUI_VIEW_OF(&test_tip), &g_snapshots[0], EGUI_VIEW_TEACHING_TIP_PART_SECONDARY));
    EGUI_TEST_ASSERT_TRUE(egui_view_teaching_tip_part_is_enabled(&test_tip, EGUI_VIEW_OF(&test_tip), &g_snapshots[0], EGUI_VIEW_TEACHING_TIP_PART_CLOSE));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_SECONDARY,
                               egui_view_teaching_tip_resolve_default_part(&test_tip, EGUI_VIEW_OF(&test_tip), &g_secondary_only_snapshot));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_CLOSE,
                               egui_view_teaching_tip_resolve_default_part(&test_tip, EGUI_VIEW_OF(&test_tip), &g_close_only_snapshot));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_TARGET,
                               egui_view_teaching_tip_resolve_default_part(&test_tip, EGUI_VIEW_OF(&test_tip), &g_closed_snapshot));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_teaching_tip_collect_parts(&test_tip, EGUI_VIEW_OF(&test_tip), &g_snapshots[0], parts, 4));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_TARGET, parts[0]);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, parts[1]);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, parts[2]);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_CLOSE, parts[3]);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_teaching_tip_find_part_index(parts, 4, EGUI_VIEW_TEACHING_TIP_PART_PRIMARY));
    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, 68).full, mixed.full);

    egui_view_teaching_tip_set_read_only_mode(EGUI_VIEW_OF(&test_tip), 1);
    EGUI_TEST_ASSERT_FALSE(egui_view_teaching_tip_part_is_enabled(&test_tip, EGUI_VIEW_OF(&test_tip), &g_snapshots[0], EGUI_VIEW_TEACHING_TIP_PART_PRIMARY));
    egui_view_teaching_tip_set_read_only_mode(EGUI_VIEW_OF(&test_tip), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_tip), 0);
    EGUI_TEST_ASSERT_FALSE(egui_view_teaching_tip_part_is_enabled(&test_tip, EGUI_VIEW_OF(&test_tip), &g_snapshots[0], EGUI_VIEW_TEACHING_TIP_PART_TARGET));
}

static void test_teaching_tip_metrics_and_hit_testing(void)
{
    egui_view_teaching_tip_metrics_t metrics;
    egui_region_t target_region;
    egui_region_t primary_region;
    egui_region_t secondary_region;
    egui_region_t close_region;
    egui_dim_t target_x;
    egui_dim_t target_y;
    egui_dim_t primary_x;
    egui_dim_t primary_y;
    egui_dim_t secondary_x;
    egui_dim_t secondary_y;
    egui_dim_t close_x;
    egui_dim_t close_y;

    setup_tip();
    layout_tip();
    get_metrics(&metrics);

    EGUI_TEST_ASSERT_TRUE(metrics.show_bubble);
    EGUI_TEST_ASSERT_TRUE(metrics.show_primary);
    EGUI_TEST_ASSERT_TRUE(metrics.show_secondary);
    EGUI_TEST_ASSERT_TRUE(metrics.show_close);
    EGUI_TEST_ASSERT_TRUE(metrics.target_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.bubble_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.primary_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.secondary_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.close_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(metrics.bubble_region.location.y > metrics.target_region.location.y);

    EGUI_TEST_ASSERT_TRUE(egui_view_teaching_tip_get_part_region(EGUI_VIEW_OF(&test_tip), EGUI_VIEW_TEACHING_TIP_PART_TARGET, &target_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_teaching_tip_get_part_region(EGUI_VIEW_OF(&test_tip), EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, &primary_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_teaching_tip_get_part_region(EGUI_VIEW_OF(&test_tip), EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, &secondary_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_teaching_tip_get_part_region(EGUI_VIEW_OF(&test_tip), EGUI_VIEW_TEACHING_TIP_PART_CLOSE, &close_region));

    get_region_center(&target_region, &target_x, &target_y);
    get_region_center(&primary_region, &primary_x, &primary_y);
    get_region_center(&secondary_region, &secondary_x, &secondary_y);
    get_region_center(&close_region, &close_x, &close_y);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_TARGET, egui_view_teaching_tip_hit_part(&test_tip, EGUI_VIEW_OF(&test_tip), target_x, target_y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, egui_view_teaching_tip_hit_part(&test_tip, EGUI_VIEW_OF(&test_tip), primary_x, primary_y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_SECONDARY,
                               egui_view_teaching_tip_hit_part(&test_tip, EGUI_VIEW_OF(&test_tip), secondary_x, secondary_y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_CLOSE, egui_view_teaching_tip_hit_part(&test_tip, EGUI_VIEW_OF(&test_tip), close_x, close_y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_NONE,
                               egui_view_teaching_tip_hit_part(&test_tip, EGUI_VIEW_OF(&test_tip), EGUI_VIEW_OF(&test_tip)->region_screen.location.x,
                                                               EGUI_VIEW_OF(&test_tip)->region_screen.location.y));

    egui_view_teaching_tip_set_current_snapshot(EGUI_VIEW_OF(&test_tip), 1);
    get_metrics(&metrics);
    EGUI_TEST_ASSERT_TRUE(metrics.bubble_region.location.y < metrics.target_region.location.y);

    egui_view_teaching_tip_set_compact_mode(EGUI_VIEW_OF(&test_tip), 1);
    get_metrics(&metrics);
    EGUI_TEST_ASSERT_FALSE(metrics.show_secondary);
    EGUI_TEST_ASSERT_FALSE(metrics.show_close);
    EGUI_TEST_ASSERT_FALSE(metrics.show_body);
    EGUI_TEST_ASSERT_TRUE(egui_view_teaching_tip_get_part_region(EGUI_VIEW_OF(&test_tip), EGUI_VIEW_TEACHING_TIP_PART_TARGET, &target_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_teaching_tip_get_part_region(EGUI_VIEW_OF(&test_tip), EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, &primary_region));
    EGUI_TEST_ASSERT_FALSE(egui_view_teaching_tip_get_part_region(EGUI_VIEW_OF(&test_tip), EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, &secondary_region));
    EGUI_TEST_ASSERT_FALSE(egui_view_teaching_tip_get_part_region(EGUI_VIEW_OF(&test_tip), EGUI_VIEW_TEACHING_TIP_PART_CLOSE, &close_region));

    egui_view_teaching_tip_set_compact_mode(EGUI_VIEW_OF(&test_tip), 0);
    egui_view_teaching_tip_set_snapshots(EGUI_VIEW_OF(&test_tip), &g_closed_snapshot, 1);
    get_metrics(&metrics);
    EGUI_TEST_ASSERT_FALSE(metrics.show_bubble);
    EGUI_TEST_ASSERT_TRUE(metrics.show_closed_hint);
    EGUI_TEST_ASSERT_TRUE(egui_view_teaching_tip_get_part_region(EGUI_VIEW_OF(&test_tip), EGUI_VIEW_TEACHING_TIP_PART_TARGET, &target_region));
    EGUI_TEST_ASSERT_FALSE(egui_view_teaching_tip_get_part_region(EGUI_VIEW_OF(&test_tip), EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, &primary_region));
}

static void test_teaching_tip_touch_interaction_and_cancel(void)
{
    egui_region_t target_region;
    egui_region_t primary_region;
    egui_region_t secondary_region;
    egui_region_t close_region;
    egui_dim_t target_x;
    egui_dim_t target_y;
    egui_dim_t primary_x;
    egui_dim_t primary_y;
    egui_dim_t secondary_x;
    egui_dim_t secondary_y;
    egui_dim_t close_x;
    egui_dim_t close_y;

    setup_tip();
    layout_tip();

    EGUI_TEST_ASSERT_TRUE(egui_view_teaching_tip_get_part_region(EGUI_VIEW_OF(&test_tip), EGUI_VIEW_TEACHING_TIP_PART_TARGET, &target_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_teaching_tip_get_part_region(EGUI_VIEW_OF(&test_tip), EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, &primary_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_teaching_tip_get_part_region(EGUI_VIEW_OF(&test_tip), EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, &secondary_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_teaching_tip_get_part_region(EGUI_VIEW_OF(&test_tip), EGUI_VIEW_TEACHING_TIP_PART_CLOSE, &close_region));

    get_region_center(&target_region, &target_x, &target_y);
    get_region_center(&primary_region, &primary_x, &primary_y);
    get_region_center(&secondary_region, &secondary_x, &secondary_y);
    get_region_center(&close_region, &close_x, &close_y);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, secondary_x, secondary_y));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_tip)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, test_tip.pressed_part);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, secondary_x, secondary_y));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_tip)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_NONE, test_tip.pressed_part);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, last_part);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, close_x, close_y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, target_x, target_y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, primary_x, primary_y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_CANCEL, primary_x, primary_y));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_tip)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_NONE, test_tip.pressed_part);
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, close_x, close_y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, close_x, close_y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_CLOSE, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_CLOSE, last_part);

    egui_view_teaching_tip_set_snapshots(EGUI_VIEW_OF(&test_tip), &g_closed_snapshot, 1);
    layout_tip();
    EGUI_TEST_ASSERT_TRUE(egui_view_teaching_tip_get_part_region(EGUI_VIEW_OF(&test_tip), EGUI_VIEW_TEACHING_TIP_PART_TARGET, &target_region));
    get_region_center(&target_region, &target_x, &target_y);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, target_x, target_y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, target_x, target_y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_TARGET, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_TARGET, last_part);

    egui_view_teaching_tip_set_read_only_mode(EGUI_VIEW_OF(&test_tip), 1);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, target_x, target_y));
    egui_view_teaching_tip_set_read_only_mode(EGUI_VIEW_OF(&test_tip), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_tip), 0);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, target_x, target_y));
}

static void test_teaching_tip_keyboard_navigation_and_guards(void)
{
    setup_tip();
    reset_changed_state();

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, last_part);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_PRIMARY, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_CLOSE, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_TARGET, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_SECONDARY, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ESCAPE));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_TARGET, egui_view_teaching_tip_get_current_part(EGUI_VIEW_OF(&test_tip)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(7, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_TARGET, last_part);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_SPACE));
    EGUI_TEST_ASSERT_EQUAL_INT(8, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TEACHING_TIP_PART_TARGET, last_part);

    egui_view_teaching_tip_set_read_only_mode(EGUI_VIEW_OF(&test_tip), 1);
    reset_changed_state();
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(0, changed_count);

    egui_view_teaching_tip_set_read_only_mode(EGUI_VIEW_OF(&test_tip), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_tip), 0);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_LEFT));
}

void test_teaching_tip_run(void)
{
    EGUI_TEST_SUITE_BEGIN(teaching_tip);
    EGUI_TEST_RUN(test_teaching_tip_set_snapshots_clamp_and_default_part);
    EGUI_TEST_RUN(test_teaching_tip_snapshot_and_part_guards);
    EGUI_TEST_RUN(test_teaching_tip_font_palette_and_internal_helpers);
    EGUI_TEST_RUN(test_teaching_tip_metrics_and_hit_testing);
    EGUI_TEST_RUN(test_teaching_tip_touch_interaction_and_cancel);
    EGUI_TEST_RUN(test_teaching_tip_keyboard_navigation_and_guards);
    EGUI_TEST_SUITE_END();
}
