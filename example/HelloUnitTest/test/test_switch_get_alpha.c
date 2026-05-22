#include <string.h>

#include "egui.h"
#include "widget/egui_view_switch.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_switch_get_alpha.h"

static egui_view_switch_t s_sw;

static void setup(void)
{
    memset(&s_sw, 0, sizeof(s_sw));
    egui_view_switch_init(EGUI_VIEW_OF(&s_sw), uicode_get_core());
}

static void test_switch_get_alpha_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALPHA_100, (int)egui_view_switch_get_alpha(EGUI_VIEW_OF(&s_sw)));
}

static void test_switch_get_alpha_after_set(void)
{
    setup();
    s_sw.alpha = 100;
    EGUI_TEST_ASSERT_EQUAL_INT(100, (int)egui_view_switch_get_alpha(EGUI_VIEW_OF(&s_sw)));
}

static void test_switch_get_alpha_update(void)
{
    setup();
    s_sw.alpha = 80;
    EGUI_TEST_ASSERT_EQUAL_INT(80, (int)egui_view_switch_get_alpha(EGUI_VIEW_OF(&s_sw)));
    s_sw.alpha = 200;
    EGUI_TEST_ASSERT_EQUAL_INT(200, (int)egui_view_switch_get_alpha(EGUI_VIEW_OF(&s_sw)));
}

static void test_switch_get_alpha_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_switch_get_alpha(NULL));
}

void test_switch_get_alpha_run(void)
{
    EGUI_TEST_SUITE_BEGIN(switch_get_alpha);

    EGUI_TEST_RUN(test_switch_get_alpha_default);
    EGUI_TEST_RUN(test_switch_get_alpha_after_set);
    EGUI_TEST_RUN(test_switch_get_alpha_update);
    EGUI_TEST_RUN(test_switch_get_alpha_null_self);

    EGUI_TEST_SUITE_END();
}
