#include "egui.h"
#include "test/egui_test.h"
#include "test_rating_control.h"

#include <string.h>

#include "../../HelloCustomWidgets/input/rating_control/egui_view_rating_control.h"
#include "../../HelloCustomWidgets/input/rating_control/egui_view_rating_control.c"

static egui_view_rating_control_t test_rating_control;
static uint8_t g_changed_value = 0xFF;
static uint8_t g_changed_part = EGUI_VIEW_RATING_CONTROL_PART_NONE;
static uint8_t g_changed_count = 0;

static void reset_changed_state(void)
{
    g_changed_value = 0xFF;
    g_changed_part = EGUI_VIEW_RATING_CONTROL_PART_NONE;
    g_changed_count = 0;
}

static void on_rating_changed(egui_view_t *self, uint8_t value, uint8_t part)
{
    EGUI_UNUSED(self);
    g_changed_value = value;
    g_changed_part = part;
    g_changed_count++;
}

static void setup_rating_control(uint8_t value)
{
    egui_view_rating_control_init(EGUI_VIEW_OF(&test_rating_control));
    egui_view_rating_control_set_item_count(EGUI_VIEW_OF(&test_rating_control), 5);
    egui_view_rating_control_set_clear_enabled(EGUI_VIEW_OF(&test_rating_control), 1);
    egui_view_rating_control_set_on_changed_listener(EGUI_VIEW_OF(&test_rating_control), on_rating_changed);
    egui_view_rating_control_set_value(EGUI_VIEW_OF(&test_rating_control), value);
    reset_changed_state();
}

static void layout_rating_control(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = x;
    region.location.y = y;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(EGUI_VIEW_OF(&test_rating_control), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_rating_control)->region_screen, &region);
}

static int send_touch_event(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_rating_control)->api->on_touch_event(EGUI_VIEW_OF(&test_rating_control), &event);
}

static int send_key_event(uint8_t type, uint8_t key_code)
{
    egui_key_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.key_code = key_code;
    return EGUI_VIEW_OF(&test_rating_control)->api->on_key_event(EGUI_VIEW_OF(&test_rating_control), &event);
}

static void assert_changed_state(uint8_t count, uint8_t value, uint8_t part)
{
    EGUI_TEST_ASSERT_EQUAL_INT(count, g_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(value, g_changed_value);
    EGUI_TEST_ASSERT_EQUAL_INT(part, g_changed_part);
}

static void test_rating_control_enter_clear_commits_zero(void)
{
    setup_rating_control(3);

    egui_view_rating_control_set_current_part(EGUI_VIEW_OF(&test_rating_control), EGUI_VIEW_RATING_CONTROL_PART_CLEAR);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_RATING_CONTROL_PART_CLEAR, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    assert_changed_state(1, 0, EGUI_VIEW_RATING_CONTROL_PART_CLEAR);
}

static void test_rating_control_space_clear_commits_zero(void)
{
    setup_rating_control(4);

    egui_view_rating_control_set_current_part(EGUI_VIEW_OF(&test_rating_control), EGUI_VIEW_RATING_CONTROL_PART_CLEAR);
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_SPACE));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    assert_changed_state(1, 0, EGUI_VIEW_RATING_CONTROL_PART_CLEAR);
}

static void test_rating_control_escape_on_clear_commits_zero(void)
{
    setup_rating_control(2);

    egui_view_rating_control_set_current_part(EGUI_VIEW_OF(&test_rating_control), EGUI_VIEW_RATING_CONTROL_PART_CLEAR);
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_ESCAPE));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    assert_changed_state(1, 0, EGUI_VIEW_RATING_CONTROL_PART_CLEAR);
}

static void test_rating_control_left_from_clear_restores_committed_part(void)
{
    setup_rating_control(3);

    egui_view_rating_control_set_current_part(EGUI_VIEW_OF(&test_rating_control), EGUI_VIEW_RATING_CONTROL_PART_CLEAR);
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_rating_control_enter_star_commits_current_part(void)
{
    setup_rating_control(1);

    egui_view_rating_control_set_current_part(EGUI_VIEW_OF(&test_rating_control), 3);
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    assert_changed_state(1, 3, 3);
}

static void test_rating_control_home_end_commit_bounds(void)
{
    setup_rating_control(3);

    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    assert_changed_state(1, 5, 5);

    reset_changed_state();
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    assert_changed_state(1, 1, 1);
}

static void test_rating_control_right_increments_and_stops_at_max(void)
{
    setup_rating_control(3);

    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    assert_changed_state(1, 4, 4);

    reset_changed_state();
    egui_view_rating_control_set_value(EGUI_VIEW_OF(&test_rating_control), 5);
    reset_changed_state();
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_rating_control_up_matches_left_navigation(void)
{
    setup_rating_control(3);

    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_UP));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    assert_changed_state(1, 2, 2);
}

static void test_rating_control_down_increments_and_ignores_clear_focus(void)
{
    setup_rating_control(3);

    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    assert_changed_state(1, 4, 4);

    reset_changed_state();
    egui_view_rating_control_set_current_part(EGUI_VIEW_OF(&test_rating_control), EGUI_VIEW_RATING_CONTROL_PART_CLEAR);
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_RATING_CONTROL_PART_CLEAR, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_rating_control_escape_on_star_resets_focus_without_clearing(void)
{
    setup_rating_control(4);

    egui_view_rating_control_set_current_part(EGUI_VIEW_OF(&test_rating_control), 2);
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_ESCAPE));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_rating_control_tab_cycles_to_clear_and_wraps(void)
{
    setup_rating_control(3);

    egui_view_rating_control_set_current_part(EGUI_VIEW_OF(&test_rating_control), 5);
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_RATING_CONTROL_PART_CLEAR, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);

    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_rating_control_tab_skips_clear_when_value_empty(void)
{
    setup_rating_control(0);

    egui_view_rating_control_set_current_part(EGUI_VIEW_OF(&test_rating_control), 5);
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_rating_control_tab_skips_clear_when_disabled_by_flag(void)
{
    setup_rating_control(4);

    egui_view_rating_control_set_clear_enabled(EGUI_VIEW_OF(&test_rating_control), 0);
    egui_view_rating_control_set_current_part(EGUI_VIEW_OF(&test_rating_control), 5);
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_rating_control_tab_skips_clear_in_compact_mode(void)
{
    setup_rating_control(4);

    egui_view_rating_control_set_compact_mode(EGUI_VIEW_OF(&test_rating_control), 1);
    egui_view_rating_control_set_current_part(EGUI_VIEW_OF(&test_rating_control), 5);
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_rating_control_navigation_ignored_when_read_only(void)
{
    setup_rating_control(4);

    egui_view_rating_control_set_read_only_mode(EGUI_VIEW_OF(&test_rating_control), 1);
    egui_view_rating_control_set_current_part(EGUI_VIEW_OF(&test_rating_control), 3);
    EGUI_TEST_ASSERT_FALSE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_rating_control_navigation_ignored_when_disabled(void)
{
    setup_rating_control(4);

    egui_view_set_enable(EGUI_VIEW_OF(&test_rating_control), 0);
    egui_view_rating_control_set_current_part(EGUI_VIEW_OF(&test_rating_control), 3);
    EGUI_TEST_ASSERT_FALSE(egui_view_rating_control_handle_navigation_key(EGUI_VIEW_OF(&test_rating_control), EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_rating_control_part_region_follows_visibility_rules(void)
{
    egui_region_t clear_region;
    egui_region_t star_region;

    setup_rating_control(4);
    layout_rating_control(12, 18, 196, 92);

    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_get_part_region(EGUI_VIEW_OF(&test_rating_control), EGUI_VIEW_RATING_CONTROL_PART_CLEAR, &clear_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_get_part_region(EGUI_VIEW_OF(&test_rating_control), 1, &star_region));
    EGUI_TEST_ASSERT_TRUE(clear_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(star_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(clear_region.location.x > star_region.location.x);
    EGUI_TEST_ASSERT_TRUE(clear_region.location.y < star_region.location.y);

    egui_view_rating_control_set_value(EGUI_VIEW_OF(&test_rating_control), 0);
    EGUI_TEST_ASSERT_FALSE(egui_view_rating_control_get_part_region(EGUI_VIEW_OF(&test_rating_control), EGUI_VIEW_RATING_CONTROL_PART_CLEAR, &clear_region));

    egui_view_rating_control_set_value(EGUI_VIEW_OF(&test_rating_control), 4);
    egui_view_rating_control_set_clear_enabled(EGUI_VIEW_OF(&test_rating_control), 0);
    EGUI_TEST_ASSERT_FALSE(egui_view_rating_control_get_part_region(EGUI_VIEW_OF(&test_rating_control), EGUI_VIEW_RATING_CONTROL_PART_CLEAR, &clear_region));

    egui_view_rating_control_set_clear_enabled(EGUI_VIEW_OF(&test_rating_control), 1);
    egui_view_rating_control_set_compact_mode(EGUI_VIEW_OF(&test_rating_control), 1);
    EGUI_TEST_ASSERT_FALSE(egui_view_rating_control_get_part_region(EGUI_VIEW_OF(&test_rating_control), EGUI_VIEW_RATING_CONTROL_PART_CLEAR, &clear_region));

    egui_view_rating_control_set_compact_mode(EGUI_VIEW_OF(&test_rating_control), 0);
    egui_view_rating_control_set_read_only_mode(EGUI_VIEW_OF(&test_rating_control), 1);
    EGUI_TEST_ASSERT_FALSE(egui_view_rating_control_get_part_region(EGUI_VIEW_OF(&test_rating_control), EGUI_VIEW_RATING_CONTROL_PART_CLEAR, &clear_region));
}

static void test_rating_control_item_count_value_and_label_count_normalize(void)
{
    static const char *labels[16] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15"};

    setup_rating_control(4);

    egui_view_rating_control_set_item_count(EGUI_VIEW_OF(&test_rating_control), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_rating_control.item_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));

    egui_view_rating_control_set_item_count(EGUI_VIEW_OF(&test_rating_control), 99);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_RATING_CONTROL_MAX_ITEMS, test_rating_control.item_count);

    egui_view_rating_control_set_value(EGUI_VIEW_OF(&test_rating_control), 99);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_RATING_CONTROL_MAX_ITEMS, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_RATING_CONTROL_MAX_ITEMS, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));

    egui_view_rating_control_set_value_labels(EGUI_VIEW_OF(&test_rating_control), labels, 16);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_RATING_CONTROL_MAX_ITEMS + 1, test_rating_control.label_count);
}

static void test_rating_control_set_value_clear_uses_clear_notification(void)
{
    setup_rating_control(4);

    egui_view_rating_control_set_value(EGUI_VIEW_OF(&test_rating_control), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    assert_changed_state(1, 0, EGUI_VIEW_RATING_CONTROL_PART_CLEAR);
}

static void test_rating_control_set_value_same_value_is_quiet(void)
{
    setup_rating_control(4);

    egui_view_rating_control_set_value(EGUI_VIEW_OF(&test_rating_control), 4);
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_rating_control_set_current_part_ignores_invalid_target(void)
{
    setup_rating_control(3);

    egui_view_rating_control_set_current_part(EGUI_VIEW_OF(&test_rating_control), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);

    egui_view_rating_control_set_value(EGUI_VIEW_OF(&test_rating_control), 0);
    reset_changed_state();
    egui_view_rating_control_set_current_part(EGUI_VIEW_OF(&test_rating_control), EGUI_VIEW_RATING_CONTROL_PART_CLEAR);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_rating_control_set_item_count_shrinks_value_without_notifying(void)
{
    setup_rating_control(5);

    egui_view_rating_control_set_item_count(EGUI_VIEW_OF(&test_rating_control), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(3, test_rating_control.item_count);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_rating_control_touch_star_commits_value(void)
{
    egui_region_t star_region;
    egui_dim_t x;
    egui_dim_t y;

    setup_rating_control(2);
    layout_rating_control(12, 18, 196, 92);
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_get_part_region(EGUI_VIEW_OF(&test_rating_control), 4, &star_region));
    x = star_region.location.x + star_region.size.width / 2;
    y = star_region.location.y + star_region.size.height / 2;

    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_rating_control)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(4, test_rating_control.pressed_part);

    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_rating_control)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_RATING_CONTROL_PART_NONE, test_rating_control.pressed_part);
    assert_changed_state(1, 4, 4);
}

static void test_rating_control_touch_clear_commits_zero(void)
{
    egui_region_t clear_region;
    egui_dim_t x;
    egui_dim_t y;

    setup_rating_control(4);
    layout_rating_control(12, 18, 196, 92);
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_get_part_region(EGUI_VIEW_OF(&test_rating_control), EGUI_VIEW_RATING_CONTROL_PART_CLEAR, &clear_region));
    x = clear_region.location.x + clear_region.size.width / 2;
    y = clear_region.location.y + clear_region.size.height / 2;

    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_RATING_CONTROL_PART_CLEAR, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_RATING_CONTROL_PART_CLEAR, test_rating_control.pressed_part);

    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_RATING_CONTROL_PART_NONE, test_rating_control.pressed_part);
    assert_changed_state(1, 0, EGUI_VIEW_RATING_CONTROL_PART_CLEAR);
}

static void test_rating_control_touch_move_outside_cancels_commit(void)
{
    egui_region_t star_region;
    egui_dim_t x;
    egui_dim_t y;

    setup_rating_control(2);
    layout_rating_control(12, 18, 196, 92);
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_get_part_region(EGUI_VIEW_OF(&test_rating_control), 3, &star_region));
    x = star_region.location.x + star_region.size.width / 2;
    y = star_region.location.y + star_region.size.height / 2;

    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_MOVE, 400, 400));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_RATING_CONTROL_PART_NONE, test_rating_control.pressed_part);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_rating_control)->is_pressed);

    EGUI_TEST_ASSERT_FALSE(send_touch_event(EGUI_MOTION_EVENT_ACTION_UP, 400, 400));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_RATING_CONTROL_PART_NONE, test_rating_control.pressed_part);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_rating_control)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_rating_control_touch_gap_prefers_right_star(void)
{
    egui_region_t left_region;
    egui_region_t right_region;
    egui_dim_t gap_start;
    egui_dim_t gap_width;
    egui_dim_t x;
    egui_dim_t y;

    setup_rating_control(2);
    layout_rating_control(12, 18, 106, 42);
    egui_view_rating_control_set_compact_mode(EGUI_VIEW_OF(&test_rating_control), 1);
    layout_rating_control(12, 18, 106, 42);
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_get_part_region(EGUI_VIEW_OF(&test_rating_control), 3, &left_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_get_part_region(EGUI_VIEW_OF(&test_rating_control), 4, &right_region));
    gap_start = left_region.location.x + left_region.size.width;
    gap_width = right_region.location.x - gap_start;
    x = gap_start + gap_width - 1;
    y = left_region.location.y + left_region.size.height / 2;

    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    assert_changed_state(1, 4, 4);
}

static void test_rating_control_touch_drag_to_new_star_commits_latest_part(void)
{
    egui_region_t start_region;
    egui_region_t end_region;
    egui_dim_t start_x;
    egui_dim_t start_y;
    egui_dim_t end_x;
    egui_dim_t end_y;

    setup_rating_control(1);
    layout_rating_control(12, 18, 196, 92);
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_get_part_region(EGUI_VIEW_OF(&test_rating_control), 2, &start_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_get_part_region(EGUI_VIEW_OF(&test_rating_control), 5, &end_region));
    start_x = start_region.location.x + start_region.size.width / 2;
    start_y = start_region.location.y + start_region.size.height / 2;
    end_x = end_region.location.x + end_region.size.width / 2;
    end_y = end_region.location.y + end_region.size.height / 2;

    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, start_x, start_y));
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_rating_control.pressed_part);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));

    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_MOVE, end_x, end_y));
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_rating_control.pressed_part);
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_rating_control)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_UP, end_x, end_y));
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_rating_control)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_RATING_CONTROL_PART_NONE, test_rating_control.pressed_part);
    assert_changed_state(1, 5, 5);
}

static void test_rating_control_touch_cancel_clears_pressed_state_without_commit(void)
{
    egui_region_t star_region;
    egui_dim_t x;
    egui_dim_t y;

    setup_rating_control(3);
    layout_rating_control(12, 18, 196, 92);
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_get_part_region(EGUI_VIEW_OF(&test_rating_control), 5, &star_region));
    x = star_region.location.x + star_region.size.width / 2;
    y = star_region.location.y + star_region.size.height / 2;

    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_rating_control.pressed_part);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_rating_control)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_CANCEL, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_rating_control)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_RATING_CONTROL_PART_NONE, test_rating_control.pressed_part);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_rating_control_touch_ignored_when_read_only_or_disabled(void)
{
    egui_region_t star_region;
    egui_dim_t x;
    egui_dim_t y;

    setup_rating_control(4);
    layout_rating_control(12, 18, 196, 92);
    EGUI_TEST_ASSERT_TRUE(egui_view_rating_control_get_part_region(EGUI_VIEW_OF(&test_rating_control), 2, &star_region));
    x = star_region.location.x + star_region.size.width / 2;
    y = star_region.location.y + star_region.size.height / 2;

    egui_view_rating_control_set_read_only_mode(EGUI_VIEW_OF(&test_rating_control), 1);
    EGUI_TEST_ASSERT_FALSE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);

    egui_view_rating_control_set_read_only_mode(EGUI_VIEW_OF(&test_rating_control), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_rating_control), 0);
    EGUI_TEST_ASSERT_FALSE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_rating_control_key_event_action_down_consumes_navigation_without_commit(void)
{
    setup_rating_control(3);

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_rating_control_key_event_action_up_commits_navigation(void)
{
    setup_rating_control(3);

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    assert_changed_state(1, 4, 4);
}

static void test_rating_control_key_event_unknown_key_falls_back(void)
{
    setup_rating_control(3);

    EGUI_TEST_ASSERT_FALSE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_A));
    EGUI_TEST_ASSERT_FALSE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_A));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_rating_control_key_event_read_only_down_is_consumed_but_up_is_quiet(void)
{
    setup_rating_control(4);

    egui_view_rating_control_set_read_only_mode(EGUI_VIEW_OF(&test_rating_control), 1);
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_FALSE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_rating_control_key_event_disabled_down_is_consumed_but_up_is_quiet(void)
{
    setup_rating_control(4);

    egui_view_set_enable(EGUI_VIEW_OF(&test_rating_control), 0);
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_FALSE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_value(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_rating_control_get_current_part(EGUI_VIEW_OF(&test_rating_control)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

void test_rating_control_run(void)
{
    EGUI_TEST_SUITE_BEGIN(rating_control);

    EGUI_TEST_RUN(test_rating_control_enter_clear_commits_zero);
    EGUI_TEST_RUN(test_rating_control_space_clear_commits_zero);
    EGUI_TEST_RUN(test_rating_control_escape_on_clear_commits_zero);
    EGUI_TEST_RUN(test_rating_control_left_from_clear_restores_committed_part);
    EGUI_TEST_RUN(test_rating_control_enter_star_commits_current_part);
    EGUI_TEST_RUN(test_rating_control_home_end_commit_bounds);
    EGUI_TEST_RUN(test_rating_control_right_increments_and_stops_at_max);
    EGUI_TEST_RUN(test_rating_control_up_matches_left_navigation);
    EGUI_TEST_RUN(test_rating_control_down_increments_and_ignores_clear_focus);
    EGUI_TEST_RUN(test_rating_control_escape_on_star_resets_focus_without_clearing);
    EGUI_TEST_RUN(test_rating_control_tab_cycles_to_clear_and_wraps);
    EGUI_TEST_RUN(test_rating_control_tab_skips_clear_when_value_empty);
    EGUI_TEST_RUN(test_rating_control_tab_skips_clear_when_disabled_by_flag);
    EGUI_TEST_RUN(test_rating_control_tab_skips_clear_in_compact_mode);
    EGUI_TEST_RUN(test_rating_control_navigation_ignored_when_read_only);
    EGUI_TEST_RUN(test_rating_control_navigation_ignored_when_disabled);
    EGUI_TEST_RUN(test_rating_control_part_region_follows_visibility_rules);
    EGUI_TEST_RUN(test_rating_control_item_count_value_and_label_count_normalize);
    EGUI_TEST_RUN(test_rating_control_set_value_clear_uses_clear_notification);
    EGUI_TEST_RUN(test_rating_control_set_value_same_value_is_quiet);
    EGUI_TEST_RUN(test_rating_control_set_current_part_ignores_invalid_target);
    EGUI_TEST_RUN(test_rating_control_set_item_count_shrinks_value_without_notifying);
    EGUI_TEST_RUN(test_rating_control_touch_star_commits_value);
    EGUI_TEST_RUN(test_rating_control_touch_clear_commits_zero);
    EGUI_TEST_RUN(test_rating_control_touch_move_outside_cancels_commit);
    EGUI_TEST_RUN(test_rating_control_touch_gap_prefers_right_star);
    EGUI_TEST_RUN(test_rating_control_touch_drag_to_new_star_commits_latest_part);
    EGUI_TEST_RUN(test_rating_control_touch_cancel_clears_pressed_state_without_commit);
    EGUI_TEST_RUN(test_rating_control_touch_ignored_when_read_only_or_disabled);
    EGUI_TEST_RUN(test_rating_control_key_event_action_down_consumes_navigation_without_commit);
    EGUI_TEST_RUN(test_rating_control_key_event_action_up_commits_navigation);
    EGUI_TEST_RUN(test_rating_control_key_event_unknown_key_falls_back);
    EGUI_TEST_RUN(test_rating_control_key_event_read_only_down_is_consumed_but_up_is_quiet);
    EGUI_TEST_RUN(test_rating_control_key_event_disabled_down_is_consumed_but_up_is_quiet);

    EGUI_TEST_SUITE_END();
}
