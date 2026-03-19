#ifndef _EGUI_VIEW_VIRTUAL_VIEWPORT_H_
#define _EGUI_VIEW_VIRTUAL_VIEWPORT_H_

#include "egui_view_group.h"
#include "core/egui_scroller.h"

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
typedef struct egui_view_virtual_viewport_params egui_view_virtual_viewport_params_t;

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
    uint32_t indexed_item_count;
    int32_t indexed_cross_extent;
    int32_t *index_block_extents;
    int32_t *index_block_origins;
    uint8_t *index_block_valid;

    egui_scroller_t scroller;

    egui_view_virtual_viewport_slot_t slots[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS];
};

struct egui_view_virtual_viewport_params
{
    egui_region_t region;
    uint8_t orientation;
    uint8_t overscan_before;
    uint8_t overscan_after;
    uint8_t max_keepalive_slots;
    int32_t estimated_item_extent;
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

void egui_view_virtual_viewport_apply_params(egui_view_t *self, const egui_view_virtual_viewport_params_t *params);
void egui_view_virtual_viewport_init_with_params(egui_view_t *self, const egui_view_virtual_viewport_params_t *params);

void egui_view_virtual_viewport_set_adapter(egui_view_t *self, const egui_view_virtual_viewport_adapter_t *adapter, void *adapter_context);
const egui_view_virtual_viewport_adapter_t *egui_view_virtual_viewport_get_adapter(egui_view_t *self);
void *egui_view_virtual_viewport_get_adapter_context(egui_view_t *self);

void egui_view_virtual_viewport_set_orientation(egui_view_t *self, uint8_t orientation);
uint8_t egui_view_virtual_viewport_get_orientation(egui_view_t *self);

void egui_view_virtual_viewport_set_overscan(egui_view_t *self, uint8_t before, uint8_t after);
uint8_t egui_view_virtual_viewport_get_overscan_before(egui_view_t *self);
uint8_t egui_view_virtual_viewport_get_overscan_after(egui_view_t *self);

void egui_view_virtual_viewport_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots);
uint8_t egui_view_virtual_viewport_get_keepalive_limit(egui_view_t *self);

void egui_view_virtual_viewport_set_estimated_item_extent(egui_view_t *self, int32_t extent);
int32_t egui_view_virtual_viewport_get_estimated_item_extent(egui_view_t *self);

void egui_view_virtual_viewport_set_logical_extent(egui_view_t *self, int32_t extent);
int32_t egui_view_virtual_viewport_get_logical_extent(egui_view_t *self);

void egui_view_virtual_viewport_set_logical_offset(egui_view_t *self, int32_t offset);
void egui_view_virtual_viewport_scroll_by(egui_view_t *self, int32_t delta);
void egui_view_virtual_viewport_scroll_to_item(egui_view_t *self, uint32_t index, int32_t item_offset);
void egui_view_virtual_viewport_scroll_to_stable_id(egui_view_t *self, uint32_t stable_id, int32_t item_offset);
int32_t egui_view_virtual_viewport_get_logical_offset(egui_view_t *self);
int32_t egui_view_virtual_viewport_find_item_index_by_stable_id(egui_view_t *self, uint32_t stable_id);
int32_t egui_view_virtual_viewport_get_item_main_origin(egui_view_t *self, uint32_t index);
int32_t egui_view_virtual_viewport_get_item_main_size(egui_view_t *self, uint32_t index);

void egui_view_virtual_viewport_set_anchor(egui_view_t *self, uint32_t stable_id, int32_t anchor_offset);
uint32_t egui_view_virtual_viewport_get_anchor_stable_id(egui_view_t *self);
int32_t egui_view_virtual_viewport_get_anchor_offset(egui_view_t *self);

void egui_view_virtual_viewport_notify_data_changed(egui_view_t *self);
void egui_view_virtual_viewport_notify_item_changed(egui_view_t *self, uint32_t index);
void egui_view_virtual_viewport_notify_item_changed_by_stable_id(egui_view_t *self, uint32_t stable_id);
void egui_view_virtual_viewport_notify_item_inserted(egui_view_t *self, uint32_t index, uint32_t count);
void egui_view_virtual_viewport_notify_item_removed(egui_view_t *self, uint32_t index, uint32_t count);
void egui_view_virtual_viewport_notify_item_moved(egui_view_t *self, uint32_t from_index, uint32_t to_index);
void egui_view_virtual_viewport_notify_item_resized(egui_view_t *self, uint32_t index);
void egui_view_virtual_viewport_notify_item_resized_by_stable_id(egui_view_t *self, uint32_t stable_id);

egui_view_t *egui_view_virtual_viewport_get_content_layer(egui_view_t *self);
uint8_t egui_view_virtual_viewport_get_slot_count(egui_view_t *self);
const egui_view_virtual_viewport_slot_t *egui_view_virtual_viewport_get_slot(egui_view_t *self, uint8_t slot_index);

void egui_view_virtual_viewport_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_VIRTUAL_VIEWPORT_H_ */
