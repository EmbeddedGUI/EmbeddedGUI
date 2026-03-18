#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_badge_group.h"

#include "../../HelloCustomWidgets/display/badge_group/egui_view_badge_group.h"
#include "../../HelloCustomWidgets/display/badge_group/egui_view_badge_group.c"

static egui_view_badge_group_t test_group;
static int click_count;

static const egui_view_badge_group_item_t g_items_0[] = {
        {"Review", "4", 0, 1, 0},
        {"Ready", "12", 1, 1, 0},
        {"Risk", "1", 2, 0, 1},
        {"Archive", "7", 3, 0, 1},
};

static const egui_view_badge_group_item_t g_items_1[] = {
        {"Online", "8", 1, 1, 0},
        {"Shadow", "2", 3, 0, 1},
        {"Sync", "3", 0, 0, 1},
        {"Alert", "1", 2, 1, 0},
};

static const egui_view_badge_group_item_t g_items_2[] = {
        {"Queued", "5", 3, 0, 1},
        {"Hold", "2", 2, 1, 0},
        {"Owner", "A", 0, 1, 0},
        {"Done", "9", 1, 0, 1},
};

static const egui_view_badge_group_item_t g_items_3[] = {
        {"Pinned", "6", 0, 0, 1},
        {"Calm", "3", 3, 1, 0},
        {"Watch", "2", 2, 0, 1},
        {"Live", "4", 1, 0, 1},
};

static const egui_view_badge_group_item_t g_extra_items[] = {
        {"A", "1", 0, 0, 0}, {"B", "2", 1, 1, 0}, {"C", "3", 2, 0, 1}, {"D", "4", 3, 1, 0}, {"E", "5", 0, 0, 1}, {"F", "6", 1, 0, 1}, {"G", "7", 2, 1, 0},
};

static const egui_view_badge_group_snapshot_t g_snapshots[] = {
        {"TRIAGE", "Release lanes", "Mixed badges stay aligned.", "Summary follows focus.", g_items_0, 4, 0},
        {"QUEUE", "Ops handoff", "Success tone leads the row.", "Success drives footer.", g_items_1, 4, 0},
        {"RISK", "Change review", "Warning focus stays visible.", "Warning stays visible.", g_items_2, 4, 1},
        {"CALM", "Archive sweep", "Neutral focus softens the card.", "Neutral stays calm.", g_items_3, 4, 1},
};

static const egui_view_badge_group_snapshot_t g_overflow_snapshots[] = {
        {"A", "A", "A", "A", g_extra_items, 7, 0}, {"B", "B", "B", "B", g_extra_items, 7, 1}, {"C", "C", "C", "C", g_extra_items, 7, 2},
        {"D", "D", "D", "D", g_extra_items, 7, 3}, {"E", "E", "E", "E", g_extra_items, 7, 4}, {"F", "F", "F", "F", g_extra_items, 7, 5},
        {"G", "G", "G", "G", g_extra_items, 7, 6},
};

static const egui_view_badge_group_snapshot_t g_invalid_focus_snapshot = {
        "T", "Title", "Body", "Footer", g_items_0, 4, 9,
};

static void on_group_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    click_count++;
}

static void setup_group(void)
{
    egui_view_badge_group_init(EGUI_VIEW_OF(&test_group));
    egui_view_set_size(EGUI_VIEW_OF(&test_group), 196, 118);
    egui_view_badge_group_set_snapshots(EGUI_VIEW_OF(&test_group), g_snapshots, 4);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&test_group), on_group_click);
    click_count = 0;
}

static void layout_group(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 196;
    region.size.height = 118;
    egui_view_layout(EGUI_VIEW_OF(&test_group), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_group)->region_screen, &region);
}

static int send_touch(uint8_t type)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = 54;
    event.location.y = 58;
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

static void test_badge_group_set_snapshots_clamp_and_reset_current(void)
{
    setup_group();

    egui_view_badge_group_set_snapshots(EGUI_VIEW_OF(&test_group), g_overflow_snapshots, 7);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_BADGE_GROUP_MAX_SNAPSHOTS, test_group.snapshot_count);

    test_group.current_snapshot = 5;
    egui_view_badge_group_set_snapshots(EGUI_VIEW_OF(&test_group), g_snapshots, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_group.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_badge_group_get_current_snapshot(EGUI_VIEW_OF(&test_group)));

    egui_view_badge_group_set_snapshots(EGUI_VIEW_OF(&test_group), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_group.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_badge_group_get_current_snapshot(EGUI_VIEW_OF(&test_group)));
}

static void test_badge_group_set_current_snapshot_ignores_out_of_range(void)
{
    setup_group();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_badge_group_get_current_snapshot(EGUI_VIEW_OF(&test_group)));
    egui_view_badge_group_set_current_snapshot(EGUI_VIEW_OF(&test_group), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_badge_group_get_current_snapshot(EGUI_VIEW_OF(&test_group)));

    egui_view_badge_group_set_current_snapshot(EGUI_VIEW_OF(&test_group), 8);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_badge_group_get_current_snapshot(EGUI_VIEW_OF(&test_group)));

    egui_view_badge_group_set_snapshots(EGUI_VIEW_OF(&test_group), NULL, 0);
    egui_view_badge_group_set_current_snapshot(EGUI_VIEW_OF(&test_group), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_badge_group_get_current_snapshot(EGUI_VIEW_OF(&test_group)));
}

static void test_badge_group_font_modes_and_palette_update(void)
{
    setup_group();

    egui_view_badge_group_set_font(EGUI_VIEW_OF(&test_group), NULL);
    egui_view_badge_group_set_meta_font(EGUI_VIEW_OF(&test_group), NULL);
    EGUI_TEST_ASSERT_TRUE(test_group.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_group.meta_font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_badge_group_set_compact_mode(EGUI_VIEW_OF(&test_group), 2);
    egui_view_badge_group_set_locked_mode(EGUI_VIEW_OF(&test_group), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_group.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_group.locked_mode);

    egui_view_badge_group_set_compact_mode(EGUI_VIEW_OF(&test_group), 0);
    egui_view_badge_group_set_locked_mode(EGUI_VIEW_OF(&test_group), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_group.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_group.locked_mode);

    egui_view_badge_group_set_palette(EGUI_VIEW_OF(&test_group), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                      EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152), EGUI_COLOR_HEX(0x606162), EGUI_COLOR_HEX(0x707172),
                                      EGUI_COLOR_HEX(0x808182));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_group.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_group.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_group.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_group.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_group.accent_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, test_group.success_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x707172).full, test_group.warning_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full, test_group.neutral_color.full);
}

static void test_badge_group_touch_and_key_click_listener(void)
{
    setup_group();
    layout_group();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_group)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_group)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(1, click_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(2, click_count);

    egui_view_set_enable(EGUI_VIEW_OF(&test_group), 0);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_group)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(2, click_count);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
}

static void test_badge_group_internal_helpers_cover_focus_tone_text_and_width(void)
{
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);
    egui_color_t mixed = egui_view_badge_group_mix_disabled(sample);

    setup_group();
    egui_view_badge_group_set_palette(EGUI_VIEW_OF(&test_group), EGUI_COLOR_HEX(0x111111), EGUI_COLOR_HEX(0x222222), EGUI_COLOR_HEX(0x333333),
                                      EGUI_COLOR_HEX(0x444444), EGUI_COLOR_HEX(0x555555), EGUI_COLOR_HEX(0x666666), EGUI_COLOR_HEX(0x777777),
                                      EGUI_COLOR_HEX(0x888888));

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_BADGE_GROUP_MAX_SNAPSHOTS, egui_view_badge_group_clamp_snapshot_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_BADGE_GROUP_MAX_ITEMS, egui_view_badge_group_clamp_item_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_badge_group_text_len(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(6, egui_view_badge_group_text_len("Review"));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_badge_group_focus_index(NULL, 1));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_badge_group_focus_index(&g_invalid_focus_snapshot, 4));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_badge_group_focus_index(&g_snapshots[2], 4));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x555555).full, egui_view_badge_group_tone_color(&test_group, 0).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x666666).full, egui_view_badge_group_tone_color(&test_group, 1).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x777777).full, egui_view_badge_group_tone_color(&test_group, 2).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x888888).full, egui_view_badge_group_tone_color(&test_group, 3).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x555555).full, egui_view_badge_group_tone_color(&test_group, 9).full);
    EGUI_TEST_ASSERT_EQUAL_INT(24, egui_view_badge_group_pill_width("", 1, 24, 64));
    EGUI_TEST_ASSERT_EQUAL_INT(38, egui_view_badge_group_pill_width("AB", 0, 28, 50));
    EGUI_TEST_ASSERT_EQUAL_INT(40, egui_view_badge_group_pill_width("Long label", 0, 28, 40));
    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, 68).full, mixed.full);
}

void test_badge_group_run(void)
{
    EGUI_TEST_SUITE_BEGIN(badge_group);
    EGUI_TEST_RUN(test_badge_group_set_snapshots_clamp_and_reset_current);
    EGUI_TEST_RUN(test_badge_group_set_current_snapshot_ignores_out_of_range);
    EGUI_TEST_RUN(test_badge_group_font_modes_and_palette_update);
    EGUI_TEST_RUN(test_badge_group_touch_and_key_click_listener);
    EGUI_TEST_RUN(test_badge_group_internal_helpers_cover_focus_tone_text_and_width);
    EGUI_TEST_SUITE_END();
}
