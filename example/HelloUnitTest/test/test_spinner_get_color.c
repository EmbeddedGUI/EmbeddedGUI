#include <string.h>

#include "egui.h"
#include "widget/egui_view_spinner.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_spinner_get_color.h"

static egui_view_spinner_t s_spinner;

static void setup(void)
{
    memset(&s_spinner, 0, sizeof(s_spinner));
    egui_view_spinner_init(EGUI_VIEW_OF(&s_spinner), uicode_get_core());
}

/* After set_color, getter returns the same color. */
static void test_spinner_get_color_after_set(void)
{
    egui_color_t c;
    c.full = 0xFF0000u;
    setup();
    egui_view_spinner_set_color(EGUI_VIEW_OF(&s_spinner), c);
    EGUI_TEST_ASSERT_EQUAL_INT((int)c.full, (int)egui_view_spinner_get_color(EGUI_VIEW_OF(&s_spinner)).full);
}

/* Updating the color reflects in the getter. */
static void test_spinner_get_color_update(void)
{
    egui_color_t c1, c2;
    c1.full = 0xFF0000u;
    c2.full = 0x00FF00u;
    setup();
    egui_view_spinner_set_color(EGUI_VIEW_OF(&s_spinner), c1);
    egui_view_spinner_set_color(EGUI_VIEW_OF(&s_spinner), c2);
    EGUI_TEST_ASSERT_EQUAL_INT((int)c2.full, (int)egui_view_spinner_get_color(EGUI_VIEW_OF(&s_spinner)).full);
}

/* Default color after init is EGUI_THEME_PRIMARY. */
static void test_spinner_get_color_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_PRIMARY.full, (int)egui_view_spinner_get_color(EGUI_VIEW_OF(&s_spinner)).full);
}

/* NULL self returns zeroed color without crash. */
static void test_spinner_get_color_null_self(void)
{
    egui_color_t got = egui_view_spinner_get_color(NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)got.full);
}

void test_spinner_get_color_run(void)
{
    EGUI_TEST_SUITE_BEGIN(spinner_get_color);

    EGUI_TEST_RUN(test_spinner_get_color_after_set);
    EGUI_TEST_RUN(test_spinner_get_color_update);
    EGUI_TEST_RUN(test_spinner_get_color_default);
    EGUI_TEST_RUN(test_spinner_get_color_null_self);

    EGUI_TEST_SUITE_END();
}
