#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_circular_pb_get_state.h"

static egui_view_circular_progress_bar_t s_bar;
static int s_progress_value;

static void on_progress_changed(egui_view_t *self, uint8_t progress)
{
    EGUI_UNUSED(self);
    s_progress_value = progress;
}

static void setup(void)
{
    memset(&s_bar, 0, sizeof(s_bar));
    s_progress_value = -1;
    egui_view_circular_progress_bar_init(EGUI_VIEW_OF(&s_bar), uicode_get_core());
}

static void test_circular_pb_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_circular_progress_bar_get_font(EGUI_VIEW_OF(&s_bar)));
    EGUI_TEST_ASSERT_NULL(egui_view_circular_progress_bar_get_on_progress_listener(EGUI_VIEW_OF(&s_bar)));
}

static void test_circular_pb_get_state_after_setters(void)
{
    setup();
    egui_view_circular_progress_bar_set_font(EGUI_VIEW_OF(&s_bar), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_circular_progress_bar_set_on_progress_listener(EGUI_VIEW_OF(&s_bar), on_progress_changed);

    EGUI_TEST_ASSERT_TRUE(egui_view_circular_progress_bar_get_font(EGUI_VIEW_OF(&s_bar)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(egui_view_circular_progress_bar_get_on_progress_listener(EGUI_VIEW_OF(&s_bar)) == on_progress_changed);
    EGUI_TEST_ASSERT_EQUAL_INT(-1, s_progress_value);
}

static void test_circular_pb_get_state_clear(void)
{
    setup();
    egui_view_circular_progress_bar_set_font(EGUI_VIEW_OF(&s_bar), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_circular_progress_bar_set_on_progress_listener(EGUI_VIEW_OF(&s_bar), on_progress_changed);

    egui_view_circular_progress_bar_set_font(EGUI_VIEW_OF(&s_bar), NULL);
    egui_view_circular_progress_bar_set_on_progress_listener(EGUI_VIEW_OF(&s_bar), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_circular_progress_bar_get_font(EGUI_VIEW_OF(&s_bar)));
    EGUI_TEST_ASSERT_NULL(egui_view_circular_progress_bar_get_on_progress_listener(EGUI_VIEW_OF(&s_bar)));
}

static void test_circular_pb_get_state_listener_fires_on_change(void)
{
    setup();
    egui_view_circular_progress_bar_set_on_progress_listener(EGUI_VIEW_OF(&s_bar), on_progress_changed);
    egui_view_circular_progress_bar_set_process(EGUI_VIEW_OF(&s_bar), 63);

    EGUI_TEST_ASSERT_EQUAL_INT(63, s_progress_value);
    EGUI_TEST_ASSERT_TRUE(egui_view_circular_progress_bar_get_on_progress_listener(EGUI_VIEW_OF(&s_bar)) == on_progress_changed);
}

static void test_circular_pb_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_circular_progress_bar_get_font(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_circular_progress_bar_get_on_progress_listener(NULL));
}

void test_circular_pb_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(circular_pb_get_state);

    EGUI_TEST_RUN(test_circular_pb_get_state_defaults);
    EGUI_TEST_RUN(test_circular_pb_get_state_after_setters);
    EGUI_TEST_RUN(test_circular_pb_get_state_clear);
    EGUI_TEST_RUN(test_circular_pb_get_state_listener_fires_on_change);
    EGUI_TEST_RUN(test_circular_pb_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
