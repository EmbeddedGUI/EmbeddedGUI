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

    egui_dlist_t childs; // used for child views
};

typedef struct egui_view_root_group egui_view_root_group_t;
struct egui_view_root_group
{
    egui_view_group_t base;

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    uint8_t is_disallow_process_touch_event; // if set, the root group will not process touch events
#endif
};

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
typedef struct egui_view_group_touch_state_snapshot egui_view_group_touch_state_snapshot_t;
struct egui_view_group_touch_state_snapshot
{
    uint8_t is_active;
    uint8_t is_disallow_intercept;
    uint8_t path_len;
    egui_view_t *path[EGUI_CONFIG_TOUCH_CAPTURE_PATH_MAX];
};
#else
typedef struct egui_view_group_touch_state_snapshot egui_view_group_touch_state_snapshot_t;
struct egui_view_group_touch_state_snapshot
{
    uint8_t unused;
};
#endif

// ============== Group Params ==============
typedef struct egui_view_group_params egui_view_group_params_t;
struct egui_view_group_params
{
    egui_region_t region;
};

#define EGUI_VIEW_GROUP_PARAMS_INIT(_name, _x, _y, _w, _h) static const egui_view_group_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

void egui_view_group_apply_params(egui_view_t *self, const egui_view_group_params_t *params);
void egui_view_group_init_with_params(egui_view_t *self, const egui_view_group_params_t *params);

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
void egui_view_group_touch_state_exchange(egui_view_group_touch_state_snapshot_t *snapshot);
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
void egui_view_root_group_init(egui_view_t *self);

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
int egui_view_group_dispatch_key_event(egui_view_t *self, egui_key_event_t *event);
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_LAYER
void egui_view_group_reorder_child(egui_view_t *self, egui_view_t *child);
void egui_view_group_bring_child_to_front(egui_view_t *self, egui_view_t *child);
void egui_view_group_send_child_to_back(egui_view_t *self, egui_view_t *child);
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_GROUP_H_ */
