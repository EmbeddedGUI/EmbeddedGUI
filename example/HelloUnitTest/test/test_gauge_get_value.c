#include <string.h>

#include "egui.h"
#include "widget/egui_view_gauge.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_gauge_get_value.h"

static egui_view_gauge_t s_gauge;

static void setup(void)
{
    memset(&s_gauge, 0, sizeof(s_gauge));
    egui_view_gauge_init(EGUI_VIEW_OF(&s_gauge), uicode_get_core());
}

static void test_gauge_get_value_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_gauge_get_value(EGUI_VIEW_OF(&s_gauge)));
}

static void test_gauge_get_value_after_set(void)
{
    setup();
    egui_view_gauge_set_value(EGUI_VIEW_OF(&s_gauge), 75);
    EGUI_TEST_ASSERT_EQUAL_INT(75, (int)egui_view_gauge_get_value(EGUI_VIEW_OF(&s_gauge)));
}

static void test_gauge_get_value_update(void)
{
    setup();
    egui_view_gauge_set_value(EGUI_VIEW_OF(&s_gauge), 30);
    EGUI_TEST_ASSERT_EQUAL_INT(30, (int)egui_view_gauge_get_value(EGUI_VIEW_OF(&s_gauge)));
    egui_view_gauge_set_value(EGUI_VIEW_OF(&s_gauge), 90);
    EGUI_TEST_ASSERT_EQUAL_INT(90, (int)egui_view_gauge_get_value(EGUI_VIEW_OF(&s_gauge)));
}

static void test_gauge_get_value_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_gauge_get_value(NULL));
}

void test_gauge_get_value_run(void)
{
    EGUI_TEST_SUITE_BEGIN(gauge_get_value);

    EGUI_TEST_RUN(test_gauge_get_value_default);
    EGUI_TEST_RUN(test_gauge_get_value_after_set);
    EGUI_TEST_RUN(test_gauge_get_value_update);
    EGUI_TEST_RUN(test_gauge_get_value_null_self);

    EGUI_TEST_SUITE_END();
}
