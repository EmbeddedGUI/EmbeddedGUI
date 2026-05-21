#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "core/egui_core_internal.h"
#include "core/egui_timer.h"
#include "test/egui_test.h"
#include "test_long_press.h"

#if EGUI_CONFIG_FUNCTION_LONG_PRESS && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

static egui_view_t s_view;
static int s_click_count;
static int s_long_press_count;

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

static void on_long_press(egui_view_t *self)
{
    EGUI_UNUSED(self);
    s_long_press_count++;
}

static void init_view(void)
{
    egui_core_t *core = get_core();

    s_click_count = 0;
    s_long_press_count = 0;
    egui_view_init(&s_view, core);
    s_view.core = core;
    s_view.is_clickable = 1;
    s_view.region_screen.location.x = 0;
    s_view.region_screen.location.y = 0;
    s_view.region_screen.size.width = 100;
    s_view.region_screen.size.height = 100;
    egui_view_set_on_click_listener(&s_view, on_click);
    egui_view_set_on_long_press_listener(&s_view, on_long_press);
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

/* Simulate the long-press threshold elapsing by backdating _lp_press_tick. */
static void simulate_hold_expired(void)
{
    /* Backdate unconditionally; uint32 wraparound arithmetic handles t=0 correctly. */
    s_view._lp_press_tick = egui_timer_get_current_time() - EGUI_CONFIG_LONG_PRESS_DURATION_MS - 1;
}

/* ---- Tests ---- */

/* Getter returns NULL by default; setting and getting round-trips. */
static void test_lp_getter_setter(void)
{
    init_view();
    EGUI_TEST_ASSERT_NULL(egui_view_get_on_long_press_listener(NULL));
    EGUI_TEST_ASSERT_TRUE(egui_view_get_on_long_press_listener(&s_view) == on_long_press);
    egui_view_set_on_long_press_listener(&s_view, NULL);
    EGUI_TEST_ASSERT_NULL(egui_view_get_on_long_press_listener(&s_view));
}

/* After DOWN + quick UP the click fires and long-press does not. */
static void test_lp_short_tap_fires_click_not_long_press(void)
{
    init_view();
    send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 50, 50);
    /* Do NOT backdate the tick — this simulates a fast tap. */
    send_touch(EGUI_MOTION_EVENT_ACTION_UP, 50, 50);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_click_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_long_press_count);
}

/* After DOWN + threshold expired + poll: long-press fires, click does not fire on UP. */
static void test_lp_hold_fires_long_press_and_suppresses_click(void)
{
    init_view();
    send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 50, 50);
    simulate_hold_expired();
    egui_view_poll_long_press(&s_view);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_long_press_count);
    send_touch(EGUI_MOTION_EVENT_ACTION_UP, 50, 50);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_click_count); /* click suppressed */
}

/* egui_view_poll_long_press is idempotent: fires only once even if called many times. */
static void test_lp_poll_fires_only_once(void)
{
    init_view();
    send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 50, 50);
    simulate_hold_expired();
    egui_view_poll_long_press(&s_view);
    egui_view_poll_long_press(&s_view);
    egui_view_poll_long_press(&s_view);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_long_press_count);
}

/* Sliding the finger out cancels the pending long-press. */
static void test_lp_slide_out_cancels(void)
{
    init_view();
    send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 50, 50);
    /* Slide out of the view bounds */
    send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 200, 200);
    simulate_hold_expired();
    egui_view_poll_long_press(&s_view);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_long_press_count);
}

/* CANCEL event resets state so long-press does not fire. */
static void test_lp_cancel_resets_state(void)
{
    init_view();
    send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 50, 50);
    send_touch(EGUI_MOTION_EVENT_ACTION_CANCEL, 50, 50);
    simulate_hold_expired();
    egui_view_poll_long_press(&s_view);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_long_press_count);
}

/* No long-press listener installed: poll is a no-op, click still fires normally. */
static void test_lp_no_listener_click_still_works(void)
{
    init_view();
    egui_view_set_on_long_press_listener(&s_view, NULL);
    send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 50, 50);
    simulate_hold_expired();
    egui_view_poll_long_press(&s_view);
    send_touch(EGUI_MOTION_EVENT_ACTION_UP, 50, 50);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_click_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_long_press_count);
}

/* poll_long_press on NULL view is safe. */
static void test_lp_poll_null_safe(void)
{
    egui_view_poll_long_press(NULL);
    /* no crash */
}

#endif /* EGUI_CONFIG_FUNCTION_LONG_PRESS && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH */

void test_long_press_run(void)
{
#if EGUI_CONFIG_FUNCTION_LONG_PRESS && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    EGUI_TEST_RUN(test_lp_getter_setter);
    EGUI_TEST_RUN(test_lp_short_tap_fires_click_not_long_press);
    EGUI_TEST_RUN(test_lp_hold_fires_long_press_and_suppresses_click);
    EGUI_TEST_RUN(test_lp_poll_fires_only_once);
    EGUI_TEST_RUN(test_lp_slide_out_cancels);
    EGUI_TEST_RUN(test_lp_cancel_resets_state);
    EGUI_TEST_RUN(test_lp_no_listener_click_still_works);
    EGUI_TEST_RUN(test_lp_poll_null_safe);
#endif
}
