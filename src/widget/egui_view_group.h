#ifndef _EGUI_VIEW_GROUP_H_
#define _EGUI_VIEW_GROUP_H_

#include "egui_view.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

struct egui_view_group
{
    egui_view_t base;

    uint8_t is_disallow_intercept;           // if set, the group will not intercept touch events
    uint8_t is_disallow_process_touch_event; // if set, the group will not process touch events

    egui_view_t *first_touch_target; // used for touch event handling
    egui_dlist_t childs;             // used for child views
};

#define EGUI_VIEW_API_DEFINE_BASE_GROUP(_name, _dispatch_touch_event, _on_touch_event, _on_intercept_touch_event, _compute_scroll, _calculate_layout,          \
                                        _request_layout, _draw, _on_attach_to_window, _on_draw, _on_detach_from_window)                                        \
    EGUI_VIEW_API_DEFINE(_name, _dispatch_touch_event == NULL ? egui_view_group_dispatch_touch_event : _dispatch_touch_event,                                  \
                         _on_touch_event == NULL ? egui_view_group_on_touch_event : _on_touch_event,                                                           \
                         _on_intercept_touch_event == NULL ? egui_view_group_on_intercept_touch_event : _on_intercept_touch_event,                             \
                         _compute_scroll == NULL ? egui_view_group_compute_scroll : _compute_scroll,                                                           \
                         _calculate_layout == NULL ? egui_view_group_calculate_layout : _calculate_layout,                                                     \
                         _request_layout == NULL ? egui_view_group_request_layout : _request_layout, _draw == NULL ? egui_view_group_draw : _draw,             \
                         _on_attach_to_window == NULL ? egui_view_group_on_attach_to_window : _on_attach_to_window, _on_draw,                                  \
                         _on_detach_from_window == NULL ? egui_view_group_on_detach_from_window : _on_detach_from_window)

#define EGUI_VIEW_GROUP_ADD_CHILD_TREE(_group, _child_tree) egui_view_group_add_child_tree((_group), (_child_tree), EGUI_ARRAY_SIZE(_child_tree))

void egui_view_group_add_child(egui_view_t *self, egui_view_t *child);
void egui_view_group_remove_child(egui_view_t *self, egui_view_t *child);
void egui_view_group_clear_childs(egui_view_t *self);
int egui_view_group_get_child_count(egui_view_t *self);
egui_view_t *egui_view_group_get_first_child(egui_view_t *self);

void egui_view_group_calculate_all_child_width(egui_view_t *self, egui_dim_t *width);
void egui_view_group_calculate_all_child_height(egui_view_t *self, egui_dim_t *height);
void egui_view_group_get_max_child_width(egui_view_t *self, egui_dim_t *width);
void egui_view_group_get_max_child_height(egui_view_t *self, egui_dim_t *height);
void egui_view_group_layout_childs(egui_view_t *self, uint8_t is_orientation_horizontal, uint8_t is_auto_width, uint8_t is_auto_height, uint8_t align_type);

void egui_view_group_set_disallow_process_touch_event(egui_view_t *self, int disallow);
void egui_view_group_request_disallow_intercept_touch_event(egui_view_t *self, int disallow);
int egui_view_group_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event);
void egui_view_group_compute_scroll(egui_view_t *self);
int egui_view_group_dispatch_touch_event(egui_view_t *self, egui_motion_event_t *event);
int egui_view_group_on_touch_event(egui_view_t *self, egui_motion_event_t *event);
void egui_view_group_on_attach_to_window(egui_view_t *self);
void egui_view_group_on_detach_from_window(egui_view_t *self);
void egui_view_group_draw(egui_view_t *self);
void egui_view_group_request_layout(egui_view_t *self);
void egui_view_group_calculate_layout(egui_view_t *self);
void egui_view_group_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_GROUP_H_ */
