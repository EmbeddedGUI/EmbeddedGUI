#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_view_get_size.h"

static egui_view_t s_view;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_view, 0, sizeof(s_view));
    egui_view_init(&s_view, core);
}

/* get_width after set_size returns the set width. */
static void test_get_width_after_set_size(void)
{
    setup();
    egui_view_set_size(&s_view, 100, 50);
    EGUI_TEST_ASSERT_EQUAL_INT(100, (int)egui_view_get_width(&s_view));
}

/* get_height after set_size returns the set height. */
static void test_get_height_after_set_size(void)
{
    setup();
    egui_view_set_size(&s_view, 100, 50);
    EGUI_TEST_ASSERT_EQUAL_INT(50, (int)egui_view_get_height(&s_view));
}

/* get_width with NULL self returns 0. */
static void test_get_width_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_get_width(NULL));
}

/* get_height with NULL self returns 0. */
static void test_get_height_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_get_height(NULL));
}

/* Updating size is reflected by both getters. */
static void test_get_size_update(void)
{
    setup();
    egui_view_set_size(&s_view, 200, 80);
    egui_view_set_size(&s_view, 320, 240);
    EGUI_TEST_ASSERT_EQUAL_INT(320, (int)egui_view_get_width(&s_view));
    EGUI_TEST_ASSERT_EQUAL_INT(240, (int)egui_view_get_height(&s_view));
}

void test_view_get_size_run(void)
{
    EGUI_TEST_SUITE_BEGIN(view_get_size);

    EGUI_TEST_RUN(test_get_width_after_set_size);
    EGUI_TEST_RUN(test_get_height_after_set_size);
    EGUI_TEST_RUN(test_get_width_null_self);
    EGUI_TEST_RUN(test_get_height_null_self);
    EGUI_TEST_RUN(test_get_size_update);

    EGUI_TEST_SUITE_END();
}
