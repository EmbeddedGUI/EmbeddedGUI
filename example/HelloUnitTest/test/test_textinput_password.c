#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_textinput_password.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

/* ---- Test fixture ---- */

static egui_view_textinput_t s_ti;

static egui_core_t *get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

static void setup(void)
{
    memset(&s_ti, 0, sizeof(s_ti));
    egui_view_textinput_init(EGUI_VIEW_OF(&s_ti), get_core());
}

/* ---- Tests ---- */

/* Password mode is off by default. */
static void test_pw_default_off(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_textinput_get_password_mode(EGUI_VIEW_OF(&s_ti)));
}

/* NULL-safe getter returns 0. */
static void test_pw_null_safe_getter(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_textinput_get_password_mode(NULL));
}

/* Setting and getting password mode round-trips correctly. */
static void test_pw_set_get_on(void)
{
    setup();
    egui_view_textinput_set_password_mode(EGUI_VIEW_OF(&s_ti), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_textinput_get_password_mode(EGUI_VIEW_OF(&s_ti)));
}

/* Disabling password mode after it was enabled works. */
static void test_pw_set_get_off(void)
{
    setup();
    egui_view_textinput_set_password_mode(EGUI_VIEW_OF(&s_ti), 1);
    egui_view_textinput_set_password_mode(EGUI_VIEW_OF(&s_ti), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_textinput_get_password_mode(EGUI_VIEW_OF(&s_ti)));
}

/* get_text() always returns the real text buffer regardless of password mode. */
static void test_pw_get_text_returns_real_text(void)
{
    setup();
    egui_view_textinput_set_text(EGUI_VIEW_OF(&s_ti), "secret");
    egui_view_textinput_set_password_mode(EGUI_VIEW_OF(&s_ti), 1);

    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_textinput_get_password_mode(EGUI_VIEW_OF(&s_ti)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp("secret", egui_view_textinput_get_text(EGUI_VIEW_OF(&s_ti))));
}

/* text_len is correct when password mode is enabled. */
static void test_pw_text_len_correct(void)
{
    setup();
    egui_view_textinput_set_password_mode(EGUI_VIEW_OF(&s_ti), 1);
    egui_view_textinput_set_text(EGUI_VIEW_OF(&s_ti), "abc");

    EGUI_TEST_ASSERT_EQUAL_INT(3, s_ti.text_len);
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp("abc", egui_view_textinput_get_text(EGUI_VIEW_OF(&s_ti))));
}

/* Toggling non-zero values all enable password mode. */
static void test_pw_nonzero_value_enables(void)
{
    setup();
    egui_view_textinput_set_password_mode(EGUI_VIEW_OF(&s_ti), 42);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_textinput_get_password_mode(EGUI_VIEW_OF(&s_ti)));
}

#endif /* EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS */

/* ---- Suite runner ---- */

void test_textinput_password_run(void)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    EGUI_TEST_RUN(test_pw_default_off);
    EGUI_TEST_RUN(test_pw_null_safe_getter);
    EGUI_TEST_RUN(test_pw_set_get_on);
    EGUI_TEST_RUN(test_pw_set_get_off);
    EGUI_TEST_RUN(test_pw_get_text_returns_real_text);
    EGUI_TEST_RUN(test_pw_text_len_correct);
    EGUI_TEST_RUN(test_pw_nonzero_value_enables);
#endif /* EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS */
}
