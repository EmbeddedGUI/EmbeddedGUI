#include "egui_view_autocomplete.h"

/**
 * @file egui_view_autocomplete.c
 * @brief Autocomplete API implemented as a thin combobox wrapper.
 *
 * The wrapper mainly renames combobox concepts to "suggestions" so example
 * code reads closer to search boxes and input-assist widgets.
 */

/**
 * @brief Replace the borrowed suggestion list while avoiding redundant redraws.
 */
void egui_view_autocomplete_set_suggestions(egui_view_t *self, const char **suggestions, uint8_t count)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (local->items == suggestions && local->item_count == count)
    {
        return;
    }
    egui_view_combobox_set_items(self, suggestions, count);
}

const char **egui_view_autocomplete_get_suggestions(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    return local->items;
}

/**
 * @brief Return how many suggestions are currently available.
 */
uint8_t egui_view_autocomplete_get_suggestion_count(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    return local->item_count;
}

/**
 * @brief Change the active suggestion without firing the selection listener.
 */
void egui_view_autocomplete_set_current_index(egui_view_t *self, uint8_t index)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (local->current_index == index)
    {
        return;
    }
    egui_view_combobox_set_current_index(self, index);
}

/**
 * @brief Forward the active suggestion index from the wrapped combobox.
 */
uint8_t egui_view_autocomplete_get_current_index(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return egui_view_combobox_get_current_index(self);
}

/**
 * @brief Forward the currently selected suggestion text.
 */
const char *egui_view_autocomplete_get_current_text(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    return egui_view_combobox_get_current_text(self);
}

/**
 * @brief Clamp and forward the maximum number of visible suggestion rows.
 */
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

uint8_t egui_view_autocomplete_get_max_visible_items(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    return local->max_visible_items;
}

/**
 * @brief Override the font used to render suggestions and the header value.
 */
void egui_view_autocomplete_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (font == NULL || local->font == font)
    {
        return;
    }
    egui_view_combobox_set_font(self, font);
}

const egui_font_t *egui_view_autocomplete_get_font(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    return local->font;
}

/**
 * @brief Expand the suggestion list if it is currently collapsed.
 */
void egui_view_autocomplete_expand(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (local->is_expanded)
    {
        return;
    }
    egui_view_combobox_expand(self);
}

/**
 * @brief Collapse the suggestion list if it is currently expanded.
 */
void egui_view_autocomplete_collapse(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (!local->is_expanded)
    {
        return;
    }
    egui_view_combobox_collapse(self);
}

/**
 * @brief Return whether the suggestion list is expanded.
 */
uint8_t egui_view_autocomplete_is_expanded(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return egui_view_combobox_is_expanded(self);
}

/**
 * @brief Register the callback fired when one suggestion is chosen.
 */
void egui_view_autocomplete_set_on_selected_listener(egui_view_t *self, egui_view_on_autocomplete_selected_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (local->on_selected == listener)
    {
        return;
    }
    egui_view_combobox_set_on_selected_listener(self, listener);
}

egui_view_on_autocomplete_selected_listener_t egui_view_autocomplete_get_on_selected_listener(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    return local->on_selected;
}

/**
 * @brief Initialize the autocomplete wrapper on top of combobox behavior.
 */
void egui_view_autocomplete_init(egui_view_t *self, egui_core_t *core)
{
    egui_view_combobox_init(self, core);
    egui_view_set_view_name(self, "egui_view_autocomplete");
}

/**
 * @brief Translate autocomplete-style params into the combobox param block.
 */
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

/**
 * @brief Convenience initializer that chains autocomplete init and params.
 */
void egui_view_autocomplete_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_autocomplete_params_t *params)
{
    egui_view_autocomplete_init(self, core);
    egui_view_autocomplete_apply_params(self, params);
}
