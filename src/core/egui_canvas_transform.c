#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_canvas.h"
#include "egui_api.h"
#include "image/egui_image_std.h"
#include "font/egui_font.h"
#include "font/egui_font_std.h"

/* Q15 quarter sine table: sin_q15[i] = round(sin(i deg) * 32767), i = 0..90 */
static const int16_t transform_sin_q15_lut[91] = {
        0,     572,   1144,  1715,  2286,  2856,  3425,  3993,  4560,  5126,  5690,  6252,  6813,  7371,  7927,  8481,  9032,  9580,  10126,
        10668, 11207, 11743, 12275, 12803, 13328, 13848, 14365, 14876, 15384, 15886, 16384, 16877, 17364, 17847, 18324, 18795, 19261, 19720,
        20174, 20622, 21063, 21498, 21926, 22348, 22763, 23170, 23571, 23965, 24351, 24730, 25102, 25466, 25822, 26170, 26510, 26842, 27166,
        27482, 27789, 28088, 28378, 28660, 28932, 29197, 29452, 29698, 29935, 30163, 30382, 30592, 30792, 30983, 31164, 31336, 31499, 31651,
        31795, 31928, 32052, 32167, 32270, 32365, 32449, 32524, 32588, 32643, 32689, 32724, 32749, 32764, 32767,
};

static int32_t transform_sin_q15(int32_t deg)
{
    deg = ((deg % 360) + 360) % 360;
    int32_t sign = 1;
    if (deg > 180)
    {
        deg = 360 - deg;
        sign = -1;
    }
    if (deg > 90)
    {
        deg = 180 - deg;
    }
    return transform_sin_q15_lut[deg] * sign;
}

static int32_t transform_cos_q15(int32_t deg)
{
    return transform_sin_q15(deg + 90);
}

static int16_t transform_normalize_angle_deg(int32_t deg)
{
    deg %= 360;
    if (deg < 0)
    {
        deg += 360;
    }
    return (int16_t)deg;
}

static int16_t transform_sanitize_scale_q8(int16_t scale_q8)
{
    return scale_q8 > 0 ? scale_q8 : 256;
}

static int transform_prepare_arc_sweep_range(int16_t start_angle, int16_t sweep_angle, int16_t *draw_start, int16_t *draw_end)
{
    int32_t start = start_angle;
    int32_t sweep = sweep_angle;

    if (sweep == 0 || draw_start == NULL || draw_end == NULL)
    {
        return 0;
    }

    if (sweep > 360)
    {
        sweep = 360;
    }
    else if (sweep < -360)
    {
        sweep = -360;
    }

    if (sweep < 0)
    {
        start += sweep;
        sweep = -sweep;
    }

    start = transform_normalize_angle_deg(start);
    *draw_start = (int16_t)start;
    *draw_end = (int16_t)(start + sweep);
    return 1;
}

static void transform_compute_rotated_offset(int32_t dx, int32_t dy, int16_t angle_deg, int16_t scale_q8, int32_t *out_dx, int32_t *out_dy)
{
    int32_t sinA = transform_sin_q15(angle_deg);
    int32_t cosA = transform_cos_q15(angle_deg);
    int16_t scale = transform_sanitize_scale_q8(scale_q8);
    int32_t fwd_m00 = (cosA * scale) >> 8;
    int32_t fwd_m01 = (-sinA * scale) >> 8;
    int32_t fwd_m10 = (sinA * scale) >> 8;
    int32_t fwd_m11 = (cosA * scale) >> 8;

    if (out_dx != NULL)
    {
        *out_dx = (int32_t)((((int64_t)dx * fwd_m00) + ((int64_t)dy * fwd_m01) + (1 << 14)) >> 15);
    }
    if (out_dy != NULL)
    {
        *out_dy = (int32_t)((((int64_t)dx * fwd_m10) + ((int64_t)dy * fwd_m11) + (1 << 14)) >> 15);
    }
}

static void transform_compute_center_from_anchor_pivot(egui_dim_t anchor_x, egui_dim_t anchor_y, egui_dim_t src_w, egui_dim_t src_h, egui_dim_t pivot_x,
                                                       egui_dim_t pivot_y, int16_t angle_deg, int16_t scale_q8, egui_dim_t *center_x, egui_dim_t *center_y)
{
    int32_t dx = (int32_t)pivot_x - ((int32_t)src_w / 2);
    int32_t dy = (int32_t)pivot_y - ((int32_t)src_h / 2);
    int32_t rotated_dx = 0;
    int32_t rotated_dy = 0;

    transform_compute_rotated_offset(dx, dy, angle_deg, scale_q8, &rotated_dx, &rotated_dy);

    if (center_x != NULL)
    {
        *center_x = (egui_dim_t)((int32_t)anchor_x + (int32_t)pivot_x - rotated_dx);
    }
    if (center_y != NULL)
    {
        *center_y = (egui_dim_t)((int32_t)anchor_y + (int32_t)pivot_y - rotated_dy);
    }
}

static int transform_get_image_size(const egui_image_t *img, egui_dim_t *width, egui_dim_t *height)
{
    const egui_image_std_info_t *info;

    if (img == NULL || img->res == NULL)
    {
        return 0;
    }

    info = (const egui_image_std_info_t *)img->res;
    if (info->width == 0 || info->height == 0)
    {
        return 0;
    }

    if (width != NULL)
    {
        *width = (egui_dim_t)info->width;
    }
    if (height != NULL)
    {
        *height = (egui_dim_t)info->height;
    }
    return 1;
}

static int transform_measure_text_box(const egui_font_t *font, const void *string, egui_dim_t *width, egui_dim_t *height)
{
    egui_dim_t text_width = 0;
    egui_dim_t text_height = 0;

    if (font == NULL || string == NULL || font->api == NULL || font->api->get_str_size == NULL)
    {
        return 0;
    }

    if (font->api->get_str_size(font, string, 1, 0, &text_width, &text_height) != 0)
    {
        return 0;
    }

    if (text_width <= 0 || text_height <= 0)
    {
        return 0;
    }

    if (width != NULL)
    {
        *width = text_width;
    }
    if (height != NULL)
    {
        *height = text_height;
    }
    return 1;
}

void egui_canvas_draw_arc_sweep(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t sweep_angle, egui_dim_t stroke_width,
                                egui_color_t color, egui_alpha_t alpha)
{
    int16_t draw_start;
    int16_t draw_end;

    if (!transform_prepare_arc_sweep_range(start_angle, sweep_angle, &draw_start, &draw_end))
    {
        return;
    }

    egui_canvas_draw_arc(center_x, center_y, radius, draw_start, draw_end, stroke_width, color, alpha);
}

void egui_canvas_draw_arc_fill_sweep(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t sweep_angle, egui_color_t color,
                                     egui_alpha_t alpha)
{
    int16_t draw_start;
    int16_t draw_end;

    if (!transform_prepare_arc_sweep_range(start_angle, sweep_angle, &draw_start, &draw_end))
    {
        return;
    }

    egui_canvas_draw_arc_fill(center_x, center_y, radius, draw_start, draw_end, color, alpha);
}

void egui_canvas_draw_arc_round_cap_sweep_hq(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t sweep_angle,
                                             egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha)
{
    int16_t draw_start;
    int16_t draw_end;

    if (!transform_prepare_arc_sweep_range(start_angle, sweep_angle, &draw_start, &draw_end))
    {
        return;
    }

    egui_canvas_draw_arc_round_cap_hq(center_x, center_y, radius, draw_start, draw_end, stroke_width, color, alpha);
}

static void transform_draw_image_rotate_pivot_impl(const egui_image_t *img, egui_dim_t x, egui_dim_t y, egui_dim_t pivot_x, egui_dim_t pivot_y,
                                                   int16_t angle_deg, int16_t scale_q8)
{
    egui_dim_t image_w;
    egui_dim_t image_h;
    egui_dim_t center_x;
    egui_dim_t center_y;
    int16_t normalized_angle = transform_normalize_angle_deg(angle_deg);
    int16_t final_scale = transform_sanitize_scale_q8(scale_q8);

    if (img == NULL)
    {
        return;
    }

    if (normalized_angle == 0 && final_scale == 256)
    {
        egui_canvas_draw_image(img, x, y);
        return;
    }

    if (!transform_get_image_size(img, &image_w, &image_h))
    {
        return;
    }

    transform_compute_center_from_anchor_pivot(x, y, image_w, image_h, pivot_x, pivot_y, normalized_angle, final_scale, &center_x, &center_y);
    egui_canvas_draw_image_transform(img, center_x, center_y, normalized_angle, final_scale);
}

void egui_canvas_draw_image_rotate(const egui_image_t *img, egui_dim_t x, egui_dim_t y, int16_t angle_deg)
{
    egui_dim_t image_w;
    egui_dim_t image_h;

    if (!transform_get_image_size(img, &image_w, &image_h))
    {
        return;
    }

    transform_draw_image_rotate_pivot_impl(img, x, y, image_w / 2, image_h / 2, angle_deg, 256);
}

void egui_canvas_draw_image_rotate_scale(const egui_image_t *img, egui_dim_t x, egui_dim_t y, int16_t angle_deg, int16_t scale_q8)
{
    egui_dim_t image_w;
    egui_dim_t image_h;

    if (!transform_get_image_size(img, &image_w, &image_h))
    {
        return;
    }

    transform_draw_image_rotate_pivot_impl(img, x, y, image_w / 2, image_h / 2, angle_deg, scale_q8);
}

void egui_canvas_draw_image_rotate_pivot(const egui_image_t *img, egui_dim_t x, egui_dim_t y, egui_dim_t pivot_x, egui_dim_t pivot_y, int16_t angle_deg,
                                         int16_t scale_q8)
{
    transform_draw_image_rotate_pivot_impl(img, x, y, pivot_x, pivot_y, angle_deg, scale_q8);
}

static void transform_draw_text_rotate_pivot_impl(const egui_font_t *font, const void *string, egui_dim_t x, egui_dim_t y, egui_dim_t pivot_x,
                                                  egui_dim_t pivot_y, int16_t angle_deg, int16_t scale_q8, egui_color_t color, egui_alpha_t alpha)
{
    egui_dim_t text_w;
    egui_dim_t text_h;
    egui_dim_t center_x;
    egui_dim_t center_y;
    int16_t normalized_angle = transform_normalize_angle_deg(angle_deg);
    int16_t final_scale = transform_sanitize_scale_q8(scale_q8);

    if (font == NULL || string == NULL)
    {
        return;
    }

    if (normalized_angle == 0 && final_scale == 256)
    {
        egui_canvas_draw_text(font, string, x, y, color, alpha);
        return;
    }

    if (!transform_measure_text_box(font, string, &text_w, &text_h))
    {
        return;
    }

    transform_compute_center_from_anchor_pivot(x, y, text_w, text_h, pivot_x, pivot_y, normalized_angle, final_scale, &center_x, &center_y);
    egui_canvas_draw_text_transform(font, string, center_x, center_y, normalized_angle, final_scale, color, alpha);
}

void egui_canvas_draw_text_rotate(const egui_font_t *font, const void *string, egui_dim_t x, egui_dim_t y, int16_t angle_deg, egui_color_t color,
                                  egui_alpha_t alpha)
{
    egui_dim_t text_w;
    egui_dim_t text_h;

    if (!transform_measure_text_box(font, string, &text_w, &text_h))
    {
        return;
    }

    transform_draw_text_rotate_pivot_impl(font, string, x, y, text_w / 2, text_h / 2, angle_deg, 256, color, alpha);
}

void egui_canvas_draw_text_rotate_scale(const egui_font_t *font, const void *string, egui_dim_t x, egui_dim_t y, int16_t angle_deg, int16_t scale_q8,
                                        egui_color_t color, egui_alpha_t alpha)
{
    egui_dim_t text_w;
    egui_dim_t text_h;

    if (!transform_measure_text_box(font, string, &text_w, &text_h))
    {
        return;
    }

    transform_draw_text_rotate_pivot_impl(font, string, x, y, text_w / 2, text_h / 2, angle_deg, scale_q8, color, alpha);
}

void egui_canvas_draw_text_rotate_pivot(const egui_font_t *font, const void *string, egui_dim_t x, egui_dim_t y, egui_dim_t pivot_x, egui_dim_t pivot_y,
                                        int16_t angle_deg, int16_t scale_q8, egui_color_t color, egui_alpha_t alpha)
{
    transform_draw_text_rotate_pivot_impl(font, string, x, y, pivot_x, pivot_y, angle_deg, scale_q8, color, alpha);
}

/**
 * Bilinear interpolate 4 RGB565 pixels in packed domain.
 * Avoids 3 separate egui_rgb_mix calls (saves 2 pack/unpack + 6 branches).
 * 5-bit alpha
 * precision, visually identical to chained egui_rgb_mix.
 */
__EGUI_STATIC_INLINE__ uint16_t bilinear_rgb565_packed(uint16_t c00, uint16_t c01, uint16_t c10, uint16_t c11, uint8_t fx, uint8_t fy)
{
    uint32_t p00 = (c00 | ((uint32_t)c00 << 16)) & 0x07E0F81FUL;
    uint32_t p01 = (c01 | ((uint32_t)c01 << 16)) & 0x07E0F81FUL;
    uint32_t p10 = (c10 | ((uint32_t)c10 << 16)) & 0x07E0F81FUL;
    uint32_t p11 = (c11 | ((uint32_t)c11 << 16)) & 0x07E0F81FUL;
    uint32_t fa = (uint32_t)fx >> 3;
    uint32_t h0 = (p00 + ((p01 - p00) * fa >> 5)) & 0x07E0F81FUL;
    uint32_t h1 = (p10 + ((p11 - p10) * fa >> 5)) & 0x07E0F81FUL;
    uint32_t fb = (uint32_t)fy >> 3;
    uint32_t res = (h0 + ((h1 - h0) * fb >> 5)) & 0x07E0F81FUL;
    return (uint16_t)(res | (res >> 16));
}

/**
 * Compute the number of pixels to skip at the start of a scanline for rotation.
 *
 * For inverse-mapped rotation, pixels at the start (and end) of each output
 * scanline may map outside the source rectangle. The "entered + break" pattern
 * handles the right side efficiently, but left-side dead pixels are checked one
 * by one. This function computes an approximate skip count to jump past most of
 * the left dead zone, reducing per-pixel overhead in edge/corner PFB tiles.
 *
 * @param rotatedX  Q15 source X at first pixel of scanline
 * @param rotatedY  Q15 source Y at first pixel of scanline
 * @param inv_m00   Q15 per-pixel X step
 * @param inv_m10   Q15 per-pixel Y step
 * @param src_lo_x  Q15 lower bound for source X (typically -(1<<15))
 * @param src_hi_x  Q15 upper bound for source X (typically src_w << 15)
 * @param src_lo_y  Q15 lower bound for source Y (typically -(1<<15))
 * @param src_hi_y  Q15 upper bound for source Y (typically src_h << 15)
 * @param max_skip  Maximum allowed skip (draw_x1 - draw_x0)
 * @return Number of pixels to skip (0 if first pixel is already valid)
 */
static int32_t transform_scanline_skip(int32_t rotatedX, int32_t rotatedY, int32_t inv_m00, int32_t inv_m10, int32_t src_lo_x, int32_t src_hi_x,
                                       int32_t src_lo_y, int32_t src_hi_y, int32_t max_skip)
{
    /* Quick check: if first pixel is already in bounds, no skip needed */
    if (rotatedX >= src_lo_x && rotatedX < src_hi_x && rotatedY >= src_lo_y && rotatedY < src_hi_y)
    {
        return 0;
    }

    /* Compute skip for each out-of-bounds axis.
     * For axis with step > 0: skip = ceil((bound_lo - current) / step)
     * For axis with step < 0: skip = ceil((current - bound_hi + 1) / (-step))
     * Take the max of all skips. */
    int32_t skip = 0;

    /* X axis */
    if (inv_m00 > 0)
    {
        if (rotatedX < src_lo_x)
        {
            int32_t need = src_lo_x - rotatedX;
            int32_t s = (need + inv_m00 - 1) / inv_m00;
            if (s > skip)
                skip = s;
        }
    }
    else if (inv_m00 < 0)
    {
        if (rotatedX >= src_hi_x)
        {
            int32_t need = rotatedX - src_hi_x + 1;
            int32_t neg_m = -inv_m00;
            int32_t s = (need + neg_m - 1) / neg_m;
            if (s > skip)
                skip = s;
        }
    }

    /* Y axis */
    if (inv_m10 > 0)
    {
        if (rotatedY < src_lo_y)
        {
            int32_t need = src_lo_y - rotatedY;
            int32_t s = (need + inv_m10 - 1) / inv_m10;
            if (s > skip)
                skip = s;
        }
    }
    else if (inv_m10 < 0)
    {
        if (rotatedY >= src_hi_y)
        {
            int32_t need = rotatedY - src_hi_y + 1;
            int32_t neg_m = -inv_m10;
            int32_t s = (need + neg_m - 1) / neg_m;
            if (s > skip)
                skip = s;
        }
    }

    /* Clamp to maximum and subtract 1 for safety (rounding margin) */
    if (skip > 1)
    {
        skip -= 1;
    }
    if (skip > max_skip)
    {
        skip = max_skip;
    }

    return skip;
}

/**
 * Compute alpha buffer byte size for a given image dimension and alpha type.
 * Used by external resource loading and row-byte calculations.
 */
__EGUI_STATIC_INLINE__ uint32_t image_alpha_buf_size(int16_t w, int16_t h, uint8_t alpha_type)
{
    switch (alpha_type)
    {
    case EGUI_IMAGE_ALPHA_TYPE_8:
        return (uint32_t)w * h;
    case EGUI_IMAGE_ALPHA_TYPE_4:
        return (uint32_t)((w + 1) >> 1) * h;
    case EGUI_IMAGE_ALPHA_TYPE_2:
        return (uint32_t)((w + 3) >> 2) * h;
    case EGUI_IMAGE_ALPHA_TYPE_1:
        return (uint32_t)((w + 7) >> 3) * h;
    default:
        return 0;
    }
}

static int image_transform_get_alpha_row_bytes(int16_t w, uint8_t alpha_type)
{
    switch (alpha_type)
    {
    case EGUI_IMAGE_ALPHA_TYPE_8:
        return w;
    case EGUI_IMAGE_ALPHA_TYPE_4:
        return (w + 1) >> 1;
    case EGUI_IMAGE_ALPHA_TYPE_2:
        return (w + 3) >> 2;
    case EGUI_IMAGE_ALPHA_TYPE_1:
        return (w + 7) >> 3;
    default:
        return 0;
    }
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
#define EGUI_IMAGE_TRANSFORM_EXTERNAL_DATA_CACHE_MAX_BYTES  EGUI_CONFIG_IMAGE_EXTERNAL_DATA_CACHE_MAX_BYTES
#define EGUI_IMAGE_TRANSFORM_EXTERNAL_ALPHA_CACHE_MAX_BYTES EGUI_CONFIG_IMAGE_EXTERNAL_ALPHA_CACHE_MAX_BYTES

#ifndef EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE
#define EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE 1
#endif

typedef struct
{
    const egui_image_std_info_t *info;
    const void *src_addr;
    int32_t chunk_row_start;
    int32_t chunk_row_count;
    uint32_t row_bytes;
    uint32_t rows_per_chunk;
    uint32_t shared_generation;
} image_transform_external_data_row_slot_t;

typedef struct
{
    const egui_image_std_info_t *info;
    const void *src_addr;
    int32_t chunk_row_start;
    int32_t chunk_row_count;
    uint32_t row_bytes;
    uint32_t rows_per_chunk;
    uint32_t shared_generation;
} image_transform_external_alpha_row_slot_t;

typedef struct
{
    const egui_image_std_info_t *info;
    uint32_t data_row_bytes;
    uint32_t alpha_row_bytes;
#if !EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE
    image_transform_external_data_row_slot_t *data_cache;
    image_transform_external_alpha_row_slot_t *alpha_cache;
#endif
} image_transform_external_source_t;

#if EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE
static image_transform_external_data_row_slot_t g_image_transform_external_data_row_cache = {0};
static image_transform_external_alpha_row_slot_t g_image_transform_external_alpha_row_cache = {0};
#endif

static void image_transform_sync_external_row_cache_generation(uint32_t *shared_generation, int32_t *chunk_row_start, int32_t *chunk_row_count)
{
    uint32_t generation = egui_image_std_claim_shared_external_row_cache(EGUI_IMAGE_EXTERNAL_ROW_CACHE_OWNER_TRANSFORM);

    if (shared_generation != NULL && *shared_generation != generation)
    {
        if (chunk_row_start != NULL)
        {
            *chunk_row_start = -1;
        }
        if (chunk_row_count != NULL)
        {
            *chunk_row_count = 0;
        }
        *shared_generation = generation;
    }
}

static uint16_t *image_transform_get_external_data_cache_pixels(image_transform_external_data_row_slot_t *cache)
{
    EGUI_UNUSED(cache);
    return egui_image_std_get_shared_external_data_cache();
}

static uint8_t *image_transform_get_external_alpha_cache_bytes(image_transform_external_alpha_row_slot_t *cache)
{
    EGUI_UNUSED(cache);
    return egui_image_std_get_shared_external_alpha_cache();
}

static image_transform_external_data_row_slot_t *image_transform_get_external_data_row_cache(const image_transform_external_source_t *source)
{
#if EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE
    EGUI_UNUSED(source);
    return &g_image_transform_external_data_row_cache;
#else
    return source != NULL ? source->data_cache : NULL;
#endif
}

static image_transform_external_alpha_row_slot_t *image_transform_get_external_alpha_row_cache(const image_transform_external_source_t *source)
{
#if EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE
    EGUI_UNUSED(source);
    return &g_image_transform_external_alpha_row_cache;
#else
    return source != NULL ? source->alpha_cache : NULL;
#endif
}

static void image_transform_release_external_row_cache(void)
{
#if EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE
    egui_api_memset(&g_image_transform_external_data_row_cache, 0, sizeof(g_image_transform_external_data_row_cache));
    egui_api_memset(&g_image_transform_external_alpha_row_cache, 0, sizeof(g_image_transform_external_alpha_row_cache));
#endif
}

static int image_transform_prepare_external_source(const egui_image_std_info_t *info, int source_is_opaque, image_transform_external_source_t *source)
{
    if (info == NULL || source == NULL || info->width <= 0 || info->height <= 0)
    {
        return 0;
    }

    source->info = info;
    source->data_row_bytes = (uint32_t)info->width * sizeof(uint16_t);
    source->alpha_row_bytes = 0;
    if (info->alpha_buf != NULL && !source_is_opaque)
    {
        source->alpha_row_bytes = (uint32_t)image_transform_get_alpha_row_bytes(info->width, info->alpha_type);
    }

    image_transform_external_data_row_slot_t *data_cache = image_transform_get_external_data_row_cache(source);
    image_transform_external_alpha_row_slot_t *alpha_cache = image_transform_get_external_alpha_row_cache(source);

    image_transform_sync_external_row_cache_generation(&data_cache->shared_generation, &data_cache->chunk_row_start, &data_cache->chunk_row_count);
    image_transform_sync_external_row_cache_generation(&alpha_cache->shared_generation, &alpha_cache->chunk_row_start, &alpha_cache->chunk_row_count);
    return 1;
}

__EGUI_STATIC_INLINE__ int32_t image_transform_align_external_chunk_row(int32_t row, uint32_t rows_per_chunk)
{
    if (row <= 0 || rows_per_chunk <= 1)
    {
        return row;
    }

    return row - (row % (int32_t)rows_per_chunk);
}

__EGUI_STATIC_INLINE__ uint32_t image_transform_external_rows_per_chunk(uint32_t cache_max_bytes, uint32_t row_bytes)
{
    uint32_t rows_per_chunk;

    if (row_bytes == 0)
    {
        return 1;
    }

    rows_per_chunk = cache_max_bytes / row_bytes;
    return rows_per_chunk != 0 ? rows_per_chunk : 1;
}

__EGUI_STATIC_INLINE__ int image_transform_external_rows_share_chunk(int32_t row0, int32_t row1, uint32_t rows_per_chunk)
{
    return image_transform_align_external_chunk_row(row0, rows_per_chunk) == image_transform_align_external_chunk_row(row1, rows_per_chunk);
}

static const uint16_t *image_transform_get_external_data_row(const image_transform_external_source_t *source, int32_t row)
{
    image_transform_external_data_row_slot_t *cache = image_transform_get_external_data_row_cache(source);
    int32_t load_row;
    int32_t rows_to_load;
    uint16_t *pixels;

    if (source == NULL || source->info == NULL || row < 0 || row >= source->info->height || source->data_row_bytes == 0 ||
        source->data_row_bytes > EGUI_IMAGE_TRANSFORM_EXTERNAL_DATA_CACHE_MAX_BYTES)
    {
        return NULL;
    }

    pixels = image_transform_get_external_data_cache_pixels(cache);

    if (cache->row_bytes != source->data_row_bytes || cache->rows_per_chunk == 0)
    {
        cache->rows_per_chunk = EGUI_IMAGE_TRANSFORM_EXTERNAL_DATA_CACHE_MAX_BYTES / source->data_row_bytes;
        if (cache->rows_per_chunk == 0)
        {
            cache->rows_per_chunk = 1;
        }
    }

    if (cache->info != source->info || cache->src_addr != source->info->data_buf || cache->row_bytes != source->data_row_bytes)
    {
        cache->chunk_row_start = -1;
        cache->chunk_row_count = 0;
    }

    cache->info = source->info;
    cache->src_addr = source->info->data_buf;
    cache->row_bytes = source->data_row_bytes;

    if (row < cache->chunk_row_start || row >= (cache->chunk_row_start + cache->chunk_row_count))
    {
        load_row = image_transform_align_external_chunk_row(row, cache->rows_per_chunk);
        rows_to_load = source->info->height - load_row;
        if ((uint32_t)rows_to_load > cache->rows_per_chunk)
        {
            rows_to_load = (int32_t)cache->rows_per_chunk;
        }

        egui_api_load_external_resource(pixels, (egui_uintptr_t)source->info->data_buf, (uint32_t)load_row * source->data_row_bytes,
                                        (uint32_t)rows_to_load * source->data_row_bytes);
        cache->chunk_row_start = load_row;
        cache->chunk_row_count = rows_to_load;
    }

    return (const uint16_t *)((const uint8_t *)pixels + (uint32_t)(row - cache->chunk_row_start) * cache->row_bytes);
}

static const uint8_t *image_transform_get_external_alpha_row(const image_transform_external_source_t *source, int32_t row)
{
    image_transform_external_alpha_row_slot_t *cache = image_transform_get_external_alpha_row_cache(source);
    int32_t load_row;
    int32_t rows_to_load;
    uint8_t *bytes;

    if (source == NULL || source->info == NULL || source->info->alpha_buf == NULL || row < 0 || row >= source->info->height || source->alpha_row_bytes == 0 ||
        source->alpha_row_bytes > EGUI_IMAGE_TRANSFORM_EXTERNAL_ALPHA_CACHE_MAX_BYTES)
    {
        return NULL;
    }

    bytes = image_transform_get_external_alpha_cache_bytes(cache);

    if (cache->row_bytes != source->alpha_row_bytes || cache->rows_per_chunk == 0)
    {
        cache->rows_per_chunk = EGUI_IMAGE_TRANSFORM_EXTERNAL_ALPHA_CACHE_MAX_BYTES / source->alpha_row_bytes;
        if (cache->rows_per_chunk == 0)
        {
            cache->rows_per_chunk = 1;
        }
    }

    if (cache->info != source->info || cache->src_addr != source->info->alpha_buf || cache->row_bytes != source->alpha_row_bytes)
    {
        cache->chunk_row_start = -1;
        cache->chunk_row_count = 0;
    }

    cache->info = source->info;
    cache->src_addr = source->info->alpha_buf;
    cache->row_bytes = source->alpha_row_bytes;

    if (row < cache->chunk_row_start || row >= (cache->chunk_row_start + cache->chunk_row_count))
    {
        load_row = image_transform_align_external_chunk_row(row, cache->rows_per_chunk);
        rows_to_load = source->info->height - load_row;
        if ((uint32_t)rows_to_load > cache->rows_per_chunk)
        {
            rows_to_load = (int32_t)cache->rows_per_chunk;
        }

        egui_api_load_external_resource(bytes, (egui_uintptr_t)source->info->alpha_buf, (uint32_t)load_row * source->alpha_row_bytes,
                                        (uint32_t)rows_to_load * source->alpha_row_bytes);
        cache->chunk_row_start = load_row;
        cache->chunk_row_count = rows_to_load;
    }

    return bytes + (uint32_t)(row - cache->chunk_row_start) * cache->row_bytes;
}

__EGUI_STATIC_INLINE__ uint16_t image_transform_read_rgb565_external(const image_transform_external_source_t *source, int32_t x, int32_t y)
{
    const uint16_t *row = image_transform_get_external_data_row(source, y);

    if (row != NULL)
    {
        return row[x];
    }

    {
        uint16_t pixel = 0;
        uint32_t offset = ((uint32_t)y * (uint32_t)source->info->width + (uint32_t)x) * sizeof(uint16_t);
        egui_api_load_external_resource(&pixel, (egui_uintptr_t)source->info->data_buf, offset, sizeof(pixel));
        return pixel;
    }
}

__EGUI_STATIC_INLINE__ void image_transform_fetch_bilinear_rgb565_external(const image_transform_external_source_t *source, int32_t x, int32_t y, uint16_t *d00,
                                                                           uint16_t *d01, uint16_t *d10, uint16_t *d11)
{
    uint32_t rows_per_chunk = image_transform_external_rows_per_chunk(EGUI_IMAGE_TRANSFORM_EXTERNAL_DATA_CACHE_MAX_BYTES, source->data_row_bytes);
    const uint16_t *row0 = NULL;
    const uint16_t *row1 = NULL;

    /* The external transform path keeps one chunk buffer per source plane.
     * When y and y + 1 cross a chunk boundary, fetching row1 would reload the
     * shared buffer and invalidate row0. Fall back to point loads there so the
     * bilinear sample still uses the correct source rows. */
    if (image_transform_external_rows_share_chunk(y, y + 1, rows_per_chunk))
    {
        row0 = image_transform_get_external_data_row(source, y);
        row1 = image_transform_get_external_data_row(source, y + 1);

        if (row0 != NULL && row1 != NULL)
        {
            *d00 = row0[x];
            *d01 = row0[x + 1];
            *d10 = row1[x];
            *d11 = row1[x + 1];
            return;
        }
    }

    *d00 = image_transform_read_rgb565_external(source, x, y);
    *d01 = image_transform_read_rgb565_external(source, x + 1, y);
    *d10 = image_transform_read_rgb565_external(source, x, y + 1);
    *d11 = image_transform_read_rgb565_external(source, x + 1, y + 1);
}
#endif

/**
 * Read a single alpha value from a packed image alpha buffer at (x, y).
 * Supports all alpha types (1/2/4/8 bits per pixel).
 * The alpha_row_bytes parameter is pre-computed for the specific alpha_type.
 */
static inline uint8_t image_transform_read_alpha_from_row(const uint8_t *row, int x, uint8_t alpha_type)
{
    switch (alpha_type)
    {
    case EGUI_IMAGE_ALPHA_TYPE_8:
        return row[x];
    case EGUI_IMAGE_ALPHA_TYPE_4:
        return egui_alpha_change_table_4[(row[x >> 1] >> ((x & 1) << 2)) & 0x0F];
    case EGUI_IMAGE_ALPHA_TYPE_2:
        return egui_alpha_change_table_2[(row[x >> 2] >> ((x & 3) << 1)) & 0x03];
    case EGUI_IMAGE_ALPHA_TYPE_1:
        return (row[x >> 3] >> (x & 7)) & 1 ? 255 : 0;
    default:
        return 0;
    }
}

static inline uint8_t image_transform_read_alpha(const uint8_t *alpha_buf, int alpha_row_bytes, int x, int y, uint8_t alpha_type)
{
    const uint8_t *row = alpha_buf + y * alpha_row_bytes;
    return image_transform_read_alpha_from_row(row, x, alpha_type);
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static inline uint8_t image_transform_read_alpha_external(const image_transform_external_source_t *source, int32_t x, int32_t y)
{
    const uint8_t *row = image_transform_get_external_alpha_row(source, y);

    if (row != NULL)
    {
        return image_transform_read_alpha_from_row(row, x, source->info->alpha_type);
    }

    switch (source->info->alpha_type)
    {
    case EGUI_IMAGE_ALPHA_TYPE_8:
    {
        uint8_t value = 0;
        uint32_t offset = (uint32_t)y * source->alpha_row_bytes + (uint32_t)x;
        egui_api_load_external_resource(&value, (egui_uintptr_t)source->info->alpha_buf, offset, sizeof(value));
        return value;
    }
    case EGUI_IMAGE_ALPHA_TYPE_4:
    {
        uint8_t packed = 0;
        uint32_t offset = (uint32_t)y * source->alpha_row_bytes + ((uint32_t)x >> 1);
        egui_api_load_external_resource(&packed, (egui_uintptr_t)source->info->alpha_buf, offset, sizeof(packed));
        return egui_alpha_change_table_4[(packed >> ((x & 1) << 2)) & 0x0F];
    }
    case EGUI_IMAGE_ALPHA_TYPE_2:
    {
        uint8_t packed = 0;
        uint32_t offset = (uint32_t)y * source->alpha_row_bytes + ((uint32_t)x >> 2);
        egui_api_load_external_resource(&packed, (egui_uintptr_t)source->info->alpha_buf, offset, sizeof(packed));
        return egui_alpha_change_table_2[(packed >> ((x & 3) << 1)) & 0x03];
    }
    case EGUI_IMAGE_ALPHA_TYPE_1:
    {
        uint8_t packed = 0;
        uint32_t offset = (uint32_t)y * source->alpha_row_bytes + ((uint32_t)x >> 3);
        egui_api_load_external_resource(&packed, (egui_uintptr_t)source->info->alpha_buf, offset, sizeof(packed));
        return ((packed >> (x & 7)) & 1) ? 255 : 0;
    }
    default:
        return 0;
    }
}
#endif

static inline uint8_t image_transform_bilinear_alpha_from_raw_4(uint8_t a00, uint8_t a01, uint8_t a10, uint8_t a11, uint8_t fx, uint8_t fy)
{
    int32_t h0_q8 = ((int32_t)a00 << 8) + ((int32_t)(a01 - a00) * fx);
    int32_t h1_q8 = ((int32_t)a10 << 8) + ((int32_t)(a11 - a10) * fx);
    uint16_t ah0 = (uint16_t)(((h0_q8 << 4) + h0_q8 + 128) >> 8);
    uint16_t ah1 = (uint16_t)(((h1_q8 << 4) + h1_q8 + 128) >> 8);

    return (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));
}

static inline uint8_t image_transform_bilinear_alpha_from_raw_2(uint8_t a00, uint8_t a01, uint8_t a10, uint8_t a11, uint8_t fx, uint8_t fy)
{
    int32_t h0_q8 = ((int32_t)a00 << 8) + ((int32_t)(a01 - a00) * fx);
    int32_t h1_q8 = ((int32_t)a10 << 8) + ((int32_t)(a11 - a10) * fx);
    uint16_t ah0 = (uint16_t)(((h0_q8 << 6) + (h0_q8 << 4) + (h0_q8 << 2) + h0_q8 + 128) >> 8);
    uint16_t ah1 = (uint16_t)(((h1_q8 << 6) + (h1_q8 << 4) + (h1_q8 << 2) + h1_q8 + 128) >> 8);

    return (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));
}

static inline uint8_t image_transform_bilinear_alpha_from_raw_1(uint8_t a00, uint8_t a01, uint8_t a10, uint8_t a11, uint8_t fx, uint8_t fy)
{
    int32_t h0_q8 = ((int32_t)a00 << 8) + ((int32_t)(a01 - a00) * fx);
    int32_t h1_q8 = ((int32_t)a10 << 8) + ((int32_t)(a11 - a10) * fx);
    uint16_t ah0 = (uint16_t)(((h0_q8 << 8) - h0_q8 + 128) >> 8);
    uint16_t ah1 = (uint16_t)(((h1_q8 << 8) - h1_q8 + 128) >> 8);

    return (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));
}

static inline uint8_t image_transform_sample_alpha_bilinear_from_rows(const uint8_t *row0, const uint8_t *row1, int x, uint8_t alpha_type, uint8_t fx,
                                                                      uint8_t fy)
{
    switch (alpha_type)
    {
    case EGUI_IMAGE_ALPHA_TYPE_4:
    {
        uint8_t a00, a01, a10, a11;
        int idx = x >> 1;

        if ((x & 1) == 0)
        {
            uint8_t packed0 = row0[idx];
            uint8_t packed1 = row1[idx];

            if ((packed0 | packed1) == 0)
            {
                return 0;
            }

            if ((packed0 & packed1) == 0xFF)
            {
                return EGUI_ALPHA_100;
            }

            a00 = packed0 & 0x0F;
            a01 = (packed0 >> 4) & 0x0F;
            a10 = packed1 & 0x0F;
            a11 = (packed1 >> 4) & 0x0F;
        }
        else
        {
            uint8_t packed00 = row0[idx];
            uint8_t packed01 = row0[idx + 1];
            uint8_t packed10 = row1[idx];
            uint8_t packed11 = row1[idx + 1];

            a00 = (packed00 >> 4) & 0x0F;
            a01 = packed01 & 0x0F;
            a10 = (packed10 >> 4) & 0x0F;
            a11 = packed11 & 0x0F;

            if ((a00 | a01 | a10 | a11) == 0)
            {
                return 0;
            }

            if ((a00 & a01 & a10 & a11) == 0x0F)
            {
                return EGUI_ALPHA_100;
            }
        }

        return image_transform_bilinear_alpha_from_raw_4(a00, a01, a10, a11, fx, fy);
    }
    case EGUI_IMAGE_ALPHA_TYPE_2:
    {
        uint8_t a00, a01, a10, a11;
        int idx = x >> 2;
        int local_x = x & 3;

        if (local_x != 3)
        {
            int shift = local_x << 1;
            uint8_t pair0 = (row0[idx] >> shift) & 0x0F;
            uint8_t pair1 = (row1[idx] >> shift) & 0x0F;

            if ((pair0 | pair1) == 0)
            {
                return 0;
            }

            if ((pair0 & pair1) == 0x0F)
            {
                return EGUI_ALPHA_100;
            }

            a00 = pair0 & 0x03;
            a01 = (pair0 >> 2) & 0x03;
            a10 = pair1 & 0x03;
            a11 = (pair1 >> 2) & 0x03;
        }
        else
        {
            uint8_t packed00 = row0[idx];
            uint8_t packed01 = row0[idx + 1];
            uint8_t packed10 = row1[idx];
            uint8_t packed11 = row1[idx + 1];

            a00 = (packed00 >> 6) & 0x03;
            a01 = packed01 & 0x03;
            a10 = (packed10 >> 6) & 0x03;
            a11 = packed11 & 0x03;

            if ((a00 | a01 | a10 | a11) == 0)
            {
                return 0;
            }

            if ((a00 & a01 & a10 & a11) == 0x03)
            {
                return EGUI_ALPHA_100;
            }
        }

        return image_transform_bilinear_alpha_from_raw_2(a00, a01, a10, a11, fx, fy);
    }
    case EGUI_IMAGE_ALPHA_TYPE_1:
    {
        uint8_t a00, a01, a10, a11;
        int idx = x >> 3;
        int bit = x & 7;

        if (bit != 7)
        {
            uint8_t pair0 = (row0[idx] >> bit) & 0x03;
            uint8_t pair1 = (row1[idx] >> bit) & 0x03;

            if ((pair0 | pair1) == 0)
            {
                return 0;
            }

            if ((pair0 & pair1) == 0x03)
            {
                return EGUI_ALPHA_100;
            }

            a00 = pair0 & 0x01;
            a01 = (pair0 >> 1) & 0x01;
            a10 = pair1 & 0x01;
            a11 = (pair1 >> 1) & 0x01;
        }
        else
        {
            uint8_t packed00 = row0[idx];
            uint8_t packed01 = row0[idx + 1];
            uint8_t packed10 = row1[idx];
            uint8_t packed11 = row1[idx + 1];

            a00 = (packed00 >> 7) & 0x01;
            a01 = packed01 & 0x01;
            a10 = (packed10 >> 7) & 0x01;
            a11 = packed11 & 0x01;

            if ((a00 | a01 | a10 | a11) == 0)
            {
                return 0;
            }

            if ((a00 & a01 & a10 & a11) == 0x01)
            {
                return EGUI_ALPHA_100;
            }
        }

        return image_transform_bilinear_alpha_from_raw_1(a00, a01, a10, a11, fx, fy);
    }
    case EGUI_IMAGE_ALPHA_TYPE_8:
    default:
    {
        uint16_t a00 = row0[x];
        uint16_t a01 = row0[x + 1];
        uint16_t a10 = row1[x];
        uint16_t a11 = row1[x + 1];

        if ((a00 | a01 | a10 | a11) == 0)
        {
            return 0;
        }

        if ((a00 & a01 & a10 & a11) == EGUI_ALPHA_100)
        {
            return EGUI_ALPHA_100;
        }

        {
            uint16_t ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
            uint16_t ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
            return (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));
        }
    }
    }
}

static inline uint8_t image_transform_sample_alpha_bilinear_fast(const uint8_t *alpha_buf, int alpha_row_bytes, int x, int y, uint8_t alpha_type, uint8_t fx,
                                                                 uint8_t fy)
{
    const uint8_t *row0 = alpha_buf + y * alpha_row_bytes;
    const uint8_t *row1 = row0 + alpha_row_bytes;
    return image_transform_sample_alpha_bilinear_from_rows(row0, row1, x, alpha_type, fx, fy);
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static inline uint8_t image_transform_sample_alpha_bilinear_external(const image_transform_external_source_t *source, int32_t x, int32_t y, uint8_t fx,
                                                                     uint8_t fy)
{
    uint32_t rows_per_chunk = image_transform_external_rows_per_chunk(EGUI_IMAGE_TRANSFORM_EXTERNAL_ALPHA_CACHE_MAX_BYTES, source->alpha_row_bytes);
    const uint8_t *row0 = NULL;
    const uint8_t *row1 = NULL;

    if (image_transform_external_rows_share_chunk(y, y + 1, rows_per_chunk))
    {
        row0 = image_transform_get_external_alpha_row(source, y);
        row1 = image_transform_get_external_alpha_row(source, y + 1);

        if (row0 != NULL && row1 != NULL)
        {
            return image_transform_sample_alpha_bilinear_from_rows(row0, row1, x, source->info->alpha_type, fx, fy);
        }
    }

    {
        uint16_t a00 = image_transform_read_alpha_external(source, x, y);
        uint16_t a01 = image_transform_read_alpha_external(source, x + 1, y);
        uint16_t a10 = image_transform_read_alpha_external(source, x, y + 1);
        uint16_t a11 = image_transform_read_alpha_external(source, x + 1, y + 1);
        uint16_t ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
        uint16_t ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
        return (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));
    }
}
#endif

__EGUI_STATIC_INLINE__ int transform_prepare_row_overlay(egui_mask_t *mask, egui_dim_t y, egui_color_t *overlay_color, egui_alpha_t *overlay_alpha)
{
    overlay_color->full = 0;
    *overlay_alpha = 0;
    return (mask != NULL && mask->api->mask_get_row_overlay != NULL && mask->api->mask_get_row_overlay(mask, y, overlay_color, overlay_alpha));
}

__EGUI_STATIC_INLINE__ int transform_prepare_row_color(egui_mask_t *mask, egui_dim_t y, egui_color_t *row_color)
{
    return (mask != NULL && mask->api->mask_blend_row_color != NULL && mask->api->mask_blend_row_color(mask, y, row_color));
}

#if (EGUI_CONFIG_COLOR_DEPTH == 16)
__EGUI_STATIC_INLINE__ void transform_blend_pixel_partial_rgb565(egui_color_int_t *dst, egui_color_t color, egui_alpha_t alpha)
{
    uint16_t bg = *dst;
    uint16_t fg = color.full;
    uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
    uint32_t fg_rb_g = (fg | ((uint32_t)fg << 16)) & 0x07E0F81FUL;
    uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * ((uint32_t)alpha >> 3) >> 5)) & 0x07E0F81FUL;
    *dst = (uint16_t)(result | (result >> 16));
}
#endif

__EGUI_STATIC_INLINE__ void transform_blend_pixel(egui_color_int_t *dst, egui_color_t color, egui_alpha_t alpha)
{
    if (alpha == EGUI_ALPHA_100)
    {
        *dst = color.full;
    }
    else if (alpha > 0)
    {
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
        if (alpha > 251)
        {
            *dst = color.full;
        }
        else if (alpha >= 4)
        {
            transform_blend_pixel_partial_rgb565(dst, color, alpha);
        }
#else
        egui_color_t *back = (egui_color_t *)dst;
        egui_rgb_mix_ptr(back, &color, back, alpha);
#endif
    }
}

__EGUI_STATIC_INLINE__ void transform_apply_mask_and_blend(egui_mask_t *mask, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t pixel_alpha,
                                                           egui_alpha_t canvas_alpha, egui_color_int_t *dst)
{
    if (mask != NULL)
    {
        mask->api->mask_point(mask, x, y, &color, &pixel_alpha);
    }

    transform_blend_pixel(dst, color, egui_color_alpha_mix(canvas_alpha, pixel_alpha));
}

/**
 * Draw a rotated and scaled image using inverse-mapping with bilinear interpolation.
 *
 * @param img       Source image (must be egui_image_std_t with RGB565 data)
 * @param x         Center X position in view coordinates
 * @param y         Center Y position in view coordinates
 * @param angle_deg Rotation angle in degrees (0-360, counter-clockwise)
 * @param scale_q8  Scale factor in Q8 format (256 = 1.0x, 512 = 2.0x, 128 = 0.5x)
 */
__EGUI_OPTIMIZE_SIZE__ void egui_canvas_draw_image_transform(const egui_image_t *img, egui_dim_t x, egui_dim_t y, int16_t angle_deg, int16_t scale_q8)
{
    egui_canvas_t *canvas = &canvas_data;
    egui_image_std_info_t *info = (egui_image_std_info_t *)img->res;

    int source_is_opaque = egui_image_std_rgb565_is_opaque_source(info);

    int16_t src_w = info->width;
    int16_t src_h = info->height;
    int16_t cx = src_w / 2;
    int16_t cy = src_h / 2;

    if (scale_q8 == 0)
    {
        scale_q8 = 1;
    }

    int32_t sinA = transform_sin_q15(angle_deg);
    int32_t cosA = transform_cos_q15(angle_deg);

    /* Forward affine matrix with scale (Q15) */
    int32_t fwd_m00 = (cosA * scale_q8) >> 8;
    int32_t fwd_m01 = (-sinA * scale_q8) >> 8;
    int32_t fwd_m10 = (sinA * scale_q8) >> 8;
    int32_t fwd_m11 = (cosA * scale_q8) >> 8;

    /* Compute bounding box of rotated image by transforming 4 corners */
    int32_t min_bx = 0x7FFFFFFF, min_by = 0x7FFFFFFF;
    int32_t max_bx = -0x7FFFFFFF, max_by = -0x7FFFFFFF;
    int32_t corners[4][2] = {{0, 0}, {src_w, 0}, {0, src_h}, {src_w, src_h}};
    for (int i = 0; i < 4; i++)
    {
        int32_t dx_rel = corners[i][0] - cx;
        int32_t dy_rel = corners[i][1] - cy;
        int32_t dx = (((int64_t)dx_rel * fwd_m00 + (int64_t)dy_rel * fwd_m01) + (1 << 14)) >> 15;
        int32_t dy = (((int64_t)dx_rel * fwd_m10 + (int64_t)dy_rel * fwd_m11) + (1 << 14)) >> 15;
        dx += x;
        dy += y;
        if (dx < min_bx)
            min_bx = dx;
        if (dy < min_by)
            min_by = dy;
        if (dx > max_bx)
            max_bx = dx;
        if (dy > max_by)
            max_by = dy;
    }
    max_bx++;
    max_by++;

    /* Clip bounding box to canvas work region */
    egui_region_t *work = &canvas->base_view_work_region;
    int32_t clip_x0 = work->location.x;
    int32_t clip_y0 = work->location.y;
    int32_t clip_x1 = clip_x0 + work->size.width;
    int32_t clip_y1 = clip_y0 + work->size.height;

    int32_t draw_x0 = (min_bx > clip_x0) ? min_bx : clip_x0;
    int32_t draw_y0 = (min_by > clip_y0) ? min_by : clip_y0;
    int32_t draw_x1 = (max_bx < clip_x1) ? max_bx : clip_x1;
    int32_t draw_y1 = (max_by < clip_y1) ? max_by : clip_y1;

    if (draw_x0 >= draw_x1 || draw_y0 >= draw_y1)
    {
        return;
    }

    /* Inverse affine matrix (Q15): rotate back and undo scale */
    int32_t inv_m00 = (cosA << 8) / scale_q8;
    int32_t inv_m01 = (sinA << 8) / scale_q8;
    int32_t inv_m10 = (-sinA << 8) / scale_q8;
    int32_t inv_m11 = (cosA << 8) / scale_q8;

    /* Center correction for even-sized images (0.5px shift) */
    int32_t offx = (src_w & 1) ? 0 : (inv_m00 + inv_m01 - 32767) / 2;
    int32_t offy = (src_h & 1) ? 0 : (inv_m10 + inv_m11 - 32767) / 2;

    /* PFB write setup */
    egui_dim_t pfb_w = canvas->pfb_region.size.width;
    egui_dim_t pfb_ox = canvas->pfb_location_in_base_view.x;
    egui_dim_t pfb_oy = canvas->pfb_location_in_base_view.y;
    egui_color_int_t *pfb = canvas->pfb;
    egui_alpha_t canvas_alpha = canvas->alpha;
    egui_mask_t *mask = canvas->mask;

    /* Source pixel data */
    const uint16_t *data = NULL;
    const uint8_t *alpha_buf = NULL;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    image_transform_external_source_t external_source_storage;
    const image_transform_external_source_t *external_source = NULL;
#if !EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE
    image_transform_external_data_row_slot_t external_data_row_cache = {0};
    image_transform_external_alpha_row_slot_t external_alpha_row_cache = {0};
#endif
#endif

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
#if !EGUI_CONFIG_IMAGE_TRANSFORM_EXTERNAL_ROW_PERSISTENT_CACHE_ENABLE
        external_source_storage.data_cache = &external_data_row_cache;
        external_source_storage.alpha_cache = &external_alpha_row_cache;
#endif
        if (!image_transform_prepare_external_source(info, source_is_opaque, &external_source_storage))
        {
            return;
        }
        external_source = &external_source_storage;
    }
    else
#endif
    {
        data = (const uint16_t *)info->data_buf;
        alpha_buf = source_is_opaque ? NULL : (const uint8_t *)info->alpha_buf;
    }
    int has_alpha = 0;
    uint8_t alpha_type = info->alpha_type;
    int alpha_row_bytes = 0;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (external_source != NULL)
    {
        has_alpha = (external_source->alpha_row_bytes > 0);
        alpha_row_bytes = (int)external_source->alpha_row_bytes;
    }
    else
#endif
    {
        has_alpha = (alpha_buf != NULL);
        if (has_alpha)
        {
            alpha_row_bytes = image_transform_get_alpha_row_bytes(src_w, alpha_type);
            if (alpha_row_bytes == 0)
            {
                has_alpha = 0;
            }
        }
    }
    int opaque_mode = (!has_alpha && canvas_alpha == EGUI_ALPHA_100);

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
#define TRANSFORM_FETCH_BILINEAR_RGB565(_px, _py, _d00, _d01, _d10, _d11)                                                                                      \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if (external_source != NULL)                                                                                                                           \
        {                                                                                                                                                      \
            image_transform_fetch_bilinear_rgb565_external(external_source, (_px), (_py), &(_d00), &(_d01), &(_d10), &(_d11));                                 \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            int32_t _offs = (_py) * src_w + (_px);                                                                                                             \
            (_d00) = data[_offs];                                                                                                                              \
            (_d01) = data[_offs + 1];                                                                                                                          \
            (_d10) = data[_offs + src_w];                                                                                                                      \
            (_d11) = data[_offs + src_w + 1];                                                                                                                  \
        }                                                                                                                                                      \
    } while (0)
#define TRANSFORM_SAMPLE_ALPHA_BILINEAR(_px, _py, _fx, _fy)                                                                                                    \
    ((external_source != NULL) ? image_transform_sample_alpha_bilinear_external(external_source, (_px), (_py), (_fx), (_fy))                                   \
                               : image_transform_sample_alpha_bilinear_fast(alpha_buf, alpha_row_bytes, (_px), (_py), alpha_type, (_fx), (_fy)))
#define TRANSFORM_READ_PIXEL_POINT(_px, _py)                                                                                                                   \
    ((external_source != NULL) ? image_transform_read_rgb565_external(external_source, (_px), (_py)) : data[(_py) * src_w + (_px)])
#define TRANSFORM_READ_ALPHA_POINT(_px, _py)                                                                                                                   \
    ((external_source != NULL) ? image_transform_read_alpha_external(external_source, (_px), (_py))                                                            \
                               : image_transform_read_alpha(alpha_buf, alpha_row_bytes, (_px), (_py), alpha_type))
#else
#define TRANSFORM_FETCH_BILINEAR_RGB565(_px, _py, _d00, _d01, _d10, _d11)                                                                                      \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        int32_t _offs = (_py) * src_w + (_px);                                                                                                                 \
        (_d00) = data[_offs];                                                                                                                                  \
        (_d01) = data[_offs + 1];                                                                                                                              \
        (_d10) = data[_offs + src_w];                                                                                                                          \
        (_d11) = data[_offs + src_w + 1];                                                                                                                      \
    } while (0)
#define TRANSFORM_SAMPLE_ALPHA_BILINEAR(_px, _py, _fx, _fy)                                                                                                    \
    image_transform_sample_alpha_bilinear_fast(alpha_buf, alpha_row_bytes, (_px), (_py), alpha_type, (_fx), (_fy))
#define TRANSFORM_READ_PIXEL_POINT(_px, _py) data[(_py) * src_w + (_px)]
#define TRANSFORM_READ_ALPHA_POINT(_px, _py) image_transform_read_alpha(alpha_buf, alpha_row_bytes, (_px), (_py), alpha_type)
#endif

    /* Row-constant terms for incremental scanning */
    int32_t Cx_base = inv_m01 * ((int32_t)draw_y0 - y) + ((int32_t)cx << 15) + offx;
    int32_t Cy_base = inv_m11 * ((int32_t)draw_y0 - y) + ((int32_t)cy << 15) + offy;

    /* Hoist loop-invariant row-start offset computed from draw_x0 */
    int32_t row_start_offset_x = inv_m00 * ((int32_t)draw_x0 - x);
    int32_t row_start_offset_y = inv_m10 * ((int32_t)draw_x0 - x);

    /* Source bounds in Q15 for scanline skip */
    int32_t src_lo_x_q15 = -(1 << 15);
    int32_t src_hi_x_q15 = (int32_t)src_w << 15;
    int32_t src_lo_y_q15 = -(1 << 15);
    int32_t src_hi_y_q15 = (int32_t)src_h << 15;

    /* Interior bounds in Q15 for SIR (Scanline Interior Range) */
    int32_t int_max_x_q15 = ((int32_t)(src_w - 1)) << 15;
    int32_t int_max_y_q15 = ((int32_t)(src_h - 1)) << 15;

    for (int32_t dy = draw_y0; dy < draw_y1; dy++)
    {
        int32_t rotatedX = row_start_offset_x + Cx_base;
        int32_t rotatedY = row_start_offset_y + Cy_base;

        egui_color_int_t *dst_row = &pfb[(dy - pfb_oy) * pfb_w + (draw_x0 - pfb_ox)];
        int entered = 0;
        int32_t dx = draw_x0;

        egui_color_t row_overlay_color;
        egui_alpha_t row_overlay_alpha;
        int row_overlay_handled = transform_prepare_row_overlay(mask, (egui_dim_t)dy, &row_overlay_color, &row_overlay_alpha);
        int mask_requires_point = (mask != NULL && !row_overlay_handled);

        /* Skip left dead zone where inverse-mapped coords are outside source */
        if (rotatedX < src_lo_x_q15 || rotatedX >= src_hi_x_q15 || rotatedY < src_lo_y_q15 || rotatedY >= src_hi_y_q15)
        {
            int32_t skip =
                    transform_scanline_skip(rotatedX, rotatedY, inv_m00, inv_m10, src_lo_x_q15, src_hi_x_q15, src_lo_y_q15, src_hi_y_q15, draw_x1 - draw_x0);
            if (skip > 0)
            {
                rotatedX += skip * inv_m00;
                rotatedY += skip * inv_m10;
                dst_row += skip;
                dx += skip;
            }
        }

        for (; dx < draw_x1; dx++)
        {
            int32_t sx = rotatedX >> 15;
            int32_t sy = rotatedY >> 15;

            if (sx >= 0 && sx < src_w - 1 && sy >= 0 && sy < src_h - 1)
            {
                entered = 1;

                /* SIR: compute how many consecutive pixels remain interior.
                 * For each axis, find distance to exit boundary based on step direction. */
                int32_t sir_count = draw_x1 - dx;
                if (inv_m00 > 0)
                {
                    int32_t n = (int_max_x_q15 - rotatedX) / inv_m00;
                    if (n < sir_count)
                        sir_count = n;
                }
                else if (inv_m00 < 0)
                {
                    int32_t n = rotatedX / (-inv_m00);
                    if (n < sir_count)
                        sir_count = n;
                }
                if (inv_m10 > 0)
                {
                    int32_t n = (int_max_y_q15 - rotatedY) / inv_m10;
                    if (n < sir_count)
                        sir_count = n;
                }
                else if (inv_m10 < 0)
                {
                    int32_t n = rotatedY / (-inv_m10);
                    if (n < sir_count)
                        sir_count = n;
                }
                if (sir_count < 1)
                    sir_count = 1;

                /* Tight interior loop: no boundary checks needed */
                if (opaque_mode && !mask_requires_point)
                {
                    for (int32_t i = 0; i < sir_count; i++)
                    {
                        int32_t px = rotatedX >> 15;
                        int32_t py = rotatedY >> 15;
                        uint8_t fx = (rotatedX >> 7) & 0xFF;
                        uint8_t fy = (rotatedY >> 7) & 0xFF;
                        uint16_t d00, d01, d10, d11;
                        TRANSFORM_FETCH_BILINEAR_RGB565(px, py, d00, d01, d10, d11);
                        egui_color_t color;
                        color.full = bilinear_rgb565_packed(d00, d01, d10, d11, fx, fy);
                        if (row_overlay_alpha > 0)
                        {
                            egui_rgb_mix_ptr(&color, &row_overlay_color, &color, row_overlay_alpha);
                        }
                        *dst_row = color.full;
                        rotatedX += inv_m00;
                        rotatedY += inv_m10;
                        dst_row++;
                    }
                }
                else if (has_alpha && canvas_alpha == EGUI_ALPHA_100 && !mask_requires_point)
                {
                    /* Alpha image with fully opaque canvas: read alpha first
                     * to skip color bilinear for transparent pixels */
                    for (int32_t i = 0; i < sir_count; i++)
                    {
                        int32_t px = rotatedX >> 15;
                        int32_t py = rotatedY >> 15;
                        uint8_t fx = (rotatedX >> 7) & 0xFF;
                        uint8_t fy = (rotatedY >> 7) & 0xFF;
                        egui_alpha_t pixel_alpha = TRANSFORM_SAMPLE_ALPHA_BILINEAR(px, py, fx, fy);

                        if (pixel_alpha > 0)
                        {
                            uint16_t d00, d01, d10, d11;
                            TRANSFORM_FETCH_BILINEAR_RGB565(px, py, d00, d01, d10, d11);
                            egui_color_t color;
                            color.full = bilinear_rgb565_packed(d00, d01, d10, d11, fx, fy);
                            if (row_overlay_alpha > 0)
                            {
                                egui_rgb_mix_ptr(&color, &row_overlay_color, &color, row_overlay_alpha);
                            }

                            if (pixel_alpha == EGUI_ALPHA_100)
                            {
                                *dst_row = color.full;
                            }
                            else
                            {
                                egui_color_t *back = (egui_color_t *)dst_row;
                                egui_rgb_mix_ptr(back, &color, back, pixel_alpha);
                            }
                        }

                        rotatedX += inv_m00;
                        rotatedY += inv_m10;
                        dst_row++;
                    }
                }
                else
                {
                    /* Generic SIR loop: handles any alpha type and/or canvas_alpha != 100 */
                    for (int32_t i = 0; i < sir_count; i++)
                    {
                        int32_t px = rotatedX >> 15;
                        int32_t py = rotatedY >> 15;
                        uint8_t fx = (rotatedX >> 7) & 0xFF;
                        uint8_t fy = (rotatedY >> 7) & 0xFF;
                        uint16_t d00, d01, d10, d11;
                        TRANSFORM_FETCH_BILINEAR_RGB565(px, py, d00, d01, d10, d11);
                        egui_color_t color;
                        color.full = bilinear_rgb565_packed(d00, d01, d10, d11, fx, fy);
                        if (row_overlay_handled && row_overlay_alpha > 0)
                        {
                            egui_rgb_mix_ptr(&color, &row_overlay_color, &color, row_overlay_alpha);
                        }

                        egui_alpha_t pixel_alpha = EGUI_ALPHA_100;
                        if (has_alpha)
                        {
                            pixel_alpha = TRANSFORM_SAMPLE_ALPHA_BILINEAR(px, py, fx, fy);
                        }
                        if (mask_requires_point)
                        {
                            transform_apply_mask_and_blend(mask, (egui_dim_t)(dx + i), (egui_dim_t)dy, color, pixel_alpha, canvas_alpha, dst_row);
                        }
                        else
                        {
                            egui_alpha_t final_alpha = egui_color_alpha_mix(canvas_alpha, pixel_alpha);
                            if (final_alpha == EGUI_ALPHA_100)
                            {
                                *dst_row = color.full;
                            }
                            else if (final_alpha > 0)
                            {
                                egui_color_t *back = (egui_color_t *)dst_row;
                                egui_rgb_mix_ptr(back, &color, back, final_alpha);
                            }
                        }
                        rotatedX += inv_m00;
                        rotatedY += inv_m10;
                        dst_row++;
                    }
                }
                dx += sir_count - 1;
                /* After SIR, continue with per-pixel checks for remaining edge pixels */
                continue;
            }
            else if (sx >= -1 && sx < src_w && sy >= -1 && sy < src_h)
            {
                entered = 1;
                /* Edge: 1-pixel border, use destination as fallback for out-of-bounds samples */
                uint8_t fx = (rotatedX >> 7) & 0xFF;
                uint8_t fy = (rotatedY >> 7) & 0xFF;

                uint16_t bg = *dst_row;
#define TRANSFORM_FETCH_PIXEL(px, py) (((px) >= 0 && (px) < src_w && (py) >= 0 && (py) < src_h) ? TRANSFORM_READ_PIXEL_POINT((px), (py)) : bg)
                uint16_t d00 = TRANSFORM_FETCH_PIXEL(sx, sy);
                uint16_t d01 = TRANSFORM_FETCH_PIXEL(sx + 1, sy);
                uint16_t d10 = TRANSFORM_FETCH_PIXEL(sx, sy + 1);
                uint16_t d11 = TRANSFORM_FETCH_PIXEL(sx + 1, sy + 1);
#undef TRANSFORM_FETCH_PIXEL

                egui_color_t color;
                color.full = bilinear_rgb565_packed(d00, d01, d10, d11, fx, fy);
                if (row_overlay_handled && row_overlay_alpha > 0)
                {
                    egui_rgb_mix_ptr(&color, &row_overlay_color, &color, row_overlay_alpha);
                }

                egui_alpha_t pixel_alpha = EGUI_ALPHA_100;
                if (has_alpha)
                {
                    /* Bilinear alpha from edge samples (out-of-bounds → 0) */
#define TRANSFORM_FETCH_ALPHA(px, py) (((px) >= 0 && (px) < src_w && (py) >= 0 && (py) < src_h) ? TRANSFORM_READ_ALPHA_POINT((px), (py)) : 0)
                    uint16_t a00 = TRANSFORM_FETCH_ALPHA(sx, sy);
                    uint16_t a01 = TRANSFORM_FETCH_ALPHA(sx + 1, sy);
                    uint16_t a10 = TRANSFORM_FETCH_ALPHA(sx, sy + 1);
                    uint16_t a11 = TRANSFORM_FETCH_ALPHA(sx + 1, sy + 1);
#undef TRANSFORM_FETCH_ALPHA
                    uint16_t ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
                    uint16_t ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
                    pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));
                }

                if (mask_requires_point)
                {
                    transform_apply_mask_and_blend(mask, (egui_dim_t)dx, (egui_dim_t)dy, color, pixel_alpha, canvas_alpha, dst_row);
                }
                else
                {
                    egui_alpha_t final_alpha = egui_color_alpha_mix(canvas_alpha, pixel_alpha);
                    if (final_alpha == EGUI_ALPHA_100)
                    {
                        *dst_row = color.full;
                    }
                    else if (final_alpha > 0)
                    {
                        egui_color_t *back = (egui_color_t *)dst_row;
                        egui_rgb_mix_ptr(back, &color, back, final_alpha);
                    }
                }
            }
            else if (entered)
            {
                break; /* Early exit: left source region (SCGUI-style scanline break) */
            }

            rotatedX += inv_m00;
            rotatedY += inv_m10;
            dst_row++;
        }

        Cx_base += inv_m01;
        Cy_base += inv_m11;
    }

#undef TRANSFORM_FETCH_BILINEAR_RGB565
#undef TRANSFORM_SAMPLE_ALPHA_BILINEAR
#undef TRANSFORM_READ_PIXEL_POINT
#undef TRANSFORM_READ_ALPHA_POINT
}

// ============================================================================
// Text transform shared helpers
// ============================================================================

typedef struct
{
    const uint8_t *data; /* packed bitmap data pointer (in ROM) */
    uint32_t pixel_idx;  /* external resource byte offset, internal path keeps 0 */
    int16_t x;           /* glyph box left edge in text space */
    int16_t y;           /* glyph box top edge in text space */
    uint8_t box_w;       /* glyph box width */
    uint8_t box_h;       /* glyph box height */
    uint8_t row_bytes;   /* packed bytes per glyph row */
    uint8_t line;        /* tile-local line index */
} text_transform_glyph_t;

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES
#define EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES 4096
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_STACK_MAX_BYTES
#define EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_STACK_MAX_BYTES 0
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE
#define EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE 0
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_GLYPHS
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_GLYPHS 0
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_LINES
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_LINES 0
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_CACHE_ENABLE
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_CACHE_ENABLE 1
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_HEAP_ENABLE
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_HEAP_ENABLE 1
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_DIM_CACHE_ENABLE
#define EGUI_CONFIG_TEXT_TRANSFORM_DIM_CACHE_ENABLE 1
#endif

#if !EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_HEAP_ENABLE && EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_CACHE_ENABLE
#error "EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_HEAP_ENABLE=0 requires EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_CACHE_ENABLE=0"
#endif

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
typedef struct
{
    const text_transform_glyph_t *glyph;
    int16_t row0_y;
    int16_t row1_y;
    uint16_t row_capacity;
    uint8_t *row0;
    uint8_t *row1;
} text_transform_external_glyph_row_cache_t;
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT 0
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT 0
#endif

#if EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT
typedef uint16_t text_transform_layout_pixel_idx_t;
#define TEXT_TRANSFORM_LAYOUT_PIXEL_IDX_MAX UINT16_MAX
#else
typedef uint32_t text_transform_layout_pixel_idx_t;
#define TEXT_TRANSFORM_LAYOUT_PIXEL_IDX_MAX UINT32_MAX
#endif

#if EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT
typedef uint16_t text_transform_layout_index_t;
#define TEXT_TRANSFORM_LAYOUT_INDEX_MAX UINT16_MAX
#else
typedef int text_transform_layout_index_t;
#define TEXT_TRANSFORM_LAYOUT_INDEX_MAX 0x7FFFFFFF
#endif

typedef struct
{
    text_transform_layout_pixel_idx_t pixel_idx; /* packed bitmap byte offset */
    int16_t x;                                   /* glyph box left edge in text space */
    int16_t y;                                   /* glyph box top edge in text space */
    uint8_t box_w;                               /* glyph box width */
    uint8_t box_h;                               /* glyph box height */
} text_transform_layout_glyph_t;

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
typedef struct
{
    uint8_t chunk_rows;
    uint16_t row_capacity;
    uint8_t *row_buf;
} text_transform_external_layout_glyph_row_scratch_t;
#endif

typedef struct
{
    const egui_font_t *font;
    const void *string;
    egui_dim_t line_space;
    int count;
    int line_count;
} text_transform_layout_cache_t;

typedef struct
{
    text_transform_layout_index_t start;
    text_transform_layout_index_t end;
    int16_t x_min;
    int16_t x_max;
    int16_t y_min;
    int16_t y_max;
} text_transform_layout_line_t;

#if EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE
typedef struct
{
    uint8_t *block;
    text_transform_layout_glyph_t *glyphs;
    text_transform_layout_line_t *lines;
} text_transform_layout_scratch_t;

typedef struct
{
    uint8_t *block;
    text_transform_glyph_t *glyphs;
    text_transform_layout_line_t *lines;
} text_transform_tile_scratch_t;

__EGUI_STATIC_INLINE__ size_t text_transform_align_scratch_size(size_t size)
{
    const size_t align = sizeof(void *);
    return (size + align - 1U) & ~(align - 1U);
}

static int text_transform_prepare_layout_scratch(int glyph_count, int line_count, text_transform_layout_scratch_t *scratch)
{
    size_t glyph_bytes;
    size_t line_bytes;
    size_t line_offset;
    uint8_t *block;

    if (scratch == NULL || glyph_count <= 0 || line_count <= 0)
    {
        return -1;
    }

    glyph_bytes = (size_t)glyph_count * sizeof(text_transform_layout_glyph_t);
    line_bytes = (size_t)line_count * sizeof(text_transform_layout_line_t);
    line_offset = text_transform_align_scratch_size(glyph_bytes);

    block = (uint8_t *)egui_malloc((int)(line_offset + line_bytes));
    if (block == NULL)
    {
        return -1;
    }

    scratch->block = block;
    scratch->glyphs = (text_transform_layout_glyph_t *)block;
    scratch->lines = (text_transform_layout_line_t *)(block + line_offset);
    return 0;
}

static void text_transform_release_layout_scratch(text_transform_layout_scratch_t *scratch)
{
    if (scratch == NULL)
    {
        return;
    }

    if (scratch->block != NULL)
    {
        egui_free(scratch->block);
    }

    scratch->block = NULL;
    scratch->glyphs = NULL;
    scratch->lines = NULL;
}

static int text_transform_prepare_tile_scratch(int glyph_count, int line_count, text_transform_tile_scratch_t *scratch)
{
    size_t glyph_bytes;
    size_t line_bytes;
    size_t line_offset;
    uint8_t *block;

    if (scratch == NULL || glyph_count <= 0 || line_count <= 0)
    {
        return -1;
    }

    glyph_bytes = (size_t)glyph_count * sizeof(text_transform_glyph_t);
    line_bytes = (size_t)line_count * sizeof(text_transform_layout_line_t);
    line_offset = text_transform_align_scratch_size(glyph_bytes);

    block = (uint8_t *)egui_malloc((int)(line_offset + line_bytes));
    if (block == NULL)
    {
        return -1;
    }

    scratch->block = block;
    scratch->glyphs = (text_transform_glyph_t *)block;
    scratch->lines = (text_transform_layout_line_t *)(block + line_offset);
    return 0;
}

static void text_transform_release_tile_scratch(text_transform_tile_scratch_t *scratch)
{
    if (scratch == NULL)
    {
        return;
    }

    if (scratch->block != NULL)
    {
        egui_free(scratch->block);
    }

    scratch->block = NULL;
    scratch->glyphs = NULL;
    scratch->lines = NULL;
}
#endif

/**
 * Extract alpha value from packed font bitmap at local glyph coordinates.
 * Each row is byte-aligned in the packed data.
 */
static inline uint8_t extract_packed_alpha(const uint8_t *data, uint8_t box_w, int local_x, int local_y, uint8_t bpp)
{
    switch (bpp)
    {
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
    case 1:
    {
        int row_bytes = (box_w + 7) >> 3;
        int idx = local_y * row_bytes + (local_x >> 3);
        int bit = local_x & 7;
        return (data[idx] >> bit) & 1 ? 255 : 0;
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
    case 2:
    {
        int row_bytes = (box_w + 3) >> 2;
        int idx = local_y * row_bytes + (local_x >> 2);
        int bit = (local_x & 3) << 1;
        return egui_alpha_change_table_2[(data[idx] >> bit) & 0x03];
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    case 4:
    {
        int row_bytes = (box_w + 1) >> 1;
        int idx = local_y * row_bytes + (local_x >> 1);
        int bit = (local_x & 1) << 2;
        return egui_alpha_change_table_4[(data[idx] >> bit) & 0x0F];
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
    case 8:
        return data[local_y * box_w + local_x];
#endif
    default:
        return 0;
    }
}

static inline uint8_t extract_packed_alpha_4(const uint8_t *data, uint8_t box_w, int local_x, int local_y)
{
    int row_bytes = (box_w + 1) >> 1;
    uint8_t packed = data[local_y * row_bytes + (local_x >> 1)];
    return egui_alpha_change_table_4[(packed >> ((local_x & 1) << 2)) & 0x0F];
}

static inline uint8_t extract_packed_alpha_4_rb(const uint8_t *data, uint8_t row_bytes, int local_x, int local_y)
{
    uint8_t packed = data[local_y * row_bytes + (local_x >> 1)];
    return egui_alpha_change_table_4[(packed >> ((local_x & 1) << 2)) & 0x0F];
}

static inline void extract_packed_alpha_pair_4_rb(const uint8_t *data, uint8_t row_bytes, int local_x, int local_y, uint16_t *a0, uint16_t *a1)
{
    const uint8_t *row = data + local_y * row_bytes;
    int idx = local_x >> 1;

    if ((local_x & 1) == 0)
    {
        uint8_t packed = row[idx];
        *a0 = egui_alpha_change_table_4[packed & 0x0F];
        *a1 = egui_alpha_change_table_4[(packed >> 4) & 0x0F];
    }
    else
    {
        uint8_t packed0 = row[idx];
        uint8_t packed1 = row[idx + 1];
        *a0 = egui_alpha_change_table_4[(packed0 >> 4) & 0x0F];
        *a1 = egui_alpha_change_table_4[packed1 & 0x0F];
    }
}

static inline uint8_t extract_packed_alpha_from_row(const uint8_t *row, int local_x, uint8_t bpp)
{
    switch (bpp)
    {
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
    case 1:
        return (row[local_x >> 3] >> (local_x & 7)) & 1 ? 255 : 0;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
    case 2:
        return egui_alpha_change_table_2[(row[local_x >> 2] >> ((local_x & 3) << 1)) & 0x03];
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    case 4:
        return egui_alpha_change_table_4[(row[local_x >> 1] >> ((local_x & 1) << 2)) & 0x0F];
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
    case 8:
        return row[local_x];
#endif
    default:
        return 0;
    }
}

static inline void extract_packed_alpha_pair_4_from_row(const uint8_t *row, int local_x, uint16_t *a0, uint16_t *a1)
{
    int idx = local_x >> 1;

    if ((local_x & 1) == 0)
    {
        uint8_t packed = row[idx];
        *a0 = egui_alpha_change_table_4[packed & 0x0F];
        *a1 = egui_alpha_change_table_4[(packed >> 4) & 0x0F];
    }
    else
    {
        uint8_t packed0 = row[idx];
        uint8_t packed1 = row[idx + 1];
        *a0 = egui_alpha_change_table_4[(packed0 >> 4) & 0x0F];
        *a1 = egui_alpha_change_table_4[packed1 & 0x0F];
    }
}

__EGUI_STATIC_INLINE__ void extract_packed_alpha_pair_from_row(const uint8_t *row, int local_x, uint8_t bpp, uint16_t *a0, uint16_t *a1)
{
    if (bpp == 4)
    {
        extract_packed_alpha_pair_4_from_row(row, local_x, a0, a1);
        return;
    }

    *a0 = extract_packed_alpha_from_row(row, local_x, bpp);
    *a1 = extract_packed_alpha_from_row(row, local_x + 1, bpp);
}

__EGUI_STATIC_INLINE__ void batch_extract_glyph_alpha_from_rows(const uint8_t *row0, const uint8_t *row1, int lx, uint8_t bpp, uint16_t *a00, uint16_t *a01,
                                                                uint16_t *a10, uint16_t *a11)
{
    if (bpp == 4)
    {
        extract_packed_alpha_pair_4_from_row(row0, lx, a00, a01);
        extract_packed_alpha_pair_4_from_row(row1, lx, a10, a11);
        return;
    }

    *a00 = extract_packed_alpha_from_row(row0, lx, bpp);
    *a01 = extract_packed_alpha_from_row(row0, lx + 1, bpp);
    *a10 = extract_packed_alpha_from_row(row1, lx, bpp);
    *a11 = extract_packed_alpha_from_row(row1, lx + 1, bpp);
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static int text_transform_load_external_glyph_row(const text_transform_glyph_t *glyph, int ly, text_transform_external_glyph_row_cache_t *cache,
                                                  const uint8_t **row)
{
    if (glyph == NULL || cache == NULL || row == NULL || ly < 0 || ly >= glyph->box_h || glyph->row_bytes == 0 || glyph->row_bytes > cache->row_capacity ||
        cache->row0 == NULL)
    {
        EGUI_ASSERT(0);
        return 0;
    }

    if (cache->glyph == glyph)
    {
        if (cache->row0_y == ly)
        {
            *row = cache->row0;
            return 1;
        }
        if (cache->row1_y == ly)
        {
            *row = cache->row1;
            return 1;
        }
    }

    egui_api_load_external_resource(cache->row0, (egui_uintptr_t)glyph->data, glyph->pixel_idx + (uint32_t)ly * glyph->row_bytes, glyph->row_bytes);
    cache->glyph = glyph;
    cache->row0_y = (int16_t)ly;
    cache->row1_y = -1;
    *row = cache->row0;
    return 1;
}

static int text_transform_load_external_glyph_row_pair(const text_transform_glyph_t *glyph, int ly, text_transform_external_glyph_row_cache_t *cache,
                                                       const uint8_t **row0, const uint8_t **row1)
{
    if (glyph == NULL || cache == NULL || row0 == NULL || row1 == NULL || ly < 0 || (ly + 1) >= glyph->box_h || glyph->row_bytes == 0 ||
        glyph->row_bytes > cache->row_capacity || cache->row0 == NULL || cache->row1 == NULL)
    {
        EGUI_ASSERT(0);
        return 0;
    }

    if (cache->glyph == glyph && cache->row0_y == ly && cache->row1_y == (ly + 1))
    {
        *row0 = cache->row0;
        *row1 = cache->row1;
        return 1;
    }

    egui_api_load_external_resource(cache->row0, (egui_uintptr_t)glyph->data, glyph->pixel_idx + (uint32_t)ly * glyph->row_bytes, glyph->row_bytes);
    egui_api_load_external_resource(cache->row1, (egui_uintptr_t)glyph->data, glyph->pixel_idx + (uint32_t)(ly + 1) * glyph->row_bytes, glyph->row_bytes);
    cache->glyph = glyph;
    cache->row0_y = (int16_t)ly;
    cache->row1_y = (int16_t)(ly + 1);
    *row0 = cache->row0;
    *row1 = cache->row1;
    return 1;
}

static int text_transform_batch_extract_external_glyph_alpha(const text_transform_glyph_t *glyph, int lx, int ly, uint8_t bpp,
                                                             text_transform_external_glyph_row_cache_t *cache, uint16_t *a00, uint16_t *a01, uint16_t *a10,
                                                             uint16_t *a11)
{
    const uint8_t *row0;
    const uint8_t *row1;

    if (!text_transform_load_external_glyph_row_pair(glyph, ly, cache, &row0, &row1))
    {
        return 0;
    }

    batch_extract_glyph_alpha_from_rows(row0, row1, lx, bpp, a00, a01, a10, a11);
    return 1;
}

static int text_transform_extract_external_glyph_alpha_from_row(const text_transform_glyph_t *glyph, int lx, int ly, uint8_t bpp,
                                                                text_transform_external_glyph_row_cache_t *cache, uint16_t *alpha)
{
    const uint8_t *row;

    if (alpha == NULL || !text_transform_load_external_glyph_row(glyph, ly, cache, &row))
    {
        return 0;
    }

    *alpha = extract_packed_alpha_from_row(row, lx, bpp);
    return 1;
}

static int text_transform_extract_external_glyph_alpha_pair_from_row(const text_transform_glyph_t *glyph, int lx, int ly, uint8_t bpp,
                                                                     text_transform_external_glyph_row_cache_t *cache, uint16_t *a0, uint16_t *a1)
{
    const uint8_t *row;

    if (a0 == NULL || a1 == NULL || !text_transform_load_external_glyph_row(glyph, ly, cache, &row))
    {
        return 0;
    }

    extract_packed_alpha_pair_from_row(row, lx, bpp, a0, a1);
    return 1;
}
#endif

static inline void batch_extract_glyph_alpha_4(const uint8_t *data, uint8_t box_w, int lx, int ly, uint16_t *a00, uint16_t *a01, uint16_t *a10, uint16_t *a11)
{
    int rb = (box_w + 1) >> 1;
    const uint8_t *row0 = data + ly * rb;
    const uint8_t *row1 = row0 + rb;
    int idx = lx >> 1;

    if ((lx & 1) == 0)
    {
        uint8_t packed0 = row0[idx];
        uint8_t packed1 = row1[idx];

        *a00 = egui_alpha_change_table_4[packed0 & 0x0F];
        *a01 = egui_alpha_change_table_4[(packed0 >> 4) & 0x0F];
        *a10 = egui_alpha_change_table_4[packed1 & 0x0F];
        *a11 = egui_alpha_change_table_4[(packed1 >> 4) & 0x0F];
    }
    else
    {
        uint8_t packed00 = row0[idx];
        uint8_t packed01 = row0[idx + 1];
        uint8_t packed10 = row1[idx];
        uint8_t packed11 = row1[idx + 1];

        *a00 = egui_alpha_change_table_4[(packed00 >> 4) & 0x0F];
        *a01 = egui_alpha_change_table_4[packed01 & 0x0F];
        *a10 = egui_alpha_change_table_4[(packed10 >> 4) & 0x0F];
        *a11 = egui_alpha_change_table_4[packed11 & 0x0F];
    }
}

static inline void batch_extract_glyph_alpha_4_rb(const uint8_t *data, uint8_t row_bytes, int lx, int ly, uint16_t *a00, uint16_t *a01, uint16_t *a10,
                                                  uint16_t *a11)
{
    const uint8_t *row0 = data + ly * row_bytes;
    const uint8_t *row1 = row0 + row_bytes;
    int idx = lx >> 1;

    if ((lx & 1) == 0)
    {
        uint8_t packed0 = row0[idx];
        uint8_t packed1 = row1[idx];

        *a00 = egui_alpha_change_table_4[packed0 & 0x0F];
        *a01 = egui_alpha_change_table_4[(packed0 >> 4) & 0x0F];
        *a10 = egui_alpha_change_table_4[packed1 & 0x0F];
        *a11 = egui_alpha_change_table_4[(packed1 >> 4) & 0x0F];
    }
    else
    {
        uint8_t packed00 = row0[idx];
        uint8_t packed01 = row0[idx + 1];
        uint8_t packed10 = row1[idx];
        uint8_t packed11 = row1[idx + 1];

        *a00 = egui_alpha_change_table_4[(packed00 >> 4) & 0x0F];
        *a01 = egui_alpha_change_table_4[packed01 & 0x0F];
        *a10 = egui_alpha_change_table_4[(packed10 >> 4) & 0x0F];
        *a11 = egui_alpha_change_table_4[packed11 & 0x0F];
    }
}

static inline uint8_t bilinear_alpha_from_raw_4(uint8_t a00, uint8_t a01, uint8_t a10, uint8_t a11, uint8_t fx, uint8_t fy)
{
    int32_t h0_q8 = ((int32_t)a00 << 8) + ((int32_t)a01 - a00) * fx;
    int32_t h1_q8 = ((int32_t)a10 << 8) + ((int32_t)a11 - a10) * fx;
    uint16_t ah0 = (uint16_t)(((h0_q8 << 4) + h0_q8 + 128) >> 8);
    uint16_t ah1 = (uint16_t)(((h1_q8 << 4) + h1_q8 + 128) >> 8);

    return (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));
}

/**
 * Batch extract 4 bilinear alpha samples from a single glyph's packed bitmap.
 * One switch evaluation instead of 4, shared row-byte-offset computation.
 * Requires (lx, ly) and (lx+1, ly+1) all within glyph bounds.
 */
static inline void batch_extract_glyph_alpha(const uint8_t *data, uint8_t box_w, int lx, int ly, uint8_t bpp, uint16_t *a00, uint16_t *a01, uint16_t *a10,
                                             uint16_t *a11)
{
    switch (bpp)
    {
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    case 4:
        batch_extract_glyph_alpha_4(data, box_w, lx, ly, a00, a01, a10, a11);
        break;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
    case 2:
    {
        int rb = (box_w + 3) >> 2;
        int base0 = ly * rb;
        int base1 = base0 + rb;
        int x0_idx = lx >> 2;
        int x0_bit = (lx & 3) << 1;
        int x1_idx = (lx + 1) >> 2;
        int x1_bit = ((lx + 1) & 3) << 1;
        *a00 = egui_alpha_change_table_2[(data[base0 + x0_idx] >> x0_bit) & 0x03];
        *a01 = egui_alpha_change_table_2[(data[base0 + x1_idx] >> x1_bit) & 0x03];
        *a10 = egui_alpha_change_table_2[(data[base1 + x0_idx] >> x0_bit) & 0x03];
        *a11 = egui_alpha_change_table_2[(data[base1 + x1_idx] >> x1_bit) & 0x03];
        break;
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
    case 1:
    {
        int rb = (box_w + 7) >> 3;
        int base0 = ly * rb;
        int base1 = base0 + rb;
        int x0_idx = lx >> 3;
        int x0_bit = lx & 7;
        int x1_idx = (lx + 1) >> 3;
        int x1_bit = (lx + 1) & 7;
        *a00 = (data[base0 + x0_idx] >> x0_bit) & 1 ? 255 : 0;
        *a01 = (data[base0 + x1_idx] >> x1_bit) & 1 ? 255 : 0;
        *a10 = (data[base1 + x0_idx] >> x0_bit) & 1 ? 255 : 0;
        *a11 = (data[base1 + x1_idx] >> x1_bit) & 1 ? 255 : 0;
        break;
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
    case 8:
    {
        int base0 = ly * box_w;
        int base1 = base0 + box_w;
        *a00 = data[base0 + lx];
        *a01 = data[base0 + lx + 1];
        *a10 = data[base1 + lx];
        *a11 = data[base1 + lx + 1];
        break;
    }
#endif
    default:
        *a00 = *a01 = *a10 = *a11 = 0;
        break;
    }
}

/**
 * Compute packed row byte count for a given width and bpp.
 */
static inline int packed_row_bytes(int width, uint8_t bpp)
{
    switch (bpp)
    {
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
    case 1:
        return (width + 7) >> 3;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
    case 2:
        return (width + 3) >> 2;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    case 4:
        return (width + 1) >> 1;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
    case 8:
        return width;
#endif
    default:
        return 0;
    }
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static int text_transform_external_glyph_row_cache_init(text_transform_external_glyph_row_cache_t *cache, int row_capacity)
{
    uint8_t *rows;

    if (cache == NULL)
    {
        return -1;
    }

    cache->glyph = NULL;
    cache->row0_y = -1;
    cache->row1_y = -1;
    cache->row_capacity = 0;
    cache->row0 = NULL;
    cache->row1 = NULL;

    if (row_capacity <= 0)
    {
        return 0;
    }

    rows = (uint8_t *)egui_malloc(row_capacity * 2);
    if (rows == NULL)
    {
        return -1;
    }

    cache->row_capacity = (uint16_t)row_capacity;
    cache->row0 = rows;
    cache->row1 = rows + row_capacity;
    return 0;
}

static void text_transform_external_glyph_row_cache_release(text_transform_external_glyph_row_cache_t *cache)
{
    if (cache == NULL)
    {
        return;
    }

    if (cache->row0 != NULL)
    {
        egui_free(cache->row0);
    }

    cache->glyph = NULL;
    cache->row0_y = -1;
    cache->row1_y = -1;
    cache->row_capacity = 0;
    cache->row0 = NULL;
    cache->row1 = NULL;
}

static int text_transform_external_layout_glyph_row_scratch_init(text_transform_external_layout_glyph_row_scratch_t *scratch, int row_capacity)
{
    enum
    {
        TEXT_TRANSFORM_EXTERNAL_LAYOUT_GLYPH_SCRATCH_ROWS = 14
    };
    int scratch_size;

    if (scratch == NULL)
    {
        return -1;
    }

    scratch->chunk_rows = 0;
    scratch->row_capacity = 0;
    scratch->row_buf = NULL;

    if (row_capacity <= 0)
    {
        return 0;
    }

    scratch_size = row_capacity * TEXT_TRANSFORM_EXTERNAL_LAYOUT_GLYPH_SCRATCH_ROWS;
    scratch->row_buf = (uint8_t *)egui_malloc(scratch_size);
    if (scratch->row_buf == NULL)
    {
        return -1;
    }

    scratch->chunk_rows = TEXT_TRANSFORM_EXTERNAL_LAYOUT_GLYPH_SCRATCH_ROWS;
    scratch->row_capacity = (uint16_t)row_capacity;
    return 0;
}

static void text_transform_external_layout_glyph_row_scratch_release(text_transform_external_layout_glyph_row_scratch_t *scratch)
{
    if (scratch == NULL)
    {
        return;
    }

    if (scratch->row_buf != NULL)
    {
        egui_free(scratch->row_buf);
    }

    scratch->chunk_rows = 0;
    scratch->row_capacity = 0;
    scratch->row_buf = NULL;
}

static int text_transform_load_external_layout_glyph_rows(const egui_font_std_info_t *font_info, const text_transform_layout_glyph_t *glyph, int start_row,
                                                          int row_count, text_transform_external_layout_glyph_row_scratch_t *scratch)
{
    int row_bytes;

    if (font_info == NULL || glyph == NULL || scratch == NULL || start_row < 0 || row_count <= 0 || (start_row + row_count) > glyph->box_h)
    {
        EGUI_ASSERT(0);
        return 0;
    }

    row_bytes = (glyph->box_w + 1) >> 1;
    if (row_bytes <= 0 || row_bytes > scratch->row_capacity || row_count > scratch->chunk_rows || scratch->row_buf == NULL)
    {
        EGUI_ASSERT(0);
        return 0;
    }

    egui_api_load_external_resource(scratch->row_buf, (egui_uintptr_t)font_info->pixel_buffer, glyph->pixel_idx + (uint32_t)start_row * row_bytes,
                                    row_bytes * row_count);
    return 1;
}

static int text_transform_measure_tile_max_row_bytes(const text_transform_glyph_t *glyphs, int glyph_count)
{
    int max_row_bytes = 0;

    if (glyphs == NULL || glyph_count <= 0)
    {
        return 0;
    }

    for (int i = 0; i < glyph_count; i++)
    {
        if (glyphs[i].row_bytes > max_row_bytes)
        {
            max_row_bytes = glyphs[i].row_bytes;
        }
    }

    return max_row_bytes;
}

static int text_transform_measure_visible_max_row_bytes(const text_transform_layout_glyph_t *layout_glyphs, const text_transform_layout_index_t *glyph_indices,
                                                        int glyph_count)
{
    int max_row_bytes = 0;

    if (layout_glyphs == NULL || glyph_indices == NULL || glyph_count <= 0)
    {
        return 0;
    }

    for (int i = 0; i < glyph_count; i++)
    {
        const text_transform_layout_glyph_t *glyph = &layout_glyphs[glyph_indices[i]];
        int row_bytes;

        if (glyph->box_w == 0)
        {
            continue;
        }

        row_bytes = (glyph->box_w + 1) >> 1;
        if (row_bytes > max_row_bytes)
        {
            max_row_bytes = row_bytes;
        }
    }

    return max_row_bytes;
}
#endif

static inline uint8_t extract_packed_raw(const uint8_t *data, int data_w, int x, int y, uint8_t bpp)
{
    switch (bpp)
    {
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
    case 1:
    {
        int rb = (data_w + 7) >> 3;
        return (data[y * rb + (x >> 3)] >> (x & 7)) & 1;
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
    case 2:
    {
        int rb = (data_w + 3) >> 2;
        return (data[y * rb + (x >> 2)] >> ((x & 3) << 1)) & 0x03;
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    case 4:
    {
        int rb = (data_w + 1) >> 1;
        return (data[y * rb + (x >> 1)] >> ((x & 1) << 2)) & 0x0F;
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
    case 8:
        return data[y * data_w + x];
#endif
    default:
        return 0;
    }
}

static inline void write_packed_raw(uint8_t *buf, int buf_w, int x, int y, uint8_t raw_val, uint8_t bpp)
{
    switch (bpp)
    {
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
    case 1:
    {
        int rb = (buf_w + 7) >> 3;
        buf[y * rb + (x >> 3)] |= (raw_val & 1) << (x & 7);
        break;
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
    case 2:
    {
        int rb = (buf_w + 3) >> 2;
        buf[y * rb + (x >> 2)] |= (raw_val & 0x03) << ((x & 3) << 1);
        break;
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    case 4:
    {
        int rb = (buf_w + 1) >> 1;
        buf[y * rb + (x >> 1)] |= (raw_val & 0x0F) << ((x & 1) << 2);
        break;
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
    case 8:
        buf[y * buf_w + x] = raw_val;
        break;
#endif
    }
}

/**
 * Read alpha value from a packed mask buffer (int width, supports text_w > 255).
 * Same logic as extract_packed_alpha() but with int width parameter.
 */
__EGUI_STATIC_INLINE__ uint8_t read_packed_mask_alpha(const uint8_t *buf, int buf_w, int x, int y, uint8_t bpp)
{
    switch (bpp)
    {
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
    case 1:
    {
        int rb = (buf_w + 7) >> 3;
        return (buf[y * rb + (x >> 3)] >> (x & 7)) & 1 ? 255 : 0;
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
    case 2:
    {
        int rb = (buf_w + 3) >> 2;
        return egui_alpha_change_table_2[(buf[y * rb + (x >> 2)] >> ((x & 3) << 1)) & 0x03];
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    case 4:
    {
        int rb = (buf_w + 1) >> 1;
        return egui_alpha_change_table_4[(buf[y * rb + (x >> 1)] >> ((x & 1) << 2)) & 0x0F];
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
    case 8:
        return buf[y * buf_w + x];
#endif
    default:
        return 0;
    }
}

/**
 * Compute common transform parameters for text rotation.
 * Returns 0 on success, -1 if draw region is empty (nothing to do).
 */
typedef struct
{
    int16_t src_w, src_h, cx, cy;
    int32_t inv_m00, inv_m01, inv_m10, inv_m11;
    int32_t draw_x0, draw_y0, draw_x1, draw_y1;
    int32_t Cx_base, Cy_base;
    egui_dim_t pfb_w, pfb_ox, pfb_oy;
    egui_color_int_t *pfb;
    egui_alpha_t canvas_alpha;
    egui_mask_t *mask; /* canvas mask for per-row color overlay (gradient support) */
} text_transform_ctx_t;

typedef struct
{
    uint8_t valid;
    int16_t text_w, text_h, cx, cy;
    egui_dim_t x, y;
    int16_t angle_deg;
    int16_t scale_q8;
    egui_alpha_t alpha;
    int32_t inv_m00, inv_m01, inv_m10, inv_m11;
    int32_t min_bx, min_by, max_bx, max_by;
    int32_t offx, offy;
} text_transform_prepare_cache_t;

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_PREPARE_CACHE_ENABLE
#define EGUI_CONFIG_TEXT_TRANSFORM_PREPARE_CACHE_ENABLE 1
#endif

#if EGUI_CONFIG_TEXT_TRANSFORM_PREPARE_CACHE_ENABLE
static text_transform_prepare_cache_t g_text_transform_prepare_cache = {0};
#endif

static int text_transform_draw_visible_alpha8_tile_layout(const text_transform_ctx_t *ctx, egui_dim_t x, egui_dim_t y, egui_color_t color,
                                                          const egui_font_std_info_t *font_info, const text_transform_layout_glyph_t *layout_glyphs,
                                                          const text_transform_layout_index_t *glyph_indices, int glyph_count, int16_t src_x0, int16_t src_y0,
                                                          int16_t src_x1, int16_t src_y1, int glyphs_overlap);

static int text_transform_prepare(int16_t text_w, int16_t text_h, egui_dim_t x, egui_dim_t y, int16_t angle_deg, int16_t scale_q8, egui_alpha_t alpha,
                                  text_transform_ctx_t *ctx)
{
#if EGUI_CONFIG_TEXT_TRANSFORM_PREPARE_CACHE_ENABLE
    text_transform_prepare_cache_t *cache = &g_text_transform_prepare_cache;
#else
    text_transform_prepare_cache_t local_cache = {0};
    text_transform_prepare_cache_t *cache = &local_cache;
#endif

    if (text_w <= 0 || text_h <= 0)
    {
        return -1;
    }

    egui_canvas_t *canvas = &canvas_data;
    ctx->src_w = text_w;
    ctx->src_h = text_h;
    ctx->cx = text_w / 2;
    ctx->cy = text_h / 2;

    if (scale_q8 == 0)
    {
        scale_q8 = 1;
    }

    if (!(cache->valid && cache->text_w == text_w && cache->text_h == text_h && cache->x == x && cache->y == y && cache->angle_deg == angle_deg &&
          cache->scale_q8 == scale_q8 && cache->alpha == alpha))
    {
        int32_t sinA = transform_sin_q15(angle_deg);
        int32_t cosA = transform_cos_q15(angle_deg);

        /* Forward affine matrix with scale (Q15) */
        int32_t fwd_m00 = (cosA * scale_q8) >> 8;
        int32_t fwd_m01 = (-sinA * scale_q8) >> 8;
        int32_t fwd_m10 = (sinA * scale_q8) >> 8;
        int32_t fwd_m11 = (cosA * scale_q8) >> 8;

        /* Compute bounding box of rotated text */
        int32_t min_bx = 0x7FFFFFFF, min_by = 0x7FFFFFFF;
        int32_t max_bx = -0x7FFFFFFF, max_by = -0x7FFFFFFF;
        int32_t corners[4][2] = {{0, 0}, {text_w, 0}, {0, text_h}, {text_w, text_h}};
        for (int i = 0; i < 4; i++)
        {
            int32_t dx_rel = corners[i][0] - ctx->cx;
            int32_t dy_rel = corners[i][1] - ctx->cy;
            int32_t dx = (((int64_t)dx_rel * fwd_m00 + (int64_t)dy_rel * fwd_m01) + (1 << 14)) >> 15;
            int32_t dy = (((int64_t)dx_rel * fwd_m10 + (int64_t)dy_rel * fwd_m11) + (1 << 14)) >> 15;
            dx += x;
            dy += y;
            if (dx < min_bx)
                min_bx = dx;
            if (dy < min_by)
                min_by = dy;
            if (dx > max_bx)
                max_bx = dx;
            if (dy > max_by)
                max_by = dy;
        }
        max_bx++;
        max_by++;

        cache->valid = 1;
        cache->text_w = text_w;
        cache->text_h = text_h;
        cache->cx = ctx->cx;
        cache->cy = ctx->cy;
        cache->x = x;
        cache->y = y;
        cache->angle_deg = angle_deg;
        cache->scale_q8 = scale_q8;
        cache->alpha = alpha;
        cache->inv_m00 = (cosA << 8) / scale_q8;
        cache->inv_m01 = (sinA << 8) / scale_q8;
        cache->inv_m10 = (-sinA << 8) / scale_q8;
        cache->inv_m11 = (cosA << 8) / scale_q8;
        cache->min_bx = min_bx;
        cache->min_by = min_by;
        cache->max_bx = max_bx;
        cache->max_by = max_by;
        cache->offx = (text_w & 1) ? 0 : (cache->inv_m00 + cache->inv_m01 - 32767) / 2;
        cache->offy = (text_h & 1) ? 0 : (cache->inv_m10 + cache->inv_m11 - 32767) / 2;
    }

    /* Clip to canvas work region */
    egui_region_t *work = &canvas->base_view_work_region;
    int32_t clip_x0 = work->location.x;
    int32_t clip_y0 = work->location.y;
    int32_t clip_x1 = clip_x0 + work->size.width;
    int32_t clip_y1 = clip_y0 + work->size.height;

    ctx->draw_x0 = (cache->min_bx > clip_x0) ? cache->min_bx : clip_x0;
    ctx->draw_y0 = (cache->min_by > clip_y0) ? cache->min_by : clip_y0;
    ctx->draw_x1 = (cache->max_bx < clip_x1) ? cache->max_bx : clip_x1;
    ctx->draw_y1 = (cache->max_by < clip_y1) ? cache->max_by : clip_y1;

    if (ctx->draw_x0 >= ctx->draw_x1 || ctx->draw_y0 >= ctx->draw_y1)
    {
        return -1;
    }

    ctx->inv_m00 = cache->inv_m00;
    ctx->inv_m01 = cache->inv_m01;
    ctx->inv_m10 = cache->inv_m10;
    ctx->inv_m11 = cache->inv_m11;

    /* PFB write setup */
    ctx->pfb_w = canvas->pfb_region.size.width;
    ctx->pfb_ox = canvas->pfb_location_in_base_view.x;
    ctx->pfb_oy = canvas->pfb_location_in_base_view.y;
    ctx->pfb = canvas->pfb;
    ctx->canvas_alpha = egui_color_alpha_mix(canvas->alpha, alpha);
    ctx->mask = canvas->mask;

    /* Row-constant terms for incremental scanning */
    ctx->Cx_base = ctx->inv_m01 * ((int32_t)ctx->draw_y0 - y) + ((int32_t)ctx->cx << 15) + cache->offx;
    ctx->Cy_base = ctx->inv_m11 * ((int32_t)ctx->draw_y0 - y) + ((int32_t)ctx->cy << 15) + cache->offy;

    return 0;
}

// ============================================================================
// Zero-buffer text transform: PFB-proportional memory, no malloc
// ============================================================================

/*
 * Max glyphs visible per PFB tile. Determined by PFB geometry and min glyph size.
 * Typical: PFB 60x60, 26pt font → ~6x3 = ~18 glyphs. With 45° rotation and 2x scale
 * the source bbox expands but is still bounded. Default 128 covers scale >= 0.5x.
 * Can be overridden in app_egui_config.h.
 */
#ifndef EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_GLYPHS
#define EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_GLYPHS 128
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_LINES
#define EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_LINES 32
#endif

#if EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_CACHE_ENABLE
static text_transform_layout_cache_t g_text_transform_layout_cache = {
        .font = NULL,
        .string = NULL,
        .line_space = 0,
        .count = 0,
        .line_count = 0,
};
#endif
#if EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_HEAP_ENABLE
static text_transform_layout_glyph_t *g_text_transform_layout_glyphs = NULL;
static int g_text_transform_layout_capacity = 0;
static text_transform_layout_line_t *g_text_transform_layout_lines = NULL;
static int g_text_transform_layout_line_capacity = 0;
#endif

typedef struct
{
    uint8_t *buf;
    int capacity;
} text_transform_visible_tile_cache_t;

static text_transform_visible_tile_cache_t g_text_transform_visible_tile_cache = {0};

static void text_transform_release_layout_cache(void)
{
#if EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_HEAP_ENABLE
    if (g_text_transform_layout_glyphs != NULL)
    {
        egui_free(g_text_transform_layout_glyphs);
        g_text_transform_layout_glyphs = NULL;
    }
    if (g_text_transform_layout_lines != NULL)
    {
        egui_free(g_text_transform_layout_lines);
        g_text_transform_layout_lines = NULL;
    }

#if EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_CACHE_ENABLE
    egui_api_memset(&g_text_transform_layout_cache, 0, sizeof(g_text_transform_layout_cache));
#endif
    g_text_transform_layout_capacity = 0;
    g_text_transform_layout_line_capacity = 0;
#endif
}

static void text_transform_release_visible_tile_cache(void)
{
    if (g_text_transform_visible_tile_cache.buf != NULL)
    {
        egui_free(g_text_transform_visible_tile_cache.buf);
    }

    egui_api_memset(&g_text_transform_visible_tile_cache, 0, sizeof(g_text_transform_visible_tile_cache));
}

static void text_transform_measure_layout_slots(const char *string, int *glyph_count, int *line_count)
{
    const char *s = string;
    int glyphs = 0;
    int lines = 0;
    uint32_t utf8_code;

    if (string != NULL && *string != '\0')
    {
        lines = 1;
    }

    while (*s != '\0')
    {
        if (*s == '\r')
        {
            s++;
            continue;
        }

        if (*s == '\n')
        {
            s++;
            lines++;
            continue;
        }

        int char_bytes = egui_font_get_utf8_code_fast(s, &utf8_code);
        if (char_bytes <= 0)
        {
            break;
        }

        s += char_bytes;
        glyphs++;
    }

    if (glyph_count != NULL)
    {
        *glyph_count = glyphs;
    }

    if (line_count != NULL)
    {
        *line_count = lines;
    }
}

__EGUI_STATIC_INLINE__ uint8_t read_packed_mask_alpha_4(const uint8_t *buf, int buf_w, int x, int y)
{
    int rb = (buf_w + 1) >> 1;
    uint8_t packed = buf[y * rb + (x >> 1)];
    return egui_alpha_change_table_4[(packed >> ((x & 1) << 2)) & 0x0F];
}

static inline uint8_t read_packed_mask_alpha_4_rb(const uint8_t *buf, int row_bytes, int x, int y)
{
    uint8_t packed = buf[y * row_bytes + (x >> 1)];
    return egui_alpha_change_table_4[(packed >> ((x & 1) << 2)) & 0x0F];
}

#if EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_HEAP_ENABLE
static int text_transform_ensure_layout_capacity(int needed)
{
    text_transform_layout_glyph_t *new_layout;

    if (needed <= g_text_transform_layout_capacity)
    {
        return 0;
    }

    new_layout = (text_transform_layout_glyph_t *)egui_malloc(needed * sizeof(text_transform_layout_glyph_t));
    if (new_layout == NULL)
    {
        return -1;
    }

    if (g_text_transform_layout_glyphs != NULL)
    {
        egui_free(g_text_transform_layout_glyphs);
    }

    g_text_transform_layout_glyphs = new_layout;
    g_text_transform_layout_capacity = needed;
    return 0;
}

static int text_transform_ensure_line_capacity(int needed)
{
    text_transform_layout_line_t *new_lines;

    if (needed <= g_text_transform_layout_line_capacity)
    {
        return 0;
    }

    new_lines = (text_transform_layout_line_t *)egui_malloc(needed * sizeof(text_transform_layout_line_t));
    if (new_lines == NULL)
    {
        return -1;
    }

    if (g_text_transform_layout_lines != NULL)
    {
        egui_free(g_text_transform_layout_lines);
    }

    g_text_transform_layout_lines = new_lines;
    g_text_transform_layout_line_capacity = needed;
    return 0;
}
#endif

void egui_canvas_transform_release_frame_cache(void)
{
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    image_transform_release_external_row_cache();
#endif

#if EGUI_CONFIG_TEXT_TRANSFORM_PREPARE_CACHE_ENABLE
    egui_api_memset(&g_text_transform_prepare_cache, 0, sizeof(g_text_transform_prepare_cache));
#endif
    text_transform_release_layout_cache();
    text_transform_release_visible_tile_cache();
}

static int text_transform_build_layout(const egui_font_std_info_t *font_info, const char *string, text_transform_layout_glyph_t *glyphs, int max_glyphs,
                                       text_transform_layout_line_t *lines, int max_lines, int *line_count, egui_dim_t line_space)
{
    const char *s = string;
    egui_font_std_char_descriptor_t external_desc_scratch;
    int cursor_x = 0;
    int cursor_y = 0;
    int count = 0;
    int current_line = 0;
    int current_line_start = 0;
    int current_line_has_glyph = 0;
    uint32_t utf8_code;

    if (line_count != NULL)
    {
        *line_count = 0;
    }

    if (max_lines > 0)
    {
        lines[0].start = 0;
        lines[0].end = 0;
        lines[0].x_min = 0;
        lines[0].x_max = 0;
        lines[0].y_min = 0;
        lines[0].y_max = 0;
    }

    while (*s != '\0')
    {
        if (*s == '\r')
        {
            s++;
            continue;
        }
        if (*s == '\n')
        {
            if (current_line < max_lines)
            {
                lines[current_line].start = current_line_start;
                lines[current_line].end = count;
                if (!current_line_has_glyph)
                {
                    lines[current_line].y_min = cursor_y;
                    lines[current_line].y_max = cursor_y;
                }
            }

            current_line++;
            current_line_start = count;
            current_line_has_glyph = 0;
            s++;
            cursor_x = 0;
            cursor_y += font_info->height + line_space;
            if (current_line < max_lines)
            {
                lines[current_line].start = count;
                lines[current_line].end = count;
                lines[current_line].x_min = 0;
                lines[current_line].x_max = 0;
                lines[current_line].y_min = cursor_y;
                lines[current_line].y_max = cursor_y;
            }
            continue;
        }

        int char_bytes = egui_font_get_utf8_code_fast(s, &utf8_code);
        const egui_font_std_char_descriptor_t *desc;

        if (char_bytes <= 0)
        {
            break;
        }
        s += char_bytes;

        desc = egui_font_std_get_desc_fast_api(font_info, utf8_code, &external_desc_scratch);
        if (desc == NULL || desc->size == 0)
        {
            cursor_x += font_info->height >> 1;
            continue;
        }

        if (count < max_glyphs)
        {
            if (desc->idx > TEXT_TRANSFORM_LAYOUT_PIXEL_IDX_MAX)
            {
                return -1;
            }

            glyphs[count].pixel_idx = (text_transform_layout_pixel_idx_t)desc->idx;
            glyphs[count].x = cursor_x + desc->off_x;
            glyphs[count].y = cursor_y + desc->off_y;
            glyphs[count].box_w = desc->box_w;
            glyphs[count].box_h = desc->box_h;

            if (current_line < max_lines)
            {
                int16_t glyph_y_min = glyphs[count].y;
                int16_t glyph_y_max = glyphs[count].y + glyphs[count].box_h;

                if (!current_line_has_glyph)
                {
                    lines[current_line].start = count;
                    lines[current_line].x_min = glyphs[count].x;
                    lines[current_line].x_max = glyphs[count].x + glyphs[count].box_w;
                    lines[current_line].y_min = glyph_y_min;
                    lines[current_line].y_max = glyph_y_max;
                }
                else
                {
                    int16_t glyph_x_min = glyphs[count].x;
                    int16_t glyph_x_max = glyphs[count].x + glyphs[count].box_w;

                    if (glyph_x_min < lines[current_line].x_min)
                    {
                        lines[current_line].x_min = glyph_x_min;
                    }
                    if (glyph_x_max > lines[current_line].x_max)
                    {
                        lines[current_line].x_max = glyph_x_max;
                    }
                    if (glyph_y_min < lines[current_line].y_min)
                    {
                        lines[current_line].y_min = glyph_y_min;
                    }
                    if (glyph_y_max > lines[current_line].y_max)
                    {
                        lines[current_line].y_max = glyph_y_max;
                    }
                }
                lines[current_line].end = count + 1;
            }

            current_line_has_glyph = 1;
            count++;
        }

        cursor_x += desc->adv;
    }

    if (current_line < max_lines)
    {
        lines[current_line].start = current_line_start;
        lines[current_line].end = count;
        if (!current_line_has_glyph)
        {
            lines[current_line].y_min = cursor_y;
            lines[current_line].y_max = cursor_y;
        }
    }

    if (line_count != NULL)
    {
        *line_count = current_line + ((string != NULL && *string != '\0') ? 1 : 0);
    }

    return count;
}

#if EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_HEAP_ENABLE
static int text_transform_prepare_layout(const egui_font_t *font, const egui_font_std_info_t *font_info, const char *string, egui_dim_t line_space,
                                         const text_transform_layout_glyph_t **layout, int *count, const text_transform_layout_line_t **lines, int *line_count)
{
    int build_count;
    int needed;
    int line_needed;
#if EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_CACHE_ENABLE
    text_transform_layout_cache_t *cache = &g_text_transform_layout_cache;
#else
    text_transform_layout_cache_t local_cache = {0};
    text_transform_layout_cache_t *cache = &local_cache;
#endif

    if (font == NULL || font_info == NULL || string == NULL || layout == NULL || count == NULL || lines == NULL || line_count == NULL)
    {
        return -1;
    }

    if (cache->font == font && cache->string == string && cache->line_space == line_space)
    {
        *layout = g_text_transform_layout_glyphs;
        *count = cache->count;
        *lines = g_text_transform_layout_lines;
        *line_count = cache->line_count;
        return 0;
    }

    text_transform_measure_layout_slots(string, &needed, &line_needed);
    if (needed > TEXT_TRANSFORM_LAYOUT_INDEX_MAX)
    {
        return -1;
    }
    if (needed > 0 && text_transform_ensure_layout_capacity(needed) != 0)
    {
        return -1;
    }
    if (line_needed > 0 && text_transform_ensure_line_capacity(line_needed) != 0)
    {
        return -1;
    }

    cache->font = font;
    cache->string = string;
    cache->line_space = line_space;
    build_count = text_transform_build_layout(font_info, string, g_text_transform_layout_glyphs, needed, g_text_transform_layout_lines, line_needed,
                                              &cache->line_count, line_space);
    if (build_count < 0)
    {
#if EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_CACHE_ENABLE
        egui_api_memset(cache, 0, sizeof(*cache));
#endif
        return -1;
    }
    cache->count = build_count;

    *layout = g_text_transform_layout_glyphs;
    *count = cache->count;
    *lines = g_text_transform_layout_lines;
    *line_count = cache->line_count;
    return 0;
}
#endif

/**
 * Collect only cached layout glyphs that overlap the source bounding box [sx0..sx1, sy0..sy1].
 * Returns number of glyphs stored.
 */
static int collect_visible_glyphs(const egui_font_std_info_t *font_info, const text_transform_layout_glyph_t *layout_glyphs,
                                  const text_transform_layout_line_t *layout_lines, int layout_line_count, text_transform_glyph_t *glyphs, int max_glyphs,
                                  text_transform_layout_line_t *visible_lines, int max_visible_lines, int *visible_line_count, int16_t sx0, int16_t sy0,
                                  int16_t sx1, int16_t sy1, int16_t *bbox_x0, int16_t *bbox_y0, int16_t *bbox_x1, int16_t *bbox_y1, int *glyphs_overlap,
                                  int with_sample_meta)
{
    int count = 0;
    int line_count = 0;
    int bbox_found = 0;
    int overlap_found = 0;
    int prev_visible_line_found = 0;
    int16_t prev_visible_line_y_max = 0;

    if (visible_line_count != NULL)
    {
        *visible_line_count = 0;
    }
    if (glyphs_overlap != NULL)
    {
        *glyphs_overlap = 0;
    }
    if (bbox_x0 != NULL)
    {
        *bbox_x0 = 0;
    }
    if (bbox_y0 != NULL)
    {
        *bbox_y0 = 0;
    }
    if (bbox_x1 != NULL)
    {
        *bbox_x1 = 0;
    }
    if (bbox_y1 != NULL)
    {
        *bbox_y1 = 0;
    }

    for (int line = 0; line < layout_line_count; line++)
    {
        const text_transform_layout_line_t *src_line = &layout_lines[line];
        int line_start = count;
        int16_t line_x_min = 0;
        int16_t line_x_max = 0;
        int16_t line_rightmost_x = 0;

        if (src_line->end <= src_line->start)
        {
            continue;
        }

        if (src_line->y_max <= sy0)
        {
            continue;
        }

        if (src_line->y_min >= sy1)
        {
            break;
        }

        if (src_line->x_max <= sx0 || src_line->x_min >= sx1)
        {
            continue;
        }

        for (int i = src_line->start; i < src_line->end; i++)
        {
            const text_transform_layout_glyph_t *src = &layout_glyphs[i];

            if (src->x >= sx1)
            {
                break;
            }

            if (src->x + src->box_w <= sx0)
            {
                continue;
            }

            /* Check overlap with source bbox */
            if (src->y + src->box_h > sy0 && src->y < sy1)
            {
                if (count < max_glyphs)
                {
                    int16_t glyph_x0 = src->x;
                    int16_t glyph_y0 = src->y;
                    int16_t glyph_x1 = src->x + src->box_w;
                    int16_t glyph_y1 = src->y + src->box_h;

                    if (count == line_start)
                    {
                        line_x_min = src->x;
                        line_x_max = src->x + src->box_w;
                        line_rightmost_x = glyph_x1;
                    }
                    else
                    {
                        if (src->x < line_x_min)
                        {
                            line_x_min = src->x;
                        }
                        if (src->x + src->box_w > line_x_max)
                        {
                            line_x_max = src->x + src->box_w;
                        }
                        if (glyphs_overlap != NULL && glyph_x0 < line_rightmost_x)
                        {
                            overlap_found = 1;
                        }
                        if (glyph_x1 > line_rightmost_x)
                        {
                            line_rightmost_x = glyph_x1;
                        }
                    }
                    if (!bbox_found)
                    {
                        if (bbox_x0 != NULL)
                        {
                            *bbox_x0 = glyph_x0;
                        }
                        if (bbox_y0 != NULL)
                        {
                            *bbox_y0 = glyph_y0;
                        }
                        if (bbox_x1 != NULL)
                        {
                            *bbox_x1 = glyph_x1;
                        }
                        if (bbox_y1 != NULL)
                        {
                            *bbox_y1 = glyph_y1;
                        }
                        bbox_found = 1;
                    }
                    else
                    {
                        if (bbox_x0 != NULL && glyph_x0 < *bbox_x0)
                        {
                            *bbox_x0 = glyph_x0;
                        }
                        if (bbox_y0 != NULL && glyph_y0 < *bbox_y0)
                        {
                            *bbox_y0 = glyph_y0;
                        }
                        if (bbox_x1 != NULL && glyph_x1 > *bbox_x1)
                        {
                            *bbox_x1 = glyph_x1;
                        }
                        if (bbox_y1 != NULL && glyph_y1 > *bbox_y1)
                        {
                            *bbox_y1 = glyph_y1;
                        }
                    }
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                    if (font_info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
                    {
                        glyphs[count].data = font_info->pixel_buffer;
                        glyphs[count].pixel_idx = src->pixel_idx;
                    }
                    else
#endif
                    {
                        glyphs[count].data = font_info->pixel_buffer + src->pixel_idx;
                        glyphs[count].pixel_idx = 0;
                    }
                    glyphs[count].x = src->x;
                    glyphs[count].y = src->y;
                    glyphs[count].box_w = src->box_w;
                    glyphs[count].box_h = src->box_h;
                    if (with_sample_meta)
                    {
                        glyphs[count].row_bytes = (uint8_t)packed_row_bytes(src->box_w, font_info->font_bit_mode);
                        glyphs[count].line = (uint8_t)line_count;
                    }
                    count++;
                }
            }
        }

        if (with_sample_meta && count > line_start && line_count < max_visible_lines)
        {
            visible_lines[line_count].start = line_start;
            visible_lines[line_count].end = count;
            visible_lines[line_count].x_min = line_x_min;
            visible_lines[line_count].x_max = line_x_max;
            visible_lines[line_count].y_min = src_line->y_min;
            visible_lines[line_count].y_max = src_line->y_max;
            line_count++;
        }
        if (count > line_start)
        {
            if (glyphs_overlap != NULL && prev_visible_line_found && src_line->y_min < prev_visible_line_y_max)
            {
                overlap_found = 1;
            }
            if (!prev_visible_line_found || src_line->y_max > prev_visible_line_y_max)
            {
                prev_visible_line_y_max = src_line->y_max;
            }
            prev_visible_line_found = 1;
        }
    }

    if (visible_line_count != NULL)
    {
        *visible_line_count = line_count;
    }
    if (glyphs_overlap != NULL)
    {
        *glyphs_overlap = overlap_found;
    }

    return count;
}

static int collect_visible_layout_glyphs_alpha8(const text_transform_layout_glyph_t *layout_glyphs, const text_transform_layout_line_t *layout_lines,
                                                int layout_line_count, text_transform_layout_index_t *glyph_indices, int max_glyphs, int16_t sx0, int16_t sy0,
                                                int16_t sx1, int16_t sy1, int16_t *bbox_x0, int16_t *bbox_y0, int16_t *bbox_x1, int16_t *bbox_y1,
                                                int *glyphs_overlap)
{
    int count = 0;
    int bbox_found = 0;
    int overlap_found = 0;
    int prev_visible_line_found = 0;
    int16_t prev_visible_line_y_max = 0;
    int16_t local_bbox_x0 = 0;
    int16_t local_bbox_y0 = 0;
    int16_t local_bbox_x1 = 0;
    int16_t local_bbox_y1 = 0;

    for (int line = 0; line < layout_line_count; line++)
    {
        const text_transform_layout_line_t *src_line = &layout_lines[line];
        int line_start = count;
        int16_t line_rightmost_x = 0;

        if (src_line->end <= src_line->start)
        {
            continue;
        }
        if (src_line->y_max <= sy0)
        {
            continue;
        }
        if (src_line->y_min >= sy1)
        {
            break;
        }
        if (src_line->x_max <= sx0 || src_line->x_min >= sx1)
        {
            continue;
        }

        for (int i = src_line->start; i < src_line->end; i++)
        {
            const text_transform_layout_glyph_t *src = &layout_glyphs[i];

            if (src->x >= sx1)
            {
                break;
            }
            if (src->x + src->box_w <= sx0)
            {
                continue;
            }
            if (src->y + src->box_h > sy0 && src->y < sy1)
            {
                int16_t glyph_x0 = src->x;
                int16_t glyph_y0 = src->y;
                int16_t glyph_x1 = glyph_x0 + src->box_w;
                int16_t glyph_y1 = glyph_y0 + src->box_h;

                if (count < max_glyphs)
                {
                    glyph_indices[count++] = (text_transform_layout_index_t)i;
                }

                if (count == line_start + 1)
                {
                    line_rightmost_x = glyph_x1;
                }
                else
                {
                    if (glyph_x0 < line_rightmost_x)
                    {
                        overlap_found = 1;
                    }
                    if (glyph_x1 > line_rightmost_x)
                    {
                        line_rightmost_x = glyph_x1;
                    }
                }

                if (!bbox_found)
                {
                    local_bbox_x0 = glyph_x0;
                    local_bbox_y0 = glyph_y0;
                    local_bbox_x1 = glyph_x1;
                    local_bbox_y1 = glyph_y1;
                    bbox_found = 1;
                }
                else
                {
                    if (glyph_x0 < local_bbox_x0)
                    {
                        local_bbox_x0 = glyph_x0;
                    }
                    if (glyph_y0 < local_bbox_y0)
                    {
                        local_bbox_y0 = glyph_y0;
                    }
                    if (glyph_x1 > local_bbox_x1)
                    {
                        local_bbox_x1 = glyph_x1;
                    }
                    if (glyph_y1 > local_bbox_y1)
                    {
                        local_bbox_y1 = glyph_y1;
                    }
                }
            }
        }

        if (count > line_start)
        {
            if (prev_visible_line_found && src_line->y_min < prev_visible_line_y_max)
            {
                overlap_found = 1;
            }
            if (!prev_visible_line_found || src_line->y_max > prev_visible_line_y_max)
            {
                prev_visible_line_y_max = src_line->y_max;
            }
            prev_visible_line_found = 1;
        }
    }

    if (bbox_x0 != NULL)
    {
        *bbox_x0 = local_bbox_x0;
    }
    if (bbox_y0 != NULL)
    {
        *bbox_y0 = local_bbox_y0;
    }
    if (bbox_x1 != NULL)
    {
        *bbox_x1 = local_bbox_x1;
    }
    if (bbox_y1 != NULL)
    {
        *bbox_y1 = local_bbox_y1;
    }
    if (glyphs_overlap != NULL)
    {
        *glyphs_overlap = overlap_found;
    }

    return count;
}

/**
 * Sample text alpha by searching the PFB-local glyph array.
 * With typically <20 glyph entries, linear scan with hint is efficient.
 */
static inline uint8_t sample_tile_alpha_from_line(const text_transform_glyph_t *glyphs, const text_transform_layout_line_t *line, int line_index, int sx,
                                                  int sy, uint8_t bpp, int *hint, int *hint_line)
{
    for (int i = line->start; i < line->end; i++)
    {
        const text_transform_glyph_t *g = &glyphs[i];
        int lx = sx - g->x;

        if (lx < 0)
        {
            break;
        }

        if (lx >= g->box_w)
        {
            continue;
        }

        int ly = sy - g->y;
        if (ly < 0 || ly >= g->box_h)
        {
            continue;
        }

        *hint = i;
        *hint_line = line_index;
        return (bpp == 4) ? extract_packed_alpha_4(g->data, g->box_w, lx, ly) : extract_packed_alpha(g->data, g->box_w, lx, ly, bpp);
    }

    *hint_line = line_index;
    return 0;
}

static inline uint8_t sample_tile_alpha_from_line_4(const text_transform_glyph_t *glyphs, const text_transform_layout_line_t *line, int line_index, int sx,
                                                    int sy, int *hint, int *hint_line)
{
    for (int i = line->start; i < line->end; i++)
    {
        const text_transform_glyph_t *g = &glyphs[i];
        int lx = sx - g->x;

        if (lx < 0)
        {
            break;
        }

        if (lx >= g->box_w)
        {
            continue;
        }

        {
            int ly = sy - g->y;
            if (ly < 0 || ly >= g->box_h)
            {
                continue;
            }

            *hint = i;
            *hint_line = line_index;
            return extract_packed_alpha_4_rb(g->data, g->row_bytes, lx, ly);
        }
    }

    *hint_line = line_index;
    return 0;
}

static inline int sample_tile_find_pair_line(const text_transform_layout_line_t *lines, int line_count, int sx, int sy, int hint_line)
{
    int sx_next = sx + 1;

    if (sy < 0)
    {
        return -1;
    }

    if (hint_line >= 0 && hint_line < line_count)
    {
        const text_transform_layout_line_t *line = &lines[hint_line];

        if (sy >= line->y_min && sy < line->y_max && sx < line->x_max && sx_next >= line->x_min)
        {
            return hint_line;
        }
    }

    for (int i = 0; i < line_count; i++)
    {
        const text_transform_layout_line_t *line = &lines[i];

        if (sy < line->y_min)
        {
            break;
        }

        if (sy >= line->y_max)
        {
            continue;
        }

        if (sx >= line->x_max || sx_next < line->x_min)
        {
            continue;
        }

        return i;
    }

    return -1;
}

static inline void sample_tile_alpha_pair_from_line(const text_transform_glyph_t *glyphs, const text_transform_layout_line_t *line, int sx, int sy, uint8_t bpp,
                                                    uint16_t *a0, uint16_t *a1, int *hint0, int *hint1)
{
    int sx_next = sx + 1;

    *a0 = 0;
    *a1 = 0;
    *hint0 = -1;
    *hint1 = -1;

    for (int i = line->start; i < line->end; i++)
    {
        const text_transform_glyph_t *g = &glyphs[i];
        int ly = sy - g->y;

        if (g->x > sx_next)
        {
            break;
        }

        if (g->x + g->box_w <= sx || ly < 0 || ly >= g->box_h)
        {
            continue;
        }

        if (bpp == 4)
        {
            if (sx >= g->x && sx_next < g->x + g->box_w)
            {
                int lx = sx - g->x;

                *hint0 = i;
                *hint1 = i;
                extract_packed_alpha_pair_4_rb(g->data, g->row_bytes, lx, ly, a0, a1);
                break;
            }

            if (sx >= g->x && sx < g->x + g->box_w)
            {
                int lx = sx - g->x;

                *hint0 = i;
                *a0 = extract_packed_alpha_4_rb(g->data, g->row_bytes, lx, ly);
            }

            if (sx_next >= g->x && sx_next < g->x + g->box_w)
            {
                int lx = sx_next - g->x;

                *hint1 = i;
                *a1 = extract_packed_alpha_4_rb(g->data, g->row_bytes, lx, ly);
            }
        }
        else
        {
            if (sx >= g->x && sx < g->x + g->box_w)
            {
                int lx = sx - g->x;

                *hint0 = i;
                *a0 = extract_packed_alpha(g->data, g->box_w, lx, ly, bpp);
            }

            if (sx_next >= g->x && sx_next < g->x + g->box_w)
            {
                int lx = sx_next - g->x;

                *hint1 = i;
                *a1 = extract_packed_alpha(g->data, g->box_w, lx, ly, bpp);
            }
        }

        if (*hint0 >= 0 && *hint1 >= 0)
        {
            break;
        }
    }
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static inline void sample_tile_alpha_pair_from_line_external(const text_transform_glyph_t *glyphs, const text_transform_layout_line_t *line, int sx, int sy,
                                                             uint8_t bpp, uint16_t *a0, uint16_t *a1, int *hint0, int *hint1,
                                                             text_transform_external_glyph_row_cache_t *row_cache)
{
    int sx_next = sx + 1;

    *a0 = 0;
    *a1 = 0;
    *hint0 = -1;
    *hint1 = -1;

    for (int i = line->start; i < line->end; i++)
    {
        const text_transform_glyph_t *g = &glyphs[i];
        int ly = sy - g->y;

        if (g->x > sx_next)
        {
            break;
        }

        if (g->x + g->box_w <= sx || ly < 0 || ly >= g->box_h)
        {
            continue;
        }

        if (sx >= g->x && sx_next < g->x + g->box_w)
        {
            int lx = sx - g->x;

            *hint0 = i;
            *hint1 = i;
            if (!text_transform_extract_external_glyph_alpha_pair_from_row(g, lx, ly, bpp, row_cache, a0, a1))
            {
                *a0 = 0;
                *a1 = 0;
                *hint0 = -1;
                *hint1 = -1;
            }
            break;
        }

        if (sx >= g->x && sx < g->x + g->box_w)
        {
            int lx = sx - g->x;

            *hint0 = i;
            if (!text_transform_extract_external_glyph_alpha_from_row(g, lx, ly, bpp, row_cache, a0))
            {
                *a0 = 0;
                *hint0 = -1;
            }
        }

        if (sx_next >= g->x && sx_next < g->x + g->box_w)
        {
            int lx = sx_next - g->x;

            *hint1 = i;
            if (!text_transform_extract_external_glyph_alpha_from_row(g, lx, ly, bpp, row_cache, a1))
            {
                *a1 = 0;
                *hint1 = -1;
            }
        }

        if (*hint0 >= 0 && *hint1 >= 0)
        {
            break;
        }
    }
}
#endif

__EGUI_STATIC_INLINE__ uint8_t sample_tile_alpha(const text_transform_glyph_t *glyphs, int count, const text_transform_layout_line_t *lines, int line_count,
                                                 int sx, int sy, uint8_t bpp, int *hint, int *hint_line)
{
    if (sx < 0 || sy < 0)
    {
        return 0;
    }

    /* Fast path: check cached glyph */
    if (*hint >= 0 && *hint < count)
    {
        const text_transform_glyph_t *g = &glyphs[*hint];
        int lx = sx - g->x;
        if (lx >= 0 && lx < g->box_w)
        {
            int ly = sy - g->y;
            if (ly >= 0 && ly < g->box_h)
            {
                *hint_line = g->line;
                return (bpp == 4) ? extract_packed_alpha_4(g->data, g->box_w, lx, ly) : extract_packed_alpha(g->data, g->box_w, lx, ly, bpp);
            }
        }
    }

    if (*hint_line >= 0 && *hint_line < line_count)
    {
        const text_transform_layout_line_t *line = &lines[*hint_line];

        if (sy >= line->y_min && sy < line->y_max && sx >= line->x_min && sx < line->x_max)
        {
            uint8_t alpha = sample_tile_alpha_from_line(glyphs, line, *hint_line, sx, sy, bpp, hint, hint_line);
            if (alpha > 0)
            {
                return alpha;
            }
        }
    }

    for (int i = 0; i < line_count; i++)
    {
        const text_transform_layout_line_t *line = &lines[i];

        if (sy < line->y_min)
        {
            break;
        }

        if (sy >= line->y_max || i == *hint_line)
        {
            continue;
        }

        if (sx < line->x_min || sx >= line->x_max)
        {
            continue;
        }

        {
            uint8_t alpha = sample_tile_alpha_from_line(glyphs, line, i, sx, sy, bpp, hint, hint_line);
            if (alpha > 0)
            {
                return alpha;
            }
        }
    }

    /* No glyph found: signal whitespace to caller for early exit */
    *hint = -1;
    return 0;
}

__EGUI_STATIC_INLINE__ uint8_t sample_tile_alpha_4(const text_transform_glyph_t *glyphs, int count, const text_transform_layout_line_t *lines, int line_count,
                                                   int sx, int sy, int *hint, int *hint_line)
{
    if (sx < 0 || sy < 0)
    {
        return 0;
    }

    if (*hint >= 0 && *hint < count)
    {
        const text_transform_glyph_t *g = &glyphs[*hint];
        int lx = sx - g->x;
        if (lx >= 0 && lx < g->box_w)
        {
            int ly = sy - g->y;
            if (ly >= 0 && ly < g->box_h)
            {
                *hint_line = g->line;
                return extract_packed_alpha_4_rb(g->data, g->row_bytes, lx, ly);
            }
        }
    }

    if (*hint_line >= 0 && *hint_line < line_count)
    {
        const text_transform_layout_line_t *line = &lines[*hint_line];

        if (sy >= line->y_min && sy < line->y_max && sx >= line->x_min && sx < line->x_max)
        {
            uint8_t alpha = sample_tile_alpha_from_line_4(glyphs, line, *hint_line, sx, sy, hint, hint_line);
            if (alpha > 0)
            {
                return alpha;
            }
        }
    }

    for (int i = 0; i < line_count; i++)
    {
        const text_transform_layout_line_t *line = &lines[i];

        if (sy < line->y_min)
        {
            break;
        }

        if (sy >= line->y_max || i == *hint_line)
        {
            continue;
        }

        if (sx < line->x_min || sx >= line->x_max)
        {
            continue;
        }

        {
            uint8_t alpha = sample_tile_alpha_from_line_4(glyphs, line, i, sx, sy, hint, hint_line);
            if (alpha > 0)
            {
                return alpha;
            }
        }
    }

    *hint = -1;
    return 0;
}

static int text_transform_try_draw_axis_aligned(const egui_font_t *font, const void *string, egui_dim_t x, egui_dim_t y, int16_t angle_deg, int16_t scale_q8,
                                                egui_color_t color, egui_alpha_t alpha)
{
    static const egui_font_t *s_axis_font = NULL;
    static const void *s_axis_string = NULL;
    static egui_dim_t s_axis_w = 0;
    static egui_dim_t s_axis_h = 0;
    egui_region_t rect;
    int16_t norm_angle = angle_deg % 360;

    if (norm_angle < 0)
    {
        norm_angle += 360;
    }

    if (norm_angle != 0 || scale_q8 != 256 || font == NULL || string == NULL)
    {
        return 0;
    }

    if (font != s_axis_font || string != s_axis_string)
    {
        egui_dim_t tw = 0;
        egui_dim_t th = 0;

        font->api->get_str_size(font, string, 1, 0, &tw, &th);
        s_axis_w = tw;
        s_axis_h = th;
        s_axis_font = font;
        s_axis_string = string;
    }

    if (s_axis_w <= 0 || s_axis_h <= 0)
    {
        return 1;
    }

    rect.location.x = x - (s_axis_w >> 1);
    rect.location.y = y - (s_axis_h >> 1);
    rect.size.width = s_axis_w;
    rect.size.height = s_axis_h;
    egui_canvas_draw_text_in_rect_with_line_space(font, string, &rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, 0, color, alpha);
    return 1;
}

/**
 * Draw rotated and scaled text (zero-buffer, PFB-proportional memory).
 *
 * Per PFB tile: inverse-maps tile corners to find source bbox, walks string to
 * collect only visible glyphs into per-draw scratch, then samples directly
 * from packed ROM font data. Extra scratch size depends on visible glyph count,
 * line count, and transformed tile bounds instead of total text content.
 * When EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE=1, the layout/tile
 * collectors use transient heap sized by the measured glyph and line count.
 * Text dimensions are cached (12 bytes static) to avoid per-tile string measurement.
 */
__EGUI_OPTIMIZE_SIZE__ void egui_canvas_draw_text_transform(const egui_font_t *font, const void *string, egui_dim_t x, egui_dim_t y, int16_t angle_deg,
                                                                     int16_t scale_q8, egui_color_t color, egui_alpha_t alpha)
{
    if (!font || !string || !font->res)
    {
        return;
    }

    if (text_transform_try_draw_axis_aligned(font, string, x, y, angle_deg, scale_q8, color, alpha))
    {
        return;
    }

#if EGUI_CONFIG_TEXT_TRANSFORM_DIM_CACHE_ENABLE
    /* Lightweight dimension cache: avoid per-tile get_str_size string walk.
     * Only 12 bytes static, independent of text content/font size. */
    static const egui_font_t *s_dim_font = NULL;
    static const void *s_dim_string = NULL;
    static int16_t s_dim_w = 0, s_dim_h = 0;

    if (font != s_dim_font || string != s_dim_string)
    {
        egui_dim_t tw = 0, th = 0;
        font->api->get_str_size(font, string, 1, 0, &tw, &th);
        s_dim_w = tw;
        s_dim_h = th;
        s_dim_font = font;
        s_dim_string = string;
    }
#else
    int16_t s_dim_w = 0;
    int16_t s_dim_h = 0;
    {
        egui_dim_t tw = 0;
        egui_dim_t th = 0;

        font->api->get_str_size(font, string, 1, 0, &tw, &th);
        s_dim_w = tw;
        s_dim_h = th;
    }
#endif

    text_transform_ctx_t ctx;
    if (text_transform_prepare(s_dim_w, s_dim_h, x, y, angle_deg, scale_q8, alpha, &ctx) != 0)
    {
        return;
    }

    egui_font_std_info_t *font_info = (egui_font_std_info_t *)font->res;
    uint8_t bpp = font_info->font_bit_mode;
    const text_transform_layout_glyph_t *layout_glyphs = NULL;
    int layout_count = 0;
    const text_transform_layout_line_t *layout_lines = NULL;
    int layout_line_count = 0;
#if EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE
    text_transform_layout_scratch_t layout_scratch = {0};
#else
#if EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_GLYPHS > 0 && EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_LINES > 0
    text_transform_layout_glyph_t stack_layout_glyphs[EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_GLYPHS];
    text_transform_layout_line_t stack_layout_lines[EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_LINES];
#endif
#endif
#if EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE
    text_transform_tile_scratch_t tile_scratch = {0};
    text_transform_glyph_t *tile_glyphs = NULL;
    text_transform_layout_index_t *tile_alpha8_layout_glyph_indices = NULL;
    text_transform_layout_line_t *tile_lines = NULL;
    int tile_glyph_capacity = 0;
    int tile_line_capacity = 0;
#else
    text_transform_glyph_t tile_glyphs_storage[EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_GLYPHS];
    text_transform_layout_index_t tile_layout_glyph_indices_storage[EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_GLYPHS];
    text_transform_layout_line_t tile_lines_storage[EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_LINES];
    text_transform_glyph_t *tile_glyphs = tile_glyphs_storage;
    text_transform_layout_index_t *tile_alpha8_layout_glyph_indices = tile_layout_glyph_indices_storage;
    text_transform_layout_line_t *tile_lines = tile_lines_storage;
    int tile_glyph_capacity = EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_GLYPHS;
    int tile_line_capacity = EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_LINES;
#endif
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    int draw_font_is_external = (font_info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL);
    text_transform_external_glyph_row_cache_t external_row_cache;
    int external_row_cache_ready = 0;
#else
    int draw_font_is_external = 0;
#endif

    {
        const char *text = (const char *)string;

#if EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE
        int scratch_needed = 0;
        int scratch_line_needed = 0;

        text_transform_measure_layout_slots(text, &scratch_needed, &scratch_line_needed);
        if (scratch_needed == 0 || scratch_line_needed == 0 || scratch_needed > TEXT_TRANSFORM_LAYOUT_INDEX_MAX)
        {
            return;
        }

        if (text_transform_prepare_layout_scratch(scratch_needed, scratch_line_needed, &layout_scratch) != 0)
        {
            return;
        }

        layout_count = text_transform_build_layout(font_info, text, layout_scratch.glyphs, scratch_needed, layout_scratch.lines, scratch_line_needed,
                                                   &layout_line_count, 0);
        if (layout_count < 0)
        {
            goto cleanup;
        }

        layout_glyphs = layout_scratch.glyphs;
        layout_lines = layout_scratch.lines;
#else
#if EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_GLYPHS > 0 && EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_LINES > 0
        int stack_needed = 0;
        int stack_line_needed = 0;

        text_transform_measure_layout_slots(text, &stack_needed, &stack_line_needed);
        if (stack_needed == 0 || stack_line_needed == 0)
        {
            return;
        }

        if (stack_needed > 0 && stack_needed <= EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_GLYPHS && stack_line_needed > 0 &&
            stack_line_needed <= EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_STACK_MAX_LINES && stack_needed <= TEXT_TRANSFORM_LAYOUT_INDEX_MAX)
        {
            layout_count = text_transform_build_layout(font_info, text, stack_layout_glyphs, stack_needed, stack_layout_lines, stack_line_needed,
                                                       &layout_line_count, 0);
            if (layout_count < 0)
            {
                return;
            }
            layout_glyphs = stack_layout_glyphs;
            layout_lines = stack_layout_lines;
        }
        else
#endif
#if EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_HEAP_ENABLE
                if (text_transform_prepare_layout(font, font_info, text, 0, &layout_glyphs, &layout_count, &layout_lines, &layout_line_count) != 0)
        {
            return;
        }
#else
        {
            EGUI_ASSERT(0);
            return;
        }
#endif
#endif
    }

    if (layout_glyphs == NULL || layout_lines == NULL)
    {
        goto cleanup;
    }

    if (layout_count == 0)
    {
        goto cleanup;
    }

#if EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE
    tile_glyph_capacity = layout_count;
    tile_line_capacity = layout_line_count;
    if (tile_glyph_capacity <= 0 || tile_line_capacity <= 0)
    {
        goto cleanup;
    }
#endif

    /* Compute source bbox: inverse-map the 4 corners of the draw region */
    int32_t src_min_x = 0x7FFFFFFF, src_min_y = 0x7FFFFFFF;
    int32_t src_max_x = -0x7FFFFFFF, src_max_y = -0x7FFFFFFF;
    int32_t draw_corners[4][2] = {
            {ctx.draw_x0, ctx.draw_y0},
            {ctx.draw_x1, ctx.draw_y0},
            {ctx.draw_x0, ctx.draw_y1},
            {ctx.draw_x1, ctx.draw_y1},
    };
    for (int i = 0; i < 4; i++)
    {
        int32_t rx = ctx.inv_m00 * (draw_corners[i][0] - x) + ctx.inv_m01 * (draw_corners[i][1] - y) + ((int32_t)ctx.cx << 15);
        int32_t ry = ctx.inv_m10 * (draw_corners[i][0] - x) + ctx.inv_m11 * (draw_corners[i][1] - y) + ((int32_t)ctx.cy << 15);
        int32_t sxi = rx >> 15;
        int32_t syi = ry >> 15;
        if (sxi < src_min_x)
            src_min_x = sxi;
        if (syi < src_min_y)
            src_min_y = syi;
        if (sxi > src_max_x)
            src_max_x = sxi;
        if (syi > src_max_y)
            src_max_y = syi;
    }
    /* Expand by 2 pixels for bilinear sampling at edges */
    src_min_x -= 2;
    src_min_y -= 2;
    src_max_x += 2;
    src_max_y += 2;

    /* Collect only glyphs visible in this tile's source bbox.
     * Keep the line buffer in the transient tile cache so it does not occupy persistent .bss. */
    int tile_line_count = 0;
    int16_t tile_src_x0 = 0;
    int16_t tile_src_y0 = 0;
    int16_t tile_src_x1 = 0;
    int16_t tile_src_y1 = 0;
    int tile_glyphs_overlap = 0;
    int tile_count = 0;
    {
        int use_visible_alpha8_tile = (bpp == 4 && scale_q8 >= 256 &&
                                       (ctx.mask == NULL || (ctx.mask->api != NULL && ctx.mask->api->kind == EGUI_MASK_KIND_GRADIENT &&
                                                             ctx.mask->api->mask_blend_row_color != NULL)));

        if (use_visible_alpha8_tile)
        {
#if EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE
            if (tile_alpha8_layout_glyph_indices == NULL)
            {
                tile_alpha8_layout_glyph_indices =
                        (text_transform_layout_index_t *)egui_malloc((int)((size_t)tile_glyph_capacity * sizeof(*tile_alpha8_layout_glyph_indices)));
                if (tile_alpha8_layout_glyph_indices == NULL)
                {
                    goto cleanup;
                }
            }
#endif
            tile_count = collect_visible_layout_glyphs_alpha8(layout_glyphs, layout_lines, layout_line_count, tile_alpha8_layout_glyph_indices,
                                                              tile_glyph_capacity, src_min_x, src_min_y, src_max_x, src_max_y, &tile_src_x0, &tile_src_y0,
                                                              &tile_src_x1, &tile_src_y1, &tile_glyphs_overlap);

            if (tile_count == 0)
            {
                goto cleanup;
            }

            if (text_transform_draw_visible_alpha8_tile_layout(&ctx, x, y, color, font_info, layout_glyphs, tile_alpha8_layout_glyph_indices, tile_count,
                                                               tile_src_x0, tile_src_y0, tile_src_x1, tile_src_y1, tile_glyphs_overlap))
            {
                goto cleanup;
            }
        }

#if EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE
        if (tile_alpha8_layout_glyph_indices != NULL)
        {
            egui_free((void *)tile_alpha8_layout_glyph_indices);
            tile_alpha8_layout_glyph_indices = NULL;
        }

        if (text_transform_prepare_tile_scratch(tile_glyph_capacity, tile_line_capacity, &tile_scratch) != 0)
        {
            goto cleanup;
        }

        tile_glyphs = tile_scratch.glyphs;
        tile_lines = tile_scratch.lines;
#endif
        tile_count = collect_visible_glyphs(font_info, layout_glyphs, layout_lines, layout_line_count, tile_glyphs, tile_glyph_capacity, tile_lines,
                                            tile_line_capacity, &tile_line_count, src_min_x, src_min_y, src_max_x, src_max_y, NULL, NULL, NULL, NULL, NULL, 1);

        if (tile_count == 0)
        {
            goto cleanup;
        }
#if EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE
        text_transform_release_layout_scratch(&layout_scratch);
        layout_glyphs = NULL;
        layout_lines = NULL;
#endif
    }

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (draw_font_is_external)
    {
        int max_external_row_bytes = text_transform_measure_tile_max_row_bytes(tile_glyphs, tile_count);

        if (max_external_row_bytes > 0)
        {
            if (text_transform_external_glyph_row_cache_init(&external_row_cache, max_external_row_bytes) != 0)
            {
                EGUI_ASSERT(0);
                goto cleanup;
            }
            external_row_cache_ready = 1;
        }
    }
#endif

    int hint = -1;
    int hint_line = -1;
    /* Hoist loop-invariant row-start offset computed from draw_x0 */
    int32_t row_start_offset_x = ctx.inv_m00 * ((int32_t)ctx.draw_x0 - x);
    int32_t row_start_offset_y = ctx.inv_m10 * ((int32_t)ctx.draw_x0 - x);

    /* Source bounds in Q15 for scanline skip */
    int32_t text_src_lo_x_q15 = -(1 << 15);
    int32_t text_src_hi_x_q15 = (int32_t)ctx.src_w << 15;
    int32_t text_src_lo_y_q15 = -(1 << 15);
    int32_t text_src_hi_y_q15 = (int32_t)ctx.src_h << 15;

    for (int32_t dy = ctx.draw_y0; dy < ctx.draw_y1; dy++)
    {
        /* Per-row color: apply gradient mask overlay if set */
        egui_color_t row_color = color;
        int row_color_handled = transform_prepare_row_color(ctx.mask, (egui_dim_t)dy, &row_color);
        int mask_requires_point = (ctx.mask != NULL && !row_color_handled);
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
        uint32_t fg_rb_g = (row_color.full | ((uint32_t)row_color.full << 16)) & 0x07E0F81FUL;
#endif

        int32_t rotatedX = row_start_offset_x + ctx.Cx_base;
        int32_t rotatedY = row_start_offset_y + ctx.Cy_base;

        egui_color_int_t *dst_row = &ctx.pfb[(dy - ctx.pfb_oy) * ctx.pfb_w + (ctx.draw_x0 - ctx.pfb_ox)];
        int entered = 0;
        int32_t dx = ctx.draw_x0;

        /* Skip left dead zone where inverse-mapped coords are outside source */
        if (rotatedX < text_src_lo_x_q15 || rotatedX >= text_src_hi_x_q15 || rotatedY < text_src_lo_y_q15 || rotatedY >= text_src_hi_y_q15)
        {
            int32_t skip = transform_scanline_skip(rotatedX, rotatedY, ctx.inv_m00, ctx.inv_m10, text_src_lo_x_q15, text_src_hi_x_q15, text_src_lo_y_q15,
                                                   text_src_hi_y_q15, ctx.draw_x1 - ctx.draw_x0);
            if (skip > 0)
            {
                rotatedX += skip * ctx.inv_m00;
                rotatedY += skip * ctx.inv_m10;
                dst_row += skip;
                dx += skip;
            }
        }

        for (; dx < ctx.draw_x1; dx++)
        {
            int32_t sx = rotatedX >> 15;
            int32_t sy = rotatedY >> 15;

            if (sx >= -1 && sx < ctx.src_w && sy >= -1 && sy < ctx.src_h)
            {
                entered = 1;
                uint8_t fx = (rotatedX >> 7) & 0xFF;
                uint8_t fy = (rotatedY >> 7) & 0xFF;

                /* Fast path: SIR within hint glyph interior */
                if (hint >= 0)
                {
                    const text_transform_glyph_t *g = &tile_glyphs[hint];
                    int lx = sx - g->x;
                    int ly = sy - g->y;
                    if (lx >= 0 && lx + 1 < g->box_w && ly >= 0 && ly + 1 < g->box_h)
                    {
                        /* Compute SIR count within glyph interior */
                        int32_t g_int_max_x_q15 = ((int32_t)(g->x + g->box_w - 2)) << 15;
                        int32_t g_int_min_x_q15 = (int32_t)g->x << 15;
                        int32_t g_int_max_y_q15 = ((int32_t)(g->y + g->box_h - 2)) << 15;
                        int32_t g_int_min_y_q15 = (int32_t)g->y << 15;

                        int32_t sir_count = ctx.draw_x1 - dx;
                        if (ctx.inv_m00 > 0)
                        {
                            int32_t n = (g_int_max_x_q15 - rotatedX) / ctx.inv_m00;
                            if (n < sir_count)
                                sir_count = n;
                        }
                        else if (ctx.inv_m00 < 0)
                        {
                            int32_t n = (rotatedX - g_int_min_x_q15) / (-ctx.inv_m00);
                            if (n < sir_count)
                                sir_count = n;
                        }
                        if (ctx.inv_m10 > 0)
                        {
                            int32_t n = (g_int_max_y_q15 - rotatedY) / ctx.inv_m10;
                            if (n < sir_count)
                                sir_count = n;
                        }
                        else if (ctx.inv_m10 < 0)
                        {
                            int32_t n = (rotatedY - g_int_min_y_q15) / (-ctx.inv_m10);
                            if (n < sir_count)
                                sir_count = n;
                        }
                        if (sir_count < 1)
                            sir_count = 1;

                        /* Tight glyph-interior loop: no boundary checks */
                        if (ctx.canvas_alpha == EGUI_ALPHA_100)
                        {
                            const uint8_t *g_data = g->data;
                            int g_x = g->x;
                            int g_y = g->y;
                            if (bpp == 4)
                            {
                                for (int32_t i = 0; i < sir_count; i++)
                                {
                                    uint16_t sa00 = 0, sa01 = 0, sa10 = 0, sa11 = 0;
                                    int glyph_lx = (rotatedX >> 15) - g_x;
                                    int glyph_ly = (rotatedY >> 15) - g_y;

                                    if (draw_font_is_external)
                                    {
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                                        if (!text_transform_batch_extract_external_glyph_alpha(g, glyph_lx, glyph_ly, bpp, &external_row_cache, &sa00, &sa01,
                                                                                               &sa10, &sa11))
                                        {
                                            goto cleanup;
                                        }
#endif
                                    }
                                    else
                                    {
                                        batch_extract_glyph_alpha_4_rb(g_data, g->row_bytes, glyph_lx, glyph_ly, &sa00, &sa01, &sa10, &sa11);
                                    }

                                    if ((sa00 | sa01 | sa10 | sa11) != 0)
                                    {
                                        uint8_t fx_i = (rotatedX >> 7) & 0xFF;
                                        uint8_t fy_i = (rotatedY >> 7) & 0xFF;
                                        uint16_t ah0 = sa00 + (((int32_t)(sa01 - sa00) * fx_i + 128) >> 8);
                                        uint16_t ah1 = sa10 + (((int32_t)(sa11 - sa10) * fx_i + 128) >> 8);
                                        uint8_t pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy_i + 128) >> 8));

                                        if (mask_requires_point)
                                        {
                                            transform_apply_mask_and_blend(ctx.mask, (egui_dim_t)(dx + i), (egui_dim_t)dy, row_color, pixel_alpha,
                                                                           ctx.canvas_alpha, dst_row);
                                        }
                                        else if (pixel_alpha == EGUI_ALPHA_100)
                                        {
                                            *dst_row = row_color.full;
                                        }
                                        else if (pixel_alpha > 0)
                                        {
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
                                            uint16_t bg = *dst_row;
                                            uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                                            uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * ((uint32_t)pixel_alpha >> 3) >> 5)) & 0x07E0F81FUL;
                                            *dst_row = (uint16_t)(result | (result >> 16));
#else
                                            egui_color_t *back = (egui_color_t *)dst_row;
                                            egui_rgb_mix_ptr(back, &row_color, back, pixel_alpha);
#endif
                                        }
                                    }

                                    rotatedX += ctx.inv_m00;
                                    rotatedY += ctx.inv_m10;
                                    dst_row++;
                                }
                            }
                            else
                            {
                                for (int32_t i = 0; i < sir_count; i++)
                                {
                                    uint16_t sa00 = 0, sa01 = 0, sa10 = 0, sa11 = 0;
                                    int glyph_lx = (rotatedX >> 15) - g_x;
                                    int glyph_ly = (rotatedY >> 15) - g_y;

                                    if (draw_font_is_external)
                                    {
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                                        if (!text_transform_batch_extract_external_glyph_alpha(g, glyph_lx, glyph_ly, bpp, &external_row_cache, &sa00, &sa01,
                                                                                               &sa10, &sa11))
                                        {
                                            goto cleanup;
                                        }
#endif
                                    }
                                    else
                                    {
                                        batch_extract_glyph_alpha(g_data, g->box_w, glyph_lx, glyph_ly, bpp, &sa00, &sa01, &sa10, &sa11);
                                    }

                                    if ((sa00 | sa01 | sa10 | sa11) != 0)
                                    {
                                        uint8_t fx_i = (rotatedX >> 7) & 0xFF;
                                        uint8_t fy_i = (rotatedY >> 7) & 0xFF;
                                        uint16_t ah0 = sa00 + (((int32_t)(sa01 - sa00) * fx_i + 128) >> 8);
                                        uint16_t ah1 = sa10 + (((int32_t)(sa11 - sa10) * fx_i + 128) >> 8);
                                        uint8_t pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy_i + 128) >> 8));

                                        if (mask_requires_point)
                                        {
                                            transform_apply_mask_and_blend(ctx.mask, (egui_dim_t)(dx + i), (egui_dim_t)dy, row_color, pixel_alpha,
                                                                           ctx.canvas_alpha, dst_row);
                                        }
                                        else if (pixel_alpha == EGUI_ALPHA_100)
                                        {
                                            *dst_row = row_color.full;
                                        }
                                        else if (pixel_alpha > 0)
                                        {
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
                                            uint16_t bg = *dst_row;
                                            uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                                            uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * ((uint32_t)pixel_alpha >> 3) >> 5)) & 0x07E0F81FUL;
                                            *dst_row = (uint16_t)(result | (result >> 16));
#else
                                            egui_color_t *back = (egui_color_t *)dst_row;
                                            egui_rgb_mix_ptr(back, &row_color, back, pixel_alpha);
#endif
                                        }
                                    }

                                    rotatedX += ctx.inv_m00;
                                    rotatedY += ctx.inv_m10;
                                    dst_row++;
                                }
                            }
                        }
                        else
                        {
                            if (bpp == 4)
                            {
                                for (int32_t i = 0; i < sir_count; i++)
                                {
                                    uint16_t sa00 = 0, sa01 = 0, sa10 = 0, sa11 = 0;
                                    int glyph_lx = (rotatedX >> 15) - g->x;
                                    int glyph_ly = (rotatedY >> 15) - g->y;

                                    if (draw_font_is_external)
                                    {
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                                        if (!text_transform_batch_extract_external_glyph_alpha(g, glyph_lx, glyph_ly, bpp, &external_row_cache, &sa00, &sa01,
                                                                                               &sa10, &sa11))
                                        {
                                            goto cleanup;
                                        }
#endif
                                    }
                                    else
                                    {
                                        batch_extract_glyph_alpha_4_rb(g->data, g->row_bytes, glyph_lx, glyph_ly, &sa00, &sa01, &sa10, &sa11);
                                    }

                                    if ((sa00 | sa01 | sa10 | sa11) != 0)
                                    {
                                        uint8_t fx_i = (rotatedX >> 7) & 0xFF;
                                        uint8_t fy_i = (rotatedY >> 7) & 0xFF;
                                        uint16_t ah0 = sa00 + (((int32_t)(sa01 - sa00) * fx_i + 128) >> 8);
                                        uint16_t ah1 = sa10 + (((int32_t)(sa11 - sa10) * fx_i + 128) >> 8);
                                        uint8_t pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy_i + 128) >> 8));

                                        if (pixel_alpha > 0)
                                        {
                                            if (mask_requires_point)
                                            {
                                                transform_apply_mask_and_blend(ctx.mask, (egui_dim_t)(dx + i), (egui_dim_t)dy, row_color, pixel_alpha,
                                                                               ctx.canvas_alpha, dst_row);
                                            }
                                            else
                                            {
                                                egui_alpha_t final_alpha = ((uint16_t)ctx.canvas_alpha * pixel_alpha + 128) >> 8;
                                                if (final_alpha == EGUI_ALPHA_100)
                                                {
                                                    *dst_row = row_color.full;
                                                }
                                                else if (final_alpha > 0)
                                                {
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
                                                    uint16_t bg = *dst_row;
                                                    uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                                                    uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * ((uint32_t)final_alpha >> 3) >> 5)) & 0x07E0F81FUL;
                                                    *dst_row = (uint16_t)(result | (result >> 16));
#else
                                                    egui_color_t *back = (egui_color_t *)dst_row;
                                                    egui_rgb_mix_ptr(back, &row_color, back, final_alpha);
#endif
                                                }
                                            }
                                        }
                                    }

                                    rotatedX += ctx.inv_m00;
                                    rotatedY += ctx.inv_m10;
                                    dst_row++;
                                }
                            }
                            else
                            {
                                for (int32_t i = 0; i < sir_count; i++)
                                {
                                    uint16_t sa00 = 0, sa01 = 0, sa10 = 0, sa11 = 0;
                                    int glyph_lx = (rotatedX >> 15) - g->x;
                                    int glyph_ly = (rotatedY >> 15) - g->y;

                                    if (draw_font_is_external)
                                    {
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                                        if (!text_transform_batch_extract_external_glyph_alpha(g, glyph_lx, glyph_ly, bpp, &external_row_cache, &sa00, &sa01,
                                                                                               &sa10, &sa11))
                                        {
                                            goto cleanup;
                                        }
#endif
                                    }
                                    else
                                    {
                                        batch_extract_glyph_alpha(g->data, g->box_w, glyph_lx, glyph_ly, bpp, &sa00, &sa01, &sa10, &sa11);
                                    }

                                    if ((sa00 | sa01 | sa10 | sa11) != 0)
                                    {
                                        uint8_t fx_i = (rotatedX >> 7) & 0xFF;
                                        uint8_t fy_i = (rotatedY >> 7) & 0xFF;
                                        uint16_t ah0 = sa00 + (((int32_t)(sa01 - sa00) * fx_i + 128) >> 8);
                                        uint16_t ah1 = sa10 + (((int32_t)(sa11 - sa10) * fx_i + 128) >> 8);
                                        uint8_t pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy_i + 128) >> 8));

                                        if (pixel_alpha > 0)
                                        {
                                            if (mask_requires_point)
                                            {
                                                transform_apply_mask_and_blend(ctx.mask, (egui_dim_t)(dx + i), (egui_dim_t)dy, row_color, pixel_alpha,
                                                                               ctx.canvas_alpha, dst_row);
                                            }
                                            else
                                            {
                                                egui_alpha_t final_alpha = ((uint16_t)ctx.canvas_alpha * pixel_alpha + 128) >> 8;
                                                if (final_alpha == EGUI_ALPHA_100)
                                                {
                                                    *dst_row = row_color.full;
                                                }
                                                else if (final_alpha > 0)
                                                {
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
                                                    uint16_t bg = *dst_row;
                                                    uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                                                    uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * ((uint32_t)final_alpha >> 3) >> 5)) & 0x07E0F81FUL;
                                                    *dst_row = (uint16_t)(result | (result >> 16));
#else
                                                    egui_color_t *back = (egui_color_t *)dst_row;
                                                    egui_rgb_mix_ptr(back, &row_color, back, final_alpha);
#endif
                                                }
                                            }
                                        }
                                    }

                                    rotatedX += ctx.inv_m00;
                                    rotatedY += ctx.inv_m10;
                                    dst_row++;
                                }
                            }
                        }

                        dx += sir_count - 1;
                        continue;
                    }
                }
                /* Ungrouped slow path: per-sample glyph lookup */
                {
                    uint16_t a00, a01, a10, a11;
                    int line0 = sample_tile_find_pair_line(tile_lines, tile_line_count, sx, sy, hint_line);
                    int line1;
                    int hint00 = -1;
                    int hint01 = -1;
                    int hint10 = -1;
                    int hint11 = -1;

                    a00 = 0;
                    a01 = 0;
                    a10 = 0;
                    a11 = 0;

                    if (line0 >= 0)
                    {
                        if (draw_font_is_external)
                        {
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                            sample_tile_alpha_pair_from_line_external(tile_glyphs, &tile_lines[line0], sx, sy, bpp, &a00, &a01, &hint00, &hint01,
                                                                      &external_row_cache);
#endif
                        }
                        else
                        {
                            sample_tile_alpha_pair_from_line(tile_glyphs, &tile_lines[line0], sx, sy, bpp, &a00, &a01, &hint00, &hint01);
                        }
                    }

                    /* Early exit: preserve existing whitespace skip behavior based on a00 only */
                    if (a00 == 0 && hint00 < 0)
                    {
                        hint = -1;
                        hint_line = line0;
                        goto text_next_pixel;
                    }

                    line1 = sample_tile_find_pair_line(tile_lines, tile_line_count, sx, sy + 1, line0);
                    if (line1 >= 0)
                    {
                        if (draw_font_is_external)
                        {
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                            sample_tile_alpha_pair_from_line_external(tile_glyphs, &tile_lines[line1], sx, sy + 1, bpp, &a10, &a11, &hint10, &hint11,
                                                                      &external_row_cache);
#endif
                        }
                        else
                        {
                            sample_tile_alpha_pair_from_line(tile_glyphs, &tile_lines[line1], sx, sy + 1, bpp, &a10, &a11, &hint10, &hint11);
                        }
                    }

                    if (hint11 >= 0)
                    {
                        hint = hint11;
                        hint_line = line1;
                    }
                    else if (hint10 >= 0)
                    {
                        hint = hint10;
                        hint_line = line1;
                    }
                    else if (hint01 >= 0)
                    {
                        hint = hint01;
                        hint_line = line0;
                    }
                    else if (hint00 >= 0)
                    {
                        hint = hint00;
                        hint_line = line0;
                    }
                    else
                    {
                        hint = -1;
                        hint_line = (line1 >= 0) ? line1 : line0;
                    }

                    uint16_t ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
                    uint16_t ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
                    uint8_t pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));

                    if (pixel_alpha > 0)
                    {
                        if (mask_requires_point)
                        {
                            transform_apply_mask_and_blend(ctx.mask, (egui_dim_t)dx, (egui_dim_t)dy, row_color, pixel_alpha, ctx.canvas_alpha, dst_row);
                        }
                        else if (ctx.canvas_alpha == EGUI_ALPHA_100)
                        {
                            if (pixel_alpha == EGUI_ALPHA_100)
                            {
                                *dst_row = row_color.full;
                            }
                            else
                            {
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
                                uint16_t bg = *dst_row;
                                uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                                uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * ((uint32_t)pixel_alpha >> 3) >> 5)) & 0x07E0F81FUL;
                                *dst_row = (uint16_t)(result | (result >> 16));
#else
                                egui_color_t *back = (egui_color_t *)dst_row;
                                egui_rgb_mix_ptr(back, &row_color, back, pixel_alpha);
#endif
                            }
                        }
                        else
                        {
                            egui_alpha_t final_alpha = ((uint16_t)ctx.canvas_alpha * pixel_alpha + 128) >> 8;
                            if (final_alpha == EGUI_ALPHA_100)
                            {
                                *dst_row = row_color.full;
                            }
                            else if (final_alpha > 0)
                            {
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
                                uint16_t bg = *dst_row;
                                uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                                uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * ((uint32_t)final_alpha >> 3) >> 5)) & 0x07E0F81FUL;
                                *dst_row = (uint16_t)(result | (result >> 16));
#else
                                egui_color_t *back = (egui_color_t *)dst_row;
                                egui_rgb_mix_ptr(back, &row_color, back, final_alpha);
#endif
                            }
                        }
                    }
                }
            }
            else if (entered)
            {
                break; /* Early exit: left source region (SCGUI-style scanline break) */
            }

        text_next_pixel:
            rotatedX += ctx.inv_m00;
            rotatedY += ctx.inv_m10;
            dst_row++;
        }

        ctx.Cx_base += ctx.inv_m01;
        ctx.Cy_base += ctx.inv_m11;
    }

cleanup:
    ;
#if EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE
    if (tile_alpha8_layout_glyph_indices != NULL)
    {
        egui_free((void *)tile_alpha8_layout_glyph_indices);
    }
    text_transform_release_tile_scratch(&tile_scratch);
    text_transform_release_layout_scratch(&layout_scratch);
#endif
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (external_row_cache_ready)
    {
        text_transform_external_glyph_row_cache_release(&external_row_cache);
    }
#endif
}

// ============================================================================
// Text transform alpha8 tile helpers
// ============================================================================

#define EGUI_ALPHA4_EXPAND_PAIR(_hi, _lo) (uint16_t)((((uint16_t)(_hi) * 0x11u) << 8) | ((uint16_t)(_lo) * 0x11u))
#define EGUI_ALPHA4_EXPAND_PAIR_ROW(_hi)                                                                                                                       \
    EGUI_ALPHA4_EXPAND_PAIR(_hi, 0x0), EGUI_ALPHA4_EXPAND_PAIR(_hi, 0x1), EGUI_ALPHA4_EXPAND_PAIR(_hi, 0x2), EGUI_ALPHA4_EXPAND_PAIR(_hi, 0x3),                \
            EGUI_ALPHA4_EXPAND_PAIR(_hi, 0x4), EGUI_ALPHA4_EXPAND_PAIR(_hi, 0x5), EGUI_ALPHA4_EXPAND_PAIR(_hi, 0x6), EGUI_ALPHA4_EXPAND_PAIR(_hi, 0x7),        \
            EGUI_ALPHA4_EXPAND_PAIR(_hi, 0x8), EGUI_ALPHA4_EXPAND_PAIR(_hi, 0x9), EGUI_ALPHA4_EXPAND_PAIR(_hi, 0xA), EGUI_ALPHA4_EXPAND_PAIR(_hi, 0xB),        \
            EGUI_ALPHA4_EXPAND_PAIR(_hi, 0xC), EGUI_ALPHA4_EXPAND_PAIR(_hi, 0xD), EGUI_ALPHA4_EXPAND_PAIR(_hi, 0xE), EGUI_ALPHA4_EXPAND_PAIR(_hi, 0xF)

static const uint16_t g_alpha4_expand_pair_table[256] = {
        EGUI_ALPHA4_EXPAND_PAIR_ROW(0x0), EGUI_ALPHA4_EXPAND_PAIR_ROW(0x1), EGUI_ALPHA4_EXPAND_PAIR_ROW(0x2), EGUI_ALPHA4_EXPAND_PAIR_ROW(0x3),
        EGUI_ALPHA4_EXPAND_PAIR_ROW(0x4), EGUI_ALPHA4_EXPAND_PAIR_ROW(0x5), EGUI_ALPHA4_EXPAND_PAIR_ROW(0x6), EGUI_ALPHA4_EXPAND_PAIR_ROW(0x7),
        EGUI_ALPHA4_EXPAND_PAIR_ROW(0x8), EGUI_ALPHA4_EXPAND_PAIR_ROW(0x9), EGUI_ALPHA4_EXPAND_PAIR_ROW(0xA), EGUI_ALPHA4_EXPAND_PAIR_ROW(0xB),
        EGUI_ALPHA4_EXPAND_PAIR_ROW(0xC), EGUI_ALPHA4_EXPAND_PAIR_ROW(0xD), EGUI_ALPHA4_EXPAND_PAIR_ROW(0xE), EGUI_ALPHA4_EXPAND_PAIR_ROW(0xF),
};
#undef EGUI_ALPHA4_EXPAND_PAIR_ROW
#undef EGUI_ALPHA4_EXPAND_PAIR

static void rasterize_glyph4_to_alpha8_inside(uint8_t *buf, int buf_w, int dst_x, int dst_y, const uint8_t *glyph_data, int box_w, int box_h);
static void rasterize_glyph4_to_alpha8_inside_overwrite(uint8_t *buf, int buf_w, int dst_x, int dst_y, const uint8_t *glyph_data, int box_w, int box_h);

static void rasterize_glyph4_to_alpha8_inside(uint8_t *buf, int buf_w, int dst_x, int dst_y, const uint8_t *glyph_data, int box_w, int box_h)
{
    int row_bytes = (box_w + 1) >> 1;
    int pair_count = box_w >> 1;
    int has_tail = box_w & 1;

    for (int row = 0; row < box_h; row++)
    {
        const uint8_t *src = glyph_data + row * row_bytes;
        uint8_t *dst = buf + (dst_y + row) * buf_w + dst_x;

        for (int pair_idx = 0; pair_idx < pair_count; pair_idx++)
        {
            uint8_t packed = *src++;

            if (packed == 0)
            {
                dst += 2;
                continue;
            }
            if (packed == 0xFF)
            {
                dst[0] = EGUI_ALPHA_100;
                dst[1] = EGUI_ALPHA_100;
                dst += 2;
                continue;
            }

            {
                uint16_t pair = g_alpha4_expand_pair_table[packed];
                uint8_t alpha0 = (uint8_t)(pair & 0xFF);
                uint8_t alpha1 = (uint8_t)(pair >> 8);

                if (alpha0 > dst[0])
                {
                    dst[0] = alpha0;
                }
                if (alpha1 > dst[1])
                {
                    dst[1] = alpha1;
                }
            }

            dst += 2;
        }

        if (has_tail)
        {
            uint8_t packed = *src;
            uint8_t alpha0;

            if ((packed & 0x0F) == 0)
            {
                continue;
            }
            if ((packed & 0x0F) == 0x0F)
            {
                dst[0] = EGUI_ALPHA_100;
                continue;
            }

            alpha0 = (uint8_t)(g_alpha4_expand_pair_table[packed] & 0xFF);
            if (alpha0 > dst[0])
            {
                dst[0] = alpha0;
            }
        }
    }
}

static void rasterize_glyph4_to_alpha8_inside_overwrite(uint8_t *buf, int buf_w, int dst_x, int dst_y, const uint8_t *glyph_data, int box_w, int box_h)
{
    int row_bytes = (box_w + 1) >> 1;
    int pair_count = box_w >> 1;
    int has_tail = box_w & 1;

    for (int row = 0; row < box_h; row++)
    {
        const uint8_t *src = glyph_data + row * row_bytes;
        uint8_t *dst = buf + (dst_y + row) * buf_w + dst_x;

        for (int pair_idx = 0; pair_idx < pair_count; pair_idx++)
        {
            uint8_t packed = *src++;

            if (packed == 0)
            {
                dst += 2;
                continue;
            }
            if (packed == 0xFF)
            {
                dst[0] = EGUI_ALPHA_100;
                dst[1] = EGUI_ALPHA_100;
                dst += 2;
                continue;
            }

            {
                uint16_t pair = g_alpha4_expand_pair_table[packed];
                dst[0] = (uint8_t)(pair & 0xFF);
                dst[1] = (uint8_t)(pair >> 8);
            }

            dst += 2;
        }

        if (has_tail)
        {
            uint8_t packed = *src;

            if ((packed & 0x0F) == 0)
            {
                continue;
            }
            if ((packed & 0x0F) == 0x0F)
            {
                dst[0] = EGUI_ALPHA_100;
                continue;
            }

            dst[0] = (uint8_t)(g_alpha4_expand_pair_table[packed] & 0xFF);
        }
    }
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static int rasterize_external_layout_glyph4_to_alpha8_inside_common(uint8_t *buf, int buf_w, int dst_x, int dst_y, const egui_font_std_info_t *font_info,
                                                                    const text_transform_layout_glyph_t *glyph, int overwrite,
                                                                    text_transform_external_layout_glyph_row_scratch_t *scratch)
{
    int chunk_rows;
    int row_bytes;
    int pair_count;
    int has_tail;

    if (glyph == NULL || glyph->box_w <= 0 || glyph->box_h <= 0)
    {
        return 1;
    }

    row_bytes = (glyph->box_w + 1) >> 1;
    pair_count = glyph->box_w >> 1;
    has_tail = glyph->box_w & 1;
    if (buf == NULL || font_info == NULL || scratch == NULL || scratch->row_buf == NULL || row_bytes <= 0 || row_bytes > scratch->row_capacity)
    {
        EGUI_ASSERT(0);
        return 0;
    }

    chunk_rows = scratch->chunk_rows;
    if (chunk_rows <= 0)
    {
        EGUI_ASSERT(0);
        return 0;
    }

    for (int row = 0; row < glyph->box_h;)
    {
        int rows_this_chunk = glyph->box_h - row;

        if (rows_this_chunk > chunk_rows)
        {
            rows_this_chunk = chunk_rows;
        }
        if (!text_transform_load_external_layout_glyph_rows(font_info, glyph, row, rows_this_chunk, scratch))
        {
            return 0;
        }

        for (int chunk_row = 0; chunk_row < rows_this_chunk; chunk_row++)
        {
            const uint8_t *src = scratch->row_buf + chunk_row * row_bytes;
            uint8_t *dst = buf + (dst_y + row + chunk_row) * buf_w + dst_x;

            for (int pair_idx = 0; pair_idx < pair_count; pair_idx++)
            {
                uint8_t packed = *src++;

                if (packed == 0)
                {
                    dst += 2;
                    continue;
                }
                if (packed == 0xFF)
                {
                    dst[0] = EGUI_ALPHA_100;
                    dst[1] = EGUI_ALPHA_100;
                    dst += 2;
                    continue;
                }

                {
                    uint16_t pair = g_alpha4_expand_pair_table[packed];
                    uint8_t alpha0 = (uint8_t)(pair & 0xFF);
                    uint8_t alpha1 = (uint8_t)(pair >> 8);

                    if (overwrite)
                    {
                        dst[0] = alpha0;
                        dst[1] = alpha1;
                    }
                    else
                    {
                        if (alpha0 > dst[0])
                        {
                            dst[0] = alpha0;
                        }
                        if (alpha1 > dst[1])
                        {
                            dst[1] = alpha1;
                        }
                    }
                }

                dst += 2;
            }

            if (has_tail)
            {
                uint8_t packed = *src;
                uint8_t alpha0;

                if ((packed & 0x0F) == 0)
                {
                    continue;
                }
                if ((packed & 0x0F) == 0x0F)
                {
                    dst[0] = EGUI_ALPHA_100;
                    continue;
                }

                alpha0 = (uint8_t)(g_alpha4_expand_pair_table[packed] & 0xFF);
                if (overwrite || alpha0 > dst[0])
                {
                    dst[0] = alpha0;
                }
            }
        }

        row += rows_this_chunk;
    }

    return 1;
}

static int rasterize_external_layout_glyph4_to_alpha8_inside(uint8_t *buf, int buf_w, int dst_x, int dst_y, const egui_font_std_info_t *font_info,
                                                             const text_transform_layout_glyph_t *glyph,
                                                             text_transform_external_layout_glyph_row_scratch_t *scratch)
{
    return rasterize_external_layout_glyph4_to_alpha8_inside_common(buf, buf_w, dst_x, dst_y, font_info, glyph, 0, scratch);
}

static int rasterize_external_layout_glyph4_to_alpha8_inside_overwrite(uint8_t *buf, int buf_w, int dst_x, int dst_y, const egui_font_std_info_t *font_info,
                                                                       const text_transform_layout_glyph_t *glyph,
                                                                       text_transform_external_layout_glyph_row_scratch_t *scratch)
{
    return rasterize_external_layout_glyph4_to_alpha8_inside_common(buf, buf_w, dst_x, dst_y, font_info, glyph, 1, scratch);
}
#endif

static int text_transform_rasterize_visible_alpha8_layout(uint8_t *alpha8_buf, int buf_w, int buf_h, int16_t src_x0, int16_t src_y0,
                                                          const egui_font_std_info_t *font_info, const text_transform_layout_glyph_t *layout_glyphs,
                                                          const text_transform_layout_index_t *glyph_indices, int glyph_count, int glyphs_overlap)
{
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    text_transform_external_layout_glyph_row_scratch_t external_row_scratch = {
            0,
    };
#endif
    int status = 0;

    if (alpha8_buf == NULL || font_info == NULL || layout_glyphs == NULL || glyph_indices == NULL || glyph_count <= 0 || buf_w <= 0 || buf_h <= 0)
    {
        return -1;
    }

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (font_info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        int max_row_bytes = text_transform_measure_visible_max_row_bytes(layout_glyphs, glyph_indices, glyph_count);

        if (max_row_bytes > 0 && text_transform_external_layout_glyph_row_scratch_init(&external_row_scratch, max_row_bytes) != 0)
        {
            EGUI_ASSERT(0);
            return -1;
        }
    }
#endif

    egui_api_memset(alpha8_buf, 0, buf_w * buf_h);

    if (glyphs_overlap)
    {
        for (int i = 0; i < glyph_count; i++)
        {
            const text_transform_layout_glyph_t *glyph = &layout_glyphs[glyph_indices[i]];

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
            if (font_info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
            {
                if (!rasterize_external_layout_glyph4_to_alpha8_inside(alpha8_buf, buf_w, glyph->x - src_x0, glyph->y - src_y0, font_info, glyph,
                                                                       &external_row_scratch))
                {
                    status = -1;
                    goto cleanup;
                }
            }
            else
#endif
            {
                rasterize_glyph4_to_alpha8_inside(alpha8_buf, buf_w, glyph->x - src_x0, glyph->y - src_y0, font_info->pixel_buffer + glyph->pixel_idx,
                                                  glyph->box_w, glyph->box_h);
            }
        }
    }
    else
    {
        for (int i = 0; i < glyph_count; i++)
        {
            const text_transform_layout_glyph_t *glyph = &layout_glyphs[glyph_indices[i]];

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
            if (font_info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
            {
                if (!rasterize_external_layout_glyph4_to_alpha8_inside_overwrite(alpha8_buf, buf_w, glyph->x - src_x0, glyph->y - src_y0, font_info, glyph,
                                                                                 &external_row_scratch))
                {
                    status = -1;
                    goto cleanup;
                }
            }
            else
#endif
            {
                rasterize_glyph4_to_alpha8_inside_overwrite(alpha8_buf, buf_w, glyph->x - src_x0, glyph->y - src_y0, font_info->pixel_buffer + glyph->pixel_idx,
                                                            glyph->box_w, glyph->box_h);
            }
        }
    }

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
cleanup:
    ;
    text_transform_external_layout_glyph_row_scratch_release(&external_row_scratch);
#endif
    return status;
}

static int text_transform_ensure_visible_tile_capacity(int needed, uint8_t **buf, int *capacity)
{
    int alloc_size;
    uint8_t *new_buf;

    if (*buf != NULL && needed <= *capacity)
    {
        return 0;
    }

    alloc_size = needed;
#if EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES > 0
    if (alloc_size < EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES)
    {
        /*
         * Text rotation often visits multiple tiles in ascending visible sizes
         * within the same frame. Reserve the alpha8 fast-path ceiling upfront
         * so we do not stack old+new transient buffers during repeated growth.
         */
        alloc_size = EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES;
    }
#endif

    new_buf = (uint8_t *)egui_malloc(alloc_size);
    if (new_buf == NULL)
    {
        return -1;
    }

    if (*buf != NULL)
    {
        egui_free(*buf);
    }

    *buf = new_buf;
    *capacity = alloc_size;
    return 0;
}

static int text_transform_draw_alpha8_buffer_opaque_nomask(const text_transform_ctx_t *ctx, egui_dim_t x, egui_dim_t y, egui_color_t color,
                                                           const uint8_t *alpha8_buf, int16_t src_x0, int16_t src_y0, int buf_w, int buf_h,
                                                           int32_t core_src_lo_y_q15, int32_t core_src_hi_y_q15)
{
    int32_t inv_m00;
    int32_t inv_m01;
    int32_t inv_m10;
    int32_t inv_m11;
    int32_t draw_x0;
    int32_t draw_x1;
    int32_t draw_y0;
    int32_t draw_y1;
    int32_t pfb_w;
    int32_t pfb_ox;
    int32_t pfb_oy;
    int32_t row_start_offset_x;
    int32_t row_start_offset_y;
    int32_t Cx_base;
    int32_t Cy_base;
    int32_t buf_src_hi_x_q15;
    int32_t buf_src_hi_y_q15;
    int32_t buf_int_max_x_q15;
    int32_t buf_int_max_y_q15;
    egui_color_int_t solid_color = color.full;
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
    uint32_t fg_rb_g;
#endif

    inv_m00 = ctx->inv_m00;
    inv_m01 = ctx->inv_m01;
    inv_m10 = ctx->inv_m10;
    inv_m11 = ctx->inv_m11;
    draw_x0 = ctx->draw_x0;
    draw_x1 = ctx->draw_x1;
    draw_y0 = ctx->draw_y0;
    draw_y1 = ctx->draw_y1;
    pfb_w = ctx->pfb_w;
    pfb_ox = ctx->pfb_ox;
    pfb_oy = ctx->pfb_oy;
    row_start_offset_x = inv_m00 * (draw_x0 - x);
    row_start_offset_y = inv_m10 * (draw_x0 - x);
    Cx_base = ctx->Cx_base - ((int32_t)src_x0 << 15);
    Cy_base = ctx->Cy_base - ((int32_t)src_y0 << 15);
    buf_src_hi_x_q15 = (int32_t)buf_w << 15;
    buf_src_hi_y_q15 = (int32_t)buf_h << 15;
    buf_int_max_x_q15 = ((int32_t)buf_w - 2) << 15;
    buf_int_max_y_q15 = ((int32_t)buf_h - 2) << 15;
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
    fg_rb_g = (solid_color | ((uint32_t)solid_color << 16)) & 0x07E0F81FUL;
#endif

    for (int32_t dy = draw_y0; dy < draw_y1; dy++)
    {
        int32_t rotatedX = row_start_offset_x + Cx_base;
        int32_t rotatedY = row_start_offset_y + Cy_base;
        egui_color_int_t *dst_row = &ctx->pfb[(dy - pfb_oy) * pfb_w + (draw_x0 - pfb_ox)];
        int entered = 0;
        int32_t dx = draw_x0;

        if (rotatedX < 0 || rotatedX >= buf_src_hi_x_q15 || rotatedY < 0 || rotatedY >= buf_src_hi_y_q15)
        {
            int32_t skip = transform_scanline_skip(rotatedX, rotatedY, inv_m00, inv_m10, 0, buf_src_hi_x_q15, 0, buf_src_hi_y_q15, draw_x1 - draw_x0);
            if (skip > 0)
            {
                rotatedX += skip * inv_m00;
                rotatedY += skip * inv_m10;
                dst_row += skip;
                dx += skip;
            }
        }

        for (; dx < draw_x1; dx++)
        {
            int32_t sx = rotatedX >> 15;
            int32_t sy = rotatedY >> 15;

            if (sx >= 0 && sx < buf_w - 1 && sy >= 0 && sy < buf_h - 1)
            {
                entered = 1;
                int32_t sir_count = draw_x1 - dx;

                if (inv_m00 > 0)
                {
                    int32_t n = (buf_int_max_x_q15 - rotatedX) / inv_m00;
                    if (n < sir_count)
                    {
                        sir_count = n;
                    }
                }
                else if (inv_m00 < 0)
                {
                    int32_t n = rotatedX / (-inv_m00);
                    if (n < sir_count)
                    {
                        sir_count = n;
                    }
                }

                if (inv_m10 > 0)
                {
                    int32_t n = (buf_int_max_y_q15 - rotatedY) / inv_m10;
                    if (n < sir_count)
                    {
                        sir_count = n;
                    }
                }
                else if (inv_m10 < 0)
                {
                    int32_t n = rotatedY / (-inv_m10);
                    if (n < sir_count)
                    {
                        sir_count = n;
                    }
                }

                if (sir_count < 1)
                {
                    sir_count = 1;
                }

                for (int32_t i = 0; i < sir_count; i++)
                {
                    if (rotatedY < core_src_lo_y_q15 || rotatedY >= core_src_hi_y_q15)
                    {
                        rotatedX += inv_m00;
                        rotatedY += inv_m10;
                        dst_row++;
                        continue;
                    }

                    int32_t sample_sx = rotatedX >> 15;
                    int32_t sample_sy = rotatedY >> 15;
                    const uint8_t *row0 = alpha8_buf + sample_sy * buf_w + sample_sx;
                    const uint8_t *row1 = row0 + buf_w;
                    uint8_t a00 = row0[0];
                    uint8_t a01 = row0[1];
                    uint8_t a10 = row1[0];
                    uint8_t a11 = row1[1];
                    uint8_t pixel_alpha;

                    if ((a00 | a01 | a10 | a11) == 0)
                    {
                        rotatedX += inv_m00;
                        rotatedY += inv_m10;
                        dst_row++;
                        continue;
                    }

                    if ((a00 & a01 & a10 & a11) == EGUI_ALPHA_100)
                    {
                        pixel_alpha = EGUI_ALPHA_100;
                    }
                    else
                    {
                        uint8_t fx = (rotatedX >> 7) & 0xFF;
                        uint8_t fy = (rotatedY >> 7) & 0xFF;
                        uint16_t ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
                        uint16_t ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
                        pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));
                    }

                    if (pixel_alpha > 0)
                    {
                        if (pixel_alpha == EGUI_ALPHA_100)
                        {
                            *dst_row = solid_color;
                        }
                        else
                        {
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
                            uint16_t bg = *dst_row;
                            uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                            uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * ((uint32_t)pixel_alpha >> 3) >> 5)) & 0x07E0F81FUL;
                            *dst_row = (uint16_t)(result | (result >> 16));
#else
                            egui_color_t *back = (egui_color_t *)dst_row;
                            egui_rgb_mix_ptr(back, &color, back, pixel_alpha);
#endif
                        }
                    }

                    rotatedX += inv_m00;
                    rotatedY += inv_m10;
                    dst_row++;
                }

                dx += sir_count - 1;
                continue;
            }
            else if (sx >= -1 && sx < buf_w && sy >= -1 && sy < buf_h)
            {
                uint8_t fx = (rotatedX >> 7) & 0xFF;
                uint8_t fy = (rotatedY >> 7) & 0xFF;
                uint8_t a00 = 0;
                uint8_t a01 = 0;
                uint8_t a10 = 0;
                uint8_t a11 = 0;
                uint8_t pixel_alpha;

                entered = 1;

                if (rotatedY < core_src_lo_y_q15 || rotatedY >= core_src_hi_y_q15)
                {
                    rotatedX += inv_m00;
                    rotatedY += inv_m10;
                    dst_row++;
                    continue;
                }

                if ((uint32_t)sy < (uint32_t)buf_h)
                {
                    const uint8_t *row0 = alpha8_buf + sy * buf_w;

                    if ((uint32_t)sx < (uint32_t)buf_w)
                    {
                        a00 = row0[sx];
                    }
                    if ((uint32_t)(sx + 1) < (uint32_t)buf_w)
                    {
                        a01 = row0[sx + 1];
                    }
                }

                if ((uint32_t)(sy + 1) < (uint32_t)buf_h)
                {
                    const uint8_t *row1 = alpha8_buf + (sy + 1) * buf_w;

                    if ((uint32_t)sx < (uint32_t)buf_w)
                    {
                        a10 = row1[sx];
                    }
                    if ((uint32_t)(sx + 1) < (uint32_t)buf_w)
                    {
                        a11 = row1[sx + 1];
                    }
                }

                if ((a00 | a01 | a10 | a11) == 0)
                {
                    pixel_alpha = 0;
                }
                else if ((a00 & a01 & a10 & a11) == EGUI_ALPHA_100)
                {
                    pixel_alpha = EGUI_ALPHA_100;
                }
                else
                {
                    uint16_t ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
                    uint16_t ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
                    pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));
                }

                if (pixel_alpha > 0)
                {
                    if (pixel_alpha == EGUI_ALPHA_100)
                    {
                        *dst_row = solid_color;
                    }
                    else
                    {
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
                        uint16_t bg = *dst_row;
                        uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                        uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * ((uint32_t)pixel_alpha >> 3) >> 5)) & 0x07E0F81FUL;
                        *dst_row = (uint16_t)(result | (result >> 16));
#else
                        egui_color_t *back = (egui_color_t *)dst_row;
                        egui_rgb_mix_ptr(back, &color, back, pixel_alpha);
#endif
                    }
                }
            }
            else if (entered)
            {
                break;
            }

            rotatedX += inv_m00;
            rotatedY += inv_m10;
            dst_row++;
        }

        Cx_base += inv_m01;
        Cy_base += inv_m11;
    }

    return 1;
}

static int text_transform_draw_alpha8_buffer_opaque_row_color(const text_transform_ctx_t *ctx, egui_dim_t x, egui_dim_t y, egui_color_t color,
                                                              const uint8_t *alpha8_buf, int16_t src_x0, int16_t src_y0, int buf_w, int buf_h,
                                                              int32_t core_src_lo_y_q15, int32_t core_src_hi_y_q15)
{
    int32_t inv_m00;
    int32_t inv_m01;
    int32_t inv_m10;
    int32_t inv_m11;
    int32_t draw_x0;
    int32_t draw_x1;
    int32_t draw_y0;
    int32_t draw_y1;
    int32_t pfb_w;
    int32_t pfb_ox;
    int32_t pfb_oy;
    int32_t row_start_offset_x;
    int32_t row_start_offset_y;
    int32_t Cx_base;
    int32_t Cy_base;
    int32_t buf_src_hi_x_q15;
    int32_t buf_src_hi_y_q15;
    int32_t buf_int_max_x_q15;
    int32_t buf_int_max_y_q15;

    inv_m00 = ctx->inv_m00;
    inv_m01 = ctx->inv_m01;
    inv_m10 = ctx->inv_m10;
    inv_m11 = ctx->inv_m11;
    draw_x0 = ctx->draw_x0;
    draw_x1 = ctx->draw_x1;
    draw_y0 = ctx->draw_y0;
    draw_y1 = ctx->draw_y1;
    pfb_w = ctx->pfb_w;
    pfb_ox = ctx->pfb_ox;
    pfb_oy = ctx->pfb_oy;
    row_start_offset_x = inv_m00 * (draw_x0 - x);
    row_start_offset_y = inv_m10 * (draw_x0 - x);
    Cx_base = ctx->Cx_base - ((int32_t)src_x0 << 15);
    Cy_base = ctx->Cy_base - ((int32_t)src_y0 << 15);
    buf_src_hi_x_q15 = (int32_t)buf_w << 15;
    buf_src_hi_y_q15 = (int32_t)buf_h << 15;
    buf_int_max_x_q15 = ((int32_t)buf_w - 2) << 15;
    buf_int_max_y_q15 = ((int32_t)buf_h - 2) << 15;

    for (int32_t dy = draw_y0; dy < draw_y1; dy++)
    {
        egui_color_t row_color = color;
        egui_color_int_t solid_color;
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
        uint32_t row_fg_rb_g;
#endif
        int32_t rotatedX;
        int32_t rotatedY;
        egui_color_int_t *dst_row;
        int entered = 0;
        int32_t dx = draw_x0;

        if (!transform_prepare_row_color(ctx->mask, (egui_dim_t)dy, &row_color))
        {
            return 0;
        }

        solid_color = row_color.full;
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
        row_fg_rb_g = (solid_color | ((uint32_t)solid_color << 16)) & 0x07E0F81FUL;
#endif
        rotatedX = row_start_offset_x + Cx_base;
        rotatedY = row_start_offset_y + Cy_base;
        dst_row = &ctx->pfb[(dy - pfb_oy) * pfb_w + (draw_x0 - pfb_ox)];

        if (rotatedX < 0 || rotatedX >= buf_src_hi_x_q15 || rotatedY < 0 || rotatedY >= buf_src_hi_y_q15)
        {
            int32_t skip = transform_scanline_skip(rotatedX, rotatedY, inv_m00, inv_m10, 0, buf_src_hi_x_q15, 0, buf_src_hi_y_q15, draw_x1 - draw_x0);
            if (skip > 0)
            {
                rotatedX += skip * inv_m00;
                rotatedY += skip * inv_m10;
                dst_row += skip;
                dx += skip;
            }
        }

        for (; dx < draw_x1; dx++)
        {
            int32_t sx = rotatedX >> 15;
            int32_t sy = rotatedY >> 15;

            if (sx >= 0 && sx < buf_w - 1 && sy >= 0 && sy < buf_h - 1)
            {
                entered = 1;
                int32_t sir_count = draw_x1 - dx;

                if (inv_m00 > 0)
                {
                    int32_t n = (buf_int_max_x_q15 - rotatedX) / inv_m00;
                    if (n < sir_count)
                    {
                        sir_count = n;
                    }
                }
                else if (inv_m00 < 0)
                {
                    int32_t n = rotatedX / (-inv_m00);
                    if (n < sir_count)
                    {
                        sir_count = n;
                    }
                }

                if (inv_m10 > 0)
                {
                    int32_t n = (buf_int_max_y_q15 - rotatedY) / inv_m10;
                    if (n < sir_count)
                    {
                        sir_count = n;
                    }
                }
                else if (inv_m10 < 0)
                {
                    int32_t n = rotatedY / (-inv_m10);
                    if (n < sir_count)
                    {
                        sir_count = n;
                    }
                }

                if (sir_count < 1)
                {
                    sir_count = 1;
                }

                for (int32_t i = 0; i < sir_count; i++)
                {
                    if (rotatedY < core_src_lo_y_q15 || rotatedY >= core_src_hi_y_q15)
                    {
                        rotatedX += inv_m00;
                        rotatedY += inv_m10;
                        dst_row++;
                        continue;
                    }

                    int32_t sample_sx = rotatedX >> 15;
                    int32_t sample_sy = rotatedY >> 15;
                    const uint8_t *row0 = alpha8_buf + sample_sy * buf_w + sample_sx;
                    const uint8_t *row1 = row0 + buf_w;
                    uint8_t a00 = row0[0];
                    uint8_t a01 = row0[1];
                    uint8_t a10 = row1[0];
                    uint8_t a11 = row1[1];
                    uint8_t pixel_alpha;

                    if ((a00 | a01 | a10 | a11) == 0)
                    {
                        rotatedX += inv_m00;
                        rotatedY += inv_m10;
                        dst_row++;
                        continue;
                    }

                    if ((a00 & a01 & a10 & a11) == EGUI_ALPHA_100)
                    {
                        pixel_alpha = EGUI_ALPHA_100;
                    }
                    else
                    {
                        uint8_t fx = (rotatedX >> 7) & 0xFF;
                        uint8_t fy = (rotatedY >> 7) & 0xFF;
                        uint16_t ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
                        uint16_t ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
                        pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));
                    }

                    if (pixel_alpha > 0)
                    {
                        if (pixel_alpha == EGUI_ALPHA_100)
                        {
                            *dst_row = solid_color;
                        }
                        else
                        {
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
                            uint16_t bg = *dst_row;
                            uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                            uint32_t result = (bg_rb_g + ((row_fg_rb_g - bg_rb_g) * ((uint32_t)pixel_alpha >> 3) >> 5)) & 0x07E0F81FUL;
                            *dst_row = (uint16_t)(result | (result >> 16));
#else
                            egui_color_t *back = (egui_color_t *)dst_row;
                            egui_rgb_mix_ptr(back, &row_color, back, pixel_alpha);
#endif
                        }
                    }

                    rotatedX += inv_m00;
                    rotatedY += inv_m10;
                    dst_row++;
                }

                dx += sir_count - 1;
                continue;
            }
            else if (sx >= -1 && sx < buf_w && sy >= -1 && sy < buf_h)
            {
                uint8_t fx = (rotatedX >> 7) & 0xFF;
                uint8_t fy = (rotatedY >> 7) & 0xFF;
                uint8_t a00 = 0;
                uint8_t a01 = 0;
                uint8_t a10 = 0;
                uint8_t a11 = 0;
                uint8_t pixel_alpha;

                entered = 1;

                if (rotatedY < core_src_lo_y_q15 || rotatedY >= core_src_hi_y_q15)
                {
                    rotatedX += inv_m00;
                    rotatedY += inv_m10;
                    dst_row++;
                    continue;
                }

                if ((uint32_t)sy < (uint32_t)buf_h)
                {
                    const uint8_t *row0 = alpha8_buf + sy * buf_w;

                    if ((uint32_t)sx < (uint32_t)buf_w)
                    {
                        a00 = row0[sx];
                    }
                    if ((uint32_t)(sx + 1) < (uint32_t)buf_w)
                    {
                        a01 = row0[sx + 1];
                    }
                }

                if ((uint32_t)(sy + 1) < (uint32_t)buf_h)
                {
                    const uint8_t *row1 = alpha8_buf + (sy + 1) * buf_w;

                    if ((uint32_t)sx < (uint32_t)buf_w)
                    {
                        a10 = row1[sx];
                    }
                    if ((uint32_t)(sx + 1) < (uint32_t)buf_w)
                    {
                        a11 = row1[sx + 1];
                    }
                }

                if ((a00 | a01 | a10 | a11) == 0)
                {
                    pixel_alpha = 0;
                }
                else if ((a00 & a01 & a10 & a11) == EGUI_ALPHA_100)
                {
                    pixel_alpha = EGUI_ALPHA_100;
                }
                else
                {
                    uint16_t ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
                    uint16_t ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
                    pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));
                }

                if (pixel_alpha > 0)
                {
                    if (pixel_alpha == EGUI_ALPHA_100)
                    {
                        *dst_row = solid_color;
                    }
                    else
                    {
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
                        uint16_t bg = *dst_row;
                        uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                        uint32_t result = (bg_rb_g + ((row_fg_rb_g - bg_rb_g) * ((uint32_t)pixel_alpha >> 3) >> 5)) & 0x07E0F81FUL;
                        *dst_row = (uint16_t)(result | (result >> 16));
#else
                        egui_color_t *back = (egui_color_t *)dst_row;
                        egui_rgb_mix_ptr(back, &row_color, back, pixel_alpha);
#endif
                    }
                }
            }
            else if (entered)
            {
                break;
            }

            rotatedX += inv_m00;
            rotatedY += inv_m10;
            dst_row++;
        }

        Cx_base += inv_m01;
        Cy_base += inv_m11;
    }

    return 1;
}

static int text_transform_draw_alpha8_buffer(const text_transform_ctx_t *ctx, egui_dim_t x, egui_dim_t y, egui_color_t color, const uint8_t *alpha8_buf,
                                             int16_t src_x0, int16_t src_y0, int buf_w, int buf_h, int16_t core_src_y0, int16_t core_src_y1)
{
    int row_color_only_mode = 0;
    int32_t inv_m00;
    int32_t inv_m01;
    int32_t inv_m10;
    int32_t inv_m11;
    int32_t draw_x0;
    int32_t draw_x1;
    int32_t draw_y0;
    int32_t draw_y1;
    int32_t pfb_w;
    int32_t pfb_ox;
    int32_t pfb_oy;
    int32_t canvas_alpha;
    int32_t row_start_offset_x;
    int32_t row_start_offset_y;
    int32_t Cx_base;
    int32_t Cy_base;
    int32_t buf_src_hi_x_q15;
    int32_t buf_src_hi_y_q15;
    int32_t buf_int_max_x_q15;
    int32_t buf_int_max_y_q15;
    int32_t core_src_lo_y_q15;
    int32_t core_src_hi_y_q15;
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
    uint32_t fg_rb_g;
#endif

    if (ctx == NULL || alpha8_buf == NULL || buf_w <= 0 || buf_h <= 0)
    {
        return 0;
    }

    if (core_src_y1 <= core_src_y0)
    {
        return 1;
    }

    core_src_lo_y_q15 = ((int32_t)core_src_y0 - src_y0) << 15;
    core_src_hi_y_q15 = ((int32_t)core_src_y1 - src_y0) << 15;

    if (core_src_lo_y_q15 < 0)
    {
        core_src_lo_y_q15 = 0;
    }
    if (core_src_hi_y_q15 > ((int32_t)buf_h << 15))
    {
        core_src_hi_y_q15 = (int32_t)buf_h << 15;
    }

    if (ctx->mask == NULL && ctx->canvas_alpha == EGUI_ALPHA_100)
    {
        return text_transform_draw_alpha8_buffer_opaque_nomask(ctx, x, y, color, alpha8_buf, src_x0, src_y0, buf_w, buf_h, core_src_lo_y_q15,
                                                               core_src_hi_y_q15);
    }
    if (ctx->canvas_alpha == EGUI_ALPHA_100 && ctx->mask != NULL && ctx->mask->api != NULL && ctx->mask->api->kind == EGUI_MASK_KIND_GRADIENT &&
        ctx->mask->api->mask_blend_row_color != NULL)
    {
        egui_color_t probe_color = color;

        if (transform_prepare_row_color(ctx->mask, (egui_dim_t)ctx->draw_y0, &probe_color))
        {
            return text_transform_draw_alpha8_buffer_opaque_row_color(ctx, x, y, color, alpha8_buf, src_x0, src_y0, buf_w, buf_h, core_src_lo_y_q15,
                                                                      core_src_hi_y_q15);
        }
    }

    inv_m00 = ctx->inv_m00;
    inv_m01 = ctx->inv_m01;
    inv_m10 = ctx->inv_m10;
    inv_m11 = ctx->inv_m11;
    draw_x0 = ctx->draw_x0;
    draw_x1 = ctx->draw_x1;
    draw_y0 = ctx->draw_y0;
    draw_y1 = ctx->draw_y1;
    pfb_w = ctx->pfb_w;
    pfb_ox = ctx->pfb_ox;
    pfb_oy = ctx->pfb_oy;
    canvas_alpha = ctx->canvas_alpha;

    if (ctx->mask != NULL)
    {
        egui_color_t probe_color = color;

        if (ctx->mask->api == NULL || ctx->mask->api->kind != EGUI_MASK_KIND_GRADIENT || ctx->mask->api->mask_blend_row_color == NULL ||
            !transform_prepare_row_color(ctx->mask, (egui_dim_t)ctx->draw_y0, &probe_color))
        {
            return 0;
        }

        row_color_only_mode = 1;
    }

    row_start_offset_x = inv_m00 * (draw_x0 - x);
    row_start_offset_y = inv_m10 * (draw_x0 - x);
    Cx_base = ctx->Cx_base - ((int32_t)src_x0 << 15);
    Cy_base = ctx->Cy_base - ((int32_t)src_y0 << 15);
    buf_src_hi_x_q15 = (int32_t)buf_w << 15;
    buf_src_hi_y_q15 = (int32_t)buf_h << 15;
    buf_int_max_x_q15 = ((int32_t)buf_w - 2) << 15;
    buf_int_max_y_q15 = ((int32_t)buf_h - 2) << 15;
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
    fg_rb_g = (color.full | ((uint32_t)color.full << 16)) & 0x07E0F81FUL;
#endif

    for (int32_t dy = draw_y0; dy < draw_y1; dy++)
    {
        egui_color_t row_color = color;
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
        uint32_t row_fg_rb_g = fg_rb_g;
#endif

        if (row_color_only_mode)
        {
            transform_prepare_row_color(ctx->mask, (egui_dim_t)dy, &row_color);
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
            row_fg_rb_g = (row_color.full | ((uint32_t)row_color.full << 16)) & 0x07E0F81FUL;
#endif
        }

        int32_t rotatedX = row_start_offset_x + Cx_base;
        int32_t rotatedY = row_start_offset_y + Cy_base;
        egui_color_int_t *dst_row = &ctx->pfb[(dy - pfb_oy) * pfb_w + (draw_x0 - pfb_ox)];
        int entered = 0;
        int32_t dx = draw_x0;

        if (rotatedX < 0 || rotatedX >= buf_src_hi_x_q15 || rotatedY < 0 || rotatedY >= buf_src_hi_y_q15)
        {
            int32_t skip = transform_scanline_skip(rotatedX, rotatedY, inv_m00, inv_m10, 0, buf_src_hi_x_q15, 0, buf_src_hi_y_q15, draw_x1 - draw_x0);
            if (skip > 0)
            {
                rotatedX += skip * inv_m00;
                rotatedY += skip * inv_m10;
                dst_row += skip;
                dx += skip;
            }
        }

        for (; dx < draw_x1; dx++)
        {
            int32_t sx = rotatedX >> 15;
            int32_t sy = rotatedY >> 15;

            if (sx >= 0 && sx < buf_w - 1 && sy >= 0 && sy < buf_h - 1)
            {
                entered = 1;
                int32_t sir_count = draw_x1 - dx;

                if (inv_m00 > 0)
                {
                    int32_t n = (buf_int_max_x_q15 - rotatedX) / inv_m00;
                    if (n < sir_count)
                    {
                        sir_count = n;
                    }
                }
                else if (inv_m00 < 0)
                {
                    int32_t n = rotatedX / (-inv_m00);
                    if (n < sir_count)
                    {
                        sir_count = n;
                    }
                }

                if (inv_m10 > 0)
                {
                    int32_t n = (buf_int_max_y_q15 - rotatedY) / inv_m10;
                    if (n < sir_count)
                    {
                        sir_count = n;
                    }
                }
                else if (inv_m10 < 0)
                {
                    int32_t n = rotatedY / (-inv_m10);
                    if (n < sir_count)
                    {
                        sir_count = n;
                    }
                }

                if (sir_count < 1)
                {
                    sir_count = 1;
                }

                if (canvas_alpha == EGUI_ALPHA_100)
                {
                    for (int32_t i = 0; i < sir_count; i++)
                    {
                        if (rotatedY < core_src_lo_y_q15 || rotatedY >= core_src_hi_y_q15)
                        {
                            rotatedX += inv_m00;
                            rotatedY += inv_m10;
                            dst_row++;
                            continue;
                        }

                        int32_t sample_sx = rotatedX >> 15;
                        int32_t sample_sy = rotatedY >> 15;
                        const uint8_t *row0 = alpha8_buf + sample_sy * buf_w + sample_sx;
                        const uint8_t *row1 = row0 + buf_w;
                        uint8_t a00 = row0[0];
                        uint8_t a01 = row0[1];
                        uint8_t a10 = row1[0];
                        uint8_t a11 = row1[1];
                        uint8_t pixel_alpha;

                        if ((a00 | a01 | a10 | a11) == 0)
                        {
                            rotatedX += inv_m00;
                            rotatedY += inv_m10;
                            dst_row++;
                            continue;
                        }

                        if ((a00 & a01 & a10 & a11) == EGUI_ALPHA_100)
                        {
                            pixel_alpha = EGUI_ALPHA_100;
                        }
                        else
                        {
                            uint8_t fx = (rotatedX >> 7) & 0xFF;
                            uint8_t fy = (rotatedY >> 7) & 0xFF;
                            uint16_t ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
                            uint16_t ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
                            pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));
                        }

                        if (pixel_alpha > 0)
                        {
                            if (pixel_alpha == EGUI_ALPHA_100)
                            {
                                *dst_row = row_color.full;
                            }
                            else if (pixel_alpha > 0)
                            {
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
                                uint16_t bg = *dst_row;
                                uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                                uint32_t result = (bg_rb_g + ((row_fg_rb_g - bg_rb_g) * ((uint32_t)pixel_alpha >> 3) >> 5)) & 0x07E0F81FUL;
                                *dst_row = (uint16_t)(result | (result >> 16));
#else
                                egui_color_t *back = (egui_color_t *)dst_row;
                                egui_rgb_mix_ptr(back, &row_color, back, pixel_alpha);
#endif
                            }
                        }

                        rotatedX += inv_m00;
                        rotatedY += inv_m10;
                        dst_row++;
                    }
                }
                else
                {
                    for (int32_t i = 0; i < sir_count; i++)
                    {
                        if (rotatedY < core_src_lo_y_q15 || rotatedY >= core_src_hi_y_q15)
                        {
                            rotatedX += inv_m00;
                            rotatedY += inv_m10;
                            dst_row++;
                            continue;
                        }

                        int32_t sample_sx = rotatedX >> 15;
                        int32_t sample_sy = rotatedY >> 15;
                        const uint8_t *row0 = alpha8_buf + sample_sy * buf_w + sample_sx;
                        const uint8_t *row1 = row0 + buf_w;
                        uint8_t a00 = row0[0];
                        uint8_t a01 = row0[1];
                        uint8_t a10 = row1[0];
                        uint8_t a11 = row1[1];
                        uint8_t pixel_alpha;

                        if ((a00 | a01 | a10 | a11) == 0)
                        {
                            rotatedX += inv_m00;
                            rotatedY += inv_m10;
                            dst_row++;
                            continue;
                        }

                        if ((a00 & a01 & a10 & a11) == EGUI_ALPHA_100)
                        {
                            pixel_alpha = EGUI_ALPHA_100;
                        }
                        else
                        {
                            uint8_t fx = (rotatedX >> 7) & 0xFF;
                            uint8_t fy = (rotatedY >> 7) & 0xFF;
                            uint16_t ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
                            uint16_t ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
                            pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));
                        }

                        if (pixel_alpha > 0)
                        {
                            egui_alpha_t final_alpha = ((uint16_t)canvas_alpha * pixel_alpha + 128) >> 8;

                            if (final_alpha == EGUI_ALPHA_100)
                            {
                                *dst_row = row_color.full;
                            }
                            else if (final_alpha > 0)
                            {
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
                                uint16_t bg = *dst_row;
                                uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                                uint32_t result = (bg_rb_g + ((row_fg_rb_g - bg_rb_g) * ((uint32_t)final_alpha >> 3) >> 5)) & 0x07E0F81FUL;
                                *dst_row = (uint16_t)(result | (result >> 16));
#else
                                egui_color_t *back = (egui_color_t *)dst_row;
                                egui_rgb_mix_ptr(back, &row_color, back, final_alpha);
#endif
                            }
                        }

                        rotatedX += inv_m00;
                        rotatedY += inv_m10;
                        dst_row++;
                    }
                }

                dx += sir_count - 1;
                continue;
            }
            else if (sx >= -1 && sx < buf_w && sy >= -1 && sy < buf_h)
            {
                uint8_t fx = (rotatedX >> 7) & 0xFF;
                uint8_t fy = (rotatedY >> 7) & 0xFF;
                uint8_t a00 = 0;
                uint8_t a01 = 0;
                uint8_t a10 = 0;
                uint8_t a11 = 0;

                entered = 1;

                if (rotatedY < core_src_lo_y_q15 || rotatedY >= core_src_hi_y_q15)
                {
                    rotatedX += inv_m00;
                    rotatedY += inv_m10;
                    dst_row++;
                    continue;
                }

                if ((uint32_t)sy < (uint32_t)buf_h)
                {
                    const uint8_t *row0 = alpha8_buf + sy * buf_w;

                    if ((uint32_t)sx < (uint32_t)buf_w)
                    {
                        a00 = row0[sx];
                    }
                    if ((uint32_t)(sx + 1) < (uint32_t)buf_w)
                    {
                        a01 = row0[sx + 1];
                    }
                }

                if ((uint32_t)(sy + 1) < (uint32_t)buf_h)
                {
                    const uint8_t *row1 = alpha8_buf + (sy + 1) * buf_w;

                    if ((uint32_t)sx < (uint32_t)buf_w)
                    {
                        a10 = row1[sx];
                    }
                    if ((uint32_t)(sx + 1) < (uint32_t)buf_w)
                    {
                        a11 = row1[sx + 1];
                    }
                }

                {
                    uint8_t pixel_alpha;

                    if ((a00 | a01 | a10 | a11) == 0)
                    {
                        pixel_alpha = 0;
                    }
                    else if ((a00 & a01 & a10 & a11) == EGUI_ALPHA_100)
                    {
                        pixel_alpha = EGUI_ALPHA_100;
                    }
                    else
                    {
                        uint16_t ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
                        uint16_t ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
                        pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));
                    }

                    if (pixel_alpha > 0)
                    {
                        if (canvas_alpha == EGUI_ALPHA_100)
                        {
                            if (pixel_alpha == EGUI_ALPHA_100)
                            {
                                *dst_row = row_color.full;
                            }
                            else
                            {
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
                                uint16_t bg = *dst_row;
                                uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                                uint32_t result = (bg_rb_g + ((row_fg_rb_g - bg_rb_g) * ((uint32_t)pixel_alpha >> 3) >> 5)) & 0x07E0F81FUL;
                                *dst_row = (uint16_t)(result | (result >> 16));
#else
                                egui_color_t *back = (egui_color_t *)dst_row;
                                egui_rgb_mix_ptr(back, &row_color, back, pixel_alpha);
#endif
                            }
                        }
                        else
                        {
                            egui_alpha_t final_alpha = ((uint16_t)canvas_alpha * pixel_alpha + 128) >> 8;

                            if (final_alpha == EGUI_ALPHA_100)
                            {
                                *dst_row = row_color.full;
                            }
                            else if (final_alpha > 0)
                            {
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
                                uint16_t bg = *dst_row;
                                uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                                uint32_t result = (bg_rb_g + ((row_fg_rb_g - bg_rb_g) * ((uint32_t)final_alpha >> 3) >> 5)) & 0x07E0F81FUL;
                                *dst_row = (uint16_t)(result | (result >> 16));
#else
                                egui_color_t *back = (egui_color_t *)dst_row;
                                egui_rgb_mix_ptr(back, &row_color, back, final_alpha);
#endif
                            }
                        }
                    }
                }
            }
            else if (entered)
            {
                break;
            }

            rotatedX += inv_m00;
            rotatedY += inv_m10;
            dst_row++;
        }

        Cx_base += inv_m01;
        Cy_base += inv_m11;
    }

    return 1;
}

static inline void rasterize_glyph4_row_to_packed(uint8_t *dst_row, int dst_x, const uint8_t *src_row, int box_w, int overwrite)
{
    int dst_byte = dst_x >> 1;
    int full_pairs = box_w >> 1;
    int has_tail = box_w & 1;

    if ((dst_x & 1) == 0)
    {
        for (int i = 0; i < full_pairs; i++)
        {
            if (overwrite)
            {
                dst_row[dst_byte + i] = src_row[i];
            }
            else
            {
                dst_row[dst_byte + i] |= src_row[i];
            }
        }

        if (has_tail)
        {
            if (overwrite)
            {
                dst_row[dst_byte + full_pairs] = (dst_row[dst_byte + full_pairs] & 0xF0) | (src_row[full_pairs] & 0x0F);
            }
            else
            {
                dst_row[dst_byte + full_pairs] |= src_row[full_pairs] & 0x0F;
            }
        }

        return;
    }

    for (int i = 0; i < full_pairs; i++)
    {
        uint8_t packed = src_row[i];
        uint8_t dst0 = (uint8_t)((packed & 0x0F) << 4);
        uint8_t dst1 = (uint8_t)((packed >> 4) & 0x0F);

        if (overwrite)
        {
            dst_row[dst_byte + i] = (dst_row[dst_byte + i] & 0x0F) | dst0;
            dst_row[dst_byte + i + 1] = (dst_row[dst_byte + i + 1] & 0xF0) | dst1;
        }
        else
        {
            dst_row[dst_byte + i] |= dst0;
            dst_row[dst_byte + i + 1] |= dst1;
        }
    }

    if (has_tail)
    {
        uint8_t dst0 = (uint8_t)((src_row[full_pairs] & 0x0F) << 4);

        if (overwrite)
        {
            dst_row[dst_byte + full_pairs] = (dst_row[dst_byte + full_pairs] & 0x0F) | dst0;
        }
        else
        {
            dst_row[dst_byte + full_pairs] |= dst0;
        }
    }
}

static void rasterize_glyph4_to_packed_inside_common(uint8_t *buf, int buf_w, int buf_h, int dst_x, int dst_y, const uint8_t *glyph_data, int box_w, int box_h,
                                                     int overwrite)
{
    int src_rb = (box_w + 1) >> 1;
    int dst_rb = (buf_w + 1) >> 1;

    if (dst_x >= 0 && dst_y >= 0 && (dst_x + box_w) <= buf_w && (dst_y + box_h) <= buf_h)
    {
        for (int row = 0; row < box_h; row++)
        {
            const uint8_t *src_row = glyph_data + row * src_rb;
            uint8_t *dst_row = buf + (dst_y + row) * dst_rb;

            rasterize_glyph4_row_to_packed(dst_row, dst_x, src_row, box_w, overwrite);
        }
        return;
    }

    for (int row = 0; row < box_h; row++)
    {
        int py = dst_y + row;

        if (py < 0 || py >= buf_h)
        {
            continue;
        }

        for (int col = 0; col < box_w; col++)
        {
            int px = dst_x + col;
            uint8_t raw;

            if (px < 0 || px >= buf_w)
            {
                continue;
            }

            raw = extract_packed_raw(glyph_data, box_w, col, row, 4);
            if (raw)
            {
                write_packed_raw(buf, buf_w, px, py, raw, 4);
            }
        }
    }
}

static void rasterize_glyph4_to_packed_inside(uint8_t *buf, int buf_w, int buf_h, int dst_x, int dst_y, const uint8_t *glyph_data, int box_w, int box_h)
{
    rasterize_glyph4_to_packed_inside_common(buf, buf_w, buf_h, dst_x, dst_y, glyph_data, box_w, box_h, 0);
}

static void rasterize_glyph4_to_packed_inside_overwrite(uint8_t *buf, int buf_w, int buf_h, int dst_x, int dst_y, const uint8_t *glyph_data, int box_w,
                                                        int box_h)
{
    rasterize_glyph4_to_packed_inside_common(buf, buf_w, buf_h, dst_x, dst_y, glyph_data, box_w, box_h, 1);
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static int rasterize_external_layout_glyph4_to_packed_inside_common(uint8_t *buf, int buf_w, int buf_h, int dst_x, int dst_y,
                                                                    const egui_font_std_info_t *font_info, const text_transform_layout_glyph_t *glyph,
                                                                    int overwrite, text_transform_external_layout_glyph_row_scratch_t *scratch)
{
    int chunk_rows;
    int row_bytes;
    int dst_rb;

    if (glyph == NULL || glyph->box_w <= 0 || glyph->box_h <= 0)
    {
        return 1;
    }

    row_bytes = (glyph->box_w + 1) >> 1;
    dst_rb = (buf_w + 1) >> 1;
    if (buf == NULL || font_info == NULL || scratch == NULL || scratch->row_buf == NULL || row_bytes <= 0 || row_bytes > scratch->row_capacity)
    {
        EGUI_ASSERT(0);
        return 0;
    }

    chunk_rows = scratch->chunk_rows;
    if (chunk_rows <= 0)
    {
        EGUI_ASSERT(0);
        return 0;
    }

    if (dst_x >= 0 && dst_y >= 0 && (dst_x + glyph->box_w) <= buf_w && (dst_y + glyph->box_h) <= buf_h)
    {
        for (int row = 0; row < glyph->box_h;)
        {
            int rows_this_chunk = glyph->box_h - row;

            if (rows_this_chunk > chunk_rows)
            {
                rows_this_chunk = chunk_rows;
            }
            if (!text_transform_load_external_layout_glyph_rows(font_info, glyph, row, rows_this_chunk, scratch))
            {
                return 0;
            }

            for (int chunk_row = 0; chunk_row < rows_this_chunk; chunk_row++)
            {
                const uint8_t *src_row = scratch->row_buf + chunk_row * row_bytes;
                uint8_t *dst_row = buf + (dst_y + row + chunk_row) * dst_rb;

                rasterize_glyph4_row_to_packed(dst_row, dst_x, src_row, glyph->box_w, overwrite);
            }

            row += rows_this_chunk;
        }
        return 1;
    }

    for (int row = 0; row < glyph->box_h;)
    {
        int rows_this_chunk = glyph->box_h - row;

        if (rows_this_chunk > chunk_rows)
        {
            rows_this_chunk = chunk_rows;
        }
        if (!text_transform_load_external_layout_glyph_rows(font_info, glyph, row, rows_this_chunk, scratch))
        {
            return 0;
        }

        for (int chunk_row = 0; chunk_row < rows_this_chunk; chunk_row++)
        {
            const uint8_t *src_row = scratch->row_buf + chunk_row * row_bytes;
            int py = dst_y + row + chunk_row;

            if (py < 0 || py >= buf_h)
            {
                continue;
            }

            for (int col = 0; col < glyph->box_w; col++)
            {
                int px = dst_x + col;
                uint8_t raw;

                if (px < 0 || px >= buf_w)
                {
                    continue;
                }

                raw = (uint8_t)((src_row[col >> 1] >> ((col & 1) << 2)) & 0x0F);
                if (raw)
                {
                    write_packed_raw(buf, buf_w, px, py, raw, 4);
                }
            }
        }

        row += rows_this_chunk;
    }

    return 1;
}

static int rasterize_external_layout_glyph4_to_packed_inside(uint8_t *buf, int buf_w, int buf_h, int dst_x, int dst_y, const egui_font_std_info_t *font_info,
                                                             const text_transform_layout_glyph_t *glyph,
                                                             text_transform_external_layout_glyph_row_scratch_t *scratch)
{
    return rasterize_external_layout_glyph4_to_packed_inside_common(buf, buf_w, buf_h, dst_x, dst_y, font_info, glyph, 0, scratch);
}

static int rasterize_external_layout_glyph4_to_packed_inside_overwrite(uint8_t *buf, int buf_w, int buf_h, int dst_x, int dst_y,
                                                                       const egui_font_std_info_t *font_info, const text_transform_layout_glyph_t *glyph,
                                                                       text_transform_external_layout_glyph_row_scratch_t *scratch)
{
    return rasterize_external_layout_glyph4_to_packed_inside_common(buf, buf_w, buf_h, dst_x, dst_y, font_info, glyph, 1, scratch);
}
#endif

static inline int batch_extract_packed_raw_4_trivial(const uint8_t *buf, int rb, int sx, int sy, uint8_t *pixel_alpha, uint8_t *a00, uint8_t *a01, uint8_t *a10,
                                                     uint8_t *a11);

static int text_transform_draw_packed4_buffer(const text_transform_ctx_t *ctx, egui_dim_t x, egui_dim_t y, egui_color_t color, const uint8_t *packed_buf,
                                              int16_t src_x0, int16_t src_y0, int buf_w, int buf_h)
{
    int32_t Cx_base;
    int32_t Cy_base;
    int32_t row_start_offset_x;
    int32_t row_start_offset_y;
    int32_t buf_src_lo_x_q15 = 0;
    int32_t buf_src_hi_x_q15 = ((int32_t)buf_w - 1) << 15;
    int32_t buf_src_lo_y_q15 = 0;
    int32_t buf_src_hi_y_q15 = ((int32_t)buf_h - 1) << 15;
    int32_t buf_int_max_x_q15 = ((int32_t)(buf_w - 2)) << 15;
    int32_t buf_int_max_y_q15 = ((int32_t)(buf_h - 2)) << 15;
    int packed_rb = (buf_w + 1) >> 1;

    if (ctx == NULL || packed_buf == NULL || buf_w <= 0 || buf_h <= 0)
    {
        return 0;
    }

    row_start_offset_x = ctx->inv_m00 * ((int32_t)ctx->draw_x0 - x);
    row_start_offset_y = ctx->inv_m10 * ((int32_t)ctx->draw_x0 - x);
    Cx_base = ctx->Cx_base - ((int32_t)src_x0 << 15);
    Cy_base = ctx->Cy_base - ((int32_t)src_y0 << 15);

    for (int32_t dy = ctx->draw_y0; dy < ctx->draw_y1; dy++)
    {
        egui_color_t row_color = color;
        int row_color_handled = transform_prepare_row_color(ctx->mask, (egui_dim_t)dy, &row_color);
        int mask_requires_point = (ctx->mask != NULL && !row_color_handled);
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
        uint32_t fg_rb_g = (row_color.full | ((uint32_t)row_color.full << 16)) & 0x07E0F81FUL;
#endif
        int32_t rotatedX = row_start_offset_x + Cx_base;
        int32_t rotatedY = row_start_offset_y + Cy_base;
        egui_color_int_t *dst_row = &ctx->pfb[(dy - ctx->pfb_oy) * ctx->pfb_w + (ctx->draw_x0 - ctx->pfb_ox)];
        int32_t dx = ctx->draw_x0;
        int entered = 0;

        if (rotatedX < buf_src_lo_x_q15 || rotatedX >= buf_src_hi_x_q15 || rotatedY < buf_src_lo_y_q15 || rotatedY >= buf_src_hi_y_q15)
        {
            int32_t skip = transform_scanline_skip(rotatedX, rotatedY, ctx->inv_m00, ctx->inv_m10, buf_src_lo_x_q15, buf_src_hi_x_q15, buf_src_lo_y_q15,
                                                   buf_src_hi_y_q15, ctx->draw_x1 - ctx->draw_x0);
            if (skip > 0)
            {
                rotatedX += skip * ctx->inv_m00;
                rotatedY += skip * ctx->inv_m10;
                dst_row += skip;
                dx += skip;
            }
        }

        for (; dx < ctx->draw_x1; dx++)
        {
            int32_t sx = rotatedX >> 15;
            int32_t sy = rotatedY >> 15;

            if (sx >= 0 && sx < buf_w - 1 && sy >= 0 && sy < buf_h - 1)
            {
                int32_t sir_count = ctx->draw_x1 - dx;

                entered = 1;

                if (ctx->inv_m00 > 0)
                {
                    int32_t n = (buf_int_max_x_q15 - rotatedX) / ctx->inv_m00;
                    if (n < sir_count)
                    {
                        sir_count = n;
                    }
                }
                else if (ctx->inv_m00 < 0)
                {
                    int32_t n = rotatedX / (-ctx->inv_m00);
                    if (n < sir_count)
                    {
                        sir_count = n;
                    }
                }

                if (ctx->inv_m10 > 0)
                {
                    int32_t n = (buf_int_max_y_q15 - rotatedY) / ctx->inv_m10;
                    if (n < sir_count)
                    {
                        sir_count = n;
                    }
                }
                else if (ctx->inv_m10 < 0)
                {
                    int32_t n = rotatedY / (-ctx->inv_m10);
                    if (n < sir_count)
                    {
                        sir_count = n;
                    }
                }

                if (sir_count < 1)
                {
                    sir_count = 1;
                }

                if (ctx->canvas_alpha == EGUI_ALPHA_100)
                {
                    if (mask_requires_point)
                    {
                        for (int32_t i = 0; i < sir_count; i++)
                        {
                            uint8_t pixel_alpha;
                            uint8_t a00;
                            uint8_t a01;
                            uint8_t a10;
                            uint8_t a11;
                            int trivial = batch_extract_packed_raw_4_trivial(packed_buf, packed_rb, rotatedX >> 15, rotatedY >> 15, &pixel_alpha, &a00, &a01,
                                                                             &a10, &a11);

                            if (!trivial)
                            {
                                pixel_alpha = bilinear_alpha_from_raw_4(a00, a01, a10, a11, (rotatedX >> 7) & 0xFF, (rotatedY >> 7) & 0xFF);
                            }

                            if (pixel_alpha > 0)
                            {
                                transform_apply_mask_and_blend(ctx->mask, (egui_dim_t)(dx + i), (egui_dim_t)dy, row_color, pixel_alpha, ctx->canvas_alpha,
                                                               dst_row);
                            }

                            rotatedX += ctx->inv_m00;
                            rotatedY += ctx->inv_m10;
                            dst_row++;
                        }
                    }
                    else
                    {
                        for (int32_t i = 0; i < sir_count; i++)
                        {
                            uint8_t pixel_alpha;
                            uint8_t a00;
                            uint8_t a01;
                            uint8_t a10;
                            uint8_t a11;
                            int trivial = batch_extract_packed_raw_4_trivial(packed_buf, packed_rb, rotatedX >> 15, rotatedY >> 15, &pixel_alpha, &a00, &a01,
                                                                             &a10, &a11);

                            if (!trivial)
                            {
                                pixel_alpha = bilinear_alpha_from_raw_4(a00, a01, a10, a11, (rotatedX >> 7) & 0xFF, (rotatedY >> 7) & 0xFF);
                            }

                            if (pixel_alpha == EGUI_ALPHA_100)
                            {
                                *dst_row = row_color.full;
                            }
                            else if (pixel_alpha > 0)
                            {
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
                                uint16_t bg = *dst_row;
                                uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                                uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * ((uint32_t)pixel_alpha >> 3) >> 5)) & 0x07E0F81FUL;
                                *dst_row = (uint16_t)(result | (result >> 16));
#else
                                egui_color_t *back = (egui_color_t *)dst_row;
                                egui_rgb_mix_ptr(back, &row_color, back, pixel_alpha);
#endif
                            }

                            rotatedX += ctx->inv_m00;
                            rotatedY += ctx->inv_m10;
                            dst_row++;
                        }
                    }
                }
                else
                {
                    if (mask_requires_point)
                    {
                        for (int32_t i = 0; i < sir_count; i++)
                        {
                            uint8_t pixel_alpha;
                            uint8_t a00;
                            uint8_t a01;
                            uint8_t a10;
                            uint8_t a11;
                            int trivial = batch_extract_packed_raw_4_trivial(packed_buf, packed_rb, rotatedX >> 15, rotatedY >> 15, &pixel_alpha, &a00, &a01,
                                                                             &a10, &a11);

                            if (!trivial)
                            {
                                pixel_alpha = bilinear_alpha_from_raw_4(a00, a01, a10, a11, (rotatedX >> 7) & 0xFF, (rotatedY >> 7) & 0xFF);
                            }

                            if (pixel_alpha > 0)
                            {
                                transform_apply_mask_and_blend(ctx->mask, (egui_dim_t)(dx + i), (egui_dim_t)dy, row_color, pixel_alpha, ctx->canvas_alpha,
                                                               dst_row);
                            }

                            rotatedX += ctx->inv_m00;
                            rotatedY += ctx->inv_m10;
                            dst_row++;
                        }
                    }
                    else
                    {
                        for (int32_t i = 0; i < sir_count; i++)
                        {
                            uint8_t pixel_alpha;
                            uint8_t a00;
                            uint8_t a01;
                            uint8_t a10;
                            uint8_t a11;
                            int trivial = batch_extract_packed_raw_4_trivial(packed_buf, packed_rb, rotatedX >> 15, rotatedY >> 15, &pixel_alpha, &a00, &a01,
                                                                             &a10, &a11);

                            if (!trivial)
                            {
                                pixel_alpha = bilinear_alpha_from_raw_4(a00, a01, a10, a11, (rotatedX >> 7) & 0xFF, (rotatedY >> 7) & 0xFF);
                            }

                            if (pixel_alpha > 0)
                            {
                                egui_alpha_t final_alpha = ((uint16_t)ctx->canvas_alpha * pixel_alpha + 128) >> 8;

                                if (final_alpha == EGUI_ALPHA_100)
                                {
                                    *dst_row = row_color.full;
                                }
                                else if (final_alpha > 0)
                                {
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
                                    uint16_t bg = *dst_row;
                                    uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                                    uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * ((uint32_t)final_alpha >> 3) >> 5)) & 0x07E0F81FUL;
                                    *dst_row = (uint16_t)(result | (result >> 16));
#else
                                    egui_color_t *back = (egui_color_t *)dst_row;
                                    egui_rgb_mix_ptr(back, &row_color, back, final_alpha);
#endif
                                }
                            }

                            rotatedX += ctx->inv_m00;
                            rotatedY += ctx->inv_m10;
                            dst_row++;
                        }
                    }
                }

                dx += sir_count - 1;
                continue;
            }
            else if (sx >= -1 && sx < buf_w && sy >= -1 && sy < buf_h)
            {
                uint16_t a00;
                uint16_t a01;
                uint16_t a10;
                uint16_t a11;
                uint8_t fx = (rotatedX >> 7) & 0xFF;
                uint8_t fy = (rotatedY >> 7) & 0xFF;
                uint16_t ah0;
                uint16_t ah1;
                uint8_t pixel_alpha;

                entered = 1;

#define PACKED_FETCH_ALPHA_4(px, py)                                                                                                                           \
    (((px) >= 0 && (px) < buf_w && (py) >= 0 && (py) < buf_h) ? read_packed_mask_alpha_4_rb(packed_buf, packed_rb, (px), (py)) : 0)
                a00 = PACKED_FETCH_ALPHA_4(sx, sy);
                a01 = PACKED_FETCH_ALPHA_4(sx + 1, sy);
                a10 = PACKED_FETCH_ALPHA_4(sx, sy + 1);
                a11 = PACKED_FETCH_ALPHA_4(sx + 1, sy + 1);
#undef PACKED_FETCH_ALPHA_4

                ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
                ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
                pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));

                if (pixel_alpha > 0)
                {
                    if (mask_requires_point)
                    {
                        transform_apply_mask_and_blend(ctx->mask, (egui_dim_t)dx, (egui_dim_t)dy, row_color, pixel_alpha, ctx->canvas_alpha, dst_row);
                    }
                    else
                    {
                        egui_alpha_t final_alpha = egui_color_alpha_mix(ctx->canvas_alpha, pixel_alpha);

                        if (final_alpha == EGUI_ALPHA_100)
                        {
                            *dst_row = row_color.full;
                        }
                        else if (final_alpha > 0)
                        {
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
                            uint16_t bg = *dst_row;
                            uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                            uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * ((uint32_t)final_alpha >> 3) >> 5)) & 0x07E0F81FUL;
                            *dst_row = (uint16_t)(result | (result >> 16));
#else
                            egui_color_t *back = (egui_color_t *)dst_row;
                            egui_rgb_mix_ptr(back, &row_color, back, final_alpha);
#endif
                        }
                    }
                }
            }
            else if (entered)
            {
                break;
            }

            rotatedX += ctx->inv_m00;
            rotatedY += ctx->inv_m10;
            dst_row++;
        }

        Cx_base += ctx->inv_m01;
        Cy_base += ctx->inv_m11;
    }

    return 1;
}

static int text_transform_draw_visible_alpha8_tile_layout(const text_transform_ctx_t *ctx, egui_dim_t x, egui_dim_t y, egui_color_t color,
                                                          const egui_font_std_info_t *font_info, const text_transform_layout_glyph_t *layout_glyphs,
                                                          const text_transform_layout_index_t *glyph_indices, int glyph_count, int16_t src_x0, int16_t src_y0,
                                                          int16_t src_x1, int16_t src_y1, int glyphs_overlap)
{
    int buf_w;
    int buf_h;
    int buf_size;
    int packed_size;
    uint8_t *packed_buf;
    uint8_t *alpha8_buf;

    if (ctx == NULL || font_info == NULL || layout_glyphs == NULL || glyph_indices == NULL || glyph_count <= 0)
    {
        return 0;
    }

    src_x0--;
    src_y0--;
    src_x1++;
    src_y1++;

    buf_w = src_x1 - src_x0;
    buf_h = src_y1 - src_y0;
    if (buf_w <= 0 || buf_h <= 0)
    {
        return 1;
    }

    buf_size = buf_w * buf_h;
#if EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_STACK_MAX_BYTES > 0
    if (buf_size > 0 && buf_size <= EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_STACK_MAX_BYTES)
    {
        uint8_t alpha8_stack_buf[EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_STACK_MAX_BYTES];

        if (text_transform_rasterize_visible_alpha8_layout(alpha8_stack_buf, buf_w, buf_h, src_x0, src_y0, font_info, layout_glyphs, glyph_indices, glyph_count,
                                                           glyphs_overlap) == 0 &&
            text_transform_draw_alpha8_buffer(ctx, x, y, color, alpha8_stack_buf, src_x0, src_y0, buf_w, buf_h, src_y0, src_y1))
        {
            return 1;
        }
    }
#endif

    /* Keep the alpha8 fast path for smaller visible tiles, but cap its peak heap usage. */
    if (buf_size > 0 && buf_size <= EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES &&
        text_transform_ensure_visible_tile_capacity(buf_size, &g_text_transform_visible_tile_cache.buf, &g_text_transform_visible_tile_cache.capacity) == 0)
    {
        alpha8_buf = g_text_transform_visible_tile_cache.buf;
        if (text_transform_rasterize_visible_alpha8_layout(alpha8_buf, buf_w, buf_h, src_x0, src_y0, font_info, layout_glyphs, glyph_indices, glyph_count,
                                                           glyphs_overlap) == 0 &&
            text_transform_draw_alpha8_buffer(ctx, x, y, color, alpha8_buf, src_x0, src_y0, buf_w, buf_h, src_y0, src_y1))
        {
            return 1;
        }
    }

    /* Larger tiles fall back to packed4 to avoid large transient heap buffers. */
    packed_size = packed_row_bytes(buf_w, 4) * buf_h;
    if (text_transform_ensure_visible_tile_capacity(packed_size, &g_text_transform_visible_tile_cache.buf, &g_text_transform_visible_tile_cache.capacity) == 0)
    {
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        text_transform_external_layout_glyph_row_scratch_t external_row_scratch = {
                0,
        };
#endif
        packed_buf = g_text_transform_visible_tile_cache.buf;
        egui_api_memset(packed_buf, 0, packed_size);

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        if (font_info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
        {
            int max_row_bytes = text_transform_measure_visible_max_row_bytes(layout_glyphs, glyph_indices, glyph_count);

            if (max_row_bytes > 0 && text_transform_external_layout_glyph_row_scratch_init(&external_row_scratch, max_row_bytes) != 0)
            {
                EGUI_ASSERT(0);
                return 0;
            }
        }
#endif

        if (glyphs_overlap)
        {
            for (int i = 0; i < glyph_count; i++)
            {
                const text_transform_layout_glyph_t *glyph = &layout_glyphs[glyph_indices[i]];

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                if (font_info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
                {
                    if (!rasterize_external_layout_glyph4_to_packed_inside(packed_buf, buf_w, buf_h, glyph->x - src_x0, glyph->y - src_y0, font_info, glyph,
                                                                           &external_row_scratch))
                    {
                        text_transform_external_layout_glyph_row_scratch_release(&external_row_scratch);
                        return 0;
                    }
                }
                else
#endif
                {
                    rasterize_glyph4_to_packed_inside(packed_buf, buf_w, buf_h, glyph->x - src_x0, glyph->y - src_y0,
                                                      font_info->pixel_buffer + glyph->pixel_idx, glyph->box_w, glyph->box_h);
                }
            }
        }
        else
        {
            for (int i = 0; i < glyph_count; i++)
            {
                const text_transform_layout_glyph_t *glyph = &layout_glyphs[glyph_indices[i]];

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                if (font_info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
                {
                    if (!rasterize_external_layout_glyph4_to_packed_inside_overwrite(packed_buf, buf_w, buf_h, glyph->x - src_x0, glyph->y - src_y0, font_info,
                                                                                     glyph, &external_row_scratch))
                    {
                        text_transform_external_layout_glyph_row_scratch_release(&external_row_scratch);
                        return 0;
                    }
                }
                else
#endif
                {
                    rasterize_glyph4_to_packed_inside_overwrite(packed_buf, buf_w, buf_h, glyph->x - src_x0, glyph->y - src_y0,
                                                                font_info->pixel_buffer + glyph->pixel_idx, glyph->box_w, glyph->box_h);
                }
            }
        }

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        text_transform_external_layout_glyph_row_scratch_release(&external_row_scratch);
#endif
        if (text_transform_draw_packed4_buffer(ctx, x, y, color, packed_buf, src_x0, src_y0, buf_w, buf_h))
        {
            return 1;
        }
    }

    return 0;
}

/**
 * Batch read 4 bilinear alpha samples from packed mask buffer.
 * One switch per pixel instead of 4; shares row offset and bit position calculations.
 * Requires all 4 sample coordinates to be in bounds.
 */
__EGUI_STATIC_INLINE__ void batch_bilinear_packed_alpha(const uint8_t *buf, int rb, int sx, int sy, uint8_t bpp, uint16_t *a00, uint16_t *a01,
                                                        uint16_t *a10, uint16_t *a11)
{
    int base0 = sy * rb;
    int base1 = base0 + rb;
    switch (bpp)
    {
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    case 4:
    {
        const uint8_t *row0 = buf + base0;
        const uint8_t *row1 = buf + base1;
        int idx = sx >> 1;

        if ((sx & 1) == 0)
        {
            uint8_t packed0 = row0[idx];
            uint8_t packed1 = row1[idx];

            *a00 = egui_alpha_change_table_4[packed0 & 0x0F];
            *a01 = egui_alpha_change_table_4[(packed0 >> 4) & 0x0F];
            *a10 = egui_alpha_change_table_4[packed1 & 0x0F];
            *a11 = egui_alpha_change_table_4[(packed1 >> 4) & 0x0F];
        }
        else
        {
            uint8_t packed00 = row0[idx];
            uint8_t packed01 = row0[idx + 1];
            uint8_t packed10 = row1[idx];
            uint8_t packed11 = row1[idx + 1];

            *a00 = egui_alpha_change_table_4[(packed00 >> 4) & 0x0F];
            *a01 = egui_alpha_change_table_4[packed01 & 0x0F];
            *a10 = egui_alpha_change_table_4[(packed10 >> 4) & 0x0F];
            *a11 = egui_alpha_change_table_4[packed11 & 0x0F];
        }
        break;
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
    case 2:
    {
        int x0_idx = (sx >> 2);
        int x0_bit = (sx & 3) << 1;
        int x1_idx = ((sx + 1) >> 2);
        int x1_bit = ((sx + 1) & 3) << 1;
        *a00 = egui_alpha_change_table_2[(buf[base0 + x0_idx] >> x0_bit) & 0x03];
        *a01 = egui_alpha_change_table_2[(buf[base0 + x1_idx] >> x1_bit) & 0x03];
        *a10 = egui_alpha_change_table_2[(buf[base1 + x0_idx] >> x0_bit) & 0x03];
        *a11 = egui_alpha_change_table_2[(buf[base1 + x1_idx] >> x1_bit) & 0x03];
        break;
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
    case 1:
    {
        int x0_idx = (sx >> 3);
        int x0_bit = sx & 7;
        int x1_idx = ((sx + 1) >> 3);
        int x1_bit = (sx + 1) & 7;
        *a00 = (buf[base0 + x0_idx] >> x0_bit) & 1 ? 255 : 0;
        *a01 = (buf[base0 + x1_idx] >> x1_bit) & 1 ? 255 : 0;
        *a10 = (buf[base1 + x0_idx] >> x0_bit) & 1 ? 255 : 0;
        *a11 = (buf[base1 + x1_idx] >> x1_bit) & 1 ? 255 : 0;
        break;
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
    case 8:
        *a00 = buf[base0 + sx];
        *a01 = buf[base0 + sx + 1];
        *a10 = buf[base1 + sx];
        *a11 = buf[base1 + sx + 1];
        break;
#endif
    default:
        *a00 = *a01 = *a10 = *a11 = 0;
        break;
    }
}

static inline int batch_extract_packed_raw_4_trivial(const uint8_t *buf, int rb, int sx, int sy, uint8_t *pixel_alpha, uint8_t *a00, uint8_t *a01, uint8_t *a10,
                                                     uint8_t *a11)
{
    const uint8_t *row0 = buf + sy * rb;
    const uint8_t *row1 = row0 + rb;
    int idx = sx >> 1;

    if ((sx & 1) == 0)
    {
        uint8_t packed0 = row0[idx];
        uint8_t packed1 = row1[idx];

        if ((packed0 | packed1) == 0)
        {
            *pixel_alpha = 0;
            return 1;
        }

        if ((packed0 & packed1) == 0xFF)
        {
            *pixel_alpha = EGUI_ALPHA_100;
            return 1;
        }

        *a00 = packed0 & 0x0F;
        *a01 = (packed0 >> 4) & 0x0F;
        *a10 = packed1 & 0x0F;
        *a11 = (packed1 >> 4) & 0x0F;
        return 0;
    }

    {
        uint8_t packed00 = row0[idx];
        uint8_t packed01 = row0[idx + 1];
        uint8_t packed10 = row1[idx];
        uint8_t packed11 = row1[idx + 1];

        *a00 = (packed00 >> 4) & 0x0F;
        *a01 = packed01 & 0x0F;
        *a10 = (packed10 >> 4) & 0x0F;
        *a11 = packed11 & 0x0F;
    }

    if ((*a00 | *a01 | *a10 | *a11) == 0)
    {
        *pixel_alpha = 0;
        return 1;
    }

    if ((*a00 & *a01 & *a10 & *a11) == 0x0F)
    {
        *pixel_alpha = EGUI_ALPHA_100;
        return 1;
    }

    return 0;
}
