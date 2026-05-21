#include <string.h>

#include "egui.h"
#include "widget/egui_view_textinput.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_textinput_get_state.h"

static egui_view_textinput_t s_ti;

static void setup(void)
{
    memset(&s_ti, 0, sizeof(s_ti));
    egui_view_textinput_init(EGUI_VIEW_OF(&s_ti), uicode_get_core());
}

static void on_text_changed(egui_view_t *self, const char *text)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(text);
}

static void on_submit(egui_view_t *self, const char *text)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(text);
}

static void test_textinput_get_state_default(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_textinput_get_font(EGUI_VIEW_OF(&s_ti)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT_PRIMARY.full, (int)egui_view_textinput_get_text_color(EGUI_VIEW_OF(&s_ti)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_100, (int)egui_view_textinput_get_text_alpha(EGUI_VIEW_OF(&s_ti)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT_SECONDARY.full, (int)egui_view_textinput_get_placeholder_color(EGUI_VIEW_OF(&s_ti)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_100, (int)egui_view_textinput_get_placeholder_alpha(EGUI_VIEW_OF(&s_ti)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_PRIMARY.full, (int)egui_view_textinput_get_cursor_color(EGUI_VIEW_OF(&s_ti)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_textinput_get_cursor_active(EGUI_VIEW_OF(&s_ti)));
    EGUI_TEST_ASSERT_NULL(egui_view_textinput_get_on_text_changed(EGUI_VIEW_OF(&s_ti)));
    EGUI_TEST_ASSERT_NULL(egui_view_textinput_get_on_submit(EGUI_VIEW_OF(&s_ti)));
}

static void test_textinput_get_state_after_set(void)
{
    egui_color_t text_color = EGUI_COLOR_RED;
    egui_color_t placeholder_color = EGUI_COLOR_BLUE;
    egui_color_t cursor_color = EGUI_COLOR_GREEN;

    setup();
    egui_view_textinput_set_font(EGUI_VIEW_OF(&s_ti), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_textinput_set_text_color(EGUI_VIEW_OF(&s_ti), text_color, EGUI_ALPHA_80);
    egui_view_textinput_set_placeholder_color(EGUI_VIEW_OF(&s_ti), placeholder_color, EGUI_ALPHA_50);
    egui_view_textinput_set_cursor_color(EGUI_VIEW_OF(&s_ti), cursor_color);
    egui_view_textinput_set_cursor_active(EGUI_VIEW_OF(&s_ti), 1);
    egui_view_textinput_set_on_text_changed(EGUI_VIEW_OF(&s_ti), on_text_changed);
    egui_view_textinput_set_on_submit(EGUI_VIEW_OF(&s_ti), on_submit);

    EGUI_TEST_ASSERT_TRUE(egui_view_textinput_get_font(EGUI_VIEW_OF(&s_ti)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_EQUAL_INT((int)text_color.full, (int)egui_view_textinput_get_text_color(EGUI_VIEW_OF(&s_ti)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_80, (int)egui_view_textinput_get_text_alpha(EGUI_VIEW_OF(&s_ti)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)placeholder_color.full, (int)egui_view_textinput_get_placeholder_color(EGUI_VIEW_OF(&s_ti)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_50, (int)egui_view_textinput_get_placeholder_alpha(EGUI_VIEW_OF(&s_ti)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)cursor_color.full, (int)egui_view_textinput_get_cursor_color(EGUI_VIEW_OF(&s_ti)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_textinput_get_cursor_active(EGUI_VIEW_OF(&s_ti)));
    EGUI_TEST_ASSERT_TRUE(egui_view_textinput_get_on_text_changed(EGUI_VIEW_OF(&s_ti)) == on_text_changed);
    EGUI_TEST_ASSERT_TRUE(egui_view_textinput_get_on_submit(EGUI_VIEW_OF(&s_ti)) == on_submit);
}

static void test_textinput_get_state_clear_optional(void)
{
    setup();
    egui_view_textinput_set_font(EGUI_VIEW_OF(&s_ti), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_textinput_set_cursor_active(EGUI_VIEW_OF(&s_ti), 1);
    egui_view_textinput_set_on_text_changed(EGUI_VIEW_OF(&s_ti), on_text_changed);
    egui_view_textinput_set_on_submit(EGUI_VIEW_OF(&s_ti), on_submit);

    egui_view_textinput_set_font(EGUI_VIEW_OF(&s_ti), NULL);
    egui_view_textinput_set_cursor_active(EGUI_VIEW_OF(&s_ti), 0);
    egui_view_textinput_set_on_text_changed(EGUI_VIEW_OF(&s_ti), NULL);
    egui_view_textinput_set_on_submit(EGUI_VIEW_OF(&s_ti), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_textinput_get_font(EGUI_VIEW_OF(&s_ti)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_textinput_get_cursor_active(EGUI_VIEW_OF(&s_ti)));
    EGUI_TEST_ASSERT_NULL(egui_view_textinput_get_on_text_changed(EGUI_VIEW_OF(&s_ti)));
    EGUI_TEST_ASSERT_NULL(egui_view_textinput_get_on_submit(EGUI_VIEW_OF(&s_ti)));
}

static void test_textinput_get_state_cursor_active_normalizes(void)
{
    setup();
    egui_view_textinput_set_cursor_active(EGUI_VIEW_OF(&s_ti), 7);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_textinput_get_cursor_active(EGUI_VIEW_OF(&s_ti)));
}

static void test_textinput_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_textinput_get_font(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textinput_get_text_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textinput_get_text_alpha(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textinput_get_placeholder_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textinput_get_placeholder_alpha(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textinput_get_cursor_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_textinput_get_cursor_active(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_textinput_get_on_text_changed(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_textinput_get_on_submit(NULL));
}

void test_textinput_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(textinput_get_state);

    EGUI_TEST_RUN(test_textinput_get_state_default);
    EGUI_TEST_RUN(test_textinput_get_state_after_set);
    EGUI_TEST_RUN(test_textinput_get_state_clear_optional);
    EGUI_TEST_RUN(test_textinput_get_state_cursor_active_normalizes);
    EGUI_TEST_RUN(test_textinput_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
