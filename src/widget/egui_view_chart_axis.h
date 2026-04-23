// File: src/widget/egui_view_chart_axis.h
#ifndef _EGUI_VIEW_CHART_AXIS_H_
#define _EGUI_VIEW_CHART_AXIS_H_

#include "egui_view_chart_common.h"

/**
 * @brief Shared axis-chart base widget used by line, scatter, and bar charts.
 *
 * This layer combines the reusable chart-axis state from `chart_common` with
 * the normal `egui_view_t` lifecycle so subclasses only need to implement the
 * data-specific drawing logic.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// ============== Extended API Table (with draw_data virtual) ==============
typedef struct egui_view_chart_axis_api egui_view_chart_axis_api_t;
/** Extended API table used by axis-based chart subclasses. */
struct egui_view_chart_axis_api
{
    egui_view_api_t base;
    /* Subclass hook that draws chart data inside the clipped plot area. */
    void (*draw_data)(egui_view_t *self, const egui_region_t *plot_area);
};

// ============== Axis Base View (inherits egui_view_t) ==============
typedef struct egui_view_chart_axis egui_view_chart_axis_t;
/** Base widget shared by line, scatter, and bar charts. */
struct egui_view_chart_axis
{
    egui_view_t base;
    /* Shared axis/legend/viewport state consumed by the chart common helpers. */
    egui_chart_axis_base_t ab;

    /* Optional subclass-provided clip slack metadata. */
    egui_dim_t clip_margin;
};

// ============== Axis Base Params ==============
typedef struct egui_view_chart_axis_params egui_view_chart_axis_params_t;
struct egui_view_chart_axis_params
{
    egui_region_t region;
};

/** Convenience macro for one static chart-axis parameter block. */
#define EGUI_VIEW_CHART_AXIS_PARAMS_INIT(_name, _x, _y, _w, _h) static const egui_view_chart_axis_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

// ============== Axis Base Lifecycle ==============

/** Initialize the reusable axis-based chart base widget. */
void egui_view_chart_axis_init(egui_view_t *self, egui_core_t *core);
/** Apply the chart region from one parameter block. */
void egui_view_chart_axis_apply_params(egui_view_t *self, const egui_view_chart_axis_params_t *params);

// ============== Axis Base Draw (calls draw_data virtual) ==============

/** Default draw hook for axis-based charts. It draws background, axes, clipped subclass data, and legend in one pass. */
void egui_view_chart_axis_on_draw(egui_view_t *self);

// ============== Axis Base Touch ==============

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
/** Shared touch handler for pan and pinch gestures on axis-based charts. */
int egui_view_chart_axis_on_touch_event(egui_view_t *self, egui_motion_event_t *event);
#endif

// ============== Unified Setters ==============

// data
/** Borrow the series array for this chart. The series and point buffers stay owned by the caller. */
void egui_view_chart_axis_set_series(egui_view_t *self, const egui_chart_series_t *series, uint8_t count);

// axis
/** Set the X axis range and tick step directly. On zoom-enabled builds this also resets the X viewport to the full range. */
void egui_view_chart_axis_set_axis_x(egui_view_t *self, int16_t min_val, int16_t max_val, int16_t tick_step);
/** Set the Y axis range and tick step directly. On zoom-enabled builds this also resets the Y viewport to the full range. */
void egui_view_chart_axis_set_axis_y(egui_view_t *self, int16_t min_val, int16_t max_val, int16_t tick_step);
/** Copy a full X-axis configuration block. On zoom-enabled builds this also resets the X viewport. */
void egui_view_chart_axis_set_axis_x_config(egui_view_t *self, const egui_chart_axis_config_t *config);
/** Copy a full Y-axis configuration block. On zoom-enabled builds this also resets the Y viewport. */
void egui_view_chart_axis_set_axis_y_config(egui_view_t *self, const egui_chart_axis_config_t *config);

// legend
/** Set the legend position, or disable the legend entirely with `EGUI_CHART_LEGEND_NONE`. */
void egui_view_chart_axis_set_legend_pos(egui_view_t *self, uint8_t pos);

// style
/** Set the background, axis, grid, and text colors used by the shared chart chrome. */
void egui_view_chart_axis_set_colors(egui_view_t *self, egui_color_t bg, egui_color_t axis, egui_color_t grid, egui_color_t text);
/** Set the font used by shared axis labels, titles, and legend text. */
void egui_view_chart_axis_set_font(egui_view_t *self, const egui_font_t *font);

// clip margin (subclass sets this for proper clip expansion)
/** Store a subclass-defined clip-margin hint alongside the shared axis base state. */
void egui_view_chart_axis_set_clip_margin(egui_view_t *self, egui_dim_t margin);

// ============== Unified Zoom API ==============

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
/** Enable or disable zoom support. Disabling it resets the viewport back to the configured full range. */
void egui_view_chart_axis_set_zoom_enabled(egui_view_t *self, uint8_t enabled);
/** Zoom both axes in. No effect when zoom support is disabled. */
void egui_view_chart_axis_zoom_in(egui_view_t *self);
/** Zoom both axes out. No effect when zoom support is disabled. */
void egui_view_chart_axis_zoom_out(egui_view_t *self);
/** Reset both zoom axes back to the configured full range. */
void egui_view_chart_axis_zoom_reset(egui_view_t *self);
/** Zoom only the X axis in. */
void egui_view_chart_axis_zoom_in_x(egui_view_t *self);
/** Zoom only the X axis out. */
void egui_view_chart_axis_zoom_out_x(egui_view_t *self);
/** Reset only the X viewport. */
void egui_view_chart_axis_zoom_reset_x(egui_view_t *self);
/** Zoom only the Y axis in. */
void egui_view_chart_axis_zoom_in_y(egui_view_t *self);
/** Zoom only the Y axis out. */
void egui_view_chart_axis_zoom_out_y(egui_view_t *self);
/** Reset only the Y viewport. */
void egui_view_chart_axis_zoom_reset_y(egui_view_t *self);
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CHART_AXIS_H_ */
