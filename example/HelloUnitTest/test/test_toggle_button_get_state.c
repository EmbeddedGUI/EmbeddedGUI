#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_toggle_button_get_state.h"

static egui_view_toggle_button_t s_button;
static int s_toggled_value;

static void on_toggled(egui_view_t *self, uint8_t is_toggled)
{
    EGUI_UNUSED(self);
    s_toggled_value = is_toggled;
}

static void setup(void)
{
    memset(&s_button, 0, sizeof(s_button));
    s_toggled_value = -1;
    egui_view_toggle_button_init(EGUI_VIEW_OF(&s_button), uicode_get_core());
}

static void test_toggle_button_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_toggle_button_get_on_toggled_listener(EGUI_VIEW_OF(&s_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_toggle_button_get_toggled(EGUI_VIEW_OF(&s_button)));
}

static void test_toggle_button_get_state_after_setter(void)
{
    setup();
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&s_button), on_toggled);

    EGUI_TEST_ASSERT_TRUE(egui_view_toggle_button_get_on_toggled_listener(EGUI_VIEW_OF(&s_button)) == on_toggled);
    EGUI_TEST_ASSERT_EQUAL_INT(-1, s_toggled_value);
}

static void test_toggle_button_get_state_clear(void)
{
    setup();
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&s_button), on_toggled);
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&s_button), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_toggle_button_get_on_toggled_listener(EGUI_VIEW_OF(&s_button)));
}

static void test_toggle_button_get_state_listener_fires_on_change(void)
{
    setup();
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&s_button), on_toggled);
    egui_view_toggle_button_set_toggled(EGUI_VIEW_OF(&s_button), 1);

    EGUI_TEST_ASSERT_EQUAL_INT(1, s_toggled_value);
    EGUI_TEST_ASSERT_TRUE(egui_view_toggle_button_get_on_toggled_listener(EGUI_VIEW_OF(&s_button)) == on_toggled);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_toggle_button_get_toggled(EGUI_VIEW_OF(&s_button)));
}

static void test_toggle_button_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_toggle_button_get_on_toggled_listener(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_toggle_button_get_toggled(NULL));
}

void test_toggle_button_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(toggle_button_get_state);

    EGUI_TEST_RUN(test_toggle_button_get_state_defaults);
    EGUI_TEST_RUN(test_toggle_button_get_state_after_setter);
    EGUI_TEST_RUN(test_toggle_button_get_state_clear);
    EGUI_TEST_RUN(test_toggle_button_get_state_listener_fires_on_change);
    EGUI_TEST_RUN(test_toggle_button_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
