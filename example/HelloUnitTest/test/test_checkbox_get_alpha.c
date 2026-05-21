#include <string.h>

#include "egui.h"
#include "widget/egui_view_checkbox.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_checkbox_get_alpha.h"

static egui_view_checkbox_t s_cb;

static void setup(void)
{
    memset(&s_cb, 0, sizeof(s_cb));
    egui_view_checkbox_init(EGUI_VIEW_OF(&s_cb), uicode_get_core());
}

static void test_checkbox_get_alpha_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALPHA_100,
                               (int)egui_view_checkbox_get_alpha(EGUI_VIEW_OF(&s_cb)));
}

static void test_checkbox_get_alpha_after_set(void)
{
    setup();
    s_cb.alpha = 128;
    EGUI_TEST_ASSERT_EQUAL_INT(128, (int)egui_view_checkbox_get_alpha(EGUI_VIEW_OF(&s_cb)));
}

static void test_checkbox_get_alpha_update(void)
{
    setup();
    s_cb.alpha = 100;
    EGUI_TEST_ASSERT_EQUAL_INT(100, (int)egui_view_checkbox_get_alpha(EGUI_VIEW_OF(&s_cb)));
    s_cb.alpha = 200;
    EGUI_TEST_ASSERT_EQUAL_INT(200, (int)egui_view_checkbox_get_alpha(EGUI_VIEW_OF(&s_cb)));
}

static void test_checkbox_get_alpha_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_checkbox_get_alpha(NULL));
}

void test_checkbox_get_alpha_run(void)
{
    EGUI_TEST_SUITE_BEGIN(checkbox_get_alpha);

    EGUI_TEST_RUN(test_checkbox_get_alpha_default);
    EGUI_TEST_RUN(test_checkbox_get_alpha_after_set);
    EGUI_TEST_RUN(test_checkbox_get_alpha_update);
    EGUI_TEST_RUN(test_checkbox_get_alpha_null_self);

    EGUI_TEST_SUITE_END();
}
