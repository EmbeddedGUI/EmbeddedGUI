#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_password_box.h"

#include "../../HelloCustomWidgets/input/password_box/egui_view_password_box.h"
#include "../../HelloCustomWidgets/input/password_box/egui_view_password_box.c"

static egui_view_password_box_t test_password_box;

static void setup_password_box(const char *text)
{
    egui_view_password_box_init(EGUI_VIEW_OF(&test_password_box));
    egui_view_set_size(EGUI_VIEW_OF(&test_password_box), 196, 70);
    egui_view_password_box_set_text(EGUI_VIEW_OF(&test_password_box), text);
}

static void layout_password_box(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = x;
    region.location.y = y;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(EGUI_VIEW_OF(&test_password_box), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_password_box)->region_screen, &region);
}

static int send_touch_event(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_password_box)->api->on_touch_event(EGUI_VIEW_OF(&test_password_box), &event);
}

static int press_key(uint8_t key_code, uint8_t is_shift)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    event.is_shift = is_shift ? 1 : 0;
    handled |= EGUI_VIEW_OF(&test_password_box)->api->on_key_event(EGUI_VIEW_OF(&test_password_box), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_password_box)->api->on_key_event(EGUI_VIEW_OF(&test_password_box), &event);
    return handled;
}

static void test_password_box_tab_cycles_to_reveal_when_text_exists(void)
{
    setup_password_box("abcd");
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PASSWORD_BOX_PART_FIELD, egui_view_password_box_get_current_part(EGUI_VIEW_OF(&test_password_box)));
    EGUI_TEST_ASSERT_TRUE(egui_view_password_box_handle_navigation_key(EGUI_VIEW_OF(&test_password_box), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PASSWORD_BOX_PART_REVEAL, egui_view_password_box_get_current_part(EGUI_VIEW_OF(&test_password_box)));
    EGUI_TEST_ASSERT_TRUE(egui_view_password_box_handle_navigation_key(EGUI_VIEW_OF(&test_password_box), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PASSWORD_BOX_PART_FIELD, egui_view_password_box_get_current_part(EGUI_VIEW_OF(&test_password_box)));
}

static void test_password_box_tab_stays_on_field_when_empty_or_read_only(void)
{
    setup_password_box("");
    EGUI_TEST_ASSERT_TRUE(egui_view_password_box_handle_navigation_key(EGUI_VIEW_OF(&test_password_box), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PASSWORD_BOX_PART_FIELD, egui_view_password_box_get_current_part(EGUI_VIEW_OF(&test_password_box)));

    setup_password_box("abcd");
    egui_view_password_box_set_read_only_mode(EGUI_VIEW_OF(&test_password_box), 1);
    EGUI_TEST_ASSERT_FALSE(egui_view_password_box_handle_navigation_key(EGUI_VIEW_OF(&test_password_box), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PASSWORD_BOX_PART_FIELD, egui_view_password_box_get_current_part(EGUI_VIEW_OF(&test_password_box)));
}

static void test_password_box_reveal_toggle_via_keyboard(void)
{
    setup_password_box("secret");
    egui_view_password_box_set_current_part(EGUI_VIEW_OF(&test_password_box), EGUI_VIEW_PASSWORD_BOX_PART_REVEAL);
    EGUI_TEST_ASSERT_FALSE(egui_view_password_box_get_revealed(EGUI_VIEW_OF(&test_password_box)));
    EGUI_TEST_ASSERT_TRUE(egui_view_password_box_handle_navigation_key(EGUI_VIEW_OF(&test_password_box), EGUI_KEY_CODE_SPACE));
    EGUI_TEST_ASSERT_TRUE(egui_view_password_box_get_revealed(EGUI_VIEW_OF(&test_password_box)));
    EGUI_TEST_ASSERT_TRUE(egui_view_password_box_handle_navigation_key(EGUI_VIEW_OF(&test_password_box), EGUI_KEY_CODE_ESCAPE));
    EGUI_TEST_ASSERT_FALSE(egui_view_password_box_get_revealed(EGUI_VIEW_OF(&test_password_box)));
}

static void test_password_box_insert_and_backspace_respect_cursor(void)
{
    setup_password_box("abc");
    egui_view_password_box_set_cursor_pos(EGUI_VIEW_OF(&test_password_box), 1);
    EGUI_TEST_ASSERT_TRUE(press_key(EGUI_KEY_CODE_X, 0));
    EGUI_TEST_ASSERT_TRUE(strcmp("axbc", egui_view_password_box_get_text(EGUI_VIEW_OF(&test_password_box))) == 0);
    EGUI_TEST_ASSERT_TRUE(press_key(EGUI_KEY_CODE_BACKSPACE, 0));
    EGUI_TEST_ASSERT_TRUE(strcmp("abc", egui_view_password_box_get_text(EGUI_VIEW_OF(&test_password_box))) == 0);
}

static void test_password_box_delete_forward_and_navigation(void)
{
    setup_password_box("abcd");
    egui_view_password_box_move_cursor_home(EGUI_VIEW_OF(&test_password_box));
    egui_view_password_box_move_cursor_right(EGUI_VIEW_OF(&test_password_box));
    EGUI_TEST_ASSERT_TRUE(press_key(EGUI_KEY_CODE_DELETE, 0));
    EGUI_TEST_ASSERT_TRUE(strcmp("acd", egui_view_password_box_get_text(EGUI_VIEW_OF(&test_password_box))) == 0);
    EGUI_TEST_ASSERT_TRUE(egui_view_password_box_handle_navigation_key(EGUI_VIEW_OF(&test_password_box), EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_TRUE(egui_view_password_box_handle_navigation_key(EGUI_VIEW_OF(&test_password_box), EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PASSWORD_BOX_PART_REVEAL, egui_view_password_box_get_current_part(EGUI_VIEW_OF(&test_password_box)));
}

static void test_password_box_touch_reveal_toggle(void)
{
    egui_region_t region;

    setup_password_box("secret");
    layout_password_box(10, 20, 196, 70);
    EGUI_TEST_ASSERT_TRUE(egui_view_password_box_get_part_region(EGUI_VIEW_OF(&test_password_box), EGUI_VIEW_PASSWORD_BOX_PART_REVEAL, &region));
    EGUI_TEST_ASSERT_TRUE(
            send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(egui_view_password_box_get_revealed(EGUI_VIEW_OF(&test_password_box)));
}

static void test_password_box_read_only_ignores_changes(void)
{
    setup_password_box("secret");
    egui_view_password_box_set_read_only_mode(EGUI_VIEW_OF(&test_password_box), 1);
    EGUI_TEST_ASSERT_FALSE(press_key(EGUI_KEY_CODE_A, 0));
    EGUI_TEST_ASSERT_TRUE(strcmp("secret", egui_view_password_box_get_text(EGUI_VIEW_OF(&test_password_box))) == 0);
    egui_view_password_box_set_revealed(EGUI_VIEW_OF(&test_password_box), 1);
    EGUI_TEST_ASSERT_FALSE(egui_view_password_box_get_revealed(EGUI_VIEW_OF(&test_password_box)));
}

void test_password_box_run(void)
{
    EGUI_TEST_SUITE_BEGIN(password_box);
    EGUI_TEST_RUN(test_password_box_tab_cycles_to_reveal_when_text_exists);
    EGUI_TEST_RUN(test_password_box_tab_stays_on_field_when_empty_or_read_only);
    EGUI_TEST_RUN(test_password_box_reveal_toggle_via_keyboard);
    EGUI_TEST_RUN(test_password_box_insert_and_backspace_respect_cursor);
    EGUI_TEST_RUN(test_password_box_delete_forward_and_navigation);
    EGUI_TEST_RUN(test_password_box_touch_reveal_toggle);
    EGUI_TEST_RUN(test_password_box_read_only_ignores_changes);
    EGUI_TEST_SUITE_END();
}
