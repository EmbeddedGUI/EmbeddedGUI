#include "egui_view_scroll_bar.h"

#include "utils/egui_sprintf.h"

#define SB_STD_RADIUS         10
#define SB_STD_PAD_X          10
#define SB_STD_PAD_Y          8
#define SB_STD_LABEL_H        10
#define SB_STD_LABEL_GAP      4
#define SB_STD_HELPER_H       10
#define SB_STD_HELPER_GAP     4
#define SB_STD_PREVIEW_W      48
#define SB_STD_BAR_GAP        10
#define SB_STD_SUMMARY_H      12
#define SB_STD_BAR_W          28
#define SB_STD_BUTTON_H       18
#define SB_STD_THUMB_MIN_H    22
#define SB_STD_PREVIEW_BOX_Y  12
#define SB_STD_PREVIEW_FOOTER 10

#define SB_COMPACT_RADIUS      8
#define SB_COMPACT_PAD_X       7
#define SB_COMPACT_PAD_Y       6
#define SB_COMPACT_SUMMARY_W   34
#define SB_COMPACT_SUMMARY_H   18
#define SB_COMPACT_BAR_GAP     6
#define SB_COMPACT_BAR_W       18
#define SB_COMPACT_BUTTON_H    10
#define SB_COMPACT_THUMB_MIN_H 14

typedef struct egui_view_scroll_bar_metrics egui_view_scroll_bar_metrics_t;
struct egui_view_scroll_bar_metrics
{
    egui_region_t content_region;
    egui_region_t label_region;
    egui_region_t helper_region;
    egui_region_t preview_region;
    egui_region_t preview_box_region;
    egui_region_t bar_panel_region;
    egui_region_t summary_region;
    egui_region_t bar_column_region;
    egui_region_t decrease_region;
    egui_region_t track_region;
    egui_region_t thumb_region;
    egui_region_t increase_region;
    uint8_t show_label;
    uint8_t show_helper;
    uint8_t show_preview;
};

static uint8_t scroll_bar_has_text(const char *text)
{
    return (text != NULL && text[0] != '\0') ? 1 : 0;
}

static egui_dim_t scroll_bar_get_max_offset_inner(egui_view_scroll_bar_t *local)
{
    return local->content_length > local->viewport_length ? (local->content_length - local->viewport_length) : 0;
}

static egui_dim_t scroll_bar_get_total_length_inner(egui_view_scroll_bar_t *local)
{
    egui_dim_t total = local->content_length > local->viewport_length ? local->content_length : local->viewport_length;

    return total > 0 ? total : 1;
}

static void scroll_bar_normalize_state(egui_view_scroll_bar_t *local)
{
    egui_dim_t max_offset;

    if (local->content_length < 1)
    {
        local->content_length = 1;
    }
    if (local->viewport_length < 1)
    {
        local->viewport_length = 1;
    }
    if (local->line_step < 1)
    {
        local->line_step = 1;
    }
    if (local->page_step < 1)
    {
        local->page_step = local->viewport_length > local->line_step ? local->viewport_length : local->line_step;
    }

    max_offset = scroll_bar_get_max_offset_inner(local);
    if (local->offset < 0)
    {
        local->offset = 0;
    }
    else if (local->offset > max_offset)
    {
        local->offset = max_offset;
    }

    if (local->current_part != EGUI_VIEW_SCROLL_BAR_PART_DECREASE && local->current_part != EGUI_VIEW_SCROLL_BAR_PART_THUMB &&
        local->current_part != EGUI_VIEW_SCROLL_BAR_PART_INCREASE)
    {
        local->current_part = EGUI_VIEW_SCROLL_BAR_PART_THUMB;
    }

    if (local->pressed_part != EGUI_VIEW_SCROLL_BAR_PART_NONE && local->pressed_part != EGUI_VIEW_SCROLL_BAR_PART_DECREASE &&
        local->pressed_part != EGUI_VIEW_SCROLL_BAR_PART_THUMB && local->pressed_part != EGUI_VIEW_SCROLL_BAR_PART_INCREASE &&
        local->pressed_part != EGUI_VIEW_SCROLL_BAR_PART_TRACK)
    {
        local->pressed_part = EGUI_VIEW_SCROLL_BAR_PART_NONE;
    }

    if (local->compact_mode || local->read_only_mode)
    {
        local->pressed_part = EGUI_VIEW_SCROLL_BAR_PART_NONE;
        local->pressed_track_direction = 0;
        local->thumb_dragging = 0;
    }
}

static void scroll_bar_format_pair(egui_dim_t a, egui_dim_t b, char *buffer, int buffer_size)
{
    int pos = 0;

    pos += egui_sprintf_int(buffer, buffer_size, a);
    pos += egui_sprintf_str(&buffer[pos], buffer_size - pos, " / ");
    egui_sprintf_int(&buffer[pos], buffer_size - pos, b);
}

static void scroll_bar_format_percent(egui_dim_t value, char *buffer, int buffer_size)
{
    int pos = 0;

    pos += egui_sprintf_int(buffer, buffer_size, value);
    egui_sprintf_char(&buffer[pos], buffer_size - pos, '%');
}

static void scroll_bar_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align, egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (!scroll_bar_has_text(text))
    {
        return;
    }

    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void scroll_bar_draw_focus(egui_view_t *self, const egui_region_t *region, egui_dim_t radius, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_draw_round_rectangle(region->location.x - 1, region->location.y - 1, region->size.width + 2, region->size.height + 2, radius, 1, color,
                                     egui_color_alpha_mix(self->alpha, alpha));
}

static void scroll_bar_draw_chevron(egui_view_t *self, const egui_region_t *region, egui_color_t color, uint8_t is_increase)
{
    egui_dim_t cx = region->location.x + region->size.width / 2;
    egui_dim_t cy = region->location.y + region->size.height / 2;
    egui_alpha_t alpha = egui_color_alpha_mix(self->alpha, 92);

    if (is_increase)
    {
        egui_canvas_draw_line(cx - 3, cy - 1, cx, cy + 2, 1, color, alpha);
        egui_canvas_draw_line(cx, cy + 2, cx + 3, cy - 1, 1, color, alpha);
    }
    else
    {
        egui_canvas_draw_line(cx - 3, cy + 1, cx, cy - 2, 1, color, alpha);
        egui_canvas_draw_line(cx, cy - 2, cx + 3, cy + 1, 1, color, alpha);
    }
}

static void scroll_bar_get_metrics(egui_view_scroll_bar_t *local, egui_view_t *self, egui_view_scroll_bar_metrics_t *metrics)
{
    egui_region_t region;
    egui_dim_t pad_x = local->compact_mode ? SB_COMPACT_PAD_X : SB_STD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? SB_COMPACT_PAD_Y : SB_STD_PAD_Y;
    egui_dim_t cursor_y;
    egui_dim_t helper_total = 0;
    egui_dim_t bar_width;
    egui_dim_t button_h;
    egui_dim_t thumb_min_h;
    egui_dim_t total_length;
    egui_dim_t max_offset;
    egui_dim_t thumb_h;
    egui_dim_t thumb_travel;
    egui_dim_t thumb_pos = 0;

    egui_view_get_work_region(self, &region);
    metrics->content_region.location.x = region.location.x + pad_x;
    metrics->content_region.location.y = region.location.y + pad_y;
    metrics->content_region.size.width = region.size.width - pad_x * 2;
    metrics->content_region.size.height = region.size.height - pad_y * 2;
    if (metrics->content_region.size.width < 0)
    {
        metrics->content_region.size.width = 0;
    }
    if (metrics->content_region.size.height < 0)
    {
        metrics->content_region.size.height = 0;
    }

    metrics->show_label = (!local->compact_mode && scroll_bar_has_text(local->label)) ? 1 : 0;
    metrics->show_helper = (!local->compact_mode && scroll_bar_has_text(local->helper)) ? 1 : 0;
    metrics->show_preview = local->compact_mode ? 0 : 1;

    cursor_y = metrics->content_region.location.y;
    if (metrics->show_label)
    {
        metrics->label_region.location.x = metrics->content_region.location.x;
        metrics->label_region.location.y = cursor_y;
        metrics->label_region.size.width = metrics->content_region.size.width;
        metrics->label_region.size.height = SB_STD_LABEL_H;
        cursor_y += SB_STD_LABEL_H + SB_STD_LABEL_GAP;
    }
    else
    {
        metrics->label_region.location.x = 0;
        metrics->label_region.location.y = 0;
        metrics->label_region.size.width = 0;
        metrics->label_region.size.height = 0;
    }

    if (metrics->show_helper)
    {
        helper_total = SB_STD_HELPER_GAP + SB_STD_HELPER_H;
        metrics->helper_region.location.x = metrics->content_region.location.x;
        metrics->helper_region.location.y = metrics->content_region.location.y + metrics->content_region.size.height - SB_STD_HELPER_H;
        metrics->helper_region.size.width = metrics->content_region.size.width;
        metrics->helper_region.size.height = SB_STD_HELPER_H;
    }
    else
    {
        metrics->helper_region.location.x = 0;
        metrics->helper_region.location.y = 0;
        metrics->helper_region.size.width = 0;
        metrics->helper_region.size.height = 0;
    }

    if (local->compact_mode)
    {
        egui_dim_t summary_w = SB_COMPACT_SUMMARY_W;

        if (summary_w > metrics->content_region.size.width / 2)
        {
            summary_w = metrics->content_region.size.width / 2;
        }
        if (summary_w < 24)
        {
            summary_w = 24;
        }

        metrics->summary_region.location.x = metrics->content_region.location.x;
        metrics->summary_region.location.y = metrics->content_region.location.y + (metrics->content_region.size.height - SB_COMPACT_SUMMARY_H) / 2;
        metrics->summary_region.size.width = summary_w;
        metrics->summary_region.size.height = SB_COMPACT_SUMMARY_H;

        metrics->bar_panel_region.location.x = metrics->summary_region.location.x + metrics->summary_region.size.width + SB_COMPACT_BAR_GAP;
        metrics->bar_panel_region.location.y = metrics->content_region.location.y;
        metrics->bar_panel_region.size.width = metrics->content_region.location.x + metrics->content_region.size.width - metrics->bar_panel_region.location.x;
        metrics->bar_panel_region.size.height = metrics->content_region.size.height;

        metrics->preview_region.location.x = 0;
        metrics->preview_region.location.y = 0;
        metrics->preview_region.size.width = 0;
        metrics->preview_region.size.height = 0;
        metrics->preview_box_region = metrics->preview_region;

        bar_width = SB_COMPACT_BAR_W;
        button_h = SB_COMPACT_BUTTON_H;
        thumb_min_h = SB_COMPACT_THUMB_MIN_H;
        metrics->bar_column_region.location.x = metrics->bar_panel_region.location.x + (metrics->bar_panel_region.size.width - bar_width) / 2;
        metrics->bar_column_region.location.y = metrics->bar_panel_region.location.y + 1;
        metrics->bar_column_region.size.width = bar_width;
        metrics->bar_column_region.size.height = metrics->bar_panel_region.size.height - 2;
    }
    else
    {
        egui_dim_t body_height = metrics->content_region.location.y + metrics->content_region.size.height - cursor_y - helper_total;
        egui_dim_t preview_w = SB_STD_PREVIEW_W;

        if (body_height < 0)
        {
            body_height = 0;
        }
        if (preview_w > metrics->content_region.size.width / 2)
        {
            preview_w = metrics->content_region.size.width / 2;
        }
        if (preview_w < 36)
        {
            preview_w = 36;
        }

        metrics->preview_region.location.x = metrics->content_region.location.x;
        metrics->preview_region.location.y = cursor_y;
        metrics->preview_region.size.width = preview_w;
        metrics->preview_region.size.height = body_height;

        metrics->preview_box_region.location.x = metrics->preview_region.location.x;
        metrics->preview_box_region.location.y = metrics->preview_region.location.y + SB_STD_PREVIEW_BOX_Y;
        metrics->preview_box_region.size.width = metrics->preview_region.size.width;
        metrics->preview_box_region.size.height = metrics->preview_region.size.height - SB_STD_PREVIEW_BOX_Y - SB_STD_PREVIEW_FOOTER;
        if (metrics->preview_box_region.size.height < 18)
        {
            metrics->preview_box_region.size.height = 18;
        }

        metrics->bar_panel_region.location.x = metrics->preview_region.location.x + metrics->preview_region.size.width + SB_STD_BAR_GAP;
        metrics->bar_panel_region.location.y = cursor_y;
        metrics->bar_panel_region.size.width = metrics->content_region.location.x + metrics->content_region.size.width - metrics->bar_panel_region.location.x;
        metrics->bar_panel_region.size.height = body_height;

        metrics->summary_region.location.x = metrics->bar_panel_region.location.x;
        metrics->summary_region.location.y = metrics->bar_panel_region.location.y;
        metrics->summary_region.size.width = metrics->bar_panel_region.size.width;
        metrics->summary_region.size.height = SB_STD_SUMMARY_H;

        bar_width = SB_STD_BAR_W;
        button_h = SB_STD_BUTTON_H;
        thumb_min_h = SB_STD_THUMB_MIN_H;
        metrics->bar_column_region.location.x = metrics->bar_panel_region.location.x + (metrics->bar_panel_region.size.width - bar_width) / 2;
        metrics->bar_column_region.location.y = metrics->bar_panel_region.location.y + metrics->summary_region.size.height + 4;
        metrics->bar_column_region.size.width = bar_width;
        metrics->bar_column_region.size.height =
                metrics->bar_panel_region.location.y + metrics->bar_panel_region.size.height - metrics->bar_column_region.location.y;
    }

    if (metrics->bar_panel_region.size.width < 0)
    {
        metrics->bar_panel_region.size.width = 0;
    }
    if (metrics->bar_panel_region.size.height < 0)
    {
        metrics->bar_panel_region.size.height = 0;
    }
    if (metrics->bar_column_region.size.width < 0)
    {
        metrics->bar_column_region.size.width = 0;
    }
    if (metrics->bar_column_region.size.height < 0)
    {
        metrics->bar_column_region.size.height = 0;
    }

    if (button_h > metrics->bar_column_region.size.height / 3)
    {
        button_h = metrics->bar_column_region.size.height / 3;
    }
    if (button_h < 8)
    {
        button_h = metrics->bar_column_region.size.height > 0 ? metrics->bar_column_region.size.height / 3 : 0;
    }

    metrics->decrease_region.location.x = metrics->bar_column_region.location.x;
    metrics->decrease_region.location.y = metrics->bar_column_region.location.y;
    metrics->decrease_region.size.width = metrics->bar_column_region.size.width;
    metrics->decrease_region.size.height = button_h;

    metrics->increase_region.location.x = metrics->bar_column_region.location.x;
    metrics->increase_region.location.y = metrics->bar_column_region.location.y + metrics->bar_column_region.size.height - button_h;
    metrics->increase_region.size.width = metrics->bar_column_region.size.width;
    metrics->increase_region.size.height = button_h;

    metrics->track_region.location.x = metrics->bar_column_region.location.x;
    metrics->track_region.location.y = metrics->decrease_region.location.y + metrics->decrease_region.size.height;
    metrics->track_region.size.width = metrics->bar_column_region.size.width;
    metrics->track_region.size.height = metrics->increase_region.location.y - metrics->track_region.location.y;
    if (metrics->track_region.size.height < 0)
    {
        metrics->track_region.size.height = 0;
    }

    total_length = scroll_bar_get_total_length_inner(local);
    max_offset = scroll_bar_get_max_offset_inner(local);
    thumb_h = metrics->track_region.size.height;
    if (metrics->track_region.size.height > 0)
    {
        thumb_h = (egui_dim_t)(((int32_t)metrics->track_region.size.height * local->viewport_length) / total_length);
        if (thumb_h < thumb_min_h)
        {
            thumb_h = thumb_min_h;
        }
        if (thumb_h > metrics->track_region.size.height)
        {
            thumb_h = metrics->track_region.size.height;
        }
    }
    if (thumb_h < 0)
    {
        thumb_h = 0;
    }

    thumb_travel = metrics->track_region.size.height - thumb_h;
    if (thumb_travel > 0 && max_offset > 0)
    {
        thumb_pos = (egui_dim_t)(((int32_t)local->offset * thumb_travel) / max_offset);
        if (thumb_pos > thumb_travel)
        {
            thumb_pos = thumb_travel;
        }
    }

    metrics->thumb_region.location.x = metrics->track_region.location.x;
    metrics->thumb_region.location.y = metrics->track_region.location.y + thumb_pos;
    metrics->thumb_region.size.width = metrics->track_region.size.width;
    metrics->thumb_region.size.height = thumb_h;
}

static uint8_t scroll_bar_apply_offset(egui_view_t *self, egui_dim_t offset, uint8_t notify, uint8_t source_part)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);
    egui_dim_t max_offset = scroll_bar_get_max_offset_inner(local);
    egui_dim_t old_offset = local->offset;

    if (offset < 0)
    {
        offset = 0;
    }
    else if (offset > max_offset)
    {
        offset = max_offset;
    }

    if (old_offset == offset)
    {
        return 0;
    }

    local->offset = offset;
    egui_view_invalidate(self);
    if (notify && local->on_changed != NULL)
    {
        local->on_changed(self, local->offset, max_offset, source_part);
    }
    return 1;
}

static void scroll_bar_set_current_part_inner(egui_view_t *self, uint8_t part)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);

    if (part != EGUI_VIEW_SCROLL_BAR_PART_DECREASE && part != EGUI_VIEW_SCROLL_BAR_PART_THUMB && part != EGUI_VIEW_SCROLL_BAR_PART_INCREASE)
    {
        return;
    }
    if (local->current_part == part)
    {
        return;
    }

    local->current_part = part;
    egui_view_invalidate(self);
}

static uint8_t scroll_bar_hit_part(egui_view_scroll_bar_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_scroll_bar_metrics_t metrics;
    egui_dim_t local_x = x - self->region_screen.location.x;
    egui_dim_t local_y = y - self->region_screen.location.y;

    scroll_bar_get_metrics(local, self, &metrics);
    if (egui_region_pt_in_rect(&metrics.decrease_region, local_x, local_y))
    {
        return EGUI_VIEW_SCROLL_BAR_PART_DECREASE;
    }
    if (egui_region_pt_in_rect(&metrics.increase_region, local_x, local_y))
    {
        return EGUI_VIEW_SCROLL_BAR_PART_INCREASE;
    }
    if (egui_region_pt_in_rect(&metrics.thumb_region, local_x, local_y))
    {
        return EGUI_VIEW_SCROLL_BAR_PART_THUMB;
    }
    if (egui_region_pt_in_rect(&metrics.track_region, local_x, local_y))
    {
        return EGUI_VIEW_SCROLL_BAR_PART_TRACK;
    }
    return EGUI_VIEW_SCROLL_BAR_PART_NONE;
}

static uint8_t scroll_bar_track_direction_from_point(egui_view_scroll_bar_t *local, egui_view_t *self, egui_dim_t y)
{
    egui_view_scroll_bar_metrics_t metrics;
    egui_dim_t local_y = y - self->region_screen.location.y;

    scroll_bar_get_metrics(local, self, &metrics);
    if (local_y < metrics.thumb_region.location.y)
    {
        return 1;
    }
    if (local_y >= metrics.thumb_region.location.y + metrics.thumb_region.size.height)
    {
        return 2;
    }
    return 0;
}

static void scroll_bar_drag_thumb_to(egui_view_t *self, egui_dim_t local_y)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);
    egui_view_scroll_bar_metrics_t metrics;
    egui_dim_t max_offset = scroll_bar_get_max_offset_inner(local);
    egui_dim_t thumb_travel;
    egui_dim_t delta_y;
    egui_dim_t target_offset;

    if (!local->thumb_dragging)
    {
        return;
    }

    scroll_bar_get_metrics(local, self, &metrics);
    thumb_travel = metrics.track_region.size.height - metrics.thumb_region.size.height;
    if (thumb_travel <= 0 || max_offset <= 0)
    {
        scroll_bar_apply_offset(self, 0, 1, EGUI_VIEW_SCROLL_BAR_PART_THUMB);
        return;
    }

    delta_y = local_y - local->drag_anchor_y;
    target_offset = local->drag_anchor_offset + (egui_dim_t)(((int32_t)delta_y * max_offset) / thumb_travel);
    scroll_bar_apply_offset(self, target_offset, 1, EGUI_VIEW_SCROLL_BAR_PART_THUMB);
}

static void scroll_bar_draw_preview(egui_view_t *self, egui_view_scroll_bar_t *local, const egui_view_scroll_bar_metrics_t *metrics, egui_color_t surface_color,
                                    egui_color_t border_color, egui_color_t text_color, egui_color_t muted_color, egui_color_t accent_color,
                                    egui_color_t preview_color)
{
    egui_region_t preview_box = metrics->preview_box_region;
    egui_region_t preview_inner;
    egui_region_t title_region = metrics->preview_region;
    egui_region_t footer_region = metrics->preview_region;
    egui_dim_t total_length = scroll_bar_get_total_length_inner(local);
    egui_dim_t max_offset = scroll_bar_get_max_offset_inner(local);
    egui_dim_t viewport_h;
    egui_dim_t viewport_y = 0;
    char footer_text[24];
    uint8_t index;

    title_region.size.height = 10;
    footer_region.location.y = metrics->preview_region.location.y + metrics->preview_region.size.height - SB_STD_PREVIEW_FOOTER;
    footer_region.size.height = SB_STD_PREVIEW_FOOTER;

    preview_inner.location.x = preview_box.location.x + 5;
    preview_inner.location.y = preview_box.location.y + 5;
    preview_inner.size.width = preview_box.size.width - 10;
    preview_inner.size.height = preview_box.size.height - 10;
    if (preview_inner.size.width < 4)
    {
        preview_inner.size.width = 4;
    }
    if (preview_inner.size.height < 8)
    {
        preview_inner.size.height = 8;
    }

    egui_canvas_draw_round_rectangle_fill(preview_box.location.x, preview_box.location.y, preview_box.size.width, preview_box.size.height, 7,
                                          egui_rgb_mix(surface_color, preview_color, 10), egui_color_alpha_mix(self->alpha, 92));
    egui_canvas_draw_round_rectangle(preview_box.location.x, preview_box.location.y, preview_box.size.width, preview_box.size.height, 7, 1,
                                     egui_rgb_mix(border_color, preview_color, 18), egui_color_alpha_mix(self->alpha, 58));

    for (index = 0; index < 4; index++)
    {
        egui_dim_t y = preview_inner.location.y + 2 + index * (preview_inner.size.height - 4) / 4;
        egui_dim_t width = preview_inner.size.width - (index % 2 == 0 ? 2 : 6);

        if (width < 4)
        {
            width = 4;
        }
        egui_canvas_draw_rectangle_fill(preview_inner.location.x + 1, y, width, 2, egui_rgb_mix(preview_color, muted_color, 40),
                                        egui_color_alpha_mix(self->alpha, 48));
    }

    viewport_h = (egui_dim_t)(((int32_t)preview_inner.size.height * local->viewport_length) / total_length);
    if (viewport_h < 10)
    {
        viewport_h = 10;
    }
    if (viewport_h > preview_inner.size.height)
    {
        viewport_h = preview_inner.size.height;
    }
    if (preview_inner.size.height > viewport_h && max_offset > 0)
    {
        viewport_y = (egui_dim_t)(((int32_t)local->offset * (preview_inner.size.height - viewport_h)) / max_offset);
    }

    egui_canvas_draw_round_rectangle_fill(preview_inner.location.x, preview_inner.location.y + viewport_y, preview_inner.size.width, viewport_h, 5,
                                          egui_rgb_mix(preview_color, accent_color, 42), egui_color_alpha_mix(self->alpha, 80));
    egui_canvas_draw_round_rectangle(preview_inner.location.x, preview_inner.location.y + viewport_y, preview_inner.size.width, viewport_h, 5, 1,
                                     egui_rgb_mix(accent_color, EGUI_COLOR_WHITE, 10), egui_color_alpha_mix(self->alpha, 76));

    scroll_bar_draw_text(local->meta_font, self, "Viewport", &title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_color);
    scroll_bar_format_pair(local->viewport_length, total_length, footer_text, (int)sizeof(footer_text));
    scroll_bar_draw_text(local->meta_font, self, footer_text, &footer_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, egui_rgb_mix(text_color, muted_color, 34));
}

static void scroll_bar_draw_button(egui_view_t *self, const egui_region_t *region, egui_color_t surface_color, egui_color_t border_color,
                                   egui_color_t icon_color, egui_color_t accent_color, uint8_t focused, uint8_t pressed, uint8_t is_increase)
{
    egui_color_t fill_color = egui_rgb_mix(surface_color, focused ? accent_color : border_color, pressed ? 18 : (focused ? 12 : 8));
    egui_color_t line_color = egui_rgb_mix(border_color, focused ? accent_color : icon_color, focused ? 24 : 12);

    if (focused)
    {
        scroll_bar_draw_focus(self, region, 5, egui_rgb_mix(accent_color, EGUI_COLOR_WHITE, 8), 56);
    }

    egui_canvas_draw_round_rectangle_fill(region->location.x, region->location.y, region->size.width, region->size.height, 5, fill_color,
                                          egui_color_alpha_mix(self->alpha, 94));
    egui_canvas_draw_round_rectangle(region->location.x, region->location.y, region->size.width, region->size.height, 5, 1, line_color,
                                     egui_color_alpha_mix(self->alpha, 60));
    scroll_bar_draw_chevron(self, region, egui_rgb_mix(icon_color, accent_color, focused ? 24 : 10), is_increase);
}

static void scroll_bar_draw_track(egui_view_t *self, egui_view_scroll_bar_t *local, const egui_view_scroll_bar_metrics_t *metrics, egui_color_t surface_color,
                                  egui_color_t border_color, egui_color_t accent_color, egui_color_t preview_color)
{
    egui_region_t thumb_region = metrics->thumb_region;
    egui_color_t track_fill = egui_rgb_mix(surface_color, border_color, 10);
    egui_color_t thumb_fill = scroll_bar_get_max_offset_inner(local) > 0 ? accent_color : egui_rgb_mix(preview_color, border_color, 32);
    egui_color_t thumb_border = egui_rgb_mix(thumb_fill, EGUI_COLOR_WHITE, 8);
    egui_color_t grip_color = egui_rgb_mix(thumb_fill, EGUI_COLOR_WHITE, 26);
    uint8_t thumb_focused = (local->current_part == EGUI_VIEW_SCROLL_BAR_PART_THUMB && !local->read_only_mode && !local->compact_mode) ? 1 : 0;
    uint8_t thumb_pressed = (local->pressed_part == EGUI_VIEW_SCROLL_BAR_PART_THUMB || local->thumb_dragging) ? 1 : 0;

    egui_canvas_draw_round_rectangle_fill(metrics->track_region.location.x, metrics->track_region.location.y, metrics->track_region.size.width,
                                          metrics->track_region.size.height, 6, track_fill, egui_color_alpha_mix(self->alpha, 86));
    egui_canvas_draw_round_rectangle(metrics->track_region.location.x, metrics->track_region.location.y, metrics->track_region.size.width,
                                     metrics->track_region.size.height, 6, 1, egui_rgb_mix(border_color, preview_color, 14),
                                     egui_color_alpha_mix(self->alpha, 48));

    if (local->pressed_part == EGUI_VIEW_SCROLL_BAR_PART_TRACK && local->pressed_track_direction != 0)
    {
        egui_region_t page_region = metrics->track_region;

        if (local->pressed_track_direction == 1)
        {
            page_region.size.height = thumb_region.location.y - page_region.location.y;
        }
        else
        {
            page_region.location.y = thumb_region.location.y + thumb_region.size.height;
            page_region.size.height = metrics->track_region.location.y + metrics->track_region.size.height - page_region.location.y;
        }

        if (page_region.size.height > 0)
        {
            egui_canvas_draw_round_rectangle_fill(page_region.location.x + 2, page_region.location.y, page_region.size.width - 4, page_region.size.height, 4,
                                                  egui_rgb_mix(preview_color, accent_color, 28), egui_color_alpha_mix(self->alpha, 32));
        }
    }

    if (thumb_region.size.height <= 0)
    {
        return;
    }

    if (thumb_focused)
    {
        scroll_bar_draw_focus(self, &thumb_region, 6, egui_rgb_mix(accent_color, EGUI_COLOR_WHITE, 8), 60);
    }

    if (thumb_pressed)
    {
        thumb_fill = egui_rgb_mix(thumb_fill, EGUI_COLOR_WHITE, 8);
        thumb_border = egui_rgb_mix(thumb_border, accent_color, 12);
    }

    egui_canvas_draw_round_rectangle_fill(thumb_region.location.x, thumb_region.location.y, thumb_region.size.width, thumb_region.size.height, 6, thumb_fill,
                                          egui_color_alpha_mix(self->alpha, 94));
    egui_canvas_draw_round_rectangle(thumb_region.location.x, thumb_region.location.y, thumb_region.size.width, thumb_region.size.height, 6, 1, thumb_border,
                                     egui_color_alpha_mix(self->alpha, 78));

    if (thumb_region.size.height > 10)
    {
        egui_dim_t cx = thumb_region.location.x + thumb_region.size.width / 2;
        egui_dim_t cy = thumb_region.location.y + thumb_region.size.height / 2;

        egui_canvas_draw_line(cx - 4, cy - 2, cx + 4, cy - 2, 1, grip_color, egui_color_alpha_mix(self->alpha, 84));
        egui_canvas_draw_line(cx - 4, cy + 2, cx + 4, cy + 2, 1, grip_color, egui_color_alpha_mix(self->alpha, 84));
    }
}

static void egui_view_scroll_bar_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);
    egui_view_scroll_bar_metrics_t metrics;
    egui_region_t region;
    egui_color_t surface_color = local->surface_color;
    egui_color_t border_color = local->border_color;
    egui_color_t text_color = local->text_color;
    egui_color_t muted_color = local->muted_text_color;
    egui_color_t accent_color = local->accent_color;
    egui_color_t preview_color = local->preview_color;
    egui_color_t shadow_color;
    uint8_t enabled = egui_view_get_enable(self) ? 1 : 0;
    char summary_text[24];
    egui_dim_t percent;
    uint8_t decrease_focused;
    uint8_t increase_focused;

    scroll_bar_normalize_state(local);
    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0)
    {
        return;
    }

    if (local->read_only_mode)
    {
        accent_color = egui_rgb_mix(accent_color, muted_color, 64);
        preview_color = egui_rgb_mix(preview_color, muted_color, 52);
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xFBFCFD), 32);
        border_color = egui_rgb_mix(border_color, muted_color, 28);
        text_color = egui_rgb_mix(text_color, muted_color, 34);
    }
    if (!enabled)
    {
        accent_color = egui_rgb_mix(accent_color, muted_color, 72);
        preview_color = egui_rgb_mix(preview_color, muted_color, 64);
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xFBFCFD), 34);
        border_color = egui_rgb_mix(border_color, muted_color, 34);
        text_color = egui_rgb_mix(text_color, muted_color, 40);
        muted_color = egui_rgb_mix(muted_color, surface_color, 14);
    }

    scroll_bar_get_metrics(local, self, &metrics);
    shadow_color = egui_rgb_mix(border_color, surface_color, 36);
    decrease_focused = (local->current_part == EGUI_VIEW_SCROLL_BAR_PART_DECREASE && !local->read_only_mode && !local->compact_mode) ? 1 : 0;
    increase_focused = (local->current_part == EGUI_VIEW_SCROLL_BAR_PART_INCREASE && !local->read_only_mode && !local->compact_mode) ? 1 : 0;

    if (!local->compact_mode)
    {
        egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y + 2, region.size.width, region.size.height, SB_STD_RADIUS + 1, shadow_color,
                                              egui_color_alpha_mix(self->alpha, enabled ? 18 : 10));
    }
    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height,
                                          local->compact_mode ? SB_COMPACT_RADIUS : SB_STD_RADIUS, surface_color,
                                          egui_color_alpha_mix(self->alpha, local->compact_mode ? 94 : 96));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height,
                                     local->compact_mode ? SB_COMPACT_RADIUS : SB_STD_RADIUS, 1, border_color,
                                     egui_color_alpha_mix(self->alpha, local->compact_mode ? 56 : 60));

    if (metrics.show_label)
    {
        scroll_bar_draw_text(local->font, self, local->label, &metrics.label_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color);
    }
    if (metrics.show_helper)
    {
        scroll_bar_draw_text(local->meta_font, self, local->helper, &metrics.helper_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                             egui_rgb_mix(text_color, muted_color, 36));
    }
    if (metrics.show_preview)
    {
        scroll_bar_draw_preview(self, local, &metrics, surface_color, border_color, text_color, muted_color, accent_color, preview_color);
        scroll_bar_format_pair(local->offset, scroll_bar_get_max_offset_inner(local), summary_text, (int)sizeof(summary_text));
        scroll_bar_draw_text(local->meta_font, self, summary_text, &metrics.summary_region, EGUI_ALIGN_CENTER, egui_rgb_mix(text_color, muted_color, 28));
    }
    else
    {
        percent = scroll_bar_get_max_offset_inner(local) > 0 ? (egui_dim_t)(((int32_t)local->offset * 100) / scroll_bar_get_max_offset_inner(local)) : 0;
        scroll_bar_format_percent(percent, summary_text, (int)sizeof(summary_text));
        egui_canvas_draw_round_rectangle_fill(metrics.summary_region.location.x, metrics.summary_region.location.y, metrics.summary_region.size.width,
                                              metrics.summary_region.size.height, 6, egui_rgb_mix(surface_color, preview_color, 12),
                                              egui_color_alpha_mix(self->alpha, 94));
        egui_canvas_draw_round_rectangle(metrics.summary_region.location.x, metrics.summary_region.location.y, metrics.summary_region.size.width,
                                         metrics.summary_region.size.height, 6, 1, egui_rgb_mix(border_color, preview_color, 18),
                                         egui_color_alpha_mix(self->alpha, 60));
        scroll_bar_draw_text(local->meta_font, self, summary_text, &metrics.summary_region, EGUI_ALIGN_CENTER, egui_rgb_mix(text_color, accent_color, 18));
    }

    scroll_bar_draw_button(self, &metrics.decrease_region, surface_color, border_color, text_color, accent_color, decrease_focused,
                           local->pressed_part == EGUI_VIEW_SCROLL_BAR_PART_DECREASE ? 1 : 0, 0);
    scroll_bar_draw_track(self, local, &metrics, surface_color, border_color, accent_color, preview_color);
    scroll_bar_draw_button(self, &metrics.increase_region, surface_color, border_color, text_color, accent_color, increase_focused,
                           local->pressed_part == EGUI_VIEW_SCROLL_BAR_PART_INCREASE ? 1 : 0, 1);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_scroll_bar_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);
    uint8_t hit_part;
    egui_dim_t local_y = event->location.y - self->region_screen.location.y;

    if (!egui_view_get_enable(self) || local->read_only_mode || local->compact_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_part = scroll_bar_hit_part(local, self, event->location.x, event->location.y);
        if (hit_part == EGUI_VIEW_SCROLL_BAR_PART_NONE)
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
        local->pressed_track_direction =
                hit_part == EGUI_VIEW_SCROLL_BAR_PART_TRACK ? scroll_bar_track_direction_from_point(local, self, event->location.y) : 0;
        local->thumb_dragging = hit_part == EGUI_VIEW_SCROLL_BAR_PART_THUMB ? 1 : 0;
        local->drag_anchor_y = local_y;
        local->drag_anchor_offset = local->offset;
        egui_view_set_pressed(self, true);
        if (hit_part == EGUI_VIEW_SCROLL_BAR_PART_DECREASE)
        {
            scroll_bar_set_current_part_inner(self, EGUI_VIEW_SCROLL_BAR_PART_DECREASE);
        }
        else if (hit_part == EGUI_VIEW_SCROLL_BAR_PART_INCREASE)
        {
            scroll_bar_set_current_part_inner(self, EGUI_VIEW_SCROLL_BAR_PART_INCREASE);
        }
        else
        {
            scroll_bar_set_current_part_inner(self, EGUI_VIEW_SCROLL_BAR_PART_THUMB);
        }
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        if (local->pressed_part == EGUI_VIEW_SCROLL_BAR_PART_NONE)
        {
            return 0;
        }
        if (local->thumb_dragging)
        {
            scroll_bar_drag_thumb_to(self, local_y);
        }
        else if (local->pressed_part == EGUI_VIEW_SCROLL_BAR_PART_TRACK)
        {
            local->pressed_track_direction = scroll_bar_track_direction_from_point(local, self, event->location.y);
            egui_view_invalidate(self);
        }
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_part = scroll_bar_hit_part(local, self, event->location.x, event->location.y);
        if (local->thumb_dragging)
        {
            scroll_bar_drag_thumb_to(self, local_y);
        }
        else if (local->pressed_part == EGUI_VIEW_SCROLL_BAR_PART_DECREASE && hit_part == EGUI_VIEW_SCROLL_BAR_PART_DECREASE)
        {
            scroll_bar_apply_offset(self, local->offset - local->line_step, 1, EGUI_VIEW_SCROLL_BAR_PART_DECREASE);
        }
        else if (local->pressed_part == EGUI_VIEW_SCROLL_BAR_PART_INCREASE && hit_part == EGUI_VIEW_SCROLL_BAR_PART_INCREASE)
        {
            scroll_bar_apply_offset(self, local->offset + local->line_step, 1, EGUI_VIEW_SCROLL_BAR_PART_INCREASE);
        }
        else if (local->pressed_part == EGUI_VIEW_SCROLL_BAR_PART_TRACK && hit_part == EGUI_VIEW_SCROLL_BAR_PART_TRACK)
        {
            if (local->pressed_track_direction == 1)
            {
                scroll_bar_apply_offset(self, local->offset - local->page_step, 1, EGUI_VIEW_SCROLL_BAR_PART_TRACK);
            }
            else if (local->pressed_track_direction == 2)
            {
                scroll_bar_apply_offset(self, local->offset + local->page_step, 1, EGUI_VIEW_SCROLL_BAR_PART_TRACK);
            }
        }

        local->pressed_part = EGUI_VIEW_SCROLL_BAR_PART_NONE;
        local->pressed_track_direction = 0;
        local->thumb_dragging = 0;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_part = EGUI_VIEW_SCROLL_BAR_PART_NONE;
        local->pressed_track_direction = 0;
        local->thumb_dragging = 0;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

uint8_t egui_view_scroll_bar_handle_navigation_key(egui_view_t *self, uint8_t key_code)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);

    if (!egui_view_get_enable(self) || local->read_only_mode || local->compact_mode)
    {
        return 0;
    }

    switch (key_code)
    {
    case EGUI_KEY_CODE_TAB:
        if (local->current_part == EGUI_VIEW_SCROLL_BAR_PART_DECREASE)
        {
            scroll_bar_set_current_part_inner(self, EGUI_VIEW_SCROLL_BAR_PART_THUMB);
        }
        else if (local->current_part == EGUI_VIEW_SCROLL_BAR_PART_THUMB)
        {
            scroll_bar_set_current_part_inner(self, EGUI_VIEW_SCROLL_BAR_PART_INCREASE);
        }
        else
        {
            scroll_bar_set_current_part_inner(self, EGUI_VIEW_SCROLL_BAR_PART_DECREASE);
        }
        return 1;
    case EGUI_KEY_CODE_UP:
        scroll_bar_set_current_part_inner(self, EGUI_VIEW_SCROLL_BAR_PART_THUMB);
        scroll_bar_apply_offset(self, local->offset - local->line_step, 1, EGUI_VIEW_SCROLL_BAR_PART_THUMB);
        return 1;
    case EGUI_KEY_CODE_DOWN:
        scroll_bar_set_current_part_inner(self, EGUI_VIEW_SCROLL_BAR_PART_THUMB);
        scroll_bar_apply_offset(self, local->offset + local->line_step, 1, EGUI_VIEW_SCROLL_BAR_PART_THUMB);
        return 1;
    case EGUI_KEY_CODE_HOME:
        scroll_bar_set_current_part_inner(self, EGUI_VIEW_SCROLL_BAR_PART_THUMB);
        scroll_bar_apply_offset(self, 0, 1, EGUI_VIEW_SCROLL_BAR_PART_THUMB);
        return 1;
    case EGUI_KEY_CODE_END:
        scroll_bar_set_current_part_inner(self, EGUI_VIEW_SCROLL_BAR_PART_THUMB);
        scroll_bar_apply_offset(self, scroll_bar_get_max_offset_inner(local), 1, EGUI_VIEW_SCROLL_BAR_PART_THUMB);
        return 1;
    case EGUI_KEY_CODE_MINUS:
        scroll_bar_set_current_part_inner(self, EGUI_VIEW_SCROLL_BAR_PART_THUMB);
        scroll_bar_apply_offset(self, local->offset - local->page_step, 1, EGUI_VIEW_SCROLL_BAR_PART_TRACK);
        return 1;
    case EGUI_KEY_CODE_PLUS:
        scroll_bar_set_current_part_inner(self, EGUI_VIEW_SCROLL_BAR_PART_THUMB);
        scroll_bar_apply_offset(self, local->offset + local->page_step, 1, EGUI_VIEW_SCROLL_BAR_PART_TRACK);
        return 1;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        if (local->current_part == EGUI_VIEW_SCROLL_BAR_PART_DECREASE)
        {
            scroll_bar_apply_offset(self, local->offset - local->line_step, 1, EGUI_VIEW_SCROLL_BAR_PART_DECREASE);
        }
        else if (local->current_part == EGUI_VIEW_SCROLL_BAR_PART_INCREASE)
        {
            scroll_bar_apply_offset(self, local->offset + local->line_step, 1, EGUI_VIEW_SCROLL_BAR_PART_INCREASE);
        }
        else
        {
            scroll_bar_apply_offset(self, local->offset + local->page_step, 1, EGUI_VIEW_SCROLL_BAR_PART_TRACK);
        }
        return 1;
    case EGUI_KEY_CODE_ESCAPE:
        if (local->current_part != EGUI_VIEW_SCROLL_BAR_PART_THUMB)
        {
            scroll_bar_set_current_part_inner(self, EGUI_VIEW_SCROLL_BAR_PART_THUMB);
            return 1;
        }
        return 0;
    default:
        return 0;
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_scroll_bar_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);

    if (!egui_view_get_enable(self) || local->read_only_mode || local->compact_mode)
    {
        return 0;
    }

    if (event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        switch (event->key_code)
        {
        case EGUI_KEY_CODE_TAB:
        case EGUI_KEY_CODE_UP:
        case EGUI_KEY_CODE_DOWN:
        case EGUI_KEY_CODE_HOME:
        case EGUI_KEY_CODE_END:
        case EGUI_KEY_CODE_MINUS:
        case EGUI_KEY_CODE_PLUS:
        case EGUI_KEY_CODE_ENTER:
        case EGUI_KEY_CODE_SPACE:
        case EGUI_KEY_CODE_ESCAPE:
            return 1;
        default:
            return 0;
        }
    }

    if (egui_view_scroll_bar_handle_navigation_key(self, event->key_code))
    {
        return 1;
    }

    return egui_view_on_key_event(self, event);
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static void egui_view_scroll_bar_on_focus_change(egui_view_t *self, int is_focused)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);
    uint8_t needs_invalidate = 0;

    if (is_focused)
    {
        return;
    }

    if (local->pressed_part != EGUI_VIEW_SCROLL_BAR_PART_NONE || local->thumb_dragging)
    {
        local->pressed_part = EGUI_VIEW_SCROLL_BAR_PART_NONE;
        local->pressed_track_direction = 0;
        local->thumb_dragging = 0;
        egui_view_set_pressed(self, false);
        needs_invalidate = 1;
    }
    if (local->current_part != EGUI_VIEW_SCROLL_BAR_PART_THUMB)
    {
        local->current_part = EGUI_VIEW_SCROLL_BAR_PART_THUMB;
        needs_invalidate = 1;
    }
    if (needs_invalidate)
    {
        egui_view_invalidate(self);
    }
}
#endif

void egui_view_scroll_bar_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);

    local->font = font == NULL ? (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT : font;
    egui_view_invalidate(self);
}

void egui_view_scroll_bar_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);

    local->meta_font = font == NULL ? (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT : font;
    egui_view_invalidate(self);
}

void egui_view_scroll_bar_set_label(egui_view_t *self, const char *label)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);

    local->label = label;
    egui_view_invalidate(self);
}

void egui_view_scroll_bar_set_helper(egui_view_t *self, const char *helper)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);

    local->helper = helper;
    egui_view_invalidate(self);
}

void egui_view_scroll_bar_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                      egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t preview_color)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);

    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->preview_color = preview_color;
    egui_view_invalidate(self);
}

void egui_view_scroll_bar_set_content_metrics(egui_view_t *self, egui_dim_t content_length, egui_dim_t viewport_length)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);
    egui_dim_t old_offset = local->offset;

    local->content_length = content_length;
    local->viewport_length = viewport_length;
    scroll_bar_normalize_state(local);
    egui_view_invalidate(self);
    if (old_offset != local->offset && local->on_changed != NULL)
    {
        local->on_changed(self, local->offset, scroll_bar_get_max_offset_inner(local), EGUI_VIEW_SCROLL_BAR_PART_THUMB);
    }
}

egui_dim_t egui_view_scroll_bar_get_content_length(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);
    scroll_bar_normalize_state(local);
    return local->content_length;
}

egui_dim_t egui_view_scroll_bar_get_viewport_length(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);
    scroll_bar_normalize_state(local);
    return local->viewport_length;
}

void egui_view_scroll_bar_set_step_size(egui_view_t *self, egui_dim_t line_step, egui_dim_t page_step)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);

    local->line_step = line_step;
    local->page_step = page_step;
    scroll_bar_normalize_state(local);
    egui_view_invalidate(self);
}

void egui_view_scroll_bar_set_offset(egui_view_t *self, egui_dim_t offset)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);
    scroll_bar_normalize_state(local);
    scroll_bar_apply_offset(self, offset, 1, local->current_part);
}

egui_dim_t egui_view_scroll_bar_get_offset(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);
    scroll_bar_normalize_state(local);
    return local->offset;
}

egui_dim_t egui_view_scroll_bar_get_max_offset(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);
    scroll_bar_normalize_state(local);
    return scroll_bar_get_max_offset_inner(local);
}

void egui_view_scroll_bar_set_current_part(egui_view_t *self, uint8_t part)
{
    scroll_bar_set_current_part_inner(self, part);
}

uint8_t egui_view_scroll_bar_get_current_part(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);
    scroll_bar_normalize_state(local);
    return local->current_part;
}

void egui_view_scroll_bar_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);

    compact_mode = compact_mode ? 1 : 0;
    if (local->compact_mode == compact_mode)
    {
        return;
    }

    local->compact_mode = compact_mode;
    local->pressed_part = EGUI_VIEW_SCROLL_BAR_PART_NONE;
    local->pressed_track_direction = 0;
    local->thumb_dragging = 0;
    local->current_part = EGUI_VIEW_SCROLL_BAR_PART_THUMB;
    egui_view_set_pressed(self, false);
    egui_view_invalidate(self);
}

void egui_view_scroll_bar_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);

    read_only_mode = read_only_mode ? 1 : 0;
    if (local->read_only_mode == read_only_mode)
    {
        return;
    }

    local->read_only_mode = read_only_mode;
    local->pressed_part = EGUI_VIEW_SCROLL_BAR_PART_NONE;
    local->pressed_track_direction = 0;
    local->thumb_dragging = 0;
    local->current_part = EGUI_VIEW_SCROLL_BAR_PART_THUMB;
    egui_view_set_pressed(self, false);
    egui_view_invalidate(self);
}

void egui_view_scroll_bar_set_on_changed_listener(egui_view_t *self, egui_view_on_scroll_bar_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);
    local->on_changed = listener;
}

uint8_t egui_view_scroll_bar_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region)
{
    EGUI_LOCAL_INIT(egui_view_scroll_bar_t);
    egui_view_scroll_bar_metrics_t metrics;

    if (region == NULL)
    {
        return 0;
    }

    scroll_bar_normalize_state(local);
    scroll_bar_get_metrics(local, self, &metrics);

    if (part == EGUI_VIEW_SCROLL_BAR_PART_DECREASE)
    {
        *region = metrics.decrease_region;
    }
    else if (part == EGUI_VIEW_SCROLL_BAR_PART_THUMB)
    {
        *region = metrics.thumb_region;
    }
    else if (part == EGUI_VIEW_SCROLL_BAR_PART_INCREASE)
    {
        *region = metrics.increase_region;
    }
    else if (part == EGUI_VIEW_SCROLL_BAR_PART_TRACK)
    {
        *region = metrics.track_region;
    }
    else
    {
        return 0;
    }

    region->location.x += self->region_screen.location.x;
    region->location.y += self->region_screen.location.y;
    return 1;
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_scroll_bar_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_scroll_bar_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_scroll_bar_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_scroll_bar_on_key_event,
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        .on_focus_changed = egui_view_scroll_bar_on_focus_change,
#endif
};

void egui_view_scroll_bar_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_scroll_bar_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_scroll_bar_t);
    egui_view_set_padding_all(self, 2);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(self, true);
#endif

    local->on_changed = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->label = NULL;
    local->helper = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD7DFE7);
    local->text_color = EGUI_COLOR_HEX(0x18222D);
    local->muted_text_color = EGUI_COLOR_HEX(0x6D7C8B);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->preview_color = EGUI_COLOR_HEX(0x58A6FF);
    local->content_length = 840;
    local->viewport_length = 220;
    local->offset = 168;
    local->line_step = 24;
    local->page_step = 160;
    local->drag_anchor_y = 0;
    local->drag_anchor_offset = 0;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    local->current_part = EGUI_VIEW_SCROLL_BAR_PART_THUMB;
    local->pressed_part = EGUI_VIEW_SCROLL_BAR_PART_NONE;
    local->pressed_track_direction = 0;
    local->thumb_dragging = 0;

    egui_view_set_view_name(self, "egui_view_scroll_bar");
}
