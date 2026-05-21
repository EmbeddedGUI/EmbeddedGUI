#include <string.h>

#include "egui.h"
#include "widget/egui_view_number_picker.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_number_picker_get_min_value.h"

static egui_view_number_picker_t s_picker;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_picker, 0, sizeof(s_picker));
    egui_view_number_picker_init(EGUI_VIEW_OF(&s_picker), core);
}

/* Default min_value after init is 0. */
static void test_number_picker_get_min_value_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_number_picker_get_min_value(EGUI_VIEW_OF(&s_picker)));
}

/* After set_range, get_min_value reflects the new minimum. */
static void test_number_picker_get_min_value_after_set_range(void)
{
    setup();
    egui_view_number_picker_set_range(EGUI_VIEW_OF(&s_picker), -50, 50);
    EGUI_TEST_ASSERT_EQUAL_INT(-50, (int)egui_view_number_picker_get_min_value(EGUI_VIEW_OF(&s_picker)));
}

/* Updating range a second time reflects the updated minimum. */
static void test_number_picker_get_min_value_update(void)
{
    setup();
    egui_view_number_picker_set_range(EGUI_VIEW_OF(&s_picker), 10, 90);
    EGUI_TEST_ASSERT_EQUAL_INT(10, (int)egui_view_number_picker_get_min_value(EGUI_VIEW_OF(&s_picker)));
}

/* NULL self returns 0 without crash. */
static void test_number_picker_get_min_value_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_number_picker_get_min_value(NULL));
}

void test_number_picker_get_min_value_run(void)
{
    EGUI_TEST_SUITE_BEGIN(number_picker_get_min_value);

    EGUI_TEST_RUN(test_number_picker_get_min_value_default);
    EGUI_TEST_RUN(test_number_picker_get_min_value_after_set_range);
    EGUI_TEST_RUN(test_number_picker_get_min_value_update);
    EGUI_TEST_RUN(test_number_picker_get_min_value_null_self);

    EGUI_TEST_SUITE_END();
}
