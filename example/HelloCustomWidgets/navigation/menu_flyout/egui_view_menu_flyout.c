#include "egui_view_menu_flyout.h"

static uint8_t egui_view_menu_flyout_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_MENU_FLYOUT_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_MENU_FLYOUT_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_menu_flyout_clamp_item_count(uint8_t count)
{
    if (count > EGUI_VIEW_MENU_FLYOUT_MAX_ITEMS)
    {
        return EGUI_VIEW_MENU_FLYOUT_MAX_ITEMS;
    }
    return count;
}

static uint8_t egui_view_menu_flyout_text_len(const char *text)
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

static uint8_t egui_view_menu_flyout_focus_index(const egui_view_menu_flyout_snapshot_t *snapshot, uint8_t item_count)
{
    if (snapshot == NULL || item_count == 0 || snapshot->focus_index >= item_count)
    {
        return 0;
    }

    return snapshot->focus_index;
}

static egui_color_t egui_view_menu_flyout_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 68);
}

static egui_color_t egui_view_menu_flyout_tone_color(egui_view_menu_flyout_t *local, uint8_t tone)
{
    switch (tone)
    {
    case 1:
        return local->success_color;
    case 2:
        return local->warning_color;
    case 3:
        return local->danger_color;
    default:
        return local->accent_color;
    }
}

static egui_dim_t egui_view_menu_flyout_meta_width(const char *text, uint8_t compact_mode, egui_dim_t max_w)
{
    egui_dim_t width;

    if (text == NULL || text[0] == '\0')
    {
        return 0;
    }

    width = 6 + egui_view_menu_flyout_text_len(text) * (compact_mode ? 4 : 5);
    if (width > max_w)
    {
        width = max_w;
    }

    return width;
}

static void egui_view_menu_flyout_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                            egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (text == NULL || text[0] == '\0')
    {
        return;
    }

    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void egui_view_menu_flyout_draw_row(egui_view_t *self, egui_view_menu_flyout_t *local, const egui_view_menu_flyout_item_t *item, egui_dim_t x,
                                           egui_dim_t y, egui_dim_t width, egui_dim_t height, uint8_t focused)
{
    egui_region_t text_region;
    egui_color_t tone_color = egui_view_menu_flyout_tone_color(local, item->tone);
    egui_color_t icon_fill = egui_rgb_mix(local->surface_color, tone_color, focused ? 16 : 10);
    egui_color_t icon_text = focused ? tone_color : egui_rgb_mix(local->text_color, tone_color, 12);
    egui_color_t title_color = item->tone == 3 ? tone_color : local->text_color;
    egui_color_t meta_color = egui_rgb_mix(local->muted_text_color, tone_color, focused ? 20 : 10);
    egui_color_t row_fill = egui_rgb_mix(local->surface_color, tone_color, focused ? 11 : (item->emphasized ? 5 : 3));
    egui_color_t separator_color = egui_rgb_mix(local->border_color, local->surface_color, 38);
    egui_dim_t icon_size = local->compact_mode ? 10 : 12;
    egui_dim_t icon_x = x + (local->compact_mode ? 6 : 7);
    egui_dim_t icon_y = y + (height - icon_size) / 2;
    egui_dim_t trailing_w = item->trailing_kind == EGUI_VIEW_MENU_FLYOUT_TRAILING_SUBMENU ? (local->compact_mode ? 8 : 10) : 0;
    egui_dim_t trailing_x = x + width - trailing_w - (local->compact_mode ? 8 : 9);
    egui_dim_t meta_max_w = width / 3;
    egui_dim_t meta_w = egui_view_menu_flyout_meta_width(item->meta, local->compact_mode, meta_max_w);
    egui_dim_t meta_x = trailing_x - meta_w - (meta_w > 0 && trailing_w > 0 ? 3 : 0);
    egui_dim_t title_x = icon_x + icon_size + (local->compact_mode ? 6 : 8);

    if (item->separator_before)
    {
        egui_canvas_draw_line(x + 8, y - 1, x + width - 8, y - 1, 1, separator_color, egui_color_alpha_mix(self->alpha, 42));
    }

    if (item->enabled == 0 || local->disabled_mode)
    {
        icon_fill = egui_rgb_mix(icon_fill, local->surface_color, 26);
        icon_text = egui_rgb_mix(icon_text, local->muted_text_color, 30);
        title_color = egui_rgb_mix(title_color, local->muted_text_color, 28);
        meta_color = egui_rgb_mix(meta_color, local->muted_text_color, 36);
        row_fill = egui_rgb_mix(row_fill, local->surface_color, 26);
    }

    if (!egui_view_get_enable(self))
    {
        icon_fill = egui_view_menu_flyout_mix_disabled(icon_fill);
        icon_text = egui_view_menu_flyout_mix_disabled(icon_text);
        title_color = egui_view_menu_flyout_mix_disabled(title_color);
        meta_color = egui_view_menu_flyout_mix_disabled(meta_color);
        row_fill = egui_view_menu_flyout_mix_disabled(row_fill);
    }

    egui_canvas_draw_round_rectangle_fill(x, y, width, height, local->compact_mode ? 6 : 7, row_fill, egui_color_alpha_mix(self->alpha, focused ? 94 : 64));
    egui_canvas_draw_round_rectangle_fill(icon_x, icon_y, icon_size, icon_size, local->compact_mode ? 3 : 4, icon_fill, egui_color_alpha_mix(self->alpha, 96));

    text_region.location.x = icon_x;
    text_region.location.y = icon_y;
    text_region.size.width = icon_size;
    text_region.size.height = icon_size;
    egui_view_menu_flyout_draw_text(local->meta_font, self, item->icon_text, &text_region, EGUI_ALIGN_CENTER, icon_text);

    text_region.location.x = title_x;
    text_region.location.y = y;
    text_region.size.width = meta_x - title_x - (local->compact_mode ? 5 : 6);
    text_region.size.height = height;
    egui_view_menu_flyout_draw_text(local->font, self, item->title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color);

    if (meta_w > 0)
    {
        text_region.location.x = meta_x;
        text_region.location.y = y;
        text_region.size.width = meta_w;
        text_region.size.height = height;
        egui_view_menu_flyout_draw_text(local->meta_font, self, item->meta, &text_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER, meta_color);
    }

    if (item->trailing_kind == EGUI_VIEW_MENU_FLYOUT_TRAILING_SUBMENU)
    {
        text_region.location.x = trailing_x;
        text_region.location.y = y;
        text_region.size.width = trailing_w;
        text_region.size.height = height;
        egui_view_menu_flyout_draw_text(local->meta_font, self, ">", &text_region, EGUI_ALIGN_CENTER, meta_color);
    }
}

void egui_view_menu_flyout_set_snapshots(egui_view_t *self, const egui_view_menu_flyout_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_menu_flyout_t);

    local->snapshots = snapshots;
    local->snapshot_count = egui_view_menu_flyout_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }

    egui_view_invalidate(self);
}

void egui_view_menu_flyout_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_menu_flyout_t);

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

uint8_t egui_view_menu_flyout_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_menu_flyout_t);
    return local->current_snapshot;
}

void egui_view_menu_flyout_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_menu_flyout_t);

    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_menu_flyout_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_menu_flyout_t);

    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_menu_flyout_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_menu_flyout_t);

    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_menu_flyout_set_disabled_mode(egui_view_t *self, uint8_t disabled_mode)
{
    EGUI_LOCAL_INIT(egui_view_menu_flyout_t);

    local->disabled_mode = disabled_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_menu_flyout_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                       egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color, egui_color_t warning_color,
                                       egui_color_t danger_color, egui_color_t shadow_color)
{
    EGUI_LOCAL_INIT(egui_view_menu_flyout_t);

    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->success_color = success_color;
    local->warning_color = warning_color;
    local->danger_color = danger_color;
    local->shadow_color = shadow_color;
    egui_view_invalidate(self);
}

static void egui_view_menu_flyout_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_menu_flyout_t);
    egui_region_t region;
    const egui_view_menu_flyout_snapshot_t *snapshot;
    uint8_t item_count;
    uint8_t focus_index;
    egui_color_t border_color = local->border_color;
    egui_color_t surface_color = local->surface_color;
    egui_color_t shadow_color = local->shadow_color;
    egui_dim_t x;
    egui_dim_t y;
    egui_dim_t w;
    egui_dim_t h;
    egui_dim_t padding;
    egui_dim_t row_h;
    egui_dim_t cursor_y;
    egui_dim_t max_y;
    egui_dim_t radius;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    item_count = egui_view_menu_flyout_clamp_item_count(snapshot->item_count);
    if (item_count == 0)
    {
        return;
    }

    focus_index = egui_view_menu_flyout_focus_index(snapshot, item_count);
    if (local->disabled_mode)
    {
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xF3F5F8), 32);
        border_color = egui_rgb_mix(border_color, EGUI_COLOR_HEX(0xC8D1DA), 24);
        shadow_color = egui_rgb_mix(shadow_color, surface_color, 36);
    }

    if (!egui_view_get_enable(self))
    {
        surface_color = egui_view_menu_flyout_mix_disabled(surface_color);
        border_color = egui_view_menu_flyout_mix_disabled(border_color);
        shadow_color = egui_view_menu_flyout_mix_disabled(shadow_color);
    }

    x = region.location.x;
    y = region.location.y;
    w = region.size.width;
    h = region.size.height;
    padding = local->compact_mode ? 5 : 7;
    row_h = local->compact_mode ? 18 : 20;
    cursor_y = y + padding;
    max_y = y + h - padding;
    radius = local->compact_mode ? 8 : 10;

    egui_canvas_draw_round_rectangle_fill(x + 2, y + 3, w - 2, h - 2, radius, shadow_color, egui_color_alpha_mix(self->alpha, local->compact_mode ? 16 : 22));
    egui_canvas_draw_round_rectangle_fill(x, y, w, h, radius, surface_color, egui_color_alpha_mix(self->alpha, 100));
    egui_canvas_draw_round_rectangle(x, y, w, h, radius, 1, border_color, egui_color_alpha_mix(self->alpha, 62));

    for (i = 0; i < item_count; i++)
    {
        egui_dim_t row_y = cursor_y;

        if (snapshot->items[i].separator_before)
        {
            row_y += 2;
        }

        if (row_y + row_h > max_y)
        {
            break;
        }

        egui_view_menu_flyout_draw_row(self, local, &snapshot->items[i], x + padding, row_y, w - padding * 2, row_h, i == focus_index ? 1 : 0);
        cursor_y = row_y + row_h + 1;
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_menu_flyout_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_menu_flyout_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_menu_flyout_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_menu_flyout_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_menu_flyout_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD6DDE5);
    local->text_color = EGUI_COLOR_HEX(0x1A2430);
    local->muted_text_color = EGUI_COLOR_HEX(0x6B7887);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->success_color = EGUI_COLOR_HEX(0x178454);
    local->warning_color = EGUI_COLOR_HEX(0xB87A16);
    local->danger_color = EGUI_COLOR_HEX(0xB33A33);
    local->shadow_color = EGUI_COLOR_HEX(0xC8D2DC);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->compact_mode = 0;
    local->disabled_mode = 0;
}
