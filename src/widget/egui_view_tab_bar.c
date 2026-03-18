#include <stdio.h>
#include <assert.h>

#include "egui_view_tab_bar.h"
#include "core/egui_canvas_gradient.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"

static const egui_font_t *egui_view_tab_bar_get_icon_font(egui_view_tab_bar_t *local, egui_dim_t area_size)
{
    if (local->icon_font != NULL)
    {
        return local->icon_font;
    }

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

void egui_view_tab_bar_set_tabs(egui_view_t *self, const char **tab_texts, uint8_t tab_count)
{
    EGUI_LOCAL_INIT(egui_view_tab_bar_t);
    local->tab_texts = tab_texts;
    local->tab_count = tab_count;
    if (local->current_index >= tab_count)
    {
        local->current_index = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_tab_bar_set_current_index(egui_view_t *self, uint8_t index)
{
    EGUI_LOCAL_INIT(egui_view_tab_bar_t);
    if (index >= local->tab_count)
    {
        return;
    }
    if (local->current_index == index)
    {
        return;
    }
    local->current_index = index;
    if (local->on_tab_changed)
    {
        local->on_tab_changed(self, index);
    }
    egui_view_invalidate(self);
}

void egui_view_tab_bar_set_on_tab_changed_listener(egui_view_t *self, egui_view_on_tab_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_tab_bar_t);
    local->on_tab_changed = listener;
}

void egui_view_tab_bar_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_tab_bar_t);
    local->font = font;
    egui_view_invalidate(self);
}

void egui_view_tab_bar_set_tab_icons(egui_view_t *self, const char **tab_icons)
{
    EGUI_LOCAL_INIT(egui_view_tab_bar_t);
    if (local->tab_icons == tab_icons)
    {
        return;
    }

    local->tab_icons = tab_icons;
    egui_view_invalidate(self);
}

void egui_view_tab_bar_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_tab_bar_t);
    if (local->icon_font == font)
    {
        return;
    }

    local->icon_font = font;
    egui_view_invalidate(self);
}

void egui_view_tab_bar_set_icon_text_gap(egui_view_t *self, egui_dim_t gap)
{
    EGUI_LOCAL_INIT(egui_view_tab_bar_t);
    if (local->icon_text_gap == gap)
    {
        return;
    }

    local->icon_text_gap = gap;
    egui_view_invalidate(self);
}

void egui_view_tab_bar_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_tab_bar_t);

    if (local->tab_count == 0 || (local->tab_texts == NULL && local->tab_icons == NULL) || local->font == NULL)
    {
        return;
    }

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t tab_w = region.size.width / local->tab_count;
    egui_dim_t indicator_h = 3;

    uint8_t i;
    for (i = 0; i < local->tab_count; i++)
    {
        egui_region_t tab_rect;
        const char *tab_text = (local->tab_texts != NULL) ? local->tab_texts[i] : NULL;
        const char *tab_icon = (local->tab_icons != NULL) ? local->tab_icons[i] : NULL;
        if (tab_text == NULL)
        {
            tab_text = "";
        }
        tab_rect.location.x = region.location.x + i * tab_w;
        tab_rect.location.y = region.location.y;
        tab_rect.size.width = tab_w;
        tab_rect.size.height = region.size.height - indicator_h;

        egui_color_t color = (i == local->current_index) ? local->active_text_color : local->text_color;
        if (tab_icon != NULL)
        {
            if (tab_text != NULL && tab_text[0] != '\0')
            {
                egui_dim_t text_w = 0;
                egui_dim_t text_h = 0;
                egui_dim_t text_gap = local->icon_text_gap;
                egui_dim_t icon_area_height;
                egui_dim_t content_height;
                egui_dim_t content_y;
                egui_region_t icon_rect;
                egui_region_t text_rect;

                if (local->font->api != NULL)
                {
                    local->font->api->get_str_size(local->font, tab_text, 0, 0, &text_w, &text_h);
                }
                if (text_h < 0)
                {
                    text_h = 0;
                }
                if (text_gap < 0)
                {
                    text_gap = 0;
                }

                icon_area_height = tab_rect.size.height - text_h - text_gap;
                if (icon_area_height < tab_rect.size.height / 2)
                {
                    icon_area_height = tab_rect.size.height / 2;
                }
                if (icon_area_height > tab_rect.size.height)
                {
                    icon_area_height = tab_rect.size.height;
                }

                content_height = icon_area_height + ((text_h > 0) ? (text_h + text_gap) : 0);
                if (content_height > tab_rect.size.height)
                {
                    content_height = tab_rect.size.height;
                }
                content_y = tab_rect.location.y + (tab_rect.size.height - content_height) / 2;

                icon_rect.location.x = tab_rect.location.x;
                icon_rect.location.y = content_y;
                icon_rect.size.width = tab_rect.size.width;
                icon_rect.size.height = icon_area_height;

                text_rect.location.x = tab_rect.location.x;
                text_rect.location.y = content_y + icon_area_height;
                text_rect.size.width = tab_rect.size.width;
                text_rect.size.height = tab_rect.location.y + tab_rect.size.height - text_rect.location.y;

                egui_canvas_draw_text_in_rect(egui_view_tab_bar_get_icon_font(local, icon_area_height), tab_icon, &icon_rect, EGUI_ALIGN_CENTER, color,
                                              local->alpha);
                egui_canvas_draw_text_in_rect(local->font, tab_text, &text_rect, EGUI_ALIGN_CENTER, color, local->alpha);
            }
            else
            {
                egui_canvas_draw_text_in_rect(egui_view_tab_bar_get_icon_font(local, tab_rect.size.height), tab_icon, &tab_rect, EGUI_ALIGN_CENTER, color,
                                              local->alpha);
            }
        }
        else
        {
            egui_canvas_draw_text_in_rect(local->font, tab_text, &tab_rect, EGUI_ALIGN_CENTER, color, local->alpha);
        }

        // Draw indicator under current tab
        if (i == local->current_index)
        {
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
            {
                egui_color_t color_light = egui_rgb_mix(local->indicator_color, EGUI_COLOR_WHITE, 80);
                egui_gradient_stop_t stops[2] = {
                        {.position = 0, .color = color_light},
                        {.position = 255, .color = local->indicator_color},
                };
                egui_gradient_t grad = {
                        .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                        .stop_count = 2,
                        .alpha = local->alpha,
                        .stops = stops,
                };
                egui_canvas_draw_rectangle_fill_gradient(tab_rect.location.x, region.location.y + region.size.height - indicator_h, tab_w, indicator_h, &grad);
            }
#else
            egui_canvas_draw_rectangle_fill(tab_rect.location.x, region.location.y + region.size.height - indicator_h, tab_w, indicator_h,
                                            local->indicator_color, local->alpha);
#endif
        }
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static uint8_t egui_view_tab_bar_get_hit_index(egui_view_t *self, egui_view_tab_bar_t *local, egui_dim_t touch_x, egui_dim_t touch_y)
{
    egui_dim_t local_x;
    egui_dim_t tab_w;

    if (touch_x < self->region_screen.location.x || touch_y < self->region_screen.location.y ||
        touch_x >= self->region_screen.location.x + self->region_screen.size.width || touch_y >= self->region_screen.location.y + self->region_screen.size.height)
    {
        return EGUI_VIEW_TAB_BAR_PRESSED_NONE;
    }

    local_x = touch_x - self->region_screen.location.x;
    tab_w = self->region.size.width / local->tab_count;
    if (tab_w <= 0)
    {
        return EGUI_VIEW_TAB_BAR_PRESSED_NONE;
    }

    {
        uint8_t index = (uint8_t)(local_x / tab_w);
        if (index >= local->tab_count)
        {
            index = local->tab_count - 1;
        }
        return index;
    }
}

int egui_view_tab_bar_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_tab_bar_t);
    uint8_t hit_index = egui_view_tab_bar_get_hit_index(self, local, event->location.x, event->location.y);

    if (self->is_enable == false || local->tab_count == 0)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
    {
        local->pressed_index = hit_index;
        egui_view_set_pressed(self, hit_index != EGUI_VIEW_TAB_BAR_PRESSED_NONE);
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_MOVE:
    {
        egui_view_set_pressed(self, local->pressed_index != EGUI_VIEW_TAB_BAR_PRESSED_NONE && local->pressed_index == hit_index);
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_UP:
    {
        int was_pressed = self->is_pressed;
        egui_view_set_pressed(self, false);
        if (!was_pressed || local->pressed_index == EGUI_VIEW_TAB_BAR_PRESSED_NONE || local->pressed_index != hit_index)
        {
            local->pressed_index = EGUI_VIEW_TAB_BAR_PRESSED_NONE;
            break;
        }

        egui_view_tab_bar_set_current_index(self, hit_index);
        local->pressed_index = EGUI_VIEW_TAB_BAR_PRESSED_NONE;
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
    {
        local->pressed_index = EGUI_VIEW_TAB_BAR_PRESSED_NONE;
        egui_view_set_pressed(self, false);
        break;
    }
    default:
        break;
    }

    return 1;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_tab_bar_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_tab_bar_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_tab_bar_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_tab_bar_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_tab_bar_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_tab_bar_t);

    // init local data.
    local->on_tab_changed = NULL;
    local->tab_texts = NULL;
    local->tab_icons = NULL;
    local->tab_count = 0;
    local->current_index = 0;
    local->pressed_index = EGUI_VIEW_TAB_BAR_PRESSED_NONE;
    local->alpha = EGUI_ALPHA_100;
    local->text_color = EGUI_THEME_TEXT_SECONDARY;
    local->active_text_color = EGUI_THEME_PRIMARY;
    local->indicator_color = EGUI_THEME_PRIMARY;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->icon_font = NULL;
    local->icon_text_gap = 2;

    egui_view_set_view_name(self, "egui_view_tab_bar");
}

void egui_view_tab_bar_apply_params(egui_view_t *self, const egui_view_tab_bar_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_tab_bar_t);

    self->region = params->region;

    local->tab_texts = params->tab_texts;
    local->tab_count = params->tab_count;

    egui_view_invalidate(self);
}

void egui_view_tab_bar_init_with_params(egui_view_t *self, const egui_view_tab_bar_params_t *params)
{
    egui_view_tab_bar_init(self);
    egui_view_tab_bar_apply_params(self, params);
}
