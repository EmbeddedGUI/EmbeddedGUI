#ifndef _EGUI_VIEW_NOTIFICATION_BADGE_INTERNAL_H_
#define _EGUI_VIEW_NOTIFICATION_BADGE_INTERNAL_H_

#include "egui_view_notification_badge.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Shrink the outer badge region by one pixel on each side to get a safe text drawing box. */
uint8_t egui_view_notification_badge_get_text_region(const egui_region_t *region, egui_region_t *text_region);
/** Format `count` into the badge's internal text buffer, using `max_display+` when needed. */
void egui_view_notification_badge_format_count_text(egui_view_notification_badge_t *local);
/** Draw either the circular or pill-shaped badge background inside the supplied region. */
void egui_view_notification_badge_draw_background(egui_canvas_t *canvas, const egui_region_t *region, egui_color_t badge_color, uint8_t use_circle);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_NOTIFICATION_BADGE_INTERNAL_H_ */
