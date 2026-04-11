#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_test_performance.h"
#include "egui.h"

#include "core/egui_canvas_gradient.h"
#include "mask/egui_mask_gradient.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
#include "shadow/egui_shadow.h"
#endif

#include "mask/egui_mask_circle.h"
#include "mask/egui_mask_round_rectangle.h"
#include "mask/egui_mask_image.h"

#if EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE
#include "image/egui_image_qoi.h"
#endif

#if EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE
#include "image/egui_image_rle.h"
#endif

#include "anim/egui_animation_translate.h"
#include "anim/egui_animation_alpha.h"
#include "anim/egui_animation_scale_size.h"
#include "anim/egui_animation_set.h"
#include "anim/egui_interpolator_linear.h"

#include "app_egui_resource_generate.h"

#include "widget/egui_view_image.h"
#include "widget/egui_view_chart_line.h"
#include "widget/egui_view_chart_bar.h"
#include "widget/egui_view_chart_scatter.h"
#include "widget/egui_view_chart_pie.h"

#if EGUI_CONFIG_FUNCTION_IMAGE_FILE
#include "../HelloBasic/file_image/decoder_bmp_stream.h"
#include "../HelloBasic/file_image/decoder_stb.h"
#include "../HelloBasic/file_image/decoder_tjpgd_stream.h"
#include "../HelloBasic/file_image/file_image_stack.h"
#include "file_image_port_io.h"
#endif

#define PERF_WIDGET_BG_COLOR       EGUI_COLOR_HEX(0x0F172A)
#define PERF_WIDGET_AXIS_COLOR     EGUI_COLOR_HEX(0x94A3B8)
#define PERF_WIDGET_GRID_COLOR     EGUI_COLOR_HEX(0x334155)
#define PERF_WIDGET_TEXT_COLOR     EGUI_COLOR_HEX(0xE2E8F0)
#define PERF_FILE_IMAGE_BG_COLOR   EGUI_COLOR_HEX(0x111827)
#define PERF_CHART_LINE_SERIES_CNT 2u
#define PERF_CHART_BAR_SERIES_CNT  2u
#define PERF_CHART_SCATTER_SERIES_CNT 2u
#define PERF_CHART_LINE_POINT_COUNT    128u
#define PERF_CHART_BAR_POINT_COUNT     32u
#define PERF_CHART_SCATTER_POINT_COUNT 96u
#define PERF_CHART_PIE_SLICE_COUNT     16u

#if EGUI_CONFIG_FUNCTION_IMAGE_FILE
#define PERF_FILE_IMAGE_JPG_PATH "sd:sample_landscape.jpg"
#define PERF_FILE_IMAGE_PNG_PATH "lfs:sample_overlay.png"
#define PERF_FILE_IMAGE_BMP_PATH "flash:sample_badge.bmp"
#endif

static egui_region_t s_perf_widget_dirty_backup[EGUI_CONFIG_DIRTY_AREA_COUNT];
static uint8_t s_perf_chart_data_ready;

static egui_view_chart_line_t s_perf_chart_line_view;
static egui_view_chart_bar_t s_perf_chart_bar_view;
static egui_view_chart_scatter_t s_perf_chart_scatter_view;
static egui_view_chart_pie_t s_perf_chart_pie_view;

static uint8_t s_perf_chart_line_ready;
static uint8_t s_perf_chart_bar_ready;
static uint8_t s_perf_chart_scatter_ready;
static uint8_t s_perf_chart_pie_ready;
static uint8_t s_perf_chart_line_layout_ready;
static uint8_t s_perf_chart_bar_layout_ready;
static uint8_t s_perf_chart_scatter_layout_ready;
static uint8_t s_perf_chart_pie_layout_ready;

static egui_chart_point_t s_perf_chart_line_points_a[PERF_CHART_LINE_POINT_COUNT];
static egui_chart_point_t s_perf_chart_line_points_b[PERF_CHART_LINE_POINT_COUNT];
static egui_chart_point_t s_perf_chart_bar_points_a[PERF_CHART_BAR_POINT_COUNT];
static egui_chart_point_t s_perf_chart_bar_points_b[PERF_CHART_BAR_POINT_COUNT];
static egui_chart_point_t s_perf_chart_scatter_points_a[PERF_CHART_SCATTER_POINT_COUNT];
static egui_chart_point_t s_perf_chart_scatter_points_b[PERF_CHART_SCATTER_POINT_COUNT];
static egui_chart_pie_slice_t s_perf_chart_pie_slices[PERF_CHART_PIE_SLICE_COUNT];

static const egui_chart_series_t s_perf_chart_line_series[PERF_CHART_LINE_SERIES_CNT] = {
        {.points = s_perf_chart_line_points_a, .point_count = (uint8_t)PERF_CHART_LINE_POINT_COUNT, .color = EGUI_COLOR_MAKE(96, 165, 250), .name = "A"},
        {.points = s_perf_chart_line_points_b, .point_count = (uint8_t)PERF_CHART_LINE_POINT_COUNT, .color = EGUI_COLOR_MAKE(52, 211, 153), .name = "B"},
};
static const egui_chart_series_t s_perf_chart_bar_series[PERF_CHART_BAR_SERIES_CNT] = {
        {.points = s_perf_chart_bar_points_a, .point_count = (uint8_t)PERF_CHART_BAR_POINT_COUNT, .color = EGUI_COLOR_MAKE(251, 191, 36), .name = "A"},
        {.points = s_perf_chart_bar_points_b, .point_count = (uint8_t)PERF_CHART_BAR_POINT_COUNT, .color = EGUI_COLOR_MAKE(248, 113, 113), .name = "B"},
};
static const egui_chart_series_t s_perf_chart_scatter_series[PERF_CHART_SCATTER_SERIES_CNT] = {
        {.points = s_perf_chart_scatter_points_a, .point_count = (uint8_t)PERF_CHART_SCATTER_POINT_COUNT, .color = EGUI_COLOR_MAKE(196, 181, 253), .name = "A"},
        {.points = s_perf_chart_scatter_points_b, .point_count = (uint8_t)PERF_CHART_SCATTER_POINT_COUNT, .color = EGUI_COLOR_MAKE(125, 211, 252), .name = "B"},
};

#if EGUI_CONFIG_FUNCTION_IMAGE_FILE
static egui_view_image_t s_perf_file_image_view;
static egui_image_file_t s_perf_file_image;
static file_image_port_context_t s_perf_file_image_mount_ctx = {
        .root_prefix = "example/HelloPerformance/files/",
        .alt_root_prefix = "../example/HelloPerformance/files/",
};
static egui_image_file_io_t s_perf_file_image_mount_io;
static file_image_stack_state_t s_perf_file_image_stack_state;
static uint8_t s_perf_file_image_stack_ready;
static uint8_t s_perf_file_image_ready;
static uint8_t s_perf_file_image_layout_ready;
static int s_perf_file_image_scene_mode = -1;

static const file_image_mount_router_entry_t s_perf_file_image_mount_entries[] = {
        {"sd:", &s_perf_file_image_mount_io, 1},
        {"lfs:", &s_perf_file_image_mount_io, 1},
        {"flash:", &s_perf_file_image_mount_io, 1},
};
static const file_image_decoder_registry_config_t s_perf_file_image_decoder_config = {
        .bmp_stream = &g_file_image_bmp_stream_decoder,
        .jpeg_vendor = NULL,
#if EGUI_PORT == EGUI_PORT_TYPE_QEMU
        .jpeg_stream = NULL,
#else
        .jpeg_stream = &g_file_image_tjpgd_stream_decoder,
#endif
        .png_vendor = NULL,
        .generic_fallback = &g_file_image_stb_decoder,
        .clear_first = 1,
};
static const file_image_stack_config_t s_perf_file_image_stack_config = {
        .default_io = NULL,
        .mount_entries = s_perf_file_image_mount_entries,
        .mount_entry_count = sizeof(s_perf_file_image_mount_entries) / sizeof(s_perf_file_image_mount_entries[0]),
        .fallback_io = &s_perf_file_image_mount_io,
        .decoder_config = &s_perf_file_image_decoder_config,
};
#endif

// Large image tests (480px/240px) stay in the shipped benchmark set.

// Small image tests (40x40 tiled): always enabled, negligible flash cost.

/*
 * HelloPerformance asset policy:
 * - test_perf_* stays opaque RGB565 only.
 * - any scene that exercises alpha uses star_* assets.
 * This keeps transparent-image coverage representative and avoids carrying
 * redundant alpha variants for opaque benchmark art.
 */
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_0 egui_res_image_test_perf_240_rgb565##_0
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_1 egui_res_image_star_240_rgb565##_1
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_2 egui_res_image_star_240_rgb565##_2
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_4 egui_res_image_star_240_rgb565##_4
#define EGUI_TEST_PERFORMANCE_IMAGE_NAME_8 egui_res_image_star_240_rgb565##_8

#define EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_0 egui_res_image_test_perf_120_rgb565##_0
#define EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_1 egui_res_image_star_120_rgb565##_1
#define EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_2 egui_res_image_star_120_rgb565##_2
#define EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_4 egui_res_image_star_120_rgb565##_4
#define EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_8 egui_res_image_star_120_rgb565##_8

#define EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_0 egui_res_image_test_perf_40_rgb565##_0
#define EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_1 egui_res_image_star_40_rgb565##_1
#define EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_2 egui_res_image_star_40_rgb565##_2
#define EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_4 egui_res_image_star_40_rgb565##_4
#define EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_8 egui_res_image_star_40_rgb565##_8

#define EGUI_TEST_PERFORMANCE_STAR_RESIZE_IMAGE_NAME_1 egui_res_image_star_120_rgb565##_1
#define EGUI_TEST_PERFORMANCE_STAR_RESIZE_IMAGE_NAME_2 egui_res_image_star_120_rgb565##_2
#define EGUI_TEST_PERFORMANCE_STAR_RESIZE_IMAGE_NAME_4 egui_res_image_star_120_rgb565##_4
#define EGUI_TEST_PERFORMANCE_STAR_RESIZE_IMAGE_NAME_8 egui_res_image_star_120_rgb565##_8

#define EGUI_TEST_PERFORMANCE_IMAGE_COLOR_NAME        egui_res_image_star_pure_240_alpha_8
#define EGUI_TEST_PERFORMANCE_IMAGE_RESIZE_COLOR_NAME egui_res_image_star_pure_120_alpha_8

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
#define EGUI_TEST_PERFORMANCE_EXTERN_IMAGE_NAME_0        egui_res_image_test_perf_240_ext_rgb565_0_bin
#define EGUI_TEST_PERFORMANCE_EXTERN_IMAGE_NAME_1        egui_res_image_star_240_ext_rgb565_1_bin
#define EGUI_TEST_PERFORMANCE_EXTERN_IMAGE_NAME_2        egui_res_image_star_240_ext_rgb565_2_bin
#define EGUI_TEST_PERFORMANCE_EXTERN_IMAGE_NAME_4        egui_res_image_star_240_ext_rgb565_4_bin
#define EGUI_TEST_PERFORMANCE_EXTERN_IMAGE_NAME_8        egui_res_image_star_240_ext_rgb565_8_bin
#define EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_0 egui_res_image_test_perf_120_ext_rgb565_0_bin
#define EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_1 egui_res_image_star_120_ext_rgb565_1_bin
#define EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_2 egui_res_image_star_120_ext_rgb565_2_bin
#define EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_4 egui_res_image_star_120_ext_rgb565_4_bin
#define EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_8 egui_res_image_star_120_ext_rgb565_8_bin
#define EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_0  egui_res_image_test_perf_40_ext_rgb565_0_bin
#define EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_1  egui_res_image_star_40_ext_rgb565_1_bin
#define EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_2  egui_res_image_star_40_ext_rgb565_2_bin
#define EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_4  egui_res_image_star_40_ext_rgb565_4_bin
#define EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_8  egui_res_image_star_40_ext_rgb565_8_bin
#endif

uint16_t egui_view_test_performance_adjust_circle(uint16_t radius)
{
    return EGUI_MIN(radius, 300 - 2);
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
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_IMAGE_NAME_0;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_NAME_0, 0, 0);
}

static void egui_view_test_performance_test_image_565_1(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_IMAGE_NAME_1;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_NAME_1, 0, 0);
}

static void egui_view_test_performance_test_image_565_2(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_IMAGE_NAME_2;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_NAME_2, 0, 0);
}

static void egui_view_test_performance_test_image_565_4(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_IMAGE_NAME_4;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_NAME_4, 0, 0);
}

static void egui_view_test_performance_test_image_565_8(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_IMAGE_NAME_8;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_NAME_8, 0, 0);
}

static void egui_view_test_performance_test_image_resize_565(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_0;
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_0, 0, 0, self->region.size.width, self->region.size.height);
}

static void egui_view_test_performance_test_image_resize_565_1(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_1;
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_1, 0, 0, self->region.size.width, self->region.size.height);
}

static void egui_view_test_performance_test_image_resize_565_2(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_2;
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_2, 0, 0, self->region.size.width, self->region.size.height);
}

static void egui_view_test_performance_test_image_resize_565_4(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_4;
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_4, 0, 0, self->region.size.width, self->region.size.height);
}

static void egui_view_test_performance_test_image_resize_565_8(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_8;
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_8, 0, 0, self->region.size.width, self->region.size.height);
}

// ============================================================================
// Legacy star-image aliases
// Transparent image benchmarks now use star assets directly in the generic
// alpha scenes, so these helpers are kept only as compatibility aliases.
// ============================================================================

static void egui_view_test_performance_test_image_resize_star_565_1(egui_view_t *self)
{
    egui_view_test_performance_test_image_resize_565_1(self);
}

static void egui_view_test_performance_test_image_resize_star_565_2(egui_view_t *self)
{
    egui_view_test_performance_test_image_resize_565_2(self);
}

static void egui_view_test_performance_test_image_resize_star_565_4(egui_view_t *self)
{
    egui_view_test_performance_test_image_resize_565_4(self);
}

static void egui_view_test_performance_test_image_resize_star_565_8(egui_view_t *self)
{
    egui_view_test_performance_test_image_resize_565_8(self);
}

/* Multi-line text string for rotation test (~240x240 at 26pt font) */
static const char text_rect_str[] = "0123456789\n"
                                    "EFGHIJKLMN\n"
                                    "stuvwxyz01\n"
                                    "UVWXYZ1234\n"
                                    "6789abcdef\n"
                                    "KLMNOPQRSX\n"
                                    "YZ0123456";

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
    egui_canvas_draw_text_in_rect((egui_font_t *)&egui_res_font_montserrat_26_4, text_rect_str, &text_rect, EGUI_ALIGN_LEFT, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

static void egui_view_test_performance_draw_text_with_font(const egui_font_t *font)
{
    egui_canvas_draw_text((egui_font_t *)font, test_str, 0, 0, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

static void egui_view_test_performance_draw_text_rect_with_font(const egui_font_t *font)
{
    EGUI_REGION_DEFINE(text_rect, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, 200);
    egui_canvas_draw_text_in_rect((egui_font_t *)font, text_rect_str, &text_rect, EGUI_ALIGN_LEFT, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_internal_text(egui_view_t *self)
{
    egui_view_test_performance_draw_text_with_font((egui_font_t *)&egui_res_font_montserrat_perf_26_4);
}

static void egui_view_test_performance_test_internal_text_rect(egui_view_t *self)
{
    egui_view_test_performance_draw_text_rect_with_font((egui_font_t *)&egui_res_font_montserrat_perf_26_4);
}

static void egui_view_test_performance_test_internal_text_rle4(egui_view_t *self)
{
    egui_view_test_performance_draw_text_with_font((egui_font_t *)&egui_res_font_montserrat_perf_26_4_rle4);
}

static void egui_view_test_performance_test_internal_text_rect_rle4(egui_view_t *self)
{
    egui_view_test_performance_draw_text_rect_with_font((egui_font_t *)&egui_res_font_montserrat_perf_26_4_rle4);
}

static void egui_view_test_performance_test_internal_text_rle4_xor(egui_view_t *self)
{
    egui_view_test_performance_draw_text_with_font((egui_font_t *)&egui_res_font_montserrat_perf_26_4_rle4xor);
}

static void egui_view_test_performance_test_internal_text_rect_rle4_xor(egui_view_t *self)
{
    egui_view_test_performance_draw_text_rect_with_font((egui_font_t *)&egui_res_font_montserrat_perf_26_4_rle4xor);
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static void egui_view_test_performance_test_extern_text(egui_view_t *self)
{
    egui_view_test_performance_draw_text_with_font((egui_font_t *)&egui_res_font_montserrat_perf_26_4_bin);
}

static void egui_view_test_performance_test_extern_text_rect(egui_view_t *self)
{
    egui_view_test_performance_draw_text_rect_with_font((egui_font_t *)&egui_res_font_montserrat_perf_26_4_bin);
}

static void egui_view_test_performance_test_extern_text_rle4(egui_view_t *self)
{
    egui_view_test_performance_draw_text_with_font((egui_font_t *)&egui_res_font_montserrat_perf_26_4_rle4_bin);
}

static void egui_view_test_performance_test_extern_text_rect_rle4(egui_view_t *self)
{
    egui_view_test_performance_draw_text_rect_with_font((egui_font_t *)&egui_res_font_montserrat_perf_26_4_rle4_bin);
}

static void egui_view_test_performance_test_extern_text_rle4_xor(egui_view_t *self)
{
    egui_view_test_performance_draw_text_with_font((egui_font_t *)&egui_res_font_montserrat_perf_26_4_rle4xor_bin);
}

static void egui_view_test_performance_test_extern_text_rect_rle4_xor(egui_view_t *self)
{
    egui_view_test_performance_draw_text_rect_with_font((egui_font_t *)&egui_res_font_montserrat_perf_26_4_rle4xor_bin);
}
#endif

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

// ============================================================================
// Polygon tests
// ============================================================================

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

// ============================================================================
// Bezier tests
// ============================================================================

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

// ============================================================================
// HQ circle/arc tests
// ============================================================================

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

// ============================================================================
// HQ line test
// ============================================================================

static void egui_view_test_performance_test_line_hq(egui_view_t *self)
{
    egui_dim_t line_len = EGUI_MIN(self->region.size.width, self->region.size.height);
    egui_canvas_draw_line_hq(0, 0, line_len, line_len, 20, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

// ============================================================================
// Image color tint tests
// ============================================================================

static void egui_view_test_performance_test_image_color(egui_view_t *self)
{
    egui_canvas_draw_image_color((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_COLOR_NAME, 0, 0, EGUI_COLOR_MAKE(0xFF, 0x80, 0x00), EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_image_resize_color(egui_view_t *self)
{
    egui_canvas_draw_image_resize_color((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_RESIZE_COLOR_NAME, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT,
                                        EGUI_COLOR_MAKE(0xFF, 0x80, 0x00), EGUI_ALPHA_100);
}

// ============================================================================
// Gradient tests
// ============================================================================

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

/* Angular gradient (R->G->B->R conic sweep) */
static const egui_gradient_stop_t perf_angular_stops[] = {
        {.position = 0, .color = EGUI_COLOR_MAKE(0xFF, 0x00, 0x00)},
        {.position = 85, .color = EGUI_COLOR_MAKE(0x00, 0xFF, 0x00)},
        {.position = 170, .color = EGUI_COLOR_MAKE(0x00, 0x00, 0xFF)},
        {.position = 255, .color = EGUI_COLOR_MAKE(0xFF, 0x00, 0x00)},
};
static const egui_gradient_t perf_gradient_angular_tmpl = {
        .type = EGUI_GRADIENT_TYPE_ANGULAR,
        .stop_count = 4,
        .alpha = EGUI_ALPHA_100,
        .stops = perf_angular_stops,
        .center_x = 0,
        .center_y = 0,
        .radius = 0,
};

/* 3-stop multi-color linear gradient (R->G->B) */
static const egui_gradient_stop_t perf_multi_stops[] = {
        {.position = 0, .color = EGUI_COLOR_MAKE(0xFF, 0x00, 0x00)},
        {.position = 128, .color = EGUI_COLOR_MAKE(0x00, 0xFF, 0x00)},
        {.position = 255, .color = EGUI_COLOR_MAKE(0x00, 0x00, 0xFF)},
};
static const egui_gradient_t perf_gradient_multi_stop = {
        .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
        .stop_count = 3,
        .alpha = EGUI_ALPHA_100,
        .stops = perf_multi_stops,
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

static void egui_view_test_performance_test_gradient_arc_ring(egui_view_t *self)
{
    egui_dim_t cx = EGUI_CONFIG_SCEEN_WIDTH / 2;
    egui_dim_t cy = EGUI_CONFIG_SCEEN_HEIGHT / 2;
    egui_dim_t outer_r = egui_view_test_performance_adjust_circle((EGUI_MIN(EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT) / 2) - 4);
    egui_dim_t inner_r = outer_r - 20;
    egui_canvas_draw_arc_ring_fill_gradient(cx, cy, outer_r, inner_r, -90, 270, &perf_gradient_v);
}

static void egui_view_test_performance_test_gradient_arc_ring_round_cap(egui_view_t *self)
{
    egui_dim_t cx = EGUI_CONFIG_SCEEN_WIDTH / 2;
    egui_dim_t cy = EGUI_CONFIG_SCEEN_HEIGHT / 2;
    egui_dim_t outer_r = egui_view_test_performance_adjust_circle((EGUI_MIN(EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT) / 2) - 4);
    egui_dim_t inner_r = outer_r - 20;
    egui_canvas_draw_arc_ring_fill_gradient_round_cap(cx, cy, outer_r, inner_r, -90, 270, &perf_gradient_v, EGUI_ARC_CAP_BOTH);
}

static void egui_view_test_performance_test_gradient_radial(egui_view_t *self)
{
    egui_dim_t cx = EGUI_CONFIG_SCEEN_WIDTH / 2;
    egui_dim_t cy = EGUI_CONFIG_SCEEN_HEIGHT / 2;
    egui_gradient_t grad = perf_gradient_radial;
    grad.center_x = cx;
    grad.center_y = cy;
    grad.radius = EGUI_MIN(cx, cy);
    egui_canvas_draw_rectangle_fill_gradient(0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, &grad);
}

static void egui_view_test_performance_test_gradient_angular(egui_view_t *self)
{
    egui_gradient_t grad = perf_gradient_angular_tmpl;
    grad.center_x = EGUI_CONFIG_SCEEN_WIDTH / 2;
    grad.center_y = EGUI_CONFIG_SCEEN_HEIGHT / 2;
    egui_canvas_draw_rectangle_fill_gradient(0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, &grad);
}

static void egui_view_test_performance_test_gradient_round_rect_ring(egui_view_t *self)
{
    egui_dim_t margin = 20;
    egui_dim_t stroke_w = 20;
    egui_dim_t radius = 30;
    egui_canvas_draw_round_rectangle_ring_fill_gradient(margin, margin, EGUI_CONFIG_SCEEN_WIDTH - margin * 2, EGUI_CONFIG_SCEEN_HEIGHT - margin * 2, stroke_w,
                                                        radius, &perf_gradient_v);
}

static void egui_view_test_performance_test_gradient_line_capsule(egui_view_t *self)
{
    egui_dim_t stroke_w = 20;
    egui_canvas_draw_line_capsule_fill_gradient(stroke_w, EGUI_CONFIG_SCEEN_HEIGHT / 2, EGUI_CONFIG_SCEEN_WIDTH - stroke_w, EGUI_CONFIG_SCEEN_HEIGHT / 2,
                                                stroke_w, &perf_gradient_v);
}

static void egui_view_test_performance_test_gradient_multi_stop(egui_view_t *self)
{
    egui_canvas_draw_rectangle_fill_gradient(0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, &perf_gradient_multi_stop);
}

static void egui_view_test_performance_test_gradient_round_rect_corners(egui_view_t *self)
{
    egui_dim_t radius = 40;
    egui_canvas_draw_round_rectangle_corners_fill_gradient(0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, radius, 0, 0, radius, &perf_gradient_v);
}

static void egui_view_test_performance_test_image_gradient_overlay(egui_view_t *self)
{
    egui_canvas_draw_image_gradient_overlay((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_NAME_0, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT,
                                            &perf_gradient_v, EGUI_ALPHA_60);
}

// ============================================================================
// Shadow tests
// ============================================================================

#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
static void egui_view_test_performance_test_shadow(egui_view_t *self)
{
    egui_canvas_draw_rectangle_fill(0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, EGUI_COLOR_LIGHT_GREY, EGUI_ALPHA_100);
    EGUI_SHADOW_PARAM_INIT(shadow, 20, 5, 5, EGUI_COLOR_BLACK, EGUI_ALPHA_60);
    egui_dim_t margin = 40;
    EGUI_REGION_DEFINE(region, margin, margin, EGUI_CONFIG_SCEEN_WIDTH - margin * 2, EGUI_CONFIG_SCEEN_HEIGHT - margin * 2);
    egui_shadow_draw(&shadow, &region);
    egui_canvas_draw_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_shadow_round(egui_view_t *self)
{
    egui_canvas_draw_rectangle_fill(0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, EGUI_COLOR_LIGHT_GREY, EGUI_ALPHA_100);
    EGUI_SHADOW_PARAM_INIT_ROUND(shadow, 20, 5, 5, EGUI_COLOR_BLACK, EGUI_ALPHA_60, 30);
    egui_dim_t margin = 40;
    EGUI_REGION_DEFINE(region, margin, margin, EGUI_CONFIG_SCEEN_WIDTH - margin * 2, EGUI_CONFIG_SCEEN_HEIGHT - margin * 2);
    egui_shadow_draw(&shadow, &region);
    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, 30, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}
#endif

// ============================================================================
// Mask performance tests
// ============================================================================

// Test image resources (perf-local, 120x120)
extern const egui_image_std_t EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_0;
extern const egui_image_std_t EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_8;
extern const egui_image_std_t EGUI_TEST_PERFORMANCE_STAR_RESIZE_IMAGE_NAME_8;

// Multi-size test dimensions
#define PERF_SIZE_QUARTER (EGUI_CONFIG_SCEEN_WIDTH / 2)
#define PERF_SIZE_FULL    EGUI_CONFIG_SCEEN_WIDTH
#define PERF_SIZE_DOUBLE  (EGUI_CONFIG_SCEEN_WIDTH * 2)

#define MASK_TEST_SIZE   PERF_SIZE_FULL
#define MASK_TEST_RADIUS (MASK_TEST_SIZE / 16)

static void prepare_perf_round_rect_mask(egui_mask_round_rectangle_t *mask, uint16_t size, uint16_t radius)
{
    egui_mask_round_rectangle_init((egui_mask_t *)mask);
    egui_mask_set_position((egui_mask_t *)mask, 0, 0);
    egui_mask_set_size((egui_mask_t *)mask, size, size);
    egui_mask_round_rectangle_set_radius((egui_mask_t *)mask, radius);
}

static void prepare_perf_circle_mask(egui_mask_circle_t *mask, uint16_t size)
{
    egui_mask_circle_init((egui_mask_t *)mask);
    egui_mask_set_position((egui_mask_t *)mask, 0, 0);
    egui_mask_set_size((egui_mask_t *)mask, size, size);
}

static void prepare_perf_image_mask(egui_mask_image_t *mask)
{
    egui_mask_image_init((egui_mask_t *)mask);
    egui_mask_set_position((egui_mask_t *)mask, 0, 0);
    egui_mask_set_size((egui_mask_t *)mask, PERF_SIZE_QUARTER, PERF_SIZE_QUARTER);
    egui_mask_image_set_image((egui_mask_t *)mask, (egui_image_t *)&EGUI_TEST_PERFORMANCE_STAR_RESIZE_IMAGE_NAME_8);
}

static void prepare_perf_gradient_mask(egui_mask_gradient_t *mask)
{
    egui_mask_gradient_init((egui_mask_t *)mask);
    egui_mask_set_position((egui_mask_t *)mask, 0, 0);
    egui_mask_set_size((egui_mask_t *)mask, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_mask_gradient_set_gradient((egui_mask_t *)mask, &perf_gradient_v);
    egui_mask_gradient_set_overlay_alpha((egui_mask_t *)mask, EGUI_ALPHA_100);
}

static void ensure_perf_masks_initialized(void)
{
    // Stack-local perf masks are prepared at each draw site.
}

static void perf_prepare_widget_layout(egui_view_t *view, const egui_view_t *host, uint8_t *layout_ready)
{
    egui_region_t *dirty_regions;

    if (view == NULL || view->api == NULL || layout_ready == NULL || *layout_ready)
    {
        return;
    }

    dirty_regions = egui_core_get_region_dirty_arr();
    memcpy(s_perf_widget_dirty_backup, dirty_regions, sizeof(s_perf_widget_dirty_backup));

    if (view->parent == NULL)
    {
        if (host != NULL)
        {
            view->region.location.x = (egui_dim_t)(view->region.location.x + host->region.location.x);
            view->region.location.y = (egui_dim_t)(view->region.location.y + host->region.location.y);
        }
        egui_view_request_layout(view);
    }

    view->api->calculate_layout(view);
    memcpy(dirty_regions, s_perf_widget_dirty_backup, sizeof(s_perf_widget_dirty_backup));
    *layout_ready = 1U;
}

static void perf_draw_widget(egui_view_t *view)
{
    if (view == NULL || view->api == NULL)
    {
        return;
    }

    view->api->draw(view);
}

static void perf_configure_axis_chart_view(egui_view_t *view)
{
    egui_view_set_position(view, 0, 0);
    egui_view_set_size(view, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_chart_axis_set_colors(view, PERF_WIDGET_BG_COLOR, PERF_WIDGET_AXIS_COLOR, PERF_WIDGET_GRID_COLOR, PERF_WIDGET_TEXT_COLOR);
}

static void perf_ensure_chart_data_ready(void)
{
    static const uint8_t pie_colors[PERF_CHART_PIE_SLICE_COUNT][3] = {
            {59, 130, 246},
            {16, 185, 129},
            {245, 158, 11},
            {239, 68, 68},
            {168, 85, 247},
            {14, 165, 233},
            {34, 197, 94},
            {251, 146, 60},
            {244, 114, 182},
            {163, 230, 53},
            {129, 140, 248},
            {45, 212, 191},
            {250, 204, 21},
            {248, 113, 113},
            {192, 132, 252},
            {56, 189, 248},
    };
    uint16_t i;

    if (s_perf_chart_data_ready)
    {
        return;
    }

    for (i = 0; i < PERF_CHART_LINE_POINT_COUNT; i++)
    {
        s_perf_chart_line_points_a[i].x = (int16_t)i;
        s_perf_chart_line_points_a[i].y = (int16_t)(8 + ((i * 37u + (i % 9u) * 5u) % 86u));
        s_perf_chart_line_points_b[i].x = (int16_t)i;
        s_perf_chart_line_points_b[i].y = (int16_t)(10 + ((i * 53u + (i % 7u) * 3u) % 80u));
    }

    for (i = 0; i < PERF_CHART_BAR_POINT_COUNT; i++)
    {
        s_perf_chart_bar_points_a[i].x = (int16_t)i;
        s_perf_chart_bar_points_a[i].y = (int16_t)(12 + ((i * 23u + (i % 4u) * 9u) % 78u));
        s_perf_chart_bar_points_b[i].x = (int16_t)i;
        s_perf_chart_bar_points_b[i].y = (int16_t)(10 + ((i * 31u + (i % 5u) * 7u) % 82u));
    }

    for (i = 0; i < PERF_CHART_SCATTER_POINT_COUNT; i++)
    {
        s_perf_chart_scatter_points_a[i].x = (int16_t)((i * 29u + (i % 6u) * 7u) % PERF_CHART_LINE_POINT_COUNT);
        s_perf_chart_scatter_points_a[i].y = (int16_t)(6 + ((i * 41u + (i % 5u) * 11u) % 90u));
        s_perf_chart_scatter_points_b[i].x = (int16_t)((i * 17u + (i % 8u) * 13u) % PERF_CHART_LINE_POINT_COUNT);
        s_perf_chart_scatter_points_b[i].y = (int16_t)(4 + ((i * 47u + (i % 7u) * 9u) % 92u));
    }

    for (i = 0; i < PERF_CHART_PIE_SLICE_COUNT; i++)
    {
        s_perf_chart_pie_slices[i].value = (uint16_t)(8 + (i % 5u) * 3u + (i % 3u));
        s_perf_chart_pie_slices[i].color = EGUI_COLOR_MAKE(pie_colors[i][0], pie_colors[i][1], pie_colors[i][2]);
        s_perf_chart_pie_slices[i].name = NULL;
    }

    s_perf_chart_data_ready = 1U;
}

static void perf_init_chart_line_view(void)
{
    if (s_perf_chart_line_ready)
    {
        return;
    }

    perf_ensure_chart_data_ready();
    egui_view_chart_line_init(EGUI_VIEW_OF(&s_perf_chart_line_view));
    perf_configure_axis_chart_view(EGUI_VIEW_OF(&s_perf_chart_line_view));
    egui_view_chart_line_set_axis_x(EGUI_VIEW_OF(&s_perf_chart_line_view), 0, (int16_t)(PERF_CHART_LINE_POINT_COUNT - 1), 16);
    egui_view_chart_line_set_axis_y(EGUI_VIEW_OF(&s_perf_chart_line_view), 0, 100, 20);
    egui_view_chart_line_set_series(EGUI_VIEW_OF(&s_perf_chart_line_view), s_perf_chart_line_series, (uint8_t)PERF_CHART_LINE_SERIES_CNT);
    egui_view_chart_line_set_legend_pos(EGUI_VIEW_OF(&s_perf_chart_line_view), EGUI_CHART_LEGEND_BOTTOM);
    egui_view_chart_line_set_line_width(EGUI_VIEW_OF(&s_perf_chart_line_view), 1);
    egui_view_chart_line_set_point_radius(EGUI_VIEW_OF(&s_perf_chart_line_view), 1);
    s_perf_chart_line_ready = 1U;
}

static void perf_init_chart_bar_view(void)
{
    if (s_perf_chart_bar_ready)
    {
        return;
    }

    perf_ensure_chart_data_ready();
    egui_view_chart_bar_init(EGUI_VIEW_OF(&s_perf_chart_bar_view));
    perf_configure_axis_chart_view(EGUI_VIEW_OF(&s_perf_chart_bar_view));
    egui_view_chart_bar_set_axis_x(EGUI_VIEW_OF(&s_perf_chart_bar_view), 0, (int16_t)(PERF_CHART_BAR_POINT_COUNT - 1), 4);
    egui_view_chart_bar_set_axis_y(EGUI_VIEW_OF(&s_perf_chart_bar_view), 0, 100, 20);
    egui_view_chart_bar_set_series(EGUI_VIEW_OF(&s_perf_chart_bar_view), s_perf_chart_bar_series, (uint8_t)PERF_CHART_BAR_SERIES_CNT);
    egui_view_chart_bar_set_legend_pos(EGUI_VIEW_OF(&s_perf_chart_bar_view), EGUI_CHART_LEGEND_BOTTOM);
    egui_view_chart_bar_set_bar_gap(EGUI_VIEW_OF(&s_perf_chart_bar_view), 1);
    s_perf_chart_bar_ready = 1U;
}

static void perf_init_chart_scatter_view(void)
{
    if (s_perf_chart_scatter_ready)
    {
        return;
    }

    perf_ensure_chart_data_ready();
    egui_view_chart_scatter_init(EGUI_VIEW_OF(&s_perf_chart_scatter_view));
    perf_configure_axis_chart_view(EGUI_VIEW_OF(&s_perf_chart_scatter_view));
    egui_view_chart_scatter_set_axis_x(EGUI_VIEW_OF(&s_perf_chart_scatter_view), 0, (int16_t)(PERF_CHART_LINE_POINT_COUNT - 1), 16);
    egui_view_chart_scatter_set_axis_y(EGUI_VIEW_OF(&s_perf_chart_scatter_view), 0, 100, 20);
    egui_view_chart_scatter_set_series(EGUI_VIEW_OF(&s_perf_chart_scatter_view), s_perf_chart_scatter_series, (uint8_t)PERF_CHART_SCATTER_SERIES_CNT);
    egui_view_chart_scatter_set_legend_pos(EGUI_VIEW_OF(&s_perf_chart_scatter_view), EGUI_CHART_LEGEND_BOTTOM);
    egui_view_chart_scatter_set_point_radius(EGUI_VIEW_OF(&s_perf_chart_scatter_view), 2);
    s_perf_chart_scatter_ready = 1U;
}

static void perf_init_chart_pie_view(void)
{
    if (s_perf_chart_pie_ready)
    {
        return;
    }

    perf_ensure_chart_data_ready();
    egui_view_chart_pie_init(EGUI_VIEW_OF(&s_perf_chart_pie_view));
    egui_view_set_position(EGUI_VIEW_OF(&s_perf_chart_pie_view), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&s_perf_chart_pie_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_chart_pie_set_slices(EGUI_VIEW_OF(&s_perf_chart_pie_view), s_perf_chart_pie_slices, (uint8_t)PERF_CHART_PIE_SLICE_COUNT);
    egui_view_chart_pie_set_legend_pos(EGUI_VIEW_OF(&s_perf_chart_pie_view), EGUI_CHART_LEGEND_NONE);
    egui_view_chart_pie_set_colors(EGUI_VIEW_OF(&s_perf_chart_pie_view), PERF_WIDGET_BG_COLOR, PERF_WIDGET_TEXT_COLOR);
    s_perf_chart_pie_ready = 1U;
}

#if EGUI_CONFIG_FUNCTION_IMAGE_FILE
static const char *perf_get_file_image_path(int test_mode)
{
    switch (test_mode)
    {
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_FILE_IMAGE_JPG:
        return PERF_FILE_IMAGE_JPG_PATH;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_FILE_IMAGE_PNG:
        return PERF_FILE_IMAGE_PNG_PATH;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_FILE_IMAGE_BMP:
        return PERF_FILE_IMAGE_BMP_PATH;
    default:
        return NULL;
    }
}

static int perf_init_file_image_stack(void)
{
    if (s_perf_file_image_stack_ready)
    {
        return 1;
    }

    file_image_port_io_init(&s_perf_file_image_mount_io, &s_perf_file_image_mount_ctx);
    if (!file_image_stack_apply(&s_perf_file_image_stack_state, &s_perf_file_image_stack_config))
    {
        return 0;
    }

    s_perf_file_image_stack_ready = 1U;
    return 1;
}

static void perf_prepare_file_image_scene(int test_mode)
{
    const char *path = perf_get_file_image_path(test_mode);

    if (path == NULL || !perf_init_file_image_stack())
    {
        return;
    }

    if (!s_perf_file_image_ready)
    {
        egui_image_file_init(&s_perf_file_image);
        egui_view_image_init(EGUI_VIEW_OF(&s_perf_file_image_view));
        egui_view_set_position(EGUI_VIEW_OF(&s_perf_file_image_view), 0, 0);
        egui_view_set_size(EGUI_VIEW_OF(&s_perf_file_image_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
        egui_view_image_set_image(EGUI_VIEW_OF(&s_perf_file_image_view), (egui_image_t *)&s_perf_file_image);
        egui_view_image_set_image_type(EGUI_VIEW_OF(&s_perf_file_image_view), EGUI_VIEW_IMAGE_TYPE_RESIZE);
        s_perf_file_image_ready = 1U;
    }

    if (s_perf_file_image_scene_mode != test_mode)
    {
        egui_image_file_set_path(&s_perf_file_image, path);
        egui_image_file_reload(&s_perf_file_image);
        s_perf_file_image_scene_mode = test_mode;
    }
}
#endif

static void perf_draw_chart_line_scene(egui_view_t *self)
{
    perf_init_chart_line_view();
    perf_prepare_widget_layout(EGUI_VIEW_OF(&s_perf_chart_line_view), self, &s_perf_chart_line_layout_ready);
    perf_draw_widget(EGUI_VIEW_OF(&s_perf_chart_line_view));
}

static void perf_draw_chart_bar_scene(egui_view_t *self)
{
    perf_init_chart_bar_view();
    perf_prepare_widget_layout(EGUI_VIEW_OF(&s_perf_chart_bar_view), self, &s_perf_chart_bar_layout_ready);
    perf_draw_widget(EGUI_VIEW_OF(&s_perf_chart_bar_view));
}

static void perf_draw_chart_scatter_scene(egui_view_t *self)
{
    perf_init_chart_scatter_view();
    perf_prepare_widget_layout(EGUI_VIEW_OF(&s_perf_chart_scatter_view), self, &s_perf_chart_scatter_layout_ready);
    perf_draw_widget(EGUI_VIEW_OF(&s_perf_chart_scatter_view));
}

static void perf_draw_chart_pie_scene(egui_view_t *self)
{
    perf_init_chart_pie_view();
    perf_prepare_widget_layout(EGUI_VIEW_OF(&s_perf_chart_pie_view), self, &s_perf_chart_pie_layout_ready);
    perf_draw_widget(EGUI_VIEW_OF(&s_perf_chart_pie_view));
}

#if EGUI_CONFIG_FUNCTION_IMAGE_FILE
static void perf_draw_file_image_scene(egui_view_t *self, int test_mode)
{
    perf_prepare_file_image_scene(test_mode);
    egui_canvas_draw_rectangle_fill(0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, PERF_FILE_IMAGE_BG_COLOR, EGUI_ALPHA_100);
    if (s_perf_file_image_ready)
    {
        perf_prepare_widget_layout(EGUI_VIEW_OF(&s_perf_file_image_view), self, &s_perf_file_image_layout_ready);
        perf_draw_widget(EGUI_VIEW_OF(&s_perf_file_image_view));
    }
}
#endif

static void set_perf_round_rect_mask(egui_mask_round_rectangle_t *mask, uint16_t size, uint16_t radius)
{
    prepare_perf_round_rect_mask(mask, size, radius);
    egui_canvas_set_mask((egui_mask_t *)mask);
}

static void set_perf_circle_mask(egui_mask_circle_t *mask, uint16_t size)
{
    prepare_perf_circle_mask(mask, size);
    egui_canvas_set_mask((egui_mask_t *)mask);
}

static void set_perf_image_mask(egui_mask_image_t *mask)
{
    prepare_perf_image_mask(mask);
    egui_canvas_set_mask((egui_mask_t *)mask);
}

static void set_perf_gradient_mask(egui_mask_gradient_t *mask)
{
    prepare_perf_gradient_mask(mask);
    egui_canvas_set_mask((egui_mask_t *)mask);
}

static void egui_view_test_performance_test_mask_rect_fill_no_mask(egui_view_t *self)
{
    egui_canvas_draw_rectangle_fill(0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_mask_rect_fill_round_rect(egui_view_t *self)
{
    egui_mask_round_rectangle_t mask;
    set_perf_round_rect_mask(&mask, MASK_TEST_SIZE, MASK_TEST_RADIUS);
    egui_canvas_draw_rectangle_fill(0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_rect_fill_circle(egui_view_t *self)
{
    egui_mask_circle_t mask;
    set_perf_circle_mask(&mask, MASK_TEST_SIZE);
    egui_canvas_draw_rectangle_fill(0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_rect_fill_image(egui_view_t *self)
{
    egui_mask_image_t mask;
    set_perf_image_mask(&mask);
    egui_canvas_draw_rectangle_fill(0, 0, PERF_SIZE_QUARTER, PERF_SIZE_QUARTER, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_no_mask(egui_view_t *self)
{
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_8, 0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE);
}

static void egui_view_test_performance_test_mask_image_round_rect(egui_view_t *self)
{
    egui_mask_round_rectangle_t mask;
    set_perf_round_rect_mask(&mask, MASK_TEST_SIZE, MASK_TEST_RADIUS);
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_8, 0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_circle(egui_view_t *self)
{
    egui_mask_circle_t mask;
    set_perf_circle_mask(&mask, MASK_TEST_SIZE);
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_8, 0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_image(egui_view_t *self)
{
    egui_mask_image_t mask;
    set_perf_image_mask(&mask);
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_8, 0, 0, PERF_SIZE_QUARTER, PERF_SIZE_QUARTER);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_test_perf_no_mask(egui_view_t *self)
{
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_0, 0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE);
}

static void egui_view_test_performance_test_mask_image_test_perf_round_rect(egui_view_t *self)
{
    egui_mask_round_rectangle_t mask;
    set_perf_round_rect_mask(&mask, MASK_TEST_SIZE, MASK_TEST_RADIUS);
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_0, 0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_test_perf_circle(egui_view_t *self)
{
    egui_mask_circle_t mask;
    set_perf_circle_mask(&mask, MASK_TEST_SIZE);
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_0, 0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_test_perf_image(egui_view_t *self)
{
    egui_mask_image_t mask;
    set_perf_image_mask(&mask);
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_0, 0, 0, PERF_SIZE_QUARTER, PERF_SIZE_QUARTER);
    egui_canvas_clear_mask();
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static void egui_view_test_performance_test_extern_mask_image_no_mask(egui_view_t *self)
{
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_8, 0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE);
}

static void egui_view_test_performance_test_extern_mask_image_round_rect(egui_view_t *self)
{
    egui_mask_round_rectangle_t mask;
    set_perf_round_rect_mask(&mask, MASK_TEST_SIZE, MASK_TEST_RADIUS);
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_8, 0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_extern_mask_image_circle(egui_view_t *self)
{
    egui_mask_circle_t mask;
    set_perf_circle_mask(&mask, MASK_TEST_SIZE);
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_8, 0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_extern_mask_image_image(egui_view_t *self)
{
    egui_mask_image_t mask;
    set_perf_image_mask(&mask);
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_8, 0, 0, PERF_SIZE_QUARTER, PERF_SIZE_QUARTER);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_extern_mask_image_test_perf_no_mask(egui_view_t *self)
{
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_0, 0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE);
}

static void egui_view_test_performance_test_extern_mask_image_test_perf_round_rect(egui_view_t *self)
{
    egui_mask_round_rectangle_t mask;
    set_perf_round_rect_mask(&mask, MASK_TEST_SIZE, MASK_TEST_RADIUS);
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_0, 0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_extern_mask_image_test_perf_circle(egui_view_t *self)
{
    egui_mask_circle_t mask;
    set_perf_circle_mask(&mask, MASK_TEST_SIZE);
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_0, 0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_extern_mask_image_test_perf_image(egui_view_t *self)
{
    egui_mask_image_t mask;
    set_perf_image_mask(&mask);
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_0, 0, 0, PERF_SIZE_QUARTER, PERF_SIZE_QUARTER);
    egui_canvas_clear_mask();
}
#endif

static void egui_view_test_performance_test_mask_round_rect_fill_no_mask(egui_view_t *self)
{
    egui_canvas_draw_round_rectangle_fill(0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE, MASK_TEST_RADIUS, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_mask_round_rect_fill_with_mask(egui_view_t *self)
{
    egui_mask_round_rectangle_t mask;
    set_perf_round_rect_mask(&mask, MASK_TEST_SIZE, MASK_TEST_RADIUS);
    egui_canvas_draw_rectangle_fill(0, 0, MASK_TEST_SIZE, MASK_TEST_SIZE, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_clear_mask();
}

// ============================================================================
// Animation performance tests
// ============================================================================

#define ANIM_TEST_SIZE 60

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(anim_perf_bg_param, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(anim_perf_bg_params, &anim_perf_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(anim_perf_bg, &anim_perf_bg_params);

static void init_anim_perf_context(egui_view_t *view, egui_interpolator_linear_t *interp)
{
    egui_view_init(EGUI_VIEW_OF(view));
    egui_view_set_position(EGUI_VIEW_OF(view), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(view), ANIM_TEST_SIZE, ANIM_TEST_SIZE);
    egui_view_set_background(EGUI_VIEW_OF(view), EGUI_BG_OF(&anim_perf_bg));
    egui_interpolator_linear_init((egui_interpolator_t *)interp);
}

static void egui_view_test_performance_test_animation_translate(egui_view_t *self)
{
    egui_view_t anim_perf_view;
    egui_interpolator_linear_t anim_perf_interp;
    egui_animation_translate_t anim;
    EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(params, 0, 0, 0, EGUI_CONFIG_SCEEN_HEIGHT - ANIM_TEST_SIZE);

    init_anim_perf_context(&anim_perf_view, &anim_perf_interp);
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
    egui_view_t anim_perf_view;
    egui_interpolator_linear_t anim_perf_interp;
    egui_animation_alpha_t anim;
    EGUI_ANIMATION_ALPHA_PARAMS_INIT(params, EGUI_ALPHA_100, EGUI_ALPHA_0);

    init_anim_perf_context(&anim_perf_view, &anim_perf_interp);
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
    egui_view_t anim_perf_view;
    egui_interpolator_linear_t anim_perf_interp;
    egui_animation_scale_size_t anim;
    EGUI_ANIMATION_SCALE_SIZE_PARAMS_INIT(params, EGUI_FLOAT_VALUE(0.5f), EGUI_FLOAT_VALUE(1.5f));

    init_anim_perf_context(&anim_perf_view, &anim_perf_interp);
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
    egui_view_t anim_perf_view;
    egui_interpolator_linear_t anim_perf_interp;
    egui_animation_translate_t anim_tr;
    egui_animation_alpha_t anim_al;
    egui_animation_set_t anim_set;
    EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(tr_params, 0, 0, 0, EGUI_CONFIG_SCEEN_HEIGHT - ANIM_TEST_SIZE);
    EGUI_ANIMATION_ALPHA_PARAMS_INIT(al_params, EGUI_ALPHA_100, EGUI_ALPHA_0);

    init_anim_perf_context(&anim_perf_view, &anim_perf_interp);
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
// Multi-size image tests
// ============================================================================

static void egui_view_test_performance_test_image_565_quarter(egui_view_t *self)
{
    // Full-size image drawn at screen center �?only 1/4 visible, tests clipping path
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_IMAGE_NAME_0;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_NAME_0, PERF_SIZE_QUARTER, PERF_SIZE_QUARTER);
}

static void egui_view_test_performance_test_image_565_double(egui_view_t *self)
{
    extern const egui_image_std_t egui_res_image_test_perf_480_rgb565_0;
    egui_canvas_draw_image((egui_image_t *)&egui_res_image_test_perf_480_rgb565_0, 0, 0);
}

static void egui_view_test_performance_test_image_565_8_quarter(egui_view_t *self)
{
    // Full-size image drawn at screen center �?only 1/4 visible, tests clipping path
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_IMAGE_NAME_8;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERFORMANCE_IMAGE_NAME_8, PERF_SIZE_QUARTER, PERF_SIZE_QUARTER);
}

static void egui_view_test_performance_test_image_565_8_double(egui_view_t *self)
{
    /*
     * Disabled to avoid carrying a dedicated 480x480 alpha asset in flash.
     * Transparent direct-draw coverage comes from the 240x240 star variants.
     */
    (void)self;
}

// ============================================================================
// Image transform (rotation) tests
// ============================================================================

static void egui_view_test_performance_test_image_rotate_565(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_0;
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    egui_canvas_draw_image_transform((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_0, cx, cy, 45, 256);
}

static void egui_view_test_performance_test_image_rotate_565_resize(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_0;
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    int16_t scale = (int16_t)((int32_t)self->region.size.width * 256 / 240);
    egui_canvas_draw_image_transform((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_0, cx, cy, 45, scale);
}

static void egui_view_test_performance_test_image_rotate_565_quarter(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_0;
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    egui_canvas_draw_image_transform((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_0, cx, cy, 45, 128);
}

static void egui_view_test_performance_test_image_rotate_565_1(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_1;
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    egui_canvas_draw_image_transform((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_1, cx, cy, 45, 256);
}

static void egui_view_test_performance_test_image_rotate_565_2(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_2;
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    egui_canvas_draw_image_transform((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_2, cx, cy, 45, 256);
}

static void egui_view_test_performance_test_image_rotate_565_4(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_4;
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    egui_canvas_draw_image_transform((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_4, cx, cy, 45, 256);
}

static void egui_view_test_performance_test_image_rotate_565_8(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_8;
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    egui_canvas_draw_image_transform((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_8, cx, cy, 45, 256);
}

static void egui_view_test_performance_test_image_rotate_565_double(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_0;
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    int16_t scale = (int16_t)((int32_t)self->region.size.width * 2 * 256 / 240);
    egui_canvas_draw_image_transform((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_0, cx, cy, 45, scale);
}

// ============================================================================
// External image transform (rotation) tests
// ============================================================================

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static void egui_view_test_performance_test_extern_image_rotate_565(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_0;
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    egui_canvas_draw_image_transform((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_0, cx, cy, 45, 256);
}

static void egui_view_test_performance_test_extern_image_rotate_565_1(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_1;
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    egui_canvas_draw_image_transform((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_1, cx, cy, 45, 256);
}

static void egui_view_test_performance_test_extern_image_rotate_565_2(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_2;
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    egui_canvas_draw_image_transform((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_2, cx, cy, 45, 256);
}

static void egui_view_test_performance_test_extern_image_rotate_565_4(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_4;
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    egui_canvas_draw_image_transform((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_4, cx, cy, 45, 256);
}

static void egui_view_test_performance_test_extern_image_rotate_565_8(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_8;
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    egui_canvas_draw_image_transform((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_8, cx, cy, 45, 256);
}
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE

// ============================================================================
// Legacy star image transform aliases
// ============================================================================

static void egui_view_test_performance_test_image_rotate_star_565_1(egui_view_t *self)
{
    egui_view_test_performance_test_image_rotate_565_1(self);
}

static void egui_view_test_performance_test_image_rotate_star_565_2(egui_view_t *self)
{
    egui_view_test_performance_test_image_rotate_565_2(self);
}

static void egui_view_test_performance_test_image_rotate_star_565_4(egui_view_t *self)
{
    egui_view_test_performance_test_image_rotate_565_4(self);
}

static void egui_view_test_performance_test_image_rotate_star_565_8(egui_view_t *self)
{
    egui_view_test_performance_test_image_rotate_565_8(self);
}

// ============================================================================
// Tiled small-image tests
// Tile dimensions: draw=40x40, resize=160x160, rotate-slot=80x80
// Sources: test_perf_Nx opaque RGB565 vs star_Nx transparent RGB565+alpha
// Purpose: flash-friendly alternative covering same total pixel workload as
//          large-image tests, works on small screens and embedded flash.
// ============================================================================

#define EGUI_TEST_TILED_DRAW_STEP   40
#define EGUI_TEST_TILED_RESIZE_STEP 160
#define EGUI_TEST_TILED_ROTATE_STEP 80

static void egui_test_tiled_draw_impl(egui_view_t *self, const egui_image_t *img)
{
    egui_dim_t W = self->region.size.width;
    egui_dim_t H = self->region.size.height;
    for (egui_dim_t y = 0; y < H; y += EGUI_TEST_TILED_DRAW_STEP)
    {
        for (egui_dim_t x = 0; x < W; x += EGUI_TEST_TILED_DRAW_STEP)
        {
            egui_canvas_draw_image(img, x, y);
        }
    }
}

static void egui_test_tiled_resize_impl(egui_view_t *self, const egui_image_t *img)
{
    egui_dim_t W = self->region.size.width;
    egui_dim_t H = self->region.size.height;
    for (egui_dim_t y = 0; y < H; y += EGUI_TEST_TILED_RESIZE_STEP)
    {
        for (egui_dim_t x = 0; x < W; x += EGUI_TEST_TILED_RESIZE_STEP)
        {
            egui_canvas_draw_image_resize(img, x, y, EGUI_TEST_TILED_RESIZE_STEP, EGUI_TEST_TILED_RESIZE_STEP);
        }
    }
}

static void egui_test_tiled_rotate_impl(egui_view_t *self, const egui_image_t *img)
{
    egui_dim_t W = self->region.size.width;
    egui_dim_t H = self->region.size.height;
    egui_dim_t half = EGUI_TEST_TILED_ROTATE_STEP / 2;
    for (egui_dim_t y = half; y <= H; y += EGUI_TEST_TILED_ROTATE_STEP)
    {
        for (egui_dim_t x = half; x <= W; x += EGUI_TEST_TILED_ROTATE_STEP)
        {
            egui_canvas_draw_image_transform(img, x, y, 45, 256);
        }
    }
}

/* --- tiled draw: test_perf_40 --- */
static void egui_view_test_performance_test_image_tiled_565_0(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_0;
    egui_test_tiled_draw_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_0);
}
static void egui_view_test_performance_test_image_tiled_565_1(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_1;
    egui_test_tiled_draw_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_1);
}
static void egui_view_test_performance_test_image_tiled_565_2(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_2;
    egui_test_tiled_draw_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_2);
}
static void egui_view_test_performance_test_image_tiled_565_4(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_4;
    egui_test_tiled_draw_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_4);
}
static void egui_view_test_performance_test_image_tiled_565_8(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_8;
    egui_test_tiled_draw_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_8);
}

/* --- tiled draw: star_40 --- */
static void egui_view_test_performance_test_image_tiled_star_565_0(egui_view_t *self)
{
    egui_view_test_performance_test_image_tiled_565_0(self);
}
static void egui_view_test_performance_test_image_tiled_star_565_1(egui_view_t *self)
{
    egui_view_test_performance_test_image_tiled_565_1(self);
}
static void egui_view_test_performance_test_image_tiled_star_565_2(egui_view_t *self)
{
    egui_view_test_performance_test_image_tiled_565_2(self);
}
static void egui_view_test_performance_test_image_tiled_star_565_4(egui_view_t *self)
{
    egui_view_test_performance_test_image_tiled_565_4(self);
}
static void egui_view_test_performance_test_image_tiled_star_565_8(egui_view_t *self)
{
    egui_view_test_performance_test_image_tiled_565_8(self);
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static void egui_view_test_performance_test_extern_image_tiled_565_0(egui_view_t *self)
{
    egui_test_tiled_draw_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_0);
}

static void egui_view_test_performance_test_extern_image_tiled_565_1(egui_view_t *self)
{
    egui_test_tiled_draw_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_1);
}

static void egui_view_test_performance_test_extern_image_tiled_565_2(egui_view_t *self)
{
    egui_test_tiled_draw_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_2);
}

static void egui_view_test_performance_test_extern_image_tiled_565_4(egui_view_t *self)
{
    egui_test_tiled_draw_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_4);
}

static void egui_view_test_performance_test_extern_image_tiled_565_8(egui_view_t *self)
{
    egui_test_tiled_draw_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_8);
}
#endif

/* --- tiled resize: test_perf_40 (src 40x40 -> 160x160 slot) --- */
static void egui_view_test_performance_test_image_resize_tiled_565_0(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_0;
    egui_test_tiled_resize_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_0);
}
static void egui_view_test_performance_test_image_resize_tiled_565_1(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_1;
    egui_test_tiled_resize_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_1);
}
static void egui_view_test_performance_test_image_resize_tiled_565_2(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_2;
    egui_test_tiled_resize_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_2);
}
static void egui_view_test_performance_test_image_resize_tiled_565_4(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_4;
    egui_test_tiled_resize_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_4);
}
static void egui_view_test_performance_test_image_resize_tiled_565_8(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_8;
    egui_test_tiled_resize_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_8);
}

/* --- tiled resize: star_40 (src 40x40 -> 160x160 slot) --- */
static void egui_view_test_performance_test_image_resize_tiled_star_565_0(egui_view_t *self)
{
    egui_view_test_performance_test_image_resize_tiled_565_0(self);
}
static void egui_view_test_performance_test_image_resize_tiled_star_565_1(egui_view_t *self)
{
    egui_view_test_performance_test_image_resize_tiled_565_1(self);
}
static void egui_view_test_performance_test_image_resize_tiled_star_565_2(egui_view_t *self)
{
    egui_view_test_performance_test_image_resize_tiled_565_2(self);
}
static void egui_view_test_performance_test_image_resize_tiled_star_565_4(egui_view_t *self)
{
    egui_view_test_performance_test_image_resize_tiled_565_4(self);
}
static void egui_view_test_performance_test_image_resize_tiled_star_565_8(egui_view_t *self)
{
    egui_view_test_performance_test_image_resize_tiled_565_8(self);
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static void egui_view_test_performance_test_extern_image_resize_tiled_565_0(egui_view_t *self)
{
    egui_test_tiled_resize_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_0);
}

static void egui_view_test_performance_test_extern_image_resize_tiled_565_1(egui_view_t *self)
{
    egui_test_tiled_resize_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_1);
}

static void egui_view_test_performance_test_extern_image_resize_tiled_565_2(egui_view_t *self)
{
    egui_test_tiled_resize_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_2);
}

static void egui_view_test_performance_test_extern_image_resize_tiled_565_4(egui_view_t *self)
{
    egui_test_tiled_resize_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_4);
}

static void egui_view_test_performance_test_extern_image_resize_tiled_565_8(egui_view_t *self)
{
    egui_test_tiled_resize_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_8);
}
#endif

/* --- tiled rotate: test_perf_40 (src 40x40, 80x80 slots, 45deg) --- */
static void egui_view_test_performance_test_image_rotate_tiled_565_0(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_0;
    egui_test_tiled_rotate_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_0);
}
static void egui_view_test_performance_test_image_rotate_tiled_565_1(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_1;
    egui_test_tiled_rotate_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_1);
}
static void egui_view_test_performance_test_image_rotate_tiled_565_2(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_2;
    egui_test_tiled_rotate_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_2);
}
static void egui_view_test_performance_test_image_rotate_tiled_565_4(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_4;
    egui_test_tiled_rotate_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_4);
}
static void egui_view_test_performance_test_image_rotate_tiled_565_8(egui_view_t *self)
{
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_8;
    egui_test_tiled_rotate_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_TILED_IMAGE_NAME_8);
}

/* --- tiled rotate: star_40 (src 40x40, 80x80 slots, 45deg) --- */
static void egui_view_test_performance_test_image_rotate_tiled_star_565_0(egui_view_t *self)
{
    egui_view_test_performance_test_image_rotate_tiled_565_0(self);
}
static void egui_view_test_performance_test_image_rotate_tiled_star_565_1(egui_view_t *self)
{
    egui_view_test_performance_test_image_rotate_tiled_565_1(self);
}
static void egui_view_test_performance_test_image_rotate_tiled_star_565_2(egui_view_t *self)
{
    egui_view_test_performance_test_image_rotate_tiled_565_2(self);
}
static void egui_view_test_performance_test_image_rotate_tiled_star_565_4(egui_view_t *self)
{
    egui_view_test_performance_test_image_rotate_tiled_565_4(self);
}
static void egui_view_test_performance_test_image_rotate_tiled_star_565_8(egui_view_t *self)
{
    egui_view_test_performance_test_image_rotate_tiled_565_8(self);
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static void egui_view_test_performance_test_extern_image_rotate_tiled_565_0(egui_view_t *self)
{
    egui_test_tiled_rotate_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_0);
}

static void egui_view_test_performance_test_extern_image_rotate_tiled_565_1(egui_view_t *self)
{
    egui_test_tiled_rotate_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_1);
}

static void egui_view_test_performance_test_extern_image_rotate_tiled_565_2(egui_view_t *self)
{
    egui_test_tiled_rotate_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_2);
}

static void egui_view_test_performance_test_extern_image_rotate_tiled_565_4(egui_view_t *self)
{
    egui_test_tiled_rotate_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_4);
}

static void egui_view_test_performance_test_extern_image_rotate_tiled_565_8(egui_view_t *self)
{
    egui_test_tiled_rotate_impl(self, (egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_TILED_IMAGE_NAME_8);
}
#endif

// ============================================================================
// Text transform (rotation) tests
// ============================================================================

static void egui_view_test_performance_test_text_rotate_none(egui_view_t *self)
{
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    egui_canvas_draw_text_transform((egui_font_t *)&egui_res_font_montserrat_26_4, text_rect_str, cx, cy, 0, 256, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_text_rotate(egui_view_t *self)
{
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    egui_canvas_draw_text_transform((egui_font_t *)&egui_res_font_montserrat_26_4, text_rect_str, cx, cy, 45, 256, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_text_rotate_resize(egui_view_t *self)
{
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    int16_t scale = (int16_t)((int32_t)self->region.size.width * 256 / 240);
    egui_canvas_draw_text_transform((egui_font_t *)&egui_res_font_montserrat_26_4, text_rect_str, cx, cy, 45, scale, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_text_rotate_quarter(egui_view_t *self)
{
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    egui_canvas_draw_text_transform((egui_font_t *)&egui_res_font_montserrat_26_4, text_rect_str, cx, cy, 45, 128, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_text_rotate_double(egui_view_t *self)
{
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    int16_t scale = (int16_t)((int32_t)self->region.size.width * 2 * 256 / 240);
    egui_canvas_draw_text_transform((egui_font_t *)&egui_res_font_montserrat_26_4, text_rect_str, cx, cy, 45, scale, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

// ============================================================================
// External text transform (rotation) tests
// ============================================================================

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static void egui_view_test_performance_test_extern_text_rotate(egui_view_t *self)
{
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    egui_canvas_draw_text_transform((egui_font_t *)&egui_res_font_montserrat_perf_26_4_bin, text_rect_str, cx, cy, 45, 256, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE

// ============================================================================
// Text + gradient overlay tests
// ============================================================================

static void egui_view_test_performance_test_text_gradient(egui_view_t *self)
{
    egui_mask_gradient_t mask;
    ensure_perf_masks_initialized();
    set_perf_gradient_mask(&mask);
    egui_canvas_draw_text((egui_font_t *)&egui_res_font_montserrat_26_4, text_rect_str, 0, 0, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_set_mask(NULL);
}

static void egui_view_test_performance_test_text_rect_gradient(egui_view_t *self)
{
    egui_mask_gradient_t mask;
    ensure_perf_masks_initialized();
    EGUI_REGION_DEFINE(text_rect, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, 200);
    set_perf_gradient_mask(&mask);
    egui_canvas_draw_text_in_rect((egui_font_t *)&egui_res_font_montserrat_26_4, text_rect_str, &text_rect, EGUI_ALIGN_LEFT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_set_mask(NULL);
}

static void egui_view_test_performance_test_text_rotate_gradient(egui_view_t *self)
{
    egui_mask_gradient_t mask;
    ensure_perf_masks_initialized();
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    set_perf_gradient_mask(&mask);
    egui_canvas_draw_text_transform((egui_font_t *)&egui_res_font_montserrat_26_4, text_rect_str, cx, cy, 45, 256, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_set_mask(NULL);
}

static void egui_view_test_performance_test_mask_gradient_rect_fill(egui_view_t *self)
{
    egui_mask_gradient_t mask;
    ensure_perf_masks_initialized();
    set_perf_gradient_mask(&mask);
    egui_canvas_draw_rectangle_fill(0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_set_mask(NULL);
}

static void egui_view_test_performance_test_mask_gradient_image(egui_view_t *self)
{
    egui_mask_gradient_t mask;
    ensure_perf_masks_initialized();
    set_perf_gradient_mask(&mask);
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_STAR_RESIZE_IMAGE_NAME_4;
    egui_dim_t x = (self->region.size.width - 120) / 2;
    egui_dim_t y = (self->region.size.height - 120) / 2;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERFORMANCE_STAR_RESIZE_IMAGE_NAME_4, x, y);
    egui_canvas_set_mask(NULL);
}

static void egui_view_test_performance_test_mask_gradient_image_rotate(egui_view_t *self)
{
    egui_mask_gradient_t mask;
    ensure_perf_masks_initialized();
    set_perf_gradient_mask(&mask);
    extern const egui_image_std_t EGUI_TEST_PERFORMANCE_STAR_RESIZE_IMAGE_NAME_4;
    egui_dim_t cx = self->region.size.width / 2;
    egui_dim_t cy = self->region.size.height / 2;
    egui_canvas_draw_image_transform((egui_image_t *)&EGUI_TEST_PERFORMANCE_STAR_RESIZE_IMAGE_NAME_4, cx, cy, 45, 256);
    egui_canvas_set_mask(NULL);
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static void egui_view_test_performance_test_extern_image_565(egui_view_t *self)
{
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_IMAGE_NAME_0, 0, 0);
}

static void egui_view_test_performance_test_extern_image_565_1(egui_view_t *self)
{
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_IMAGE_NAME_1, 0, 0);
}

static void egui_view_test_performance_test_extern_image_565_2(egui_view_t *self)
{
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_IMAGE_NAME_2, 0, 0);
}

static void egui_view_test_performance_test_extern_image_565_4(egui_view_t *self)
{
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_IMAGE_NAME_4, 0, 0);
}

static void egui_view_test_performance_test_extern_image_565_8(egui_view_t *self)
{
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_IMAGE_NAME_8, 0, 0);
}

static void egui_view_test_performance_test_extern_image_resize_565(egui_view_t *self)
{
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_0, 0, 0, PERF_SIZE_FULL, PERF_SIZE_FULL);
}

static void egui_view_test_performance_test_extern_image_resize_565_1(egui_view_t *self)
{
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_1, 0, 0, PERF_SIZE_FULL, PERF_SIZE_FULL);
}

static void egui_view_test_performance_test_extern_image_resize_565_2(egui_view_t *self)
{
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_2, 0, 0, PERF_SIZE_FULL, PERF_SIZE_FULL);
}

static void egui_view_test_performance_test_extern_image_resize_565_4(egui_view_t *self)
{
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_4, 0, 0, PERF_SIZE_FULL, PERF_SIZE_FULL);
}

static void egui_view_test_performance_test_extern_image_resize_565_8(egui_view_t *self)
{
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_EXTERN_RESIZE_IMAGE_NAME_8, 0, 0, PERF_SIZE_FULL, PERF_SIZE_FULL);
}
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE

// ============================================================================
// Multi-size mask rect fill tests
// ============================================================================

static void egui_view_test_performance_test_mask_rect_fill_no_mask_quarter(egui_view_t *self)
{
    // Full-size rect at screen center �?only 1/4 visible
    egui_canvas_draw_rectangle_fill(PERF_SIZE_QUARTER, PERF_SIZE_QUARTER, PERF_SIZE_FULL, PERF_SIZE_FULL, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_mask_rect_fill_no_mask_double(egui_view_t *self)
{
    egui_canvas_draw_rectangle_fill(0, 0, PERF_SIZE_DOUBLE, PERF_SIZE_DOUBLE, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_mask_rect_fill_round_rect_quarter(egui_view_t *self)
{
    egui_mask_round_rectangle_t mask;
    // Full-size mask + rect at screen center �?only 1/4 visible, tests clipping path
    set_perf_round_rect_mask(&mask, MASK_TEST_SIZE, MASK_TEST_RADIUS);
    egui_canvas_draw_rectangle_fill(PERF_SIZE_QUARTER, PERF_SIZE_QUARTER, PERF_SIZE_FULL, PERF_SIZE_FULL, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_rect_fill_round_rect_double(egui_view_t *self)
{
    egui_mask_round_rectangle_t mask;
    set_perf_round_rect_mask(&mask, PERF_SIZE_DOUBLE, PERF_SIZE_DOUBLE / 16);
    egui_canvas_draw_rectangle_fill(0, 0, PERF_SIZE_DOUBLE, PERF_SIZE_DOUBLE, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_rect_fill_circle_quarter(egui_view_t *self)
{
    egui_mask_circle_t mask;
    // Full-size mask + rect at screen center �?only 1/4 visible, tests clipping path
    set_perf_circle_mask(&mask, MASK_TEST_SIZE);
    egui_canvas_draw_rectangle_fill(PERF_SIZE_QUARTER, PERF_SIZE_QUARTER, PERF_SIZE_FULL, PERF_SIZE_FULL, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_rect_fill_circle_double(egui_view_t *self)
{
    egui_mask_circle_t mask;
    set_perf_circle_mask(&mask, (uint16_t)(egui_view_test_performance_adjust_circle(EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE) * 2));
    egui_canvas_draw_rectangle_fill(0, 0, PERF_SIZE_DOUBLE, PERF_SIZE_DOUBLE, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_rect_fill_image_quarter(egui_view_t *self)
{
    egui_mask_image_t mask;
    // Full-size mask + rect at screen center �?only 1/4 visible, tests clipping path
    set_perf_image_mask(&mask);
    egui_canvas_draw_rectangle_fill(PERF_SIZE_QUARTER, PERF_SIZE_QUARTER, 72, 72, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_rect_fill_image_double(egui_view_t *self)
{
    egui_mask_image_t mask;
    set_perf_image_mask(&mask);
    egui_canvas_draw_rectangle_fill(0, 0, 72, 72, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_clear_mask();
}

// ============================================================================
// Multi-size mask image tests
// ============================================================================

static void egui_view_test_performance_test_mask_image_no_mask_quarter(egui_view_t *self)
{
    // Full-size image resize at screen center �?only 1/4 visible, tests clipping path
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_8, PERF_SIZE_QUARTER, PERF_SIZE_QUARTER, PERF_SIZE_FULL,
                                  PERF_SIZE_FULL);
}

static void egui_view_test_performance_test_mask_image_no_mask_double(egui_view_t *self)
{
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_8, 0, 0, PERF_SIZE_DOUBLE, PERF_SIZE_DOUBLE);
}

static void egui_view_test_performance_test_mask_image_round_rect_quarter(egui_view_t *self)
{
    egui_mask_round_rectangle_t mask;
    // Full-size mask + image resize at screen center �?only 1/4 visible, tests clipping path
    set_perf_round_rect_mask(&mask, MASK_TEST_SIZE, MASK_TEST_RADIUS);
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_8, PERF_SIZE_QUARTER, PERF_SIZE_QUARTER, PERF_SIZE_FULL,
                                  PERF_SIZE_FULL);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_round_rect_double(egui_view_t *self)
{
    egui_mask_round_rectangle_t mask;
    set_perf_round_rect_mask(&mask, PERF_SIZE_DOUBLE, PERF_SIZE_DOUBLE / 16);
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_8, 0, 0, PERF_SIZE_DOUBLE, PERF_SIZE_DOUBLE);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_circle_quarter(egui_view_t *self)
{
    egui_mask_circle_t mask;
    // Full-size mask + image resize at screen center �?only 1/4 visible, tests clipping path
    set_perf_circle_mask(&mask, MASK_TEST_SIZE);
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_8, PERF_SIZE_QUARTER, PERF_SIZE_QUARTER, PERF_SIZE_FULL,
                                  PERF_SIZE_FULL);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_circle_double(egui_view_t *self)
{
    egui_mask_circle_t mask;
    set_perf_circle_mask(&mask, (uint16_t)(egui_view_test_performance_adjust_circle(EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE) * 2));
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_8, 0, 0, PERF_SIZE_DOUBLE, PERF_SIZE_DOUBLE);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_image_quarter(egui_view_t *self)
{
    egui_mask_image_t mask;
    set_perf_image_mask(&mask);
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_8, PERF_SIZE_QUARTER, PERF_SIZE_QUARTER, PERF_SIZE_QUARTER,
                                  PERF_SIZE_QUARTER);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_image_double(egui_view_t *self)
{
    egui_mask_image_t mask;
    set_perf_image_mask(&mask);
    egui_canvas_draw_image_resize((egui_image_t *)&EGUI_TEST_PERFORMANCE_RESIZE_IMAGE_NAME_8, 0, 0, PERF_SIZE_QUARTER, PERF_SIZE_QUARTER);
    egui_canvas_clear_mask();
}

// ============================================================================
// Multi-size shape fill tests
// ============================================================================

static void egui_view_test_performance_test_circle_fill_quarter(egui_view_t *self)
{
    // Full-size circle centered at (PERF_SIZE_FULL, PERF_SIZE_FULL) �?only 1/4 visible, tests clipping path
    egui_dim_t central_x = PERF_SIZE_FULL;
    egui_dim_t central_y = PERF_SIZE_FULL;
    egui_dim_t radius = egui_view_test_performance_adjust_circle((EGUI_MIN(EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT) >> 1) - 1);
    egui_canvas_draw_circle_fill(central_x, central_y, radius, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_circle_fill_double(egui_view_t *self)
{
    egui_dim_t central = PERF_SIZE_DOUBLE >> 1;
    egui_dim_t radius = egui_view_test_performance_adjust_circle(central - 1);
    egui_canvas_draw_circle_fill(central, central, radius, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_round_rectangle_fill_quarter(egui_view_t *self)
{
    // Full-size round rect at screen center �?only 1/4 visible, tests clipping path
    egui_dim_t size = PERF_SIZE_FULL;
    egui_dim_t radius = egui_view_test_performance_adjust_circle(size >> 1);
    egui_canvas_draw_round_rectangle_fill(PERF_SIZE_QUARTER, PERF_SIZE_QUARTER, size, size, radius, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_round_rectangle_fill_double(egui_view_t *self)
{
    egui_dim_t size = PERF_SIZE_DOUBLE;
    egui_dim_t radius = egui_view_test_performance_adjust_circle(size >> 1);
    egui_canvas_draw_round_rectangle_fill(0, 0, size, size, radius, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_triangle_fill_quarter(egui_view_t *self)
{
    // Full-size triangle shifted by (PERF_SIZE_QUARTER, PERF_SIZE_QUARTER) �?only 1/4 visible, tests clipping path
    egui_dim_t size = PERF_SIZE_FULL;
    egui_dim_t ox = PERF_SIZE_QUARTER;
    egui_dim_t oy = PERF_SIZE_QUARTER;
    egui_canvas_draw_triangle_fill(ox + (size >> 1), oy + 0, ox + 0, oy + size - 1, ox + size - 1, oy + size - 1, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void egui_view_test_performance_test_triangle_fill_double(egui_view_t *self)
{
    egui_dim_t size = PERF_SIZE_DOUBLE;
    egui_canvas_draw_triangle_fill(size >> 1, 0, 0, size - 1, size - 1, size - 1, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

// ============================================================================
// Compressed image tests (QOI)
// ============================================================================
#if EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE

// Compressed image resource name macros (QOI)
#define EGUI_TEST_PERF_QOI_IMAGE_NAME_0   egui_res_image_test_perf_240_qoi_rgb565_0
#define EGUI_TEST_PERF_QOI_IMAGE_NAME_8   egui_res_image_star_240_qoi_rgb565_8
#define EGUI_TEST_PERF_QOI_TILED_NAME_0   egui_res_image_test_perf_40_qoi_rgb565_0
#define EGUI_TEST_PERF_QOI_TILED_NAME_8   egui_res_image_star_40_qoi_rgb565_8
#define EGUI_TEST_PERF_QOI_MASK_NAME_0    egui_res_image_test_perf_240_qoi_rgb565_0
#define EGUI_TEST_PERF_QOI_MASK_NAME_8    egui_res_image_star_240_qoi_rgb565_8
#define EGUI_TEST_PERF_QOI_MASK_QUARTER_0 egui_res_image_test_perf_120_qoi_rgb565_0
#define EGUI_TEST_PERF_QOI_MASK_QUARTER_8 egui_res_image_star_120_qoi_rgb565_8
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
#define EGUI_TEST_PERF_EXTERN_QOI_IMAGE_NAME_0   egui_res_image_test_perf_240_ext_qoi_rgb565_0_bin
#define EGUI_TEST_PERF_EXTERN_QOI_IMAGE_NAME_8   egui_res_image_star_240_ext_qoi_rgb565_8_bin
#define EGUI_TEST_PERF_EXTERN_QOI_MASK_NAME_0    EGUI_TEST_PERF_EXTERN_QOI_IMAGE_NAME_0
#define EGUI_TEST_PERF_EXTERN_QOI_MASK_NAME_8    EGUI_TEST_PERF_EXTERN_QOI_IMAGE_NAME_8
#define EGUI_TEST_PERF_EXTERN_QOI_MASK_QUARTER_0 egui_res_image_test_perf_120_ext_qoi_rgb565_0_bin
#define EGUI_TEST_PERF_EXTERN_QOI_MASK_QUARTER_8 egui_res_image_star_120_ext_qoi_rgb565_8_bin
#endif

/* direct draw */
static void egui_view_test_performance_test_image_qoi_565(egui_view_t *self)
{
    extern const egui_image_qoi_t EGUI_TEST_PERF_QOI_IMAGE_NAME_0;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_QOI_IMAGE_NAME_0, 0, 0);
}

static void egui_view_test_performance_test_image_qoi_565_8(egui_view_t *self)
{
    extern const egui_image_qoi_t EGUI_TEST_PERF_QOI_IMAGE_NAME_8;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_QOI_IMAGE_NAME_8, 0, 0);
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static void egui_view_test_performance_test_extern_image_qoi_565(egui_view_t *self)
{
    extern const egui_image_qoi_t EGUI_TEST_PERF_EXTERN_QOI_IMAGE_NAME_0;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_QOI_IMAGE_NAME_0, 0, 0);
}

static void egui_view_test_performance_test_extern_image_qoi_565_8(egui_view_t *self)
{
    extern const egui_image_qoi_t EGUI_TEST_PERF_EXTERN_QOI_IMAGE_NAME_8;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_QOI_IMAGE_NAME_8, 0, 0);
}
#endif

/* tiled draw */
static void egui_view_test_performance_test_image_tiled_qoi_565_0(egui_view_t *self)
{
    extern const egui_image_qoi_t EGUI_TEST_PERF_QOI_TILED_NAME_0;
    egui_test_tiled_draw_impl(self, (egui_image_t *)&EGUI_TEST_PERF_QOI_TILED_NAME_0);
}

static void egui_view_test_performance_test_image_tiled_qoi_565_8(egui_view_t *self)
{
    extern const egui_image_qoi_t EGUI_TEST_PERF_QOI_TILED_NAME_8;
    egui_test_tiled_draw_impl(self, (egui_image_t *)&EGUI_TEST_PERF_QOI_TILED_NAME_8);
}

/* mask + opaque QOI */
static void egui_view_test_performance_test_mask_image_qoi_no_mask(egui_view_t *self)
{
    extern const egui_image_qoi_t EGUI_TEST_PERF_QOI_MASK_NAME_0;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_QOI_MASK_NAME_0, 0, 0);
}

static void egui_view_test_performance_test_mask_image_qoi_round_rect(egui_view_t *self)
{
    egui_mask_round_rectangle_t mask;
    extern const egui_image_qoi_t EGUI_TEST_PERF_QOI_MASK_NAME_0;
    set_perf_round_rect_mask(&mask, MASK_TEST_SIZE, MASK_TEST_RADIUS);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_QOI_MASK_NAME_0, 0, 0);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_qoi_circle(egui_view_t *self)
{
    egui_mask_circle_t mask;
    extern const egui_image_qoi_t EGUI_TEST_PERF_QOI_MASK_NAME_0;
    set_perf_circle_mask(&mask, MASK_TEST_SIZE);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_QOI_MASK_NAME_0, 0, 0);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_qoi_image(egui_view_t *self)
{
    egui_mask_image_t mask;
    extern const egui_image_qoi_t EGUI_TEST_PERF_QOI_MASK_QUARTER_0;
    set_perf_image_mask(&mask);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_QOI_MASK_QUARTER_0, 0, 0);
    egui_canvas_clear_mask();
}

/* mask + alpha=8 QOI */
static void egui_view_test_performance_test_mask_image_qoi_8_no_mask(egui_view_t *self)
{
    extern const egui_image_qoi_t EGUI_TEST_PERF_QOI_MASK_NAME_8;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_QOI_MASK_NAME_8, 0, 0);
}

static void egui_view_test_performance_test_mask_image_qoi_8_round_rect(egui_view_t *self)
{
    egui_mask_round_rectangle_t mask;
    extern const egui_image_qoi_t EGUI_TEST_PERF_QOI_MASK_NAME_8;
    set_perf_round_rect_mask(&mask, MASK_TEST_SIZE, MASK_TEST_RADIUS);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_QOI_MASK_NAME_8, 0, 0);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_qoi_8_circle(egui_view_t *self)
{
    egui_mask_circle_t mask;
    extern const egui_image_qoi_t EGUI_TEST_PERF_QOI_MASK_NAME_8;
    set_perf_circle_mask(&mask, MASK_TEST_SIZE);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_QOI_MASK_NAME_8, 0, 0);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_qoi_8_image(egui_view_t *self)
{
    egui_mask_image_t mask;
    extern const egui_image_qoi_t EGUI_TEST_PERF_QOI_MASK_QUARTER_8;
    set_perf_image_mask(&mask);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_QOI_MASK_QUARTER_8, 0, 0);
    egui_canvas_clear_mask();
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static void egui_view_test_performance_test_extern_mask_image_qoi_no_mask(egui_view_t *self)
{
    extern const egui_image_qoi_t EGUI_TEST_PERF_EXTERN_QOI_MASK_NAME_0;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_QOI_MASK_NAME_0, 0, 0);
}

static void egui_view_test_performance_test_extern_mask_image_qoi_round_rect(egui_view_t *self)
{
    egui_mask_round_rectangle_t mask;
    extern const egui_image_qoi_t EGUI_TEST_PERF_EXTERN_QOI_MASK_NAME_0;
    set_perf_round_rect_mask(&mask, MASK_TEST_SIZE, MASK_TEST_RADIUS);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_QOI_MASK_NAME_0, 0, 0);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_extern_mask_image_qoi_circle(egui_view_t *self)
{
    egui_mask_circle_t mask;
    extern const egui_image_qoi_t EGUI_TEST_PERF_EXTERN_QOI_MASK_NAME_0;
    set_perf_circle_mask(&mask, MASK_TEST_SIZE);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_QOI_MASK_NAME_0, 0, 0);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_extern_mask_image_qoi_image(egui_view_t *self)
{
    egui_mask_image_t mask;
    extern const egui_image_qoi_t EGUI_TEST_PERF_EXTERN_QOI_MASK_QUARTER_0;
    set_perf_image_mask(&mask);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_QOI_MASK_QUARTER_0, 0, 0);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_extern_mask_image_qoi_8_no_mask(egui_view_t *self)
{
    extern const egui_image_qoi_t EGUI_TEST_PERF_EXTERN_QOI_MASK_NAME_8;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_QOI_MASK_NAME_8, 0, 0);
}

static void egui_view_test_performance_test_extern_mask_image_qoi_8_round_rect(egui_view_t *self)
{
    egui_mask_round_rectangle_t mask;
    extern const egui_image_qoi_t EGUI_TEST_PERF_EXTERN_QOI_MASK_NAME_8;
    set_perf_round_rect_mask(&mask, MASK_TEST_SIZE, MASK_TEST_RADIUS);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_QOI_MASK_NAME_8, 0, 0);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_extern_mask_image_qoi_8_circle(egui_view_t *self)
{
    egui_mask_circle_t mask;
    extern const egui_image_qoi_t EGUI_TEST_PERF_EXTERN_QOI_MASK_NAME_8;
    set_perf_circle_mask(&mask, MASK_TEST_SIZE);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_QOI_MASK_NAME_8, 0, 0);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_extern_mask_image_qoi_8_image(egui_view_t *self)
{
    egui_mask_image_t mask;
    extern const egui_image_qoi_t EGUI_TEST_PERF_EXTERN_QOI_MASK_QUARTER_8;
    set_perf_image_mask(&mask);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_QOI_MASK_QUARTER_8, 0, 0);
    egui_canvas_clear_mask();
}
#endif

#endif // EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE

// ============================================================================
// Compressed image tests (RLE)
// ============================================================================
#if EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE

// Compressed image resource name macros (RLE)
#define EGUI_TEST_PERF_RLE_IMAGE_NAME_0   egui_res_image_test_perf_240_rle_rgb565_0
#define EGUI_TEST_PERF_RLE_IMAGE_NAME_8   egui_res_image_star_240_rle_rgb565_8
#define EGUI_TEST_PERF_RLE_TILED_NAME_0   egui_res_image_test_perf_40_rle_rgb565_0
#define EGUI_TEST_PERF_RLE_TILED_NAME_8   egui_res_image_star_40_rle_rgb565_8
#define EGUI_TEST_PERF_RLE_MASK_NAME_0    egui_res_image_test_perf_240_rle_rgb565_0
#define EGUI_TEST_PERF_RLE_MASK_NAME_8    egui_res_image_star_240_rle_rgb565_8
#define EGUI_TEST_PERF_RLE_MASK_QUARTER_0 egui_res_image_test_perf_120_rle_rgb565_0
#define EGUI_TEST_PERF_RLE_MASK_QUARTER_8 egui_res_image_star_120_rle_rgb565_8
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
#define EGUI_TEST_PERF_EXTERN_RLE_IMAGE_NAME_0   egui_res_image_test_perf_240_ext_rle_rgb565_0_bin
#define EGUI_TEST_PERF_EXTERN_RLE_IMAGE_NAME_8   egui_res_image_star_240_ext_rle_rgb565_8_bin
#define EGUI_TEST_PERF_EXTERN_RLE_MASK_NAME_0    EGUI_TEST_PERF_EXTERN_RLE_IMAGE_NAME_0
#define EGUI_TEST_PERF_EXTERN_RLE_MASK_NAME_8    EGUI_TEST_PERF_EXTERN_RLE_IMAGE_NAME_8
#define EGUI_TEST_PERF_EXTERN_RLE_MASK_QUARTER_0 egui_res_image_test_perf_120_ext_rle_rgb565_0_bin
#define EGUI_TEST_PERF_EXTERN_RLE_MASK_QUARTER_8 egui_res_image_star_120_ext_rle_rgb565_8_bin
#endif

/* direct draw */
static void egui_view_test_performance_test_image_rle_565(egui_view_t *self)
{
    extern const egui_image_rle_t EGUI_TEST_PERF_RLE_IMAGE_NAME_0;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_RLE_IMAGE_NAME_0, 0, 0);
}

static void egui_view_test_performance_test_image_rle_565_8(egui_view_t *self)
{
    extern const egui_image_rle_t EGUI_TEST_PERF_RLE_IMAGE_NAME_8;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_RLE_IMAGE_NAME_8, 0, 0);
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static void egui_view_test_performance_test_extern_image_rle_565(egui_view_t *self)
{
    extern const egui_image_rle_t EGUI_TEST_PERF_EXTERN_RLE_IMAGE_NAME_0;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_RLE_IMAGE_NAME_0, 0, 0);
}

static void egui_view_test_performance_test_extern_image_rle_565_8(egui_view_t *self)
{
    extern const egui_image_rle_t EGUI_TEST_PERF_EXTERN_RLE_IMAGE_NAME_8;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_RLE_IMAGE_NAME_8, 0, 0);
}
#endif

/* tiled draw */
static void egui_view_test_performance_test_image_tiled_rle_565_0(egui_view_t *self)
{
    extern const egui_image_rle_t EGUI_TEST_PERF_RLE_TILED_NAME_0;
    egui_test_tiled_draw_impl(self, (egui_image_t *)&EGUI_TEST_PERF_RLE_TILED_NAME_0);
}

static void egui_view_test_performance_test_image_tiled_rle_565_8(egui_view_t *self)
{
    extern const egui_image_rle_t EGUI_TEST_PERF_RLE_TILED_NAME_8;
    egui_test_tiled_draw_impl(self, (egui_image_t *)&EGUI_TEST_PERF_RLE_TILED_NAME_8);
}

/* mask + opaque RLE */
static void egui_view_test_performance_test_mask_image_rle_no_mask(egui_view_t *self)
{
    extern const egui_image_rle_t EGUI_TEST_PERF_RLE_MASK_NAME_0;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_RLE_MASK_NAME_0, 0, 0);
}

static void egui_view_test_performance_test_mask_image_rle_round_rect(egui_view_t *self)
{
    egui_mask_round_rectangle_t mask;
    extern const egui_image_rle_t EGUI_TEST_PERF_RLE_MASK_NAME_0;
    set_perf_round_rect_mask(&mask, MASK_TEST_SIZE, MASK_TEST_RADIUS);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_RLE_MASK_NAME_0, 0, 0);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_rle_circle(egui_view_t *self)
{
    egui_mask_circle_t mask;
    extern const egui_image_rle_t EGUI_TEST_PERF_RLE_MASK_NAME_0;
    set_perf_circle_mask(&mask, MASK_TEST_SIZE);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_RLE_MASK_NAME_0, 0, 0);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_rle_image(egui_view_t *self)
{
    egui_mask_image_t mask;
    extern const egui_image_rle_t EGUI_TEST_PERF_RLE_MASK_QUARTER_0;
    set_perf_image_mask(&mask);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_RLE_MASK_QUARTER_0, 0, 0);
    egui_canvas_clear_mask();
}

/* mask + alpha=8 RLE */
static void egui_view_test_performance_test_mask_image_rle_8_no_mask(egui_view_t *self)
{
    extern const egui_image_rle_t EGUI_TEST_PERF_RLE_MASK_NAME_8;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_RLE_MASK_NAME_8, 0, 0);
}

static void egui_view_test_performance_test_mask_image_rle_8_round_rect(egui_view_t *self)
{
    egui_mask_round_rectangle_t mask;
    extern const egui_image_rle_t EGUI_TEST_PERF_RLE_MASK_NAME_8;
    set_perf_round_rect_mask(&mask, MASK_TEST_SIZE, MASK_TEST_RADIUS);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_RLE_MASK_NAME_8, 0, 0);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_rle_8_circle(egui_view_t *self)
{
    egui_mask_circle_t mask;
    extern const egui_image_rle_t EGUI_TEST_PERF_RLE_MASK_NAME_8;
    set_perf_circle_mask(&mask, MASK_TEST_SIZE);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_RLE_MASK_NAME_8, 0, 0);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_mask_image_rle_8_image(egui_view_t *self)
{
    egui_mask_image_t mask;
    extern const egui_image_rle_t EGUI_TEST_PERF_RLE_MASK_QUARTER_8;
    set_perf_image_mask(&mask);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_RLE_MASK_QUARTER_8, 0, 0);
    egui_canvas_clear_mask();
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static void egui_view_test_performance_test_extern_mask_image_rle_no_mask(egui_view_t *self)
{
    extern const egui_image_rle_t EGUI_TEST_PERF_EXTERN_RLE_MASK_NAME_0;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_RLE_MASK_NAME_0, 0, 0);
}

static void egui_view_test_performance_test_extern_mask_image_rle_round_rect(egui_view_t *self)
{
    egui_mask_round_rectangle_t mask;
    extern const egui_image_rle_t EGUI_TEST_PERF_EXTERN_RLE_MASK_NAME_0;
    set_perf_round_rect_mask(&mask, MASK_TEST_SIZE, MASK_TEST_RADIUS);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_RLE_MASK_NAME_0, 0, 0);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_extern_mask_image_rle_circle(egui_view_t *self)
{
    egui_mask_circle_t mask;
    extern const egui_image_rle_t EGUI_TEST_PERF_EXTERN_RLE_MASK_NAME_0;
    set_perf_circle_mask(&mask, MASK_TEST_SIZE);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_RLE_MASK_NAME_0, 0, 0);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_extern_mask_image_rle_image(egui_view_t *self)
{
    egui_mask_image_t mask;
    extern const egui_image_rle_t EGUI_TEST_PERF_EXTERN_RLE_MASK_QUARTER_0;
    set_perf_image_mask(&mask);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_RLE_MASK_QUARTER_0, 0, 0);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_extern_mask_image_rle_8_no_mask(egui_view_t *self)
{
    extern const egui_image_rle_t EGUI_TEST_PERF_EXTERN_RLE_MASK_NAME_8;
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_RLE_MASK_NAME_8, 0, 0);
}

static void egui_view_test_performance_test_extern_mask_image_rle_8_round_rect(egui_view_t *self)
{
    egui_mask_round_rectangle_t mask;
    extern const egui_image_rle_t EGUI_TEST_PERF_EXTERN_RLE_MASK_NAME_8;
    set_perf_round_rect_mask(&mask, MASK_TEST_SIZE, MASK_TEST_RADIUS);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_RLE_MASK_NAME_8, 0, 0);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_extern_mask_image_rle_8_circle(egui_view_t *self)
{
    egui_mask_circle_t mask;
    extern const egui_image_rle_t EGUI_TEST_PERF_EXTERN_RLE_MASK_NAME_8;
    set_perf_circle_mask(&mask, MASK_TEST_SIZE);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_RLE_MASK_NAME_8, 0, 0);
    egui_canvas_clear_mask();
}

static void egui_view_test_performance_test_extern_mask_image_rle_8_image(egui_view_t *self)
{
    egui_mask_image_t mask;
    extern const egui_image_rle_t EGUI_TEST_PERF_EXTERN_RLE_MASK_QUARTER_8;
    set_perf_image_mask(&mask);
    egui_canvas_draw_image((egui_image_t *)&EGUI_TEST_PERF_EXTERN_RLE_MASK_QUARTER_8, 0, 0);
    egui_canvas_clear_mask();
}
#endif

#endif // EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE

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
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_INTERNAL_TEXT:
        egui_view_test_performance_test_internal_text(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_INTERNAL_TEXT_RECT:
        egui_view_test_performance_test_internal_text_rect(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_INTERNAL_TEXT_RLE4:
        egui_view_test_performance_test_internal_text_rle4(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_INTERNAL_TEXT_RECT_RLE4:
        egui_view_test_performance_test_internal_text_rect_rle4(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_INTERNAL_TEXT_RLE4_XOR:
        egui_view_test_performance_test_internal_text_rle4_xor(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_INTERNAL_TEXT_RECT_RLE4_XOR:
        egui_view_test_performance_test_internal_text_rect_rle4_xor(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT:
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        egui_view_test_performance_test_extern_text(self);
#endif
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT_RECT:
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        egui_view_test_performance_test_extern_text_rect(self);
#endif
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT_RLE4:
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        egui_view_test_performance_test_extern_text_rle4(self);
#endif
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT_RECT_RLE4:
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        egui_view_test_performance_test_extern_text_rect_rle4(self);
#endif
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT_RLE4_XOR:
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        egui_view_test_performance_test_extern_text_rle4_xor(self);
#endif
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT_RECT_RLE4_XOR:
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        egui_view_test_performance_test_extern_text_rect_rle4_xor(self);
#endif
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
        egui_view_test_performance_test_ellipse(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ELLIPSE_FILL:
        egui_view_test_performance_test_ellipse_fill(self);
        break;

    // Polygon
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_POLYGON:
        egui_view_test_performance_test_polygon(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_POLYGON_FILL:
        egui_view_test_performance_test_polygon_fill(self);
        break;

    // Bezier
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_BEZIER_QUAD:
        egui_view_test_performance_test_bezier_quad(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_BEZIER_CUBIC:
        egui_view_test_performance_test_bezier_cubic(self);
        break;

    // HQ circle/arc
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE_HQ:
        egui_view_test_performance_test_circle_hq(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE_FILL_HQ:
        egui_view_test_performance_test_circle_fill_hq(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ARC_HQ:
        egui_view_test_performance_test_arc_hq(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ARC_FILL_HQ:
        egui_view_test_performance_test_arc_fill_hq(self);
        break;

    // HQ line
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_LINE_HQ:
        egui_view_test_performance_test_line_hq(self);
        break;

    // Gradient
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_RECT:
        egui_view_test_performance_test_gradient_rect(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ROUND_RECT:
        egui_view_test_performance_test_gradient_round_rect(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_CIRCLE:
        egui_view_test_performance_test_gradient_circle(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_TRIANGLE:
        egui_view_test_performance_test_gradient_triangle(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ARC_RING:
        egui_view_test_performance_test_gradient_arc_ring(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ARC_RING_ROUND_CAP:
        egui_view_test_performance_test_gradient_arc_ring_round_cap(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_RADIAL:
        egui_view_test_performance_test_gradient_radial(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ANGULAR:
        egui_view_test_performance_test_gradient_angular(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ROUND_RECT_RING:
        egui_view_test_performance_test_gradient_round_rect_ring(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_LINE_CAPSULE:
        egui_view_test_performance_test_gradient_line_capsule(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_MULTI_STOP:
        egui_view_test_performance_test_gradient_multi_stop(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ROUND_RECT_CORNERS:
        egui_view_test_performance_test_gradient_round_rect_corners(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_GRADIENT_OVERLAY:
        egui_view_test_performance_test_image_gradient_overlay(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_GRADIENT_RECT_FILL:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_gradient_rect_fill(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_GRADIENT_IMAGE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_gradient_image(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_GRADIENT_IMAGE_ROTATE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_gradient_image_rotate(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_COLOR:
        egui_view_test_performance_test_image_color(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_COLOR:
        egui_view_test_performance_test_image_resize_color(self);
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
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_NO_MASK:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_no_mask(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_ROUND_RECT:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_round_rect(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_CIRCLE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_circle(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_IMAGE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_image(self);
        break;
#endif
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_TEST_PERF_NO_MASK:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_test_perf_no_mask(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_TEST_PERF_ROUND_RECT:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_test_perf_round_rect(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_TEST_PERF_CIRCLE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_test_perf_circle(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_TEST_PERF_IMAGE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_test_perf_image(self);
        break;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_TEST_PERF_NO_MASK:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_test_perf_no_mask(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_test_perf_round_rect(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_test_perf_circle(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_TEST_PERF_IMAGE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_test_perf_image(self);
        break;
#endif
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

    // Image multi-size
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_QUARTER:
        egui_view_test_performance_test_image_565_quarter(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_DOUBLE:
        egui_view_test_performance_test_image_565_double(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_8_QUARTER:
        egui_view_test_performance_test_image_565_8_quarter(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_8_DOUBLE:
        egui_view_test_performance_test_image_565_8_double(self);
        break;

        // Extern image tests
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_565:
        egui_view_test_performance_test_extern_image_565(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_565_1:
        egui_view_test_performance_test_extern_image_565_1(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_565_2:
        egui_view_test_performance_test_extern_image_565_2(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_565_4:
        egui_view_test_performance_test_extern_image_565_4(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_565_8:
        egui_view_test_performance_test_extern_image_565_8(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_565:
        egui_view_test_performance_test_extern_image_resize_565(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_565_1:
        egui_view_test_performance_test_extern_image_resize_565_1(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_565_2:
        egui_view_test_performance_test_extern_image_resize_565_2(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_565_4:
        egui_view_test_performance_test_extern_image_resize_565_4(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_565_8:
        egui_view_test_performance_test_extern_image_resize_565_8(self);
        break;
#endif

    // Mask rect fill multi-size
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_NO_MASK_QUARTER:
        egui_view_test_performance_test_mask_rect_fill_no_mask_quarter(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_NO_MASK_DOUBLE:
        egui_view_test_performance_test_mask_rect_fill_no_mask_double(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_ROUND_RECT_QUARTER:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_rect_fill_round_rect_quarter(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_ROUND_RECT_DOUBLE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_rect_fill_round_rect_double(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_CIRCLE_QUARTER:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_rect_fill_circle_quarter(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_CIRCLE_DOUBLE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_rect_fill_circle_double(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_IMAGE_QUARTER:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_rect_fill_image_quarter(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_IMAGE_DOUBLE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_rect_fill_image_double(self);
        break;

    // Mask image multi-size
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_NO_MASK_QUARTER:
        egui_view_test_performance_test_mask_image_no_mask_quarter(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_NO_MASK_DOUBLE:
        egui_view_test_performance_test_mask_image_no_mask_double(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_ROUND_RECT_QUARTER:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_round_rect_quarter(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_ROUND_RECT_DOUBLE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_round_rect_double(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_CIRCLE_QUARTER:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_circle_quarter(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_CIRCLE_DOUBLE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_circle_double(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_IMAGE_QUARTER:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_image_quarter(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_IMAGE_DOUBLE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_image_double(self);
        break;

    // Shape fill multi-size
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE_FILL_QUARTER:
        egui_view_test_performance_test_circle_fill_quarter(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE_FILL_DOUBLE:
        egui_view_test_performance_test_circle_fill_double(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ROUND_RECTANGLE_FILL_QUARTER:
        egui_view_test_performance_test_round_rectangle_fill_quarter(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ROUND_RECTANGLE_FILL_DOUBLE:
        egui_view_test_performance_test_round_rectangle_fill_double(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TRIANGLE_FILL_QUARTER:
        egui_view_test_performance_test_triangle_fill_quarter(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TRIANGLE_FILL_DOUBLE:
        egui_view_test_performance_test_triangle_fill_double(self);
        break;

    // Image transform (rotation)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565:
        egui_view_test_performance_test_image_rotate_565(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_1:
        egui_view_test_performance_test_image_rotate_565_1(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_2:
        egui_view_test_performance_test_image_rotate_565_2(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_4:
        egui_view_test_performance_test_image_rotate_565_4(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_8:
        egui_view_test_performance_test_image_rotate_565_8(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_RESIZE:
        egui_view_test_performance_test_image_rotate_565_resize(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_QUARTER:
        egui_view_test_performance_test_image_rotate_565_quarter(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_DOUBLE:
        egui_view_test_performance_test_image_rotate_565_double(self);
        break;

        // External image transform (rotation)
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_565:
        egui_view_test_performance_test_extern_image_rotate_565(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_565_1:
        egui_view_test_performance_test_extern_image_rotate_565_1(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_565_2:
        egui_view_test_performance_test_extern_image_rotate_565_2(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_565_4:
        egui_view_test_performance_test_extern_image_rotate_565_4(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_565_8:
        egui_view_test_performance_test_extern_image_rotate_565_8(self);
        break;
#endif

    // Star image tests (real alpha)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_STAR_565_1:
        egui_view_test_performance_test_image_resize_star_565_1(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_STAR_565_2:
        egui_view_test_performance_test_image_resize_star_565_2(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_STAR_565_4:
        egui_view_test_performance_test_image_resize_star_565_4(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_STAR_565_8:
        egui_view_test_performance_test_image_resize_star_565_8(self);
        break;

    // Star image transform (rotation, real alpha)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_STAR_565_1:
        egui_view_test_performance_test_image_rotate_star_565_1(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_STAR_565_2:
        egui_view_test_performance_test_image_rotate_star_565_2(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_STAR_565_4:
        egui_view_test_performance_test_image_rotate_star_565_4(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_STAR_565_8:
        egui_view_test_performance_test_image_rotate_star_565_8(self);
        break;

    // Tiled draw (40x40 tiles, test_perf)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_565_0:
        egui_view_test_performance_test_image_tiled_565_0(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_565_1:
        egui_view_test_performance_test_image_tiled_565_1(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_565_2:
        egui_view_test_performance_test_image_tiled_565_2(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_565_4:
        egui_view_test_performance_test_image_tiled_565_4(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_565_8:
        egui_view_test_performance_test_image_tiled_565_8(self);
        break;

    // Tiled draw (40x40 tiles, star)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_STAR_565_0:
        egui_view_test_performance_test_image_tiled_star_565_0(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_STAR_565_1:
        egui_view_test_performance_test_image_tiled_star_565_1(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_STAR_565_2:
        egui_view_test_performance_test_image_tiled_star_565_2(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_STAR_565_4:
        egui_view_test_performance_test_image_tiled_star_565_4(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_STAR_565_8:
        egui_view_test_performance_test_image_tiled_star_565_8(self);
        break;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_TILED_565_0:
        egui_view_test_performance_test_extern_image_tiled_565_0(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_TILED_565_1:
        egui_view_test_performance_test_extern_image_tiled_565_1(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_TILED_565_2:
        egui_view_test_performance_test_extern_image_tiled_565_2(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_TILED_565_4:
        egui_view_test_performance_test_extern_image_tiled_565_4(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_TILED_565_8:
        egui_view_test_performance_test_extern_image_tiled_565_8(self);
        break;
#endif

    // Tiled resize (160x160 tiles, test_perf)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_565_0:
        egui_view_test_performance_test_image_resize_tiled_565_0(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_565_1:
        egui_view_test_performance_test_image_resize_tiled_565_1(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_565_2:
        egui_view_test_performance_test_image_resize_tiled_565_2(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_565_4:
        egui_view_test_performance_test_image_resize_tiled_565_4(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_565_8:
        egui_view_test_performance_test_image_resize_tiled_565_8(self);
        break;

    // Tiled resize (160x160 tiles, star)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_STAR_565_0:
        egui_view_test_performance_test_image_resize_tiled_star_565_0(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_STAR_565_1:
        egui_view_test_performance_test_image_resize_tiled_star_565_1(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_STAR_565_2:
        egui_view_test_performance_test_image_resize_tiled_star_565_2(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_STAR_565_4:
        egui_view_test_performance_test_image_resize_tiled_star_565_4(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_STAR_565_8:
        egui_view_test_performance_test_image_resize_tiled_star_565_8(self);
        break;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_TILED_565_0:
        egui_view_test_performance_test_extern_image_resize_tiled_565_0(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_TILED_565_1:
        egui_view_test_performance_test_extern_image_resize_tiled_565_1(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_TILED_565_2:
        egui_view_test_performance_test_extern_image_resize_tiled_565_2(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_TILED_565_4:
        egui_view_test_performance_test_extern_image_resize_tiled_565_4(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_TILED_565_8:
        egui_view_test_performance_test_extern_image_resize_tiled_565_8(self);
        break;
#endif

    // Tiled rotate (80x80 slots 45deg, test_perf)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_565_0:
        egui_view_test_performance_test_image_rotate_tiled_565_0(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_565_1:
        egui_view_test_performance_test_image_rotate_tiled_565_1(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_565_2:
        egui_view_test_performance_test_image_rotate_tiled_565_2(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_565_4:
        egui_view_test_performance_test_image_rotate_tiled_565_4(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_565_8:
        egui_view_test_performance_test_image_rotate_tiled_565_8(self);
        break;

    // Tiled rotate (80x80 slots 45deg, star)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_STAR_565_0:
        egui_view_test_performance_test_image_rotate_tiled_star_565_0(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_STAR_565_1:
        egui_view_test_performance_test_image_rotate_tiled_star_565_1(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_STAR_565_2:
        egui_view_test_performance_test_image_rotate_tiled_star_565_2(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_STAR_565_4:
        egui_view_test_performance_test_image_rotate_tiled_star_565_4(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_STAR_565_8:
        egui_view_test_performance_test_image_rotate_tiled_star_565_8(self);
        break;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_TILED_565_0:
        egui_view_test_performance_test_extern_image_rotate_tiled_565_0(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_TILED_565_1:
        egui_view_test_performance_test_extern_image_rotate_tiled_565_1(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_TILED_565_2:
        egui_view_test_performance_test_extern_image_rotate_tiled_565_2(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_TILED_565_4:
        egui_view_test_performance_test_extern_image_rotate_tiled_565_4(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_TILED_565_8:
        egui_view_test_performance_test_extern_image_rotate_tiled_565_8(self);
        break;
#endif

    // Text transform (rotation)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_ROTATE_NONE:
        egui_view_test_performance_test_text_rotate_none(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_ROTATE:
        egui_view_test_performance_test_text_rotate(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_ROTATE_RESIZE:
        egui_view_test_performance_test_text_rotate_resize(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_ROTATE_QUARTER:
        egui_view_test_performance_test_text_rotate_quarter(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_ROTATE_DOUBLE:
        egui_view_test_performance_test_text_rotate_double(self);
        break;

        // External text transform (rotation)
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT_ROTATE:
        egui_view_test_performance_test_extern_text_rotate(self);
        break;
#endif

    // Text with gradient overlay
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_GRADIENT:
        egui_view_test_performance_test_text_gradient(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_RECT_GRADIENT:
        egui_view_test_performance_test_text_rect_gradient(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_ROTATE_GRADIENT:
        egui_view_test_performance_test_text_rotate_gradient(self);
        break;

        // Compressed image tests (QOI)
#if EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_QOI_565:
        egui_view_test_performance_test_image_qoi_565(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_QOI_565_8:
        egui_view_test_performance_test_image_qoi_565_8(self);
        break;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_QOI_565:
        egui_view_test_performance_test_extern_image_qoi_565(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_QOI_565_8:
        egui_view_test_performance_test_extern_image_qoi_565_8(self);
        break;
#endif
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_QOI_565_0:
        egui_view_test_performance_test_image_tiled_qoi_565_0(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_QOI_565_8:
        egui_view_test_performance_test_image_tiled_qoi_565_8(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_NO_MASK:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_qoi_no_mask(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_ROUND_RECT:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_qoi_round_rect(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_CIRCLE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_qoi_circle(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_IMAGE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_qoi_image(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_8_NO_MASK:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_qoi_8_no_mask(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_8_ROUND_RECT:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_qoi_8_round_rect(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_8_CIRCLE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_qoi_8_circle(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_8_IMAGE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_qoi_8_image(self);
        break;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_NO_MASK:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_qoi_no_mask(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_ROUND_RECT:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_qoi_round_rect(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_CIRCLE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_qoi_circle(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_IMAGE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_qoi_image(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_NO_MASK:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_qoi_8_no_mask(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_qoi_8_round_rect(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_CIRCLE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_qoi_8_circle(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_IMAGE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_qoi_8_image(self);
        break;
#endif
#endif // EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE

        // Compressed image tests (RLE)
#if EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RLE_565:
        egui_view_test_performance_test_image_rle_565(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RLE_565_8:
        egui_view_test_performance_test_image_rle_565_8(self);
        break;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RLE_565:
        egui_view_test_performance_test_extern_image_rle_565(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RLE_565_8:
        egui_view_test_performance_test_extern_image_rle_565_8(self);
        break;
#endif
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_RLE_565_0:
        egui_view_test_performance_test_image_tiled_rle_565_0(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_RLE_565_8:
        egui_view_test_performance_test_image_tiled_rle_565_8(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_NO_MASK:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_rle_no_mask(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_ROUND_RECT:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_rle_round_rect(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_CIRCLE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_rle_circle(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_IMAGE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_rle_image(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_8_NO_MASK:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_rle_8_no_mask(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_8_ROUND_RECT:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_rle_8_round_rect(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_8_CIRCLE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_rle_8_circle(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_8_IMAGE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_mask_image_rle_8_image(self);
        break;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_NO_MASK:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_rle_no_mask(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_ROUND_RECT:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_rle_round_rect(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_CIRCLE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_rle_circle(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_IMAGE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_rle_image(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_NO_MASK:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_rle_8_no_mask(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_rle_8_round_rect(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_CIRCLE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_rle_8_circle(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_IMAGE:
        ensure_perf_masks_initialized();
        egui_view_test_performance_test_extern_mask_image_rle_8_image(self);
        break;
#endif
#endif // EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE

    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_FILE_IMAGE_JPG:
#if EGUI_CONFIG_FUNCTION_IMAGE_FILE
        perf_draw_file_image_scene(self, view->test_mode);
#endif
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_FILE_IMAGE_PNG:
#if EGUI_CONFIG_FUNCTION_IMAGE_FILE
        perf_draw_file_image_scene(self, view->test_mode);
#endif
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_FILE_IMAGE_BMP:
#if EGUI_CONFIG_FUNCTION_IMAGE_FILE
        perf_draw_file_image_scene(self, view->test_mode);
#endif
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CHART_LINE_DENSE:
        perf_draw_chart_line_scene(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CHART_BAR_DENSE:
        perf_draw_chart_bar_scene(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CHART_SCATTER_DENSE:
        perf_draw_chart_scatter_scene(self);
        break;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CHART_PIE_DENSE:
        perf_draw_chart_pie_scene(self);
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
    // Large image tests stay in the shipped benchmark set
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_8:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_1:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_2:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_8:
        return 1;

    // 480px alpha variant direct-draw �?disabled on QEMU to save flash (alpha=255, same perf as _0)
    // 240px alpha variant direct-draw (IMAGE_565_1/2/4) - always enabled
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_1:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_2:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_4:
        return 1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ELLIPSE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ELLIPSE_FILL:
        return 1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_POLYGON:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_POLYGON_FILL:
        return 1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_BEZIER_QUAD:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_BEZIER_CUBIC:
        return 1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE_HQ:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE_FILL_HQ:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ARC_HQ:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ARC_FILL_HQ:
        return 1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_LINE_HQ:
        return 1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_TRIANGLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ARC_RING:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ARC_RING_ROUND_CAP:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_RADIAL:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ANGULAR:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ROUND_RECT_RING:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_LINE_CAPSULE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_MULTI_STOP:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ROUND_RECT_CORNERS:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_GRADIENT_RECT_FILL:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_GRADIENT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_RECT_GRADIENT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_ROTATE_GRADIENT:
        return 1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_GRADIENT_IMAGE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_GRADIENT_IMAGE_ROTATE:
        return 1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_GRADIENT_OVERLAY:
        return 1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_COLOR:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_COLOR:
        return 1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_SHADOW:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_SHADOW_ROUND:
#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
        return 1;
#else
        return 0;
#endif

    // Image multi-size scenes stay enabled
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_QUARTER:
        return 1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_DOUBLE:
        return 1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_8_QUARTER:
        return 1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_8_DOUBLE:
        return 1;

    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_INTERNAL_TEXT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_INTERNAL_TEXT_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_INTERNAL_TEXT_RLE4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_INTERNAL_TEXT_RECT_RLE4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_INTERNAL_TEXT_RLE4_XOR:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_INTERNAL_TEXT_RECT_RLE4_XOR:
        return 1;

    // Extern image tests: gate on external resource support, skip on QEMU (no semihosting file support)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT_RLE4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT_RECT_RLE4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT_RLE4_XOR:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT_RECT_RLE4_XOR:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_565:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_565_1:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_565_2:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_565_4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_565_8:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_565:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_565_1:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_565_2:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_565_4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_565_8:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_NO_MASK:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_IMAGE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_TEST_PERF_NO_MASK:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_TEST_PERF_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_TEST_PERF_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_TEST_PERF_IMAGE:
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        return 1;
#else
        return 0;
#endif

    // Image transform scenes stay enabled
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_1:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_2:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_8:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_RESIZE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_QUARTER:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_565_DOUBLE:
        return 1;

    // External image transform: gate on image transform + external resource
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_565:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_565_1:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_565_2:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_565_4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_565_8:
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        return 1;
#else
        return 0;
#endif

    // Star image tests: gate on image config
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_STAR_565_1:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_STAR_565_2:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_STAR_565_4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_STAR_565_8:
        return 1;

    // Star image transform: gate on image transform + image config
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_STAR_565_1:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_STAR_565_2:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_STAR_565_4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_STAR_565_8:
        return 1;

    // Tiled draw (40x40): always enabled (40x40 images are flash-trivial)
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_565_0:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_565_1:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_565_2:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_565_4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_565_8:
        return 1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_STAR_565_0:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_STAR_565_1:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_STAR_565_2:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_STAR_565_4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_STAR_565_8:
        return 1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_TILED_565_0:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_TILED_565_1:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_TILED_565_2:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_TILED_565_4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_TILED_565_8:
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        return 1;
#else
        return 0;
#endif

    // Tiled resize (40->160): always enabled
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_565_0:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_565_1:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_565_2:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_565_4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_565_8:
        return 1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_STAR_565_0:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_STAR_565_1:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_STAR_565_2:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_STAR_565_4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_TILED_STAR_565_8:
        return 1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_TILED_565_0:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_TILED_565_1:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_TILED_565_2:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_TILED_565_4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RESIZE_TILED_565_8:
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        return 1;
#else
        return 0;
#endif

    // Tiled rotate (40x80 slot 45deg): always enabled when transform supported
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_565_0:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_565_1:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_565_2:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_565_4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_565_8:
        return 1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_STAR_565_0:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_STAR_565_1:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_STAR_565_2:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_STAR_565_4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_ROTATE_TILED_STAR_565_8:
        return 1;
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_TILED_565_0:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_TILED_565_1:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_TILED_565_2:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_TILED_565_4:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_ROTATE_TILED_565_8:
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        return 1;
#else
        return 0;
#endif

    // Text transform: gate on image transform support
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_ROTATE_NONE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_ROTATE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_ROTATE_RESIZE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_ROTATE_QUARTER:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_ROTATE_DOUBLE:
        return 1;

    // External text transform: gate on image transform + external resource
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_TEXT_ROTATE:
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        return 1;
#else
        return 0;
#endif

    // Compressed image tests (QOI): gate on codec enable
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_QOI_565:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_QOI_565_8:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_QOI_565:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_QOI_565_8:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_QOI_565_0:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_QOI_565_8:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_NO_MASK:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_IMAGE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_8_NO_MASK:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_8_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_8_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_QOI_8_IMAGE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_NO_MASK:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_IMAGE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_NO_MASK:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_IMAGE:
#if EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE && EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        return 1;
#elif EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE
        return test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_QOI_565 && test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_QOI_565_8 &&
               test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_NO_MASK &&
               test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_ROUND_RECT &&
               test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_CIRCLE &&
               test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_IMAGE &&
               test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_NO_MASK &&
               test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_ROUND_RECT &&
               test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_CIRCLE &&
               test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_QOI_8_IMAGE;
#else
        return 0;
#endif

    // Compressed image tests (RLE): gate on codec enable
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RLE_565:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RLE_565_8:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RLE_565:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RLE_565_8:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_RLE_565_0:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_TILED_RLE_565_8:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_NO_MASK:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_IMAGE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_8_NO_MASK:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_8_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_8_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_RLE_8_IMAGE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_NO_MASK:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_IMAGE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_NO_MASK:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_CIRCLE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_IMAGE:
#if EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE && EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        return 1;
#elif EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE
        return test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RLE_565 && test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_IMAGE_RLE_565_8 &&
               test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_NO_MASK &&
               test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_ROUND_RECT &&
               test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_CIRCLE &&
               test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_IMAGE &&
               test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_NO_MASK &&
               test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_ROUND_RECT &&
               test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_CIRCLE &&
               test_mode != EGUI_VIEW_TEST_PERFORMANCE_TYPE_EXTERN_MASK_IMAGE_RLE_8_IMAGE;
#else
        return 0;
#endif

    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_FILE_IMAGE_JPG:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_FILE_IMAGE_PNG:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_FILE_IMAGE_BMP:
#if EGUI_CONFIG_FUNCTION_IMAGE_FILE
        return 1;
#else
        return 0;
#endif
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CHART_LINE_DENSE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CHART_BAR_DENSE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CHART_SCATTER_DENSE:
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CHART_PIE_DENSE:
        return 1;

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
