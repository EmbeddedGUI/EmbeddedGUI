#include "egui.h"
#include "test/egui_test.h"
#include "test_canvas_active.h"
#include <string.h>

#define TEST_CANVAS_W 80
#define TEST_CANVAS_H 40

static egui_color_int_t test_pfb[TEST_CANVAS_W * TEST_CANVAS_H];
static egui_color_int_t expected_pfb[TEST_CANVAS_W * TEST_CANVAS_H];

static const uint16_t canvas_helper_image_data[] = {
        0xF800, 0x07E0, 0x001F, 0xFFE0, 0xFFFF, 0x07FF, 0xF81F, 0x8410, 0x0000,
};

static const egui_image_std_info_t canvas_helper_image_info = {
        .data_buf = canvas_helper_image_data,
        .alpha_buf = NULL,
        .data_type = EGUI_IMAGE_DATA_TYPE_RGB565,
        .alpha_type = EGUI_IMAGE_ALPHA_TYPE_1,
        .res_type = EGUI_RESOURCE_TYPE_INTERNAL,
        .width = 3,
        .height = 3,
};

EGUI_IMAGE_SUB_DEFINE_STATIC(egui_image_std_t, canvas_helper_image, &canvas_helper_image_info);

#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
static const uint16_t canvas_circle_mask_image_data[] = {
        0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x001F, 0x001F, 0x001F,
        0x001F, 0x001F, 0xFFE0, 0xFFE0, 0xFFE0, 0xFFE0, 0xFFE0, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
};

static const egui_image_std_info_t canvas_circle_mask_image_info = {
        .data_buf = canvas_circle_mask_image_data,
        .alpha_buf = NULL,
        .data_type = EGUI_IMAGE_DATA_TYPE_RGB565,
        .alpha_type = EGUI_IMAGE_ALPHA_TYPE_1,
        .res_type = EGUI_RESOURCE_TYPE_INTERNAL,
        .width = 5,
        .height = 5,
};

EGUI_IMAGE_SUB_DEFINE_STATIC(egui_image_std_t, canvas_circle_mask_image, &canvas_circle_mask_image_info);
#endif

static void setup_canvas(const egui_region_t *pfb_region)
{
    egui_region_t base_region;

    egui_canvas_init(test_pfb, (egui_region_t *)pfb_region);
    egui_region_init(&base_region, 30, 50, 80, 80);
    egui_canvas_calc_work_region(&base_region);
}

static void setup_canvas_local_full(void)
{
    egui_region_t pfb_region;
    egui_region_t base_region;

    memset(test_pfb, 0, sizeof(test_pfb));
    egui_region_init(&pfb_region, 0, 0, TEST_CANVAS_W, TEST_CANVAS_H);
    egui_canvas_init(test_pfb, &pfb_region);
    egui_region_init(&base_region, 0, 0, TEST_CANVAS_W, TEST_CANVAS_H);
    egui_canvas_calc_work_region(&base_region);
}

static void test_canvas_is_region_active_inside(void)
{
    egui_region_t pfb_region;
    egui_region_t region;

    egui_region_init(&pfb_region, 40, 60, 20, 20);
    setup_canvas(&pfb_region);

    egui_region_init(&region, 45, 65, 10, 10);
    EGUI_TEST_ASSERT_TRUE(egui_canvas_is_region_active(&region));
}

static void test_canvas_is_region_active_outside(void)
{
    egui_region_t pfb_region;
    egui_region_t region;

    egui_region_init(&pfb_region, 40, 60, 20, 20);
    setup_canvas(&pfb_region);

    egui_region_init(&region, 5, 5, 10, 10);
    EGUI_TEST_ASSERT_FALSE(egui_canvas_is_region_active(&region));
}

static void test_canvas_is_region_active_partial(void)
{
    egui_region_t pfb_region;
    egui_region_t region;

    egui_region_init(&pfb_region, 40, 60, 20, 20);
    setup_canvas(&pfb_region);

    egui_region_init(&region, 55, 75, 20, 10);
    EGUI_TEST_ASSERT_TRUE(egui_canvas_is_region_active(&region));
}

static void test_canvas_is_region_active_edge(void)
{
    egui_region_t pfb_region;
    egui_region_t region;

    egui_region_init(&pfb_region, 40, 60, 20, 20);
    setup_canvas(&pfb_region);

    egui_region_init(&region, 60, 60, 10, 10);
    EGUI_TEST_ASSERT_FALSE(egui_canvas_is_region_active(&region));
}

static void test_canvas_round_rect_fill_tiny_size_falls_back_to_rect(void)
{
    egui_color_t color = EGUI_COLOR_RED;

    setup_canvas_local_full();
    egui_canvas_draw_round_rectangle_fill(2, 3, 1, 1, 8, color, EGUI_ALPHA_100);

    EGUI_TEST_ASSERT_EQUAL_INT((int)color.full, (int)test_pfb[3 * TEST_CANVAS_W + 2]);
}

static void test_canvas_round_rect_stroke_tiny_size_falls_back_to_fill(void)
{
    egui_color_t color = EGUI_COLOR_BLUE;

    setup_canvas_local_full();
    egui_canvas_draw_round_rectangle(5, 6, 1, 1, 8, 2, color, EGUI_ALPHA_100);

    EGUI_TEST_ASSERT_EQUAL_INT((int)color.full, (int)test_pfb[6 * TEST_CANVAS_W + 5]);
}

static void test_canvas_round_rect_corners_fill_tiny_size_falls_back_to_rect(void)
{
    egui_color_t color = EGUI_COLOR_GREEN;

    setup_canvas_local_full();
    egui_canvas_draw_round_rectangle_corners_fill(8, 4, 1, 1, 6, 6, 6, 6, color, EGUI_ALPHA_100);

    EGUI_TEST_ASSERT_EQUAL_INT((int)color.full, (int)test_pfb[4 * TEST_CANVAS_W + 8]);
}

static void test_canvas_round_rect_corners_stroke_tiny_size_falls_back_to_fill(void)
{
    egui_color_t color = EGUI_COLOR_ORANGE;

    setup_canvas_local_full();
    egui_canvas_draw_round_rectangle_corners(11, 7, 1, 1, 6, 6, 6, 6, 2, color, EGUI_ALPHA_100);

    EGUI_TEST_ASSERT_EQUAL_INT((int)color.full, (int)test_pfb[7 * TEST_CANVAS_W + 11]);
}

static void test_canvas_arc_sweep_helper_matches_direct_range(void)
{
    setup_canvas_local_full();
    egui_canvas_draw_arc(20, 20, 8, 0, 90, 2, EGUI_COLOR_RED, EGUI_ALPHA_100);
    memcpy(expected_pfb, test_pfb, sizeof(test_pfb));

    setup_canvas_local_full();
    egui_canvas_draw_arc_sweep(20, 20, 8, 90, -90, 2, EGUI_COLOR_RED, EGUI_ALPHA_100);

    EGUI_TEST_ASSERT_TRUE(memcmp(expected_pfb, test_pfb, sizeof(test_pfb)) == 0);
}

static void test_canvas_image_rotate_helper_zero_angle_matches_draw_image(void)
{
    const egui_image_t *image = (const egui_image_t *)&canvas_helper_image;

    setup_canvas_local_full();
    egui_canvas_draw_image(image, 6, 7);
    memcpy(expected_pfb, test_pfb, sizeof(test_pfb));

    setup_canvas_local_full();
    egui_canvas_draw_image_rotate(image, 6, 7, 360);

    EGUI_TEST_ASSERT_TRUE(memcmp(expected_pfb, test_pfb, sizeof(test_pfb)) == 0);
}

static void test_canvas_text_rotate_helper_zero_angle_matches_draw_text(void)
{
    const egui_font_t *font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;

    setup_canvas_local_full();
    egui_canvas_draw_text(font, "A", 8, 6, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    memcpy(expected_pfb, test_pfb, sizeof(test_pfb));

    setup_canvas_local_full();
    egui_canvas_draw_text_rotate(font, "A", 8, 6, 360, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    EGUI_TEST_ASSERT_TRUE(memcmp(expected_pfb, test_pfb, sizeof(test_pfb)) == 0);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
static void test_canvas_circle_mask_draw_image_handles_cold_mask_cache(void)
{
    enum
    {
        image_x = 18,
        image_y = 9,
        image_size = 5,
    };
    const egui_image_t *image = (const egui_image_t *)&canvas_circle_mask_image;
    egui_mask_circle_t mask;
    egui_mask_t *base = (egui_mask_t *)&mask;
    int has_non_zero_pixel = 0;

    memset(&mask, 0, sizeof(mask));
    egui_mask_circle_init(base);
    egui_mask_set_position(base, image_x, image_y);
    egui_mask_set_size(base, image_size, image_size);

    setup_canvas_local_full();
    for (egui_dim_t y = 0; y < image_size; y++)
    {
        for (egui_dim_t x = 0; x < image_size; x++)
        {
            egui_color_t color;
            egui_alpha_t alpha = EGUI_ALPHA_100;

            color.full = EGUI_COLOR_RGB565_TRANS(canvas_circle_mask_image_data[y * image_size + x]);
            base->api->mask_point(base, image_x + x, image_y + y, &color, &alpha);
            if (alpha != 0)
            {
                has_non_zero_pixel = 1;
                egui_canvas_draw_point(image_x + x, image_y + y, color, alpha);
            }
        }
    }
    memcpy(expected_pfb, test_pfb, sizeof(test_pfb));

    memset(&mask, 0, sizeof(mask));
    egui_mask_circle_init(base);
    egui_mask_set_position(base, image_x, image_y);
    egui_mask_set_size(base, image_size, image_size);

    setup_canvas_local_full();
    egui_canvas_set_mask(base);
    egui_canvas_draw_image(image, image_x, image_y);
    egui_canvas_clear_mask();

    EGUI_TEST_ASSERT_TRUE(has_non_zero_pixel);
    EGUI_TEST_ASSERT_TRUE(memcmp(expected_pfb, test_pfb, sizeof(test_pfb)) == 0);
}
#endif

void test_canvas_active_run(void)
{
    EGUI_TEST_SUITE_BEGIN(canvas_active);
    EGUI_TEST_RUN(test_canvas_is_region_active_inside);
    EGUI_TEST_RUN(test_canvas_is_region_active_outside);
    EGUI_TEST_RUN(test_canvas_is_region_active_partial);
    EGUI_TEST_RUN(test_canvas_is_region_active_edge);
    EGUI_TEST_RUN(test_canvas_round_rect_fill_tiny_size_falls_back_to_rect);
    EGUI_TEST_RUN(test_canvas_round_rect_stroke_tiny_size_falls_back_to_fill);
    EGUI_TEST_RUN(test_canvas_round_rect_corners_fill_tiny_size_falls_back_to_rect);
    EGUI_TEST_RUN(test_canvas_round_rect_corners_stroke_tiny_size_falls_back_to_fill);
    EGUI_TEST_RUN(test_canvas_arc_sweep_helper_matches_direct_range);
    EGUI_TEST_RUN(test_canvas_image_rotate_helper_zero_angle_matches_draw_image);
    EGUI_TEST_RUN(test_canvas_text_rotate_helper_zero_angle_matches_draw_text);
#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
    EGUI_TEST_RUN(test_canvas_circle_mask_draw_image_handles_cold_mask_cache);
#endif
    EGUI_TEST_SUITE_END();
}
