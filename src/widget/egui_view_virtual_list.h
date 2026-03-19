#ifndef _EGUI_VIEW_VIRTUAL_LIST_H_
#define _EGUI_VIEW_VIRTUAL_LIST_H_

#include "egui_view_virtual_viewport.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef egui_view_virtual_viewport_adapter_t egui_view_virtual_list_adapter_t;
typedef egui_view_virtual_viewport_slot_t egui_view_virtual_list_slot_t;

typedef struct egui_view_virtual_list egui_view_virtual_list_t;
typedef struct egui_view_virtual_list_data_source egui_view_virtual_list_data_source_t;
typedef struct egui_view_virtual_list_params egui_view_virtual_list_params_t;

struct egui_view_virtual_list
{
    egui_view_virtual_viewport_t base;
    const egui_view_virtual_list_data_source_t *data_source;
    void *data_source_context;
    egui_view_virtual_list_adapter_t data_source_adapter;
};

struct egui_view_virtual_list_params
{
    egui_region_t region;
    uint8_t overscan_before;
    uint8_t overscan_after;
    uint8_t max_keepalive_slots;
    int32_t estimated_item_height;
};

#define EGUI_VIEW_VIRTUAL_LIST_PARAMS_INIT(_name, _x, _y, _w, _h)                                                                                              \
    static const egui_view_virtual_list_params_t _name = {                                                                                                     \
            .region = {{(_x), (_y)}, {(_w), (_h)}},                                                                                                            \
            .overscan_before = 1,                                                                                                                              \
            .overscan_after = 1,                                                                                                                               \
            .max_keepalive_slots = 2,                                                                                                                          \
            .estimated_item_height = 40,                                                                                                                       \
    }

struct egui_view_virtual_list_data_source
{
    uint32_t (*get_count)(void *data_source_context);
    uint32_t (*get_stable_id)(void *data_source_context, uint32_t index);
    int32_t (*find_index_by_stable_id)(void *data_source_context, uint32_t stable_id);
    uint16_t (*get_view_type)(void *data_source_context, uint32_t index);
    int32_t (*measure_item_height)(void *data_source_context, uint32_t index, int32_t width_hint);
    egui_view_t *(*create_item_view)(void *data_source_context, uint16_t view_type);
    void (*destroy_item_view)(void *data_source_context, egui_view_t *view, uint16_t view_type);
    void (*bind_item_view)(void *data_source_context, egui_view_t *view, uint32_t index, uint32_t stable_id);
    void (*unbind_item_view)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    uint8_t (*should_keep_alive)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*save_item_state)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*restore_item_state)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    uint16_t default_view_type;
};

#define EGUI_VIEW_VIRTUAL_LIST_DATA_SOURCE_INIT(_name)                                                                                                         \
    static const egui_view_virtual_list_data_source_t _name = {                                                                                                \
            .get_count = NULL,                                                                                                                                 \
            .get_stable_id = NULL,                                                                                                                             \
            .find_index_by_stable_id = NULL,                                                                                                                   \
            .get_view_type = NULL,                                                                                                                             \
            .measure_item_height = NULL,                                                                                                                       \
            .create_item_view = NULL,                                                                                                                          \
            .destroy_item_view = NULL,                                                                                                                         \
            .bind_item_view = NULL,                                                                                                                            \
            .unbind_item_view = NULL,                                                                                                                          \
            .should_keep_alive = NULL,                                                                                                                         \
            .save_item_state = NULL,                                                                                                                           \
            .restore_item_state = NULL,                                                                                                                        \
            .default_view_type = 0,                                                                                                                            \
    }

void egui_view_virtual_list_apply_params(egui_view_t *self, const egui_view_virtual_list_params_t *params);
void egui_view_virtual_list_init_with_params(egui_view_t *self, const egui_view_virtual_list_params_t *params);

void egui_view_virtual_list_set_adapter(egui_view_t *self, const egui_view_virtual_list_adapter_t *adapter, void *adapter_context);
const egui_view_virtual_list_adapter_t *egui_view_virtual_list_get_adapter(egui_view_t *self);
void *egui_view_virtual_list_get_adapter_context(egui_view_t *self);

void egui_view_virtual_list_set_data_source(egui_view_t *self, const egui_view_virtual_list_data_source_t *data_source, void *data_source_context);
const egui_view_virtual_list_data_source_t *egui_view_virtual_list_get_data_source(egui_view_t *self);
void *egui_view_virtual_list_get_data_source_context(egui_view_t *self);

void egui_view_virtual_list_set_overscan(egui_view_t *self, uint8_t before, uint8_t after);
uint8_t egui_view_virtual_list_get_overscan_before(egui_view_t *self);
uint8_t egui_view_virtual_list_get_overscan_after(egui_view_t *self);

void egui_view_virtual_list_set_estimated_item_height(egui_view_t *self, int32_t height);
int32_t egui_view_virtual_list_get_estimated_item_height(egui_view_t *self);

void egui_view_virtual_list_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots);
uint8_t egui_view_virtual_list_get_keepalive_limit(egui_view_t *self);
void egui_view_virtual_list_set_state_cache_limits(egui_view_t *self, uint16_t max_entries, uint32_t max_bytes);
void egui_view_virtual_list_clear_item_state_cache(egui_view_t *self);
void egui_view_virtual_list_remove_item_state_by_stable_id(egui_view_t *self, uint32_t stable_id);
uint8_t egui_view_virtual_list_write_item_state(egui_view_t *self, uint32_t stable_id, const void *data, uint16_t size);
uint16_t egui_view_virtual_list_read_item_state(egui_view_t *self, uint32_t stable_id, void *data, uint16_t capacity);
uint8_t egui_view_virtual_list_write_item_state_for_view(egui_view_t *item_view, uint32_t stable_id, const void *data, uint16_t size);
uint16_t egui_view_virtual_list_read_item_state_for_view(egui_view_t *item_view, uint32_t stable_id, void *data, uint16_t capacity);

void egui_view_virtual_list_set_scroll_y(egui_view_t *self, int32_t offset);
void egui_view_virtual_list_scroll_by(egui_view_t *self, int32_t delta);
void egui_view_virtual_list_scroll_to_item(egui_view_t *self, uint32_t index, int32_t item_offset);
void egui_view_virtual_list_scroll_to_stable_id(egui_view_t *self, uint32_t stable_id, int32_t item_offset);
int32_t egui_view_virtual_list_get_scroll_y(egui_view_t *self);
int32_t egui_view_virtual_list_find_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
int32_t egui_view_virtual_list_get_item_y(egui_view_t *self, uint32_t index);
int32_t egui_view_virtual_list_get_item_height(egui_view_t *self, uint32_t index);

void egui_view_virtual_list_notify_data_changed(egui_view_t *self);
void egui_view_virtual_list_notify_item_changed(egui_view_t *self, uint32_t index);
void egui_view_virtual_list_notify_item_changed_by_stable_id(egui_view_t *self, uint32_t stable_id);
void egui_view_virtual_list_notify_item_inserted(egui_view_t *self, uint32_t index, uint32_t count);
void egui_view_virtual_list_notify_item_removed(egui_view_t *self, uint32_t index, uint32_t count);
void egui_view_virtual_list_notify_item_moved(egui_view_t *self, uint32_t from_index, uint32_t to_index);
void egui_view_virtual_list_notify_item_resized(egui_view_t *self, uint32_t index);
void egui_view_virtual_list_notify_item_resized_by_stable_id(egui_view_t *self, uint32_t stable_id);

uint8_t egui_view_virtual_list_get_slot_count(egui_view_t *self);
const egui_view_virtual_list_slot_t *egui_view_virtual_list_get_slot(egui_view_t *self, uint8_t slot_index);

void egui_view_virtual_list_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_VIRTUAL_LIST_H_ */
