#include <string.h>

#include "egui.h"
#include "widget/egui_view_checkbox.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_checkbox_get_box_fill_color.h"

static egui_view_checkbox_t s_cb;

static void setup(void)
{
    memset(&s_cb, 0, sizeof(s_cb));
    egui_view_checkbox_init(EGUI_VIEW_OF(&s_cb), uicode_get_core());
}

static void test_checkbox_get_box_fill_color_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_PRIMARY.full,
                               (int)egui_view_checkbox_get_box_fill_color(EGUI_VIEW_OF(&s_cb)).full);
}

static void test_checkbox_get_box_fill_color_after_set(void)
{
    egui_color_t c;
    c.full = 0x112233u;
    setup();
    s_cb.box_fill_color = c;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c.full,
                               (int)egui_view_checkbox_get_box_fill_color(EGUI_VIEW_OF(&s_cb)).full);
}

static void test_checkbox_get_box_fill_color_update(void)
{
    egui_color_t c1, c2;
    c1.full = 0x112233u;
    c2.full = 0x445566u;
    setup();
    s_cb.box_fill_color = c1;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c1.full,
                               (int)egui_view_checkbox_get_box_fill_color(EGUI_VIEW_OF(&s_cb)).full);
    s_cb.box_fill_color = c2;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c2.full,
                               (int)egui_view_checkbox_get_box_fill_color(EGUI_VIEW_OF(&s_cb)).full);
}

static void test_checkbox_get_box_fill_color_null_self(void)
{
    egui_color_t got = egui_view_checkbox_get_box_fill_color(NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)got.full);
}

void test_checkbox_get_box_fill_color_run(void)
{
    EGUI_TEST_SUITE_BEGIN(checkbox_get_box_fill_color);

    EGUI_TEST_RUN(test_checkbox_get_box_fill_color_default);
    EGUI_TEST_RUN(test_checkbox_get_box_fill_color_after_set);
    EGUI_TEST_RUN(test_checkbox_get_box_fill_color_update);
    EGUI_TEST_RUN(test_checkbox_get_box_fill_color_null_self);

    EGUI_TEST_SUITE_END();
}
