#include <stdio.h>
#include <assert.h>

#include "egui_image_std.h"
#include "core/egui_api.h"
#include "mask/egui_mask_circle.h"
#include "mask/egui_mask_round_rectangle.h"
#include "mask/egui_mask_image.h"

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
typedef void(egui_image_std_blend_repeat2_row_func)(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                    egui_dim_t src_x_start, egui_dim_t src_count, egui_alpha_t canvas_alpha);

void egui_image_std_load_data_resource(void *dest, egui_image_std_info_t *image, uint32_t start_offset, uint32_t size);
void egui_image_std_load_alpha_resource(void *dest, egui_image_std_info_t *image, uint32_t start_offset, uint32_t size);

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

typedef struct
{
    void *data_buf;
    void *alpha_buf;
    uint32_t data_row_size;
    uint32_t alpha_row_size;
    egui_dim_t chunk_row_start;
    egui_dim_t chunk_row_count;
    egui_dim_t rows_per_chunk;
} egui_image_std_external_alpha_row_cache_t;

static int egui_image_std_init_external_alpha_row_cache(egui_image_std_external_alpha_row_cache_t *cache, uint32_t data_row_size, uint32_t alpha_row_size)
{
    uint32_t data_rows;
    uint32_t alpha_rows;
    uint32_t data_chunk_size;
    uint32_t alpha_chunk_size;

    cache->data_buf = NULL;
    cache->alpha_buf = NULL;
    cache->data_row_size = data_row_size;
    cache->alpha_row_size = alpha_row_size;
    cache->chunk_row_start = -1;
    cache->chunk_row_count = 0;
    cache->rows_per_chunk = 0;

    if (data_row_size == 0 || alpha_row_size == 0)
    {
        return 0;
    }

    data_rows = sizeof(g_external_image_data_cache) / data_row_size;
    alpha_rows = sizeof(g_external_image_alpha_cache) / alpha_row_size;
    if (data_rows == 0)
    {
        data_rows = 1;
    }
    if (alpha_rows == 0)
    {
        alpha_rows = 1;
    }

    cache->rows_per_chunk = (egui_dim_t)((data_rows < alpha_rows) ? data_rows : alpha_rows);
    data_chunk_size = data_row_size * (uint32_t)cache->rows_per_chunk;
    alpha_chunk_size = alpha_row_size * (uint32_t)cache->rows_per_chunk;

    cache->data_buf = egui_image_std_acquire_external_buffer(data_chunk_size, g_external_image_data_cache, sizeof(g_external_image_data_cache));
    cache->alpha_buf = egui_image_std_acquire_external_buffer(alpha_chunk_size, g_external_image_alpha_cache, sizeof(g_external_image_alpha_cache));
    if (cache->data_buf == NULL || cache->alpha_buf == NULL)
    {
        egui_image_std_release_external_buffer(cache->data_buf, g_external_image_data_cache);
        egui_image_std_release_external_buffer(cache->alpha_buf, g_external_image_alpha_cache);
        cache->data_buf = NULL;
        cache->alpha_buf = NULL;
        return 0;
    }

    return 1;
}

static void egui_image_std_release_external_alpha_row_cache(egui_image_std_external_alpha_row_cache_t *cache)
{
    if (cache == NULL)
    {
        return;
    }

    egui_image_std_release_external_buffer(cache->data_buf, g_external_image_data_cache);
    egui_image_std_release_external_buffer(cache->alpha_buf, g_external_image_alpha_cache);
}

static int egui_image_std_load_external_alpha_row_cache(egui_image_std_external_alpha_row_cache_t *cache, egui_image_std_info_t *image, egui_dim_t row)
{
    egui_dim_t rows_to_load;

    if (cache == NULL || image == NULL || row < 0 || row >= image->height)
    {
        return 0;
    }

    if (row >= cache->chunk_row_start && row < (cache->chunk_row_start + cache->chunk_row_count))
    {
        return 1;
    }

    rows_to_load = image->height - row;
    if (rows_to_load > cache->rows_per_chunk)
    {
        rows_to_load = cache->rows_per_chunk;
    }

    egui_image_std_load_data_resource(cache->data_buf, image, (uint32_t)row * cache->data_row_size, (uint32_t)rows_to_load * cache->data_row_size);
    egui_image_std_load_alpha_resource(cache->alpha_buf, image, (uint32_t)row * cache->alpha_row_size, (uint32_t)rows_to_load * cache->alpha_row_size);

    cache->chunk_row_start = row;
    cache->chunk_row_count = rows_to_load;
    return 1;
}

__EGUI_STATIC_INLINE__ const uint16_t *egui_image_std_get_external_alpha_row_data(const egui_image_std_external_alpha_row_cache_t *cache, egui_dim_t row)
{
    return (const uint16_t *)((const uint8_t *)cache->data_buf + (uint32_t)(row - cache->chunk_row_start) * cache->data_row_size);
}

__EGUI_STATIC_INLINE__ const uint8_t *egui_image_std_get_external_alpha_row_alpha(const egui_image_std_external_alpha_row_cache_t *cache, egui_dim_t row)
{
    return (const uint8_t *)cache->alpha_buf + (uint32_t)(row - cache->chunk_row_start) * cache->alpha_row_size;
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
    egui_float_t src_x_acc = (egui_float_t)((int64_t)x * (int64_t)width_radio);

    EGUI_ASSERT(count <= EGUI_CONFIG_PFB_WIDTH);
    for (egui_dim_t i = 0; i < count; i++)
    {
        src_x_map[i] = EGUI_FLOAT_INT_PART(src_x_acc);
        src_x_acc += width_radio;
    }

    return count;
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_image_std_prepare_resize_src_x_map_limit(egui_dim_t *src_x_map, egui_dim_t x, egui_dim_t x_total,
                                                                                egui_float_t width_radio)
{
    egui_dim_t count = x_total - x;
    egui_float_t width_step = (width_radio >> 8);
    egui_float_t src_x_acc = x * width_step;

    EGUI_ASSERT(count <= EGUI_CONFIG_PFB_WIDTH);
    for (egui_dim_t i = 0; i < count; i++)
    {
        src_x_map[i] = src_x_acc >> 8;
        src_x_acc += width_step;
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

#define EGUI_IMAGE_STD_BLEND_RGB565_PACKED_ALPHA_REPEAT2_ROW_DEFINE(_func_name, _get_alpha_func)                                                           \
    __EGUI_STATIC_INLINE__ void _func_name(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,                             \
                                           egui_dim_t src_x_start, egui_dim_t src_count, egui_alpha_t canvas_alpha)                                       \
    {                                                                                                                                                       \
        if (canvas_alpha == 0 || src_count <= 0)                                                                                                           \
        {                                                                                                                                                   \
            return;                                                                                                                                         \
        }                                                                                                                                                   \
                                                                                                                                                            \
        if (canvas_alpha == EGUI_ALPHA_100)                                                                                                                 \
        {                                                                                                                                                   \
            for (egui_dim_t i = 0; i < src_count; i++)                                                                                                      \
            {                                                                                                                                               \
                egui_dim_t src_x = src_x_start + i;                                                                                                         \
                egui_alpha_t src_alpha = _get_alpha_func(src_alpha_row, src_x);                                                                             \
                egui_dim_t dst_x = i << 1;                                                                                                                  \
                                                                                                                                                            \
                if (src_alpha == 0)                                                                                                                         \
                {                                                                                                                                           \
                    continue;                                                                                                                                \
                }                                                                                                                                           \
                                                                                                                                                            \
                egui_color_int_t pixel = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);                                                                           \
                if (src_alpha == EGUI_ALPHA_100)                                                                                                            \
                {                                                                                                                                           \
                    dst_row[dst_x] = pixel;                                                                                                                 \
                    dst_row[dst_x + 1] = pixel;                                                                                                             \
                }                                                                                                                                           \
                else                                                                                                                                        \
                {                                                                                                                                           \
                    egui_color_t color;                                                                                                                     \
                    color.full = pixel;                                                                                                                     \
                    egui_image_std_blend_resize_pixel(&dst_row[dst_x], color, src_alpha);                                                                  \
                    egui_image_std_blend_resize_pixel(&dst_row[dst_x + 1], color, src_alpha);                                                              \
                }                                                                                                                                           \
            }                                                                                                                                               \
            return;                                                                                                                                         \
        }                                                                                                                                                   \
                                                                                                                                                            \
        for (egui_dim_t i = 0; i < src_count; i++)                                                                                                          \
        {                                                                                                                                                   \
            egui_dim_t src_x = src_x_start + i;                                                                                                             \
            egui_alpha_t src_alpha = _get_alpha_func(src_alpha_row, src_x);                                                                                 \
            egui_dim_t dst_x = i << 1;                                                                                                                      \
            egui_alpha_t alpha;                                                                                                                             \
                                                                                                                                                            \
            if (src_alpha == 0)                                                                                                                             \
            {                                                                                                                                               \
                continue;                                                                                                                                    \
            }                                                                                                                                               \
                                                                                                                                                            \
            alpha = (src_alpha == EGUI_ALPHA_100) ? canvas_alpha : egui_color_alpha_mix(canvas_alpha, src_alpha);                                          \
            if (alpha == 0)                                                                                                                                 \
            {                                                                                                                                               \
                continue;                                                                                                                                    \
            }                                                                                                                                               \
                                                                                                                                                            \
            egui_color_t color;                                                                                                                             \
            color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);                                                                                           \
            egui_image_std_blend_resize_pixel(&dst_row[dst_x], color, alpha);                                                                              \
            egui_image_std_blend_resize_pixel(&dst_row[dst_x + 1], color, alpha);                                                                          \
        }                                                                                                                                                   \
    }

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha8_row(egui_color_int_t *dst_row, const uint16_t *src_pixels, const uint8_t *src_alpha_row,
                                                                   egui_dim_t count, egui_alpha_t canvas_alpha)
{
    egui_dim_t i = 0;

    if (canvas_alpha == 0)
    {
        return;
    }

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        while (i < count)
        {
            while (i + 4 <= count && *(const uint32_t *)&src_alpha_row[i] == 0x00000000)
            {
                i += 4;
            }
            while (i < count && src_alpha_row[i] == 0)
            {
                i++;
            }

            {
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
                    egui_image_std_copy_rgb565_row(&dst_row[opaque_start], &src_pixels[opaque_start], i - opaque_start);
                    continue;
                }
            }

            if (i < count)
            {
                egui_alpha_t alpha = src_alpha_row[i];
                if (alpha != 0)
                {
                    egui_color_t color;
                    color.full = EGUI_COLOR_RGB565_TRANS(src_pixels[i]);
                    egui_image_std_blend_resize_pixel(&dst_row[i], color, alpha);
                }
                i++;
            }
        }
        return;
    }

    while (i < count)
    {
        while (i + 4 <= count && *(const uint32_t *)&src_alpha_row[i] == 0x00000000)
        {
            i += 4;
        }
        while (i < count && src_alpha_row[i] == 0)
        {
            i++;
        }

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

    if (canvas_alpha == 0)
    {
        return;
    }

    if (canvas_alpha == EGUI_ALPHA_100)
    {
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
                egui_image_std_blend_rgb565_mapped_row(&dst_row[opaque_start], src_row, &src_x_map[opaque_start], i - opaque_start, EGUI_ALPHA_100);
                continue;
            }

            if (i < count)
            {
                egui_dim_t src_x = src_x_map[i];
                egui_color_t color;
                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                egui_image_std_blend_resize_pixel(&dst_row[i], color, src_alpha_row[src_x]);
                i++;
            }
        }
        return;
    }

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

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha8_masked_mapped_segment(egui_canvas_t *canvas, egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                      const uint8_t *src_alpha_row, const egui_dim_t *src_x_map, egui_dim_t count,
                                                                                      egui_dim_t screen_x, egui_dim_t screen_y, egui_alpha_t canvas_alpha)
{
    egui_mask_t *mask = canvas->mask;

    if (mask == NULL || count == 0 || canvas_alpha == 0)
    {
        return;
    }

    if (mask->api->kind == EGUI_MASK_KIND_CIRCLE)
    {
        egui_mask_circle_t *circle_mask = (egui_mask_circle_t *)mask;
        egui_dim_t row_index;
        egui_dim_t center_x = circle_mask->center_x;
        egui_dim_t radius = circle_mask->radius;
        egui_dim_t screen_x_end = screen_x + count;
        const egui_circle_info_t *info = circle_mask->info;
        const egui_circle_item_t *items;

        if (screen_y == circle_mask->point_cached_y)
        {
            if (!circle_mask->point_cached_row_valid || info == NULL)
            {
                return;
            }
            row_index = circle_mask->point_cached_row_index;
        }
        else
        {
            egui_dim_t dy = (screen_y > circle_mask->center_y) ? (screen_y - circle_mask->center_y) : (circle_mask->center_y - screen_y);
            if (dy > radius || info == NULL)
            {
                circle_mask->point_cached_y = screen_y;
                circle_mask->point_cached_row_valid = 0;
                return;
            }
            row_index = radius - dy;
            circle_mask->point_cached_y = screen_y;
            circle_mask->point_cached_row_index = row_index;
            circle_mask->point_cached_row_valid = 1;
        }

        items = (const egui_circle_item_t *)info->items;

        if (canvas_alpha == EGUI_ALPHA_100)
        {
            if (screen_x_end <= center_x)
            {
                egui_dim_t seg_start = EGUI_MAX(screen_x, center_x - radius);
                egui_dim_t seg_end = EGUI_MIN(screen_x_end, center_x);
                egui_dim_t corner_col = seg_start - (center_x - radius);
                egui_dim_t i = seg_start - screen_x;
                egui_dim_t end_index = seg_end - screen_x;
                egui_dim_t item_count = (egui_dim_t)info->item_count;

                if (corner_col < row_index)
                {
                    egui_dim_t mirror_end = EGUI_MIN(end_index, i + (row_index - corner_col));
                    for (; i < mirror_end; i++, corner_col++)
                    {
                        egui_dim_t src_x = src_x_map[i];
                        egui_alpha_t alpha = src_alpha_row[src_x];

                        if (alpha == 0)
                        {
                            continue;
                        }

                        {
                            const egui_circle_item_t *item = &items[corner_col];
                            egui_dim_t start_offset = (egui_dim_t)item->start_offset;

                            if (row_index < start_offset)
                            {
                                continue;
                            }

                            alpha = egui_color_alpha_mix(info->data[item->data_offset + row_index - start_offset], alpha);
                        }

                        if (alpha == 0)
                        {
                            continue;
                        }

                        {
                            egui_color_t color;
                            color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                            egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
                        }
                    }
                }

                if (i < end_index && row_index < item_count)
                {
                    const egui_circle_item_t *row_item = &items[row_index];
                    egui_dim_t row_start = (egui_dim_t)row_item->start_offset;
                    const uint8_t *row_data = &info->data[row_item->data_offset];

                    if (corner_col < row_start)
                    {
                        egui_dim_t skip = EGUI_MIN(end_index - i, row_start - corner_col);
                        i += skip;
                        corner_col += skip;
                    }

                    for (; i < end_index; i++, corner_col++)
                    {
                        egui_dim_t src_x = src_x_map[i];
                        egui_alpha_t alpha = src_alpha_row[src_x];

                        if (alpha == 0)
                        {
                            continue;
                        }

                        alpha = egui_color_alpha_mix(row_data[corner_col - row_start], alpha);
                        if (alpha == 0)
                        {
                            continue;
                        }

                        {
                            egui_color_t color;
                            color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                            egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
                        }
                    }
                }
                return;
            }

            if (screen_x > center_x)
            {
                egui_dim_t seg_start = EGUI_MAX(screen_x, center_x + 1);
                egui_dim_t seg_end = EGUI_MIN(screen_x_end, center_x + radius + 1);
                egui_dim_t corner_col = (center_x + radius) - seg_start;
                egui_dim_t i = seg_start - screen_x;
                egui_dim_t end_index = seg_end - screen_x;
                egui_dim_t item_count = (egui_dim_t)info->item_count;

                if (corner_col > row_index && row_index < item_count)
                {
                    const egui_circle_item_t *row_item = &items[row_index];
                    egui_dim_t row_start = (egui_dim_t)row_item->start_offset;
                    const uint8_t *row_data = &info->data[row_item->data_offset];
                    egui_dim_t row_phase_end = EGUI_MIN(end_index, i + (corner_col - row_index));

                    for (; i < row_phase_end; i++, corner_col--)
                    {
                        egui_dim_t src_x = src_x_map[i];
                        egui_alpha_t alpha = src_alpha_row[src_x];

                        if (alpha == 0 || corner_col < row_start)
                        {
                            continue;
                        }

                        alpha = egui_color_alpha_mix(row_data[corner_col - row_start], alpha);
                        if (alpha == 0)
                        {
                            continue;
                        }

                        {
                            egui_color_t color;
                            color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                            egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
                        }
                    }
                }

                for (; i < end_index; i++, corner_col--)
                {
                    egui_dim_t src_x = src_x_map[i];
                    egui_alpha_t alpha = src_alpha_row[src_x];

                    if (alpha == 0)
                    {
                        continue;
                    }

                    {
                        const egui_circle_item_t *item = &items[corner_col];
                        egui_dim_t start_offset = (egui_dim_t)item->start_offset;

                        if (row_index < start_offset)
                        {
                            continue;
                        }

                        alpha = egui_color_alpha_mix(info->data[item->data_offset + row_index - start_offset], alpha);
                    }

                    if (alpha == 0)
                    {
                        continue;
                    }

                    {
                        egui_color_t color;
                        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                        egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
                    }
                }
                return;
            }

            for (egui_dim_t i = 0; i < count; i++)
            {
                egui_dim_t dx;
                egui_dim_t src_x = src_x_map[i];
                egui_alpha_t alpha = src_alpha_row[src_x];

                if (alpha == 0)
                {
                    continue;
                }

                dx = (screen_x + i > center_x) ? ((screen_x + i) - center_x) : (center_x - (screen_x + i));
                if (dx > radius)
                {
                    continue;
                }

                alpha = egui_color_alpha_mix(egui_canvas_get_circle_corner_value_fixed_row(row_index, radius - dx, info, items), alpha);
                if (alpha == 0)
                {
                    continue;
                }

                egui_color_t color;
                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                if (alpha == EGUI_ALPHA_100)
                {
                    dst_row[i] = color.full;
                }
                else
                {
                    egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
                }
            }
        }
        else
        {
            if (screen_x_end <= center_x)
            {
                egui_dim_t seg_start = EGUI_MAX(screen_x, center_x - radius);
                egui_dim_t seg_end = EGUI_MIN(screen_x_end, center_x);
                egui_dim_t corner_col = seg_start - (center_x - radius);
                egui_dim_t i = seg_start - screen_x;
                egui_dim_t end_index = seg_end - screen_x;
                egui_dim_t item_count = (egui_dim_t)info->item_count;

                if (corner_col < row_index)
                {
                    egui_dim_t mirror_end = EGUI_MIN(end_index, i + (row_index - corner_col));
                    for (; i < mirror_end; i++, corner_col++)
                    {
                        egui_dim_t src_x = src_x_map[i];
                        egui_alpha_t alpha = src_alpha_row[src_x];

                        if (alpha == 0)
                        {
                            continue;
                        }

                        {
                            const egui_circle_item_t *item = &items[corner_col];
                            egui_dim_t start_offset = (egui_dim_t)item->start_offset;

                            if (row_index < start_offset)
                            {
                                continue;
                            }

                            alpha = egui_color_alpha_mix(info->data[item->data_offset + row_index - start_offset], alpha);
                        }

                        alpha = egui_color_alpha_mix(canvas_alpha, alpha);
                        if (alpha == 0)
                        {
                            continue;
                        }

                        {
                            egui_color_t color;
                            color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                            egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
                        }
                    }
                }

                if (i < end_index && row_index < item_count)
                {
                    const egui_circle_item_t *row_item = &items[row_index];
                    egui_dim_t row_start = (egui_dim_t)row_item->start_offset;
                    const uint8_t *row_data = &info->data[row_item->data_offset];

                    if (corner_col < row_start)
                    {
                        egui_dim_t skip = EGUI_MIN(end_index - i, row_start - corner_col);
                        i += skip;
                        corner_col += skip;
                    }

                    for (; i < end_index; i++, corner_col++)
                    {
                        egui_dim_t src_x = src_x_map[i];
                        egui_alpha_t alpha = src_alpha_row[src_x];

                        if (alpha == 0)
                        {
                            continue;
                        }

                        alpha = egui_color_alpha_mix(row_data[corner_col - row_start], alpha);
                        alpha = egui_color_alpha_mix(canvas_alpha, alpha);
                        if (alpha == 0)
                        {
                            continue;
                        }

                        {
                            egui_color_t color;
                            color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                            egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
                        }
                    }
                }
                return;
            }

            if (screen_x > center_x)
            {
                egui_dim_t seg_start = EGUI_MAX(screen_x, center_x + 1);
                egui_dim_t seg_end = EGUI_MIN(screen_x_end, center_x + radius + 1);
                egui_dim_t corner_col = (center_x + radius) - seg_start;
                egui_dim_t i = seg_start - screen_x;
                egui_dim_t end_index = seg_end - screen_x;
                egui_dim_t item_count = (egui_dim_t)info->item_count;

                if (corner_col > row_index && row_index < item_count)
                {
                    const egui_circle_item_t *row_item = &items[row_index];
                    egui_dim_t row_start = (egui_dim_t)row_item->start_offset;
                    const uint8_t *row_data = &info->data[row_item->data_offset];
                    egui_dim_t row_phase_end = EGUI_MIN(end_index, i + (corner_col - row_index));

                    for (; i < row_phase_end; i++, corner_col--)
                    {
                        egui_dim_t src_x = src_x_map[i];
                        egui_alpha_t alpha = src_alpha_row[src_x];

                        if (alpha == 0 || corner_col < row_start)
                        {
                            continue;
                        }

                        alpha = egui_color_alpha_mix(row_data[corner_col - row_start], alpha);
                        alpha = egui_color_alpha_mix(canvas_alpha, alpha);
                        if (alpha == 0)
                        {
                            continue;
                        }

                        {
                            egui_color_t color;
                            color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                            egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
                        }
                    }
                }

                for (; i < end_index; i++, corner_col--)
                {
                    egui_dim_t src_x = src_x_map[i];
                    egui_alpha_t alpha = src_alpha_row[src_x];

                    if (alpha == 0)
                    {
                        continue;
                    }

                    {
                        const egui_circle_item_t *item = &items[corner_col];
                        egui_dim_t start_offset = (egui_dim_t)item->start_offset;

                        if (row_index < start_offset)
                        {
                            continue;
                        }

                        alpha = egui_color_alpha_mix(info->data[item->data_offset + row_index - start_offset], alpha);
                    }

                    alpha = egui_color_alpha_mix(canvas_alpha, alpha);
                    if (alpha == 0)
                    {
                        continue;
                    }

                    {
                        egui_color_t color;
                        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                        egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
                    }
                }
                return;
            }

            for (egui_dim_t i = 0; i < count; i++)
            {
                egui_dim_t dx;
                egui_dim_t src_x = src_x_map[i];
                egui_alpha_t alpha = src_alpha_row[src_x];

                if (alpha == 0)
                {
                    continue;
                }

                dx = (screen_x + i > center_x) ? ((screen_x + i) - center_x) : (center_x - (screen_x + i));
                if (dx > radius)
                {
                    continue;
                }

                alpha = egui_color_alpha_mix(egui_canvas_get_circle_corner_value_fixed_row(row_index, radius - dx, info, items), alpha);
                alpha = egui_color_alpha_mix(canvas_alpha, alpha);
                if (alpha == 0)
                {
                    continue;
                }

                egui_color_t color;
                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                if (alpha == EGUI_ALPHA_100)
                {
                    dst_row[i] = color.full;
                }
                else
                {
                    egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
                }
            }
        }
        return;
    }

    if (mask->api->kind == EGUI_MASK_KIND_ROUND_RECTANGLE)
    {
        if (egui_mask_round_rectangle_blend_rgb565_alpha8_segment(mask, dst_row, src_row, src_alpha_row, src_x_map, count, screen_x, screen_y, canvas_alpha))
        {
            return;
        }
    }

    if (mask->api->kind == EGUI_MASK_KIND_IMAGE)
    {
        if (egui_mask_image_blend_rgb565_alpha8_segment(mask, dst_row, src_row, src_alpha_row, src_x_map, count, screen_x, screen_y, canvas_alpha))
        {
            return;
        }
    }

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        for (egui_dim_t i = 0; i < count; i++)
        {
            egui_dim_t src_x = src_x_map[i];
            egui_color_t color;
            egui_alpha_t alpha = src_alpha_row[src_x];

            color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
            mask->api->mask_point(mask, screen_x + i, screen_y, &color, &alpha);

            if (alpha == 0)
            {
                continue;
            }

            if (alpha == EGUI_ALPHA_100)
            {
                dst_row[i] = color.full;
            }
            else
            {
                egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
            }
        }
        return;
    }

    for (egui_dim_t i = 0; i < count; i++)
    {
        egui_dim_t src_x = src_x_map[i];
        egui_color_t color;
        egui_alpha_t alpha = src_alpha_row[src_x];

        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
        mask->api->mask_point(mask, screen_x + i, screen_y, &color, &alpha);
        alpha = egui_color_alpha_mix(canvas_alpha, alpha);

        if (alpha == 0)
        {
            continue;
        }

        if (alpha == EGUI_ALPHA_100)
        {
            dst_row[i] = color.full;
        }
        else
        {
            egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_circle_masked_left_segment_fixed_row(egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                              const egui_dim_t *src_x_map, egui_dim_t count,
                                                                                              egui_dim_t screen_x, egui_dim_t center_x,
                                                                                              egui_dim_t radius, egui_dim_t row_index,
                                                                                              egui_alpha_t canvas_alpha,
                                                                                              const egui_circle_info_t *info,
                                                                                              const egui_circle_item_t *items)
{
    egui_dim_t corner_col = screen_x - (center_x - radius);

    for (egui_dim_t i = 0; i < count; i++, corner_col++)
    {
        egui_alpha_t alpha;
        egui_dim_t src_x;
        egui_color_t color;

        if (corner_col < 0)
        {
            continue;
        }

        if (corner_col > radius)
        {
            break;
        }

        alpha = egui_canvas_get_circle_corner_value_fixed_row(row_index, corner_col, info, items);
        if (alpha == 0)
        {
            continue;
        }

        if (canvas_alpha != EGUI_ALPHA_100)
        {
            alpha = egui_color_alpha_mix(canvas_alpha, alpha);
            if (alpha == 0)
            {
                continue;
            }
        }

        src_x = src_x_map[i];
        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
        if (alpha == EGUI_ALPHA_100)
        {
            dst_row[i] = color.full;
        }
        else
        {
            egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_circle_masked_right_segment_fixed_row(egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                               const egui_dim_t *src_x_map, egui_dim_t count,
                                                                                               egui_dim_t screen_x, egui_dim_t center_x,
                                                                                               egui_dim_t radius, egui_dim_t row_index,
                                                                                               egui_alpha_t canvas_alpha,
                                                                                               const egui_circle_info_t *info,
                                                                                               const egui_circle_item_t *items)
{
    egui_dim_t corner_col = (center_x + radius) - screen_x;

    for (egui_dim_t i = 0; i < count; i++, corner_col--)
    {
        egui_alpha_t alpha;
        egui_dim_t src_x;
        egui_color_t color;

        if (corner_col < 0)
        {
            break;
        }

        if (corner_col > radius)
        {
            continue;
        }

        alpha = egui_canvas_get_circle_corner_value_fixed_row(row_index, corner_col, info, items);
        if (alpha == 0)
        {
            continue;
        }

        if (canvas_alpha != EGUI_ALPHA_100)
        {
            alpha = egui_color_alpha_mix(canvas_alpha, alpha);
            if (alpha == 0)
            {
                continue;
            }
        }

        src_x = src_x_map[i];
        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
        if (alpha == EGUI_ALPHA_100)
        {
            dst_row[i] = color.full;
        }
        else
        {
            egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_circle_masked_mapped_segment_fixed_row(egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                                 const egui_dim_t *src_x_map, egui_dim_t count,
                                                                                                 egui_dim_t screen_x, egui_dim_t center_x,
                                                                                                 egui_dim_t radius, egui_dim_t row_index,
                                                                                                 egui_alpha_t canvas_alpha,
                                                                                                 const egui_circle_info_t *info,
                                                                                                 const egui_circle_item_t *items)
{
    egui_dim_t processed = 0;

    if (count == 0)
    {
        return;
    }

    if (screen_x < center_x)
    {
        egui_dim_t left_count = EGUI_MIN(count, center_x - screen_x);
        if (left_count > 0)
        {
            egui_image_std_blend_rgb565_circle_masked_left_segment_fixed_row(dst_row, src_row, src_x_map, left_count, screen_x, center_x, radius, row_index,
                                                                             canvas_alpha, info,
                                                                             items);
            processed = left_count;
        }
    }

    if (processed < count && screen_x + processed == center_x)
    {
        egui_alpha_t alpha = egui_canvas_get_circle_corner_value_fixed_row(row_index, radius, info, items);

        if (alpha != 0)
        {
            egui_dim_t src_x = src_x_map[processed];
            egui_color_t color;
            color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
            if (canvas_alpha != EGUI_ALPHA_100)
            {
                alpha = egui_color_alpha_mix(canvas_alpha, alpha);
            }
            if (alpha == EGUI_ALPHA_100)
            {
                dst_row[processed] = color.full;
            }
            else if (alpha != 0)
            {
                egui_rgb_mix_ptr((egui_color_t *)&dst_row[processed], &color, (egui_color_t *)&dst_row[processed], alpha);
            }
        }
        processed++;
    }

    if (processed < count)
    {
        egui_image_std_blend_rgb565_circle_masked_right_segment_fixed_row(&dst_row[processed], src_row, &src_x_map[processed], count - processed,
                                                                          screen_x + processed, center_x, radius, row_index, canvas_alpha, info, items);
    }
}

__EGUI_STATIC_INLINE__ uint32_t egui_image_std_circle_isqrt(uint32_t n)
{
    uint32_t root = 0;
    uint32_t bit = 1UL << 30;

    while (bit > n)
    {
        bit >>= 2;
    }

    while (bit != 0)
    {
        if (n >= root + bit)
        {
            n -= root + bit;
            root = (root >> 1) + bit;
        }
        else
        {
            root >>= 1;
        }
        bit >>= 2;
    }

    return root;
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_image_std_get_circle_visible_half_cached(egui_dim_t dy, uint32_t visible_radius_sq, egui_dim_t *cached_dy,
                                                                                egui_dim_t *cached_half)
{
    if (dy == *cached_dy)
    {
        return *cached_half;
    }

    {
        uint32_t dy_sq = (uint32_t)dy * (uint32_t)dy;
        egui_dim_t half = *cached_half;

        if (*cached_dy >= 0)
        {
            egui_dim_t delta = dy - *cached_dy;
            if (delta < 0)
            {
                delta = -delta;
            }

            if (delta <= 4)
            {
                while (half > 0 && ((uint32_t)half * (uint32_t)half + dy_sq) > visible_radius_sq)
                {
                    half--;
                }

                while (((uint32_t)(half + 1) * (uint32_t)(half + 1) + dy_sq) <= visible_radius_sq)
                {
                    half++;
                }

                *cached_dy = dy;
                *cached_half = half;
                return half;
            }
        }

        half = (egui_dim_t)egui_image_std_circle_isqrt((dy_sq < visible_radius_sq) ? (visible_radius_sq - dy_sq) : 0);
        *cached_dy = dy;
        *cached_half = half;
        return half;
    }
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_image_std_get_circle_opaque_boundary_fixed_row(egui_dim_t row_index, const egui_circle_info_t *info,
                                                                                       const egui_circle_item_t *items)
{
    egui_dim_t item_count = (egui_dim_t)info->item_count;
    egui_dim_t boundary;
    egui_dim_t mirror_limit = EGUI_MIN(row_index, item_count);

    if (row_index >= item_count)
    {
        return 0;
    }

    {
        const egui_circle_item_t *item = &items[row_index];
        boundary = (egui_dim_t)item->start_offset + (egui_dim_t)item->valid_count;
    }

    if (mirror_limit > 0)
    {
        egui_dim_t low = 0;
        egui_dim_t high = mirror_limit;

        while (low < high)
        {
            egui_dim_t mid = low + ((high - low) >> 1);
            const egui_circle_item_t *item = &items[mid];
            egui_dim_t threshold = (egui_dim_t)item->start_offset + (egui_dim_t)item->valid_count;

            if (row_index >= threshold)
            {
                high = mid;
            }
            else
            {
                low = mid + 1;
            }
        }

        if (low < mirror_limit)
        {
            boundary = EGUI_MIN(boundary, low);
        }
    }

    return boundary;
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_image_std_get_circle_opaque_boundary_cached(egui_dim_t row_index, egui_dim_t radius, const egui_circle_info_t *info,
                                                                                    const egui_circle_item_t *items, egui_dim_t *cached_row_index,
                                                                                    egui_dim_t *cached_boundary)
{
    egui_dim_t boundary;

    if (row_index == *cached_row_index)
    {
        return *cached_boundary;
    }

    if (*cached_row_index >= 0)
    {
        egui_dim_t delta = row_index - *cached_row_index;
        if (delta < 0)
        {
            delta = -delta;
        }

        if (delta <= 4)
        {
            boundary = *cached_boundary;
            if (boundary < 0)
            {
                boundary = 0;
            }
            else if (boundary > radius)
            {
                boundary = radius;
            }

            while (boundary > 0 && egui_canvas_get_circle_corner_value_fixed_row(row_index, boundary - 1, info, items) == EGUI_ALPHA_100)
            {
                boundary--;
            }

            while (boundary < radius && egui_canvas_get_circle_corner_value_fixed_row(row_index, boundary, info, items) != EGUI_ALPHA_100)
            {
                boundary++;
            }

            *cached_row_index = row_index;
            *cached_boundary = boundary;
            return boundary;
        }
    }

    boundary = egui_image_std_get_circle_opaque_boundary_fixed_row(row_index, info, items);
    *cached_row_index = row_index;
    *cached_boundary = boundary;
    return boundary;
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_image_std_get_circle_visible_boundary_fixed_row(egui_dim_t row_index, const egui_circle_info_t *info,
                                                                                       const egui_circle_item_t *items)
{
    egui_dim_t item_count = (egui_dim_t)info->item_count;
    egui_dim_t boundary;
    egui_dim_t mirror_limit = EGUI_MIN(row_index, item_count);

    if (row_index >= item_count)
    {
        return 0;
    }

    boundary = (egui_dim_t)items[row_index].start_offset;

    if (mirror_limit > 0)
    {
        egui_dim_t low = 0;
        egui_dim_t high = mirror_limit;

        while (low < high)
        {
            egui_dim_t mid = low + ((high - low) >> 1);
            if (row_index >= (egui_dim_t)items[mid].start_offset)
            {
                high = mid;
            }
            else
            {
                low = mid + 1;
            }
        }

        if (low < mirror_limit)
        {
            boundary = EGUI_MIN(boundary, low);
        }
    }

    return boundary;
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_image_std_get_circle_visible_boundary_cached(egui_dim_t row_index, egui_dim_t radius, const egui_circle_info_t *info,
                                                                                     const egui_circle_item_t *items, egui_dim_t *cached_row_index,
                                                                                     egui_dim_t *cached_boundary)
{
    egui_dim_t boundary;

    if (row_index == *cached_row_index)
    {
        return *cached_boundary;
    }

    if (*cached_row_index >= 0)
    {
        egui_dim_t delta = row_index - *cached_row_index;
        if (delta < 0)
        {
            delta = -delta;
        }

        if (delta <= 4)
        {
            boundary = *cached_boundary;
            if (boundary < 0)
            {
                boundary = 0;
            }
            else if (boundary > radius)
            {
                boundary = radius;
            }

            while (boundary > 0 && egui_canvas_get_circle_corner_value_fixed_row(row_index, boundary - 1, info, items) != 0)
            {
                boundary--;
            }

            while (boundary < radius && egui_canvas_get_circle_corner_value_fixed_row(row_index, boundary, info, items) == 0)
            {
                boundary++;
            }

            *cached_row_index = row_index;
            *cached_boundary = boundary;
            return boundary;
        }
    }

    boundary = egui_image_std_get_circle_visible_boundary_fixed_row(row_index, info, items);
    *cached_row_index = row_index;
    *cached_boundary = boundary;
    return boundary;
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_round_rect_masked_left_segment_fixed_row(egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                                  const egui_dim_t *src_x_map, egui_dim_t count,
                                                                                                  egui_dim_t screen_x, egui_dim_t mask_x,
                                                                                                  egui_dim_t row_index, const egui_circle_info_t *info,
                                                                                                  const egui_circle_item_t *items)
{
    egui_dim_t corner_col = screen_x - mask_x;
    egui_dim_t i = 0;
    egui_dim_t end_index = count;
    egui_dim_t item_count = (egui_dim_t)info->item_count;

    if (row_index >= item_count)
    {
        egui_dim_t skip = 0;

        if (screen_x < mask_x)
        {
            skip = EGUI_MIN(count, mask_x - screen_x);
        }

        if (skip < count)
        {
            egui_image_std_blend_rgb565_mapped_row(&dst_row[skip], src_row, &src_x_map[skip], count - skip, EGUI_ALPHA_100);
        }
        return;
    }

    if (corner_col < row_index)
    {
        egui_dim_t mirror_end = EGUI_MIN(end_index, row_index - corner_col);
        for (; i < mirror_end; i++, corner_col++)
        {
            egui_alpha_t alpha;

            {
                const egui_circle_item_t *item = &items[corner_col];
                egui_dim_t start_offset = (egui_dim_t)item->start_offset;
                egui_dim_t valid_count = (egui_dim_t)item->valid_count;
                egui_dim_t opaque_start = start_offset + valid_count;

                if (row_index < start_offset)
                {
                    continue;
                }

                if (row_index >= opaque_start)
                {
                    alpha = EGUI_ALPHA_100;
                }
                else
                {
                    alpha = info->data[item->data_offset + row_index - start_offset];
                }
            }

            if (alpha == 0)
            {
                continue;
            }

            {
                egui_dim_t src_x = src_x_map[i];
                egui_color_t color;

                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                if (alpha == EGUI_ALPHA_100)
                {
                    dst_row[i] = color.full;
                }
                else
                {
                    egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
                }
            }
        }
    }

    if (i < end_index && row_index < item_count)
    {
        const egui_circle_item_t *row_item = &items[row_index];
        egui_dim_t row_start = (egui_dim_t)row_item->start_offset;
        egui_dim_t row_valid_count = (egui_dim_t)row_item->valid_count;
        egui_dim_t row_opaque_start = row_start + row_valid_count;
        const uint8_t *row_data = &info->data[row_item->data_offset];

        if (corner_col < row_start)
        {
            egui_dim_t skip = EGUI_MIN(end_index - i, row_start - corner_col);
            i += skip;
            corner_col += skip;
        }

        for (; i < end_index; i++, corner_col++)
        {
            egui_alpha_t alpha;

            if (corner_col >= row_opaque_start)
            {
                alpha = EGUI_ALPHA_100;
            }
            else
            {
                alpha = row_data[corner_col - row_start];
            }

            if (alpha == 0)
            {
                continue;
            }

            {
                egui_dim_t src_x = src_x_map[i];
                egui_color_t color;

                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                if (alpha == EGUI_ALPHA_100)
                {
                    dst_row[i] = color.full;
                }
                else
                {
                    egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
                }
            }
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_round_rect_masked_right_segment_fixed_row(egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                                   const egui_dim_t *src_x_map, egui_dim_t count,
                                                                                                   egui_dim_t screen_x, egui_dim_t mask_x_end,
                                                                                                   egui_dim_t row_index, const egui_circle_info_t *info,
                                                                                                   const egui_circle_item_t *items)
{
    egui_dim_t corner_col = mask_x_end - 1 - screen_x;
    egui_dim_t i = 0;
    egui_dim_t end_index = count;
    egui_dim_t item_count = (egui_dim_t)info->item_count;

    if (row_index >= item_count)
    {
        egui_dim_t right_bound = mask_x_end - 1;

        if (screen_x <= right_bound)
        {
            egui_dim_t valid_count = EGUI_MIN(count, right_bound - screen_x + 1);
            egui_image_std_blend_rgb565_mapped_row(dst_row, src_row, src_x_map, valid_count, EGUI_ALPHA_100);
        }
        return;
    }

    if (corner_col > row_index && row_index < item_count)
    {
        const egui_circle_item_t *row_item = &items[row_index];
        egui_dim_t row_start = (egui_dim_t)row_item->start_offset;
        egui_dim_t row_valid_count = (egui_dim_t)row_item->valid_count;
        egui_dim_t row_opaque_start = row_start + row_valid_count;
        const uint8_t *row_data = &info->data[row_item->data_offset];
        egui_dim_t row_phase_end = EGUI_MIN(end_index, corner_col - row_index);

        for (; i < row_phase_end; i++, corner_col--)
        {
            if (corner_col < row_start)
            {
                continue;
            }

            {
                egui_alpha_t alpha;

                if (corner_col >= row_opaque_start)
                {
                    alpha = EGUI_ALPHA_100;
                }
                else
                {
                    alpha = row_data[corner_col - row_start];
                }

                if (alpha == 0)
                {
                    continue;
                }

                {
                    egui_dim_t src_x = src_x_map[i];
                    egui_color_t color;

                    color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                    if (alpha == EGUI_ALPHA_100)
                    {
                        dst_row[i] = color.full;
                    }
                    else
                    {
                        egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
                    }
                }
            }
        }
    }

    for (; i < end_index; i++, corner_col--)
    {
        egui_alpha_t alpha;

        {
            const egui_circle_item_t *item = &items[corner_col];
            egui_dim_t start_offset = (egui_dim_t)item->start_offset;
            egui_dim_t valid_count = (egui_dim_t)item->valid_count;
            egui_dim_t opaque_start = start_offset + valid_count;

            if (row_index < start_offset)
            {
                continue;
            }

            if (row_index >= opaque_start)
            {
                alpha = EGUI_ALPHA_100;
            }
            else
            {
                alpha = info->data[item->data_offset + row_index - start_offset];
            }
        }

        if (alpha == 0)
        {
            continue;
        }

        {
            egui_dim_t src_x = src_x_map[i];
            egui_color_t color;

            color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
            if (alpha == EGUI_ALPHA_100)
            {
                dst_row[i] = color.full;
            }
            else
            {
                egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
            }
        }
    }
}

typedef struct
{
    const egui_mask_t *mask;
    egui_dim_t cached_x;
    egui_dim_t cached_y;
    egui_dim_t cached_width;
    egui_dim_t cached_height;
    egui_dim_t cached_radius;
    egui_dim_t row_cache_y[EGUI_CONFIG_PFB_HEIGHT];
    egui_dim_t row_cache_visible_boundary[EGUI_CONFIG_PFB_HEIGHT];
    egui_dim_t row_cache_opaque_boundary[EGUI_CONFIG_PFB_HEIGHT];
    egui_dim_t visible_cached_row_index;
    egui_dim_t visible_cached_boundary;
    egui_dim_t opaque_cached_row_index;
    egui_dim_t opaque_cached_boundary;
    const egui_circle_info_t *info;
} egui_image_std_round_rect_fast_cache_t;

static egui_image_std_round_rect_fast_cache_t g_egui_image_std_round_rect_fast_cache[2];
static uint8_t g_egui_image_std_round_rect_fast_cache_next = 0;

__EGUI_STATIC_INLINE__ void egui_image_std_round_rect_fast_cache_invalidate(egui_image_std_round_rect_fast_cache_t *cache)
{
    for (egui_dim_t i = 0; i < EGUI_CONFIG_PFB_HEIGHT; i++)
    {
        cache->row_cache_y[i] = -32768;
    }
}

__EGUI_STATIC_INLINE__ egui_image_std_round_rect_fast_cache_t *egui_image_std_round_rect_fast_cache_get(const egui_mask_t *mask)
{
    for (uint8_t i = 0; i < (uint8_t)(sizeof(g_egui_image_std_round_rect_fast_cache) / sizeof(g_egui_image_std_round_rect_fast_cache[0])); i++)
    {
        if (g_egui_image_std_round_rect_fast_cache[i].mask == mask)
        {
            return &g_egui_image_std_round_rect_fast_cache[i];
        }
    }

    {
        egui_image_std_round_rect_fast_cache_t *cache = &g_egui_image_std_round_rect_fast_cache[g_egui_image_std_round_rect_fast_cache_next];

        g_egui_image_std_round_rect_fast_cache_next++;
        g_egui_image_std_round_rect_fast_cache_next &= 0x01;
        cache->mask = mask;
        cache->cached_x = -1;
        cache->cached_y = -1;
        cache->cached_width = -1;
        cache->cached_height = -1;
        cache->cached_radius = -1;
        cache->visible_cached_row_index = -1;
        cache->visible_cached_boundary = 0;
        cache->opaque_cached_row_index = -1;
        cache->opaque_cached_boundary = 0;
        cache->info = NULL;
        egui_image_std_round_rect_fast_cache_invalidate(cache);
        return cache;
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_round_rect_fast_cache_refresh(egui_image_std_round_rect_fast_cache_t *cache, const egui_mask_t *mask,
                                                                         egui_dim_t radius)
{
    if (cache->cached_x == mask->region.location.x && cache->cached_y == mask->region.location.y && cache->cached_width == mask->region.size.width &&
        cache->cached_height == mask->region.size.height && cache->cached_radius == radius)
    {
        return;
    }

    cache->cached_x = mask->region.location.x;
    cache->cached_y = mask->region.location.y;
    cache->cached_width = mask->region.size.width;
    cache->cached_height = mask->region.size.height;
    cache->cached_radius = radius;
    cache->visible_cached_row_index = -1;
    cache->visible_cached_boundary = 0;
    cache->opaque_cached_row_index = -1;
    cache->opaque_cached_boundary = 0;
    cache->info = egui_canvas_get_circle_item(radius);
    egui_image_std_round_rect_fast_cache_invalidate(cache);
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

EGUI_IMAGE_STD_BLEND_RGB565_PACKED_ALPHA_REPEAT2_ROW_DEFINE(egui_image_std_blend_rgb565_alpha4_repeat2_row, egui_image_std_get_alpha_rgb565_4_row)
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

EGUI_IMAGE_STD_BLEND_RGB565_PACKED_ALPHA_REPEAT2_ROW_DEFINE(egui_image_std_blend_rgb565_alpha2_repeat2_row, egui_image_std_get_alpha_rgb565_2_row)
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

EGUI_IMAGE_STD_BLEND_RGB565_PACKED_ALPHA_REPEAT2_ROW_DEFINE(egui_image_std_blend_rgb565_alpha1_repeat2_row, egui_image_std_get_alpha_rgb565_1_row)
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
            if (image->alpha_buf == NULL
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                || (image->res_type == EGUI_RESOURCE_TYPE_INTERNAL && egui_image_std_rgb565_is_opaque_source(image))
#else
                || egui_image_std_rgb565_is_opaque_source(image)
#endif
            )
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
        else if (canvas->mask->api->mask_get_row_overlay != NULL)                                                                                              \
        {                                                                                                                                                      \
            for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                        \
            {                                                                                                                                                  \
                egui_dim_t ov_sy = y_base + y_;                                                                                                                \
                egui_color_t ov_color;                                                                                                                         \
                egui_alpha_t ov_alpha;                                                                                                                         \
                if (canvas->mask->api->mask_get_row_overlay(canvas->mask, ov_sy, &ov_color, &ov_alpha))                                                        \
                {                                                                                                                                              \
                    for (egui_dim_t x_ = x; x_ < x_total; x_++)                                                                                                \
                    {                                                                                                                                          \
                        _get_pixel_func(image, x_, y_, &color, &alpha);                                                                                        \
                        if (ov_alpha > 0)                                                                                                                      \
                        {                                                                                                                                      \
                            egui_rgb_mix_ptr(&color, &ov_color, &color, ov_alpha);                                                                             \
                        }                                                                                                                                      \
                        egui_canvas_draw_point_limit_skip_mask((x_base + x_), ov_sy, color, alpha);                                                            \
                    }                                                                                                                                          \
                }                                                                                                                                              \
                else                                                                                                                                           \
                {                                                                                                                                              \
                    for (egui_dim_t x_ = x; x_ < x_total; x_++)                                                                                                \
                    {                                                                                                                                          \
                        _get_pixel_func(image, x_, y_, &color, &alpha);                                                                                        \
                        egui_canvas_draw_point_limit((x_base + x_), ov_sy, color, alpha);                                                                      \
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
        int image_mask_full_row_fast_path = (canvas->mask != NULL && canvas->mask->api->kind == EGUI_MASK_KIND_IMAGE);
        if (!image_mask_full_row_fast_path && canvas->mask != NULL && canvas->mask->api->mask_get_row_range != NULL)
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
            if (image_mask_full_row_fast_path)
            {
                egui_alpha_t canvas_alpha = canvas->alpha;
                egui_dim_t pfb_width = canvas->pfb_region.size.width;
                egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
                egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
                const uint16_t *src_pixels = (const uint16_t *)p_data + x;
                const uint8_t *src_alpha_row = (const uint8_t *)p_alpha + x;
                egui_dim_t count = x_total - x;

                if (egui_mask_image_blend_rgb565_alpha8_row_segment(canvas->mask, dst_row, src_pixels, src_alpha_row, count, x_base + x, rr_sy, canvas_alpha))
                {
                    continue;
                }

                image_mask_full_row_fast_path = 0;
            }

            if (rr_res < 0 && canvas->mask->api->mask_get_row_range != NULL)
            {
                rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, x_base + x, x_base + x_total, &rr_x_start, &rr_x_end);
                if (rr_res == EGUI_MASK_ROW_OUTSIDE)
                {
                    continue;
                }
            }

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
                    int image_mask_fast_path = (canvas->mask->api->kind == EGUI_MASK_KIND_IMAGE);
                    egui_dim_t rr_img_xs = rr_x_start - x_base;
                    egui_dim_t rr_img_xe = rr_x_end - x_base;
                    egui_dim_t visible_x_start = x_base + x;
                    egui_dim_t visible_x_end = x_base + x_total;
                    int has_visible_range = 0;

                    if (image_mask_fast_path && canvas->mask->api->mask_get_row_visible_range != NULL)
                    {
                        has_visible_range = canvas->mask->api->mask_get_row_visible_range(canvas->mask, rr_sy, x_base + x, x_base + x_total, &visible_x_start,
                                                                                          &visible_x_end);
                        if (!has_visible_range)
                        {
                            continue;
                        }
                    }

                    if (has_visible_range)
                    {
                        egui_dim_t vis_img_xs = EGUI_MAX(x, visible_x_start - x_base);
                        egui_dim_t vis_img_xe = EGUI_MIN(x_total, visible_x_end - x_base);
                        egui_dim_t left_img_xe = EGUI_MIN(rr_img_xs, vis_img_xe);

                        if (vis_img_xs < left_img_xe)
                        {
                            egui_alpha_t canvas_alpha = canvas->alpha;
                            egui_dim_t pfb_width = canvas->pfb_region.size.width;
                            egui_dim_t dst_x = (x_base + vis_img_xs) - canvas->pfb_location_in_base_view.x;
                            egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x];
                            const uint16_t *src_pixels = (const uint16_t *)p_data + vis_img_xs;
                            const uint8_t *src_alpha_seg = (const uint8_t *)p_alpha + vis_img_xs;

                            if (!egui_mask_image_blend_rgb565_alpha8_row_segment(canvas->mask, dst_row, src_pixels, src_alpha_seg, left_img_xe - vis_img_xs,
                                                                                 x_base + vis_img_xs, rr_sy, canvas_alpha))
                            {
                                for (egui_dim_t x_ = vis_img_xs; x_ < left_img_xe; x_++)
                                {
                                    egui_image_std_get_col_pixel_rgb565_8(p_data, p_alpha, x_, &color, &alpha);
                                    egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                                }
                            }
                        }
                    }
                    else
                    {
                        for (egui_dim_t x_ = x; x_ < rr_img_xs; x_++)
                        {
                            egui_image_std_get_col_pixel_rgb565_8(p_data, p_alpha, x_, &color, &alpha);
                            egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                        }
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

                    if (has_visible_range)
                    {
                        egui_dim_t vis_img_xs = EGUI_MAX(x, visible_x_start - x_base);
                        egui_dim_t vis_img_xe = EGUI_MIN(x_total, visible_x_end - x_base);
                        egui_dim_t right_img_xs = EGUI_MAX(rr_img_xe, vis_img_xs);

                        if (right_img_xs < vis_img_xe)
                        {
                            egui_alpha_t canvas_alpha = canvas->alpha;
                            egui_dim_t pfb_width = canvas->pfb_region.size.width;
                            egui_dim_t dst_x = (x_base + right_img_xs) - canvas->pfb_location_in_base_view.x;
                            egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x];
                            const uint16_t *src_pixels = (const uint16_t *)p_data + right_img_xs;
                            const uint8_t *src_alpha_seg = (const uint8_t *)p_alpha + right_img_xs;

                            if (!egui_mask_image_blend_rgb565_alpha8_row_segment(canvas->mask, dst_row, src_pixels, src_alpha_seg, vis_img_xe - right_img_xs,
                                                                                 x_base + right_img_xs, rr_sy, canvas_alpha))
                            {
                                for (egui_dim_t x_ = right_img_xs; x_ < vis_img_xe; x_++)
                                {
                                    egui_image_std_get_col_pixel_rgb565_8(p_data, p_alpha, x_, &color, &alpha);
                                    egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                                }
                            }
                        }
                    }
                    else
                    {
                        for (egui_dim_t x_ = rr_img_xe; x_ < x_total; x_++)
                        {
                            egui_image_std_get_col_pixel_rgb565_8(p_data, p_alpha, x_, &color, &alpha);
                            egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                        }
                    }
                }
            }
            else if (canvas->mask->api->mask_get_row_overlay != NULL)
            {
                egui_color_t ov_color;
                egui_alpha_t ov_alpha;
                if (canvas->mask->api->mask_get_row_overlay(canvas->mask, rr_sy, &ov_color, &ov_alpha))
                {
                    // Row-uniform overlay: apply overlay to each pixel, skip mask_point
                    egui_alpha_t canvas_alpha = canvas->alpha;
                    egui_dim_t pfb_width = canvas->pfb_region.size.width;
                    egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
                    egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
                    const uint16_t *src_pixels = (const uint16_t *)p_data + x;
                    const uint8_t *src_alpha_row = (const uint8_t *)p_alpha + x;
                    egui_dim_t count = x_total - x;

                    if (ov_alpha == 0)
                    {
                        egui_image_std_blend_rgb565_alpha8_row(dst_row, src_pixels, src_alpha_row, count, canvas_alpha);
                    }
                    else
                    {
                        for (egui_dim_t i = 0; i < count; i++)
                        {
                            egui_alpha_t sa = src_alpha_row[i];
                            if (sa == 0)
                            {
                                continue;
                            }
                            egui_color_t c;
                            c.full = EGUI_COLOR_RGB565_TRANS(src_pixels[i]);
                            egui_rgb_mix_ptr(&c, &ov_color, &c, ov_alpha);
                            egui_alpha_t a = egui_color_alpha_mix(canvas_alpha, sa);
                            if (a != 0)
                            {
                                egui_image_std_blend_resize_pixel(&dst_row[i], c, a);
                            }
                        }
                    }
                }
                else
                {
                    for (egui_dim_t x_ = x; x_ < x_total; x_++)
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
    egui_image_std_external_alpha_row_cache_t row_cache;
    int use_external_row_cache = (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL);
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        if (!egui_image_std_init_external_alpha_row_cache(&row_cache, data_row_size, alpha_row_size))
        {
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
            EGUI_UNUSED(row_start);
            EGUI_UNUSED(row_start_alpha);
            if (!egui_image_std_load_external_alpha_row_cache(&row_cache, image, y_))
            {
                continue;
            }
            p_data = egui_image_std_get_external_alpha_row_data(&row_cache, y_);
            p_alpha = egui_image_std_get_external_alpha_row_alpha(&row_cache, y_);
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
    if (use_external_row_cache)
    {
        egui_image_std_release_external_alpha_row_cache(&row_cache);
    }
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
    egui_image_std_external_alpha_row_cache_t row_cache;
    int use_external_row_cache = (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL);
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        if (!egui_image_std_init_external_alpha_row_cache(&row_cache, data_row_size, alpha_row_size))
        {
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
            EGUI_UNUSED(row_start);
            EGUI_UNUSED(row_start_alpha);
            if (!egui_image_std_load_external_alpha_row_cache(&row_cache, image, y_))
            {
                continue;
            }
            p_data = egui_image_std_get_external_alpha_row_data(&row_cache, y_);
            p_alpha = egui_image_std_get_external_alpha_row_alpha(&row_cache, y_);
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
    if (use_external_row_cache)
    {
        egui_image_std_release_external_alpha_row_cache(&row_cache);
    }
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
    egui_image_std_external_alpha_row_cache_t row_cache;
    int use_external_row_cache = (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL);
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        if (!egui_image_std_init_external_alpha_row_cache(&row_cache, data_row_size, alpha_row_size))
        {
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
            EGUI_UNUSED(row_start);
            EGUI_UNUSED(row_start_alpha);
            if (!egui_image_std_load_external_alpha_row_cache(&row_cache, image, y_))
            {
                continue;
            }
            p_data = egui_image_std_get_external_alpha_row_data(&row_cache, y_);
            p_alpha = egui_image_std_get_external_alpha_row_alpha(&row_cache, y_);
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
    if (use_external_row_cache)
    {
        egui_image_std_release_external_alpha_row_cache(&row_cache);
    }
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

            egui_image_std_copy_rgb565_row(dst_row, src, count);

            // Advance to next row using addition (no multiply)
            dst_row += pfb_width;
        }
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        egui_image_std_release_external_buffer(data_buf, g_external_image_data_cache);
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    }
    else if ((egui_canvas_get_canvas()->alpha == EGUI_ALPHA_100) && (egui_canvas_get_canvas()->mask != NULL) &&
             (egui_canvas_get_canvas()->mask->api->mask_get_row_overlay != NULL))
    {
        // Fast path: RGB565 image with row-level overlay (e.g. linear-vertical gradient overlay).
        // Pre-compute blend factors per row and write directly to PFB.
        egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
        egui_canvas_t *canvas = egui_canvas_get_canvas();
        egui_dim_t pfb_width = canvas->pfb_region.size.width;
        egui_dim_t count = x_total - x;
        egui_dim_t img_width = image->width;

        egui_dim_t dst_y_start = (y_base + y) - canvas->pfb_location_in_base_view.y;
        egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
        egui_color_int_t *dst_row = &canvas->pfb[dst_y_start * pfb_width + dst_x_start];

        const uint16_t *src_row = NULL;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
        {
            // External resource: fall through to generic path
            EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565, self, x, y, x_total, y_total, x_base, y_base);
        }
        else
#endif
        {
            src_row = (const uint16_t *)image->data_buf + (y * img_width) + x;

            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                egui_dim_t screen_y = y_base + y_;
                egui_color_t ov_color;
                egui_alpha_t ov_alpha;

                if (canvas->mask->api->mask_get_row_overlay(canvas->mask, screen_y, &ov_color, &ov_alpha))
                {
                    if (ov_alpha > 251)
                    {
                        // Nearly fully opaque overlay: fill with overlay color
                        for (egui_dim_t i = 0; i < count; i++)
                        {
                            dst_row[i] = ov_color.full;
                        }
                    }
                    else if (ov_alpha < 4)
                    {
                        // Nearly transparent overlay: just copy source
                        egui_image_std_copy_rgb565_row(dst_row, src_row, count);
                    }
                    else
                    {
                        // Typical case: blend overlay color into each source pixel
                        uint16_t fg = ov_color.full;
                        uint32_t fg_rb_g = (fg | ((uint32_t)fg << 16)) & 0x07E0F81FUL;
                        uint32_t alpha5 = (uint32_t)ov_alpha >> 3;

                        for (egui_dim_t i = 0; i < count; i++)
                        {
                            uint16_t bg = src_row[i];
                            uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                            uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * alpha5 >> 5)) & 0x07E0F81FUL;
                            dst_row[i] = (uint16_t)(result | (result >> 16));
                        }
                    }
                }
                else
                {
                    // Overlay not applicable: per-pixel mask fallback
                    for (egui_dim_t i = 0; i < count; i++)
                    {
                        egui_color_t color;
                        egui_alpha_t alpha;
                        color.full = src_row[i];
                        alpha = EGUI_ALPHA_100;
                        canvas->mask->api->mask_point(canvas->mask, x_base + x + i, screen_y, &color, &alpha);
                        if (alpha == 0)
                            continue;
                        if (alpha == EGUI_ALPHA_100)
                        {
                            dst_row[i] = color.full;
                        }
                        else
                        {
                            egui_image_std_blend_resize_pixel(&dst_row[i], color, alpha);
                        }
                    }
                }

                src_row += img_width;
                dst_row += pfb_width;
            }
        }
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

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
typedef struct
{
    const egui_image_std_info_t *image;
    uint8_t is_all_opaque;
} egui_image_std_alpha_opaque_cache_t;

static egui_image_std_alpha_opaque_cache_t g_egui_image_std_alpha_opaque_cache[4];
static uint8_t g_egui_image_std_alpha_opaque_cache_next = 0;

__EGUI_STATIC_INLINE__ uint16_t egui_image_std_rgb565_alpha_row_size(egui_dim_t width, uint8_t alpha_type)
{
    switch (alpha_type)
    {
    case EGUI_IMAGE_ALPHA_TYPE_1:
        return (uint16_t)((width + 7) >> 3);
    case EGUI_IMAGE_ALPHA_TYPE_2:
        return (uint16_t)((width + 3) >> 2);
    case EGUI_IMAGE_ALPHA_TYPE_4:
        return (uint16_t)((width + 1) >> 1);
    case EGUI_IMAGE_ALPHA_TYPE_8:
        return (uint16_t)(width);
    default:
        return 0;
    }
}

__EGUI_STATIC_INLINE__ int egui_image_std_rgb565_alpha_row_is_all_opaque(const uint8_t *alpha_row, egui_dim_t width, uint8_t alpha_type)
{
    uint16_t full_bytes = 0;
    uint8_t partial_bits = 0;

    switch (alpha_type)
    {
    case EGUI_IMAGE_ALPHA_TYPE_1:
        full_bytes = (uint16_t)(width >> 3);
        partial_bits = (uint8_t)(width & 0x07);
        break;
    case EGUI_IMAGE_ALPHA_TYPE_2:
        full_bytes = (uint16_t)(width >> 2);
        partial_bits = (uint8_t)((width & 0x03) << 1);
        break;
    case EGUI_IMAGE_ALPHA_TYPE_4:
        full_bytes = (uint16_t)(width >> 1);
        partial_bits = (uint8_t)((width & 0x01) << 2);
        break;
    case EGUI_IMAGE_ALPHA_TYPE_8:
        full_bytes = (uint16_t)(width);
        partial_bits = 0;
        break;
    default:
        return 0;
    }

    for (uint16_t i = 0; i < full_bytes; i++)
    {
        if (alpha_row[i] != 0xFF)
        {
            return 0;
        }
    }

    if (partial_bits != 0)
    {
        uint8_t mask = (uint8_t)((1u << partial_bits) - 1u);
        if ((alpha_row[full_bytes] & mask) != mask)
        {
            return 0;
        }
    }

    return 1;
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
__EGUI_STATIC_INLINE__ int egui_image_std_rgb565_external_alpha_is_all_opaque(const egui_image_std_info_t *image, uint16_t alpha_row_size)
{
    uint32_t chunk_rows = (uint32_t)(sizeof(g_external_image_alpha_cache) / alpha_row_size);
    uint32_t chunk_size;
    void *alpha_buf;
    int is_all_opaque = 1;

    if (chunk_rows == 0)
    {
        chunk_rows = 1;
    }

    chunk_size = alpha_row_size * chunk_rows;
    alpha_buf = egui_image_std_acquire_external_buffer(chunk_size, g_external_image_alpha_cache, sizeof(g_external_image_alpha_cache));
    if (alpha_buf == NULL)
    {
        return 0;
    }

    for (egui_dim_t y = 0; y < image->height; y += (egui_dim_t)chunk_rows)
    {
        egui_dim_t rows_in_chunk = image->height - y;
        const uint8_t *chunk_ptr = (const uint8_t *)alpha_buf;

        if (rows_in_chunk > (egui_dim_t)chunk_rows)
        {
            rows_in_chunk = (egui_dim_t)chunk_rows;
        }

        egui_image_std_load_alpha_resource(alpha_buf, (egui_image_std_info_t *)image, (uint32_t)y * alpha_row_size, (uint32_t)rows_in_chunk * alpha_row_size);

        for (egui_dim_t row = 0; row < rows_in_chunk; row++)
        {
            if (!egui_image_std_rgb565_alpha_row_is_all_opaque(chunk_ptr, image->width, image->alpha_type))
            {
                is_all_opaque = 0;
                break;
            }
            chunk_ptr += alpha_row_size;
        }

        if (!is_all_opaque)
        {
            break;
        }
    }

    egui_image_std_release_external_buffer(alpha_buf, g_external_image_alpha_cache);
    return is_all_opaque;
}
#endif

__EGUI_STATIC_INLINE__ int egui_image_std_rgb565_alpha_is_all_opaque(const egui_image_std_info_t *image)
{
    if (image == NULL || image->data_type != EGUI_IMAGE_DATA_TYPE_RGB565 || image->alpha_buf == NULL)
    {
        return 0;
    }

    if (image->opaque_alpha_hint == EGUI_IMAGE_OPAQUE_ALPHA_HINT_OPAQUE)
    {
        return 1;
    }

    if (image->opaque_alpha_hint == EGUI_IMAGE_OPAQUE_ALPHA_HINT_NON_OPAQUE)
    {
        return 0;
    }

    for (uint8_t i = 0; i < (uint8_t)(sizeof(g_egui_image_std_alpha_opaque_cache) / sizeof(g_egui_image_std_alpha_opaque_cache[0])); i++)
    {
        if (g_egui_image_std_alpha_opaque_cache[i].image == image)
        {
            return g_egui_image_std_alpha_opaque_cache[i].is_all_opaque;
        }
    }

    {
        const uint8_t *alpha = (const uint8_t *)image->alpha_buf;
        uint16_t alpha_row_size = egui_image_std_rgb565_alpha_row_size(image->width, image->alpha_type);
        int is_all_opaque = 1;

        if (image->width == 0 || image->height == 0 || alpha_row_size == 0)
        {
            is_all_opaque = 0;
        }
        else
        {
            if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
            {
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                is_all_opaque = egui_image_std_rgb565_external_alpha_is_all_opaque(image, alpha_row_size);
#else
                is_all_opaque = 0;
#endif
            }
            else
            {
                for (egui_dim_t y = 0; y < image->height; y++)
                {
                    if (!egui_image_std_rgb565_alpha_row_is_all_opaque(alpha, image->width, image->alpha_type))
                    {
                        is_all_opaque = 0;
                        break;
                    }
                    alpha += alpha_row_size;
                }
            }
        }

        g_egui_image_std_alpha_opaque_cache[g_egui_image_std_alpha_opaque_cache_next].image = image;
        g_egui_image_std_alpha_opaque_cache[g_egui_image_std_alpha_opaque_cache_next].is_all_opaque = (uint8_t)is_all_opaque;
        g_egui_image_std_alpha_opaque_cache_next++;
        g_egui_image_std_alpha_opaque_cache_next &= 0x03;
        return is_all_opaque;
    }
}

int egui_image_std_rgb565_is_opaque_source(const egui_image_std_info_t *image)
{
    if (image == NULL || image->data_type != EGUI_IMAGE_DATA_TYPE_RGB565)
    {
        return 0;
    }

    if (image->alpha_buf == NULL)
    {
        return 1;
    }

    if (image->opaque_alpha_hint == EGUI_IMAGE_OPAQUE_ALPHA_HINT_OPAQUE)
    {
        return 1;
    }

    if (image->opaque_alpha_hint == EGUI_IMAGE_OPAQUE_ALPHA_HINT_NON_OPAQUE)
    {
        return 0;
    }

    return egui_image_std_rgb565_alpha_is_all_opaque(image);
}

__EGUI_STATIC_INLINE__ int egui_image_std_rgb565_can_use_opaque_draw_fast_path(const egui_image_std_info_t *image, const egui_canvas_t *canvas)
{
    if (!egui_image_std_rgb565_is_opaque_source(image))
    {
        return 0;
    }

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        return canvas == NULL || (canvas->alpha == EGUI_ALPHA_100 && canvas->mask == NULL);
    }
#endif

    if (canvas == NULL || canvas->mask == NULL)
    {
        return 1;
    }

    return canvas->mask->api->kind == EGUI_MASK_KIND_CIRCLE || canvas->mask->api->kind == EGUI_MASK_KIND_ROUND_RECTANGLE;
}

__EGUI_STATIC_INLINE__ int egui_image_std_rgb565_can_use_opaque_resize_fast_path(const egui_image_std_info_t *image, const egui_canvas_t *canvas)
{
    if (!egui_image_std_rgb565_is_opaque_source(image))
    {
        return 0;
    }

    if (canvas == NULL || canvas->mask == NULL)
    {
        return 1;
    }

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        return 1;
    }
#endif

    return canvas->mask->api->kind == EGUI_MASK_KIND_CIRCLE || canvas->mask->api->kind == EGUI_MASK_KIND_ROUND_RECTANGLE;
}
#endif

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

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
    if (image->alpha_buf != NULL && egui_image_std_rgb565_can_use_opaque_draw_fast_path(image, egui_canvas_get_canvas()))
    {
        egui_image_std_set_image_rgb565(self, region.location.x, region.location.y, x_total, y_total, x, y);
        return;
    }
#endif

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

                    egui_dim_t left_img_xe = EGUI_MIN(rr_img_xs, vis_img_xe);
                    if (vis_img_xs < left_img_xe)
                    {
                        egui_dim_t left_offset = vis_img_xs - x;
                        egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + vis_img_xs - canvas->pfb_location_in_base_view.x)];

                        egui_image_std_blend_rgb565_alpha8_masked_mapped_segment(canvas, dst_row, src_row, src_alpha_row, &src_x_map[left_offset],
                                                                                left_img_xe - vis_img_xs, x_base + vis_img_xs, rr_sy, canvas_alpha);
                    }

                    if (rr_img_xs < rr_img_xe)
                    {
                        egui_dim_t mid_offset = rr_img_xs - x;
                        egui_dim_t mid_count = rr_img_xe - rr_img_xs;
                        egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + rr_img_xs - canvas->pfb_location_in_base_view.x)];

                        egui_image_std_blend_rgb565_alpha8_mapped_row(dst_row, src_row, src_alpha_row, &src_x_map[mid_offset], mid_count, canvas_alpha);
                    }

                    egui_dim_t right_img_xs = EGUI_MAX(rr_img_xe, vis_img_xs);
                    if (right_img_xs < vis_img_xe)
                    {
                        egui_dim_t right_offset = right_img_xs - x;
                        egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + right_img_xs - canvas->pfb_location_in_base_view.x)];

                        egui_image_std_blend_rgb565_alpha8_masked_mapped_segment(canvas, dst_row, src_row, src_alpha_row, &src_x_map[right_offset],
                                                                                vis_img_xe - right_img_xs, x_base + right_img_xs, rr_sy, canvas_alpha);
                    }
                }
            }
        }
        else if (canvas->mask->api->mask_get_row_overlay != NULL)
        {
            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                egui_dim_t rr_sy = y_base + y_;
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

                egui_color_t ov_color;
                egui_alpha_t ov_alpha;
                if (canvas->mask->api->mask_get_row_overlay(canvas->mask, rr_sy, &ov_color, &ov_alpha))
                {
                    egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
                    if (ov_alpha == 0)
                    {
                        egui_image_std_blend_rgb565_alpha8_mapped_row(dst_row, src_row, src_alpha_row, src_x_map, count, canvas_alpha);
                    }
                    else
                    {
                        for (egui_dim_t i = 0; i < count; i++)
                        {
                            egui_dim_t src_x = src_x_map[i];
                            egui_alpha_t sa = src_alpha_row[src_x];
                            if (sa == 0)
                            {
                                continue;
                            }
                            egui_color_t c;
                            c.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                            egui_rgb_mix_ptr(&c, &ov_color, &c, ov_alpha);
                            egui_alpha_t a = egui_color_alpha_mix(canvas_alpha, sa);
                            if (a != 0)
                            {
                                egui_image_std_blend_resize_pixel(&dst_row[i], c, a);
                            }
                        }
                    }
                }
                else
                {
                    for (egui_dim_t i = 0; i < count; i++)
                    {
                        egui_dim_t src_x = src_x_map[i];
                        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                        alpha = src_alpha_row[src_x];
                        egui_canvas_draw_point_limit(screen_x_start + i, rr_sy, color, alpha);
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
__EGUI_STATIC_INLINE__ int egui_image_std_can_use_resize_repeat2_fast_path(egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total,
                                                                           egui_float_t width_radio, egui_float_t height_radio)
{
    return (width_radio == EGUI_FLOAT_VALUE(0.5f)) && (height_radio == EGUI_FLOAT_VALUE(0.5f)) && ((x & 0x01) == 0) && ((y & 0x01) == 0) &&
           (((x_total - x) & 0x01) == 0) && (((y_total - y) & 0x01) == 0);
}

static void egui_image_std_draw_image_resize_external_alpha(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total,
                                                            egui_dim_t x_base, egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio,
                                                            uint32_t alpha_row_size, egui_image_std_get_col_pixel_with_alpha *get_col_pixel,
                                                            egui_image_std_blend_mapped_row_func *blend_row_func,
                                                            egui_image_std_blend_repeat2_row_func *blend_repeat2_row_func)
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
    egui_image_std_external_alpha_row_cache_t row_cache;
    int cached_src_y = -1;

    if (!egui_image_std_init_external_alpha_row_cache(&row_cache, data_row_size, alpha_row_size))
    {
        return;
    }

    if (canvas->mask == NULL && blend_repeat2_row_func != NULL &&
        egui_image_std_can_use_resize_repeat2_fast_path(x, y, x_total, y_total, width_radio, height_radio))
    {
        egui_dim_t src_x_start = x >> 1;
        egui_dim_t src_count = (x_total - x) >> 1;

        for (egui_dim_t y_ = y; y_ < y_total; y_ += 2)
        {
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;
            egui_color_int_t *dst_row0 = &canvas->pfb[dst_y * pfb_width + dst_x_start];
            egui_color_int_t *dst_row1 = &canvas->pfb[(dst_y + 1) * pfb_width + dst_x_start];

            src_y = y_ >> 1;
            if (cached_src_y != src_y)
            {
                if (!egui_image_std_load_external_alpha_row_cache(&row_cache, image, src_y))
                {
                    continue;
                }
                cached_src_y = src_y;
            }

            const uint16_t *src_row = egui_image_std_get_external_alpha_row_data(&row_cache, src_y);
            const uint8_t *src_alpha_row = egui_image_std_get_external_alpha_row_alpha(&row_cache, src_y);

            blend_repeat2_row_func(dst_row0, src_row, src_alpha_row, src_x_start, src_count, canvas_alpha);
            blend_repeat2_row_func(dst_row1, src_row, src_alpha_row, src_x_start, src_count, canvas_alpha);
        }

        egui_image_std_release_external_alpha_row_cache(&row_cache);
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
                    if (!egui_image_std_load_external_alpha_row_cache(&row_cache, image, src_y))
                    {
                        continue;
                    }
                    cached_src_y = src_y;
                }

                const uint16_t *src_row = egui_image_std_get_external_alpha_row_data(&row_cache, src_y);
                const uint8_t *src_alpha_row = egui_image_std_get_external_alpha_row_alpha(&row_cache, src_y);

                int rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &rr_x_start, &rr_x_end);
                if (rr_res == EGUI_MASK_ROW_OUTSIDE)
                {
                    continue;
                }
                if (rr_res == EGUI_MASK_ROW_INSIDE)
                {
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
                    /* Use batch row blender: single call instead of per-pixel function pointer */
                    blend_row_func(dst_row, src_row, src_alpha_row, src_x_map, count, canvas_alpha);
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
                        get_col_pixel(src_row, src_alpha_row, src_x_map[x_ - x], &color, &alpha);
                        egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, alpha);
                    }

                    if (rr_img_xs < rr_img_xe)
                    {
                        egui_dim_t mid_offset = rr_img_xs - x;
                        egui_dim_t mid_count = rr_img_xe - rr_img_xs;
                        egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + rr_img_xs - canvas->pfb_location_in_base_view.x)];
                        /* Use batch row blender for the unmasked middle section */
                        blend_row_func(dst_row, src_row, src_alpha_row, &src_x_map[mid_offset], mid_count, canvas_alpha);
                    }

                    for (egui_dim_t x_ = EGUI_MAX(rr_img_xe, vis_img_xs); x_ < vis_img_xe; x_++)
                    {
                        get_col_pixel(src_row, src_alpha_row, src_x_map[x_ - x], &color, &alpha);
                        egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, alpha);
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
                    if (!egui_image_std_load_external_alpha_row_cache(&row_cache, image, src_y))
                    {
                        continue;
                    }
                    cached_src_y = src_y;
                }

                const uint16_t *src_row = egui_image_std_get_external_alpha_row_data(&row_cache, src_y);
                const uint8_t *src_alpha_row = egui_image_std_get_external_alpha_row_alpha(&row_cache, src_y);

                for (egui_dim_t i = 0; i < count; i++)
                {
                    get_col_pixel(src_row, src_alpha_row, src_x_map[i], &color, &alpha);
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
            src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
            if (cached_src_y != src_y)
            {
                if (!egui_image_std_load_external_alpha_row_cache(&row_cache, image, src_y))
                {
                    continue;
                }
                cached_src_y = src_y;
            }

            /* Use batch row blender: single call instead of per-pixel function pointer */
            blend_row_func(dst_row, egui_image_std_get_external_alpha_row_data(&row_cache, src_y),
                           egui_image_std_get_external_alpha_row_alpha(&row_cache, src_y), src_x_map, count, canvas_alpha);
        }
    }

    egui_image_std_release_external_alpha_row_cache(&row_cache);
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
        else if (canvas->mask->api->mask_get_row_overlay != NULL)                                                                                              \
        {                                                                                                                                                      \
            for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                        \
            {                                                                                                                                                  \
                egui_dim_t ov_sy = y_base + y_;                                                                                                                \
                src_y = (egui_dim_t)EGUI_FLOAT_MULT(y_, height_radio);                                                                                         \
                egui_color_t ov_color;                                                                                                                         \
                egui_alpha_t ov_alpha;                                                                                                                         \
                if (canvas->mask->api->mask_get_row_overlay(canvas->mask, ov_sy, &ov_color, &ov_alpha))                                                        \
                {                                                                                                                                              \
                    egui_dim_t ov_dst_y = ov_sy - canvas->pfb_location_in_base_view.y;                                                                         \
                    egui_color_int_t *ov_dst_row = &canvas->pfb[ov_dst_y * pfb_width + dst_x_start];                                                           \
                    for (egui_dim_t i = 0; i < count; i++)                                                                                                     \
                    {                                                                                                                                          \
                        _get_pixel_func(image, src_x_map[i], src_y, &color, &alpha);                                                                           \
                        if (ov_alpha > 0)                                                                                                                      \
                        {                                                                                                                                      \
                            egui_rgb_mix_ptr(&color, &ov_color, &color, ov_alpha);                                                                             \
                        }                                                                                                                                      \
                        alpha = egui_color_alpha_mix(canvas_alpha, alpha);                                                                                     \
                        egui_image_std_blend_resize_pixel(&ov_dst_row[i], color, alpha);                                                                       \
                    }                                                                                                                                          \
                }                                                                                                                                              \
                else                                                                                                                                           \
                {                                                                                                                                              \
                    for (egui_dim_t i = 0; i < count; i++)                                                                                                     \
                    {                                                                                                                                          \
                        _get_pixel_func(image, src_x_map[i], src_y, &color, &alpha);                                                                           \
                        egui_canvas_draw_point_limit(screen_x_start + i, ov_sy, color, alpha);                                                                 \
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
                                                        egui_image_std_blend_rgb565_alpha4_mapped_row,
                                                        egui_image_std_blend_rgb565_alpha4_repeat2_row);
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
                                                        egui_image_std_blend_rgb565_alpha2_mapped_row,
                                                        egui_image_std_blend_rgb565_alpha2_repeat2_row);
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
                                                        egui_image_std_blend_rgb565_alpha1_mapped_row,
                                                        egui_image_std_blend_rgb565_alpha1_repeat2_row);
        return;
    }
#endif
    egui_image_std_set_image_resize_rgb565_1_common(self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
__EGUI_STATIC_INLINE__ int egui_image_std_set_image_resize_rgb565_round_rect_fast(const egui_image_std_info_t *image, const egui_dim_t *src_x_map,
                                                                                   egui_dim_t count, egui_dim_t x, egui_dim_t y, egui_dim_t x_total,
                                                                                   egui_dim_t y_total, egui_dim_t x_base, egui_dim_t y_base,
                                                                                   egui_float_t height_radio)
{
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_mask_round_rectangle_t *round_rect_mask;
    egui_image_std_round_rect_fast_cache_t *round_rect_cache;
    const egui_circle_info_t *round_rect_info;
    const egui_circle_item_t *round_rect_items;
    const uint16_t *src_pixels = image->data_buf;
    egui_dim_t pfb_width = canvas->pfb_region.size.width;
    egui_dim_t screen_x_start = x_base + x;
    egui_dim_t dst_x_start = screen_x_start - canvas->pfb_location_in_base_view.x;
    egui_dim_t round_rect_x;
    egui_dim_t round_rect_y;
    egui_dim_t round_rect_x_end;
    egui_dim_t round_rect_y_end;
    egui_dim_t round_rect_radius;
    egui_dim_t src_y;

    if (canvas->mask == NULL || canvas->mask->api->kind != EGUI_MASK_KIND_ROUND_RECTANGLE || canvas->alpha != EGUI_ALPHA_100)
    {
        return 0;
    }

    round_rect_mask = (egui_mask_round_rectangle_t *)canvas->mask;
    round_rect_cache = egui_image_std_round_rect_fast_cache_get(canvas->mask);
    round_rect_radius = round_rect_mask->radius;
    egui_image_std_round_rect_fast_cache_refresh(round_rect_cache, canvas->mask, round_rect_radius);
    round_rect_x = round_rect_cache->cached_x;
    round_rect_y = round_rect_cache->cached_y;
    round_rect_x_end = round_rect_x + round_rect_cache->cached_width;
    round_rect_y_end = round_rect_y + round_rect_cache->cached_height;
    round_rect_info = round_rect_cache->info;
    if (round_rect_info == NULL)
    {
        return 0;
    }

    round_rect_items = (const egui_circle_item_t *)round_rect_info->items;

    for (egui_dim_t y_ = y; y_ < y_total; y_++)
    {
        egui_dim_t rr_sy = y_base + y_;
        egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
        egui_dim_t rr_x_start;
        egui_dim_t rr_x_end;

        if (rr_sy < round_rect_y || rr_sy >= round_rect_y_end)
        {
            continue;
        }

        src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);

        if (round_rect_radius <= 0 || (rr_sy >= round_rect_y + round_rect_radius && rr_sy < round_rect_y_end - round_rect_radius))
        {
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
            const uint16_t *src_row = &src_pixels[src_y * image->width];
            egui_image_std_blend_rgb565_mapped_row(dst_row, src_row, src_x_map, count, EGUI_ALPHA_100);
            continue;
        }

        {
            egui_dim_t row_cache_slot = ((uint16_t)rr_sy) % EGUI_CONFIG_PFB_HEIGHT;
            egui_dim_t row_index = (rr_sy < round_rect_y + round_rect_radius) ? (rr_sy - round_rect_y) : (round_rect_y_end - 1 - rr_sy);
            egui_dim_t visible_boundary;
            egui_dim_t opaque_boundary;
            egui_dim_t visible_x_start;
            egui_dim_t visible_x_end;
            egui_dim_t rr_img_xs;
            egui_dim_t rr_img_xe;
            egui_dim_t vis_img_xs;
            egui_dim_t vis_img_xe;
            const uint16_t *src_row = &src_pixels[src_y * image->width];

            if (round_rect_cache->row_cache_y[row_cache_slot] == rr_sy)
            {
                visible_boundary = round_rect_cache->row_cache_visible_boundary[row_cache_slot];
                opaque_boundary = round_rect_cache->row_cache_opaque_boundary[row_cache_slot];
            }
            else
            {
                visible_boundary = egui_image_std_get_circle_visible_boundary_cached(row_index, round_rect_radius, round_rect_info, round_rect_items,
                                                                                     &round_rect_cache->visible_cached_row_index,
                                                                                     &round_rect_cache->visible_cached_boundary);
                opaque_boundary = egui_image_std_get_circle_opaque_boundary_cached(row_index, round_rect_radius, round_rect_info, round_rect_items,
                                                                                   &round_rect_cache->opaque_cached_row_index,
                                                                                   &round_rect_cache->opaque_cached_boundary);
                round_rect_cache->row_cache_y[row_cache_slot] = rr_sy;
                round_rect_cache->row_cache_visible_boundary[row_cache_slot] = visible_boundary;
                round_rect_cache->row_cache_opaque_boundary[row_cache_slot] = opaque_boundary;
            }

            visible_x_start = EGUI_MAX(round_rect_x + visible_boundary, screen_x_start);
            visible_x_end = EGUI_MIN(round_rect_x_end - visible_boundary, x_base + x_total);

            if (visible_x_start >= visible_x_end)
            {
                continue;
            }

            rr_x_start = EGUI_MAX(round_rect_x + opaque_boundary, screen_x_start);
            rr_x_end = EGUI_MIN(round_rect_x_end - opaque_boundary, x_base + x_total);
            rr_img_xs = EGUI_MAX(x, rr_x_start - x_base);
            rr_img_xe = EGUI_MIN(x_total, rr_x_end - x_base);
            vis_img_xs = EGUI_MAX(x, visible_x_start - x_base);
            vis_img_xe = EGUI_MIN(x_total, visible_x_end - x_base);

            {
                egui_dim_t left_img_xe = EGUI_MIN(rr_img_xs, vis_img_xe);
                if (vis_img_xs < left_img_xe)
                {
                    egui_dim_t left_offset = vis_img_xs - x;
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + vis_img_xs - canvas->pfb_location_in_base_view.x)];

                    egui_image_std_blend_rgb565_round_rect_masked_left_segment_fixed_row(dst_row, src_row, &src_x_map[left_offset], left_img_xe - vis_img_xs,
                                                                                        x_base + vis_img_xs, round_rect_x, row_index, round_rect_info,
                                                                                        round_rect_items);
                }
            }

            if (rr_img_xs < rr_img_xe)
            {
                egui_dim_t mid_offset = rr_img_xs - x;
                egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + rr_img_xs - canvas->pfb_location_in_base_view.x)];
                egui_image_std_blend_rgb565_mapped_row(dst_row, src_row, &src_x_map[mid_offset], rr_img_xe - rr_img_xs, EGUI_ALPHA_100);
            }

            {
                egui_dim_t right_img_xs = EGUI_MAX(rr_img_xe, vis_img_xs);
                if (right_img_xs < vis_img_xe)
                {
                    egui_dim_t right_offset = right_img_xs - x;
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + right_img_xs - canvas->pfb_location_in_base_view.x)];

                    egui_image_std_blend_rgb565_round_rect_masked_right_segment_fixed_row(dst_row, src_row, &src_x_map[right_offset], vis_img_xe - right_img_xs,
                                                                                         x_base + right_img_xs, round_rect_x_end, row_index,
                                                                                         round_rect_info, round_rect_items);
                }
            }
        }
    }

    return 1;
}

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
        if (egui_image_std_set_image_resize_rgb565_round_rect_fast(image, src_x_map, count, x, y, x_total, y_total, x_base, y_base, height_radio))
        {
            return;
        }

        if (canvas->mask->api->mask_get_row_range != NULL)
        {
            egui_dim_t rr_x_start, rr_x_end;
            int use_circle_edge_fast_path = (canvas->mask->api->kind == EGUI_MASK_KIND_CIRCLE);
            int circle_fast_path_initialized = 0;
            egui_mask_circle_t *circle_mask_fast = NULL;
            const egui_circle_info_t *circle_info = NULL;
            const egui_circle_item_t *circle_items = NULL;
            egui_dim_t circle_center_x = 0;
            egui_dim_t circle_center_y = 0;
            egui_dim_t circle_radius = 0;
            uint32_t circle_visible_radius_sq = 0;
            egui_dim_t circle_visible_cached_dy = -1;
            egui_dim_t circle_visible_cached_half = 0;

            if (use_circle_edge_fast_path)
            {
                circle_mask_fast = (egui_mask_circle_t *)canvas->mask;
            }

            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                egui_dim_t rr_sy = y_base + y_;
                egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                egui_dim_t circle_row_index = 0;
                egui_dim_t circle_visible_half = 0;
                int circle_row_ready = 0;
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                const uint16_t *src_row = &src_pixels[src_y * image->width];
                int rr_res;
                rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &rr_x_start, &rr_x_end);

                if (rr_res == EGUI_MASK_ROW_OUTSIDE)
                {
                    continue;
                }

                if (use_circle_edge_fast_path && !circle_fast_path_initialized)
                {
                    circle_info = circle_mask_fast->info;
                    if (circle_info == NULL)
                    {
                        use_circle_edge_fast_path = 0;
                    }
                    else
                    {
                        circle_items = (const egui_circle_item_t *)circle_info->items;
                        circle_center_x = circle_mask_fast->center_x;
                        circle_center_y = circle_mask_fast->center_y;
                        circle_radius = circle_mask_fast->radius;
                        circle_visible_radius_sq = (uint32_t)(circle_radius + 1) * (uint32_t)(circle_radius + 1);
                        circle_fast_path_initialized = 1;
                    }
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

                    if (use_circle_edge_fast_path && circle_mask_fast->point_cached_y == rr_sy && circle_mask_fast->point_cached_row_valid)
                    {
                        egui_dim_t row_cache_slot = ((uint16_t)rr_sy) % EGUI_CONFIG_PFB_HEIGHT;
                        egui_dim_t dy = (rr_sy > circle_center_y) ? (rr_sy - circle_center_y) : (circle_center_y - rr_sy);

                        circle_row_index = circle_mask_fast->point_cached_row_index;
                        if (circle_mask_fast->row_cache_y[row_cache_slot] == rr_sy)
                        {
                            circle_visible_half = circle_mask_fast->row_cache_visible_half[row_cache_slot];
                        }
                        else if (dy <= circle_radius)
                        {
                            circle_visible_half = egui_image_std_get_circle_visible_half_cached(dy, circle_visible_radius_sq, &circle_visible_cached_dy,
                                                                                                &circle_visible_cached_half);
                            if (circle_visible_half > circle_radius)
                            {
                                circle_visible_half = circle_radius;
                            }
                        }
                        else
                        {
                            circle_visible_half = 0;
                        }
                        circle_row_ready = 1;
                    }

                    if (circle_row_ready)
                    {
                        visible_x_start = EGUI_MAX(circle_center_x - circle_visible_half, screen_x_start);
                        visible_x_end = EGUI_MIN(circle_center_x + circle_visible_half + 1, x_base + x_total);
                        if (visible_x_start >= visible_x_end)
                        {
                            continue;
                        }
                    }
                    else if (canvas->mask->api->mask_get_row_visible_range != NULL &&
                             !canvas->mask->api->mask_get_row_visible_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &visible_x_start, &visible_x_end))
                    {
                        continue;
                    }
                    egui_dim_t rr_img_xs = EGUI_MAX(x, rr_x_start - x_base);
                    egui_dim_t rr_img_xe = EGUI_MIN(x_total, rr_x_end - x_base);
                    egui_dim_t vis_img_xs = EGUI_MAX(x, visible_x_start - x_base);
                    egui_dim_t vis_img_xe = EGUI_MIN(x_total, visible_x_end - x_base);

                    if (circle_row_ready)
                    {
                        egui_dim_t left_img_xe = EGUI_MIN(rr_img_xs, vis_img_xe);
                        if (vis_img_xs < left_img_xe)
                        {
                            egui_dim_t left_offset = vis_img_xs - x;
                            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + vis_img_xs - canvas->pfb_location_in_base_view.x)];

                            egui_image_std_blend_rgb565_circle_masked_mapped_segment_fixed_row(dst_row, src_row, &src_x_map[left_offset], left_img_xe - vis_img_xs,
                                                                                               x_base + vis_img_xs, circle_center_x, circle_radius,
                                                                                               circle_row_index, canvas_alpha, circle_info, circle_items);
                        }

                        if (rr_img_xs < rr_img_xe)
                        {
                            egui_dim_t mid_offset = rr_img_xs - x;
                            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + rr_img_xs - canvas->pfb_location_in_base_view.x)];
                            egui_image_std_blend_rgb565_mapped_row(dst_row, src_row, &src_x_map[mid_offset], rr_img_xe - rr_img_xs, canvas_alpha);
                        }

                        egui_dim_t right_img_xs = EGUI_MAX(rr_img_xe, vis_img_xs);
                        if (right_img_xs < vis_img_xe)
                        {
                            egui_dim_t right_offset = right_img_xs - x;
                            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + right_img_xs - canvas->pfb_location_in_base_view.x)];

                            egui_image_std_blend_rgb565_circle_masked_mapped_segment_fixed_row(dst_row, src_row, &src_x_map[right_offset], vis_img_xe - right_img_xs,
                                                                                               x_base + right_img_xs, circle_center_x, circle_radius,
                                                                                               circle_row_index, canvas_alpha, circle_info, circle_items);
                        }
                    }
                    else
                    {
                        egui_dim_t left_img_xe = EGUI_MIN(rr_img_xs, vis_img_xe);
                        if (vis_img_xs < left_img_xe)
                        {
                            for (egui_dim_t x_ = vis_img_xs; x_ < left_img_xe; x_++)
                            {
                                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[x_ - x]]);
                                egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, EGUI_ALPHA_100);
                            }
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

                        egui_dim_t right_img_xs = EGUI_MAX(rr_img_xe, vis_img_xs);
                        if (right_img_xs < vis_img_xe)
                        {
                            for (egui_dim_t x_ = right_img_xs; x_ < vis_img_xe; x_++)
                            {
                                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[x_ - x]]);
                                egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, EGUI_ALPHA_100);
                            }
                        }
                    }
                }
            }
        }
        else if (canvas->mask->api->mask_get_row_overlay != NULL)
        {
            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                egui_dim_t rr_sy = y_base + y_;
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                const uint16_t *src_row = &src_pixels[src_y * image->width];
                egui_color_t ov_color;
                egui_alpha_t ov_alpha;
                if (canvas->mask->api->mask_get_row_overlay(canvas->mask, rr_sy, &ov_color, &ov_alpha))
                {
                    egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
                    if (ov_alpha == 0)
                    {
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
                        // Pre-compute packed blend factors for overlay
                        uint16_t fg = ov_color.full;
                        uint32_t fg_rb_g = (fg | ((uint32_t)fg << 16)) & 0x07E0F81FUL;
                        uint32_t alpha5 = (uint32_t)ov_alpha >> 3;

                        if (canvas_alpha == EGUI_ALPHA_100)
                        {
                            for (egui_dim_t i = 0; i < count; i++)
                            {
                                uint16_t bg = src_row[src_x_map[i]];
                                uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                                uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * alpha5 >> 5)) & 0x07E0F81FUL;
                                dst_row[i] = (uint16_t)(result | (result >> 16));
                            }
                        }
                        else
                        {
                            for (egui_dim_t i = 0; i < count; i++)
                            {
                                uint16_t bg = src_row[src_x_map[i]];
                                uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                                uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * alpha5 >> 5)) & 0x07E0F81FUL;
                                color.full = (uint16_t)(result | (result >> 16));
                                egui_image_std_blend_resize_pixel(&dst_row[i], color, canvas_alpha);
                            }
                        }
                    }
                }
                else
                {
                    for (egui_dim_t i = 0; i < count; i++)
                    {
                        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                        egui_canvas_draw_point_limit(screen_x_start + i, rr_sy, color, EGUI_ALPHA_100);
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

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
    if (image->alpha_buf != NULL && egui_image_std_rgb565_can_use_opaque_resize_fast_path(image, egui_canvas_get_canvas()))
    {
        egui_image_std_set_image_resize_rgb565(self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
        return;
    }
#endif

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
    if (canvas->mask != NULL)                                                                                                                                  \
    {                                                                                                                                                          \
        for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                            \
        {                                                                                                                                                      \
            const uint8_t *p_data = (const uint8_t *)image->data_buf + (uint32_t)y_ * alpha_row_size;                                                          \
            for (egui_dim_t x_ = x; x_ < x_total; x_++)                                                                                                        \
            {                                                                                                                                                  \
                _get_col_alpha_func(p_data, x_, &pixel_alpha);                                                                                                 \
                if (pixel_alpha)                                                                                                                               \
                {                                                                                                                                              \
                    egui_alpha_t fa_ = (color_alpha == EGUI_ALPHA_100) ? pixel_alpha                                                                           \
                                                                       : (egui_alpha_t)(pixel_alpha * color_alpha / EGUI_ALPHA_100);                           \
                    egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, fa_);                                                                    \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    }                                                                                                                                                          \
    else                                                                                                                                                       \
    {                                                                                                                                                          \
        egui_dim_t pfb_w_ = canvas->pfb_region.size.width;                                                                                                     \
        egui_dim_t pfb_x0_ = (x_base + x) - canvas->pfb_location_in_base_view.x;                                                                              \
        egui_dim_t pfb_yoff_ = y_base - canvas->pfb_location_in_base_view.y;                                                                                   \
        egui_alpha_t comb_a_ = egui_color_alpha_mix(canvas->alpha, color_alpha);                                                                               \
        int comb_is_100_ = (comb_a_ == EGUI_ALPHA_100);                                                                                                        \
        egui_dim_t col_count_ = x_total - x;                                                                                                                   \
        for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                            \
        {                                                                                                                                                      \
            const uint8_t *p_data = (const uint8_t *)image->data_buf + (uint32_t)y_ * alpha_row_size;                                                          \
            egui_color_int_t *dst_row_ = &canvas->pfb[(pfb_yoff_ + y_) * pfb_w_ + pfb_x0_];                                                                   \
            for (egui_dim_t i_ = 0; i_ < col_count_; i_++)                                                                                                     \
            {                                                                                                                                                  \
                _get_col_alpha_func(p_data, x + i_, &pixel_alpha);                                                                                             \
                if (pixel_alpha)                                                                                                                               \
                {                                                                                                                                              \
                    egui_alpha_t fa_ = comb_is_100_ ? pixel_alpha                                                                                              \
                                                    : (egui_alpha_t)(((uint16_t)comb_a_ * pixel_alpha + 128) >> 8);                                            \
                    egui_image_std_blend_resize_pixel(&dst_row_[i_], color, fa_);                                                                               \
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

/**
 * Inner loop macro for resize-color no-mask path.
 * Hoists src_y to outer loop, caches p_data_row per source row,
 * uses pre-computed src_x_map, and writes directly to PFB.
 */
#define EGUI_IMAGE_RESIZE_COLOR_FAST_LOOP(_get_alpha_func, _row_size)                                                                                          \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        int cached_src_y_ = -1;                                                                                                                                \
        const uint8_t *p_row_ = NULL;                                                                                                                          \
        for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)                                                                                            \
        {                                                                                                                                                      \
            egui_dim_t sy_ = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);                                                                             \
            if (cached_src_y_ != sy_)                                                                                                                          \
            {                                                                                                                                                  \
                p_row_ = (const uint8_t *)image->data_buf + (uint32_t)sy_ * (_row_size);                                                                       \
                cached_src_y_ = sy_;                                                                                                                           \
            }                                                                                                                                                  \
            egui_color_int_t *dst_row_ = &canvas->pfb[(pfb_y_off + y_) * pfb_width + pfb_x_start];                                                            \
            for (egui_dim_t i_ = 0; i_ < count; i_++)                                                                                                          \
            {                                                                                                                                                  \
                egui_alpha_t pa_;                                                                                                                               \
                _get_alpha_func(p_row_, src_x_map[i_], &pa_);                                                                                                  \
                if (pa_)                                                                                                                                       \
                {                                                                                                                                              \
                    egui_alpha_t fa_ = combined_is_100 ? pa_ : (egui_alpha_t)(((uint16_t)combined_alpha * pa_ + 128) >> 8);                                    \
                    egui_image_std_blend_resize_pixel(&dst_row_[i_], color, fa_);                                                                               \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    } while (0)

/**
 * Inner loop macro for resize-color mask path.
 * Uses src_x_map and hoisted src_y, but still calls draw_point_limit for mask support.
 */
#define EGUI_IMAGE_RESIZE_COLOR_MASK_LOOP(_get_alpha_func, _row_size)                                                                                          \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        int cached_src_y_ = -1;                                                                                                                                \
        const uint8_t *p_row_ = NULL;                                                                                                                          \
        for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)                                                                                            \
        {                                                                                                                                                      \
            egui_dim_t sy_ = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);                                                                             \
            if (cached_src_y_ != sy_)                                                                                                                          \
            {                                                                                                                                                  \
                p_row_ = (const uint8_t *)image->data_buf + (uint32_t)sy_ * (_row_size);                                                                       \
                cached_src_y_ = sy_;                                                                                                                           \
            }                                                                                                                                                  \
            egui_dim_t screen_y_ = y + y_;                                                                                                                     \
            for (egui_dim_t i_ = 0; i_ < count; i_++)                                                                                                          \
            {                                                                                                                                                  \
                egui_alpha_t pa_;                                                                                                                               \
                _get_alpha_func(p_row_, src_x_map[i_], &pa_);                                                                                                  \
                if (pa_)                                                                                                                                       \
                {                                                                                                                                              \
                    egui_alpha_t fa_ = (color_alpha == EGUI_ALPHA_100) ? pa_ : (egui_alpha_t)(pa_ * color_alpha / EGUI_ALPHA_100);                             \
                    egui_canvas_draw_point_limit(screen_x_start + i_, screen_y_, color, fa_);                                                                  \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    } while (0)

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
        egui_canvas_t *canvas = egui_canvas_get_canvas();

        // Pre-compute src_x mapping to eliminate per-pixel FLOAT_MULT
        egui_dim_t src_x_map[EGUI_CONFIG_PFB_WIDTH];
        egui_dim_t count = egui_image_std_prepare_resize_src_x_map_limit(src_x_map, region.location.x, x_total, width_radio);

        if (canvas->mask != NULL)
        {
            // Mask path: use per-pixel draw_point_limit for mask support
            // Still benefits from src_x_map and hoisted src_y
            egui_dim_t screen_x_start = x + region.location.x;

            switch (image->alpha_type)
            {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8
            case EGUI_IMAGE_ALPHA_TYPE_8:
                EGUI_IMAGE_RESIZE_COLOR_MASK_LOOP(egui_image_std_get_col_alpha_8, image->width);
                break;
#endif
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4
            case EGUI_IMAGE_ALPHA_TYPE_4:
                EGUI_IMAGE_RESIZE_COLOR_MASK_LOOP(egui_image_std_get_col_alpha_4, ((image->width + 1) >> 1));
                break;
#endif
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2
            case EGUI_IMAGE_ALPHA_TYPE_2:
                EGUI_IMAGE_RESIZE_COLOR_MASK_LOOP(egui_image_std_get_col_alpha_2, ((image->width + 3) >> 2));
                break;
#endif
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1
            case EGUI_IMAGE_ALPHA_TYPE_1:
                EGUI_IMAGE_RESIZE_COLOR_MASK_LOOP(egui_image_std_get_col_alpha_1, ((image->width + 7) >> 3));
                break;
#endif
            default:
                EGUI_ASSERT(0);
                break;
            }
        }
        else
        {
            // No-mask fast path: direct PFB writes
            egui_dim_t pfb_width = canvas->pfb_region.size.width;
            egui_dim_t pfb_x_start = (x + region.location.x) - canvas->pfb_location_in_base_view.x;
            egui_dim_t pfb_y_off = y - canvas->pfb_location_in_base_view.y;

            // Pre-combine canvas alpha and color alpha
            egui_alpha_t combined_alpha = egui_color_alpha_mix(canvas->alpha, color_alpha);
            int combined_is_100 = (combined_alpha == EGUI_ALPHA_100);

            switch (image->alpha_type)
            {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8
            case EGUI_IMAGE_ALPHA_TYPE_8:
                EGUI_IMAGE_RESIZE_COLOR_FAST_LOOP(egui_image_std_get_col_alpha_8, image->width);
                break;
#endif
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4
            case EGUI_IMAGE_ALPHA_TYPE_4:
                EGUI_IMAGE_RESIZE_COLOR_FAST_LOOP(egui_image_std_get_col_alpha_4, ((image->width + 1) >> 1));
                break;
#endif
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2
            case EGUI_IMAGE_ALPHA_TYPE_2:
                EGUI_IMAGE_RESIZE_COLOR_FAST_LOOP(egui_image_std_get_col_alpha_2, ((image->width + 3) >> 2));
                break;
#endif
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1
            case EGUI_IMAGE_ALPHA_TYPE_1:
                EGUI_IMAGE_RESIZE_COLOR_FAST_LOOP(egui_image_std_get_col_alpha_1, ((image->width + 7) >> 3));
                break;
#endif
            default:
                EGUI_ASSERT(0);
                break;
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
