#ifndef _EGUI_VIEW_CIRCLE_DIRTY_H_
#define _EGUI_VIEW_CIRCLE_DIRTY_H_

#include "egui_view.h"

#define EGUI_VIEW_CIRCLE_DIRTY_AA_PAD 2

int16_t egui_view_circle_dirty_normalize_angle(int32_t angle);
void egui_view_circle_dirty_get_circle_point(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t angle_deg, egui_dim_t *x, egui_dim_t *y);
uint8_t egui_view_circle_dirty_compute_arc_region(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t mid_radius, egui_dim_t expand_radius,
                                                  int16_t start_angle, uint16_t sweep, egui_region_t *dirty_region);
void egui_view_circle_dirty_union_region(egui_region_t *dirty_region, const egui_region_t *other);
void egui_view_circle_dirty_add_circle_region(egui_region_t *dirty_region, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t pad);
void egui_view_circle_dirty_add_line_region(egui_region_t *dirty_region, egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_width,
                                            egui_dim_t pad);
void egui_view_circle_dirty_add_rect_region(egui_region_t *dirty_region, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t pad);

#endif /* _EGUI_VIEW_CIRCLE_DIRTY_H_ */
