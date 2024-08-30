#include <stdio.h>
#include <assert.h>

#include "egui_background.h"
#include "widget/egui_view.h"

void egui_background_set_params(egui_background_t *self, const egui_background_params_t *params)
{
    self->params = params;
}

void egui_background_draw(egui_background_t *self, egui_view_t *view)
{
    const egui_background_params_t *params = self->params;
    const void *sel_param = NULL;
    if (params == NULL)
    {
        return;
    }

    sel_param = params->normal_param;
    if (!view->is_enable)
    {
        if (params->disabled_param != NULL)
        {
            sel_param = params->disabled_param;
        }
    }
    else if (view->is_pressed)
    {
        if (params->pressed_param != NULL)
        {
            sel_param = params->pressed_param;
        }
    }

    self->api->on_draw(self, &view->region, sel_param);
}

void egui_background_on_draw(egui_background_t *self, egui_region_t *region, const void *param)
{
}

const egui_background_api_t egui_background_t_api_table = {
        .draw = egui_background_draw,
        .on_draw = egui_background_on_draw,
};

void egui_background_init(egui_background_t *self)
{
    // init api
    self->api = &egui_background_t_api_table;
}
