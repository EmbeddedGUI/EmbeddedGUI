#include <string.h>

#include "egui.h"
#include "widget/egui_view_tab_bar.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_tab_bar_get_tab_count.h"

static const char *s_tabs[] = {"A", "B", "C"};
static egui_view_tab_bar_t s_bar;

static void setup(void)
{
    memset(&s_bar, 0, sizeof(s_bar));
    egui_view_tab_bar_init(EGUI_VIEW_OF(&s_bar), uicode_get_core());
}

/* After set_tabs with 3, getter returns 3. */
static void test_tab_bar_get_tab_count_after_set(void)
{
    setup();
    egui_view_tab_bar_set_tabs(EGUI_VIEW_OF(&s_bar), s_tabs, 3);
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_tab_bar_get_tab_count(EGUI_VIEW_OF(&s_bar)));
}

/* After set_tabs with 2, getter returns 2. */
static void test_tab_bar_get_tab_count_after_update(void)
{
    setup();
    egui_view_tab_bar_set_tabs(EGUI_VIEW_OF(&s_bar), s_tabs, 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_tab_bar_get_tab_count(EGUI_VIEW_OF(&s_bar)));
}

/* Default tab_count after init is 0. */
static void test_tab_bar_get_tab_count_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tab_bar_get_tab_count(EGUI_VIEW_OF(&s_bar)));
}

/* NULL self returns 0 without crash. */
static void test_tab_bar_get_tab_count_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tab_bar_get_tab_count(NULL));
}

void test_tab_bar_get_tab_count_run(void)
{
    EGUI_TEST_SUITE_BEGIN(tab_bar_get_tab_count);

    EGUI_TEST_RUN(test_tab_bar_get_tab_count_after_set);
    EGUI_TEST_RUN(test_tab_bar_get_tab_count_after_update);
    EGUI_TEST_RUN(test_tab_bar_get_tab_count_default);
    EGUI_TEST_RUN(test_tab_bar_get_tab_count_null_self);

    EGUI_TEST_SUITE_END();
}
