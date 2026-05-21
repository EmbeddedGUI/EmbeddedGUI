#include <string.h>

#include "egui.h"
#include "widget/egui_view_textblock.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_textblock_get_state.h"

static egui_view_textblock_t s_textblock;

static void setup(void)
{
    memset(&s_textblock, 0, sizeof(s_textblock));
    egui_view_textblock_init(EGUI_VIEW_OF(&s_textblock), uicode_get_core());
}

static void test_textblock_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_textblock_get_line_space(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_max_lines(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_auto_height(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_textblock_get_auto_wrap(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_textblock_get_scroll_enabled(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_NULL(egui_view_textblock_get_font(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_NULL(egui_view_textblock_get_text(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT_PRIMARY.full, (int)egui_view_textblock_get_font_color(EGUI_VIEW_OF(&s_textblock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALPHA_100, (int)egui_view_textblock_get_font_alpha(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)(EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP), (int)egui_view_textblock_get_align_type(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_content_width(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_content_height(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_content_line_count(EGUI_VIEW_OF(&s_textblock)));
}

static void test_textblock_get_state_after_setters(void)
{
    const char *text = "Hello";
    const egui_font_t *font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_color_t color = {.full = 0x1234};

    setup();
    egui_view_set_size(EGUI_VIEW_OF(&s_textblock), 100, 40);
    egui_view_textblock_set_line_space(EGUI_VIEW_OF(&s_textblock), 5);
    egui_view_textblock_set_max_lines(EGUI_VIEW_OF(&s_textblock), 2);
    egui_view_textblock_set_auto_height(EGUI_VIEW_OF(&s_textblock), 1);
    egui_view_textblock_set_auto_wrap(EGUI_VIEW_OF(&s_textblock), 0);
    egui_view_textblock_set_scroll_enabled(EGUI_VIEW_OF(&s_textblock), 0);
    egui_view_textblock_set_font(EGUI_VIEW_OF(&s_textblock), font);
    egui_view_textblock_set_font_color(EGUI_VIEW_OF(&s_textblock), color, 77);
    egui_view_textblock_set_align_type(EGUI_VIEW_OF(&s_textblock), EGUI_ALIGN_CENTER);
    egui_view_textblock_set_text(EGUI_VIEW_OF(&s_textblock), text);

    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_textblock_get_line_space(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_textblock_get_max_lines(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_textblock_get_auto_height(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_auto_wrap(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_scroll_enabled(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_TRUE(egui_view_textblock_get_font(EGUI_VIEW_OF(&s_textblock)) == font);
    EGUI_TEST_ASSERT_TRUE(egui_view_textblock_get_text(EGUI_VIEW_OF(&s_textblock)) == text);
    EGUI_TEST_ASSERT_EQUAL_INT((int)color.full, (int)egui_view_textblock_get_font_color(EGUI_VIEW_OF(&s_textblock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(77, (int)egui_view_textblock_get_font_alpha(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALIGN_CENTER, (int)egui_view_textblock_get_align_type(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_TRUE(egui_view_textblock_get_content_width(EGUI_VIEW_OF(&s_textblock)) > 0);
    EGUI_TEST_ASSERT_TRUE(egui_view_textblock_get_content_height(EGUI_VIEW_OF(&s_textblock)) > 0);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_textblock_get_content_line_count(EGUI_VIEW_OF(&s_textblock)));
}

static void test_textblock_get_state_border_and_scrollbar(void)
{
    egui_color_t border = {.full = 0x4567};

    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_border_enabled(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_RADIUS_SM, (int)egui_view_textblock_get_border_radius(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_BORDER.full, (int)egui_view_textblock_get_border_color(EGUI_VIEW_OF(&s_textblock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_scrollbar_enabled(EGUI_VIEW_OF(&s_textblock)));

    egui_view_textblock_set_border_enabled(EGUI_VIEW_OF(&s_textblock), 1);
    egui_view_textblock_set_border_radius(EGUI_VIEW_OF(&s_textblock), 6);
    egui_view_textblock_set_border_color(EGUI_VIEW_OF(&s_textblock), border);
    egui_view_textblock_set_scrollbar_enabled(EGUI_VIEW_OF(&s_textblock), 1);

    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_textblock_get_border_enabled(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_textblock_get_border_radius(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)border.full, (int)egui_view_textblock_get_border_color(EGUI_VIEW_OF(&s_textblock)).full);
#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_textblock_get_scrollbar_enabled(EGUI_VIEW_OF(&s_textblock)));
#else
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_scrollbar_enabled(EGUI_VIEW_OF(&s_textblock)));
#endif
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static void test_textblock_get_state_edit_state(void)
{
    egui_color_t cursor_color = {.full = 0x2468};

    setup();
    egui_view_textblock_set_text(EGUI_VIEW_OF(&s_textblock), "abc");
    egui_view_textblock_set_editable(EGUI_VIEW_OF(&s_textblock), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_textblock_get_editable(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_TRUE(strcmp(egui_view_textblock_get_edit_text(EGUI_VIEW_OF(&s_textblock)), "abc") == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_textblock_get_cursor_pos(EGUI_VIEW_OF(&s_textblock)));

    egui_view_textblock_set_cursor_pos(EGUI_VIEW_OF(&s_textblock), 1);
    egui_view_textblock_set_cursor_color(EGUI_VIEW_OF(&s_textblock), cursor_color);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_textblock_get_cursor_pos(EGUI_VIEW_OF(&s_textblock)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)cursor_color.full, (int)egui_view_textblock_get_cursor_color(EGUI_VIEW_OF(&s_textblock)).full);
}
#endif

static void test_textblock_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_line_space(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_max_lines(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_auto_height(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_auto_wrap(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_scroll_enabled(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_textblock_get_font(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_textblock_get_text(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_font_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_font_alpha(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_align_type(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_content_width(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_content_height(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_content_line_count(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_border_enabled(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_border_radius(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_border_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_scrollbar_enabled(NULL));
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_editable(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_cursor_pos(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textblock_get_cursor_color(NULL).full);
    EGUI_TEST_ASSERT_NULL(egui_view_textblock_get_edit_text(NULL));
#endif
}

void test_textblock_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(textblock_get_state);

    EGUI_TEST_RUN(test_textblock_get_state_defaults);
    EGUI_TEST_RUN(test_textblock_get_state_after_setters);
    EGUI_TEST_RUN(test_textblock_get_state_border_and_scrollbar);
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    EGUI_TEST_RUN(test_textblock_get_state_edit_state);
#endif
    EGUI_TEST_RUN(test_textblock_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
