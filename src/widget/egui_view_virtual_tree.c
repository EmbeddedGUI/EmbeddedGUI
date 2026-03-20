#include "egui_view_virtual_tree.h"

#include <string.h>

typedef struct egui_view_virtual_tree_iter_frame
{
    uint32_t parent_stable_id;
    uint32_t child_count;
    uint32_t next_child_index;
    uint8_t depth;
    uint8_t children_visible;
} egui_view_virtual_tree_iter_frame_t;

static uint32_t egui_view_virtual_tree_default_stable_id(uint32_t index)
{
    return index + 1U;
}

static void egui_view_virtual_tree_reset_entry(egui_view_virtual_tree_entry_t *entry)
{
    if (entry == NULL)
    {
        return;
    }

    entry->visible_index = EGUI_VIEW_VIRTUAL_TREE_INVALID_INDEX;
    entry->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    entry->parent_stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    entry->child_count = 0;
    entry->depth = 0;
    entry->has_children = 0;
    entry->is_expanded = 0;
}

static egui_view_virtual_tree_t *egui_view_virtual_tree_from_flat_context(void *adapter_context)
{
    return (egui_view_virtual_tree_t *)adapter_context;
}

static const egui_view_virtual_tree_data_source_t *egui_view_virtual_tree_get_data_source_bridge(egui_view_virtual_tree_t *local, void **data_source_context)
{
    if (data_source_context != NULL)
    {
        *data_source_context = local->data_source_context;
    }

    return local->data_source;
}

static uint32_t egui_view_virtual_tree_get_root_count_internal(egui_view_virtual_tree_t *local)
{
    void *data_source_context;
    const egui_view_virtual_tree_data_source_t *data_source = egui_view_virtual_tree_get_data_source_bridge(local, &data_source_context);

    if (data_source == NULL || data_source->get_root_count == NULL)
    {
        return 0;
    }

    return data_source->get_root_count(data_source_context);
}

static uint32_t egui_view_virtual_tree_get_root_stable_id_internal(egui_view_virtual_tree_t *local, uint32_t root_index)
{
    void *data_source_context;
    const egui_view_virtual_tree_data_source_t *data_source = egui_view_virtual_tree_get_data_source_bridge(local, &data_source_context);

    if (data_source != NULL && data_source->get_root_stable_id != NULL)
    {
        return data_source->get_root_stable_id(data_source_context, root_index);
    }

    return egui_view_virtual_tree_default_stable_id(root_index);
}

static uint32_t egui_view_virtual_tree_get_child_count_internal(egui_view_virtual_tree_t *local, uint32_t stable_id)
{
    void *data_source_context;
    const egui_view_virtual_tree_data_source_t *data_source = egui_view_virtual_tree_get_data_source_bridge(local, &data_source_context);

    if (data_source == NULL || data_source->get_child_count == NULL)
    {
        return 0;
    }

    return data_source->get_child_count(data_source_context, stable_id);
}

static uint32_t egui_view_virtual_tree_get_child_stable_id_internal(egui_view_virtual_tree_t *local, uint32_t stable_id, uint32_t child_index)
{
    void *data_source_context;
    const egui_view_virtual_tree_data_source_t *data_source = egui_view_virtual_tree_get_data_source_bridge(local, &data_source_context);

    if (data_source != NULL && data_source->get_child_stable_id != NULL)
    {
        return data_source->get_child_stable_id(data_source_context, stable_id, child_index);
    }

    return EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static uint8_t egui_view_virtual_tree_is_node_expanded_internal(egui_view_virtual_tree_t *local, uint32_t stable_id, uint8_t has_children)
{
    void *data_source_context;
    const egui_view_virtual_tree_data_source_t *data_source = egui_view_virtual_tree_get_data_source_bridge(local, &data_source_context);

    if (!has_children)
    {
        return 0;
    }

    if (data_source != NULL && data_source->is_node_expanded != NULL)
    {
        return data_source->is_node_expanded(data_source_context, stable_id);
    }

    return 1;
}

static uint8_t egui_view_virtual_tree_walk_internal(egui_view_virtual_tree_t *local, uint8_t traverse_all_nodes, uint32_t target_visible_index,
                                                    uint32_t target_stable_id, egui_view_virtual_tree_entry_t *entry, uint32_t *visible_count_out)
{
    egui_view_virtual_tree_iter_frame_t frames[EGUI_VIEW_VIRTUAL_TREE_MAX_DEPTH];
    uint32_t visible_count = 0;
    int32_t top = 0;
    uint8_t found = 0;

    egui_view_virtual_tree_reset_entry(entry);

    frames[0].parent_stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    frames[0].child_count = egui_view_virtual_tree_get_root_count_internal(local);
    frames[0].next_child_index = 0;
    frames[0].depth = 0;
    frames[0].children_visible = 1;

    while (top >= 0)
    {
        egui_view_virtual_tree_iter_frame_t *frame = &frames[top];
        egui_view_virtual_tree_entry_t current;
        uint32_t child_index;
        uint32_t stable_id;
        uint32_t child_count;

        if (frame->next_child_index >= frame->child_count)
        {
            top--;
            continue;
        }

        child_index = frame->next_child_index++;
        stable_id = frame->parent_stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID
                            ? egui_view_virtual_tree_get_root_stable_id_internal(local, child_index)
                            : egui_view_virtual_tree_get_child_stable_id_internal(local, frame->parent_stable_id, child_index);
        if (stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
        {
            continue;
        }

        child_count = egui_view_virtual_tree_get_child_count_internal(local, stable_id);

        current.visible_index = frame->children_visible ? visible_count : EGUI_VIEW_VIRTUAL_TREE_INVALID_INDEX;
        current.stable_id = stable_id;
        current.parent_stable_id = frame->parent_stable_id;
        current.child_count = child_count;
        current.depth = frame->depth;
        current.has_children = child_count > 0 ? 1U : 0U;
        current.is_expanded = egui_view_virtual_tree_is_node_expanded_internal(local, stable_id, current.has_children);

        if (frame->children_visible)
        {
            visible_count++;
        }

        if (target_visible_index != EGUI_VIEW_VIRTUAL_TREE_INVALID_INDEX && current.visible_index == target_visible_index)
        {
            if (entry != NULL)
            {
                *entry = current;
            }
            found = 1;
            if (target_stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID && visible_count_out == NULL)
            {
                return 1;
            }
        }

        if (target_stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID && current.stable_id == target_stable_id)
        {
            if (entry != NULL)
            {
                *entry = current;
            }
            found = 1;
            if (visible_count_out == NULL)
            {
                return 1;
            }
        }

        if (current.has_children && (top + 1) < (int32_t)EGUI_VIEW_VIRTUAL_TREE_MAX_DEPTH)
        {
            uint8_t children_visible = (uint8_t)(frame->children_visible && current.is_expanded);

            if (traverse_all_nodes || children_visible)
            {
                top++;
                frames[top].parent_stable_id = stable_id;
                frames[top].child_count = child_count;
                frames[top].next_child_index = 0;
                frames[top].depth = (uint8_t)(frame->depth + 1U);
                frames[top].children_visible = children_visible;
            }
        }
    }

    if (visible_count_out != NULL)
    {
        *visible_count_out = visible_count;
    }

    return found;
}

static uint8_t egui_view_virtual_tree_resolve_visible_index_internal(egui_view_virtual_tree_t *local, uint32_t visible_index,
                                                                     egui_view_virtual_tree_entry_t *entry)
{
    return egui_view_virtual_tree_walk_internal(local, 0, visible_index, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID, entry, NULL);
}

static uint8_t egui_view_virtual_tree_resolve_node_internal(egui_view_virtual_tree_t *local, uint32_t stable_id, egui_view_virtual_tree_entry_t *entry)
{
    return egui_view_virtual_tree_walk_internal(local, 1, EGUI_VIEW_VIRTUAL_TREE_INVALID_INDEX, stable_id, entry, NULL);
}

static const egui_view_virtual_tree_slot_t *egui_view_virtual_tree_find_slot_by_view(egui_view_virtual_tree_t *local, egui_view_t *view)
{
    uint8_t slot_index;

    for (slot_index = 0; slot_index < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; slot_index++)
    {
        const egui_view_virtual_tree_slot_t *slot = &local->base.base.slots[slot_index];

        if (slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED && slot->view == view)
        {
            return slot;
        }
    }

    return NULL;
}

static int32_t egui_view_virtual_tree_find_slot_index_by_stable_id_internal(egui_view_virtual_tree_t *local, uint32_t stable_id)
{
    uint8_t slot_index;

    for (slot_index = 0; slot_index < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; slot_index++)
    {
        const egui_view_virtual_tree_slot_t *slot = &local->base.base.slots[slot_index];

        if (slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED && slot->stable_id == stable_id)
        {
            return (int32_t)slot_index;
        }
    }

    return -1;
}

static uint8_t egui_view_virtual_tree_resolve_slot_internal(egui_view_virtual_tree_t *local, const egui_view_virtual_tree_slot_t *slot,
                                                            egui_view_virtual_tree_entry_t *entry)
{
    if (slot == NULL || slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_UNUSED)
    {
        egui_view_virtual_tree_reset_entry(entry);
        return 0;
    }

    if (egui_view_virtual_tree_resolve_visible_index_internal(local, slot->index, entry) && entry->stable_id == slot->stable_id)
    {
        return 1;
    }

    return egui_view_virtual_tree_resolve_node_internal(local, slot->stable_id, entry);
}

static uint32_t egui_view_virtual_tree_flat_get_count(void *adapter_context)
{
    egui_view_virtual_tree_t *local = egui_view_virtual_tree_from_flat_context(adapter_context);
    uint32_t visible_count = 0;

    (void)egui_view_virtual_tree_walk_internal(local, 0, EGUI_VIEW_VIRTUAL_TREE_INVALID_INDEX, EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID, NULL, &visible_count);
    return visible_count;
}

static uint32_t egui_view_virtual_tree_flat_get_stable_id(void *adapter_context, uint32_t index)
{
    egui_view_virtual_tree_t *local = egui_view_virtual_tree_from_flat_context(adapter_context);
    egui_view_virtual_tree_entry_t entry;

    if (!egui_view_virtual_tree_resolve_visible_index_internal(local, index, &entry))
    {
        return EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    }

    return entry.stable_id;
}

static int32_t egui_view_virtual_tree_flat_find_index_by_stable_id(void *adapter_context, uint32_t stable_id)
{
    egui_view_virtual_tree_t *local = egui_view_virtual_tree_from_flat_context(adapter_context);
    egui_view_virtual_tree_entry_t entry;

    if (!egui_view_virtual_tree_resolve_node_internal(local, stable_id, &entry) || entry.visible_index == EGUI_VIEW_VIRTUAL_TREE_INVALID_INDEX)
    {
        return -1;
    }

    return (int32_t)entry.visible_index;
}

static uint16_t egui_view_virtual_tree_flat_get_view_type(void *adapter_context, uint32_t index)
{
    egui_view_virtual_tree_t *local = egui_view_virtual_tree_from_flat_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_tree_data_source_t *data_source = egui_view_virtual_tree_get_data_source_bridge(local, &data_source_context);
    egui_view_virtual_tree_entry_t entry;

    if (!egui_view_virtual_tree_resolve_visible_index_internal(local, index, &entry))
    {
        return 0;
    }

    if (data_source != NULL && data_source->get_node_view_type != NULL)
    {
        return data_source->get_node_view_type(data_source_context, &entry);
    }

    return data_source != NULL ? data_source->default_view_type : 0;
}

static int32_t egui_view_virtual_tree_flat_measure_item_height(void *adapter_context, uint32_t index, int32_t width_hint)
{
    egui_view_virtual_tree_t *local = egui_view_virtual_tree_from_flat_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_tree_data_source_t *data_source = egui_view_virtual_tree_get_data_source_bridge(local, &data_source_context);
    egui_view_virtual_tree_entry_t entry;

    if (!egui_view_virtual_tree_resolve_visible_index_internal(local, index, &entry))
    {
        return egui_view_virtual_tree_get_estimated_node_height(EGUI_VIEW_OF(local));
    }

    if (data_source != NULL && data_source->measure_node_height != NULL)
    {
        return data_source->measure_node_height(data_source_context, &entry, width_hint);
    }

    return egui_view_virtual_tree_get_estimated_node_height(EGUI_VIEW_OF(local));
}

static egui_view_t *egui_view_virtual_tree_flat_create_view(void *adapter_context, uint16_t view_type)
{
    egui_view_virtual_tree_t *local = egui_view_virtual_tree_from_flat_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_tree_data_source_t *data_source = egui_view_virtual_tree_get_data_source_bridge(local, &data_source_context);

    if (data_source == NULL || data_source->create_node_view == NULL)
    {
        return NULL;
    }

    return data_source->create_node_view(data_source_context, view_type);
}

static void egui_view_virtual_tree_flat_destroy_view(void *adapter_context, egui_view_t *view, uint16_t view_type)
{
    egui_view_virtual_tree_t *local = egui_view_virtual_tree_from_flat_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_tree_data_source_t *data_source = egui_view_virtual_tree_get_data_source_bridge(local, &data_source_context);

    if (data_source != NULL && data_source->destroy_node_view != NULL)
    {
        data_source->destroy_node_view(data_source_context, view, view_type);
    }
}

static void egui_view_virtual_tree_flat_bind_view(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id)
{
    egui_view_virtual_tree_t *local = egui_view_virtual_tree_from_flat_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_tree_data_source_t *data_source = egui_view_virtual_tree_get_data_source_bridge(local, &data_source_context);
    egui_view_virtual_tree_entry_t entry;

    EGUI_UNUSED(stable_id);

    if (data_source == NULL || data_source->bind_node_view == NULL)
    {
        return;
    }
    if (!egui_view_virtual_tree_resolve_visible_index_internal(local, index, &entry))
    {
        return;
    }

    data_source->bind_node_view(data_source_context, view, &entry);
}

static void egui_view_virtual_tree_flat_unbind_view(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_virtual_tree_t *local = egui_view_virtual_tree_from_flat_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_tree_data_source_t *data_source = egui_view_virtual_tree_get_data_source_bridge(local, &data_source_context);

    if (data_source != NULL && data_source->unbind_node_view != NULL)
    {
        data_source->unbind_node_view(data_source_context, view, stable_id);
    }
}

static uint8_t egui_view_virtual_tree_flat_should_keep_alive(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_virtual_tree_t *local = egui_view_virtual_tree_from_flat_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_tree_data_source_t *data_source = egui_view_virtual_tree_get_data_source_bridge(local, &data_source_context);

    if (data_source != NULL && data_source->should_keep_alive != NULL)
    {
        return data_source->should_keep_alive(data_source_context, view, stable_id);
    }

    return 0;
}

static void egui_view_virtual_tree_flat_save_state(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_virtual_tree_t *local = egui_view_virtual_tree_from_flat_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_tree_data_source_t *data_source = egui_view_virtual_tree_get_data_source_bridge(local, &data_source_context);

    if (data_source != NULL && data_source->save_node_state != NULL)
    {
        data_source->save_node_state(data_source_context, view, stable_id);
    }
}

static void egui_view_virtual_tree_flat_restore_state(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_virtual_tree_t *local = egui_view_virtual_tree_from_flat_context(adapter_context);
    void *data_source_context;
    const egui_view_virtual_tree_data_source_t *data_source = egui_view_virtual_tree_get_data_source_bridge(local, &data_source_context);

    if (data_source != NULL && data_source->restore_node_state != NULL)
    {
        data_source->restore_node_state(data_source_context, view, stable_id);
    }
}

void egui_view_virtual_tree_apply_params(egui_view_t *self, const egui_view_virtual_tree_params_t *params)
{
    egui_view_virtual_list_params_t list_params;

    list_params.region = params->region;
    list_params.overscan_before = params->overscan_before;
    list_params.overscan_after = params->overscan_after;
    list_params.max_keepalive_slots = params->max_keepalive_slots;
    list_params.estimated_item_height = params->estimated_node_height;

    egui_view_virtual_list_apply_params(self, &list_params);
}

void egui_view_virtual_tree_init_with_params(egui_view_t *self, const egui_view_virtual_tree_params_t *params)
{
    egui_view_virtual_tree_init(self);
    egui_view_virtual_tree_apply_params(self, params);
}

void egui_view_virtual_tree_apply_setup(egui_view_t *self, const egui_view_virtual_tree_setup_t *setup)
{
    if (setup == NULL)
    {
        return;
    }

    if (setup->params != NULL)
    {
        egui_view_virtual_tree_apply_params(self, setup->params);
    }

    egui_view_virtual_tree_set_data_source(self, setup->data_source, setup->data_source_context);
    egui_view_virtual_tree_set_state_cache_limits(self, setup->state_cache_max_entries, setup->state_cache_max_bytes);
}

void egui_view_virtual_tree_init_with_setup(egui_view_t *self, const egui_view_virtual_tree_setup_t *setup)
{
    egui_view_virtual_tree_init(self);
    egui_view_virtual_tree_apply_setup(self, setup);
}

void egui_view_virtual_tree_set_data_source(egui_view_t *self, const egui_view_virtual_tree_data_source_t *data_source, void *data_source_context)
{
    EGUI_LOCAL_INIT(egui_view_virtual_tree_t);

    local->data_source = data_source;
    local->data_source_context = data_source_context;

    if (data_source == NULL)
    {
        memset(&local->flat_data_source, 0, sizeof(local->flat_data_source));
        egui_view_virtual_list_set_data_source(self, NULL, NULL);
        return;
    }

    local->flat_data_source.get_count = egui_view_virtual_tree_flat_get_count;
    local->flat_data_source.get_stable_id = egui_view_virtual_tree_flat_get_stable_id;
    local->flat_data_source.find_index_by_stable_id = egui_view_virtual_tree_flat_find_index_by_stable_id;
    local->flat_data_source.get_view_type = egui_view_virtual_tree_flat_get_view_type;
    local->flat_data_source.measure_item_height = egui_view_virtual_tree_flat_measure_item_height;
    local->flat_data_source.create_item_view = egui_view_virtual_tree_flat_create_view;
    local->flat_data_source.destroy_item_view = egui_view_virtual_tree_flat_destroy_view;
    local->flat_data_source.bind_item_view = egui_view_virtual_tree_flat_bind_view;
    local->flat_data_source.unbind_item_view = egui_view_virtual_tree_flat_unbind_view;
    local->flat_data_source.should_keep_alive = egui_view_virtual_tree_flat_should_keep_alive;
    local->flat_data_source.save_item_state = egui_view_virtual_tree_flat_save_state;
    local->flat_data_source.restore_item_state = egui_view_virtual_tree_flat_restore_state;
    local->flat_data_source.default_view_type = data_source->default_view_type;

    egui_view_virtual_list_set_data_source(self, &local->flat_data_source, local);
}

const egui_view_virtual_tree_data_source_t *egui_view_virtual_tree_get_data_source(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_tree_t);
    return local->data_source;
}

void *egui_view_virtual_tree_get_data_source_context(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_tree_t);
    return local->data_source_context;
}

uint32_t egui_view_virtual_tree_get_root_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_tree_t);
    return egui_view_virtual_tree_get_root_count_internal(local);
}

uint32_t egui_view_virtual_tree_get_visible_node_count(egui_view_t *self)
{
    return egui_view_virtual_list_get_item_count(self);
}

void egui_view_virtual_tree_set_overscan(egui_view_t *self, uint8_t before, uint8_t after)
{
    egui_view_virtual_list_set_overscan(self, before, after);
}

uint8_t egui_view_virtual_tree_get_overscan_before(egui_view_t *self)
{
    return egui_view_virtual_list_get_overscan_before(self);
}

uint8_t egui_view_virtual_tree_get_overscan_after(egui_view_t *self)
{
    return egui_view_virtual_list_get_overscan_after(self);
}

void egui_view_virtual_tree_set_estimated_node_height(egui_view_t *self, int32_t height)
{
    egui_view_virtual_list_set_estimated_item_height(self, height);
}

int32_t egui_view_virtual_tree_get_estimated_node_height(egui_view_t *self)
{
    return egui_view_virtual_list_get_estimated_item_height(self);
}

void egui_view_virtual_tree_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots)
{
    egui_view_virtual_list_set_keepalive_limit(self, max_keepalive_slots);
}

uint8_t egui_view_virtual_tree_get_keepalive_limit(egui_view_t *self)
{
    return egui_view_virtual_list_get_keepalive_limit(self);
}

void egui_view_virtual_tree_set_state_cache_limits(egui_view_t *self, uint16_t max_entries, uint32_t max_bytes)
{
    egui_view_virtual_list_set_state_cache_limits(self, max_entries, max_bytes);
}

void egui_view_virtual_tree_clear_node_state_cache(egui_view_t *self)
{
    egui_view_virtual_list_clear_item_state_cache(self);
}

void egui_view_virtual_tree_remove_node_state_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    egui_view_virtual_list_remove_item_state_by_stable_id(self, stable_id);
}

uint8_t egui_view_virtual_tree_write_node_state(egui_view_t *self, uint32_t stable_id, const void *data, uint16_t size)
{
    return egui_view_virtual_list_write_item_state(self, stable_id, data, size);
}

uint16_t egui_view_virtual_tree_read_node_state(egui_view_t *self, uint32_t stable_id, void *data, uint16_t capacity)
{
    return egui_view_virtual_list_read_item_state(self, stable_id, data, capacity);
}

uint8_t egui_view_virtual_tree_write_node_state_for_view(egui_view_t *node_view, uint32_t stable_id, const void *data, uint16_t size)
{
    return egui_view_virtual_list_write_item_state_for_view(node_view, stable_id, data, size);
}

uint16_t egui_view_virtual_tree_read_node_state_for_view(egui_view_t *node_view, uint32_t stable_id, void *data, uint16_t capacity)
{
    return egui_view_virtual_list_read_item_state_for_view(node_view, stable_id, data, capacity);
}

void egui_view_virtual_tree_set_scroll_y(egui_view_t *self, int32_t offset)
{
    egui_view_virtual_list_set_scroll_y(self, offset);
}

void egui_view_virtual_tree_scroll_by(egui_view_t *self, int32_t delta)
{
    egui_view_virtual_list_scroll_by(self, delta);
}

void egui_view_virtual_tree_scroll_to_visible_node(egui_view_t *self, uint32_t visible_index, int32_t node_offset)
{
    egui_view_virtual_list_scroll_to_item(self, visible_index, node_offset);
}

void egui_view_virtual_tree_scroll_to_node_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t node_offset)
{
    egui_view_virtual_list_scroll_to_stable_id(self, stable_id, node_offset);
}

int32_t egui_view_virtual_tree_get_scroll_y(egui_view_t *self)
{
    return egui_view_virtual_list_get_scroll_y(self);
}

int32_t egui_view_virtual_tree_find_visible_index_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    return egui_view_virtual_list_find_index_by_stable_id(self, stable_id);
}

uint8_t egui_view_virtual_tree_resolve_node_by_stable_id(egui_view_t *self, uint32_t stable_id, egui_view_virtual_tree_entry_t *entry)
{
    EGUI_LOCAL_INIT(egui_view_virtual_tree_t);
    return egui_view_virtual_tree_resolve_node_internal(local, stable_id, entry);
}

uint8_t egui_view_virtual_tree_resolve_node_by_view(egui_view_t *self, egui_view_t *node_view, egui_view_virtual_tree_entry_t *entry)
{
    EGUI_LOCAL_INIT(egui_view_virtual_tree_t);
    const egui_view_virtual_tree_slot_t *slot = egui_view_virtual_tree_find_slot_by_view(local, node_view);

    return egui_view_virtual_tree_resolve_slot_internal(local, slot, entry);
}

int32_t egui_view_virtual_tree_get_visible_node_y(egui_view_t *self, uint32_t visible_index)
{
    return egui_view_virtual_list_get_item_y(self, visible_index);
}

int32_t egui_view_virtual_tree_get_visible_node_height(egui_view_t *self, uint32_t visible_index)
{
    return egui_view_virtual_list_get_item_height(self, visible_index);
}

int32_t egui_view_virtual_tree_get_node_y_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t index = egui_view_virtual_tree_find_visible_index_by_stable_id(self, stable_id);

    if (index < 0)
    {
        return -1;
    }

    return egui_view_virtual_tree_get_visible_node_y(self, (uint32_t)index);
}

int32_t egui_view_virtual_tree_get_node_height_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t index = egui_view_virtual_tree_find_visible_index_by_stable_id(self, stable_id);

    if (index < 0)
    {
        return -1;
    }

    return egui_view_virtual_tree_get_visible_node_height(self, (uint32_t)index);
}

uint8_t egui_view_virtual_tree_ensure_node_visible_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t inset)
{
    if (stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID || egui_view_virtual_tree_find_visible_index_by_stable_id(self, stable_id) < 0)
    {
        return 0;
    }

    return egui_view_virtual_list_ensure_item_visible_by_stable_id(self, stable_id, inset);
}

void egui_view_virtual_tree_notify_data_changed(egui_view_t *self)
{
    egui_view_virtual_list_notify_data_changed(self);
}

void egui_view_virtual_tree_notify_visible_node_changed(egui_view_t *self, uint32_t visible_index)
{
    egui_view_virtual_list_notify_item_changed(self, visible_index);
}

void egui_view_virtual_tree_notify_node_changed_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    egui_view_virtual_list_notify_item_changed_by_stable_id(self, stable_id);
}

void egui_view_virtual_tree_notify_visible_node_resized(egui_view_t *self, uint32_t visible_index)
{
    egui_view_virtual_list_notify_item_resized(self, visible_index);
}

void egui_view_virtual_tree_notify_node_resized_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    egui_view_virtual_list_notify_item_resized_by_stable_id(self, stable_id);
}

uint8_t egui_view_virtual_tree_get_slot_count(egui_view_t *self)
{
    return egui_view_virtual_list_get_slot_count(self);
}

const egui_view_virtual_tree_slot_t *egui_view_virtual_tree_get_slot(egui_view_t *self, uint8_t slot_index)
{
    return egui_view_virtual_list_get_slot(self, slot_index);
}

int32_t egui_view_virtual_tree_find_slot_index_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    EGUI_LOCAL_INIT(egui_view_virtual_tree_t);
    return egui_view_virtual_tree_find_slot_index_by_stable_id_internal(local, stable_id);
}

const egui_view_virtual_tree_slot_t *egui_view_virtual_tree_find_slot_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t slot_index = egui_view_virtual_tree_find_slot_index_by_stable_id(self, stable_id);

    return slot_index >= 0 ? egui_view_virtual_tree_get_slot(self, (uint8_t)slot_index) : NULL;
}

egui_view_t *egui_view_virtual_tree_find_view_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    const egui_view_virtual_tree_slot_t *slot = egui_view_virtual_tree_find_slot_by_stable_id(self, stable_id);

    return slot != NULL ? slot->view : NULL;
}

uint8_t egui_view_virtual_tree_get_slot_entry(egui_view_t *self, uint8_t slot_index, egui_view_virtual_tree_entry_t *entry)
{
    EGUI_LOCAL_INIT(egui_view_virtual_tree_t);
    const egui_view_virtual_tree_slot_t *slot = egui_view_virtual_tree_get_slot(self, slot_index);

    return egui_view_virtual_tree_resolve_slot_internal(local, slot, entry);
}

uint8_t egui_view_virtual_tree_visit_visible_nodes(egui_view_t *self, egui_view_virtual_tree_visible_node_visitor_t visitor, void *context)
{
    uint8_t slot_count;
    uint8_t slot_index;
    uint8_t visited = 0;

    if (visitor == NULL)
    {
        return 0;
    }

    slot_count = egui_view_virtual_tree_get_slot_count(self);
    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        egui_view_virtual_tree_entry_t entry;
        const egui_view_virtual_tree_slot_t *slot = egui_view_virtual_tree_get_slot(self, slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->view == NULL)
        {
            continue;
        }
        if (!egui_view_virtual_tree_get_slot_entry(self, slot_index, &entry))
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

typedef struct egui_view_virtual_tree_find_visible_node_context
{
    egui_view_virtual_tree_visible_node_matcher_t matcher;
    void *context;
    uint32_t best_visible_index;
    egui_view_t *best_view;
    egui_view_virtual_tree_entry_t best_entry;
} egui_view_virtual_tree_find_visible_node_context_t;

static uint8_t egui_view_virtual_tree_find_visible_node_visitor(egui_view_t *self, const egui_view_virtual_tree_slot_t *slot,
                                                                const egui_view_virtual_tree_entry_t *entry, egui_view_t *node_view, void *context)
{
    egui_view_virtual_tree_find_visible_node_context_t *ctx = (egui_view_virtual_tree_find_visible_node_context_t *)context;

    if (entry == NULL || node_view == NULL)
    {
        return 1;
    }
    if (ctx->matcher != NULL && !ctx->matcher(self, slot, entry, node_view, ctx->context))
    {
        return 1;
    }
    if (ctx->best_view == NULL || entry->visible_index < ctx->best_visible_index)
    {
        ctx->best_visible_index = entry->visible_index;
        ctx->best_entry = *entry;
        ctx->best_view = node_view;
    }

    return 1;
}

egui_view_t *egui_view_virtual_tree_find_first_visible_node_view(egui_view_t *self, egui_view_virtual_tree_visible_node_matcher_t matcher,
                                                                 void *context, egui_view_virtual_tree_entry_t *entry_out)
{
    egui_view_virtual_tree_find_visible_node_context_t find_ctx = {
            .matcher = matcher,
            .context = context,
            .best_visible_index = EGUI_VIEW_VIRTUAL_TREE_INVALID_INDEX,
            .best_view = NULL,
    };

    egui_view_virtual_tree_visit_visible_nodes(self, egui_view_virtual_tree_find_visible_node_visitor, &find_ctx);
    if (find_ctx.best_view != NULL && entry_out != NULL)
    {
        *entry_out = find_ctx.best_entry;
    }

    return find_ctx.best_view;
}

void egui_view_virtual_tree_init(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_tree_t);

    egui_view_virtual_list_init(self);
    local->data_source = NULL;
    local->data_source_context = NULL;
    memset(&local->flat_data_source, 0, sizeof(local->flat_data_source));
    egui_view_set_view_name(self, "egui_view_virtual_tree");
}
