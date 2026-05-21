#include <string.h>

#include "egui.h"
#include "widget/egui_view_led.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_led_get_border_width.h"

static egui_view_led_t s_led;

static void setup(void)
{
    memset(&s_led, 0, sizeof(s_led));
    egui_view_led_init(EGUI_VIEW_OF(&s_led), uicode_get_core());
}

/* Default border_width after init is 2. */
static void test_led_get_border_width_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_led_get_border_width(EGUI_VIEW_OF(&s_led)));
}

/* After assigning border_width, getter returns the same value. */
static void test_led_get_border_width_after_set(void)
{
    setup();
    s_led.border_width = 4;
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_led_get_border_width(EGUI_VIEW_OF(&s_led)));
}

/* Updating border_width reflects in the getter. */
static void test_led_get_border_width_update(void)
{
    setup();
    s_led.border_width = 1;
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_led_get_border_width(EGUI_VIEW_OF(&s_led)));
    s_led.border_width = 6;
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_led_get_border_width(EGUI_VIEW_OF(&s_led)));
}

/* NULL self returns 0 without crash. */
static void test_led_get_border_width_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_led_get_border_width(NULL));
}

void test_led_get_border_width_run(void)
{
    EGUI_TEST_SUITE_BEGIN(led_get_border_width);

    EGUI_TEST_RUN(test_led_get_border_width_default);
    EGUI_TEST_RUN(test_led_get_border_width_after_set);
    EGUI_TEST_RUN(test_led_get_border_width_update);
    EGUI_TEST_RUN(test_led_get_border_width_null_self);

    EGUI_TEST_SUITE_END();
}
