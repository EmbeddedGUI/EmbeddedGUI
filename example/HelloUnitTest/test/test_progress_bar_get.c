#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_progress_bar_get.h"

static egui_view_progress_bar_t s_bar;
static egui_view_group_t        s_parent;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_parent, 0, sizeof(s_parent));
    memset(&s_bar, 0, sizeof(s_bar));
    egui_view_group_init(EGUI_VIEW_OF(&s_parent), core);
    egui_view_progress_bar_init(EGUI_VIEW_OF(&s_bar), core);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_parent), EGUI_VIEW_OF(&s_bar));
}

/* After init, process is the default value (10). */
static void test_pb_get_init_zero(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(10, (int)egui_view_progress_bar_get_process(EGUI_VIEW_OF(&s_bar)));
}

/* After set(50), get returns 50. */
static void test_pb_get_set_50(void)
{
    setup();
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&s_bar), 50);
    EGUI_TEST_ASSERT_EQUAL_INT(50, (int)egui_view_progress_bar_get_process(EGUI_VIEW_OF(&s_bar)));
}

/* After set(100), get returns 100. */
static void test_pb_get_set_100(void)
{
    setup();
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&s_bar), 100);
    EGUI_TEST_ASSERT_EQUAL_INT(100, (int)egui_view_progress_bar_get_process(EGUI_VIEW_OF(&s_bar)));
}

/* Clamping: set(200) clamps to 100, get returns 100. */
static void test_pb_get_clamp_over_100(void)
{
    setup();
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&s_bar), 200);
    EGUI_TEST_ASSERT_EQUAL_INT(100, (int)egui_view_progress_bar_get_process(EGUI_VIEW_OF(&s_bar)));
}

/* NULL self: get returns 0. */
static void test_pb_get_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_progress_bar_get_process(NULL));
}

void test_progress_bar_get_run(void)
{
    EGUI_TEST_SUITE_BEGIN(progress_bar_get);

    EGUI_TEST_RUN(test_pb_get_init_zero);
    EGUI_TEST_RUN(test_pb_get_set_50);
    EGUI_TEST_RUN(test_pb_get_set_100);
    EGUI_TEST_RUN(test_pb_get_clamp_over_100);
    EGUI_TEST_RUN(test_pb_get_null_self);

    EGUI_TEST_SUITE_END();
}
