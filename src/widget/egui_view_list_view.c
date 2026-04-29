#include "egui_view_list_view.h"

#include <string.h>

#include "core/egui_api.h"
#include "core/egui_common.h"

/**
 * @file egui_view_list_view.c
 * @brief Holder-style adapter that bridges `egui_view_list_view_*` APIs into the virtual-list core.
 *
 * Reading notes:
 * - callers speak in terms of data models and reusable holders;
 * - this file converts those callbacks into the generic virtual-list data source;
 * - each realized item gets an internal host container that owns one holder subtree.
 */

typedef struct egui_view_list_view_item_host egui_view_list_view_item_host_t;

struct egui_view_list_view_item_host
{
    egui_view_group_t root;
    egui_view_list_view_holder_t *holder;
};

/** Default stable-id policy used when the data model does not provide one. */
static uint32_t egui_view_list_view_default_stable_id(uint32_t index)
{
    return index + 1U;
}

/** Reset an entry structure to the virtual-list invalid-id sentinel values. */
static void egui_view_list_view_reset_entry(egui_view_list_view_entry_t *entry)
{
    if (entry == NULL)
    {
        return;
    }

    entry->index = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    entry->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

/** Mark a holder as currently unbound from any logical item. */
static void egui_view_list_view_reset_holder_binding(egui_view_list_view_holder_t *holder)
{
    if (holder == NULL)
    {
        return;
    }

    holder->bound_index = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    holder->bound_stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

/** Interpret the generic host view pointer as the list-view item-host wrapper. */
static egui_view_list_view_item_host_t *egui_view_list_view_host_from_view(egui_view_t *view)
{
    return (egui_view_list_view_item_host_t *)view;
}

/** Resolve the holder stored inside one host view. */
static egui_view_list_view_holder_t *egui_view_list_view_holder_from_host_view(egui_view_t *host_view)
{
    egui_view_list_view_item_host_t *host;

    if (host_view == NULL)
    {
        return NULL;
    }

    host = egui_view_list_view_host_from_view(host_view);
    return host->holder;
}

/** Return the current data-model descriptor and optionally its opaque callback context. */
static const egui_view_list_view_data_model_t *egui_view_list_view_get_data_model_bridge(egui_view_list_view_t *local, void **data_model_context)
{
    if (data_model_context != NULL)
    {
        *data_model_context = local->data_model_context;
    }

    return local->data_model;
}

/** Destroy one holder, preferring the caller-supplied destroy hook when present. */
static void egui_view_list_view_destroy_holder_internal(egui_view_list_view_t *local, egui_view_list_view_holder_t *holder, uint16_t view_type)
{
    if (holder == NULL)
    {
        return;
    }

    if (local->holder_ops != NULL && local->holder_ops->destroy_holder != NULL)
    {
        local->holder_ops->destroy_holder(local->data_model_context, holder, view_type);
        return;
    }

    egui_free(EGUI_VIEW_OF(local)->core, holder);
}

/** Bridge the data-model count callback into the virtual-list data source. */
static uint32_t egui_view_list_view_data_source_get_count(void *data_source_context)
{
    egui_view_list_view_t *local = (egui_view_list_view_t *)data_source_context;
    void *model_context;
    const egui_view_list_view_data_model_t *data_model = egui_view_list_view_get_data_model_bridge(local, &model_context);

    if (data_model == NULL || data_model->get_count == NULL)
    {
        return 0;
    }

    return data_model->get_count(model_context);
}

/** Bridge stable-id lookup, falling back to the default `index + 1` policy. */
static uint32_t egui_view_list_view_data_source_get_stable_id(void *data_source_context, uint32_t index)
{
    egui_view_list_view_t *local = (egui_view_list_view_t *)data_source_context;
    void *model_context;
    const egui_view_list_view_data_model_t *data_model = egui_view_list_view_get_data_model_bridge(local, &model_context);

    if (data_model != NULL && data_model->get_stable_id != NULL)
    {
        return data_model->get_stable_id(model_context, index);
    }

    return egui_view_list_view_default_stable_id(index);
}

/** Resolve an index from a stable id using callbacks, linear search, or the default-id rule. */
static int32_t egui_view_list_view_data_source_find_index_by_stable_id(void *data_source_context, uint32_t stable_id)
{
    egui_view_list_view_t *local = (egui_view_list_view_t *)data_source_context;
    void *model_context;
    const egui_view_list_view_data_model_t *data_model = egui_view_list_view_get_data_model_bridge(local, &model_context);
    uint32_t count;
    uint32_t index;

    if (data_model != NULL && data_model->find_index_by_stable_id != NULL)
    {
        return data_model->find_index_by_stable_id(model_context, stable_id);
    }

    count = egui_view_list_view_data_source_get_count(data_source_context);
    if (data_model != NULL && data_model->get_stable_id != NULL)
    {
        for (index = 0; index < count; index++)
        {
            if (data_model->get_stable_id(model_context, index) == stable_id)
            {
                return (int32_t)index;
            }
        }
        return -1;
    }

    if (stable_id == 0U)
    {
        return -1;
    }

    index = stable_id - 1U;
    return index < count ? (int32_t)index : -1;
}

/** Bridge item view-type lookup into the virtual-list data source. */
static uint16_t egui_view_list_view_data_source_get_view_type(void *data_source_context, uint32_t index)
{
    egui_view_list_view_t *local = (egui_view_list_view_t *)data_source_context;
    void *model_context;
    const egui_view_list_view_data_model_t *data_model = egui_view_list_view_get_data_model_bridge(local, &model_context);

    if (data_model != NULL && data_model->get_view_type != NULL)
    {
        return data_model->get_view_type(model_context, index);
    }

    return data_model != NULL ? data_model->default_view_type : 0;
}

/** Bridge dynamic height measurement, falling back to the virtual-list estimate. */
static int32_t egui_view_list_view_data_source_measure_item_height(void *data_source_context, uint32_t index, int32_t width_hint)
{
    egui_view_list_view_t *local = (egui_view_list_view_t *)data_source_context;
    void *model_context;
    const egui_view_list_view_data_model_t *data_model = egui_view_list_view_get_data_model_bridge(local, &model_context);

    if (data_model != NULL && data_model->measure_item_height != NULL)
    {
        return data_model->measure_item_height(model_context, index, width_hint);
    }

    return egui_view_virtual_list_get_estimated_item_height(EGUI_VIEW_OF(local));
}

/** Create one holder plus an internal host group that the virtual list can own directly. */
static egui_view_t *egui_view_list_view_data_source_create_item_view(void *data_source_context, uint16_t view_type)
{
    egui_view_list_view_t *local = (egui_view_list_view_t *)data_source_context;
    egui_view_list_view_item_host_t *host;
    egui_view_list_view_holder_t *holder;

    if (local->holder_ops == NULL || local->holder_ops->create_holder == NULL)
    {
        return NULL;
    }

    holder = local->holder_ops->create_holder(local->data_model_context, view_type);
    if (holder == NULL || holder->item_view == NULL)
    {
        if (holder != NULL)
        {
            egui_view_list_view_destroy_holder_internal(local, holder, view_type);
        }
        return NULL;
    }

    host = (egui_view_list_view_item_host_t *)egui_malloc(EGUI_VIEW_OF(local)->core, sizeof(egui_view_list_view_item_host_t));
    if (host == NULL)
    {
        egui_view_list_view_destroy_holder_internal(local, holder, view_type);
        return NULL;
    }

    egui_api_memset(host, 0, sizeof(*host));
    egui_view_group_init(EGUI_VIEW_OF(&host->root), EGUI_VIEW_OF(local)->core);
    egui_view_set_dirty_passthrough(EGUI_VIEW_OF(&host->root), 1);
    egui_view_set_view_name(EGUI_VIEW_OF(&host->root), "egui_view_list_view_item");

    host->holder = holder;
    holder->host_view = EGUI_VIEW_OF(&host->root);
    holder->view_type = view_type;
    egui_view_list_view_reset_holder_binding(holder);

    egui_view_group_add_child(EGUI_VIEW_OF(&host->root), holder->item_view);
    return EGUI_VIEW_OF(&host->root);
}

/** Detach a realized holder subtree, destroy the holder, and free the host wrapper. */
static void egui_view_list_view_data_source_destroy_item_view(void *data_source_context, egui_view_t *view, uint16_t view_type)
{
    egui_view_list_view_t *local = (egui_view_list_view_t *)data_source_context;
    egui_view_list_view_item_host_t *host = egui_view_list_view_host_from_view(view);

    if (host == NULL)
    {
        return;
    }

    if (host->holder != NULL && host->holder->item_view != NULL && host->holder->item_view->parent == &host->root)
    {
        egui_view_group_remove_child(EGUI_VIEW_OF(&host->root), host->holder->item_view);
        egui_view_set_parent(host->holder->item_view, NULL);
    }

    egui_view_list_view_destroy_holder_internal(local, host->holder, view_type);
    host->holder = NULL;
    egui_free(EGUI_VIEW_OF(local)->core, host);
}

/** Resize the host and holder subtree, then invoke the caller's bind callback. */
static void egui_view_list_view_data_source_bind_item_view(void *data_source_context, egui_view_t *view, uint32_t index, uint32_t stable_id)
{
    egui_view_list_view_t *local = (egui_view_list_view_t *)data_source_context;
    egui_view_list_view_holder_t *holder = egui_view_list_view_holder_from_host_view(view);
    egui_dim_t item_width;
    int32_t item_height;

    if (holder == NULL)
    {
        return;
    }

    item_width = EGUI_VIEW_OF(local)->region.size.width;
    item_height = egui_view_virtual_list_get_item_height(EGUI_VIEW_OF(local), index);
    if (item_height <= 0)
    {
        item_height = egui_view_virtual_list_get_estimated_item_height(EGUI_VIEW_OF(local));
    }

    egui_view_set_size(view, item_width, (egui_dim_t)item_height);
    if (holder->item_view != NULL)
    {
        egui_view_set_position(holder->item_view, 0, 0);
        egui_view_set_size(holder->item_view, item_width, (egui_dim_t)item_height);
    }

    holder->bound_index = index;
    holder->bound_stable_id = stable_id;

    if (local->holder_ops != NULL && local->holder_ops->bind_holder != NULL)
    {
        local->holder_ops->bind_holder(local->data_model_context, holder, index, stable_id);
    }
}

/** Invoke the caller's unbind callback and clear the holder's current binding metadata. */
static void egui_view_list_view_data_source_unbind_item_view(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_list_view_t *local = (egui_view_list_view_t *)data_source_context;
    egui_view_list_view_holder_t *holder = egui_view_list_view_holder_from_host_view(view);

    if (holder == NULL)
    {
        return;
    }

    if (local->holder_ops != NULL && local->holder_ops->unbind_holder != NULL)
    {
        local->holder_ops->unbind_holder(local->data_model_context, holder, stable_id);
    }

    egui_view_list_view_reset_holder_binding(holder);
}

/** Ask caller code whether an off-screen holder should stay alive for reuse. */
static uint8_t egui_view_list_view_data_source_should_keep_alive(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_list_view_t *local = (egui_view_list_view_t *)data_source_context;
    egui_view_list_view_holder_t *holder = egui_view_list_view_holder_from_host_view(view);

    if (holder == NULL || local->holder_ops == NULL || local->holder_ops->should_keep_alive == NULL)
    {
        return 0;
    }

    return local->holder_ops->should_keep_alive(local->data_model_context, holder, stable_id);
}

/** Forward holder state saving into caller-provided lifecycle hooks. */
static void egui_view_list_view_data_source_save_item_state(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_list_view_t *local = (egui_view_list_view_t *)data_source_context;
    egui_view_list_view_holder_t *holder = egui_view_list_view_holder_from_host_view(view);

    if (holder == NULL || local->holder_ops == NULL || local->holder_ops->save_holder_state == NULL)
    {
        return;
    }

    local->holder_ops->save_holder_state(local->data_model_context, holder, stable_id);
}

/** Forward holder state restoration into caller-provided lifecycle hooks. */
static void egui_view_list_view_data_source_restore_item_state(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_list_view_t *local = (egui_view_list_view_t *)data_source_context;
    egui_view_list_view_holder_t *holder = egui_view_list_view_holder_from_host_view(view);

    if (holder == NULL || local->holder_ops == NULL || local->holder_ops->restore_holder_state == NULL)
    {
        return;
    }

    local->holder_ops->restore_holder_state(local->data_model_context, holder, stable_id);
}

/** Apply geometry parameters by forwarding directly to the virtual-list base widget. */
void egui_view_list_view_apply_params(egui_view_t *self, const egui_view_list_view_params_t *params)
{
    egui_view_virtual_list_apply_params(self, params);
}

/** Convenience helper that initializes the list view before applying params. */
void egui_view_list_view_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_list_view_params_t *params)
{
    egui_view_list_view_init(self, core);
    egui_view_list_view_apply_params(self, params);
}

/** Apply one setup bundle containing params, model hooks, and state-cache limits. */
void egui_view_list_view_apply_setup(egui_view_t *self, const egui_view_list_view_setup_t *setup)
{
    if (setup == NULL)
    {
        return;
    }

    if (setup->params != NULL)
    {
        egui_view_list_view_apply_params(self, setup->params);
    }

    egui_view_list_view_set_data_model(self, setup->data_model, setup->holder_ops, setup->data_model_context);
    egui_view_list_view_set_state_cache_limits(self, setup->state_cache_max_entries, setup->state_cache_max_bytes);
}

/** Convenience helper that initializes the list view before applying a setup bundle. */
void egui_view_list_view_init_with_setup(egui_view_t *self, egui_core_t *core, const egui_view_list_view_setup_t *setup)
{
    egui_view_list_view_init(self, core);
    egui_view_list_view_apply_setup(self, setup);
}

/** Attach or detach the data model, then rebuild the bridged virtual-list data source table. */
void egui_view_list_view_set_data_model(egui_view_t *self, const egui_view_list_view_data_model_t *data_model,
                                        const egui_view_list_view_holder_ops_t *holder_ops, void *data_model_context)
{
    EGUI_LOCAL_INIT(egui_view_list_view_t);

    local->data_model = data_model;
    local->holder_ops = holder_ops;
    local->data_model_context = data_model_context;

    if (data_model == NULL || holder_ops == NULL || holder_ops->create_holder == NULL)
    {
        egui_api_memset(&local->bridge_data_source, 0, sizeof(local->bridge_data_source));
        egui_view_virtual_list_set_data_source(self, NULL, NULL);
        return;
    }

    local->bridge_data_source.get_count = egui_view_list_view_data_source_get_count;
    local->bridge_data_source.get_stable_id = egui_view_list_view_data_source_get_stable_id;
    local->bridge_data_source.find_index_by_stable_id = egui_view_list_view_data_source_find_index_by_stable_id;
    local->bridge_data_source.get_view_type = egui_view_list_view_data_source_get_view_type;
    local->bridge_data_source.measure_item_height = egui_view_list_view_data_source_measure_item_height;
    local->bridge_data_source.create_item_view = egui_view_list_view_data_source_create_item_view;
    local->bridge_data_source.destroy_item_view = egui_view_list_view_data_source_destroy_item_view;
    local->bridge_data_source.bind_item_view = egui_view_list_view_data_source_bind_item_view;
    local->bridge_data_source.unbind_item_view = egui_view_list_view_data_source_unbind_item_view;
    local->bridge_data_source.should_keep_alive = egui_view_list_view_data_source_should_keep_alive;
    local->bridge_data_source.save_item_state = egui_view_list_view_data_source_save_item_state;
    local->bridge_data_source.restore_item_state = egui_view_list_view_data_source_restore_item_state;
    local->bridge_data_source.default_view_type = data_model->default_view_type;

    egui_view_virtual_list_set_data_source(self, &local->bridge_data_source, local);
}

/** Return the currently attached data-model descriptor. */
const egui_view_list_view_data_model_t *egui_view_list_view_get_data_model(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_list_view_t);
    return local->data_model;
}

/** Return the currently attached holder-ops descriptor. */
const egui_view_list_view_holder_ops_t *egui_view_list_view_get_holder_ops(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_list_view_t);
    return local->holder_ops;
}

/** Return the opaque callback context currently stored on the list view. */
void *egui_view_list_view_get_data_model_context(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_list_view_t);
    return local->data_model_context;
}

/** Return the current item count reported by the underlying virtual list. */
uint32_t egui_view_list_view_get_item_count(egui_view_t *self)
{
    return egui_view_virtual_list_get_item_count(self);
}

/** Forward overscan configuration to the virtual-list base widget. */
void egui_view_list_view_set_overscan(egui_view_t *self, uint8_t before, uint8_t after)
{
    egui_view_virtual_list_set_overscan(self, before, after);
}

/** Return the leading overscan count from the virtual-list base widget. */
uint8_t egui_view_list_view_get_overscan_before(egui_view_t *self)
{
    return egui_view_virtual_list_get_overscan_before(self);
}

/** Return the trailing overscan count from the virtual-list base widget. */
uint8_t egui_view_list_view_get_overscan_after(egui_view_t *self)
{
    return egui_view_virtual_list_get_overscan_after(self);
}

/** Set the fallback item-height estimate used before real measurement is known. */
void egui_view_list_view_set_estimated_item_height(egui_view_t *self, int32_t height)
{
    egui_view_virtual_list_set_estimated_item_height(self, height);
}

/** Return the current fallback item-height estimate. */
int32_t egui_view_list_view_get_estimated_item_height(egui_view_t *self)
{
    return egui_view_virtual_list_get_estimated_item_height(self);
}

/** Set the keepalive-holder limit on the virtual-list base widget. */
void egui_view_list_view_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots)
{
    egui_view_virtual_list_set_keepalive_limit(self, max_keepalive_slots);
}

/** Return the keepalive-holder limit from the virtual-list base widget. */
uint8_t egui_view_list_view_get_keepalive_limit(egui_view_t *self)
{
    return egui_view_virtual_list_get_keepalive_limit(self);
}

/** Configure per-item state-cache limits on the virtual-list base widget. */
void egui_view_list_view_set_state_cache_limits(egui_view_t *self, uint16_t max_entries, uint32_t max_bytes)
{
    egui_view_virtual_list_set_state_cache_limits(self, max_entries, max_bytes);
}

/** Return the cached-state entry limit from the virtual-list base widget. */
uint16_t egui_view_list_view_get_state_cache_entry_limit(egui_view_t *self)
{
    return egui_view_virtual_list_get_state_cache_entry_limit(self);
}

/** Return the cached-state byte limit from the virtual-list base widget. */
uint32_t egui_view_list_view_get_state_cache_byte_limit(egui_view_t *self)
{
    return egui_view_virtual_list_get_state_cache_byte_limit(self);
}

/** Drop all cached item state stored by the virtual-list base widget. */
void egui_view_list_view_clear_item_state_cache(egui_view_t *self)
{
    egui_view_virtual_list_clear_item_state_cache(self);
}

/** Remove cached state for one stable id. */
void egui_view_list_view_remove_item_state_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    egui_view_virtual_list_remove_item_state_by_stable_id(self, stable_id);
}

/** Write custom state bytes for one stable id through the virtual-list state cache. */
uint8_t egui_view_list_view_write_item_state(egui_view_t *self, uint32_t stable_id, const void *data, uint16_t size)
{
    return egui_view_virtual_list_write_item_state(self, stable_id, data, size);
}

/** Read cached state bytes for one stable id through the virtual-list state cache. */
uint16_t egui_view_list_view_read_item_state(egui_view_t *self, uint32_t stable_id, void *data, uint16_t capacity)
{
    return egui_view_virtual_list_read_item_state(self, stable_id, data, capacity);
}

/** Convenience helper that writes cached state for the item containing `item_view`. */
uint8_t egui_view_list_view_write_item_state_for_view(egui_view_t *item_view, uint32_t stable_id, const void *data, uint16_t size)
{
    return egui_view_virtual_list_write_item_state_for_view(item_view, stable_id, data, size);
}

/** Convenience helper that reads cached state for the item containing `item_view`. */
uint16_t egui_view_list_view_read_item_state_for_view(egui_view_t *item_view, uint32_t stable_id, void *data, uint16_t capacity)
{
    return egui_view_virtual_list_read_item_state_for_view(item_view, stable_id, data, capacity);
}

/** Jump to an absolute vertical scroll offset. */
void egui_view_list_view_set_scroll_y(egui_view_t *self, int32_t offset)
{
    egui_view_virtual_list_set_scroll_y(self, offset);
}

/** Scroll the viewport by a signed delta. */
void egui_view_list_view_scroll_by(egui_view_t *self, int32_t delta)
{
    egui_view_virtual_list_scroll_by(self, delta);
}

/** Scroll so the target index appears at the requested offset inside the viewport. */
void egui_view_list_view_scroll_to_item(egui_view_t *self, uint32_t index, int32_t item_offset)
{
    egui_view_virtual_list_scroll_to_item(self, index, item_offset);
}

/** Scroll to an item identified by stable id instead of transient index. */
void egui_view_list_view_scroll_to_stable_id(egui_view_t *self, uint32_t stable_id, int32_t item_offset)
{
    egui_view_virtual_list_scroll_to_stable_id(self, stable_id, item_offset);
}

/** Return the current vertical scroll offset. */
int32_t egui_view_list_view_get_scroll_y(egui_view_t *self)
{
    return egui_view_virtual_list_get_scroll_y(self);
}

/** Resolve the current index for one stable id. */
int32_t egui_view_list_view_find_index_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    return egui_view_virtual_list_find_index_by_stable_id(self, stable_id);
}

/** Resolve entry metadata for one stable id. */
uint8_t egui_view_list_view_resolve_item_by_stable_id(egui_view_t *self, uint32_t stable_id, egui_view_list_view_entry_t *entry)
{
    return egui_view_virtual_list_resolve_item_by_stable_id(self, stable_id, entry);
}

/** Walk up from any descendant view until a realized list item entry can be resolved. */
uint8_t egui_view_list_view_resolve_item_by_view(egui_view_t *self, egui_view_t *item_view, egui_view_list_view_entry_t *entry)
{
    egui_view_t *cursor = item_view;

    egui_view_list_view_reset_entry(entry);
    while (cursor != NULL)
    {
        if (egui_view_virtual_list_resolve_item_by_view(self, cursor, entry))
        {
            return 1;
        }
        cursor = EGUI_VIEW_PARENT(cursor);
    }

    return 0;
}

/** Return the top Y coordinate of one item index. */
int32_t egui_view_list_view_get_item_y(egui_view_t *self, uint32_t index)
{
    return egui_view_virtual_list_get_item_y(self, index);
}

/** Return the measured or estimated height of one item index. */
int32_t egui_view_list_view_get_item_height(egui_view_t *self, uint32_t index)
{
    return egui_view_virtual_list_get_item_height(self, index);
}

/** Return the top Y coordinate of one stable-id item. */
int32_t egui_view_list_view_get_item_y_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    return egui_view_virtual_list_get_item_y_by_stable_id(self, stable_id);
}

/** Return the measured or estimated height of one stable-id item. */
int32_t egui_view_list_view_get_item_height_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    return egui_view_virtual_list_get_item_height_by_stable_id(self, stable_id);
}

/** Scroll just enough to keep the target stable id visible with the requested inset. */
uint8_t egui_view_list_view_ensure_item_visible_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t inset)
{
    return egui_view_virtual_list_ensure_item_visible_by_stable_id(self, stable_id, inset);
}

/** Notify the virtual list that the whole data set changed. */
void egui_view_list_view_notify_data_changed(egui_view_t *self)
{
    egui_view_virtual_list_notify_data_changed(self);
}

/** Notify that one item changed in place by index. */
void egui_view_list_view_notify_item_changed(egui_view_t *self, uint32_t index)
{
    egui_view_virtual_list_notify_item_changed(self, index);
}

/** Notify that one item changed in place by stable id. */
void egui_view_list_view_notify_item_changed_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    egui_view_virtual_list_notify_item_changed_by_stable_id(self, stable_id);
}

/** Notify that a range of items was inserted. */
void egui_view_list_view_notify_item_inserted(egui_view_t *self, uint32_t index, uint32_t count)
{
    egui_view_virtual_list_notify_item_inserted(self, index, count);
}

/** Notify that a range of items was removed. */
void egui_view_list_view_notify_item_removed(egui_view_t *self, uint32_t index, uint32_t count)
{
    egui_view_virtual_list_notify_item_removed(self, index, count);
}

/** Notify that one item moved to a different index. */
void egui_view_list_view_notify_item_moved(egui_view_t *self, uint32_t from_index, uint32_t to_index)
{
    egui_view_virtual_list_notify_item_moved(self, from_index, to_index);
}

/** Notify that one item's measured height changed by index. */
void egui_view_list_view_notify_item_resized(egui_view_t *self, uint32_t index)
{
    egui_view_virtual_list_notify_item_resized(self, index);
}

/** Notify that one item's measured height changed by stable id. */
void egui_view_list_view_notify_item_resized_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    egui_view_virtual_list_notify_item_resized_by_stable_id(self, stable_id);
}

/** Return the currently realized holder for one stable id, if it is active. */
egui_view_list_view_holder_t *egui_view_list_view_find_holder_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    return egui_view_list_view_holder_from_host_view(egui_view_virtual_list_find_view_by_stable_id(self, stable_id));
}

/** Return the holder's real item view for one stable id, if it is active. */
egui_view_t *egui_view_list_view_find_item_view_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    egui_view_list_view_holder_t *holder = egui_view_list_view_find_holder_by_stable_id(self, stable_id);
    return holder != NULL ? holder->item_view : NULL;
}

/** Resolve both holder and entry metadata by walking up from any descendant view. */
uint8_t egui_view_list_view_resolve_holder_by_view(egui_view_t *self, egui_view_t *item_view, egui_view_list_view_holder_t **holder_out,
                                                   egui_view_list_view_entry_t *entry_out)
{
    egui_view_list_view_entry_t entry;
    egui_view_list_view_holder_t *holder;

    if (!egui_view_list_view_resolve_item_by_view(self, item_view, &entry))
    {
        if (holder_out != NULL)
        {
            *holder_out = NULL;
        }
        egui_view_list_view_reset_entry(entry_out);
        return 0;
    }

    holder = egui_view_list_view_find_holder_by_stable_id(self, entry.stable_id);
    if (holder_out != NULL)
    {
        *holder_out = holder;
    }
    if (entry_out != NULL)
    {
        *entry_out = entry;
    }
    return holder != NULL;
}

/** Initialize the list view as a virtual list plus an empty bridged data-source table. */
void egui_view_list_view_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_LOCAL_INIT(egui_view_list_view_t);

    egui_view_virtual_list_init(self, core);
    local->data_model = NULL;
    local->holder_ops = NULL;
    local->data_model_context = NULL;
    egui_api_memset(&local->bridge_data_source, 0, sizeof(local->bridge_data_source));
    egui_view_set_view_name(self, "egui_view_list_view");
}
