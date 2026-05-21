#include <string.h>

#include "egui.h"
#include "core/egui_timer.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_view_periodic.h"

static egui_view_t s_view;
static egui_view_group_t s_parent;
static egui_timer_t s_timer;
static int s_timer_fired;

static void on_timer_cb(egui_timer_t *t)
{
    EGUI_UNUSED(t);
    s_timer_fired++;
}

static egui_core_t *get_core(void)
{
    egui_core_t *core = uicode_get_core();
    EGUI_ASSERT(core != NULL);
    return core;
}

static void setup_view(void)
{
    egui_core_t *core = get_core();

    s_timer_fired = 0;
    memset(&s_parent, 0, sizeof(s_parent));
    memset(&s_view, 0, sizeof(s_view));
    egui_view_group_init(EGUI_VIEW_OF(&s_parent), core);
    egui_view_init(&s_view, core);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_parent), &s_view);
}

/* start_periodic registers the timer in the core. */
static void test_periodic_start_registers_timer(void)
{
    egui_core_t *core = get_core();
    setup_view();
    egui_view_start_periodic(&s_view, &s_timer, NULL, on_timer_cb, 100);
    EGUI_TEST_ASSERT_TRUE(egui_timer_check_timer_start(core, &s_timer));
    egui_view_stop_periodic(&s_view, &s_timer);
}

/* stop_periodic unregisters the timer. */
static void test_periodic_stop_unregisters_timer(void)
{
    egui_core_t *core = get_core();
    setup_view();
    egui_view_start_periodic(&s_view, &s_timer, NULL, on_timer_cb, 100);
    egui_view_stop_periodic(&s_view, &s_timer);
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(core, &s_timer));
}

/* NULL view: start_periodic does not crash. */
static void test_periodic_null_view_no_crash(void)
{
    egui_view_start_periodic(NULL, &s_timer, NULL, on_timer_cb, 100);
    EGUI_TEST_ASSERT_TRUE(1); /* no crash */
}

/* NULL timer: start_periodic does not crash. */
static void test_periodic_null_timer_no_crash(void)
{
    setup_view();
    egui_view_start_periodic(&s_view, NULL, NULL, on_timer_cb, 100);
    EGUI_TEST_ASSERT_TRUE(1); /* no crash */
}

/* NULL callback: start_periodic does not crash. */
static void test_periodic_null_callback_no_crash(void)
{
    setup_view();
    egui_view_start_periodic(&s_view, &s_timer, NULL, NULL, 100);
    EGUI_TEST_ASSERT_TRUE(1); /* no crash */
}

/* NULL view: stop_periodic does not crash. */
static void test_periodic_stop_null_no_crash(void)
{
    egui_view_stop_periodic(NULL, &s_timer);
    EGUI_TEST_ASSERT_TRUE(1); /* no crash */
}

void test_view_periodic_run(void)
{
    EGUI_TEST_SUITE_BEGIN(view_periodic);

    EGUI_TEST_RUN(test_periodic_start_registers_timer);
    EGUI_TEST_RUN(test_periodic_stop_unregisters_timer);
    EGUI_TEST_RUN(test_periodic_null_view_no_crash);
    EGUI_TEST_RUN(test_periodic_null_timer_no_crash);
    EGUI_TEST_RUN(test_periodic_null_callback_no_crash);
    EGUI_TEST_RUN(test_periodic_stop_null_no_crash);

    EGUI_TEST_SUITE_END();
}
