#include <string.h>

#include "egui.h"
#include "widget/egui_view_arc_slider.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_arc_slider_get_is_dragging.h"

static egui_view_arc_slider_t s_arc;

static void setup(void)
{
    memset(&s_arc, 0, sizeof(s_arc));
    egui_view_arc_slider_init(EGUI_VIEW_OF(&s_arc), uicode_get_core());
}

/* Default dragging state after init is 0. */
static void test_arc_slider_get_is_dragging_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_arc_slider_get_is_dragging(EGUI_VIEW_OF(&s_arc)));
}

/* Reflect the internal is_dragging flag when it is set. */
static void test_arc_slider_get_is_dragging_active(void)
{
    setup();
    s_arc.is_dragging = 1;
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_arc_slider_get_is_dragging(EGUI_VIEW_OF(&s_arc)));
}

/* Clearing the flag returns to 0. */
static void test_arc_slider_get_is_dragging_cleared(void)
{
    setup();
    s_arc.is_dragging = 1;
    s_arc.is_dragging = 0;
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_arc_slider_get_is_dragging(EGUI_VIEW_OF(&s_arc)));
}

/* NULL self returns 0 without crash. */
static void test_arc_slider_get_is_dragging_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_arc_slider_get_is_dragging(NULL));
}

void test_arc_slider_get_is_dragging_run(void)
{
    EGUI_TEST_SUITE_BEGIN(arc_slider_get_is_dragging);

    EGUI_TEST_RUN(test_arc_slider_get_is_dragging_default);
    EGUI_TEST_RUN(test_arc_slider_get_is_dragging_active);
    EGUI_TEST_RUN(test_arc_slider_get_is_dragging_cleared);
    EGUI_TEST_RUN(test_arc_slider_get_is_dragging_null_self);

    EGUI_TEST_SUITE_END();
}
