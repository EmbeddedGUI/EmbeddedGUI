#include <string.h>

#include "egui.h"
#include "widget/egui_view_spinner.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_spinner_is_spinning.h"

static egui_view_spinner_t s_spinner;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_spinner, 0, sizeof(s_spinner));
    egui_view_spinner_init(EGUI_VIEW_OF(&s_spinner), core);
}

/* Default state after init is not spinning. */
static void test_spinner_is_spinning_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_spinner_is_spinning(EGUI_VIEW_OF(&s_spinner)));
}

/* After start, is_spinning returns 1. */
static void test_spinner_is_spinning_after_start(void)
{
    setup();
    egui_view_spinner_start(EGUI_VIEW_OF(&s_spinner));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_spinner_is_spinning(EGUI_VIEW_OF(&s_spinner)));
}

/* After start then stop, is_spinning returns 0. */
static void test_spinner_is_spinning_after_stop(void)
{
    setup();
    egui_view_spinner_start(EGUI_VIEW_OF(&s_spinner));
    egui_view_spinner_stop(EGUI_VIEW_OF(&s_spinner));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_spinner_is_spinning(EGUI_VIEW_OF(&s_spinner)));
}

/* NULL self returns 0 without crash. */
static void test_spinner_is_spinning_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_spinner_is_spinning(NULL));
}

void test_spinner_is_spinning_run(void)
{
    EGUI_TEST_SUITE_BEGIN(spinner_is_spinning);

    EGUI_TEST_RUN(test_spinner_is_spinning_default);
    EGUI_TEST_RUN(test_spinner_is_spinning_after_start);
    EGUI_TEST_RUN(test_spinner_is_spinning_after_stop);
    EGUI_TEST_RUN(test_spinner_is_spinning_null_self);

    EGUI_TEST_SUITE_END();
}
