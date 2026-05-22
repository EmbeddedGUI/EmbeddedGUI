#include <string.h>

#include "egui.h"
#include "widget/egui_view_slider.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_slider_get_max_value.h"

static egui_view_slider_t s_slider;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_slider, 0, sizeof(s_slider));
    egui_view_slider_init(EGUI_VIEW_OF(&s_slider), core);
}

/* Default max value after init is 100. */
static void test_slider_get_max_value_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(100, (int)egui_view_slider_get_max_value(EGUI_VIEW_OF(&s_slider)));
}

/* Max value is independent from the current value. */
static void test_slider_max_value_unchanged_after_set_value(void)
{
    setup();
    egui_view_slider_set_value(EGUI_VIEW_OF(&s_slider), 50);
    EGUI_TEST_ASSERT_EQUAL_INT(100, (int)egui_view_slider_get_max_value(EGUI_VIEW_OF(&s_slider)));
}

/* Max value differs from current value. */
static void test_slider_max_value_gt_value(void)
{
    setup();
    egui_view_slider_set_value(EGUI_VIEW_OF(&s_slider), 30);
    EGUI_TEST_ASSERT_TRUE((int)egui_view_slider_get_max_value(EGUI_VIEW_OF(&s_slider)) > (int)egui_view_slider_get_value(EGUI_VIEW_OF(&s_slider)));
}

/* NULL self returns 100 (effective max) without crash. */
static void test_slider_get_max_value_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(100, (int)egui_view_slider_get_max_value(NULL));
}

void test_slider_get_max_value_run(void)
{
    EGUI_TEST_SUITE_BEGIN(slider_get_max_value);

    EGUI_TEST_RUN(test_slider_get_max_value_default);
    EGUI_TEST_RUN(test_slider_max_value_unchanged_after_set_value);
    EGUI_TEST_RUN(test_slider_max_value_gt_value);
    EGUI_TEST_RUN(test_slider_get_max_value_null_self);

    EGUI_TEST_SUITE_END();
}
