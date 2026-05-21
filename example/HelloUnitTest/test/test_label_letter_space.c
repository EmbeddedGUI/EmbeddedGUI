#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_label_letter_space.h"

#if EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE

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

/* After init, letter_space defaults to 0. */
static void test_letter_space_init_zero(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)s_label.letter_space);
}

/* set_letter_space stores the value. */
static void test_letter_space_set(void)
{
    setup();
    egui_view_label_set_letter_space(EGUI_VIEW_OF(&s_label), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)s_label.letter_space);
}

/* Setting to 0 is accepted. */
static void test_letter_space_set_zero(void)
{
    setup();
    egui_view_label_set_letter_space(EGUI_VIEW_OF(&s_label), 5);
    egui_view_label_set_letter_space(EGUI_VIEW_OF(&s_label), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)s_label.letter_space);
}

/* NULL self does not crash. */
static void test_letter_space_null_self(void)
{
    egui_view_label_set_letter_space(NULL, 2);
    EGUI_TEST_ASSERT_TRUE(1);
}

/* Set a large value: stored correctly. */
static void test_letter_space_large_value(void)
{
    setup();
    egui_view_label_set_letter_space(EGUI_VIEW_OF(&s_label), 100);
    EGUI_TEST_ASSERT_EQUAL_INT(100, (int)s_label.letter_space);
}

#endif /* EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE */

void test_label_letter_space_run(void)
{
    EGUI_TEST_SUITE_BEGIN(label_letter_space);

#if EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE
    EGUI_TEST_RUN(test_letter_space_init_zero);
    EGUI_TEST_RUN(test_letter_space_set);
    EGUI_TEST_RUN(test_letter_space_set_zero);
    EGUI_TEST_RUN(test_letter_space_null_self);
    EGUI_TEST_RUN(test_letter_space_large_value);
#endif

    EGUI_TEST_SUITE_END();
}
