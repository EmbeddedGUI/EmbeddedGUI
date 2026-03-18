#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_menu_flyout.h"

#include "../../HelloCustomWidgets/navigation/menu_flyout/egui_view_menu_flyout.h"
#include "../../HelloCustomWidgets/navigation/menu_flyout/egui_view_menu_flyout.c"

static egui_view_menu_flyout_t test_flyout;
static uint8_t click_count;

static const egui_view_menu_flyout_item_t g_items_a[] = {
        {"NW", "New tab", "Ctrl+N", 0, 0, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
        {"OP", "Open recent", "", 0, 1, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_SUBMENU},
        {"DL", "Delete", "", 3, 0, 1, 1, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
};

static const egui_view_menu_flyout_item_t g_items_b[] = {
        {"PN", "Pin", "On", 1, 1, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
        {"LY", "Layout", "", 2, 0, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_SUBMENU},
};

static const egui_view_menu_flyout_item_t g_items_c[] = {
        {"AR", "Archive", "", 0, 0, 1, 0, EGUI_VIEW_MENU_FLYOUT_TRAILING_NONE},
};

static const egui_view_menu_flyout_snapshot_t g_snapshots[] = {
        {g_items_a, 3, 1},
        {g_items_b, 2, 0},
        {g_items_c, 1, 0},
};

static const egui_view_menu_flyout_snapshot_t g_overflow_snapshots[] = {
        {g_items_a, 3, 0}, {g_items_b, 2, 0}, {g_items_c, 1, 0}, {g_items_a, 3, 1}, {g_items_b, 2, 1}, {g_items_c, 1, 0}, {g_items_a, 3, 2},
};

static void on_flyout_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    click_count++;
}

static void setup_flyout(void)
{
    egui_view_menu_flyout_init(EGUI_VIEW_OF(&test_flyout));
    egui_view_set_size(EGUI_VIEW_OF(&test_flyout), 120, 92);
    egui_view_menu_flyout_set_snapshots(EGUI_VIEW_OF(&test_flyout), g_snapshots, 3);
    click_count = 0;
}

static void layout_flyout(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 120;
    region.size.height = 92;
    egui_view_layout(EGUI_VIEW_OF(&test_flyout), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_flyout)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_flyout)->api->on_touch_event(EGUI_VIEW_OF(&test_flyout), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_flyout)->api->on_key_event(EGUI_VIEW_OF(&test_flyout), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_flyout)->api->on_key_event(EGUI_VIEW_OF(&test_flyout), &event);
    return handled;
}

static void test_menu_flyout_set_snapshots_clamps_count_and_resets_current_snapshot(void)
{
    setup_flyout();
    egui_view_menu_flyout_set_snapshots(EGUI_VIEW_OF(&test_flyout), g_overflow_snapshots, 7);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_MENU_FLYOUT_MAX_SNAPSHOTS, test_flyout.snapshot_count);

    test_flyout.current_snapshot = 5;
    egui_view_menu_flyout_set_snapshots(EGUI_VIEW_OF(&test_flyout), g_snapshots, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_flyout.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_menu_flyout_get_current_snapshot(EGUI_VIEW_OF(&test_flyout)));
}

static void test_menu_flyout_set_current_snapshot_ignores_out_of_range(void)
{
    setup_flyout();
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_menu_flyout_get_current_snapshot(EGUI_VIEW_OF(&test_flyout)));

    egui_view_menu_flyout_set_current_snapshot(EGUI_VIEW_OF(&test_flyout), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_menu_flyout_get_current_snapshot(EGUI_VIEW_OF(&test_flyout)));

    egui_view_menu_flyout_set_current_snapshot(EGUI_VIEW_OF(&test_flyout), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_menu_flyout_get_current_snapshot(EGUI_VIEW_OF(&test_flyout)));
}

static void test_menu_flyout_font_modes_and_palette_update(void)
{
    setup_flyout();

    egui_view_menu_flyout_set_font(EGUI_VIEW_OF(&test_flyout), NULL);
    egui_view_menu_flyout_set_meta_font(EGUI_VIEW_OF(&test_flyout), NULL);
    EGUI_TEST_ASSERT_TRUE(test_flyout.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_flyout.meta_font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_menu_flyout_set_compact_mode(EGUI_VIEW_OF(&test_flyout), 1);
    egui_view_menu_flyout_set_disabled_mode(EGUI_VIEW_OF(&test_flyout), 1);
    EGUI_TEST_ASSERT_TRUE(test_flyout.compact_mode);
    EGUI_TEST_ASSERT_TRUE(test_flyout.disabled_mode);

    egui_view_menu_flyout_set_palette(EGUI_VIEW_OF(&test_flyout), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                      EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152), EGUI_COLOR_HEX(0x606162), EGUI_COLOR_HEX(0x707172),
                                      EGUI_COLOR_HEX(0x808182), EGUI_COLOR_HEX(0x909192));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_flyout.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x808182).full, test_flyout.danger_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x909192).full, test_flyout.shadow_color.full);
}

static void test_menu_flyout_touch_and_enter_trigger_click_listener(void)
{
    setup_flyout();
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&test_flyout), on_flyout_click);
    layout_flyout();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 48, 46));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_flyout)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 48, 46));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_flyout)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(1, click_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(2, click_count);
}

static void test_menu_flyout_internal_helpers_clamp_focus_and_meta(void)
{
    egui_view_menu_flyout_snapshot_t snapshot = {g_items_a, 3, 9};

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_MENU_FLYOUT_MAX_SNAPSHOTS, egui_view_menu_flyout_clamp_snapshot_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_MENU_FLYOUT_MAX_ITEMS, egui_view_menu_flyout_clamp_item_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_menu_flyout_text_len(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_menu_flyout_text_len("Ctrl"));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_menu_flyout_focus_index(&snapshot, 3));
    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_menu_flyout_meta_width("Ctrl+Shift+P", 0, 20));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_menu_flyout_meta_width(NULL, 1, 20));
}

void test_menu_flyout_run(void)
{
    EGUI_TEST_SUITE_BEGIN(menu_flyout);
    EGUI_TEST_RUN(test_menu_flyout_set_snapshots_clamps_count_and_resets_current_snapshot);
    EGUI_TEST_RUN(test_menu_flyout_set_current_snapshot_ignores_out_of_range);
    EGUI_TEST_RUN(test_menu_flyout_font_modes_and_palette_update);
    EGUI_TEST_RUN(test_menu_flyout_touch_and_enter_trigger_click_listener);
    EGUI_TEST_RUN(test_menu_flyout_internal_helpers_clamp_focus_and_meta);
    EGUI_TEST_SUITE_END();
}
