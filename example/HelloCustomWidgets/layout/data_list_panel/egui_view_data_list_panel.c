#include "egui_view_data_list_panel.h"

#define EGUI_VIEW_DATA_LIST_PANEL_STANDARD_PAD_X    8
#define EGUI_VIEW_DATA_LIST_PANEL_STANDARD_PAD_Y    7
#define EGUI_VIEW_DATA_LIST_PANEL_STANDARD_RADIUS   10
#define EGUI_VIEW_DATA_LIST_PANEL_STANDARD_ROWS_Y   29
#define EGUI_VIEW_DATA_LIST_PANEL_STANDARD_ROW_H    14
#define EGUI_VIEW_DATA_LIST_PANEL_STANDARD_ROW_GAP  3
#define EGUI_VIEW_DATA_LIST_PANEL_STANDARD_FOOTER_H 9

#define EGUI_VIEW_DATA_LIST_PANEL_COMPACT_PAD_X    6
#define EGUI_VIEW_DATA_LIST_PANEL_COMPACT_PAD_Y    5
#define EGUI_VIEW_DATA_LIST_PANEL_COMPACT_RADIUS   8
#define EGUI_VIEW_DATA_LIST_PANEL_COMPACT_ROWS_Y   18
#define EGUI_VIEW_DATA_LIST_PANEL_COMPACT_ROW_H    10
#define EGUI_VIEW_DATA_LIST_PANEL_COMPACT_ROW_GAP  2
#define EGUI_VIEW_DATA_LIST_PANEL_COMPACT_FOOTER_H 8

typedef struct egui_view_data_list_panel_metrics egui_view_data_list_panel_metrics_t;
struct egui_view_data_list_panel_metrics
{
    egui_region_t content_region;
    egui_region_t row_regions[EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS];
    egui_region_t eyebrow_region;
    egui_region_t title_region;
    egui_region_t summary_region;
    egui_region_t footer_region;
};

static uint8_t egui_view_data_list_panel_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_DATA_LIST_PANEL_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_DATA_LIST_PANEL_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_data_list_panel_clamp_item_count(uint8_t count)
{
    if (count > EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS)
    {
        return EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS;
    }
    return count;
}

static uint8_t egui_view_data_list_panel_text_len(const char *text)
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

static egui_color_t egui_view_data_list_panel_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 68);
}

static egui_color_t egui_view_data_list_panel_tone_color(egui_view_data_list_panel_t *local, uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_DATA_LIST_PANEL_TONE_SUCCESS:
        return local->success_color;
    case EGUI_VIEW_DATA_LIST_PANEL_TONE_WARNING:
        return local->warning_color;
    case EGUI_VIEW_DATA_LIST_PANEL_TONE_NEUTRAL:
        return local->neutral_color;
    default:
        return local->accent_color;
    }
}

static const egui_view_data_list_panel_snapshot_t *egui_view_data_list_panel_get_snapshot(egui_view_data_list_panel_t *local)
{
    if (local->snapshots == NULL || local->snapshot_count == 0 || local->current_snapshot >= local->snapshot_count)
    {
        return NULL;
    }
    return &local->snapshots[local->current_snapshot];
}

static const egui_view_data_list_panel_item_t *egui_view_data_list_panel_get_item(egui_view_data_list_panel_t *local)
{
    const egui_view_data_list_panel_snapshot_t *snapshot = egui_view_data_list_panel_get_snapshot(local);

    if (snapshot == NULL || snapshot->items == NULL || snapshot->item_count == 0 || local->current_index >= snapshot->item_count)
    {
        return NULL;
    }
    return &snapshot->items[local->current_index];
}

static void egui_view_data_list_panel_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                                egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (text == NULL || text[0] == '\0')
    {
        return;
    }
    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static egui_dim_t egui_view_data_list_panel_pill_width(const char *text, uint8_t compact_mode, egui_dim_t min_w, egui_dim_t max_w)
{
    egui_dim_t width = min_w + egui_view_data_list_panel_text_len(text) * (compact_mode ? 4 : 5);

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

static egui_dim_t egui_view_data_list_panel_footer_width(const char *text, uint8_t compact_mode, egui_dim_t max_w)
{
    egui_dim_t width = (compact_mode ? 18 : 22) + egui_view_data_list_panel_text_len(text) * (compact_mode ? 4 : 5);

    if (width > max_w)
    {
        width = max_w;
    }
    return width;
}

static void egui_view_data_list_panel_notify_change(egui_view_t *self, egui_view_data_list_panel_t *local)
{
    if (local->on_selection_changed)
    {
        local->on_selection_changed(self, local->current_snapshot, local->current_index);
    }
}

static void egui_view_data_list_panel_set_current_index_inner(egui_view_t *self, uint8_t item_index, uint8_t notify);

static void egui_view_data_list_panel_get_metrics(egui_view_data_list_panel_t *local, egui_view_t *self, egui_view_data_list_panel_metrics_t *metrics)
{
    const egui_view_data_list_panel_snapshot_t *snapshot = egui_view_data_list_panel_get_snapshot(local);
    egui_region_t work_region;
    egui_dim_t pad_x = local->compact_mode ? EGUI_VIEW_DATA_LIST_PANEL_COMPACT_PAD_X : EGUI_VIEW_DATA_LIST_PANEL_STANDARD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? EGUI_VIEW_DATA_LIST_PANEL_COMPACT_PAD_Y : EGUI_VIEW_DATA_LIST_PANEL_STANDARD_PAD_Y;
    egui_dim_t row_h = local->compact_mode ? EGUI_VIEW_DATA_LIST_PANEL_COMPACT_ROW_H : EGUI_VIEW_DATA_LIST_PANEL_STANDARD_ROW_H;
    egui_dim_t row_gap = local->compact_mode ? EGUI_VIEW_DATA_LIST_PANEL_COMPACT_ROW_GAP : EGUI_VIEW_DATA_LIST_PANEL_STANDARD_ROW_GAP;
    egui_dim_t footer_h = local->compact_mode ? EGUI_VIEW_DATA_LIST_PANEL_COMPACT_FOOTER_H : EGUI_VIEW_DATA_LIST_PANEL_STANDARD_FOOTER_H;
    egui_dim_t rows_y = local->compact_mode ? EGUI_VIEW_DATA_LIST_PANEL_COMPACT_ROWS_Y : EGUI_VIEW_DATA_LIST_PANEL_STANDARD_ROWS_Y;
    egui_dim_t available_h;
    egui_dim_t total_h;
    egui_dim_t start_y;
    uint8_t item_count = 0;
    uint8_t i;

    egui_view_get_work_region(self, &work_region);
    metrics->content_region.location.x = work_region.location.x + pad_x;
    metrics->content_region.location.y = work_region.location.y + pad_y;
    metrics->content_region.size.width = work_region.size.width - pad_x * 2;
    metrics->content_region.size.height = work_region.size.height - pad_y * 2;

    if (snapshot != NULL)
    {
        item_count = egui_view_data_list_panel_clamp_item_count(snapshot->item_count);
    }

    for (i = 0; i < EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS; i++)
    {
        metrics->row_regions[i].location.x = 0;
        metrics->row_regions[i].location.y = 0;
        metrics->row_regions[i].size.width = 0;
        metrics->row_regions[i].size.height = 0;
    }

    metrics->eyebrow_region.location.x = metrics->content_region.location.x + 2;
    metrics->eyebrow_region.location.y = metrics->content_region.location.y + 1;
    metrics->eyebrow_region.size.width = metrics->content_region.size.width - 4;
    metrics->eyebrow_region.size.height = 8;

    metrics->title_region.location.x = metrics->content_region.location.x + 2;
    metrics->title_region.location.y = metrics->content_region.location.y + (local->compact_mode ? 4 : 10);
    metrics->title_region.size.width = metrics->content_region.size.width - 4;
    metrics->title_region.size.height = local->compact_mode ? 9 : 11;

    metrics->summary_region.location.x = metrics->content_region.location.x + 4;
    metrics->summary_region.location.y = metrics->content_region.location.y + (local->compact_mode ? 0 : 22);
    metrics->summary_region.size.width = metrics->content_region.size.width - 8;
    metrics->summary_region.size.height = local->compact_mode ? 0 : 8;

    metrics->footer_region.location.x = metrics->content_region.location.x + 4;
    metrics->footer_region.location.y = metrics->content_region.location.y + metrics->content_region.size.height - footer_h - (local->compact_mode ? 1 : 0);
    metrics->footer_region.size.width = metrics->content_region.size.width - 8;
    metrics->footer_region.size.height = footer_h;

    available_h = metrics->footer_region.location.y - (metrics->content_region.location.y + rows_y) - (local->compact_mode ? 2 : 5);
    total_h = item_count > 0 ? (item_count * row_h + (item_count - 1) * row_gap) : 0;
    if (total_h > available_h)
    {
        row_gap = 1;
        total_h = item_count > 0 ? (item_count * row_h + (item_count - 1) * row_gap) : 0;
    }
    if (total_h > available_h)
    {
        row_h = local->compact_mode ? 9 : 13;
        total_h = item_count > 0 ? (item_count * row_h + (item_count - 1) * row_gap) : 0;
    }

    start_y = metrics->content_region.location.y + rows_y;
    if (available_h > total_h)
    {
        start_y += (available_h - total_h) / 2;
    }

    for (i = 0; i < item_count; i++)
    {
        metrics->row_regions[i].location.x = metrics->content_region.location.x + 4;
        metrics->row_regions[i].location.y = start_y + i * (row_h + row_gap);
        metrics->row_regions[i].size.width = metrics->content_region.size.width - 8;
        metrics->row_regions[i].size.height = row_h;
    }
}

static uint8_t egui_view_data_list_panel_hit_index(egui_view_data_list_panel_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    const egui_view_data_list_panel_snapshot_t *snapshot = egui_view_data_list_panel_get_snapshot(local);
    egui_view_data_list_panel_metrics_t metrics;
    uint8_t item_count;
    uint8_t i;

    if (snapshot == NULL)
    {
        return EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS;
    }

    item_count = egui_view_data_list_panel_clamp_item_count(snapshot->item_count);
    egui_view_data_list_panel_get_metrics(local, self, &metrics);

    for (i = 0; i < item_count; i++)
    {
        if (egui_region_pt_in_rect(&metrics.row_regions[i], x, y))
        {
            return i;
        }
    }
    return EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS;
}

static void egui_view_data_list_panel_set_current_snapshot_inner(egui_view_t *self, uint8_t snapshot_index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_data_list_panel_t);
    const egui_view_data_list_panel_snapshot_t *snapshot;

    if (local->snapshots == NULL || local->snapshot_count == 0 || snapshot_index >= local->snapshot_count)
    {
        return;
    }
    if (local->current_snapshot == snapshot_index)
    {
        return;
    }

    local->current_snapshot = snapshot_index;
    snapshot = egui_view_data_list_panel_get_snapshot(local);
    if (snapshot != NULL)
    {
        local->current_index = snapshot->focus_index;
        if (local->current_index >= snapshot->item_count)
        {
            local->current_index = 0;
        }
    }
    local->pressed_index = EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS;
    if (notify)
    {
        egui_view_data_list_panel_notify_change(self, local);
    }
    egui_view_invalidate(self);
}

static void egui_view_data_list_panel_set_current_index_inner(egui_view_t *self, uint8_t item_index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_data_list_panel_t);
    const egui_view_data_list_panel_snapshot_t *snapshot = egui_view_data_list_panel_get_snapshot(local);

    if (snapshot == NULL || snapshot->items == NULL || snapshot->item_count == 0 || item_index >= snapshot->item_count)
    {
        return;
    }
    if (local->current_index == item_index)
    {
        return;
    }

    local->current_index = item_index;
    if (notify)
    {
        egui_view_data_list_panel_notify_change(self, local);
    }
    egui_view_invalidate(self);
}

void egui_view_data_list_panel_set_snapshots(egui_view_t *self, const egui_view_data_list_panel_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_data_list_panel_t);
    const egui_view_data_list_panel_snapshot_t *snapshot;

    local->snapshots = snapshots;
    local->snapshot_count = egui_view_data_list_panel_clamp_snapshot_count(snapshot_count);
    local->current_snapshot = 0;
    local->current_index = 0;
    local->pressed_index = EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS;

    snapshot = egui_view_data_list_panel_get_snapshot(local);
    if (snapshot != NULL && snapshot->item_count > 0)
    {
        local->current_index = snapshot->focus_index;
        if (local->current_index >= snapshot->item_count)
        {
            local->current_index = 0;
        }
    }
    egui_view_invalidate(self);
}

void egui_view_data_list_panel_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    egui_view_data_list_panel_set_current_snapshot_inner(self, snapshot_index, 1);
}

uint8_t egui_view_data_list_panel_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_data_list_panel_t);
    return local->current_snapshot;
}

void egui_view_data_list_panel_set_current_index(egui_view_t *self, uint8_t item_index)
{
    egui_view_data_list_panel_set_current_index_inner(self, item_index, 1);
}

uint8_t egui_view_data_list_panel_get_current_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_data_list_panel_t);
    return local->current_index;
}

void egui_view_data_list_panel_set_on_selection_changed_listener(egui_view_t *self, egui_view_on_data_list_panel_selection_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_data_list_panel_t);
    local->on_selection_changed = listener;
}

void egui_view_data_list_panel_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_data_list_panel_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_data_list_panel_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_data_list_panel_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_data_list_panel_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_data_list_panel_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_data_list_panel_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_data_list_panel_t);
    local->read_only_mode = read_only_mode ? 1 : 0;
    local->pressed_index = EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS;
    egui_view_set_pressed(self, false);
    egui_view_invalidate(self);
}

void egui_view_data_list_panel_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t section_color,
                                           egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color,
                                           egui_color_t warning_color, egui_color_t neutral_color)
{
    EGUI_LOCAL_INIT(egui_view_data_list_panel_t);

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

static void egui_view_data_list_panel_draw_row(egui_view_t *self, egui_view_data_list_panel_t *local, const egui_view_data_list_panel_item_t *item,
                                               const egui_region_t *region, uint8_t selected, uint8_t pressed, uint8_t last)
{
    egui_region_t text_region;
    egui_color_t tone_color = egui_view_data_list_panel_tone_color(local, item->tone);
    egui_color_t row_fill = egui_rgb_mix(local->surface_color, tone_color, selected ? 12 : (local->compact_mode ? 4 : 7));
    egui_color_t row_border = egui_rgb_mix(local->border_color, tone_color, selected ? 24 : (local->compact_mode ? 10 : 14));
    egui_color_t glyph_fill = egui_rgb_mix(local->surface_color, tone_color, selected ? 20 : (local->compact_mode ? 18 : 12));
    egui_color_t glyph_text = selected ? tone_color : egui_rgb_mix(local->text_color, tone_color, local->compact_mode ? 14 : 10);
    egui_color_t title_color = selected ? egui_rgb_mix(local->text_color, tone_color, 8) : local->text_color;
    egui_color_t value_fill = egui_rgb_mix(local->surface_color, tone_color, selected ? 16 : 10);
    egui_color_t value_border = egui_rgb_mix(local->border_color, tone_color, selected ? 24 : 16);
    egui_color_t value_color = selected ? tone_color : egui_rgb_mix(local->muted_text_color, tone_color, 18);
    egui_color_t divider_color = egui_rgb_mix(local->border_color, tone_color, 10);
    egui_dim_t glyph_size = local->compact_mode ? 8 : 10;
    egui_dim_t glyph_x = region->location.x + (local->compact_mode ? 5 : 6);
    egui_dim_t glyph_y = region->location.y + (region->size.height - glyph_size) / 2;
    egui_dim_t title_x = glyph_x + glyph_size + (local->compact_mode ? 6 : 6);
    egui_dim_t value_h = local->compact_mode ? 9 : 13;
    egui_dim_t value_w = local->compact_mode ? (egui_dim_t)(egui_view_data_list_panel_text_len(item->value) * 4 + 6)
                                             : egui_view_data_list_panel_pill_width(item->value, local->compact_mode, 24, region->size.width / 3);
    egui_dim_t value_x = region->location.x + region->size.width - value_w - (local->compact_mode ? 4 : 6);
    egui_dim_t value_y = region->location.y + (region->size.height - value_h) / 2;
    egui_dim_t indicator_w = local->compact_mode ? 2 : 3;
    uint8_t draw_row_box = (!local->compact_mode) || selected || pressed;

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
        row_fill = egui_rgb_mix(row_fill, local->surface_color, 24);
        row_border = egui_rgb_mix(row_border, local->muted_text_color, 20);
        glyph_fill = egui_rgb_mix(glyph_fill, local->surface_color, 28);
        glyph_text = egui_rgb_mix(glyph_text, local->muted_text_color, 26);
        title_color = egui_rgb_mix(title_color, local->muted_text_color, 14);
        value_fill = egui_rgb_mix(value_fill, local->surface_color, 26);
        value_border = egui_rgb_mix(value_border, local->muted_text_color, 20);
        value_color = egui_rgb_mix(value_color, local->muted_text_color, 26);
        divider_color = egui_rgb_mix(divider_color, local->muted_text_color, 18);
    }
    if (!egui_view_get_enable(self))
    {
        row_fill = egui_view_data_list_panel_mix_disabled(row_fill);
        row_border = egui_view_data_list_panel_mix_disabled(row_border);
        glyph_fill = egui_view_data_list_panel_mix_disabled(glyph_fill);
        glyph_text = egui_view_data_list_panel_mix_disabled(glyph_text);
        title_color = egui_view_data_list_panel_mix_disabled(title_color);
        value_fill = egui_view_data_list_panel_mix_disabled(value_fill);
        value_border = egui_view_data_list_panel_mix_disabled(value_border);
        value_color = egui_view_data_list_panel_mix_disabled(value_color);
        divider_color = egui_view_data_list_panel_mix_disabled(divider_color);
    }

    if (draw_row_box)
    {
        egui_canvas_draw_round_rectangle_fill(region->location.x, region->location.y, region->size.width, region->size.height, local->compact_mode ? 5 : 7,
                                              row_fill, egui_color_alpha_mix(self->alpha, local->compact_mode ? 84 : 92));
        egui_canvas_draw_round_rectangle(region->location.x, region->location.y, region->size.width, region->size.height, local->compact_mode ? 5 : 7, 1,
                                         row_border, egui_color_alpha_mix(self->alpha, local->compact_mode ? 28 : (selected ? 42 : 28)));
    }
    else if (!last)
    {
        egui_canvas_draw_line(region->location.x + 11, region->location.y + region->size.height, region->location.x + region->size.width - 10,
                              region->location.y + region->size.height, 1, divider_color, egui_color_alpha_mix(self->alpha, 18));
    }

    if (selected)
    {
        egui_canvas_draw_round_rectangle_fill(region->location.x + 1, region->location.y + 2, indicator_w, region->size.height - 4, 1, tone_color,
                                              egui_color_alpha_mix(self->alpha, 100));
    }

    egui_canvas_draw_round_rectangle_fill(glyph_x, glyph_y, glyph_size, glyph_size, local->compact_mode ? 3 : 4, glyph_fill,
                                          egui_color_alpha_mix(self->alpha, 96));
    if (local->compact_mode)
    {
        egui_canvas_draw_round_rectangle(glyph_x, glyph_y, glyph_size, glyph_size, 3, 1, row_border, egui_color_alpha_mix(self->alpha, 28));
    }
    text_region.location.x = glyph_x;
    text_region.location.y = glyph_y;
    text_region.size.width = glyph_size;
    text_region.size.height = glyph_size;
    egui_view_data_list_panel_draw_text(local->meta_font, self, item->glyph, &text_region, EGUI_ALIGN_CENTER, glyph_text);

    text_region.location.x = value_x;
    text_region.location.y = value_y;
    text_region.size.width = value_w;
    text_region.size.height = value_h;
    if (local->compact_mode)
    {
        egui_view_data_list_panel_draw_text(local->meta_font, self, item->value, &text_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER, value_color);
    }
    else
    {
        egui_canvas_draw_round_rectangle_fill(value_x, value_y, value_w, value_h, value_h / 2, value_fill, egui_color_alpha_mix(self->alpha, 94));
        egui_canvas_draw_round_rectangle(value_x, value_y, value_w, value_h, value_h / 2, 1, value_border, egui_color_alpha_mix(self->alpha, 36));
        egui_view_data_list_panel_draw_text(local->meta_font, self, item->value, &text_region, EGUI_ALIGN_CENTER, value_color);
    }

    text_region.location.x = title_x;
    text_region.location.y = region->location.y;
    text_region.size.width = value_x - title_x - (local->compact_mode ? 4 : 6);
    text_region.size.height = region->size.height;
    egui_view_data_list_panel_draw_text(local->font, self, item->title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color);
}

static void egui_view_data_list_panel_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_data_list_panel_t);
    const egui_view_data_list_panel_snapshot_t *snapshot = egui_view_data_list_panel_get_snapshot(local);
    const egui_view_data_list_panel_item_t *item = egui_view_data_list_panel_get_item(local);
    egui_view_data_list_panel_metrics_t metrics;
    egui_region_t text_region;
    egui_color_t card_fill;
    egui_color_t card_border;
    egui_color_t eyebrow_color;
    egui_color_t title_color;
    egui_color_t summary_color;
    egui_color_t footer_fill;
    egui_color_t footer_border;
    egui_color_t footer_text;
    egui_color_t tone_color;
    egui_dim_t card_radius = local->compact_mode ? EGUI_VIEW_DATA_LIST_PANEL_COMPACT_RADIUS : EGUI_VIEW_DATA_LIST_PANEL_STANDARD_RADIUS;
    egui_dim_t footer_w;
    uint8_t item_count;
    uint8_t i;

    if (snapshot == NULL || item == NULL)
    {
        return;
    }

    egui_view_data_list_panel_get_metrics(local, self, &metrics);
    if (metrics.content_region.size.width <= 0 || metrics.content_region.size.height <= 0)
    {
        return;
    }

    item_count = egui_view_data_list_panel_clamp_item_count(snapshot->item_count);
    tone_color = egui_view_data_list_panel_tone_color(local, item->tone);
    card_fill = egui_rgb_mix(local->surface_color, local->section_color, local->compact_mode ? 5 : 8);
    card_border = egui_rgb_mix(local->border_color, tone_color, local->compact_mode ? 12 : 16);
    eyebrow_color = egui_rgb_mix(tone_color, local->muted_text_color, 18);
    title_color = local->text_color;
    summary_color = egui_rgb_mix(local->muted_text_color, tone_color, 14);
    footer_fill = egui_rgb_mix(local->surface_color, tone_color, local->compact_mode ? 7 : 14);
    footer_border = egui_rgb_mix(local->border_color, tone_color, local->compact_mode ? 10 : 18);
    footer_text = local->compact_mode ? egui_rgb_mix(local->muted_text_color, tone_color, 20) : tone_color;

    if (local->read_only_mode)
    {
        card_fill = egui_rgb_mix(card_fill, local->surface_color, 22);
        card_border = egui_rgb_mix(card_border, local->muted_text_color, 20);
        eyebrow_color = egui_rgb_mix(eyebrow_color, local->muted_text_color, 24);
        title_color = egui_rgb_mix(title_color, local->muted_text_color, 12);
        summary_color = egui_rgb_mix(summary_color, local->muted_text_color, 18);
        footer_fill = egui_rgb_mix(footer_fill, local->surface_color, 24);
        footer_border = egui_rgb_mix(footer_border, local->muted_text_color, 20);
        footer_text = egui_rgb_mix(footer_text, local->muted_text_color, 28);
    }
    if (!egui_view_get_enable(self))
    {
        card_fill = egui_view_data_list_panel_mix_disabled(card_fill);
        card_border = egui_view_data_list_panel_mix_disabled(card_border);
        eyebrow_color = egui_view_data_list_panel_mix_disabled(eyebrow_color);
        title_color = egui_view_data_list_panel_mix_disabled(title_color);
        summary_color = egui_view_data_list_panel_mix_disabled(summary_color);
        footer_fill = egui_view_data_list_panel_mix_disabled(footer_fill);
        footer_border = egui_view_data_list_panel_mix_disabled(footer_border);
        footer_text = egui_view_data_list_panel_mix_disabled(footer_text);
    }

    egui_canvas_draw_round_rectangle_fill(metrics.content_region.location.x - 2, metrics.content_region.location.y - 2, metrics.content_region.size.width + 4,
                                          metrics.content_region.size.height + 4, card_radius, card_fill, egui_color_alpha_mix(self->alpha, 96));
    egui_canvas_draw_round_rectangle(metrics.content_region.location.x - 2, metrics.content_region.location.y - 2, metrics.content_region.size.width + 4,
                                     metrics.content_region.size.height + 4, card_radius, 1, card_border, egui_color_alpha_mix(self->alpha, 56));

    if (!local->compact_mode)
    {
        egui_view_data_list_panel_draw_text(local->meta_font, self, snapshot->eyebrow, &metrics.eyebrow_region, EGUI_ALIGN_CENTER, eyebrow_color);
    }
    egui_view_data_list_panel_draw_text(local->font, self, snapshot->title, &metrics.title_region, EGUI_ALIGN_CENTER, title_color);
    if (!local->compact_mode)
    {
        egui_view_data_list_panel_draw_text(local->meta_font, self, snapshot->summary, &metrics.summary_region, EGUI_ALIGN_CENTER, summary_color);
    }

    for (i = 0; i < item_count; i++)
    {
        egui_view_data_list_panel_draw_row(self, local, &snapshot->items[i], &metrics.row_regions[i], i == local->current_index, i == local->pressed_index,
                                           i + 1 >= item_count);
    }

    footer_w = egui_view_data_list_panel_footer_width(item->footer, local->compact_mode, metrics.footer_region.size.width);
    text_region = metrics.footer_region;
    text_region.location.x = metrics.content_region.location.x + (metrics.content_region.size.width - footer_w) / 2;
    text_region.size.width = footer_w;

    egui_canvas_draw_round_rectangle_fill(text_region.location.x, text_region.location.y, text_region.size.width, text_region.size.height,
                                          text_region.size.height / 2, footer_fill, egui_color_alpha_mix(self->alpha, local->compact_mode ? 78 : 98));
    if (!local->compact_mode)
    {
        egui_canvas_draw_round_rectangle(text_region.location.x, text_region.location.y, text_region.size.width, text_region.size.height,
                                         text_region.size.height / 2, 1, footer_border, egui_color_alpha_mix(self->alpha, 34));
    }
    egui_view_data_list_panel_draw_text(local->meta_font, self, item->footer, &text_region, EGUI_ALIGN_CENTER, footer_text);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_data_list_panel_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_data_list_panel_t);
    uint8_t hit_index;

    if (local->snapshots == NULL || local->snapshot_count == 0 || !egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_index = egui_view_data_list_panel_hit_index(local, self, event->location.x, event->location.y);
        if (hit_index >= EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS)
        {
            return 0;
        }
        local->pressed_index = hit_index;
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_index = egui_view_data_list_panel_hit_index(local, self, event->location.x, event->location.y);
        if (local->pressed_index == hit_index && hit_index < EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS)
        {
            egui_view_data_list_panel_set_current_index_inner(self, hit_index, 1);
        }
        local->pressed_index = EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_index < EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_index = EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_data_list_panel_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_data_list_panel_t);
    const egui_view_data_list_panel_snapshot_t *snapshot = egui_view_data_list_panel_get_snapshot(local);
    uint8_t next_index;

    if (snapshot == NULL || snapshot->item_count == 0 || !egui_view_get_enable(self) || local->read_only_mode || event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        return 0;
    }

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_UP:
    case EGUI_KEY_CODE_LEFT:
        next_index = local->current_index > 0 ? (local->current_index - 1) : 0;
        egui_view_data_list_panel_set_current_index_inner(self, next_index, 1);
        return 1;
    case EGUI_KEY_CODE_DOWN:
    case EGUI_KEY_CODE_RIGHT:
        next_index = local->current_index + 1 < snapshot->item_count ? (local->current_index + 1) : (snapshot->item_count - 1);
        egui_view_data_list_panel_set_current_index_inner(self, next_index, 1);
        return 1;
    case EGUI_KEY_CODE_HOME:
        egui_view_data_list_panel_set_current_index_inner(self, 0, 1);
        return 1;
    case EGUI_KEY_CODE_END:
        egui_view_data_list_panel_set_current_index_inner(self, snapshot->item_count - 1, 1);
        return 1;
    case EGUI_KEY_CODE_TAB:
        next_index = local->current_index + 1;
        if (next_index >= snapshot->item_count)
        {
            next_index = 0;
        }
        egui_view_data_list_panel_set_current_index_inner(self, next_index, 1);
        return 1;
    default:
        return egui_view_on_key_event(self, event);
    }
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_data_list_panel_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_data_list_panel_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_data_list_panel_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_data_list_panel_on_key_event,
#endif
};

void egui_view_data_list_panel_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_data_list_panel_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_data_list_panel_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
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
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->current_index = 0;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    local->pressed_index = EGUI_VIEW_DATA_LIST_PANEL_MAX_ITEMS;

    egui_view_set_view_name(self, "egui_view_data_list_panel");
}
