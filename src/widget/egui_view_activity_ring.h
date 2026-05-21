#ifndef _EGUI_VIEW_ACTIVITY_RING_H_
#define _EGUI_VIEW_ACTIVITY_RING_H_

#include "egui_view.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_ACTIVITY_RING_MAX_RINGS 3

typedef struct egui_view_activity_ring egui_view_activity_ring_t;
struct egui_view_activity_ring
{
    egui_view_t base;

    uint8_t ring_count;
    uint8_t values[EGUI_VIEW_ACTIVITY_RING_MAX_RINGS];
    egui_dim_t ring_gap;
    egui_dim_t stroke_width;
    int16_t start_angle;
    egui_color_t ring_colors[EGUI_VIEW_ACTIVITY_RING_MAX_RINGS];
    egui_color_t ring_bg_colors[EGUI_VIEW_ACTIVITY_RING_MAX_RINGS];
    uint8_t show_round_cap;
};

// ============== Activity Ring Params ==============
typedef struct egui_view_activity_ring_params egui_view_activity_ring_params_t;
struct egui_view_activity_ring_params
{
    egui_region_t region;
};

#define EGUI_VIEW_ACTIVITY_RING_PARAMS_INIT(_name, _x, _y, _w, _h)                                                                                             \
    static const egui_view_activity_ring_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

/** Apply an activity ring parameter block after initialization. */
void egui_view_activity_ring_apply_params(egui_view_t *self, const egui_view_activity_ring_params_t *params);
/** Initialize an activity ring and immediately apply its parameter block. */
void egui_view_activity_ring_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_activity_ring_params_t *params);

/** Set one ring value as a percentage in the 0-100 range. */
void egui_view_activity_ring_set_value(egui_view_t *self, uint8_t ring_index, uint8_t value);
/** Read the stored percentage for one ring slot. Invalid indexes return 0. */
uint8_t egui_view_activity_ring_get_value(egui_view_t *self, uint8_t ring_index);
/** Choose how many concentric rings are drawn. The count is clamped to the widget maximum. */
void egui_view_activity_ring_set_ring_count(egui_view_t *self, uint8_t count);
/** Return how many concentric rings are drawn. */
uint8_t egui_view_activity_ring_get_ring_count(egui_view_t *self);
/** Set the foreground color for one ring slot. */
void egui_view_activity_ring_set_ring_color(egui_view_t *self, uint8_t ring_index, egui_color_t color);
/** Return the foreground color for one ring slot. Invalid indexes return 0. */
egui_color_t egui_view_activity_ring_get_ring_color(egui_view_t *self, uint8_t ring_index);
/** Set the background track color for one ring slot. */
void egui_view_activity_ring_set_ring_bg_color(egui_view_t *self, uint8_t ring_index, egui_color_t color);
/** Return the background track color for one ring slot. Invalid indexes return 0. */
egui_color_t egui_view_activity_ring_get_ring_bg_color(egui_view_t *self, uint8_t ring_index);
/** Set the stroke width shared by every visible ring. */
void egui_view_activity_ring_set_stroke_width(egui_view_t *self, egui_dim_t stroke_width);
/** Return the stroke width shared by every visible ring. */
egui_dim_t egui_view_activity_ring_get_stroke_width(egui_view_t *self);
/** Set the spacing between neighboring rings. */
void egui_view_activity_ring_set_ring_gap(egui_view_t *self, egui_dim_t ring_gap);
/** Return the spacing between neighboring rings. */
egui_dim_t egui_view_activity_ring_get_ring_gap(egui_view_t *self);
/** Rotate the start angle used by every ring. */
void egui_view_activity_ring_set_start_angle(egui_view_t *self, int16_t start_angle);
/** Return the start angle used by every ring. */
int16_t egui_view_activity_ring_get_start_angle(egui_view_t *self);
/** Toggle rounded caps on the active arc ends. */
void egui_view_activity_ring_set_show_round_cap(egui_view_t *self, uint8_t show);
/** Return whether rounded caps are enabled on the active arc ends. */
uint8_t egui_view_activity_ring_get_show_round_cap(egui_view_t *self);
/** Default draw hook used by the activity ring API table. */
void egui_view_activity_ring_on_draw(egui_view_t *self);
/** Initialize a multi-ring activity indicator with themed defaults. */
void egui_view_activity_ring_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_ACTIVITY_RING_H_ */
