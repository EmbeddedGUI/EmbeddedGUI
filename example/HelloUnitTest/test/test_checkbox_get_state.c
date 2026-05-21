#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_checkbox_get_state.h"

static egui_view_checkbox_t s_checkbox;
static const char s_mark_icon[] = "x";
static int s_checked_value;

static void on_checked(egui_view_t *self, int is_checked)
{
    EGUI_UNUSED(self);
    s_checked_value = is_checked;
}

static void setup(void)
{
    memset(&s_checkbox, 0, sizeof(s_checkbox));
    s_checked_value = -1;
    egui_view_checkbox_init(EGUI_VIEW_OF(&s_checkbox), uicode_get_core());
}

static void test_checkbox_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_checkbox_get_font(EGUI_VIEW_OF(&s_checkbox)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_VIEW_CHECKBOX_MARK_STYLE_VECTOR, (int)egui_view_checkbox_get_mark_style(EGUI_VIEW_OF(&s_checkbox)));
    EGUI_TEST_ASSERT_NOT_NULL(egui_view_checkbox_get_mark_icon(EGUI_VIEW_OF(&s_checkbox)));
    EGUI_TEST_ASSERT_NULL(egui_view_checkbox_get_icon_font(EGUI_VIEW_OF(&s_checkbox)));
    EGUI_TEST_ASSERT_NULL(egui_view_checkbox_get_on_checked_listener(EGUI_VIEW_OF(&s_checkbox)));
}

static void test_checkbox_get_state_after_setters(void)
{
    setup();
    egui_view_checkbox_set_font(EGUI_VIEW_OF(&s_checkbox), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_checkbox_set_mark_style(EGUI_VIEW_OF(&s_checkbox), EGUI_VIEW_CHECKBOX_MARK_STYLE_ICON);
    egui_view_checkbox_set_mark_icon(EGUI_VIEW_OF(&s_checkbox), s_mark_icon);
    egui_view_checkbox_set_icon_font(EGUI_VIEW_OF(&s_checkbox), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_checkbox_set_on_checked_listener(EGUI_VIEW_OF(&s_checkbox), on_checked);

    EGUI_TEST_ASSERT_TRUE(egui_view_checkbox_get_font(EGUI_VIEW_OF(&s_checkbox)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_VIEW_CHECKBOX_MARK_STYLE_ICON, (int)egui_view_checkbox_get_mark_style(EGUI_VIEW_OF(&s_checkbox)));
    EGUI_TEST_ASSERT_TRUE(egui_view_checkbox_get_mark_icon(EGUI_VIEW_OF(&s_checkbox)) == s_mark_icon);
    EGUI_TEST_ASSERT_TRUE(egui_view_checkbox_get_icon_font(EGUI_VIEW_OF(&s_checkbox)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(egui_view_checkbox_get_on_checked_listener(EGUI_VIEW_OF(&s_checkbox)) == on_checked);
    EGUI_TEST_ASSERT_EQUAL_INT(-1, s_checked_value);
}

static void test_checkbox_get_state_clear(void)
{
    setup();
    egui_view_checkbox_set_font(EGUI_VIEW_OF(&s_checkbox), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_checkbox_set_icon_font(EGUI_VIEW_OF(&s_checkbox), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_checkbox_set_mark_icon(EGUI_VIEW_OF(&s_checkbox), s_mark_icon);
    egui_view_checkbox_set_on_checked_listener(EGUI_VIEW_OF(&s_checkbox), on_checked);

    egui_view_checkbox_set_font(EGUI_VIEW_OF(&s_checkbox), NULL);
    egui_view_checkbox_set_icon_font(EGUI_VIEW_OF(&s_checkbox), NULL);
    egui_view_checkbox_set_mark_icon(EGUI_VIEW_OF(&s_checkbox), NULL);
    egui_view_checkbox_set_on_checked_listener(EGUI_VIEW_OF(&s_checkbox), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_checkbox_get_font(EGUI_VIEW_OF(&s_checkbox)));
    EGUI_TEST_ASSERT_NULL(egui_view_checkbox_get_icon_font(EGUI_VIEW_OF(&s_checkbox)));
    EGUI_TEST_ASSERT_NULL(egui_view_checkbox_get_mark_icon(EGUI_VIEW_OF(&s_checkbox)));
    EGUI_TEST_ASSERT_NULL(egui_view_checkbox_get_on_checked_listener(EGUI_VIEW_OF(&s_checkbox)));
}

static void test_checkbox_get_state_listener_fires_on_change(void)
{
    setup();
    egui_view_checkbox_set_on_checked_listener(EGUI_VIEW_OF(&s_checkbox), on_checked);
    egui_view_checkbox_set_checked(EGUI_VIEW_OF(&s_checkbox), 1);

    EGUI_TEST_ASSERT_EQUAL_INT(1, s_checked_value);
    EGUI_TEST_ASSERT_TRUE(egui_view_checkbox_get_on_checked_listener(EGUI_VIEW_OF(&s_checkbox)) == on_checked);
}

static void test_checkbox_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_checkbox_get_font(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_VIEW_CHECKBOX_MARK_STYLE_VECTOR, (int)egui_view_checkbox_get_mark_style(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_checkbox_get_mark_icon(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_checkbox_get_icon_font(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_checkbox_get_on_checked_listener(NULL));
}

void test_checkbox_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(checkbox_get_state);

    EGUI_TEST_RUN(test_checkbox_get_state_defaults);
    EGUI_TEST_RUN(test_checkbox_get_state_after_setters);
    EGUI_TEST_RUN(test_checkbox_get_state_clear);
    EGUI_TEST_RUN(test_checkbox_get_state_listener_fires_on_change);
    EGUI_TEST_RUN(test_checkbox_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
