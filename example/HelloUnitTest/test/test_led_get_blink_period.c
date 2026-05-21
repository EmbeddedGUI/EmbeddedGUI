#include <string.h>

#include "egui.h"
#include "widget/egui_view_led.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_led_get_blink_period.h"

static egui_view_led_t s_led;

static void setup(void)
{
    memset(&s_led, 0, sizeof(s_led));
    egui_view_led_init(EGUI_VIEW_OF(&s_led), uicode_get_core());
}

/* Default blink_period after init is 500. */
static void test_led_get_blink_period_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(500, (int)egui_view_led_get_blink_period(EGUI_VIEW_OF(&s_led)));
}

/* After calling set_blink with 200, getter returns 200. */
static void test_led_get_blink_period_after_set(void)
{
    setup();
    egui_view_led_set_blink(EGUI_VIEW_OF(&s_led), 200);
    EGUI_TEST_ASSERT_EQUAL_INT(200, (int)egui_view_led_get_blink_period(EGUI_VIEW_OF(&s_led)));
}

/* After calling set_blink with another value, getter reflects it. */
static void test_led_get_blink_period_update(void)
{
    setup();
    egui_view_led_set_blink(EGUI_VIEW_OF(&s_led), 1000);
    EGUI_TEST_ASSERT_EQUAL_INT(1000, (int)egui_view_led_get_blink_period(EGUI_VIEW_OF(&s_led)));
}

/* NULL self returns 0 without crash. */
static void test_led_get_blink_period_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_led_get_blink_period(NULL));
}

void test_led_get_blink_period_run(void)
{
    EGUI_TEST_SUITE_BEGIN(led_get_blink_period);

    EGUI_TEST_RUN(test_led_get_blink_period_default);
    EGUI_TEST_RUN(test_led_get_blink_period_after_set);
    EGUI_TEST_RUN(test_led_get_blink_period_update);
    EGUI_TEST_RUN(test_led_get_blink_period_null_self);

    EGUI_TEST_SUITE_END();
}
