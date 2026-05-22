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
#if EGUI_CONFIG_FUNCTION_SCROLL_LISTENER
typedef void (*egui_view_scroll_listener_t)(egui_view_t *self, egui_dim_t scroll_y);
#endif

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

#if EGUI_CONFIG_FUNCTION_SCROLL_SNAP
    egui_dim_t snap_interval_y; /* Snap interval in pixels (0 = no snap; positive to enable). */
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    /* Runtime flags for the optional right-side scrollbar overlay and thumb drag. */
    uint8_t is_scrollbar_enabled;
    uint8_t is_scrollbar_dragging;
#endif

#if EGUI_CONFIG_FUNCTION_SCROLL_LISTENER
    /** Callback invoked each time the scroll position changes.
     *  @param self   The scroll view.
     *  @param scroll_y  Current scroll offset in pixels
     * from the top (0 = at top). */
    egui_view_scroll_listener_t on_scroll;
#endif

#if EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
    uint8_t is_horizontal; /**< 1 = horizontal scroll; 0 = vertical (default). */
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
/** Return non-zero when the scrollbar overlay is currently enabled. */
uint8_t egui_view_scroll_get_scrollbar_enabled(egui_view_t *self);

#if EGUI_CONFIG_FUNCTION_SCROLL_SNAP
/**
 * @brief Set the vertical snap interval.
 *
 * When non-zero, lifting the finger after a drag will animate the scroll
 * position to the nearest multiple of @p interval pixels instead of starting
 * a free-form fling.  Pass 0 to restore free fling behaviour.
 */
void egui_view_scroll_set_snap_interval_y(egui_view_t *self, egui_dim_t interval);
/** Return the current vertical snap interval (0 means no snap). */
egui_dim_t egui_view_scroll_get_snap_interval_y(egui_view_t *self);
#endif /* EGUI_CONFIG_FUNCTION_SCROLL_SNAP */

#if EGUI_CONFIG_FUNCTION_SCROLL_LISTENER
/** Register a callback fired each time the scroll position changes.
 *  The callback receives the current scroll offset in pixels from the top (0 = fully
 * scrolled to top). */
void egui_view_scroll_set_on_scroll_listener(egui_view_t *self, egui_view_scroll_listener_t cb);
/** Return the callback fired each time the scroll position changes. */
egui_view_scroll_listener_t egui_view_scroll_get_on_scroll_listener(egui_view_t *self);
#endif /* EGUI_CONFIG_FUNCTION_SCROLL_LISTENER */

/** Return the current scroll offset (pixels from top). */
egui_dim_t egui_view_scroll_get_scroll_y(egui_view_t *self);
/** Return the current horizontal scroll offset (pixels from left). */
egui_dim_t egui_view_scroll_get_scroll_x(egui_view_t *self);

#if EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
/**
 * @brief Switch the scroll direction to horizontal (or back to vertical).
 *
 * Must be called right after egui_view_scroll_init() and before adding children.
 * In horizontal mode the internal container uses a horizontal linear layout
 * and touch events are tracked on the X axis instead of the Y axis.
 *
 * @param self         The scroll view.
 * @param is_horizontal  1 for horizontal, 0 for vertical (default).
 */
void egui_view_scroll_set_horizontal(egui_view_t *self, uint8_t is_horizontal);
/** Return non-zero when the scroll view is in horizontal mode. */
uint8_t egui_view_scroll_get_horizontal(egui_view_t *self);
#endif /* EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL */

/**
 * @brief Scroll the view the minimum distance needed so that @p child is fully visible.
 *
 * If the child is already fully within the viewport no action is taken.
 * @p child must be a direct child of the scroll view's internal container
 * (added via egui_view_scroll_add_child).
 *
 * @param self      The scroll view.
 * @param child     Direct child of the internal container to bring into view.
 * @param animated  Non-zero for a smooth animation; zero for an immediate jump.
 */
void egui_view_scroll_scroll_to_child(egui_view_t *self, egui_view_t *child, uint8_t animated);

/**
 * @brief Scroll the vertical scroll view to an absolute Y pixel offset.
 *
 * @p y is clamped to the valid range [0, content_height - viewport_height].
 * Pass animated=1 to use the snap animation, 0 for an immediate jump.
 *
 * @param self      The scroll view.
 * @param y         Target scroll offset in pixels from the top (0 = top).
 * @param animated  Non-zero for animated scrolling; zero for an immediate jump.
 */
void egui_view_scroll_to_y(egui_view_t *self, egui_dim_t y, uint8_t animated);

/**
 * @brief Scroll the horizontal scroll view to an absolute X pixel offset.
 *
 * @p x is clamped to the valid range [0, content_width - viewport_width].
 * Pass animated=1 to use the snap animation, 0 for an immediate jump.
 *
 * @param self      The scroll view.
 * @param x         Target scroll offset in pixels from the left (0 = left).
 * @param animated  Non-zero for animated scrolling; zero for an immediate jump.
 */
void egui_view_scroll_to_x(egui_view_t *self, egui_dim_t x, uint8_t animated);

/** Return non-zero when the scroll view is positioned at the very beginning (top or left). */
uint8_t egui_view_scroll_is_at_top(egui_view_t *self);
/** Return non-zero when the scroll view is positioned at the very end (bottom or right). */
uint8_t egui_view_scroll_is_at_bottom(egui_view_t *self);

/**
 * @brief Scroll the view by a relative pixel offset in the Y direction.
 *
 * Equivalent to scroll_to_y(current_y + delta_y).
 */
void egui_view_scroll_scroll_by_y(egui_view_t *self, egui_dim_t delta_y, uint8_t animated);
/**
 * @brief Scroll the view by a relative pixel offset in the X direction.
 *
 * Equivalent to scroll_to_x(current_x + delta_x).
 */
void egui_view_scroll_scroll_by_x(egui_view_t *self, egui_dim_t delta_x, uint8_t animated);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SCROLL_H_ */
