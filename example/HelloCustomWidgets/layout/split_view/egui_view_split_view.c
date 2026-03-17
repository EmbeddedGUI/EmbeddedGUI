#include "egui_view_split_view.h"

#define SV_STD_PAD_X         8
#define SV_STD_PAD_Y         7
#define SV_STD_RADIUS        10
#define SV_STD_HEADER_H      18
#define SV_STD_EXPANDED_W    78
#define SV_STD_COLLAPSED_W   30
#define SV_STD_GAP           6
#define SV_STD_TOGGLE_W      14
#define SV_STD_TOGGLE_H      12
#define SV_STD_ROW_H         15
#define SV_STD_ROW_GAP       4
#define SV_STD_GLYPH         10
#define SV_STD_DETAIL_RADIUS 8
#define SV_STD_DETAIL_PAD_X  7
#define SV_STD_DETAIL_PAD_Y  6
#define SV_STD_TITLE_H       12
#define SV_STD_META_H        9
#define SV_STD_BODY_H        9
#define SV_STD_BODY_GAP      3
#define SV_STD_FOOTER_H      13

#define SV_COMPACT_PAD_X         6
#define SV_COMPACT_PAD_Y         5
#define SV_COMPACT_RADIUS        8
#define SV_COMPACT_HEADER_H      14
#define SV_COMPACT_EXPANDED_W    54
#define SV_COMPACT_COLLAPSED_W   26
#define SV_COMPACT_GAP           4
#define SV_COMPACT_TOGGLE_W      12
#define SV_COMPACT_TOGGLE_H      10
#define SV_COMPACT_ROW_H         12
#define SV_COMPACT_ROW_GAP       3
#define SV_COMPACT_GLYPH         8
#define SV_COMPACT_DETAIL_RADIUS 6
#define SV_COMPACT_DETAIL_PAD_X  5
#define SV_COMPACT_DETAIL_PAD_Y  4
#define SV_COMPACT_TITLE_H       9
#define SV_COMPACT_META_H        8
#define SV_COMPACT_BODY_H        8
#define SV_COMPACT_BODY_GAP      2
#define SV_COMPACT_FOOTER_H      10

typedef struct egui_view_split_view_metrics egui_view_split_view_metrics_t;
struct egui_view_split_view_metrics
{
    egui_region_t content;
    egui_region_t pane;
    egui_region_t toggle;
    egui_region_t title;
    egui_region_t detail;
    egui_region_t rows[EGUI_VIEW_SPLIT_VIEW_MAX_ITEMS];
    uint8_t show_secondary_body;
    uint8_t show_meta;
    uint8_t draw_detail;
};

static uint8_t sv_clamp_count(uint8_t count)
{
    return count > EGUI_VIEW_SPLIT_VIEW_MAX_ITEMS ? EGUI_VIEW_SPLIT_VIEW_MAX_ITEMS : count;
}

static const egui_view_split_view_item_t *sv_get_current_item(egui_view_split_view_t *local)
{
    if (local->items == NULL || local->item_count == 0 || local->current_index >= local->item_count)
    {
        return NULL;
    }
    return &local->items[local->current_index];
}

static uint8_t sv_text_len(const char *text)
{
    uint8_t len = 0;

    if (text == NULL)
    {
        return 0;
    }
    while (text[len] != '\0')
    {
        len++;
    }
    return len;
}

static egui_color_t sv_tone_color(egui_view_split_view_t *local, uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_SPLIT_VIEW_TONE_SUCCESS:
        return local->success_color;
    case EGUI_VIEW_SPLIT_VIEW_TONE_WARNING:
        return local->warning_color;
    case EGUI_VIEW_SPLIT_VIEW_TONE_NEUTRAL:
        return local->neutral_color;
    default:
        return local->accent_color;
    }
}

static egui_color_t sv_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 68);
}

static void sv_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align, egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (font == NULL || text == NULL || text[0] == '\0' || region->size.width <= 0 || region->size.height <= 0)
    {
        return;
    }
    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static egui_dim_t sv_meta_width(const char *text, uint8_t compact_mode)
{
    egui_dim_t width;

    if (text == NULL || text[0] == '\0')
    {
        return 0;
    }
    width = (compact_mode ? 12 : 16) + sv_text_len(text) * (compact_mode ? 4 : 5);
    return width > (compact_mode ? 20 : 28) ? (compact_mode ? 20 : 28) : width;
}

static egui_dim_t sv_footer_width(const char *text, uint8_t compact_mode, egui_dim_t max_width)
{
    egui_dim_t width = (compact_mode ? 18 : 22) + sv_text_len(text) * (compact_mode ? 4 : 5);
    return width > max_width ? max_width : width;
}

static void sv_get_metrics(egui_view_split_view_t *local, egui_view_t *self, egui_view_split_view_metrics_t *metrics)
{
    egui_region_t work;
    egui_dim_t pad_x = local->compact_mode ? SV_COMPACT_PAD_X : SV_STD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? SV_COMPACT_PAD_Y : SV_STD_PAD_Y;
    egui_dim_t header_h = local->compact_mode ? SV_COMPACT_HEADER_H : SV_STD_HEADER_H;
    egui_dim_t pane_w = local->pane_expanded ? (local->compact_mode ? SV_COMPACT_EXPANDED_W : SV_STD_EXPANDED_W)
                                             : (local->compact_mode ? SV_COMPACT_COLLAPSED_W : SV_STD_COLLAPSED_W);
    egui_dim_t gap = local->compact_mode ? SV_COMPACT_GAP : SV_STD_GAP;
    egui_dim_t toggle_w = local->compact_mode ? SV_COMPACT_TOGGLE_W : SV_STD_TOGGLE_W;
    egui_dim_t toggle_h = local->compact_mode ? SV_COMPACT_TOGGLE_H : SV_STD_TOGGLE_H;
    egui_dim_t row_h = local->compact_mode ? SV_COMPACT_ROW_H : SV_STD_ROW_H;
    egui_dim_t row_gap = local->compact_mode ? SV_COMPACT_ROW_GAP : SV_STD_ROW_GAP;
    egui_dim_t total_h;
    egui_dim_t available_h;
    egui_dim_t start_y;
    egui_dim_t min_detail = local->compact_mode ? 34 : 48;
    uint8_t i;

    egui_view_get_work_region(self, &work);
    metrics->content.location.x = work.location.x + pad_x;
    metrics->content.location.y = work.location.y + pad_y;
    metrics->content.size.width = work.size.width - pad_x * 2;
    metrics->content.size.height = work.size.height - pad_y * 2;
    metrics->show_secondary_body = 0;
    metrics->show_meta = 0;
    metrics->draw_detail = 0;

    for (i = 0; i < EGUI_VIEW_SPLIT_VIEW_MAX_ITEMS; i++)
    {
        metrics->rows[i].location.x = 0;
        metrics->rows[i].location.y = 0;
        metrics->rows[i].size.width = 0;
        metrics->rows[i].size.height = 0;
    }

    if (metrics->content.size.width <= 0 || metrics->content.size.height <= 0)
    {
        return;
    }

    if (metrics->content.size.width < pane_w + gap + min_detail)
    {
        pane_w = metrics->content.size.width - gap - min_detail;
        if (pane_w < (local->compact_mode ? 22 : 24))
        {
            pane_w = local->compact_mode ? 22 : 24;
            gap = 2;
        }
    }

    metrics->pane.location.x = metrics->content.location.x;
    metrics->pane.location.y = metrics->content.location.y;
    metrics->pane.size.width = pane_w;
    metrics->pane.size.height = metrics->content.size.height;
    metrics->toggle.location.x = metrics->pane.location.x + 3;
    metrics->toggle.location.y = metrics->pane.location.y + 2;
    metrics->toggle.size.width = toggle_w;
    metrics->toggle.size.height = toggle_h;
    metrics->title.location.x = metrics->toggle.location.x + toggle_w + 4;
    metrics->title.location.y = metrics->toggle.location.y;
    metrics->title.size.width = metrics->pane.location.x + metrics->pane.size.width - metrics->title.location.x - 3;
    metrics->title.size.height = toggle_h;
    metrics->detail.location.x = metrics->pane.location.x + metrics->pane.size.width + gap;
    metrics->detail.location.y = metrics->content.location.y;
    metrics->detail.size.width = metrics->content.size.width - metrics->pane.size.width - gap;
    metrics->detail.size.height = metrics->content.size.height;
    metrics->draw_detail = metrics->detail.size.width > 12 ? 1 : 0;

    available_h = metrics->pane.size.height - header_h - 3;
    total_h = local->item_count > 0 ? (local->item_count * row_h + (local->item_count - 1) * row_gap) : 0;
    if (total_h > available_h)
    {
        row_gap = 1;
        total_h = local->item_count > 0 ? (local->item_count * row_h + (local->item_count - 1) * row_gap) : 0;
    }
    if (total_h > available_h)
    {
        row_h = local->compact_mode ? 10 : 12;
        total_h = local->item_count > 0 ? (local->item_count * row_h + (local->item_count - 1) * row_gap) : 0;
    }

    start_y = metrics->pane.location.y + header_h + 2;
    if ((!local->pane_expanded || local->compact_mode) && available_h > total_h)
    {
        start_y += (available_h - total_h) / 2;
    }

    for (i = 0; i < local->item_count; i++)
    {
        metrics->rows[i].location.x = metrics->pane.location.x + 2;
        metrics->rows[i].location.y = start_y + i * (row_h + row_gap);
        metrics->rows[i].size.width = metrics->pane.size.width - 4;
        metrics->rows[i].size.height = row_h;
    }

    metrics->show_secondary_body =
            (!local->compact_mode && local->pane_expanded && metrics->detail.size.width >= 70 && metrics->detail.size.height >= 58) ? 1 : 0;
    metrics->show_meta = metrics->detail.size.width >= (local->compact_mode ? 38 : 52) ? 1 : 0;
}

static uint8_t sv_hit_index(egui_view_split_view_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_split_view_metrics_t metrics;
    uint8_t i;

    sv_get_metrics(local, self, &metrics);
    for (i = 0; i < local->item_count; i++)
    {
        if (egui_region_pt_in_rect(&metrics.rows[i], x, y))
        {
            return i;
        }
    }
    return EGUI_VIEW_SPLIT_VIEW_INDEX_NONE;
}

static uint8_t sv_hit_toggle(egui_view_split_view_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_split_view_metrics_t metrics;

    sv_get_metrics(local, self, &metrics);
    return egui_region_pt_in_rect(&metrics.toggle, x, y) ? 1 : 0;
}

static void sv_set_current_index_inner(egui_view_t *self, uint8_t index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_split_view_t);

    if (local->items == NULL || local->item_count == 0 || index >= local->item_count || local->current_index == index)
    {
        return;
    }
    local->current_index = index;
    if (notify && local->on_selection_changed)
    {
        local->on_selection_changed(self, index);
    }
    egui_view_invalidate(self);
}

static void sv_set_pane_expanded_inner(egui_view_t *self, uint8_t expanded, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_split_view_t);
    expanded = expanded ? 1 : 0;

    if (local->pane_expanded == expanded)
    {
        return;
    }
    local->pane_expanded = expanded;
    if (notify && local->on_pane_state_changed)
    {
        local->on_pane_state_changed(self, expanded);
    }
    egui_view_invalidate(self);
}
void egui_view_split_view_set_items(egui_view_t *self, const egui_view_split_view_item_t *items, uint8_t item_count)
{
    EGUI_LOCAL_INIT(egui_view_split_view_t);

    local->items = items;
    local->item_count = sv_clamp_count(item_count);
    if (local->current_index >= local->item_count)
    {
        local->current_index = 0;
    }
    local->pressed_index = EGUI_VIEW_SPLIT_VIEW_INDEX_NONE;
    local->pressed_toggle = 0;
    egui_view_invalidate(self);
}

uint8_t egui_view_split_view_get_item_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_split_view_t);
    return local->item_count;
}

void egui_view_split_view_set_current_index(egui_view_t *self, uint8_t item_index)
{
    sv_set_current_index_inner(self, item_index, 1);
}

uint8_t egui_view_split_view_get_current_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_split_view_t);
    return local->current_index;
}

void egui_view_split_view_set_on_selection_changed_listener(egui_view_t *self, egui_view_on_split_view_selection_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_split_view_t);
    local->on_selection_changed = listener;
}

void egui_view_split_view_set_on_pane_state_changed_listener(egui_view_t *self, egui_view_on_split_view_pane_state_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_split_view_t);
    local->on_pane_state_changed = listener;
}

void egui_view_split_view_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_split_view_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_split_view_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_split_view_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_split_view_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_split_view_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_split_view_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_split_view_t);
    local->read_only_mode = read_only_mode ? 1 : 0;
    local->pressed_index = EGUI_VIEW_SPLIT_VIEW_INDEX_NONE;
    local->pressed_toggle = 0;
    egui_view_set_pressed(self, false);
    egui_view_invalidate(self);
}

void egui_view_split_view_set_pane_expanded(egui_view_t *self, uint8_t pane_expanded)
{
    sv_set_pane_expanded_inner(self, pane_expanded, 1);
}

uint8_t egui_view_split_view_get_pane_expanded(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_split_view_t);
    return local->pane_expanded;
}

void egui_view_split_view_toggle_pane(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_split_view_t);
    sv_set_pane_expanded_inner(self, local->pane_expanded ? 0 : 1, 1);
}

void egui_view_split_view_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t section_color,
                                      egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color,
                                      egui_color_t warning_color, egui_color_t neutral_color)
{
    EGUI_LOCAL_INIT(egui_view_split_view_t);

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

static void sv_draw_toggle(egui_view_t *self, egui_view_split_view_t *local, const egui_region_t *region, egui_color_t tone_color, uint8_t pressed)
{
    egui_color_t fill_color = egui_rgb_mix(local->surface_color, tone_color, local->pane_expanded ? 10 : 14);
    egui_color_t border_color = egui_rgb_mix(local->border_color, tone_color, local->pane_expanded ? 20 : 26);
    egui_color_t icon_color = local->pane_expanded ? tone_color : egui_rgb_mix(local->text_color, tone_color, 14);
    egui_dim_t cx = region->location.x + region->size.width / 2;
    egui_dim_t cy = region->location.y + region->size.height / 2;

    if (pressed)
    {
        fill_color = egui_rgb_mix(fill_color, tone_color, 12);
    }
    if (local->read_only_mode)
    {
        fill_color = egui_rgb_mix(fill_color, local->surface_color, 28);
        border_color = egui_rgb_mix(border_color, local->muted_text_color, 24);
        icon_color = egui_rgb_mix(icon_color, local->muted_text_color, 30);
    }
    if (!egui_view_get_enable(self))
    {
        fill_color = sv_mix_disabled(fill_color);
        border_color = sv_mix_disabled(border_color);
        icon_color = sv_mix_disabled(icon_color);
    }

    egui_canvas_draw_round_rectangle_fill(region->location.x, region->location.y, region->size.width, region->size.height, region->size.height / 2, fill_color,
                                          egui_color_alpha_mix(self->alpha, 98));
    egui_canvas_draw_round_rectangle(region->location.x, region->location.y, region->size.width, region->size.height, region->size.height / 2, 1, border_color,
                                     egui_color_alpha_mix(self->alpha, 40));

    if (local->pane_expanded)
    {
        egui_canvas_draw_line(cx + 2, cy - 3, cx - 1, cy, 1, icon_color, egui_color_alpha_mix(self->alpha, 92));
        egui_canvas_draw_line(cx - 1, cy, cx + 2, cy + 3, 1, icon_color, egui_color_alpha_mix(self->alpha, 92));
    }
    else
    {
        egui_canvas_draw_line(cx - 2, cy - 3, cx + 1, cy, 1, icon_color, egui_color_alpha_mix(self->alpha, 92));
        egui_canvas_draw_line(cx + 1, cy, cx - 2, cy + 3, 1, icon_color, egui_color_alpha_mix(self->alpha, 92));
    }
}

static void sv_draw_row(egui_view_t *self, egui_view_split_view_t *local, const egui_view_split_view_item_t *item, const egui_region_t *region,
                        uint8_t selected, uint8_t pressed)
{
    egui_region_t text_region;
    egui_color_t tone_color = sv_tone_color(local, item->tone);
    egui_color_t row_fill = egui_rgb_mix(local->surface_color, tone_color, selected ? 14 : (local->pane_expanded ? 7 : 11));
    egui_color_t row_border = egui_rgb_mix(local->border_color, tone_color, selected ? 26 : (local->pane_expanded ? 14 : 20));
    egui_color_t glyph_fill = egui_rgb_mix(local->surface_color, tone_color, selected ? 18 : 10);
    egui_color_t glyph_text = selected ? tone_color : egui_rgb_mix(local->text_color, tone_color, 12);
    egui_color_t title_color = selected ? egui_rgb_mix(local->text_color, tone_color, 8) : local->text_color;
    egui_color_t meta_fill = egui_rgb_mix(local->surface_color, tone_color, selected ? 16 : 10);
    egui_color_t meta_border = egui_rgb_mix(local->border_color, tone_color, selected ? 24 : 16);
    egui_color_t meta_color = selected ? tone_color : egui_rgb_mix(local->muted_text_color, tone_color, 18);
    egui_dim_t glyph_size = local->compact_mode ? SV_COMPACT_GLYPH : SV_STD_GLYPH;
    egui_dim_t radius = local->compact_mode ? 5 : 6;
    egui_dim_t meta_w = 0;
    egui_dim_t meta_h = local->compact_mode ? 8 : 10;
    egui_dim_t glyph_x;
    egui_dim_t glyph_y;
    egui_dim_t title_x;
    egui_dim_t value_x;

    if (item->emphasized)
    {
        row_fill = egui_rgb_mix(row_fill, tone_color, 8);
        row_border = egui_rgb_mix(row_border, tone_color, 8);
    }
    if (pressed)
    {
        row_fill = egui_rgb_mix(row_fill, tone_color, 16);
    }
    if (local->read_only_mode)
    {
        row_fill = egui_rgb_mix(row_fill, local->surface_color, 24);
        row_border = egui_rgb_mix(row_border, local->muted_text_color, 20);
        glyph_fill = egui_rgb_mix(glyph_fill, local->surface_color, 28);
        glyph_text = egui_rgb_mix(glyph_text, local->muted_text_color, 28);
        title_color = egui_rgb_mix(title_color, local->muted_text_color, 16);
        meta_fill = egui_rgb_mix(meta_fill, local->surface_color, 26);
        meta_border = egui_rgb_mix(meta_border, local->muted_text_color, 22);
        meta_color = egui_rgb_mix(meta_color, local->muted_text_color, 28);
    }
    if (!egui_view_get_enable(self))
    {
        row_fill = sv_mix_disabled(row_fill);
        row_border = sv_mix_disabled(row_border);
        glyph_fill = sv_mix_disabled(glyph_fill);
        glyph_text = sv_mix_disabled(glyph_text);
        title_color = sv_mix_disabled(title_color);
        meta_fill = sv_mix_disabled(meta_fill);
        meta_border = sv_mix_disabled(meta_border);
        meta_color = sv_mix_disabled(meta_color);
    }

    egui_canvas_draw_round_rectangle_fill(region->location.x, region->location.y, region->size.width, region->size.height, radius, row_fill,
                                          egui_color_alpha_mix(self->alpha, local->pane_expanded ? 92 : 96));
    egui_canvas_draw_round_rectangle(region->location.x, region->location.y, region->size.width, region->size.height, radius, 1, row_border,
                                     egui_color_alpha_mix(self->alpha, selected ? 44 : 26));

    if (selected)
    {
        egui_canvas_draw_round_rectangle_fill(region->location.x + 1, region->location.y + 2, local->compact_mode ? 2 : 3, region->size.height - 4, 1,
                                              tone_color, egui_color_alpha_mix(self->alpha, 98));
    }

    if (local->pane_expanded)
    {
        glyph_x = region->location.x + (local->compact_mode ? 5 : 6);
        glyph_y = region->location.y + (region->size.height - glyph_size) / 2;
        title_x = glyph_x + glyph_size + (local->compact_mode ? 5 : 6);
        meta_w = sv_meta_width(item->meta, local->compact_mode);
        value_x = region->location.x + region->size.width - meta_w - (local->compact_mode ? 4 : 5);
        egui_canvas_draw_round_rectangle_fill(glyph_x, glyph_y, glyph_size, glyph_size, local->compact_mode ? 3 : 4, glyph_fill,
                                              egui_color_alpha_mix(self->alpha, 96));
        text_region.location.x = glyph_x;
        text_region.location.y = glyph_y;
        text_region.size.width = glyph_size;
        text_region.size.height = glyph_size;
        sv_draw_text(local->meta_font, self, item->glyph, &text_region, EGUI_ALIGN_CENTER, glyph_text);
        if (meta_w > 0 && value_x > title_x)
        {
            egui_canvas_draw_round_rectangle_fill(value_x, region->location.y + (region->size.height - meta_h) / 2, meta_w, meta_h, meta_h / 2, meta_fill,
                                                  egui_color_alpha_mix(self->alpha, 94));
            egui_canvas_draw_round_rectangle(value_x, region->location.y + (region->size.height - meta_h) / 2, meta_w, meta_h, meta_h / 2, 1, meta_border,
                                             egui_color_alpha_mix(self->alpha, 34));
            text_region.location.x = value_x;
            text_region.location.y = region->location.y + (region->size.height - meta_h) / 2;
            text_region.size.width = meta_w;
            text_region.size.height = meta_h;
            sv_draw_text(local->meta_font, self, item->meta, &text_region, EGUI_ALIGN_CENTER, meta_color);
        }
        else
        {
            value_x = region->location.x + region->size.width - 4;
        }
        text_region.location.x = title_x;
        text_region.location.y = region->location.y;
        text_region.size.width = value_x - title_x - 4;
        text_region.size.height = region->size.height;
        sv_draw_text(local->font, self, item->title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color);
    }
    else
    {
        glyph_size = local->compact_mode ? 9 : 11;
        glyph_x = region->location.x + (region->size.width - glyph_size) / 2;
        glyph_y = region->location.y + (region->size.height - glyph_size) / 2;
        egui_canvas_draw_round_rectangle_fill(glyph_x, glyph_y, glyph_size, glyph_size, local->compact_mode ? 3 : 4, glyph_fill,
                                              egui_color_alpha_mix(self->alpha, 98));
        text_region.location.x = glyph_x;
        text_region.location.y = glyph_y;
        text_region.size.width = glyph_size;
        text_region.size.height = glyph_size;
        sv_draw_text(local->meta_font, self, item->glyph, &text_region, EGUI_ALIGN_CENTER, glyph_text);
    }
}
static void egui_view_split_view_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_split_view_t);
    const egui_view_split_view_item_t *item = sv_get_current_item(local);
    egui_view_split_view_metrics_t metrics;
    egui_region_t text_region;
    egui_color_t tone_color;
    egui_color_t focus_color = EGUI_THEME_FOCUS;
    egui_color_t card_fill;
    egui_color_t card_border;
    egui_color_t pane_fill;
    egui_color_t pane_border;
    egui_color_t detail_fill;
    egui_color_t detail_border;
    egui_color_t pane_title_color;
    egui_color_t eyebrow_color;
    egui_color_t title_color;
    egui_color_t meta_color;
    egui_color_t body_color;
    egui_color_t footer_fill;
    egui_color_t footer_border;
    egui_color_t footer_text;
    egui_dim_t radius = local->compact_mode ? SV_COMPACT_RADIUS : SV_STD_RADIUS;
    egui_dim_t detail_radius = local->compact_mode ? SV_COMPACT_DETAIL_RADIUS : SV_STD_DETAIL_RADIUS;
    egui_dim_t detail_pad_x = local->compact_mode ? SV_COMPACT_DETAIL_PAD_X : SV_STD_DETAIL_PAD_X;
    egui_dim_t detail_pad_y = local->compact_mode ? SV_COMPACT_DETAIL_PAD_Y : SV_STD_DETAIL_PAD_Y;
    egui_dim_t title_h = local->compact_mode ? SV_COMPACT_TITLE_H : SV_STD_TITLE_H;
    egui_dim_t meta_h = local->compact_mode ? SV_COMPACT_META_H : SV_STD_META_H;
    egui_dim_t body_h = local->compact_mode ? SV_COMPACT_BODY_H : SV_STD_BODY_H;
    egui_dim_t body_gap = local->compact_mode ? SV_COMPACT_BODY_GAP : SV_STD_BODY_GAP;
    egui_dim_t footer_h = local->compact_mode ? SV_COMPACT_FOOTER_H : SV_STD_FOOTER_H;
    egui_dim_t footer_w;
    egui_dim_t cursor_y;
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    uint8_t is_focused = self->is_focused ? 1 : 0;
#else
    uint8_t is_focused = 0;
#endif
    uint8_t i;

    if (item == NULL)
    {
        return;
    }

    sv_get_metrics(local, self, &metrics);
    if (metrics.content.size.width <= 0 || metrics.content.size.height <= 0)
    {
        return;
    }

    tone_color = sv_tone_color(local, item->tone);
    card_fill = egui_rgb_mix(local->surface_color, tone_color, local->compact_mode ? 5 : 7);
    card_border = egui_rgb_mix(local->border_color, tone_color, local->compact_mode ? 14 : 16);
    pane_fill = egui_rgb_mix(local->surface_color, tone_color, local->pane_expanded ? 8 : 12);
    pane_border = egui_rgb_mix(local->border_color, tone_color, local->pane_expanded ? 16 : 22);
    detail_fill = egui_rgb_mix(local->section_color, tone_color, item->emphasized ? 16 : 11);
    detail_border = egui_rgb_mix(local->border_color, tone_color, item->emphasized ? 26 : 20);
    pane_title_color = egui_rgb_mix(local->muted_text_color, tone_color, 22);
    eyebrow_color = tone_color;
    title_color = local->text_color;
    meta_color = egui_rgb_mix(local->muted_text_color, tone_color, 16);
    body_color = egui_rgb_mix(local->text_color, local->muted_text_color, local->compact_mode ? 26 : 16);
    footer_fill = egui_rgb_mix(local->surface_color, tone_color, local->compact_mode ? 10 : 18);
    footer_border = egui_rgb_mix(local->border_color, tone_color, local->compact_mode ? 12 : 22);
    footer_text = local->compact_mode ? egui_rgb_mix(local->muted_text_color, tone_color, 22) : tone_color;

    if (local->read_only_mode)
    {
        focus_color = egui_rgb_mix(focus_color, local->muted_text_color, 24);
        card_fill = egui_rgb_mix(card_fill, local->surface_color, 18);
        card_border = egui_rgb_mix(card_border, local->muted_text_color, 20);
        pane_fill = egui_rgb_mix(pane_fill, local->surface_color, 24);
        pane_border = egui_rgb_mix(pane_border, local->muted_text_color, 20);
        detail_fill = egui_rgb_mix(detail_fill, local->surface_color, 24);
        detail_border = egui_rgb_mix(detail_border, local->muted_text_color, 20);
        pane_title_color = egui_rgb_mix(pane_title_color, local->muted_text_color, 20);
        eyebrow_color = egui_rgb_mix(eyebrow_color, local->muted_text_color, 30);
        title_color = egui_rgb_mix(title_color, local->muted_text_color, 18);
        meta_color = egui_rgb_mix(meta_color, local->muted_text_color, 22);
        body_color = egui_rgb_mix(body_color, local->muted_text_color, 20);
        footer_fill = egui_rgb_mix(footer_fill, local->surface_color, 24);
        footer_border = egui_rgb_mix(footer_border, local->muted_text_color, 20);
        footer_text = egui_rgb_mix(footer_text, local->muted_text_color, 28);
    }
    if (!egui_view_get_enable(self))
    {
        focus_color = sv_mix_disabled(focus_color);
        card_fill = sv_mix_disabled(card_fill);
        card_border = sv_mix_disabled(card_border);
        pane_fill = sv_mix_disabled(pane_fill);
        pane_border = sv_mix_disabled(pane_border);
        detail_fill = sv_mix_disabled(detail_fill);
        detail_border = sv_mix_disabled(detail_border);
        pane_title_color = sv_mix_disabled(pane_title_color);
        eyebrow_color = sv_mix_disabled(eyebrow_color);
        title_color = sv_mix_disabled(title_color);
        meta_color = sv_mix_disabled(meta_color);
        body_color = sv_mix_disabled(body_color);
        footer_fill = sv_mix_disabled(footer_fill);
        footer_border = sv_mix_disabled(footer_border);
        footer_text = sv_mix_disabled(footer_text);
    }

    egui_canvas_draw_round_rectangle_fill(metrics.content.location.x - 2, metrics.content.location.y - 2, metrics.content.size.width + 4,
                                          metrics.content.size.height + 4, radius, card_fill, egui_color_alpha_mix(self->alpha, 96));
    egui_canvas_draw_round_rectangle(metrics.content.location.x - 2, metrics.content.location.y - 2, metrics.content.size.width + 4,
                                     metrics.content.size.height + 4, radius, 1, card_border, egui_color_alpha_mix(self->alpha, 58));

    if (is_focused && egui_view_get_enable(self) && !local->read_only_mode)
    {
        egui_canvas_draw_round_rectangle(metrics.content.location.x - 2, metrics.content.location.y - 2, metrics.content.size.width + 4,
                                         metrics.content.size.height + 4, radius, 2, focus_color, egui_color_alpha_mix(self->alpha, 94));
        egui_canvas_draw_round_rectangle(metrics.content.location.x, metrics.content.location.y, metrics.content.size.width, metrics.content.size.height,
                                         detail_radius, 1, focus_color, egui_color_alpha_mix(self->alpha, 44));
    }

    egui_canvas_draw_round_rectangle_fill(metrics.pane.location.x, metrics.pane.location.y, metrics.pane.size.width, metrics.pane.size.height, detail_radius,
                                          pane_fill, egui_color_alpha_mix(self->alpha, 94));
    egui_canvas_draw_round_rectangle(metrics.pane.location.x, metrics.pane.location.y, metrics.pane.size.width, metrics.pane.size.height, detail_radius, 1,
                                     pane_border, egui_color_alpha_mix(self->alpha, 32));
    egui_canvas_draw_round_rectangle_fill(metrics.pane.location.x + 2, metrics.pane.location.y + 2, metrics.pane.size.width - 4, local->compact_mode ? 3 : 4,
                                          detail_radius, tone_color, egui_color_alpha_mix(self->alpha, local->read_only_mode ? 14 : 20));

    sv_draw_toggle(self, local, &metrics.toggle, tone_color, local->pressed_toggle);
    if (local->pane_expanded && metrics.title.size.width > 0)
    {
        sv_draw_text(local->meta_font, self, "Pane", &metrics.title, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, pane_title_color);
    }

    for (i = 0; i < local->item_count; i++)
    {
        sv_draw_row(self, local, &local->items[i], &metrics.rows[i], i == local->current_index, i == local->pressed_index);
    }

    if (!metrics.draw_detail)
    {
        return;
    }

    egui_canvas_draw_round_rectangle_fill(metrics.detail.location.x, metrics.detail.location.y, metrics.detail.size.width, metrics.detail.size.height,
                                          detail_radius, detail_fill, egui_color_alpha_mix(self->alpha, 96));
    egui_canvas_draw_round_rectangle(metrics.detail.location.x, metrics.detail.location.y, metrics.detail.size.width, metrics.detail.size.height, detail_radius,
                                     1, detail_border, egui_color_alpha_mix(self->alpha, 34));
    egui_canvas_draw_round_rectangle_fill(metrics.detail.location.x + 1, metrics.detail.location.y + 1, metrics.detail.size.width - 2,
                                          local->compact_mode ? 3 : 4, detail_radius, tone_color,
                                          egui_color_alpha_mix(self->alpha, local->read_only_mode ? 16 : 24));

    cursor_y = metrics.detail.location.y + detail_pad_y;
    if (!local->compact_mode && item->detail_eyebrow != NULL && item->detail_eyebrow[0] != '\0')
    {
        text_region.location.x = metrics.detail.location.x + detail_pad_x;
        text_region.location.y = cursor_y;
        text_region.size.width = metrics.detail.size.width - detail_pad_x * 2;
        text_region.size.height = 8;
        sv_draw_text(local->meta_font, self, item->detail_eyebrow, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, eyebrow_color);
        cursor_y += 10;
    }

    text_region.location.x = metrics.detail.location.x + detail_pad_x;
    text_region.location.y = cursor_y;
    text_region.size.width = metrics.detail.size.width - detail_pad_x * 2;
    text_region.size.height = title_h;
    sv_draw_text(local->font, self, item->detail_title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color);
    cursor_y += title_h + 2;

    if (metrics.show_meta)
    {
        text_region.location.y = cursor_y;
        text_region.size.height = meta_h;
        sv_draw_text(local->meta_font, self, item->detail_meta, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, meta_color);
        cursor_y += meta_h + 2;
    }

    text_region.location.y = cursor_y;
    text_region.size.height = body_h;
    sv_draw_text(local->meta_font, self, item->detail_body_primary, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, body_color);
    if (metrics.show_secondary_body)
    {
        cursor_y += body_h + body_gap;
        text_region.location.y = cursor_y;
        text_region.size.height = body_h;
        sv_draw_text(local->meta_font, self, item->detail_body_secondary, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, body_color);
    }

    footer_w = sv_footer_width(item->detail_footer, local->compact_mode, metrics.detail.size.width - detail_pad_x * 2);
    egui_canvas_draw_round_rectangle_fill(metrics.detail.location.x + detail_pad_x,
                                          metrics.detail.location.y + metrics.detail.size.height - footer_h - detail_pad_y, footer_w, footer_h, footer_h / 2,
                                          footer_fill, egui_color_alpha_mix(self->alpha, local->compact_mode ? 82 : 98));
    egui_canvas_draw_round_rectangle(metrics.detail.location.x + detail_pad_x, metrics.detail.location.y + metrics.detail.size.height - footer_h - detail_pad_y,
                                     footer_w, footer_h, footer_h / 2, 1, footer_border, egui_color_alpha_mix(self->alpha, local->compact_mode ? 28 : 36));
    text_region.location.x = metrics.detail.location.x + detail_pad_x;
    text_region.location.y = metrics.detail.location.y + metrics.detail.size.height - footer_h - detail_pad_y;
    text_region.size.width = footer_w;
    text_region.size.height = footer_h;
    sv_draw_text(local->meta_font, self, item->detail_footer, &text_region, EGUI_ALIGN_CENTER, footer_text);
}
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_split_view_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_split_view_t);
    uint8_t hit_index;
    uint8_t hit_toggle;

    if (local->items == NULL || local->item_count == 0 || !egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_toggle = sv_hit_toggle(local, self, event->location.x, event->location.y);
        hit_index = sv_hit_index(local, self, event->location.x, event->location.y);
        if (!hit_toggle && hit_index == EGUI_VIEW_SPLIT_VIEW_INDEX_NONE)
        {
            return 0;
        }
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (self->is_focusable)
        {
            egui_view_request_focus(self);
        }
#endif
        local->pressed_toggle = hit_toggle;
        local->pressed_index = hit_toggle ? EGUI_VIEW_SPLIT_VIEW_INDEX_NONE : hit_index;
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_toggle = sv_hit_toggle(local, self, event->location.x, event->location.y);
        hit_index = sv_hit_index(local, self, event->location.x, event->location.y);
        if (local->pressed_toggle && hit_toggle)
        {
            egui_view_split_view_toggle_pane(self);
        }
        else if (local->pressed_index != EGUI_VIEW_SPLIT_VIEW_INDEX_NONE && local->pressed_index == hit_index)
        {
            sv_set_current_index_inner(self, hit_index, 1);
        }
        local->pressed_toggle = 0;
        local->pressed_index = EGUI_VIEW_SPLIT_VIEW_INDEX_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return (hit_toggle || hit_index != EGUI_VIEW_SPLIT_VIEW_INDEX_NONE) ? 1 : 0;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_toggle = 0;
        local->pressed_index = EGUI_VIEW_SPLIT_VIEW_INDEX_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_split_view_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_split_view_t);
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
        case EGUI_KEY_CODE_LEFT:
        case EGUI_KEY_CODE_RIGHT:
        case EGUI_KEY_CODE_HOME:
        case EGUI_KEY_CODE_END:
        case EGUI_KEY_CODE_TAB:
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
        sv_set_current_index_inner(self, next_index, 1);
        return 1;
    case EGUI_KEY_CODE_DOWN:
        next_index = local->current_index + 1 < local->item_count ? (local->current_index + 1) : (local->item_count - 1);
        sv_set_current_index_inner(self, next_index, 1);
        return 1;
    case EGUI_KEY_CODE_LEFT:
        sv_set_pane_expanded_inner(self, 0, 1);
        return 1;
    case EGUI_KEY_CODE_RIGHT:
        sv_set_pane_expanded_inner(self, 1, 1);
        return 1;
    case EGUI_KEY_CODE_HOME:
        sv_set_current_index_inner(self, 0, 1);
        return 1;
    case EGUI_KEY_CODE_END:
        sv_set_current_index_inner(self, local->item_count - 1, 1);
        return 1;
    case EGUI_KEY_CODE_TAB:
        next_index = local->current_index + 1;
        if (next_index >= local->item_count)
        {
            next_index = 0;
        }
        sv_set_current_index_inner(self, next_index, 1);
        return 1;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        egui_view_split_view_toggle_pane(self);
        return 1;
    default:
        return egui_view_on_key_event(self, event);
    }
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_split_view_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_split_view_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_split_view_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_split_view_on_key_event,
#endif
};

void egui_view_split_view_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_split_view_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_split_view_t);
    egui_view_set_padding_all(self, 2);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(self, true);
#endif

    local->items = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->on_selection_changed = NULL;
    local->on_pane_state_changed = NULL;
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
    local->pane_expanded = 1;
    local->pressed_index = EGUI_VIEW_SPLIT_VIEW_INDEX_NONE;
    local->pressed_toggle = 0;

    egui_view_set_view_name(self, "egui_view_split_view");
}
