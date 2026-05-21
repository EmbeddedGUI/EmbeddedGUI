#include <string.h>

#include "egui.h"
#include "widget/egui_view_number_picker.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_number_picker_get_step.h"

static egui_view_number_picker_t s_picker;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_picker, 0, sizeof(s_picker));
    egui_view_number_picker_init(EGUI_VIEW_OF(&s_picker), core);
}

/* Default step after init is 1. */
static void test_number_picker_get_step_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_number_picker_get_step(EGUI_VIEW_OF(&s_picker)));
}

/* After set_step, get_step reflects the new value. */
static void test_number_picker_get_step_after_set(void)
{
    setup();
    egui_view_number_picker_set_step(EGUI_VIEW_OF(&s_picker), 5);
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_number_picker_get_step(EGUI_VIEW_OF(&s_picker)));
}

/* Step can be updated multiple times. */
static void test_number_picker_get_step_update(void)
{
    setup();
    egui_view_number_picker_set_step(EGUI_VIEW_OF(&s_picker), 10);
    egui_view_number_picker_set_step(EGUI_VIEW_OF(&s_picker), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_number_picker_get_step(EGUI_VIEW_OF(&s_picker)));
}

/* NULL self returns 0 without crash. */
static void test_number_picker_get_step_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_number_picker_get_step(NULL));
}

void test_number_picker_get_step_run(void)
{
    EGUI_TEST_SUITE_BEGIN(number_picker_get_step);

    EGUI_TEST_RUN(test_number_picker_get_step_default);
    EGUI_TEST_RUN(test_number_picker_get_step_after_set);
    EGUI_TEST_RUN(test_number_picker_get_step_update);
    EGUI_TEST_RUN(test_number_picker_get_step_null_self);

    EGUI_TEST_SUITE_END();
}
