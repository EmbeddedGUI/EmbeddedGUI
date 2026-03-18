#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_alert_banner.h"

#include "../../HelloCustomWidgets/feedback/alert_banner/egui_view_alert_banner.h"
#include "../../HelloCustomWidgets/feedback/alert_banner/egui_view_alert_banner.c"

static egui_view_alert_banner_t test_banner;
static int click_count;

static const egui_view_alert_banner_item_t g_items_0[] = {
        {"Cache drift", "OBS", 0, 1},
        {"Auth retry", "WRN", 1, 0},
        {"DB lag peak", "HOT", 2, 0},
        {"CDN warmup", "INF", 0, 1},
};

static const egui_view_alert_banner_item_t g_items_1[] = {
        {"Index backlog", "WRN", 1, 0},
        {"Queue drain", "HOT", 2, 0},
        {"Edge sync ok", "OBS", 0, 1},
        {"Billing recover", "INF", 0, 1},
};

static const egui_view_alert_banner_item_t g_items_2[] = {
        {"Burst", "HOT", 2, 0},
        {"Lag", "WRN", 1, 0},
        {"Ack", "OBS", 0, 1},
        {"Recover", "INF", 0, 1},
};

static const egui_view_alert_banner_snapshot_t g_snapshots[] = {
        {"Queue A", g_items_0, 4, 2},
        {"Queue B", g_items_1, 4, 1},
        {"Queue C", g_items_2, 4, 0},
};

static const egui_view_alert_banner_snapshot_t g_overflow_snapshots[] = {
        {"A", g_items_0, 4, 0},
        {"B", g_items_1, 4, 1},
        {"C", g_items_2, 4, 2},
        {"D", g_items_0, 4, 3},
};

static void on_banner_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    click_count++;
}

static void setup_banner(void)
{
    egui_view_alert_banner_init(EGUI_VIEW_OF(&test_banner));
    egui_view_set_size(EGUI_VIEW_OF(&test_banner), 176, 132);
    egui_view_alert_banner_set_snapshots(EGUI_VIEW_OF(&test_banner), g_snapshots, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&test_banner), on_banner_click);
    click_count = 0;
}

static void layout_banner(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 176;
    region.size.height = 132;
    egui_view_layout(EGUI_VIEW_OF(&test_banner), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_banner)->region_screen, &region);
}

static int send_touch(uint8_t type)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = 64;
    event.location.y = 76;
    return EGUI_VIEW_OF(&test_banner)->api->on_touch_event(EGUI_VIEW_OF(&test_banner), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_banner)->api->on_key_event(EGUI_VIEW_OF(&test_banner), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_banner)->api->on_key_event(EGUI_VIEW_OF(&test_banner), &event);
    return handled;
}

static void test_alert_banner_set_snapshots_clamp_and_reset_current(void)
{
    setup_banner();

    egui_view_alert_banner_set_snapshots(EGUI_VIEW_OF(&test_banner), g_overflow_snapshots, 4);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_ALERT_BANNER_MAX_SNAPSHOTS, test_banner.snapshot_count);

    test_banner.current_snapshot = 5;
    egui_view_alert_banner_set_snapshots(EGUI_VIEW_OF(&test_banner), g_snapshots, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_banner.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_alert_banner_get_current_snapshot(EGUI_VIEW_OF(&test_banner)));

    egui_view_alert_banner_set_snapshots(EGUI_VIEW_OF(&test_banner), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_banner.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_alert_banner_get_current_snapshot(EGUI_VIEW_OF(&test_banner)));
}

static void test_alert_banner_set_current_snapshot_and_focus_update(void)
{
    setup_banner();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_alert_banner_get_current_snapshot(EGUI_VIEW_OF(&test_banner)));
    egui_view_alert_banner_set_current_snapshot(EGUI_VIEW_OF(&test_banner), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_alert_banner_get_current_snapshot(EGUI_VIEW_OF(&test_banner)));

    egui_view_alert_banner_set_current_snapshot(EGUI_VIEW_OF(&test_banner), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_alert_banner_get_current_snapshot(EGUI_VIEW_OF(&test_banner)));

    egui_view_alert_banner_set_focus_item(EGUI_VIEW_OF(&test_banner), 4);
    EGUI_TEST_ASSERT_EQUAL_INT(4, test_banner.focus_item);

    egui_view_alert_banner_set_snapshots(EGUI_VIEW_OF(&test_banner), NULL, 0);
    egui_view_alert_banner_set_current_snapshot(EGUI_VIEW_OF(&test_banner), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_alert_banner_get_current_snapshot(EGUI_VIEW_OF(&test_banner)));
}

static void test_alert_banner_font_modes_and_palette_update(void)
{
    setup_banner();

    egui_view_alert_banner_set_font(EGUI_VIEW_OF(&test_banner), NULL);
    EGUI_TEST_ASSERT_TRUE(test_banner.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_alert_banner_set_show_header(EGUI_VIEW_OF(&test_banner), 2);
    egui_view_alert_banner_set_compact_mode(EGUI_VIEW_OF(&test_banner), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_banner.show_header);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_banner.compact_mode);

    egui_view_alert_banner_set_show_header(EGUI_VIEW_OF(&test_banner), 0);
    egui_view_alert_banner_set_compact_mode(EGUI_VIEW_OF(&test_banner), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_banner.show_header);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_banner.compact_mode);

    egui_view_alert_banner_set_palette(EGUI_VIEW_OF(&test_banner), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                       EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_banner.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_banner.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_banner.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_banner.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_banner.active_color.full);
}

static void test_alert_banner_touch_and_key_click_listener(void)
{
    setup_banner();
    layout_banner();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_banner)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_banner)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(1, click_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(2, click_count);

    egui_view_set_enable(EGUI_VIEW_OF(&test_banner), 0);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_banner)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(2, click_count);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
}

static void test_alert_banner_internal_helpers_cover_clamp_severity_and_mix(void)
{
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);
    egui_color_t mixed = egui_view_alert_banner_mix_disabled(sample);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_ALERT_BANNER_MAX_SNAPSHOTS, egui_view_alert_banner_clamp_snapshot_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_ALERT_BANNER_MAX_ITEMS, egui_view_alert_banner_clamp_item_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x38BDF8).full, egui_view_alert_banner_severity_color(0).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0xF59E0B).full, egui_view_alert_banner_severity_color(1).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0xFB7185).full, egui_view_alert_banner_severity_color(2).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x38BDF8).full, egui_view_alert_banner_severity_color(9).full);
    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_70).full, mixed.full);
}

void test_alert_banner_run(void)
{
    EGUI_TEST_SUITE_BEGIN(alert_banner);
    EGUI_TEST_RUN(test_alert_banner_set_snapshots_clamp_and_reset_current);
    EGUI_TEST_RUN(test_alert_banner_set_current_snapshot_and_focus_update);
    EGUI_TEST_RUN(test_alert_banner_font_modes_and_palette_update);
    EGUI_TEST_RUN(test_alert_banner_touch_and_key_click_listener);
    EGUI_TEST_RUN(test_alert_banner_internal_helpers_cover_clamp_severity_and_mix);
    EGUI_TEST_SUITE_END();
}
