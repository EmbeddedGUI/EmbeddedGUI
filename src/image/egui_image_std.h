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

void egui_image_std_draw_image(const egui_image_t *self, egui_dim_t x, egui_dim_t y);
void egui_image_std_draw_image_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height);
int egui_image_std_rgb565_is_opaque_source(const egui_image_std_info_t *image);
int egui_image_std_get_linear_src_x_segment(const egui_dim_t *src_x_map, egui_dim_t start, egui_dim_t end, egui_dim_t *src_x_start);
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
const egui_image_std_info_t *egui_image_std_prepare_external_persistent_cache(const egui_image_std_info_t *image);

#if EGUI_CONFIG_IMAGE_EXTERNAL_ROW_CACHE_SHARE_BUFFERS
typedef enum
{
    EGUI_IMAGE_EXTERNAL_ROW_CACHE_OWNER_NONE = 0,
    EGUI_IMAGE_EXTERNAL_ROW_CACHE_OWNER_STD = 1,
    EGUI_IMAGE_EXTERNAL_ROW_CACHE_OWNER_TRANSFORM = 2,
} egui_image_external_row_cache_owner_t;

uint32_t egui_image_std_claim_shared_external_row_cache(egui_image_external_row_cache_owner_t owner);
uint16_t *egui_image_std_get_shared_external_data_cache(void);
uint8_t *egui_image_std_get_shared_external_alpha_cache(void);
#endif
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
