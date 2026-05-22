#include <string.h>

#include "egui.h"
#include "widget/egui_view_label.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_label_get_alpha.h"

static egui_view_label_t s_label;

static void setup(void)
{
    memset(&s_label, 0, sizeof(s_label));
    egui_view_label_init(EGUI_VIEW_OF(&s_label), uicode_get_core());
}

/* After set_font_color the alpha getter returns the same value. */
static void test_label_get_alpha_after_set(void)
{
    egui_color_t c;
    c.full = 0;
    setup();
    egui_view_label_set_font_color(EGUI_VIEW_OF(&s_label), c, EGUI_ALPHA_50);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALPHA_50, (int)egui_view_label_get_alpha(EGUI_VIEW_OF(&s_label)));
}

/* Updating alpha reflects in the getter. */
static void test_label_get_alpha_update(void)
{
    egui_color_t c;
    c.full = 0;
    setup();
    egui_view_label_set_font_color(EGUI_VIEW_OF(&s_label), c, EGUI_ALPHA_50);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&s_label), c, EGUI_ALPHA_100);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALPHA_100, (int)egui_view_label_get_alpha(EGUI_VIEW_OF(&s_label)));
}

/* Default alpha after plain init is EGUI_ALPHA_100. */
static void test_label_get_alpha_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALPHA_100, (int)egui_view_label_get_alpha(EGUI_VIEW_OF(&s_label)));
}

/* NULL self returns 0 without crash. */
static void test_label_get_alpha_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_label_get_alpha(NULL));
}

void test_label_get_alpha_run(void)
{
    EGUI_TEST_SUITE_BEGIN(label_get_alpha);

    EGUI_TEST_RUN(test_label_get_alpha_after_set);
    EGUI_TEST_RUN(test_label_get_alpha_update);
    EGUI_TEST_RUN(test_label_get_alpha_default);
    EGUI_TEST_RUN(test_label_get_alpha_null_self);

    EGUI_TEST_SUITE_END();
}
