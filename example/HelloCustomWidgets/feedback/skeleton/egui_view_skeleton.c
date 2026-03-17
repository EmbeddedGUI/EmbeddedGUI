#include <stdlib.h>

#include "egui_view_skeleton.h"

static uint8_t egui_view_skeleton_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_SKELETON_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_SKELETON_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_skeleton_clamp_block_count(uint8_t count)
{
    if (count > EGUI_VIEW_SKELETON_MAX_BLOCKS)
    {
        return EGUI_VIEW_SKELETON_MAX_BLOCKS;
    }
    return count;
}

static egui_color_t egui_view_skeleton_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 62);
}

static void egui_view_skeleton_tick(egui_timer_t *timer)
{
    egui_view_t *self = (egui_view_t *)timer->user_data;
    EGUI_LOCAL_INIT(egui_view_skeleton_t);

    local->anim_phase = (uint8_t)((local->anim_phase + 1) % 24);
    egui_view_invalidate(self);
}

static void egui_view_skeleton_start_timer(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_skeleton_t);

    if (local->timer_started || local->animation_mode == EGUI_VIEW_SKELETON_ANIM_NONE || local->locked_mode)
    {
        return;
    }

    egui_timer_start_timer(&local->anim_timer, 80, 80);
    local->timer_started = 1;
}

static void egui_view_skeleton_stop_timer(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_skeleton_t);

    if (!local->timer_started)
    {
        return;
    }

    egui_timer_stop_timer(&local->anim_timer);
    local->timer_started = 0;
}

void egui_view_skeleton_set_snapshots(egui_view_t *self, const egui_view_skeleton_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_skeleton_t);

    local->snapshots = snapshots;
    local->snapshot_count = egui_view_skeleton_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_skeleton_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_skeleton_t);

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

uint8_t egui_view_skeleton_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_skeleton_t);
    return local->current_snapshot;
}

void egui_view_skeleton_set_emphasis_block(egui_view_t *self, uint8_t block_index)
{
    EGUI_LOCAL_INIT(egui_view_skeleton_t);

    local->emphasis_block = block_index;
    egui_view_invalidate(self);
}

void egui_view_skeleton_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_skeleton_t);

    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_skeleton_set_show_footer(egui_view_t *self, uint8_t show_footer)
{
    EGUI_LOCAL_INIT(egui_view_skeleton_t);

    local->show_footer = show_footer ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_skeleton_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_skeleton_t);

    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_skeleton_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_skeleton_t);

    local->locked_mode = locked_mode ? 1 : 0;
    if (local->locked_mode)
    {
        egui_view_skeleton_stop_timer(self);
    }
    else
    {
        egui_view_skeleton_start_timer(self);
    }
    egui_view_invalidate(self);
}

void egui_view_skeleton_set_animation_mode(egui_view_t *self, uint8_t animation_mode)
{
    EGUI_LOCAL_INIT(egui_view_skeleton_t);

    if (animation_mode > EGUI_VIEW_SKELETON_ANIM_PULSE)
    {
        animation_mode = EGUI_VIEW_SKELETON_ANIM_PULSE;
    }
    local->animation_mode = animation_mode;
    if (local->animation_mode == EGUI_VIEW_SKELETON_ANIM_NONE || local->locked_mode)
    {
        egui_view_skeleton_stop_timer(self);
    }
    else
    {
        egui_view_skeleton_start_timer(self);
    }
    egui_view_invalidate(self);
}

void egui_view_skeleton_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t block_color, egui_color_t text_color,
                                    egui_color_t muted_text_color, egui_color_t accent_color)
{
    EGUI_LOCAL_INIT(egui_view_skeleton_t);

    local->surface_color = surface_color;
    local->border_color = border_color;
    local->block_color = block_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    egui_view_invalidate(self);
}

static uint8_t egui_view_skeleton_get_pulse_mix(uint8_t phase)
{
    uint8_t local_phase = phase % 12;

    if (local_phase > 6)
    {
        local_phase = 12 - local_phase;
    }
    return 12 + local_phase * 5;
}

static void egui_view_skeleton_draw_footer(egui_view_skeleton_t *local, egui_view_t *self, const char *text, egui_dim_t x, egui_dim_t y, egui_dim_t width,
                                           egui_color_t color)
{
    egui_region_t text_region;

    if (text == NULL)
    {
        return;
    }

    text_region.location.x = x;
    text_region.location.y = y;
    text_region.size.width = width;
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(local->font, text, &text_region, EGUI_ALIGN_LEFT, color, self->alpha);
}

static void egui_view_skeleton_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_skeleton_t);
    egui_region_t region;
    const egui_view_skeleton_snapshot_t *snapshot;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t block_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_dim_t radius;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_width;
    egui_dim_t content_height;
    egui_dim_t footer_y;
    uint8_t block_count;
    uint8_t i;
    uint8_t is_enabled;
    uint8_t pulse_mix = egui_view_skeleton_get_pulse_mix(local->anim_phase);

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    block_count = egui_view_skeleton_clamp_block_count(snapshot->block_count);
    if (snapshot->blocks == NULL || block_count == 0)
    {
        return;
    }

    is_enabled = egui_view_get_enable(self) ? 1 : 0;
    surface_color = local->surface_color;
    border_color = local->border_color;
    block_color = local->block_color;
    text_color = local->text_color;
    muted_text_color = local->muted_text_color;
    accent_color = local->accent_color;
    radius = local->compact_mode ? 7 : 10;

    if (local->locked_mode)
    {
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xFAFBFC), 18);
        border_color = egui_rgb_mix(border_color, EGUI_COLOR_HEX(0xF7F9FB), 30);
        block_color = egui_rgb_mix(block_color, EGUI_COLOR_HEX(0xF7F9FB), 22);
        text_color = egui_rgb_mix(text_color, muted_text_color, 76);
        accent_color = egui_rgb_mix(block_color, muted_text_color, 34);
    }
    else if (!local->compact_mode)
    {
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xFFFFFF), 14);
        border_color = egui_rgb_mix(border_color, EGUI_COLOR_HEX(0xFFFFFF), 28);
    }

    if (!is_enabled)
    {
        surface_color = egui_view_skeleton_mix_disabled(surface_color);
        border_color = egui_view_skeleton_mix_disabled(border_color);
        block_color = egui_view_skeleton_mix_disabled(block_color);
        text_color = egui_view_skeleton_mix_disabled(text_color);
        muted_text_color = egui_view_skeleton_mix_disabled(muted_text_color);
        accent_color = egui_view_skeleton_mix_disabled(accent_color);
    }

    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, radius, surface_color,
                                          egui_color_alpha_mix(self->alpha, local->compact_mode ? 90 : 94));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, radius, 1, border_color,
                                     egui_color_alpha_mix(self->alpha, local->compact_mode ? 66 : 70));

    content_x = region.location.x + (local->compact_mode ? 8 : 10);
    content_y = region.location.y + (local->compact_mode ? 8 : 10);
    content_width = region.size.width - (local->compact_mode ? 16 : 20);
    content_height = region.size.height - (local->compact_mode ? 16 : 20);
    if (local->show_footer && !local->compact_mode)
    {
        content_height -= 12;
    }
    if (content_width <= 0 || content_height <= 0)
    {
        return;
    }

    for (i = 0; i < block_count; i++)
    {
        const egui_view_skeleton_block_t *block = &snapshot->blocks[i];
        egui_dim_t x = content_x + block->x;
        egui_dim_t y = content_y + block->y;
        egui_dim_t w = block->width;
        egui_dim_t h = block->height;
        egui_dim_t block_right;
        egui_dim_t band_width;
        egui_dim_t band_x;
        egui_color_t fill_color;
        egui_color_t line_color;
        uint8_t is_emphasis;

        if (w <= 0 || h <= 0)
        {
            continue;
        }
        if (x + w > content_x + content_width || y + h > content_y + content_height)
        {
            continue;
        }

        is_emphasis = (i == snapshot->emphasis_block) || (i == local->emphasis_block);
        fill_color = egui_rgb_mix(block_color, accent_color, is_emphasis ? 9 : 2);
        line_color = egui_rgb_mix(border_color, accent_color, is_emphasis ? 18 : 4);

        if (local->animation_mode == EGUI_VIEW_SKELETON_ANIM_PULSE && is_emphasis && !local->locked_mode)
        {
            fill_color = egui_rgb_mix(block_color, accent_color, pulse_mix);
            line_color = egui_rgb_mix(border_color, accent_color, pulse_mix + 8);
        }

        egui_canvas_draw_round_rectangle_fill(x, y, w, h, block->radius, fill_color, egui_color_alpha_mix(self->alpha, local->compact_mode ? 72 : 84));
        egui_canvas_draw_round_rectangle(x, y, w, h, block->radius, 1, line_color,
                                         egui_color_alpha_mix(self->alpha, local->compact_mode ? (is_emphasis ? 54 : 22) : (is_emphasis ? 62 : 28)));

        if (local->animation_mode == EGUI_VIEW_SKELETON_ANIM_WAVE && !local->locked_mode)
        {
            egui_dim_t overlap_x0;
            egui_dim_t overlap_x1;
            egui_color_t band_color = egui_rgb_mix(block_color, accent_color, is_emphasis ? 18 : 12);

            band_width = local->compact_mode ? 10 : 14;
            block_right = content_width + (local->compact_mode ? 16 : 24);
            band_x = content_x - band_width + ((egui_dim_t)local->anim_phase * block_right) / 24;
            overlap_x0 = EGUI_MAX(x + 1, band_x);
            overlap_x1 = EGUI_MIN(x + w - 1, band_x + band_width);
            if (overlap_x1 > overlap_x0)
            {
                egui_canvas_draw_round_rectangle_fill(overlap_x0, y + 1, overlap_x1 - overlap_x0, h - 2, EGUI_MIN(block->radius, 3), band_color,
                                                      egui_color_alpha_mix(self->alpha, is_emphasis ? 46 : 30));
            }
        }
    }

    if (local->show_footer && !local->compact_mode)
    {
        footer_y = content_y + content_height + 3;
        egui_view_skeleton_draw_footer(local, self, snapshot->footer ? snapshot->footer : "Loading content", content_x, footer_y, content_width,
                                       local->locked_mode ? muted_text_color : text_color);
    }
}

static void egui_view_skeleton_on_attach(egui_view_t *self)
{
    egui_view_on_attach_to_window(self);
    egui_view_skeleton_start_timer(self);
}

static void egui_view_skeleton_on_detach(egui_view_t *self)
{
    egui_view_skeleton_stop_timer(self);
    egui_view_on_detach_from_window(self);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_skeleton_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_skeleton_on_attach,
        .on_draw = egui_view_skeleton_on_draw,
        .on_detach_from_window = egui_view_skeleton_on_detach,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_skeleton_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_skeleton_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_skeleton_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD7E0E8);
    local->block_color = EGUI_COLOR_HEX(0xE7EDF3);
    local->text_color = EGUI_COLOR_HEX(0x5E6F80);
    local->muted_text_color = EGUI_COLOR_HEX(0x8592A1);
    local->accent_color = EGUI_COLOR_HEX(0x74A9F9);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->emphasis_block = 0xFF;
    local->show_footer = 1;
    local->compact_mode = 0;
    local->locked_mode = 0;
    local->animation_mode = EGUI_VIEW_SKELETON_ANIM_WAVE;
    local->anim_phase = 0;
    local->timer_started = 0;
    egui_timer_init_timer(&local->anim_timer, self, egui_view_skeleton_tick);
}
