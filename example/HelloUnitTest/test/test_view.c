#include "egui.h"
#include "test/egui_test.h"
#include "test_view.h"

static egui_view_t test_view;

static void test_view_init_defaults(void)
{
    egui_view_init(&test_view);
    EGUI_TEST_ASSERT_TRUE(test_view.is_enable == 1);
    EGUI_TEST_ASSERT_TRUE(test_view.is_visible == 1);
    EGUI_TEST_ASSERT_TRUE(test_view.is_gone == 0);
    EGUI_TEST_ASSERT_TRUE(test_view.is_pressed == 0);
    EGUI_TEST_ASSERT_TRUE(test_view.is_clickable == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_100, test_view.alpha);
    EGUI_TEST_ASSERT_NULL(test_view.parent);
    EGUI_TEST_ASSERT_NULL(test_view.background);
}

static void test_view_set_position(void)
{
    egui_view_init(&test_view);
    egui_view_set_position(&test_view, 10, 20);
    EGUI_TEST_ASSERT_EQUAL_INT(10, test_view.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(20, test_view.region.location.y);
}

static void test_view_set_size(void)
{
    egui_view_init(&test_view);
    egui_view_set_size(&test_view, 100, 200);
    EGUI_TEST_ASSERT_EQUAL_INT(100, test_view.region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(200, test_view.region.size.height);
}

static void test_view_visibility(void)
{
    egui_view_init(&test_view);

    // Default visible
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_get_visible(&test_view));

    // Set invisible
    egui_view_set_visible(&test_view, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_visible(&test_view));

    // Set visible again
    egui_view_set_visible(&test_view, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_get_visible(&test_view));
}

static void test_view_gone(void)
{
    egui_view_init(&test_view);

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_gone(&test_view));

    egui_view_set_gone(&test_view, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_get_gone(&test_view));

    egui_view_set_gone(&test_view, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_gone(&test_view));
}

static void test_view_enable(void)
{
    egui_view_init(&test_view);

    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_get_enable(&test_view));

    egui_view_set_enable(&test_view, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_enable(&test_view));
}

static void test_view_clickable(void)
{
    egui_view_init(&test_view);

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_clickable(&test_view));

    egui_view_set_clickable(&test_view, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_get_clickable(&test_view));
}

static void test_view_alpha(void)
{
    egui_view_init(&test_view);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_100, test_view.alpha);

    egui_view_set_alpha(&test_view, EGUI_ALPHA_50);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_50, test_view.alpha);
}

static void test_view_padding(void)
{
    egui_view_init(&test_view);

    egui_view_set_padding(&test_view, 1, 2, 3, 4);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_view.padding.left);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_view.padding.right);
    EGUI_TEST_ASSERT_EQUAL_INT(3, test_view.padding.top);
    EGUI_TEST_ASSERT_EQUAL_INT(4, test_view.padding.bottom);

    egui_view_set_padding_all(&test_view, 5);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.padding.left);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.padding.right);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.padding.top);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.padding.bottom);
}

static void test_view_margin(void)
{
    egui_view_init(&test_view);

    egui_view_set_margin(&test_view, 1, 2, 3, 4);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_view.margin.left);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_view.margin.right);
    EGUI_TEST_ASSERT_EQUAL_INT(3, test_view.margin.top);
    EGUI_TEST_ASSERT_EQUAL_INT(4, test_view.margin.bottom);

    egui_view_set_margin_all(&test_view, 5);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.margin.left);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.margin.right);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.margin.top);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.margin.bottom);
}

void test_view_run(void)
{
    EGUI_TEST_SUITE_BEGIN(view);

    EGUI_TEST_RUN(test_view_init_defaults);
    EGUI_TEST_RUN(test_view_set_position);
    EGUI_TEST_RUN(test_view_set_size);
    EGUI_TEST_RUN(test_view_visibility);
    EGUI_TEST_RUN(test_view_gone);
    EGUI_TEST_RUN(test_view_enable);
    EGUI_TEST_RUN(test_view_clickable);
    EGUI_TEST_RUN(test_view_alpha);
    EGUI_TEST_RUN(test_view_padding);
    EGUI_TEST_RUN(test_view_margin);

    EGUI_TEST_SUITE_END();
}
