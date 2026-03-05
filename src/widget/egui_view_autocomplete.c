#include "egui_view_autocomplete.h"

void egui_view_autocomplete_set_suggestions(egui_view_t *self, const char **suggestions, uint8_t count)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (local->items == suggestions && local->item_count == count)
    {
        return;
    }
    egui_view_combobox_set_items(self, suggestions, count);
}

uint8_t egui_view_autocomplete_get_suggestion_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    return local->item_count;
}

void egui_view_autocomplete_set_current_index(egui_view_t *self, uint8_t index)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (local->current_index == index)
    {
        return;
    }
    egui_view_combobox_set_current_index(self, index);
}

uint8_t egui_view_autocomplete_get_current_index(egui_view_t *self)
{
    return egui_view_combobox_get_current_index(self);
}

const char *egui_view_autocomplete_get_current_text(egui_view_t *self)
{
    return egui_view_combobox_get_current_text(self);
}

void egui_view_autocomplete_set_max_visible_items(egui_view_t *self, uint8_t max_items)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (max_items == 0)
    {
        max_items = 1;
    }
    if (local->max_visible_items == max_items)
    {
        return;
    }
    egui_view_combobox_set_max_visible_items(self, max_items);
}

void egui_view_autocomplete_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (font == NULL || local->font == font)
    {
        return;
    }
    egui_view_combobox_set_font(self, font);
}

void egui_view_autocomplete_expand(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (local->is_expanded)
    {
        return;
    }
    egui_view_combobox_expand(self);
}

void egui_view_autocomplete_collapse(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (!local->is_expanded)
    {
        return;
    }
    egui_view_combobox_collapse(self);
}

uint8_t egui_view_autocomplete_is_expanded(egui_view_t *self)
{
    return egui_view_combobox_is_expanded(self);
}

void egui_view_autocomplete_set_on_selected_listener(egui_view_t *self, egui_view_on_autocomplete_selected_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (local->on_selected == listener)
    {
        return;
    }
    egui_view_combobox_set_on_selected_listener(self, listener);
}

void egui_view_autocomplete_init(egui_view_t *self)
{
    egui_view_combobox_init(self);
    egui_view_set_view_name(self, "egui_view_autocomplete");
}

void egui_view_autocomplete_apply_params(egui_view_t *self, const egui_view_autocomplete_params_t *params)
{
    egui_view_combobox_params_t combobox_params = {
            .region = params->region,
            .items = params->suggestions,
            .item_count = params->suggestion_count,
            .current_index = params->current_index,
    };
    egui_view_combobox_apply_params(self, &combobox_params);
}

void egui_view_autocomplete_init_with_params(egui_view_t *self, const egui_view_autocomplete_params_t *params)
{
    egui_view_autocomplete_init(self);
    egui_view_autocomplete_apply_params(self, params);
}
