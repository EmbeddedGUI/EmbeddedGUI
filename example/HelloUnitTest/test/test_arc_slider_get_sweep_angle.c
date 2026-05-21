#include <string.h>

#include "egui.h"
#include "widget/egui_view_arc_slider.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_arc_slider_get_sweep_angle.h"

static egui_view_arc_slider_t s_arc;

static void setup(void)
{
    memset(&s_arc, 0, sizeof(s_arc));
    egui_view_arc_slider_init(EGUI_VIEW_OF(&s_arc), uicode_get_core());
}

/* Default sweep_angle after init is 240. */
static void test_arc_slider_get_sweep_angle_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(240, (int)egui_view_arc_slider_get_sweep_angle(EGUI_VIEW_OF(&s_arc)));
}

/* After assigning sweep_angle, getter returns the same value. */
static void test_arc_slider_get_sweep_angle_after_set(void)
{
    setup();
    s_arc.sweep_angle = 180;
    EGUI_TEST_ASSERT_EQUAL_INT(180, (int)egui_view_arc_slider_get_sweep_angle(EGUI_VIEW_OF(&s_arc)));
}

/* Updating sweep_angle reflects in the getter. */
static void test_arc_slider_get_sweep_angle_update(void)
{
    setup();
    s_arc.sweep_angle = 90;
    EGUI_TEST_ASSERT_EQUAL_INT(90, (int)egui_view_arc_slider_get_sweep_angle(EGUI_VIEW_OF(&s_arc)));
    s_arc.sweep_angle = 300;
    EGUI_TEST_ASSERT_EQUAL_INT(300, (int)egui_view_arc_slider_get_sweep_angle(EGUI_VIEW_OF(&s_arc)));
}

/* NULL self returns 0 without crash. */
static void test_arc_slider_get_sweep_angle_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_arc_slider_get_sweep_angle(NULL));
}

void test_arc_slider_get_sweep_angle_run(void)
{
    EGUI_TEST_SUITE_BEGIN(arc_slider_get_sweep_angle);

    EGUI_TEST_RUN(test_arc_slider_get_sweep_angle_default);
    EGUI_TEST_RUN(test_arc_slider_get_sweep_angle_after_set);
    EGUI_TEST_RUN(test_arc_slider_get_sweep_angle_update);
    EGUI_TEST_RUN(test_arc_slider_get_sweep_angle_null_self);

    EGUI_TEST_SUITE_END();
}
