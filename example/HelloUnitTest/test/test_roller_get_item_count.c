#include <string.h>

#include "egui.h"
#include "widget/egui_view_roller.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_roller_get_item_count.h"

static egui_view_roller_t s_roller;

static const char *s_items4[] = {"A", "B", "C", "D"};
static const char *s_items2[] = {"X", "Y"};

static void setup(void)
{
    memset(&s_roller, 0, sizeof(s_roller));
    egui_view_roller_init(EGUI_VIEW_OF(&s_roller), uicode_get_core());
}

/* Default item count after plain init is 0. */
static void test_roller_get_item_count_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_roller_get_item_count(EGUI_VIEW_OF(&s_roller)));
}

/* After set_items the count reflects the supplied number. */
static void test_roller_get_item_count_after_set(void)
{
    setup();
    egui_view_roller_set_items(EGUI_VIEW_OF(&s_roller), s_items4, 4);
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_roller_get_item_count(EGUI_VIEW_OF(&s_roller)));
}

/* Replacing the list with a smaller one updates the count. */
static void test_roller_get_item_count_replace(void)
{
    setup();
    egui_view_roller_set_items(EGUI_VIEW_OF(&s_roller), s_items4, 4);
    egui_view_roller_set_items(EGUI_VIEW_OF(&s_roller), s_items2, 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_roller_get_item_count(EGUI_VIEW_OF(&s_roller)));
}

/* NULL self returns 0 without crash. */
static void test_roller_get_item_count_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_roller_get_item_count(NULL));
}

void test_roller_get_item_count_run(void)
{
    EGUI_TEST_SUITE_BEGIN(roller_get_item_count);

    EGUI_TEST_RUN(test_roller_get_item_count_default);
    EGUI_TEST_RUN(test_roller_get_item_count_after_set);
    EGUI_TEST_RUN(test_roller_get_item_count_replace);
    EGUI_TEST_RUN(test_roller_get_item_count_null_self);

    EGUI_TEST_SUITE_END();
}
