#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_label_get_text.h"

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

/* After init, get_text returns NULL. */
static void test_label_get_text_init_null(void)
{
    setup();
    EGUI_TEST_ASSERT_NULL(egui_view_label_get_text(EGUI_VIEW_OF(&s_label)));
}

/* After set_text, get_text returns the same pointer. */
static void test_label_get_text_after_set(void)
{
    const char *text = "hello";

    setup();
    egui_view_label_set_text(EGUI_VIEW_OF(&s_label), text);
    EGUI_TEST_ASSERT_EQUAL_INT((int)(intptr_t)text, (int)(intptr_t)egui_view_label_get_text(EGUI_VIEW_OF(&s_label)));
}

/* Overwriting text: get_text reflects the latest pointer. */
static void test_label_get_text_overwrite(void)
{
    const char *t1 = "first";
    const char *t2 = "second";

    setup();
    egui_view_label_set_text(EGUI_VIEW_OF(&s_label), t1);
    egui_view_label_set_text(EGUI_VIEW_OF(&s_label), t2);
    EGUI_TEST_ASSERT_EQUAL_INT((int)(intptr_t)t2, (int)(intptr_t)egui_view_label_get_text(EGUI_VIEW_OF(&s_label)));
}

/* NULL self: get_text returns NULL. */
static void test_label_get_text_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_label_get_text(NULL));
}

/* set_text(NULL) is reflected by get_text. */
static void test_label_get_text_set_null_text(void)
{
    setup();
    egui_view_label_set_text(EGUI_VIEW_OF(&s_label), "x");
    egui_view_label_set_text(EGUI_VIEW_OF(&s_label), NULL);
    EGUI_TEST_ASSERT_NULL(egui_view_label_get_text(EGUI_VIEW_OF(&s_label)));
}

void test_label_get_text_run(void)
{
    EGUI_TEST_SUITE_BEGIN(label_get_text);

    EGUI_TEST_RUN(test_label_get_text_init_null);
    EGUI_TEST_RUN(test_label_get_text_after_set);
    EGUI_TEST_RUN(test_label_get_text_overwrite);
    EGUI_TEST_RUN(test_label_get_text_null_self);
    EGUI_TEST_RUN(test_label_get_text_set_null_text);

    EGUI_TEST_SUITE_END();
}
