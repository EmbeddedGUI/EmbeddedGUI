#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_menu.h"

#define TEST_MENU_PRESSED_NONE (-1)
#define TEST_MENU_PRESSED_BACK (-2)

static egui_view_menu_t test_menu;
static uint8_t g_click_count;
static uint8_t g_last_page_index;
static uint8_t g_last_item_index;

static const egui_view_menu_item_t g_root_items[] = {
    {"Settings", 1, NULL},
    {"About", EGUI_VIEW_MENU_ITEM_LEAF, NULL},
};

static const egui_view_menu_item_t g_settings_items[] = {
    {"Display", EGUI_VIEW_MENU_ITEM_LEAF, NULL},
};

static const egui_view_menu_page_t g_pages[] = {
    {"Main", g_root_items, 2},
    {"Settings", g_settings_items, 1},
};

static void on_item_click(egui_view_t *self, uint8_t page_index, uint8_t item_index)
{
    EGUI_UNUSED(self);
    g_click_count++;
    g_last_page_index = page_index;
    g_last_item_index = item_index;
}

static void setup_menu(void)
{
    egui_view_menu_init(EGUI_VIEW_OF(&test_menu));
    egui_view_set_size(EGUI_VIEW_OF(&test_menu), 180, 150);
    egui_view_menu_set_pages(EGUI_VIEW_OF(&test_menu), g_pages, 2);
    egui_view_menu_set_header_height(EGUI_VIEW_OF(&test_menu), 30);
    egui_view_menu_set_item_height(EGUI_VIEW_OF(&test_menu), 30);
    egui_view_menu_set_on_item_click(EGUI_VIEW_OF(&test_menu), on_item_click);
    g_click_count = 0;
    g_last_page_index = 0xFF;
    g_last_item_index = 0xFF;
}

static void layout_menu(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 180;
    region.size.height = 150;
    egui_view_layout(EGUI_VIEW_OF(&test_menu), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_menu)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_menu)->api->on_touch_event(EGUI_VIEW_OF(&test_menu), &event);
}

static void get_item_center(uint8_t index, egui_dim_t *x, egui_dim_t *y)
{
    *x = EGUI_VIEW_OF(&test_menu)->region_screen.location.x + EGUI_VIEW_OF(&test_menu)->region.size.width / 2;
    *y = EGUI_VIEW_OF(&test_menu)->region_screen.location.y + test_menu.header_height + index * test_menu.item_height + test_menu.item_height / 2;
}

static void get_back_center(egui_dim_t *x, egui_dim_t *y)
{
    *x = EGUI_VIEW_OF(&test_menu)->region_screen.location.x + test_menu.header_height / 2;
    *y = EGUI_VIEW_OF(&test_menu)->region_screen.location.y + test_menu.header_height / 2;
}

static void test_menu_release_requires_same_hit_target(void)
{
    egui_dim_t x_settings = 0;
    egui_dim_t y_settings = 0;
    egui_dim_t x_about = 0;
    egui_dim_t y_about = 0;
    egui_dim_t x_back = 0;
    egui_dim_t y_back = 0;
    egui_dim_t x_display = 0;
    egui_dim_t y_display = 0;

    setup_menu();
    layout_menu();
    get_item_center(0, &x_settings, &y_settings);
    get_item_center(1, &x_about, &y_about);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x_settings, y_settings));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_menu.pressed_index);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_menu)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x_about, y_about));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_menu.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_menu)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x_about, y_about));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_menu.current_page);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_menu.stack_depth);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_click_count);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_MENU_PRESSED_NONE, test_menu.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_menu)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x_settings, y_settings));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x_about, y_about));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x_settings, y_settings));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_menu)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x_settings, y_settings));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_menu.current_page);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_menu.stack_depth);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_click_count);

    get_back_center(&x_back, &y_back);
    get_item_center(0, &x_display, &y_display);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x_back, y_back));
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_MENU_PRESSED_BACK, test_menu.pressed_index);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_menu)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x_display, y_display));
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_MENU_PRESSED_BACK, test_menu.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_menu)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x_display, y_display));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_menu.current_page);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_menu.stack_depth);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_click_count);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x_back, y_back));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x_display, y_display));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x_back, y_back));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_menu)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x_back, y_back));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_menu.current_page);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_menu.stack_depth);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_click_count);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_MENU_PRESSED_NONE, test_menu.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_menu)->is_pressed);
}

void test_menu_run(void)
{
    EGUI_TEST_SUITE_BEGIN(menu);
    EGUI_TEST_RUN(test_menu_release_requires_same_hit_target);
    EGUI_TEST_SUITE_END();
}
