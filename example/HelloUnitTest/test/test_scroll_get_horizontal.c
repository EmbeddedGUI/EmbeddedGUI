#include <string.h>

#include "egui.h"
#include "widget/egui_view_scroll.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_scroll_get_horizontal.h"

static egui_view_scroll_t s_scroll;

static void setup(void)
{
    memset(&s_scroll, 0, sizeof(s_scroll));
    egui_view_scroll_init(EGUI_VIEW_OF(&s_scroll), uicode_get_core());
}

/* Default is_horizontal after init is 0. */
static void test_scroll_get_horizontal_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scroll_get_horizontal(EGUI_VIEW_OF(&s_scroll)));
}

/* After setting horizontal, getter returns 1. */
static void test_scroll_get_horizontal_set(void)
{
    setup();
    egui_view_scroll_set_horizontal(EGUI_VIEW_OF(&s_scroll), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_scroll_get_horizontal(EGUI_VIEW_OF(&s_scroll)));
}

/* After clearing horizontal, getter returns 0. */
static void test_scroll_get_horizontal_cleared(void)
{
    setup();
    egui_view_scroll_set_horizontal(EGUI_VIEW_OF(&s_scroll), 1);
    egui_view_scroll_set_horizontal(EGUI_VIEW_OF(&s_scroll), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scroll_get_horizontal(EGUI_VIEW_OF(&s_scroll)));
}

/* NULL self returns 0 without crash. */
static void test_scroll_get_horizontal_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scroll_get_horizontal(NULL));
}

void test_scroll_get_horizontal_run(void)
{
    EGUI_TEST_SUITE_BEGIN(scroll_get_horizontal);

    EGUI_TEST_RUN(test_scroll_get_horizontal_default);
    EGUI_TEST_RUN(test_scroll_get_horizontal_set);
    EGUI_TEST_RUN(test_scroll_get_horizontal_cleared);
    EGUI_TEST_RUN(test_scroll_get_horizontal_null_self);

    EGUI_TEST_SUITE_END();
}
