#include <stdlib.h>
#include <string.h>

#include "egui_view_tab_expose.h"

static uint8_t egui_view_tab_expose_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_TAB_EXPOSE_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_TAB_EXPOSE_MAX_SNAPSHOTS;
    }
    return count;
}

void egui_view_tab_expose_set_snapshots(egui_view_t *self, const egui_view_tab_expose_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_tab_expose_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_tab_expose_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_tab_expose_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_tab_expose_t);
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

uint8_t egui_view_tab_expose_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_tab_expose_t);
    return local->current_snapshot;
}

void egui_view_tab_expose_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_tab_expose_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_tab_expose_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_tab_expose_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_tab_expose_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_tab_expose_t);
    local->locked_mode = locked_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_tab_expose_set_palette(
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
    EGUI_LOCAL_INIT(egui_view_tab_expose_t);
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

static egui_color_t egui_view_tab_expose_get_status_color(egui_view_tab_expose_t *local, const egui_view_tab_expose_snapshot_t *snapshot)
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

static egui_dim_t egui_view_tab_expose_get_pill_width(const egui_view_tab_expose_snapshot_t *snapshot, uint8_t compact_mode)
{
    egui_dim_t width;
    size_t len;

    len = strlen(snapshot->status);
    width = compact_mode ? (egui_dim_t)(14 + len * 5) : (egui_dim_t)(14 + len * 6);
    if (compact_mode)
    {
        if (width < 30)
        {
            width = 30;
        }
        if (width > 40)
        {
            width = 40;
        }
    }
    else
    {
        if (width < 38)
        {
            width = 38;
        }
        if (width > 52)
        {
            width = 52;
        }
    }
    return width;
}

static void egui_view_tab_expose_draw_preview_card(
        egui_view_t *self,
        egui_view_tab_expose_t *local,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t w,
        egui_dim_t h,
        uint8_t highlighted,
        egui_color_t accent_color)
{
    egui_dim_t inner_h;
    egui_dim_t inner_w;
    egui_dim_t chrome_h;

    egui_canvas_draw_round_rectangle_fill(
            x,
            y,
            w,
            h,
            6,
            highlighted ? accent_color : egui_rgb_mix(local->panel_color, local->border_color, 24),
            egui_color_alpha_mix(self->alpha, highlighted ? 74 : (local->compact_mode ? 46 : 44)));
    egui_canvas_draw_round_rectangle(
            x,
            y,
            w,
            h,
            6,
            1,
            highlighted ? local->focus_color : local->border_color,
            egui_color_alpha_mix(self->alpha, highlighted ? 68 : 34));

    inner_w = w - 8;
    inner_h = h - (local->compact_mode ? 7 : 9);
    chrome_h = local->compact_mode ? 3 : 4;
    egui_canvas_draw_round_rectangle_fill(
            x + 4,
            y + 3,
            inner_w,
            chrome_h,
            4,
            local->surface_color,
            egui_color_alpha_mix(self->alpha, highlighted ? 44 : 30));
    egui_canvas_draw_round_rectangle_fill(
            x + w - (local->compact_mode ? 11 : 13),
            y + 4,
            local->compact_mode ? 4 : 5,
            local->compact_mode ? 2 : 3,
            1,
            highlighted ? local->focus_color : local->muted_text_color,
            egui_color_alpha_mix(self->alpha, highlighted ? 72 : 36));
    egui_canvas_draw_round_rectangle_fill(
            x + 4,
            y + 5 + chrome_h,
            inner_w,
            inner_h / 2 - 1,
            3,
            local->surface_color,
            egui_color_alpha_mix(self->alpha, highlighted ? 28 : 18));
    egui_canvas_draw_round_rectangle_fill(
            x + 6,
            y + h - (local->compact_mode ? 6 : 8),
            w - (local->compact_mode ? 12 : 14),
            2,
            1,
            local->text_color,
            egui_color_alpha_mix(self->alpha, highlighted ? 50 : (local->compact_mode ? 34 : 30)));
}

static void egui_view_tab_expose_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_tab_expose_t);
    egui_region_t region;
    egui_region_t text_region;
    const egui_view_tab_expose_snapshot_t *snapshot;
    egui_color_t status_color;
    egui_color_t shell_color;
    egui_color_t summary_color;
    egui_color_t footer_color;
    egui_color_t title_color;
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
    egui_dim_t tab_w;
    egui_dim_t preview_x;
    egui_dim_t preview_y;
    egui_dim_t preview_w;
    egui_dim_t preview_h;
    egui_dim_t preview_gap;
    egui_dim_t footer_y;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    status_color = egui_view_tab_expose_get_status_color(local, snapshot);
    shell_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, 28);
    title_color = local->compact_mode ? egui_rgb_mix(local->muted_text_color, status_color, local->locked_mode ? 24 : 44) : local->muted_text_color;
    summary_color = local->locked_mode ? egui_rgb_mix(local->text_color, local->muted_text_color, 38)
                                       : (local->compact_mode ? local->text_color : egui_rgb_mix(local->text_color, local->muted_text_color, 18));
    footer_color = local->locked_mode ? egui_rgb_mix(local->muted_text_color, local->border_color, 34)
                                      : (local->compact_mode ? local->muted_text_color
                                                             : egui_rgb_mix(local->muted_text_color, local->text_color, 12));

    panel_x = region.location.x;
    panel_y = region.location.y;
    panel_w = region.size.width;
    panel_h = region.size.height;
    outer_padding = local->compact_mode ? 11 : 14;
    header_top = local->compact_mode ? 7 : 9;
    pill_w = egui_view_tab_expose_get_pill_width(snapshot, local->compact_mode);

    egui_canvas_draw_round_rectangle_fill(
            panel_x,
            panel_y,
            panel_w,
            panel_h,
            10,
            shell_color,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? 40 : 52));
    egui_canvas_draw_round_rectangle(
            panel_x,
            panel_y,
            panel_w,
            panel_h,
            10,
            1,
            local->border_color,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? 56 : 68));

    pill_x = panel_x + panel_w - outer_padding - pill_w;
    title_w = pill_x - panel_x - outer_padding - (local->compact_mode ? 12 : 15);

    text_region.location.x = panel_x + outer_padding + (local->compact_mode ? 2 : 2);
    text_region.location.y = panel_y + header_top;
    text_region.size.width = title_w;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(local->font, snapshot->title, &text_region, EGUI_ALIGN_LEFT, title_color, self->alpha);

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

    summary_y = panel_y + (local->compact_mode ? 20 : 29);
    text_region.location.x = panel_x + outer_padding + (local->compact_mode ? 2 : 5);
    text_region.location.y = summary_y;
    text_region.size.width = panel_w - outer_padding * 2 - (local->compact_mode ? 4 : 10);
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(local->font, snapshot->summary, &text_region, EGUI_ALIGN_CENTER, summary_color, self->alpha);

    strip_x = panel_x + outer_padding;
    strip_y = panel_y + (local->compact_mode ? 30 : 46);
    strip_w = panel_w - outer_padding * 2;
    strip_h = local->compact_mode ? 10 : 14;
    tab_w = (strip_w - (local->compact_mode ? 3 : 6) * 2) / 3;

    for (i = 0; i < 3; i++)
    {
        egui_dim_t tab_x;
        egui_dim_t current_tab_w;
        uint8_t active;

        tab_x = strip_x + i * (tab_w + (local->compact_mode ? 3 : 6));
        current_tab_w = (i == 2) ? (strip_x + strip_w - tab_x) : tab_w;
        active = (snapshot->active_tab == i) ? 1 : 0;
        egui_canvas_draw_round_rectangle_fill(
                tab_x,
                strip_y,
                current_tab_w,
                strip_h,
                strip_h / 2,
                active ? status_color : egui_rgb_mix(local->panel_color, local->border_color, 24),
                egui_color_alpha_mix(self->alpha, active ? 72 : 38));
        egui_canvas_draw_round_rectangle(
                tab_x,
                strip_y,
                current_tab_w,
                strip_h,
                strip_h / 2,
                1,
                active ? local->focus_color : local->border_color,
                egui_color_alpha_mix(self->alpha, active ? 70 : 30));
        egui_canvas_draw_round_rectangle_fill(
                tab_x + current_tab_w / 2 - (active ? 7 : 4),
                strip_y + strip_h - (active ? 5 : 4),
                active ? 14 : 8,
                2,
                1,
                active ? local->focus_color : local->muted_text_color,
                egui_color_alpha_mix(self->alpha, active ? 96 : 26));
    }

    preview_x = panel_x + outer_padding;
    preview_y = strip_y + strip_h + (local->compact_mode ? 2 : 6);
    preview_w = panel_w - outer_padding * 2;
    preview_h = local->compact_mode ? 15 : 28;
    preview_gap = local->compact_mode ? 3 : 5;

    for (i = 0; i < 3; i++)
    {
        egui_dim_t card_x;
        egui_dim_t card_w;
        uint8_t highlighted;

        card_x = preview_x + i * ((preview_w - preview_gap * 2) / 3 + preview_gap);
        card_w = (i == 2) ? (preview_x + preview_w - card_x) : ((preview_w - preview_gap * 2) / 3);
        highlighted = (snapshot->preview_mask & (1u << i)) ? 1 : 0;
        egui_view_tab_expose_draw_preview_card(self, local, card_x, preview_y, card_w, preview_h, highlighted, status_color);
    }

    if (!local->compact_mode)
    {
        egui_dim_t active_center_x;

        active_center_x = preview_x + snapshot->active_tab * ((preview_w - preview_gap * 2) / 3 + preview_gap) + ((preview_w - preview_gap * 2) / 3) / 2;
        egui_canvas_draw_round_rectangle_fill(
                active_center_x - 1,
                preview_y - 4,
                2,
                3,
                1,
                status_color,
                egui_color_alpha_mix(self->alpha, 72));
    }

    footer_y = preview_y + preview_h + (local->compact_mode ? 2 : 5);
    text_region.location.x = panel_x + outer_padding + (local->compact_mode ? 5 : 6);
    text_region.location.y = footer_y;
    text_region.size.width = panel_w - outer_padding * 2 - (local->compact_mode ? 10 : 12);
    text_region.size.height = EGUI_MAX(panel_h - (footer_y - panel_y) - 6, 10);
    egui_canvas_draw_text_in_rect(local->font, snapshot->footer, &text_region, EGUI_ALIGN_CENTER, footer_color, self->alpha);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_tab_expose_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_tab_expose_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_tab_expose_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_tab_expose_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_tab_expose_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x111923);
    local->panel_color = EGUI_COLOR_HEX(0x1A2636);
    local->border_color = EGUI_COLOR_HEX(0x435873);
    local->text_color = EGUI_COLOR_HEX(0xEFF5FF);
    local->muted_text_color = EGUI_COLOR_HEX(0x9AACBF);
    local->accent_color = EGUI_COLOR_HEX(0x67D6FF);
    local->warn_color = EGUI_COLOR_HEX(0xF2B05E);
    local->lock_color = EGUI_COLOR_HEX(0xD6A36E);
    local->focus_color = EGUI_COLOR_HEX(0x95EEFF);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->compact_mode = 0;
    local->locked_mode = 0;
}
