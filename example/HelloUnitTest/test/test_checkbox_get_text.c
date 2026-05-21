#include <string.h>

#include "egui.h"
#include "widget/egui_view_checkbox.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_checkbox_get_text.h"

static egui_view_checkbox_t s_cb;

static void setup(void)
{
    memset(&s_cb, 0, sizeof(s_cb));
    egui_view_checkbox_init(EGUI_VIEW_OF(&s_cb), uicode_get_core());
}

/* Default label text after init is NULL. */
static void test_checkbox_get_text_default(void)
{
    setup();
    EGUI_TEST_ASSERT_NULL(egui_view_checkbox_get_text(EGUI_VIEW_OF(&s_cb)));
}

/* After set_text, getter returns the same borrowed pointer. */
static void test_checkbox_get_text_after_set(void)
{
    static const char *kLabel = "Enable";
    setup();
    egui_view_checkbox_set_text(EGUI_VIEW_OF(&s_cb), kLabel);
    EGUI_TEST_ASSERT_TRUE(egui_view_checkbox_get_text(EGUI_VIEW_OF(&s_cb)) == kLabel);
}

/* Setting text back to NULL clears it. */
static void test_checkbox_get_text_clear_to_null(void)
{
    setup();
    egui_view_checkbox_set_text(EGUI_VIEW_OF(&s_cb), "Enable");
    egui_view_checkbox_set_text(EGUI_VIEW_OF(&s_cb), NULL);
    EGUI_TEST_ASSERT_NULL(egui_view_checkbox_get_text(EGUI_VIEW_OF(&s_cb)));
}

/* NULL self returns NULL without crash. */
static void test_checkbox_get_text_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_checkbox_get_text(NULL));
}

void test_checkbox_get_text_run(void)
{
    EGUI_TEST_SUITE_BEGIN(checkbox_get_text);

    EGUI_TEST_RUN(test_checkbox_get_text_default);
    EGUI_TEST_RUN(test_checkbox_get_text_after_set);
    EGUI_TEST_RUN(test_checkbox_get_text_clear_to_null);
    EGUI_TEST_RUN(test_checkbox_get_text_null_self);

    EGUI_TEST_SUITE_END();
}
