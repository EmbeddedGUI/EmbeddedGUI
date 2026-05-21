#ifndef _EGUI_VIEW_VIEWPAGE_H_
#define _EGUI_VIEW_VIEWPAGE_H_

#include "egui_view_group.h"
#include "core/egui_scroller.h"
#include "font/egui_font.h"

#include "egui_view_linearlayout.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Listener fired when the stored current page changes. Both jump and animated page selection call it immediately. */
typedef void (*egui_view_viewpage_on_page_changed_t)(egui_view_t *self, int page_index);

typedef struct egui_view_viewpage egui_view_viewpage_t;
struct egui_view_viewpage
{
    egui_view_group_t base;

    egui_view_linearlayout_t container; // container in view scroll

    egui_dim_t touch_slop;
    egui_dim_t last_motion_x;
    uint8_t is_begin_dragged;

    uint8_t current_page_index;

    egui_scroller_t scroller;

    egui_view_viewpage_on_page_changed_t on_page_changed;

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    uint8_t is_scrollbar_enabled;
    uint8_t is_scrollbar_dragging;
#endif
};

// ============== ViewPage Params ==============
typedef struct egui_view_viewpage_params egui_view_viewpage_params_t;
struct egui_view_viewpage_params
{
    egui_region_t region;
};

#define EGUI_VIEW_VIEWPAGE_PARAMS_INIT(_name, _x, _y, _w, _h) static const egui_view_viewpage_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

/** Apply a simple region-based parameter block to a viewpage. */
void egui_view_viewpage_apply_params(egui_view_t *self, const egui_view_viewpage_params_t *params);
/** Initialize a viewpage and immediately apply its parameter block. */
void egui_view_viewpage_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_viewpage_params_t *params);

/** Add one page child into the internal horizontal container. Children are laid out left-to-right. */
void egui_view_viewpage_add_child(egui_view_t *self, egui_view_t *child);
/** Remove one page child from the internal container. */
void egui_view_viewpage_remove_child(egui_view_t *self, egui_view_t *child);
/** Relayout child pages inside the internal horizontal linearlayout. */
void egui_view_viewpage_layout_childs(egui_view_t *self);
/** Set the viewport size and keep the internal container height in sync. Container width stays child-driven. */
void egui_view_viewpage_set_size(egui_view_t *self, egui_dim_t width, egui_dim_t height);
/** Move the internal container by a clamped X offset. Mainly used by drag, fling, and animation code. */
void egui_view_viewpage_start_container_scroll(egui_view_t *self, int diff_x);
/** Start inertial horizontal scrolling. */
void egui_view_viewpage_fling(egui_view_t *self, egui_float_t velocity_x);
/** Animate to the specified page index and update the current-page callback immediately if the target changes. */
void egui_view_viewpage_scroll_to_page(egui_view_t *self, int page_index);
/** Snap to the nearest page by animating from the current drag position. */
void egui_view_viewpage_slow_scroll_to_page(egui_view_t *self);
/** Jump to the specified page immediately without scroll animation. */
void egui_view_viewpage_set_current_page(egui_view_t *self, int page_index);
/** Return the index of the currently visible page. */
int egui_view_viewpage_get_current_page(egui_view_t *self);
/** Return the total number of pages (child views) in the container. */
int egui_view_viewpage_get_page_count(egui_view_t *self);
/** Advance any active scrolling animation. Usually called by the framework. */
void egui_view_viewpage_compute_scroll(egui_view_t *self);
/** Promote the current gesture to a horizontal drag when movement exceeds touch slop. */
void egui_view_viewpage_check_begin_dragged(egui_view_t *self, egui_dim_t delta);
/** Intercept touch when this viewpage needs to capture the current gesture. */
int egui_view_viewpage_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event);
/** Handle drag, snap, fling, and optional bottom-scrollbar interaction. */
int egui_view_viewpage_on_touch_event(egui_view_t *self, egui_motion_event_t *event);
/** Register the callback fired when the current page changes. */
void egui_view_viewpage_set_on_page_changed(egui_view_t *self, egui_view_viewpage_on_page_changed_t callback);
/** Return the registered page-change callback, or NULL when unset or self is NULL. */
egui_view_viewpage_on_page_changed_t egui_view_viewpage_get_on_page_changed(egui_view_t *self);
/** Initialize the horizontal page container widget. */
void egui_view_viewpage_init(egui_view_t *self, egui_core_t *core);

/** Enable or disable the built-in horizontal scrollbar overlay drawn along the bottom edge. */
void egui_view_viewpage_set_scrollbar_enabled(egui_view_t *self, uint8_t enabled);
/** Return non-zero when the horizontal scrollbar overlay is currently enabled. */
uint8_t egui_view_viewpage_get_scrollbar_enabled(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_VIEWPAGE_H_ */
