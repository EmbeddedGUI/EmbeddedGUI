#define NOTIFY_PANEL_SHADOW_ALPHA 20
#define NOTIFY_PANEL_FILL_ALPHA 32
#define NOTIFY_PANEL_BORDER_ALPHA 52
#define NOTIFY_INNER_BORDER_ALPHA 20
#define NOTIFY_HEADER_PILL_MIN_WIDTH 82
#define NOTIFY_HEADER_LINE_ALPHA 22
#define NOTIFY_FOOTER_WIDTH 84
#define NOTIFY_FOOTER_FILL_ALPHA 44
#define NOTIFY_FOOTER_BORDER_ALPHA 22
#define NOTIFY_MINI_BADGE_WIDTH 30
#define NOTIFY_MINI_BADGE_FILL_ALPHA 70
#define NOTIFY_MINI_BADGE_BORDER_ALPHA 20
#define NOTIFY_MINI_TOP_STRIP_ALPHA 18
#define NOTIFY_MINI_BOTTOM_STRIP_ALPHA 18
#define NOTIFY_DISABLED_OVERLAY_ALPHA 14
#define NOTIFY_DISABLED_CROSS_ALPHA 12

#include <stdlib.h>
#include <string.h>

#include "egui_view_notification_stack.h"

static const egui_color_t severity_palette[] = {
        EGUI_COLOR_HEX(0x38BDF8),
        EGUI_COLOR_HEX(0xF59E0B),
        EGUI_COLOR_HEX(0xF43F5E),
};

static uint8_t egui_view_notification_stack_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_NOTIFICATION_STACK_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_NOTIFICATION_STACK_MAX_SNAPSHOTS;
    }
    return count;
}

static egui_color_t egui_view_notification_stack_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_70);
}

static egui_color_t egui_view_notification_stack_get_severity_color(uint8_t level)
{
    if (level >= (sizeof(severity_palette) / sizeof(severity_palette[0])))
    {
        level = 0;
    }
    return severity_palette[level];
}

static const char *egui_view_notification_stack_get_header_text(egui_view_notification_stack_t *local, uint8_t is_enabled)
{
    if (!is_enabled)
    {
        return "Locked";
    }
    return local->snapshots[local->current_snapshot].title;
}

static const char *egui_view_notification_stack_get_footer_text(egui_view_notification_stack_t *local, uint8_t is_enabled)
{
    if (!is_enabled)
    {
        return "Read Only";
    }
    return (local->current_snapshot == 0) ? "Queue A" : ((local->current_snapshot == 1) ? "Queue B" : "Queue C");
}

static void egui_view_notification_stack_draw_badge(
        const egui_font_t *font,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t width,
        const char *text,
        egui_color_t fill_color,
        egui_color_t text_color,
        egui_alpha_t alpha)
{
    egui_region_t text_region;

    egui_canvas_draw_round_rectangle_fill(x, y, width, 10, 4, fill_color, alpha);
    text_region.location.x = x;
    text_region.location.y = y;
    text_region.size.width = width;
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(font, text, &text_region, EGUI_ALIGN_CENTER, text_color, alpha);
}

void egui_view_notification_stack_set_snapshots(egui_view_t *self, const egui_view_notification_stack_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_notification_stack_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_notification_stack_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_notification_stack_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_notification_stack_t);
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

uint8_t egui_view_notification_stack_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_notification_stack_t);
    return local->current_snapshot;
}

void egui_view_notification_stack_set_focus_card(egui_view_t *self, uint8_t card_index)
{
    EGUI_LOCAL_INIT(egui_view_notification_stack_t);
    local->focus_card = card_index;
    egui_view_invalidate(self);
}

void egui_view_notification_stack_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_notification_stack_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_notification_stack_set_show_header(egui_view_t *self, uint8_t show_header)
{
    EGUI_LOCAL_INIT(egui_view_notification_stack_t);
    local->show_header = show_header ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_notification_stack_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_notification_stack_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_notification_stack_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t border_color,
        egui_color_t text_color,
        egui_color_t muted_text_color,
        egui_color_t focus_color)
{
    EGUI_LOCAL_INIT(egui_view_notification_stack_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->focus_color = focus_color;
    egui_view_invalidate(self);
}

static void egui_view_notification_stack_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_notification_stack_t);
    egui_region_t region;
    egui_region_t header_region;
    egui_region_t text_region;
    const egui_view_notification_stack_snapshot_t *snapshot;
    egui_color_t panel_color;
    egui_color_t shadow_color;
    uint8_t is_enabled;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_width;
    egui_dim_t content_height;
    egui_dim_t footer_height = 0;
    uint8_t visible_count;
    uint8_t i;
    char count_text[5];
    char compact_badge[3] = {'L', 'K', '\0'};
    const char *header_text;
    const char *footer_text;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    visible_count = snapshot->visible_card_count;
    if (visible_count > EGUI_VIEW_NOTIFICATION_STACK_MAX_ITEMS)
    {
        visible_count = EGUI_VIEW_NOTIFICATION_STACK_MAX_ITEMS;
    }
    if (snapshot->card_titles == NULL || visible_count == 0)
    {
        return;
    }

    is_enabled = egui_view_get_enable(self) ? 1 : 0;
    panel_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_30);
    shadow_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_10);
    if (!is_enabled)
    {
        panel_color = egui_view_notification_stack_mix_disabled(panel_color);
        shadow_color = egui_view_notification_stack_mix_disabled(shadow_color);
    }

    egui_canvas_draw_round_rectangle_fill(region.location.x + 1, region.location.y + 2, region.size.width, region.size.height, 8, shadow_color,
                                          egui_color_alpha_mix(self->alpha, NOTIFY_PANEL_SHADOW_ALPHA));
    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, 8, panel_color,
                                          egui_color_alpha_mix(self->alpha, NOTIFY_PANEL_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, 8, 1, local->border_color,
                                     egui_color_alpha_mix(self->alpha, NOTIFY_PANEL_BORDER_ALPHA));
    if (NOTIFY_INNER_BORDER_ALPHA > 0)
    {
        egui_canvas_draw_round_rectangle(region.location.x + 2, region.location.y + 2, region.size.width - 4, region.size.height - 4, 6, 1,
                                         egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, NOTIFY_INNER_BORDER_ALPHA));
    }

    content_x = region.location.x + 6;
    content_y = region.location.y + 4;
    content_width = region.size.width - 12;
    content_height = region.size.height - 8;

    if (!local->show_header)
    {
        if (is_enabled)
        {
            compact_badge[0] = (local->current_snapshot == 0) ? 'A' : ((local->current_snapshot == 1) ? 'B' : 'C');
            compact_badge[1] = '\0';
        }
        if (NOTIFY_MINI_TOP_STRIP_ALPHA > 0)
        {
            egui_canvas_draw_round_rectangle_fill(region.location.x + 12, region.location.y + 6, region.size.width - 20, 2, 1,
                                                  is_enabled ? local->text_color : local->muted_text_color,
                                                  egui_color_alpha_mix(self->alpha, NOTIFY_MINI_TOP_STRIP_ALPHA));
        }
        header_region.location.x = content_x + (content_width - NOTIFY_MINI_BADGE_WIDTH) / 2;
        header_region.location.y = content_y + 6;
        header_region.size.width = NOTIFY_MINI_BADGE_WIDTH;
        header_region.size.height = 10;
        egui_canvas_draw_round_rectangle_fill(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 4,
                                              egui_rgb_mix(local->surface_color, local->border_color, EGUI_ALPHA_10),
                                              egui_color_alpha_mix(self->alpha, NOTIFY_MINI_BADGE_FILL_ALPHA));
        egui_canvas_draw_round_rectangle(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 4, 1,
                                         egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, NOTIFY_MINI_BADGE_BORDER_ALPHA));
        egui_canvas_draw_text_in_rect(local->font, compact_badge, &header_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color, self->alpha);
        content_y += 12;
        content_height -= 12;
    }

    if (local->show_header)
    {
        egui_dim_t pill_w;

        header_text = egui_view_notification_stack_get_header_text(local, is_enabled);
        pill_w = 34 + (egui_dim_t)strlen(header_text) * 6;
        if (pill_w < NOTIFY_HEADER_PILL_MIN_WIDTH)
        {
            pill_w = NOTIFY_HEADER_PILL_MIN_WIDTH;
        }
        if (pill_w > content_width)
        {
            pill_w = content_width;
        }
        header_region.location.x = content_x + (content_width - pill_w) / 2;
        header_region.location.y = content_y;
        header_region.size.width = pill_w;
        header_region.size.height = 14;
        egui_canvas_draw_round_rectangle_fill(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 6,
                                              egui_rgb_mix(local->surface_color, local->border_color, EGUI_ALPHA_10),
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));
        egui_canvas_draw_round_rectangle(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 6, 1,
                                         egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        egui_canvas_draw_text_in_rect(local->font, header_text, &header_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color, self->alpha);
        if (NOTIFY_HEADER_LINE_ALPHA > 0)
        {
            egui_canvas_draw_line(content_x + 5, content_y + 17, content_x + content_width - 6, content_y + 17, 1, local->border_color,
                                  egui_color_alpha_mix(self->alpha, NOTIFY_HEADER_LINE_ALPHA));
        }
        content_y += 20;
        content_height -= 20;
        footer_height = 14;
    }
    if (content_width <= 24 || content_height <= 24)
    {
        return;
    }
    if (footer_height > 0 && content_height > footer_height)
    {
        content_height -= footer_height;
    }

    count_text[0] = '+';
    count_text[1] = '0' + (snapshot->total_card_count / 10);
    count_text[2] = '0' + (snapshot->total_card_count % 10);
    count_text[3] = 0;
    if (count_text[1] == '0')
    {
        count_text[1] = count_text[2];
        count_text[2] = 0;
    }
    egui_view_notification_stack_draw_badge(
            local->font,
            region.location.x + region.size.width - 25,
            region.location.y + 7,
            18,
            count_text,
            is_enabled ? egui_rgb_mix(local->focus_color, EGUI_COLOR_WHITE, EGUI_ALPHA_20) : egui_view_notification_stack_mix_disabled(local->focus_color),
            local->text_color,
            egui_color_alpha_mix(self->alpha, EGUI_ALPHA_70));

    for (i = 0; i < visible_count; i++)
    {
        egui_dim_t card_x = content_x + i * (local->compact_mode ? 3 : 4);
        egui_dim_t card_y = content_y + i * (local->compact_mode ? 10 : 18);
        egui_dim_t card_width = content_width - i * (local->compact_mode ? 6 : 8);
        egui_dim_t card_height = local->compact_mode ? 16 : 28;
        egui_color_t sev_color = egui_view_notification_stack_get_severity_color(snapshot->severity_levels ? snapshot->severity_levels[i] : 0);
        egui_color_t fill_color;
        egui_color_t border_color;
        uint8_t is_focus = (i == snapshot->focus_card) || (i == local->focus_card);

        if (card_width <= 12 || card_y + card_height > content_y + content_height)
        {
            continue;
        }

        fill_color = egui_rgb_mix(local->surface_color, sev_color, is_focus ? EGUI_ALPHA_30 : EGUI_ALPHA_20);
        border_color = is_focus ? local->focus_color : egui_rgb_mix(local->border_color, sev_color, EGUI_ALPHA_30);
        if (!is_enabled)
        {
            fill_color = egui_view_notification_stack_mix_disabled(fill_color);
            border_color = egui_view_notification_stack_mix_disabled(border_color);
            sev_color = egui_view_notification_stack_mix_disabled(sev_color);
        }

        egui_canvas_draw_round_rectangle_fill(card_x, card_y, card_width, card_height, 5, fill_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40));
        egui_canvas_draw_round_rectangle(card_x, card_y, card_width, card_height, 5, is_focus ? 2 : 1, border_color,
                                         egui_color_alpha_mix(self->alpha, is_focus ? EGUI_ALPHA_80 : EGUI_ALPHA_50));
        egui_canvas_draw_round_rectangle_fill(card_x + 3, card_y + 3, 5, card_height - 6, 2, sev_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_70));

        if (!local->compact_mode && snapshot->card_tags != NULL && is_focus)
        {
            egui_view_notification_stack_draw_badge(
                    local->font,
                    card_x + card_width - 26,
                    card_y + 4,
                    22,
                    snapshot->card_tags[i],
                    egui_rgb_mix(sev_color, EGUI_COLOR_WHITE, EGUI_ALPHA_20),
                    local->text_color,
                    egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));
        }

        text_region.location.x = card_x + 12;
        text_region.location.y = card_y + 2;
        text_region.size.width = card_width - (local->compact_mode ? 16 : (is_focus ? 42 : 18));
        text_region.size.height = card_height - 4;
        egui_canvas_draw_text_in_rect(local->font, snapshot->card_titles[i], &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                                      is_enabled ? local->text_color : local->muted_text_color, self->alpha);
    }

    if (!is_enabled)
    {
        egui_canvas_draw_round_rectangle_fill(content_x + 1, content_y + 1, content_width - 2, content_height - 2, 4, EGUI_COLOR_BLACK,
                                              egui_color_alpha_mix(self->alpha, NOTIFY_DISABLED_OVERLAY_ALPHA));
        egui_canvas_draw_line(content_x + 2, content_y + 2, content_x + content_width - 3, content_y + content_height - 3, 1, local->muted_text_color,
                              egui_color_alpha_mix(self->alpha, NOTIFY_DISABLED_CROSS_ALPHA));
        egui_canvas_draw_line(content_x + 2, content_y + content_height - 3, content_x + content_width - 3, content_y + 2, 1, local->muted_text_color,
                              egui_color_alpha_mix(self->alpha, NOTIFY_DISABLED_CROSS_ALPHA));
    }

    if (local->show_header)
    {
        footer_text = egui_view_notification_stack_get_footer_text(local, is_enabled);
        text_region.location.x = content_x;
        text_region.location.y = content_y + content_height + 3;
        text_region.size.width = content_width;
        text_region.size.height = 11;
        egui_canvas_draw_round_rectangle_fill(content_x + (content_width - NOTIFY_FOOTER_WIDTH) / 2, content_y + content_height + 2, NOTIFY_FOOTER_WIDTH, 11, 5, panel_color,
                                              egui_color_alpha_mix(self->alpha, NOTIFY_FOOTER_FILL_ALPHA));
        egui_canvas_draw_round_rectangle(content_x + (content_width - NOTIFY_FOOTER_WIDTH) / 2, content_y + content_height + 2, NOTIFY_FOOTER_WIDTH, 11, 5, 1,
                                         egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, NOTIFY_FOOTER_BORDER_ALPHA));
        egui_canvas_draw_text_in_rect(local->font, footer_text, &text_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color, self->alpha);
    }
    else if (NOTIFY_MINI_BOTTOM_STRIP_ALPHA > 0)
    {
        egui_canvas_draw_round_rectangle_fill(content_x + 8, region.location.y + region.size.height - 8, content_width - 14, 2, 1,
                                              is_enabled ? local->text_color : local->muted_text_color,
                                              egui_color_alpha_mix(self->alpha, NOTIFY_MINI_BOTTOM_STRIP_ALPHA));
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_notification_stack_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_notification_stack_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_notification_stack_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_notification_stack_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_notification_stack_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x101723);
    local->border_color = EGUI_COLOR_HEX(0x4A5568);
    local->text_color = EGUI_COLOR_HEX(0xE2E8F0);
    local->muted_text_color = EGUI_COLOR_HEX(0x94A3B8);
    local->focus_color = EGUI_COLOR_HEX(0x38BDF8);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->focus_card = 0;
    local->show_header = 1;
    local->compact_mode = 0;
}
