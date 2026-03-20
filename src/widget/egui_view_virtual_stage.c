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
    slot->last_used_seq = 0;
    slot->needs_bind = 0;
    slot->needs_layout = 0;
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
    if (local->draw_order != NULL)
    {
        egui_free(local->draw_order);
        local->draw_order = NULL;
    }

    local->cached_node_count = 0;
    local->cached_capacity = 0;
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

static uint32_t egui_view_virtual_stage_get_node_count(egui_view_virtual_stage_t *local)
{
    if (local->adapter == NULL || local->adapter->get_count == NULL)
    {
        return 0;
    }

    return local->adapter->get_count(local->adapter_context);
}

static void egui_view_virtual_stage_sort_draw_order(egui_view_virtual_stage_t *local)
{
    egui_view_virtual_stage_cached_node_t *nodes = egui_view_virtual_stage_get_nodes(local);
    uint32_t i;

    if (nodes == NULL || local->draw_order == NULL)
    {
        return;
    }

    for (i = 1; i < local->cached_node_count; i++)
    {
        uint32_t key = local->draw_order[i];
        uint32_t j = i;

        while (j > 0)
        {
            egui_view_virtual_stage_cached_node_t *lhs = &nodes[local->draw_order[j - 1]];
            egui_view_virtual_stage_cached_node_t *rhs = &nodes[key];

            if (lhs->desc.z_order < rhs->desc.z_order)
            {
                break;
            }
            if (lhs->desc.z_order == rhs->desc.z_order && lhs->index <= rhs->index)
            {
                break;
            }

            local->draw_order[j] = local->draw_order[j - 1];
            j--;
        }

        local->draw_order[j] = key;
    }
}

static uint8_t egui_view_virtual_stage_reload_cache(egui_view_virtual_stage_t *local)
{
    uint32_t count = egui_view_virtual_stage_get_node_count(local);
    egui_view_virtual_stage_cached_node_t *new_nodes = NULL;
    uint32_t *new_order = NULL;
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

    new_order = (uint32_t *)egui_malloc((int)(sizeof(uint32_t) * count));
    if (new_order == NULL)
    {
        egui_free(new_nodes);
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
        new_order[write_count] = write_count;
        write_count++;
    }

    egui_view_virtual_stage_release_cache(local);

    if (write_count == 0)
    {
        egui_free(new_nodes);
        egui_free(new_order);
        return 1;
    }

    local->node_cache = new_nodes;
    local->draw_order = new_order;
    local->cached_node_count = write_count;
    local->cached_capacity = count;
    egui_view_virtual_stage_sort_draw_order(local);
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
    egui_view_set_position(EGUI_VIEW_OF(&local->content_layer), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&local->content_layer), self->region.size.width, self->region.size.height);
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

static void egui_view_virtual_stage_save_and_unbind(egui_view_virtual_stage_t *local, egui_view_virtual_stage_slot_t *slot,
                                                   const egui_view_virtual_stage_cached_node_t *node)
{
    uint32_t index = node != NULL ? node->index : slot->index;
    const egui_virtual_stage_node_desc_t *desc = node != NULL ? &node->desc : NULL;

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
}

static void egui_view_virtual_stage_destroy_slot_view(egui_view_t *self, egui_view_virtual_stage_t *local, egui_view_virtual_stage_slot_t *slot,
                                                     const egui_view_virtual_stage_cached_node_t *node)
{
    if (slot->view == NULL)
    {
        egui_view_virtual_stage_reset_slot(slot);
        return;
    }

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

static int egui_view_virtual_stage_find_slot_by_stable_id(egui_view_virtual_stage_t *local, uint32_t stable_id)
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
    const egui_view_virtual_stage_slot_t *slot = &local->slots[slot_index];

    if (local->captured_slot == slot_index)
    {
        return 1;
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (slot->view != NULL && slot->view->is_focused)
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

static uint8_t egui_view_virtual_stage_bind_slot(egui_view_t *self, egui_view_virtual_stage_t *local, egui_view_virtual_stage_slot_t *slot,
                                                const egui_view_virtual_stage_cached_node_t *node, uint8_t state)
{
    uint8_t needs_bind = 0;
    uint8_t needs_restore = 0;
    uint8_t preserve_runtime_region = 0;

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

    if (needs_bind && local->adapter != NULL && local->adapter->bind_view != NULL)
    {
        local->adapter->bind_view(local->adapter_context, slot->view, node->index, node->desc.stable_id, &node->desc);
    }

    if (needs_restore && local->adapter != NULL && local->adapter->restore_state != NULL)
    {
        local->adapter->restore_state(local->adapter_context, slot->view, node->index, node->desc.stable_id, &node->desc);
    }

    preserve_runtime_region = (!needs_bind && !local->is_layout_dirty && !slot->needs_layout &&
                               slot->state != EGUI_VIEW_VIRTUAL_STAGE_SLOT_STATE_POOLED && slot->stable_id == node->desc.stable_id);

    slot->state = state;
    slot->view_type = node->desc.view_type;
    slot->index = node->index;
    slot->stable_id = node->desc.stable_id;
    slot->last_used_seq = ++local->slot_use_seq;
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
    egui_virtual_stage_node_desc_t old_desc;
    egui_virtual_stage_node_desc_t new_desc;
    uint8_t needs_layout;
    int slot_index;

    if (stable_id == EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID || local->adapter == NULL || local->adapter->get_desc == NULL || local->node_cache == NULL ||
        local->draw_order == NULL || local->is_data_dirty)
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
        egui_view_virtual_stage_sort_draw_order(local);
    }

    needs_layout = force_layout || !egui_region_equal(&old_desc.region, &new_desc.region) || old_desc.view_type != new_desc.view_type ||
                   ((old_desc.flags ^ new_desc.flags) & EGUI_VIRTUAL_STAGE_NODE_FLAG_HIDDEN) != 0;

    slot_index = egui_view_virtual_stage_find_slot_by_stable_id(local, stable_id);
    if (slot_index >= 0)
    {
        local->slots[slot_index].needs_bind = 1;
        if (needs_layout)
        {
            local->slots[slot_index].needs_layout = 1;
        }
    }

    egui_view_virtual_stage_invalidate_node_region(self, &old_desc);
    if (!egui_region_equal(&old_desc.region, &new_desc.region) || old_desc.view_type != new_desc.view_type || old_desc.flags != new_desc.flags ||
        old_desc.z_order != new_desc.z_order)
    {
        egui_view_virtual_stage_invalidate_node_region(self, &new_desc);
    }

    if (slot_index >= 0 || egui_view_virtual_stage_node_requires_materialization(local, node))
    {
        egui_view_request_layout(self);
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

    slot_index = egui_view_virtual_stage_find_slot_by_stable_id(local, node->desc.stable_id);
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
        slot_index = egui_view_virtual_stage_find_unused_slot(local);
        if (slot_index >= 0)
        {
            return slot_index;
        }
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
    uint32_t draw_order_pos;

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
            if (local->captured_slot == i)
            {
                local->captured_slot = EGUI_VIEW_VIRTUAL_STAGE_INVALID_SLOT;
            }
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

    for (draw_order_pos = 0; draw_order_pos < local->cached_node_count; draw_order_pos++)
    {
        egui_view_virtual_stage_cached_node_t *nodes = egui_view_virtual_stage_get_nodes(local);
        egui_view_virtual_stage_cached_node_t *node;
        int slot_index;

        if (nodes == NULL || local->draw_order == NULL)
        {
            break;
        }

        node = &nodes[local->draw_order[draw_order_pos]];
        if (!egui_view_virtual_stage_node_requires_materialization(local, node))
        {
            continue;
        }

        slot_index = egui_view_virtual_stage_find_slot_by_stable_id(local, node->desc.stable_id);
        if (slot_index >= 0)
        {
            continue;
        }

        slot_index = egui_view_virtual_stage_acquire_slot(self, local, node);
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
}

static uint8_t egui_view_virtual_stage_hit_test_node(egui_view_t *self, egui_view_virtual_stage_t *local, const egui_view_virtual_stage_cached_node_t *node,
                                                    const egui_motion_event_t *event)
{
    egui_region_t screen_region;

    if ((node->desc.flags & EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE) == 0)
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

static int egui_view_virtual_stage_dispatch_touch_to_slot(egui_view_virtual_stage_t *local, egui_view_virtual_stage_slot_t *slot, egui_motion_event_t *event)
{
    if (slot == NULL || slot->view == NULL)
    {
        return 0;
    }

    slot->last_used_seq = ++local->slot_use_seq;
    return slot->view->api->dispatch_touch_event(slot->view, event);
}

static int egui_view_virtual_stage_dispatch_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);

    if (!self->is_enable || !self->is_visible || self->is_gone)
    {
        return 0;
    }

    if (self->is_request_layout || EGUI_VIEW_OF(&local->content_layer)->is_request_layout || local->is_data_dirty || local->is_layout_dirty)
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

        if (nodes == NULL || local->draw_order == NULL)
        {
            return egui_view_dispatch_touch_event(self, event);
        }

        for (draw_pos = (int32_t)local->cached_node_count - 1; draw_pos >= 0; draw_pos--)
        {
            egui_view_virtual_stage_cached_node_t *node = &nodes[local->draw_order[draw_pos]];
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

        if (local->captured_slot == EGUI_VIEW_VIRTUAL_STAGE_INVALID_SLOT || local->captured_slot >= EGUI_VIEW_VIRTUAL_STAGE_MAX_SLOTS)
        {
            return 0;
        }

        slot = &local->slots[local->captured_slot];
        node = egui_view_virtual_stage_find_node_by_stable_id(local, slot->stable_id);
        (void)egui_view_virtual_stage_dispatch_touch_to_slot(local, slot, event);

        if (event->type == EGUI_MOTION_EVENT_ACTION_UP || event->type == EGUI_MOTION_EVENT_ACTION_CANCEL)
        {
            local->captured_slot = EGUI_VIEW_VIRTUAL_STAGE_INVALID_SLOT;
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

    egui_canvas_set_extra_clip(&self->region_screen);
    egui_view_draw(self);

    if (!self->is_visible || self->is_gone || nodes == NULL || local->draw_order == NULL)
    {
        egui_canvas_set_alpha(alpha);
        egui_canvas_clear_extra_clip();
        return;
    }

    for (draw_pos = 0; draw_pos < local->cached_node_count; draw_pos++)
    {
        egui_view_virtual_stage_cached_node_t *node = &nodes[local->draw_order[draw_pos]];
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

        slot_index = egui_view_virtual_stage_find_slot_by_stable_id(local, node->desc.stable_id);
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
    egui_canvas_clear_extra_clip();
}

static void egui_view_virtual_stage_calculate_layout(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);

    egui_view_calculate_layout(self);
    egui_view_virtual_stage_sync_content_layer(self, local);
    egui_view_calculate_layout(EGUI_VIEW_OF(&local->content_layer));

    if (local->is_data_dirty || local->is_layout_dirty || local->slot_count > 0)
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

uint8_t egui_view_virtual_stage_pin_node(egui_view_t *self, uint32_t stable_id)
{
    EGUI_LOCAL_INIT(egui_view_virtual_stage_t);
    egui_view_virtual_stage_pin_entry_t *pins;
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
    local->draw_order = NULL;
    local->cached_node_count = 0;
    local->cached_capacity = 0;
    local->pin_count = 0;
    local->pin_capacity = 0;
    local->slot_use_seq = 0;
    local->live_slot_limit = 4;
    local->slot_count = 0;
    local->is_data_dirty = 1;
    local->is_layout_dirty = 1;
    local->is_attached_to_window = 0;
    local->captured_slot = EGUI_VIEW_VIRTUAL_STAGE_INVALID_SLOT;

    egui_view_virtual_stage_reset_slots(local);
    egui_view_set_view_name(self, "egui_view_virtual_stage");
}
