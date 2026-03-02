#include "egui.h"
#include "test/egui_test.h"
#include "test_dlist.h"

typedef struct test_dlist_item
{
    int value;
    egui_dnode_t node;
} test_dlist_item_t;

static void test_dlist_init_empty(void)
{
    egui_dlist_t list;
    egui_dlist_init(&list);
    EGUI_TEST_ASSERT_TRUE(egui_dlist_is_empty(&list));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_dlist_size(&list));
    EGUI_TEST_ASSERT_NULL(egui_dlist_peek_head(&list));
    EGUI_TEST_ASSERT_NULL(egui_dlist_peek_tail(&list));
}

static void test_dlist_append_single(void)
{
    egui_dlist_t list;
    test_dlist_item_t item1 = {.value = 1};

    egui_dlist_init(&list);
    egui_dnode_init(&item1.node);
    egui_dlist_append(&list, &item1.node);

    EGUI_TEST_ASSERT_FALSE(egui_dlist_is_empty(&list));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_dlist_size(&list));
    EGUI_TEST_ASSERT_TRUE(egui_dlist_peek_head(&list) == &item1.node);
    EGUI_TEST_ASSERT_TRUE(egui_dlist_peek_tail(&list) == &item1.node);
}

static void test_dlist_append_multiple(void)
{
    egui_dlist_t list;
    test_dlist_item_t item1 = {.value = 1};
    test_dlist_item_t item2 = {.value = 2};
    test_dlist_item_t item3 = {.value = 3};

    egui_dlist_init(&list);
    egui_dnode_init(&item1.node);
    egui_dnode_init(&item2.node);
    egui_dnode_init(&item3.node);

    egui_dlist_append(&list, &item1.node);
    egui_dlist_append(&list, &item2.node);
    egui_dlist_append(&list, &item3.node);

    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_dlist_size(&list));
    EGUI_TEST_ASSERT_TRUE(egui_dlist_peek_head(&list) == &item1.node);
    EGUI_TEST_ASSERT_TRUE(egui_dlist_peek_tail(&list) == &item3.node);
}

static void test_dlist_prepend(void)
{
    egui_dlist_t list;
    test_dlist_item_t item1 = {.value = 1};
    test_dlist_item_t item2 = {.value = 2};

    egui_dlist_init(&list);
    egui_dnode_init(&item1.node);
    egui_dnode_init(&item2.node);

    egui_dlist_append(&list, &item1.node);
    egui_dlist_prepend(&list, &item2.node);

    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_dlist_size(&list));
    EGUI_TEST_ASSERT_TRUE(egui_dlist_peek_head(&list) == &item2.node);
    EGUI_TEST_ASSERT_TRUE(egui_dlist_peek_tail(&list) == &item1.node);
}

static void test_dlist_remove(void)
{
    egui_dlist_t list;
    test_dlist_item_t item1 = {.value = 1};
    test_dlist_item_t item2 = {.value = 2};
    test_dlist_item_t item3 = {.value = 3};

    egui_dlist_init(&list);
    egui_dnode_init(&item1.node);
    egui_dnode_init(&item2.node);
    egui_dnode_init(&item3.node);

    egui_dlist_append(&list, &item1.node);
    egui_dlist_append(&list, &item2.node);
    egui_dlist_append(&list, &item3.node);

    // Remove middle
    egui_dlist_remove(&item2.node);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_dlist_size(&list));
    EGUI_TEST_ASSERT_TRUE(egui_dlist_peek_head(&list) == &item1.node);
    EGUI_TEST_ASSERT_TRUE(egui_dlist_peek_tail(&list) == &item3.node);

    // Remove head
    egui_dlist_remove(&item1.node);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_dlist_size(&list));
    EGUI_TEST_ASSERT_TRUE(egui_dlist_peek_head(&list) == &item3.node);

    // Remove last
    egui_dlist_remove(&item3.node);
    EGUI_TEST_ASSERT_TRUE(egui_dlist_is_empty(&list));
}

static void test_dlist_get(void)
{
    egui_dlist_t list;
    test_dlist_item_t item1 = {.value = 1};
    test_dlist_item_t item2 = {.value = 2};

    egui_dlist_init(&list);
    egui_dnode_init(&item1.node);
    egui_dnode_init(&item2.node);

    egui_dlist_append(&list, &item1.node);
    egui_dlist_append(&list, &item2.node);

    egui_dnode_t *got = egui_dlist_get(&list);
    EGUI_TEST_ASSERT_TRUE(got == &item1.node);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_dlist_size(&list));

    got = egui_dlist_get(&list);
    EGUI_TEST_ASSERT_TRUE(got == &item2.node);
    EGUI_TEST_ASSERT_TRUE(egui_dlist_is_empty(&list));

    got = egui_dlist_get(&list);
    EGUI_TEST_ASSERT_NULL(got);
}

static void test_dlist_iteration(void)
{
    egui_dlist_t list;
    test_dlist_item_t items[3] = {{.value = 10}, {.value = 20}, {.value = 30}};

    egui_dlist_init(&list);
    for (int i = 0; i < 3; i++)
    {
        egui_dnode_init(&items[i].node);
        egui_dlist_append(&list, &items[i].node);
    }

    int count = 0;
    int expected_values[] = {10, 20, 30};
    egui_dnode_t *dn;
    EGUI_DLIST_FOR_EACH_NODE(&list, dn)
    {
        test_dlist_item_t *item = EGUI_DLIST_ENTRY(dn, test_dlist_item_t, node);
        EGUI_TEST_ASSERT_EQUAL_INT(expected_values[count], item->value);
        count++;
    }
    EGUI_TEST_ASSERT_EQUAL_INT(3, count);
}

static void test_dlist_has_multiple_nodes(void)
{
    egui_dlist_t list;
    test_dlist_item_t item1 = {.value = 1};
    test_dlist_item_t item2 = {.value = 2};

    egui_dlist_init(&list);
    egui_dnode_init(&item1.node);
    egui_dnode_init(&item2.node);

    EGUI_TEST_ASSERT_FALSE(egui_dlist_has_multiple_nodes(&list));

    egui_dlist_append(&list, &item1.node);
    EGUI_TEST_ASSERT_FALSE(egui_dlist_has_multiple_nodes(&list));

    egui_dlist_append(&list, &item2.node);
    EGUI_TEST_ASSERT_TRUE(egui_dlist_has_multiple_nodes(&list));
}

void test_dlist_run(void)
{
    EGUI_TEST_SUITE_BEGIN(dlist);

    EGUI_TEST_RUN(test_dlist_init_empty);
    EGUI_TEST_RUN(test_dlist_append_single);
    EGUI_TEST_RUN(test_dlist_append_multiple);
    EGUI_TEST_RUN(test_dlist_prepend);
    EGUI_TEST_RUN(test_dlist_remove);
    EGUI_TEST_RUN(test_dlist_get);
    EGUI_TEST_RUN(test_dlist_iteration);
    EGUI_TEST_RUN(test_dlist_has_multiple_nodes);

    EGUI_TEST_SUITE_END();
}
