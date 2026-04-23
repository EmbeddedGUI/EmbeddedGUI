#ifndef _EGUI_VIEW_VIRTUAL_GRID_H_
#define _EGUI_VIEW_VIRTUAL_GRID_H_

#include "egui_view_virtual_list.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_VIRTUAL_GRID_INVALID_INDEX 0xFFFFFFFFUL
#define EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS   6U

typedef egui_view_virtual_viewport_slot_t egui_view_virtual_grid_slot_t;

typedef struct egui_view_virtual_grid egui_view_virtual_grid_t;
typedef struct egui_view_virtual_grid_data_source egui_view_virtual_grid_data_source_t;
typedef struct egui_view_virtual_grid_entry egui_view_virtual_grid_entry_t;
typedef struct egui_view_virtual_grid_params egui_view_virtual_grid_params_t;
typedef struct egui_view_virtual_grid_setup egui_view_virtual_grid_setup_t;
/** Predicate used by `egui_view_virtual_grid_find_first_visible_item_view`. */
typedef uint8_t (*egui_view_virtual_grid_visible_item_matcher_t)(egui_view_t *self, const egui_view_virtual_grid_slot_t *slot,
                                                                 const egui_view_virtual_grid_entry_t *entry, egui_view_t *item_view, void *context);
/** Visitor used by `egui_view_virtual_grid_visit_visible_items`. Return 0 to stop early. */
typedef uint8_t (*egui_view_virtual_grid_visible_item_visitor_t)(egui_view_t *self, const egui_view_virtual_grid_slot_t *slot,
                                                                 const egui_view_virtual_grid_entry_t *entry, egui_view_t *item_view, void *context);

/** Grid wrapper that virtualizes rows while exposing item-level helpers for each visible cell. */
struct egui_view_virtual_grid
{
    egui_view_virtual_list_t base;
    const egui_view_virtual_grid_data_source_t *data_source;
    void *data_source_context;
    egui_view_virtual_list_data_source_t row_data_source;
    uint8_t column_count;
    egui_dim_t column_spacing;
    egui_dim_t row_spacing;
    int32_t estimated_item_height;
};

/** One-shot parameter block for grid geometry and virtualization behavior. */
struct egui_view_virtual_grid_params
{
    egui_region_t region;
    uint8_t column_count;
    uint8_t overscan_before;
    uint8_t overscan_after;
    uint8_t max_keepalive_slots;
    egui_dim_t column_spacing;
    egui_dim_t row_spacing;
    int32_t estimated_item_height;
};

/** Resolved metadata for one logical grid item. */
struct egui_view_virtual_grid_entry
{
    uint32_t index;
    uint32_t row_index;
    uint8_t column_index;
    uint32_t stable_id;
};

#define EGUI_VIEW_VIRTUAL_GRID_PARAMS_INIT(_name, _x, _y, _w, _h)                                                                                              \
    static const egui_view_virtual_grid_params_t _name = {                                                                                                     \
            .region = {{(_x), (_y)}, {(_w), (_h)}},                                                                                                            \
            .column_count = 2,                                                                                                                                 \
            .overscan_before = 1,                                                                                                                              \
            .overscan_after = 1,                                                                                                                               \
            .max_keepalive_slots = 2,                                                                                                                          \
            .column_spacing = 6,                                                                                                                               \
            .row_spacing = 6,                                                                                                                                  \
            .estimated_item_height = 64,                                                                                                                       \
    }

/** One-shot bundle that combines params, data-source hooks, and state-cache limits. */
struct egui_view_virtual_grid_setup
{
    const egui_view_virtual_grid_params_t *params;
    const egui_view_virtual_grid_data_source_t *data_source;
    void *data_source_context;
    uint16_t state_cache_max_entries;
    uint32_t state_cache_max_bytes;
};

#define EGUI_VIEW_VIRTUAL_GRID_SETUP_INIT(_name, _params, _data_source, _data_source_context)                                                                  \
    static const egui_view_virtual_grid_setup_t _name = {                                                                                                      \
            .params = (_params),                                                                                                                               \
            .data_source = (_data_source),                                                                                                                     \
            .data_source_context = (_data_source_context),                                                                                                     \
            .state_cache_max_entries = 0,                                                                                                                      \
            .state_cache_max_bytes = 0,                                                                                                                        \
    }

/** Item-level grid hooks. Internally the grid bridges them onto a row-based virtual list. */
struct egui_view_virtual_grid_data_source
{
    uint32_t (*get_count)(void *data_source_context);
    uint32_t (*get_stable_id)(void *data_source_context, uint32_t index);
    int32_t (*find_index_by_stable_id)(void *data_source_context, uint32_t stable_id);
    uint16_t (*get_item_view_type)(void *data_source_context, uint32_t index);
    int32_t (*measure_item_height)(void *data_source_context, uint32_t index, int32_t width_hint);
    egui_view_t *(*create_item_view)(void *data_source_context, uint16_t view_type);
    void (*destroy_item_view)(void *data_source_context, egui_view_t *view, uint16_t view_type);
    void (*bind_item_view)(void *data_source_context, egui_view_t *view, uint32_t index, uint32_t stable_id);
    void (*unbind_item_view)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    uint8_t (*should_keep_alive)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*save_item_state)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*restore_item_state)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    uint16_t default_item_view_type;
};

#define EGUI_VIEW_VIRTUAL_GRID_DATA_SOURCE_INIT(_name)                                                                                                         \
    static const egui_view_virtual_grid_data_source_t _name = {                                                                                                \
            .get_count = NULL,                                                                                                                                 \
            .get_stable_id = NULL,                                                                                                                             \
            .find_index_by_stable_id = NULL,                                                                                                                   \
            .get_item_view_type = NULL,                                                                                                                        \
            .measure_item_height = NULL,                                                                                                                       \
            .create_item_view = NULL,                                                                                                                          \
            .destroy_item_view = NULL,                                                                                                                         \
            .bind_item_view = NULL,                                                                                                                            \
            .unbind_item_view = NULL,                                                                                                                          \
            .should_keep_alive = NULL,                                                                                                                         \
            .save_item_state = NULL,                                                                                                                           \
            .restore_item_state = NULL,                                                                                                                        \
            .default_item_view_type = 0,                                                                                                                       \
    }

/** Apply a grid parameter block after initialization. */
void egui_view_virtual_grid_apply_params(egui_view_t *self, const egui_view_virtual_grid_params_t *params);
/** Initialize the grid and immediately apply its parameter block. */
void egui_view_virtual_grid_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_virtual_grid_params_t *params);
/** Apply params, data-source hooks, and state-cache limits from one setup bundle. */
void egui_view_virtual_grid_apply_setup(egui_view_t *self, const egui_view_virtual_grid_setup_t *setup);
/** Initialize the grid and immediately apply a setup bundle. */
void egui_view_virtual_grid_init_with_setup(egui_view_t *self, egui_core_t *core, const egui_view_virtual_grid_setup_t *setup);

/** Attach the item-level data source used by the grid. Pass NULL to detach it. */
void egui_view_virtual_grid_set_data_source(egui_view_t *self, const egui_view_virtual_grid_data_source_t *data_source, void *data_source_context);
/** Return the currently attached grid data source, or NULL. */
const egui_view_virtual_grid_data_source_t *egui_view_virtual_grid_get_data_source(egui_view_t *self);
/** Return the opaque context pointer passed to the active grid data source. */
void *egui_view_virtual_grid_get_data_source_context(egui_view_t *self);

/** Return the current item count reported by the grid data source. */
uint32_t egui_view_virtual_grid_get_item_count(egui_view_t *self);
/** Return the current logical row count derived from item count and column count. */
uint32_t egui_view_virtual_grid_get_row_count(egui_view_t *self);

/** Keep additional rows realized before and after the viewport to reduce recycle churn while scrolling. */
void egui_view_virtual_grid_set_overscan(egui_view_t *self, uint8_t before, uint8_t after);
/** Return the leading overscan row count. */
uint8_t egui_view_virtual_grid_get_overscan_before(egui_view_t *self);
/** Return the trailing overscan row count. */
uint8_t egui_view_virtual_grid_get_overscan_after(egui_view_t *self);

/** Set the column count. Values are clamped to `1..EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS`. */
void egui_view_virtual_grid_set_column_count(egui_view_t *self, uint8_t column_count);
/** Return the sanitized column count currently used by the grid. */
uint8_t egui_view_virtual_grid_get_column_count(egui_view_t *self);

/** Set inter-column and inter-row spacing. Negative values are clamped to zero. */
void egui_view_virtual_grid_set_spacing(egui_view_t *self, egui_dim_t column_spacing, egui_dim_t row_spacing);
/** Return the sanitized spacing between columns. */
egui_dim_t egui_view_virtual_grid_get_column_spacing(egui_view_t *self);
/** Return the sanitized spacing between rows. */
egui_dim_t egui_view_virtual_grid_get_row_spacing(egui_view_t *self);

/** Set the fallback item height used before real measurements are known. */
void egui_view_virtual_grid_set_estimated_item_height(egui_view_t *self, int32_t height);
/** Return the fallback item height used by the grid. */
int32_t egui_view_virtual_grid_get_estimated_item_height(egui_view_t *self);

/** Limit how many off-screen row slots may stay alive for quick rebinding. */
void egui_view_virtual_grid_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots);
/** Return the current keepalive-row limit. */
uint8_t egui_view_virtual_grid_get_keepalive_limit(egui_view_t *self);

/** Configure the per-item state cache. Both limits must be non-zero or the cache is disabled and cleared. */
void egui_view_virtual_grid_set_state_cache_limits(egui_view_t *self, uint16_t max_entries, uint32_t max_bytes);
/** Return the maximum number of cached item-state entries. */
uint16_t egui_view_virtual_grid_get_state_cache_entry_limit(egui_view_t *self);
/** Return the maximum byte budget for cached item state. */
uint32_t egui_view_virtual_grid_get_state_cache_byte_limit(egui_view_t *self);
/** Drop all cached per-item state. */
void egui_view_virtual_grid_clear_item_state_cache(egui_view_t *self);
/** Remove cached state for one stable item id. */
void egui_view_virtual_grid_remove_item_state_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Store custom state bytes for one stable item id. Passing `size == 0` removes the entry. */
uint8_t egui_view_virtual_grid_write_item_state(egui_view_t *self, uint32_t stable_id, const void *data, uint16_t size);
/** Read cached state for one stable item id. Returns the stored size even if `capacity` is smaller. */
uint16_t egui_view_virtual_grid_read_item_state(egui_view_t *self, uint32_t stable_id, void *data, uint16_t capacity);
/** Convenience helper that stores state for the realized item root view returned by the data source. */
uint8_t egui_view_virtual_grid_write_item_state_for_view(egui_view_t *item_view, uint32_t stable_id, const void *data, uint16_t size);
/** Convenience helper that reads state for the realized item root view returned by the data source. */
uint16_t egui_view_virtual_grid_read_item_state_for_view(egui_view_t *item_view, uint32_t stable_id, void *data, uint16_t capacity);

/** Jump the vertical scroll offset to an absolute Y position. */
void egui_view_virtual_grid_set_scroll_y(egui_view_t *self, int32_t offset);
/** Scroll by a signed Y delta. */
void egui_view_virtual_grid_scroll_by(egui_view_t *self, int32_t delta);
/** Scroll to the row containing the given item index. */
void egui_view_virtual_grid_scroll_to_item(egui_view_t *self, uint32_t index, int32_t item_offset);
/** Scroll to the row containing the given stable item id. */
void egui_view_virtual_grid_scroll_to_item_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t item_offset);
/** Return the current vertical scroll offset. */
int32_t egui_view_virtual_grid_get_scroll_y(egui_view_t *self);

/** Resolve a stable id back to the current item index. */
int32_t egui_view_virtual_grid_find_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Resolve item metadata for a stable id. */
uint8_t egui_view_virtual_grid_resolve_item_by_stable_id(egui_view_t *self, uint32_t stable_id, egui_view_virtual_grid_entry_t *entry);
/** Resolve item metadata from the realized item root view returned by the data source. */
uint8_t egui_view_virtual_grid_resolve_item_by_view(egui_view_t *self, egui_view_t *item_view, egui_view_virtual_grid_entry_t *entry);

/** Return the left X position of one item inside the grid. */
int32_t egui_view_virtual_grid_get_item_x(egui_view_t *self, uint32_t index);
/** Return the top Y position of one item's row. */
int32_t egui_view_virtual_grid_get_item_y(egui_view_t *self, uint32_t index);
/** Return the computed width of one item's column. */
int32_t egui_view_virtual_grid_get_item_width(egui_view_t *self, uint32_t index);
/** Return the measured or estimated height of one item. */
int32_t egui_view_virtual_grid_get_item_height(egui_view_t *self, uint32_t index);
/** Return the left X position of an item found by stable id. */
int32_t egui_view_virtual_grid_get_item_x_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the top Y position of an item found by stable id. */
int32_t egui_view_virtual_grid_get_item_y_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the computed width of an item found by stable id. */
int32_t egui_view_virtual_grid_get_item_width_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the height of an item found by stable id. */
int32_t egui_view_virtual_grid_get_item_height_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Scroll if needed so the target stable id becomes visible within the requested inset tolerance. */
uint8_t egui_view_virtual_grid_ensure_item_visible_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t inset);
/** Return the realized root view for a stable id, or NULL if the item is not currently realized. */
egui_view_t *egui_view_virtual_grid_find_view_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the number of tracked row slots currently owned by the grid. */
uint8_t egui_view_virtual_grid_get_slot_count(egui_view_t *self);
/** Access one tracked row slot. Check `slot->state` before using it. */
const egui_view_virtual_grid_slot_t *egui_view_virtual_grid_get_slot(egui_view_t *self, uint8_t slot_index);
/** Return the row-slot index that currently contains the target stable id, or -1. */
int32_t egui_view_virtual_grid_find_slot_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the active row slot that currently contains the target stable id, or NULL. */
const egui_view_virtual_grid_slot_t *egui_view_virtual_grid_find_slot_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return how many bound item views currently live inside one realized row slot. */
uint8_t egui_view_virtual_grid_get_slot_item_count(egui_view_t *self, uint8_t slot_index);
/** Resolve one item entry inside a realized row slot. */
uint8_t egui_view_virtual_grid_get_slot_entry(egui_view_t *self, uint8_t slot_index, uint8_t column_index, egui_view_virtual_grid_entry_t *entry);
/** Return the realized item view stored at one column inside a row slot, or NULL. */
egui_view_t *egui_view_virtual_grid_get_slot_item_view(egui_view_t *self, uint8_t slot_index, uint8_t column_index);
/** Visit currently visible realized items. Returns the number of items passed to the visitor. */
uint8_t egui_view_virtual_grid_visit_visible_items(egui_view_t *self, egui_view_virtual_grid_visible_item_visitor_t visitor, void *context);
/** Return the lowest-index visible item that matches the predicate, optionally filling `entry_out`. */
egui_view_t *egui_view_virtual_grid_find_first_visible_item_view(egui_view_t *self, egui_view_virtual_grid_visible_item_matcher_t matcher, void *context,
                                                                 egui_view_virtual_grid_entry_t *entry_out);

/** Tell the grid that the whole data set changed and row mapping should be rebuilt. */
void egui_view_virtual_grid_notify_data_changed(egui_view_t *self);
/** Tell the grid that one item changed in place. The row containing it is invalidated. */
void egui_view_virtual_grid_notify_item_changed(egui_view_t *self, uint32_t index);
/** Notify one in-place item change by stable id. The containing row is invalidated. */
void egui_view_virtual_grid_notify_item_changed_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Notify that items were inserted so row mapping and cached measurements should be refreshed. */
void egui_view_virtual_grid_notify_item_inserted(egui_view_t *self, uint32_t index, uint32_t count);
/** Notify that items were removed so row mapping and cached measurements should be refreshed. */
void egui_view_virtual_grid_notify_item_removed(egui_view_t *self, uint32_t index, uint32_t count);
/** Notify that one item moved to a new index. Row mapping is rebuilt. */
void egui_view_virtual_grid_notify_item_moved(egui_view_t *self, uint32_t from_index, uint32_t to_index);
/** Notify that one item's measured height changed. The row containing it is invalidated. */
void egui_view_virtual_grid_notify_item_resized(egui_view_t *self, uint32_t index);
/** Notify that one item's measured height changed, addressed by stable id. */
void egui_view_virtual_grid_notify_item_resized_by_stable_id(egui_view_t *self, uint32_t stable_id);

/** Initialize the grid-style virtual viewport wrapper. */
void egui_view_virtual_grid_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_VIRTUAL_GRID_H_ */
