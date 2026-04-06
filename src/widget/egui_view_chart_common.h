#ifndef _EGUI_VIEW_CHART_COMMON_H_
#define _EGUI_VIEW_CHART_COMMON_H_

#include "egui_view.h"
#include "font/egui_font_std.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// ============== Legend Position ==============
typedef enum egui_chart_legend_pos
{
    EGUI_CHART_LEGEND_NONE = 0,
    EGUI_CHART_LEGEND_TOP,
    EGUI_CHART_LEGEND_BOTTOM,
    EGUI_CHART_LEGEND_RIGHT,
} egui_chart_legend_pos_t;

// ============== Data Point ==============
typedef struct egui_chart_point egui_chart_point_t;
struct egui_chart_point
{
    int16_t x;
    int16_t y;
};

// ============== Data Series (line/scatter/bar) ==============
typedef struct egui_chart_series egui_chart_series_t;
struct egui_chart_series
{
    const egui_chart_point_t *points;
    uint8_t point_count;
    egui_color_t color;
    const char *name; // legend name, NULL = not shown
};

// ============== Pie Slice ==============
typedef struct egui_chart_pie_slice egui_chart_pie_slice_t;
struct egui_chart_pie_slice
{
    uint16_t value;
    egui_color_t color;
    const char *name; // legend name, NULL = not shown
};

// ============== Axis Configuration ==============
typedef struct egui_chart_axis_config egui_chart_axis_config_t;
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
typedef void (*egui_chart_draw_axis_x_fn)(egui_chart_axis_base_t *ab, egui_region_t *plot_area, egui_dim_t font_h, int16_t view_x_min, int16_t view_x_max);
typedef void (*egui_chart_draw_legend_series_fn)(egui_chart_axis_base_t *ab, egui_region_t *region, egui_region_t *plot_area);
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

// Initialize axis base with default values
void egui_chart_axis_base_init_defaults(egui_chart_axis_base_t *ab);
void egui_chart_axis_base_set_axis_x_categorical(egui_chart_axis_base_t *ab, uint8_t is_categorical);
void egui_chart_axis_base_set_font(egui_chart_axis_base_t *ab, const egui_font_t *font);

// Convert int16_t to string without snprintf (embedded-safe)
void egui_chart_int_to_str(int16_t value, char *buf, int buf_size);

// Get font height with fallback
egui_dim_t egui_chart_get_font_height(const egui_font_t *font);

// Get the effective viewport min/max (considers zoom)
void egui_chart_get_view_x(egui_chart_axis_base_t *ab, int16_t *out_min, int16_t *out_max);
void egui_chart_get_view_y(egui_chart_axis_base_t *ab, int16_t *out_min, int16_t *out_max);

// Map data coordinates to pixel coordinates
egui_dim_t egui_chart_map_x(egui_chart_axis_base_t *ab, int16_t data_x, egui_dim_t plot_x, egui_dim_t plot_w);
egui_dim_t egui_chart_map_y(egui_chart_axis_base_t *ab, int16_t data_y, egui_dim_t plot_y, egui_dim_t plot_h);

// Calculate plot area (the area where data is drawn, excluding axes/legend)
void egui_chart_calc_plot_area(egui_chart_axis_base_t *ab, egui_region_t *region, egui_region_t *plot_area);

// Draw axes (ticks, grid, labels)
void egui_chart_draw_axes(egui_chart_axis_base_t *ab, egui_region_t *region, egui_region_t *plot_area);

// Draw legend for series data (line/scatter/bar)
void egui_chart_draw_legend_series(egui_chart_axis_base_t *ab, egui_region_t *region, egui_region_t *plot_area);

// ============== Zoom Functions (multi-touch) ==============
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH

void egui_chart_clamp_viewport(egui_chart_axis_base_t *ab);
int egui_chart_is_zoomed(egui_chart_axis_base_t *ab);
void egui_chart_apply_zoom(egui_chart_axis_base_t *ab, int zoom_in);
void egui_chart_apply_zoom_axis(egui_chart_axis_base_t *ab, int zoom_in, int axis_x, int axis_y);
int egui_chart_axis_on_touch_event(egui_view_t *self, egui_chart_axis_base_t *ab, egui_motion_event_t *event);

#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CHART_COMMON_H_ */
