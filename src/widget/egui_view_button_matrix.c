#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_button_matrix.h"
#include "font/egui_font.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

static const egui_font_t *egui_view_button_matrix_get_icon_font(egui_view_button_matrix_t *local, egui_dim_t area_size)
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

static void egui_view_button_matrix_draw_text_clipped(const egui_font_t *font, const char *text, const egui_region_t *text_rect, const egui_region_t *clip_rect,
                                                      uint8_t align, egui_color_t color, egui_alpha_t alpha)
{
    egui_region_t draw_rect = *text_rect;
    egui_region_t text_clip = *clip_rect;
    const egui_region_t *prev_clip = egui_canvas_get_extra_clip();

    if (prev_clip != NULL)
    {
        egui_region_intersect(&text_clip, prev_clip, &text_clip);
    }

    if (egui_region_is_empty(&text_clip))
    {
        return;
    }

    egui_canvas_set_extra_clip(&text_clip);
    egui_canvas_draw_text_in_rect(font, text, &draw_rect, align, color, alpha);
    if (prev_clip != NULL)
    {
        egui_canvas_set_extra_clip(prev_clip);
    }
    else
    {
        egui_canvas_clear_extra_clip();
    }
}

void egui_view_button_matrix_set_labels(egui_view_t *self, const char **labels, uint8_t count, uint8_t cols)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    local->labels = labels;
    if (count > EGUI_VIEW_BUTTON_MATRIX_MAX_BUTTONS)
    {
        count = EGUI_VIEW_BUTTON_MATRIX_MAX_BUTTONS;
    }
    local->btn_count = count;
    local->cols = cols;
    if (local->selected_index >= local->btn_count)
    {
        local->selected_index = EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE;
    }
    egui_view_invalidate(self);
}

void egui_view_button_matrix_set_on_click(egui_view_t *self, egui_view_button_matrix_click_cb_t callback)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    local->on_click = callback;
}

void egui_view_button_matrix_set_selection_enabled(egui_view_t *self, uint8_t enabled)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    enabled = enabled ? 1 : 0;
    if (local->selection_enabled == enabled)
    {
        return;
    }
    local->selection_enabled = enabled;
    if (!enabled)
    {
        local->selected_index = EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE;
    }
    egui_view_invalidate(self);
}

void egui_view_button_matrix_set_selected_index(egui_view_t *self, uint8_t index)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (index == EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE || local->btn_count == 0)
    {
        index = EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE;
    }
    else if (index >= local->btn_count)
    {
        index = (uint8_t)(local->btn_count - 1U);
    }
    if (local->selected_index == index)
    {
        return;
    }
    local->selected_index = index;
    egui_view_invalidate(self);
}

uint8_t egui_view_button_matrix_get_selected_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    return local->selected_index;
}

void egui_view_button_matrix_set_btn_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    local->btn_color = color;
    egui_view_invalidate(self);
}

void egui_view_button_matrix_set_btn_pressed_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    local->btn_pressed_color = color;
    egui_view_invalidate(self);
}

void egui_view_button_matrix_set_text_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    local->text_color = color;
    egui_view_invalidate(self);
}

void egui_view_button_matrix_set_gap(egui_view_t *self, uint8_t gap)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    local->gap = gap;
    egui_view_invalidate(self);
}

void egui_view_button_matrix_set_corner_radius(egui_view_t *self, uint8_t radius)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    local->corner_radius = radius;
    egui_view_invalidate(self);
}

void egui_view_button_matrix_set_border_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    local->border_color = color;
    egui_view_invalidate(self);
}

void egui_view_button_matrix_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    local->font = font;
    egui_view_invalidate(self);
}

void egui_view_button_matrix_set_icons(egui_view_t *self, const char **icons)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->icons == icons)
    {
        return;
    }

    local->icons = icons;
    egui_view_invalidate(self);
}

void egui_view_button_matrix_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->icon_font == font)
    {
        return;
    }

    local->icon_font = font;
    egui_view_invalidate(self);
}

void egui_view_button_matrix_set_icon_text_gap(egui_view_t *self, egui_dim_t gap)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->icon_text_gap == gap)
    {
        return;
    }

    local->icon_text_gap = gap;
    egui_view_invalidate(self);
}

void egui_view_button_matrix_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);

    if ((local->labels == NULL && local->icons == NULL) || local->btn_count == 0 || local->cols == 0)
    {
        return;
    }

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t x = region.location.x;
    egui_dim_t y = region.location.y;
    egui_dim_t w = region.size.width;
    egui_dim_t h = region.size.height;

    uint8_t cols = local->cols;
    uint8_t rows = (local->btn_count + cols - 1) / cols;
    uint8_t gap = local->gap;

    egui_dim_t btn_w = (w - gap * (cols - 1)) / cols;
    egui_dim_t btn_h = (h - gap * (rows - 1)) / rows;

    const egui_font_t *font = local->font ? local->font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;

    uint8_t i;
    for (i = 0; i < local->btn_count; i++)
    {
        uint8_t col = i % cols;
        uint8_t row = i / cols;
        const char *label = (local->labels != NULL) ? local->labels[i] : NULL;
        const char *icon = (local->icons != NULL) ? local->icons[i] : NULL;

        egui_dim_t btn_x = x + col * (btn_w + gap);
        egui_dim_t btn_y = y + row * (btn_h + gap);

        egui_color_t bg_color = local->btn_color;
        if (i == local->pressed_index)
        {
            bg_color = local->btn_pressed_color;
        }
        else if (local->selection_enabled && i == local->selected_index)
        {
            bg_color = local->btn_pressed_color;
        }

        egui_dim_t r = local->corner_radius;
        if (r > btn_h / 2)
        {
            r = btn_h / 2;
        }
        if (r > btn_w / 2)
        {
            r = btn_w / 2;
        }

        // Draw rounded rectangle background
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
                    .alpha = EGUI_ALPHA_100,
                    .stops = stops,
            };
            egui_canvas_draw_round_rectangle_fill_gradient(btn_x, btn_y, btn_w, btn_h, r, &grad);
        }
#else
        egui_canvas_draw_round_rectangle_fill(btn_x, btn_y, btn_w, btn_h, r, bg_color, EGUI_ALPHA_100);
#endif

        // Draw border, emphasize selected/pressed state for better affordance.
        egui_dim_t border_w = 1;
        egui_color_t border_color = local->border_color;
        if (i == local->pressed_index || (local->selection_enabled && i == local->selected_index))
        {
            border_w = 2;
            border_color = EGUI_COLOR_WHITE;
        }
        egui_canvas_draw_round_rectangle(btn_x, btn_y, btn_w, btn_h, r, border_w, border_color, EGUI_ALPHA_100);

        // Draw centered content with per-cell clip to avoid overflow into neighboring cells.
        egui_region_t content_rect = {{btn_x + 4, btn_y + 4}, {btn_w > 8 ? (btn_w - 8) : btn_w, btn_h > 8 ? (btn_h - 8) : btn_h}};
        if (icon != NULL)
        {
            if (label != NULL && label[0] != '\0')
            {
                egui_dim_t text_w = 0;
                egui_dim_t text_h = 0;
                egui_dim_t icon_area_h;
                egui_dim_t content_h;
                egui_dim_t content_y;
                egui_dim_t text_gap = local->icon_text_gap;
                egui_region_t icon_rect;
                egui_region_t text_rect;

                if (font->api != NULL && font->api->get_str_size != NULL)
                {
                    font->api->get_str_size(font, label, 0, 0, &text_w, &text_h);
                }
                if (text_h < 0)
                {
                    text_h = 0;
                }
                if (text_gap < 0)
                {
                    text_gap = 0;
                }
                if (text_w <= 0)
                {
                    text_gap = 0;
                }

                icon_area_h = content_rect.size.height - text_h - text_gap;
                if (icon_area_h < content_rect.size.height / 2)
                {
                    icon_area_h = content_rect.size.height / 2;
                }
                if (icon_area_h > content_rect.size.height)
                {
                    icon_area_h = content_rect.size.height;
                }

                content_h = icon_area_h + ((text_h > 0) ? (text_h + text_gap) : 0);
                if (content_h > content_rect.size.height)
                {
                    content_h = content_rect.size.height;
                }
                content_y = content_rect.location.y + (content_rect.size.height - content_h) / 2;

                icon_rect.location.x = content_rect.location.x;
                icon_rect.location.y = content_y;
                icon_rect.size.width = content_rect.size.width;
                icon_rect.size.height = icon_area_h;

                text_rect.location.x = content_rect.location.x;
                text_rect.location.y = content_y + icon_area_h;
                text_rect.size.width = content_rect.size.width;
                text_rect.size.height = content_rect.location.y + content_rect.size.height - text_rect.location.y;

                egui_view_button_matrix_draw_text_clipped(egui_view_button_matrix_get_icon_font(local, EGUI_MIN(icon_rect.size.width, icon_rect.size.height)),
                                                          icon, &icon_rect, &content_rect, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
                egui_view_button_matrix_draw_text_clipped(font, label, &text_rect, &content_rect, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
            }
            else
            {
                egui_view_button_matrix_draw_text_clipped(
                        egui_view_button_matrix_get_icon_font(local, EGUI_MIN(content_rect.size.width, content_rect.size.height)), icon, &content_rect,
                        &content_rect, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
            }
        }
        else if (label != NULL)
        {
            egui_view_button_matrix_draw_text_clipped(font, label, &content_rect, &content_rect, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
        }
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_button_matrix_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);

    if (self->is_enable == false)
    {
        return 0;
    }

    if ((local->labels == NULL && local->icons == NULL) || local->btn_count == 0 || local->cols == 0)
    {
        return 0;
    }

    egui_dim_t w = self->region.size.width;
    egui_dim_t h = self->region.size.height;
    uint8_t cols = local->cols;
    uint8_t rows = (local->btn_count + cols - 1) / cols;
    uint8_t gap = local->gap;

    egui_dim_t btn_w = (w - gap * (cols - 1)) / cols;
    egui_dim_t btn_h = (h - gap * (rows - 1)) / rows;

    egui_dim_t touch_x = event->location.x - self->region_screen.location.x;
    egui_dim_t touch_y = event->location.y - self->region_screen.location.y;

    // Calculate which button was touched
    uint8_t hit_index = EGUI_VIEW_BUTTON_MATRIX_PRESSED_NONE;
    if (btn_w > 0 && btn_h > 0)
    {
        // Account for gap: check if touch is within a button or in a gap
        uint8_t col;
        uint8_t row;
        egui_dim_t cell_w = btn_w + gap;
        egui_dim_t cell_h = btn_h + gap;

        if (cell_w > 0 && cell_h > 0)
        {
            col = (uint8_t)(touch_x / cell_w);
            row = (uint8_t)(touch_y / cell_h);

            // Check if within button area (not in gap)
            egui_dim_t local_x = touch_x - col * cell_w;
            egui_dim_t local_y = touch_y - row * cell_h;

            if (col < cols && row < rows && local_x < btn_w && local_y < btn_h)
            {
                uint8_t idx = row * cols + col;
                if (idx < local->btn_count)
                {
                    hit_index = idx;
                }
            }
        }
    }

    if (event->type == EGUI_MOTION_EVENT_ACTION_DOWN)
    {
        local->pressed_index = hit_index;
        egui_view_invalidate(self);
    }
    else if (event->type == EGUI_MOTION_EVENT_ACTION_UP)
    {
        if (hit_index == local->pressed_index && hit_index != EGUI_VIEW_BUTTON_MATRIX_PRESSED_NONE)
        {
            if (local->selection_enabled)
            {
                local->selected_index = hit_index;
            }
            if (local->on_click)
            {
                local->on_click(self, hit_index);
            }
        }
        local->pressed_index = EGUI_VIEW_BUTTON_MATRIX_PRESSED_NONE;
        egui_view_invalidate(self);
    }
    else if (event->type == EGUI_MOTION_EVENT_ACTION_CANCEL)
    {
        local->pressed_index = EGUI_VIEW_BUTTON_MATRIX_PRESSED_NONE;
        egui_view_invalidate(self);
    }

    return 1;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_button_matrix_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_button_matrix_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_button_matrix_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_button_matrix_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_button_matrix_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_button_matrix_t);

    // init local data.
    local->labels = NULL;
    local->icons = NULL;
    local->btn_count = 0;
    local->cols = 4;
    local->gap = 2;
    local->pressed_index = EGUI_VIEW_BUTTON_MATRIX_PRESSED_NONE;
    local->selected_index = EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE;
    local->selection_enabled = 0;
    local->btn_color = EGUI_COLOR_DARK_GREY;
    local->btn_pressed_color = EGUI_COLOR_LIGHT_GREY;
    local->text_color = EGUI_COLOR_WHITE;
    local->border_color = EGUI_COLOR_WHITE;
    local->corner_radius = 4;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->icon_font = NULL;
    local->icon_text_gap = 2;
    local->on_click = NULL;

    egui_view_set_view_name(self, "egui_view_button_matrix");
}

void egui_view_button_matrix_apply_params(egui_view_t *self, const egui_view_button_matrix_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);

    self->region = params->region;
    local->cols = params->cols;
    local->gap = params->gap;

    egui_view_invalidate(self);
}

void egui_view_button_matrix_init_with_params(egui_view_t *self, const egui_view_button_matrix_params_t *params)
{
    egui_view_button_matrix_init(self);
    egui_view_button_matrix_apply_params(self, params);
}
