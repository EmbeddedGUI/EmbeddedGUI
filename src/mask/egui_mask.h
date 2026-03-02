#ifndef _EGUI_MASK_H_
#define _EGUI_MASK_H_

#include "core/egui_common.h"
#include "core/egui_region.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    EGUI_MASK_ROW_OUTSIDE = 0, // Entire row invisible (masked out)
    EGUI_MASK_ROW_INSIDE,      // Entire row fully visible (no AA)
    EGUI_MASK_ROW_PARTIAL,     // Partially visible, [x_start, x_end) is opaque span
} egui_mask_row_result_t;

typedef struct egui_mask_api egui_mask_api_t;
struct egui_mask_api
{
    void (*mask_point)(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha);
    // Optional: row-range query for batch optimization. NULL means not supported (fallback to per-pixel).
    // Given row y and horizontal range [x_min, x_max), returns the opaque span [*x_start, *x_end).
    int (*mask_get_row_range)(egui_mask_t *self, egui_dim_t y, egui_dim_t x_min, egui_dim_t x_max, egui_dim_t *x_start, egui_dim_t *x_end);
};

struct egui_mask
{
    egui_region_t region;

    const egui_mask_api_t *api; // virtual api
};

void egui_mask_set_position(egui_mask_t *self, egui_dim_t x, egui_dim_t y);
void egui_mask_set_size(egui_mask_t *self, egui_dim_t width, egui_dim_t height);
void egui_mask_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha);
void egui_mask_init(egui_mask_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_MASK_H_ */
