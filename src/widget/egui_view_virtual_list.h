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
typedef struct egui_view_virtual_list_params egui_view_virtual_list_params_t;

struct egui_view_virtual_list
{
    egui_view_virtual_viewport_t base;
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

void egui_view_virtual_list_apply_params(egui_view_t *self, const egui_view_virtual_list_params_t *params);
void egui_view_virtual_list_init_with_params(egui_view_t *self, const egui_view_virtual_list_params_t *params);

void egui_view_virtual_list_set_adapter(egui_view_t *self, const egui_view_virtual_list_adapter_t *adapter, void *adapter_context);
const egui_view_virtual_list_adapter_t *egui_view_virtual_list_get_adapter(egui_view_t *self);
void *egui_view_virtual_list_get_adapter_context(egui_view_t *self);

void egui_view_virtual_list_set_overscan(egui_view_t *self, uint8_t before, uint8_t after);
uint8_t egui_view_virtual_list_get_overscan_before(egui_view_t *self);
uint8_t egui_view_virtual_list_get_overscan_after(egui_view_t *self);

void egui_view_virtual_list_set_estimated_item_height(egui_view_t *self, int32_t height);
int32_t egui_view_virtual_list_get_estimated_item_height(egui_view_t *self);

void egui_view_virtual_list_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots);
uint8_t egui_view_virtual_list_get_keepalive_limit(egui_view_t *self);

void egui_view_virtual_list_set_scroll_y(egui_view_t *self, int32_t offset);
void egui_view_virtual_list_scroll_by(egui_view_t *self, int32_t delta);
void egui_view_virtual_list_scroll_to_item(egui_view_t *self, uint32_t index, int32_t item_offset);
int32_t egui_view_virtual_list_get_scroll_y(egui_view_t *self);
int32_t egui_view_virtual_list_get_item_y(egui_view_t *self, uint32_t index);
int32_t egui_view_virtual_list_get_item_height(egui_view_t *self, uint32_t index);

void egui_view_virtual_list_notify_data_changed(egui_view_t *self);
void egui_view_virtual_list_notify_item_changed(egui_view_t *self, uint32_t index);
void egui_view_virtual_list_notify_item_inserted(egui_view_t *self, uint32_t index, uint32_t count);
void egui_view_virtual_list_notify_item_removed(egui_view_t *self, uint32_t index, uint32_t count);
void egui_view_virtual_list_notify_item_moved(egui_view_t *self, uint32_t from_index, uint32_t to_index);
void egui_view_virtual_list_notify_item_resized(egui_view_t *self, uint32_t index);

uint8_t egui_view_virtual_list_get_slot_count(egui_view_t *self);
const egui_view_virtual_list_slot_t *egui_view_virtual_list_get_slot(egui_view_t *self, uint8_t slot_index);

void egui_view_virtual_list_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_VIRTUAL_LIST_H_ */
