#ifndef _EGUI_VIEW_CHART_PIE_H_
#define _EGUI_VIEW_CHART_PIE_H_

#include "egui_view_chart_common.h"

/**
 * @brief Standalone pie-chart widget with optional legend and label rendering.
 *
 * The widget borrows a caller-owned slice array, computes its own circle
 * geometry from the available view rectangle, and can reserve space for a
 * simple legend on the top, bottom, or right side.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// ============== Pie Chart Widget ==============
typedef struct egui_view_chart_pie egui_view_chart_pie_t;
typedef struct egui_view_chart_pie_text_ops egui_view_chart_pie_text_ops_t;
/** Pie-chart widget. The slice array is borrowed from caller storage. */
struct egui_view_chart_pie
{
    egui_view_t base;

    /* Borrowed slice array supplied by the caller. */
    const egui_chart_pie_slice_t *pie_slices;
    /* Number of entries in `pie_slices`. */
    uint8_t pie_slice_count;

    /* Legend placement using `egui_chart_legend_pos_t`. */
    uint8_t legend_pos;

    /* Background color drawn behind the pie and legend. */
    egui_color_t bg_color;
    /* Text color used by legend labels. */
    egui_color_t text_color;
    /* Optional custom font. NULL enables the lightweight fallback path. */
    const egui_font_t *font;
    /* Internal text backend selected from `font`. */
    const egui_view_chart_pie_text_ops_t *text_ops;
};

// ============== Pie Chart Params ==============
typedef struct egui_view_chart_pie_params egui_view_chart_pie_params_t;
struct egui_view_chart_pie_params
{
    egui_region_t region;
};

/** Convenience macro for one static pie-chart parameter block. */
#define EGUI_VIEW_CHART_PIE_PARAMS_INIT(_name, _x, _y, _w, _h) static const egui_view_chart_pie_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

// ============== API ==============

// lifecycle
/** Initialize a pie chart with no slices and legend hidden by default. */
void egui_view_chart_pie_init(egui_view_t *self, egui_core_t *core);
/** Apply the chart region from one parameter block. */
void egui_view_chart_pie_apply_params(egui_view_t *self, const egui_view_chart_pie_params_t *params);
/** Initialize a pie chart and immediately apply its parameter block. */
void egui_view_chart_pie_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_chart_pie_params_t *params);

// data
/** Borrow the slice array from caller storage. */
void egui_view_chart_pie_set_slices(egui_view_t *self, const egui_chart_pie_slice_t *slices, uint8_t count);

// legend
/** Set the legend position, or disable it with `EGUI_CHART_LEGEND_NONE`. */
void egui_view_chart_pie_set_legend_pos(egui_view_t *self, uint8_t pos);

// style
/** Set the background and label text colors used by the pie chart. */
void egui_view_chart_pie_set_colors(egui_view_t *self, egui_color_t bg, egui_color_t text);
/** Set the font used for pie labels and legend text. Passing NULL enables the lightweight fallback text ops. */
void egui_view_chart_pie_set_font(egui_view_t *self, const egui_font_t *font);

// draw (internal, but exposed for potential override)
/** Default draw hook used by the pie-chart API table to render background, slices, and legend. */
void egui_view_chart_pie_on_draw(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CHART_PIE_H_ */
