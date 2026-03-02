#include "egui.h"
#include "test/egui_test.h"
#include "test_view_group.h"

static egui_view_group_t test_group;
static egui_view_t test_child1;
static egui_view_t test_child2;
static egui_view_t test_child3;

static void test_vg_init_defaults(void)
{
    egui_view_group_init(EGUI_VIEW_OF(&test_group));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_group_get_child_count(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_NULL(egui_view_group_get_first_child(EGUI_VIEW_OF(&test_group)));
}

static void test_vg_add_child(void)
{
    egui_view_group_init(EGUI_VIEW_OF(&test_group));
    egui_view_init(&test_child1);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_group_get_child_count(EGUI_VIEW_OF(&test_group)));
    EGUI_TEST_ASSERT_TRUE(egui_view_group_get_first_child(EGUI_VIEW_OF(&test_group)) == &test_child1);
}

static void test_vg_add_multiple(void)
{
    egui_view_group_init(EGUI_VIEW_OF(&test_group));
    egui_view_init(&test_child1);
    egui_view_init(&test_child2);
    egui_view_init(&test_child3);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child2);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child3);

    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_group_get_child_count(EGUI_VIEW_OF(&test_group)));
}

static void test_vg_parent_link(void)
{
    egui_view_group_init(EGUI_VIEW_OF(&test_group));
    egui_view_init(&test_child1);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child1);
    EGUI_TEST_ASSERT_TRUE(test_child1.parent == &test_group);
}

static void test_vg_remove_child(void)
{
    egui_view_group_init(EGUI_VIEW_OF(&test_group));
    egui_view_init(&test_child1);
    egui_view_init(&test_child2);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_group_get_child_count(EGUI_VIEW_OF(&test_group)));

    egui_view_group_remove_child(EGUI_VIEW_OF(&test_group), &test_child1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_group_get_child_count(EGUI_VIEW_OF(&test_group)));
}

static void test_vg_clear_childs(void)
{
    egui_view_group_init(EGUI_VIEW_OF(&test_group));
    egui_view_init(&test_child1);
    egui_view_init(&test_child2);
    egui_view_init(&test_child3);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child2);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child3);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_group_get_child_count(EGUI_VIEW_OF(&test_group)));

    egui_view_group_clear_childs(EGUI_VIEW_OF(&test_group));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_group_get_child_count(EGUI_VIEW_OF(&test_group)));
}

void test_view_group_run(void)
{
    EGUI_TEST_SUITE_BEGIN(view_group);

    EGUI_TEST_RUN(test_vg_init_defaults);
    EGUI_TEST_RUN(test_vg_add_child);
    EGUI_TEST_RUN(test_vg_add_multiple);
    EGUI_TEST_RUN(test_vg_parent_link);
    EGUI_TEST_RUN(test_vg_remove_child);
    EGUI_TEST_RUN(test_vg_clear_childs);

    EGUI_TEST_SUITE_END();
}
