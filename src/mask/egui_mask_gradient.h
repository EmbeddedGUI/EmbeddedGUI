#ifndef _EGUI_MASK_GRADIENT_H_
#define _EGUI_MASK_GRADIENT_H_

#include "egui_mask.h"
#include "core/egui_canvas_gradient.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Gradient color overlay mask.
 *
 * When set on the canvas, this mask intercepts every drawn pixel and blends
 * the gradient color on top of the original draw color.
 *
 * overlay_alpha controls blend strength:
 *   - 0   = no change (original draw color is preserved)
 *   - 255 = full gradient color replaces the draw color
 *
 * The original pixel alpha (shape AA, image transparency) is never modified,
 * so gradient overlay preserves shape edges and image transparency perfectly.
 */
typedef struct egui_mask_gradient egui_mask_gradient_t;
struct egui_mask_gradient
{
    egui_mask_t base;

    const egui_gradient_t *gradient;
    egui_alpha_t overlay_alpha;

    egui_dim_t cached_key;     /* cached row y (for LINEAR_VERTICAL) */
    egui_color_t cached_color; /* gradient color for cached_key row */
};

void egui_mask_gradient_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha);

void egui_mask_gradient_init(egui_mask_t *self);

void egui_mask_gradient_set_gradient(egui_mask_t *self, const egui_gradient_t *gradient);

void egui_mask_gradient_set_overlay_alpha(egui_mask_t *self, egui_alpha_t overlay_alpha);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_MASK_GRADIENT_H_ */
