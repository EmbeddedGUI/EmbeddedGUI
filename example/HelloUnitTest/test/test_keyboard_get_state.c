#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_keyboard_get_state.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static egui_view_keyboard_t s_keyboard;
static egui_view_textinput_t s_textinput;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_keyboard, 0, sizeof(s_keyboard));
    memset(&s_textinput, 0, sizeof(s_textinput));
    egui_view_keyboard_init(EGUI_VIEW_OF(&s_keyboard), core);
    egui_view_textinput_init(EGUI_VIEW_OF(&s_textinput), core);
    egui_view_set_position(EGUI_VIEW_OF(&s_keyboard), 0, 200);
    egui_view_set_position(EGUI_VIEW_OF(&s_textinput), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&s_textinput), 100, 24);
}

static void test_keyboard_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_KEYBOARD_MODE_LOWERCASE, (int)egui_view_keyboard_get_mode(EGUI_VIEW_OF(&s_keyboard)));
    EGUI_TEST_ASSERT_TRUE(egui_view_keyboard_get_font(EGUI_VIEW_OF(&s_keyboard)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_NULL(egui_view_keyboard_get_icon_font(EGUI_VIEW_OF(&s_keyboard)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(EGUI_ICON_MS_KEYBOARD_ARROW_UP, egui_view_keyboard_get_shift_icon(EGUI_VIEW_OF(&s_keyboard))));
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(EGUI_ICON_MS_BACKSPACE, egui_view_keyboard_get_backspace_icon(EGUI_VIEW_OF(&s_keyboard))));
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(EGUI_ICON_MS_DONE, egui_view_keyboard_get_enter_icon(EGUI_VIEW_OF(&s_keyboard))));
    EGUI_TEST_ASSERT_NULL(egui_view_keyboard_get_target(EGUI_VIEW_OF(&s_keyboard)));
}

static void test_keyboard_get_state_after_setters(void)
{
    const egui_font_t *font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    const char *shift_icon = "shift";
    const char *backspace_icon = "back";
    const char *enter_icon = "enter";

    setup();
    egui_view_keyboard_set_mode(EGUI_VIEW_OF(&s_keyboard), EGUI_KEYBOARD_MODE_SYMBOLS);
    egui_view_keyboard_set_font(EGUI_VIEW_OF(&s_keyboard), font);
    egui_view_keyboard_set_icon_font(EGUI_VIEW_OF(&s_keyboard), font);
    egui_view_keyboard_set_special_key_icons(EGUI_VIEW_OF(&s_keyboard), shift_icon, backspace_icon, enter_icon);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_KEYBOARD_MODE_SYMBOLS, (int)egui_view_keyboard_get_mode(EGUI_VIEW_OF(&s_keyboard)));
    EGUI_TEST_ASSERT_TRUE(egui_view_keyboard_get_font(EGUI_VIEW_OF(&s_keyboard)) == font);
    EGUI_TEST_ASSERT_TRUE(egui_view_keyboard_get_icon_font(EGUI_VIEW_OF(&s_keyboard)) == font);
    EGUI_TEST_ASSERT_TRUE(egui_view_keyboard_get_shift_icon(EGUI_VIEW_OF(&s_keyboard)) == shift_icon);
    EGUI_TEST_ASSERT_TRUE(egui_view_keyboard_get_backspace_icon(EGUI_VIEW_OF(&s_keyboard)) == backspace_icon);
    EGUI_TEST_ASSERT_TRUE(egui_view_keyboard_get_enter_icon(EGUI_VIEW_OF(&s_keyboard)) == enter_icon);
}

static void test_keyboard_get_state_normalizes_inputs(void)
{
    setup();
    egui_view_keyboard_set_mode(EGUI_VIEW_OF(&s_keyboard), 0xFF);
    egui_view_keyboard_set_icon_font(EGUI_VIEW_OF(&s_keyboard), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_keyboard_set_icon_font(EGUI_VIEW_OF(&s_keyboard), NULL);
    egui_view_keyboard_set_special_key_icons(EGUI_VIEW_OF(&s_keyboard), "shift", "back", "enter");
    egui_view_keyboard_set_special_key_icons(EGUI_VIEW_OF(&s_keyboard), NULL, NULL, NULL);

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_KEYBOARD_MODE_LOWERCASE, (int)egui_view_keyboard_get_mode(EGUI_VIEW_OF(&s_keyboard)));
    EGUI_TEST_ASSERT_NULL(egui_view_keyboard_get_icon_font(EGUI_VIEW_OF(&s_keyboard)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(EGUI_ICON_MS_KEYBOARD_ARROW_UP, egui_view_keyboard_get_shift_icon(EGUI_VIEW_OF(&s_keyboard))));
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(EGUI_ICON_MS_BACKSPACE, egui_view_keyboard_get_backspace_icon(EGUI_VIEW_OF(&s_keyboard))));
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(EGUI_ICON_MS_DONE, egui_view_keyboard_get_enter_icon(EGUI_VIEW_OF(&s_keyboard))));
}

static void test_keyboard_get_state_target_lifecycle(void)
{
    setup();

    egui_view_keyboard_show(EGUI_VIEW_OF(&s_keyboard), EGUI_VIEW_OF(&s_textinput));
    EGUI_TEST_ASSERT_TRUE(egui_view_keyboard_get_target(EGUI_VIEW_OF(&s_keyboard)) == EGUI_VIEW_OF(&s_textinput));

    egui_view_keyboard_hide(EGUI_VIEW_OF(&s_keyboard));
    EGUI_TEST_ASSERT_NULL(egui_view_keyboard_get_target(EGUI_VIEW_OF(&s_keyboard)));
}

static void test_keyboard_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_KEYBOARD_MODE_LOWERCASE, (int)egui_view_keyboard_get_mode(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_keyboard_get_font(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_keyboard_get_icon_font(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_keyboard_get_shift_icon(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_keyboard_get_backspace_icon(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_keyboard_get_enter_icon(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_keyboard_get_target(NULL));
}
#endif

void test_keyboard_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(keyboard_get_state);
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    EGUI_TEST_RUN(test_keyboard_get_state_defaults);
    EGUI_TEST_RUN(test_keyboard_get_state_after_setters);
    EGUI_TEST_RUN(test_keyboard_get_state_normalizes_inputs);
    EGUI_TEST_RUN(test_keyboard_get_state_target_lifecycle);
    EGUI_TEST_RUN(test_keyboard_get_state_null_self);
#endif
    EGUI_TEST_SUITE_END();
}
