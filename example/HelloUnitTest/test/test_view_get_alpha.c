#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_view_get_alpha.h"

static egui_view_t s_view;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_view, 0, sizeof(s_view));
    egui_view_init(&s_view, core);
}

/* Default alpha after init is EGUI_ALPHA_100. */
static void test_get_alpha_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_100, (int)egui_view_get_alpha(&s_view));
}

/* get_alpha returns the value set by set_alpha. */
static void test_get_alpha_after_set(void)
{
    setup();
    egui_view_set_alpha(&s_view, EGUI_ALPHA_50);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_50, (int)egui_view_get_alpha(&s_view));
}

/* set/get round-trip with value 0 (transparent). */
static void test_get_alpha_zero(void)
{
    setup();
    egui_view_set_alpha(&s_view, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_get_alpha(&s_view));
}

/* NULL self: returns EGUI_ALPHA_100 without crash. */
static void test_get_alpha_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_100, (int)egui_view_get_alpha(NULL));
}

/* Multiple set/get transitions. */
static void test_get_alpha_multiple_sets(void)
{
    setup();
    egui_view_set_alpha(&s_view, 100);
    egui_view_set_alpha(&s_view, 200);
    EGUI_TEST_ASSERT_EQUAL_INT(200, (int)egui_view_get_alpha(&s_view));
}

void test_view_get_alpha_run(void)
{
    EGUI_TEST_SUITE_BEGIN(view_get_alpha);

    EGUI_TEST_RUN(test_get_alpha_default);
    EGUI_TEST_RUN(test_get_alpha_after_set);
    EGUI_TEST_RUN(test_get_alpha_zero);
    EGUI_TEST_RUN(test_get_alpha_null_self);
    EGUI_TEST_RUN(test_get_alpha_multiple_sets);

    EGUI_TEST_SUITE_END();
}
