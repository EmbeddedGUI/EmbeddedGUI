#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_list.h"
#include "core/egui_core.h"
#include "core/egui_api.h"
#include "egui_view_icon_font.h"
#include "font/egui_font.h"
#include "resource/egui_resource.h"
#include "style/egui_theme.h"

#define EGUI_VIEW_LIST_ITEM_MARGIN_X    6
#define EGUI_VIEW_LIST_ITEM_MARGIN_Y    3
#define EGUI_VIEW_LIST_TEXT_PADDING     12
#define EGUI_VIEW_LIST_ICON_GAP_DEFAULT 8
#define EGUI_VIEW_LIST_SELECTION_INSET  2
#define EGUI_VIEW_LIST_SELECTION_STROKE 2

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
void egui_view_scroll_draw(egui_view_t *self);
#endif

/**
 * @file egui_view_list.c
 * @brief Fixed-capacity scrollable list that reuses button rows and custom row drawing.
 *
 * Reading notes:
 * - each logical
 * row owns an internal button for press and click behavior;
 * - icon and text painting is centralized here so all rows align consistently;
 * - layout is
 * delegated to the embedded scroll container after row sizes update.
 */

/** Resolve the icon font, falling back to an automatic size based on row height. */
static const egui_font_t *egui_view_list_resolve_icon_font(egui_view_list_t *local)
{
    if (local->icon_font != NULL)
    {
        return local->icon_font;
    }

    return egui_view_icon_font_get_auto(local->item_height, 30, 36);
}

static void egui_view_list_clear_item_refs(egui_view_list_t *local)
{
    uint8_t i;

    for (i = 0; i < EGUI_VIEW_LIST_MAX_ITEMS; i++)
    {
        local->item_icons[i] = NULL;
        local->item_texts[i] = NULL;
    }
}

/** Measure the widest icon slot needed across all rows that currently show icons. */
static egui_dim_t egui_view_list_get_icon_area_width(egui_core_t *core, egui_view_list_t *local)
{
    const egui_font_t *icon_font = egui_view_list_resolve_icon_font(local);
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

            egui_font_get_str_size_with_core(icon_font, core, local->item_icons[i], 0, 0, &icon_width, &icon_height);
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

/** Return the usable row width after outer margins and optional scrollbar reserve. */
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

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static uint8_t egui_view_list_normalize_selected_index(egui_view_list_t *local)
{
    if (local->item_count == 0)
    {
        local->selected_index = EGUI_VIEW_LIST_SELECTED_NONE;
    }
    else if (local->selected_index == EGUI_VIEW_LIST_SELECTED_NONE || local->selected_index >= local->item_count)
    {
        local->selected_index = 0;
    }

    return local->selected_index;
}
#endif

static void egui_view_list_set_selected_index_internal(egui_view_t *self, uint8_t index, uint8_t ensure_visible)
{
    EGUI_LOCAL_INIT(egui_view_list_t);

    if (local->item_count == 0)
    {
        local->selected_index = EGUI_VIEW_LIST_SELECTED_NONE;
        egui_view_invalidate(self);
        return;
    }

    if (index >= local->item_count)
    {
        return;
    }

    if (local->selected_index != index)
    {
        if (local->selected_index != EGUI_VIEW_LIST_SELECTED_NONE && local->selected_index < local->item_count)
        {
            egui_view_set_pressed(EGUI_VIEW_OF(&local->items[local->selected_index]), false);
        }
        local->selected_index = index;
        egui_view_invalidate(self);
    }

    if (ensure_visible)
    {
        egui_view_t *container = EGUI_VIEW_OF(&local->base.container);
        egui_view_t *item_view = EGUI_VIEW_OF(&local->items[index]);
        egui_dim_t item_top = container->region.location.y + item_view->region.location.y - item_view->margin.top;
        egui_dim_t item_bottom = container->region.location.y + item_view->region.location.y + item_view->region.size.height + item_view->margin.bottom;

        if (item_top < 0)
        {
            egui_view_scroll_start_container_scroll(self, -item_top);
        }
        else if (item_bottom > self->region.size.height)
        {
            egui_view_scroll_start_container_scroll(self, self->region.size.height - item_bottom);
        }
    }
}

/** Sync one row button's size and basic label alignment with current list settings. */
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

/** Refresh every row size, then ask the scroll container to relayout them vertically. */
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

/** Draw row icons and labels on top of the button backgrounds inside the list clip. */
static void egui_view_list_draw_item_contents(egui_view_t *self)
{
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    EGUI_LOCAL_INIT(egui_view_list_t);
    const egui_font_t *icon_font = egui_view_list_resolve_icon_font(local);
    const egui_region_t *prev_clip = egui_canvas_get_extra_clip(canvas);
    egui_dim_t icon_width = egui_view_list_get_icon_area_width(egui_view_get_core(self), local);
    uint8_t i;

    if (icon_font == NULL)
    {
        icon_width = 0;
    }

    egui_canvas_set_extra_clip(canvas, &self->region_screen);

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

        egui_canvas_clear_mask(canvas);
        egui_canvas_calc_work_region(canvas, &item_view->region_screen);
        if (!egui_region_is_empty(egui_canvas_get_base_view_work_region(canvas)))
        {
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
            if (self->is_focused && local->selected_index == i && item_view->region.size.width > (EGUI_VIEW_LIST_SELECTION_INSET * 2 + 4) &&
                item_view->region.size.height > (EGUI_VIEW_LIST_SELECTION_INSET * 2 + 4))
            {
                egui_canvas_draw_round_rectangle_fill(canvas, EGUI_VIEW_LIST_SELECTION_INSET, EGUI_VIEW_LIST_SELECTION_INSET,
                                                      item_view->region.size.width - (EGUI_VIEW_LIST_SELECTION_INSET * 2),
                                                      item_view->region.size.height - (EGUI_VIEW_LIST_SELECTION_INSET * 2), EGUI_THEME_RADIUS_SM,
                                                      EGUI_THEME_WARNING, EGUI_ALPHA_20);
            }
#endif

            if (local->item_icons[i] != NULL && icon_width > 0)
            {
                icon_region.location.x = EGUI_VIEW_LIST_TEXT_PADDING;
                icon_region.location.y = 0;
                icon_region.size.width = icon_width;
                icon_region.size.height = item_view->region.size.height;

                icon_color = egui_view_get_enable(item_view) ? local->icon_color : EGUI_THEME_DISABLED;
                egui_canvas_draw_text_in_rect(canvas, icon_font, local->item_icons[i], &icon_region, EGUI_ALIGN_CENTER, icon_color, item_view->alpha);
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
                    egui_canvas_draw_text_in_rect(canvas, text_font, local->item_texts[i], &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color,
                                                  text_alpha);
                }
            }

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
            if (self->is_focused && local->selected_index == i && item_view->region.size.width > (EGUI_VIEW_LIST_SELECTION_INSET * 2 + 4) &&
                item_view->region.size.height > (EGUI_VIEW_LIST_SELECTION_INSET * 2 + 4))
            {
                egui_dim_t x = EGUI_VIEW_LIST_SELECTION_INSET;
                egui_dim_t y = EGUI_VIEW_LIST_SELECTION_INSET;
                egui_dim_t w = item_view->region.size.width - (EGUI_VIEW_LIST_SELECTION_INSET * 2);
                egui_dim_t h = item_view->region.size.height - (EGUI_VIEW_LIST_SELECTION_INSET * 2);
                egui_dim_t inner_inset = EGUI_VIEW_LIST_SELECTION_INSET + EGUI_VIEW_LIST_SELECTION_STROKE + 1;

                egui_canvas_draw_round_rectangle(canvas, x, y, w, h, EGUI_THEME_RADIUS_SM, EGUI_VIEW_LIST_SELECTION_STROKE, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
                if (item_view->region.size.width > (inner_inset * 2 + 2) && item_view->region.size.height > (inner_inset * 2 + 2))
                {
                    egui_canvas_draw_round_rectangle(canvas, inner_inset, inner_inset, item_view->region.size.width - (inner_inset * 2),
                                                     item_view->region.size.height - (inner_inset * 2),
                                                     EGUI_THEME_RADIUS_SM > inner_inset ? (EGUI_THEME_RADIUS_SM - inner_inset) : 1, 1, EGUI_COLOR_WHITE,
                                                     EGUI_ALPHA_80);
                }
            }
#endif
        }
    }

    if (prev_clip != NULL)
    {
        egui_canvas_set_extra_clip(canvas, prev_clip);
    }
    else
    {
        egui_canvas_clear_extra_clip(canvas);
    }
}

/** Draw the scroll container first, then overlay the list's custom icon and text content. */
static void egui_view_list_draw(egui_view_t *self)
{
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    egui_alpha_t alpha = egui_canvas_get_alpha(canvas);

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    egui_view_scroll_draw(self);
#else
    egui_canvas_set_extra_clip(canvas, &self->region_screen);
    egui_view_group_draw(self);
    egui_canvas_clear_extra_clip(canvas);
#endif

    if (self->is_visible && !self->is_gone)
    {
        egui_canvas_mix_alpha(canvas, self->alpha);
        egui_view_list_draw_item_contents(self);
    }

    egui_canvas_set_alpha(canvas, alpha);
}

/** Translate one internal row-button click back into the owning list and row index. */
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

    btn = (egui_view_button_t *)self;
    list = (egui_view_list_t *)scroll_view;
    index = (uint8_t)(btn - &list->items[0]);
    if (index < list->item_count)
    {
        egui_view_list_set_selected_index_internal(scroll_view, index, 0);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        egui_view_request_focus(scroll_view);
#endif
        if (list->on_item_click != NULL)
        {
            list->on_item_click(scroll_view, index);
        }
    }
}

/** Create one internal row button, append borrowed icon/text data, and relayout the list. */
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

    egui_view_button_init(item_view, self->core);
    egui_view_label_set_font(item_view, (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    /* Row labels are drawn by the list itself so icon and text can share one layout rule. */
    egui_view_label_set_text(item_view, NULL);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(item_view, false);
#endif
    egui_view_set_position(item_view, 0, 0);
    egui_view_set_margin(item_view, EGUI_VIEW_LIST_ITEM_MARGIN_X, EGUI_VIEW_LIST_ITEM_MARGIN_X, EGUI_VIEW_LIST_ITEM_MARGIN_Y, EGUI_VIEW_LIST_ITEM_MARGIN_Y);
    egui_view_set_on_click_listener(item_view, egui_view_list_item_click_handler);

    local->item_icons[idx] = icon;
    local->item_texts[idx] = text;
    local->item_count++;
    if (local->selected_index == EGUI_VIEW_LIST_SELECTED_NONE)
    {
        local->selected_index = idx;
    }

    egui_view_list_update_item_style(self, idx);
    egui_view_scroll_add_child(self, item_view);
    egui_view_scroll_layout_childs(self);
    egui_view_invalidate(self);

    return (int8_t)idx;
}

/** Append one text-only row to the list. */
int8_t egui_view_list_add_item(egui_view_t *self, const char *text)
{
    return egui_view_list_add_item_internal(self, NULL, text);
}

/** Append one row that shows both an icon glyph and a text label. */
int8_t egui_view_list_add_item_with_icon(egui_view_t *self, const char *icon, const char *text)
{
    return egui_view_list_add_item_internal(self, icon, text);
}

/** Remove all rows, clear borrowed icon and text pointers, and redraw the widget. */
void egui_view_list_clear(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_list_t);

    egui_view_group_clear_childs(EGUI_VIEW_OF(&local->base.container));
    local->item_count = 0;
    local->selected_index = EGUI_VIEW_LIST_SELECTED_NONE;
    egui_view_list_clear_item_refs(local);

    egui_view_invalidate(self);
}

uint8_t egui_view_list_get_item_count(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_list_t);
    return local->item_count;
}

/** Update the shared row height and restyle all existing rows when it changes. */
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

egui_dim_t egui_view_list_get_item_height(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_list_t);
    return local->item_height;
}

/** Replace the icon glyph for one row and refresh layout that depends on icon width. */
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

const char *egui_view_list_get_item_icon(egui_view_t *self, uint8_t index)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_list_t);
    if (index >= local->item_count)
    {
        return NULL;
    }
    return local->item_icons[index];
}

const char *egui_view_list_get_item_text(egui_view_t *self, uint8_t index)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_list_t);
    if (index >= local->item_count)
    {
        return NULL;
    }
    return local->item_texts[index];
}

/** Override the icon font and restyle rows so icon-space measurement stays current. */
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

const egui_font_t *egui_view_list_get_icon_font(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_list_t);
    return local->icon_font;
}

/** Set the horizontal gap between an icon and its label, clamping negative values to zero. */
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

egui_dim_t egui_view_list_get_icon_text_gap(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_list_t);
    return local->icon_gap;
}

/** Set the tint color used for row icons. */
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

egui_color_t egui_view_list_get_icon_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_list_t);
    return local->icon_color;
}

/** Store the callback fired when an internal row button is clicked. */
void egui_view_list_set_on_item_click(egui_view_t *self, egui_view_list_item_click_cb_t callback)
{
    EGUI_LOCAL_INIT(egui_view_list_t);
    local->on_item_click = callback;
}

egui_view_list_item_click_cb_t egui_view_list_get_on_item_click(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_list_t);
    return local->on_item_click;
}

void egui_view_list_set_selected_index(egui_view_t *self, uint8_t index)
{
    egui_view_list_set_selected_index_internal(self, index, 1);
}

uint8_t egui_view_list_get_selected_index(egui_view_t *self)
{
    if (self == NULL)
    {
        return EGUI_VIEW_LIST_SELECTED_NONE;
    }
    EGUI_LOCAL_INIT(egui_view_list_t);
    return local->selected_index;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_list_move_selected_index(egui_view_t *self, uint8_t key_code)
{
    EGUI_LOCAL_INIT(egui_view_list_t);
    uint8_t old_index;
    uint8_t new_index;

    if (local->item_count == 0)
    {
        return 1;
    }

    old_index = egui_view_list_normalize_selected_index(local);
    new_index = old_index;

    switch (key_code)
    {
    case EGUI_KEY_CODE_UP:
        if (new_index > 0)
        {
            new_index--;
        }
        break;
    case EGUI_KEY_CODE_DOWN:
        if (new_index + 1 < local->item_count)
        {
            new_index++;
        }
        break;
    case EGUI_KEY_CODE_HOME:
        new_index = 0;
        break;
    case EGUI_KEY_CODE_END:
        new_index = (uint8_t)(local->item_count - 1);
        break;
    default:
        return 0;
    }

    egui_view_list_set_selected_index_internal(self, new_index, 1);
    return 1;
}

static void egui_view_list_set_selected_pressed(egui_view_t *self, uint8_t pressed)
{
    EGUI_LOCAL_INIT(egui_view_list_t);
    uint8_t selected_index = egui_view_list_normalize_selected_index(local);

    if (selected_index != EGUI_VIEW_LIST_SELECTED_NONE && selected_index < local->item_count)
    {
        egui_view_set_pressed(EGUI_VIEW_OF(&local->items[selected_index]), pressed != 0);
    }
}

static int egui_view_list_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_list_t);

    if (self->is_enable == false || event == NULL)
    {
        return 0;
    }

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_UP:
    case EGUI_KEY_CODE_DOWN:
    case EGUI_KEY_CODE_HOME:
    case EGUI_KEY_CODE_END:
        if (event->type == EGUI_KEY_EVENT_ACTION_DOWN)
        {
            return 1;
        }
        if (event->type == EGUI_KEY_EVENT_ACTION_UP || event->type == EGUI_KEY_EVENT_ACTION_REPEAT)
        {
            return egui_view_list_move_selected_index(self, event->key_code);
        }
        return 1;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        if (event->type == EGUI_KEY_EVENT_ACTION_DOWN)
        {
            egui_view_list_set_selected_pressed(self, 1);
            return 1;
        }
        if (event->type == EGUI_KEY_EVENT_ACTION_UP)
        {
            uint8_t selected_index = egui_view_list_normalize_selected_index(local);
            int should_click = 0;

            if (selected_index != EGUI_VIEW_LIST_SELECTED_NONE && selected_index < local->item_count)
            {
                egui_view_t *item_view = EGUI_VIEW_OF(&local->items[selected_index]);
                should_click = item_view->is_pressed;
                egui_view_set_pressed(item_view, false);
            }

            if (should_click && local->on_item_click != NULL)
            {
                local->on_item_click(self, selected_index);
            }
            return 1;
        }
        return 1;
    default:
        return egui_view_on_key_event(self, event);
    }
}
#endif

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
        .on_key_event = egui_view_list_on_key_event,
#endif
};

/** Initialize the list as a scroll view with zero rows and stock icon styling. */
void egui_view_list_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_list_t);

    egui_view_scroll_init(self, core);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_list_t);

    local->item_count = 0;
    local->item_height = 30;
    local->icon_gap = EGUI_VIEW_LIST_ICON_GAP_DEFAULT;
    local->icon_color = EGUI_THEME_TEXT_SECONDARY;
    local->icon_font = NULL;
    local->on_item_click = NULL;
    local->selected_index = EGUI_VIEW_LIST_SELECTED_NONE;
    egui_view_list_clear_item_refs(local);

    egui_view_set_view_name(self, "egui_view_list");
}

/** Apply geometry and shared row height from one parameter block. */
void egui_view_list_apply_params(egui_view_t *self, const egui_view_list_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_list_t);

    egui_view_set_position(self, params->region.location.x, params->region.location.y);
    egui_view_scroll_set_size(self, params->region.size.width, params->region.size.height);

    local->item_height = params->item_height;
    egui_view_list_update_all_items(self);
    egui_view_invalidate(self);
}

/** Convenience helper that initializes the list before applying params. */
void egui_view_list_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_list_params_t *params)
{
    egui_view_list_init(self, core);
    egui_view_list_apply_params(self, params);
}
