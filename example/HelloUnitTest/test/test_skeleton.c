#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_skeleton.h"

#include "../../HelloCustomWidgets/feedback/skeleton/egui_view_skeleton.h"
#include "../../HelloCustomWidgets/feedback/skeleton/egui_view_skeleton.c"

static egui_view_skeleton_t test_skeleton;
static int click_count;

static const egui_view_skeleton_block_t g_blocks_a[] = {
        {0, 0, 24, 8, 4},
        {0, 12, 56, 7, 3},
        {0, 24, 72, 18, 7},
        {0, 48, 48, 6, 3},
};

static const egui_view_skeleton_block_t g_blocks_b[] = {
        {0, 0, 18, 18, 9}, {24, 2, 72, 7, 3}, {24, 14, 48, 6, 3}, {0, 32, 92, 14, 5}, {0, 52, 64, 6, 3},
};

static const egui_view_skeleton_snapshot_t g_snapshots[] = {
        {"Article", "Loading article", g_blocks_a, 4, 2},
        {"Feed", "Loading feed", g_blocks_b, 5, 3},
};

static const egui_view_skeleton_snapshot_t g_overflow_snapshots[] = {
        {"A", "A", g_blocks_a, 4, 0}, {"B", "B", g_blocks_b, 5, 1}, {"C", "C", g_blocks_a, 4, 2}, {"D", "D", g_blocks_b, 5, 3}, {"E", "E", g_blocks_a, 4, 0},
};

static void on_skeleton_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    click_count++;
}

static void setup_skeleton(void)
{
    egui_view_skeleton_init(EGUI_VIEW_OF(&test_skeleton));
    egui_view_set_size(EGUI_VIEW_OF(&test_skeleton), 196, 96);
    egui_view_skeleton_set_snapshots(EGUI_VIEW_OF(&test_skeleton), g_snapshots, 2);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&test_skeleton), on_skeleton_click);
    click_count = 0;
}

static void layout_skeleton(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 196;
    region.size.height = 96;
    egui_view_layout(EGUI_VIEW_OF(&test_skeleton), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_skeleton)->region_screen, &region);
}

static int send_touch(uint8_t type)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = 44;
    event.location.y = 46;
    return EGUI_VIEW_OF(&test_skeleton)->api->on_touch_event(EGUI_VIEW_OF(&test_skeleton), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_skeleton)->api->on_key_event(EGUI_VIEW_OF(&test_skeleton), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_skeleton)->api->on_key_event(EGUI_VIEW_OF(&test_skeleton), &event);
    return handled;
}

static void stop_timer_if_started(void)
{
    if (test_skeleton.timer_started)
    {
        egui_view_dispatch_detach_from_window(EGUI_VIEW_OF(&test_skeleton));
    }
}

static void test_skeleton_set_snapshots_clamp_and_reset_current(void)
{
    setup_skeleton();

    egui_view_skeleton_set_snapshots(EGUI_VIEW_OF(&test_skeleton), g_overflow_snapshots, 5);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SKELETON_MAX_SNAPSHOTS, test_skeleton.snapshot_count);

    test_skeleton.current_snapshot = 3;
    egui_view_skeleton_set_snapshots(EGUI_VIEW_OF(&test_skeleton), g_snapshots, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_skeleton.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_skeleton_get_current_snapshot(EGUI_VIEW_OF(&test_skeleton)));

    egui_view_skeleton_set_current_snapshot(EGUI_VIEW_OF(&test_skeleton), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_skeleton_get_current_snapshot(EGUI_VIEW_OF(&test_skeleton)));

    egui_view_skeleton_set_snapshots(EGUI_VIEW_OF(&test_skeleton), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_skeleton.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_skeleton_get_current_snapshot(EGUI_VIEW_OF(&test_skeleton)));
}

static void test_skeleton_set_current_snapshot_and_emphasis(void)
{
    setup_skeleton();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_skeleton_get_current_snapshot(EGUI_VIEW_OF(&test_skeleton)));
    egui_view_skeleton_set_current_snapshot(EGUI_VIEW_OF(&test_skeleton), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_skeleton_get_current_snapshot(EGUI_VIEW_OF(&test_skeleton)));

    egui_view_skeleton_set_emphasis_block(EGUI_VIEW_OF(&test_skeleton), 6);
    EGUI_TEST_ASSERT_EQUAL_INT(6, test_skeleton.emphasis_block);
}

static void test_skeleton_font_footer_modes_palette_and_animation(void)
{
    setup_skeleton();

    egui_view_skeleton_set_font(EGUI_VIEW_OF(&test_skeleton), NULL);
    EGUI_TEST_ASSERT_TRUE(test_skeleton.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_skeleton_set_show_footer(EGUI_VIEW_OF(&test_skeleton), 2);
    egui_view_skeleton_set_compact_mode(EGUI_VIEW_OF(&test_skeleton), 3);
    egui_view_skeleton_set_locked_mode(EGUI_VIEW_OF(&test_skeleton), 4);
    egui_view_skeleton_set_animation_mode(EGUI_VIEW_OF(&test_skeleton), 9);

    EGUI_TEST_ASSERT_EQUAL_INT(1, test_skeleton.show_footer);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_skeleton.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_skeleton.locked_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SKELETON_ANIM_PULSE, test_skeleton.animation_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_skeleton.timer_started);

    egui_view_skeleton_set_palette(EGUI_VIEW_OF(&test_skeleton), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                   EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152), EGUI_COLOR_HEX(0x606162));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_skeleton.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_skeleton.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_skeleton.block_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_skeleton.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_skeleton.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x606162).full, test_skeleton.accent_color.full);

    egui_view_skeleton_set_locked_mode(EGUI_VIEW_OF(&test_skeleton), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_skeleton.timer_started);

    egui_view_skeleton_set_animation_mode(EGUI_VIEW_OF(&test_skeleton), EGUI_VIEW_SKELETON_ANIM_NONE);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SKELETON_ANIM_NONE, test_skeleton.animation_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_skeleton.timer_started);

    egui_view_skeleton_set_show_footer(EGUI_VIEW_OF(&test_skeleton), 0);
    egui_view_skeleton_set_compact_mode(EGUI_VIEW_OF(&test_skeleton), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_skeleton.show_footer);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_skeleton.compact_mode);
}

static void test_skeleton_attach_detach_and_helper_functions(void)
{
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);

    setup_skeleton();

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SKELETON_MAX_SNAPSHOTS, egui_view_skeleton_clamp_snapshot_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SKELETON_MAX_BLOCKS, egui_view_skeleton_clamp_block_count(20));
    EGUI_TEST_ASSERT_EQUAL_INT(12, egui_view_skeleton_get_pulse_mix(0));
    EGUI_TEST_ASSERT_EQUAL_INT(42, egui_view_skeleton_get_pulse_mix(6));
    EGUI_TEST_ASSERT_EQUAL_INT(27, egui_view_skeleton_get_pulse_mix(9));
    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, 62).full, egui_view_skeleton_mix_disabled(sample).full);

    egui_view_skeleton_set_animation_mode(EGUI_VIEW_OF(&test_skeleton), EGUI_VIEW_SKELETON_ANIM_WAVE);
    egui_view_dispatch_attach_to_window(EGUI_VIEW_OF(&test_skeleton));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_skeleton.timer_started);

    test_skeleton.anim_phase = 23;
    egui_view_skeleton_tick(&test_skeleton.anim_timer);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_skeleton.anim_phase);

    egui_view_dispatch_detach_from_window(EGUI_VIEW_OF(&test_skeleton));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_skeleton.timer_started);
}

static void test_skeleton_touch_and_key_click_listener(void)
{
    setup_skeleton();
    layout_skeleton();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_skeleton)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_skeleton)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(1, click_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(2, click_count);

    egui_view_set_enable(EGUI_VIEW_OF(&test_skeleton), 0);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_skeleton)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(2, click_count);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));

    stop_timer_if_started();
}

void test_skeleton_run(void)
{
    EGUI_TEST_SUITE_BEGIN(skeleton);
    EGUI_TEST_RUN(test_skeleton_set_snapshots_clamp_and_reset_current);
    EGUI_TEST_RUN(test_skeleton_set_current_snapshot_and_emphasis);
    EGUI_TEST_RUN(test_skeleton_font_footer_modes_palette_and_animation);
    EGUI_TEST_RUN(test_skeleton_attach_detach_and_helper_functions);
    EGUI_TEST_RUN(test_skeleton_touch_and_key_click_listener);
    EGUI_TEST_SUITE_END();
}
