#include <string.h>

#include "egui.h"
#include "widget/egui_view_arc_slider.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_arc_slider_get_start_angle.h"

static egui_view_arc_slider_t s_arc;

static void setup(void)
{
    memset(&s_arc, 0, sizeof(s_arc));
    egui_view_arc_slider_init(EGUI_VIEW_OF(&s_arc), uicode_get_core());
}

/* Default start_angle after init is 150. */
static void test_arc_slider_get_start_angle_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(150, (int)egui_view_arc_slider_get_start_angle(EGUI_VIEW_OF(&s_arc)));
}

/* After assigning start_angle, getter returns the same value. */
static void test_arc_slider_get_start_angle_after_set(void)
{
    setup();
    s_arc.start_angle = 90;
    EGUI_TEST_ASSERT_EQUAL_INT(90, (int)egui_view_arc_slider_get_start_angle(EGUI_VIEW_OF(&s_arc)));
}

/* Updating start_angle reflects in the getter. */
static void test_arc_slider_get_start_angle_update(void)
{
    setup();
    s_arc.start_angle = 45;
    EGUI_TEST_ASSERT_EQUAL_INT(45, (int)egui_view_arc_slider_get_start_angle(EGUI_VIEW_OF(&s_arc)));
    s_arc.start_angle = 270;
    EGUI_TEST_ASSERT_EQUAL_INT(270, (int)egui_view_arc_slider_get_start_angle(EGUI_VIEW_OF(&s_arc)));
}

/* NULL self returns 0 without crash. */
static void test_arc_slider_get_start_angle_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_arc_slider_get_start_angle(NULL));
}

void test_arc_slider_get_start_angle_run(void)
{
    EGUI_TEST_SUITE_BEGIN(arc_slider_get_start_angle);

    EGUI_TEST_RUN(test_arc_slider_get_start_angle_default);
    EGUI_TEST_RUN(test_arc_slider_get_start_angle_after_set);
    EGUI_TEST_RUN(test_arc_slider_get_start_angle_update);
    EGUI_TEST_RUN(test_arc_slider_get_start_angle_null_self);

    EGUI_TEST_SUITE_END();
}
