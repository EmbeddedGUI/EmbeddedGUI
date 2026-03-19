#include "egui_view_virtual_list.h"

static void egui_view_virtual_list_sync_vertical_mode(egui_view_t *self)
{
    egui_view_virtual_viewport_set_orientation(self, EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_VERTICAL);
}

void egui_view_virtual_list_apply_params(egui_view_t *self, const egui_view_virtual_list_params_t *params)
{
    egui_view_virtual_viewport_params_t viewport_params;

    viewport_params.region = params->region;
    viewport_params.orientation = EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_VERTICAL;
    viewport_params.overscan_before = params->overscan_before;
    viewport_params.overscan_after = params->overscan_after;
    viewport_params.max_keepalive_slots = params->max_keepalive_slots;
    viewport_params.estimated_item_extent = params->estimated_item_height;

    egui_view_virtual_viewport_apply_params(self, &viewport_params);
    egui_view_virtual_list_sync_vertical_mode(self);
}

void egui_view_virtual_list_init_with_params(egui_view_t *self, const egui_view_virtual_list_params_t *params)
{
    egui_view_virtual_list_init(self);
    egui_view_virtual_list_apply_params(self, params);
}

void egui_view_virtual_list_set_adapter(egui_view_t *self, const egui_view_virtual_list_adapter_t *adapter, void *adapter_context)
{
    egui_view_virtual_viewport_set_adapter(self, adapter, adapter_context);
}

const egui_view_virtual_list_adapter_t *egui_view_virtual_list_get_adapter(egui_view_t *self)
{
    return egui_view_virtual_viewport_get_adapter(self);
}

void *egui_view_virtual_list_get_adapter_context(egui_view_t *self)
{
    return egui_view_virtual_viewport_get_adapter_context(self);
}

void egui_view_virtual_list_set_overscan(egui_view_t *self, uint8_t before, uint8_t after)
{
    egui_view_virtual_viewport_set_overscan(self, before, after);
}

uint8_t egui_view_virtual_list_get_overscan_before(egui_view_t *self)
{
    return egui_view_virtual_viewport_get_overscan_before(self);
}

uint8_t egui_view_virtual_list_get_overscan_after(egui_view_t *self)
{
    return egui_view_virtual_viewport_get_overscan_after(self);
}

void egui_view_virtual_list_set_estimated_item_height(egui_view_t *self, int32_t height)
{
    egui_view_virtual_viewport_set_estimated_item_extent(self, height);
}

int32_t egui_view_virtual_list_get_estimated_item_height(egui_view_t *self)
{
    return egui_view_virtual_viewport_get_estimated_item_extent(self);
}

void egui_view_virtual_list_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots)
{
    egui_view_virtual_viewport_set_keepalive_limit(self, max_keepalive_slots);
}

uint8_t egui_view_virtual_list_get_keepalive_limit(egui_view_t *self)
{
    return egui_view_virtual_viewport_get_keepalive_limit(self);
}

void egui_view_virtual_list_set_scroll_y(egui_view_t *self, int32_t offset)
{
    egui_view_virtual_viewport_set_logical_offset(self, offset);
}

void egui_view_virtual_list_scroll_by(egui_view_t *self, int32_t delta)
{
    egui_view_virtual_viewport_scroll_by(self, delta);
}

void egui_view_virtual_list_scroll_to_item(egui_view_t *self, uint32_t index, int32_t item_offset)
{
    egui_view_virtual_viewport_scroll_to_item(self, index, item_offset);
}

int32_t egui_view_virtual_list_get_scroll_y(egui_view_t *self)
{
    return egui_view_virtual_viewport_get_logical_offset(self);
}

int32_t egui_view_virtual_list_get_item_y(egui_view_t *self, uint32_t index)
{
    return egui_view_virtual_viewport_get_item_main_origin(self, index);
}

int32_t egui_view_virtual_list_get_item_height(egui_view_t *self, uint32_t index)
{
    return egui_view_virtual_viewport_get_item_main_size(self, index);
}

void egui_view_virtual_list_notify_data_changed(egui_view_t *self)
{
    egui_view_virtual_viewport_notify_data_changed(self);
}

void egui_view_virtual_list_notify_item_changed(egui_view_t *self, uint32_t index)
{
    egui_view_virtual_viewport_notify_item_changed(self, index);
}

void egui_view_virtual_list_notify_item_inserted(egui_view_t *self, uint32_t index, uint32_t count)
{
    egui_view_virtual_viewport_notify_item_inserted(self, index, count);
}

void egui_view_virtual_list_notify_item_removed(egui_view_t *self, uint32_t index, uint32_t count)
{
    egui_view_virtual_viewport_notify_item_removed(self, index, count);
}

void egui_view_virtual_list_notify_item_moved(egui_view_t *self, uint32_t from_index, uint32_t to_index)
{
    egui_view_virtual_viewport_notify_item_moved(self, from_index, to_index);
}

void egui_view_virtual_list_notify_item_resized(egui_view_t *self, uint32_t index)
{
    egui_view_virtual_viewport_notify_item_resized(self, index);
}

uint8_t egui_view_virtual_list_get_slot_count(egui_view_t *self)
{
    return egui_view_virtual_viewport_get_slot_count(self);
}

const egui_view_virtual_list_slot_t *egui_view_virtual_list_get_slot(egui_view_t *self, uint8_t slot_index)
{
    return egui_view_virtual_viewport_get_slot(self, slot_index);
}

void egui_view_virtual_list_init(egui_view_t *self)
{
    egui_view_virtual_viewport_init(self);
    egui_view_virtual_list_sync_vertical_mode(self);
    egui_view_set_view_name(self, "egui_view_virtual_list");
}
