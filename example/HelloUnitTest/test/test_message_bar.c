#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_message_bar.h"

#include "../../HelloCustomWidgets/feedback/message_bar/egui_view_message_bar.h"
#include "../../HelloCustomWidgets/feedback/message_bar/egui_view_message_bar.c"

static egui_view_message_bar_t test_bar;
static int click_count;

static const egui_view_message_bar_snapshot_t g_snapshots[] = {
        {"Updates ready", "Open latest release notes.", "View notes", 0, 1, 1},
        {"Settings saved", "New defaults are active.", "Open panel", 1, 1, 1},
        {"Storage almost full", "Clear logs before next sync.", "View logs", 2, 1, 1},
        {"Connection lost", "Uploads pause. Link is down.", "Retry now", 3, 1, 1},
};

static const egui_view_message_bar_snapshot_t g_overflow_snapshots[] = {
        {"A", "A", "A", 0, 1, 1}, {"B", "B", "B", 1, 1, 1}, {"C", "C", "C", 2, 1, 1}, {"D", "D", "D", 3, 1, 1},
        {"E", "E", "E", 0, 0, 0}, {"F", "F", "F", 1, 0, 0}, {"G", "G", "G", 2, 0, 0},
};

static void on_bar_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    click_count++;
}

static void setup_bar(void)
{
    egui_view_message_bar_init(EGUI_VIEW_OF(&test_bar));
    egui_view_set_size(EGUI_VIEW_OF(&test_bar), 196, 96);
    egui_view_message_bar_set_snapshots(EGUI_VIEW_OF(&test_bar), g_snapshots, 4);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&test_bar), on_bar_click);
    click_count = 0;
}

static void layout_bar(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 196;
    region.size.height = 96;
    egui_view_layout(EGUI_VIEW_OF(&test_bar), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_bar)->region_screen, &region);
}

static int send_touch(uint8_t type)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = 48;
    event.location.y = 48;
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

static void test_message_bar_set_snapshots_clamp_and_reset_current(void)
{
    setup_bar();

    egui_view_message_bar_set_snapshots(EGUI_VIEW_OF(&test_bar), g_overflow_snapshots, 7);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_MESSAGE_BAR_MAX_SNAPSHOTS, test_bar.snapshot_count);

    test_bar.current_snapshot = 5;
    egui_view_message_bar_set_snapshots(EGUI_VIEW_OF(&test_bar), g_snapshots, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_bar.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_message_bar_get_current_snapshot(EGUI_VIEW_OF(&test_bar)));

    egui_view_message_bar_set_snapshots(EGUI_VIEW_OF(&test_bar), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_bar.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_message_bar_get_current_snapshot(EGUI_VIEW_OF(&test_bar)));
}

static void test_message_bar_set_current_snapshot_ignores_out_of_range(void)
{
    setup_bar();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_message_bar_get_current_snapshot(EGUI_VIEW_OF(&test_bar)));
    egui_view_message_bar_set_current_snapshot(EGUI_VIEW_OF(&test_bar), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_message_bar_get_current_snapshot(EGUI_VIEW_OF(&test_bar)));

    egui_view_message_bar_set_current_snapshot(EGUI_VIEW_OF(&test_bar), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_message_bar_get_current_snapshot(EGUI_VIEW_OF(&test_bar)));

    egui_view_message_bar_set_snapshots(EGUI_VIEW_OF(&test_bar), NULL, 0);
    egui_view_message_bar_set_current_snapshot(EGUI_VIEW_OF(&test_bar), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_message_bar_get_current_snapshot(EGUI_VIEW_OF(&test_bar)));
}

static void test_message_bar_font_modes_and_palette_update(void)
{
    setup_bar();

    egui_view_message_bar_set_font(EGUI_VIEW_OF(&test_bar), NULL);
    EGUI_TEST_ASSERT_TRUE(test_bar.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_message_bar_set_compact_mode(EGUI_VIEW_OF(&test_bar), 2);
    egui_view_message_bar_set_locked_mode(EGUI_VIEW_OF(&test_bar), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_bar.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_bar.locked_mode);

    egui_view_message_bar_set_compact_mode(EGUI_VIEW_OF(&test_bar), 0);
    egui_view_message_bar_set_locked_mode(EGUI_VIEW_OF(&test_bar), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_bar.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_bar.locked_mode);

    egui_view_message_bar_set_palette(EGUI_VIEW_OF(&test_bar), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                      EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152), EGUI_COLOR_HEX(0x606162), EGUI_COLOR_HEX(0x707172),
                                      EGUI_COLOR_HEX(0x808182), EGUI_COLOR_HEX(0x909192));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_bar.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_bar.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_bar.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_bar.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_bar.accent_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, test_bar.info_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x707172).full, test_bar.success_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full, test_bar.warning_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x909192).full, test_bar.error_color.full);
}

static void test_message_bar_touch_and_key_click_listener(void)
{
    setup_bar();
    layout_bar();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_bar)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_bar)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(1, click_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(2, click_count);

    egui_view_set_enable(EGUI_VIEW_OF(&test_bar), 0);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_EQUAL_INT(2, click_count);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_bar)->is_pressed);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
}

static void test_message_bar_internal_helpers_cover_severity_and_text(void)
{
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);
    egui_color_t mixed = egui_view_message_bar_mix_disabled(sample);

    setup_bar();
    egui_view_message_bar_set_palette(EGUI_VIEW_OF(&test_bar), EGUI_COLOR_HEX(0x111111), EGUI_COLOR_HEX(0x222222), EGUI_COLOR_HEX(0x333333),
                                      EGUI_COLOR_HEX(0x444444), EGUI_COLOR_HEX(0x555555), EGUI_COLOR_HEX(0x666666), EGUI_COLOR_HEX(0x777777),
                                      EGUI_COLOR_HEX(0x888888), EGUI_COLOR_HEX(0x999999));

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_MESSAGE_BAR_MAX_SNAPSHOTS, egui_view_message_bar_clamp_snapshot_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_message_bar_text_len(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_message_bar_text_len("Retry"));
    EGUI_TEST_ASSERT_TRUE(strcmp("i", egui_view_message_bar_severity_glyph(0)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("+", egui_view_message_bar_severity_glyph(1)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("!", egui_view_message_bar_severity_glyph(2)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("x", egui_view_message_bar_severity_glyph(3)) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x666666).full, egui_view_message_bar_severity_color(&test_bar, 0).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x777777).full, egui_view_message_bar_severity_color(&test_bar, 1).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x888888).full, egui_view_message_bar_severity_color(&test_bar, 2).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x999999).full, egui_view_message_bar_severity_color(&test_bar, 3).full);
    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, 65).full, mixed.full);
}

void test_message_bar_run(void)
{
    EGUI_TEST_SUITE_BEGIN(message_bar);
    EGUI_TEST_RUN(test_message_bar_set_snapshots_clamp_and_reset_current);
    EGUI_TEST_RUN(test_message_bar_set_current_snapshot_ignores_out_of_range);
    EGUI_TEST_RUN(test_message_bar_font_modes_and_palette_update);
    EGUI_TEST_RUN(test_message_bar_touch_and_key_click_listener);
    EGUI_TEST_RUN(test_message_bar_internal_helpers_cover_severity_and_text);
    EGUI_TEST_SUITE_END();
}
