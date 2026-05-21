#include <string.h>

#include "egui.h"
#include "widget/egui_view_scroll.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_scroll_scroll_by.h"

static egui_view_scroll_t s_scroll;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_scroll, 0, sizeof(s_scroll));
    egui_view_scroll_init(EGUI_VIEW_OF(&s_scroll), core);
}

/* NULL self for scroll_by_y does not crash. */
static void test_scroll_by_null_self_y(void)
{
    egui_view_scroll_scroll_by_y(NULL, 50, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(1, 1);
}

/* NULL self for scroll_by_x does not crash. */
static void test_scroll_by_null_self_x(void)
{
    egui_view_scroll_scroll_by_x(NULL, 50, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(1, 1);
}

/* scroll_by_y with delta 0 keeps position at 0. */
static void test_scroll_by_y_zero_delta(void)
{
    setup();
    egui_view_scroll_scroll_by_y(EGUI_VIEW_OF(&s_scroll), 0, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scroll_get_scroll_y(EGUI_VIEW_OF(&s_scroll)));
}

/* scroll_by_x with delta 0 keeps position at 0. */
static void test_scroll_by_x_zero_delta(void)
{
    setup();
    egui_view_scroll_scroll_by_x(EGUI_VIEW_OF(&s_scroll), 0, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scroll_get_scroll_x(EGUI_VIEW_OF(&s_scroll)));
}

void test_scroll_scroll_by_run(void)
{
    EGUI_TEST_SUITE_BEGIN(scroll_scroll_by);

    EGUI_TEST_RUN(test_scroll_by_null_self_y);
    EGUI_TEST_RUN(test_scroll_by_null_self_x);
    EGUI_TEST_RUN(test_scroll_by_y_zero_delta);
    EGUI_TEST_RUN(test_scroll_by_x_zero_delta);

    EGUI_TEST_SUITE_END();
}
