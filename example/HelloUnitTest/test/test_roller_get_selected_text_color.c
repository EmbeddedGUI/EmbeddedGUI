#include <string.h>

#include "egui.h"
#include "widget/egui_view_roller.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_roller_get_selected_text_color.h"

static egui_view_roller_t s_roller;

static void setup(void)
{
    memset(&s_roller, 0, sizeof(s_roller));
    egui_view_roller_init(EGUI_VIEW_OF(&s_roller), uicode_get_core());
}

static void test_roller_get_selected_text_color_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT_PRIMARY.full,
                               (int)egui_view_roller_get_selected_text_color(EGUI_VIEW_OF(&s_roller)).full);
}

static void test_roller_get_selected_text_color_after_set(void)
{
    egui_color_t c;
    c.full = 0x334455u;
    setup();
    s_roller.selected_text_color = c;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c.full,
                               (int)egui_view_roller_get_selected_text_color(EGUI_VIEW_OF(&s_roller)).full);
}

static void test_roller_get_selected_text_color_update(void)
{
    egui_color_t c1, c2;
    c1.full = 0x334455u;
    c2.full = 0x667788u;
    setup();
    s_roller.selected_text_color = c1;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c1.full,
                               (int)egui_view_roller_get_selected_text_color(EGUI_VIEW_OF(&s_roller)).full);
    s_roller.selected_text_color = c2;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c2.full,
                               (int)egui_view_roller_get_selected_text_color(EGUI_VIEW_OF(&s_roller)).full);
}

static void test_roller_get_selected_text_color_null_self(void)
{
    egui_color_t got = egui_view_roller_get_selected_text_color(NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)got.full);
}

void test_roller_get_selected_text_color_run(void)
{
    EGUI_TEST_SUITE_BEGIN(roller_get_selected_text_color);

    EGUI_TEST_RUN(test_roller_get_selected_text_color_default);
    EGUI_TEST_RUN(test_roller_get_selected_text_color_after_set);
    EGUI_TEST_RUN(test_roller_get_selected_text_color_update);
    EGUI_TEST_RUN(test_roller_get_selected_text_color_null_self);

    EGUI_TEST_SUITE_END();
}
