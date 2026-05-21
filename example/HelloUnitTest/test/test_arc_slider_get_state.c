#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_arc_slider_get_state.h"

static egui_view_arc_slider_t s_arc;
static int s_value_changed;

static void on_value_changed(egui_view_t *self, uint8_t value)
{
    EGUI_UNUSED(self);
    s_value_changed = value;
}

static void setup(void)
{
    memset(&s_arc, 0, sizeof(s_arc));
    s_value_changed = -1;
    egui_view_arc_slider_init(EGUI_VIEW_OF(&s_arc), uicode_get_core());
}

static void test_arc_slider_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_arc_slider_get_on_value_changed_listener(EGUI_VIEW_OF(&s_arc)));
}

static void test_arc_slider_get_state_after_setter(void)
{
    setup();
    egui_view_arc_slider_set_on_value_changed_listener(EGUI_VIEW_OF(&s_arc), on_value_changed);

    EGUI_TEST_ASSERT_TRUE(egui_view_arc_slider_get_on_value_changed_listener(EGUI_VIEW_OF(&s_arc)) == on_value_changed);
    EGUI_TEST_ASSERT_EQUAL_INT(-1, s_value_changed);
}

static void test_arc_slider_get_state_clear(void)
{
    setup();
    egui_view_arc_slider_set_on_value_changed_listener(EGUI_VIEW_OF(&s_arc), on_value_changed);
    egui_view_arc_slider_set_on_value_changed_listener(EGUI_VIEW_OF(&s_arc), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_arc_slider_get_on_value_changed_listener(EGUI_VIEW_OF(&s_arc)));
}

static void test_arc_slider_get_state_listener_fires_on_change(void)
{
    setup();
    egui_view_arc_slider_set_on_value_changed_listener(EGUI_VIEW_OF(&s_arc), on_value_changed);
    egui_view_arc_slider_set_value(EGUI_VIEW_OF(&s_arc), 64);

    EGUI_TEST_ASSERT_EQUAL_INT(64, s_value_changed);
    EGUI_TEST_ASSERT_TRUE(egui_view_arc_slider_get_on_value_changed_listener(EGUI_VIEW_OF(&s_arc)) == on_value_changed);
}

static void test_arc_slider_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_arc_slider_get_on_value_changed_listener(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_arc_slider_get_value(NULL));
}

void test_arc_slider_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(arc_slider_get_state);

    EGUI_TEST_RUN(test_arc_slider_get_state_defaults);
    EGUI_TEST_RUN(test_arc_slider_get_state_after_setter);
    EGUI_TEST_RUN(test_arc_slider_get_state_clear);
    EGUI_TEST_RUN(test_arc_slider_get_state_listener_fires_on_change);
    EGUI_TEST_RUN(test_arc_slider_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
