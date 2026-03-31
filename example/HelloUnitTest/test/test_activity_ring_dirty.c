#include "egui.h"
#include "test/egui_test.h"
#include "test_activity_ring_dirty.h"

static egui_view_activity_ring_t test_ring;

static int32_t region_area(const egui_region_t *region)
{
    return (int32_t)region->size.width * region->size.height;
}

static void setup_ring(void)
{
    egui_region_t region;

    egui_view_activity_ring_init(EGUI_VIEW_OF(&test_ring));
    egui_view_set_size(EGUI_VIEW_OF(&test_ring), 120, 120);
    egui_view_activity_ring_set_stroke_width(EGUI_VIEW_OF(&test_ring), 12);
    egui_view_activity_ring_set_ring_gap(EGUI_VIEW_OF(&test_ring), 3);

    egui_region_init(&region, 10, 20, 120, 120);
    egui_view_layout(EGUI_VIEW_OF(&test_ring), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_ring)->region_screen, &region);
}

static void test_activity_ring_value_change_uses_partial_dirty_region(void)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr();
    int32_t dirty_area;
    int32_t full_area;

    setup_ring();

    egui_core_clear_region_dirty();
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&test_ring), 0, 75);

    egui_core_clear_region_dirty();
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&test_ring), 0, 80);

    dirty_area = region_area(&arr[0]);
    full_area = region_area(&EGUI_VIEW_OF(&test_ring)->region_screen);

    EGUI_TEST_ASSERT_FALSE(egui_region_is_empty(&arr[0]));
    EGUI_TEST_ASSERT_FALSE(egui_region_equal(&arr[0], &EGUI_VIEW_OF(&test_ring)->region_screen));
    EGUI_TEST_ASSERT_TRUE(dirty_area < full_area);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

static void test_activity_ring_repeated_value_change_same_frame_falls_back_to_full_dirty_region(void)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    setup_ring();
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&test_ring), 0, 75);

    egui_core_clear_region_dirty();
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&test_ring), 0, 80);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&test_ring), 0, 25);

    EGUI_TEST_ASSERT_REGION_EQUAL(&EGUI_VIEW_OF(&test_ring)->region_screen, &arr[0]);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

static void test_activity_ring_hidden_ring_change_skips_dirty_region(void)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    setup_ring();
    egui_view_activity_ring_set_ring_count(EGUI_VIEW_OF(&test_ring), 1);

    egui_core_clear_region_dirty();
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&test_ring), 2, 50);

    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[0]));
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

void test_activity_ring_dirty_run(void)
{
    EGUI_TEST_SUITE_BEGIN(activity_ring_dirty);
    EGUI_TEST_RUN(test_activity_ring_value_change_uses_partial_dirty_region);
    EGUI_TEST_RUN(test_activity_ring_repeated_value_change_same_frame_falls_back_to_full_dirty_region);
    EGUI_TEST_RUN(test_activity_ring_hidden_ring_change_skips_dirty_region);
    EGUI_TEST_SUITE_END();
}
