#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_toast_stack.h"

#include "../../HelloCustomWidgets/feedback/toast_stack/egui_view_toast_stack.h"
#include "../../HelloCustomWidgets/feedback/toast_stack/egui_view_toast_stack.c"

static egui_view_toast_stack_t test_stack;
static int click_count;

static const egui_view_toast_stack_snapshot_t g_snapshots[] = {
        {"Backup ready", "Open the latest note.", "Open", "Now", "Shift starts", "Daily sync", 0, 1},
        {"Draft", "Team can review the build.", "Review", "1 min", "Queue", "Pinned", 1, 1},
        {"Storage low", "Archive logs before sync.", "Manage", "4 min", "Sync waiting", "Quota alert", 2, 1},
        {"Upload failed", "Reconnect to send report.", "Retry", "Offline", "Auth expired", "Queue paused", 3, 0},
};

static const egui_view_toast_stack_snapshot_t g_overflow_snapshots[] = {
        {"A", "A", "A", "A", "A", "A", 0, 1}, {"B", "B", "B", "B", "B", "B", 1, 1}, {"C", "C", "C", "C", "C", "C", 2, 1},
        {"D", "D", "D", "D", "D", "D", 3, 0}, {"E", "E", "E", "E", "E", "E", 0, 0}, {"F", "F", "F", "F", "F", "F", 1, 0},
};

static void on_stack_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    click_count++;
}

static void setup_stack(void)
{
    egui_view_toast_stack_init(EGUI_VIEW_OF(&test_stack));
    egui_view_set_size(EGUI_VIEW_OF(&test_stack), 196, 108);
    egui_view_toast_stack_set_snapshots(EGUI_VIEW_OF(&test_stack), g_snapshots, 4);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&test_stack), on_stack_click);
    click_count = 0;
}

static void layout_stack(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 196;
    region.size.height = 108;
    egui_view_layout(EGUI_VIEW_OF(&test_stack), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_stack)->region_screen, &region);
}

static int send_touch(uint8_t type)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = 44;
    event.location.y = 48;
    return EGUI_VIEW_OF(&test_stack)->api->on_touch_event(EGUI_VIEW_OF(&test_stack), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_stack)->api->on_key_event(EGUI_VIEW_OF(&test_stack), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_stack)->api->on_key_event(EGUI_VIEW_OF(&test_stack), &event);
    return handled;
}

static void test_toast_stack_set_snapshots_clamp_and_reset_current(void)
{
    setup_stack();

    egui_view_toast_stack_set_snapshots(EGUI_VIEW_OF(&test_stack), g_overflow_snapshots, 6);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOAST_STACK_MAX_SNAPSHOTS, test_stack.snapshot_count);

    test_stack.current_snapshot = 3;
    egui_view_toast_stack_set_snapshots(EGUI_VIEW_OF(&test_stack), g_snapshots, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_stack.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_toast_stack_get_current_snapshot(EGUI_VIEW_OF(&test_stack)));

    egui_view_toast_stack_set_snapshots(EGUI_VIEW_OF(&test_stack), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_stack.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_toast_stack_get_current_snapshot(EGUI_VIEW_OF(&test_stack)));
}

static void test_toast_stack_set_current_snapshot_ignores_out_of_range(void)
{
    setup_stack();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_toast_stack_get_current_snapshot(EGUI_VIEW_OF(&test_stack)));
    egui_view_toast_stack_set_current_snapshot(EGUI_VIEW_OF(&test_stack), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_toast_stack_get_current_snapshot(EGUI_VIEW_OF(&test_stack)));

    egui_view_toast_stack_set_current_snapshot(EGUI_VIEW_OF(&test_stack), 7);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_toast_stack_get_current_snapshot(EGUI_VIEW_OF(&test_stack)));

    egui_view_toast_stack_set_snapshots(EGUI_VIEW_OF(&test_stack), NULL, 0);
    egui_view_toast_stack_set_current_snapshot(EGUI_VIEW_OF(&test_stack), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_toast_stack_get_current_snapshot(EGUI_VIEW_OF(&test_stack)));
}

static void test_toast_stack_font_modes_and_palette_update(void)
{
    setup_stack();

    egui_view_toast_stack_set_font(EGUI_VIEW_OF(&test_stack), NULL);
    egui_view_toast_stack_set_meta_font(EGUI_VIEW_OF(&test_stack), NULL);
    EGUI_TEST_ASSERT_TRUE(test_stack.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_stack.meta_font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_toast_stack_set_compact_mode(EGUI_VIEW_OF(&test_stack), 2);
    egui_view_toast_stack_set_locked_mode(EGUI_VIEW_OF(&test_stack), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_stack.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_stack.locked_mode);

    egui_view_toast_stack_set_compact_mode(EGUI_VIEW_OF(&test_stack), 0);
    egui_view_toast_stack_set_locked_mode(EGUI_VIEW_OF(&test_stack), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_stack.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_stack.locked_mode);

    egui_view_toast_stack_set_palette(EGUI_VIEW_OF(&test_stack), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                      EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152), EGUI_COLOR_HEX(0x606162), EGUI_COLOR_HEX(0x707172),
                                      EGUI_COLOR_HEX(0x808182), EGUI_COLOR_HEX(0x909192));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_stack.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_stack.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_stack.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_stack.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_stack.accent_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, test_stack.info_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x707172).full, test_stack.success_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full, test_stack.warning_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x909192).full, test_stack.error_color.full);
}

static void test_toast_stack_touch_and_key_click_listener(void)
{
    setup_stack();
    layout_stack();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_stack)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_stack)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(1, click_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(2, click_count);

    egui_view_set_enable(EGUI_VIEW_OF(&test_stack), 0);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_stack)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(2, click_count);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
}

static void test_toast_stack_internal_helpers_cover_severity_and_text(void)
{
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);
    egui_color_t mixed = egui_view_toast_stack_mix_disabled(sample);

    setup_stack();
    egui_view_toast_stack_set_palette(EGUI_VIEW_OF(&test_stack), EGUI_COLOR_HEX(0x111111), EGUI_COLOR_HEX(0x222222), EGUI_COLOR_HEX(0x333333),
                                      EGUI_COLOR_HEX(0x444444), EGUI_COLOR_HEX(0x555555), EGUI_COLOR_HEX(0x666666), EGUI_COLOR_HEX(0x777777),
                                      EGUI_COLOR_HEX(0x888888), EGUI_COLOR_HEX(0x999999));

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOAST_STACK_MAX_SNAPSHOTS, egui_view_toast_stack_clamp_snapshot_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_toast_stack_text_len(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_toast_stack_text_len("Retry"));
    EGUI_TEST_ASSERT_TRUE(strcmp("i", egui_view_toast_stack_severity_glyph(0)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("+", egui_view_toast_stack_severity_glyph(1)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("!", egui_view_toast_stack_severity_glyph(2)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("x", egui_view_toast_stack_severity_glyph(3)) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x666666).full, egui_view_toast_stack_severity_color(&test_stack, 0).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x777777).full, egui_view_toast_stack_severity_color(&test_stack, 1).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x888888).full, egui_view_toast_stack_severity_color(&test_stack, 2).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x999999).full, egui_view_toast_stack_severity_color(&test_stack, 3).full);
    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, 68).full, mixed.full);
}

void test_toast_stack_run(void)
{
    EGUI_TEST_SUITE_BEGIN(toast_stack);
    EGUI_TEST_RUN(test_toast_stack_set_snapshots_clamp_and_reset_current);
    EGUI_TEST_RUN(test_toast_stack_set_current_snapshot_ignores_out_of_range);
    EGUI_TEST_RUN(test_toast_stack_font_modes_and_palette_update);
    EGUI_TEST_RUN(test_toast_stack_touch_and_key_click_listener);
    EGUI_TEST_RUN(test_toast_stack_internal_helpers_cover_severity_and_text);
    EGUI_TEST_SUITE_END();
}
