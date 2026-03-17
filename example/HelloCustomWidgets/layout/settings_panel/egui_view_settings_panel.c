#include "egui_view_settings_panel.h"

static uint8_t egui_view_settings_panel_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_SETTINGS_PANEL_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_SETTINGS_PANEL_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_settings_panel_clamp_item_count(uint8_t count)
{
    if (count > EGUI_VIEW_SETTINGS_PANEL_MAX_ITEMS)
    {
        return EGUI_VIEW_SETTINGS_PANEL_MAX_ITEMS;
    }
    return count;
}

static uint8_t egui_view_settings_panel_text_len(const char *text)
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

static uint8_t egui_view_settings_panel_focus_index(const egui_view_settings_panel_snapshot_t *snapshot, uint8_t item_count)
{
    if (snapshot == NULL || item_count == 0 || snapshot->focus_index >= item_count)
    {
        return 0;
    }
    return snapshot->focus_index;
}

static egui_color_t egui_view_settings_panel_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 68);
}

static egui_color_t egui_view_settings_panel_tone_color(egui_view_settings_panel_t *local, uint8_t tone)
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

static egui_dim_t egui_view_settings_panel_pill_width(const char *text, uint8_t compact_mode, egui_dim_t min_w, egui_dim_t max_w)
{
    egui_dim_t width = min_w + egui_view_settings_panel_text_len(text) * (compact_mode ? 4 : 5);

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

static egui_dim_t egui_view_settings_panel_trailing_inset(uint8_t compact_mode)
{
    return compact_mode ? 7 : 6;
}

static egui_dim_t egui_view_settings_panel_title_gap(uint8_t compact_mode)
{
    return compact_mode ? 6 : 4;
}

static void egui_view_settings_panel_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                               egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (text == NULL || text[0] == '\0')
    {
        return;
    }
    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void egui_view_settings_panel_draw_switch(egui_view_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, uint8_t is_checked,
                                                 egui_color_t tone_color, egui_color_t muted_color)
{
    egui_color_t track_color = is_checked ? egui_rgb_mix(EGUI_COLOR_WHITE, tone_color, 72) : egui_rgb_mix(EGUI_COLOR_WHITE, muted_color, 34);
    egui_color_t border_color = is_checked ? egui_rgb_mix(muted_color, tone_color, 50) : egui_rgb_mix(muted_color, EGUI_COLOR_WHITE, 12);
    egui_color_t thumb_color = is_checked ? EGUI_COLOR_WHITE : egui_rgb_mix(EGUI_COLOR_WHITE, muted_color, 20);
    egui_dim_t radius = height / 2;
    egui_dim_t thumb_r = radius - 2;
    egui_dim_t thumb_x = is_checked ? (x + width - radius) : (x + radius);
    egui_dim_t thumb_y = y + radius;

    if (thumb_r < 1)
    {
        thumb_r = 1;
    }

    egui_canvas_draw_round_rectangle_fill(x, y, width, height, radius, track_color, egui_color_alpha_mix(self->alpha, 90));
    egui_canvas_draw_round_rectangle(x, y, width, height, radius, 1, border_color, egui_color_alpha_mix(self->alpha, 52));
    egui_canvas_draw_circle_fill(thumb_x, thumb_y, thumb_r, thumb_color, egui_color_alpha_mix(self->alpha, 100));
}

static void egui_view_settings_panel_draw_row(egui_view_t *self, egui_view_settings_panel_t *local, const egui_view_settings_panel_item_t *item, egui_dim_t x,
                                              egui_dim_t y, egui_dim_t width, egui_dim_t height, uint8_t focused, uint8_t last)
{
    egui_region_t text_region;
    egui_color_t tone_color = egui_view_settings_panel_tone_color(local, item->tone);
    egui_color_t row_fill = egui_rgb_mix(local->section_color, tone_color, focused ? 12 : (item->emphasized ? 8 : 4));
    egui_color_t row_border = egui_rgb_mix(local->border_color, tone_color, focused ? 22 : (item->emphasized ? 16 : 8));
    egui_color_t icon_fill = egui_rgb_mix(local->surface_color, tone_color, focused ? 20 : 14);
    egui_color_t icon_text = focused ? tone_color : egui_rgb_mix(local->text_color, tone_color, 10);
    egui_color_t title_color = focused ? egui_rgb_mix(local->text_color, tone_color, 8) : local->text_color;
    egui_color_t divider_color = egui_rgb_mix(local->border_color, tone_color, 10);
    egui_color_t value_fill = egui_rgb_mix(local->surface_color, tone_color, focused ? 14 : 10);
    egui_color_t value_border = egui_rgb_mix(local->border_color, tone_color, focused ? 24 : 16);
    egui_color_t value_color = focused ? tone_color : egui_rgb_mix(local->muted_text_color, tone_color, 18);
    egui_dim_t icon_size = local->compact_mode ? 12 : 14;
    egui_dim_t title_x = x + icon_size + 10;
    egui_dim_t trailing_w = 0;
    egui_dim_t trailing_h = local->compact_mode ? 11 : 13;
    egui_dim_t trailing_x;
    egui_dim_t trailing_y;

    if (local->locked_mode)
    {
        row_fill = egui_rgb_mix(row_fill, local->surface_color, 24);
        row_border = egui_rgb_mix(row_border, local->muted_text_color, 20);
        icon_fill = egui_rgb_mix(icon_fill, local->surface_color, 28);
        icon_text = egui_rgb_mix(icon_text, local->muted_text_color, 28);
        title_color = egui_rgb_mix(title_color, local->muted_text_color, 16);
        divider_color = egui_rgb_mix(divider_color, local->muted_text_color, 12);
        value_fill = egui_rgb_mix(value_fill, local->surface_color, 30);
        value_border = egui_rgb_mix(value_border, local->muted_text_color, 20);
        value_color = egui_rgb_mix(value_color, local->muted_text_color, 28);
    }

    if (!egui_view_get_enable(self))
    {
        row_fill = egui_view_settings_panel_mix_disabled(row_fill);
        row_border = egui_view_settings_panel_mix_disabled(row_border);
        icon_fill = egui_view_settings_panel_mix_disabled(icon_fill);
        icon_text = egui_view_settings_panel_mix_disabled(icon_text);
        title_color = egui_view_settings_panel_mix_disabled(title_color);
        divider_color = egui_view_settings_panel_mix_disabled(divider_color);
        value_fill = egui_view_settings_panel_mix_disabled(value_fill);
        value_border = egui_view_settings_panel_mix_disabled(value_border);
        value_color = egui_view_settings_panel_mix_disabled(value_color);
    }

    egui_canvas_draw_round_rectangle_fill(x, y, width, height, local->compact_mode ? 6 : 8, row_fill, egui_color_alpha_mix(self->alpha, focused ? 88 : 82));
    egui_canvas_draw_round_rectangle(x, y, width, height, local->compact_mode ? 6 : 8, 1, row_border, egui_color_alpha_mix(self->alpha, focused ? 52 : 28));

    egui_canvas_draw_round_rectangle_fill(x + 5, y + (height - icon_size) / 2, icon_size, icon_size, local->compact_mode ? 4 : 5, icon_fill,
                                          egui_color_alpha_mix(self->alpha, 96));

    text_region.location.x = x + 5;
    text_region.location.y = y + (height - icon_size) / 2;
    text_region.size.width = icon_size;
    text_region.size.height = icon_size;
    egui_view_settings_panel_draw_text(local->meta_font, self, item->icon_text, &text_region, EGUI_ALIGN_CENTER, icon_text);

    trailing_y = y + (height - trailing_h) / 2;
    switch (item->trailing_kind)
    {
    case EGUI_VIEW_SETTINGS_PANEL_TRAILING_SWITCH_ON:
    case EGUI_VIEW_SETTINGS_PANEL_TRAILING_SWITCH_OFF:
        trailing_w = local->compact_mode ? 20 : 24;
        trailing_h = local->compact_mode ? 10 : 12;
        trailing_y = y + (height - trailing_h) / 2;
        break;
    case EGUI_VIEW_SETTINGS_PANEL_TRAILING_CHEVRON:
        trailing_w = local->compact_mode ? 8 : 10;
        break;
    default:
        trailing_w = egui_view_settings_panel_pill_width(item->value, local->compact_mode, local->compact_mode ? 20 : 26, width / 3);
        break;
    }
    trailing_x = x + width - trailing_w - egui_view_settings_panel_trailing_inset(local->compact_mode);

    text_region.location.x = title_x;
    text_region.location.y = y;
    text_region.size.width = trailing_x - title_x - egui_view_settings_panel_title_gap(local->compact_mode);
    text_region.size.height = height;
    egui_view_settings_panel_draw_text(local->font, self, item->title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color);

    switch (item->trailing_kind)
    {
    case EGUI_VIEW_SETTINGS_PANEL_TRAILING_SWITCH_ON:
        egui_view_settings_panel_draw_switch(self, trailing_x, trailing_y, trailing_w, trailing_h, 1, tone_color, local->muted_text_color);
        break;
    case EGUI_VIEW_SETTINGS_PANEL_TRAILING_SWITCH_OFF:
        egui_view_settings_panel_draw_switch(self, trailing_x, trailing_y, trailing_w, trailing_h, 0, tone_color, local->muted_text_color);
        break;
    case EGUI_VIEW_SETTINGS_PANEL_TRAILING_CHEVRON:
        text_region.location.x = trailing_x;
        text_region.location.y = y;
        text_region.size.width = trailing_w;
        text_region.size.height = height;
        egui_view_settings_panel_draw_text(local->meta_font, self, ">", &text_region, EGUI_ALIGN_CENTER, value_color);
        break;
    default:
        egui_canvas_draw_round_rectangle_fill(trailing_x, trailing_y, trailing_w, trailing_h, trailing_h / 2, value_fill,
                                              egui_color_alpha_mix(self->alpha, 92));
        egui_canvas_draw_round_rectangle(trailing_x, trailing_y, trailing_w, trailing_h, trailing_h / 2, 1, value_border,
                                         egui_color_alpha_mix(self->alpha, 42));

        text_region.location.x = trailing_x;
        text_region.location.y = trailing_y;
        text_region.size.width = trailing_w;
        text_region.size.height = trailing_h;
        egui_view_settings_panel_draw_text(local->meta_font, self, item->value, &text_region, EGUI_ALIGN_CENTER, value_color);
        break;
    }

    if (!last)
    {
        egui_canvas_draw_line(x + 8, y + height, x + width - 8, y + height, 1, divider_color, egui_color_alpha_mix(self->alpha, 30));
    }
}

void egui_view_settings_panel_set_snapshots(egui_view_t *self, const egui_view_settings_panel_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_settings_panel_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_settings_panel_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_settings_panel_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_settings_panel_t);
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

uint8_t egui_view_settings_panel_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_settings_panel_t);
    return local->current_snapshot;
}

void egui_view_settings_panel_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_settings_panel_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_settings_panel_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_settings_panel_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_settings_panel_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_settings_panel_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_settings_panel_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_settings_panel_t);
    local->locked_mode = locked_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_settings_panel_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t section_color, egui_color_t border_color,
                                          egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color,
                                          egui_color_t warning_color, egui_color_t neutral_color)
{
    EGUI_LOCAL_INIT(egui_view_settings_panel_t);
    local->surface_color = surface_color;
    local->section_color = section_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->success_color = success_color;
    local->warning_color = warning_color;
    local->neutral_color = neutral_color;
    egui_view_invalidate(self);
}

static void egui_view_settings_panel_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_settings_panel_t);
    egui_region_t region;
    egui_region_t text_region;
    const egui_view_settings_panel_snapshot_t *snapshot;
    const egui_view_settings_panel_item_t *focus_item;
    egui_color_t focus_color;
    egui_color_t card_fill;
    egui_color_t card_border;
    egui_color_t title_color;
    egui_color_t body_color;
    egui_color_t group_fill;
    egui_color_t group_border;
    egui_color_t eyebrow_fill;
    egui_color_t eyebrow_border;
    egui_color_t footer_fill;
    egui_color_t footer_border;
    egui_color_t footer_color;
    egui_color_t focus_fill;
    egui_color_t focus_border;
    egui_dim_t x;
    egui_dim_t y;
    egui_dim_t w;
    egui_dim_t h;
    egui_dim_t padding;
    egui_dim_t eyebrow_h;
    egui_dim_t eyebrow_w;
    egui_dim_t title_y;
    egui_dim_t body_y;
    egui_dim_t body_h;
    egui_dim_t group_y;
    egui_dim_t group_h;
    egui_dim_t row_h;
    egui_dim_t row_gap;
    egui_dim_t footer_h;
    egui_dim_t footer_y;
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
    item_count = egui_view_settings_panel_clamp_item_count(snapshot->item_count);
    if (item_count == 0)
    {
        return;
    }

    focus_index = egui_view_settings_panel_focus_index(snapshot, item_count);
    focus_item = &snapshot->items[focus_index];
    focus_color = egui_view_settings_panel_tone_color(local, focus_item->tone);
    card_fill = egui_rgb_mix(local->surface_color, focus_color, local->compact_mode ? 3 : 5);
    card_border = egui_rgb_mix(local->border_color, focus_color, local->compact_mode ? 10 : 14);
    title_color = local->text_color;
    body_color = egui_rgb_mix(local->muted_text_color, local->text_color, local->compact_mode ? 15 : 20);
    group_fill = egui_rgb_mix(local->section_color, focus_color, local->compact_mode ? 3 : 5);
    group_border = egui_rgb_mix(local->border_color, focus_color, local->compact_mode ? 10 : 14);
    eyebrow_fill = egui_rgb_mix(local->surface_color, focus_color, local->compact_mode ? 12 : 16);
    eyebrow_border = egui_rgb_mix(local->border_color, focus_color, local->compact_mode ? 18 : 22);
    footer_fill = egui_rgb_mix(local->surface_color, focus_color, local->compact_mode ? 8 : 10);
    footer_border = egui_rgb_mix(local->border_color, focus_color, local->compact_mode ? 16 : 20);
    footer_color = egui_rgb_mix(local->muted_text_color, focus_color, local->compact_mode ? 16 : 22);
    focus_fill = egui_rgb_mix(local->surface_color, focus_color, local->compact_mode ? 12 : 16);
    focus_border = egui_rgb_mix(local->border_color, focus_color, local->compact_mode ? 18 : 22);

    if (local->locked_mode)
    {
        focus_color = egui_rgb_mix(focus_color, local->muted_text_color, 76);
        card_fill = egui_rgb_mix(card_fill, local->surface_color, 18);
        card_border = egui_rgb_mix(card_border, local->muted_text_color, 18);
        title_color = egui_rgb_mix(title_color, local->muted_text_color, 16);
        body_color = egui_rgb_mix(body_color, local->muted_text_color, 18);
        group_fill = egui_rgb_mix(group_fill, local->surface_color, 16);
        group_border = egui_rgb_mix(group_border, local->muted_text_color, 18);
        eyebrow_fill = egui_rgb_mix(eyebrow_fill, local->surface_color, 20);
        eyebrow_border = egui_rgb_mix(eyebrow_border, local->muted_text_color, 18);
        footer_fill = egui_rgb_mix(footer_fill, local->surface_color, 22);
        footer_border = egui_rgb_mix(footer_border, local->muted_text_color, 18);
        footer_color = egui_rgb_mix(footer_color, local->muted_text_color, 24);
        focus_fill = egui_rgb_mix(focus_fill, local->surface_color, 24);
        focus_border = egui_rgb_mix(focus_border, local->muted_text_color, 20);
    }

    if (!egui_view_get_enable(self))
    {
        focus_color = egui_view_settings_panel_mix_disabled(focus_color);
        card_fill = egui_view_settings_panel_mix_disabled(card_fill);
        card_border = egui_view_settings_panel_mix_disabled(card_border);
        title_color = egui_view_settings_panel_mix_disabled(title_color);
        body_color = egui_view_settings_panel_mix_disabled(body_color);
        group_fill = egui_view_settings_panel_mix_disabled(group_fill);
        group_border = egui_view_settings_panel_mix_disabled(group_border);
        eyebrow_fill = egui_view_settings_panel_mix_disabled(eyebrow_fill);
        eyebrow_border = egui_view_settings_panel_mix_disabled(eyebrow_border);
        footer_fill = egui_view_settings_panel_mix_disabled(footer_fill);
        footer_border = egui_view_settings_panel_mix_disabled(footer_border);
        footer_color = egui_view_settings_panel_mix_disabled(footer_color);
        focus_fill = egui_view_settings_panel_mix_disabled(focus_fill);
        focus_border = egui_view_settings_panel_mix_disabled(focus_border);
    }

    x = region.location.x;
    y = region.location.y;
    w = region.size.width;
    h = region.size.height;
    padding = local->compact_mode ? 7 : 9;
    eyebrow_h = local->compact_mode ? 11 : 13;
    eyebrow_w = egui_view_settings_panel_pill_width(snapshot->eyebrow, local->compact_mode, local->compact_mode ? 26 : 30, w / 2);
    title_y = y + padding + eyebrow_h + 4;
    body_y = title_y + (local->compact_mode ? 11 : 12) + 1;
    body_h = local->compact_mode ? 0 : 10;
    group_y = body_y + body_h + (local->compact_mode ? 4 : 6);
    footer_h = local->compact_mode ? 12 : 16;
    footer_y = y + h - padding - footer_h;
    group_h = footer_y - group_y - 4;
    row_h = local->compact_mode ? 16 : 18;
    row_gap = local->compact_mode ? 2 : 3;
    focus_pill_h = local->compact_mode ? 0 : 12;
    focus_pill_w = egui_view_settings_panel_pill_width(focus_item->title, local->compact_mode, 26, (w - padding * 2) / 3);
    focus_pill_x = x + padding + 4;
    focus_pill_y = footer_y + 2;

    egui_canvas_draw_round_rectangle_fill(x, y, w, h, local->compact_mode ? 8 : 10, card_fill, egui_color_alpha_mix(self->alpha, 100));
    egui_canvas_draw_round_rectangle(x, y, w, h, local->compact_mode ? 8 : 10, 1, card_border, egui_color_alpha_mix(self->alpha, 66));
    egui_canvas_draw_round_rectangle_fill(x + 2, y + 2, w - 4, local->compact_mode ? 3 : 4, local->compact_mode ? 6 : 8, focus_color,
                                          egui_color_alpha_mix(self->alpha, local->locked_mode ? 14 : 24));

    egui_canvas_draw_round_rectangle_fill(x + padding, y + padding, eyebrow_w, eyebrow_h, eyebrow_h / 2, eyebrow_fill, egui_color_alpha_mix(self->alpha, 92));
    egui_canvas_draw_round_rectangle(x + padding, y + padding, eyebrow_w, eyebrow_h, eyebrow_h / 2, 1, eyebrow_border, egui_color_alpha_mix(self->alpha, 54));

    text_region.location.x = x + padding;
    text_region.location.y = y + padding;
    text_region.size.width = eyebrow_w;
    text_region.size.height = eyebrow_h;
    egui_view_settings_panel_draw_text(local->meta_font, self, snapshot->eyebrow, &text_region, EGUI_ALIGN_CENTER, focus_color);

    text_region.location.x = x + padding;
    text_region.location.y = title_y;
    text_region.size.width = w - padding * 2;
    text_region.size.height = local->compact_mode ? 11 : 12;
    egui_view_settings_panel_draw_text(local->font, self, snapshot->title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color);

    if (!local->compact_mode)
    {
        text_region.location.x = x + padding;
        text_region.location.y = body_y;
        text_region.size.width = w - padding * 2;
        text_region.size.height = body_h;
        egui_view_settings_panel_draw_text(local->meta_font, self, snapshot->body, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, body_color);
    }

    egui_canvas_draw_round_rectangle_fill(x + padding, group_y, w - padding * 2, group_h, local->compact_mode ? 7 : 8, group_fill,
                                          egui_color_alpha_mix(self->alpha, 90));
    egui_canvas_draw_round_rectangle(x + padding, group_y, w - padding * 2, group_h, local->compact_mode ? 7 : 8, 1, group_border,
                                     egui_color_alpha_mix(self->alpha, 34));

    for (i = 0; i < item_count; i++)
    {
        egui_dim_t row_y = group_y + 3 + i * (row_h + row_gap);
        egui_dim_t row_x = x + padding + 3;
        egui_dim_t row_w = w - padding * 2 - 6;

        if (row_y + row_h > footer_y - 2)
        {
            break;
        }

        egui_view_settings_panel_draw_row(self, local, &snapshot->items[i], row_x, row_y, row_w, row_h, i == focus_index ? 1 : 0, i == item_count - 1 ? 1 : 0);
    }

    egui_canvas_draw_round_rectangle_fill(x + padding, footer_y, w - padding * 2, footer_h, local->compact_mode ? 6 : 8, footer_fill,
                                          egui_color_alpha_mix(self->alpha, local->compact_mode ? 22 : 28));
    egui_canvas_draw_round_rectangle(x + padding, footer_y, w - padding * 2, footer_h, local->compact_mode ? 6 : 8, 1, footer_border,
                                     egui_color_alpha_mix(self->alpha, local->compact_mode ? 30 : 36));

    if (!local->compact_mode)
    {
        egui_canvas_draw_round_rectangle_fill(focus_pill_x, focus_pill_y, focus_pill_w, focus_pill_h, focus_pill_h / 2, focus_fill,
                                              egui_color_alpha_mix(self->alpha, 94));
        egui_canvas_draw_round_rectangle(focus_pill_x, focus_pill_y, focus_pill_w, focus_pill_h, focus_pill_h / 2, 1, focus_border,
                                         egui_color_alpha_mix(self->alpha, 52));

        text_region.location.x = focus_pill_x;
        text_region.location.y = focus_pill_y;
        text_region.size.width = focus_pill_w;
        text_region.size.height = focus_pill_h;
        egui_view_settings_panel_draw_text(local->meta_font, self, focus_item->title, &text_region, EGUI_ALIGN_CENTER, focus_color);

        text_region.location.x = focus_pill_x + focus_pill_w + 4;
        text_region.location.y = footer_y;
        text_region.size.width = x + w - padding - 4 - text_region.location.x;
        text_region.size.height = footer_h;
        egui_view_settings_panel_draw_text(local->meta_font, self, snapshot->footer, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, footer_color);
    }
    else
    {
        text_region.location.x = x + padding + 4;
        text_region.location.y = footer_y;
        text_region.size.width = w - padding * 2 - 8;
        text_region.size.height = footer_h;
        egui_view_settings_panel_draw_text(local->meta_font, self, snapshot->footer, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, footer_color);
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_settings_panel_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_settings_panel_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_settings_panel_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_settings_panel_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_settings_panel_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->section_color = EGUI_COLOR_HEX(0xF8FAFC);
    local->border_color = EGUI_COLOR_HEX(0xD7DFE7);
    local->text_color = EGUI_COLOR_HEX(0x17212B);
    local->muted_text_color = EGUI_COLOR_HEX(0x657487);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->success_color = EGUI_COLOR_HEX(0x178454);
    local->warning_color = EGUI_COLOR_HEX(0xB67619);
    local->neutral_color = EGUI_COLOR_HEX(0x768392);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->compact_mode = 0;
    local->locked_mode = 0;
}
