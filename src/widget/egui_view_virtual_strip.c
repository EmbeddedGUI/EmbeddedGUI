#include "egui_view_virtual_strip.h"

#include <string.h>

static void egui_view_virtual_strip_sync_horizontal_mode(egui_view_t *self)
{
    egui_view_virtual_viewport_set_orientation(self, EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_HORIZONTAL);
}

static egui_view_virtual_strip_t *egui_view_virtual_strip_from_data_source_context(void *adapter_context)
{
    return (egui_view_virtual_strip_t *)adapter_context;
}

static const egui_view_virtual_strip_data_source_t *egui_view_virtual_strip_get_data_source_bridge(void *adapter_context, void **data_source_context)
{
    egui_view_virtual_strip_t *local = egui_view_virtual_strip_from_data_source_context(adapter_context);

    if (data_source_context != NULL)
    {
        *data_source_context = local->data_source_context;
    }

    return local->data_source;
}

static uint32_t egui_view_virtual_strip_default_stable_id(uint32_t index)
{
    return index + 1U;
}

static void egui_view_virtual_strip_reset_entry(egui_view_virtual_strip_entry_t *entry)
{
    if (entry == NULL)
    {
        return;
    }

    entry->index = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    entry->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static uint8_t egui_view_virtual_strip_resolve_item_by_index_internal(egui_view_t *self, uint32_t index, egui_view_virtual_strip_entry_t *entry)
{
    const egui_view_virtual_viewport_adapter_t *adapter = egui_view_virtual_viewport_get_adapter(self);
    void *adapter_context = egui_view_virtual_viewport_get_adapter_context(self);
    uint32_t count;

    egui_view_virtual_strip_reset_entry(entry);

    if (adapter == NULL || adapter->get_count == NULL)
    {
        return 0;
    }

    count = adapter->get_count(adapter_context);
    if (index >= count)
    {
        return 0;
    }

    if (entry != NULL)
    {
        entry->index = index;
        entry->stable_id = adapter->get_stable_id != NULL ? adapter->get_stable_id(adapter_context, index) : egui_view_virtual_strip_default_stable_id(index);
    }

    return 1;
}

static const egui_view_virtual_strip_slot_t *egui_view_virtual_strip_find_slot_by_view(egui_view_virtual_strip_t *local, egui_view_t *view)
{
    uint8_t slot_index;

    for (slot_index = 0; slot_index < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; slot_index++)
    {
        const egui_view_virtual_strip_slot_t *slot = &local->base.slots[slot_index];

        if (slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED && slot->view == view)
        {
            return slot;
        }
    }

    return NULL;
}

static int32_t egui_view_virtual_strip_find_slot_index_by_stable_id_internal(egui_view_virtual_strip_t *local, uint32_t stable_id)
{
    uint8_t slot_index;

    for (slot_index = 0; slot_index < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; slot_index++)
    {
        const egui_view_virtual_strip_slot_t *slot = &local->base.slots[slot_index];

        if (slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED && slot->stable_id == stable_id)
        {
            return (int32_t)slot_index;
        }
    }

    return -1;
}

static uint8_t egui_view_virtual_strip_resolve_slot_internal(egui_view_t *self, const egui_view_virtual_strip_slot_t *slot,
                                                             egui_view_virtual_strip_entry_t *entry)
{
    if (slot == NULL || slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED)
    {
        egui_view_virtual_strip_reset_entry(entry);
        return 0;
    }

    if (egui_view_virtual_strip_resolve_item_by_index_internal(self, slot->index, entry) && entry->stable_id == slot->stable_id)
    {
        return 1;
    }

    return egui_view_virtual_strip_resolve_item_by_stable_id(self, slot->stable_id, entry);
}

static uint32_t egui_view_virtual_strip_data_source_get_count(void *adapter_context)
{
    void *data_source_context;
    const egui_view_virtual_strip_data_source_t *data_source = egui_view_virtual_strip_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source == NULL || data_source->get_count == NULL)
    {
        return 0;
    }

    return data_source->get_count(data_source_context);
}

static uint32_t egui_view_virtual_strip_data_source_get_stable_id(void *adapter_context, uint32_t index)
{
    void *data_source_context;
    const egui_view_virtual_strip_data_source_t *data_source = egui_view_virtual_strip_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source != NULL && data_source->get_stable_id != NULL)
    {
        return data_source->get_stable_id(data_source_context, index);
    }

    return egui_view_virtual_strip_default_stable_id(index);
}

static int32_t egui_view_virtual_strip_data_source_find_index_by_stable_id(void *adapter_context, uint32_t stable_id)
{
    void *data_source_context;
    const egui_view_virtual_strip_data_source_t *data_source = egui_view_virtual_strip_get_data_source_bridge(adapter_context, &data_source_context);
    uint32_t count;
    uint32_t index;

    if (data_source != NULL && data_source->find_index_by_stable_id != NULL)
    {
        return data_source->find_index_by_stable_id(data_source_context, stable_id);
    }

    count = egui_view_virtual_strip_data_source_get_count(adapter_context);

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

static uint16_t egui_view_virtual_strip_data_source_get_view_type(void *adapter_context, uint32_t index)
{
    void *data_source_context;
    const egui_view_virtual_strip_data_source_t *data_source = egui_view_virtual_strip_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source != NULL && data_source->get_item_view_type != NULL)
    {
        return data_source->get_item_view_type(data_source_context, index);
    }

    if (data_source != NULL)
    {
        return data_source->default_item_view_type;
    }

    return 0;
}

static int32_t egui_view_virtual_strip_data_source_measure_main_size(void *adapter_context, uint32_t index, int32_t cross_size_hint)
{
    egui_view_virtual_strip_t *local = egui_view_virtual_strip_from_data_source_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_strip_data_source_t *data_source = egui_view_virtual_strip_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source != NULL && data_source->measure_item_width != NULL)
    {
        return data_source->measure_item_width(data_source_context, index, cross_size_hint);
    }

    return egui_view_virtual_strip_get_estimated_item_width(EGUI_VIEW_OF(local));
}

static egui_view_t *egui_view_virtual_strip_data_source_create_view(void *adapter_context, uint16_t view_type)
{
    void *data_source_context;
    const egui_view_virtual_strip_data_source_t *data_source = egui_view_virtual_strip_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source == NULL || data_source->create_item_view == NULL)
    {
        return NULL;
    }

    return data_source->create_item_view(data_source_context, view_type);
}

static void egui_view_virtual_strip_data_source_destroy_view(void *adapter_context, egui_view_t *view, uint16_t view_type)
{
    void *data_source_context;
    const egui_view_virtual_strip_data_source_t *data_source = egui_view_virtual_strip_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source != NULL && data_source->destroy_item_view != NULL)
    {
        data_source->destroy_item_view(data_source_context, view, view_type);
    }
}

static void egui_view_virtual_strip_data_source_bind_view(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id)
{
    void *data_source_context;
    const egui_view_virtual_strip_data_source_t *data_source = egui_view_virtual_strip_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source != NULL && data_source->bind_item_view != NULL)
    {
        data_source->bind_item_view(data_source_context, view, index, stable_id);
    }
}

static void egui_view_virtual_strip_data_source_unbind_view(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    void *data_source_context;
    const egui_view_virtual_strip_data_source_t *data_source = egui_view_virtual_strip_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source != NULL && data_source->unbind_item_view != NULL)
    {
        data_source->unbind_item_view(data_source_context, view, stable_id);
    }
}

static uint8_t egui_view_virtual_strip_data_source_should_keep_alive(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    void *data_source_context;
    const egui_view_virtual_strip_data_source_t *data_source = egui_view_virtual_strip_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source != NULL && data_source->should_keep_alive != NULL)
    {
        return data_source->should_keep_alive(data_source_context, view, stable_id);
    }

    return 0;
}

static void egui_view_virtual_strip_data_source_save_state(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    void *data_source_context;
    const egui_view_virtual_strip_data_source_t *data_source = egui_view_virtual_strip_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source != NULL && data_source->save_item_state != NULL)
    {
        data_source->save_item_state(data_source_context, view, stable_id);
    }
}

static void egui_view_virtual_strip_data_source_restore_state(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    void *data_source_context;
    const egui_view_virtual_strip_data_source_t *data_source = egui_view_virtual_strip_get_data_source_bridge(adapter_context, &data_source_context);

    if (data_source != NULL && data_source->restore_item_state != NULL)
    {
        data_source->restore_item_state(data_source_context, view, stable_id);
    }
}

void egui_view_virtual_strip_apply_params(egui_view_t *self, const egui_view_virtual_strip_params_t *params)
{
    egui_view_virtual_viewport_params_t viewport_params;

    viewport_params.region = params->region;
    viewport_params.orientation = EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_HORIZONTAL;
    viewport_params.overscan_before = params->overscan_before;
    viewport_params.overscan_after = params->overscan_after;
    viewport_params.max_keepalive_slots = params->max_keepalive_slots;
    viewport_params.estimated_item_extent = params->estimated_item_width;

    egui_view_virtual_viewport_apply_params(self, &viewport_params);
    egui_view_virtual_strip_sync_horizontal_mode(self);
}

void egui_view_virtual_strip_init_with_params(egui_view_t *self, const egui_view_virtual_strip_params_t *params)
{
    egui_view_virtual_strip_init(self);
    egui_view_virtual_strip_apply_params(self, params);
}

void egui_view_virtual_strip_apply_setup(egui_view_t *self, const egui_view_virtual_strip_setup_t *setup)
{
    if (setup == NULL)
    {
        return;
    }

    if (setup->params != NULL)
    {
        egui_view_virtual_strip_apply_params(self, setup->params);
    }

    egui_view_virtual_strip_set_data_source(self, setup->data_source, setup->data_source_context);
    egui_view_virtual_strip_set_state_cache_limits(self, setup->state_cache_max_entries, setup->state_cache_max_bytes);
}

void egui_view_virtual_strip_init_with_setup(egui_view_t *self, const egui_view_virtual_strip_setup_t *setup)
{
    egui_view_virtual_strip_init(self);
    egui_view_virtual_strip_apply_setup(self, setup);
}

void egui_view_virtual_strip_set_adapter(egui_view_t *self, const egui_view_virtual_strip_adapter_t *adapter, void *adapter_context)
{
    EGUI_LOCAL_INIT(egui_view_virtual_strip_t);

    local->data_source = NULL;
    local->data_source_context = NULL;
    egui_view_virtual_viewport_set_adapter(self, adapter, adapter_context);
}

const egui_view_virtual_strip_adapter_t *egui_view_virtual_strip_get_adapter(egui_view_t *self)
{
    return egui_view_virtual_viewport_get_adapter(self);
}

void *egui_view_virtual_strip_get_adapter_context(egui_view_t *self)
{
    return egui_view_virtual_viewport_get_adapter_context(self);
}

void egui_view_virtual_strip_set_data_source(egui_view_t *self, const egui_view_virtual_strip_data_source_t *data_source, void *data_source_context)
{
    EGUI_LOCAL_INIT(egui_view_virtual_strip_t);

    local->data_source = data_source;
    local->data_source_context = data_source_context;

    if (data_source == NULL)
    {
        memset(&local->data_source_adapter, 0, sizeof(local->data_source_adapter));
        egui_view_virtual_viewport_set_adapter(self, NULL, NULL);
        return;
    }

    local->data_source_adapter.get_count = egui_view_virtual_strip_data_source_get_count;
    local->data_source_adapter.get_stable_id = egui_view_virtual_strip_data_source_get_stable_id;
    local->data_source_adapter.find_index_by_stable_id = egui_view_virtual_strip_data_source_find_index_by_stable_id;
    local->data_source_adapter.get_view_type = egui_view_virtual_strip_data_source_get_view_type;
    local->data_source_adapter.measure_main_size = egui_view_virtual_strip_data_source_measure_main_size;
    local->data_source_adapter.create_view = egui_view_virtual_strip_data_source_create_view;
    local->data_source_adapter.destroy_view = egui_view_virtual_strip_data_source_destroy_view;
    local->data_source_adapter.bind_view = egui_view_virtual_strip_data_source_bind_view;
    local->data_source_adapter.unbind_view = egui_view_virtual_strip_data_source_unbind_view;
    local->data_source_adapter.should_keep_alive = egui_view_virtual_strip_data_source_should_keep_alive;
    local->data_source_adapter.save_state = egui_view_virtual_strip_data_source_save_state;
    local->data_source_adapter.restore_state = egui_view_virtual_strip_data_source_restore_state;

    egui_view_virtual_viewport_set_adapter(self, &local->data_source_adapter, local);
}

const egui_view_virtual_strip_data_source_t *egui_view_virtual_strip_get_data_source(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_strip_t);
    return local->data_source;
}

void *egui_view_virtual_strip_get_data_source_context(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_strip_t);
    return local->data_source_context;
}

uint32_t egui_view_virtual_strip_get_item_count(egui_view_t *self)
{
    const egui_view_virtual_viewport_adapter_t *adapter = egui_view_virtual_viewport_get_adapter(self);
    void *adapter_context = egui_view_virtual_viewport_get_adapter_context(self);

    if (adapter == NULL || adapter->get_count == NULL)
    {
        return 0;
    }

    return adapter->get_count(adapter_context);
}

void egui_view_virtual_strip_set_overscan(egui_view_t *self, uint8_t before, uint8_t after)
{
    egui_view_virtual_viewport_set_overscan(self, before, after);
}

uint8_t egui_view_virtual_strip_get_overscan_before(egui_view_t *self)
{
    return egui_view_virtual_viewport_get_overscan_before(self);
}

uint8_t egui_view_virtual_strip_get_overscan_after(egui_view_t *self)
{
    return egui_view_virtual_viewport_get_overscan_after(self);
}

void egui_view_virtual_strip_set_estimated_item_width(egui_view_t *self, int32_t width)
{
    egui_view_virtual_viewport_set_estimated_item_extent(self, width);
}

int32_t egui_view_virtual_strip_get_estimated_item_width(egui_view_t *self)
{
    return egui_view_virtual_viewport_get_estimated_item_extent(self);
}

void egui_view_virtual_strip_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots)
{
    egui_view_virtual_viewport_set_keepalive_limit(self, max_keepalive_slots);
}

uint8_t egui_view_virtual_strip_get_keepalive_limit(egui_view_t *self)
{
    return egui_view_virtual_viewport_get_keepalive_limit(self);
}

void egui_view_virtual_strip_set_state_cache_limits(egui_view_t *self, uint16_t max_entries, uint32_t max_bytes)
{
    egui_view_virtual_viewport_set_state_cache_limits(self, max_entries, max_bytes);
}

uint16_t egui_view_virtual_strip_get_state_cache_entry_limit(egui_view_t *self)
{
    return egui_view_virtual_viewport_get_state_cache_entry_limit(self);
}

uint32_t egui_view_virtual_strip_get_state_cache_byte_limit(egui_view_t *self)
{
    return egui_view_virtual_viewport_get_state_cache_byte_limit(self);
}

void egui_view_virtual_strip_clear_item_state_cache(egui_view_t *self)
{
    egui_view_virtual_viewport_clear_state_cache(self);
}

void egui_view_virtual_strip_remove_item_state_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    egui_view_virtual_viewport_remove_state_by_stable_id(self, stable_id);
}

uint8_t egui_view_virtual_strip_write_item_state(egui_view_t *self, uint32_t stable_id, const void *data, uint16_t size)
{
    return egui_view_virtual_viewport_write_state(self, stable_id, data, size);
}

uint16_t egui_view_virtual_strip_read_item_state(egui_view_t *self, uint32_t stable_id, void *data, uint16_t capacity)
{
    return egui_view_virtual_viewport_read_state(self, stable_id, data, capacity);
}

uint8_t egui_view_virtual_strip_write_item_state_for_view(egui_view_t *item_view, uint32_t stable_id, const void *data, uint16_t size)
{
    return egui_view_virtual_viewport_write_state_for_view(item_view, stable_id, data, size);
}

uint16_t egui_view_virtual_strip_read_item_state_for_view(egui_view_t *item_view, uint32_t stable_id, void *data, uint16_t capacity)
{
    return egui_view_virtual_viewport_read_state_for_view(item_view, stable_id, data, capacity);
}

void egui_view_virtual_strip_set_scroll_x(egui_view_t *self, int32_t offset)
{
    egui_view_virtual_viewport_set_logical_offset(self, offset);
}

void egui_view_virtual_strip_scroll_by(egui_view_t *self, int32_t delta)
{
    egui_view_virtual_viewport_scroll_by(self, delta);
}

void egui_view_virtual_strip_scroll_to_item(egui_view_t *self, uint32_t index, int32_t item_offset)
{
    egui_view_virtual_viewport_scroll_to_item(self, index, item_offset);
}

void egui_view_virtual_strip_scroll_to_stable_id(egui_view_t *self, uint32_t stable_id, int32_t item_offset)
{
    egui_view_virtual_viewport_scroll_to_stable_id(self, stable_id, item_offset);
}

int32_t egui_view_virtual_strip_get_scroll_x(egui_view_t *self)
{
    return egui_view_virtual_viewport_get_logical_offset(self);
}

int32_t egui_view_virtual_strip_find_index_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    return egui_view_virtual_viewport_find_item_index_by_stable_id(self, stable_id);
}

uint8_t egui_view_virtual_strip_resolve_item_by_stable_id(egui_view_t *self, uint32_t stable_id, egui_view_virtual_strip_entry_t *entry)
{
    int32_t index = egui_view_virtual_strip_find_index_by_stable_id(self, stable_id);

    egui_view_virtual_strip_reset_entry(entry);
    if (index < 0)
    {
        return 0;
    }

    return egui_view_virtual_strip_resolve_item_by_index_internal(self, (uint32_t)index, entry);
}

uint8_t egui_view_virtual_strip_resolve_item_by_view(egui_view_t *self, egui_view_t *item_view, egui_view_virtual_strip_entry_t *entry)
{
    EGUI_LOCAL_INIT(egui_view_virtual_strip_t);
    const egui_view_virtual_strip_slot_t *slot = egui_view_virtual_strip_find_slot_by_view(local, item_view);

    return egui_view_virtual_strip_resolve_slot_internal(self, slot, entry);
}

int32_t egui_view_virtual_strip_get_item_x(egui_view_t *self, uint32_t index)
{
    return egui_view_virtual_viewport_get_item_main_origin(self, index);
}

int32_t egui_view_virtual_strip_get_item_width(egui_view_t *self, uint32_t index)
{
    return egui_view_virtual_viewport_get_item_main_size(self, index);
}

int32_t egui_view_virtual_strip_get_item_x_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t index = egui_view_virtual_strip_find_index_by_stable_id(self, stable_id);

    if (index < 0)
    {
        return -1;
    }

    return egui_view_virtual_strip_get_item_x(self, (uint32_t)index);
}

int32_t egui_view_virtual_strip_get_item_width_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t index = egui_view_virtual_strip_find_index_by_stable_id(self, stable_id);

    if (index < 0)
    {
        return -1;
    }

    return egui_view_virtual_strip_get_item_width(self, (uint32_t)index);
}

uint8_t egui_view_virtual_strip_ensure_item_visible_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t inset)
{
    const egui_view_virtual_strip_slot_t *slot;

    if (stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID || egui_view_virtual_strip_find_index_by_stable_id(self, stable_id) < 0)
    {
        return 0;
    }

    slot = egui_view_virtual_strip_find_slot_by_stable_id(self, stable_id);
    if (egui_view_virtual_viewport_is_slot_fully_visible(self, slot, inset))
    {
        return 1;
    }

    egui_view_virtual_strip_scroll_to_stable_id(self, stable_id, inset);
    return 1;
}

void egui_view_virtual_strip_notify_data_changed(egui_view_t *self)
{
    egui_view_virtual_viewport_notify_data_changed(self);
}

void egui_view_virtual_strip_notify_item_changed(egui_view_t *self, uint32_t index)
{
    egui_view_virtual_viewport_notify_item_changed(self, index);
}

void egui_view_virtual_strip_notify_item_changed_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    egui_view_virtual_viewport_notify_item_changed_by_stable_id(self, stable_id);
}

void egui_view_virtual_strip_notify_item_inserted(egui_view_t *self, uint32_t index, uint32_t count)
{
    egui_view_virtual_viewport_notify_item_inserted(self, index, count);
}

void egui_view_virtual_strip_notify_item_removed(egui_view_t *self, uint32_t index, uint32_t count)
{
    egui_view_virtual_viewport_notify_item_removed(self, index, count);
}

void egui_view_virtual_strip_notify_item_moved(egui_view_t *self, uint32_t from_index, uint32_t to_index)
{
    egui_view_virtual_viewport_notify_item_moved(self, from_index, to_index);
}

void egui_view_virtual_strip_notify_item_resized(egui_view_t *self, uint32_t index)
{
    egui_view_virtual_viewport_notify_item_resized(self, index);
}

void egui_view_virtual_strip_notify_item_resized_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    egui_view_virtual_viewport_notify_item_resized_by_stable_id(self, stable_id);
}

uint8_t egui_view_virtual_strip_get_slot_count(egui_view_t *self)
{
    return egui_view_virtual_viewport_get_slot_count(self);
}

const egui_view_virtual_strip_slot_t *egui_view_virtual_strip_get_slot(egui_view_t *self, uint8_t slot_index)
{
    return egui_view_virtual_viewport_get_slot(self, slot_index);
}

int32_t egui_view_virtual_strip_find_slot_index_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    EGUI_LOCAL_INIT(egui_view_virtual_strip_t);
    return egui_view_virtual_strip_find_slot_index_by_stable_id_internal(local, stable_id);
}

const egui_view_virtual_strip_slot_t *egui_view_virtual_strip_find_slot_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t slot_index = egui_view_virtual_strip_find_slot_index_by_stable_id(self, stable_id);

    return slot_index >= 0 ? egui_view_virtual_strip_get_slot(self, (uint8_t)slot_index) : NULL;
}

egui_view_t *egui_view_virtual_strip_find_view_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    const egui_view_virtual_strip_slot_t *slot = egui_view_virtual_strip_find_slot_by_stable_id(self, stable_id);

    return slot != NULL ? slot->view : NULL;
}

uint8_t egui_view_virtual_strip_get_slot_entry(egui_view_t *self, uint8_t slot_index, egui_view_virtual_strip_entry_t *entry)
{
    const egui_view_virtual_strip_slot_t *slot = egui_view_virtual_strip_get_slot(self, slot_index);
    return egui_view_virtual_strip_resolve_slot_internal(self, slot, entry);
}

uint8_t egui_view_virtual_strip_visit_visible_items(egui_view_t *self, egui_view_virtual_strip_visible_item_visitor_t visitor, void *context)
{
    uint8_t slot_count;
    uint8_t slot_index;
    uint8_t visited = 0;

    if (visitor == NULL)
    {
        return 0;
    }

    slot_count = egui_view_virtual_strip_get_slot_count(self);
    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_strip_entry_t entry;
        const egui_view_virtual_strip_slot_t *slot = egui_view_virtual_strip_get_slot(self, slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL)
        {
            continue;
        }
        if (!egui_view_virtual_strip_get_slot_entry(self, slot_index, &entry))
        {
            continue;
        }

        visited++;
        if (!visitor(self, slot, &entry, slot->view, context))
        {
            break;
        }
    }

    return visited;
}

typedef struct egui_view_virtual_strip_find_visible_item_context
{
    egui_view_virtual_strip_visible_item_matcher_t matcher;
    void *context;
    uint32_t best_index;
    egui_view_t *best_view;
    egui_view_virtual_strip_entry_t best_entry;
} egui_view_virtual_strip_find_visible_item_context_t;

static uint8_t egui_view_virtual_strip_find_visible_item_visitor(egui_view_t *self, const egui_view_virtual_strip_slot_t *slot,
                                                                 const egui_view_virtual_strip_entry_t *entry, egui_view_t *item_view, void *context)
{
    egui_view_virtual_strip_find_visible_item_context_t *ctx = (egui_view_virtual_strip_find_visible_item_context_t *)context;

    if (entry == NULL || item_view == NULL)
    {
        return 1;
    }
    if (ctx->matcher != NULL && !ctx->matcher(self, slot, entry, item_view, ctx->context))
    {
        return 1;
    }
    if (ctx->best_view == NULL || entry->index < ctx->best_index)
    {
        ctx->best_index = entry->index;
        ctx->best_entry = *entry;
        ctx->best_view = item_view;
    }

    return 1;
}

egui_view_t *egui_view_virtual_strip_find_first_visible_item_view(egui_view_t *self, egui_view_virtual_strip_visible_item_matcher_t matcher, void *context,
                                                                  egui_view_virtual_strip_entry_t *entry_out)
{
    egui_view_virtual_strip_find_visible_item_context_t find_ctx = {
            .matcher = matcher,
            .context = context,
            .best_index = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID,
            .best_view = NULL,
    };

    egui_view_virtual_strip_visit_visible_items(self, egui_view_virtual_strip_find_visible_item_visitor, &find_ctx);
    if (find_ctx.best_view != NULL && entry_out != NULL)
    {
        *entry_out = find_ctx.best_entry;
    }

    return find_ctx.best_view;
}

void egui_view_virtual_strip_init(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_strip_t);

    egui_view_virtual_viewport_init(self);
    local->data_source = NULL;
    local->data_source_context = NULL;
    memset(&local->data_source_adapter, 0, sizeof(local->data_source_adapter));
    egui_view_virtual_strip_sync_horizontal_mode(self);
    egui_view_set_view_name(self, "egui_view_virtual_strip");
}
