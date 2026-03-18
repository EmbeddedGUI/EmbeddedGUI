#include <string.h>

#include "egui_view_annotated_scroll_bar.h"
#include "utils/egui_sprintf.h"

#define ASB_STD_RADIUS      12
#define ASB_STD_PAD_X       10
#define ASB_STD_PAD_Y       8
#define ASB_STD_TITLE_H     10
#define ASB_STD_TITLE_GAP   4
#define ASB_STD_HELPER_H    10
#define ASB_STD_HELPER_GAP  5
#define ASB_STD_SUMMARY_W   84
#define ASB_STD_SUMMARY_GAP 8
#define ASB_STD_BAR_W       22
#define ASB_STD_BUTTON_H    18
#define ASB_STD_INDICATOR_H 4
#define ASB_STD_MARKER_H    3
#define ASB_STD_BUBBLE_H    22

#define ASB_COMPACT_RADIUS      9
#define ASB_COMPACT_PAD_X       7
#define ASB_COMPACT_PAD_Y       6
#define ASB_COMPACT_SUMMARY_W   48
#define ASB_COMPACT_SUMMARY_H   18
#define ASB_COMPACT_SUMMARY_GAP 7
#define ASB_COMPACT_BAR_W       18
#define ASB_COMPACT_BUTTON_H    9
#define ASB_COMPACT_INDICATOR_H 3
#define ASB_COMPACT_MARKER_H    3

typedef struct egui_view_annotated_scroll_bar_metrics egui_view_annotated_scroll_bar_metrics_t;
struct egui_view_annotated_scroll_bar_metrics
{
    egui_region_t content_region;
    egui_region_t title_region;
    egui_region_t helper_region;
    egui_region_t body_region;
    egui_region_t summary_region;
    egui_region_t count_region;
    egui_region_t bubble_region;
    egui_region_t rail_panel_region;
    egui_region_t label_column_region;
    egui_region_t bar_column_region;
    egui_region_t decrease_region;
    egui_region_t track_region;
    egui_region_t indicator_region;
    egui_region_t increase_region;
    egui_region_t marker_regions[EGUI_VIEW_ANNOTATED_SCROLL_BAR_MAX_MARKERS];
    egui_region_t marker_label_regions[EGUI_VIEW_ANNOTATED_SCROLL_BAR_MAX_MARKERS];
    uint8_t marker_label_visible[EGUI_VIEW_ANNOTATED_SCROLL_BAR_MAX_MARKERS];
    uint8_t show_title;
    uint8_t show_helper;
};

static uint8_t annotated_scroll_bar_has_text(const char *text)
{
    return (text != NULL && text[0] != '\0') ? 1 : 0;
}

static egui_dim_t annotated_scroll_bar_get_max_offset_inner(egui_view_annotated_scroll_bar_t *local)
{
    return local->content_length > local->viewport_length ? (local->content_length - local->viewport_length) : 0;
}

static egui_dim_t annotated_scroll_bar_clamp_marker_offset(egui_view_annotated_scroll_bar_t *local, egui_dim_t offset)
{
    egui_dim_t max_offset = annotated_scroll_bar_get_max_offset_inner(local);

    if (offset < 0)
    {
        return 0;
    }
    if (offset > max_offset)
    {
        return max_offset;
    }
    return offset;
}

static egui_dim_t annotated_scroll_bar_abs(egui_dim_t value)
{
    return value < 0 ? -value : value;
}

static uint8_t annotated_scroll_bar_find_active_marker(egui_view_annotated_scroll_bar_t *local)
{
    egui_dim_t best_distance;
    uint8_t best_index = 0;
    uint8_t i;

    if (local->markers == NULL || local->marker_count == 0)
    {
        return 0;
    }

    best_distance = annotated_scroll_bar_abs(local->offset - annotated_scroll_bar_clamp_marker_offset(local, local->markers[0].offset));
    for (i = 1; i < local->marker_count; i++)
    {
        egui_dim_t candidate_offset = annotated_scroll_bar_clamp_marker_offset(local, local->markers[i].offset);
        egui_dim_t distance = annotated_scroll_bar_abs(local->offset - candidate_offset);

        if (distance < best_distance)
        {
            best_distance = distance;
            best_index = i;
        }
    }
    return best_index;
}

static void annotated_scroll_bar_normalize_state(egui_view_annotated_scroll_bar_t *local)
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
    if (local->small_change < 1)
    {
        local->small_change = 1;
    }
    if (local->large_change < local->small_change)
    {
        local->large_change = local->viewport_length > local->small_change ? local->viewport_length : local->small_change;
    }
    if (local->marker_count > EGUI_VIEW_ANNOTATED_SCROLL_BAR_MAX_MARKERS)
    {
        local->marker_count = EGUI_VIEW_ANNOTATED_SCROLL_BAR_MAX_MARKERS;
    }
    if (local->markers == NULL)
    {
        local->marker_count = 0;
    }

    max_offset = annotated_scroll_bar_get_max_offset_inner(local);
    if (local->offset < 0)
    {
        local->offset = 0;
    }
    else if (local->offset > max_offset)
    {
        local->offset = max_offset;
    }

    if (local->current_part != EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_DECREASE && local->current_part != EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL &&
        local->current_part != EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_INCREASE)
    {
        local->current_part = EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL;
    }

    if (local->pressed_part != EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_NONE && local->pressed_part != EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_DECREASE &&
        local->pressed_part != EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL && local->pressed_part != EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_INCREASE &&
        local->pressed_part != EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_MARKER)
    {
        local->pressed_part = EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_NONE;
    }
    if (local->pressed_marker >= local->marker_count)
    {
        local->pressed_marker = 0;
    }
    if (local->compact_mode || local->read_only_mode)
    {
        local->pressed_part = EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_NONE;
        local->pressed_marker = 0;
        local->rail_dragging = 0;
    }

    local->active_marker = annotated_scroll_bar_find_active_marker(local);
}

static void annotated_scroll_bar_format_count(uint8_t active_index, uint8_t count, char *buffer, int buffer_size)
{
    int pos = 0;

    if (buffer == NULL || buffer_size <= 0)
    {
        return;
    }
    if (count == 0)
    {
        egui_sprintf_str(buffer, buffer_size, "0 / 0");
        return;
    }
    pos += egui_sprintf_int(buffer, buffer_size, active_index + 1);
    pos += egui_sprintf_str(&buffer[pos], buffer_size - pos, " / ");
    egui_sprintf_int(&buffer[pos], buffer_size - pos, count);
}

static void annotated_scroll_bar_format_offset_pair(egui_dim_t offset, egui_dim_t max_offset, char *buffer, int buffer_size)
{
    int pos = 0;

    pos += egui_sprintf_int(buffer, buffer_size, offset);
    pos += egui_sprintf_str(&buffer[pos], buffer_size - pos, " / ");
    egui_sprintf_int(&buffer[pos], buffer_size - pos, max_offset);
}

static void annotated_scroll_bar_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                           egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (!annotated_scroll_bar_has_text(text) || region->size.width <= 0 || region->size.height <= 0)
    {
        return;
    }
    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void annotated_scroll_bar_draw_focus(egui_view_t *self, const egui_region_t *region, egui_dim_t radius, egui_color_t color)
{
    if (region->size.width <= 0 || region->size.height <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle(region->location.x - 1, region->location.y - 1, region->size.width + 2, region->size.height + 2, radius, 1, color,
                                     egui_color_alpha_mix(self->alpha, 76));
}

static void annotated_scroll_bar_draw_chevron(egui_view_t *self, const egui_region_t *region, egui_color_t color, uint8_t is_increase)
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

static egui_dim_t annotated_scroll_bar_offset_to_track_y(egui_view_annotated_scroll_bar_t *local, const egui_region_t *track_region, egui_dim_t offset)
{
    egui_dim_t max_offset = annotated_scroll_bar_get_max_offset_inner(local);

    if (track_region->size.height <= 0)
    {
        return track_region->location.y;
    }
    if (max_offset <= 0 || track_region->size.height == 1)
    {
        return track_region->location.y + track_region->size.height / 2;
    }
    return track_region->location.y +
           (egui_dim_t)(((int32_t)annotated_scroll_bar_clamp_marker_offset(local, offset) * (track_region->size.height - 1)) / max_offset);
}

static egui_dim_t annotated_scroll_bar_track_y_to_offset(egui_view_annotated_scroll_bar_t *local, const egui_region_t *track_region, egui_dim_t local_y)
{
    egui_dim_t max_offset = annotated_scroll_bar_get_max_offset_inner(local);
    egui_dim_t relative_y;

    if (track_region->size.height <= 1 || max_offset <= 0)
    {
        return 0;
    }

    relative_y = local_y - track_region->location.y;
    if (relative_y < 0)
    {
        relative_y = 0;
    }
    else if (relative_y > track_region->size.height - 1)
    {
        relative_y = track_region->size.height - 1;
    }
    return (egui_dim_t)(((int32_t)relative_y * max_offset) / (track_region->size.height - 1));
}

static void annotated_scroll_bar_get_metrics(egui_view_annotated_scroll_bar_t *local, egui_view_t *self, egui_view_annotated_scroll_bar_metrics_t *metrics)
{
    egui_region_t work_region;
    egui_dim_t pad_x = local->compact_mode ? ASB_COMPACT_PAD_X : ASB_STD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? ASB_COMPACT_PAD_Y : ASB_STD_PAD_Y;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_w;
    egui_dim_t content_h;
    egui_dim_t body_y;
    egui_dim_t body_h;
    egui_dim_t summary_w;
    egui_dim_t bar_w;
    egui_dim_t button_h;
    egui_dim_t indicator_h;
    egui_dim_t marker_h;
    egui_dim_t indicator_y;
    egui_dim_t bubble_h = local->compact_mode ? ASB_COMPACT_SUMMARY_H : ASB_STD_BUBBLE_H;
    egui_dim_t last_visible_bottom = -32000;
    uint8_t i;

    memset(metrics, 0, sizeof(*metrics));

    annotated_scroll_bar_normalize_state(local);
    egui_view_get_work_region(self, &work_region);
    content_x = work_region.location.x + pad_x;
    content_y = work_region.location.y + pad_y;
    content_w = work_region.size.width - pad_x * 2;
    content_h = work_region.size.height - pad_y * 2;
    if (content_w < 0)
    {
        content_w = 0;
    }
    if (content_h < 0)
    {
        content_h = 0;
    }

    metrics->content_region.location.x = content_x;
    metrics->content_region.location.y = content_y;
    metrics->content_region.size.width = content_w;
    metrics->content_region.size.height = content_h;

    metrics->show_title = (!local->compact_mode && annotated_scroll_bar_has_text(local->title)) ? 1 : 0;
    metrics->show_helper = (!local->compact_mode && annotated_scroll_bar_has_text(local->helper)) ? 1 : 0;

    body_y = content_y;
    body_h = content_h;

    if (metrics->show_title)
    {
        metrics->title_region.location.x = content_x;
        metrics->title_region.location.y = content_y;
        metrics->title_region.size.width = content_w;
        metrics->title_region.size.height = ASB_STD_TITLE_H;
        body_y += ASB_STD_TITLE_H + ASB_STD_TITLE_GAP;
        body_h -= ASB_STD_TITLE_H + ASB_STD_TITLE_GAP;
    }

    if (metrics->show_helper)
    {
        metrics->helper_region.location.x = content_x;
        metrics->helper_region.location.y = content_y + content_h - ASB_STD_HELPER_H;
        metrics->helper_region.size.width = content_w;
        metrics->helper_region.size.height = ASB_STD_HELPER_H;
        body_h -= ASB_STD_HELPER_H + ASB_STD_HELPER_GAP;
    }

    if (body_h < 0)
    {
        body_h = 0;
    }
    metrics->body_region.location.x = content_x;
    metrics->body_region.location.y = body_y;
    metrics->body_region.size.width = content_w;
    metrics->body_region.size.height = body_h;

    if (local->compact_mode)
    {
        summary_w = ASB_COMPACT_SUMMARY_W;
        if (summary_w > content_w / 2)
        {
            summary_w = content_w / 2;
        }
        if (summary_w < 32)
        {
            summary_w = 32;
        }
        metrics->summary_region.location.x = content_x;
        metrics->summary_region.location.y = body_y + (body_h - ASB_COMPACT_SUMMARY_H) / 2;
        metrics->summary_region.size.width = summary_w;
        metrics->summary_region.size.height = ASB_COMPACT_SUMMARY_H;
        metrics->count_region = metrics->summary_region;

        metrics->rail_panel_region.location.x = metrics->summary_region.location.x + metrics->summary_region.size.width + ASB_COMPACT_SUMMARY_GAP;
        metrics->rail_panel_region.location.y = body_y;
        metrics->rail_panel_region.size.width = content_x + content_w - metrics->rail_panel_region.location.x;
        metrics->rail_panel_region.size.height = body_h;
        if (metrics->rail_panel_region.size.width < 0)
        {
            metrics->rail_panel_region.size.width = 0;
        }

        bar_w = ASB_COMPACT_BAR_W;
        if (bar_w > metrics->rail_panel_region.size.width)
        {
            bar_w = metrics->rail_panel_region.size.width;
        }
        button_h = ASB_COMPACT_BUTTON_H;
        indicator_h = ASB_COMPACT_INDICATOR_H;
        marker_h = ASB_COMPACT_MARKER_H;

        metrics->bar_column_region.location.x = metrics->rail_panel_region.location.x + (metrics->rail_panel_region.size.width - bar_w) / 2;
        metrics->bar_column_region.location.y = body_y;
        metrics->bar_column_region.size.width = bar_w;
        metrics->bar_column_region.size.height = body_h;
    }
    else
    {
        summary_w = ASB_STD_SUMMARY_W;
        if (summary_w > content_w - 34)
        {
            summary_w = content_w - 34;
        }
        if (summary_w < 56)
        {
            summary_w = 56;
        }
        metrics->summary_region.location.x = content_x;
        metrics->summary_region.location.y = body_y;
        metrics->summary_region.size.width = summary_w;
        metrics->summary_region.size.height = body_h;

        metrics->count_region.location.x = metrics->summary_region.location.x + 5;
        metrics->count_region.location.y = metrics->summary_region.location.y + 5;
        metrics->count_region.size.width = metrics->summary_region.size.width - 10;
        metrics->count_region.size.height = 11;

        metrics->rail_panel_region.location.x = metrics->summary_region.location.x + metrics->summary_region.size.width + ASB_STD_SUMMARY_GAP;
        metrics->rail_panel_region.location.y = body_y;
        metrics->rail_panel_region.size.width = content_x + content_w - metrics->rail_panel_region.location.x;
        metrics->rail_panel_region.size.height = body_h;
        if (metrics->rail_panel_region.size.width < 0)
        {
            metrics->rail_panel_region.size.width = 0;
        }

        bar_w = ASB_STD_BAR_W;
        if (bar_w > metrics->rail_panel_region.size.width)
        {
            bar_w = metrics->rail_panel_region.size.width;
        }
        metrics->bar_column_region.location.x = metrics->rail_panel_region.location.x + metrics->rail_panel_region.size.width - bar_w;
        metrics->bar_column_region.location.y = body_y;
        metrics->bar_column_region.size.width = bar_w;
        metrics->bar_column_region.size.height = body_h;

        metrics->label_column_region.location.x = metrics->rail_panel_region.location.x;
        metrics->label_column_region.location.y = body_y;
        metrics->label_column_region.size.width = metrics->bar_column_region.location.x - metrics->rail_panel_region.location.x - 6;
        metrics->label_column_region.size.height = body_h;
        if (metrics->label_column_region.size.width < 0)
        {
            metrics->label_column_region.size.width = 0;
        }

        button_h = ASB_STD_BUTTON_H;
        indicator_h = ASB_STD_INDICATOR_H;
        marker_h = ASB_STD_MARKER_H;
    }

    if (button_h > metrics->bar_column_region.size.height / 4)
    {
        button_h = metrics->bar_column_region.size.height / 4;
    }
    if (button_h < 8 && metrics->bar_column_region.size.height > 24)
    {
        button_h = 8;
    }

    metrics->decrease_region.location.x = metrics->bar_column_region.location.x;
    metrics->decrease_region.location.y = metrics->bar_column_region.location.y + 1;
    metrics->decrease_region.size.width = metrics->bar_column_region.size.width;
    metrics->decrease_region.size.height = button_h;

    metrics->increase_region.location.x = metrics->bar_column_region.location.x;
    metrics->increase_region.location.y = metrics->bar_column_region.location.y + metrics->bar_column_region.size.height - button_h - 1;
    metrics->increase_region.size.width = metrics->bar_column_region.size.width;
    metrics->increase_region.size.height = button_h;

    metrics->track_region.location.x = metrics->bar_column_region.location.x;
    metrics->track_region.location.y = metrics->decrease_region.location.y + metrics->decrease_region.size.height + 3;
    metrics->track_region.size.width = metrics->bar_column_region.size.width;
    metrics->track_region.size.height = metrics->increase_region.location.y - metrics->track_region.location.y - 3;
    if (metrics->track_region.size.height < 0)
    {
        metrics->track_region.size.height = 0;
    }

    indicator_y = annotated_scroll_bar_offset_to_track_y(local, &metrics->track_region, local->offset) - indicator_h / 2;
    if (indicator_y < metrics->track_region.location.y)
    {
        indicator_y = metrics->track_region.location.y;
    }
    if (indicator_y + indicator_h > metrics->track_region.location.y + metrics->track_region.size.height)
    {
        indicator_y = metrics->track_region.location.y + metrics->track_region.size.height - indicator_h;
    }
    metrics->indicator_region.location.x = metrics->track_region.location.x + 1;
    metrics->indicator_region.location.y = indicator_y;
    metrics->indicator_region.size.width = metrics->track_region.size.width - 2;
    metrics->indicator_region.size.height = indicator_h;
    if (metrics->indicator_region.size.width < 0)
    {
        metrics->indicator_region.size.width = 0;
    }

    if (!local->compact_mode)
    {
        egui_dim_t bubble_y = metrics->indicator_region.location.y + metrics->indicator_region.size.height / 2 - bubble_h / 2;
        egui_dim_t bubble_min_y = metrics->summary_region.location.y + 42;
        egui_dim_t bubble_max_y = metrics->summary_region.location.y + metrics->summary_region.size.height - bubble_h - 8;

        if (bubble_max_y < bubble_min_y)
        {
            bubble_min_y = metrics->summary_region.location.y + 22;
            bubble_max_y = metrics->summary_region.location.y + metrics->summary_region.size.height - bubble_h - 6;
        }
        if (bubble_y < bubble_min_y)
        {
            bubble_y = bubble_min_y;
        }
        if (bubble_y > bubble_max_y)
        {
            bubble_y = bubble_max_y;
        }
        metrics->bubble_region.location.x = metrics->summary_region.location.x + 5;
        metrics->bubble_region.location.y = bubble_y;
        metrics->bubble_region.size.width = metrics->summary_region.size.width - 10;
        metrics->bubble_region.size.height = bubble_h;
    }
    else
    {
        metrics->bubble_region = metrics->summary_region;
    }

    for (i = 0; i < local->marker_count; i++)
    {
        egui_dim_t marker_y = annotated_scroll_bar_offset_to_track_y(local, &metrics->track_region, local->markers[i].offset);
        egui_region_t *marker_region = &metrics->marker_regions[i];
        egui_region_t *label_region = &metrics->marker_label_regions[i];
        uint8_t visible = 0;

        marker_region->location.x = metrics->track_region.location.x + (local->compact_mode ? 4 : 5);
        marker_region->location.y = marker_y - marker_h / 2;
        marker_region->size.width = metrics->track_region.size.width - (local->compact_mode ? 8 : 10);
        marker_region->size.height = marker_h;
        if (marker_region->size.width < 0)
        {
            marker_region->size.width = 0;
        }
        if (!local->compact_mode && metrics->label_column_region.size.width > 0)
        {
            label_region->location.x = metrics->label_column_region.location.x;
            label_region->location.y = marker_y - 5;
            label_region->size.width = metrics->label_column_region.size.width;
            label_region->size.height = 10;
            if (label_region->location.y < metrics->label_column_region.location.y)
            {
                label_region->location.y = metrics->label_column_region.location.y;
            }
            if (label_region->location.y + label_region->size.height > metrics->label_column_region.location.y + metrics->label_column_region.size.height)
            {
                label_region->location.y = metrics->label_column_region.location.y + metrics->label_column_region.size.height - label_region->size.height;
            }
            visible = annotated_scroll_bar_has_text(local->markers[i].label) ? 1 : 0;
            if (visible && i != local->active_marker && i != 0 && i + 1 != local->marker_count && label_region->location.y < last_visible_bottom + 3)
            {
                visible = 0;
            }
            if (visible)
            {
                last_visible_bottom = label_region->location.y + label_region->size.height;
            }
            metrics->marker_label_visible[i] = visible;
        }
    }
}

static egui_color_t annotated_scroll_bar_get_active_accent(egui_view_annotated_scroll_bar_t *local)
{
    if (local->markers != NULL && local->marker_count > 0)
    {
        return egui_rgb_mix(local->accent_color, local->markers[local->active_marker].accent_color, 58);
    }
    return local->accent_color;
}

static void annotated_scroll_bar_notify(egui_view_t *self, uint8_t part)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    if (local->on_changed != NULL)
    {
        local->on_changed(self, local->offset, annotated_scroll_bar_get_max_offset_inner(local), local->active_marker, part);
    }
}

static uint8_t annotated_scroll_bar_apply_offset(egui_view_t *self, egui_dim_t offset, uint8_t notify, uint8_t source_part)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);
    egui_dim_t max_offset = annotated_scroll_bar_get_max_offset_inner(local);
    egui_dim_t old_offset = local->offset;
    uint8_t old_marker = local->active_marker;

    if (offset < 0)
    {
        offset = 0;
    }
    else if (offset > max_offset)
    {
        offset = max_offset;
    }

    local->offset = offset;
    annotated_scroll_bar_normalize_state(local);
    if (old_offset == local->offset && old_marker == local->active_marker)
    {
        return 0;
    }

    egui_view_invalidate(self);
    if (notify)
    {
        annotated_scroll_bar_notify(self, source_part);
    }
    return 1;
}

static void annotated_scroll_bar_set_current_part_inner(egui_view_t *self, uint8_t part)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    if (part != EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_DECREASE && part != EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL &&
        part != EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_INCREASE)
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

static void annotated_scroll_bar_select_marker(egui_view_t *self, uint8_t index, uint8_t notify, uint8_t source_part)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    if (local->markers == NULL || index >= local->marker_count)
    {
        return;
    }

    local->current_part = EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL;
    annotated_scroll_bar_apply_offset(self, local->markers[index].offset, notify, source_part);
}

static void annotated_scroll_bar_drag_rail_to(egui_view_t *self, egui_dim_t local_y)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);
    egui_view_annotated_scroll_bar_metrics_t metrics;

    if (!local->rail_dragging)
    {
        return;
    }

    annotated_scroll_bar_get_metrics(local, self, &metrics);
    annotated_scroll_bar_apply_offset(self, annotated_scroll_bar_track_y_to_offset(local, &metrics.track_region, local_y), 1,
                                      EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
}

static uint8_t annotated_scroll_bar_hit_part(egui_view_annotated_scroll_bar_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y, uint8_t *index)
{
    egui_view_annotated_scroll_bar_metrics_t metrics;
    egui_dim_t local_x = x - self->region_screen.location.x;
    egui_dim_t local_y = y - self->region_screen.location.y;
    uint8_t i;

    if (index != NULL)
    {
        *index = 0;
    }

    annotated_scroll_bar_get_metrics(local, self, &metrics);
    if (egui_region_pt_in_rect(&metrics.decrease_region, local_x, local_y))
    {
        return EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_DECREASE;
    }
    if (egui_region_pt_in_rect(&metrics.increase_region, local_x, local_y))
    {
        return EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_INCREASE;
    }
    for (i = 0; i < local->marker_count; i++)
    {
        if (egui_region_pt_in_rect(&metrics.marker_regions[i], local_x, local_y) ||
            (metrics.marker_label_visible[i] && egui_region_pt_in_rect(&metrics.marker_label_regions[i], local_x, local_y)))
        {
            if (index != NULL)
            {
                *index = i;
            }
            return EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_MARKER;
        }
    }
    if (egui_region_pt_in_rect(&metrics.track_region, local_x, local_y))
    {
        return EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL;
    }
    return EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_NONE;
}

static void annotated_scroll_bar_draw_button(egui_view_t *self, const egui_region_t *region, egui_color_t surface_color, egui_color_t border_color,
                                             egui_color_t text_color, egui_color_t accent_color, uint8_t focused, uint8_t pressed, uint8_t enabled,
                                             uint8_t is_increase)
{
    egui_color_t fill_color = surface_color;
    egui_color_t stroke_color = border_color;
    egui_color_t icon_color = enabled ? text_color : border_color;
    egui_dim_t radius = region->size.height > 12 ? 7 : 5;

    if (pressed)
    {
        fill_color = egui_rgb_mix(fill_color, accent_color, 20);
        stroke_color = egui_rgb_mix(stroke_color, accent_color, 24);
    }
    else if (enabled)
    {
        fill_color = egui_rgb_mix(fill_color, accent_color, 8);
        stroke_color = egui_rgb_mix(stroke_color, accent_color, 12);
    }

    egui_canvas_draw_round_rectangle_fill(region->location.x, region->location.y, region->size.width, region->size.height, radius, fill_color,
                                          egui_color_alpha_mix(self->alpha, 94));
    egui_canvas_draw_round_rectangle(region->location.x, region->location.y, region->size.width, region->size.height, radius, 1, stroke_color,
                                     egui_color_alpha_mix(self->alpha, 48));
    annotated_scroll_bar_draw_chevron(self, region, icon_color, is_increase);
    if (focused)
    {
        annotated_scroll_bar_draw_focus(self, region, radius, accent_color);
    }
}

static void annotated_scroll_bar_draw_summary(egui_view_t *self, egui_view_annotated_scroll_bar_t *local,
                                              const egui_view_annotated_scroll_bar_metrics_t *metrics, egui_color_t surface_color, egui_color_t border_color,
                                              egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color)
{
    egui_region_t label_region;
    egui_region_t meta_region;
    egui_region_t footer_region;
    char count_text[16];
    char offset_text[24];
    const char *active_label = "No markers";
    const char *detail_text = "Add section labels to the rail";

    if (local->marker_count > 0)
    {
        if (annotated_scroll_bar_has_text(local->markers[local->active_marker].label))
        {
            active_label = local->markers[local->active_marker].label;
        }
        if (annotated_scroll_bar_has_text(local->markers[local->active_marker].detail))
        {
            detail_text = local->markers[local->active_marker].detail;
        }
        else
        {
            detail_text = active_label;
        }
    }

    if (local->compact_mode)
    {
        egui_canvas_draw_round_rectangle_fill(metrics->summary_region.location.x, metrics->summary_region.location.y, metrics->summary_region.size.width,
                                              metrics->summary_region.size.height, 6, egui_rgb_mix(surface_color, accent_color, 16),
                                              egui_color_alpha_mix(self->alpha, 94));
        egui_canvas_draw_round_rectangle(metrics->summary_region.location.x, metrics->summary_region.location.y, metrics->summary_region.size.width,
                                         metrics->summary_region.size.height, 6, 1, egui_rgb_mix(border_color, accent_color, 20),
                                         egui_color_alpha_mix(self->alpha, 52));
        annotated_scroll_bar_draw_text(local->meta_font, self, active_label, &metrics->summary_region, EGUI_ALIGN_CENTER,
                                       egui_rgb_mix(text_color, accent_color, 20));
        return;
    }

    egui_canvas_draw_round_rectangle_fill(metrics->summary_region.location.x, metrics->summary_region.location.y, metrics->summary_region.size.width,
                                          metrics->summary_region.size.height, 10, egui_rgb_mix(surface_color, accent_color, 12),
                                          egui_color_alpha_mix(self->alpha, 96));
    egui_canvas_draw_round_rectangle(metrics->summary_region.location.x, metrics->summary_region.location.y, metrics->summary_region.size.width,
                                     metrics->summary_region.size.height, 10, 1, egui_rgb_mix(border_color, accent_color, 22),
                                     egui_color_alpha_mix(self->alpha, 54));

    annotated_scroll_bar_format_count(local->active_marker, local->marker_count, count_text, sizeof(count_text));
    egui_canvas_draw_round_rectangle_fill(metrics->count_region.location.x, metrics->count_region.location.y, metrics->count_region.size.width,
                                          metrics->count_region.size.height, 6, egui_rgb_mix(surface_color, accent_color, 18),
                                          egui_color_alpha_mix(self->alpha, 94));
    egui_canvas_draw_round_rectangle(metrics->count_region.location.x, metrics->count_region.location.y, metrics->count_region.size.width,
                                     metrics->count_region.size.height, 6, 1, egui_rgb_mix(border_color, accent_color, 26),
                                     egui_color_alpha_mix(self->alpha, 48));
    annotated_scroll_bar_draw_text(local->meta_font, self, count_text, &metrics->count_region, EGUI_ALIGN_CENTER, egui_rgb_mix(text_color, accent_color, 12));

    label_region.location.x = metrics->summary_region.location.x + 7;
    label_region.location.y = metrics->summary_region.location.y + 24;
    label_region.size.width = metrics->summary_region.size.width - 14;
    label_region.size.height = 12;
    annotated_scroll_bar_draw_text(local->font, self, active_label, &label_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color);

    meta_region = label_region;
    meta_region.location.y += 12;
    meta_region.size.height = 10;
    annotated_scroll_bar_draw_text(local->meta_font, self, "Current section", &meta_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);

    egui_canvas_draw_round_rectangle_fill(metrics->bubble_region.location.x, metrics->bubble_region.location.y, metrics->bubble_region.size.width,
                                          metrics->bubble_region.size.height, 8, egui_rgb_mix(surface_color, accent_color, 22),
                                          egui_color_alpha_mix(self->alpha, 94));
    egui_canvas_draw_round_rectangle(metrics->bubble_region.location.x, metrics->bubble_region.location.y, metrics->bubble_region.size.width,
                                     metrics->bubble_region.size.height, 8, 1, egui_rgb_mix(border_color, accent_color, 30),
                                     egui_color_alpha_mix(self->alpha, 56));
    annotated_scroll_bar_draw_text(local->meta_font, self, detail_text, &metrics->bubble_region, EGUI_ALIGN_CENTER, text_color);
    egui_canvas_draw_line(metrics->bubble_region.location.x + metrics->bubble_region.size.width,
                          metrics->bubble_region.location.y + metrics->bubble_region.size.height / 2, metrics->track_region.location.x - 3,
                          metrics->indicator_region.location.y + metrics->indicator_region.size.height / 2, 1, egui_rgb_mix(border_color, accent_color, 28),
                          egui_color_alpha_mix(self->alpha, 54));
    egui_canvas_draw_circle_fill(metrics->bubble_region.location.x + metrics->bubble_region.size.width - 2,
                                 metrics->bubble_region.location.y + metrics->bubble_region.size.height / 2, 2, accent_color,
                                 egui_color_alpha_mix(self->alpha, 92));
    egui_canvas_draw_circle_fill(metrics->track_region.location.x - 3, metrics->indicator_region.location.y + metrics->indicator_region.size.height / 2, 1,
                                 accent_color, egui_color_alpha_mix(self->alpha, 84));

    annotated_scroll_bar_format_offset_pair(local->offset, annotated_scroll_bar_get_max_offset_inner(local), offset_text, sizeof(offset_text));
    footer_region.location.x = metrics->summary_region.location.x + 7;
    footer_region.location.y = metrics->summary_region.location.y + metrics->summary_region.size.height - 16;
    footer_region.size.width = metrics->summary_region.size.width - 14;
    footer_region.size.height = 10;
    annotated_scroll_bar_draw_text(local->meta_font, self, offset_text, &footer_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                                   egui_rgb_mix(text_color, muted_text_color, 34));
}

static void annotated_scroll_bar_draw_rail(egui_view_t *self, egui_view_annotated_scroll_bar_t *local, const egui_view_annotated_scroll_bar_metrics_t *metrics,
                                           egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color, egui_color_t muted_text_color,
                                           egui_color_t accent_color)
{
    uint8_t i;
    uint8_t can_decrease = local->offset > 0 ? 1 : 0;
    uint8_t can_increase = local->offset < annotated_scroll_bar_get_max_offset_inner(local) ? 1 : 0;

    egui_canvas_draw_round_rectangle_fill(metrics->track_region.location.x, metrics->track_region.location.y, metrics->track_region.size.width,
                                          metrics->track_region.size.height, metrics->track_region.size.width / 2,
                                          egui_rgb_mix(surface_color, local->preview_color, local->compact_mode ? 18 : 14),
                                          egui_color_alpha_mix(self->alpha, 88));
    egui_canvas_draw_round_rectangle(metrics->track_region.location.x, metrics->track_region.location.y, metrics->track_region.size.width,
                                     metrics->track_region.size.height, metrics->track_region.size.width / 2, 1,
                                     egui_rgb_mix(border_color, accent_color, local->compact_mode ? 12 : 18), egui_color_alpha_mix(self->alpha, 42));

    for (i = 0; i < local->marker_count; i++)
    {
        egui_color_t marker_color = i == local->active_marker ? accent_color : egui_rgb_mix(local->preview_color, border_color, 26);
        egui_region_t draw_region = metrics->marker_regions[i];

        if (i != local->active_marker)
        {
            draw_region.location.x += 1;
            draw_region.size.width -= 2;
        }
        if (draw_region.size.width <= 0)
        {
            continue;
        }
        egui_canvas_draw_round_rectangle_fill(draw_region.location.x, draw_region.location.y, draw_region.size.width, draw_region.size.height,
                                              draw_region.size.height / 2, marker_color,
                                              egui_color_alpha_mix(self->alpha, i == local->active_marker ? 96 : 72));
        if (!local->compact_mode && metrics->marker_label_visible[i])
        {
            egui_region_t label_region = metrics->marker_label_regions[i];
            egui_dim_t line_y = metrics->marker_regions[i].location.y + metrics->marker_regions[i].size.height / 2;
            egui_color_t label_color = i == local->active_marker ? accent_color : muted_text_color;
            egui_color_t connector_color = egui_rgb_mix(border_color, accent_color, i == local->active_marker ? 38 : 12);

            annotated_scroll_bar_draw_text(local->meta_font, self, local->markers[i].label, &label_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER, label_color);
            egui_canvas_draw_line(label_region.location.x + label_region.size.width + 1, line_y, metrics->marker_regions[i].location.x - 2, line_y, 1,
                                  connector_color, egui_color_alpha_mix(self->alpha, i == local->active_marker ? 66 : 54));
            if (i == local->active_marker)
            {
                egui_canvas_draw_circle_fill(label_region.location.x + label_region.size.width + 1, line_y, 1, accent_color,
                                             egui_color_alpha_mix(self->alpha, 88));
            }
        }
    }

    egui_canvas_draw_round_rectangle_fill(metrics->indicator_region.location.x, metrics->indicator_region.location.y, metrics->indicator_region.size.width,
                                          metrics->indicator_region.size.height, metrics->indicator_region.size.height / 2, accent_color,
                                          egui_color_alpha_mix(self->alpha, 98));
    egui_canvas_draw_round_rectangle(metrics->indicator_region.location.x, metrics->indicator_region.location.y, metrics->indicator_region.size.width,
                                     metrics->indicator_region.size.height, metrics->indicator_region.size.height / 2, 1,
                                     egui_rgb_mix(border_color, accent_color, 28), egui_color_alpha_mix(self->alpha, 54));

    annotated_scroll_bar_draw_button(self, &metrics->decrease_region, surface_color, border_color, text_color, accent_color,
                                     local->current_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_DECREASE && !local->read_only_mode,
                                     local->pressed_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_DECREASE, can_decrease, 0);
    annotated_scroll_bar_draw_button(self, &metrics->increase_region, surface_color, border_color, text_color, accent_color,
                                     local->current_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_INCREASE && !local->read_only_mode,
                                     local->pressed_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_INCREASE, can_increase, 1);

    if (local->current_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL && !local->read_only_mode)
    {
        annotated_scroll_bar_draw_focus(self, &metrics->track_region, metrics->track_region.size.width / 2 + 1, accent_color);
    }
}

static void egui_view_annotated_scroll_bar_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);
    egui_view_annotated_scroll_bar_metrics_t metrics;
    egui_color_t surface_color = local->surface_color;
    egui_color_t border_color = local->border_color;
    egui_color_t text_color = local->text_color;
    egui_color_t muted_text_color = local->muted_text_color;
    egui_color_t accent_color;
    egui_dim_t outer_radius = local->compact_mode ? ASB_COMPACT_RADIUS : ASB_STD_RADIUS;

    annotated_scroll_bar_normalize_state(local);
    accent_color = annotated_scroll_bar_get_active_accent(local);

    if (local->read_only_mode)
    {
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xEEF2F6), 42);
        border_color = egui_rgb_mix(border_color, EGUI_COLOR_HEX(0x9AA8B5), 38);
        text_color = egui_rgb_mix(text_color, EGUI_COLOR_HEX(0x748190), 34);
        muted_text_color = egui_rgb_mix(muted_text_color, EGUI_COLOR_HEX(0x8D99A5), 30);
        accent_color = egui_rgb_mix(accent_color, EGUI_COLOR_HEX(0xA7B4C1), 48);
    }

    annotated_scroll_bar_get_metrics(local, self, &metrics);
    egui_canvas_draw_round_rectangle_fill(self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width,
                                          self->region_screen.size.height, outer_radius, surface_color, egui_color_alpha_mix(self->alpha, 96));
    egui_canvas_draw_round_rectangle(self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width,
                                     self->region_screen.size.height, outer_radius, 1, border_color, egui_color_alpha_mix(self->alpha, 58));

    annotated_scroll_bar_draw_text(local->meta_font, self, local->title, &metrics.title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);
    annotated_scroll_bar_draw_text(local->meta_font, self, local->helper, &metrics.helper_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);
    annotated_scroll_bar_draw_summary(self, local, &metrics, surface_color, border_color, text_color, muted_text_color, accent_color);
    annotated_scroll_bar_draw_rail(self, local, &metrics, surface_color, border_color, text_color, muted_text_color, accent_color);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_annotated_scroll_bar_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);
    uint8_t hit_part;
    uint8_t hit_index = 0;
    egui_dim_t local_y = event->location.y - self->region_screen.location.y;

    annotated_scroll_bar_normalize_state(local);
    if (!egui_view_get_enable(self) || local->read_only_mode || local->compact_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_part = annotated_scroll_bar_hit_part(local, self, event->location.x, event->location.y, &hit_index);
        if (hit_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_NONE)
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
        local->pressed_marker = hit_index;
        local->rail_dragging = hit_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL ? 1 : 0;
        egui_view_set_pressed(self, true);
        if (hit_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_DECREASE)
        {
            annotated_scroll_bar_set_current_part_inner(self, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_DECREASE);
        }
        else if (hit_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_INCREASE)
        {
            annotated_scroll_bar_set_current_part_inner(self, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_INCREASE);
        }
        else
        {
            annotated_scroll_bar_set_current_part_inner(self, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
        }
        if (hit_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL)
        {
            annotated_scroll_bar_drag_rail_to(self, local_y);
        }
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        if (local->pressed_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_MARKER)
        {
            local->pressed_part = EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL;
            local->rail_dragging = 1;
            annotated_scroll_bar_set_current_part_inner(self, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
            annotated_scroll_bar_drag_rail_to(self, local_y);
            return 1;
        }
        if (local->pressed_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL)
        {
            annotated_scroll_bar_drag_rail_to(self, local_y);
            return 1;
        }
        return local->pressed_part != EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_NONE;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_part = annotated_scroll_bar_hit_part(local, self, event->location.x, event->location.y, &hit_index);
        if (local->pressed_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_DECREASE && hit_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_DECREASE)
        {
            annotated_scroll_bar_apply_offset(self, local->offset - local->small_change, 1, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_DECREASE);
        }
        else if (local->pressed_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_INCREASE && hit_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_INCREASE)
        {
            annotated_scroll_bar_apply_offset(self, local->offset + local->small_change, 1, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_INCREASE);
        }
        else if (local->pressed_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_MARKER && hit_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_MARKER &&
                 hit_index == local->pressed_marker)
        {
            annotated_scroll_bar_select_marker(self, hit_index, 1, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_MARKER);
        }
        else if (local->pressed_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL)
        {
            annotated_scroll_bar_drag_rail_to(self, local_y);
        }

        local->pressed_part = EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_NONE;
        local->pressed_marker = 0;
        local->rail_dragging = 0;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_part = EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_NONE;
        local->pressed_marker = 0;
        local->rail_dragging = 0;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

uint8_t egui_view_annotated_scroll_bar_handle_navigation_key(egui_view_t *self, uint8_t key_code)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    annotated_scroll_bar_normalize_state(local);
    if (!egui_view_get_enable(self) || local->read_only_mode || local->compact_mode)
    {
        return 0;
    }

    switch (key_code)
    {
    case EGUI_KEY_CODE_TAB:
        if (local->current_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_DECREASE)
        {
            annotated_scroll_bar_set_current_part_inner(self, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
        }
        else if (local->current_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL)
        {
            annotated_scroll_bar_set_current_part_inner(self, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_INCREASE);
        }
        else
        {
            annotated_scroll_bar_set_current_part_inner(self, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_DECREASE);
        }
        return 1;
    case EGUI_KEY_CODE_UP:
        annotated_scroll_bar_set_current_part_inner(self, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
        annotated_scroll_bar_apply_offset(self, local->offset - local->small_change, 1, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
        return 1;
    case EGUI_KEY_CODE_DOWN:
        annotated_scroll_bar_set_current_part_inner(self, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
        annotated_scroll_bar_apply_offset(self, local->offset + local->small_change, 1, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
        return 1;
    case EGUI_KEY_CODE_HOME:
        annotated_scroll_bar_set_current_part_inner(self, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
        annotated_scroll_bar_apply_offset(self, 0, 1, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
        return 1;
    case EGUI_KEY_CODE_END:
        annotated_scroll_bar_set_current_part_inner(self, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
        annotated_scroll_bar_apply_offset(self, annotated_scroll_bar_get_max_offset_inner(local), 1, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
        return 1;
    case EGUI_KEY_CODE_MINUS:
        annotated_scroll_bar_set_current_part_inner(self, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
        annotated_scroll_bar_apply_offset(self, local->offset - local->large_change, 1, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
        return 1;
    case EGUI_KEY_CODE_PLUS:
        annotated_scroll_bar_set_current_part_inner(self, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
        annotated_scroll_bar_apply_offset(self, local->offset + local->large_change, 1, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
        return 1;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        if (local->current_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_DECREASE)
        {
            annotated_scroll_bar_apply_offset(self, local->offset - local->small_change, 1, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_DECREASE);
        }
        else if (local->current_part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_INCREASE)
        {
            annotated_scroll_bar_apply_offset(self, local->offset + local->small_change, 1, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_INCREASE);
        }
        else
        {
            annotated_scroll_bar_apply_offset(self, local->offset + local->large_change, 1, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
        }
        return 1;
    case EGUI_KEY_CODE_ESCAPE:
        if (local->current_part != EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL)
        {
            annotated_scroll_bar_set_current_part_inner(self, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
            return 1;
        }
        return 0;
    default:
        return 0;
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_annotated_scroll_bar_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

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

    if (egui_view_annotated_scroll_bar_handle_navigation_key(self, event->key_code))
    {
        return 1;
    }
    return egui_view_on_key_event(self, event);
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static void egui_view_annotated_scroll_bar_on_focus_change(egui_view_t *self, int is_focused)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);
    uint8_t invalidate = 0;

    if (is_focused)
    {
        return;
    }
    if (local->pressed_part != EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_NONE || local->rail_dragging)
    {
        local->pressed_part = EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_NONE;
        local->pressed_marker = 0;
        local->rail_dragging = 0;
        egui_view_set_pressed(self, false);
        invalidate = 1;
    }
    if (local->current_part != EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL)
    {
        local->current_part = EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL;
        invalidate = 1;
    }
    if (invalidate)
    {
        egui_view_invalidate(self);
    }
}
#endif

void egui_view_annotated_scroll_bar_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    local->font = font == NULL ? (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT : font;
    egui_view_invalidate(self);
}

void egui_view_annotated_scroll_bar_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    local->meta_font = font == NULL ? (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT : font;
    egui_view_invalidate(self);
}

void egui_view_annotated_scroll_bar_set_title(egui_view_t *self, const char *title)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    local->title = title;
    egui_view_invalidate(self);
}

void egui_view_annotated_scroll_bar_set_helper(egui_view_t *self, const char *helper)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    local->helper = helper;
    egui_view_invalidate(self);
}

void egui_view_annotated_scroll_bar_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                                egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t preview_color)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->preview_color = preview_color;
    egui_view_invalidate(self);
}

void egui_view_annotated_scroll_bar_set_markers(egui_view_t *self, const egui_view_annotated_scroll_bar_marker_t *markers, uint8_t marker_count)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    local->markers = markers;
    local->marker_count = marker_count;
    annotated_scroll_bar_normalize_state(local);
    egui_view_invalidate(self);
}

uint8_t egui_view_annotated_scroll_bar_get_marker_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    annotated_scroll_bar_normalize_state(local);
    return local->marker_count;
}

void egui_view_annotated_scroll_bar_set_content_metrics(egui_view_t *self, egui_dim_t content_length, egui_dim_t viewport_length)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);
    egui_dim_t old_offset = local->offset;

    local->content_length = content_length;
    local->viewport_length = viewport_length;
    annotated_scroll_bar_normalize_state(local);
    egui_view_invalidate(self);
    if (old_offset != local->offset && local->on_changed != NULL)
    {
        annotated_scroll_bar_notify(self, EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL);
    }
}

egui_dim_t egui_view_annotated_scroll_bar_get_content_length(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    annotated_scroll_bar_normalize_state(local);
    return local->content_length;
}

egui_dim_t egui_view_annotated_scroll_bar_get_viewport_length(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    annotated_scroll_bar_normalize_state(local);
    return local->viewport_length;
}

void egui_view_annotated_scroll_bar_set_step_size(egui_view_t *self, egui_dim_t small_change, egui_dim_t large_change)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    local->small_change = small_change;
    local->large_change = large_change;
    annotated_scroll_bar_normalize_state(local);
    egui_view_invalidate(self);
}

void egui_view_annotated_scroll_bar_set_offset(egui_view_t *self, egui_dim_t offset)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    annotated_scroll_bar_normalize_state(local);
    annotated_scroll_bar_apply_offset(self, offset, 1, local->current_part);
}

egui_dim_t egui_view_annotated_scroll_bar_get_offset(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    annotated_scroll_bar_normalize_state(local);
    return local->offset;
}

egui_dim_t egui_view_annotated_scroll_bar_get_max_offset(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    annotated_scroll_bar_normalize_state(local);
    return annotated_scroll_bar_get_max_offset_inner(local);
}

uint8_t egui_view_annotated_scroll_bar_get_active_marker(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    annotated_scroll_bar_normalize_state(local);
    return local->active_marker;
}

void egui_view_annotated_scroll_bar_set_current_part(egui_view_t *self, uint8_t part)
{
    annotated_scroll_bar_set_current_part_inner(self, part);
}

uint8_t egui_view_annotated_scroll_bar_get_current_part(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    annotated_scroll_bar_normalize_state(local);
    return local->current_part;
}

void egui_view_annotated_scroll_bar_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    compact_mode = compact_mode ? 1 : 0;
    if (local->compact_mode == compact_mode)
    {
        return;
    }

    local->compact_mode = compact_mode;
    local->pressed_part = EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_NONE;
    local->pressed_marker = 0;
    local->rail_dragging = 0;
    local->current_part = EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL;
    egui_view_set_pressed(self, false);
    egui_view_invalidate(self);
}

void egui_view_annotated_scroll_bar_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);

    read_only_mode = read_only_mode ? 1 : 0;
    if (local->read_only_mode == read_only_mode)
    {
        return;
    }

    local->read_only_mode = read_only_mode;
    local->pressed_part = EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_NONE;
    local->pressed_marker = 0;
    local->rail_dragging = 0;
    local->current_part = EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL;
    egui_view_set_pressed(self, false);
    egui_view_invalidate(self);
}

void egui_view_annotated_scroll_bar_set_on_changed_listener(egui_view_t *self, egui_view_on_annotated_scroll_bar_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);
    local->on_changed = listener;
}

uint8_t egui_view_annotated_scroll_bar_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);
    egui_view_annotated_scroll_bar_metrics_t metrics;

    if (region == NULL)
    {
        return 0;
    }

    annotated_scroll_bar_get_metrics(local, self, &metrics);
    if (part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_DECREASE)
    {
        *region = metrics.decrease_region;
    }
    else if (part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL)
    {
        *region = metrics.track_region;
    }
    else if (part == EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_INCREASE)
    {
        *region = metrics.increase_region;
    }
    else
    {
        return 0;
    }

    region->location.x += self->region_screen.location.x;
    region->location.y += self->region_screen.location.y;
    return 1;
}

uint8_t egui_view_annotated_scroll_bar_get_marker_region(egui_view_t *self, uint8_t index, egui_region_t *region)
{
    EGUI_LOCAL_INIT(egui_view_annotated_scroll_bar_t);
    egui_view_annotated_scroll_bar_metrics_t metrics;

    if (region == NULL)
    {
        return 0;
    }
    annotated_scroll_bar_normalize_state(local);
    if (index >= local->marker_count)
    {
        return 0;
    }

    annotated_scroll_bar_get_metrics(local, self, &metrics);
    *region = metrics.marker_regions[index];
    region->location.x += self->region_screen.location.x;
    region->location.y += self->region_screen.location.y;
    return 1;
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_annotated_scroll_bar_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_annotated_scroll_bar_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_annotated_scroll_bar_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_annotated_scroll_bar_on_key_event,
#endif
};

void egui_view_annotated_scroll_bar_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_annotated_scroll_bar_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_annotated_scroll_bar_t);
    self->is_clickable = true;
    egui_view_set_padding_all(self, 2);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(self, true);
    egui_view_set_on_focus_change_listener(self, egui_view_annotated_scroll_bar_on_focus_change);
#endif

    local->on_changed = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->title = NULL;
    local->helper = NULL;
    local->markers = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD7DFE7);
    local->text_color = EGUI_COLOR_HEX(0x18222D);
    local->muted_text_color = EGUI_COLOR_HEX(0x6D7C8B);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->preview_color = EGUI_COLOR_HEX(0x7CB7FF);
    local->content_length = 960;
    local->viewport_length = 240;
    local->offset = 280;
    local->small_change = 24;
    local->large_change = 120;
    local->marker_count = 0;
    local->active_marker = 0;
    local->current_part = EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_RAIL;
    local->pressed_part = EGUI_VIEW_ANNOTATED_SCROLL_BAR_PART_NONE;
    local->pressed_marker = 0;
    local->rail_dragging = 0;
    local->compact_mode = 0;
    local->read_only_mode = 0;

    egui_view_set_view_name(self, "egui_view_annotated_scroll_bar");
}
