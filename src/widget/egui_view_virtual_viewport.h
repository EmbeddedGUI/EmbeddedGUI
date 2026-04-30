#ifndef _EGUI_VIEW_VIRTUAL_VIEWPORT_H_
#define _EGUI_VIEW_VIRTUAL_VIEWPORT_H_

#include "egui_view_group.h"
#include "core/egui_scroller.h"
#include "utils/egui_dlist.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS         12
#define EGUI_VIEW_VIRTUAL_VIEWPORT_INDEX_BLOCK_ITEMS 16
#define EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID        0xFFFFFFFFUL

enum
{
    EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_VERTICAL = 0,
    EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_HORIZONTAL = 1,
};

enum
{
    EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED = 0,
    EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE,
    EGUI_VIEW_VIRTUAL_SLOT_STATE_KEEPALIVE,
    EGUI_VIEW_VIRTUAL_SLOT_STATE_POOLED,
};

typedef struct egui_view_virtual_viewport_adapter egui_view_virtual_viewport_adapter_t;
typedef struct egui_view_virtual_viewport_slot egui_view_virtual_viewport_slot_t;
typedef struct egui_view_virtual_viewport egui_view_virtual_viewport_t;
typedef struct egui_view_virtual_viewport_entry egui_view_virtual_viewport_entry_t;
typedef struct egui_view_virtual_viewport_params egui_view_virtual_viewport_params_t;
typedef struct egui_view_virtual_viewport_setup egui_view_virtual_viewport_setup_t;
/** Predicate used by `egui_view_virtual_viewport_find_first_visible_item_view`. */
typedef uint8_t (*egui_view_virtual_viewport_visible_item_matcher_t)(egui_view_t *self, const egui_view_virtual_viewport_slot_t *slot,
                                                                     const egui_view_virtual_viewport_entry_t *entry, egui_view_t *item_view, void *context);
/** Visitor used by `egui_view_virtual_viewport_visit_visible_items`. Return 0 to stop early. */
typedef uint8_t (*egui_view_virtual_viewport_visible_item_visitor_t)(egui_view_t *self, const egui_view_virtual_viewport_slot_t *slot,
                                                                     const egui_view_virtual_viewport_entry_t *entry, egui_view_t *item_view, void *context);

/** Adapter hooks implemented by user code to expose a large item set through a small pool of realized views. */
struct egui_view_virtual_viewport_adapter
{
    uint32_t (*get_count)(void *adapter_context);
    uint32_t (*get_stable_id)(void *adapter_context, uint32_t index);
    int32_t (*find_index_by_stable_id)(void *adapter_context, uint32_t stable_id);
    uint16_t (*get_view_type)(void *adapter_context, uint32_t index);
    int32_t (*measure_main_size)(void *adapter_context, uint32_t index, int32_t cross_size_hint);
    egui_view_t *(*create_view)(void *adapter_context, uint16_t view_type);
    void (*destroy_view)(void *adapter_context, egui_view_t *view, uint16_t view_type);
    void (*bind_view)(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id);
    void (*unbind_view)(void *adapter_context, egui_view_t *view, uint32_t stable_id);
    uint8_t (*should_keep_alive)(void *adapter_context, egui_view_t *view, uint32_t stable_id);
    void (*save_state)(void *adapter_context, egui_view_t *view, uint32_t stable_id);
    void (*restore_state)(void *adapter_context, egui_view_t *view, uint32_t stable_id);
};

struct egui_view_virtual_viewport_slot
{
    uint8_t state;
    uint16_t view_type;
    uint32_t index;
    uint32_t stable_id;
    egui_view_t *view;
    egui_region_t render_region;
    int32_t logical_main_origin;
    int32_t logical_main_size;
    uint32_t last_used_seq;
};

struct egui_view_virtual_viewport
{
    egui_view_group_t base;

    egui_view_group_t content_layer;

    const egui_view_virtual_viewport_adapter_t *adapter;
    void *adapter_context;

    uint8_t orientation;
    uint8_t overscan_before;
    uint8_t overscan_after;
    uint8_t max_keepalive_slots;
    uint8_t slot_count;
    uint8_t is_data_dirty;
    uint8_t is_layout_dirty;
    uint8_t is_attached_to_window;
    uint8_t is_begin_dragged;

    egui_dim_t touch_slop;
    egui_dim_t last_motion_main;

    uint32_t anchor_stable_id;
    uint32_t slot_use_seq;
    int32_t anchor_offset;
    int32_t logical_offset;
    int32_t logical_extent;
    int32_t viewport_extent;
    int32_t cross_extent;
    int32_t estimated_item_extent;
    uint8_t are_index_prefixes_dirty;
    uint16_t index_block_count;
    uint16_t index_block_capacity;
    uint16_t max_state_entries;
    uint16_t state_entry_count;
    uint32_t indexed_item_count;
    uint32_t max_state_bytes;
    uint32_t state_total_bytes;
    int32_t indexed_cross_extent;
    int32_t *index_block_extents;
    int32_t *index_block_origins;
    uint8_t *index_block_valid;
    egui_dlist_t state_entries;

    egui_scroller_t scroller;

    egui_view_virtual_viewport_slot_t slots[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS];
};

/** One-shot parameter block for viewport geometry, scroll axis, and virtualization behavior. */
struct egui_view_virtual_viewport_params
{
    egui_region_t region;
    uint8_t orientation;
    uint8_t overscan_before;
    uint8_t overscan_after;
    uint8_t max_keepalive_slots;
    int32_t estimated_item_extent;
};

/** Resolved metadata for one item in the current data set. */
struct egui_view_virtual_viewport_entry
{
    uint32_t index;
    uint32_t stable_id;
    uint16_t view_type;
};

/** One-shot bundle that combines params, adapter hooks, and state-cache limits. */
struct egui_view_virtual_viewport_setup
{
    const egui_view_virtual_viewport_params_t *params;
    const egui_view_virtual_viewport_adapter_t *adapter;
    void *adapter_context;
    uint16_t state_cache_max_entries;
    uint32_t state_cache_max_bytes;
};

#define EGUI_VIEW_VIRTUAL_VIEWPORT_PARAMS_INIT(_name, _x, _y, _w, _h)                                                                                          \
    static const egui_view_virtual_viewport_params_t _name = {                                                                                                 \
            .region = {{(_x), (_y)}, {(_w), (_h)}},                                                                                                            \
            .orientation = EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_VERTICAL,                                                                                    \
            .overscan_before = 1,                                                                                                                              \
            .overscan_after = 1,                                                                                                                               \
            .max_keepalive_slots = 2,                                                                                                                          \
            .estimated_item_extent = 40,                                                                                                                       \
    }

#define EGUI_VIEW_VIRTUAL_VIEWPORT_SETUP_INIT(_name, _params, _adapter, _adapter_context)                                                                      \
    static const egui_view_virtual_viewport_setup_t _name = {                                                                                                  \
            .params = (_params),                                                                                                                               \
            .adapter = (_adapter),                                                                                                                             \
            .adapter_context = (_adapter_context),                                                                                                             \
            .state_cache_max_entries = 0,                                                                                                                      \
            .state_cache_max_bytes = 0,                                                                                                                        \
    }

/** Apply a viewport parameter block after initialization. */
void egui_view_virtual_viewport_apply_params(egui_view_t *self, const egui_view_virtual_viewport_params_t *params);
/** Initialize the viewport and immediately apply its parameter block. */
void egui_view_virtual_viewport_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_virtual_viewport_params_t *params);
/** Apply params, adapter hooks, and state-cache limits from one setup bundle. */
void egui_view_virtual_viewport_apply_setup(egui_view_t *self, const egui_view_virtual_viewport_setup_t *setup);
/** Initialize the viewport and immediately apply a setup bundle. */
void egui_view_virtual_viewport_init_with_setup(egui_view_t *self, egui_core_t *core, const egui_view_virtual_viewport_setup_t *setup);

/** Attach a raw adapter. Replacing it drops realized slots, measurement caches, and cached item state. */
void egui_view_virtual_viewport_set_adapter(egui_view_t *self, const egui_view_virtual_viewport_adapter_t *adapter, void *adapter_context);
/** Return the currently attached raw adapter, or NULL. */
const egui_view_virtual_viewport_adapter_t *egui_view_virtual_viewport_get_adapter(egui_view_t *self);
/** Return the opaque context pointer passed back to the active adapter. */
void *egui_view_virtual_viewport_get_adapter_context(egui_view_t *self);

/** Switch the main scrolling axis between vertical and horizontal mode. */
void egui_view_virtual_viewport_set_orientation(egui_view_t *self, uint8_t orientation);
/** Return the current main scrolling axis. */
uint8_t egui_view_virtual_viewport_get_orientation(egui_view_t *self);

/** Keep additional items realized before and after the visible range to reduce recycle churn while scrolling. */
void egui_view_virtual_viewport_set_overscan(egui_view_t *self, uint8_t before, uint8_t after);
/** Return the leading overscan item count. */
uint8_t egui_view_virtual_viewport_get_overscan_before(egui_view_t *self);
/** Return the trailing overscan item count. */
uint8_t egui_view_virtual_viewport_get_overscan_after(egui_view_t *self);

/** Limit how many off-screen slots may stay alive for quick rebinding. */
void egui_view_virtual_viewport_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots);
/** Return the current keepalive-slot limit. */
uint8_t egui_view_virtual_viewport_get_keepalive_limit(egui_view_t *self);

/** Configure the per-stable-id state cache. Both limits must be non-zero or the cache is disabled and cleared. */
void egui_view_virtual_viewport_set_state_cache_limits(egui_view_t *self, uint16_t max_entries, uint32_t max_bytes);
/** Return the maximum number of cached state entries. */
uint16_t egui_view_virtual_viewport_get_state_cache_entry_limit(egui_view_t *self);
/** Return the maximum total byte budget for cached state. */
uint32_t egui_view_virtual_viewport_get_state_cache_byte_limit(egui_view_t *self);
/** Drop all cached per-item state. */
void egui_view_virtual_viewport_clear_state_cache(egui_view_t *self);
/** Remove cached state for one stable item id. */
void egui_view_virtual_viewport_remove_state_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Store custom state bytes for one stable item id. Passing `size == 0` removes the entry. */
uint8_t egui_view_virtual_viewport_write_state(egui_view_t *self, uint32_t stable_id, const void *data, uint16_t size);
/** Read cached state for one stable item id. Returns the stored size even if `capacity` is smaller. */
uint16_t egui_view_virtual_viewport_read_state(egui_view_t *self, uint32_t stable_id, void *data, uint16_t capacity);
/** Convenience helper that stores state for the viewport item containing `item_view` or one of its descendants. */
uint8_t egui_view_virtual_viewport_write_state_for_view(egui_view_t *item_view, uint32_t stable_id, const void *data, uint16_t size);
/** Convenience helper that reads state for the viewport item containing `item_view` or one of its descendants. */
uint16_t egui_view_virtual_viewport_read_state_for_view(egui_view_t *item_view, uint32_t stable_id, void *data, uint16_t capacity);

/** Set the fallback main-axis size used before real measurements are known. */
void egui_view_virtual_viewport_set_estimated_item_extent(egui_view_t *self, int32_t extent);
/** Return the fallback main-axis size used by the viewport. */
int32_t egui_view_virtual_viewport_get_estimated_item_extent(egui_view_t *self);

/** Override the cached total logical extent and clamp the current scroll offset. */
void egui_view_virtual_viewport_set_logical_extent(egui_view_t *self, int32_t extent);
/** Return the cached total logical extent. */
int32_t egui_view_virtual_viewport_get_logical_extent(egui_view_t *self);

/** Jump the logical scroll offset to an absolute main-axis position. */
void egui_view_virtual_viewport_set_logical_offset(egui_view_t *self, int32_t offset);
/** Scroll by a signed main-axis delta. */
void egui_view_virtual_viewport_scroll_by(egui_view_t *self, int32_t delta);
/** Scroll so the target item starts near `-item_offset` inside the viewport. */
void egui_view_virtual_viewport_scroll_to_item(egui_view_t *self, uint32_t index, int32_t item_offset);
/** Scroll to an item by stable id instead of transient index. */
void egui_view_virtual_viewport_scroll_to_stable_id(egui_view_t *self, uint32_t stable_id, int32_t item_offset);
/** Return the current logical scroll offset. */
int32_t egui_view_virtual_viewport_get_logical_offset(egui_view_t *self);
/** Resolve a stable id back to the current item index. */
int32_t egui_view_virtual_viewport_find_item_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the main-axis origin of one item in logical coordinates. */
int32_t egui_view_virtual_viewport_get_item_main_origin(egui_view_t *self, uint32_t index);
/** Return the measured or estimated main-axis size of one item. */
int32_t egui_view_virtual_viewport_get_item_main_size(egui_view_t *self, uint32_t index);
/** Test whether the center of a logical span is inside the viewport. */
uint8_t egui_view_virtual_viewport_is_main_span_center_visible(egui_view_t *self, int32_t main_origin, int32_t main_size);
/** Test whether a logical span fits inside the viewport, allowing the given inset tolerance. */
uint8_t egui_view_virtual_viewport_is_main_span_fully_visible(egui_view_t *self, int32_t main_origin, int32_t main_size, int32_t inset);

/** Pin future data refreshes to a stable item plus an offset inside that item. */
void egui_view_virtual_viewport_set_anchor(egui_view_t *self, uint32_t stable_id, int32_t anchor_offset);
/** Return the currently tracked anchor stable id, or `INVALID_ID`. */
uint32_t egui_view_virtual_viewport_get_anchor_stable_id(egui_view_t *self);
/** Return the offset stored for the current anchor item. */
int32_t egui_view_virtual_viewport_get_anchor_offset(egui_view_t *self);

/** Tell the viewport that the whole data set changed and every visible item may need rebuilding. */
void egui_view_virtual_viewport_notify_data_changed(egui_view_t *self);
/** Tell the viewport that one item changed in place. */
void egui_view_virtual_viewport_notify_item_changed(egui_view_t *self, uint32_t index);
/** Notify one in-place item change by stable id. */
void egui_view_virtual_viewport_notify_item_changed_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Notify that items were inserted so cached measurements and bindings should be refreshed. */
void egui_view_virtual_viewport_notify_item_inserted(egui_view_t *self, uint32_t index, uint32_t count);
/** Notify that items were removed so cached measurements and bindings should be refreshed. */
void egui_view_virtual_viewport_notify_item_removed(egui_view_t *self, uint32_t index, uint32_t count);
/** Notify that one item moved to a new index. */
void egui_view_virtual_viewport_notify_item_moved(egui_view_t *self, uint32_t from_index, uint32_t to_index);
/** Notify that one item's measured main-axis size changed. */
void egui_view_virtual_viewport_notify_item_resized(egui_view_t *self, uint32_t index);
/** Notify that one item's measured main-axis size changed, addressed by stable id. */
void egui_view_virtual_viewport_notify_item_resized_by_stable_id(egui_view_t *self, uint32_t stable_id);

/** Return the internal group that owns realized item views. */
egui_view_t *egui_view_virtual_viewport_get_content_layer(egui_view_t *self);
/** Return the number of tracked slot records currently owned by the viewport. */
uint8_t egui_view_virtual_viewport_get_slot_count(egui_view_t *self);
/** Access one tracked slot. Check `slot->state` before using it. */
const egui_view_virtual_viewport_slot_t *egui_view_virtual_viewport_get_slot(egui_view_t *self, uint8_t slot_index);
/** Return the tracked slot index for an active stable id, or -1 if it is not realized. */
int32_t egui_view_virtual_viewport_find_slot_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the active slot for a stable id, or NULL if the item is not currently realized. */
const egui_view_virtual_viewport_slot_t *egui_view_virtual_viewport_find_slot_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the realized root view for a stable id, or NULL if the item is not currently realized. */
egui_view_t *egui_view_virtual_viewport_find_view_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Resolve a slot back to the current item index, stable id, and view type. */
uint8_t egui_view_virtual_viewport_get_slot_entry(egui_view_t *self, uint8_t slot_index, egui_view_virtual_viewport_entry_t *entry);
/** Resolve metadata for any existing stable id, even if the item is currently off-screen. */
uint8_t egui_view_virtual_viewport_resolve_item_by_stable_id(egui_view_t *self, uint32_t stable_id, egui_view_virtual_viewport_entry_t *entry);
/** Resolve metadata from a realized item root view or any descendant inside that item. */
uint8_t egui_view_virtual_viewport_resolve_item_by_view(egui_view_t *self, egui_view_t *item_view, egui_view_virtual_viewport_entry_t *entry);
/** Scroll if needed so the target stable id becomes visible within the requested inset tolerance. */
uint8_t egui_view_virtual_viewport_ensure_item_visible_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t inset);
/** Visit currently visible realized items. Returns the number of items passed to the visitor. */
uint8_t egui_view_virtual_viewport_visit_visible_items(egui_view_t *self, egui_view_virtual_viewport_visible_item_visitor_t visitor, void *context);
/** Return the lowest-index visible item that matches the predicate, optionally filling `entry_out`. */
egui_view_t *egui_view_virtual_viewport_find_first_visible_item_view(egui_view_t *self, egui_view_virtual_viewport_visible_item_matcher_t matcher,
                                                                     void *context, egui_view_virtual_viewport_entry_t *entry_out);
/** Test whether the center of a realized visible slot is inside the viewport. */
uint8_t egui_view_virtual_viewport_is_slot_center_visible(egui_view_t *self, const egui_view_virtual_viewport_slot_t *slot);
/** Test whether a realized visible slot fits inside the viewport, allowing the given inset tolerance. */
uint8_t egui_view_virtual_viewport_is_slot_fully_visible(egui_view_t *self, const egui_view_virtual_viewport_slot_t *slot, int32_t inset);

/** Initialize the generic virtual viewport widget. */
void egui_view_virtual_viewport_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_VIRTUAL_VIEWPORT_H_ */
