// File: src/widget/egui_view_chart_axis.h
#ifndef _EGUI_VIEW_CHART_AXIS_H_
#define _EGUI_VIEW_CHART_AXIS_H_

#include "egui_view_chart_common.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// ============== Extended API Table (with draw_data virtual) ==============
typedef struct egui_view_chart_axis_api egui_view_chart_axis_api_t;
struct egui_view_chart_axis_api
{
    egui_view_api_t base;
    void (*draw_data)(egui_view_t *self, const egui_region_t *plot_area);
};

// ============== Axis Base View (inherits egui_view_t) ==============
typedef struct egui_view_chart_axis egui_view_chart_axis_t;
struct egui_view_chart_axis
{
    egui_view_t base;
    egui_chart_axis_base_t ab;

    // clip expansion for subclass data drawing
    egui_dim_t clip_margin;
};

// ============== Axis Base Params ==============
typedef struct egui_view_chart_axis_params egui_view_chart_axis_params_t;
struct egui_view_chart_axis_params
{
    egui_region_t region;
};

#define EGUI_VIEW_CHART_AXIS_PARAMS_INIT(_name, _x, _y, _w, _h) static const egui_view_chart_axis_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

// ============== Axis Base Lifecycle ==============

void egui_view_chart_axis_init(egui_view_t *self);
void egui_view_chart_axis_apply_params(egui_view_t *self, const egui_view_chart_axis_params_t *params);

// ============== Axis Base Draw (calls draw_data virtual) ==============

void egui_view_chart_axis_on_draw(egui_view_t *self);

// ============== Axis Base Touch ==============

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
int egui_view_chart_axis_on_touch_event(egui_view_t *self, egui_motion_event_t *event);
#endif

// ============== Unified Setters ==============

// data
void egui_view_chart_axis_set_series(egui_view_t *self, const egui_chart_series_t *series, uint8_t count);

// axis
void egui_view_chart_axis_set_axis_x(egui_view_t *self, int16_t min_val, int16_t max_val, int16_t tick_step);
void egui_view_chart_axis_set_axis_y(egui_view_t *self, int16_t min_val, int16_t max_val, int16_t tick_step);
void egui_view_chart_axis_set_axis_x_config(egui_view_t *self, const egui_chart_axis_config_t *config);
void egui_view_chart_axis_set_axis_y_config(egui_view_t *self, const egui_chart_axis_config_t *config);

// legend
void egui_view_chart_axis_set_legend_pos(egui_view_t *self, uint8_t pos);

// style
void egui_view_chart_axis_set_colors(egui_view_t *self, egui_color_t bg, egui_color_t axis, egui_color_t grid, egui_color_t text);
void egui_view_chart_axis_set_font(egui_view_t *self, const egui_font_t *font);

// clip margin (subclass sets this for proper clip expansion)
void egui_view_chart_axis_set_clip_margin(egui_view_t *self, egui_dim_t margin);

// ============== Unified Zoom API ==============

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
void egui_view_chart_axis_set_zoom_enabled(egui_view_t *self, uint8_t enabled);
void egui_view_chart_axis_zoom_in(egui_view_t *self);
void egui_view_chart_axis_zoom_out(egui_view_t *self);
void egui_view_chart_axis_zoom_reset(egui_view_t *self);
void egui_view_chart_axis_zoom_in_x(egui_view_t *self);
void egui_view_chart_axis_zoom_out_x(egui_view_t *self);
void egui_view_chart_axis_zoom_reset_x(egui_view_t *self);
void egui_view_chart_axis_zoom_in_y(egui_view_t *self);
void egui_view_chart_axis_zoom_out_y(egui_view_t *self);
void egui_view_chart_axis_zoom_reset_y(egui_view_t *self);
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CHART_AXIS_H_ */
