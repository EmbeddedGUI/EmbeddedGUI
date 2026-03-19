#include "egui.h"
#include "test/egui_test.h"
#include "test_canvas_active.h"
#include <string.h>

static egui_color_int_t test_pfb[20 * 20];

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
    egui_region_init(&pfb_region, 0, 0, 20, 20);
    egui_canvas_init(test_pfb, &pfb_region);
    egui_region_init(&base_region, 0, 0, 20, 20);
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

    EGUI_TEST_ASSERT_EQUAL_INT((int)color.full, (int)test_pfb[3 * 20 + 2]);
}

static void test_canvas_round_rect_stroke_tiny_size_falls_back_to_fill(void)
{
    egui_color_t color = EGUI_COLOR_BLUE;

    setup_canvas_local_full();
    egui_canvas_draw_round_rectangle(5, 6, 1, 1, 8, 2, color, EGUI_ALPHA_100);

    EGUI_TEST_ASSERT_EQUAL_INT((int)color.full, (int)test_pfb[6 * 20 + 5]);
}

static void test_canvas_round_rect_corners_fill_tiny_size_falls_back_to_rect(void)
{
    egui_color_t color = EGUI_COLOR_GREEN;

    setup_canvas_local_full();
    egui_canvas_draw_round_rectangle_corners_fill(8, 4, 1, 1, 6, 6, 6, 6, color, EGUI_ALPHA_100);

    EGUI_TEST_ASSERT_EQUAL_INT((int)color.full, (int)test_pfb[4 * 20 + 8]);
}

static void test_canvas_round_rect_corners_stroke_tiny_size_falls_back_to_fill(void)
{
    egui_color_t color = EGUI_COLOR_ORANGE;

    setup_canvas_local_full();
    egui_canvas_draw_round_rectangle_corners(11, 7, 1, 1, 6, 6, 6, 6, 2, color, EGUI_ALPHA_100);

    EGUI_TEST_ASSERT_EQUAL_INT((int)color.full, (int)test_pfb[7 * 20 + 11]);
}

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
    EGUI_TEST_SUITE_END();
}
