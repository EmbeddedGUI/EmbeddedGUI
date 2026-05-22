#include <string.h>

#include "egui.h"
#include "widget/egui_view_number_picker.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_number_picker_get_alpha.h"

static egui_view_number_picker_t s_np;

static void setup(void)
{
    memset(&s_np, 0, sizeof(s_np));
    egui_view_number_picker_init(EGUI_VIEW_OF(&s_np), uicode_get_core());
}

/* Default alpha after init is EGUI_ALPHA_100 (255). */
static void test_number_picker_get_alpha_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALPHA_100, (int)egui_view_number_picker_get_alpha(EGUI_VIEW_OF(&s_np)));
}

/* After assigning alpha, getter returns the same value. */
static void test_number_picker_get_alpha_after_set(void)
{
    setup();
    s_np.alpha = 128;
    EGUI_TEST_ASSERT_EQUAL_INT(128, (int)egui_view_number_picker_get_alpha(EGUI_VIEW_OF(&s_np)));
}

/* Updating alpha reflects in the getter. */
static void test_number_picker_get_alpha_update(void)
{
    setup();
    s_np.alpha = 64;
    EGUI_TEST_ASSERT_EQUAL_INT(64, (int)egui_view_number_picker_get_alpha(EGUI_VIEW_OF(&s_np)));
    s_np.alpha = 200;
    EGUI_TEST_ASSERT_EQUAL_INT(200, (int)egui_view_number_picker_get_alpha(EGUI_VIEW_OF(&s_np)));
}

/* NULL self returns 0 without crash. */
static void test_number_picker_get_alpha_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_number_picker_get_alpha(NULL));
}

void test_number_picker_get_alpha_run(void)
{
    EGUI_TEST_SUITE_BEGIN(number_picker_get_alpha);

    EGUI_TEST_RUN(test_number_picker_get_alpha_default);
    EGUI_TEST_RUN(test_number_picker_get_alpha_after_set);
    EGUI_TEST_RUN(test_number_picker_get_alpha_update);
    EGUI_TEST_RUN(test_number_picker_get_alpha_null_self);

    EGUI_TEST_SUITE_END();
}
