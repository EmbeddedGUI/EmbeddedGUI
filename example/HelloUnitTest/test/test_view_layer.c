#include "egui.h"
#include "test/egui_test.h"
#include "test_view_layer.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_LAYER

static egui_view_group_t test_group;
static egui_view_t test_child1;
static egui_view_t test_child2;
static egui_view_t test_child3;
static egui_view_t test_child4;

static void test_layer_default_value(void)
{
    egui_view_init(&test_child1);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_LAYER_DEFAULT, egui_view_get_layer(&test_child1));
}

static void test_layer_set_get(void)
{
    egui_view_init(&test_child1);
    egui_view_set_layer(&test_child1, EGUI_VIEW_LAYER_OVERLAY);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_LAYER_OVERLAY, egui_view_get_layer(&test_child1));
}

static void test_layer_same_layer_order(void)
{
    egui_view_group_init(EGUI_VIEW_OF(&test_group));
    egui_view_init(&test_child1);
    egui_view_init(&test_child2);
    egui_view_init(&test_child3);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child2);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child3);

    // All same layer, first child should be child1 (insertion order preserved)
    EGUI_TEST_ASSERT_TRUE(egui_view_group_get_first_child(EGUI_VIEW_OF(&test_group)) == &test_child1);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_group_get_child_count(EGUI_VIEW_OF(&test_group)));
}

static void test_layer_ordering(void)
{
    egui_view_group_init(EGUI_VIEW_OF(&test_group));
    egui_view_init(&test_child1);
    egui_view_init(&test_child2);
    egui_view_init(&test_child3);

    egui_view_set_layer(&test_child1, EGUI_VIEW_LAYER_OVERLAY); // 192
    egui_view_set_layer(&test_child2, EGUI_VIEW_LAYER_DEFAULT); // 0
    egui_view_set_layer(&test_child3, EGUI_VIEW_LAYER_CONTENT); // 64

    // Add in arbitrary order
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child1); // layer 192
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child2); // layer 0
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child3); // layer 64

    // Expected list order: child2 (0), child3 (64), child1 (192)
    egui_view_t *first = egui_view_group_get_first_child(EGUI_VIEW_OF(&test_group));
    EGUI_TEST_ASSERT_TRUE(first == &test_child2);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_group_get_child_count(EGUI_VIEW_OF(&test_group)));
}

static void test_layer_runtime_change(void)
{
    egui_view_group_init(EGUI_VIEW_OF(&test_group));
    egui_view_init(&test_child1);
    egui_view_init(&test_child2);
    egui_view_init(&test_child3);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child2);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child3);

    // All at layer 0, order is child1, child2, child3
    EGUI_TEST_ASSERT_TRUE(egui_view_group_get_first_child(EGUI_VIEW_OF(&test_group)) == &test_child1);

    // Move child1 to a higher layer
    egui_view_set_layer(&test_child1, EGUI_VIEW_LAYER_TOP);

    // child2 should now be first (layer 0), child1 moved to end (layer 255)
    EGUI_TEST_ASSERT_TRUE(egui_view_group_get_first_child(EGUI_VIEW_OF(&test_group)) == &test_child2);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_group_get_child_count(EGUI_VIEW_OF(&test_group)));
}

static void test_layer_stable_within_layer(void)
{
    egui_view_group_init(EGUI_VIEW_OF(&test_group));
    egui_view_init(&test_child1);
    egui_view_init(&test_child2);
    egui_view_init(&test_child3);
    egui_view_init(&test_child4);

    egui_view_set_layer(&test_child1, 10);
    egui_view_set_layer(&test_child2, 10);
    egui_view_set_layer(&test_child3, 20);
    egui_view_set_layer(&test_child4, 10);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child1); // layer 10
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child2); // layer 10
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child3); // layer 20
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child4); // layer 10

    // Expected order: child1(10), child2(10), child4(10), child3(20)
    EGUI_TEST_ASSERT_TRUE(egui_view_group_get_first_child(EGUI_VIEW_OF(&test_group)) == &test_child1);
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_group_get_child_count(EGUI_VIEW_OF(&test_group)));
}

static void test_layer_bring_to_front(void)
{
    egui_view_group_init(EGUI_VIEW_OF(&test_group));
    egui_view_init(&test_child1);
    egui_view_init(&test_child2);
    egui_view_init(&test_child3);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child2);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child3);

    // Bring child1 to front
    egui_view_group_bring_child_to_front(EGUI_VIEW_OF(&test_group), &test_child1);

    // child1 should now have EGUI_VIEW_LAYER_TOP and be at the end
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_LAYER_TOP, egui_view_get_layer(&test_child1));
    EGUI_TEST_ASSERT_TRUE(egui_view_group_get_first_child(EGUI_VIEW_OF(&test_group)) == &test_child2);
}

static void test_layer_send_to_back(void)
{
    egui_view_group_init(EGUI_VIEW_OF(&test_group));
    egui_view_init(&test_child1);
    egui_view_init(&test_child2);
    egui_view_init(&test_child3);

    egui_view_set_layer(&test_child1, EGUI_VIEW_LAYER_CONTENT);
    egui_view_set_layer(&test_child2, EGUI_VIEW_LAYER_CONTENT);
    egui_view_set_layer(&test_child3, EGUI_VIEW_LAYER_CONTENT);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child1);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child2);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_group), &test_child3);

    // Send child3 to back
    egui_view_group_send_child_to_back(EGUI_VIEW_OF(&test_group), &test_child3);

    // child3 should now have EGUI_VIEW_LAYER_BACKGROUND and be first
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_LAYER_BACKGROUND, egui_view_get_layer(&test_child3));
    EGUI_TEST_ASSERT_TRUE(egui_view_group_get_first_child(EGUI_VIEW_OF(&test_group)) == &test_child3);
}

void test_view_layer_run(void)
{
    EGUI_TEST_SUITE_BEGIN(view_layer);

    EGUI_TEST_RUN(test_layer_default_value);
    EGUI_TEST_RUN(test_layer_set_get);
    EGUI_TEST_RUN(test_layer_same_layer_order);
    EGUI_TEST_RUN(test_layer_ordering);
    EGUI_TEST_RUN(test_layer_runtime_change);
    EGUI_TEST_RUN(test_layer_stable_within_layer);
    EGUI_TEST_RUN(test_layer_bring_to_front);
    EGUI_TEST_RUN(test_layer_send_to_back);

    EGUI_TEST_SUITE_END();
}

#else // !EGUI_CONFIG_FUNCTION_SUPPORT_LAYER

void test_view_layer_run(void)
{
    // Layer support disabled, skip tests
}

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_LAYER
