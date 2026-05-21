#include <string.h>

#include "egui.h"
#include "widget/egui_view_tab_bar.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_tab_bar_get_icon_text_gap.h"

static egui_view_tab_bar_t s_tab;

static void setup(void)
{
    memset(&s_tab, 0, sizeof(s_tab));
    egui_view_tab_bar_init(EGUI_VIEW_OF(&s_tab), uicode_get_core());
}

/* Default icon_text_gap after init is 2. */
static void test_tab_bar_get_icon_text_gap_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_tab_bar_get_icon_text_gap(EGUI_VIEW_OF(&s_tab)));
}

/* After calling set_icon_text_gap, getter returns the same value. */
static void test_tab_bar_get_icon_text_gap_after_set(void)
{
    setup();
    egui_view_tab_bar_set_icon_text_gap(EGUI_VIEW_OF(&s_tab), 6);
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_tab_bar_get_icon_text_gap(EGUI_VIEW_OF(&s_tab)));
}

/* Updating via set_icon_text_gap reflects in the getter. */
static void test_tab_bar_get_icon_text_gap_update(void)
{
    setup();
    egui_view_tab_bar_set_icon_text_gap(EGUI_VIEW_OF(&s_tab), 4);
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_tab_bar_get_icon_text_gap(EGUI_VIEW_OF(&s_tab)));
    egui_view_tab_bar_set_icon_text_gap(EGUI_VIEW_OF(&s_tab), 10);
    EGUI_TEST_ASSERT_EQUAL_INT(10, (int)egui_view_tab_bar_get_icon_text_gap(EGUI_VIEW_OF(&s_tab)));
}

/* NULL self returns 0 without crash. */
static void test_tab_bar_get_icon_text_gap_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tab_bar_get_icon_text_gap(NULL));
}

void test_tab_bar_get_icon_text_gap_run(void)
{
    EGUI_TEST_SUITE_BEGIN(tab_bar_get_icon_text_gap);

    EGUI_TEST_RUN(test_tab_bar_get_icon_text_gap_default);
    EGUI_TEST_RUN(test_tab_bar_get_icon_text_gap_after_set);
    EGUI_TEST_RUN(test_tab_bar_get_icon_text_gap_update);
    EGUI_TEST_RUN(test_tab_bar_get_icon_text_gap_null_self);

    EGUI_TEST_SUITE_END();
}
