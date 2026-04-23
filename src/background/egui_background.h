#ifndef _EGUI_BACKGROUND_H_
#define _EGUI_BACKGROUND_H_

#include "canvas/egui_canvas.h"
#include "core/egui_region.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Build one reusable state table from normal, pressed, and disabled parameters.
 *
 * The parameter pointers are usually addresses of static ROM data owned by the
 * concrete background type.
 */
#define EGUI_BACKGROUND_PARAM_INIT(_name, _normal_param, _pressed_param, _disabled_param)                                                                      \
    static const egui_background_params_t _name = {                                                                                                            \
            .normal_param = _normal_param,                                                                                                                     \
            .pressed_param = _pressed_param,                                                                                                                   \
            .disabled_param = _disabled_param,                                                                                                                 \
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
/**
 * @brief Build one reusable state table that also includes a focused-state entry.
 */
#define EGUI_BACKGROUND_PARAM_INIT_WITH_FOCUS(_name, _normal_param, _pressed_param, _disabled_param, _focused_param)                                           \
    static const egui_background_params_t _name = {                                                                                                            \
            .normal_param = _normal_param,                                                                                                                     \
            .pressed_param = _pressed_param,                                                                                                                   \
            .disabled_param = _disabled_param,                                                                                                                 \
            .focused_param = _focused_param,                                                                                                                   \
    }
#endif

typedef struct egui_background_api egui_background_api_t;

/**
 * @brief Virtual API shared by every background implementation.
 *
 * `draw` handles state selection from the target view, while `on_draw`
 * receives the already-selected parameter block and performs the real drawing.
 */
struct egui_background_api
{
    void (*draw)(egui_background_t *self, egui_canvas_t *canvas, egui_view_t *view);
    void (*on_draw)(egui_background_t *self, egui_canvas_t *canvas, egui_region_t *region, const void *param);
};

typedef struct egui_background_params egui_background_params_t;

/**
 * @brief Table of optional parameter blocks keyed by view state.
 */
struct egui_background_params
{
    const void *normal_param;   // parameters of the normal background
    const void *pressed_param;  // parameters of the pressed background
    const void *disabled_param; // parameters of the disabled background
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    const void *focused_param; // parameters of the focused background
#endif
};

/**
 * @brief Base background object embedded by concrete background subtypes.
 */
struct egui_background
{
    const egui_background_params_t *params; // parameters of the background
    const egui_background_api_t *api;       // api of the view
};

/**
 * @brief Bind the borrowed state-parameter table used by this background.
 */
void egui_background_set_params(egui_background_t *self, const egui_background_params_t *params);

/**
 * @brief Select the active state parameters from the target view, then draw.
 */
void egui_background_draw(egui_background_t *self, egui_canvas_t *canvas, egui_view_t *view);

/**
 * @brief Low-level draw hook that receives the already-selected parameter block.
 */
void egui_background_on_draw(egui_background_t *self, egui_canvas_t *canvas, egui_region_t *view, const void *params);

/**
 * @brief Initialize the base background dispatcher with the default API table.
 */
void egui_background_init(egui_background_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_BACKGROUND_H_ */
