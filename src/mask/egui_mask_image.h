#ifndef _EGUI_MASK_IMAGE_H_
#define _EGUI_MASK_IMAGE_H_

#include "egui_mask.h"
#include "image/egui_image.h"
#include "utils/egui_fixmath.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_mask_image egui_mask_image_t;
struct egui_mask_image
{
    egui_mask_t base;

    egui_image_t *img;
    egui_image_t *cached_img;
    egui_float_t width_radio;
    egui_float_t height_radio;
    const void *alpha_buf;
    egui_dim_t src_width;
    egui_dim_t src_height;
    egui_dim_t cached_x;
    egui_dim_t cached_y;
    egui_dim_t cached_width;
    egui_dim_t cached_height;
    egui_dim_t cached_x_end;
    egui_dim_t cached_y_end;
    egui_dim_t point_cached_y;
    egui_dim_t point_cached_src_y;
    egui_dim_t row_cached_src_y;
    egui_dim_t row_visible_x_start;
    egui_dim_t row_visible_x_end;
    egui_dim_t row_opaque_x_start;
    egui_dim_t row_opaque_x_end;
    const uint8_t *point_cached_alpha_row;
    const uint8_t *row_cached_alpha_row;
    uint8_t alpha_type;
    uint8_t res_type;
    uint8_t fast_path_supported;
    uint8_t identity_scale;
    uint8_t point_cached_valid;
    uint8_t row_cached_valid;
    uint8_t row_cached_has_visible;
    uint8_t row_cached_has_opaque;
};

void egui_mask_image_set_image(egui_mask_t *self, egui_image_t *img);
void egui_mask_image_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha);
int egui_mask_image_fill_row_segment(egui_mask_t *self, egui_color_int_t *dst, egui_dim_t y, egui_dim_t x_start, egui_dim_t x_end, egui_color_t color,
                                     egui_alpha_t alpha);
int egui_mask_image_blend_rgb565_row_segment(egui_mask_t *self, egui_color_int_t *dst_row, const uint16_t *src_row,
                                             egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y, egui_alpha_t canvas_alpha);
int egui_mask_image_blend_rgb565_row_block(egui_mask_t *self, egui_color_int_t *dst_row, egui_dim_t dst_stride, const uint16_t *src_row,
                                           egui_dim_t src_stride, egui_dim_t row_count, egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y,
                                           egui_alpha_t canvas_alpha);
int egui_mask_image_blend_rgb565_alpha8_row_segment(egui_mask_t *self, egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                    egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y, egui_alpha_t canvas_alpha);
int egui_mask_image_blend_rgb565_alpha8_row_block(egui_mask_t *self, egui_color_int_t *dst_row, egui_dim_t dst_stride, const uint16_t *src_row,
                                                  egui_dim_t src_stride, const uint8_t *src_alpha_row, egui_dim_t alpha_stride, egui_dim_t row_count,
                                                  egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y, egui_alpha_t canvas_alpha);
int egui_mask_image_blend_rgb565_alpha8_segment(egui_mask_t *self, egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                const egui_dim_t *src_x_map, egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y,
                                                egui_alpha_t canvas_alpha);
void egui_mask_image_init(egui_mask_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_MASK_IMAGE_H_ */
