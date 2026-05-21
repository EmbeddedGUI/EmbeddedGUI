#include <string.h>

#include "egui.h"
#include "widget/egui_view_circular_progress_bar.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_circular_progress_bar_get_process.h"

static egui_view_circular_progress_bar_t s_bar;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_bar, 0, sizeof(s_bar));
    egui_view_circular_progress_bar_init(EGUI_VIEW_OF(&s_bar), core);
}

/* Default process after init is 0. */
static void test_circular_bar_get_process_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_circular_progress_bar_get_process(EGUI_VIEW_OF(&s_bar)));
}

/* Process reflects the value set with set_process. */
static void test_circular_bar_get_process_after_set(void)
{
    setup();
    egui_view_circular_progress_bar_set_process(EGUI_VIEW_OF(&s_bar), 75);
    EGUI_TEST_ASSERT_EQUAL_INT(75, (int)egui_view_circular_progress_bar_get_process(EGUI_VIEW_OF(&s_bar)));
}

/* set_process clamps to 100. */
static void test_circular_bar_get_process_clamped(void)
{
    setup();
    egui_view_circular_progress_bar_set_process(EGUI_VIEW_OF(&s_bar), 200);
    EGUI_TEST_ASSERT_EQUAL_INT(100, (int)egui_view_circular_progress_bar_get_process(EGUI_VIEW_OF(&s_bar)));
}

/* NULL self returns 0 without crash. */
static void test_circular_bar_get_process_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_circular_progress_bar_get_process(NULL));
}

void test_circular_progress_bar_get_process_run(void)
{
    EGUI_TEST_SUITE_BEGIN(circular_progress_bar_get_process);

    EGUI_TEST_RUN(test_circular_bar_get_process_default);
    EGUI_TEST_RUN(test_circular_bar_get_process_after_set);
    EGUI_TEST_RUN(test_circular_bar_get_process_clamped);
    EGUI_TEST_RUN(test_circular_bar_get_process_null_self);

    EGUI_TEST_SUITE_END();
}
