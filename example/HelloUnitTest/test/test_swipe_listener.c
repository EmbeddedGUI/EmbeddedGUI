#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "core/egui_core_internal.h"
#include "test/egui_test.h"
#include "test_swipe_listener.h"

#if EGUI_CONFIG_FUNCTION_SWIPE_LISTENER
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

/* ---- Test fixture ---- */

#define VIEW_X 0
#define VIEW_Y 0
#define VIEW_W 100
#define VIEW_H 100

static egui_view_t s_view;
static int s_swipe_count;
static egui_swipe_dir_t s_last_dir;
static int s_click_count;

static egui_core_t *get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

static void on_swipe(egui_view_t *self, egui_swipe_dir_t dir)
{
    EGUI_UNUSED(self);
    s_swipe_count++;
    s_last_dir = dir;
}

static void on_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    s_click_count++;
}

static void init_view(void)
{
    egui_core_t *core = get_core();

    s_swipe_count = 0;
    s_click_count = 0;
    s_last_dir = (egui_swipe_dir_t)(-1);

    egui_view_init(&s_view, core);
    s_view.core = core;
    s_view.region_screen.location.x = VIEW_X;
    s_view.region_screen.location.y = VIEW_Y;
    s_view.region_screen.size.width = VIEW_W;
    s_view.region_screen.size.height = VIEW_H;
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

/* ---- Tests ---- */

/* Default swipe listener is NULL. */
static void test_sw_default_null(void)
{
    init_view();
    EGUI_TEST_ASSERT_NULL(egui_view_get_on_swipe_listener(&s_view));
}

/* NULL-safe getter returns NULL. */
static void test_sw_null_safe_getter(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_get_on_swipe_listener(NULL));
}

/* Setting and getting swipe listener round-trips. */
static void test_sw_set_get(void)
{
    init_view();
    egui_view_set_on_swipe_listener(&s_view, on_swipe);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_on_swipe_listener(&s_view) == on_swipe);
    egui_view_set_on_swipe_listener(&s_view, NULL);
    EGUI_TEST_ASSERT_NULL(egui_view_get_on_swipe_listener(&s_view));
}

/* Short tap (within deadband) does NOT fire swipe. */
static void test_sw_short_tap_no_swipe(void)
{
    init_view();
    egui_view_set_on_swipe_listener(&s_view, on_swipe);
    send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 50, 50);
    /* Move only 5 px — below EGUI_CONFIG_SWIPE_MIN_DISPLACEMENT_PX */
    send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 55, 50);
    send_touch(EGUI_MOTION_EVENT_ACTION_UP, 55, 50);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_swipe_count);
}

/* Horizontal left swipe fires EGUI_SWIPE_DIR_LEFT. */
static void test_sw_swipe_left(void)
{
    init_view();
    egui_view_set_on_swipe_listener(&s_view, on_swipe);
    send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 80, 50);
    send_touch(EGUI_MOTION_EVENT_ACTION_UP, 30, 50); /* dx = -50, dy = 0 */
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_swipe_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_SWIPE_DIR_LEFT, (int)s_last_dir);
}

/* Horizontal right swipe fires EGUI_SWIPE_DIR_RIGHT. */
static void test_sw_swipe_right(void)
{
    init_view();
    egui_view_set_on_swipe_listener(&s_view, on_swipe);
    send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 10, 50);
    send_touch(EGUI_MOTION_EVENT_ACTION_UP, 60, 50); /* dx = +50, dy = 0 */
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_swipe_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_SWIPE_DIR_RIGHT, (int)s_last_dir);
}

/* Vertical up swipe fires EGUI_SWIPE_DIR_UP. */
static void test_sw_swipe_up(void)
{
    init_view();
    egui_view_set_on_swipe_listener(&s_view, on_swipe);
    send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 50, 80);
    send_touch(EGUI_MOTION_EVENT_ACTION_UP, 50, 30); /* dx = 0, dy = -50 */
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_swipe_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_SWIPE_DIR_UP, (int)s_last_dir);
}

/* Vertical down swipe fires EGUI_SWIPE_DIR_DOWN. */
static void test_sw_swipe_down(void)
{
    init_view();
    egui_view_set_on_swipe_listener(&s_view, on_swipe);
    send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 50, 10);
    send_touch(EGUI_MOTION_EVENT_ACTION_UP, 50, 60); /* dx = 0, dy = +50 */
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_swipe_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_SWIPE_DIR_DOWN, (int)s_last_dir);
}

/* Dominant axis determines direction for diagonal movement. */
static void test_sw_diagonal_dominant_axis(void)
{
    init_view();
    egui_view_set_on_swipe_listener(&s_view, on_swipe);
    /* Mostly horizontal: dx=50, dy=20 -> LEFT */
    send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 80, 50);
    send_touch(EGUI_MOTION_EVENT_ACTION_UP, 30, 70);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_swipe_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_SWIPE_DIR_LEFT, (int)s_last_dir);
}

/* Swipe with both click and swipe listeners: click fires normally on short tap. */
static void test_sw_click_and_swipe_coexist_click(void)
{
    init_view();
    egui_view_set_on_click_listener(&s_view, on_click);
    egui_view_set_on_swipe_listener(&s_view, on_swipe);
    send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 50, 50);
    send_touch(EGUI_MOTION_EVENT_ACTION_UP, 50, 50); /* no displacement */
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_click_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_swipe_count);
}

/* Swipe with both listeners: swipe fires and click suppressed by out-of-view UP. */
static void test_sw_swipe_no_click(void)
{
    init_view();
    egui_view_set_on_click_listener(&s_view, on_click);
    egui_view_set_on_swipe_listener(&s_view, on_swipe);
    send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 80, 50);
    /* Move outside view to cancel click (is_inside becomes false) */
    send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 150, 50);
    send_touch(EGUI_MOTION_EVENT_ACTION_UP, 150, 50);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_click_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_swipe_count);
}

#endif /* EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH */
#endif /* EGUI_CONFIG_FUNCTION_SWIPE_LISTENER */

/* ---- Suite runner ---- */

void test_swipe_listener_run(void)
{
#if EGUI_CONFIG_FUNCTION_SWIPE_LISTENER
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    EGUI_TEST_RUN(test_sw_default_null);
    EGUI_TEST_RUN(test_sw_null_safe_getter);
    EGUI_TEST_RUN(test_sw_set_get);
    EGUI_TEST_RUN(test_sw_short_tap_no_swipe);
    EGUI_TEST_RUN(test_sw_swipe_left);
    EGUI_TEST_RUN(test_sw_swipe_right);
    EGUI_TEST_RUN(test_sw_swipe_up);
    EGUI_TEST_RUN(test_sw_swipe_down);
    EGUI_TEST_RUN(test_sw_diagonal_dominant_axis);
    EGUI_TEST_RUN(test_sw_click_and_swipe_coexist_click);
    EGUI_TEST_RUN(test_sw_swipe_no_click);
#endif /* EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH */
#endif /* EGUI_CONFIG_FUNCTION_SWIPE_LISTENER */
}
