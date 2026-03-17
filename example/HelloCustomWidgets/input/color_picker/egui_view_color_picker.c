#include "egui_view_color_picker.h"

#include <string.h>

#define COLOR_PICKER_STANDARD_RADIUS         10
#define COLOR_PICKER_STANDARD_FILL_ALPHA     94
#define COLOR_PICKER_STANDARD_BORDER_ALPHA   60
#define COLOR_PICKER_STANDARD_PAD_X          10
#define COLOR_PICKER_STANDARD_PAD_Y          8
#define COLOR_PICKER_STANDARD_LABEL_HEIGHT   10
#define COLOR_PICKER_STANDARD_LABEL_GAP      3
#define COLOR_PICKER_STANDARD_PREVIEW_HEIGHT 16
#define COLOR_PICKER_STANDARD_PREVIEW_GAP    4
#define COLOR_PICKER_STANDARD_HELPER_HEIGHT  10
#define COLOR_PICKER_STANDARD_HELPER_GAP     4
#define COLOR_PICKER_STANDARD_INNER_RADIUS   7
#define COLOR_PICKER_STANDARD_MAIN_GAP       6
#define COLOR_PICKER_STANDARD_SWATCH_SIZE    12
#define COLOR_PICKER_STANDARD_MODE_WIDTH     34

#define COLOR_PICKER_COMPACT_RADIUS         8
#define COLOR_PICKER_COMPACT_FILL_ALPHA     92
#define COLOR_PICKER_COMPACT_BORDER_ALPHA   56
#define COLOR_PICKER_COMPACT_PAD_X          8
#define COLOR_PICKER_COMPACT_PAD_Y          6
#define COLOR_PICKER_COMPACT_PREVIEW_HEIGHT 12
#define COLOR_PICKER_COMPACT_PREVIEW_GAP    3
#define COLOR_PICKER_COMPACT_INNER_RADIUS   6
#define COLOR_PICKER_COMPACT_MAIN_GAP       4
#define COLOR_PICKER_COMPACT_SWATCH_SIZE    10
#define COLOR_PICKER_COMPACT_MODE_WIDTH     24

typedef struct egui_view_color_picker_metrics egui_view_color_picker_metrics_t;
struct egui_view_color_picker_metrics
{
    egui_region_t content_region;
    egui_region_t label_region;
    egui_region_t preview_region;
    egui_region_t swatch_region;
    egui_region_t hex_region;
    egui_region_t mode_region;
    egui_region_t palette_region;
    egui_region_t hue_region;
    egui_region_t helper_region;
    uint8_t show_meta;
};

static uint8_t color_picker_clamp_index(uint8_t value, uint8_t count)
{
    if (count == 0)
    {
        return 0;
    }
    if (value >= count)
    {
        return (uint8_t)(count - 1);
    }
    return value;
}

static uint8_t color_picker_normalize_part(egui_view_color_picker_t *local, uint8_t part)
{
    EGUI_UNUSED(local);
    if (part == EGUI_VIEW_COLOR_PICKER_PART_HUE)
    {
        return EGUI_VIEW_COLOR_PICKER_PART_HUE;
    }
    return EGUI_VIEW_COLOR_PICKER_PART_PALETTE;
}

static egui_color_t color_picker_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 72);
}

static void color_picker_hsv_to_rgb(uint16_t hue_deg, uint8_t saturation, uint8_t value, uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint8_t region;
    uint8_t remainder;
    uint16_t p;
    uint16_t q;
    uint16_t t;

    if (saturation == 0)
    {
        *r = value;
        *g = value;
        *b = value;
        return;
    }

    hue_deg %= 360;
    region = (uint8_t)(hue_deg / 60);
    remainder = (uint8_t)(((hue_deg % 60) * 255) / 60);
    p = ((uint16_t)value * (uint16_t)(255 - saturation) + 127) / 255;
    q = ((uint16_t)value * (uint16_t)(255 - (((uint16_t)saturation * remainder + 127) / 255)) + 127) / 255;
    t = ((uint16_t)value * (uint16_t)(255 - (((uint16_t)saturation * (255 - remainder) + 127) / 255)) + 127) / 255;

    switch (region)
    {
    case 0:
        *r = value;
        *g = (uint8_t)t;
        *b = (uint8_t)p;
        break;
    case 1:
        *r = (uint8_t)q;
        *g = value;
        *b = (uint8_t)p;
        break;
    case 2:
        *r = (uint8_t)p;
        *g = value;
        *b = (uint8_t)t;
        break;
    case 3:
        *r = (uint8_t)p;
        *g = (uint8_t)q;
        *b = value;
        break;
    case 4:
        *r = (uint8_t)t;
        *g = (uint8_t)p;
        *b = value;
        break;
    default:
        *r = value;
        *g = (uint8_t)p;
        *b = (uint8_t)q;
        break;
    }
}

static void color_picker_selection_to_rgb(uint8_t hue_index, uint8_t saturation_index, uint8_t value_index, uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint16_t hue_deg = (uint16_t)color_picker_clamp_index(hue_index, EGUI_VIEW_COLOR_PICKER_HUE_COUNT) * (360 / EGUI_VIEW_COLOR_PICKER_HUE_COUNT);
    uint8_t saturation = (uint8_t)((uint16_t)color_picker_clamp_index(saturation_index, EGUI_VIEW_COLOR_PICKER_SATURATION_COUNT) * 255 /
                                   (EGUI_VIEW_COLOR_PICKER_SATURATION_COUNT - 1));
    uint8_t value = (uint8_t)(255 - ((uint16_t)color_picker_clamp_index(value_index, EGUI_VIEW_COLOR_PICKER_VALUE_COUNT) * 144 /
                                     (EGUI_VIEW_COLOR_PICKER_VALUE_COUNT - 1)));

    color_picker_hsv_to_rgb(hue_deg, saturation, value, r, g, b);
}

static void color_picker_update_selection_color(egui_view_color_picker_t *local)
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    static const char digits[] = "0123456789ABCDEF";

    color_picker_selection_to_rgb(local->hue_index, local->saturation_index, local->value_index, &r, &g, &b);
    local->selected_color = EGUI_COLOR_MAKE(r, g, b);
    local->hex_text[0] = '#';
    local->hex_text[1] = digits[(r >> 4) & 0x0F];
    local->hex_text[2] = digits[r & 0x0F];
    local->hex_text[3] = digits[(g >> 4) & 0x0F];
    local->hex_text[4] = digits[g & 0x0F];
    local->hex_text[5] = digits[(b >> 4) & 0x0F];
    local->hex_text[6] = digits[b & 0x0F];
    local->hex_text[7] = '\0';
}

static void color_picker_emit_changed(egui_view_t *self, uint8_t part)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);

    if (local->on_changed != NULL)
    {
        local->on_changed(self, local->selected_color, local->hue_index, local->saturation_index, local->value_index, part);
    }
}

static uint8_t color_picker_set_selection_inner(egui_view_t *self, uint8_t hue_index, uint8_t saturation_index, uint8_t value_index, uint8_t notify,
                                                uint8_t part)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);
    uint8_t next_hue = color_picker_clamp_index(hue_index, EGUI_VIEW_COLOR_PICKER_HUE_COUNT);
    uint8_t next_sat = color_picker_clamp_index(saturation_index, EGUI_VIEW_COLOR_PICKER_SATURATION_COUNT);
    uint8_t next_value = color_picker_clamp_index(value_index, EGUI_VIEW_COLOR_PICKER_VALUE_COUNT);

    if (local->hue_index == next_hue && local->saturation_index == next_sat && local->value_index == next_value)
    {
        return 0;
    }

    local->hue_index = next_hue;
    local->saturation_index = next_sat;
    local->value_index = next_value;
    color_picker_update_selection_color(local);
    egui_view_invalidate(self);
    if (notify)
    {
        color_picker_emit_changed(self, part);
    }
    return 1;
}

static void color_picker_set_current_part_inner(egui_view_t *self, uint8_t part, uint8_t invalidate)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);
    uint8_t normalized = color_picker_normalize_part(local, part);

    if (local->read_only_mode)
    {
        normalized = EGUI_VIEW_COLOR_PICKER_PART_PALETTE;
    }
    if (local->current_part == normalized)
    {
        return;
    }
    local->current_part = normalized;
    if (invalidate)
    {
        egui_view_invalidate(self);
    }
}

static void color_picker_normalize_state(egui_view_color_picker_t *local)
{
    local->hue_index = color_picker_clamp_index(local->hue_index, EGUI_VIEW_COLOR_PICKER_HUE_COUNT);
    local->saturation_index = color_picker_clamp_index(local->saturation_index, EGUI_VIEW_COLOR_PICKER_SATURATION_COUNT);
    local->value_index = color_picker_clamp_index(local->value_index, EGUI_VIEW_COLOR_PICKER_VALUE_COUNT);
    local->current_part = color_picker_normalize_part(local, local->current_part);
    if (local->pressed_part != EGUI_VIEW_COLOR_PICKER_PART_PALETTE && local->pressed_part != EGUI_VIEW_COLOR_PICKER_PART_HUE)
    {
        local->pressed_part = EGUI_VIEW_COLOR_PICKER_PART_NONE;
    }
    if (local->read_only_mode)
    {
        local->current_part = EGUI_VIEW_COLOR_PICKER_PART_PALETTE;
        local->pressed_part = EGUI_VIEW_COLOR_PICKER_PART_NONE;
    }
    color_picker_update_selection_color(local);
}

void egui_view_color_picker_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);

    local->font = font != NULL ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_color_picker_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);

    local->meta_font = font != NULL ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_color_picker_set_label(egui_view_t *self, const char *label)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);

    local->label = label;
    egui_view_invalidate(self);
}

void egui_view_color_picker_set_helper(egui_view_t *self, const char *helper)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);

    local->helper = helper;
    egui_view_invalidate(self);
}

void egui_view_color_picker_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                        egui_color_t muted_text_color, egui_color_t accent_color)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);

    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    egui_view_invalidate(self);
}

void egui_view_color_picker_set_selection(egui_view_t *self, uint8_t hue_index, uint8_t saturation_index, uint8_t value_index)
{
    color_picker_set_selection_inner(self, hue_index, saturation_index, value_index, 0, EGUI_VIEW_COLOR_PICKER_PART_PALETTE);
}

uint8_t egui_view_color_picker_get_hue_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);
    color_picker_normalize_state(local);
    return local->hue_index;
}

uint8_t egui_view_color_picker_get_saturation_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);
    color_picker_normalize_state(local);
    return local->saturation_index;
}

uint8_t egui_view_color_picker_get_value_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);
    color_picker_normalize_state(local);
    return local->value_index;
}

egui_color_t egui_view_color_picker_get_color(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);
    color_picker_normalize_state(local);
    return local->selected_color;
}

const char *egui_view_color_picker_get_hex_text(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);
    color_picker_normalize_state(local);
    return local->hex_text;
}

void egui_view_color_picker_set_current_part(egui_view_t *self, uint8_t part)
{
    color_picker_set_current_part_inner(self, part, 1);
}

uint8_t egui_view_color_picker_get_current_part(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);
    color_picker_normalize_state(local);
    return local->current_part;
}

void egui_view_color_picker_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);
    uint8_t normalized = compact_mode ? 1 : 0;

    if (local->compact_mode == normalized)
    {
        return;
    }

    local->compact_mode = normalized;
    egui_view_invalidate(self);
}

void egui_view_color_picker_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);
    uint8_t normalized = read_only_mode ? 1 : 0;

    if (local->read_only_mode == normalized)
    {
        return;
    }

    local->read_only_mode = normalized;
    local->pressed_part = EGUI_VIEW_COLOR_PICKER_PART_NONE;
    local->current_part = EGUI_VIEW_COLOR_PICKER_PART_PALETTE;
    egui_view_invalidate(self);
}

void egui_view_color_picker_set_on_changed_listener(egui_view_t *self, egui_view_on_color_picker_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);
    local->on_changed = listener;
}
static void color_picker_get_metrics(egui_view_color_picker_t *local, egui_view_t *self, egui_view_color_picker_metrics_t *metrics)
{
    egui_region_t region;
    egui_dim_t pad_x = local->compact_mode ? COLOR_PICKER_COMPACT_PAD_X : COLOR_PICKER_STANDARD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? COLOR_PICKER_COMPACT_PAD_Y : COLOR_PICKER_STANDARD_PAD_Y;
    egui_dim_t preview_h = local->compact_mode ? COLOR_PICKER_COMPACT_PREVIEW_HEIGHT : COLOR_PICKER_STANDARD_PREVIEW_HEIGHT;
    egui_dim_t preview_gap = local->compact_mode ? COLOR_PICKER_COMPACT_PREVIEW_GAP : COLOR_PICKER_STANDARD_PREVIEW_GAP;
    egui_dim_t swatch_size = local->compact_mode ? COLOR_PICKER_COMPACT_SWATCH_SIZE : COLOR_PICKER_STANDARD_SWATCH_SIZE;
    egui_dim_t mode_w = local->compact_mode ? COLOR_PICKER_COMPACT_MODE_WIDTH : COLOR_PICKER_STANDARD_MODE_WIDTH;
    egui_dim_t label_h = COLOR_PICKER_STANDARD_LABEL_HEIGHT;
    egui_dim_t label_gap = COLOR_PICKER_STANDARD_LABEL_GAP;
    egui_dim_t helper_h = COLOR_PICKER_STANDARD_HELPER_HEIGHT;
    egui_dim_t helper_gap = COLOR_PICKER_STANDARD_HELPER_GAP;
    egui_dim_t rail_gap = local->compact_mode ? COLOR_PICKER_COMPACT_MAIN_GAP : COLOR_PICKER_STANDARD_MAIN_GAP;
    egui_dim_t hue_w = local->compact_mode ? 10 : 12;
    egui_dim_t current_y;
    egui_dim_t main_bottom;

    memset(metrics, 0, sizeof(*metrics));
    egui_view_get_work_region(self, &region);
    metrics->show_meta = local->compact_mode ? 0 : 1;

    metrics->content_region.location.x = region.location.x + pad_x;
    metrics->content_region.location.y = region.location.y + pad_y;
    metrics->content_region.size.width = region.size.width - pad_x * 2;
    metrics->content_region.size.height = region.size.height - pad_y * 2;

    current_y = metrics->content_region.location.y;
    if (metrics->show_meta)
    {
        metrics->label_region.location.x = metrics->content_region.location.x;
        metrics->label_region.location.y = current_y;
        metrics->label_region.size.width = metrics->content_region.size.width;
        metrics->label_region.size.height = label_h;
        current_y += label_h + label_gap;
    }

    metrics->preview_region.location.x = metrics->content_region.location.x;
    metrics->preview_region.location.y = current_y;
    metrics->preview_region.size.width = metrics->content_region.size.width;
    metrics->preview_region.size.height = preview_h;
    current_y += preview_h + preview_gap;

    if (metrics->show_meta)
    {
        metrics->helper_region.location.x = metrics->content_region.location.x;
        metrics->helper_region.location.y = metrics->content_region.location.y + metrics->content_region.size.height - helper_h;
        metrics->helper_region.size.width = metrics->content_region.size.width;
        metrics->helper_region.size.height = helper_h;
        main_bottom = metrics->helper_region.location.y - helper_gap;
    }
    else
    {
        main_bottom = metrics->content_region.location.y + metrics->content_region.size.height;
    }

    if (main_bottom < current_y)
    {
        main_bottom = current_y;
    }

    metrics->palette_region.location.x = metrics->content_region.location.x;
    metrics->palette_region.location.y = current_y;
    metrics->palette_region.size.width = metrics->content_region.size.width - hue_w - rail_gap;
    if (metrics->palette_region.size.width < 24)
    {
        metrics->palette_region.size.width = 24;
    }
    metrics->palette_region.size.height = main_bottom - current_y;
    if (metrics->palette_region.size.height < 12)
    {
        metrics->palette_region.size.height = 12;
    }

    metrics->hue_region.location.x = metrics->palette_region.location.x + metrics->palette_region.size.width + rail_gap;
    metrics->hue_region.location.y = current_y;
    metrics->hue_region.size.width = hue_w;
    metrics->hue_region.size.height = metrics->palette_region.size.height;

    metrics->swatch_region.location.x = metrics->preview_region.location.x;
    metrics->swatch_region.location.y = metrics->preview_region.location.y + (preview_h - swatch_size) / 2;
    metrics->swatch_region.size.width = swatch_size;
    metrics->swatch_region.size.height = swatch_size;

    metrics->mode_region.location.x = metrics->preview_region.location.x + metrics->preview_region.size.width - mode_w;
    metrics->mode_region.location.y = metrics->preview_region.location.y;
    metrics->mode_region.size.width = mode_w;
    metrics->mode_region.size.height = preview_h;

    metrics->hex_region.location.x = metrics->swatch_region.location.x + swatch_size + 6;
    metrics->hex_region.location.y = metrics->preview_region.location.y;
    metrics->hex_region.size.width = metrics->mode_region.location.x - metrics->hex_region.location.x - 4;
    metrics->hex_region.size.height = preview_h;
}

static void color_picker_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align, egui_color_t color,
                                   egui_alpha_t alpha)
{
    egui_region_t draw_region = *region;

    if (font == NULL || text == NULL || text[0] == '\0' || egui_region_is_empty(&draw_region))
    {
        return;
    }

    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, egui_color_alpha_mix(self->alpha, alpha));
}

static void color_picker_draw_preview(egui_view_t *self, egui_view_color_picker_t *local, const egui_view_color_picker_metrics_t *metrics,
                                      egui_color_t panel_fill, egui_color_t border_color, egui_color_t text_color, egui_color_t muted_color,
                                      egui_color_t accent_color)
{
    const char *mode_text = local->read_only_mode ? "LOCK" : (local->current_part == EGUI_VIEW_COLOR_PICKER_PART_HUE ? "Hue" : "Tone");
    egui_color_t mode_fill = egui_rgb_mix(local->surface_color, accent_color, local->read_only_mode ? 12 : 16);
    egui_color_t swatch_border = egui_rgb_mix(border_color, local->selected_color, 24);
    egui_dim_t radius = local->compact_mode ? COLOR_PICKER_COMPACT_INNER_RADIUS : COLOR_PICKER_STANDARD_INNER_RADIUS;

    egui_canvas_draw_round_rectangle_fill(metrics->preview_region.location.x, metrics->preview_region.location.y, metrics->preview_region.size.width,
                                          metrics->preview_region.size.height, radius, panel_fill, egui_color_alpha_mix(self->alpha, 38));
    egui_canvas_draw_round_rectangle(metrics->preview_region.location.x, metrics->preview_region.location.y, metrics->preview_region.size.width,
                                     metrics->preview_region.size.height, radius, 1, border_color, egui_color_alpha_mix(self->alpha, 40));

    egui_canvas_draw_round_rectangle_fill(metrics->swatch_region.location.x, metrics->swatch_region.location.y, metrics->swatch_region.size.width,
                                          metrics->swatch_region.size.height, 3, local->selected_color, egui_color_alpha_mix(self->alpha, 100));
    egui_canvas_draw_round_rectangle(metrics->swatch_region.location.x, metrics->swatch_region.location.y, metrics->swatch_region.size.width,
                                     metrics->swatch_region.size.height, 3, 1, swatch_border, egui_color_alpha_mix(self->alpha, 72));

    color_picker_draw_text(local->font, self, local->hex_text, &metrics->hex_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color, 100);

    egui_canvas_draw_round_rectangle_fill(metrics->mode_region.location.x, metrics->mode_region.location.y, metrics->mode_region.size.width,
                                          metrics->mode_region.size.height, radius, mode_fill, egui_color_alpha_mix(self->alpha, 30));
    egui_canvas_draw_round_rectangle(metrics->mode_region.location.x, metrics->mode_region.location.y, metrics->mode_region.size.width,
                                     metrics->mode_region.size.height, radius, 1, egui_rgb_mix(border_color, accent_color, 18),
                                     egui_color_alpha_mix(self->alpha, 36));
    color_picker_draw_text(local->meta_font != NULL ? local->meta_font : local->font, self, mode_text, &metrics->mode_region, EGUI_ALIGN_CENTER,
                           local->read_only_mode ? muted_color : accent_color, 100);
}

static void color_picker_draw_palette_cells(egui_view_t *self, egui_view_color_picker_t *local, const egui_view_color_picker_metrics_t *metrics,
                                            egui_color_t accent_color, egui_color_t border_color, egui_color_t surface_color, uint8_t disabled_mix)
{
    egui_dim_t gap = local->compact_mode ? 1 : 2;
    egui_dim_t cell_w = (metrics->palette_region.size.width - gap * (EGUI_VIEW_COLOR_PICKER_SATURATION_COUNT - 1)) / EGUI_VIEW_COLOR_PICKER_SATURATION_COUNT;
    egui_dim_t cell_h = (metrics->palette_region.size.height - gap * (EGUI_VIEW_COLOR_PICKER_VALUE_COUNT - 1)) / EGUI_VIEW_COLOR_PICKER_VALUE_COUNT;
    uint8_t value_index;
    uint8_t saturation_index;

    if (cell_w < 4)
    {
        cell_w = 4;
    }
    if (cell_h < 3)
    {
        cell_h = 3;
    }

    for (value_index = 0; value_index < EGUI_VIEW_COLOR_PICKER_VALUE_COUNT; ++value_index)
    {
        for (saturation_index = 0; saturation_index < EGUI_VIEW_COLOR_PICKER_SATURATION_COUNT; ++saturation_index)
        {
            egui_dim_t x = metrics->palette_region.location.x + saturation_index * (cell_w + gap);
            egui_dim_t y = metrics->palette_region.location.y + value_index * (cell_h + gap);
            uint8_t r;
            uint8_t g;
            uint8_t b;
            egui_color_t cell_color;
            egui_color_t outline_color;
            uint8_t selected = (local->saturation_index == saturation_index && local->value_index == value_index) ? 1 : 0;

            color_picker_selection_to_rgb(local->hue_index, saturation_index, value_index, &r, &g, &b);
            cell_color = EGUI_COLOR_MAKE(r, g, b);
            if (disabled_mix)
            {
                cell_color = egui_rgb_mix(cell_color, surface_color, disabled_mix);
            }

            egui_canvas_draw_round_rectangle_fill(x, y, cell_w, cell_h, local->compact_mode ? 2 : 3, cell_color, egui_color_alpha_mix(self->alpha, 100));

            if (selected)
            {
                outline_color = self->is_focused && local->current_part == EGUI_VIEW_COLOR_PICKER_PART_PALETTE ? accent_color : border_color;
                egui_canvas_draw_round_rectangle(x - 1, y - 1, cell_w + 2, cell_h + 2, local->compact_mode ? 2 : 3, 1, outline_color,
                                                 egui_color_alpha_mix(self->alpha, 92));
                egui_canvas_draw_round_rectangle(x, y, cell_w, cell_h, local->compact_mode ? 2 : 3, 1, EGUI_COLOR_WHITE, egui_color_alpha_mix(self->alpha, 70));
            }
        }
    }
}

static void color_picker_draw_hue_rail(egui_view_t *self, egui_view_color_picker_t *local, const egui_view_color_picker_metrics_t *metrics,
                                       egui_color_t accent_color, egui_color_t border_color, egui_color_t surface_color, uint8_t disabled_mix)
{
    egui_dim_t gap = 1;
    egui_dim_t segment_h = (metrics->hue_region.size.height - gap * (EGUI_VIEW_COLOR_PICKER_HUE_COUNT - 1)) / EGUI_VIEW_COLOR_PICKER_HUE_COUNT;
    uint8_t hue_index;

    if (segment_h < 2)
    {
        segment_h = 2;
    }

    for (hue_index = 0; hue_index < EGUI_VIEW_COLOR_PICKER_HUE_COUNT; ++hue_index)
    {
        egui_dim_t y = metrics->hue_region.location.y + hue_index * (segment_h + gap);
        uint8_t r;
        uint8_t g;
        uint8_t b;
        egui_color_t segment_color;
        uint8_t selected = local->hue_index == hue_index ? 1 : 0;

        color_picker_selection_to_rgb(hue_index, EGUI_VIEW_COLOR_PICKER_SATURATION_COUNT - 1, 0, &r, &g, &b);
        segment_color = EGUI_COLOR_MAKE(r, g, b);
        if (disabled_mix)
        {
            segment_color = egui_rgb_mix(segment_color, surface_color, disabled_mix);
        }

        egui_canvas_draw_round_rectangle_fill(metrics->hue_region.location.x, y, metrics->hue_region.size.width, segment_h, 2, segment_color,
                                              egui_color_alpha_mix(self->alpha, 100));
        if (selected)
        {
            egui_color_t outline = self->is_focused && local->current_part == EGUI_VIEW_COLOR_PICKER_PART_HUE ? accent_color : border_color;
            egui_canvas_draw_round_rectangle(metrics->hue_region.location.x - 1, y - 1, metrics->hue_region.size.width + 2, segment_h + 2, 2, 1, outline,
                                             egui_color_alpha_mix(self->alpha, 92));
        }
    }
}

static void egui_view_color_picker_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);
    egui_region_t region;
    egui_view_color_picker_metrics_t metrics;
    egui_color_t surface_color = local->surface_color;
    egui_color_t border_color = local->border_color;
    egui_color_t text_color = local->text_color;
    egui_color_t muted_color = local->muted_text_color;
    egui_color_t accent_color = local->accent_color;
    egui_color_t outer_fill;
    egui_color_t outer_border;
    egui_color_t panel_fill;
    uint8_t disabled_mix = 0;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0)
    {
        return;
    }

    color_picker_normalize_state(local);

    if (local->read_only_mode)
    {
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xFBFCFD), 18);
        border_color = egui_rgb_mix(border_color, muted_color, 18);
        text_color = egui_rgb_mix(text_color, muted_color, 22);
        accent_color = egui_rgb_mix(accent_color, muted_color, 96);
    }

    if (!egui_view_get_enable(self))
    {
        border_color = color_picker_mix_disabled(border_color);
        text_color = color_picker_mix_disabled(text_color);
        muted_color = color_picker_mix_disabled(muted_color);
        accent_color = color_picker_mix_disabled(accent_color);
        disabled_mix = 96;
    }

    outer_fill = egui_rgb_mix(surface_color, accent_color, local->compact_mode ? 4 : 6);
    outer_border = egui_rgb_mix(border_color, accent_color, local->read_only_mode ? 10 : 18);
    panel_fill = egui_rgb_mix(surface_color, accent_color, local->read_only_mode ? 4 : 8);

    color_picker_get_metrics(local, self, &metrics);

    egui_canvas_draw_round_rectangle_fill(
            region.location.x, region.location.y, region.size.width, region.size.height,
            local->compact_mode ? COLOR_PICKER_COMPACT_RADIUS : COLOR_PICKER_STANDARD_RADIUS, outer_fill,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? COLOR_PICKER_COMPACT_FILL_ALPHA : COLOR_PICKER_STANDARD_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(
            region.location.x, region.location.y, region.size.width, region.size.height,
            local->compact_mode ? COLOR_PICKER_COMPACT_RADIUS : COLOR_PICKER_STANDARD_RADIUS, 1, outer_border,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? COLOR_PICKER_COMPACT_BORDER_ALPHA : COLOR_PICKER_STANDARD_BORDER_ALPHA));
    egui_canvas_draw_round_rectangle_fill(region.location.x + 2, region.location.y + 2, region.size.width - 4, local->compact_mode ? 3 : 4,
                                          local->compact_mode ? COLOR_PICKER_COMPACT_RADIUS : COLOR_PICKER_STANDARD_RADIUS, accent_color,
                                          egui_color_alpha_mix(self->alpha, local->read_only_mode ? 12 : 24));

    if (metrics.show_meta)
    {
        color_picker_draw_text(local->meta_font, self, local->label, &metrics.label_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_color, 100);
    }

    color_picker_draw_preview(self, local, &metrics, panel_fill, border_color, text_color, muted_color, accent_color);
    color_picker_draw_palette_cells(self, local, &metrics, accent_color, border_color, surface_color, disabled_mix);
    color_picker_draw_hue_rail(self, local, &metrics, accent_color, border_color, surface_color, disabled_mix);

    if (self->is_focused && !local->read_only_mode)
    {
        const egui_region_t *focus_region = local->current_part == EGUI_VIEW_COLOR_PICKER_PART_HUE ? &metrics.hue_region : &metrics.palette_region;
        egui_canvas_draw_round_rectangle(focus_region->location.x - 2, focus_region->location.y - 2, focus_region->size.width + 4,
                                         focus_region->size.height + 4, local->compact_mode ? 4 : 5, 1, accent_color, egui_color_alpha_mix(self->alpha, 72));
    }

    if (metrics.show_meta)
    {
        color_picker_draw_text(local->meta_font, self, local->helper, &metrics.helper_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_color, 100);
    }
}
static uint8_t color_picker_hit_part(egui_view_color_picker_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_color_picker_metrics_t metrics;

    color_picker_get_metrics(local, self, &metrics);
    if (egui_region_pt_in_rect(&metrics.palette_region, x, y))
    {
        return EGUI_VIEW_COLOR_PICKER_PART_PALETTE;
    }
    if (egui_region_pt_in_rect(&metrics.hue_region, x, y))
    {
        return EGUI_VIEW_COLOR_PICKER_PART_HUE;
    }
    return EGUI_VIEW_COLOR_PICKER_PART_NONE;
}

static void color_picker_point_to_selection(const egui_region_t *region, egui_dim_t x, egui_dim_t y, uint8_t count_x, uint8_t count_y, uint8_t *out_x,
                                            uint8_t *out_y)
{
    egui_dim_t rel_x = x - region->location.x;
    egui_dim_t rel_y = y - region->location.y;

    if (rel_x < 0)
    {
        rel_x = 0;
    }
    if (rel_y < 0)
    {
        rel_y = 0;
    }
    if (rel_x >= region->size.width)
    {
        rel_x = region->size.width - 1;
    }
    if (rel_y >= region->size.height)
    {
        rel_y = region->size.height - 1;
    }

    *out_x = (uint8_t)(((uint32_t)rel_x * count_x) / region->size.width);
    *out_y = (uint8_t)(((uint32_t)rel_y * count_y) / region->size.height);
    *out_x = color_picker_clamp_index(*out_x, count_x);
    *out_y = color_picker_clamp_index(*out_y, count_y);
}

static void color_picker_update_from_point(egui_view_t *self, uint8_t part, egui_dim_t x, egui_dim_t y, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);
    egui_view_color_picker_metrics_t metrics;

    color_picker_get_metrics(local, self, &metrics);
    if (part == EGUI_VIEW_COLOR_PICKER_PART_PALETTE)
    {
        uint8_t saturation_index;
        uint8_t value_index;

        color_picker_point_to_selection(&metrics.palette_region, x, y, EGUI_VIEW_COLOR_PICKER_SATURATION_COUNT, EGUI_VIEW_COLOR_PICKER_VALUE_COUNT,
                                        &saturation_index, &value_index);
        color_picker_set_selection_inner(self, local->hue_index, saturation_index, value_index, notify, EGUI_VIEW_COLOR_PICKER_PART_PALETTE);
    }
    else if (part == EGUI_VIEW_COLOR_PICKER_PART_HUE)
    {
        uint8_t hue_index;
        uint8_t unused;

        color_picker_point_to_selection(&metrics.hue_region, x, y, 1, EGUI_VIEW_COLOR_PICKER_HUE_COUNT, &unused, &hue_index);
        color_picker_set_selection_inner(self, hue_index, local->saturation_index, local->value_index, notify, EGUI_VIEW_COLOR_PICKER_PART_HUE);
    }
}

uint8_t egui_view_color_picker_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);
    egui_view_color_picker_metrics_t metrics;

    if (region == NULL)
    {
        return 0;
    }

    color_picker_get_metrics(local, self, &metrics);
    if (part == EGUI_VIEW_COLOR_PICKER_PART_PALETTE)
    {
        egui_region_copy(region, &metrics.palette_region);
        return 1;
    }
    if (part == EGUI_VIEW_COLOR_PICKER_PART_HUE)
    {
        egui_region_copy(region, &metrics.hue_region);
        return 1;
    }
    egui_region_init_empty(region);
    return 0;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_color_picker_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);
    uint8_t hit_part;

    if (!egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    hit_part = color_picker_hit_part(local, self, event->location.x, event->location.y);

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        if (hit_part == EGUI_VIEW_COLOR_PICKER_PART_NONE)
        {
            return 0;
        }
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (self->is_focusable)
        {
            egui_view_request_focus(self);
        }
#endif
        local->pressed_part = hit_part;
        egui_view_set_pressed(self, true);
        color_picker_set_current_part_inner(self, hit_part, 0);
        color_picker_update_from_point(self, hit_part, event->location.x, event->location.y, 1);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        if (local->pressed_part == EGUI_VIEW_COLOR_PICKER_PART_NONE)
        {
            return 0;
        }
        if (hit_part != EGUI_VIEW_COLOR_PICKER_PART_NONE)
        {
            local->pressed_part = hit_part;
            color_picker_set_current_part_inner(self, hit_part, 0);
            color_picker_update_from_point(self, hit_part, event->location.x, event->location.y, 1);
        }
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        if (local->pressed_part != EGUI_VIEW_COLOR_PICKER_PART_NONE && hit_part != EGUI_VIEW_COLOR_PICKER_PART_NONE)
        {
            color_picker_set_current_part_inner(self, hit_part, 0);
            color_picker_update_from_point(self, hit_part, event->location.x, event->location.y, 1);
        }
        local->pressed_part = EGUI_VIEW_COLOR_PICKER_PART_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_part != EGUI_VIEW_COLOR_PICKER_PART_NONE ? 1 : 0;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_part = EGUI_VIEW_COLOR_PICKER_PART_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

static uint8_t color_picker_adjust_palette(egui_view_t *self, int8_t delta_saturation, int8_t delta_value, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);
    int saturation = (int)local->saturation_index + delta_saturation;
    int value = (int)local->value_index + delta_value;

    if (saturation < 0)
    {
        saturation = 0;
    }
    if (saturation >= EGUI_VIEW_COLOR_PICKER_SATURATION_COUNT)
    {
        saturation = EGUI_VIEW_COLOR_PICKER_SATURATION_COUNT - 1;
    }
    if (value < 0)
    {
        value = 0;
    }
    if (value >= EGUI_VIEW_COLOR_PICKER_VALUE_COUNT)
    {
        value = EGUI_VIEW_COLOR_PICKER_VALUE_COUNT - 1;
    }

    return color_picker_set_selection_inner(self, local->hue_index, (uint8_t)saturation, (uint8_t)value, notify, EGUI_VIEW_COLOR_PICKER_PART_PALETTE);
}

static uint8_t color_picker_adjust_hue(egui_view_t *self, int8_t delta, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_color_picker_t);
    int hue = (int)local->hue_index + delta;

    if (hue < 0)
    {
        hue = 0;
    }
    if (hue >= EGUI_VIEW_COLOR_PICKER_HUE_COUNT)
    {
        hue = EGUI_VIEW_COLOR_PICKER_HUE_COUNT - 1;
    }

    return color_picker_set_selection_inner(self, (uint8_t)hue, local->saturation_index, local->value_index, notify, EGUI_VIEW_COLOR_PICKER_PART_HUE);
}

static uint8_t color_picker_handle_navigation_key_inner(egui_view_t *self, uint8_t key_code)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    EGUI_LOCAL_INIT(egui_view_color_picker_t);

    if (!egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    switch (key_code)
    {
    case EGUI_KEY_CODE_TAB:
        color_picker_set_current_part_inner(
                self, local->current_part == EGUI_VIEW_COLOR_PICKER_PART_PALETTE ? EGUI_VIEW_COLOR_PICKER_PART_HUE : EGUI_VIEW_COLOR_PICKER_PART_PALETTE, 1);
        return 1;
    case EGUI_KEY_CODE_LEFT:
        if (local->current_part == EGUI_VIEW_COLOR_PICKER_PART_HUE)
        {
            color_picker_set_current_part_inner(self, EGUI_VIEW_COLOR_PICKER_PART_PALETTE, 1);
            return 1;
        }
        color_picker_adjust_palette(self, -1, 0, 1);
        return 1;
    case EGUI_KEY_CODE_RIGHT:
        if (local->current_part == EGUI_VIEW_COLOR_PICKER_PART_HUE)
        {
            return 1;
        }
        if (local->saturation_index >= EGUI_VIEW_COLOR_PICKER_SATURATION_COUNT - 1)
        {
            color_picker_set_current_part_inner(self, EGUI_VIEW_COLOR_PICKER_PART_HUE, 1);
            return 1;
        }
        color_picker_adjust_palette(self, 1, 0, 1);
        return 1;
    case EGUI_KEY_CODE_UP:
        if (local->current_part == EGUI_VIEW_COLOR_PICKER_PART_HUE)
        {
            color_picker_adjust_hue(self, -1, 1);
        }
        else
        {
            color_picker_adjust_palette(self, 0, -1, 1);
        }
        return 1;
    case EGUI_KEY_CODE_DOWN:
        if (local->current_part == EGUI_VIEW_COLOR_PICKER_PART_HUE)
        {
            color_picker_adjust_hue(self, 1, 1);
        }
        else
        {
            color_picker_adjust_palette(self, 0, 1, 1);
        }
        return 1;
    case EGUI_KEY_CODE_HOME:
        if (local->current_part == EGUI_VIEW_COLOR_PICKER_PART_HUE)
        {
            color_picker_set_selection_inner(self, 0, local->saturation_index, local->value_index, 1, EGUI_VIEW_COLOR_PICKER_PART_HUE);
        }
        else
        {
            color_picker_set_selection_inner(self, local->hue_index, 0, 0, 1, EGUI_VIEW_COLOR_PICKER_PART_PALETTE);
        }
        return 1;
    case EGUI_KEY_CODE_END:
        if (local->current_part == EGUI_VIEW_COLOR_PICKER_PART_HUE)
        {
            color_picker_set_selection_inner(self, EGUI_VIEW_COLOR_PICKER_HUE_COUNT - 1, local->saturation_index, local->value_index, 1,
                                             EGUI_VIEW_COLOR_PICKER_PART_HUE);
        }
        else
        {
            color_picker_set_selection_inner(self, local->hue_index, EGUI_VIEW_COLOR_PICKER_SATURATION_COUNT - 1, EGUI_VIEW_COLOR_PICKER_VALUE_COUNT - 1, 1,
                                             EGUI_VIEW_COLOR_PICKER_PART_PALETTE);
        }
        return 1;
    case EGUI_KEY_CODE_ESCAPE:
        color_picker_set_current_part_inner(self, EGUI_VIEW_COLOR_PICKER_PART_PALETTE, 1);
        return 1;
    default:
        return 0;
    }
#else
    EGUI_UNUSED(self);
    EGUI_UNUSED(key_code);
    return 0;
#endif
}

uint8_t egui_view_color_picker_handle_navigation_key(egui_view_t *self, uint8_t key_code)
{
    return color_picker_handle_navigation_key_inner(self, key_code);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_color_picker_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    if (event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        switch (event->key_code)
        {
        case EGUI_KEY_CODE_TAB:
        case EGUI_KEY_CODE_LEFT:
        case EGUI_KEY_CODE_RIGHT:
        case EGUI_KEY_CODE_UP:
        case EGUI_KEY_CODE_DOWN:
        case EGUI_KEY_CODE_HOME:
        case EGUI_KEY_CODE_END:
        case EGUI_KEY_CODE_ESCAPE:
            return 1;
        default:
            return 0;
        }
    }

    if (color_picker_handle_navigation_key_inner(self, event->key_code))
    {
        return 1;
    }
    return egui_view_on_key_event(self, event);
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_color_picker_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_color_picker_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_color_picker_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_color_picker_on_key_event,
#endif
};

void egui_view_color_picker_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_color_picker_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_color_picker_t);
    egui_view_set_padding_all(self, 2);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(self, true);
#endif
    self->is_clickable = 1;

    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->on_changed = NULL;
    local->label = "Accent color";
    local->helper = "Tap tone grid or hue rail";
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD7DFE7);
    local->text_color = EGUI_COLOR_HEX(0x1B2732);
    local->muted_text_color = EGUI_COLOR_HEX(0x728190);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->hue_index = 7;
    local->saturation_index = 4;
    local->value_index = 1;
    local->current_part = EGUI_VIEW_COLOR_PICKER_PART_PALETTE;
    local->pressed_part = EGUI_VIEW_COLOR_PICKER_PART_NONE;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    color_picker_update_selection_color(local);
}
