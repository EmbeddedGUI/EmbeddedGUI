#include "egui.h"
#include "test/egui_test.h"
#include "test_slist.h"

typedef struct test_slist_item
{
    int value;
    egui_snode_t node;
} test_slist_item_t;

static void test_slist_init_empty(void)
{
    egui_slist_t list;
    egui_slist_init(&list);
    EGUI_TEST_ASSERT_TRUE(egui_slist_is_empty(&list));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_slist_size(&list));
    EGUI_TEST_ASSERT_NULL(egui_slist_peek_head(&list));
    EGUI_TEST_ASSERT_NULL(egui_slist_peek_tail(&list));
}

static void test_slist_append_single(void)
{
    egui_slist_t list;
    test_slist_item_t item1 = {.value = 1};

    egui_slist_init(&list);
    egui_slist_append(&list, &item1.node);

    EGUI_TEST_ASSERT_FALSE(egui_slist_is_empty(&list));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_slist_size(&list));
    EGUI_TEST_ASSERT_TRUE(egui_slist_peek_head(&list) == &item1.node);
    EGUI_TEST_ASSERT_TRUE(egui_slist_peek_tail(&list) == &item1.node);
}

static void test_slist_append_multiple(void)
{
    egui_slist_t list;
    test_slist_item_t item1 = {.value = 1};
    test_slist_item_t item2 = {.value = 2};
    test_slist_item_t item3 = {.value = 3};

    egui_slist_init(&list);
    egui_slist_append(&list, &item1.node);
    egui_slist_append(&list, &item2.node);
    egui_slist_append(&list, &item3.node);

    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_slist_size(&list));
    EGUI_TEST_ASSERT_TRUE(egui_slist_peek_head(&list) == &item1.node);
    EGUI_TEST_ASSERT_TRUE(egui_slist_peek_tail(&list) == &item3.node);
}

static void test_slist_prepend(void)
{
    egui_slist_t list;
    test_slist_item_t item1 = {.value = 1};
    test_slist_item_t item2 = {.value = 2};

    egui_slist_init(&list);
    egui_slist_append(&list, &item1.node);
    egui_slist_prepend(&list, &item2.node);

    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_slist_size(&list));
    EGUI_TEST_ASSERT_TRUE(egui_slist_peek_head(&list) == &item2.node);
    EGUI_TEST_ASSERT_TRUE(egui_slist_peek_tail(&list) == &item1.node);
}

static void test_slist_get(void)
{
    egui_slist_t list;
    test_slist_item_t item1 = {.value = 1};
    test_slist_item_t item2 = {.value = 2};

    egui_slist_init(&list);
    egui_slist_append(&list, &item1.node);
    egui_slist_append(&list, &item2.node);

    egui_snode_t *got = egui_slist_get(&list);
    EGUI_TEST_ASSERT_TRUE(got == &item1.node);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_slist_size(&list));

    got = egui_slist_get(&list);
    EGUI_TEST_ASSERT_TRUE(got == &item2.node);
    EGUI_TEST_ASSERT_TRUE(egui_slist_is_empty(&list));

    got = egui_slist_get(&list);
    EGUI_TEST_ASSERT_NULL(got);
}

static void test_slist_find_and_remove(void)
{
    egui_slist_t list;
    test_slist_item_t item1 = {.value = 1};
    test_slist_item_t item2 = {.value = 2};
    test_slist_item_t item3 = {.value = 3};

    egui_slist_init(&list);
    egui_slist_append(&list, &item1.node);
    egui_slist_append(&list, &item2.node);
    egui_slist_append(&list, &item3.node);

    // Remove middle
    EGUI_TEST_ASSERT_TRUE(egui_slist_find_and_remove(&list, &item2.node));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_slist_size(&list));

    // Remove head
    EGUI_TEST_ASSERT_TRUE(egui_slist_find_and_remove(&list, &item1.node));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_slist_size(&list));
    EGUI_TEST_ASSERT_TRUE(egui_slist_peek_head(&list) == &item3.node);

    // Remove last
    EGUI_TEST_ASSERT_TRUE(egui_slist_find_and_remove(&list, &item3.node));
    EGUI_TEST_ASSERT_TRUE(egui_slist_is_empty(&list));
}

static void test_slist_iteration(void)
{
    egui_slist_t list;
    test_slist_item_t items[3] = {{.value = 10}, {.value = 20}, {.value = 30}};

    egui_slist_init(&list);
    for (int i = 0; i < 3; i++)
    {
        egui_slist_append(&list, &items[i].node);
    }

    int count = 0;
    int expected_values[] = {10, 20, 30};
    egui_snode_t *sn;
    EGUI_SLIST_FOR_EACH_NODE(&list, sn)
    {
        test_slist_item_t *item = EGUI_SLIST_ENTRY(sn, test_slist_item_t, node);
        EGUI_TEST_ASSERT_EQUAL_INT(expected_values[count], item->value);
        count++;
    }
    EGUI_TEST_ASSERT_EQUAL_INT(3, count);
}

void test_slist_run(void)
{
    EGUI_TEST_SUITE_BEGIN(slist);

    EGUI_TEST_RUN(test_slist_init_empty);
    EGUI_TEST_RUN(test_slist_append_single);
    EGUI_TEST_RUN(test_slist_append_multiple);
    EGUI_TEST_RUN(test_slist_prepend);
    EGUI_TEST_RUN(test_slist_get);
    EGUI_TEST_RUN(test_slist_find_and_remove);
    EGUI_TEST_RUN(test_slist_iteration);

    EGUI_TEST_SUITE_END();
}
