#ifndef _EGUI_VIEW_ACTIVITY_RING_H_
#define _EGUI_VIEW_ACTIVITY_RING_H_

#include "egui_view.h"
#include "core/egui_theme.h"

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

void egui_view_activity_ring_apply_params(egui_view_t *self, const egui_view_activity_ring_params_t *params);
void egui_view_activity_ring_init_with_params(egui_view_t *self, const egui_view_activity_ring_params_t *params);

void egui_view_activity_ring_set_value(egui_view_t *self, uint8_t ring_index, uint8_t value);
uint8_t egui_view_activity_ring_get_value(egui_view_t *self, uint8_t ring_index);
void egui_view_activity_ring_set_ring_count(egui_view_t *self, uint8_t count);
void egui_view_activity_ring_set_ring_color(egui_view_t *self, uint8_t ring_index, egui_color_t color);
void egui_view_activity_ring_set_ring_bg_color(egui_view_t *self, uint8_t ring_index, egui_color_t color);
void egui_view_activity_ring_set_stroke_width(egui_view_t *self, egui_dim_t stroke_width);
void egui_view_activity_ring_set_ring_gap(egui_view_t *self, egui_dim_t ring_gap);
void egui_view_activity_ring_set_start_angle(egui_view_t *self, int16_t start_angle);
void egui_view_activity_ring_set_show_round_cap(egui_view_t *self, uint8_t show);
void egui_view_activity_ring_on_draw(egui_view_t *self);
void egui_view_activity_ring_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_ACTIVITY_RING_H_ */
