#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "core/egui_core_internal.h"
#include "test/egui_test.h"
#include "test_draggable_view.h"

#if EGUI_CONFIG_FUNCTION_DRAGGABLE_VIEW && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

static egui_view_t s_view;

static egui_core_t *get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

static void init_view(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    egui_core_t *core = get_core();

    egui_view_init(&s_view, core);
    s_view.core = core;
    s_view.region.location.x = x;
    s_view.region.location.y = y;
    s_view.region.size.width = w;
    s_view.region.size.height = h;
    s_view.region_screen.location.x = x;
    s_view.region_screen.location.y = y;
    s_view.region_screen.size.width = w;
    s_view.region_screen.size.height = h;
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

/* is_draggable defaults to 0 after init. */
static void test_drag_default_off(void)
{
    init_view(0, 0, 50, 50);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_draggable(&s_view));
}

/* NULL-safe getter returns 0. */
static void test_drag_null_safe(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_draggable(NULL));
}

/* Set/get round-trip. */
static void test_drag_set_get(void)
{
    init_view(0, 0, 50, 50);
    egui_view_set_draggable(&s_view, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_get_draggable(&s_view));
    egui_view_set_draggable(&s_view, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_draggable(&s_view));
}

/* When draggable=0, MOVE events do not change the region location. */
static void test_drag_off_no_move(void)
{
    init_view(10, 20, 50, 50);
    /* not draggable — DOWN is not consumed, so MOVE won't route here via capture path,
       but send both to exercise the path */
    send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 30, 40);
    send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 50, 60);
    EGUI_TEST_ASSERT_EQUAL_INT(10, s_view.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(20, s_view.region.location.y);
}

/* When draggable=1, a MOVE after DOWN shifts the region by the delta. */
static void test_drag_move_updates_region(void)
{
    init_view(10, 20, 50, 50);
    egui_view_set_draggable(&s_view, 1);
    /* DOWN at (30, 40) — inside the 10,20 + 50×50 view */
    send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 30, 40);
    /* MOVE to (35, 45) — delta (+5, +5) */
    send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 35, 45);
    EGUI_TEST_ASSERT_EQUAL_INT(15, s_view.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(25, s_view.region.location.y);
}

/* Multiple consecutive MOVEs accumulate correctly. */
static void test_drag_cumulative_moves(void)
{
    init_view(0, 0, 60, 60);
    egui_view_set_draggable(&s_view, 1);
    send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 10, 10);
    send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 15, 10); /* dx=+5, dy=0  → (5,0) */
    send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 15, 20); /* dx=0,  dy=+10 → (5,10) */
    send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 5, 20);  /* dx=-10,dy=0  → (-5,10) */
    EGUI_TEST_ASSERT_EQUAL_INT(-5, s_view.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(10, s_view.region.location.y);
}

/* DOWN outside the view does not start a drag (returns 0, position unchanged). */
static void test_drag_down_outside_no_drag(void)
{
    init_view(10, 10, 40, 40);
    egui_view_set_draggable(&s_view, 1);
    int consumed = send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 100, 100); /* outside */
    EGUI_TEST_ASSERT_EQUAL_INT(0, consumed);
    /* MOVE should not translate since DOWN was outside */
    send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 110, 110);
    EGUI_TEST_ASSERT_EQUAL_INT(10, s_view.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(10, s_view.region.location.y);
}

#endif /* EGUI_CONFIG_FUNCTION_DRAGGABLE_VIEW && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH */

void test_draggable_view_run(void)
{
#if EGUI_CONFIG_FUNCTION_DRAGGABLE_VIEW && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    EGUI_TEST_RUN(test_drag_default_off);
    EGUI_TEST_RUN(test_drag_null_safe);
    EGUI_TEST_RUN(test_drag_set_get);
    EGUI_TEST_RUN(test_drag_off_no_move);
    EGUI_TEST_RUN(test_drag_move_updates_region);
    EGUI_TEST_RUN(test_drag_cumulative_moves);
    EGUI_TEST_RUN(test_drag_down_outside_no_drag);
#endif
}
