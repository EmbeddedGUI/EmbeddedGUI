#include <string.h>

#include "egui.h"
#include "widget/egui_view_led.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_led_get_on_color.h"

static egui_view_led_t s_led;

static void setup(void)
{
    memset(&s_led, 0, sizeof(s_led));
    egui_view_led_init(EGUI_VIEW_OF(&s_led), uicode_get_core());
}

/* Default on_color after init is EGUI_THEME_PRIMARY. */
static void test_led_get_on_color_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_PRIMARY.full,
                               (int)egui_view_led_get_on_color(EGUI_VIEW_OF(&s_led)).full);
}

/* After calling set_colors, getter returns the on_color supplied. */
static void test_led_get_on_color_after_set(void)
{
    egui_color_t c_on, c_off;
    c_on.full = 0x001122u;
    c_off.full = 0x334455u;
    setup();
    egui_view_led_set_colors(EGUI_VIEW_OF(&s_led), c_on, c_off);
    EGUI_TEST_ASSERT_EQUAL_INT((int)c_on.full,
                               (int)egui_view_led_get_on_color(EGUI_VIEW_OF(&s_led)).full);
}

/* Updating via set_colors reflects in the getter. */
static void test_led_get_on_color_update(void)
{
    egui_color_t c1, c2, c_off;
    c1.full = 0x001122u;
    c2.full = 0x667788u;
    c_off.full = 0x000000u;
    setup();
    egui_view_led_set_colors(EGUI_VIEW_OF(&s_led), c1, c_off);
    EGUI_TEST_ASSERT_EQUAL_INT((int)c1.full,
                               (int)egui_view_led_get_on_color(EGUI_VIEW_OF(&s_led)).full);
    egui_view_led_set_colors(EGUI_VIEW_OF(&s_led), c2, c_off);
    EGUI_TEST_ASSERT_EQUAL_INT((int)c2.full,
                               (int)egui_view_led_get_on_color(EGUI_VIEW_OF(&s_led)).full);
}

/* NULL self returns zeroed color without crash. */
static void test_led_get_on_color_null_self(void)
{
    egui_color_t got = egui_view_led_get_on_color(NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)got.full);
}

void test_led_get_on_color_run(void)
{
    EGUI_TEST_SUITE_BEGIN(led_get_on_color);

    EGUI_TEST_RUN(test_led_get_on_color_default);
    EGUI_TEST_RUN(test_led_get_on_color_after_set);
    EGUI_TEST_RUN(test_led_get_on_color_update);
    EGUI_TEST_RUN(test_led_get_on_color_null_self);

    EGUI_TEST_SUITE_END();
}
