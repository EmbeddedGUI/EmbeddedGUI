#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_card_panel.h"

#include "../../HelloCustomWidgets/display/card_panel/egui_view_card_panel.h"
#include "../../HelloCustomWidgets/display/card_panel/egui_view_card_panel.c"

static egui_view_card_panel_t test_panel;
static int click_count;

static const egui_view_card_panel_snapshot_t g_snapshots[] = {
        {"OVERVIEW", "Workspace status", "Three flows stay aligned.", "98%", "uptime", "Today", "Two checks wait.", "Footer stays readable.", "Open", 0, 1},
        {"SYNC", "Design review", "New handoff needs approval.", "4", "changes", "Next step", "Confirm spacing tokens.", "Summary stays close.", "Review", 2,
         1},
        {"DEPLOY", "Release notes", "Ready for staged publish.", "6", "items", "Channel", "Internal preview for QA.", "Card stays calm on dense pages.",
         "Publish", 1, 0},
        {"ARCHIVE", "Readback summary", "Older detail stays available.", "12", "pages", "History", "Summary stays visible.", "Read only mode still works.",
         "Browse", 3, 0},
};

static const egui_view_card_panel_snapshot_t g_overflow_snapshots[] = {
        {"A", "A", "A", "1", "a", "A", "A", "A", "A", 0, 0}, {"B", "B", "B", "2", "b", "B", "B", "B", "B", 1, 1},
        {"C", "C", "C", "3", "c", "C", "C", "C", "C", 2, 0}, {"D", "D", "D", "4", "d", "D", "D", "D", "D", 3, 1},
        {"E", "E", "E", "5", "e", "E", "E", "E", "E", 0, 0}, {"F", "F", "F", "6", "f", "F", "F", "F", "F", 1, 1},
        {"G", "G", "G", "7", "g", "G", "G", "G", "G", 2, 0},
};

static void on_panel_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    click_count++;
}

static void setup_panel(void)
{
    egui_view_card_panel_init(EGUI_VIEW_OF(&test_panel));
    egui_view_set_size(EGUI_VIEW_OF(&test_panel), 196, 122);
    egui_view_card_panel_set_snapshots(EGUI_VIEW_OF(&test_panel), g_snapshots, 4);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&test_panel), on_panel_click);
    click_count = 0;
}

static void layout_panel(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 196;
    region.size.height = 122;
    egui_view_layout(EGUI_VIEW_OF(&test_panel), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_panel)->region_screen, &region);
}

static int send_touch(uint8_t type)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = 52;
    event.location.y = 60;
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

static void test_card_panel_set_snapshots_clamp_and_reset_current(void)
{
    setup_panel();

    egui_view_card_panel_set_snapshots(EGUI_VIEW_OF(&test_panel), g_overflow_snapshots, 7);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_CARD_PANEL_MAX_SNAPSHOTS, test_panel.snapshot_count);

    test_panel.current_snapshot = 5;
    egui_view_card_panel_set_snapshots(EGUI_VIEW_OF(&test_panel), g_snapshots, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_panel.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_card_panel_get_current_snapshot(EGUI_VIEW_OF(&test_panel)));

    egui_view_card_panel_set_snapshots(EGUI_VIEW_OF(&test_panel), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_panel.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_card_panel_get_current_snapshot(EGUI_VIEW_OF(&test_panel)));
}

static void test_card_panel_set_current_snapshot_ignores_out_of_range(void)
{
    setup_panel();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_card_panel_get_current_snapshot(EGUI_VIEW_OF(&test_panel)));
    egui_view_card_panel_set_current_snapshot(EGUI_VIEW_OF(&test_panel), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_card_panel_get_current_snapshot(EGUI_VIEW_OF(&test_panel)));

    egui_view_card_panel_set_current_snapshot(EGUI_VIEW_OF(&test_panel), 8);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_card_panel_get_current_snapshot(EGUI_VIEW_OF(&test_panel)));

    egui_view_card_panel_set_snapshots(EGUI_VIEW_OF(&test_panel), NULL, 0);
    egui_view_card_panel_set_current_snapshot(EGUI_VIEW_OF(&test_panel), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_card_panel_get_current_snapshot(EGUI_VIEW_OF(&test_panel)));
}

static void test_card_panel_font_modes_and_palette_update(void)
{
    setup_panel();

    egui_view_card_panel_set_font(EGUI_VIEW_OF(&test_panel), NULL);
    egui_view_card_panel_set_meta_font(EGUI_VIEW_OF(&test_panel), NULL);
    EGUI_TEST_ASSERT_TRUE(test_panel.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_panel.meta_font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_card_panel_set_compact_mode(EGUI_VIEW_OF(&test_panel), 2);
    egui_view_card_panel_set_locked_mode(EGUI_VIEW_OF(&test_panel), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_panel.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_panel.locked_mode);

    egui_view_card_panel_set_compact_mode(EGUI_VIEW_OF(&test_panel), 0);
    egui_view_card_panel_set_locked_mode(EGUI_VIEW_OF(&test_panel), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_panel.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_panel.locked_mode);

    egui_view_card_panel_set_palette(EGUI_VIEW_OF(&test_panel), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                     EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152), EGUI_COLOR_HEX(0x606162), EGUI_COLOR_HEX(0x707172),
                                     EGUI_COLOR_HEX(0x808182));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_panel.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_panel.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_panel.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_panel.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_panel.accent_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, test_panel.success_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x707172).full, test_panel.warning_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full, test_panel.neutral_color.full);
}

static void test_card_panel_touch_and_key_click_listener(void)
{
    setup_panel();
    layout_panel();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_panel)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_panel)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(1, click_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(2, click_count);

    egui_view_set_enable(EGUI_VIEW_OF(&test_panel), 0);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_panel)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(2, click_count);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
}

static void test_card_panel_internal_helpers_cover_tone_text_and_pill_width(void)
{
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);
    egui_color_t mixed = egui_view_card_panel_mix_disabled(sample);

    setup_panel();
    egui_view_card_panel_set_palette(EGUI_VIEW_OF(&test_panel), EGUI_COLOR_HEX(0x111111), EGUI_COLOR_HEX(0x222222), EGUI_COLOR_HEX(0x333333),
                                     EGUI_COLOR_HEX(0x444444), EGUI_COLOR_HEX(0x555555), EGUI_COLOR_HEX(0x666666), EGUI_COLOR_HEX(0x777777),
                                     EGUI_COLOR_HEX(0x888888));

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_CARD_PANEL_MAX_SNAPSHOTS, egui_view_card_panel_clamp_snapshot_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_card_panel_text_len(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(6, egui_view_card_panel_text_len("Review"));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x555555).full, egui_view_card_panel_tone_color(&test_panel, 0).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x666666).full, egui_view_card_panel_tone_color(&test_panel, 1).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x777777).full, egui_view_card_panel_tone_color(&test_panel, 2).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x888888).full, egui_view_card_panel_tone_color(&test_panel, 3).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x555555).full, egui_view_card_panel_tone_color(&test_panel, 9).full);
    EGUI_TEST_ASSERT_EQUAL_INT(34, egui_view_card_panel_pill_width("", 1, 34, 64));
    EGUI_TEST_ASSERT_EQUAL_INT(52, egui_view_card_panel_pill_width("AB", 0, 42, 78));
    EGUI_TEST_ASSERT_EQUAL_INT(50, egui_view_card_panel_pill_width("Long label", 0, 42, 50));
    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, 68).full, mixed.full);
}

void test_card_panel_run(void)
{
    EGUI_TEST_SUITE_BEGIN(card_panel);
    EGUI_TEST_RUN(test_card_panel_set_snapshots_clamp_and_reset_current);
    EGUI_TEST_RUN(test_card_panel_set_current_snapshot_ignores_out_of_range);
    EGUI_TEST_RUN(test_card_panel_font_modes_and_palette_update);
    EGUI_TEST_RUN(test_card_panel_touch_and_key_click_listener);
    EGUI_TEST_RUN(test_card_panel_internal_helpers_cover_tone_text_and_pill_width);
    EGUI_TEST_SUITE_END();
}
