#include <stdio.h>
#include <assert.h>

#include "egui_view_list.h"
#include "font/egui_font.h"
#include "resource/egui_resource.h"

static void egui_view_list_item_click_handler(egui_view_t *self)
{
    // Walk up to find the list parent.
    // The button's parent is the container (linearlayout inside scroll),
    // and the container's parent is the scroll (which is the list base).
    egui_view_t *container = (egui_view_t *)self->parent;
    if (container == NULL)
    {
        return;
    }
    egui_view_t *scroll_view = (egui_view_t *)container->parent;
    if (scroll_view == NULL)
    {
        return;
    }
    egui_view_list_t *list = (egui_view_list_t *)scroll_view;

    if (list->on_item_click == NULL)
    {
        return;
    }

    // Compute index from pointer arithmetic
    egui_view_button_t *btn = (egui_view_button_t *)self;
    uint8_t index = (uint8_t)(btn - &list->items[0]);
    if (index < list->item_count)
    {
        list->on_item_click(scroll_view, index);
    }
}

int8_t egui_view_list_add_item(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_list_t);

    if (local->item_count >= EGUI_VIEW_LIST_MAX_ITEMS)
    {
        return -1;
    }

    uint8_t idx = local->item_count;
    egui_view_t *item_view = EGUI_VIEW_OF(&local->items[idx]);

    // Initialize button
    egui_view_button_init(item_view);
    egui_view_label_set_font(item_view, (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_text(item_view, text);
    egui_dim_t horizontal_margin = 12;
    egui_dim_t item_width = self->region.size.width;
    if (item_width > horizontal_margin)
    {
        item_width -= horizontal_margin;
    }
    egui_view_set_size(item_view, item_width, local->item_height);
    egui_view_set_position(item_view, 0, 0);
    egui_view_set_margin(item_view, 6, 3, 6, 3);
    egui_view_set_on_click_listener(item_view, egui_view_list_item_click_handler);

    // Add as child of scroll
    egui_view_scroll_add_child(self, item_view);

    local->item_count++;

    // Re-layout children
    egui_view_scroll_layout_childs(self);

    egui_view_invalidate(self);

    return (int8_t)idx;
}

void egui_view_list_clear(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_list_t);

    // Clear all children from the scroll container
    egui_view_group_clear_childs(EGUI_VIEW_OF(&local->base.container));
    local->item_count = 0;

    egui_view_invalidate(self);
}

void egui_view_list_set_item_height(egui_view_t *self, egui_dim_t height)
{
    EGUI_LOCAL_INIT(egui_view_list_t);

    if (local->item_height != height)
    {
        local->item_height = height;
        egui_view_invalidate(self);
    }
}

void egui_view_list_set_on_item_click(egui_view_t *self, egui_view_list_item_click_cb_t callback)
{
    EGUI_LOCAL_INIT(egui_view_list_t);
    local->on_item_click = callback;
}

void egui_view_list_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_list_t);
    // call super init.
    egui_view_scroll_init(self);

    // init local data.
    local->item_count = 0;
    local->item_height = 30;
    local->on_item_click = NULL;

    egui_view_set_view_name(self, "egui_view_list");
}

void egui_view_list_apply_params(egui_view_t *self, const egui_view_list_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_list_t);

    // Apply region via scroll's set_size
    egui_view_set_position(self, params->region.location.x, params->region.location.y);
    egui_view_scroll_set_size(self, params->region.size.width, params->region.size.height);

    local->item_height = params->item_height;

    egui_view_invalidate(self);
}

void egui_view_list_init_with_params(egui_view_t *self, const egui_view_list_params_t *params)
{
    egui_view_list_init(self);
    egui_view_list_apply_params(self, params);
}
