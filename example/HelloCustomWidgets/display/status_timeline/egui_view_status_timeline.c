#define TIMELINE_PANEL_SHADOW_ALPHA      20
#define TIMELINE_PANEL_FILL_ALPHA        32
#define TIMELINE_PANEL_BORDER_ALPHA      52
#define TIMELINE_INNER_BORDER_ALPHA      20
#define TIMELINE_HEADER_PILL_MIN_WIDTH   82
#define TIMELINE_HEADER_LINE_ALPHA       22
#define TIMELINE_FOOTER_WIDTH            84
#define TIMELINE_FOOTER_FILL_ALPHA       44
#define TIMELINE_FOOTER_BORDER_ALPHA     22
#define TIMELINE_MINI_BADGE_WIDTH        30
#define TIMELINE_MINI_BADGE_FILL_ALPHA   70
#define TIMELINE_MINI_BADGE_BORDER_ALPHA 20
#define TIMELINE_MINI_TOP_STRIP_ALPHA    18
#define TIMELINE_MINI_BOTTOM_STRIP_ALPHA 18
#define TIMELINE_DISABLED_OVERLAY_ALPHA  14
#define TIMELINE_DISABLED_CROSS_ALPHA    12

#include <stdlib.h>
#include <string.h>

#include "egui_view_status_timeline.h"

typedef struct egui_view_status_timeline_rect egui_view_status_timeline_rect_t;
struct egui_view_status_timeline_rect
{
    egui_dim_t x;
    egui_dim_t y;
    egui_dim_t width;
    egui_dim_t height;
};

static uint8_t egui_view_status_timeline_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_STATUS_TIMELINE_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_STATUS_TIMELINE_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_status_timeline_get_step_count(const egui_view_status_timeline_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return 0;
    }
    if (snapshot->step_count > EGUI_VIEW_STATUS_TIMELINE_MAX_STEPS)
    {
        return EGUI_VIEW_STATUS_TIMELINE_MAX_STEPS;
    }
    return snapshot->step_count;
}

static egui_color_t egui_view_status_timeline_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_70);
}

static const char *egui_view_status_timeline_get_header_text(egui_view_status_timeline_t *local, uint8_t is_enabled)
{
    if (!is_enabled)
    {
        return "Locked";
    }
    return local->snapshots[local->current_snapshot].title;
}

static const char *egui_view_status_timeline_get_footer_text(egui_view_status_timeline_t *local, uint8_t is_enabled)
{
    if (!is_enabled)
    {
        return "Read Only";
    }
    return (local->current_snapshot == 0) ? "Stage A" : ((local->current_snapshot == 1) ? "Stage B" : "Stage C");
}

static void egui_view_status_timeline_draw_tag(const egui_font_t *font, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_color_t fill_color,
                                               egui_color_t text_color, const char *text, egui_alpha_t alpha)
{
    egui_region_t text_region;

    if (width <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle_fill(x, y, width, 9, 4, fill_color, alpha);
    text_region.location.x = x;
    text_region.location.y = y;
    text_region.size.width = width;
    text_region.size.height = 9;
    egui_canvas_draw_text_in_rect(font, text, &text_region, EGUI_ALIGN_CENTER, text_color, alpha);
}

static void egui_view_status_timeline_draw_step(egui_view_status_timeline_t *local, const egui_view_status_timeline_step_t *step, uint8_t step_index,
                                                uint8_t current_step, egui_view_status_timeline_rect_t rect, egui_alpha_t alpha, uint8_t is_enabled)
{
    egui_region_t text_region;
    egui_color_t node_color;
    egui_color_t card_fill;
    egui_color_t card_border;
    egui_color_t title_color;
    egui_color_t tag_fill;
    egui_color_t line_color;
    egui_dim_t dot_x = rect.x;
    egui_dim_t dot_y = rect.y + 4;
    egui_dim_t card_x = rect.x + 11;
    egui_dim_t card_y = rect.y;
    egui_dim_t card_width = rect.width - 11;
    uint8_t is_done = step_index < current_step;
    uint8_t is_active = step_index == current_step;
    uint8_t is_focus = step_index == local->focus_step;

    if (is_done)
    {
        node_color = local->done_color;
    }
    else if (is_active)
    {
        node_color = local->active_color;
    }
    else
    {
        node_color = local->muted_text_color;
    }

    line_color = egui_rgb_mix(local->border_color, node_color, EGUI_ALPHA_40);
    card_fill = egui_rgb_mix(local->surface_color, node_color, is_active ? EGUI_ALPHA_30 : EGUI_ALPHA_20);
    card_border = is_focus ? local->active_color : egui_rgb_mix(local->border_color, node_color, EGUI_ALPHA_30);
    title_color = is_active ? local->text_color : (is_done ? local->text_color : local->muted_text_color);
    tag_fill = egui_rgb_mix(node_color, EGUI_COLOR_WHITE, EGUI_ALPHA_20);

    if (!is_enabled)
    {
        node_color = egui_view_status_timeline_mix_disabled(node_color);
        line_color = egui_view_status_timeline_mix_disabled(line_color);
        card_fill = egui_view_status_timeline_mix_disabled(card_fill);
        card_border = egui_view_status_timeline_mix_disabled(card_border);
        title_color = egui_view_status_timeline_mix_disabled(title_color);
        tag_fill = egui_view_status_timeline_mix_disabled(tag_fill);
    }

    egui_canvas_draw_line(dot_x + 3, rect.y, dot_x + 3, rect.y + rect.height, 1, line_color, egui_color_alpha_mix(alpha, EGUI_ALPHA_50));
    egui_canvas_draw_round_rectangle_fill(dot_x, dot_y, 7, 7, 3, node_color, egui_color_alpha_mix(alpha, is_active ? EGUI_ALPHA_100 : EGUI_ALPHA_70));
    egui_canvas_draw_round_rectangle(dot_x, dot_y, 7, 7, 3, 1, local->border_color, egui_color_alpha_mix(alpha, EGUI_ALPHA_50));

    egui_canvas_draw_round_rectangle_fill(card_x, card_y, card_width, rect.height - 2, 4, card_fill, egui_color_alpha_mix(alpha, EGUI_ALPHA_30));
    egui_canvas_draw_round_rectangle(card_x, card_y, card_width, rect.height - 2, 4, is_focus ? 2 : 1, card_border,
                                     egui_color_alpha_mix(alpha, is_focus ? EGUI_ALPHA_80 : EGUI_ALPHA_50));

    if (!local->compact_mode && step->tag != NULL && step->tag[0] != 0)
    {
        egui_view_status_timeline_draw_tag(local->font, card_x + 4, card_y + 3, 24, tag_fill, local->text_color, step->tag,
                                           egui_color_alpha_mix(alpha, EGUI_ALPHA_70));
    }

    text_region.location.x = card_x + (local->compact_mode ? 4 : 32);
    text_region.location.y = card_y + 2;
    text_region.size.width = card_width - (local->compact_mode ? 8 : 36);
    text_region.size.height = rect.height - 6;
    egui_canvas_draw_text_in_rect(local->font, step->title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color, alpha);
}

void egui_view_status_timeline_set_snapshots(egui_view_t *self, const egui_view_status_timeline_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_status_timeline_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_status_timeline_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_status_timeline_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_status_timeline_t);
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

uint8_t egui_view_status_timeline_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_status_timeline_t);
    return local->current_snapshot;
}

void egui_view_status_timeline_set_focus_step(egui_view_t *self, uint8_t step_index)
{
    EGUI_LOCAL_INIT(egui_view_status_timeline_t);
    local->focus_step = step_index;
    egui_view_invalidate(self);
}

void egui_view_status_timeline_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_status_timeline_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_status_timeline_set_show_header(egui_view_t *self, uint8_t show_header)
{
    EGUI_LOCAL_INIT(egui_view_status_timeline_t);
    local->show_header = show_header ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_status_timeline_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_status_timeline_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_status_timeline_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                           egui_color_t muted_text_color, egui_color_t active_color, egui_color_t done_color)
{
    EGUI_LOCAL_INIT(egui_view_status_timeline_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->active_color = active_color;
    local->done_color = done_color;
    egui_view_invalidate(self);
}

static void egui_view_status_timeline_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_status_timeline_t);
    egui_region_t region;
    egui_region_t header_region;
    egui_region_t text_region;
    egui_view_status_timeline_rect_t content;
    const egui_view_status_timeline_snapshot_t *snapshot;
    uint8_t step_count;
    egui_color_t panel_color;
    egui_color_t shadow_color;
    uint8_t is_enabled;
    egui_dim_t row_height;
    egui_dim_t footer_height = 0;
    egui_dim_t i;
    char compact_badge[3] = {'L', 'K', '\0'};
    const char *header_text;
    const char *footer_text;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    step_count = egui_view_status_timeline_get_step_count(snapshot);
    if (snapshot->steps == NULL || step_count == 0)
    {
        return;
    }

    is_enabled = egui_view_get_enable(self) ? 1 : 0;
    panel_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_30);
    shadow_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_10);
    if (!is_enabled)
    {
        panel_color = egui_view_status_timeline_mix_disabled(panel_color);
        shadow_color = egui_view_status_timeline_mix_disabled(shadow_color);
    }

    egui_canvas_draw_round_rectangle_fill(region.location.x + 1, region.location.y + 2, region.size.width, region.size.height, 8, shadow_color,
                                          egui_color_alpha_mix(self->alpha, TIMELINE_PANEL_SHADOW_ALPHA));
    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, 8, panel_color,
                                          egui_color_alpha_mix(self->alpha, TIMELINE_PANEL_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, 8, 1, local->border_color,
                                     egui_color_alpha_mix(self->alpha, TIMELINE_PANEL_BORDER_ALPHA));
    if (TIMELINE_INNER_BORDER_ALPHA > 0)
    {
        egui_canvas_draw_round_rectangle(region.location.x + 2, region.location.y + 2, region.size.width - 4, region.size.height - 4, 6, 1,
                                         egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, TIMELINE_INNER_BORDER_ALPHA));
    }

    content.x = region.location.x + 5;
    content.y = region.location.y + 4;
    content.width = region.size.width - 10;
    content.height = region.size.height - 8;

    if (!local->show_header)
    {
        if (is_enabled)
        {
            compact_badge[0] = (local->current_snapshot == 0) ? 'A' : ((local->current_snapshot == 1) ? 'B' : 'C');
            compact_badge[1] = '\0';
        }
        if (TIMELINE_MINI_TOP_STRIP_ALPHA > 0)
        {
            egui_canvas_draw_round_rectangle_fill(region.location.x + 12, region.location.y + 6, region.size.width - 20, 2, 1,
                                                  is_enabled ? local->text_color : local->muted_text_color,
                                                  egui_color_alpha_mix(self->alpha, TIMELINE_MINI_TOP_STRIP_ALPHA));
        }
        header_region.location.x = content.x + (content.width - TIMELINE_MINI_BADGE_WIDTH) / 2;
        header_region.location.y = content.y + 6;
        header_region.size.width = TIMELINE_MINI_BADGE_WIDTH;
        header_region.size.height = 10;
        egui_canvas_draw_round_rectangle_fill(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 4,
                                              egui_rgb_mix(local->surface_color, local->border_color, EGUI_ALPHA_10),
                                              egui_color_alpha_mix(self->alpha, TIMELINE_MINI_BADGE_FILL_ALPHA));
        egui_canvas_draw_round_rectangle(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 4, 1,
                                         egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, TIMELINE_MINI_BADGE_BORDER_ALPHA));
        egui_canvas_draw_text_in_rect(local->font, compact_badge, &header_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color,
                                      self->alpha);
        content.y += 12;
        content.height -= 12;
    }

    if (local->show_header)
    {
        egui_dim_t pill_w;

        header_text = egui_view_status_timeline_get_header_text(local, is_enabled);
        pill_w = 34 + (egui_dim_t)strlen(header_text) * 6;
        if (pill_w < TIMELINE_HEADER_PILL_MIN_WIDTH)
        {
            pill_w = TIMELINE_HEADER_PILL_MIN_WIDTH;
        }
        if (pill_w > content.width)
        {
            pill_w = content.width;
        }
        header_region.location.x = content.x + (content.width - pill_w) / 2;
        header_region.location.y = content.y;
        header_region.size.width = pill_w;
        header_region.size.height = 14;
        egui_canvas_draw_round_rectangle_fill(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 6,
                                              egui_rgb_mix(local->surface_color, local->border_color, EGUI_ALPHA_10),
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));
        egui_canvas_draw_round_rectangle(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 6, 1,
                                         egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        egui_canvas_draw_text_in_rect(local->font, header_text, &header_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color,
                                      self->alpha);
        if (TIMELINE_HEADER_LINE_ALPHA > 0)
        {
            egui_canvas_draw_line(content.x + 5, content.y + 17, content.x + content.width - 6, content.y + 17, 1, local->border_color,
                                  egui_color_alpha_mix(self->alpha, TIMELINE_HEADER_LINE_ALPHA));
        }
        content.y += 20;
        content.height -= 20;
        footer_height = 14;
    }
    if (content.width <= 16 || content.height <= 12)
    {
        return;
    }
    if (footer_height > 0 && content.height > footer_height)
    {
        content.height -= footer_height;
    }

    row_height = content.height / step_count;
    if (row_height < 14)
    {
        row_height = 14;
    }
    for (i = 0; i < step_count; i++)
    {
        egui_view_status_timeline_rect_t row_rect;

        row_rect.x = content.x;
        row_rect.y = content.y + i * row_height;
        row_rect.width = content.width;
        row_rect.height = row_height;
        if (i == step_count - 1)
        {
            row_rect.height = content.y + content.height - row_rect.y;
        }
        egui_view_status_timeline_draw_step(local, &snapshot->steps[i], i, snapshot->current_step, row_rect, self->alpha, is_enabled);
    }

    if (!is_enabled)
    {
        egui_canvas_draw_round_rectangle_fill(content.x + 1, content.y + 1, content.width - 2, content.height - 2, 4, EGUI_COLOR_BLACK,
                                              egui_color_alpha_mix(self->alpha, TIMELINE_DISABLED_OVERLAY_ALPHA));
        egui_canvas_draw_line(content.x + 3, content.y + 3, content.x + content.width - 4, content.y + content.height - 4, 1, local->muted_text_color,
                              egui_color_alpha_mix(self->alpha, TIMELINE_DISABLED_CROSS_ALPHA));
        egui_canvas_draw_line(content.x + 3, content.y + content.height - 4, content.x + content.width - 4, content.y + 3, 1, local->muted_text_color,
                              egui_color_alpha_mix(self->alpha, TIMELINE_DISABLED_CROSS_ALPHA));
    }

    if (local->show_header)
    {
        footer_text = egui_view_status_timeline_get_footer_text(local, is_enabled);
        text_region.location.x = content.x;
        text_region.location.y = content.y + content.height + 3;
        text_region.size.width = content.width;
        text_region.size.height = 11;
        egui_canvas_draw_round_rectangle_fill(content.x + (content.width - TIMELINE_FOOTER_WIDTH) / 2, content.y + content.height + 2, TIMELINE_FOOTER_WIDTH,
                                              11, 5, panel_color, egui_color_alpha_mix(self->alpha, TIMELINE_FOOTER_FILL_ALPHA));
        egui_canvas_draw_round_rectangle(content.x + (content.width - TIMELINE_FOOTER_WIDTH) / 2, content.y + content.height + 2, TIMELINE_FOOTER_WIDTH, 11, 5,
                                         1, egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, TIMELINE_FOOTER_BORDER_ALPHA));
        egui_canvas_draw_text_in_rect(local->font, footer_text, &text_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color,
                                      self->alpha);
    }
    else if (TIMELINE_MINI_BOTTOM_STRIP_ALPHA > 0)
    {
        egui_canvas_draw_round_rectangle_fill(content.x + 8, region.location.y + region.size.height - 8, content.width - 14, 2, 1,
                                              is_enabled ? local->text_color : local->muted_text_color,
                                              egui_color_alpha_mix(self->alpha, TIMELINE_MINI_BOTTOM_STRIP_ALPHA));
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_status_timeline_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_status_timeline_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_status_timeline_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_status_timeline_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_status_timeline_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x101723);
    local->border_color = EGUI_COLOR_HEX(0x4A5568);
    local->text_color = EGUI_COLOR_HEX(0xE2E8F0);
    local->muted_text_color = EGUI_COLOR_HEX(0x94A3B8);
    local->active_color = EGUI_COLOR_HEX(0x38BDF8);
    local->done_color = EGUI_COLOR_HEX(0x22C55E);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->focus_step = 1;
    local->show_header = 1;
    local->compact_mode = 0;
}
