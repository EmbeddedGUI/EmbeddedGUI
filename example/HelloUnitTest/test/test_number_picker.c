#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_number_picker.h"

static egui_view_number_picker_t test_picker;
static uint8_t g_change_count;
static int16_t g_last_value;

static void on_value_changed(egui_view_t *self, int16_t value)
{
    EGUI_UNUSED(self);
    g_change_count++;
    g_last_value = value;
}

static void setup_picker(void)
{
    egui_view_number_picker_init(EGUI_VIEW_OF(&test_picker));
    egui_view_set_size(EGUI_VIEW_OF(&test_picker), 60, 90);
    egui_view_number_picker_set_range(EGUI_VIEW_OF(&test_picker), 0, 100);
    egui_view_number_picker_set_value(EGUI_VIEW_OF(&test_picker), 10);
    egui_view_number_picker_set_step(EGUI_VIEW_OF(&test_picker), 5);
    egui_view_number_picker_set_on_value_changed_listener(EGUI_VIEW_OF(&test_picker), on_value_changed);
    g_change_count = 0;
    g_last_value = 0;
}

static void layout_picker(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 60;
    region.size.height = 90;
    egui_view_layout(EGUI_VIEW_OF(&test_picker), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_picker)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_picker)->api->on_touch_event(EGUI_VIEW_OF(&test_picker), &event);
}

static void get_zone_center(int8_t zone, egui_dim_t *x, egui_dim_t *y)
{
    egui_dim_t third_h = EGUI_VIEW_OF(&test_picker)->region.size.height / 3;

    *x = EGUI_VIEW_OF(&test_picker)->region_screen.location.x + EGUI_VIEW_OF(&test_picker)->region.size.width / 2;
    if (zone > 0)
    {
        *y = EGUI_VIEW_OF(&test_picker)->region_screen.location.y + third_h / 2;
    }
    else
    {
        *y = EGUI_VIEW_OF(&test_picker)->region_screen.location.y + EGUI_VIEW_OF(&test_picker)->region.size.height - third_h / 2;
    }
}

static void test_number_picker_release_requires_same_zone(void)
{
    egui_dim_t x_top = 0;
    egui_dim_t y_top = 0;
    egui_dim_t x_bottom = 0;
    egui_dim_t y_bottom = 0;

    setup_picker();
    layout_picker();
    get_zone_center(1, &x_top, &y_top);
    get_zone_center(-1, &x_bottom, &y_bottom);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x_top, y_top));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_picker.pressed_zone);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_picker)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x_bottom, y_bottom));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_picker.pressed_zone);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_picker)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x_bottom, y_bottom));
    EGUI_TEST_ASSERT_EQUAL_INT(10, egui_view_number_picker_get_value(EGUI_VIEW_OF(&test_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_change_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_picker.pressed_zone);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_picker)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x_top, y_top));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x_bottom, y_bottom));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x_top, y_top));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_picker.pressed_zone);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_picker)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x_top, y_top));
    EGUI_TEST_ASSERT_EQUAL_INT(15, egui_view_number_picker_get_value(EGUI_VIEW_OF(&test_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_change_count);
    EGUI_TEST_ASSERT_EQUAL_INT(15, g_last_value);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_picker.pressed_zone);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x_bottom, y_bottom));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, test_picker.pressed_zone);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x_top, y_top));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, test_picker.pressed_zone);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_picker)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x_top, y_top));
    EGUI_TEST_ASSERT_EQUAL_INT(15, egui_view_number_picker_get_value(EGUI_VIEW_OF(&test_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_change_count);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x_bottom, y_bottom));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x_top, y_top));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x_bottom, y_bottom));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, test_picker.pressed_zone);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_picker)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x_bottom, y_bottom));
    EGUI_TEST_ASSERT_EQUAL_INT(10, egui_view_number_picker_get_value(EGUI_VIEW_OF(&test_picker)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_change_count);
    EGUI_TEST_ASSERT_EQUAL_INT(10, g_last_value);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_picker.pressed_zone);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_picker)->is_pressed);
}

void test_number_picker_run(void)
{
    EGUI_TEST_SUITE_BEGIN(number_picker);
    EGUI_TEST_RUN(test_number_picker_release_requires_same_zone);
    EGUI_TEST_SUITE_END();
}
