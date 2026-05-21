#include <string.h>

#include "egui.h"
#include "widget/egui_view_label.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_label_get_align_type.h"

static egui_view_label_t s_label;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_label, 0, sizeof(s_label));
    egui_view_label_init(EGUI_VIEW_OF(&s_label), core);
}

/* get_align_type returns EGUI_ALIGN_CENTER after init (default). */
static void test_label_get_align_type_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALIGN_CENTER, (int)egui_view_label_get_align_type(EGUI_VIEW_OF(&s_label)));
}

/* get_align_type returns the value set by set_align_type. */
static void test_label_get_align_type_after_set(void)
{
    setup();
    egui_view_label_set_align_type(EGUI_VIEW_OF(&s_label), EGUI_ALIGN_LEFT);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALIGN_LEFT, (int)egui_view_label_get_align_type(EGUI_VIEW_OF(&s_label)));
}

/* set then change: latest value is returned. */
static void test_label_get_align_type_update(void)
{
    setup();
    egui_view_label_set_align_type(EGUI_VIEW_OF(&s_label), EGUI_ALIGN_LEFT);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&s_label), EGUI_ALIGN_RIGHT);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALIGN_RIGHT, (int)egui_view_label_get_align_type(EGUI_VIEW_OF(&s_label)));
}

/* NULL self returns 0 without crash. */
static void test_label_get_align_type_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_label_get_align_type(NULL));
}

void test_label_get_align_type_run(void)
{
    EGUI_TEST_SUITE_BEGIN(label_get_align_type);

    EGUI_TEST_RUN(test_label_get_align_type_default);
    EGUI_TEST_RUN(test_label_get_align_type_after_set);
    EGUI_TEST_RUN(test_label_get_align_type_update);
    EGUI_TEST_RUN(test_label_get_align_type_null_self);

    EGUI_TEST_SUITE_END();
}
