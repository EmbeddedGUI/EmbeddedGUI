#include <stdlib.h>

#include "egui_view_clip_launcher_grid.h"

static const char *const clip_launcher_grid_scene_labels[EGUI_VIEW_CLIP_LAUNCHER_GRID_SCENE_COUNT] = {"A", "B", "C", "D"};
static const char *const clip_launcher_grid_track_labels[EGUI_VIEW_CLIP_LAUNCHER_GRID_TRACK_COUNT] = {"DRM", "BAS", "VOC", "FX"};
static const char *const clip_launcher_grid_track_compact_labels[EGUI_VIEW_CLIP_LAUNCHER_GRID_TRACK_COUNT] = {"D", "B", "V", "F"};

static uint8_t egui_view_clip_launcher_grid_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_CLIP_LAUNCHER_GRID_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_CLIP_LAUNCHER_GRID_MAX_SNAPSHOTS;
    }
    return count;
}

void egui_view_clip_launcher_grid_set_snapshots(egui_view_t *self, const egui_view_clip_launcher_grid_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_clip_launcher_grid_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_clip_launcher_grid_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_clip_launcher_grid_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_clip_launcher_grid_t);
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

uint8_t egui_view_clip_launcher_grid_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_clip_launcher_grid_t);
    return local->current_snapshot;
}

void egui_view_clip_launcher_grid_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_clip_launcher_grid_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_clip_launcher_grid_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_clip_launcher_grid_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_clip_launcher_grid_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_clip_launcher_grid_t);
    local->locked_mode = locked_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_clip_launcher_grid_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t panel_color, egui_color_t border_color,
                                              egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t queue_color,
                                              egui_color_t lock_color, egui_color_t play_color)
{
    EGUI_LOCAL_INIT(egui_view_clip_launcher_grid_t);
    local->surface_color = surface_color;
    local->panel_color = panel_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->queue_color = queue_color;
    local->lock_color = lock_color;
    local->play_color = play_color;
    egui_view_invalidate(self);
}

static egui_color_t egui_view_clip_launcher_grid_get_status_color(egui_view_clip_launcher_grid_t *local,
                                                                  const egui_view_clip_launcher_grid_snapshot_t *snapshot)
{
    if (local->locked_mode || snapshot->accent_mode >= 2)
    {
        return local->lock_color;
    }
    if (snapshot->accent_mode == 1)
    {
        return local->queue_color;
    }
    return local->accent_color;
}

static void egui_view_clip_launcher_grid_draw_text(egui_view_clip_launcher_grid_t *local, egui_view_t *self, const char *text, egui_dim_t x, egui_dim_t y,
                                                   egui_dim_t width, egui_dim_t height, uint8_t align, egui_color_t color)
{
    egui_region_t text_region;

    text_region.location.x = x;
    text_region.location.y = y;
    text_region.size.width = width;
    text_region.size.height = height;
    egui_canvas_draw_text_in_rect(local->font, text, &text_region, align, color, self->alpha);
}

static void egui_view_clip_launcher_grid_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_clip_launcher_grid_t);
    egui_region_t region;
    const egui_view_clip_launcher_grid_snapshot_t *snapshot;
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
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_w;
    egui_dim_t label_col_w;
    egui_dim_t scene_gap;
    egui_dim_t scene_w;
    egui_dim_t scene_h;
    egui_dim_t scene_origin_x;
    egui_dim_t scene_band_w;
    egui_dim_t grid_shell_y;
    egui_dim_t grid_shell_h;
    egui_dim_t row_gap;
    egui_dim_t cell_h;
    egui_dim_t track;
    egui_dim_t scene;
    uint8_t playing_scene;
    uint8_t queued_scene;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    status_color = egui_view_clip_launcher_grid_get_status_color(local, snapshot);
    shell_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, 30);

    panel_x = region.location.x;
    panel_y = region.location.y;
    panel_w = region.size.width;
    panel_h = region.size.height;
    outer_padding = local->compact_mode ? 10 : 11;
    header_top = local->compact_mode ? 7 : 8;
    pill_w = local->compact_mode ? 38 : 46;

    egui_canvas_draw_round_rectangle_fill(panel_x, panel_y, panel_w, panel_h, 10, shell_color, egui_color_alpha_mix(self->alpha, 46));
    egui_canvas_draw_round_rectangle(panel_x, panel_y, panel_w, panel_h, 10, 1, local->border_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));
    egui_canvas_draw_line(panel_x + 9, panel_y + 3, panel_x + 18, panel_y + 3, 1, local->border_color, egui_color_alpha_mix(self->alpha, 26));
    egui_canvas_draw_line(panel_x + panel_w - 19, panel_y + panel_h - 4, panel_x + panel_w - 10, panel_y + panel_h - 4, 1, local->border_color,
                          egui_color_alpha_mix(self->alpha, 22));

    pill_x = panel_x + panel_w - outer_padding - pill_w;
    title_w = pill_x - panel_x - outer_padding - 8;
    egui_view_clip_launcher_grid_draw_text(local, self, snapshot->title ? snapshot->title : "CLIPS", panel_x + outer_padding, panel_y + header_top, title_w, 11,
                                           EGUI_ALIGN_LEFT, local->muted_text_color);

    egui_canvas_draw_round_rectangle_fill(pill_x, panel_y + header_top, pill_w, 11, 5, status_color,
                                          egui_color_alpha_mix(self->alpha, local->locked_mode ? 32 : 70));
    egui_canvas_draw_line(pill_x + 5, panel_y + header_top + 2, pill_x + pill_w - 6, panel_y + header_top + 2, 1, local->text_color,
                          egui_color_alpha_mix(self->alpha, local->locked_mode ? 18 : 30));
    egui_canvas_draw_line(pill_x + pill_w - 14, panel_y + header_top + 9, pill_x + pill_w - 5, panel_y + header_top + 9, 1, status_color,
                          egui_color_alpha_mix(self->alpha, local->locked_mode ? 18 : 34));
    egui_view_clip_launcher_grid_draw_text(local, self, snapshot->status ? snapshot->status : "LIVE", pill_x + 1, panel_y + header_top, pill_w - 2, 11,
                                           EGUI_ALIGN_CENTER, local->text_color);

    content_x = panel_x + outer_padding;
    content_y = panel_y + (local->compact_mode ? 22 : 25);
    content_w = panel_w - outer_padding * 2;
    label_col_w = local->compact_mode ? 10 : 18;
    scene_gap = local->compact_mode ? 2 : 3;
    scene_h = local->compact_mode ? 10 : 11;
    scene_w = (content_w - label_col_w - scene_gap * 3 - 2) / EGUI_VIEW_CLIP_LAUNCHER_GRID_SCENE_COUNT;
    if (scene_w < 12)
    {
        return;
    }
    scene_origin_x = content_x + label_col_w + 2;
    scene_band_w = scene_w * EGUI_VIEW_CLIP_LAUNCHER_GRID_SCENE_COUNT + scene_gap * (EGUI_VIEW_CLIP_LAUNCHER_GRID_SCENE_COUNT - 1);

    egui_canvas_draw_line(content_x + 1, content_y - 3, content_x + content_w - 2, content_y - 3, 1, local->border_color,
                          egui_color_alpha_mix(self->alpha, 16));
    egui_canvas_draw_line(pill_x + 3, content_y - 3, panel_x + panel_w - outer_padding - 3, content_y - 3, 1, status_color,
                          egui_color_alpha_mix(self->alpha, local->locked_mode ? 26 : 34));

    playing_scene = snapshot->playing_scene;
    queued_scene = snapshot->queued_scene;
    if (playing_scene >= EGUI_VIEW_CLIP_LAUNCHER_GRID_SCENE_COUNT)
    {
        playing_scene = 0;
    }
    if (queued_scene >= EGUI_VIEW_CLIP_LAUNCHER_GRID_SCENE_COUNT)
    {
        queued_scene = playing_scene;
    }

    egui_canvas_draw_round_rectangle_fill(scene_origin_x - 2, content_y - 2, scene_band_w + 4, scene_h + 5, 5,
                                          egui_rgb_mix(local->panel_color, status_color, local->compact_mode ? 10 : 12),
                                          egui_color_alpha_mix(self->alpha, local->compact_mode ? 18 : 22));
    egui_canvas_draw_round_rectangle(scene_origin_x - 2, content_y - 2, scene_band_w + 4, scene_h + 5, 5, 1, local->border_color,
                                     egui_color_alpha_mix(self->alpha, 18));
    egui_canvas_draw_line(scene_origin_x + 4, content_y - 1, scene_origin_x + scene_band_w - 5, content_y - 1, 1, status_color,
                          egui_color_alpha_mix(self->alpha, local->compact_mode ? 12 : 16));
    egui_canvas_draw_line(scene_origin_x + 4, content_y + scene_h + 1, scene_origin_x + scene_band_w - 5, content_y + scene_h + 1, 1, local->border_color,
                          egui_color_alpha_mix(self->alpha, local->compact_mode ? 10 : 14));
    egui_canvas_draw_round_rectangle_fill(scene_origin_x - 1, content_y + 3, 2, scene_h - 1, 1, status_color,
                                          egui_color_alpha_mix(self->alpha, local->compact_mode ? 10 : 14));
    egui_canvas_draw_round_rectangle_fill(scene_origin_x + scene_band_w - 1, content_y + 3, 2, scene_h - 1, 1, status_color,
                                          egui_color_alpha_mix(self->alpha, local->compact_mode ? 10 : 14));

    for (scene = 0; scene < EGUI_VIEW_CLIP_LAUNCHER_GRID_SCENE_COUNT; scene++)
    {
        egui_dim_t scene_x;
        egui_color_t chip_fill;
        egui_color_t chip_border;
        egui_color_t chip_text;
        egui_alpha_t chip_alpha;
        uint8_t is_playing_scene;
        uint8_t is_queued_scene;

        scene_x = scene_origin_x + scene * (scene_w + scene_gap);
        is_playing_scene = scene == playing_scene;
        is_queued_scene = scene == queued_scene;
        chip_fill = egui_rgb_mix(local->panel_color, local->border_color, 18);
        chip_border = egui_rgb_mix(local->panel_color, local->border_color, 28);
        chip_text = local->muted_text_color;
        chip_alpha = 44;

        if (is_playing_scene)
        {
            chip_fill = local->play_color;
            chip_border = local->play_color;
            chip_text = local->text_color;
            chip_alpha = 82;
        }
        else if (is_queued_scene)
        {
            chip_fill = egui_rgb_mix(local->panel_color, local->queue_color, 10);
            chip_border = local->queue_color;
            chip_text = local->text_color;
            chip_alpha = local->locked_mode ? 20 : 30;
        }

        egui_canvas_draw_round_rectangle_fill(scene_x, content_y, scene_w, scene_h, 4, chip_fill, egui_color_alpha_mix(self->alpha, chip_alpha));
        egui_canvas_draw_round_rectangle(scene_x, content_y, scene_w, scene_h, 4, 1, chip_border, egui_color_alpha_mix(self->alpha, 70));
        egui_view_clip_launcher_grid_draw_text(local, self, clip_launcher_grid_scene_labels[scene], scene_x, content_y, scene_w, scene_h, EGUI_ALIGN_CENTER,
                                               chip_text);

        if (is_playing_scene)
        {
            egui_canvas_draw_round_rectangle_fill(scene_x + 3, content_y + scene_h - 3, scene_w - 6, 2, 1, local->text_color,
                                                  egui_color_alpha_mix(self->alpha, 62));
            egui_canvas_draw_round_rectangle_fill(scene_x + scene_w / 2 - 1, content_y + scene_h - 1, 2, 3, 1, local->play_color,
                                                  egui_color_alpha_mix(self->alpha, local->locked_mode ? 20 : 54));
            egui_canvas_draw_line(scene_x + 1, content_y + scene_h / 2, scene_x + 2, content_y + scene_h / 2, 1, local->text_color,
                                  egui_color_alpha_mix(self->alpha, 44));
            egui_canvas_draw_line(scene_x + scene_w - 3, content_y + scene_h / 2, scene_x + scene_w - 2, content_y + scene_h / 2, 1, local->text_color,
                                  egui_color_alpha_mix(self->alpha, 44));
        }
        if (is_queued_scene)
        {
            egui_canvas_draw_round_rectangle_fill(scene_x + 3, content_y + 2, scene_w - 6, 2, 1, local->queue_color,
                                                  egui_color_alpha_mix(self->alpha, local->locked_mode ? 36 : 82));
            egui_canvas_draw_line(scene_x + scene_w / 2 - 2, content_y + scene_h, scene_x + scene_w / 2 - 2, content_y + scene_h + 2, 1, local->queue_color,
                                  egui_color_alpha_mix(self->alpha, local->locked_mode ? 18 : 40));
            egui_canvas_draw_line(scene_x + scene_w / 2 + 2, content_y + scene_h, scene_x + scene_w / 2 + 2, content_y + scene_h + 2, 1, local->queue_color,
                                  egui_color_alpha_mix(self->alpha, local->locked_mode ? 18 : 40));
            egui_canvas_draw_circle_fill(scene_x + 3, content_y + scene_h - 1, 1, local->queue_color, egui_color_alpha_mix(self->alpha, 46));
            egui_canvas_draw_circle_fill(scene_x + scene_w - 4, content_y + scene_h - 1, 1, local->queue_color, egui_color_alpha_mix(self->alpha, 46));
        }
    }

    row_gap = local->compact_mode ? 4 : 5;
    cell_h = local->compact_mode ? 10 : 12;
    grid_shell_y = content_y + scene_h + 4;
    grid_shell_h = cell_h * EGUI_VIEW_CLIP_LAUNCHER_GRID_TRACK_COUNT + row_gap * (EGUI_VIEW_CLIP_LAUNCHER_GRID_TRACK_COUNT - 1) + 8;

    egui_canvas_draw_round_rectangle_fill(content_x, grid_shell_y, content_w, grid_shell_h, 8, local->panel_color, egui_color_alpha_mix(self->alpha, 70));
    egui_canvas_draw_round_rectangle(content_x, grid_shell_y, content_w, grid_shell_h, 8, 1, local->border_color, egui_color_alpha_mix(self->alpha, 36));
    egui_canvas_draw_line(content_x + 6, grid_shell_y + 3, content_x + content_w - 7, grid_shell_y + 3, 1, local->border_color,
                          egui_color_alpha_mix(self->alpha, local->locked_mode ? 14 : 20));
    egui_canvas_draw_line(content_x + 8, grid_shell_y + grid_shell_h - 4, content_x + content_w - 9, grid_shell_y + grid_shell_h - 4, 1, local->border_color,
                          egui_color_alpha_mix(self->alpha, local->locked_mode ? 12 : 16));

    if (!local->compact_mode)
    {
        egui_dim_t bridge_y;
        egui_dim_t bridge_h;
        egui_dim_t bridge_center_x;

        bridge_y = content_y + scene_h + 1;
        bridge_h = grid_shell_y - bridge_y;
        if (bridge_h > 0)
        {
            bridge_center_x = scene_origin_x + playing_scene * (scene_w + scene_gap) + scene_w / 2;
            egui_canvas_draw_round_rectangle_fill(bridge_center_x - 1, bridge_y, 3, bridge_h + 1, 1, local->play_color,
                                                  egui_color_alpha_mix(self->alpha, local->locked_mode ? 18 : 42));
            egui_canvas_draw_circle_fill(bridge_center_x, grid_shell_y, 2, local->play_color, egui_color_alpha_mix(self->alpha, 42));
            egui_canvas_draw_line(bridge_center_x - scene_w / 2 + 5, grid_shell_y + 1, bridge_center_x + scene_w / 2 - 5, grid_shell_y + 1, 1,
                                  local->play_color, egui_color_alpha_mix(self->alpha, local->locked_mode ? 14 : 24));

            bridge_center_x = scene_origin_x + queued_scene * (scene_w + scene_gap) + scene_w / 2;
            egui_canvas_draw_line(bridge_center_x - 2, bridge_y, bridge_center_x - 2, grid_shell_y, 1, local->queue_color,
                                  egui_color_alpha_mix(self->alpha, local->locked_mode ? 18 : 38));
            egui_canvas_draw_line(bridge_center_x + 2, bridge_y, bridge_center_x + 2, grid_shell_y, 1, local->queue_color,
                                  egui_color_alpha_mix(self->alpha, local->locked_mode ? 18 : 38));
            egui_canvas_draw_round_rectangle_fill(bridge_center_x - 3, grid_shell_y - 1, 7, 2, 1, local->queue_color,
                                                  egui_color_alpha_mix(self->alpha, local->locked_mode ? 24 : 42));
            egui_canvas_draw_line(bridge_center_x - scene_w / 2 + 6, grid_shell_y + 1, bridge_center_x + scene_w / 2 - 6, grid_shell_y + 1, 1,
                                  local->queue_color, egui_color_alpha_mix(self->alpha, local->locked_mode ? 14 : 22));
        }
    }

    if (local->compact_mode)
    {
        egui_canvas_draw_round_rectangle_fill(content_x + 1, grid_shell_y + 2, label_col_w - 1, grid_shell_h - 4, 4,
                                              egui_rgb_mix(local->panel_color, local->border_color, 10), egui_color_alpha_mix(self->alpha, 16));
        egui_canvas_draw_line(content_x + 3, grid_shell_y + 4, content_x + label_col_w - 2, grid_shell_y + 4, 1, local->border_color,
                              egui_color_alpha_mix(self->alpha, 18));
        egui_canvas_draw_line(scene_origin_x - 2, grid_shell_y + 5, scene_origin_x - 2, grid_shell_y + grid_shell_h - 6, 1, local->border_color,
                              egui_color_alpha_mix(self->alpha, 18));
    }
    else
    {
        egui_canvas_draw_line(scene_origin_x - 8, grid_shell_y + 5, scene_origin_x - 8, grid_shell_y + grid_shell_h - 6, 1, local->border_color,
                              egui_color_alpha_mix(self->alpha, 14));
    }

    if (local->locked_mode)
    {
        egui_canvas_draw_round_rectangle(content_x + 3, grid_shell_y + 3, content_w - 6, grid_shell_h - 6, 6, 1, local->lock_color,
                                         egui_color_alpha_mix(self->alpha, 18));
        egui_canvas_draw_round_rectangle_fill(content_x + 5, grid_shell_y + 5, content_w - 10, grid_shell_h - 10, 5, EGUI_COLOR_BLACK,
                                              egui_color_alpha_mix(self->alpha, 12));
        egui_canvas_draw_line(content_x + 8, grid_shell_y + 2, content_x + 18, grid_shell_y + 2, 1, local->lock_color, egui_color_alpha_mix(self->alpha, 34));
        egui_canvas_draw_line(content_x + 8, grid_shell_y + 2, content_x + 8, grid_shell_y + 8, 1, local->lock_color, egui_color_alpha_mix(self->alpha, 24));
        egui_canvas_draw_line(content_x + content_w - 19, grid_shell_y + 2, content_x + content_w - 9, grid_shell_y + 2, 1, local->lock_color,
                              egui_color_alpha_mix(self->alpha, 34));
        egui_canvas_draw_line(content_x + content_w - 9, grid_shell_y + 2, content_x + content_w - 9, grid_shell_y + 8, 1, local->lock_color,
                              egui_color_alpha_mix(self->alpha, 24));
        egui_canvas_draw_line(content_x + 8, grid_shell_y + grid_shell_h - 3, content_x + 18, grid_shell_y + grid_shell_h - 3, 1, local->lock_color,
                              egui_color_alpha_mix(self->alpha, 28));
        egui_canvas_draw_line(content_x + 8, grid_shell_y + grid_shell_h - 9, content_x + 8, grid_shell_y + grid_shell_h - 3, 1, local->lock_color,
                              egui_color_alpha_mix(self->alpha, 20));
        egui_canvas_draw_line(content_x + content_w - 19, grid_shell_y + grid_shell_h - 3, content_x + content_w - 9, grid_shell_y + grid_shell_h - 3, 1,
                              local->lock_color, egui_color_alpha_mix(self->alpha, 28));
        egui_canvas_draw_line(content_x + content_w - 9, grid_shell_y + grid_shell_h - 9, content_x + content_w - 9, grid_shell_y + grid_shell_h - 3, 1,
                              local->lock_color, egui_color_alpha_mix(self->alpha, 20));
        egui_canvas_draw_line(content_x + 11, grid_shell_y + grid_shell_h / 2, content_x + content_w - 12, grid_shell_y + grid_shell_h / 2, 1,
                              local->lock_color, egui_color_alpha_mix(self->alpha, 24));
        egui_canvas_draw_circle_fill(content_x + 11, grid_shell_y + grid_shell_h / 2, 2, local->lock_color, egui_color_alpha_mix(self->alpha, 30));
        egui_canvas_draw_circle_fill(content_x + content_w / 2, grid_shell_y + grid_shell_h / 2, 2, local->lock_color, egui_color_alpha_mix(self->alpha, 26));
        egui_canvas_draw_line(content_x + content_w / 2, grid_shell_y + grid_shell_h / 2 - 4, content_x + content_w / 2, grid_shell_y + grid_shell_h / 2 + 4, 1,
                              local->lock_color, egui_color_alpha_mix(self->alpha, 18));
        egui_canvas_draw_circle_fill(content_x + content_w - 12, grid_shell_y + grid_shell_h / 2, 2, local->lock_color, egui_color_alpha_mix(self->alpha, 30));
    }

    {
        egui_dim_t highlight_x;
        highlight_x = scene_origin_x + playing_scene * (scene_w + scene_gap) - 2;
        egui_canvas_draw_round_rectangle_fill(highlight_x, grid_shell_y + 2, scene_w + 4, grid_shell_h - 4, 5, local->play_color,
                                              egui_color_alpha_mix(self->alpha, local->locked_mode ? 12 : 18));
        egui_canvas_draw_round_rectangle_fill(highlight_x + 3, grid_shell_y + 3, scene_w - 2, 2, 1, local->play_color, egui_color_alpha_mix(self->alpha, 44));
        egui_canvas_draw_round_rectangle_fill(highlight_x + scene_w / 2, grid_shell_y + 8, 2, grid_shell_h - 16, 1, local->play_color,
                                              egui_color_alpha_mix(self->alpha, local->locked_mode ? 16 : 34));
        egui_canvas_draw_circle_fill(highlight_x + scene_w / 2 + 1, grid_shell_y + grid_shell_h - 5, 2, local->play_color,
                                     egui_color_alpha_mix(self->alpha, local->locked_mode ? 16 : 34));

        highlight_x = scene_origin_x + queued_scene * (scene_w + scene_gap) - 1;
        egui_canvas_draw_round_rectangle(highlight_x, grid_shell_y + 3, scene_w + 2, grid_shell_h - 6, 5, 1, local->queue_color,
                                         egui_color_alpha_mix(self->alpha, local->locked_mode ? 20 : 35));
        egui_canvas_draw_round_rectangle_fill(highlight_x + 2, grid_shell_y + 4, scene_w - 2, 2, 1, local->queue_color, egui_color_alpha_mix(self->alpha, 52));
        egui_canvas_draw_round_rectangle_fill(highlight_x + 2, grid_shell_y + grid_shell_h - 6, scene_w - 2, 2, 1, local->queue_color,
                                              egui_color_alpha_mix(self->alpha, local->locked_mode ? 28 : 52));
        egui_canvas_draw_line(highlight_x + 3, grid_shell_y + 9, highlight_x + 3, grid_shell_y + grid_shell_h - 10, 1, local->queue_color,
                              egui_color_alpha_mix(self->alpha, local->locked_mode ? 16 : 24));
        egui_canvas_draw_line(highlight_x + scene_w, grid_shell_y + 9, highlight_x + scene_w, grid_shell_y + grid_shell_h - 10, 1, local->queue_color,
                              egui_color_alpha_mix(self->alpha, local->locked_mode ? 16 : 24));
        egui_canvas_draw_circle_fill(highlight_x + 3, grid_shell_y + grid_shell_h / 2, 1, local->queue_color,
                                     egui_color_alpha_mix(self->alpha, local->locked_mode ? 18 : 38));
        egui_canvas_draw_circle_fill(highlight_x + scene_w, grid_shell_y + grid_shell_h / 2, 1, local->queue_color,
                                     egui_color_alpha_mix(self->alpha, local->locked_mode ? 18 : 38));
    }

    for (track = 1; track < EGUI_VIEW_CLIP_LAUNCHER_GRID_TRACK_COUNT; track++)
    {
        egui_dim_t separator_y;

        separator_y = grid_shell_y + 4 + track * (cell_h + row_gap) - row_gap / 2;
        egui_canvas_draw_line(scene_origin_x - (local->compact_mode ? 0 : 1), separator_y, content_x + content_w - 4, separator_y, 1,
                              local->locked_mode ? local->lock_color : local->border_color, egui_color_alpha_mix(self->alpha, local->locked_mode ? 14 : 18));
    }

    for (track = 0; track < EGUI_VIEW_CLIP_LAUNCHER_GRID_TRACK_COUNT; track++)
    {
        egui_dim_t row_y;
        egui_dim_t anchor_x;
        egui_color_t row_state_color;
        egui_alpha_t row_fill_alpha;
        egui_alpha_t row_border_alpha;
        uint8_t track_mask;
        uint8_t is_focus_track;
        uint8_t is_armed_track;

        row_y = grid_shell_y + 4 + track * (cell_h + row_gap);
        track_mask = snapshot->track_scene_masks[track];
        is_focus_track = track == snapshot->focus_track;
        is_armed_track = track == snapshot->armed_track;
        row_state_color = local->border_color;
        row_fill_alpha = local->compact_mode ? 8 : 0;
        row_border_alpha = local->compact_mode ? 12 : 0;

        if (is_armed_track)
        {
            row_state_color = local->queue_color;
            row_fill_alpha = local->locked_mode ? 12 : 20;
            row_border_alpha = local->locked_mode ? 24 : 46;
        }
        else if (is_focus_track)
        {
            row_state_color = local->accent_color;
            row_fill_alpha = local->locked_mode ? 10 : 16;
            row_border_alpha = local->locked_mode ? 18 : 36;
        }

        if (local->compact_mode || is_focus_track || is_armed_track)
        {
            egui_canvas_draw_round_rectangle_fill(content_x + 1, row_y - 1, content_w - 2, cell_h + 2, 4,
                                                  egui_rgb_mix(local->panel_color, row_state_color, local->compact_mode ? 14 : 18),
                                                  egui_color_alpha_mix(self->alpha, row_fill_alpha));
            egui_canvas_draw_round_rectangle(content_x + 1, row_y - 1, content_w - 2, cell_h + 2, 4, 1, row_state_color,
                                             egui_color_alpha_mix(self->alpha, row_border_alpha));
        }

        if (local->compact_mode)
        {
            egui_color_t label_fill;
            egui_color_t label_text;

            label_fill = egui_rgb_mix(local->panel_color, row_state_color, is_armed_track ? 26 : (is_focus_track ? 20 : 10));
            label_text = (is_focus_track || is_armed_track) ? local->text_color : local->muted_text_color;

            egui_canvas_draw_round_rectangle_fill(content_x + 1, row_y, label_col_w - 3, cell_h, 3, label_fill,
                                                  egui_color_alpha_mix(self->alpha, is_focus_track || is_armed_track ? 54 : 22));
            egui_canvas_draw_round_rectangle(content_x + 1, row_y, label_col_w - 3, cell_h, 3, 1, row_state_color,
                                             egui_color_alpha_mix(self->alpha, is_focus_track || is_armed_track ? 56 : 20));
            if (is_focus_track || is_armed_track)
            {
                egui_canvas_draw_line(content_x + 3, row_y + cell_h - 2, content_x + label_col_w - 5, row_y + cell_h - 2, 1, row_state_color,
                                      egui_color_alpha_mix(self->alpha, is_armed_track ? 54 : 42));
            }
            egui_view_clip_launcher_grid_draw_text(local, self, clip_launcher_grid_track_compact_labels[track], content_x + 1, row_y, label_col_w - 3, cell_h,
                                                   EGUI_ALIGN_CENTER, label_text);
        }
        else
        {
            egui_color_t label_fill;
            egui_color_t label_text;

            label_fill = egui_rgb_mix(local->panel_color, local->border_color, 14);
            label_text = local->muted_text_color;
            if (is_armed_track)
            {
                label_fill = local->queue_color;
                label_text = local->text_color;
            }
            else if (is_focus_track)
            {
                label_fill = local->accent_color;
                label_text = local->text_color;
            }

            egui_canvas_draw_round_rectangle_fill(content_x + 2, row_y, label_col_w - 4, cell_h, 4, label_fill,
                                                  egui_color_alpha_mix(self->alpha, is_focus_track || is_armed_track ? 58 : 24));
            egui_canvas_draw_round_rectangle(content_x + 2, row_y, label_col_w - 4, cell_h, 4, 1, label_fill, egui_color_alpha_mix(self->alpha, 60));
            egui_view_clip_launcher_grid_draw_text(local, self, clip_launcher_grid_track_labels[track], content_x + 2, row_y, label_col_w - 4, cell_h,
                                                   EGUI_ALIGN_CENTER, label_text);
        }

        if (local->compact_mode || is_focus_track || is_armed_track)
        {
            egui_canvas_draw_round_rectangle_fill(scene_origin_x - 2, row_y + 1, 2, cell_h - 2, 1, row_state_color,
                                                  egui_color_alpha_mix(self->alpha, is_armed_track ? 88 : (is_focus_track ? 70 : 18)));
        }

        anchor_x = local->compact_mode ? (scene_origin_x - 4) : (scene_origin_x - 5);
        egui_canvas_draw_circle_fill(anchor_x, row_y + cell_h / 2, local->compact_mode ? 1 : 2, local->locked_mode ? local->lock_color : row_state_color,
                                     egui_color_alpha_mix(self->alpha, is_armed_track ? 72 : (is_focus_track ? 56 : (local->locked_mode ? 22 : 18))));
        if (!local->compact_mode)
        {
            egui_canvas_draw_line(scene_origin_x - 8, row_y + cell_h / 2, scene_origin_x - 2, row_y + cell_h / 2, 1, row_state_color,
                                  egui_color_alpha_mix(self->alpha, is_armed_track ? 34 : (is_focus_track ? 26 : 14)));
        }
        if (local->locked_mode)
        {
            egui_canvas_draw_line(scene_origin_x + 2, row_y + cell_h / 2, content_x + content_w - 8, row_y + cell_h / 2, 1, local->lock_color,
                                  egui_color_alpha_mix(self->alpha, is_armed_track ? 18 : (is_focus_track ? 14 : 10)));
        }

        if (local->compact_mode && (is_focus_track || is_armed_track))
        {
            egui_canvas_draw_round_rectangle_fill(content_x + content_w - 6, row_y + cell_h / 2 - 1, 3, 2, 1, row_state_color,
                                                  egui_color_alpha_mix(self->alpha, is_armed_track ? 64 : 48));
            egui_canvas_draw_circle_fill(content_x + content_w - 9, row_y + cell_h / 2, 1, row_state_color,
                                         egui_color_alpha_mix(self->alpha, is_armed_track ? 52 : 38));
            egui_canvas_draw_line(scene_origin_x - 1, row_y + cell_h / 2, scene_origin_x + 4, row_y + cell_h / 2, 1, row_state_color,
                                  egui_color_alpha_mix(self->alpha, is_armed_track ? 32 : 24));
        }
        else if (!local->compact_mode && (is_focus_track || is_armed_track))
        {
            egui_canvas_draw_round_rectangle_fill(content_x + content_w - 7, row_y + cell_h / 2 - 1, 4, 2, 1, row_state_color,
                                                  egui_color_alpha_mix(self->alpha, is_armed_track ? 52 : 38));
        }

        for (scene = 0; scene < EGUI_VIEW_CLIP_LAUNCHER_GRID_SCENE_COUNT; scene++)
        {
            egui_dim_t cell_x;
            egui_color_t cell_fill;
            egui_color_t cell_border;
            egui_color_t inner_fill;
            egui_alpha_t cell_alpha;
            uint8_t has_clip;
            uint8_t is_playing;
            uint8_t is_queued;

            cell_x = scene_origin_x + scene * (scene_w + scene_gap);
            has_clip = (track_mask >> scene) & 0x01;
            is_playing = has_clip && scene == playing_scene;
            is_queued = scene == queued_scene;
            cell_fill = egui_rgb_mix(local->panel_color, local->border_color, has_clip ? 24 : 10);
            cell_border = has_clip ? local->border_color : egui_rgb_mix(local->panel_color, local->border_color, 12);
            cell_alpha = has_clip ? 42 : 14;
            inner_fill = egui_rgb_mix(local->accent_color, local->panel_color, 36);

            if (is_playing)
            {
                cell_fill = local->play_color;
                cell_border = local->play_color;
                inner_fill = local->text_color;
                cell_alpha = local->locked_mode ? 30 : 76;
            }
            else if (is_queued && has_clip)
            {
                cell_fill = egui_rgb_mix(local->panel_color, local->queue_color, 22);
                cell_border = local->queue_color;
                inner_fill = local->queue_color;
                cell_alpha = local->locked_mode ? 18 : 28;
            }
            else if (is_focus_track && has_clip)
            {
                cell_fill = local->accent_color;
                cell_border = local->accent_color;
                inner_fill = egui_rgb_mix(local->accent_color, local->text_color, 18);
                cell_alpha = local->locked_mode ? 22 : 52;
            }

            egui_canvas_draw_round_rectangle_fill(cell_x, row_y, scene_w, cell_h, 3, cell_fill, egui_color_alpha_mix(self->alpha, cell_alpha));
            egui_canvas_draw_round_rectangle(cell_x, row_y, scene_w, cell_h, 3, 1, cell_border,
                                             egui_color_alpha_mix(self->alpha, has_clip || is_queued ? 58 : 20));

            if (has_clip)
            {
                if (is_playing)
                {
                    egui_canvas_draw_round_rectangle_fill(cell_x + 3, row_y + 2, scene_w - 6, cell_h - 4, 2, inner_fill, egui_color_alpha_mix(self->alpha, 90));
                    egui_canvas_draw_round_rectangle_fill(cell_x + 4, row_y + cell_h - 3, scene_w - 8, 2, 1, local->play_color,
                                                          egui_color_alpha_mix(self->alpha, 72));
                    egui_canvas_draw_line(cell_x + scene_w / 2, row_y + 2, cell_x + scene_w / 2, row_y + cell_h - 3, 1, local->play_color,
                                          egui_color_alpha_mix(self->alpha, local->locked_mode ? 26 : 40));
                }
                else if (is_queued)
                {
                    egui_canvas_draw_round_rectangle_fill(cell_x + 3, row_y + 2, scene_w - 6, 3, 1, inner_fill,
                                                          egui_color_alpha_mix(self->alpha, local->locked_mode ? 42 : 88));
                    egui_canvas_draw_round_rectangle_fill(cell_x + scene_w / 2 - 3, row_y + cell_h - 4, 6, 2, 1, inner_fill,
                                                          egui_color_alpha_mix(self->alpha, 60));
                    egui_canvas_draw_circle_fill(cell_x + 4, row_y + cell_h - 3, 1, inner_fill, egui_color_alpha_mix(self->alpha, 36));
                    egui_canvas_draw_circle_fill(cell_x + scene_w - 5, row_y + cell_h - 3, 1, inner_fill, egui_color_alpha_mix(self->alpha, 36));
                }
                else
                {
                    egui_canvas_draw_round_rectangle_fill(cell_x + 4, row_y + 3, scene_w - 8, cell_h - 6, 2, inner_fill,
                                                          egui_color_alpha_mix(self->alpha, is_focus_track ? 74 : 68));
                    if (is_focus_track)
                    {
                        egui_canvas_draw_round_rectangle_fill(cell_x + 4, row_y + 2, scene_w - 8, 2, 1, local->accent_color,
                                                              egui_color_alpha_mix(self->alpha, local->locked_mode ? 24 : 48));
                    }
                }

                if (is_armed_track)
                {
                    egui_canvas_draw_line(cell_x + 2, row_y + cell_h - 2, cell_x + scene_w - 3, row_y + cell_h - 2, 1, local->queue_color,
                                          egui_color_alpha_mix(self->alpha, 80));
                }
            }
            else if (is_queued)
            {
                egui_canvas_draw_line(cell_x + 4, row_y + cell_h / 2, cell_x + scene_w - 5, row_y + cell_h / 2, 1, local->queue_color,
                                      egui_color_alpha_mix(self->alpha, 45));
                egui_canvas_draw_circle_fill(cell_x + 4, row_y + cell_h / 2, 1, local->queue_color, egui_color_alpha_mix(self->alpha, 34));
                egui_canvas_draw_circle_fill(cell_x + scene_w - 5, row_y + cell_h / 2, 1, local->queue_color, egui_color_alpha_mix(self->alpha, 34));
            }
            else
            {
                egui_canvas_draw_circle_fill(cell_x + scene_w / 2, row_y + cell_h / 2, 1, local->border_color,
                                             egui_color_alpha_mix(self->alpha, local->compact_mode ? 10 : 12));
            }
        }
    }

    {
        egui_dim_t footer_y;
        egui_dim_t footer_h;
        egui_color_t footer_left_color;
        egui_color_t footer_right_color;

        footer_y = grid_shell_y + grid_shell_h + (local->compact_mode ? 4 : 5);
        footer_h = panel_h - (footer_y - panel_y) - (local->compact_mode ? 6 : 7);
        if (footer_h < 10)
        {
            return;
        }

        footer_left_color = (snapshot->armed_track == snapshot->focus_track) ? local->queue_color : local->accent_color;
        if (local->locked_mode)
        {
            footer_left_color = local->lock_color;
        }
        footer_right_color = local->locked_mode ? local->lock_color : (queued_scene == playing_scene ? local->play_color : local->queue_color);

        if (local->compact_mode)
        {
            egui_canvas_draw_round_rectangle_fill(content_x + 12, footer_y, content_w - 24, footer_h, 5, egui_rgb_mix(local->panel_color, status_color, 16),
                                                  egui_color_alpha_mix(self->alpha, 54));
            egui_canvas_draw_round_rectangle(content_x + 12, footer_y, content_w - 24, footer_h, 5, 1, status_color, egui_color_alpha_mix(self->alpha, 44));
            egui_canvas_draw_round_rectangle_fill(content_x + 18, footer_y + 2, content_w - 36, 2, 1, status_color, egui_color_alpha_mix(self->alpha, 52));
            egui_canvas_draw_line(content_x + content_w / 2 - 4, footer_y + footer_h - 3, content_x + content_w / 2 + 3, footer_y + footer_h - 3, 1,
                                  status_color, egui_color_alpha_mix(self->alpha, 40));
            egui_canvas_draw_circle_fill(content_x + 18, footer_y + footer_h / 2, 2, status_color, egui_color_alpha_mix(self->alpha, 56));
            egui_canvas_draw_circle_fill(content_x + content_w / 2, footer_y + footer_h / 2, 1, status_color, egui_color_alpha_mix(self->alpha, 34));
            egui_canvas_draw_circle_fill(content_x + content_w - 18, footer_y + footer_h / 2, 2, status_color, egui_color_alpha_mix(self->alpha, 34));
            if (local->locked_mode)
            {
                egui_canvas_draw_circle_fill(content_x + 24, footer_y + footer_h - 3, 1, status_color, egui_color_alpha_mix(self->alpha, 30));
                egui_canvas_draw_circle_fill(content_x + content_w - 24, footer_y + footer_h - 3, 1, status_color, egui_color_alpha_mix(self->alpha, 30));
            }
            egui_view_clip_launcher_grid_draw_text(local, self, snapshot->footer_left ? snapshot->footer_left : "Queued", content_x + 12, footer_y,
                                                   content_w - 24, footer_h, EGUI_ALIGN_CENTER,
                                                   local->locked_mode ? local->muted_text_color : local->text_color);
            return;
        }

        {
            egui_dim_t pill_gap;
            egui_dim_t left_w;
            egui_dim_t right_w;

            pill_gap = 8;
            left_w = (content_w - pill_gap) / 2;
            right_w = content_w - left_w - pill_gap;

            egui_canvas_draw_round_rectangle_fill(content_x + 2, footer_y - 1, content_w - 4, footer_h + 2, 5,
                                                  egui_rgb_mix(local->panel_color, local->border_color, 8), egui_color_alpha_mix(self->alpha, 14));
            egui_canvas_draw_circle_fill(content_x + left_w + pill_gap / 2, footer_y + footer_h / 2, 1, local->border_color,
                                         egui_color_alpha_mix(self->alpha, 24));
            egui_canvas_draw_line(content_x + left_w + pill_gap / 2, footer_y + 2, content_x + left_w + pill_gap / 2, footer_y + footer_h - 3, 1,
                                  local->border_color, egui_color_alpha_mix(self->alpha, 14));

            egui_canvas_draw_round_rectangle_fill(content_x, footer_y, left_w, footer_h, 5, egui_rgb_mix(local->panel_color, footer_left_color, 16),
                                                  egui_color_alpha_mix(self->alpha, 54));
            egui_canvas_draw_round_rectangle(content_x, footer_y, left_w, footer_h, 5, 1, footer_left_color, egui_color_alpha_mix(self->alpha, 42));
            egui_canvas_draw_round_rectangle_fill(content_x + 8, footer_y + 2, left_w - 16, 2, 1, footer_left_color, egui_color_alpha_mix(self->alpha, 48));
            egui_canvas_draw_line(content_x + left_w / 2 - 5, footer_y + 2, content_x + left_w / 2 + 4, footer_y + 2, 1, footer_left_color,
                                  egui_color_alpha_mix(self->alpha, 30));
            egui_canvas_draw_circle_fill(content_x + 10, footer_y + footer_h / 2, 2, footer_left_color, egui_color_alpha_mix(self->alpha, 64));
            egui_canvas_draw_circle_fill(content_x + left_w - 10, footer_y + footer_h / 2, 2, footer_left_color, egui_color_alpha_mix(self->alpha, 40));
            egui_view_clip_launcher_grid_draw_text(local, self, snapshot->footer_left ? snapshot->footer_left : "Track focus", content_x + 6, footer_y,
                                                   left_w - 12, footer_h, EGUI_ALIGN_CENTER, local->locked_mode ? local->muted_text_color : local->text_color);

            egui_canvas_draw_round_rectangle_fill(content_x + left_w + pill_gap, footer_y, right_w, footer_h, 5,
                                                  egui_rgb_mix(local->panel_color, footer_right_color, 16), egui_color_alpha_mix(self->alpha, 54));
            egui_canvas_draw_round_rectangle(content_x + left_w + pill_gap, footer_y, right_w, footer_h, 5, 1, footer_right_color,
                                             egui_color_alpha_mix(self->alpha, 42));
            egui_canvas_draw_round_rectangle_fill(content_x + left_w + pill_gap + 8, footer_y + 2, right_w - 16, 2, 1, footer_right_color,
                                                  egui_color_alpha_mix(self->alpha, 48));
            egui_canvas_draw_circle_fill(content_x + left_w + pill_gap + 10, footer_y + footer_h / 2, 2, footer_right_color,
                                         egui_color_alpha_mix(self->alpha, 64));
            egui_canvas_draw_line(content_x + left_w + pill_gap + right_w / 2 - 5, footer_y + footer_h - 3, content_x + left_w + pill_gap + right_w / 2 + 4,
                                  footer_y + footer_h - 3, 1, footer_right_color, egui_color_alpha_mix(self->alpha, 30));
            egui_canvas_draw_circle_fill(content_x + left_w + pill_gap + right_w - 10, footer_y + footer_h / 2, 2, footer_right_color,
                                         egui_color_alpha_mix(self->alpha, 40));
            egui_view_clip_launcher_grid_draw_text(local, self, snapshot->footer_right ? snapshot->footer_right : "Scene status",
                                                   content_x + left_w + pill_gap + 6, footer_y, right_w - 12, footer_h, EGUI_ALIGN_CENTER,
                                                   local->locked_mode ? local->muted_text_color : local->text_color);
        }
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_clip_launcher_grid_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_clip_launcher_grid_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_clip_launcher_grid_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_clip_launcher_grid_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_clip_launcher_grid_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x101723);
    local->panel_color = EGUI_COLOR_HEX(0x182231);
    local->border_color = EGUI_COLOR_HEX(0x44576E);
    local->text_color = EGUI_COLOR_HEX(0xE5EDF7);
    local->muted_text_color = EGUI_COLOR_HEX(0x8396AE);
    local->accent_color = EGUI_COLOR_HEX(0x5FC5FF);
    local->queue_color = EGUI_COLOR_HEX(0xF3B34D);
    local->lock_color = EGUI_COLOR_HEX(0xD9A66F);
    local->play_color = EGUI_COLOR_HEX(0x5CE2AF);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->compact_mode = 0;
    local->locked_mode = 0;
}
