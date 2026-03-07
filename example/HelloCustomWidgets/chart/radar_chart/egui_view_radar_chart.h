#ifndef _EGUI_VIEW_RADAR_CHART_H_
#define _EGUI_VIEW_RADAR_CHART_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_RADAR_CHART_MAX_AXES 8
#define EGUI_VIEW_RADAR_CHART_MAX_VALUE_SETS 3

typedef struct egui_view_radar_chart egui_view_radar_chart_t;
struct egui_view_radar_chart
{
    egui_view_t base;
    const char **axis_labels;
    const uint8_t *value_sets[EGUI_VIEW_RADAR_CHART_MAX_VALUE_SETS];
    const uint8_t *compare_values;
    const egui_font_t *font;
    egui_color_t surface_color;
    egui_color_t primary_fill_color;
    egui_color_t primary_line_color;
    egui_color_t secondary_fill_color;
    egui_color_t secondary_line_color;
    egui_color_t grid_color;
    egui_color_t axis_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    uint8_t axis_count;
    uint8_t value_set_count;
    uint8_t current_value_set;
    uint8_t grid_levels;
    uint8_t show_compare;
    uint8_t show_axis_labels;
};

void egui_view_radar_chart_init(egui_view_t *self);
void egui_view_radar_chart_set_axis_labels(egui_view_t *self, const char **axis_labels, uint8_t axis_count);
void egui_view_radar_chart_set_value_set(egui_view_t *self, uint8_t set_index, const uint8_t *values);
void egui_view_radar_chart_set_current_value_set(egui_view_t *self, uint8_t set_index);
uint8_t egui_view_radar_chart_get_current_value_set(egui_view_t *self);
void egui_view_radar_chart_set_compare_values(egui_view_t *self, const uint8_t *values);
void egui_view_radar_chart_set_show_compare(egui_view_t *self, uint8_t show_compare);
void egui_view_radar_chart_set_grid_levels(egui_view_t *self, uint8_t grid_levels);
void egui_view_radar_chart_set_show_axis_labels(egui_view_t *self, uint8_t show_axis_labels);
void egui_view_radar_chart_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_radar_chart_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t primary_fill_color,
        egui_color_t primary_line_color,
        egui_color_t secondary_fill_color,
        egui_color_t secondary_line_color,
        egui_color_t grid_color,
        egui_color_t text_color,
        egui_color_t muted_text_color);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_RADAR_CHART_H_ */
