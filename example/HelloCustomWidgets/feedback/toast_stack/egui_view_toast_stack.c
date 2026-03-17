#include <stdlib.h>

#include "egui_view_toast_stack.h"

#define TOAST_STACK_STANDARD_SHIFT_X 9
#define TOAST_STACK_STANDARD_SHIFT_Y 9
#define TOAST_STACK_COMPACT_SHIFT_X  4
#define TOAST_STACK_COMPACT_SHIFT_Y  4

static uint8_t egui_view_toast_stack_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_TOAST_STACK_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_TOAST_STACK_MAX_SNAPSHOTS;
    }
    return count;
}

static egui_color_t egui_view_toast_stack_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 68);
}

static egui_color_t egui_view_toast_stack_severity_color(egui_view_toast_stack_t *local, uint8_t severity)
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

static const char *egui_view_toast_stack_severity_glyph(uint8_t severity)
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

static uint8_t egui_view_toast_stack_text_len(const char *text)
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

void egui_view_toast_stack_set_snapshots(egui_view_t *self, const egui_view_toast_stack_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_toast_stack_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_toast_stack_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_toast_stack_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_toast_stack_t);
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

uint8_t egui_view_toast_stack_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_toast_stack_t);
    return local->current_snapshot;
}

void egui_view_toast_stack_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_toast_stack_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_toast_stack_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_toast_stack_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_toast_stack_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_toast_stack_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_toast_stack_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_toast_stack_t);
    local->locked_mode = locked_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_toast_stack_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                       egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t info_color, egui_color_t success_color,
                                       egui_color_t warning_color, egui_color_t error_color)
{
    EGUI_LOCAL_INIT(egui_view_toast_stack_t);
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

static void egui_view_toast_stack_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                            egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (text == NULL || text[0] == '\0')
    {
        return;
    }
    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void egui_view_toast_stack_draw_pill(const egui_font_t *font, egui_view_t *self, const char *text, egui_dim_t x, egui_dim_t y, egui_dim_t width,
                                            egui_dim_t height, egui_dim_t radius, egui_color_t fill_color, egui_alpha_t fill_alpha, egui_color_t border_color,
                                            egui_alpha_t border_alpha, egui_color_t text_color)
{
    egui_region_t text_region;

    if (width <= 0 || height <= 0 || text == NULL || text[0] == '\0')
    {
        return;
    }

    egui_canvas_draw_round_rectangle_fill(x, y, width, height, radius, fill_color, egui_color_alpha_mix(self->alpha, fill_alpha));
    egui_canvas_draw_round_rectangle(x, y, width, height, radius, 1, border_color, egui_color_alpha_mix(self->alpha, border_alpha));

    text_region.location.x = x;
    text_region.location.y = y;
    text_region.size.width = width;
    text_region.size.height = height;
    egui_canvas_draw_text_in_rect(font, text, &text_region, EGUI_ALIGN_CENTER, text_color, self->alpha);
}

static void egui_view_toast_stack_draw_back_card(egui_view_t *self, const egui_font_t *font, const egui_region_t *card_region, const char *title,
                                                 egui_color_t fill_color, egui_color_t border_color, egui_color_t strip_color, egui_color_t text_color,
                                                 uint8_t compact_mode)
{
    egui_region_t title_region;
    egui_dim_t radius = compact_mode ? 5 : 6;
    egui_dim_t strip_w = compact_mode ? 3 : 4;
    egui_dim_t footer_w;

    if (card_region->size.width <= 12 || card_region->size.height <= 12)
    {
        return;
    }

    egui_canvas_draw_round_rectangle_fill(card_region->location.x, card_region->location.y, card_region->size.width, card_region->size.height, radius,
                                          fill_color, egui_color_alpha_mix(self->alpha, compact_mode ? 92 : 96));
    egui_canvas_draw_round_rectangle(card_region->location.x, card_region->location.y, card_region->size.width, card_region->size.height, radius, 1,
                                     border_color, egui_color_alpha_mix(self->alpha, compact_mode ? 58 : 64));
    egui_canvas_draw_round_rectangle_fill(card_region->location.x + 1, card_region->location.y + 1, strip_w, card_region->size.height - 2, radius - 2,
                                          strip_color, egui_color_alpha_mix(self->alpha, compact_mode ? 52 : 58));

    title_region.location.x = card_region->location.x + strip_w + (compact_mode ? 5 : 7);
    title_region.location.y = card_region->location.y;
    title_region.size.width = card_region->size.width - strip_w - (compact_mode ? 10 : 14);
    title_region.size.height = card_region->size.height;
    egui_view_toast_stack_draw_text(font, self, title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color);

    footer_w = card_region->size.width / (compact_mode ? 2 : 3);
    if (footer_w < 16)
    {
        footer_w = 16;
    }
    egui_canvas_draw_round_rectangle_fill(card_region->location.x + strip_w + (compact_mode ? 5 : 7),
                                          card_region->location.y + card_region->size.height - (compact_mode ? 8 : 10), footer_w, 2, 1, border_color,
                                          egui_color_alpha_mix(self->alpha, compact_mode ? 18 : 22));
}

static void egui_view_toast_stack_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_toast_stack_t);
    egui_region_t region;
    egui_region_t front_region;
    egui_region_t mid_region;
    egui_region_t back_region;
    egui_region_t text_region;
    const egui_view_toast_stack_snapshot_t *snapshot;
    egui_color_t severity_color;
    egui_color_t front_fill;
    egui_color_t front_border;
    egui_color_t back_fill;
    egui_color_t back_border;
    egui_color_t body_color;
    egui_color_t meta_color;
    egui_color_t shadow_color;
    egui_color_t action_fill;
    egui_color_t action_border;
    egui_color_t action_text;
    uint8_t is_enabled;
    uint8_t show_action;
    uint8_t show_close;
    egui_dim_t shift_x;
    egui_dim_t shift_y;
    egui_dim_t radius;
    egui_dim_t strip_w;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_w;
    egui_dim_t content_h;
    egui_dim_t icon_size;
    egui_dim_t close_w;
    egui_dim_t title_x;
    egui_dim_t title_w;
    egui_dim_t title_h;
    egui_dim_t body_y;
    egui_dim_t body_h;
    egui_dim_t footer_y;
    egui_dim_t action_h;
    egui_dim_t action_w;
    egui_dim_t meta_h;
    egui_dim_t meta_w;
    egui_dim_t footer_gap;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    shift_x = local->compact_mode ? TOAST_STACK_COMPACT_SHIFT_X : TOAST_STACK_STANDARD_SHIFT_X;
    shift_y = local->compact_mode ? TOAST_STACK_COMPACT_SHIFT_Y : TOAST_STACK_STANDARD_SHIFT_Y;
    radius = local->compact_mode ? 6 : 8;
    strip_w = local->compact_mode ? 3 : 4;

    front_region.location.x = region.location.x;
    front_region.location.y = region.location.y;
    front_region.size.width = region.size.width - shift_x * 2;
    front_region.size.height = region.size.height - shift_y * 2;
    mid_region.location.x = region.location.x + shift_x;
    mid_region.location.y = region.location.y + shift_y;
    mid_region.size.width = front_region.size.width;
    mid_region.size.height = front_region.size.height;
    back_region.location.x = region.location.x + shift_x * 2;
    back_region.location.y = region.location.y + shift_y * 2;
    back_region.size.width = front_region.size.width;
    back_region.size.height = front_region.size.height;

    if (front_region.size.width <= 32 || front_region.size.height <= 30)
    {
        return;
    }

    is_enabled = egui_view_get_enable(self) ? 1 : 0;
    severity_color = egui_view_toast_stack_severity_color(local, snapshot->severity);
    front_fill = egui_rgb_mix(local->surface_color, severity_color, local->compact_mode ? 4 : 6);
    front_border = egui_rgb_mix(local->border_color, severity_color, local->compact_mode ? 8 : 12);
    back_fill = egui_rgb_mix(local->surface_color, severity_color, local->compact_mode ? 1 : 2);
    back_border = egui_rgb_mix(local->border_color, severity_color, local->compact_mode ? 4 : 6);
    body_color = local->muted_text_color;
    meta_color = egui_rgb_mix(local->muted_text_color, local->text_color, local->compact_mode ? 30 : 26);
    shadow_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->border_color, 10);
    action_fill = egui_rgb_mix(local->surface_color, local->accent_color, 10);
    action_border = egui_rgb_mix(local->border_color, local->accent_color, 18);
    action_text = local->accent_color;

    if (local->locked_mode)
    {
        severity_color = egui_rgb_mix(severity_color, local->muted_text_color, 72);
        front_fill = egui_rgb_mix(front_fill, EGUI_COLOR_HEX(0xFBFCFD), 18);
        front_border = egui_rgb_mix(front_border, local->muted_text_color, 16);
        back_fill = egui_rgb_mix(back_fill, EGUI_COLOR_HEX(0xFBFCFD), 14);
        back_border = egui_rgb_mix(back_border, local->muted_text_color, 12);
        body_color = egui_rgb_mix(body_color, local->text_color, 24);
        meta_color = egui_rgb_mix(meta_color, local->muted_text_color, 22);
        action_fill = egui_rgb_mix(action_fill, local->surface_color, 32);
        action_border = egui_rgb_mix(action_border, local->muted_text_color, 26);
        action_text = egui_rgb_mix(action_text, local->muted_text_color, 78);
    }

    if (!is_enabled)
    {
        severity_color = egui_view_toast_stack_mix_disabled(severity_color);
        front_fill = egui_view_toast_stack_mix_disabled(front_fill);
        front_border = egui_view_toast_stack_mix_disabled(front_border);
        back_fill = egui_view_toast_stack_mix_disabled(back_fill);
        back_border = egui_view_toast_stack_mix_disabled(back_border);
        body_color = egui_view_toast_stack_mix_disabled(body_color);
        meta_color = egui_view_toast_stack_mix_disabled(meta_color);
        shadow_color = egui_view_toast_stack_mix_disabled(shadow_color);
        action_fill = egui_view_toast_stack_mix_disabled(action_fill);
        action_border = egui_view_toast_stack_mix_disabled(action_border);
        action_text = egui_view_toast_stack_mix_disabled(action_text);
    }

    egui_view_toast_stack_draw_back_card(self, local->meta_font, &back_region, snapshot->back_title ? snapshot->back_title : snapshot->title, back_fill,
                                         back_border, severity_color, meta_color, local->compact_mode);
    egui_view_toast_stack_draw_back_card(self, local->meta_font, &mid_region, snapshot->mid_title ? snapshot->mid_title : snapshot->title,
                                         egui_rgb_mix(back_fill, local->surface_color, 10), egui_rgb_mix(back_border, local->border_color, 8), severity_color,
                                         meta_color, local->compact_mode);

    egui_canvas_draw_round_rectangle_fill(front_region.location.x + 2, front_region.location.y + 3, front_region.size.width, front_region.size.height, radius,
                                          shadow_color, egui_color_alpha_mix(self->alpha, local->compact_mode ? 8 : 10));
    egui_canvas_draw_round_rectangle_fill(front_region.location.x, front_region.location.y, front_region.size.width, front_region.size.height, radius,
                                          front_fill, egui_color_alpha_mix(self->alpha, local->compact_mode ? 96 : 98));
    egui_canvas_draw_round_rectangle(front_region.location.x, front_region.location.y, front_region.size.width, front_region.size.height, radius, 1,
                                     front_border, egui_color_alpha_mix(self->alpha, local->compact_mode ? 60 : 68));
    egui_canvas_draw_round_rectangle_fill(front_region.location.x + 1, front_region.location.y + 1, strip_w, front_region.size.height - 2, radius - 2,
                                          severity_color, egui_color_alpha_mix(self->alpha, local->locked_mode ? 38 : 88));

    content_x = front_region.location.x + (local->compact_mode ? 8 : 10);
    content_y = front_region.location.y + (local->compact_mode ? 6 : 8);
    content_w = front_region.size.width - (local->compact_mode ? 14 : 18);
    content_h = front_region.size.height - (local->compact_mode ? 12 : 16);
    icon_size = local->compact_mode ? 10 : 12;
    close_w = (!local->compact_mode && snapshot->closable && !local->locked_mode && is_enabled) ? 10 : 0;
    show_close = close_w > 0 ? 1 : 0;
    title_x = content_x + icon_size + 6;
    title_w = content_w - icon_size - 6 - (show_close ? 12 : 0);
    title_h = local->compact_mode ? 11 : 12;

    egui_canvas_draw_circle_fill(content_x + icon_size / 2, content_y + icon_size / 2, icon_size / 2, severity_color,
                                 egui_color_alpha_mix(self->alpha, local->locked_mode ? 34 : 92));
    text_region.location.x = content_x;
    text_region.location.y = content_y - 1;
    text_region.size.width = icon_size;
    text_region.size.height = icon_size + 2;
    egui_view_toast_stack_draw_text(local->meta_font, self, egui_view_toast_stack_severity_glyph(snapshot->severity), &text_region, EGUI_ALIGN_CENTER,
                                    local->surface_color);

    text_region.location.x = title_x;
    text_region.location.y = content_y - 1;
    text_region.size.width = title_w;
    text_region.size.height = title_h;
    egui_view_toast_stack_draw_text(local->font, self, snapshot->title, &text_region, EGUI_ALIGN_LEFT, local->text_color);

    if (show_close)
    {
        egui_dim_t close_x = content_x + content_w - 10;

        egui_canvas_draw_line(close_x, content_y + 2, close_x + 4, content_y + 6, 1, body_color, egui_color_alpha_mix(self->alpha, 74));
        egui_canvas_draw_line(close_x + 4, content_y + 2, close_x, content_y + 6, 1, body_color, egui_color_alpha_mix(self->alpha, 74));
    }

    body_y = content_y + title_h + (local->compact_mode ? 3 : 5);
    body_h = local->compact_mode ? 16 : 18;
    if (body_y + body_h > content_y + content_h - (local->compact_mode ? 12 : 16))
    {
        body_h = content_y + content_h - body_y - (local->compact_mode ? 12 : 16);
    }
    if (body_h < 10)
    {
        body_h = 10;
    }

    text_region.location.x = title_x;
    text_region.location.y = body_y;
    text_region.size.width = content_w - icon_size - 6;
    text_region.size.height = body_h;
    egui_view_toast_stack_draw_text(local->font, self, snapshot->body, &text_region, EGUI_ALIGN_LEFT, body_color);

    footer_gap = local->compact_mode ? 2 : 4;
    action_h = local->compact_mode ? 11 : 13;
    meta_h = local->compact_mode ? 10 : 12;
    footer_y = content_y + content_h - action_h;
    show_action = (snapshot->action != NULL && snapshot->action[0] != '\0' && !local->locked_mode) ? 1 : 0;
    action_w = 18 + egui_view_toast_stack_text_len(snapshot->action) * (local->compact_mode ? 4 : 5);
    meta_w = 12 + egui_view_toast_stack_text_len(snapshot->meta) * (local->compact_mode ? 4 : 5);

    if (show_action)
    {
        if (action_w < (local->compact_mode ? 26 : 38))
        {
            action_w = local->compact_mode ? 26 : 38;
        }
        if (action_w > content_w / 2)
        {
            action_w = content_w / 2;
        }
        egui_view_toast_stack_draw_pill(local->meta_font, self, snapshot->action, title_x, footer_y, action_w, action_h, 5, action_fill,
                                        local->compact_mode ? 34 : 40, action_border, local->compact_mode ? 46 : 54, action_text);
    }

    if (snapshot->meta != NULL && snapshot->meta[0] != '\0')
    {
        egui_dim_t meta_x;
        egui_dim_t min_meta_w = local->compact_mode ? 20 : 28;

        if (meta_w < min_meta_w)
        {
            meta_w = min_meta_w;
        }
        if (meta_w > content_w / 2)
        {
            meta_w = content_w / 2;
        }
        meta_x = content_x + content_w - meta_w;
        if (show_action && meta_x < title_x + action_w + footer_gap)
        {
            meta_x = title_x + action_w + footer_gap;
            meta_w = content_x + content_w - meta_x;
        }

        egui_view_toast_stack_draw_pill(local->meta_font, self, snapshot->meta, meta_x, content_y + content_h - meta_h, meta_w, meta_h, 5,
                                        egui_rgb_mix(local->surface_color, local->border_color, 6), local->compact_mode ? 28 : 32,
                                        egui_rgb_mix(local->border_color, severity_color, 8), local->compact_mode ? 34 : 40, meta_color);
    }

    if (local->locked_mode || !is_enabled)
    {
        egui_canvas_draw_line(content_x + 1, content_y + content_h, content_x + content_w - 2, content_y + content_h, 1, front_border,
                              egui_color_alpha_mix(self->alpha, 34));
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_toast_stack_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_toast_stack_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_toast_stack_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_toast_stack_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_toast_stack_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD8DFE7);
    local->text_color = EGUI_COLOR_HEX(0x17212B);
    local->muted_text_color = EGUI_COLOR_HEX(0x5B6878);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->info_color = EGUI_COLOR_HEX(0x2563EB);
    local->success_color = EGUI_COLOR_HEX(0x0F9D58);
    local->warning_color = EGUI_COLOR_HEX(0xC77C11);
    local->error_color = EGUI_COLOR_HEX(0xC93C37);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->compact_mode = 0;
    local->locked_mode = 0;
}
