#ifndef _EGUI_VIEW_VIRTUAL_STRIP_H_
#define _EGUI_VIEW_VIRTUAL_STRIP_H_

#include "egui_view_virtual_viewport.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef egui_view_virtual_viewport_adapter_t egui_view_virtual_strip_adapter_t;
typedef egui_view_virtual_viewport_slot_t egui_view_virtual_strip_slot_t;

typedef struct egui_view_virtual_strip egui_view_virtual_strip_t;
typedef struct egui_view_virtual_strip_data_source egui_view_virtual_strip_data_source_t;
typedef struct egui_view_virtual_strip_entry egui_view_virtual_strip_entry_t;
typedef struct egui_view_virtual_strip_params egui_view_virtual_strip_params_t;
typedef struct egui_view_virtual_strip_setup egui_view_virtual_strip_setup_t;
/** Predicate used by `egui_view_virtual_strip_find_first_visible_item_view`. */
typedef uint8_t (*egui_view_virtual_strip_visible_item_matcher_t)(egui_view_t *self, const egui_view_virtual_strip_slot_t *slot,
                                                                  const egui_view_virtual_strip_entry_t *entry, egui_view_t *item_view, void *context);
/** Visitor used by `egui_view_virtual_strip_visit_visible_items`. Return 0 to stop early. */
typedef uint8_t (*egui_view_virtual_strip_visible_item_visitor_t)(egui_view_t *self, const egui_view_virtual_strip_slot_t *slot,
                                                                  const egui_view_virtual_strip_entry_t *entry, egui_view_t *item_view, void *context);

/** Horizontal strip wrapper around the generic virtual viewport. */
struct egui_view_virtual_strip
{
    egui_view_virtual_viewport_t base;
    const egui_view_virtual_strip_data_source_t *data_source;
    void *data_source_context;
    egui_view_virtual_strip_adapter_t data_source_adapter;
};

/** One-shot parameter block for strip geometry and virtualization behavior. */
struct egui_view_virtual_strip_params
{
    egui_region_t region;
    uint8_t overscan_before;
    uint8_t overscan_after;
    uint8_t max_keepalive_slots;
    int32_t estimated_item_width;
};

#define EGUI_VIEW_VIRTUAL_STRIP_PARAMS_INIT(_name, _x, _y, _w, _h)                                                                                             \
    static const egui_view_virtual_strip_params_t _name = {                                                                                                    \
            .region = {{(_x), (_y)}, {(_w), (_h)}},                                                                                                            \
            .overscan_before = 1,                                                                                                                              \
            .overscan_after = 1,                                                                                                                               \
            .max_keepalive_slots = 2,                                                                                                                          \
            .estimated_item_width = 56,                                                                                                                        \
    }

/** One-shot bundle that combines params, data-source hooks, and state-cache limits. */
struct egui_view_virtual_strip_setup
{
    const egui_view_virtual_strip_params_t *params;
    const egui_view_virtual_strip_data_source_t *data_source;
    void *data_source_context;
    uint16_t state_cache_max_entries;
    uint32_t state_cache_max_bytes;
};

#define EGUI_VIEW_VIRTUAL_STRIP_SETUP_INIT(_name, _params, _data_source, _data_source_context)                                                                 \
    static const egui_view_virtual_strip_setup_t _name = {                                                                                                     \
            .params = (_params),                                                                                                                               \
            .data_source = (_data_source),                                                                                                                     \
            .data_source_context = (_data_source_context),                                                                                                     \
            .state_cache_max_entries = 0,                                                                                                                      \
            .state_cache_max_bytes = 0,                                                                                                                        \
    }

/** Resolved metadata for one logical strip item. */
struct egui_view_virtual_strip_entry
{
    uint32_t index;
    uint32_t stable_id;
};

/** Strip-style data-source hooks that are bridged onto the generic virtual viewport adapter. */
struct egui_view_virtual_strip_data_source
{
    uint32_t (*get_count)(void *data_source_context);
    uint32_t (*get_stable_id)(void *data_source_context, uint32_t index);
    int32_t (*find_index_by_stable_id)(void *data_source_context, uint32_t stable_id);
    uint16_t (*get_item_view_type)(void *data_source_context, uint32_t index);
    int32_t (*measure_item_width)(void *data_source_context, uint32_t index, int32_t height_hint);
    egui_view_t *(*create_item_view)(void *data_source_context, uint16_t view_type);
    void (*destroy_item_view)(void *data_source_context, egui_view_t *view, uint16_t view_type);
    void (*bind_item_view)(void *data_source_context, egui_view_t *view, uint32_t index, uint32_t stable_id);
    void (*unbind_item_view)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    uint8_t (*should_keep_alive)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*save_item_state)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*restore_item_state)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    uint16_t default_item_view_type;
};

#define EGUI_VIEW_VIRTUAL_STRIP_DATA_SOURCE_INIT(_name)                                                                                                        \
    static const egui_view_virtual_strip_data_source_t _name = {                                                                                               \
            .get_count = NULL,                                                                                                                                 \
            .get_stable_id = NULL,                                                                                                                             \
            .find_index_by_stable_id = NULL,                                                                                                                   \
            .get_item_view_type = NULL,                                                                                                                        \
            .measure_item_width = NULL,                                                                                                                        \
            .create_item_view = NULL,                                                                                                                          \
            .destroy_item_view = NULL,                                                                                                                         \
            .bind_item_view = NULL,                                                                                                                            \
            .unbind_item_view = NULL,                                                                                                                          \
            .should_keep_alive = NULL,                                                                                                                         \
            .save_item_state = NULL,                                                                                                                           \
            .restore_item_state = NULL,                                                                                                                        \
            .default_item_view_type = 0,                                                                                                                       \
    }

/** Apply a strip parameter block after initialization. */
void egui_view_virtual_strip_apply_params(egui_view_t *self, const egui_view_virtual_strip_params_t *params);
/** Initialize the strip and immediately apply its parameter block. */
void egui_view_virtual_strip_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_virtual_strip_params_t *params);
/** Apply params, data-source hooks, and state-cache limits from one setup bundle. */
void egui_view_virtual_strip_apply_setup(egui_view_t *self, const egui_view_virtual_strip_setup_t *setup);
/** Initialize the strip and immediately apply a setup bundle. */
void egui_view_virtual_strip_init_with_setup(egui_view_t *self, egui_core_t *core, const egui_view_virtual_strip_setup_t *setup);

/** Attach a raw viewport adapter instead of a strip-style data source. This clears any current data source. */
void egui_view_virtual_strip_set_adapter(egui_view_t *self, const egui_view_virtual_strip_adapter_t *adapter, void *adapter_context);
/** Return the currently attached raw adapter, or NULL. */
const egui_view_virtual_strip_adapter_t *egui_view_virtual_strip_get_adapter(egui_view_t *self);
/** Return the opaque context pointer passed back to the active raw adapter. */
void *egui_view_virtual_strip_get_adapter_context(egui_view_t *self);

/** Attach a strip data source. This installs an internal viewport-adapter bridge. Pass NULL to detach it. */
void egui_view_virtual_strip_set_data_source(egui_view_t *self, const egui_view_virtual_strip_data_source_t *data_source, void *data_source_context);
/** Return the currently attached strip data source, or NULL. */
const egui_view_virtual_strip_data_source_t *egui_view_virtual_strip_get_data_source(egui_view_t *self);
/** Return the opaque context pointer passed to the active strip data source. */
void *egui_view_virtual_strip_get_data_source_context(egui_view_t *self);
/** Return the current item count reported by the active adapter or data source. */
uint32_t egui_view_virtual_strip_get_item_count(egui_view_t *self);

/** Keep additional items realized before and after the viewport to reduce recycle churn while scrolling. */
void egui_view_virtual_strip_set_overscan(egui_view_t *self, uint8_t before, uint8_t after);
/** Return the leading overscan item count. */
uint8_t egui_view_virtual_strip_get_overscan_before(egui_view_t *self);
/** Return the trailing overscan item count. */
uint8_t egui_view_virtual_strip_get_overscan_after(egui_view_t *self);

/** Set the fallback item width used before real measurements are known. */
void egui_view_virtual_strip_set_estimated_item_width(egui_view_t *self, int32_t width);
/** Return the fallback item width used by the virtual strip. */
int32_t egui_view_virtual_strip_get_estimated_item_width(egui_view_t *self);

/** Limit how many off-screen item views may stay alive for quick rebinding. */
void egui_view_virtual_strip_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots);
/** Return the current keepalive-view limit. */
uint8_t egui_view_virtual_strip_get_keepalive_limit(egui_view_t *self);
/** Configure the per-item state cache. Both limits must be non-zero or the cache is disabled and cleared. */
void egui_view_virtual_strip_set_state_cache_limits(egui_view_t *self, uint16_t max_entries, uint32_t max_bytes);
/** Return the maximum number of cached item-state entries. */
uint16_t egui_view_virtual_strip_get_state_cache_entry_limit(egui_view_t *self);
/** Return the maximum byte budget for cached item state. */
uint32_t egui_view_virtual_strip_get_state_cache_byte_limit(egui_view_t *self);
/** Drop all cached per-item state. */
void egui_view_virtual_strip_clear_item_state_cache(egui_view_t *self);
/** Remove cached state for one stable item id. */
void egui_view_virtual_strip_remove_item_state_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Store custom state bytes for one stable item id. Passing `size == 0` removes the entry. */
uint8_t egui_view_virtual_strip_write_item_state(egui_view_t *self, uint32_t stable_id, const void *data, uint16_t size);
/** Read cached state for one stable item id. Returns the stored size even if `capacity` is smaller. */
uint16_t egui_view_virtual_strip_read_item_state(egui_view_t *self, uint32_t stable_id, void *data, uint16_t capacity);
/** Convenience helper that stores state for the realized item root view passed by the data source. */
uint8_t egui_view_virtual_strip_write_item_state_for_view(egui_view_t *item_view, uint32_t stable_id, const void *data, uint16_t size);
/** Convenience helper that reads state for the realized item root view passed by the data source. */
uint16_t egui_view_virtual_strip_read_item_state_for_view(egui_view_t *item_view, uint32_t stable_id, void *data, uint16_t capacity);

/** Jump the horizontal scroll offset to an absolute X position. */
void egui_view_virtual_strip_set_scroll_x(egui_view_t *self, int32_t offset);
/** Scroll by a signed X delta. */
void egui_view_virtual_strip_scroll_by(egui_view_t *self, int32_t delta);
/** Scroll so the target item starts near `-item_offset` inside the viewport. */
void egui_view_virtual_strip_scroll_to_item(egui_view_t *self, uint32_t index, int32_t item_offset);
/** Scroll to an item by stable id instead of transient index. */
void egui_view_virtual_strip_scroll_to_stable_id(egui_view_t *self, uint32_t stable_id, int32_t item_offset);
/** Return the current horizontal scroll offset. */
int32_t egui_view_virtual_strip_get_scroll_x(egui_view_t *self);
/** Resolve a stable id back to the current item index. */
int32_t egui_view_virtual_strip_find_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Resolve item metadata for a stable id. */
uint8_t egui_view_virtual_strip_resolve_item_by_stable_id(egui_view_t *self, uint32_t stable_id, egui_view_virtual_strip_entry_t *entry);
/** Resolve item metadata from the realized item root view returned by the data source. */
uint8_t egui_view_virtual_strip_resolve_item_by_view(egui_view_t *self, egui_view_t *item_view, egui_view_virtual_strip_entry_t *entry);
/** Return the left X position of one item. */
int32_t egui_view_virtual_strip_get_item_x(egui_view_t *self, uint32_t index);
/** Return the measured or estimated width of one item. */
int32_t egui_view_virtual_strip_get_item_width(egui_view_t *self, uint32_t index);
/** Return the left X position of an item found by stable id. */
int32_t egui_view_virtual_strip_get_item_x_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the width of an item found by stable id. */
int32_t egui_view_virtual_strip_get_item_width_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Scroll if needed so the target stable id becomes visible within the requested inset tolerance. */
uint8_t egui_view_virtual_strip_ensure_item_visible_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t inset);

/** Tell the virtual strip that the whole data set changed and visible items may need rebuilding. */
void egui_view_virtual_strip_notify_data_changed(egui_view_t *self);
/** Tell the virtual strip that one item changed in place. */
void egui_view_virtual_strip_notify_item_changed(egui_view_t *self, uint32_t index);
/** Notify one in-place item change by stable id. */
void egui_view_virtual_strip_notify_item_changed_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Notify that items were inserted so cached measurements and bindings should be refreshed. */
void egui_view_virtual_strip_notify_item_inserted(egui_view_t *self, uint32_t index, uint32_t count);
/** Notify that items were removed so cached measurements and bindings should be refreshed. */
void egui_view_virtual_strip_notify_item_removed(egui_view_t *self, uint32_t index, uint32_t count);
/** Notify that one item moved to a new index. */
void egui_view_virtual_strip_notify_item_moved(egui_view_t *self, uint32_t from_index, uint32_t to_index);
/** Notify that one item's measured width changed. */
void egui_view_virtual_strip_notify_item_resized(egui_view_t *self, uint32_t index);
/** Notify that one item's measured width changed, addressed by stable id. */
void egui_view_virtual_strip_notify_item_resized_by_stable_id(egui_view_t *self, uint32_t stable_id);

/** Return the number of tracked slot records currently owned by the strip. */
uint8_t egui_view_virtual_strip_get_slot_count(egui_view_t *self);
/** Access one tracked slot. Check `slot->state` before using it. */
const egui_view_virtual_strip_slot_t *egui_view_virtual_strip_get_slot(egui_view_t *self, uint8_t slot_index);
/** Return the tracked slot index for an active stable id, or -1 if it is not realized. */
int32_t egui_view_virtual_strip_find_slot_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the active slot for a stable id, or NULL if the item is not currently realized. */
const egui_view_virtual_strip_slot_t *egui_view_virtual_strip_find_slot_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the realized root view for a stable id, or NULL if the item is not currently realized. */
egui_view_t *egui_view_virtual_strip_find_view_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Resolve a tracked slot back to the current item index and stable id. */
uint8_t egui_view_virtual_strip_get_slot_entry(egui_view_t *self, uint8_t slot_index, egui_view_virtual_strip_entry_t *entry);
/** Visit currently visible realized items. Returns the number of items passed to the visitor. */
uint8_t egui_view_virtual_strip_visit_visible_items(egui_view_t *self, egui_view_virtual_strip_visible_item_visitor_t visitor, void *context);
/** Return the lowest-index visible item that matches the predicate, optionally filling `entry_out`. */
egui_view_t *egui_view_virtual_strip_find_first_visible_item_view(egui_view_t *self, egui_view_virtual_strip_visible_item_matcher_t matcher, void *context,
                                                                  egui_view_virtual_strip_entry_t *entry_out);

/** Initialize the horizontal strip-style virtual viewport wrapper. */
void egui_view_virtual_strip_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_VIRTUAL_STRIP_H_ */
