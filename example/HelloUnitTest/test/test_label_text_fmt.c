#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_label_text_fmt.h"

#if EGUI_CONFIG_FUNCTION_LABEL_TEXT_FMT

static egui_view_label_t s_label;
static egui_view_group_t s_parent;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_parent, 0, sizeof(s_parent));
    memset(&s_label,  0, sizeof(s_label));
    egui_view_group_init(EGUI_VIEW_OF(&s_parent), core);
    egui_view_label_init(EGUI_VIEW_OF(&s_label), core);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_parent), EGUI_VIEW_OF(&s_label));
}

/* set_text_fmt with plain string: get_text returns matching content. */
static void test_fmt_plain_string(void)
{
    setup();
    egui_view_label_set_text_fmt(EGUI_VIEW_OF(&s_label), "hello");
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(egui_view_label_get_text(EGUI_VIEW_OF(&s_label)), "hello"));
}

/* set_text_fmt with integer substitution. */
static void test_fmt_integer(void)
{
    setup();
    egui_view_label_set_text_fmt(EGUI_VIEW_OF(&s_label), "%d%%", 75);
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(egui_view_label_get_text(EGUI_VIEW_OF(&s_label)), "75%"));
}

/* set_text_fmt with string substitution. */
static void test_fmt_string_subst(void)
{
    setup();
    egui_view_label_set_text_fmt(EGUI_VIEW_OF(&s_label), "val=%s", "ok");
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(egui_view_label_get_text(EGUI_VIEW_OF(&s_label)), "val=ok"));
}

/* Calling twice overwrites the first result. */
static void test_fmt_overwrite(void)
{
    setup();
    egui_view_label_set_text_fmt(EGUI_VIEW_OF(&s_label), "first");
    egui_view_label_set_text_fmt(EGUI_VIEW_OF(&s_label), "second");
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(egui_view_label_get_text(EGUI_VIEW_OF(&s_label)), "second"));
}

/* NULL self: does not crash. */
static void test_fmt_null_self(void)
{
    egui_view_label_set_text_fmt(NULL, "x");
    EGUI_TEST_ASSERT_TRUE(1);
}

#endif /* EGUI_CONFIG_FUNCTION_LABEL_TEXT_FMT */

void test_label_text_fmt_run(void)
{
    EGUI_TEST_SUITE_BEGIN(label_text_fmt);

#if EGUI_CONFIG_FUNCTION_LABEL_TEXT_FMT
    EGUI_TEST_RUN(test_fmt_plain_string);
    EGUI_TEST_RUN(test_fmt_integer);
    EGUI_TEST_RUN(test_fmt_string_subst);
    EGUI_TEST_RUN(test_fmt_overwrite);
    EGUI_TEST_RUN(test_fmt_null_self);
#endif

    EGUI_TEST_SUITE_END();
}
