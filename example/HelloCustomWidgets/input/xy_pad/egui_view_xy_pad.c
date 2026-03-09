#include <stdlib.h>

#include "egui_view_xy_pad.h"

static uint8_t egui_view_xy_pad_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_XY_PAD_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_XY_PAD_MAX_SNAPSHOTS;
    }
    return count;
}

void egui_view_xy_pad_set_snapshots(egui_view_t *self, const egui_view_xy_pad_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_xy_pad_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_xy_pad_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_xy_pad_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_xy_pad_t);
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

uint8_t egui_view_xy_pad_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_xy_pad_t);
    return local->current_snapshot;
}

void egui_view_xy_pad_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_xy_pad_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_xy_pad_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_xy_pad_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_xy_pad_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_xy_pad_t);
    local->locked_mode = locked_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_xy_pad_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t pad_color,
        egui_color_t border_color,
        egui_color_t text_color,
        egui_color_t muted_text_color,
        egui_color_t accent_color,
        egui_color_t warn_color,
        egui_color_t lock_color,
        egui_color_t focus_color)
{
    EGUI_LOCAL_INIT(egui_view_xy_pad_t);
    local->surface_color = surface_color;
    local->pad_color = pad_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->warn_color = warn_color;
    local->lock_color = lock_color;
    local->focus_color = focus_color;
    egui_view_invalidate(self);
}

static egui_color_t egui_view_xy_pad_get_status_color(egui_view_xy_pad_t *local, const egui_view_xy_pad_snapshot_t *snapshot)
{
    if (snapshot->accent_mode >= 2)
    {
        return local->lock_color;
    }
    if (snapshot->accent_mode == 1)
    {
        return local->warn_color;
    }
    return local->accent_color;
}

static void egui_view_xy_pad_draw_grid(
        egui_view_xy_pad_t *local,
        egui_view_t *self,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t width,
        egui_dim_t height)
{
    egui_dim_t col_step;
    egui_dim_t row_step;
    egui_dim_t i;

    col_step = width / 4;
    row_step = height / 4;
    for (i = 1; i < 4; i++)
    {
        egui_canvas_draw_line(
                x + col_step * i,
                y + 2,
                x + col_step * i,
                y + height - 3,
                1,
                local->border_color,
                egui_color_alpha_mix(self->alpha, i == 2 ? 35 : EGUI_ALPHA_20));
        egui_canvas_draw_line(
                x + 2,
                y + row_step * i,
                x + width - 3,
                y + row_step * i,
                1,
                local->border_color,
                egui_color_alpha_mix(self->alpha, i == 2 ? 35 : EGUI_ALPHA_20));
    }
}

static void egui_view_xy_pad_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_xy_pad_t);
    egui_region_t region;
    egui_region_t text_region;
    const egui_view_xy_pad_snapshot_t *snapshot;
    egui_color_t status_color;
    egui_color_t shell_color;
    egui_dim_t panel_x;
    egui_dim_t panel_y;
    egui_dim_t panel_w;
    egui_dim_t panel_h;
    egui_dim_t outer_padding;
    egui_dim_t header_top;
    egui_dim_t pill_w;
    egui_dim_t pill_x;
    egui_dim_t title_w;
    egui_dim_t pad_x;
    egui_dim_t pad_y;
    egui_dim_t pad_w;
    egui_dim_t pad_h;
    egui_dim_t knob_x;
    egui_dim_t knob_y;
    egui_dim_t footer_y;
    egui_dim_t footer_h;
    egui_dim_t ring_radius;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    status_color = egui_view_xy_pad_get_status_color(local, snapshot);
    shell_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, 28);

    panel_x = region.location.x;
    panel_y = region.location.y;
    panel_w = region.size.width;
    panel_h = region.size.height;
    outer_padding = local->compact_mode ? 14 : 13;
    header_top = local->compact_mode ? 9 : 8;
    pill_w = local->compact_mode ? 36 : 44;

    egui_canvas_draw_round_rectangle_fill(panel_x, panel_y, panel_w, panel_h, 10, shell_color, egui_color_alpha_mix(self->alpha, 45));
    egui_canvas_draw_round_rectangle(panel_x, panel_y, panel_w, panel_h, 10, 1, local->border_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));

    pill_x = panel_x + panel_w - outer_padding - pill_w;
    title_w = pill_x - panel_x - outer_padding - (local->compact_mode ? 14 : 11);
    text_region.location.x = panel_x + outer_padding + (local->compact_mode ? 1 : 0);
    text_region.location.y = panel_y + header_top;
    text_region.size.width = title_w;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(local->font, snapshot->title ? snapshot->title : "XY PAD", &text_region, EGUI_ALIGN_LEFT, local->muted_text_color, self->alpha);

    egui_canvas_draw_round_rectangle_fill(
            pill_x,
            panel_y + header_top,
            pill_w,
            11,
            5,
            status_color,
            egui_color_alpha_mix(self->alpha, local->locked_mode ? 32 : 62));
    text_region.location.x = pill_x + 1;
    text_region.location.y = panel_y + header_top;
    text_region.size.width = pill_w - 2;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(local->font, snapshot->status ? snapshot->status : "LIVE", &text_region, EGUI_ALIGN_CENTER, local->text_color, self->alpha);

    pad_x = panel_x + outer_padding;
    pad_y = panel_y + (local->compact_mode ? 24 : 26);
    pad_w = panel_w - outer_padding * 2;
    pad_h = local->compact_mode ? 27 : 50;

    egui_canvas_draw_round_rectangle_fill(pad_x, pad_y, pad_w, pad_h, 8, local->pad_color, egui_color_alpha_mix(self->alpha, 76));
    egui_canvas_draw_round_rectangle(pad_x, pad_y, pad_w, pad_h, 8, 1, local->border_color, egui_color_alpha_mix(self->alpha, 45));
    egui_view_xy_pad_draw_grid(local, self, pad_x, pad_y, pad_w, pad_h);

    knob_x = pad_x + 4 + (snapshot->point_x * (pad_w - 8)) / 100;
    knob_y = pad_y + 4 + ((100 - snapshot->point_y) * (pad_h - 8)) / 100;

    egui_canvas_draw_line(knob_x, pad_y + 4, knob_x, pad_y + pad_h - 5, 1, local->focus_color, egui_color_alpha_mix(self->alpha, 35));
    egui_canvas_draw_line(pad_x + 4, knob_y, pad_x + pad_w - 5, knob_y, 1, local->focus_color, egui_color_alpha_mix(self->alpha, 35));

    ring_radius = local->compact_mode ? (snapshot->orbit_size / 3) + 3 : (snapshot->orbit_size / 2) + 5;
    egui_canvas_draw_circle(knob_x, knob_y, ring_radius, 1, status_color, egui_color_alpha_mix(self->alpha, 35));
    egui_canvas_draw_circle_fill(knob_x, knob_y, local->compact_mode ? 3 : 4, status_color, egui_color_alpha_mix(self->alpha, 95));
    egui_canvas_draw_circle_fill(knob_x, knob_y, local->compact_mode ? 1 : 2, local->text_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_100));

    egui_canvas_draw_circle_fill(pad_x + 10, pad_y + 10, 2, local->focus_color, egui_color_alpha_mix(self->alpha, 28));
    egui_canvas_draw_circle_fill(pad_x + pad_w - 11, pad_y + 10, 2, local->focus_color, egui_color_alpha_mix(self->alpha, 28));
    egui_canvas_draw_circle_fill(pad_x + 10, pad_y + pad_h - 11, 2, local->focus_color, egui_color_alpha_mix(self->alpha, 28));
    egui_canvas_draw_circle_fill(pad_x + pad_w - 11, pad_y + pad_h - 11, 2, local->focus_color, egui_color_alpha_mix(self->alpha, 28));

    footer_y = pad_y + pad_h + (local->compact_mode ? 1 : 1);
    footer_h = panel_h - (footer_y - panel_y) - (local->compact_mode ? 6 : 7);

    text_region.location.x = panel_x + outer_padding + (local->compact_mode ? 7 : 5);
    text_region.location.y = footer_y;
    text_region.size.width = panel_w - outer_padding * 2 - (local->compact_mode ? 14 : 10);
    text_region.size.height = local->compact_mode ? footer_h : 10;
    egui_canvas_draw_text_in_rect(local->font, snapshot->preset ? snapshot->preset : "Preset", &text_region, EGUI_ALIGN_CENTER, local->text_color, self->alpha);

    if (!local->compact_mode)
    {
        text_region.location.y = footer_y + 7;
        text_region.size.height = EGUI_MAX(footer_h - 9, 10);
        egui_canvas_draw_text_in_rect(local->font, snapshot->footer ? snapshot->footer : "tilt", &text_region, EGUI_ALIGN_CENTER, local->muted_text_color, self->alpha);
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_xy_pad_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_xy_pad_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_xy_pad_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_xy_pad_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_xy_pad_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x0E1626);
    local->pad_color = EGUI_COLOR_HEX(0x15253A);
    local->border_color = EGUI_COLOR_HEX(0x3E5774);
    local->text_color = EGUI_COLOR_HEX(0xEAF1FC);
    local->muted_text_color = EGUI_COLOR_HEX(0x8597AD);
    local->accent_color = EGUI_COLOR_HEX(0x57D2F5);
    local->warn_color = EGUI_COLOR_HEX(0xF2B05E);
    local->lock_color = EGUI_COLOR_HEX(0xD5A370);
    local->focus_color = EGUI_COLOR_HEX(0x7CE9FF);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->compact_mode = 0;
    local->locked_mode = 0;
}
