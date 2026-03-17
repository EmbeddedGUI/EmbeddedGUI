#include <string.h>

#include "egui_view_transport_bar.h"
#include "utils/egui_sprintf.h"

#define TB_STD_RADIUS      12
#define TB_STD_PAD_X       10
#define TB_STD_PAD_Y       9
#define TB_STD_TITLE_H     10
#define TB_STD_TITLE_GAP   5
#define TB_STD_HELPER_H    10
#define TB_STD_HELPER_GAP  5
#define TB_STD_SHADOW_X    2
#define TB_STD_SHADOW_Y    4
#define TB_STD_CARD_PAD_X  12
#define TB_STD_CARD_PAD_Y  10
#define TB_STD_BUTTON_YOFF 9
#define TB_STD_SEEK_GAP    8
#define TB_STD_PREV_SIZE   18
#define TB_STD_PLAY_SIZE   24
#define TB_STD_NEXT_SIZE   18
#define TB_STD_BTN_GAP     10

#define TB_COMPACT_RADIUS      9
#define TB_COMPACT_PAD_X       7
#define TB_COMPACT_PAD_Y       6
#define TB_COMPACT_CARD_PAD_X  8
#define TB_COMPACT_CARD_PAD_Y  7
#define TB_COMPACT_BUTTON_YOFF 7
#define TB_COMPACT_SEEK_GAP    6
#define TB_COMPACT_PREV_SIZE   14
#define TB_COMPACT_PLAY_SIZE   18
#define TB_COMPACT_NEXT_SIZE   14
#define TB_COMPACT_BTN_GAP     7

#define TB_SEEK_STEP_SECONDS 10
#define TB_SKIP_STEP_SECONDS 30

typedef struct egui_view_transport_bar_metrics egui_view_transport_bar_metrics_t;
struct egui_view_transport_bar_metrics
{
    egui_region_t title_region;
    egui_region_t card_region;
    egui_region_t helper_region;
    egui_region_t previous_region;
    egui_region_t play_pause_region;
    egui_region_t next_region;
    egui_region_t seek_region;
    egui_region_t headline_region;
    egui_region_t body_region;
    egui_region_t footer_region;
    uint8_t show_title;
    uint8_t show_helper;
};

static uint8_t transport_bar_has_text(const char *text)
{
    return (text != NULL && text[0] != '\0') ? 1 : 0;
}

static void transport_bar_draw_round_fill_safe(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t radius, egui_color_t color,
                                               egui_alpha_t alpha)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle_fill(x, y, w, h, radius, color, alpha);
}

static void transport_bar_draw_round_stroke_safe(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t radius, egui_dim_t stroke_width,
                                                 egui_color_t color, egui_alpha_t alpha)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle(x, y, w, h, radius, stroke_width, color, alpha);
}

static void transport_bar_format_single_time(uint16_t seconds, char *buffer, int buffer_size)
{
    int pos = 0;
    uint16_t minutes = (uint16_t)(seconds / 60);
    uint16_t remainder = (uint16_t)(seconds % 60);

    if (buffer == NULL || buffer_size <= 0)
    {
        return;
    }

    if (minutes >= 60)
    {
        uint16_t hours = (uint16_t)(minutes / 60);
        minutes = (uint16_t)(minutes % 60);
        pos += egui_sprintf_int(buffer, buffer_size, hours);
        pos += egui_sprintf_char(&buffer[pos], buffer_size - pos, ':');
        pos += egui_sprintf_int_pad(&buffer[pos], buffer_size - pos, minutes, 2, '0');
    }
    else
    {
        pos += egui_sprintf_int(buffer, buffer_size, minutes);
    }
    pos += egui_sprintf_char(&buffer[pos], buffer_size - pos, ':');
    egui_sprintf_int_pad(&buffer[pos], buffer_size - pos, remainder, 2, '0');
}

static void transport_bar_format_time_pair(uint16_t position_seconds, uint16_t duration_seconds, char *buffer, int buffer_size)
{
    char left[12];
    char right[12];
    int pos = 0;

    if (buffer == NULL || buffer_size <= 0)
    {
        return;
    }

    transport_bar_format_single_time(position_seconds, left, sizeof(left));
    transport_bar_format_single_time(duration_seconds, right, sizeof(right));
    pos += egui_sprintf_str(buffer, buffer_size, left);
    pos += egui_sprintf_str(&buffer[pos], buffer_size - pos, " / ");
    egui_sprintf_str(&buffer[pos], buffer_size - pos, right);
}

static void transport_bar_normalize_state(egui_view_transport_bar_t *local)
{
    if (local->snapshot == NULL)
    {
        local->position_seconds = 0;
        local->duration_seconds = 0;
        local->playing = 0;
        local->current_part = EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE;
        local->pressed_part = EGUI_VIEW_TRANSPORT_BAR_PART_NONE;
        local->dragging_seek = 0;
        return;
    }

    if (local->position_seconds > local->duration_seconds)
    {
        local->position_seconds = local->duration_seconds;
    }

    if (local->current_part != EGUI_VIEW_TRANSPORT_BAR_PART_PREVIOUS && local->current_part != EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE &&
        local->current_part != EGUI_VIEW_TRANSPORT_BAR_PART_NEXT && local->current_part != EGUI_VIEW_TRANSPORT_BAR_PART_SEEK)
    {
        local->current_part = EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE;
    }

    if (local->compact_mode || local->read_only_mode)
    {
        local->pressed_part = EGUI_VIEW_TRANSPORT_BAR_PART_NONE;
        local->dragging_seek = 0;
    }
}

static void transport_bar_notify(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);

    if (local->on_changed != NULL)
    {
        local->on_changed(self, local->position_seconds, local->duration_seconds, local->playing, local->current_part);
    }
}

static void transport_bar_set_part_inner(egui_view_t *self, uint8_t part, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    uint8_t changed = 0;

    transport_bar_normalize_state(local);
    if (local->snapshot == NULL)
    {
        part = EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE;
    }
    if (part != EGUI_VIEW_TRANSPORT_BAR_PART_PREVIOUS && part != EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE && part != EGUI_VIEW_TRANSPORT_BAR_PART_NEXT &&
        part != EGUI_VIEW_TRANSPORT_BAR_PART_SEEK)
    {
        part = EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE;
    }

    if (local->current_part != part)
    {
        local->current_part = part;
        changed = 1;
    }

    if (changed)
    {
        egui_view_invalidate(self);
    }
    if (notify)
    {
        transport_bar_notify(self);
    }
}

static void transport_bar_set_playing_inner(egui_view_t *self, uint8_t playing, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    uint8_t new_playing = playing ? 1 : 0;

    transport_bar_normalize_state(local);
    if (local->snapshot == NULL)
    {
        new_playing = 0;
    }

    if (local->playing != new_playing)
    {
        local->playing = new_playing;
        egui_view_invalidate(self);
        if (notify)
        {
            transport_bar_notify(self);
        }
    }
}

static void transport_bar_set_position_inner(egui_view_t *self, int position_seconds, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    uint16_t clamped = 0;

    transport_bar_normalize_state(local);
    if (local->snapshot != NULL)
    {
        if (position_seconds < 0)
        {
            clamped = 0;
        }
        else if ((uint16_t)position_seconds > local->duration_seconds)
        {
            clamped = local->duration_seconds;
        }
        else
        {
            clamped = (uint16_t)position_seconds;
        }
    }

    if (local->position_seconds != clamped)
    {
        local->position_seconds = clamped;
        egui_view_invalidate(self);
        if (notify)
        {
            transport_bar_notify(self);
        }
    }
}

static void transport_bar_adjust_position(egui_view_t *self, int delta_seconds, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    transport_bar_set_position_inner(self, (int)local->position_seconds + delta_seconds, notify);
}

static void transport_bar_apply_snapshot_state(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);

    if (local->snapshot == NULL)
    {
        local->position_seconds = 0;
        local->duration_seconds = 0;
        local->playing = 0;
    }
    else
    {
        local->position_seconds = local->snapshot->position_seconds;
        local->duration_seconds = local->snapshot->duration_seconds;
        local->playing = local->snapshot->playing ? 1 : 0;
    }
    local->current_part = EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE;
    transport_bar_normalize_state(local);
}

static void transport_bar_move_focus(egui_view_t *self, int direction, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    static const uint8_t order[] = {EGUI_VIEW_TRANSPORT_BAR_PART_PREVIOUS, EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE, EGUI_VIEW_TRANSPORT_BAR_PART_NEXT,
                                    EGUI_VIEW_TRANSPORT_BAR_PART_SEEK};
    uint8_t i;
    uint8_t index = 1;

    transport_bar_normalize_state(local);
    for (i = 0; i < (uint8_t)(sizeof(order) / sizeof(order[0])); i++)
    {
        if (order[i] == local->current_part)
        {
            index = i;
            break;
        }
    }

    if (direction < 0)
    {
        index = (uint8_t)((index == 0) ? (sizeof(order) / sizeof(order[0]) - 1) : (index - 1));
    }
    else
    {
        index = (uint8_t)((index + 1) % (sizeof(order) / sizeof(order[0])));
    }

    transport_bar_set_part_inner(self, order[index], notify);
}

static void transport_bar_activate_part(egui_view_t *self, uint8_t part, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);

    transport_bar_normalize_state(local);
    if (local->snapshot == NULL)
    {
        return;
    }

    transport_bar_set_part_inner(self, part, 0);
    switch (part)
    {
    case EGUI_VIEW_TRANSPORT_BAR_PART_PREVIOUS:
        transport_bar_adjust_position(self, -TB_SKIP_STEP_SECONDS, notify);
        break;
    case EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE:
        transport_bar_set_playing_inner(self, local->playing ? 0 : 1, notify);
        break;
    case EGUI_VIEW_TRANSPORT_BAR_PART_NEXT:
        transport_bar_adjust_position(self, TB_SKIP_STEP_SECONDS, notify);
        break;
    case EGUI_VIEW_TRANSPORT_BAR_PART_SEEK:
        if (notify)
        {
            transport_bar_notify(self);
        }
        break;
    default:
        break;
    }
}

static void transport_bar_get_metrics(egui_view_transport_bar_t *local, egui_view_t *self, egui_view_transport_bar_metrics_t *metrics)
{
    egui_region_t work_region;
    egui_dim_t pad_x = local->compact_mode ? TB_COMPACT_PAD_X : TB_STD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? TB_COMPACT_PAD_Y : TB_STD_PAD_Y;
    egui_dim_t card_pad_x = local->compact_mode ? TB_COMPACT_CARD_PAD_X : TB_STD_CARD_PAD_X;
    egui_dim_t card_pad_y = local->compact_mode ? TB_COMPACT_CARD_PAD_Y : TB_STD_CARD_PAD_Y;
    egui_dim_t prev_size = local->compact_mode ? TB_COMPACT_PREV_SIZE : TB_STD_PREV_SIZE;
    egui_dim_t play_size = local->compact_mode ? TB_COMPACT_PLAY_SIZE : TB_STD_PLAY_SIZE;
    egui_dim_t next_size = local->compact_mode ? TB_COMPACT_NEXT_SIZE : TB_STD_NEXT_SIZE;
    egui_dim_t btn_gap = local->compact_mode ? TB_COMPACT_BTN_GAP : TB_STD_BTN_GAP;
    egui_dim_t seek_gap = local->compact_mode ? TB_COMPACT_SEEK_GAP : TB_STD_SEEK_GAP;
    egui_dim_t button_yoff = local->compact_mode ? TB_COMPACT_BUTTON_YOFF : TB_STD_BUTTON_YOFF;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_w;
    egui_dim_t content_h;
    egui_dim_t button_y;
    egui_dim_t center_x;

    transport_bar_normalize_state(local);
    egui_view_get_work_region(self, &work_region);

    content_x = work_region.location.x + pad_x;
    content_y = work_region.location.y + pad_y;
    content_w = work_region.size.width - pad_x * 2;
    content_h = work_region.size.height - pad_y * 2;

    metrics->show_title = (!local->compact_mode && transport_bar_has_text(local->title)) ? 1 : 0;
    metrics->show_helper = (!local->compact_mode && transport_bar_has_text(local->helper)) ? 1 : 0;

    metrics->title_region.location.x = content_x;
    metrics->title_region.location.y = content_y;
    metrics->title_region.size.width = content_w;
    metrics->title_region.size.height = TB_STD_TITLE_H;

    if (metrics->show_title)
    {
        content_y += TB_STD_TITLE_H + TB_STD_TITLE_GAP;
        content_h -= TB_STD_TITLE_H + TB_STD_TITLE_GAP;
    }
    if (metrics->show_helper)
    {
        content_h -= TB_STD_HELPER_H + TB_STD_HELPER_GAP;
    }

    metrics->helper_region.location.x = content_x;
    metrics->helper_region.location.y = content_y + content_h + TB_STD_HELPER_GAP;
    metrics->helper_region.size.width = content_w;
    metrics->helper_region.size.height = TB_STD_HELPER_H;

    metrics->card_region.location.x = content_x;
    metrics->card_region.location.y = content_y;
    metrics->card_region.size.width = content_w;
    metrics->card_region.size.height = content_h;
    if (metrics->card_region.size.height < (local->compact_mode ? 42 : 66))
    {
        metrics->card_region.size.height = local->compact_mode ? 42 : 66;
    }

    center_x = metrics->card_region.location.x + metrics->card_region.size.width / 2;
    button_y = metrics->card_region.location.y + metrics->card_region.size.height - button_yoff - play_size;

    metrics->play_pause_region.location.x = center_x - play_size / 2;
    metrics->play_pause_region.location.y = button_y;
    metrics->play_pause_region.size.width = play_size;
    metrics->play_pause_region.size.height = play_size;

    metrics->previous_region.location.x = metrics->play_pause_region.location.x - btn_gap - prev_size;
    metrics->previous_region.location.y = button_y + (play_size - prev_size) / 2;
    metrics->previous_region.size.width = prev_size;
    metrics->previous_region.size.height = prev_size;

    metrics->next_region.location.x = metrics->play_pause_region.location.x + play_size + btn_gap;
    metrics->next_region.location.y = button_y + (play_size - next_size) / 2;
    metrics->next_region.size.width = next_size;
    metrics->next_region.size.height = next_size;

    metrics->seek_region.location.x = metrics->card_region.location.x + card_pad_x;
    metrics->seek_region.location.y = metrics->play_pause_region.location.y - seek_gap - (local->compact_mode ? 10 : 12);
    metrics->seek_region.size.width = metrics->card_region.size.width - card_pad_x * 2;
    metrics->seek_region.size.height = local->compact_mode ? 10 : 12;

    metrics->headline_region.location.x = metrics->card_region.location.x + card_pad_x + 8;
    metrics->headline_region.location.y = metrics->card_region.location.y + card_pad_y + (local->compact_mode ? 18 : 22);
    metrics->headline_region.size.width = metrics->card_region.size.width - (card_pad_x + 8) * 2;
    metrics->headline_region.size.height = local->compact_mode ? 10 : 14;

    metrics->body_region.location.x = metrics->headline_region.location.x;
    metrics->body_region.location.y = metrics->headline_region.location.y + (local->compact_mode ? 0 : 14);
    metrics->body_region.size.width = metrics->headline_region.size.width;
    metrics->body_region.size.height = local->compact_mode ? 0 : 10;

    metrics->footer_region.location.x = metrics->headline_region.location.x;
    metrics->footer_region.location.y = metrics->seek_region.location.y - 10;
    metrics->footer_region.size.width = metrics->headline_region.size.width;
    metrics->footer_region.size.height = 0;
}

static void transport_bar_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                    egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (!transport_bar_has_text(text))
    {
        return;
    }
    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void transport_bar_draw_focus(egui_view_t *self, const egui_region_t *region, egui_dim_t radius, egui_color_t color)
{
    transport_bar_draw_round_stroke_safe(region->location.x - 1, region->location.y - 1, region->size.width + 2, region->size.height + 2, radius, 1, color,
                                         egui_color_alpha_mix(self->alpha, 76));
}

static void transport_bar_draw_pill(egui_view_t *self, const egui_font_t *font, const char *text, egui_dim_t x, egui_dim_t y, egui_dim_t width,
                                    egui_color_t fill_color, egui_color_t border_color, egui_color_t text_color)
{
    egui_region_t region;

    region.location.x = x;
    region.location.y = y;
    region.size.width = width;
    region.size.height = 12;
    transport_bar_draw_round_fill_safe(region.location.x, region.location.y, region.size.width, region.size.height, 6, fill_color,
                                       egui_color_alpha_mix(self->alpha, 88));
    transport_bar_draw_round_stroke_safe(region.location.x, region.location.y, region.size.width, region.size.height, 6, 1, border_color,
                                         egui_color_alpha_mix(self->alpha, 54));
    transport_bar_draw_text(font, self, text, &region, EGUI_ALIGN_CENTER, text_color);
}

static void transport_bar_draw_seek(egui_view_t *self, egui_view_transport_bar_t *local, const egui_region_t *region, egui_color_t accent_color,
                                    egui_color_t rail_color, egui_color_t thumb_color)
{
    egui_dim_t rail_y = region->location.y + region->size.height / 2 - 2;
    egui_dim_t rail_w = region->size.width;
    egui_dim_t fill_w = 0;
    egui_dim_t thumb_x;
    egui_dim_t thumb_y = rail_y + 2;

    transport_bar_draw_round_fill_safe(region->location.x, rail_y, rail_w, 4, 2, rail_color, egui_color_alpha_mix(self->alpha, 86));
    if (local->duration_seconds > 0)
    {
        fill_w = (egui_dim_t)(((int32_t)rail_w * local->position_seconds) / local->duration_seconds);
    }
    if (fill_w < 0)
    {
        fill_w = 0;
    }
    if (fill_w > rail_w)
    {
        fill_w = rail_w;
    }
    if (fill_w > 0)
    {
        transport_bar_draw_round_fill_safe(region->location.x, rail_y, fill_w, 4, 2, accent_color, egui_color_alpha_mix(self->alpha, 96));
    }

    thumb_x = region->location.x + fill_w;
    if (thumb_x < region->location.x + 2)
    {
        thumb_x = region->location.x + 2;
    }
    if (thumb_x > region->location.x + rail_w - 2)
    {
        thumb_x = region->location.x + rail_w - 2;
    }
    egui_canvas_draw_circle_fill(thumb_x, thumb_y, local->compact_mode ? 3 : 4, thumb_color, egui_color_alpha_mix(self->alpha, 100));
    egui_canvas_draw_circle(thumb_x, thumb_y, local->compact_mode ? 3 : 4, 1, egui_rgb_mix(thumb_color, EGUI_COLOR_HEX(0xFFFFFF), 24),
                            egui_color_alpha_mix(self->alpha, 68));
}

static void transport_bar_draw_button(egui_view_t *self, egui_view_transport_bar_t *local, const egui_region_t *region, const char *label,
                                      egui_color_t fill_color, egui_color_t border_color, egui_color_t text_color, uint8_t emphasized, uint8_t part)
{
    egui_dim_t radius = emphasized ? (region->size.height / 2) : 7;
    egui_alpha_t fill_alpha = emphasized ? 94 : 88;

    if (local->read_only_mode)
    {
        fill_color = egui_rgb_mix(fill_color, local->inactive_color, 36);
        border_color = egui_rgb_mix(border_color, local->inactive_color, 28);
        text_color = egui_rgb_mix(text_color, local->inactive_color, 32);
    }
    else if (local->pressed_part == part)
    {
        fill_color = egui_rgb_mix(fill_color, EGUI_COLOR_HEX(0x0A1724), 10);
    }

    transport_bar_draw_round_fill_safe(region->location.x, region->location.y, region->size.width, region->size.height, radius, fill_color,
                                       egui_color_alpha_mix(self->alpha, fill_alpha));
    transport_bar_draw_round_stroke_safe(region->location.x, region->location.y, region->size.width, region->size.height, radius, 1, border_color,
                                         egui_color_alpha_mix(self->alpha, 62));
    transport_bar_draw_text(local->meta_font, self, label, region, EGUI_ALIGN_CENTER, text_color);
}

static void transport_bar_draw_card(egui_view_t *self, egui_view_transport_bar_t *local, const egui_view_transport_bar_metrics_t *metrics)
{
    egui_region_t badge_region;
    egui_region_t time_region;
    egui_color_t shell_color;
    egui_color_t accent_color;
    egui_color_t title_color;
    egui_color_t body_color;
    egui_color_t card_border_color;
    egui_color_t button_fill;
    egui_color_t button_border;
    egui_color_t rail_color;
    char time_buffer[32];
    const char *play_icon;
    const egui_view_transport_bar_snapshot_t *snapshot = local->snapshot;
    egui_dim_t badge_w;
    egui_dim_t time_w;

    if (snapshot == NULL)
    {
        return;
    }

    shell_color = egui_rgb_mix(snapshot->surface_color, local->surface_color, local->read_only_mode ? 42 : 12);
    accent_color = local->read_only_mode ? egui_rgb_mix(snapshot->accent_color, local->inactive_color, 46) : snapshot->accent_color;
    title_color = local->read_only_mode ? egui_rgb_mix(local->text_color, local->inactive_color, 30) : local->text_color;
    body_color = local->read_only_mode ? egui_rgb_mix(local->muted_text_color, local->inactive_color, 28)
                                       : egui_rgb_mix(local->text_color, local->muted_text_color, 26);
    card_border_color = egui_rgb_mix(accent_color, local->border_color, 24);
    button_fill = egui_rgb_mix(local->surface_color, accent_color, local->read_only_mode ? 18 : 8);
    button_border = egui_rgb_mix(local->border_color, accent_color, 18);
    rail_color = egui_rgb_mix(local->border_color, local->surface_color, 16);

    transport_bar_draw_round_fill_safe(metrics->card_region.location.x + TB_STD_SHADOW_X, metrics->card_region.location.y + TB_STD_SHADOW_Y,
                                       metrics->card_region.size.width, metrics->card_region.size.height, local->compact_mode ? 10 : 12,
                                       egui_rgb_mix(local->border_color, EGUI_COLOR_HEX(0x09121A), 58), egui_color_alpha_mix(self->alpha, 22));
    transport_bar_draw_round_fill_safe(metrics->card_region.location.x, metrics->card_region.location.y, metrics->card_region.size.width,
                                       metrics->card_region.size.height, local->compact_mode ? 10 : 12, shell_color, egui_color_alpha_mix(self->alpha, 100));
    transport_bar_draw_round_stroke_safe(metrics->card_region.location.x, metrics->card_region.location.y, metrics->card_region.size.width,
                                         metrics->card_region.size.height, local->compact_mode ? 10 : 12, 1, card_border_color,
                                         egui_color_alpha_mix(self->alpha, 62));

    transport_bar_draw_round_fill_safe(metrics->card_region.location.x + 10, metrics->card_region.location.y + 10, 10, metrics->card_region.size.height - 20, 5,
                                       accent_color, egui_color_alpha_mix(self->alpha, 82));

    badge_w = (egui_dim_t)(22 + strlen(snapshot->eyebrow ? snapshot->eyebrow : "") * 4);
    if (badge_w < 40)
    {
        badge_w = 40;
    }
    if (badge_w > metrics->card_region.size.width - 80)
    {
        badge_w = metrics->card_region.size.width - 80;
    }
    badge_region.location.x = metrics->card_region.location.x + 26;
    badge_region.location.y = metrics->card_region.location.y + (local->compact_mode ? 7 : 10);
    badge_region.size.width = badge_w;
    badge_region.size.height = 12;
    transport_bar_draw_round_fill_safe(badge_region.location.x, badge_region.location.y, badge_region.size.width, badge_region.size.height, 6, accent_color,
                                       egui_color_alpha_mix(self->alpha, local->read_only_mode ? 56 : 86));
    transport_bar_draw_text(local->meta_font, self, snapshot->eyebrow, &badge_region, EGUI_ALIGN_CENTER, EGUI_COLOR_HEX(0xFFFFFF));

    if (local->compact_mode)
    {
        transport_bar_format_single_time(local->position_seconds, time_buffer, sizeof(time_buffer));
    }
    else
    {
        transport_bar_format_time_pair(local->position_seconds, local->duration_seconds, time_buffer, sizeof(time_buffer));
    }
    time_w = (egui_dim_t)(28 + strlen(time_buffer) * 4);
    if (time_w < 56)
    {
        time_w = 56;
    }
    if (time_w > metrics->card_region.size.width - 80)
    {
        time_w = metrics->card_region.size.width - 80;
    }
    time_region.location.x = metrics->card_region.location.x + metrics->card_region.size.width - time_w - 12;
    time_region.location.y = badge_region.location.y;
    time_region.size.width = time_w;
    time_region.size.height = 12;
    transport_bar_draw_pill(self, local->meta_font, time_buffer, time_region.location.x, time_region.location.y, time_region.size.width,
                            egui_rgb_mix(accent_color, local->surface_color, local->read_only_mode ? 44 : 22),
                            egui_rgb_mix(accent_color, local->border_color, 18), title_color);

    transport_bar_draw_text(local->compact_mode ? local->meta_font : local->font, self, snapshot->title, &metrics->headline_region, EGUI_ALIGN_LEFT,
                            title_color);

    if (!local->compact_mode)
    {
        transport_bar_draw_text(local->meta_font, self, snapshot->subtitle, &metrics->body_region, EGUI_ALIGN_LEFT, body_color);
    }

    transport_bar_draw_seek(self, local, &metrics->seek_region, accent_color, rail_color,
                            local->read_only_mode ? egui_rgb_mix(accent_color, local->inactive_color, 20) : EGUI_COLOR_HEX(0xFFFFFF));

    play_icon = local->playing ? "||" : ">";
    transport_bar_draw_button(self, local, &metrics->previous_region, "<<", button_fill, button_border, title_color, 0, EGUI_VIEW_TRANSPORT_BAR_PART_PREVIOUS);
    transport_bar_draw_button(self, local, &metrics->play_pause_region, play_icon, accent_color, egui_rgb_mix(accent_color, local->border_color, 20),
                              EGUI_COLOR_HEX(0xFFFFFF), 1, EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE);
    transport_bar_draw_button(self, local, &metrics->next_region, ">>", button_fill, button_border, title_color, 0, EGUI_VIEW_TRANSPORT_BAR_PART_NEXT);

    if (!local->read_only_mode)
    {
        if (local->current_part == EGUI_VIEW_TRANSPORT_BAR_PART_PREVIOUS)
        {
            transport_bar_draw_focus(self, &metrics->previous_region, 7, accent_color);
        }
        else if (local->current_part == EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE)
        {
            transport_bar_draw_focus(self, &metrics->play_pause_region, metrics->play_pause_region.size.height / 2, accent_color);
        }
        else if (local->current_part == EGUI_VIEW_TRANSPORT_BAR_PART_NEXT)
        {
            transport_bar_draw_focus(self, &metrics->next_region, 7, accent_color);
        }
        else if (local->current_part == EGUI_VIEW_TRANSPORT_BAR_PART_SEEK)
        {
            transport_bar_draw_focus(self, &metrics->seek_region, 6, accent_color);
        }
    }
}

static uint8_t transport_bar_seek_from_coord(egui_view_transport_bar_t *local, egui_view_t *self, egui_dim_t x, uint8_t notify)
{
    egui_view_transport_bar_metrics_t metrics;
    int32_t numerator;
    uint16_t position = 0;

    if (local->snapshot == NULL || local->duration_seconds == 0)
    {
        return 0;
    }

    transport_bar_get_metrics(local, self, &metrics);
    if (metrics.seek_region.size.width <= 0)
    {
        return 0;
    }

    numerator = (int32_t)x - metrics.seek_region.location.x;
    if (numerator < 0)
    {
        numerator = 0;
    }
    if (numerator > metrics.seek_region.size.width)
    {
        numerator = metrics.seek_region.size.width;
    }

    position = (uint16_t)((numerator * local->duration_seconds) / metrics.seek_region.size.width);
    transport_bar_set_part_inner(self, EGUI_VIEW_TRANSPORT_BAR_PART_SEEK, 0);
    transport_bar_set_position_inner(self, position, notify);
    return 1;
}

void egui_view_transport_bar_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_transport_bar_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_transport_bar_set_title(egui_view_t *self, const char *title)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    local->title = title;
    egui_view_invalidate(self);
}

void egui_view_transport_bar_set_helper(egui_view_t *self, const char *helper)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    local->helper = helper;
    egui_view_invalidate(self);
}

void egui_view_transport_bar_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                         egui_color_t muted_text_color, egui_color_t inactive_color)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->inactive_color = inactive_color;
    egui_view_invalidate(self);
}

void egui_view_transport_bar_set_snapshot(egui_view_t *self, const egui_view_transport_bar_snapshot_t *snapshot)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    local->snapshot = snapshot;
    transport_bar_apply_snapshot_state(self);
    egui_view_invalidate(self);
}

const egui_view_transport_bar_snapshot_t *egui_view_transport_bar_get_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    return local->snapshot;
}

void egui_view_transport_bar_set_current_part(egui_view_t *self, uint8_t part)
{
    transport_bar_set_part_inner(self, part, 0);
}

uint8_t egui_view_transport_bar_get_current_part(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    transport_bar_normalize_state(local);
    return local->current_part;
}

void egui_view_transport_bar_set_position_seconds(egui_view_t *self, uint16_t position_seconds)
{
    transport_bar_set_position_inner(self, position_seconds, 0);
}

uint16_t egui_view_transport_bar_get_position_seconds(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    transport_bar_normalize_state(local);
    return local->position_seconds;
}

void egui_view_transport_bar_set_duration_seconds(egui_view_t *self, uint16_t duration_seconds)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    local->duration_seconds = duration_seconds;
    if (local->position_seconds > local->duration_seconds)
    {
        local->position_seconds = local->duration_seconds;
    }
    egui_view_invalidate(self);
}

uint16_t egui_view_transport_bar_get_duration_seconds(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    transport_bar_normalize_state(local);
    return local->duration_seconds;
}

void egui_view_transport_bar_set_playing(egui_view_t *self, uint8_t playing)
{
    transport_bar_set_playing_inner(self, playing, 0);
}

uint8_t egui_view_transport_bar_get_playing(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    transport_bar_normalize_state(local);
    return local->playing;
}

void egui_view_transport_bar_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    local->compact_mode = compact_mode ? 1 : 0;
    transport_bar_normalize_state(local);
    egui_view_invalidate(self);
}

void egui_view_transport_bar_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    local->read_only_mode = read_only_mode ? 1 : 0;
    transport_bar_normalize_state(local);
    egui_view_invalidate(self);
}

void egui_view_transport_bar_set_on_changed_listener(egui_view_t *self, egui_view_on_transport_bar_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    local->on_changed = listener;
}

uint8_t egui_view_transport_bar_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    egui_view_transport_bar_metrics_t metrics;

    if (region == NULL || local->snapshot == NULL)
    {
        return 0;
    }

    transport_bar_get_metrics(local, self, &metrics);
    if (part == EGUI_VIEW_TRANSPORT_BAR_PART_PREVIOUS)
    {
        *region = metrics.previous_region;
        return 1;
    }
    if (part == EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE)
    {
        *region = metrics.play_pause_region;
        return 1;
    }
    if (part == EGUI_VIEW_TRANSPORT_BAR_PART_NEXT)
    {
        *region = metrics.next_region;
        return 1;
    }
    if (part == EGUI_VIEW_TRANSPORT_BAR_PART_SEEK)
    {
        *region = metrics.seek_region;
        return 1;
    }
    return 0;
}

uint8_t egui_view_transport_bar_handle_navigation_key(egui_view_t *self, uint8_t key_code)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);

    transport_bar_normalize_state(local);
    if (local->compact_mode || local->read_only_mode || local->snapshot == NULL || !egui_view_get_enable(self))
    {
        return 0;
    }

    switch (key_code)
    {
    case EGUI_KEY_CODE_LEFT:
        if (local->current_part == EGUI_VIEW_TRANSPORT_BAR_PART_SEEK)
        {
            transport_bar_adjust_position(self, -TB_SEEK_STEP_SECONDS, 1);
        }
        else
        {
            transport_bar_move_focus(self, -1, 1);
        }
        return 1;
    case EGUI_KEY_CODE_RIGHT:
        if (local->current_part == EGUI_VIEW_TRANSPORT_BAR_PART_SEEK)
        {
            transport_bar_adjust_position(self, TB_SEEK_STEP_SECONDS, 1);
        }
        else
        {
            transport_bar_move_focus(self, 1, 1);
        }
        return 1;
    case EGUI_KEY_CODE_HOME:
        transport_bar_set_part_inner(self, EGUI_VIEW_TRANSPORT_BAR_PART_SEEK, 0);
        transport_bar_set_position_inner(self, 0, 1);
        return 1;
    case EGUI_KEY_CODE_END:
        transport_bar_set_part_inner(self, EGUI_VIEW_TRANSPORT_BAR_PART_SEEK, 0);
        transport_bar_set_position_inner(self, local->duration_seconds, 1);
        return 1;
    case EGUI_KEY_CODE_TAB:
        transport_bar_move_focus(self, 1, 1);
        return 1;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        transport_bar_activate_part(self, local->current_part, 1);
        return 1;
    case EGUI_KEY_CODE_PLUS:
        transport_bar_set_part_inner(self, EGUI_VIEW_TRANSPORT_BAR_PART_SEEK, 0);
        transport_bar_adjust_position(self, TB_SEEK_STEP_SECONDS, 1);
        return 1;
    case EGUI_KEY_CODE_MINUS:
        transport_bar_set_part_inner(self, EGUI_VIEW_TRANSPORT_BAR_PART_SEEK, 0);
        transport_bar_adjust_position(self, -TB_SEEK_STEP_SECONDS, 1);
        return 1;
    case EGUI_KEY_CODE_ESCAPE:
        transport_bar_set_part_inner(self, EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE, 1);
        return 1;
    default:
        return 0;
    }
}

static void egui_view_transport_bar_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    egui_view_transport_bar_metrics_t metrics;
    egui_color_t surface_color = local->surface_color;
    egui_color_t border_color = local->border_color;
    egui_color_t muted_text_color = local->muted_text_color;

    transport_bar_get_metrics(local, self, &metrics);

    if (local->read_only_mode)
    {
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xF2F4F7), 44);
        border_color = egui_rgb_mix(border_color, EGUI_COLOR_HEX(0xA7B1BD), 38);
        muted_text_color = egui_rgb_mix(muted_text_color, EGUI_COLOR_HEX(0x96A3AF), 36);
    }

    transport_bar_draw_round_fill_safe(self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width,
                                       self->region_screen.size.height, local->compact_mode ? TB_COMPACT_RADIUS : TB_STD_RADIUS, surface_color,
                                       egui_color_alpha_mix(self->alpha, 100));
    transport_bar_draw_round_stroke_safe(self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width,
                                         self->region_screen.size.height, local->compact_mode ? TB_COMPACT_RADIUS : TB_STD_RADIUS, 1, border_color,
                                         egui_color_alpha_mix(self->alpha, 56));

    if (metrics.show_title)
    {
        transport_bar_draw_text(local->meta_font, self, local->title, &metrics.title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);
    }

    if (local->snapshot == NULL)
    {
        transport_bar_draw_text(local->meta_font, self, "No track", &metrics.card_region, EGUI_ALIGN_CENTER, muted_text_color);
        return;
    }

    transport_bar_draw_card(self, local, &metrics);

    if (metrics.show_helper)
    {
        transport_bar_draw_text(local->meta_font, self, local->helper, &metrics.helper_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static uint8_t transport_bar_hit_part(egui_view_transport_bar_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_transport_bar_metrics_t metrics;

    if (local->snapshot == NULL)
    {
        return EGUI_VIEW_TRANSPORT_BAR_PART_NONE;
    }

    transport_bar_get_metrics(local, self, &metrics);
    if (egui_region_pt_in_rect(&metrics.previous_region, x, y))
    {
        return EGUI_VIEW_TRANSPORT_BAR_PART_PREVIOUS;
    }
    if (egui_region_pt_in_rect(&metrics.play_pause_region, x, y))
    {
        return EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE;
    }
    if (egui_region_pt_in_rect(&metrics.next_region, x, y))
    {
        return EGUI_VIEW_TRANSPORT_BAR_PART_NEXT;
    }
    if (egui_region_pt_in_rect(&metrics.seek_region, x, y))
    {
        return EGUI_VIEW_TRANSPORT_BAR_PART_SEEK;
    }
    return EGUI_VIEW_TRANSPORT_BAR_PART_NONE;
}

static int egui_view_transport_bar_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_transport_bar_t);
    uint8_t hit_part;

    transport_bar_normalize_state(local);
    if (local->compact_mode || local->read_only_mode || local->snapshot == NULL || !egui_view_get_enable(self))
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_part = transport_bar_hit_part(local, self, event->location.x, event->location.y);
        if (hit_part == EGUI_VIEW_TRANSPORT_BAR_PART_NONE)
        {
            return 0;
        }
        local->pressed_part = hit_part;
        local->dragging_seek = hit_part == EGUI_VIEW_TRANSPORT_BAR_PART_SEEK ? 1 : 0;
        egui_view_set_pressed(self, true);
        transport_bar_set_part_inner(self, hit_part, 0);
        if (hit_part == EGUI_VIEW_TRANSPORT_BAR_PART_SEEK)
        {
            transport_bar_seek_from_coord(local, self, event->location.x, 0);
        }
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        if (local->pressed_part != EGUI_VIEW_TRANSPORT_BAR_PART_SEEK || !local->dragging_seek)
        {
            return 0;
        }
        transport_bar_seek_from_coord(local, self, event->location.x, 0);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        if (local->pressed_part == EGUI_VIEW_TRANSPORT_BAR_PART_NONE)
        {
            return 0;
        }
        hit_part = transport_bar_hit_part(local, self, event->location.x, event->location.y);
        if (local->pressed_part == EGUI_VIEW_TRANSPORT_BAR_PART_SEEK)
        {
            transport_bar_seek_from_coord(local, self, event->location.x, 1);
        }
        else if (local->pressed_part == hit_part)
        {
            transport_bar_activate_part(self, local->pressed_part, 1);
        }
        local->pressed_part = EGUI_VIEW_TRANSPORT_BAR_PART_NONE;
        local->dragging_seek = 0;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_part = EGUI_VIEW_TRANSPORT_BAR_PART_NONE;
        local->dragging_seek = 0;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_transport_bar_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    if (event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        return 0;
    }

    if (egui_view_transport_bar_handle_navigation_key(self, event->key_code))
    {
        return 1;
    }

    return egui_view_on_key_event(self, event);
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_transport_bar_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_transport_bar_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_transport_bar_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_transport_bar_on_key_event,
#endif
};

void egui_view_transport_bar_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_transport_bar_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_transport_bar_t);
    self->is_clickable = true;
    egui_view_set_padding_all(self, 2);

    local->on_changed = NULL;
    local->snapshot = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->title = NULL;
    local->helper = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD6DEE7);
    local->text_color = EGUI_COLOR_HEX(0x172331);
    local->muted_text_color = EGUI_COLOR_HEX(0x6E7C8B);
    local->inactive_color = EGUI_COLOR_HEX(0xA6B0BD);
    local->position_seconds = 0;
    local->duration_seconds = 0;
    local->playing = 0;
    local->current_part = EGUI_VIEW_TRANSPORT_BAR_PART_PLAY_PAUSE;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    local->pressed_part = EGUI_VIEW_TRANSPORT_BAR_PART_NONE;
    local->dragging_seek = 0;

    egui_view_set_view_name(self, "egui_view_transport_bar");
}
