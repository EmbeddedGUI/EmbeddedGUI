#include <string.h>

#include "egui_mask_gradient.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"
#include "core/egui_canvas_gradient.h"

void egui_mask_gradient_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    EGUI_LOCAL_INIT(egui_mask_gradient_t);

    if (local->gradient == NULL || local->overlay_alpha == 0)
    {
        return;
    }

    egui_dim_t pw = self->region.size.width;
    egui_dim_t ph = self->region.size.height;
    egui_dim_t px = x - self->region.location.x;
    egui_dim_t py = y - self->region.location.y;
    egui_color_t grad_color;

    /* Clamp to gradient region */
    if (px < 0 || py < 0 || px >= pw || py >= ph)
    {
        return;
    }

    /* Row-level cache for LINEAR_VERTICAL: gradient depends only on py,
     * so all pixels in the same row share the same gradient color. */
    if (local->gradient->type == EGUI_GRADIENT_TYPE_LINEAR_VERTICAL && py == local->cached_key)
    {
        grad_color = local->cached_color;
    }
    else
    {
        grad_color = egui_gradient_color_at_pos(local->gradient, px, py, pw, ph);
        if (local->gradient->type == EGUI_GRADIENT_TYPE_LINEAR_VERTICAL)
        {
            local->cached_key = py;
            local->cached_color = grad_color;
        }
    }

    /* Blend gradient color over original draw color */
    egui_rgb_mix_ptr(color, &grad_color, color, local->overlay_alpha);

    /* alpha is intentionally NOT modified: shape AA and image transparency are preserved */
}

const egui_mask_api_t egui_mask_gradient_t_api_table = {
        .mask_point = egui_mask_gradient_mask_point,
        .mask_get_row_range = NULL,
};

void egui_mask_gradient_init(egui_mask_t *self)
{
    EGUI_LOCAL_INIT(egui_mask_gradient_t);
    egui_mask_init(self);
    self->api = &egui_mask_gradient_t_api_table;

    local->gradient = NULL;
    local->overlay_alpha = EGUI_ALPHA_100;
    local->cached_key = -32768; /* sentinel: won't match any valid coordinate */
}

void egui_mask_gradient_set_gradient(egui_mask_t *self, const egui_gradient_t *gradient)
{
    EGUI_LOCAL_INIT(egui_mask_gradient_t);
    local->gradient = gradient;
}

void egui_mask_gradient_set_overlay_alpha(egui_mask_t *self, egui_alpha_t overlay_alpha)
{
    EGUI_LOCAL_INIT(egui_mask_gradient_t);
    local->overlay_alpha = overlay_alpha;
}
