#ifndef _EGUI_VIEW_VIRTUAL_STAGE_H_
#define _EGUI_VIEW_VIRTUAL_STAGE_H_

#include <stddef.h>

#include "egui_view_group.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS      8
#define EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID     0xFFFFFFFFUL
#define EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE 0U

enum
{
    EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_UNUSED = 0,
    EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_ACTIVE,
    EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_KEEPALIVE,
    EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_POOLED,
};

enum
{
    EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE = 0x01,
    EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE = 0x02,
    EGUI_VIRTUAL_STAGE_NODE_FLAG_HIDDEN = 0x04,
};

typedef struct egui_virtual_stage_node_desc egui_virtual_stage_node_desc_t;
typedef struct egui_view_virtual_stage_adapter egui_view_virtual_stage_adapter_t;
typedef struct egui_view_virtual_stage_slot egui_view_virtual_stage_slot_t;
typedef struct egui_view_virtual_stage egui_view_virtual_stage_t;
typedef struct egui_view_virtual_stage_params egui_view_virtual_stage_params_t;
typedef struct egui_view_virtual_stage_setup egui_view_virtual_stage_setup_t;
typedef struct egui_view_virtual_stage_entry egui_view_virtual_stage_entry_t;
typedef struct egui_view_virtual_stage_array_source egui_view_virtual_stage_array_source_t;
typedef struct egui_view_virtual_stage_array_ops egui_view_virtual_stage_array_ops_t;
typedef struct egui_view_virtual_stage_array_adapter egui_view_virtual_stage_array_adapter_t;
typedef struct egui_view_virtual_stage_array_setup egui_view_virtual_stage_array_setup_t;

struct egui_virtual_stage_node_desc
{
    egui_region_t region;
    uint32_t stable_id;
    uint16_t view_type;
    int16_t z_order;
    uint8_t flags;
    uint8_t reserved;
};

struct egui_view_virtual_stage_adapter
{
    uint32_t (*get_count)(void *adapter_context);
    uint8_t (*get_desc)(void *adapter_context, uint32_t index, egui_virtual_stage_node_desc_t *out_desc);
    egui_view_t *(*create_view)(void *adapter_context, uint16_t view_type);
    void (*destroy_view)(void *adapter_context, egui_view_t *view, uint16_t view_type);
    void (*bind_view)(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc);
    void (*unbind_view)(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc);
    void (*save_state)(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc);
    void (*restore_state)(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc);
    void (*draw_node)(void *adapter_context, egui_view_t *page, uint32_t index, const egui_virtual_stage_node_desc_t *desc, const egui_region_t *screen_region);
    uint8_t (*hit_test)(void *adapter_context, uint32_t index, const egui_virtual_stage_node_desc_t *desc, const egui_region_t *screen_region,
                        egui_dim_t screen_x, egui_dim_t screen_y);
    uint8_t (*should_keep_alive)(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc);
};

struct egui_view_virtual_stage_slot
{
    uint8_t state;
    uint16_t view_type;
    uint32_t index;
    uint32_t stable_id;
    egui_view_t *view;
    egui_region_t render_region;
    egui_virtual_stage_node_desc_t bound_desc;
    uint32_t last_used_seq;
    uint8_t needs_bind;
    uint8_t needs_layout;
    egui_view_group_touch_state_snapshot_t touch_state;
};

struct egui_view_virtual_stage
{
    egui_view_group_t base;

    egui_view_group_t content_layer;

    const egui_view_virtual_stage_adapter_t *adapter;
    void *adapter_context;

    void *node_cache;
    void *pin_entries;
    uint32_t *draw_order;
    uint32_t cached_node_count;
    uint32_t cached_capacity;
    uint16_t pin_count;
    uint16_t pin_capacity;
    uint32_t slot_use_seq;
    uint8_t live_slot_limit;
    uint8_t slot_count;
    uint8_t is_data_dirty;
    uint8_t is_layout_dirty;
    uint8_t needs_slot_sync;
    uint8_t is_attached_to_window;
    uint8_t captured_slot;

    egui_view_virtual_stage_slot_t slots[EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS];
};

struct egui_view_virtual_stage_params
{
    egui_region_t region;
    uint8_t live_slot_limit;
};

struct egui_view_virtual_stage_setup
{
    const egui_view_virtual_stage_params_t *params;
    const egui_view_virtual_stage_adapter_t *adapter;
    void *adapter_context;
};

struct egui_view_virtual_stage_entry
{
    uint32_t index;
    uint32_t stable_id;
    egui_virtual_stage_node_desc_t desc;
};

struct egui_view_virtual_stage_array_source
{
    const void *items;
    uint32_t count;
    uint16_t item_size;
    uint16_t desc_offset;
};

struct egui_view_virtual_stage_array_ops
{
    egui_view_t *(*create_view)(void *user_context, uint16_t view_type);
    void (*destroy_view)(void *user_context, egui_view_t *view, uint16_t view_type);
    void (*bind_view)(void *user_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc);
    void (*unbind_view)(void *user_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc);
    void (*save_state)(void *user_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc);
    void (*restore_state)(void *user_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc);
    void (*draw_node)(void *user_context, egui_view_t *page, uint32_t index, const egui_virtual_stage_node_desc_t *desc, const egui_region_t *screen_region);
    uint8_t (*hit_test)(void *user_context, uint32_t index, const egui_virtual_stage_node_desc_t *desc, const egui_region_t *screen_region, egui_dim_t screen_x,
                        egui_dim_t screen_y);
    uint8_t (*should_keep_alive)(void *user_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc);
};

struct egui_view_virtual_stage_array_adapter
{
    egui_view_virtual_stage_adapter_t adapter;
    const egui_view_virtual_stage_array_source_t *node_source;
    const egui_view_virtual_stage_array_ops_t *ops;
    void *user_context;
};

struct egui_view_virtual_stage_array_setup
{
    const egui_view_virtual_stage_params_t *params;
    const egui_view_virtual_stage_array_source_t *node_source;
    const egui_view_virtual_stage_array_ops_t *ops;
    void *user_context;
};

#define EGUI_VIEW_VIRTUAL_STAGE_PARAMS_INIT(_name, _x, _y, _w, _h)                                                                                             \
    static const egui_view_virtual_stage_params_t _name = {                                                                                                    \
            .region = {{(_x), (_y)}, {(_w), (_h)}},                                                                                                            \
            .live_slot_limit = 4,                                                                                                                              \
    }

#define EGUI_VIEW_VIRTUAL_STAGE_SETUP_INIT(_name, _params, _adapter, _adapter_context)                                                                         \
    static const egui_view_virtual_stage_setup_t _name = {                                                                                                     \
            .params = (_params),                                                                                                                               \
            .adapter = (_adapter),                                                                                                                             \
            .adapter_context = (_adapter_context),                                                                                                             \
    }

#define EGUI_VIEW_VIRTUAL_STAGE_DESC_ARRAY_SOURCE_INIT(_name, _items)                                                                                          \
    static const egui_view_virtual_stage_array_source_t _name = {                                                                                              \
            .items = (_items),                                                                                                                                 \
            .count = EGUI_ARRAY_SIZE(_items),                                                                                                                  \
            .item_size = (uint16_t)sizeof((_items)[0]),                                                                                                        \
            .desc_offset = 0,                                                                                                                                  \
    }

#define EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_SOURCE_INIT(_name, _items, _item_type, _desc_member)                                                                \
    static const egui_view_virtual_stage_array_source_t _name = {                                                                                              \
            .items = (_items),                                                                                                                                 \
            .count = EGUI_ARRAY_SIZE(_items),                                                                                                                  \
            .item_size = (uint16_t)sizeof((_items)[0]),                                                                                                        \
            .desc_offset = (uint16_t)offsetof(_item_type, _desc_member),                                                                                       \
    }

#define EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_INIT(_name)                                                                                                          \
    static const egui_view_virtual_stage_array_ops_t _name = {                                                                                                 \
            .create_view = NULL,                                                                                                                               \
            .destroy_view = NULL,                                                                                                                              \
            .bind_view = NULL,                                                                                                                                 \
            .unbind_view = NULL,                                                                                                                               \
            .save_state = NULL,                                                                                                                                \
            .restore_state = NULL,                                                                                                                             \
            .draw_node = NULL,                                                                                                                                 \
            .hit_test = NULL,                                                                                                                                  \
            .should_keep_alive = NULL,                                                                                                                         \
    }

#define EGUI_VIEW_VIRTUAL_STAGE_ARRAY_SETUP_INIT(_name, _params, _node_source, _ops, _user_context)                                                            \
    static const egui_view_virtual_stage_array_setup_t _name = {                                                                                               \
            .params = (_params),                                                                                                                               \
            .node_source = (_node_source),                                                                                                                     \
            .ops = (_ops),                                                                                                                                     \
            .user_context = (_user_context),                                                                                                                   \
    }

#define EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES_CHANGED(_self, _stable_ids)                                                                                       \
    egui_view_virtual_stage_notify_nodes_changed((_self), (_stable_ids), EGUI_ARRAY_SIZE(_stable_ids))

#define EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE_BOUNDS_CHANGED(_self, _stable_ids)                                                                                 \
    egui_view_virtual_stage_notify_nodes_bounds_changed((_self), (_stable_ids), EGUI_ARRAY_SIZE(_stable_ids))

void egui_view_virtual_stage_apply_params(egui_view_t *self, const egui_view_virtual_stage_params_t *params);
void egui_view_virtual_stage_init_with_params(egui_view_t *self, const egui_view_virtual_stage_params_t *params);
void egui_view_virtual_stage_apply_setup(egui_view_t *self, const egui_view_virtual_stage_setup_t *setup);
void egui_view_virtual_stage_init_with_setup(egui_view_t *self, const egui_view_virtual_stage_setup_t *setup);
void egui_view_virtual_stage_apply_array_setup(egui_view_t *self, egui_view_virtual_stage_array_adapter_t *adapter,
                                               const egui_view_virtual_stage_array_setup_t *setup);
void egui_view_virtual_stage_init_with_array_setup(egui_view_t *self, egui_view_virtual_stage_array_adapter_t *adapter,
                                                   const egui_view_virtual_stage_array_setup_t *setup);
void egui_view_virtual_stage_array_adapter_init(egui_view_virtual_stage_array_adapter_t *adapter, const egui_view_virtual_stage_array_source_t *node_source,
                                                const egui_view_virtual_stage_array_ops_t *ops, void *user_context);

void egui_view_virtual_stage_set_adapter(egui_view_t *self, const egui_view_virtual_stage_adapter_t *adapter, void *adapter_context);
const egui_view_virtual_stage_adapter_t *egui_view_virtual_stage_get_adapter(egui_view_t *self);
void *egui_view_virtual_stage_get_adapter_context(egui_view_t *self);

void egui_view_virtual_stage_set_live_slot_limit(egui_view_t *self, uint8_t live_slot_limit);
uint8_t egui_view_virtual_stage_get_live_slot_limit(egui_view_t *self);

void egui_view_virtual_stage_notify_data_changed(egui_view_t *self);
void egui_view_virtual_stage_notify_node_changed(egui_view_t *self, uint32_t stable_id);
void egui_view_virtual_stage_notify_node_bounds_changed(egui_view_t *self, uint32_t stable_id);
void egui_view_virtual_stage_notify_nodes_changed(egui_view_t *self, const uint32_t *stable_ids, uint32_t count);
void egui_view_virtual_stage_notify_nodes_bounds_changed(egui_view_t *self, const uint32_t *stable_ids, uint32_t count);

uint8_t egui_view_virtual_stage_pin_node(egui_view_t *self, uint32_t stable_id);
void egui_view_virtual_stage_unpin_node(egui_view_t *self, uint32_t stable_id);
uint8_t egui_view_virtual_stage_is_node_pinned(egui_view_t *self, uint32_t stable_id);
uint8_t egui_view_virtual_stage_toggle_pin_node(egui_view_t *self, uint32_t stable_id);

uint8_t egui_view_virtual_stage_get_slot_count(egui_view_t *self);
const egui_view_virtual_stage_slot_t *egui_view_virtual_stage_get_slot(egui_view_t *self, uint8_t slot_index);
const egui_view_virtual_stage_slot_t *egui_view_virtual_stage_find_slot_by_stable_id(egui_view_t *self, uint32_t stable_id);
egui_view_t *egui_view_virtual_stage_find_view_by_stable_id(egui_view_t *self, uint32_t stable_id);
uint8_t egui_view_virtual_stage_resolve_node_by_view(egui_view_t *self, egui_view_t *node_view, egui_view_virtual_stage_entry_t *entry);
uint8_t egui_view_virtual_stage_resolve_stable_id_by_view(egui_view_t *self, egui_view_t *node_view, uint32_t *stable_id_out);

void egui_view_virtual_stage_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_VIRTUAL_STAGE_H_ */
