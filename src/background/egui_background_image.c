#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_background_image.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "canvas/egui_canvas.h"

/**
 * @file egui_background_image.c
 * @brief Image-background implementation that stretches one image over a view.
 *
 * Unlike the color and gradient variants, this background delegates almost all
 * behavior to the image module: decode, scaling, and per-pixel blending are
 * handled there once the target bounds are known.
 */

/**
 * @brief Bind the borrowed image pointer stored in one parameter block.
 */
void egui_background_image_param_init(egui_background_image_param_t *param, egui_image_t *img)
{
    if (param == NULL)
    {
        return;
    }

    param->img = img;
}

/**
 * @brief Draw the configured image resized to the full local view region.
 *
 * The background layer does not preserve aspect ratio here. It intentionally
 * behaves like a "stretch to fit" background so the whole widget area is
 * covered without extra positioning rules.
 */
void egui_background_image_on_draw(egui_background_t *self, egui_canvas_t *canvas, egui_region_t *region, const void *param)
{
    EGUI_LOCAL_INIT(egui_background_image_t);
    const egui_background_image_param_t *img_param = param;
    EGUI_UNUSED(local);

    if (img_param->img != NULL)
    {
        egui_canvas_draw_image_resize(canvas, img_param->img, 0, 0, region->size.width, region->size.height);
    }
}

const egui_background_api_t egui_background_image_t_api_table = {
        .draw = egui_background_draw,
        .on_draw = egui_background_image_on_draw,
};

/**
 * @brief Initialize an image background and install the image-specific API table.
 */
void egui_background_image_init(egui_background_t *self)
{
    egui_background_init(self);
    self->api = &egui_background_image_t_api_table;
}

/**
 * @brief Initialize an image background and immediately bind one parameter table.
 */
void egui_background_image_init_with_params(egui_background_t *self, const egui_background_params_t *params)
{
    egui_background_image_init(self);
    egui_background_set_params(self, params);
}
