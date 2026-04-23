#ifndef _EGUI_VIEW_CHART_COMMON_H_
#define _EGUI_VIEW_CHART_COMMON_H_

#include "egui_view.h"
#include "font/egui_font_std.h"

/**
 * @brief Shared data structures and helpers for egui chart widgets.
 *
 * Axis-based charts reuse this file for series definitions, axis configuration,
 * viewport math, and the default axis/legend rendering helpers.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// ============== Legend Position ==============
typedef enum egui_chart_legend_pos
{
    /** Hide the legend entirely. */
    EGUI_CHART_LEGEND_NONE = 0,
    /** Place the legend above the plot area. */
    EGUI_CHART_LEGEND_TOP,
    /** Place the legend below the plot area. */
    EGUI_CHART_LEGEND_BOTTOM,
    /** Place the legend to the right of the plot area. */
    EGUI_CHART_LEGEND_RIGHT,
} egui_chart_legend_pos_t;

// ============== Data Point ==============
typedef struct egui_chart_point egui_chart_point_t;
/** One `(x, y)` sample used by line, scatter, and bar charts. */
struct egui_chart_point
{
    int16_t x;
    int16_t y;
};

// ============== Data Series (line/scatter/bar) ==============
typedef struct egui_chart_series egui_chart_series_t;
/** One colored data series. The `points` pointer is borrowed from caller storage. */
struct egui_chart_series
{
    const egui_chart_point_t *points;
    uint8_t point_count;
    egui_color_t color;
    const char *name; // legend name, NULL = not shown
};

// ============== Pie Slice ==============
typedef struct egui_chart_pie_slice egui_chart_pie_slice_t;
/** One slice in a pie chart. The `name` pointer is borrowed from caller storage. */
struct egui_chart_pie_slice
{
    uint16_t value;
    egui_color_t color;
    const char *name; // legend name, NULL = not shown
};

// ============== Axis Configuration ==============
typedef struct egui_chart_axis_config egui_chart_axis_config_t;
/** Axis configuration shared by line, scatter, and bar charts. */
struct egui_chart_axis_config
{
    int16_t min_value;
    int16_t max_value;
    int16_t tick_step;  // 0 = auto calculate from tick_count
    uint8_t tick_count; // used when tick_step == 0, default 5
    uint8_t show_grid : 1;
    uint8_t show_labels : 1;
    uint8_t show_axis : 1;
    uint8_t is_categorical : 1; // 1 = categorical axis (bar chart), labels centered on slots
    uint8_t reserved : 4;
    const char *title; // NULL = no title
};

// ============== Axis Base (shared by line/scatter/bar) ==============
typedef struct egui_chart_axis_base egui_chart_axis_base_t;
typedef struct egui_chart_text_ops egui_chart_text_ops_t;
/** Strategy hook used internally to draw either categorical or continuous X axes. */
typedef void (*egui_chart_draw_axis_x_fn)(egui_canvas_t *canvas, egui_chart_axis_base_t *ab, egui_region_t *plot_area, egui_dim_t font_h, int16_t view_x_min,
                                          int16_t view_x_max);
/** Optional legend hook that lets specialized chart widgets override the shared legend layout. */
typedef void (*egui_chart_draw_legend_series_fn)(egui_canvas_t *canvas, egui_chart_axis_base_t *ab, egui_region_t *region, egui_region_t *plot_area);
/** Shared runtime state used by axis-based charts. This is public mainly for custom chart subclasses. */
struct egui_chart_axis_base
{
    // series data
    const egui_chart_series_t *series;
    uint8_t series_count;

    // axis config
    egui_chart_axis_config_t axis_x;
    egui_chart_axis_config_t axis_y;

    // legend
    uint8_t legend_pos; // egui_chart_legend_pos_t

    // style
    egui_color_t bg_color;
    egui_color_t axis_color;
    egui_color_t grid_color;
    egui_color_t text_color;
    const egui_font_t *font;
    /* Internal text operation table selected by `egui_chart_axis_base_set_font`. */
    const egui_chart_text_ops_t *text_ops;
    egui_chart_draw_axis_x_fn draw_axis_x;
    egui_chart_draw_legend_series_fn draw_legend_series;

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    // zoom/viewport state
    uint8_t zoom_enabled : 1;
    uint8_t is_panning : 1;
    uint8_t is_pinching : 1;
    uint8_t reserved_zoom : 5;

    // current viewport (visible data range when zoomed)
    int16_t view_x_min;
    int16_t view_x_max;
    int16_t view_y_min;
    int16_t view_y_max;

    // pan tracking
    egui_dim_t pan_last_x;
    egui_dim_t pan_last_y;

    // pinch tracking
    int32_t pinch_start_dist;
    int32_t pinch_start_dx_abs;
    int32_t pinch_start_dy_abs;
    int16_t pinch_start_view_x_min;
    int16_t pinch_start_view_x_max;
    int16_t pinch_start_view_y_min;
    int16_t pinch_start_view_y_max;
#endif
};

// ============== Shared Utility Functions ==============

/** Initialize an axis-base state block with default colors, axes, and legend settings. */
void egui_chart_axis_base_init_defaults(egui_chart_axis_base_t *ab);
/** Switch the X axis between continuous and categorical drawing modes. */
void egui_chart_axis_base_set_axis_x_categorical(egui_chart_axis_base_t *ab, uint8_t is_categorical);
/** Set the font used by shared chart text helpers. Passing NULL enables the lightweight fallback text ops. */
void egui_chart_axis_base_set_font(egui_chart_axis_base_t *ab, const egui_font_t *font);

/** Convert a signed 16-bit integer to text without relying on `snprintf`. */
void egui_chart_int_to_str(int16_t value, char *buf, int buf_size);

/** Return the font height, or a small built-in fallback when `font` is NULL. */
egui_dim_t egui_chart_get_font_height(const egui_font_t *font);

/** Return the effective X viewport range, taking zoom state into account when enabled. */
void egui_chart_get_view_x(egui_chart_axis_base_t *ab, int16_t *out_min, int16_t *out_max);
/** Return the effective Y viewport range, taking zoom state into account when enabled. */
void egui_chart_get_view_y(egui_chart_axis_base_t *ab, int16_t *out_min, int16_t *out_max);

/** Map one data-space X value into plot-space pixels. */
egui_dim_t egui_chart_map_x(egui_chart_axis_base_t *ab, int16_t data_x, egui_dim_t plot_x, egui_dim_t plot_w);
/** Map one data-space Y value into plot-space pixels. */
egui_dim_t egui_chart_map_y(egui_chart_axis_base_t *ab, int16_t data_y, egui_dim_t plot_y, egui_dim_t plot_h);

/** Calculate the plot rectangle after reserving room for axes, labels, and legend. */
void egui_chart_calc_plot_area(egui_chart_axis_base_t *ab, egui_region_t *region, egui_region_t *plot_area);

/** Draw axes, ticks, grid lines, labels, and titles for one axis-based chart. */
void egui_chart_draw_axes(egui_canvas_t *canvas, egui_chart_axis_base_t *ab, egui_region_t *region, egui_region_t *plot_area);

/** Draw the shared legend layout for line, scatter, and bar series. */
void egui_chart_draw_legend_series(egui_canvas_t *canvas, egui_chart_axis_base_t *ab, egui_region_t *region, egui_region_t *plot_area);

// ============== Zoom Functions (multi-touch) ==============
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH

/** Clamp the zoom viewport so it stays inside the configured axis ranges. */
void egui_chart_clamp_viewport(egui_chart_axis_base_t *ab);
/** Return nonzero when the chart is currently zoomed away from the full configured range. */
int egui_chart_is_zoomed(egui_chart_axis_base_t *ab);
/** Zoom both axes in or out around the current viewport center. */
void egui_chart_apply_zoom(egui_chart_axis_base_t *ab, int zoom_in);
/** Zoom only the selected axes in or out. */
void egui_chart_apply_zoom_axis(egui_chart_axis_base_t *ab, int zoom_in, int axis_x, int axis_y);
/** Shared multi-touch handler used by axis-based chart widgets for pinch-zoom and pan. */
int egui_chart_axis_on_touch_event(egui_view_t *self, egui_chart_axis_base_t *ab, egui_motion_event_t *event);

#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CHART_COMMON_H_ */
