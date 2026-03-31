#include "egui_view_virtual_grid.h"

#include <string.h>

typedef struct egui_view_virtual_grid_row_cell egui_view_virtual_grid_row_cell_t;
typedef struct egui_view_virtual_grid_row_view egui_view_virtual_grid_row_view_t;

struct egui_view_virtual_grid_row_cell
{
    egui_view_t *view;
    uint16_t view_type;
    uint32_t index;
    uint32_t stable_id;
    int32_t height;
    uint8_t is_bound;
};

struct egui_view_virtual_grid_row_view
{
    egui_view_group_t root;
    uint32_t row_index;
    uint32_t stable_id;
    uint8_t bound_count;
    egui_view_virtual_grid_row_cell_t cells[EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS];
};

static int32_t egui_view_virtual_grid_get_column_width_internal(egui_view_t *self, egui_view_virtual_grid_t *local, uint8_t column_index);
static int32_t egui_view_virtual_grid_get_column_x_internal(egui_view_t *self, egui_view_virtual_grid_t *local, uint8_t column_index);

static egui_view_virtual_grid_t *egui_view_virtual_grid_from_row_context(void *adapter_context)
{
    return (egui_view_virtual_grid_t *)adapter_context;
}

static const egui_view_virtual_grid_data_source_t *egui_view_virtual_grid_get_data_source_bridge(egui_view_virtual_grid_t *local, void **data_source_context)
{
    if (data_source_context != NULL)
    {
        *data_source_context = local->data_source_context;
    }

    return local->data_source;
}

static uint8_t egui_view_virtual_grid_sanitize_column_count(uint8_t column_count)
{
    if (column_count == 0)
    {
        return 1;
    }

    return column_count > EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS ? EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS : column_count;
}

static egui_dim_t egui_view_virtual_grid_sanitize_spacing(egui_dim_t spacing)
{
    return spacing < 0 ? 0 : spacing;
}

static int32_t egui_view_virtual_grid_sanitize_item_height(int32_t height)
{
    return height > 0 ? height : 1;
}

static uint8_t egui_view_virtual_grid_get_column_count_internal(egui_view_virtual_grid_t *local)
{
    return egui_view_virtual_grid_sanitize_column_count(local->column_count);
}

static uint32_t egui_view_virtual_grid_default_stable_id(uint32_t index)
{
    return index + 1U;
}

static void egui_view_virtual_grid_reset_entry(egui_view_virtual_grid_entry_t *entry)
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

static uint32_t egui_view_virtual_grid_get_item_count_internal(egui_view_virtual_grid_t *local)
{
    void *data_source_context;
    const egui_view_virtual_grid_data_source_t *data_source = egui_view_virtual_grid_get_data_source_bridge(local, &data_source_context);

    if (data_source == NULL || data_source->get_count == NULL)
    {
        return 0;
    }

    return data_source->get_count(data_source_context);
}

static uint32_t egui_view_virtual_grid_get_row_count_from_item_count(uint32_t item_count, uint8_t column_count)
{
    if (item_count == 0)
    {
        return 0;
    }

    return (item_count + column_count - 1U) / column_count;
}

static uint32_t egui_view_virtual_grid_get_row_count_internal(egui_view_virtual_grid_t *local)
{
    return egui_view_virtual_grid_get_row_count_from_item_count(egui_view_virtual_grid_get_item_count_internal(local),
                                                                egui_view_virtual_grid_get_column_count_internal(local));
}

static uint32_t egui_view_virtual_grid_get_item_stable_id_internal(egui_view_virtual_grid_t *local, uint32_t index)
{
    void *data_source_context;
    const egui_view_virtual_grid_data_source_t *data_source = egui_view_virtual_grid_get_data_source_bridge(local, &data_source_context);

    if (data_source != NULL && data_source->get_stable_id != NULL)
    {
        return data_source->get_stable_id(data_source_context, index);
    }

    return egui_view_virtual_grid_default_stable_id(index);
}

static int32_t egui_view_virtual_grid_find_item_index_by_stable_id_internal(egui_view_virtual_grid_t *local, uint32_t stable_id)
{
    void *data_source_context;
    const egui_view_virtual_grid_data_source_t *data_source = egui_view_virtual_grid_get_data_source_bridge(local, &data_source_context);
    uint32_t count;
    uint32_t index;

    if (stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID || stable_id == 0U)
    {
        return -1;
    }

    if (data_source != NULL && data_source->find_index_by_stable_id != NULL)
    {
        int32_t found_index = data_source->find_index_by_stable_id(data_source_context, stable_id);

        if (found_index >= 0 && (uint32_t)found_index < egui_view_virtual_grid_get_item_count_internal(local))
        {
            return found_index;
        }
    }

    count = egui_view_virtual_grid_get_item_count_internal(local);
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

    index = stable_id - 1U;
    if (index >= count)
    {
        return -1;
    }

    return (int32_t)index;
}

static uint16_t egui_view_virtual_grid_get_item_view_type_internal(egui_view_virtual_grid_t *local, uint32_t index)
{
    void *data_source_context;
    const egui_view_virtual_grid_data_source_t *data_source = egui_view_virtual_grid_get_data_source_bridge(local, &data_source_context);

    if (data_source != NULL && data_source->get_item_view_type != NULL)
    {
        return data_source->get_item_view_type(data_source_context, index);
    }

    return data_source != NULL ? data_source->default_item_view_type : 0;
}

static int32_t egui_view_virtual_grid_measure_item_height_internal(egui_view_t *self, egui_view_virtual_grid_t *local, uint32_t index)
{
    void *data_source_context;
    const egui_view_virtual_grid_data_source_t *data_source = egui_view_virtual_grid_get_data_source_bridge(local, &data_source_context);
    int32_t height;
    uint8_t column_count = egui_view_virtual_grid_get_column_count_internal(local);
    uint8_t column_index = (uint8_t)(index % column_count);

    if (data_source != NULL && data_source->measure_item_height != NULL)
    {
        height = data_source->measure_item_height(data_source_context, index, egui_view_virtual_grid_get_column_width_internal(self, local, column_index));
        return egui_view_virtual_grid_sanitize_item_height(height);
    }

    return egui_view_virtual_grid_sanitize_item_height(local->estimated_item_height);
}

static uint32_t egui_view_virtual_grid_get_row_first_item_index(uint8_t column_count, uint32_t row_index)
{
    return (uint32_t)column_count * row_index;
}

static uint32_t egui_view_virtual_grid_get_row_item_count_internal(egui_view_virtual_grid_t *local, uint32_t row_index)
{
    uint8_t column_count = egui_view_virtual_grid_get_column_count_internal(local);
    uint32_t item_count = egui_view_virtual_grid_get_item_count_internal(local);
    uint32_t first_index = egui_view_virtual_grid_get_row_first_item_index(column_count, row_index);
    uint32_t remaining;

    if (first_index >= item_count)
    {
        return 0;
    }

    remaining = item_count - first_index;
    return remaining > column_count ? column_count : remaining;
}

static uint32_t egui_view_virtual_grid_get_row_stable_id_internal(egui_view_virtual_grid_t *local, uint32_t row_index)
{
    uint8_t column_count = egui_view_virtual_grid_get_column_count_internal(local);
    uint32_t first_index = egui_view_virtual_grid_get_row_first_item_index(column_count, row_index);

    if (first_index >= egui_view_virtual_grid_get_item_count_internal(local))
    {
        return EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    }

    return egui_view_virtual_grid_get_item_stable_id_internal(local, first_index);
}

static uint8_t egui_view_virtual_grid_resolve_item_by_index_internal(egui_view_virtual_grid_t *local, uint32_t index, egui_view_virtual_grid_entry_t *entry)
{
    uint8_t column_count = egui_view_virtual_grid_get_column_count_internal(local);
    uint32_t count = egui_view_virtual_grid_get_item_count_internal(local);

    egui_view_virtual_grid_reset_entry(entry);
    if (index >= count)
    {
        return 0;
    }

    if (entry != NULL)
    {
        entry->index = index;
        entry->row_index = index / column_count;
        entry->column_index = (uint8_t)(index % column_count);
        entry->stable_id = egui_view_virtual_grid_get_item_stable_id_internal(local, index);
    }

    return 1;
}

static int32_t egui_view_virtual_grid_get_column_width_internal(egui_view_t *self, egui_view_virtual_grid_t *local, uint8_t column_index)
{
    uint8_t column_count = egui_view_virtual_grid_get_column_count_internal(local);
    egui_dim_t spacing = egui_view_virtual_grid_sanitize_spacing(local->column_spacing);
    int32_t total_width = self->region.size.width > 0 ? self->region.size.width : 1;
    int32_t available_width = total_width - (int32_t)(column_count - 1U) * spacing;
    int32_t base_width;

    if (column_index >= column_count)
    {
        return -1;
    }

    if (available_width < (int32_t)column_count)
    {
        available_width = column_count;
    }

    base_width = available_width / column_count;
    if (base_width <= 0)
    {
        base_width = 1;
    }

    if ((uint8_t)(column_index + 1U) < column_count)
    {
        return base_width;
    }

    return available_width - base_width * (int32_t)(column_count - 1U);
}

static int32_t egui_view_virtual_grid_get_column_x_internal(egui_view_t *self, egui_view_virtual_grid_t *local, uint8_t column_index)
{
    egui_dim_t spacing = egui_view_virtual_grid_sanitize_spacing(local->column_spacing);
    int32_t x = 0;
    uint8_t i;

    for (i = 0; i < column_index; i++)
    {
        x += egui_view_virtual_grid_get_column_width_internal(self, local, i);
        x += spacing;
    }

    return x;
}

static int32_t egui_view_virtual_grid_measure_row_height_internal(egui_view_t *self, egui_view_virtual_grid_t *local, uint32_t row_index)
{
    uint8_t column_count = egui_view_virtual_grid_get_column_count_internal(local);
    uint32_t row_count = egui_view_virtual_grid_get_row_count_internal(local);
    uint32_t item_count = egui_view_virtual_grid_get_row_item_count_internal(local, row_index);
    uint32_t first_index = egui_view_virtual_grid_get_row_first_item_index(column_count, row_index);
    egui_dim_t row_spacing = egui_view_virtual_grid_sanitize_spacing(local->row_spacing);
    int32_t row_height = 1;
    uint32_t i;

    if (row_index >= row_count)
    {
        return -1;
    }

    for (i = 0; i < item_count; i++)
    {
        int32_t item_height = egui_view_virtual_grid_measure_item_height_internal(self, local, first_index + i);

        if (item_height > row_height)
        {
            row_height = item_height;
        }
    }

    if ((row_index + 1U) < row_count)
    {
        row_height += row_spacing;
    }

    return row_height;
}

static uint8_t egui_view_virtual_grid_is_attached_to_window(egui_view_virtual_grid_t *local)
{
    return local->base.base.is_attached_to_window ? 1U : 0U;
}

static void egui_view_virtual_grid_row_reset_binding(egui_view_virtual_grid_row_cell_t *cell)
{
    cell->index = EGUI_VIEW_VIRTUAL_GRID_INVALID_INDEX;
    cell->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    cell->height = 0;
    cell->is_bound = 0;
}

static void egui_view_virtual_grid_row_hide_cell(egui_view_virtual_grid_row_view_t *row_view, egui_view_virtual_grid_row_cell_t *cell)
{
    egui_dim_t hidden_x;

    if (cell->view == NULL)
    {
        return;
    }

    hidden_x = (egui_dim_t)(-(EGUI_VIEW_OF(&row_view->root)->region.size.width + cell->view->region.size.width + 4));
    egui_view_set_gone(cell->view, 1);
    egui_view_set_position(cell->view, hidden_x, 0);
}

static void egui_view_virtual_grid_row_detach_child(egui_view_virtual_grid_t *local, egui_view_virtual_grid_row_view_t *row_view, egui_view_t *child)
{
    if (child == NULL)
    {
        return;
    }

    if (egui_view_virtual_grid_is_attached_to_window(local))
    {
        child->api->on_detach_from_window(child);
    }

    if (child->parent == &row_view->root)
    {
        egui_view_group_remove_child(EGUI_VIEW_OF(&row_view->root), child);
    }
    egui_view_set_parent(child, NULL);
}

static void egui_view_virtual_grid_row_save_bound_cell(egui_view_virtual_grid_t *local, egui_view_virtual_grid_row_cell_t *cell)
{
    void *data_source_context;
    const egui_view_virtual_grid_data_source_t *data_source = egui_view_virtual_grid_get_data_source_bridge(local, &data_source_context);

    if (data_source == NULL || data_source->save_item_state == NULL || !cell->is_bound || cell->view == NULL ||
        cell->stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        return;
    }

    data_source->save_item_state(data_source_context, cell->view, cell->stable_id);
}

static void egui_view_virtual_grid_row_unbind_cell(egui_view_virtual_grid_t *local, egui_view_virtual_grid_row_view_t *row_view,
                                                   egui_view_virtual_grid_row_cell_t *cell)
{
    void *data_source_context;
    const egui_view_virtual_grid_data_source_t *data_source = egui_view_virtual_grid_get_data_source_bridge(local, &data_source_context);

    if (data_source != NULL && data_source->unbind_item_view != NULL && cell->is_bound && cell->view != NULL &&
        cell->stable_id != EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        data_source->unbind_item_view(data_source_context, cell->view, cell->stable_id);
    }

    egui_view_virtual_grid_row_reset_binding(cell);
    egui_view_virtual_grid_row_hide_cell(row_view, cell);
}

static uint8_t egui_view_virtual_grid_row_prepare_cell_view(egui_view_virtual_grid_t *local, egui_view_virtual_grid_row_view_t *row_view, uint8_t column_index,
                                                            uint16_t view_type)
{
    void *data_source_context;
    const egui_view_virtual_grid_data_source_t *data_source = egui_view_virtual_grid_get_data_source_bridge(local, &data_source_context);
    egui_view_virtual_grid_row_cell_t *cell = &row_view->cells[column_index];
    egui_view_t *view;

    if (cell->view != NULL && cell->view_type == view_type)
    {
        return 1;
    }

    if (cell->view != NULL)
    {
        if (data_source == NULL || data_source->destroy_item_view == NULL)
        {
            return 0;
        }

        egui_view_virtual_grid_row_detach_child(local, row_view, cell->view);
        data_source->destroy_item_view(data_source_context, cell->view, cell->view_type);
        cell->view = NULL;
        cell->view_type = 0;
    }

    if (data_source == NULL || data_source->create_item_view == NULL)
    {
        return 0;
    }

    view = data_source->create_item_view(data_source_context, view_type);
    if (view == NULL)
    {
        return 0;
    }

    cell->view = view;
    cell->view_type = view_type;
    egui_view_group_add_child(EGUI_VIEW_OF(&row_view->root), view);
    if (egui_view_virtual_grid_is_attached_to_window(local))
    {
        view->api->on_attach_to_window(view);
    }

    egui_view_virtual_grid_row_hide_cell(row_view, cell);
    return 1;
}

static void egui_view_virtual_grid_row_release_cell_view(egui_view_virtual_grid_t *local, egui_view_virtual_grid_row_view_t *row_view,
                                                         egui_view_virtual_grid_row_cell_t *cell)
{
    void *data_source_context;
    const egui_view_virtual_grid_data_source_t *data_source = egui_view_virtual_grid_get_data_source_bridge(local, &data_source_context);

    if (cell->is_bound)
    {
        egui_view_virtual_grid_row_unbind_cell(local, row_view, cell);
    }
    if (cell->view == NULL)
    {
        cell->view_type = 0;
        return;
    }

    egui_view_virtual_grid_row_detach_child(local, row_view, cell->view);
    if (data_source != NULL && data_source->destroy_item_view != NULL)
    {
        data_source->destroy_item_view(data_source_context, cell->view, cell->view_type);
    }

    cell->view = NULL;
    cell->view_type = 0;
    egui_view_virtual_grid_row_reset_binding(cell);
}

static void egui_view_virtual_grid_row_view_init(egui_view_virtual_grid_row_view_t *row_view)
{
    uint8_t i;

    egui_view_group_init(EGUI_VIEW_OF(&row_view->root));
    row_view->row_index = EGUI_VIEW_VIRTUAL_GRID_INVALID_INDEX;
    row_view->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    row_view->bound_count = 0;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS; i++)
    {
        row_view->cells[i].view = NULL;
        row_view->cells[i].view_type = 0;
        egui_view_virtual_grid_row_reset_binding(&row_view->cells[i]);
    }

    egui_view_set_view_name(EGUI_VIEW_OF(&row_view->root), "egui_view_virtual_grid_row");
}

static egui_view_virtual_grid_row_view_t *egui_view_virtual_grid_row_view_from_view(egui_view_t *view)
{
    return (egui_view_virtual_grid_row_view_t *)view;
}

static egui_view_t *egui_view_virtual_grid_row_create_view(void *adapter_context, uint16_t view_type)
{
    egui_view_virtual_grid_row_view_t *row_view;

    EGUI_UNUSED(adapter_context);
    EGUI_UNUSED(view_type);

    row_view = (egui_view_virtual_grid_row_view_t *)egui_malloc(sizeof(egui_view_virtual_grid_row_view_t));
    if (row_view == NULL)
    {
        return NULL;
    }

    egui_api_memset(row_view, 0, sizeof(*row_view));
    egui_view_virtual_grid_row_view_init(row_view);
    return EGUI_VIEW_OF(&row_view->root);
}

static void egui_view_virtual_grid_row_destroy_view(void *adapter_context, egui_view_t *view, uint16_t view_type)
{
    egui_view_virtual_grid_t *local = egui_view_virtual_grid_from_row_context(adapter_context);
    egui_view_virtual_grid_row_view_t *row_view = egui_view_virtual_grid_row_view_from_view(view);
    uint8_t i;

    EGUI_UNUSED(view_type);

    if (row_view == NULL)
    {
        return;
    }

    for (i = 0; i < EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS; i++)
    {
        egui_view_virtual_grid_row_release_cell_view(local, row_view, &row_view->cells[i]);
    }

    egui_free(row_view);
}

static void egui_view_virtual_grid_row_bind_view(void *adapter_context, egui_view_t *view, uint32_t row_index, uint32_t stable_id)
{
    egui_view_virtual_grid_t *local = egui_view_virtual_grid_from_row_context(adapter_context);
    egui_view_virtual_grid_row_view_t *row_view = egui_view_virtual_grid_row_view_from_view(view);
    void *data_source_context;
    const egui_view_virtual_grid_data_source_t *data_source = egui_view_virtual_grid_get_data_source_bridge(local, &data_source_context);
    uint8_t column_count = egui_view_virtual_grid_get_column_count_internal(local);
    uint32_t row_item_count = egui_view_virtual_grid_get_row_item_count_internal(local, row_index);
    uint8_t i;

    row_view->row_index = row_index;
    row_view->stable_id = stable_id;
    row_view->bound_count = (uint8_t)row_item_count;

    for (i = 0; i < EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS; i++)
    {
        egui_view_virtual_grid_row_cell_t *cell = &row_view->cells[i];

        if (i >= column_count || i >= row_item_count)
        {
            if (cell->is_bound)
            {
                egui_view_virtual_grid_row_save_bound_cell(local, cell);
                egui_view_virtual_grid_row_unbind_cell(local, row_view, cell);
            }
            else
            {
                egui_view_virtual_grid_row_hide_cell(row_view, cell);
            }
            continue;
        }

        {
            uint32_t index = egui_view_virtual_grid_get_row_first_item_index(column_count, row_index) + i;
            uint32_t item_stable_id = egui_view_virtual_grid_get_item_stable_id_internal(local, index);
            uint16_t view_type = egui_view_virtual_grid_get_item_view_type_internal(local, index);
            int32_t width = egui_view_virtual_grid_get_column_width_internal(EGUI_VIEW_OF(local), local, i);
            int32_t height = egui_view_virtual_grid_measure_item_height_internal(EGUI_VIEW_OF(local), local, index);
            int32_t x = egui_view_virtual_grid_get_column_x_internal(EGUI_VIEW_OF(local), local, i);
            egui_view_t *old_view = cell->view;
            uint16_t old_view_type = cell->view_type;
            uint32_t old_stable_id = cell->stable_id;
            uint8_t old_is_bound = cell->is_bound;
            uint8_t needs_restore;
            egui_region_t region;

            if (cell->is_bound && (cell->stable_id != item_stable_id || cell->view_type != view_type))
            {
                egui_view_virtual_grid_row_save_bound_cell(local, cell);
                egui_view_virtual_grid_row_unbind_cell(local, row_view, cell);
            }

            if (!egui_view_virtual_grid_row_prepare_cell_view(local, row_view, i, view_type))
            {
                egui_view_virtual_grid_row_reset_binding(cell);
                egui_view_virtual_grid_row_hide_cell(row_view, cell);
                continue;
            }

            region.location.x = x;
            region.location.y = 0;
            region.size.width = width > 0 ? width : 1;
            region.size.height = height > 0 ? height : 1;
            egui_view_layout(cell->view, &region);
            egui_view_set_gone(cell->view, 0);

            if (data_source != NULL && data_source->bind_item_view != NULL)
            {
                data_source->bind_item_view(data_source_context, cell->view, index, item_stable_id);
            }

            needs_restore = (uint8_t)(!old_is_bound || old_stable_id != item_stable_id || old_view != cell->view || old_view_type != view_type);
            if (needs_restore && data_source != NULL && data_source->restore_item_state != NULL)
            {
                data_source->restore_item_state(data_source_context, cell->view, item_stable_id);
            }

            cell->index = index;
            cell->stable_id = item_stable_id;
            cell->height = height;
            cell->is_bound = 1;
            cell->view_type = view_type;
        }
    }
}

static void egui_view_virtual_grid_row_unbind_view(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_virtual_grid_t *local = egui_view_virtual_grid_from_row_context(adapter_context);
    egui_view_virtual_grid_row_view_t *row_view = egui_view_virtual_grid_row_view_from_view(view);
    uint8_t i;

    EGUI_UNUSED(stable_id);

    for (i = 0; i < EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS; i++)
    {
        if (row_view->cells[i].is_bound)
        {
            egui_view_virtual_grid_row_unbind_cell(local, row_view, &row_view->cells[i]);
        }
        else
        {
            egui_view_virtual_grid_row_hide_cell(row_view, &row_view->cells[i]);
        }
    }

    row_view->row_index = EGUI_VIEW_VIRTUAL_GRID_INVALID_INDEX;
    row_view->stable_id = EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID;
    row_view->bound_count = 0;
}

static uint8_t egui_view_virtual_grid_row_should_keep_alive(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_virtual_grid_t *local = egui_view_virtual_grid_from_row_context(adapter_context);
    egui_view_virtual_grid_row_view_t *row_view = egui_view_virtual_grid_row_view_from_view(view);
    void *data_source_context;
    const egui_view_virtual_grid_data_source_t *data_source = egui_view_virtual_grid_get_data_source_bridge(local, &data_source_context);
    uint8_t i;

    EGUI_UNUSED(stable_id);

    if (data_source == NULL || data_source->should_keep_alive == NULL)
    {
        return 0;
    }

    for (i = 0; i < EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS; i++)
    {
        egui_view_virtual_grid_row_cell_t *cell = &row_view->cells[i];

        if (!cell->is_bound || cell->view == NULL)
        {
            continue;
        }
        if (data_source->should_keep_alive(data_source_context, cell->view, cell->stable_id))
        {
            return 1;
        }
    }

    return 0;
}

static void egui_view_virtual_grid_row_save_state(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_virtual_grid_t *local = egui_view_virtual_grid_from_row_context(adapter_context);
    egui_view_virtual_grid_row_view_t *row_view = egui_view_virtual_grid_row_view_from_view(view);
    uint8_t i;

    EGUI_UNUSED(stable_id);

    for (i = 0; i < EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS; i++)
    {
        egui_view_virtual_grid_row_save_bound_cell(local, &row_view->cells[i]);
    }
}

static void egui_view_virtual_grid_row_restore_state(void *adapter_context, egui_view_t *view, uint32_t stable_id)
{
    egui_view_virtual_grid_t *local = egui_view_virtual_grid_from_row_context(adapter_context);
    egui_view_virtual_grid_row_view_t *row_view = egui_view_virtual_grid_row_view_from_view(view);
    void *data_source_context;
    const egui_view_virtual_grid_data_source_t *data_source = egui_view_virtual_grid_get_data_source_bridge(local, &data_source_context);
    uint8_t i;

    EGUI_UNUSED(stable_id);

    if (data_source == NULL || data_source->restore_item_state == NULL)
    {
        return;
    }

    for (i = 0; i < EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS; i++)
    {
        egui_view_virtual_grid_row_cell_t *cell = &row_view->cells[i];

        if (!cell->is_bound || cell->view == NULL)
        {
            continue;
        }

        data_source->restore_item_state(data_source_context, cell->view, cell->stable_id);
    }
}

static uint32_t egui_view_virtual_grid_row_get_count(void *adapter_context)
{
    egui_view_virtual_grid_t *local = egui_view_virtual_grid_from_row_context(adapter_context);
    return egui_view_virtual_grid_get_row_count_internal(local);
}

static uint32_t egui_view_virtual_grid_row_get_stable_id(void *adapter_context, uint32_t row_index)
{
    egui_view_virtual_grid_t *local = egui_view_virtual_grid_from_row_context(adapter_context);
    return egui_view_virtual_grid_get_row_stable_id_internal(local, row_index);
}

static int32_t egui_view_virtual_grid_row_find_index_by_stable_id(void *adapter_context, uint32_t stable_id)
{
    egui_view_virtual_grid_t *local = egui_view_virtual_grid_from_row_context(adapter_context);
    int32_t item_index = egui_view_virtual_grid_find_item_index_by_stable_id_internal(local, stable_id);

    if (item_index < 0)
    {
        return -1;
    }

    return item_index / egui_view_virtual_grid_get_column_count_internal(local);
}

static uint8_t egui_view_virtual_grid_is_row_slot_active(const egui_view_virtual_grid_slot_t *slot)
{
    if (slot == NULL || slot->view == NULL)
    {
        return 0;
    }

    return slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE || slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_KEEPALIVE;
}

static const egui_view_virtual_grid_slot_t *egui_view_virtual_grid_find_slot_containing_item_internal(egui_view_virtual_grid_t *local, uint32_t stable_id,
                                                                                                      egui_view_virtual_grid_row_cell_t **cell_out)
{
    uint8_t i;

    if (cell_out != NULL)
    {
        *cell_out = NULL;
    }

    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        const egui_view_virtual_grid_slot_t *slot = &local->base.base.slots[i];
        egui_view_virtual_grid_row_view_t *row_view;
        uint8_t column;

        if (!egui_view_virtual_grid_is_row_slot_active(slot))
        {
            continue;
        }

        row_view = egui_view_virtual_grid_row_view_from_view(slot->view);
        for (column = 0; column < EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS; column++)
        {
            egui_view_virtual_grid_row_cell_t *cell = &row_view->cells[column];

            if (!cell->is_bound || cell->stable_id != stable_id)
            {
                continue;
            }

            if (cell_out != NULL)
            {
                *cell_out = cell;
            }
            return slot;
        }
    }

    return NULL;
}

static uint8_t egui_view_virtual_grid_resolve_slot_cell_internal(egui_view_virtual_grid_t *local, const egui_view_virtual_grid_slot_t *slot,
                                                                 uint8_t column_index, egui_view_virtual_grid_entry_t *entry)
{
    egui_view_virtual_grid_row_view_t *row_view;
    egui_view_virtual_grid_row_cell_t *cell;

    egui_view_virtual_grid_reset_entry(entry);
    if (!egui_view_virtual_grid_is_row_slot_active(slot) || column_index >= EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS)
    {
        return 0;
    }

    row_view = egui_view_virtual_grid_row_view_from_view(slot->view);
    if (row_view == NULL || column_index >= row_view->bound_count)
    {
        return 0;
    }

    cell = &row_view->cells[column_index];
    if (!cell->is_bound || cell->stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID)
    {
        return 0;
    }

    return egui_view_virtual_grid_resolve_item_by_index_internal(local, cell->index, entry);
}

static egui_view_t *egui_view_virtual_grid_get_slot_cell_view_internal(const egui_view_virtual_grid_slot_t *slot, uint8_t column_index)
{
    egui_view_virtual_grid_row_view_t *row_view;
    egui_view_virtual_grid_row_cell_t *cell;

    if (!egui_view_virtual_grid_is_row_slot_active(slot) || column_index >= EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS)
    {
        return NULL;
    }

    row_view = egui_view_virtual_grid_row_view_from_view(slot->view);
    if (row_view == NULL || column_index >= row_view->bound_count)
    {
        return NULL;
    }

    cell = &row_view->cells[column_index];
    if (!cell->is_bound)
    {
        return NULL;
    }

    return cell->view;
}

static uint16_t egui_view_virtual_grid_row_get_view_type(void *adapter_context, uint32_t row_index)
{
    EGUI_UNUSED(adapter_context);
    EGUI_UNUSED(row_index);
    return 0;
}

static int32_t egui_view_virtual_grid_row_measure_height(void *adapter_context, uint32_t row_index, int32_t width_hint)
{
    egui_view_virtual_grid_t *local = egui_view_virtual_grid_from_row_context(adapter_context);

    EGUI_UNUSED(width_hint);
    return egui_view_virtual_grid_measure_row_height_internal(EGUI_VIEW_OF(local), local, row_index);
}

void egui_view_virtual_grid_apply_params(egui_view_t *self, const egui_view_virtual_grid_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    egui_view_virtual_list_params_t list_params;

    local->column_count = egui_view_virtual_grid_sanitize_column_count(params->column_count);
    local->column_spacing = egui_view_virtual_grid_sanitize_spacing(params->column_spacing);
    local->row_spacing = egui_view_virtual_grid_sanitize_spacing(params->row_spacing);
    local->estimated_item_height = egui_view_virtual_grid_sanitize_item_height(params->estimated_item_height);

    list_params.region = params->region;
    list_params.overscan_before = params->overscan_before;
    list_params.overscan_after = params->overscan_after;
    list_params.max_keepalive_slots = params->max_keepalive_slots;
    list_params.estimated_item_height = local->estimated_item_height + local->row_spacing;
    egui_view_virtual_list_apply_params(self, &list_params);
}

void egui_view_virtual_grid_init_with_params(egui_view_t *self, const egui_view_virtual_grid_params_t *params)
{
    egui_view_virtual_grid_init(self);
    egui_view_virtual_grid_apply_params(self, params);
}

void egui_view_virtual_grid_apply_setup(egui_view_t *self, const egui_view_virtual_grid_setup_t *setup)
{
    if (setup == NULL)
    {
        return;
    }

    if (setup->params != NULL)
    {
        egui_view_virtual_grid_apply_params(self, setup->params);
    }

    egui_view_virtual_grid_set_data_source(self, setup->data_source, setup->data_source_context);
    egui_view_virtual_grid_set_state_cache_limits(self, setup->state_cache_max_entries, setup->state_cache_max_bytes);
}

void egui_view_virtual_grid_init_with_setup(egui_view_t *self, const egui_view_virtual_grid_setup_t *setup)
{
    egui_view_virtual_grid_init(self);
    egui_view_virtual_grid_apply_setup(self, setup);
}

void egui_view_virtual_grid_set_data_source(egui_view_t *self, const egui_view_virtual_grid_data_source_t *data_source, void *data_source_context)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);

    local->data_source = data_source;
    local->data_source_context = data_source_context;

    if (data_source == NULL)
    {
        egui_api_memset(&local->row_data_source, 0, sizeof(local->row_data_source));
        egui_view_virtual_list_set_data_source(self, NULL, NULL);
        return;
    }

    local->row_data_source.get_count = egui_view_virtual_grid_row_get_count;
    local->row_data_source.get_stable_id = egui_view_virtual_grid_row_get_stable_id;
    local->row_data_source.find_index_by_stable_id = egui_view_virtual_grid_row_find_index_by_stable_id;
    local->row_data_source.get_view_type = egui_view_virtual_grid_row_get_view_type;
    local->row_data_source.measure_item_height = egui_view_virtual_grid_row_measure_height;
    local->row_data_source.create_item_view = egui_view_virtual_grid_row_create_view;
    local->row_data_source.destroy_item_view = egui_view_virtual_grid_row_destroy_view;
    local->row_data_source.bind_item_view = egui_view_virtual_grid_row_bind_view;
    local->row_data_source.unbind_item_view = egui_view_virtual_grid_row_unbind_view;
    local->row_data_source.should_keep_alive = egui_view_virtual_grid_row_should_keep_alive;
    local->row_data_source.save_item_state = egui_view_virtual_grid_row_save_state;
    local->row_data_source.restore_item_state = egui_view_virtual_grid_row_restore_state;
    local->row_data_source.default_view_type = 0;

    egui_view_virtual_list_set_data_source(self, &local->row_data_source, local);
}

const egui_view_virtual_grid_data_source_t *egui_view_virtual_grid_get_data_source(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    return local->data_source;
}

void *egui_view_virtual_grid_get_data_source_context(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    return local->data_source_context;
}

uint32_t egui_view_virtual_grid_get_item_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    return egui_view_virtual_grid_get_item_count_internal(local);
}

uint32_t egui_view_virtual_grid_get_row_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    return egui_view_virtual_grid_get_row_count_internal(local);
}

void egui_view_virtual_grid_set_overscan(egui_view_t *self, uint8_t before, uint8_t after)
{
    egui_view_virtual_list_set_overscan(self, before, after);
}

uint8_t egui_view_virtual_grid_get_overscan_before(egui_view_t *self)
{
    return egui_view_virtual_list_get_overscan_before(self);
}

uint8_t egui_view_virtual_grid_get_overscan_after(egui_view_t *self)
{
    return egui_view_virtual_list_get_overscan_after(self);
}

void egui_view_virtual_grid_set_column_count(egui_view_t *self, uint8_t column_count)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    uint8_t sanitized = egui_view_virtual_grid_sanitize_column_count(column_count);

    if (local->column_count == sanitized)
    {
        return;
    }

    local->column_count = sanitized;
    egui_view_virtual_grid_notify_data_changed(self);
}

uint8_t egui_view_virtual_grid_get_column_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    return egui_view_virtual_grid_get_column_count_internal(local);
}

void egui_view_virtual_grid_set_spacing(egui_view_t *self, egui_dim_t column_spacing, egui_dim_t row_spacing)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    egui_dim_t sanitized_column_spacing = egui_view_virtual_grid_sanitize_spacing(column_spacing);
    egui_dim_t sanitized_row_spacing = egui_view_virtual_grid_sanitize_spacing(row_spacing);

    if (local->column_spacing == sanitized_column_spacing && local->row_spacing == sanitized_row_spacing)
    {
        return;
    }

    local->column_spacing = sanitized_column_spacing;
    local->row_spacing = sanitized_row_spacing;
    egui_view_virtual_list_set_estimated_item_height(self, local->estimated_item_height + local->row_spacing);
    egui_view_virtual_grid_notify_data_changed(self);
}

egui_dim_t egui_view_virtual_grid_get_column_spacing(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    return egui_view_virtual_grid_sanitize_spacing(local->column_spacing);
}

egui_dim_t egui_view_virtual_grid_get_row_spacing(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    return egui_view_virtual_grid_sanitize_spacing(local->row_spacing);
}

void egui_view_virtual_grid_set_estimated_item_height(egui_view_t *self, int32_t height)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    int32_t sanitized = egui_view_virtual_grid_sanitize_item_height(height);

    if (local->estimated_item_height == sanitized)
    {
        return;
    }

    local->estimated_item_height = sanitized;
    egui_view_virtual_list_set_estimated_item_height(self, local->estimated_item_height + local->row_spacing);
    egui_view_virtual_grid_notify_data_changed(self);
}

int32_t egui_view_virtual_grid_get_estimated_item_height(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    return local->estimated_item_height;
}

void egui_view_virtual_grid_set_keepalive_limit(egui_view_t *self, uint8_t max_keepalive_slots)
{
    egui_view_virtual_list_set_keepalive_limit(self, max_keepalive_slots);
}

uint8_t egui_view_virtual_grid_get_keepalive_limit(egui_view_t *self)
{
    return egui_view_virtual_list_get_keepalive_limit(self);
}

void egui_view_virtual_grid_set_state_cache_limits(egui_view_t *self, uint16_t max_entries, uint32_t max_bytes)
{
    egui_view_virtual_list_set_state_cache_limits(self, max_entries, max_bytes);
}

uint16_t egui_view_virtual_grid_get_state_cache_entry_limit(egui_view_t *self)
{
    return egui_view_virtual_list_get_state_cache_entry_limit(self);
}

uint32_t egui_view_virtual_grid_get_state_cache_byte_limit(egui_view_t *self)
{
    return egui_view_virtual_list_get_state_cache_byte_limit(self);
}

void egui_view_virtual_grid_clear_item_state_cache(egui_view_t *self)
{
    egui_view_virtual_list_clear_item_state_cache(self);
}

void egui_view_virtual_grid_remove_item_state_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    egui_view_virtual_list_remove_item_state_by_stable_id(self, stable_id);
}

uint8_t egui_view_virtual_grid_write_item_state(egui_view_t *self, uint32_t stable_id, const void *data, uint16_t size)
{
    return egui_view_virtual_list_write_item_state(self, stable_id, data, size);
}

uint16_t egui_view_virtual_grid_read_item_state(egui_view_t *self, uint32_t stable_id, void *data, uint16_t capacity)
{
    return egui_view_virtual_list_read_item_state(self, stable_id, data, capacity);
}

uint8_t egui_view_virtual_grid_write_item_state_for_view(egui_view_t *item_view, uint32_t stable_id, const void *data, uint16_t size)
{
    return egui_view_virtual_list_write_item_state_for_view(item_view, stable_id, data, size);
}

uint16_t egui_view_virtual_grid_read_item_state_for_view(egui_view_t *item_view, uint32_t stable_id, void *data, uint16_t capacity)
{
    return egui_view_virtual_list_read_item_state_for_view(item_view, stable_id, data, capacity);
}

void egui_view_virtual_grid_set_scroll_y(egui_view_t *self, int32_t offset)
{
    egui_view_virtual_list_set_scroll_y(self, offset);
}

void egui_view_virtual_grid_scroll_by(egui_view_t *self, int32_t delta)
{
    egui_view_virtual_list_scroll_by(self, delta);
}

void egui_view_virtual_grid_scroll_to_item(egui_view_t *self, uint32_t index, int32_t item_offset)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);

    if (index >= egui_view_virtual_grid_get_item_count_internal(local))
    {
        return;
    }

    egui_view_virtual_list_scroll_to_item(self, index / egui_view_virtual_grid_get_column_count_internal(local), item_offset);
}

void egui_view_virtual_grid_scroll_to_item_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t item_offset)
{
    int32_t index = egui_view_virtual_grid_find_index_by_stable_id(self, stable_id);

    if (index < 0)
    {
        return;
    }

    egui_view_virtual_grid_scroll_to_item(self, (uint32_t)index, item_offset);
}

uint8_t egui_view_virtual_grid_ensure_item_visible_by_stable_id(egui_view_t *self, uint32_t stable_id, int32_t inset)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    egui_view_virtual_grid_row_cell_t *cell = NULL;
    const egui_view_virtual_grid_slot_t *slot;
    int32_t item_y;

    if (stable_id == EGUI_VIEW_VIRTUAL_VIEWPORT_INVALID_ID || egui_view_virtual_grid_find_index_by_stable_id(self, stable_id) < 0)
    {
        return 0;
    }

    slot = egui_view_virtual_grid_find_slot_containing_item_internal(local, stable_id, &cell);
    if (slot != NULL && slot->state == EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE && cell != NULL && cell->is_bound && cell->view != NULL &&
        self->region.size.height > 0)
    {
        item_y = slot->render_region.location.y + cell->view->region.location.y;
        if (egui_view_virtual_viewport_is_main_span_fully_visible(self, item_y, cell->view->region.size.height, inset))
        {
            return 1;
        }
    }

    egui_view_virtual_grid_scroll_to_item_by_stable_id(self, stable_id, inset);
    return 1;
}

int32_t egui_view_virtual_grid_get_scroll_y(egui_view_t *self)
{
    return egui_view_virtual_list_get_scroll_y(self);
}

int32_t egui_view_virtual_grid_find_index_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    return egui_view_virtual_grid_find_item_index_by_stable_id_internal(local, stable_id);
}

uint8_t egui_view_virtual_grid_resolve_item_by_stable_id(egui_view_t *self, uint32_t stable_id, egui_view_virtual_grid_entry_t *entry)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    int32_t index = egui_view_virtual_grid_find_item_index_by_stable_id_internal(local, stable_id);

    egui_view_virtual_grid_reset_entry(entry);
    if (index < 0)
    {
        return 0;
    }

    return egui_view_virtual_grid_resolve_item_by_index_internal(local, (uint32_t)index, entry);
}

uint8_t egui_view_virtual_grid_resolve_item_by_view(egui_view_t *self, egui_view_t *item_view, egui_view_virtual_grid_entry_t *entry)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    uint8_t i;

    egui_view_virtual_grid_reset_entry(entry);
    for (i = 0; i < EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS; i++)
    {
        const egui_view_virtual_list_slot_t *slot = &local->base.base.slots[i];
        egui_view_virtual_grid_row_view_t *row_view;
        uint8_t column;

        if ((slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE && slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_KEEPALIVE) || slot->view == NULL)
        {
            continue;
        }

        row_view = egui_view_virtual_grid_row_view_from_view(slot->view);
        for (column = 0; column < EGUI_VIEW_VIRTUAL_GRID_MAX_COLUMNS; column++)
        {
            egui_view_virtual_grid_row_cell_t *cell = &row_view->cells[column];

            if (!cell->is_bound || cell->view != item_view)
            {
                continue;
            }

            return egui_view_virtual_grid_resolve_item_by_index_internal(local, cell->index, entry);
        }
    }

    return 0;
}

int32_t egui_view_virtual_grid_get_item_x(egui_view_t *self, uint32_t index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    egui_view_virtual_grid_entry_t entry;

    if (!egui_view_virtual_grid_resolve_item_by_index_internal(local, index, &entry))
    {
        return -1;
    }

    return egui_view_virtual_grid_get_column_x_internal(self, local, entry.column_index);
}

int32_t egui_view_virtual_grid_get_item_y(egui_view_t *self, uint32_t index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    egui_view_virtual_grid_entry_t entry;

    if (!egui_view_virtual_grid_resolve_item_by_index_internal(local, index, &entry))
    {
        return -1;
    }

    return egui_view_virtual_list_get_item_y(self, entry.row_index);
}

int32_t egui_view_virtual_grid_get_item_width(egui_view_t *self, uint32_t index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    egui_view_virtual_grid_entry_t entry;

    if (!egui_view_virtual_grid_resolve_item_by_index_internal(local, index, &entry))
    {
        return -1;
    }

    return egui_view_virtual_grid_get_column_width_internal(self, local, entry.column_index);
}

int32_t egui_view_virtual_grid_get_item_height(egui_view_t *self, uint32_t index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);

    if (index >= egui_view_virtual_grid_get_item_count_internal(local))
    {
        return -1;
    }

    return egui_view_virtual_grid_measure_item_height_internal(self, local, index);
}

int32_t egui_view_virtual_grid_get_item_x_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t index = egui_view_virtual_grid_find_index_by_stable_id(self, stable_id);
    return index < 0 ? -1 : egui_view_virtual_grid_get_item_x(self, (uint32_t)index);
}

int32_t egui_view_virtual_grid_get_item_y_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t index = egui_view_virtual_grid_find_index_by_stable_id(self, stable_id);
    return index < 0 ? -1 : egui_view_virtual_grid_get_item_y(self, (uint32_t)index);
}

int32_t egui_view_virtual_grid_get_item_width_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t index = egui_view_virtual_grid_find_index_by_stable_id(self, stable_id);
    return index < 0 ? -1 : egui_view_virtual_grid_get_item_width(self, (uint32_t)index);
}

int32_t egui_view_virtual_grid_get_item_height_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t index = egui_view_virtual_grid_find_index_by_stable_id(self, stable_id);
    return index < 0 ? -1 : egui_view_virtual_grid_get_item_height(self, (uint32_t)index);
}

egui_view_t *egui_view_virtual_grid_find_view_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    const egui_view_virtual_grid_slot_t *slot;
    egui_view_virtual_grid_row_cell_t *cell;

    slot = egui_view_virtual_grid_find_slot_containing_item_internal(local, stable_id, &cell);
    return slot != NULL && cell != NULL ? cell->view : NULL;
}

uint8_t egui_view_virtual_grid_get_slot_count(egui_view_t *self)
{
    return egui_view_virtual_list_get_slot_count(self);
}

const egui_view_virtual_grid_slot_t *egui_view_virtual_grid_get_slot(egui_view_t *self, uint8_t slot_index)
{
    return egui_view_virtual_list_get_slot(self, slot_index);
}

int32_t egui_view_virtual_grid_find_slot_index_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    const egui_view_virtual_grid_slot_t *slot = egui_view_virtual_grid_find_slot_containing_item_internal(local, stable_id, NULL);

    if (slot == NULL)
    {
        return -1;
    }

    return (int32_t)(slot - local->base.base.slots);
}

const egui_view_virtual_grid_slot_t *egui_view_virtual_grid_find_slot_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t slot_index = egui_view_virtual_grid_find_slot_index_by_stable_id(self, stable_id);

    if (slot_index < 0)
    {
        return NULL;
    }

    return egui_view_virtual_grid_get_slot(self, (uint8_t)slot_index);
}

uint8_t egui_view_virtual_grid_get_slot_item_count(egui_view_t *self, uint8_t slot_index)
{
    const egui_view_virtual_grid_slot_t *slot = egui_view_virtual_grid_get_slot(self, slot_index);
    egui_view_virtual_grid_row_view_t *row_view;

    if (!egui_view_virtual_grid_is_row_slot_active(slot))
    {
        return 0;
    }

    row_view = egui_view_virtual_grid_row_view_from_view(slot->view);
    return row_view != NULL ? row_view->bound_count : 0;
}

uint8_t egui_view_virtual_grid_get_slot_entry(egui_view_t *self, uint8_t slot_index, uint8_t column_index, egui_view_virtual_grid_entry_t *entry)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);
    const egui_view_virtual_grid_slot_t *slot = egui_view_virtual_grid_get_slot(self, slot_index);

    return egui_view_virtual_grid_resolve_slot_cell_internal(local, slot, column_index, entry);
}

egui_view_t *egui_view_virtual_grid_get_slot_item_view(egui_view_t *self, uint8_t slot_index, uint8_t column_index)
{
    const egui_view_virtual_grid_slot_t *slot = egui_view_virtual_grid_get_slot(self, slot_index);

    return egui_view_virtual_grid_get_slot_cell_view_internal(slot, column_index);
}

uint8_t egui_view_virtual_grid_visit_visible_items(egui_view_t *self, egui_view_virtual_grid_visible_item_visitor_t visitor, void *context)
{
    uint8_t slot_count;
    uint8_t slot_index;
    uint8_t visited = 0;

    if (visitor == NULL)
    {
        return 0;
    }

    slot_count = egui_view_virtual_grid_get_slot_count(self);
    for (slot_index = 0; slot_index < slot_count; slot_index++)
    {
        uint8_t item_count;
        uint8_t column_index;
        const egui_view_virtual_grid_slot_t *slot = egui_view_virtual_grid_get_slot(self, slot_index);

        if (slot == NULL || slot->state != EGUI_VIEW_VIRTUAL_SLOT_STATE_VISIBLE)
        {
            continue;
        }

        item_count = egui_view_virtual_grid_get_slot_item_count(self, slot_index);
        for (column_index = 0; column_index < item_count; column_index++)
        {
            egui_view_virtual_grid_entry_t entry;
            egui_view_t *item_view;

            if (!egui_view_virtual_grid_get_slot_entry(self, slot_index, column_index, &entry))
            {
                continue;
            }

            item_view = egui_view_virtual_grid_get_slot_item_view(self, slot_index, column_index);
            if (item_view == NULL)
            {
                continue;
            }

            visited++;
            if (!visitor(self, slot, &entry, item_view, context))
            {
                return visited;
            }
        }
    }

    return visited;
}

typedef struct egui_view_virtual_grid_find_visible_item_context
{
    egui_view_virtual_grid_visible_item_matcher_t matcher;
    void *context;
    uint32_t best_index;
    egui_view_t *best_view;
    egui_view_virtual_grid_entry_t best_entry;
} egui_view_virtual_grid_find_visible_item_context_t;

static uint8_t egui_view_virtual_grid_find_visible_item_visitor(egui_view_t *self, const egui_view_virtual_grid_slot_t *slot,
                                                                const egui_view_virtual_grid_entry_t *entry, egui_view_t *item_view, void *context)
{
    egui_view_virtual_grid_find_visible_item_context_t *ctx = (egui_view_virtual_grid_find_visible_item_context_t *)context;

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

egui_view_t *egui_view_virtual_grid_find_first_visible_item_view(egui_view_t *self, egui_view_virtual_grid_visible_item_matcher_t matcher, void *context,
                                                                 egui_view_virtual_grid_entry_t *entry_out)
{
    egui_view_virtual_grid_find_visible_item_context_t find_ctx = {
            .matcher = matcher,
            .context = context,
            .best_index = EGUI_VIEW_VIRTUAL_GRID_INVALID_INDEX,
            .best_view = NULL,
    };

    egui_view_virtual_grid_visit_visible_items(self, egui_view_virtual_grid_find_visible_item_visitor, &find_ctx);
    if (find_ctx.best_view != NULL && entry_out != NULL)
    {
        *entry_out = find_ctx.best_entry;
    }

    return find_ctx.best_view;
}

void egui_view_virtual_grid_notify_data_changed(egui_view_t *self)
{
    egui_view_virtual_list_notify_data_changed(self);
}

void egui_view_virtual_grid_notify_item_changed(egui_view_t *self, uint32_t index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);

    if (index >= egui_view_virtual_grid_get_item_count_internal(local))
    {
        return;
    }

    egui_view_virtual_list_notify_item_changed(self, index / egui_view_virtual_grid_get_column_count_internal(local));
}

void egui_view_virtual_grid_notify_item_changed_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t index = egui_view_virtual_grid_find_index_by_stable_id(self, stable_id);

    if (index < 0)
    {
        return;
    }

    egui_view_virtual_grid_notify_item_changed(self, (uint32_t)index);
}

void egui_view_virtual_grid_notify_item_inserted(egui_view_t *self, uint32_t index, uint32_t count)
{
    EGUI_UNUSED(index);
    EGUI_UNUSED(count);
    egui_view_virtual_grid_notify_data_changed(self);
}

void egui_view_virtual_grid_notify_item_removed(egui_view_t *self, uint32_t index, uint32_t count)
{
    EGUI_UNUSED(index);
    EGUI_UNUSED(count);
    egui_view_virtual_grid_notify_data_changed(self);
}

void egui_view_virtual_grid_notify_item_moved(egui_view_t *self, uint32_t from_index, uint32_t to_index)
{
    EGUI_UNUSED(from_index);
    EGUI_UNUSED(to_index);
    egui_view_virtual_grid_notify_data_changed(self);
}

void egui_view_virtual_grid_notify_item_resized(egui_view_t *self, uint32_t index)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);

    if (index >= egui_view_virtual_grid_get_item_count_internal(local))
    {
        return;
    }

    egui_view_virtual_list_notify_item_resized(self, index / egui_view_virtual_grid_get_column_count_internal(local));
}

void egui_view_virtual_grid_notify_item_resized_by_stable_id(egui_view_t *self, uint32_t stable_id)
{
    int32_t index = egui_view_virtual_grid_find_index_by_stable_id(self, stable_id);

    if (index < 0)
    {
        return;
    }

    egui_view_virtual_grid_notify_item_resized(self, (uint32_t)index);
}

void egui_view_virtual_grid_init(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_virtual_grid_t);

    egui_view_virtual_list_init(self);
    local->data_source = NULL;
    local->data_source_context = NULL;
    egui_api_memset(&local->row_data_source, 0, sizeof(local->row_data_source));
    local->column_count = 2;
    local->column_spacing = 6;
    local->row_spacing = 6;
    local->estimated_item_height = 64;
    egui_view_virtual_list_set_estimated_item_height(self, local->estimated_item_height + local->row_spacing);
    egui_view_set_view_name(self, "egui_view_virtual_grid");
}
