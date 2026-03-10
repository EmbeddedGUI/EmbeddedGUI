#ifndef _EGUI_VIEW_HEATMAP_CHART_H_
#define _EGUI_VIEW_HEATMAP_CHART_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_HEATMAP_CHART_MAX_VALUE_SETS 3
#define EGUI_VIEW_HEATMAP_CHART_MAX_ROWS 6
#define EGUI_VIEW_HEATMAP_CHART_MAX_COLS 6

typedef struct egui_view_heatmap_chart egui_view_heatmap_chart_t;
struct egui_view_heatmap_chart
{
    egui_view_t base;
    const uint8_t *value_sets[EGUI_VIEW_HEATMAP_CHART_MAX_VALUE_SETS];
    const char **row_labels;
    const char **col_labels;
    const egui_font_t *font;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t cold_color;
    egui_color_t hot_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    uint8_t rows;
    uint8_t cols;
    uint8_t value_set_count;
    uint8_t current_value_set;
    uint8_t show_values;
    uint8_t show_header;
    uint8_t show_axis_labels;
};

void egui_view_heatmap_chart_init(egui_view_t *self);
void egui_view_heatmap_chart_set_shape(egui_view_t *self, uint8_t rows, uint8_t cols);
void egui_view_heatmap_chart_set_axis_labels(egui_view_t *self, const char **row_labels, const char **col_labels);
void egui_view_heatmap_chart_set_value_set(egui_view_t *self, uint8_t set_index, const uint8_t *values);
void egui_view_heatmap_chart_set_current_value_set(egui_view_t *self, uint8_t set_index);
uint8_t egui_view_heatmap_chart_get_current_value_set(egui_view_t *self);
void egui_view_heatmap_chart_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_heatmap_chart_set_show_values(egui_view_t *self, uint8_t show_values);
void egui_view_heatmap_chart_set_show_header(egui_view_t *self, uint8_t show_header);
void egui_view_heatmap_chart_set_show_axis_labels(egui_view_t *self, uint8_t show_axis_labels);
void egui_view_heatmap_chart_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t border_color,
        egui_color_t cold_color,
        egui_color_t hot_color,
        egui_color_t text_color,
        egui_color_t muted_text_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_HEATMAP_CHART_H_ */
