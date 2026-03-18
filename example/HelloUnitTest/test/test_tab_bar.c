#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_tab_bar.h"

static egui_view_tab_bar_t test_tab_bar;
static uint8_t g_tab_changed_count;
static uint8_t g_last_tab_index;

static const char *g_tab_texts[] = {"Home", "Logs", "Tools"};

static void on_tab_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    g_tab_changed_count++;
    g_last_tab_index = index;
}

static void setup_tab_bar(void)
{
    egui_view_tab_bar_init(EGUI_VIEW_OF(&test_tab_bar));
    egui_view_set_size(EGUI_VIEW_OF(&test_tab_bar), 180, 40);
    egui_view_tab_bar_set_tabs(EGUI_VIEW_OF(&test_tab_bar), g_tab_texts, 3);
    egui_view_tab_bar_set_on_tab_changed_listener(EGUI_VIEW_OF(&test_tab_bar), on_tab_changed);
    g_tab_changed_count = 0;
    g_last_tab_index = EGUI_VIEW_TAB_BAR_PRESSED_NONE;
}

static void layout_tab_bar(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 180;
    region.size.height = 40;
    egui_view_layout(EGUI_VIEW_OF(&test_tab_bar), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_tab_bar)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_tab_bar)->api->on_touch_event(EGUI_VIEW_OF(&test_tab_bar), &event);
}

static void get_tab_center(uint8_t index, egui_dim_t *x, egui_dim_t *y)
{
    egui_dim_t tab_w = EGUI_VIEW_OF(&test_tab_bar)->region.size.width / test_tab_bar.tab_count;

    *x = EGUI_VIEW_OF(&test_tab_bar)->region_screen.location.x + tab_w * index + tab_w / 2;
    *y = EGUI_VIEW_OF(&test_tab_bar)->region_screen.location.y + EGUI_VIEW_OF(&test_tab_bar)->region.size.height / 2;
}

static void test_tab_bar_release_requires_same_tab(void)
{
    egui_dim_t x1 = 0;
    egui_dim_t y1 = 0;
    egui_dim_t x2 = 0;
    egui_dim_t y2 = 0;

    setup_tab_bar();
    layout_tab_bar();
    get_tab_center(1, &x1, &y1);
    get_tab_center(2, &x2, &y2);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x2, y2));
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_tab_bar.pressed_index);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_tab_bar)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x1, y1));
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_tab_bar.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_tab_bar)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x1, y1));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_tab_bar.current_index);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_tab_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TAB_BAR_PRESSED_NONE, test_tab_bar.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_tab_bar)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x2, y2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x1, y1));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x2, y2));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_tab_bar)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x2, y2));
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_tab_bar.current_index);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_tab_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_last_tab_index);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TAB_BAR_PRESSED_NONE, test_tab_bar.pressed_index);
}

void test_tab_bar_run(void)
{
    EGUI_TEST_SUITE_BEGIN(tab_bar);
    EGUI_TEST_RUN(test_tab_bar_release_requires_same_tab);
    EGUI_TEST_SUITE_END();
}
