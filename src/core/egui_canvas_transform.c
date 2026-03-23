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

/**
 * Bilinear interpolate 4 RGB565 pixels in packed domain.
 * Avoids 3 separate egui_rgb_mix calls (saves 2 pack/unpack + 6 branches).
 * 5-bit alpha precision, visually identical to chained egui_rgb_mix.
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
static inline uint32_t image_alpha_buf_size(int16_t w, int16_t h, uint8_t alpha_type)
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

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
typedef struct
{
    const egui_image_std_info_t *info;
    const void *data_addr;
    const void *alpha_addr;
    void *data_buf;
    void *alpha_buf;
    uint32_t data_size;
    uint32_t alpha_size;
} image_transform_external_cache_t;

static image_transform_external_cache_t g_image_transform_external_cache = {0};

static void image_transform_release_external_cache(image_transform_external_cache_t *cache)
{
    if (cache->data_buf != NULL)
    {
        egui_free(cache->data_buf);
    }
    if (cache->alpha_buf != NULL)
    {
        egui_free(cache->alpha_buf);
    }

    memset(cache, 0, sizeof(*cache));
}

static int image_transform_prepare_external_cache(const egui_image_std_info_t *info, int source_is_opaque, const uint16_t **data, const uint8_t **alpha_buf)
{
    image_transform_external_cache_t *cache = &g_image_transform_external_cache;
    uint32_t data_size = (uint32_t)info->width * info->height * sizeof(uint16_t);
    uint32_t alpha_size = 0;
    int data_hit;
    int alpha_hit;

    if (info->alpha_buf != NULL && !source_is_opaque)
    {
        alpha_size = image_alpha_buf_size(info->width, info->height, info->alpha_type);
    }

    data_hit = cache->info == info && cache->data_addr == info->data_buf && cache->data_size == data_size && cache->data_buf != NULL;
    alpha_hit = alpha_size == 0 ||
                (cache->info == info && cache->alpha_addr == info->alpha_buf && cache->alpha_size == alpha_size && cache->alpha_buf != NULL);

    if (!data_hit || !alpha_hit)
    {
        void *new_data_buf = egui_malloc((int)data_size);
        void *new_alpha_buf = NULL;

        if (new_data_buf == NULL)
        {
            return 0;
        }

        egui_api_load_external_resource(new_data_buf, (egui_uintptr_t)(info->data_buf), 0, data_size);

        if (alpha_size > 0)
        {
            new_alpha_buf = egui_malloc((int)alpha_size);
            if (new_alpha_buf == NULL)
            {
                egui_free(new_data_buf);
                return 0;
            }

            egui_api_load_external_resource(new_alpha_buf, (egui_uintptr_t)(info->alpha_buf), 0, alpha_size);
        }

        image_transform_release_external_cache(cache);
        cache->info = info;
        cache->data_addr = info->data_buf;
        cache->alpha_addr = info->alpha_buf;
        cache->data_buf = new_data_buf;
        cache->alpha_buf = new_alpha_buf;
        cache->data_size = data_size;
        cache->alpha_size = alpha_size;
    }

    *data = (const uint16_t *)cache->data_buf;
    *alpha_buf = alpha_size > 0 ? (const uint8_t *)cache->alpha_buf : NULL;
    return 1;
}
#endif

/**
 * Read a single alpha value from a packed image alpha buffer at (x, y).
 * Supports all alpha types (1/2/4/8 bits per pixel).
 * The alpha_row_bytes parameter is pre-computed for the specific alpha_type.
 */
static inline uint8_t image_transform_read_alpha(const uint8_t *alpha_buf, int alpha_row_bytes, int x, int y, uint8_t alpha_type)
{
    const uint8_t *row = alpha_buf + y * alpha_row_bytes;
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

static inline uint8_t image_transform_sample_alpha_bilinear_fast(const uint8_t *alpha_buf, int alpha_row_bytes, int x, int y, uint8_t alpha_type,
                                                                 uint8_t fx, uint8_t fy)
{
    const uint8_t *row0 = alpha_buf + y * alpha_row_bytes;
    const uint8_t *row1 = row0 + alpha_row_bytes;

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
void egui_canvas_draw_image_transform(const egui_image_t *img, egui_dim_t x, egui_dim_t y, int16_t angle_deg, int16_t scale_q8)
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
    const uint16_t *data;
    const uint8_t *alpha_buf;

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        if (!image_transform_prepare_external_cache(info, source_is_opaque, &data, &alpha_buf))
        {
            return;
        }
    }
    else
#endif
    {
        data = (const uint16_t *)info->data_buf;
        alpha_buf = source_is_opaque ? NULL : (const uint8_t *)info->alpha_buf;
    }
    int has_alpha = (alpha_buf != NULL);
    uint8_t alpha_type = info->alpha_type;
    int alpha_row_bytes = 0;
    if (has_alpha)
    {
        switch (alpha_type)
        {
        case EGUI_IMAGE_ALPHA_TYPE_8:
            alpha_row_bytes = src_w;
            break;
        case EGUI_IMAGE_ALPHA_TYPE_4:
            alpha_row_bytes = (src_w + 1) >> 1;
            break;
        case EGUI_IMAGE_ALPHA_TYPE_2:
            alpha_row_bytes = (src_w + 3) >> 2;
            break;
        case EGUI_IMAGE_ALPHA_TYPE_1:
            alpha_row_bytes = (src_w + 7) >> 3;
            break;
        default:
            has_alpha = 0;
            break;
        }
    }
    int opaque_mode = (!has_alpha && canvas_alpha == EGUI_ALPHA_100);

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
            int32_t skip = transform_scanline_skip(rotatedX, rotatedY, inv_m00, inv_m10, src_lo_x_q15, src_hi_x_q15, src_lo_y_q15, src_hi_y_q15,
                                                   draw_x1 - draw_x0);
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
                    if (n < sir_count) sir_count = n;
                }
                else if (inv_m00 < 0)
                {
                    int32_t n = rotatedX / (-inv_m00);
                    if (n < sir_count) sir_count = n;
                }
                if (inv_m10 > 0)
                {
                    int32_t n = (int_max_y_q15 - rotatedY) / inv_m10;
                    if (n < sir_count) sir_count = n;
                }
                else if (inv_m10 < 0)
                {
                    int32_t n = rotatedY / (-inv_m10);
                    if (n < sir_count) sir_count = n;
                }
                if (sir_count < 1) sir_count = 1;

                /* Tight interior loop: no boundary checks needed */
                if (opaque_mode && !mask_requires_point)
                {
                    for (int32_t i = 0; i < sir_count; i++)
                    {
                        int32_t offs = (rotatedY >> 15) * src_w + (rotatedX >> 15);
                        uint8_t fx = (rotatedX >> 7) & 0xFF;
                        uint8_t fy = (rotatedY >> 7) & 0xFF;
                        uint16_t d00 = data[offs];
                        uint16_t d01 = data[offs + 1];
                        uint16_t d10 = data[offs + src_w];
                        uint16_t d11 = data[offs + src_w + 1];
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
                        int32_t offs = (rotatedY >> 15) * src_w + (rotatedX >> 15);
                        uint8_t fx = (rotatedX >> 7) & 0xFF;
                        uint8_t fy = (rotatedY >> 7) & 0xFF;
                        egui_alpha_t pixel_alpha =
                                image_transform_sample_alpha_bilinear_fast(alpha_buf, alpha_row_bytes, rotatedX >> 15, rotatedY >> 15, alpha_type, fx, fy);

                        if (pixel_alpha > 0)
                        {
                            uint16_t d00 = data[offs];
                            uint16_t d01 = data[offs + 1];
                            uint16_t d10 = data[offs + src_w];
                            uint16_t d11 = data[offs + src_w + 1];
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
                        int32_t offs = py * src_w + px;
                        uint8_t fx = (rotatedX >> 7) & 0xFF;
                        uint8_t fy = (rotatedY >> 7) & 0xFF;
                        uint16_t d00 = data[offs];
                        uint16_t d01 = data[offs + 1];
                        uint16_t d10 = data[offs + src_w];
                        uint16_t d11 = data[offs + src_w + 1];
                        egui_color_t color;
                        color.full = bilinear_rgb565_packed(d00, d01, d10, d11, fx, fy);
                        if (row_overlay_handled && row_overlay_alpha > 0)
                        {
                            egui_rgb_mix_ptr(&color, &row_overlay_color, &color, row_overlay_alpha);
                        }

                        egui_alpha_t pixel_alpha = EGUI_ALPHA_100;
                        if (has_alpha)
                        {
                            pixel_alpha = image_transform_sample_alpha_bilinear_fast(alpha_buf, alpha_row_bytes, px, py, alpha_type, fx, fy);
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
#define TRANSFORM_FETCH_PIXEL(px, py) (((px) >= 0 && (px) < src_w && (py) >= 0 && (py) < src_h) ? data[(py) * src_w + (px)] : bg)
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
#define TRANSFORM_FETCH_ALPHA(px, py) (((px) >= 0 && (px) < src_w && (py) >= 0 && (py) < src_h) ? image_transform_read_alpha(alpha_buf, alpha_row_bytes, (px), (py), alpha_type) : 0)
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
}

// ============================================================================
// Text transform shared helpers
// ============================================================================

typedef struct
{
    const uint8_t *data; /* packed bitmap data pointer (in ROM) */
    int16_t x;           /* glyph box left edge in text space */
    int16_t y;           /* glyph box top edge in text space */
    uint8_t box_w;       /* glyph box width */
    uint8_t box_h;       /* glyph box height */
    uint8_t row_bytes;   /* packed bytes per glyph row */
    uint8_t line;        /* tile-local line index */
} text_transform_glyph_t;

typedef struct
{
    uint32_t pixel_idx; /* packed bitmap byte offset */
    int16_t x;          /* glyph box left edge in text space */
    int16_t y;          /* glyph box top edge in text space */
    uint8_t box_w;      /* glyph box width */
    uint8_t box_h;      /* glyph box height */
} text_transform_layout_glyph_t;

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
    int start;
    int end;
    int16_t x_min;
    int16_t x_max;
    int16_t y_min;
    int16_t y_max;
} text_transform_layout_line_t;

/**
 * Extract alpha value from packed font bitmap at local glyph coordinates.
 * Each row is byte-aligned in the packed data.
 */
static inline uint8_t extract_packed_alpha(const uint8_t *data, uint8_t box_w, int local_x, int local_y, uint8_t bpp)
{
    switch (bpp)
    {
    case 1:
    {
        int row_bytes = (box_w + 7) >> 3;
        int idx = local_y * row_bytes + (local_x >> 3);
        int bit = local_x & 7;
        return (data[idx] >> bit) & 1 ? 255 : 0;
    }
    case 2:
    {
        int row_bytes = (box_w + 3) >> 2;
        int idx = local_y * row_bytes + (local_x >> 2);
        int bit = (local_x & 3) << 1;
        return egui_alpha_change_table_2[(data[idx] >> bit) & 0x03];
    }
    case 4:
    {
        int row_bytes = (box_w + 1) >> 1;
        int idx = local_y * row_bytes + (local_x >> 1);
        int bit = (local_x & 1) << 2;
        return egui_alpha_change_table_4[(data[idx] >> bit) & 0x0F];
    }
    case 8:
        return data[local_y * box_w + local_x];
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

static inline void batch_extract_glyph_alpha_4(const uint8_t *data, uint8_t box_w, int lx, int ly, uint16_t *a00, uint16_t *a01, uint16_t *a10,
                                               uint16_t *a11)
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
    case 4:
        batch_extract_glyph_alpha_4(data, box_w, lx, ly, a00, a01, a10, a11);
        break;
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
    default: /* 8bpp */
    {
        int base0 = ly * box_w;
        int base1 = base0 + box_w;
        *a00 = data[base0 + lx];
        *a01 = data[base0 + lx + 1];
        *a10 = data[base1 + lx];
        *a11 = data[base1 + lx + 1];
        break;
    }
    }
}

/**
 * Compute packed row byte count for a given width and bpp.
 */
static inline int packed_row_bytes(int width, uint8_t bpp)
{
    switch (bpp)
    {
    case 1:
        return (width + 7) >> 3;
    case 2:
        return (width + 3) >> 2;
    case 4:
        return (width + 1) >> 1;
    default:
        return width; /* 8bpp or fallback */
    }
}

/**
 * Extract raw packed value from font/mask data (no alpha table conversion).
 * Width can exceed 255 (unified text mask buffer).
 */
static inline uint8_t extract_packed_raw(const uint8_t *data, int data_w, int x, int y, uint8_t bpp)
{
    switch (bpp)
    {
    case 1:
    {
        int rb = (data_w + 7) >> 3;
        return (data[y * rb + (x >> 3)] >> (x & 7)) & 1;
    }
    case 2:
    {
        int rb = (data_w + 3) >> 2;
        return (data[y * rb + (x >> 2)] >> ((x & 3) << 1)) & 0x03;
    }
    case 4:
    {
        int rb = (data_w + 1) >> 1;
        return (data[y * rb + (x >> 1)] >> ((x & 1) << 2)) & 0x0F;
    }
    case 8:
        return data[y * data_w + x];
    default:
        return 0;
    }
}

/**
 * Write a raw packed value into a packed buffer (OR-in, buffer must be pre-zeroed).
 */
static inline void write_packed_raw(uint8_t *buf, int buf_w, int x, int y, uint8_t raw_val, uint8_t bpp)
{
    switch (bpp)
    {
    case 1:
    {
        int rb = (buf_w + 7) >> 3;
        buf[y * rb + (x >> 3)] |= (raw_val & 1) << (x & 7);
        break;
    }
    case 2:
    {
        int rb = (buf_w + 3) >> 2;
        buf[y * rb + (x >> 2)] |= (raw_val & 0x03) << ((x & 3) << 1);
        break;
    }
    case 4:
    {
        int rb = (buf_w + 1) >> 1;
        buf[y * rb + (x >> 1)] |= (raw_val & 0x0F) << ((x & 1) << 2);
        break;
    }
    case 8:
        buf[y * buf_w + x] = raw_val;
        break;
    }
}

/**
 * Read alpha value from a packed mask buffer (int width, supports text_w > 255).
 * Same logic as extract_packed_alpha() but with int width parameter.
 */
static inline uint8_t read_packed_mask_alpha(const uint8_t *buf, int buf_w, int x, int y, uint8_t bpp)
{
    switch (bpp)
    {
    case 1:
    {
        int rb = (buf_w + 7) >> 3;
        return (buf[y * rb + (x >> 3)] >> (x & 7)) & 1 ? 255 : 0;
    }
    case 2:
    {
        int rb = (buf_w + 3) >> 2;
        return egui_alpha_change_table_2[(buf[y * rb + (x >> 2)] >> ((x & 3) << 1)) & 0x03];
    }
    case 4:
    {
        int rb = (buf_w + 1) >> 1;
        return egui_alpha_change_table_4[(buf[y * rb + (x >> 1)] >> ((x & 1) << 2)) & 0x0F];
    }
    case 8:
        return buf[y * buf_w + x];
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

static text_transform_prepare_cache_t g_text_transform_prepare_cache = {0};

static int text_transform_draw_visible_alpha8_tile_layout(const text_transform_ctx_t *ctx, egui_dim_t x, egui_dim_t y, egui_color_t color,
                                                          const egui_font_std_info_t *font_info,
                                                          const text_transform_layout_glyph_t *const *glyphs, int glyph_count, int16_t src_x0, int16_t src_y0,
                                                          int16_t src_x1, int16_t src_y1, int glyphs_overlap);

static int text_transform_prepare(int16_t text_w, int16_t text_h, egui_dim_t x, egui_dim_t y, int16_t angle_deg, int16_t scale_q8, egui_alpha_t alpha,
                                  text_transform_ctx_t *ctx)
{
    text_transform_prepare_cache_t *cache = &g_text_transform_prepare_cache;

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
 * For extreme downscaling (< 0.5x), use egui_canvas_draw_text_transform_buffered().
 * Can be overridden in app_egui_config.h.
 */
#ifndef EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_GLYPHS
#define EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_GLYPHS 128
#endif

#ifndef EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_LINES
#define EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_LINES 32
#endif

static text_transform_layout_cache_t g_text_transform_layout_cache = {
        .font = NULL,
        .string = NULL,
        .line_space = 0,
        .count = 0,
        .line_count = 0,
};
static text_transform_layout_glyph_t *g_text_transform_layout_glyphs = NULL;
static int g_text_transform_layout_capacity = 0;
static text_transform_layout_line_t *g_text_transform_layout_lines = NULL;
static int g_text_transform_layout_line_capacity = 0;

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

static inline uint8_t read_packed_mask_alpha_4(const uint8_t *buf, int buf_w, int x, int y)
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

static int text_transform_build_layout(const egui_font_std_info_t *font_info, const char *string, text_transform_layout_glyph_t *glyphs, int max_glyphs,
                                       text_transform_layout_line_t *lines, int max_lines, int *line_count, egui_dim_t line_space)
{
    const char *s = string;
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
        if (char_bytes <= 0)
        {
            break;
        }
        s += char_bytes;

        int found = egui_font_std_find_code_index(font_info, utf8_code);
        if (found < 0)
        {
            cursor_x += font_info->height >> 1;
            continue;
        }

        const egui_font_std_char_descriptor_t *desc = &font_info->char_array[found];
        if (desc->size == 0)
        {
            cursor_x += font_info->height >> 1;
            continue;
        }

        if (count < max_glyphs)
        {
            glyphs[count].pixel_idx = desc->idx;
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

static int text_transform_prepare_layout(const egui_font_t *font, const egui_font_std_info_t *font_info, const char *string, egui_dim_t line_space,
                                         const text_transform_layout_glyph_t **layout, int *count, const text_transform_layout_line_t **lines, int *line_count)
{
    int needed;
    int line_needed;

    if (font == NULL || font_info == NULL || string == NULL || layout == NULL || count == NULL || lines == NULL || line_count == NULL)
    {
        return -1;
    }

    if (g_text_transform_layout_cache.font == font && g_text_transform_layout_cache.string == string && g_text_transform_layout_cache.line_space == line_space)
    {
        *layout = g_text_transform_layout_glyphs;
        *count = g_text_transform_layout_cache.count;
        *lines = g_text_transform_layout_lines;
        *line_count = g_text_transform_layout_cache.line_count;
        return 0;
    }

    text_transform_measure_layout_slots(string, &needed, &line_needed);
    if (needed > 0 && text_transform_ensure_layout_capacity(needed) != 0)
    {
        return -1;
    }
    if (line_needed > 0 && text_transform_ensure_line_capacity(line_needed) != 0)
    {
        return -1;
    }

    g_text_transform_layout_cache.font = font;
    g_text_transform_layout_cache.string = string;
    g_text_transform_layout_cache.line_space = line_space;
    g_text_transform_layout_cache.count =
            text_transform_build_layout(font_info, string, g_text_transform_layout_glyphs, needed, g_text_transform_layout_lines, line_needed,
                                        &g_text_transform_layout_cache.line_count, line_space);

    *layout = g_text_transform_layout_glyphs;
    *count = g_text_transform_layout_cache.count;
    *lines = g_text_transform_layout_lines;
    *line_count = g_text_transform_layout_cache.line_count;
    return 0;
}

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
                    glyphs[count].data = font_info->pixel_buffer + src->pixel_idx;
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
                                                int layout_line_count, const text_transform_layout_glyph_t **glyphs, int max_glyphs, int16_t sx0, int16_t sy0,
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
                if (count < max_glyphs)
                {
                    int16_t glyph_x0 = src->x;
                    int16_t glyph_y0 = src->y;
                    int16_t glyph_x1 = glyph_x0 + src->box_w;
                    int16_t glyph_y1 = glyph_y0 + src->box_h;

                    glyphs[count++] = src;

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

static inline uint8_t sample_tile_alpha(const text_transform_glyph_t *glyphs, int count, const text_transform_layout_line_t *lines, int line_count, int sx,
                                        int sy, uint8_t bpp, int *hint, int *hint_line)
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

static inline uint8_t sample_tile_alpha_4(const text_transform_glyph_t *glyphs, int count, const text_transform_layout_line_t *lines, int line_count, int sx,
                                          int sy, int *hint, int *hint_line)
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
 * collect only visible glyphs into a small stack array, then samples directly
 * from packed ROM font data. Extra memory = ~1280 bytes stack (128 glyphs × 10 bytes),
 * proportional to PFB size, independent of total text content.
 * Text dimensions are cached (12 bytes static) to avoid per-tile string measurement.
 */
void egui_canvas_draw_text_transform(const egui_font_t *font, const void *string, egui_dim_t x, egui_dim_t y, int16_t angle_deg, int16_t scale_q8,
                                     egui_color_t color, egui_alpha_t alpha)
{
    if (!font || !string || !font->res)
    {
        return;
    }

    if (text_transform_try_draw_axis_aligned(font, string, x, y, angle_deg, scale_q8, color, alpha))
    {
        return;
    }

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

    text_transform_ctx_t ctx;
    if (text_transform_prepare(s_dim_w, s_dim_h, x, y, angle_deg, scale_q8, alpha, &ctx) != 0)
    {
        return;
    }

    egui_font_std_info_t *font_info = (egui_font_std_info_t *)font->res;
    uint8_t bpp = font_info->font_bit_mode;
    egui_font_std_access_t font_access;
    const text_transform_layout_glyph_t *layout_glyphs = NULL;
    int layout_count = 0;
    const text_transform_layout_line_t *layout_lines = NULL;
    int layout_line_count = 0;

    if (egui_font_std_prepare_access(font_info, &font_access) != 0)
    {
        return;
    }
    font_info = &font_access.info;

    if (text_transform_prepare_layout(font, font_info, (const char *)string, 0, &layout_glyphs, &layout_count, &layout_lines, &layout_line_count) != 0)
    {
        egui_font_std_release_access(&font_access);
        return;
    }

    if (layout_count == 0)
    {
        egui_font_std_release_access(&font_access);
        return;
    }

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
     * Use a static pointer allocated once via egui_malloc to avoid large stack usage. */
    static text_transform_glyph_t *s_tile_glyphs = NULL;
    static const text_transform_layout_glyph_t **s_tile_layout_glyphs = NULL;
    static text_transform_layout_line_t s_tile_lines[EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_LINES];
    int tile_line_count = 0;
    int16_t tile_src_x0 = 0;
    int16_t tile_src_y0 = 0;
    int16_t tile_src_x1 = 0;
    int16_t tile_src_y1 = 0;
    int tile_glyphs_overlap = 0;
    if (s_tile_glyphs == NULL)
    {
        s_tile_glyphs = (text_transform_glyph_t *)egui_malloc(EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_GLYPHS * sizeof(text_transform_glyph_t));
        if (s_tile_glyphs == NULL)
        {
            egui_font_std_release_access(&font_access);
            return;
        }
    }
    {
        int use_visible_alpha8_tile =
                (bpp == 4 && scale_q8 >= 256 &&
                 (ctx.mask == NULL || (ctx.mask->api != NULL && ctx.mask->api->kind == EGUI_MASK_KIND_GRADIENT && ctx.mask->api->mask_blend_row_color != NULL)));
        int tile_count;

        if (use_visible_alpha8_tile)
        {
            if (s_tile_layout_glyphs == NULL)
            {
                s_tile_layout_glyphs =
                        (const text_transform_layout_glyph_t **)egui_malloc(EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_GLYPHS * sizeof(*s_tile_layout_glyphs));
                if (s_tile_layout_glyphs == NULL)
                {
                    egui_font_std_release_access(&font_access);
                    return;
                }
            }

            tile_count = collect_visible_layout_glyphs_alpha8(layout_glyphs, layout_lines, layout_line_count, s_tile_layout_glyphs,
                                                              EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_GLYPHS, src_min_x, src_min_y, src_max_x, src_max_y,
                                                              &tile_src_x0, &tile_src_y0, &tile_src_x1, &tile_src_y1, &tile_glyphs_overlap);

            if (tile_count == 0)
            {
                egui_font_std_release_access(&font_access);
                return;
            }

            if (text_transform_draw_visible_alpha8_tile_layout(&ctx, x, y, color, font_info, s_tile_layout_glyphs, tile_count, tile_src_x0, tile_src_y0,
                                                               tile_src_x1, tile_src_y1, tile_glyphs_overlap))
            {
                egui_font_std_release_access(&font_access);
                return;
            }
        }

        tile_count = collect_visible_glyphs(font_info, layout_glyphs, layout_lines, layout_line_count, s_tile_glyphs,
                                            EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_GLYPHS, s_tile_lines, EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_LINES,
                                            &tile_line_count, src_min_x, src_min_y, src_max_x, src_max_y, NULL, NULL, NULL, NULL, NULL, 1);

        if (tile_count == 0)
        {
            egui_font_std_release_access(&font_access);
            return;
        }

        (void)tile_count;
    }

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
                    const text_transform_glyph_t *g = &s_tile_glyphs[hint];
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
                                    uint16_t sa00, sa01, sa10, sa11;
                                    batch_extract_glyph_alpha_4_rb(g_data, g->row_bytes, (rotatedX >> 15) - g_x, (rotatedY >> 15) - g_y, &sa00, &sa01, &sa10,
                                                                   &sa11);

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
                                    uint16_t sa00, sa01, sa10, sa11;
                                    batch_extract_glyph_alpha(g_data, g->box_w, (rotatedX >> 15) - g_x, (rotatedY >> 15) - g_y, bpp, &sa00, &sa01, &sa10,
                                                             &sa11);

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
                                    uint16_t sa00, sa01, sa10, sa11;
                                    batch_extract_glyph_alpha_4_rb(g->data, g->row_bytes, (rotatedX >> 15) - g->x, (rotatedY >> 15) - g->y, &sa00, &sa01,
                                                                   &sa10, &sa11);

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
                                    uint16_t sa00, sa01, sa10, sa11;
                                    batch_extract_glyph_alpha(g->data, g->box_w, (rotatedX >> 15) - g->x, (rotatedY >> 15) - g->y, bpp, &sa00, &sa01, &sa10,
                                                             &sa11);

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
                    int line0 = sample_tile_find_pair_line(s_tile_lines, tile_line_count, sx, sy, hint_line);
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
                        sample_tile_alpha_pair_from_line(s_tile_glyphs, &s_tile_lines[line0], sx, sy, bpp, &a00, &a01, &hint00, &hint01);
                    }

                    /* Early exit: preserve existing whitespace skip behavior based on a00 only */
                    if (a00 == 0 && hint00 < 0)
                    {
                        hint = -1;
                        hint_line = line0;
                        goto text_next_pixel;
                    }

                    line1 = sample_tile_find_pair_line(s_tile_lines, tile_line_count, sx, sy + 1, line0);
                    if (line1 >= 0)
                    {
                        sample_tile_alpha_pair_from_line(s_tile_glyphs, &s_tile_lines[line1], sx, sy + 1, bpp, &a10, &a11, &hint10, &hint11);
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

    egui_font_std_release_access(&font_access);
}

// ============================================================================
// Buffered text transform: packed bpp mask buffer (SCGUI-style)
// ============================================================================

/**
 * Rasterize a single glyph into a packed bpp mask buffer at the original font bpp.
 * Pixel-by-pixel via extract_packed_raw() + write_packed_raw().
 * Rasterization is cached (one-time per string change), so simplicity > speed.
 */
static void rasterize_glyph_to_packed(uint8_t *buf, int buf_w, int buf_h, int dst_x, int dst_y, const uint8_t *glyph_data, int box_w, int box_h, uint8_t bpp)
{
    if (bpp == 4 && dst_x >= 0 && dst_y >= 0 && dst_x + box_w <= buf_w && dst_y + box_h <= buf_h)
    {
        int src_rb = (box_w + 1) >> 1;
        int dst_rb = (buf_w + 1) >> 1;
        int dst_byte = dst_x >> 1;

        if ((dst_x & 1) == 0)
        {
            for (int row = 0; row < box_h; row++)
            {
                const uint8_t *src = glyph_data + row * src_rb;
                uint8_t *dst = buf + (dst_y + row) * dst_rb + dst_byte;

                for (int i = 0; i < src_rb; i++)
                {
                    dst[i] |= src[i];
                }
            }
            return;
        }
        else
        {
            int full_bytes = box_w >> 1;
            int has_tail = box_w & 1;

            for (int row = 0; row < box_h; row++)
            {
                const uint8_t *src = glyph_data + row * src_rb;
                uint8_t *dst = buf + (dst_y + row) * dst_rb + dst_byte;

                for (int i = 0; i < full_bytes; i++)
                {
                    uint8_t packed = src[i];

                    dst[i] |= (uint8_t)((packed & 0x0F) << 4);
                    dst[i + 1] |= (uint8_t)((packed >> 4) & 0x0F);
                }

                if (has_tail)
                {
                    dst[full_bytes] |= (uint8_t)((src[full_bytes] & 0x0F) << 4);
                }
            }
            return;
        }
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
            if (px < 0 || px >= buf_w)
            {
                continue;
            }
            uint8_t raw = extract_packed_raw(glyph_data, box_w, col, row, bpp);
            if (raw)
            {
                write_packed_raw(buf, buf_w, px, py, raw, bpp);
            }
        }
    }
}

static void rasterize_layout_to_packed(const egui_font_std_info_t *font_info, const text_transform_layout_glyph_t *glyphs, int glyph_count, uint8_t *packed_buf,
                                       int buf_w, int buf_h, uint8_t bpp)
{
    for (int i = 0; i < glyph_count; i++)
    {
        const text_transform_layout_glyph_t *glyph = &glyphs[i];
        const uint8_t *p_pixel = font_info->pixel_buffer + glyph->pixel_idx;

        rasterize_glyph_to_packed(packed_buf, buf_w, buf_h, glyph->x, glyph->y, p_pixel, glyph->box_w, glyph->box_h, bpp);
    }
}

static uint16_t g_alpha4_expand_pair_table[256];
static uint8_t g_alpha4_expand_pair_table_ready = 0;

static void ensure_alpha4_expand_pair_table(void)
{
    if (g_alpha4_expand_pair_table_ready)
    {
        return;
    }

    for (int i = 0; i < 256; i++)
    {
        g_alpha4_expand_pair_table[i] = (uint16_t)(egui_alpha_change_table_4[i & 0x0F] | (egui_alpha_change_table_4[(i >> 4) & 0x0F] << 8));
    }

    g_alpha4_expand_pair_table_ready = 1;
}

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
            uint16_t pair;
            uint8_t alpha0;
            uint8_t alpha1;

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

            pair = g_alpha4_expand_pair_table[packed];
            alpha0 = (uint8_t)(pair & 0xFF);
            alpha1 = (uint8_t)(pair >> 8);

            if (alpha0 > dst[0])
            {
                dst[0] = alpha0;
            }
            if (alpha1 > dst[1])
            {
                dst[1] = alpha1;
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

static void rasterize_glyph_to_alpha8(uint8_t *buf, int buf_w, int buf_h, int dst_x, int dst_y, const uint8_t *glyph_data, int box_w, int box_h, uint8_t bpp)
{
    if (bpp == 4)
    {
        if (dst_x >= 0 && dst_y >= 0 && (dst_x + box_w) <= buf_w && (dst_y + box_h) <= buf_h)
        {
            rasterize_glyph4_to_alpha8_inside(buf, buf_w, dst_x, dst_y, glyph_data, box_w, box_h);
            return;
        }
    }

    for (int row = 0; row < box_h; row++)
    {
        int py = dst_y + row;
        if (py < 0 || py >= buf_h)
        {
            continue;
        }

        uint8_t *dst = buf + py * buf_w;
        for (int col = 0; col < box_w; col++)
        {
            int px = dst_x + col;
            uint8_t alpha;

            if (px < 0 || px >= buf_w)
            {
                continue;
            }

            alpha = (bpp == 8) ? glyph_data[row * box_w + col] : extract_packed_alpha(glyph_data, (uint8_t)box_w, col, row, bpp);
            if (alpha > dst[px])
            {
                dst[px] = alpha;
            }
        }
    }
}

static int text_transform_get_layout_alpha8_bbox(const text_transform_layout_glyph_t *glyphs, int glyph_count, int16_t limit_w, int16_t limit_h, int16_t *src_x0,
                                                 int16_t *src_y0, int16_t *src_w, int16_t *src_h)
{
    int found = 0;
    int16_t bbox_x0 = 0;
    int16_t bbox_y0 = 0;
    int16_t bbox_x1 = 0;
    int16_t bbox_y1 = 0;

    if (glyphs == NULL || glyph_count <= 0 || limit_w <= 0 || limit_h <= 0 || src_x0 == NULL || src_y0 == NULL || src_w == NULL || src_h == NULL)
    {
        return 0;
    }

    for (int i = 0; i < glyph_count; i++)
    {
        int16_t glyph_x0 = glyphs[i].x;
        int16_t glyph_y0 = glyphs[i].y;
        int16_t glyph_x1 = glyphs[i].x + glyphs[i].box_w;
        int16_t glyph_y1 = glyphs[i].y + glyphs[i].box_h;

        if (glyphs[i].box_w == 0 || glyphs[i].box_h == 0)
        {
            continue;
        }

        if (glyph_x0 < 0)
        {
            glyph_x0 = 0;
        }
        if (glyph_y0 < 0)
        {
            glyph_y0 = 0;
        }
        if (glyph_x1 > limit_w)
        {
            glyph_x1 = limit_w;
        }
        if (glyph_y1 > limit_h)
        {
            glyph_y1 = limit_h;
        }

        if (glyph_x0 >= glyph_x1 || glyph_y0 >= glyph_y1)
        {
            continue;
        }

        if (!found)
        {
            bbox_x0 = glyph_x0;
            bbox_y0 = glyph_y0;
            bbox_x1 = glyph_x1;
            bbox_y1 = glyph_y1;
            found = 1;
        }
        else
        {
            if (glyph_x0 < bbox_x0)
            {
                bbox_x0 = glyph_x0;
            }
            if (glyph_y0 < bbox_y0)
            {
                bbox_y0 = glyph_y0;
            }
            if (glyph_x1 > bbox_x1)
            {
                bbox_x1 = glyph_x1;
            }
            if (glyph_y1 > bbox_y1)
            {
                bbox_y1 = glyph_y1;
            }
        }
    }

    if (!found)
    {
        return 0;
    }

    if (bbox_x0 > 0)
    {
        bbox_x0--;
    }
    if (bbox_y0 > 0)
    {
        bbox_y0--;
    }
    if (bbox_x1 < limit_w)
    {
        bbox_x1++;
    }
    if (bbox_y1 < limit_h)
    {
        bbox_y1++;
    }

    *src_x0 = bbox_x0;
    *src_y0 = bbox_y0;
    *src_w = bbox_x1 - bbox_x0;
    *src_h = bbox_y1 - bbox_y0;
    return (*src_w > 0 && *src_h > 0);
}

static void rasterize_layout_to_alpha8_offset(const egui_font_std_info_t *font_info, const text_transform_layout_glyph_t *glyphs, int glyph_count, uint8_t *alpha8_buf,
                                              int buf_w, int buf_h, uint8_t bpp, int16_t src_x0, int16_t src_y0)
{
    if (bpp == 4)
    {
        ensure_alpha4_expand_pair_table();

        for (int i = 0; i < glyph_count; i++)
        {
            const text_transform_layout_glyph_t *glyph = &glyphs[i];
            const uint8_t *p_pixel = font_info->pixel_buffer + glyph->pixel_idx;

            rasterize_glyph4_to_alpha8_inside(alpha8_buf, buf_w, glyph->x - src_x0, glyph->y - src_y0, p_pixel, glyph->box_w, glyph->box_h);
        }
        return;
    }

    for (int i = 0; i < glyph_count; i++)
    {
        const text_transform_layout_glyph_t *glyph = &glyphs[i];
        const uint8_t *p_pixel = font_info->pixel_buffer + glyph->pixel_idx;

        rasterize_glyph_to_alpha8(alpha8_buf, buf_w, buf_h, glyph->x - src_x0, glyph->y - src_y0, p_pixel, glyph->box_w, glyph->box_h, bpp);
    }
}

static int text_transform_ensure_visible_alpha8_capacity(int needed, uint8_t **buf, int *capacity)
{
    uint8_t *new_buf;

    if (*buf != NULL && needed <= *capacity)
    {
        return 0;
    }

    new_buf = (uint8_t *)egui_malloc(needed);
    if (new_buf == NULL)
    {
        return -1;
    }

    if (*buf != NULL)
    {
        egui_free(*buf);
    }

    *buf = new_buf;
    *capacity = needed;
    return 0;
}

static int text_transform_draw_alpha8_buffer(const text_transform_ctx_t *ctx, egui_dim_t x, egui_dim_t y, egui_color_t color, const uint8_t *alpha8_buf,
                                             int16_t src_x0, int16_t src_y0, int buf_w, int buf_h)
{
    int row_color_only_mode = 0;
    int32_t row_start_offset_x;
    int32_t row_start_offset_y;
    int32_t Cx_base;
    int32_t Cy_base;
    int32_t buf_src_hi_x_q15;
    int32_t buf_src_hi_y_q15;
    int32_t buf_int_max_x_q15;
    int32_t buf_int_max_y_q15;
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
    uint32_t fg_rb_g;
#endif

    if (ctx == NULL || alpha8_buf == NULL || buf_w <= 0 || buf_h <= 0)
    {
        return 0;
    }

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

    row_start_offset_x = ctx->inv_m00 * ((int32_t)ctx->draw_x0 - x);
    row_start_offset_y = ctx->inv_m10 * ((int32_t)ctx->draw_x0 - x);
    Cx_base = ctx->Cx_base - ((int32_t)src_x0 << 15);
    Cy_base = ctx->Cy_base - ((int32_t)src_y0 << 15);
    buf_src_hi_x_q15 = (int32_t)buf_w << 15;
    buf_src_hi_y_q15 = (int32_t)buf_h << 15;
    buf_int_max_x_q15 = ((int32_t)buf_w - 2) << 15;
    buf_int_max_y_q15 = ((int32_t)buf_h - 2) << 15;
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
    fg_rb_g = (color.full | ((uint32_t)color.full << 16)) & 0x07E0F81FUL;
#endif

    for (int32_t dy = ctx->draw_y0; dy < ctx->draw_y1; dy++)
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
        egui_color_int_t *dst_row = &ctx->pfb[(dy - ctx->pfb_oy) * ctx->pfb_w + (ctx->draw_x0 - ctx->pfb_ox)];
        int entered = 0;
        int32_t dx = ctx->draw_x0;

        if (rotatedX < 0 || rotatedX >= buf_src_hi_x_q15 || rotatedY < 0 || rotatedY >= buf_src_hi_y_q15)
        {
            int32_t skip = transform_scanline_skip(rotatedX, rotatedY, ctx->inv_m00, ctx->inv_m10, 0, buf_src_hi_x_q15, 0, buf_src_hi_y_q15,
                                                   ctx->draw_x1 - ctx->draw_x0);
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
                entered = 1;
                int32_t sir_count = ctx->draw_x1 - dx;

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
                    for (int32_t i = 0; i < sir_count; i++)
                    {
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
                            rotatedX += ctx->inv_m00;
                            rotatedY += ctx->inv_m10;
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

                        rotatedX += ctx->inv_m00;
                        rotatedY += ctx->inv_m10;
                        dst_row++;
                    }
                }
                else
                {
                    for (int32_t i = 0; i < sir_count; i++)
                    {
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
                            rotatedX += ctx->inv_m00;
                            rotatedY += ctx->inv_m10;
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
                                uint32_t result = (bg_rb_g + ((row_fg_rb_g - bg_rb_g) * ((uint32_t)final_alpha >> 3) >> 5)) & 0x07E0F81FUL;
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
                        if (ctx->canvas_alpha == EGUI_ALPHA_100)
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
                                                          const egui_font_std_info_t *font_info,
                                                          const text_transform_layout_glyph_t *const *glyphs, int glyph_count, int16_t src_x0, int16_t src_y0,
                                                          int16_t src_x1, int16_t src_y1, int glyphs_overlap)
{
    static uint8_t *s_visible_alpha8_buf = NULL;
    static int s_visible_alpha8_capacity = 0;
    const uint8_t *pixel_buffer;
    int buf_w;
    int buf_h;
    int buf_size;
    uint8_t *alpha8_buf;

    if (ctx == NULL || font_info == NULL || glyphs == NULL || glyph_count <= 0)
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
    if (text_transform_ensure_visible_alpha8_capacity(buf_size, &s_visible_alpha8_buf, &s_visible_alpha8_capacity) != 0)
    {
        return 0;
    }

    alpha8_buf = s_visible_alpha8_buf;
    memset(alpha8_buf, 0, buf_size);
    ensure_alpha4_expand_pair_table();
    pixel_buffer = font_info->pixel_buffer;

    if (glyphs_overlap)
    {
        for (int i = 0; i < glyph_count; i++)
        {
            const text_transform_layout_glyph_t *glyph = glyphs[i];

            rasterize_glyph4_to_alpha8_inside(alpha8_buf, buf_w, glyph->x - src_x0, glyph->y - src_y0, pixel_buffer + glyph->pixel_idx, glyph->box_w,
                                              glyph->box_h);
        }
    }
    else
    {
        for (int i = 0; i < glyph_count; i++)
        {
            const text_transform_layout_glyph_t *glyph = glyphs[i];

            rasterize_glyph4_to_alpha8_inside_overwrite(alpha8_buf, buf_w, glyph->x - src_x0, glyph->y - src_y0, pixel_buffer + glyph->pixel_idx,
                                                        glyph->box_w, glyph->box_h);
        }
    }

    return text_transform_draw_alpha8_buffer(ctx, x, y, color, alpha8_buf, src_x0, src_y0, buf_w, buf_h);
}

/**
 * Rasterize multi-line text string to a packed bpp mask buffer.
 * Keeps original font bpp (1/2/4/8) – no expansion to 8bpp.
 * Returns 0 on success, -1 on failure.
 */
static int rasterize_text_to_packed(const egui_font_std_info_t *font_info, const char *string, uint8_t *packed_buf, int buf_w, int buf_h, uint8_t bpp,
                                    egui_dim_t line_space)
{
    if (!font_info)
    {
        return -1;
    }

    int buf_bytes = packed_row_bytes(buf_w, bpp) * buf_h;
    memset(packed_buf, 0, buf_bytes);

    const char *s = string;
    int cursor_x = 0;
    int cursor_y = 0;
    uint32_t utf8_code;

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
            cursor_x = 0;
            cursor_y += font_info->height + line_space;
            continue;
        }

        int char_bytes = egui_font_get_utf8_code_fast(s, &utf8_code);
        s += char_bytes;

        int found = egui_font_std_find_code_index(font_info, utf8_code);

        if (found < 0)
        {
            cursor_x += font_info->height >> 1;
            continue;
        }

        const egui_font_std_char_descriptor_t *desc = &font_info->char_array[found];
        if (desc->size == 0)
        {
            cursor_x += font_info->height >> 1;
            continue;
        }

        const uint8_t *p_pixel = font_info->pixel_buffer + desc->idx;
        int glyph_x = cursor_x + desc->off_x;
        int glyph_y = cursor_y + desc->off_y;

        rasterize_glyph_to_packed(packed_buf, buf_w, buf_h, glyph_x, glyph_y, p_pixel, desc->box_w, desc->box_h, bpp);

        cursor_x += desc->adv;
    }

    return 0;
}

static int rasterize_text_to_alpha8(const egui_font_std_info_t *font_info, const char *string, uint8_t *alpha8_buf, int buf_w, int buf_h, uint8_t bpp,
                                    egui_dim_t line_space)
{
    if (!font_info)
    {
        return -1;
    }

    memset(alpha8_buf, 0, buf_w * buf_h);

    if (bpp == 4)
    {
        ensure_alpha4_expand_pair_table();
    }

    {
        const char *s = string;
        int cursor_x = 0;
        int cursor_y = 0;
        uint32_t utf8_code;

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
                cursor_x = 0;
                cursor_y += font_info->height + line_space;
                continue;
            }

            {
                int char_bytes = egui_font_get_utf8_code_fast(s, &utf8_code);
                int found;
                const egui_font_std_char_descriptor_t *desc;
                const uint8_t *p_pixel;
                int glyph_x;
                int glyph_y;

                s += char_bytes;
                found = egui_font_std_find_code_index(font_info, utf8_code);

                if (found < 0)
                {
                    cursor_x += font_info->height >> 1;
                    continue;
                }

                desc = &font_info->char_array[found];
                if (desc->size == 0)
                {
                    cursor_x += font_info->height >> 1;
                    continue;
                }

                p_pixel = font_info->pixel_buffer + desc->idx;
                glyph_x = cursor_x + desc->off_x;
                glyph_y = cursor_y + desc->off_y;

                rasterize_glyph_to_alpha8(alpha8_buf, buf_w, buf_h, glyph_x, glyph_y, p_pixel, desc->box_w, desc->box_h, bpp);
                cursor_x += desc->adv;
            }
        }
    }

    return 0;
}

/**
 * Draw rotated/scaled text using a pre-rasterized packed bpp mask buffer (SCGUI-style).
 *
 * Keeps the original font bpp format (1/2/4/8) in the mask buffer, saving memory:
 *   4bpp → 50% of 8bpp, 2bpp → 25%, 1bpp → 12.5%.
 * Buffer = packed_row_bytes(text_w, bpp) * text_h, allocated once via egui_malloc.
 * Sampling uses read_packed_mask_alpha() with bit extraction + lookup table conversion.
 *
 * @param font      Font to use
 * @param string    Multi-line text (supports \n)
 * @param x         Center X position in view coordinates
 * @param y         Center Y position in view coordinates
 * @param angle_deg Rotation angle in degrees (0-360)
 * @param scale_q8  Scale factor in Q8 format (256 = 1.0x)
 * @param color     Foreground color
 * @param alpha     Global alpha (EGUI_ALPHA_100 for fully opaque)
 */

/**
 * Batch read 4 bilinear alpha samples from packed mask buffer.
 * One switch per pixel instead of 4; shares row offset and bit position calculations.
 * Requires all 4 sample coordinates to be in bounds.
 */
static inline void batch_bilinear_packed_alpha(const uint8_t *buf, int rb, int sx, int sy, uint8_t bpp, uint16_t *a00, uint16_t *a01, uint16_t *a10,
                                               uint16_t *a11)
{
    int base0 = sy * rb;
    int base1 = base0 + rb;
    switch (bpp)
    {
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
    default: /* 8bpp */
        *a00 = buf[base0 + sx];
        *a01 = buf[base0 + sx + 1];
        *a10 = buf[base1 + sx];
        *a11 = buf[base1 + sx + 1];
        break;
    }
}

static inline int batch_extract_packed_raw_4_trivial(const uint8_t *buf, int rb, int sx, int sy, uint8_t *pixel_alpha, uint8_t *a00, uint8_t *a01,
                                                     uint8_t *a10, uint8_t *a11)
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

void egui_canvas_draw_text_transform_buffered(const egui_font_t *font, const void *string, egui_dim_t x, egui_dim_t y, int16_t angle_deg, int16_t scale_q8,
                                              egui_color_t color, egui_alpha_t alpha)
{
    if (!font || !string || !font->res)
    {
        return;
    }

    if (text_transform_try_draw_axis_aligned(font, string, x, y, angle_deg, scale_q8, color, alpha))
    {
        return;
    }

    egui_font_std_info_t *font_info = (egui_font_std_info_t *)font->res;
    uint8_t bpp = font_info->font_bit_mode;

    /* Static cache: rasterize once, reuse across PFB tiles */
    static uint8_t *s_packed_buf = NULL;
    static uint8_t s_packed_buf_bpp = 0;
    static const egui_font_t *s_cached_font = NULL;
    static const void *s_cached_string = NULL;
    static int16_t s_cached_w = 0, s_cached_h = 0;
    static int16_t s_packed_src_x0 = 0, s_packed_src_y0 = 0;
    static int16_t s_packed_src_w = 0, s_packed_src_h = 0;

    int16_t text_w = s_cached_w, text_h = s_cached_h;
    if (font != s_cached_font || string != s_cached_string)
    {
        /* Measure text dimensions */
        egui_dim_t tw = 0, th = 0;
        font->api->get_str_size(font, string, 1, 0, &tw, &th);
        text_w = tw;
        text_h = th;
    }

    text_transform_ctx_t ctx;
    if (text_transform_prepare(text_w, text_h, x, y, angle_deg, scale_q8, alpha, &ctx) != 0)
    {
        return;
    }

    if (font != s_cached_font || string != s_cached_string || text_w != s_cached_w || text_h != s_cached_h)
    {
        if (s_packed_buf)
        {
            egui_free(s_packed_buf);
            s_packed_buf = NULL;
        }

        int buf_size = 0;
        uint8_t raster_bpp = bpp;
        int16_t packed_src_x0 = 0;
        int16_t packed_src_y0 = 0;
        int16_t packed_src_w = text_w;
        int16_t packed_src_h = text_h;
        egui_font_std_access_t font_access;
        const egui_font_std_info_t *raster_font_info = font_info;
        const text_transform_layout_glyph_t *layout_glyphs = NULL;
        const text_transform_layout_line_t *layout_lines = NULL;
        int layout_count = 0;
        int layout_line_count = 0;

        if (egui_font_std_prepare_access(font_info, &font_access) != 0)
        {
            s_cached_font = NULL;
            s_cached_string = NULL;
            s_packed_buf_bpp = 0;
            return;
        }

        raster_font_info = &font_access.info;

        if (text_transform_prepare_layout(font, raster_font_info, (const char *)string, 0, &layout_glyphs, &layout_count, &layout_lines, &layout_line_count) == 0 &&
            bpp == 4)
        {
            text_transform_get_layout_alpha8_bbox(layout_glyphs, layout_count, text_w, text_h, &packed_src_x0, &packed_src_y0, &packed_src_w, &packed_src_h);
        }

        if (bpp == 4)
        {
            buf_size = packed_src_w * packed_src_h;
            s_packed_buf = (uint8_t *)egui_malloc(buf_size);
            if (s_packed_buf != NULL)
            {
                raster_bpp = 8;
            }
            else
            {
                packed_src_x0 = 0;
                packed_src_y0 = 0;
                packed_src_w = text_w;
                packed_src_h = text_h;
            }
        }

        if (s_packed_buf == NULL)
        {
            buf_size = packed_row_bytes(text_w, bpp) * text_h;
            s_packed_buf = (uint8_t *)egui_malloc(buf_size);
            raster_bpp = bpp;
        }

        if (!s_packed_buf)
        {
            egui_font_std_release_access(&font_access);
            s_cached_font = NULL;
            s_cached_string = NULL;
            s_packed_buf_bpp = 0;
            return;
        }

        EGUI_UNUSED(layout_lines);
        EGUI_UNUSED(layout_line_count);
        if (layout_glyphs != NULL)
        {
            if (raster_bpp == 8 && bpp == 4)
            {
                memset(s_packed_buf, 0, buf_size);
                rasterize_layout_to_alpha8_offset(raster_font_info, layout_glyphs, layout_count, s_packed_buf, packed_src_w, packed_src_h, bpp, packed_src_x0,
                                                  packed_src_y0);
            }
            else
            {
                memset(s_packed_buf, 0, buf_size);
                rasterize_layout_to_packed(raster_font_info, layout_glyphs, layout_count, s_packed_buf, text_w, text_h, bpp);
            }
        }
        else if (((raster_bpp == 8 && bpp == 4) && rasterize_text_to_alpha8(raster_font_info, (const char *)string, s_packed_buf, text_w, text_h, bpp, 0) != 0) ||
                 ((raster_bpp != 8 || bpp != 4) && rasterize_text_to_packed(raster_font_info, (const char *)string, s_packed_buf, text_w, text_h, bpp, 0) != 0))
        {
            egui_font_std_release_access(&font_access);
            egui_free(s_packed_buf);
            s_packed_buf = NULL;
            s_cached_font = NULL;
            s_cached_string = NULL;
            s_packed_buf_bpp = 0;
            return;
        }

        egui_font_std_release_access(&font_access);

        s_cached_font = font;
        s_cached_string = string;
        s_cached_w = text_w;
        s_cached_h = text_h;
        s_packed_buf_bpp = raster_bpp;
        s_packed_src_x0 = packed_src_x0;
        s_packed_src_y0 = packed_src_y0;
        s_packed_src_w = packed_src_w;
        s_packed_src_h = packed_src_h;
    }

    uint8_t *packed_buf = s_packed_buf;
    uint8_t packed_bpp = s_packed_buf_bpp;
    int16_t src_x0 = s_packed_src_x0;
    int16_t src_y0 = s_packed_src_y0;
    int16_t src_w = s_packed_src_w;
    int16_t src_h = s_packed_src_h;
    int packed_rb = packed_row_bytes(src_w, packed_bpp);

    if (packed_bpp == 8)
    {
        if (text_transform_draw_alpha8_buffer(&ctx, x, y, color, packed_buf, src_x0, src_y0, src_w, src_h))
        {
            return;
        }
    }

    /* Hoist loop-invariant row-start offset computed from draw_x0 */
    int32_t row_start_offset_x = ctx.inv_m00 * ((int32_t)ctx.draw_x0 - x);
    int32_t row_start_offset_y = ctx.inv_m10 * ((int32_t)ctx.draw_x0 - x);

    /* Source bounds in Q15 for scanline skip */
    int32_t buf_src_lo_x_q15 = 0;
    int32_t buf_src_hi_x_q15 = ((int32_t)src_w - 1) << 15;
    int32_t buf_src_lo_y_q15 = 0;
    int32_t buf_src_hi_y_q15 = ((int32_t)src_h - 1) << 15;

    /* Interior bounds in Q15 for SIR (Scanline Interior Range).
     * Interior = bilinear samples (sx..sx+1, sy..sy+1) all in-bounds. */
    int32_t buf_int_max_x_q15 = ((int32_t)(src_w - 2)) << 15;
    int32_t buf_int_max_y_q15 = ((int32_t)(src_h - 2)) << 15;

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
        int32_t dx = ctx.draw_x0;
        int entered = 0;

        /* Skip left dead zone */
        if (rotatedX < buf_src_lo_x_q15 || rotatedX >= buf_src_hi_x_q15 || rotatedY < buf_src_lo_y_q15 || rotatedY >= buf_src_hi_y_q15)
        {
            int32_t skip = transform_scanline_skip(rotatedX, rotatedY, ctx.inv_m00, ctx.inv_m10, buf_src_lo_x_q15, buf_src_hi_x_q15, buf_src_lo_y_q15,
                                                   buf_src_hi_y_q15, ctx.draw_x1 - ctx.draw_x0);
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

            if (sx >= 0 && sx < src_w - 1 && sy >= 0 && sy < src_h - 1)
            {
                entered = 1;

                /* SIR: compute how many consecutive pixels remain interior */
                int32_t sir_count = ctx.draw_x1 - dx;
                if (ctx.inv_m00 > 0)
                {
                    int32_t n = (buf_int_max_x_q15 - rotatedX) / ctx.inv_m00;
                    if (n < sir_count)
                        sir_count = n;
                }
                else if (ctx.inv_m00 < 0)
                {
                    int32_t n = rotatedX / (-ctx.inv_m00);
                    if (n < sir_count)
                        sir_count = n;
                }
                if (ctx.inv_m10 > 0)
                {
                    int32_t n = (buf_int_max_y_q15 - rotatedY) / ctx.inv_m10;
                    if (n < sir_count)
                        sir_count = n;
                }
                else if (ctx.inv_m10 < 0)
                {
                    int32_t n = rotatedY / (-ctx.inv_m10);
                    if (n < sir_count)
                        sir_count = n;
                }
                if (sir_count < 1)
                    sir_count = 1;

                /* Tight interior loop: no boundary checks */
                if (ctx.canvas_alpha == EGUI_ALPHA_100)
                {
                    if (packed_bpp == 4)
                    {
                        if (mask_requires_point)
                        {
                            for (int32_t i = 0; i < sir_count; i++)
                            {
                                uint8_t pixel_alpha;
                                uint8_t a00, a01, a10, a11;
                                int trivial = batch_extract_packed_raw_4_trivial(packed_buf, packed_rb, rotatedX >> 15, rotatedY >> 15, &pixel_alpha, &a00,
                                                                                 &a01, &a10, &a11);

                                if (!trivial)
                                {
                                    pixel_alpha = bilinear_alpha_from_raw_4(a00, a01, a10, a11, (rotatedX >> 7) & 0xFF, (rotatedY >> 7) & 0xFF);
                                }

                                if (pixel_alpha > 0)
                                {
                                    transform_apply_mask_and_blend(ctx.mask, (egui_dim_t)(dx + i), (egui_dim_t)dy, row_color, pixel_alpha, ctx.canvas_alpha,
                                                                   dst_row);
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
                                uint8_t pixel_alpha;
                                uint8_t a00, a01, a10, a11;
                                int trivial = batch_extract_packed_raw_4_trivial(packed_buf, packed_rb, rotatedX >> 15, rotatedY >> 15, &pixel_alpha, &a00,
                                                                                 &a01, &a10, &a11);

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

                                rotatedX += ctx.inv_m00;
                                rotatedY += ctx.inv_m10;
                                dst_row++;
                            }
                        }
                    }
                    else
                    {
                        for (int32_t i = 0; i < sir_count; i++)
                        {
                            uint16_t a00, a01, a10, a11;
                            batch_bilinear_packed_alpha(packed_buf, packed_rb, rotatedX >> 15, rotatedY >> 15, packed_bpp, &a00, &a01, &a10, &a11);

                            if ((a00 | a01 | a10 | a11) != 0)
                            {
                                uint8_t fx = (rotatedX >> 7) & 0xFF;
                                uint8_t fy = (rotatedY >> 7) & 0xFF;
                                uint16_t ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
                                uint16_t ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
                                uint8_t pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));

                                if (mask_requires_point)
                                {
                                    transform_apply_mask_and_blend(ctx.mask, (egui_dim_t)(dx + i), (egui_dim_t)dy, row_color, pixel_alpha, ctx.canvas_alpha,
                                                                   dst_row);
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
                    if (packed_bpp == 4)
                    {
                        if (mask_requires_point)
                        {
                            for (int32_t i = 0; i < sir_count; i++)
                            {
                                uint8_t pixel_alpha;
                                uint8_t a00, a01, a10, a11;
                                int trivial = batch_extract_packed_raw_4_trivial(packed_buf, packed_rb, rotatedX >> 15, rotatedY >> 15, &pixel_alpha, &a00,
                                                                                 &a01, &a10, &a11);

                                if (!trivial)
                                {
                                    pixel_alpha = bilinear_alpha_from_raw_4(a00, a01, a10, a11, (rotatedX >> 7) & 0xFF, (rotatedY >> 7) & 0xFF);
                                }

                                if (pixel_alpha > 0)
                                {
                                    transform_apply_mask_and_blend(ctx.mask, (egui_dim_t)(dx + i), (egui_dim_t)dy, row_color, pixel_alpha, ctx.canvas_alpha,
                                                                   dst_row);
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
                                uint8_t pixel_alpha;
                                uint8_t a00, a01, a10, a11;
                                int trivial = batch_extract_packed_raw_4_trivial(packed_buf, packed_rb, rotatedX >> 15, rotatedY >> 15, &pixel_alpha, &a00,
                                                                                 &a01, &a10, &a11);

                                if (!trivial)
                                {
                                    pixel_alpha = bilinear_alpha_from_raw_4(a00, a01, a10, a11, (rotatedX >> 7) & 0xFF, (rotatedY >> 7) & 0xFF);
                                }

                                if (pixel_alpha > 0)
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

                                rotatedX += ctx.inv_m00;
                                rotatedY += ctx.inv_m10;
                                dst_row++;
                            }
                        }
                    }
                    else
                    {
                        for (int32_t i = 0; i < sir_count; i++)
                        {
                            uint16_t a00, a01, a10, a11;
                            batch_bilinear_packed_alpha(packed_buf, packed_rb, rotatedX >> 15, rotatedY >> 15, packed_bpp, &a00, &a01, &a10, &a11);

                            if ((a00 | a01 | a10 | a11) != 0)
                            {
                                uint8_t fx = (rotatedX >> 7) & 0xFF;
                                uint8_t fy = (rotatedY >> 7) & 0xFF;
                                uint16_t ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
                                uint16_t ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
                                uint8_t pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));

                                if (pixel_alpha > 0)
                                {
                                    if (mask_requires_point)
                                    {
                                        transform_apply_mask_and_blend(ctx.mask, (egui_dim_t)(dx + i), (egui_dim_t)dy, row_color, pixel_alpha, ctx.canvas_alpha,
                                                                       dst_row);
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
            else if (sx >= -1 && sx < src_w && sy >= -1 && sy < src_h)
            {
                /* Edge: clamp out-of-bounds to 0 alpha */
                uint8_t fx = (rotatedX >> 7) & 0xFF;
                uint8_t fy = (rotatedY >> 7) & 0xFF;
                uint16_t a00;
                uint16_t a01;
                uint16_t a10;
                uint16_t a11;

                if (packed_bpp == 4)
                {
#define PACKED_FETCH_ALPHA_4(px, py) (((px) >= 0 && (px) < src_w && (py) >= 0 && (py) < src_h) ? read_packed_mask_alpha_4_rb(packed_buf, packed_rb, (px), (py)) : 0)
                    a00 = PACKED_FETCH_ALPHA_4(sx, sy);
                    a01 = PACKED_FETCH_ALPHA_4(sx + 1, sy);
                    a10 = PACKED_FETCH_ALPHA_4(sx, sy + 1);
                    a11 = PACKED_FETCH_ALPHA_4(sx + 1, sy + 1);
#undef PACKED_FETCH_ALPHA_4
                }
                else
                {
#define PACKED_FETCH_ALPHA(px, py) (((px) >= 0 && (px) < src_w && (py) >= 0 && (py) < src_h) ? read_packed_mask_alpha(packed_buf, src_w, (px), (py), packed_bpp) : 0)
                    a00 = PACKED_FETCH_ALPHA(sx, sy);
                    a01 = PACKED_FETCH_ALPHA(sx + 1, sy);
                    a10 = PACKED_FETCH_ALPHA(sx, sy + 1);
                    a11 = PACKED_FETCH_ALPHA(sx + 1, sy + 1);
#undef PACKED_FETCH_ALPHA
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
                    else
                    {
                        egui_alpha_t final_alpha = egui_color_alpha_mix(ctx.canvas_alpha, pixel_alpha);
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
                entered = 1;
            }
            else if (entered)
            {
                break;
            }

            rotatedX += ctx.inv_m00;
            rotatedY += ctx.inv_m10;
            dst_row++;
        }

        ctx.Cx_base += ctx.inv_m01;
        ctx.Cy_base += ctx.inv_m11;
    }
}
