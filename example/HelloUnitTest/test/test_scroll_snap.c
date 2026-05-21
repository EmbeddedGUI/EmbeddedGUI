#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_scroll_snap.h"

#if EGUI_CONFIG_FUNCTION_SCROLL_SNAP

/* ---- Test fixture ---- */

static egui_view_scroll_t s_scroll;
static egui_view_label_t  s_item;

static egui_core_t *get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

static void setup(void)
{
    egui_core_t *core = get_core();

    memset(&s_scroll, 0, sizeof(s_scroll));
    memset(&s_item,   0, sizeof(s_item));

    egui_view_scroll_init(EGUI_VIEW_OF(&s_scroll), core);
    egui_view_set_size(EGUI_VIEW_OF(&s_scroll), 100, 100);

    /* Lay out the scroll view so region_screen is populated. */
    egui_region_t r;
    egui_region_init(&r, 0, 0, 100, 100);
    egui_view_layout(EGUI_VIEW_OF(&s_scroll), &r);
    egui_region_copy(&EGUI_VIEW_OF(&s_scroll)->region_screen, &r);
}

/* ---- Tests ---- */

/* Default snap interval is 0 (disabled). */
static void test_snap_default_zero(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_scroll_get_snap_interval_y(EGUI_VIEW_OF(&s_scroll)));
}

/* NULL-safe getter returns 0. */
static void test_snap_null_safe(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_scroll_get_snap_interval_y(NULL));
}

/* Set/get round-trip. */
static void test_snap_set_get(void)
{
    setup();
    egui_view_scroll_set_snap_interval_y(EGUI_VIEW_OF(&s_scroll), 50);
    EGUI_TEST_ASSERT_EQUAL_INT(50, egui_view_scroll_get_snap_interval_y(EGUI_VIEW_OF(&s_scroll)));
}

/* Setting to 0 disables snap. */
static void test_snap_set_zero_disables(void)
{
    setup();
    egui_view_scroll_set_snap_interval_y(EGUI_VIEW_OF(&s_scroll), 100);
    egui_view_scroll_set_snap_interval_y(EGUI_VIEW_OF(&s_scroll), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_scroll_get_snap_interval_y(EGUI_VIEW_OF(&s_scroll)));
}

/* With snap = 0, fling path is used (scroller stays finished after UP without drag). */
static void test_snap_zero_no_animation_started(void)
{
    setup();
    egui_view_scroll_set_snap_interval_y(EGUI_VIEW_OF(&s_scroll), 0);
    /* No drag happened — scroller should remain finished (idle). */
    EGUI_TEST_ASSERT_TRUE(s_scroll.scroller.finished);
}

#endif /* EGUI_CONFIG_FUNCTION_SCROLL_SNAP */

/* ---- Suite runner ---- */

void test_scroll_snap_run(void)
{
#if EGUI_CONFIG_FUNCTION_SCROLL_SNAP
    EGUI_TEST_RUN(test_snap_default_zero);
    EGUI_TEST_RUN(test_snap_null_safe);
    EGUI_TEST_RUN(test_snap_set_get);
    EGUI_TEST_RUN(test_snap_set_zero_disables);
    EGUI_TEST_RUN(test_snap_zero_no_animation_started);
#endif /* EGUI_CONFIG_FUNCTION_SCROLL_SNAP */
}
