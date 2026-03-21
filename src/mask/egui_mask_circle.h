#ifndef _EGUI_MASK_COLOR_H_
#define _EGUI_MASK_COLOR_H_

#include "egui_mask.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_circle_info egui_circle_info_t;

typedef struct egui_mask_circle egui_mask_circle_t;
struct egui_mask_circle
{
    egui_mask_t base;

    egui_dim_t cached_x;
    egui_dim_t cached_y;
    egui_dim_t cached_width;
    egui_dim_t cached_height;
    egui_dim_t cached_x_end;
    egui_dim_t cached_y_end;
    egui_dim_t center_x;
    egui_dim_t center_y;
    egui_dim_t radius;
    uint32_t visible_radius_sq;
    egui_dim_t visible_cached_dy;
    egui_dim_t visible_cached_half;
    egui_dim_t opaque_cached_row_index;
    egui_dim_t opaque_cached_boundary;
    egui_dim_t point_cached_y;
    egui_dim_t point_cached_row_index;
    uint8_t point_cached_row_valid;
    egui_dim_t row_cache_y[EGUI_CONFIG_PFB_HEIGHT];
    egui_dim_t row_cache_visible_half[EGUI_CONFIG_PFB_HEIGHT];
    egui_dim_t row_cache_opaque_boundary[EGUI_CONFIG_PFB_HEIGHT];
    const egui_circle_info_t *info;
};

void egui_mask_circle_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha);
int egui_mask_circle_fill_row_segment(egui_mask_t *self, egui_color_int_t *dst, egui_dim_t y, egui_dim_t x_start, egui_dim_t x_end, egui_color_t color,
                                      egui_alpha_t alpha);
void egui_mask_circle_init(egui_mask_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_MASK_COLOR_H_ */
