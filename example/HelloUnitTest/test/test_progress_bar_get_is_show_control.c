#include <string.h>

#include "egui.h"
#include "widget/egui_view_progress_bar.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_progress_bar_get_is_show_control.h"

static egui_view_progress_bar_t s_pb;

static void setup(void)
{
    memset(&s_pb, 0, sizeof(s_pb));
    egui_view_progress_bar_init(EGUI_VIEW_OF(&s_pb), uicode_get_core());
}

static void test_progress_bar_get_is_show_control_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_progress_bar_get_is_show_control(EGUI_VIEW_OF(&s_pb)));
}

static void test_progress_bar_get_is_show_control_after_set(void)
{
    setup();
    s_pb.is_show_control = 1;
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_progress_bar_get_is_show_control(EGUI_VIEW_OF(&s_pb)));
}

static void test_progress_bar_get_is_show_control_update(void)
{
    setup();
    s_pb.is_show_control = 1;
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_progress_bar_get_is_show_control(EGUI_VIEW_OF(&s_pb)));
    s_pb.is_show_control = 0;
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_progress_bar_get_is_show_control(EGUI_VIEW_OF(&s_pb)));
}

static void test_progress_bar_get_is_show_control_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_progress_bar_get_is_show_control(NULL));
}

void test_progress_bar_get_is_show_control_run(void)
{
    EGUI_TEST_SUITE_BEGIN(progress_bar_get_is_show_control);

    EGUI_TEST_RUN(test_progress_bar_get_is_show_control_default);
    EGUI_TEST_RUN(test_progress_bar_get_is_show_control_after_set);
    EGUI_TEST_RUN(test_progress_bar_get_is_show_control_update);
    EGUI_TEST_RUN(test_progress_bar_get_is_show_control_null_self);

    EGUI_TEST_SUITE_END();
}
