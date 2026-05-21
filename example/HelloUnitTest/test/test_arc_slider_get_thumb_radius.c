#include <string.h>

#include "egui.h"
#include "widget/egui_view_arc_slider.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_arc_slider_get_thumb_radius.h"

static egui_view_arc_slider_t s_arc;

static void setup(void)
{
    memset(&s_arc, 0, sizeof(s_arc));
    egui_view_arc_slider_init(EGUI_VIEW_OF(&s_arc), uicode_get_core());
}

/* Default thumb_radius after init is 6. */
static void test_arc_slider_get_thumb_radius_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_arc_slider_get_thumb_radius(EGUI_VIEW_OF(&s_arc)));
}

/* After assigning thumb_radius, getter returns the same value. */
static void test_arc_slider_get_thumb_radius_after_set(void)
{
    setup();
    s_arc.thumb_radius = 10;
    EGUI_TEST_ASSERT_EQUAL_INT(10, (int)egui_view_arc_slider_get_thumb_radius(EGUI_VIEW_OF(&s_arc)));
}

/* Updating thumb_radius reflects in the getter. */
static void test_arc_slider_get_thumb_radius_update(void)
{
    setup();
    s_arc.thumb_radius = 4;
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_arc_slider_get_thumb_radius(EGUI_VIEW_OF(&s_arc)));
    s_arc.thumb_radius = 8;
    EGUI_TEST_ASSERT_EQUAL_INT(8, (int)egui_view_arc_slider_get_thumb_radius(EGUI_VIEW_OF(&s_arc)));
}

/* NULL self returns 0 without crash. */
static void test_arc_slider_get_thumb_radius_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_arc_slider_get_thumb_radius(NULL));
}

void test_arc_slider_get_thumb_radius_run(void)
{
    EGUI_TEST_SUITE_BEGIN(arc_slider_get_thumb_radius);

    EGUI_TEST_RUN(test_arc_slider_get_thumb_radius_default);
    EGUI_TEST_RUN(test_arc_slider_get_thumb_radius_after_set);
    EGUI_TEST_RUN(test_arc_slider_get_thumb_radius_update);
    EGUI_TEST_RUN(test_arc_slider_get_thumb_radius_null_self);

    EGUI_TEST_SUITE_END();
}
