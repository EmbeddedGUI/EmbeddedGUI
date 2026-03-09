#include <stdlib.h>

#include "egui_view_window_snap_grid.h"

static uint8_t egui_view_window_snap_grid_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_WINDOW_SNAP_GRID_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_WINDOW_SNAP_GRID_MAX_SNAPSHOTS;
    }
    return count;
}

void egui_view_window_snap_grid_set_snapshots(egui_view_t *self, const egui_view_window_snap_grid_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_window_snap_grid_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_window_snap_grid_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_window_snap_grid_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_window_snap_grid_t);
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

uint8_t egui_view_window_snap_grid_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_window_snap_grid_t);
    return local->current_snapshot;
}

void egui_view_window_snap_grid_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_window_snap_grid_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_window_snap_grid_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_window_snap_grid_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_window_snap_grid_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_window_snap_grid_t);
    local->locked_mode = locked_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_window_snap_grid_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t panel_color,
        egui_color_t border_color,
        egui_color_t text_color,
        egui_color_t muted_text_color,
        egui_color_t accent_color,
        egui_color_t warn_color,
        egui_color_t lock_color,
        egui_color_t focus_color)
{
    EGUI_LOCAL_INIT(egui_view_window_snap_grid_t);
    local->surface_color = surface_color;
    local->panel_color = panel_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->warn_color = warn_color;
    local->lock_color = lock_color;
    local->focus_color = focus_color;
    egui_view_invalidate(self);
}

static egui_color_t egui_view_window_snap_grid_get_status_color(
        egui_view_window_snap_grid_t *local,
        const egui_view_window_snap_grid_snapshot_t *snapshot)
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

static void egui_view_window_snap_grid_draw_layout(
        egui_view_t *self,
        egui_view_window_snap_grid_t *local,
        const egui_view_window_snap_grid_snapshot_t *snapshot,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t w,
        egui_dim_t h)
{
    egui_color_t status_color;
    egui_dim_t zone_gap;
    egui_dim_t half_w;
    egui_dim_t half_h;
    egui_dim_t third_w;
    uint8_t i;

    status_color = egui_view_window_snap_grid_get_status_color(local, snapshot);
    zone_gap = local->compact_mode ? 3 : 5;
    half_w = (w - zone_gap) / 2;
    half_h = (h - zone_gap) / 2;
    third_w = (w - zone_gap * 2) / 3;

    egui_canvas_draw_round_rectangle_fill(x, y, w, h, 9, local->panel_color, egui_color_alpha_mix(self->alpha, 82));
    egui_canvas_draw_round_rectangle(x, y, w, h, 9, 1, local->border_color, egui_color_alpha_mix(self->alpha, 52));

    if (snapshot->layout_mode == 0)
    {
        egui_dim_t top_h;
        egui_dim_t bottom_h;
        top_h = (h - zone_gap) / 2;
        bottom_h = h - top_h - zone_gap;

        for (i = 0; i < 3; i++)
        {
            egui_dim_t zx;
            egui_dim_t zy;
            egui_dim_t zw;
            egui_dim_t zh;
            uint8_t highlighted;

            if (i == 0)
            {
                zx = x;
                zy = y;
                zw = w;
                zh = top_h;
            }
            else if (i == 1)
            {
                zx = x;
                zy = y + top_h + zone_gap;
                zw = half_w;
                zh = bottom_h;
            }
            else
            {
                zx = x + half_w + zone_gap;
                zy = y + top_h + zone_gap;
                zw = w - half_w - zone_gap;
                zh = bottom_h;
            }

            highlighted = (snapshot->highlight_mask & (1u << i)) ? 1 : 0;
            egui_canvas_draw_round_rectangle_fill(
                    zx,
                    zy,
                    zw,
                    zh,
                    6,
                    highlighted ? status_color : egui_rgb_mix(local->panel_color, local->border_color, 26),
                    egui_color_alpha_mix(self->alpha, highlighted ? 76 : 42));
            egui_canvas_draw_round_rectangle(
                    zx,
                    zy,
                    zw,
                    zh,
                    6,
                    1,
                    highlighted ? local->focus_color : local->border_color,
                    egui_color_alpha_mix(self->alpha, highlighted ? 72 : 34));
        }
    }
    else if (snapshot->layout_mode == 1)
    {
        for (i = 0; i < 3; i++)
        {
            egui_dim_t zx;
            uint8_t highlighted;

            zx = x + i * (third_w + zone_gap);
            highlighted = (snapshot->highlight_mask & (1u << i)) ? 1 : 0;
            egui_canvas_draw_round_rectangle_fill(
                    zx,
                    y,
                    (i == 2) ? (w - (third_w + zone_gap) * 2) : third_w,
                    h,
                    6,
                    highlighted ? status_color : egui_rgb_mix(local->panel_color, local->border_color, 26),
                    egui_color_alpha_mix(self->alpha, highlighted ? 76 : 42));
            egui_canvas_draw_round_rectangle(
                    zx,
                    y,
                    (i == 2) ? (w - (third_w + zone_gap) * 2) : third_w,
                    h,
                    6,
                    1,
                    highlighted ? local->focus_color : local->border_color,
                    egui_color_alpha_mix(self->alpha, highlighted ? 72 : 34));
        }
    }
    else
    {
        for (i = 0; i < 4; i++)
        {
            egui_dim_t zx;
            egui_dim_t zy;
            uint8_t highlighted;

            zx = x + ((i % 2) ? (half_w + zone_gap) : 0);
            zy = y + ((i / 2) ? (half_h + zone_gap) : 0);
            highlighted = (snapshot->highlight_mask & (1u << i)) ? 1 : 0;
            egui_canvas_draw_round_rectangle_fill(
                    zx,
                    zy,
                    (i % 2) ? (w - half_w - zone_gap) : half_w,
                    (i / 2) ? (h - half_h - zone_gap) : half_h,
                    6,
                    highlighted ? status_color : egui_rgb_mix(local->panel_color, local->border_color, 26),
                    egui_color_alpha_mix(self->alpha, highlighted ? 76 : 42));
            egui_canvas_draw_round_rectangle(
                    zx,
                    zy,
                    (i % 2) ? (w - half_w - zone_gap) : half_w,
                    (i / 2) ? (h - half_h - zone_gap) : half_h,
                    6,
                    1,
                    highlighted ? local->focus_color : local->border_color,
                    egui_color_alpha_mix(self->alpha, highlighted ? 72 : 34));
        }
    }

    if (snapshot->focus_slot < 4)
    {
        egui_dim_t badge_x;
        egui_dim_t badge_y;

        badge_x = x + (local->compact_mode ? 10 : 12);
        badge_y = y + h - (local->compact_mode ? 8 : 10);
        egui_canvas_draw_circle_fill(badge_x, badge_y, local->compact_mode ? 2 : 3, status_color, egui_color_alpha_mix(self->alpha, 90));
    }
}

static void egui_view_window_snap_grid_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_window_snap_grid_t);
    egui_region_t region;
    egui_region_t text_region;
    const egui_view_window_snap_grid_snapshot_t *snapshot;
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
    egui_dim_t summary_y;
    egui_dim_t preview_x;
    egui_dim_t preview_y;
    egui_dim_t preview_w;
    egui_dim_t preview_h;
    egui_dim_t footer_y;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    status_color = egui_view_window_snap_grid_get_status_color(local, snapshot);
    shell_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, 28);

    panel_x = region.location.x;
    panel_y = region.location.y;
    panel_w = region.size.width;
    panel_h = region.size.height;
    outer_padding = local->compact_mode ? 13 : 14;
    header_top = local->compact_mode ? 10 : 9;
    pill_w = local->compact_mode ? 37 : 45;

    egui_canvas_draw_round_rectangle_fill(panel_x, panel_y, panel_w, panel_h, 10, shell_color, egui_color_alpha_mix(self->alpha, 45));
    egui_canvas_draw_round_rectangle(panel_x, panel_y, panel_w, panel_h, 10, 1, local->border_color, egui_color_alpha_mix(self->alpha, 62));

    pill_x = panel_x + panel_w - outer_padding - pill_w;
    title_w = pill_x - panel_x - outer_padding - (local->compact_mode ? 12 : 15);

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
            egui_color_alpha_mix(self->alpha, local->locked_mode ? 30 : 62));
    text_region.location.x = pill_x + 1;
    text_region.location.y = panel_y + header_top;
    text_region.size.width = pill_w - 2;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(local->font, snapshot->status, &text_region, EGUI_ALIGN_CENTER, local->text_color, self->alpha);

    summary_y = panel_y + (local->compact_mode ? 24 : 29);
    text_region.location.x = panel_x + outer_padding + (local->compact_mode ? 2 : 5);
    text_region.location.y = summary_y;
    text_region.size.width = panel_w - outer_padding * 2 - (local->compact_mode ? 4 : 10);
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(local->font, snapshot->summary, &text_region, EGUI_ALIGN_CENTER, local->text_color, self->alpha);

    preview_x = panel_x + outer_padding;
    preview_y = panel_y + (local->compact_mode ? 39 : 49);
    preview_w = panel_w - outer_padding * 2;
    preview_h = local->compact_mode ? 23 : 37;
    egui_view_window_snap_grid_draw_layout(self, local, snapshot, preview_x, preview_y, preview_w, preview_h);

    footer_y = preview_y + preview_h + (local->compact_mode ? 1 : 6);
    text_region.location.x = panel_x + outer_padding + (local->compact_mode ? 5 : 6);
    text_region.location.y = footer_y;
    text_region.size.width = panel_w - outer_padding * 2 - (local->compact_mode ? 10 : 12);
    text_region.size.height = EGUI_MAX(panel_h - (footer_y - panel_y) - 6, 10);
    egui_canvas_draw_text_in_rect(local->font, snapshot->footer, &text_region, EGUI_ALIGN_CENTER, local->muted_text_color, self->alpha);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_window_snap_grid_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_window_snap_grid_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_window_snap_grid_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_window_snap_grid_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_window_snap_grid_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x101923);
    local->panel_color = EGUI_COLOR_HEX(0x1A2637);
    local->border_color = EGUI_COLOR_HEX(0x435873);
    local->text_color = EGUI_COLOR_HEX(0xEFF5FF);
    local->muted_text_color = EGUI_COLOR_HEX(0x9BACBF);
    local->accent_color = EGUI_COLOR_HEX(0x67D6FF);
    local->warn_color = EGUI_COLOR_HEX(0xF2B05E);
    local->lock_color = EGUI_COLOR_HEX(0xD6A36E);
    local->focus_color = EGUI_COLOR_HEX(0x95EEFF);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->compact_mode = 0;
    local->locked_mode = 0;
}
