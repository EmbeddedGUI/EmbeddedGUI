#include "egui.h"
#include "uicode.h"

#include "core/egui_canvas_gradient.h"
#include "image/egui_image_qoi.h"
#include "image/egui_image_rle.h"
#include "image/egui_image_std.h"
#include "mask/egui_mask_circle.h"
#include "mask/egui_mask_image.h"
#include "mask/egui_mask_round_rectangle.h"
#include "size_analysis_probe_config.h"

#if defined(__GNUC__)
#define EGUI_SIZE_PROBE_FUNC static void __attribute__((unused))
#else
#define EGUI_SIZE_PROBE_FUNC static void
#endif

static const egui_dim_t probe_polyline_points[] = {
        2, 2, 10, 8, 18, 2,
};

static const egui_gradient_stop_t probe_gradient_stops[] = {
        {.position = 0, .color = EGUI_COLOR_RED},
        {.position = 255, .color = EGUI_COLOR_BLUE},
};

static const egui_gradient_t probe_linear_gradient = {
        .type = EGUI_GRADIENT_TYPE_LINEAR_HORIZONTAL,
        .stop_count = 2,
        .alpha = EGUI_ALPHA_100,
        .stops = probe_gradient_stops,
};

static const uint16_t probe_image_rgb565_data[] = {
        0xF800,
        0x07E0,
        0x001F,
        0xFFFF,
};

static const uint8_t probe_image_alpha8_data[] = {
        255,
        160,
        96,
        32,
};

static const egui_image_std_info_t probe_image_std_info = {
        .data_buf = probe_image_rgb565_data,
        .alpha_buf = probe_image_alpha8_data,
        .data_type = EGUI_IMAGE_DATA_TYPE_RGB565,
        .alpha_type = EGUI_IMAGE_ALPHA_TYPE_8,
        .res_type = EGUI_RESOURCE_TYPE_INTERNAL,
        .width = 2,
        .height = 2,
};

EGUI_IMAGE_SUB_DEFINE_CONST(egui_image_std_t, probe_image_std, &probe_image_std_info);

#if EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE
static const uint8_t probe_image_rle_data[] = {
        0x00,
};

static const egui_image_rle_info_t probe_image_rle_info = {
        .data_buf = probe_image_rle_data,
        .alpha_buf = NULL,
        .data_type = EGUI_IMAGE_DATA_TYPE_RGB565,
        .alpha_type = EGUI_IMAGE_ALPHA_TYPE_1,
        .res_type = EGUI_RESOURCE_TYPE_INTERNAL,
        .width = 1,
        .height = 1,
        .data_size = sizeof(probe_image_rle_data),
        .alpha_size = 0,
        .decompressed_size = 2,
};

EGUI_IMAGE_SUB_DEFINE_CONST(egui_image_rle_t, probe_image_rle, &probe_image_rle_info);
#endif

#if EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE
static const uint8_t probe_image_qoi_data[] = {
        0x00,
};

static const egui_image_qoi_info_t probe_image_qoi_info = {
        .data_buf = probe_image_qoi_data,
        .alpha_buf = NULL,
        .data_type = EGUI_IMAGE_DATA_TYPE_RGB565,
        .alpha_type = EGUI_IMAGE_ALPHA_TYPE_8,
        .res_type = EGUI_RESOURCE_TYPE_INTERNAL,
        .channels = 4,
        .width = 1,
        .height = 1,
        .data_size = sizeof(probe_image_qoi_data),
        .decompressed_size = 4,
};

EGUI_IMAGE_SUB_DEFINE_CONST(egui_image_qoi_t, probe_image_qoi, &probe_image_qoi_info);
#endif

volatile int g_canvas_size_probe_runtime_enable = 0;

EGUI_SIZE_PROBE_FUNC canvas_probe_rect_stroke(void)
{
    egui_canvas_draw_rectangle(0, 0, 12, 8, 2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_rect_fill(void)
{
    egui_canvas_draw_rectangle_fill(1, 1, 10, 6, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_round_rect_stroke(void)
{
    egui_canvas_draw_round_rectangle(0, 0, 16, 10, 4, 2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_round_rect_fill(void)
{
    egui_canvas_draw_round_rectangle_fill(0, 0, 16, 10, 4, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_triangle_stroke(void)
{
    egui_canvas_draw_triangle(0, 10, 8, 0, 16, 10, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_triangle_fill(void)
{
    egui_canvas_draw_triangle_fill(0, 12, 8, 2, 16, 12, EGUI_COLOR_BLUE, EGUI_ALPHA_100);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_circle_basic_stroke(void)
{
    egui_canvas_draw_circle_basic(12, 12, 8, 2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_circle_basic_fill(void)
{
    egui_canvas_draw_circle_fill_basic(12, 12, 8, EGUI_COLOR_RED, EGUI_ALPHA_100);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_arc_basic_stroke(void)
{
    egui_canvas_draw_arc_basic(12, 12, 8, 0, 120, 2, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_arc_basic_fill(void)
{
    egui_canvas_draw_arc_fill_basic(12, 12, 8, 0, 120, EGUI_COLOR_BLUE, EGUI_ALPHA_100);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_line(void)
{
    egui_canvas_draw_line(0, 0, 18, 8, 2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_polyline(void)
{
    egui_canvas_draw_polyline(probe_polyline_points, 3, 2, EGUI_COLOR_BLUE, EGUI_ALPHA_100);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_gradient_rect(void)
{
    egui_canvas_draw_rectangle_fill_gradient(0, 0, 18, 10, &probe_linear_gradient);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_gradient_round_rect(void)
{
    egui_canvas_draw_round_rectangle_fill_gradient(0, 0, 18, 10, 4, &probe_linear_gradient);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_gradient_circle(void)
{
    egui_canvas_draw_circle_fill_gradient(10, 10, 8, &probe_linear_gradient);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_mask_circle(void)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
    egui_mask_circle_t mask = {0};

    egui_mask_circle_init((egui_mask_t *)&mask);
    egui_mask_set_position((egui_mask_t *)&mask, 0, 0);
    egui_mask_set_size((egui_mask_t *)&mask, 20, 20);
    egui_canvas_set_mask((egui_mask_t *)&mask);
    egui_canvas_draw_rectangle_fill(0, 0, 20, 20, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_clear_mask();
#endif
}

EGUI_SIZE_PROBE_FUNC canvas_probe_mask_round_rect(void)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
    egui_mask_round_rectangle_t mask = {0};

    egui_mask_round_rectangle_init((egui_mask_t *)&mask);
    egui_mask_set_position((egui_mask_t *)&mask, 0, 0);
    egui_mask_set_size((egui_mask_t *)&mask, 20, 20);
    egui_mask_round_rectangle_set_radius((egui_mask_t *)&mask, 4);
    egui_canvas_set_mask((egui_mask_t *)&mask);
    egui_canvas_draw_rectangle_fill(0, 0, 20, 20, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_clear_mask();
#endif
}

EGUI_SIZE_PROBE_FUNC canvas_probe_mask_image(void)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
    egui_mask_image_t mask = {0};

    egui_mask_image_init((egui_mask_t *)&mask);
    egui_mask_set_position((egui_mask_t *)&mask, 0, 0);
    egui_mask_set_size((egui_mask_t *)&mask, 4, 4);
    egui_mask_image_set_image((egui_mask_t *)&mask, (egui_image_t *)&probe_image_std);
    egui_canvas_set_mask((egui_mask_t *)&mask);
    egui_canvas_draw_rectangle_fill(0, 0, 4, 4, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_clear_mask();
#endif
}

EGUI_SIZE_PROBE_FUNC canvas_probe_image_draw(void)
{
    egui_canvas_draw_image((const egui_image_t *)&probe_image_std, 0, 0);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_image_resize(void)
{
    egui_canvas_draw_image_resize((const egui_image_t *)&probe_image_std, 0, 0, 4, 4);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_image_tint(void)
{
    egui_canvas_draw_image_color((const egui_image_t *)&probe_image_std, 0, 0, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_image_rotate(void)
{
    egui_canvas_draw_image_rotate((const egui_image_t *)&probe_image_std, 0, 0, 30);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_text_rotate(void)
{
    egui_canvas_draw_text_rotate((const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT, "A", 0, 0, 30, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

EGUI_SIZE_PROBE_FUNC canvas_probe_rle_draw(void)
{
#if EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE
    egui_canvas_draw_image((const egui_image_t *)&probe_image_rle, 0, 0);
#endif
}

EGUI_SIZE_PROBE_FUNC canvas_probe_qoi_draw(void)
{
#if EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE
    egui_canvas_draw_image((const egui_image_t *)&probe_image_qoi, 0, 0);
#endif
}

static void canvas_probe_link_enabled_paths(void)
{
#if defined(EGUI_SIZE_PROBE_RECT_STROKE_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_rect_stroke();
    }
#endif

#if defined(EGUI_SIZE_PROBE_RECT_FILL_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_rect_fill();
    }
#endif

#if defined(EGUI_SIZE_PROBE_ROUND_RECT_STROKE_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_round_rect_stroke();
    }
#endif

#if defined(EGUI_SIZE_PROBE_ROUND_RECT_FILL_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_round_rect_fill();
    }
#endif

#if defined(EGUI_SIZE_PROBE_TRIANGLE_STROKE_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_triangle_stroke();
    }
#endif

#if defined(EGUI_SIZE_PROBE_TRIANGLE_FILL_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_triangle_fill();
    }
#endif

#if defined(EGUI_SIZE_PROBE_CIRCLE_BASIC_STROKE_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_circle_basic_stroke();
    }
#endif

#if defined(EGUI_SIZE_PROBE_CIRCLE_BASIC_FILL_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_circle_basic_fill();
    }
#endif

#if defined(EGUI_SIZE_PROBE_ARC_BASIC_STROKE_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_arc_basic_stroke();
    }
#endif

#if defined(EGUI_SIZE_PROBE_ARC_BASIC_FILL_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_arc_basic_fill();
    }
#endif

#if defined(EGUI_SIZE_PROBE_LINE_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_line();
    }
#endif

#if defined(EGUI_SIZE_PROBE_POLYLINE_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_polyline();
    }
#endif

#if defined(EGUI_SIZE_PROBE_GRADIENT_RECT_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_gradient_rect();
    }
#endif

#if defined(EGUI_SIZE_PROBE_GRADIENT_ROUND_RECT_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_gradient_round_rect();
    }
#endif

#if defined(EGUI_SIZE_PROBE_GRADIENT_CIRCLE_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_gradient_circle();
    }
#endif

#if defined(EGUI_SIZE_PROBE_MASK_CIRCLE_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_mask_circle();
    }
#endif

#if defined(EGUI_SIZE_PROBE_MASK_ROUND_RECT_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_mask_round_rect();
    }
#endif

#if defined(EGUI_SIZE_PROBE_MASK_IMAGE_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_mask_image();
    }
#endif

#if defined(EGUI_SIZE_PROBE_IMAGE_DRAW_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_image_draw();
    }
#endif

#if defined(EGUI_SIZE_PROBE_IMAGE_RESIZE_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_image_resize();
    }
#endif

#if defined(EGUI_SIZE_PROBE_IMAGE_TINT_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_image_tint();
    }
#endif

#if defined(EGUI_SIZE_PROBE_IMAGE_ROTATE_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_image_rotate();
    }
#endif

#if defined(EGUI_SIZE_PROBE_TEXT_ROTATE_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_text_rotate();
    }
#endif

#if defined(EGUI_SIZE_PROBE_RLE_DRAW_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_rle_draw();
    }
#endif

#if defined(EGUI_SIZE_PROBE_QOI_DRAW_PATH)
    if (g_canvas_size_probe_runtime_enable)
    {
        canvas_probe_qoi_draw();
    }
#endif
}

void uicode_create_ui(void)
{
    canvas_probe_link_enabled_paths();
}
