#ifndef _EGUI_VIEW_CHART_SCATTER_H_
#define _EGUI_VIEW_CHART_SCATTER_H_

#include "egui_view_chart_axis.h"

/**
 * @brief Axis-based scatter chart widget with circular point markers.
 *
 * Unlike the line chart, points are rendered independently and are not joined
 *
 * by connecting segments.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// ============== Scatter Chart Widget ==============
typedef struct egui_view_chart_scatter egui_view_chart_scatter_t;
/** Scatter-chart widget built on top of the shared axis base. */
struct egui_view_chart_scatter
{
    egui_view_chart_axis_t axis_base;

    /* Marker radius used when drawing each scatter point. */
    uint8_t point_radius;
};

// ============== Scatter Chart Params ==============
typedef struct egui_view_chart_scatter_params egui_view_chart_scatter_params_t;
struct egui_view_chart_scatter_params
{
    egui_region_t region;
};

/** Convenience macro for one static scatter-chart parameter block. */
#define EGUI_VIEW_CHART_SCATTER_PARAMS_INIT(_name, _x, _y, _w, _h)                                                                                             \
    static const egui_view_chart_scatter_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

// ============== API ==============

// lifecycle
/** Initialize a scatter chart with shared axis defaults and point radius `3`. */
void egui_view_chart_scatter_init(egui_view_t *self, egui_core_t *core);
/** Apply the chart region from one parameter block. */
void egui_view_chart_scatter_apply_params(egui_view_t *self, const egui_view_chart_scatter_params_t *params);
/** Initialize a scatter chart and immediately apply its parameter block. */
void egui_view_chart_scatter_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_chart_scatter_params_t *params);

// scatter-specific
/** Set the radius used when drawing each scatter point. */
void egui_view_chart_scatter_set_point_radius(egui_view_t *self, uint8_t radius);

// ============== Backward-compatible macros (delegate to axis base) ==============

#define egui_view_chart_scatter_set_series        egui_view_chart_axis_set_series
#define egui_view_chart_scatter_set_axis_x        egui_view_chart_axis_set_axis_x
#define egui_view_chart_scatter_set_axis_y        egui_view_chart_axis_set_axis_y
#define egui_view_chart_scatter_set_axis_x_config egui_view_chart_axis_set_axis_x_config
#define egui_view_chart_scatter_set_axis_y_config egui_view_chart_axis_set_axis_y_config
#define egui_view_chart_scatter_set_legend_pos    egui_view_chart_axis_set_legend_pos
#define egui_view_chart_scatter_set_colors        egui_view_chart_axis_set_colors
#define egui_view_chart_scatter_set_font          egui_view_chart_axis_set_font

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
#define egui_view_chart_scatter_set_zoom_enabled egui_view_chart_axis_set_zoom_enabled
#define egui_view_chart_scatter_zoom_in          egui_view_chart_axis_zoom_in
#define egui_view_chart_scatter_zoom_out         egui_view_chart_axis_zoom_out
#define egui_view_chart_scatter_zoom_reset       egui_view_chart_axis_zoom_reset
#define egui_view_chart_scatter_zoom_in_x        egui_view_chart_axis_zoom_in_x
#define egui_view_chart_scatter_zoom_out_x       egui_view_chart_axis_zoom_out_x
#define egui_view_chart_scatter_zoom_reset_x     egui_view_chart_axis_zoom_reset_x
#define egui_view_chart_scatter_zoom_in_y        egui_view_chart_axis_zoom_in_y
#define egui_view_chart_scatter_zoom_out_y       egui_view_chart_axis_zoom_out_y
#define egui_view_chart_scatter_zoom_reset_y     egui_view_chart_axis_zoom_reset_y
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CHART_SCATTER_H_ */
