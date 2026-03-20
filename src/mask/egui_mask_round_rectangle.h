#ifndef _EGUI_MASK_ROUND_RECTANGLE_H_
#define _EGUI_MASK_ROUND_RECTANGLE_H_

#include "egui_mask.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_mask_round_rectangle egui_mask_round_rectangle_t;
struct egui_mask_round_rectangle
{
    egui_mask_t base;

    egui_dim_t radius;
};

void egui_mask_round_rectangle_set_radius(egui_mask_t *self, egui_dim_t radius);
void egui_mask_round_rectangle_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha);
int egui_mask_round_rectangle_fill_row_segment(egui_mask_t *self, egui_color_int_t *dst, egui_dim_t y, egui_dim_t x_start, egui_dim_t x_end, egui_color_t color,
                                               egui_alpha_t alpha);
int egui_mask_round_rectangle_blend_rgb565_alpha8_segment(egui_mask_t *self, egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                          const egui_dim_t *src_x_map, egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y,
                                                          egui_alpha_t canvas_alpha);
void egui_mask_round_rectangle_init(egui_mask_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_MASK_ROUND_RECTANGLE_H_ */
