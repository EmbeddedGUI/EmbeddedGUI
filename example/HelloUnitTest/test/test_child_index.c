#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_child_index.h"

static egui_view_group_t s_group;
static egui_view_t       s_child0;
static egui_view_t       s_child1;
static egui_view_t       s_child2;
static egui_view_t       s_orphan;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_group,  0, sizeof(s_group));
    memset(&s_child0, 0, sizeof(s_child0));
    memset(&s_child1, 0, sizeof(s_child1));
    memset(&s_child2, 0, sizeof(s_child2));
    memset(&s_orphan, 0, sizeof(s_orphan));

    egui_view_group_init(EGUI_VIEW_OF(&s_group), core);
    egui_view_init(&s_child0, core);
    egui_view_init(&s_child1, core);
    egui_view_init(&s_child2, core);
    egui_view_init(&s_orphan, core);

    egui_view_group_add_child(EGUI_VIEW_OF(&s_group), &s_child0);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_group), &s_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_group), &s_child2);
}

/* get_child_at returns the correct child for a valid index. */
static void test_get_child_at_valid(void)
{
    setup();
    EGUI_TEST_ASSERT_TRUE(egui_view_group_get_child_at(EGUI_VIEW_OF(&s_group), 0) == &s_child0);
    EGUI_TEST_ASSERT_TRUE(egui_view_group_get_child_at(EGUI_VIEW_OF(&s_group), 1) == &s_child1);
    EGUI_TEST_ASSERT_TRUE(egui_view_group_get_child_at(EGUI_VIEW_OF(&s_group), 2) == &s_child2);
}

/* get_child_at returns NULL for out-of-range index. */
static void test_get_child_at_out_of_range(void)
{
    setup();
    EGUI_TEST_ASSERT_NULL(egui_view_group_get_child_at(EGUI_VIEW_OF(&s_group), 3));
    EGUI_TEST_ASSERT_NULL(egui_view_group_get_child_at(EGUI_VIEW_OF(&s_group), -1));
}

/* get_child_at: NULL self returns NULL. */
static void test_get_child_at_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_group_get_child_at(NULL, 0));
}

/* get_child_index returns correct index. */
static void test_get_child_index_valid(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_group_get_child_index(EGUI_VIEW_OF(&s_group), &s_child0));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_group_get_child_index(EGUI_VIEW_OF(&s_group), &s_child1));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_group_get_child_index(EGUI_VIEW_OF(&s_group), &s_child2));
}

/* get_child_index returns -1 for a child not in the group. */
static void test_get_child_index_not_found(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_group_get_child_index(EGUI_VIEW_OF(&s_group), &s_orphan));
}

/* get_child_index: NULL self or child returns -1. */
static void test_get_child_index_null_safe(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_group_get_child_index(NULL, &s_child0));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_view_group_get_child_index(EGUI_VIEW_OF(&s_group), NULL));
}

void test_child_index_run(void)
{
    EGUI_TEST_SUITE_BEGIN(child_index);

    EGUI_TEST_RUN(test_get_child_at_valid);
    EGUI_TEST_RUN(test_get_child_at_out_of_range);
    EGUI_TEST_RUN(test_get_child_at_null_self);
    EGUI_TEST_RUN(test_get_child_index_valid);
    EGUI_TEST_RUN(test_get_child_index_not_found);
    EGUI_TEST_RUN(test_get_child_index_null_safe);

    EGUI_TEST_SUITE_END();
}
