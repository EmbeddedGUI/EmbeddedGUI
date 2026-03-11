#include <stdlib.h>

#include "egui_view_signal_beacon.h"

static uint8_t egui_view_signal_beacon_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_SIGNAL_BEACON_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_SIGNAL_BEACON_MAX_SNAPSHOTS;
    }
    return count;
}

void egui_view_signal_beacon_set_snapshots(egui_view_t *self, const egui_view_signal_beacon_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_signal_beacon_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_signal_beacon_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_signal_beacon_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_signal_beacon_t);
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

uint8_t egui_view_signal_beacon_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_signal_beacon_t);
    return local->current_snapshot;
}

void egui_view_signal_beacon_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_signal_beacon_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_signal_beacon_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_signal_beacon_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_signal_beacon_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_signal_beacon_t);
    local->locked_mode = locked_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_signal_beacon_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                         egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t warn_color, egui_color_t critical_color,
                                         egui_color_t node_fill_color)
{
    EGUI_LOCAL_INIT(egui_view_signal_beacon_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->warn_color = warn_color;
    local->critical_color = critical_color;
    local->node_fill_color = node_fill_color;
    egui_view_invalidate(self);
}

static egui_color_t egui_view_signal_beacon_get_signal_color(egui_view_signal_beacon_t *local, const egui_view_signal_beacon_snapshot_t *snapshot)
{
    if (snapshot->is_alert >= 2)
    {
        return local->critical_color;
    }
    if (snapshot->is_alert == 1)
    {
        return local->warn_color;
    }
    return local->accent_color;
}

static void egui_view_signal_beacon_draw_node(egui_view_signal_beacon_t *local, egui_view_t *self, egui_dim_t cx, egui_dim_t cy, egui_dim_t radius,
                                              egui_color_t signal_color, uint8_t is_focus)
{
    egui_color_t fill_color;
    fill_color = is_focus ? egui_rgb_mix(local->node_fill_color, signal_color, EGUI_ALPHA_30) : local->node_fill_color;
    egui_canvas_draw_circle_fill(cx, cy, radius, fill_color, egui_color_alpha_mix(self->alpha, is_focus ? EGUI_ALPHA_90 : EGUI_ALPHA_70));
    egui_canvas_draw_circle(cx, cy, radius, is_focus ? 2 : 1, is_focus ? signal_color : local->border_color,
                            egui_color_alpha_mix(self->alpha, is_focus ? EGUI_ALPHA_90 : EGUI_ALPHA_50));
    egui_canvas_draw_circle_fill(cx, cy, EGUI_MAX(radius / 3, 2), signal_color, egui_color_alpha_mix(self->alpha, is_focus ? EGUI_ALPHA_90 : EGUI_ALPHA_50));
}

static void egui_view_signal_beacon_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_signal_beacon_t);
    egui_region_t region;
    egui_region_t text_region;
    const egui_view_signal_beacon_snapshot_t *snapshot;
    egui_color_t signal_color;
    egui_color_t halo_color;
    egui_dim_t panel_x;
    egui_dim_t panel_y;
    egui_dim_t panel_w;
    egui_dim_t panel_h;
    egui_dim_t side_padding;
    egui_dim_t pill_w;
    egui_dim_t pill_x;
    egui_dim_t pill_right_padding;
    egui_dim_t title_w;
    egui_dim_t header_top_padding;
    egui_dim_t center_x;
    egui_dim_t center_y;
    egui_dim_t outer_radius;
    egui_dim_t mid_radius;
    egui_dim_t core_radius;
    egui_dim_t node_offset;
    egui_dim_t node_radius;
    egui_dim_t pulse_y;
    egui_dim_t pulse_w;
    egui_dim_t pulse_gap;
    egui_dim_t footer_y;
    egui_dim_t footer_side_padding;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    signal_color = egui_view_signal_beacon_get_signal_color(local, snapshot);
    halo_color = egui_rgb_mix(signal_color, EGUI_COLOR_WHITE, EGUI_ALPHA_10);

    panel_x = region.location.x;
    panel_y = region.location.y;
    panel_w = region.size.width;
    panel_h = region.size.height;
    side_padding = local->compact_mode ? 8 : 10;
    pill_w = local->compact_mode ? 30 : 42;
    pill_right_padding = local->compact_mode ? 9 : 11;
    header_top_padding = 6;

    egui_canvas_draw_round_rectangle_fill(panel_x, panel_y, panel_w, panel_h, 10, egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_30),
                                          egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40));
    egui_canvas_draw_round_rectangle(panel_x, panel_y, panel_w, panel_h, 10, 1, local->border_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));

    pill_x = panel_x + panel_w - pill_w - pill_right_padding;
    title_w = pill_x - panel_x - side_padding - 6;

    text_region.location.x = panel_x + side_padding;
    text_region.location.y = panel_y + header_top_padding;
    text_region.size.width = title_w;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(local->font, snapshot->title ? snapshot->title : "BEACON", &text_region, EGUI_ALIGN_LEFT, local->muted_text_color,
                                  self->alpha);

    egui_canvas_draw_round_rectangle_fill(pill_x, panel_y + header_top_padding, pill_w, 11, 5, signal_color,
                                          egui_color_alpha_mix(self->alpha, local->locked_mode ? EGUI_ALPHA_30 : EGUI_ALPHA_60));
    text_region.location.x = pill_x;
    text_region.location.y = panel_y + header_top_padding;
    text_region.size.width = pill_w;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(local->font, snapshot->status ? snapshot->status : "LIVE", &text_region, EGUI_ALIGN_CENTER, local->text_color, self->alpha);

    center_x = panel_x + panel_w / 2;
    center_y = panel_y + (local->compact_mode ? 39 : 55);
    outer_radius = local->compact_mode ? 17 : 24;
    mid_radius = local->compact_mode ? 11 : 16;
    core_radius = local->compact_mode ? 6 : 9;
    node_offset = local->compact_mode ? 24 : 37;
    node_radius = local->compact_mode ? 5 : 6;

    egui_canvas_draw_circle_fill(center_x, center_y, outer_radius + 4, halo_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_10));
    for (i = 0; i < 3; i++)
    {
        if (i >= snapshot->signal_level)
        {
            break;
        }
        egui_canvas_draw_circle(center_x, center_y, outer_radius - i * (local->compact_mode ? 4 : 5), 1, signal_color,
                                egui_color_alpha_mix(self->alpha, (i == 0) ? EGUI_ALPHA_60 : EGUI_ALPHA_30));
    }

    egui_canvas_draw_circle_fill(center_x, center_y, mid_radius, egui_rgb_mix(local->surface_color, signal_color, EGUI_ALPHA_20),
                                 egui_color_alpha_mix(self->alpha, EGUI_ALPHA_80));
    egui_canvas_draw_circle(center_x, center_y, mid_radius, 1, signal_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_70));
    egui_canvas_draw_circle_fill(center_x, center_y, core_radius, signal_color,
                                 egui_color_alpha_mix(self->alpha, local->locked_mode ? EGUI_ALPHA_40 : EGUI_ALPHA_90));

    egui_canvas_draw_line(center_x - node_offset + node_radius, center_y, center_x - outer_radius - 3, center_y, 1, local->border_color,
                          egui_color_alpha_mix(self->alpha, EGUI_ALPHA_50));
    egui_canvas_draw_line(center_x + outer_radius + 3, center_y, center_x + node_offset - node_radius, center_y, 1, local->border_color,
                          egui_color_alpha_mix(self->alpha, EGUI_ALPHA_50));

    egui_view_signal_beacon_draw_node(local, self, center_x - node_offset, center_y, node_radius, signal_color, snapshot->focus_node == 0);
    egui_view_signal_beacon_draw_node(local, self, center_x + node_offset, center_y, node_radius, signal_color, snapshot->focus_node == 2);

    pulse_w = local->compact_mode ? 8 : 11;
    pulse_gap = local->compact_mode ? 4 : 4;
    pulse_y = center_y + outer_radius + (local->compact_mode ? 5 : 7);
    for (i = 0; i < 3; i++)
    {
        egui_dim_t pulse_x;
        pulse_x = center_x - ((pulse_w * 3 + pulse_gap * 2) / 2) + i * (pulse_w + pulse_gap);
        egui_canvas_draw_round_rectangle_fill(pulse_x, pulse_y, pulse_w, 3, 1, (i < snapshot->signal_level) ? signal_color : local->border_color,
                                              egui_color_alpha_mix(self->alpha, (i < snapshot->signal_level) ? EGUI_ALPHA_80 : EGUI_ALPHA_20));
    }

    footer_y = pulse_y + (local->compact_mode ? 5 : 5);
    footer_side_padding = side_padding + (local->compact_mode ? 0 : 1);
    text_region.location.x = panel_x + footer_side_padding;
    text_region.location.y = footer_y;
    text_region.size.width = panel_w - footer_side_padding * 2;
    text_region.size.height = 12;
    if (local->compact_mode)
    {
        egui_canvas_draw_text_in_rect(local->font, snapshot->caption ? snapshot->caption : "LINK", &text_region, EGUI_ALIGN_CENTER, local->muted_text_color,
                                      self->alpha);
    }
    else
    {
        egui_canvas_draw_text_in_rect(local->font, snapshot->caption ? snapshot->caption : "Signal stable", &text_region, EGUI_ALIGN_LEFT,
                                      local->muted_text_color, self->alpha);
        egui_canvas_draw_text_in_rect(local->font, snapshot->footer ? snapshot->footer : "node clear", &text_region, EGUI_ALIGN_RIGHT,
                                      local->locked_mode ? local->muted_text_color : local->text_color, self->alpha);
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_signal_beacon_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_signal_beacon_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_signal_beacon_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_signal_beacon_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_signal_beacon_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x13202D);
    local->border_color = EGUI_COLOR_HEX(0x3C5568);
    local->text_color = EGUI_COLOR_HEX(0xE5F3FF);
    local->muted_text_color = EGUI_COLOR_HEX(0x8EA6B7);
    local->accent_color = EGUI_COLOR_HEX(0x38BDF8);
    local->warn_color = EGUI_COLOR_HEX(0xF59E0B);
    local->critical_color = EGUI_COLOR_HEX(0xF97373);
    local->node_fill_color = EGUI_COLOR_HEX(0x0E1720);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->compact_mode = 0;
    local->locked_mode = 0;
}
