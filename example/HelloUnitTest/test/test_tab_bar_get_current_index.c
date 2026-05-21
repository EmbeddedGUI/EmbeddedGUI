#include <string.h>

#include "egui.h"
#include "widget/egui_view_tab_bar.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_tab_bar_get_current_index.h"

static egui_view_tab_bar_t s_tab_bar;
static const char *s_tabs[] = {"A", "B", "C"};

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_tab_bar, 0, sizeof(s_tab_bar));
    egui_view_tab_bar_init(EGUI_VIEW_OF(&s_tab_bar), core);
    egui_view_tab_bar_set_tabs(EGUI_VIEW_OF(&s_tab_bar), s_tabs, 3);
}

/* Default current index after init is 0. */
static void test_tab_bar_get_current_index_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tab_bar_get_current_index(EGUI_VIEW_OF(&s_tab_bar)));
}

/* After set_current_index, get returns the new value. */
static void test_tab_bar_get_current_index_after_set(void)
{
    setup();
    egui_view_tab_bar_set_current_index(EGUI_VIEW_OF(&s_tab_bar), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_tab_bar_get_current_index(EGUI_VIEW_OF(&s_tab_bar)));
}

/* Index can be updated multiple times. */
static void test_tab_bar_get_current_index_update(void)
{
    setup();
    egui_view_tab_bar_set_current_index(EGUI_VIEW_OF(&s_tab_bar), 1);
    egui_view_tab_bar_set_current_index(EGUI_VIEW_OF(&s_tab_bar), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_tab_bar_get_current_index(EGUI_VIEW_OF(&s_tab_bar)));
}

/* NULL self returns 0 without crash. */
static void test_tab_bar_get_current_index_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tab_bar_get_current_index(NULL));
}

void test_tab_bar_get_current_index_run(void)
{
    EGUI_TEST_SUITE_BEGIN(tab_bar_get_current_index);

    EGUI_TEST_RUN(test_tab_bar_get_current_index_default);
    EGUI_TEST_RUN(test_tab_bar_get_current_index_after_set);
    EGUI_TEST_RUN(test_tab_bar_get_current_index_update);
    EGUI_TEST_RUN(test_tab_bar_get_current_index_null_self);

    EGUI_TEST_SUITE_END();
}
