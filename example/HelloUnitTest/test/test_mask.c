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

static void test_circle_corner_fixed_row_matches_general_lookup(void)
{
    egui_dim_t radii[] = {1, 2, 5, 15, 30, EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE};

    for (size_t i = 0; i < sizeof(radii) / sizeof(radii[0]); i++)
    {
        egui_dim_t radius = radii[i];
        const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
        const egui_circle_item_t *items;

        if (radius > EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE || info == NULL)
        {
            continue;
        }

        items = (const egui_circle_item_t *)info->items;
        for (egui_dim_t row = 0; row <= radius; row++)
        {
            for (egui_dim_t col = 0; col <= radius; col++)
            {
                egui_alpha_t expected = egui_canvas_get_circle_corner_value(row, col, info);
                egui_alpha_t actual = egui_canvas_get_circle_corner_value_fixed_row(row, col, info, items);
                EGUI_TEST_ASSERT_EQUAL_INT(expected, actual);
            }
        }
    }
}

static void scan_circle_mask_row(egui_mask_t *base, egui_dim_t y, egui_alpha_t *row_alpha, egui_dim_t width)
{
    for (egui_dim_t x = 0; x < width; x++)
    {
        egui_color_t color = EGUI_COLOR_WHITE;
        egui_alpha_t alpha = EGUI_ALPHA_100;

        base->api->mask_point(base, x, y, &color, &alpha);
        row_alpha[x] = alpha;
    }
}

static void test_circle_mask_visible_range_center_row_without_row_range_cache(void)
{
    enum
    {
        circle_size = 100,
    };
    egui_mask_circle_t mask;
    egui_mask_t *base = (egui_mask_t *)&mask;
    egui_alpha_t row_alpha[circle_size];
    egui_dim_t visible_x_start = -1;
    egui_dim_t visible_x_end = -1;
    egui_dim_t expected_visible_start = circle_size;
    egui_dim_t expected_visible_end = 0;
    egui_dim_t center_y = circle_size >> 1;
    int visible_result;

    memset(&mask, 0, sizeof(mask));
    egui_mask_circle_init(base);
    egui_mask_set_position(base, 0, 0);
    egui_mask_set_size(base, circle_size, circle_size);

    visible_result = base->api->mask_get_row_visible_range(base, center_y, 0, circle_size, &visible_x_start, &visible_x_end);
    scan_circle_mask_row(base, center_y, row_alpha, circle_size);

    for (egui_dim_t x = 0; x < circle_size; x++)
    {
        if (row_alpha[x] != 0)
        {
            if (expected_visible_start == circle_size)
            {
                expected_visible_start = x;
            }
            expected_visible_end = x + 1;
        }
    }

    EGUI_TEST_ASSERT_TRUE(visible_result);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_visible_start, visible_x_start);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_visible_end, visible_x_end);
}

static void test_circle_mask_row_queries_match_point_sampling(void)
{
    enum
    {
        circle_size = 100,
    };
    egui_mask_circle_t mask;
    egui_mask_t *base = (egui_mask_t *)&mask;
    egui_alpha_t row_alpha[circle_size];

    memset(&mask, 0, sizeof(mask));
    egui_mask_circle_init(base);
    egui_mask_set_position(base, 0, 0);
    egui_mask_set_size(base, circle_size, circle_size);

    for (egui_dim_t y = 0; y < circle_size; y++)
    {
        egui_dim_t opaque_x_start = -1;
        egui_dim_t opaque_x_end = -1;
        egui_dim_t visible_x_start = -1;
        egui_dim_t visible_x_end = -1;
        egui_dim_t expected_opaque_start = circle_size;
        egui_dim_t expected_opaque_end = 0;
        egui_dim_t expected_visible_start = circle_size;
        egui_dim_t expected_visible_end = 0;
        int expected_range_result;
        int actual_range_result;
        int actual_visible_result;
        int expected_visible_result;

        actual_range_result = base->api->mask_get_row_range(base, y, 0, circle_size, &opaque_x_start, &opaque_x_end);
        actual_visible_result = base->api->mask_get_row_visible_range(base, y, 0, circle_size, &visible_x_start, &visible_x_end);

        scan_circle_mask_row(base, y, row_alpha, circle_size);

        for (egui_dim_t x = 0; x < circle_size; x++)
        {
            if (row_alpha[x] != 0)
            {
                if (expected_visible_start == circle_size)
                {
                    expected_visible_start = x;
                }
                expected_visible_end = x + 1;
            }

            if (row_alpha[x] == EGUI_ALPHA_100)
            {
                if (expected_opaque_start == circle_size)
                {
                    expected_opaque_start = x;
                }
                expected_opaque_end = x + 1;
            }
        }

        expected_visible_result = (expected_visible_start < expected_visible_end);
        if (!expected_visible_result)
        {
            expected_range_result = EGUI_MASK_ROW_OUTSIDE;
            expected_opaque_start = 0;
            expected_opaque_end = 0;
            expected_visible_start = 0;
            expected_visible_end = 0;
        }
        else if (expected_opaque_start == 0 && expected_opaque_end == circle_size)
        {
            expected_range_result = EGUI_MASK_ROW_INSIDE;
        }
        else
        {
            expected_range_result = EGUI_MASK_ROW_PARTIAL;
            if (expected_opaque_start >= expected_opaque_end)
            {
                expected_opaque_start = 0;
                expected_opaque_end = 0;
            }
        }

        EGUI_TEST_ASSERT_EQUAL_INT(expected_range_result, actual_range_result);
        if (expected_range_result != EGUI_MASK_ROW_OUTSIDE)
        {
            EGUI_TEST_ASSERT_EQUAL_INT(expected_opaque_start, opaque_x_start);
            EGUI_TEST_ASSERT_EQUAL_INT(expected_opaque_end, opaque_x_end);
        }

        EGUI_TEST_ASSERT_EQUAL_INT(expected_visible_result, actual_visible_result);
        if (expected_visible_result)
        {
            EGUI_TEST_ASSERT_TRUE(visible_x_start <= expected_visible_start);
            EGUI_TEST_ASSERT_TRUE(visible_x_end >= expected_visible_end);
            EGUI_TEST_ASSERT_TRUE(visible_x_start >= 0);
            EGUI_TEST_ASSERT_TRUE(visible_x_end <= circle_size);
        }
    }
}

void test_mask_run(void)
{
    EGUI_TEST_SUITE_BEGIN(mask);

    EGUI_TEST_RUN(test_image_mask_null_point_passthrough);
    EGUI_TEST_RUN(test_image_mask_null_row_queries_passthrough);
    EGUI_TEST_RUN(test_round_rectangle_init_resets_radius);
    EGUI_TEST_RUN(test_round_rectangle_radius_is_clamped);
    EGUI_TEST_RUN(test_round_rectangle_row_range_outside_when_no_overlap);
    EGUI_TEST_RUN(test_circle_corner_fixed_row_matches_general_lookup);
    EGUI_TEST_RUN(test_circle_mask_visible_range_center_row_without_row_range_cache);
    EGUI_TEST_RUN(test_circle_mask_row_queries_match_point_sampling);

    EGUI_TEST_SUITE_END();
}
