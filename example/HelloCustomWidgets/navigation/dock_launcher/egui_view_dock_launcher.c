#include <stdlib.h>

#include "egui_view_dock_launcher.h"

static uint8_t egui_view_dock_launcher_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_DOCK_LAUNCHER_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_DOCK_LAUNCHER_MAX_SNAPSHOTS;
    }
    return count;
}

void egui_view_dock_launcher_set_snapshots(egui_view_t *self, const egui_view_dock_launcher_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_dock_launcher_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_dock_launcher_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_dock_launcher_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_dock_launcher_t);
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

uint8_t egui_view_dock_launcher_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_dock_launcher_t);
    return local->current_snapshot;
}

void egui_view_dock_launcher_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_dock_launcher_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_dock_launcher_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_dock_launcher_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_dock_launcher_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_dock_launcher_t);
    local->locked_mode = locked_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_dock_launcher_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t strip_color,
        egui_color_t border_color,
        egui_color_t text_color,
        egui_color_t muted_text_color,
        egui_color_t accent_color,
        egui_color_t warn_color,
        egui_color_t lock_color,
        egui_color_t focus_color)
{
    EGUI_LOCAL_INIT(egui_view_dock_launcher_t);
    local->surface_color = surface_color;
    local->strip_color = strip_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->warn_color = warn_color;
    local->lock_color = lock_color;
    local->focus_color = focus_color;
    egui_view_invalidate(self);
}

static egui_color_t egui_view_dock_launcher_get_status_color(egui_view_dock_launcher_t *local, const egui_view_dock_launcher_snapshot_t *snapshot)
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

static void egui_view_dock_launcher_draw_icons(
        egui_view_dock_launcher_t *local,
        egui_view_t *self,
        const egui_view_dock_launcher_snapshot_t *snapshot,
        egui_dim_t strip_x,
        egui_dim_t strip_y,
        egui_dim_t strip_w,
        egui_dim_t strip_h)
{
    egui_dim_t icon_count;
    egui_dim_t i;
    egui_dim_t center_x;
    egui_dim_t center_y;
    egui_dim_t slot_step;
    egui_dim_t focus_index;
    egui_color_t status_color;

    icon_count = local->compact_mode ? 3 : 5;
    slot_step = strip_w / (icon_count + 1);
    center_y = strip_y + strip_h / 2 - (local->compact_mode ? 1 : 2);
    focus_index = snapshot->focus_index % icon_count;
    status_color = egui_view_dock_launcher_get_status_color(local, snapshot);

    for (i = 0; i < icon_count; i++)
    {
        egui_dim_t icon_size;
        egui_dim_t icon_x;
        egui_dim_t icon_y;
        egui_color_t icon_fill;
        uint8_t alpha;

        center_x = strip_x + slot_step * (i + 1);
        icon_size = (i == focus_index) ? (local->compact_mode ? 15 : 19) : (local->compact_mode ? 11 : 13);
        icon_x = center_x - icon_size / 2;
        icon_y = center_y - icon_size / 2;
        icon_fill = (i == focus_index) ? status_color : egui_rgb_mix(local->surface_color, local->border_color, 38);
        alpha = (uint8_t)((i == focus_index) ? 92 : 72);

        egui_canvas_draw_round_rectangle_fill(
                icon_x,
                icon_y,
                icon_size,
                icon_size,
                local->compact_mode ? 4 : 5,
                icon_fill,
                egui_color_alpha_mix(self->alpha, alpha));
        egui_canvas_draw_round_rectangle(
                icon_x,
                icon_y,
                icon_size,
                icon_size,
                local->compact_mode ? 4 : 5,
                1,
                (i == focus_index) ? local->focus_color : local->border_color,
                egui_color_alpha_mix(self->alpha, (i == focus_index) ? 70 : 42));
        egui_canvas_draw_circle_fill(
                center_x,
                center_y,
                (i == focus_index) ? (local->compact_mode ? 2 : 3) : 1,
                local->text_color,
                egui_color_alpha_mix(self->alpha, (i == focus_index) ? 92 : 76));

        if (snapshot->active_mask & (1u << i))
        {
            egui_canvas_draw_circle_fill(
                    center_x,
                    strip_y + strip_h - (local->compact_mode ? 4 : 5),
                    1,
                    status_color,
                    egui_color_alpha_mix(self->alpha, 92));
        }
    }

    if (snapshot->badge_count > 0)
    {
        center_x = strip_x + slot_step * (focus_index + 1);
        egui_canvas_draw_circle_fill(
                center_x + (local->compact_mode ? 7 : 9),
                center_y - (local->compact_mode ? 6 : 8),
                local->compact_mode ? 3 : 4,
                status_color,
                egui_color_alpha_mix(self->alpha, 96));
        egui_canvas_draw_circle_fill(
                center_x + (local->compact_mode ? 7 : 9),
                center_y - (local->compact_mode ? 6 : 8),
                1,
                local->text_color,
                egui_color_alpha_mix(self->alpha, 96));
    }
}

static void egui_view_dock_launcher_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_dock_launcher_t);
    egui_region_t region;
    egui_region_t text_region;
    const egui_view_dock_launcher_snapshot_t *snapshot;
    egui_color_t shell_color;
    egui_color_t status_color;
    egui_dim_t panel_x;
    egui_dim_t panel_y;
    egui_dim_t panel_w;
    egui_dim_t panel_h;
    egui_dim_t outer_padding;
    egui_dim_t header_top;
    egui_dim_t pill_w;
    egui_dim_t pill_x;
    egui_dim_t title_w;
    egui_dim_t summary_y;
    egui_dim_t strip_x;
    egui_dim_t strip_y;
    egui_dim_t strip_w;
    egui_dim_t strip_h;
    egui_dim_t footer_y;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    status_color = egui_view_dock_launcher_get_status_color(local, snapshot);
    shell_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, 28);

    panel_x = region.location.x;
    panel_y = region.location.y;
    panel_w = region.size.width;
    panel_h = region.size.height;
    outer_padding = local->compact_mode ? 14 : 13;
    header_top = local->compact_mode ? 9 : 8;
    pill_w = local->compact_mode ? 37 : 46;

    egui_canvas_draw_round_rectangle_fill(panel_x, panel_y, panel_w, panel_h, 10, shell_color, egui_color_alpha_mix(self->alpha, 45));
    egui_canvas_draw_round_rectangle(panel_x, panel_y, panel_w, panel_h, 10, 1, local->border_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));

    pill_x = panel_x + panel_w - outer_padding - pill_w;
    title_w = pill_x - panel_x - outer_padding - (local->compact_mode ? 14 : 15);

    text_region.location.x = panel_x + outer_padding + (local->compact_mode ? 3 : 1);
    text_region.location.y = panel_y + header_top;
    text_region.size.width = title_w;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(local->font, snapshot->title ? snapshot->title : "DOCK", &text_region, EGUI_ALIGN_LEFT, local->muted_text_color, self->alpha);

    egui_canvas_draw_round_rectangle_fill(
            pill_x,
            panel_y + header_top,
            pill_w,
            11,
            5,
            status_color,
            egui_color_alpha_mix(self->alpha, local->locked_mode ? 30 : 62));
    text_region.location.x = pill_x + 2;
    text_region.location.y = panel_y + header_top;
    text_region.size.width = pill_w - 4;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(local->font, snapshot->status ? snapshot->status : "FOCUS", &text_region, EGUI_ALIGN_CENTER, local->text_color, self->alpha);

    summary_y = panel_y + (local->compact_mode ? 24 : 29);
    text_region.location.x = panel_x + outer_padding + (local->compact_mode ? 2 : 6);
    text_region.location.y = summary_y;
    text_region.size.width = panel_w - outer_padding * 2 - (local->compact_mode ? 4 : 12);
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(local->font, snapshot->summary ? snapshot->summary : "Summary", &text_region, EGUI_ALIGN_CENTER, local->text_color, self->alpha);

    if (!local->compact_mode)
    {
        text_region.location.y = summary_y + 7;
        text_region.size.height = 10;
        egui_canvas_draw_text_in_rect(local->font, snapshot->footer ? snapshot->footer : "Footer", &text_region, EGUI_ALIGN_CENTER, local->muted_text_color, self->alpha);
    }

    strip_x = panel_x + outer_padding;
    strip_y = panel_y + (local->compact_mode ? 40 : 57);
    strip_w = panel_w - outer_padding * 2;
    strip_h = local->compact_mode ? 21 : 33;

    egui_canvas_draw_round_rectangle_fill(strip_x, strip_y, strip_w, strip_h, 8, local->strip_color, egui_color_alpha_mix(self->alpha, 78));
    egui_canvas_draw_round_rectangle(strip_x, strip_y, strip_w, strip_h, 8, 1, local->border_color, egui_color_alpha_mix(self->alpha, 44));

    egui_canvas_draw_line(
            strip_x + 8,
            strip_y + strip_h / 2,
            strip_x + strip_w - 9,
            strip_y + strip_h / 2,
            1,
            local->border_color,
            egui_color_alpha_mix(self->alpha, 22));

    egui_view_dock_launcher_draw_icons(local, self, snapshot, strip_x, strip_y, strip_w, strip_h);

    footer_y = strip_y + strip_h + (local->compact_mode ? 1 : 4);
    if (local->compact_mode)
    {
        text_region.location.x = panel_x + outer_padding + 8;
        text_region.location.y = footer_y;
        text_region.size.width = panel_w - outer_padding * 2 - 16;
        text_region.size.height = EGUI_MAX(panel_h - (footer_y - panel_y) - 6, 10);
        egui_canvas_draw_text_in_rect(local->font, snapshot->footer ? snapshot->footer : "footer", &text_region, EGUI_ALIGN_CENTER, local->muted_text_color, self->alpha);
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_dock_launcher_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_dock_launcher_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_dock_launcher_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_dock_launcher_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_dock_launcher_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x0E1523);
    local->strip_color = EGUI_COLOR_HEX(0x18253A);
    local->border_color = EGUI_COLOR_HEX(0x415777);
    local->text_color = EGUI_COLOR_HEX(0xEEF4FF);
    local->muted_text_color = EGUI_COLOR_HEX(0x8A9DB8);
    local->accent_color = EGUI_COLOR_HEX(0x67D6FF);
    local->warn_color = EGUI_COLOR_HEX(0xF2B05E);
    local->lock_color = EGUI_COLOR_HEX(0xD6A36E);
    local->focus_color = EGUI_COLOR_HEX(0x91EEFF);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->compact_mode = 0;
    local->locked_mode = 0;
}
