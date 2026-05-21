#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "core/egui_core_internal.h"
#include "test/egui_test.h"
#include "test_ext_click_area.h"

#if EGUI_CONFIG_FUNCTION_EXT_CLICK_AREA

static egui_view_t s_view;
static int s_click_count;

static egui_core_t *get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

static void on_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    s_click_count++;
}

static void init_view(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    egui_core_t *core = get_core();

    s_click_count = 0;
    egui_view_init(&s_view, core);
    s_view.core = core;
    s_view.is_clickable = 1;
    s_view.region_screen.location.x = x;
    s_view.region_screen.location.y = y;
    s_view.region_screen.size.width = w;
    s_view.region_screen.size.height = h;
    egui_view_set_on_click_listener(&s_view, on_click);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return s_view.api->dispatch_touch_event(&s_view, &event);
}

static void simulate_click(egui_dim_t x, egui_dim_t y)
{
    send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y);
    send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y);
}

/* Default getter returns 0 after init. */
static void test_ext_click_area_default_zero(void)
{
    init_view(10, 10, 40, 40);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_ext_click_area(&s_view));
}

/* Setter / getter round-trip. */
static void test_ext_click_area_set_get(void)
{
    init_view(10, 10, 40, 40);
    egui_view_set_ext_click_area(&s_view, 8);
    EGUI_TEST_ASSERT_EQUAL_INT(8, egui_view_get_ext_click_area(&s_view));
    egui_view_set_ext_click_area(&s_view, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_ext_click_area(&s_view));
}

/* Without ext_click_area a touch just outside the view does not trigger a click. */
static void test_ext_click_area_no_ext_outside_miss(void)
{
    init_view(10, 10, 40, 40);
    /* point is 1 pixel to the right of the right edge */
    simulate_click(50, 25);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_click_count);
}

/* With ext_click_area the same touch now hits and fires the click listener. */
static void test_ext_click_area_ext_expands_hit_right(void)
{
    init_view(10, 10, 40, 40);
    egui_view_set_ext_click_area(&s_view, 5);
    /* 1 pixel outside the right edge — within the 5-pixel extension */
    simulate_click(50, 25);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_click_count);
}

/* Extension applies to all four sides: left, top, bottom. */
static void test_ext_click_area_ext_all_sides(void)
{
    /* left edge */
    init_view(10, 10, 40, 40);
    egui_view_set_ext_click_area(&s_view, 5);
    simulate_click(5, 25);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_click_count);

    /* top edge */
    init_view(10, 10, 40, 40);
    egui_view_set_ext_click_area(&s_view, 5);
    simulate_click(25, 5);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_click_count);

    /* bottom edge */
    init_view(10, 10, 40, 40);
    egui_view_set_ext_click_area(&s_view, 5);
    simulate_click(25, 54);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_click_count);
}

/* A point outside the extension still misses. */
static void test_ext_click_area_ext_outside_still_miss(void)
{
    init_view(10, 10, 40, 40);
    egui_view_set_ext_click_area(&s_view, 5);
    /* 6 pixels beyond right edge — just outside the 5-pixel extension */
    simulate_click(56, 25);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_click_count);
}

/* A touch inside the visual bounds always hits regardless of ext value. */
static void test_ext_click_area_inside_always_hit(void)
{
    init_view(10, 10, 40, 40);
    simulate_click(30, 30);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_click_count);
}

#endif /* EGUI_CONFIG_FUNCTION_EXT_CLICK_AREA */

void test_ext_click_area_run(void)
{
#if EGUI_CONFIG_FUNCTION_EXT_CLICK_AREA
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    EGUI_TEST_RUN(test_ext_click_area_default_zero);
    EGUI_TEST_RUN(test_ext_click_area_set_get);
    EGUI_TEST_RUN(test_ext_click_area_no_ext_outside_miss);
    EGUI_TEST_RUN(test_ext_click_area_ext_expands_hit_right);
    EGUI_TEST_RUN(test_ext_click_area_ext_all_sides);
    EGUI_TEST_RUN(test_ext_click_area_ext_outside_still_miss);
    EGUI_TEST_RUN(test_ext_click_area_inside_always_hit);
#endif /* EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH */
#endif /* EGUI_CONFIG_FUNCTION_EXT_CLICK_AREA */
}
