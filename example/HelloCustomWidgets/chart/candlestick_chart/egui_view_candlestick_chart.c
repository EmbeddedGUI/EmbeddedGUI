#define CANDLE_PANEL_SHADOW_ALPHA 20
#define CANDLE_PANEL_FILL_ALPHA 32
#define CANDLE_PANEL_BORDER_ALPHA 52
#define CANDLE_INNER_BORDER_ALPHA 20
#define CANDLE_HEADER_PILL_MIN_WIDTH 82
#define CANDLE_HEADER_LINE_ALPHA 22
#define CANDLE_FOOTER_WIDTH 88
#define CANDLE_FOOTER_FILL_ALPHA 44
#define CANDLE_FOOTER_BORDER_ALPHA 22
#define CANDLE_MINI_BADGE_WIDTH 30
#define CANDLE_MINI_BADGE_FILL_ALPHA 70
#define CANDLE_MINI_BADGE_BORDER_ALPHA 20
#define CANDLE_MINI_TOP_STRIP_ALPHA 18
#define CANDLE_MINI_BOTTOM_STRIP_ALPHA 18
#define CANDLE_DISABLED_OVERLAY_ALPHA 14
#define CANDLE_DISABLED_CROSS_ALPHA 12
#define CANDLE_DISABLED_MIX_ALPHA 70

#include <stdlib.h>
#include <string.h>

#include "egui_view_candlestick_chart.h"

static uint8_t egui_view_candlestick_chart_clamp_item_count(uint8_t count)
{
    if (count > EGUI_VIEW_CANDLESTICK_CHART_MAX_ITEMS)
    {
        return EGUI_VIEW_CANDLESTICK_CHART_MAX_ITEMS;
    }
    return count;
}

static uint8_t egui_view_candlestick_chart_clamp_value_set_count(uint8_t count)
{
    if (count > EGUI_VIEW_CANDLESTICK_CHART_MAX_VALUE_SETS)
    {
        return EGUI_VIEW_CANDLESTICK_CHART_MAX_VALUE_SETS;
    }
    return count;
}

static egui_color_t egui_view_candlestick_chart_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, CANDLE_DISABLED_MIX_ALPHA);
}

static const char *egui_view_candlestick_chart_get_header_text(egui_view_candlestick_chart_t *local, uint8_t is_enabled)
{
    if (!is_enabled)
    {
        return "Locked";
    }
    return (local->current_value_set == 0) ? "Series A" : ((local->current_value_set == 1) ? "Series B" : "Series C");
}

static const char *egui_view_candlestick_chart_get_footer_text(
        const uint8_t *open_values,
        const uint8_t *close_values,
        uint8_t item_count)
{
    uint8_t first_open;
    uint8_t last_close;

    if (open_values == NULL || close_values == NULL || item_count == 0)
    {
        return "Trend --";
    }
    first_open = open_values[0];
    last_close = close_values[item_count - 1];
    if (last_close > first_open)
    {
        return "Trend Up";
    }
    if (last_close < first_open)
    {
        return "Trend Down";
    }
    return "Trend Flat";
}

static void egui_view_candlestick_chart_format_value(uint8_t value, char out_text[4])
{
    if (value > 99)
    {
        out_text[0] = '1';
        out_text[1] = '0';
        out_text[2] = '0';
        out_text[3] = 0;
        return;
    }
    if (value > 9)
    {
        out_text[0] = (char)('0' + (value / 10));
        out_text[1] = (char)('0' + (value % 10));
        out_text[2] = 0;
        return;
    }
    out_text[0] = (char)('0' + value);
    out_text[1] = 0;
}

static egui_dim_t egui_view_candlestick_chart_value_to_y(uint8_t value, uint8_t min_value, uint8_t max_value, egui_dim_t top, egui_dim_t height)
{
    int32_t range = (int32_t)max_value - (int32_t)min_value;
    if (height <= 1 || range <= 0)
    {
        return top + (height / 2);
    }
    return (egui_dim_t)(top + (((int32_t)max_value - (int32_t)value) * (height - 1)) / range);
}

static void egui_view_candlestick_chart_find_bounds(
        const uint8_t *open_values,
        const uint8_t *high_values,
        const uint8_t *low_values,
        const uint8_t *close_values,
        uint8_t count,
        uint8_t *out_min_value,
        uint8_t *out_max_value)
{
    uint8_t i;
    uint8_t min_value = 100;
    uint8_t max_value = 0;

    for (i = 0; i < count; i++)
    {
        uint8_t o = open_values[i];
        uint8_t h = high_values[i];
        uint8_t l = low_values[i];
        uint8_t c = close_values[i];
        if (h < o)
        {
            h = o;
        }
        if (h < c)
        {
            h = c;
        }
        if (l > o)
        {
            l = o;
        }
        if (l > c)
        {
            l = c;
        }
        if (l < min_value)
        {
            min_value = l;
        }
        if (h > max_value)
        {
            max_value = h;
        }
    }
    if (max_value <= min_value)
    {
        if (max_value < 100)
        {
            max_value++;
        }
        else if (min_value > 0)
        {
            min_value--;
        }
    }
    *out_min_value = min_value;
    *out_max_value = max_value;
}

static uint8_t egui_view_candlestick_chart_find_max_range_index(
        const uint8_t *high_values,
        const uint8_t *low_values,
        uint8_t count,
        uint8_t *out_range_value)
{
    uint8_t i;
    uint8_t max_index = 0;
    uint8_t max_range = 0;

    if (high_values == NULL || low_values == NULL || count == 0)
    {
        if (out_range_value != NULL)
        {
            *out_range_value = 0;
        }
        return 0;
    }

    for (i = 0; i < count; i++)
    {
        uint8_t h = high_values[i];
        uint8_t l = low_values[i];
        uint8_t range = (h >= l) ? (uint8_t)(h - l) : (uint8_t)(l - h);
        if (range > max_range)
        {
            max_range = range;
            max_index = i;
        }
    }
    if (out_range_value != NULL)
    {
        *out_range_value = max_range;
    }
    return max_index;
}

static void egui_view_candlestick_chart_draw_labels(
        egui_view_candlestick_chart_t *local,
        egui_region_t label_region,
        egui_alpha_t alpha)
{
    uint8_t i;
    egui_region_t text_region;
    if (!local->show_labels || local->item_labels == NULL || label_region.size.height <= 6)
    {
        return;
    }
    for (i = 0; i < local->item_count; i++)
    {
        egui_dim_t x0 = label_region.location.x + (egui_dim_t)(((int32_t)label_region.size.width * i) / local->item_count);
        egui_dim_t x1 = label_region.location.x + (egui_dim_t)(((int32_t)label_region.size.width * (i + 1)) / local->item_count);
        text_region.location.x = x0;
        text_region.location.y = label_region.location.y;
        text_region.size.width = x1 - x0;
        text_region.size.height = label_region.size.height;
        if (text_region.size.width < 7)
        {
            continue;
        }
        egui_canvas_draw_text_in_rect(local->font, local->item_labels[i], &text_region, EGUI_ALIGN_CENTER, local->muted_text_color, alpha);
    }
}

static void egui_view_candlestick_chart_draw_candle(
        egui_view_candlestick_chart_t *local,
        uint8_t open_value,
        uint8_t high_value,
        uint8_t low_value,
        uint8_t close_value,
        uint8_t is_latest,
        uint8_t min_value,
        uint8_t max_value,
        egui_dim_t chart_top,
        egui_dim_t chart_height,
        egui_dim_t x0,
        egui_dim_t x1,
        egui_alpha_t alpha,
        uint8_t is_enabled)
{
    egui_dim_t center_x;
    egui_dim_t open_y;
    egui_dim_t close_y;
    egui_dim_t high_y;
    egui_dim_t low_y;
    egui_dim_t body_x;
    egui_dim_t body_y;
    egui_dim_t body_width;
    egui_dim_t body_height;
    egui_color_t candle_color;
    egui_color_t border_color;

    if (x1 <= x0 + 1)
    {
        return;
    }
    if (high_value < open_value)
    {
        high_value = open_value;
    }
    if (high_value < close_value)
    {
        high_value = close_value;
    }
    if (low_value > open_value)
    {
        low_value = open_value;
    }
    if (low_value > close_value)
    {
        low_value = close_value;
    }

    if (close_value > open_value)
    {
        candle_color = local->rise_color;
    }
    else if (close_value < open_value)
    {
        candle_color = local->fall_color;
    }
    else
    {
        candle_color = local->neutral_color;
    }
    if (!is_enabled)
    {
        candle_color = egui_view_candlestick_chart_mix_disabled(candle_color);
    }
    border_color = egui_rgb_mix(candle_color, local->border_color, EGUI_ALPHA_40);

    center_x = (egui_dim_t)((x0 + x1) / 2);
    open_y = egui_view_candlestick_chart_value_to_y(open_value, min_value, max_value, chart_top, chart_height);
    close_y = egui_view_candlestick_chart_value_to_y(close_value, min_value, max_value, chart_top, chart_height);
    high_y = egui_view_candlestick_chart_value_to_y(high_value, min_value, max_value, chart_top, chart_height);
    low_y = egui_view_candlestick_chart_value_to_y(low_value, min_value, max_value, chart_top, chart_height);

    egui_canvas_draw_line(center_x, high_y, center_x, low_y, 1, border_color, egui_color_alpha_mix(alpha, EGUI_ALPHA_60));
    egui_canvas_draw_line(center_x - 2, high_y, center_x + 2, high_y, 1, border_color, egui_color_alpha_mix(alpha, EGUI_ALPHA_50));
    egui_canvas_draw_line(center_x - 2, low_y, center_x + 2, low_y, 1, border_color, egui_color_alpha_mix(alpha, EGUI_ALPHA_50));

    body_x = x0 + 2;
    body_width = x1 - x0 - 4;
    if (body_width < 2)
    {
        body_width = 2;
        body_x = center_x - 1;
    }
    body_y = open_y < close_y ? open_y : close_y;
    body_height = (egui_dim_t)abs(close_y - open_y) + 1;
    if (body_height < 3)
    {
        body_height = 3;
        body_y = body_y - 1;
    }
    if (body_y < chart_top)
    {
        body_y = chart_top;
    }
    egui_canvas_draw_round_rectangle_fill(body_x, body_y, body_width, body_height, 2, candle_color, alpha);
    egui_canvas_draw_round_rectangle(body_x, body_y, body_width, body_height, 2, 1, border_color, egui_color_alpha_mix(alpha, EGUI_ALPHA_70));
    if (is_latest && is_enabled)
    {
        egui_canvas_draw_round_rectangle(
                body_x - 1,
                body_y - 1,
                body_width + 2,
                body_height + 2,
                3,
                1,
                candle_color,
                egui_color_alpha_mix(alpha, EGUI_ALPHA_60));
        egui_dim_t close_marker_y = close_y;
        egui_canvas_draw_circle_fill(center_x, close_marker_y, 2, candle_color, egui_color_alpha_mix(alpha, EGUI_ALPHA_90));
        egui_canvas_draw_line(
                center_x + 3,
                close_marker_y,
                x1 - 1,
                close_marker_y,
                1,
                border_color,
                egui_color_alpha_mix(alpha, EGUI_ALPHA_50));
    }
}

static void egui_view_candlestick_chart_draw_close_path(
        egui_view_candlestick_chart_t *local,
        const uint8_t *close_values,
        uint8_t count,
        uint8_t min_value,
        uint8_t max_value,
        egui_region_t chart_region,
        egui_alpha_t alpha)
{
    uint8_t i;

    if (close_values == NULL || count < 2)
    {
        return;
    }
    if (chart_region.size.width < 120)
    {
        return;
    }
    for (i = 1; i < count; i++)
    {
        egui_dim_t x0 = chart_region.location.x + (egui_dim_t)(((int32_t)chart_region.size.width * (i - 1)) / count);
        egui_dim_t x1 = chart_region.location.x + (egui_dim_t)(((int32_t)chart_region.size.width * i) / count);
        egui_dim_t x2 = chart_region.location.x + (egui_dim_t)(((int32_t)chart_region.size.width * (i + 1)) / count);
        egui_dim_t c0 = (egui_dim_t)((x0 + x1) / 2);
        egui_dim_t c1 = (egui_dim_t)((x1 + x2) / 2);
        egui_dim_t y0 = egui_view_candlestick_chart_value_to_y(close_values[i - 1], min_value, max_value, chart_region.location.y + 1, chart_region.size.height - 2);
        egui_dim_t y1 = egui_view_candlestick_chart_value_to_y(close_values[i], min_value, max_value, chart_region.location.y + 1, chart_region.size.height - 2);
        egui_color_t path_color = (close_values[i] >= close_values[i - 1]) ? local->rise_color : local->fall_color;
        path_color = egui_rgb_mix(path_color, local->muted_text_color, EGUI_ALPHA_40);
        egui_canvas_draw_line(c0, y0, c1, y1, 1, path_color, egui_color_alpha_mix(alpha, EGUI_ALPHA_40));
    }
}

static void egui_view_candlestick_chart_draw_reference_lines(
        egui_view_candlestick_chart_t *local,
        egui_region_t chart_region,
        egui_alpha_t alpha,
        uint8_t is_enabled)
{
    egui_color_t line_color;
    egui_dim_t top_y;
    egui_dim_t mid_y;
    egui_dim_t bottom_y;

    if (chart_region.size.width <= 4 || chart_region.size.height <= 4)
    {
        return;
    }
    line_color = is_enabled ? egui_rgb_mix(local->muted_text_color, local->border_color, EGUI_ALPHA_40)
                            : egui_rgb_mix(local->muted_text_color, local->border_color, EGUI_ALPHA_30);
    top_y = chart_region.location.y + 1;
    mid_y = chart_region.location.y + chart_region.size.height / 2;
    bottom_y = chart_region.location.y + chart_region.size.height - 2;

    egui_canvas_draw_line(
            chart_region.location.x + 1,
            top_y,
            chart_region.location.x + chart_region.size.width - 2,
            top_y,
            1,
            line_color,
            egui_color_alpha_mix(alpha, EGUI_ALPHA_30));
    egui_canvas_draw_line(
            chart_region.location.x + 1,
            mid_y,
            chart_region.location.x + chart_region.size.width - 2,
            mid_y,
            1,
            line_color,
            egui_color_alpha_mix(alpha, EGUI_ALPHA_30));
    egui_canvas_draw_line(
            chart_region.location.x + 1,
            bottom_y,
            chart_region.location.x + chart_region.size.width - 2,
            bottom_y,
            1,
            line_color,
            egui_color_alpha_mix(alpha, EGUI_ALPHA_30));
}

static void egui_view_candlestick_chart_draw_bound_labels(
        egui_view_candlestick_chart_t *local,
        egui_region_t chart_region,
        uint8_t min_value,
        uint8_t max_value,
        egui_alpha_t alpha)
{
    egui_region_t text_region;
    char max_text[4] = {0};
    char min_text[4] = {0};

    if (chart_region.size.width < 42 || chart_region.size.height < 24)
    {
        return;
    }
    if (chart_region.size.width < 120)
    {
        return;
    }
    egui_view_candlestick_chart_format_value(max_value, max_text);
    egui_view_candlestick_chart_format_value(min_value, min_text);

    text_region.location.x = chart_region.location.x + 1;
    text_region.location.y = chart_region.location.y + 1;
    text_region.size.width = 16;
    text_region.size.height = 8;
    egui_canvas_draw_text_in_rect(local->font, max_text, &text_region, EGUI_ALIGN_LEFT, local->muted_text_color, alpha);

    text_region.location.y = chart_region.location.y + chart_region.size.height - 9;
    egui_canvas_draw_text_in_rect(local->font, min_text, &text_region, EGUI_ALIGN_LEFT, local->muted_text_color, alpha);
}

static void egui_view_candlestick_chart_draw_range_highlight(
        egui_view_candlestick_chart_t *local,
        egui_region_t chart_region,
        uint8_t item_count,
        uint8_t max_range_index,
        uint8_t range_value,
        egui_alpha_t alpha)
{
    egui_dim_t x0;
    egui_dim_t x1;
    egui_color_t highlight_color;

    if (item_count == 0 || range_value == 0 || max_range_index >= item_count)
    {
        return;
    }
    if (chart_region.size.width < 120)
    {
        return;
    }
    x0 = chart_region.location.x + (egui_dim_t)(((int32_t)chart_region.size.width * max_range_index) / item_count);
    x1 = chart_region.location.x + (egui_dim_t)(((int32_t)chart_region.size.width * (max_range_index + 1)) / item_count);
    if (x1 <= x0 + 3)
    {
        return;
    }
    highlight_color = egui_rgb_mix(local->surface_color, local->text_color, EGUI_ALPHA_20);
    egui_canvas_draw_round_rectangle_fill(
            x0 + 1,
            chart_region.location.y + 1,
            x1 - x0 - 2,
            chart_region.size.height - 2,
            2,
            highlight_color,
            egui_color_alpha_mix(alpha, EGUI_ALPHA_30));
}

void egui_view_candlestick_chart_set_item_labels(egui_view_t *self, const char **item_labels, uint8_t item_count)
{
    EGUI_LOCAL_INIT(egui_view_candlestick_chart_t);
    local->item_labels = item_labels;
    local->item_count = egui_view_candlestick_chart_clamp_item_count(item_count);
    if (local->current_value_set >= local->value_set_count)
    {
        local->current_value_set = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_candlestick_chart_set_value_set(
        egui_view_t *self,
        uint8_t set_index,
        const uint8_t *open_values,
        const uint8_t *high_values,
        const uint8_t *low_values,
        const uint8_t *close_values)
{
    EGUI_LOCAL_INIT(egui_view_candlestick_chart_t);
    if (set_index >= EGUI_VIEW_CANDLESTICK_CHART_MAX_VALUE_SETS)
    {
        return;
    }
    local->open_sets[set_index] = open_values;
    local->high_sets[set_index] = high_values;
    local->low_sets[set_index] = low_values;
    local->close_sets[set_index] = close_values;
    if (local->value_set_count <= set_index)
    {
        local->value_set_count = set_index + 1;
    }
    local->value_set_count = egui_view_candlestick_chart_clamp_value_set_count(local->value_set_count);
    egui_view_invalidate(self);
}

void egui_view_candlestick_chart_set_current_value_set(egui_view_t *self, uint8_t set_index)
{
    EGUI_LOCAL_INIT(egui_view_candlestick_chart_t);
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

uint8_t egui_view_candlestick_chart_get_current_value_set(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_candlestick_chart_t);
    return local->current_value_set;
}

void egui_view_candlestick_chart_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_candlestick_chart_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_candlestick_chart_set_show_labels(egui_view_t *self, uint8_t show_labels)
{
    EGUI_LOCAL_INIT(egui_view_candlestick_chart_t);
    local->show_labels = show_labels ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_candlestick_chart_set_show_header(egui_view_t *self, uint8_t show_header)
{
    EGUI_LOCAL_INIT(egui_view_candlestick_chart_t);
    local->show_header = show_header ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_candlestick_chart_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t border_color,
        egui_color_t rise_color,
        egui_color_t fall_color,
        egui_color_t neutral_color,
        egui_color_t text_color,
        egui_color_t muted_text_color)
{
    EGUI_LOCAL_INIT(egui_view_candlestick_chart_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->rise_color = rise_color;
    local->fall_color = fall_color;
    local->neutral_color = neutral_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    egui_view_invalidate(self);
}

static void egui_view_candlestick_chart_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_candlestick_chart_t);
    egui_region_t region;
    egui_region_t content_region;
    egui_region_t label_region;
    egui_region_t header_region;
    egui_region_t text_region;
    const uint8_t *open_values;
    const uint8_t *high_values;
    const uint8_t *low_values;
    const uint8_t *close_values;
    const char *header_text;
    const char *footer_text;
    egui_color_t panel_color;
    egui_color_t shadow_color;
    uint8_t min_value;
    uint8_t max_value;
    uint8_t max_range_value;
    uint8_t max_range_index;
    uint8_t is_enabled;
    uint8_t i;
    char compact_badge[3] = {'L', 'K', '\0'};

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->item_count == 0)
    {
        return;
    }

    open_values = local->open_sets[local->current_value_set];
    high_values = local->high_sets[local->current_value_set];
    low_values = local->low_sets[local->current_value_set];
    close_values = local->close_sets[local->current_value_set];
    if (open_values == NULL || high_values == NULL || low_values == NULL || close_values == NULL)
    {
        return;
    }

    is_enabled = egui_view_get_enable(self) ? 1 : 0;
    panel_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_40);
    shadow_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_10);
    if (!is_enabled)
    {
        panel_color = egui_view_candlestick_chart_mix_disabled(panel_color);
        shadow_color = egui_view_candlestick_chart_mix_disabled(shadow_color);
    }
    if (CANDLE_PANEL_SHADOW_ALPHA > 0)
    {
        egui_canvas_draw_round_rectangle_fill(
                region.location.x + 1,
                region.location.y + 2,
                region.size.width,
                region.size.height,
                8,
                shadow_color,
                egui_color_alpha_mix(self->alpha, CANDLE_PANEL_SHADOW_ALPHA));
    }
    egui_canvas_draw_round_rectangle_fill(
            region.location.x,
            region.location.y,
            region.size.width,
            region.size.height,
            8,
            panel_color,
            egui_color_alpha_mix(self->alpha, CANDLE_PANEL_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(
            region.location.x,
            region.location.y,
            region.size.width,
            region.size.height,
            8,
            1,
            local->border_color,
            egui_color_alpha_mix(self->alpha, CANDLE_PANEL_BORDER_ALPHA));
    if (CANDLE_INNER_BORDER_ALPHA > 0)
    {
        egui_canvas_draw_round_rectangle(
                region.location.x + 2,
                region.location.y + 2,
                region.size.width - 4,
                region.size.height - 4,
                6,
                1,
                egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                egui_color_alpha_mix(self->alpha, CANDLE_INNER_BORDER_ALPHA));
    }

    content_region.location.x = region.location.x + 4;
    content_region.location.y = region.location.y + 4;
    content_region.size.width = region.size.width - 8;
    content_region.size.height = region.size.height - 8;

    if (!local->show_header)
    {
        if (is_enabled)
        {
            compact_badge[0] = (local->current_value_set == 0) ? 'A' : ((local->current_value_set == 1) ? 'B' : 'C');
            compact_badge[1] = '\0';
        }
        if (CANDLE_MINI_TOP_STRIP_ALPHA > 0)
        {
            egui_canvas_draw_round_rectangle_fill(
                    region.location.x + 12,
                    region.location.y + 6,
                    region.size.width - 20,
                    2,
                    1,
                    is_enabled ? local->text_color : local->muted_text_color,
                    egui_color_alpha_mix(self->alpha, CANDLE_MINI_TOP_STRIP_ALPHA));
        }
        header_region.location.x = content_region.location.x + (content_region.size.width - CANDLE_MINI_BADGE_WIDTH) / 2;
        header_region.location.y = content_region.location.y + 6;
        header_region.size.width = CANDLE_MINI_BADGE_WIDTH;
        header_region.size.height = 10;
        egui_canvas_draw_round_rectangle_fill(
                header_region.location.x,
                header_region.location.y,
                header_region.size.width,
                header_region.size.height,
                4,
                egui_rgb_mix(local->surface_color, local->border_color, EGUI_ALPHA_10),
                egui_color_alpha_mix(self->alpha, CANDLE_MINI_BADGE_FILL_ALPHA));
        egui_canvas_draw_round_rectangle(
                header_region.location.x,
                header_region.location.y,
                header_region.size.width,
                header_region.size.height,
                4,
                1,
                egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                egui_color_alpha_mix(self->alpha, CANDLE_MINI_BADGE_BORDER_ALPHA));
        egui_canvas_draw_text_in_rect(local->font, compact_badge, &header_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color, self->alpha);
        content_region.location.y += 12;
        content_region.size.height -= 12;
    }

    if (local->show_header)
    {
        egui_dim_t pill_w;

        header_text = egui_view_candlestick_chart_get_header_text(local, is_enabled);
        pill_w = 34 + (egui_dim_t)strlen(header_text) * 6;
        if (pill_w < CANDLE_HEADER_PILL_MIN_WIDTH)
        {
            pill_w = CANDLE_HEADER_PILL_MIN_WIDTH;
        }
        if (pill_w > content_region.size.width)
        {
            pill_w = content_region.size.width;
        }
        header_region.location.x = content_region.location.x + (content_region.size.width - pill_w) / 2;
        header_region.location.y = content_region.location.y;
        header_region.size.width = pill_w;
        header_region.size.height = 14;
        egui_canvas_draw_round_rectangle_fill(
                header_region.location.x,
                header_region.location.y,
                header_region.size.width,
                header_region.size.height,
                6,
                egui_rgb_mix(local->surface_color, local->border_color, EGUI_ALPHA_10),
                egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));
        egui_canvas_draw_round_rectangle(
                header_region.location.x,
                header_region.location.y,
                header_region.size.width,
                header_region.size.height,
                6,
                1,
                egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        egui_canvas_draw_text_in_rect(local->font, header_text, &header_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color, self->alpha);
        if (CANDLE_HEADER_LINE_ALPHA > 0)
        {
            egui_canvas_draw_line(
                    content_region.location.x + 5,
                    content_region.location.y + 17,
                    content_region.location.x + content_region.size.width - 6,
                    content_region.location.y + 17,
                    1,
                    local->border_color,
                    egui_color_alpha_mix(self->alpha, CANDLE_HEADER_LINE_ALPHA));
        }
        content_region.location.y += 20;
        content_region.size.height -= 20;
    }
    if (content_region.size.width <= 10 || content_region.size.height <= 10)
    {
        return;
    }

    label_region.location.x = content_region.location.x;
    label_region.location.y = content_region.location.y + content_region.size.height - 10;
    label_region.size.width = content_region.size.width;
    label_region.size.height = 10;
    if (!local->show_labels || content_region.size.height < 22)
    {
        label_region.size.height = 0;
    }
    else
    {
        content_region.size.height -= label_region.size.height;
    }
    if (local->show_header && content_region.size.height > 14)
    {
        content_region.size.height -= 14;
    }
    if (content_region.size.height <= 8)
    {
        return;
    }

    egui_view_candlestick_chart_find_bounds(open_values, high_values, low_values, close_values, local->item_count, &min_value, &max_value);
    max_range_index = egui_view_candlestick_chart_find_max_range_index(high_values, low_values, local->item_count, &max_range_value);
    if (local->show_labels)
    {
        egui_view_candlestick_chart_draw_reference_lines(local, content_region, self->alpha, is_enabled);
        egui_view_candlestick_chart_draw_range_highlight(local, content_region, local->item_count, max_range_index, max_range_value, self->alpha);
        egui_view_candlestick_chart_draw_bound_labels(local, content_region, min_value, max_value, self->alpha);
        egui_view_candlestick_chart_draw_close_path(local, close_values, local->item_count, min_value, max_value, content_region, self->alpha);
    }

    for (i = 0; i < local->item_count; i++)
    {
        egui_dim_t x0 = content_region.location.x + (egui_dim_t)(((int32_t)content_region.size.width * i) / local->item_count);
        egui_dim_t x1 = content_region.location.x + (egui_dim_t)(((int32_t)content_region.size.width * (i + 1)) / local->item_count);
        egui_view_candlestick_chart_draw_candle(
                local,
                open_values[i],
                high_values[i],
                low_values[i],
                close_values[i],
                (uint8_t)(local->show_labels && i == (uint8_t)(local->item_count - 1)),
                min_value,
                max_value,
                content_region.location.y + 1,
                content_region.size.height - 2,
                x0,
                x1,
                self->alpha,
                is_enabled);
    }
    if (!is_enabled)
    {
        if (CANDLE_DISABLED_OVERLAY_ALPHA > 0)
        {
            egui_canvas_draw_round_rectangle_fill(
                    content_region.location.x + 1,
                    content_region.location.y + 1,
                    content_region.size.width - 2,
                    content_region.size.height - 2,
                    4,
                    EGUI_COLOR_BLACK,
                    egui_color_alpha_mix(self->alpha, CANDLE_DISABLED_OVERLAY_ALPHA));
        }
        if (CANDLE_DISABLED_CROSS_ALPHA > 0)
        {
            egui_canvas_draw_line(
                    content_region.location.x + 2,
                    content_region.location.y + 2,
                    content_region.location.x + content_region.size.width - 3,
                    content_region.location.y + content_region.size.height - 3,
                    1,
                    local->muted_text_color,
                    egui_color_alpha_mix(self->alpha, CANDLE_DISABLED_CROSS_ALPHA));
            egui_canvas_draw_line(
                    content_region.location.x + 2,
                    content_region.location.y + content_region.size.height - 3,
                    content_region.location.x + content_region.size.width - 3,
                    content_region.location.y + 2,
                    1,
                    local->muted_text_color,
                    egui_color_alpha_mix(self->alpha, CANDLE_DISABLED_CROSS_ALPHA));
        }
    }

    if (local->show_labels && label_region.size.height > 0)
    {
        egui_view_candlestick_chart_draw_labels(local, label_region, self->alpha);
    }

    if (local->show_header)
    {
        egui_dim_t footer_x;

        footer_text = egui_view_candlestick_chart_get_footer_text(open_values, close_values, local->item_count);
        footer_x = content_region.location.x + (content_region.size.width - CANDLE_FOOTER_WIDTH) / 2;
        text_region.location.x = content_region.location.x;
        text_region.location.y = content_region.location.y + content_region.size.height + 3;
        text_region.size.width = content_region.size.width;
        text_region.size.height = 11;
        egui_canvas_draw_round_rectangle_fill(
                footer_x,
                content_region.location.y + content_region.size.height + 2,
                CANDLE_FOOTER_WIDTH,
                11,
                5,
                panel_color,
                egui_color_alpha_mix(self->alpha, CANDLE_FOOTER_FILL_ALPHA));
        egui_canvas_draw_round_rectangle(
                footer_x,
                content_region.location.y + content_region.size.height + 2,
                CANDLE_FOOTER_WIDTH,
                11,
                5,
                1,
                egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                egui_color_alpha_mix(self->alpha, CANDLE_FOOTER_BORDER_ALPHA));
        egui_canvas_draw_text_in_rect(local->font, footer_text, &text_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color, self->alpha);
    }
    else if (CANDLE_MINI_BOTTOM_STRIP_ALPHA > 0)
    {
        egui_canvas_draw_round_rectangle_fill(
                content_region.location.x + 8,
                region.location.y + region.size.height - 8,
                content_region.size.width - 14,
                2,
                1,
                is_enabled ? local->text_color : local->muted_text_color,
                egui_color_alpha_mix(self->alpha, CANDLE_MINI_BOTTOM_STRIP_ALPHA));
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_candlestick_chart_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_candlestick_chart_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_candlestick_chart_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_candlestick_chart_t);
    uint8_t i;
    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_candlestick_chart_t);
    egui_view_set_padding_all(self, 2);

    local->item_labels = NULL;
    for (i = 0; i < EGUI_VIEW_CANDLESTICK_CHART_MAX_VALUE_SETS; i++)
    {
        local->open_sets[i] = NULL;
        local->high_sets[i] = NULL;
        local->low_sets[i] = NULL;
        local->close_sets[i] = NULL;
    }
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x0E1624);
    local->border_color = EGUI_COLOR_HEX(0x4A5A70);
    local->rise_color = EGUI_COLOR_HEX(0x22C55E);
    local->fall_color = EGUI_COLOR_HEX(0xEF4444);
    local->neutral_color = EGUI_COLOR_HEX(0x94A3B8);
    local->text_color = EGUI_COLOR_HEX(0xE2E8F0);
    local->muted_text_color = EGUI_COLOR_HEX(0x94A3B8);
    local->item_count = 0;
    local->value_set_count = 0;
    local->current_value_set = 0;
    local->show_labels = 1;
    local->show_header = 1;
}
