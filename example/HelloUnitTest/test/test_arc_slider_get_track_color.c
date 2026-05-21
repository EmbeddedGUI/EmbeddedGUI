#include <string.h>

#include "egui.h"
#include "widget/egui_view_arc_slider.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_arc_slider_get_track_color.h"

static egui_view_arc_slider_t s_arc;

static void setup(void)
{
    memset(&s_arc, 0, sizeof(s_arc));
    egui_view_arc_slider_init(EGUI_VIEW_OF(&s_arc), uicode_get_core());
}

/* Default track_color after init is EGUI_COLOR_DARK_GREY. */
static void test_arc_slider_get_track_color_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_COLOR_DARK_GREY.full,
                               (int)egui_view_arc_slider_get_track_color(EGUI_VIEW_OF(&s_arc)).full);
}

/* After assigning track_color, getter returns the same color. */
static void test_arc_slider_get_track_color_after_set(void)
{
    egui_color_t c;
    c.full = 0x112233u;
    setup();
    s_arc.track_color = c;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c.full,
                               (int)egui_view_arc_slider_get_track_color(EGUI_VIEW_OF(&s_arc)).full);
}

/* Updating the color reflects in the getter. */
static void test_arc_slider_get_track_color_update(void)
{
    egui_color_t c1, c2;
    c1.full = 0x112233u;
    c2.full = 0x445566u;
    setup();
    s_arc.track_color = c1;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c1.full,
                               (int)egui_view_arc_slider_get_track_color(EGUI_VIEW_OF(&s_arc)).full);
    s_arc.track_color = c2;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c2.full,
                               (int)egui_view_arc_slider_get_track_color(EGUI_VIEW_OF(&s_arc)).full);
}

/* NULL self returns zeroed color without crash. */
static void test_arc_slider_get_track_color_null_self(void)
{
    egui_color_t got = egui_view_arc_slider_get_track_color(NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)got.full);
}

void test_arc_slider_get_track_color_run(void)
{
    EGUI_TEST_SUITE_BEGIN(arc_slider_get_track_color);

    EGUI_TEST_RUN(test_arc_slider_get_track_color_default);
    EGUI_TEST_RUN(test_arc_slider_get_track_color_after_set);
    EGUI_TEST_RUN(test_arc_slider_get_track_color_update);
    EGUI_TEST_RUN(test_arc_slider_get_track_color_null_self);

    EGUI_TEST_SUITE_END();
}
