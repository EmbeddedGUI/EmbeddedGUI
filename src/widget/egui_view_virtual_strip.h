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
typedef uint8_t (*egui_view_virtual_strip_visible_item_matcher_t)(egui_view_t *self, const egui_view_virtual_strip_slot_t *slot,
                                                                  const egui_view_virtual_strip_entry_t *entry, egui_view_t *item_view, void *context);
typedef uint8_t (*egui_view_virtual_strip_visible_item_visitor_t)(egui_view_t *self, const egui_view_virtual_strip_slot_t *slot,
                                                                  const egui_view_virtual_strip_entry_t *entry, egui_view_t *item_view, void *context);

struct egui_view_virtual_strip
{
    egui_view_virtual_viewport_t base;
    const egui_view_virtual_strip_data_source_t *data_source;
    void *data_source_context;
    egui_view_virtual_strip_adapter_t data_source_adapter;
};

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

struct egui_view_virtual_strip_entry
{
    uint32_t index;
    uint32_t stable_id;
};

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

void egui_view_virtual_strip_apply_params(egui_view_t *self, const egui_view_virtual_strip_params_t *params);
void egui_view_virtual_strip_init_with_params(egui_view_t *self, const egui_view_virtual_strip_params_t *params);
void egui_view_virtual_strip_apply_setup(egui_view_t *self, const egui_view_virtual_strip_setup_t *setup);
void egui_view_virtual_strip_init_with_setup(egui_view_t *self, const egui_view_virtual_strip_setup_t *setup);

void egui_view_virtual_strip_set_adapter(egui_view_t *self, const egui_view_virtual_strip_adapter_t *adapter, void *adapter_context);
const egui_view_virtual_strip_adapter_t *egui_view_virtual_strip_get_adapter(egui_view_t *self);
void *egui_view_virtual_strip_get_adapter_context(egui_view_t *self);

void egui_view_virtual_strip_set_data_source(egui_view_t *self, const egui_view_virtual_strip_data_source_t *data_source, void *data_source_context);
const egui_view_virtual_strip_data_source_t *egui_view_virtual_strip_get_data_source(egui_view_t *self);
void *egui_view_virtual_strip_get_data_source_context(egui_view_t *self);
uint32_t egui_view_virtual_strip_get_item_count(egui_view_t *self);

void egui_view_virtual_strip_set_overscan(egui_view_t *self, uint8_t before, uint8_t after);
uint8_t egui_view_virtual_strip_get_overscan_before(egui_view_t *self);
uint8_t egui_view_virtual_strip_get_overscan_after(egui_view_t *self);

void egui_view_virtual_strip_set_estimated_item_width(egui_view_t *self, int32_t width);
int32_t egui_view_virtual_strip_get_estimated_item_width(egui_view_t *self);

void egui_view_virtual_strip_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots);
uint8_t egui_view_virtual_strip_get_keepalive_limit(egui_view_t *self);
void egui_view_virtual_strip_set_state_cache_limits(egui_view_t *self, uint16_t max_entries, uint32_t max_bytes);
uint16_t egui_view_virtual_strip_get_state_cache_entry_limit(egui_view_t *self);
uint32_t egui_view_virtual_strip_get_state_cache_byte_limit(egui_view_t *self);
void egui_view_virtual_strip_clear_item_state_cache(egui_view_t *self);
void egui_view_virtual_strip_remove_item_state_by_stable_id(egui_view_t *self, uint32_t stable_id);
uint8_t egui_view_virtual_strip_write_item_state(egui_view_t *self, uint32_t stable_id, const void *data, uint16_t size);
uint16_t egui_view_virtual_strip_read_item_state(egui_view_t *self, uint32_t stable_id, void *data, uint16_t capacity);
uint8_t egui_view_virtual_strip_write_item_state_for_view(egui_view_t *item_view, uint32_t stable_id, const void *data, uint16_t size);
uint16_t egui_view_virtual_strip_read_item_state_for_view(egui_view_t *item_view, uint32_t stable_id, void *data, uint16_t capacity);

void egui_view_virtual_strip_set_scroll_x(egui_view_t *self, int32_t offset);
void egui_view_virtual_strip_scroll_by(egui_view_t *self, int32_t delta);
void egui_view_virtual_strip_scroll_to_item(egui_view_t *self, uint32_t index, int32_t item_offset);
void egui_view_virtual_strip_scroll_to_stable_id(egui_view_t *self, uint32_t stable_id, int32_t item_offset);
int32_t egui_view_virtual_strip_get_scroll_x(egui_view_t *self);
int32_t egui_view_virtual_strip_find_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
uint8_t egui_view_virtual_strip_resolve_item_by_stable_id(egui_view_t *self, uint32_t stable_id, egui_view_virtual_strip_entry_t *entry);
uint8_t egui_view_virtual_strip_resolve_item_by_view(egui_view_t *self, egui_view_t *item_view, egui_view_virtual_strip_entry_t *entry);
int32_t egui_view_virtual_strip_get_item_x(egui_view_t *self, uint32_t index);
int32_t egui_view_virtual_strip_get_item_width(egui_view_t *self, uint32_t index);
int32_t egui_view_virtual_strip_get_item_x_by_stable_id(egui_view_t *self, uint32_t stable_id);
int32_t egui_view_virtual_strip_get_item_width_by_stable_id(egui_view_t *self, uint32_t stable_id);
uint8_t egui_view_virtual_strip_ensure_item_visible_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t inset);

void egui_view_virtual_strip_notify_data_changed(egui_view_t *self);
void egui_view_virtual_strip_notify_item_changed(egui_view_t *self, uint32_t index);
void egui_view_virtual_strip_notify_item_changed_by_stable_id(egui_view_t *self, uint32_t stable_id);
void egui_view_virtual_strip_notify_item_inserted(egui_view_t *self, uint32_t index, uint32_t count);
void egui_view_virtual_strip_notify_item_removed(egui_view_t *self, uint32_t index, uint32_t count);
void egui_view_virtual_strip_notify_item_moved(egui_view_t *self, uint32_t from_index, uint32_t to_index);
void egui_view_virtual_strip_notify_item_resized(egui_view_t *self, uint32_t index);
void egui_view_virtual_strip_notify_item_resized_by_stable_id(egui_view_t *self, uint32_t stable_id);

uint8_t egui_view_virtual_strip_get_slot_count(egui_view_t *self);
const egui_view_virtual_strip_slot_t *egui_view_virtual_strip_get_slot(egui_view_t *self, uint8_t slot_index);
int32_t egui_view_virtual_strip_find_slot_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
const egui_view_virtual_strip_slot_t *egui_view_virtual_strip_find_slot_by_stable_id(egui_view_t *self, uint32_t stable_id);
egui_view_t *egui_view_virtual_strip_find_view_by_stable_id(egui_view_t *self, uint32_t stable_id);
uint8_t egui_view_virtual_strip_get_slot_entry(egui_view_t *self, uint8_t slot_index, egui_view_virtual_strip_entry_t *entry);
uint8_t egui_view_virtual_strip_visit_visible_items(egui_view_t *self, egui_view_virtual_strip_visible_item_visitor_t visitor, void *context);
egui_view_t *egui_view_virtual_strip_find_first_visible_item_view(egui_view_t *self, egui_view_virtual_strip_visible_item_matcher_t matcher, void *context,
                                                                  egui_view_virtual_strip_entry_t *entry_out);

void egui_view_virtual_strip_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_VIRTUAL_STRIP_H_ */
