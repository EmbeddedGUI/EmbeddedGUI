#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_label_get_letter_space.h"

#if EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE

static egui_view_label_t s_label;

static void setup(void)
{
    memset(&s_label, 0, sizeof(s_label));
    egui_view_label_init(EGUI_VIEW_OF(&s_label), uicode_get_core());
}

/* After set_letter_space the getter returns the same value. */
static void test_label_get_letter_space_after_set(void)
{
    setup();
    egui_view_label_set_letter_space(EGUI_VIEW_OF(&s_label), 4);
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_label_get_letter_space(EGUI_VIEW_OF(&s_label)));
}

/* Updating the value reflects in the getter. */
static void test_label_get_letter_space_update(void)
{
    setup();
    egui_view_label_set_letter_space(EGUI_VIEW_OF(&s_label), 4);
    egui_view_label_set_letter_space(EGUI_VIEW_OF(&s_label), 8);
    EGUI_TEST_ASSERT_EQUAL_INT(8, (int)egui_view_label_get_letter_space(EGUI_VIEW_OF(&s_label)));
}

/* Default letter space after plain init is 0. */
static void test_label_get_letter_space_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_label_get_letter_space(EGUI_VIEW_OF(&s_label)));
}

/* NULL self returns 0 without crash. */
static void test_label_get_letter_space_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_label_get_letter_space(NULL));
}

#endif /* EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE */

void test_label_get_letter_space_run(void)
{
    EGUI_TEST_SUITE_BEGIN(label_get_letter_space);

#if EGUI_CONFIG_FUNCTION_LABEL_LETTER_SPACE
    EGUI_TEST_RUN(test_label_get_letter_space_after_set);
    EGUI_TEST_RUN(test_label_get_letter_space_update);
    EGUI_TEST_RUN(test_label_get_letter_space_default);
    EGUI_TEST_RUN(test_label_get_letter_space_null_self);
#endif

    EGUI_TEST_SUITE_END();
}
