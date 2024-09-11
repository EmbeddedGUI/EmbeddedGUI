#include <stdio.h>
#include <assert.h>

#include "egui_view_test_performance.h"
#include "egui.h"

// Becouse of flash size limit, we can only enable some test cases.
#define EGUI_TEST_CONFIG_IMAGE_565     1
#define EGUI_TEST_CONFIG_IMAGE_565_1   0
#define EGUI_TEST_CONFIG_IMAGE_565_2   0
#define EGUI_TEST_CONFIG_IMAGE_565_4   0
#define EGUI_TEST_CONFIG_IMAGE_565_8   0

// Test real alpha 
#define EGUI_TEST_REAL_ALPHA_IMAGE     0

#if EGUI_TEST_REAL_ALPHA_IMAGE == 0
#if EGUI_CONFIG_SCEEN_WIDTH == 320 && EGUI_CONFIG_SCEEN_HEIGHT == 240
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_0    egui_res_image_test_img_1_1280_960_rgb565##_0
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_1    egui_res_image_test_img_1_1280_960_rgb565##_1
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_2    egui_res_image_test_img_1_1280_960_rgb565##_2
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_4    egui_res_image_test_img_1_1280_960_rgb565##_4
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_8    egui_res_image_test_img_1_1280_960_rgb565##_8
#elif EGUI_CONFIG_SCEEN_WIDTH == 240 && EGUI_CONFIG_SCEEN_HEIGHT == 240
// #define EGUI_TEST_PERFORMANCE_IMAGE_NAME egui_res_image_test_img_4_1280_1280_rgb565
// #define EGUI_TEST_PERFORMANCE_IMAGE_NAME egui_res_image_test_img_5_1280_1280_rgb565
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_0    egui_res_image_test_img_2_1280_1280_rgb565##_0
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_1    egui_res_image_test_img_2_1280_1280_rgb565##_1
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_2    egui_res_image_test_img_2_1280_1280_rgb565##_2
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_4    egui_res_image_test_img_2_1280_1280_rgb565##_4
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_8    egui_res_image_test_img_2_1280_1280_rgb565##_8
#elif EGUI_CONFIG_SCEEN_WIDTH == 320 && EGUI_CONFIG_SCEEN_HEIGHT == 172
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_0    egui_res_image_test_img_6_1280_688_rgb565##_0
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_1    egui_res_image_test_img_6_1280_688_rgb565##_1
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_2    egui_res_image_test_img_6_1280_688_rgb565##_2
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_4    egui_res_image_test_img_6_1280_688_rgb565##_4
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_8    egui_res_image_test_img_6_1280_688_rgb565##_8
#else
// default is 240*320
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_0    egui_res_image_test_img_0_960_1280_rgb565##_0
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_1    egui_res_image_test_img_0_960_1280_rgb565##_1
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_2    egui_res_image_test_img_0_960_1280_rgb565##_2
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_4    egui_res_image_test_img_0_960_1280_rgb565##_4
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_8    egui_res_image_test_img_0_960_1280_rgb565##_8
#endif
#else
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_0    egui_res_image_star_rgb565##_0
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_1    egui_res_image_star_rgb565##_1
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_2    egui_res_image_star_rgb565##_2
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_4    egui_res_image_star_rgb565##_4
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_8    egui_res_image_star_rgb565##_8
#endif


uint16_t egui_view_test_performance_adjust_circle(uint16_t radius)
{
    return EGUI_MIN(radius, EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE);
}

static void egui_view_test_performance_test_line(egui_view_t *self)
{
    egui_dim_t line_len = EGUI_MIN(self->region.size.width, self->region.size.height);
    egui_canvas_draw_line(0, 0, line_len, line_len, 20, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    // egui_canvas_draw_line(line_len, line_len, 0, 0, 20, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    // egui_canvas_draw_line(0, line_len, line_len, 0, 20, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    // egui_canvas_draw_line(0, line_len, line_len/2, 0, 20, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_image_565(egui_view_t *self)
{
#if EGUI_TEST_CONFIG_IMAGE_565
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_IMAGE_NAME_0;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_NAME_0, 0, 0);
#endif // EGUI_TEST_CONFIG_IMAGE_565
}

static void egui_view_test_performance_test_image_565_1(egui_view_t *self)
{
#if EGUI_TEST_CONFIG_IMAGE_565_1
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_IMAGE_NAME_1;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_NAME_1, 0, 0);
#endif // EGUI_TEST_CONFIG_IMAGE_565_1
}

static void egui_view_test_performance_test_image_565_2(egui_view_t *self)
{
#if EGUI_TEST_CONFIG_IMAGE_565_2
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_IMAGE_NAME_2;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_NAME_2, 0, 0);
#endif // EGUI_TEST_CONFIG_IMAGE_565_2
}

static void egui_view_test_performance_test_image_565_4(egui_view_t *self)
{
#if EGUI_TEST_CONFIG_IMAGE_565_4
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_IMAGE_NAME_4;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_NAME_4, 0, 0);
#endif // EGUI_TEST_CONFIG_IMAGE_565_4
}

static void egui_view_test_performance_test_image_565_8(egui_view_t *self)
{
#if EGUI_TEST_CONFIG_IMAGE_565_8
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_IMAGE_NAME_8;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_NAME_8, 0, 0);
#endif // EGUI_TEST_CONFIG_IMAGE_565_8
}

static void egui_view_test_performance_test_image_resize_565(egui_view_t *self)
{
#if EGUI_TEST_CONFIG_IMAGE_565
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_IMAGE_NAME_0;
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_NAME_0, 0, 0, self->region.size.width, self->region.size.height);
#endif // EGUI_TEST_CONFIG_IMAGE_565
}

static void egui_view_test_performance_test_image_resize_565_1(egui_view_t *self)
{
#if EGUI_TEST_CONFIG_IMAGE_565_1
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_IMAGE_NAME_1;
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_NAME_1, 0, 0, self->region.size.width, self->region.size.height);
#endif // EGUI_TEST_CONFIG_IMAGE_565_1
}

static void egui_view_test_performance_test_image_resize_565_2(egui_view_t *self)
{
#if EGUI_TEST_CONFIG_IMAGE_565_2
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_IMAGE_NAME_2;
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_NAME_2, 0, 0, self->region.size.width, self->region.size.height);
#endif // EGUI_TEST_CONFIG_IMAGE_565_2
}

static void egui_view_test_performance_test_image_resize_565_4(egui_view_t *self)
{
#if EGUI_TEST_CONFIG_IMAGE_565_4
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_IMAGE_NAME_4;
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_NAME_4, 0, 0, self->region.size.width, self->region.size.height);
#endif // EGUI_TEST_CONFIG_IMAGE_565_4
}

static void egui_view_test_performance_test_image_resize_565_8(egui_view_t *self)
{
#if EGUI_TEST_CONFIG_IMAGE_565_8
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_IMAGE_NAME_8;
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_NAME_8, 0, 0, self->region.size.width, self->region.size.height);
#endif // EGUI_TEST_CONFIG_IMAGE_565_8
}

const char test_str[] =
        "0123456789abcdefghijklmnopqrstuvwxyz^&*()_+-=[]{}|;':\\\",./"
        "<>?;"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnop"
        "qrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOP"
        "QRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdef"
        "ghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEF"
        "GHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ012345"
        "6789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuv"
        "wxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUV"
        "WXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijkl"
        "mnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKL"
        "MNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789ab"
        "cdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzAB"
        "CDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01"
        "23456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqr"
        "stuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQR"
        "STUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefgh"
        "ijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGH"
        "IJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567"
        "89abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

const char test_str_short[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static void egui_view_test_performance_test_text(egui_view_t *self)
{
    egui_canvas_draw_text((egui_font_t *)&egui_res_font_montserrat_26_4, test_str, 0, 0, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_text_rect(egui_view_t *self)
{
    EGUI_REGION_DEFINE(text_rect, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, 200);
    egui_canvas_draw_text_in_rect((egui_font_t *)&egui_res_font_montserrat_26_4, test_str, &text_rect, EGUI_ALIGN_LEFT, EGUI_COLOR_GREEN,
                                  EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_rectangle(egui_view_t *self)
{
    egui_dim_t x = 0;
    egui_dim_t y = 0;
    egui_dim_t width = EGUI_CONFIG_SCEEN_WIDTH;
    egui_dim_t height = EGUI_CONFIG_SCEEN_HEIGHT;
    egui_dim_t stroke_width = EGUI_MIN(width, height) >> 2;

    egui_canvas_draw_rectangle(x, y, width, height, stroke_width, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_circle(egui_view_t *self)
{
    egui_dim_t central_x = EGUI_CONFIG_SCEEN_WIDTH >> 1;
    egui_dim_t central_y = EGUI_CONFIG_SCEEN_HEIGHT >> 1;
    egui_dim_t radius = egui_view_test_performance_adjust_circle((EGUI_MIN(EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT) >> 1) - 1);
    egui_dim_t stroke_width = radius >> 1;
    egui_canvas_draw_circle(central_x, central_y, radius, stroke_width, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_arc(egui_view_t *self)
{
    egui_dim_t central_x = EGUI_CONFIG_SCEEN_WIDTH >> 1;
    egui_dim_t central_y = EGUI_CONFIG_SCEEN_HEIGHT >> 1;
    egui_dim_t radius = egui_view_test_performance_adjust_circle((EGUI_MIN(EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT) >> 1) - 1);
    egui_canvas_draw_arc(central_x, central_y, radius, 0, 60, 10, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_arc(central_x, central_y, radius, 90 + 0, 90 + 60, 10, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_arc(central_x, central_y, radius, 180 + 0, 180 + 60, 10, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_arc(central_x, central_y, radius, 270 + 0, 270 + 60, 10, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_round_rectangle(egui_view_t *self)
{
    egui_dim_t x = 0;
    egui_dim_t y = 0;
    egui_dim_t width = EGUI_CONFIG_SCEEN_WIDTH;
    egui_dim_t height = EGUI_CONFIG_SCEEN_HEIGHT;
    egui_dim_t radius = egui_view_test_performance_adjust_circle(EGUI_MIN(width, height) >> 1);
    egui_dim_t stroke_width = radius >> 1;

    egui_canvas_draw_round_rectangle(x, y, width, height, radius, stroke_width, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_round_rectangle_corners(egui_view_t *self)
{
    egui_dim_t x = 0;
    egui_dim_t y = 0;
    egui_dim_t width = EGUI_CONFIG_SCEEN_WIDTH;
    egui_dim_t height = EGUI_CONFIG_SCEEN_HEIGHT;
    egui_dim_t radius = egui_view_test_performance_adjust_circle(EGUI_MIN(width, height) >> 1);
    egui_dim_t radius_top_left = radius - 1;
    egui_dim_t radius_top_right = radius - 2;
    egui_dim_t radius_bottom_left = radius - 4;
    egui_dim_t radius_bottom_right = radius - 3;
    egui_dim_t stroke_width = (radius - 4) >> 1;

    egui_canvas_draw_round_rectangle_corners(x, y, width, height, radius_top_left, radius_bottom_left, radius_top_right, radius_bottom_right,
                                             stroke_width, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_rectangle_fill(egui_view_t *self)
{
    egui_dim_t x = 0;
    egui_dim_t y = 0;
    egui_dim_t width = EGUI_CONFIG_SCEEN_WIDTH;
    egui_dim_t height = EGUI_CONFIG_SCEEN_HEIGHT;

    egui_canvas_draw_rectangle_fill(x, y, width, height, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_circle_fill(egui_view_t *self)
{
    egui_dim_t central_x = EGUI_CONFIG_SCEEN_WIDTH >> 1;
    egui_dim_t central_y = EGUI_CONFIG_SCEEN_HEIGHT >> 1;
    egui_dim_t radius = egui_view_test_performance_adjust_circle((EGUI_MIN(EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT) >> 1) - 1);
    egui_canvas_draw_circle_fill(central_x, central_y, radius, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_arc_fill(egui_view_t *self)
{
    egui_dim_t central_x = EGUI_CONFIG_SCEEN_WIDTH >> 1;
    egui_dim_t central_y = EGUI_CONFIG_SCEEN_HEIGHT >> 1;
    egui_dim_t radius = egui_view_test_performance_adjust_circle((EGUI_MIN(EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT) >> 1) - 1);
    egui_canvas_draw_arc_fill(central_x, central_y, radius, 0, 60, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_arc_fill(central_x, central_y, radius, 90 + 0, 90 + 60, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_arc_fill(central_x, central_y, radius, 180 + 0, 180 + 60, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_arc_fill(central_x, central_y, radius, 270 + 0, 270 + 60, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_round_rectangle_fill(egui_view_t *self)
{
    egui_dim_t x = 0;
    egui_dim_t y = 0;
    egui_dim_t width = EGUI_CONFIG_SCEEN_WIDTH;
    egui_dim_t height = EGUI_CONFIG_SCEEN_HEIGHT;
    egui_dim_t radius = egui_view_test_performance_adjust_circle(EGUI_MIN(width, height) >> 1);

    egui_canvas_draw_round_rectangle_fill(x, y, width, height, radius, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_round_rectangle_corners_fill(egui_view_t *self)
{
    egui_dim_t x = 0;
    egui_dim_t y = 0;
    egui_dim_t width = EGUI_CONFIG_SCEEN_WIDTH;
    egui_dim_t height = EGUI_CONFIG_SCEEN_HEIGHT;
    egui_dim_t radius = egui_view_test_performance_adjust_circle(EGUI_MIN(width, height) >> 1);
    egui_dim_t radius_top_left = radius - 1;
    egui_dim_t radius_top_right = radius - 2;
    egui_dim_t radius_bottom_left = radius - 4;
    egui_dim_t radius_bottom_right = radius - 3;

    egui_canvas_draw_round_rectangle_corners_fill(x, y, width, height, radius_top_left, radius_bottom_left, radius_top_right, radius_bottom_right,
                                                  EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

// static void egui_view_test_performance_test_round_rectangle_border(egui_view_t *self)
// {
//     egui_dim_t radius = 40;
//     egui_dim_t width = 200;
//     egui_dim_t height = 250;
//     egui_dim_t stroke_width = 50;
//     egui_dim_t x = 10;
//     egui_dim_t y = 10;

//     egui_canvas_draw_round_rectangle_fill(x + stroke_width / 2, y + stroke_width / 2, width - stroke_width, height - stroke_width,
//                                           radius - stroke_width / 2, EGUI_COLOR_GREEN, EGUI_ALPHA_60);
//     egui_canvas_draw_round_rectangle(x, y, width, height, radius, stroke_width, EGUI_COLOR_WHITE, EGUI_ALPHA_60);
// }

// static void egui_view_test_performance_test_round_rectangle_corners_border(egui_view_t *self)
// {
//     egui_dim_t radius_left_top = 10;
//     egui_dim_t radius_left_bottom = 45;
//     egui_dim_t radius_right_top = 15;
//     egui_dim_t radius_right_bottom = 40;
//     egui_dim_t width = 200;
//     egui_dim_t height = 250;
//     egui_dim_t stroke_width = 20;
//     egui_dim_t x = 10;
//     egui_dim_t y = 10;

//     egui_canvas_draw_round_rectangle_corners_fill(x + stroke_width / 2, y + stroke_width / 2, width - stroke_width, height - stroke_width,
//                                                   radius_left_top - stroke_width / 2, radius_left_bottom - stroke_width / 2,
//                                                   radius_right_top - stroke_width / 2, radius_right_bottom - stroke_width / 2, EGUI_COLOR_GREEN, EGUI_ALPHA_60);
//     egui_canvas_draw_round_rectangle_corners(x, y, width, height, radius_left_top, radius_left_bottom, radius_right_top, radius_right_bottom,
//                                              stroke_width, EGUI_COLOR_WHITE, EGUI_ALPHA_60);
// }

void egui_view_test_performance_on_draw(egui_view_t *self)
{
    egui_view_test_performance_t *view = (egui_view_test_performance_t *)self;

    switch (view->test_mode)
    {
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_LINE:
        egui_view_test_performance_test_line(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565:
        egui_view_test_performance_test_image_565(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_1:
        egui_view_test_performance_test_image_565_1(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_2:
        egui_view_test_performance_test_image_565_2(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_4:
        egui_view_test_performance_test_image_565_4(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_8:
        egui_view_test_performance_test_image_565_8(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565:
        egui_view_test_performance_test_image_resize_565(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_1:
        egui_view_test_performance_test_image_resize_565_1(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_2:
        egui_view_test_performance_test_image_resize_565_2(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_4:
        egui_view_test_performance_test_image_resize_565_4(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_8:
        egui_view_test_performance_test_image_resize_565_8(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT:
        egui_view_test_performance_test_text(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_RECT:
        egui_view_test_performance_test_text_rect(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_RECTANGLE:
        egui_view_test_performance_test_rectangle(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE:
        egui_view_test_performance_test_circle(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ARC:
        egui_view_test_performance_test_arc(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ROUND_RECTANGLE:
        egui_view_test_performance_test_round_rectangle(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ROUND_RECTANGLE_CORNERS:
        egui_view_test_performance_test_round_rectangle_corners(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_RECTANGLE_FILL:
        egui_view_test_performance_test_rectangle_fill(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE_FILL:
        egui_view_test_performance_test_circle_fill(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ARC_FILL:
        egui_view_test_performance_test_arc_fill(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ROUND_RECTANGLE_FILL:
        egui_view_test_performance_test_round_rectangle_fill(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ROUND_RECTANGLE_CORNERS_FILL:
        egui_view_test_performance_test_round_rectangle_corners_fill(self);
        break;

    default:
        break;
    }
}

EGUI_VIEW_API_DEFINE(egui_view_test_performance_t, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, egui_view_test_performance_on_draw, NULL);

void egui_view_test_performance_init(egui_view_t *self)
{
    egui_view_test_performance_t *local = (egui_view_test_performance_t *)self;
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_test_performance_t);

    // init local data.
    local->test_mode = EGUI_VIEW_TEST_PERFORMANCE_TYPE_LINE;
}
