#include <string.h>

#include "egui.h"
#include "widget/egui_view_led.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_led_get_border_color.h"

static egui_view_led_t s_led;

static void setup(void)
{
    memset(&s_led, 0, sizeof(s_led));
    egui_view_led_init(EGUI_VIEW_OF(&s_led), uicode_get_core());
}

/* Default border_color after init is EGUI_THEME_BORDER. */
static void test_led_get_border_color_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_BORDER.full, (int)egui_view_led_get_border_color(EGUI_VIEW_OF(&s_led)).full);
}

/* After assigning border_color, getter returns the same color. */
static void test_led_get_border_color_after_set(void)
{
    egui_color_t c;
    c.full = 0xAABBCCu;
    setup();
    s_led.border_color = c;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c.full, (int)egui_view_led_get_border_color(EGUI_VIEW_OF(&s_led)).full);
}

/* Updating border_color reflects in the getter. */
static void test_led_get_border_color_update(void)
{
    egui_color_t c1, c2;
    c1.full = 0xAABBCCu;
    c2.full = 0xDDEEFFu;
    setup();
    s_led.border_color = c1;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c1.full, (int)egui_view_led_get_border_color(EGUI_VIEW_OF(&s_led)).full);
    s_led.border_color = c2;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c2.full, (int)egui_view_led_get_border_color(EGUI_VIEW_OF(&s_led)).full);
}

/* NULL self returns zeroed color without crash. */
static void test_led_get_border_color_null_self(void)
{
    egui_color_t got = egui_view_led_get_border_color(NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)got.full);
}

void test_led_get_border_color_run(void)
{
    EGUI_TEST_SUITE_BEGIN(led_get_border_color);

    EGUI_TEST_RUN(test_led_get_border_color_default);
    EGUI_TEST_RUN(test_led_get_border_color_after_set);
    EGUI_TEST_RUN(test_led_get_border_color_update);
    EGUI_TEST_RUN(test_led_get_border_color_null_self);

    EGUI_TEST_SUITE_END();
}
