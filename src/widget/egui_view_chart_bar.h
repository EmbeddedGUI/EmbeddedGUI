#ifndef _EGUI_VIEW_CHART_BAR_H_
#define _EGUI_VIEW_CHART_BAR_H_

#include "egui_view_chart_axis.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// ============== Bar Chart Widget ==============
typedef struct egui_view_chart_bar egui_view_chart_bar_t;
struct egui_view_chart_bar
{
    egui_view_chart_axis_t axis_base;

    // bar chart specific
    uint8_t bar_gap; // default 2
};

// ============== Bar Chart Params ==============
typedef struct egui_view_chart_bar_params egui_view_chart_bar_params_t;
struct egui_view_chart_bar_params
{
    egui_region_t region;
};

#define EGUI_VIEW_CHART_BAR_PARAMS_INIT(_name, _x, _y, _w, _h) static const egui_view_chart_bar_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

// ============== API ==============

// lifecycle
void egui_view_chart_bar_init(egui_view_t *self);
void egui_view_chart_bar_apply_params(egui_view_t *self, const egui_view_chart_bar_params_t *params);
void egui_view_chart_bar_init_with_params(egui_view_t *self, const egui_view_chart_bar_params_t *params);

// style
void egui_view_chart_bar_set_bar_gap(egui_view_t *self, uint8_t gap);

// ============== Backward-compatible macros (delegate to axis base) ==============

#define egui_view_chart_bar_set_series        egui_view_chart_axis_set_series
#define egui_view_chart_bar_set_axis_x        egui_view_chart_axis_set_axis_x
#define egui_view_chart_bar_set_axis_y        egui_view_chart_axis_set_axis_y
#define egui_view_chart_bar_set_axis_x_config egui_view_chart_axis_set_axis_x_config
#define egui_view_chart_bar_set_axis_y_config egui_view_chart_axis_set_axis_y_config
#define egui_view_chart_bar_set_legend_pos    egui_view_chart_axis_set_legend_pos
#define egui_view_chart_bar_set_colors        egui_view_chart_axis_set_colors
#define egui_view_chart_bar_set_font          egui_view_chart_axis_set_font

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
#define egui_view_chart_bar_set_zoom_enabled egui_view_chart_axis_set_zoom_enabled
#define egui_view_chart_bar_zoom_in          egui_view_chart_axis_zoom_in
#define egui_view_chart_bar_zoom_out         egui_view_chart_axis_zoom_out
#define egui_view_chart_bar_zoom_reset       egui_view_chart_axis_zoom_reset
#define egui_view_chart_bar_zoom_in_x        egui_view_chart_axis_zoom_in_x
#define egui_view_chart_bar_zoom_out_x       egui_view_chart_axis_zoom_out_x
#define egui_view_chart_bar_zoom_reset_x     egui_view_chart_axis_zoom_reset_x
#define egui_view_chart_bar_zoom_in_y        egui_view_chart_axis_zoom_in_y
#define egui_view_chart_bar_zoom_out_y       egui_view_chart_axis_zoom_out_y
#define egui_view_chart_bar_zoom_reset_y     egui_view_chart_axis_zoom_reset_y
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CHART_BAR_H_ */
