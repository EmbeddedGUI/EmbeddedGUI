#include "egui.h"
#include "test/egui_test.h"
#include "test_dirty_region.h"

#if EGUI_CONFIG_DIRTY_AREA_COUNT != 2
#error "EGUI_CONFIG_DIRTY_AREA_COUNT should be 2"
#endif

// Test: single region, basic case
static void test_dirty_single_basic(void)
{
    egui_region_t region;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    egui_core_clear_region_dirty();
    egui_region_init(&region, 0, 0, 50, 50);
    egui_core_update_region_dirty(&region);

    EGUI_TEST_ASSERT_REGION_EQUAL(&region, &arr[0]);
    for (int i = 1; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[i]));
    }
}

// Test: full screen region
static void test_dirty_single_fullscreen(void)
{
    egui_region_t region;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    egui_region_init(&region, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_core_clear_region_dirty();
    egui_core_update_region_dirty(&region);

    EGUI_TEST_ASSERT_REGION_EQUAL(&region, &arr[0]);
    for (int i = 1; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[i]));
    }
}

// Test: region exceeding screen gets clamped
static void test_dirty_single_clamp_overflow(void)
{
    egui_region_t region;
    egui_region_t expected;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    egui_region_init(&region, 0, 0, EGUI_CONFIG_SCEEN_WIDTH + 30, EGUI_CONFIG_SCEEN_HEIGHT + 30);
    egui_core_clear_region_dirty();
    egui_core_update_region_dirty(&region);

    egui_region_init(&expected, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected, &arr[0]);
}

// Test: region with offset exceeding screen gets clamped
static void test_dirty_single_clamp_offset(void)
{
    egui_region_t region;
    egui_region_t expected;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    egui_region_init(&region, 30, 30, EGUI_CONFIG_SCEEN_WIDTH + 30, EGUI_CONFIG_SCEEN_HEIGHT + 30);
    egui_core_clear_region_dirty();
    egui_core_update_region_dirty(&region);

    egui_region_init(&expected, 30, 30, EGUI_CONFIG_SCEEN_WIDTH - 30, EGUI_CONFIG_SCEEN_HEIGHT - 30);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected, &arr[0]);
}

// Test: region completely outside screen (right-bottom)
static void test_dirty_single_outside_rb(void)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    egui_region_t region;
    egui_region_init(&region, EGUI_CONFIG_SCEEN_WIDTH + 30, EGUI_CONFIG_SCEEN_HEIGHT + 30, 30, 30);
    egui_core_clear_region_dirty();
    egui_core_update_region_dirty(&region);

    for (int i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[i]));
    }
}

// Test: region completely outside screen (right)
static void test_dirty_single_outside_right(void)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    egui_region_t region;
    egui_region_init(&region, EGUI_CONFIG_SCEEN_WIDTH + 30, 0, 30, 30);
    egui_core_clear_region_dirty();
    egui_core_update_region_dirty(&region);

    for (int i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[i]));
    }
}

// Test: region completely outside screen (bottom)
static void test_dirty_single_outside_bottom(void)
{
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    egui_region_t region;
    egui_region_init(&region, 0, EGUI_CONFIG_SCEEN_HEIGHT + 30, 30, 30);
    egui_core_clear_region_dirty();
    egui_core_update_region_dirty(&region);

    for (int i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[i]));
    }
}

// Test: two non-overlapping regions
static void test_dirty_two_separate(void)
{
    egui_region_t region;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    egui_core_clear_region_dirty();
    egui_region_init(&region, 0, 0, 50, 50);
    egui_core_update_region_dirty(&region);
    egui_region_init(&region, 50, 0, 50, 50);
    egui_core_update_region_dirty(&region);

    egui_region_init(&region, 0, 0, 50, 50);
    EGUI_TEST_ASSERT_REGION_EQUAL(&region, &arr[0]);
    egui_region_init(&region, 50, 0, 50, 50);
    EGUI_TEST_ASSERT_REGION_EQUAL(&region, &arr[1]);
}

// Test: second region contains first
static void test_dirty_two_contain(void)
{
    egui_region_t region;
    egui_region_t expected;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    egui_core_clear_region_dirty();
    egui_region_init(&region, 0, 0, 50, 50);
    egui_core_update_region_dirty(&region);
    egui_region_init(&region, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_core_update_region_dirty(&region);

    egui_region_init(&expected, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected, &arr[0]);
    for (int i = 1; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[i]));
    }
}

// Test: second region exceeds screen, clamped and contains first
static void test_dirty_two_overflow_contain(void)
{
    egui_region_t region;
    egui_region_t expected;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    egui_core_clear_region_dirty();
    egui_region_init(&region, 0, 0, 50, 50);
    egui_core_update_region_dirty(&region);
    egui_region_init(&region, 0, 0, EGUI_CONFIG_SCEEN_WIDTH + 30, EGUI_CONFIG_SCEEN_HEIGHT + 30);
    egui_core_update_region_dirty(&region);

    egui_region_init(&expected, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected, &arr[0]);
    for (int i = 1; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[i]));
    }
}

// Test: second region is outside, first remains
static void test_dirty_two_second_outside(void)
{
    egui_region_t region;
    egui_region_t expected;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    egui_core_clear_region_dirty();
    egui_region_init(&region, 0, 0, 50, 50);
    egui_core_update_region_dirty(&region);
    egui_region_init(&region, EGUI_CONFIG_SCEEN_WIDTH, 0, EGUI_CONFIG_SCEEN_WIDTH + 30, EGUI_CONFIG_SCEEN_HEIGHT + 30);
    egui_core_update_region_dirty(&region);

    egui_region_init(&expected, 0, 0, 50, 50);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected, &arr[0]);
    for (int i = 1; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[i]));
    }
}

// Test: three regions, third covers all (fullscreen merge)
static void test_dirty_two_three_fullscreen(void)
{
    egui_region_t region;
    egui_region_t expected;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    egui_core_clear_region_dirty();
    egui_region_init(&region, 0, 0, 50, 50);
    egui_core_update_region_dirty(&region);
    egui_region_init(&region, 50, 0, 50, 50);
    egui_core_update_region_dirty(&region);
    egui_region_init(&region, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_core_update_region_dirty(&region);

    egui_region_init(&expected, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected, &arr[0]);
    for (int i = 1; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[i]));
    }
}

// Test: three overlapping regions merged
static void test_dirty_two_three_overlap_merge(void)
{
    egui_region_t region;
    egui_region_t expected;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    egui_core_clear_region_dirty();
    egui_region_init(&region, 0, 0, 50, 50);
    egui_core_update_region_dirty(&region);
    egui_region_init(&region, 40, 0, 50, 50);
    egui_core_update_region_dirty(&region);
    egui_region_init(&region, 0, 40, 50, 50);
    egui_core_update_region_dirty(&region);

    egui_region_init(&expected, 0, 0, 50 + 40, 50 + 40);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected, &arr[0]);
    for (int i = 1; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[i]));
    }
}

// Test: three regions, exceed dirty area count, merged with smallest union area
static void test_dirty_three_exceed_merge(void)
{
    egui_region_t region;
    egui_region_t expected;
    egui_region_t *arr = egui_core_get_region_dirty_arr();

    egui_core_clear_region_dirty();
    // slot 0: (0,0,50,50)
    egui_region_init(&region, 0, 0, 50, 50);
    egui_core_update_region_dirty(&region);
    // slot 1: (50,0,50,50)
    egui_region_init(&region, 50, 0, 50, 50);
    egui_core_update_region_dirty(&region);
    // no empty slot, merge with smallest union area: slot 0 union (0,50,50,50) = (0,0,50,100)
    egui_region_init(&region, 0, 50, 50, 50);
    egui_core_update_region_dirty(&region);

    // slot 0 merged: (0,0) size (50,100)
    egui_region_init(&expected, 0, 0, 50, 100);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected, &arr[0]);
    // slot 1 unchanged: (50,0) size (50,50)
    egui_region_init(&expected, 50, 0, 50, 50);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected, &arr[1]);
}

void test_dirty_region_run(void)
{
    EGUI_TEST_SUITE_BEGIN(dirty_region);

    EGUI_TEST_RUN(test_dirty_single_basic);
    EGUI_TEST_RUN(test_dirty_single_fullscreen);
    EGUI_TEST_RUN(test_dirty_single_clamp_overflow);
    EGUI_TEST_RUN(test_dirty_single_clamp_offset);
    EGUI_TEST_RUN(test_dirty_single_outside_rb);
    EGUI_TEST_RUN(test_dirty_single_outside_right);
    EGUI_TEST_RUN(test_dirty_single_outside_bottom);
    EGUI_TEST_RUN(test_dirty_two_separate);
    EGUI_TEST_RUN(test_dirty_two_contain);
    EGUI_TEST_RUN(test_dirty_two_overflow_contain);
    EGUI_TEST_RUN(test_dirty_two_second_outside);
    EGUI_TEST_RUN(test_dirty_two_three_fullscreen);
    EGUI_TEST_RUN(test_dirty_two_three_overlap_merge);
    EGUI_TEST_RUN(test_dirty_three_exceed_merge);

    EGUI_TEST_SUITE_END();
}
