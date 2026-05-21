#include <string.h>

#include "egui.h"
#include "widget/egui_view_led.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_led_get_is_blinking.h"

static egui_view_led_t s_led;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_led, 0, sizeof(s_led));
    egui_view_led_init(EGUI_VIEW_OF(&s_led), core);
}

/* Default state after init is not blinking. */
static void test_led_get_is_blinking_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_led_get_is_blinking(EGUI_VIEW_OF(&s_led)));
}

/* After set_blink, get_is_blinking returns 1. */
static void test_led_get_is_blinking_after_set_blink(void)
{
    setup();
    egui_view_led_set_blink(EGUI_VIEW_OF(&s_led), 500);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_led_get_is_blinking(EGUI_VIEW_OF(&s_led)));
}

/* After set_blink then stop_blink, get_is_blinking returns 0. */
static void test_led_get_is_blinking_after_stop_blink(void)
{
    setup();
    egui_view_led_set_blink(EGUI_VIEW_OF(&s_led), 500);
    egui_view_led_stop_blink(EGUI_VIEW_OF(&s_led));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_led_get_is_blinking(EGUI_VIEW_OF(&s_led)));
}

/* NULL self returns 0 without crash. */
static void test_led_get_is_blinking_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_led_get_is_blinking(NULL));
}

void test_led_get_is_blinking_run(void)
{
    EGUI_TEST_SUITE_BEGIN(led_get_is_blinking);

    EGUI_TEST_RUN(test_led_get_is_blinking_default);
    EGUI_TEST_RUN(test_led_get_is_blinking_after_set_blink);
    EGUI_TEST_RUN(test_led_get_is_blinking_after_stop_blink);
    EGUI_TEST_RUN(test_led_get_is_blinking_null_self);

    EGUI_TEST_SUITE_END();
}
