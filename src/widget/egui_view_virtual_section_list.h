#ifndef _EGUI_VIEW_VIRTUAL_SECTION_LIST_H_
#define _EGUI_VIEW_VIRTUAL_SECTION_LIST_H_

#include "egui_view_virtual_list.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_VIRTUAL_SECTION_LIST_INVALID_INDEX 0xFFFFFFFFUL

typedef egui_view_virtual_viewport_slot_t egui_view_virtual_section_list_slot_t;

typedef struct egui_view_virtual_section_list egui_view_virtual_section_list_t;
typedef struct egui_view_virtual_section_list_data_source egui_view_virtual_section_list_data_source_t;
typedef struct egui_view_virtual_section_list_entry egui_view_virtual_section_list_entry_t;
typedef struct egui_view_virtual_section_list_params egui_view_virtual_section_list_params_t;
typedef struct egui_view_virtual_section_list_setup egui_view_virtual_section_list_setup_t;
typedef uint8_t (*egui_view_virtual_section_list_visible_entry_matcher_t)(egui_view_t *self, const egui_view_virtual_section_list_slot_t *slot,
                                                                          const egui_view_virtual_section_list_entry_t *entry, egui_view_t *entry_view,
                                                                          void *context);
typedef uint8_t (*egui_view_virtual_section_list_visible_entry_visitor_t)(egui_view_t *self, const egui_view_virtual_section_list_slot_t *slot,
                                                                          const egui_view_virtual_section_list_entry_t *entry, egui_view_t *entry_view,
                                                                          void *context);

struct egui_view_virtual_section_list
{
    egui_view_virtual_list_t base;
    const egui_view_virtual_section_list_data_source_t *data_source;
    void *data_source_context;
    egui_view_virtual_list_data_source_t flat_data_source;
};

struct egui_view_virtual_section_list_params
{
    egui_region_t region;
    uint8_t overscan_before;
    uint8_t overscan_after;
    uint8_t max_keepalive_slots;
    int32_t estimated_entry_height;
};

struct egui_view_virtual_section_list_entry
{
    uint8_t is_section_header;
    uint32_t section_index;
    uint32_t item_index;
    uint32_t stable_id;
};

#define EGUI_VIEW_VIRTUAL_SECTION_LIST_PARAMS_INIT(_name, _x, _y, _w, _h)                                                                                      \
    static const egui_view_virtual_section_list_params_t _name = {                                                                                             \
            .region = {{(_x), (_y)}, {(_w), (_h)}},                                                                                                            \
            .overscan_before = 1,                                                                                                                              \
            .overscan_after = 1,                                                                                                                               \
            .max_keepalive_slots = 2,                                                                                                                          \
            .estimated_entry_height = 40,                                                                                                                      \
    }

struct egui_view_virtual_section_list_setup
{
    const egui_view_virtual_section_list_params_t *params;
    const egui_view_virtual_section_list_data_source_t *data_source;
    void *data_source_context;
    uint16_t state_cache_max_entries;
    uint32_t state_cache_max_bytes;
};

#define EGUI_VIEW_VIRTUAL_SECTION_LIST_SETUP_INIT(_name, _params, _data_source, _data_source_context)                                                          \
    static const egui_view_virtual_section_list_setup_t _name = {                                                                                              \
            .params = (_params),                                                                                                                               \
            .data_source = (_data_source),                                                                                                                     \
            .data_source_context = (_data_source_context),                                                                                                     \
            .state_cache_max_entries = 0,                                                                                                                      \
            .state_cache_max_bytes = 0,                                                                                                                        \
    }

struct egui_view_virtual_section_list_data_source
{
    uint32_t (*get_section_count)(void *data_source_context);
    uint32_t (*get_section_stable_id)(void *data_source_context, uint32_t section_index);
    int32_t (*find_section_index_by_stable_id)(void *data_source_context, uint32_t stable_id);
    uint32_t (*get_item_count)(void *data_source_context, uint32_t section_index);
    uint32_t (*get_item_stable_id)(void *data_source_context, uint32_t section_index, uint32_t item_index);
    uint8_t (*find_item_position_by_stable_id)(void *data_source_context, uint32_t stable_id, uint32_t *section_index, uint32_t *item_index);
    uint16_t (*get_section_header_view_type)(void *data_source_context, uint32_t section_index);
    uint16_t (*get_item_view_type)(void *data_source_context, uint32_t section_index, uint32_t item_index);
    int32_t (*measure_section_header_height)(void *data_source_context, uint32_t section_index, int32_t width_hint);
    int32_t (*measure_item_height)(void *data_source_context, uint32_t section_index, uint32_t item_index, int32_t width_hint);
    egui_view_t *(*create_section_header_view)(void *data_source_context, uint16_t view_type);
    egui_view_t *(*create_item_view)(void *data_source_context, uint16_t view_type);
    void (*destroy_section_header_view)(void *data_source_context, egui_view_t *view, uint16_t view_type);
    void (*destroy_item_view)(void *data_source_context, egui_view_t *view, uint16_t view_type);
    void (*bind_section_header_view)(void *data_source_context, egui_view_t *view, uint32_t section_index, uint32_t stable_id);
    void (*bind_item_view)(void *data_source_context, egui_view_t *view, uint32_t section_index, uint32_t item_index, uint32_t stable_id);
    void (*unbind_section_header_view)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*unbind_item_view)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    uint8_t (*should_keep_section_header_alive)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    uint8_t (*should_keep_item_alive)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*save_section_header_state)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*save_item_state)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*restore_section_header_state)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    void (*restore_item_state)(void *data_source_context, egui_view_t *view, uint32_t stable_id);
    uint16_t default_section_header_view_type;
    uint16_t default_item_view_type;
};

#define EGUI_VIEW_VIRTUAL_SECTION_LIST_DATA_SOURCE_INIT(_name)                                                                                                 \
    static const egui_view_virtual_section_list_data_source_t _name = {                                                                                        \
            .get_section_count = NULL,                                                                                                                         \
            .get_section_stable_id = NULL,                                                                                                                     \
            .find_section_index_by_stable_id = NULL,                                                                                                           \
            .get_item_count = NULL,                                                                                                                            \
            .get_item_stable_id = NULL,                                                                                                                        \
            .find_item_position_by_stable_id = NULL,                                                                                                           \
            .get_section_header_view_type = NULL,                                                                                                              \
            .get_item_view_type = NULL,                                                                                                                        \
            .measure_section_header_height = NULL,                                                                                                             \
            .measure_item_height = NULL,                                                                                                                       \
            .create_section_header_view = NULL,                                                                                                                \
            .create_item_view = NULL,                                                                                                                          \
            .destroy_section_header_view = NULL,                                                                                                               \
            .destroy_item_view = NULL,                                                                                                                         \
            .bind_section_header_view = NULL,                                                                                                                  \
            .bind_item_view = NULL,                                                                                                                            \
            .unbind_section_header_view = NULL,                                                                                                                \
            .unbind_item_view = NULL,                                                                                                                          \
            .should_keep_section_header_alive = NULL,                                                                                                          \
            .should_keep_item_alive = NULL,                                                                                                                    \
            .save_section_header_state = NULL,                                                                                                                 \
            .save_item_state = NULL,                                                                                                                           \
            .restore_section_header_state = NULL,                                                                                                              \
            .restore_item_state = NULL,                                                                                                                        \
            .default_section_header_view_type = 0,                                                                                                             \
            .default_item_view_type = 0,                                                                                                                       \
    }

void egui_view_virtual_section_list_apply_params(egui_view_t *self, const egui_view_virtual_section_list_params_t *params);
void egui_view_virtual_section_list_init_with_params(egui_view_t *self, const egui_view_virtual_section_list_params_t *params);
void egui_view_virtual_section_list_apply_setup(egui_view_t *self, const egui_view_virtual_section_list_setup_t *setup);
void egui_view_virtual_section_list_init_with_setup(egui_view_t *self, const egui_view_virtual_section_list_setup_t *setup);

void egui_view_virtual_section_list_set_data_source(egui_view_t *self, const egui_view_virtual_section_list_data_source_t *data_source,
                                                    void *data_source_context);
const egui_view_virtual_section_list_data_source_t *egui_view_virtual_section_list_get_data_source(egui_view_t *self);
void *egui_view_virtual_section_list_get_data_source_context(egui_view_t *self);
uint32_t egui_view_virtual_section_list_get_section_count(egui_view_t *self);
uint32_t egui_view_virtual_section_list_get_item_count(egui_view_t *self, uint32_t section_index);

void egui_view_virtual_section_list_set_overscan(egui_view_t *self, uint8_t before, uint8_t after);
uint8_t egui_view_virtual_section_list_get_overscan_before(egui_view_t *self);
uint8_t egui_view_virtual_section_list_get_overscan_after(egui_view_t *self);

void egui_view_virtual_section_list_set_estimated_entry_height(egui_view_t *self, int32_t height);
int32_t egui_view_virtual_section_list_get_estimated_entry_height(egui_view_t *self);

void egui_view_virtual_section_list_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots);
uint8_t egui_view_virtual_section_list_get_keepalive_limit(egui_view_t *self);

void egui_view_virtual_section_list_set_state_cache_limits(egui_view_t *self, uint16_t max_entries, uint32_t max_bytes);
uint16_t egui_view_virtual_section_list_get_state_cache_entry_limit(egui_view_t *self);
uint32_t egui_view_virtual_section_list_get_state_cache_byte_limit(egui_view_t *self);
void egui_view_virtual_section_list_clear_entry_state_cache(egui_view_t *self);
void egui_view_virtual_section_list_remove_entry_state_by_stable_id(egui_view_t *self, uint32_t stable_id);
uint8_t egui_view_virtual_section_list_write_entry_state(egui_view_t *self, uint32_t stable_id, const void *data, uint16_t size);
uint16_t egui_view_virtual_section_list_read_entry_state(egui_view_t *self, uint32_t stable_id, void *data, uint16_t capacity);
uint8_t egui_view_virtual_section_list_write_entry_state_for_view(egui_view_t *entry_view, uint32_t stable_id, const void *data, uint16_t size);
uint16_t egui_view_virtual_section_list_read_entry_state_for_view(egui_view_t *entry_view, uint32_t stable_id, void *data, uint16_t capacity);

void egui_view_virtual_section_list_set_scroll_y(egui_view_t *self, int32_t offset);
void egui_view_virtual_section_list_scroll_by(egui_view_t *self, int32_t delta);
void egui_view_virtual_section_list_scroll_to_section(egui_view_t *self, uint32_t section_index, int32_t section_offset);
void egui_view_virtual_section_list_scroll_to_section_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t section_offset);
void egui_view_virtual_section_list_scroll_to_item(egui_view_t *self, uint32_t section_index, uint32_t item_index, int32_t item_offset);
void egui_view_virtual_section_list_scroll_to_item_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t item_offset);
int32_t egui_view_virtual_section_list_get_scroll_y(egui_view_t *self);

int32_t egui_view_virtual_section_list_find_section_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
uint8_t egui_view_virtual_section_list_find_item_position_by_stable_id(egui_view_t *self, uint32_t stable_id, uint32_t *section_index, uint32_t *item_index);
uint8_t egui_view_virtual_section_list_resolve_entry_by_stable_id(egui_view_t *self, uint32_t stable_id, egui_view_virtual_section_list_entry_t *entry);
uint8_t egui_view_virtual_section_list_resolve_entry_by_view(egui_view_t *self, egui_view_t *entry_view, egui_view_virtual_section_list_entry_t *entry);

int32_t egui_view_virtual_section_list_get_section_header_y(egui_view_t *self, uint32_t section_index);
int32_t egui_view_virtual_section_list_get_section_header_height(egui_view_t *self, uint32_t section_index);
int32_t egui_view_virtual_section_list_get_item_y(egui_view_t *self, uint32_t section_index, uint32_t item_index);
int32_t egui_view_virtual_section_list_get_item_height(egui_view_t *self, uint32_t section_index, uint32_t item_index);
int32_t egui_view_virtual_section_list_get_entry_y_by_stable_id(egui_view_t *self, uint32_t stable_id);
int32_t egui_view_virtual_section_list_get_entry_height_by_stable_id(egui_view_t *self, uint32_t stable_id);
uint8_t egui_view_virtual_section_list_ensure_entry_visible_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t inset);

void egui_view_virtual_section_list_notify_data_changed(egui_view_t *self);
void egui_view_virtual_section_list_notify_section_header_changed(egui_view_t *self, uint32_t section_index);
void egui_view_virtual_section_list_notify_section_header_changed_by_stable_id(egui_view_t *self, uint32_t stable_id);
void egui_view_virtual_section_list_notify_section_header_resized(egui_view_t *self, uint32_t section_index);
void egui_view_virtual_section_list_notify_section_header_resized_by_stable_id(egui_view_t *self, uint32_t stable_id);
void egui_view_virtual_section_list_notify_item_changed(egui_view_t *self, uint32_t section_index, uint32_t item_index);
void egui_view_virtual_section_list_notify_item_changed_by_stable_id(egui_view_t *self, uint32_t stable_id);
void egui_view_virtual_section_list_notify_item_resized(egui_view_t *self, uint32_t section_index, uint32_t item_index);
void egui_view_virtual_section_list_notify_item_resized_by_stable_id(egui_view_t *self, uint32_t stable_id);
void egui_view_virtual_section_list_notify_section_inserted(egui_view_t *self, uint32_t section_index, uint32_t count);
void egui_view_virtual_section_list_notify_section_removed(egui_view_t *self, uint32_t section_index, uint32_t count);
void egui_view_virtual_section_list_notify_section_moved(egui_view_t *self, uint32_t from_index, uint32_t to_index);
void egui_view_virtual_section_list_notify_item_inserted(egui_view_t *self, uint32_t section_index, uint32_t item_index, uint32_t count);
void egui_view_virtual_section_list_notify_item_removed(egui_view_t *self, uint32_t section_index, uint32_t item_index, uint32_t count);
void egui_view_virtual_section_list_notify_item_moved(egui_view_t *self, uint32_t from_section_index, uint32_t from_item_index, uint32_t to_section_index,
                                                      uint32_t to_item_index);

uint8_t egui_view_virtual_section_list_get_slot_count(egui_view_t *self);
const egui_view_virtual_section_list_slot_t *egui_view_virtual_section_list_get_slot(egui_view_t *self, uint8_t slot_index);
int32_t egui_view_virtual_section_list_find_slot_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
const egui_view_virtual_section_list_slot_t *egui_view_virtual_section_list_find_slot_by_stable_id(egui_view_t *self, uint32_t stable_id);
egui_view_t *egui_view_virtual_section_list_find_view_by_stable_id(egui_view_t *self, uint32_t stable_id);
uint8_t egui_view_virtual_section_list_get_slot_entry(egui_view_t *self, uint8_t slot_index, egui_view_virtual_section_list_entry_t *entry);
uint8_t egui_view_virtual_section_list_visit_visible_entries(egui_view_t *self, egui_view_virtual_section_list_visible_entry_visitor_t visitor, void *context);
egui_view_t *egui_view_virtual_section_list_find_first_visible_entry_view(egui_view_t *self, egui_view_virtual_section_list_visible_entry_matcher_t matcher,
                                                                          void *context, egui_view_virtual_section_list_entry_t *entry_out);

void egui_view_virtual_section_list_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_VIRTUAL_SECTION_LIST_H_ */
