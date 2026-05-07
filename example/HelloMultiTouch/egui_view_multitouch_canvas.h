#ifndef _EGUI_VIEW_MULTITOUCH_CANVAS_H_
#define _EGUI_VIEW_MULTITOUCH_CANVAS_H_

#include "widget/egui_view.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MULTITOUCH_TRACE_MAX_POINTS 128

typedef struct multitouch_trace_point
{
    egui_dim_t x;
    egui_dim_t y;
} multitouch_trace_point_t;

typedef struct egui_view_multitouch_canvas
{
    egui_view_t base;
    multitouch_trace_point_t traces[EGUI_CONFIG_TOUCH_MAX_POINTS][MULTITOUCH_TRACE_MAX_POINTS];
    uint16_t trace_counts[EGUI_CONFIG_TOUCH_MAX_POINTS];
    egui_location_t last_points[EGUI_CONFIG_TOUCH_MAX_POINTS];
    uint8_t active_points;
    uint8_t is_tracking;
    uint8_t is_pinching;
    uint8_t last_event_type;
    uint16_t event_count;
    int16_t pinch_start_distance;
    int16_t pinch_current_distance;
    int16_t scale_percent;
} egui_view_multitouch_canvas_t;

void egui_view_multitouch_canvas_init(egui_view_t *self, egui_core_t *core);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_MULTITOUCH_CANVAS_H_ */
