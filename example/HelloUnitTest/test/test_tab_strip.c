#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_tab_strip.h"

#include "../../HelloCustomWidgets/navigation/tab_strip/egui_view_tab_strip.h"
#include "../../HelloCustomWidgets/navigation/tab_strip/egui_view_tab_strip.c"

static egui_view_tab_strip_t test_tab_strip;
static uint8_t g_listener_index;
static uint8_t g_listener_count;

static const char *g_tabs_primary[] = {"Overview", "Usage", "Access"};
static const char *g_tabs_overflow[] = {"One", "Two", "Three", "Four", "Five", "Six", "Seven"};
static const char *g_tabs_long[] = {"Documents", "Operations", "Administration"};

static void on_tab_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    g_listener_index = index;
    g_listener_count++;
}

static void setup_tab_strip(void)
{
    egui_view_tab_strip_init(EGUI_VIEW_OF(&test_tab_strip));
    egui_view_set_size(EGUI_VIEW_OF(&test_tab_strip), 196, 48);
    egui_view_tab_strip_set_tabs(EGUI_VIEW_OF(&test_tab_strip), g_tabs_primary, 3);
    egui_view_tab_strip_set_on_tab_changed_listener(EGUI_VIEW_OF(&test_tab_strip), on_tab_changed);
    g_listener_index = 0xFF;
    g_listener_count = 0;
}

static void layout_tab_strip(egui_dim_t width)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = width;
    region.size.height = 48;
    egui_view_layout(EGUI_VIEW_OF(&test_tab_strip), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_tab_strip)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_tab_strip)->api->on_touch_event(EGUI_VIEW_OF(&test_tab_strip), &event);
}

static void get_item_center(uint8_t item_index, egui_dim_t *x, egui_dim_t *y)
{
    egui_region_t work_region;
    egui_view_tab_strip_layout_item_t items[EGUI_VIEW_TAB_STRIP_MAX_TABS];
    egui_dim_t content_x;
    egui_dim_t content_w;
    uint8_t count;

    egui_view_get_work_region(EGUI_VIEW_OF(&test_tab_strip), &work_region);
    content_x = work_region.location.x + EGUI_VIEW_TAB_STRIP_STANDARD_CONTENT_PAD_X;
    content_w = work_region.size.width - EGUI_VIEW_TAB_STRIP_STANDARD_CONTENT_PAD_X * 2;
    count = egui_view_tab_strip_prepare_layout(&test_tab_strip, content_x, content_w, items);

    EGUI_TEST_ASSERT_TRUE(item_index < count);
    *x = items[item_index].x + items[item_index].width / 2;
    *y = work_region.location.y + work_region.size.height / 2;
}

static void test_tab_strip_set_tabs_and_current_index(void)
{
    setup_tab_strip();

    test_tab_strip.current_index = 6;
    egui_view_tab_strip_set_tabs(EGUI_VIEW_OF(&test_tab_strip), g_tabs_overflow, 7);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TAB_STRIP_MAX_TABS, test_tab_strip.tab_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_tab_strip_get_current_index(EGUI_VIEW_OF(&test_tab_strip)));

    egui_view_tab_strip_set_current_index(EGUI_VIEW_OF(&test_tab_strip), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_tab_strip_get_current_index(EGUI_VIEW_OF(&test_tab_strip)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_listener_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_listener_index);

    egui_view_tab_strip_set_current_index(EGUI_VIEW_OF(&test_tab_strip), 2);
    egui_view_tab_strip_set_current_index(EGUI_VIEW_OF(&test_tab_strip), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_listener_count);

    egui_view_tab_strip_set_tabs(EGUI_VIEW_OF(&test_tab_strip), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_tab_strip.tab_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_tab_strip_get_current_index(EGUI_VIEW_OF(&test_tab_strip)));
}

static void test_tab_strip_font_modes_palette_and_helpers(void)
{
    egui_view_tab_strip_layout_item_t items[EGUI_VIEW_TAB_STRIP_MAX_TABS];
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);
    egui_dim_t width_standard;
    egui_dim_t width_compact;
    uint8_t count;

    setup_tab_strip();

    egui_view_tab_strip_set_font(EGUI_VIEW_OF(&test_tab_strip), NULL);
    EGUI_TEST_ASSERT_TRUE(test_tab_strip.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_tab_strip_set_compact_mode(EGUI_VIEW_OF(&test_tab_strip), 2);
    egui_view_tab_strip_set_locked_mode(EGUI_VIEW_OF(&test_tab_strip), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_tab_strip.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_tab_strip.locked_mode);

    egui_view_tab_strip_set_palette(EGUI_VIEW_OF(&test_tab_strip), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                    EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_tab_strip.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_tab_strip.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_tab_strip.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_tab_strip.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_tab_strip.accent_color.full);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TAB_STRIP_MAX_TABS, egui_view_tab_strip_clamp_tab_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_tab_strip_text_len(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_tab_strip_text_len("Usage"));
    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, 64).full, egui_view_tab_strip_mix_disabled(sample).full);

    memset(items, 0, sizeof(items));
    egui_view_tab_strip_copy_elided(items[0].label, sizeof(items[0].label), "Documents", 6);
    EGUI_TEST_ASSERT_TRUE(strcmp("Doc...", items[0].label) == 0);
    egui_view_tab_strip_copy_elided(items[1].label, sizeof(items[1].label), "Usage", 3);
    EGUI_TEST_ASSERT_TRUE(strcmp("...", items[1].label) == 0);

    width_standard = egui_view_tab_strip_measure_tab_width(0, 1, "Overview");
    width_compact = egui_view_tab_strip_measure_tab_width(1, 0, "Overview");
    EGUI_TEST_ASSERT_TRUE(width_standard >= EGUI_VIEW_TAB_STRIP_STANDARD_MIN_WIDTH);
    EGUI_TEST_ASSERT_TRUE(width_compact >= EGUI_VIEW_TAB_STRIP_COMPACT_MIN_WIDTH);
    EGUI_TEST_ASSERT_TRUE(width_standard > width_compact);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TAB_STRIP_COMPACT_GAP, egui_view_tab_strip_item_gap(1));

    egui_view_tab_strip_set_compact_mode(EGUI_VIEW_OF(&test_tab_strip), 0);
    egui_view_tab_strip_set_locked_mode(EGUI_VIEW_OF(&test_tab_strip), 0);
    egui_view_tab_strip_set_tabs(EGUI_VIEW_OF(&test_tab_strip), g_tabs_long, 3);
    count = egui_view_tab_strip_prepare_layout(&test_tab_strip, 0, 72, items);
    EGUI_TEST_ASSERT_EQUAL_INT(3, count);
    EGUI_TEST_ASSERT_TRUE(items[0].width == items[1].width);
    EGUI_TEST_ASSERT_TRUE(strcmp(items[0].label, g_tabs_long[0]) != 0);
    EGUI_TEST_ASSERT_TRUE(strcmp(items[1].label, g_tabs_long[1]) != 0);
    EGUI_TEST_ASSERT_TRUE(items[1].x > items[0].x);
    EGUI_TEST_ASSERT_EQUAL_INT(0xFF, egui_view_tab_strip_resolve_hit(&test_tab_strip, EGUI_VIEW_OF(&test_tab_strip), 4));
}

static void test_tab_strip_touch_selects_and_cancel_resets_pressed_state(void)
{
    egui_dim_t x;
    egui_dim_t y;

    setup_tab_strip();
    layout_tab_strip(196);
    get_item_center(1, &x, &y);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_tab_strip.pressed_index);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_tab_strip)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_tab_strip_get_current_index(EGUI_VIEW_OF(&test_tab_strip)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_listener_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_listener_index);
    EGUI_TEST_ASSERT_EQUAL_INT(0xFF, test_tab_strip.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_tab_strip)->is_pressed);

    get_item_center(2, &x, &y);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_tab_strip.pressed_index);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_CANCEL, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_tab_strip_get_current_index(EGUI_VIEW_OF(&test_tab_strip)));
    EGUI_TEST_ASSERT_EQUAL_INT(0xFF, test_tab_strip.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_tab_strip)->is_pressed);
}

static void test_tab_strip_locked_or_disabled_ignores_touch(void)
{
    egui_dim_t x;
    egui_dim_t y;

    setup_tab_strip();
    layout_tab_strip(196);
    get_item_center(2, &x, &y);

    egui_view_tab_strip_set_locked_mode(EGUI_VIEW_OF(&test_tab_strip), 1);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_tab_strip_get_current_index(EGUI_VIEW_OF(&test_tab_strip)));

    egui_view_tab_strip_set_locked_mode(EGUI_VIEW_OF(&test_tab_strip), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_tab_strip), 0);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_tab_strip_get_current_index(EGUI_VIEW_OF(&test_tab_strip)));
}

void test_tab_strip_run(void)
{
    EGUI_TEST_SUITE_BEGIN(tab_strip);
    EGUI_TEST_RUN(test_tab_strip_set_tabs_and_current_index);
    EGUI_TEST_RUN(test_tab_strip_font_modes_palette_and_helpers);
    EGUI_TEST_RUN(test_tab_strip_touch_selects_and_cancel_resets_pressed_state);
    EGUI_TEST_RUN(test_tab_strip_locked_or_disabled_ignores_touch);
    EGUI_TEST_SUITE_END();
}
