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
typedef uint8_t (*egui_view_virtual_tree_visible_node_matcher_t)(egui_view_t *self, const egui_view_virtual_tree_slot_t *slot,
                                                                 const egui_view_virtual_tree_entry_t *entry, egui_view_t *node_view, void *context);
typedef uint8_t (*egui_view_virtual_tree_visible_node_visitor_t)(egui_view_t *self, const egui_view_virtual_tree_slot_t *slot,
                                                                 const egui_view_virtual_tree_entry_t *entry, egui_view_t *node_view, void *context);

struct egui_view_virtual_tree
{
    egui_view_virtual_list_t base;
    const egui_view_virtual_tree_data_source_t *data_source;
    void *data_source_context;
    egui_view_virtual_list_data_source_t flat_data_source;
};

struct egui_view_virtual_tree_params
{
    egui_region_t region;
    uint8_t overscan_before;
    uint8_t overscan_after;
    uint8_t max_keepalive_slots;
    int32_t estimated_node_height;
};

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

void egui_view_virtual_tree_apply_params(egui_view_t *self, const egui_view_virtual_tree_params_t *params);
void egui_view_virtual_tree_init_with_params(egui_view_t *self, const egui_view_virtual_tree_params_t *params);
void egui_view_virtual_tree_apply_setup(egui_view_t *self, const egui_view_virtual_tree_setup_t *setup);
void egui_view_virtual_tree_init_with_setup(egui_view_t *self, const egui_view_virtual_tree_setup_t *setup);

void egui_view_virtual_tree_set_data_source(egui_view_t *self, const egui_view_virtual_tree_data_source_t *data_source, void *data_source_context);
const egui_view_virtual_tree_data_source_t *egui_view_virtual_tree_get_data_source(egui_view_t *self);
void *egui_view_virtual_tree_get_data_source_context(egui_view_t *self);
uint32_t egui_view_virtual_tree_get_root_count(egui_view_t *self);
uint32_t egui_view_virtual_tree_get_visible_node_count(egui_view_t *self);

void egui_view_virtual_tree_set_overscan(egui_view_t *self, uint8_t before, uint8_t after);
uint8_t egui_view_virtual_tree_get_overscan_before(egui_view_t *self);
uint8_t egui_view_virtual_tree_get_overscan_after(egui_view_t *self);

void egui_view_virtual_tree_set_estimated_node_height(egui_view_t *self, int32_t height);
int32_t egui_view_virtual_tree_get_estimated_node_height(egui_view_t *self);

void egui_view_virtual_tree_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots);
uint8_t egui_view_virtual_tree_get_keepalive_limit(egui_view_t *self);

void egui_view_virtual_tree_set_state_cache_limits(egui_view_t *self, uint16_t max_entries, uint32_t max_bytes);
void egui_view_virtual_tree_clear_node_state_cache(egui_view_t *self);
void egui_view_virtual_tree_remove_node_state_by_stable_id(egui_view_t *self, uint32_t stable_id);
uint8_t egui_view_virtual_tree_write_node_state(egui_view_t *self, uint32_t stable_id, const void *data, uint16_t size);
uint16_t egui_view_virtual_tree_read_node_state(egui_view_t *self, uint32_t stable_id, void *data, uint16_t capacity);
uint8_t egui_view_virtual_tree_write_node_state_for_view(egui_view_t *node_view, uint32_t stable_id, const void *data, uint16_t size);
uint16_t egui_view_virtual_tree_read_node_state_for_view(egui_view_t *node_view, uint32_t stable_id, void *data, uint16_t capacity);

void egui_view_virtual_tree_set_scroll_y(egui_view_t *self, int32_t offset);
void egui_view_virtual_tree_scroll_by(egui_view_t *self, int32_t delta);
void egui_view_virtual_tree_scroll_to_visible_node(egui_view_t *self, uint32_t visible_index, int32_t node_offset);
void egui_view_virtual_tree_scroll_to_node_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t node_offset);
int32_t egui_view_virtual_tree_get_scroll_y(egui_view_t *self);
int32_t egui_view_virtual_tree_find_visible_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
uint8_t egui_view_virtual_tree_resolve_node_by_stable_id(egui_view_t *self, uint32_t stable_id, egui_view_virtual_tree_entry_t *entry);
uint8_t egui_view_virtual_tree_resolve_node_by_view(egui_view_t *self, egui_view_t *node_view, egui_view_virtual_tree_entry_t *entry);
int32_t egui_view_virtual_tree_get_visible_node_y(egui_view_t *self, uint32_t visible_index);
int32_t egui_view_virtual_tree_get_visible_node_height(egui_view_t *self, uint32_t visible_index);
int32_t egui_view_virtual_tree_get_node_y_by_stable_id(egui_view_t *self, uint32_t stable_id);
int32_t egui_view_virtual_tree_get_node_height_by_stable_id(egui_view_t *self, uint32_t stable_id);
uint8_t egui_view_virtual_tree_ensure_node_visible_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t inset);

void egui_view_virtual_tree_notify_data_changed(egui_view_t *self);
void egui_view_virtual_tree_notify_visible_node_changed(egui_view_t *self, uint32_t visible_index);
void egui_view_virtual_tree_notify_node_changed_by_stable_id(egui_view_t *self, uint32_t stable_id);
void egui_view_virtual_tree_notify_visible_node_resized(egui_view_t *self, uint32_t visible_index);
void egui_view_virtual_tree_notify_node_resized_by_stable_id(egui_view_t *self, uint32_t stable_id);

uint8_t egui_view_virtual_tree_get_slot_count(egui_view_t *self);
const egui_view_virtual_tree_slot_t *egui_view_virtual_tree_get_slot(egui_view_t *self, uint8_t slot_index);
int32_t egui_view_virtual_tree_find_slot_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
const egui_view_virtual_tree_slot_t *egui_view_virtual_tree_find_slot_by_stable_id(egui_view_t *self, uint32_t stable_id);
egui_view_t *egui_view_virtual_tree_find_view_by_stable_id(egui_view_t *self, uint32_t stable_id);
uint8_t egui_view_virtual_tree_get_slot_entry(egui_view_t *self, uint8_t slot_index, egui_view_virtual_tree_entry_t *entry);
uint8_t egui_view_virtual_tree_visit_visible_nodes(egui_view_t *self, egui_view_virtual_tree_visible_node_visitor_t visitor, void *context);
egui_view_t *egui_view_virtual_tree_find_first_visible_node_view(egui_view_t *self, egui_view_virtual_tree_visible_node_matcher_t matcher,
                                                                 void *context, egui_view_virtual_tree_entry_t *entry_out);

void egui_view_virtual_tree_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_VIRTUAL_TREE_H_ */
