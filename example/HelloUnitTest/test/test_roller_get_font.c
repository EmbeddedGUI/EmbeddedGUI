#include <string.h>

#include "egui.h"
#include "widget/egui_view_roller.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_roller_get_font.h"

static egui_view_roller_t s_roller;

static void setup(void)
{
    memset(&s_roller, 0, sizeof(s_roller));
    egui_view_roller_init(EGUI_VIEW_OF(&s_roller), uicode_get_core());
}

/* After init, font is EGUI_CONFIG_FONT_DEFAULT (non-NULL). */
static void test_roller_get_font_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_roller_get_font(EGUI_VIEW_OF(&s_roller)) != NULL);
}

/* After assigning a different font pointer, getter returns it. */
static void test_roller_get_font_after_set(void)
{
    const egui_font_t *f = (const egui_font_t *)0x12345678u;
    setup();
    s_roller.font = f;
    EGUI_TEST_ASSERT_EQUAL_INT((int)(uintptr_t)f,
                               (int)(uintptr_t)egui_view_roller_get_font(EGUI_VIEW_OF(&s_roller)));
}

/* Updating font pointer reflects in the getter. */
static void test_roller_get_font_update(void)
{
    const egui_font_t *f1 = (const egui_font_t *)0x11111111u;
    const egui_font_t *f2 = (const egui_font_t *)0x22222222u;
    setup();
    s_roller.font = f1;
    EGUI_TEST_ASSERT_EQUAL_INT((int)(uintptr_t)f1,
                               (int)(uintptr_t)egui_view_roller_get_font(EGUI_VIEW_OF(&s_roller)));
    s_roller.font = f2;
    EGUI_TEST_ASSERT_EQUAL_INT((int)(uintptr_t)f2,
                               (int)(uintptr_t)egui_view_roller_get_font(EGUI_VIEW_OF(&s_roller)));
}

/* NULL self returns NULL without crash. */
static void test_roller_get_font_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_roller_get_font(NULL) == NULL);
}

void test_roller_get_font_run(void)
{
    EGUI_TEST_SUITE_BEGIN(roller_get_font);

    EGUI_TEST_RUN(test_roller_get_font_default);
    EGUI_TEST_RUN(test_roller_get_font_after_set);
    EGUI_TEST_RUN(test_roller_get_font_update);
    EGUI_TEST_RUN(test_roller_get_font_null_self);

    EGUI_TEST_SUITE_END();
}
