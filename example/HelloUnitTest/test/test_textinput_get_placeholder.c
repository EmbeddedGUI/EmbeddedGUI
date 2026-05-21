#include <string.h>

#include "egui.h"
#include "widget/egui_view_textinput.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_textinput_get_placeholder.h"

static egui_view_textinput_t s_ti;

static void setup(void)
{
    memset(&s_ti, 0, sizeof(s_ti));
    egui_view_textinput_init(EGUI_VIEW_OF(&s_ti), uicode_get_core());
}

/* Default placeholder after init is NULL. */
static void test_textinput_get_placeholder_default(void)
{
    setup();
    EGUI_TEST_ASSERT_NULL(egui_view_textinput_get_placeholder(EGUI_VIEW_OF(&s_ti)));
}

/* After set_placeholder, getter returns the same borrowed pointer. */
static void test_textinput_get_placeholder_after_set(void)
{
    static const char *kHint = "type...";
    setup();
    egui_view_textinput_set_placeholder(EGUI_VIEW_OF(&s_ti), kHint);
    EGUI_TEST_ASSERT_TRUE(egui_view_textinput_get_placeholder(EGUI_VIEW_OF(&s_ti)) == kHint);
}

/* Setting placeholder back to NULL clears it. */
static void test_textinput_get_placeholder_clear_to_null(void)
{
    setup();
    egui_view_textinput_set_placeholder(EGUI_VIEW_OF(&s_ti), "hint");
    egui_view_textinput_set_placeholder(EGUI_VIEW_OF(&s_ti), NULL);
    EGUI_TEST_ASSERT_NULL(egui_view_textinput_get_placeholder(EGUI_VIEW_OF(&s_ti)));
}

/* NULL self returns NULL without crash. */
static void test_textinput_get_placeholder_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_textinput_get_placeholder(NULL));
}

void test_textinput_get_placeholder_run(void)
{
    EGUI_TEST_SUITE_BEGIN(textinput_get_placeholder);

    EGUI_TEST_RUN(test_textinput_get_placeholder_default);
    EGUI_TEST_RUN(test_textinput_get_placeholder_after_set);
    EGUI_TEST_RUN(test_textinput_get_placeholder_clear_to_null);
    EGUI_TEST_RUN(test_textinput_get_placeholder_null_self);

    EGUI_TEST_SUITE_END();
}
