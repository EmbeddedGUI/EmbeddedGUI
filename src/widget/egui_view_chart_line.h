#ifndef _EGUI_VIEW_CHART_LINE_H_
#define _EGUI_VIEW_CHART_LINE_H_

#include "egui_view_chart_axis.h"

/**
 * @brief Axis-based polyline chart widget with optional point markers.
 *
 * Series data is rendered as connected line segments, and each point can also
 *
 * draw a circular marker on top of the stroke.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// ============== Line Chart Widget ==============
typedef struct egui_view_chart_line egui_view_chart_line_t;
/** Line-chart widget built on top of the shared axis base. */
struct egui_view_chart_line
{
    egui_view_chart_axis_t axis_base;

    /* Stroke width used when drawing the polyline. */
    uint8_t line_width;
    /* Marker radius used when drawing each data point. */
    uint8_t point_radius;
};

// ============== Line Chart Params ==============
typedef struct egui_view_chart_line_params egui_view_chart_line_params_t;
struct egui_view_chart_line_params
{
    egui_region_t region;
};

/** Convenience macro for one static line-chart parameter block. */
#define EGUI_VIEW_CHART_LINE_PARAMS_INIT(_name, _x, _y, _w, _h) static const egui_view_chart_line_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

// ============== API ==============

// lifecycle
/** Initialize a line chart with shared axis defaults, line width `2`, and point radius `3`. */
void egui_view_chart_line_init(egui_view_t *self, egui_core_t *core);
/** Apply the chart region from one parameter block. */
void egui_view_chart_line_apply_params(egui_view_t *self, const egui_view_chart_line_params_t *params);
/** Initialize a line chart and immediately apply its parameter block. */
void egui_view_chart_line_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_chart_line_params_t *params);

// line chart specific setters
/** Set the stroke width used for line segments. */
void egui_view_chart_line_set_line_width(egui_view_t *self, uint8_t width);
/** Return the stroke width used for line segments. */
uint8_t egui_view_chart_line_get_line_width(egui_view_t *self);
/** Set the radius used when drawing each data point marker. */
void egui_view_chart_line_set_point_radius(egui_view_t *self, uint8_t radius);
/** Return the radius used when drawing each data point marker. */
uint8_t egui_view_chart_line_get_point_radius(egui_view_t *self);

// ============== Backward-compatible macros (delegate to axis base) ==============

#define egui_view_chart_line_set_series        egui_view_chart_axis_set_series
#define egui_view_chart_line_set_axis_x        egui_view_chart_axis_set_axis_x
#define egui_view_chart_line_set_axis_y        egui_view_chart_axis_set_axis_y
#define egui_view_chart_line_set_axis_x_config egui_view_chart_axis_set_axis_x_config
#define egui_view_chart_line_set_axis_y_config egui_view_chart_axis_set_axis_y_config
#define egui_view_chart_line_set_legend_pos    egui_view_chart_axis_set_legend_pos
#define egui_view_chart_line_set_colors        egui_view_chart_axis_set_colors
#define egui_view_chart_line_set_font          egui_view_chart_axis_set_font

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
#define egui_view_chart_line_set_zoom_enabled egui_view_chart_axis_set_zoom_enabled
#define egui_view_chart_line_zoom_in          egui_view_chart_axis_zoom_in
#define egui_view_chart_line_zoom_out         egui_view_chart_axis_zoom_out
#define egui_view_chart_line_zoom_reset       egui_view_chart_axis_zoom_reset
#define egui_view_chart_line_zoom_in_x        egui_view_chart_axis_zoom_in_x
#define egui_view_chart_line_zoom_out_x       egui_view_chart_axis_zoom_out_x
#define egui_view_chart_line_zoom_reset_x     egui_view_chart_axis_zoom_reset_x
#define egui_view_chart_line_zoom_in_y        egui_view_chart_axis_zoom_in_y
#define egui_view_chart_line_zoom_out_y       egui_view_chart_axis_zoom_out_y
#define egui_view_chart_line_zoom_reset_y     egui_view_chart_axis_zoom_reset_y
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CHART_LINE_H_ */
