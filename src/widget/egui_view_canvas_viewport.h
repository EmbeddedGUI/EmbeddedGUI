#ifndef _EGUI_VIEW_CANVAS_VIEWPORT_H_
#define _EGUI_VIEW_CANVAS_VIEWPORT_H_

#include "egui_view_group.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_canvas_viewport egui_view_canvas_viewport_t;
typedef struct egui_view_canvas_viewport_params egui_view_canvas_viewport_params_t;

struct egui_view_canvas_viewport
{
    egui_view_group_t base;

    egui_view_group_t content_layer;
    egui_view_t *content_view;

    egui_dim_t canvas_width;
    egui_dim_t canvas_height;
    egui_dim_t offset_x;
    egui_dim_t offset_y;
    egui_dim_t drag_bar_height;

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_dim_t touch_slop;
    egui_dim_t down_x;
    egui_dim_t down_y;
    egui_dim_t last_x;
    egui_dim_t last_y;
    uint8_t child_drag_axis_mask;
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

#define EGUI_VIEW_CANVAS_VIEWPORT_PARAMS_INIT(_name, _x, _y, _w, _h, _canvas_w, _canvas_h)                                                                     \
    static const egui_view_canvas_viewport_params_t _name = {                                                                                                  \
            .region = {{(_x), (_y)}, {(_w), (_h)}},                                                                                                            \
            .canvas_width = (_canvas_w),                                                                                                                       \
            .canvas_height = (_canvas_h),                                                                                                                      \
    }

void egui_view_canvas_viewport_apply_params(egui_view_t *self, const egui_view_canvas_viewport_params_t *params);
void egui_view_canvas_viewport_init_with_params(egui_view_t *self, const egui_view_canvas_viewport_params_t *params);

void egui_view_canvas_viewport_set_canvas_size(egui_view_t *self, egui_dim_t canvas_width, egui_dim_t canvas_height);
egui_dim_t egui_view_canvas_viewport_get_canvas_width(egui_view_t *self);
egui_dim_t egui_view_canvas_viewport_get_canvas_height(egui_view_t *self);

void egui_view_canvas_viewport_set_content_view(egui_view_t *self, egui_view_t *content_view);
egui_view_t *egui_view_canvas_viewport_get_content_view(egui_view_t *self);
egui_view_t *egui_view_canvas_viewport_get_content_layer(egui_view_t *self);

void egui_view_canvas_viewport_set_drag_bar_height(egui_view_t *self, egui_dim_t drag_bar_height);
egui_dim_t egui_view_canvas_viewport_get_drag_bar_height(egui_view_t *self);

void egui_view_canvas_viewport_set_offset(egui_view_t *self, egui_dim_t offset_x, egui_dim_t offset_y);
void egui_view_canvas_viewport_scroll_by(egui_view_t *self, egui_dim_t delta_x, egui_dim_t delta_y);
egui_dim_t egui_view_canvas_viewport_get_offset_x(egui_view_t *self);
egui_dim_t egui_view_canvas_viewport_get_offset_y(egui_view_t *self);

void egui_view_canvas_viewport_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CANVAS_VIEWPORT_H_ */
