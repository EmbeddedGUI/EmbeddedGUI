#include <string.h>

#include "egui.h"
#include "widget/egui_view_number_picker.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_number_picker_get_button_color.h"

static egui_view_number_picker_t s_np;

static void setup(void)
{
    memset(&s_np, 0, sizeof(s_np));
    egui_view_number_picker_init(EGUI_VIEW_OF(&s_np), uicode_get_core());
}

/* Default button_color after init is EGUI_THEME_PRIMARY. */
static void test_number_picker_get_button_color_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_PRIMARY.full,
                               (int)egui_view_number_picker_get_button_color(EGUI_VIEW_OF(&s_np)).full);
}

/* After assigning button_color, getter returns the same color. */
static void test_number_picker_get_button_color_after_set(void)
{
    egui_color_t c;
    c.full = 0x334455u;
    setup();
    s_np.button_color = c;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c.full,
                               (int)egui_view_number_picker_get_button_color(EGUI_VIEW_OF(&s_np)).full);
}

/* Updating button_color reflects in the getter. */
static void test_number_picker_get_button_color_update(void)
{
    egui_color_t c1, c2;
    c1.full = 0x334455u;
    c2.full = 0x667788u;
    setup();
    s_np.button_color = c1;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c1.full,
                               (int)egui_view_number_picker_get_button_color(EGUI_VIEW_OF(&s_np)).full);
    s_np.button_color = c2;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c2.full,
                               (int)egui_view_number_picker_get_button_color(EGUI_VIEW_OF(&s_np)).full);
}

/* NULL self returns zeroed color without crash. */
static void test_number_picker_get_button_color_null_self(void)
{
    egui_color_t got = egui_view_number_picker_get_button_color(NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)got.full);
}

void test_number_picker_get_button_color_run(void)
{
    EGUI_TEST_SUITE_BEGIN(number_picker_get_button_color);

    EGUI_TEST_RUN(test_number_picker_get_button_color_default);
    EGUI_TEST_RUN(test_number_picker_get_button_color_after_set);
    EGUI_TEST_RUN(test_number_picker_get_button_color_update);
    EGUI_TEST_RUN(test_number_picker_get_button_color_null_self);

    EGUI_TEST_SUITE_END();
}
