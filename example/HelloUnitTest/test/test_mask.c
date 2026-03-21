#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_mask.h"

static void test_image_mask_null_point_passthrough(void)
{
    egui_mask_image_t mask;
    egui_mask_t *base = (egui_mask_t *)&mask;
    egui_color_t color;
    egui_color_t original_color;
    egui_alpha_t alpha;
    egui_alpha_t original_alpha;

    memset(&mask, 0, sizeof(mask));
    egui_mask_image_init(base);
    egui_mask_set_position(base, 10, 10);
    egui_mask_set_size(base, 20, 20);

    original_color = EGUI_COLOR_HEX(0x13579B);
    color = original_color;
    original_alpha = 123;
    alpha = original_alpha;
    base->api->mask_point(base, 15, 15, &color, &alpha);
    EGUI_TEST_ASSERT_EQUAL_INT(original_color.full, color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(original_alpha, alpha);

    color = original_color;
    alpha = original_alpha;
    base->api->mask_point(base, -5, -5, &color, &alpha);
    EGUI_TEST_ASSERT_EQUAL_INT(original_color.full, color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(original_alpha, alpha);
}

static void test_image_mask_null_row_queries_passthrough(void)
{
    egui_mask_image_t mask;
    egui_mask_t *base = (egui_mask_t *)&mask;
    egui_dim_t x_start = -1;
    egui_dim_t x_end = -1;
    int result;

    memset(&mask, 0, sizeof(mask));
    egui_mask_image_init(base);
    egui_mask_set_position(base, 10, 10);
    egui_mask_set_size(base, 20, 20);

    result = base->api->mask_get_row_range(base, -3, 2, 9, &x_start, &x_end);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_MASK_ROW_INSIDE, result);
    EGUI_TEST_ASSERT_EQUAL_INT(2, x_start);
    EGUI_TEST_ASSERT_EQUAL_INT(9, x_end);

    result = base->api->mask_get_row_visible_range(base, 100, 4, 11, &x_start, &x_end);
    EGUI_TEST_ASSERT_TRUE(result);
    EGUI_TEST_ASSERT_EQUAL_INT(4, x_start);
    EGUI_TEST_ASSERT_EQUAL_INT(11, x_end);
}

static void test_round_rectangle_init_resets_radius(void)
{
    egui_mask_round_rectangle_t mask;
    egui_mask_t *base = (egui_mask_t *)&mask;
    egui_dim_t x_start = -1;
    egui_dim_t x_end = -1;
    int result;

    memset(&mask, 0x7F, sizeof(mask));
    egui_mask_round_rectangle_init(base);
    egui_mask_set_position(base, 10, 20);
    egui_mask_set_size(base, 30, 40);

    result = base->api->mask_get_row_range(base, 25, 10, 40, &x_start, &x_end);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_MASK_ROW_INSIDE, result);
    EGUI_TEST_ASSERT_EQUAL_INT(10, x_start);
    EGUI_TEST_ASSERT_EQUAL_INT(40, x_end);
}

static void test_round_rectangle_radius_is_clamped(void)
{
    egui_mask_round_rectangle_t mask;
    egui_mask_t *base = (egui_mask_t *)&mask;
    egui_dim_t x_start = -1;
    egui_dim_t x_end = -1;
    int result;

    memset(&mask, 0, sizeof(mask));
    egui_mask_round_rectangle_init(base);
    egui_mask_set_position(base, 0, 0);
    egui_mask_set_size(base, 20, 13);
    egui_mask_round_rectangle_set_radius(base, 100);

    result = base->api->mask_get_row_range(base, 6, 0, 20, &x_start, &x_end);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_MASK_ROW_INSIDE, result);
    EGUI_TEST_ASSERT_EQUAL_INT(0, x_start);
    EGUI_TEST_ASSERT_EQUAL_INT(20, x_end);
}

static void test_round_rectangle_row_range_outside_when_no_overlap(void)
{
    egui_mask_round_rectangle_t mask;
    egui_mask_t *base = (egui_mask_t *)&mask;
    egui_dim_t x_start = -1;
    egui_dim_t x_end = -1;
    int result;

    memset(&mask, 0, sizeof(mask));
    egui_mask_round_rectangle_init(base);
    egui_mask_set_position(base, 10, 10);
    egui_mask_set_size(base, 20, 10);
    egui_mask_round_rectangle_set_radius(base, 2);

    result = base->api->mask_get_row_range(base, 12, 0, 5, &x_start, &x_end);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_MASK_ROW_OUTSIDE, result);
}

void test_mask_run(void)
{
    EGUI_TEST_SUITE_BEGIN(mask);

    EGUI_TEST_RUN(test_image_mask_null_point_passthrough);
    EGUI_TEST_RUN(test_image_mask_null_row_queries_passthrough);
    EGUI_TEST_RUN(test_round_rectangle_init_resets_radius);
    EGUI_TEST_RUN(test_round_rectangle_radius_is_clamped);
    EGUI_TEST_RUN(test_round_rectangle_row_range_outside_when_no_overlap);

    EGUI_TEST_SUITE_END();
}
