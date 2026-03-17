#include "egui_view_master_detail.h"

#define EGUI_VIEW_MASTER_DETAIL_STANDARD_PAD_X          8
#define EGUI_VIEW_MASTER_DETAIL_STANDARD_PAD_Y          7
#define EGUI_VIEW_MASTER_DETAIL_STANDARD_RADIUS         10
#define EGUI_VIEW_MASTER_DETAIL_STANDARD_MASTER_WIDTH   74
#define EGUI_VIEW_MASTER_DETAIL_STANDARD_PANE_GAP       8
#define EGUI_VIEW_MASTER_DETAIL_STANDARD_ROW_HEIGHT     18
#define EGUI_VIEW_MASTER_DETAIL_STANDARD_ROW_GAP        4
#define EGUI_VIEW_MASTER_DETAIL_STANDARD_DETAIL_RADIUS  8
#define EGUI_VIEW_MASTER_DETAIL_STANDARD_BODY_GAP       3
#define EGUI_VIEW_MASTER_DETAIL_STANDARD_FOOTER_HEIGHT  13
#define EGUI_VIEW_MASTER_DETAIL_STANDARD_HEADER_HEIGHT  9
#define EGUI_VIEW_MASTER_DETAIL_STANDARD_HEADER_GAP     3
#define EGUI_VIEW_MASTER_DETAIL_STANDARD_META_HEIGHT    9
#define EGUI_VIEW_MASTER_DETAIL_STANDARD_META_GAP       2
#define EGUI_VIEW_MASTER_DETAIL_STANDARD_BODY_HEIGHT    9
#define EGUI_VIEW_MASTER_DETAIL_STANDARD_DIVIDER_MARGIN 4

#define EGUI_VIEW_MASTER_DETAIL_COMPACT_PAD_X          5
#define EGUI_VIEW_MASTER_DETAIL_COMPACT_PAD_Y          5
#define EGUI_VIEW_MASTER_DETAIL_COMPACT_RADIUS         8
#define EGUI_VIEW_MASTER_DETAIL_COMPACT_MASTER_WIDTH   28
#define EGUI_VIEW_MASTER_DETAIL_COMPACT_PANE_GAP       4
#define EGUI_VIEW_MASTER_DETAIL_COMPACT_ROW_HEIGHT     14
#define EGUI_VIEW_MASTER_DETAIL_COMPACT_ROW_GAP        3
#define EGUI_VIEW_MASTER_DETAIL_COMPACT_DETAIL_RADIUS  6
#define EGUI_VIEW_MASTER_DETAIL_COMPACT_FOOTER_HEIGHT  11
#define EGUI_VIEW_MASTER_DETAIL_COMPACT_HEADER_HEIGHT  8
#define EGUI_VIEW_MASTER_DETAIL_COMPACT_HEADER_GAP     2
#define EGUI_VIEW_MASTER_DETAIL_COMPACT_META_HEIGHT    8
#define EGUI_VIEW_MASTER_DETAIL_COMPACT_META_GAP       1
#define EGUI_VIEW_MASTER_DETAIL_COMPACT_BODY_HEIGHT    8
#define EGUI_VIEW_MASTER_DETAIL_COMPACT_DIVIDER_MARGIN 3

typedef struct egui_view_master_detail_metrics egui_view_master_detail_metrics_t;
struct egui_view_master_detail_metrics
{
    egui_region_t content_region;
    egui_region_t master_region;
    egui_region_t detail_region;
    egui_region_t row_regions[EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS];
    uint8_t body_line_count;
};

static uint8_t egui_view_master_detail_clamp_item_count(uint8_t count)
{
    if (count > EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS)
    {
        return EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS;
    }
    return count;
}

static const egui_view_master_detail_item_t *egui_view_master_detail_get_current_item(egui_view_master_detail_t *local)
{
    if (local->items == NULL || local->item_count == 0 || local->current_index >= local->item_count)
    {
        return NULL;
    }

    return &local->items[local->current_index];
}

static uint8_t egui_view_master_detail_text_len(const char *text)
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

static egui_color_t egui_view_master_detail_tone_color(egui_view_master_detail_t *local, uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_MASTER_DETAIL_TONE_SUCCESS:
        return local->success_color;
    case EGUI_VIEW_MASTER_DETAIL_TONE_WARNING:
        return local->warning_color;
    case EGUI_VIEW_MASTER_DETAIL_TONE_NEUTRAL:
        return local->neutral_color;
    default:
        return local->accent_color;
    }
}

static egui_color_t egui_view_master_detail_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 68);
}

static void egui_view_master_detail_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                              egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (text == NULL || text[0] == '\0')
    {
        return;
    }

    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static egui_dim_t egui_view_master_detail_footer_width(const char *text, uint8_t compact_mode, egui_dim_t max_width)
{
    egui_dim_t width = (compact_mode ? 18 : 22) + egui_view_master_detail_text_len(text) * (compact_mode ? 4 : 5);

    if (width > max_width)
    {
        width = max_width;
    }

    return width;
}

static void egui_view_master_detail_get_metrics(egui_view_master_detail_t *local, egui_view_t *self, egui_view_master_detail_metrics_t *metrics)
{
    egui_region_t work_region;
    egui_dim_t pad_x = local->compact_mode ? EGUI_VIEW_MASTER_DETAIL_COMPACT_PAD_X : EGUI_VIEW_MASTER_DETAIL_STANDARD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? EGUI_VIEW_MASTER_DETAIL_COMPACT_PAD_Y : EGUI_VIEW_MASTER_DETAIL_STANDARD_PAD_Y;
    egui_dim_t master_width = local->compact_mode ? EGUI_VIEW_MASTER_DETAIL_COMPACT_MASTER_WIDTH : EGUI_VIEW_MASTER_DETAIL_STANDARD_MASTER_WIDTH;
    egui_dim_t pane_gap = local->compact_mode ? EGUI_VIEW_MASTER_DETAIL_COMPACT_PANE_GAP : EGUI_VIEW_MASTER_DETAIL_STANDARD_PANE_GAP;
    egui_dim_t row_height = local->compact_mode ? EGUI_VIEW_MASTER_DETAIL_COMPACT_ROW_HEIGHT : EGUI_VIEW_MASTER_DETAIL_STANDARD_ROW_HEIGHT;
    egui_dim_t row_gap = local->compact_mode ? EGUI_VIEW_MASTER_DETAIL_COMPACT_ROW_GAP : EGUI_VIEW_MASTER_DETAIL_STANDARD_ROW_GAP;
    egui_dim_t total_height;
    egui_dim_t start_y;
    uint8_t i;

    egui_view_get_work_region(self, &work_region);
    metrics->content_region.location.x = work_region.location.x + pad_x;
    metrics->content_region.location.y = work_region.location.y + pad_y;
    metrics->content_region.size.width = work_region.size.width - pad_x * 2;
    metrics->content_region.size.height = work_region.size.height - pad_y * 2;

    if (metrics->content_region.size.width < master_width + pane_gap + 28)
    {
        master_width = metrics->content_region.size.width / 3;
    }

    metrics->master_region.location.x = metrics->content_region.location.x;
    metrics->master_region.location.y = metrics->content_region.location.y;
    metrics->master_region.size.width = master_width;
    metrics->master_region.size.height = metrics->content_region.size.height;

    metrics->detail_region.location.x = metrics->master_region.location.x + metrics->master_region.size.width + pane_gap;
    metrics->detail_region.location.y = metrics->content_region.location.y;
    metrics->detail_region.size.width = metrics->content_region.size.width - metrics->master_region.size.width - pane_gap;
    metrics->detail_region.size.height = metrics->content_region.size.height;

    total_height = local->item_count > 0 ? (local->item_count * row_height + (local->item_count - 1) * row_gap) : 0;
    if (total_height > metrics->master_region.size.height)
    {
        row_gap = 1;
        total_height = local->item_count > 0 ? (local->item_count * row_height + (local->item_count - 1) * row_gap) : 0;
    }
    if (total_height > metrics->master_region.size.height)
    {
        row_height = local->compact_mode ? 12 : 15;
        total_height = local->item_count > 0 ? (local->item_count * row_height + (local->item_count - 1) * row_gap) : 0;
    }

    start_y = metrics->master_region.location.y + (metrics->master_region.size.height - total_height) / 2;
    for (i = 0; i < EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS; i++)
    {
        metrics->row_regions[i].location.x = metrics->master_region.location.x;
        metrics->row_regions[i].location.y = start_y + i * (row_height + row_gap);
        metrics->row_regions[i].size.width = metrics->master_region.size.width;
        metrics->row_regions[i].size.height = row_height;
    }

    metrics->body_line_count = 1;
}

static uint8_t egui_view_master_detail_hit_index(egui_view_master_detail_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_master_detail_metrics_t metrics;
    uint8_t i;

    egui_view_master_detail_get_metrics(local, self, &metrics);
    for (i = 0; i < local->item_count; i++)
    {
        if (egui_region_pt_in_rect(&metrics.row_regions[i], x, y))
        {
            return i;
        }
    }

    return EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS;
}

static void egui_view_master_detail_set_current_index_inner(egui_view_t *self, uint8_t item_index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_master_detail_t);

    if (local->items == NULL || local->item_count == 0 || item_index >= local->item_count)
    {
        return;
    }
    if (local->current_index == item_index)
    {
        return;
    }

    local->current_index = item_index;
    if (notify && local->on_selection_changed)
    {
        local->on_selection_changed(self, item_index);
    }
    egui_view_invalidate(self);
}

void egui_view_master_detail_set_items(egui_view_t *self, const egui_view_master_detail_item_t *items, uint8_t item_count)
{
    EGUI_LOCAL_INIT(egui_view_master_detail_t);

    local->items = items;
    local->item_count = egui_view_master_detail_clamp_item_count(item_count);
    if (local->current_index >= local->item_count)
    {
        local->current_index = 0;
    }
    local->pressed_index = EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS;
    egui_view_invalidate(self);
}

uint8_t egui_view_master_detail_get_item_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_master_detail_t);
    return local->item_count;
}

void egui_view_master_detail_set_current_index(egui_view_t *self, uint8_t item_index)
{
    egui_view_master_detail_set_current_index_inner(self, item_index, 1);
}

uint8_t egui_view_master_detail_get_current_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_master_detail_t);
    return local->current_index;
}

void egui_view_master_detail_set_on_selection_changed_listener(egui_view_t *self, egui_view_on_master_detail_selection_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_master_detail_t);
    local->on_selection_changed = listener;
}

void egui_view_master_detail_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_master_detail_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_master_detail_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_master_detail_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_master_detail_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_master_detail_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_master_detail_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_master_detail_t);
    local->read_only_mode = read_only_mode ? 1 : 0;
    local->pressed_index = EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS;
    egui_view_set_pressed(self, false);
    egui_view_invalidate(self);
}

void egui_view_master_detail_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t section_color,
                                         egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color,
                                         egui_color_t warning_color, egui_color_t neutral_color)
{
    EGUI_LOCAL_INIT(egui_view_master_detail_t);

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

static void egui_view_master_detail_draw_row(egui_view_t *self, egui_view_master_detail_t *local, const egui_view_master_detail_item_t *item,
                                             const egui_region_t *region, uint8_t selected, uint8_t pressed)
{
    egui_region_t text_region;
    egui_color_t tone_color = egui_view_master_detail_tone_color(local, item->tone);
    egui_color_t row_fill = egui_rgb_mix(local->surface_color, tone_color, selected ? 13 : 7);
    egui_color_t row_border = egui_rgb_mix(local->border_color, tone_color, selected ? 26 : 14);
    egui_color_t glyph_fill = egui_rgb_mix(local->surface_color, tone_color, selected ? 22 : 12);
    egui_color_t glyph_text = selected ? tone_color : egui_rgb_mix(local->text_color, tone_color, 10);
    egui_color_t title_color = selected ? egui_rgb_mix(local->text_color, tone_color, 8) : local->text_color;
    egui_dim_t glyph_size = local->compact_mode ? 10 : 12;
    egui_dim_t inner_x = region->location.x + (local->compact_mode ? 4 : 6);
    egui_dim_t glyph_y = region->location.y + (region->size.height - glyph_size) / 2;
    egui_dim_t title_x = inner_x + glyph_size + (local->compact_mode ? 4 : 6);
    egui_dim_t indicator_w = local->compact_mode ? 2 : 3;

    if (item->emphasized)
    {
        row_fill = egui_rgb_mix(row_fill, tone_color, 8);
        row_border = egui_rgb_mix(row_border, tone_color, 8);
    }
    if (pressed)
    {
        row_fill = egui_rgb_mix(row_fill, tone_color, 14);
    }
    if (local->read_only_mode)
    {
        row_fill = egui_rgb_mix(row_fill, local->surface_color, 26);
        row_border = egui_rgb_mix(row_border, local->muted_text_color, 20);
        glyph_fill = egui_rgb_mix(glyph_fill, local->surface_color, 30);
        glyph_text = egui_rgb_mix(glyph_text, local->muted_text_color, 26);
        title_color = egui_rgb_mix(title_color, local->muted_text_color, 14);
    }
    if (!egui_view_get_enable(self))
    {
        row_fill = egui_view_master_detail_mix_disabled(row_fill);
        row_border = egui_view_master_detail_mix_disabled(row_border);
        glyph_fill = egui_view_master_detail_mix_disabled(glyph_fill);
        glyph_text = egui_view_master_detail_mix_disabled(glyph_text);
        title_color = egui_view_master_detail_mix_disabled(title_color);
    }

    egui_canvas_draw_round_rectangle_fill(region->location.x, region->location.y, region->size.width, region->size.height, local->compact_mode ? 6 : 7,
                                          row_fill, egui_color_alpha_mix(self->alpha, 92));
    egui_canvas_draw_round_rectangle(region->location.x, region->location.y, region->size.width, region->size.height, local->compact_mode ? 6 : 7, 1,
                                     row_border, egui_color_alpha_mix(self->alpha, selected ? 42 : 28));

    if (selected)
    {
        egui_canvas_draw_round_rectangle_fill(region->location.x + 1, region->location.y + 2, indicator_w, region->size.height - 4, 1, tone_color,
                                              egui_color_alpha_mix(self->alpha, 100));
    }

    egui_canvas_draw_round_rectangle_fill(inner_x, glyph_y, glyph_size, glyph_size, local->compact_mode ? 3 : 4, glyph_fill,
                                          egui_color_alpha_mix(self->alpha, 96));
    text_region.location.x = inner_x;
    text_region.location.y = glyph_y;
    text_region.size.width = glyph_size;
    text_region.size.height = glyph_size;
    egui_view_master_detail_draw_text(local->meta_font, self, item->master_glyph, &text_region, EGUI_ALIGN_CENTER, glyph_text);

    if (local->compact_mode)
    {
        return;
    }

    text_region.location.x = title_x;
    text_region.location.y = region->location.y;
    text_region.size.width = region->size.width - (title_x - region->location.x) - 4;
    text_region.size.height = region->size.height;
    egui_view_master_detail_draw_text(local->font, self, item->master_title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color);
}

static void egui_view_master_detail_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_master_detail_t);
    const egui_view_master_detail_item_t *item = egui_view_master_detail_get_current_item(local);
    egui_view_master_detail_metrics_t metrics;
    egui_region_t text_region;
    egui_color_t card_fill;
    egui_color_t card_border;
    egui_color_t divider_color;
    egui_color_t detail_fill;
    egui_color_t detail_border;
    egui_color_t eyebrow_color;
    egui_color_t title_color;
    egui_color_t meta_color;
    egui_color_t body_color;
    egui_color_t footer_fill;
    egui_color_t footer_border;
    egui_color_t footer_text;
    egui_color_t tone_color;
    egui_dim_t card_radius = local->compact_mode ? EGUI_VIEW_MASTER_DETAIL_COMPACT_RADIUS : EGUI_VIEW_MASTER_DETAIL_STANDARD_RADIUS;
    egui_dim_t detail_radius = local->compact_mode ? EGUI_VIEW_MASTER_DETAIL_COMPACT_DETAIL_RADIUS : EGUI_VIEW_MASTER_DETAIL_STANDARD_DETAIL_RADIUS;
    egui_dim_t divider_margin = local->compact_mode ? EGUI_VIEW_MASTER_DETAIL_COMPACT_DIVIDER_MARGIN : EGUI_VIEW_MASTER_DETAIL_STANDARD_DIVIDER_MARGIN;
    egui_dim_t header_h = local->compact_mode ? EGUI_VIEW_MASTER_DETAIL_COMPACT_HEADER_HEIGHT : EGUI_VIEW_MASTER_DETAIL_STANDARD_HEADER_HEIGHT;
    egui_dim_t header_gap = local->compact_mode ? EGUI_VIEW_MASTER_DETAIL_COMPACT_HEADER_GAP : EGUI_VIEW_MASTER_DETAIL_STANDARD_HEADER_GAP;
    egui_dim_t meta_h = local->compact_mode ? EGUI_VIEW_MASTER_DETAIL_COMPACT_META_HEIGHT : EGUI_VIEW_MASTER_DETAIL_STANDARD_META_HEIGHT;
    egui_dim_t meta_gap = local->compact_mode ? EGUI_VIEW_MASTER_DETAIL_COMPACT_META_GAP : EGUI_VIEW_MASTER_DETAIL_STANDARD_META_GAP;
    egui_dim_t body_h = local->compact_mode ? EGUI_VIEW_MASTER_DETAIL_COMPACT_BODY_HEIGHT : EGUI_VIEW_MASTER_DETAIL_STANDARD_BODY_HEIGHT;
    egui_dim_t body_gap = EGUI_VIEW_MASTER_DETAIL_STANDARD_BODY_GAP;
    egui_dim_t footer_h = local->compact_mode ? EGUI_VIEW_MASTER_DETAIL_COMPACT_FOOTER_HEIGHT : EGUI_VIEW_MASTER_DETAIL_STANDARD_FOOTER_HEIGHT;
    egui_dim_t footer_w;
    egui_dim_t cursor_y;
    egui_dim_t divider_x;
    uint8_t i;

    if (item == NULL)
    {
        return;
    }

    egui_view_master_detail_get_metrics(local, self, &metrics);
    if (metrics.content_region.size.width <= 0 || metrics.content_region.size.height <= 0)
    {
        return;
    }

    tone_color = egui_view_master_detail_tone_color(local, item->tone);
    card_fill = egui_rgb_mix(local->surface_color, tone_color, local->compact_mode ? 4 : 6);
    card_border = egui_rgb_mix(local->border_color, tone_color, local->compact_mode ? 14 : 16);
    divider_color = egui_rgb_mix(local->border_color, tone_color, local->compact_mode ? 18 : 20);
    detail_fill = egui_rgb_mix(local->section_color, tone_color, item->emphasized ? 16 : 12);
    detail_border = egui_rgb_mix(local->border_color, tone_color, item->emphasized ? 26 : 20);
    eyebrow_color = tone_color;
    title_color = local->text_color;
    meta_color = egui_rgb_mix(local->muted_text_color, tone_color, 14);
    body_color = egui_rgb_mix(local->text_color, local->muted_text_color, local->compact_mode ? 28 : 18);
    footer_fill = egui_rgb_mix(local->surface_color, tone_color, 18);
    footer_border = egui_rgb_mix(local->border_color, tone_color, 24);
    footer_text = tone_color;

    if (local->read_only_mode)
    {
        detail_fill = egui_rgb_mix(detail_fill, local->surface_color, 22);
        detail_border = egui_rgb_mix(detail_border, local->muted_text_color, 18);
        eyebrow_color = egui_rgb_mix(eyebrow_color, local->muted_text_color, 24);
        title_color = egui_rgb_mix(title_color, local->muted_text_color, 12);
        meta_color = egui_rgb_mix(meta_color, local->muted_text_color, 16);
        body_color = egui_rgb_mix(body_color, local->muted_text_color, 16);
        footer_fill = egui_rgb_mix(footer_fill, local->surface_color, 24);
        footer_border = egui_rgb_mix(footer_border, local->muted_text_color, 20);
        footer_text = egui_rgb_mix(footer_text, local->muted_text_color, 28);
    }
    if (!egui_view_get_enable(self))
    {
        card_fill = egui_view_master_detail_mix_disabled(card_fill);
        card_border = egui_view_master_detail_mix_disabled(card_border);
        divider_color = egui_view_master_detail_mix_disabled(divider_color);
        detail_fill = egui_view_master_detail_mix_disabled(detail_fill);
        detail_border = egui_view_master_detail_mix_disabled(detail_border);
        eyebrow_color = egui_view_master_detail_mix_disabled(eyebrow_color);
        title_color = egui_view_master_detail_mix_disabled(title_color);
        meta_color = egui_view_master_detail_mix_disabled(meta_color);
        body_color = egui_view_master_detail_mix_disabled(body_color);
        footer_fill = egui_view_master_detail_mix_disabled(footer_fill);
        footer_border = egui_view_master_detail_mix_disabled(footer_border);
        footer_text = egui_view_master_detail_mix_disabled(footer_text);
    }

    egui_canvas_draw_round_rectangle_fill(metrics.content_region.location.x - 2, metrics.content_region.location.y - 2, metrics.content_region.size.width + 4,
                                          metrics.content_region.size.height + 4, card_radius, card_fill, egui_color_alpha_mix(self->alpha, 94));
    egui_canvas_draw_round_rectangle(metrics.content_region.location.x - 2, metrics.content_region.location.y - 2, metrics.content_region.size.width + 4,
                                     metrics.content_region.size.height + 4, card_radius, 1, card_border, egui_color_alpha_mix(self->alpha, 56));

    divider_x = metrics.master_region.location.x + metrics.master_region.size.width +
                (metrics.detail_region.location.x - metrics.master_region.location.x - metrics.master_region.size.width) / 2;
    egui_canvas_draw_line(divider_x, metrics.content_region.location.y + divider_margin, divider_x,
                          metrics.content_region.location.y + metrics.content_region.size.height - divider_margin, 1, divider_color,
                          egui_color_alpha_mix(self->alpha, 34));

    for (i = 0; i < local->item_count; i++)
    {
        egui_view_master_detail_draw_row(self, local, &local->items[i], &metrics.row_regions[i], i == local->current_index, i == local->pressed_index);
    }

    egui_canvas_draw_round_rectangle_fill(metrics.detail_region.location.x, metrics.detail_region.location.y, metrics.detail_region.size.width,
                                          metrics.detail_region.size.height, detail_radius, detail_fill, egui_color_alpha_mix(self->alpha, 96));
    egui_canvas_draw_round_rectangle(metrics.detail_region.location.x, metrics.detail_region.location.y, metrics.detail_region.size.width,
                                     metrics.detail_region.size.height, detail_radius, 1, detail_border, egui_color_alpha_mix(self->alpha, 34));

    cursor_y = metrics.detail_region.location.y + (local->compact_mode ? 6 : 6);
    text_region.location.x = metrics.detail_region.location.x + (local->compact_mode ? 5 : 7);
    text_region.location.y = cursor_y;
    text_region.size.width = metrics.detail_region.size.width - (local->compact_mode ? 10 : 14);

    if (local->compact_mode)
    {
        text_region.size.height = 9;
        egui_view_master_detail_draw_text(local->font, self, item->detail_title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color);

        cursor_y += 10;
        text_region.location.y = cursor_y;
        text_region.size.height = meta_h;
        egui_view_master_detail_draw_text(local->meta_font, self, item->detail_meta, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, meta_color);

        cursor_y += meta_h + 1;
        text_region.location.y = cursor_y;
        text_region.size.height = body_h;
        egui_view_master_detail_draw_text(local->meta_font, self, item->detail_body_primary, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, body_color);
    }
    else
    {
        text_region.size.height = header_h;
        egui_view_master_detail_draw_text(local->meta_font, self, item->detail_eyebrow, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, eyebrow_color);

        cursor_y += header_h + header_gap;
        text_region.location.y = cursor_y;
        text_region.size.height = 12;
        egui_view_master_detail_draw_text(local->font, self, item->detail_title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color);

        cursor_y += text_region.size.height + meta_gap;
        text_region.location.y = cursor_y;
        text_region.size.height = meta_h;
        egui_view_master_detail_draw_text(local->meta_font, self, item->detail_meta, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, meta_color);

        cursor_y += meta_h + meta_gap;
        text_region.location.y = cursor_y;
        text_region.size.height = body_h;
        egui_view_master_detail_draw_text(local->meta_font, self, item->detail_body_primary, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, body_color);

        if (metrics.body_line_count > 1)
        {
            cursor_y += body_h + body_gap;
            text_region.location.y = cursor_y;
            text_region.size.height = body_h;
            egui_view_master_detail_draw_text(local->meta_font, self, item->detail_body_secondary, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                                              body_color);
        }
    }

    footer_w =
            egui_view_master_detail_footer_width(item->detail_footer, local->compact_mode, metrics.detail_region.size.width - (local->compact_mode ? 12 : 14));
    egui_canvas_draw_round_rectangle_fill(metrics.detail_region.location.x + (local->compact_mode ? 6 : 7),
                                          metrics.detail_region.location.y + metrics.detail_region.size.height - footer_h - (local->compact_mode ? 5 : 6),
                                          footer_w, footer_h, footer_h / 2, footer_fill, egui_color_alpha_mix(self->alpha, 98));
    egui_canvas_draw_round_rectangle(metrics.detail_region.location.x + (local->compact_mode ? 6 : 7),
                                     metrics.detail_region.location.y + metrics.detail_region.size.height - footer_h - (local->compact_mode ? 5 : 6), footer_w,
                                     footer_h, footer_h / 2, 1, footer_border, egui_color_alpha_mix(self->alpha, 34));

    text_region.location.x = metrics.detail_region.location.x + (local->compact_mode ? 6 : 7);
    text_region.location.y = metrics.detail_region.location.y + metrics.detail_region.size.height - footer_h - (local->compact_mode ? 5 : 6);
    text_region.size.width = footer_w;
    text_region.size.height = footer_h;
    egui_view_master_detail_draw_text(local->meta_font, self, item->detail_footer, &text_region, EGUI_ALIGN_CENTER, footer_text);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_master_detail_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_master_detail_t);
    uint8_t hit_index;

    if (local->items == NULL || local->item_count == 0 || !egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_index = egui_view_master_detail_hit_index(local, self, event->location.x, event->location.y);
        if (hit_index >= local->item_count)
        {
            return 0;
        }
        local->pressed_index = hit_index;
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_index = egui_view_master_detail_hit_index(local, self, event->location.x, event->location.y);
        if (local->pressed_index == hit_index && hit_index < local->item_count)
        {
            egui_view_master_detail_set_current_index_inner(self, hit_index, 1);
        }
        local->pressed_index = EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_index < local->item_count;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_index = EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_master_detail_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_master_detail_t);
    uint8_t next_index;

    if (local->items == NULL || local->item_count == 0 || !egui_view_get_enable(self) || local->read_only_mode || event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        return 0;
    }

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_UP:
    case EGUI_KEY_CODE_LEFT:
        next_index = local->current_index > 0 ? (local->current_index - 1) : 0;
        egui_view_master_detail_set_current_index_inner(self, next_index, 1);
        return 1;
    case EGUI_KEY_CODE_DOWN:
    case EGUI_KEY_CODE_RIGHT:
        next_index = local->current_index + 1 < local->item_count ? (local->current_index + 1) : (local->item_count - 1);
        egui_view_master_detail_set_current_index_inner(self, next_index, 1);
        return 1;
    case EGUI_KEY_CODE_HOME:
        egui_view_master_detail_set_current_index_inner(self, 0, 1);
        return 1;
    case EGUI_KEY_CODE_END:
        egui_view_master_detail_set_current_index_inner(self, local->item_count - 1, 1);
        return 1;
    case EGUI_KEY_CODE_TAB:
        next_index = local->current_index + 1;
        if (next_index >= local->item_count)
        {
            next_index = 0;
        }
        egui_view_master_detail_set_current_index_inner(self, next_index, 1);
        return 1;
    default:
        return egui_view_on_key_event(self, event);
    }
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_master_detail_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_master_detail_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_master_detail_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_master_detail_on_key_event,
#endif
};

void egui_view_master_detail_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_master_detail_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_master_detail_t);
    egui_view_set_padding_all(self, 2);

    local->items = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->on_selection_changed = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD6DEE7);
    local->section_color = EGUI_COLOR_HEX(0xF6F8FA);
    local->text_color = EGUI_COLOR_HEX(0x1E2933);
    local->muted_text_color = EGUI_COLOR_HEX(0x708090);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->success_color = EGUI_COLOR_HEX(0x178454);
    local->warning_color = EGUI_COLOR_HEX(0xB87A16);
    local->neutral_color = EGUI_COLOR_HEX(0x7A8795);
    local->item_count = 0;
    local->current_index = 0;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    local->pressed_index = EGUI_VIEW_MASTER_DETAIL_MAX_ITEMS;

    egui_view_set_view_name(self, "egui_view_master_detail");
}
