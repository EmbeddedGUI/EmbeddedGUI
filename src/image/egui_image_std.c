#include <stdio.h>
#include <assert.h>

#include "egui_image_std.h"
#include "core/egui_api.h"

const uint8_t egui_image_data_type_size_table[] = {
        4, /* EGUI_IMAGE_DATA_TYPE_RGB32 */
        2, /* EGUI_IMAGE_DATA_TYPE_RGB565 */
        1, /* EGUI_IMAGE_DATA_TYPE_GRAY8 */
        0, /* EGUI_IMAGE_DATA_TYPE_ALPHA */
};

const uint8_t egui_image_alpha_type_size_table[] = {
        1, /* EGUI_IMAGE_ALPHA_TYPE_1 */
        2, /* EGUI_IMAGE_ALPHA_TYPE_2 */
        4, /* EGUI_IMAGE_ALPHA_TYPE_4 */
        8, /* EGUI_IMAGE_ALPHA_TYPE_8 */
};

typedef void(egui_image_std_get_pixel)(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha);

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
typedef void(egui_image_std_get_col_pixel_with_alpha)(const uint16_t *p_data, const uint8_t *p_alpha, egui_dim_t col_index, egui_color_t *color,
                                                      egui_alpha_t *alpha);

typedef void(egui_image_std_blend_mapped_row_func)(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                   const egui_dim_t *src_x_map, egui_dim_t count, egui_alpha_t canvas_alpha);

#define EGUI_IMAGE_STD_EXTERNAL_DATA_CACHE_SIZE  2048
#define EGUI_IMAGE_STD_EXTERNAL_ALPHA_CACHE_SIZE 1024

static uint8_t g_external_image_data_cache[EGUI_IMAGE_STD_EXTERNAL_DATA_CACHE_SIZE];
static uint8_t g_external_image_alpha_cache[EGUI_IMAGE_STD_EXTERNAL_ALPHA_CACHE_SIZE];

static void *egui_image_std_acquire_external_buffer(uint32_t size, uint8_t *static_buf, uint32_t static_size)
{
    if (size <= static_size)
    {
        return static_buf;
    }

    void *buf = egui_malloc(size);
    if (buf == NULL)
    {
        EGUI_ASSERT(0);
    }
    return buf;
}

static void egui_image_std_release_external_buffer(void *buf, uint8_t *static_buf)
{
    if (buf != NULL && buf != static_buf)
    {
        egui_free(buf);
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb32(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    uint32_t sel_color = ((uint32_t *)image->data_buf)[x + y * image->width];
    color->full = EGUI_COLOR_RGB888_TRANS(sel_color);
    *alpha = (sel_color >> 24) & 0xFF;
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_pixel_rgb565_limit(const uint16_t *p_data, egui_dim_t col_index, egui_color_t *color)
{
    uint32_t sel_pos = col_index;
    uint16_t sel_color = p_data[sel_pos];

    color->full = EGUI_COLOR_RGB565_TRANS(sel_color);
}

__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb565_limit(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color)
{
    uint32_t row_start = y * image->width;
    const uint16_t *p_data = image->data_buf;

    egui_image_std_get_col_pixel_rgb565_limit(&p_data[row_start], x, color);
}

__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb565(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    egui_image_std_get_pixel_rgb565_limit(image, x, y, color);
    *alpha = EGUI_ALPHA_100;
}

// __EGUI_STATIC_INLINE__ void egui_image_std_get_col_pixel_rgb565(const uint16_t *p_data, const uint8_t *p_alpha, egui_dim_t col_index, egui_color_t *color,
// egui_alpha_t *alpha)
// {
//     egui_image_std_get_col_pixel_rgb565_limit(p_data, col_index, color);
//     *alpha = EGUI_ALPHA_100;
// }
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_pixel_rgb565_8(const uint16_t *p_data, const uint8_t *p_alpha, egui_dim_t col_index, egui_color_t *color,
                                                                  egui_alpha_t *alpha)
{
    uint32_t sel_pos = col_index;
    uint16_t sel_color = p_data[sel_pos];
    uint8_t sel_alpha = p_alpha[sel_pos];

    color->full = EGUI_COLOR_RGB565_TRANS(sel_color);
    *alpha = sel_alpha;
}

__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb565_8(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color,
                                                              egui_alpha_t *alpha)
{
    uint32_t row_start = y * image->width;
    const uint16_t *p_data = image->data_buf;
    const uint8_t *p_alpha = image->alpha_buf;

    egui_image_std_get_col_pixel_rgb565_8(&p_data[row_start], &p_alpha[row_start], x, color, alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_pixel_rgb565_4(const uint16_t *p_data, const uint8_t *p_alpha, egui_dim_t col_index, egui_color_t *color,
                                                                  egui_alpha_t *alpha)
{
    uint8_t bit_pos;
    uint32_t sel_pos = col_index;
    // get alpha row position.
    uint32_t sel_alpha_pos = col_index >> 1; // same to: x / 2
    uint16_t sel_color = p_data[sel_pos];
    uint8_t sel_alpha;

    bit_pos = col_index & 0x01; // 0x01
    bit_pos = bit_pos << 2;     // same to: bit_pos * 4

    sel_alpha = ((p_alpha[sel_alpha_pos]) >> bit_pos) & 0x0F;

    color->full = EGUI_COLOR_RGB565_TRANS(sel_color);
    *alpha = egui_alpha_change_table_4[sel_alpha];
}

__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb565_4(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color,
                                                              egui_alpha_t *alpha)
{
    uint32_t row_start = y * image->width;
    uint32_t row_start_alpha = y * ((image->width + 1) >> 1); // same to: ((image->width + 1) / 2);
    const uint16_t *p_data = image->data_buf;
    const uint8_t *p_alpha = image->alpha_buf;

    egui_image_std_get_col_pixel_rgb565_4(&p_data[row_start], &p_alpha[row_start_alpha], x, color, alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_pixel_rgb565_2(const uint16_t *p_data, const uint8_t *p_alpha, egui_dim_t col_index, egui_color_t *color,
                                                                  egui_alpha_t *alpha)
{
    uint8_t bit_pos;
    uint32_t sel_pos = col_index;
    // get alpha row position.
    uint32_t sel_alpha_pos = col_index >> 2; // same to: x / 4
    uint16_t sel_color = p_data[sel_pos];
    uint8_t sel_alpha;

    bit_pos = col_index & 0x03; // 0x03
    bit_pos = bit_pos << 1;     // same to: bit_pos * 2

    sel_alpha = ((p_alpha[sel_alpha_pos]) >> bit_pos) & 0x03;

    color->full = EGUI_COLOR_RGB565_TRANS(sel_color);
    *alpha = egui_alpha_change_table_2[sel_alpha];
}

__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb565_2(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color,
                                                              egui_alpha_t *alpha)
{
    uint32_t row_start = y * image->width;
    uint32_t row_start_alpha = y * ((image->width + 3) >> 2); // same to: ((image->width + 3) / 4);
    const uint16_t *p_data = image->data_buf;
    const uint8_t *p_alpha = image->alpha_buf;

    egui_image_std_get_col_pixel_rgb565_2(&p_data[row_start], &p_alpha[row_start_alpha], x, color, alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_pixel_rgb565_1(const uint16_t *p_data, const uint8_t *p_alpha, egui_dim_t col_index, egui_color_t *color,
                                                                  egui_alpha_t *alpha)
{
    uint8_t bit_pos;
    uint32_t sel_pos = col_index;
    // get alpha row position.
    uint32_t sel_alpha_pos = col_index >> 3; // same to x / 8
    uint16_t sel_color = p_data[sel_pos];
    uint8_t sel_alpha;

    bit_pos = col_index & 0x07; // 0x07

    sel_alpha = ((p_alpha[sel_alpha_pos]) >> bit_pos) & 0x01;

    color->full = EGUI_COLOR_RGB565_TRANS(sel_color);
    *alpha = (sel_alpha ? 0xff : 0x00);
}

__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb565_1(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color,
                                                              egui_alpha_t *alpha)
{
    uint32_t row_start = y * image->width;
    uint32_t row_start_alpha = y * ((image->width + 7) >> 3); // same to ((image->width + 7) / 8);
    const uint16_t *p_data = image->data_buf;
    const uint8_t *p_alpha = image->alpha_buf;

    egui_image_std_get_col_pixel_rgb565_1(&p_data[row_start], &p_alpha[row_start_alpha], x, color, alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1

__EGUI_STATIC_INLINE__ egui_dim_t egui_image_std_prepare_resize_src_x_map(egui_dim_t *src_x_map, egui_dim_t x, egui_dim_t x_total, egui_float_t width_radio)
{
    egui_dim_t count = x_total - x;

    EGUI_ASSERT(count <= EGUI_CONFIG_PFB_WIDTH);
    for (egui_dim_t i = 0; i < count; i++)
    {
        src_x_map[i] = (egui_dim_t)EGUI_FLOAT_MULT(x + i, width_radio);
    }

    return count;
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_image_std_prepare_resize_src_x_map_limit(egui_dim_t *src_x_map, egui_dim_t x, egui_dim_t x_total,
                                                                                egui_float_t width_radio)
{
    egui_dim_t count = x_total - x;

    EGUI_ASSERT(count <= EGUI_CONFIG_PFB_WIDTH);
    for (egui_dim_t i = 0; i < count; i++)
    {
        src_x_map[i] = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(x + i, width_radio);
    }

    return count;
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_resize_pixel(egui_color_int_t *dst, egui_color_t color, egui_alpha_t alpha)
{
    if (alpha == 0)
    {
        return;
    }

    if (alpha == EGUI_ALPHA_100)
    {
        *dst = color.full;
    }
    else
    {
        egui_color_t *back_color = (egui_color_t *)dst;
        egui_rgb_mix_ptr(back_color, &color, back_color, alpha);
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_copy_rgb565_row(egui_color_int_t *dst_row, const uint16_t *src_row, egui_dim_t count)
{
    const uint32_t *src32 = (const uint32_t *)src_row;
    uint32_t *dst32 = (uint32_t *)dst_row;
    egui_dim_t n32 = count >> 1;

    for (egui_dim_t i = 0; i < n32; i++)
    {
        dst32[i] = src32[i];
    }

    if (count & 1)
    {
        dst_row[count - 1] = src_row[count - 1];
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_row(egui_color_int_t *dst_row, const uint16_t *src_row, egui_dim_t count, egui_alpha_t alpha)
{
    if (alpha == 0)
    {
        return;
    }

    if (alpha == EGUI_ALPHA_100)
    {
        egui_image_std_copy_rgb565_row(dst_row, src_row, count);
        return;
    }

    for (egui_dim_t i = 0; i < count; i++)
    {
        egui_color_t color;
        color.full = EGUI_COLOR_RGB565_TRANS(src_row[i]);
        egui_image_std_blend_resize_pixel(&dst_row[i], color, alpha);
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_mapped_row(egui_color_int_t *dst_row, const uint16_t *src_row, const egui_dim_t *src_x_map,
                                                                   egui_dim_t count, egui_alpha_t alpha)
{
    if (alpha == 0)
    {
        return;
    }

    if (alpha == EGUI_ALPHA_100)
    {
        for (egui_dim_t i = 0; i < count; i++)
        {
            dst_row[i] = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
        }
        return;
    }

    for (egui_dim_t i = 0; i < count; i++)
    {
        egui_color_t color;
        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
        egui_image_std_blend_resize_pixel(&dst_row[i], color, alpha);
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha8_row(egui_color_int_t *dst_row, const uint16_t *src_pixels, const uint8_t *src_alpha_row,
                                                                   egui_dim_t count, egui_alpha_t canvas_alpha)
{
    egui_dim_t i = 0;

    while (i < count)
    {
        // Word-level transparent skip (4 bytes at a time)
        while (i + 4 <= count && *(const uint32_t *)&src_alpha_row[i] == 0x00000000)
        {
            i += 4;
        }
        while (i < count && src_alpha_row[i] == 0)
        {
            i++;
        }

        // Word-level opaque run detection (4 bytes at a time)
        egui_dim_t opaque_start = i;
        while (i + 4 <= count && *(const uint32_t *)&src_alpha_row[i] == 0xFFFFFFFF)
        {
            i += 4;
        }
        while (i < count && src_alpha_row[i] == EGUI_ALPHA_100)
        {
            i++;
        }

        if (opaque_start < i)
        {
            egui_image_std_blend_rgb565_row(&dst_row[opaque_start], &src_pixels[opaque_start], i - opaque_start, canvas_alpha);
            continue;
        }

        if (i < count)
        {
            egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, src_alpha_row[i]);
            if (alpha != 0)
            {
                egui_color_t color;
                color.full = EGUI_COLOR_RGB565_TRANS(src_pixels[i]);
                egui_image_std_blend_resize_pixel(&dst_row[i], color, alpha);
            }
            i++;
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha8_mapped_row(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                                          const egui_dim_t *src_x_map, egui_dim_t count, egui_alpha_t canvas_alpha)
{
    egui_dim_t i = 0;

    while (i < count)
    {
        while (i < count && src_alpha_row[src_x_map[i]] == 0)
        {
            i++;
        }

        egui_dim_t opaque_start = i;
        while (i < count && src_alpha_row[src_x_map[i]] == EGUI_ALPHA_100)
        {
            i++;
        }

        if (opaque_start < i)
        {
            egui_image_std_blend_rgb565_mapped_row(&dst_row[opaque_start], src_row, &src_x_map[opaque_start], i - opaque_start, canvas_alpha);
            continue;
        }

        if (i < count)
        {
            egui_dim_t src_x = src_x_map[i];
            egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, src_alpha_row[src_x]);
            if (alpha != 0)
            {
                egui_color_t color;
                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                egui_image_std_blend_resize_pixel(&dst_row[i], color, alpha);
            }
            i++;
        }
    }
}

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
__EGUI_STATIC_INLINE__ egui_alpha_t egui_image_std_get_alpha_rgb565_4_row(const uint8_t *src_alpha_row, egui_dim_t src_x)
{
    uint8_t packed = src_alpha_row[src_x >> 1];
    uint8_t shift = (src_x & 0x01) << 2;

    return egui_alpha_change_table_4[(packed >> shift) & 0x0F];
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha4_mapped_row(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                                          const egui_dim_t *src_x_map, egui_dim_t count, egui_alpha_t canvas_alpha)
{
    egui_dim_t i = 0;

    while (i < count)
    {
        while (i < count && egui_image_std_get_alpha_rgb565_4_row(src_alpha_row, src_x_map[i]) == 0)
        {
            i++;
        }

        egui_dim_t opaque_start = i;
        while (i < count && egui_image_std_get_alpha_rgb565_4_row(src_alpha_row, src_x_map[i]) == EGUI_ALPHA_100)
        {
            i++;
        }

        if (opaque_start < i)
        {
            egui_image_std_blend_rgb565_mapped_row(&dst_row[opaque_start], src_row, &src_x_map[opaque_start], i - opaque_start, canvas_alpha);
            continue;
        }

        if (i < count)
        {
            egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, egui_image_std_get_alpha_rgb565_4_row(src_alpha_row, src_x_map[i]));
            if (alpha != 0)
            {
                egui_color_t color;
                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                egui_image_std_blend_resize_pixel(&dst_row[i], color, alpha);
            }
            i++;
        }
    }
}

// Row-level blend with opaque-run batch copy for 4-bit alpha.
// Typical images have opaque interiors: 0xFF byte = 2 pixels both fully opaque.
__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha4_row(egui_color_int_t *dst_row, const uint16_t *src_pixels, const uint8_t *alpha_buf,
                                                                   egui_dim_t start_col, egui_dim_t count, egui_alpha_t canvas_alpha)
{
    egui_dim_t end_col = start_col + count;
    egui_dim_t col = start_col;
    egui_dim_t dst_i = 0;

    // Handle leading unaligned pixel (if start_col is odd)
    if ((col & 1) && col < end_col)
    {
        egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, egui_image_std_get_alpha_rgb565_4_row(alpha_buf, col));
        if (alpha != 0)
        {
            egui_color_t c;
            c.full = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
            egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, alpha);
        }
        col++;
        dst_i++;
    }

    // Process byte-aligned ranges (2 pixels per byte, 0xFF = both fully opaque)
    while (col + 2 <= end_col)
    {
        uint8_t ab = alpha_buf[col >> 1];
        if (ab == 0xFF)
        {
            // Both pixels fully opaque - find run of consecutive all-opaque bytes
            egui_dim_t run_start_col = col;
            egui_dim_t run_start_dst = dst_i;
            do
            {
                col += 2;
                dst_i += 2;
            } while (col + 2 <= end_col && alpha_buf[col >> 1] == 0xFF);
            // Batch copy entire opaque run
            egui_image_std_blend_rgb565_row(&dst_row[run_start_dst], &src_pixels[run_start_col], col - run_start_col, canvas_alpha);
        }
        else if (ab == 0x00)
        {
            // Both transparent - skip
            col += 2;
            dst_i += 2;
        }
        else
        {
            // Mixed byte - process individual pixels
            for (int p = 0; p < 2; p++)
            {
                egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, egui_image_std_get_alpha_rgb565_4_row(alpha_buf, col));
                if (alpha != 0)
                {
                    egui_color_t c;
                    c.full = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
                    egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, alpha);
                }
                col++;
                dst_i++;
            }
        }
    }

    // Handle trailing pixel
    if (col < end_col)
    {
        egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, egui_image_std_get_alpha_rgb565_4_row(alpha_buf, col));
        if (alpha != 0)
        {
            egui_color_t c;
            c.full = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
            egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, alpha);
        }
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
__EGUI_STATIC_INLINE__ egui_alpha_t egui_image_std_get_alpha_rgb565_2_row(const uint8_t *src_alpha_row, egui_dim_t src_x)
{
    uint8_t packed = src_alpha_row[src_x >> 2];
    uint8_t shift = (src_x & 0x03) << 1;

    return egui_alpha_change_table_2[(packed >> shift) & 0x03];
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha2_mapped_row(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                                          const egui_dim_t *src_x_map, egui_dim_t count, egui_alpha_t canvas_alpha)
{
    egui_dim_t i = 0;

    while (i < count)
    {
        while (i < count && egui_image_std_get_alpha_rgb565_2_row(src_alpha_row, src_x_map[i]) == 0)
        {
            i++;
        }

        egui_dim_t opaque_start = i;
        while (i < count && egui_image_std_get_alpha_rgb565_2_row(src_alpha_row, src_x_map[i]) == EGUI_ALPHA_100)
        {
            i++;
        }

        if (opaque_start < i)
        {
            egui_image_std_blend_rgb565_mapped_row(&dst_row[opaque_start], src_row, &src_x_map[opaque_start], i - opaque_start, canvas_alpha);
            continue;
        }

        if (i < count)
        {
            egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, egui_image_std_get_alpha_rgb565_2_row(src_alpha_row, src_x_map[i]));
            if (alpha != 0)
            {
                egui_color_t color;
                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                egui_image_std_blend_resize_pixel(&dst_row[i], color, alpha);
            }
            i++;
        }
    }
}

// Row-level blend with opaque-run batch copy for 2-bit alpha.
// Typical images have opaque interiors: 0xFF byte = 4 pixels all fully opaque.
__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha2_row(egui_color_int_t *dst_row, const uint16_t *src_pixels, const uint8_t *alpha_buf,
                                                                   egui_dim_t start_col, egui_dim_t count, egui_alpha_t canvas_alpha)
{
    egui_dim_t end_col = start_col + count;
    egui_dim_t col = start_col;
    egui_dim_t dst_i = 0;

    // Handle leading unaligned pixels until 4-pixel (byte) boundary
    while (col < end_col && (col & 3))
    {
        egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, egui_image_std_get_alpha_rgb565_2_row(alpha_buf, col));
        if (alpha != 0)
        {
            egui_color_t c;
            c.full = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
            egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, alpha);
        }
        col++;
        dst_i++;
    }

    // Process byte-aligned ranges (4 pixels per byte, 0xFF = all fully opaque)
    while (col + 4 <= end_col)
    {
        uint8_t ab = alpha_buf[col >> 2];
        if (ab == 0xFF)
        {
            // All 4 pixels fully opaque - find run of consecutive all-opaque bytes
            egui_dim_t run_start_col = col;
            egui_dim_t run_start_dst = dst_i;
            do
            {
                col += 4;
                dst_i += 4;
            } while (col + 4 <= end_col && alpha_buf[col >> 2] == 0xFF);
            // Batch copy entire opaque run
            egui_image_std_blend_rgb565_row(&dst_row[run_start_dst], &src_pixels[run_start_col], col - run_start_col, canvas_alpha);
        }
        else if (ab == 0x00)
        {
            // All transparent - skip
            col += 4;
            dst_i += 4;
        }
        else
        {
            // Mixed byte - process individual pixels
            for (int p = 0; p < 4; p++)
            {
                egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, egui_image_std_get_alpha_rgb565_2_row(alpha_buf, col));
                if (alpha != 0)
                {
                    egui_color_t c;
                    c.full = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
                    egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, alpha);
                }
                col++;
                dst_i++;
            }
        }
    }

    // Handle trailing pixels
    while (col < end_col)
    {
        egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, egui_image_std_get_alpha_rgb565_2_row(alpha_buf, col));
        if (alpha != 0)
        {
            egui_color_t c;
            c.full = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
            egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, alpha);
        }
        col++;
        dst_i++;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
__EGUI_STATIC_INLINE__ egui_alpha_t egui_image_std_get_alpha_rgb565_1_row(const uint8_t *src_alpha_row, egui_dim_t src_x)
{
    return ((src_alpha_row[src_x >> 3] >> (src_x & 0x07)) & 0x01) ? EGUI_ALPHA_100 : 0;
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha1_mapped_row(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                                          const egui_dim_t *src_x_map, egui_dim_t count, egui_alpha_t canvas_alpha)
{
    egui_dim_t i = 0;

    while (i < count)
    {
        while (i < count && egui_image_std_get_alpha_rgb565_1_row(src_alpha_row, src_x_map[i]) == 0)
        {
            i++;
        }

        egui_dim_t opaque_start = i;
        while (i < count && egui_image_std_get_alpha_rgb565_1_row(src_alpha_row, src_x_map[i]) == EGUI_ALPHA_100)
        {
            i++;
        }

        if (opaque_start < i)
        {
            egui_image_std_blend_rgb565_mapped_row(&dst_row[opaque_start], src_row, &src_x_map[opaque_start], i - opaque_start, canvas_alpha);
            continue;
        }

        if (i < count)
        {
            egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, egui_image_std_get_alpha_rgb565_1_row(src_alpha_row, src_x_map[i]));
            if (alpha != 0)
            {
                egui_color_t color;
                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                egui_image_std_blend_resize_pixel(&dst_row[i], color, alpha);
            }
            i++;
        }
    }
}

// Row-level blend with opaque-run batch copy for 1-bit alpha.
// Typical images have opaque interiors: 0xFF byte = 8 pixels all fully opaque.
__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha1_row(egui_color_int_t *dst_row, const uint16_t *src_pixels, const uint8_t *alpha_buf,
                                                                   egui_dim_t start_col, egui_dim_t count, egui_alpha_t canvas_alpha)
{
    egui_dim_t end_col = start_col + count;
    egui_dim_t col = start_col;
    egui_dim_t dst_i = 0;

    // Handle leading unaligned bits until 8-pixel (byte) boundary
    while (col < end_col && (col & 7))
    {
        if ((alpha_buf[col >> 3] >> (col & 7)) & 1)
        {
            egui_color_t c;
            c.full = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
            egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, canvas_alpha);
        }
        col++;
        dst_i++;
    }

    // Process byte-aligned ranges (8 pixels per byte, 0xFF = all fully opaque)
    while (col + 8 <= end_col)
    {
        uint8_t ab = alpha_buf[col >> 3];
        if (ab == 0xFF)
        {
            // All 8 pixels fully opaque - find run of consecutive all-opaque bytes
            egui_dim_t run_start_col = col;
            egui_dim_t run_start_dst = dst_i;
            do
            {
                col += 8;
                dst_i += 8;
            } while (col + 8 <= end_col && alpha_buf[col >> 3] == 0xFF);
            // Batch copy entire opaque run
            egui_image_std_blend_rgb565_row(&dst_row[run_start_dst], &src_pixels[run_start_col], col - run_start_col, canvas_alpha);
        }
        else if (ab == 0x00)
        {
            // All 8 pixels transparent - skip
            col += 8;
            dst_i += 8;
        }
        else
        {
            // Mixed byte - process individual bits
            for (int bit = 0; bit < 8; bit++)
            {
                if ((ab >> bit) & 1)
                {
                    egui_color_t c;
                    c.full = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
                    egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, canvas_alpha);
                }
                col++;
                dst_i++;
            }
        }
    }

    // Handle trailing unaligned bits
    while (col < end_col)
    {
        if ((alpha_buf[col >> 3] >> (col & 7)) & 1)
        {
            egui_color_t c;
            c.full = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
            egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, canvas_alpha);
        }
        col++;
        dst_i++;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_alpha_8(const uint8_t *p_data, egui_dim_t col_index, egui_alpha_t *alpha)
{
    *alpha = p_data[col_index];
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_alpha_4(const uint8_t *p_data, egui_dim_t col_index, egui_alpha_t *alpha)
{
    uint8_t bit_pos;
    uint32_t sel_pos = col_index >> 1; // same to: col_index / 2
    uint8_t sel_alpha;

    bit_pos = col_index & 0x01; // 0x01
    bit_pos = bit_pos << 2;     // same to: bit_pos * 4

    sel_alpha = ((p_data[sel_pos]) >> bit_pos) & 0x0F;

    *alpha = egui_alpha_change_table_4[sel_alpha];
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_alpha_2(const uint8_t *p_data, egui_dim_t col_index, egui_alpha_t *alpha)
{
    uint8_t bit_pos;
    uint32_t sel_pos = col_index >> 2; // same to: col_index / 4
    uint8_t sel_alpha;

    bit_pos = col_index & 0x03; // 0x03
    bit_pos = bit_pos << 1;     // same to: bit_pos * 2

    sel_alpha = ((p_data[sel_pos]) >> bit_pos) & 0x03;

    *alpha = egui_alpha_change_table_2[sel_alpha];
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_alpha_1(const uint8_t *p_data, egui_dim_t col_index, egui_alpha_t *alpha)
{
    uint8_t bit_pos;
    uint32_t sel_pos = col_index >> 3; // same to col_index / 8
    uint8_t sel_alpha;

    bit_pos = col_index & 0x07; // 0x07

    sel_alpha = ((p_data[sel_pos]) >> bit_pos) & 0x01;

    *alpha = (sel_alpha ? 0xff : 0x00);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1

egui_image_std_get_pixel *egui_image_get_point_func(const egui_image_t *self)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    // select get_pixel function.
    egui_image_std_get_pixel *get_pixel = NULL;
    if (image)
    {
        switch (image->data_type)
        {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
        case EGUI_IMAGE_DATA_TYPE_RGB32:
            get_pixel = egui_image_std_get_pixel_rgb32;
            break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
        case EGUI_IMAGE_DATA_TYPE_RGB565:
            if (image->alpha_buf == NULL)
            {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
                get_pixel = egui_image_std_get_pixel_rgb565;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
            }
            else
            {
                switch (image->alpha_type)
                {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
                case EGUI_IMAGE_ALPHA_TYPE_1:
                    get_pixel = egui_image_std_get_pixel_rgb565_1;
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
                case EGUI_IMAGE_ALPHA_TYPE_2:
                    get_pixel = egui_image_std_get_pixel_rgb565_2;
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
                case EGUI_IMAGE_ALPHA_TYPE_4:
                    get_pixel = egui_image_std_get_pixel_rgb565_4;
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
                case EGUI_IMAGE_ALPHA_TYPE_8:
                    get_pixel = egui_image_std_get_pixel_rgb565_8;
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
                default:
                    EGUI_ASSERT(0);
                    break;
                }
            }
            break;
        default:
            EGUI_ASSERT(0);
            break;
        }
    }

    return get_pixel;
}

int egui_image_std_get_point(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_image_std_get_pixel *get_pixel = egui_image_get_point_func(self);

    if ((x >= image->width) || (y >= image->height))
    {
        return 0;
    }

    get_pixel(image, x, y, color, alpha);

    return 1;
}

int egui_image_std_get_point_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t *color,
                                    egui_alpha_t *alpha)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    if (width == 0 || height == 0)
    {
        return 0;
    }
    egui_float_t width_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->width), EGUI_FLOAT_VALUE_INT(width));
    egui_float_t height_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->height), EGUI_FLOAT_VALUE_INT(height));
    egui_dim_t src_x;
    egui_dim_t src_y;

    if ((x >= width) || (y >= height))
    {
        return 0;
    }

    // select get_pixel function.
    egui_image_std_get_pixel *get_pixel = egui_image_get_point_func(self);

    // For speed, use nearestScaler to scale the image.
    src_x = (egui_dim_t)EGUI_FLOAT_MULT(x, width_radio);
    src_y = (egui_dim_t)EGUI_FLOAT_MULT(y, height_radio);

    get_pixel(image, src_x, src_y, color, alpha);

    return 1;
}

void egui_image_std_load_data_resource(void *dest, egui_image_std_info_t *image, uint32_t start_offset, uint32_t size)
{
    // EGUI_LOG_INF("egui_image_std_load_data_resource, start_offset: %d, size: %d\n", start_offset, size);
    if (image->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
    {
        egui_memcpy(dest, (const void *)((const uint8_t *)image->data_buf + start_offset), size);
    }
    else
    {
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        egui_api_load_external_resource(dest, (egui_uintptr_t)(image->data_buf), start_offset, size);
#else
        EGUI_ASSERT(0);
#endif
    }

    // EGUI_LOG_INF("egui_image_std_load_data_resource, data: %08x:%08x:%08x:%08x:%08x:%08x:%08x:%08x\n", ((uint32_t *)dest)[0], ((uint32_t *)dest)[1],
    // ((uint32_t *)dest)[2], ((uint32_t *)dest)[3], ((uint32_t *)dest)[4], ((uint32_t *)dest)[5], ((uint32_t *)dest)[6], ((uint32_t *)dest)[7]);
}

void egui_image_std_load_alpha_resource(void *dest, egui_image_std_info_t *image, uint32_t start_offset, uint32_t size)
{
    // EGUI_LOG_INF("egui_image_std_load_alpha_resource, start_offset: %d, size: %d\n", start_offset, size);
    if (image->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
    {
        egui_memcpy(dest, (const void *)((const uint8_t *)image->alpha_buf + start_offset), size);
    }
    else
    {
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        egui_api_load_external_resource(dest, (egui_uintptr_t)(image->alpha_buf), start_offset, size);
#else
        EGUI_ASSERT(0);
#endif
    }
    // EGUI_LOG_INF("egui_image_std_load_alpha_resource, data: %08x:%08x:%08x:%08x:%08x:%08x:%08x:%08x\n", ((uint32_t *)dest)[0], ((uint32_t *)dest)[1],
    // ((uint32_t *)dest)[2], ((uint32_t *)dest)[3], ((uint32_t *)dest)[4], ((uint32_t *)dest)[5], ((uint32_t *)dest)[6], ((uint32_t *)dest)[7]);
}

#if EGUI_CONFIG_REDUCE_IMAGE_CODE_SIZE
void egui_image_std_draw_image(const egui_image_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_color_t color;
    egui_alpha_t alpha;
    egui_dim_t width = image->width;
    egui_dim_t height = image->height;
    const uint32_t *p_data = image->data_buf;
    // const uint8_t* p_alpha = image->alpha_buf;

    egui_dim_t x_total;
    egui_dim_t y_total;

    // select get_pixel function.
    egui_image_std_get_pixel *get_pixel = egui_image_get_point_func(self);

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, egui_canvas_get_base_view_work_region(), &region);
    if (egui_region_is_empty(&region))
    {
        return;
    }

    // change to image coordinate.
    region.location.x -= x;
    region.location.y -= y;

    // for speed, calculate total positions outside of the loop
    x_total = region.location.x + region.size.width;
    y_total = region.location.y + region.size.height;

    if (canvas->mask != NULL)
    {
        if (canvas->mask->api->mask_get_row_range != NULL)
        {
            egui_dim_t x_start, x_end;
            for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)
            {
                egui_dim_t screen_y = y + y_;
                egui_dim_t screen_x_min = x + region.location.x;
                egui_dim_t screen_x_max = x + x_total;
                int result = canvas->mask->api->mask_get_row_range(canvas->mask, screen_y, screen_x_min, screen_x_max, &x_start, &x_end);
                if (result == EGUI_MASK_ROW_OUTSIDE)
                {
                    continue;
                }
                if (result == EGUI_MASK_ROW_INSIDE)
                {
                    for (egui_dim_t x_ = region.location.x; x_ < x_total; x_++)
                    {
                        get_pixel(image, x_, y_, &color, &alpha);
                        egui_canvas_draw_point_limit_skip_mask((x + x_), screen_y, color, alpha);
                    }
                }
                else // PARTIAL
                {
                    egui_dim_t img_x_start = x_start - x;
                    egui_dim_t img_x_end = x_end - x;
                    // Left edge: per-pixel with mask
                    for (egui_dim_t x_ = region.location.x; x_ < img_x_start; x_++)
                    {
                        get_pixel(image, x_, y_, &color, &alpha);
                        egui_canvas_draw_point_limit((x + x_), screen_y, color, alpha);
                    }
                    // Middle: skip mask
                    for (egui_dim_t x_ = img_x_start; x_ < img_x_end; x_++)
                    {
                        get_pixel(image, x_, y_, &color, &alpha);
                        egui_canvas_draw_point_limit_skip_mask((x + x_), screen_y, color, alpha);
                    }
                    // Right edge: per-pixel with mask
                    for (egui_dim_t x_ = img_x_end; x_ < x_total; x_++)
                    {
                        get_pixel(image, x_, y_, &color, &alpha);
                        egui_canvas_draw_point_limit((x + x_), screen_y, color, alpha);
                    }
                }
            }
        }
        else
        {
            for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)
            {
                for (egui_dim_t x_ = region.location.x; x_ < x_total; x_++)
                {
                    get_pixel(image, x_, y_, &color, &alpha);
                    egui_canvas_draw_point_limit((x + x_), (y + y_), color, alpha);
                }
            }
        }
    }
    else
    {
        for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)
        {
            for (egui_dim_t x_ = region.location.x; x_ < x_total; x_++)
            {
                get_pixel(image, x_, y_, &color, &alpha);

                // change to real position in canvas.
                egui_canvas_draw_point_limit_skip_mask((x + x_), (y + y_), color, alpha);
            }
        }
    }
}

void egui_image_std_draw_image_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    if (width == image->width && height == image->height)
    {
        egui_image_std_draw_image(self, x, y);
        return;
    }
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_color_t color;
    egui_alpha_t alpha;
    egui_float_t width_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->width), EGUI_FLOAT_VALUE_INT(width));
    egui_float_t height_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->height), EGUI_FLOAT_VALUE_INT(height));
    const uint32_t *p_data = image->data_buf;
    // const uint8_t* p_alpha = image->alpha_buf;
    egui_dim_t src_x;
    egui_dim_t src_y;

    egui_dim_t x_total;
    egui_dim_t y_total;

    // select get_pixel function.
    egui_image_std_get_pixel *get_pixel = egui_image_get_point_func(self);

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, egui_canvas_get_base_view_work_region(), &region);
    if (egui_region_is_empty(&region))
    {
        return;
    }

    // change to image coordinate.
    region.location.x -= x;
    region.location.y -= y;

    // for speed, calculate total positions outside of the loop
    x_total = region.location.x + region.size.width;
    y_total = region.location.y + region.size.height;

    if (canvas->mask != NULL)
    {
        if (canvas->mask->api->mask_get_row_range != NULL)
        {
            egui_dim_t rr_x_start, rr_x_end;
            for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)
            {
                egui_dim_t screen_y = y + y_;
                int rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, screen_y, x + region.location.x, x + x_total, &rr_x_start, &rr_x_end);
                if (rr_res == EGUI_MASK_ROW_OUTSIDE)
                    continue;
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                if (rr_res == EGUI_MASK_ROW_INSIDE)
                {
                    for (egui_dim_t x_ = region.location.x; x_ < x_total; x_++)
                    {
                        src_x = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(x_, width_radio);
                        get_pixel(image, src_x, src_y, &color, &alpha);
                        egui_canvas_draw_point_limit_skip_mask((x + x_), screen_y, color, alpha);
                    }
                }
                else
                {
                    egui_dim_t img_xs = rr_x_start - x;
                    egui_dim_t img_xe = rr_x_end - x;
                    for (egui_dim_t x_ = region.location.x; x_ < img_xs; x_++)
                    {
                        src_x = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(x_, width_radio);
                        get_pixel(image, src_x, src_y, &color, &alpha);
                        egui_canvas_draw_point_limit((x + x_), screen_y, color, alpha);
                    }
                    for (egui_dim_t x_ = img_xs; x_ < img_xe; x_++)
                    {
                        src_x = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(x_, width_radio);
                        get_pixel(image, src_x, src_y, &color, &alpha);
                        egui_canvas_draw_point_limit_skip_mask((x + x_), screen_y, color, alpha);
                    }
                    for (egui_dim_t x_ = img_xe; x_ < x_total; x_++)
                    {
                        src_x = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(x_, width_radio);
                        get_pixel(image, src_x, src_y, &color, &alpha);
                        egui_canvas_draw_point_limit((x + x_), screen_y, color, alpha);
                    }
                }
            }
        }
        else
        {
            for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)
            {
                for (egui_dim_t x_ = region.location.x; x_ < x_total; x_++)
                {
                    src_x = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(x_, width_radio);
                    src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                    get_pixel(image, src_x, src_y, &color, &alpha);
                    egui_canvas_draw_point_limit((x + x_), (y + y_), color, alpha);
                }
            }
        }
    }
    else
    {
        for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)
        {
            for (egui_dim_t x_ = region.location.x; x_ < x_total; x_++)
            {
                src_x = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(x_, width_radio);
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                get_pixel(image, src_x, src_y, &color, &alpha);
                egui_canvas_draw_point_limit_skip_mask((x + x_), (y + y_), color, alpha);
            }
        }
    }
}

#else // EGUI_CONFIG_REDUCE_IMAGE_CODE_SIZE

#define EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(_get_pixel_func, self, x, y, x_total, y_total, x_base, y_base)                                                   \
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;                                                                                         \
    egui_color_t color;                                                                                                                                        \
    egui_alpha_t alpha;                                                                                                                                        \
    egui_canvas_t *canvas = egui_canvas_get_canvas();                                                                                                          \
    if (canvas->mask != NULL)                                                                                                                                  \
    {                                                                                                                                                          \
        if (canvas->mask->api->mask_get_row_range != NULL)                                                                                                     \
        {                                                                                                                                                      \
            egui_dim_t rr_x_start, rr_x_end;                                                                                                                   \
            for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                        \
            {                                                                                                                                                  \
                egui_dim_t rr_sy = y_base + y_;                                                                                                                \
                int rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, x_base + x, x_base + x_total, &rr_x_start, &rr_x_end);                 \
                if (rr_res == EGUI_MASK_ROW_OUTSIDE)                                                                                                           \
                    continue;                                                                                                                                  \
                if (rr_res == EGUI_MASK_ROW_INSIDE)                                                                                                            \
                {                                                                                                                                              \
                    for (egui_dim_t x_ = x; x_ < x_total; x_++)                                                                                                \
                    {                                                                                                                                          \
                        _get_pixel_func(image, x_, y_, &color, &alpha);                                                                                        \
                        egui_canvas_draw_point_limit_skip_mask((x_base + x_), rr_sy, color, alpha);                                                            \
                    }                                                                                                                                          \
                }                                                                                                                                              \
                else                                                                                                                                           \
                {                                                                                                                                              \
                    egui_dim_t rr_img_xs = rr_x_start - x_base;                                                                                                \
                    egui_dim_t rr_img_xe = rr_x_end - x_base;                                                                                                  \
                    for (egui_dim_t x_ = x; x_ < rr_img_xs; x_++)                                                                                              \
                    {                                                                                                                                          \
                        _get_pixel_func(image, x_, y_, &color, &alpha);                                                                                        \
                        egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);                                                                      \
                    }                                                                                                                                          \
                    for (egui_dim_t x_ = rr_img_xs; x_ < rr_img_xe; x_++)                                                                                      \
                    {                                                                                                                                          \
                        _get_pixel_func(image, x_, y_, &color, &alpha);                                                                                        \
                        egui_canvas_draw_point_limit_skip_mask((x_base + x_), rr_sy, color, alpha);                                                            \
                    }                                                                                                                                          \
                    for (egui_dim_t x_ = rr_img_xe; x_ < x_total; x_++)                                                                                        \
                    {                                                                                                                                          \
                        _get_pixel_func(image, x_, y_, &color, &alpha);                                                                                        \
                        egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);                                                                      \
                    }                                                                                                                                          \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                        \
            {                                                                                                                                                  \
                for (egui_dim_t x_ = x; x_ < x_total; x_++)                                                                                                    \
                {                                                                                                                                              \
                    _get_pixel_func(image, x_, y_, &color, &alpha);                                                                                            \
                    egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, alpha);                                                                  \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    }                                                                                                                                                          \
    else                                                                                                                                                       \
    {                                                                                                                                                          \
        for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                            \
        {                                                                                                                                                      \
            for (egui_dim_t x_ = x; x_ < x_total; x_++)                                                                                                        \
            {                                                                                                                                                  \
                _get_pixel_func(image, x_, y_, &color, &alpha);                                                                                                \
                                                                                                                                                               \
                /* change to real position in canvas. */                                                                                                       \
                egui_canvas_draw_point_limit_skip_mask((x_base + x_), (y_base + y_), color, alpha);                                                            \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    }

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
void egui_image_std_set_image_rgb565_8(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                       egui_dim_t y_base)
{
    // EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565_8, self, x, y, x_total, y_total, x_base, y_base);

    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_color_t color;
    egui_alpha_t alpha;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    uint16_t data_row_size = image->width << 1; // same to image->width * 2
    uint16_t alpha_row_size = image->width;
    EGUI_UNUSED(data_row_size);
    EGUI_UNUSED(alpha_row_size);
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    void *data_buf = NULL;
    void *alpha_buf = NULL;
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        data_buf = egui_image_std_acquire_external_buffer(data_row_size, g_external_image_data_cache, sizeof(g_external_image_data_cache));
        alpha_buf = egui_image_std_acquire_external_buffer(alpha_row_size, g_external_image_alpha_cache, sizeof(g_external_image_alpha_cache));
        if (data_buf == NULL || alpha_buf == NULL)
        {
            egui_image_std_release_external_buffer(data_buf, g_external_image_data_cache);
            egui_image_std_release_external_buffer(alpha_buf, g_external_image_alpha_cache);
            return;
        }
    }
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE

    for (egui_dim_t y_ = y; y_ < y_total; y_++)
    {
        egui_dim_t rr_sy = y_base + y_;
        // Check mask row range BEFORE loading data (avoids wasted I/O for extern resources)
        egui_dim_t rr_x_start = 0, rr_x_end = 0;
        int rr_res = -1; // -1 = no mask or no row_range API
        if (canvas->mask != NULL && canvas->mask->api->mask_get_row_range != NULL)
        {
            rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, x_base + x, x_base + x_total, &rr_x_start, &rr_x_end);
            if (rr_res == EGUI_MASK_ROW_OUTSIDE)
            {
                continue; // Skip data loading entirely for this row
            }
        }

        uint32_t row_start = y_ * image->width;
        const void *p_data = NULL;
        const void *p_alpha = NULL;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
        {
            uint32_t start_pos = x;
            uint32_t end_pos = x_total;

            p_data = data_buf;
            p_alpha = alpha_buf;
            egui_image_std_load_data_resource((uint8_t *)data_buf + (start_pos << 1), image, (row_start + start_pos) << 1, (end_pos - start_pos) << 1);
            egui_image_std_load_alpha_resource((uint8_t *)alpha_buf + (start_pos), image, (row_start + start_pos), (end_pos - start_pos));
        }
        else
#endif
        {
            p_data = (const void *)((const uint8_t *)image->data_buf + (row_start << 1));
            p_alpha = (const void *)((const uint8_t *)image->alpha_buf + (row_start));
        }
        if (canvas->mask != NULL)
        {
            if (rr_res >= 0) // mask_get_row_range was available
            {
                if (rr_res == EGUI_MASK_ROW_INSIDE)
                {
                    // Fast path: row fully inside mask — direct PFB access
                    egui_alpha_t canvas_alpha = canvas->alpha;
                    egui_dim_t pfb_width = canvas->pfb_region.size.width;
                    egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
                    egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
                    const uint16_t *src_pixels = (const uint16_t *)p_data + x;
                    const uint8_t *src_alpha_row = (const uint8_t *)p_alpha + x;
                    egui_dim_t count = x_total - x;

                    egui_image_std_blend_rgb565_alpha8_row(dst_row, src_pixels, src_alpha_row, count, canvas_alpha);
                }
                else // PARTIAL
                {
                    egui_dim_t rr_img_xs = rr_x_start - x_base;
                    egui_dim_t rr_img_xe = rr_x_end - x_base;
                    // Left edge: per-pixel with mask
                    for (egui_dim_t x_ = x; x_ < rr_img_xs; x_++)
                    {
                        egui_image_std_get_col_pixel_rgb565_8(p_data, p_alpha, x_, &color, &alpha);
                        egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                    }
                    // Middle: direct PFB access (inside mask)
                    {
                        egui_alpha_t canvas_alpha = canvas->alpha;
                        egui_dim_t pfb_width = canvas->pfb_region.size.width;
                        egui_dim_t dst_x = rr_x_start - canvas->pfb_location_in_base_view.x;
                        egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                        egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x];
                        const uint16_t *src_pixels = (const uint16_t *)p_data + rr_img_xs;
                        const uint8_t *src_alpha_mid = (const uint8_t *)p_alpha + rr_img_xs;
                        egui_dim_t mid_count = rr_img_xe - rr_img_xs;

                        egui_image_std_blend_rgb565_alpha8_row(dst_row, src_pixels, src_alpha_mid, mid_count, canvas_alpha);
                    }
                    // Right edge: per-pixel with mask
                    for (egui_dim_t x_ = rr_img_xe; x_ < x_total; x_++)
                    {
                        egui_image_std_get_col_pixel_rgb565_8(p_data, p_alpha, x_, &color, &alpha);
                        egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                    }
                }
            }
            else
            {
                for (egui_dim_t x_ = x; x_ < x_total; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_8(p_data, p_alpha, x_, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, alpha);
                }
            }
        }
        else
        {
            // Fast path: direct PFB access, no mask, pre-calculate row pointers
            egui_alpha_t canvas_alpha = canvas->alpha;
            egui_dim_t pfb_width = canvas->pfb_region.size.width;
            egui_dim_t count = x_total - x;
            egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];

            const uint16_t *src_pixels = (const uint16_t *)p_data + x;
            const uint8_t *src_alpha_row = (const uint8_t *)p_alpha + x;

            egui_image_std_blend_rgb565_alpha8_row(dst_row, src_pixels, src_alpha_row, count, canvas_alpha);
        }
    }
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_std_release_external_buffer(data_buf, g_external_image_data_cache);
    egui_image_std_release_external_buffer(alpha_buf, g_external_image_alpha_cache);
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
void egui_image_std_set_image_rgb565_4(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                       egui_dim_t y_base)
{
    // EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565_4, self, x, y, x_total, y_total, x_base, y_base);

    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_color_t color;
    egui_alpha_t alpha;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    uint16_t data_row_size = image->width << 1;          // same to image->width * 2
    uint16_t alpha_row_size = ((image->width + 1) >> 1); // same to: ((image->width + 1) / 2);
    EGUI_UNUSED(data_row_size);
    EGUI_UNUSED(alpha_row_size);
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    void *data_buf = NULL;
    void *alpha_buf = NULL;
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        data_buf = egui_image_std_acquire_external_buffer(data_row_size, g_external_image_data_cache, sizeof(g_external_image_data_cache));
        alpha_buf = egui_image_std_acquire_external_buffer(alpha_row_size, g_external_image_alpha_cache, sizeof(g_external_image_alpha_cache));
        if (data_buf == NULL || alpha_buf == NULL)
        {
            egui_image_std_release_external_buffer(data_buf, g_external_image_data_cache);
            egui_image_std_release_external_buffer(alpha_buf, g_external_image_alpha_cache);
            return;
        }
    }
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE

    for (egui_dim_t y_ = y; y_ < y_total; y_++)
    {
        egui_dim_t rr_sy = y_base + y_;
        // Check mask row range BEFORE loading data (avoids wasted I/O for extern resources)
        egui_dim_t rr_x_start = 0, rr_x_end = 0;
        int rr_res = -1; // -1 = no mask or no row_range API
        if (canvas->mask != NULL && canvas->mask->api->mask_get_row_range != NULL)
        {
            rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, x_base + x, x_base + x_total, &rr_x_start, &rr_x_end);
            if (rr_res == EGUI_MASK_ROW_OUTSIDE)
            {
                continue; // Skip data loading entirely for this row
            }
        }

        uint32_t row_start = y_ * image->width;
        uint32_t row_start_alpha = y_ * alpha_row_size;
        const void *p_data = NULL;
        const void *p_alpha = NULL;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
        {
            uint32_t start_pos = x;
            uint32_t start_pos_alpha = x >> 1; // same to: x / 2
            uint32_t end_pos = x_total;
            uint32_t end_pos_alpha = (x_total + 1) >> 1; // same to: x / 2

            p_data = data_buf;
            p_alpha = alpha_buf;
            egui_image_std_load_data_resource((uint8_t *)data_buf + (start_pos << 1), image, (row_start + start_pos) << 1, (end_pos - start_pos) << 1);
            egui_image_std_load_alpha_resource((uint8_t *)alpha_buf + (start_pos_alpha), image, (row_start_alpha + start_pos_alpha),
                                               (end_pos_alpha - start_pos_alpha));
        }
        else
#endif
        {
            p_data = (const void *)((const uint8_t *)image->data_buf + (row_start << 1));
            p_alpha = (const void *)((const uint8_t *)image->alpha_buf + (row_start_alpha));
        }

        if (canvas->mask != NULL)
        {
            if (rr_res == EGUI_MASK_ROW_INSIDE)
            {
                // Fast path: row fully inside mask - direct PFB access with batch copy
                egui_alpha_t canvas_alpha = canvas->alpha;
                egui_dim_t pfb_width = canvas->pfb_region.size.width;
                egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
                egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];

                egui_image_std_blend_rgb565_alpha4_row(dst_row, (const uint16_t *)p_data, (const uint8_t *)p_alpha, x, x_total - x, canvas_alpha);
            }
            else if (rr_res == EGUI_MASK_ROW_PARTIAL)
            {
                egui_dim_t rr_img_xs = rr_x_start - x_base;
                egui_dim_t rr_img_xe = rr_x_end - x_base;
                // Left edge: per-pixel with mask
                for (egui_dim_t x_ = x; x_ < rr_img_xs; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_4(p_data, p_alpha, x_, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                }
                // Middle: direct PFB access with batch copy
                {
                    egui_alpha_t canvas_alpha = canvas->alpha;
                    egui_dim_t pfb_width = canvas->pfb_region.size.width;
                    egui_dim_t dst_x = rr_x_start - canvas->pfb_location_in_base_view.x;
                    egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x];

                    egui_image_std_blend_rgb565_alpha4_row(dst_row, (const uint16_t *)p_data, (const uint8_t *)p_alpha, rr_img_xs, rr_img_xe - rr_img_xs,
                                                           canvas_alpha);
                }
                // Right edge: per-pixel with mask
                for (egui_dim_t x_ = rr_img_xe; x_ < x_total; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_4(p_data, p_alpha, x_, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                }
            }
            else
            {
                for (egui_dim_t x_ = x; x_ < x_total; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_4(p_data, p_alpha, x_, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, alpha);
                }
            }
        }
        else
        {
            // Fast path: direct PFB access with opaque-run batch copy
            egui_alpha_t canvas_alpha = canvas->alpha;
            egui_dim_t pfb_width = canvas->pfb_region.size.width;
            egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];

            egui_image_std_blend_rgb565_alpha4_row(dst_row, (const uint16_t *)p_data, (const uint8_t *)p_alpha, x, x_total - x, canvas_alpha);
        }
    }
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_std_release_external_buffer(data_buf, g_external_image_data_cache);
    egui_image_std_release_external_buffer(alpha_buf, g_external_image_alpha_cache);
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
void egui_image_std_set_image_rgb565_2(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                       egui_dim_t y_base)
{
    // EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565_2, self, x, y, x_total, y_total, x_base, y_base);

    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_color_t color;
    egui_alpha_t alpha;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    uint16_t data_row_size = image->width << 1;          // same to image->width * 2
    uint16_t alpha_row_size = ((image->width + 3) >> 2); // same to: ((image->width + 3) / 4);
    EGUI_UNUSED(data_row_size);
    EGUI_UNUSED(alpha_row_size);
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    void *data_buf = NULL;
    void *alpha_buf = NULL;
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        data_buf = egui_image_std_acquire_external_buffer(data_row_size, g_external_image_data_cache, sizeof(g_external_image_data_cache));
        alpha_buf = egui_image_std_acquire_external_buffer(alpha_row_size, g_external_image_alpha_cache, sizeof(g_external_image_alpha_cache));
        if (data_buf == NULL || alpha_buf == NULL)
        {
            egui_image_std_release_external_buffer(data_buf, g_external_image_data_cache);
            egui_image_std_release_external_buffer(alpha_buf, g_external_image_alpha_cache);
            return;
        }
    }
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE

    for (egui_dim_t y_ = y; y_ < y_total; y_++)
    {
        egui_dim_t rr_sy = y_base + y_;
        // Check mask row range BEFORE loading data (avoids wasted I/O for extern resources)
        egui_dim_t rr_x_start = 0, rr_x_end = 0;
        int rr_res = -1; // -1 = no mask or no row_range API
        if (canvas->mask != NULL && canvas->mask->api->mask_get_row_range != NULL)
        {
            rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, x_base + x, x_base + x_total, &rr_x_start, &rr_x_end);
            if (rr_res == EGUI_MASK_ROW_OUTSIDE)
            {
                continue; // Skip data loading entirely for this row
            }
        }

        uint32_t row_start = y_ * image->width;
        uint32_t row_start_alpha = y_ * alpha_row_size;
        const void *p_data = NULL;
        const void *p_alpha = NULL;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
        {
            uint32_t start_pos = x;
            uint32_t start_pos_alpha = x >> 2; // same to: x / 4
            uint32_t end_pos = x_total;
            uint32_t end_pos_alpha = ((x_total + 3) >> 2); // same to: x / 4

            p_data = data_buf;
            p_alpha = alpha_buf;
            egui_image_std_load_data_resource((uint8_t *)data_buf + (start_pos << 1), image, (row_start + start_pos) << 1, (end_pos - start_pos) << 1);
            egui_image_std_load_alpha_resource((uint8_t *)alpha_buf + (start_pos_alpha), image, (row_start_alpha + start_pos_alpha),
                                               (end_pos_alpha - start_pos_alpha));
        }
        else
#endif
        {
            p_data = (const void *)((const uint8_t *)image->data_buf + (row_start << 1));
            p_alpha = (const void *)((const uint8_t *)image->alpha_buf + (row_start_alpha));
        }

        if (canvas->mask != NULL)
        {
            if (rr_res == EGUI_MASK_ROW_INSIDE)
            {
                // Fast path: row fully inside mask - direct PFB access with batch copy
                egui_alpha_t canvas_alpha = canvas->alpha;
                egui_dim_t pfb_width = canvas->pfb_region.size.width;
                egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
                egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];

                egui_image_std_blend_rgb565_alpha2_row(dst_row, (const uint16_t *)p_data, (const uint8_t *)p_alpha, x, x_total - x, canvas_alpha);
            }
            else if (rr_res == EGUI_MASK_ROW_PARTIAL)
            {
                egui_dim_t rr_img_xs = rr_x_start - x_base;
                egui_dim_t rr_img_xe = rr_x_end - x_base;
                // Left edge: per-pixel with mask
                for (egui_dim_t x_ = x; x_ < rr_img_xs; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_2(p_data, p_alpha, x_, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                }
                // Middle: direct PFB access with batch copy
                {
                    egui_alpha_t canvas_alpha = canvas->alpha;
                    egui_dim_t pfb_width = canvas->pfb_region.size.width;
                    egui_dim_t dst_x = rr_x_start - canvas->pfb_location_in_base_view.x;
                    egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x];

                    egui_image_std_blend_rgb565_alpha2_row(dst_row, (const uint16_t *)p_data, (const uint8_t *)p_alpha, rr_img_xs, rr_img_xe - rr_img_xs,
                                                           canvas_alpha);
                }
                // Right edge: per-pixel with mask
                for (egui_dim_t x_ = rr_img_xe; x_ < x_total; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_2(p_data, p_alpha, x_, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                }
            }
            else
            {
                for (egui_dim_t x_ = x; x_ < x_total; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_2(p_data, p_alpha, x_, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, alpha);
                }
            }
        }
        else
        {
            // Fast path: direct PFB access with opaque-run batch copy
            egui_alpha_t canvas_alpha = canvas->alpha;
            egui_dim_t pfb_width = canvas->pfb_region.size.width;
            egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];

            egui_image_std_blend_rgb565_alpha2_row(dst_row, (const uint16_t *)p_data, (const uint8_t *)p_alpha, x, x_total - x, canvas_alpha);
        }
    }
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_std_release_external_buffer(data_buf, g_external_image_data_cache);
    egui_image_std_release_external_buffer(alpha_buf, g_external_image_alpha_cache);
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
void egui_image_std_set_image_rgb565_1(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                       egui_dim_t y_base)
{
    // EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565_1, self, x, y, x_total, y_total, x_base, y_base);

    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_color_t color;
    egui_alpha_t alpha;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    uint16_t data_row_size = image->width << 1;          // same to image->width * 2
    uint16_t alpha_row_size = ((image->width + 7) >> 3); // same to ((image->width + 7) / 8);
    EGUI_UNUSED(data_row_size);
    EGUI_UNUSED(alpha_row_size);
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    void *data_buf = NULL;
    void *alpha_buf = NULL;
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        data_buf = egui_image_std_acquire_external_buffer(data_row_size, g_external_image_data_cache, sizeof(g_external_image_data_cache));
        alpha_buf = egui_image_std_acquire_external_buffer(alpha_row_size, g_external_image_alpha_cache, sizeof(g_external_image_alpha_cache));
        if (data_buf == NULL || alpha_buf == NULL)
        {
            egui_image_std_release_external_buffer(data_buf, g_external_image_data_cache);
            egui_image_std_release_external_buffer(alpha_buf, g_external_image_alpha_cache);
            return;
        }
    }
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE

    for (egui_dim_t y_ = y; y_ < y_total; y_++)
    {
        egui_dim_t rr_sy = y_base + y_;
        // Check mask row range BEFORE loading data (avoids wasted I/O for extern resources)
        egui_dim_t rr_x_start = 0, rr_x_end = 0;
        int rr_res = -1; // -1 = no mask or no row_range API
        if (canvas->mask != NULL && canvas->mask->api->mask_get_row_range != NULL)
        {
            rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, x_base + x, x_base + x_total, &rr_x_start, &rr_x_end);
            if (rr_res == EGUI_MASK_ROW_OUTSIDE)
            {
                continue; // Skip data loading entirely for this row
            }
        }

        uint32_t row_start = y_ * image->width;
        uint32_t row_start_alpha = y_ * alpha_row_size;
        const void *p_data = NULL;
        const void *p_alpha = NULL;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
        {
            uint32_t start_pos = x;
            uint32_t start_pos_alpha = x >> 3; // same to x / 8
            uint32_t end_pos = x_total;
            uint32_t end_pos_alpha = ((x_total + 7) >> 3); // same to x / 8

            p_data = data_buf;
            p_alpha = alpha_buf;
            egui_image_std_load_data_resource((uint8_t *)data_buf + (start_pos << 1), image, (row_start + start_pos) << 1, (end_pos - start_pos) << 1);
            egui_image_std_load_alpha_resource((uint8_t *)alpha_buf + (start_pos_alpha), image, (row_start_alpha + start_pos_alpha),
                                               (end_pos_alpha - start_pos_alpha));
        }
        else
#endif
        {
            p_data = (const void *)((const uint8_t *)image->data_buf + (row_start << 1));
            p_alpha = (const void *)((const uint8_t *)image->alpha_buf + (row_start_alpha));
        }

        if (canvas->mask != NULL)
        {
            if (rr_res == EGUI_MASK_ROW_INSIDE)
            {
                // Fast path: row fully inside mask - direct PFB access with batch copy
                egui_alpha_t canvas_alpha = canvas->alpha;
                egui_dim_t pfb_width = canvas->pfb_region.size.width;
                egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
                egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];

                egui_image_std_blend_rgb565_alpha1_row(dst_row, (const uint16_t *)p_data, (const uint8_t *)p_alpha, x, x_total - x, canvas_alpha);
            }
            else if (rr_res == EGUI_MASK_ROW_PARTIAL)
            {
                egui_dim_t rr_img_xs = rr_x_start - x_base;
                egui_dim_t rr_img_xe = rr_x_end - x_base;
                // Left edge: per-pixel with mask
                for (egui_dim_t x_ = x; x_ < rr_img_xs; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_1(p_data, p_alpha, x_, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                }
                // Middle: direct PFB access with batch copy
                {
                    egui_alpha_t canvas_alpha = canvas->alpha;
                    egui_dim_t pfb_width = canvas->pfb_region.size.width;
                    egui_dim_t dst_x = rr_x_start - canvas->pfb_location_in_base_view.x;
                    egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x];

                    egui_image_std_blend_rgb565_alpha1_row(dst_row, (const uint16_t *)p_data, (const uint8_t *)p_alpha, rr_img_xs, rr_img_xe - rr_img_xs,
                                                           canvas_alpha);
                }
                // Right edge: per-pixel with mask
                for (egui_dim_t x_ = rr_img_xe; x_ < x_total; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_1(p_data, p_alpha, x_, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                }
            }
            else
            {
                for (egui_dim_t x_ = x; x_ < x_total; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_1(p_data, p_alpha, x_, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, alpha);
                }
            }
        }
        else
        {
            // Fast path: direct PFB access with opaque-run batch copy
            egui_alpha_t canvas_alpha = canvas->alpha;
            egui_dim_t pfb_width = canvas->pfb_region.size.width;
            egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];

            egui_image_std_blend_rgb565_alpha1_row(dst_row, (const uint16_t *)p_data, (const uint8_t *)p_alpha, x, x_total - x, canvas_alpha);
        }
    }
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_std_release_external_buffer(data_buf, g_external_image_data_cache);
    egui_image_std_release_external_buffer(alpha_buf, g_external_image_alpha_cache);
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
void egui_image_std_set_image_rgb565(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                     egui_dim_t y_base)
{
    if ((egui_canvas_get_canvas()->alpha == EGUI_ALPHA_100) && (egui_canvas_get_canvas()->mask == NULL))
    {
        egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
        egui_canvas_t *canvas = egui_canvas_get_canvas();
        egui_dim_t pfb_width = canvas->pfb_region.size.width;
        egui_dim_t count = x_total - x;
        egui_dim_t img_width = image->width;

        // Pre-calculate starting pointers (avoid per-row multiply)
        egui_dim_t dst_y_start = (y_base + y) - canvas->pfb_location_in_base_view.y;
        egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
        egui_color_int_t *dst_row = &canvas->pfb[dst_y_start * pfb_width + dst_x_start];

        const uint16_t *src_row = NULL;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        void *data_buf = NULL;
        if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
        {
            data_buf = egui_image_std_acquire_external_buffer(img_width << 1, g_external_image_data_cache, sizeof(g_external_image_data_cache));
            if (data_buf == NULL)
            {
                return;
            }
        }
        else
#endif
        {
            src_row = (const uint16_t *)image->data_buf + (y * img_width) + x;
        }

        for (egui_dim_t y_ = y; y_ < y_total; y_++)
        {
            const uint16_t *src = NULL;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
            if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
            {
                uint32_t row_start = y_ * img_width;

                egui_image_std_load_data_resource((uint8_t *)data_buf + (x << 1), image, (row_start + x) << 1, count << 1);
                src = (const uint16_t *)data_buf + x;
            }
            else
#endif
            {
                src = src_row;
                src_row += img_width;
            }

            // No conversion needed: 32-bit word copy
            {
                const uint32_t *src32 = (const uint32_t *)src;
                uint32_t *dst32 = (uint32_t *)dst_row;
                egui_dim_t n32 = count >> 1;
                for (egui_dim_t i = 0; i < n32; i++)
                {
                    dst32[i] = src32[i];
                }
                if (count & 1)
                {
                    dst_row[count - 1] = src[count - 1];
                }
            }

            // Advance to next row using addition (no multiply)
            dst_row += pfb_width;
        }
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        egui_image_std_release_external_buffer(data_buf, g_external_image_data_cache);
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    }
    else
    {
        EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565, self, x, y, x_total, y_total, x_base, y_base);
    }
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
void egui_image_std_set_image_rgb32(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                    egui_dim_t y_base)
{
    EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(egui_image_std_get_pixel_rgb32, self, x, y, x_total, y_total, x_base, y_base);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32

void egui_image_std_draw_image(const egui_image_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    // egui_color_t color;
    // egui_alpha_t alpha;
    egui_dim_t width = image->width;
    egui_dim_t height = image->height;

    egui_dim_t x_total;
    egui_dim_t y_total;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, egui_canvas_get_base_view_work_region(), &region);

    if (egui_region_is_empty(&region))
    {
        return;
    }

    // change to image coordinate.
    region.location.x -= x;
    region.location.y -= y;

    // for speed, calculate total positions outside of the loop
    x_total = region.location.x + region.size.width;
    y_total = region.location.y + region.size.height;

    if (image)
    {
        switch (image->data_type)
        {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
        case EGUI_IMAGE_DATA_TYPE_RGB32:
            egui_image_std_set_image_rgb32(self, region.location.x, region.location.y, x_total, y_total, x, y);
            break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
        case EGUI_IMAGE_DATA_TYPE_RGB565:
            if (image->alpha_buf == NULL)
            {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
                egui_image_std_set_image_rgb565(self, region.location.x, region.location.y, x_total, y_total, x, y);
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
            }
            else
            {
                switch (image->alpha_type)
                {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
                case EGUI_IMAGE_ALPHA_TYPE_1:
                    egui_image_std_set_image_rgb565_1(self, region.location.x, region.location.y, x_total, y_total, x, y);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
                case EGUI_IMAGE_ALPHA_TYPE_2:
                    egui_image_std_set_image_rgb565_2(self, region.location.x, region.location.y, x_total, y_total, x, y);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
                case EGUI_IMAGE_ALPHA_TYPE_4:
                    egui_image_std_set_image_rgb565_4(self, region.location.x, region.location.y, x_total, y_total, x, y);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
                case EGUI_IMAGE_ALPHA_TYPE_8:
                    egui_image_std_set_image_rgb565_8(self, region.location.x, region.location.y, x_total, y_total, x, y);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
                default:
                    EGUI_ASSERT(0);
                    break;
                }
            }
            break;
        default:
            EGUI_ASSERT(0);
            break;
        }
    }
}

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
static void egui_image_std_set_image_resize_rgb565_8_common(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total,
                                                            egui_dim_t x_base, egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_color_t color;
    egui_alpha_t alpha;
    egui_dim_t src_y;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_alpha_t canvas_alpha = canvas->alpha;
    egui_dim_t pfb_width = canvas->pfb_region.size.width;
    egui_dim_t screen_x_start = x_base + x;
    egui_dim_t dst_x_start = screen_x_start - canvas->pfb_location_in_base_view.x;
    egui_dim_t count = x_total - x;
    egui_dim_t src_x_map[EGUI_CONFIG_PFB_WIDTH];
    const uint16_t *src_row = NULL;
    const uint8_t *src_alpha_row = NULL;
    int cached_src_y = -1;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    void *data_buf = NULL;
    void *alpha_buf = NULL;
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        uint32_t data_row_size = image->width << 1;
        uint32_t alpha_row_size = image->width;

        data_buf = egui_image_std_acquire_external_buffer(data_row_size, g_external_image_data_cache, sizeof(g_external_image_data_cache));
        alpha_buf = egui_image_std_acquire_external_buffer(alpha_row_size, g_external_image_alpha_cache, sizeof(g_external_image_alpha_cache));
        if (data_buf == NULL || alpha_buf == NULL)
        {
            egui_image_std_release_external_buffer(data_buf, g_external_image_data_cache);
            egui_image_std_release_external_buffer(alpha_buf, g_external_image_alpha_cache);
            return;
        }
    }
#endif

    count = egui_image_std_prepare_resize_src_x_map_limit(src_x_map, x, x_total, width_radio);

    if (canvas->mask != NULL)
    {
        if (canvas->mask->api->mask_get_row_range != NULL)
        {
            egui_dim_t rr_x_start, rr_x_end;
            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                egui_dim_t rr_sy = y_base + y_;
                egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                if (cached_src_y != src_y)
                {
                    uint32_t row_start = src_y * image->width;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
                    {
                        egui_image_std_load_data_resource(data_buf, image, row_start << 1, image->width << 1);
                        egui_image_std_load_alpha_resource(alpha_buf, image, row_start, image->width);
                        src_row = (const uint16_t *)data_buf;
                        src_alpha_row = (const uint8_t *)alpha_buf;
                    }
                    else
#endif
                    {
                        src_row = (const uint16_t *)image->data_buf + row_start;
                        src_alpha_row = (const uint8_t *)image->alpha_buf + row_start;
                    }
                    cached_src_y = src_y;
                }

                int rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &rr_x_start, &rr_x_end);
                if (rr_res == EGUI_MASK_ROW_OUTSIDE)
                {
                    continue;
                }

                if (rr_res == EGUI_MASK_ROW_INSIDE)
                {
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
                    egui_image_std_blend_rgb565_alpha8_mapped_row(dst_row, src_row, src_alpha_row, src_x_map, count, canvas_alpha);
                }
                else
                {
                    egui_dim_t visible_x_start = screen_x_start;
                    egui_dim_t visible_x_end = x_base + x_total;
                    if (canvas->mask->api->mask_get_row_visible_range != NULL &&
                        !canvas->mask->api->mask_get_row_visible_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &visible_x_start, &visible_x_end))
                    {
                        continue;
                    }

                    egui_dim_t rr_img_xs = EGUI_MAX(x, rr_x_start - x_base);
                    egui_dim_t rr_img_xe = EGUI_MIN(x_total, rr_x_end - x_base);
                    egui_dim_t vis_img_xs = EGUI_MAX(x, visible_x_start - x_base);
                    egui_dim_t vis_img_xe = EGUI_MIN(x_total, visible_x_end - x_base);

                    for (egui_dim_t x_ = vis_img_xs; x_ < EGUI_MIN(rr_img_xs, vis_img_xe); x_++)
                    {
                        egui_dim_t src_x = src_x_map[x_ - x];
                        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                        alpha = src_alpha_row[src_x];
                        egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, alpha);
                    }

                    if (rr_img_xs < rr_img_xe)
                    {
                        egui_dim_t mid_offset = rr_img_xs - x;
                        egui_dim_t mid_count = rr_img_xe - rr_img_xs;
                        egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + rr_img_xs - canvas->pfb_location_in_base_view.x)];

                        egui_image_std_blend_rgb565_alpha8_mapped_row(dst_row, src_row, src_alpha_row, &src_x_map[mid_offset], mid_count, canvas_alpha);
                    }

                    for (egui_dim_t x_ = EGUI_MAX(rr_img_xe, vis_img_xs); x_ < vis_img_xe; x_++)
                    {
                        egui_dim_t src_x = src_x_map[x_ - x];
                        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                        alpha = src_alpha_row[src_x];
                        egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, alpha);
                    }
                }
            }
        }
        else
        {
            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                src_y = (egui_dim_t)EGUI_FLOAT_MULT(y_, height_radio);
                if (cached_src_y != src_y)
                {
                    uint32_t row_start = src_y * image->width;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
                    {
                        egui_image_std_load_data_resource(data_buf, image, row_start << 1, image->width << 1);
                        egui_image_std_load_alpha_resource(alpha_buf, image, row_start, image->width);
                        src_row = (const uint16_t *)data_buf;
                        src_alpha_row = (const uint8_t *)alpha_buf;
                    }
                    else
#endif
                    {
                        src_row = (const uint16_t *)image->data_buf + row_start;
                        src_alpha_row = (const uint8_t *)image->alpha_buf + row_start;
                    }
                    cached_src_y = src_y;
                }

                for (egui_dim_t i = 0; i < count; i++)
                {
                    egui_dim_t src_x = src_x_map[i];
                    color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                    alpha = src_alpha_row[src_x];
                    egui_canvas_draw_point_limit(screen_x_start + i, y_base + y_, color, alpha);
                }
            }
        }
    }
    else
    {
        for (egui_dim_t y_ = y; y_ < y_total; y_++)
        {
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
            src_y = (egui_dim_t)EGUI_FLOAT_MULT(y_, height_radio);
            if (cached_src_y != src_y)
            {
                uint32_t row_start = src_y * image->width;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
                {
                    egui_image_std_load_data_resource(data_buf, image, row_start << 1, image->width << 1);
                    egui_image_std_load_alpha_resource(alpha_buf, image, row_start, image->width);
                    src_row = (const uint16_t *)data_buf;
                    src_alpha_row = (const uint8_t *)alpha_buf;
                }
                else
#endif
                {
                    src_row = (const uint16_t *)image->data_buf + row_start;
                    src_alpha_row = (const uint8_t *)image->alpha_buf + row_start;
                }
                cached_src_y = src_y;
            }

            egui_image_std_blend_rgb565_alpha8_mapped_row(dst_row, src_row, src_alpha_row, src_x_map, count, canvas_alpha);
        }
    }

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_std_release_external_buffer(data_buf, g_external_image_data_cache);
    egui_image_std_release_external_buffer(alpha_buf, g_external_image_alpha_cache);
#endif
}
#endif

#define EGUI_IMAGE_STD_SET_IMAGE_RESIZE_RGB565_PACKED_ALPHA_COMMON(_func_name, _alpha_row_size_expr, _get_alpha_func, _blend_row_func)                         \
    static void _func_name(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base, egui_dim_t y_base, \
                           egui_float_t width_radio, egui_float_t height_radio)                                                                                \
    {                                                                                                                                                          \
        egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;                                                                                     \
        egui_color_t color;                                                                                                                                    \
        egui_alpha_t alpha;                                                                                                                                    \
        egui_dim_t src_y;                                                                                                                                      \
        egui_canvas_t *canvas = egui_canvas_get_canvas();                                                                                                      \
        egui_alpha_t canvas_alpha = canvas->alpha;                                                                                                             \
        egui_dim_t pfb_width = canvas->pfb_region.size.width;                                                                                                  \
        egui_dim_t screen_x_start = x_base + x;                                                                                                                \
        egui_dim_t dst_x_start = screen_x_start - canvas->pfb_location_in_base_view.x;                                                                         \
        egui_dim_t count = x_total - x;                                                                                                                        \
        egui_dim_t src_x_map[EGUI_CONFIG_PFB_WIDTH];                                                                                                           \
        const uint16_t *src_row;                                                                                                                               \
        const uint8_t *src_alpha_row;                                                                                                                          \
        uint32_t alpha_row_size = (_alpha_row_size_expr);                                                                                                      \
                                                                                                                                                               \
        count = egui_image_std_prepare_resize_src_x_map_limit(src_x_map, x, x_total, width_radio);                                                             \
        if (canvas->mask != NULL)                                                                                                                              \
        {                                                                                                                                                      \
            if (canvas->mask->api->mask_get_row_range != NULL)                                                                                                 \
            {                                                                                                                                                  \
                egui_dim_t rr_x_start, rr_x_end;                                                                                                               \
                for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                    \
                {                                                                                                                                              \
                    egui_dim_t rr_sy = y_base + y_;                                                                                                            \
                    egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;                                                                            \
                    src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);                                                                               \
                    src_row = (const uint16_t *)image->data_buf + (src_y * image->width);                                                                      \
                    src_alpha_row = (const uint8_t *)image->alpha_buf + (src_y * alpha_row_size);                                                              \
                                                                                                                                                               \
                    int rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &rr_x_start, &rr_x_end);         \
                    if (rr_res == EGUI_MASK_ROW_OUTSIDE)                                                                                                       \
                    {                                                                                                                                          \
                        continue;                                                                                                                              \
                    }                                                                                                                                          \
                                                                                                                                                               \
                    if (rr_res == EGUI_MASK_ROW_INSIDE)                                                                                                        \
                    {                                                                                                                                          \
                        egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];                                                             \
                        _blend_row_func(dst_row, src_row, src_alpha_row, src_x_map, count, canvas_alpha);                                                      \
                    }                                                                                                                                          \
                    else                                                                                                                                       \
                    {                                                                                                                                          \
                        egui_dim_t visible_x_start = screen_x_start;                                                                                           \
                        egui_dim_t visible_x_end = x_base + x_total;                                                                                           \
                        if (canvas->mask->api->mask_get_row_visible_range != NULL &&                                                                           \
                            !canvas->mask->api->mask_get_row_visible_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &visible_x_start,            \
                                                                           &visible_x_end))                                                                    \
                        {                                                                                                                                      \
                            continue;                                                                                                                          \
                        }                                                                                                                                      \
                                                                                                                                                               \
                        egui_dim_t rr_img_xs = EGUI_MAX(x, rr_x_start - x_base);                                                                               \
                        egui_dim_t rr_img_xe = EGUI_MIN(x_total, rr_x_end - x_base);                                                                           \
                        egui_dim_t vis_img_xs = EGUI_MAX(x, visible_x_start - x_base);                                                                         \
                        egui_dim_t vis_img_xe = EGUI_MIN(x_total, visible_x_end - x_base);                                                                     \
                                                                                                                                                               \
                        for (egui_dim_t x_ = vis_img_xs; x_ < EGUI_MIN(rr_img_xs, vis_img_xe); x_++)                                                           \
                        {                                                                                                                                      \
                            egui_dim_t src_x = src_x_map[x_ - x];                                                                                              \
                            color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);                                                                              \
                            alpha = _get_alpha_func(src_alpha_row, src_x);                                                                                     \
                            egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, alpha);                                                                    \
                        }                                                                                                                                      \
                                                                                                                                                               \
                        if (rr_img_xs < rr_img_xe)                                                                                                             \
                        {                                                                                                                                      \
                            egui_dim_t mid_offset = rr_img_xs - x;                                                                                             \
                            egui_dim_t mid_count = rr_img_xe - rr_img_xs;                                                                                      \
                            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + rr_img_xs - canvas->pfb_location_in_base_view.x)];          \
                                                                                                                                                               \
                            _blend_row_func(dst_row, src_row, src_alpha_row, &src_x_map[mid_offset], mid_count, canvas_alpha);                                 \
                        }                                                                                                                                      \
                                                                                                                                                               \
                        for (egui_dim_t x_ = EGUI_MAX(rr_img_xe, vis_img_xs); x_ < vis_img_xe; x_++)                                                           \
                        {                                                                                                                                      \
                            egui_dim_t src_x = src_x_map[x_ - x];                                                                                              \
                            color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);                                                                              \
                            alpha = _get_alpha_func(src_alpha_row, src_x);                                                                                     \
                            egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, alpha);                                                                    \
                        }                                                                                                                                      \
                    }                                                                                                                                          \
                }                                                                                                                                              \
            }                                                                                                                                                  \
            else                                                                                                                                               \
            {                                                                                                                                                  \
                for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                    \
                {                                                                                                                                              \
                    src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);                                                                               \
                    src_row = (const uint16_t *)image->data_buf + (src_y * image->width);                                                                      \
                    src_alpha_row = (const uint8_t *)image->alpha_buf + (src_y * alpha_row_size);                                                              \
                                                                                                                                                               \
                    for (egui_dim_t i = 0; i < count; i++)                                                                                                     \
                    {                                                                                                                                          \
                        egui_dim_t src_x = src_x_map[i];                                                                                                       \
                        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);                                                                                  \
                        alpha = _get_alpha_func(src_alpha_row, src_x);                                                                                         \
                        egui_canvas_draw_point_limit(screen_x_start + i, y_base + y_, color, alpha);                                                           \
                    }                                                                                                                                          \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                        \
            {                                                                                                                                                  \
                egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;                                                                        \
                egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];                                                                     \
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);                                                                                   \
                src_row = (const uint16_t *)image->data_buf + (src_y * image->width);                                                                          \
                src_alpha_row = (const uint8_t *)image->alpha_buf + (src_y * alpha_row_size);                                                                  \
                                                                                                                                                               \
                _blend_row_func(dst_row, src_row, src_alpha_row, src_x_map, count, canvas_alpha);                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    }

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
EGUI_IMAGE_STD_SET_IMAGE_RESIZE_RGB565_PACKED_ALPHA_COMMON(egui_image_std_set_image_resize_rgb565_4_common, ((image->width + 1) >> 1),
                                                           egui_image_std_get_alpha_rgb565_4_row, egui_image_std_blend_rgb565_alpha4_mapped_row)
#endif

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
EGUI_IMAGE_STD_SET_IMAGE_RESIZE_RGB565_PACKED_ALPHA_COMMON(egui_image_std_set_image_resize_rgb565_2_common, ((image->width + 3) >> 2),
                                                           egui_image_std_get_alpha_rgb565_2_row, egui_image_std_blend_rgb565_alpha2_mapped_row)
#endif

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
EGUI_IMAGE_STD_SET_IMAGE_RESIZE_RGB565_PACKED_ALPHA_COMMON(egui_image_std_set_image_resize_rgb565_1_common, ((image->width + 7) >> 3),
                                                           egui_image_std_get_alpha_rgb565_1_row, egui_image_std_blend_rgb565_alpha1_mapped_row)
#endif

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static void egui_image_std_draw_image_resize_external_alpha(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total,
                                                            egui_dim_t x_base, egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio,
                                                            uint32_t alpha_row_size, egui_image_std_get_col_pixel_with_alpha *get_col_pixel,
                                                            egui_image_std_blend_mapped_row_func *blend_row_func)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_color_t color;
    egui_alpha_t alpha;
    egui_dim_t src_y;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_alpha_t canvas_alpha = canvas->alpha;
    egui_dim_t pfb_width = canvas->pfb_region.size.width;
    egui_dim_t screen_x_start = x_base + x;
    egui_dim_t dst_x_start = screen_x_start - canvas->pfb_location_in_base_view.x;
    egui_dim_t count = x_total - x;
    egui_dim_t src_x_map[EGUI_CONFIG_PFB_WIDTH];
    uint32_t data_row_size = image->width << 1;
    void *data_buf = egui_image_std_acquire_external_buffer(data_row_size, g_external_image_data_cache, sizeof(g_external_image_data_cache));
    void *alpha_buf = egui_image_std_acquire_external_buffer(alpha_row_size, g_external_image_alpha_cache, sizeof(g_external_image_alpha_cache));
    int cached_src_y = -1;

    if (data_buf == NULL || alpha_buf == NULL)
    {
        egui_image_std_release_external_buffer(data_buf, g_external_image_data_cache);
        egui_image_std_release_external_buffer(alpha_buf, g_external_image_alpha_cache);
        return;
    }

    count = egui_image_std_prepare_resize_src_x_map(src_x_map, x, x_total, width_radio);

    if (canvas->mask != NULL)
    {
        if (canvas->mask->api->mask_get_row_range != NULL)
        {
            egui_dim_t rr_x_start, rr_x_end;
            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                egui_dim_t rr_sy = y_base + y_;
                egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                src_y = (egui_dim_t)EGUI_FLOAT_MULT(y_, height_radio);
                if (cached_src_y != src_y)
                {
                    uint32_t row_start = src_y * image->width;
                    uint32_t row_start_alpha = src_y * alpha_row_size;

                    egui_image_std_load_data_resource(data_buf, image, row_start << 1, data_row_size);
                    egui_image_std_load_alpha_resource(alpha_buf, image, row_start_alpha, alpha_row_size);
                    cached_src_y = src_y;
                }

                int rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &rr_x_start, &rr_x_end);
                if (rr_res == EGUI_MASK_ROW_OUTSIDE)
                {
                    continue;
                }
                if (rr_res == EGUI_MASK_ROW_INSIDE)
                {
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
                    /* Use batch row blender: single call instead of per-pixel function pointer */
                    blend_row_func(dst_row, (const uint16_t *)data_buf, (const uint8_t *)alpha_buf, src_x_map, count, canvas_alpha);
                }
                else
                {
                    egui_dim_t visible_x_start = screen_x_start;
                    egui_dim_t visible_x_end = x_base + x_total;
                    if (canvas->mask->api->mask_get_row_visible_range != NULL &&
                        !canvas->mask->api->mask_get_row_visible_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &visible_x_start, &visible_x_end))
                    {
                        continue;
                    }
                    egui_dim_t rr_img_xs = EGUI_MAX(x, rr_x_start - x_base);
                    egui_dim_t rr_img_xe = EGUI_MIN(x_total, rr_x_end - x_base);
                    egui_dim_t vis_img_xs = EGUI_MAX(x, visible_x_start - x_base);
                    egui_dim_t vis_img_xe = EGUI_MIN(x_total, visible_x_end - x_base);

                    for (egui_dim_t x_ = vis_img_xs; x_ < EGUI_MIN(rr_img_xs, vis_img_xe); x_++)
                    {
                        get_col_pixel((const uint16_t *)data_buf, (const uint8_t *)alpha_buf, src_x_map[x_ - x], &color, &alpha);
                        egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, alpha);
                    }

                    if (rr_img_xs < rr_img_xe)
                    {
                        egui_dim_t mid_offset = rr_img_xs - x;
                        egui_dim_t mid_count = rr_img_xe - rr_img_xs;
                        egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + rr_img_xs - canvas->pfb_location_in_base_view.x)];
                        /* Use batch row blender for the unmasked middle section */
                        blend_row_func(dst_row, (const uint16_t *)data_buf, (const uint8_t *)alpha_buf, &src_x_map[mid_offset], mid_count, canvas_alpha);
                    }

                    for (egui_dim_t x_ = EGUI_MAX(rr_img_xe, vis_img_xs); x_ < vis_img_xe; x_++)
                    {
                        get_col_pixel((const uint16_t *)data_buf, (const uint8_t *)alpha_buf, src_x_map[x_ - x], &color, &alpha);
                        egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, alpha);
                    }
                }
            }
        }
        else
        {
            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                src_y = (egui_dim_t)EGUI_FLOAT_MULT(y_, height_radio);
                if (cached_src_y != src_y)
                {
                    uint32_t row_start = src_y * image->width;
                    uint32_t row_start_alpha = src_y * alpha_row_size;

                    egui_image_std_load_data_resource(data_buf, image, row_start << 1, data_row_size);
                    egui_image_std_load_alpha_resource(alpha_buf, image, row_start_alpha, alpha_row_size);
                    cached_src_y = src_y;
                }

                for (egui_dim_t i = 0; i < count; i++)
                {
                    get_col_pixel((const uint16_t *)data_buf, (const uint8_t *)alpha_buf, src_x_map[i], &color, &alpha);
                    egui_canvas_draw_point_limit(screen_x_start + i, y_base + y_, color, alpha);
                }
            }
        }
    }
    else
    {
        for (egui_dim_t y_ = y; y_ < y_total; y_++)
        {
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
            src_y = (egui_dim_t)EGUI_FLOAT_MULT(y_, height_radio);
            if (cached_src_y != src_y)
            {
                uint32_t row_start = src_y * image->width;
                uint32_t row_start_alpha = src_y * alpha_row_size;

                egui_image_std_load_data_resource(data_buf, image, row_start << 1, data_row_size);
                egui_image_std_load_alpha_resource(alpha_buf, image, row_start_alpha, alpha_row_size);
                cached_src_y = src_y;
            }

            /* Use batch row blender: single call instead of per-pixel function pointer */
            blend_row_func(dst_row, (const uint16_t *)data_buf, (const uint8_t *)alpha_buf, src_x_map, count, canvas_alpha);
        }
    }

    egui_image_std_release_external_buffer(data_buf, g_external_image_data_cache);
    egui_image_std_release_external_buffer(alpha_buf, g_external_image_alpha_cache);
}

static void egui_image_std_set_image_resize_rgb565_external(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total,
                                                            egui_dim_t x_base, egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_color_t color;
    egui_alpha_t canvas_alpha = canvas->alpha;
    egui_dim_t src_y;
    egui_dim_t pfb_width = canvas->pfb_region.size.width;
    egui_dim_t screen_x_start = x_base + x;
    egui_dim_t dst_x_start = screen_x_start - canvas->pfb_location_in_base_view.x;
    egui_dim_t count = x_total - x;
    egui_dim_t src_x_map[EGUI_CONFIG_PFB_WIDTH];
    uint32_t data_row_size = image->width << 1;
    void *data_buf = egui_image_std_acquire_external_buffer(data_row_size, g_external_image_data_cache, sizeof(g_external_image_data_cache));
    int cached_src_y = -1;

    if (data_buf == NULL)
    {
        return;
    }

    count = egui_image_std_prepare_resize_src_x_map_limit(src_x_map, x, x_total, width_radio);

    if (canvas->mask != NULL)
    {
        if (canvas->mask->api->mask_get_row_range != NULL)
        {
            egui_dim_t rr_x_start, rr_x_end;
            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                egui_dim_t rr_sy = y_base + y_;
                egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                if (cached_src_y != src_y)
                {
                    uint32_t row_start = src_y * image->width;

                    egui_image_std_load_data_resource(data_buf, image, row_start << 1, data_row_size);
                    cached_src_y = src_y;
                }

                const uint16_t *src_row = (const uint16_t *)data_buf;
                int rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &rr_x_start, &rr_x_end);

                if (rr_res == EGUI_MASK_ROW_OUTSIDE)
                {
                    continue;
                }

                if (rr_res == EGUI_MASK_ROW_INSIDE)
                {
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
                    if (canvas_alpha == EGUI_ALPHA_100)
                    {
                        for (egui_dim_t i = 0; i < count; i++)
                        {
                            dst_row[i] = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                        }
                    }
                    else
                    {
                        for (egui_dim_t i = 0; i < count; i++)
                        {
                            color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                            egui_image_std_blend_resize_pixel(&dst_row[i], color, canvas_alpha);
                        }
                    }
                }
                else
                {
                    egui_dim_t visible_x_start = screen_x_start;
                    egui_dim_t visible_x_end = x_base + x_total;
                    if (canvas->mask->api->mask_get_row_visible_range != NULL &&
                        !canvas->mask->api->mask_get_row_visible_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &visible_x_start, &visible_x_end))
                    {
                        continue;
                    }
                    egui_dim_t rr_img_xs = EGUI_MAX(x, rr_x_start - x_base);
                    egui_dim_t rr_img_xe = EGUI_MIN(x_total, rr_x_end - x_base);
                    egui_dim_t vis_img_xs = EGUI_MAX(x, visible_x_start - x_base);
                    egui_dim_t vis_img_xe = EGUI_MIN(x_total, visible_x_end - x_base);

                    for (egui_dim_t x_ = vis_img_xs; x_ < EGUI_MIN(rr_img_xs, vis_img_xe); x_++)
                    {
                        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[x_ - x]]);
                        egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, EGUI_ALPHA_100);
                    }

                    if (rr_img_xs < rr_img_xe)
                    {
                        egui_dim_t mid_offset = rr_img_xs - x;
                        egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + rr_img_xs - canvas->pfb_location_in_base_view.x)];
                        if (canvas_alpha == EGUI_ALPHA_100)
                        {
                            for (egui_dim_t i = mid_offset; i < (rr_img_xe - x); i++)
                            {
                                dst_row[i - mid_offset] = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                            }
                        }
                        else
                        {
                            for (egui_dim_t i = mid_offset; i < (rr_img_xe - x); i++)
                            {
                                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                                egui_image_std_blend_resize_pixel(&dst_row[i - mid_offset], color, canvas_alpha);
                            }
                        }
                    }

                    for (egui_dim_t x_ = EGUI_MAX(rr_img_xe, vis_img_xs); x_ < vis_img_xe; x_++)
                    {
                        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[x_ - x]]);
                        egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, EGUI_ALPHA_100);
                    }
                }
            }
        }
        else
        {
            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                if (cached_src_y != src_y)
                {
                    uint32_t row_start = src_y * image->width;

                    egui_image_std_load_data_resource(data_buf, image, row_start << 1, data_row_size);
                    cached_src_y = src_y;
                }

                const uint16_t *src_row = (const uint16_t *)data_buf;
                for (egui_dim_t i = 0; i < count; i++)
                {
                    color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                    egui_canvas_draw_point_limit(screen_x_start + i, y_base + y_, color, EGUI_ALPHA_100);
                }
            }
        }
    }
    else
    {
        for (egui_dim_t y_ = y; y_ < y_total; y_++)
        {
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
            src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
            if (cached_src_y != src_y)
            {
                uint32_t row_start = src_y * image->width;

                egui_image_std_load_data_resource(data_buf, image, row_start << 1, data_row_size);
                cached_src_y = src_y;
            }

            const uint16_t *src_row = (const uint16_t *)data_buf;
            if (canvas_alpha == EGUI_ALPHA_100)
            {
                for (egui_dim_t i = 0; i < count; i++)
                {
                    dst_row[i] = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                }
            }
            else
            {
                for (egui_dim_t i = 0; i < count; i++)
                {
                    color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                    egui_image_std_blend_resize_pixel(&dst_row[i], color, canvas_alpha);
                }
            }
        }
    }

    egui_image_std_release_external_buffer(data_buf, g_external_image_data_cache);
}
#endif

#define EGUI_IMAGE_STD_DRAW_IMAGE_RESIZE_FUNC_DEFINE(_get_pixel_func, self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio)                 \
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;                                                                                         \
    egui_color_t color;                                                                                                                                        \
    egui_alpha_t alpha;                                                                                                                                        \
    egui_dim_t src_y;                                                                                                                                          \
    egui_canvas_t *canvas = egui_canvas_get_canvas();                                                                                                          \
    egui_alpha_t canvas_alpha = canvas->alpha;                                                                                                                 \
    egui_dim_t pfb_width = canvas->pfb_region.size.width;                                                                                                      \
    egui_dim_t screen_x_start = x_base + x;                                                                                                                    \
    egui_dim_t dst_x_start = screen_x_start - canvas->pfb_location_in_base_view.x;                                                                             \
    egui_dim_t count = x_total - x;                                                                                                                            \
    egui_dim_t src_x_map[EGUI_CONFIG_PFB_WIDTH];                                                                                                               \
    count = egui_image_std_prepare_resize_src_x_map(src_x_map, x, x_total, width_radio);                                                                       \
    if (canvas->mask != NULL)                                                                                                                                  \
    {                                                                                                                                                          \
        if (canvas->mask->api->mask_get_row_range != NULL)                                                                                                     \
        {                                                                                                                                                      \
            egui_dim_t rr_x_start, rr_x_end;                                                                                                                   \
            for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                        \
            {                                                                                                                                                  \
                egui_dim_t rr_sy = y_base + y_;                                                                                                                \
                egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;                                                                                \
                src_y = (egui_dim_t)EGUI_FLOAT_MULT(y_, height_radio);                                                                                         \
                int rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &rr_x_start, &rr_x_end);             \
                if (rr_res == EGUI_MASK_ROW_OUTSIDE)                                                                                                           \
                {                                                                                                                                              \
                    continue;                                                                                                                                  \
                }                                                                                                                                              \
                if (rr_res == EGUI_MASK_ROW_INSIDE)                                                                                                            \
                {                                                                                                                                              \
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];                                                                 \
                    for (egui_dim_t i = 0; i < count; i++)                                                                                                     \
                    {                                                                                                                                          \
                        _get_pixel_func(image, src_x_map[i], src_y, &color, &alpha);                                                                           \
                        alpha = egui_color_alpha_mix(canvas_alpha, alpha);                                                                                     \
                        egui_image_std_blend_resize_pixel(&dst_row[i], color, alpha);                                                                          \
                    }                                                                                                                                          \
                }                                                                                                                                              \
                else                                                                                                                                           \
                {                                                                                                                                              \
                    egui_dim_t visible_x_start = screen_x_start;                                                                                               \
                    egui_dim_t visible_x_end = x_base + x_total;                                                                                               \
                    if (canvas->mask->api->mask_get_row_visible_range != NULL &&                                                                               \
                        !canvas->mask->api->mask_get_row_visible_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &visible_x_start,                \
                                                                       &visible_x_end))                                                                        \
                    {                                                                                                                                          \
                        continue;                                                                                                                              \
                    }                                                                                                                                          \
                    egui_dim_t rr_img_xs = EGUI_MAX(x, rr_x_start - x_base);                                                                                   \
                    egui_dim_t rr_img_xe = EGUI_MIN(x_total, rr_x_end - x_base);                                                                               \
                    egui_dim_t vis_img_xs = EGUI_MAX(x, visible_x_start - x_base);                                                                             \
                    egui_dim_t vis_img_xe = EGUI_MIN(x_total, visible_x_end - x_base);                                                                         \
                    for (egui_dim_t x_ = vis_img_xs; x_ < EGUI_MIN(rr_img_xs, vis_img_xe); x_++)                                                               \
                    {                                                                                                                                          \
                        _get_pixel_func(image, src_x_map[x_ - x], src_y, &color, &alpha);                                                                      \
                        egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, alpha);                                                                        \
                    }                                                                                                                                          \
                    if (rr_img_xs < rr_img_xe)                                                                                                                 \
                    {                                                                                                                                          \
                        egui_dim_t mid_offset = rr_img_xs - x;                                                                                                 \
                        egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + rr_img_xs - canvas->pfb_location_in_base_view.x)];              \
                        for (egui_dim_t i = mid_offset; i < (rr_img_xe - x); i++)                                                                              \
                        {                                                                                                                                      \
                            _get_pixel_func(image, src_x_map[i], src_y, &color, &alpha);                                                                       \
                            alpha = egui_color_alpha_mix(canvas_alpha, alpha);                                                                                 \
                            egui_image_std_blend_resize_pixel(&dst_row[i - mid_offset], color, alpha);                                                         \
                        }                                                                                                                                      \
                    }                                                                                                                                          \
                    for (egui_dim_t x_ = EGUI_MAX(rr_img_xe, vis_img_xs); x_ < vis_img_xe; x_++)                                                               \
                    {                                                                                                                                          \
                        _get_pixel_func(image, src_x_map[x_ - x], src_y, &color, &alpha);                                                                      \
                        egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, alpha);                                                                        \
                    }                                                                                                                                          \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                        \
            {                                                                                                                                                  \
                src_y = (egui_dim_t)EGUI_FLOAT_MULT(y_, height_radio);                                                                                         \
                for (egui_dim_t i = 0; i < count; i++)                                                                                                         \
                {                                                                                                                                              \
                    _get_pixel_func(image, src_x_map[i], src_y, &color, &alpha);                                                                               \
                    egui_canvas_draw_point_limit(screen_x_start + i, y_base + y_, color, alpha);                                                               \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    }                                                                                                                                                          \
    else                                                                                                                                                       \
    {                                                                                                                                                          \
        for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                            \
        {                                                                                                                                                      \
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;                                                                            \
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];                                                                         \
            src_y = (egui_dim_t)EGUI_FLOAT_MULT(y_, height_radio);                                                                                             \
            for (egui_dim_t i = 0; i < count; i++)                                                                                                             \
            {                                                                                                                                                  \
                _get_pixel_func(image, src_x_map[i], src_y, &color, &alpha);                                                                                   \
                alpha = egui_color_alpha_mix(canvas_alpha, alpha);                                                                                             \
                egui_image_std_blend_resize_pixel(&dst_row[i], color, alpha);                                                                                  \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    }

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
void egui_image_std_set_image_resize_rgb565_8(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                              egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio)
{
    egui_image_std_set_image_resize_rgb565_8_common(self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
void egui_image_std_set_image_resize_rgb565_4(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                              egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio)
{
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_std_info_t *image_info = (egui_image_std_info_t *)self->res;
    if (image_info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        egui_image_std_draw_image_resize_external_alpha(self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio, (image_info->width + 1) >> 1,
                                                        egui_image_std_get_col_pixel_rgb565_4,
                                                        egui_image_std_blend_rgb565_alpha4_mapped_row);
        return;
    }
#endif
    egui_image_std_set_image_resize_rgb565_4_common(self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
void egui_image_std_set_image_resize_rgb565_2(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                              egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio)
{
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_std_info_t *image_info = (egui_image_std_info_t *)self->res;
    if (image_info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        egui_image_std_draw_image_resize_external_alpha(self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio, (image_info->width + 3) >> 2,
                                                        egui_image_std_get_col_pixel_rgb565_2,
                                                        egui_image_std_blend_rgb565_alpha2_mapped_row);
        return;
    }
#endif
    egui_image_std_set_image_resize_rgb565_2_common(self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
void egui_image_std_set_image_resize_rgb565_1(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                              egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio)
{
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_std_info_t *image_info = (egui_image_std_info_t *)self->res;
    if (image_info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        egui_image_std_draw_image_resize_external_alpha(self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio, (image_info->width + 7) >> 3,
                                                        egui_image_std_get_col_pixel_rgb565_1,
                                                        egui_image_std_blend_rgb565_alpha1_mapped_row);
        return;
    }
#endif
    egui_image_std_set_image_resize_rgb565_1_common(self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
void egui_image_std_set_image_resize_rgb565(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                            egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        egui_image_std_set_image_resize_rgb565_external(self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio);
        return;
    }
#endif
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_color_t color;
    egui_alpha_t canvas_alpha = canvas->alpha;
    egui_dim_t src_y;
    egui_dim_t pfb_width = canvas->pfb_region.size.width;
    egui_dim_t screen_x_start = x_base + x;
    egui_dim_t dst_x_start = screen_x_start - canvas->pfb_location_in_base_view.x;
    egui_dim_t count = x_total - x;
    egui_dim_t src_x_map[EGUI_CONFIG_PFB_WIDTH];
    const uint16_t *src_pixels = image->data_buf;

    count = egui_image_std_prepare_resize_src_x_map_limit(src_x_map, x, x_total, width_radio);

    if (canvas->mask != NULL)
    {
        if (canvas->mask->api->mask_get_row_range != NULL)
        {
            egui_dim_t rr_x_start, rr_x_end;
            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                egui_dim_t rr_sy = y_base + y_;
                egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                const uint16_t *src_row = &src_pixels[src_y * image->width];
                int rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &rr_x_start, &rr_x_end);

                if (rr_res == EGUI_MASK_ROW_OUTSIDE)
                {
                    continue;
                }

                if (rr_res == EGUI_MASK_ROW_INSIDE)
                {
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
                    if (canvas_alpha == EGUI_ALPHA_100)
                    {
                        for (egui_dim_t i = 0; i < count; i++)
                        {
                            dst_row[i] = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                        }
                    }
                    else
                    {
                        for (egui_dim_t i = 0; i < count; i++)
                        {
                            color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                            egui_image_std_blend_resize_pixel(&dst_row[i], color, canvas_alpha);
                        }
                    }
                }
                else
                {
                    egui_dim_t visible_x_start = screen_x_start;
                    egui_dim_t visible_x_end = x_base + x_total;
                    if (canvas->mask->api->mask_get_row_visible_range != NULL &&
                        !canvas->mask->api->mask_get_row_visible_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &visible_x_start, &visible_x_end))
                    {
                        continue;
                    }
                    egui_dim_t rr_img_xs = EGUI_MAX(x, rr_x_start - x_base);
                    egui_dim_t rr_img_xe = EGUI_MIN(x_total, rr_x_end - x_base);
                    egui_dim_t vis_img_xs = EGUI_MAX(x, visible_x_start - x_base);
                    egui_dim_t vis_img_xe = EGUI_MIN(x_total, visible_x_end - x_base);

                    for (egui_dim_t x_ = vis_img_xs; x_ < EGUI_MIN(rr_img_xs, vis_img_xe); x_++)
                    {
                        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[x_ - x]]);
                        egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, EGUI_ALPHA_100);
                    }

                    if (rr_img_xs < rr_img_xe)
                    {
                        egui_dim_t mid_offset = rr_img_xs - x;
                        egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + rr_img_xs - canvas->pfb_location_in_base_view.x)];
                        if (canvas_alpha == EGUI_ALPHA_100)
                        {
                            for (egui_dim_t i = mid_offset; i < (rr_img_xe - x); i++)
                            {
                                dst_row[i - mid_offset] = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                            }
                        }
                        else
                        {
                            for (egui_dim_t i = mid_offset; i < (rr_img_xe - x); i++)
                            {
                                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                                egui_image_std_blend_resize_pixel(&dst_row[i - mid_offset], color, canvas_alpha);
                            }
                        }
                    }

                    for (egui_dim_t x_ = EGUI_MAX(rr_img_xe, vis_img_xs); x_ < vis_img_xe; x_++)
                    {
                        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[x_ - x]]);
                        egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, EGUI_ALPHA_100);
                    }
                }
            }
        }
        else
        {
            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                const uint16_t *src_row = &src_pixels[src_y * image->width];
                for (egui_dim_t i = 0; i < count; i++)
                {
                    color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                    egui_canvas_draw_point_limit(screen_x_start + i, y_base + y_, color, EGUI_ALPHA_100);
                }
            }
        }
    }
    else
    {
        for (egui_dim_t y_ = y; y_ < y_total; y_++)
        {
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
            src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
            const uint16_t *src_row = &src_pixels[src_y * image->width];

            if (canvas_alpha == EGUI_ALPHA_100)
            {
                for (egui_dim_t i = 0; i < count; i++)
                {
                    dst_row[i] = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                }
            }
            else
            {
                for (egui_dim_t i = 0; i < count; i++)
                {
                    color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                    egui_image_std_blend_resize_pixel(&dst_row[i], color, canvas_alpha);
                }
            }
        }
    }
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
void egui_image_std_set_image_resize_rgb32(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                           egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio)
{
    EGUI_IMAGE_STD_DRAW_IMAGE_RESIZE_FUNC_DEFINE(egui_image_std_get_pixel_rgb32, self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32

void egui_image_std_draw_image_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    if (width == 0 || height == 0)
    {
        return;
    }
    if (width == image->width && height == image->height)
    {
        egui_image_std_draw_image(self, x, y);
        return;
    }
    // egui_color_t color;
    // egui_alpha_t alpha;
    egui_float_t width_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->width), EGUI_FLOAT_VALUE_INT(width));
    egui_float_t height_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->height), EGUI_FLOAT_VALUE_INT(height));
    // const uint32_t *p_data = image->data_buf;
    // const uint8_t* p_alpha = image->alpha_buf;
    // egui_dim_t src_x;
    // egui_dim_t src_y;

    egui_dim_t x_total;
    egui_dim_t y_total;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, egui_canvas_get_base_view_work_region(), &region);

    if (egui_region_is_empty(&region))
    {
        return;
    }

    // change to image coordinate.
    region.location.x -= x;
    region.location.y -= y;

    // for speed, calculate total positions outside of the loop
    x_total = region.location.x + region.size.width;
    y_total = region.location.y + region.size.height;

    if (image)
    {
        switch (image->data_type)
        {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
        case EGUI_IMAGE_DATA_TYPE_RGB32:
            egui_image_std_set_image_resize_rgb32(self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
            break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
        case EGUI_IMAGE_DATA_TYPE_RGB565:
            if (image->alpha_buf == NULL)
            {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
                egui_image_std_set_image_resize_rgb565(self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
            }
            else
            {
                switch (image->alpha_type)
                {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
                case EGUI_IMAGE_ALPHA_TYPE_1:
                    egui_image_std_set_image_resize_rgb565_1(self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
                case EGUI_IMAGE_ALPHA_TYPE_2:
                    egui_image_std_set_image_resize_rgb565_2(self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
                case EGUI_IMAGE_ALPHA_TYPE_4:
                    egui_image_std_set_image_resize_rgb565_4(self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
                case EGUI_IMAGE_ALPHA_TYPE_8:
                    egui_image_std_set_image_resize_rgb565_8(self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
                default:
                    EGUI_ASSERT(0);
                    break;
                }
            }
            break;
        default:
            EGUI_ASSERT(0);
            break;
        }
    }
}

#endif // EGUI_CONFIG_REDUCE_IMAGE_CODE_SIZE

// Alpha-only image color draw functions
// These functions draw alpha-only images (EGUI_IMAGE_DATA_TYPE_ALPHA) with a specified color,
// following the same pattern as font rendering (egui_font_std_draw_4).

#define EGUI_IMAGE_STD_DRAW_ALPHA_COLOR_FUNC(_get_col_alpha_func, _alpha_row_size_expr, self, x, y, x_total, y_total, x_base, y_base, color, color_alpha)      \
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;                                                                                         \
    egui_alpha_t pixel_alpha;                                                                                                                                  \
    egui_canvas_t *canvas = egui_canvas_get_canvas();                                                                                                          \
    uint16_t alpha_row_size = _alpha_row_size_expr;                                                                                                            \
    EGUI_UNUSED(alpha_row_size);                                                                                                                               \
    for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                                \
    {                                                                                                                                                          \
        uint32_t row_start_alpha = y_ * alpha_row_size;                                                                                                        \
        const uint8_t *p_data = (const uint8_t *)image->data_buf + row_start_alpha;                                                                            \
        if (canvas->mask != NULL)                                                                                                                              \
        {                                                                                                                                                      \
            for (egui_dim_t x_ = x; x_ < x_total; x_++)                                                                                                        \
            {                                                                                                                                                  \
                _get_col_alpha_func(p_data, x_, &pixel_alpha);                                                                                                 \
                if (pixel_alpha)                                                                                                                               \
                {                                                                                                                                              \
                    if (color_alpha == EGUI_ALPHA_100)                                                                                                         \
                    {                                                                                                                                          \
                        egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, pixel_alpha);                                                        \
                    }                                                                                                                                          \
                    else                                                                                                                                       \
                    {                                                                                                                                          \
                        egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, pixel_alpha * color_alpha / EGUI_ALPHA_100);                         \
                    }                                                                                                                                          \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            for (egui_dim_t x_ = x; x_ < x_total; x_++)                                                                                                        \
            {                                                                                                                                                  \
                _get_col_alpha_func(p_data, x_, &pixel_alpha);                                                                                                 \
                if (pixel_alpha)                                                                                                                               \
                {                                                                                                                                              \
                    if (color_alpha == EGUI_ALPHA_100)                                                                                                         \
                    {                                                                                                                                          \
                        egui_canvas_draw_point_limit_skip_mask((x_base + x_), (y_base + y_), color, pixel_alpha);                                              \
                    }                                                                                                                                          \
                    else                                                                                                                                       \
                    {                                                                                                                                          \
                        egui_canvas_draw_point_limit_skip_mask((x_base + x_), (y_base + y_), color, pixel_alpha * color_alpha / EGUI_ALPHA_100);               \
                    }                                                                                                                                          \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    }

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8
static void egui_image_std_set_image_alpha_color_8(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total,
                                                   egui_dim_t x_base, egui_dim_t y_base, egui_color_t color, egui_alpha_t color_alpha)
{
    EGUI_IMAGE_STD_DRAW_ALPHA_COLOR_FUNC(egui_image_std_get_col_alpha_8, image->width, self, x, y, x_total, y_total, x_base, y_base, color, color_alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4
static void egui_image_std_set_image_alpha_color_4(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total,
                                                   egui_dim_t x_base, egui_dim_t y_base, egui_color_t color, egui_alpha_t color_alpha)
{
    EGUI_IMAGE_STD_DRAW_ALPHA_COLOR_FUNC(egui_image_std_get_col_alpha_4, ((image->width + 1) >> 1), self, x, y, x_total, y_total, x_base, y_base, color,
                                         color_alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2
static void egui_image_std_set_image_alpha_color_2(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total,
                                                   egui_dim_t x_base, egui_dim_t y_base, egui_color_t color, egui_alpha_t color_alpha)
{
    EGUI_IMAGE_STD_DRAW_ALPHA_COLOR_FUNC(egui_image_std_get_col_alpha_2, ((image->width + 3) >> 2), self, x, y, x_total, y_total, x_base, y_base, color,
                                         color_alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1
static void egui_image_std_set_image_alpha_color_1(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total,
                                                   egui_dim_t x_base, egui_dim_t y_base, egui_color_t color, egui_alpha_t color_alpha)
{
    EGUI_IMAGE_STD_DRAW_ALPHA_COLOR_FUNC(egui_image_std_get_col_alpha_1, ((image->width + 7) >> 3), self, x, y, x_total, y_total, x_base, y_base, color,
                                         color_alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1

void egui_image_std_draw_image_color(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t color_alpha)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_dim_t width = image->width;
    egui_dim_t height = image->height;

    egui_dim_t x_total;
    egui_dim_t y_total;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, egui_canvas_get_base_view_work_region(), &region);

    if (egui_region_is_empty(&region))
    {
        return;
    }

    // change to image coordinate.
    region.location.x -= x;
    region.location.y -= y;

    // for speed, calculate total positions outside of the loop
    x_total = region.location.x + region.size.width;
    y_total = region.location.y + region.size.height;

    if (image)
    {
        switch (image->alpha_type)
        {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1
        case EGUI_IMAGE_ALPHA_TYPE_1:
            egui_image_std_set_image_alpha_color_1(self, region.location.x, region.location.y, x_total, y_total, x, y, color, color_alpha);
            break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2
        case EGUI_IMAGE_ALPHA_TYPE_2:
            egui_image_std_set_image_alpha_color_2(self, region.location.x, region.location.y, x_total, y_total, x, y, color, color_alpha);
            break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4
        case EGUI_IMAGE_ALPHA_TYPE_4:
            egui_image_std_set_image_alpha_color_4(self, region.location.x, region.location.y, x_total, y_total, x, y, color, color_alpha);
            break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8
        case EGUI_IMAGE_ALPHA_TYPE_8:
            egui_image_std_set_image_alpha_color_8(self, region.location.x, region.location.y, x_total, y_total, x, y, color, color_alpha);
            break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8
        default:
            EGUI_ASSERT(0);
            break;
        }
    }
}

void egui_image_std_draw_image_resize_color(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color,
                                            egui_alpha_t color_alpha)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    if (width == 0 || height == 0)
    {
        return;
    }
    egui_float_t width_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->width), EGUI_FLOAT_VALUE_INT(width));
    egui_float_t height_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->height), EGUI_FLOAT_VALUE_INT(height));
    egui_alpha_t pixel_alpha;
    egui_dim_t src_x;
    egui_dim_t src_y;

    egui_dim_t x_total;
    egui_dim_t y_total;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, egui_canvas_get_base_view_work_region(), &region);

    if (egui_region_is_empty(&region))
    {
        return;
    }

    // change to image coordinate.
    region.location.x -= x;
    region.location.y -= y;

    // for speed, calculate total positions outside of the loop
    x_total = region.location.x + region.size.width;
    y_total = region.location.y + region.size.height;

    if (image)
    {
        // Use a generic resize approach: map destination pixel to source pixel, then extract alpha
        uint16_t alpha_row_size_8 = image->width;
        uint16_t alpha_row_size_4 = ((image->width + 1) >> 1);
        uint16_t alpha_row_size_2 = ((image->width + 3) >> 2);
        uint16_t alpha_row_size_1 = ((image->width + 7) >> 3);
        EGUI_UNUSED(alpha_row_size_8);
        EGUI_UNUSED(alpha_row_size_4);
        EGUI_UNUSED(alpha_row_size_2);
        EGUI_UNUSED(alpha_row_size_1);
        egui_canvas_t *canvas = egui_canvas_get_canvas();

        for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)
        {
            for (egui_dim_t x_ = region.location.x; x_ < x_total; x_++)
            {
                // For speed, use nearestScaler to scale the image.
                src_x = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(x_, width_radio);
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);

                pixel_alpha = 0;
                switch (image->alpha_type)
                {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8
                case EGUI_IMAGE_ALPHA_TYPE_8:
                {
                    const uint8_t *p_data = (const uint8_t *)image->data_buf + src_y * alpha_row_size_8;
                    egui_image_std_get_col_alpha_8(p_data, src_x, &pixel_alpha);
                    break;
                }
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4
                case EGUI_IMAGE_ALPHA_TYPE_4:
                {
                    const uint8_t *p_data = (const uint8_t *)image->data_buf + src_y * alpha_row_size_4;
                    egui_image_std_get_col_alpha_4(p_data, src_x, &pixel_alpha);
                    break;
                }
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2
                case EGUI_IMAGE_ALPHA_TYPE_2:
                {
                    const uint8_t *p_data = (const uint8_t *)image->data_buf + src_y * alpha_row_size_2;
                    egui_image_std_get_col_alpha_2(p_data, src_x, &pixel_alpha);
                    break;
                }
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1
                case EGUI_IMAGE_ALPHA_TYPE_1:
                {
                    const uint8_t *p_data = (const uint8_t *)image->data_buf + src_y * alpha_row_size_1;
                    egui_image_std_get_col_alpha_1(p_data, src_x, &pixel_alpha);
                    break;
                }
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1
                default:
                    EGUI_ASSERT(0);
                    break;
                }

                if (pixel_alpha)
                {
                    egui_alpha_t final_alpha = (color_alpha == EGUI_ALPHA_100) ? pixel_alpha : (pixel_alpha * color_alpha / EGUI_ALPHA_100);
                    if (canvas->mask != NULL)
                    {
                        egui_canvas_draw_point_limit((x + x_), (y + y_), color, final_alpha);
                    }
                    else
                    {
                        egui_canvas_draw_point_limit_skip_mask((x + x_), (y + y_), color, final_alpha);
                    }
                }
            }
        }
    }
}

void egui_image_std_get_width_height(const egui_image_t *self, egui_dim_t *width, egui_dim_t *height)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    *width = image->width;
    *height = image->height;
}

const egui_image_api_t egui_image_std_t_api_table = {.get_point = egui_image_std_get_point,
                                                     .get_point_resize = egui_image_std_get_point_resize,
                                                     .draw_image = egui_image_std_draw_image,
                                                     .draw_image_resize = egui_image_std_draw_image_resize};

void egui_image_std_init(egui_image_t *self, const void *res)
{
    EGUI_LOCAL_INIT(egui_image_std_t);
    // call super init.
    egui_image_init(self, res);

    // update api.
    self->api = &egui_image_std_t_api_table;
}
