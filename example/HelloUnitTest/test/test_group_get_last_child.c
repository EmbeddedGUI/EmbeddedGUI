#include <string.h>

#include "egui.h"
#include "widget/egui_view_group.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_group_get_last_child.h"

static egui_view_group_t s_group;
static egui_view_t s_child1;
static egui_view_t s_child2;
static egui_view_t s_child3;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_group, 0, sizeof(s_group));
    memset(&s_child1, 0, sizeof(s_child1));
    memset(&s_child2, 0, sizeof(s_child2));
    memset(&s_child3, 0, sizeof(s_child3));
    egui_view_group_init(EGUI_VIEW_OF(&s_group), core);
    egui_view_init(&s_child1, core);
    egui_view_init(&s_child2, core);
    egui_view_init(&s_child3, core);
}

/* Empty group returns NULL. */
static void test_group_get_last_child_empty(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_group_get_last_child(EGUI_VIEW_OF(&s_group)) == NULL);
}

/* Single child: last == first. */
static void test_group_get_last_child_single(void)
{
    setup();
    egui_view_group_add_child(EGUI_VIEW_OF(&s_group), &s_child1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_group_get_last_child(EGUI_VIEW_OF(&s_group)) == &s_child1);
}

/* Multiple children: last is the most recently added. */
static void test_group_get_last_child_multiple(void)
{
    setup();
    egui_view_group_add_child(EGUI_VIEW_OF(&s_group), &s_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_group), &s_child2);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_group), &s_child3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_group_get_last_child(EGUI_VIEW_OF(&s_group)) == &s_child3);
}

/* NULL self returns NULL without crash. */
static void test_group_get_last_child_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_group_get_last_child(NULL) == NULL);
}

void test_group_get_last_child_run(void)
{
    EGUI_TEST_SUITE_BEGIN(group_get_last_child);

    EGUI_TEST_RUN(test_group_get_last_child_empty);
    EGUI_TEST_RUN(test_group_get_last_child_single);
    EGUI_TEST_RUN(test_group_get_last_child_multiple);
    EGUI_TEST_RUN(test_group_get_last_child_null_self);

    EGUI_TEST_SUITE_END();
}
