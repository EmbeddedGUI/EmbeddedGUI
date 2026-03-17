#include <string.h>

#include "egui_view_chapter_strip.h"
#include "utils/egui_sprintf.h"

#define CS_STD_RADIUS      12
#define CS_STD_PAD_X       10
#define CS_STD_PAD_Y       9
#define CS_STD_TITLE_H     10
#define CS_STD_TITLE_GAP   3
#define CS_STD_SUMMARY_H   28
#define CS_STD_SUMMARY_GAP 4
#define CS_STD_ROW_H       38
#define CS_STD_HELPER_H    10
#define CS_STD_HELPER_GAP  5
#define CS_STD_BUTTON_W    22
#define CS_STD_BUTTON_GAP  8
#define CS_STD_CHAPTER_GAP 6
#define CS_STD_CHAPTER_R   8

#define CS_COMPACT_RADIUS      9
#define CS_COMPACT_PAD_X       8
#define CS_COMPACT_PAD_Y       7
#define CS_COMPACT_ROW_H       24
#define CS_COMPACT_BUTTON_W    18
#define CS_COMPACT_BUTTON_GAP  6
#define CS_COMPACT_CHAPTER_GAP 4
#define CS_COMPACT_CHAPTER_R   6

typedef struct egui_view_chapter_strip_metrics egui_view_chapter_strip_metrics_t;
struct egui_view_chapter_strip_metrics
{
    egui_region_t title_region;
    egui_region_t helper_region;
    egui_region_t summary_region;
    egui_region_t row_region;
    egui_region_t previous_region;
    egui_region_t rail_region;
    egui_region_t next_region;
    uint8_t show_title;
    uint8_t show_helper;
};

static uint8_t chapter_strip_has_text(const char *text)
{
    return text != NULL && text[0] != '\0';
}

static void chapter_strip_draw_fill(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t radius, egui_color_t color, egui_alpha_t alpha)
{
    if (w > 0 && h > 0)
    {
        egui_canvas_draw_round_rectangle_fill(x, y, w, h, radius, color, alpha);
    }
}

static void chapter_strip_draw_stroke(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t radius, egui_color_t color, egui_alpha_t alpha)
{
    if (w > 0 && h > 0)
    {
        egui_canvas_draw_round_rectangle(x, y, w, h, radius, 1, color, alpha);
    }
}

static void chapter_strip_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                    egui_color_t color)
{
    egui_region_t draw_region;

    if (region == NULL || region->size.width <= 0 || region->size.height <= 0 || !chapter_strip_has_text(text))
    {
        return;
    }
    draw_region = *region;
    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void chapter_strip_draw_focus(egui_view_t *self, const egui_region_t *region, egui_dim_t radius, egui_color_t color)
{
    chapter_strip_draw_stroke(region->location.x - 1, region->location.y - 1, region->size.width + 2, region->size.height + 2, radius, color,
                              egui_color_alpha_mix(self->alpha, 72));
}

static void chapter_strip_draw_title_marker(egui_view_t *self, const egui_region_t *region, egui_color_t accent_color)
{
    egui_dim_t size = 5;
    egui_dim_t x;
    egui_dim_t y;

    if (region == NULL || region->size.width <= 0 || region->size.height <= 0)
    {
        return;
    }

    x = region->location.x;
    y = region->location.y + (region->size.height - size) / 2;
    chapter_strip_draw_fill(x, y, size, size, 2, egui_rgb_mix(EGUI_COLOR_HEX(0xFFFFFF), accent_color, 20), egui_color_alpha_mix(self->alpha, 90));
    chapter_strip_draw_stroke(x, y, size, size, 2, egui_rgb_mix(EGUI_COLOR_HEX(0xC8D3DE), accent_color, 26), egui_color_alpha_mix(self->alpha, 54));
}

static void chapter_strip_draw_title_anchor(egui_view_t *self, const egui_region_t *region, egui_color_t accent_color)
{
    egui_dim_t width = 22;
    egui_dim_t height = 2;

    chapter_strip_draw_fill(region->location.x, region->location.y + region->size.height + 1, width, height, 1,
                            egui_rgb_mix(EGUI_COLOR_HEX(0xFFFFFF), accent_color, 22), egui_color_alpha_mix(self->alpha, 88));
}

static void chapter_strip_format_index_label(uint8_t index, char *buffer, int buffer_size)
{
    int pos = 0;

    if (buffer == NULL || buffer_size <= 0)
    {
        return;
    }
    if (index + 1 < 10)
    {
        pos += egui_sprintf_char(buffer, buffer_size, '0');
    }
    egui_sprintf_int(&buffer[pos], buffer_size - pos, index + 1);
}

static void chapter_strip_format_progress_label(uint8_t current_index, uint8_t chapter_count, char *buffer, int buffer_size)
{
    char current_text[8];
    char total_text[8];
    int pos = 0;

    if (buffer == NULL || buffer_size <= 0 || chapter_count == 0)
    {
        return;
    }

    chapter_strip_format_index_label(current_index, current_text, sizeof(current_text));
    chapter_strip_format_index_label((uint8_t)(chapter_count - 1), total_text, sizeof(total_text));

    pos += egui_sprintf_str(buffer, buffer_size, current_text);
    pos += egui_sprintf_char(&buffer[pos], buffer_size - pos, '/');
    egui_sprintf_str(&buffer[pos], buffer_size - pos, total_text);
}

static void chapter_strip_normalize(egui_view_chapter_strip_t *local)
{
    if (local->items == NULL || local->chapter_count == 0)
    {
        local->items = NULL;
        local->chapter_count = 0;
        local->current_index = 0;
        local->current_part = EGUI_VIEW_CHAPTER_STRIP_PART_NONE;
        local->pressed_part = EGUI_VIEW_CHAPTER_STRIP_PART_NONE;
        local->pressed_index = 0;
        return;
    }
    if (local->chapter_count > EGUI_VIEW_CHAPTER_STRIP_MAX_CHAPTERS)
    {
        local->chapter_count = EGUI_VIEW_CHAPTER_STRIP_MAX_CHAPTERS;
    }
    if (local->current_index >= local->chapter_count)
    {
        local->current_index = (uint8_t)(local->chapter_count - 1);
    }
    if (local->current_part != EGUI_VIEW_CHAPTER_STRIP_PART_PREVIOUS && local->current_part != EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER &&
        local->current_part != EGUI_VIEW_CHAPTER_STRIP_PART_NEXT)
    {
        local->current_part = EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER;
    }
    if (local->compact_mode || local->read_only_mode)
    {
        local->pressed_part = EGUI_VIEW_CHAPTER_STRIP_PART_NONE;
        local->pressed_index = 0;
    }
}

static egui_color_t chapter_strip_get_accent(egui_view_chapter_strip_t *local)
{
    if (local->items != NULL && local->chapter_count > 0)
    {
        return local->items[local->current_index].accent_color;
    }
    return EGUI_COLOR_HEX(0x2563EB);
}

static uint8_t chapter_strip_part_enabled(egui_view_chapter_strip_t *local, egui_view_t *self, uint8_t part)
{
    if (!egui_view_get_enable(self) || local->read_only_mode || local->chapter_count == 0)
    {
        return 0;
    }
    if (part == EGUI_VIEW_CHAPTER_STRIP_PART_PREVIOUS)
    {
        return local->current_index > 0;
    }
    if (part == EGUI_VIEW_CHAPTER_STRIP_PART_NEXT)
    {
        return local->current_index + 1 < local->chapter_count;
    }
    return part == EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER;
}

static void chapter_strip_notify(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);

    if (local->on_changed != NULL)
    {
        local->on_changed(self, local->current_index, local->chapter_count, local->current_part);
    }
}

static void chapter_strip_set_part_inner(egui_view_t *self, uint8_t part, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);

    chapter_strip_normalize(local);
    if (local->chapter_count == 0)
    {
        part = EGUI_VIEW_CHAPTER_STRIP_PART_NONE;
    }
    else if (!chapter_strip_part_enabled(local, self, part) && part != EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER)
    {
        part = EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER;
    }
    if (local->current_part != part)
    {
        local->current_part = part;
        egui_view_invalidate(self);
    }
    if (notify)
    {
        chapter_strip_notify(self);
    }
}

static void chapter_strip_set_index_inner(egui_view_t *self, uint8_t index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);

    chapter_strip_normalize(local);
    if (local->chapter_count == 0)
    {
        return;
    }
    if (index >= local->chapter_count)
    {
        index = (uint8_t)(local->chapter_count - 1);
    }
    if (local->current_index != index)
    {
        local->current_index = index;
        egui_view_invalidate(self);
    }
    if (notify)
    {
        chapter_strip_notify(self);
    }
}

static void chapter_strip_change_index(egui_view_t *self, int delta, uint8_t focus_part)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);
    int next;

    chapter_strip_normalize(local);
    if (local->chapter_count == 0)
    {
        return;
    }
    next = (int)local->current_index + delta;
    if (next < 0)
    {
        next = 0;
    }
    if (next >= local->chapter_count)
    {
        next = (int)local->chapter_count - 1;
    }
    local->current_part = focus_part;
    chapter_strip_set_index_inner(self, (uint8_t)next, 1);
}

static void chapter_strip_get_metrics(egui_view_chapter_strip_t *local, egui_view_t *self, egui_view_chapter_strip_metrics_t *metrics)
{
    egui_region_t work_region;
    egui_dim_t pad_x = local->compact_mode ? CS_COMPACT_PAD_X : CS_STD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? CS_COMPACT_PAD_Y : CS_STD_PAD_Y;
    egui_dim_t button_w = local->compact_mode ? CS_COMPACT_BUTTON_W : CS_STD_BUTTON_W;
    egui_dim_t button_gap = local->compact_mode ? CS_COMPACT_BUTTON_GAP : CS_STD_BUTTON_GAP;
    egui_dim_t row_h = local->compact_mode ? CS_COMPACT_ROW_H : CS_STD_ROW_H;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_w;
    egui_dim_t content_h;
    egui_dim_t summary_h = 0;

    chapter_strip_normalize(local);
    egui_view_get_work_region(self, &work_region);
    content_x = work_region.location.x + pad_x;
    content_y = work_region.location.y + pad_y;
    content_w = work_region.size.width - pad_x * 2;
    content_h = work_region.size.height - pad_y * 2;

    metrics->show_title = (!local->compact_mode && chapter_strip_has_text(local->title)) ? 1 : 0;
    metrics->show_helper = (!local->compact_mode && chapter_strip_has_text(local->helper)) ? 1 : 0;

    metrics->title_region.location.x = content_x;
    metrics->title_region.location.y = content_y;
    metrics->title_region.size.width = content_w;
    metrics->title_region.size.height = CS_STD_TITLE_H;
    if (metrics->show_title)
    {
        content_y += CS_STD_TITLE_H + CS_STD_TITLE_GAP;
        content_h -= CS_STD_TITLE_H + CS_STD_TITLE_GAP;
    }
    if (metrics->show_helper)
    {
        content_h -= CS_STD_HELPER_H + CS_STD_HELPER_GAP;
    }
    if (!local->compact_mode)
    {
        summary_h = content_h - row_h - CS_STD_SUMMARY_GAP;
        if (summary_h > CS_STD_SUMMARY_H)
        {
            summary_h = CS_STD_SUMMARY_H;
        }
        if (summary_h < 20)
        {
            summary_h = 20;
        }
    }
    metrics->summary_region.location.x = content_x;
    metrics->summary_region.location.y = content_y;
    metrics->summary_region.size.width = content_w;
    metrics->summary_region.size.height = summary_h;
    metrics->row_region.location.x = content_x;
    metrics->row_region.location.y = local->compact_mode ? (content_y + (content_h - row_h) / 2) : (content_y + summary_h + CS_STD_SUMMARY_GAP);
    metrics->row_region.size.width = content_w;
    metrics->row_region.size.height = row_h;
    metrics->previous_region.location.x = content_x;
    metrics->previous_region.location.y = metrics->row_region.location.y;
    metrics->previous_region.size.width = button_w;
    metrics->previous_region.size.height = row_h;
    metrics->next_region.location.x = content_x + content_w - button_w;
    metrics->next_region.location.y = metrics->row_region.location.y;
    metrics->next_region.size.width = button_w;
    metrics->next_region.size.height = row_h;
    metrics->rail_region.location.x = metrics->previous_region.location.x + button_w + button_gap;
    metrics->rail_region.location.y = metrics->row_region.location.y;
    metrics->rail_region.size.width = content_w - button_w * 2 - button_gap * 2;
    metrics->rail_region.size.height = row_h;
    metrics->helper_region.location.x = content_x;
    metrics->helper_region.location.y = metrics->row_region.location.y + row_h + CS_STD_HELPER_GAP;
    metrics->helper_region.size.width = content_w;
    metrics->helper_region.size.height = CS_STD_HELPER_H;
}

static uint8_t chapter_strip_get_chapter_region(egui_view_chapter_strip_t *local, egui_view_t *self, uint8_t index, egui_region_t *region)
{
    egui_view_chapter_strip_metrics_t metrics;
    egui_dim_t gap = local->compact_mode ? CS_COMPACT_CHAPTER_GAP : CS_STD_CHAPTER_GAP;
    egui_dim_t available_w;
    egui_dim_t cell_w;
    egui_dim_t remain;
    uint8_t i;

    if (region == NULL || index >= local->chapter_count || local->chapter_count == 0)
    {
        return 0;
    }
    chapter_strip_get_metrics(local, self, &metrics);
    available_w = metrics.rail_region.size.width - gap * (local->chapter_count - 1);
    if (available_w <= 0)
    {
        return 0;
    }
    cell_w = available_w / local->chapter_count;
    remain = available_w - cell_w * local->chapter_count;
    region->location.x = metrics.rail_region.location.x;
    for (i = 0; i < index; i++)
    {
        region->location.x += cell_w + gap + (i < remain ? 1 : 0);
    }
    region->location.y = metrics.rail_region.location.y;
    region->size.width = cell_w + (index < remain ? 1 : 0);
    region->size.height = metrics.rail_region.size.height;
    return 1;
}

static uint8_t chapter_strip_hit_part(egui_view_chapter_strip_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y, uint8_t *index)
{
    egui_view_chapter_strip_metrics_t metrics;
    egui_region_t region;
    uint8_t i;

    if (index != NULL)
    {
        *index = 0;
    }
    chapter_strip_get_metrics(local, self, &metrics);
    if (egui_region_pt_in_rect(&metrics.previous_region, x, y))
    {
        return EGUI_VIEW_CHAPTER_STRIP_PART_PREVIOUS;
    }
    if (egui_region_pt_in_rect(&metrics.next_region, x, y))
    {
        return EGUI_VIEW_CHAPTER_STRIP_PART_NEXT;
    }
    for (i = 0; i < local->chapter_count; i++)
    {
        if (chapter_strip_get_chapter_region(local, self, i, &region) && egui_region_pt_in_rect(&region, x, y))
        {
            if (index != NULL)
            {
                *index = i;
            }
            return EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER;
        }
    }
    return EGUI_VIEW_CHAPTER_STRIP_PART_NONE;
}

static void chapter_strip_draw_chevron(egui_view_t *self, const egui_region_t *region, egui_color_t color, uint8_t is_next)
{
    egui_dim_t cx = region->location.x + region->size.width / 2;
    egui_dim_t cy = region->location.y + region->size.height / 2;

    if (is_next)
    {
        egui_canvas_draw_line(cx - 2, cy - 4, cx + 1, cy, 1, color, egui_color_alpha_mix(self->alpha, 92));
        egui_canvas_draw_line(cx + 1, cy, cx - 2, cy + 4, 1, color, egui_color_alpha_mix(self->alpha, 92));
    }
    else
    {
        egui_canvas_draw_line(cx + 2, cy - 4, cx - 1, cy, 1, color, egui_color_alpha_mix(self->alpha, 92));
        egui_canvas_draw_line(cx - 1, cy, cx + 2, cy + 4, 1, color, egui_color_alpha_mix(self->alpha, 92));
    }
}

static void chapter_strip_draw_compact_summary(egui_view_t *self, egui_view_chapter_strip_t *local, const egui_view_chapter_strip_metrics_t *metrics,
                                               egui_color_t accent_color, egui_color_t text_color)
{
    const egui_view_chapter_strip_item_t *item;
    egui_region_t pill_region;
    egui_dim_t width;
    const char *text;

    if (!local->compact_mode || local->chapter_count == 0)
    {
        return;
    }

    item = &local->items[local->current_index];
    text = chapter_strip_has_text(item->title) ? item->title : item->stamp;
    width = (egui_dim_t)(20 + strlen(text) * 4);
    if (width < 34)
    {
        width = 34;
    }
    if (width > metrics->row_region.size.width - 12)
    {
        width = metrics->row_region.size.width - 12;
    }

    pill_region.size.width = width;
    pill_region.size.height = 12;
    pill_region.location.x = metrics->row_region.location.x + (metrics->row_region.size.width - pill_region.size.width) / 2;
    pill_region.location.y = metrics->row_region.location.y - 14;

    chapter_strip_draw_fill(pill_region.location.x, pill_region.location.y, pill_region.size.width, pill_region.size.height, 6,
                            egui_rgb_mix(local->surface_color, accent_color, local->read_only_mode ? 18 : 26), egui_color_alpha_mix(self->alpha, 92));
    chapter_strip_draw_stroke(pill_region.location.x, pill_region.location.y, pill_region.size.width, pill_region.size.height, 6,
                              egui_rgb_mix(local->border_color, accent_color, local->read_only_mode ? 20 : 30), egui_color_alpha_mix(self->alpha, 52));
    chapter_strip_draw_text(local->meta_font, self, text, &pill_region, EGUI_ALIGN_CENTER, local->read_only_mode ? local->muted_text_color : text_color);
}

static void chapter_strip_draw_row_backdrop(egui_view_t *self, egui_view_chapter_strip_t *local, const egui_view_chapter_strip_metrics_t *metrics,
                                            egui_color_t surface_color, egui_color_t border_color, egui_color_t accent_color)
{
    egui_region_t band_region = metrics->row_region;
    egui_region_t track_region = metrics->rail_region;
    egui_region_t active_region;
    egui_color_t band_fill;
    egui_color_t track_fill;
    egui_color_t bridge_fill;
    egui_dim_t band_inset_x = local->compact_mode ? 1 : 2;
    egui_dim_t band_inset_y = local->compact_mode ? 4 : 6;
    egui_dim_t band_radius = local->compact_mode ? 7 : 9;
    egui_dim_t track_h = local->compact_mode ? 3 : 4;

    band_region.location.x += band_inset_x;
    band_region.size.width -= band_inset_x * 2;
    band_region.location.y += band_inset_y;
    band_region.size.height -= band_inset_y * 2;
    if (band_region.size.width <= 0 || band_region.size.height <= 0)
    {
        return;
    }

    band_fill = egui_rgb_mix(surface_color, local->inactive_color, local->compact_mode ? 18 : 14);
    if (local->read_only_mode)
    {
        band_fill = egui_rgb_mix(band_fill, EGUI_COLOR_HEX(0xEFF3F6), 24);
    }
    chapter_strip_draw_fill(band_region.location.x, band_region.location.y, band_region.size.width, band_region.size.height, band_radius, band_fill,
                            egui_color_alpha_mix(self->alpha, 82));
    chapter_strip_draw_stroke(band_region.location.x, band_region.location.y, band_region.size.width, band_region.size.height, band_radius,
                              egui_rgb_mix(border_color, accent_color, local->read_only_mode ? 8 : 14), egui_color_alpha_mix(self->alpha, 40));

    track_region.location.x += local->compact_mode ? 5 : 6;
    track_region.size.width -= local->compact_mode ? 10 : 12;
    track_region.location.y = band_region.location.y + (band_region.size.height - track_h) / 2;
    track_region.size.height = track_h;
    if (track_region.size.width > 0)
    {
        track_fill = egui_rgb_mix(local->inactive_color, accent_color, local->read_only_mode ? 10 : 18);
        chapter_strip_draw_fill(track_region.location.x, track_region.location.y, track_region.size.width, track_region.size.height, track_h / 2, track_fill,
                                egui_color_alpha_mix(self->alpha, 74));
    }

    if (!chapter_strip_get_chapter_region(local, self, local->current_index, &active_region))
    {
        return;
    }

    active_region.location.y = band_region.location.y + (band_region.size.height - (local->compact_mode ? 7 : 9)) / 2;
    active_region.size.height = local->compact_mode ? 7 : 9;
    bridge_fill = egui_rgb_mix(accent_color, EGUI_COLOR_HEX(0xFFFFFF), local->read_only_mode ? 24 : 14);
    chapter_strip_draw_fill(active_region.location.x, active_region.location.y, active_region.size.width, active_region.size.height,
                            local->compact_mode ? 3 : 4, bridge_fill, egui_color_alpha_mix(self->alpha, 84));

    if (!local->compact_mode && metrics->summary_region.size.height > 0)
    {
        egui_region_t bridge_region;
        egui_region_t cap_region;
        egui_dim_t bridge_bottom = band_region.location.y + 3;
        egui_dim_t bridge_w = 5;

        bridge_region.location.x = active_region.location.x + active_region.size.width / 2 - bridge_w / 2;
        bridge_region.location.y = metrics->summary_region.location.y + metrics->summary_region.size.height - 1;
        bridge_region.size.width = bridge_w;
        bridge_region.size.height = bridge_bottom - bridge_region.location.y;
        if (bridge_region.size.height > 0)
        {
            chapter_strip_draw_fill(bridge_region.location.x, bridge_region.location.y, bridge_region.size.width, bridge_region.size.height, 2,
                                    egui_rgb_mix(surface_color, accent_color, 30), egui_color_alpha_mix(self->alpha, 80));
            cap_region.location.x = bridge_region.location.x - 4;
            cap_region.location.y = bridge_region.location.y;
            cap_region.size.width = bridge_region.size.width + 8;
            cap_region.size.height = 4;
            chapter_strip_draw_fill(cap_region.location.x, cap_region.location.y, cap_region.size.width, cap_region.size.height, 2,
                                    egui_rgb_mix(surface_color, accent_color, 24), egui_color_alpha_mix(self->alpha, 80));
        }
    }
}

static void chapter_strip_draw_helper_strip(egui_view_t *self, egui_view_chapter_strip_t *local, const egui_region_t *region, egui_color_t surface_color,
                                            egui_color_t border_color, egui_color_t accent_color, egui_color_t muted_text_color)
{
    egui_region_t strip_region;
    egui_region_t accent_region;
    egui_region_t text_region;

    if (region == NULL || region->size.width <= 0 || region->size.height <= 0 || !chapter_strip_has_text(local->helper))
    {
        return;
    }

    strip_region = *region;
    strip_region.location.y += 1;
    strip_region.size.height -= 2;
    if (strip_region.size.height <= 0)
    {
        return;
    }

    chapter_strip_draw_fill(strip_region.location.x, strip_region.location.y, strip_region.size.width, strip_region.size.height, 5,
                            egui_rgb_mix(surface_color, accent_color, local->read_only_mode ? 4 : 8), egui_color_alpha_mix(self->alpha, 74));
    chapter_strip_draw_stroke(strip_region.location.x, strip_region.location.y, strip_region.size.width, strip_region.size.height, 5,
                              egui_rgb_mix(border_color, accent_color, local->read_only_mode ? 6 : 12), egui_color_alpha_mix(self->alpha, 28));

    accent_region.location.x = strip_region.location.x + 5;
    accent_region.location.y = strip_region.location.y + strip_region.size.height / 2 - 1;
    accent_region.size.width = 6;
    accent_region.size.height = 2;
    chapter_strip_draw_fill(accent_region.location.x, accent_region.location.y, accent_region.size.width, accent_region.size.height, 1,
                            egui_rgb_mix(accent_color, EGUI_COLOR_HEX(0xFFFFFF), local->read_only_mode ? 22 : 14), egui_color_alpha_mix(self->alpha, 72));

    text_region = strip_region;
    text_region.location.x += 14;
    text_region.size.width -= 16;
    chapter_strip_draw_text(local->meta_font, self, local->helper, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);
}

void egui_view_chapter_strip_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_chapter_strip_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_chapter_strip_set_title(egui_view_t *self, const char *title)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);
    local->title = title;
    egui_view_invalidate(self);
}

void egui_view_chapter_strip_set_helper(egui_view_t *self, const char *helper)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);
    local->helper = helper;
    egui_view_invalidate(self);
}

void egui_view_chapter_strip_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                         egui_color_t muted_text_color, egui_color_t inactive_color)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->inactive_color = inactive_color;
    egui_view_invalidate(self);
}

void egui_view_chapter_strip_set_items(egui_view_t *self, const egui_view_chapter_strip_item_t *items, uint8_t chapter_count, uint8_t current_index)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);
    local->items = items;
    local->chapter_count = chapter_count;
    local->current_index = current_index;
    local->current_part = chapter_count == 0 ? EGUI_VIEW_CHAPTER_STRIP_PART_NONE : EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER;
    chapter_strip_normalize(local);
    egui_view_invalidate(self);
}

uint8_t egui_view_chapter_strip_get_chapter_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);
    chapter_strip_normalize(local);
    return local->chapter_count;
}

uint8_t egui_view_chapter_strip_get_current_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);
    chapter_strip_normalize(local);
    return local->current_index;
}

void egui_view_chapter_strip_set_current_index(egui_view_t *self, uint8_t current_index)
{
    chapter_strip_set_index_inner(self, current_index, 0);
}

void egui_view_chapter_strip_set_current_part(egui_view_t *self, uint8_t part)
{
    chapter_strip_set_part_inner(self, part, 0);
}

uint8_t egui_view_chapter_strip_get_current_part(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);
    chapter_strip_normalize(local);
    return local->current_part;
}

void egui_view_chapter_strip_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);
    local->compact_mode = compact_mode ? 1 : 0;
    chapter_strip_normalize(local);
    egui_view_invalidate(self);
}

void egui_view_chapter_strip_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);
    local->read_only_mode = read_only_mode ? 1 : 0;
    chapter_strip_normalize(local);
    egui_view_invalidate(self);
}

void egui_view_chapter_strip_set_on_changed_listener(egui_view_t *self, egui_view_on_chapter_strip_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);
    local->on_changed = listener;
}

uint8_t egui_view_chapter_strip_get_part_region(egui_view_t *self, uint8_t part, uint8_t index, egui_region_t *region)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);
    egui_view_chapter_strip_metrics_t metrics;

    if (region == NULL)
    {
        return 0;
    }
    chapter_strip_get_metrics(local, self, &metrics);
    if (part == EGUI_VIEW_CHAPTER_STRIP_PART_PREVIOUS)
    {
        *region = metrics.previous_region;
        return 1;
    }
    if (part == EGUI_VIEW_CHAPTER_STRIP_PART_NEXT)
    {
        *region = metrics.next_region;
        return 1;
    }
    if (part == EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER)
    {
        return chapter_strip_get_chapter_region(local, self, index, region);
    }
    return 0;
}

uint8_t egui_view_chapter_strip_handle_navigation_key(egui_view_t *self, uint8_t key_code)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);
    uint8_t next_part;

    chapter_strip_normalize(local);
    if (local->read_only_mode || local->compact_mode || local->chapter_count == 0 || !egui_view_get_enable(self))
    {
        return 0;
    }
    switch (key_code)
    {
    case EGUI_KEY_CODE_LEFT:
        if (local->current_part == EGUI_VIEW_CHAPTER_STRIP_PART_NEXT)
        {
            chapter_strip_set_part_inner(self, EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER, 1);
        }
        else
        {
            chapter_strip_change_index(self, -1, EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER);
        }
        return 1;
    case EGUI_KEY_CODE_RIGHT:
        if (local->current_part == EGUI_VIEW_CHAPTER_STRIP_PART_PREVIOUS)
        {
            chapter_strip_set_part_inner(self, EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER, 1);
        }
        else
        {
            chapter_strip_change_index(self, 1, EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER);
        }
        return 1;
    case EGUI_KEY_CODE_HOME:
        local->current_part = EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER;
        chapter_strip_set_index_inner(self, 0, 1);
        return 1;
    case EGUI_KEY_CODE_END:
        local->current_part = EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER;
        chapter_strip_set_index_inner(self, (uint8_t)(local->chapter_count - 1), 1);
        return 1;
    case EGUI_KEY_CODE_TAB:
        if (local->current_part == EGUI_VIEW_CHAPTER_STRIP_PART_PREVIOUS)
        {
            next_part = EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER;
        }
        else if (local->current_part == EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER)
        {
            next_part = EGUI_VIEW_CHAPTER_STRIP_PART_NEXT;
        }
        else
        {
            next_part = EGUI_VIEW_CHAPTER_STRIP_PART_PREVIOUS;
        }
        chapter_strip_set_part_inner(self, next_part, 1);
        return 1;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        if (local->current_part == EGUI_VIEW_CHAPTER_STRIP_PART_PREVIOUS)
        {
            chapter_strip_change_index(self, -1, EGUI_VIEW_CHAPTER_STRIP_PART_PREVIOUS);
        }
        else if (local->current_part == EGUI_VIEW_CHAPTER_STRIP_PART_NEXT)
        {
            chapter_strip_change_index(self, 1, EGUI_VIEW_CHAPTER_STRIP_PART_NEXT);
        }
        else
        {
            chapter_strip_notify(self);
        }
        return 1;
    case EGUI_KEY_CODE_PLUS:
        chapter_strip_change_index(self, 1, EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER);
        return 1;
    case EGUI_KEY_CODE_MINUS:
        chapter_strip_change_index(self, -1, EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER);
        return 1;
    case EGUI_KEY_CODE_ESCAPE:
        chapter_strip_set_part_inner(self, EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER, 1);
        return 1;
    default:
        return 0;
    }
}

static void egui_view_chapter_strip_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);
    egui_view_chapter_strip_metrics_t metrics;
    egui_color_t surface_color = local->surface_color;
    egui_color_t border_color = local->border_color;
    egui_color_t text_color = local->text_color;
    egui_color_t muted_text_color = local->muted_text_color;
    egui_color_t accent_color;
    egui_region_t previous_draw_region;
    egui_region_t next_draw_region;
    uint8_t previous_enabled;
    uint8_t next_enabled;
    uint8_t i;

    chapter_strip_normalize(local);
    if (local->read_only_mode)
    {
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xEEF2F6), 44);
        border_color = egui_rgb_mix(border_color, EGUI_COLOR_HEX(0x9AA7B4), 38);
        text_color = egui_rgb_mix(text_color, EGUI_COLOR_HEX(0x7B8794), 32);
        muted_text_color = egui_rgb_mix(muted_text_color, EGUI_COLOR_HEX(0x96A3AF), 36);
    }
    accent_color = chapter_strip_get_accent(local);
    if (local->read_only_mode)
    {
        accent_color = egui_rgb_mix(accent_color, EGUI_COLOR_HEX(0x9FB1C5), 36);
    }
    chapter_strip_get_metrics(local, self, &metrics);

    chapter_strip_draw_fill(self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width, self->region_screen.size.height,
                            local->compact_mode ? CS_COMPACT_RADIUS : CS_STD_RADIUS, surface_color, egui_color_alpha_mix(self->alpha, 98));
    chapter_strip_draw_stroke(self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width, self->region_screen.size.height,
                              local->compact_mode ? CS_COMPACT_RADIUS : CS_STD_RADIUS, border_color, egui_color_alpha_mix(self->alpha, 58));

    if (metrics.show_title)
    {
        egui_region_t title_draw_region = metrics.title_region;

        title_draw_region.location.x += 9;
        title_draw_region.size.width -= 9;
        chapter_strip_draw_title_marker(self, &metrics.title_region, accent_color);
        chapter_strip_draw_text(local->meta_font, self, local->title, &title_draw_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);
        chapter_strip_draw_title_anchor(self, &title_draw_region, accent_color);
    }
    if (local->chapter_count == 0)
    {
        egui_region_t empty_region = local->compact_mode ? metrics.row_region : metrics.summary_region;
        chapter_strip_draw_text(local->meta_font, self, "No chapters", &empty_region, EGUI_ALIGN_CENTER, muted_text_color);
        return;
    }
    if (!local->compact_mode)
    {
        egui_region_t eyebrow_region = metrics.summary_region;
        egui_region_t content_region = metrics.summary_region;
        egui_region_t divider_region = metrics.summary_region;
        egui_region_t meta_panel_region = metrics.summary_region;
        egui_region_t title_region = metrics.summary_region;
        egui_region_t stamp_region = metrics.summary_region;
        egui_region_t progress_region = metrics.summary_region;
        const egui_view_chapter_strip_item_t *item = &local->items[local->current_index];
        const char *eyebrow_text = chapter_strip_has_text(item->eyebrow) ? item->eyebrow : "Chapter";
        char progress_text[16];
        egui_dim_t eyebrow_width;

        chapter_strip_draw_fill(metrics.summary_region.location.x, metrics.summary_region.location.y, metrics.summary_region.size.width,
                                metrics.summary_region.size.height, 10, egui_rgb_mix(surface_color, accent_color, 12), egui_color_alpha_mix(self->alpha, 96));
        chapter_strip_draw_stroke(metrics.summary_region.location.x, metrics.summary_region.location.y, metrics.summary_region.size.width,
                                  metrics.summary_region.size.height, 10, egui_rgb_mix(border_color, accent_color, 20), egui_color_alpha_mix(self->alpha, 54));

        meta_panel_region.location.x = metrics.summary_region.location.x + metrics.summary_region.size.width - 46;
        meta_panel_region.location.y += 4;
        meta_panel_region.size.width = 38;
        meta_panel_region.size.height -= 8;

        content_region.location.x += 5;
        content_region.location.y += 3;
        content_region.size.width = meta_panel_region.location.x - content_region.location.x - 6;
        content_region.size.height -= 6;
        chapter_strip_draw_fill(content_region.location.x, content_region.location.y, content_region.size.width, content_region.size.height, 8,
                                egui_rgb_mix(surface_color, accent_color, local->read_only_mode ? 6 : 10), egui_color_alpha_mix(self->alpha, 52));
        chapter_strip_draw_stroke(content_region.location.x, content_region.location.y, content_region.size.width, content_region.size.height, 8,
                                  egui_rgb_mix(border_color, accent_color, local->read_only_mode ? 8 : 14), egui_color_alpha_mix(self->alpha, 26));

        divider_region.location.x = meta_panel_region.location.x - 4;
        divider_region.location.y = metrics.summary_region.location.y + 6;
        divider_region.size.width = 1;
        divider_region.size.height = metrics.summary_region.size.height - 12;
        if (divider_region.size.height > 0)
        {
            egui_canvas_draw_line(divider_region.location.x, divider_region.location.y, divider_region.location.x,
                                  divider_region.location.y + divider_region.size.height - 1, 1,
                                  egui_rgb_mix(border_color, accent_color, local->read_only_mode ? 12 : 20), egui_color_alpha_mix(self->alpha, 56));
        }

        eyebrow_width = (egui_dim_t)(18 + strlen(eyebrow_text) * 4);
        if (eyebrow_width < 34)
        {
            eyebrow_width = 34;
        }
        if (eyebrow_width > 54)
        {
            eyebrow_width = 54;
        }
        if (eyebrow_width > content_region.size.width - 8)
        {
            eyebrow_width = content_region.size.width - 8;
        }
        eyebrow_region.location.x = content_region.location.x + 4;
        eyebrow_region.location.y = content_region.location.y + 3;
        eyebrow_region.size.width = eyebrow_width;
        eyebrow_region.size.height = 12;
        chapter_strip_draw_fill(eyebrow_region.location.x, eyebrow_region.location.y, eyebrow_region.size.width, eyebrow_region.size.height, 6, accent_color,
                                egui_color_alpha_mix(self->alpha, 92));
        chapter_strip_draw_text(local->meta_font, self, eyebrow_text, &eyebrow_region, EGUI_ALIGN_CENTER, EGUI_COLOR_HEX(0xFFFFFF));
        chapter_strip_draw_fill(meta_panel_region.location.x, meta_panel_region.location.y, meta_panel_region.size.width, meta_panel_region.size.height, 7,
                                egui_rgb_mix(surface_color, accent_color, local->read_only_mode ? 10 : 14), egui_color_alpha_mix(self->alpha, 90));
        chapter_strip_draw_stroke(meta_panel_region.location.x, meta_panel_region.location.y, meta_panel_region.size.width, meta_panel_region.size.height, 7,
                                  egui_rgb_mix(border_color, accent_color, local->read_only_mode ? 14 : 18), egui_color_alpha_mix(self->alpha, 44));

        stamp_region.location.x = meta_panel_region.location.x + 3;
        stamp_region.location.y = meta_panel_region.location.y + 3;
        stamp_region.size.width = meta_panel_region.size.width - 6;
        stamp_region.size.height = 12;
        chapter_strip_draw_fill(stamp_region.location.x, stamp_region.location.y, stamp_region.size.width, stamp_region.size.height, 6,
                                egui_rgb_mix(surface_color, accent_color, 18), egui_color_alpha_mix(self->alpha, 92));
        chapter_strip_draw_text(local->meta_font, self, item->stamp, &stamp_region, EGUI_ALIGN_CENTER, text_color);
        chapter_strip_format_progress_label(local->current_index, local->chapter_count, progress_text, sizeof(progress_text));
        progress_region.location.x = meta_panel_region.location.x + 4;
        progress_region.location.y = meta_panel_region.location.y + 18;
        progress_region.size.width = meta_panel_region.size.width - 8;
        progress_region.size.height = 10;
        chapter_strip_draw_fill(progress_region.location.x, progress_region.location.y, progress_region.size.width, progress_region.size.height, 5,
                                egui_rgb_mix(surface_color, accent_color, local->read_only_mode ? 12 : 18), egui_color_alpha_mix(self->alpha, 92));
        chapter_strip_draw_stroke(progress_region.location.x, progress_region.location.y, progress_region.size.width, progress_region.size.height, 5,
                                  egui_rgb_mix(border_color, accent_color, local->read_only_mode ? 16 : 22), egui_color_alpha_mix(self->alpha, 48));
        chapter_strip_draw_text(local->meta_font, self, progress_text, &progress_region, EGUI_ALIGN_CENTER,
                                local->read_only_mode ? muted_text_color : text_color);
        title_region.location.x = content_region.location.x + 5;
        title_region.location.y = content_region.location.y + 14;
        title_region.size.width = content_region.size.width - 10;
        title_region.size.height = content_region.size.height - 14;
        chapter_strip_draw_text(local->font, self, item->title, &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color);
    }
    else
    {
        chapter_strip_draw_compact_summary(self, local, &metrics, accent_color, text_color);
    }

    chapter_strip_draw_row_backdrop(self, local, &metrics, surface_color, border_color, accent_color);

    previous_enabled = chapter_strip_part_enabled(local, self, EGUI_VIEW_CHAPTER_STRIP_PART_PREVIOUS);
    next_enabled = chapter_strip_part_enabled(local, self, EGUI_VIEW_CHAPTER_STRIP_PART_NEXT);
    previous_draw_region = metrics.previous_region;
    next_draw_region = metrics.next_region;
    if (!local->compact_mode)
    {
        previous_draw_region.location.x += 1;
        previous_draw_region.size.width -= 2;
        previous_draw_region.location.y += 4;
        previous_draw_region.size.height -= 8;
        next_draw_region.location.x += 1;
        next_draw_region.size.width -= 2;
        next_draw_region.location.y += 4;
        next_draw_region.size.height -= 8;
    }

    chapter_strip_draw_fill(
            previous_draw_region.location.x, previous_draw_region.location.y, previous_draw_region.size.width, previous_draw_region.size.height,
            local->compact_mode ? 5 : 7,
            egui_rgb_mix(surface_color, accent_color,
                         local->pressed_part == EGUI_VIEW_CHAPTER_STRIP_PART_PREVIOUS ? 22 : (previous_enabled ? (local->compact_mode ? 10 : 7) : 2)),
            egui_color_alpha_mix(self->alpha, local->compact_mode ? 94 : 90));
    chapter_strip_draw_stroke(previous_draw_region.location.x, previous_draw_region.location.y, previous_draw_region.size.width,
                              previous_draw_region.size.height, local->compact_mode ? 5 : 7,
                              egui_rgb_mix(border_color, accent_color, previous_enabled ? (local->compact_mode ? 18 : 12) : 6),
                              egui_color_alpha_mix(self->alpha, local->compact_mode ? 50 : 42));
    chapter_strip_draw_chevron(self, &previous_draw_region,
                               previous_enabled ? egui_rgb_mix(text_color, muted_text_color, local->compact_mode ? 0 : 18) : muted_text_color, 0);

    chapter_strip_draw_fill(next_draw_region.location.x, next_draw_region.location.y, next_draw_region.size.width, next_draw_region.size.height,
                            local->compact_mode ? 5 : 7,
                            egui_rgb_mix(surface_color, accent_color,
                                         local->pressed_part == EGUI_VIEW_CHAPTER_STRIP_PART_NEXT ? 22 : (next_enabled ? (local->compact_mode ? 10 : 7) : 2)),
                            egui_color_alpha_mix(self->alpha, local->compact_mode ? 94 : 90));
    chapter_strip_draw_stroke(next_draw_region.location.x, next_draw_region.location.y, next_draw_region.size.width, next_draw_region.size.height,
                              local->compact_mode ? 5 : 7, egui_rgb_mix(border_color, accent_color, next_enabled ? (local->compact_mode ? 18 : 12) : 6),
                              egui_color_alpha_mix(self->alpha, local->compact_mode ? 50 : 42));
    chapter_strip_draw_chevron(self, &next_draw_region,
                               next_enabled ? egui_rgb_mix(text_color, muted_text_color, local->compact_mode ? 0 : 18) : muted_text_color, 1);

    for (i = 0; i < local->chapter_count; i++)
    {
        egui_region_t region;
        egui_region_t draw_region;
        egui_region_t text_region;
        egui_color_t item_accent = local->items[i].accent_color;
        egui_color_t index_color;
        char index_text[8];
        uint8_t active = i == local->current_index;

        if (!chapter_strip_get_chapter_region(local, self, i, &region))
        {
            continue;
        }
        chapter_strip_format_index_label(i, index_text, sizeof(index_text));
        draw_region = region;
        if (local->compact_mode && !active)
        {
            draw_region.location.y += 3;
            draw_region.size.height -= 6;
        }
        else if (!local->compact_mode && !active)
        {
            draw_region.location.x += 1;
            draw_region.size.width -= 2;
            draw_region.location.y += 4;
            draw_region.size.height -= 7;
        }
        chapter_strip_draw_fill(draw_region.location.x, draw_region.location.y, draw_region.size.width, draw_region.size.height,
                                local->compact_mode ? CS_COMPACT_CHAPTER_R : CS_STD_CHAPTER_R,
                                active ? item_accent : egui_rgb_mix(surface_color, local->inactive_color, local->read_only_mode ? 24 : 38),
                                egui_color_alpha_mix(self->alpha, active ? 96 : (local->compact_mode ? 90 : 86)));
        chapter_strip_draw_stroke(draw_region.location.x, draw_region.location.y, draw_region.size.width, draw_region.size.height,
                                  local->compact_mode ? CS_COMPACT_CHAPTER_R : CS_STD_CHAPTER_R, egui_rgb_mix(border_color, item_accent, active ? 18 : 10),
                                  egui_color_alpha_mix(self->alpha, active ? 54 : (local->compact_mode ? 54 : 46)));
        chapter_strip_draw_fill(draw_region.location.x + (active ? 4 : (local->compact_mode ? 4 : 5)), draw_region.location.y + (active ? 5 : 6),
                                draw_region.size.width - (active ? 8 : (local->compact_mode ? 8 : 10)), local->compact_mode ? 4 : (active ? 5 : 4), 2,
                                active ? EGUI_COLOR_HEX(0xFFFFFF) : egui_rgb_mix(local->inactive_color, item_accent, local->read_only_mode ? 16 : 34),
                                egui_color_alpha_mix(self->alpha, active ? 90 : (local->compact_mode ? 76 : 68)));
        text_region = draw_region;
        text_region.location.y += local->compact_mode ? 4 : 10;
        text_region.size.height = 10;
        index_color = active ? EGUI_COLOR_HEX(0xFFFFFF) : egui_rgb_mix(text_color, muted_text_color, local->read_only_mode ? 26 : 42);
        chapter_strip_draw_text(local->meta_font, self, index_text, &text_region, EGUI_ALIGN_CENTER, index_color);
        if (!local->compact_mode)
        {
            text_region.location.y = region.location.y + 22;
            chapter_strip_draw_text(local->meta_font, self, local->items[i].stamp, &text_region, EGUI_ALIGN_CENTER,
                                    active ? EGUI_COLOR_HEX(0xF1F5F9) : muted_text_color);
        }
        if (local->current_part == EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER && active && !local->read_only_mode)
        {
            chapter_strip_draw_focus(self, &region, local->compact_mode ? CS_COMPACT_CHAPTER_R : CS_STD_CHAPTER_R, accent_color);
        }
    }

    if (local->current_part == EGUI_VIEW_CHAPTER_STRIP_PART_PREVIOUS && !local->read_only_mode)
    {
        chapter_strip_draw_focus(self, local->compact_mode ? &metrics.previous_region : &previous_draw_region, local->compact_mode ? 5 : 7, accent_color);
    }
    else if (local->current_part == EGUI_VIEW_CHAPTER_STRIP_PART_NEXT && !local->read_only_mode)
    {
        chapter_strip_draw_focus(self, local->compact_mode ? &metrics.next_region : &next_draw_region, local->compact_mode ? 5 : 7, accent_color);
    }

    if (metrics.show_helper)
    {
        chapter_strip_draw_helper_strip(self, local, &metrics.helper_region, surface_color, border_color, accent_color, muted_text_color);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_chapter_strip_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_chapter_strip_t);
    uint8_t hit_part;
    uint8_t hit_index = 0;

    chapter_strip_normalize(local);
    if (local->read_only_mode || local->compact_mode || local->chapter_count == 0 || !egui_view_get_enable(self))
    {
        return 0;
    }
    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_part = chapter_strip_hit_part(local, self, event->location.x, event->location.y, &hit_index);
        if (hit_part == EGUI_VIEW_CHAPTER_STRIP_PART_NONE)
        {
            return 0;
        }
        local->pressed_part = hit_part;
        local->pressed_index = hit_index;
        local->current_part = hit_part == EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER ? EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER : hit_part;
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_part = chapter_strip_hit_part(local, self, event->location.x, event->location.y, &hit_index);
        if (local->pressed_part == hit_part)
        {
            if (hit_part == EGUI_VIEW_CHAPTER_STRIP_PART_PREVIOUS)
            {
                chapter_strip_change_index(self, -1, EGUI_VIEW_CHAPTER_STRIP_PART_PREVIOUS);
            }
            else if (hit_part == EGUI_VIEW_CHAPTER_STRIP_PART_NEXT)
            {
                chapter_strip_change_index(self, 1, EGUI_VIEW_CHAPTER_STRIP_PART_NEXT);
            }
            else if (hit_part == EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER && hit_index == local->pressed_index)
            {
                local->current_index = hit_index;
                local->current_part = EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER;
                egui_view_invalidate(self);
                chapter_strip_notify(self);
            }
        }
        local->pressed_part = EGUI_VIEW_CHAPTER_STRIP_PART_NONE;
        local->pressed_index = 0;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_part != EGUI_VIEW_CHAPTER_STRIP_PART_NONE;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_part = EGUI_VIEW_CHAPTER_STRIP_PART_NONE;
        local->pressed_index = 0;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_chapter_strip_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    if (event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        return 0;
    }
    if (egui_view_chapter_strip_handle_navigation_key(self, event->key_code))
    {
        return 1;
    }
    return egui_view_on_key_event(self, event);
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_chapter_strip_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_chapter_strip_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_chapter_strip_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_chapter_strip_on_key_event,
#endif
};

void egui_view_chapter_strip_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_chapter_strip_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_chapter_strip_t);
    self->is_clickable = true;
    egui_view_set_padding_all(self, 2);

    local->on_changed = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->title = NULL;
    local->helper = NULL;
    local->items = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD6DEE7);
    local->text_color = EGUI_COLOR_HEX(0x1A2630);
    local->muted_text_color = EGUI_COLOR_HEX(0x6F7B89);
    local->inactive_color = EGUI_COLOR_HEX(0xAAB6C3);
    local->chapter_count = 0;
    local->current_index = 0;
    local->current_part = EGUI_VIEW_CHAPTER_STRIP_PART_NONE;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    local->pressed_part = EGUI_VIEW_CHAPTER_STRIP_PART_NONE;
    local->pressed_index = 0;

    egui_view_set_view_name(self, "egui_view_chapter_strip");
}
