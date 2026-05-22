#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_scroll_get_scroll_x.h"

static egui_view_scroll_t s_scroll;
static egui_view_group_t s_parent;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_parent, 0, sizeof(s_parent));
    memset(&s_scroll, 0, sizeof(s_scroll));
    egui_view_group_init(EGUI_VIEW_OF(&s_parent), core);
    egui_view_scroll_init(EGUI_VIEW_OF(&s_scroll), core);
    egui_view_scroll_set_size(EGUI_VIEW_OF(&s_scroll), 100, 100);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_parent), EGUI_VIEW_OF(&s_scroll));
}

/* After init, get_scroll_x returns 0. */
static void test_scroll_x_init_zero(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scroll_get_scroll_x(EGUI_VIEW_OF(&s_scroll)));
}

/* NULL self: get_scroll_x returns 0. */
static void test_scroll_x_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scroll_get_scroll_x(NULL));
}

/* get_scroll_y and get_scroll_x start at 0 independently. */
static void test_scroll_x_independent_of_y(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scroll_get_scroll_x(EGUI_VIEW_OF(&s_scroll)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scroll_get_scroll_y(EGUI_VIEW_OF(&s_scroll)));
}

/* Symmetry: scroll_x returns 0 when no horizontal scroll has occurred. */
static void test_scroll_x_no_horizontal_scroll(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_scroll_get_scroll_x(EGUI_VIEW_OF(&s_scroll)));
}

void test_scroll_get_scroll_x_run(void)
{
    EGUI_TEST_SUITE_BEGIN(scroll_get_scroll_x);

    EGUI_TEST_RUN(test_scroll_x_init_zero);
    EGUI_TEST_RUN(test_scroll_x_null_self);
    EGUI_TEST_RUN(test_scroll_x_independent_of_y);
    EGUI_TEST_RUN(test_scroll_x_no_horizontal_scroll);

    EGUI_TEST_SUITE_END();
}
