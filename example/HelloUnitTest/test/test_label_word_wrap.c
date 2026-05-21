#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_label_word_wrap.h"

#if EGUI_CONFIG_FUNCTION_LABEL_WORD_WRAP && EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE

static egui_view_label_t s_label;
static egui_view_group_t s_parent;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_parent, 0, sizeof(s_parent));
    memset(&s_label, 0, sizeof(s_label));
    egui_view_group_init(EGUI_VIEW_OF(&s_parent), core);
    egui_view_label_init(EGUI_VIEW_OF(&s_label), core);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_parent), EGUI_VIEW_OF(&s_label));
}

/* EGUI_LABEL_LONG_WRAP constant has the expected value (2). */
static void test_long_wrap_constant_value(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(2, EGUI_LABEL_LONG_WRAP);
}

/* set_long_mode to WRAP stores the value. */
static void test_long_wrap_set_mode(void)
{
    setup();
    egui_view_label_set_long_mode(EGUI_VIEW_OF(&s_label), EGUI_LABEL_LONG_WRAP);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_LABEL_LONG_WRAP, (int)s_label.long_mode);
}

/* Switching away from WRAP is accepted. */
static void test_long_wrap_switch_to_clip(void)
{
    setup();
    egui_view_label_set_long_mode(EGUI_VIEW_OF(&s_label), EGUI_LABEL_LONG_WRAP);
    egui_view_label_set_long_mode(EGUI_VIEW_OF(&s_label), EGUI_LABEL_LONG_CLIP);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_LABEL_LONG_CLIP, (int)s_label.long_mode);
}

/* NULL self does not crash. */
static void test_long_wrap_null_self(void)
{
    egui_view_label_set_long_mode(NULL, EGUI_LABEL_LONG_WRAP);
    EGUI_TEST_ASSERT_TRUE(1);
}

/* WRAP and DOTS are distinct mode values. */
static void test_long_wrap_distinct_from_dots(void)
{
    EGUI_TEST_ASSERT_TRUE(EGUI_LABEL_LONG_WRAP != EGUI_LABEL_LONG_DOTS);
}

#endif /* EGUI_CONFIG_FUNCTION_LABEL_WORD_WRAP && EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE */

void test_label_word_wrap_run(void)
{
    EGUI_TEST_SUITE_BEGIN(label_word_wrap);

#if EGUI_CONFIG_FUNCTION_LABEL_WORD_WRAP && EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE
    EGUI_TEST_RUN(test_long_wrap_constant_value);
    EGUI_TEST_RUN(test_long_wrap_set_mode);
    EGUI_TEST_RUN(test_long_wrap_switch_to_clip);
    EGUI_TEST_RUN(test_long_wrap_null_self);
    EGUI_TEST_RUN(test_long_wrap_distinct_from_dots);
#endif

    EGUI_TEST_SUITE_END();
}
