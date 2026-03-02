#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_background_image.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

void egui_background_image_on_draw(egui_background_t *self, egui_region_t *region, const void *param)
{
    EGUI_LOCAL_INIT(egui_background_image_t);
    const egui_background_image_param_t *img_param = param;

    if (img_param->img != NULL)
    {
        egui_canvas_draw_image_resize(img_param->img, 0, 0, region->size.width, region->size.height);
    }
}

// name must be _type##_api_table, it will be used by EGUI_VIEW_DEFINE to init api.
const egui_background_api_t egui_background_image_t_api_table = {
        .draw = egui_background_draw,
        .on_draw = egui_background_image_on_draw,
};

void egui_background_image_init(egui_background_t *self)
{
    EGUI_LOCAL_INIT(egui_background_image_t);
    // call super init.
    egui_background_init(self);
    // update api.
    self->api = &egui_background_image_t_api_table;

    // init local data.
}

void egui_background_image_init_with_params(egui_background_t *self, const egui_background_params_t *params)
{
    egui_background_image_init(self);
    egui_background_set_params(self, params);
}
