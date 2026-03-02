#include <stdio.h>
#include <assert.h>

#include "egui_view_test_performance.h"
#include "egui.h"

#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_GRADIENT
#include "core/egui_canvas_gradient.h"
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
#include "shadow/egui_shadow.h"
#endif

#include "mask/egui_mask_circle.h"
#include "mask/egui_mask_round_rectangle.h"
#include "mask/egui_mask_image.h"

#include "anim/egui_animation_translate.h"
#include "anim/egui_animation_alpha.h"
#include "anim/egui_animation_scale_size.h"
#include "anim/egui_animation_set.h"
#include "anim/egui_interpolator_linear.h"

// Because of flash size limit, we can only enable some test cases.
// These can be overridden from app_egui_config.h for QEMU full coverage.
#ifndef EGUI_TEST_CONFIG_IMAGE_565
#define EGUI_TEST_CONFIG_IMAGE_565 1
#endif
#ifndef EGUI_TEST_CONFIG_IMAGE_565_1
#define EGUI_TEST_CONFIG_IMAGE_565_1 0
#endif
#ifndef EGUI_TEST_CONFIG_IMAGE_565_2
#define EGUI_TEST_CONFIG_IMAGE_565_2 0
#endif
#ifndef EGUI_TEST_CONFIG_IMAGE_565_4
#define EGUI_TEST_CONFIG_IMAGE_565_4 0
#endif
#ifndef EGUI_TEST_CONFIG_IMAGE_565_8
#define EGUI_TEST_CONFIG_IMAGE_565_8 0
#endif

// Test real alpha
#define EGUI_TEST_REAL_ALPHA_IMAGE 0

#if EGUI_TEST_REAL_ALPHA_IMAGE == 0
#if EGUI_CONFIG_SCEEN_WIDTH == 320 && EGUI_CONFIG_SCEEN_HEIGHT == 240
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_0 egui_res_image_test_img_1_1280_960_rgb565##_0
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_1 egui_res_image_test_img_1_1280_960_rgb565##_1
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_2 egui_res_image_test_img_1_1280_960_rgb565##_2
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_4 egui_res_image_test_img_1_1280_960_rgb565##_4
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_8 egui_res_image_test_img_1_1280_960_rgb565##_8
#elif EGUI_CONFIG_SCEEN_WIDTH == 240 && EGUI_CONFIG_SCEEN_HEIGHT == 240
// #define EGUI_TEST_PERFORMANCE_IMAGE_NAME egui_res_image_test_img_4_1280_1280_rgb565
// #define EGUI_TEST_PERFORMANCE_IMAGE_NAME egui_res_image_test_img_5_1280_1280_rgb565
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_0 egui_res_image_test_img_2_1280_1280_rgb565##_0
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_1 egui_res_image_test_img_2_1280_1280_rgb565##_1
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_2 egui_res_image_test_img_2_1280_1280_rgb565##_2
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_4 egui_res_image_test_img_2_1280_1280_rgb565##_4
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_8 egui_res_image_test_img_2_1280_1280_rgb565##_8
#elif EGUI_CONFIG_SCEEN_WIDTH == 320 && EGUI_CONFIG_SCEEN_HEIGHT == 172
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_0 egui_res_image_test_img_6_1280_688_rgb565##_0
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_1 egui_res_image_test_img_6_1280_688_rgb565##_1
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_2 egui_res_image_test_img_6_1280_688_rgb565##_2
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_4 egui_res_image_test_img_6_1280_688_rgb565##_4
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_8 egui_res_image_test_img_6_1280_688_rgb565##_8
#else
// default is 240*320
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_0 egui_res_image_test_img_0_960_1280_rgb565##_0
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_1 egui_res_image_test_img_0_960_1280_rgb565##_1
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_2 egui_res_image_test_img_0_960_1280_rgb565##_2
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_4 egui_res_image_test_img_0_960_1280_rgb565##_4
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_8 egui_res_image_test_img_0_960_1280_rgb565##_8
#endif
#else
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_0 egui_res_image_star_rgb565##_0
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_1 egui_res_image_star_rgb565##_1
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_2 egui_res_image_star_rgb565##_2
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_4 egui_res_image_star_rgb565##_4
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_8 egui_res_image_star_rgb565##_8
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
    egui_canvas_draw_text_in_rect((egui_font_t *)&egui_res_font_montserrat_26_4, test_str, &text_rect, EGUI_ALIGN_LEFT, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
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

    egui_canvas_draw_round_rectangle_corners(x, y, width, height, radius_top_left, radius_bottom_left, radius_top_right, radius_bottom_right, stroke_width,
                                             EGUI_COLOR_WHITE, EGUI_ALPHA_100);
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
//                                                   radius_right_top - stroke_width / 2, radius_right_bottom - stroke_width / 2, EGUI_COLOR_GREEN,
//                                                   EGUI_ALPHA_60);
//     egui_canvas_draw_round_rectangle_corners(x, y, width, height, radius_left_top, radius_left_bottom, radius_right_top, radius_right_bottom,
//                                              stroke_width, EGUI_COLOR_WHITE, EGUI_ALPHA_60);
// }

// ============================================================================
// Triangle tests
// ============================================================================

static void egui_view_test_performance_test_triangle(egui_view_t *self)
{
    egui_dim_t w = EGUI_CONFIG_SCEEN_WIDTH;
    egui_dim_t h = EGUI_CONFIG_SCEEN_HEIGHT;
    egui_canvas_draw_triangle(w / 2, 0, 0, h - 1, w - 1, h - 1, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_triangle_fill(egui_view_t *self)
{
    egui_dim_t w = EGUI_CONFIG_SCEEN_WIDTH;
    egui_dim_t h = EGUI_CONFIG_SCEEN_HEIGHT;
    egui_canvas_draw_triangle_fill(w / 2, 0, 0, h - 1, w - 1, h - 1, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

// ============================================================================
// Ellipse tests
// ============================================================================

#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_ELLIPSE
static void egui_view_test_performance_test_ellipse(egui_view_t *self)
{
    egui_dim_t cx = EGUI_CONFIG_SCEEN_WIDTH >> 1;
    egui_dim_t cy = EGUI_CONFIG_SCEEN_HEIGHT >> 1;
    egui_dim_t rx = (EGUI_CONFIG_SCEEN_WIDTH >> 1) - 1;
    egui_dim_t ry = (EGUI_CONFIG_SCEEN_HEIGHT >> 1) - 1;
    egui_canvas_draw_ellipse(cx, cy, rx, ry, 10, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_ellipse_fill(egui_view_t *self)
{
    egui_dim_t cx = EGUI_CONFIG_SCEEN_WIDTH >> 1;
    egui_dim_t cy = EGUI_CONFIG_SCEEN_HEIGHT >> 1;
    egui_dim_t rx = (EGUI_CONFIG_SCEEN_WIDTH >> 1) - 1;
    egui_dim_t ry = (EGUI_CONFIG_SCEEN_HEIGHT >> 1) - 1;
    egui_canvas_draw_ellipse_fill(cx, cy, rx, ry, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}
#endif

// ============================================================================
// Polygon tests
// ============================================================================

#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_POLYGON
static void egui_view_test_performance_test_polygon(egui_view_t *self)
{
    egui_dim_t w = EGUI_CONFIG_SCEEN_WIDTH;
    egui_dim_t h = EGUI_CONFIG_SCEEN_HEIGHT;
    /* Pentagon */
    const egui_dim_t points[] = {
            w / 2, 0, w - 1, h * 2 / 5, w * 4 / 5, h - 1, w / 5, h - 1, 0, h * 2 / 5,
    };
    egui_canvas_draw_polygon(points, 5, 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_polygon_fill(egui_view_t *self)
{
    egui_dim_t w = EGUI_CONFIG_SCEEN_WIDTH;
    egui_dim_t h = EGUI_CONFIG_SCEEN_HEIGHT;
    /* Pentagon */
    const egui_dim_t points[] = {
            w / 2, 0, w - 1, h * 2 / 5, w * 4 / 5, h - 1, w / 5, h - 1, 0, h * 2 / 5,
    };
    egui_canvas_draw_polygon_fill(points, 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}
#endif

// ============================================================================
// Bezier tests
// ============================================================================

#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_BEZIER
static void egui_view_test_performance_test_bezier_quad(egui_view_t *self)
{
    egui_dim_t w = EGUI_CONFIG_SCEEN_WIDTH;
    egui_dim_t h = EGUI_CONFIG_SCEEN_HEIGHT;
    egui_canvas_draw_bezier_quad(0, h - 1, w / 2, 0, w - 1, h - 1, 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_bezier_cubic(egui_view_t *self)
{
    egui_dim_t w = EGUI_CONFIG_SCEEN_WIDTH;
    egui_dim_t h = EGUI_CONFIG_SCEEN_HEIGHT;
    egui_canvas_draw_bezier_cubic(0, h / 2, w / 3, 0, w * 2 / 3, h - 1, w - 1, h / 2, 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}
#endif

// ============================================================================
// HQ circle/arc tests
// ============================================================================

#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_CIRCLE_HQ
static void egui_view_test_performance_test_circle_hq(egui_view_t *self)
{
    egui_dim_t cx = EGUI_CONFIG_SCEEN_WIDTH >> 1;
    egui_dim_t cy = EGUI_CONFIG_SCEEN_HEIGHT >> 1;
    egui_dim_t radius = (EGUI_MIN(EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT) >> 1) - 1;
    egui_dim_t stroke_width = radius >> 1;
    egui_canvas_draw_circle_hq(cx, cy, radius, stroke_width, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_circle_fill_hq(egui_view_t *self)
{
    egui_dim_t cx = EGUI_CONFIG_SCEEN_WIDTH >> 1;
    egui_dim_t cy = EGUI_CONFIG_SCEEN_HEIGHT >> 1;
    egui_dim_t radius = (EGUI_MIN(EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT) >> 1) - 1;
    egui_canvas_draw_circle_fill_hq(cx, cy, radius, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_arc_hq(egui_view_t *self)
{
    egui_dim_t cx = EGUI_CONFIG_SCEEN_WIDTH >> 1;
    egui_dim_t cy = EGUI_CONFIG_SCEEN_HEIGHT >> 1;
    egui_dim_t radius = (EGUI_MIN(EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT) >> 1) - 1;
    egui_canvas_draw_arc_hq(cx, cy, radius, 0, 60, 10, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_arc_hq(cx, cy, radius, 90, 150, 10, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_arc_hq(cx, cy, radius, 180, 240, 10, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_arc_hq(cx, cy, radius, 270, 330, 10, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_arc_fill_hq(egui_view_t *self)
{
    egui_dim_t cx = EGUI_CONFIG_SCEEN_WIDTH >> 1;
    egui_dim_t cy = EGUI_CONFIG_SCEEN_HEIGHT >> 1;
    egui_dim_t radius = (EGUI_MIN(EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT) >> 1) - 1;
    egui_canvas_draw_arc_fill_hq(cx, cy, radius, 0, 60, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_arc_fill_hq(cx, cy, radius, 90, 150, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_arc_fill_hq(cx, cy, radius, 180, 240, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_arc_fill_hq(cx, cy, radius, 270, 330, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}
#endif

// ============================================================================
// HQ line test
// ============================================================================

#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_LINE_HQ
static void egui_view_test_performance_test_line_hq(egui_view_t *self)
{
    egui_dim_t line_len = EGUI_MIN(self->region.size.width, self->region.size.height);
    egui_canvas_draw_line_hq(0, 0, line_len, line_len, 20, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}
#endif

// ============================================================================
// Gradient tests
// ============================================================================

#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_GRADIENT
EGUI_GRADIENT_LINEAR_2COLOR(perf_gradient_v, EGUI_GRADIENT_TYPE_LINEAR_VERTICAL, EGUI_COLOR_MAKE(0xFF, 0x00, 0x00), EGUI_COLOR_MAKE(0x00, 0x00, 0xFF),
                            EGUI_ALPHA_100);

static const egui_gradient_stop_t perf_radial_stops[] = {
        {.position = 0, .color = EGUI_COLOR_MAKE(0xFF, 0xFF, 0x00)},
        {.position = 255, .color = EGUI_COLOR_MAKE(0x00, 0x80, 0x00)},
};
static const egui_gradient_t perf_gradient_radial = {
        .type = EGUI_GRADIENT_TYPE_RADIAL,
        .stop_count = 2,
        .alpha = EGUI_ALPHA_100,
        .stops = perf_radial_stops,
        .center_x = 0,
        .center_y = 0,
        .radius = 0, /* will be set per shape */
};

static void egui_view_test_performance_test_gradient_rect(egui_view_t *self)
{
    egui_canvas_draw_rectangle_fill_gradient(0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, &perf_gradient_v);
}

static void egui_view_test_performance_test_gradient_round_rect(egui_view_t *self)
{
    egui_dim_t radius = egui_view_test_performance_adjust_circle(EGUI_MIN(EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT) >> 2);
    egui_canvas_draw_round_rectangle_fill_gradient(0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, radius, &perf_gradient_v);
}

static void egui_view_test_performance_test_gradient_circle(egui_view_t *self)
{
    egui_dim_t cx = EGUI_CONFIG_SCEEN_WIDTH >> 1;
    egui_dim_t cy = EGUI_CONFIG_SCEEN_HEIGHT >> 1;
    egui_dim_t radius = egui_view_test_performance_adjust_circle((EGUI_MIN(EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT) >> 1) - 1);
    /* Use radial gradient with center at circle center */
    egui_gradient_t grad = perf_gradient_radial;
    grad.center_x = cx;
    grad.center_y = cy;
    grad.radius = radius;
    egui_canvas_draw_circle_fill_gradient(cx, cy, radius, &grad);
}

static void egui_view_test_performance_test_gradient_triangle(egui_view_t *self)
{
    egui_dim_t w = EGUI_CONFIG_SCEEN_WIDTH;
    egui_dim_t h = EGUI_CONFIG_SCEEN_HEIGHT;
    egui_canvas_draw_triangle_fill_gradient(w / 2, 0, 0, h - 1, w - 1, h - 1, &perf_gradient_v);
}
#endif

// ============================================================================
// Shadow tests
// ============================================================================

#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
static void egui_view_test_performance_test_shadow(egui_view_t *self)
{
    EGUI_SHADOW_PARAM_INIT(shadow, 20, 5, 5, EGUI_COLOR_BLACK, EGUI_ALPHA_60);
    egui_dim_t margin = 30;
    EGUI_REGION_DEFINE(region, margin, margin, EGUI_CONFIG_SCEEN_WIDTH - margin * 2, EGUI_CONFIG_SCEEN_HEIGHT - margin * 2);
    egui_shadow_draw(&shadow, &region);
}

static void egui_view_test_performance_test_shadow_round(egui_view_t *self)
{
    EGUI_SHADOW_PARAM_INIT_ROUND(shadow, 20, 5, 5, EGUI_COLOR_BLACK, EGUI_ALPHA_60, 30);
    egui_dim_t margin = 30;
    EGUI_REGION_DEFINE(region, margin, margin, EGUI_CONFIG_SCEEN_WIDTH - margin * 2, EGUI_CONFIG_SCEEN_HEIGHT - margin * 2);
    egui_shadow_draw(&shadow, &region);
}
#endif

// ============================================================================
// Mask performance tests
// ============================================================================

// Test image resource (reuse from HelloBasic/mask)
extern const egui_image_std_t egui_res_image_test_rgb565_8;
extern const egui_image_std_t egui_res_image_star_rgb565_8;

// Static mask instances
static egui_mask_round_rectangle_t perf_mask_round_rect;
static egui_mask_circle_t perf_mask_circle;
static egui_mask_image_t perf_mask_image;

#define MASK_TEST_SIZE   200
#define MASK_TEST_RADIUS 30

static int perf_masks_initialized = 0;

static void ensure_perf_masks_initialized(void)
{
    if (perf_masks_initialized)
    {
        return;
    }
    perf_masks_initialized = 1;

    // Round-rectangle mask
    egui_mask_round_rectangle_init((egui_mask_t *)&perf_mask_round_rect);
    egui_mask_set_position((egui_mask_t *)&perf_mask_round_rect, 0, 0);
    egui_mask_set_size((egui_mask_t *)&perf_mask_round_rect, MASK_TEST_SIZE, MASK_TEST_SIZE);
    egui_mask_round_rectangle_set_radius((egui_mask_t *)&perf_mask_round_rect, MASK_TEST_RADIUS);

    // Circle mask
    egui_mask_circle_init((egui_mask_t *)&perf_mask_circle);
    egui_mask_set_position((egui_mask_t *)&perf_mask_circle, 0, 0);
    egui_mask_set_size((egui_mask_t *)&perf_mask_circle, MASK_TEST_SIZE, MASK_TEST_SIZE);

    // Image mask (star shape)
    egui_mask_image_init((egui_mask_t *)&perf_mask_image);
    egui_mask_set_position((egui_mask_t *)&perf_mask_image, 0, 0);
    egui_mask_set_size((egui_mask_t *)&perf_mask_image, 72, 72);
    egui_mask_image_set_image((egui_mask_t *)&perf_mask_image, (egui_image_t *)&egui_res_image_star_rgb565_8);
}

static void egui_view_test_performance_test_mask_rect_fill_no_mask(egui_view_t *self)
{
    egui_canvas_draw_rectangle_fill(0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_mask_rect_fill_round_rect(egui_view_t *self)
{
    egui_canvas_set_mask((egui_mask_t *)&perf_mask_round_rect);
    egui_canvas_draw_rectangle_fill(0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_rect_fill_circle(egui_view_t *self)
{
    egui_canvas_set_mask((egui_mask_t *)&perf_mask_circle);
    egui_canvas_draw_rectangle_fill(0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_rect_fill_image(egui_view_t *self)
{
    egui_canvas_set_mask((egui_mask_t *)&perf_mask_image);
    egui_canvas_draw_rectangle_fill(0, 0, 72, 72, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_no_mask(egui_view_t *self)
{
    egui_canvas_draw_image_resize((egui_image_t *)&egui_res_image_test_rgb565_8, 0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE);
}

static void egui_view_test_performance_test_mask_image_round_rect(egui_view_t *self)
{
    egui_canvas_set_mask((egui_mask_t *)&perf_mask_round_rect);
    egui_canvas_draw_image_resize((egui_image_t *)&egui_res_image_test_rgb565_8, 0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_circle(egui_view_t *self)
{
    egui_canvas_set_mask((egui_mask_t *)&perf_mask_circle);
    egui_canvas_draw_image_resize((egui_image_t *)&egui_res_image_test_rgb565_8, 0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_image(egui_view_t *self)
{
    egui_canvas_set_mask((egui_mask_t *)&perf_mask_image);
    egui_canvas_draw_image_resize((egui_image_t *)&egui_res_image_test_rgb565_8, 0, 0, 72, 72);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_round_rect_fill_no_mask(egui_view_t *self)
{
    egui_canvas_draw_round_rectangle_fill(0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE, MASK_TEST_RADIUS, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_mask_round_rect_fill_with_mask(egui_view_t *self)
{
    egui_canvas_set_mask((egui_mask_t *)&perf_mask_round_rect);
    egui_canvas_draw_rectangle_fill(0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_clear_mask();
}

// ============================================================================
// Animation performance tests
// ============================================================================

#define ANIM_TEST_SIZE 60

static egui_view_t anim_perf_view;
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(anim_perf_bg_param, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(anim_perf_bg_params, &anim_perf_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(anim_perf_bg, &anim_perf_bg_params);

static int anim_perf_view_initialized = 0;
static egui_interpolator_linear_t anim_perf_interp;

static void ensure_anim_perf_view_initialized(void)
{
    if (anim_perf_view_initialized)
    {
        return;
    }
    anim_perf_view_initialized = 1;

    egui_view_init(EGUI_VIEW_OF(&anim_perf_view));
    egui_view_set_position(EGUI_VIEW_OF(&anim_perf_view), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&anim_perf_view), ANIM_TEST_SIZE, ANIM_TEST_SIZE);
    egui_view_set_background(EGUI_VIEW_OF(&anim_perf_view), EGUI_BG_OF(&anim_perf_bg));
    egui_interpolator_linear_init((egui_interpolator_t *)&anim_perf_interp);
}

static void egui_view_test_performance_test_animation_translate(egui_view_t *self)
{
    ensure_anim_perf_view_initialized();
    static egui_animation_translate_t anim;
    EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(params, 0, 0, 0, EGUI_CONFIG_SCEEN_HEIGHT - ANIM_TEST_SIZE);

    egui_animation_translate_init(EGUI_ANIM_OF(&anim));
    egui_animation_translate_params_set(&anim, &params);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim), 1000);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&anim), (egui_interpolator_t *)&anim_perf_interp);
    egui_animation_target_view_set(EGUI_ANIM_OF(&anim), EGUI_VIEW_OF(&anim_perf_view));
    egui_animation_start(EGUI_ANIM_OF(&anim));
    egui_animation_update(EGUI_ANIM_OF(&anim), 500);
    egui_canvas_draw_rectangle_fill(anim_perf_view.region.location.x, anim_perf_view.region.location.y, ANIM_TEST_SIZE, ANIM_TEST_SIZE, EGUI_COLOR_GREEN,
                                    EGUI_ALPHA_100);
    egui_animation_stop(EGUI_ANIM_OF(&anim));
    egui_view_set_position(EGUI_VIEW_OF(&anim_perf_view), 0, 0);
}

static void egui_view_test_performance_test_animation_alpha(egui_view_t *self)
{
    ensure_anim_perf_view_initialized();
    static egui_animation_alpha_t anim;
    EGUI_ANIMATION_ALPHA_PARAMS_INIT(params, EGUI_ALPHA_100, EGUI_ALPHA_0);

    egui_animation_alpha_init(EGUI_ANIM_OF(&anim));
    egui_animation_alpha_params_set(&anim, &params);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim), 1000);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&anim), (egui_interpolator_t *)&anim_perf_interp);
    egui_animation_target_view_set(EGUI_ANIM_OF(&anim), EGUI_VIEW_OF(&anim_perf_view));
    egui_animation_start(EGUI_ANIM_OF(&anim));
    egui_animation_update(EGUI_ANIM_OF(&anim), 500);
    egui_canvas_draw_rectangle_fill(0, 0, ANIM_TEST_SIZE, ANIM_TEST_SIZE, EGUI_COLOR_GREEN, anim_perf_view.alpha);
    egui_animation_stop(EGUI_ANIM_OF(&anim));
    egui_view_set_alpha(EGUI_VIEW_OF(&anim_perf_view), EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_animation_scale(egui_view_t *self)
{
    ensure_anim_perf_view_initialized();
    static egui_animation_scale_size_t anim;
    EGUI_ANIMATION_SCALE_SIZE_PARAMS_INIT(params, EGUI_FLOAT_VALUE(0.5f), EGUI_FLOAT_VALUE(1.5f));

    egui_animation_scale_size_init(EGUI_ANIM_OF(&anim));
    egui_animation_scale_size_params_set(&anim, &params);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim), 1000);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&anim), (egui_interpolator_t *)&anim_perf_interp);
    egui_animation_target_view_set(EGUI_ANIM_OF(&anim), EGUI_VIEW_OF(&anim_perf_view));
    egui_animation_start(EGUI_ANIM_OF(&anim));
    egui_animation_update(EGUI_ANIM_OF(&anim), 500);
    egui_canvas_draw_rectangle_fill(anim_perf_view.region.location.x, anim_perf_view.region.location.y, anim_perf_view.region.size.width,
                                    anim_perf_view.region.size.height, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_animation_stop(EGUI_ANIM_OF(&anim));
    egui_view_set_position(EGUI_VIEW_OF(&anim_perf_view), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&anim_perf_view), ANIM_TEST_SIZE, ANIM_TEST_SIZE);
}

static void egui_view_test_performance_test_animation_set(egui_view_t *self)
{
    ensure_anim_perf_view_initialized();
    static egui_animation_translate_t anim_tr;
    static egui_animation_alpha_t anim_al;
    static egui_animation_set_t anim_set;
    EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(tr_params, 0, 0, 0, EGUI_CONFIG_SCEEN_HEIGHT - ANIM_TEST_SIZE);
    EGUI_ANIMATION_ALPHA_PARAMS_INIT(al_params, EGUI_ALPHA_100, EGUI_ALPHA_0);

    egui_animation_translate_init(EGUI_ANIM_OF(&anim_tr));
    egui_animation_translate_params_set(&anim_tr, &tr_params);
    egui_animation_alpha_init(EGUI_ANIM_OF(&anim_al));
    egui_animation_alpha_params_set(&anim_al, &al_params);

    egui_animation_set_init(EGUI_ANIM_OF(&anim_set));
    egui_animation_set_add_animation(&anim_set, EGUI_ANIM_OF(&anim_tr));
    egui_animation_set_add_animation(&anim_set, EGUI_ANIM_OF(&anim_al));
    egui_animation_set_set_mask(&anim_set, 1, 1, 1, 1, 1);

    egui_animation_duration_set(EGUI_ANIM_OF(&anim_set), 1000);
    egui_animation_interpolator_set(EGUI_ANIM_OF(&anim_set), (egui_interpolator_t *)&anim_perf_interp);
    egui_animation_target_view_set(EGUI_ANIM_OF(&anim_set), EGUI_VIEW_OF(&anim_perf_view));
    egui_animation_start(EGUI_ANIM_OF(&anim_set));
    egui_animation_update(EGUI_ANIM_OF(&anim_set), 500);
    egui_canvas_draw_rectangle_fill(anim_perf_view.region.location.x, anim_perf_view.region.location.y, ANIM_TEST_SIZE, ANIM_TEST_SIZE, EGUI_COLOR_GREEN,
                                    anim_perf_view.alpha);
    egui_animation_stop(EGUI_ANIM_OF(&anim_set));
    egui_view_set_position(EGUI_VIEW_OF(&anim_perf_view), 0, 0);
    egui_view_set_alpha(EGUI_VIEW_OF(&anim_perf_view), EGUI_ALPHA_100);
}

// ============================================================================
// on_draw dispatch
// ============================================================================

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

    // Triangle
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TRIANGLE:
        egui_view_test_performance_test_triangle(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TRIANGLE_FILL:
        egui_view_test_performance_test_triangle_fill(self);
        break;

    // Ellipse
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ELLIPSE:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_ELLIPSE
        egui_view_test_performance_test_ellipse(self);
#endif
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ELLIPSE_FILL:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_ELLIPSE
        egui_view_test_performance_test_ellipse_fill(self);
#endif
        break;

    // Polygon
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_POLYGON:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_POLYGON
        egui_view_test_performance_test_polygon(self);
#endif
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_POLYGON_FILL:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_POLYGON
        egui_view_test_performance_test_polygon_fill(self);
#endif
        break;

    // Bezier
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_BEZIER_QUAD:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_BEZIER
        egui_view_test_performance_test_bezier_quad(self);
#endif
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_BEZIER_CUBIC:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_BEZIER
        egui_view_test_performance_test_bezier_cubic(self);
#endif
        break;

    // HQ circle/arc
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE_HQ:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_CIRCLE_HQ
        egui_view_test_performance_test_circle_hq(self);
#endif
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE_FILL_HQ:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_CIRCLE_HQ
        egui_view_test_performance_test_circle_fill_hq(self);
#endif
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ARC_HQ:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_CIRCLE_HQ
        egui_view_test_performance_test_arc_hq(self);
#endif
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ARC_FILL_HQ:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_CIRCLE_HQ
        egui_view_test_performance_test_arc_fill_hq(self);
#endif
        break;

    // HQ line
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_LINE_HQ:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_LINE_HQ
        egui_view_test_performance_test_line_hq(self);
#endif
        break;

    // Gradient
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_RECT:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_GRADIENT
        egui_view_test_performance_test_gradient_rect(self);
#endif
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ROUND_RECT:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_GRADIENT
        egui_view_test_performance_test_gradient_round_rect(self);
#endif
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_CIRCLE:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_GRADIENT
        egui_view_test_performance_test_gradient_circle(self);
#endif
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_TRIANGLE:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_GRADIENT
        egui_view_test_performance_test_gradient_triangle(self);
#endif
        break;

    // Shadow
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_SHADOW:
#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
        egui_view_test_performance_test_shadow(self);
#endif
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_SHADOW_ROUND:
#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
        egui_view_test_performance_test_shadow_round(self);
#endif
        break;

    // Mask performance tests
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_NO_MASK:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_rect_fill_no_mask(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_ROUND_RECT:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_rect_fill_round_rect(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_CIRCLE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_rect_fill_circle(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_IMAGE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_rect_fill_image(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_NO_MASK:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_no_mask(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_ROUND_RECT:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_round_rect(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_CIRCLE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_circle(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_IMAGE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_image(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_ROUND_RECT_FILL_NO_MASK:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_round_rect_fill_no_mask(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_ROUND_RECT_FILL_WITH_MASK:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_round_rect_fill_with_mask(self);
        break;

    // Animation performance tests
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ANIMATION_TRANSLATE:
        egui_view_test_performance_test_animation_translate(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ANIMATION_ALPHA:
        egui_view_test_performance_test_animation_alpha(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ANIMATION_SCALE:
        egui_view_test_performance_test_animation_scale(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ANIMATION_SET:
        egui_view_test_performance_test_animation_set(self);
        break;

    default:
        break;
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_test_performance_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_test_performance_on_draw, // changed
        .on_detach_from_window = egui_view_on_detach_from_window,
};

int egui_view_test_performance_is_enabled(int test_mode)
{
    switch (test_mode)
    {
    // Image tests gated by compile-time config
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565:
        return EGUI_TEST_CONFIG_IMAGE_565;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_1:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_1:
        return EGUI_TEST_CONFIG_IMAGE_565_1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_2:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_2:
        return EGUI_TEST_CONFIG_IMAGE_565_2;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_4:
        return EGUI_TEST_CONFIG_IMAGE_565_4;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_8:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_8:
        return EGUI_TEST_CONFIG_IMAGE_565_8;

    // Optional feature tests
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ELLIPSE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ELLIPSE_FILL:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_ELLIPSE
        return 1;
#else
        return 0;
#endif
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_POLYGON:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_POLYGON_FILL:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_POLYGON
        return 1;
#else
        return 0;
#endif
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_BEZIER_QUAD:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_BEZIER_CUBIC:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_BEZIER
        return 1;
#else
        return 0;
#endif
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE_HQ:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE_FILL_HQ:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ARC_HQ:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ARC_FILL_HQ:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_CIRCLE_HQ
        return 1;
#else
        return 0;
#endif
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_LINE_HQ:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_LINE_HQ
        return 1;
#else
        return 0;
#endif
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_TRIANGLE:
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_GRADIENT
        return 1;
#else
        return 0;
#endif
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_SHADOW:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_SHADOW_ROUND:
#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
        return 1;
#else
        return 0;
#endif

    default:
        return 1;
    }
}

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
