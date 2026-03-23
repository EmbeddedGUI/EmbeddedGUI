#include <string.h>

#include "egui.h"
#include "background/egui_background_color.h"
#include "test/egui_test.h"
#include "test_view.h"

static egui_view_t test_view;
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int g_view_click_count;
#endif

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_view_bg_normal_param, EGUI_COLOR_BLUE, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_view_bg_pressed_param, EGUI_COLOR_RED, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(s_view_bg_params, &s_view_bg_normal_param, &s_view_bg_pressed_param, NULL);
static egui_background_color_t s_view_pressed_background;
static uint8_t s_view_pressed_background_ready;

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static void test_view_click_cb(egui_view_t *self)
{
    EGUI_UNUSED(self);
    g_view_click_count++;
}

static void test_view_layout_rect(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = x;
    region.location.y = y;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(&test_view, &region);
    egui_region_copy(&test_view.region_screen, &region);
}

static int test_view_send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return test_view.api->dispatch_touch_event(&test_view, &event);
}
#endif

static void test_view_init_defaults(void)
{
    egui_view_init(&test_view);
    EGUI_TEST_ASSERT_TRUE(test_view.is_enable == 1);
    EGUI_TEST_ASSERT_TRUE(test_view.is_visible == 1);
    EGUI_TEST_ASSERT_TRUE(test_view.is_gone == 0);
    EGUI_TEST_ASSERT_TRUE(test_view.is_pressed == 0);
    EGUI_TEST_ASSERT_TRUE(test_view.is_clickable == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_100, test_view.alpha);
    EGUI_TEST_ASSERT_NULL(test_view.parent);
    EGUI_TEST_ASSERT_NULL(test_view.background);
}

static void test_view_set_position(void)
{
    egui_view_init(&test_view);
    egui_view_set_position(&test_view, 10, 20);
    EGUI_TEST_ASSERT_EQUAL_INT(10, test_view.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(20, test_view.region.location.y);
}

static void test_view_set_size(void)
{
    egui_view_init(&test_view);
    egui_view_set_size(&test_view, 100, 200);
    EGUI_TEST_ASSERT_EQUAL_INT(100, test_view.region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(200, test_view.region.size.height);
}

static void test_view_visibility(void)
{
    egui_view_init(&test_view);

    // Default visible
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_get_visible(&test_view));

    // Set invisible
    egui_view_set_visible(&test_view, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_visible(&test_view));

    // Set visible again
    egui_view_set_visible(&test_view, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_get_visible(&test_view));
}

static void test_view_gone(void)
{
    egui_view_init(&test_view);

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_gone(&test_view));

    egui_view_set_gone(&test_view, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_get_gone(&test_view));

    egui_view_set_gone(&test_view, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_gone(&test_view));
}

static void test_view_enable(void)
{
    egui_view_init(&test_view);

    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_get_enable(&test_view));

    egui_view_set_enable(&test_view, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_enable(&test_view));
}

static void test_view_clickable(void)
{
    egui_view_init(&test_view);

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_clickable(&test_view));

    egui_view_set_clickable(&test_view, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_get_clickable(&test_view));
}

static void test_view_alpha(void)
{
    egui_view_init(&test_view);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_100, test_view.alpha);

    egui_view_set_alpha(&test_view, EGUI_ALPHA_50);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_50, test_view.alpha);
}

static void test_view_padding(void)
{
    egui_view_init(&test_view);

    egui_view_set_padding(&test_view, 1, 2, 3, 4);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_view.padding.left);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_view.padding.right);
    EGUI_TEST_ASSERT_EQUAL_INT(3, test_view.padding.top);
    EGUI_TEST_ASSERT_EQUAL_INT(4, test_view.padding.bottom);

    egui_view_set_padding_all(&test_view, 5);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.padding.left);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.padding.right);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.padding.top);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.padding.bottom);
}

static void test_view_margin(void)
{
    egui_view_init(&test_view);

    egui_view_set_margin(&test_view, 1, 2, 3, 4);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_view.margin.left);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_view.margin.right);
    EGUI_TEST_ASSERT_EQUAL_INT(3, test_view.margin.top);
    EGUI_TEST_ASSERT_EQUAL_INT(4, test_view.margin.bottom);

    egui_view_set_margin_all(&test_view, 5);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.margin.left);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.margin.right);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.margin.top);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.margin.bottom);
}

static void ensure_view_pressed_background(void)
{
    if (s_view_pressed_background_ready)
    {
        return;
    }

    egui_background_color_init_with_params((egui_background_t *)&s_view_pressed_background, &s_view_bg_params);
    s_view_pressed_background_ready = 1;
}

static void test_view_set_pressed_same_state_skips_dirty(void)
{
    egui_region_t region;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    egui_view_init(&test_view);
    egui_region_init(&region, 10, 20, 100, 40);
    egui_region_copy(&test_view.region, &region);
    egui_region_copy(&test_view.region_screen, &region);
    test_view.is_request_layout = 0;

    egui_core_clear_region_dirty();
    egui_view_set_pressed(&test_view, false);

    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[0]));
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

static void test_view_set_pressed_with_region_respects_background_pressed_state(void)
{
    egui_region_t region;
    egui_region_t dirty_region;
    egui_region_t expected;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    egui_view_init(&test_view);
    egui_region_init(&region, 10, 20, 100, 40);
    egui_region_copy(&test_view.region, &region);
    egui_region_copy(&test_view.region_screen, &region);
    test_view.is_request_layout = 0;

    egui_region_init(&dirty_region, 5, 6, 20, 10);

    egui_core_clear_region_dirty();
    egui_view_set_pressed_with_region(&test_view, true, &dirty_region);
    egui_region_init(&expected, 15, 26, 20, 10);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected, &arr[0]);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));

    egui_core_clear_region_dirty();
    egui_view_set_pressed_with_region(&test_view, false, &dirty_region);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected, &arr[0]);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));

    ensure_view_pressed_background();
    egui_view_set_background(&test_view, (egui_background_t *)&s_view_pressed_background.base);
    egui_core_clear_region_dirty();
    egui_view_set_pressed_with_region(&test_view, true, &dirty_region);
    EGUI_TEST_ASSERT_REGION_EQUAL(&test_view.region_screen, &arr[0]);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static void test_view_click_requires_release_inside(void)
{
    egui_view_init(&test_view);
    test_view_layout_rect(10, 20, 100, 40);
    egui_view_set_on_click_listener(&test_view, test_view_click_cb);
    g_view_click_count = 0;

    EGUI_TEST_ASSERT_TRUE(test_view_send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 20, 30));
    EGUI_TEST_ASSERT_TRUE(test_view.is_pressed);

    EGUI_TEST_ASSERT_TRUE(test_view_send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 160, 30));
    EGUI_TEST_ASSERT_FALSE(test_view.is_pressed);

    EGUI_TEST_ASSERT_TRUE(test_view_send_touch(EGUI_MOTION_EVENT_ACTION_UP, 160, 30));
    EGUI_TEST_ASSERT_FALSE(test_view.is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_view_click_count);

    EGUI_TEST_ASSERT_TRUE(test_view_send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 20, 30));
    EGUI_TEST_ASSERT_TRUE(test_view_send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 24, 34));
    EGUI_TEST_ASSERT_TRUE(test_view.is_pressed);
    EGUI_TEST_ASSERT_TRUE(test_view_send_touch(EGUI_MOTION_EVENT_ACTION_UP, 24, 34));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_view_click_count);
}
#endif

void test_view_run(void)
{
    EGUI_TEST_SUITE_BEGIN(view);

    EGUI_TEST_RUN(test_view_init_defaults);
    EGUI_TEST_RUN(test_view_set_position);
    EGUI_TEST_RUN(test_view_set_size);
    EGUI_TEST_RUN(test_view_visibility);
    EGUI_TEST_RUN(test_view_gone);
    EGUI_TEST_RUN(test_view_enable);
    EGUI_TEST_RUN(test_view_clickable);
    EGUI_TEST_RUN(test_view_alpha);
    EGUI_TEST_RUN(test_view_padding);
    EGUI_TEST_RUN(test_view_margin);
    EGUI_TEST_RUN(test_view_set_pressed_same_state_skips_dirty);
    EGUI_TEST_RUN(test_view_set_pressed_with_region_respects_background_pressed_state);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    EGUI_TEST_RUN(test_view_click_requires_release_inside);
#endif

    EGUI_TEST_SUITE_END();
}
