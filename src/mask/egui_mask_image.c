#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_mask_image.h"
#include "image/egui_image_std.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "canvas/egui_canvas.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK

/**
 * @brief Resolve the source x coordinate for one destination column.
 *
 * Some callers provide a direct x-map for resampled rows, while simpler paths
 *
 * use a one-to-one index. This helper hides that difference.
 */
__EGUI_STATIC_INLINE__ egui_dim_t egui_mask_image_get_src_x(const egui_dim_t *src_x_map, egui_dim_t index)
{
    return (src_x_map != NULL) ? src_x_map[index] : index;
}

/**
 * @brief Reset all per-row caches derived from the current image mask.
 *
 * The mask keeps one cache for repeated point sampling and one for row-level
 *
 * visible/opaque range queries so scanline renderers can reuse alpha data.
 */
static void egui_mask_image_invalidate_row_cache(egui_mask_image_t *local)
{
    local->point_cached_y = -32768;
    local->point_cached_src_y = -32768;
    local->row_cached_src_y = -32768;
    local->point_cached_alpha_row = NULL;
    local->row_cached_alpha_row = NULL;
    local->point_cached_valid = 0;
    local->row_cached_valid = 0;
    local->row_cached_has_visible = 0;
    local->row_cached_has_opaque = 0;
    local->row_visible_x_start = 0;
    local->row_visible_x_end = 0;
    local->row_opaque_x_start = 0;
    local->row_opaque_x_end = 0;
}

/**
 * @brief Refresh cached geometry, scaling, and fast-path flags after a change.
 *
 * The public mask only knows its destination rectangle and image
 * pointer. This
 * helper converts them into the values the rasterizer needs: bounds, source
 * size, scaling ratios, alpha buffer metadata, and optimization
 * availability.
 */
static void egui_mask_image_refresh_cache(egui_mask_image_t *local)
{
    egui_mask_t *self = &local->base;
    egui_image_std_info_t *image;

    if (local->img == local->cached_img && local->cached_x == self->region.location.x && local->cached_y == self->region.location.y &&
        local->cached_width == self->region.size.width && local->cached_height == self->region.size.height)
    {
        return;
    }

    local->cached_img = local->img;
    local->cached_x = self->region.location.x;
    local->cached_y = self->region.location.y;
    local->cached_width = self->region.size.width;
    local->cached_height = self->region.size.height;
    local->cached_x_end = self->region.location.x + self->region.size.width;
    local->cached_y_end = self->region.location.y + self->region.size.height;

    if (local->img == NULL)
    {
        local->width_radio = 0;
        local->height_radio = 0;
        local->alpha_buf = NULL;
        local->src_width = 0;
        local->src_height = 0;
        local->alpha_type = 0;
        local->res_type = EGUI_RESOURCE_TYPE_INTERNAL;
        local->fast_path_supported = 0;
        local->identity_scale = 0;
        egui_mask_image_invalidate_row_cache(local);
        return;
    }

    image = (egui_image_std_info_t *)local->img->res;
    local->alpha_buf = image->alpha_buf;
    local->src_width = image->width;
    local->src_height = image->height;
    local->alpha_type = image->alpha_type;
    local->res_type = image->res_type;

    if (local->cached_width > 0 && local->cached_height > 0)
    {
        local->width_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->width), EGUI_FLOAT_VALUE_INT(local->cached_width));
        local->height_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->height), EGUI_FLOAT_VALUE_INT(local->cached_height));
    }
    else
    {
        local->width_radio = 0;
        local->height_radio = 0;
    }

    local->fast_path_supported = (local->alpha_buf != NULL && local->alpha_type == EGUI_IMAGE_ALPHA_TYPE_8 && local->res_type == EGUI_RESOURCE_TYPE_INTERNAL &&
                                  local->src_width > 0 && local->src_height > 0 && local->cached_width > 0 && local->cached_height > 0);
    local->identity_scale = (local->cached_width == local->src_width && local->cached_height == local->src_height &&
                             local->width_radio == EGUI_FLOAT_VALUE_INT(1) && local->height_radio == EGUI_FLOAT_VALUE_INT(1));

    egui_mask_image_invalidate_row_cache(local);
}

/**
 * @brief Report whether the mask can read an internal alpha8 buffer directly.
 */
static int egui_mask_image_supports_internal_alpha8_fast_path(const egui_mask_image_t *local)
{
    return local->fast_path_supported;
}

/**
 * @brief Report whether destination pixels map 1:1 onto source pixels.
 */
static int egui_mask_image_is_identity_scale(const egui_mask_image_t *local)
{
    return local->identity_scale;
}

/**
 * @brief Blend a solid-color run into the destination buffer.
 *
 * This helper is used when a contiguous destination span is fully covered by
 * the mask
 * and only the caller-provided color and alpha matter.
 */
static void egui_mask_image_blend_solid_row(egui_color_int_t *dst, egui_dim_t count, egui_color_t color, egui_alpha_t alpha)
{
    if (count <= 0 || alpha == 0)
    {
        return;
    }

    if (alpha == EGUI_ALPHA_100)
    {
        egui_canvas_fill_color_buffer(dst, count, color);
        return;
    }

    for (egui_dim_t i = 0; i < count; i++)
    {
        egui_rgb_mix_ptr((egui_color_t *)&dst[i], &color, (egui_color_t *)&dst[i], alpha);
    }
}

/**
 * @brief Copy one RGB565 row as cheaply as possible.
 *
 * Fully opaque paths can bypass per-pixel blending and copy source pixels
 * directly, including
 * aligned 32-bit bulk copies on 16-bit color builds.
 */
static void egui_mask_image_copy_rgb565_row(egui_color_int_t *dst_row, const uint16_t *src_row, egui_dim_t count)
{
    if (count <= 0)
    {
        return;
    }

#if EGUI_CONFIG_COLOR_DEPTH == 16
    if ((((egui_uintptr_t)dst_row ^ (egui_uintptr_t)src_row) & 0x03U) != 0U)
    {
        for (egui_dim_t i = 0; i < count; i++)
        {
            dst_row[i] = src_row[i];
        }
        return;
    }

    if ((((egui_uintptr_t)dst_row) & 0x03U) != 0U)
    {
        *dst_row++ = *src_row++;
        count--;
    }

    if (count >= 2)
    {
        const uint32_t *src32 = (const uint32_t *)src_row;
        uint32_t *dst32 = (uint32_t *)dst_row;

        while (count >= 8)
        {
            dst32[0] = src32[0];
            dst32[1] = src32[1];
            dst32[2] = src32[2];
            dst32[3] = src32[3];
            src32 += 4;
            dst32 += 4;
            count -= 8;
        }

        while (count >= 2)
        {
            *dst32++ = *src32++;
            count -= 2;
        }

        src_row = (const uint16_t *)src32;
        dst_row = (egui_color_int_t *)dst32;
    }

    if (count != 0)
    {
        *dst_row = *src_row;
    }
#else
    for (egui_dim_t i = 0; i < count; i++)
    {
        dst_row[i] = EGUI_COLOR_RGB565_TRANS(src_row[i]);
    }
#endif
}

/**
 * @brief Blend one RGB565 row with a uniform alpha multiplier.
 */
static void egui_mask_image_blend_rgb565_row(egui_color_int_t *dst_row, const uint16_t *src_row, egui_dim_t count, egui_alpha_t alpha)
{
    if (count <= 0 || alpha == 0)
    {
        return;
    }

    if (alpha == EGUI_ALPHA_100)
    {
        egui_mask_image_copy_rgb565_row(dst_row, src_row, count);
        return;
    }

    for (egui_dim_t i = 0; i < count; i++)
    {
        egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[i], src_row[i], alpha);
    }
}

/**
 * @brief Blend one identity-scale RGB565 row with both source alpha and mask alpha.
 *
 * The loop recognizes long transparent and fully opaque runs so
 * common cases
 * can skip work or fall back to row copies instead of blending pixel-by-pixel.
 */
static void egui_mask_image_blend_rgb565_alpha8_identity_row(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                             const uint8_t *mask_alpha_row, egui_dim_t count, egui_alpha_t canvas_alpha)
{
    egui_dim_t i = 0;

    if (count <= 0 || canvas_alpha == 0)
    {
        return;
    }

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        while (i < count)
        {
            while (i + 4 <= count && (*(const uint32_t *)&mask_alpha_row[i] == 0x00000000U || *(const uint32_t *)&src_alpha_row[i] == 0x00000000U))
            {
                i += 4;
            }
            while (i < count && (mask_alpha_row[i] == 0 || src_alpha_row[i] == 0))
            {
                i++;
            }

            {
                egui_dim_t opaque_start = i;

                while (i + 4 <= count && *(const uint32_t *)&mask_alpha_row[i] == 0xFFFFFFFFU && *(const uint32_t *)&src_alpha_row[i] == 0xFFFFFFFFU)
                {
                    i += 4;
                }
                while (i < count && mask_alpha_row[i] == EGUI_ALPHA_100 && src_alpha_row[i] == EGUI_ALPHA_100)
                {
                    i++;
                }

                if (opaque_start < i)
                {
                    egui_mask_image_copy_rgb565_row(&dst_row[opaque_start], &src_row[opaque_start], i - opaque_start);
                    continue;
                }
            }

            if (i < count)
            {
                egui_alpha_t alpha = src_alpha_row[i];

                if (mask_alpha_row[i] != EGUI_ALPHA_100)
                {
                    alpha = egui_color_alpha_mix(mask_alpha_row[i], alpha);
                }

                if (alpha != 0)
                {
                    egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[i], src_row[i], alpha);
                }
                i++;
            }
        }
        return;
    }

    while (i < count)
    {
        while (i + 4 <= count && (*(const uint32_t *)&mask_alpha_row[i] == 0x00000000U || *(const uint32_t *)&src_alpha_row[i] == 0x00000000U))
        {
            i += 4;
        }
        while (i < count && (mask_alpha_row[i] == 0 || src_alpha_row[i] == 0))
        {
            i++;
        }

        {
            egui_dim_t opaque_start = i;

            while (i + 4 <= count && *(const uint32_t *)&mask_alpha_row[i] == 0xFFFFFFFFU && *(const uint32_t *)&src_alpha_row[i] == 0xFFFFFFFFU)
            {
                i += 4;
            }
            while (i < count && mask_alpha_row[i] == EGUI_ALPHA_100 && src_alpha_row[i] == EGUI_ALPHA_100)
            {
                i++;
            }

            if (opaque_start < i)
            {
                egui_mask_image_blend_rgb565_row(&dst_row[opaque_start], &src_row[opaque_start], i - opaque_start, canvas_alpha);
                continue;
            }
        }

        if (i < count)
        {
            egui_alpha_t alpha = src_alpha_row[i];

            if (mask_alpha_row[i] != EGUI_ALPHA_100)
            {
                alpha = egui_color_alpha_mix(mask_alpha_row[i], alpha);
            }
            if (canvas_alpha != EGUI_ALPHA_100)
            {
                alpha = egui_color_alpha_mix(canvas_alpha, alpha);
            }

            if (alpha != 0)
            {
                egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[i], src_row[i], alpha);
            }
            i++;
        }
    }
}

/**
 * @brief Blend one identity-scale RGB565 row using mask alpha only.
 *
 * This is the simpler sibling of the function above for sources that do not
 *
 * carry their own per-pixel alpha channel.
 */
static void egui_mask_image_blend_rgb565_identity_row(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *mask_alpha_row, egui_dim_t count,
                                                      egui_alpha_t canvas_alpha)
{
    egui_dim_t i = 0;

    if (count <= 0 || canvas_alpha == 0)
    {
        return;
    }

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        while (i < count)
        {
            while (i + 4 <= count && *(const uint32_t *)&mask_alpha_row[i] == 0x00000000U)
            {
                i += 4;
            }
            while (i < count && mask_alpha_row[i] == 0)
            {
                i++;
            }

            {
                egui_dim_t opaque_start = i;

                while (i + 4 <= count && *(const uint32_t *)&mask_alpha_row[i] == 0xFFFFFFFFU)
                {
                    i += 4;
                }
                while (i < count && mask_alpha_row[i] == EGUI_ALPHA_100)
                {
                    i++;
                }

                if (opaque_start < i)
                {
                    egui_mask_image_copy_rgb565_row(&dst_row[opaque_start], &src_row[opaque_start], i - opaque_start);
                    continue;
                }
            }

            if (i < count)
            {
                egui_alpha_t alpha = mask_alpha_row[i];

                if (alpha != 0)
                {
                    egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[i], src_row[i], alpha);
                }
                i++;
            }
        }
        return;
    }

    while (i < count)
    {
        while (i + 4 <= count && *(const uint32_t *)&mask_alpha_row[i] == 0x00000000U)
        {
            i += 4;
        }
        while (i < count && mask_alpha_row[i] == 0)
        {
            i++;
        }

        {
            egui_dim_t opaque_start = i;

            while (i + 4 <= count && *(const uint32_t *)&mask_alpha_row[i] == 0xFFFFFFFFU)
            {
                i += 4;
            }
            while (i < count && mask_alpha_row[i] == EGUI_ALPHA_100)
            {
                i++;
            }

            if (opaque_start < i)
            {
                egui_mask_image_blend_rgb565_row(&dst_row[opaque_start], &src_row[opaque_start], i - opaque_start, canvas_alpha);
                continue;
            }
        }

        if (i < count)
        {
            egui_alpha_t alpha = mask_alpha_row[i];

            if (canvas_alpha != EGUI_ALPHA_100)
            {
                alpha = egui_color_alpha_mix(canvas_alpha, alpha);
            }

            if (alpha != 0)
            {
                egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[i], src_row[i], alpha);
            }
            i++;
        }
    }
}

/**
 * @brief Ask whether one destination span maps to a linear source span.
 *
 * If so, callers can still reuse the identity-row helpers even when an
 *
 * external source x-map is present.
 */
static int egui_mask_image_get_linear_src_segment(const egui_dim_t *src_x_map, egui_dim_t start, egui_dim_t end, egui_dim_t *src_x_start)
{
    return egui_image_std_get_linear_src_x_segment(src_x_map, start, end, src_x_start);
}

/**
 * @brief Scan one mask row and cache its visible and opaque horizontal ranges.
 *
 * For internal alpha8 images, this summarizes the row into two useful
 * spans:
 * the first-to-last non-zero coverage and the longest fully opaque run.
 */
static int egui_mask_image_refresh_row_scan(egui_mask_image_t *local, egui_dim_t y)
{
    egui_dim_t local_y;
    egui_dim_t src_y;
    const uint8_t *alpha_row;
    egui_dim_t first_vis = -1;
    egui_dim_t last_vis = -1;
    egui_dim_t run_start = -1;
    egui_dim_t best_start = -1;
    egui_dim_t best_len = 0;

    if (!egui_mask_image_supports_internal_alpha8_fast_path(local))
    {
        return 0;
    }

    if (y < local->cached_y || y >= local->cached_y_end)
    {
        local->row_cached_valid = 1;
        local->row_cached_has_visible = 0;
        local->row_cached_has_opaque = 0;
        local->row_cached_src_y = -32768;
        local->row_cached_alpha_row = NULL;
        return 1;
    }

    local_y = y - local->cached_y;
    src_y = (egui_dim_t)EGUI_FLOAT_MULT(local_y, local->height_radio);
    if (src_y < 0 || src_y >= local->src_height)
    {
        local->row_cached_valid = 1;
        local->row_cached_has_visible = 0;
        local->row_cached_has_opaque = 0;
        local->row_cached_src_y = src_y;
        local->row_cached_alpha_row = NULL;
        return 1;
    }

    if (local->row_cached_valid && local->row_cached_src_y == src_y)
    {
        return 1;
    }

    alpha_row = (const uint8_t *)local->alpha_buf + (uint32_t)src_y * local->src_width;
    local->row_cached_alpha_row = alpha_row;
    local->row_cached_src_y = src_y;
    local->row_cached_valid = 1;
    local->row_cached_has_visible = 0;
    local->row_cached_has_opaque = 0;

    for (egui_dim_t i = 0; i < local->src_width; i++)
    {
        uint8_t a = alpha_row[i];

        if (a != 0)
        {
            if (first_vis < 0)
            {
                first_vis = i;
            }
            last_vis = i;
        }

        if (a == EGUI_ALPHA_100)
        {
            if (run_start < 0)
            {
                run_start = i;
            }
        }
        else if (run_start >= 0)
        {
            egui_dim_t len = i - run_start;
            if (len > best_len)
            {
                best_start = run_start;
                best_len = len;
            }
            run_start = -1;
        }
    }

    if (run_start >= 0)
    {
        egui_dim_t len = local->src_width - run_start;
        if (len > best_len)
        {
            best_start = run_start;
            best_len = len;
        }
    }

    if (first_vis >= 0)
    {
        local->row_cached_has_visible = 1;
        local->row_visible_x_start = local->cached_x + (egui_dim_t)(((int32_t)first_vis * local->cached_width) / local->src_width);
        local->row_visible_x_end = local->cached_x + (egui_dim_t)(((int32_t)last_vis * local->cached_width + local->src_width - 1) / local->src_width) + 1;
    }

    if (best_len > 0)
    {
        egui_dim_t best_end = best_start + best_len;
        local->row_cached_has_opaque = 1;
        local->row_opaque_x_start = local->cached_x + (egui_dim_t)(((int32_t)best_start * local->cached_width + local->src_width - 1) / local->src_width);
        local->row_opaque_x_end = local->cached_x + (egui_dim_t)(((int32_t)(best_end - 1) * local->cached_width) / local->src_width) + 1;
    }

    return 1;
}

/**
 * @brief Attach a new image resource to the mask and rebuild derived caches.
 */
void egui_mask_image_set_image(egui_mask_t *self, egui_image_t *img)
{
    EGUI_LOCAL_INIT(egui_mask_image_t);
    local->img = img;
    egui_mask_image_refresh_cache(local);
}

/**
 * @brief Query the image mask coverage at one destination pixel.
 *
 * Fast-path mode samples the cached alpha8 row directly. Otherwise the code
 * falls
 * back to the generic image API and uses that pixel's alpha as coverage.
 */
void egui_mask_image_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    EGUI_LOCAL_INIT(egui_mask_image_t);
    egui_dim_t local_x;
    egui_dim_t local_y;
    egui_dim_t src_x;
    egui_dim_t src_y;

    egui_mask_image_refresh_cache(local);
    if (local->img == NULL)
    {
        return;
    }

    if (x < local->cached_x || x >= local->cached_x_end || y < local->cached_y || y >= local->cached_y_end)
    {
        color->full = 0;
        *alpha = 0;
        return;
    }

    local_x = x - local->cached_x;
    local_y = y - local->cached_y;

    if (egui_mask_image_supports_internal_alpha8_fast_path(local))
    {
        if (local_y == local->point_cached_y)
        {
            if (!local->point_cached_valid || local->point_cached_alpha_row == NULL)
            {
                color->full = 0;
                *alpha = 0;
                return;
            }
        }
        else
        {
            src_y = (egui_dim_t)EGUI_FLOAT_MULT(local_y, local->height_radio);
            local->point_cached_y = local_y;
            local->point_cached_src_y = src_y;
            if (src_y < 0 || src_y >= local->src_height)
            {
                local->point_cached_valid = 0;
                local->point_cached_alpha_row = NULL;
                color->full = 0;
                *alpha = 0;
                return;
            }
            local->point_cached_alpha_row = (const uint8_t *)local->alpha_buf + (uint32_t)src_y * local->src_width;
            local->point_cached_valid = 1;
        }

        src_x = (egui_dim_t)EGUI_FLOAT_MULT(local_x, local->width_radio);
        if (src_x < 0 || src_x >= local->src_width)
        {
            color->full = 0;
            *alpha = 0;
            return;
        }

        *alpha = local->point_cached_alpha_row[src_x];
        return;
    }

    src_x = (egui_dim_t)EGUI_FLOAT_MULT(local_x, local->width_radio);
    src_y = (egui_dim_t)EGUI_FLOAT_MULT(local_y, local->height_radio);

    egui_color_t orign_color;
    egui_alpha_t orign_alpha;
    if (local->img->api->get_point(local->img, src_x, src_y, &orign_color, &orign_alpha))
    {
        *alpha = orign_alpha;
    }
    else
    {
        *alpha = 0;
    }
}

/**
 * @brief Fill one destination scanline segment through the image mask.
 *
 * Identity-scale rows can skip empty runs and bulk-fill opaque runs, while
 *
 * scaled rows compute the matching source x coordinate per destination pixel.
 */
int egui_mask_image_fill_row_segment(egui_mask_t *self, egui_color_int_t *dst, egui_dim_t y, egui_dim_t x_start, egui_dim_t x_end, egui_color_t color,
                                     egui_alpha_t alpha)
{
    EGUI_LOCAL_INIT(egui_mask_image_t);
    egui_dim_t seg_start;
    egui_dim_t seg_end;
    egui_dim_t local_y;
    egui_dim_t src_y;
    egui_float_t src_x_acc;

    egui_mask_image_refresh_cache(local);
    if (!egui_mask_image_supports_internal_alpha8_fast_path(local))
    {
        return 0;
    }

    seg_start = EGUI_MAX(x_start, local->cached_x);
    seg_end = EGUI_MIN(x_end, local->cached_x_end);
    if (y < local->cached_y || y >= local->cached_y_end || seg_start >= seg_end)
    {
        return 1;
    }

    local_y = y - local->cached_y;
    if (local_y == local->point_cached_y)
    {
        if (!local->point_cached_valid || local->point_cached_alpha_row == NULL)
        {
            return 1;
        }
    }
    else
    {
        src_y = (egui_dim_t)EGUI_FLOAT_MULT(local_y, local->height_radio);
        local->point_cached_y = local_y;
        local->point_cached_src_y = src_y;
        if (src_y < 0 || src_y >= local->src_height)
        {
            local->point_cached_valid = 0;
            local->point_cached_alpha_row = NULL;
            return 1;
        }
        local->point_cached_alpha_row = (const uint8_t *)local->alpha_buf + (uint32_t)src_y * local->src_width;
        local->point_cached_valid = 1;
    }

    dst += seg_start - x_start;
    if (egui_mask_image_is_identity_scale(local))
    {
        const uint8_t *mask_alpha_row = local->point_cached_alpha_row + (seg_start - local->cached_x);
        egui_dim_t remaining = seg_end - seg_start;

        while (remaining > 0)
        {
            egui_dim_t run = 0;

            while (run + 4 <= remaining && *(const uint32_t *)&mask_alpha_row[run] == 0x00000000U)
            {
                run += 4;
            }
            while (run < remaining && mask_alpha_row[run] == 0)
            {
                run++;
            }

            dst += run;
            mask_alpha_row += run;
            remaining -= run;
            if (remaining == 0)
            {
                break;
            }

            run = 0;
            while (run + 4 <= remaining && *(const uint32_t *)&mask_alpha_row[run] == 0xFFFFFFFFU)
            {
                run += 4;
            }
            while (run < remaining && mask_alpha_row[run] == EGUI_ALPHA_100)
            {
                run++;
            }

            if (run > 0)
            {
                egui_mask_image_blend_solid_row(dst, run, color, alpha);
                dst += run;
                mask_alpha_row += run;
                remaining -= run;
                continue;
            }

            {
                egui_alpha_t pixel_alpha = egui_color_alpha_mix(mask_alpha_row[0], alpha);

                if (pixel_alpha == EGUI_ALPHA_100)
                {
                    *dst = color.full;
                }
                else if (pixel_alpha != 0)
                {
                    egui_rgb_mix_ptr((egui_color_t *)dst, &color, (egui_color_t *)dst, pixel_alpha);
                }
            }

            dst++;
            mask_alpha_row++;
            remaining--;
        }

        return 1;
    }

    src_x_acc = (egui_float_t)(seg_start - local->cached_x) * local->width_radio;
    for (egui_dim_t xp = seg_start; xp < seg_end; xp++, dst++, src_x_acc += local->width_radio)
    {
        egui_dim_t src_x = EGUI_FLOAT_INT_PART(src_x_acc);
        egui_alpha_t pixel_alpha;

        if (src_x < 0 || src_x >= local->src_width)
        {
            continue;
        }

        pixel_alpha = egui_color_alpha_mix(local->point_cached_alpha_row[src_x], alpha);
        if (pixel_alpha == 0)
        {
            continue;
        }

        if (pixel_alpha == EGUI_ALPHA_100)
        {
            *dst = color.full;
        }
        else
        {
            egui_rgb_mix_ptr((egui_color_t *)dst, &color, (egui_color_t *)dst, pixel_alpha);
        }
    }

    return 1;
}

/**
 * @brief Blend an RGB565+alpha8 source row through the image mask.
 *
 * Source alpha and mask alpha are combined first, then optional canvas alpha is
 *
 * applied on top. Identity-scale rows delegate to a run-aware helper.
 */
int egui_mask_image_blend_rgb565_alpha8_row_segment(egui_mask_t *self, egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                    egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y, egui_alpha_t canvas_alpha)
{
    EGUI_LOCAL_INIT(egui_mask_image_t);
    egui_dim_t seg_start;
    egui_dim_t seg_end;
    egui_dim_t local_y;
    egui_dim_t src_y;
    egui_float_t mask_src_acc;

    egui_mask_image_refresh_cache(local);
    if (!egui_mask_image_supports_internal_alpha8_fast_path(local))
    {
        return 0;
    }

    seg_start = EGUI_MAX(screen_x, local->cached_x);
    seg_end = EGUI_MIN(screen_x + count, local->cached_x_end);
    if (screen_y < local->cached_y || screen_y >= local->cached_y_end || seg_start >= seg_end)
    {
        return 1;
    }

    local_y = screen_y - local->cached_y;
    if (local_y == local->point_cached_y)
    {
        if (!local->point_cached_valid || local->point_cached_alpha_row == NULL)
        {
            return 1;
        }
    }
    else
    {
        src_y = (egui_dim_t)EGUI_FLOAT_MULT(local_y, local->height_radio);
        local->point_cached_y = local_y;
        local->point_cached_src_y = src_y;
        if (src_y < 0 || src_y >= local->src_height)
        {
            local->point_cached_valid = 0;
            local->point_cached_alpha_row = NULL;
            return 1;
        }
        local->point_cached_alpha_row = (const uint8_t *)local->alpha_buf + (uint32_t)src_y * local->src_width;
        local->point_cached_valid = 1;
    }

    dst_row += seg_start - screen_x;
    src_row += seg_start - screen_x;
    src_alpha_row += seg_start - screen_x;
    if (egui_mask_image_is_identity_scale(local))
    {
        const uint8_t *mask_alpha_row = local->point_cached_alpha_row + (seg_start - local->cached_x);
        egui_mask_image_blend_rgb565_alpha8_identity_row(dst_row, src_row, src_alpha_row, mask_alpha_row, seg_end - seg_start, canvas_alpha);
        return 1;
    }

    mask_src_acc = (egui_float_t)(seg_start - local->cached_x) * local->width_radio;

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        for (egui_dim_t i = seg_start; i < seg_end; i++, dst_row++, src_row++, src_alpha_row++, mask_src_acc += local->width_radio)
        {
            egui_dim_t mask_src_x = EGUI_FLOAT_INT_PART(mask_src_acc);
            egui_alpha_t alpha;

            if (mask_src_x < 0 || mask_src_x >= local->src_width || *src_alpha_row == 0)
            {
                continue;
            }

            alpha = egui_color_alpha_mix(local->point_cached_alpha_row[mask_src_x], *src_alpha_row);
            if (alpha == 0)
            {
                continue;
            }

            egui_image_std_blend_rgb565_src_pixel_fast(dst_row, *src_row, alpha);
        }
    }
    else
    {
        for (egui_dim_t i = seg_start; i < seg_end; i++, dst_row++, src_row++, src_alpha_row++, mask_src_acc += local->width_radio)
        {
            egui_dim_t mask_src_x = EGUI_FLOAT_INT_PART(mask_src_acc);
            egui_alpha_t alpha;

            if (mask_src_x < 0 || mask_src_x >= local->src_width || *src_alpha_row == 0)
            {
                continue;
            }

            alpha = egui_color_alpha_mix(local->point_cached_alpha_row[mask_src_x], *src_alpha_row);
            alpha = egui_color_alpha_mix(canvas_alpha, alpha);
            if (alpha == 0)
            {
                continue;
            }

            egui_image_std_blend_rgb565_src_pixel_fast(dst_row, *src_row, alpha);
        }
    }

    return 1;
}

/**
 * @brief Blend a block of RGB565+alpha8 rows when mask and source use identity scale.
 *
 * This batched version amortizes setup across consecutive rows
 * while reusing
 * the same identity-row optimization as the segment path.
 */
int egui_mask_image_blend_rgb565_alpha8_row_block(egui_mask_t *self, egui_color_int_t *dst_row, egui_dim_t dst_stride, const uint16_t *src_row,
                                                  egui_dim_t src_stride, const uint8_t *src_alpha_row, egui_dim_t alpha_stride, egui_dim_t row_count,
                                                  egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y, egui_alpha_t canvas_alpha)
{
    EGUI_LOCAL_INIT(egui_mask_image_t);
    egui_dim_t seg_start;
    egui_dim_t seg_end;
    egui_dim_t seg_count;
    egui_dim_t seg_offset;
    egui_dim_t mask_x_offset;
    egui_dim_t last_valid_local_y = -32768;

    egui_mask_image_refresh_cache(local);
    if (!egui_mask_image_supports_internal_alpha8_fast_path(local) || !egui_mask_image_is_identity_scale(local))
    {
        return 0;
    }

    if (row_count <= 0 || count <= 0 || canvas_alpha == 0)
    {
        return 1;
    }

    seg_start = EGUI_MAX(screen_x, local->cached_x);
    seg_end = EGUI_MIN(screen_x + count, local->cached_x_end);
    if (seg_start >= seg_end)
    {
        return 1;
    }

    seg_count = seg_end - seg_start;
    seg_offset = seg_start - screen_x;
    mask_x_offset = seg_start - local->cached_x;
    dst_row += seg_offset;
    src_row += seg_offset;
    src_alpha_row += seg_offset;

    for (egui_dim_t row = 0; row < row_count; row++)
    {
        egui_dim_t local_y = (screen_y + row) - local->cached_y;

        if (local_y >= 0 && local_y < local->src_height)
        {
            const uint8_t *mask_alpha_row = local->alpha_buf + ((uint32_t)local_y * local->src_width) + mask_x_offset;

            egui_mask_image_blend_rgb565_alpha8_identity_row(dst_row, src_row, src_alpha_row, mask_alpha_row, seg_count, canvas_alpha);
            last_valid_local_y = local_y;
        }

        dst_row += dst_stride;
        src_row += src_stride;
        src_alpha_row += alpha_stride;
    }

    if (last_valid_local_y >= 0)
    {
        local->point_cached_y = last_valid_local_y;
        local->point_cached_src_y = last_valid_local_y;
        local->point_cached_alpha_row = (const uint8_t *)local->alpha_buf + (uint32_t)last_valid_local_y * local->src_width;
        local->point_cached_valid = 1;
    }
    else
    {
        local->point_cached_valid = 0;
        local->point_cached_alpha_row = NULL;
    }

    return 1;
}

/**
 * @brief Blend an RGB565 source row through the image mask.
 *
 * This is the non-source-alpha sibling of the function above: coverage comes
 * only from
 * the image mask itself plus the optional canvas alpha.
 */
int egui_mask_image_blend_rgb565_row_segment(egui_mask_t *self, egui_color_int_t *dst_row, const uint16_t *src_row, egui_dim_t count, egui_dim_t screen_x,
                                             egui_dim_t screen_y, egui_alpha_t canvas_alpha)
{
    EGUI_LOCAL_INIT(egui_mask_image_t);
    egui_dim_t seg_start;
    egui_dim_t seg_end;
    egui_dim_t local_y;
    egui_dim_t src_y;
    egui_float_t mask_src_acc;

    egui_mask_image_refresh_cache(local);
    if (!egui_mask_image_supports_internal_alpha8_fast_path(local))
    {
        return 0;
    }

    seg_start = EGUI_MAX(screen_x, local->cached_x);
    seg_end = EGUI_MIN(screen_x + count, local->cached_x_end);
    if (screen_y < local->cached_y || screen_y >= local->cached_y_end || seg_start >= seg_end)
    {
        return 1;
    }

    local_y = screen_y - local->cached_y;
    if (local_y == local->point_cached_y)
    {
        if (!local->point_cached_valid || local->point_cached_alpha_row == NULL)
        {
            return 1;
        }
    }
    else
    {
        src_y = (egui_dim_t)EGUI_FLOAT_MULT(local_y, local->height_radio);
        local->point_cached_y = local_y;
        local->point_cached_src_y = src_y;
        if (src_y < 0 || src_y >= local->src_height)
        {
            local->point_cached_valid = 0;
            local->point_cached_alpha_row = NULL;
            return 1;
        }
        local->point_cached_alpha_row = (const uint8_t *)local->alpha_buf + (uint32_t)src_y * local->src_width;
        local->point_cached_valid = 1;
    }

    dst_row += seg_start - screen_x;
    src_row += seg_start - screen_x;
    if (egui_mask_image_is_identity_scale(local))
    {
        const uint8_t *mask_alpha_row = local->point_cached_alpha_row + (seg_start - local->cached_x);

        egui_mask_image_blend_rgb565_identity_row(dst_row, src_row, mask_alpha_row, seg_end - seg_start, canvas_alpha);
        return 1;
    }

    mask_src_acc = (egui_float_t)(seg_start - local->cached_x) * local->width_radio;
    if (canvas_alpha == EGUI_ALPHA_100)
    {
        for (egui_dim_t i = seg_start; i < seg_end; i++, dst_row++, src_row++, mask_src_acc += local->width_radio)
        {
            egui_dim_t mask_src_x = EGUI_FLOAT_INT_PART(mask_src_acc);
            egui_alpha_t alpha;

            if (mask_src_x < 0 || mask_src_x >= local->src_width)
            {
                continue;
            }

            alpha = local->point_cached_alpha_row[mask_src_x];
            if (alpha == 0)
            {
                continue;
            }

            egui_image_std_blend_rgb565_src_pixel_fast(dst_row, *src_row, alpha);
        }
    }
    else
    {
        for (egui_dim_t i = seg_start; i < seg_end; i++, dst_row++, src_row++, mask_src_acc += local->width_radio)
        {
            egui_dim_t mask_src_x = EGUI_FLOAT_INT_PART(mask_src_acc);
            egui_alpha_t alpha;

            if (mask_src_x < 0 || mask_src_x >= local->src_width)
            {
                continue;
            }

            alpha = local->point_cached_alpha_row[mask_src_x];
            if (alpha == 0)
            {
                continue;
            }

            alpha = egui_color_alpha_mix(canvas_alpha, alpha);
            if (alpha == 0)
            {
                continue;
            }

            egui_image_std_blend_rgb565_src_pixel_fast(dst_row, *src_row, alpha);
        }
    }

    return 1;
}

/**
 * @brief Blend a block of RGB565 rows when the image mask is identity-scaled.
 */
int egui_mask_image_blend_rgb565_row_block(egui_mask_t *self, egui_color_int_t *dst_row, egui_dim_t dst_stride, const uint16_t *src_row, egui_dim_t src_stride,
                                           egui_dim_t row_count, egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y, egui_alpha_t canvas_alpha)
{
    EGUI_LOCAL_INIT(egui_mask_image_t);
    egui_dim_t seg_start;
    egui_dim_t seg_end;
    egui_dim_t seg_count;
    egui_dim_t seg_offset;
    egui_dim_t mask_x_offset;
    egui_dim_t last_valid_local_y = -32768;

    egui_mask_image_refresh_cache(local);
    if (!egui_mask_image_supports_internal_alpha8_fast_path(local) || !egui_mask_image_is_identity_scale(local))
    {
        return 0;
    }

    if (row_count <= 0 || count <= 0 || canvas_alpha == 0)
    {
        return 1;
    }

    seg_start = EGUI_MAX(screen_x, local->cached_x);
    seg_end = EGUI_MIN(screen_x + count, local->cached_x_end);
    if (seg_start >= seg_end)
    {
        return 1;
    }

    seg_count = seg_end - seg_start;
    seg_offset = seg_start - screen_x;
    mask_x_offset = seg_start - local->cached_x;
    dst_row += seg_offset;
    src_row += seg_offset;

    for (egui_dim_t row = 0; row < row_count; row++)
    {
        egui_dim_t local_y = (screen_y + row) - local->cached_y;

        if (local_y >= 0 && local_y < local->src_height)
        {
            const uint8_t *mask_alpha_row = local->alpha_buf + ((uint32_t)local_y * local->src_width) + mask_x_offset;

            egui_mask_image_blend_rgb565_identity_row(dst_row, src_row, mask_alpha_row, seg_count, canvas_alpha);
            last_valid_local_y = local_y;
        }

        dst_row += dst_stride;
        src_row += src_stride;
    }

    if (last_valid_local_y >= 0)
    {
        local->point_cached_y = last_valid_local_y;
        local->point_cached_src_y = last_valid_local_y;
        local->point_cached_alpha_row = (const uint8_t *)local->alpha_buf + (uint32_t)last_valid_local_y * local->src_width;
        local->point_cached_valid = 1;
    }
    else
    {
        local->point_cached_valid = 0;
        local->point_cached_alpha_row = NULL;
    }

    return 1;
}

/**
 * @brief Blend an RGB565+alpha8 segment that may use an external source x-map.
 *
 * When the x-map still resolves to one linear source span, the code
 * reuses the
 * identity helper. Otherwise it samples the mapped source coordinate per pixel.
 */
int egui_mask_image_blend_rgb565_alpha8_segment(egui_mask_t *self, egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                const egui_dim_t *src_x_map, egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y,
                                                egui_alpha_t canvas_alpha)
{
    EGUI_LOCAL_INIT(egui_mask_image_t);
    egui_dim_t seg_start;
    egui_dim_t seg_end;
    egui_dim_t local_y;
    egui_dim_t src_y;
    egui_float_t mask_src_acc;

    egui_mask_image_refresh_cache(local);
    if (!egui_mask_image_supports_internal_alpha8_fast_path(local))
    {
        return 0;
    }

    seg_start = EGUI_MAX(screen_x, local->cached_x);
    seg_end = EGUI_MIN(screen_x + count, local->cached_x_end);
    if (screen_y < local->cached_y || screen_y >= local->cached_y_end || seg_start >= seg_end)
    {
        return 1;
    }

    local_y = screen_y - local->cached_y;
    if (local_y == local->point_cached_y)
    {
        if (!local->point_cached_valid || local->point_cached_alpha_row == NULL)
        {
            return 1;
        }
    }
    else
    {
        src_y = (egui_dim_t)EGUI_FLOAT_MULT(local_y, local->height_radio);
        local->point_cached_y = local_y;
        local->point_cached_src_y = src_y;
        if (src_y < 0 || src_y >= local->src_height)
        {
            local->point_cached_valid = 0;
            local->point_cached_alpha_row = NULL;
            return 1;
        }
        local->point_cached_alpha_row = (const uint8_t *)local->alpha_buf + (uint32_t)src_y * local->src_width;
        local->point_cached_valid = 1;
    }

    if (egui_mask_image_is_identity_scale(local))
    {
        egui_dim_t dst_start = seg_start - screen_x;
        egui_dim_t dst_end = seg_end - screen_x;
        egui_dim_t src_x_start;

        if (egui_mask_image_get_linear_src_segment(src_x_map, dst_start, dst_end, &src_x_start))
        {
            const uint8_t *mask_alpha_row = local->point_cached_alpha_row + (seg_start - local->cached_x);

            egui_mask_image_blend_rgb565_alpha8_identity_row(&dst_row[dst_start], &src_row[src_x_start], &src_alpha_row[src_x_start], mask_alpha_row,
                                                             dst_end - dst_start, canvas_alpha);
            return 1;
        }
    }

    mask_src_acc = (egui_float_t)(seg_start - local->cached_x) * local->width_radio;
    if (canvas_alpha == EGUI_ALPHA_100)
    {
        for (egui_dim_t i = seg_start - screen_x; i < seg_end - screen_x; i++, mask_src_acc += local->width_radio)
        {
            egui_dim_t mask_src_x = EGUI_FLOAT_INT_PART(mask_src_acc);
            egui_dim_t src_x = egui_mask_image_get_src_x(src_x_map, i);
            egui_alpha_t alpha = src_alpha_row[src_x];

            if (alpha == 0 || mask_src_x < 0 || mask_src_x >= local->src_width)
            {
                continue;
            }

            alpha = egui_color_alpha_mix(local->point_cached_alpha_row[mask_src_x], alpha);
            if (alpha == 0)
            {
                continue;
            }

            egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[i], src_row[src_x], alpha);
        }
    }
    else
    {
        for (egui_dim_t i = seg_start - screen_x; i < seg_end - screen_x; i++, mask_src_acc += local->width_radio)
        {
            egui_dim_t mask_src_x = EGUI_FLOAT_INT_PART(mask_src_acc);
            egui_dim_t src_x = egui_mask_image_get_src_x(src_x_map, i);
            egui_alpha_t alpha = src_alpha_row[src_x];

            if (alpha == 0 || mask_src_x < 0 || mask_src_x >= local->src_width)
            {
                continue;
            }

            alpha = egui_color_alpha_mix(local->point_cached_alpha_row[mask_src_x], alpha);
            alpha = egui_color_alpha_mix(canvas_alpha, alpha);
            if (alpha == 0)
            {
                continue;
            }

            egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[i], src_row[src_x], alpha);
        }
    }

    return 1;
}

/**
 * @brief Report the fully opaque horizontal span for one mask row.
 *
 * Rows with no alpha buffer behave like a plain rectangular mask. Alpha8 rows
 * use
 * the cached row scan to expose the best opaque run while still returning
 * PARTIAL when only translucent coverage exists.
 */
static int egui_mask_image_get_row_range(egui_mask_t *self, egui_dim_t y, egui_dim_t x_min, egui_dim_t x_max, egui_dim_t *x_start, egui_dim_t *x_end)
{
    EGUI_LOCAL_INIT(egui_mask_image_t);
    egui_dim_t local_y;
    egui_dim_t src_y;

    (void)self;

    egui_mask_image_refresh_cache(local);
    if (local->img == NULL)
    {
        *x_start = x_min;
        *x_end = x_max;
        return EGUI_MASK_ROW_INSIDE;
    }

    if (y < local->cached_y || y >= local->cached_y_end)
    {
        return EGUI_MASK_ROW_OUTSIDE;
    }

    local_y = y - local->cached_y;
    src_y = (egui_dim_t)EGUI_FLOAT_MULT(local_y, local->height_radio);
    if (src_y < 0 || src_y >= local->src_height)
    {
        return EGUI_MASK_ROW_OUTSIDE;
    }

    if (local->alpha_buf == NULL)
    {
        *x_start = EGUI_MAX(x_min, local->cached_x);
        *x_end = EGUI_MIN(x_max, local->cached_x_end);
        if (*x_start >= *x_end)
        {
            return EGUI_MASK_ROW_OUTSIDE;
        }
        return (*x_start <= x_min && *x_end >= x_max) ? EGUI_MASK_ROW_INSIDE : EGUI_MASK_ROW_PARTIAL;
    }

    if (egui_mask_image_refresh_row_scan(local, y))
    {
        if (!local->row_cached_has_visible)
        {
            return EGUI_MASK_ROW_OUTSIDE;
        }

        if (!local->row_cached_has_opaque)
        {
            *x_start = x_min;
            *x_end = x_min;
            return EGUI_MASK_ROW_PARTIAL;
        }

        *x_start = EGUI_MAX(local->row_opaque_x_start, x_min);
        *x_end = EGUI_MIN(local->row_opaque_x_end, x_max);
        if (*x_start >= *x_end)
        {
            *x_start = x_min;
            *x_end = x_min;
            return EGUI_MASK_ROW_PARTIAL;
        }

        if (*x_start <= x_min && *x_end >= x_max)
        {
            return EGUI_MASK_ROW_INSIDE;
        }

        return EGUI_MASK_ROW_PARTIAL;
    }

    *x_start = x_min;
    *x_end = x_min;
    if (x_max <= local->cached_x || x_min >= local->cached_x_end)
    {
        return EGUI_MASK_ROW_OUTSIDE;
    }
    return EGUI_MASK_ROW_PARTIAL;
}

/**
 * @brief Report the broad visible horizontal span for one mask row.
 *
 * This includes all non-zero coverage, not just the fully opaque run, so
 * callers
 * can use it as a cheap clipping bound before per-pixel work.
 */
static int egui_mask_image_get_row_visible_range(egui_mask_t *self, egui_dim_t y, egui_dim_t x_min, egui_dim_t x_max, egui_dim_t *x_start, egui_dim_t *x_end)
{
    EGUI_LOCAL_INIT(egui_mask_image_t);
    egui_dim_t local_y;
    egui_dim_t src_y;

    (void)self;

    egui_mask_image_refresh_cache(local);
    if (local->img == NULL)
    {
        *x_start = x_min;
        *x_end = x_max;
        return (*x_start < *x_end);
    }

    if (y < local->cached_y || y >= local->cached_y_end)
    {
        return 0;
    }

    local_y = y - local->cached_y;
    src_y = (egui_dim_t)EGUI_FLOAT_MULT(local_y, local->height_radio);
    if (src_y < 0 || src_y >= local->src_height)
    {
        return 0;
    }

    if (local->alpha_buf == NULL)
    {
        *x_start = EGUI_MAX(x_min, local->cached_x);
        *x_end = EGUI_MIN(x_max, local->cached_x_end);
        return (*x_start < *x_end);
    }

    if (egui_mask_image_refresh_row_scan(local, y))
    {
        if (!local->row_cached_has_visible)
        {
            return 0;
        }

        *x_start = EGUI_MAX(local->row_visible_x_start, x_min);
        *x_end = EGUI_MIN(local->row_visible_x_end, x_max);
        return (*x_start < *x_end);
    }

    *x_start = EGUI_MAX(x_min, local->cached_x);
    *x_end = EGUI_MIN(x_max, local->cached_x_end);
    return (*x_start < *x_end);
}

const egui_mask_api_t egui_mask_image_t_api_table = {
        .kind = EGUI_MASK_KIND_IMAGE,
        .mask_point = egui_mask_image_mask_point,
        .mask_get_row_range = egui_mask_image_get_row_range,
        .mask_get_row_visible_range = egui_mask_image_get_row_visible_range,
        .mask_blend_row_color = NULL,
        .mask_get_row_overlay = NULL,
};

/**
 * @brief Initialize the image mask and clear all derived cache state.
 *
 * The actual image and scaling information are attached later, so init mostly
 *
 * wires the API table and resets every cached field to an empty sentinel value.
 */
void egui_mask_image_init(egui_mask_t *self)
{
    EGUI_LOCAL_INIT(egui_mask_image_t);
    egui_mask_init(self);
    self->api = &egui_mask_image_t_api_table;

    local->img = NULL;
    local->cached_img = NULL;
    local->width_radio = 0;
    local->height_radio = 0;
    local->alpha_buf = NULL;
    local->src_width = 0;
    local->src_height = 0;
    local->cached_x = -1;
    local->cached_y = -1;
    local->cached_width = -1;
    local->cached_height = -1;
    local->cached_x_end = -1;
    local->cached_y_end = -1;
    local->alpha_type = 0;
    local->res_type = EGUI_RESOURCE_TYPE_INTERNAL;
    egui_mask_image_invalidate_row_cache(local);
}

#endif
