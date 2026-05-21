#include <string.h>

#include "egui.h"
#include "widget/egui_view_label.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_label_get_line_space.h"

static egui_view_label_t s_label;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_label, 0, sizeof(s_label));
    egui_view_label_init(EGUI_VIEW_OF(&s_label), core);
}

/* get_line_space returns the value set by set_line_space. */
static void test_label_get_line_space_after_set(void)
{
    setup();
    egui_view_label_set_line_space(EGUI_VIEW_OF(&s_label), 4);
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_label_get_line_space(EGUI_VIEW_OF(&s_label)));
}

/* get_line_space returns 0 after init (no extra spacing). */
static void test_label_get_line_space_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_label_get_line_space(EGUI_VIEW_OF(&s_label)));
}

/* Updated line space is reflected. */
static void test_label_get_line_space_update(void)
{
    setup();
    egui_view_label_set_line_space(EGUI_VIEW_OF(&s_label), 2);
    egui_view_label_set_line_space(EGUI_VIEW_OF(&s_label), 8);
    EGUI_TEST_ASSERT_EQUAL_INT(8, (int)egui_view_label_get_line_space(EGUI_VIEW_OF(&s_label)));
}

/* NULL self returns 0 without crash. */
static void test_label_get_line_space_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_label_get_line_space(NULL));
}

void test_label_get_line_space_run(void)
{
    EGUI_TEST_SUITE_BEGIN(label_get_line_space);

    EGUI_TEST_RUN(test_label_get_line_space_after_set);
    EGUI_TEST_RUN(test_label_get_line_space_default);
    EGUI_TEST_RUN(test_label_get_line_space_update);
    EGUI_TEST_RUN(test_label_get_line_space_null_self);

    EGUI_TEST_SUITE_END();
}
