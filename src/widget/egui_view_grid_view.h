#ifndef _EGUI_VIEW_GRID_VIEW_H_
#define _EGUI_VIEW_GRID_VIEW_H_

#include "egui_view_virtual_grid.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef egui_view_virtual_grid_entry_t egui_view_grid_view_entry_t;
typedef egui_view_virtual_grid_params_t egui_view_grid_view_params_t;

#define EGUI_VIEW_GRID_VIEW_PARAMS_INIT(_name, _x, _y, _w, _h) EGUI_VIEW_VIRTUAL_GRID_PARAMS_INIT(_name, _x, _y, _w, _h)

typedef struct egui_view_grid_view egui_view_grid_view_t;
typedef struct egui_view_grid_view_data_model egui_view_grid_view_data_model_t;
typedef struct egui_view_grid_view_holder egui_view_grid_view_holder_t;
typedef struct egui_view_grid_view_holder_ops egui_view_grid_view_holder_ops_t;
typedef struct egui_view_grid_view_setup egui_view_grid_view_setup_t;

/** Reusable holder returned by user code for one realized grid item. */
struct egui_view_grid_view_holder
{
    egui_view_t *item_view;
    egui_view_t *host_view;
    uint16_t view_type;
    uint32_t bound_index;
    uint32_t bound_stable_id;
};

/** Data-model hooks used to expose arbitrary item data to the virtual grid core. */
struct egui_view_grid_view_data_model
{
    uint32_t (*get_count)(void *data_model_context);
    uint32_t (*get_stable_id)(void *data_model_context, uint32_t index);
    int32_t (*find_index_by_stable_id)(void *data_model_context, uint32_t stable_id);
    uint16_t (*get_view_type)(void *data_model_context, uint32_t index);
    int32_t (*measure_item_height)(void *data_model_context, uint32_t index, int32_t width_hint);
    uint16_t default_view_type;
};

#define EGUI_VIEW_GRID_VIEW_DATA_MODEL_INIT(_name)                                                                                                             \
    static const egui_view_grid_view_data_model_t _name = {                                                                                                    \
            .get_count = NULL,                                                                                                                                 \
            .get_stable_id = NULL,                                                                                                                             \
            .find_index_by_stable_id = NULL,                                                                                                                   \
            .get_view_type = NULL,                                                                                                                             \
            .measure_item_height = NULL,                                                                                                                       \
            .default_view_type = 0,                                                                                                                            \
    }

/** Lifecycle hooks used to create, bind, recycle, and persist per-item views. */
struct egui_view_grid_view_holder_ops
{
    egui_view_grid_view_holder_t *(*create_holder)(void *data_model_context, uint16_t view_type);
    void (*destroy_holder)(void *data_model_context, egui_view_grid_view_holder_t *holder, uint16_t view_type);
    void (*bind_holder)(void *data_model_context, egui_view_grid_view_holder_t *holder, uint32_t index, uint32_t stable_id);
    void (*unbind_holder)(void *data_model_context, egui_view_grid_view_holder_t *holder, uint32_t stable_id);
    uint8_t (*should_keep_alive)(void *data_model_context, egui_view_grid_view_holder_t *holder, uint32_t stable_id);
    void (*save_holder_state)(void *data_model_context, egui_view_grid_view_holder_t *holder, uint32_t stable_id);
    void (*restore_holder_state)(void *data_model_context, egui_view_grid_view_holder_t *holder, uint32_t stable_id);
};

#define EGUI_VIEW_GRID_VIEW_HOLDER_OPS_INIT(_name)                                                                                                             \
    static const egui_view_grid_view_holder_ops_t _name = {                                                                                                    \
            .create_holder = NULL,                                                                                                                             \
            .destroy_holder = NULL,                                                                                                                            \
            .bind_holder = NULL,                                                                                                                               \
            .unbind_holder = NULL,                                                                                                                             \
            .should_keep_alive = NULL,                                                                                                                         \
            .save_holder_state = NULL,                                                                                                                         \
            .restore_holder_state = NULL,                                                                                                                      \
    }

struct egui_view_grid_view
{
    egui_view_virtual_grid_t base;
    const egui_view_grid_view_data_model_t *data_model;
    const egui_view_grid_view_holder_ops_t *holder_ops;
    void *data_model_context;
    egui_view_virtual_grid_data_source_t bridge_data_source;
};

/** One-shot configuration bundle for initializing a grid view. */
struct egui_view_grid_view_setup
{
    const egui_view_grid_view_params_t *params;
    const egui_view_grid_view_data_model_t *data_model;
    const egui_view_grid_view_holder_ops_t *holder_ops;
    void *data_model_context;
    uint16_t state_cache_max_entries;
    uint32_t state_cache_max_bytes;
};

#define EGUI_VIEW_GRID_VIEW_SETUP_INIT(_name, _params, _data_model, _holder_ops, _data_model_context)                                                          \
    static const egui_view_grid_view_setup_t _name = {                                                                                                         \
            .params = (_params),                                                                                                                               \
            .data_model = (_data_model),                                                                                                                       \
            .holder_ops = (_holder_ops),                                                                                                                       \
            .data_model_context = (_data_model_context),                                                                                                       \
            .state_cache_max_entries = 0,                                                                                                                      \
            .state_cache_max_bytes = 0,                                                                                                                        \
    }

/** Apply the underlying virtual-grid parameter block after initialization. */
void egui_view_grid_view_apply_params(egui_view_t *self, const egui_view_grid_view_params_t *params);
/** Initialize a grid view and immediately apply its parameter block. */
void egui_view_grid_view_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_grid_view_params_t *params);
/** Apply params, model hooks, and state-cache limits from one setup bundle. */
void egui_view_grid_view_apply_setup(egui_view_t *self, const egui_view_grid_view_setup_t *setup);
/** Initialize a grid view and immediately apply a setup bundle. */
void egui_view_grid_view_init_with_setup(egui_view_t *self, egui_core_t *core, const egui_view_grid_view_setup_t *setup);

/** Attach the data model and holder factory used to realize visible cells. Pass NULL to detach the model. */
void egui_view_grid_view_set_data_model(egui_view_t *self, const egui_view_grid_view_data_model_t *data_model,
                                        const egui_view_grid_view_holder_ops_t *holder_ops, void *data_model_context);
/** Return the currently attached data-model descriptor. */
const egui_view_grid_view_data_model_t *egui_view_grid_view_get_data_model(egui_view_t *self);
/** Return the currently attached holder-ops table. */
const egui_view_grid_view_holder_ops_t *egui_view_grid_view_get_holder_ops(egui_view_t *self);
/** Return the opaque context pointer passed to the active data model. */
void *egui_view_grid_view_get_data_model_context(egui_view_t *self);
/** Return the current item count reported by the data model. */
uint32_t egui_view_grid_view_get_item_count(egui_view_t *self);
/** Return the row count implied by the current item count and column count. */
uint32_t egui_view_grid_view_get_row_count(egui_view_t *self);

/** Keep additional rows realized before and after the viewport to reduce recycle churn while scrolling. */
void egui_view_grid_view_set_overscan(egui_view_t *self, uint8_t before, uint8_t after);
/** Return the leading overscan row count. */
uint8_t egui_view_grid_view_get_overscan_before(egui_view_t *self);
/** Return the trailing overscan row count. */
uint8_t egui_view_grid_view_get_overscan_after(egui_view_t *self);

/** Set how many columns are used to divide the viewport width into cells. */
void egui_view_grid_view_set_column_count(egui_view_t *self, uint8_t column_count);
/** Return the current column count. */
uint8_t egui_view_grid_view_get_column_count(egui_view_t *self);

/** Set the spacing between columns and rows. */
void egui_view_grid_view_set_spacing(egui_view_t *self, egui_dim_t column_spacing, egui_dim_t row_spacing);
/** Return the current column spacing. */
egui_dim_t egui_view_grid_view_get_column_spacing(egui_view_t *self);
/** Return the current row spacing. */
egui_dim_t egui_view_grid_view_get_row_spacing(egui_view_t *self);

/** Set the fallback item height used before real measurements are known. */
void egui_view_grid_view_set_estimated_item_height(egui_view_t *self, int32_t height);
/** Return the fallback item height used by the virtual grid core. */
int32_t egui_view_grid_view_get_estimated_item_height(egui_view_t *self);

/** Limit how many off-screen holders may stay alive for fast reuse. */
void egui_view_grid_view_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots);
/** Return the current keepalive-holder limit. */
uint8_t egui_view_grid_view_get_keepalive_limit(egui_view_t *self);

/** Configure the per-item state cache used across holder recycling. */
void egui_view_grid_view_set_state_cache_limits(egui_view_t *self, uint16_t max_entries, uint32_t max_bytes);
/** Return the maximum number of cached state entries. */
uint16_t egui_view_grid_view_get_state_cache_entry_limit(egui_view_t *self);
/** Return the maximum number of bytes reserved for cached item state. */
uint32_t egui_view_grid_view_get_state_cache_byte_limit(egui_view_t *self);
/** Drop all cached item state. Use this when the stored state schema changes. */
void egui_view_grid_view_clear_item_state_cache(egui_view_t *self);
/** Remove cached state for one stable item id. */
void egui_view_grid_view_remove_item_state_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Store custom state bytes for one stable item id. */
uint8_t egui_view_grid_view_write_item_state(egui_view_t *self, uint32_t stable_id, const void *data, uint16_t size);
/** Read cached state bytes for one stable item id. Returns the number of bytes copied. */
uint16_t egui_view_grid_view_read_item_state(egui_view_t *self, uint32_t stable_id, void *data, uint16_t capacity);
/** Convenience helper that stores state for the grid item containing `item_view`. */
uint8_t egui_view_grid_view_write_item_state_for_view(egui_view_t *item_view, uint32_t stable_id, const void *data, uint16_t size);
/** Convenience helper that reads state for the grid item containing `item_view`. */
uint16_t egui_view_grid_view_read_item_state_for_view(egui_view_t *item_view, uint32_t stable_id, void *data, uint16_t capacity);

/** Jump the vertical scroll offset to an absolute Y position. */
void egui_view_grid_view_set_scroll_y(egui_view_t *self, int32_t offset);
/** Scroll by a signed Y delta. */
void egui_view_grid_view_scroll_by(egui_view_t *self, int32_t delta);
/** Scroll so the target index appears at `item_offset` inside the viewport. */
void egui_view_grid_view_scroll_to_item(egui_view_t *self, uint32_t index, int32_t item_offset);
/** Scroll to an item by stable id instead of transient index. */
void egui_view_grid_view_scroll_to_stable_id(egui_view_t *self, uint32_t stable_id, int32_t item_offset);
/** Return the current vertical scroll offset. */
int32_t egui_view_grid_view_get_scroll_y(egui_view_t *self);
/** Resolve a stable id back to the current item index. */
int32_t egui_view_grid_view_find_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Resolve item metadata for a stable id. */
uint8_t egui_view_grid_view_resolve_item_by_stable_id(egui_view_t *self, uint32_t stable_id, egui_view_grid_view_entry_t *entry);
/** Resolve item metadata by walking up from any child view inside a realized cell. */
uint8_t egui_view_grid_view_resolve_item_by_view(egui_view_t *self, egui_view_t *item_view, egui_view_grid_view_entry_t *entry);
/** Return the X position of one item. */
int32_t egui_view_grid_view_get_item_x(egui_view_t *self, uint32_t index);
/** Return the Y position of one item. */
int32_t egui_view_grid_view_get_item_y(egui_view_t *self, uint32_t index);
/** Return the measured or estimated width of one item. */
int32_t egui_view_grid_view_get_item_width(egui_view_t *self, uint32_t index);
/** Return the measured or estimated height of one item. */
int32_t egui_view_grid_view_get_item_height(egui_view_t *self, uint32_t index);
/** Return the X position of an item found by stable id. */
int32_t egui_view_grid_view_get_item_x_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the Y position of an item found by stable id. */
int32_t egui_view_grid_view_get_item_y_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the width of an item found by stable id. */
int32_t egui_view_grid_view_get_item_width_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the height of an item found by stable id. */
int32_t egui_view_grid_view_get_item_height_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Scroll just enough to keep the target stable id visible with the given inset. */
uint8_t egui_view_grid_view_ensure_item_visible_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t inset);

/** Tell the virtual grid that the whole data set changed. */
void egui_view_grid_view_notify_data_changed(egui_view_t *self);
/** Tell the virtual grid that one item changed in place. */
void egui_view_grid_view_notify_item_changed(egui_view_t *self, uint32_t index);
/** Notify one in-place item change by stable id. */
void egui_view_grid_view_notify_item_changed_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Notify that `count` items were inserted starting at `index`. */
void egui_view_grid_view_notify_item_inserted(egui_view_t *self, uint32_t index, uint32_t count);
/** Notify that `count` items were removed starting at `index`. */
void egui_view_grid_view_notify_item_removed(egui_view_t *self, uint32_t index, uint32_t count);
/** Notify that one item moved to a new index. */
void egui_view_grid_view_notify_item_moved(egui_view_t *self, uint32_t from_index, uint32_t to_index);
/** Notify that one item's measured height changed. */
void egui_view_grid_view_notify_item_resized(egui_view_t *self, uint32_t index);
/** Notify that one item's measured height changed, addressed by stable id. */
void egui_view_grid_view_notify_item_resized_by_stable_id(egui_view_t *self, uint32_t stable_id);

/** Return the currently realized holder for a stable id, or NULL if the item is not active. */
egui_view_grid_view_holder_t *egui_view_grid_view_find_holder_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Return the currently realized item view for a stable id, or NULL if the item is not active. */
egui_view_t *egui_view_grid_view_find_item_view_by_stable_id(egui_view_t *self, uint32_t stable_id);
/** Resolve both holder and entry information from any descendant view inside a realized cell. */
uint8_t egui_view_grid_view_resolve_holder_by_view(egui_view_t *self, egui_view_t *item_view, egui_view_grid_view_holder_t **holder_out,
                                                   egui_view_grid_view_entry_t *entry_out);

/** Initialize the holder-based virtual grid adapter. */
void egui_view_grid_view_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_GRID_VIEW_H_ */
