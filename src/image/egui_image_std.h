#ifndef _EGUI_IMAGE_STD_H_
#define _EGUI_IMAGE_STD_H_

#include "egui_image.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Image data type and alpha type macros are now in egui_image.h */

typedef struct
{
    const void *data_buf;
    const void *alpha_buf;
    uint8_t data_type;  // image data type, EGUI_IMAGE_DATA_TYPE_RGB32, EGUI_IMAGE_DATA_TYPE_RGB565, EGUI_IMAGE_DATA_TYPE_GRAY8
    uint8_t alpha_type; // image bit size 1, 2, 4, 8
    uint8_t res_type;   // EGUI_RESOURCE_TYPE_INTERNAL, EGUI_RESOURCE_TYPE_EXTERNAL
    uint16_t width;     // image width
    uint16_t height;    // image height
} egui_image_std_info_t;

typedef struct egui_image_std egui_image_std_t;
struct egui_image_std
{
    egui_image_t base;
};

extern const egui_image_api_t egui_image_std_t_api_table;

static inline void egui_image_std_blend_rgb565_src_pixel_fast(egui_color_int_t *dst, uint16_t src_pixel, egui_alpha_t alpha)
{
    if (alpha == 0)
    {
        return;
    }

#if EGUI_CONFIG_COLOR_DEPTH == 16
    if (alpha > 251)
    {
        *dst = src_pixel;
        return;
    }
    if (alpha < 4)
    {
        return;
    }

    {
        uint16_t bg = *dst;
        uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
        uint32_t fg_rb_g = (src_pixel | ((uint32_t)src_pixel << 16)) & 0x07E0F81FUL;
        uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * ((uint32_t)alpha >> 3) >> 5)) & 0x07E0F81FUL;

        *dst = (uint16_t)(result | (result >> 16));
    }
#else
    {
        egui_color_t color;

        color.full = EGUI_COLOR_RGB565_TRANS(src_pixel);
        if (alpha == EGUI_ALPHA_100)
        {
            *dst = color.full;
        }
        else
        {
            egui_rgb_mix_ptr((egui_color_t *)dst, &color, (egui_color_t *)dst, alpha);
        }
    }
#endif
}

static inline void egui_image_std_copy_rgb565_row_fast(egui_color_int_t *dst_row, const uint16_t *src_row, egui_dim_t count)
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

static inline void egui_image_std_copy_rgb565_row_block_fast(egui_color_int_t *dst_row, egui_dim_t dst_stride, const uint16_t *src_row, egui_dim_t src_stride,
                                                             egui_dim_t row_count, egui_dim_t col_count)
{
    if (row_count <= 0 || col_count <= 0)
    {
        return;
    }

#if EGUI_CONFIG_COLOR_DEPTH == 16
    if (((dst_stride | src_stride) & 0x01U) != 0U)
    {
        for (egui_dim_t row = 0; row < row_count; row++)
        {
            egui_image_std_copy_rgb565_row_fast(dst_row, src_row, col_count);
            dst_row += dst_stride;
            src_row += src_stride;
        }
        return;
    }

    if ((((egui_uintptr_t)dst_row ^ (egui_uintptr_t)src_row) & 0x03U) != 0U)
    {
        for (egui_dim_t row = 0; row < row_count; row++)
        {
            for (egui_dim_t col = 0; col < col_count; col++)
            {
                dst_row[col] = src_row[col];
            }
            dst_row += dst_stride;
            src_row += src_stride;
        }
        return;
    }

    {
        int has_lead_pixel = ((((egui_uintptr_t)dst_row) & 0x03U) != 0U);
        egui_dim_t bulk_count = has_lead_pixel ? (col_count - 1) : col_count;
        egui_dim_t bulk8_count = bulk_count / 8;
        egui_dim_t bulk2_count = (bulk_count & 0x07) / 2;
        egui_dim_t tail_count = bulk_count & 0x01;

        for (egui_dim_t row = 0; row < row_count; row++)
        {
            egui_color_int_t *row_dst = dst_row;
            const uint16_t *row_src = src_row;

            if (has_lead_pixel)
            {
                *row_dst++ = *row_src++;
            }

            if (bulk_count >= 2)
            {
                uint32_t *dst32 = (uint32_t *)row_dst;
                const uint32_t *src32 = (const uint32_t *)row_src;

                for (egui_dim_t i = 0; i < bulk8_count; i++)
                {
                    dst32[0] = src32[0];
                    dst32[1] = src32[1];
                    dst32[2] = src32[2];
                    dst32[3] = src32[3];
                    dst32 += 4;
                    src32 += 4;
                }

                for (egui_dim_t i = 0; i < bulk2_count; i++)
                {
                    *dst32++ = *src32++;
                }

                row_dst = (egui_color_int_t *)dst32;
                row_src = (const uint16_t *)src32;
            }

            if (tail_count != 0)
            {
                *row_dst = *row_src;
            }

            dst_row += dst_stride;
            src_row += src_stride;
        }
    }
#else
    for (egui_dim_t row = 0; row < row_count; row++)
    {
        egui_image_std_copy_rgb565_row_fast(dst_row, src_row, col_count);
        dst_row += dst_stride;
        src_row += src_stride;
    }
#endif
}

static inline void egui_image_std_blend_rgb565_row_fast(egui_color_int_t *dst_row, const uint16_t *src_row, egui_dim_t count, egui_alpha_t alpha)
{
    if (alpha == 0)
    {
        return;
    }

    if (alpha == EGUI_ALPHA_100)
    {
        egui_image_std_copy_rgb565_row_fast(dst_row, src_row, count);
        return;
    }

    for (egui_dim_t i = 0; i < count; i++)
    {
        egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[i], src_row[i], alpha);
    }
}

static inline void egui_image_std_blend_rgb565_row_block_fast(egui_color_int_t *dst_row, egui_dim_t dst_stride, const uint16_t *src_row, egui_dim_t src_stride,
                                                              egui_dim_t row_count, egui_dim_t col_count, egui_alpha_t alpha)
{
    if (row_count <= 0 || col_count <= 0 || alpha == 0)
    {
        return;
    }

    if (alpha == EGUI_ALPHA_100)
    {
        egui_image_std_copy_rgb565_row_block_fast(dst_row, dst_stride, src_row, src_stride, row_count, col_count);
        return;
    }

    for (egui_dim_t row = 0; row < row_count; row++)
    {
        for (egui_dim_t col = 0; col < col_count; col++)
        {
            egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[col], src_row[col], alpha);
        }
        dst_row += dst_stride;
        src_row += src_stride;
    }
}

static inline void egui_image_std_blend_rgb565_alpha8_row_full_alpha_fast(egui_color_int_t *dst_row, const uint16_t *src_pixels, const uint8_t *src_alpha_row,
                                                                          egui_dim_t count)
{
    egui_dim_t i = 0;

    while (i < count)
    {
        while (i + 4 <= count && *(const uint32_t *)&src_alpha_row[i] == 0x00000000U)
        {
            i += 4;
        }
        while (i < count && src_alpha_row[i] == 0)
        {
            i++;
        }

        {
            egui_dim_t opaque_start = i;

            while (i + 4 <= count && *(const uint32_t *)&src_alpha_row[i] == 0xFFFFFFFFU)
            {
                i += 4;
            }
            while (i < count && src_alpha_row[i] == EGUI_ALPHA_100)
            {
                i++;
            }

            if (opaque_start < i)
            {
                egui_image_std_copy_rgb565_row_fast(&dst_row[opaque_start], &src_pixels[opaque_start], i - opaque_start);
                continue;
            }
        }

        if (i < count)
        {
#if EGUI_CONFIG_COLOR_DEPTH == 16
            if (src_alpha_row[i] >= 4)
#else
            if (src_alpha_row[i] >= 1)
#endif
            {
                egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[i], src_pixels[i], src_alpha_row[i]);
            }
            i++;
        }
    }
}

static inline void egui_image_std_blend_rgb565_alpha8_row_fast(egui_color_int_t *dst_row, const uint16_t *src_pixels, const uint8_t *src_alpha_row,
                                                               egui_dim_t count, egui_alpha_t canvas_alpha)
{
    egui_dim_t i = 0;

    if (canvas_alpha == 0)
    {
        return;
    }

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        egui_image_std_blend_rgb565_alpha8_row_full_alpha_fast(dst_row, src_pixels, src_alpha_row, count);
        return;
    }

    while (i < count)
    {
        while (i + 4 <= count && *(const uint32_t *)&src_alpha_row[i] == 0x00000000U)
        {
            i += 4;
        }
        while (i < count && src_alpha_row[i] == 0)
        {
            i++;
        }

        {
            egui_dim_t opaque_start = i;

            while (i + 4 <= count && *(const uint32_t *)&src_alpha_row[i] == 0xFFFFFFFFU)
            {
                i += 4;
            }
            while (i < count && src_alpha_row[i] == EGUI_ALPHA_100)
            {
                i++;
            }

            if (opaque_start < i)
            {
                egui_image_std_blend_rgb565_row_fast(&dst_row[opaque_start], &src_pixels[opaque_start], i - opaque_start, canvas_alpha);
                continue;
            }
        }

        if (i < count)
        {
            egui_alpha_t src_alpha = src_alpha_row[i];

#if EGUI_CONFIG_COLOR_DEPTH == 16
            if (src_alpha >= 4)
#else
            if (src_alpha >= 1)
#endif
            {
                egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, src_alpha);

                if (alpha != 0)
                {
                    egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[i], src_pixels[i], alpha);
                }
            }
            i++;
        }
    }
}

void egui_image_std_draw_image(const egui_image_t *self, egui_dim_t x, egui_dim_t y);
void egui_image_std_draw_image_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height);
int egui_image_std_rgb565_is_opaque_source(const egui_image_std_info_t *image);
int egui_image_std_get_linear_src_x_segment(const egui_dim_t *src_x_map, egui_dim_t start, egui_dim_t end, egui_dim_t *src_x_start);
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
const egui_image_std_info_t *egui_image_std_prepare_external_persistent_cache(const egui_image_std_info_t *image);

typedef enum
{
    EGUI_IMAGE_EXTERNAL_ROW_CACHE_OWNER_NONE = 0,
    EGUI_IMAGE_EXTERNAL_ROW_CACHE_OWNER_STD = 1,
    EGUI_IMAGE_EXTERNAL_ROW_CACHE_OWNER_TRANSFORM = 2,
    EGUI_IMAGE_EXTERNAL_ROW_CACHE_OWNER_RLE = 3,
} egui_image_external_row_cache_owner_t;

uint32_t egui_image_std_claim_shared_external_row_cache(egui_image_external_row_cache_owner_t owner);
uint16_t *egui_image_std_get_shared_external_data_cache(void);
uint8_t *egui_image_std_get_shared_external_alpha_cache(void);
#endif
void egui_image_std_release_frame_cache(void);

void egui_image_std_draw_image_color(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha);
void egui_image_std_draw_image_resize_color(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color,
                                            egui_alpha_t alpha);
int egui_image_std_blend_rgb565_masked_row(egui_canvas_t *canvas, egui_color_int_t *dst_row, const uint16_t *src_row, egui_dim_t count, egui_dim_t screen_x,
                                           egui_dim_t screen_y, egui_alpha_t canvas_alpha);
int egui_image_std_blend_rgb565_masked_row_block(egui_canvas_t *canvas, egui_color_int_t *dst_row, egui_dim_t dst_stride, const uint16_t *src_row,
                                                 egui_dim_t src_stride, egui_dim_t row_count, egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y,
                                                 egui_alpha_t canvas_alpha);
void egui_image_std_blend_rgb565_alpha8_masked_row(egui_canvas_t *canvas, egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                   egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y, egui_alpha_t canvas_alpha);
int egui_image_std_blend_rgb565_alpha8_masked_row_block(egui_canvas_t *canvas, egui_color_int_t *dst_row, egui_dim_t dst_stride, const uint16_t *src_row,
                                                        egui_dim_t src_stride, const uint8_t *src_alpha_row, egui_dim_t alpha_stride, egui_dim_t row_count,
                                                        egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y, egui_alpha_t canvas_alpha);

void egui_image_std_get_width_height(const egui_image_t *self, egui_dim_t *width, egui_dim_t *height);
void egui_image_std_init(egui_image_t *self, const void *res);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_IMAGE_STD_H_ */
