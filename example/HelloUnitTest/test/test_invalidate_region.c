#include "egui.h"
#include "test/egui_test.h"
#include "test_invalidate_region.h"

static egui_view_t test_view;

static void setup_view(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    egui_view_init(&test_view);
    egui_region_init(&region, x, y, width, height);
    egui_region_copy(&test_view.region, &region);
    egui_region_copy(&test_view.region_screen, &region);
    test_view.is_request_layout = 0;
}

static int32_t region_area(const egui_region_t *region)
{
    return (int32_t)region->size.width * region->size.height;
}

static void test_invalidate_region_basic(void)
{
    egui_region_t dirty_region;
    egui_region_t expected;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    setup_view(10, 20, 100, 50);
    egui_core_clear_region_dirty();

    egui_region_init(&dirty_region, 5, 6, 20, 10);
    egui_view_invalidate_region(&test_view, &dirty_region);

    egui_region_init(&expected, 15, 26, 20, 10);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected, &arr[0]);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

static void test_invalidate_region_clamp(void)
{
    egui_region_t dirty_region;
    egui_region_t expected;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    setup_view(10, 20, 100, 50);
    egui_core_clear_region_dirty();

    egui_region_init(&dirty_region, -5, 40, 30, 20);
    egui_view_invalidate_region(&test_view, &dirty_region);

    egui_region_init(&expected, 10, 60, 25, 10);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected, &arr[0]);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

static void test_invalidate_region_invisible(void)
{
    egui_region_t dirty_region;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    setup_view(10, 20, 100, 50);
    test_view.is_visible = 0;
    egui_core_clear_region_dirty();

    egui_region_init(&dirty_region, 5, 5, 20, 10);
    egui_view_invalidate_region(&test_view, &dirty_region);

    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[0]));
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

static void test_invalidate_region_multiple(void)
{
    egui_region_t dirty_a;
    egui_region_t dirty_b;
    egui_region_t expected;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    setup_view(10, 20, 100, 50);
    egui_core_clear_region_dirty();

    egui_region_init(&dirty_a, 0, 0, 20, 10);
    egui_region_init(&dirty_b, 10, 5, 20, 10);
    egui_view_invalidate_region(&test_view, &dirty_a);
    egui_view_invalidate_region(&test_view, &dirty_b);

    egui_region_init(&expected, 10, 20, 30, 15);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected, &arr[0]);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

static void test_invalidate_region_vs_full(void)
{
    egui_region_t dirty_region;
    egui_region_t *arr = egui_core_get_region_dirty_arr();
    int32_t sub_area;
    int32_t full_area;

    setup_view(10, 20, 100, 50);

    egui_core_clear_region_dirty();
    egui_region_init(&dirty_region, 5, 5, 20, 10);
    egui_view_invalidate_region(&test_view, &dirty_region);
    sub_area = region_area(&arr[0]);

    egui_core_clear_region_dirty();
    egui_core_update_region_dirty(&test_view.region_screen);
    full_area = region_area(&arr[0]);

    EGUI_TEST_ASSERT_TRUE(sub_area < full_area);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_SUB_DIRTY_REGION
static void test_sub_region_table_invalidate(void)
{
    egui_sub_region_t regions[2];
    egui_sub_region_table_t table = {
            .regions = regions,
            .count = 2,
    };
    egui_region_t expected;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    setup_view(10, 20, 100, 50);
    egui_core_clear_region_dirty();

    egui_region_init(&regions[0].region, 0, 0, 10, 10);
    egui_region_init(&regions[1].region, 30, 15, 20, 10);
    egui_view_invalidate_sub_region(&test_view, &table, 1);

    egui_region_init(&expected, 40, 35, 20, 10);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected, &arr[0]);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

static void test_sub_region_table_bounds(void)
{
    egui_sub_region_t regions[1];
    egui_sub_region_table_t table = {
            .regions = regions,
            .count = 1,
    };
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    setup_view(10, 20, 100, 50);
    egui_core_clear_region_dirty();

    egui_region_init(&regions[0].region, 0, 0, 10, 10);
    egui_view_invalidate_sub_region(&test_view, &table, 2);

    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[0]));
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}
#endif

void test_invalidate_region_run(void)
{
    EGUI_TEST_SUITE_BEGIN(invalidate_region);
    EGUI_TEST_RUN(test_invalidate_region_basic);
    EGUI_TEST_RUN(test_invalidate_region_clamp);
    EGUI_TEST_RUN(test_invalidate_region_invisible);
    EGUI_TEST_RUN(test_invalidate_region_multiple);
    EGUI_TEST_RUN(test_invalidate_region_vs_full);
#if EGUI_CONFIG_FUNCTION_SUPPORT_SUB_DIRTY_REGION
    EGUI_TEST_RUN(test_sub_region_table_invalidate);
    EGUI_TEST_RUN(test_sub_region_table_bounds);
#endif
    EGUI_TEST_SUITE_END();
}
