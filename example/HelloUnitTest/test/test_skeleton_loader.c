#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_skeleton_loader.h"

#include "../../HelloCustomWidgets/feedback/skeleton_loader/egui_view_skeleton_loader.h"
#include "../../HelloCustomWidgets/feedback/skeleton_loader/egui_view_skeleton_loader.c"

static egui_view_skeleton_loader_t test_loader;
static int click_count;

static const egui_view_skeleton_loader_block_t g_blocks_a[] = {
        {0, 0, 22, 22, 11},
        {28, 2, 72, 8, 4},
        {28, 14, 56, 7, 4},
        {0, 32, 100, 8, 4},
        {0, 44, 86, 8, 4},
        {0, 62, 136, 34, 6},
        {0, 104, 40, 10, 5},
        {48, 104, 34, 10, 5},
};

static const egui_view_skeleton_loader_block_t g_blocks_b[] = {
        {0, 0, 136, 34, 6},
        {0, 42, 90, 8, 4},
        {0, 54, 110, 8, 4},
        {0, 72, 22, 22, 11},
        {28, 74, 62, 8, 4},
        {28, 86, 48, 7, 4},
        {0, 104, 34, 10, 5},
        {42, 104, 46, 10, 5},
};

static const egui_view_skeleton_loader_snapshot_t g_snapshots[] = {
        {"Layout A", g_blocks_a, 8, 5},
        {"Layout B", g_blocks_b, 8, 0},
};

static const egui_view_skeleton_loader_snapshot_t g_overflow_snapshots[] = {
        {"A", g_blocks_a, 8, 0},
        {"B", g_blocks_b, 8, 1},
        {"C", g_blocks_a, 8, 2},
        {"D", g_blocks_b, 8, 3},
};

static void on_loader_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    click_count++;
}

static void setup_loader(void)
{
    egui_view_skeleton_loader_init(EGUI_VIEW_OF(&test_loader));
    egui_view_set_size(EGUI_VIEW_OF(&test_loader), 176, 132);
    egui_view_skeleton_loader_set_snapshots(EGUI_VIEW_OF(&test_loader), g_snapshots, 2);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&test_loader), on_loader_click);
    click_count = 0;
}

static void layout_loader(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 176;
    region.size.height = 132;
    egui_view_layout(EGUI_VIEW_OF(&test_loader), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_loader)->region_screen, &region);
}

static int send_touch(uint8_t type)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = 64;
    event.location.y = 76;
    return EGUI_VIEW_OF(&test_loader)->api->on_touch_event(EGUI_VIEW_OF(&test_loader), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_loader)->api->on_key_event(EGUI_VIEW_OF(&test_loader), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_loader)->api->on_key_event(EGUI_VIEW_OF(&test_loader), &event);
    return handled;
}

static void test_skeleton_loader_set_snapshots_clamp_and_reset_current(void)
{
    setup_loader();

    egui_view_skeleton_loader_set_snapshots(EGUI_VIEW_OF(&test_loader), g_overflow_snapshots, 4);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SKELETON_LOADER_MAX_SNAPSHOTS, test_loader.snapshot_count);

    test_loader.current_snapshot = 2;
    egui_view_skeleton_loader_set_snapshots(EGUI_VIEW_OF(&test_loader), g_snapshots, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_loader.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_skeleton_loader_get_current_snapshot(EGUI_VIEW_OF(&test_loader)));

    egui_view_skeleton_loader_set_current_snapshot(EGUI_VIEW_OF(&test_loader), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_skeleton_loader_get_current_snapshot(EGUI_VIEW_OF(&test_loader)));

    egui_view_skeleton_loader_set_snapshots(EGUI_VIEW_OF(&test_loader), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_loader.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_skeleton_loader_get_current_snapshot(EGUI_VIEW_OF(&test_loader)));
}

static void test_skeleton_loader_set_current_snapshot_and_focus_update(void)
{
    setup_loader();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_skeleton_loader_get_current_snapshot(EGUI_VIEW_OF(&test_loader)));
    egui_view_skeleton_loader_set_current_snapshot(EGUI_VIEW_OF(&test_loader), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_skeleton_loader_get_current_snapshot(EGUI_VIEW_OF(&test_loader)));

    egui_view_skeleton_loader_set_current_snapshot(EGUI_VIEW_OF(&test_loader), 8);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_skeleton_loader_get_current_snapshot(EGUI_VIEW_OF(&test_loader)));

    egui_view_skeleton_loader_set_focus_block(EGUI_VIEW_OF(&test_loader), 6);
    EGUI_TEST_ASSERT_EQUAL_INT(6, test_loader.focus_block);
}

static void test_skeleton_loader_font_modes_and_palette_update(void)
{
    setup_loader();

    egui_view_skeleton_loader_set_font(EGUI_VIEW_OF(&test_loader), NULL);
    EGUI_TEST_ASSERT_TRUE(test_loader.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_skeleton_loader_set_show_header(EGUI_VIEW_OF(&test_loader), 2);
    egui_view_skeleton_loader_set_compact_mode(EGUI_VIEW_OF(&test_loader), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_loader.show_header);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_loader.compact_mode);

    egui_view_skeleton_loader_set_show_header(EGUI_VIEW_OF(&test_loader), 0);
    egui_view_skeleton_loader_set_compact_mode(EGUI_VIEW_OF(&test_loader), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_loader.show_header);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_loader.compact_mode);

    egui_view_skeleton_loader_set_palette(EGUI_VIEW_OF(&test_loader), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                          EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_loader.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_loader.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_loader.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_loader.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_loader.active_color.full);
}

static void test_skeleton_loader_internal_helpers_cover_clamp_block_and_mix(void)
{
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);
    egui_color_t mixed = egui_view_skeleton_loader_mix_disabled(sample);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SKELETON_LOADER_MAX_SNAPSHOTS, egui_view_skeleton_loader_clamp_snapshot_count(7));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_SKELETON_LOADER_MAX_BLOCKS, egui_view_skeleton_loader_clamp_block_count(13));
    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_70).full, mixed.full);
}

static void test_skeleton_loader_touch_and_key_click_listener(void)
{
    setup_loader();
    layout_loader();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_loader)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_loader)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(1, click_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(2, click_count);

    egui_view_set_enable(EGUI_VIEW_OF(&test_loader), 0);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_loader)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(2, click_count);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
}

void test_skeleton_loader_run(void)
{
    EGUI_TEST_SUITE_BEGIN(skeleton_loader);
    EGUI_TEST_RUN(test_skeleton_loader_set_snapshots_clamp_and_reset_current);
    EGUI_TEST_RUN(test_skeleton_loader_set_current_snapshot_and_focus_update);
    EGUI_TEST_RUN(test_skeleton_loader_font_modes_and_palette_update);
    EGUI_TEST_RUN(test_skeleton_loader_internal_helpers_cover_clamp_block_and_mix);
    EGUI_TEST_RUN(test_skeleton_loader_touch_and_key_click_listener);
    EGUI_TEST_SUITE_END();
}
