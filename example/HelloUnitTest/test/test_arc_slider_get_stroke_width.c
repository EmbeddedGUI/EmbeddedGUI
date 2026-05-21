#include <string.h>

#include "egui.h"
#include "widget/egui_view_arc_slider.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_arc_slider_get_stroke_width.h"

static egui_view_arc_slider_t s_arc;

static void setup(void)
{
    memset(&s_arc, 0, sizeof(s_arc));
    egui_view_arc_slider_init(EGUI_VIEW_OF(&s_arc), uicode_get_core());
}

/* Default stroke_width after init is 8. */
static void test_arc_slider_get_stroke_width_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(8, (int)egui_view_arc_slider_get_stroke_width(EGUI_VIEW_OF(&s_arc)));
}

/* After assigning stroke_width, getter returns the same value. */
static void test_arc_slider_get_stroke_width_after_set(void)
{
    setup();
    s_arc.stroke_width = 12;
    EGUI_TEST_ASSERT_EQUAL_INT(12, (int)egui_view_arc_slider_get_stroke_width(EGUI_VIEW_OF(&s_arc)));
}

/* Updating stroke_width reflects in the getter. */
static void test_arc_slider_get_stroke_width_update(void)
{
    setup();
    s_arc.stroke_width = 4;
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_arc_slider_get_stroke_width(EGUI_VIEW_OF(&s_arc)));
    s_arc.stroke_width = 16;
    EGUI_TEST_ASSERT_EQUAL_INT(16, (int)egui_view_arc_slider_get_stroke_width(EGUI_VIEW_OF(&s_arc)));
}

/* NULL self returns 0 without crash. */
static void test_arc_slider_get_stroke_width_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_arc_slider_get_stroke_width(NULL));
}

void test_arc_slider_get_stroke_width_run(void)
{
    EGUI_TEST_SUITE_BEGIN(arc_slider_get_stroke_width);

    EGUI_TEST_RUN(test_arc_slider_get_stroke_width_default);
    EGUI_TEST_RUN(test_arc_slider_get_stroke_width_after_set);
    EGUI_TEST_RUN(test_arc_slider_get_stroke_width_update);
    EGUI_TEST_RUN(test_arc_slider_get_stroke_width_null_self);

    EGUI_TEST_SUITE_END();
}
