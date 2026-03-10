#define TREEMAP_PANEL_SHADOW_ALPHA 20
#define TREEMAP_PANEL_FILL_ALPHA 32
#define TREEMAP_PANEL_BORDER_ALPHA 52
#define TREEMAP_INNER_BORDER_ALPHA 22
#define TREEMAP_HEADER_MODE 2
#define TREEMAP_HEADER_PILL_MIN_WIDTH 80
#define TREEMAP_HEADER_LINE_ALPHA 22
#define TREEMAP_FOOTER_MODE 2
#define TREEMAP_FOOTER_WIDTH 76
#define TREEMAP_FOOTER_FILL_ALPHA 46
#define TREEMAP_FOOTER_BORDER_ALPHA 24
#define TREEMAP_MINI_BADGE_MODE 2
#define TREEMAP_MINI_BADGE_WIDTH 28
#define TREEMAP_MINI_BADGE_FILL_ALPHA 70
#define TREEMAP_MINI_BADGE_BORDER_ALPHA 20
#define TREEMAP_MINI_TOP_STRIP_ALPHA 20
#define TREEMAP_MINI_BOTTOM_STRIP_ALPHA 20
#define TREEMAP_DISABLED_OVERLAY_ALPHA 14
#define TREEMAP_DISABLED_CROSS_ALPHA 6
#define TREEMAP_DISABLED_MIX_ALPHA 70
#include <stdlib.h>
#include <string.h>

#include "egui_view_treemap_chart.h"

typedef struct egui_view_treemap_chart_region egui_view_treemap_chart_region_t;
struct egui_view_treemap_chart_region
{
    egui_dim_t x;
    egui_dim_t y;
    egui_dim_t width;
    egui_dim_t height;
};

static uint8_t egui_view_treemap_chart_clamp_item_count(uint8_t count)
{
    if (count > EGUI_VIEW_TREEMAP_CHART_MAX_ITEMS)
    {
        return EGUI_VIEW_TREEMAP_CHART_MAX_ITEMS;
    }
    return count;
}

static uint8_t egui_view_treemap_chart_clamp_value_set_count(uint8_t count)
{
    if (count > EGUI_VIEW_TREEMAP_CHART_MAX_VALUE_SETS)
    {
        return EGUI_VIEW_TREEMAP_CHART_MAX_VALUE_SETS;
    }
    return count;
}

static egui_color_t egui_view_treemap_chart_get_item_color(uint8_t index)
{
    static const egui_color_t palette[] = {
            EGUI_COLOR_HEX(0x38BDF8),
            EGUI_COLOR_HEX(0x22C55E),
            EGUI_COLOR_HEX(0xF59E0B),
            EGUI_COLOR_HEX(0xA855F7),
            EGUI_COLOR_HEX(0xF43F5E),
            EGUI_COLOR_HEX(0x14B8A6),
    };
    return palette[index % (sizeof(palette) / sizeof(palette[0]))];
}

static egui_color_t egui_view_treemap_chart_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, TREEMAP_DISABLED_MIX_ALPHA);
}

static const char *egui_view_treemap_chart_get_header_text(egui_view_treemap_chart_t *local, uint8_t is_enabled)
{
    if (!is_enabled)
    {
        return "Locked";
    }
    return (local->current_value_set == 0) ? "Profile A" : ((local->current_value_set == 1) ? "Profile B" : "Profile C");
}

static const char *egui_view_treemap_chart_get_footer_text(egui_view_treemap_chart_t *local)
{
    return (local->current_value_set == 0) ? "Load A" : ((local->current_value_set == 1) ? "Load B" : "Load C");
}

static uint32_t egui_view_treemap_chart_get_total(const uint8_t *values, uint8_t count)
{
    uint32_t total = 0;
    uint8_t i;

    for (i = 0; i < count; i++)
    {
        total += values[i];
    }
    return total;
}

static void egui_view_treemap_chart_draw_item(
        egui_view_treemap_chart_t *local,
        uint8_t item_index,
        egui_view_treemap_chart_region_t region,
        egui_alpha_t alpha,
        uint8_t is_enabled)
{
    egui_color_t base_color = egui_view_treemap_chart_get_item_color(item_index);
    egui_color_t fill_color;
    egui_region_t text_region;
    const char *label = (local->item_labels != NULL && local->item_labels[item_index] != NULL) ? local->item_labels[item_index] : "";
    const char *draw_label = label;
    char short_label[5] = {0};
    uint8_t len = 0;

    if (!is_enabled)
    {
        base_color = egui_view_treemap_chart_mix_disabled(base_color);
    }
    fill_color = egui_rgb_mix(local->surface_color, base_color, EGUI_ALPHA_80);

    while (label[len] != 0 && len < 4)
    {
        short_label[len] = label[len];
        len++;
    }
    short_label[len] = 0;

    egui_canvas_draw_round_rectangle_fill(region.x, region.y, region.width, region.height, 5, fill_color, alpha);
    egui_canvas_draw_round_rectangle(region.x, region.y, region.width, region.height, 5, 1, local->border_color, egui_color_alpha_mix(alpha, EGUI_ALPHA_60));

    if (!local->show_labels || region.width < 24 || region.height < 14)
    {
        return;
    }

    text_region.location.x = region.x + 3;
    text_region.location.y = region.y + 2;
    text_region.size.width = region.width - 6;
    text_region.size.height = region.height - 4;

    if (region.width < 34)
    {
        return;
    }
    if (region.width < 52 && len >= 3)
    {
        short_label[3] = 0;
        draw_label = short_label;
    }
    egui_canvas_draw_text_in_rect(local->font, draw_label, &text_region, EGUI_ALIGN_CENTER, local->text_color, alpha);
}

static void egui_view_treemap_chart_draw_items(
        egui_view_treemap_chart_t *local,
        const uint8_t *values,
        uint8_t start_index,
        uint8_t count,
        egui_view_treemap_chart_region_t region,
        uint8_t split_vertical,
        egui_alpha_t alpha,
        uint8_t is_enabled)
{
    uint32_t total = egui_view_treemap_chart_get_total(values + start_index, count);
    egui_view_treemap_chart_region_t current = region;
    egui_view_treemap_chart_region_t remaining = region;
    uint8_t item_index = start_index;
    uint32_t value;

    if (count == 0 || total == 0)
    {
        return;
    }

    if (count == 1)
    {
        egui_view_treemap_chart_draw_item(local, item_index, region, alpha, is_enabled);
        return;
    }

    value = values[item_index];
    if (split_vertical)
    {
        current.width = (egui_dim_t)((int32_t)region.width * value / total);
        if (current.width < 14)
        {
            current.width = 14;
        }
        if (current.width > region.width - 12)
        {
            current.width = region.width - 12;
        }
        remaining.x += current.width;
        remaining.width -= current.width;
    }
    else
    {
        current.height = (egui_dim_t)((int32_t)region.height * value / total);
        if (current.height < 14)
        {
            current.height = 14;
        }
        if (current.height > region.height - 12)
        {
            current.height = region.height - 12;
        }
        remaining.y += current.height;
        remaining.height -= current.height;
    }

    if (current.width > 2 && current.height > 2)
    {
        egui_view_treemap_chart_draw_item(local, item_index, current, alpha, is_enabled);
    }
    if (remaining.width > 2 && remaining.height > 2)
    {
        egui_view_treemap_chart_draw_items(local, values, start_index + 1, count - 1, remaining, !split_vertical, alpha, is_enabled);
    }
}

void egui_view_treemap_chart_set_item_labels(egui_view_t *self, const char **item_labels, uint8_t item_count)
{
    EGUI_LOCAL_INIT(egui_view_treemap_chart_t);
    local->item_labels = item_labels;
    local->item_count = egui_view_treemap_chart_clamp_item_count(item_count);
    if (local->current_value_set >= local->value_set_count)
    {
        local->current_value_set = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_treemap_chart_set_value_set(egui_view_t *self, uint8_t set_index, const uint8_t *values)
{
    EGUI_LOCAL_INIT(egui_view_treemap_chart_t);
    if (set_index >= EGUI_VIEW_TREEMAP_CHART_MAX_VALUE_SETS)
    {
        return;
    }
    local->value_sets[set_index] = values;
    if (local->value_set_count <= set_index)
    {
        local->value_set_count = set_index + 1;
    }
    local->value_set_count = egui_view_treemap_chart_clamp_value_set_count(local->value_set_count);
    egui_view_invalidate(self);
}

void egui_view_treemap_chart_set_current_value_set(egui_view_t *self, uint8_t set_index)
{
    EGUI_LOCAL_INIT(egui_view_treemap_chart_t);
    if (local->value_set_count == 0 || set_index >= local->value_set_count)
    {
        return;
    }
    if (local->current_value_set == set_index)
    {
        return;
    }
    local->current_value_set = set_index;
    egui_view_invalidate(self);
}

uint8_t egui_view_treemap_chart_get_current_value_set(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_treemap_chart_t);
    return local->current_value_set;
}

void egui_view_treemap_chart_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_treemap_chart_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_treemap_chart_set_show_labels(egui_view_t *self, uint8_t show_labels)
{
    EGUI_LOCAL_INIT(egui_view_treemap_chart_t);
    local->show_labels = show_labels ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_treemap_chart_set_show_header(egui_view_t *self, uint8_t show_header)
{
    EGUI_LOCAL_INIT(egui_view_treemap_chart_t);
    local->show_header = show_header ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_treemap_chart_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color, egui_color_t muted_text_color)
{
    EGUI_LOCAL_INIT(egui_view_treemap_chart_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    egui_view_invalidate(self);
}

static void egui_view_treemap_chart_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_treemap_chart_t);
    egui_region_t region;
    egui_view_treemap_chart_region_t content;
    const uint8_t *values;
    egui_region_t text_region;
    egui_region_t header_region;
    egui_dim_t pill_w;
    egui_dim_t footer_x;
    const char *header_text;
    const char *footer_text;
    egui_color_t panel_color;
    egui_color_t shadow_color;
    uint8_t is_enabled;
    char compact_badge[3] = {'L', 'K', '\0'};

    egui_view_get_work_region(self, &region);
    if (local->item_count == 0 || local->item_count > EGUI_VIEW_TREEMAP_CHART_MAX_ITEMS)
    {
        return;
    }
    if (region.size.width <= 0 || region.size.height <= 0)
    {
        return;
    }

    values = local->value_sets[local->current_value_set];
    if (values == NULL)
    {
        return;
    }
    is_enabled = egui_view_get_enable(self) ? 1 : 0;

    panel_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_40);
    shadow_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_10);
    if (!is_enabled)
    {
        panel_color = egui_view_treemap_chart_mix_disabled(panel_color);
        shadow_color = egui_view_treemap_chart_mix_disabled(shadow_color);
    }

    if (TREEMAP_PANEL_SHADOW_ALPHA > 0)
    {
        egui_canvas_draw_round_rectangle_fill(region.location.x + 1, region.location.y + 2, region.size.width, region.size.height, 8, shadow_color,
                                              egui_color_alpha_mix(self->alpha, TREEMAP_PANEL_SHADOW_ALPHA));
    }
    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, 8, panel_color,
                                          egui_color_alpha_mix(self->alpha, TREEMAP_PANEL_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, 8, 1, local->border_color,
                                     egui_color_alpha_mix(self->alpha, TREEMAP_PANEL_BORDER_ALPHA));
    if (TREEMAP_INNER_BORDER_ALPHA > 0)
    {
        egui_canvas_draw_round_rectangle(region.location.x + 2, region.location.y + 2, region.size.width - 4, region.size.height - 4, 6, 1,
                                         egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20), egui_color_alpha_mix(self->alpha, TREEMAP_INNER_BORDER_ALPHA));
    }

    content.x = region.location.x + 4;
    content.y = region.location.y + 4;
    content.width = region.size.width - 8;
    content.height = region.size.height - 8;

    if (!local->show_header)
    {
        if (is_enabled)
        {
            compact_badge[0] = (local->current_value_set == 0) ? 'A' : ((local->current_value_set == 1) ? 'B' : 'C');
            compact_badge[1] = '\0';
        }
        if (TREEMAP_MINI_TOP_STRIP_ALPHA > 0)
        {
            egui_canvas_draw_round_rectangle_fill(region.location.x + 12, region.location.y + 6, region.size.width - 20, 2, 1,
                                                  is_enabled ? local->text_color : local->muted_text_color,
                                                  egui_color_alpha_mix(self->alpha, TREEMAP_MINI_TOP_STRIP_ALPHA));
        }
        if (TREEMAP_MINI_BADGE_MODE > 0)
        {
            header_region.location.x = content.x + (content.width - TREEMAP_MINI_BADGE_WIDTH) / 2;
            header_region.location.y = content.y + 6;
            header_region.size.width = TREEMAP_MINI_BADGE_WIDTH;
            header_region.size.height = 10;
            if (TREEMAP_MINI_BADGE_MODE > 1)
            {
                egui_canvas_draw_round_rectangle_fill(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 4,
                                                      egui_rgb_mix(local->surface_color, local->border_color, EGUI_ALPHA_10),
                                                      egui_color_alpha_mix(self->alpha, TREEMAP_MINI_BADGE_FILL_ALPHA));
                egui_canvas_draw_round_rectangle(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 4, 1,
                                                 egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                                                 egui_color_alpha_mix(self->alpha, TREEMAP_MINI_BADGE_BORDER_ALPHA));
            }
            egui_canvas_draw_text_in_rect(local->font, compact_badge, &header_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color, self->alpha);
            content.y += 12;
            content.height -= 12;
        }
    }

    if (local->show_header)
    {
        header_text = egui_view_treemap_chart_get_header_text(local, is_enabled);
        pill_w = 34 + (egui_dim_t)strlen(header_text) * 6;
        if (pill_w < TREEMAP_HEADER_PILL_MIN_WIDTH)
        {
            pill_w = TREEMAP_HEADER_PILL_MIN_WIDTH;
        }
        if (pill_w > content.width)
        {
            pill_w = content.width;
        }

        header_region.location.x = (TREEMAP_HEADER_MODE == 0) ? (content.x + 4) : (content.x + (content.width - pill_w) / 2);
        header_region.location.y = content.y;
        header_region.size.width = (TREEMAP_HEADER_MODE == 0) ? (content.width - 8) : pill_w;
        header_region.size.height = 14;

        if (TREEMAP_HEADER_MODE == 2)
        {
            egui_canvas_draw_round_rectangle_fill(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 6,
                                                  egui_rgb_mix(local->surface_color, local->border_color, EGUI_ALPHA_10), egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));
            egui_canvas_draw_round_rectangle(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 6, 1,
                                             egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20), egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        }

        egui_canvas_draw_text_in_rect(local->font, header_text, &header_region, (TREEMAP_HEADER_MODE == 0) ? EGUI_ALIGN_LEFT : EGUI_ALIGN_CENTER,
                                      is_enabled ? local->text_color : local->muted_text_color, self->alpha);
        if (TREEMAP_HEADER_LINE_ALPHA > 0)
        {
            egui_canvas_draw_line(content.x + 5, content.y + 17, content.x + content.width - 6, content.y + 17, 1, local->border_color,
                                  egui_color_alpha_mix(self->alpha, TREEMAP_HEADER_LINE_ALPHA));
        }
        content.y += 20;
        content.height -= 20;
        if (TREEMAP_FOOTER_MODE > 0 && content.height > 14)
        {
            content.height -= 14;
        }
    }

    if (content.width <= 6 || content.height <= 6)
    {
        return;
    }

    egui_view_treemap_chart_draw_items(local, values, 0, local->item_count, content, 1, self->alpha, is_enabled);

    if (!local->show_header && TREEMAP_MINI_BOTTOM_STRIP_ALPHA > 0)
    {
        egui_canvas_draw_round_rectangle_fill(content.x + 10, region.location.y + region.size.height - 8, content.width - 16, 2, 1,
                                              is_enabled ? local->text_color : local->muted_text_color,
                                              egui_color_alpha_mix(self->alpha, TREEMAP_MINI_BOTTOM_STRIP_ALPHA));
    }

    if (!is_enabled)
    {
        if (TREEMAP_DISABLED_OVERLAY_ALPHA > 0)
        {
            egui_canvas_draw_round_rectangle_fill(content.x + 1, content.y + 1, content.width - 2, content.height - 2, 4, EGUI_COLOR_BLACK,
                                                  egui_color_alpha_mix(self->alpha, TREEMAP_DISABLED_OVERLAY_ALPHA));
        }
        if (TREEMAP_DISABLED_CROSS_ALPHA > 0)
        {
            egui_canvas_draw_line(content.x + 8, content.y + 7, content.x + content.width - 9, content.y + content.height - 8, 1, local->muted_text_color,
                                  egui_color_alpha_mix(self->alpha, TREEMAP_DISABLED_CROSS_ALPHA));
            egui_canvas_draw_line(content.x + 8, content.y + content.height - 8, content.x + content.width - 9, content.y + 7, 1, local->muted_text_color,
                                  egui_color_alpha_mix(self->alpha, TREEMAP_DISABLED_CROSS_ALPHA));
        }
    }

    if (local->show_header && TREEMAP_FOOTER_MODE > 0)
    {
        footer_text = egui_view_treemap_chart_get_footer_text(local);
        footer_x = content.x + (content.width - TREEMAP_FOOTER_WIDTH) / 2;
        text_region.location.x = content.x;
        text_region.location.y = content.y + content.height + 3;
        text_region.size.width = content.width;
        text_region.size.height = 11;
        if (TREEMAP_FOOTER_MODE > 1)
        {
            egui_canvas_draw_round_rectangle_fill(footer_x, content.y + content.height + 2, TREEMAP_FOOTER_WIDTH, 11, 5, panel_color,
                                                  egui_color_alpha_mix(self->alpha, TREEMAP_FOOTER_FILL_ALPHA));
            egui_canvas_draw_round_rectangle(footer_x, content.y + content.height + 2, TREEMAP_FOOTER_WIDTH, 11, 5, 1,
                                             egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                                             egui_color_alpha_mix(self->alpha, TREEMAP_FOOTER_BORDER_ALPHA));
        }
        egui_canvas_draw_text_in_rect(local->font, footer_text, &text_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color, self->alpha);
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_treemap_chart_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_treemap_chart_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_treemap_chart_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_treemap_chart_t);
    uint8_t i;

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_treemap_chart_t);
    egui_view_set_padding_all(self, 2);

    local->item_labels = NULL;
    for (i = 0; i < EGUI_VIEW_TREEMAP_CHART_MAX_VALUE_SETS; i++)
    {
        local->value_sets[i] = NULL;
    }
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x0F172A);
    local->border_color = EGUI_COLOR_HEX(0x475569);
    local->text_color = EGUI_COLOR_HEX(0xE2E8F0);
    local->muted_text_color = EGUI_COLOR_HEX(0x94A3B8);
    local->item_count = 0;
    local->value_set_count = 0;
    local->current_value_set = 0;
    local->show_labels = 1;
    local->show_header = 1;
}
