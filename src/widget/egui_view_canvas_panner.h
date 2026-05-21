#ifndef _EGUI_VIEW_CANVAS_PANNER_H_
#define _EGUI_VIEW_CANVAS_PANNER_H_

#include "egui_view_group.h"

/**
 * @brief Group-based canvas panner for large logical content areas.
 *
 * Child views are added directly to this widget. The panner exposes a logical
 * canvas that can be larger than the visible viewport, then shifts child layout
 * and drawing by the current pan offset.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_canvas_panner egui_view_canvas_panner_t;
typedef struct egui_view_canvas_panner_params egui_view_canvas_panner_params_t;

struct egui_view_canvas_panner
{
    egui_view_group_t base;

    /* Logical canvas size used when computing the scrollable range. */
    egui_dim_t canvas_width;
    egui_dim_t canvas_height;
    /* Current pan offset inside the logical canvas. */
    egui_dim_t offset_x;
    egui_dim_t offset_y;

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    /* Minimum drag distance before the widget claims a pan gesture. */
    egui_dim_t touch_slop;
    /* Initial and latest touch positions used for drag tracking. */
    egui_dim_t down_x;
    egui_dim_t down_y;
    egui_dim_t last_x;
    egui_dim_t last_y;
    /* Cached drag-axis capability of the child under the initial touch point. */
    uint8_t child_drag_axis_mask;
    /* Gesture ownership state used while negotiating child-vs-panner dragging. */
    uint8_t is_drag_target_resolved;
    uint8_t is_self_touch_owner;
    uint8_t is_dragging;
#endif
};

struct egui_view_canvas_panner_params
{
    egui_region_t region;
    egui_dim_t canvas_width;
    egui_dim_t canvas_height;
};

/** Convenience macro for one static canvas-panner parameter block. */
#define EGUI_VIEW_CANVAS_PANNER_PARAMS_INIT(_name, _x, _y, _w, _h, _canvas_w, _canvas_h)                                                                       \
    static const egui_view_canvas_panner_params_t _name = {                                                                                                    \
            .region = {{(_x), (_y)}, {(_w), (_h)}},                                                                                                            \
            .canvas_width = (_canvas_w),                                                                                                                       \
            .canvas_height = (_canvas_h),                                                                                                                      \
    }

/** Apply the viewport region and logical canvas size from one parameter block. */
void egui_view_canvas_panner_apply_params(egui_view_t *self, const egui_view_canvas_panner_params_t *params);
/** Initialize a canvas panner and immediately apply its parameter block. */
void egui_view_canvas_panner_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_canvas_panner_params_t *params);

/** Set the logical canvas size used for panning. Offsets are clamped to the new scrollable range. */
void egui_view_canvas_panner_set_canvas_size(egui_view_t *self, egui_dim_t canvas_width, egui_dim_t canvas_height);
/** Return the effective canvas width, which is never smaller than the viewport width. */
egui_dim_t egui_view_canvas_panner_get_canvas_width(egui_view_t *self);
/** Return the effective canvas height, which is never smaller than the viewport height. */
egui_dim_t egui_view_canvas_panner_get_canvas_height(egui_view_t *self);

/** Set the pan offset in pixels. Values outside the scrollable range are clamped automatically. */
void egui_view_canvas_panner_set_offset(egui_view_t *self, egui_dim_t offset_x, egui_dim_t offset_y);
/** Scroll relative to the current offset. This is a convenience wrapper around `set_offset()`. */
void egui_view_canvas_panner_scroll_by(egui_view_t *self, egui_dim_t delta_x, egui_dim_t delta_y);
/** Return the current horizontal pan offset. */
egui_dim_t egui_view_canvas_panner_get_offset_x(egui_view_t *self);
/** Return the current vertical pan offset. */
egui_dim_t egui_view_canvas_panner_get_offset_y(egui_view_t *self);
/** Return the maximum reachable horizontal pan offset. */
egui_dim_t egui_view_canvas_panner_get_max_offset_x(egui_view_t *self);
/** Return the maximum reachable vertical pan offset. */
egui_dim_t egui_view_canvas_panner_get_max_offset_y(egui_view_t *self);

/** Initialize the group-based panner. Add child views directly to this widget and drag it to pan the larger canvas. */
void egui_view_canvas_panner_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CANVAS_PANNER_H_ */
