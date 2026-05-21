#ifndef _EGUI_VIEW_ARC_SLIDER_H_
#define _EGUI_VIEW_ARC_SLIDER_H_

#include "egui_view.h"

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

/** Apply an arc slider parameter block after initialization. */
void egui_view_arc_slider_apply_params(egui_view_t *self, const egui_view_arc_slider_params_t *params);
/** Initialize an arc slider and immediately apply its parameter block. */
void egui_view_arc_slider_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_arc_slider_params_t *params);

/** Register the callback fired whenever the slider value changes. */
void egui_view_arc_slider_set_on_value_changed_listener(egui_view_t *self, egui_view_on_arc_value_changed_listener_t listener);
/** Return the callback fired whenever the slider value changes. */
egui_view_on_arc_value_changed_listener_t egui_view_arc_slider_get_on_value_changed_listener(egui_view_t *self);
/** Set the slider value in the 0-100 range and fire the listener on real changes. */
void egui_view_arc_slider_set_value(egui_view_t *self, uint8_t value);
/** Read the current slider percentage. */
uint8_t egui_view_arc_slider_get_value(egui_view_t *self);
/** Return non-zero when the arc slider is currently being dragged. */
int egui_view_arc_slider_get_is_dragging(egui_view_t *self);
/** Return the background track color. */
egui_color_t egui_view_arc_slider_get_track_color(egui_view_t *self);
/** Return the filled progress arc color. */
egui_color_t egui_view_arc_slider_get_active_color(egui_view_t *self);
/** Return the thumb circle color. */
egui_color_t egui_view_arc_slider_get_thumb_color(egui_view_t *self);
/** Return the arc stroke width in pixels. */
egui_dim_t egui_view_arc_slider_get_stroke_width(egui_view_t *self);
/** Return the thumb circle radius in pixels. */
egui_dim_t egui_view_arc_slider_get_thumb_radius(egui_view_t *self);
/** Return the start angle of the arc in degrees. */
int16_t egui_view_arc_slider_get_start_angle(egui_view_t *self);
/** Return the sweep angle of the arc in degrees. */
int16_t egui_view_arc_slider_get_sweep_angle(egui_view_t *self);
/** Default draw hook used by the arc slider API table. */
void egui_view_arc_slider_on_draw(egui_view_t *self);
/** Initialize an interactive arc slider that updates continuously while dragged. */
void egui_view_arc_slider_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_ARC_SLIDER_H_ */
