#ifndef _EGUI_VIEW_SPINNER_H_
#define _EGUI_VIEW_SPINNER_H_

#include "egui_view.h"
#include "core/egui_timer.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_spinner egui_view_spinner_t;
struct egui_view_spinner
{
    egui_view_t base;

    int16_t arc_length;
    int16_t rotation_angle;
    egui_dim_t stroke_width;
    egui_color_t color;
    uint8_t is_spinning;
    egui_timer_t spin_timer;
};

// ============== Spinner Params ==============
typedef struct egui_view_spinner_params egui_view_spinner_params_t;
struct egui_view_spinner_params
{
    egui_region_t region;
};

#define EGUI_VIEW_SPINNER_PARAMS_INIT(_name, _x, _y, _w, _h) static const egui_view_spinner_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

void egui_view_spinner_apply_params(egui_view_t *self, const egui_view_spinner_params_t *params);
void egui_view_spinner_init_with_params(egui_view_t *self, const egui_view_spinner_params_t *params);

void egui_view_spinner_start(egui_view_t *self);
void egui_view_spinner_stop(egui_view_t *self);
void egui_view_spinner_set_color(egui_view_t *self, egui_color_t color);
void egui_view_spinner_on_draw(egui_view_t *self);
void egui_view_spinner_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SPINNER_H_ */
