#include <string.h>

#include "egui.h"
#include "widget/egui_view_textinput.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_textinput_get_cursor_pos.h"

static egui_view_textinput_t s_ti;

static void setup(void)
{
    memset(&s_ti, 0, sizeof(s_ti));
    egui_view_textinput_init(EGUI_VIEW_OF(&s_ti), uicode_get_core());
}

/* Default cursor position after init is 0. */
static void test_textinput_get_cursor_pos_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_textinput_get_cursor_pos(EGUI_VIEW_OF(&s_ti)));
}

/* After setting text and moving the caret, getter returns the new position. */
static void test_textinput_get_cursor_pos_after_set(void)
{
    setup();
    egui_view_textinput_set_text(EGUI_VIEW_OF(&s_ti), "abcd");
    egui_view_textinput_set_cursor_pos(EGUI_VIEW_OF(&s_ti), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_textinput_get_cursor_pos(EGUI_VIEW_OF(&s_ti)));
}

/* set_cursor_pos clamps to text_len on an empty buffer. */
static void test_textinput_get_cursor_pos_clamped_empty(void)
{
    setup();
    egui_view_textinput_set_cursor_pos(EGUI_VIEW_OF(&s_ti), 5);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_textinput_get_cursor_pos(EGUI_VIEW_OF(&s_ti)));
}

/* NULL self returns 0 without crash. */
static void test_textinput_get_cursor_pos_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_textinput_get_cursor_pos(NULL));
}

void test_textinput_get_cursor_pos_run(void)
{
    EGUI_TEST_SUITE_BEGIN(textinput_get_cursor_pos);

    EGUI_TEST_RUN(test_textinput_get_cursor_pos_default);
    EGUI_TEST_RUN(test_textinput_get_cursor_pos_after_set);
    EGUI_TEST_RUN(test_textinput_get_cursor_pos_clamped_empty);
    EGUI_TEST_RUN(test_textinput_get_cursor_pos_null_self);

    EGUI_TEST_SUITE_END();
}
