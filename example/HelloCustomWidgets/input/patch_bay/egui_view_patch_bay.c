#include <stdlib.h>

#include "egui_view_patch_bay.h"

static const char *const patch_bay_left_labels[] = {"A", "B", "C"};
static const char *const patch_bay_right_labels[] = {"1", "2", "3"};

static uint8_t egui_view_patch_bay_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_PATCH_BAY_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_PATCH_BAY_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_patch_bay_clamp_port_index(uint8_t index)
{
    if (index > 2)
    {
        return 2;
    }
    return index;
}

void egui_view_patch_bay_set_snapshots(egui_view_t *self, const egui_view_patch_bay_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_patch_bay_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_patch_bay_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_patch_bay_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_patch_bay_t);
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

uint8_t egui_view_patch_bay_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_patch_bay_t);
    return local->current_snapshot;
}

void egui_view_patch_bay_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_patch_bay_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_patch_bay_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_patch_bay_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_patch_bay_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_patch_bay_t);
    local->locked_mode = locked_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_patch_bay_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t panel_color,
        egui_color_t border_color,
        egui_color_t text_color,
        egui_color_t muted_text_color,
        egui_color_t accent_color,
        egui_color_t warn_color,
        egui_color_t lock_color,
        egui_color_t route_bg_color)
{
    EGUI_LOCAL_INIT(egui_view_patch_bay_t);
    local->surface_color = surface_color;
    local->panel_color = panel_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->warn_color = warn_color;
    local->lock_color = lock_color;
    local->route_bg_color = route_bg_color;
    egui_view_invalidate(self);
}

static egui_color_t egui_view_patch_bay_get_signal_color(egui_view_patch_bay_t *local, const egui_view_patch_bay_snapshot_t *snapshot)
{
    if (snapshot->alert_level >= 2)
    {
        return local->lock_color;
    }
    if (snapshot->alert_level == 1)
    {
        return local->warn_color;
    }
    return local->accent_color;
}

static void egui_view_patch_bay_draw_port(
        egui_view_patch_bay_t *local,
        egui_view_t *self,
        egui_dim_t cx,
        egui_dim_t cy,
        egui_dim_t radius,
        egui_color_t signal_color,
        uint8_t is_active)
{
    egui_color_t fill_color;

    fill_color = is_active ? egui_rgb_mix(local->route_bg_color, signal_color, EGUI_ALPHA_30) : local->route_bg_color;
    egui_canvas_draw_circle_fill(cx, cy, radius + (is_active ? 2 : 1), fill_color, egui_color_alpha_mix(self->alpha, is_active ? EGUI_ALPHA_90 : 55));
    egui_canvas_draw_circle(cx, cy, radius, is_active ? 2 : 1, is_active ? signal_color : local->border_color, egui_color_alpha_mix(self->alpha, is_active ? EGUI_ALPHA_90 : EGUI_ALPHA_50));
    egui_canvas_draw_circle_fill(cx, cy, EGUI_MAX(radius / 2, 2), is_active ? signal_color : local->muted_text_color, egui_color_alpha_mix(self->alpha, is_active ? EGUI_ALPHA_90 : 45));
}

static void egui_view_patch_bay_draw_route(
        egui_view_patch_bay_t *local,
        egui_view_t *self,
        egui_dim_t x1,
        egui_dim_t y1,
        egui_dim_t x2,
        egui_dim_t y2,
        egui_dim_t hub_x,
        egui_color_t signal_color,
        uint8_t is_active)
{
    egui_alpha_t alpha;
    egui_color_t color;
    egui_dim_t stroke;
    egui_dim_t joint_radius;

    alpha = egui_color_alpha_mix(
            self->alpha,
            is_active ? (local->locked_mode ? 60 : 90) : (local->compact_mode ? EGUI_ALPHA_10 : 12));
    color = is_active ? signal_color : local->border_color;
    stroke = is_active ? (local->compact_mode ? 2 : 3) : 1;
    joint_radius = is_active ? (local->compact_mode ? 2 : 3) : 1;

    egui_canvas_draw_line(x1, y1, hub_x, y1, stroke, color, alpha);
    egui_canvas_draw_line(hub_x, y1, hub_x, y2, stroke, color, alpha);
    egui_canvas_draw_line(hub_x, y2, x2, y2, stroke, color, alpha);
    egui_canvas_draw_circle_fill(hub_x, y1, joint_radius, color, alpha);
    egui_canvas_draw_circle_fill(hub_x, y2, joint_radius, color, alpha);
}

static void egui_view_patch_bay_draw_label(
        egui_view_patch_bay_t *local,
        egui_view_t *self,
        const char *text,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t width,
        uint8_t align,
        egui_color_t color)
{
    egui_region_t text_region;

    text_region.location.x = x;
    text_region.location.y = y;
    text_region.size.width = width;
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(local->font, text, &text_region, align, color, self->alpha);
}

static void egui_view_patch_bay_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_patch_bay_t);
    egui_region_t region;
    egui_region_t text_region;
    const egui_view_patch_bay_snapshot_t *snapshot;
    egui_color_t signal_color;
    egui_color_t shell_color;
    egui_dim_t panel_x;
    egui_dim_t panel_y;
    egui_dim_t panel_w;
    egui_dim_t panel_h;
    egui_dim_t outer_padding;
    egui_dim_t header_top;
    egui_dim_t pill_w;
    egui_dim_t pill_side_gap;
    egui_dim_t pill_x;
    egui_dim_t title_w;
    egui_dim_t bay_x;
    egui_dim_t bay_y;
    egui_dim_t bay_w;
    egui_dim_t bay_h;
    egui_dim_t left_port_x;
    egui_dim_t right_port_x;
    egui_dim_t port_y0;
    egui_dim_t port_gap;
    egui_dim_t port_radius;
    egui_dim_t stub_len;
    egui_dim_t hub_x;
    egui_dim_t label_y;
    egui_dim_t footer_y;
    egui_dim_t footer_h;
    egui_dim_t route_text_w;
    uint8_t active_left;
    uint8_t active_right;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    signal_color = egui_view_patch_bay_get_signal_color(local, snapshot);
    shell_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, 28);

    panel_x = region.location.x;
    panel_y = region.location.y;
    panel_w = region.size.width;
    panel_h = region.size.height;
    outer_padding = local->compact_mode ? 11 : 13;
    header_top = local->compact_mode ? 6 : 7;
    pill_w = local->compact_mode ? 33 : 41;
    pill_side_gap = local->compact_mode ? 11 : 13;

    egui_canvas_draw_round_rectangle_fill(panel_x, panel_y, panel_w, panel_h, 10, shell_color, egui_color_alpha_mix(self->alpha, 45));
    egui_canvas_draw_round_rectangle(panel_x, panel_y, panel_w, panel_h, 10, 1, local->border_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));

    pill_x = panel_x + panel_w - pill_side_gap - pill_w;
    title_w = pill_x - panel_x - outer_padding - 8;
    text_region.location.x = panel_x + outer_padding;
    text_region.location.y = panel_y + header_top;
    text_region.size.width = title_w;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(local->font, snapshot->title ? snapshot->title : "PATCH", &text_region, EGUI_ALIGN_LEFT, local->muted_text_color, self->alpha);

    egui_canvas_draw_round_rectangle_fill(
            pill_x,
            panel_y + header_top,
            pill_w,
            11,
            5,
            signal_color,
            egui_color_alpha_mix(self->alpha, local->locked_mode ? 32 : 62));
    text_region.location.x = pill_x + 1;
    text_region.location.y = panel_y + header_top;
    text_region.size.width = pill_w - 2;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(local->font, snapshot->status ? snapshot->status : "LIVE", &text_region, EGUI_ALIGN_CENTER, local->text_color, self->alpha);

    bay_x = panel_x + outer_padding;
    bay_y = panel_y + (local->compact_mode ? 22 : 24);
    bay_w = panel_w - outer_padding * 2;
    bay_h = local->compact_mode ? 29 : 45;

    egui_canvas_draw_round_rectangle_fill(bay_x, bay_y, bay_w, bay_h, 8, local->panel_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_70));
    egui_canvas_draw_round_rectangle(bay_x, bay_y, bay_w, bay_h, 8, 1, local->border_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40));

    label_y = bay_y + (local->compact_mode ? 3 : 4);
    left_port_x = bay_x + (local->compact_mode ? 14 : 18);
    right_port_x = bay_x + bay_w - (local->compact_mode ? 14 : 18);
    port_y0 = bay_y + (local->compact_mode ? 11 : 17);
    port_gap = local->compact_mode ? 8 : 11;
    port_radius = local->compact_mode ? 4 : 5;
    stub_len = local->compact_mode ? 6 : 8;
    hub_x = bay_x + bay_w / 2;

    egui_view_patch_bay_draw_label(
            local,
            self,
            "IN",
            left_port_x - (local->compact_mode ? 7 : 8),
            label_y,
            local->compact_mode ? 14 : 16,
            EGUI_ALIGN_CENTER,
            local->muted_text_color);
    egui_view_patch_bay_draw_label(
            local,
            self,
            "OUT",
            right_port_x - (local->compact_mode ? 8 : 9),
            label_y,
            local->compact_mode ? 16 : 18,
            EGUI_ALIGN_CENTER,
            local->muted_text_color);

    active_left = egui_view_patch_bay_clamp_port_index(snapshot->left_port);
    active_right = egui_view_patch_bay_clamp_port_index(snapshot->right_port);

    for (i = 0; i < 3; i++)
    {
        egui_dim_t port_y;
        egui_dim_t label_y_offset;
        uint8_t is_left_active;
        uint8_t is_right_active;

        port_y = port_y0 + i * port_gap;
        label_y_offset = local->compact_mode ? 0 : 1;
        is_left_active = i == active_left;
        is_right_active = i == active_right;

        egui_view_patch_bay_draw_port(local, self, left_port_x, port_y, port_radius, signal_color, is_left_active);
        egui_view_patch_bay_draw_port(local, self, right_port_x, port_y, port_radius, signal_color, is_right_active);
        egui_canvas_draw_line(
                left_port_x + port_radius + 2,
                port_y,
                left_port_x + port_radius + 2 + stub_len,
                port_y,
                1,
                local->border_color,
                egui_color_alpha_mix(self->alpha, is_left_active ? 45 : EGUI_ALPHA_20));
        egui_canvas_draw_line(
                right_port_x - port_radius - 2 - stub_len,
                port_y,
                right_port_x - port_radius - 2,
                port_y,
                1,
                local->border_color,
                egui_color_alpha_mix(self->alpha, is_right_active ? 45 : EGUI_ALPHA_20));

        if (!local->compact_mode)
        {
            egui_view_patch_bay_draw_label(
                    local,
                    self,
                    patch_bay_left_labels[i],
                    left_port_x - 10,
                    port_y - 4 + label_y_offset,
                    8,
                    EGUI_ALIGN_CENTER,
                    is_left_active ? local->text_color : local->muted_text_color);
            egui_view_patch_bay_draw_label(
                    local,
                    self,
                    patch_bay_right_labels[i],
                    right_port_x + 3,
                    port_y - 4 + label_y_offset,
                    8,
                    EGUI_ALIGN_CENTER,
                    is_right_active ? local->text_color : local->muted_text_color);
        }
    }

    for (i = 0; i < 3; i++)
    {
        egui_dim_t ghost_y;

        ghost_y = port_y0 + i * port_gap;
        egui_view_patch_bay_draw_route(
                local,
                self,
                left_port_x + port_radius + 2 + stub_len,
                ghost_y,
                right_port_x - port_radius - 2 - stub_len,
                ghost_y,
                hub_x,
                signal_color,
                0);
    }

    egui_view_patch_bay_draw_route(
            local,
            self,
            left_port_x + port_radius + 2 + stub_len,
            port_y0 + active_left * port_gap,
            right_port_x - port_radius - 2 - stub_len,
            port_y0 + active_right * port_gap,
            hub_x,
            signal_color,
            1);

    egui_canvas_draw_round_rectangle_fill(
            hub_x - (local->compact_mode ? 5 : 8),
            bay_y + (local->compact_mode ? 5 : 8),
            local->compact_mode ? 10 : 16,
            bay_h - (local->compact_mode ? 10 : 16),
            local->compact_mode ? 4 : 6,
            egui_rgb_mix(local->route_bg_color, signal_color, 8),
            egui_color_alpha_mix(self->alpha, 28));
    egui_canvas_draw_round_rectangle(
            hub_x - (local->compact_mode ? 5 : 8),
            bay_y + (local->compact_mode ? 5 : 8),
            local->compact_mode ? 10 : 16,
            bay_h - (local->compact_mode ? 10 : 16),
            local->compact_mode ? 4 : 6,
            1,
            local->border_color,
            egui_color_alpha_mix(self->alpha, 24));

    footer_y = bay_y + bay_h + (local->compact_mode ? 3 : 5);
    footer_h = panel_h - (footer_y - panel_y) - (local->compact_mode ? 6 : 7);
    route_text_w = panel_w - outer_padding * 2;

    text_region.location.x = panel_x + outer_padding + (local->compact_mode ? 2 : 2);
    text_region.location.y = footer_y;
    text_region.size.width = route_text_w - (local->compact_mode ? 4 : 4);
    text_region.size.height = local->compact_mode ? footer_h : 10;
    egui_canvas_draw_text_in_rect(local->font, snapshot->route ? snapshot->route : "Input -> Bus", &text_region, EGUI_ALIGN_CENTER, local->text_color, self->alpha);

    if (!local->compact_mode)
    {
        text_region.location.y = footer_y + 10;
        text_region.size.height = EGUI_MAX(footer_h - 10, 10);
        egui_canvas_draw_text_in_rect(local->font, snapshot->footer ? snapshot->footer : "route stable", &text_region, EGUI_ALIGN_CENTER, local->muted_text_color, self->alpha);
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_patch_bay_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_patch_bay_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_patch_bay_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_patch_bay_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_patch_bay_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x101927);
    local->panel_color = EGUI_COLOR_HEX(0x162234);
    local->border_color = EGUI_COLOR_HEX(0x40536C);
    local->text_color = EGUI_COLOR_HEX(0xE2ECF6);
    local->muted_text_color = EGUI_COLOR_HEX(0x8193AA);
    local->accent_color = EGUI_COLOR_HEX(0x53C9FF);
    local->warn_color = EGUI_COLOR_HEX(0xF5B24B);
    local->lock_color = EGUI_COLOR_HEX(0xE29F66);
    local->route_bg_color = EGUI_COLOR_HEX(0x0B111A);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->compact_mode = 0;
    local->locked_mode = 0;
}

