#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_menu.h"

#define TEST_MENU_PRESSED_NONE (-1)
#define TEST_MENU_PRESSED_BACK (-2)

static egui_view_menu_t test_menu;
static uint8_t g_click_count;
static uint8_t g_last_page_index;
static uint8_t g_last_item_index;

static egui_core_t *test_menu_get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

static const egui_view_menu_item_t g_root_items[] = {
        {"Settings", 1, NULL},
        {"About", EGUI_VIEW_MENU_ITEM_LEAF, NULL},
};

static const egui_view_menu_item_t g_settings_items[] = {
        {"Display", EGUI_VIEW_MENU_ITEM_LEAF, NULL},
        {"Sound", EGUI_VIEW_MENU_ITEM_LEAF, NULL},
};

static const egui_view_menu_page_t g_pages[] = {
        {"Main", g_root_items, 2},
        {"Settings", g_settings_items, 2},
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
    egui_view_menu_init(EGUI_VIEW_OF(&test_menu), test_menu_get_core());
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

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int send_key(uint8_t type, uint8_t key_code)
{
    egui_key_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.key_code = key_code;
    return EGUI_VIEW_OF(&test_menu)->api->on_key_event(EGUI_VIEW_OF(&test_menu), &event);
}

static void send_key_click(uint8_t key_code)
{
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_EVENT_ACTION_DOWN, key_code));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_EVENT_ACTION_UP, key_code));
}
#endif

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

static void test_menu_get_state_default(void)
{
    egui_view_menu_init(EGUI_VIEW_OF(&test_menu), test_menu_get_core());

    EGUI_TEST_ASSERT_NULL(egui_view_menu_get_pages(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_page_count(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_current_page(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_stack_depth(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_MENU_SELECTED_NONE, (int)egui_view_menu_get_selected_index(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(30, (int)egui_view_menu_get_header_height(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(30, (int)egui_view_menu_get_item_height(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT_PRIMARY.full, (int)egui_view_menu_get_text_color(EGUI_VIEW_OF(&test_menu)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_WHITE.full, (int)egui_view_menu_get_header_text_color(EGUI_VIEW_OF(&test_menu)).full);
    EGUI_TEST_ASSERT_NULL(egui_view_menu_get_on_item_click(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_NULL(egui_view_menu_get_icon_font(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(egui_view_menu_get_back_icon(EGUI_VIEW_OF(&test_menu)), EGUI_ICON_MS_ARROW_BACK));
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(egui_view_menu_get_submenu_icon(EGUI_VIEW_OF(&test_menu)), EGUI_ICON_MS_ARROW_FORWARD));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_menu_get_icon_text_gap(EGUI_VIEW_OF(&test_menu)));
}

static void test_menu_get_state_after_setters(void)
{
    egui_color_t color = {.full = 0x2468};
    const char *back_icon = "b";
    const char *submenu_icon = "s";

    setup_menu();
    egui_view_menu_set_header_height(EGUI_VIEW_OF(&test_menu), 24);
    egui_view_menu_set_item_height(EGUI_VIEW_OF(&test_menu), 28);
    egui_view_menu_set_header_text_color(EGUI_VIEW_OF(&test_menu), color);
    egui_view_menu_set_icon_font(EGUI_VIEW_OF(&test_menu), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_menu_set_navigation_icons(EGUI_VIEW_OF(&test_menu), back_icon, submenu_icon);
    egui_view_menu_set_icon_text_gap(EGUI_VIEW_OF(&test_menu), 9);

    EGUI_TEST_ASSERT_TRUE(egui_view_menu_get_pages(EGUI_VIEW_OF(&test_menu)) == g_pages);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_menu_get_page_count(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_current_page(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_stack_depth(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_selected_index(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(24, (int)egui_view_menu_get_header_height(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(28, (int)egui_view_menu_get_item_height(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT_PRIMARY.full, (int)egui_view_menu_get_text_color(EGUI_VIEW_OF(&test_menu)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)color.full, (int)egui_view_menu_get_header_text_color(EGUI_VIEW_OF(&test_menu)).full);
    EGUI_TEST_ASSERT_TRUE(egui_view_menu_get_on_item_click(EGUI_VIEW_OF(&test_menu)) == on_item_click);
    EGUI_TEST_ASSERT_TRUE(egui_view_menu_get_icon_font(EGUI_VIEW_OF(&test_menu)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(egui_view_menu_get_back_icon(EGUI_VIEW_OF(&test_menu)) == back_icon);
    EGUI_TEST_ASSERT_TRUE(egui_view_menu_get_submenu_icon(EGUI_VIEW_OF(&test_menu)) == submenu_icon);
    EGUI_TEST_ASSERT_EQUAL_INT(9, (int)egui_view_menu_get_icon_text_gap(EGUI_VIEW_OF(&test_menu)));

    egui_view_menu_set_navigation_icons(EGUI_VIEW_OF(&test_menu), NULL, NULL);
    egui_view_menu_set_on_item_click(EGUI_VIEW_OF(&test_menu), NULL);
    egui_view_menu_set_icon_font(EGUI_VIEW_OF(&test_menu), NULL);
    egui_view_menu_set_icon_text_gap(EGUI_VIEW_OF(&test_menu), -3);

    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(egui_view_menu_get_back_icon(EGUI_VIEW_OF(&test_menu)), EGUI_ICON_MS_ARROW_BACK));
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(egui_view_menu_get_submenu_icon(EGUI_VIEW_OF(&test_menu)), EGUI_ICON_MS_ARROW_FORWARD));
    EGUI_TEST_ASSERT_NULL(egui_view_menu_get_on_item_click(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_NULL(egui_view_menu_get_icon_font(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_icon_text_gap(EGUI_VIEW_OF(&test_menu)));
}

static void test_menu_get_state_tracks_navigation(void)
{
    setup_menu();

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_current_page(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_stack_depth(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_selected_index(EGUI_VIEW_OF(&test_menu)));

    egui_view_menu_navigate_to(EGUI_VIEW_OF(&test_menu), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_menu_get_current_page(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_menu_get_stack_depth(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_selected_index(EGUI_VIEW_OF(&test_menu)));

    egui_view_menu_navigate_to(EGUI_VIEW_OF(&test_menu), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_menu_get_current_page(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_menu_get_stack_depth(EGUI_VIEW_OF(&test_menu)));

    egui_view_menu_go_back(EGUI_VIEW_OF(&test_menu));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_current_page(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_stack_depth(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_selected_index(EGUI_VIEW_OF(&test_menu)));
}

static void test_menu_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_menu_get_pages(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_page_count(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_current_page(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_stack_depth(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_MENU_SELECTED_NONE, (int)egui_view_menu_get_selected_index(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_header_height(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_item_height(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_text_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_header_text_color(NULL).full);
    EGUI_TEST_ASSERT_NULL(egui_view_menu_get_on_item_click(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_menu_get_icon_font(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_menu_get_back_icon(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_menu_get_submenu_icon(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_icon_text_gap(NULL));
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
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_menu_get_current_page(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_menu.stack_depth);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_menu_get_stack_depth(EGUI_VIEW_OF(&test_menu)));
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
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_current_page(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_menu.stack_depth);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_stack_depth(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_click_count);
    EGUI_TEST_ASSERT_EQUAL_INT(TEST_MENU_PRESSED_NONE, test_menu.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_menu)->is_pressed);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static void test_menu_key_navigation_opens_submenu_and_selects_child(void)
{
    setup_menu();
    layout_menu();

    EGUI_TEST_ASSERT_EQUAL_INT(0, test_menu.current_page);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_menu.selected_index);

    send_key_click(EGUI_KEY_CODE_ENTER);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_menu.current_page);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_menu_get_current_page(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_menu.stack_depth);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_menu_get_stack_depth(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_menu.selected_index);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_selected_index(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_click_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_menu.selected_index);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_menu_get_selected_index(EGUI_VIEW_OF(&test_menu)));

    send_key_click(EGUI_KEY_CODE_ENTER);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_menu.current_page);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_click_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_page_index);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_item_index);

    send_key_click(EGUI_KEY_CODE_LEFT);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_menu.current_page);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_current_page(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_menu.stack_depth);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_stack_depth(EGUI_VIEW_OF(&test_menu)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_menu.selected_index);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_menu_get_selected_index(EGUI_VIEW_OF(&test_menu)));
}
#endif

void test_menu_run(void)
{
    EGUI_TEST_SUITE_BEGIN(menu);
    EGUI_TEST_RUN(test_menu_get_state_default);
    EGUI_TEST_RUN(test_menu_get_state_after_setters);
    EGUI_TEST_RUN(test_menu_get_state_tracks_navigation);
    EGUI_TEST_RUN(test_menu_get_state_null_self);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    EGUI_TEST_RUN(test_menu_release_requires_same_hit_target);
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    EGUI_TEST_RUN(test_menu_key_navigation_opens_submenu_and_selects_child);
#endif
    EGUI_TEST_SUITE_END();
}
