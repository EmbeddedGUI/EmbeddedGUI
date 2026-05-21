#include <string.h>

#include "egui.h"
#include "widget/egui_view_checkbox.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_checkbox_get_text_gap.h"

static egui_view_checkbox_t s_cb;

static void setup(void)
{
    memset(&s_cb, 0, sizeof(s_cb));
    egui_view_checkbox_init(EGUI_VIEW_OF(&s_cb), uicode_get_core());
}

static void test_checkbox_get_text_gap_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_checkbox_get_text_gap(EGUI_VIEW_OF(&s_cb)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_checkbox_get_icon_text_gap(EGUI_VIEW_OF(&s_cb)));
}

static void test_checkbox_get_text_gap_after_set(void)
{
    setup();
    egui_view_checkbox_set_icon_text_gap(EGUI_VIEW_OF(&s_cb), 10);
    EGUI_TEST_ASSERT_EQUAL_INT(10, (int)egui_view_checkbox_get_text_gap(EGUI_VIEW_OF(&s_cb)));
    EGUI_TEST_ASSERT_EQUAL_INT(10, (int)egui_view_checkbox_get_icon_text_gap(EGUI_VIEW_OF(&s_cb)));
}

static void test_checkbox_get_text_gap_update(void)
{
    setup();
    egui_view_checkbox_set_icon_text_gap(EGUI_VIEW_OF(&s_cb), 4);
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_checkbox_get_text_gap(EGUI_VIEW_OF(&s_cb)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_checkbox_get_icon_text_gap(EGUI_VIEW_OF(&s_cb)));
    egui_view_checkbox_set_icon_text_gap(EGUI_VIEW_OF(&s_cb), 12);
    EGUI_TEST_ASSERT_EQUAL_INT(12, (int)egui_view_checkbox_get_text_gap(EGUI_VIEW_OF(&s_cb)));
    EGUI_TEST_ASSERT_EQUAL_INT(12, (int)egui_view_checkbox_get_icon_text_gap(EGUI_VIEW_OF(&s_cb)));
}

static void test_checkbox_get_text_gap_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_checkbox_get_text_gap(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_checkbox_get_icon_text_gap(NULL));
}

void test_checkbox_get_text_gap_run(void)
{
    EGUI_TEST_SUITE_BEGIN(checkbox_get_text_gap);

    EGUI_TEST_RUN(test_checkbox_get_text_gap_default);
    EGUI_TEST_RUN(test_checkbox_get_text_gap_after_set);
    EGUI_TEST_RUN(test_checkbox_get_text_gap_update);
    EGUI_TEST_RUN(test_checkbox_get_text_gap_null_self);

    EGUI_TEST_SUITE_END();
}
