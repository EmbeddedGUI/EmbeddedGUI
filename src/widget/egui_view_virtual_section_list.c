#include "egui_view_virtual_section_list.h"

#include <string.h>

#define EGUI_VIEW_VIRTUAL_SECTION_LIST_HEADER_VIEW_TYPE_FLAG 0x8000U
#define EGUI_VIEW_VIRTUAL_SECTION_LIST_VIEW_TYPE_MASK        0x7FFFU

static egui_view_virtual_section_list_t *egui_view_virtual_section_list_from_flat_context(void *adapter_context)
{
    return (egui_view_virtual_section_list_t *)adapter_context;
}

static const egui_view_virtual_section_list_data_source_t *egui_view_virtual_section_list_get_data_source_bridge(egui_view_virtual_section_list_t *local,
                                                                                                                 void **data_source_context)
{
    if (data_source_context != NULL)
    {
        *data_source_context = local->data_source_context;
    }

    return local->data_source;
}

static uint8_t egui_view_virtual_section_list_is_header_view_type(uint16_t view_type)
{
    return (uint8_t)((view_type & EGUI_VIEW_VIRTUAL_SECTION_LIST_HEADER_VIEW_TYPE_FLAG) != 0U);
}

static uint16_t egui_view_virtual_section_list_encode_view_type(uint8_t is_section_header, uint16_t view_type)
{
    view_type &= EGUI_VIEW_VIRTUAL_SECTION_LIST_VIEW_TYPE_MASK;
    if (is_section_header)
    {
        view_type |= EGUI_VIEW_VIRTUAL_SECTION_LIST_HEADER_VIEW_TYPE_FLAG;
    }
    return view_type;
}

static uint16_t egui_view_virtual_section_list_decode_view_type(uint16_t view_type)
{
    return (uint16_t)(view_type & EGUI_VIEW_VIRTUAL_SECTION_LIST_VIEW_TYPE_MASK);
}

static uint32_t egui_view_virtual_section_list_default_stable_id(uint32_t flat_index)
{
    return flat_index + 1U;
}

static void egui_view_virtual_section_list_reset_entry(egui_view_virtual_section_list_entry_t *entry)
{
    if (entry == NULL)
    {
        return;
    }

    entry->is_section_header = 0;
    entry->section_index = EGUI_VIEW_VIRTUAL_SECTION_LIST_INVALID_INDEX;
    entry->item_index = EGUI_VIEW_VIRTUAL_SECTION_LIST_INVALID_INDEX;
    entry->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static uint32_t egui_view_virtual_section_list_get_section_count_internal(egui_view_virtual_section_list_t *local)
{
    void *data_source_context;
    const egui_view_virtual_section_list_data_source_t *data_source = egui_view_virtual_section_list_get_data_source_bridge(local, &data_source_context);

    if (data_source == NULL || data_source->get_section_count == NULL)
    {
        return 0;
    }

    return data_source->get_section_count(data_source_context);
}

static uint32_t egui_view_virtual_section_list_get_item_count_internal(egui_view_virtual_section_list_t *local, uint32_t section_index)
{
    void *data_source_context;
    const egui_view_virtual_section_list_data_source_t *data_source = egui_view_virtual_section_list_get_data_source_bridge(local, &data_source_context);

    if (data_source == NULL || data_source->get_item_count == NULL)
    {
        return 0;
    }

    return data_source->get_item_count(data_source_context, section_index);
}

static int32_t egui_view_virtual_section_list_get_section_header_flat_index_internal(egui_view_virtual_section_list_t *local, uint32_t section_index)
{
    uint32_t i;
    uint32_t section_count = egui_view_virtual_section_list_get_section_count_internal(local);
    int32_t flat_index = 0;

    if (section_index >= section_count)
    {
        return -1;
    }

    for (i = 0; i < section_index; i++)
    {
        flat_index += 1 + (int32_t)egui_view_virtual_section_list_get_item_count_internal(local, i);
    }

    return flat_index;
}

static int32_t egui_view_virtual_section_list_get_item_flat_index_internal(egui_view_virtual_section_list_t *local, uint32_t section_index, uint32_t item_index)
{
    int32_t header_flat_index = egui_view_virtual_section_list_get_section_header_flat_index_internal(local, section_index);
    uint32_t item_count;

    if (header_flat_index < 0)
    {
        return -1;
    }

    item_count = egui_view_virtual_section_list_get_item_count_internal(local, section_index);
    if (item_index >= item_count)
    {
        return -1;
    }

    return header_flat_index + 1 + (int32_t)item_index;
}

static uint32_t egui_view_virtual_section_list_get_total_flat_count_internal(egui_view_virtual_section_list_t *local)
{
    uint32_t section_index;
    uint32_t section_count = egui_view_virtual_section_list_get_section_count_internal(local);
    uint32_t total = 0;

    for (section_index = 0; section_index < section_count; section_index++)
    {
        total += 1U + egui_view_virtual_section_list_get_item_count_internal(local, section_index);
    }

    return total;
}

static uint32_t egui_view_virtual_section_list_get_section_stable_id_internal(egui_view_virtual_section_list_t *local, uint32_t section_index)
{
    void *data_source_context;
    const egui_view_virtual_section_list_data_source_t *data_source = egui_view_virtual_section_list_get_data_source_bridge(local, &data_source_context);

    if (data_source != NULL && data_source->get_section_stable_id != NULL)
    {
        return data_source->get_section_stable_id(data_source_context, section_index);
    }

    return egui_view_virtual_section_list_default_stable_id(
            (uint32_t)egui_view_virtual_section_list_get_section_header_flat_index_internal(local, section_index));
}

static uint32_t egui_view_virtual_section_list_get_item_stable_id_internal(egui_view_virtual_section_list_t *local, uint32_t section_index, uint32_t item_index)
{
    void *data_source_context;
    const egui_view_virtual_section_list_data_source_t *data_source = egui_view_virtual_section_list_get_data_source_bridge(local, &data_source_context);

    if (data_source != NULL && data_source->get_item_stable_id != NULL)
    {
        return data_source->get_item_stable_id(data_source_context, section_index, item_index);
    }

    return egui_view_virtual_section_list_default_stable_id(
            (uint32_t)egui_view_virtual_section_list_get_item_flat_index_internal(local, section_index, item_index));
}

static uint8_t egui_view_virtual_section_list_resolve_flat_index_internal(egui_view_virtual_section_list_t *local, uint32_t flat_index,
                                                                          egui_view_virtual_section_list_entry_t *entry_info)
{
    uint32_t section_index;
    uint32_t section_count = egui_view_virtual_section_list_get_section_count_internal(local);
    uint32_t cursor = 0;

    for (section_index = 0; section_index < section_count; section_index++)
    {
        uint32_t item_count = egui_view_virtual_section_list_get_item_count_internal(local, section_index);

        if (flat_index == cursor)
        {
            if (entry_info != NULL)
            {
                entry_info->is_section_header = 1;
                entry_info->section_index = section_index;
                entry_info->item_index = EGUI_VIEW_VIRTUAL_SECTION_LIST_INVALID_INDEX;
                entry_info->stable_id = egui_view_virtual_section_list_get_section_stable_id_internal(local, section_index);
            }
            return 1;
        }

        cursor++;
        if (flat_index < cursor + item_count)
        {
            if (entry_info != NULL)
            {
                entry_info->is_section_header = 0;
                entry_info->section_index = section_index;
                entry_info->item_index = flat_index - cursor;
                entry_info->stable_id = egui_view_virtual_section_list_get_item_stable_id_internal(local, section_index, entry_info->item_index);
            }
            return 1;
        }

        cursor += item_count;
    }

    return 0;
}

static int32_t egui_view_virtual_section_list_find_section_index_by_stable_id_internal(egui_view_virtual_section_list_t *local, uint32_t stable_id)
{
    void *data_source_context;
    const egui_view_virtual_section_list_data_source_t *data_source = egui_view_virtual_section_list_get_data_source_bridge(local, &data_source_context);
    uint32_t section_index;
    uint32_t section_count;
    egui_view_virtual_section_list_entry_t entry_info;

    if (data_source != NULL && data_source->find_section_index_by_stable_id != NULL)
    {
        return data_source->find_section_index_by_stable_id(data_source_context, stable_id);
    }

    if (stable_id == 0U || stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        return -1;
    }

    if (data_source == NULL || data_source->get_section_stable_id == NULL)
    {
        if (!egui_view_virtual_section_list_resolve_flat_index_internal(local, stable_id - 1U, &entry_info) || !entry_info.is_section_header)
        {
            return -1;
        }

        return (int32_t)entry_info.section_index;
    }

    section_count = egui_view_virtual_section_list_get_section_count_internal(local);
    for (section_index = 0; section_index < section_count; section_index++)
    {
        if (data_source->get_section_stable_id(data_source_context, section_index) == stable_id)
        {
            return (int32_t)section_index;
        }
    }

    return -1;
}

static uint8_t egui_view_virtual_section_list_find_item_position_by_stable_id_internal(egui_view_virtual_section_list_t *local, uint32_t stable_id,
                                                                                       uint32_t *section_index, uint32_t *item_index)
{
    void *data_source_context;
    const egui_view_virtual_section_list_data_source_t *data_source = egui_view_virtual_section_list_get_data_source_bridge(local, &data_source_context);
    uint32_t current_section_index;
    uint32_t section_count;
    egui_view_virtual_section_list_entry_t entry_info;

    if (section_index != NULL)
    {
        *section_index = EGUI_VIEW_VIRTUAL_SECTION_LIST_INVALID_INDEX;
    }
    if (item_index != NULL)
    {
        *item_index = EGUI_VIEW_VIRTUAL_SECTION_LIST_INVALID_INDEX;
    }

    if (data_source != NULL && data_source->find_item_position_by_stable_id != NULL)
    {
        return data_source->find_item_position_by_stable_id(data_source_context, stable_id, section_index, item_index);
    }

    if (stable_id == 0U || stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        return 0;
    }

    if (data_source == NULL || data_source->get_item_stable_id == NULL)
    {
        if (!egui_view_virtual_section_list_resolve_flat_index_internal(local, stable_id - 1U, &entry_info) || entry_info.is_section_header)
        {
            return 0;
        }

        if (section_index != NULL)
        {
            *section_index = entry_info.section_index;
        }
        if (item_index != NULL)
        {
            *item_index = entry_info.item_index;
        }
        return 1;
    }

    section_count = egui_view_virtual_section_list_get_section_count_internal(local);
    for (current_section_index = 0; current_section_index < section_count; current_section_index++)
    {
        uint32_t current_item_index;
        uint32_t item_count = egui_view_virtual_section_list_get_item_count_internal(local, current_section_index);

        for (current_item_index = 0; current_item_index < item_count; current_item_index++)
        {
            if (data_source->get_item_stable_id(data_source_context, current_section_index, current_item_index) == stable_id)
            {
                if (section_index != NULL)
                {
                    *section_index = current_section_index;
                }
                if (item_index != NULL)
                {
                    *item_index = current_item_index;
                }
                return 1;
            }
        }
    }

    return 0;
}

static const egui_view_virtual_viewport_slot_t *egui_view_virtual_section_list_find_slot_by_view(egui_view_virtual_section_list_t *local, egui_view_t *view)
{
    uint8_t slot_index;

    for (slot_index = 0; slot_index < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; slot_index++)
    {
        const egui_view_virtual_viewport_slot_t *slot = &local->base.base.slots[slot_index];

        if (slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED && slot->view == view)
        {
            return slot;
        }
    }

    return NULL;
}

static int32_t egui_view_virtual_section_list_find_slot_index_by_stable_id_internal(egui_view_virtual_section_list_t *local, uint32_t stable_id)
{
    uint8_t slot_index;

    for (slot_index = 0; slot_index < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; slot_index++)
    {
        const egui_view_virtual_section_list_slot_t *slot = &local->base.base.slots[slot_index];

        if (slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED && slot->stable_id == stable_id)
        {
            return (int32_t)slot_index;
        }
    }

    return -1;
}

static uint8_t egui_view_virtual_section_list_resolve_entry_by_stable_id_internal(egui_view_virtual_section_list_t *local, uint32_t stable_id,
                                                                                  egui_view_virtual_section_list_entry_t *entry)
{
    int32_t flat_index;

    egui_view_virtual_section_list_reset_entry(entry);

    flat_index = egui_view_virtual_list_find_index_by_stable_id(EGUI_VIEW_OF(local), stable_id);
    if (flat_index < 0)
    {
        return 0;
    }

    return egui_view_virtual_section_list_resolve_flat_index_internal(local, (uint32_t)flat_index, entry);
}

static uint8_t egui_view_virtual_section_list_resolve_slot_internal(egui_view_virtual_section_list_t *local, const egui_view_virtual_section_list_slot_t *slot,
                                                                    egui_view_virtual_section_list_entry_t *entry)
{
    if (slot == NULL || slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED)
    {
        egui_view_virtual_section_list_reset_entry(entry);
        return 0;
    }

    if (egui_view_virtual_section_list_resolve_flat_index_internal(local, slot->index, entry) && entry->stable_id == slot->stable_id)
    {
        return 1;
    }

    return egui_view_virtual_section_list_resolve_entry_by_stable_id_internal(local, slot->stable_id, entry);
}

static uint8_t egui_view_virtual_section_list_try_resolve_kind(egui_view_virtual_section_list_t *local, egui_view_t *view, uint32_t stable_id,
                                                               uint8_t *is_section_header)
{
    const egui_view_virtual_viewport_slot_t *slot = egui_view_virtual_section_list_find_slot_by_view(local, view);
    int32_t section_index;

    if (slot != NULL)
    {
        *is_section_header = egui_view_virtual_section_list_is_header_view_type(slot->view_type);
        return 1;
    }

    section_index = egui_view_virtual_section_list_find_section_index_by_stable_id_internal(local, stable_id);
    if (section_index >= 0)
    {
        *is_section_header = 1;
        return 1;
    }

    if (egui_view_virtual_section_list_find_item_position_by_stable_id_internal(local, stable_id, NULL, NULL))
    {
        *is_section_header = 0;
        return 1;
    }

    return 0;
}

static uint32_t egui_view_virtual_section_list_flat_get_count(void *adapter_context)
{
    egui_view_virtual_section_list_t *local = egui_view_virtual_section_list_from_flat_context(adapter_context);
    return egui_view_virtual_section_list_get_total_flat_count_internal(local);
}

static uint32_t egui_view_virtual_section_list_flat_get_stable_id(void *adapter_context, uint32_t index)
{
    egui_view_virtual_section_list_t *local = egui_view_virtual_section_list_from_flat_context(adapter_context);
    egui_view_virtual_section_list_entry_t entry_info;

    if (egui_view_virtual_section_list_resolve_flat_index_internal(local, index, &entry_info))
    {
        return entry_info.stable_id;
    }

    return egui_view_virtual_section_list_default_stable_id(index);
}

static int32_t egui_view_virtual_section_list_flat_find_index_by_stable_id(void *adapter_context, uint32_t stable_id)
{
    egui_view_virtual_section_list_t *local = egui_view_virtual_section_list_from_flat_context(adapter_context);
    uint32_t section_index;
    uint32_t item_index;
    int32_t header_index;

    header_index = egui_view_virtual_section_list_find_section_index_by_stable_id_internal(local, stable_id);
    if (header_index >= 0)
    {
        return egui_view_virtual_section_list_get_section_header_flat_index_internal(local, (uint32_t)header_index);
    }

    if (!egui_view_virtual_section_list_find_item_position_by_stable_id_internal(local, stable_id, &section_index, &item_index))
    {
        return -1;
    }

    return egui_view_virtual_section_list_get_item_flat_index_internal(local, section_index, item_index);
}

static uint16_t egui_view_virtual_section_list_flat_get_view_type(void *adapter_context, uint32_t index)
{
    egui_view_virtual_section_list_t *local = egui_view_virtual_section_list_from_flat_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_section_list_data_source_t *data_source = egui_view_virtual_section_list_get_data_source_bridge(local, &data_source_context);
    egui_view_virtual_section_list_entry_t entry_info;
    uint16_t view_type = 0;

    if (!egui_view_virtual_section_list_resolve_flat_index_internal(local, index, &entry_info))
    {
        return 0;
    }

    if (entry_info.is_section_header)
    {
        if (data_source != NULL && data_source->get_section_header_view_type != NULL)
        {
            view_type = data_source->get_section_header_view_type(data_source_context, entry_info.section_index);
        }
        else if (data_source != NULL)
        {
            view_type = data_source->default_section_header_view_type;
        }
    }
    else
    {
        if (data_source != NULL && data_source->get_item_view_type != NULL)
        {
            view_type = data_source->get_item_view_type(data_source_context, entry_info.section_index, entry_info.item_index);
        }
        else if (data_source != NULL)
        {
            view_type = data_source->default_item_view_type;
        }
    }

    return egui_view_virtual_section_list_encode_view_type(entry_info.is_section_header, view_type);
}

static int32_t egui_view_virtual_section_list_flat_measure_item_height(void *adapter_context, uint32_t index, int32_t width_hint)
{
    egui_view_virtual_section_list_t *local = egui_view_virtual_section_list_from_flat_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_section_list_data_source_t *data_source = egui_view_virtual_section_list_get_data_source_bridge(local, &data_source_context);
    egui_view_virtual_section_list_entry_t entry_info;

    if (!egui_view_virtual_section_list_resolve_flat_index_internal(local, index, &entry_info))
    {
        return egui_view_virtual_section_list_get_estimated_entry_height(EGUI_VIEW_OF(local));
    }

    if (entry_info.is_section_header)
    {
        if (data_source != NULL && data_source->measure_section_header_height != NULL)
        {
            return data_source->measure_section_header_height(data_source_context, entry_info.section_index, width_hint);
        }
    }
    else if (data_source != NULL && data_source->measure_item_height != NULL)
    {
        return data_source->measure_item_height(data_source_context, entry_info.section_index, entry_info.item_index, width_hint);
    }

    return egui_view_virtual_section_list_get_estimated_entry_height(EGUI_VIEW_OF(local));
}

static egui_view_t *egui_view_virtual_section_list_flat_create_item_view(void *adapter_context, uint16_t view_type)
{
    egui_view_virtual_section_list_t *local = egui_view_virtual_section_list_from_flat_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_section_list_data_source_t *data_source = egui_view_virtual_section_list_get_data_source_bridge(local, &data_source_context);

    if (data_source == NULL)
    {
        return NULL;
    }

    if (egui_view_virtual_section_list_is_header_view_type(view_type))
    {
        if (data_source->create_section_header_view == NULL)
        {
            return NULL;
        }

        return data_source->create_section_header_view(data_source_context, egui_view_virtual_section_list_decode_view_type(view_type));
    }

    if (data_source->create_item_view == NULL)
    {
        return NULL;
    }

    return data_source->create_item_view(data_source_context, egui_view_virtual_section_list_decode_view_type(view_type));
}

static void egui_view_virtual_section_list_flat_destroy_item_view(void *adapter_context, egui_view_t *view, uint16_t view_type)
{
    egui_view_virtual_section_list_t *local = egui_view_virtual_section_list_from_flat_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_section_list_data_source_t *data_source = egui_view_virtual_section_list_get_data_source_bridge(local, &data_source_context);

    if (data_source == NULL)
    {
        return;
    }

    if (egui_view_virtual_section_list_is_header_view_type(view_type))
    {
        if (data_source->destroy_section_header_view != NULL)
        {
            data_source->destroy_section_header_view(data_source_context, view, egui_view_virtual_section_list_decode_view_type(view_type));
        }
        return;
    }

    if (data_source->destroy_item_view != NULL)
    {
        data_source->destroy_item_view(data_source_context, view, egui_view_virtual_section_list_decode_view_type(view_type));
    }
}

static void egui_view_virtual_section_list_flat_bind_item_view(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id)
{
    egui_view_virtual_section_list_t *local = egui_view_virtual_section_list_from_flat_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_section_list_data_source_t *data_source = egui_view_virtual_section_list_get_data_source_bridge(local, &data_source_context);
    egui_view_virtual_section_list_entry_t entry_info;

    if (data_source == NULL || !egui_view_virtual_section_list_resolve_flat_index_internal(local, index, &entry_info))
    {
        return;
    }

    if (entry_info.is_section_header)
    {
        if (data_source->bind_section_header_view != NULL)
        {
            data_source->bind_section_header_view(data_source_context, view, entry_info.section_index, stable_id);
        }
        return;
    }

    if (data_source->bind_item_view != NULL)
    {
        data_source->bind_item_view(data_source_context, view, entry_info.section_index, entry_info.item_index, stable_id);
    }
}

static void egui_view_virtual_section_list_flat_unbind_item_view(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_virtual_section_list_t *local = egui_view_virtual_section_list_from_flat_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_section_list_data_source_t *data_source = egui_view_virtual_section_list_get_data_source_bridge(local, &data_source_context);
    uint8_t is_section_header;

    if (data_source == NULL || !egui_view_virtual_section_list_try_resolve_kind(local, view, stable_id, &is_section_header))
    {
        return;
    }

    if (is_section_header)
    {
        if (data_source->unbind_section_header_view != NULL)
        {
            data_source->unbind_section_header_view(data_source_context, view, stable_id);
        }
        return;
    }

    if (data_source->unbind_item_view != NULL)
    {
        data_source->unbind_item_view(data_source_context, view, stable_id);
    }
}

static uint8_t egui_view_virtual_section_list_flat_should_keep_alive(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_virtual_section_list_t *local = egui_view_virtual_section_list_from_flat_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_section_list_data_source_t *data_source = egui_view_virtual_section_list_get_data_source_bridge(local, &data_source_context);
    uint8_t is_section_header;

    if (data_source == NULL || !egui_view_virtual_section_list_try_resolve_kind(local, view, stable_id, &is_section_header))
    {
        return 0;
    }

    if (is_section_header)
    {
        if (data_source->should_keep_section_header_alive != NULL)
        {
            return data_source->should_keep_section_header_alive(data_source_context, view, stable_id);
        }
        return 0;
    }

    if (data_source->should_keep_item_alive != NULL)
    {
        return data_source->should_keep_item_alive(data_source_context, view, stable_id);
    }

    return 0;
}

static void egui_view_virtual_section_list_flat_save_item_state(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_virtual_section_list_t *local = egui_view_virtual_section_list_from_flat_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_section_list_data_source_t *data_source = egui_view_virtual_section_list_get_data_source_bridge(local, &data_source_context);
    uint8_t is_section_header;

    if (data_source == NULL || !egui_view_virtual_section_list_try_resolve_kind(local, view, stable_id, &is_section_header))
    {
        return;
    }

    if (is_section_header)
    {
        if (data_source->save_section_header_state != NULL)
        {
            data_source->save_section_header_state(data_source_context, view, stable_id);
        }
        return;
    }

    if (data_source->save_item_state != NULL)
    {
        data_source->save_item_state(data_source_context, view, stable_id);
    }
}

static void egui_view_virtual_section_list_flat_restore_item_state(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_virtual_section_list_t *local = egui_view_virtual_section_list_from_flat_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_section_list_data_source_t *data_source = egui_view_virtual_section_list_get_data_source_bridge(local, &data_source_context);
    uint8_t is_section_header;

    if (data_source == NULL || !egui_view_virtual_section_list_try_resolve_kind(local, view, stable_id, &is_section_header))
    {
        return;
    }

    if (is_section_header)
    {
        if (data_source->restore_section_header_state != NULL)
        {
            data_source->restore_section_header_state(data_source_context, view, stable_id);
        }
        return;
    }

    if (data_source->restore_item_state != NULL)
    {
        data_source->restore_item_state(data_source_context, view, stable_id);
    }
}

void egui_view_virtual_section_list_apply_params(egui_view_t *self, const egui_view_virtual_section_list_params_t *params)
{
    egui_view_virtual_list_params_t list_params;

    list_params.region = params->region;
    list_params.overscan_before = params->overscan_before;
    list_params.overscan_after = params->overscan_after;
    list_params.max_keepalive_slots = params->max_keepalive_slots;
    list_params.estimated_item_height = params->estimated_entry_height;

    egui_view_virtual_list_apply_params(self, &list_params);
}

void egui_view_virtual_section_list_init_with_params(egui_view_t *self, const egui_view_virtual_section_list_params_t *params)
{
    egui_view_virtual_section_list_init(self);
    egui_view_virtual_section_list_apply_params(self, params);
}

void egui_view_virtual_section_list_apply_setup(egui_view_t *self, const egui_view_virtual_section_list_setup_t *setup)
{
    if (setup == NULL)
    {
        return;
    }

    if (setup->params != NULL)
    {
        egui_view_virtual_section_list_apply_params(self, setup->params);
    }

    egui_view_virtual_section_list_set_data_source(self, setup->data_source, setup->data_source_context);
    egui_view_virtual_section_list_set_state_cache_limits(self, setup->state_cache_max_entries, setup->state_cache_max_bytes);
}

void egui_view_virtual_section_list_init_with_setup(egui_view_t *self, const egui_view_virtual_section_list_setup_t *setup)
{
    egui_view_virtual_section_list_init(self);
    egui_view_virtual_section_list_apply_setup(self, setup);
}

void egui_view_virtual_section_list_set_data_source(egui_view_t *self, const egui_view_virtual_section_list_data_source_t *data_source,
                                                    void *data_source_context)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);

    local->data_source = data_source;
    local->data_source_context = data_source_context;

    if (data_source == NULL)
    {
        memset(&local->flat_data_source, 0, sizeof(local->flat_data_source));
        egui_view_virtual_list_set_data_source(self, NULL, NULL);
        return;
    }

    local->flat_data_source.get_count = egui_view_virtual_section_list_flat_get_count;
    local->flat_data_source.get_stable_id = egui_view_virtual_section_list_flat_get_stable_id;
    local->flat_data_source.find_index_by_stable_id = egui_view_virtual_section_list_flat_find_index_by_stable_id;
    local->flat_data_source.get_view_type = egui_view_virtual_section_list_flat_get_view_type;
    local->flat_data_source.measure_item_height = egui_view_virtual_section_list_flat_measure_item_height;
    local->flat_data_source.create_item_view = egui_view_virtual_section_list_flat_create_item_view;
    local->flat_data_source.destroy_item_view = egui_view_virtual_section_list_flat_destroy_item_view;
    local->flat_data_source.bind_item_view = egui_view_virtual_section_list_flat_bind_item_view;
    local->flat_data_source.unbind_item_view = egui_view_virtual_section_list_flat_unbind_item_view;
    local->flat_data_source.should_keep_alive = egui_view_virtual_section_list_flat_should_keep_alive;
    local->flat_data_source.save_item_state = egui_view_virtual_section_list_flat_save_item_state;
    local->flat_data_source.restore_item_state = egui_view_virtual_section_list_flat_restore_item_state;
    local->flat_data_source.default_view_type = 0;

    egui_view_virtual_list_set_data_source(self, &local->flat_data_source, local);
}

const egui_view_virtual_section_list_data_source_t *egui_view_virtual_section_list_get_data_source(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    return local->data_source;
}

void *egui_view_virtual_section_list_get_data_source_context(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    return local->data_source_context;
}

uint32_t egui_view_virtual_section_list_get_section_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    return egui_view_virtual_section_list_get_section_count_internal(local);
}

uint32_t egui_view_virtual_section_list_get_item_count(egui_view_t *self, uint32_t section_index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    return egui_view_virtual_section_list_get_item_count_internal(local, section_index);
}

void egui_view_virtual_section_list_set_overscan(egui_view_t *self, uint8_t before, uint8_t after)
{
    egui_view_virtual_list_set_overscan(self, before, after);
}

uint8_t egui_view_virtual_section_list_get_overscan_before(egui_view_t *self)
{
    return egui_view_virtual_list_get_overscan_before(self);
}

uint8_t egui_view_virtual_section_list_get_overscan_after(egui_view_t *self)
{
    return egui_view_virtual_list_get_overscan_after(self);
}

void egui_view_virtual_section_list_set_estimated_entry_height(egui_view_t *self, int32_t height)
{
    egui_view_virtual_list_set_estimated_item_height(self, height);
}

int32_t egui_view_virtual_section_list_get_estimated_entry_height(egui_view_t *self)
{
    return egui_view_virtual_list_get_estimated_item_height(self);
}

void egui_view_virtual_section_list_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots)
{
    egui_view_virtual_list_set_keepalive_limit(self, max_keepalive_slots);
}

uint8_t egui_view_virtual_section_list_get_keepalive_limit(egui_view_t *self)
{
    return egui_view_virtual_list_get_keepalive_limit(self);
}

void egui_view_virtual_section_list_set_state_cache_limits(egui_view_t *self, uint16_t max_entries, uint32_t max_bytes)
{
    egui_view_virtual_list_set_state_cache_limits(self, max_entries, max_bytes);
}

uint16_t egui_view_virtual_section_list_get_state_cache_entry_limit(egui_view_t *self)
{
    return egui_view_virtual_list_get_state_cache_entry_limit(self);
}

uint32_t egui_view_virtual_section_list_get_state_cache_byte_limit(egui_view_t *self)
{
    return egui_view_virtual_list_get_state_cache_byte_limit(self);
}

void egui_view_virtual_section_list_clear_entry_state_cache(egui_view_t *self)
{
    egui_view_virtual_list_clear_item_state_cache(self);
}

void egui_view_virtual_section_list_remove_entry_state_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    egui_view_virtual_list_remove_item_state_by_stable_id(self, stable_id);
}

uint8_t egui_view_virtual_section_list_write_entry_state(egui_view_t *self, uint32_t stable_id, const void *data, uint16_t size)
{
    return egui_view_virtual_list_write_item_state(self, stable_id, data, size);
}

uint16_t egui_view_virtual_section_list_read_entry_state(egui_view_t *self, uint32_t stable_id, void *data, uint16_t capacity)
{
    return egui_view_virtual_list_read_item_state(self, stable_id, data, capacity);
}

uint8_t egui_view_virtual_section_list_write_entry_state_for_view(egui_view_t *entry_view, uint32_t stable_id, const void *data, uint16_t size)
{
    return egui_view_virtual_list_write_item_state_for_view(entry_view, stable_id, data, size);
}

uint16_t egui_view_virtual_section_list_read_entry_state_for_view(egui_view_t *entry_view, uint32_t stable_id, void *data, uint16_t capacity)
{
    return egui_view_virtual_list_read_item_state_for_view(entry_view, stable_id, data, capacity);
}

void egui_view_virtual_section_list_set_scroll_y(egui_view_t *self, int32_t offset)
{
    egui_view_virtual_list_set_scroll_y(self, offset);
}

void egui_view_virtual_section_list_scroll_by(egui_view_t *self, int32_t delta)
{
    egui_view_virtual_list_scroll_by(self, delta);
}

void egui_view_virtual_section_list_scroll_to_section(egui_view_t *self, uint32_t section_index, int32_t section_offset)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    int32_t flat_index = egui_view_virtual_section_list_get_section_header_flat_index_internal(local, section_index);

    if (flat_index < 0)
    {
        return;
    }

    egui_view_virtual_list_scroll_to_item(self, (uint32_t)flat_index, section_offset);
}

void egui_view_virtual_section_list_scroll_to_section_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t section_offset)
{
    int32_t section_index = egui_view_virtual_section_list_find_section_index_by_stable_id(self, stable_id);

    if (section_index < 0)
    {
        return;
    }

    egui_view_virtual_section_list_scroll_to_section(self, (uint32_t)section_index, section_offset);
}

void egui_view_virtual_section_list_scroll_to_item(egui_view_t *self, uint32_t section_index, uint32_t item_index, int32_t item_offset)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    int32_t flat_index = egui_view_virtual_section_list_get_item_flat_index_internal(local, section_index, item_index);

    if (flat_index < 0)
    {
        return;
    }

    egui_view_virtual_list_scroll_to_item(self, (uint32_t)flat_index, item_offset);
}

void egui_view_virtual_section_list_scroll_to_item_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t item_offset)
{
    uint32_t section_index;
    uint32_t item_index;

    if (!egui_view_virtual_section_list_find_item_position_by_stable_id(self, stable_id, &section_index, &item_index))
    {
        return;
    }

    egui_view_virtual_section_list_scroll_to_item(self, section_index, item_index, item_offset);
}

int32_t egui_view_virtual_section_list_get_scroll_y(egui_view_t *self)
{
    return egui_view_virtual_list_get_scroll_y(self);
}

int32_t egui_view_virtual_section_list_find_section_index_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    return egui_view_virtual_section_list_find_section_index_by_stable_id_internal(local, stable_id);
}

uint8_t egui_view_virtual_section_list_find_item_position_by_stable_id(egui_view_t *self, uint32_t stable_id, uint32_t *section_index, uint32_t *item_index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    return egui_view_virtual_section_list_find_item_position_by_stable_id_internal(local, stable_id, section_index, item_index);
}

uint8_t egui_view_virtual_section_list_resolve_entry_by_stable_id(egui_view_t *self, uint32_t stable_id, egui_view_virtual_section_list_entry_t *entry)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    return egui_view_virtual_section_list_resolve_entry_by_stable_id_internal(local, stable_id, entry);
}

uint8_t egui_view_virtual_section_list_resolve_entry_by_view(egui_view_t *self, egui_view_t *entry_view, egui_view_virtual_section_list_entry_t *entry)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    const egui_view_virtual_section_list_slot_t *slot = egui_view_virtual_section_list_find_slot_by_view(local, entry_view);

    return egui_view_virtual_section_list_resolve_slot_internal(local, slot, entry);
}

int32_t egui_view_virtual_section_list_get_section_header_y(egui_view_t *self, uint32_t section_index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    int32_t flat_index = egui_view_virtual_section_list_get_section_header_flat_index_internal(local, section_index);

    if (flat_index < 0)
    {
        return 0;
    }

    return egui_view_virtual_list_get_item_y(self, (uint32_t)flat_index);
}

int32_t egui_view_virtual_section_list_get_section_header_height(egui_view_t *self, uint32_t section_index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    int32_t flat_index = egui_view_virtual_section_list_get_section_header_flat_index_internal(local, section_index);

    if (flat_index < 0)
    {
        return 0;
    }

    return egui_view_virtual_list_get_item_height(self, (uint32_t)flat_index);
}

int32_t egui_view_virtual_section_list_get_item_y(egui_view_t *self, uint32_t section_index, uint32_t item_index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    int32_t flat_index = egui_view_virtual_section_list_get_item_flat_index_internal(local, section_index, item_index);

    if (flat_index < 0)
    {
        return 0;
    }

    return egui_view_virtual_list_get_item_y(self, (uint32_t)flat_index);
}

int32_t egui_view_virtual_section_list_get_item_height(egui_view_t *self, uint32_t section_index, uint32_t item_index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    int32_t flat_index = egui_view_virtual_section_list_get_item_flat_index_internal(local, section_index, item_index);

    if (flat_index < 0)
    {
        return 0;
    }

    return egui_view_virtual_list_get_item_height(self, (uint32_t)flat_index);
}

int32_t egui_view_virtual_section_list_get_entry_y_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t flat_index = egui_view_virtual_list_find_index_by_stable_id(self, stable_id);

    if (flat_index < 0)
    {
        return -1;
    }

    return egui_view_virtual_list_get_item_y(self, (uint32_t)flat_index);
}

int32_t egui_view_virtual_section_list_get_entry_height_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t flat_index = egui_view_virtual_list_find_index_by_stable_id(self, stable_id);

    if (flat_index < 0)
    {
        return -1;
    }

    return egui_view_virtual_list_get_item_height(self, (uint32_t)flat_index);
}

uint8_t egui_view_virtual_section_list_ensure_entry_visible_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t inset)
{
    return egui_view_virtual_list_ensure_item_visible_by_stable_id(self, stable_id, inset);
}

void egui_view_virtual_section_list_notify_data_changed(egui_view_t *self)
{
    egui_view_virtual_list_notify_data_changed(self);
}

void egui_view_virtual_section_list_notify_section_header_changed(egui_view_t *self, uint32_t section_index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    int32_t flat_index = egui_view_virtual_section_list_get_section_header_flat_index_internal(local, section_index);

    if (flat_index < 0)
    {
        return;
    }

    egui_view_virtual_list_notify_item_changed(self, (uint32_t)flat_index);
}

void egui_view_virtual_section_list_notify_section_header_changed_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t section_index = egui_view_virtual_section_list_find_section_index_by_stable_id(self, stable_id);

    if (section_index < 0)
    {
        return;
    }

    egui_view_virtual_section_list_notify_section_header_changed(self, (uint32_t)section_index);
}

void egui_view_virtual_section_list_notify_section_header_resized(egui_view_t *self, uint32_t section_index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    int32_t flat_index = egui_view_virtual_section_list_get_section_header_flat_index_internal(local, section_index);

    if (flat_index < 0)
    {
        return;
    }

    egui_view_virtual_list_notify_item_resized(self, (uint32_t)flat_index);
}

void egui_view_virtual_section_list_notify_section_header_resized_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t section_index = egui_view_virtual_section_list_find_section_index_by_stable_id(self, stable_id);

    if (section_index < 0)
    {
        return;
    }

    egui_view_virtual_section_list_notify_section_header_resized(self, (uint32_t)section_index);
}

void egui_view_virtual_section_list_notify_item_changed(egui_view_t *self, uint32_t section_index, uint32_t item_index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    int32_t flat_index = egui_view_virtual_section_list_get_item_flat_index_internal(local, section_index, item_index);

    if (flat_index < 0)
    {
        return;
    }

    egui_view_virtual_list_notify_item_changed(self, (uint32_t)flat_index);
}

void egui_view_virtual_section_list_notify_item_changed_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    egui_view_virtual_list_notify_item_changed_by_stable_id(self, stable_id);
}

void egui_view_virtual_section_list_notify_item_resized(egui_view_t *self, uint32_t section_index, uint32_t item_index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    int32_t flat_index = egui_view_virtual_section_list_get_item_flat_index_internal(local, section_index, item_index);

    if (flat_index < 0)
    {
        return;
    }

    egui_view_virtual_list_notify_item_resized(self, (uint32_t)flat_index);
}

void egui_view_virtual_section_list_notify_item_resized_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    egui_view_virtual_list_notify_item_resized_by_stable_id(self, stable_id);
}

void egui_view_virtual_section_list_notify_section_inserted(egui_view_t *self, uint32_t section_index, uint32_t count)
{
    EGUI_UNUSED(section_index);
    EGUI_UNUSED(count);
    egui_view_virtual_list_notify_data_changed(self);
}

void egui_view_virtual_section_list_notify_section_removed(egui_view_t *self, uint32_t section_index, uint32_t count)
{
    EGUI_UNUSED(section_index);
    EGUI_UNUSED(count);
    egui_view_virtual_list_notify_data_changed(self);
}

void egui_view_virtual_section_list_notify_section_moved(egui_view_t *self, uint32_t from_index, uint32_t to_index)
{
    EGUI_UNUSED(from_index);
    EGUI_UNUSED(to_index);
    egui_view_virtual_list_notify_data_changed(self);
}

void egui_view_virtual_section_list_notify_item_inserted(egui_view_t *self, uint32_t section_index, uint32_t item_index, uint32_t count)
{
    EGUI_UNUSED(section_index);
    EGUI_UNUSED(item_index);
    EGUI_UNUSED(count);
    egui_view_virtual_list_notify_data_changed(self);
}

void egui_view_virtual_section_list_notify_item_removed(egui_view_t *self, uint32_t section_index, uint32_t item_index, uint32_t count)
{
    EGUI_UNUSED(section_index);
    EGUI_UNUSED(item_index);
    EGUI_UNUSED(count);
    egui_view_virtual_list_notify_data_changed(self);
}

void egui_view_virtual_section_list_notify_item_moved(egui_view_t *self, uint32_t from_section_index, uint32_t from_item_index, uint32_t to_section_index,
                                                      uint32_t to_item_index)
{
    EGUI_UNUSED(from_section_index);
    EGUI_UNUSED(from_item_index);
    EGUI_UNUSED(to_section_index);
    EGUI_UNUSED(to_item_index);
    egui_view_virtual_list_notify_data_changed(self);
}

uint8_t egui_view_virtual_section_list_get_slot_count(egui_view_t *self)
{
    return egui_view_virtual_list_get_slot_count(self);
}

const egui_view_virtual_section_list_slot_t *egui_view_virtual_section_list_get_slot(egui_view_t *self, uint8_t slot_index)
{
    return egui_view_virtual_list_get_slot(self, slot_index);
}

int32_t egui_view_virtual_section_list_find_slot_index_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    return egui_view_virtual_section_list_find_slot_index_by_stable_id_internal(local, stable_id);
}

const egui_view_virtual_section_list_slot_t *egui_view_virtual_section_list_find_slot_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t slot_index = egui_view_virtual_section_list_find_slot_index_by_stable_id(self, stable_id);

    return slot_index >= 0 ? egui_view_virtual_section_list_get_slot(self, (uint8_t)slot_index) : NULL;
}

egui_view_t *egui_view_virtual_section_list_find_view_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    const egui_view_virtual_section_list_slot_t *slot = egui_view_virtual_section_list_find_slot_by_stable_id(self, stable_id);

    return slot != NULL ? slot->view : NULL;
}

uint8_t egui_view_virtual_section_list_get_slot_entry(egui_view_t *self, uint8_t slot_index, egui_view_virtual_section_list_entry_t *entry)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);
    const egui_view_virtual_section_list_slot_t *slot = egui_view_virtual_section_list_get_slot(self, slot_index);

    return egui_view_virtual_section_list_resolve_slot_internal(local, slot, entry);
}

uint8_t egui_view_virtual_section_list_visit_visible_entries(egui_view_t *self, egui_view_virtual_section_list_visible_entry_visitor_t visitor, void *context)
{
    uint8_t slot_count;
    uint8_t slot_index;
    uint8_t visited = 0;

    if (visitor == NULL)
    {
        return 0;
    }

    slot_count = egui_view_virtual_section_list_get_slot_count(self);
    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_section_list_entry_t entry;
        const egui_view_virtual_section_list_slot_t *slot = egui_view_virtual_section_list_get_slot(self, slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL)
        {
            continue;
        }
        if (!egui_view_virtual_section_list_get_slot_entry(self, slot_index, &entry))
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

typedef struct egui_view_virtual_section_list_find_visible_entry_context
{
    egui_view_virtual_section_list_visible_entry_matcher_t matcher;
    void *context;
    uint32_t best_flat_index;
    egui_view_t *best_view;
    egui_view_virtual_section_list_entry_t best_entry;
} egui_view_virtual_section_list_find_visible_entry_context_t;

static uint8_t egui_view_virtual_section_list_find_visible_entry_visitor(egui_view_t *self, const egui_view_virtual_section_list_slot_t *slot,
                                                                         const egui_view_virtual_section_list_entry_t *entry, egui_view_t *entry_view,
                                                                         void *context)
{
    egui_view_virtual_section_list_find_visible_entry_context_t *ctx = (egui_view_virtual_section_list_find_visible_entry_context_t *)context;

    if (slot == NULL || entry == NULL || entry_view == NULL)
    {
        return 1;
    }
    if (ctx->matcher != NULL && !ctx->matcher(self, slot, entry, entry_view, ctx->context))
    {
        return 1;
    }
    if (ctx->best_view == NULL || slot->index < ctx->best_flat_index)
    {
        ctx->best_flat_index = slot->index;
        ctx->best_entry = *entry;
        ctx->best_view = entry_view;
    }

    return 1;
}

egui_view_t *egui_view_virtual_section_list_find_first_visible_entry_view(egui_view_t *self, egui_view_virtual_section_list_visible_entry_matcher_t matcher,
                                                                          void *context, egui_view_virtual_section_list_entry_t *entry_out)
{
    egui_view_virtual_section_list_find_visible_entry_context_t find_ctx = {
            .matcher = matcher,
            .context = context,
            .best_flat_index = EGUI_VIEW_VIRTUAL_SECTION_LIST_INVALID_INDEX,
            .best_view = NULL,
    };

    egui_view_virtual_section_list_visit_visible_entries(self, egui_view_virtual_section_list_find_visible_entry_visitor, &find_ctx);
    if (find_ctx.best_view != NULL && entry_out != NULL)
    {
        *entry_out = find_ctx.best_entry;
    }

    return find_ctx.best_view;
}

void egui_view_virtual_section_list_init(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_section_list_t);

    egui_view_virtual_list_init(self);
    local->data_source = NULL;
    local->data_source_context = NULL;
    memset(&local->flat_data_source, 0, sizeof(local->flat_data_source));
    egui_view_set_view_name(self, "egui_view_virtual_section_list");
}
