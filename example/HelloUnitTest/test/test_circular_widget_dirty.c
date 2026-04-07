#include <string.h>

#include "egui.h"
#include "background/egui_background_color.h"
#include "test/egui_test.h"
#include "test_circular_widget_dirty.h"

static egui_view_circular_progress_bar_t test_progress_bar;
static egui_view_gauge_t test_gauge;
static egui_view_arc_slider_t test_arc_slider;
static egui_view_analog_clock_t test_clock;
static egui_view_spinner_t test_spinner;

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_pressed_bg_normal_param, EGUI_COLOR_BLUE, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_pressed_bg_pressed_param, EGUI_COLOR_RED, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(s_pressed_bg_params, &s_pressed_bg_normal_param, &s_pressed_bg_pressed_param, NULL);
static egui_background_color_t s_pressed_background;
static uint8_t s_pressed_background_ready;

static int32_t region_area(const egui_region_t *region)
{
    return (int32_t)region->size.width * region->size.height;
}

static void collect_dirty_union(egui_region_t *out_region, uint8_t *out_count)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr();
    uint8_t count = 0;
    uint8_t i;

    egui_region_init_empty(out_region);
    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if (egui_region_is_empty(&arr[i]))
        {
            continue;
        }

        if (count == 0)
        {
            egui_region_copy(out_region, &arr[i]);
        }
        else
        {
            egui_region_union(out_region, &arr[i], out_region);
        }
        count++;
    }

    if (out_count != NULL)
    {
        *out_count = count;
    }
}

static uint8_t region_contains_point(const egui_region_t *region, egui_dim_t x, egui_dim_t y)
{
    if (egui_region_is_empty((egui_region_t *)region))
    {
        return 0;
    }

    if (x < region->location.x || y < region->location.y)
    {
        return 0;
    }

    if (x >= region->location.x + region->size.width || y >= region->location.y + region->size.height)
    {
        return 0;
    }

    return 1;
}

static void setup_circular_progress_bar(void)
{
    egui_region_t region;

    egui_view_circular_progress_bar_init(EGUI_VIEW_OF(&test_progress_bar));
    egui_view_set_size(EGUI_VIEW_OF(&test_progress_bar), 120, 120);
    egui_view_circular_progress_bar_set_stroke_width(EGUI_VIEW_OF(&test_progress_bar), 12);

    egui_region_init(&region, 10, 20, 120, 120);
    egui_view_layout(EGUI_VIEW_OF(&test_progress_bar), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_progress_bar)->region_screen, &region);
}

static void setup_gauge(void)
{
    egui_region_t region;

    egui_view_gauge_init(EGUI_VIEW_OF(&test_gauge));
    egui_view_set_size(EGUI_VIEW_OF(&test_gauge), 120, 120);

    egui_region_init(&region, 10, 20, 120, 120);
    egui_view_layout(EGUI_VIEW_OF(&test_gauge), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_gauge)->region_screen, &region);
}

static void setup_arc_slider(void)
{
    egui_region_t region;

    egui_view_arc_slider_init(EGUI_VIEW_OF(&test_arc_slider));
    egui_view_set_size(EGUI_VIEW_OF(&test_arc_slider), 120, 120);

    egui_region_init(&region, 10, 20, 120, 120);
    egui_view_layout(EGUI_VIEW_OF(&test_arc_slider), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_arc_slider)->region_screen, &region);
}

static void setup_analog_clock(void)
{
    egui_region_t region;

    egui_view_analog_clock_init(EGUI_VIEW_OF(&test_clock));
    egui_view_set_size(EGUI_VIEW_OF(&test_clock), 120, 120);

    egui_region_init(&region, 10, 20, 120, 120);
    egui_view_layout(EGUI_VIEW_OF(&test_clock), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_clock)->region_screen, &region);
}

static void setup_spinner(void)
{
    egui_region_t region;

    egui_view_spinner_init(EGUI_VIEW_OF(&test_spinner));
    egui_view_set_size(EGUI_VIEW_OF(&test_spinner), 120, 120);

    egui_region_init(&region, 10, 20, 120, 120);
    egui_view_layout(EGUI_VIEW_OF(&test_spinner), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_spinner)->region_screen, &region);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static void send_arc_slider_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;

    EGUI_VIEW_OF(&test_arc_slider)->api->on_touch_event(EGUI_VIEW_OF(&test_arc_slider), &event);
}

static void get_arc_slider_thumb_center(egui_dim_t *x, egui_dim_t *y)
{
    egui_dim_t center_x = EGUI_VIEW_OF(&test_arc_slider)->region_screen.location.x + EGUI_VIEW_OF(&test_arc_slider)->region.size.width / 2;
    egui_dim_t center_y = EGUI_VIEW_OF(&test_arc_slider)->region_screen.location.y + EGUI_VIEW_OF(&test_arc_slider)->region.size.height / 2;
    egui_dim_t radius = EGUI_MIN(EGUI_VIEW_OF(&test_arc_slider)->region.size.width, EGUI_VIEW_OF(&test_arc_slider)->region.size.height) / 2 -
                        test_arc_slider.thumb_radius - 1;
    egui_dim_t thumb_track_radius = radius - test_arc_slider.stroke_width / 2;

    *x = center_x;
    *y = center_y - thumb_track_radius;
}
#endif

static void assert_partial_dirty_region(const egui_view_t *view)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr();
    int32_t dirty_area = region_area(&arr[0]);
    int32_t full_area = region_area(&view->region_screen);

    EGUI_TEST_ASSERT_FALSE(egui_region_is_empty(&arr[0]));
    EGUI_TEST_ASSERT_FALSE(egui_region_equal(&arr[0], &view->region_screen));
    EGUI_TEST_ASSERT_TRUE(dirty_area < full_area);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

static void assert_full_dirty_region(const egui_view_t *view)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    EGUI_TEST_ASSERT_REGION_EQUAL(&view->region_screen, &arr[0]);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

static void attach_pressed_background(egui_view_t *view)
{
    if (!s_pressed_background_ready)
    {
        egui_background_color_init_with_params((egui_background_t *)&s_pressed_background, &s_pressed_bg_params);
        s_pressed_background_ready = 1;
    }

    egui_view_set_background(view, (egui_background_t *)&s_pressed_background.base);
}

static void test_circular_progress_bar_process_change_uses_partial_dirty_region(void)
{
    egui_region_t dirty_union;
    egui_region_t *arr = egui_core_get_region_dirty_arr();
    egui_dim_t center_x;
    egui_dim_t center_y;
    int32_t full_area;
    uint8_t dirty_count;

    setup_circular_progress_bar();
    egui_view_circular_progress_bar_set_process(EGUI_VIEW_OF(&test_progress_bar), 75);

    egui_core_clear_region_dirty();
    egui_view_circular_progress_bar_set_process(EGUI_VIEW_OF(&test_progress_bar), 80);

    collect_dirty_union(&dirty_union, &dirty_count);
    center_x = EGUI_VIEW_OF(&test_progress_bar)->region_screen.location.x + EGUI_VIEW_OF(&test_progress_bar)->region_screen.size.width / 2;
    center_y = EGUI_VIEW_OF(&test_progress_bar)->region_screen.location.y + EGUI_VIEW_OF(&test_progress_bar)->region_screen.size.height / 2;
    full_area = region_area(&EGUI_VIEW_OF(&test_progress_bar)->region_screen);

    EGUI_TEST_ASSERT_TRUE(dirty_count >= 1 && dirty_count <= 2);
    EGUI_TEST_ASSERT_FALSE(egui_region_is_empty(&arr[0]));
    EGUI_TEST_ASSERT_FALSE(egui_region_is_empty(&dirty_union));
    EGUI_TEST_ASSERT_TRUE(region_contains_point(&dirty_union, center_x, center_y));
    EGUI_TEST_ASSERT_FALSE(egui_region_equal(&dirty_union, &EGUI_VIEW_OF(&test_progress_bar)->region_screen));
    EGUI_TEST_ASSERT_TRUE(region_area(&dirty_union) < full_area / 3);
}

static void test_circular_progress_bar_repeated_process_change_same_frame_falls_back_to_full_dirty_region(void)
{
    setup_circular_progress_bar();
    egui_view_circular_progress_bar_set_process(EGUI_VIEW_OF(&test_progress_bar), 75);

    egui_core_clear_region_dirty();
    egui_view_circular_progress_bar_set_process(EGUI_VIEW_OF(&test_progress_bar), 80);
    egui_view_circular_progress_bar_set_process(EGUI_VIEW_OF(&test_progress_bar), 25);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_progress_bar));
}

static void test_gauge_value_change_uses_partial_dirty_region(void)
{
    egui_region_t dirty_union;
    egui_region_t *arr = egui_core_get_region_dirty_arr();
    egui_dim_t probe_x;
    egui_dim_t probe_y;
    int32_t full_area;
    uint8_t dirty_count;

    setup_gauge();
    egui_view_gauge_set_value(EGUI_VIEW_OF(&test_gauge), 35);

    egui_core_clear_region_dirty();
    egui_view_gauge_set_value(EGUI_VIEW_OF(&test_gauge), 60);

    collect_dirty_union(&dirty_union, &dirty_count);
    probe_x = EGUI_VIEW_OF(&test_gauge)->region_screen.location.x + EGUI_VIEW_OF(&test_gauge)->region_screen.size.width / 2;
    probe_y = EGUI_VIEW_OF(&test_gauge)->region_screen.location.y + EGUI_VIEW_OF(&test_gauge)->region_screen.size.height / 2 + 10;
    full_area = region_area(&EGUI_VIEW_OF(&test_gauge)->region_screen);

    EGUI_TEST_ASSERT_TRUE(dirty_count >= 1 && dirty_count <= 2);
    EGUI_TEST_ASSERT_FALSE(egui_region_is_empty(&arr[0]));
    EGUI_TEST_ASSERT_FALSE(egui_region_is_empty(&dirty_union));
    EGUI_TEST_ASSERT_TRUE(region_contains_point(&dirty_union, probe_x, probe_y));
    EGUI_TEST_ASSERT_FALSE(egui_region_equal(&dirty_union, &EGUI_VIEW_OF(&test_gauge)->region_screen));
    EGUI_TEST_ASSERT_TRUE(region_area(&dirty_union) < full_area * 3 / 4);
}

static void test_gauge_repeated_value_change_same_frame_falls_back_to_full_dirty_region(void)
{
    setup_gauge();
    egui_view_gauge_set_value(EGUI_VIEW_OF(&test_gauge), 35);

    egui_core_clear_region_dirty();
    egui_view_gauge_set_value(EGUI_VIEW_OF(&test_gauge), 60);
    egui_view_gauge_set_value(EGUI_VIEW_OF(&test_gauge), 20);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_gauge));
}

static void test_arc_slider_value_change_uses_partial_dirty_region(void)
{
    setup_arc_slider();
    egui_view_arc_slider_set_value(EGUI_VIEW_OF(&test_arc_slider), 40);

    egui_core_clear_region_dirty();
    egui_view_arc_slider_set_value(EGUI_VIEW_OF(&test_arc_slider), 55);

    assert_partial_dirty_region(EGUI_VIEW_OF(&test_arc_slider));
}

static void test_arc_slider_repeated_value_change_same_frame_falls_back_to_full_dirty_region(void)
{
    setup_arc_slider();
    egui_view_arc_slider_set_value(EGUI_VIEW_OF(&test_arc_slider), 40);

    egui_core_clear_region_dirty();
    egui_view_arc_slider_set_value(EGUI_VIEW_OF(&test_arc_slider), 55);
    egui_view_arc_slider_set_value(EGUI_VIEW_OF(&test_arc_slider), 15);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_arc_slider));
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static void test_arc_slider_touch_down_same_value_skips_dirty_region(void)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr();
    egui_dim_t touch_x;
    egui_dim_t touch_y;

    setup_arc_slider();
    egui_view_arc_slider_set_value(EGUI_VIEW_OF(&test_arc_slider), 50);
    get_arc_slider_thumb_center(&touch_x, &touch_y);

    egui_core_clear_region_dirty();
    send_arc_slider_touch(EGUI_MOTION_EVENT_ACTION_DOWN, touch_x, touch_y);

    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[0]));
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

static void test_arc_slider_touch_up_same_value_skips_dirty_region(void)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr();
    egui_dim_t touch_x;
    egui_dim_t touch_y;

    setup_arc_slider();
    egui_view_arc_slider_set_value(EGUI_VIEW_OF(&test_arc_slider), 50);
    get_arc_slider_thumb_center(&touch_x, &touch_y);
    send_arc_slider_touch(EGUI_MOTION_EVENT_ACTION_DOWN, touch_x, touch_y);

    egui_core_clear_region_dirty();
    send_arc_slider_touch(EGUI_MOTION_EVENT_ACTION_UP, touch_x, touch_y);

    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[0]));
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

static void test_arc_slider_touch_down_with_pressed_background_uses_full_dirty_region(void)
{
    egui_dim_t touch_x;
    egui_dim_t touch_y;

    setup_arc_slider();
    attach_pressed_background(EGUI_VIEW_OF(&test_arc_slider));
    egui_view_arc_slider_set_value(EGUI_VIEW_OF(&test_arc_slider), 50);
    get_arc_slider_thumb_center(&touch_x, &touch_y);

    egui_core_clear_region_dirty();
    send_arc_slider_touch(EGUI_MOTION_EVENT_ACTION_DOWN, touch_x, touch_y);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_arc_slider));
}
#endif

static void test_analog_clock_second_tick_uses_partial_dirty_region(void)
{
    setup_analog_clock();
    egui_view_analog_clock_set_time(EGUI_VIEW_OF(&test_clock), 3, 15, 0);

    egui_core_clear_region_dirty();
    egui_view_analog_clock_set_time(EGUI_VIEW_OF(&test_clock), 3, 15, 1);

    assert_partial_dirty_region(EGUI_VIEW_OF(&test_clock));
}

static void test_analog_clock_repeated_time_change_same_frame_falls_back_to_full_dirty_region(void)
{
    setup_analog_clock();
    egui_view_analog_clock_set_time(EGUI_VIEW_OF(&test_clock), 3, 15, 0);

    egui_core_clear_region_dirty();
    egui_view_analog_clock_set_time(EGUI_VIEW_OF(&test_clock), 3, 15, 1);
    egui_view_analog_clock_set_time(EGUI_VIEW_OF(&test_clock), 3, 15, 2);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_clock));
}

static void test_analog_clock_hidden_second_change_skips_dirty_region(void)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    setup_analog_clock();
    egui_view_analog_clock_set_time(EGUI_VIEW_OF(&test_clock), 3, 15, 0);
    egui_view_analog_clock_show_second(EGUI_VIEW_OF(&test_clock), 0);

    egui_core_clear_region_dirty();
    egui_view_analog_clock_set_time(EGUI_VIEW_OF(&test_clock), 3, 15, 1);

    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[0]));
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

static void test_spinner_rotation_step_uses_partial_dirty_region(void)
{
    setup_spinner();

    egui_core_clear_region_dirty();
    test_spinner.spin_timer.callback(&test_spinner.spin_timer);

    assert_partial_dirty_region(EGUI_VIEW_OF(&test_spinner));
}

static void test_spinner_repeated_rotation_same_frame_falls_back_to_full_dirty_region(void)
{
    setup_spinner();

    egui_core_clear_region_dirty();
    test_spinner.spin_timer.callback(&test_spinner.spin_timer);
    test_spinner.spin_timer.callback(&test_spinner.spin_timer);

    assert_full_dirty_region(EGUI_VIEW_OF(&test_spinner));
}

void test_circular_widget_dirty_run(void)
{
    EGUI_TEST_SUITE_BEGIN(circular_widget_dirty);
    EGUI_TEST_RUN(test_circular_progress_bar_process_change_uses_partial_dirty_region);
    EGUI_TEST_RUN(test_circular_progress_bar_repeated_process_change_same_frame_falls_back_to_full_dirty_region);
    EGUI_TEST_RUN(test_gauge_value_change_uses_partial_dirty_region);
    EGUI_TEST_RUN(test_gauge_repeated_value_change_same_frame_falls_back_to_full_dirty_region);
    EGUI_TEST_RUN(test_arc_slider_value_change_uses_partial_dirty_region);
    EGUI_TEST_RUN(test_arc_slider_repeated_value_change_same_frame_falls_back_to_full_dirty_region);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    EGUI_TEST_RUN(test_arc_slider_touch_down_same_value_skips_dirty_region);
    EGUI_TEST_RUN(test_arc_slider_touch_up_same_value_skips_dirty_region);
    EGUI_TEST_RUN(test_arc_slider_touch_down_with_pressed_background_uses_full_dirty_region);
#endif
    EGUI_TEST_RUN(test_analog_clock_second_tick_uses_partial_dirty_region);
    EGUI_TEST_RUN(test_analog_clock_repeated_time_change_same_frame_falls_back_to_full_dirty_region);
    EGUI_TEST_RUN(test_analog_clock_hidden_second_change_skips_dirty_region);
    EGUI_TEST_RUN(test_spinner_rotation_step_uses_partial_dirty_region);
    EGUI_TEST_RUN(test_spinner_repeated_rotation_same_frame_falls_back_to_full_dirty_region);
    EGUI_TEST_SUITE_END();
}
