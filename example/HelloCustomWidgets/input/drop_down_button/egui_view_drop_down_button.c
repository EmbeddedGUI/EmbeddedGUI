#include "egui_view_drop_down_button.h"

#define EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_RADIUS           10
#define EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_FILL_ALPHA       98
#define EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_BORDER_ALPHA     72
#define EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_CONTENT_PAD_X    10
#define EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_CONTENT_PAD_Y    7
#define EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_TITLE_HEIGHT     10
#define EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_TITLE_GAP        3
#define EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_ROW_HEIGHT       30
#define EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_HELPER_GAP       4
#define EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_HELPER_HEIGHT    10
#define EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_ROW_RADIUS       7
#define EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_ROW_FILL_ALPHA   100
#define EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_ROW_BORDER_ALPHA 62
#define EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_GLYPH_WIDTH      16
#define EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_GLYPH_HEIGHT     14
#define EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_HINT_MIN_WIDTH   18
#define EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_HINT_HEIGHT      18
#define EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_HINT_SIDE_PAD    6

#define EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_RADIUS           8
#define EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_FILL_ALPHA       96
#define EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_BORDER_ALPHA     70
#define EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_CONTENT_PAD_X    7
#define EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_CONTENT_PAD_Y    6
#define EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_ROW_HEIGHT       22
#define EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_ROW_RADIUS       5
#define EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_ROW_FILL_ALPHA   98
#define EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_ROW_BORDER_ALPHA 58
#define EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_HINT_MIN_WIDTH   14
#define EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_HINT_HEIGHT      14
#define EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_HINT_SIDE_PAD    4

typedef struct egui_view_drop_down_button_metrics egui_view_drop_down_button_metrics_t;
struct egui_view_drop_down_button_metrics
{
    egui_region_t content_region;
    egui_region_t title_region;
    egui_region_t row_region;
    egui_region_t glyph_region;
    egui_region_t label_region;
    egui_region_t hint_region;
    egui_region_t helper_region;
    uint8_t show_title;
    uint8_t show_helper;
    uint8_t show_glyph;
};

static uint8_t egui_view_drop_down_button_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_DROP_DOWN_BUTTON_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_DROP_DOWN_BUTTON_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_drop_down_button_text_len(const char *text)
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

static const egui_view_drop_down_button_snapshot_t *egui_view_drop_down_button_get_snapshot(egui_view_drop_down_button_t *local)
{
    if (local->snapshots == NULL || local->snapshot_count == 0 || local->current_snapshot >= local->snapshot_count)
    {
        return NULL;
    }

    return &local->snapshots[local->current_snapshot];
}

static egui_color_t egui_view_drop_down_button_tone_color(egui_view_drop_down_button_t *local, uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_DROP_DOWN_BUTTON_TONE_SUCCESS:
        return local->success_color;
    case EGUI_VIEW_DROP_DOWN_BUTTON_TONE_WARNING:
        return local->warning_color;
    case EGUI_VIEW_DROP_DOWN_BUTTON_TONE_DANGER:
        return local->danger_color;
    case EGUI_VIEW_DROP_DOWN_BUTTON_TONE_NEUTRAL:
        return local->neutral_color;
    default:
        return local->accent_color;
    }
}

static egui_color_t egui_view_drop_down_button_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 68);
}

static void egui_view_drop_down_button_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                                 egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (text == NULL || text[0] == '\0')
    {
        return;
    }

    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void egui_view_drop_down_button_draw_chevron(egui_view_t *self, const egui_region_t *region, egui_color_t color, egui_alpha_t alpha)
{
    egui_dim_t cx = region->location.x + region->size.width / 2;
    egui_dim_t cy = region->location.y + region->size.height / 2;
    egui_alpha_t mixed_alpha = egui_color_alpha_mix(self->alpha, alpha);

    egui_canvas_draw_line(cx - 3, cy - 1, cx, cy + 2, 1, color, mixed_alpha);
    egui_canvas_draw_line(cx, cy + 2, cx + 3, cy - 1, 1, color, mixed_alpha);
}

static egui_dim_t egui_view_drop_down_button_hint_width(egui_view_drop_down_button_t *local, const char *hint)
{
    egui_dim_t min_width = local->compact_mode ? EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_HINT_MIN_WIDTH : EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_HINT_MIN_WIDTH;
    egui_dim_t side_pad = local->compact_mode ? EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_HINT_SIDE_PAD : EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_HINT_SIDE_PAD;
    egui_dim_t text_width = egui_view_drop_down_button_text_len(hint) * (local->compact_mode ? 4 : 5);
    egui_dim_t width = side_pad * 2 + text_width;

    if (width < min_width)
    {
        width = min_width;
    }

    return width;
}

static void egui_view_drop_down_button_get_metrics(egui_view_drop_down_button_t *local, egui_view_t *self,
                                                   const egui_view_drop_down_button_snapshot_t *snapshot, egui_view_drop_down_button_metrics_t *metrics)
{
    egui_region_t region;
    egui_dim_t pad_x = local->compact_mode ? EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_CONTENT_PAD_X : EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_CONTENT_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_CONTENT_PAD_Y : EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_CONTENT_PAD_Y;
    egui_dim_t row_h = local->compact_mode ? EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_ROW_HEIGHT : EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_ROW_HEIGHT;
    egui_dim_t title_h = EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_TITLE_HEIGHT;
    egui_dim_t title_gap = EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_TITLE_GAP;
    egui_dim_t helper_h = EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_HELPER_HEIGHT;
    egui_dim_t helper_gap = EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_HELPER_GAP;
    egui_dim_t row_y;
    egui_dim_t hint_w;
    egui_dim_t hint_h = local->compact_mode ? EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_HINT_HEIGHT : EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_HINT_HEIGHT;

    egui_view_get_work_region(self, &region);
    metrics->content_region.location.x = region.location.x + pad_x;
    metrics->content_region.location.y = region.location.y + pad_y;
    metrics->content_region.size.width = region.size.width - pad_x * 2;
    metrics->content_region.size.height = region.size.height - pad_y * 2;

    metrics->show_title = (!local->compact_mode && snapshot != NULL && snapshot->title != NULL && snapshot->title[0] != '\0') ? 1 : 0;
    metrics->show_helper = (!local->compact_mode && snapshot != NULL && snapshot->helper != NULL && snapshot->helper[0] != '\0') ? 1 : 0;
    metrics->show_glyph = (!local->compact_mode && snapshot != NULL && snapshot->glyph != NULL && snapshot->glyph[0] != '\0') ? 1 : 0;

    metrics->title_region.location.x = metrics->content_region.location.x;
    metrics->title_region.location.y = metrics->content_region.location.y;
    metrics->title_region.size.width = metrics->content_region.size.width;
    metrics->title_region.size.height = title_h;

    if (metrics->show_title)
    {
        row_y = metrics->content_region.location.y + title_h + title_gap;
    }
    else
    {
        row_y = metrics->content_region.location.y;
    }

    if (!metrics->show_helper)
    {
        row_y = metrics->content_region.location.y + (metrics->content_region.size.height - row_h) / 2;
    }

    metrics->row_region.location.x = metrics->content_region.location.x;
    metrics->row_region.location.y = row_y;
    metrics->row_region.size.width = metrics->content_region.size.width;
    metrics->row_region.size.height = row_h;

    hint_w = egui_view_drop_down_button_hint_width(local, snapshot ? snapshot->hint : NULL);
    metrics->hint_region.size.width = hint_w;
    metrics->hint_region.size.height = hint_h;
    metrics->hint_region.location.x = metrics->row_region.location.x + metrics->row_region.size.width - hint_w - (local->compact_mode ? 4 : 6);
    metrics->hint_region.location.y = metrics->row_region.location.y + (metrics->row_region.size.height - hint_h) / 2;

    if (metrics->show_glyph)
    {
        metrics->glyph_region.location.x = metrics->row_region.location.x + 6;
        metrics->glyph_region.location.y =
                metrics->row_region.location.y + (metrics->row_region.size.height - EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_GLYPH_HEIGHT) / 2;
        metrics->glyph_region.size.width = EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_GLYPH_WIDTH;
        metrics->glyph_region.size.height = EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_GLYPH_HEIGHT;

        metrics->label_region.location.x = metrics->glyph_region.location.x + metrics->glyph_region.size.width + 7;
    }
    else
    {
        metrics->glyph_region.location.x = 0;
        metrics->glyph_region.location.y = 0;
        metrics->glyph_region.size.width = 0;
        metrics->glyph_region.size.height = 0;
        metrics->label_region.location.x = metrics->row_region.location.x + (local->compact_mode ? 7 : 9);
    }

    metrics->label_region.location.y = metrics->row_region.location.y;
    metrics->label_region.size.width = metrics->hint_region.location.x - metrics->label_region.location.x - (local->compact_mode ? 4 : 6);
    metrics->label_region.size.height = metrics->row_region.size.height;

    metrics->helper_region.location.x = metrics->content_region.location.x;
    metrics->helper_region.location.y = metrics->row_region.location.y + metrics->row_region.size.height + helper_gap;
    metrics->helper_region.size.width = metrics->content_region.size.width;
    metrics->helper_region.size.height = helper_h;
}

void egui_view_drop_down_button_set_snapshots(egui_view_t *self, const egui_view_drop_down_button_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_drop_down_button_t);

    local->snapshots = snapshots;
    local->snapshot_count = egui_view_drop_down_button_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }

    egui_view_invalidate(self);
}

void egui_view_drop_down_button_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_drop_down_button_t);

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

uint8_t egui_view_drop_down_button_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_drop_down_button_t);
    return local->current_snapshot;
}

void egui_view_drop_down_button_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_drop_down_button_t);

    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_drop_down_button_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_drop_down_button_t);

    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_drop_down_button_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_drop_down_button_t);

    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_drop_down_button_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_drop_down_button_t);

    local->read_only_mode = read_only_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_drop_down_button_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                            egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color, egui_color_t warning_color,
                                            egui_color_t danger_color, egui_color_t neutral_color)
{
    EGUI_LOCAL_INIT(egui_view_drop_down_button_t);

    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->success_color = success_color;
    local->warning_color = warning_color;
    local->danger_color = danger_color;
    local->neutral_color = neutral_color;
    egui_view_invalidate(self);
}

static void egui_view_drop_down_button_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_drop_down_button_t);
    egui_region_t region;
    egui_region_t text_region;
    egui_view_drop_down_button_metrics_t metrics;
    const egui_view_drop_down_button_snapshot_t *snapshot = egui_view_drop_down_button_get_snapshot(local);
    egui_color_t tone_color;
    egui_color_t card_fill;
    egui_color_t card_border;
    egui_color_t row_fill;
    egui_color_t row_border;
    egui_color_t title_color;
    egui_color_t label_color;
    egui_color_t helper_color;
    egui_color_t hint_fill;
    egui_color_t hint_border;
    egui_color_t hint_text;
    egui_color_t glyph_fill;
    egui_color_t glyph_border;
    egui_color_t glyph_inner_border;
    egui_color_t card_inner_border;
    egui_color_t row_inner_border;
    egui_color_t hint_inner_border;
    egui_color_t focus_ring_color;
    egui_color_t read_only_overlay_color = EGUI_COLOR_WHITE;
    egui_dim_t radius = local->compact_mode ? EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_RADIUS : EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_RADIUS;
    egui_dim_t row_radius = local->compact_mode ? EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_ROW_RADIUS : EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_ROW_RADIUS;
    egui_dim_t pressed_offset_y = 0;
    egui_alpha_t read_only_row_overlay_alpha = 0;
    egui_alpha_t read_only_hint_overlay_alpha = 0;
    uint8_t is_pressed = egui_view_get_pressed(self) ? 1 : 0;
    uint8_t is_focused = 0;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || snapshot == NULL)
    {
        return;
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    is_focused = self->is_focused ? 1 : 0;
#endif

    egui_view_drop_down_button_get_metrics(local, self, snapshot, &metrics);
    tone_color = egui_view_drop_down_button_tone_color(local, snapshot->tone);
    card_fill = egui_rgb_mix(local->surface_color, EGUI_COLOR_HEX(0xEEF3F7), local->compact_mode ? 20 : 12);
    card_border = egui_rgb_mix(local->border_color, tone_color, snapshot->emphasized ? 34 : 18);
    row_fill = egui_rgb_mix(EGUI_COLOR_HEX(0xFFFFFF), tone_color, snapshot->emphasized ? 22 : 12);
    row_border = egui_rgb_mix(local->border_color, tone_color, snapshot->emphasized ? 38 : 24);
    title_color = egui_rgb_mix(local->muted_text_color, tone_color, 16);
    label_color = snapshot->emphasized ? tone_color : local->text_color;
    helper_color = egui_rgb_mix(local->muted_text_color, tone_color, 8);
    hint_fill = egui_rgb_mix(EGUI_COLOR_HEX(0xF9FBFE), tone_color, snapshot->emphasized ? 24 : 12);
    hint_border = egui_rgb_mix(local->border_color, tone_color, snapshot->emphasized ? 38 : 24);
    hint_text = snapshot->emphasized ? tone_color : egui_rgb_mix(local->text_color, tone_color, 14);
    glyph_fill = egui_rgb_mix(local->surface_color, tone_color, snapshot->emphasized ? 26 : 18);
    glyph_border = egui_rgb_mix(local->border_color, tone_color, snapshot->emphasized ? 28 : 16);
    glyph_inner_border = egui_rgb_mix(glyph_border, EGUI_COLOR_WHITE, 18);
    card_inner_border = egui_rgb_mix(card_border, EGUI_COLOR_WHITE, 20);
    row_inner_border = egui_rgb_mix(row_border, EGUI_COLOR_WHITE, 16);
    hint_inner_border = egui_rgb_mix(hint_border, EGUI_COLOR_WHITE, 12);
    focus_ring_color = egui_rgb_mix(tone_color, EGUI_COLOR_WHITE, 10);

    if (is_pressed)
    {
        pressed_offset_y = 1;
        card_fill = egui_rgb_mix(card_fill, tone_color, 18);
        card_border = egui_rgb_mix(card_border, tone_color, 30);
        row_fill = egui_rgb_mix(row_fill, tone_color, 34);
        row_border = egui_rgb_mix(row_border, tone_color, 36);
        hint_fill = egui_rgb_mix(hint_fill, tone_color, 34);
        hint_border = egui_rgb_mix(hint_border, tone_color, 30);
        glyph_fill = egui_rgb_mix(glyph_fill, tone_color, 24);
        glyph_border = egui_rgb_mix(glyph_border, tone_color, 26);
        glyph_inner_border = egui_rgb_mix(glyph_inner_border, tone_color, 20);
        card_inner_border = egui_rgb_mix(card_inner_border, tone_color, 18);
        row_inner_border = egui_rgb_mix(row_inner_border, tone_color, 26);
        hint_inner_border = egui_rgb_mix(hint_inner_border, tone_color, 24);
    }

    if (is_focused)
    {
        card_border = egui_rgb_mix(card_border, tone_color, 36);
        row_border = egui_rgb_mix(row_border, tone_color, 28);
        hint_border = egui_rgb_mix(hint_border, tone_color, 26);
        title_color = egui_rgb_mix(title_color, tone_color, 18);
        glyph_border = egui_rgb_mix(glyph_border, tone_color, 18);
        glyph_inner_border = egui_rgb_mix(glyph_inner_border, tone_color, 16);
        card_inner_border = egui_rgb_mix(card_inner_border, tone_color, 24);
        row_inner_border = egui_rgb_mix(row_inner_border, tone_color, 22);
        hint_inner_border = egui_rgb_mix(hint_inner_border, tone_color, 22);
    }

    if (local->compact_mode && !local->read_only_mode)
    {
        card_fill = egui_rgb_mix(card_fill, tone_color, snapshot->emphasized ? 8 : 4);
        card_border = egui_rgb_mix(card_border, tone_color, snapshot->emphasized ? 8 : 4);
        row_fill = egui_rgb_mix(row_fill, tone_color, snapshot->emphasized ? 8 : 4);
        row_border = egui_rgb_mix(row_border, tone_color, snapshot->emphasized ? 6 : 4);
        hint_fill = egui_rgb_mix(hint_fill, tone_color, snapshot->emphasized ? 6 : 3);
    }

    if (local->read_only_mode)
    {
        card_fill = egui_rgb_mix(card_fill, EGUI_COLOR_HEX(0xFCFDFE), 42);
        card_border = egui_rgb_mix(card_border, local->muted_text_color, 30);
        row_fill = egui_rgb_mix(row_fill, card_fill, 52);
        row_border = egui_rgb_mix(row_border, local->muted_text_color, 36);
        title_color = egui_rgb_mix(title_color, local->muted_text_color, 28);
        label_color = egui_rgb_mix(label_color, local->muted_text_color, 46);
        helper_color = egui_rgb_mix(helper_color, local->muted_text_color, 40);
        hint_fill = egui_rgb_mix(hint_fill, card_fill, 60);
        hint_border = egui_rgb_mix(hint_border, local->muted_text_color, 40);
        hint_text = egui_rgb_mix(hint_text, local->muted_text_color, 54);
        glyph_fill = egui_rgb_mix(glyph_fill, card_fill, 44);
        glyph_border = egui_rgb_mix(glyph_border, local->muted_text_color, 36);
        glyph_inner_border = egui_rgb_mix(glyph_inner_border, card_fill, 28);
        card_inner_border = egui_rgb_mix(card_inner_border, local->muted_text_color, 34);
        row_inner_border = egui_rgb_mix(row_inner_border, local->muted_text_color, 36);
        hint_inner_border = egui_rgb_mix(hint_inner_border, local->muted_text_color, 40);
        read_only_overlay_color = egui_rgb_mix(card_fill, EGUI_COLOR_WHITE, 64);
        read_only_row_overlay_alpha = local->compact_mode ? 18 : 14;
        read_only_hint_overlay_alpha = local->compact_mode ? 24 : 18;
    }

    if (!egui_view_get_enable(self))
    {
        card_fill = egui_view_drop_down_button_mix_disabled(card_fill);
        card_border = egui_view_drop_down_button_mix_disabled(card_border);
        row_fill = egui_view_drop_down_button_mix_disabled(row_fill);
        row_border = egui_view_drop_down_button_mix_disabled(row_border);
        title_color = egui_view_drop_down_button_mix_disabled(title_color);
        label_color = egui_view_drop_down_button_mix_disabled(label_color);
        helper_color = egui_view_drop_down_button_mix_disabled(helper_color);
        hint_fill = egui_view_drop_down_button_mix_disabled(hint_fill);
        hint_border = egui_view_drop_down_button_mix_disabled(hint_border);
        hint_text = egui_view_drop_down_button_mix_disabled(hint_text);
        glyph_fill = egui_view_drop_down_button_mix_disabled(glyph_fill);
        glyph_border = egui_view_drop_down_button_mix_disabled(glyph_border);
        glyph_inner_border = egui_view_drop_down_button_mix_disabled(glyph_inner_border);
        card_inner_border = egui_view_drop_down_button_mix_disabled(card_inner_border);
        row_inner_border = egui_view_drop_down_button_mix_disabled(row_inner_border);
        hint_inner_border = egui_view_drop_down_button_mix_disabled(hint_inner_border);
    }

    if (pressed_offset_y)
    {
        metrics.row_region.location.y += pressed_offset_y;
        metrics.hint_region.location.y += pressed_offset_y;
        metrics.glyph_region.location.y += pressed_offset_y;
        metrics.label_region.location.y += pressed_offset_y;
    }

    egui_canvas_draw_round_rectangle_fill(metrics.content_region.location.x - 2, metrics.content_region.location.y - 2, metrics.content_region.size.width + 4,
                                          metrics.content_region.size.height + 4, radius, card_fill,
                                          egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_FILL_ALPHA
                                                                                                : EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(metrics.content_region.location.x - 2, metrics.content_region.location.y - 2, metrics.content_region.size.width + 4,
                                     metrics.content_region.size.height + 4, radius, 1, card_border,
                                     egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_BORDER_ALPHA
                                                                                           : EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_BORDER_ALPHA));
    if (is_focused)
    {
        egui_canvas_draw_round_rectangle(metrics.content_region.location.x - 4, metrics.content_region.location.y - 4, metrics.content_region.size.width + 8,
                                         metrics.content_region.size.height + 8, radius + 2, 2, focus_ring_color,
                                         egui_color_alpha_mix(self->alpha, local->compact_mode ? 42 : 46));
    }
    egui_canvas_draw_round_rectangle(metrics.content_region.location.x - 1, metrics.content_region.location.y - 1, metrics.content_region.size.width + 2,
                                     metrics.content_region.size.height + 2, radius, 1, card_inner_border,
                                     egui_color_alpha_mix(self->alpha, local->compact_mode ? 24 : 28));

    if (metrics.show_title)
    {
        text_region = metrics.title_region;
        egui_view_drop_down_button_draw_text(local->meta_font, self, snapshot->title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color);
    }

    egui_canvas_draw_round_rectangle_fill(metrics.row_region.location.x, metrics.row_region.location.y, metrics.row_region.size.width,
                                          metrics.row_region.size.height, row_radius, row_fill,
                                          egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_ROW_FILL_ALPHA
                                                                                                : EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_ROW_FILL_ALPHA));
    if (local->read_only_mode && read_only_row_overlay_alpha > 0)
    {
        egui_canvas_draw_round_rectangle_fill(metrics.row_region.location.x + 1, metrics.row_region.location.y + 1, metrics.row_region.size.width - 2,
                                              metrics.row_region.size.height - 2, row_radius, read_only_overlay_color,
                                              egui_color_alpha_mix(self->alpha, read_only_row_overlay_alpha));
    }
    egui_canvas_draw_round_rectangle(metrics.row_region.location.x, metrics.row_region.location.y, metrics.row_region.size.width,
                                     metrics.row_region.size.height, row_radius, 1, row_border,
                                     egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_DROP_DOWN_BUTTON_COMPACT_ROW_BORDER_ALPHA
                                                                                           : EGUI_VIEW_DROP_DOWN_BUTTON_STANDARD_ROW_BORDER_ALPHA));
    egui_canvas_draw_round_rectangle(metrics.row_region.location.x + 1, metrics.row_region.location.y + 1, metrics.row_region.size.width - 2,
                                     metrics.row_region.size.height - 2, row_radius, 1, row_inner_border, egui_color_alpha_mix(self->alpha, 26));

    if (metrics.show_glyph)
    {
        egui_canvas_draw_round_rectangle_fill(metrics.glyph_region.location.x, metrics.glyph_region.location.y, metrics.glyph_region.size.width,
                                              metrics.glyph_region.size.height, 4, glyph_fill, egui_color_alpha_mix(self->alpha, 96));
        egui_canvas_draw_round_rectangle(metrics.glyph_region.location.x, metrics.glyph_region.location.y, metrics.glyph_region.size.width,
                                         metrics.glyph_region.size.height, 4, 1, glyph_border, egui_color_alpha_mix(self->alpha, 42));
        egui_canvas_draw_round_rectangle(metrics.glyph_region.location.x + 1, metrics.glyph_region.location.y + 1, metrics.glyph_region.size.width - 2,
                                         metrics.glyph_region.size.height - 2, 3, 1, glyph_inner_border, egui_color_alpha_mix(self->alpha, 24));
        text_region = metrics.glyph_region;
        egui_view_drop_down_button_draw_text(local->meta_font, self, snapshot->glyph, &text_region, EGUI_ALIGN_CENTER,
                                             snapshot->emphasized ? EGUI_COLOR_WHITE : label_color);
    }

    text_region = metrics.label_region;
    egui_view_drop_down_button_draw_text(local->font, self, snapshot->label, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, label_color);

    egui_canvas_draw_round_rectangle_fill(metrics.hint_region.location.x, metrics.hint_region.location.y, metrics.hint_region.size.width,
                                          metrics.hint_region.size.height, local->compact_mode ? 5 : 6, hint_fill, egui_color_alpha_mix(self->alpha, 98));
    if (local->read_only_mode && read_only_hint_overlay_alpha > 0)
    {
        egui_canvas_draw_round_rectangle_fill(metrics.hint_region.location.x + 1, metrics.hint_region.location.y + 1, metrics.hint_region.size.width - 2,
                                              metrics.hint_region.size.height - 2, local->compact_mode ? 4 : 5, read_only_overlay_color,
                                              egui_color_alpha_mix(self->alpha, read_only_hint_overlay_alpha));
    }
    egui_canvas_draw_round_rectangle(metrics.hint_region.location.x, metrics.hint_region.location.y, metrics.hint_region.size.width,
                                     metrics.hint_region.size.height, local->compact_mode ? 5 : 6, 1, hint_border, egui_color_alpha_mix(self->alpha, 44));
    egui_canvas_draw_round_rectangle(metrics.hint_region.location.x + 1, metrics.hint_region.location.y + 1, metrics.hint_region.size.width - 2,
                                     metrics.hint_region.size.height - 2, local->compact_mode ? 4 : 5, 1, hint_inner_border,
                                     egui_color_alpha_mix(self->alpha, 28));

    if (snapshot->hint != NULL && snapshot->hint[0] != '\0')
    {
        text_region = metrics.hint_region;
        egui_view_drop_down_button_draw_text(local->meta_font, self, snapshot->hint, &text_region, EGUI_ALIGN_CENTER, hint_text);
    }
    else
    {
        egui_view_drop_down_button_draw_chevron(self, &metrics.hint_region, hint_text, 92);
    }

    if (metrics.show_helper)
    {
        text_region = metrics.helper_region;
        egui_view_drop_down_button_draw_text(local->meta_font, self, snapshot->helper, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, helper_color);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_drop_down_button_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_drop_down_button_t);

    if (local->read_only_mode)
    {
        if (event->type == EGUI_MOTION_EVENT_ACTION_UP || event->type == EGUI_MOTION_EVENT_ACTION_CANCEL)
        {
            egui_view_set_pressed(self, false);
        }
        return 0;
    }

    return egui_view_on_touch_event(self, event);
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_drop_down_button_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_drop_down_button_t);

    if (local->read_only_mode)
    {
        return 0;
    }

    return egui_view_on_key_event(self, event);
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_drop_down_button_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_drop_down_button_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_drop_down_button_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_drop_down_button_on_key_event,
#endif
};

void egui_view_drop_down_button_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_drop_down_button_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_drop_down_button_t);
    egui_view_set_padding_all(self, 2);
    egui_view_set_clickable(self, true);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(self, true);
#endif

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD6DEE7);
    local->text_color = EGUI_COLOR_HEX(0x1D2630);
    local->muted_text_color = EGUI_COLOR_HEX(0x6F7B89);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->success_color = EGUI_COLOR_HEX(0x178454);
    local->warning_color = EGUI_COLOR_HEX(0xB87A16);
    local->danger_color = EGUI_COLOR_HEX(0xB13A35);
    local->neutral_color = EGUI_COLOR_HEX(0x7A8795);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->compact_mode = 0;
    local->read_only_mode = 0;

    egui_view_set_view_name(self, "egui_view_drop_down_button");
}
