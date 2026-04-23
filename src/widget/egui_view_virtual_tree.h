#ifndef _EGUI_VIEW_VIRTUAL_TREE_H_
#define _EGUI_VIEW_VIRTUAL_TREE_H_

#include "egui_view_virtual_list.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_VIRTUAL_TREE_INVALID_INDEX 0xFFFFFFFFUL
#define EGUI_VIEW_VIRTUAL_TREE_MAX_DEPTH     64U

typedef egui_view_virtual_viewport_slot_t egui_view_virtual_tree_slot_t;

typedef struct egui_view_virtual_tree egui_view_virtual_tree_t;
typedef struct egui_view_virtual_tree_data_source egui_view_virtual_tree_data_source_t;
typedef struct egui_view_virtual_tree_entry egui_view_virtual_tree_entry_t;
typedef struct egui_view_virtual_tree_params egui_view_virtual_tree_params_t;
typedef struct egui_view_virtual_tree_setup egui_view_virtual_tree_setup_t;
/** Predicate used by `egui_view_virtual_tree_find_first_visible_node_view`. */
typedef uint8_t (*egui_view_virtual_tree_visible_node_matcher_t)(egui_view_t *self, const egui_view_virtual_tree_slot_t *slot,
                                                                 const egui_view_virtual_tree_entry_t *entry, egui_view_t *node_view, void *context);
/** Visitor used by `egui_view_virtual_tree_visit_visible_nodes`. Return 0 to stop early. */
typedef uint8_t (*egui_view_virtual_tree_visible_node_visitor_t)(egui_view_t *self, const egui_view_virtual_tree_slot_t *slot,
                                                                 const egui_view_virtual_tree_entry_t *entry, egui_view_t *node_view, void *context);

/** Tree-style virtual list that only realizes nodes whose expansion path is visible. */
struct egui_view_virtual_tree
{
    egui_view_virtual_list_t base;
    const egui_view_virtual_tree_data_source_t *data_source;
    void *data_source_context;
    egui_view_virtual_list_data_source_t flat_data_source;
};

/** One-shot parameter block for tree geometry and virtualization behavior. */
struct egui_view_virtual_tree_params
{
    egui_region_t region;
    uint8_t overscan_before;
    uint8_t overscan_after;
    uint8_t max_keepalive_slots;
    int32_t estimated_node_height;
};

/** Resolved metadata for one tree node. Collapsed nodes use `visible_index == INVALID_INDEX`. */
struct egui_view_virtual_tree_entry
{
    uint32_t visible_index;
    uint32_t stable_id;
    uint32_t parent_stable_id;
    uint32_t child_count;
    uint8_t depth;
    uint8_t has_children;
    uint8_t is_expanded;
};

#define EGUI_VIEW_VIRTUAL_TREE_PARAMS_INIT(_name, _x, _y, _w, _h)                                                                                              \
    static const egui_view_virtual_tree_params_t _name = {                                                                                                     \
            .region = {{(_x), (_y)}, {(_w), (_h)}},                                                                                                            \
            .overscan_before = 1,                                                                                                                              \
            .overscan_after = 1,                                                                                                                               \
            .max_keepalive_slots = 2,                                                                                                                          \
            .estimated_node_height = 40,                                                                                                                       \
    }

/** One-shot bundle that combines params, tree hooks, and node-state cache limits. */
struct egui_view_virtual_tree_setup
{
    const egui_view_virtual_tree_params_t *params;
    const egui_view_virtual_tree_data_source_t *data_source;
    void *data_source_context;
    uint16_t state_cache_max_entries;
    uint32_t state_cache_max_bytes;
};

#define EGUI_VIEW_VIRTUAL_TREE_SETUP_INIT(_name, _params, _data_source, _data_source_context)                                                                  \
    static const egui_view_virtual_tree_setup_t _name = {                                                                                                      \
            .params = (_params),                                                                                                                               \
            .data_source = (_data_source),                                                                                                                     \
            .data_source_context = (_data_source_context),                                                                                                     \
            .state_cache_max_entries = 0,                                                                                                                      \
            .state_cache_max_bytes = 0,                                                                                                                        \
    }

/** Tree data-source hooks. The widget flattens only currently visible expanded nodes into one virtual list. */
struct egui_view_virtual_tree_data_source
{
    uint32_t (*get_root_count)(void *data_source_context);
    uint32_t (*get_root_stable_id)(void *data_source_context, uint32_t root_index);
    uint32_t (*get_child_count)(void *data_source_context, uint32_t stable_id);
    uint32_t (*get_child_stable_id)(void *data_source_context, uint32_t stable_id, uint32_t child_index);
    uint8_t (*is_node_expanded)(void *data_source_context, uint32_t stable_id);
    uint16_t (*get_node_view_type)(void *data_source_context, const egui_view_virtual_tree_entry_t *entry);
    int32_t (*measure_node_height)(void *data_source_context, const egui_view_virtual_tree_entry_t *entry, int32_t width_hint);
    egui_view_t *(*create_node_view)(void *data_source_context, uint16_t view_type);
    void (*destroy_node_view)(void *data_source_context, egui_view_t *view, uint16_t view_type);
    void (*bind_node_view)(void *data_source_context, egui_view_t *view, const egui_view_virtual_tree_entry_t *entry);
    void (*unbind_node_view)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    uint8_t (*should_keep_alive)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*save_node_state)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*restore_node_state)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    uint16_t default_view_type;
};

#define EGUI_VIEW_VIRTUAL_TREE_DATA_SOURCE_INIT(_name)                                                                                                         \
    static const egui_view_virtual_tree_data_source_t _name = {                                                                                                \
            .get_root_count = NULL,                                                                                                                            \
            .get_root_stable_id = NULL,                                                                                                                        \
            .get_child_count = NULL,                                                                                                                           \
            .get_child_stable_id = NULL,                                                                                                                       \
            .is_node_expanded = NULL,                                                                                                                          \
            .get_node_view_type = NULL,                                                                                                                        \
            .measure_node_height = NULL,                                                                                                                       \
            .create_node_view = NULL,                                                                                                                          \
            .destroy_node_view = NULL,                                                                                                                         \
            .bind_node_view = NULL,                                                                                                                            \
            .unbind_node_view = NULL,                                                                                                                          \
            .should_keep_alive = NULL,                                                                                                                         \
            .save_node_state = NULL,                                                                                                                           \
            .restore_node_state = NULL,                                                                                                                        \
            .default_view_type = 0,                                                                                                                            \
    }

/** Apply a tree parameter block after initialization. */
void egui_view_virtual_tree_apply_params(egui_view_t *self, const egui_view_virtual_tree_params_t *params);
/** Initialize the tree and immediately apply its parameter block. */
void egui_view_virtual_tree_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_virtual_tree_params_t *params);
/** Apply params, data-source hooks, and cache limits from one setup bundle. */
void egui_view_virtual_tree_apply_setup(egui_view_t *self, const egui_view_virtual_tree_setup_t *setup);
/** Initialize the tree and immediately apply a setup bundle. */
void egui_view_virtual_tree_init_with_setup(egui_view_t *self, egui_core_t *core, const egui_view_virtual_tree_setup_t *setup);

/** Attach a tree data source. This rebuilds the internal flat-list bridge. Pass NULL to detach it. */
void egui_view_virtual_tree_set_data_source(egui_view_t *self, const egui_view_virtual_tree_data_source_t *data_source, void *data_source_context);
/** Return the currently attached tree data source, or NULL. */
const egui_view_virtual_tree_data_source_t *egui_view_virtual_tree_get_data_source(egui_view_t *self);
/** Return the opaque context pointer passed to the active tree data source. */
void *egui_view_virtual_tree_get_data_source_context(egui_view_t *self);
/** Return the current root-node count. */
uint32_t egui_view_virtual_tree_get_root_count(egui_view_t *self);
/** Return the count of nodes whose expansion path is currently visible. */
uint32_t egui_view_virtual_tree_get_visible_node_count(egui_view_t *self);

/** Keep additional visible nodes realized before and after the viewport. */
void egui_view_virtual_tree_set_overscan(egui_view_t *self, uint8_t before, uint8_t after);
/** Return the leading overscan node count. */
uint8_t egui_view_virtual_tree_get_overscan_before(egui_view_t *self);
/** Return the trailing overscan node count. */
uint8_t egui_view_virtual_tree_get_overscan_after(egui_view_t *self);

/** Set the fallback node height used before real measurements are known. */
void egui_view_virtual_tree_set_estimated_node_height(egui_view_t *self, int32_t height);
/** Return the fallback node height used by the virtual tree. */
int32_t egui_view_virtual_tree_get_estimated_node_height(egui_view_t *self);

/** Limit how many off-screen node views may stay alive for quick rebinding. */
void egui_view_virtual_tree_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots);
/** Return the current keepalive-node limit. */
uint8_t egui_view_virtual_tree_get_keepalive_limit(egui_view_t *self);

/** Configure the per-node state cache. Both limits must be non-zero or the cache is disabled and cleared. */
void egui_view_virtual_tree_set_state_cache_limits(egui_view_t *self, uint16_t max_entries, uint32_t max_bytes);
/** Return the maximum number of cached node-state records. */
uint16_t egui_view_virtual_tree_get_state_cache_entry_limit(egui_view_t *self);
/** Return the maximum byte budget for cached node state. */
uint32_t egui_view_virtual_tree_get_state_cache_byte_limit(egui_view_t *self);
/** Drop all cached per-node state. */
void egui_view_virtual_tree_clear_node_state_cache(egui_view_t *self);
/** Remove cached state for one node stable id. */
void egui_view_virtual_tree_remove_node_state_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Store custom state bytes for one node stable id. Passing `size == 0` removes the entry. */
uint8_t egui_view_virtual_tree_write_node_state(egui_view_t *self, uint32_t stable_id, const void *data, uint16_t size);
/** Read cached state for one node stable id. Returns the stored size even if `capacity` is smaller. */
uint16_t egui_view_virtual_tree_read_node_state(egui_view_t *self, uint32_t stable_id, void *data, uint16_t capacity);
/** Convenience helper that stores state for the realized node root view returned by the data source. */
uint8_t egui_view_virtual_tree_write_node_state_for_view(egui_view_t *node_view, uint32_t stable_id, const void *data, uint16_t size);
/** Convenience helper that reads state for the realized node root view returned by the data source. */
uint16_t egui_view_virtual_tree_read_node_state_for_view(egui_view_t *node_view, uint32_t stable_id, void *data, uint16_t capacity);

/** Jump the vertical scroll offset to an absolute Y position. */
void egui_view_virtual_tree_set_scroll_y(egui_view_t *self, int32_t offset);
/** Scroll by a signed Y delta. */
void egui_view_virtual_tree_scroll_by(egui_view_t *self, int32_t delta);
/** Scroll so the visible node at `visible_index` starts near `-node_offset` inside the viewport. */
void egui_view_virtual_tree_scroll_to_visible_node(egui_view_t *self, uint32_t visible_index, int32_t node_offset);
/** Scroll to a node by stable id, but only if that node is currently visible. */
void egui_view_virtual_tree_scroll_to_node_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t node_offset);
/** Return the current vertical scroll offset. */
int32_t egui_view_virtual_tree_get_scroll_y(egui_view_t *self);
/** Resolve a stable id to its current visible index, or -1 when the node is collapsed or missing. */
int32_t egui_view_virtual_tree_find_visible_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Resolve full node metadata by stable id, even for collapsed nodes. */
uint8_t egui_view_virtual_tree_resolve_node_by_stable_id(egui_view_t *self, uint32_t stable_id, egui_view_virtual_tree_entry_t *entry);
/** Resolve node metadata from the realized node root view. Descendants are not matched. */
uint8_t egui_view_virtual_tree_resolve_node_by_view(egui_view_t *self, egui_view_t *node_view, egui_view_virtual_tree_entry_t *entry);
/** Return the top Y position of one visible node. */
int32_t egui_view_virtual_tree_get_visible_node_y(egui_view_t *self, uint32_t visible_index);
/** Return the measured or estimated height of one visible node. */
int32_t egui_view_virtual_tree_get_visible_node_height(egui_view_t *self, uint32_t visible_index);
/** Return the top Y position of a node found by stable id, or -1 when that node is not visible. */
int32_t egui_view_virtual_tree_get_node_y_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the height of a node found by stable id, or -1 when that node is not visible. */
int32_t egui_view_virtual_tree_get_node_height_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Scroll if needed so a visible node becomes visible within the requested inset tolerance. */
uint8_t egui_view_virtual_tree_ensure_node_visible_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t inset);

/** Tell the tree that structure or expansion state changed and visible nodes may need rebuilding. */
void egui_view_virtual_tree_notify_data_changed(egui_view_t *self);
/** Tell the tree that one currently visible node changed in place. */
void egui_view_virtual_tree_notify_visible_node_changed(egui_view_t *self, uint32_t visible_index);
/** Notify one visible node change by stable id. */
void egui_view_virtual_tree_notify_node_changed_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Notify that one currently visible node changed height. */
void egui_view_virtual_tree_notify_visible_node_resized(egui_view_t *self, uint32_t visible_index);
/** Notify one visible node height change by stable id. */
void egui_view_virtual_tree_notify_node_resized_by_stable_id(egui_view_t *self, uint32_t stable_id);

/** Return the number of tracked slot records currently owned by the visible-node list. */
uint8_t egui_view_virtual_tree_get_slot_count(egui_view_t *self);
/** Access one tracked slot. Check `slot->state` before using it. */
const egui_view_virtual_tree_slot_t *egui_view_virtual_tree_get_slot(egui_view_t *self, uint8_t slot_index);
/** Return the tracked slot index for a realized stable id, or -1 if it is not currently in a slot. */
int32_t egui_view_virtual_tree_find_slot_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the tracked slot for a realized stable id, or NULL. */
const egui_view_virtual_tree_slot_t *egui_view_virtual_tree_find_slot_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the realized node root view for a stable id, or NULL if the node is not currently realized. */
egui_view_t *egui_view_virtual_tree_find_view_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Resolve a tracked slot back to its current node metadata. */
uint8_t egui_view_virtual_tree_get_slot_entry(egui_view_t *self, uint8_t slot_index, egui_view_virtual_tree_entry_t *entry);
/** Visit currently visible realized node root views. Returns the number of nodes passed to the visitor. */
uint8_t egui_view_virtual_tree_visit_visible_nodes(egui_view_t *self, egui_view_virtual_tree_visible_node_visitor_t visitor, void *context);
/** Return the lowest visible-index node that matches the predicate, optionally filling `entry_out`. */
egui_view_t *egui_view_virtual_tree_find_first_visible_node_view(egui_view_t *self, egui_view_virtual_tree_visible_node_matcher_t matcher, void *context,
                                                                 egui_view_virtual_tree_entry_t *entry_out);

/** Initialize the tree wrapper around the generic virtual list. */
void egui_view_virtual_tree_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_VIRTUAL_TREE_H_ */
