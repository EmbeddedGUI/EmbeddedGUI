#ifndef _EGUI_VIEW_ARC_SLIDER_H_
#define _EGUI_VIEW_ARC_SLIDER_H_

#include "egui_view.h"
#include "core/egui_theme.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*egui_view_on_arc_value_changed_listener_t)(egui_view_t *self, uint8_t value);

typedef struct egui_view_arc_slider egui_view_arc_slider_t;
struct egui_view_arc_slider
{
    egui_view_t base;

    egui_view_on_arc_value_changed_listener_t on_value_changed;
    uint8_t value;
    int16_t start_angle;
    int16_t sweep_angle;
    egui_dim_t stroke_width;
    egui_color_t track_color;
    egui_color_t active_color;
    egui_color_t thumb_color;
    egui_dim_t thumb_radius;
    uint8_t is_dragging;
};

// ============== ArcSlider Params ==============
typedef struct egui_view_arc_slider_params egui_view_arc_slider_params_t;
struct egui_view_arc_slider_params
{
    egui_region_t region;
    uint8_t value;
};

#define EGUI_VIEW_ARC_SLIDER_PARAMS_INIT(_name, _x, _y, _w, _h, _val)                                                                                          \
    static const egui_view_arc_slider_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .value = (_val)}

void egui_view_arc_slider_apply_params(egui_view_t *self, const egui_view_arc_slider_params_t *params);
void egui_view_arc_slider_init_with_params(egui_view_t *self, const egui_view_arc_slider_params_t *params);

void egui_view_arc_slider_set_on_value_changed_listener(egui_view_t *self, egui_view_on_arc_value_changed_listener_t listener);
void egui_view_arc_slider_set_value(egui_view_t *self, uint8_t value);
uint8_t egui_view_arc_slider_get_value(egui_view_t *self);
void egui_view_arc_slider_on_draw(egui_view_t *self);
void egui_view_arc_slider_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_ARC_SLIDER_H_ */
