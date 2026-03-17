#include <stdlib.h>

#include "egui_view_message_bar.h"

static uint8_t egui_view_message_bar_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_MESSAGE_BAR_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_MESSAGE_BAR_MAX_SNAPSHOTS;
    }
    return count;
}

static egui_color_t egui_view_message_bar_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 65);
}

static egui_color_t egui_view_message_bar_severity_color(egui_view_message_bar_t *local, uint8_t severity)
{
    switch (severity)
    {
    case 1:
        return local->success_color;
    case 2:
        return local->warning_color;
    case 3:
        return local->error_color;
    default:
        return local->info_color;
    }
}

static const char *egui_view_message_bar_severity_glyph(uint8_t severity)
{
    switch (severity)
    {
    case 1:
        return "+";
    case 2:
        return "!";
    case 3:
        return "x";
    default:
        return "i";
    }
}

static uint8_t egui_view_message_bar_text_len(const char *text)
{
    uint8_t length = 0;

    if (text == NULL)
    {
        return 0;
    }
    while (text[length] != '\0')
    {
        length++;
    }
    return length;
}

void egui_view_message_bar_set_snapshots(egui_view_t *self, const egui_view_message_bar_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_message_bar_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_message_bar_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_message_bar_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_message_bar_t);
    if (local->snapshot_count == 0 || snapshot_index >= local->snapshot_count)
    {
        return;
    }
    if (local->current_snapshot == snapshot_index)
    {
        return;
    }
    local->current_snapshot = snapshot_index;
    egui_view_invalidate(self);
}

uint8_t egui_view_message_bar_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_message_bar_t);
    return local->current_snapshot;
}

void egui_view_message_bar_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_message_bar_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_message_bar_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_message_bar_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_message_bar_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_message_bar_t);
    local->locked_mode = locked_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_message_bar_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                       egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t info_color, egui_color_t success_color,
                                       egui_color_t warning_color, egui_color_t error_color)
{
    EGUI_LOCAL_INIT(egui_view_message_bar_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->info_color = info_color;
    local->success_color = success_color;
    local->warning_color = warning_color;
    local->error_color = error_color;
    egui_view_invalidate(self);
}

static void egui_view_message_bar_draw_text(egui_view_message_bar_t *local, egui_view_t *self, const char *text, egui_dim_t x, egui_dim_t y, egui_dim_t width,
                                            egui_dim_t height, uint8_t align, egui_color_t color)
{
    egui_region_t text_region;

    text_region.location.x = x;
    text_region.location.y = y;
    text_region.size.width = width;
    text_region.size.height = height;
    egui_canvas_draw_text_in_rect(local->font, text, &text_region, align, color, self->alpha);
}

static void egui_view_message_bar_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_message_bar_t);
    egui_region_t region;
    const egui_view_message_bar_snapshot_t *snapshot;
    egui_color_t severity_color;
    egui_color_t fill_color;
    egui_color_t border_color;
    egui_color_t title_color;
    egui_color_t body_color;
    uint8_t is_enabled;
    egui_dim_t radius;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_w;
    egui_dim_t content_h;
    egui_dim_t accent_w;
    egui_dim_t icon_size;
    egui_dim_t icon_x;
    egui_dim_t icon_y;
    egui_dim_t close_w;
    egui_dim_t close_x;
    egui_dim_t title_x;
    egui_dim_t title_w;
    egui_dim_t title_h;
    egui_dim_t body_y;
    egui_dim_t body_h;
    egui_dim_t action_h;
    egui_dim_t action_w;
    egui_dim_t action_x;
    egui_dim_t action_y;
    uint8_t show_close;
    uint8_t show_action;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    is_enabled = egui_view_get_enable(self) ? 1 : 0;
    severity_color = egui_view_message_bar_severity_color(local, snapshot->severity);
    fill_color = egui_rgb_mix(local->surface_color, severity_color, local->compact_mode ? 3 : 5);
    border_color = egui_rgb_mix(local->border_color, severity_color, local->compact_mode ? 5 : 8);
    title_color = local->text_color;
    body_color = local->muted_text_color;
    if (!is_enabled)
    {
        severity_color = egui_view_message_bar_mix_disabled(severity_color);
        fill_color = egui_view_message_bar_mix_disabled(fill_color);
        border_color = egui_view_message_bar_mix_disabled(border_color);
        title_color = egui_view_message_bar_mix_disabled(title_color);
        body_color = egui_view_message_bar_mix_disabled(body_color);
    }
    else if (local->locked_mode)
    {
        severity_color = egui_rgb_mix(severity_color, local->muted_text_color, 22);
        fill_color = egui_rgb_mix(fill_color, local->surface_color, 10);
        border_color = egui_rgb_mix(border_color, local->muted_text_color, 12);
        body_color = egui_rgb_mix(body_color, local->text_color, 14);
    }

    radius = local->compact_mode ? 6 : 8;
    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, radius, fill_color,
                                          egui_color_alpha_mix(self->alpha, EGUI_ALPHA_90));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, radius, 1, border_color,
                                     egui_color_alpha_mix(self->alpha, EGUI_ALPHA_100));

    accent_w = local->compact_mode ? 3 : 4;
    egui_canvas_draw_round_rectangle_fill(region.location.x + 1, region.location.y + 1, accent_w, region.size.height - 2, radius - 2, severity_color,
                                          egui_color_alpha_mix(self->alpha, EGUI_ALPHA_90));

    content_x = region.location.x + (local->compact_mode ? 8 : 11);
    content_y = region.location.y + (local->compact_mode ? 7 : 10);
    content_w = region.size.width - (local->compact_mode ? 15 : 21);
    content_h = region.size.height - (local->compact_mode ? 14 : 20);

    icon_size = local->compact_mode ? 11 : 14;
    icon_x = content_x;
    icon_y = content_y;
    egui_canvas_draw_circle_fill(icon_x + icon_size / 2, icon_y + icon_size / 2, icon_size / 2, severity_color,
                                 egui_color_alpha_mix(self->alpha, local->locked_mode ? EGUI_ALPHA_40 : EGUI_ALPHA_90));
    egui_view_message_bar_draw_text(local, self, egui_view_message_bar_severity_glyph(snapshot->severity), icon_x, icon_y - 1, icon_size, icon_size + 2,
                                    EGUI_ALIGN_CENTER, local->surface_color);

    show_close = snapshot->closable && !local->compact_mode && !local->locked_mode && is_enabled;
    close_w = show_close ? 10 : 0;
    close_x = content_x + content_w - close_w;
    if (show_close)
    {
        egui_canvas_draw_line(close_x + 3, content_y + 3, close_x + 7, content_y + 7, 1, body_color, egui_color_alpha_mix(self->alpha, 78));
        egui_canvas_draw_line(close_x + 7, content_y + 3, close_x + 3, content_y + 7, 1, body_color, egui_color_alpha_mix(self->alpha, 78));
    }

    title_x = icon_x + icon_size + 6;
    title_w = content_x + content_w - title_x - (show_close ? 12 : 0);
    title_h = local->compact_mode ? 10 : 12;
    egui_view_message_bar_draw_text(local, self, snapshot->title, title_x, content_y - 1, title_w, title_h, EGUI_ALIGN_LEFT, title_color);

    show_action = snapshot->show_action && snapshot->action != NULL && !local->locked_mode;
    action_h = local->compact_mode ? 12 : 14;
    action_w = local->compact_mode ? 34 : 52;
    if (snapshot->action != NULL)
    {
        egui_dim_t text_action_w = 14 + egui_view_message_bar_text_len(snapshot->action) * 4;
        if (text_action_w > action_w)
        {
            action_w = text_action_w;
        }
    }
    if (action_w > content_w - 12)
    {
        action_w = content_w - 12;
    }
    action_x = title_x;
    body_y = content_y + title_h + (local->compact_mode ? 3 : 5);
    if (show_action)
    {
        body_h = local->compact_mode ? 12 : 15;
        action_y = body_y + body_h + (local->compact_mode ? 3 : 7);
        if (action_y + action_h > content_y + content_h - (local->compact_mode ? 0 : 2))
        {
            action_y = content_y + content_h - action_h - (local->compact_mode ? 0 : 2);
        }
    }
    else
    {
        action_h = 0;
        action_y = 0;
        body_h = content_y + content_h - body_y;
    }
    if (body_h < (local->compact_mode ? 10 : 12))
    {
        body_h = local->compact_mode ? 10 : 12;
    }

    egui_view_message_bar_draw_text(local, self, snapshot->body, title_x, body_y, title_w, body_h, EGUI_ALIGN_LEFT, body_color);

    if (show_action)
    {
        egui_color_t action_fill = egui_rgb_mix(local->surface_color, local->accent_color, 6);
        egui_color_t action_border = egui_rgb_mix(local->border_color, local->accent_color, 14);

        egui_canvas_draw_round_rectangle_fill(action_x, action_y, action_w, action_h, 5, action_fill, egui_color_alpha_mix(self->alpha, 48));
        egui_canvas_draw_round_rectangle(action_x, action_y, action_w, action_h, 5, 1, action_border, egui_color_alpha_mix(self->alpha, 52));
        egui_view_message_bar_draw_text(local, self, snapshot->action, action_x + 2, action_y, action_w - 4, action_h, EGUI_ALIGN_CENTER, local->accent_color);
    }

    if (local->locked_mode)
    {
        egui_dim_t pin_w = local->compact_mode ? 18 : 26;
        egui_dim_t pin_h = local->compact_mode ? 10 : 11;
        egui_dim_t pin_x = content_x + content_w - pin_w;
        egui_dim_t pin_y = content_y + content_h - pin_h - (local->compact_mode ? 2 : 1);
        egui_color_t pin_fill = egui_rgb_mix(local->surface_color, severity_color, 4);
        egui_color_t pin_border = egui_rgb_mix(local->border_color, severity_color, 8);

        egui_canvas_draw_round_rectangle_fill(pin_x, pin_y, pin_w, pin_h, 5, pin_fill, egui_color_alpha_mix(self->alpha, 56));
        egui_canvas_draw_round_rectangle(pin_x, pin_y, pin_w, pin_h, 5, 1, pin_border, egui_color_alpha_mix(self->alpha, 62));
        egui_view_message_bar_draw_text(local, self, "Pin", pin_x + 1, pin_y, pin_w - 2, pin_h, EGUI_ALIGN_CENTER, body_color);
    }

    if (local->locked_mode || !is_enabled)
    {
        egui_canvas_draw_line(content_x + 1, content_y + content_h - 1, content_x + content_w - 1, content_y + content_h - 1, 1, border_color,
                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40));
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_message_bar_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_message_bar_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_message_bar_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_message_bar_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_message_bar_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD7DEE7);
    local->text_color = EGUI_COLOR_HEX(0x17212B);
    local->muted_text_color = EGUI_COLOR_HEX(0x556272);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->info_color = EGUI_COLOR_HEX(0x2563EB);
    local->success_color = EGUI_COLOR_HEX(0x0F9D58);
    local->warning_color = EGUI_COLOR_HEX(0xC27C12);
    local->error_color = EGUI_COLOR_HEX(0xC93C37);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->compact_mode = 0;
    local->locked_mode = 0;
}
