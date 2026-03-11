#include <stdlib.h>
#include <string.h>

#include "egui_view_fader_bank.h"

static uint8_t egui_view_fader_bank_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_FADER_BANK_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_FADER_BANK_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_fader_bank_clamp_channel_count(uint8_t count)
{
    if (count > EGUI_VIEW_FADER_BANK_MAX_CHANNELS)
    {
        return EGUI_VIEW_FADER_BANK_MAX_CHANNELS;
    }
    return count;
}

static egui_color_t egui_view_fader_bank_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_70);
}

static egui_color_t egui_view_fader_bank_accent_color(uint8_t accent)
{
    switch (accent)
    {
    case 1:
        return EGUI_COLOR_HEX(0xF59E0B);
    case 2:
        return EGUI_COLOR_HEX(0xFB7185);
    default:
        return EGUI_COLOR_HEX(0x34D399);
    }
}

void egui_view_fader_bank_set_snapshots(egui_view_t *self, const egui_view_fader_bank_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_fader_bank_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_fader_bank_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_fader_bank_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_fader_bank_t);
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

uint8_t egui_view_fader_bank_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_fader_bank_t);
    return local->current_snapshot;
}

void egui_view_fader_bank_set_focus_channel(egui_view_t *self, uint8_t channel_index)
{
    EGUI_LOCAL_INIT(egui_view_fader_bank_t);
    local->focus_channel = channel_index;
    egui_view_invalidate(self);
}

void egui_view_fader_bank_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_fader_bank_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_fader_bank_set_show_header(egui_view_t *self, uint8_t show_header)
{
    EGUI_LOCAL_INIT(egui_view_fader_bank_t);
    local->show_header = show_header ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_fader_bank_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_fader_bank_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_fader_bank_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                      egui_color_t muted_text_color, egui_color_t active_color)
{
    EGUI_LOCAL_INIT(egui_view_fader_bank_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->active_color = active_color;
    egui_view_invalidate(self);
}

static void egui_view_fader_bank_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_fader_bank_t);
    egui_region_t region;
    egui_region_t header_region;
    egui_region_t label_region;
    egui_region_t text_region;
    const egui_view_fader_bank_snapshot_t *snapshot;
    egui_color_t panel_color;
    egui_color_t shadow_color;
    uint8_t is_enabled;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_width;
    egui_dim_t content_height;
    uint8_t channel_count;
    uint8_t current_channel;
    egui_dim_t strip_w;
    egui_dim_t strip_gap;
    egui_dim_t strip_h;
    egui_dim_t x;
    uint8_t i;
    char compact_badge[3] = {'L', 'K', '\0'};

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    channel_count = egui_view_fader_bank_clamp_channel_count(snapshot->channel_count);
    if (snapshot->channels == NULL || channel_count == 0)
    {
        return;
    }

    is_enabled = egui_view_get_enable(self) ? 1 : 0;
    panel_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_30);
    if (!is_enabled)
    {
        panel_color = egui_view_fader_bank_mix_disabled(panel_color);
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
    if (local->compact_mode)
    {
        egui_canvas_draw_round_rectangle_fill(region.location.x + 10, region.location.y + 6, region.size.width - 20, 2, 1, local->active_color,
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        egui_canvas_draw_round_rectangle_fill(region.location.x + (region.size.width - 24) / 2, region.location.y + 5, 24, 4, 2,
                                              is_enabled ? local->active_color : local->muted_text_color,
                                              egui_color_alpha_mix(self->alpha, is_enabled ? EGUI_ALPHA_40 : EGUI_ALPHA_20));
    }

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
        header_region.location.x = content_x + (content_width - 24) / 2;
        header_region.location.y = content_y + 4;
        header_region.size.width = 24;
        header_region.size.height = 8;
        egui_canvas_draw_round_rectangle_fill(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 4,
                                              egui_rgb_mix(local->surface_color, local->active_color, EGUI_ALPHA_10),
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_50));
        egui_canvas_draw_round_rectangle(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 4, 1,
                                         egui_rgb_mix(local->border_color, local->active_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        egui_canvas_draw_text_in_rect(local->font, compact_badge, &header_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color,
                                      self->alpha);
        content_y += 12;
        content_height -= 12;
    }

    if (local->show_header)
    {
        egui_dim_t pill_w = 32 + (egui_dim_t)strlen(is_enabled ? snapshot->title : "Locked") * 6;
        if (pill_w < 62)
        {
            pill_w = 62;
        }
        if (pill_w > content_width)
        {
            pill_w = content_width;
        }
        header_region.location.x = content_x + (content_width - pill_w) / 2;
        header_region.location.y = content_y;
        header_region.size.width = pill_w;
        header_region.size.height = 13;
        egui_canvas_draw_round_rectangle_fill(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 6,
                                              egui_rgb_mix(local->surface_color, local->active_color, EGUI_ALPHA_10),
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));
        egui_canvas_draw_round_rectangle(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 6, 1,
                                         egui_rgb_mix(local->border_color, local->active_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
        egui_canvas_draw_text_in_rect(local->font, is_enabled ? snapshot->title : "Locked", &header_region, EGUI_ALIGN_CENTER,
                                      is_enabled ? local->text_color : local->muted_text_color, self->alpha);
        egui_canvas_draw_line(content_x + 5, content_y + 16, content_x + content_width - 6, content_y + 16, 1, local->border_color,
                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        content_y += 19;
        content_height -= 19;
    }
    if (content_width <= 20 || content_height <= 24)
    {
        return;
    }

    if (!local->compact_mode && content_height > 16)
    {
        content_height -= 16;
    }

    current_channel = local->focus_channel < channel_count ? local->focus_channel : snapshot->focus_channel;
    if (current_channel >= channel_count)
    {
        current_channel = 0;
    }

    strip_gap = local->compact_mode ? 4 : 5;
    strip_w = (content_width - (channel_count - 1) * strip_gap) / channel_count;
    if (strip_w < 12)
    {
        strip_w = 12;
    }
    strip_h = content_height - (local->compact_mode ? 0 : 14);
    x = content_x;

    for (i = 0; i < channel_count; i++)
    {
        const egui_view_fader_bank_channel_t *channel = &snapshot->channels[i];
        egui_color_t accent_color = egui_view_fader_bank_accent_color(channel->accent);
        egui_color_t fill_color;
        egui_color_t border_color;
        egui_dim_t strip_x = x;
        egui_dim_t strip_y = content_y + 1;
        egui_dim_t track_x = strip_x + (strip_w / 2) - 2;
        egui_dim_t track_y = strip_y + 4;
        egui_dim_t track_h = strip_h - (local->compact_mode ? 10 : 16);
        egui_dim_t fill_y;
        egui_dim_t fill_h;
        egui_dim_t handle_y;
        egui_dim_t button_y;
        uint8_t is_current = i == snapshot->focus_channel;
        uint8_t is_focus = i == current_channel;

        fill_color = egui_rgb_mix(local->surface_color, accent_color, is_current ? EGUI_ALPHA_20 : EGUI_ALPHA_10);
        border_color = is_focus ? local->active_color : egui_rgb_mix(local->border_color, accent_color, EGUI_ALPHA_30);
        if (!is_enabled)
        {
            fill_color = egui_view_fader_bank_mix_disabled(fill_color);
            border_color = egui_view_fader_bank_mix_disabled(border_color);
            accent_color = egui_view_fader_bank_mix_disabled(accent_color);
        }

        if (is_focus)
        {
            egui_canvas_draw_round_rectangle_fill(strip_x - 1, strip_y - 1, strip_w + 2, strip_h + 2, 5, accent_color,
                                                  egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
            egui_canvas_draw_line(strip_x + 2, strip_y + 2, strip_x + strip_w - 3, strip_y + 2, 1, accent_color,
                                  egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40));
            egui_canvas_draw_line(strip_x + 2, strip_y + strip_h - 3, strip_x + strip_w - 3, strip_y + strip_h - 3, 1, accent_color,
                                  egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
        }
        egui_canvas_draw_round_rectangle_fill(strip_x, strip_y, strip_w, strip_h, 4, fill_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
        egui_canvas_draw_round_rectangle(strip_x, strip_y, strip_w, strip_h, 4, is_current ? 2 : 1, border_color,
                                         egui_color_alpha_mix(self->alpha, is_current ? EGUI_ALPHA_80 : EGUI_ALPHA_40));
        egui_canvas_draw_round_rectangle_fill(track_x, track_y, 4, track_h, 2, egui_rgb_mix(local->surface_color, local->border_color, EGUI_ALPHA_20),
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40));
        fill_h = (track_h * channel->level) / 100;
        fill_y = track_y + track_h - fill_h;
        if (fill_h > 0)
        {
            egui_canvas_draw_round_rectangle_fill(track_x, fill_y, 4, fill_h, 2, accent_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40));
        }
        handle_y = track_y + track_h - (track_h * channel->level) / 100 - 4;
        if (handle_y < track_y)
        {
            handle_y = track_y;
        }
        egui_canvas_draw_round_rectangle_fill(track_x - 5, handle_y, 14, 8, 3, accent_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_80));
        egui_canvas_draw_round_rectangle(track_x - 5, handle_y, 14, 8, 3, 1, border_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));

        button_y = strip_y + strip_h - (local->compact_mode ? 14 : 18);
        egui_canvas_draw_round_rectangle_fill(strip_x + 2, button_y, (strip_w - 6) / 2, 8, 3,
                                              channel->mute_on ? EGUI_COLOR_HEX(0xFB7185)
                                                               : egui_rgb_mix(local->surface_color, local->border_color, EGUI_ALPHA_10),
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_70));
        egui_canvas_draw_round_rectangle_fill(strip_x + 4 + (strip_w - 6) / 2, button_y, (strip_w - 6) / 2, 8, 3,
                                              channel->solo_on ? EGUI_COLOR_HEX(0xF59E0B)
                                                               : egui_rgb_mix(local->surface_color, local->border_color, EGUI_ALPHA_10),
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_70));
        egui_canvas_draw_line(strip_x + strip_w / 2, button_y + 1, strip_x + strip_w / 2, button_y + 6, 1, local->border_color,
                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));

        if (!local->compact_mode && channel->label != NULL)
        {
            egui_canvas_draw_round_rectangle_fill(strip_x + 1, content_y + strip_h + 2, strip_w - 2, 10, 4,
                                                  egui_rgb_mix(local->surface_color, accent_color, is_focus ? EGUI_ALPHA_20 : EGUI_ALPHA_10),
                                                  egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40));
            egui_canvas_draw_round_rectangle(strip_x + 1, content_y + strip_h + 2, strip_w - 2, 10, 4, 1,
                                             egui_rgb_mix(border_color, accent_color, EGUI_ALPHA_20), egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
            label_region.location.x = strip_x;
            label_region.location.y = content_y + strip_h + 3;
            label_region.size.width = strip_w;
            label_region.size.height = 8;
            egui_canvas_draw_text_in_rect(local->font, channel->label, &label_region, EGUI_ALIGN_CENTER, is_focus ? local->text_color : local->muted_text_color,
                                          self->alpha);
        }

        x += strip_w + strip_gap;
    }

    if (!local->compact_mode)
    {
        const char *caption = snapshot->channels[current_channel].label != NULL ? snapshot->channels[current_channel].label : snapshot->title;
        egui_dim_t caption_w = 24 + (egui_dim_t)strlen(caption) * 6;
        if (caption_w < 46)
        {
            caption_w = 46;
        }
        if (caption_w > content_width - 24)
        {
            caption_w = content_width - 24;
        }
        text_region.location.x = content_x;
        text_region.location.y = content_y + content_height + 3;
        text_region.size.width = content_width;
        text_region.size.height = 10;
        egui_canvas_draw_round_rectangle_fill(content_x + (content_width - caption_w) / 2, content_y + content_height + 2, caption_w, 10, 5, panel_color,
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
        egui_canvas_draw_round_rectangle(content_x + (content_width - caption_w) / 2, content_y + content_height + 2, caption_w, 10, 5, 1,
                                         egui_rgb_mix(local->border_color, local->active_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        egui_canvas_draw_text_in_rect(local->font, caption, &text_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color,
                                      self->alpha);
    }

    if (!is_enabled)
    {
        egui_canvas_draw_round_rectangle_fill(content_x + 1, content_y + 1, content_width - 2, content_height - 2, 5, EGUI_COLOR_BLACK,
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_10));
        egui_canvas_draw_line(content_x + 4, content_y + 4, content_x + content_width - 5, content_y + content_height - 5, 1, local->muted_text_color,
                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        egui_canvas_draw_line(content_x + 4, content_y + content_height - 5, content_x + content_width - 5, content_y + 4, 1, local->muted_text_color,
                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_fader_bank_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_fader_bank_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_fader_bank_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_fader_bank_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_fader_bank_t);
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
