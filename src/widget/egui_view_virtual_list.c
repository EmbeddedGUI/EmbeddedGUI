#include "egui_view_virtual_list.h"
#include <string.h>

static void egui_view_virtual_list_sync_vertical_mode(egui_view_t *self)
{
    egui_view_virtual_viewport_set_orientation(self, EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_VERTICAL);
}

static egui_view_virtual_list_t *egui_view_virtual_list_from_data_source_context(void *adapter_context)
{
    return (egui_view_virtual_list_t *)adapter_context;
}

static const egui_view_virtual_list_data_source_t *egui_view_virtual_list_get_data_source_bridge(void *adapter_context, void **data_source_context)
{
    egui_view_virtual_list_t *local = egui_view_virtual_list_from_data_source_context(adapter_context);

    if (data_source_context != NULL)
    {
        *data_source_context = local->data_source_context;
    }

    return local->data_source;
}

static uint32_t egui_view_virtual_list_default_stable_id(uint32_t index)
{
    return index + 1U;
}

static uint32_t egui_view_virtual_list_data_source_get_count(void *adapter_context)
{
    void *data_source_context;
    const egui_view_virtual_list_data_source_t *data_source = egui_view_virtual_list_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source == NULL || data_source->get_count == NULL)
    {
        return 0;
    }

    return data_source->get_count(data_source_context);
}

static uint32_t egui_view_virtual_list_data_source_get_stable_id(void *adapter_context, uint32_t index)
{
    void *data_source_context;
    const egui_view_virtual_list_data_source_t *data_source = egui_view_virtual_list_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source != NULL && data_source->get_stable_id != NULL)
    {
        return data_source->get_stable_id(data_source_context, index);
    }

    return egui_view_virtual_list_default_stable_id(index);
}

static int32_t egui_view_virtual_list_data_source_find_index_by_stable_id(void *adapter_context, uint32_t stable_id)
{
    void *data_source_context;
    const egui_view_virtual_list_data_source_t *data_source = egui_view_virtual_list_get_data_source_bridge(adapter_context, &data_source_context);
    uint32_t count;
    uint32_t index;

    if (data_source != NULL && data_source->find_index_by_stable_id != NULL)
    {
        return data_source->find_index_by_stable_id(data_source_context, stable_id);
    }

    count = egui_view_virtual_list_data_source_get_count(adapter_context);

    if (data_source != NULL && data_source->get_stable_id != NULL)
    {
        for (index = 0; index < count; index++)
        {
            if (data_source->get_stable_id(data_source_context, index) == stable_id)
            {
                return (int32_t)index;
            }
        }
        return -1;
    }

    if (stable_id == 0)
    {
        return -1;
    }

    index = stable_id - 1U;
    if (index >= count)
    {
        return -1;
    }

    return (int32_t)index;
}

static uint16_t egui_view_virtual_list_data_source_get_view_type(void *adapter_context, uint32_t index)
{
    void *data_source_context;
    const egui_view_virtual_list_data_source_t *data_source = egui_view_virtual_list_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source != NULL && data_source->get_view_type != NULL)
    {
        return data_source->get_view_type(data_source_context, index);
    }

    if (data_source != NULL)
    {
        return data_source->default_view_type;
    }

    return 0;
}

static int32_t egui_view_virtual_list_data_source_measure_main_size(void *adapter_context, uint32_t index, int32_t cross_size_hint)
{
    egui_view_virtual_list_t *local = egui_view_virtual_list_from_data_source_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_list_data_source_t *data_source = egui_view_virtual_list_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source != NULL && data_source->measure_item_height != NULL)
    {
        return data_source->measure_item_height(data_source_context, index, cross_size_hint);
    }

    return egui_view_virtual_list_get_estimated_item_height(EGUI_VIEW_OF(local));
}

static egui_view_t *egui_view_virtual_list_data_source_create_view(void *adapter_context, uint16_t view_type)
{
    void *data_source_context;
    const egui_view_virtual_list_data_source_t *data_source = egui_view_virtual_list_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source == NULL || data_source->create_item_view == NULL)
    {
        return NULL;
    }

    return data_source->create_item_view(data_source_context, view_type);
}

static void egui_view_virtual_list_data_source_destroy_view(void *adapter_context, egui_view_t *view, uint16_t view_type)
{
    void *data_source_context;
    const egui_view_virtual_list_data_source_t *data_source = egui_view_virtual_list_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source != NULL && data_source->destroy_item_view != NULL)
    {
        data_source->destroy_item_view(data_source_context, view, view_type);
    }
}

static void egui_view_virtual_list_data_source_bind_view(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id)
{
    void *data_source_context;
    const egui_view_virtual_list_data_source_t *data_source = egui_view_virtual_list_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source != NULL && data_source->bind_item_view != NULL)
    {
        data_source->bind_item_view(data_source_context, view, index, stable_id);
    }
}

static void egui_view_virtual_list_data_source_unbind_view(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    void *data_source_context;
    const egui_view_virtual_list_data_source_t *data_source = egui_view_virtual_list_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source != NULL && data_source->unbind_item_view != NULL)
    {
        data_source->unbind_item_view(data_source_context, view, stable_id);
    }
}

static uint8_t egui_view_virtual_list_data_source_should_keep_alive(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    void *data_source_context;
    const egui_view_virtual_list_data_source_t *data_source = egui_view_virtual_list_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source != NULL && data_source->should_keep_alive != NULL)
    {
        return data_source->should_keep_alive(data_source_context, view, stable_id);
    }

    return 0;
}

static void egui_view_virtual_list_data_source_save_state(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    void *data_source_context;
    const egui_view_virtual_list_data_source_t *data_source = egui_view_virtual_list_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source != NULL && data_source->save_item_state != NULL)
    {
        data_source->save_item_state(data_source_context, view, stable_id);
    }
}

static void egui_view_virtual_list_data_source_restore_state(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    void *data_source_context;
    const egui_view_virtual_list_data_source_t *data_source = egui_view_virtual_list_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source != NULL && data_source->restore_item_state != NULL)
    {
        data_source->restore_item_state(data_source_context, view, stable_id);
    }
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
    EGUI_LOCAL_INIT(egui_view_virtual_list_t);

    local->data_source = NULL;
    local->data_source_context = NULL;
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

void egui_view_virtual_list_set_data_source(egui_view_t *self, const egui_view_virtual_list_data_source_t *data_source, void *data_source_context)
{
    EGUI_LOCAL_INIT(egui_view_virtual_list_t);

    local->data_source = data_source;
    local->data_source_context = data_source_context;

    if (data_source == NULL)
    {
        memset(&local->data_source_adapter, 0, sizeof(local->data_source_adapter));
        egui_view_virtual_viewport_set_adapter(self, NULL, NULL);
        return;
    }

    local->data_source_adapter.get_count = egui_view_virtual_list_data_source_get_count;
    local->data_source_adapter.get_stable_id = egui_view_virtual_list_data_source_get_stable_id;
    local->data_source_adapter.find_index_by_stable_id = egui_view_virtual_list_data_source_find_index_by_stable_id;
    local->data_source_adapter.get_view_type = egui_view_virtual_list_data_source_get_view_type;
    local->data_source_adapter.measure_main_size = egui_view_virtual_list_data_source_measure_main_size;
    local->data_source_adapter.create_view = egui_view_virtual_list_data_source_create_view;
    local->data_source_adapter.destroy_view = egui_view_virtual_list_data_source_destroy_view;
    local->data_source_adapter.bind_view = egui_view_virtual_list_data_source_bind_view;
    local->data_source_adapter.unbind_view = egui_view_virtual_list_data_source_unbind_view;
    local->data_source_adapter.should_keep_alive = egui_view_virtual_list_data_source_should_keep_alive;
    local->data_source_adapter.save_state = egui_view_virtual_list_data_source_save_state;
    local->data_source_adapter.restore_state = egui_view_virtual_list_data_source_restore_state;

    egui_view_virtual_viewport_set_adapter(self, &local->data_source_adapter, local);
}

const egui_view_virtual_list_data_source_t *egui_view_virtual_list_get_data_source(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_list_t);
    return local->data_source;
}

void *egui_view_virtual_list_get_data_source_context(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_list_t);
    return local->data_source_context;
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
    EGUI_LOCAL_INIT(egui_view_virtual_list_t);

    egui_view_virtual_viewport_init(self);
    local->data_source = NULL;
    local->data_source_context = NULL;
    memset(&local->data_source_adapter, 0, sizeof(local->data_source_adapter));
    egui_view_virtual_list_sync_vertical_mode(self);
    egui_view_set_view_name(self, "egui_view_virtual_list");
}
