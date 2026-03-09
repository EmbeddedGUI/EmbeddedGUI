#include <stdlib.h>

#include "egui_view_command_palette.h"

static uint8_t egui_view_command_palette_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_COMMAND_PALETTE_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_COMMAND_PALETTE_MAX_SNAPSHOTS;
    }
    return count;
}

void egui_view_command_palette_set_snapshots(egui_view_t *self, const egui_view_command_palette_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_command_palette_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_command_palette_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_command_palette_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_command_palette_t);
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

uint8_t egui_view_command_palette_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_command_palette_t);
    return local->current_snapshot;
}

void egui_view_command_palette_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_command_palette_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_command_palette_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_command_palette_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_command_palette_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_command_palette_t);
    local->locked_mode = locked_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_command_palette_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t border_color,
        egui_color_t track_color,
        egui_color_t query_color,
        egui_color_t text_color,
        egui_color_t muted_text_color,
        egui_color_t accent_color,
        egui_color_t warn_color,
        egui_color_t lock_color,
        egui_color_t focus_color)
{
    EGUI_LOCAL_INIT(egui_view_command_palette_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->track_color = track_color;
    local->query_color = query_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->warn_color = warn_color;
    local->lock_color = lock_color;
    local->focus_color = focus_color;
    egui_view_invalidate(self);
}

static egui_color_t egui_view_command_palette_get_status_color(egui_view_command_palette_t *local, const egui_view_command_palette_snapshot_t *snapshot)
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

static void egui_view_command_palette_draw_shortcut(
        egui_view_t *self,
        egui_view_command_palette_t *local,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t w,
        egui_dim_t h,
        const char *text,
        egui_color_t fill_color)
{
    egui_region_t text_region;

    egui_canvas_draw_round_rectangle_fill(x, y, w, h, h / 2, fill_color, egui_color_alpha_mix(self->alpha, 56));
    egui_canvas_draw_round_rectangle(x, y, w, h, h / 2, 1, local->border_color, egui_color_alpha_mix(self->alpha, 38));

    text_region.location.x = x + 1;
    text_region.location.y = y;
    text_region.size.width = w - 2;
    text_region.size.height = h;
    egui_canvas_draw_text_in_rect(local->font, text, &text_region, EGUI_ALIGN_CENTER, local->text_color, self->alpha);
}

static void egui_view_command_palette_draw_row(
        egui_view_t *self,
        egui_view_command_palette_t *local,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t w,
        egui_dim_t h,
        const char *text,
        const char *shortcut,
        uint8_t highlighted,
        egui_color_t accent_color)
{
    egui_region_t text_region;
    egui_dim_t chip_w;
    egui_dim_t text_right_padding;

    egui_canvas_draw_round_rectangle_fill(
            x,
            y,
            w,
            h,
            7,
            highlighted ? local->query_color : local->track_color,
            egui_color_alpha_mix(self->alpha, highlighted ? 88 : 58));
    egui_canvas_draw_round_rectangle(
            x,
            y,
            w,
            h,
            7,
            1,
            highlighted ? local->focus_color : local->border_color,
            egui_color_alpha_mix(self->alpha, highlighted ? 62 : 34));

    egui_canvas_draw_circle_fill(x + 8, y + h / 2, 2, accent_color, egui_color_alpha_mix(self->alpha, highlighted ? 92 : 70));

    text_right_padding = local->compact_mode ? 8 : 30;
    if (!local->compact_mode)
    {
        chip_w = 22;
        egui_view_command_palette_draw_shortcut(self, local, x + w - chip_w - 6, y + 3, chip_w, h - 6, shortcut, accent_color);
    }

    text_region.location.x = x + 14;
    text_region.location.y = y;
    text_region.size.width = w - text_right_padding - 14;
    text_region.size.height = h;
    egui_canvas_draw_text_in_rect(local->font, text, &text_region, EGUI_ALIGN_LEFT, local->text_color, self->alpha);
}

static void egui_view_command_palette_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_command_palette_t);
    egui_region_t region;
    egui_region_t text_region;
    const egui_view_command_palette_snapshot_t *snapshot;
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
    egui_dim_t query_y;
    egui_dim_t query_h;
    egui_dim_t row_x;
    egui_dim_t row_y;
    egui_dim_t row_w;
    egui_dim_t row_h;
    egui_dim_t footer_y;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    status_color = egui_view_command_palette_get_status_color(local, snapshot);
    shell_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, 30);

    panel_x = region.location.x;
    panel_y = region.location.y;
    panel_w = region.size.width;
    panel_h = region.size.height;
    outer_padding = local->compact_mode ? 13 : 13;
    header_top = local->compact_mode ? 9 : 9;
    pill_w = local->compact_mode ? 36 : 42;

    egui_canvas_draw_round_rectangle_fill(panel_x, panel_y, panel_w, panel_h, 10, shell_color, egui_color_alpha_mix(self->alpha, 45));
    egui_canvas_draw_round_rectangle(panel_x, panel_y, panel_w, panel_h, 10, 1, local->border_color, egui_color_alpha_mix(self->alpha, 62));

    pill_x = panel_x + panel_w - outer_padding - pill_w;
    title_w = pill_x - panel_x - outer_padding - (local->compact_mode ? 11 : 14);

    text_region.location.x = panel_x + outer_padding + (local->compact_mode ? 2 : 2);
    text_region.location.y = panel_y + header_top;
    text_region.size.width = title_w;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(local->font, snapshot->title, &text_region, EGUI_ALIGN_LEFT, local->muted_text_color, self->alpha);

    egui_canvas_draw_round_rectangle_fill(
            pill_x,
            panel_y + header_top,
            pill_w,
            11,
            5,
            status_color,
            egui_color_alpha_mix(self->alpha, local->locked_mode ? 28 : 62));

    text_region.location.x = pill_x + 1;
    text_region.location.y = panel_y + header_top;
    text_region.size.width = pill_w - 2;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(local->font, snapshot->status, &text_region, EGUI_ALIGN_CENTER, local->text_color, self->alpha);

    query_y = panel_y + (local->compact_mode ? 24 : 28);
    query_h = local->compact_mode ? 15 : 18;
    egui_canvas_draw_round_rectangle_fill(
            panel_x + outer_padding,
            query_y,
            panel_w - outer_padding * 2,
            query_h,
            query_h / 2,
            local->query_color,
            egui_color_alpha_mix(self->alpha, 78));
    egui_canvas_draw_round_rectangle(
            panel_x + outer_padding,
            query_y,
            panel_w - outer_padding * 2,
            query_h,
            query_h / 2,
            1,
            (snapshot->highlight_index == 0) ? local->focus_color : local->border_color,
            egui_color_alpha_mix(self->alpha, (snapshot->highlight_index == 0) ? 64 : 38));
    egui_canvas_draw_circle_fill(panel_x + outer_padding + 9, query_y + query_h / 2, 3, status_color, egui_color_alpha_mix(self->alpha, 88));
    egui_canvas_draw_circle_fill(panel_x + outer_padding + 9, query_y + query_h / 2, 1, local->surface_color, egui_color_alpha_mix(self->alpha, 96));
    egui_canvas_draw_line(
            panel_x + outer_padding + 11,
            query_y + query_h / 2 + 2,
            panel_x + outer_padding + 14,
            query_y + query_h / 2 + 5,
            1,
            status_color,
            egui_color_alpha_mix(self->alpha, 92));

    text_region.location.x = panel_x + outer_padding + (local->compact_mode ? 18 : 17);
    text_region.location.y = query_y;
    text_region.size.width = panel_w - outer_padding * 2 - (local->compact_mode ? 39 : 44);
    text_region.size.height = query_h;
    egui_canvas_draw_text_in_rect(local->font, snapshot->query, &text_region, EGUI_ALIGN_LEFT, local->text_color, self->alpha);

    egui_view_command_palette_draw_shortcut(
            self,
            local,
            panel_x + panel_w - outer_padding - (local->compact_mode ? 19 : 25),
            query_y + 2,
            local->compact_mode ? 14 : 20,
            query_h - 4,
            "GO",
            status_color);

    row_x = panel_x + outer_padding;
    row_y = query_y + query_h + (local->compact_mode ? 5 : 8);
    row_w = panel_w - outer_padding * 2;
    row_h = local->compact_mode ? 14 : 16;

    egui_view_command_palette_draw_row(self, local, row_x, row_y, row_w, row_h, snapshot->primary, "ENT", snapshot->highlight_index == 1, status_color);

    if (!local->compact_mode)
    {
        egui_view_command_palette_draw_row(
                self,
                local,
                row_x,
                row_y + row_h + 5,
                row_w,
                row_h,
                snapshot->secondary,
                "TAB",
                snapshot->highlight_index == 2,
                status_color);
        footer_y = row_y + row_h * 2 + 12;
    }
    else
    {
        footer_y = row_y + row_h + 4;
    }

    text_region.location.x = panel_x + outer_padding + (local->compact_mode ? 4 : 4);
    text_region.location.y = footer_y;
    text_region.size.width = panel_w - outer_padding * 2 - (local->compact_mode ? 8 : 8);
    text_region.size.height = EGUI_MAX(panel_h - (footer_y - panel_y) - 6, 10);
    egui_canvas_draw_text_in_rect(local->font, snapshot->hint, &text_region, EGUI_ALIGN_CENTER, local->muted_text_color, self->alpha);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_command_palette_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_command_palette_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_command_palette_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_command_palette_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_command_palette_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x111927);
    local->border_color = EGUI_COLOR_HEX(0x445770);
    local->track_color = EGUI_COLOR_HEX(0x172130);
    local->query_color = EGUI_COLOR_HEX(0x1D2A3C);
    local->text_color = EGUI_COLOR_HEX(0xECF3FF);
    local->muted_text_color = EGUI_COLOR_HEX(0x97A9C2);
    local->accent_color = EGUI_COLOR_HEX(0x67D6FF);
    local->warn_color = EGUI_COLOR_HEX(0xF1B05F);
    local->lock_color = EGUI_COLOR_HEX(0xC6A37A);
    local->focus_color = EGUI_COLOR_HEX(0x99EEFF);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->compact_mode = 0;
    local->locked_mode = 0;
}
