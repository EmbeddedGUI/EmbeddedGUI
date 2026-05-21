#include <string.h>

#include "egui.h"
#include "widget/egui_view_combobox.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_combobox_get_item_count.h"

static egui_view_combobox_t s_cb;

static const char *s_items3[] = {"A", "B", "C"};
static const char *s_items2[] = {"X", "Y"};

static void setup(void)
{
    memset(&s_cb, 0, sizeof(s_cb));
    egui_view_combobox_init(EGUI_VIEW_OF(&s_cb), uicode_get_core());
}

/* Default item count after plain init is 0. */
static void test_combobox_get_item_count_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_combobox_get_item_count(EGUI_VIEW_OF(&s_cb)));
}

/* After set_items the count reflects the supplied number. */
static void test_combobox_get_item_count_after_set(void)
{
    setup();
    egui_view_combobox_set_items(EGUI_VIEW_OF(&s_cb), s_items3, 3);
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_combobox_get_item_count(EGUI_VIEW_OF(&s_cb)));
}

/* Replacing the list with a smaller one updates the count. */
static void test_combobox_get_item_count_replace(void)
{
    setup();
    egui_view_combobox_set_items(EGUI_VIEW_OF(&s_cb), s_items3, 3);
    egui_view_combobox_set_items(EGUI_VIEW_OF(&s_cb), s_items2, 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_combobox_get_item_count(EGUI_VIEW_OF(&s_cb)));
}

/* NULL self returns 0 without crash. */
static void test_combobox_get_item_count_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_combobox_get_item_count(NULL));
}

void test_combobox_get_item_count_run(void)
{
    EGUI_TEST_SUITE_BEGIN(combobox_get_item_count);

    EGUI_TEST_RUN(test_combobox_get_item_count_default);
    EGUI_TEST_RUN(test_combobox_get_item_count_after_set);
    EGUI_TEST_RUN(test_combobox_get_item_count_replace);
    EGUI_TEST_RUN(test_combobox_get_item_count_null_self);

    EGUI_TEST_SUITE_END();
}
