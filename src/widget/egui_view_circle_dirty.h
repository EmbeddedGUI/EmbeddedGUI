#ifndef _EGUI_VIEW_CIRCLE_DIRTY_H_
#define _EGUI_VIEW_CIRCLE_DIRTY_H_

#include "egui_view.h"

/**
 * @brief Shared dirty-region utilities for widgets that draw circles, arcs, or hands.
 *
 * The helpers return conservative axis-aligned regions that are inexpensive to
 * compute and sufficient for partial redraw decisions.
 */

/** Extra padding commonly used to keep anti-aliased circular edges inside the dirty region. */
#define EGUI_VIEW_CIRCLE_DIRTY_AA_PAD 2

/** Normalize any signed angle in degrees into the range `[0, 359]`. */
int16_t egui_view_circle_dirty_normalize_angle(int32_t angle);
/** Compute one screen-space point on a circle for the given center, radius, and degree angle. */
void egui_view_circle_dirty_get_circle_point(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t angle_deg, egui_dim_t *x, egui_dim_t *y);
/** Compute a conservative dirty box for an arc path sampled at endpoints and cardinal extrema. */
uint8_t egui_view_circle_dirty_compute_arc_region(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t mid_radius, egui_dim_t expand_radius,
                                                  int16_t start_angle, uint16_t sweep, egui_region_t *dirty_region);
/** Union one non-empty region into the accumulated dirty region. */
void egui_view_circle_dirty_union_region(egui_region_t *dirty_region, const egui_region_t *other);
/** Expand the accumulated dirty region with the padded bounding box of a circle. */
void egui_view_circle_dirty_add_circle_region(egui_region_t *dirty_region, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t pad);
/** Expand the accumulated dirty region with the padded axis-aligned bounds of a stroked line segment. */
void egui_view_circle_dirty_add_line_region(egui_region_t *dirty_region, egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_width,
                                            egui_dim_t pad);
/** Expand the accumulated dirty region with a rectangle plus extra padding. */
void egui_view_circle_dirty_add_rect_region(egui_region_t *dirty_region, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t pad);

#endif /* _EGUI_VIEW_CIRCLE_DIRTY_H_ */
