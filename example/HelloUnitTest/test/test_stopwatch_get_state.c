#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_stopwatch_get_state.h"

static egui_view_stopwatch_t s_stopwatch;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_stopwatch, 0, sizeof(s_stopwatch));
    egui_view_stopwatch_init(EGUI_VIEW_OF(&s_stopwatch), core);
}

static void assert_time_text(const char *expected)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(expected, egui_view_stopwatch_get_time_text(EGUI_VIEW_OF(&s_stopwatch))));
    EGUI_TEST_ASSERT_TRUE(egui_view_label_get_text(EGUI_VIEW_OF(&s_stopwatch)) == egui_view_stopwatch_get_time_text(EGUI_VIEW_OF(&s_stopwatch)));
}

static void test_stopwatch_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_stopwatch_get_elapsed(EGUI_VIEW_OF(&s_stopwatch)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_VIEW_STOPWATCH_STATE_STOPPED, (int)egui_view_stopwatch_get_state(EGUI_VIEW_OF(&s_stopwatch)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_stopwatch_get_show_ms(EGUI_VIEW_OF(&s_stopwatch)));
    assert_time_text("00:00.00");
}

static void test_stopwatch_get_state_after_setters(void)
{
    setup();
    egui_view_stopwatch_set_state(EGUI_VIEW_OF(&s_stopwatch), EGUI_VIEW_STOPWATCH_STATE_RUNNING);
    egui_view_stopwatch_set_elapsed(EGUI_VIEW_OF(&s_stopwatch), 125340);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_VIEW_STOPWATCH_STATE_RUNNING, (int)egui_view_stopwatch_get_state(EGUI_VIEW_OF(&s_stopwatch)));
    EGUI_TEST_ASSERT_EQUAL_INT(125340, (int)egui_view_stopwatch_get_elapsed(EGUI_VIEW_OF(&s_stopwatch)));
    assert_time_text("02:05.34");

    egui_view_stopwatch_set_show_ms(EGUI_VIEW_OF(&s_stopwatch), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_stopwatch_get_show_ms(EGUI_VIEW_OF(&s_stopwatch)));
    assert_time_text("02:05");

    egui_view_stopwatch_set_elapsed(EGUI_VIEW_OF(&s_stopwatch), 3723000);
    EGUI_TEST_ASSERT_EQUAL_INT(3723000, (int)egui_view_stopwatch_get_elapsed(EGUI_VIEW_OF(&s_stopwatch)));
    assert_time_text("1:02:03");
}

static void test_stopwatch_get_state_hours_with_ms(void)
{
    setup();
    egui_view_stopwatch_set_elapsed(EGUI_VIEW_OF(&s_stopwatch), 3723450);

    EGUI_TEST_ASSERT_EQUAL_INT(3723450, (int)egui_view_stopwatch_get_elapsed(EGUI_VIEW_OF(&s_stopwatch)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_stopwatch_get_show_ms(EGUI_VIEW_OF(&s_stopwatch)));
    assert_time_text("1:02:03.45");
}

static void test_stopwatch_get_state_apply_params(void)
{
    static const egui_view_label_params_t params = {
            .region = {{3, 4}, {70, 20}},
            .text = "ignored",
    };

    setup();
    egui_view_stopwatch_apply_params(EGUI_VIEW_OF(&s_stopwatch), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_stopwatch)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_stopwatch)));
    EGUI_TEST_ASSERT_EQUAL_INT(70, (int)egui_view_get_width(EGUI_VIEW_OF(&s_stopwatch)));
    EGUI_TEST_ASSERT_EQUAL_INT(20, (int)egui_view_get_height(EGUI_VIEW_OF(&s_stopwatch)));

    egui_view_stopwatch_set_elapsed(EGUI_VIEW_OF(&s_stopwatch), 1200);
    assert_time_text("00:01.20");
}

static void test_stopwatch_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_stopwatch_get_elapsed(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_stopwatch_get_time_text(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_stopwatch_get_state(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_stopwatch_get_show_ms(NULL));
}

void test_stopwatch_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(stopwatch_get_state);

    EGUI_TEST_RUN(test_stopwatch_get_state_defaults);
    EGUI_TEST_RUN(test_stopwatch_get_state_after_setters);
    EGUI_TEST_RUN(test_stopwatch_get_state_hours_with_ms);
    EGUI_TEST_RUN(test_stopwatch_get_state_apply_params);
    EGUI_TEST_RUN(test_stopwatch_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
