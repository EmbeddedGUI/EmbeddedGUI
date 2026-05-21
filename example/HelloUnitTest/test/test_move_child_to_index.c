#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_move_child_to_index.h"

static egui_view_t       s_c0;
static egui_view_t       s_c1;
static egui_view_t       s_c2;
static egui_view_group_t s_parent;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_parent, 0, sizeof(s_parent));
    memset(&s_c0,     0, sizeof(s_c0));
    memset(&s_c1,     0, sizeof(s_c1));
    memset(&s_c2,     0, sizeof(s_c2));

    egui_view_group_init(EGUI_VIEW_OF(&s_parent), core);
    egui_view_init(&s_c0, core);
    egui_view_init(&s_c1, core);
    egui_view_init(&s_c2, core);

    egui_view_group_add_child(EGUI_VIEW_OF(&s_parent), &s_c0);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_parent), &s_c1);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_parent), &s_c2);
    /* Order: c0(0), c1(1), c2(2) */
}

/* Move last child to index 0: order becomes c2, c0, c1. */
static void test_move_last_to_front(void)
{
    setup();
    egui_view_group_move_child_to_index(EGUI_VIEW_OF(&s_parent), &s_c2, 0);
    EGUI_TEST_ASSERT_EQUAL_INT((int)(intptr_t)&s_c2,
        (int)(intptr_t)egui_view_group_get_child_at(EGUI_VIEW_OF(&s_parent), 0));
    EGUI_TEST_ASSERT_EQUAL_INT((int)(intptr_t)&s_c0,
        (int)(intptr_t)egui_view_group_get_child_at(EGUI_VIEW_OF(&s_parent), 1));
    EGUI_TEST_ASSERT_EQUAL_INT((int)(intptr_t)&s_c1,
        (int)(intptr_t)egui_view_group_get_child_at(EGUI_VIEW_OF(&s_parent), 2));
}

/* Move first child to index 2 (end): order becomes c1, c2, c0. */
static void test_move_first_to_back(void)
{
    setup();
    egui_view_group_move_child_to_index(EGUI_VIEW_OF(&s_parent), &s_c0, 2);
    EGUI_TEST_ASSERT_EQUAL_INT((int)(intptr_t)&s_c1,
        (int)(intptr_t)egui_view_group_get_child_at(EGUI_VIEW_OF(&s_parent), 0));
    EGUI_TEST_ASSERT_EQUAL_INT((int)(intptr_t)&s_c2,
        (int)(intptr_t)egui_view_group_get_child_at(EGUI_VIEW_OF(&s_parent), 1));
    EGUI_TEST_ASSERT_EQUAL_INT((int)(intptr_t)&s_c0,
        (int)(intptr_t)egui_view_group_get_child_at(EGUI_VIEW_OF(&s_parent), 2));
}

/* Move middle child to index 0. */
static void test_move_mid_to_front(void)
{
    setup();
    egui_view_group_move_child_to_index(EGUI_VIEW_OF(&s_parent), &s_c1, 0);
    EGUI_TEST_ASSERT_EQUAL_INT((int)(intptr_t)&s_c1,
        (int)(intptr_t)egui_view_group_get_child_at(EGUI_VIEW_OF(&s_parent), 0));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_group_get_child_count(EGUI_VIEW_OF(&s_parent)));
}

/* index >= count: child is appended to end. */
static void test_move_index_overflow(void)
{
    setup();
    egui_view_group_move_child_to_index(EGUI_VIEW_OF(&s_parent), &s_c0, 99);
    EGUI_TEST_ASSERT_EQUAL_INT((int)(intptr_t)&s_c0,
        (int)(intptr_t)egui_view_group_get_child_at(EGUI_VIEW_OF(&s_parent), 2));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_group_get_child_count(EGUI_VIEW_OF(&s_parent)));
}

/* NULL self or child: does not crash. */
static void test_move_null_args(void)
{
    setup();
    egui_view_group_move_child_to_index(NULL, &s_c0, 0);
    egui_view_group_move_child_to_index(EGUI_VIEW_OF(&s_parent), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_group_get_child_count(EGUI_VIEW_OF(&s_parent)));
}

/* Move child to its current position: count unchanged. */
static void test_move_same_position(void)
{
    setup();
    egui_view_group_move_child_to_index(EGUI_VIEW_OF(&s_parent), &s_c1, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_group_get_child_count(EGUI_VIEW_OF(&s_parent)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)(intptr_t)&s_c1,
        (int)(intptr_t)egui_view_group_get_child_at(EGUI_VIEW_OF(&s_parent), 1));
}

void test_move_child_to_index_run(void)
{
    EGUI_TEST_SUITE_BEGIN(move_child_to_index);

    EGUI_TEST_RUN(test_move_last_to_front);
    EGUI_TEST_RUN(test_move_first_to_back);
    EGUI_TEST_RUN(test_move_mid_to_front);
    EGUI_TEST_RUN(test_move_index_overflow);
    EGUI_TEST_RUN(test_move_null_args);
    EGUI_TEST_RUN(test_move_same_position);

    EGUI_TEST_SUITE_END();
}
