#include <string.h>

#include "egui.h"
#include "widget/egui_view_switch.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_switch_get_checked.h"

static egui_view_switch_t s_switch;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_switch, 0, sizeof(s_switch));
    egui_view_switch_init(EGUI_VIEW_OF(&s_switch), core);
}

/* Default state after init is off. */
static void test_switch_default_off(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_switch_get_checked(EGUI_VIEW_OF(&s_switch)));
}

/* get_checked returns 1 after set_checked(1). */
static void test_switch_get_checked_after_set(void)
{
    setup();
    egui_view_switch_set_checked(EGUI_VIEW_OF(&s_switch), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_switch_get_checked(EGUI_VIEW_OF(&s_switch)));
}

/* get_checked returns 0 after toggling back. */
static void test_switch_get_unchecked_after_toggle(void)
{
    setup();
    egui_view_switch_set_checked(EGUI_VIEW_OF(&s_switch), 1);
    egui_view_switch_set_checked(EGUI_VIEW_OF(&s_switch), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_switch_get_checked(EGUI_VIEW_OF(&s_switch)));
}

/* NULL self returns 0 without crash. */
static void test_switch_get_checked_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_switch_get_checked(NULL));
}

void test_switch_get_checked_run(void)
{
    EGUI_TEST_SUITE_BEGIN(switch_get_checked);

    EGUI_TEST_RUN(test_switch_default_off);
    EGUI_TEST_RUN(test_switch_get_checked_after_set);
    EGUI_TEST_RUN(test_switch_get_unchecked_after_toggle);
    EGUI_TEST_RUN(test_switch_get_checked_null_self);

    EGUI_TEST_SUITE_END();
}
