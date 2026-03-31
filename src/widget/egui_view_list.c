#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_list.h"
#include "egui_view_icon_font.h"
#include "font/egui_font.h"
#include "resource/egui_resource.h"
#include "style/egui_theme.h"

#define EGUI_VIEW_LIST_ITEM_MARGIN_X    6
#define EGUI_VIEW_LIST_ITEM_MARGIN_Y    3
#define EGUI_VIEW_LIST_TEXT_PADDING     12
#define EGUI_VIEW_LIST_ICON_GAP_DEFAULT 8

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
void egui_view_scroll_draw(egui_view_t *self);
#endif

static const egui_font_t *egui_view_list_get_icon_font(egui_view_list_t *local)
{
    if (local->icon_font != NULL)
    {
        return local->icon_font;
    }

    return egui_view_icon_font_get_auto(local->item_height, 30, 36);
}

static egui_dim_t egui_view_list_get_icon_area_width(egui_view_list_t *local)
{
    const egui_font_t *icon_font = egui_view_list_get_icon_font(local);
    egui_dim_t width = 0;
    egui_dim_t height = 0;
    uint8_t has_icon = 0;
    uint8_t i;

    if (icon_font == NULL)
    {
        return 0;
    }

    for (i = 0; i < local->item_count; i++)
    {
        if (local->item_icons[i] != NULL && local->item_icons[i][0] != '\0')
        {
            has_icon = 1;
            break;
        }
    }

    if (icon_font != NULL && icon_font->api != NULL && icon_font->api->get_str_size != NULL)
    {
        for (i = 0; i < local->item_count; i++)
        {
            egui_dim_t icon_width = 0;
            egui_dim_t icon_height = 0;

            if (local->item_icons[i] == NULL || local->item_icons[i][0] == '\0')
            {
                continue;
            }

            icon_font->api->get_str_size(icon_font, local->item_icons[i], 0, 0, &icon_width, &icon_height);
            if (icon_width > width)
            {
                width = icon_width;
            }
            if (icon_height > height)
            {
                height = icon_height;
            }
        }
    }

    if (width <= 0)
    {
        width = height;
    }
    if (width <= 0)
    {
        if (!has_icon)
        {
            return 0;
        }

        if (local->item_height <= 30)
        {
            width = 16;
        }
        else if (local->item_height <= 36)
        {
            width = 20;
        }
        else
        {
            width = 24;
        }
    }
    if (width > local->item_height)
    {
        width = local->item_height;
    }
    return width;
}

static egui_dim_t egui_view_list_get_item_width(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_list_t);

    egui_dim_t item_width = self->region.size.width - EGUI_VIEW_LIST_ITEM_MARGIN_X * 2;

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    if (local->base.is_scrollbar_enabled)
    {
        egui_dim_t scrollbar_reserve = EGUI_THEME_SCROLLBAR_THICKNESS + EGUI_THEME_SCROLLBAR_MARGIN * 2;
        if (item_width > scrollbar_reserve)
        {
            item_width -= scrollbar_reserve;
        }
        else
        {
            item_width = 0;
        }
    }
#endif

    if (item_width < 0)
    {
        item_width = 0;
    }

    return item_width;
}

static void egui_view_list_update_item_style(egui_view_t *self, uint8_t index)
{
    EGUI_LOCAL_INIT(egui_view_list_t);

    if (index >= local->item_count)
    {
        return;
    }

    egui_view_t *item_view = EGUI_VIEW_OF(&local->items[index]);

    egui_view_set_size(item_view, egui_view_list_get_item_width(self), local->item_height);
    egui_view_set_padding(item_view, 0, 0, 0, 0);
    egui_view_label_set_align_type(item_view, EGUI_ALIGN_CENTER);
}

static void egui_view_list_update_all_items(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_list_t);
    uint8_t i;

    for (i = 0; i < local->item_count; i++)
    {
        egui_view_list_update_item_style(self, i);
    }

    egui_view_scroll_layout_childs(self);
}

static void egui_view_list_draw_item_contents(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_list_t);
    const egui_font_t *icon_font = egui_view_list_get_icon_font(local);
    const egui_region_t *prev_clip = egui_canvas_get_extra_clip();
    egui_dim_t icon_width = egui_view_list_get_icon_area_width(local);
    uint8_t i;

    if (icon_font == NULL)
    {
        icon_width = 0;
    }

    egui_canvas_set_extra_clip(&self->region_screen);

    for (i = 0; i < local->item_count; i++)
    {
        egui_view_t *item_view;
        egui_view_button_t *item_button;
        egui_region_t icon_region;
        egui_region_t text_region;
        egui_color_t icon_color;
        egui_color_t text_color;
        egui_alpha_t text_alpha;
        const egui_font_t *text_font;

        if (local->item_icons[i] == NULL && local->item_texts[i] == NULL)
        {
            continue;
        }

        item_view = EGUI_VIEW_OF(&local->items[i]);
        if (!item_view->is_visible || item_view->is_gone || item_view->region_screen.size.width <= 0 || item_view->region_screen.size.height <= 0)
        {
            continue;
        }

        item_button = &local->items[i];
        text_font = item_button->base.font ? item_button->base.font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
        text_color = egui_view_get_enable(item_view) ? item_button->base.color : EGUI_THEME_DISABLED;
        text_alpha = item_button->base.alpha;

        egui_canvas_clear_mask();
        egui_canvas_calc_work_region(&item_view->region_screen);
        if (!egui_region_is_empty(egui_canvas_get_base_view_work_region()))
        {
            if (local->item_icons[i] != NULL && icon_width > 0)
            {
                icon_region.location.x = EGUI_VIEW_LIST_TEXT_PADDING;
                icon_region.location.y = 0;
                icon_region.size.width = icon_width;
                icon_region.size.height = item_view->region.size.height;

                icon_color = egui_view_get_enable(item_view) ? local->icon_color : EGUI_THEME_DISABLED;
                egui_canvas_draw_text_in_rect(icon_font, local->item_icons[i], &icon_region, EGUI_ALIGN_CENTER, icon_color, item_view->alpha);
            }

            if (local->item_texts[i] != NULL)
            {
                text_region.location.x = EGUI_VIEW_LIST_TEXT_PADDING;
                if (local->item_icons[i] != NULL && icon_width > 0)
                {
                    text_region.location.x += icon_width + local->icon_gap;
                }
                text_region.location.y = 0;
                text_region.size.width = item_view->region.size.width - text_region.location.x - EGUI_VIEW_LIST_TEXT_PADDING;
                text_region.size.height = item_view->region.size.height;

                if (text_region.size.width > 0)
                {
                    egui_canvas_draw_text_in_rect(text_font, local->item_texts[i], &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color, text_alpha);
                }
            }
        }
    }

    if (prev_clip != NULL)
    {
        egui_canvas_set_extra_clip(prev_clip);
    }
    else
    {
        egui_canvas_clear_extra_clip();
    }
}

static void egui_view_list_draw(egui_view_t *self)
{
    egui_alpha_t alpha = egui_canvas_get_alpha();

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    egui_view_scroll_draw(self);
#else
    egui_canvas_set_extra_clip(&self->region_screen);
    egui_view_group_draw(self);
    egui_canvas_clear_extra_clip();
#endif

    if (self->is_visible && !self->is_gone)
    {
        egui_canvas_mix_alpha(self->alpha);
        egui_view_list_draw_item_contents(self);
    }

    egui_canvas_set_alpha(alpha);
}

static void egui_view_list_item_click_handler(egui_view_t *self)
{
    egui_view_t *container = (egui_view_t *)self->parent;
    egui_view_t *scroll_view;
    egui_view_list_t *list;
    egui_view_button_t *btn;
    uint8_t index;

    if (container == NULL)
    {
        return;
    }

    scroll_view = (egui_view_t *)container->parent;
    if (scroll_view == NULL)
    {
        return;
    }

    list = (egui_view_list_t *)scroll_view;
    if (list->on_item_click == NULL)
    {
        return;
    }

    btn = (egui_view_button_t *)self;
    index = (uint8_t)(btn - &list->items[0]);
    if (index < list->item_count)
    {
        list->on_item_click(scroll_view, index);
    }
}

static int8_t egui_view_list_add_item_internal(egui_view_t *self, const char *icon, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_list_t);
    uint8_t idx;
    egui_view_t *item_view;

    if (local->item_count >= EGUI_VIEW_LIST_MAX_ITEMS)
    {
        return -1;
    }

    idx = local->item_count;
    item_view = EGUI_VIEW_OF(&local->items[idx]);

    egui_view_button_init(item_view);
    egui_view_label_set_font(item_view, (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_text(item_view, NULL);
    egui_view_set_position(item_view, 0, 0);
    egui_view_set_margin(item_view, EGUI_VIEW_LIST_ITEM_MARGIN_X, EGUI_VIEW_LIST_ITEM_MARGIN_X, EGUI_VIEW_LIST_ITEM_MARGIN_Y, EGUI_VIEW_LIST_ITEM_MARGIN_Y);
    egui_view_set_on_click_listener(item_view, egui_view_list_item_click_handler);

    local->item_icons[idx] = icon;
    local->item_texts[idx] = text;
    local->item_count++;

    egui_view_list_update_item_style(self, idx);
    egui_view_scroll_add_child(self, item_view);
    egui_view_scroll_layout_childs(self);
    egui_view_invalidate(self);

    return (int8_t)idx;
}

int8_t egui_view_list_add_item(egui_view_t *self, const char *text)
{
    return egui_view_list_add_item_internal(self, NULL, text);
}

int8_t egui_view_list_add_item_with_icon(egui_view_t *self, const char *icon, const char *text)
{
    return egui_view_list_add_item_internal(self, icon, text);
}

void egui_view_list_clear(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_list_t);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&local->base.container));
    local->item_count = 0;
    memset(local->item_icons, 0, sizeof(local->item_icons));
    memset(local->item_texts, 0, sizeof(local->item_texts));

    egui_view_invalidate(self);
}

void egui_view_list_set_item_height(egui_view_t *self, egui_dim_t height)
{
    EGUI_LOCAL_INIT(egui_view_list_t);

    if (local->item_height == height)
    {
        return;
    }

    local->item_height = height;
    egui_view_list_update_all_items(self);
    egui_view_invalidate(self);
}

void egui_view_list_set_item_icon(egui_view_t *self, uint8_t index, const char *icon)
{
    EGUI_LOCAL_INIT(egui_view_list_t);

    if (index >= local->item_count)
    {
        return;
    }

    local->item_icons[index] = icon;
    egui_view_list_update_item_style(self, index);
    egui_view_scroll_layout_childs(self);
    egui_view_invalidate(self);
}

void egui_view_list_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_list_t);

    if (local->icon_font == font)
    {
        return;
    }

    local->icon_font = font;
    egui_view_list_update_all_items(self);
    egui_view_invalidate(self);
}

void egui_view_list_set_icon_text_gap(egui_view_t *self, egui_dim_t gap)
{
    EGUI_LOCAL_INIT(egui_view_list_t);

    if (gap < 0)
    {
        gap = 0;
    }

    if (local->icon_gap == gap)
    {
        return;
    }

    local->icon_gap = gap;
    egui_view_list_update_all_items(self);
    egui_view_invalidate(self);
}

void egui_view_list_set_icon_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_list_t);

    if (local->icon_color.full == color.full)
    {
        return;
    }

    local->icon_color = color;
    egui_view_invalidate(self);
}

void egui_view_list_set_on_item_click(egui_view_t *self, egui_view_list_item_click_cb_t callback)
{
    EGUI_LOCAL_INIT(egui_view_list_t);
    local->on_item_click = callback;
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_list_t) = {
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .dispatch_touch_event = egui_view_group_dispatch_touch_event,
        .on_touch_event = egui_view_scroll_on_touch_event,
        .on_intercept_touch_event = egui_view_scroll_on_intercept_touch_event,
#else
        .dispatch_touch_event = egui_view_group_dispatch_touch_event,
        .on_touch_event = NULL,
        .on_intercept_touch_event = NULL,
#endif
        .compute_scroll = egui_view_scroll_compute_scroll,
        .calculate_layout = egui_view_group_calculate_layout,
        .request_layout = egui_view_group_request_layout,
        .draw = egui_view_list_draw,
        .on_attach_to_window = egui_view_group_on_attach_to_window,
        .on_draw = egui_view_on_draw,
        .on_detach_from_window = egui_view_group_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_group_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_list_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_list_t);

    egui_view_scroll_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_list_t);

    local->item_count = 0;
    local->item_height = 30;
    local->icon_gap = EGUI_VIEW_LIST_ICON_GAP_DEFAULT;
    local->icon_color = EGUI_THEME_TEXT_SECONDARY;
    local->icon_font = NULL;
    local->on_item_click = NULL;
    memset(local->item_icons, 0, sizeof(local->item_icons));
    memset(local->item_texts, 0, sizeof(local->item_texts));

    egui_view_set_view_name(self, "egui_view_list");
}

void egui_view_list_apply_params(egui_view_t *self, const egui_view_list_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_list_t);

    egui_view_set_position(self, params->region.location.x, params->region.location.y);
    egui_view_scroll_set_size(self, params->region.size.width, params->region.size.height);

    local->item_height = params->item_height;
    egui_view_list_update_all_items(self);
    egui_view_invalidate(self);
}

void egui_view_list_init_with_params(egui_view_t *self, const egui_view_list_params_t *params)
{
    egui_view_list_init(self);
    egui_view_list_apply_params(self, params);
}
