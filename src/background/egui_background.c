#include <stdio.h>
#include <assert.h>

#include "egui_background.h"
#include "widget/egui_view.h"

/**
 * @file egui_background.c
 * @brief Base background dispatcher shared by all background implementations.
 *
 * A background object does not draw pixels by itself. Its job is to:
 * 1. hold one table of state-specific parameter blocks,
 * 2. pick the block that matches the target view state,
 * 3. forward the work to the concrete background subtype.
 */

/**
 * @brief Bind the state-parameter table used by this background instance.
 *
 * The pointer is borrowed rather than copied, which keeps setup cheap and
 * makes ROM-friendly static parameter tables easy to reuse across many views.
 */
void egui_background_set_params(egui_background_t *self, const egui_background_params_t *params)
{
    self->params = params;
}

/**
 * @brief Select the active state parameters, then dispatch to the subtype.
 *
 * Reading tip:
 * - Normal state is the fallback.
 * - Disabled overrides everything else when a dedicated disabled block exists.
 * - Pressed wins over focused, because active touch feedback is usually the
 *   strongest visual state.
 * - The selected parameter block is then passed to `on_draw`, so subclasses do
 *   not need to repeat the view-state decision tree.
 */
void egui_background_draw(egui_background_t *self, egui_canvas_t *canvas, egui_view_t *view)
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
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    else if (view->is_focused)
    {
        if (params->focused_param != NULL)
        {
            sel_param = params->focused_param;
        }
    }
#endif

    self->api->on_draw(self, canvas, &view->region, sel_param);
}

/**
 * @brief Default low-level draw hook for the abstract base type.
 *
 * Concrete background implementations replace this callback with real drawing
 * logic. The base version is intentionally empty so `egui_background_init`
 * yields a safe no-op object.
 */
void egui_background_on_draw(egui_background_t *self, egui_canvas_t *canvas, egui_region_t *region, const void *param)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(canvas);
    EGUI_UNUSED(region);
    EGUI_UNUSED(param);
}

const egui_background_api_t egui_background_t_api_table = {
        .draw = egui_background_draw,
        .on_draw = egui_background_on_draw,
};

/**
 * @brief Initialize the base background object with the dispatcher API table.
 *
 * Subclasses usually call this first, then swap `self->api` to their own
 * table while reusing the same state-selection logic.
 */
void egui_background_init(egui_background_t *self)
{
    self->api = &egui_background_t_api_table;
}
