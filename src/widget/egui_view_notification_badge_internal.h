#ifndef _EGUI_VIEW_NOTIFICATION_BADGE_INTERNAL_H_
#define _EGUI_VIEW_NOTIFICATION_BADGE_INTERNAL_H_

#include "egui_view_notification_badge.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t egui_view_notification_badge_get_text_region(const egui_region_t *region, egui_region_t *text_region);
void egui_view_notification_badge_format_count_text(egui_view_notification_badge_t *local);
void egui_view_notification_badge_draw_background(const egui_region_t *region, egui_color_t badge_color, uint8_t use_circle);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_NOTIFICATION_BADGE_INTERNAL_H_ */
