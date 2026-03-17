#include "egui_view_badge_group.h"

static uint8_t egui_view_badge_group_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_BADGE_GROUP_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_BADGE_GROUP_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_badge_group_clamp_item_count(uint8_t count)
{
    if (count > EGUI_VIEW_BADGE_GROUP_MAX_ITEMS)
    {
        return EGUI_VIEW_BADGE_GROUP_MAX_ITEMS;
    }
    return count;
}

static uint8_t egui_view_badge_group_text_len(const char *text)
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

static uint8_t egui_view_badge_group_focus_index(const egui_view_badge_group_snapshot_t *snapshot, uint8_t item_count)
{
    if (snapshot == NULL || item_count == 0 || snapshot->focus_index >= item_count)
    {
        return 0;
    }
    return snapshot->focus_index;
}

static egui_color_t egui_view_badge_group_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 68);
}

static egui_color_t egui_view_badge_group_tone_color(egui_view_badge_group_t *local, uint8_t tone)
{
    switch (tone)
    {
    case 1:
        return local->success_color;
    case 2:
        return local->warning_color;
    case 3:
        return local->neutral_color;
    default:
        return local->accent_color;
    }
}

static void egui_view_badge_group_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                            egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (text == NULL || text[0] == '\0')
    {
        return;
    }
    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static egui_dim_t egui_view_badge_group_pill_width(const char *text, uint8_t compact_mode, egui_dim_t min_w, egui_dim_t max_w)
{
    egui_dim_t width = min_w + egui_view_badge_group_text_len(text) * (compact_mode ? 4 : 5);

    if (width < min_w)
    {
        width = min_w;
    }
    if (width > max_w)
    {
        width = max_w;
    }
    return width;
}

static void egui_view_badge_group_draw_badge(egui_view_t *self, egui_view_badge_group_t *local, const egui_view_badge_group_item_t *item, egui_dim_t x,
                                             egui_dim_t y, egui_dim_t width, egui_dim_t height, uint8_t focused)
{
    egui_region_t text_region;
    egui_color_t tone_color;
    egui_color_t fill_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t meta_fill;
    egui_color_t meta_text;
    egui_dim_t radius;
    egui_dim_t meta_w;
    uint8_t is_enabled;

    tone_color = egui_view_badge_group_tone_color(local, item->tone);
    fill_color = item->outlined ? local->surface_color : egui_rgb_mix(local->surface_color, tone_color, item->emphasized ? 18 : 10);
    border_color = egui_rgb_mix(local->border_color, tone_color, focused ? 28 : (item->outlined ? 20 : 16));
    text_color = focused ? tone_color : egui_rgb_mix(local->text_color, tone_color, item->emphasized ? 10 : 4);
    meta_fill = egui_rgb_mix(local->surface_color, tone_color, item->emphasized ? 26 : 16);
    meta_text = tone_color;
    radius = height / 2;
    is_enabled = egui_view_get_enable(self) ? 1 : 0;

    if (local->locked_mode)
    {
        fill_color = egui_rgb_mix(fill_color, local->surface_color, 34);
        border_color = egui_rgb_mix(border_color, local->muted_text_color, 20);
        text_color = egui_rgb_mix(text_color, local->muted_text_color, 30);
        meta_fill = egui_rgb_mix(meta_fill, local->surface_color, 38);
        meta_text = egui_rgb_mix(meta_text, local->muted_text_color, 34);
    }

    if (!is_enabled)
    {
        fill_color = egui_view_badge_group_mix_disabled(fill_color);
        border_color = egui_view_badge_group_mix_disabled(border_color);
        text_color = egui_view_badge_group_mix_disabled(text_color);
        meta_fill = egui_view_badge_group_mix_disabled(meta_fill);
        meta_text = egui_view_badge_group_mix_disabled(meta_text);
    }

    egui_canvas_draw_round_rectangle_fill(x, y, width, height, radius, fill_color,
                                          egui_color_alpha_mix(self->alpha, item->outlined ? 80 : (focused ? 100 : 94)));
    egui_canvas_draw_round_rectangle(x, y, width, height, radius, 1, border_color, egui_color_alpha_mix(self->alpha, focused ? 68 : 54));

    meta_w = 0;
    if (item->meta != NULL && item->meta[0] != '\0')
    {
        meta_w = 10 + egui_view_badge_group_text_len(item->meta) * (local->compact_mode ? 4 : 5);
        if (meta_w < (local->compact_mode ? 16 : 18))
        {
            meta_w = local->compact_mode ? 16 : 18;
        }
        if (meta_w > width / 2)
        {
            meta_w = width / 2;
        }

        egui_canvas_draw_round_rectangle_fill(x + width - meta_w - 3, y + 2, meta_w, height - 4, (height - 4) / 2, meta_fill,
                                              egui_color_alpha_mix(self->alpha, focused ? 92 : 86));

        text_region.location.x = x + width - meta_w - 3;
        text_region.location.y = y;
        text_region.size.width = meta_w;
        text_region.size.height = height;
        egui_view_badge_group_draw_text(local->meta_font, self, item->meta, &text_region, EGUI_ALIGN_CENTER, meta_text);
    }

    text_region.location.x = x + (local->compact_mode ? 6 : 8);
    text_region.location.y = y;
    text_region.size.width = width - meta_w - (local->compact_mode ? 9 : 13);
    text_region.size.height = height;
    egui_view_badge_group_draw_text(local->meta_font, self, item->label, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color);
}

void egui_view_badge_group_set_snapshots(egui_view_t *self, const egui_view_badge_group_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_badge_group_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_badge_group_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_badge_group_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_badge_group_t);
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

uint8_t egui_view_badge_group_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_badge_group_t);
    return local->current_snapshot;
}

void egui_view_badge_group_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_badge_group_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_badge_group_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_badge_group_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_badge_group_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_badge_group_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_badge_group_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_badge_group_t);
    local->locked_mode = locked_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_badge_group_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                       egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color, egui_color_t warning_color,
                                       egui_color_t neutral_color)
{
    EGUI_LOCAL_INIT(egui_view_badge_group_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->success_color = success_color;
    local->warning_color = warning_color;
    local->neutral_color = neutral_color;
    egui_view_invalidate(self);
}

static void egui_view_badge_group_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_badge_group_t);
    egui_region_t region;
    egui_region_t text_region;
    const egui_view_badge_group_snapshot_t *snapshot;
    const egui_view_badge_group_item_t *focus_item;
    egui_color_t focus_color;
    egui_color_t card_fill;
    egui_color_t card_border;
    egui_color_t title_color;
    egui_color_t body_color;
    egui_color_t footer_color;
    egui_color_t footer_fill;
    egui_color_t footer_border;
    egui_color_t eyebrow_fill;
    egui_color_t eyebrow_border;
    egui_color_t focus_fill;
    egui_color_t focus_border;
    egui_dim_t x;
    egui_dim_t y;
    egui_dim_t w;
    egui_dim_t h;
    egui_dim_t padding;
    egui_dim_t badge_h;
    egui_dim_t title_y;
    egui_dim_t body_y;
    egui_dim_t badges_y;
    egui_dim_t badges_w;
    egui_dim_t footer_y;
    egui_dim_t footer_h;
    egui_dim_t cursor_x;
    egui_dim_t cursor_y;
    egui_dim_t row_gap;
    egui_dim_t badge_gap;
    egui_dim_t eyebrow_w;
    egui_dim_t badge_w;
    egui_dim_t focus_pill_h;
    egui_dim_t focus_pill_w;
    egui_dim_t focus_pill_x;
    egui_dim_t focus_pill_y;
    uint8_t item_count;
    uint8_t focus_index;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    item_count = egui_view_badge_group_clamp_item_count(snapshot->item_count);
    if (item_count == 0)
    {
        return;
    }

    focus_index = egui_view_badge_group_focus_index(snapshot, item_count);
    focus_item = &snapshot->items[focus_index];
    focus_color = egui_view_badge_group_tone_color(local, focus_item->tone);
    card_fill = egui_rgb_mix(local->surface_color, focus_color, local->compact_mode ? 4 : 6);
    card_border = egui_rgb_mix(local->border_color, focus_color, local->compact_mode ? 10 : 14);
    title_color = local->text_color;
    body_color = egui_rgb_mix(local->muted_text_color, local->text_color, local->compact_mode ? 15 : 20);
    footer_color = egui_rgb_mix(local->muted_text_color, focus_color, local->compact_mode ? 16 : 22);
    footer_fill = egui_rgb_mix(local->surface_color, focus_color, local->compact_mode ? 7 : 10);
    footer_border = egui_rgb_mix(local->border_color, focus_color, local->compact_mode ? 16 : 20);
    eyebrow_fill = egui_rgb_mix(local->surface_color, focus_color, local->compact_mode ? 12 : 16);
    eyebrow_border = egui_rgb_mix(local->border_color, focus_color, local->compact_mode ? 16 : 22);
    focus_fill = egui_rgb_mix(local->surface_color, focus_color, local->compact_mode ? 14 : 18);
    focus_border = egui_rgb_mix(local->border_color, focus_color, local->compact_mode ? 20 : 26);

    if (local->locked_mode)
    {
        focus_color = egui_rgb_mix(focus_color, local->muted_text_color, 74);
        card_fill = egui_rgb_mix(card_fill, local->surface_color, 18);
        card_border = egui_rgb_mix(card_border, local->muted_text_color, 20);
        title_color = egui_rgb_mix(title_color, local->muted_text_color, 16);
        body_color = egui_rgb_mix(body_color, local->muted_text_color, 20);
        footer_color = egui_rgb_mix(footer_color, local->muted_text_color, 26);
        footer_fill = egui_rgb_mix(footer_fill, local->surface_color, 26);
        footer_border = egui_rgb_mix(footer_border, local->muted_text_color, 20);
        eyebrow_fill = egui_rgb_mix(eyebrow_fill, local->surface_color, 20);
        eyebrow_border = egui_rgb_mix(eyebrow_border, local->muted_text_color, 18);
        focus_fill = egui_rgb_mix(focus_fill, local->surface_color, 24);
        focus_border = egui_rgb_mix(focus_border, local->muted_text_color, 18);
    }

    if (!egui_view_get_enable(self))
    {
        focus_color = egui_view_badge_group_mix_disabled(focus_color);
        card_fill = egui_view_badge_group_mix_disabled(card_fill);
        card_border = egui_view_badge_group_mix_disabled(card_border);
        title_color = egui_view_badge_group_mix_disabled(title_color);
        body_color = egui_view_badge_group_mix_disabled(body_color);
        footer_color = egui_view_badge_group_mix_disabled(footer_color);
        footer_fill = egui_view_badge_group_mix_disabled(footer_fill);
        footer_border = egui_view_badge_group_mix_disabled(footer_border);
        eyebrow_fill = egui_view_badge_group_mix_disabled(eyebrow_fill);
        eyebrow_border = egui_view_badge_group_mix_disabled(eyebrow_border);
        focus_fill = egui_view_badge_group_mix_disabled(focus_fill);
        focus_border = egui_view_badge_group_mix_disabled(focus_border);
    }

    x = region.location.x;
    y = region.location.y;
    w = region.size.width;
    h = region.size.height;
    padding = local->compact_mode ? 7 : 9;
    badge_h = local->compact_mode ? 11 : 13;
    row_gap = local->compact_mode ? 3 : 4;
    badge_gap = local->compact_mode ? 3 : 4;
    footer_h = local->compact_mode ? 12 : 16;
    footer_y = y + h - padding - footer_h;
    title_y = y + padding + badge_h + 4;
    body_y = title_y + (local->compact_mode ? 11 : 12) + 2;
    badges_y = body_y + (local->compact_mode ? 0 : 10);
    badges_w = w - padding * 2;
    eyebrow_w = egui_view_badge_group_pill_width(snapshot->eyebrow, local->compact_mode, local->compact_mode ? 24 : 28, badges_w);
    focus_pill_h = local->compact_mode ? 10 : 12;
    focus_pill_w = egui_view_badge_group_pill_width(focus_item->label, local->compact_mode, local->compact_mode ? 20 : 24, badges_w / 2);
    focus_pill_x = x + padding + 4;
    focus_pill_y = footer_y + (footer_h - focus_pill_h) / 2;

    egui_canvas_draw_round_rectangle_fill(x, y, w, h, local->compact_mode ? 8 : 10, card_fill, egui_color_alpha_mix(self->alpha, 100));
    egui_canvas_draw_round_rectangle(x, y, w, h, local->compact_mode ? 8 : 10, 1, card_border, egui_color_alpha_mix(self->alpha, 68));
    egui_canvas_draw_round_rectangle_fill(x + 2, y + 2, w - 4, local->compact_mode ? 3 : 4, local->compact_mode ? 6 : 8, focus_color,
                                          egui_color_alpha_mix(self->alpha, local->locked_mode ? 14 : 24));

    egui_canvas_draw_round_rectangle_fill(x + padding, y + padding, eyebrow_w, badge_h, badge_h / 2, eyebrow_fill, egui_color_alpha_mix(self->alpha, 92));
    egui_canvas_draw_round_rectangle(x + padding, y + padding, eyebrow_w, badge_h, badge_h / 2, 1, eyebrow_border, egui_color_alpha_mix(self->alpha, 56));

    text_region.location.x = x + padding;
    text_region.location.y = y + padding;
    text_region.size.width = eyebrow_w;
    text_region.size.height = badge_h;
    egui_view_badge_group_draw_text(local->meta_font, self, snapshot->eyebrow, &text_region, EGUI_ALIGN_CENTER, focus_color);

    text_region.location.x = x + padding;
    text_region.location.y = title_y;
    text_region.size.width = w - padding * 2;
    text_region.size.height = local->compact_mode ? 11 : 13;
    egui_view_badge_group_draw_text(local->font, self, snapshot->title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color);

    if (!local->compact_mode)
    {
        text_region.location.x = x + padding;
        text_region.location.y = body_y;
        text_region.size.width = w - padding * 2;
        text_region.size.height = 11;
        egui_view_badge_group_draw_text(local->font, self, snapshot->body, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, body_color);
    }

    cursor_x = x + padding;
    cursor_y = badges_y;
    for (i = 0; i < item_count; i++)
    {
        badge_w = 18 + egui_view_badge_group_text_len(snapshot->items[i].label) * (local->compact_mode ? 4 : 5);
        if (snapshot->items[i].meta != NULL && snapshot->items[i].meta[0] != '\0')
        {
            badge_w += 12 + egui_view_badge_group_text_len(snapshot->items[i].meta) * (local->compact_mode ? 4 : 5);
        }

        if (badge_w < (local->compact_mode ? 34 : 42))
        {
            badge_w = local->compact_mode ? 34 : 42;
        }
        if (badge_w > badges_w)
        {
            badge_w = badges_w;
        }

        if (cursor_x + badge_w > x + padding + badges_w)
        {
            cursor_x = x + padding;
            cursor_y += badge_h + row_gap;
        }
        if (cursor_y + badge_h > footer_y - 2)
        {
            break;
        }

        egui_view_badge_group_draw_badge(self, local, &snapshot->items[i], cursor_x, cursor_y, badge_w, badge_h, i == focus_index ? 1 : 0);
        cursor_x += badge_w + badge_gap;
    }

    egui_canvas_draw_round_rectangle_fill(x + padding, footer_y, w - padding * 2, footer_h, local->compact_mode ? 6 : 8, footer_fill,
                                          egui_color_alpha_mix(self->alpha, local->compact_mode ? 22 : 28));
    egui_canvas_draw_round_rectangle(x + padding, footer_y, w - padding * 2, footer_h, local->compact_mode ? 6 : 8, 1, footer_border,
                                     egui_color_alpha_mix(self->alpha, local->compact_mode ? 32 : 38));

    if (!local->compact_mode)
    {
        egui_canvas_draw_round_rectangle_fill(focus_pill_x, focus_pill_y, focus_pill_w, focus_pill_h, focus_pill_h / 2, focus_fill,
                                              egui_color_alpha_mix(self->alpha, 94));
        egui_canvas_draw_round_rectangle(focus_pill_x, focus_pill_y, focus_pill_w, focus_pill_h, focus_pill_h / 2, 1, focus_border,
                                         egui_color_alpha_mix(self->alpha, 56));

        text_region.location.x = focus_pill_x;
        text_region.location.y = focus_pill_y;
        text_region.size.width = focus_pill_w;
        text_region.size.height = focus_pill_h;
        egui_view_badge_group_draw_text(local->meta_font, self, focus_item->label, &text_region, EGUI_ALIGN_CENTER, focus_color);

        text_region.location.x = focus_pill_x + focus_pill_w + 4;
        text_region.location.y = footer_y;
        text_region.size.width = x + w - padding - 3 - text_region.location.x;
        text_region.size.height = footer_h;
        egui_view_badge_group_draw_text(local->meta_font, self, snapshot->footer, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, footer_color);
    }
    else
    {
        text_region.location.x = x + padding + 4;
        text_region.location.y = footer_y;
        text_region.size.width = w - padding * 2 - 8;
        text_region.size.height = footer_h;
        egui_view_badge_group_draw_text(local->meta_font, self, snapshot->footer, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, footer_color);
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_badge_group_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_badge_group_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_badge_group_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_badge_group_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_badge_group_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD7DFE7);
    local->text_color = EGUI_COLOR_HEX(0x17212B);
    local->muted_text_color = EGUI_COLOR_HEX(0x617080);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->success_color = EGUI_COLOR_HEX(0x0F9D58);
    local->warning_color = EGUI_COLOR_HEX(0xC77C11);
    local->neutral_color = EGUI_COLOR_HEX(0x6A7480);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->compact_mode = 0;
    local->locked_mode = 0;
}
