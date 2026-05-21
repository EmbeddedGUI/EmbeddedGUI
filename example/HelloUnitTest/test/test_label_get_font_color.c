#include <string.h>

#include "egui.h"
#include "widget/egui_view_label.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_label_get_font_color.h"

static egui_view_label_t s_label;

static void setup(void)
{
    memset(&s_label, 0, sizeof(s_label));
    egui_view_label_init(EGUI_VIEW_OF(&s_label), uicode_get_core());
}

/* After set_font_color the getter returns the same color. */
static void test_label_get_font_color_after_set(void)
{
    egui_color_t c;
    c.full = 0xFF0000u;
    setup();
    egui_view_label_set_font_color(EGUI_VIEW_OF(&s_label), c, EGUI_ALPHA_100);
    EGUI_TEST_ASSERT_TRUE(egui_view_label_get_font_color(EGUI_VIEW_OF(&s_label)).full == c.full);
}

/* Updating the color reflects in the getter. */
static void test_label_get_font_color_update(void)
{
    egui_color_t c1, c2;
    c1.full = 0xFF0000u;
    c2.full = 0x00FF00u;
    setup();
    egui_view_label_set_font_color(EGUI_VIEW_OF(&s_label), c1, EGUI_ALPHA_100);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&s_label), c2, EGUI_ALPHA_100);
    EGUI_TEST_ASSERT_TRUE(egui_view_label_get_font_color(EGUI_VIEW_OF(&s_label)).full == c2.full);
}

/* Default color after plain init is the theme primary text color. */
static void test_label_get_font_color_default(void)
{
    egui_color_t got;
    setup();
    got = egui_view_label_get_font_color(EGUI_VIEW_OF(&s_label));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT_PRIMARY.full, (int)got.full);
}

/* NULL self returns zeroed color without crash. */
static void test_label_get_font_color_null_self(void)
{
    egui_color_t got = egui_view_label_get_font_color(NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)got.full);
}

void test_label_get_font_color_run(void)
{
    EGUI_TEST_SUITE_BEGIN(label_get_font_color);

    EGUI_TEST_RUN(test_label_get_font_color_after_set);
    EGUI_TEST_RUN(test_label_get_font_color_update);
    EGUI_TEST_RUN(test_label_get_font_color_default);
    EGUI_TEST_RUN(test_label_get_font_color_null_self);

    EGUI_TEST_SUITE_END();
}
