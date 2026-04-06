#ifndef _EGUI_VIEW_LINEAR_VALUE_HELPER_H_
#define _EGUI_VIEW_LINEAR_VALUE_HELPER_H_

#include "egui_view.h"

typedef struct egui_view_linear_value_metrics
{
    egui_dim_t track_height;
    egui_dim_t track_y;
    egui_dim_t track_radius;
    egui_dim_t knob_radius;
    egui_dim_t knob_margin;
    egui_dim_t usable_width;
    egui_dim_t start_x;
    egui_dim_t center_y;
} egui_view_linear_value_metrics_t;

uint8_t egui_view_linear_value_get_metrics(const egui_region_t *region, uint8_t reserve_knob_margin, egui_view_linear_value_metrics_t *metrics);
egui_dim_t egui_view_linear_value_get_x(const egui_view_linear_value_metrics_t *metrics, uint8_t value);
egui_dim_t egui_view_linear_value_clamp_x(const egui_view_linear_value_metrics_t *metrics, egui_dim_t x);
uint8_t egui_view_linear_value_get_value_from_local_x(const egui_view_linear_value_metrics_t *metrics, egui_dim_t local_x);

#endif
