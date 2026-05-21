#ifndef _EGUI_VIEW_GROUP_H_
#define _EGUI_VIEW_GROUP_H_

#include "egui_view.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Generic container view that owns an ordered child list. */
struct egui_view_group
{
    egui_view_t base;

    egui_dlist_t childs; // used for child views
};

typedef struct egui_view_root_group egui_view_root_group_t;
/** Special root container used directly by the core for activities, dialogs, and user roots. */
struct egui_view_root_group
{
    egui_view_group_t base;

    uint8_t is_disallow_process_touch_event; // if set, the root group will not process touch events
};

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
typedef struct egui_view_group_touch_state_snapshot egui_view_group_touch_state_snapshot_t;
/** Snapshot of the current captured touch path so callers can save and restore nested interaction state. */
struct egui_view_group_touch_state_snapshot
{
#if EGUI_CONFIG_FUNCTION_VIEW_GROUP_TOUCH_CAPTURE_PATH
    uint8_t is_active;
    uint8_t is_disallow_intercept;
    uint8_t path_len;
    egui_view_t *path[EGUI_CONFIG_TOUCH_CAPTURE_PATH_MAX];
#else
    uint8_t is_active;
    egui_view_t *captured_view;
#endif
};
#else
typedef struct egui_view_group_touch_state_snapshot egui_view_group_touch_state_snapshot_t;
/** Placeholder snapshot type when touch support is disabled. */
struct egui_view_group_touch_state_snapshot
{
    uint8_t unused;
};
#endif

// ============== Group Params ==============
typedef struct egui_view_group_params egui_view_group_params_t;
/** One-shot parameter block for the container bounds. */
struct egui_view_group_params
{
    egui_region_t region;
};

#define EGUI_VIEW_GROUP_PARAMS_INIT(_name, _x, _y, _w, _h) static const egui_view_group_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

/** Apply a simple region-based parameter block to a group view. */
void egui_view_group_apply_params(egui_view_t *self, const egui_view_group_params_t *params);
/** Initialize a group view and immediately apply its parameter block. */
void egui_view_group_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_group_params_t *params);

#define EGUI_VIEW_GROUP_ADD_CHILD_TREE(_group, _child_tree) egui_view_group_add_child_tree((_group), (_child_tree), EGUI_ARRAY_SIZE(_child_tree))

/* Child-tree management helpers. */
/** Append one child to the container, synchronizing its core and attach state. */
void egui_view_group_add_child(egui_view_t *self, egui_view_t *child);
/** Remove one child from the container and detach it from the window if needed. */
void egui_view_group_remove_child(egui_view_t *self, egui_view_t *child);
/** Remove and detach all children currently owned by the container. */
void egui_view_group_clear_childs(egui_view_t *self);
/** Return the number of children currently linked into the container. */
int egui_view_group_get_child_count(egui_view_t *self);
/** Return the first child in draw/layout order, or NULL when the container is empty. */
egui_view_t *egui_view_group_get_first_child(egui_view_t *self);
/** Return the last child in draw/layout order, or NULL when the container is empty. */
egui_view_t *egui_view_group_get_last_child(egui_view_t *self);
/** Return the child at position @p index (0-based), or NULL if out of range. */
egui_view_t *egui_view_group_get_child_at(egui_view_t *self, int index);
/** Return the 0-based index of @p child inside this container, or -1 if not found. */
int egui_view_group_get_child_index(egui_view_t *self, egui_view_t *child);
/** Move @p child to @p index (0-based) in draw/layout order; clamps to end when index >= count. */
void egui_view_group_move_child_to_index(egui_view_t *self, egui_view_t *child, int index);

/* Layout helpers for groups that position children manually. */
/** Sum all non-gone child widths plus horizontal margins. */
void egui_view_group_calculate_all_child_width(egui_view_t *self, egui_dim_t *width);
/** Sum all non-gone child heights plus vertical margins. */
void egui_view_group_calculate_all_child_height(egui_view_t *self, egui_dim_t *height);
/** Return the widest non-gone child width plus horizontal margins. */
void egui_view_group_get_max_child_width(egui_view_t *self, egui_dim_t *width);
/** Return the tallest non-gone child height plus vertical margins. */
void egui_view_group_get_max_child_height(egui_view_t *self, egui_dim_t *height);
/** Lay out children linearly inside the group's content area and optionally auto-size the group itself. */
void egui_view_group_layout_childs(egui_view_t *self, uint8_t is_orientation_horizontal, uint8_t is_auto_width, uint8_t is_auto_height, uint8_t align_type);

/** Enable or disable touch processing on a root group. */
void egui_view_group_set_disallow_process_touch_event(egui_view_t *self, int disallow);
/** Return whether touch processing is disabled on a root group. */
int egui_view_group_get_disallow_process_touch_event(egui_view_t *self);
/** Ask ancestor groups not to intercept the current touch sequence. */
void egui_view_group_request_disallow_intercept_touch_event(egui_view_t *self, int disallow);
/** Swap the current touch-capture state with a snapshot. Useful for nested modal flows. */
void egui_view_group_touch_state_exchange(egui_core_t *core, egui_view_group_touch_state_snapshot_t *snapshot);
/** Default intercept hook for base groups. Override this in scrollable containers. */
int egui_view_group_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event);
/** Propagate `compute_scroll` through all child views. */
void egui_view_group_compute_scroll(egui_view_t *self);
/** Entry point for group touch dispatch with capture-path tracking across the whole gesture. */
int egui_view_group_dispatch_touch_event(egui_view_t *self, egui_motion_event_t *event);
/** Default group touch handler implementing clickable-container behavior. */
int egui_view_group_on_touch_event(egui_view_t *self, egui_motion_event_t *event);
/** Attach all current children to the window after the group is attached. */
void egui_view_group_on_attach_to_window(egui_view_t *self);
/** Detach all current children from the window before the group is detached. */
void egui_view_group_on_detach_from_window(egui_view_t *self);
/** Draw the group itself first, then draw all visible children in order. */
void egui_view_group_draw(egui_view_t *self);
/** Request layout for the group and all current children. */
void egui_view_group_request_layout(egui_view_t *self);
/** Recalculate layout for the group and then all current children. */
void egui_view_group_calculate_layout(egui_view_t *self);
/** Initialize a generic container view that can own child views. */
void egui_view_group_init(egui_view_t *self, egui_core_t *core);
/** Initialize a root group used directly by the core, activities, or dialogs. */
void egui_view_root_group_init(egui_view_t *self, egui_core_t *core);

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
/** Dispatch key events to the focused child first, then to the group itself. */
int egui_view_group_dispatch_key_event(egui_view_t *self, egui_key_event_t *event);
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_LAYER
/** Reinsert one child according to its current layer value. */
void egui_view_group_reorder_child(egui_view_t *self, egui_view_t *child);
/** Move one child to the top-most predefined layer. */
void egui_view_group_bring_child_to_front(egui_view_t *self, egui_view_t *child);
/** Move one child to the background predefined layer. */
void egui_view_group_send_child_to_back(egui_view_t *self, egui_view_t *child);
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_GROUP_H_ */
