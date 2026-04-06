#ifndef _EGUI_VIEW_CHART_PIE_H_
#define _EGUI_VIEW_CHART_PIE_H_

#include "egui_view_chart_common.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// ============== Pie Chart Widget ==============
typedef struct egui_view_chart_pie egui_view_chart_pie_t;
typedef struct egui_view_chart_pie_text_ops egui_view_chart_pie_text_ops_t;
struct egui_view_chart_pie
{
    egui_view_t base;

    // pie data
    const egui_chart_pie_slice_t *pie_slices;
    uint8_t pie_slice_count;

    // legend
    uint8_t legend_pos; // egui_chart_legend_pos_t

    // style
    egui_color_t bg_color;
    egui_color_t text_color;
    const egui_font_t *font;
    const egui_view_chart_pie_text_ops_t *text_ops;
};

// ============== Pie Chart Params ==============
typedef struct egui_view_chart_pie_params egui_view_chart_pie_params_t;
struct egui_view_chart_pie_params
{
    egui_region_t region;
};

#define EGUI_VIEW_CHART_PIE_PARAMS_INIT(_name, _x, _y, _w, _h) static const egui_view_chart_pie_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

// ============== API ==============

// lifecycle
void egui_view_chart_pie_init(egui_view_t *self);
void egui_view_chart_pie_apply_params(egui_view_t *self, const egui_view_chart_pie_params_t *params);
void egui_view_chart_pie_init_with_params(egui_view_t *self, const egui_view_chart_pie_params_t *params);

// data
void egui_view_chart_pie_set_slices(egui_view_t *self, const egui_chart_pie_slice_t *slices, uint8_t count);

// legend
void egui_view_chart_pie_set_legend_pos(egui_view_t *self, uint8_t pos);

// style
void egui_view_chart_pie_set_colors(egui_view_t *self, egui_color_t bg, egui_color_t text);
void egui_view_chart_pie_set_font(egui_view_t *self, const egui_font_t *font);

// draw (internal, but exposed for potential override)
void egui_view_chart_pie_on_draw(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CHART_PIE_H_ */
