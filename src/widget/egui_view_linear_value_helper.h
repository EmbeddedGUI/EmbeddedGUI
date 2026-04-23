#ifndef _EGUI_VIEW_LINEAR_VALUE_HELPER_H_
#define _EGUI_VIEW_LINEAR_VALUE_HELPER_H_

#include "egui_view.h"

/**
 * @brief Shared geometry snapshot for horizontal value widgets.
 *
 * Slider-like widgets use this helper struct to agree on track thickness,
 * center line, knob radius, and the usable value span inside one region.
 * `usable_width` starts after any reserved knob margin so value conversion can
 * stay identical across sliders and progress bars.
 */
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

/** Compute track and knob geometry for slider-like widgets. `reserve_knob_margin` keeps the thumb inside the widget bounds. */
uint8_t egui_view_linear_value_get_metrics(const egui_region_t *region, uint8_t reserve_knob_margin, egui_view_linear_value_metrics_t *metrics);
/** Convert a `0..100` value into an absolute x position inside one metric set. */
egui_dim_t egui_view_linear_value_get_x(const egui_view_linear_value_metrics_t *metrics, uint8_t value);
/** Clamp an absolute x position so a knob stays within the usable track span. */
egui_dim_t egui_view_linear_value_clamp_x(const egui_view_linear_value_metrics_t *metrics, egui_dim_t x);
/** Convert a widget-local x coordinate back into a `0..100` value using the stored knob margin. */
uint8_t egui_view_linear_value_get_value_from_local_x(const egui_view_linear_value_metrics_t *metrics, egui_dim_t local_x);

#endif
