#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_parallax_view.h"

#include "../../HelloCustomWidgets/layout/parallax_view/egui_view_parallax_view.h"
#include "../../HelloCustomWidgets/layout/parallax_view/egui_view_parallax_view.c"

static egui_view_parallax_view_t test_parallax_view;
static egui_dim_t last_offset;
static uint8_t last_active_row;
static uint8_t changed_count;

static const egui_view_parallax_view_row_t test_rows[] = {
        {"Hero Banner", "Top", 0, EGUI_VIEW_PARALLAX_VIEW_TONE_ACCENT},
        {"Pinned Deck", "Mid", 180, EGUI_VIEW_PARALLAX_VIEW_TONE_SUCCESS},
        {"Quiet Layer", "Hold", 360, EGUI_VIEW_PARALLAX_VIEW_TONE_NEUTRAL},
        {"System Cards", "Tail", 560, EGUI_VIEW_PARALLAX_VIEW_TONE_WARNING},
};

static void on_parallax_changed(egui_view_t *self, egui_dim_t offset, uint8_t active_row)
{
    EGUI_UNUSED(self);
    changed_count++;
    last_offset = offset;
    last_active_row = active_row;
}

static void reset_listener_state(void)
{
    last_offset = 0;
    last_active_row = EGUI_VIEW_PARALLAX_VIEW_INDEX_NONE;
    changed_count = 0;
}

static void setup_parallax_view(void)
{
    egui_view_parallax_view_init(EGUI_VIEW_OF(&test_parallax_view));
    egui_view_set_size(EGUI_VIEW_OF(&test_parallax_view), 150, 100);
    egui_view_parallax_view_set_rows(EGUI_VIEW_OF(&test_parallax_view), test_rows, 4);
    egui_view_parallax_view_set_content_metrics(EGUI_VIEW_OF(&test_parallax_view), 720, 160);
    egui_view_parallax_view_set_step_size(EGUI_VIEW_OF(&test_parallax_view), 60, 180);
    egui_view_parallax_view_set_vertical_shift(EGUI_VIEW_OF(&test_parallax_view), 18);
    egui_view_parallax_view_set_on_changed_listener(EGUI_VIEW_OF(&test_parallax_view), on_parallax_changed);
    reset_listener_state();
}

static void layout_parallax_view(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 150;
    region.size.height = 100;
    egui_view_layout(EGUI_VIEW_OF(&test_parallax_view), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_parallax_view)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_parallax_view)->api->on_touch_event(EGUI_VIEW_OF(&test_parallax_view), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_parallax_view)->api->on_key_event(EGUI_VIEW_OF(&test_parallax_view), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_parallax_view)->api->on_key_event(EGUI_VIEW_OF(&test_parallax_view), &event);
    return handled;
}

static void test_parallax_view_clamps_metrics_and_offset(void)
{
    setup_parallax_view();

    egui_view_parallax_view_set_content_metrics(EGUI_VIEW_OF(&test_parallax_view), 300, 400);
    EGUI_TEST_ASSERT_EQUAL_INT(300, egui_view_parallax_view_get_content_length(EGUI_VIEW_OF(&test_parallax_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(400, egui_view_parallax_view_get_viewport_length(EGUI_VIEW_OF(&test_parallax_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_parallax_view_get_max_offset(EGUI_VIEW_OF(&test_parallax_view)));
    egui_view_parallax_view_set_offset(EGUI_VIEW_OF(&test_parallax_view), 80);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_parallax_view_get_offset(EGUI_VIEW_OF(&test_parallax_view)));

    reset_listener_state();
    egui_view_parallax_view_set_content_metrics(EGUI_VIEW_OF(&test_parallax_view), 720, 160);
    egui_view_parallax_view_set_offset(EGUI_VIEW_OF(&test_parallax_view), 999);
    EGUI_TEST_ASSERT_EQUAL_INT(560, egui_view_parallax_view_get_offset(EGUI_VIEW_OF(&test_parallax_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(560, last_offset);
    EGUI_TEST_ASSERT_EQUAL_INT(3, last_active_row);
}

static void test_parallax_view_active_row_tracks_offset(void)
{
    setup_parallax_view();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_parallax_view_get_active_row(EGUI_VIEW_OF(&test_parallax_view)));
    egui_view_parallax_view_set_offset(EGUI_VIEW_OF(&test_parallax_view), 200);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_parallax_view_get_active_row(EGUI_VIEW_OF(&test_parallax_view)));
    egui_view_parallax_view_set_offset(EGUI_VIEW_OF(&test_parallax_view), 400);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_parallax_view_get_active_row(EGUI_VIEW_OF(&test_parallax_view)));
    egui_view_parallax_view_set_offset(EGUI_VIEW_OF(&test_parallax_view), 560);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_parallax_view_get_active_row(EGUI_VIEW_OF(&test_parallax_view)));
}

static void test_parallax_view_keyboard_navigation(void)
{
    setup_parallax_view();

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(60, egui_view_parallax_view_get_offset(EGUI_VIEW_OF(&test_parallax_view)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_UP));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_parallax_view_get_offset(EGUI_VIEW_OF(&test_parallax_view)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_PLUS));
    EGUI_TEST_ASSERT_EQUAL_INT(180, egui_view_parallax_view_get_offset(EGUI_VIEW_OF(&test_parallax_view)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_MINUS));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_parallax_view_get_offset(EGUI_VIEW_OF(&test_parallax_view)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(560, egui_view_parallax_view_get_offset(EGUI_VIEW_OF(&test_parallax_view)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_parallax_view_get_offset(EGUI_VIEW_OF(&test_parallax_view)));
}

static void test_parallax_view_touch_selects_anchor(void)
{
    egui_region_t region;

    setup_parallax_view();
    layout_parallax_view();
    EGUI_TEST_ASSERT_TRUE(egui_view_parallax_view_get_row_region(EGUI_VIEW_OF(&test_parallax_view), 2, &region));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(360, egui_view_parallax_view_get_offset(EGUI_VIEW_OF(&test_parallax_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_parallax_view_get_active_row(EGUI_VIEW_OF(&test_parallax_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(360, last_offset);
}

static void test_parallax_view_compact_and_locked_ignore_input(void)
{
    egui_region_t region;

    setup_parallax_view();
    egui_view_parallax_view_set_compact_mode(EGUI_VIEW_OF(&test_parallax_view), 1);
    layout_parallax_view();
    EGUI_TEST_ASSERT_TRUE(egui_view_parallax_view_get_row_region(EGUI_VIEW_OF(&test_parallax_view), 1, &region));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_parallax_view_get_offset(EGUI_VIEW_OF(&test_parallax_view)));

    setup_parallax_view();
    egui_view_parallax_view_set_locked_mode(EGUI_VIEW_OF(&test_parallax_view), 1);
    layout_parallax_view();
    EGUI_TEST_ASSERT_TRUE(egui_view_parallax_view_get_row_region(EGUI_VIEW_OF(&test_parallax_view), 1, &region));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_PLUS));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_parallax_view_get_offset(EGUI_VIEW_OF(&test_parallax_view)));
}

static void test_parallax_view_row_region_is_exposed(void)
{
    egui_region_t region;

    setup_parallax_view();
    layout_parallax_view();
    EGUI_TEST_ASSERT_TRUE(egui_view_parallax_view_get_row_region(EGUI_VIEW_OF(&test_parallax_view), 0, &region));
    EGUI_TEST_ASSERT_TRUE(region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(region.size.height > 0);
    EGUI_TEST_ASSERT_FALSE(egui_view_parallax_view_get_row_region(EGUI_VIEW_OF(&test_parallax_view), 4, &region));
}

void test_parallax_view_run(void)
{
    EGUI_TEST_SUITE_BEGIN(parallax_view);
    EGUI_TEST_RUN(test_parallax_view_clamps_metrics_and_offset);
    EGUI_TEST_RUN(test_parallax_view_active_row_tracks_offset);
    EGUI_TEST_RUN(test_parallax_view_keyboard_navigation);
    EGUI_TEST_RUN(test_parallax_view_touch_selects_anchor);
    EGUI_TEST_RUN(test_parallax_view_compact_and_locked_ignore_input);
    EGUI_TEST_RUN(test_parallax_view_row_region_is_exposed);
    EGUI_TEST_SUITE_END();
}
