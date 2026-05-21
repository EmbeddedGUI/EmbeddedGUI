#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_number_picker_get_state.h"

static egui_view_number_picker_t s_picker;
static const char s_icon_inc[] = "+";
static const char s_icon_dec[] = "-";
static int s_value_changed;

static void on_value_changed(egui_view_t *self, int16_t value)
{
    EGUI_UNUSED(self);
    s_value_changed = value;
}

static void setup(void)
{
    memset(&s_picker, 0, sizeof(s_picker));
    s_value_changed = -1;
    egui_view_number_picker_init(EGUI_VIEW_OF(&s_picker), uicode_get_core());
}

static void test_number_picker_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_number_picker_get_on_value_changed_listener(EGUI_VIEW_OF(&s_picker)));
    EGUI_TEST_ASSERT_TRUE(egui_view_number_picker_get_button_icon_inc(EGUI_VIEW_OF(&s_picker)) != NULL);
    EGUI_TEST_ASSERT_TRUE(egui_view_number_picker_get_button_icon_dec(EGUI_VIEW_OF(&s_picker)) != NULL);
    EGUI_TEST_ASSERT_NULL(egui_view_number_picker_get_icon_font(EGUI_VIEW_OF(&s_picker)));
}

static void test_number_picker_get_state_after_setters(void)
{
    setup();
    egui_view_number_picker_set_on_value_changed_listener(EGUI_VIEW_OF(&s_picker), on_value_changed);
    egui_view_number_picker_set_button_icons(EGUI_VIEW_OF(&s_picker), s_icon_inc, s_icon_dec);
    egui_view_number_picker_set_icon_font(EGUI_VIEW_OF(&s_picker), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    EGUI_TEST_ASSERT_TRUE(egui_view_number_picker_get_on_value_changed_listener(EGUI_VIEW_OF(&s_picker)) == on_value_changed);
    EGUI_TEST_ASSERT_TRUE(egui_view_number_picker_get_button_icon_inc(EGUI_VIEW_OF(&s_picker)) == s_icon_inc);
    EGUI_TEST_ASSERT_TRUE(egui_view_number_picker_get_button_icon_dec(EGUI_VIEW_OF(&s_picker)) == s_icon_dec);
    EGUI_TEST_ASSERT_TRUE(egui_view_number_picker_get_icon_font(EGUI_VIEW_OF(&s_picker)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_EQUAL_INT(-1, s_value_changed);
}

static void test_number_picker_get_state_clear(void)
{
    setup();
    egui_view_number_picker_set_on_value_changed_listener(EGUI_VIEW_OF(&s_picker), on_value_changed);
    egui_view_number_picker_set_button_icons(EGUI_VIEW_OF(&s_picker), s_icon_inc, s_icon_dec);
    egui_view_number_picker_set_icon_font(EGUI_VIEW_OF(&s_picker), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_number_picker_set_on_value_changed_listener(EGUI_VIEW_OF(&s_picker), NULL);
    egui_view_number_picker_set_button_icons(EGUI_VIEW_OF(&s_picker), NULL, NULL);
    egui_view_number_picker_set_icon_font(EGUI_VIEW_OF(&s_picker), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_number_picker_get_on_value_changed_listener(EGUI_VIEW_OF(&s_picker)));
    EGUI_TEST_ASSERT_NULL(egui_view_number_picker_get_button_icon_inc(EGUI_VIEW_OF(&s_picker)));
    EGUI_TEST_ASSERT_NULL(egui_view_number_picker_get_button_icon_dec(EGUI_VIEW_OF(&s_picker)));
    EGUI_TEST_ASSERT_NULL(egui_view_number_picker_get_icon_font(EGUI_VIEW_OF(&s_picker)));
}

static void test_number_picker_get_state_listener_fires_on_change(void)
{
    setup();
    egui_view_number_picker_set_on_value_changed_listener(EGUI_VIEW_OF(&s_picker), on_value_changed);
    egui_view_number_picker_set_value(EGUI_VIEW_OF(&s_picker), 23);

    EGUI_TEST_ASSERT_EQUAL_INT(23, s_value_changed);
    EGUI_TEST_ASSERT_TRUE(egui_view_number_picker_get_on_value_changed_listener(EGUI_VIEW_OF(&s_picker)) == on_value_changed);
}

static void test_number_picker_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_number_picker_get_on_value_changed_listener(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_number_picker_get_value(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_number_picker_get_button_icon_inc(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_number_picker_get_button_icon_dec(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_number_picker_get_icon_font(NULL));
}

void test_number_picker_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(number_picker_get_state);

    EGUI_TEST_RUN(test_number_picker_get_state_defaults);
    EGUI_TEST_RUN(test_number_picker_get_state_after_setters);
    EGUI_TEST_RUN(test_number_picker_get_state_clear);
    EGUI_TEST_RUN(test_number_picker_get_state_listener_fires_on_change);
    EGUI_TEST_RUN(test_number_picker_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
