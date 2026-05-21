#include <string.h>

#include "egui.h"
#include "widget/egui_view_circular_progress_bar.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_circular_pb_get_bk_color.h"

static egui_view_circular_progress_bar_t s_cpb;

static void setup(void)
{
    memset(&s_cpb, 0, sizeof(s_cpb));
    egui_view_circular_progress_bar_init(EGUI_VIEW_OF(&s_cpb), uicode_get_core());
}

/* After set_bk_color, getter returns the same color. */
static void test_circular_pb_get_bk_color_after_set(void)
{
    egui_color_t c;
    c.full = 0xFF0000u;
    setup();
    egui_view_circular_progress_bar_set_bk_color(EGUI_VIEW_OF(&s_cpb), c);
    EGUI_TEST_ASSERT_EQUAL_INT((int)c.full,
                               (int)egui_view_circular_progress_bar_get_bk_color(EGUI_VIEW_OF(&s_cpb)).full);
}

/* Updating the color reflects in the getter. */
static void test_circular_pb_get_bk_color_update(void)
{
    egui_color_t c1, c2;
    c1.full = 0xFF0000u;
    c2.full = 0x00FF00u;
    setup();
    egui_view_circular_progress_bar_set_bk_color(EGUI_VIEW_OF(&s_cpb), c1);
    egui_view_circular_progress_bar_set_bk_color(EGUI_VIEW_OF(&s_cpb), c2);
    EGUI_TEST_ASSERT_EQUAL_INT((int)c2.full,
                               (int)egui_view_circular_progress_bar_get_bk_color(EGUI_VIEW_OF(&s_cpb)).full);
}

/* Default bk_color after init is EGUI_THEME_TRACK_BG. */
static void test_circular_pb_get_bk_color_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TRACK_BG.full,
                               (int)egui_view_circular_progress_bar_get_bk_color(EGUI_VIEW_OF(&s_cpb)).full);
}

/* NULL self returns zeroed color without crash. */
static void test_circular_pb_get_bk_color_null_self(void)
{
    egui_color_t got = egui_view_circular_progress_bar_get_bk_color(NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)got.full);
}

void test_circular_pb_get_bk_color_run(void)
{
    EGUI_TEST_SUITE_BEGIN(circular_pb_get_bk_color);

    EGUI_TEST_RUN(test_circular_pb_get_bk_color_after_set);
    EGUI_TEST_RUN(test_circular_pb_get_bk_color_update);
    EGUI_TEST_RUN(test_circular_pb_get_bk_color_default);
    EGUI_TEST_RUN(test_circular_pb_get_bk_color_null_self);

    EGUI_TEST_SUITE_END();
}
