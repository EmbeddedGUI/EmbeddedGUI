#include "egui_view_tree_view.h"

#define EGUI_VIEW_TREE_VIEW_STANDARD_RADIUS          8
#define EGUI_VIEW_TREE_VIEW_STANDARD_FILL_ALPHA      92
#define EGUI_VIEW_TREE_VIEW_STANDARD_BORDER_ALPHA    60
#define EGUI_VIEW_TREE_VIEW_STANDARD_PAD_X           9
#define EGUI_VIEW_TREE_VIEW_STANDARD_PAD_Y           8
#define EGUI_VIEW_TREE_VIEW_STANDARD_HEADER_HEIGHT   11
#define EGUI_VIEW_TREE_VIEW_STANDARD_HEADER_GAP      5
#define EGUI_VIEW_TREE_VIEW_STANDARD_CAPTION_MIN_W   30
#define EGUI_VIEW_TREE_VIEW_STANDARD_LIST_GAP        5
#define EGUI_VIEW_TREE_VIEW_STANDARD_FOOTER_HEIGHT   13
#define EGUI_VIEW_TREE_VIEW_STANDARD_ROW_HEIGHT      15
#define EGUI_VIEW_TREE_VIEW_STANDARD_ROW_GAP         2
#define EGUI_VIEW_TREE_VIEW_STANDARD_ROW_RADIUS      6
#define EGUI_VIEW_TREE_VIEW_STANDARD_INDENT          14
#define EGUI_VIEW_TREE_VIEW_STANDARD_CARET_BOX_WIDTH 7
#define EGUI_VIEW_TREE_VIEW_STANDARD_CARET_SIZE      5
#define EGUI_VIEW_TREE_VIEW_STANDARD_GLYPH_WIDTH     9
#define EGUI_VIEW_TREE_VIEW_STANDARD_GLYPH_HEIGHT    8
#define EGUI_VIEW_TREE_VIEW_STANDARD_GLYPH_RADIUS    3
#define EGUI_VIEW_TREE_VIEW_STANDARD_META_MIN_W      16
#define EGUI_VIEW_TREE_VIEW_STANDARD_META_HEIGHT     10

#define EGUI_VIEW_TREE_VIEW_COMPACT_RADIUS          7
#define EGUI_VIEW_TREE_VIEW_COMPACT_FILL_ALPHA      88
#define EGUI_VIEW_TREE_VIEW_COMPACT_BORDER_ALPHA    58
#define EGUI_VIEW_TREE_VIEW_COMPACT_PAD_X           6
#define EGUI_VIEW_TREE_VIEW_COMPACT_PAD_Y           5
#define EGUI_VIEW_TREE_VIEW_COMPACT_HEADER_HEIGHT   10
#define EGUI_VIEW_TREE_VIEW_COMPACT_HEADER_GAP      3
#define EGUI_VIEW_TREE_VIEW_COMPACT_CAPTION_MIN_W   24
#define EGUI_VIEW_TREE_VIEW_COMPACT_LIST_GAP        3
#define EGUI_VIEW_TREE_VIEW_COMPACT_FOOTER_HEIGHT   10
#define EGUI_VIEW_TREE_VIEW_COMPACT_ROW_HEIGHT      11
#define EGUI_VIEW_TREE_VIEW_COMPACT_ROW_GAP         1
#define EGUI_VIEW_TREE_VIEW_COMPACT_ROW_RADIUS      3
#define EGUI_VIEW_TREE_VIEW_COMPACT_INDENT          12
#define EGUI_VIEW_TREE_VIEW_COMPACT_CARET_BOX_WIDTH 6
#define EGUI_VIEW_TREE_VIEW_COMPACT_CARET_SIZE      4
#define EGUI_VIEW_TREE_VIEW_COMPACT_GLYPH_WIDTH     7
#define EGUI_VIEW_TREE_VIEW_COMPACT_GLYPH_HEIGHT    6
#define EGUI_VIEW_TREE_VIEW_COMPACT_GLYPH_RADIUS    2
#define EGUI_VIEW_TREE_VIEW_COMPACT_META_MIN_W      12
#define EGUI_VIEW_TREE_VIEW_COMPACT_META_HEIGHT     7

typedef struct egui_view_tree_view_metrics egui_view_tree_view_metrics_t;
struct egui_view_tree_view_metrics
{
    egui_region_t content;
    egui_region_t header_region;
    egui_region_t list_region;
    egui_region_t footer_region;
    egui_region_t item_regions[EGUI_VIEW_TREE_VIEW_MAX_ITEMS];
    uint8_t visible_item_count;
};

static uint8_t egui_view_tree_view_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_TREE_VIEW_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_TREE_VIEW_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_tree_view_clamp_item_count(uint8_t count)
{
    if (count > EGUI_VIEW_TREE_VIEW_MAX_ITEMS)
    {
        return EGUI_VIEW_TREE_VIEW_MAX_ITEMS;
    }
    return count;
}

static uint8_t egui_view_tree_view_text_len(const char *text)
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

static egui_color_t egui_view_tree_view_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 68);
}

static egui_color_t egui_view_tree_view_tone_color(egui_view_tree_view_t *local, uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_TREE_VIEW_TONE_SUCCESS:
        return local->success_color;
    case EGUI_VIEW_TREE_VIEW_TONE_WARNING:
        return local->warning_color;
    case EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL:
        return local->neutral_color;
    default:
        return local->accent_color;
    }
}

static const egui_view_tree_view_snapshot_t *egui_view_tree_view_get_snapshot(egui_view_tree_view_t *local)
{
    if (local->snapshots == NULL || local->snapshot_count == 0 || local->current_snapshot >= local->snapshot_count)
    {
        return NULL;
    }
    return &local->snapshots[local->current_snapshot];
}

static uint8_t egui_view_tree_view_focus_index(const egui_view_tree_view_snapshot_t *snapshot, uint8_t item_count)
{
    if (snapshot == NULL || item_count == 0 || snapshot->focus_index >= item_count)
    {
        return 0;
    }
    return snapshot->focus_index;
}

static egui_dim_t egui_view_tree_view_caption_width(const char *text, uint8_t compact_mode, egui_dim_t max_w)
{
    egui_dim_t min_w = compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_CAPTION_MIN_W : EGUI_VIEW_TREE_VIEW_STANDARD_CAPTION_MIN_W;
    egui_dim_t width = min_w;

    if (text != NULL && text[0] != '\0')
    {
        width += egui_view_tree_view_text_len(text) * (compact_mode ? 4 : 5);
    }

    if (width > max_w)
    {
        width = max_w;
    }
    return width;
}

static egui_dim_t egui_view_tree_view_meta_width(const char *text, uint8_t compact_mode, egui_dim_t max_w)
{
    egui_dim_t min_w = compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_META_MIN_W : EGUI_VIEW_TREE_VIEW_STANDARD_META_MIN_W;
    egui_dim_t width;

    if (text == NULL || text[0] == '\0')
    {
        return 0;
    }

    width = min_w + egui_view_tree_view_text_len(text) * (compact_mode ? 4 : 5);
    if (width > max_w)
    {
        width = max_w;
    }
    return width;
}

static void egui_view_tree_view_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                          egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (text == NULL || text[0] == '\0')
    {
        return;
    }

    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void egui_view_tree_view_draw_caret(egui_view_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t size, uint8_t expanded, egui_color_t color,
                                           egui_alpha_t alpha)
{
    if (expanded)
    {
        egui_canvas_draw_triangle_fill(x, y + 1, x + size, y + 1, x + size / 2, y + size / 2 + 2, color, egui_color_alpha_mix(self->alpha, alpha));
    }
    else
    {
        egui_canvas_draw_triangle_fill(x + 1, y, x + 1, y + size, x + size / 2 + 2, y + size / 2, color, egui_color_alpha_mix(self->alpha, alpha));
    }
}

static void egui_view_tree_view_draw_folder_glyph(egui_view_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t fill,
                                                  egui_color_t border, egui_alpha_t alpha, egui_dim_t radius)
{
    egui_dim_t tab_w = width / 2;
    egui_dim_t tab_h = height / 2;

    if (tab_w < 3)
    {
        tab_w = 3;
    }
    if (tab_h < 2)
    {
        tab_h = 2;
    }

    egui_canvas_draw_round_rectangle_fill(x + 1, y, tab_w, tab_h, radius, fill, egui_color_alpha_mix(self->alpha, alpha));
    egui_canvas_draw_round_rectangle_fill(x, y + 1, width, height - 1, radius, fill, egui_color_alpha_mix(self->alpha, alpha));
    egui_canvas_draw_round_rectangle(x, y + 1, width, height - 1, radius, 1, border, egui_color_alpha_mix(self->alpha, 42));
}

static void egui_view_tree_view_draw_leaf_glyph(egui_view_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t fill,
                                                egui_color_t border, egui_alpha_t alpha, egui_dim_t radius)
{
    egui_canvas_draw_round_rectangle_fill(x, y, width, height, radius, fill, egui_color_alpha_mix(self->alpha, alpha));
    egui_canvas_draw_round_rectangle(x, y, width, height, radius, 1, border, egui_color_alpha_mix(self->alpha, 40));
    egui_canvas_draw_line(x + 2, y + 2, x + width - 3, y + 2, 1, border, egui_color_alpha_mix(self->alpha, 32));
}

static void egui_view_tree_view_get_metrics(egui_view_tree_view_t *local, egui_view_t *self, uint8_t item_count, egui_view_tree_view_metrics_t *metrics)
{
    egui_region_t region;
    egui_dim_t pad_x = local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_PAD_X : EGUI_VIEW_TREE_VIEW_STANDARD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_PAD_Y : EGUI_VIEW_TREE_VIEW_STANDARD_PAD_Y;
    egui_dim_t header_h = local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_HEADER_HEIGHT : EGUI_VIEW_TREE_VIEW_STANDARD_HEADER_HEIGHT;
    egui_dim_t header_gap = local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_HEADER_GAP : EGUI_VIEW_TREE_VIEW_STANDARD_HEADER_GAP;
    egui_dim_t list_gap = local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_LIST_GAP : EGUI_VIEW_TREE_VIEW_STANDARD_LIST_GAP;
    egui_dim_t footer_h = local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_FOOTER_HEIGHT : EGUI_VIEW_TREE_VIEW_STANDARD_FOOTER_HEIGHT;
    egui_dim_t row_h = local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_ROW_HEIGHT : EGUI_VIEW_TREE_VIEW_STANDARD_ROW_HEIGHT;
    egui_dim_t row_gap = local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_ROW_GAP : EGUI_VIEW_TREE_VIEW_STANDARD_ROW_GAP;
    egui_dim_t required_rows_h;
    egui_dim_t list_inset = local->compact_mode ? 2 : 4;
    egui_dim_t min_row_h = local->compact_mode ? 8 : 10;
    egui_dim_t available_height;
    egui_dim_t item_y;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    metrics->content.location.x = region.location.x + pad_x;
    metrics->content.location.y = region.location.y + pad_y;
    metrics->content.size.width = region.size.width - pad_x * 2;
    metrics->content.size.height = region.size.height - pad_y * 2;

    metrics->header_region.location.x = metrics->content.location.x;
    metrics->header_region.location.y = metrics->content.location.y;
    metrics->header_region.size.width = metrics->content.size.width;
    metrics->header_region.size.height = header_h;

    metrics->footer_region.location.x = metrics->content.location.x;
    metrics->footer_region.location.y = metrics->content.location.y + metrics->content.size.height - footer_h;
    metrics->footer_region.size.width = metrics->content.size.width;
    metrics->footer_region.size.height = footer_h;

    metrics->list_region.location.x = metrics->content.location.x;
    metrics->list_region.location.y = metrics->header_region.location.y + metrics->header_region.size.height + header_gap;
    metrics->list_region.size.width = metrics->content.size.width;
    metrics->list_region.size.height = metrics->footer_region.location.y - list_gap - metrics->list_region.location.y;

    metrics->visible_item_count = egui_view_tree_view_clamp_item_count(item_count);
    if (metrics->visible_item_count == 0)
    {
        return;
    }

    if (metrics->list_region.size.height <= list_inset * 2)
    {
        list_inset = 1;
    }

    available_height = metrics->list_region.size.height - list_inset * 2;
    if (available_height < 0)
    {
        available_height = metrics->list_region.size.height;
        list_inset = 0;
    }

    required_rows_h = (egui_dim_t)metrics->visible_item_count * row_h + (egui_dim_t)(metrics->visible_item_count - 1) * row_gap;
    if (required_rows_h > available_height && metrics->visible_item_count > 0)
    {
        row_h = (available_height - (egui_dim_t)(metrics->visible_item_count - 1) * row_gap) / metrics->visible_item_count;
        if (row_h < min_row_h)
        {
            row_gap = 0;
            row_h = available_height / metrics->visible_item_count;
        }
        if (row_h < min_row_h && available_height >= (egui_dim_t)metrics->visible_item_count * min_row_h)
        {
            row_h = min_row_h;
        }
    }

    required_rows_h = (egui_dim_t)metrics->visible_item_count * row_h + (egui_dim_t)(metrics->visible_item_count - 1) * row_gap;
    item_y = metrics->list_region.location.y + list_inset;
    if (required_rows_h < metrics->list_region.size.height)
    {
        item_y = metrics->list_region.location.y + (metrics->list_region.size.height - required_rows_h) / 2;
    }
    for (i = 0; i < metrics->visible_item_count; ++i)
    {
        metrics->item_regions[i].location.x = metrics->list_region.location.x + (local->compact_mode ? 3 : 4);
        metrics->item_regions[i].location.y = item_y;
        metrics->item_regions[i].size.width = metrics->list_region.size.width - (local->compact_mode ? 6 : 8);
        metrics->item_regions[i].size.height = row_h;
        item_y += row_h + row_gap;
    }
}

static uint8_t egui_view_tree_view_has_continuation(const egui_view_tree_view_snapshot_t *snapshot, uint8_t item_count, uint8_t index, uint8_t level)
{
    uint8_t i;

    if (snapshot == NULL || level == 0 || index >= item_count)
    {
        return 0;
    }

    for (i = index + 1; i < item_count; ++i)
    {
        uint8_t next_level = snapshot->items[i].depth + 1;

        if (next_level < level)
        {
            return 0;
        }
        if (next_level == level)
        {
            return 1;
        }
    }

    return 0;
}

static uint8_t egui_view_tree_view_hit_item(egui_view_tree_view_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_tree_view_metrics_t metrics;
    const egui_view_tree_view_snapshot_t *snapshot = egui_view_tree_view_get_snapshot(local);
    uint8_t item_count;
    uint8_t i;

    if (snapshot == NULL)
    {
        return EGUI_VIEW_TREE_VIEW_INDEX_NONE;
    }

    item_count = egui_view_tree_view_clamp_item_count(snapshot->item_count);
    egui_view_tree_view_get_metrics(local, self, item_count, &metrics);
    for (i = 0; i < metrics.visible_item_count; ++i)
    {
        if (egui_region_pt_in_rect(&metrics.item_regions[i], x, y))
        {
            return i;
        }
    }

    return EGUI_VIEW_TREE_VIEW_INDEX_NONE;
}

static void egui_view_tree_view_set_current_index_inner(egui_view_t *self, uint8_t index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_tree_view_t);
    const egui_view_tree_view_snapshot_t *snapshot = egui_view_tree_view_get_snapshot(local);
    uint8_t item_count;

    if (snapshot == NULL)
    {
        local->current_index = 0;
        return;
    }

    item_count = egui_view_tree_view_clamp_item_count(snapshot->item_count);
    if (item_count == 0)
    {
        local->current_index = 0;
        return;
    }

    if (index >= item_count)
    {
        index = item_count - 1;
    }
    if (local->current_index == index)
    {
        return;
    }

    local->current_index = index;
    if (notify && local->on_selection_changed != NULL)
    {
        local->on_selection_changed(self, index);
    }
    egui_view_invalidate(self);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static void egui_view_tree_view_move_focus(egui_view_t *self, int8_t step)
{
    EGUI_LOCAL_INIT(egui_view_tree_view_t);
    const egui_view_tree_view_snapshot_t *snapshot = egui_view_tree_view_get_snapshot(local);
    uint8_t item_count;
    int16_t next_index;

    if (snapshot == NULL)
    {
        return;
    }

    item_count = egui_view_tree_view_clamp_item_count(snapshot->item_count);
    if (item_count == 0)
    {
        return;
    }

    next_index = local->current_index;
    if (next_index >= item_count)
    {
        next_index = egui_view_tree_view_focus_index(snapshot, item_count);
    }
    next_index += step;
    if (next_index < 0)
    {
        next_index = 0;
    }
    if (next_index >= item_count)
    {
        next_index = item_count - 1;
    }

    egui_view_tree_view_set_current_index_inner(self, (uint8_t)next_index, 1);
}
#endif

void egui_view_tree_view_set_snapshots(egui_view_t *self, const egui_view_tree_view_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_tree_view_t);
    const egui_view_tree_view_snapshot_t *snapshot;
    uint8_t item_count;

    local->snapshots = snapshots;
    local->snapshot_count = egui_view_tree_view_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }

    snapshot = egui_view_tree_view_get_snapshot(local);
    item_count = snapshot == NULL ? 0 : egui_view_tree_view_clamp_item_count(snapshot->item_count);
    local->current_index = egui_view_tree_view_focus_index(snapshot, item_count);
    local->pressed_index = EGUI_VIEW_TREE_VIEW_INDEX_NONE;
    egui_view_invalidate(self);
}

void egui_view_tree_view_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_tree_view_t);
    const egui_view_tree_view_snapshot_t *snapshot;
    uint8_t item_count;

    if (local->snapshot_count == 0 || snapshot_index >= local->snapshot_count)
    {
        return;
    }
    if (local->current_snapshot == snapshot_index)
    {
        return;
    }

    local->current_snapshot = snapshot_index;
    snapshot = egui_view_tree_view_get_snapshot(local);
    item_count = snapshot == NULL ? 0 : egui_view_tree_view_clamp_item_count(snapshot->item_count);
    local->current_index = egui_view_tree_view_focus_index(snapshot, item_count);
    local->pressed_index = EGUI_VIEW_TREE_VIEW_INDEX_NONE;
    egui_view_invalidate(self);
}

uint8_t egui_view_tree_view_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_tree_view_t);
    return local->current_snapshot;
}

void egui_view_tree_view_set_current_index(egui_view_t *self, uint8_t index)
{
    egui_view_tree_view_set_current_index_inner(self, index, 1);
}

uint8_t egui_view_tree_view_get_current_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_tree_view_t);
    return local->current_index;
}

void egui_view_tree_view_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_tree_view_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_tree_view_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_tree_view_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_tree_view_set_on_selection_changed_listener(egui_view_t *self, egui_view_on_tree_view_selection_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_tree_view_t);
    local->on_selection_changed = listener;
}

void egui_view_tree_view_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_tree_view_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_tree_view_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_tree_view_t);
    local->locked_mode = locked_mode ? 1 : 0;
    local->pressed_index = EGUI_VIEW_TREE_VIEW_INDEX_NONE;
    egui_view_invalidate(self);
}

void egui_view_tree_view_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t section_color, egui_color_t border_color,
                                     egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color,
                                     egui_color_t warning_color, egui_color_t neutral_color)
{
    EGUI_LOCAL_INIT(egui_view_tree_view_t);
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
static void egui_view_tree_view_draw_row(egui_view_t *self, egui_view_tree_view_t *local, const egui_view_tree_view_snapshot_t *snapshot, uint8_t item_count,
                                         const egui_view_tree_view_item_t *item, uint8_t index, const egui_region_t *item_region, egui_color_t list_fill,
                                         egui_color_t list_border, egui_color_t text_color, egui_color_t muted_text_color)
{
    egui_region_t text_region;
    egui_color_t tone_color = egui_view_tree_view_tone_color(local, item->tone);
    egui_color_t guide_color = egui_rgb_mix(list_border, tone_color, 10);
    egui_color_t row_fill =
            egui_rgb_mix(list_fill, tone_color, index == local->current_index ? (local->compact_mode ? 12 : 14) : (index == local->pressed_index ? 8 : 0));
    egui_color_t row_border = egui_rgb_mix(list_border, tone_color, index == local->current_index ? 24 : 10);
    egui_color_t caret_color = index == local->current_index ? tone_color : egui_rgb_mix(muted_text_color, tone_color, 10);
    egui_color_t glyph_fill = egui_rgb_mix(local->surface_color, tone_color, item->kind == EGUI_VIEW_TREE_VIEW_KIND_FOLDER ? 14 : 10);
    egui_color_t glyph_border = egui_rgb_mix(list_border, tone_color, item->kind == EGUI_VIEW_TREE_VIEW_KIND_FOLDER ? 20 : 12);
    egui_color_t title_color =
            item->kind == EGUI_VIEW_TREE_VIEW_KIND_FOLDER ? egui_rgb_mix(text_color, tone_color, 6) : egui_rgb_mix(text_color, muted_text_color, 18);
    egui_color_t meta_fill = egui_rgb_mix(local->surface_color, tone_color, index == local->current_index ? 16 : 10);
    egui_color_t meta_border = egui_rgb_mix(list_border, tone_color, index == local->current_index ? 24 : 16);
    egui_color_t meta_color = index == local->current_index ? tone_color : egui_rgb_mix(muted_text_color, tone_color, 16);
    egui_dim_t indent = local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_INDENT : EGUI_VIEW_TREE_VIEW_STANDARD_INDENT;
    egui_dim_t caret_box_w = local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_CARET_BOX_WIDTH : EGUI_VIEW_TREE_VIEW_STANDARD_CARET_BOX_WIDTH;
    egui_dim_t caret_size = local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_CARET_SIZE : EGUI_VIEW_TREE_VIEW_STANDARD_CARET_SIZE;
    egui_dim_t glyph_w = local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_GLYPH_WIDTH : EGUI_VIEW_TREE_VIEW_STANDARD_GLYPH_WIDTH;
    egui_dim_t glyph_h = local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_GLYPH_HEIGHT : EGUI_VIEW_TREE_VIEW_STANDARD_GLYPH_HEIGHT;
    egui_dim_t glyph_radius = local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_GLYPH_RADIUS : EGUI_VIEW_TREE_VIEW_STANDARD_GLYPH_RADIUS;
    egui_dim_t meta_h = local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_META_HEIGHT : EGUI_VIEW_TREE_VIEW_STANDARD_META_HEIGHT;
    egui_dim_t branch_base_x = item_region->location.x + (local->compact_mode ? 8 : 10);
    egui_dim_t caret_x = branch_base_x + item->depth * indent;
    egui_dim_t caret_y = item_region->location.y + (item_region->size.height - caret_size) / 2;
    egui_dim_t glyph_x = caret_x + caret_box_w + 3;
    egui_dim_t glyph_y = item_region->location.y + (item_region->size.height - glyph_h) / 2;
    egui_dim_t title_x = glyph_x + glyph_w + 6;
    egui_dim_t meta_w = egui_view_tree_view_meta_width(item->meta, local->compact_mode, item_region->size.width / 3);
    egui_dim_t meta_x = item_region->location.x + item_region->size.width - meta_w - 6;
    egui_dim_t mid_y = item_region->location.y + item_region->size.height / 2;
    egui_alpha_t guide_alpha = local->compact_mode ? 22 : 34;
    egui_dim_t guide_tail_w = local->compact_mode ? 3 : 6;
    uint8_t selected = index == local->current_index;
    uint8_t pressed = index == local->pressed_index;
    uint8_t level;

    if (local->locked_mode)
    {
        guide_color = egui_rgb_mix(guide_color, muted_text_color, 18);
        row_fill = egui_rgb_mix(row_fill, list_fill, 26);
        row_border = egui_rgb_mix(row_border, muted_text_color, 18);
        caret_color = egui_rgb_mix(caret_color, muted_text_color, 28);
        glyph_fill = egui_rgb_mix(glyph_fill, list_fill, 24);
        glyph_border = egui_rgb_mix(glyph_border, muted_text_color, 22);
        title_color = egui_rgb_mix(title_color, muted_text_color, 20);
        meta_fill = egui_rgb_mix(meta_fill, list_fill, 24);
        meta_border = egui_rgb_mix(meta_border, muted_text_color, 20);
        meta_color = egui_rgb_mix(meta_color, muted_text_color, 28);
    }

    if (!egui_view_get_enable(self))
    {
        guide_color = egui_view_tree_view_mix_disabled(guide_color);
        row_fill = egui_view_tree_view_mix_disabled(row_fill);
        row_border = egui_view_tree_view_mix_disabled(row_border);
        caret_color = egui_view_tree_view_mix_disabled(caret_color);
        glyph_fill = egui_view_tree_view_mix_disabled(glyph_fill);
        glyph_border = egui_view_tree_view_mix_disabled(glyph_border);
        title_color = egui_view_tree_view_mix_disabled(title_color);
        meta_fill = egui_view_tree_view_mix_disabled(meta_fill);
        meta_border = egui_view_tree_view_mix_disabled(meta_border);
        meta_color = egui_view_tree_view_mix_disabled(meta_color);
    }

    if (selected || pressed)
    {
        egui_canvas_draw_round_rectangle_fill(item_region->location.x, item_region->location.y, item_region->size.width, item_region->size.height,
                                              local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_ROW_RADIUS : EGUI_VIEW_TREE_VIEW_STANDARD_ROW_RADIUS, row_fill,
                                              egui_color_alpha_mix(self->alpha, selected ? 90 : 62));
        egui_canvas_draw_round_rectangle(item_region->location.x, item_region->location.y, item_region->size.width, item_region->size.height,
                                         local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_ROW_RADIUS : EGUI_VIEW_TREE_VIEW_STANDARD_ROW_RADIUS, 1, row_border,
                                         egui_color_alpha_mix(self->alpha, selected ? 48 : 28));
    }

    if (selected)
    {
        egui_dim_t indicator_w = local->compact_mode ? 2 : 3;

        egui_canvas_draw_round_rectangle_fill(item_region->location.x + 2, item_region->location.y + 2, indicator_w, item_region->size.height - 4, 1,
                                              tone_color, egui_color_alpha_mix(self->alpha, 94));
    }

    for (level = 1; level < (uint8_t)(item->depth + 1); ++level)
    {
        egui_dim_t guide_x = branch_base_x + (level - 1) * indent;
        uint8_t continuation = egui_view_tree_view_has_continuation(snapshot, item_count, index, level);

        egui_canvas_draw_line(guide_x, item_region->location.y - 1, guide_x, level == item->depth ? mid_y : item_region->location.y + item_region->size.height,
                              1, guide_color, egui_color_alpha_mix(self->alpha, guide_alpha));
        if (level == item->depth)
        {
            egui_canvas_draw_line(guide_x, mid_y, guide_x + guide_tail_w, mid_y, 1, guide_color, egui_color_alpha_mix(self->alpha, guide_alpha));
        }
        if (continuation)
        {
            egui_canvas_draw_line(guide_x, mid_y, guide_x, item_region->location.y + item_region->size.height + 1, 1, guide_color,
                                  egui_color_alpha_mix(self->alpha, guide_alpha));
        }
    }

    if (item->has_children)
    {
        egui_view_tree_view_draw_caret(self, caret_x, caret_y, caret_size, item->expanded ? 1 : 0, caret_color, selected ? 100 : 84);
    }

    if (item->kind == EGUI_VIEW_TREE_VIEW_KIND_FOLDER)
    {
        egui_view_tree_view_draw_folder_glyph(self, glyph_x, glyph_y, glyph_w, glyph_h, glyph_fill, glyph_border, 96, glyph_radius);
    }
    else
    {
        egui_view_tree_view_draw_leaf_glyph(self, glyph_x, glyph_y, glyph_w, glyph_h, glyph_fill, glyph_border, 92, glyph_radius);
    }

    if (meta_w > 0)
    {
        egui_canvas_draw_round_rectangle_fill(meta_x, item_region->location.y + (item_region->size.height - meta_h) / 2, meta_w, meta_h, meta_h / 2, meta_fill,
                                              egui_color_alpha_mix(self->alpha, 94));
        egui_canvas_draw_round_rectangle(meta_x, item_region->location.y + (item_region->size.height - meta_h) / 2, meta_w, meta_h, meta_h / 2, 1, meta_border,
                                         egui_color_alpha_mix(self->alpha, 40));

        text_region.location.x = meta_x;
        text_region.location.y = item_region->location.y + (item_region->size.height - meta_h) / 2;
        text_region.size.width = meta_w;
        text_region.size.height = meta_h;
        egui_view_tree_view_draw_text(local->meta_font, self, item->meta, &text_region, EGUI_ALIGN_CENTER, meta_color);
    }

    text_region.location.x = title_x;
    text_region.location.y = item_region->location.y;
    text_region.size.width = (meta_w > 0 ? meta_x : (item_region->location.x + item_region->size.width - 6)) - title_x - 4;
    text_region.size.height = item_region->size.height;
    egui_view_tree_view_draw_text(local->font, self, item->title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, selected ? text_color : title_color);
}

static void egui_view_tree_view_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_tree_view_t);
    egui_region_t region;
    egui_region_t text_region;
    egui_view_tree_view_metrics_t metrics;
    const egui_view_tree_view_snapshot_t *snapshot;
    const egui_view_tree_view_item_t *focus_item;
    egui_color_t focus_color;
    egui_color_t card_fill;
    egui_color_t card_border;
    egui_color_t list_fill;
    egui_color_t list_border;
    egui_color_t title_color;
    egui_color_t muted_text_color;
    egui_color_t caption_fill;
    egui_color_t caption_border;
    egui_color_t caption_color;
    egui_color_t footer_fill;
    egui_color_t footer_border;
    egui_color_t footer_color;
    egui_dim_t radius = local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_RADIUS : EGUI_VIEW_TREE_VIEW_STANDARD_RADIUS;
    egui_dim_t caption_w;
    uint8_t item_count;
    uint8_t current_index;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0)
    {
        return;
    }

    snapshot = egui_view_tree_view_get_snapshot(local);
    if (snapshot == NULL)
    {
        return;
    }

    item_count = egui_view_tree_view_clamp_item_count(snapshot->item_count);
    if (item_count == 0)
    {
        return;
    }

    current_index = local->current_index >= item_count ? egui_view_tree_view_focus_index(snapshot, item_count) : local->current_index;
    local->current_index = current_index;
    focus_item = &snapshot->items[current_index];
    focus_color = egui_view_tree_view_tone_color(local, focus_item->tone);
    card_fill = egui_rgb_mix(local->surface_color, focus_color, local->compact_mode ? 4 : 6);
    card_border = egui_rgb_mix(local->border_color, focus_color, local->compact_mode ? 12 : 14);
    list_fill = egui_rgb_mix(local->section_color, focus_color, local->compact_mode ? 2 : 4);
    list_border = egui_rgb_mix(local->border_color, focus_color, local->compact_mode ? 8 : 12);
    title_color = local->text_color;
    muted_text_color = local->muted_text_color;
    caption_fill = egui_rgb_mix(local->surface_color, focus_color, local->compact_mode ? 12 : 16);
    caption_border = egui_rgb_mix(local->border_color, focus_color, local->compact_mode ? 18 : 22);
    caption_color = focus_color;
    footer_fill = egui_rgb_mix(local->surface_color, focus_color, local->compact_mode ? 8 : 10);
    footer_border = egui_rgb_mix(local->border_color, focus_color, local->compact_mode ? 14 : 18);
    footer_color = egui_rgb_mix(local->muted_text_color, focus_color, local->compact_mode ? 16 : 22);

    if (local->locked_mode)
    {
        focus_color = egui_rgb_mix(focus_color, muted_text_color, 76);
        card_fill = egui_rgb_mix(card_fill, local->surface_color, 18);
        card_border = egui_rgb_mix(card_border, muted_text_color, 18);
        list_fill = egui_rgb_mix(list_fill, local->surface_color, 16);
        list_border = egui_rgb_mix(list_border, muted_text_color, 18);
        title_color = egui_rgb_mix(title_color, muted_text_color, 18);
        muted_text_color = egui_rgb_mix(muted_text_color, title_color, 4);
        caption_fill = egui_rgb_mix(caption_fill, local->surface_color, 20);
        caption_border = egui_rgb_mix(caption_border, muted_text_color, 20);
        caption_color = egui_rgb_mix(caption_color, muted_text_color, 30);
        footer_fill = egui_rgb_mix(footer_fill, local->surface_color, 22);
        footer_border = egui_rgb_mix(footer_border, muted_text_color, 20);
        footer_color = egui_rgb_mix(footer_color, muted_text_color, 24);
    }

    if (!egui_view_get_enable(self))
    {
        focus_color = egui_view_tree_view_mix_disabled(focus_color);
        card_fill = egui_view_tree_view_mix_disabled(card_fill);
        card_border = egui_view_tree_view_mix_disabled(card_border);
        list_fill = egui_view_tree_view_mix_disabled(list_fill);
        list_border = egui_view_tree_view_mix_disabled(list_border);
        title_color = egui_view_tree_view_mix_disabled(title_color);
        muted_text_color = egui_view_tree_view_mix_disabled(muted_text_color);
        caption_fill = egui_view_tree_view_mix_disabled(caption_fill);
        caption_border = egui_view_tree_view_mix_disabled(caption_border);
        caption_color = egui_view_tree_view_mix_disabled(caption_color);
        footer_fill = egui_view_tree_view_mix_disabled(footer_fill);
        footer_border = egui_view_tree_view_mix_disabled(footer_border);
        footer_color = egui_view_tree_view_mix_disabled(footer_color);
    }

    egui_view_tree_view_get_metrics(local, self, item_count, &metrics);

    egui_canvas_draw_round_rectangle_fill(
            region.location.x, region.location.y, region.size.width, region.size.height, radius, card_fill,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_FILL_ALPHA : EGUI_VIEW_TREE_VIEW_STANDARD_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(
            region.location.x, region.location.y, region.size.width, region.size.height, radius, 1, card_border,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_BORDER_ALPHA : EGUI_VIEW_TREE_VIEW_STANDARD_BORDER_ALPHA));
    egui_canvas_draw_round_rectangle_fill(region.location.x + 2, region.location.y + 2, region.size.width - 4, local->compact_mode ? 3 : 4, radius, focus_color,
                                          egui_color_alpha_mix(self->alpha, local->locked_mode ? 12 : 22));

    caption_w = egui_view_tree_view_caption_width(snapshot->caption, local->compact_mode, metrics.header_region.size.width / 2);
    if (snapshot->caption != NULL && snapshot->caption[0] != '\0')
    {
        egui_dim_t caption_h = local->compact_mode ? EGUI_VIEW_TREE_VIEW_COMPACT_META_HEIGHT + 2 : EGUI_VIEW_TREE_VIEW_STANDARD_META_HEIGHT + 2;

        egui_canvas_draw_round_rectangle_fill(metrics.header_region.location.x + metrics.header_region.size.width - caption_w, metrics.header_region.location.y,
                                              caption_w, caption_h, caption_h / 2, caption_fill, egui_color_alpha_mix(self->alpha, 94));
        egui_canvas_draw_round_rectangle(metrics.header_region.location.x + metrics.header_region.size.width - caption_w, metrics.header_region.location.y,
                                         caption_w, caption_h, caption_h / 2, 1, caption_border,
                                         egui_color_alpha_mix(self->alpha, local->compact_mode ? 40 : 44));

        text_region.location.x = metrics.header_region.location.x + metrics.header_region.size.width - caption_w;
        text_region.location.y = metrics.header_region.location.y;
        text_region.size.width = caption_w;
        text_region.size.height = caption_h;
        egui_view_tree_view_draw_text(local->meta_font, self, snapshot->caption, &text_region, EGUI_ALIGN_CENTER, caption_color);
    }
    else
    {
        caption_w = 0;
    }

    text_region.location.x = metrics.header_region.location.x;
    text_region.location.y = metrics.header_region.location.y;
    text_region.size.width = metrics.header_region.size.width - (caption_w > 0 ? caption_w + 6 : 0);
    text_region.size.height = metrics.header_region.size.height;
    egui_view_tree_view_draw_text(local->font, self, snapshot->title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color);

    egui_canvas_draw_round_rectangle_fill(metrics.list_region.location.x, metrics.list_region.location.y, metrics.list_region.size.width,
                                          metrics.list_region.size.height, local->compact_mode ? 6 : 7, list_fill, egui_color_alpha_mix(self->alpha, 90));
    egui_canvas_draw_round_rectangle(metrics.list_region.location.x, metrics.list_region.location.y, metrics.list_region.size.width,
                                     metrics.list_region.size.height, local->compact_mode ? 6 : 7, 1, list_border, egui_color_alpha_mix(self->alpha, 30));

    for (i = 0; i < metrics.visible_item_count; ++i)
    {
        egui_view_tree_view_draw_row(self, local, snapshot, item_count, &snapshot->items[i], i, &metrics.item_regions[i], list_fill, list_border, title_color,
                                     muted_text_color);
    }

    egui_canvas_draw_round_rectangle_fill(metrics.footer_region.location.x, metrics.footer_region.location.y, metrics.footer_region.size.width,
                                          metrics.footer_region.size.height, metrics.footer_region.size.height / 2, footer_fill,
                                          egui_color_alpha_mix(self->alpha, local->compact_mode ? 26 : 30));
    egui_canvas_draw_round_rectangle(metrics.footer_region.location.x, metrics.footer_region.location.y, metrics.footer_region.size.width,
                                     metrics.footer_region.size.height, metrics.footer_region.size.height / 2, 1, footer_border,
                                     egui_color_alpha_mix(self->alpha, local->compact_mode ? 28 : 34));

    text_region.location.x = metrics.footer_region.location.x + (local->compact_mode ? 4 : 6);
    text_region.location.y = metrics.footer_region.location.y;
    text_region.size.width = metrics.footer_region.size.width - (local->compact_mode ? 8 : 12);
    text_region.size.height = metrics.footer_region.size.height;
    egui_view_tree_view_draw_text(local->meta_font, self, snapshot->footer, &text_region,
                                  (local->compact_mode ? EGUI_ALIGN_CENTER : EGUI_ALIGN_LEFT) | EGUI_ALIGN_VCENTER, footer_color);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_tree_view_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_tree_view_t);
    uint8_t hit_index;

    if (!egui_view_get_enable(self) || local->locked_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_index = egui_view_tree_view_hit_item(local, self, event->location.x, event->location.y);
        if (hit_index == EGUI_VIEW_TREE_VIEW_INDEX_NONE)
        {
            return 0;
        }
        local->pressed_index = hit_index;
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_index = egui_view_tree_view_hit_item(local, self, event->location.x, event->location.y);
        if (local->pressed_index != EGUI_VIEW_TREE_VIEW_INDEX_NONE && local->pressed_index == hit_index)
        {
            egui_view_tree_view_set_current_index_inner(self, hit_index, 1);
        }
        local->pressed_index = EGUI_VIEW_TREE_VIEW_INDEX_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_index != EGUI_VIEW_TREE_VIEW_INDEX_NONE;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_index = EGUI_VIEW_TREE_VIEW_INDEX_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_tree_view_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_tree_view_t);

    if (!egui_view_get_enable(self) || event->type != EGUI_KEY_EVENT_ACTION_UP || local->locked_mode)
    {
        return 0;
    }

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_UP:
        egui_view_tree_view_move_focus(self, -1);
        return 1;
    case EGUI_KEY_CODE_DOWN:
        egui_view_tree_view_move_focus(self, 1);
        return 1;
    case EGUI_KEY_CODE_HOME:
        egui_view_tree_view_set_current_index_inner(self, 0, 1);
        return 1;
    case EGUI_KEY_CODE_END:
    {
        const egui_view_tree_view_snapshot_t *snapshot = egui_view_tree_view_get_snapshot(local);
        uint8_t item_count = snapshot == NULL ? 0 : egui_view_tree_view_clamp_item_count(snapshot->item_count);

        if (item_count > 0)
        {
            egui_view_tree_view_set_current_index_inner(self, item_count - 1, 1);
        }
        return 1;
    }
    default:
        return egui_view_on_key_event(self, event);
    }
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_tree_view_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_tree_view_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_tree_view_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_tree_view_on_key_event,
#endif
};

void egui_view_tree_view_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_tree_view_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_tree_view_t);
    egui_view_set_padding_all(self, 2);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(self, true);
#endif

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->on_selection_changed = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->section_color = EGUI_COLOR_HEX(0xF8FAFC);
    local->border_color = EGUI_COLOR_HEX(0xD7DFE7);
    local->text_color = EGUI_COLOR_HEX(0x18222D);
    local->muted_text_color = EGUI_COLOR_HEX(0x667688);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->success_color = EGUI_COLOR_HEX(0x178454);
    local->warning_color = EGUI_COLOR_HEX(0xB77719);
    local->neutral_color = EGUI_COLOR_HEX(0x7A8796);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->current_index = 0;
    local->compact_mode = 0;
    local->locked_mode = 0;
    local->pressed_index = EGUI_VIEW_TREE_VIEW_INDEX_NONE;

    egui_view_set_view_name(self, "egui_view_tree_view");
}
