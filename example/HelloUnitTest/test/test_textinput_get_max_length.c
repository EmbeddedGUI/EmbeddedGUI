#include <string.h>

#include "egui.h"
#include "widget/egui_view_textinput.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_textinput_get_max_length.h"

static egui_view_textinput_t s_ti;

static void setup(void)
{
    memset(&s_ti, 0, sizeof(s_ti));
    egui_view_textinput_init(EGUI_VIEW_OF(&s_ti), uicode_get_core());
}

/* Default max_length after init equals the build-time cap. */
static void test_textinput_get_max_length_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_CONFIG_TEXTINPUT_MAX_LENGTH, (int)egui_view_textinput_get_max_length(EGUI_VIEW_OF(&s_ti)));
}

/* Setting a smaller cap is reflected by the getter. */
static void test_textinput_get_max_length_after_set(void)
{
    setup();
    egui_view_textinput_set_max_length(EGUI_VIEW_OF(&s_ti), 5);
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_textinput_get_max_length(EGUI_VIEW_OF(&s_ti)));
}

/* Values above the build-time cap are clamped down. */
static void test_textinput_get_max_length_clamped(void)
{
    setup();
    egui_view_textinput_set_max_length(EGUI_VIEW_OF(&s_ti), 255);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_CONFIG_TEXTINPUT_MAX_LENGTH, (int)egui_view_textinput_get_max_length(EGUI_VIEW_OF(&s_ti)));
}

/* NULL self returns 0 without crash. */
static void test_textinput_get_max_length_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_textinput_get_max_length(NULL));
}

void test_textinput_get_max_length_run(void)
{
    EGUI_TEST_SUITE_BEGIN(textinput_get_max_length);

    EGUI_TEST_RUN(test_textinput_get_max_length_default);
    EGUI_TEST_RUN(test_textinput_get_max_length_after_set);
    EGUI_TEST_RUN(test_textinput_get_max_length_clamped);
    EGUI_TEST_RUN(test_textinput_get_max_length_null_self);

    EGUI_TEST_SUITE_END();
}
