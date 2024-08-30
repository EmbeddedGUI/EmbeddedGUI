#ifndef _EGUI_BACKGROUND_H_
#define _EGUI_BACKGROUND_H_

#include "core/egui_canvas.h"
#include "core/egui_region.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_BACKGROUND_PARAM_INIT(_name, _normal_param, _pressed_param, _disabled_param)                                                                      \
    static const egui_background_params_t _name = {                                                                                                            \
            .normal_param = _normal_param,                                                                                                                     \
            .pressed_param = _pressed_param,                                                                                                                   \
            .disabled_param = _disabled_param,                                                                                                                 \
    }

typedef struct egui_view egui_view_t;

typedef struct egui_background egui_background_t;

typedef struct egui_background_api egui_background_api_t;
struct egui_background_api
{
    void (*draw)(egui_background_t *self, egui_view_t *view);
    void (*on_draw)(egui_background_t *self, egui_region_t *region, const void *param);
};

typedef struct egui_background_params egui_background_params_t;
struct egui_background_params
{
    const void *normal_param;   // parameters of the normal background
    const void *pressed_param;  // parameters of the pressed background
    const void *disabled_param; // parameters of the disabled background
};

struct egui_background
{
    const egui_background_params_t *params; // parameters of the background
    const egui_background_api_t *api;       // api of the view
};

void egui_background_set_params(egui_background_t *self, const egui_background_params_t *params);
void egui_background_draw(egui_background_t *self, egui_view_t *view);
void egui_background_on_draw(egui_background_t *self, egui_region_t *view, const void *params);
void egui_background_init(egui_background_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_BACKGROUND_H_ */
