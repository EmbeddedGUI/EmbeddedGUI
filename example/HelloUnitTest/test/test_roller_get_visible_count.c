#include <string.h>

#include "egui.h"
#include "widget/egui_view_roller.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_roller_get_visible_count.h"

static egui_view_roller_t s_roller;

static void setup(void)
{
    memset(&s_roller, 0, sizeof(s_roller));
    egui_view_roller_init(EGUI_VIEW_OF(&s_roller), uicode_get_core());
}

static void test_roller_get_visible_count_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_roller_get_visible_count(EGUI_VIEW_OF(&s_roller)));
}

static void test_roller_get_visible_count_after_set(void)
{
    setup();
    s_roller.visible_count = 5;
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_roller_get_visible_count(EGUI_VIEW_OF(&s_roller)));
}

static void test_roller_get_visible_count_update(void)
{
    setup();
    s_roller.visible_count = 5;
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_roller_get_visible_count(EGUI_VIEW_OF(&s_roller)));
    s_roller.visible_count = 7;
    EGUI_TEST_ASSERT_EQUAL_INT(7, (int)egui_view_roller_get_visible_count(EGUI_VIEW_OF(&s_roller)));
}

static void test_roller_get_visible_count_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_roller_get_visible_count(NULL));
}

void test_roller_get_visible_count_run(void)
{
    EGUI_TEST_SUITE_BEGIN(roller_get_visible_count);

    EGUI_TEST_RUN(test_roller_get_visible_count_default);
    EGUI_TEST_RUN(test_roller_get_visible_count_after_set);
    EGUI_TEST_RUN(test_roller_get_visible_count_update);
    EGUI_TEST_RUN(test_roller_get_visible_count_null_self);

    EGUI_TEST_SUITE_END();
}
