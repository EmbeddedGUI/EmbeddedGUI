#include <string.h>

#include "egui.h"
#include "widget/egui_view_circular_progress_bar.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_circular_pb_get_stroke_width.h"

static egui_view_circular_progress_bar_t s_cpb;

static void setup(void)
{
    memset(&s_cpb, 0, sizeof(s_cpb));
    egui_view_circular_progress_bar_init(EGUI_VIEW_OF(&s_cpb), uicode_get_core());
}

/* After set_stroke_width, getter returns the same value. */
static void test_circular_pb_get_stroke_width_after_set(void)
{
    setup();
    egui_view_circular_progress_bar_set_stroke_width(EGUI_VIEW_OF(&s_cpb), 10);
    EGUI_TEST_ASSERT_EQUAL_INT(10, (int)egui_view_circular_progress_bar_get_stroke_width(EGUI_VIEW_OF(&s_cpb)));
}

/* Updating the stroke width reflects in the getter. */
static void test_circular_pb_get_stroke_width_update(void)
{
    setup();
    egui_view_circular_progress_bar_set_stroke_width(EGUI_VIEW_OF(&s_cpb), 10);
    egui_view_circular_progress_bar_set_stroke_width(EGUI_VIEW_OF(&s_cpb), 20);
    EGUI_TEST_ASSERT_EQUAL_INT(20, (int)egui_view_circular_progress_bar_get_stroke_width(EGUI_VIEW_OF(&s_cpb)));
}

/* Default stroke width after init is EGUI_THEME_TRACK_THICKNESS. */
static void test_circular_pb_get_stroke_width_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_THEME_TRACK_THICKNESS, (int)egui_view_circular_progress_bar_get_stroke_width(EGUI_VIEW_OF(&s_cpb)));
}

/* NULL self returns 0 without crash. */
static void test_circular_pb_get_stroke_width_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_circular_progress_bar_get_stroke_width(NULL));
}

void test_circular_pb_get_stroke_width_run(void)
{
    EGUI_TEST_SUITE_BEGIN(circular_pb_get_stroke_width);

    EGUI_TEST_RUN(test_circular_pb_get_stroke_width_after_set);
    EGUI_TEST_RUN(test_circular_pb_get_stroke_width_update);
    EGUI_TEST_RUN(test_circular_pb_get_stroke_width_default);
    EGUI_TEST_RUN(test_circular_pb_get_stroke_width_null_self);

    EGUI_TEST_SUITE_END();
}
