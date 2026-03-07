#ifndef _EGUI_VIEW_SCROLL_H_
#define _EGUI_VIEW_SCROLL_H_

#include "egui_view_group.h"
#include "core/egui_scroller.h"

#include "egui_view_linearlayout.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_scroll egui_view_scroll_t;
struct egui_view_scroll
{
    egui_view_group_t base;

    egui_view_linearlayout_t container; // container in view scroll

    egui_dim_t touch_slop;
    egui_dim_t last_motion_y;
    uint8_t is_begin_dragged;

    egui_scroller_t scroller;

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    uint8_t is_scrollbar_enabled;
    uint8_t is_scrollbar_dragging;
#endif
};

// ============== Scroll Params ==============
typedef struct egui_view_scroll_params egui_view_scroll_params_t;
struct egui_view_scroll_params
{
    egui_region_t region;
};

#define EGUI_VIEW_SCROLL_PARAMS_INIT(_name, _x, _y, _w, _h) static const egui_view_scroll_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

void egui_view_scroll_apply_params(egui_view_t *self, const egui_view_scroll_params_t *params);
void egui_view_scroll_init_with_params(egui_view_t *self, const egui_view_scroll_params_t *params);

void egui_view_scroll_add_child(egui_view_t *self, egui_view_t *child);
void egui_view_scroll_layout_childs(egui_view_t *self);
void egui_view_scroll_start_container_scroll(egui_view_t *self, int diff_y);
void egui_view_scroll_fling(egui_view_t *self, egui_float_t velocity_y);
void egui_view_scroll_set_size(egui_view_t *self, egui_dim_t width, egui_dim_t height);
void egui_view_scroll_compute_scroll(egui_view_t *self);
void egui_view_scroll_check_begin_dragged(egui_view_t *self, egui_dim_t delta);
int egui_view_scroll_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event);
int egui_view_scroll_on_touch_event(egui_view_t *self, egui_motion_event_t *event);
void egui_view_scroll_init(egui_view_t *self);

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
void egui_view_scroll_set_scrollbar_enabled(egui_view_t *self, uint8_t enabled);
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SCROLL_H_ */
