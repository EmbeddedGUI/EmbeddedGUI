#include <string.h>

#include "egui.h"
#include "widget/egui_view_label.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_label_get_font.h"

static egui_view_label_t s_label;
static egui_font_t       s_font;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_label, 0, sizeof(s_label));
    memset(&s_font,  0, sizeof(s_font));
    egui_view_label_init(EGUI_VIEW_OF(&s_label), core);
}

/* Default font after init is NULL (uses config default at draw time). */
static void test_label_get_font_default(void)
{
    setup();
    EGUI_TEST_ASSERT_NULL(egui_view_label_get_font(EGUI_VIEW_OF(&s_label)));
}

/* After set_font, get_font returns the same pointer. */
static void test_label_get_font_after_set(void)
{
    setup();
    egui_view_label_set_font(EGUI_VIEW_OF(&s_label), &s_font);
    EGUI_TEST_ASSERT_TRUE(egui_view_label_get_font(EGUI_VIEW_OF(&s_label)) == &s_font);
}

/* set_font(NULL) clears the font pointer. */
static void test_label_get_font_clear_to_null(void)
{
    setup();
    egui_view_label_set_font(EGUI_VIEW_OF(&s_label), &s_font);
    egui_view_label_set_font(EGUI_VIEW_OF(&s_label), NULL);
    EGUI_TEST_ASSERT_NULL(egui_view_label_get_font(EGUI_VIEW_OF(&s_label)));
}

/* NULL self returns NULL without crash. */
static void test_label_get_font_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_label_get_font(NULL));
}

void test_label_get_font_run(void)
{
    EGUI_TEST_SUITE_BEGIN(label_get_font);

    EGUI_TEST_RUN(test_label_get_font_default);
    EGUI_TEST_RUN(test_label_get_font_after_set);
    EGUI_TEST_RUN(test_label_get_font_clear_to_null);
    EGUI_TEST_RUN(test_label_get_font_null_self);

    EGUI_TEST_SUITE_END();
}
