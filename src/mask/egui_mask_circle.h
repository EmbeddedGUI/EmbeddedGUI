#ifndef _EGUI_MASK_COLOR_H_
#define _EGUI_MASK_COLOR_H_

#include "egui_mask.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_circle_info egui_circle_info_t;
typedef struct egui_mask_circle_frame_row_cache egui_mask_circle_frame_row_cache_t;

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
    egui_dim_t point_cached_y;
    egui_dim_t point_cached_row_index;
    uint8_t point_cached_row_valid;
    const egui_circle_info_t *info;
};

egui_alpha_t egui_mask_circle_get_corner_alpha(egui_dim_t radius, egui_dim_t row_index, egui_dim_t col_index);
void egui_mask_circle_get_row_metrics(egui_dim_t radius, egui_dim_t row_index, egui_dim_t *visible_half, egui_dim_t *opaque_boundary);
int egui_mask_circle_prepare_row(egui_mask_circle_t *self, egui_dim_t y, egui_dim_t *row_index, egui_dim_t *visible_half, egui_dim_t *opaque_boundary);
void egui_mask_circle_release_frame_cache(void);
void egui_mask_circle_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha);
int egui_mask_circle_fill_row_segment(egui_mask_t *self, egui_color_int_t *dst, egui_dim_t y, egui_dim_t x_start, egui_dim_t x_end, egui_color_t color,
                                      egui_alpha_t alpha);
void egui_mask_circle_init(egui_mask_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_MASK_COLOR_H_ */
