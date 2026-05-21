#include <string.h>

#include "egui.h"
#include "widget/egui_view_checkbox.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_checkbox_get_checked.h"

static egui_view_checkbox_t s_checkbox;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_checkbox, 0, sizeof(s_checkbox));
    egui_view_checkbox_init(EGUI_VIEW_OF(&s_checkbox), core);
}

/* Default state after init is unchecked. */
static void test_checkbox_default_unchecked(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_checkbox_get_checked(EGUI_VIEW_OF(&s_checkbox)));
}

/* get_checked returns 1 after set_checked(1). */
static void test_checkbox_get_checked_after_set(void)
{
    setup();
    egui_view_checkbox_set_checked(EGUI_VIEW_OF(&s_checkbox), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_checkbox_get_checked(EGUI_VIEW_OF(&s_checkbox)));
}

/* get_checked returns 0 after set_checked(0). */
static void test_checkbox_get_unchecked_after_set(void)
{
    setup();
    egui_view_checkbox_set_checked(EGUI_VIEW_OF(&s_checkbox), 1);
    egui_view_checkbox_set_checked(EGUI_VIEW_OF(&s_checkbox), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_checkbox_get_checked(EGUI_VIEW_OF(&s_checkbox)));
}

/* NULL self returns 0 without crash. */
static void test_checkbox_get_checked_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_checkbox_get_checked(NULL));
}

void test_checkbox_get_checked_run(void)
{
    EGUI_TEST_SUITE_BEGIN(checkbox_get_checked);

    EGUI_TEST_RUN(test_checkbox_default_unchecked);
    EGUI_TEST_RUN(test_checkbox_get_checked_after_set);
    EGUI_TEST_RUN(test_checkbox_get_unchecked_after_set);
    EGUI_TEST_RUN(test_checkbox_get_checked_null_self);

    EGUI_TEST_SUITE_END();
}
