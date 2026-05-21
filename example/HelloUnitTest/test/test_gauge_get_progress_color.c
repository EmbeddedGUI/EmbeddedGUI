#include <string.h>

#include "egui.h"
#include "widget/egui_view_gauge.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_gauge_get_progress_color.h"

static egui_view_gauge_t s_gauge;

static void setup(void)
{
    memset(&s_gauge, 0, sizeof(s_gauge));
    egui_view_gauge_init(EGUI_VIEW_OF(&s_gauge), uicode_get_core());
}

static void test_gauge_get_progress_color_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_PRIMARY.full,
                               (int)egui_view_gauge_get_progress_color(EGUI_VIEW_OF(&s_gauge)).full);
}

static void test_gauge_get_progress_color_after_set(void)
{
    egui_color_t c;
    c.full = 0xDEF012u;
    setup();
    s_gauge.progress_color = c;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c.full,
                               (int)egui_view_gauge_get_progress_color(EGUI_VIEW_OF(&s_gauge)).full);
}

static void test_gauge_get_progress_color_update(void)
{
    egui_color_t c1, c2;
    c1.full = 0xDEF012u;
    c2.full = 0x345678u;
    setup();
    s_gauge.progress_color = c1;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c1.full,
                               (int)egui_view_gauge_get_progress_color(EGUI_VIEW_OF(&s_gauge)).full);
    s_gauge.progress_color = c2;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c2.full,
                               (int)egui_view_gauge_get_progress_color(EGUI_VIEW_OF(&s_gauge)).full);
}

static void test_gauge_get_progress_color_null_self(void)
{
    egui_color_t got = egui_view_gauge_get_progress_color(NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)got.full);
}

void test_gauge_get_progress_color_run(void)
{
    EGUI_TEST_SUITE_BEGIN(gauge_get_progress_color);

    EGUI_TEST_RUN(test_gauge_get_progress_color_default);
    EGUI_TEST_RUN(test_gauge_get_progress_color_after_set);
    EGUI_TEST_RUN(test_gauge_get_progress_color_update);
    EGUI_TEST_RUN(test_gauge_get_progress_color_null_self);

    EGUI_TEST_SUITE_END();
}
