#include <stdlib.h>
#include <string.h>

#include "egui_view_level_meter.h"

static uint8_t egui_view_level_meter_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_LEVEL_METER_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_LEVEL_METER_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_level_meter_clamp_channel_count(uint8_t count)
{
    if (count > EGUI_VIEW_LEVEL_METER_MAX_CHANNELS)
    {
        return EGUI_VIEW_LEVEL_METER_MAX_CHANNELS;
    }
    return count;
}

static egui_color_t egui_view_level_meter_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_70);
}

static egui_color_t egui_view_level_meter_status_color(uint8_t status)
{
    switch (status)
    {
    case 1:
        return EGUI_COLOR_HEX(0xF59E0B);
    case 2:
        return EGUI_COLOR_HEX(0xFB7185);
    default:
        return EGUI_COLOR_HEX(0x34D399);
    }
}

static const char *egui_view_level_meter_get_title_text(const egui_view_level_meter_snapshot_t *snapshot, uint8_t is_enabled)
{
    if (!is_enabled)
    {
        return "Locked";
    }
    return snapshot->title != NULL ? snapshot->title : "Meter";
}

void egui_view_level_meter_set_snapshots(egui_view_t *self, const egui_view_level_meter_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_level_meter_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_level_meter_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_level_meter_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_level_meter_t);
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

uint8_t egui_view_level_meter_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_level_meter_t);
    return local->current_snapshot;
}

void egui_view_level_meter_set_focus_channel(egui_view_t *self, uint8_t channel_index)
{
    EGUI_LOCAL_INIT(egui_view_level_meter_t);
    local->focus_channel = channel_index;
    egui_view_invalidate(self);
}

void egui_view_level_meter_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_level_meter_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_level_meter_set_show_header(egui_view_t *self, uint8_t show_header)
{
    EGUI_LOCAL_INIT(egui_view_level_meter_t);
    local->show_header = show_header ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_level_meter_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_level_meter_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_level_meter_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t border_color,
        egui_color_t text_color,
        egui_color_t muted_text_color,
        egui_color_t active_color)
{
    EGUI_LOCAL_INIT(egui_view_level_meter_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->active_color = active_color;
    egui_view_invalidate(self);
}

static void egui_view_level_meter_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_level_meter_t);
    egui_region_t region;
    egui_region_t header_region;
    egui_region_t label_region;
    egui_region_t text_region;
    const egui_view_level_meter_snapshot_t *snapshot;
    egui_color_t panel_color;
    egui_color_t shadow_color;
    uint8_t is_enabled;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_width;
    egui_dim_t content_height;
    uint8_t channel_count;
    uint8_t current_channel;
    egui_dim_t bar_area_height;
    egui_dim_t bar_w;
    egui_dim_t bar_gap;
    const char *title_text;
    egui_dim_t x;
    uint8_t i;
    char compact_badge[3] = {'L', 'K', '\0'};
    char footer_text[16];

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    channel_count = egui_view_level_meter_clamp_channel_count(snapshot->channel_count);
    if (snapshot->channels == NULL || channel_count == 0)
    {
        return;
    }

    is_enabled = egui_view_get_enable(self) ? 1 : 0;
    panel_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_30);
    if (!is_enabled)
    {
        panel_color = egui_view_level_meter_mix_disabled(panel_color);
    }
    shadow_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_10);

    egui_canvas_draw_round_rectangle_fill(region.location.x + 1, region.location.y + 2, region.size.width, region.size.height, 8, shadow_color,
                                          egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, 8, panel_color,
                                          egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, 8, 1, local->border_color,
                                     egui_color_alpha_mix(self->alpha, EGUI_ALPHA_50));
    egui_canvas_draw_round_rectangle(region.location.x + 2, region.location.y + 2, region.size.width - 4, region.size.height - 4, 6, 1,
                                     egui_rgb_mix(local->active_color, local->border_color, EGUI_ALPHA_30), egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));

    title_text = egui_view_level_meter_get_title_text(snapshot, is_enabled);

    content_x = region.location.x + 6;
    content_y = region.location.y + 4;
    content_width = region.size.width - 12;
    content_height = region.size.height - 8;

    if (local->compact_mode)
    {
        size_t title_len = snapshot->title != NULL ? strlen(snapshot->title) : 0;
        if (is_enabled && title_len > 0)
        {
            compact_badge[0] = snapshot->title[title_len - 1];
            compact_badge[1] = '\0';
        }
        egui_canvas_draw_round_rectangle_fill(region.location.x + 12, region.location.y + 6, region.size.width - 24, 2, 1, local->active_color,
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        header_region.location.x = content_x + (content_width - 26) / 2;
        header_region.location.y = content_y + 6;
        header_region.size.width = 26;
        header_region.size.height = 10;
        egui_canvas_draw_round_rectangle_fill(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 4,
                                              egui_rgb_mix(local->surface_color, local->active_color, EGUI_ALPHA_10), egui_color_alpha_mix(self->alpha, EGUI_ALPHA_70));
        egui_canvas_draw_round_rectangle(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 4, 1,
                                         egui_rgb_mix(local->border_color, local->active_color, EGUI_ALPHA_20), egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        egui_canvas_draw_text_in_rect(local->font, compact_badge, &header_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color, self->alpha);
        content_y += 12;
        content_height -= 12;
    }

    if (local->show_header)
    {
        egui_dim_t pill_w = 34 + (egui_dim_t)strlen(title_text) * 6;
        if (pill_w < 74)
        {
            pill_w = 74;
        }
        if (pill_w > content_width)
        {
            pill_w = content_width;
        }
        header_region.location.x = content_x + (content_width - pill_w) / 2;
        header_region.location.y = content_y;
        header_region.size.width = pill_w;
        header_region.size.height = 14;
        egui_canvas_draw_round_rectangle_fill(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 6,
                                              egui_rgb_mix(local->surface_color, local->active_color, EGUI_ALPHA_10), egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));
        egui_canvas_draw_round_rectangle(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 6, 1,
                                         egui_rgb_mix(local->border_color, local->active_color, EGUI_ALPHA_20), egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
        egui_canvas_draw_text_in_rect(local->font, title_text, &header_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color, self->alpha);
        egui_canvas_draw_line(header_region.location.x + 6, header_region.location.y + header_region.size.height - 2,
                              header_region.location.x + header_region.size.width - 7, header_region.location.y + header_region.size.height - 2, 1, local->active_color,
                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        egui_canvas_draw_line(content_x + 5, content_y + 17, content_x + content_width - 6, content_y + 17, 1, local->border_color,
                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        content_y += 20;
        content_height -= 20;
    }
    if (content_width <= 20 || content_height <= 24)
    {
        return;
    }

    if (!local->compact_mode && content_height > 18)
    {
        content_height -= 18;
    }

    current_channel = local->focus_channel < channel_count ? local->focus_channel : snapshot->focus_channel;
    if (current_channel >= channel_count)
    {
        current_channel = 0;
    }

    bar_area_height = content_height - (local->compact_mode ? 0 : 14);
    bar_gap = local->compact_mode ? 4 : 5;
    bar_w = (content_width - (channel_count - 1) * bar_gap) / channel_count;
    if (bar_w < 12)
    {
        bar_w = 12;
    }

    x = content_x;
    for (i = 0; i < channel_count; i++)
    {
        const egui_view_level_meter_channel_t *channel = &snapshot->channels[i];
        egui_color_t status_color = egui_view_level_meter_status_color(channel->status);
        egui_color_t fill_color;
        egui_color_t border_color;
        egui_dim_t bar_x = x;
        egui_dim_t bar_y = content_y + 1;
        egui_dim_t inner_y;
        egui_dim_t inner_h;
        egui_dim_t level_h;
        egui_dim_t peak_y;
        uint8_t is_current = i == snapshot->focus_channel;
        uint8_t is_focus = i == current_channel;

        fill_color = egui_rgb_mix(local->surface_color, status_color, is_current ? EGUI_ALPHA_20 : EGUI_ALPHA_10);
        border_color = is_focus ? local->active_color : egui_rgb_mix(local->border_color, status_color, EGUI_ALPHA_30);
        if (!is_enabled)
        {
            fill_color = egui_view_level_meter_mix_disabled(fill_color);
            border_color = egui_view_level_meter_mix_disabled(border_color);
            status_color = egui_view_level_meter_mix_disabled(status_color);
        }

        if (is_focus)
        {
            egui_canvas_draw_round_rectangle_fill(bar_x - 1, bar_y - 1, bar_w + 2, bar_area_height + 2, 5, status_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
            egui_canvas_draw_line(bar_x + 2, bar_y + 2, bar_x + bar_w - 3, bar_y + 2, 1, status_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40));
            egui_canvas_draw_line(bar_x + 2, bar_y + bar_area_height - 3, bar_x + bar_w - 3, bar_y + bar_area_height - 3, 1, status_color,
                                  egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        }
        egui_canvas_draw_round_rectangle_fill(bar_x, bar_y, bar_w, bar_area_height, 4, fill_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
        egui_canvas_draw_round_rectangle(bar_x, bar_y, bar_w, bar_area_height, 4, is_current ? 2 : 1, border_color, egui_color_alpha_mix(self->alpha, is_current ? EGUI_ALPHA_80 : EGUI_ALPHA_40));

        inner_y = bar_y + 4;
        inner_h = bar_area_height - 8;
        if (inner_h < 8)
        {
            inner_h = 8;
        }
        egui_canvas_draw_round_rectangle_fill(bar_x + 3, inner_y, bar_w - 6, inner_h, 2, egui_rgb_mix(local->surface_color, local->border_color, EGUI_ALPHA_20),
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
        level_h = (inner_h * channel->level) / 100;
        peak_y = inner_y + inner_h - (inner_h * channel->peak) / 100;
        egui_canvas_draw_round_rectangle_fill(bar_x + 3, inner_y + inner_h - level_h, bar_w - 6, level_h, 3, status_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_80));
        egui_canvas_draw_line(bar_x + 3, peak_y, bar_x + bar_w - 4, peak_y, 1, is_focus ? local->active_color : local->text_color,
                              egui_color_alpha_mix(self->alpha, is_enabled ? EGUI_ALPHA_80 : EGUI_ALPHA_40));

        if (!local->compact_mode && channel->label != NULL)
        {
            egui_canvas_draw_round_rectangle_fill(bar_x + 1, content_y + bar_area_height + 2, bar_w - 2, 11, 4,
                                                  egui_rgb_mix(local->surface_color, status_color, is_focus ? EGUI_ALPHA_20 : EGUI_ALPHA_10),
                                                  egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40));
            egui_canvas_draw_round_rectangle(bar_x + 1, content_y + bar_area_height + 2, bar_w - 2, 11, 4, 1,
                                             egui_rgb_mix(border_color, status_color, EGUI_ALPHA_20), egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
            label_region.location.x = bar_x;
            label_region.location.y = content_y + bar_area_height + 3;
            label_region.size.width = bar_w;
            label_region.size.height = 9;
            egui_canvas_draw_text_in_rect(local->font, channel->label, &label_region, EGUI_ALIGN_CENTER, is_focus ? local->text_color : local->muted_text_color, self->alpha);
        }

        x += bar_w + bar_gap;
    }

    if (!local->compact_mode)
    {
        const char *caption = snapshot->channels[current_channel].label != NULL ? snapshot->channels[current_channel].label : snapshot->title;
        if (snapshot->channels[current_channel].label != NULL)
        {
            strcpy(footer_text, "Peak ");
            strncat(footer_text, caption, sizeof(footer_text) - strlen(footer_text) - 1);
            caption = footer_text;
        }
        egui_dim_t caption_w = 24 + (egui_dim_t)strlen(caption) * 6;
        if (caption_w < 72)
        {
            caption_w = 72;
        }
        if (caption_w > content_width - 24)
        {
            caption_w = content_width - 24;
        }
        text_region.location.x = content_x;
        text_region.location.y = content_y + content_height + 4;
        text_region.size.width = content_width;
        text_region.size.height = 11;
        egui_canvas_draw_round_rectangle_fill(content_x + (content_width - caption_w) / 2, content_y + content_height + 3, caption_w, 11, 5, panel_color,
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40));
        egui_canvas_draw_round_rectangle(content_x + (content_width - caption_w) / 2, content_y + content_height + 3, caption_w, 11, 5, 1,
                                         egui_rgb_mix(local->border_color, local->active_color, EGUI_ALPHA_20), egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        egui_canvas_draw_text_in_rect(local->font, caption, &text_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color, self->alpha);
    }
    else
    {
        egui_color_t footer_color = is_enabled ? local->active_color : local->muted_text_color;
        egui_canvas_draw_round_rectangle_fill(content_x + 12, region.location.y + region.size.height - 8, content_width - 24, 2, 1, footer_color,
                                              egui_color_alpha_mix(self->alpha, is_enabled ? EGUI_ALPHA_20 : EGUI_ALPHA_10));
    }

    if (!is_enabled)
    {
        egui_canvas_draw_round_rectangle_fill(content_x + 1, content_y + 1, content_width - 2, content_height - 2, 5, EGUI_COLOR_BLACK,
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_10));
        egui_canvas_draw_line(content_x + 9, content_y + 8, content_x + content_width - 10, content_y + content_height - 9, 1, local->muted_text_color,
                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_10));
        egui_canvas_draw_line(content_x + 9, content_y + content_height - 9, content_x + content_width - 10, content_y + 8, 1, local->muted_text_color,
                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_10));
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_level_meter_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_level_meter_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_level_meter_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_level_meter_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_level_meter_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x101723);
    local->border_color = EGUI_COLOR_HEX(0x4A5568);
    local->text_color = EGUI_COLOR_HEX(0xE2E8F0);
    local->muted_text_color = EGUI_COLOR_HEX(0x94A3B8);
    local->active_color = EGUI_COLOR_HEX(0x38BDF8);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->focus_channel = 2;
    local->show_header = 1;
    local->compact_mode = 0;
}
