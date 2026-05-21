#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_view_get_screen_pos.h"

static egui_view_t s_view;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_view, 0, sizeof(s_view));
    egui_view_init(&s_view, core);
}

/* After init, region_screen.location is zero. */
static void test_view_screen_pos_default_zero(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_get_screen_x(&s_view));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_get_screen_y(&s_view));
}

/* After manually setting region_screen location, getters reflect the values. */
static void test_view_screen_pos_after_set(void)
{
    setup();
    s_view.region_screen.location.x = 50;
    s_view.region_screen.location.y = 120;
    EGUI_TEST_ASSERT_EQUAL_INT(50,  (int)egui_view_get_screen_x(&s_view));
    EGUI_TEST_ASSERT_EQUAL_INT(120, (int)egui_view_get_screen_y(&s_view));
}

/* Update location and verify getters track changes. */
static void test_view_screen_pos_update(void)
{
    setup();
    s_view.region_screen.location.x = 10;
    s_view.region_screen.location.y = 20;
    EGUI_TEST_ASSERT_EQUAL_INT(10, (int)egui_view_get_screen_x(&s_view));
    s_view.region_screen.location.x = 99;
    EGUI_TEST_ASSERT_EQUAL_INT(99, (int)egui_view_get_screen_x(&s_view));
}

/* NULL self returns 0 without crash. */
static void test_view_screen_pos_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_get_screen_x(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_get_screen_y(NULL));
}

void test_view_get_screen_pos_run(void)
{
    EGUI_TEST_SUITE_BEGIN(view_get_screen_pos);

    EGUI_TEST_RUN(test_view_screen_pos_default_zero);
    EGUI_TEST_RUN(test_view_screen_pos_after_set);
    EGUI_TEST_RUN(test_view_screen_pos_update);
    EGUI_TEST_RUN(test_view_screen_pos_null_self);

    EGUI_TEST_SUITE_END();
}
