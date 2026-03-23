#include "egui_view_grid_view.h"

#include <string.h>

#include "core/egui_common.h"

typedef struct egui_view_grid_view_item_host egui_view_grid_view_item_host_t;

struct egui_view_grid_view_item_host
{
    egui_view_group_t root;
    egui_view_grid_view_holder_t *holder;
};

static uint32_t egui_view_grid_view_default_stable_id(uint32_t index)
{
    return index + 1U;
}

static void egui_view_grid_view_reset_entry(egui_view_grid_view_entry_t *entry)
{
    if (entry == NULL)
    {
        return;
    }

    entry->index = EGUI_VIEW_VIRTUAL_GRID_INVALID_INDEX;
    entry->row_index = EGUI_VIEW_VIRTUAL_GRID_INVALID_INDEX;
    entry->column_index = 0;
    entry->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static void egui_view_grid_view_reset_holder_binding(egui_view_grid_view_holder_t *holder)
{
    if (holder == NULL)
    {
        return;
    }

    holder->bound_index = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    holder->bound_stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
}

static egui_view_grid_view_item_host_t *egui_view_grid_view_host_from_view(egui_view_t *view)
{
    return (egui_view_grid_view_item_host_t *)view;
}

static egui_view_grid_view_holder_t *egui_view_grid_view_holder_from_host_view(egui_view_t *host_view)
{
    egui_view_grid_view_item_host_t *host;

    if (host_view == NULL)
    {
        return NULL;
    }

    host = egui_view_grid_view_host_from_view(host_view);
    return host->holder;
}

static const egui_view_grid_view_data_model_t *egui_view_grid_view_get_data_model_bridge(egui_view_grid_view_t *local, void **data_model_context)
{
    if (data_model_context != NULL)
    {
        *data_model_context = local->data_model_context;
    }

    return local->data_model;
}

static void egui_view_grid_view_destroy_holder_internal(egui_view_grid_view_t *local, egui_view_grid_view_holder_t *holder, uint16_t view_type)
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

    egui_free(holder);
}

static uint32_t egui_view_grid_view_data_source_get_count(void *data_source_context)
{
    egui_view_grid_view_t *local = (egui_view_grid_view_t *)data_source_context;
    void *model_context;
    const egui_view_grid_view_data_model_t *data_model = egui_view_grid_view_get_data_model_bridge(local, &model_context);

    if (data_model == NULL || data_model->get_count == NULL)
    {
        return 0;
    }

    return data_model->get_count(model_context);
}

static uint32_t egui_view_grid_view_data_source_get_stable_id(void *data_source_context, uint32_t index)
{
    egui_view_grid_view_t *local = (egui_view_grid_view_t *)data_source_context;
    void *model_context;
    const egui_view_grid_view_data_model_t *data_model = egui_view_grid_view_get_data_model_bridge(local, &model_context);

    if (data_model != NULL && data_model->get_stable_id != NULL)
    {
        return data_model->get_stable_id(model_context, index);
    }

    return egui_view_grid_view_default_stable_id(index);
}

static int32_t egui_view_grid_view_data_source_find_index_by_stable_id(void *data_source_context, uint32_t stable_id)
{
    egui_view_grid_view_t *local = (egui_view_grid_view_t *)data_source_context;
    void *model_context;
    const egui_view_grid_view_data_model_t *data_model = egui_view_grid_view_get_data_model_bridge(local, &model_context);
    uint32_t count;
    uint32_t index;

    if (data_model != NULL && data_model->find_index_by_stable_id != NULL)
    {
        return data_model->find_index_by_stable_id(model_context, stable_id);
    }

    count = egui_view_grid_view_data_source_get_count(data_source_context);
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

static uint16_t egui_view_grid_view_data_source_get_view_type(void *data_source_context, uint32_t index)
{
    egui_view_grid_view_t *local = (egui_view_grid_view_t *)data_source_context;
    void *model_context;
    const egui_view_grid_view_data_model_t *data_model = egui_view_grid_view_get_data_model_bridge(local, &model_context);

    if (data_model != NULL && data_model->get_view_type != NULL)
    {
        return data_model->get_view_type(model_context, index);
    }

    return data_model != NULL ? data_model->default_view_type : 0;
}

static int32_t egui_view_grid_view_data_source_measure_item_height(void *data_source_context, uint32_t index, int32_t width_hint)
{
    egui_view_grid_view_t *local = (egui_view_grid_view_t *)data_source_context;
    void *model_context;
    const egui_view_grid_view_data_model_t *data_model = egui_view_grid_view_get_data_model_bridge(local, &model_context);

    if (data_model != NULL && data_model->measure_item_height != NULL)
    {
        return data_model->measure_item_height(model_context, index, width_hint);
    }

    return egui_view_virtual_grid_get_estimated_item_height(EGUI_VIEW_OF(local));
}

static egui_view_t *egui_view_grid_view_data_source_create_item_view(void *data_source_context, uint16_t view_type)
{
    egui_view_grid_view_t *local = (egui_view_grid_view_t *)data_source_context;
    egui_view_grid_view_item_host_t *host;
    egui_view_grid_view_holder_t *holder;

    if (local->holder_ops == NULL || local->holder_ops->create_holder == NULL)
    {
        return NULL;
    }

    holder = local->holder_ops->create_holder(local->data_model_context, view_type);
    if (holder == NULL || holder->item_view == NULL)
    {
        if (holder != NULL)
        {
            egui_view_grid_view_destroy_holder_internal(local, holder, view_type);
        }
        return NULL;
    }

    host = (egui_view_grid_view_item_host_t *)egui_malloc(sizeof(egui_view_grid_view_item_host_t));
    if (host == NULL)
    {
        egui_view_grid_view_destroy_holder_internal(local, holder, view_type);
        return NULL;
    }

    memset(host, 0, sizeof(*host));
    egui_view_group_init(EGUI_VIEW_OF(&host->root));
    egui_view_set_view_name(EGUI_VIEW_OF(&host->root), "egui_view_grid_view_item");

    host->holder = holder;
    holder->host_view = EGUI_VIEW_OF(&host->root);
    holder->view_type = view_type;
    egui_view_grid_view_reset_holder_binding(holder);

    egui_view_group_add_child(EGUI_VIEW_OF(&host->root), holder->item_view);
    return EGUI_VIEW_OF(&host->root);
}

static void egui_view_grid_view_data_source_destroy_item_view(void *data_source_context, egui_view_t *view, uint16_t view_type)
{
    egui_view_grid_view_t *local = (egui_view_grid_view_t *)data_source_context;
    egui_view_grid_view_item_host_t *host = egui_view_grid_view_host_from_view(view);

    if (host == NULL)
    {
        return;
    }

    if (host->holder != NULL && host->holder->item_view != NULL && host->holder->item_view->parent == &host->root)
    {
        egui_view_group_remove_child(EGUI_VIEW_OF(&host->root), host->holder->item_view);
        egui_view_set_parent(host->holder->item_view, NULL);
    }

    egui_view_grid_view_destroy_holder_internal(local, host->holder, view_type);
    host->holder = NULL;
    egui_free(host);
}

static void egui_view_grid_view_data_source_bind_item_view(void *data_source_context, egui_view_t *view, uint32_t index, uint32_t stable_id)
{
    egui_view_grid_view_t *local = (egui_view_grid_view_t *)data_source_context;
    egui_view_grid_view_holder_t *holder = egui_view_grid_view_holder_from_host_view(view);
    egui_dim_t item_width;
    int32_t item_height;
    uint8_t column_count;
    egui_dim_t column_spacing;

    if (holder == NULL)
    {
        return;
    }

    item_width = (egui_dim_t)egui_view_virtual_grid_get_item_width(EGUI_VIEW_OF(local), index);
    item_height = egui_view_virtual_grid_get_item_height(EGUI_VIEW_OF(local), index);
    if (item_width <= 0)
    {
        column_count = egui_view_virtual_grid_get_column_count(EGUI_VIEW_OF(local));
        column_spacing = egui_view_virtual_grid_get_column_spacing(EGUI_VIEW_OF(local));
        if (column_count == 0)
        {
            column_count = 1;
        }
        item_width = (egui_dim_t)((EGUI_VIEW_OF(local)->region.size.width - (column_count - 1U) * column_spacing) / column_count);
    }
    if (item_height <= 0)
    {
        item_height = egui_view_virtual_grid_get_estimated_item_height(EGUI_VIEW_OF(local));
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

static void egui_view_grid_view_data_source_unbind_item_view(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_grid_view_t *local = (egui_view_grid_view_t *)data_source_context;
    egui_view_grid_view_holder_t *holder = egui_view_grid_view_holder_from_host_view(view);

    if (holder == NULL)
    {
        return;
    }

    if (local->holder_ops != NULL && local->holder_ops->unbind_holder != NULL)
    {
        local->holder_ops->unbind_holder(local->data_model_context, holder, stable_id);
    }

    egui_view_grid_view_reset_holder_binding(holder);
}

static uint8_t egui_view_grid_view_data_source_should_keep_alive(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_grid_view_t *local = (egui_view_grid_view_t *)data_source_context;
    egui_view_grid_view_holder_t *holder = egui_view_grid_view_holder_from_host_view(view);

    if (holder == NULL || local->holder_ops == NULL || local->holder_ops->should_keep_alive == NULL)
    {
        return 0;
    }

    return local->holder_ops->should_keep_alive(local->data_model_context, holder, stable_id);
}

static void egui_view_grid_view_data_source_save_item_state(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_grid_view_t *local = (egui_view_grid_view_t *)data_source_context;
    egui_view_grid_view_holder_t *holder = egui_view_grid_view_holder_from_host_view(view);

    if (holder == NULL || local->holder_ops == NULL || local->holder_ops->save_holder_state == NULL)
    {
        return;
    }

    local->holder_ops->save_holder_state(local->data_model_context, holder, stable_id);
}

static void egui_view_grid_view_data_source_restore_item_state(void *data_source_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_grid_view_t *local = (egui_view_grid_view_t *)data_source_context;
    egui_view_grid_view_holder_t *holder = egui_view_grid_view_holder_from_host_view(view);

    if (holder == NULL || local->holder_ops == NULL || local->holder_ops->restore_holder_state == NULL)
    {
        return;
    }

    local->holder_ops->restore_holder_state(local->data_model_context, holder, stable_id);
}

void egui_view_grid_view_apply_params(egui_view_t *self, const egui_view_grid_view_params_t *params)
{
    egui_view_virtual_grid_apply_params(self, params);
}

void egui_view_grid_view_init_with_params(egui_view_t *self, const egui_view_grid_view_params_t *params)
{
    egui_view_grid_view_init(self);
    egui_view_grid_view_apply_params(self, params);
}

void egui_view_grid_view_apply_setup(egui_view_t *self, const egui_view_grid_view_setup_t *setup)
{
    if (setup == NULL)
    {
        return;
    }

    if (setup->params != NULL)
    {
        egui_view_grid_view_apply_params(self, setup->params);
    }

    egui_view_grid_view_set_data_model(self, setup->data_model, setup->holder_ops, setup->data_model_context);
    egui_view_grid_view_set_state_cache_limits(self, setup->state_cache_max_entries, setup->state_cache_max_bytes);
}

void egui_view_grid_view_init_with_setup(egui_view_t *self, const egui_view_grid_view_setup_t *setup)
{
    egui_view_grid_view_init(self);
    egui_view_grid_view_apply_setup(self, setup);
}

void egui_view_grid_view_set_data_model(egui_view_t *self, const egui_view_grid_view_data_model_t *data_model,
                                        const egui_view_grid_view_holder_ops_t *holder_ops, void *data_model_context)
{
    EGUI_LOCAL_INIT(egui_view_grid_view_t);

    local->data_model = data_model;
    local->holder_ops = holder_ops;
    local->data_model_context = data_model_context;

    if (data_model == NULL || holder_ops == NULL || holder_ops->create_holder == NULL)
    {
        memset(&local->bridge_data_source, 0, sizeof(local->bridge_data_source));
        egui_view_virtual_grid_set_data_source(self, NULL, NULL);
        return;
    }

    local->bridge_data_source.get_count = egui_view_grid_view_data_source_get_count;
    local->bridge_data_source.get_stable_id = egui_view_grid_view_data_source_get_stable_id;
    local->bridge_data_source.find_index_by_stable_id = egui_view_grid_view_data_source_find_index_by_stable_id;
    local->bridge_data_source.get_item_view_type = egui_view_grid_view_data_source_get_view_type;
    local->bridge_data_source.measure_item_height = egui_view_grid_view_data_source_measure_item_height;
    local->bridge_data_source.create_item_view = egui_view_grid_view_data_source_create_item_view;
    local->bridge_data_source.destroy_item_view = egui_view_grid_view_data_source_destroy_item_view;
    local->bridge_data_source.bind_item_view = egui_view_grid_view_data_source_bind_item_view;
    local->bridge_data_source.unbind_item_view = egui_view_grid_view_data_source_unbind_item_view;
    local->bridge_data_source.should_keep_alive = egui_view_grid_view_data_source_should_keep_alive;
    local->bridge_data_source.save_item_state = egui_view_grid_view_data_source_save_item_state;
    local->bridge_data_source.restore_item_state = egui_view_grid_view_data_source_restore_item_state;
    local->bridge_data_source.default_item_view_type = data_model->default_view_type;

    egui_view_virtual_grid_set_data_source(self, &local->bridge_data_source, local);
}

const egui_view_grid_view_data_model_t *egui_view_grid_view_get_data_model(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_grid_view_t);
    return local->data_model;
}

const egui_view_grid_view_holder_ops_t *egui_view_grid_view_get_holder_ops(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_grid_view_t);
    return local->holder_ops;
}

void *egui_view_grid_view_get_data_model_context(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_grid_view_t);
    return local->data_model_context;
}

uint32_t egui_view_grid_view_get_item_count(egui_view_t *self)
{
    return egui_view_virtual_grid_get_item_count(self);
}

uint32_t egui_view_grid_view_get_row_count(egui_view_t *self)
{
    return egui_view_virtual_grid_get_row_count(self);
}

void egui_view_grid_view_set_overscan(egui_view_t *self, uint8_t before, uint8_t after)
{
    egui_view_virtual_grid_set_overscan(self, before, after);
}

uint8_t egui_view_grid_view_get_overscan_before(egui_view_t *self)
{
    return egui_view_virtual_grid_get_overscan_before(self);
}

uint8_t egui_view_grid_view_get_overscan_after(egui_view_t *self)
{
    return egui_view_virtual_grid_get_overscan_after(self);
}

void egui_view_grid_view_set_column_count(egui_view_t *self, uint8_t column_count)
{
    egui_view_virtual_grid_set_column_count(self, column_count);
}

uint8_t egui_view_grid_view_get_column_count(egui_view_t *self)
{
    return egui_view_virtual_grid_get_column_count(self);
}

void egui_view_grid_view_set_spacing(egui_view_t *self, egui_dim_t column_spacing, egui_dim_t row_spacing)
{
    egui_view_virtual_grid_set_spacing(self, column_spacing, row_spacing);
}

egui_dim_t egui_view_grid_view_get_column_spacing(egui_view_t *self)
{
    return egui_view_virtual_grid_get_column_spacing(self);
}

egui_dim_t egui_view_grid_view_get_row_spacing(egui_view_t *self)
{
    return egui_view_virtual_grid_get_row_spacing(self);
}

void egui_view_grid_view_set_estimated_item_height(egui_view_t *self, int32_t height)
{
    egui_view_virtual_grid_set_estimated_item_height(self, height);
}

int32_t egui_view_grid_view_get_estimated_item_height(egui_view_t *self)
{
    return egui_view_virtual_grid_get_estimated_item_height(self);
}

void egui_view_grid_view_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots)
{
    egui_view_virtual_grid_set_keepalive_limit(self, max_keepalive_slots);
}

uint8_t egui_view_grid_view_get_keepalive_limit(egui_view_t *self)
{
    return egui_view_virtual_grid_get_keepalive_limit(self);
}

void egui_view_grid_view_set_state_cache_limits(egui_view_t *self, uint16_t max_entries, uint32_t max_bytes)
{
    egui_view_virtual_grid_set_state_cache_limits(self, max_entries, max_bytes);
}

uint16_t egui_view_grid_view_get_state_cache_entry_limit(egui_view_t *self)
{
    return egui_view_virtual_grid_get_state_cache_entry_limit(self);
}

uint32_t egui_view_grid_view_get_state_cache_byte_limit(egui_view_t *self)
{
    return egui_view_virtual_grid_get_state_cache_byte_limit(self);
}

void egui_view_grid_view_clear_item_state_cache(egui_view_t *self)
{
    egui_view_virtual_grid_clear_item_state_cache(self);
}

void egui_view_grid_view_remove_item_state_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    egui_view_virtual_grid_remove_item_state_by_stable_id(self, stable_id);
}

uint8_t egui_view_grid_view_write_item_state(egui_view_t *self, uint32_t stable_id, const void *data, uint16_t size)
{
    return egui_view_virtual_grid_write_item_state(self, stable_id, data, size);
}

uint16_t egui_view_grid_view_read_item_state(egui_view_t *self, uint32_t stable_id, void *data, uint16_t capacity)
{
    return egui_view_virtual_grid_read_item_state(self, stable_id, data, capacity);
}

uint8_t egui_view_grid_view_write_item_state_for_view(egui_view_t *item_view, uint32_t stable_id, const void *data, uint16_t size)
{
    return egui_view_virtual_grid_write_item_state_for_view(item_view, stable_id, data, size);
}

uint16_t egui_view_grid_view_read_item_state_for_view(egui_view_t *item_view, uint32_t stable_id, void *data, uint16_t capacity)
{
    return egui_view_virtual_grid_read_item_state_for_view(item_view, stable_id, data, capacity);
}

void egui_view_grid_view_set_scroll_y(egui_view_t *self, int32_t offset)
{
    egui_view_virtual_grid_set_scroll_y(self, offset);
}

void egui_view_grid_view_scroll_by(egui_view_t *self, int32_t delta)
{
    egui_view_virtual_grid_scroll_by(self, delta);
}

void egui_view_grid_view_scroll_to_item(egui_view_t *self, uint32_t index, int32_t item_offset)
{
    egui_view_virtual_grid_scroll_to_item(self, index, item_offset);
}

void egui_view_grid_view_scroll_to_stable_id(egui_view_t *self, uint32_t stable_id, int32_t item_offset)
{
    egui_view_virtual_grid_scroll_to_item_by_stable_id(self, stable_id, item_offset);
}

int32_t egui_view_grid_view_get_scroll_y(egui_view_t *self)
{
    return egui_view_virtual_grid_get_scroll_y(self);
}

int32_t egui_view_grid_view_find_index_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    return egui_view_virtual_grid_find_index_by_stable_id(self, stable_id);
}

uint8_t egui_view_grid_view_resolve_item_by_stable_id(egui_view_t *self, uint32_t stable_id, egui_view_grid_view_entry_t *entry)
{
    return egui_view_virtual_grid_resolve_item_by_stable_id(self, stable_id, entry);
}

uint8_t egui_view_grid_view_resolve_item_by_view(egui_view_t *self, egui_view_t *item_view, egui_view_grid_view_entry_t *entry)
{
    egui_view_t *cursor = item_view;

    egui_view_grid_view_reset_entry(entry);
    while (cursor != NULL)
    {
        if (egui_view_virtual_grid_resolve_item_by_view(self, cursor, entry))
        {
            return 1;
        }
        cursor = EGUI_VIEW_PARENT(cursor);
    }

    return 0;
}

int32_t egui_view_grid_view_get_item_x(egui_view_t *self, uint32_t index)
{
    return egui_view_virtual_grid_get_item_x(self, index);
}

int32_t egui_view_grid_view_get_item_y(egui_view_t *self, uint32_t index)
{
    return egui_view_virtual_grid_get_item_y(self, index);
}

int32_t egui_view_grid_view_get_item_width(egui_view_t *self, uint32_t index)
{
    return egui_view_virtual_grid_get_item_width(self, index);
}

int32_t egui_view_grid_view_get_item_height(egui_view_t *self, uint32_t index)
{
    return egui_view_virtual_grid_get_item_height(self, index);
}

int32_t egui_view_grid_view_get_item_x_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    return egui_view_virtual_grid_get_item_x_by_stable_id(self, stable_id);
}

int32_t egui_view_grid_view_get_item_y_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    return egui_view_virtual_grid_get_item_y_by_stable_id(self, stable_id);
}

int32_t egui_view_grid_view_get_item_width_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    return egui_view_virtual_grid_get_item_width_by_stable_id(self, stable_id);
}

int32_t egui_view_grid_view_get_item_height_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    return egui_view_virtual_grid_get_item_height_by_stable_id(self, stable_id);
}

uint8_t egui_view_grid_view_ensure_item_visible_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t inset)
{
    return egui_view_virtual_grid_ensure_item_visible_by_stable_id(self, stable_id, inset);
}

void egui_view_grid_view_notify_data_changed(egui_view_t *self)
{
    egui_view_virtual_grid_notify_data_changed(self);
}

void egui_view_grid_view_notify_item_changed(egui_view_t *self, uint32_t index)
{
    egui_view_virtual_grid_notify_item_changed(self, index);
}

void egui_view_grid_view_notify_item_changed_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    egui_view_virtual_grid_notify_item_changed_by_stable_id(self, stable_id);
}

void egui_view_grid_view_notify_item_inserted(egui_view_t *self, uint32_t index, uint32_t count)
{
    egui_view_virtual_grid_notify_item_inserted(self, index, count);
}

void egui_view_grid_view_notify_item_removed(egui_view_t *self, uint32_t index, uint32_t count)
{
    egui_view_virtual_grid_notify_item_removed(self, index, count);
}

void egui_view_grid_view_notify_item_moved(egui_view_t *self, uint32_t from_index, uint32_t to_index)
{
    egui_view_virtual_grid_notify_item_moved(self, from_index, to_index);
}

void egui_view_grid_view_notify_item_resized(egui_view_t *self, uint32_t index)
{
    egui_view_virtual_grid_notify_item_resized(self, index);
}

void egui_view_grid_view_notify_item_resized_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    egui_view_virtual_grid_notify_item_resized_by_stable_id(self, stable_id);
}

egui_view_grid_view_holder_t *egui_view_grid_view_find_holder_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    return egui_view_grid_view_holder_from_host_view(egui_view_virtual_grid_find_view_by_stable_id(self, stable_id));
}

egui_view_t *egui_view_grid_view_find_item_view_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    egui_view_grid_view_holder_t *holder = egui_view_grid_view_find_holder_by_stable_id(self, stable_id);
    return holder != NULL ? holder->item_view : NULL;
}

uint8_t egui_view_grid_view_resolve_holder_by_view(egui_view_t *self, egui_view_t *item_view, egui_view_grid_view_holder_t **holder_out,
                                                   egui_view_grid_view_entry_t *entry_out)
{
    egui_view_grid_view_entry_t entry;
    egui_view_grid_view_holder_t *holder;

    if (!egui_view_grid_view_resolve_item_by_view(self, item_view, &entry))
    {
        if (holder_out != NULL)
        {
            *holder_out = NULL;
        }
        egui_view_grid_view_reset_entry(entry_out);
        return 0;
    }

    holder = egui_view_grid_view_find_holder_by_stable_id(self, entry.stable_id);
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

void egui_view_grid_view_init(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_grid_view_t);

    egui_view_virtual_grid_init(self);
    local->data_model = NULL;
    local->holder_ops = NULL;
    local->data_model_context = NULL;
    memset(&local->bridge_data_source, 0, sizeof(local->bridge_data_source));
    egui_view_set_view_name(self, "egui_view_grid_view");
}
