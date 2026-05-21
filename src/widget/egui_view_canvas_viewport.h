#ifndef _EGUI_VIEW_CANVAS_VIEWPORT_H_
#define _EGUI_VIEW_CANVAS_VIEWPORT_H_

#include "egui_view_group.h"

/**
 * @brief Clipping viewport that hosts one movable content subtree.
 *
 * The viewport owns an internal content layer, clips drawing to its visible
 * rectangle, and scrolls the hosted content view within a separate logical
 * canvas extent.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_canvas_viewport egui_view_canvas_viewport_t;
typedef struct egui_view_canvas_viewport_params egui_view_canvas_viewport_params_t;

struct egui_view_canvas_viewport
{
    egui_view_group_t base;

    /* Internal parent layer whose origin is shifted by the viewport offsets. */
    egui_view_group_t content_layer;
    /* Optional caller-owned content view attached to `content_layer`. */
    egui_view_t *content_view;

    /* Logical canvas size that the content can scroll inside. */
    egui_dim_t canvas_width;
    egui_dim_t canvas_height;
    /* Current scroll offset inside the logical canvas. */
    egui_dim_t offset_x;
    egui_dim_t offset_y;
    /* Optional bottom drag-bar height shown when scrolling is possible. */
    egui_dim_t drag_bar_height;

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    /* Minimum drag distance before the viewport claims a pan gesture. */
    egui_dim_t touch_slop;
    /* Initial and latest touch positions used for drag tracking. */
    egui_dim_t down_x;
    egui_dim_t down_y;
    egui_dim_t last_x;
    egui_dim_t last_y;
    /* Cached drag-axis capability of the hit child inside the content layer. */
    uint8_t child_drag_axis_mask;
    /* Gesture ownership state used while negotiating child-vs-viewport dragging. */
    uint8_t is_drag_target_resolved;
    uint8_t is_child_drag_target;
    uint8_t is_dragging;
#endif
};

struct egui_view_canvas_viewport_params
{
    egui_region_t region;
    egui_dim_t canvas_width;
    egui_dim_t canvas_height;
};

/** Convenience macro for one static canvas-viewport parameter block. */
#define EGUI_VIEW_CANVAS_VIEWPORT_PARAMS_INIT(_name, _x, _y, _w, _h, _canvas_w, _canvas_h)                                                                     \
    static const egui_view_canvas_viewport_params_t _name = {                                                                                                  \
            .region = {{(_x), (_y)}, {(_w), (_h)}},                                                                                                            \
            .canvas_width = (_canvas_w),                                                                                                                       \
            .canvas_height = (_canvas_h),                                                                                                                      \
    }

/** Apply the viewport region and logical canvas size from one parameter block. */
void egui_view_canvas_viewport_apply_params(egui_view_t *self, const egui_view_canvas_viewport_params_t *params);
/** Initialize a canvas viewport and immediately apply its parameter block. */
void egui_view_canvas_viewport_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_canvas_viewport_params_t *params);

/** Set the logical canvas size used for clipping and scrolling. Offsets are clamped to the new range. */
void egui_view_canvas_viewport_set_canvas_size(egui_view_t *self, egui_dim_t canvas_width, egui_dim_t canvas_height);
/** Return the current logical canvas width. */
egui_dim_t egui_view_canvas_viewport_get_canvas_width(egui_view_t *self);
/** Return the current logical canvas height. */
egui_dim_t egui_view_canvas_viewport_get_canvas_height(egui_view_t *self);

/** Replace the single content view hosted inside the internal content layer. Ownership stays with the caller. */
void egui_view_canvas_viewport_set_content_view(egui_view_t *self, egui_view_t *content_view);
/** Return the current content view pointer, or NULL when none is attached. */
egui_view_t *egui_view_canvas_viewport_get_content_view(egui_view_t *self);
/** Return the internal content layer used as the parent for the hosted content view. */
egui_view_t *egui_view_canvas_viewport_get_content_layer(egui_view_t *self);

/** Set the optional drag-bar height. Negative values clamp to 0, and the bar only appears when scrolling is possible. */
void egui_view_canvas_viewport_set_drag_bar_height(egui_view_t *self, egui_dim_t drag_bar_height);
/** Return the configured drag-bar height. */
egui_dim_t egui_view_canvas_viewport_get_drag_bar_height(egui_view_t *self);

/** Set the scroll offset in pixels. Values outside the scrollable range are clamped automatically. */
void egui_view_canvas_viewport_set_offset(egui_view_t *self, egui_dim_t offset_x, egui_dim_t offset_y);
/** Scroll relative to the current offset. This is a convenience wrapper around `set_offset()`. */
void egui_view_canvas_viewport_scroll_by(egui_view_t *self, egui_dim_t delta_x, egui_dim_t delta_y);
/** Return the current horizontal scroll offset. */
egui_dim_t egui_view_canvas_viewport_get_offset_x(egui_view_t *self);
/** Return the current vertical scroll offset. */
egui_dim_t egui_view_canvas_viewport_get_offset_y(egui_view_t *self);
/** Return the maximum reachable horizontal scroll offset. */
egui_dim_t egui_view_canvas_viewport_get_max_offset_x(egui_view_t *self);
/** Return the maximum reachable vertical scroll offset. */
egui_dim_t egui_view_canvas_viewport_get_max_offset_y(egui_view_t *self);

/** Initialize the clipping viewport and its internal content layer. */
void egui_view_canvas_viewport_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CANVAS_VIEWPORT_H_ */
