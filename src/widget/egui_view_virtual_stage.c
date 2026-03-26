#include <string.h>

#include "egui_view_virtual_stage.h"
#include "core/egui_common.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
#include "core/egui_focus.h"
#endif

#define EGUI_VIEW_VIRTUAL_STAGE_INVALID_SLOT 0xFF

typedef struct egui_view_virtual_stage_cached_node egui_view_virtual_stage_cached_node_t;
typedef struct egui_view_virtual_stage_pin_entry egui_view_virtual_stage_pin_entry_t;

struct egui_view_virtual_stage_cached_node
{
    uint32_t index;
    egui_virtual_stage_node_desc_t desc;
};

struct egui_view_virtual_stage_pin_entry
{
    uint32_t stable_id;
    uint16_t ref_count;
};

static egui_view_virtual_stage_array_adapter_t *egui_view_virtual_stage_get_array_adapter(void *adapter_context)
{
    return (egui_view_virtual_stage_array_adapter_t *)adapter_context;
}

static uint32_t egui_view_virtual_stage_array_adapter_get_count(void *adapter_context)
{
    egui_view_virtual_stage_array_adapter_t *adapter = egui_view_virtual_stage_get_array_adapter(adapter_context);

    if (adapter == NULL || adapter->node_source == NULL)
    {
        return 0;
    }

    return adapter->node_source->count;
}

static uint8_t egui_view_virtual_stage_array_adapter_get_desc(void *adapter_context, uint32_t index, egui_virtual_stage_node_desc_t *out_desc)
{
    egui_view_virtual_stage_array_adapter_t *adapter = egui_view_virtual_stage_get_array_adapter(adapter_context);
    const egui_view_virtual_stage_array_source_t *node_source;
    const uint8_t *item_ptr;

    if (adapter == NULL || out_desc == NULL)
    {
        return 0;
    }

    node_source = adapter->node_source;
    if (node_source == NULL || node_source->items == NULL || index >= node_source->count || node_source->item_size == 0)
    {
        return 0;
    }

    item_ptr = (const uint8_t *)node_source->items + index * node_source->item_size + node_source->desc_offset;
    memcpy(out_desc, item_ptr, sizeof(*out_desc));
    return 1;
}

static egui_view_t *egui_view_virtual_stage_array_adapter_create_view(void *adapter_context, uint16_t view_type)
{
    egui_view_virtual_stage_array_adapter_t *adapter = egui_view_virtual_stage_get_array_adapter(adapter_context);

    if (adapter == NULL || adapter->ops == NULL || adapter->ops->create_view == NULL)
    {
        return NULL;
    }

    return adapter->ops->create_view(adapter->user_context, view_type);
}

static void egui_view_virtual_stage_array_adapter_destroy_view(void *adapter_context, egui_view_t *view, uint16_t view_type)
{
    egui_view_virtual_stage_array_adapter_t *adapter = egui_view_virtual_stage_get_array_adapter(adapter_context);

    if (adapter == NULL || adapter->ops == NULL || adapter->ops->destroy_view == NULL)
    {
        return;
    }

    adapter->ops->destroy_view(adapter->user_context, view, view_type);
}

static void egui_view_virtual_stage_array_adapter_bind_view(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id,
                                                            const egui_virtual_stage_node_desc_t *desc)
{
    egui_view_virtual_stage_array_adapter_t *adapter = egui_view_virtual_stage_get_array_adapter(adapter_context);

    if (adapter == NULL || adapter->ops == NULL || adapter->ops->bind_view == NULL)
    {
        return;
    }

    adapter->ops->bind_view(adapter->user_context, view, index, stable_id, desc);
}

static void egui_view_virtual_stage_array_adapter_unbind_view(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id,
                                                              const egui_virtual_stage_node_desc_t *desc)
{
    egui_view_virtual_stage_array_adapter_t *adapter = egui_view_virtual_stage_get_array_adapter(adapter_context);

    if (adapter == NULL || adapter->ops == NULL || adapter->ops->unbind_view == NULL)
    {
        return;
    }

    adapter->ops->unbind_view(adapter->user_context, view, index, stable_id, desc);
}

static void egui_view_virtual_stage_array_adapter_save_state(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id,
                                                             const egui_virtual_stage_node_desc_t *desc)
{
    egui_view_virtual_stage_array_adapter_t *adapter = egui_view_virtual_stage_get_array_adapter(adapter_context);

    if (adapter == NULL || adapter->ops == NULL || adapter->ops->save_state == NULL)
    {
        return;
    }

    adapter->ops->save_state(adapter->user_context, view, index, stable_id, desc);
}

static void egui_view_virtual_stage_array_adapter_restore_state(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id,
                                                                const egui_virtual_stage_node_desc_t *desc)
{
    egui_view_virtual_stage_array_adapter_t *adapter = egui_view_virtual_stage_get_array_adapter(adapter_context);

    if (adapter == NULL || adapter->ops == NULL || adapter->ops->restore_state == NULL)
    {
        return;
    }

    adapter->ops->restore_state(adapter->user_context, view, index, stable_id, desc);
}

static void egui_view_virtual_stage_array_adapter_draw_node(void *adapter_context, egui_view_t *page, uint32_t index,
                                                            const egui_virtual_stage_node_desc_t *desc, const egui_region_t *screen_region)
{
    egui_view_virtual_stage_array_adapter_t *adapter = egui_view_virtual_stage_get_array_adapter(adapter_context);

    if (adapter == NULL || adapter->ops == NULL || adapter->ops->draw_node == NULL)
    {
        return;
    }

    adapter->ops->draw_node(adapter->user_context, page, index, desc, screen_region);
}

static uint8_t egui_view_virtual_stage_array_adapter_hit_test(void *adapter_context, uint32_t index, const egui_virtual_stage_node_desc_t *desc,
                                                              const egui_region_t *screen_region, egui_dim_t screen_x, egui_dim_t screen_y)
{
    egui_view_virtual_stage_array_adapter_t *adapter = egui_view_virtual_stage_get_array_adapter(adapter_context);

    if (adapter == NULL || adapter->ops == NULL || adapter->ops->hit_test == NULL)
    {
        return screen_region != NULL ? (egui_region_pt_in_rect(screen_region, screen_x, screen_y) ? 1U : 0U) : 0U;
    }

    return adapter->ops->hit_test(adapter->user_context, index, desc, screen_region, screen_x, screen_y);
}

static uint8_t egui_view_virtual_stage_array_adapter_should_keep_alive(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id,
                                                                       const egui_virtual_stage_node_desc_t *desc)
{
    egui_view_virtual_stage_array_adapter_t *adapter = egui_view_virtual_stage_get_array_adapter(adapter_context);

    if (adapter == NULL || adapter->ops == NULL || adapter->ops->should_keep_alive == NULL)
    {
        return 0;
    }

    return adapter->ops->should_keep_alive(adapter->user_context, view, index, stable_id, desc);
}

static egui_view_virtual_stage_cached_node_t *egui_view_virtual_stage_find_node_by_stable_id(egui_view_virtual_stage_t *local, uint32_t stable_id);
static uint8_t egui_view_virtual_stage_resolve_node_desc_by_stable_id(egui_view_virtual_stage_t *local, uint32_t stable_id,
                                                                      egui_virtual_stage_node_desc_t *out_desc);

static egui_view_virtual_stage_cached_node_t *egui_view_virtual_stage_get_nodes(egui_view_virtual_stage_t *local)
{
    return (egui_view_virtual_stage_cached_node_t *)local->node_cache;
}

static egui_view_virtual_stage_pin_entry_t *egui_view_virtual_stage_get_pins(egui_view_virtual_stage_t *local)
{
    return (egui_view_virtual_stage_pin_entry_t *)local->pin_entries;
}

static void egui_view_virtual_stage_reset_slot(egui_view_virtual_stage_slot_t *slot)
{
    slot->state = EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_UNUSED;
    slot->view_type = EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE;
    slot->index = 0;
    slot->stable_id = EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID;
    slot->view = NULL;
    memset(&slot->render_region, 0, sizeof(slot->render_region));
    memset(&slot->bound_desc, 0, sizeof(slot->bound_desc));
    slot->last_used_seq = 0;
    slot->needs_bind = 0;
    slot->needs_layout = 0;
    memset(&slot->touch_state, 0, sizeof(slot->touch_state));
}

static void egui_view_virtual_stage_reset_slots(egui_view_virtual_stage_t *local)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS; i++)
    {
        egui_view_virtual_stage_reset_slot(&local->slots[i]);
    }

    local->slot_count = 0;
    local->captured_slot = EGUI_VIEW_VIRTUAL_STAGE_INVALID_SLOT;
}

static void egui_view_virtual_stage_compact_slot_count(egui_view_virtual_stage_t *local)
{
    uint8_t i;
    uint8_t count = 0;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS; i++)
    {
        if (local->slots[i].state != EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_UNUSED)
        {
            count++;
        }
    }

    local->slot_count = count;
}

static void egui_view_virtual_stage_release_cache(egui_view_virtual_stage_t *local)
{
    if (local->node_cache != NULL)
    {
        egui_free(local->node_cache);
        local->node_cache = NULL;
    }

    local->cached_node_count = 0;
}

static void egui_view_virtual_stage_release_pins(egui_view_virtual_stage_t *local)
{
    if (local->pin_entries != NULL)
    {
        egui_free(local->pin_entries);
        local->pin_entries = NULL;
    }

    local->pin_count = 0;
    local->pin_capacity = 0;
}

static void egui_view_virtual_stage_prune_pins_against_cache(egui_view_virtual_stage_t *local)
{
    egui_view_virtual_stage_pin_entry_t *pins = egui_view_virtual_stage_get_pins(local);
    uint16_t read_index;
    uint16_t write_index = 0;

    if (pins == NULL || local->pin_count == 0)
    {
        return;
    }

    for (read_index = 0; read_index < local->pin_count; read_index++)
    {
        if (egui_view_virtual_stage_find_node_by_stable_id(local, pins[read_index].stable_id) == NULL)
        {
            continue;
        }

        if (write_index != read_index)
        {
            pins[write_index] = pins[read_index];
        }
        write_index++;
    }

    local->pin_count = write_index;
}

static uint32_t egui_view_virtual_stage_get_node_count(egui_view_virtual_stage_t *local)
{
    if (local->adapter == NULL || local->adapter->get_count == NULL)
    {
        return 0;
    }

    return local->adapter->get_count(local->adapter_context);
}

static void egui_view_virtual_stage_sort_nodes(egui_view_virtual_stage_t *local)
{
    egui_view_virtual_stage_cached_node_t *nodes = egui_view_virtual_stage_get_nodes(local);
    uint32_t i;

    if (nodes == NULL)
    {
        return;
    }

    for (i = 1; i < local->cached_node_count; i++)
    {
        egui_view_virtual_stage_cached_node_t key = nodes[i];
        uint32_t j = i;

        while (j > 0)
        {
            egui_view_virtual_stage_cached_node_t *lhs = &nodes[j - 1];
            egui_view_virtual_stage_cached_node_t *rhs = &key;

            if (lhs->desc.z_order < rhs->desc.z_order)
            {
                break;
            }
            if (lhs->desc.z_order == rhs->desc.z_order && lhs->index <= rhs->index)
            {
                break;
            }

            nodes[j] = nodes[j - 1];
            j--;
        }

        nodes[j] = key;
    }
}

static uint8_t egui_view_virtual_stage_reload_cache(egui_view_virtual_stage_t *local)
{
    uint32_t count = egui_view_virtual_stage_get_node_count(local);
    egui_view_virtual_stage_cached_node_t *new_nodes = NULL;
    uint32_t write_count = 0;
    uint32_t index;

    if (count == 0 || local->adapter == NULL || local->adapter->get_desc == NULL)
    {
        egui_view_virtual_stage_release_cache(local);
        return 1;
    }

    new_nodes = (egui_view_virtual_stage_cached_node_t *)egui_malloc((int)(sizeof(egui_view_virtual_stage_cached_node_t) * count));
    if (new_nodes == NULL)
    {
        return 0;
    }

    for (index = 0; index < count; index++)
    {
        egui_virtual_stage_node_desc_t desc;
        memset(&desc, 0, sizeof(desc));
        if (!local->adapter->get_desc(local->adapter_context, index, &desc))
        {
            continue;
        }
        if (desc.stable_id == EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID)
        {
            desc.stable_id = index;
        }
        new_nodes[write_count].index = index;
        new_nodes[write_count].desc = desc;
        write_count++;
    }

    egui_view_virtual_stage_release_cache(local);

    if (write_count == 0)
    {
        egui_free(new_nodes);
        return 1;
    }

    local->node_cache = new_nodes;
    local->cached_node_count = write_count;
    egui_view_virtual_stage_sort_nodes(local);
    egui_view_virtual_stage_prune_pins_against_cache(local);
    return 1;
}

static egui_view_virtual_stage_cached_node_t *egui_view_virtual_stage_find_node_by_stable_id(egui_view_virtual_stage_t *local, uint32_t stable_id)
{
    egui_view_virtual_stage_cached_node_t *nodes = egui_view_virtual_stage_get_nodes(local);
    uint32_t i;

    if (nodes == NULL || stable_id == EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID)
    {
        return NULL;
    }

    for (i = 0; i < local->cached_node_count; i++)
    {
        if (nodes[i].desc.stable_id == stable_id)
        {
            return &nodes[i];
        }
    }

    return NULL;
}

static uint8_t egui_view_virtual_stage_resolve_node_desc_by_stable_id(egui_view_virtual_stage_t *local, uint32_t stable_id,
                                                                      egui_virtual_stage_node_desc_t *out_desc)
{
    egui_view_virtual_stage_cached_node_t *node;

    if (stable_id == EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID)
    {
        return 0;
    }

    if (!local->is_data_dirty)
    {
        node = egui_view_virtual_stage_find_node_by_stable_id(local, stable_id);
        if (node != NULL)
        {
            if (out_desc != NULL)
            {
                *out_desc = node->desc;
            }
            return 1;
        }
    }

    if (local->adapter != NULL && local->adapter->get_count != NULL && local->adapter->get_desc != NULL)
    {
        uint32_t index;
        uint32_t count = local->adapter->get_count(local->adapter_context);

        for (index = 0; index < count; index++)
        {
            egui_virtual_stage_node_desc_t desc;
            memset(&desc, 0, sizeof(desc));
            if (!local->adapter->get_desc(local->adapter_context, index, &desc))
            {
                continue;
            }
            if (desc.stable_id == EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID)
            {
                desc.stable_id = index;
            }
            if (desc.stable_id != stable_id)
            {
                continue;
            }

            if (out_desc != NULL)
            {
                *out_desc = desc;
            }
            return 1;
        }
    }

    return 0;
}

static void egui_view_virtual_stage_get_screen_region(egui_view_t *self, const egui_virtual_stage_node_desc_t *desc, egui_region_t *screen_region)
{
    screen_region->location.x = self->region_screen.location.x + desc->region.location.x;
    screen_region->location.y = self->region_screen.location.y + desc->region.location.y;
    screen_region->size.width = desc->region.size.width;
    screen_region->size.height = desc->region.size.height;
}

static uint8_t egui_view_virtual_stage_is_node_hidden(const egui_virtual_stage_node_desc_t *desc)
{
    if ((desc->flags & EGUI_VIRTUAL_STAGE_NODE_FLAG_HIDDEN) != 0)
    {
        return 1;
    }

    return desc->region.size.width <= 0 || desc->region.size.height <= 0;
}

static void egui_view_virtual_stage_invalidate_node_region(egui_view_t *self, const egui_virtual_stage_node_desc_t *desc)
{
    if (desc == NULL || egui_view_virtual_stage_is_node_hidden(desc))
    {
        return;
    }

    egui_view_invalidate_region(self, &desc->region);
}

static void egui_view_virtual_stage_mark_dirty(egui_view_t *self, egui_view_virtual_stage_t *local, uint8_t data_dirty, uint8_t layout_dirty)
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

static void egui_view_virtual_stage_sync_content_layer(egui_view_t *self, egui_view_virtual_stage_t *local)
{
    egui_view_t *content_layer = EGUI_VIEW_OF(&local->content_layer);

    if (content_layer->region.location.x != 0 || content_layer->region.location.y != 0)
    {
        egui_view_set_position(content_layer, 0, 0);
    }

    if (content_layer->region.size.width != self->region.size.width || content_layer->region.size.height != self->region.size.height)
    {
        egui_view_set_size(content_layer, self->region.size.width, self->region.size.height);
    }
}

static int egui_view_virtual_stage_find_pin_entry(egui_view_virtual_stage_t *local, uint32_t stable_id)
{
    egui_view_virtual_stage_pin_entry_t *pins = egui_view_virtual_stage_get_pins(local);
    uint16_t i;

    if (pins == NULL)
    {
        return -1;
    }

    for (i = 0; i < local->pin_count; i++)
    {
        if (pins[i].stable_id == stable_id)
        {
            return (int)i;
        }
    }

    return -1;
}

static uint8_t egui_view_virtual_stage_is_pinned(egui_view_virtual_stage_t *local, uint32_t stable_id)
{
    return egui_view_virtual_stage_find_pin_entry(local, stable_id) >= 0;
}

static uint8_t egui_view_virtual_stage_ensure_pin_capacity(egui_view_virtual_stage_t *local, uint16_t target_count)
{
    egui_view_virtual_stage_pin_entry_t *pins;
    uint16_t new_capacity;

    if (target_count <= local->pin_capacity)
    {
        return 1;
    }

    new_capacity = local->pin_capacity == 0 ? 4 : (uint16_t)(local->pin_capacity * 2);
    if (new_capacity < target_count)
    {
        new_capacity = target_count;
    }

    pins = (egui_view_virtual_stage_pin_entry_t *)egui_malloc((int)(sizeof(egui_view_virtual_stage_pin_entry_t) * new_capacity));
    if (pins == NULL)
    {
        return 0;
    }

    if (local->pin_entries != NULL && local->pin_count > 0)
    {
        memcpy(pins, local->pin_entries, sizeof(egui_view_virtual_stage_pin_entry_t) * local->pin_count);
        egui_free(local->pin_entries);
    }

    local->pin_entries = pins;
    local->pin_capacity = new_capacity;
    return 1;
}

static void egui_view_virtual_stage_remove_pin_entry(egui_view_virtual_stage_t *local, uint16_t pin_index)
{
    egui_view_virtual_stage_pin_entry_t *pins = egui_view_virtual_stage_get_pins(local);

    if (pins == NULL || pin_index >= local->pin_count)
    {
        return;
    }

    if (pin_index + 1 < local->pin_count)
    {
        memmove(&pins[pin_index], &pins[pin_index + 1], sizeof(egui_view_virtual_stage_pin_entry_t) * (local->pin_count - pin_index - 1));
    }
    local->pin_count--;
}

static void egui_view_virtual_stage_hide_slot_view(egui_view_t *self, egui_view_virtual_stage_slot_t *slot)
{
    egui_dim_t hidden_x = (egui_dim_t)(-(self->region.size.width + 8));
    egui_dim_t hidden_y = (egui_dim_t)(-(self->region.size.height + 8));

    if (slot->view == NULL)
    {
        return;
    }

    egui_view_set_gone(slot->view, 1);
    egui_view_set_position(slot->view, hidden_x, hidden_y);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static void egui_view_virtual_stage_clear_focus_if_needed(egui_view_t *view)
{
    if (view != NULL && view->is_focused)
    {
        egui_focus_manager_clear_focus();
    }
}
#else
static void egui_view_virtual_stage_clear_focus_if_needed(egui_view_t *view)
{
    EGUI_UNUSED(view);
}
#endif

static void egui_view_virtual_stage_cancel_slot_capture(egui_view_virtual_stage_t *local, egui_view_virtual_stage_slot_t *slot)
{
    egui_motion_event_t cancel_event;
    uint8_t slot_index;

    if (slot == NULL || slot->view == NULL || local->captured_slot == EGUI_VIEW_VIRTUAL_STAGE_INVALID_SLOT)
    {
        return;
    }

    slot_index = (uint8_t)(slot - local->slots);
    if (slot_index >= EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS || local->captured_slot != slot_index)
    {
        return;
    }

    memset(&cancel_event, 0, sizeof(cancel_event));
    cancel_event.type = EGUI_MOTION_EVENT_ACTION_CANCEL;
    (void)slot->view->api->dispatch_touch_event(slot->view, &cancel_event);
    local->captured_slot = EGUI_VIEW_VIRTUAL_STAGE_INVALID_SLOT;
}

static void egui_view_virtual_stage_save_and_unbind(egui_view_virtual_stage_t *local, egui_view_virtual_stage_slot_t *slot,
                                                    const egui_view_virtual_stage_cached_node_t *node)
{
    uint32_t index = node != NULL ? node->index : slot->index;
    const egui_virtual_stage_node_desc_t *desc = NULL;

    if (slot->bound_desc.stable_id == slot->stable_id && slot->stable_id != EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID)
    {
        desc = &slot->bound_desc;
    }
    else if (node != NULL)
    {
        desc = &node->desc;
    }

    if (slot->view != NULL && slot->stable_id != EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID && local->adapter != NULL)
    {
        if (local->adapter->save_state != NULL)
        {
            local->adapter->save_state(local->adapter_context, slot->view, index, slot->stable_id, desc);
        }
        if (local->adapter->unbind_view != NULL)
        {
            local->adapter->unbind_view(local->adapter_context, slot->view, index, slot->stable_id, desc);
        }
    }

    slot->index = 0;
    slot->stable_id = EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID;
    memset(&slot->render_region, 0, sizeof(slot->render_region));
    memset(&slot->bound_desc, 0, sizeof(slot->bound_desc));
}

static void egui_view_virtual_stage_destroy_slot_view(egui_view_t *self, egui_view_virtual_stage_t *local, egui_view_virtual_stage_slot_t *slot,
                                                      const egui_view_virtual_stage_cached_node_t *node)
{
    if (slot->view == NULL)
    {
        egui_view_virtual_stage_reset_slot(slot);
        return;
    }

    egui_view_virtual_stage_cancel_slot_capture(local, slot);

    if ((slot->state == EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_ACTIVE || slot->state == EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_KEEPALIVE) &&
        slot->stable_id != EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID)
    {
        egui_view_virtual_stage_save_and_unbind(local, slot, node);
    }

    egui_view_virtual_stage_clear_focus_if_needed(slot->view);

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

    egui_view_virtual_stage_reset_slot(slot);
    EGUI_UNUSED(self);
}

static void egui_view_virtual_stage_release_slot(egui_view_t *self, egui_view_virtual_stage_t *local, egui_view_virtual_stage_slot_t *slot,
                                                 const egui_view_virtual_stage_cached_node_t *node)
{
    if (slot->state == EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_UNUSED)
    {
        return;
    }

    egui_view_virtual_stage_cancel_slot_capture(local, slot);

    if (local->adapter != NULL && local->adapter->destroy_view != NULL)
    {
        egui_view_virtual_stage_destroy_slot_view(self, local, slot, node);
        return;
    }

    if ((slot->state == EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_ACTIVE || slot->state == EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_KEEPALIVE) &&
        slot->stable_id != EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID)
    {
        egui_view_virtual_stage_save_and_unbind(local, slot, node);
    }

    egui_view_virtual_stage_clear_focus_if_needed(slot->view);

    if (slot->view != NULL)
    {
        slot->state = EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_POOLED;
        slot->last_used_seq = 0;
        egui_view_virtual_stage_hide_slot_view(self, slot);
    }
    else
    {
        egui_view_virtual_stage_reset_slot(slot);
    }
}

static void egui_view_virtual_stage_destroy_all_slots(egui_view_t *self, egui_view_virtual_stage_t *local)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS; i++)
    {
        egui_view_virtual_stage_destroy_slot_view(self, local, &local->slots[i], NULL);
    }

    local->captured_slot = EGUI_VIEW_VIRTUAL_STAGE_INVALID_SLOT;
    local->slot_count = 0;
}

static void egui_view_virtual_stage_apply_render_region(egui_view_virtual_stage_slot_t *slot, const egui_view_virtual_stage_cached_node_t *node)
{
    egui_view_layout(slot->view, (egui_region_t *)&node->desc.region);
    slot->render_region = node->desc.region;
    egui_view_set_gone(slot->view, 0);
}

static uint8_t egui_view_virtual_stage_prepare_slot_view(egui_view_t *self, egui_view_virtual_stage_t *local, egui_view_virtual_stage_slot_t *slot,
                                                         uint16_t view_type)
{
    egui_view_t *view;

    if (view_type == EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE)
    {
        return 0;
    }

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
        egui_view_virtual_stage_destroy_slot_view(self, local, slot, NULL);
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
    slot->state = EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_POOLED;

    egui_view_group_add_child(EGUI_VIEW_OF(&local->content_layer), view);
    if (local->is_attached_to_window)
    {
        view->api->on_attach_to_window(view);
    }

    egui_view_virtual_stage_hide_slot_view(self, slot);
    return 1;
}

static uint8_t egui_view_virtual_stage_slot_is_bound(const egui_view_virtual_stage_slot_t *slot)
{
    return slot->state == EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_ACTIVE || slot->state == EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_KEEPALIVE;
}

static int egui_view_virtual_stage_find_slot_index_by_stable_id(egui_view_virtual_stage_t *local, uint32_t stable_id)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS; i++)
    {
        if (egui_view_virtual_stage_slot_is_bound(&local->slots[i]) && local->slots[i].stable_id == stable_id)
        {
            return (int)i;
        }
    }

    return -1;
}

static int egui_view_virtual_stage_find_slot_by_state_and_type(egui_view_virtual_stage_t *local, uint8_t state, uint16_t view_type)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS; i++)
    {
        if (local->slots[i].state == state && local->slots[i].view_type == view_type)
        {
            return (int)i;
        }
    }

    return -1;
}

static int egui_view_virtual_stage_find_unused_slot(egui_view_virtual_stage_t *local)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS; i++)
    {
        if (local->slots[i].view == NULL && local->slots[i].state != EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_UNUSED)
        {
            egui_view_virtual_stage_reset_slot(&local->slots[i]);
        }
        if (local->slots[i].state == EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_UNUSED)
        {
            return (int)i;
        }
    }

    return -1;
}

static uint8_t egui_view_virtual_stage_slot_is_protected(const egui_view_virtual_stage_t *local, uint8_t slot_index)
{
    if (local->captured_slot == slot_index)
    {
        return 1;
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (local->slots[slot_index].view != NULL && local->slots[slot_index].view->is_focused)
    {
        return 1;
    }
#endif

    return 0;
}

static int egui_view_virtual_stage_find_oldest_evictable_slot(egui_view_virtual_stage_t *local, uint16_t view_type, uint8_t require_same_type)
{
    int candidate = -1;
    uint32_t oldest_seq = 0;
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS; i++)
    {
        egui_view_virtual_stage_slot_t *slot = &local->slots[i];

        if (slot->state == EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_UNUSED)
        {
            continue;
        }
        if (egui_view_virtual_stage_slot_is_protected(local, i))
        {
            continue;
        }
        if (require_same_type && slot->view_type != view_type)
        {
            continue;
        }

        if (candidate < 0 || slot->last_used_seq < oldest_seq)
        {
            candidate = (int)i;
            oldest_seq = slot->last_used_seq;
        }
    }

    return candidate;
}

static uint8_t egui_view_virtual_stage_node_requires_materialization(egui_view_virtual_stage_t *local, const egui_view_virtual_stage_cached_node_t *node);

static int egui_view_virtual_stage_try_acquire_slot_without_eviction(egui_view_virtual_stage_t *local, const egui_view_virtual_stage_cached_node_t *node)
{
    int slot_index;

    slot_index = egui_view_virtual_stage_find_slot_index_by_stable_id(local, node->desc.stable_id);
    if (slot_index >= 0)
    {
        return slot_index;
    }

    slot_index = egui_view_virtual_stage_find_slot_by_state_and_type(local, EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_POOLED, node->desc.view_type);
    if (slot_index >= 0)
    {
        return slot_index;
    }

    egui_view_virtual_stage_compact_slot_count(local);
    if (local->slot_count < local->live_slot_limit)
    {
        return egui_view_virtual_stage_find_unused_slot(local);
    }

    return -1;
}

static uint8_t egui_view_virtual_stage_bind_slot(egui_view_t *self, egui_view_virtual_stage_t *local, egui_view_virtual_stage_slot_t *slot,
                                                 const egui_view_virtual_stage_cached_node_t *node, uint8_t state)
{
    uint8_t needs_bind = 0;
    uint8_t needs_restore = 0;
    uint8_t preserve_runtime_region = 0;
    uint8_t recreated_view = slot->view != NULL && slot->view_type != node->desc.view_type;

    if (!egui_view_virtual_stage_prepare_slot_view(self, local, slot, node->desc.view_type))
    {
        return 0;
    }

    if (slot->stable_id != node->desc.stable_id || slot->state == EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_POOLED)
    {
        needs_bind = 1;
        needs_restore = 1;
    }
    else if (local->is_data_dirty || slot->index != node->index || slot->needs_bind)
    {
        needs_bind = 1;
    }
    if (recreated_view)
    {
        needs_restore = 1;
    }
    if (slot->stable_id != node->desc.stable_id || slot->state == EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_POOLED || recreated_view)
    {
        memset(&slot->touch_state, 0, sizeof(slot->touch_state));
    }

    if (needs_bind && local->adapter != NULL && local->adapter->bind_view != NULL)
    {
        local->adapter->bind_view(local->adapter_context, slot->view, node->index, node->desc.stable_id, &node->desc);
    }

    if (needs_restore && local->adapter != NULL && local->adapter->restore_state != NULL)
    {
        local->adapter->restore_state(local->adapter_context, slot->view, node->index, node->desc.stable_id, &node->desc);
    }

    preserve_runtime_region = (!needs_bind && !local->is_layout_dirty && !slot->needs_layout && slot->state != EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_POOLED &&
                               slot->stable_id == node->desc.stable_id);

    slot->state = state;
    slot->view_type = node->desc.view_type;
    slot->index = node->index;
    slot->stable_id = node->desc.stable_id;
    slot->last_used_seq = ++local->slot_use_seq;
    slot->bound_desc = node->desc;
    if (preserve_runtime_region)
    {
        slot->render_region = slot->view->region;
        egui_view_set_gone(slot->view, 0);
    }
    else
    {
        egui_view_virtual_stage_apply_render_region(slot, node);
    }
    slot->needs_bind = 0;
    slot->needs_layout = 0;
    slot->view->api->calculate_layout(slot->view);
    return 1;
}

static uint8_t egui_view_virtual_stage_refresh_cached_node(egui_view_t *self, egui_view_virtual_stage_t *local, uint32_t stable_id, uint8_t force_layout)
{
    egui_view_virtual_stage_cached_node_t *node;
    egui_view_virtual_stage_slot_t *slot = NULL;
    egui_virtual_stage_node_desc_t old_desc;
    egui_virtual_stage_node_desc_t new_desc;
    uint8_t needs_layout;
    uint8_t needs_resort = 0;
    int slot_index;

    if (stable_id == EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID || local->adapter == NULL || local->adapter->get_desc == NULL || local->node_cache == NULL ||
        local->is_data_dirty)
    {
        return 0;
    }

    node = egui_view_virtual_stage_find_node_by_stable_id(local, stable_id);
    if (node == NULL)
    {
        return 0;
    }

    old_desc = node->desc;
    memset(&new_desc, 0, sizeof(new_desc));
    if (!local->adapter->get_desc(local->adapter_context, node->index, &new_desc))
    {
        return 0;
    }
    if (new_desc.stable_id == EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID)
    {
        new_desc.stable_id = node->index;
    }
    if (new_desc.stable_id != stable_id)
    {
        return 0;
    }

    node->desc = new_desc;
    if (old_desc.z_order != new_desc.z_order)
    {
        needs_resort = 1;
    }

    needs_layout = force_layout || !egui_region_equal(&old_desc.region, &new_desc.region) || old_desc.view_type != new_desc.view_type ||
                   ((old_desc.flags ^ new_desc.flags) & EGUI_VIRTUAL_STAGE_NODE_FLAG_HIDDEN) != 0;

    slot_index = egui_view_virtual_stage_find_slot_index_by_stable_id(local, stable_id);
    if (slot_index >= 0)
    {
        slot = &local->slots[slot_index];
        slot->needs_bind = 1;
        if (needs_layout)
        {
            slot->needs_layout = 1;
        }
    }

    egui_view_virtual_stage_invalidate_node_region(self, &old_desc);
    if (!egui_region_equal(&old_desc.region, &new_desc.region) || old_desc.view_type != new_desc.view_type || old_desc.flags != new_desc.flags ||
        old_desc.z_order != new_desc.z_order)
    {
        egui_view_virtual_stage_invalidate_node_region(self, &new_desc);
    }

    if (slot != NULL)
    {
        if (!needs_layout && !local->is_layout_dirty && !self->is_request_layout)
        {
            if (egui_view_virtual_stage_bind_slot(self, local, slot, node, slot->state))
            {
                if (needs_resort)
                {
                    egui_view_virtual_stage_sort_nodes(local);
                }
                return 1;
            }
        }

        egui_view_request_layout(self);
    }
    else if (egui_view_virtual_stage_node_requires_materialization(local, node))
    {
        local->is_layout_dirty = 1;
        egui_view_request_layout(self);
    }

    if (needs_resort)
    {
        egui_view_virtual_stage_sort_nodes(local);
    }

    return 1;
}

static uint8_t egui_view_virtual_stage_slot_should_keep_alive(egui_view_virtual_stage_t *local, uint8_t slot_index,
                                                              const egui_view_virtual_stage_cached_node_t *node)
{
    egui_view_virtual_stage_slot_t *slot = &local->slots[slot_index];

    if (local->captured_slot == slot_index)
    {
        return 1;
    }
    if (egui_view_virtual_stage_is_pinned(local, node->desc.stable_id))
    {
        return 1;
    }
    if ((node->desc.flags & EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE) != 0)
    {
        return 1;
    }
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (slot->view != NULL && slot->view->is_focused)
    {
        return 1;
    }
#endif
    if (slot->view != NULL && local->adapter != NULL && local->adapter->should_keep_alive != NULL)
    {
        return local->adapter->should_keep_alive(local->adapter_context, slot->view, node->index, node->desc.stable_id, &node->desc) ? 1 : 0;
    }

    return 0;
}

static uint8_t egui_view_virtual_stage_node_requires_materialization(egui_view_virtual_stage_t *local, const egui_view_virtual_stage_cached_node_t *node)
{
    if (node->desc.view_type == EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE)
    {
        return 0;
    }
    if (egui_view_virtual_stage_is_node_hidden(&node->desc))
    {
        return 0;
    }
    if (egui_view_virtual_stage_is_pinned(local, node->desc.stable_id))
    {
        return 1;
    }
    if ((node->desc.flags & EGUI_VIRTUAL_STAGE_NODE_FLAG_KEEPALIVE) != 0)
    {
        return 1;
    }

    return 0;
}

static int egui_view_virtual_stage_acquire_slot(egui_view_t *self, egui_view_virtual_stage_t *local, const egui_view_virtual_stage_cached_node_t *node)
{
    int slot_index;

    slot_index = egui_view_virtual_stage_try_acquire_slot_without_eviction(local, node);
    if (slot_index >= 0)
    {
        return slot_index;
    }

    slot_index = egui_view_virtual_stage_find_oldest_evictable_slot(local, node->desc.view_type, 1);
    if (slot_index >= 0)
    {
        egui_view_virtual_stage_release_slot(self, local, &local->slots[slot_index],
                                             egui_view_virtual_stage_find_node_by_stable_id(local, local->slots[slot_index].stable_id));
        return slot_index;
    }

    if (local->adapter != NULL && local->adapter->destroy_view != NULL)
    {
        slot_index = egui_view_virtual_stage_find_oldest_evictable_slot(local, 0, 0);
        if (slot_index >= 0)
        {
            egui_view_virtual_stage_release_slot(self, local, &local->slots[slot_index],
                                                 egui_view_virtual_stage_find_node_by_stable_id(local, local->slots[slot_index].stable_id));
            return slot_index;
        }
    }

    return -1;
}

static void egui_view_virtual_stage_trim_to_limit(egui_view_t *self, egui_view_virtual_stage_t *local)
{
    while (1)
    {
        int slot_index;
        uint8_t before;

        egui_view_virtual_stage_compact_slot_count(local);
        if (local->slot_count <= local->live_slot_limit)
        {
            break;
        }

        slot_index = egui_view_virtual_stage_find_oldest_evictable_slot(local, 0, 0);
        if (slot_index < 0)
        {
            break;
        }

        before = local->slot_count;
        egui_view_virtual_stage_release_slot(self, local, &local->slots[slot_index],
                                             egui_view_virtual_stage_find_node_by_stable_id(local, local->slots[slot_index].stable_id));
        egui_view_virtual_stage_compact_slot_count(local);
        if (local->slot_count >= before)
        {
            break;
        }
    }
}

static void egui_view_virtual_stage_sync_slots(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);
    uint8_t i;
    uint32_t node_pos;

    if (local->is_data_dirty)
    {
        if (!egui_view_virtual_stage_reload_cache(local))
        {
            return;
        }
    }

    egui_view_virtual_stage_sync_content_layer(self, local);
    egui_view_calculate_layout(EGUI_VIEW_OF(&local->content_layer));

    for (i = 0; i < EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS; i++)
    {
        egui_view_virtual_stage_slot_t *slot = &local->slots[i];
        egui_view_virtual_stage_cached_node_t *node;

        if (slot->state == EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_UNUSED)
        {
            continue;
        }

        node = egui_view_virtual_stage_find_node_by_stable_id(local, slot->stable_id);
        if (node == NULL || egui_view_virtual_stage_is_node_hidden(&node->desc) || node->desc.view_type == EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE)
        {
            egui_view_virtual_stage_release_slot(self, local, slot, node);
            continue;
        }

        if (egui_view_virtual_stage_slot_should_keep_alive(local, i, node))
        {
            (void)egui_view_virtual_stage_bind_slot(self, local, slot, node,
                                                    local->captured_slot == i ? EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_ACTIVE
                                                                              : EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_KEEPALIVE);
        }
        else
        {
            egui_view_virtual_stage_release_slot(self, local, slot, node);
        }
    }

    for (node_pos = 0; node_pos < local->cached_node_count; node_pos++)
    {
        egui_view_virtual_stage_cached_node_t *nodes = egui_view_virtual_stage_get_nodes(local);
        egui_view_virtual_stage_cached_node_t *node;
        int slot_index;

        if (nodes == NULL)
        {
            break;
        }

        node = &nodes[node_pos];
        if (!egui_view_virtual_stage_node_requires_materialization(local, node))
        {
            continue;
        }

        slot_index = egui_view_virtual_stage_find_slot_index_by_stable_id(local, node->desc.stable_id);
        if (slot_index >= 0)
        {
            continue;
        }

        slot_index = egui_view_virtual_stage_try_acquire_slot_without_eviction(local, node);
        if (slot_index < 0)
        {
            continue;
        }

        (void)egui_view_virtual_stage_bind_slot(self, local, &local->slots[slot_index], node, EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_KEEPALIVE);
    }

    egui_view_virtual_stage_trim_to_limit(self, local);
    egui_view_virtual_stage_compact_slot_count(local);
    local->is_data_dirty = 0;
    local->is_layout_dirty = 0;
    local->needs_slot_sync = 0;
}

static uint8_t egui_view_virtual_stage_hit_test_node(egui_view_t *self, egui_view_virtual_stage_t *local, const egui_view_virtual_stage_cached_node_t *node,
                                                     const egui_motion_event_t *event)
{
    egui_region_t screen_region;

    if ((node->desc.flags & EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE) == 0)
    {
        return 0;
    }
    if (node->desc.view_type == EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE)
    {
        return 0;
    }
    if (egui_view_virtual_stage_is_node_hidden(&node->desc))
    {
        return 0;
    }

    egui_view_virtual_stage_get_screen_region(self, &node->desc, &screen_region);
    if (local->adapter != NULL && local->adapter->hit_test != NULL)
    {
        return local->adapter->hit_test(local->adapter_context, node->index, &node->desc, &screen_region, event->location.x, event->location.y) ? 1 : 0;
    }

    return egui_region_pt_in_rect(&screen_region, event->location.x, event->location.y) ? 1 : 0;
}

static const egui_motion_event_t *egui_view_virtual_stage_adjust_followup_event_for_hit_test(egui_view_t *self, egui_view_virtual_stage_t *local,
                                                                                             const egui_view_virtual_stage_cached_node_t *node,
                                                                                             const egui_motion_event_t *event,
                                                                                             egui_motion_event_t *adjusted_event, uint8_t *clear_capture)
{
    egui_region_t screen_region;
    egui_dim_t left_distance;
    egui_dim_t right_distance;
    egui_dim_t top_distance;
    egui_dim_t bottom_distance;
    egui_dim_t min_distance;

    if (clear_capture != NULL)
    {
        *clear_capture = 0;
    }
    if (event->type != EGUI_MOTION_EVENT_ACTION_MOVE && event->type != EGUI_MOTION_EVENT_ACTION_UP)
    {
        return event;
    }
    if (node == NULL)
    {
        *adjusted_event = *event;
        adjusted_event->type = EGUI_MOTION_EVENT_ACTION_CANCEL;
        if (clear_capture != NULL)
        {
            *clear_capture = 1;
        }
        return adjusted_event;
    }
    if ((node->desc.flags & EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE) == 0 || node->desc.view_type == EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE ||
        egui_view_virtual_stage_is_node_hidden(&node->desc))
    {
        *adjusted_event = *event;
        adjusted_event->type = EGUI_MOTION_EVENT_ACTION_CANCEL;
        if (clear_capture != NULL)
        {
            *clear_capture = 1;
        }
        return adjusted_event;
    }
    if (local->adapter == NULL || local->adapter->hit_test == NULL)
    {
        return event;
    }

    egui_view_virtual_stage_get_screen_region(self, &node->desc, &screen_region);
    if (!egui_region_pt_in_rect(&screen_region, event->location.x, event->location.y))
    {
        return event;
    }
    if (local->adapter->hit_test(local->adapter_context, node->index, &node->desc, &screen_region, event->location.x, event->location.y))
    {
        return event;
    }

    *adjusted_event = *event;
    left_distance = event->location.x - screen_region.location.x;
    right_distance = screen_region.location.x + screen_region.size.width - event->location.x;
    top_distance = event->location.y - screen_region.location.y;
    bottom_distance = screen_region.location.y + screen_region.size.height - event->location.y;
    min_distance = left_distance;
    adjusted_event->location.x = screen_region.location.x - 1;

    if (right_distance < min_distance)
    {
        min_distance = right_distance;
        adjusted_event->location.x = screen_region.location.x + screen_region.size.width;
        adjusted_event->location.y = event->location.y;
    }
    if (top_distance < min_distance)
    {
        min_distance = top_distance;
        adjusted_event->location.x = event->location.x;
        adjusted_event->location.y = screen_region.location.y - 1;
    }
    if (bottom_distance < min_distance)
    {
        adjusted_event->location.x = event->location.x;
        adjusted_event->location.y = screen_region.location.y + screen_region.size.height;
    }

    return adjusted_event;
}

static int egui_view_virtual_stage_dispatch_touch_to_slot(egui_view_virtual_stage_t *local, egui_view_virtual_stage_slot_t *slot, egui_motion_event_t *event)
{
    egui_view_group_touch_state_snapshot_t outer_touch_state;

    if (slot == NULL || slot->view == NULL)
    {
        return 0;
    }

    slot->last_used_seq = ++local->slot_use_seq;
    memset(&outer_touch_state, 0, sizeof(outer_touch_state));
    egui_view_group_touch_state_exchange(&outer_touch_state);
    egui_view_group_touch_state_exchange(&slot->touch_state);
    {
        int handled = slot->view->api->dispatch_touch_event(slot->view, event);
        egui_view_group_touch_state_exchange(&slot->touch_state);
        egui_view_group_touch_state_exchange(&outer_touch_state);
        return handled;
    }
}

static int egui_view_virtual_stage_dispatch_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);

    if (!self->is_enable || !self->is_visible || self->is_gone)
    {
        return 0;
    }

    if (self->is_request_layout || EGUI_VIEW_OF(&local->content_layer)->is_request_layout || local->is_data_dirty || local->is_layout_dirty ||
        local->needs_slot_sync)
    {
        self->api->calculate_layout(self);
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
    {
        egui_view_virtual_stage_cached_node_t *nodes = egui_view_virtual_stage_get_nodes(local);
        int32_t draw_pos;

        local->captured_slot = EGUI_VIEW_VIRTUAL_STAGE_INVALID_SLOT;
        if (!egui_region_pt_in_rect(&self->region_screen, event->location.x, event->location.y))
        {
            return 0;
        }

        if (nodes == NULL)
        {
            return egui_view_dispatch_touch_event(self, event);
        }

        for (draw_pos = (int32_t)local->cached_node_count - 1; draw_pos >= 0; draw_pos--)
        {
            egui_view_virtual_stage_cached_node_t *node = &nodes[draw_pos];
            int slot_index;

            if (!egui_view_virtual_stage_hit_test_node(self, local, node, event))
            {
                continue;
            }

            slot_index = egui_view_virtual_stage_acquire_slot(self, local, node);
            if (slot_index < 0)
            {
                return 1;
            }
            if (!egui_view_virtual_stage_bind_slot(self, local, &local->slots[slot_index], node, EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_ACTIVE))
            {
                return 1;
            }

            local->captured_slot = (uint8_t)slot_index;
            (void)egui_view_virtual_stage_dispatch_touch_to_slot(local, &local->slots[slot_index], event);
            return 1;
        }

        return egui_view_dispatch_touch_event(self, event);
    }
    case EGUI_MOTION_EVENT_ACTION_MOVE:
    case EGUI_MOTION_EVENT_ACTION_UP:
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
    {
        egui_view_virtual_stage_slot_t *slot;
        egui_view_virtual_stage_cached_node_t *node;
        egui_virtual_stage_node_desc_t released_desc;
        egui_motion_event_t adjusted_event;
        const egui_motion_event_t *dispatch_event = event;
        uint8_t clear_capture = 0;
        uint8_t released_desc_valid = 0;

        if (local->captured_slot == EGUI_VIEW_VIRTUAL_STAGE_INVALID_SLOT || local->captured_slot >= EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS)
        {
            return 0;
        }

        slot = &local->slots[local->captured_slot];
        node = egui_view_virtual_stage_find_node_by_stable_id(local, slot->stable_id);
        dispatch_event = egui_view_virtual_stage_adjust_followup_event_for_hit_test(self, local, node, event, &adjusted_event, &clear_capture);
        (void)egui_view_virtual_stage_dispatch_touch_to_slot(local, slot, (egui_motion_event_t *)dispatch_event);

        if (clear_capture || event->type == EGUI_MOTION_EVENT_ACTION_UP || event->type == EGUI_MOTION_EVENT_ACTION_CANCEL)
        {
            local->captured_slot = EGUI_VIEW_VIRTUAL_STAGE_INVALID_SLOT;
        }

        if (slot->bound_desc.stable_id == slot->stable_id && slot->stable_id != EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID)
        {
            released_desc = slot->bound_desc;
            released_desc_valid = 1;
        }
        else if (node != NULL)
        {
            released_desc = node->desc;
            released_desc_valid = 1;
        }

        if (node == NULL)
        {
            egui_view_virtual_stage_release_slot(self, local, slot, NULL);
        }
        else if (egui_view_virtual_stage_slot_should_keep_alive(local, (uint8_t)(slot - local->slots), node))
        {
            (void)egui_view_virtual_stage_bind_slot(self, local, slot, node,
                                                    local->captured_slot == (uint8_t)(slot - local->slots) ? EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_ACTIVE
                                                                                                           : EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_KEEPALIVE);
        }
        else
        {
            egui_view_virtual_stage_release_slot(self, local, slot, node);
        }

        if (!egui_view_virtual_stage_slot_is_bound(slot))
        {
            if (released_desc_valid)
            {
                egui_view_virtual_stage_invalidate_node_region(self, &released_desc);
            }
        }

        if (clear_capture || event->type == EGUI_MOTION_EVENT_ACTION_UP || event->type == EGUI_MOTION_EVENT_ACTION_CANCEL)
        {
            local->needs_slot_sync = 1;
        }

        egui_view_virtual_stage_trim_to_limit(self, local);
        egui_view_virtual_stage_compact_slot_count(local);
        return 1;
    }
    default:
        return 0;
    }
}

static void egui_view_virtual_stage_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);
    egui_alpha_t alpha = egui_canvas_get_alpha();
    egui_view_virtual_stage_cached_node_t *nodes = egui_view_virtual_stage_get_nodes(local);
    uint32_t draw_pos;
    egui_region_t clip_region;
    const egui_region_t *prev_clip = egui_canvas_get_extra_clip();
    const egui_region_t *active_clip = &self->region_screen;

    if (prev_clip != NULL)
    {
        egui_region_intersect(&self->region_screen, prev_clip, &clip_region);
        active_clip = &clip_region;
    }

    egui_canvas_set_extra_clip(active_clip);
    egui_view_draw(self);

    if (!self->is_visible || self->is_gone || nodes == NULL)
    {
        egui_canvas_set_alpha(alpha);
        if (prev_clip != NULL)
        {
            egui_canvas_set_extra_clip(prev_clip);
        }
        else
        {
            egui_canvas_clear_extra_clip();
        }
        return;
    }

    for (draw_pos = 0; draw_pos < local->cached_node_count; draw_pos++)
    {
        egui_view_virtual_stage_cached_node_t *node = &nodes[draw_pos];
        egui_region_t screen_region;
        int slot_index;

        if (egui_view_virtual_stage_is_node_hidden(&node->desc))
        {
            continue;
        }

        egui_view_virtual_stage_get_screen_region(self, &node->desc, &screen_region);
        egui_canvas_calc_work_region(&self->region_screen);
        if (!egui_canvas_is_region_active(&screen_region))
        {
            continue;
        }

        slot_index = egui_view_virtual_stage_find_slot_index_by_stable_id(local, node->desc.stable_id);
        egui_canvas_set_alpha(alpha);
        egui_canvas_mix_alpha(self->alpha);

        if (slot_index >= 0 && egui_view_virtual_stage_slot_is_bound(&local->slots[slot_index]) && local->slots[slot_index].view != NULL)
        {
            local->slots[slot_index].view->api->draw(local->slots[slot_index].view);
        }
        else if (local->adapter != NULL && local->adapter->draw_node != NULL)
        {
            egui_canvas_calc_work_region(&screen_region);
            if (!egui_region_is_empty(egui_canvas_get_base_view_work_region()))
            {
                local->adapter->draw_node(local->adapter_context, self, node->index, &node->desc, &screen_region);
            }
        }
    }

    egui_canvas_set_alpha(alpha);
    if (prev_clip != NULL)
    {
        egui_canvas_set_extra_clip(prev_clip);
    }
    else
    {
        egui_canvas_clear_extra_clip();
    }
}

static void egui_view_virtual_stage_calculate_layout(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);

    egui_view_calculate_layout(self);
    egui_view_virtual_stage_sync_content_layer(self, local);
    egui_view_calculate_layout(EGUI_VIEW_OF(&local->content_layer));

    if (local->is_data_dirty || local->is_layout_dirty || local->needs_slot_sync || local->slot_count > 0)
    {
        egui_view_virtual_stage_sync_slots(self);
    }

    EGUI_VIEW_OF(&local->content_layer)->api->calculate_layout(EGUI_VIEW_OF(&local->content_layer));
}

static void egui_view_virtual_stage_on_attach_to_window(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);

    local->is_attached_to_window = 1;
    egui_view_group_on_attach_to_window(self);
}

static void egui_view_virtual_stage_on_detach_from_window(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);

    local->is_attached_to_window = 0;
    egui_view_group_on_detach_from_window(self);
}

void egui_view_virtual_stage_apply_params(egui_view_t *self, const egui_view_virtual_stage_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);

    self->region = params->region;
    local->live_slot_limit = params->live_slot_limit > EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS ? EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS : params->live_slot_limit;
    egui_view_virtual_stage_mark_dirty(self, local, 1, 1);
}

void egui_view_virtual_stage_init_with_params(egui_view_t *self, const egui_view_virtual_stage_params_t *params)
{
    egui_view_virtual_stage_init(self);
    egui_view_virtual_stage_apply_params(self, params);
}

void egui_view_virtual_stage_apply_setup(egui_view_t *self, const egui_view_virtual_stage_setup_t *setup)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);

    if (setup == NULL)
    {
        return;
    }

    if (setup->params != NULL)
    {
        egui_view_virtual_stage_apply_params(self, setup->params);
    }
    else
    {
        egui_view_virtual_stage_mark_dirty(self, local, 1, 1);
    }

    egui_view_virtual_stage_set_adapter(self, setup->adapter, setup->adapter_context);
}

void egui_view_virtual_stage_init_with_setup(egui_view_t *self, const egui_view_virtual_stage_setup_t *setup)
{
    egui_view_virtual_stage_init(self);
    egui_view_virtual_stage_apply_setup(self, setup);
}

void egui_view_virtual_stage_apply_array_setup(egui_view_t *self, egui_view_virtual_stage_array_adapter_t *adapter,
                                               const egui_view_virtual_stage_array_setup_t *setup)
{
    egui_view_virtual_stage_setup_t bridge_setup;

    if (adapter == NULL)
    {
        return;
    }

    memset(&bridge_setup, 0, sizeof(bridge_setup));
    if (setup != NULL)
    {
        egui_view_virtual_stage_array_adapter_init(adapter, setup->node_source, setup->ops, setup->user_context);
        bridge_setup.params = setup->params;
    }
    else
    {
        egui_view_virtual_stage_array_adapter_init(adapter, NULL, NULL, NULL);
    }

    bridge_setup.adapter = &adapter->adapter;
    bridge_setup.adapter_context = adapter;
    egui_view_virtual_stage_apply_setup(self, &bridge_setup);
}

void egui_view_virtual_stage_init_with_array_setup(egui_view_t *self, egui_view_virtual_stage_array_adapter_t *adapter,
                                                   const egui_view_virtual_stage_array_setup_t *setup)
{
    egui_view_virtual_stage_init(self);
    egui_view_virtual_stage_apply_array_setup(self, adapter, setup);
}

void egui_view_virtual_stage_array_adapter_init(egui_view_virtual_stage_array_adapter_t *adapter, const egui_view_virtual_stage_array_source_t *node_source,
                                                const egui_view_virtual_stage_array_ops_t *ops, void *user_context)
{
    if (adapter == NULL)
    {
        return;
    }

    memset(adapter, 0, sizeof(*adapter));
    adapter->adapter.get_count = egui_view_virtual_stage_array_adapter_get_count;
    adapter->adapter.get_desc = egui_view_virtual_stage_array_adapter_get_desc;
    adapter->adapter.create_view = egui_view_virtual_stage_array_adapter_create_view;
    adapter->adapter.destroy_view = egui_view_virtual_stage_array_adapter_destroy_view;
    adapter->adapter.bind_view = egui_view_virtual_stage_array_adapter_bind_view;
    adapter->adapter.unbind_view = egui_view_virtual_stage_array_adapter_unbind_view;
    adapter->adapter.save_state = egui_view_virtual_stage_array_adapter_save_state;
    adapter->adapter.restore_state = egui_view_virtual_stage_array_adapter_restore_state;
    adapter->adapter.draw_node = egui_view_virtual_stage_array_adapter_draw_node;
    adapter->adapter.hit_test = egui_view_virtual_stage_array_adapter_hit_test;
    adapter->adapter.should_keep_alive = egui_view_virtual_stage_array_adapter_should_keep_alive;
    adapter->node_source = node_source;
    adapter->ops = ops;
    adapter->user_context = user_context;
}

void egui_view_virtual_stage_set_adapter(egui_view_t *self, const egui_view_virtual_stage_adapter_t *adapter, void *adapter_context)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);

    if (local->adapter == adapter && local->adapter_context == adapter_context)
    {
        return;
    }

    egui_view_virtual_stage_destroy_all_slots(self, local);
    egui_view_virtual_stage_release_cache(local);
    egui_view_virtual_stage_release_pins(local);
    local->adapter = adapter;
    local->adapter_context = adapter_context;
    egui_view_virtual_stage_mark_dirty(self, local, 1, 1);
}

const egui_view_virtual_stage_adapter_t *egui_view_virtual_stage_get_adapter(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);
    return local->adapter;
}

void *egui_view_virtual_stage_get_adapter_context(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);
    return local->adapter_context;
}

void egui_view_virtual_stage_set_live_slot_limit(egui_view_t *self, uint8_t live_slot_limit)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);

    if (live_slot_limit > EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS)
    {
        live_slot_limit = EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS;
    }

    if (local->live_slot_limit == live_slot_limit)
    {
        return;
    }

    local->live_slot_limit = live_slot_limit;
    egui_view_virtual_stage_mark_dirty(self, local, 0, 1);
}

uint8_t egui_view_virtual_stage_get_live_slot_limit(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);
    return local->live_slot_limit;
}

void egui_view_virtual_stage_notify_data_changed(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);
    egui_view_virtual_stage_mark_dirty(self, local, 1, 1);
}

void egui_view_virtual_stage_notify_node_changed(egui_view_t *self, uint32_t stable_id)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);

    if (!egui_view_virtual_stage_refresh_cached_node(self, local, stable_id, 0))
    {
        egui_view_virtual_stage_mark_dirty(self, local, 1, 1);
    }
}

void egui_view_virtual_stage_notify_node_bounds_changed(egui_view_t *self, uint32_t stable_id)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);

    if (!egui_view_virtual_stage_refresh_cached_node(self, local, stable_id, 1))
    {
        egui_view_virtual_stage_mark_dirty(self, local, 1, 1);
    }
}

void egui_view_virtual_stage_notify_nodes_changed(egui_view_t *self, const uint32_t *stable_ids, uint32_t count)
{
    uint32_t i;

    if (stable_ids == NULL)
    {
        return;
    }

    for (i = 0; i < count; i++)
    {
        egui_view_virtual_stage_notify_node_changed(self, stable_ids[i]);
    }
}

void egui_view_virtual_stage_notify_nodes_bounds_changed(egui_view_t *self, const uint32_t *stable_ids, uint32_t count)
{
    uint32_t i;

    if (stable_ids == NULL)
    {
        return;
    }

    for (i = 0; i < count; i++)
    {
        egui_view_virtual_stage_notify_node_bounds_changed(self, stable_ids[i]);
    }
}

uint8_t egui_view_virtual_stage_pin_node(egui_view_t *self, uint32_t stable_id)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);
    egui_view_virtual_stage_pin_entry_t *pins;
    egui_virtual_stage_node_desc_t desc;
    int pin_index;

    if (stable_id == EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID)
    {
        return 0;
    }

    pin_index = egui_view_virtual_stage_find_pin_entry(local, stable_id);
    if (pin_index >= 0)
    {
        egui_view_virtual_stage_get_pins(local)[pin_index].ref_count++;
        egui_view_virtual_stage_mark_dirty(self, local, 0, 1);
        return 1;
    }

    if (!egui_view_virtual_stage_resolve_node_desc_by_stable_id(local, stable_id, &desc))
    {
        return 0;
    }
    if (desc.view_type == EGUI_VIEW_VIRTUAL_STAGE_VIEW_TYPE_NONE || egui_view_virtual_stage_is_node_hidden(&desc))
    {
        return 0;
    }

    if (!egui_view_virtual_stage_ensure_pin_capacity(local, (uint16_t)(local->pin_count + 1)))
    {
        return 0;
    }

    pins = egui_view_virtual_stage_get_pins(local);
    pins[local->pin_count].stable_id = stable_id;
    pins[local->pin_count].ref_count = 1;
    local->pin_count++;
    egui_view_virtual_stage_mark_dirty(self, local, 0, 1);
    return 1;
}

void egui_view_virtual_stage_unpin_node(egui_view_t *self, uint32_t stable_id)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);
    egui_view_virtual_stage_pin_entry_t *pins = egui_view_virtual_stage_get_pins(local);
    int pin_index = egui_view_virtual_stage_find_pin_entry(local, stable_id);

    if (pin_index < 0 || pins == NULL)
    {
        return;
    }

    if (pins[pin_index].ref_count > 1)
    {
        pins[pin_index].ref_count--;
    }
    else
    {
        egui_view_virtual_stage_remove_pin_entry(local, (uint16_t)pin_index);
    }
    egui_view_virtual_stage_mark_dirty(self, local, 0, 1);
}

uint8_t egui_view_virtual_stage_is_node_pinned(egui_view_t *self, uint32_t stable_id)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);
    return egui_view_virtual_stage_is_pinned(local, stable_id);
}

uint8_t egui_view_virtual_stage_toggle_pin_node(egui_view_t *self, uint32_t stable_id)
{
    if (egui_view_virtual_stage_is_node_pinned(self, stable_id))
    {
        egui_view_virtual_stage_unpin_node(self, stable_id);
        return 0;
    }

    return egui_view_virtual_stage_pin_node(self, stable_id) ? 1U : 0U;
}

uint8_t egui_view_virtual_stage_get_slot_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);
    egui_view_virtual_stage_compact_slot_count(local);
    return local->slot_count;
}

const egui_view_virtual_stage_slot_t *egui_view_virtual_stage_get_slot(egui_view_t *self, uint8_t slot_index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);

    if (slot_index >= EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS)
    {
        return NULL;
    }

    return &local->slots[slot_index];
}

const egui_view_virtual_stage_slot_t *egui_view_virtual_stage_find_slot_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);
    int slot_index = egui_view_virtual_stage_find_slot_index_by_stable_id(local, stable_id);

    if (slot_index < 0)
    {
        return NULL;
    }

    return &local->slots[slot_index];
}

egui_view_t *egui_view_virtual_stage_find_view_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    const egui_view_virtual_stage_slot_t *slot = egui_view_virtual_stage_find_slot_by_stable_id(self, stable_id);

    return slot != NULL ? slot->view : NULL;
}

uint8_t egui_view_virtual_stage_resolve_node_by_view(egui_view_t *self, egui_view_t *node_view, egui_view_virtual_stage_entry_t *entry)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);
    egui_view_t *current = node_view;
    egui_view_virtual_stage_cached_node_t *node;
    uint8_t i;

    if (node_view == NULL || entry == NULL)
    {
        return 0;
    }

    while (current != NULL)
    {
        for (i = 0; i < EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS; i++)
        {
            const egui_view_virtual_stage_slot_t *slot = &local->slots[i];

            if (!egui_view_virtual_stage_slot_is_bound(slot) || slot->view != current)
            {
                continue;
            }

            entry->index = slot->index;
            entry->stable_id = slot->stable_id;
            if (slot->bound_desc.stable_id == slot->stable_id)
            {
                entry->desc = slot->bound_desc;
            }
            else
            {
                node = egui_view_virtual_stage_find_node_by_stable_id(local, slot->stable_id);
                if (node != NULL)
                {
                    entry->desc = node->desc;
                }
                else
                {
                    entry->desc.region = slot->render_region;
                    entry->desc.stable_id = slot->stable_id;
                    entry->desc.view_type = slot->view_type;
                    entry->desc.z_order = 0;
                    entry->desc.flags = 0;
                    entry->desc.reserved = 0;
                }
            }
            return 1;
        }

        if (current == self)
        {
            break;
        }
        current = EGUI_VIEW_PARENT(current);
    }

    return 0;
}

uint8_t egui_view_virtual_stage_resolve_stable_id_by_view(egui_view_t *self, egui_view_t *node_view, uint32_t *stable_id_out)
{
    egui_view_virtual_stage_entry_t entry;

    if (stable_id_out == NULL)
    {
        return 0;
    }

    if (!egui_view_virtual_stage_resolve_node_by_view(self, node_view, &entry))
    {
        return 0;
    }

    *stable_id_out = entry.stable_id;
    return 1;
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_virtual_stage_t) = {
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .dispatch_touch_event = egui_view_virtual_stage_dispatch_touch_event,
        .on_touch_event = egui_view_group_on_touch_event,
        .on_intercept_touch_event = egui_view_group_on_intercept_touch_event,
#else
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = NULL,
        .on_intercept_touch_event = NULL,
#endif
        .compute_scroll = egui_view_group_compute_scroll,
        .calculate_layout = egui_view_virtual_stage_calculate_layout,
        .request_layout = egui_view_group_request_layout,
        .draw = egui_view_virtual_stage_draw,
        .on_attach_to_window = egui_view_virtual_stage_on_attach_to_window,
        .on_draw = egui_view_on_draw,
        .on_detach_from_window = egui_view_virtual_stage_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_group_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_virtual_stage_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_virtual_stage_t);

    egui_view_group_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_virtual_stage_t);

    egui_view_group_init(EGUI_VIEW_OF(&local->content_layer));
    egui_view_set_position(EGUI_VIEW_OF(&local->content_layer), 0, 0);
    egui_view_group_add_child(self, EGUI_VIEW_OF(&local->content_layer));

    local->adapter = NULL;
    local->adapter_context = NULL;
    local->node_cache = NULL;
    local->pin_entries = NULL;
    local->cached_node_count = 0;
    local->pin_count = 0;
    local->pin_capacity = 0;
    local->slot_use_seq = 0;
    local->live_slot_limit = 4;
    local->slot_count = 0;
    local->is_data_dirty = 1;
    local->is_layout_dirty = 1;
    local->needs_slot_sync = 0;
    local->is_attached_to_window = 0;
    local->captured_slot = EGUI_VIEW_VIRTUAL_STAGE_INVALID_SLOT;

    egui_view_virtual_stage_reset_slots(local);
    egui_view_set_view_name(self, "egui_view_virtual_stage");
}
