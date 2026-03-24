#include <string.h>

#include "egui.h"
#include "core/egui_core_internal.h"
#include "test/egui_test.h"
#include "test_canvas_panner.h"

static egui_view_canvas_panner_t test_panner;
static egui_view_t test_child;
static egui_view_scroll_t test_scroll;
static egui_view_t test_scroll_item1;
static egui_view_t test_scroll_item2;

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static egui_view_api_t test_child_touch_api;
static int g_child_touch_down_count;
static int g_child_touch_move_count;
static int g_child_touch_up_count;
static int g_child_touch_cancel_count;
#endif

static void test_canvas_panner_setup(egui_dim_t width, egui_dim_t height, egui_dim_t canvas_width, egui_dim_t canvas_height)
{
    egui_region_t region;

    egui_view_canvas_panner_init(EGUI_VIEW_OF(&test_panner));
    egui_region_init(&region, 0, 0, width, height);
    egui_view_layout(EGUI_VIEW_OF(&test_panner), &region);
    egui_view_canvas_panner_set_canvas_size(EGUI_VIEW_OF(&test_panner), canvas_width, canvas_height);
}

static void test_canvas_panner_add_child(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_view_init(&test_child);
    egui_view_set_position(&test_child, x, y);
    egui_view_set_size(&test_child, width, height);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_panner), &test_child);
    EGUI_VIEW_OF(&test_panner)->api->calculate_layout(EGUI_VIEW_OF(&test_panner));
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int test_canvas_panner_child_touch_cb(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_UNUSED(self);

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        g_child_touch_down_count++;
        break;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        g_child_touch_move_count++;
        break;
    case EGUI_MOTION_EVENT_ACTION_UP:
        g_child_touch_up_count++;
        break;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        g_child_touch_cancel_count++;
        break;
    default:
        break;
    }

    return 1;
}

static void test_canvas_panner_setup_touch_scene(void)
{
    test_canvas_panner_setup(100, 80, 180, 140);
    test_canvas_panner_add_child(10, 10, 30, 20);
    egui_view_override_api_on_touch(&test_child, &test_child_touch_api, test_canvas_panner_child_touch_cb);

    g_child_touch_down_count = 0;
    g_child_touch_move_count = 0;
    g_child_touch_up_count = 0;
    g_child_touch_cancel_count = 0;
}

static int test_canvas_panner_send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_panner)->api->dispatch_touch_event(EGUI_VIEW_OF(&test_panner), &event);
}
#endif

static void test_canvas_panner_offset_clamps_and_shifts_child_layout(void)
{
    test_canvas_panner_setup(100, 80, 180, 140);
    test_canvas_panner_add_child(30, 40, 20, 10);

    EGUI_TEST_ASSERT_EQUAL_INT(30, test_child.region_screen.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(40, test_child.region_screen.location.y);

    egui_view_canvas_panner_set_offset(EGUI_VIEW_OF(&test_panner), 20, 15);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_panner)->is_request_layout);
    EGUI_VIEW_OF(&test_panner)->api->calculate_layout(EGUI_VIEW_OF(&test_panner));
    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_canvas_panner_get_offset_x(EGUI_VIEW_OF(&test_panner)));
    EGUI_TEST_ASSERT_EQUAL_INT(15, egui_view_canvas_panner_get_offset_y(EGUI_VIEW_OF(&test_panner)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_core_get_pfb_scan_reverse_x());
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_core_get_pfb_scan_reverse_y());
    EGUI_TEST_ASSERT_EQUAL_INT(10, test_child.region_screen.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(25, test_child.region_screen.location.y);

    egui_view_canvas_panner_set_offset(EGUI_VIEW_OF(&test_panner), 200, 100);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_panner)->is_request_layout);
    EGUI_VIEW_OF(&test_panner)->api->calculate_layout(EGUI_VIEW_OF(&test_panner));
    EGUI_TEST_ASSERT_EQUAL_INT(80, egui_view_canvas_panner_get_offset_x(EGUI_VIEW_OF(&test_panner)));
    EGUI_TEST_ASSERT_EQUAL_INT(60, egui_view_canvas_panner_get_offset_y(EGUI_VIEW_OF(&test_panner)));

    egui_view_canvas_panner_set_offset(EGUI_VIEW_OF(&test_panner), -5, -10);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_panner)->is_request_layout);
    EGUI_VIEW_OF(&test_panner)->api->calculate_layout(EGUI_VIEW_OF(&test_panner));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_canvas_panner_get_offset_x(EGUI_VIEW_OF(&test_panner)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_canvas_panner_get_offset_y(EGUI_VIEW_OF(&test_panner)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_core_get_pfb_scan_reverse_x());
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_core_get_pfb_scan_reverse_y());
    EGUI_TEST_ASSERT_EQUAL_INT(30, test_child.region_screen.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(40, test_child.region_screen.location.y);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static void test_canvas_panner_blank_drag_scrolls_canvas(void)
{
    test_canvas_panner_setup_touch_scene();

    EGUI_TEST_ASSERT_TRUE(test_canvas_panner_send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 90, 60));
    EGUI_TEST_ASSERT_TRUE(test_canvas_panner_send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 60, 40));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_panner)->is_request_layout);
    EGUI_TEST_ASSERT_TRUE(test_canvas_panner_send_touch(EGUI_MOTION_EVENT_ACTION_UP, 60, 40));

    EGUI_TEST_ASSERT_EQUAL_INT(30, egui_view_canvas_panner_get_offset_x(EGUI_VIEW_OF(&test_panner)));
    EGUI_TEST_ASSERT_EQUAL_INT(20, egui_view_canvas_panner_get_offset_y(EGUI_VIEW_OF(&test_panner)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_core_get_pfb_scan_reverse_x());
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_core_get_pfb_scan_reverse_y());
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_child_touch_down_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_child_touch_move_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_child_touch_up_count);
}

static void test_canvas_panner_non_draggable_child_drag_can_pan_canvas(void)
{
    test_canvas_panner_setup_touch_scene();

    EGUI_TEST_ASSERT_TRUE(test_canvas_panner_send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 30, 20));
    EGUI_TEST_ASSERT_TRUE(test_canvas_panner_send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 18, 12));
    EGUI_TEST_ASSERT_TRUE(test_canvas_panner_send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 8, 4));
    EGUI_TEST_ASSERT_TRUE(test_canvas_panner_send_touch(EGUI_MOTION_EVENT_ACTION_UP, 8, 4));

    EGUI_TEST_ASSERT_EQUAL_INT(10, egui_view_canvas_panner_get_offset_x(EGUI_VIEW_OF(&test_panner)));
    EGUI_TEST_ASSERT_EQUAL_INT(8, egui_view_canvas_panner_get_offset_y(EGUI_VIEW_OF(&test_panner)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_child_touch_down_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_child_touch_move_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_child_touch_up_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_child_touch_cancel_count);
}

static void test_canvas_panner_scroll_child_keeps_vertical_drag(void)
{
    test_canvas_panner_setup(100, 80, 180, 140);

    egui_view_scroll_init(EGUI_VIEW_OF(&test_scroll));
    egui_view_set_position(EGUI_VIEW_OF(&test_scroll), 10, 10);
    egui_view_set_size(EGUI_VIEW_OF(&test_scroll), 40, 30);

    egui_view_init(&test_scroll_item1);
    egui_view_set_size(&test_scroll_item1, 40, 30);
    egui_view_scroll_add_child(EGUI_VIEW_OF(&test_scroll), &test_scroll_item1);

    egui_view_init(&test_scroll_item2);
    egui_view_set_size(&test_scroll_item2, 40, 30);
    egui_view_scroll_add_child(EGUI_VIEW_OF(&test_scroll), &test_scroll_item2);

    egui_view_scroll_layout_childs(EGUI_VIEW_OF(&test_scroll));
    egui_view_group_add_child(EGUI_VIEW_OF(&test_panner), EGUI_VIEW_OF(&test_scroll));
    EGUI_VIEW_OF(&test_panner)->api->calculate_layout(EGUI_VIEW_OF(&test_panner));

    EGUI_TEST_ASSERT_TRUE(test_canvas_panner_send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 20, 20));
    EGUI_TEST_ASSERT_TRUE(test_canvas_panner_send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 20, 10));
    EGUI_TEST_ASSERT_TRUE(test_canvas_panner_send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 20, 2));
    EGUI_TEST_ASSERT_TRUE(test_canvas_panner_send_touch(EGUI_MOTION_EVENT_ACTION_UP, 20, 2));

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_canvas_panner_get_offset_x(EGUI_VIEW_OF(&test_panner)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_canvas_panner_get_offset_y(EGUI_VIEW_OF(&test_panner)));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_scroll.container)->region.location.y < 0);
}
#endif

void test_canvas_panner_run(void)
{
    EGUI_TEST_SUITE_BEGIN(canvas_panner);

    EGUI_TEST_RUN(test_canvas_panner_offset_clamps_and_shifts_child_layout);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    EGUI_TEST_RUN(test_canvas_panner_blank_drag_scrolls_canvas);
    EGUI_TEST_RUN(test_canvas_panner_non_draggable_child_drag_can_pan_canvas);
    EGUI_TEST_RUN(test_canvas_panner_scroll_child_keeps_vertical_drag);
#endif

    EGUI_TEST_SUITE_END();
}
