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

    /* Source pixel data */
    const uint16_t *data;
    const uint8_t *alpha_buf;

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    void *ext_data_buf = NULL;
    void *ext_alpha_buf = NULL;
    if (info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        uint32_t data_size = (uint32_t)src_w * src_h * sizeof(uint16_t);
        ext_data_buf = egui_malloc(data_size);
        if (ext_data_buf == NULL)
        {
            return;
        }
        egui_api_load_external_resource(ext_data_buf, (egui_uintptr_t)(info->data_buf), 0, data_size);
        data = (const uint16_t *)ext_data_buf;

        if (info->alpha_buf != NULL && info->alpha_type == EGUI_IMAGE_ALPHA_TYPE_8)
        {
            uint32_t alpha_size = (uint32_t)src_w * src_h;
            ext_alpha_buf = egui_malloc(alpha_size);
            if (ext_alpha_buf == NULL)
            {
                egui_free(ext_data_buf);
                return;
            }
            egui_api_load_external_resource(ext_alpha_buf, (egui_uintptr_t)(info->alpha_buf), 0, alpha_size);
            alpha_buf = (const uint8_t *)ext_alpha_buf;
        }
        else
        {
            alpha_buf = NULL;
        }
    }
    else
#endif
    {
        data = (const uint16_t *)info->data_buf;
        alpha_buf = (const uint8_t *)info->alpha_buf;
    }
    int has_alpha8 = (alpha_buf != NULL && info->alpha_type == EGUI_IMAGE_ALPHA_TYPE_8);
    int opaque_mode = (!has_alpha8 && canvas_alpha == EGUI_ALPHA_100);

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
                if (opaque_mode)
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
                        *dst_row = bilinear_rgb565_packed(d00, d01, d10, d11, fx, fy);
                        rotatedX += inv_m00;
                        rotatedY += inv_m10;
                        dst_row++;
                    }
                }
                else if (has_alpha8 && canvas_alpha == EGUI_ALPHA_100)
                {
                    /* Alpha8 image with fully opaque canvas: read alpha first
                     * to skip color bilinear for transparent pixels */
                    for (int32_t i = 0; i < sir_count; i++)
                    {
                        int32_t offs = (rotatedY >> 15) * src_w + (rotatedX >> 15);

                        /* Read alpha samples first for early exit */
                        uint16_t a00 = alpha_buf[offs];
                        uint16_t a01 = alpha_buf[offs + 1];
                        uint16_t a10 = alpha_buf[offs + src_w];
                        uint16_t a11 = alpha_buf[offs + src_w + 1];

                        if ((a00 | a01 | a10 | a11) != 0)
                        {
                            uint8_t fx = (rotatedX >> 7) & 0xFF;
                            uint8_t fy = (rotatedY >> 7) & 0xFF;

                            /* All-opaque shortcut: skip alpha bilinear + blend */
                            if ((a00 & a01 & a10 & a11) == 255)
                            {
                                uint16_t d00 = data[offs];
                                uint16_t d01 = data[offs + 1];
                                uint16_t d10 = data[offs + src_w];
                                uint16_t d11 = data[offs + src_w + 1];
                                *dst_row = bilinear_rgb565_packed(d00, d01, d10, d11, fx, fy);
                            }
                            else
                            {
                                uint16_t d00 = data[offs];
                                uint16_t d01 = data[offs + 1];
                                uint16_t d10 = data[offs + src_w];
                                uint16_t d11 = data[offs + src_w + 1];
                                egui_color_t color;
                                color.full = bilinear_rgb565_packed(d00, d01, d10, d11, fx, fy);

                                uint16_t ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
                                uint16_t ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
                                uint8_t pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));

                                if (pixel_alpha == EGUI_ALPHA_100)
                                {
                                    *dst_row = color.full;
                                }
                                else if (pixel_alpha > 0)
                                {
                                    egui_color_t *back = (egui_color_t *)dst_row;
                                    egui_rgb_mix_ptr(back, &color, back, pixel_alpha);
                                }
                            }
                        }

                        rotatedX += inv_m00;
                        rotatedY += inv_m10;
                        dst_row++;
                    }
                }
                else
                {
                    /* Generic SIR loop: handles alpha8 and/or canvas_alpha != 100 */
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

                        egui_alpha_t pixel_alpha = EGUI_ALPHA_100;
                        if (has_alpha8)
                        {
                            uint16_t a00 = alpha_buf[offs];
                            uint16_t a01 = alpha_buf[offs + 1];
                            uint16_t a10 = alpha_buf[offs + src_w];
                            uint16_t a11 = alpha_buf[offs + src_w + 1];
                            uint16_t ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
                            uint16_t ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
                            pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));
                        }
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

                egui_alpha_t final_alpha = canvas_alpha;

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

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (ext_data_buf != NULL)
    {
        egui_free(ext_data_buf);
    }
    if (ext_alpha_buf != NULL)
    {
        egui_free(ext_alpha_buf);
    }
#endif
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
} text_transform_glyph_t;

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
    {
        int rb = (box_w + 1) >> 1;
        int base0 = ly * rb;
        int base1 = base0 + rb;
        int x0_idx = lx >> 1;
        int x0_bit = (lx & 1) << 2;
        int x1_idx = (lx + 1) >> 1;
        int x1_bit = ((lx + 1) & 1) << 2;
        *a00 = egui_alpha_change_table_4[(data[base0 + x0_idx] >> x0_bit) & 0x0F];
        *a01 = egui_alpha_change_table_4[(data[base0 + x1_idx] >> x1_bit) & 0x0F];
        *a10 = egui_alpha_change_table_4[(data[base1 + x0_idx] >> x0_bit) & 0x0F];
        *a11 = egui_alpha_change_table_4[(data[base1 + x1_idx] >> x1_bit) & 0x0F];
        break;
    }
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
} text_transform_ctx_t;

static int text_transform_prepare(int16_t text_w, int16_t text_h, egui_dim_t x, egui_dim_t y, int16_t angle_deg, int16_t scale_q8, egui_alpha_t alpha,
                                  text_transform_ctx_t *ctx)
{
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

    /* Clip to canvas work region */
    egui_region_t *work = &canvas->base_view_work_region;
    int32_t clip_x0 = work->location.x;
    int32_t clip_y0 = work->location.y;
    int32_t clip_x1 = clip_x0 + work->size.width;
    int32_t clip_y1 = clip_y0 + work->size.height;

    ctx->draw_x0 = (min_bx > clip_x0) ? min_bx : clip_x0;
    ctx->draw_y0 = (min_by > clip_y0) ? min_by : clip_y0;
    ctx->draw_x1 = (max_bx < clip_x1) ? max_bx : clip_x1;
    ctx->draw_y1 = (max_by < clip_y1) ? max_by : clip_y1;

    if (ctx->draw_x0 >= ctx->draw_x1 || ctx->draw_y0 >= ctx->draw_y1)
    {
        return -1;
    }

    /* Inverse affine matrix (Q15) */
    ctx->inv_m00 = (cosA << 8) / scale_q8;
    ctx->inv_m01 = (sinA << 8) / scale_q8;
    ctx->inv_m10 = (-sinA << 8) / scale_q8;
    ctx->inv_m11 = (cosA << 8) / scale_q8;

    /* Center correction for even-sized text area */
    int32_t offx = (text_w & 1) ? 0 : (ctx->inv_m00 + ctx->inv_m01 - 32767) / 2;
    int32_t offy = (text_h & 1) ? 0 : (ctx->inv_m10 + ctx->inv_m11 - 32767) / 2;

    /* PFB write setup */
    ctx->pfb_w = canvas->pfb_region.size.width;
    ctx->pfb_ox = canvas->pfb_location_in_base_view.x;
    ctx->pfb_oy = canvas->pfb_location_in_base_view.y;
    ctx->pfb = canvas->pfb;
    ctx->canvas_alpha = egui_color_alpha_mix(canvas->alpha, alpha);

    /* Row-constant terms for incremental scanning */
    ctx->Cx_base = ctx->inv_m01 * ((int32_t)ctx->draw_y0 - y) + ((int32_t)ctx->cx << 15) + offx;
    ctx->Cy_base = ctx->inv_m11 * ((int32_t)ctx->draw_y0 - y) + ((int32_t)ctx->cy << 15) + offy;

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

/**
 * Walk string and collect only glyphs that overlap the source bounding box [sx0..sx1, sy0..sy1].
 * Returns number of glyphs stored.
 */
static int collect_visible_glyphs(const egui_font_std_info_t *font_info, const char *string, text_transform_glyph_t *glyphs, int max_glyphs, int16_t sx0,
                                  int16_t sy0, int16_t sx1, int16_t sy1, egui_dim_t line_space)
{
    const char *s = string;
    int cursor_x = 0;
    int cursor_y = 0;
    int count = 0;
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
            /* Skip remaining lines below source bbox */
            if (cursor_y > sy1)
            {
                break;
            }
            continue;
        }

        int char_bytes = egui_font_get_utf8_code(s, &utf8_code);
        s += char_bytes;

        /* Binary search for glyph descriptor */
        int first = 0;
        int last = font_info->count - 1;
        int found = -1;
        while (first <= last)
        {
            int mid = (first + last) / 2;
            if (font_info->code_array[mid].code < utf8_code)
            {
                first = mid + 1;
            }
            else if (font_info->code_array[mid].code == utf8_code)
            {
                found = mid;
                break;
            }
            else
            {
                last = mid - 1;
            }
        }

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

        int gx = cursor_x + desc->off_x;
        int gy = cursor_y + desc->off_y;

        /* Check overlap with source bbox */
        if (gx + desc->box_w > sx0 && gx < sx1 && gy + desc->box_h > sy0 && gy < sy1)
        {
            if (count < max_glyphs)
            {
                glyphs[count].data = font_info->pixel_buffer + desc->idx;
                glyphs[count].x = gx;
                glyphs[count].y = gy;
                glyphs[count].box_w = desc->box_w;
                glyphs[count].box_h = desc->box_h;
                count++;
            }
        }

        cursor_x += desc->adv;
    }

    return count;
}

/**
 * Sample text alpha by searching the PFB-local glyph array.
 * With typically <20 glyph entries, linear scan with hint is efficient.
 */
static inline uint8_t sample_tile_alpha(const text_transform_glyph_t *glyphs, int count, int sx, int sy, uint8_t bpp, int *hint)
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
        int ly = sy - g->y;
        if (lx >= 0 && lx < g->box_w && ly >= 0 && ly < g->box_h)
        {
            return extract_packed_alpha(g->data, g->box_w, lx, ly, bpp);
        }
    }

    /* Linear scan; glyphs are x-sorted within each line */
    for (int i = 0; i < count; i++)
    {
        const text_transform_glyph_t *g = &glyphs[i];
        int lx = sx - g->x;
        int ly = sy - g->y;
        if (lx >= 0 && lx < g->box_w && ly >= 0 && ly < g->box_h)
        {
            *hint = i;
            return extract_packed_alpha(g->data, g->box_w, lx, ly, bpp);
        }
    }

    /* No glyph found: signal whitespace to caller for early exit */
    *hint = -1;
    return 0;
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

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    /* For external fonts, load char_array and pixel_buffer into RAM */
    egui_font_std_info_t ext_font_info;
    egui_font_std_char_descriptor_t *ext_char_array = NULL;
    uint8_t *ext_pixel_buffer = NULL;
    if (font_info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        uint32_t char_array_size = font_info->count * sizeof(egui_font_std_char_descriptor_t);
        ext_char_array = (egui_font_std_char_descriptor_t *)egui_malloc(char_array_size);
        if (!ext_char_array)
        {
            return;
        }
        egui_api_load_external_resource(ext_char_array, (egui_uintptr_t)font_info->char_array, 0, char_array_size);

        /* Find total pixel_buffer size from char descriptors */
        uint32_t max_extent = 0;
        for (int i = 0; i < font_info->count; i++)
        {
            uint32_t end = ext_char_array[i].idx + ext_char_array[i].size;
            if (end > max_extent)
            {
                max_extent = end;
            }
        }

        ext_pixel_buffer = (uint8_t *)egui_malloc(max_extent);
        if (!ext_pixel_buffer)
        {
            egui_free(ext_char_array);
            return;
        }
        egui_api_load_external_resource(ext_pixel_buffer, (egui_uintptr_t)font_info->pixel_buffer, 0, max_extent);

        ext_font_info = *font_info;
        ext_font_info.char_array = ext_char_array;
        ext_font_info.pixel_buffer = ext_pixel_buffer;
        ext_font_info.res_type = EGUI_RESOURCE_TYPE_INTERNAL;
        font_info = &ext_font_info;
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
     * Use a static pointer allocated once via egui_malloc to avoid large stack usage. */
    static text_transform_glyph_t *s_tile_glyphs = NULL;
    if (s_tile_glyphs == NULL)
    {
        s_tile_glyphs = (text_transform_glyph_t *)egui_malloc(EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_GLYPHS * sizeof(text_transform_glyph_t));
        if (s_tile_glyphs == NULL)
        {
            return;
        }
    }
    int tile_count = collect_visible_glyphs(font_info, (const char *)string, s_tile_glyphs, EGUI_CONFIG_TEXT_TRANSFORM_TILE_MAX_GLYPHS, src_min_x, src_min_y,
                                            src_max_x, src_max_y, 0);

    if (tile_count == 0)
    {
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        if (ext_char_array)
        {
            egui_free(ext_char_array);
        }
        if (ext_pixel_buffer)
        {
            egui_free(ext_pixel_buffer);
        }
#endif
        return;
    }

    int hint = -1;

    /* Hoist loop-invariant row-start offset computed from draw_x0 */
    int32_t row_start_offset_x = ctx.inv_m00 * ((int32_t)ctx.draw_x0 - x);
    int32_t row_start_offset_y = ctx.inv_m10 * ((int32_t)ctx.draw_x0 - x);

    /* Source bounds in Q15 for scanline skip */
    int32_t text_src_lo_x_q15 = -(1 << 15);
    int32_t text_src_hi_x_q15 = (int32_t)ctx.src_w << 15;
    int32_t text_src_lo_y_q15 = -(1 << 15);
    int32_t text_src_hi_y_q15 = (int32_t)ctx.src_h << 15;

#if (EGUI_CONFIG_COLOR_DEPTH == 16)
    /* Pre-compute foreground color in packed rb_g domain for text blend.
     * Saves 3 ops/pixel (pack) + eliminates 2 branches in egui_rgb_mix_ptr. */
    uint32_t fg_rb_g = (color.full | ((uint32_t)color.full << 16)) & 0x07E0F81FUL;
#endif

    for (int32_t dy = ctx.draw_y0; dy < ctx.draw_y1; dy++)
    {
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
                uint16_t a00, a01, a10, a11;

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

                                    if (pixel_alpha == EGUI_ALPHA_100)
                                    {
                                        *dst_row = color.full;
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
                                        egui_rgb_mix_ptr(back, &color, back, pixel_alpha);
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
                                        egui_alpha_t final_alpha = ((uint16_t)ctx.canvas_alpha * pixel_alpha + 128) >> 8;
                                        if (final_alpha == EGUI_ALPHA_100)
                                        {
                                            *dst_row = color.full;
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
                                            egui_rgb_mix_ptr(back, &color, back, final_alpha);
#endif
                                        }
                                    }
                                }

                                rotatedX += ctx.inv_m00;
                                rotatedY += ctx.inv_m10;
                                dst_row++;
                            }
                        }

                        dx += sir_count - 1;
                        continue;
                    }
                }
                /* Ungrouped slow path: per-sample glyph lookup */
                {
                    uint16_t a00, a01, a10, a11;
                    a00 = sample_tile_alpha(s_tile_glyphs, tile_count, sx, sy, bpp, &hint);
                    /* Early exit: if first sample is 0 and hint didn't find a glyph,
                     * likely in whitespace — check remaining quickly */
                    if (a00 == 0 && hint < 0)
                    {
                        goto text_next_pixel;
                    }
                    a01 = sample_tile_alpha(s_tile_glyphs, tile_count, sx + 1, sy, bpp, &hint);
                    a10 = sample_tile_alpha(s_tile_glyphs, tile_count, sx, sy + 1, bpp, &hint);
                    a11 = sample_tile_alpha(s_tile_glyphs, tile_count, sx + 1, sy + 1, bpp, &hint);

                    uint16_t ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
                    uint16_t ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
                    uint8_t pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));

                    if (pixel_alpha > 0)
                    {
                        if (ctx.canvas_alpha == EGUI_ALPHA_100)
                        {
                            if (pixel_alpha == EGUI_ALPHA_100)
                            {
                                *dst_row = color.full;
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
                        else
                        {
                            egui_alpha_t final_alpha = ((uint16_t)ctx.canvas_alpha * pixel_alpha + 128) >> 8;
                            if (final_alpha == EGUI_ALPHA_100)
                            {
                                *dst_row = color.full;
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
                                egui_rgb_mix_ptr(back, &color, back, final_alpha);
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

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (ext_char_array)
    {
        egui_free(ext_char_array);
    }
    if (ext_pixel_buffer)
    {
        egui_free(ext_pixel_buffer);
    }
#endif
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

/**
 * Rasterize multi-line text string to a packed bpp mask buffer.
 * Keeps original font bpp (1/2/4/8) — no expansion to 8bpp.
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

        int char_bytes = egui_font_get_utf8_code(s, &utf8_code);
        s += char_bytes;

        int first = 0;
        int last = font_info->count - 1;
        int found = -1;
        while (first <= last)
        {
            int mid = (first + last) / 2;
            if (font_info->code_array[mid].code < utf8_code)
            {
                first = mid + 1;
            }
            else if (font_info->code_array[mid].code == utf8_code)
            {
                found = mid;
                break;
            }
            else
            {
                last = mid - 1;
            }
        }

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
        int x0_idx = (sx >> 1);
        int x0_bit = (sx & 1) << 2;
        int x1_idx = ((sx + 1) >> 1);
        int x1_bit = ((sx + 1) & 1) << 2;
        *a00 = egui_alpha_change_table_4[(buf[base0 + x0_idx] >> x0_bit) & 0x0F];
        *a01 = egui_alpha_change_table_4[(buf[base0 + x1_idx] >> x1_bit) & 0x0F];
        *a10 = egui_alpha_change_table_4[(buf[base1 + x0_idx] >> x0_bit) & 0x0F];
        *a11 = egui_alpha_change_table_4[(buf[base1 + x1_idx] >> x1_bit) & 0x0F];
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

void egui_canvas_draw_text_transform_buffered(const egui_font_t *font, const void *string, egui_dim_t x, egui_dim_t y, int16_t angle_deg, int16_t scale_q8,
                                              egui_color_t color, egui_alpha_t alpha)
{
    if (!font || !string || !font->res)
    {
        return;
    }

    egui_font_std_info_t *font_info = (egui_font_std_info_t *)font->res;
    uint8_t bpp = font_info->font_bit_mode;

    /* Static cache: rasterize once, reuse across PFB tiles */
    static uint8_t *s_packed_buf = NULL;
    static const egui_font_t *s_cached_font = NULL;
    static const void *s_cached_string = NULL;
    static int16_t s_cached_w = 0, s_cached_h = 0;

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

        int buf_size = packed_row_bytes(text_w, bpp) * text_h;
        s_packed_buf = (uint8_t *)egui_malloc(buf_size);
        if (!s_packed_buf)
        {
            s_cached_font = NULL;
            s_cached_string = NULL;
            return;
        }

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        /* For external fonts, load char_array and pixel_buffer into RAM for rasterization */
        const egui_font_std_info_t *raster_font_info = font_info;
        egui_font_std_info_t ext_font_info;
        egui_font_std_char_descriptor_t *ext_char_array = NULL;
        uint8_t *ext_pixel_buffer = NULL;
        if (font_info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
        {
            uint32_t char_array_size = font_info->count * sizeof(egui_font_std_char_descriptor_t);
            ext_char_array = (egui_font_std_char_descriptor_t *)egui_malloc(char_array_size);
            if (!ext_char_array)
            {
                egui_free(s_packed_buf);
                s_packed_buf = NULL;
                s_cached_font = NULL;
                s_cached_string = NULL;
                return;
            }
            egui_api_load_external_resource(ext_char_array, (egui_uintptr_t)font_info->char_array, 0, char_array_size);

            uint32_t max_extent = 0;
            for (int i = 0; i < font_info->count; i++)
            {
                uint32_t end = ext_char_array[i].idx + ext_char_array[i].size;
                if (end > max_extent)
                {
                    max_extent = end;
                }
            }

            ext_pixel_buffer = (uint8_t *)egui_malloc(max_extent);
            if (!ext_pixel_buffer)
            {
                egui_free(ext_char_array);
                egui_free(s_packed_buf);
                s_packed_buf = NULL;
                s_cached_font = NULL;
                s_cached_string = NULL;
                return;
            }
            egui_api_load_external_resource(ext_pixel_buffer, (egui_uintptr_t)font_info->pixel_buffer, 0, max_extent);

            ext_font_info = *font_info;
            ext_font_info.char_array = ext_char_array;
            ext_font_info.pixel_buffer = ext_pixel_buffer;
            ext_font_info.res_type = EGUI_RESOURCE_TYPE_INTERNAL;
            raster_font_info = &ext_font_info;
        }
#else
        const egui_font_std_info_t *raster_font_info = font_info;
#endif

        if (rasterize_text_to_packed(raster_font_info, (const char *)string, s_packed_buf, text_w, text_h, bpp, 0) != 0)
        {
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
            if (ext_char_array)
            {
                egui_free(ext_char_array);
            }
            if (ext_pixel_buffer)
            {
                egui_free(ext_pixel_buffer);
            }
#endif
            egui_free(s_packed_buf);
            s_packed_buf = NULL;
            s_cached_font = NULL;
            s_cached_string = NULL;
            return;
        }

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        if (ext_char_array)
        {
            egui_free(ext_char_array);
        }
        if (ext_pixel_buffer)
        {
            egui_free(ext_pixel_buffer);
        }
#endif

        s_cached_font = font;
        s_cached_string = string;
        s_cached_w = text_w;
        s_cached_h = text_h;
    }

    uint8_t *packed_buf = s_packed_buf;
    int16_t src_w = ctx.src_w;
    int16_t src_h = ctx.src_h;
    int packed_rb = packed_row_bytes(src_w, bpp);

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

#if (EGUI_CONFIG_COLOR_DEPTH == 16)
    uint32_t fg_rb_g = (color.full | ((uint32_t)color.full << 16)) & 0x07E0F81FUL;
#endif

    for (int32_t dy = ctx.draw_y0; dy < ctx.draw_y1; dy++)
    {
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
                    for (int32_t i = 0; i < sir_count; i++)
                    {
                        uint16_t a00, a01, a10, a11;
                        batch_bilinear_packed_alpha(packed_buf, packed_rb, rotatedX >> 15, rotatedY >> 15, bpp, &a00, &a01, &a10, &a11);

                        if ((a00 | a01 | a10 | a11) != 0)
                        {
                            uint8_t fx = (rotatedX >> 7) & 0xFF;
                            uint8_t fy = (rotatedY >> 7) & 0xFF;
                            uint16_t ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
                            uint16_t ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
                            uint8_t pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));

                            if (pixel_alpha == EGUI_ALPHA_100)
                            {
                                *dst_row = color.full;
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
                                egui_rgb_mix_ptr(back, &color, back, pixel_alpha);
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
                        uint16_t a00, a01, a10, a11;
                        batch_bilinear_packed_alpha(packed_buf, packed_rb, rotatedX >> 15, rotatedY >> 15, bpp, &a00, &a01, &a10, &a11);

                        if ((a00 | a01 | a10 | a11) != 0)
                        {
                            uint8_t fx = (rotatedX >> 7) & 0xFF;
                            uint8_t fy = (rotatedY >> 7) & 0xFF;
                            uint16_t ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
                            uint16_t ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
                            uint8_t pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));

                            if (pixel_alpha > 0)
                            {
                                egui_alpha_t final_alpha = ((uint16_t)ctx.canvas_alpha * pixel_alpha + 128) >> 8;
                                if (final_alpha == EGUI_ALPHA_100)
                                {
                                    *dst_row = color.full;
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
                                    egui_rgb_mix_ptr(back, &color, back, final_alpha);
#endif
                                }
                            }
                        }

                        rotatedX += ctx.inv_m00;
                        rotatedY += ctx.inv_m10;
                        dst_row++;
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

#define PACKED_FETCH_ALPHA(px, py) (((px) >= 0 && (px) < src_w && (py) >= 0 && (py) < src_h) ? read_packed_mask_alpha(packed_buf, src_w, (px), (py), bpp) : 0)
                uint16_t a00 = PACKED_FETCH_ALPHA(sx, sy);
                uint16_t a01 = PACKED_FETCH_ALPHA(sx + 1, sy);
                uint16_t a10 = PACKED_FETCH_ALPHA(sx, sy + 1);
                uint16_t a11 = PACKED_FETCH_ALPHA(sx + 1, sy + 1);
#undef PACKED_FETCH_ALPHA

                uint16_t ah0 = a00 + (((int32_t)(a01 - a00) * fx + 128) >> 8);
                uint16_t ah1 = a10 + (((int32_t)(a11 - a10) * fx + 128) >> 8);
                uint8_t pixel_alpha = (uint8_t)(ah0 + (((int32_t)(ah1 - ah0) * fy + 128) >> 8));

                if (pixel_alpha > 0)
                {
                    egui_alpha_t final_alpha = egui_color_alpha_mix(ctx.canvas_alpha, pixel_alpha);
                    if (final_alpha == EGUI_ALPHA_100)
                    {
                        *dst_row = color.full;
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
                        egui_rgb_mix_ptr(back, &color, back, final_alpha);
#endif
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
