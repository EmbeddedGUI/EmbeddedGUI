#include <string.h>

#include "egui_view_virtual_viewport.h"
#include "core/egui_common.h"
#include "core/egui_input.h"

typedef struct egui_view_virtual_viewport_item_layout egui_view_virtual_viewport_item_layout_t;
typedef struct egui_view_virtual_viewport_state_entry egui_view_virtual_viewport_state_entry_t;
struct egui_view_virtual_viewport_item_layout
{
    uint16_t view_type;
    uint32_t index;
    uint32_t stable_id;
    int32_t logical_main_origin;
    int32_t logical_main_size;
};

struct egui_view_virtual_viewport_state_entry
{
    egui_dnode_t node;
    uint32_t stable_id;
    uint16_t size;
    uint8_t data[];
};

extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_virtual_viewport_t);

static void egui_view_virtual_viewport_reset_slot(egui_view_virtual_viewport_slot_t *slot)
{
    slot->state = EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED;
    slot->view_type = 0;
    slot->index = 0;
    slot->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    slot->view = NULL;
    memset(&slot->render_region, 0, sizeof(slot->render_region));
    slot->logical_main_origin = 0;
    slot->logical_main_size = 0;
    slot->last_used_seq = 0;
}

static void egui_view_virtual_viewport_reset_slots(egui_view_virtual_viewport_t *local)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        egui_view_virtual_viewport_reset_slot(&local->slots[i]);
    }

    local->slot_count = 0;
}

static egui_view_t *egui_view_virtual_viewport_find_host_view(egui_view_t *view)
{
    while (view != NULL)
    {
        if (view->api == &EGUI_VIEW_API_TABLE_NAME(egui_view_virtual_viewport_t))
        {
            return view;
        }

        view = EGUI_VIEW_PARENT(view);
    }

    return NULL;
}

static uint32_t egui_view_virtual_viewport_default_stable_id(uint32_t index)
{
    return index;
}

static void egui_view_virtual_viewport_reset_entry(egui_view_virtual_viewport_entry_t *entry)
{
    if (entry == NULL)
    {
        return;
    }

    entry->index = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    entry->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    entry->view_type = 0;
}

static egui_view_virtual_viewport_state_entry_t *egui_view_virtual_viewport_find_state_entry(egui_view_virtual_viewport_t *local, uint32_t stable_id)
{
    egui_dnode_t *node;

    if (stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        return NULL;
    }

    EGUI_DLIST_FOR_EACH_NODE(&local->state_entries, node)
    {
        egui_view_virtual_viewport_state_entry_t *entry = EGUI_DLIST_ENTRY(node, egui_view_virtual_viewport_state_entry_t, node);

        if (entry->stable_id == stable_id)
        {
            return entry;
        }
    }

    return NULL;
}

static void egui_view_virtual_viewport_touch_state_entry(egui_view_virtual_viewport_t *local, egui_view_virtual_viewport_state_entry_t *entry)
{
    if (entry == NULL || egui_dlist_peek_tail(&local->state_entries) == &entry->node)
    {
        return;
    }

    egui_dlist_remove(&entry->node);
    egui_dlist_append(&local->state_entries, &entry->node);
}

static void egui_view_virtual_viewport_remove_state_entry(egui_view_virtual_viewport_t *local, egui_view_virtual_viewport_state_entry_t *entry)
{
    if (entry == NULL)
    {
        return;
    }

    egui_dlist_remove(&entry->node);
    if (local->state_entry_count > 0)
    {
        local->state_entry_count--;
    }
    if (local->state_total_bytes >= entry->size)
    {
        local->state_total_bytes -= entry->size;
    }
    else
    {
        local->state_total_bytes = 0;
    }
    egui_free(entry);
}

static void egui_view_virtual_viewport_trim_state_cache(egui_view_virtual_viewport_t *local)
{
    while (!egui_dlist_is_empty(&local->state_entries))
    {
        egui_view_virtual_viewport_state_entry_t *entry;
        uint8_t over_entry_limit;
        uint8_t over_byte_limit;

        over_entry_limit = local->max_state_entries > 0 && local->state_entry_count > local->max_state_entries;
        over_byte_limit = local->max_state_bytes > 0 && local->state_total_bytes > local->max_state_bytes;
        if (!over_entry_limit && !over_byte_limit)
        {
            break;
        }

        entry = EGUI_DLIST_ENTRY(egui_dlist_peek_head(&local->state_entries), egui_view_virtual_viewport_state_entry_t, node);
        egui_view_virtual_viewport_remove_state_entry(local, entry);
    }
}

static void egui_view_virtual_viewport_clear_state_cache_internal(egui_view_virtual_viewport_t *local)
{
    while (!egui_dlist_is_empty(&local->state_entries))
    {
        egui_view_virtual_viewport_state_entry_t *entry =
                EGUI_DLIST_ENTRY(egui_dlist_peek_head(&local->state_entries), egui_view_virtual_viewport_state_entry_t, node);
        egui_view_virtual_viewport_remove_state_entry(local, entry);
    }

    local->state_entry_count = 0;
    local->state_total_bytes = 0;
}

static void egui_view_virtual_viewport_hide_slot_view(egui_view_t *self, egui_view_virtual_viewport_t *local, egui_view_virtual_viewport_slot_t *slot)
{
    egui_dim_t hidden_x = 0;
    egui_dim_t hidden_y = 0;

    if (slot->view == NULL)
    {
        return;
    }

    if (local->orientation == EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_HORIZONTAL)
    {
        hidden_x = (egui_dim_t)(-(self->region.size.width + slot->view->region.size.width + 4));
    }
    else
    {
        hidden_y = (egui_dim_t)(-(self->region.size.height + slot->view->region.size.height + 4));
    }

    egui_view_set_gone(slot->view, 1);
    egui_view_set_position(slot->view, hidden_x, hidden_y);
}

static uint32_t egui_view_virtual_viewport_get_item_count(egui_view_virtual_viewport_t *local)
{
    if (local->adapter == NULL || local->adapter->get_count == NULL)
    {
        return 0;
    }

    return local->adapter->get_count(local->adapter_context);
}

static uint32_t egui_view_virtual_viewport_get_item_stable_id(egui_view_virtual_viewport_t *local, uint32_t index)
{
    if (local->adapter != NULL && local->adapter->get_stable_id != NULL)
    {
        return local->adapter->get_stable_id(local->adapter_context, index);
    }

    return index;
}

static uint16_t egui_view_virtual_viewport_get_item_view_type(egui_view_virtual_viewport_t *local, uint32_t index)
{
    if (local->adapter != NULL && local->adapter->get_view_type != NULL)
    {
        return local->adapter->get_view_type(local->adapter_context, index);
    }

    return 0;
}

static uint8_t egui_view_virtual_viewport_resolve_item_by_index_internal(egui_view_virtual_viewport_t *local, uint32_t index,
                                                                         egui_view_virtual_viewport_entry_t *entry)
{
    uint32_t count;

    egui_view_virtual_viewport_reset_entry(entry);

    if (local->adapter == NULL || local->adapter->get_count == NULL)
    {
        return 0;
    }

    count = local->adapter->get_count(local->adapter_context);
    if (index >= count)
    {
        return 0;
    }

    if (entry != NULL)
    {
        entry->index = index;
        entry->stable_id = local->adapter->get_stable_id != NULL ? local->adapter->get_stable_id(local->adapter_context, index)
                                                                 : egui_view_virtual_viewport_default_stable_id(index);
        entry->view_type = egui_view_virtual_viewport_get_item_view_type(local, index);
    }

    return 1;
}

static const egui_view_virtual_viewport_slot_t *egui_view_virtual_viewport_find_slot_by_view(egui_view_virtual_viewport_t *local, egui_view_t *view)
{
    egui_view_t *cursor = view;

    while (cursor != NULL)
    {
        uint8_t slot_index;

        for (slot_index = 0; slot_index < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; slot_index++)
        {
            const egui_view_virtual_viewport_slot_t *slot = &local->slots[slot_index];

            if (slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED && slot->view == cursor)
            {
                return slot;
            }
        }

        if (cursor == EGUI_VIEW_OF(local) || cursor == EGUI_VIEW_OF(&local->content_layer))
        {
            break;
        }

        cursor = EGUI_VIEW_PARENT(cursor);
    }

    return NULL;
}

static int32_t egui_view_virtual_viewport_find_slot_index_by_stable_id_internal(egui_view_virtual_viewport_t *local, uint32_t stable_id)
{
    uint8_t slot_index;

    for (slot_index = 0; slot_index < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; slot_index++)
    {
        const egui_view_virtual_viewport_slot_t *slot = &local->slots[slot_index];

        if (slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED && slot->stable_id == stable_id)
        {
            return (int32_t)slot_index;
        }
    }

    return -1;
}

static uint8_t egui_view_virtual_viewport_resolve_slot_internal(egui_view_virtual_viewport_t *local, const egui_view_virtual_viewport_slot_t *slot,
                                                                egui_view_virtual_viewport_entry_t *entry)
{
    uint32_t index;
    uint32_t count;

    if (slot == NULL || slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED)
    {
        egui_view_virtual_viewport_reset_entry(entry);
        return 0;
    }

    if (egui_view_virtual_viewport_resolve_item_by_index_internal(local, slot->index, entry) && entry->stable_id == slot->stable_id)
    {
        return 1;
    }

    count = egui_view_virtual_viewport_get_item_count(local);
    for (index = 0; index < count; index++)
    {
        if (egui_view_virtual_viewport_get_item_stable_id(local, index) == slot->stable_id)
        {
            return egui_view_virtual_viewport_resolve_item_by_index_internal(local, index, entry);
        }
    }

    egui_view_virtual_viewport_reset_entry(entry);
    return 0;
}

static int32_t egui_view_virtual_viewport_get_cross_extent(egui_view_t *self, egui_view_virtual_viewport_t *local)
{
    if (local->orientation == EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_HORIZONTAL)
    {
        return self->region.size.height;
    }

    return self->region.size.width;
}

static int32_t egui_view_virtual_viewport_get_main_extent(egui_view_t *self, egui_view_virtual_viewport_t *local)
{
    if (local->orientation == EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_HORIZONTAL)
    {
        return self->region.size.width;
    }

    return self->region.size.height;
}

static int32_t egui_view_virtual_viewport_get_slot_main_origin(const egui_view_virtual_viewport_t *local, const egui_view_virtual_viewport_slot_t *slot)
{
    if (local->orientation == EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_HORIZONTAL)
    {
        return slot->render_region.location.x;
    }

    return slot->render_region.location.y;
}

static int32_t egui_view_virtual_viewport_get_slot_main_size(const egui_view_virtual_viewport_t *local, const egui_view_virtual_viewport_slot_t *slot)
{
    if (local->orientation == EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_HORIZONTAL)
    {
        return slot->render_region.size.width;
    }

    return slot->render_region.size.height;
}

static int32_t egui_view_virtual_viewport_measure_item_main_size(egui_view_t *self, egui_view_virtual_viewport_t *local, uint32_t index)
{
    int32_t extent = local->estimated_item_extent;

    if (local->adapter != NULL && local->adapter->measure_main_size != NULL)
    {
        int32_t measured = local->adapter->measure_main_size(local->adapter_context, index, egui_view_virtual_viewport_get_cross_extent(self, local));
        if (measured > 0)
        {
            extent = measured;
        }
    }

    if (extent <= 0)
    {
        extent = 1;
    }

    return extent;
}

static uint16_t egui_view_virtual_viewport_get_index_block_count(uint32_t item_count)
{
    return (uint16_t)((item_count + EGUI_VIEW_VIRTUAL_VIEWPORT_INDEX_BLOCK_ITEMS - 1) / EGUI_VIEW_VIRTUAL_VIEWPORT_INDEX_BLOCK_ITEMS);
}

static void egui_view_virtual_viewport_release_index_storage(egui_view_virtual_viewport_t *local)
{
    if (local->index_block_extents != NULL)
    {
        egui_free(local->index_block_extents);
        local->index_block_extents = NULL;
    }
    if (local->index_block_origins != NULL)
    {
        egui_free(local->index_block_origins);
        local->index_block_origins = NULL;
    }
    if (local->index_block_valid != NULL)
    {
        egui_free(local->index_block_valid);
        local->index_block_valid = NULL;
    }

    local->index_block_count = 0;
    local->index_block_capacity = 0;
    local->indexed_item_count = 0;
    local->indexed_cross_extent = 0;
    local->are_index_prefixes_dirty = 1;
}

static void egui_view_virtual_viewport_invalidate_all_index_blocks(egui_view_virtual_viewport_t *local)
{
    if (local->index_block_valid != NULL && local->index_block_capacity > 0)
    {
        memset(local->index_block_valid, 0, local->index_block_capacity);
    }

    local->indexed_item_count = 0;
    local->indexed_cross_extent = 0;
    local->are_index_prefixes_dirty = 1;
}

static void egui_view_virtual_viewport_invalidate_index_block(egui_view_virtual_viewport_t *local, uint32_t index)
{
    uint16_t block_index;

    if (local->index_block_valid == NULL || local->index_block_count == 0)
    {
        local->are_index_prefixes_dirty = 1;
        return;
    }

    block_index = (uint16_t)(index / EGUI_VIEW_VIRTUAL_VIEWPORT_INDEX_BLOCK_ITEMS);
    if (block_index < local->index_block_count)
    {
        local->index_block_valid[block_index] = 0;
    }

    local->are_index_prefixes_dirty = 1;
}

static uint8_t egui_view_virtual_viewport_ensure_index_storage(egui_view_virtual_viewport_t *local, uint32_t item_count)
{
    uint16_t block_count = egui_view_virtual_viewport_get_index_block_count(item_count);

    if (block_count == 0)
    {
        local->index_block_count = 0;
        local->indexed_item_count = item_count;
        local->are_index_prefixes_dirty = 0;
        return 1;
    }

    if (block_count <= local->index_block_capacity)
    {
        if (local->index_block_valid != NULL && block_count > local->index_block_count)
        {
            memset(&local->index_block_valid[local->index_block_count], 0, block_count - local->index_block_count);
        }

        local->index_block_count = block_count;
        return 1;
    }
    else
    {
        int32_t *new_block_extents = (int32_t *)egui_malloc((int)(sizeof(int32_t) * block_count));
        int32_t *new_block_origins;
        uint8_t *new_block_valid;

        if (new_block_extents == NULL)
        {
            return 0;
        }

        new_block_origins = (int32_t *)egui_malloc((int)(sizeof(int32_t) * block_count));
        if (new_block_origins == NULL)
        {
            egui_free(new_block_extents);
            return 0;
        }

        new_block_valid = (uint8_t *)egui_malloc((int)block_count);
        if (new_block_valid == NULL)
        {
            egui_free(new_block_extents);
            egui_free(new_block_origins);
            return 0;
        }

        memset(new_block_valid, 0, block_count);
        egui_view_virtual_viewport_release_index_storage(local);

        local->index_block_extents = new_block_extents;
        local->index_block_origins = new_block_origins;
        local->index_block_valid = new_block_valid;
        local->index_block_capacity = block_count;
        local->index_block_count = block_count;
        local->are_index_prefixes_dirty = 1;
        return 1;
    }
}

static int32_t egui_view_virtual_viewport_measure_block_extent(egui_view_t *self, egui_view_virtual_viewport_t *local, uint16_t block_index)
{
    uint32_t start_index = (uint32_t)block_index * EGUI_VIEW_VIRTUAL_VIEWPORT_INDEX_BLOCK_ITEMS;
    uint32_t end_index = start_index + EGUI_VIEW_VIRTUAL_VIEWPORT_INDEX_BLOCK_ITEMS;
    uint32_t item_count = egui_view_virtual_viewport_get_item_count(local);
    uint32_t index;
    int32_t extent = 0;

    if (end_index > item_count)
    {
        end_index = item_count;
    }

    for (index = start_index; index < end_index; index++)
    {
        extent += egui_view_virtual_viewport_measure_item_main_size(self, local, index);
    }

    local->index_block_extents[block_index] = extent;
    local->index_block_valid[block_index] = 1;
    return extent;
}

static uint8_t egui_view_virtual_viewport_sync_index_prefixes(egui_view_t *self, egui_view_virtual_viewport_t *local)
{
    uint32_t item_count = egui_view_virtual_viewport_get_item_count(local);
    int32_t cross_extent = egui_view_virtual_viewport_get_cross_extent(self, local);
    int32_t total_extent = 0;
    uint16_t block_index;

    if (local->adapter == NULL || local->adapter->get_count == NULL)
    {
        local->logical_extent = 0;
        local->index_block_count = 0;
        local->indexed_item_count = 0;
        local->indexed_cross_extent = 0;
        local->are_index_prefixes_dirty = 0;
        return 1;
    }

    if (local->adapter->measure_main_size == NULL)
    {
        local->logical_extent = (int32_t)item_count * local->estimated_item_extent;
        local->index_block_count = 0;
        local->indexed_item_count = item_count;
        local->indexed_cross_extent = cross_extent;
        local->are_index_prefixes_dirty = 0;
        return 1;
    }

    if (local->indexed_item_count != item_count || local->indexed_cross_extent != cross_extent)
    {
        egui_view_virtual_viewport_invalidate_all_index_blocks(local);
    }

    if (!egui_view_virtual_viewport_ensure_index_storage(local, item_count))
    {
        uint32_t index;

        total_extent = 0;
        for (index = 0; index < item_count; index++)
        {
            total_extent += egui_view_virtual_viewport_measure_item_main_size(self, local, index);
        }

        local->logical_extent = total_extent;
        return 0;
    }

    if (!local->are_index_prefixes_dirty && local->indexed_item_count == item_count && local->indexed_cross_extent == cross_extent)
    {
        return 1;
    }

    total_extent = 0;
    for (block_index = 0; block_index < local->index_block_count; block_index++)
    {
        int32_t block_extent = local->index_block_valid[block_index] ? local->index_block_extents[block_index]
                                                                     : egui_view_virtual_viewport_measure_block_extent(self, local, block_index);

        local->index_block_origins[block_index] = total_extent;
        total_extent += block_extent;
    }

    local->logical_extent = total_extent;
    local->indexed_item_count = item_count;
    local->indexed_cross_extent = cross_extent;
    local->are_index_prefixes_dirty = 0;
    return 1;
}

static int32_t egui_view_virtual_viewport_get_item_main_origin_linear(egui_view_t *self, egui_view_virtual_viewport_t *local, uint32_t index)
{
    uint32_t item_count = egui_view_virtual_viewport_get_item_count(local);
    uint32_t i;
    int32_t origin = 0;

    if (index >= item_count)
    {
        return -1;
    }

    for (i = 0; i < index; i++)
    {
        origin += egui_view_virtual_viewport_measure_item_main_size(self, local, i);
    }

    return origin;
}

static int32_t egui_view_virtual_viewport_get_item_main_origin_internal(egui_view_t *self, egui_view_virtual_viewport_t *local, uint32_t index)
{
    uint32_t item_count = egui_view_virtual_viewport_get_item_count(local);
    uint32_t block_start_index;
    uint32_t i;
    int32_t origin;
    uint16_t block_index;

    if (index >= item_count)
    {
        return -1;
    }

    if (local->adapter == NULL || local->adapter->measure_main_size == NULL)
    {
        return (int32_t)index * local->estimated_item_extent;
    }

    if (!egui_view_virtual_viewport_sync_index_prefixes(self, local))
    {
        return egui_view_virtual_viewport_get_item_main_origin_linear(self, local, index);
    }

    block_index = (uint16_t)(index / EGUI_VIEW_VIRTUAL_VIEWPORT_INDEX_BLOCK_ITEMS);
    block_start_index = (uint32_t)block_index * EGUI_VIEW_VIRTUAL_VIEWPORT_INDEX_BLOCK_ITEMS;
    origin = local->index_block_origins[block_index];

    for (i = block_start_index; i < index; i++)
    {
        origin += egui_view_virtual_viewport_measure_item_main_size(self, local, i);
    }

    return origin;
}

static int32_t egui_view_virtual_viewport_get_item_main_size_internal(egui_view_t *self, egui_view_virtual_viewport_t *local, uint32_t index)
{
    if (index >= egui_view_virtual_viewport_get_item_count(local))
    {
        return -1;
    }

    return egui_view_virtual_viewport_measure_item_main_size(self, local, index);
}

static uint8_t egui_view_virtual_viewport_find_item_index_at_offset_linear(egui_view_t *self, egui_view_virtual_viewport_t *local, int32_t offset,
                                                                           uint32_t *index_out, int32_t *origin_out)
{
    uint32_t item_count = egui_view_virtual_viewport_get_item_count(local);
    uint32_t index;
    int32_t origin = 0;

    if (item_count == 0)
    {
        return 0;
    }

    if (offset <= 0)
    {
        *index_out = 0;
        *origin_out = 0;
        return 1;
    }

    for (index = 0; index < item_count; index++)
    {
        int32_t item_extent = egui_view_virtual_viewport_measure_item_main_size(self, local, index);

        if (origin + item_extent > offset)
        {
            *index_out = index;
            *origin_out = origin;
            return 1;
        }

        origin += item_extent;
    }

    *index_out = item_count - 1;
    *origin_out = origin - egui_view_virtual_viewport_measure_item_main_size(self, local, item_count - 1);
    return 1;
}

static uint8_t egui_view_virtual_viewport_find_item_index_at_offset(egui_view_t *self, egui_view_virtual_viewport_t *local, int32_t offset, uint32_t *index_out,
                                                                    int32_t *origin_out)
{
    uint32_t item_count = egui_view_virtual_viewport_get_item_count(local);

    if (item_count == 0)
    {
        return 0;
    }

    if (local->adapter == NULL || local->adapter->measure_main_size == NULL)
    {
        int32_t item_extent = local->estimated_item_extent;
        uint32_t index;

        if (item_extent <= 0)
        {
            item_extent = 1;
        }

        if (offset <= 0)
        {
            *index_out = 0;
            *origin_out = 0;
            return 1;
        }

        index = (uint32_t)(offset / item_extent);
        if (index >= item_count)
        {
            index = item_count - 1;
        }

        *index_out = index;
        *origin_out = (int32_t)index * item_extent;
        return 1;
    }

    if (!egui_view_virtual_viewport_sync_index_prefixes(self, local))
    {
        return egui_view_virtual_viewport_find_item_index_at_offset_linear(self, local, offset, index_out, origin_out);
    }

    if (offset <= 0)
    {
        *index_out = 0;
        *origin_out = 0;
        return 1;
    }

    if (offset >= local->logical_extent)
    {
        uint32_t last_index = item_count - 1;

        *index_out = last_index;
        *origin_out = egui_view_virtual_viewport_get_item_main_origin_internal(self, local, last_index);
        return 1;
    }
    else
    {
        uint16_t low = 0;
        uint16_t high = local->index_block_count;
        uint16_t block_index;
        uint32_t index;
        uint32_t end_index;
        int32_t origin;

        while ((uint16_t)(low + 1) < high)
        {
            uint16_t mid = (uint16_t)((low + high) / 2);

            if (local->index_block_origins[mid] <= offset)
            {
                low = mid;
            }
            else
            {
                high = mid;
            }
        }

        block_index = low;
        origin = local->index_block_origins[block_index];
        index = (uint32_t)block_index * EGUI_VIEW_VIRTUAL_VIEWPORT_INDEX_BLOCK_ITEMS;
        end_index = index + EGUI_VIEW_VIRTUAL_VIEWPORT_INDEX_BLOCK_ITEMS;
        if (end_index > item_count)
        {
            end_index = item_count;
        }

        for (; index < end_index; index++)
        {
            int32_t item_extent = egui_view_virtual_viewport_measure_item_main_size(self, local, index);

            if (origin + item_extent > offset)
            {
                *index_out = index;
                *origin_out = origin;
                return 1;
            }

            origin += item_extent;
        }

        *index_out = item_count - 1;
        *origin_out = egui_view_virtual_viewport_get_item_main_origin_internal(self, local, item_count - 1);
        return 1;
    }
}

static void egui_view_virtual_viewport_mark_dirty(egui_view_t *self, egui_view_virtual_viewport_t *local, uint8_t data_dirty, uint8_t layout_dirty)
{
    if (data_dirty)
    {
        local->is_data_dirty = 1;
    }
    if (layout_dirty)
    {
        local->is_layout_dirty = 1;
    }
    egui_view_invalidate(self);
}

static void egui_view_virtual_viewport_sync_metrics(egui_view_t *self, egui_view_virtual_viewport_t *local)
{
    if (local->orientation == EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_HORIZONTAL)
    {
        local->viewport_extent = self->region.size.width;
        local->cross_extent = self->region.size.height;
    }
    else
    {
        local->viewport_extent = self->region.size.height;
        local->cross_extent = self->region.size.width;
    }
}

static void egui_view_virtual_viewport_sync_content_layer(egui_view_t *self, egui_view_virtual_viewport_t *local)
{
    egui_view_set_position(EGUI_VIEW_OF(&local->content_layer), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&local->content_layer), self->region.size.width, self->region.size.height);
}

static int32_t egui_view_virtual_viewport_get_max_offset(egui_view_virtual_viewport_t *local)
{
    int32_t max_offset = local->logical_extent - local->viewport_extent;

    if (max_offset < 0)
    {
        max_offset = 0;
    }

    return max_offset;
}

static void egui_view_virtual_viewport_clamp_offset(egui_view_virtual_viewport_t *local)
{
    int32_t max_offset = egui_view_virtual_viewport_get_max_offset(local);

    if (local->logical_offset < 0)
    {
        local->logical_offset = 0;
    }
    if (local->logical_offset > max_offset)
    {
        local->logical_offset = max_offset;
    }
}

static void egui_view_virtual_viewport_save_and_unbind(egui_view_virtual_viewport_t *local, egui_view_virtual_viewport_slot_t *slot)
{
    if (slot->view != NULL && slot->stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID && local->adapter != NULL)
    {
        if (local->adapter->save_state != NULL)
        {
            local->adapter->save_state(local->adapter_context, slot->view, slot->stable_id);
        }
        if (local->adapter->unbind_view != NULL)
        {
            local->adapter->unbind_view(local->adapter_context, slot->view, slot->stable_id);
        }
    }

    slot->index = 0;
    slot->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    memset(&slot->render_region, 0, sizeof(slot->render_region));
    slot->logical_main_origin = 0;
    slot->logical_main_size = 0;
}

static uint8_t egui_view_virtual_viewport_should_keep_alive(egui_view_virtual_viewport_t *local, egui_view_virtual_viewport_slot_t *slot)
{
    if (slot->view == NULL || slot->stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID || local->adapter == NULL || local->adapter->should_keep_alive == NULL)
    {
        return 0;
    }

    return local->adapter->should_keep_alive(local->adapter_context, slot->view, slot->stable_id) ? 1 : 0;
}

static void egui_view_virtual_viewport_recycle_slot(egui_view_t *self, egui_view_virtual_viewport_t *local, egui_view_virtual_viewport_slot_t *slot)
{
    if (slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED)
    {
        return;
    }

    if (slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_KEEPALIVE)
    {
        egui_view_virtual_viewport_save_and_unbind(local, slot);
    }

    if (slot->view != NULL)
    {
        slot->state = EGUI_VIEW_VIRTUAL_SLOT_STATE_POOLED;
        slot->last_used_seq = 0;
        egui_view_virtual_viewport_hide_slot_view(self, local, slot);
    }
    else
    {
        egui_view_virtual_viewport_reset_slot(slot);
    }
}

static void egui_view_virtual_viewport_destroy_slot_view(egui_view_t *self, egui_view_virtual_viewport_t *local, egui_view_virtual_viewport_slot_t *slot)
{
    if (slot->view == NULL)
    {
        egui_view_virtual_viewport_reset_slot(slot);
        return;
    }

    if ((slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_KEEPALIVE) &&
        slot->stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        egui_view_virtual_viewport_save_and_unbind(local, slot);
    }

    if (local->is_attached_to_window)
    {
        slot->view->api->on_detach_from_window(slot->view);
    }

    if (slot->view->parent == &local->content_layer)
    {
        egui_view_group_remove_child(EGUI_VIEW_OF(&local->content_layer), slot->view);
    }
    egui_view_set_parent(slot->view, NULL);

    if (local->adapter != NULL && local->adapter->destroy_view != NULL)
    {
        local->adapter->destroy_view(local->adapter_context, slot->view, slot->view_type);
    }

    egui_view_virtual_viewport_reset_slot(slot);
    EGUI_UNUSED(self);
}

static void egui_view_virtual_viewport_destroy_all_slots(egui_view_t *self, egui_view_virtual_viewport_t *local)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        egui_view_virtual_viewport_destroy_slot_view(self, local, &local->slots[i]);
    }

    local->slot_count = 0;
}

static void egui_view_virtual_viewport_apply_render_region(egui_view_t *self, egui_view_virtual_viewport_t *local, egui_view_virtual_viewport_slot_t *slot,
                                                           int32_t logical_main_origin, int32_t logical_main_size)
{
    egui_region_t region;
    egui_dim_t x = 0;
    egui_dim_t y = 0;
    egui_dim_t width = self->region.size.width;
    egui_dim_t height = self->region.size.height;
    int32_t render_main_origin = logical_main_origin - local->logical_offset;

    if (local->orientation == EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_HORIZONTAL)
    {
        x = (egui_dim_t)render_main_origin;
        width = (egui_dim_t)logical_main_size;
    }
    else
    {
        y = (egui_dim_t)render_main_origin;
        height = (egui_dim_t)logical_main_size;
    }

    slot->logical_main_origin = logical_main_origin;
    slot->logical_main_size = logical_main_size;
    slot->render_region.location.x = x;
    slot->render_region.location.y = y;
    slot->render_region.size.width = width;
    slot->render_region.size.height = height;

    region.location.x = x;
    region.location.y = y;
    region.size.width = width;
    region.size.height = height;

    /* Use layout so both the old and new screen regions are marked dirty. */
    egui_view_layout(slot->view, &region);
    egui_view_set_gone(slot->view, 0);
}

static uint8_t egui_view_virtual_viewport_prepare_slot_view(egui_view_t *self, egui_view_virtual_viewport_t *local, egui_view_virtual_viewport_slot_t *slot,
                                                            uint16_t view_type)
{
    egui_view_t *view;

    if (slot->view != NULL && slot->view_type == view_type)
    {
        return 1;
    }

    if (slot->view != NULL)
    {
        if (local->adapter == NULL || local->adapter->destroy_view == NULL)
        {
            return 0;
        }
        egui_view_virtual_viewport_destroy_slot_view(self, local, slot);
    }

    if (local->adapter == NULL || local->adapter->create_view == NULL)
    {
        return 0;
    }

    view = local->adapter->create_view(local->adapter_context, view_type);
    if (view == NULL)
    {
        return 0;
    }

    slot->view = view;
    slot->view_type = view_type;
    slot->state = EGUI_VIEW_VIRTUAL_SLOT_STATE_POOLED;

    egui_view_group_add_child(EGUI_VIEW_OF(&local->content_layer), view);
    if (local->is_attached_to_window)
    {
        view->api->on_attach_to_window(view);
    }

    egui_view_virtual_viewport_hide_slot_view(self, local, slot);
    return 1;
}

static int32_t egui_view_virtual_viewport_find_item_main_origin_by_id(egui_view_t *self, egui_view_virtual_viewport_t *local, uint32_t stable_id)
{
    int32_t found_index = egui_view_virtual_viewport_find_item_index_by_stable_id(self, stable_id);

    if (found_index < 0)
    {
        return -1;
    }

    return egui_view_virtual_viewport_get_item_main_origin_internal(self, local, (uint32_t)found_index);
}

static void egui_view_virtual_viewport_update_logical_extent(egui_view_t *self, egui_view_virtual_viewport_t *local)
{
    int32_t extent;

    if (!egui_view_virtual_viewport_sync_index_prefixes(self, local))
    {
        return;
    }

    extent = local->logical_extent;
    if (extent < 0)
    {
        local->logical_extent = 0;
    }
}

static uint8_t egui_view_virtual_viewport_build_visible_items(egui_view_t *self, egui_view_virtual_viewport_t *local,
                                                              egui_view_virtual_viewport_item_layout_t *items)
{
    uint32_t count = egui_view_virtual_viewport_get_item_count(local);
    uint32_t first_visible_index = 0;
    uint32_t first_index;
    uint32_t index;
    int32_t first_visible_origin = 0;
    int32_t start_main_origin;
    int32_t logical_main_origin;
    int32_t item_extent;
    uint8_t item_count = 0;
    uint8_t seen_visible = 0;
    uint8_t overscan_after_used = 0;
    uint8_t i;

    if (count == 0)
    {
        return 0;
    }

    if (!egui_view_virtual_viewport_find_item_index_at_offset(self, local, local->logical_offset, &first_visible_index, &first_visible_origin))
    {
        return 0;
    }

    first_index = first_visible_index;
    start_main_origin = first_visible_origin;
    for (i = 0; i < local->overscan_before && first_index > 0; i++)
    {
        item_extent = egui_view_virtual_viewport_get_item_main_size_internal(self, local, first_index - 1);
        if (item_extent <= 0)
        {
            break;
        }

        first_index--;
        start_main_origin -= item_extent;
    }

    logical_main_origin = start_main_origin;
    for (index = first_index; index < count && item_count < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; index++)
    {
        int32_t render_main_origin;

        item_extent = egui_view_virtual_viewport_get_item_main_size_internal(self, local, index);
        if (item_extent <= 0)
        {
            break;
        }

        render_main_origin = logical_main_origin - local->logical_offset;

        if (render_main_origin < local->viewport_extent && render_main_origin + item_extent > 0)
        {
            seen_visible = 1;
        }
        else if (seen_visible)
        {
            if (overscan_after_used >= local->overscan_after)
            {
                break;
            }
            overscan_after_used++;
        }

        items[item_count].index = index;
        items[item_count].stable_id = egui_view_virtual_viewport_get_item_stable_id(local, index);
        items[item_count].view_type = egui_view_virtual_viewport_get_item_view_type(local, index);
        items[item_count].logical_main_origin = logical_main_origin;
        items[item_count].logical_main_size = item_extent;
        item_count++;

        logical_main_origin += item_extent;
    }

    return item_count;
}

static int egui_view_virtual_viewport_find_slot_index_by_stable_id_reserved(egui_view_virtual_viewport_t *local, const uint8_t *reserved, uint32_t stable_id)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        if (reserved[i])
        {
            continue;
        }
        if ((local->slots[i].state == EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || local->slots[i].state == EGUI_VIEW_VIRTUAL_SLOT_STATE_KEEPALIVE) &&
            local->slots[i].stable_id == stable_id)
        {
            return (int)i;
        }
    }

    return -1;
}

static uint8_t egui_view_virtual_viewport_has_reserved_stable_id(egui_view_virtual_viewport_t *local, const uint8_t *reserved, uint32_t stable_id)
{
    uint8_t i;

    if (reserved == NULL || stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        return 0;
    }

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        if (!reserved[i])
        {
            continue;
        }
        if ((local->slots[i].state == EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || local->slots[i].state == EGUI_VIEW_VIRTUAL_SLOT_STATE_KEEPALIVE) &&
            local->slots[i].stable_id == stable_id)
        {
            return 1;
        }
    }

    return 0;
}

static int egui_view_virtual_viewport_find_slot_by_state_and_type(egui_view_virtual_viewport_t *local, const uint8_t *reserved, uint8_t state,
                                                                  uint16_t view_type)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        if (reserved[i])
        {
            continue;
        }
        if (local->slots[i].state == state && local->slots[i].view_type == view_type)
        {
            return (int)i;
        }
    }

    return -1;
}

static int egui_view_virtual_viewport_find_unused_slot(egui_view_virtual_viewport_t *local, const uint8_t *reserved)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        if (reserved[i])
        {
            continue;
        }
        if (local->slots[i].view == NULL)
        {
            if (local->slots[i].state != EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED)
            {
                egui_view_virtual_viewport_reset_slot(&local->slots[i]);
            }
            return (int)i;
        }
        if (local->slots[i].state == EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED)
        {
            return (int)i;
        }
    }

    return -1;
}

static int egui_view_virtual_viewport_find_retypeable_slot(egui_view_virtual_viewport_t *local, const uint8_t *reserved)
{
    uint8_t i;

    if (local->adapter == NULL || local->adapter->destroy_view == NULL)
    {
        return -1;
    }

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        if (reserved[i])
        {
            continue;
        }
        if (local->slots[i].state == EGUI_VIEW_VIRTUAL_SLOT_STATE_POOLED)
        {
            return (int)i;
        }
    }

    return -1;
}

static void egui_view_virtual_viewport_compact_slot_count(egui_view_virtual_viewport_t *local)
{
    uint8_t i;
    uint8_t count = 0;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        if (local->slots[i].state != EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED)
        {
            count++;
        }
    }

    local->slot_count = count;
}

static uint8_t egui_view_virtual_viewport_count_keepalive_slots(egui_view_virtual_viewport_t *local, const uint8_t *reserved)
{
    uint8_t i;
    uint8_t count = 0;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        if (reserved != NULL && reserved[i])
        {
            continue;
        }
        if (local->slots[i].state == EGUI_VIEW_VIRTUAL_SLOT_STATE_KEEPALIVE)
        {
            count++;
        }
    }

    return count;
}

static int egui_view_virtual_viewport_find_oldest_keepalive_slot(egui_view_virtual_viewport_t *local, const uint8_t *reserved)
{
    uint8_t i;
    int slot_index = -1;
    uint32_t oldest_seq = 0;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        egui_view_virtual_viewport_slot_t *slot = &local->slots[i];

        if ((reserved != NULL && reserved[i]) || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_KEEPALIVE)
        {
            continue;
        }

        if (slot_index < 0 || slot->last_used_seq < oldest_seq)
        {
            slot_index = (int)i;
            oldest_seq = slot->last_used_seq;
        }
    }

    return slot_index;
}

static void egui_view_virtual_viewport_trim_keepalive_slots(egui_view_t *self, egui_view_virtual_viewport_t *local, const uint8_t *reserved)
{
    uint8_t keepalive_count = egui_view_virtual_viewport_count_keepalive_slots(local, reserved);

    while (keepalive_count > local->max_keepalive_slots)
    {
        int slot_index = egui_view_virtual_viewport_find_oldest_keepalive_slot(local, reserved);

        if (slot_index < 0)
        {
            break;
        }

        egui_view_virtual_viewport_recycle_slot(self, local, &local->slots[slot_index]);
        keepalive_count--;
    }
}

static void egui_view_virtual_viewport_prepare_unreserved_slots(egui_view_t *self, egui_view_virtual_viewport_t *local, const uint8_t *reserved)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        egui_view_virtual_viewport_slot_t *slot = &local->slots[i];

        if (reserved[i] || slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED)
        {
            continue;
        }

        if (egui_view_virtual_viewport_has_reserved_stable_id(local, reserved, slot->stable_id))
        {
            egui_view_virtual_viewport_recycle_slot(self, local, slot);
            continue;
        }

        if ((slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_KEEPALIVE) &&
            egui_view_virtual_viewport_should_keep_alive(local, slot))
        {
            if (slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_KEEPALIVE)
            {
                slot->last_used_seq = ++local->slot_use_seq;
            }
            slot->state = EGUI_VIEW_VIRTUAL_SLOT_STATE_KEEPALIVE;
            egui_view_virtual_viewport_hide_slot_view(self, local, slot);
            continue;
        }

        egui_view_virtual_viewport_recycle_slot(self, local, slot);
    }

    egui_view_virtual_viewport_trim_keepalive_slots(self, local, reserved);
}

static uint8_t egui_view_virtual_viewport_bind_slot(egui_view_t *self, egui_view_virtual_viewport_t *local, egui_view_virtual_viewport_slot_t *slot,
                                                    const egui_view_virtual_viewport_item_layout_t *item)
{
    uint8_t needs_bind = 0;
    uint8_t needs_restore = 0;

    if (!egui_view_virtual_viewport_prepare_slot_view(self, local, slot, item->view_type))
    {
        return 0;
    }

    if (slot->stable_id != item->stable_id || slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_POOLED)
    {
        needs_bind = 1;
        needs_restore = 1;
    }
    else if (local->is_data_dirty || slot->index != item->index)
    {
        needs_bind = 1;
    }

    if (needs_bind && local->adapter != NULL && local->adapter->bind_view != NULL)
    {
        local->adapter->bind_view(local->adapter_context, slot->view, item->index, item->stable_id);
    }

    if (needs_restore && local->adapter != NULL && local->adapter->restore_state != NULL)
    {
        local->adapter->restore_state(local->adapter_context, slot->view, item->stable_id);
    }

    slot->state = EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE;
    slot->view_type = item->view_type;
    slot->index = item->index;
    slot->stable_id = item->stable_id;
    slot->last_used_seq = ++local->slot_use_seq;
    egui_view_virtual_viewport_apply_render_region(self, local, slot, item->logical_main_origin, item->logical_main_size);
    return 1;
}

static void egui_view_virtual_viewport_update_anchor(egui_view_virtual_viewport_t *local, const egui_view_virtual_viewport_item_layout_t *items,
                                                     uint8_t item_count)
{
    uint8_t i;

    local->anchor_stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    local->anchor_offset = 0;

    for (i = 0; i < item_count; i++)
    {
        int32_t item_end = items[i].logical_main_origin + items[i].logical_main_size;

        if (local->logical_offset >= items[i].logical_main_origin && local->logical_offset < item_end)
        {
            local->anchor_stable_id = items[i].stable_id;
            local->anchor_offset = local->logical_offset - items[i].logical_main_origin;
            return;
        }
    }

    if (item_count > 0)
    {
        local->anchor_stable_id = items[0].stable_id;
        local->anchor_offset = local->logical_offset - items[0].logical_main_origin;
    }
}

static void egui_view_virtual_viewport_sync_slots(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    egui_view_virtual_viewport_item_layout_t items[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS];
    uint8_t reserved[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS] = {0};
    uint8_t item_assigned[EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS] = {0};
    uint8_t item_count;
    uint8_t i;

    egui_view_virtual_viewport_update_logical_extent(self, local);

    if (local->is_data_dirty && local->anchor_stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        int32_t anchor_origin = egui_view_virtual_viewport_find_item_main_origin_by_id(self, local, local->anchor_stable_id);
        if (anchor_origin >= 0)
        {
            local->logical_offset = anchor_origin + local->anchor_offset;
        }
    }

    egui_view_virtual_viewport_clamp_offset(local);
    item_count = egui_view_virtual_viewport_build_visible_items(self, local, items);

    for (i = 0; i < item_count; i++)
    {
        int slot_index = egui_view_virtual_viewport_find_slot_index_by_stable_id_reserved(local, reserved, items[i].stable_id);
        if (slot_index >= 0)
        {
            reserved[slot_index] = 1;
            item_assigned[i] = 1;
            (void)egui_view_virtual_viewport_bind_slot(self, local, &local->slots[slot_index], &items[i]);
        }
    }

    egui_view_virtual_viewport_prepare_unreserved_slots(self, local, reserved);

    for (i = 0; i < item_count; i++)
    {
        int slot_index;

        if (item_assigned[i])
        {
            continue;
        }

        slot_index = egui_view_virtual_viewport_find_slot_by_state_and_type(local, reserved, EGUI_VIEW_VIRTUAL_SLOT_STATE_POOLED, items[i].view_type);
        if (slot_index < 0)
        {
            slot_index = egui_view_virtual_viewport_find_unused_slot(local, reserved);
        }
        if (slot_index < 0)
        {
            slot_index = egui_view_virtual_viewport_find_slot_by_state_and_type(local, reserved, EGUI_VIEW_VIRTUAL_SLOT_STATE_KEEPALIVE, items[i].view_type);
            if (slot_index >= 0)
            {
                egui_view_virtual_viewport_recycle_slot(self, local, &local->slots[slot_index]);
            }
        }
        if (slot_index < 0)
        {
            slot_index = egui_view_virtual_viewport_find_retypeable_slot(local, reserved);
        }
        if (slot_index < 0)
        {
            continue;
        }

        if (egui_view_virtual_viewport_bind_slot(self, local, &local->slots[slot_index], &items[i]))
        {
            reserved[slot_index] = 1;
            item_assigned[i] = 1;
        }
    }

    egui_view_virtual_viewport_prepare_unreserved_slots(self, local, reserved);
    egui_view_virtual_viewport_compact_slot_count(local);
    egui_view_virtual_viewport_update_anchor(local, items, item_count);
    local->is_data_dirty = 0;
    local->is_layout_dirty = 0;
}

static int egui_view_virtual_viewport_can_scroll(egui_view_virtual_viewport_t *local)
{
    return local->logical_extent > local->viewport_extent;
}

static egui_dim_t egui_view_virtual_viewport_get_motion_main(egui_view_virtual_viewport_t *local, const egui_motion_event_t *event)
{
    if (local->orientation == EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_HORIZONTAL)
    {
        return event->location.x;
    }

    return event->location.y;
}

static egui_float_t egui_view_virtual_viewport_get_velocity_main(egui_view_virtual_viewport_t *local)
{
    if (local->orientation == EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_HORIZONTAL)
    {
        return egui_input_get_velocity_x();
    }

    return egui_input_get_velocity_y();
}

static void egui_view_virtual_viewport_check_begin_dragged(egui_view_t *self, egui_dim_t delta)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);

    if (!local->is_begin_dragged && EGUI_ABS(delta) > local->touch_slop)
    {
        local->is_begin_dragged = 1;
        if (self->parent != NULL)
        {
            egui_view_group_request_disallow_intercept_touch_event(EGUI_VIEW_OF(self->parent), 1);
        }
    }
}

static void egui_view_virtual_viewport_fling(egui_view_t *self, egui_float_t velocity)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    int32_t delta;

    if (!egui_view_virtual_viewport_can_scroll(local) || velocity == 0)
    {
        return;
    }

    if (velocity < 0)
    {
        delta = egui_view_virtual_viewport_get_max_offset(local) - local->logical_offset;
    }
    else
    {
        delta = local->logical_offset;
    }

    if (delta > 0)
    {
        egui_scroller_start_filing(&local->scroller, (egui_dim_t)delta, velocity);
    }
}

static int egui_view_virtual_viewport_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);

    if (!egui_view_virtual_viewport_can_scroll(local))
    {
        return egui_view_group_on_intercept_touch_event(self, event);
    }

    if ((event->type == EGUI_MOTION_EVENT_ACTION_MOVE) && local->is_begin_dragged)
    {
        return 1;
    }

    if (egui_view_group_on_intercept_touch_event(self, event))
    {
        return 1;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        egui_view_virtual_viewport_check_begin_dragged(self, egui_view_virtual_viewport_get_motion_main(local, event) - local->last_motion_main);
        break;
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        local->last_motion_main = egui_view_virtual_viewport_get_motion_main(local, event);
        local->is_begin_dragged = !local->scroller.finished;
        break;
    case EGUI_MOTION_EVENT_ACTION_UP:
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->is_begin_dragged = 0;
        break;
    default:
        break;
    }

    return local->is_begin_dragged;
}

static int egui_view_virtual_viewport_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        if (!local->scroller.finished)
        {
            egui_scroller_about_animation(&local->scroller);
        }
        local->last_motion_main = egui_view_virtual_viewport_get_motion_main(local, event);
        return egui_view_virtual_viewport_can_scroll(local);
    case EGUI_MOTION_EVENT_ACTION_MOVE:
    {
        egui_dim_t delta = egui_view_virtual_viewport_get_motion_main(local, event) - local->last_motion_main;
        egui_view_virtual_viewport_check_begin_dragged(self, delta);
        if (local->is_begin_dragged)
        {
            local->last_motion_main = egui_view_virtual_viewport_get_motion_main(local, event);
            egui_view_virtual_viewport_scroll_by(self, -(int32_t)delta);
            return 1;
        }
        return 0;
    }
    case EGUI_MOTION_EVENT_ACTION_UP:
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        if (local->is_begin_dragged)
        {
            egui_view_virtual_viewport_fling(self, egui_view_virtual_viewport_get_velocity_main(local));
            local->is_begin_dragged = 0;
            return 1;
        }
        return 0;
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    case EGUI_MOTION_EVENT_ACTION_SCROLL:
        if (!egui_view_virtual_viewport_can_scroll(local))
        {
            return 0;
        }
        if (event->scroll_delta > 0)
        {
            egui_view_virtual_viewport_scroll_by(self, -local->estimated_item_extent);
        }
        else if (event->scroll_delta < 0)
        {
            egui_view_virtual_viewport_scroll_by(self, local->estimated_item_extent);
        }
        return 1;
#endif
    default:
        return 0;
    }
}

static void egui_view_virtual_viewport_compute_scroll(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    int offset;

    egui_view_group_compute_scroll(self);

    offset = egui_scroller_compute_scroll_offset(&local->scroller);
    if (offset != 0)
    {
        egui_view_virtual_viewport_scroll_by(self, -offset);
    }
}

static void egui_view_virtual_viewport_calculate_layout(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    int32_t old_viewport_extent = local->viewport_extent;
    int32_t cross_extent = egui_view_virtual_viewport_get_cross_extent(self, local);

    egui_view_virtual_viewport_sync_metrics(self, local);
    if (old_viewport_extent != local->viewport_extent)
    {
        local->is_layout_dirty = 1;
    }
    if (local->adapter != NULL && local->adapter->measure_main_size != NULL && local->indexed_cross_extent != 0 && local->indexed_cross_extent != cross_extent)
    {
        egui_view_virtual_viewport_invalidate_all_index_blocks(local);
        local->is_layout_dirty = 1;
    }

    egui_view_virtual_viewport_sync_content_layer(self, local);
    egui_view_virtual_viewport_clamp_offset(local);

    if (local->is_data_dirty || local->is_layout_dirty)
    {
        egui_view_virtual_viewport_sync_slots(self);
    }

    egui_view_group_calculate_layout(self);
}

static void egui_view_virtual_viewport_draw(egui_view_t *self)
{
    egui_canvas_set_extra_clip(&self->region_screen);
    egui_view_group_draw(self);
    egui_canvas_clear_extra_clip();
}

static void egui_view_virtual_viewport_on_attach_to_window(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);

    local->is_attached_to_window = 1;
    egui_view_group_on_attach_to_window(self);
}

static void egui_view_virtual_viewport_on_detach_from_window(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);

    local->is_attached_to_window = 0;
    egui_view_group_on_detach_from_window(self);
}

void egui_view_virtual_viewport_set_adapter(egui_view_t *self, const egui_view_virtual_viewport_adapter_t *adapter, void *adapter_context)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);

    if (local->adapter == adapter && local->adapter_context == adapter_context)
    {
        return;
    }

    egui_view_virtual_viewport_destroy_all_slots(self, local);
    egui_view_virtual_viewport_release_index_storage(local);
    egui_view_virtual_viewport_clear_state_cache_internal(local);
    local->adapter = adapter;
    local->adapter_context = adapter_context;
    egui_view_virtual_viewport_mark_dirty(self, local, 1, 1);
}

const egui_view_virtual_viewport_adapter_t *egui_view_virtual_viewport_get_adapter(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    return local->adapter;
}

void *egui_view_virtual_viewport_get_adapter_context(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    return local->adapter_context;
}

void egui_view_virtual_viewport_set_orientation(egui_view_t *self, uint8_t orientation)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);

    if (orientation > EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_HORIZONTAL)
    {
        orientation = EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_VERTICAL;
    }

    if (local->orientation == orientation)
    {
        return;
    }

    local->orientation = orientation;
    egui_view_virtual_viewport_invalidate_all_index_blocks(local);
    egui_view_virtual_viewport_mark_dirty(self, local, 0, 1);
}

uint8_t egui_view_virtual_viewport_get_orientation(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    return local->orientation;
}

void egui_view_virtual_viewport_set_overscan(egui_view_t *self, uint8_t before, uint8_t after)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);

    if (local->overscan_before == before && local->overscan_after == after)
    {
        return;
    }

    local->overscan_before = before;
    local->overscan_after = after;
    egui_view_virtual_viewport_mark_dirty(self, local, 0, 1);
}

uint8_t egui_view_virtual_viewport_get_overscan_before(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    return local->overscan_before;
}

uint8_t egui_view_virtual_viewport_get_overscan_after(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    return local->overscan_after;
}

void egui_view_virtual_viewport_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);

    if (local->max_keepalive_slots == max_keepalive_slots)
    {
        return;
    }

    local->max_keepalive_slots = max_keepalive_slots;
    egui_view_virtual_viewport_mark_dirty(self, local, 0, 1);
}

uint8_t egui_view_virtual_viewport_get_keepalive_limit(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    return local->max_keepalive_slots;
}

void egui_view_virtual_viewport_set_state_cache_limits(egui_view_t *self, uint16_t max_entries, uint32_t max_bytes)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);

    if (local->max_state_entries == max_entries && local->max_state_bytes == max_bytes)
    {
        return;
    }

    local->max_state_entries = max_bytes == 0 ? 0 : max_entries;
    local->max_state_bytes = max_entries == 0 ? 0 : max_bytes;

    if (local->max_state_entries == 0 || local->max_state_bytes == 0)
    {
        local->max_state_entries = 0;
        local->max_state_bytes = 0;
        egui_view_virtual_viewport_clear_state_cache_internal(local);
        return;
    }

    egui_view_virtual_viewport_trim_state_cache(local);
}

uint16_t egui_view_virtual_viewport_get_state_cache_entry_limit(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    return local->max_state_entries;
}

uint32_t egui_view_virtual_viewport_get_state_cache_byte_limit(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    return local->max_state_bytes;
}

void egui_view_virtual_viewport_clear_state_cache(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    egui_view_virtual_viewport_clear_state_cache_internal(local);
}

void egui_view_virtual_viewport_remove_state_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    egui_view_virtual_viewport_state_entry_t *entry = egui_view_virtual_viewport_find_state_entry(local, stable_id);

    egui_view_virtual_viewport_remove_state_entry(local, entry);
}

uint8_t egui_view_virtual_viewport_write_state(egui_view_t *self, uint32_t stable_id, const void *data, uint16_t size)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    egui_view_virtual_viewport_state_entry_t *entry;

    if (stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        return 0;
    }

    if (size == 0)
    {
        egui_view_virtual_viewport_remove_state_by_stable_id(self, stable_id);
        return 1;
    }

    if (data == NULL || local->max_state_entries == 0 || local->max_state_bytes == 0 || size > local->max_state_bytes)
    {
        return 0;
    }

    entry = egui_view_virtual_viewport_find_state_entry(local, stable_id);
    if (entry != NULL && entry->size == size)
    {
        memcpy(entry->data, data, size);
        egui_view_virtual_viewport_touch_state_entry(local, entry);
        return 1;
    }

    entry = (egui_view_virtual_viewport_state_entry_t *)egui_malloc((int)(sizeof(egui_view_virtual_viewport_state_entry_t) + size));
    if (entry == NULL)
    {
        return 0;
    }

    entry->stable_id = stable_id;
    entry->size = size;
    memcpy(entry->data, data, size);
    egui_dlist_append(&local->state_entries, &entry->node);
    local->state_entry_count++;
    local->state_total_bytes += size;

    {
        egui_view_virtual_viewport_state_entry_t *old_entry = egui_view_virtual_viewport_find_state_entry(local, stable_id);
        if (old_entry != NULL && old_entry != entry)
        {
            egui_view_virtual_viewport_remove_state_entry(local, old_entry);
        }
    }

    egui_view_virtual_viewport_trim_state_cache(local);
    return 1;
}

uint16_t egui_view_virtual_viewport_read_state(egui_view_t *self, uint32_t stable_id, void *data, uint16_t capacity)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    egui_view_virtual_viewport_state_entry_t *entry = egui_view_virtual_viewport_find_state_entry(local, stable_id);

    if (entry == NULL)
    {
        return 0;
    }

    egui_view_virtual_viewport_touch_state_entry(local, entry);
    if (data != NULL && capacity > 0)
    {
        memcpy(data, entry->data, entry->size < capacity ? entry->size : capacity);
    }

    return entry->size;
}

uint8_t egui_view_virtual_viewport_write_state_for_view(egui_view_t *item_view, uint32_t stable_id, const void *data, uint16_t size)
{
    egui_view_t *viewport = egui_view_virtual_viewport_find_host_view(item_view);

    if (viewport == NULL)
    {
        return 0;
    }

    return egui_view_virtual_viewport_write_state(viewport, stable_id, data, size);
}

uint16_t egui_view_virtual_viewport_read_state_for_view(egui_view_t *item_view, uint32_t stable_id, void *data, uint16_t capacity)
{
    egui_view_t *viewport = egui_view_virtual_viewport_find_host_view(item_view);

    if (viewport == NULL)
    {
        return 0;
    }

    return egui_view_virtual_viewport_read_state(viewport, stable_id, data, capacity);
}

void egui_view_virtual_viewport_set_estimated_item_extent(egui_view_t *self, int32_t extent)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);

    if (extent <= 0)
    {
        extent = 1;
    }

    if (local->estimated_item_extent == extent)
    {
        return;
    }

    local->estimated_item_extent = extent;
    egui_view_virtual_viewport_invalidate_all_index_blocks(local);
    egui_view_virtual_viewport_mark_dirty(self, local, 0, 1);
}

int32_t egui_view_virtual_viewport_get_estimated_item_extent(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    return local->estimated_item_extent;
}

void egui_view_virtual_viewport_set_logical_extent(egui_view_t *self, int32_t extent)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);

    if (extent < 0)
    {
        extent = 0;
    }

    if (local->logical_extent == extent)
    {
        return;
    }

    local->logical_extent = extent;
    egui_view_virtual_viewport_clamp_offset(local);
    egui_view_virtual_viewport_mark_dirty(self, local, 0, 1);
}

int32_t egui_view_virtual_viewport_get_logical_extent(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    return local->logical_extent;
}

void egui_view_virtual_viewport_set_logical_offset(egui_view_t *self, int32_t offset)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);

    local->logical_offset = offset;
    egui_view_virtual_viewport_clamp_offset(local);
    egui_view_virtual_viewport_mark_dirty(self, local, 0, 1);
}

void egui_view_virtual_viewport_scroll_by(egui_view_t *self, int32_t delta)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    egui_view_virtual_viewport_set_logical_offset(self, local->logical_offset + delta);
}

void egui_view_virtual_viewport_scroll_to_item(egui_view_t *self, uint32_t index, int32_t item_offset)
{
    int32_t origin = egui_view_virtual_viewport_get_item_main_origin(self, index);

    if (origin < 0)
    {
        return;
    }

    egui_view_virtual_viewport_set_logical_offset(self, origin + item_offset);
}

void egui_view_virtual_viewport_scroll_to_stable_id(egui_view_t *self, uint32_t stable_id, int32_t item_offset)
{
    int32_t index = egui_view_virtual_viewport_find_item_index_by_stable_id(self, stable_id);

    if (index < 0)
    {
        return;
    }

    egui_view_virtual_viewport_scroll_to_item(self, (uint32_t)index, item_offset);
}

int32_t egui_view_virtual_viewport_get_logical_offset(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    return local->logical_offset;
}

int32_t egui_view_virtual_viewport_find_item_index_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    uint32_t count = egui_view_virtual_viewport_get_item_count(local);
    uint32_t index;

    if (stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        return -1;
    }

    if (local->adapter != NULL && local->adapter->find_index_by_stable_id != NULL)
    {
        int32_t found_index = local->adapter->find_index_by_stable_id(local->adapter_context, stable_id);
        if (found_index >= 0 && (uint32_t)found_index < count)
        {
            return found_index;
        }
    }

    for (index = 0; index < count; index++)
    {
        if (egui_view_virtual_viewport_get_item_stable_id(local, index) == stable_id)
        {
            return (int32_t)index;
        }
    }

    return -1;
}

int32_t egui_view_virtual_viewport_get_item_main_origin(egui_view_t *self, uint32_t index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    return egui_view_virtual_viewport_get_item_main_origin_internal(self, local, index);
}

int32_t egui_view_virtual_viewport_get_item_main_size(egui_view_t *self, uint32_t index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    return egui_view_virtual_viewport_get_item_main_size_internal(self, local, index);
}

uint8_t egui_view_virtual_viewport_is_main_span_center_visible(egui_view_t *self, int32_t main_origin, int32_t main_size)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    int32_t viewport_extent = egui_view_virtual_viewport_get_main_extent(self, local);
    int32_t center;

    if (main_size <= 0 || viewport_extent <= 0)
    {
        return 0;
    }

    center = main_origin + main_size / 2;
    return (uint8_t)(center >= 0 && center < viewport_extent);
}

uint8_t egui_view_virtual_viewport_is_main_span_fully_visible(egui_view_t *self, int32_t main_origin, int32_t main_size, int32_t inset)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    int32_t viewport_extent = egui_view_virtual_viewport_get_main_extent(self, local);
    int32_t main_end;

    if (main_size <= 0 || viewport_extent <= 0)
    {
        return 0;
    }

    main_end = main_origin + main_size;
    return (uint8_t)(main_origin >= (-inset) && main_end <= (viewport_extent + inset));
}

void egui_view_virtual_viewport_set_anchor(egui_view_t *self, uint32_t stable_id, int32_t anchor_offset)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);

    local->anchor_stable_id = stable_id;
    local->anchor_offset = anchor_offset;
    egui_view_virtual_viewport_mark_dirty(self, local, 0, 1);
}

uint32_t egui_view_virtual_viewport_get_anchor_stable_id(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    return local->anchor_stable_id;
}

int32_t egui_view_virtual_viewport_get_anchor_offset(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    return local->anchor_offset;
}

void egui_view_virtual_viewport_notify_data_changed(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    egui_view_virtual_viewport_invalidate_all_index_blocks(local);
    egui_view_virtual_viewport_mark_dirty(self, local, 1, 1);
}

void egui_view_virtual_viewport_notify_item_changed(egui_view_t *self, uint32_t index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    egui_view_virtual_viewport_invalidate_index_block(local, index);
    egui_view_virtual_viewport_mark_dirty(self, local, 1, 1);
}

void egui_view_virtual_viewport_notify_item_changed_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t index = egui_view_virtual_viewport_find_item_index_by_stable_id(self, stable_id);

    if (index < 0)
    {
        return;
    }

    egui_view_virtual_viewport_notify_item_changed(self, (uint32_t)index);
}

void egui_view_virtual_viewport_notify_item_inserted(egui_view_t *self, uint32_t index, uint32_t count)
{
    EGUI_UNUSED(index);
    EGUI_UNUSED(count);

    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    egui_view_virtual_viewport_invalidate_all_index_blocks(local);
    egui_view_virtual_viewport_mark_dirty(self, local, 1, 1);
}

void egui_view_virtual_viewport_notify_item_removed(egui_view_t *self, uint32_t index, uint32_t count)
{
    EGUI_UNUSED(index);
    EGUI_UNUSED(count);

    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    egui_view_virtual_viewport_invalidate_all_index_blocks(local);
    egui_view_virtual_viewport_mark_dirty(self, local, 1, 1);
}

void egui_view_virtual_viewport_notify_item_moved(egui_view_t *self, uint32_t from_index, uint32_t to_index)
{
    EGUI_UNUSED(from_index);
    EGUI_UNUSED(to_index);

    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    egui_view_virtual_viewport_invalidate_all_index_blocks(local);
    egui_view_virtual_viewport_mark_dirty(self, local, 1, 1);
}

void egui_view_virtual_viewport_notify_item_resized(egui_view_t *self, uint32_t index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    egui_view_virtual_viewport_invalidate_index_block(local, index);
    egui_view_virtual_viewport_mark_dirty(self, local, 1, 1);
}

void egui_view_virtual_viewport_notify_item_resized_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t index = egui_view_virtual_viewport_find_item_index_by_stable_id(self, stable_id);

    if (index < 0)
    {
        return;
    }

    egui_view_virtual_viewport_notify_item_resized(self, (uint32_t)index);
}

egui_view_t *egui_view_virtual_viewport_get_content_layer(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    return EGUI_VIEW_OF(&local->content_layer);
}

uint8_t egui_view_virtual_viewport_get_slot_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    return local->slot_count;
}

const egui_view_virtual_viewport_slot_t *egui_view_virtual_viewport_get_slot(egui_view_t *self, uint8_t slot_index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);

    if (slot_index >= EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS)
    {
        return NULL;
    }

    return &local->slots[slot_index];
}

int32_t egui_view_virtual_viewport_find_slot_index_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    return egui_view_virtual_viewport_find_slot_index_by_stable_id_internal(local, stable_id);
}

const egui_view_virtual_viewport_slot_t *egui_view_virtual_viewport_find_slot_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t slot_index = egui_view_virtual_viewport_find_slot_index_by_stable_id(self, stable_id);

    return slot_index >= 0 ? egui_view_virtual_viewport_get_slot(self, (uint8_t)slot_index) : NULL;
}

egui_view_t *egui_view_virtual_viewport_find_view_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    const egui_view_virtual_viewport_slot_t *slot = egui_view_virtual_viewport_find_slot_by_stable_id(self, stable_id);

    return slot != NULL ? slot->view : NULL;
}

uint8_t egui_view_virtual_viewport_get_slot_entry(egui_view_t *self, uint8_t slot_index, egui_view_virtual_viewport_entry_t *entry)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    const egui_view_virtual_viewport_slot_t *slot = egui_view_virtual_viewport_get_slot(self, slot_index);

    return egui_view_virtual_viewport_resolve_slot_internal(local, slot, entry);
}

uint8_t egui_view_virtual_viewport_resolve_item_by_stable_id(egui_view_t *self, uint32_t stable_id, egui_view_virtual_viewport_entry_t *entry)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    int32_t index;

    egui_view_virtual_viewport_reset_entry(entry);

    if (stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        return 0;
    }

    index = egui_view_virtual_viewport_find_item_index_by_stable_id(self, stable_id);
    if (index < 0)
    {
        return 0;
    }

    return egui_view_virtual_viewport_resolve_item_by_index_internal(local, (uint32_t)index, entry);
}

uint8_t egui_view_virtual_viewport_resolve_item_by_view(egui_view_t *self, egui_view_t *item_view, egui_view_virtual_viewport_entry_t *entry)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);
    const egui_view_virtual_viewport_slot_t *slot = egui_view_virtual_viewport_find_slot_by_view(local, item_view);

    return egui_view_virtual_viewport_resolve_slot_internal(local, slot, entry);
}

uint8_t egui_view_virtual_viewport_ensure_item_visible_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t inset)
{
    const egui_view_virtual_viewport_slot_t *slot;

    if (stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID || egui_view_virtual_viewport_find_item_index_by_stable_id(self, stable_id) < 0)
    {
        return 0;
    }

    slot = egui_view_virtual_viewport_find_slot_by_stable_id(self, stable_id);
    if (egui_view_virtual_viewport_is_slot_fully_visible(self, slot, inset))
    {
        return 1;
    }

    egui_view_virtual_viewport_scroll_to_stable_id(self, stable_id, inset);
    return 1;
}

uint8_t egui_view_virtual_viewport_visit_visible_items(egui_view_t *self, egui_view_virtual_viewport_visible_item_visitor_t visitor, void *context)
{
    uint8_t slot_count;
    uint8_t slot_index;
    uint8_t visited = 0;

    if (visitor == NULL)
    {
        return 0;
    }

    slot_count = egui_view_virtual_viewport_get_slot_count(self);
    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_viewport_entry_t entry;
        const egui_view_virtual_viewport_slot_t *slot = egui_view_virtual_viewport_get_slot(self, slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL)
        {
            continue;
        }
        if (!egui_view_virtual_viewport_get_slot_entry(self, slot_index, &entry))
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

typedef struct egui_view_virtual_viewport_find_visible_item_context
{
    egui_view_virtual_viewport_visible_item_matcher_t matcher;
    void *context;
    uint32_t best_index;
    egui_view_t *best_view;
    egui_view_virtual_viewport_entry_t best_entry;
} egui_view_virtual_viewport_find_visible_item_context_t;

static uint8_t egui_view_virtual_viewport_find_visible_item_visitor(egui_view_t *self, const egui_view_virtual_viewport_slot_t *slot,
                                                                    const egui_view_virtual_viewport_entry_t *entry, egui_view_t *item_view, void *context)
{
    egui_view_virtual_viewport_find_visible_item_context_t *ctx = (egui_view_virtual_viewport_find_visible_item_context_t *)context;

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

egui_view_t *egui_view_virtual_viewport_find_first_visible_item_view(egui_view_t *self, egui_view_virtual_viewport_visible_item_matcher_t matcher,
                                                                     void *context, egui_view_virtual_viewport_entry_t *entry_out)
{
    egui_view_virtual_viewport_find_visible_item_context_t find_ctx = {
            .matcher = matcher,
            .context = context,
            .best_index = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID,
            .best_view = NULL,
    };

    egui_view_virtual_viewport_visit_visible_items(self, egui_view_virtual_viewport_find_visible_item_visitor, &find_ctx);
    if (find_ctx.best_view != NULL && entry_out != NULL)
    {
        *entry_out = find_ctx.best_entry;
    }

    return find_ctx.best_view;
}

uint8_t egui_view_virtual_viewport_is_slot_center_visible(egui_view_t *self, const egui_view_virtual_viewport_slot_t *slot)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);

    if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL)
    {
        return 0;
    }

    return egui_view_virtual_viewport_is_main_span_center_visible(self, egui_view_virtual_viewport_get_slot_main_origin(local, slot),
                                                                  egui_view_virtual_viewport_get_slot_main_size(local, slot));
}

uint8_t egui_view_virtual_viewport_is_slot_fully_visible(egui_view_t *self, const egui_view_virtual_viewport_slot_t *slot, int32_t inset)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);

    if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL)
    {
        return 0;
    }

    return egui_view_virtual_viewport_is_main_span_fully_visible(self, egui_view_virtual_viewport_get_slot_main_origin(local, slot),
                                                                 egui_view_virtual_viewport_get_slot_main_size(local, slot), inset);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_virtual_viewport_t) = {
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .dispatch_touch_event = egui_view_group_dispatch_touch_event,
        .on_touch_event = egui_view_virtual_viewport_on_touch_event,
        .on_intercept_touch_event = egui_view_virtual_viewport_on_intercept_touch_event,
#else
        .dispatch_touch_event = egui_view_group_dispatch_touch_event,
        .on_touch_event = NULL,
        .on_intercept_touch_event = NULL,
#endif
        .compute_scroll = egui_view_virtual_viewport_compute_scroll,
        .calculate_layout = egui_view_virtual_viewport_calculate_layout,
        .request_layout = egui_view_group_request_layout,
        .draw = egui_view_virtual_viewport_draw,
        .on_attach_to_window = egui_view_virtual_viewport_on_attach_to_window,
        .on_draw = egui_view_on_draw,
        .on_detach_from_window = egui_view_virtual_viewport_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_group_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_virtual_viewport_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_virtual_viewport_t);

    egui_view_group_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_virtual_viewport_t);

    egui_view_group_init(EGUI_VIEW_OF(&local->content_layer));
    egui_view_set_position(EGUI_VIEW_OF(&local->content_layer), 0, 0);
    egui_view_group_add_child(self, EGUI_VIEW_OF(&local->content_layer));

    local->adapter = NULL;
    local->adapter_context = NULL;
    local->orientation = EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_VERTICAL;
    local->overscan_before = 1;
    local->overscan_after = 1;
    local->max_keepalive_slots = 2;
    local->is_data_dirty = 1;
    local->is_layout_dirty = 1;
    local->is_attached_to_window = 0;
    local->is_begin_dragged = 0;
    local->touch_slop = 5;
    local->last_motion_main = 0;
    local->anchor_stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    local->slot_use_seq = 0;
    local->anchor_offset = 0;
    local->logical_offset = 0;
    local->logical_extent = 0;
    local->viewport_extent = 0;
    local->cross_extent = 0;
    local->estimated_item_extent = 40;
    local->are_index_prefixes_dirty = 1;
    local->index_block_count = 0;
    local->index_block_capacity = 0;
    local->max_state_entries = 0;
    local->state_entry_count = 0;
    local->indexed_item_count = 0;
    local->max_state_bytes = 0;
    local->state_total_bytes = 0;
    local->indexed_cross_extent = 0;
    local->index_block_extents = NULL;
    local->index_block_origins = NULL;
    local->index_block_valid = NULL;
    egui_dlist_init(&local->state_entries);

    egui_scroller_init(&local->scroller);
    egui_view_virtual_viewport_reset_slots(local);
    egui_view_set_view_name(self, "egui_view_virtual_viewport");
}

void egui_view_virtual_viewport_apply_params(egui_view_t *self, const egui_view_virtual_viewport_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_virtual_viewport_t);

    egui_view_set_position(self, params->region.location.x, params->region.location.y);
    egui_view_set_size(self, params->region.size.width, params->region.size.height);

    local->orientation =
            params->orientation > EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_HORIZONTAL ? EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_VERTICAL : params->orientation;
    local->overscan_before = params->overscan_before;
    local->overscan_after = params->overscan_after;
    local->max_keepalive_slots = params->max_keepalive_slots;
    local->estimated_item_extent = params->estimated_item_extent > 0 ? params->estimated_item_extent : 1;
    egui_view_virtual_viewport_invalidate_all_index_blocks(local);

    egui_view_virtual_viewport_sync_metrics(self, local);
    egui_view_virtual_viewport_sync_content_layer(self, local);
    egui_view_virtual_viewport_clamp_offset(local);
    egui_view_virtual_viewport_mark_dirty(self, local, 0, 1);
}

void egui_view_virtual_viewport_init_with_params(egui_view_t *self, const egui_view_virtual_viewport_params_t *params)
{
    egui_view_virtual_viewport_init(self);
    egui_view_virtual_viewport_apply_params(self, params);
}

void egui_view_virtual_viewport_apply_setup(egui_view_t *self, const egui_view_virtual_viewport_setup_t *setup)
{
    if (setup == NULL)
    {
        return;
    }

    if (setup->params != NULL)
    {
        egui_view_virtual_viewport_apply_params(self, setup->params);
    }

    egui_view_virtual_viewport_set_adapter(self, setup->adapter, setup->adapter_context);
    egui_view_virtual_viewport_set_state_cache_limits(self, setup->state_cache_max_entries, setup->state_cache_max_bytes);
}

void egui_view_virtual_viewport_init_with_setup(egui_view_t *self, const egui_view_virtual_viewport_setup_t *setup)
{
    egui_view_virtual_viewport_init(self);
    egui_view_virtual_viewport_apply_setup(self, setup);
}
