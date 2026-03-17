#include "egui_view_expander.h"

#define EGUI_VIEW_EXPANDER_STANDARD_PAD_X         8
#define EGUI_VIEW_EXPANDER_STANDARD_PAD_Y         7
#define EGUI_VIEW_EXPANDER_STANDARD_RADIUS        10
#define EGUI_VIEW_EXPANDER_STANDARD_HEADER_HEIGHT 17
#define EGUI_VIEW_EXPANDER_STANDARD_ITEM_GAP      4
#define EGUI_VIEW_EXPANDER_STANDARD_HEADER_RADIUS 6
#define EGUI_VIEW_EXPANDER_STANDARD_BODY_GAP      2
#define EGUI_VIEW_EXPANDER_STANDARD_BODY_HEIGHT   28
#define EGUI_VIEW_EXPANDER_STANDARD_BODY_RADIUS   7
#define EGUI_VIEW_EXPANDER_STANDARD_META_HEIGHT   11
#define EGUI_VIEW_EXPANDER_STANDARD_EYEBROW_H     11
#define EGUI_VIEW_EXPANDER_STANDARD_FOOTER_H      12
#define EGUI_VIEW_EXPANDER_STANDARD_GLYPH_W       10
#define EGUI_VIEW_EXPANDER_STANDARD_GLYPH_H       10

#define EGUI_VIEW_EXPANDER_COMPACT_PAD_X         5
#define EGUI_VIEW_EXPANDER_COMPACT_PAD_Y         4
#define EGUI_VIEW_EXPANDER_COMPACT_RADIUS        8
#define EGUI_VIEW_EXPANDER_COMPACT_HEADER_HEIGHT 12
#define EGUI_VIEW_EXPANDER_COMPACT_ITEM_GAP      3
#define EGUI_VIEW_EXPANDER_COMPACT_HEADER_RADIUS 4
#define EGUI_VIEW_EXPANDER_COMPACT_BODY_GAP      1
#define EGUI_VIEW_EXPANDER_COMPACT_BODY_HEIGHT   16
#define EGUI_VIEW_EXPANDER_COMPACT_BODY_RADIUS   5
#define EGUI_VIEW_EXPANDER_COMPACT_META_HEIGHT   8
#define EGUI_VIEW_EXPANDER_COMPACT_EYEBROW_H     8
#define EGUI_VIEW_EXPANDER_COMPACT_FOOTER_H      8
#define EGUI_VIEW_EXPANDER_COMPACT_GLYPH_W       8
#define EGUI_VIEW_EXPANDER_COMPACT_GLYPH_H       8

typedef struct egui_view_expander_metrics egui_view_expander_metrics_t;
struct egui_view_expander_metrics
{
    egui_region_t content_region;
    egui_region_t item_regions[EGUI_VIEW_EXPANDER_MAX_ITEMS];
    egui_region_t header_regions[EGUI_VIEW_EXPANDER_MAX_ITEMS];
    egui_region_t body_regions[EGUI_VIEW_EXPANDER_MAX_ITEMS];
    egui_dim_t body_height;
};

static uint8_t expander_clamp_item_count(uint8_t count)
{
    return count > EGUI_VIEW_EXPANDER_MAX_ITEMS ? EGUI_VIEW_EXPANDER_MAX_ITEMS : count;
}

static uint8_t expander_text_len(const char *text)
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

static egui_color_t expander_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 68);
}

static egui_color_t expander_tone_color(egui_view_expander_t *local, uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_EXPANDER_TONE_SUCCESS:
        return local->success_color;
    case EGUI_VIEW_EXPANDER_TONE_WARNING:
        return local->warning_color;
    case EGUI_VIEW_EXPANDER_TONE_NEUTRAL:
        return local->neutral_color;
    default:
        return local->accent_color;
    }
}

static const egui_view_expander_item_t *expander_get_current_item(egui_view_expander_t *local)
{
    if (local->items == NULL || local->item_count == 0 || local->current_index >= local->item_count)
    {
        return NULL;
    }
    return &local->items[local->current_index];
}

static egui_dim_t expander_meta_width(const char *text, uint8_t compact_mode, egui_dim_t max_width)
{
    egui_dim_t width;
    egui_dim_t min_width = compact_mode ? 16 : 22;

    if (text == NULL || text[0] == '\0')
    {
        return 0;
    }

    width = min_width + expander_text_len(text) * (compact_mode ? 4 : 5);
    if (width > max_width)
    {
        width = max_width;
    }
    return width;
}

static egui_dim_t expander_pill_width(const char *text, uint8_t compact_mode, egui_dim_t min_width, egui_dim_t max_width)
{
    egui_dim_t width = min_width;

    if (text != NULL && text[0] != '\0')
    {
        width += expander_text_len(text) * (compact_mode ? 4 : 5);
    }
    if (width > max_width)
    {
        width = max_width;
    }
    return width;
}

static void expander_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align, egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (text == NULL || text[0] == '\0')
    {
        return;
    }

    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void expander_draw_chevron(egui_view_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, uint8_t expanded, egui_color_t color,
                                  egui_alpha_t alpha)
{
    if (expanded)
    {
        egui_canvas_draw_triangle_fill(x, y + 1, x + width, y + 1, x + width / 2, y + height - 1, color, egui_color_alpha_mix(self->alpha, alpha));
    }
    else
    {
        egui_canvas_draw_triangle_fill(x + 1, y, x + 1, y + height, x + width - 1, y + height / 2, color, egui_color_alpha_mix(self->alpha, alpha));
    }
}

static void expander_get_metrics(egui_view_expander_t *local, egui_view_t *self, egui_view_expander_metrics_t *metrics)
{
    egui_region_t region;
    egui_dim_t pad_x = local->compact_mode ? EGUI_VIEW_EXPANDER_COMPACT_PAD_X : EGUI_VIEW_EXPANDER_STANDARD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? EGUI_VIEW_EXPANDER_COMPACT_PAD_Y : EGUI_VIEW_EXPANDER_STANDARD_PAD_Y;
    egui_dim_t header_height = local->compact_mode ? EGUI_VIEW_EXPANDER_COMPACT_HEADER_HEIGHT : EGUI_VIEW_EXPANDER_STANDARD_HEADER_HEIGHT;
    egui_dim_t item_gap = local->compact_mode ? EGUI_VIEW_EXPANDER_COMPACT_ITEM_GAP : EGUI_VIEW_EXPANDER_STANDARD_ITEM_GAP;
    egui_dim_t body_gap = local->compact_mode ? EGUI_VIEW_EXPANDER_COMPACT_BODY_GAP : EGUI_VIEW_EXPANDER_STANDARD_BODY_GAP;
    egui_dim_t body_height = local->compact_mode ? EGUI_VIEW_EXPANDER_COMPACT_BODY_HEIGHT : EGUI_VIEW_EXPANDER_STANDARD_BODY_HEIGHT;
    egui_dim_t min_body_height = local->compact_mode ? 9 : 16;
    egui_dim_t total_height;
    egui_dim_t available_height;
    egui_dim_t cursor_y;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    metrics->content_region.location.x = region.location.x + pad_x;
    metrics->content_region.location.y = region.location.y + pad_y;
    metrics->content_region.size.width = region.size.width - pad_x * 2;
    metrics->content_region.size.height = region.size.height - pad_y * 2;
    metrics->body_height = 0;

    if (local->item_count == 0)
    {
        return;
    }

    available_height = metrics->content_region.size.height;
    total_height = local->item_count * header_height + (local->item_count - 1) * item_gap;
    if (local->expanded_index != EGUI_VIEW_EXPANDER_INDEX_NONE)
    {
        if (total_height + body_gap + body_height > available_height)
        {
            if (available_height > total_height + body_gap)
            {
                body_height = available_height - total_height - body_gap;
                if (body_height < min_body_height)
                {
                    body_height = min_body_height;
                }
            }
            else
            {
                body_height = 0;
            }
        }
        total_height += body_gap + body_height;
        metrics->body_height = body_height;
    }

    cursor_y = metrics->content_region.location.y;
    if (available_height > total_height)
    {
        cursor_y += (available_height - total_height) / 2;
    }

    for (i = 0; i < local->item_count; ++i)
    {
        metrics->header_regions[i].location.x = metrics->content_region.location.x;
        metrics->header_regions[i].location.y = cursor_y;
        metrics->header_regions[i].size.width = metrics->content_region.size.width;
        metrics->header_regions[i].size.height = header_height;
        metrics->body_regions[i].location.x = metrics->content_region.location.x;
        metrics->body_regions[i].location.y = cursor_y + header_height + body_gap;
        metrics->body_regions[i].size.width = metrics->content_region.size.width;
        metrics->body_regions[i].size.height = 0;
        metrics->item_regions[i] = metrics->header_regions[i];

        if (local->expanded_index == i && metrics->body_height > 0)
        {
            metrics->body_regions[i].size.height = metrics->body_height;
            metrics->item_regions[i].size.height = header_height + body_gap + metrics->body_height;
            cursor_y += metrics->item_regions[i].size.height;
        }
        else
        {
            cursor_y += header_height;
        }

        if (i + 1 < local->item_count)
        {
            cursor_y += item_gap;
        }
    }
}

static uint8_t expander_hit_index(egui_view_expander_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_expander_metrics_t metrics;
    uint8_t i;

    if (local->item_count == 0)
    {
        return EGUI_VIEW_EXPANDER_INDEX_NONE;
    }

    expander_get_metrics(local, self, &metrics);
    for (i = 0; i < local->item_count; ++i)
    {
        if (egui_region_pt_in_rect(&metrics.header_regions[i], x, y))
        {
            return i;
        }
    }

    return EGUI_VIEW_EXPANDER_INDEX_NONE;
}

static void expander_set_current_index_inner(egui_view_t *self, uint8_t item_index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_expander_t);

    if (local->item_count == 0)
    {
        local->current_index = 0;
        return;
    }
    if (item_index >= local->item_count)
    {
        item_index = local->item_count - 1;
    }
    if (local->current_index == item_index)
    {
        return;
    }

    local->current_index = item_index;
    if (notify && local->on_selection_changed != NULL)
    {
        local->on_selection_changed(self, item_index);
    }
    egui_view_invalidate(self);
}

static void expander_set_expanded_index_inner(egui_view_t *self, uint8_t item_index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_expander_t);
    uint8_t normalized = item_index;

    if (normalized != EGUI_VIEW_EXPANDER_INDEX_NONE)
    {
        if (local->item_count == 0)
        {
            normalized = EGUI_VIEW_EXPANDER_INDEX_NONE;
        }
        else if (normalized >= local->item_count)
        {
            normalized = local->item_count - 1;
        }
    }

    if (local->expanded_index == normalized)
    {
        return;
    }

    local->expanded_index = normalized;
    if (notify && local->on_expanded_changed != NULL)
    {
        uint8_t callback_index = normalized == EGUI_VIEW_EXPANDER_INDEX_NONE ? local->current_index : normalized;
        local->on_expanded_changed(self, callback_index, normalized == EGUI_VIEW_EXPANDER_INDEX_NONE ? 0 : 1);
    }
    egui_view_invalidate(self);
}

static void expander_toggle_index_inner(egui_view_t *self, uint8_t item_index)
{
    EGUI_LOCAL_INIT(egui_view_expander_t);

    if (local->item_count == 0 || item_index >= local->item_count)
    {
        return;
    }

    expander_set_current_index_inner(self, item_index, 1);
    if (local->expanded_index == item_index)
    {
        expander_set_expanded_index_inner(self, EGUI_VIEW_EXPANDER_INDEX_NONE, 1);
    }
    else
    {
        expander_set_expanded_index_inner(self, item_index, 1);
    }
}

void egui_view_expander_set_items(egui_view_t *self, const egui_view_expander_item_t *items, uint8_t item_count)
{
    EGUI_LOCAL_INIT(egui_view_expander_t);

    local->items = items;
    local->item_count = expander_clamp_item_count(item_count);
    local->current_index = 0;
    local->expanded_index = local->item_count > 0 ? 0 : EGUI_VIEW_EXPANDER_INDEX_NONE;
    local->pressed_index = EGUI_VIEW_EXPANDER_INDEX_NONE;
    egui_view_invalidate(self);
}

uint8_t egui_view_expander_get_item_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_expander_t);
    return local->item_count;
}

void egui_view_expander_set_current_index(egui_view_t *self, uint8_t item_index)
{
    expander_set_current_index_inner(self, item_index, 1);
}

uint8_t egui_view_expander_get_current_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_expander_t);
    return local->current_index;
}

void egui_view_expander_set_expanded_index(egui_view_t *self, uint8_t item_index)
{
    expander_set_expanded_index_inner(self, item_index, 1);
}

uint8_t egui_view_expander_get_expanded_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_expander_t);
    return local->expanded_index;
}

void egui_view_expander_toggle_current(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_expander_t);

    if (local->item_count == 0)
    {
        return;
    }
    expander_toggle_index_inner(self, local->current_index);
}

void egui_view_expander_set_on_selection_changed_listener(egui_view_t *self, egui_view_on_expander_selection_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_expander_t);
    local->on_selection_changed = listener;
}

void egui_view_expander_set_on_expanded_changed_listener(egui_view_t *self, egui_view_on_expander_expanded_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_expander_t);
    local->on_expanded_changed = listener;
}

void egui_view_expander_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_expander_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_expander_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_expander_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_expander_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_expander_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_expander_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_expander_t);
    local->read_only_mode = read_only_mode ? 1 : 0;
    local->pressed_index = EGUI_VIEW_EXPANDER_INDEX_NONE;
    egui_view_invalidate(self);
}

void egui_view_expander_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t section_color,
                                    egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color,
                                    egui_color_t warning_color, egui_color_t neutral_color)
{
    EGUI_LOCAL_INIT(egui_view_expander_t);

    local->surface_color = surface_color;
    local->border_color = border_color;
    local->section_color = section_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->success_color = success_color;
    local->warning_color = warning_color;
    local->neutral_color = neutral_color;
    egui_view_invalidate(self);
}

static void expander_draw_header(egui_view_t *self, egui_view_expander_t *local, const egui_view_expander_item_t *item, const egui_region_t *region,
                                 uint8_t selected, uint8_t expanded, uint8_t pressed)
{
    egui_region_t text_region;
    egui_color_t tone_color = expander_tone_color(local, item->tone);
    egui_color_t header_fill = egui_rgb_mix(local->section_color, tone_color, expanded ? 14 : (selected ? 10 : (item->emphasized ? 8 : 4)));
    egui_color_t header_border = egui_rgb_mix(local->border_color, tone_color, expanded ? 24 : (selected ? 18 : 10));
    egui_color_t header_text = selected || expanded ? egui_rgb_mix(local->text_color, tone_color, 10) : local->text_color;
    egui_color_t chevron_color = selected || expanded ? tone_color : egui_rgb_mix(local->muted_text_color, tone_color, 18);
    egui_color_t meta_fill = egui_rgb_mix(local->surface_color, tone_color, expanded ? 16 : 10);
    egui_color_t meta_border = egui_rgb_mix(local->border_color, tone_color, expanded ? 24 : 16);
    egui_color_t meta_color = expanded ? tone_color : egui_rgb_mix(local->muted_text_color, tone_color, 20);
    egui_dim_t radius = local->compact_mode ? EGUI_VIEW_EXPANDER_COMPACT_HEADER_RADIUS : EGUI_VIEW_EXPANDER_STANDARD_HEADER_RADIUS;
    egui_dim_t glyph_w = local->compact_mode ? EGUI_VIEW_EXPANDER_COMPACT_GLYPH_W : EGUI_VIEW_EXPANDER_STANDARD_GLYPH_W;
    egui_dim_t glyph_h = local->compact_mode ? EGUI_VIEW_EXPANDER_COMPACT_GLYPH_H : EGUI_VIEW_EXPANDER_STANDARD_GLYPH_H;
    egui_dim_t meta_h = local->compact_mode ? EGUI_VIEW_EXPANDER_COMPACT_META_HEIGHT : EGUI_VIEW_EXPANDER_STANDARD_META_HEIGHT;
    egui_dim_t meta_w = expander_meta_width(item->meta, local->compact_mode, region->size.width / 3);
    egui_dim_t inset = local->compact_mode ? 5 : 7;
    egui_dim_t title_x = region->location.x + inset + glyph_w + 6;
    egui_dim_t meta_x = region->location.x + region->size.width - inset - meta_w;

    if (pressed)
    {
        header_fill = egui_rgb_mix(header_fill, tone_color, 12);
        header_border = egui_rgb_mix(header_border, tone_color, 16);
    }

    if (local->read_only_mode)
    {
        header_fill = egui_rgb_mix(header_fill, local->surface_color, 24);
        header_border = egui_rgb_mix(header_border, local->muted_text_color, 20);
        header_text = egui_rgb_mix(header_text, local->muted_text_color, 18);
        chevron_color = egui_rgb_mix(chevron_color, local->muted_text_color, 24);
        meta_fill = egui_rgb_mix(meta_fill, local->surface_color, 24);
        meta_border = egui_rgb_mix(meta_border, local->muted_text_color, 18);
        meta_color = egui_rgb_mix(meta_color, local->muted_text_color, 22);
    }

    if (!egui_view_get_enable(self))
    {
        header_fill = expander_mix_disabled(header_fill);
        header_border = expander_mix_disabled(header_border);
        header_text = expander_mix_disabled(header_text);
        chevron_color = expander_mix_disabled(chevron_color);
        meta_fill = expander_mix_disabled(meta_fill);
        meta_border = expander_mix_disabled(meta_border);
        meta_color = expander_mix_disabled(meta_color);
    }

    egui_canvas_draw_round_rectangle_fill(region->location.x, region->location.y, region->size.width, region->size.height, radius, header_fill,
                                          egui_color_alpha_mix(self->alpha, expanded ? 94 : 88));
    egui_canvas_draw_round_rectangle(region->location.x, region->location.y, region->size.width, region->size.height, radius, 1, header_border,
                                     egui_color_alpha_mix(self->alpha, expanded ? 54 : 34));

    if (selected || expanded)
    {
        egui_dim_t indicator_w = local->compact_mode ? 2 : 3;

        egui_canvas_draw_round_rectangle_fill(region->location.x + 2, region->location.y + 2, indicator_w, region->size.height - 4, 1, tone_color,
                                              egui_color_alpha_mix(self->alpha, expanded ? 96 : 86));
    }

    expander_draw_chevron(self, region->location.x + inset, region->location.y + (region->size.height - glyph_h) / 2, glyph_w, glyph_h, expanded, chevron_color,
                          pressed ? 100 : 92);

    if (meta_w > 0)
    {
        egui_canvas_draw_round_rectangle_fill(meta_x, region->location.y + (region->size.height - meta_h) / 2, meta_w, meta_h, meta_h / 2, meta_fill,
                                              egui_color_alpha_mix(self->alpha, 94));
        egui_canvas_draw_round_rectangle(meta_x, region->location.y + (region->size.height - meta_h) / 2, meta_w, meta_h, meta_h / 2, 1, meta_border,
                                         egui_color_alpha_mix(self->alpha, 42));

        text_region.location.x = meta_x;
        text_region.location.y = region->location.y + (region->size.height - meta_h) / 2;
        text_region.size.width = meta_w;
        text_region.size.height = meta_h;
        expander_draw_text(local->meta_font, self, item->meta, &text_region, EGUI_ALIGN_CENTER, meta_color);
    }

    text_region.location.x = title_x;
    text_region.location.y = region->location.y;
    text_region.size.width = (meta_w > 0 ? meta_x - 4 : region->location.x + region->size.width - inset) - title_x;
    text_region.size.height = region->size.height;
    expander_draw_text(local->font, self, item->title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, header_text);
}

static void expander_draw_body(egui_view_t *self, egui_view_expander_t *local, const egui_view_expander_item_t *item, const egui_region_t *region)
{
    egui_region_t text_region;
    egui_color_t tone_color = expander_tone_color(local, item->tone);
    egui_color_t body_fill = egui_rgb_mix(local->surface_color, tone_color, item->emphasized ? 16 : 12);
    egui_color_t body_border = egui_rgb_mix(local->border_color, tone_color, item->emphasized ? 26 : 20);
    egui_color_t eyebrow_fill = egui_rgb_mix(local->surface_color, tone_color, 20);
    egui_color_t eyebrow_border = egui_rgb_mix(local->border_color, tone_color, 24);
    egui_color_t eyebrow_color = tone_color;
    egui_color_t primary_color = egui_rgb_mix(local->text_color, tone_color, 8);
    egui_color_t secondary_color = egui_rgb_mix(local->muted_text_color, tone_color, 18);
    egui_color_t footer_fill = egui_rgb_mix(local->section_color, tone_color, 12);
    egui_color_t footer_border = egui_rgb_mix(local->border_color, tone_color, 18);
    egui_color_t footer_color = egui_rgb_mix(local->muted_text_color, tone_color, 22);
    egui_dim_t eyebrow_h = local->compact_mode ? EGUI_VIEW_EXPANDER_COMPACT_EYEBROW_H : EGUI_VIEW_EXPANDER_STANDARD_EYEBROW_H;
    egui_dim_t footer_h = local->compact_mode ? EGUI_VIEW_EXPANDER_COMPACT_FOOTER_H : EGUI_VIEW_EXPANDER_STANDARD_FOOTER_H;
    egui_dim_t inner_x = region->location.x + (local->compact_mode ? 5 : 7);
    egui_dim_t inner_y = region->location.y + (local->compact_mode ? 4 : 5);
    egui_dim_t inner_w = region->size.width - (local->compact_mode ? 10 : 14);
    egui_dim_t footer_y = region->location.y + region->size.height - footer_h - (local->compact_mode ? 4 : 5);
    egui_dim_t eyebrow_w = expander_pill_width(item->eyebrow, local->compact_mode, local->compact_mode ? 16 : 20, inner_w / 2);
    egui_dim_t radius = local->compact_mode ? EGUI_VIEW_EXPANDER_COMPACT_BODY_RADIUS : EGUI_VIEW_EXPANDER_STANDARD_BODY_RADIUS;

    if (local->read_only_mode)
    {
        body_fill = egui_rgb_mix(body_fill, local->surface_color, 24);
        body_border = egui_rgb_mix(body_border, local->muted_text_color, 20);
        eyebrow_fill = egui_rgb_mix(eyebrow_fill, local->surface_color, 26);
        eyebrow_border = egui_rgb_mix(eyebrow_border, local->muted_text_color, 20);
        eyebrow_color = egui_rgb_mix(eyebrow_color, local->muted_text_color, 30);
        primary_color = egui_rgb_mix(primary_color, local->muted_text_color, 18);
        secondary_color = egui_rgb_mix(secondary_color, local->muted_text_color, 22);
        footer_fill = egui_rgb_mix(footer_fill, local->surface_color, 24);
        footer_border = egui_rgb_mix(footer_border, local->muted_text_color, 18);
        footer_color = egui_rgb_mix(footer_color, local->muted_text_color, 24);
    }

    if (!egui_view_get_enable(self))
    {
        body_fill = expander_mix_disabled(body_fill);
        body_border = expander_mix_disabled(body_border);
        eyebrow_fill = expander_mix_disabled(eyebrow_fill);
        eyebrow_border = expander_mix_disabled(eyebrow_border);
        eyebrow_color = expander_mix_disabled(eyebrow_color);
        primary_color = expander_mix_disabled(primary_color);
        secondary_color = expander_mix_disabled(secondary_color);
        footer_fill = expander_mix_disabled(footer_fill);
        footer_border = expander_mix_disabled(footer_border);
        footer_color = expander_mix_disabled(footer_color);
    }

    egui_canvas_draw_round_rectangle_fill(region->location.x, region->location.y, region->size.width, region->size.height, radius, body_fill,
                                          egui_color_alpha_mix(self->alpha, 92));
    egui_canvas_draw_round_rectangle(region->location.x, region->location.y, region->size.width, region->size.height, radius, 1, body_border,
                                     egui_color_alpha_mix(self->alpha, 40));

    egui_canvas_draw_round_rectangle_fill(region->location.x + 3, region->location.y + 3, 3, region->size.height - 6, 1, tone_color,
                                          egui_color_alpha_mix(self->alpha, local->read_only_mode ? 44 : 82));

    if (!local->compact_mode && item->eyebrow != NULL && item->eyebrow[0] != '\0')
    {
        egui_canvas_draw_round_rectangle_fill(inner_x, inner_y, eyebrow_w, eyebrow_h, eyebrow_h / 2, eyebrow_fill, egui_color_alpha_mix(self->alpha, 94));
        egui_canvas_draw_round_rectangle(inner_x, inner_y, eyebrow_w, eyebrow_h, eyebrow_h / 2, 1, eyebrow_border, egui_color_alpha_mix(self->alpha, 42));

        text_region.location.x = inner_x;
        text_region.location.y = inner_y;
        text_region.size.width = eyebrow_w;
        text_region.size.height = eyebrow_h;
        expander_draw_text(local->meta_font, self, item->eyebrow, &text_region, EGUI_ALIGN_CENTER, eyebrow_color);
    }
    else
    {
        eyebrow_h = 0;
    }

    text_region.location.x = inner_x;
    text_region.location.y = local->compact_mode ? (region->location.y + (region->size.height - 8) / 2) : (inner_y + eyebrow_h + 3);
    text_region.size.width = inner_w;
    text_region.size.height = local->compact_mode ? 8 : 10;
    expander_draw_text(local->meta_font, self, item->body_primary, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, primary_color);

    if (local->compact_mode)
    {
        return;
    }

    if (item->body_secondary != NULL && item->body_secondary[0] != '\0')
    {
        text_region.location.y += text_region.size.height;
        text_region.size.height = 9;
        expander_draw_text(local->meta_font, self, item->body_secondary, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, secondary_color);
    }

    egui_canvas_draw_round_rectangle_fill(inner_x, footer_y, inner_w, footer_h, footer_h / 2, footer_fill, egui_color_alpha_mix(self->alpha, 90));
    egui_canvas_draw_round_rectangle(inner_x, footer_y, inner_w, footer_h, footer_h / 2, 1, footer_border, egui_color_alpha_mix(self->alpha, 36));

    text_region.location.x = inner_x + (local->compact_mode ? 3 : 5);
    text_region.location.y = footer_y;
    text_region.size.width = inner_w - (local->compact_mode ? 6 : 10);
    text_region.size.height = footer_h;
    expander_draw_text(local->meta_font, self, item->footer, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, footer_color);
}

static void egui_view_expander_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_expander_t);
    egui_region_t region;
    egui_view_expander_metrics_t metrics;
    const egui_view_expander_item_t *current_item;
    egui_color_t focus_color;
    egui_color_t card_fill;
    egui_color_t card_border;
    uint8_t current_index;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->items == NULL || local->item_count == 0)
    {
        return;
    }

    current_index = local->current_index >= local->item_count ? 0 : local->current_index;
    local->current_index = current_index;
    if (local->expanded_index != EGUI_VIEW_EXPANDER_INDEX_NONE && local->expanded_index >= local->item_count)
    {
        local->expanded_index = 0;
    }
    current_item = expander_get_current_item(local);
    if (current_item == NULL)
    {
        return;
    }

    focus_color = expander_tone_color(local, current_item->tone);
    card_fill = egui_rgb_mix(local->surface_color, focus_color, local->compact_mode ? 4 : 6);
    card_border = egui_rgb_mix(local->border_color, focus_color, local->compact_mode ? 12 : 14);
    if (local->read_only_mode)
    {
        focus_color = egui_rgb_mix(focus_color, local->muted_text_color, 76);
        card_fill = egui_rgb_mix(card_fill, local->surface_color, 18);
        card_border = egui_rgb_mix(card_border, local->muted_text_color, 18);
    }
    if (!egui_view_get_enable(self))
    {
        focus_color = expander_mix_disabled(focus_color);
        card_fill = expander_mix_disabled(card_fill);
        card_border = expander_mix_disabled(card_border);
    }

    expander_get_metrics(local, self, &metrics);

    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height,
                                          local->compact_mode ? EGUI_VIEW_EXPANDER_COMPACT_RADIUS : EGUI_VIEW_EXPANDER_STANDARD_RADIUS, card_fill,
                                          egui_color_alpha_mix(self->alpha, 100));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height,
                                     local->compact_mode ? EGUI_VIEW_EXPANDER_COMPACT_RADIUS : EGUI_VIEW_EXPANDER_STANDARD_RADIUS, 1, card_border,
                                     egui_color_alpha_mix(self->alpha, local->compact_mode ? 58 : 64));
    egui_canvas_draw_round_rectangle_fill(region.location.x + 2, region.location.y + 2, region.size.width - 4, local->compact_mode ? 3 : 4,
                                          local->compact_mode ? EGUI_VIEW_EXPANDER_COMPACT_RADIUS : EGUI_VIEW_EXPANDER_STANDARD_RADIUS, focus_color,
                                          egui_color_alpha_mix(self->alpha, local->read_only_mode ? 14 : 24));

    for (i = 0; i < local->item_count; ++i)
    {
        expander_draw_header(self, local, &local->items[i], &metrics.header_regions[i], local->current_index == i ? 1 : 0, local->expanded_index == i ? 1 : 0,
                             local->pressed_index == i ? 1 : 0);
        if (local->expanded_index == i && metrics.body_regions[i].size.height > 0)
        {
            expander_draw_body(self, local, &local->items[i], &metrics.body_regions[i]);
        }
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_expander_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_expander_t);
    uint8_t hit_index;

    if (local->items == NULL || local->item_count == 0 || !egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_index = expander_hit_index(local, self, event->location.x, event->location.y);
        if (hit_index == EGUI_VIEW_EXPANDER_INDEX_NONE)
        {
            return 0;
        }
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (self->is_focusable)
        {
            egui_view_request_focus(self);
        }
#endif
        local->pressed_index = hit_index;
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_index = expander_hit_index(local, self, event->location.x, event->location.y);
        if (local->pressed_index != EGUI_VIEW_EXPANDER_INDEX_NONE && local->pressed_index == hit_index)
        {
            expander_toggle_index_inner(self, hit_index);
        }
        local->pressed_index = EGUI_VIEW_EXPANDER_INDEX_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_index != EGUI_VIEW_EXPANDER_INDEX_NONE ? 1 : 0;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_index = EGUI_VIEW_EXPANDER_INDEX_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_expander_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_expander_t);
    uint8_t next_index;

    if (local->items == NULL || local->item_count == 0 || !egui_view_get_enable(self) || local->read_only_mode)
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
            return 1;
        default:
            return 0;
        }
    }

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_UP:
        next_index = local->current_index > 0 ? (local->current_index - 1) : 0;
        expander_set_current_index_inner(self, next_index, 1);
        return 1;
    case EGUI_KEY_CODE_DOWN:
        next_index = local->current_index + 1 < local->item_count ? (local->current_index + 1) : (local->item_count - 1);
        expander_set_current_index_inner(self, next_index, 1);
        return 1;
    case EGUI_KEY_CODE_HOME:
        expander_set_current_index_inner(self, 0, 1);
        return 1;
    case EGUI_KEY_CODE_END:
        expander_set_current_index_inner(self, local->item_count - 1, 1);
        return 1;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        egui_view_expander_toggle_current(self);
        return 1;
    default:
        return egui_view_on_key_event(self, event);
    }
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_expander_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_expander_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_expander_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_expander_on_key_event,
#endif
};

void egui_view_expander_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_expander_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_expander_t);
    egui_view_set_padding_all(self, 2);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(self, true);
#endif

    local->items = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->on_selection_changed = NULL;
    local->on_expanded_changed = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD7DFE7);
    local->section_color = EGUI_COLOR_HEX(0xF8FAFC);
    local->text_color = EGUI_COLOR_HEX(0x18222D);
    local->muted_text_color = EGUI_COLOR_HEX(0x667688);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->success_color = EGUI_COLOR_HEX(0x178454);
    local->warning_color = EGUI_COLOR_HEX(0xB77719);
    local->neutral_color = EGUI_COLOR_HEX(0x7A8796);
    local->item_count = 0;
    local->current_index = 0;
    local->expanded_index = EGUI_VIEW_EXPANDER_INDEX_NONE;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    local->pressed_index = EGUI_VIEW_EXPANDER_INDEX_NONE;

    egui_view_set_view_name(self, "egui_view_expander");
}
