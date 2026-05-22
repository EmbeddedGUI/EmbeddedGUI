#include <string.h>

#include "egui.h"
#include "widget/egui_view_number_picker.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_number_picker_get_max_value.h"

static egui_view_number_picker_t s_picker;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_picker, 0, sizeof(s_picker));
    egui_view_number_picker_init(EGUI_VIEW_OF(&s_picker), core);
}

/* Default max_value after init is 100. */
static void test_number_picker_get_max_value_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(100, (int)egui_view_number_picker_get_max_value(EGUI_VIEW_OF(&s_picker)));
}

/* After set_range, get_max_value reflects the new maximum. */
static void test_number_picker_get_max_value_after_set_range(void)
{
    setup();
    egui_view_number_picker_set_range(EGUI_VIEW_OF(&s_picker), 0, 200);
    EGUI_TEST_ASSERT_EQUAL_INT(200, (int)egui_view_number_picker_get_max_value(EGUI_VIEW_OF(&s_picker)));
}

/* max is always >= min after a valid set_range call. */
static void test_number_picker_get_max_value_gt_min(void)
{
    setup();
    egui_view_number_picker_set_range(EGUI_VIEW_OF(&s_picker), -100, 0);
    EGUI_TEST_ASSERT_TRUE(egui_view_number_picker_get_max_value(EGUI_VIEW_OF(&s_picker)) > egui_view_number_picker_get_min_value(EGUI_VIEW_OF(&s_picker)));
}

/* NULL self returns 0 without crash. */
static void test_number_picker_get_max_value_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_number_picker_get_max_value(NULL));
}

void test_number_picker_get_max_value_run(void)
{
    EGUI_TEST_SUITE_BEGIN(number_picker_get_max_value);

    EGUI_TEST_RUN(test_number_picker_get_max_value_default);
    EGUI_TEST_RUN(test_number_picker_get_max_value_after_set_range);
    EGUI_TEST_RUN(test_number_picker_get_max_value_gt_min);
    EGUI_TEST_RUN(test_number_picker_get_max_value_null_self);

    EGUI_TEST_SUITE_END();
}
