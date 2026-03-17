#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_transport_bar.h"

#include "../../HelloCustomWidgets/media/transport_bar/egui_view_transport_bar.h"
#include "../../HelloCustomWidgets/media/transport_bar/egui_view_transport_bar.c"

static egui_view_transport_bar_t test_transport_bar;

static const egui_view_transport_bar_snapshot_t unit_snapshot = {
        "Live", "Studio mix", "Cue-ready bed with host marker.", "Space toggles pause and resume.", EGUI_COLOR_HEX(0xE8F3FF), EGUI_COLOR_HEX(0x2563EB), 90, 180,
        1};
static const egui_view_transport_bar_snapshot_t overflow_snapshot = {
        "Live", "Overflow clip", "Clamp position to duration.", "Overflow test.", EGUI_COLOR_HEX(0xE8F3FF), EGUI_COLOR_HEX(0x2563EB), 240, 180, 1};

static void setup_transport_bar(void)
{
    egui_view_transport_bar_init(EGUI_VIEW_OF(&test_transport_bar));
    egui_view_set_size(EGUI_VIEW_OF(&test_transport_bar), 164, 96);
    egui_view_transport_bar_set_snapshot(EGUI_VIEW_OF(&test_transport_bar), &unit_snapshot);
}

static void layout_transport_bar(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 164;
    region.size.height = 96;
    egui_view_layout(EGUI_VIEW_OF(&test_transport_bar), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_transport_bar)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_transport_bar)->api->on_touch_event(EGUI_VIEW_OF(&test_transport_bar), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_transport_bar)->api->on_key_event(EGUI_VIEW_OF(&test_transport_bar), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_transport_bar)->api->on_key_event(EGUI_VIEW_OF(&test_transport_bar), &event);
    return handled;
}

static void test_transport_bar_default_state_and_clamp(void)
{
    egui_view_transport_bar_init(EGUI_VIEW_OF(&test_transport_bar));
    egui_view_set_size(EGUI_VIEW_OF(&test_transport_bar), 164, 96);
    egui_view_transport_bar_set_snapshot(EGUI_VIEW_OF(&test_transport_bar), &overflow_snapshot);

    EGUI_TEST_ASSERT_TRUE(egui_view_transport_bar_get_snapshot(EGUI_VIEW_OF(&test_transport_bar)) != NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(180, egui_view_transport_bar_get_position_seconds(EGUI_VIEW_OF(&test_transport_bar)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE, egui_view_transport_bar_get_current_part(EGUI_VIEW_OF(&test_transport_bar)));

    egui_view_transport_bar_set_duration_seconds(EGUI_VIEW_OF(&test_transport_bar), 120);
    EGUI_TEST_ASSERT_EQUAL_INT(120, egui_view_transport_bar_get_position_seconds(EGUI_VIEW_OF(&test_transport_bar)));
}

static void test_transport_bar_tab_cycles_parts(void)
{
    setup_transport_bar();
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TRANSPORT_BAR_PART_NEXT, egui_view_transport_bar_get_current_part(EGUI_VIEW_OF(&test_transport_bar)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TRANSPORT_BAR_PART_SEEK, egui_view_transport_bar_get_current_part(EGUI_VIEW_OF(&test_transport_bar)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TRANSPORT_BAR_PART_PREVIOUS, egui_view_transport_bar_get_current_part(EGUI_VIEW_OF(&test_transport_bar)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE, egui_view_transport_bar_get_current_part(EGUI_VIEW_OF(&test_transport_bar)));
}

static void test_transport_bar_space_toggles_play_pause(void)
{
    setup_transport_bar();
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_transport_bar_get_playing(EGUI_VIEW_OF(&test_transport_bar)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_SPACE));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_transport_bar_get_playing(EGUI_VIEW_OF(&test_transport_bar)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_SPACE));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_transport_bar_get_playing(EGUI_VIEW_OF(&test_transport_bar)));
}

static void test_transport_bar_seek_keys_adjust_position(void)
{
    setup_transport_bar();
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_transport_bar_get_position_seconds(EGUI_VIEW_OF(&test_transport_bar)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(180, egui_view_transport_bar_get_position_seconds(EGUI_VIEW_OF(&test_transport_bar)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_MINUS));
    EGUI_TEST_ASSERT_EQUAL_INT(170, egui_view_transport_bar_get_position_seconds(EGUI_VIEW_OF(&test_transport_bar)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_PLUS));
    EGUI_TEST_ASSERT_EQUAL_INT(180, egui_view_transport_bar_get_position_seconds(EGUI_VIEW_OF(&test_transport_bar)));
}

static void test_transport_bar_touch_buttons_and_seek(void)
{
    egui_region_t region;
    egui_dim_t seek_x;

    setup_transport_bar();
    layout_transport_bar();

    EGUI_TEST_ASSERT_TRUE(egui_view_transport_bar_get_part_region(EGUI_VIEW_OF(&test_transport_bar), EGUI_VIEW_TRANSPORT_BAR_PART_PREVIOUS, &region));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(60, egui_view_transport_bar_get_position_seconds(EGUI_VIEW_OF(&test_transport_bar)));

    EGUI_TEST_ASSERT_TRUE(egui_view_transport_bar_get_part_region(EGUI_VIEW_OF(&test_transport_bar), EGUI_VIEW_TRANSPORT_BAR_PART_NEXT, &region));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(90, egui_view_transport_bar_get_position_seconds(EGUI_VIEW_OF(&test_transport_bar)));

    EGUI_TEST_ASSERT_TRUE(egui_view_transport_bar_get_part_region(EGUI_VIEW_OF(&test_transport_bar), EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE, &region));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_transport_bar_get_playing(EGUI_VIEW_OF(&test_transport_bar)));

    EGUI_TEST_ASSERT_TRUE(egui_view_transport_bar_get_part_region(EGUI_VIEW_OF(&test_transport_bar), EGUI_VIEW_TRANSPORT_BAR_PART_SEEK, &region));
    seek_x = region.location.x + (region.size.width * 3) / 4;
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, seek_x, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, seek_x, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(egui_view_transport_bar_get_position_seconds(EGUI_VIEW_OF(&test_transport_bar)) >= 130);
}

static void test_transport_bar_part_region_exposes_all_parts(void)
{
    egui_region_t region;

    setup_transport_bar();
    layout_transport_bar();
    EGUI_TEST_ASSERT_TRUE(egui_view_transport_bar_get_part_region(EGUI_VIEW_OF(&test_transport_bar), EGUI_VIEW_TRANSPORT_BAR_PART_PREVIOUS, &region));
    EGUI_TEST_ASSERT_TRUE(region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(egui_view_transport_bar_get_part_region(EGUI_VIEW_OF(&test_transport_bar), EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE, &region));
    EGUI_TEST_ASSERT_TRUE(egui_view_transport_bar_get_part_region(EGUI_VIEW_OF(&test_transport_bar), EGUI_VIEW_TRANSPORT_BAR_PART_NEXT, &region));
    EGUI_TEST_ASSERT_TRUE(egui_view_transport_bar_get_part_region(EGUI_VIEW_OF(&test_transport_bar), EGUI_VIEW_TRANSPORT_BAR_PART_SEEK, &region));
}

static void test_transport_bar_compact_and_read_only_ignore_input(void)
{
    egui_region_t region;

    setup_transport_bar();
    egui_view_transport_bar_set_compact_mode(EGUI_VIEW_OF(&test_transport_bar), 1);
    layout_transport_bar();
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_SPACE));
    EGUI_TEST_ASSERT_TRUE(egui_view_transport_bar_get_part_region(EGUI_VIEW_OF(&test_transport_bar), EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE, &region));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));

    setup_transport_bar();
    egui_view_transport_bar_set_read_only_mode(EGUI_VIEW_OF(&test_transport_bar), 1);
    layout_transport_bar();
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_SPACE));
    EGUI_TEST_ASSERT_TRUE(egui_view_transport_bar_get_part_region(EGUI_VIEW_OF(&test_transport_bar), EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE, &region));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
}

void test_transport_bar_run(void)
{
    EGUI_TEST_SUITE_BEGIN(transport_bar);
    EGUI_TEST_RUN(test_transport_bar_default_state_and_clamp);
    EGUI_TEST_RUN(test_transport_bar_tab_cycles_parts);
    EGUI_TEST_RUN(test_transport_bar_space_toggles_play_pause);
    EGUI_TEST_RUN(test_transport_bar_seek_keys_adjust_position);
    EGUI_TEST_RUN(test_transport_bar_touch_buttons_and_seek);
    EGUI_TEST_RUN(test_transport_bar_part_region_exposes_all_parts);
    EGUI_TEST_RUN(test_transport_bar_compact_and_read_only_ignore_input);
    EGUI_TEST_SUITE_END();
}
