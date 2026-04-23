#ifndef _EGUI_VIEW_SCROLL_H_
#define _EGUI_VIEW_SCROLL_H_

#include "egui_view_group.h"
#include "core/egui_scroller.h"

#include "egui_view_linearlayout.h"

/**
 * @brief Vertical scroll view backed by an internal linear-layout container.
 *
 * Children are attached to the inner container rather than the outer widget.
 * The scroll view moves that container along the Y axis, optionally drives
 * inertial scrolling through `egui_scroller_t`, and can draw a right-side
 * scrollbar overlay.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_scroll egui_view_scroll_t;
struct egui_view_scroll
{
    egui_view_group_t base;

    /* Internal vertical container that owns caller-added child views. */
    egui_view_linearlayout_t container;

    /* Minimum pointer travel before the widget claims a drag gesture. */
    egui_dim_t touch_slop;
    /* Last touch Y coordinate used to compute drag deltas. */
    egui_dim_t last_motion_y;
    /* Nonzero once the current gesture has been promoted to dragging. */
    uint8_t is_begin_dragged;

    /* Physics helper used for fling animation after the finger is released. */
    egui_scroller_t scroller;

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    /* Runtime flags for the optional right-side scrollbar overlay and thumb drag. */
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

/** Convenience macro for one static scroll-view parameter block. */
#define EGUI_VIEW_SCROLL_PARAMS_INIT(_name, _x, _y, _w, _h) static const egui_view_scroll_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

/** Apply a simple region-based parameter block to a scroll view. */
void egui_view_scroll_apply_params(egui_view_t *self, const egui_view_scroll_params_t *params);
/** Initialize a scroll view and immediately apply its parameter block. */
void egui_view_scroll_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_scroll_params_t *params);

/** Add one child into the internal vertical scrolling container rather than directly to the outer view. */
void egui_view_scroll_add_child(egui_view_t *self, egui_view_t *child);
/** Relayout the internal linear container after child changes. */
void egui_view_scroll_layout_childs(egui_view_t *self);
/** Move the internal container by a clamped offset and stop animation when the top or bottom edge is reached. */
void egui_view_scroll_start_container_scroll(egui_view_t *self, int diff_y);
/** Start inertial scrolling using the latest tracked Y velocity. */
void egui_view_scroll_fling(egui_view_t *self, egui_float_t velocity_y);
/** Set the viewport size, keep container width equal to the viewport, and leave height child-driven. */
void egui_view_scroll_set_size(egui_view_t *self, egui_dim_t width, egui_dim_t height);
/** Advance any active scroller animation. Usually called by the framework. */
void egui_view_scroll_compute_scroll(egui_view_t *self);
/** Promote the current gesture to a drag when the movement exceeds touch slop. */
void egui_view_scroll_check_begin_dragged(egui_view_t *self, egui_dim_t delta);
/** Intercept touch when this scroll view needs to capture the current gesture. */
int egui_view_scroll_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event);
/** Handle drag, fling, and optional right-side scrollbar dragging. */
int egui_view_scroll_on_touch_event(egui_view_t *self, egui_motion_event_t *event);
/** Initialize the scroll view and its internal container/scroller state. */
void egui_view_scroll_init(egui_view_t *self, egui_core_t *core);

/** Enable or disable the built-in vertical scrollbar overlay drawn on the right edge. */
void egui_view_scroll_set_scrollbar_enabled(egui_view_t *self, uint8_t enabled);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SCROLL_H_ */
