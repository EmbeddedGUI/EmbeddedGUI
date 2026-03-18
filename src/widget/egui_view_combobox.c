#include <stdio.h>
#include <assert.h>

#include "egui_view_combobox.h"
#include "widget/egui_view_group.h"
#include "resource/egui_resource.h"
#include "core/egui_core.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

static const egui_font_t *egui_view_combobox_get_icon_font(egui_dim_t area_size)
{
    if (area_size <= 18)
    {
        return EGUI_FONT_ICON_MS_16;
    }
    if (area_size <= 22)
    {
        return EGUI_FONT_ICON_MS_20;
    }
    return EGUI_FONT_ICON_MS_24;
}

static const egui_font_t *egui_view_combobox_get_item_icon_font(egui_view_combobox_t *local, egui_dim_t area_size)
{
    if (local->icon_font != NULL)
    {
        return local->icon_font;
    }

    return egui_view_combobox_get_icon_font(area_size);
}

static const char *egui_view_combobox_get_arrow_icon(egui_view_combobox_t *local, uint8_t is_expanded)
{
    if (is_expanded)
    {
        return local->collapse_icon;
    }

    return local->expand_icon;
}

static egui_dim_t egui_view_combobox_get_content_icon_width(egui_view_combobox_t *local, egui_dim_t area_height, const char *icon)
{
    const egui_font_t *icon_font = egui_view_combobox_get_item_icon_font(local, area_height);
    egui_dim_t icon_width = 0;
    egui_dim_t icon_height = 0;

    if (icon_font != NULL && icon != NULL && icon_font->api != NULL && icon_font->api->get_str_size != NULL)
    {
        icon_font->api->get_str_size(icon_font, icon, 0, 0, &icon_width, &icon_height);
    }

    if (icon_width <= 0)
    {
        icon_width = icon_height;
    }
    if (icon_width <= 0)
    {
        if (area_height <= 18)
        {
            icon_width = 16;
        }
        else if (area_height <= 22)
        {
            icon_width = 20;
        }
        else
        {
            icon_width = 24;
        }
    }
    if (icon_width > area_height)
    {
        icon_width = area_height;
    }

    return icon_width;
}

static void egui_view_combobox_draw_item_content(egui_view_combobox_t *local, const egui_region_t *rect, const char *icon, const char *text, egui_color_t color,
                                                 egui_alpha_t alpha)
{
    egui_region_t text_rect = *rect;

    if (icon != NULL && icon[0] != '\0')
    {
        egui_region_t icon_rect = *rect;
        egui_dim_t icon_width = egui_view_combobox_get_content_icon_width(local, rect->size.height, icon);
        egui_dim_t gap = local->icon_text_gap;

        if (gap < 0)
        {
            gap = 0;
        }

        icon_rect.size.width = EGUI_MIN(icon_width, rect->size.width);
        egui_canvas_draw_text_in_rect(egui_view_combobox_get_item_icon_font(local, rect->size.height), icon, &icon_rect, EGUI_ALIGN_CENTER, color, alpha);

        text_rect.location.x += icon_rect.size.width;
        text_rect.size.width -= icon_rect.size.width;

        if (text != NULL && text[0] != '\0' && text_rect.size.width > gap)
        {
            text_rect.location.x += gap;
            text_rect.size.width -= gap;
        }
        else
        {
            text_rect.size.width = 0;
        }
    }

    if (text != NULL && text[0] != '\0' && text_rect.size.width > 0)
    {
        egui_canvas_draw_text_in_rect(local->font, text, &text_rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, color, alpha);
    }
}

static uint8_t egui_view_combobox_get_max_visible_count(const egui_view_combobox_t *local)
{
    uint8_t visible_count = local->item_count;
    if (visible_count > local->max_visible_items)
    {
        visible_count = local->max_visible_items;
    }
    return visible_count;
}

static uint8_t egui_view_combobox_get_visible_count_for_height(const egui_view_combobox_t *local, egui_dim_t total_height, uint8_t max_visible_count)
{
    if (max_visible_count == 0 || local->item_height <= 0 || total_height <= local->collapsed_height)
    {
        return 0;
    }

    egui_dim_t item_space = total_height - local->collapsed_height;
    uint8_t fit_count = (uint8_t)(item_space / local->item_height);
    if (fit_count > max_visible_count)
    {
        fit_count = max_visible_count;
    }
    return fit_count;
}

static egui_dim_t egui_view_combobox_get_available_bottom(egui_view_t *self)
{
    egui_dim_t available_bottom = EGUI_CONFIG_SCEEN_HEIGHT;
    egui_view_t *parent = (egui_view_t *)self->parent;

    while (parent != NULL)
    {
        egui_dim_t parent_bottom = parent->region_screen.location.y + parent->padding.top;

        if (parent->region_screen.size.height > parent->padding.top + parent->padding.bottom)
        {
            parent_bottom += parent->region_screen.size.height - parent->padding.top - parent->padding.bottom;
        }

        if (parent_bottom < available_bottom)
        {
            available_bottom = parent_bottom;
        }

        parent = (egui_view_t *)parent->parent;
    }

    return available_bottom;
}

static uint8_t egui_view_combobox_get_expand_fit_count(egui_view_t *self, const egui_view_combobox_t *local)
{
    uint8_t max_visible_count = egui_view_combobox_get_max_visible_count(local);
    if (max_visible_count == 0)
    {
        return 0;
    }

    egui_dim_t available_bottom = egui_view_combobox_get_available_bottom(self);
    egui_dim_t available_height = 0;

    if (self->region_screen.location.y < available_bottom)
    {
        available_height = available_bottom - self->region_screen.location.y;
    }

    return egui_view_combobox_get_visible_count_for_height(local, available_height, max_visible_count);
}

static uint8_t egui_view_combobox_get_current_visible_count(egui_view_t *self, const egui_view_combobox_t *local)
{
    uint8_t max_visible_count = egui_view_combobox_get_max_visible_count(local);
    return egui_view_combobox_get_visible_count_for_height(local, self->region.size.height, max_visible_count);
}

static uint8_t egui_view_combobox_get_visible_start_index(const egui_view_combobox_t *local, uint8_t visible_count)
{
    uint8_t start_index = 0;

    if (visible_count == 0 || local->item_count <= visible_count)
    {
        return 0;
    }

    if (local->current_index >= visible_count)
    {
        start_index = (uint8_t)(local->current_index + 1 - visible_count);
    }

    if ((uint16_t)start_index + visible_count > local->item_count)
    {
        start_index = (uint8_t)(local->item_count - visible_count);
    }

    return start_index;
}

static void egui_view_combobox_notify_selected(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);

    if (local->on_selected != NULL && local->current_index < local->item_count)
    {
        local->on_selected(self, local->current_index);
    }
}

static void egui_view_combobox_commit_current_index(egui_view_t *self, uint8_t index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);

    if (index >= local->item_count)
    {
        return;
    }

    if (local->current_index != index)
    {
        local->current_index = index;
        egui_view_invalidate(self);
    }

    if (notify)
    {
        egui_view_combobox_notify_selected(self);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_combobox_on_key_event(egui_view_t *self, egui_key_event_t *event);
#endif

void egui_view_combobox_set_on_selected_listener(egui_view_t *self, egui_view_on_combobox_selected_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    local->on_selected = listener;
}

void egui_view_combobox_set_items(egui_view_t *self, const char **items, uint8_t count)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    local->items = items;
    local->item_count = count;
    if (local->current_index >= count)
    {
        local->current_index = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_combobox_set_item_icons(egui_view_t *self, const char **item_icons)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);

    if (local->item_icons == item_icons)
    {
        return;
    }

    local->item_icons = item_icons;
    egui_view_invalidate(self);
}

void egui_view_combobox_set_current_index(egui_view_t *self, uint8_t index)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (index >= local->item_count)
    {
        return;
    }
    if (index != local->current_index)
    {
        local->current_index = index;
        egui_view_invalidate(self);
    }
}

uint8_t egui_view_combobox_get_current_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    return local->current_index;
}

const char *egui_view_combobox_get_current_text(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (local->items == NULL || local->item_count == 0)
    {
        return NULL;
    }
    return local->items[local->current_index];
}

void egui_view_combobox_set_max_visible_items(egui_view_t *self, uint8_t max_items)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (max_items < 1)
    {
        max_items = 1;
    }
    local->max_visible_items = max_items;
    egui_view_invalidate(self);
}

void egui_view_combobox_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    local->font = font;
    egui_view_invalidate(self);
}

void egui_view_combobox_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);

    if (local->icon_font == font)
    {
        return;
    }

    local->icon_font = font;
    egui_view_invalidate(self);
}

void egui_view_combobox_set_arrow_icons(egui_view_t *self, const char *expand_icon, const char *collapse_icon)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);

    if (expand_icon == NULL)
    {
        expand_icon = EGUI_ICON_MS_EXPAND_MORE;
    }

    if (collapse_icon == NULL)
    {
        collapse_icon = EGUI_ICON_MS_EXPAND_LESS;
    }

    if (local->expand_icon == expand_icon && local->collapse_icon == collapse_icon)
    {
        return;
    }

    local->expand_icon = expand_icon;
    local->collapse_icon = collapse_icon;
    egui_view_invalidate(self);
}

void egui_view_combobox_set_icon_text_gap(egui_view_t *self, egui_dim_t gap)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);

    if (local->icon_text_gap == gap)
    {
        return;
    }

    local->icon_text_gap = gap;
    egui_view_invalidate(self);
}

void egui_view_combobox_expand(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (!local->is_expanded && local->item_count > 0)
    {
        uint8_t visible_count = egui_view_combobox_get_expand_fit_count(self, local);
        if (visible_count == 0)
        {
            return;
        }

        local->is_expanded = 1;
#if EGUI_CONFIG_FUNCTION_SUPPORT_LAYER
        egui_view_set_layer(self, EGUI_VIEW_LAYER_TOP);
#endif
        egui_dim_t expanded_height = local->collapsed_height + visible_count * local->item_height;
        // Mark old region dirty before changing size
        egui_core_update_region_dirty(&self->region_screen);
        self->region.size.height = expanded_height;
        self->region_screen.size.height = expanded_height;
        // Mark new region dirty as well
        egui_core_update_region_dirty(&self->region_screen);
        egui_view_invalidate(self);
    }
}

void egui_view_combobox_collapse(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (local->is_expanded)
    {
        local->is_expanded = 0;
#if EGUI_CONFIG_FUNCTION_SUPPORT_LAYER
        egui_view_set_layer(self, EGUI_VIEW_LAYER_DEFAULT);
#endif
        // Mark old (expanded) region dirty before changing size
        egui_core_update_region_dirty(&self->region_screen);
        self->region.size.height = local->collapsed_height;
        self->region_screen.size.height = local->collapsed_height;
        egui_view_invalidate(self);
    }
}

uint8_t egui_view_combobox_is_expanded(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    return local->is_expanded;
}

static void egui_view_combobox_draw_arrow(egui_view_combobox_t *local, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, uint8_t is_expanded,
                                          egui_color_t color, egui_alpha_t alpha)
{
    egui_region_t arrow_rect = {{x, y}, {width, height}};
    const egui_font_t *icon_font = egui_view_combobox_get_item_icon_font(local, EGUI_MIN(width, height));
    const char *icon_text = egui_view_combobox_get_arrow_icon(local, is_expanded);

    egui_canvas_draw_text_in_rect(icon_font, icon_text, &arrow_rect, EGUI_ALIGN_CENTER, color, alpha);
}

void egui_view_combobox_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    bool is_enabled = egui_view_get_enable(self);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    bool is_focused = self->is_focused ? true : false;
#else
    bool is_focused = false;
#endif
    egui_color_t text_color = local->text_color;
    egui_color_t bg_color = local->bg_color;
    egui_color_t border_color = local->border_color;
    egui_color_t highlight_color = local->highlight_color;
    egui_color_t arrow_color = local->arrow_color;
    egui_color_t focus_color = EGUI_THEME_FOCUS;

    if (local->font == NULL)
    {
        return;
    }

    if (!is_enabled)
    {
        text_color = egui_rgb_mix(text_color, bg_color, EGUI_ALPHA_40);
        border_color = egui_rgb_mix(border_color, EGUI_THEME_DISABLED, EGUI_ALPHA_50);
        highlight_color = egui_rgb_mix(highlight_color, bg_color, EGUI_ALPHA_60);
        arrow_color = egui_rgb_mix(arrow_color, bg_color, EGUI_ALPHA_40);
        focus_color = egui_rgb_mix(focus_color, EGUI_THEME_DISABLED, EGUI_ALPHA_50);
    }

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t header_height = local->collapsed_height;
    egui_dim_t padding = 4;
    egui_dim_t arrow_slot_width = header_height;
    egui_dim_t border_radius = EGUI_THEME_RADIUS_SM;

    if (local->is_expanded)
    {
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        {
            egui_color_t color_light = egui_rgb_mix(bg_color, EGUI_COLOR_WHITE, 80);
            egui_gradient_stop_t stops[2] = {
                    {.position = 0, .color = color_light},
                    {.position = 255, .color = bg_color},
            };
            egui_gradient_t grad = {
                    .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                    .stop_count = 2,
                    .alpha = local->alpha,
                    .stops = stops,
            };
            egui_canvas_draw_round_rectangle_fill_gradient(region.location.x, region.location.y, region.size.width, region.size.height, border_radius, &grad);
        }
#else
        egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, border_radius, bg_color,
                                              local->alpha);
#endif
        egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, border_radius, 1, border_color,
                                         local->alpha);
    }
    else
    {
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        {
            egui_color_t color_light = egui_rgb_mix(bg_color, EGUI_COLOR_WHITE, 80);
            egui_gradient_stop_t stops[2] = {
                    {.position = 0, .color = color_light},
                    {.position = 255, .color = bg_color},
            };
            egui_gradient_t grad = {
                    .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                    .stop_count = 2,
                    .alpha = local->alpha,
                    .stops = stops,
            };
            egui_canvas_draw_round_rectangle_fill_gradient(region.location.x, region.location.y, region.size.width, header_height, border_radius, &grad);
        }
#else
        egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, header_height, border_radius, bg_color, local->alpha);
#endif
        egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, header_height, border_radius, 1, border_color, local->alpha);
    }

    if (is_focused && is_enabled)
    {
        egui_dim_t focus_height = local->is_expanded ? region.size.height : header_height;

        if (region.size.width > 4 && focus_height > 4)
        {
            egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, focus_height, border_radius, 2, focus_color,
                                             egui_color_alpha_mix(local->alpha, 100));
            egui_canvas_draw_round_rectangle(region.location.x + 2, region.location.y + 2, region.size.width - 4, focus_height - 4,
                                             border_radius > 2 ? (border_radius - 2) : border_radius, 1, focus_color, egui_color_alpha_mix(local->alpha, 56));
        }
    }

    egui_region_t text_rect;
    text_rect.location.x = region.location.x + padding;
    text_rect.location.y = region.location.y;
    text_rect.size.width = region.size.width - padding * 2 - arrow_slot_width;
    if (text_rect.size.width < 0)
    {
        text_rect.size.width = 0;
    }
    text_rect.size.height = header_height;

    if (local->items != NULL && local->item_count > 0)
    {
        const char *current_text = local->items[local->current_index];
        const char *current_icon = (local->item_icons != NULL) ? local->item_icons[local->current_index] : NULL;
        egui_view_combobox_draw_item_content(local, &text_rect, current_icon, current_text, text_color, local->alpha);
    }
    else if (local->item_icons != NULL && local->item_count > 0)
    {
        egui_view_combobox_draw_item_content(local, &text_rect, local->item_icons[local->current_index], NULL, text_color, local->alpha);
    }

    egui_dim_t arrow_x = region.location.x + region.size.width - arrow_slot_width;
    egui_view_combobox_draw_arrow(local, arrow_x, region.location.y, arrow_slot_width, header_height, local->is_expanded, arrow_color, local->alpha);

    if (local->is_expanded && local->items != NULL)
    {
        egui_dim_t item_y = region.location.y + header_height;
        uint8_t visible_count = egui_view_combobox_get_current_visible_count(self, local);
        uint8_t start_index = egui_view_combobox_get_visible_start_index(local, visible_count);

        for (uint8_t i = 0; i < visible_count; i++)
        {
            uint8_t item_index = (uint8_t)(start_index + i);
            egui_region_t item_rect;
            egui_color_t item_text_color = text_color;
            item_rect.location.x = region.location.x + padding;
            item_rect.location.y = item_y;
            item_rect.size.width = region.size.width - padding * 2;
            item_rect.size.height = local->item_height;

            if (item_index == local->current_index)
            {
                egui_canvas_draw_rectangle_fill(region.location.x + 1, item_y, region.size.width - 2, local->item_height, highlight_color, local->alpha);
                item_text_color = EGUI_COLOR_WHITE;
            }

            egui_view_combobox_draw_item_content(local, &item_rect, (local->item_icons != NULL) ? local->item_icons[item_index] : NULL,
                                                 (local->items != NULL) ? local->items[item_index] : NULL, item_text_color, local->alpha);

            item_y += local->item_height;
        }
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
int egui_view_combobox_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);

    if (self->is_enable == false || (local->items == NULL && local->item_icons == NULL) || local->item_count == 0)
    {
        return 0;
    }

    if (event->type == EGUI_MOTION_EVENT_ACTION_DOWN)
    {
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (self->is_focusable)
        {
            egui_view_request_focus(self);
        }
#endif
        return 1;
    }

    if (event->type == EGUI_MOTION_EVENT_ACTION_UP)
    {
        egui_dim_t local_y = event->location.y - self->region_screen.location.y;

        if (local->is_expanded)
        {
            if (local_y < local->collapsed_height)
            {
                egui_view_combobox_collapse(self);
            }
            else
            {
                egui_dim_t item_y = local_y - local->collapsed_height;
                uint8_t clicked_index = item_y / local->item_height;
                uint8_t visible_count = egui_view_combobox_get_current_visible_count(self, local);
                uint8_t start_index = egui_view_combobox_get_visible_start_index(local, visible_count);

                if (clicked_index < visible_count)
                {
                    egui_view_combobox_commit_current_index(self, (uint8_t)(start_index + clicked_index), 0);
                    egui_view_combobox_collapse(self);
                    egui_view_combobox_notify_selected(self);
                }
            }
        }
        else
        {
            egui_view_combobox_expand(self);
        }
    }

    return 1;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_combobox_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    uint8_t count = local->item_count;
    uint8_t limit_count = count;
    uint8_t next_index;

    if (!egui_view_get_enable(self) || count == 0)
    {
        return 0;
    }

    if (event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        switch (event->key_code)
        {
        case EGUI_KEY_CODE_UP:
        case EGUI_KEY_CODE_DOWN:
        case EGUI_KEY_CODE_HOME:
        case EGUI_KEY_CODE_END:
        case EGUI_KEY_CODE_ENTER:
        case EGUI_KEY_CODE_SPACE:
        case EGUI_KEY_CODE_ESCAPE:
            return 1;
        default:
            return 0;
        }
    }

    next_index = local->current_index;
    if (next_index >= limit_count)
    {
        next_index = (uint8_t)(limit_count - 1);
    }

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_DOWN:
        if (!local->is_expanded)
        {
            egui_view_combobox_expand(self);
            return 1;
        }
        if (next_index + 1 < limit_count)
        {
            next_index++;
        }
        egui_view_combobox_commit_current_index(self, next_index, 1);
        return 1;
    case EGUI_KEY_CODE_UP:
        if (!local->is_expanded)
        {
            egui_view_combobox_expand(self);
            return 1;
        }
        if (next_index > 0)
        {
            next_index--;
        }
        egui_view_combobox_commit_current_index(self, next_index, 1);
        return 1;
    case EGUI_KEY_CODE_HOME:
        egui_view_combobox_commit_current_index(self, 0, 1);
        return 1;
    case EGUI_KEY_CODE_END:
        egui_view_combobox_commit_current_index(self, (uint8_t)(limit_count - 1), 1);
        return 1;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        if (local->is_expanded)
        {
            egui_view_combobox_collapse(self);
            egui_view_combobox_notify_selected(self);
        }
        else
        {
            egui_view_combobox_expand(self);
        }
        return 1;
    case EGUI_KEY_CODE_ESCAPE:
        if (local->is_expanded)
        {
            egui_view_combobox_collapse(self);
            return 1;
        }
        return 0;
    default:
        return egui_view_on_key_event(self, event);
    }
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_combobox_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_combobox_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_combobox_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_combobox_on_key_event,
#endif
};

void egui_view_combobox_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_combobox_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_combobox_t);

    // init local data.
    local->on_selected = NULL;
    local->items = NULL;
    local->item_icons = NULL;
    local->item_count = 0;
    local->current_index = 0;
    local->is_expanded = 0;
    local->max_visible_items = 5;

    local->alpha = EGUI_ALPHA_100;
    local->text_color = EGUI_THEME_TEXT_PRIMARY;
    local->bg_color = EGUI_THEME_SURFACE;
    local->border_color = EGUI_THEME_BORDER;
    local->highlight_color = EGUI_THEME_PRIMARY;
    local->arrow_color = EGUI_THEME_TEXT_SECONDARY;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->icon_font = NULL;
    local->expand_icon = EGUI_ICON_MS_EXPAND_MORE;
    local->collapse_icon = EGUI_ICON_MS_EXPAND_LESS;

    local->collapsed_height = 30;
    local->item_height = 25;
    local->icon_text_gap = 6;

    egui_view_set_view_name(self, "egui_view_combobox");
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(self, true);
#endif
}

void egui_view_combobox_apply_params(egui_view_t *self, const egui_view_combobox_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);

    self->region = params->region;
    local->collapsed_height = params->region.size.height;

    local->items = params->items;
    local->item_icons = params->item_icons;
    local->item_count = params->item_count;
    local->current_index = params->current_index;
    if (local->item_count == 0 || local->current_index >= local->item_count)
    {
        local->current_index = 0;
    }

    egui_view_invalidate(self);
}

void egui_view_combobox_init_with_params(egui_view_t *self, const egui_view_combobox_params_t *params)
{
    egui_view_combobox_init(self);
    egui_view_combobox_apply_params(self, params);
}
