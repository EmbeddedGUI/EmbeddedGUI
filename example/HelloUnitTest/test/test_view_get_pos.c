#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_view_get_pos.h"

static egui_view_t s_view;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_view, 0, sizeof(s_view));
    egui_view_init(&s_view, core);
}

/* get_x after set_position returns the set x. */
static void test_get_x_after_set_position(void)
{
    setup();
    egui_view_set_position(&s_view, 30, 20);
    EGUI_TEST_ASSERT_EQUAL_INT(30, (int)egui_view_get_x(&s_view));
}

/* get_y after set_position returns the set y. */
static void test_get_y_after_set_position(void)
{
    setup();
    egui_view_set_position(&s_view, 30, 20);
    EGUI_TEST_ASSERT_EQUAL_INT(20, (int)egui_view_get_y(&s_view));
}

/* get_x with NULL self returns 0. */
static void test_get_x_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_get_x(NULL));
}

/* get_y with NULL self returns 0. */
static void test_get_y_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_get_y(NULL));
}

/* Updating position is reflected by both getters. */
static void test_get_pos_update(void)
{
    setup();
    egui_view_set_position(&s_view, 10, 10);
    egui_view_set_position(&s_view, 50, 70);
    EGUI_TEST_ASSERT_EQUAL_INT(50, (int)egui_view_get_x(&s_view));
    EGUI_TEST_ASSERT_EQUAL_INT(70, (int)egui_view_get_y(&s_view));
}

void test_view_get_pos_run(void)
{
    EGUI_TEST_SUITE_BEGIN(view_get_pos);

    EGUI_TEST_RUN(test_get_x_after_set_position);
    EGUI_TEST_RUN(test_get_y_after_set_position);
    EGUI_TEST_RUN(test_get_x_null_self);
    EGUI_TEST_RUN(test_get_y_null_self);
    EGUI_TEST_RUN(test_get_pos_update);

    EGUI_TEST_SUITE_END();
}
