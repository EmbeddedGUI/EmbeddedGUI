#include <string.h>

#include "egui.h"
#include "widget/egui_view_led.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_led_get_is_on.h"

static egui_view_led_t s_led;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_led, 0, sizeof(s_led));
    egui_view_led_init(EGUI_VIEW_OF(&s_led), core);
}

/* Default state after init is off. */
static void test_led_get_is_on_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_led_get_is_on(EGUI_VIEW_OF(&s_led)));
}

/* After set_on, get_is_on returns 1. */
static void test_led_get_is_on_after_set_on(void)
{
    setup();
    egui_view_led_set_on(EGUI_VIEW_OF(&s_led));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_led_get_is_on(EGUI_VIEW_OF(&s_led)));
}

/* After set_on then set_off, get_is_on returns 0. */
static void test_led_get_is_on_after_set_off(void)
{
    setup();
    egui_view_led_set_on(EGUI_VIEW_OF(&s_led));
    egui_view_led_set_off(EGUI_VIEW_OF(&s_led));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_led_get_is_on(EGUI_VIEW_OF(&s_led)));
}

/* NULL self returns 0 without crash. */
static void test_led_get_is_on_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_led_get_is_on(NULL));
}

void test_led_get_is_on_run(void)
{
    EGUI_TEST_SUITE_BEGIN(led_get_is_on);

    EGUI_TEST_RUN(test_led_get_is_on_default);
    EGUI_TEST_RUN(test_led_get_is_on_after_set_on);
    EGUI_TEST_RUN(test_led_get_is_on_after_set_off);
    EGUI_TEST_RUN(test_led_get_is_on_null_self);

    EGUI_TEST_SUITE_END();
}
